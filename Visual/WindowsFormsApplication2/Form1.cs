using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.IO.Ports;

using System.Runtime.InteropServices;     // DLL support


namespace WindowsFormsApplication2
{
    public partial class Form1 : Form
    {
        string  rxString = "";
        byte[] R = new byte[24],G = new byte[24],B = new byte[24];
 
        public Form1()
        {
            InitializeComponent();
            OpenPort(Properties.l405.Default.port);
            this.Width = Properties.l405.Default.width;
            this.Height = Properties.l405.Default.height;
            this.BackColor = LR.BackColor = LD.BackColor = BR.BackColor = Color.FromArgb(70, 70, 70);
        }

        private void connectToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            connectToolStripMenuItem.DropDownItems.Clear();
            string[] ports = SerialPort.GetPortNames();
            foreach (string port in ports)
                connectToolStripMenuItem.DropDownItems.Add(new ToolStripMenuItem(port));
        }

        private void connectToolStripMenuItem_DropDownItemClicked(object sender, ToolStripItemClickedEventArgs e)
        {
            Text = "...connecting";
            Properties.l405.Default.port = e.ClickedItem.Text;
            Properties.l405.Default.Save();
            if (com.IsOpen)
                com.Close();
             OpenPort(Properties.l405.Default.port);
        }


        private void OpenPort(string portname)
        {

            try
            {
                if (portname.Substring(0, 3) == "COM")
                {
                    if (com.IsOpen)
                        com.Close();
                    com.PortName = portname;
                    com.Open();

                }
                login.Interval = 1000;
                login.Enabled = true;
            }
            catch { }
            Properties.l405.Default.port = portname;
            Properties.l405.Default.Save();
        }

        private void com_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                rxString += com.ReadExisting();
                this.Invoke(new EventHandler(ParsingRxData));
            }
            catch { }
        }

        private void ParsingRxData(object sender, EventArgs e)
        {
            if (login.Enabled == true)
            {
                rxString = "";
                com.Write("\r+D 4\r");
                Text = com.PortName + " connected";
                login.Enabled = false;
            }
            else
            {
                string[] l = rxString.Split(new char[] {'\r', '\n'}, StringSplitOptions.RemoveEmptyEntries);
                foreach (string ll in l)
                {
                    string[] s = ll.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                    if (s.Length == 8 && s[1] == "*") {
                        int i=Int32.Parse(s[2], System.Globalization.NumberStyles.HexNumber);
                        int n = Int32.Parse(s[6], System.Globalization.NumberStyles.HexNumber);
                        int c = Int32.Parse(s[7], System.Globalization.NumberStyles.Integer);
                        Color col;
                        for (int j = 0; j < n; ++j)
                        {
                            R[i+j]=Byte.Parse(s[3], System.Globalization.NumberStyles.HexNumber);
                            G[i+j]=Byte.Parse(s[4], System.Globalization.NumberStyles.HexNumber);
                            B[i+j]=Byte.Parse(s[5], System.Globalization.NumberStyles.HexNumber);
                        }
                        if (c < 0)
                        {
                            c = -c - 1;
                            col = Color.FromArgb(70, 70, 70);
                        }
                        else
                            col = Color.FromArgb(255, 5 * R[i] / 2, 5 * G[i] / 2, 5 * B[i] / 2);

                        switch (c)
                        {
                            case 0:
                                LR.BackColor = col;
                                break;
                            case 1:
                                LD.BackColor = col;
                                break;
                            case 2:
                                BR.BackColor = col;
                                break;
                        }

                    } else
                        rxString = ll;
               }
               Refresh();             
            }



        }

        private void login_Tick(object sender, EventArgs e)
        {
            if (com.IsOpen)
                com.Write("\r");
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }


        private void Form1_Paint(object sender, PaintEventArgs e)
        {
            e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
            Graphics g = e.Graphics;
            Pen pen = new Pen(Color.LightGray, 2);
            Brush brush = Brushes.DarkGray;
            int a = 3*Math.Min(Height, Width)/4;
            int m = a / 10;
            int n = m + (int)Math.Sqrt(2) * (a / 5) / 2;
            int k = m + (int)Math.Sqrt(2) * 4 * (a / 5) / 2;

            Rectangle high = new Rectangle(m,m, a, a);
            Rectangle low = new Rectangle(n, n, 4 * a / 5, 4 * a / 5);
            Rectangle min = new Rectangle(k, k, a / 5, a / 5);

            g.FillEllipse(new SolidBrush(Color.FromArgb(0, 0, 50)), high);
            for (int i = 0; i < 4; ++i)
                g.DrawPie(new Pen(Color.LightGray,3), high, 360 * i / 4, 360 / 4);
            for (int i = 0; i < 12; ++i)
                g.DrawPie(pen, low, 360 * i / 12, 360 / 12);
            g.DrawEllipse(new Pen(Color.FromArgb(0, 0, 50), 3), low);
            g.FillEllipse(new SolidBrush(Color.FromArgb(0, 0, 50)), min);

            for (int i = 0; i < 24; ++i)
            {
                SolidBrush b = new SolidBrush(Color.FromArgb(255, 5 * R[i] / 2, 5 * G[i] / 2, 5 * B[i] / 2));
                if (R[i] != 0 || G[i] != 0 || B[i] != 0)
                   g.FillPie(b, high, 180 + 360 / 48 + 360 * i / 24, 360 / 24);
//                    g.FillPie(brush, high, 180 + 360 / 48 + 360 * i / 24, 360 / 24);
 //               else
//                g.DrawPie(pen, high, 180 + 360 / 48 + 360 * i / 24, 360 / 24);
             }
//            g.FillEllipse(brush, low);
 
        }

        private void Form1_Resize(object sender, EventArgs e)
        {
            LR.Left = Width - 200;
            LD.Left = Width - 150;
            BR.Left = Width - 100;
            Refresh();
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            Properties.l405.Default.width = this.Width;
            Properties.l405.Default.height = this.Height;
            Properties.l405.Default.Save();

        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e) 
        {
            if (System.Windows.Forms.Application.MessageLoop)
                System.Windows.Forms.Application.Exit();
            else
                System.Environment.Exit(1);
        }
    }
}
