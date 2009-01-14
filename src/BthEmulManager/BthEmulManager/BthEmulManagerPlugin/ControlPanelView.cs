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

namespace BthEmul.View
{
    using System;
    using System.Windows.Forms;

    using Microsoft.RemoteToolSdk.PluginComponents;
    using BthEmul.Data;

    /// <summary>
    /// Plugin view class for My Plugin.
    /// </summary>
    public class ControlPanelView : PluginDataView
    {
        private Panel panel1;
        private ListBox lbCommLog;
        private ContextMenuStrip contextMenuStrip1;
        private System.ComponentModel.IContainer components;
        private ToolStripMenuItem miClearAllToolStripMenuItem;
        private Label lblRemoteCPU;
        private Label lblRemoteVersion;
        private Label lblRemoteDescription;
        private Label lblHardwareState;
        private PictureBox pbHardwareState;
        private Label lblRemotePlatform;
        private Panel panel2;
        private CheckBox cbCommLogging;
        private CheckBox cbDesktopLogging;
        private CheckBox cbDeviceLogging;
        private Label lblLMPVersion;
        private Label lblHCIVersion;
        private Label lblManufacturer;
        private Label lblBtAddr;
        private Label lblTitle;        

        /// <summary>
        /// Construct a view. This object is created on the primary UI thread.
        /// </summary>
        /// <param name="data">Plugin data object</param>
        public ControlPanelView(ControlPanelData data) : base(data)
        {
            InitializeComponent();            
        }

        /// <summary>
        /// Initialize view controls
        /// </summary>
        /// <remarks>
        /// This method is called right before the view is
        /// to be rendered for the first time. It is guaranteed
        /// to be running on the primary UI thread, so you do not
        /// need to Invoke.
        ///
        /// You may also use the designer to layout your controls. If you
        /// move the call to InitializeComponent() to this method, you
        /// can improve plugin load-time, as the child controls will not
        /// get created until they are needed.
        /// </remarks>
        protected override void OnBuildControls()
        {
        }

