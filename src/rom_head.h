/************************************************************************//**
 * \brief Megadrive ROM patch module.
 *
 * Currently the module only allows to patch the ROM header, adding the
 * wflash header information, and changing the entry point, for the
 * bootloader to be launched instead of the flashed ROM.
 *
 * \author Jesús Alonso (@doragasu)
 * \date   2019
 *
 * \defgroup RomHead rom_head
 * \{
 ****************************************************************************/

#ifndef _ROM_HEAD_H_
#define _ROM_HEAD_H_

#include <stdint.h>

/// ROM header information structure, including interrupt vectors
struct rom_head {
	// 64 interrupt/exception vectors
	uint32_t stackPtr;
	uint32_t entryPoint;
	uint32_t busErrEx;
	uint32_t addrErrEx;
	uint32_t illegalInstrEx;
	uint32_t zeroDivEx;
	uint32_t chkInstr;
	uint32_t trapvInstr;
	uint32_t privViol;
	uint32_t trace;
	uint32_t line1010Emu;
	uint32_t line1111Emu;
	uint32_t errEx[13];
	uint32_t int0;
	uint32_t extInt;
	uint32_t int1;
	uint32_t hInt;
	uint32_t int2;
	uint32_t vInt;
	uint32_t int3[33];

	// Header information
	char console[16];       ///< Console Name (16)
	char copyright[16];     ///< Copyright Information (16)
	char title_local[48];   ///< Domestic Name (48)
	char title_int[48];     ///< Overseas Name (48)
	char serial[14];        ///< Serial Number (2, 12)
	uint16_t checksum;      ///< Checksum (2)
	char IOSupport[16];     ///< I/O Support (16)
	uint32_t rom_start;     ///< ROM Start Address (4)
	uint32_t rom_end;       ///< ROM End Address (4)
	uint32_t ram_start;     ///< Start of Backup RAM (4)
	uint32_t ram_end;       ///< End of Backup RAM (4)
	char sram_sig[2];       ///< "RA" for save ram (2)
	uint16_t sram_type;     ///< 0xF820 for save ram on odd bytes (2)
	uint32_t sram_start;    ///< SRAM start address - normally 0x200001 (4)
	uint32_t sram_end;      ///< SRAM end address - start + 2*sram_size (4)
	char modem_support[12]; ///< Modem Support (24)
	char notes[40];         ///< Memo (40)
	char region[16];        ///< Country Support (16)
};

/// Lengh of the complete header (including vectors) in bytes
#define ROM_HEAD_LEN	sizeof (struct rom_head)

#define ROM_NOTES_OFF		offsetof(struct rom_head, notes)

/// Additional info to store in flashed ROM header
struct rom_head_add_info {
	uint32_t entry_point;	///< Entry point of the loader
	uint32_t title_id;	///< Title id of the flashed game
	char title[32];		///< Readable title
	uint8_t ver_major;	///< Flashed title, major version
	uint8_t ver_minor;	///< Flashed title, minor version
};

#define ROM_TITLE_OFF		(ROM_NOTES_OFF + \
		offsetof(struct rom_head_add_info, title))

#define ROM_TITLE	(const char*)ROM_TITLE_OFF

#define ROM_VER_OFF		(ROM_NOTES_OFF + \
		offsetof(struct rom_head_add_info, ver_major)

#define ROM_VER_MAJOR	*((const uint8_t*)ROM_VER_OFF)
#define ROM_VER_MINOR	*((const uint8_t*)(ROM_VER_OFF + 1))

/************************************************************************//**
 * Patches the ROM header for the wflash bootloader to be launched instead
 * of the ROM, while trying to still make the ROM "launchable"
 *
 * \param[inout] head Pointer to the ROM, including the complete header
 * \param[inout] info ROM information used to patch the header
 ****************************************************************************/
uint32_t rom_head_patch(struct rom_head *head, struct rom_head_add_info *info);

#endif /*_ROM_HEAD_H_*/

/** \} */

