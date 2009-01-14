/**
 *   This file is part of Bluetooth for Microsoft Device Emulator
 * 
 *   Copyright (C) 2008-2009 Dmitry Klionsky <dm.klionsky@gmail.com>
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

#ifndef __DEVICE_DEBUG_H__
#define __DEVICE_DEBUG_H__

#include <windows.h>
#include <bt_debug.h>
#include "bt_tdbg.h" // !!! modified version used !!!

DECLARE_DEBUG_VARS();

LPCTSTR szDebugKey = _T("Software\\Microsoft\\Bluetooth\\Debug");

#ifdef DEBUG
#define TRACE
#endif

#ifdef TRACE
#include "DebugOutput.h"
#define TRACE_HEADER L"BthEmul: "
#endif

#ifdef TRACE
#define TRACE0( msg ) \
   DebugOutput( TRACE_HEADER ).Print( L##msg );

#define TRACE1( msg, param ) \
   DebugOutput( TRACE_HEADER ).Print( L##msg, param );

#define TRACE2( msg, param1, param2 ) \
   DebugOutput( TRACE_HEADER ).Print( L##msg, param1, param2 );

#define TRACE3( msg, param1, param2, param3 ) \
   DebugOutput( TRACE_HEADER ).Print( L##msg, param1, param2, param3 );
#else
#define TRACE0( msg )
#define TRACE1( msg, param )
#define TRACE2( msg, param1, param2 )
#define TRACE3( msg, param1, param2, param3 )
#endif

/**
@func BOOL | SetLogLevel | Sets logging level.
@parm DWORD | dwLevel | Desired logging level. Currently only 0 - logging off, 0xFFFFFFFF - logging on implemented.
@rdesc The function should return a value that indicates its success or failure. 
*/
BOOL SetLogLevel( DWORD dwLevel )
{
   //DebugOut( DEBUG_OUTPUT, L"+SetLogLevel dwLevel: 0x%08x\n", dwLevel );

   BOOL bRet = FALSE;

   HKEY hk;
   DWORD dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szDebugKey, 0, KEY_WRITE, &hk );
   if ( ERROR_SUCCESS == dwStatus ) {
      DWORD dwType = REG_DWORD;
      DWORD dwSize = sizeof( dwLevel );
      dwStatus = RegSetValueEx( hk, _T("Mask"), NULL, dwType, (LPBYTE)&dwLevel, dwSize );
      if ( ERROR_SUCCESS == dwStatus ) {
         bRet = TRUE;
      } else {
         TRACE3( "RegSetValueEx %s %s ret: 0x%08x", szDebugKey, _T("Mask"), dwStatus );
         DebugOut( DEBUG_OUTPUT, L"RegSetValueEx %s %s ret: 0x%08x\n", szDebugKey, _T("Mask"), dwStatus );
      }
      RegCloseKey( hk );
   } else {
      TRACE2( "RegOpenKeyEx %s ret: 0x%08x", szDebugKey, dwStatus );
      DebugOut( DEBUG_OUTPUT, L"RegOpenKeyEx %s ret: 0x%08x\n", szDebugKey, dwStatus );
   }	

   //IFDBG( DebugOut( DEBUG_OUTPUT, L"-SetLogLevel ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | GetLogLevel | Gets logging level.
@parm DWORD& | dwLevel | Currently installed logging level. Currently only 0 - logging off, 0xFFFFFFFF - logging on implemented.
@rdesc The function should return a value that indicates its success or failure.
*/
BOOL GetLogLevel( DWORD& dwLevel )
{
   //DebugOut( DEBUG_OUTPUT, L"+GetLogLevel\n" );

   BOOL bRet = FALSE;
   dwLevel = 0;

   HKEY hk;
   DWORD dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szDebugKey, 0, KEY_READ, &hk );
   if ( ERROR_SUCCESS == dwStatus ) {
      DWORD dwType = 0;
      DWORD dwData = 0;
      DWORD dwSize = sizeof( dwData );
      dwStatus = RegQueryValueEx( hk, _T("Mask"), NULL, &dwType, (LPBYTE)&dwData, &dwSize );
      if ( ( ERROR_SUCCESS == dwStatus ) &&
         ( dwType == REG_DWORD ) && ( dwSize == sizeof( dwData ) ) ) {
            dwLevel = dwData;
            bRet = TRUE;
      } else {
         TRACE3( "RegQueryValueEx %s %s ret: 0x%08x", szDebugKey, _T("Mask"), dwStatus );
         DebugOut( DEBUG_OUTPUT, L"RegQueryValueEx %s %s ret: 0x%08x\n", szDebugKey, _T("Mask"), dwStatus );
      }
      RegCloseKey( hk );
   } else {
      TRACE2( "RegOpenKeyEx %s ret: 0x%08x", szDebugKey, dwStatus );
      DebugOut( DEBUG_OUTPUT, L"RegOpenKeyEx %s ret: 0x%08x\n", szDebugKey, dwStatus );
   }

   //DebugOut( DEBUG_OUTPUT, L"-GetLogLevel dwLevel: 0x%08x ret: %d\n", dwLevel, bRet );
   return bRet;
}

#ifdef IFDBG
#undef IFDBG
#define IFDBG( out )   \
{  DWORD dwLevel = 0;   \
   if ( GetLogLevel( dwLevel ) && 0xFFFFFFFF == dwLevel ) { out; } \
}
#endif

#endif