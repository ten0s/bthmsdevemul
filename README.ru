=======================================
Bluetooth для Microsoft Device Emulator
version 0.9.4
April 2nd, 2009
=======================================


ВСТУПЛЕНИЕ
==========

Bluetooth для Microsoft Device Emulator добавляет поддержку Bluetooth в эмулятор.

Bluetooth был проверен со следующими образами операционных систем:

  * Windows Mobile 5.0 Pocket PC SDK
  * Windows Mobile 5.0 Smartphone SDK
  * Windows Mobile 6 Professional SDK
  * Windows Mobile 6 Standard SDK 


ТРЕБОВАНИЯ
==========

Пожалуйста обратите внимание, что ВСЕ ниже перечисленное ДОЛЖНО БЫТЬ установлено:

  * Windows XP or Windows Vista 32-bit with the lastest service packs.
  * Microsoft Visual Studio 2005 or 2008 with the lastest service packs. You Visual Studio installation must include the Smart Device Programmability feature.
  * Windows Mobile 5.0 Pocket PC SDK and/or Windows Mobile 5.0 Smartphone SDK and/or Windows Mobile 6 Professional SDK and/or Windows Mobile 6 Standard SDK emulator images
  * Microsoft Device Emulator (Recommended: Version 3.0).
  * Microsoft Remote Tools Framework 1.00. 

Also you should have an available Bluetooth USB dongle.


ВНИМАНИЕ
========

This version of Bluetooth for Microsoft Device Emulator uses FreeBT project <http://freebt.net/> to access the Bluetooth USB dongle.


ОБЗОР АРХИТЕКТУРЫ
=================

The project consists from the followings components: 
  1. Device side 
    * Bluetooth HCI Transport Driver <http://msdn.microsoft.com/en-us/library/ms890918.aspx>.
    * Serial Port Driver <http://msdn.microsoft.com/en-us/library/ms923741.aspx> to abstract the transport driver from  knowing means of communication.
    * Microsoft Remote Tools Framework remote agent.
  2. Desktop side
    * Microsoft Remote Tools Framework desktop plugin.
    * FreeBT USB Driver runtime.
    * FreeBT USB Driver.
    * Connected Bluetooth USB device.


АППАРАТНОЕ ОБЕСПЕЧЕНИЕ
======================

The FreeBT USB Driver is written in accordance with the Bluetooth USB HCI spec. Any device that follows the recommendations set down in that spec should be compatible with the FreeBT driver.

The fbtusb.inf will, by default, install any USB dongle that identifies itself with Class 0xE0 (Wireless Controller), SubClass 0x01 (RF Controller) and Protocol 0x01 (Bluetooth programming), in accordance with the USB HCI spec.

Specifically, FreeBT USB has been tested with the following devices:

  * Acer BT-700 (Class 1 device)
  * D-Link DBT-120 (Class 2 device)
  * Any CSR or Silicon Wave-based USB devices 

Actually, both of these devices incorporate a CSR BlueCore01 chip, (as do most of the commercially available Bluetooth dongles at present), which (apart from their amplifiers) makes them identical.


УСТАНОВКА BLUETOOTH USB ДРАЙВЕРА
================================

  1. Open the Windows Device Manager.
  2. In the Device Manager, locate the Bluetooth device to be used as the FreeBT USB device.
  3. Click the right mouse button and select "Update Driver..." in the popup-menu.
  4. Select "No, not this time" and click "Next >".
  5. Select "Install from a list or specific location" and click "Next >".
  6. Select "Don’t search. I will choose the driver to install" and click "Next >".
  7. Select the device driver and click the "Have Disk...".
  8. In the Locate File dialog, browse to fbtusb.inf file and click "Next >".
  9. When a "Hardware Installation warning" appears, click "Continue Anyway".
  10. Once the installation has been completed, click "Finish".
  11. The Device Manager should now display "FreeBT USB Driver" in the list of USB controllers. 

Please refer to FreeBT documentation for more details.


УСТАНОВКА
=========

  1. Install the Visual Studio 2005 or 2008 with the latest service packs. Your Visual Studio installation should include the Smart Device    Programmability.
  2. Install the Microsoft Device Emulator. You should have already installed one along with the VS installation. Update it to the Microsoft Device Emulator 3.0 -- Standalone Release <http://www.microsoft.com/downloads/details.aspx?familyid=A6F6ADAF-12E3-4B2F-A394-356E2C2FB114&displaylang=en>.
  3. Install emulator images you need.
  4. You must have an USB Bluetooth dongle available. Plugin in it into an available USB port.
  5. Install the FreeBT driver as described in the BLUETOOTH USB DRIVER INSTALLATION section.
  6. Install the Microsoft Remote Tools Framework 1.00 <http://www.microsoft.com/downloads/details.aspx?FamilyID=35e9ef0f-833f-4987-9d1f-157a0a6a76e4&DisplayLang=en>.


