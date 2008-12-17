namespace BthEmul
{
    partial class AboutForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.lblUrlAddress = new System.Windows.Forms.LinkLabel();
            this.lblTitle = new System.Windows.Forms.Label();
            this.lblCopyright = new System.Windows.Forms.Label();
            this.pbBluetooth = new System.Windows.Forms.PictureBox();
            this.lblDescription = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.pbBluetooth)).BeginInit();
            this.SuspendLayout();
            // 
            // lblUrlAddress
            // 
            this.lblUrlAddress.AutoSize = true;
            this.lblUrlAddress.Location = new System.Drawing.Point(153, 122);
            this.lblUrlAddress.Name = "lblUrlAddress";
            this.lblUrlAddress.Size = new System.Drawing.Size(68, 13);
            this.lblUrlAddress.TabIndex = 0;
            this.lblUrlAddress.TabStop = true;
            this.lblUrlAddress.Text = "lblUrlAddress";
            this.lblUrlAddress.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.lblUrlAddress_LinkClicked);
            // 
            // lblTitle
            // 
            this.lblTitle.AutoSize = true;
            this.lblTitle.Location = new System.Drawing.Point(153, 14);
            this.lblTitle.Name = "lblTitle";
            this.lblTitle.Size = new System.Drawing.Size(37, 13);
            this.lblTitle.TabIndex = 1;
            this.lblTitle.Text = "lblTitle";
            // 
            // lblCopyright
            // 
            this.lblCopyright.AutoSize = true;
            this.lblCopyright.Location = new System.Drawing.Point(153, 107);
            this.lblCopyright.Name = "lblCopyright";
            this.lblCopyright.Size = new System.Drawing.Size(61, 13);
            this.lblCopyright.TabIndex = 4;
            this.lblCopyright.Text = "lblCopyright";
            // 
            // pbBluetooth
            // 
            this.pbBluetooth.ErrorImage = null;
            this.pbBluetooth.InitialImage = null;
            this.pbBluetooth.Location = new System.Drawing.Point(12, 13);
            this.pbBluetooth.Name = "pbBluetooth";
            this.pbBluetooth.Size = new System.Drawing.Size(129, 125);
            this.pbBluetooth.TabIndex = 5;
            this.pbBluetooth.TabStop = false;
            // 
            // lblDescription
            // 
            this.lblDescription.AllowDrop = true;
            this.lblDescription.AutoSize = true;
            this.lblDescription.Location = new System.Drawing.Point(153, 28);
            this.lblDescription.Name = "lblDescription";
            this.lblDescription.Size = new System.Drawing.Size(70, 13);
            this.lblDescription.TabIndex = 2;
            this.lblDescription.Text = "lblDescription";
            // 
            // AboutForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.ClientSize = new System.Drawing.Size(296, 150);
            this.Controls.Add(this.pbBluetooth);
            this.Controls.Add(this.lblCopyright);
            this.Controls.Add(this.lblDescription);
            this.Controls.Add(this.lblTitle);
            this.Controls.Add(this.lblUrlAddress);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AboutForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "frmAbout";
            this.Load += new System.EventHandler(this.AboutForm_Load);
            ((System.ComponentModel.ISupportInitialize)(this.pbBluetooth)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.LinkLabel lblUrlAddress;
        private System.Windows.Forms.Label lblTitle;
        private System.Windows.Forms.Label lblCopyright;
        private System.Windows.Forms.PictureBox pbBluetooth;
        private System.Windows.Forms.Label lblDescription;

    }
}