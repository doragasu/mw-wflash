# mw-wflash
WiFi bootloader for MegaWiFi cartridges. This is a tiny bootloader that enables MegaWiFi cartridges to be able do download and Flash ROMs remotely via WiFi. It has been created to ease developing games using MegaWiFi capabilities, but it also allows uploading commercial ROMs.

## Usage

### Building the bootloader
You will need a working GNU GCC cross compiler for the m68k architecture and a MegaWiFi programmer to burn the ROM to a MegaWiFi cartridge. If you have your development environment properly installed, the makefile should do all the hard work for you. Just browse the Makefile to suit it to your dev environment and type:
```
make cart
```
The bootloader should be built and written to the cart in your programmer. Two things are written: a 512 byte header at the top of the ROM, and the bootloader itself at the bottom. The entire process is lightning fast.

### Burning ROMs
Once the bootloader is flashed to the cartridge, insert the cart in the console and turn it on. You will be greeted with a 3-options menu:
- `START`: Starts a game previosly downloaded to the cartridge.
- `DOWNLOAD MODE`: Joins a previously configured AP, and waits for a wflash client to send a ROM. IP address is displayed to ease sending the ROM from the wflash client.
- `CONFIGURATION`: Allows to configure Access Point parameters and time servers.

Usage is pretty straightforward, you can see a demo in the following video:

<div align="center">
  <a href="https://www.youtube.com/watch?v=ky1rRQWyCqo"><img src="https://img.youtube.com/vi/ky1rRQWyCqo/0.jpg" alt="wflash demo"></a>
</div>

## Limitations and future work
The bootloader takes a small amount of memory at the bottom of the ROM (currently 32 KiB). This means that currently if you want to flash a 32 megabit ROM, it will not fit unless at least 32 KiB at the end of the ROM are free. Alternatively you can disable ROM patching on the wflash client, for the ROM to be properly flashed and booted, but of course this will wipe the bootloader from the cart (you will have to burn it again to re-enable wireless flashing).

Flashing speed is currently relatively low. On my tests, burning a 2 MiB (16 megabit) ROM takes about 70 seconds (at a 5 meter distance from the WiFi router). This makes the speed similar to that of an old 256K ADSL line. The maximum theoretical achievable speed is about 1 Mbps (limited by the flash chip program time), but reaching this bitrate will require much bigger buffers than I'm currently using, and also careful parallelization of the flashing and data receiving routines. So there is much room for improvement speed wise, but it will not be easy.

## Bugs
After changing IP configuration of an Access Point, it might be necessary to reboot the console for the changes to take effect.

## Author and contributions
This program has been written by doragasu. The boot code has been grabbed from SGDK, and has been trimmed and slightly modified to meet the needs of this project. Contributions are welcome. Please don't hesitate sending a pull request.
