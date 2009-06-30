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

#ifndef __FBTRT_H__
#define __FBTRT_H__

#include <windows.h>

#define INVALID_DEVICE_ID -1

typedef struct {
   unsigned char bdaddr[6];
   unsigned char hci_ver;
   unsigned short hci_rev;
   unsigned char lmp_ver;
   unsigned short manufacturer;
   unsigned short lmp_subver;

} LOCAL_DEVICE_INFO;

#ifdef __cplusplus 
extern "C" {
#endif
   int __stdcall OpenDevice();
   BOOL __stdcall CloseDevice( int devId );

   BOOL __stdcall SendHCICommand( int devId, BYTE* /*in*/pCmdBuffer, DWORD dwCmdLength );
   BOOL __stdcall GetDeviceInfo( int devId, LOCAL_DEVICE_INFO* /*in*/pDevInfo );   
   BOOL __stdcall GetManufacturerName( USHORT usManufacturer, LPTSTR /*in*/szInBuffer, DWORD dwBufferLength );

   typedef DWORD ( __stdcall *HCI_EVENT_LISTENER)( BYTE* /*in*/pEventBuffer, DWORD dwEventLength );
   BOOL __stdcall SubscribeHCIEvent( int devId, HCI_EVENT_LISTENER hciEventListener );
   
   BOOL __stdcall SetLogFileName( LPCTSTR szFileName );
   BOOL __stdcall SetLogLevel( UINT uLevel );

#ifdef __cplusplus 
}
#endif

#endif //__FBTRT_H__
