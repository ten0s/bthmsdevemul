/**
 *   This file is part of Bluetooth for Microsoft Device Emulator
 * 
 *   Copyright (C) 2008-2009 Dmitry Klionsky aka ten0s <dm.klionsky@gmail.com>
 *   
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define LOCALDBG            L"bthemul"
#include "DeviceDebug.h"

#include <bt_buffer.h>
#include <bt_hcip.h>
#include <Pkfuncs.h>

static HANDLE g_hFile = INVALID_HANDLE_VALUE;
static HANDLE g_hWriteEvent = NULL;
static HANDLE g_hReadEvent = NULL;
static HCI_TransportCallback g_pfCallback = NULL;

#define PACKET_SIZE_R       (4096)
#define PACKET_SIZE_W       (4096)

#define DEFAULT_BTE_NAME    L"BTE1:"

#define READ_BUFFER_HEADER    4
#define READ_BUFFER_TRAILER   0
#define WRITE_BUFFER_HEADER   4
#define WRITE_BUFFER_TRAILER  0

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

/**
@func BOOL | DllMain | This function is an optional method of entry into a DLL.
@parm HANDLE | hModule | Handle to the DLL. 
@parm DWORD | ul_reason_for_call | Specifies a flag indicating why the DLL entry-point function is being called. 
@parm LPVOID | lpReserved | Specifies further aspects of DLL initialization and cleanup. 
@rdesc When the system calls the DllMain function with the DLL_PROCESS_ATTACH value, the function returns TRUE if it succeeds or FALSE if initialization fails.
If the return value is FALSE when DllMain is called because the process uses the LoadLibrary function, LoadLibrary returns NULL.
If the return value is FALSE when DllMain is called during process initialization, the process terminates with an error.
To get extended error information, call GetLastError.
When the system calls the DllMain function with a value other than DLL_PROCESS_ATTACH, the return value is ignored. 
*/
BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
	switch ( ul_reason_for_call ) {
	case DLL_PROCESS_ATTACH:
		DebugInit();
		IFDBG( DebugOut( DEBUG_OUTPUT, L"DllMain: DLL_PROCESS_ATTACH\n" ) );
		DisableThreadLibraryCalls( (HMODULE)hModule );
		break;
	case DLL_PROCESS_DETACH:
		IFDBG( DebugOut( DEBUG_OUTPUT, L"DllMain: DLL_PROCESS_DETACH\n" ) );
		DebugDeInit();		
		break;
	}
	return TRUE;
}

/**
@func int | HCI_SetCallback | This function is called by HCI to set the callback function to indicate device insertion/removal notifications.
@parm HCI_TransportCallback | pfCallback | Pointer to the HCI transport callback.
@rdesc Ignored.
@remark Routine exported by a transport driver.  
*/
int HCI_SetCallback( HCI_TransportCallback pfCallback ) 
{
	IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_SetCallback\n" ) );

	int nRet = ERROR_SUCCESS;

	g_pfCallback = pfCallback;

	IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_SetCallback ret: %d\n", nRet ) );
	return nRet;
}

