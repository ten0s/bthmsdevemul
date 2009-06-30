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

namespace BthEmul
{
    using System;
    using System.Text;
    using System.Timers;
    using System.Runtime.InteropServices;
    
    using Microsoft.RemoteToolSdk.PluginComponents;
    using BthEmul.Data;
    using BthEmul.View;

    /// <summary>
    /// </summary>
    public class PluginClass : PluginComponent
    {
        /// <summary>
        /// Guid for top level tree node
        /// </summary>
        private string guidTopLevelNode = "2A7B2FDB-D92D-4438-9428-06E37CCB1A6A";        
        
        private BthRuntime.HciEventListenerDelegate hciEventListener = null;
        private System.Timers.Timer watchDogTimer = null;
        
        // These are object(s) that contain the actual data uploaded from the device.
        // The data classes must implement a OnGetData() method, which is the function that
        // is responsible for filling these object(s) with data.
        //
        // The user is free to implement these data objects however they want, as long
        // as they are based on the PluginData class. That class provides a
        // OnGetData method to override, as well as other data-centric functions.

        private PluginNode topNode = null;

        /// <summary>
        /// Plugin control panel data object
        /// </summary>
        private ControlPanelData ctrlPanelData = null;

        /// <summary>
        /// Plugin control panel view object
        /// </summary>
        private ControlPanelView ctrlPanelView = null;

        private int devId = BthRuntime.INVALID_DEVICE_ID;
        
        /// <summary>
        /// Constructor for this object. You can set some attributes here.
        /// </summary>
        /// <param name="deviceGuid">Guid for device</param>
        /// <param name="pluginGuid">Guid for plugin</param>
        public PluginClass(string deviceGuid, string pluginGuid) : base(deviceGuid, pluginGuid)
        {
            
        }      

        /// <summary>
        /// Called by the Remote Tools Framework at load-time for the plugin.
        /// You need to define the node structure and create your data objects here.
        /// </summary>
        protected override void OnInit()
        {
            // build data objects.
            ctrlPanelData = new ControlPanelData(this, guidTopLevelNode);
            ctrlPanelData.DeviceLoggingChanged += new LoggingChangedEventHandler(controlPanelData_DeviceLoggingChanged);
            ctrlPanelData.DesktopLoggingChanged += new LoggingChangedEventHandler(controlPanelData_DesktopLoggingChanged);
            ctrlPanelData.CommLoggingChanged += new LoggingChangedEventHandler(controlPanelData_CommLoggingChanged);            

            AddData(ctrlPanelData);

            // build view objects.
            ctrlPanelView = new ControlPanelView(ctrlPanelData);

            // If the plugin is running "standalone", that means that it does not need
            // to share the TreeView control in the shell with anyone else. It would
            // be redundant to add a top node to identify the plugin (the one with a
            // hammer and wrench).
            if (Standalone)
            {
                guidTopLevelNode = null;
            }
            else
            {
                // Otherwise... this plugin is sharing the TreeView with other plugins,
                // and therefore should add a top level node to idenifty the plugin.
                //
                // This will add a top level node by setting the guid for the parent to null.
                // There is no view panel associated with this node.
                topNode = new PluginNode(Title, guidTopLevelNode, null, ctrlPanelData, ctrlPanelView, this, BuiltInIcon.Custom);
                topNode.CustomIcon = Resources.bluetooth_regular;
                AddNode(topNode);                
            }

            // add "About" menu item.
            CommandUICommand cmdAbout = new CommandUICommand(CommandUI.CommandRoot.HelpMenu, Resources.About);
            cmdAbout.Clicked += new EventHandler(cmdAbout_Clicked);
            CommandUI.PluginCommands.Add(cmdAbout);

            OpenDevice();
        }

