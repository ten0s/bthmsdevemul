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

using System;
using System.IO;
using System.Xml.Serialization;

namespace BthEmul
{
    [XmlRoot("Settings")]
    public class Settings
    {
        private bool deviceLogging;
        private bool desktopLogging;
        private bool commLogging;
        private static string FILE_NAME = "Settings.xml";

        public Settings()
        {
            this.DeviceLogging = false;
            this.DesktopLogging = false;
            this.CommLogging = false;
        }

        public void Serialize(string settingsPath)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(Settings));
            TextWriter writer = null;
            try
            {
                writer = new StreamWriter(settingsPath + "\\" + FILE_NAME);
                serializer.Serialize(writer, this);                
            }
            catch {;}
            finally
            {
                if (writer != null)
                {
                    writer.Close();
                }
            }    
        }

        public void Deserialize(string settingsPath)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(Settings));
            TextReader reader = null;
            Settings settings = null;
            try
            {
                reader = new StreamReader(settingsPath + "\\" + FILE_NAME);
                settings = (Settings)serializer.Deserialize(reader);
            }
            catch {;}
            finally
            {
                if (reader != null)
                {
                    reader.Close();
                }                
            }            

            if (settings != null)
            {
                this.DeviceLogging = settings.DeviceLogging;
                this.DesktopLogging = settings.DesktopLogging;
                this.CommLogging = settings.CommLogging;
            }            
        }

        [XmlAttribute("DeviceLogging")]
        public bool DeviceLogging
        {
            get { return this.deviceLogging; }
            set { this.deviceLogging = value; }
        }

        [XmlAttribute("DesktopLogging")]
        public bool DesktopLogging
        {
            get { return this.desktopLogging; }
            set { this.desktopLogging = value; }
        }

        [XmlAttribute("CommLogging")]
        public bool CommLogging
        {
            get { return this.commLogging; }
            set { this.commLogging = value; }
        }
    }
}
