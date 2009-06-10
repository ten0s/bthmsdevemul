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

namespace BthEmul.Data
{
    using System;
    using System.Collections;
    using System.Timers;
    using System.Runtime.InteropServices;

    using Microsoft.RemoteToolSdk.PluginComponents;
    using BthEmul.View;

    // A delegate type for hooking up logging change notifications.
    public delegate void LoggingChangedEventHandler(object sender, EventArgs e);
    
    /// <summary>
    /// The device side app expects a command with a value of 1 sent to it,
    /// where it will then fill the return packet with some dummy data.
    /// </summary>
    public class ControlPanelData : PluginData
    {
        private HARDWARE_STATE hardwareState;
        private Settings settings;
        private ArrayList commLog;
        private int hardwareErrorCode;
        private string hardwareErrorMessage;
        private string settingsPath;
        private string remotePlatform;
        private string remoteDescription;
        private string remoteCPU;
        private string remoteVersion;
        private DEVICE_INFO deviceInfo;
        private string manufacturer;
        private int devId = BthRuntime.INVALID_DEVICE_ID;
        private System.Timers.Timer connectionMonitorTimer = null;
        
        /// <summary>
        /// Constructor: 
        /// </summary>
        /// <param name="host">Plugin owning this data</param>
        /// <param name="guid">Guid of the node owning this data</param>
        public ControlPanelData(PluginComponent host, string guid)
            : base(host, guid)
        {
            InitDataAtViewTime = false;
            HardwareState = HARDWARE_STATE.DETACHED;
            settings = new Settings();
            commLog = new ArrayList();
            deviceInfo = new DEVICE_INFO();

            // query remote device info.
            DeviceDatbaseItem item = Host.DeviceDatabase.FindByGuid(Host.DeviceConnection.Guid);
            RemotePlatform = item.Platform;
            RemoteDescription = item.Description;
            RemoteVersion = string.Format("{0}.{1:00}", item.OsMajor, item.OsMinor);
            RemoteCPU = item.CPU;
            
            // restore settings from file.
            settingsPath = host.SettingsPath;
            settings.Deserialize(settingsPath);            
        }

        ~ControlPanelData()
        {
            CloseDevice();
            settings.Serialize(settingsPath);            
        }

        public HARDWARE_STATE HardwareState
        {
            get { return hardwareState;  }
            set { hardwareState = value;  }
        }

        public bool DeviceLogging
        {
            get { return settings.DeviceLogging; }
            set
            {
                if (value != settings.DeviceLogging)
                {
                    settings.DeviceLogging = value;

                    if (DeviceLoggingChanged != null)
                    {
                        DeviceLoggingChanged(this, EventArgs.Empty);
                    }
                }                
            }
        }

        public bool DesktopLogging
        {
            get { return settings.DesktopLogging; }
            set
            {
                if (value != settings.DesktopLogging)
                {
                    settings.DesktopLogging = value;

                    if (DesktopLoggingChanged != null)
                    {
                        DesktopLoggingChanged(this, EventArgs.Empty);
                    }
                }
            }
        }

        public bool CommLogging
        {
            get { return settings.CommLogging; }
            set
            {
                if (value != settings.CommLogging)
                {
                    settings.CommLogging = value;

                    if (CommLoggingChanged != null)
                    {
                        CommLoggingChanged(this, EventArgs.Empty);
                    }
                }
            }
        }

        public event LoggingChangedEventHandler DeviceLoggingChanged;
        public event LoggingChangedEventHandler DesktopLoggingChanged;
        public event LoggingChangedEventHandler CommLoggingChanged;        

        public ArrayList CommLog
        {
            get { return commLog; }
            set { commLog = value; }
        }

        public int HardwareErrorCode
        {
            get { return hardwareErrorCode;  }
            set { hardwareErrorCode = value;  }
        }

        public string HardwareErrorMessage
        {
            get { return hardwareErrorMessage; }
            set { hardwareErrorMessage = value; }
        }

        public string RemotePlatform
        {
            get { return remotePlatform; }
            set { remotePlatform = value; }
        }

        public string RemoteDescription
        {
            get { return remoteDescription; }
            set { remoteDescription = value; }
        }

        public string RemoteCPU
        {
            get { return remoteCPU; }
            set { remoteCPU = value; }
        }

        public string RemoteVersion
        {
            get { return remoteVersion; }
            set { remoteVersion = value; }
        }

        public DEVICE_INFO DeviceInfo
        {
            get { return deviceInfo; }
            set { deviceInfo = value;  }
        }

        public string Manufacturer
        {
            get { return manufacturer;  }
            set { manufacturer = value;  }
        }

        public void ClearCommLog()
        {
            CommLog.Clear();
            RenderViews(null);
        }

