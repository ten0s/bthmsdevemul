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

#define LOCALDBG            L"BthEmulAgent"
#include "DeviceDebug.h"

#include "cecorecon.h"

#define REMOTE_AGENT
#include "MsgQueueDef.h"

#include <atlbase.h>
#include <statreg.h> // CRegObject

#include "..\..\..\bthemulcom\bthemulcom.h"

#define ERROR_CREATE_EVENT                   -1
#define ERROR_CREATE_WORKING_THREAD          -2
#define ERROR_CREATE_MSG_QUEUES              -3
#define ERROR_COPY_DRIVERS                   -4
#define ERROR_ACTIVATE_DRIVER                -5
#define ERROR_PRIVISION_DEVICE               -6
#define ERROR_CREATE_WATCHDOG_THREAD         -7
#define ERROR_CREATE_COMMAND                 -8

// Forward declaration of callback function that receives Command 
// Packets from the desktop via CommandTransport.ProcessCommand
// or CommandTransport.SendCommand
HRESULT STDAPICALLTYPE CommandCallback( DWORD dwCmd, const CCommandPacket* pCmdDataIn, CCommandPacket* pCmdDataOut );

// This object is contained in the Remote Tool SDK library
CDeviceRemoteTool g_DeviceRemoteTool;
CCommandPacket* g_pCmd = NULL;

void ReadDesktopWriteDevicePacket( DWORD dwCmd, const CCommandPacket* pCmdDataIn );
void ReadDeviceWriteDesktop();

BOOL CopyDriversToWindowsDir();
BOOL ActivateDriver();
BOOL DeactivateDriver();
BOOL ProvisionDeviceWithRgs( LPCWSTR szwFileName );
BOOL ProvisionDevice();
BOOL SendCommand( DWORD dwCmd, DWORD dwMsgId );
BOOL SendCommand( DWORD dwCmd, BYTE* pData, DWORD cbData );
BOOL Initialize();
BOOL Uninitialize();

#define WORKING_THREAD_SLEEP_TIMEOUT   100
DWORD WINAPI WorkingThread( LPVOID lpParam );

#define WATCHDOG_SLEEP_TIMEOUT         5000
#define WATCHDOG_MAX_COUNTER           (60000/WATCHDOG_SLEEP_TIMEOUT)
LONG g_lWatchDogCounter = WATCHDOG_MAX_COUNTER;
DWORD WINAPI WatchDogThread( LPVOID lpParam );

HANDLE g_hQuitEvent = NULL;
HANDLE g_hDevice = NULL;
HANDLE g_hWorkingThread = NULL;

LONG g_lIncomeMsgCounter = 0;
LONG g_lOutcomeMsgCounter = 0;
CRITICAL_SECTION g_criticalSection;

