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

#define LOCALDBG            L"bthemulcom"
#include "DeviceDebug.h"

#include <devload.h>
#include <pegdser.h>
#include <svsutil.hxx>
#include "bthemulcom.h"
#include "MsgQueueDef.h"

extern "C" int svsutil_AssertBroken( TCHAR *lpszFile, int iLine ) { return 0; }

#if defined( DEBUG ) || defined( _DEBUG )
	#undef DEBUGCHK
	#define DEBUGCHK    SVSUTIL_ASSERT
#endif

#define DEVICE_CONTEXT	0x1450
#define OPEN_CONTEXT    0x1451

static HMODULE g_hModule = NULL;
static GUID g_guidClass;
static TCHAR g_szDeviceName[MAX_PATH];
static HANDLE g_hReadThread = NULL;
static HANDLE g_hQuitEvent = NULL;
static unsigned char g_buffer[MSG_BUFFER_SIZE];
static DWORD g_dwBufSize = 0;
static DWORD g_dwBufReadPos = 0;

/**
@func BOOL | ConvertStringToGuid | Converts a string into a GUID.
@parm LPCTSTR | pszGuid | String GUID.
@parm GUID* | pGuid | Pointer to GUID structure.
@rdesc Returns TRUE if the conversion was successful.
*/
BOOL ConvertStringToGuid( LPCTSTR pszGuid, GUID* pGuid )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+ConvertStringToGuid\n" ) );

   UINT Data4[8];
   BOOL bRet = FALSE;
   TCHAR *pszGuidFormat = _T("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

   DEBUGCHK( pGuid != NULL && pszGuid != NULL );
   __try {
      if ( _stscanf( pszGuid, pszGuidFormat, &pGuid->Data1, &pGuid->Data2, &pGuid->Data3, 
            &Data4[0], &Data4[1], &Data4[2], &Data4[3], &Data4[4], &Data4[5], &Data4[6], &Data4[7]) == 11 ) {
         for( int count = 0; count < (sizeof(Data4) / sizeof(Data4[0])); ++count ) {
            pGuid->Data4[count] = (UCHAR)Data4[count];
         }
      }
      bRet = TRUE;
   } __except( EXCEPTION_EXECUTE_HANDLER ) {
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-ConvertStringToGuid ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | NeedAdvertiseInterface | Reads the registry settings to determine the need to advertise driver's interface.
@parm LPCTSTR | szRegKey | Driver's registry key.
@rdesc Returns TRUE if needed and FALSE otherwise.
*/
BOOL NeedAdvertiseInterface( LPCTSTR szRegKey )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+NeedAdvertiseInterface\n" ) );

   BOOL bRet = FALSE;

   HKEY hk = NULL;
   DWORD dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRegKey, 0, 0, &hk );
   if( dwStatus == ERROR_SUCCESS ) {
      DWORD dwType, dwSize, dwValue;
      dwSize = sizeof( dwValue );
      dwStatus = RegQueryValueEx( hk, _T("AdvertiseInterface"), NULL, &dwType, (LPBYTE)&dwValue, &dwSize );
      if( dwStatus == ERROR_SUCCESS && dwType == REG_DWORD ) {
         bRet = dwValue;
      } else {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"RegQueryValueEx %s %s ret: 0x%08x", REG_KEY_NAME, _T("AdvertiseInterface"), dwStatus ) );
      }

      // release the registry key.
      RegCloseKey( hk );
      hk = NULL;
   } else {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"RegOpenKeyEx %s ret: 0x%08x", REG_KEY_NAME, dwStatus ) );
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-NeedAdvertiseInterface ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | AdvertiseInterface | Reads device name and its guid from the given registry key and advertises the driver's interface.
@parm LPCTSTR | szRegKey | Driver's registry key.
@rdesc Returns TRUE on success.
*/
BOOL AdvertiseInterface( LPCTSTR szRegKey )
{
	IFDBG( DebugOut( DEBUG_OUTPUT, L"+AdvertiseInterface\n" ) );
   
   BOOL bRet = FALSE;
   
   HKEY hk = NULL;
   DWORD dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRegKey, 0, 0, &hk );
	if( dwStatus == ERROR_SUCCESS ) {
      TCHAR szTemp[MAX_PATH] = {0};
      DWORD dwType, dwSize;
		dwSize = sizeof( szTemp );
		// figure out what device guid to advertise.
		dwStatus = RegQueryValueEx( hk, DEVLOAD_ICLASS_VALNAME, NULL, &dwType, (LPBYTE)szTemp, &dwSize );
		szTemp[MAX_PATH-1] = 0;
		if( dwStatus == ERROR_SUCCESS && dwType == REG_SZ ) {
			// got a guid string, convert it to a guid.
			GUID guidTemp;
			bRet = ConvertStringToGuid( szTemp, &guidTemp );
			DEBUGCHK(bRet);
			if( bRet ) {
				g_guidClass = guidTemp;
			}
      } else {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"RegQueryValueEx %s %s ret: 0x%08x\n", REG_KEY_NAME, DEVLOAD_ICLASS_VALNAME, dwStatus ) );
      }

		// figure out what device name to advertise.
		dwSize = sizeof( szTemp );
		dwStatus = RegQueryValueEx( hk, DEVLOAD_DEVNAME_VALNAME, NULL, &dwType, (LPBYTE)szTemp, &dwSize );
		szTemp[MAX_PATH-1] = 0;
		if( dwStatus == ERROR_SUCCESS && dwType == DEVLOAD_DEVNAME_VALTYPE ) {
			memset( g_szDeviceName, 0, sizeof( g_szDeviceName ) );
			_tcscpy( g_szDeviceName, szTemp );			
      } else {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"RegQueryValueEx %s %s ret: 0x%08x\n", REG_KEY_NAME, DEVLOAD_DEVNAME_VALNAME, dwStatus ) );
      }

		// release the registry key.
		RegCloseKey( hk );
      hk = NULL;
   } else {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"RegOpenKeyEx %s ret: 0x%08x\n", REG_KEY_NAME, dwStatus ) );
   }

	// now advertise the interface.
	if( bRet ) {
		bRet = AdvertiseInterface( &g_guidClass, g_szDeviceName, TRUE );
		DEBUGCHK( bRet );
	}
    
   IFDBG( DebugOut( DEBUG_OUTPUT, L"-AdvertiseInterface ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | WritePacket | Writes the given packet to the message queue.
@parm const Packet& | packet | Packet object const reference.
@rdesc Returns TRUE on success.
*/
BOOL WritePacket( const Packet& packet )
{
   //IFDBG( DebugOut( DEBUG_OUTPUT, L"+WritePacket\n" ) );
   
   BOOL bRet = FALSE;

   DEBUGCHK( g_hWriteQueue );
   if ( g_hWriteQueue ) {
      unsigned char buffer[MSG_BUFFER_SIZE] = {0};
      size_t size = packet.serialize( buffer, packet.length() );
      //IFDBG( DebugOut( DEBUG_OUTPUT, L"Data to queue:\n") );
      //IFDBG( DumpBuff( DEBUG_OUTPUT, buffer, size ) );

      bRet = WriteMsgQueue( g_hWriteQueue, buffer, size, MSG_QUEUE_WRITE_TIMEOUT, 0 );
      DEBUGCHK( bRet );
      if ( !bRet ) {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"WriteMsgQueue ret: 0x%08x\n", GetLastError() ) );
      }
   }   

   //IFDBG( DebugOut( DEBUG_OUTPUT, L"-WritePacket ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | ReadPacket | Reads data from the message queue to the given packet.
@parm Packet& | packet | Packet object reference.
@rdesc Returns TRUE on success.
*/
BOOL ReadPacket( Packet& packet )
{
   //IFDBG( DebugOut( DEBUG_OUTPUT, L"+ReadPacket\n" ) );  

   BOOL bRet = FALSE;

   DEBUGCHK( g_hReadQueue );
   if ( g_hReadQueue ) {
      unsigned char buffer[MSG_BUFFER_SIZE] = {0};
      DWORD dwReaded = 0;
      DWORD dwFlags = 0;

      bRet = ReadMsgQueue( g_hReadQueue, buffer, MSG_BUFFER_SIZE, &dwReaded, MSG_QUEUE_READ_TIMEOUT, &dwFlags );
      DEBUGCHK( bRet );
      if ( bRet ) {
         //IFDBG( DebugOut( DEBUG_OUTPUT, L"Data from queue:\n" ) );
         //IFDBG( DumpBuff( DEBUG_OUTPUT, (unsigned char*)buffer, dwReaded ) );
         size_t size = packet.deserialize( buffer, dwReaded );
         bRet = ( size > 0 );
      } else {
         IFDBG( DebugOut( DEBUG_OUTPUT, L"ReadMsgQueue ret: 0x%08x\n", GetLastError() ) );
      }
   }

   //IFDBG( DebugOut( DEBUG_OUTPUT, L"-ReadPacket ret: %d\n", bRet ) );
   return bRet;
}

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
	  g_hModule = (HMODULE)hModule;
	  DisableThreadLibraryCalls( (HMODULE)g_hModule );
	  DebugInit();
	  IFDBG( DebugOut( DEBUG_OUTPUT, L"DllMain: DLL_PROCESS_ATTACH\n" ) );
	  break;
   case DLL_PROCESS_DETACH:
	  IFDBG( DebugOut( DEBUG_OUTPUT, L"DllMain: DLL_PROCESS_DETACH\n" ) );
	  DebugDeInit();
     break;   
   }

   return TRUE;
}