        void OpenDevice()
        {
            devId = ctrlPanelData.OpenDevice();
            if (devId != BthRuntime.INVALID_DEVICE_ID)
            {
                // subscribe to hci events.
                hciEventListener = new BthRuntime.HciEventListenerDelegate(OnHciEvent);
                BthRuntime.SubscribeHCIEvent(devId, hciEventListener);

                if (watchDogTimer != null)
                {
                    watchDogTimer.Stop();
                    watchDogTimer = null;
                }

                // start watchdog timer.
                watchDogTimer = new System.Timers.Timer();
                watchDogTimer.Elapsed += new ElapsedEventHandler(WatchDogTimerEvent);
                watchDogTimer.Interval = 5000;
                watchDogTimer.Start();
            }            
        }

        void controlPanelData_CommLoggingChanged(object sender, EventArgs e)
        {
            ctrlPanelData.ClearCommLog();
        }

        void controlPanelData_DesktopLoggingChanged(object sender, EventArgs e)
        {
            int level = ctrlPanelData.DesktopLogging ? 255 : 0;
            BthRuntime.SetLogLevel(level);
        }

        void controlPanelData_DeviceLoggingChanged(object sender, EventArgs e)
        {
            SendDeviceLogging();
        }

        private void cmdAbout_Clicked(object sender, EventArgs e)
        {
            AboutForm frm = new AboutForm();
            frm.ShowDialog();
        }

        private void AddCommLog(string log)
        {
            if (ctrlPanelData.CommLogging)
            {
                ctrlPanelData.CommLog.Add(log);
                ctrlPanelData.RenderViews(null);
            }            
        }

        private void SendDeviceLogging()
        {
            CommandPacket cmd = new CommandPacket();
            cmd.CommandId = (uint)PACKET_TYPE.MESSAGE_PACKET;
            MESSAGE_ID cmdId = ctrlPanelData.DeviceLogging ? MESSAGE_ID.AGENT_LOGGING_ON_MSG : MESSAGE_ID.AGENT_LOGGING_OFF_MSG;
            cmd.AddParameterDWORD((uint)cmdId);
            SendCommand(cmd);
        }

        private void AddCommLog(string packetType, byte[] bytes, int length, string result)
        {
            if (ctrlPanelData.CommLogging)
            {
                string packetData = BytesToHex(bytes, length);
                AddCommLog(string.Format(GlobalData.LOG_FORMAT, packetType, packetData, result));
            }
        }

        private static string BytesToHex(byte[] bytes, int length)
        {
            StringBuilder hexString = new StringBuilder(length);
            
            for (int i = 0; i < length; ++i)
            {
                hexString.Append(bytes[i].ToString("x2"));
            }

            return hexString.ToString();
        }

        private int OnHciEvent(IntPtr pEventBuf, uint eventLen)
        {
            byte[] eventBuf = new byte[eventLen];
            Marshal.Copy(pEventBuf, eventBuf, 0, (int)eventLen);

            SendHCIEvent(eventBuf);

            return 0;
        }

        private void SendHCIEvent(byte[] bytes)
        {
            AddCommLog(GlobalData.EVENT_PACKET, bytes, bytes.Length, GlobalData.OK);

            // send to device...
            CommandPacket cmd = new CommandPacket();
            cmd.CommandId = (uint)PACKET_TYPE.HCI_DATA_PACKET;
            cmd.AddParameterBytes(bytes);
            SendCommand(cmd);
        }

        private bool SendCommand(CommandPacket cmd)
        {
            if (CommandTransport != null && 
                CommandTransport.ConnectionState == CommandTransport.ConnectState.Connected &&
                BthRuntime.INVALID_DEVICE_ID != devId)
            {
                // send to device...
                CommandTransport.ProcessCommand(cmd, ctrlPanelData);
                return true;
            }

            return false;
        }
              
        private void WatchDogTimerEvent(object source, ElapsedEventArgs e)
        {
            if (Connected)
            {
                // send to device...
                CommandPacket cmd = new CommandPacket();
                cmd.CommandId = (uint)PACKET_TYPE.MESSAGE_PACKET;
                cmd.AddParameterDWORD((uint)MESSAGE_ID.AGENT_PING_MSG);
                SendCommand(cmd);                
            }
            else
            {
                ctrlPanelData.CloseDevice();
            }
        }