/**
@func int | WinMain | This function is called by the system as the initial entry point for Windows CE-based applications.
@parm HINSTANCE | hInstance | Handle to the current instance of the application. 
@parm HINSTANCE | hPrevInstance | Handle to the previous instance of the application. For a Win32-based application, this parameter is always NULL.
@parm LPTSTR | lpCmdLine | Pointer to a null-terminated string that specifies the command line for the application, excluding the program name.
@parm int | nCmdShow | Specifies how the window is to be shown.
@rdesc Returns zero on success or negative number if an error occurs.
*/
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
   DebugInit();
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+WinMain\n" ) );

   int nRet = 0;
   
   // initialize critical section used to synchronize access to SendCommand functions.
   InitializeCriticalSection( &g_criticalSection );
   
   // create quit event.
   g_hQuitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
   ASSERT( g_hQuitEvent );

   if ( !g_hQuitEvent ) {
      nRet = ERROR_CREATE_EVENT;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-WinMain ret: %d GetLastError: 0x%08x\n", nRet, GetLastError() ) );
      return nRet;
   }

   // create watch dog thread.
   HANDLE hWatchDogThread = CreateThread( NULL, 0, WatchDogThread, NULL, 0, NULL );
   ASSERT( hWatchDogThread );

   if ( !hWatchDogThread ) {
      nRet = ERROR_CREATE_WATCHDOG_THREAD;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-WinMain ret: %d GetLastError: 0x%08x\n", nRet, GetLastError() ) );
      return nRet;
   }

   // create command globally. using local command creating causes PushCommand 0x8007000d error after 113 cycles.
   HRESULT hr = g_DeviceRemoteTool.CreateCommand( &g_pCmd );
   ASSERT( SUCCEEDED( hr ) );

   if ( FAILED( hr ) ) {
      nRet = ERROR_CREATE_COMMAND;
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-WinMain ret: %d GetLastError: 0x%08x\n", nRet, hr ) );
      return nRet;
   }   
  
   // The lpCmdLine has special information in it.
   // Do not tamper with the values.
   g_DeviceRemoteTool.StartCommandHandler( lpCmdLine, g_DeviceRemoteTool.COMMANDHANDLER_ASYNC, CommandCallback );
   
   // wait for the exit command.
   WaitForSingleObject( g_hQuitEvent, INFINITE );

   // stop remote agent.
   g_DeviceRemoteTool.StopCommandHandler();

   // delete command.
   hr = g_DeviceRemoteTool.FreeCommand( g_pCmd );
   ASSERT( SUCCEEDED( hr ) );

   // uninitialize agent.
   Uninitialize();

   if ( g_hWorkingThread ) {
      PulseEvent( g_hQuitEvent );
      WaitForSingleObject( g_hWorkingThread, INFINITE );
      CloseHandle( g_hWorkingThread );
      g_hWorkingThread = NULL;
   }

   if ( g_hQuitEvent ) {
      CloseHandle( g_hQuitEvent );
      g_hQuitEvent = NULL;
   }

   DeleteCriticalSection( &g_criticalSection );

   IFDBG( DebugOut( DEBUG_OUTPUT, L"Total income message counter: %d\n", g_lIncomeMsgCounter ) );
   IFDBG( DebugOut( DEBUG_OUTPUT, L"Total outcome message counter: %d\n", g_lOutcomeMsgCounter ) );
   
   if ( nRet ) {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-WinMain ret: %d GetLastError: 0x%08x\n", nRet, GetLastError() ) );
   } else {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"-WinMain ret: %d\n", nRet ) );
   }
   
   DebugDeInit();
   return nRet;
}

/**
@func void | ReadDesktopWriteDevicePacket | Reads data from desktop and writes them to the message queue.
@parm DWORD | dwCmd | The commandId the desktop sent.
@parm const CCommandPacket* | pCmdDataIn | Command object with the data from the desktop.
*/
void ReadDesktopWriteDevicePacket( DWORD dwCmd, const CCommandPacket* pCmdDataIn )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+ReadDesktopWriteDevicePacket\n" ) );
      
   ASSERT( g_hWriteQueue );
   ASSERT( pCmdDataIn );
   if ( g_hWriteQueue && pCmdDataIn ) {
      CCommandPacket::DATATYPE dataType = CCommandPacket::DATATYPE_END;
      DWORD dwSize = 0; 
      if ( pCmdDataIn->GetNextParameterType( &dataType, &dwSize ) ) {
         unsigned char buffer[MSG_BUFFER_SIZE] = {0};
         
         switch( dwCmd ) {
            case MESSAGE_PACKET: {
               if ( dataType != CCommandPacket::DATATYPE_END && dataType == CCommandPacket::DATATYPE_DWORD && dwSize > 0 ) {
                  DWORD dwMsgId = 0;
                  if ( pCmdDataIn->GetParameterDWORD( &dwMsgId ) ) {
                     TRACE0( "Readed packet from desktop" );
                     IFDBG( DebugOut( DEBUG_OUTPUT, L"Data from desktop: dwCmd: 0x%08x dwMsgId: 0x%08x\n", dwCmd, dwMsgId ) );
                     Packet packet;
                     packet.writeInt( dwCmd );
                     packet.writeInt( dwMsgId );
                     dwSize = packet.serialize( buffer, packet.length() );                     
                  }
               }               
            }
            break;

            case HCI_DATA_PACKET: {
               if ( dataType != CCommandPacket::DATATYPE_END && dataType == CCommandPacket::DATATYPE_BYTES && dwSize > 0 && dwSize < MSG_BUFFER_SIZE ) {
                  if ( pCmdDataIn->GetParameterBytes( buffer, dwSize ) ) {
                     TRACE0( "Readed packet from desktop" );
                     IFDBG( DebugOut( DEBUG_OUTPUT, L"Data from desktop: dwCmd: 0x%08x\n", dwCmd ) );
                     IFDBG( DumpBuff( DEBUG_OUTPUT, buffer, dwSize ) );
                     Packet packet;
                     packet.writeInt( dwCmd );
                     packet.writeUCharArray( buffer, dwSize );
                     dwSize = packet.serialize( buffer, packet.length() );                     
                  }
               }
            }
            break;  

            default:
               IFDBG( DebugOut( DEBUG_OUTPUT, L"Unknown packet type: 0x%08x\n", dwCmd ) );
               break;
         }

         BOOL bRet = WriteMsgQueue( g_hWriteQueue, buffer, dwSize, MSG_QUEUE_WRITE_TIMEOUT, 0 );
         if ( bRet ) {
            TRACE0( "Written packet to device" );
            IFDBG( DebugOut( DEBUG_OUTPUT, L"Data to device:\n" ) );
            IFDBG( DumpBuff( DEBUG_OUTPUT, buffer, dwSize ) );
         } else {
            TRACE1( "WriteMsgQueue ret: 0x%08x", GetLastError() );
            IFDBG( DebugOut( DEBUG_OUTPUT, L"WriteMsgQueue ret: 0x%08x\n", GetLastError() ) );
         }    
         
         
      } else {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"COMMAND_PACKET GetNextParameterType ret: FAILED\n" ) );
      }
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-ReadDesktopWriteDevicePacket\n" ) );
}