/**
@func int | HCI_ReadHciParameters | This function is called by HCI to read the current transport driver parameters.
@parm HCI_PARAMETERS* | pParms | Pointer to the HCI parameters structure.
@rdesc Returns TRUE on successful completion. FALSE if error has occurred. If this function returns FALSE, the stack interface to hardware will immediately be brought  down by calling HCI_CloseConnection. 
@remark Routine exported by a transport driver.  
*/
int HCI_ReadHciParameters( HCI_PARAMETERS* pParms )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_ReadHciParameters\n" ) );
   
   int nRet = TRUE;

   if( pParms->uiSize < sizeof( *pParms ) ) {
	   nRet = FALSE;
	   IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadHciParameters ret: %d\n", nRet ) );
      return nRet;
   }

    memset( pParms, 0, sizeof( *pParms ) );

    pParms->uiSize                  = sizeof( *pParms) ;
    pParms->fInterfaceVersion       = HCI_INTERFACE_VERSION_1_1;
    pParms->iMaxSizeRead            = PACKET_SIZE_R;
    pParms->iMaxSizeWrite           = PACKET_SIZE_W;
    pParms->fHardwareVersion        = HCI_HARDWARE_VERSION_V_1_0_A;
    pParms->uiWriteTimeout          = HCI_DEFAULT_WRITE_TIMEOUT;
    pParms->uiDriftFactor           = HCI_DEFAULT_DRIFT;
    pParms->iReadBufferHeader       = READ_BUFFER_HEADER;
    pParms->iReadBufferTrailer      = READ_BUFFER_TRAILER;
    pParms->iWriteBufferHeader      = WRITE_BUFFER_HEADER;
    pParms->iWriteBufferTrailer     = WRITE_BUFFER_TRAILER;
    pParms->uiFlags                 = 0;

    HKEY hk;
     if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Bluetooth\\hci", 0, KEY_READ, &hk ) == ERROR_SUCCESS ) { 
        DWORD dwType = 0;
        DWORD dwData = 0;
        DWORD dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"InterfaceVersion", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->fInterfaceVersion = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"MaxSizeRead", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iMaxSizeRead = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"MaxSizeWrite", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iMaxSizeWrite = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"ReadBufferHeader", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iReadBufferHeader = dwData;
        
        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"ReadBufferTrailer", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iReadBufferTrailer = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"WriteBufferHeader", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iWriteBufferHeader = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"WriteBufferTrailer", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iWriteBufferTrailer = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"Flags", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->uiFlags = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"HardwareVersion", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->fHardwareVersion = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"ResetDelay", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
            ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
            pParms->uiResetDelay = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"WriteTimeout", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
            ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
            pParms->uiWriteTimeout = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"DriftFactor", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
            ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
            pParms->uiDriftFactor = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"ScoWriteLowNumPackets", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iScoWriteLowNumPackets = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"ScoWriteNumPackets", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iScoWriteNumPackets = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"ScoWritePacketSize", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iScoWritePacketSize = dwData;

        dwData = 0;
        dwSize = sizeof( dwData );
        if ( ( RegQueryValueEx( hk, L"ScoSampleSize", NULL, &dwType, (LPBYTE)&dwData, &dwSize ) == ERROR_SUCCESS ) &&
           ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) && dwData )
           pParms->iScoSampleSize = dwData;

        RegCloseKey( hk );
    }

	IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI parameters:\n") );
	IFDBG( DumpBuff( DEBUG_OUTPUT, (unsigned char*)pParms, sizeof( HCI_PARAMETERS ) ) );

	IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadHciParameters ret: %d\n", nRet ) );
	return nRet;
}

/**
@func int | HCI_StartHardware | This function is called by HCI to start the hardware. In this case we indicate a device up notification so HCI will proceed with a call to HCI_OpenConnection.
@rdesc Ignored.
@remark Routine exported by a transport driver.  
*/
int HCI_StartHardware() 
{
    IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_StartHardware\n" ) );
    
    if ( g_hFile != INVALID_HANDLE_VALUE ) {
        IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_StartHardware (already started)\n" ) );
        return TRUE;
    }

    if ( !g_pfCallback ) {
        IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_StartHardware (not registered)\n" ) );
        return FALSE;
    }

    IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_StartHardware\n" ) );
    return ERROR_SUCCESS == g_pfCallback( DEVICE_UP, NULL );
}

/**
@func int | HCI_StopHardware | This function is called by HCI to stop the hardware.  In this case we indicate a device up notification so HCI will proceed with a call to HCI_CloseConnection.
@rdesc Ignored.
@remark Routine exported by a transport driver.  
*/
int HCI_StopHardware() 
{
    IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_StopHardware\n" ) );
    
    if ( g_hFile == INVALID_HANDLE_VALUE ) {
        IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_StopHardware (already stopped)\n" ) );
        return TRUE;
    }

    if ( !g_pfCallback ) {
        IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_StopHardware (not registered)\n" ) );
        return FALSE;
    }

    IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_StopHardware\n" ) );
    return ERROR_SUCCESS == g_pfCallback( DEVICE_DOWN, NULL );
}

