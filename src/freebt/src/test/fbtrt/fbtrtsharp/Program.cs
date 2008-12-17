using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace fbtrtsharp
{
    class Program
    {
        private static HciEventListenerDelegate hciEventListener = null;

        enum W32_ERROR
        {
            ERROR_SUCCESS = 0,
            ERROR_DEVICE_IN_USE = 2404,
            ERROR_DEVICE_NOT_AVAILABLE = 4319
        }

        enum HCI_TYPE
        {
            COMMAND_PACKET = 1,
            DATA_PACKET_ACL = 2,
            DATA_PACKET_SCO = 3,
            EVENT_PACKET = 4,
            ETYPE_FINISH = 5
        };

        [DllImport("kernel32.dll")]
        public static extern int GetLastError();

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int AttachHardware();

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int DetachHardware();

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int SendHCICommand([MarshalAs(UnmanagedType.LPArray)] byte[] cmdBuf, uint cmdLen);

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int GetHCIEvent([MarshalAs(UnmanagedType.LPArray)] byte[] eventBuf, ref uint eventLen);

        public delegate int HciEventListenerDelegate(IntPtr pEventBuf, uint eventLen);

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int SubscribeHCIEvent(HciEventListenerDelegate hciEventListener);

        [DllImport("fbtrt.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        public static extern int SetLogFileName(string fileName);

        [DllImport("fbtrt.dll", SetLastError = true)]
        public static extern int SetLogLevel(uint level);

        
        static void Main( string[] args )
        {
            SetLogFileName("fbtrt.log");
            SetLogLevel(255);
            
            if ( 0 == AttachHardware() )
            {
                W32_ERROR error = (W32_ERROR)GetLastError();
                if ( W32_ERROR.ERROR_DEVICE_IN_USE == error )
                {
                    Console.WriteLine( "ERROR_DEVICE_IN_USE" );
                }
                else if ( W32_ERROR.ERROR_DEVICE_NOT_AVAILABLE == error )
                {
                    Console.WriteLine("ERROR_DEVICE_NOT_AVAILABLE" );
                }
            }
            else
            {
                hciEventListener = new HciEventListenerDelegate( OnHciEvent );
                SubscribeHCIEvent( hciEventListener );

                Console.WriteLine("Enter HCI command (empty for exit):");
                for (; ; )
                {
                    string cmd = Console.ReadLine();
                    if (cmd.Length == 0) break;
                }

                byte[] reset = new byte[] { 0x01, 0x03, 0x0c, 0x00 };
                //byte[] connect = new byte[] { 0x05, 0x04, 0x0d, 0xf7, 0x00, 0xfd, 0x76, 0x02, 0x00, 0x18, 0xcc, 0x01, 0x00, 0x00, 0x00, 0x01 };

                //byte[] acl = new byte[] { 0x01, 0x20, 0x0c, 0x00, 0x08, 0x00, 0x01, 0x00, 0x02, 0x0a, 0x04, 0x00, 0x01, 0x00, 0x43, 0x00 };

                byte[] buffer = reset;
                int ret = SendHCICommand(buffer, (uint)buffer.Length);
                string result = ( ret != 0 ) ? "OK" : "Fail";

                string packetType = "COMMAND_PACKET";
                string packetData = BytesToHex(buffer, buffer.Length);
                Console.WriteLine( string.Format( "{0}: {1} {2}", packetType, packetData, result ) );

                Console.ReadLine();

                /*
                uint sent = 0;
                buffer = acl;
                ret = SendData(buffer, (uint)buffer.Length, ref sent);
                result = (ret != 0) ? "OK" : "Fail";
                packetType = "ACL_PACKET";
                packetData = BytesToHex(buffer, buffer.Length);
                Console.WriteLine(string.Format("{0}: {1} {2}", packetType, packetData, result));

                Console.ReadLine();

                byte[] bigBuffer = new byte[1024];
                uint readed = 0;
                ret = GetData(bigBuffer, (uint)bigBuffer.Length, ref readed);
                result = (ret != 0) ? "OK" : "Fail";
                packetType = "Get Data";
                packetData = BytesToHex(bigBuffer, (int)readed);
                Console.WriteLine(string.Format("{0}: {1} {2}", packetType, packetData, result));
                 */

                
                DetachHardware();
            }
        }

        public static int OnHciEvent( IntPtr pEventBuffer, uint dwEventLength )
        {
            byte[] eventBuffer = new byte[dwEventLength];
            Marshal.Copy( pEventBuffer, eventBuffer, 0, (int)dwEventLength );

            string packetType = "EVENT_PACKET";
            string result = "OK";

            string packetData = BytesToHex( eventBuffer, (int)dwEventLength );
            Console.WriteLine( string.Format( "{0}: {1} {2}", packetType, packetData, result ) );

            byte[] buf = new byte[dwEventLength + 1];
            buf[0] = (byte)HCI_TYPE.EVENT_PACKET;
            for ( int i = 1; i < dwEventLength; ++i )
            {
                buf[i] = eventBuffer[i - 1];
            }

            return (int)W32_ERROR.ERROR_SUCCESS;
        }


        public static string BytesToHex(byte[] bytes, int length)
        {
            StringBuilder hexString = new StringBuilder(length);
            for (int i = 0; i < length; i++)
            {
                hexString.Append(bytes[i].ToString("x2"));
            }
            return hexString.ToString();
        }
    }
}