/**
@func void | ReadDesktopWriteDevicePacket | Reads data from the message queue and writes them to the desktop.
*/
void ReadDeviceWriteDesktop()
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+ReadDeviceWriteDesktop\n" ) );

   //ASSERT( g_hReadQueue );   
   if ( g_hReadQueue ) {
      unsigned char buffer[MSG_BUFFER_SIZE] = {0};

      DWORD dwReaded = 0;
      DWORD dwFlags = 0;          
      BOOL bRet = ReadMsgQueue( g_hReadQueue, buffer, MSG_BUFFER_SIZE, &dwReaded, 0/*MSG_QUEUE_READ_TIMEOUT*/, &dwFlags );
      if ( bRet ) {
         TRACE0( "Readed packet from device" );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"Data from device:\n" ) );
         Packet packet;
         packet.deserialize( buffer, dwReaded );
         IFDBG( DumpBuff( DEBUG_OUTPUT, buffer, dwReaded ) );
         
         // read packet type.
         DWORD dwCmd = packet.readInt();
         IFDBG( DebugOut( DEBUG_OUTPUT, L"Data to desktop: dwCmd: 0x%08x\n", dwCmd ) );

         switch( dwCmd ) {               
         
         case MESSAGE_PACKET: {
            DWORD dwMsgId = packet.readInt();
            // send command...
            SendCommand( MESSAGE_PACKET, dwMsgId );
            }
            break;

         case HCI_DATA_PACKET: {
            // read data packet if any.
            size_t size = packet.readUCharArray( buffer, sizeof( buffer ) );
            // send command...
            SendCommand( HCI_DATA_PACKET, buffer, size );
            }
            break;

         default:
            IFDBG( DebugOut( DEBUG_OUTPUT, L"Unknown packet type: 0x%08x\n", dwCmd ) );
            break;
         }                                   

      } else {
         TRACE1( "ReadMsgQueue ret: 0x%08x", GetLastError() );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"ReadMsgQueue ret: 0x%08x\n", GetLastError() ) );         
      }         
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-ReadDeviceWriteDesktop\n" ) );
}