/**
@func int | HCI_OpenConnection | This function is called by HCI to open a connection to a transport driver.  This function first sees if there is a PnP device present and if not tries to open a built-in driver.
@rdesc TRUE indicates that the transport is active. When this condition occurs, the stack calls the HCI_ReadHciParameters function. 
@remark Routine exported by a transport driver.  
*/
int HCI_OpenConnection() 
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_OpenConnection\n" ) );

   // sleep a little. see async.cpp::Initialize
   Sleep( 500 );

   int nRet = TRUE;

   if ( g_hFile != INVALID_HANDLE_VALUE ) {
		nRet = FALSE;
		IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_OpenConnection ret: %d\n", nRet ) );
		return nRet;
   }

   WCHAR szPortName[_MAX_PATH];
   wcscpy( szPortName, DEFAULT_BTE_NAME );

   DWORD dwBaud = 115200;

   HKEY hk;
   if( ERROR_SUCCESS == RegOpenKeyEx ( HKEY_LOCAL_MACHINE, L"software\\microsoft\\bluetooth\\hci", 0, KEY_READ, &hk ) ) {
      DWORD dwType;
      DWORD dwSize = sizeof(szPortName);
      if( ( ERROR_SUCCESS == RegQueryValueEx( hk, L"Name", NULL, &dwType, (LPBYTE)szPortName, &dwSize ) ) &&
         ( dwType == REG_SZ ) && ( dwSize > 0 ) && ( dwSize < _MAX_PATH ) )
         ;
      else
         wcscpy( szPortName, DEFAULT_BTE_NAME );

      dwSize = sizeof( dwBaud );
      if ( ( ERROR_SUCCESS == RegQueryValueEx( hk, L"baud", NULL, &dwType, (LPBYTE)&dwBaud, &dwSize ) ) &&
         ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwBaud ) ) )
         ;
      else
         dwBaud = 115200;      

      RegCloseKey( hk );
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"Opening port %s (rate %d) for I/O with unit\n", szPortName, dwBaud ) );

   g_hFile = CreateFile( szPortName,
      GENERIC_READ | GENERIC_WRITE,
      0,                            // comm devices must be opened w/exclusive-access
      NULL,                         // no security attrs
      OPEN_EXISTING,                // comm devices must use OPEN_EXISTING
      FILE_ATTRIBUTE_NORMAL,        // overlapped I/O 
      NULL                          // hTemplate must be NULL for comm devices  
      );

   if ( g_hFile == INVALID_HANDLE_VALUE ) {
      nRet = FALSE;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"CreateFile ret: 0x%08x\n", GetLastError() ) );
	   IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_OpenConnection ret: %d\n", nRet ) );
	   return nRet;
   }

   // purge any information in the buffer
   if ( !PurgeComm( g_hFile, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ) {
      nRet = FALSE;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"PurgeComm ret: 0x%08x\n", GetLastError() ) );
      CloseHandle( g_hFile );
      g_hFile = INVALID_HANDLE_VALUE;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_OpenConnection ret: %d\n", nRet ) );
      return nRet;
   }

   /* 2009-04-13 Dm.Klionsky 
   Commented out the following calls. The default COM port values should be all rights.

   if ( !SetupComm( g_hFile, 20000, 20000 ) ) {
      // Ignore this failure
      IFDBG( DebugOut( DEBUG_OUTPUT, L"SetupComm ret: 0x%08x\n", GetLastError() ) );
   }

   COMMTIMEOUTS commTimeOuts = {0};
   commTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
   commTimeOuts.ReadTotalTimeoutMultiplier = 0;
   commTimeOuts.ReadTotalTimeoutConstant = 1000;
   commTimeOuts.WriteTotalTimeoutMultiplier = 0;
   commTimeOuts.WriteTotalTimeoutConstant = 1000;

   if ( !SetCommTimeouts( g_hFile, &commTimeOuts ) ) {
      nRet = FALSE;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"SetCommTimeouts ret: 0x%08x\n", GetLastError() ) );
      CloseHandle( g_hFile );
      g_hFile = INVALID_HANDLE_VALUE;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_OpenConnection ret: %d\n", nRet ) );
      return nRet;
   }

   DCB dcb = {0};
   dcb.DCBlength = sizeof( dcb );
   dcb.BaudRate = dwBaud;
   dcb.fBinary = TRUE;
   dcb.fParity = FALSE;
   dcb.fOutxCtsFlow = TRUE;
   dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
   dcb.fOutX = FALSE;
   dcb.fInX = FALSE;
   dcb.fOutxDsrFlow = FALSE;    
   dcb.fDsrSensitivity = FALSE;
   dcb.fDtrControl = DTR_CONTROL_ENABLE;
   dcb.fTXContinueOnXoff = FALSE;
   dcb.fErrorChar = FALSE;
   dcb.fNull = FALSE;
   dcb.fAbortOnError = TRUE;
   //    dcb.wReserved = 0;
   dcb.ByteSize = 8;
   dcb.Parity = NOPARITY;
   dcb.StopBits = ONESTOPBIT;
   dcb.XonChar = 0x11;
   dcb.XoffChar = 0x13;
   dcb.XonLim = 3000;
   dcb.XoffLim = 9000;

   if ( !SetCommState( g_hFile, &dcb ) ) {
      nRet = FALSE;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"SetCommState ret: 0x%08x\n", GetLastError() ) );
      CloseHandle( g_hFile );
      g_hFile = INVALID_HANDLE_VALUE;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_OpenConnection ret: %d\n", nRet ) );
      return nRet;
   }
   */

   g_hWriteEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
   g_hReadEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_OpenConnection ret: %d\n", nRet ) );
   return nRet;
}

