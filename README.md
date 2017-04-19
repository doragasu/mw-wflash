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
Once the bootloader is flashed to the cartridge, insert the cart in the console and turn it on. The bootloader will automatically:
 - Detect the WiFi hardware and firmware version.
 - Join a previously configured Access Point (e.g. a WiFi router).
 - Obtain the configured IP address and print it on the screen.
 - Open a TCP server socket on the displayed IP and configured port (1989 by default).
 - Print the "READY!" prompt.

Once you see the ready prompt, use a wflash client (such as mw-wf-cli) to remotely burn ROMs. To do this you will need the IP displayed on the screen. The default port number is `1989`.

The wflash client can also be used to boot the flashed ROM, or alternatively you can boot it by pressing START button on the first controller, when the bootloader is idle.

## Limitations and future work
The bootloader takes a small amount of memory at the bottom of the ROM (currently just 8 KiB, but this should grow when more features are added). This means that currently if you want to flash a 32 megabit ROM, it will not fit unless at least 8 KiB at the end of the ROM are free. Alternatively you can disable ROM patching on the wflash client, for the ROM to be properly flashed and booted, but of course this will wipe the bootloader from the cart (you will have to burn it again to re-enable wireless flashing).

Currently the bootloader does not allow configuring the APN and IP parameters. This configuration must be done using other program, and set to the slot 1. Adding this feature to the bootloader is a priority, but requires a lot of work (adding a proper menu system with virtual keyboard, scanning function, etc) while keeping bootloader size as low as possible.

Flashing speed is currently relatively low. On my tests, burning a 2 MiB (16 megabit) ROM takes about 70 seconds (at a 5 meter distance from the WiFi router). This makes the speed similar to that of an old 256K ADSL line. The maximum theoretical achievable speed is about 1 Mbps (limited by the flash chip program time), but reaching this bitrate will require much bigger buffers than I'm currently using, and also careful parallelization of the flashing and data receiving routines. So there is much room for improvement speed wise, but it will not be easy.

## Author and contributions
This program has been written by doragasu. The boot code has been grabbed from SGDK, and has been trimmed and slightly modified to meet the needs of this project. Contributions are welcome. Please don't hesitate sending a pull request.
