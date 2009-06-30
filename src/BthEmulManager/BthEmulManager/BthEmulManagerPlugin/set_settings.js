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

var xmlDoc = new ActiveXObject("MSXML2.DOMDocument.6.0");
xmlDoc.async = false;
xmlDoc.validateOnParse = false;
xmlDoc.resolveExternals = false;
xmlDoc.setProperty("SelectionLanguage", "XPath");

var version = "";
var url = "";

xmlDoc.load(getLocalPath() + "settings.xml");
var node = xmlDoc.selectSingleNode("/root/version");
if (node != null) {
	version = node.text;
}

node = xmlDoc.selectSingleNode("/root/url");
if (node != null) {
	url = node.text;
}

xmlDoc.setProperty("SelectionNamespaces", "xmlns:rtfx='http://schemas.microsoft.com/windowsce/rtfx/'");
xmlDoc.load(getLocalPath() + "BthEmulManager.cebundleinfo");
var node = xmlDoc.selectSingleNode("/rtfx:RTFxPlugin/rtfx:Version");
if (node != null) {
	node.text = version;
}
var node = xmlDoc.selectSingleNode("/rtfx:RTFxPlugin/rtfx:URL");
if (node != null) {
	node.text = url;
}
xmlDoc.save(getLocalPath() + "BthEmulManager.cebundleinfo");

xmlDoc.load(getLocalPath() + "Resources.resx");
var node = xmlDoc.selectSingleNode("/root/data[@name='VersionNumber']/value");
if (node != null) {
	node.text = version;
}
var node = xmlDoc.selectSingleNode("/root/data[@name='UrlAddress']/value");
if (node != null) {
	node.text = url;
}
xmlDoc.save(getLocalPath() + "Resources.resx");

WScript.Echo("Settings were applied successfully");

function getLocalPath() {
	// determine the script's local path
	return WScript.ScriptFullName.replace(WScript.ScriptName, "");
}