/**
@func HRESULT | CommandCallback | Function prototype for the callback function to StartCommandHandler.
@parm DWORD | dwCmd | The commandId the desktop sent.
@parm const CCommandPacket* | pCmdDataIn | Command object with the data from the desktop.
@parm CCommandPacket* | pCmdDataOut | Command object you use to send data back to the desktop.
@rdesc Once this function returns with an S_OK, the response will be sent back up the wire for ProcessCommand. If the desktop component
called SendCommand, then the pCmdDataOut parameter is ignored.
@remark This callback is called in response to a command packet coming down the wire from the desktop via ProcessCommand or SendCommand.
*/
HRESULT STDAPICALLTYPE CommandCallback( DWORD dwCmd, const CCommandPacket* pCmdDataIn, CCommandPacket* pCmdDataOut )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+CommandCallback dwCmd: 0x%08x\n", dwCmd ) );
   ++g_lIncomeMsgCounter;

   HRESULT hRes = S_OK;

   switch ( dwCmd ) {
      case END_COMMAND:
         PulseEvent( g_hQuitEvent );
         break;         

      case MESSAGE_PACKET: {
         CCommandPacket::DATATYPE dataType = CCommandPacket::DATATYPE_END;
         DWORD dwSize = 0; 
         if ( pCmdDataIn->GetNextParameterType( &dataType, &dwSize ) ) {
            if ( dataType != CCommandPacket::DATATYPE_END && dataType == CCommandPacket::DATATYPE_DWORD && dwSize > 0 ) {
               DWORD dwMsgId = 0;
               if ( pCmdDataIn->GetParameterDWORD( &dwMsgId ) ) {

                  switch( dwMsgId ) {
                  case AGENT_ACK_MSG: {
                     //IFDBG( DebugOut( DEBUG_OUTPUT, L"AGENT_ACK_MSG back\n" ) );
                     SendCommand( MESSAGE_PACKET, AGENT_ACK_MSG );
                  }
                  break;

                  case AGENT_PING_MSG:
                     //IFDBG( DebugOut( DEBUG_OUTPUT, L"AGENT_PING_MSG back\n" ) );
                     InterlockedExchange( &g_lWatchDogCounter, WATCHDOG_MAX_COUNTER );
                     SendCommand( MESSAGE_PACKET, AGENT_PING_MSG );
                     break;

                  case AGENT_INITIALIZE_MSG: {
                     //IFDBG( DebugOut( DEBUG_OUTPUT, L"AGENT_INITIALIZE_MSG back\n" ) );
                     // initialize agent...
                     Initialize();
                     SendCommand( MESSAGE_PACKET, AGENT_INITIALIZE_MSG );                     
                  }
                  break;
                       
                  case AGENT_UNINITIALIZE_MSG: {
                     //IFDBG( DebugOut( DEBUG_OUTPUT, L"AGENT_UNINITIALIZE_MSG back\n" ) );
                     // uninitialize agent...
                     Uninitialize();
                     SendCommand( MESSAGE_PACKET, AGENT_UNINITIALIZE_MSG );                     
                  }
                  break;

                  case AGENT_LOGGING_ON_MSG: {
                     //IFDBG( DebugOut( DEBUG_OUTPUT, L"AGENT_LOGGING_ON_MSG back\n" ) );
                     // set logging on.
                     SetLogLevel( 0xFFFFFFFF );
                     SendCommand( MESSAGE_PACKET, AGENT_LOGGING_ON_MSG );                     
                  }
                  break;

                  case AGENT_LOGGING_OFF_MSG: {
                     //IFDBG( DebugOut( DEBUG_OUTPUT, L"AGENT_LOGGING_OFF_MSG back\n" ) );
                     // set logging off.
                     SetLogLevel( 0 );
                     SendCommand( MESSAGE_PACKET, AGENT_LOGGING_OFF_MSG );                     
                  }
                  break;

                  default:
                     IFDBG( DebugOut( DEBUG_OUTPUT, L"COMMAND_PACKET : Unknown command id: 0x%08x\n", dwMsgId ) );
                     break;
                  }
               } else {
                  IFDBG( DebugOut( DEBUG_OUTPUT, L"COMMAND_PACKET GetParameterWORD ret: FAILED\n" ) );
               }
            } else {
               IFDBG( DebugOut( DEBUG_OUTPUT, L"COMMAND_PACKET DATATYPE ret: FAILED\n" ) );
            }
         } else {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"COMMAND_PACKET GetNextParameterType ret: FAILED\n" ) );
         }         
         break;
      }

      case HCI_DATA_PACKET:
         InterlockedExchange( &g_lWatchDogCounter, WATCHDOG_MAX_COUNTER );
         ReadDesktopWriteDevicePacket( dwCmd, pCmdDataIn );
         break;

      case HCI_DATA_ERROR_PACKET: {
            CCommandPacket::DATATYPE dataType = CCommandPacket::DATATYPE_END;
            DWORD dwSize = 0; 
            if ( pCmdDataIn->GetNextParameterType( &dataType, &dwSize ) ) {
               if ( dataType != CCommandPacket::DATATYPE_END && dataType == CCommandPacket::DATATYPE_DWORD && dwSize > 0 ) {
                  DWORD dwLastError = 0;
                  if ( pCmdDataIn->GetParameterDWORD( &dwLastError ) ) {                  
                     BOOL bRet = WriteMsgQueue( g_hErrorQueue, &dwLastError, sizeof(DWORD), MSG_QUEUE_WRITE_TIMEOUT, 0 );
                     if ( bRet ) {
                        IFDBG( DebugOut( DEBUG_OUTPUT, L"Last error received: 0x%08x\n", dwLastError ) );
                     } else {
                        TRACE1( "WriteMsgQueue ret: 0x%08x", GetLastError() );
                        IFDBG( DebugOut( DEBUG_OUTPUT, L"WriteMsgQueue ret: 0x%08x\n", GetLastError() ) );
                     }    
                  }
               }
            }
            break;
         } 
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-CommandCallback ret: %d\n", hRes ) );
   return hRes;
}