/**
@func void | HCI_CloseConnection | This function is called by HCI to close a connection to a transport driver.
@rdesc
@remark Routine exported by a transport driver.  
*/
void HCI_CloseConnection() 
{
    IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_CloseConnection\n" ) );

    if ( g_hFile == INVALID_HANDLE_VALUE ) {
        IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_CloseConnection - not active\n" ) );
        return;
    }

    CloseHandle( g_hFile );
    CloseHandle( g_hWriteEvent );
    CloseHandle( g_hReadEvent );

    g_hFile = INVALID_HANDLE_VALUE;
    g_hWriteEvent = NULL;
    g_hReadEvent = NULL;    

    IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_CloseConnection\n" ) );

    return;
}

static BOOL WriteCommPort( unsigned char* pBuffer, unsigned int cSize )
{
   //IFDBG( DebugOut( DEBUG_OUTPUT, L"+WriteCommPort cSize: %d\n", cSize ) );

   DWORD dwFilledSoFar = 0;
   while ((int)dwFilledSoFar < cSize) {
      DWORD dwWrit = 0;
      if ( ( !WriteFile( g_hFile, &(pBuffer[dwFilledSoFar]), cSize - dwFilledSoFar, &dwWrit, NULL ) ) && ( dwWrit == 0 ) ) {
         if ( g_hFile != INVALID_HANDLE_VALUE ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"WriteFile ret: 0x%08x\n", GetLastError() ) );
         }
         IFDBG( DebugOut( DEBUG_OUTPUT, L"-WriteCommPort \n" ) );
			return FALSE;
      }

      dwFilledSoFar += dwWrit;
   }

   //IFDBG( DebugOut( DEBUG_OUTPUT, L"-WriteCommPort \n" ) );
	return TRUE;
}

static BOOL ReadCommPort( unsigned char* pBuffer, DWORD dwLen )
{
   //IFDBG( DebugOut( DEBUG_OUTPUT, L"+ReadCommPort dwLen: %d\n", dwLen ) );
	
   DWORD dwFilledSoFar = 0;
   while ( dwFilledSoFar < dwLen ) {
      DWORD dwRead = 0;
      if ( ( !ReadFile( g_hFile, &pBuffer[dwFilledSoFar], dwLen - dwFilledSoFar, &dwRead, NULL ) ) && ( dwRead == 0 ) ) {
         if ( g_hFile != INVALID_HANDLE_VALUE ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"ReadFile ret: 0x%08x\n", GetLastError() ) );
         }
         IFDBG( DebugOut( DEBUG_OUTPUT, L"-ReadCommPort \n" ) );
			return FALSE;
        }
    dwFilledSoFar += dwRead;
    }

   //IFDBG( DebugOut( DEBUG_OUTPUT, L"-ReadCommPort \n" ) );
	return TRUE;
}