НАЧАЛО РАБОТЫ
=============

  1. Install the FreeBT USB Driver as described in the BLUETOOTH USB DRIVER INSTALLATION section.
  2. Install all REQUIREMENTS section as described in the INSTALLATION section.
  3. Launch the Remote Tools Framework plugin BthEmulManager.cetool from "RTFx Plugin" folder.
  4. Select an emulator image from the list.
  5. Wait for the emulator to be connected.
  6. Select "Bluetooth for Microsoft Device Emulator" node.
  7. If you have a Bluetooth device attached succesfully then the Bluetooth device info (Address, Manufacturer,   HCI Version, LMP Version) will be displayed. Bluetooth should now be operational in your emulator. An error code with the description will be displayed otherwise.
  8. To clear the communication log click the right mouse button and select "Clear All".
  9. To copy current device information to the clipboard select "Bluetooth for Microsoft Device Emulator" node, click the right mouse button and select "Copy to Clipboard". 
  10. To switch off Bluetooth on the emulator click "Connection" menu item and select "Disconnect from ... emulator".
  11. Microsoft Remote Tools Framework allows you to start two device emulators at the same time. If you have installed two or more FreeBT USB devices click "Windows" menu item and select "Split Windows View". Repeat steps 3-6. You will get two Bluetooth powered emulators are running simultaneously.
  12. It is possible to enable/disable device side logging. If you enable "Device Logging" checkbox then remote logging will be enabled on the emulator. Have a look at \\Temp directory on the emulator. There should be created btd_bthemul_0.txt, btd_BthEmulAgent_0.txt, btd_bthemulcom_0.txt files. 
  13. It is possible to enable/disable desktop side logging. If you enable "Desktop Logging" checkbox then local logging will be enabled. Have a look at your installation directory. There should be created BthEmulManager.txt file. 
  14. It is possible to enable/disable communication logging. Communication logging allows to see communication activities between the emulator and the Bluetooth device.


РЕШЕНИЕ ПРОБЛЕМ
===============

  Q. I've installed the Free BT Driver, but after deploying the emulator, it is still not working.
  A. Install the Microsoft Remote Tools Framework 1.00 if you haven't done it yet. Launch the Remote Tools Framework plugin BthEmulManager.cetool from "RTFx Plugin" folder.
  
  Q. I'm trying to connecter Pocket PC 2003 SE Emulator but I receive an error all the time: "The device Pocket PC 2003 SE VGA Emulator has a CPU type of ARMV4 and an operating system version of 4.21."
  A. Pocket PC 2003 and Smartphone 2003 aren't supported yet.

  Q. Which Windows Device Manager the README refers to ?
  A. Win2K&XP: Control Panel -> System -> Hardware tab -> Device Manager
     Vista: Control Panel -> Hardware and Sound -> Device Manager

  Q. I have started the BthEmulManager.cetool plugin for the first time but nothinng happens. There isn't Bluetooth in the emulator.
  A. Try to restart the plugin for the previously choosen emulator. Also go to Settings -> Connections -> Bluetooth -> Mode tab and turn on Bluetooth.

You may also have problems with the FreeBT USB Driver itself. The driver is still (and will probably remain at this stage) in alpha version. There are a number of reports saying it has problems. The problems with this driver are the following:

  1. It stops working for no reason.
  2. It may miss packets in write/read operations. The problems start when you send large packets very fast. The most probably the problem in an incorrect using HCI buffers (Read Buffer Size Command, Number Of Completed Packets Event).
  3. It isn't stable with some chipset like CSR.

But the driver works quite stable for me at least. I encourage you if you have such problems and have experience in the Windows driver development try to improve the driver and share your results.


БЛАГОДАРНОСТИ
=============

Спасибо Antony C. Roberts за FreeBT <http://freebt.net/> проект. Без него этот проект мог бы быть невозможным.


ПРАВОВАЯ ИНФОРМАЦИЯ
===================

Bluetooth для Microsoft Device Emulator
Copyright (C) 2008-2009 Dmitry Klionsky <dm.klionsky@gmail.com>

FreeBT
Copyright (C) 2004 Antony C. Roberts <http://www.freebt.net/>

Bluetooth(TM)
Copyright (C) Bluetooth SIG, Inc. All rights reserved. 

Windows(TM), Windows XP(TM), Windows Mobile(TM),
Microsoft Visual Studio, Microsoft Device Emulator, 
Microsoft Remote Tools Framework
Copyright (C) Microsoft Corporation. All rights reserved.