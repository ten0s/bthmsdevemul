//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

    bt_tdbg.h

Abstract:  

    Debug subsystem for bluetooth transports
    
Functions:


Notes:

--*/

#ifndef _DBGUTIL_H_
#define _DBGUTIL_H_

#include <regext.h>  // RegistryNotifyCallback, RegistryCloseNotification
#pragma comment(lib, "aygshell.lib")

#if defined (LOCALDBG)
#define DECLARE_DEBUG_VARS()	\
	HANDLE hDebugOutput = INVALID_HANDLE_VALUE;	\
	int    cDebugOutput = 0;					\
	CRITICAL_SECTION csDebugOutput;                                      \
   HREGNOTIFY hRegNotify = NULL;                                        \
   LPCTSTR REG_DEBUG_KEY = _T("Software\\Microsoft\\Bluetooth\\Debug"); \
   LPCTSTR REG_VALUE_KEY = _T("Mask");                                  \
   DWORD dwDebugLevel = 0x0;

#if ! defined (LOCALDBG_MAX)
#define LOCALDBG_MAX	OUTPUT_FILE_MAX
#endif

extern HANDLE hDebugOutput;
extern int    cDebugOutput;
extern CRITICAL_SECTION csDebugOutput;
extern HREGNOTIFY hRegNotify;
extern LPCTSTR REG_DEBUG_KEY;
extern LPCTSTR REG_VALUE_KEY;
extern DWORD dwDebugLevel;

// Forward declaration.
BOOL GetLogLevel( DWORD& dwLevel );
// Forward declaration the call back function for Registry Notifications.
void RegistryNotifyCallbackFunc(HREGNOTIFY hNotify, DWORD dwUserData, const PBYTE pData, const UINT cbData);