/**
@func int | HCI_WritePacket | This function is called by HCI to write a packet. This function calls directly into the active transport driver.
@parm HCI_TYPE | peType | Defines the HCI type.
@parm BD_BUFFER* | pBuff | Pointer to the Bluetooth device buffer.
@rdesc TRUE on successful completion. FALSE if error has occurred. If this function returns FALSE, the stack interface to hardware will immediately be brought down by calling HCI_CloseConnection. 
@remark Routine exported by a transport driver.
*/
int HCI_WritePacket( HCI_TYPE eType, BD_BUFFER* pBuff ) 
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_WritePacket HCI_TYPE: 0x%02x len %d\n", eType, BufferTotal(pBuff) ) );
   IFDBG( DumpBuff( DEBUG_OUTPUT, pBuff->pBuffer + pBuff->cStart, BufferTotal( pBuff ) ) );

   ASSERT( !( pBuff->cStart & 0x3 ) );

   if ( ( (int)BufferTotal(pBuff) > PACKET_SIZE_W ) || ( pBuff->cStart < 1 ) ) {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"[EMUL] Packet too big (%d, should be <= %d), or no space for header!\n", BufferTotal( pBuff ), PACKET_SIZE_W ) );
      return FALSE;
   }

   if ( g_hFile == INVALID_HANDLE_VALUE ) {
      DebugOut( DEBUG_OUTPUT, L"HCI_WritePacket - not active\n" );
      return FALSE;
   }    

   pBuff->pBuffer[--pBuff->cStart] = (unsigned char)eType;

   if ( !WriteCommPort(pBuff->pBuffer + pBuff->cStart, BufferTotal( pBuff ) ) ) {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_WritePacket - writing failed\n" ) );
      return FALSE;
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_WritePacket : DONE type 0x%02x len %d\n", eType, BufferTotal(pBuff) ) );
   return TRUE;
}