/**
@func HANDLE | BTE_INIT | Serial device initialization.
@parm ULONG	| uIdentifier | Port identifier.  The device loader passes in the registry key that contains information about the active device.
@rdesc Returns a pointer to the serial head which is passed into the BTE_OPEN and BTE_DEINIT entry points as a device handle.
@remark This routine is called at device load time in order to perform any initialization. Typically the init routine does as little as possible, postponing memory allocation and device power-on to Open time.
@remark Routine exported by a device driver.  
*/
DWORD BTE_Init( LPCTSTR pContext, LPCVOID lpvBusContext )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_Init\n" ) );

   DWORD dwRet = 0;   
   BOOL bRet = CreateMsgQueues();
   DEBUGCHK( bRet );
   if ( bRet ) {
      if ( NeedAdvertiseInterface( REG_KEY_NAME ) ) {
         bRet = AdvertiseInterface( REG_KEY_NAME );
         DEBUGCHK( bRet );
         if ( bRet ) {
            dwRet = DEVICE_CONTEXT;
         }
      } else {
         dwRet = DEVICE_CONTEXT;
      }
   } else {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"CreateMsgQueues ret: 0x%08x\n", GetLastError() ) );
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_Init ret: %lu\n", dwRet ) );
   return dwRet;
}

/**
@func BOOL | BTE_Deinit | De-initialize serial port.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@rdesc None.
@remark Routine exported by a device driver.  
*/
BOOL BTE_Deinit( DWORD hDeviceContext )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_Deinit\n" ) );
   
   BOOL bRet = FALSE;
   if ( hDeviceContext == DEVICE_CONTEXT ) {
      if ( NeedAdvertiseInterface( REG_KEY_NAME ) ) {
         bRet = AdvertiseInterface( &g_guidClass, g_szDeviceName, FALSE );
         DEBUGCHK( bRet );
      }

      CloseMsgQueue( g_hWriteQueue );
      g_hWriteQueue = NULL;

      CloseMsgQueue( g_hReadQueue );     
      g_hReadQueue = NULL;

      CloseMsgQueue( g_hErrorQueue );
      g_hErrorQueue = NULL;
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_Deinit ret: %d\n", bRet ) );
   return bRet;
}