inline void __DebugOut (unsigned int cMask, WCHAR *lpszFormat, ...)
{
	EnterCriticalSection (&csDebugOutput);

	__try {
		if (hDebugOutput != INVALID_HANDLE_VALUE) {
			va_list args;

			va_start (args, lpszFormat);
			WCHAR szBigLine[256];
         int iRes = wvsprintf (szBigLine, lpszFormat, args);
         va_end (args);

         // added timing info.
         WCHAR szTotalLine[256+20];
         iRes = wsprintf (szTotalLine, L"%lu: %s", GetTickCount(), szBigLine);

			char szMB[256+20];
			DWORD cBytes = WideCharToMultiByte (CP_ACP, 0, szTotalLine, -1, szMB, sizeof(szMB), NULL, NULL) - 1;

			if (cBytes > 0)	{
				DWORD dwOffset = SetFilePointer (hDebugOutput, 0, NULL, FILE_END);
				DWORD dwWritten = 0;

				WriteFile (hDebugOutput, szMB, cBytes, &dwWritten, NULL);

				if (((int)dwOffset > 0) && (dwOffset > LOCALDBG_MAX)) {
					wsprintf (szBigLine, L"\\temp\\btd_" LOCALDBG L"_%d.txt", (++cDebugOutput) & 1);
					CloseHandle (hDebugOutput);
					hDebugOutput = CreateFile (szBigLine, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				}
			}
		}
	} __except (1) {
	}

	LeaveCriticalSection (&csDebugOutput);
}

inline void __DumpBuff (unsigned int cMask, unsigned char *lpBuffer, unsigned int cBuffer) {
	WCHAR szLine[5 + 7 + 2 + 4 * 8];

	for (int i = 0 ; i < (int)cBuffer ; i += 8) {
		int bpr = cBuffer - i;
		if (bpr > 8)
			bpr = 8;

		wsprintf (szLine, L"%04x ", i);
		WCHAR *p = szLine + wcslen (szLine);

      int j;
		for ( j = 0 ; j < bpr ; ++j) {
			WCHAR c = (lpBuffer[i + j] >> 4) & 0xf;
			if (c > 9) c += L'a' - 10; else c += L'0';
			*p++ = c;
			c = lpBuffer[i + j] & 0xf;
			if (c > 9) c += L'a' - 10; else c += L'0';
			*p++ = c;
			*p++ = L' ';
		}

		for ( ; j < 8 ; ++j) {
			*p++ = L' ';
			*p++ = L' ';
			*p++ = L' ';
		}

		*p++ = L' ';
		*p++ = L' ';
		*p++ = L' ';
		*p++ = L'|';
		*p++ = L' ';
		*p++ = L' ';
		*p++ = L' ';

		for (j = 0 ; j < bpr ; ++j) {
			WCHAR c = lpBuffer[i + j];
			if ((c < L' ') || (c >= 127))
				c = L'.';

			*p++ = c;
		}

		for ( ; j < 8 ; ++j) {
			*p++ = L' ';
		}

		*p++ = L'\n';
		*p++ = L'\0';

		__DebugOut (0xffffffff, L"%s", szLine);
	}
}

inline void DebugInit ()
{
	InitializeCriticalSection (&csDebugOutput);

	DeleteFile (L"\\temp\\btd_" LOCALDBG L"_0.txt");
	DeleteFile (L"\\temp\\btd_" LOCALDBG L"_1.txt");

	cDebugOutput = 0;
	hDebugOutput = CreateFile (L"\\temp\\btd_" LOCALDBG L"_0.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hDebugOutput == INVALID_HANDLE_VALUE)
		return;

	SYSTEMTIME st;
	
	GetLocalTime (&st);
	__DebugOut (0xffffffff, L"Debug log started @ %02d:%02d:%02d %02d/%02d/%d\n",
		st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear);

   // determine the initial value.
   GetLogLevel(dwDebugLevel);

   // subscribe to registry notifications
   HRESULT hr = RegistryNotifyCallback(HKEY_LOCAL_MACHINE, 
      REG_DEBUG_KEY,
      REG_VALUE_KEY,
      RegistryNotifyCallbackFunc,
      0L,
      NULL,
      &hRegNotify);
}

inline void DebugDeInit()
{
	if (hDebugOutput == INVALID_HANDLE_VALUE)
		return;

	SYSTEMTIME st;
	GetLocalTime (&st);
	__DebugOut (0xffffffff, L"Debug log ended @ %02d:%02d:%02d %02d/%02d/%d\n",
		st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear);

	CloseHandle (hDebugOutput);
	hDebugOutput = INVALID_HANDLE_VALUE;
	cDebugOutput = 0;

   // unsubscribe the registry notifications.
   RegistryCloseNotification(hRegNotify);

	DeleteCriticalSection (&csDebugOutput);
}

#define DebugOut __DebugOut
#define DumpBuff __DumpBuff
#undef IFDBG
#define IFDBG(c) c

#else	// LOCALDBG

#if (defined (DEBUG) || defined(_DEBUG) || defined (RETAILLOG)) && defined (UNDER_CE)
typedef void (*DEBUGOUT)(unsigned int cMask, WCHAR *lpszFormat, ...);
typedef void (*DUMPBUFF)(unsigned int cMask, unsigned char *lpBuffer, unsigned int cBuffer);

extern DEBUGOUT		pfnDebugOut;
extern DUMPBUFF		pfnDumpBuff;
extern HINSTANCE	ghInstBTD;
extern DWORD		gdwRefCount;

#define DECLARE_DEBUG_VARS()	\
	DEBUGOUT	pfnDebugOut = NULL;								\
	DUMPBUFF	pfnDumpBuff = NULL;								\
	HINSTANCE	ghInstBTD = NULL;								\
	DWORD		gdwRefCount = 0;

inline void DebugInit()
{
	gdwRefCount++;
	if (!ghInstBTD)
	{
		ghInstBTD = LoadLibrary(L"btd.dll");
		if (ghInstBTD)
		{
			pfnDebugOut = (DEBUGOUT )GetProcAddress(ghInstBTD, L"DebugOut");
			pfnDumpBuff = (DUMPBUFF )GetProcAddress(ghInstBTD, L"DumpBuff");
		}
	}
}

inline void DebugDeInit()
{
	gdwRefCount--;
	if (gdwRefCount == 0 && ghInstBTD)
	{
		FreeLibrary(ghInstBTD);
		ghInstBTD = NULL;
		pfnDebugOut = NULL;
		pfnDumpBuff = NULL;
	}
}

#define DebugOut (*pfnDebugOut)
#define DumpBuff (*pfnDumpBuff)

#else		// (defined (DEBUG) || defined(_DEBUG) || defined (RETAILLOG)) && defined (UNDER_CE)

#define DECLARE_DEBUG_VARS()

inline void DebugInit() {}
inline void DebugDeInit() {}


#endif		// (defined (DEBUG) || defined(_DEBUG) || defined (RETAILLOG)) && defined (UNDER_CE)

#endif		// LOCALDBG

#endif		// _DBGUTIL_H_

