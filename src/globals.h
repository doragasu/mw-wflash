#ifndef _GLOBALS_H_
#define _GLOBALS_H_

/// Default channel to use for MegaWiFi communications
#define GL_CHANNEL	1

/// Address where the entry point is stored in the notes section
#define GL_NOTES_ENTRY_POINT	0x01C8

/// Entry point address is stored at the beginning of the NOTES section of
/// the cartridge header
#define GL_ENTRY_POINT_ADDR	(*((uint32_t*)GL_NOTES_ENTRY_POINT))

/// Bootloader address is currently the 68000 start entry
#define GL_BOOTLOADER_ADDR	(*((uint32_t*)0x000004))

/// Maximum program length is 4 Megabyte minus a 64 KiB sector
#define GL_PROG_LEN_MAX		(4 * 1024 - 64) * 1024

/// Major version
#define GL_VER_MAJOR	0

/// Minor version
#define GL_VER_MINOR	9

/// Configuration to signal game wants to configure WiFi
#define MAGIC_WIFI_CONFIG      0xA5A5A5A5

/// Configuration to signal user wants to configure WiFi
#define USER_WIFI_CONFIG       (GP_LEFT_MASK + GP_UP_MASK + GP_START_MASK + GP_C_MASK)

#endif /*_GLOBALS_H_*/