/**
@func DWORD | WorkingThread | A main working thread, used to read data from the message queue and writes them to the desktop.
@parm LPVOID | lpParam | Thread data passed to the function using the lpParameter parameter of the CreateThread function. 
@rdesc The function should return a value that indicates its success or failure. 
*/
DWORD WINAPI WorkingThread( LPVOID lpParam )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+WorkingThread\n" ) );

   DWORD dwRes = 0;
   HANDLE handles[] = { g_hQuitEvent, g_hReadQueue };

   DWORD dwWait = WAIT_FAILED;
   for (;;) {
      dwWait = WaitForMultipleObjects( sizeof( handles )/sizeof( handles[0] ), handles, FALSE, INFINITE );
      if ( WAIT_OBJECT_0 == dwWait ) {
         // exit the loop...
         break;
      } else if ( WAIT_OBJECT_0 + 1 == dwWait ) {
         // data available...
      ReadDeviceWriteDesktop();
      } else if (WAIT_TIMEOUT == dwWait ) {
         // this one should never been called.
         TRACE0( "WaitForMultipleObjects ret: WAIT_TIMEOUT" );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"WaitForMultipleObjects ret: WAIT_TIMEOUT\n" ) );
      } else {
         // WAIT_FAILED
         DWORD dwError = GetLastError();
         TRACE1( "WaitForMultipleObjects ret: 0x%08x", dwError );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"WaitForMultipleObjects ret: 0x%08x\n", dwError ) );
      }
    }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-WorkingThread ret: %lu\n", dwRes ) ); 
   return dwRes;
}

/**
@func DWORD | WatchDogThread | The watch dog thread, used to determine the connection loss with the desktop.
@parm LPVOID | lpParam | Thread data passed to the function using the lpParameter parameter of the CreateThread function. 
@rdesc The function should return a value that indicates its success or failure. 
*/
DWORD WINAPI WatchDogThread( LPVOID lpParam )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+WatchDogThread\n" ) );

   DWORD dwRes = 0;

   while ( WAIT_OBJECT_0 != WaitForSingleObject( g_hQuitEvent, WATCHDOG_SLEEP_TIMEOUT ) ) {
      // if the counter has been zeroed exit.
      if ( 0 >= InterlockedDecrement( &g_lWatchDogCounter ) ) {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"WatchDog Fired !!!\n" ) );
         SetEvent( g_hQuitEvent );
      } else {
         //IFDBG( DebugOut( DEBUG_OUTPUT, L"WatchDog: %d\n", g_lWatchDogCounter ) );
      }
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-WatchDogThread ret: %lu\n", dwRes ) ); 
   return dwRes;
}