/**
@func HANDLE | BTE_Open | Serial port driver initialization.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@parm DWORD | dwAccess | requested access ( combination of GENERIC_READ and GENERIC_WRITE )
@parm DWORD | dwShareMode | requested share mode ( combination of FILE_SHARE_READ and FILE_SHARE_WRITE )
@rdesc Returns a DWORD which will be passed to Read, Write, etc or NULL if unable to open device.
@remark This routine must be called by the user to open the serial device. The HANDLE returned must be used by the application in all subsequent calls to the serial driver. This routine starts the thread which handles the serial events.
@remark Routine exported by a device driver.  
*/
DWORD BTE_Open( DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_Open\n" ) );

   DWORD dwRet = 0;   
   if ( hDeviceContext == DEVICE_CONTEXT ) {
      // send acknowledgement packet.
      Packet packet;
      packet.writeInt( MESSAGE_PACKET );
      packet.writeInt( COM_OPEN_MSG );
      if ( WritePacket( packet ) ) {
         dwRet = OPEN_CONTEXT;
      } else {
         SetLastError( ERROR_TIMEOUT );
      }
   } else {
      SetLastError( ERROR_INVALID_HANDLE );
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_Open ret: %lu\n", dwRet ) );
   return dwRet;
}

/**
@func BOOL | BTE_Close | Close the serial device.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@rdesc TRUE if success; FALSE if failure
@remark This routine is called by the device manager to close the device.
@remark Routine exported by a device driver.  
*/
BOOL BTE_Close( DWORD hOpenContext )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_Close\n" ) );
   
   BOOL bRet = FALSE;
   if ( hOpenContext == OPEN_CONTEXT ) {
      // send goodbye packet.
      Packet packet;
      packet.writeInt( MESSAGE_PACKET );
      packet.writeInt( COM_CLOSE_MSG );
      if ( WritePacket( packet ) ) {
         bRet = TRUE;
      } else {
         SetLastError( ERROR_TIMEOUT );
      }
   } else {
      SetLastError( ERROR_INVALID_HANDLE );
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_Close ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | BTE_IOControl | Device IO control routine.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@parm DWORD | dwIoControlCode | IO control code to be performed
@parm PBYTE | pBufIn | Input data to the device
@parm DWORD | dwLenIn | Number of bytes being passed in
@parm PBYTE | pBufOut | Output data from the device
@parm DWORD | dwLenOut | Maximum number of bytes to receive from device
@parm PDWORD | pdwActualOut | Actual number of bytes received from device
@rdesc Returns TRUE for success, FALSE for failure
@remark Routine exported by a device driver. "COM" is the string passed in as lpszType in RegisterDevice
@remark Routine exported by a device driver.  
*/
BOOL BTE_IOControl( DWORD hOpenContext, DWORD dwIoControlCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_IOControl\n" ) );
   
   BOOL bRet = FALSE;
   if ( hOpenContext == OPEN_CONTEXT ) {

      LPCWSTR szIOControlCode = L"UNKNOWN_CODE";
      switch( dwIoControlCode ) {
         case IOCTL_SERIAL_SET_BREAK_ON: szIOControlCode = L"IOCTL_SERIAL_SET_BREAK_ON"; break;
         case IOCTL_SERIAL_SET_BREAK_OFF: szIOControlCode = L"IOCTL_SERIAL_SET_BREAK_OFF"; break;
         case IOCTL_SERIAL_SET_DTR: szIOControlCode = L"IOCTL_SERIAL_SET_DTR"; break;
         case IOCTL_SERIAL_CLR_DTR: szIOControlCode = L"IOCTL_SERIAL_CLR_DTR"; break;
         case IOCTL_SERIAL_SET_RTS: szIOControlCode = L"IOCTL_SERIAL_SET_RTS"; break;
         case IOCTL_SERIAL_CLR_RTS: szIOControlCode = L"IOCTL_SERIAL_CLR_RTS"; break;
         case IOCTL_SERIAL_SET_XOFF: szIOControlCode = L"IOCTL_SERIAL_SET_XOFF"; break;
         case IOCTL_SERIAL_SET_XON: szIOControlCode = L"IOCTL_SERIAL_SET_XON"; break;
         case IOCTL_SERIAL_GET_WAIT_MASK: szIOControlCode = L"IOCTL_SERIAL_GET_WAIT_MASK"; break;
         case IOCTL_SERIAL_SET_WAIT_MASK: szIOControlCode = L"IOCTL_SERIAL_SET_WAIT_MASK"; break;
         case IOCTL_SERIAL_WAIT_ON_MASK: szIOControlCode = L"IOCTL_SERIAL_WAIT_ON_MASK"; break;
         case IOCTL_SERIAL_GET_COMMSTATUS: szIOControlCode = L"IOCTL_SERIAL_GET_COMMSTATUS"; break;
         case IOCTL_SERIAL_GET_MODEMSTATUS: szIOControlCode = L"IOCTL_SERIAL_GET_MODEMSTATUS"; break;
         case IOCTL_SERIAL_GET_PROPERTIES: szIOControlCode = L"IOCTL_SERIAL_GET_PROPERTIES"; break;
         case IOCTL_SERIAL_SET_TIMEOUTS: szIOControlCode = L"IOCTL_SERIAL_SET_TIMEOUTS"; break;
         case IOCTL_SERIAL_GET_TIMEOUTS: szIOControlCode = L"IOCTL_SERIAL_GET_TIMEOUTS"; break;
         case IOCTL_SERIAL_PURGE: szIOControlCode = L"IOCTL_SERIAL_PURGE"; break;
         case IOCTL_SERIAL_SET_QUEUE_SIZE: szIOControlCode = L"IOCTL_SERIAL_SET_QUEUE_SIZE"; break;
         case IOCTL_SERIAL_IMMEDIATE_CHAR: szIOControlCode = L"IOCTL_SERIAL_IMMEDIATE_CHAR"; break;
         case IOCTL_SERIAL_GET_DCB: szIOControlCode = L"IOCTL_SERIAL_GET_DCB"; break;
         case IOCTL_SERIAL_SET_DCB: szIOControlCode = L"IOCTL_SERIAL_SET_DCB"; break;
         case IOCTL_SERIAL_ENABLE_IR: szIOControlCode = L"IOCTL_SERIAL_ENABLE_IR"; break;
         case IOCTL_SERIAL_DISABLE_IR: szIOControlCode = L"IOCTL_SERIAL_DISABLE_IR"; break;
      }
      IFDBG( DebugOut( DEBUG_OUTPUT, L"ControlCode: %lu (%s)\n", dwIoControlCode, szIOControlCode ) );
      IFDBG( DumpBuff( DEBUG_OUTPUT, pBufIn, dwLenIn ) );
      
      bRet = TRUE;
   } else {
      SetLastError( ERROR_INVALID_HANDLE );
   }
   
   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_IOControl ret: %d\n", bRet ) );
   return bRet;
}

/**
@func BOOL | BTE_PowerUp | Turn power on to serial device.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@rdesc This routine returns a status of 1 if unsuccessful and 0 otherwise.
@remark Routine exported by a device driver.  
*/
void BTE_PowerUp( DWORD hDeviceContext )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"BTE_PowerUp\n" ) );
}

