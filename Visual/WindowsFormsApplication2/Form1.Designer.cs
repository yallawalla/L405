namespace WindowsFormsApplication2
{
    partial class Form1
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.connectToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.com = new System.IO.Ports.SerialPort(this.components);
            this.login = new System.Windows.Forms.Timer(this.components);
            this.LR = new System.Windows.Forms.Button();
            this.LD = new System.Windows.Forms.Button();
            this.BR = new System.Windows.Forms.Button();
            this.menuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip1
            // 
            this.menuStrip1.ImageScalingSize = new System.Drawing.Size(24, 24);
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.connectToolStripMenuItem,
            this.exitToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Padding = new System.Windows.Forms.Padding(9, 3, 0, 3);
            this.menuStrip1.Size = new System.Drawing.Size(933, 35);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // connectToolStripMenuItem
            // 
            this.connectToolStripMenuItem.Name = "connectToolStripMenuItem";
            this.connectToolStripMenuItem.Size = new System.Drawing.Size(89, 29);
            this.connectToolStripMenuItem.Text = "Connect";
            this.connectToolStripMenuItem.DropDownOpening += new System.EventHandler(this.connectToolStripMenuItem_DropDownOpening);
            this.connectToolStripMenuItem.DropDownItemClicked += new System.Windows.Forms.ToolStripItemClickedEventHandler(this.connectToolStripMenuItem_DropDownItemClicked);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(51, 29);
            this.exitToolStripMenuItem.Text = "Exit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // com
            // 
            this.com.BaudRate = 921600;
            this.com.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.com_DataReceived);
            // 
            // login
            // 
            this.login.Tick += new System.EventHandler(this.login_Tick);
            // 
            // LR
            // 
            this.LR.ForeColor = System.Drawing.SystemColors.ButtonFace;
            this.LR.Location = new System.Drawing.Point(478, 60);
            this.LR.Name = "LR";
            this.LR.Size = new System.Drawing.Size(73, 59);
            this.LR.TabIndex = 1;
            this.LR.Text = "LR";
            this.LR.UseVisualStyleBackColor = true;
            this.LR.Click += new System.EventHandler(this.LR_Click);
            // 
            // LD
            // 
            this.LD.ForeColor = System.Drawing.SystemColors.ButtonFace;
            this.LD.Location = new System.Drawing.Point(579, 60);
            this.LD.Name = "LD";
            this.LD.Size = new System.Drawing.Size(73, 59);
            this.LD.TabIndex = 2;
            this.LD.Text = "LD";
            this.LD.UseVisualStyleBackColor = true;
            this.LD.Click += new System.EventHandler(this.LD_Click);
            // 
            // BR
            // 
            this.BR.ForeColor = System.Drawing.SystemColors.ButtonFace;
            this.BR.Location = new System.Drawing.Point(677, 60);
            this.BR.Name = "BR";
            this.BR.Size = new System.Drawing.Size(73, 59);
            this.BR.TabIndex = 3;
            this.BR.Text = "BR";
            this.BR.UseVisualStyleBackColor = true;
            this.BR.Click += new System.EventHandler(this.BR_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(933, 402);
            this.Controls.Add(this.BR);
            this.Controls.Add(this.LD);
            this.Controls.Add(this.LR);
            this.Controls.Add(this.menuStrip1);
            this.DoubleBuffered = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.menuStrip1;
            this.Margin = new System.Windows.Forms.Padding(4, 5, 4, 5);
            this.Name = "Form1";
            this.Text = "...";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.Form1_FormClosed);
            this.Load += new System.EventHandler(this.Form1_Load);
            this.Paint += new System.Windows.Forms.PaintEventHandler(this.Form1_Paint);
            this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Form1_MouseDown);
            this.Resize += new System.EventHandler(this.Form1_Resize);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem connectToolStripMenuItem;
        private System.IO.Ports.SerialPort com;
        private System.Windows.Forms.Timer login;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.Button LR;
        private System.Windows.Forms.Button LD;
        private System.Windows.Forms.Button BR;

    }
}