        private bool OnDeviceDataReceived(byte[] bytes)
        {
            int length = bytes.Length;
            if (length > 0) 
            {
                HCI_TYPE hciType = (HCI_TYPE)bytes[0];

                // send command/data to bluetooth device...
                int lastError = 0;
                int ret = BthRuntime.SendHCICommand(devId, bytes, (uint)bytes.Length);

                string result = GlobalData.OK;
                if (ret == 0)
                {
                    lastError = Marshal.GetLastWin32Error();
                    string errMsg = new System.ComponentModel.Win32Exception(lastError).Message;
                    result = string.Format("Fail: {0} ({1})", lastError, errMsg);
                }

                // send error code to device...
                CommandPacket cmd = new CommandPacket();
                cmd.CommandId = (uint)PACKET_TYPE.HCI_DATA_ERROR_PACKET;
                cmd.AddParameterDWORD((uint)lastError);
                SendCommand(cmd);

                // add log entry.
                AddCommLog(hciType.ToString(), bytes, bytes.Length, result);
            }

            return false;            
        }

        protected override void OnStart()
        {
            // send the first message to device...
            CommandPacket cmd = new CommandPacket();
            cmd.CommandId = (uint)PACKET_TYPE.MESSAGE_PACKET;
            cmd.AddParameterDWORD((uint)MESSAGE_ID.AGENT_ACK_MSG);
            SendCommand(cmd);
        }

        /// <summary>
        /// Received a command packet
        /// </summary>
        /// <param name="commandPacket">Command packet received</param>
        protected override void OnCommandPacketReceived(CommandPacket commandPacket)
        {
            if (commandPacket == null)
                return;
            
            PACKET_TYPE packetType = (PACKET_TYPE)commandPacket.CommandId;            
            switch (packetType)
            {
                case PACKET_TYPE.MESSAGE_PACKET:
                    {
                        MESSAGE_ID msgId = (MESSAGE_ID)commandPacket.GetParameterDWORD();
                        CommandPacket cmd = new CommandPacket();
                        cmd.CommandId = (uint)PACKET_TYPE.MESSAGE_PACKET;

                        AddCommLog(msgId.ToString());
                        switch (msgId)
                        {
                            case MESSAGE_ID.AGENT_ACK_MSG:
                                cmd.AddParameterDWORD((uint)MESSAGE_ID.AGENT_INITIALIZE_MSG);
                                SendCommand(cmd);        
                                break;

                            case MESSAGE_ID.AGENT_INITIALIZE_MSG:
                                SendDeviceLogging();
                                break;

                            case MESSAGE_ID.AGENT_UNINITIALIZE_MSG:
                                cmd.AddParameterDWORD((uint)MESSAGE_ID.AGENT_INITIALIZE_MSG);
                                SendCommand(cmd);
                                break;

                            case MESSAGE_ID.AGENT_LOGGING_ON_MSG:
                                break;

                            case MESSAGE_ID.AGENT_LOGGING_OFF_MSG:
                                break;

                            case MESSAGE_ID.COM_OPEN_MSG:
                                break;

                            case MESSAGE_ID.AGENT_PING_MSG:
                                break;

                            case MESSAGE_ID.COM_CLOSE_MSG:
                                cmd.AddParameterDWORD((uint)MESSAGE_ID.AGENT_UNINITIALIZE_MSG);
                                SendCommand(cmd);
                                break;
                        }
                    }
                    break;               

                case PACKET_TYPE.HCI_DATA_PACKET:
                    {
                        CommandPacketParameter cpp;
                        while ((cpp = commandPacket.GetNextParameter()) != null)
                        {
                            switch (cpp.Type)
                            {
                                case CommandPacketParameterType.Bytes:
                                    OnDeviceDataReceived(cpp.BytesParameter);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    break;               
            }            
        }        
    }
}