/**
@func BOOL | BTE_PowerDown | Turns off power to serial device.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@rdesc This routine returns a status of 1 if unsuccessful and 0 otherwise.
@remark Routine exported by a device driver.  
*/
void BTE_PowerDown( DWORD hDeviceContext )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"BTE_PowerDown\n" ) );
}

/**
@func ULONG | BTE_Read | Allows application to receive characters from serial port. This routine sets the buffer and bufferlength to be used by the reading thread. It also enables reception and controlling when to return to the user. It writes to the referent of the fourth argument the number of bytes transacted. It returns the status of the call.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@parm PUCHAR | pTargetBuffer | Pointer to valid memory.
@parm ULONG | uBufferLength | Size in bytes of pTargetBuffer.
@rdesc This routine returns: -1 if error, or number of bytes read.
@remark Routine exported by a device driver.  
*/
DWORD BTE_Read( DWORD hOpenContext, LPVOID pBuffer, DWORD dwCount )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_Read len: %d\n", dwCount ) );

   DWORD dwRet = -1;
   if ( hOpenContext == OPEN_CONTEXT ) {
      
      DEBUGCHK( pBuffer != NULL && dwCount > 0 );
      if ( pBuffer != NULL && dwCount > 0 ) {
         // if any data in cash buffer ?
         DWORD dwDataSize = g_dwBufSize - g_dwBufReadPos;
         if ( dwDataSize > 0 ) { // data exist
            DWORD dwToRead = dwCount > dwDataSize ? dwDataSize : dwCount;
            memcpy( pBuffer, g_buffer + g_dwBufReadPos, dwToRead );

            IFDBG( DebugOut( DEBUG_OUTPUT, L"Output buffer:\n" ) );
            IFDBG( DumpBuff( DEBUG_OUTPUT, (unsigned char*)pBuffer, dwToRead ) );

            g_dwBufReadPos += dwToRead;
            dwRet = dwToRead;
         } else { // no data. read data from the queue...
            //IFDBG( DebugOut( DEBUG_OUTPUT, L"Buffer read to:\n" ) );
            //IFDBG( DumpBuff( DEBUG_OUTPUT, (unsigned char*)pBuffer, dwCount ) );
            Packet packet;
            if ( ReadPacket( packet ) ) {
               int type = packet.readInt();
               if ( HCI_DATA_PACKET == type ) {
                  // store data to cash buffer.
                  memset( g_buffer, 0, MSG_BUFFER_SIZE );
                  g_dwBufSize = 0;
                  g_dwBufReadPos = 0;
                  g_dwBufSize = packet.readUCharArray( (unsigned char*)g_buffer, MSG_BUFFER_SIZE );
                  IFDBG( DebugOut( DEBUG_OUTPUT, L"Data from queue:\n" ) );
                  IFDBG( DumpBuff( DEBUG_OUTPUT, (unsigned char*)g_buffer, g_dwBufSize ) );

                  // if any data in cash buffer again ?
                  dwDataSize = g_dwBufSize - g_dwBufReadPos;
                  if ( dwDataSize > 0 ) { // data exist
                     DWORD dwToRead = dwCount > dwDataSize ? dwDataSize : dwCount;
                     memcpy( pBuffer, g_buffer + g_dwBufReadPos, dwToRead );

                     IFDBG( DebugOut( DEBUG_OUTPUT, L"Output buffer:\n" ) );
                     IFDBG( DumpBuff( DEBUG_OUTPUT, (unsigned char*)pBuffer, dwToRead ) );

                     g_dwBufReadPos += dwToRead;
                     dwRet = dwToRead;
                  } else {
                     SetLastError( ERROR_NO_DATA );
                  }
               } else {
                  SetLastError( ERROR_BAD_COMMAND );
               }
            } else {
               SetLastError( ERROR_TIMEOUT );
            }
         }
      } else {
         SetLastError( ERROR_INVALID_HANDLE );
      }
   } else {
      SetLastError( ERROR_INVALID_USER_BUFFER );
   }

   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_Read ret: %lu\n", dwRet ) );
   return dwRet;
}