        /// <summary>
        /// Fill in control data
        /// </summary>
        /// <param name="hint">Additional information for controls</param>
        /// <remarks>
        /// This method is called on the primary UI thread whenever the
        /// Remote Tool Framework needs to refresh the view to reflect
        /// data changes. The hint parameter is set to null if the
        /// Remote Tools Framework generated this method call. The
        /// data objects can also call their RenderViews method, which
        /// will cause this method to be called on all views that are
        /// hooked up to the data. RenderViews can set the hint parameter
        /// to whatever you like.
        /// </remarks>
        protected override void OnPopulateControls(object hint)
        {
            ControlPanelData data = (ControlPanelData)this.Data;
            
            System.Drawing.Bitmap image = Resources.bluetooth_disabled_72x72;
            string state = Resources.HardwareDetached;
            switch (data.HardwareState)
            {
                case HARDWARE_STATE.ATTACHED:
                    state = Resources.HardwareAttached;
                    image = Resources.bluetooth_regular_72x72;
                    break;
                case HARDWARE_STATE.DETACHED:
                    state = Resources.HardwareDetached;
                    image = Resources.bluetooth_disabled_72x72;
                    break;
                case HARDWARE_STATE.UNAVAILABLE:
                    state = string.Format("{0} ({1} {2})", Resources.HardwareUnavailable, data.HardwareErrorCode, data.HardwareErrorMessage);
                    image = Resources.bluetooth_disabled_72x72;
                    break;
            }
            lblHardwareState.Text = state;
            pbHardwareState.Image = image;

            cbDeviceLogging.Checked = data.DeviceLogging;
            cbDesktopLogging.Checked = data.DesktopLogging;
            cbCommLogging.Checked = data.CommLogging;

            int index = this.lbCommLog.SelectedIndex;
            bool lastIndex = (index == lbCommLog.Items.Count - 1);
            lbCommLog.Items.Clear();

            for (int i = 0; i < data.CommLog.Count; ++i)
            {
                lbCommLog.Items.Add(data.CommLog[i]);
            }
            lbCommLog.SelectedIndex = lastIndex ? lbCommLog.Items.Count - 1 : index;
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.panel1 = new System.Windows.Forms.Panel();
            this.lblLMPVersion = new System.Windows.Forms.Label();
            this.lblHCIVersion = new System.Windows.Forms.Label();
            this.lblManufacturer = new System.Windows.Forms.Label();
            this.lblBtAddr = new System.Windows.Forms.Label();
            this.panel2 = new System.Windows.Forms.Panel();
            this.cbCommLogging = new System.Windows.Forms.CheckBox();
            this.cbDesktopLogging = new System.Windows.Forms.CheckBox();
            this.cbDeviceLogging = new System.Windows.Forms.CheckBox();
            this.lblRemotePlatform = new System.Windows.Forms.Label();
            this.lblRemoteCPU = new System.Windows.Forms.Label();
            this.lblRemoteVersion = new System.Windows.Forms.Label();
            this.lblRemoteDescription = new System.Windows.Forms.Label();
            this.lblHardwareState = new System.Windows.Forms.Label();
            this.pbHardwareState = new System.Windows.Forms.PictureBox();
            this.lblTitle = new System.Windows.Forms.Label();
            this.lbCommLog = new System.Windows.Forms.ListBox();
            this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.miClearAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.panel1.SuspendLayout();
            this.panel2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbHardwareState)).BeginInit();
            this.contextMenuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.lblLMPVersion);
            this.panel1.Controls.Add(this.lblHCIVersion);
            this.panel1.Controls.Add(this.lblManufacturer);
            this.panel1.Controls.Add(this.lblBtAddr);
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Controls.Add(this.lblRemotePlatform);
            this.panel1.Controls.Add(this.lblRemoteCPU);
            this.panel1.Controls.Add(this.lblRemoteVersion);
            this.panel1.Controls.Add(this.lblRemoteDescription);
            this.panel1.Controls.Add(this.lblHardwareState);
            this.panel1.Controls.Add(this.pbHardwareState);
            this.panel1.Controls.Add(this.lblTitle);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(850, 185);
            this.panel1.TabIndex = 7;
            // 
            // lblLMPVersion
            // 
            this.lblLMPVersion.AutoSize = true;
            this.lblLMPVersion.Location = new System.Drawing.Point(106, 89);
            this.lblLMPVersion.Name = "lblLMPVersion";
            this.lblLMPVersion.Size = new System.Drawing.Size(74, 13);
            this.lblLMPVersion.TabIndex = 31;
            this.lblLMPVersion.Text = "lblLMPVersion";
            // 
            // lblHCIVersion
            // 
            this.lblHCIVersion.AutoSize = true;
            this.lblHCIVersion.Location = new System.Drawing.Point(106, 71);
            this.lblHCIVersion.Name = "lblHCIVersion";
            this.lblHCIVersion.Size = new System.Drawing.Size(70, 13);
            this.lblHCIVersion.TabIndex = 30;
            this.lblHCIVersion.Text = "lblHCIVersion";
            // 
            // lblManufacturer
            // 
            this.lblManufacturer.AutoSize = true;
            this.lblManufacturer.Location = new System.Drawing.Point(106, 53);
            this.lblManufacturer.Name = "lblManufacturer";
            this.lblManufacturer.Size = new System.Drawing.Size(80, 13);
            this.lblManufacturer.TabIndex = 29;
            this.lblManufacturer.Text = "lblManufacturer";
            // 
            // lblBtAddr
            // 
            this.lblBtAddr.AutoSize = true;
            this.lblBtAddr.Location = new System.Drawing.Point(106, 35);
            this.lblBtAddr.Name = "lblBtAddr";
            this.lblBtAddr.Size = new System.Drawing.Size(49, 13);
            this.lblBtAddr.TabIndex = 28;
            this.lblBtAddr.Text = "lblBtAddr";
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.cbCommLogging);
            this.panel2.Controls.Add(this.cbDesktopLogging);
            this.panel2.Controls.Add(this.cbDeviceLogging);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel2.Location = new System.Drawing.Point(0, 120);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(813, 65);
            this.panel2.TabIndex = 27;
            // 
            // cbCommLogging
            // 
            this.cbCommLogging.AutoSize = true;
            this.cbCommLogging.Location = new System.Drawing.Point(3, 43);
            this.cbCommLogging.Name = "cbCommLogging";
            this.cbCommLogging.Size = new System.Drawing.Size(105, 17);
            this.cbCommLogging.TabIndex = 25;
            this.cbCommLogging.Text = "cbCommLogging";
            this.cbCommLogging.UseVisualStyleBackColor = true;
            this.cbCommLogging.CheckedChanged += new System.EventHandler(this.cbCommLogging_CheckedChanged);
            // 
            // cbDesktopLogging
            // 
            this.cbDesktopLogging.AutoSize = true;
            this.cbDesktopLogging.Location = new System.Drawing.Point(3, 23);
            this.cbDesktopLogging.Name = "cbDesktopLogging";
            this.cbDesktopLogging.Size = new System.Drawing.Size(116, 17);
            this.cbDesktopLogging.TabIndex = 24;
            this.cbDesktopLogging.Text = "cbDesktopLogging";
            this.cbDesktopLogging.UseVisualStyleBackColor = true;
            this.cbDesktopLogging.CheckedChanged += new System.EventHandler(this.cbDesktopLogging_CheckedChanged);
            // 
            // cbDeviceLogging
            // 
            this.cbDeviceLogging.AutoSize = true;
            this.cbDeviceLogging.Location = new System.Drawing.Point(3, 3);
            this.cbDeviceLogging.Name = "cbDeviceLogging";
            this.cbDeviceLogging.Size = new System.Drawing.Size(110, 17);
            this.cbDeviceLogging.TabIndex = 23;
            this.cbDeviceLogging.Text = "cbDeviceLogging";
            this.cbDeviceLogging.UseVisualStyleBackColor = true;
            this.cbDeviceLogging.CheckedChanged += new System.EventHandler(this.cbDeviceLogging_CheckedChanged);
            // 
            // lblRemotePlatform
            // 
            this.lblRemotePlatform.AutoSize = true;
            this.lblRemotePlatform.Location = new System.Drawing.Point(435, 35);
            this.lblRemotePlatform.Name = "lblRemotePlatform";
            this.lblRemotePlatform.Size = new System.Drawing.Size(92, 13);
            this.lblRemotePlatform.TabIndex = 26;
            this.lblRemotePlatform.Text = "lblRemotePlatform";
            // 
            // lblRemoteCPU
            // 
            this.lblRemoteCPU.AutoSize = true;
            this.lblRemoteCPU.Location = new System.Drawing.Point(435, 89);
            this.lblRemoteCPU.Name = "lblRemoteCPU";
            this.lblRemoteCPU.Size = new System.Drawing.Size(76, 13);
            this.lblRemoteCPU.TabIndex = 25;
            this.lblRemoteCPU.Text = "lblRemoteCPU";
            // 
            // lblRemoteVersion
            // 
            this.lblRemoteVersion.AutoSize = true;
            this.lblRemoteVersion.Location = new System.Drawing.Point(435, 71);
            this.lblRemoteVersion.Name = "lblRemoteVersion";
            this.lblRemoteVersion.Size = new System.Drawing.Size(89, 13);
            this.lblRemoteVersion.TabIndex = 24;
            this.lblRemoteVersion.Text = "lblRemoteVersion";
            // 
            // lblRemoteDescription
            // 
            this.lblRemoteDescription.AutoSize = true;
            this.lblRemoteDescription.Location = new System.Drawing.Point(435, 53);
            this.lblRemoteDescription.Name = "lblRemoteDescription";
            this.lblRemoteDescription.Size = new System.Drawing.Size(107, 13);
            this.lblRemoteDescription.TabIndex = 23;
            this.lblRemoteDescription.Text = "lblRemoteDescription";
            // 
            // lblHardwareState
            // 
            this.lblHardwareState.AutoSize = true;
            this.lblHardwareState.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.lblHardwareState.Location = new System.Drawing.Point(106, 17);
            this.lblHardwareState.Name = "lblHardwareState";
            this.lblHardwareState.Size = new System.Drawing.Size(104, 13);
            this.lblHardwareState.TabIndex = 19;
            this.lblHardwareState.Text = "lblHardwareState";
            // 
            // pbHardwareState
            // 
            this.pbHardwareState.Location = new System.Drawing.Point(17, 17);
            this.pbHardwareState.Name = "pbHardwareState";
            this.pbHardwareState.Size = new System.Drawing.Size(57, 50);
            this.pbHardwareState.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.pbHardwareState.TabIndex = 18;
            this.pbHardwareState.TabStop = false;
            // 
            // lblTitle
            // 
            this.lblTitle.AutoSize = true;
            this.lblTitle.Dock = System.Windows.Forms.DockStyle.Right;
            this.lblTitle.Location = new System.Drawing.Point(813, 0);
            this.lblTitle.Name = "lblTitle";
            this.lblTitle.Size = new System.Drawing.Size(37, 13);
            this.lblTitle.TabIndex = 12;
            this.lblTitle.Text = "lblTitle";
            // 
            // lbCommLog
            // 
            this.lbCommLog.ContextMenuStrip = this.contextMenuStrip1;
            this.lbCommLog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lbCommLog.FormattingEnabled = true;
            this.lbCommLog.Location = new System.Drawing.Point(0, 185);
            this.lbCommLog.Name = "lbCommLog";
            this.lbCommLog.Size = new System.Drawing.Size(850, 225);
            this.lbCommLog.TabIndex = 8;
            // 
            // contextMenuStrip1
            // 
            this.contextMenuStrip1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.contextMenuStrip1.ImageScalingSize = new System.Drawing.Size(8, 8);
            this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.miClearAllToolStripMenuItem});
            this.contextMenuStrip1.Name = "contextMenuStrip1";
            this.contextMenuStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.contextMenuStrip1.ShowImageMargin = false;
            this.contextMenuStrip1.Size = new System.Drawing.Size(98, 26);
            // 
            // miClearAllToolStripMenuItem
            // 
            this.miClearAllToolStripMenuItem.Name = "miClearAllToolStripMenuItem";
            this.miClearAllToolStripMenuItem.Size = new System.Drawing.Size(97, 22);
            this.miClearAllToolStripMenuItem.Text = "miClearAll";
            this.miClearAllToolStripMenuItem.Click += new System.EventHandler(this.miClearAllToolStripMenuItem_Click);
            // 
            // ControlPanelView
            // 
            this.Controls.Add(this.lbCommLog);
            this.Controls.Add(this.panel1);
            this.Name = "ControlPanelView";
            this.Size = new System.Drawing.Size(850, 417);
            this.Load += new System.EventHandler(this.ControlPanelView_Load);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbHardwareState)).EndInit();
            this.contextMenuStrip1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        void cbDeviceLogging_CheckedChanged(object sender, EventArgs e)
        {
            ((ControlPanelData)this.Data).DeviceLogging = cbDeviceLogging.Checked;
        }

        private void cbDesktopLogging_CheckedChanged(object sender, EventArgs e)
        {
            ((ControlPanelData)this.Data).DesktopLogging = cbDesktopLogging.Checked;
        }

        private void cbCommLogging_CheckedChanged(object sender, EventArgs e)
        {
            ((ControlPanelData)this.Data).CommLogging = cbCommLogging.Checked;
        }

        private void miClearAllToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ControlPanelData data = (ControlPanelData)this.Data;
            data.ClearCommLog();            
        }

        private void ControlPanelView_Load( object sender, EventArgs e )
        {
            ControlPanelData data = (ControlPanelData)this.Data;

            lblTitle.Text = string.Format("{0} {1}", Resources.Title, GlobalData.Version);
            cbDeviceLogging.Text = Resources.DeviceLogging;
            cbDesktopLogging.Text = Resources.DesktopLogging;
            cbCommLogging.Text = Resources.CommLogging;
            miClearAllToolStripMenuItem.Text = Resources.ClearAll;

            BT_ADDR btAddr = data.DeviceInfo.btAddr;
            lblBtAddr.Text = string.Format("{0}: {1:X2}:{2:X2}:{3:X2}:{4:X2}:{5:X2}:{6:X2}", Resources.Address, btAddr.btAddr5, btAddr.btAddr4, btAddr.btAddr3, btAddr.btAddr2, btAddr.btAddr1, btAddr.btAddr0);
            lblHCIVersion.Text = string.Format("{0}: {1}.{2:00}", Resources.HCIVersion, data.DeviceInfo.hciVersion, data.DeviceInfo.hciRevision);
            lblLMPVersion.Text = string.Format("{0}: {1}.{2:00}", Resources.LMPVersion, data.DeviceInfo.lmpVersion, data.DeviceInfo.lmpSubVersion);
            lblManufacturer.Text = string.Format("{0}: {1}", Resources.Manufacturer, data.Manufacturer);

            lblRemotePlatform.Text = string.Format("{0}: {1}", Resources.Platform, data.RemotePlatform);
            lblRemoteDescription.Text = string.Format("{0}: {1}", Resources.Description2, data.RemoteDescription);
            lblRemoteVersion.Text = string.Format("{0}: {1}", Resources.Version, data.RemoteVersion);
            lblRemoteCPU.Text = string.Format("{0}: {1}", Resources.CPU, data.RemoteCPU);
        }        
    }
}