/**
@func BOOL | ActivateDriver | Loads and activates a new driver instance.
@rdesc A nonzero value indicates success. A value of zero indicates failure. To get extended error information, call GetLastError.
*/
BOOL ActivateDriver()
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+ActivateDriver\n" ) );

   BOOL bRet = FALSE;

   ASSERT( g_hDevice == NULL );

   TCHAR szDeviceName[MAX_PATH];
   for( int index = 1; index <= 9; ++index ) {		
      // generate a new device name.
      memset( szDeviceName, 0, sizeof( szDeviceName ) );
      _stprintf( szDeviceName, _T("%s%d:"), DEVICE_PREFIX, index ); 

      // store the name to the registry. at initialization time the driver will read this info.
      HKEY hk = NULL;
      DWORD dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_KEY_NAME, 0, 0, &hk );
      if( ERROR_SUCCESS == dwStatus ) {
         DWORD dwType, dwSize;
         dwType = REG_SZ;
         dwSize = _tcslen( szDeviceName ) * sizeof( TCHAR );
         dwStatus = RegSetValueEx( hk, _T("Name"), NULL, dwType, (LPBYTE)szDeviceName, dwSize );
         if( dwStatus == ERROR_SUCCESS ) {
            // register driver.
            g_hDevice = RegisterDevice( DEVICE_PREFIX, index, COMMUNICATION_DRIVER_FILENAME, (DWORD)REG_KEY_NAME );
            if ( !g_hDevice ) {
               TRACE3( "RegisterDevice %s%d ret: 0x%08x", DEVICE_PREFIX, index, GetLastError() );
               IFDBG( DebugOut( DEBUG_OUTPUT, L"RegisterDevice %s%d ret: 0x%08x\n", DEVICE_PREFIX, index, GetLastError() ) );
            } else {
               bRet = TRUE;               
            }

            // release the registry key
            RegCloseKey( hk );

            if ( bRet ) break;
         } else {
            TRACE3( "RegSetValueEx %s %s ret: 0x%08x", REG_KEY_NAME, _T("Name"), dwStatus );
            IFDBG( DebugOut( DEBUG_OUTPUT, L"RegSetValueEx %s %s ret: 0x%08x\n", REG_KEY_NAME, _T("Name"), dwStatus ) );
         }
      } else {
         TRACE2( "RegOpenKeyEx %s ret: 0x%08x", REG_KEY_NAME, dwStatus );
         IFDBG( DebugOut( DEBUG_OUTPUT, L"RegOpenKeyEx %s ret: 0x%08x\n", REG_KEY_NAME, dwStatus ) );
      }	   		
   }  

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-ActivateDriver ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | DeactivateDriver | Deactivates the loaded driver instance.
@rdesc A nonzero value indicates success. A value of zero indicates failure. To get extended error information, call GetLastError.
*/
BOOL DeactivateDriver()
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+DeactivateDriver\n" ) );

   BOOL bRet = FALSE;
   
   if ( g_hDevice ) {
      // unregister the driver.
      bRet = DeregisterDevice( g_hDevice );
      ASSERT( bRet );
      g_hDevice = NULL;
   }   
   
   IFDBG( DebugOut( DEBUG_OUTPUT, L"-DeactivateDriver ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | CopyDriversToWindowsDir | Copyes transport and communication drivers to Windows directory.
@rdesc A nonzero value indicates success. A value of zero indicates failure. To get extended error information, call GetLastError.
*/
BOOL CopyDriversToWindowsDir()
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+CopyDriversToWindowsDir\n" ) );

   TCHAR szFromPath[MAX_PATH] = {0};
   TCHAR szToPath[MAX_PATH] = {0};

   // get current directory.
   TCHAR szAppPath[MAX_PATH] = {0};
   TCHAR szCurrDir[MAX_PATH] = {0};
   szCurrDir[0] = 0;
   GetModuleFileName( NULL, szAppPath, MAX_PATH );
   for( int i = _tcslen( szAppPath ) - 1; i >= 0; i-- ) {
      if( szAppPath[i] == _T('\\') ) {
         if( i == 0 ) 
            _tcscpy( szCurrDir, _T("\\") );
         else {
            _tcsncpy( szCurrDir, szAppPath, i + 1 );
            szCurrDir[i + 1] = _T('\0');
         }
         break;
      }
   }

   // copy transport driver to \Windows directory.
   _stprintf( szFromPath, _T("%s%s"), szCurrDir, TRANSPORT_DRIVER_FILENAME );
   _stprintf( szToPath, _T("%s%s"), _T("\\Windows\\"), TRANSPORT_DRIVER_FILENAME );
   BOOL bRet = CopyFile( szFromPath, szToPath, FALSE );
   ASSERT( bRet );

   // copy communication driver to \Windows directory.
   _stprintf( szFromPath, _T("%s%s"), szCurrDir, COMMUNICATION_DRIVER_FILENAME );
   _stprintf( szToPath, _T("%s%s"), _T("\\Windows\\"), COMMUNICATION_DRIVER_FILENAME );
   BOOL bRet2 = CopyFile( szFromPath, szToPath, FALSE );
   ASSERT( bRet );	

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-CopyDriversToWindowsDir ret: %d\n", bRet & bRet2 ) );
   return bRet & bRet2;
}

/**
@func BOOL | ProvisionDevice |
@rdesc The function should return a value that indicates its success or failure. 
*/
BOOL ProvisionDevice()
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+ProvisionDevice\n" ) );

   BOOL bRet = FALSE;

   TCHAR szProvisionRgs[MAX_PATH] = {0};

   // get current directory.
   TCHAR szAppPath[MAX_PATH] = {0};
   TCHAR szCurrDir[MAX_PATH] = {0};
   szCurrDir[0] = 0;
   GetModuleFileName( NULL, szAppPath, MAX_PATH );
   for( int i = _tcslen( szAppPath ) - 1; i >= 0; i-- ) {
      if( szAppPath[i] == _T('\\') ) {
         if( i == 0 ) 
            _tcscpy( szCurrDir, _T("\\") );
         else {
            _tcsncpy( szCurrDir, szAppPath, i + 1 );
            szCurrDir[i + 1] = _T('\0');
         }
         break;
      }
   }

   _stprintf( szProvisionRgs, _T("%s%s"), szCurrDir, SETTINGS_FILENAME );
   bRet = ProvisionDeviceWithRgs( szProvisionRgs );

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-ProvisionDevice ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | ProvisionDeviceWithRgs | Provisions device with a given registry script ( rgs ) file.
@parm LPCWSTR | szwFileName | Registry script file name.
@rdesc The function should return a value that indicates its success or failure. 
*/
BOOL ProvisionDeviceWithRgs( LPCWSTR szwFileName )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+ProvisionDeviceWithRgs\n" ) );

   BOOL bRet = FALSE;

   ATL::CRegObject regObject;
   HRESULT hr = regObject.FileRegister( szwFileName );
   ASSERT( SUCCEEDED( hr ) );
   regObject.FinalConstruct();   

   bRet = SUCCEEDED( hr );

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-ProvisionDeviceWithRgs ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | SendCommand | Sends message packet to desktop.
@parm DWORD | dwCmd | Command Id.
@parm DWORD | dwMsgId | Message Id.
@rdesc The function should return a value that indicates its success or failure. 
*/
BOOL SendCommand( DWORD dwCmd, DWORD dwMsgId )
{
   EnterCriticalSection( &g_criticalSection );
   
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+SendCommand dwCmd: 0x%08x dwMsgId: 0x%08x\n", dwCmd, dwMsgId ) );

   BOOL bRet = FALSE;

   if ( g_pCmd ) {
      g_pCmd->Reset();
      g_pCmd->AddParameterDWORD( dwMsgId );      
      
      HRESULT hr = g_DeviceRemoteTool.PushCommand( dwCmd, g_pCmd );
      ASSERT( SUCCEEDED( hr ) );
      if ( SUCCEEDED( hr ) ) {
         ++g_lOutcomeMsgCounter;
         bRet = TRUE;      
      } else {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"PushCommand ret: 0x%08x\n", hr ) );         
      }
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-SendCommand ret: %d\n", bRet ) );

   LeaveCriticalSection( &g_criticalSection );

   return bRet;   
}

