/**
 *   This file is part of Bluetooth for Microsoft Device Emulator
 * 
 *   Copyright (C) 2008 Dmitry Klionsky <dm.klionsky@gmail.com>
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

using System;
using System.Runtime.InteropServices;

namespace BthEmul
{
    public enum HARDWARE_STATE
    {
        ATTACHED,
        DETACHED,
        UNAVAILABLE
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct BT_ADDR
    {
        public byte btAddr0;
        public byte btAddr1;
        public byte btAddr2;
        public byte btAddr3;
        public byte btAddr4;
        public byte btAddr5;        
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct DEVICE_INFO
    {
        public BT_ADDR btAddr;
        public byte hciVersion;
        public ushort hciRevision;
        public byte lmpVersion;
        public ushort manufacturer;
        public ushort lmpSubVersion;
    }
    
    enum HCI_TYPE
    {
        COMMAND_PACKET = 1,
        DATA_PACKET_ACL = 2,
        DATA_PACKET_SCO = 3,
        EVENT_PACKET = 4,
        ETYPE_FINISH = 5
    };

    enum PACKET_TYPE
    {
        HCI_DATA_PACKET = 0,
        HCI_DATA_ERROR_PACKET,
        MESSAGE_PACKET
    };

    // messages ids for PACKET_TYPE::MESSAGE_PACKET
    enum MESSAGE_ID
    {
        // agent related messages
        AGENT_ACK_MSG = 0,
        AGENT_PING_MSG,
        AGENT_INITIALIZE_MSG,
        AGENT_UNINITIALIZE_MSG,
        AGENT_LOGGING_ON_MSG,
        AGENT_LOGGING_OFF_MSG,

        // communication related messages
        COM_OPEN_MSG,
        COM_CLOSE_MSG
    };

    class BthRuntime
    {
        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int OpenDevice();

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int CloseDevice(int devId);

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int SendHCICommand(int devId, [MarshalAs(UnmanagedType.LPArray)] byte[] cmdBuf, uint cmdLen);

        [DllImport("fbtrt.dll", SetLastError=true)]
        public static extern int GetDeviceInfo(int devId, ref DEVICE_INFO devInfo);

        [DllImport("fbtrt.dll", SetLastError=true, CharSet=CharSet.Unicode)]
        public static extern string GetManufacturerName(ushort manufacturer);

        public delegate int HciEventListenerDelegate(IntPtr pEventBuf, uint eventLen);

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int SubscribeHCIEvent(int devId, HciEventListenerDelegate hciEventListener);

        [DllImport("fbtrt.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int SetLogFileName(string fileName);

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int SetLogLevel(int level);
    }
}
