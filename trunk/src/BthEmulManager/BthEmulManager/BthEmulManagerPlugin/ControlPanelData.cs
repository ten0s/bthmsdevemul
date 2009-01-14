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
        
        /// <summary>
        /// Constructor: 
        /// </summary>
        /// <param name="host">Plugin owning this data</param>
        /// <param name="guid">Guid of the node owning this data</param>
        public ControlPanelData( PluginComponent host, string guid )
            : base( host, guid )
        {
            this.InitDataAtViewTime = false;
            this.HardwareState = HARDWARE_STATE.DETACHED;
            this.settings = new Settings();
            this.commLog = new ArrayList();
            this.deviceInfo = new DEVICE_INFO();

            settingsPath = host.SettingsPath;
            this.settings.Deserialize(settingsPath);            
        }

        ~ControlPanelData()
        {
            this.settings.Serialize(this.settingsPath);        
        }

        public HARDWARE_STATE HardwareState
        {
            get { return this.hardwareState;  }
            set { this.hardwareState = value;  }
        }

        public bool DeviceLogging
        {
            get { return this.settings.DeviceLogging; }
            set
            {
                if (value != this.settings.DeviceLogging)
                {
                    this.settings.DeviceLogging = value;

                    if (DeviceLoggingChanged != null)
                    {
                        DeviceLoggingChanged(this, EventArgs.Empty);
                    }
                }                
            }
        }

        public bool DesktopLogging
        {
            get { return this.settings.DesktopLogging; }
            set
            {
                if (value != this.settings.DesktopLogging)
                {
                    this.settings.DesktopLogging = value;

                    if (DesktopLoggingChanged != null)
                    {
                        DesktopLoggingChanged(this, EventArgs.Empty);
                    }
                }
            }
        }

        public bool CommLogging
        {
            get { return this.settings.CommLogging; }
            set
            {
                if (value != this.settings.CommLogging)
                {
                    this.settings.CommLogging = value;

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
            get { return this.commLog; }
            set { this.commLog = value; }
        }

        public int HardwareErrorCode
        {
            get { return this.hardwareErrorCode;  }
            set { this.hardwareErrorCode = value;  }
        }

        public string HardwareErrorMessage
        {
            get { return this.hardwareErrorMessage; }
            set { this.hardwareErrorMessage = value; }
        }

        public string RemotePlatform
        {
            get { return this.remotePlatform; }
            set { this.remotePlatform = value; }
        }

        public string RemoteDescription
        {
            get { return this.remoteDescription; }
            set { this.remoteDescription = value; }
        }

        public string RemoteCPU
        {
            get { return this.remoteCPU; }
            set { this.remoteCPU = value; }
        }

        public string RemoteVersion
        {
            get { return this.remoteVersion; }
            set { this.remoteVersion = value; }
        }

        public DEVICE_INFO DeviceInfo
        {
            get { return this.deviceInfo; }
            set { this.deviceInfo = value;  }
        }

        public string Manufacturer
        {
            get { return this.manufacturer;  }
            set { this.manufacturer = value;  }
        }

        public void ClearCommLog()
        {
            this.CommLog.Clear();
            RenderViews(null);
        }

        /// <summary>
        /// Retrieve data from the device and store it in the data items.
        /// </summary>
        protected override void OnGetData()
        {
            // By setting Initialized to true, the view panel(s) hooked up
            // to this data object will refresh.
            this.Initialized = true;
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
            dataAcceptor.AddItem(category, this.HardwareState.ToString(), "");
            if (this.HardwareState == HARDWARE_STATE.UNAVAILABLE)
            {
                dataAcceptor.AddItem(category, "ErrorCode", this.HardwareErrorCode.ToString());
                dataAcceptor.AddItem(category, "ErrorMessage", this.HardwareErrorMessage);
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
            for (int i = 0; i < this.commLog.Count; i++)
            {
                dataAcceptor.AddItem(category, i.ToString(), this.commLog[i].ToString());
            }
        }
    }
}