/**
@func ULONG | BTE_Write | Allows application to transmit bytes to the serial port.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@parm PUCHAR | pSourceBytes | Buffer containing data.
@parm ULONG | uNumberOfBytes | Maximum length to write.
@rdesc Returns -1 for error, otherwise the number of bytes written.  The length returned is guaranteed to be the length requested unless an error condition occurs.
@remark Routine exported by a device driver.  
*/
DWORD BTE_Write( DWORD hOpenContext, LPVOID pBuffer, DWORD dwCount )
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_Write\n" ) );
   
   DWORD dwRet = -1;
   if ( hOpenContext == OPEN_CONTEXT ) {
      IFDBG( DebugOut( DEBUG_OUTPUT, L"Write buffer:\n") );
      IFDBG( DumpBuff( DEBUG_OUTPUT, (unsigned char*)pBuffer, dwCount ) );
      Packet packet;
      packet.writeInt( HCI_DATA_PACKET );
      packet.writeUCharArray( (unsigned char*)pBuffer, dwCount );
      if ( WritePacket( packet ) ) {
         // check the remote operation return code.
         DWORD dwLastError = 0;
         DWORD dwNumberOfBytesRead = 0;
         DWORD dwFlags = 0;
         BOOL bRet = ReadMsgQueue( g_hErrorQueue, &dwLastError, sizeof(DWORD), &dwNumberOfBytesRead, MSG_QUEUE_WRITE_TIMEOUT, &dwFlags );
         DEBUGCHK( bRet );
         if ( !bRet ) {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"ReadMsgQueue ret: 0x%08x\n", GetLastError() ) );
         } else {
            IFDBG( DebugOut( DEBUG_OUTPUT, L"Last error received: 0x%08x\n", dwLastError ) );
            if ( dwLastError == ERROR_SUCCESS ) {
               dwRet = dwCount;
            } else {
               SetLastError( dwLastError );
            }
         }
      } else {
         SetLastError( ERROR_TIMEOUT );
      }
   } else {
      SetLastError( ERROR_INVALID_USER_BUFFER );
   }
   
   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_Write ret: %lu\n", dwRet ) );
   return dwRet;
}

/**
@func ULONG | BTE_Write | Allows application to transmit bytes to the serial port.
@parm HANDLE | hOpenContext | Context pointer returned from BTE_Init.
@parm LONG | lPosition | Position to seek to ( relative to type ).
@parm DWORD | dwType | FILE_BEGIN, FILE_CURRENT, or FILE_END
@rdesc Returns current position relative to start of file, or INVALID_SET_FILE_POINTER on error.
@remark Routine exported by a device driver.  
*/
DWORD BTE_Seek( DWORD hOpenContext, long lAmount, WORD wType ) 
{
   IFDBG( DebugOut( DEBUG_OUTPUT, L"+BTE_Seek\n" ) );

   DWORD dwRet = INVALID_SET_FILE_POINTER;   
   SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
   
   IFDBG( DebugOut( DEBUG_OUTPUT, L"-BTE_Seek ret: %lu\n", dwRet ) );
   return dwRet;
}