/**
@func int | HCI_ReadPacket | This function is called by HCI to read a packet.  This function calls directly into the active transport driver.
@parm HCI_TYPE* | peType | Pointer to the HCI type. These values can be DATA_PACKET_ACL, DATA_PACKET_SCO, or EVENT_PACKET.
@parm BD_BUFFER* | pBuff | Pointer to the packet buffer.
@rdesc TRUE on successful completion. FALSE if error has occurred. If this function returns FALSE, the stack interface to hardware will immediately be brought down by calling HCI_CloseConnection. 
@remark Routine exported by a transport driver.  
*/
int HCI_ReadPacket( HCI_TYPE* peType, BD_BUFFER* pBuff ) 
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+HCI_ReadPacket\n" ) );

   if ( g_hFile == INVALID_HANDLE_VALUE ) {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket - not active\n" ) );
      return FALSE;
   }

   pBuff->cStart = 3;
   pBuff->cEnd = pBuff->cSize;

   if ( BufferTotal( pBuff ) < 257 ) {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket - failed buffer too small (%d bytes)\n", BufferTotal( pBuff ) ) );
      return FALSE;
   }

   if ( !ReadCommPort( pBuff->pBuffer + pBuff->cStart, 1 ) ) {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket - failed to read 1st byte\n" ) );
      return FALSE;
   }

   ++pBuff->cStart;

   switch ( pBuff->pBuffer[pBuff->cStart - 1] ) {   
      case COMMAND_PACKET:    // Command packet
         IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket - unexpected packet type: COMMAND_PACKET\n" ) );
         break;

      case DATA_PACKET_ACL:   // ACL packet
         if ( !ReadCommPort( pBuff->pBuffer + pBuff->cStart, 4 ) ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: no data\n" ) );
            return FALSE;
         }

         pBuff->cEnd = pBuff->cStart + 4 + (pBuff->pBuffer[pBuff->cStart + 2] | (pBuff->pBuffer[pBuff->cStart + 3] << 8 ) );

         if( pBuff->cEnd > pBuff->cSize) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: buffer too small\n" ) );
            return FALSE;
         }

         if ( !ReadCommPort( pBuff->pBuffer + pBuff->cStart + 4, BufferTotal( pBuff ) - 4 ) )
            return FALSE;

         *peType = DATA_PACKET_ACL;
         IFDBG( DebugOut( DEBUG_OUTPUT, L"Packet received HCI_TYPE: 0x%02x\n", *peType ) );
         IFDBG( DumpBuff( DEBUG_OUTPUT, pBuff->pBuffer + pBuff->cStart, BufferTotal( pBuff ) ) );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket\n" ) );
         return TRUE;

      case DATA_PACKET_SCO:   // SCO packet
         if ( !ReadCommPort( pBuff->pBuffer + pBuff->cStart, 3 ) ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: no data\n" ) );
            return FALSE;
         }

         pBuff->cEnd = pBuff->cStart + 3 + pBuff->pBuffer[pBuff->cStart + 2];

         if( pBuff->cEnd > pBuff->cSize ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: buffer too small\n" ) );
            return FALSE;
         }

         if( !ReadCommPort( pBuff->pBuffer + pBuff->cStart + 3, BufferTotal( pBuff ) - 3 ) ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: no data 2\n" ) );
            return FALSE;
         }

         *peType = DATA_PACKET_SCO;
         IFDBG( DebugOut( DEBUG_OUTPUT, L"Packet received HCI_TYPE: 0x%02x\n", *peType ) );
         IFDBG( DumpBuff( DEBUG_OUTPUT, pBuff->pBuffer + pBuff->cStart, BufferTotal( pBuff ) ) );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket\n" ) );
         return TRUE;

      case EVENT_PACKET:   // HCI Event         
         if ( !ReadCommPort( pBuff->pBuffer + pBuff->cStart, 2 ) ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: no data\n" ) );
            return FALSE;
         }

         pBuff->cEnd = pBuff->cStart + 2 + pBuff->pBuffer[pBuff->cStart + 1];

         if ( pBuff->cEnd > pBuff->cSize ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: buffer too small\n" ) );
            return FALSE;
         }

         if ( !ReadCommPort( pBuff->pBuffer + pBuff->cStart + 2, BufferTotal( pBuff ) - 2 ) ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"HCI_ReadPacket - failed: no data 2\n" ) );
            return FALSE;
         }

         *peType = EVENT_PACKET;
         IFDBG( DebugOut( DEBUG_OUTPUT, L"Packet received HCI_TYPE: 0x%02x\n", *peType ) );
         IFDBG( DumpBuff( DEBUG_OUTPUT, pBuff->pBuffer + pBuff->cStart, BufferTotal( pBuff ) ) );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket\n" ) );
         return TRUE;
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-HCI_ReadPacket - unknown packet type\n" ) );

   IFDBG( DebugOut( DEBUG_OUTPUT, L"Dumping the failing buffer...\n" ) );
   unsigned char buff[128];
   buff[0] = pBuff->pBuffer[pBuff->cStart - 1];
   int buff_size = 128;
   int buff_offset = 1;

   while ( buff_offset < buff_size ) {
      DWORD dwRead = 0;
      if ( ( !ReadFile( g_hFile, &buff[buff_offset], buff_size - buff_offset, &dwRead, NULL ) ) && ( dwRead == 0 ) ) {
         if( g_hFile != INVALID_HANDLE_VALUE ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"ReadFile ret: 0x%08x\n", GetLastError() ) );
         }
         break;
      }

      if ( dwRead == 0 )
         break;

      buff_offset += dwRead;
   }

   IFDBG( DumpBuff( DEBUG_OUTPUT, buff, buff_offset ) );

   return FALSE;
}