/**
@func BOOL | SendCommand | Sends message packet to desktop.
@parm DWORD | dwCmd | Command Id.
@parm BYTE* | pData | Data buffer.
@parm DWORD | cbData | Data buffer size.
@rdesc The function should return a value that indicates its success or failure. 
*/
BOOL SendCommand( DWORD dwCmd, BYTE* pData, DWORD cbData )
{
   EnterCriticalSection( &g_criticalSection );
   
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+SendCommand pData:\n" ) );
   IFDBG( DumpBuff( DEBUG_OUTPUT, pData, cbData ) );

   BOOL bRet = FALSE;

   if ( g_pCmd ) {
      if ( cbData ) {
         g_pCmd->Reset();
         g_pCmd->AddParameterBytes( pData, cbData );
      }

      HRESULT hr = g_DeviceRemoteTool.PushCommand( dwCmd, g_pCmd );
      ASSERT( SUCCEEDED( hr ) );
      if ( SUCCEEDED( hr ) ) {
         ++g_lOutcomeMsgCounter;
         bRet = TRUE;      
      } else {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"PushCommand ret: 0x%08x\n", hr ) );         
      }
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-SendCommand ret: %d\n", bRet ) );
   
   LeaveCriticalSection( &g_criticalSection );
   
   return bRet;   
}