        public int OpenDevice()
        {
            BthRuntime.SetLogFileName("BthEmulManager.txt");

            int level = DesktopLogging ? 255 : 0;
            BthRuntime.SetLogLevel(level);

            devId = BthRuntime.OpenDevice();
            if (BthRuntime.INVALID_DEVICE_ID == devId)
            {
                HardwareState = HARDWARE_STATE.UNAVAILABLE;

                int lastError = Marshal.GetLastWin32Error();
                HardwareErrorCode = lastError;
                HardwareErrorMessage = new System.ComponentModel.Win32Exception(lastError).Message;
                RenderViews(null);
            }
            else
            {
                HardwareState = HARDWARE_STATE.ATTACHED;

                // get device info.
                DEVICE_INFO deviceInfo = new DEVICE_INFO();
                if (1 == BthRuntime.GetDeviceInfo(devId, ref deviceInfo))
                {
                    DeviceInfo = deviceInfo;
                    Manufacturer = BthRuntime.GetManufacturerName(deviceInfo.manufacturer);
                }

                // start connection monitor timer.
                connectionMonitorTimer = new System.Timers.Timer();
                connectionMonitorTimer.Elapsed += new ElapsedEventHandler(ConnectionMonitorTimerEvent);
                connectionMonitorTimer.Interval = 3000;
                connectionMonitorTimer.Start();
            }

            return devId;
        }

        public void CloseDevice()
        {
            if (BthRuntime.INVALID_DEVICE_ID != devId)
            {
                BthRuntime.CloseDevice(devId);
                devId = BthRuntime.INVALID_DEVICE_ID;

                HardwareState = HARDWARE_STATE.DETACHED;
                RenderViews(null);

                if (connectionMonitorTimer != null)
                {
                    connectionMonitorTimer.Stop();
                    connectionMonitorTimer = null;
                }
            }
        }

        private void ConnectionMonitorTimerEvent(object source, ElapsedEventArgs e)
        {
            if (CommandTransport.ConnectionState == CommandTransport.ConnectState.Disconnected)
            {
                CloseDevice();
            }
        }

        /// <summary>
        /// Retrieve data from the device and store it in the data items.
        /// </summary>
        protected override void OnGetData()
        {
            // By setting Initialized to true, the view panel(s) hooked up
            // to this data object will refresh.
            Initialized = true;
        }

        /// <summary>
        /// Render this object's data items in a generic fashion.
        /// </summary>
        /// <param name="dataAcceptor">Data acceptor to render data items to
        /// </param>
        protected override void OnRenderGeneric(GenericDataAcceptor dataAcceptor)
        {
            // application version.
            dataAcceptor.AddItem("Version:", string.Format("{0} {1}", Resources.Title, GlobalData.Version), "");
            
            // bluetooth address.
            string category = "Hardware:";
            dataAcceptor.AddItem(category, HardwareState.ToString(), "");
            if (HardwareState == HARDWARE_STATE.UNAVAILABLE)
            {
                dataAcceptor.AddItem(category, "ErrorCode", HardwareErrorCode.ToString());
                dataAcceptor.AddItem(category, "ErrorMessage", HardwareErrorMessage);
            }
            dataAcceptor.AddItem("Address:", string.Format("{0:X2}:{1:X2}:{2:X2}:{3:X2}:{4:X2}:{5:X2}", DeviceInfo.btAddr.btAddr5, DeviceInfo.btAddr.btAddr4, DeviceInfo.btAddr.btAddr3, DeviceInfo.btAddr.btAddr2, DeviceInfo.btAddr.btAddr1, DeviceInfo.btAddr.btAddr0), "");
            dataAcceptor.AddItem("HCI Version:", string.Format("{0}.{1:00}", DeviceInfo.hciVersion, DeviceInfo.hciRevision), ""); 
            dataAcceptor.AddItem("LMP Version:", string.Format("{0}.{1:00}", DeviceInfo.lmpVersion, DeviceInfo.lmpSubVersion), "");
            dataAcceptor.AddItem("Manufacturer:", string.Format("{0} ({1})", DeviceInfo.manufacturer, Manufacturer), "");
            
            // remote device.
            dataAcceptor.AddItem("Platform:", RemotePlatform, "");
            dataAcceptor.AddItem("Description:", RemoteDescription, "");
            dataAcceptor.AddItem("Version:", RemoteVersion, "");
            dataAcceptor.AddItem("CPU:", RemoteCPU, "");

            // separator
            dataAcceptor.AddItem("", "", "");

            category = "Log:";
            for (int i = 0; i < commLog.Count; ++i)
            {
                dataAcceptor.AddItem(category, i.ToString(), commLog[i].ToString());
            }
        }
    }
}