/**
@func BOOL | Initialize | Initializes communication means.
@rdesc The function should return a value that indicates its success or failure. 
*/
BOOL Initialize()
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+Initialize\n" ) );

   // sleep a little. see bthemul.cxx::HCI_OpenConnection
   Sleep( 500 );

   int nRet = 0;
   // create messages queues to communicate with.
   BOOL bRet = CreateMsgQueues();
   ASSERT( bRet );

   if ( bRet ) {
      bRet = ProvisionDevice();
      ASSERT( bRet );

      if ( bRet ) {
         // copy transport and communication drivers to Windows directory.
         bRet = CopyDriversToWindowsDir();
         ASSERT( bRet );

         if ( bRet ) {  
            // activate driver.
            BOOL bRet = ActivateDriver();
            ASSERT( bRet );

            if ( bRet ) {
               // create working thread.
               g_hWorkingThread = CreateThread( NULL, 0, WorkingThread, NULL, 0, NULL );
               ASSERT( g_hWorkingThread );

               if ( !g_hWorkingThread ) {
                  nRet = ERROR_CREATE_WORKING_THREAD;
                  IFDBG( DebugOut( DEBUG_OUTPUT, L"CreateThread GetLastError: 0x%08x\n", GetLastError() ) );
                  return nRet;
               }
            } else {
               nRet = ERROR_ACTIVATE_DRIVER;
            }
         } else {
            nRet = ERROR_COPY_DRIVERS;
         }
      } else {
         nRet = ERROR_PRIVISION_DEVICE;
      }      
   } else {
      nRet = ERROR_CREATE_MSG_QUEUES;
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-Initialize ret: %d\n", nRet ) );
   return nRet;
}

/**
@func BOOL | Uninitialize | Uninitializes communication means.
@rdesc The function should return a value that indicates its success or failure. 
*/
BOOL Uninitialize()
{   
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+Uninitialize\n" ) );
   
   // close messages queues.
   BOOL bRet = CloseMsgQueue( g_hWriteQueue );
   ASSERT( bRet );
   g_hWriteQueue = NULL;

   bRet = CloseMsgQueue( g_hReadQueue );
   ASSERT( bRet );
   g_hReadQueue = NULL;

   bRet = CloseMsgQueue( g_hErrorQueue );
   ASSERT( bRet );
   g_hErrorQueue = NULL;

   // deactivate driver.
   bRet = DeactivateDriver();
   ASSERT( bRet );

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-Uninitialize ret: %d\n", bRet ) );
   return bRet;
}