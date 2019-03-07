/************************************************************************//**
 * \brief This module allows to manage (mainly read and write) from flash
 * memory chips such as S29GL032.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 * \defgroup flash flash
 * \{
 ****************************************************************************/

#ifndef _FLASH_H_
#define _FLASH_H_

#include <stdint.h>
#include "util.h"

/// Flash chip length: 4 MiB
#define FLASH_CHIP_LENGTH	(4LU*1024LU*1024LU)

/// Write data buffer length in words
#define FLASH_CHIP_WBUFLEN	16

/// Put data in section .flash.rodata.[name]
#define FS_RO(name)	SECTION(.flash.rodata.name)

/// Put function in section .flash.text.[name]
#define FS_T(name)	SECTION(.flash.text.name)

/// Data used to perform different flash commands.
typedef struct {
	uint16_t addr;	///< Flash address
	uint8_t data;	///< Flash data
} FlashCmd;

typedef void (*completion_cb)(int err, void *ctx);

/* 
 * Command definitions. NOTE: Commands use only 12-bit addresses. higher bits
 * are don't care.
 */

/// Number of write cycles to reset Flash interface
#define FLASH_RESET_CYC 1
/// Reset command data
static const FlashCmd FLASH_RESET[FLASH_RESET_CYC] FS_RO(RESET) = {
	{0xAAB, 0xF0}
};

/// Number of cycles to unlock
#define FLASH_UNLOCK_CYC 2
/// Unlock command addresses and data
static const FlashCmd FLASH_UNLOCK[FLASH_UNLOCK_CYC] FS_RO(UNLOCK) = {
	{0xAAB, 0xAA}, {0x555, 0x55},
};

/*
 * Autoselect commands. Cycles beyong the third, are read ones, and must
 * present a valid address according to part datasheet. Before the autoselect
 * command, an unlock command must be issued.
 */
/// Number of cycles of the autoselect command
#define FLASH_AUTOSEL_CYC 1
/// Autosel command addresses and data
static const FlashCmd FLASH_AUTOSEL[FLASH_AUTOSEL_CYC] FS_RO(AUTOSEL) = {
	{0xAAB, 0x90},
};

/// Number of cycles of the manufacturer ID request command.
#define FLASH_MANID_CYC 1
/// Manufacturer ID request data. Read must be prefixed by FLASH_AUTOSEL.
static const uint16_t FLASH_MANID_RD[FLASH_MANID_CYC] FS_RO(MANID_RD) = {
	0x001
};

/// Number of cycles of the device ID request command.
#define FLASH_DEVID_CYC 3
/// Device ID request data. Read must be prefixed by FLASH_AUTOSEL.
static const uint16_t FLASH_DEVID_RD[FLASH_DEVID_CYC] FS_RO(DEVID_RD) = {
	0x003, 0x01D, 0x01F
};

/// Number of cycles of the program command.
#define FLASH_PROG_CYC 1
/// Program. Must be prefixed by FLASH_UNLOCK, and followed by a write cycle.
static const FlashCmd FLASH_PROG[FLASH_PROG_CYC] FS_RO(PROG) = {
	{0xAAB, 0xA0}
};

/// Number of cycles of the write to buffer command.
#define FLASH_WR_BUF_CYC 1
/// Write to buffer. Must be prefixed with FLASH_UNLOCK, and followed with
/// buffer write sequence. On this cycle, address must be SA (see datasheet).
static const uint8_t FLASH_WR_BUF[FLASH_WR_BUF_CYC] FS_RO(WR_BUF) = {0x25};

/// Number of cycles of the program buffer to flash command.
#define FLASH_PRG_BUF_CYC	1
/// Program buffer to flash data.
/// \note Address must be SA (see datasheet), but data is fixed.
static const uint8_t FLASH_PRG_BUF[FLASH_PRG_BUF_CYC] FS_RO(BUF_CYC) = {0x29};

/// Number of cycles of the unlock bypass command.
#define FLASH_UL_BYP_CYC 1
/// Unlock bypass command data. Must be prefixed with FLASH_UNLOCK.
static const FlashCmd FLASH_UL_BYP[FLASH_UL_BYP_CYC] FS_RO(UL_BYP) = {
	{0xAAB, 0x20}
};

/// Number of cycles of the unlock bypass program command.
#define FLASH_UL_BYP_PROG_CYC 1
/// Unlock bypass program data. \note address is don't care. Must be followed
/// by a write cycle.
static const uint8_t FLASH_UL_BYP_PROG[FLASH_UL_BYP_PROG_CYC]
	FS_RO(UL_BYP_PROG) = {0xA0};

/// Number of cycles of the unlock bypass reset command.
#define FLASH_UL_BYP_RST_CYC 2
/// Unlock bypass reset data. \note addresses are don't care
static const uint8_t FLASH_UL_BYP_RST[FLASH_UL_BYP_RST_CYC]
	FS_RO(UL_BYP_RST) = {0x90, 0x00};

/// Number of cycles of the chip erase command.
#define FLASH_CHIP_ERASE_CYC 4
/// Chip erase data. Must be prefixed with FLASH_UNLOCK.
static const FlashCmd FLASH_CHIP_ERASE[FLASH_CHIP_ERASE_CYC]
	FS_RO(CHIP_ERASE) = {
	{0xAAB, 0x80}, {0xAAB, 0xAA}, {0x555, 0x55}, {0xAAB, 0x10}
};

/// Number of cycles of the sector erase command.
#define FLASH_SEC_ERASE_CYC 3
/// Sector erase. Must be prefixed with FLASH_UNLOCK. Address on last cycle
/// must be SA (see datasheet).
static const FlashCmd FLASH_SEC_ERASE[FLASH_SEC_ERASE_CYC]
	FS_RO(SEC_ERASE) = {
	{0xAAB, 0x80}, {0xAAB, 0xAA}, {0x555, 0x55}
};

/// Data to be written along with sector address after FLASH_SEC_ERASE
static const uint8_t FLASH_SEC_ERASE_WR[1] FS_RO(SEC_ERASE) = {0x30};

/*
 * Public functions
 */

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************//**
 * \brief Module initialization. Configures the 68k bus.
 ****************************************************************************/
void FlashInit(void);

/************************************************************************//**
 * \brief Writes a word to specified address.
 *
 * \param[in] addr Address to which data will be written.
 * \param[in] data Data to write to addr address.
 *
 * \note Do not mistake this function with the program ones.
 ****************************************************************************/
FS_T(Write)
static inline void FlashWrite(uint32_t addr, uint8_t data) {
	*((volatile uint8_t*)addr) = data;
}

/************************************************************************//**
 * \brief Writes a byte to specified address.
 *
 * \param[in] addr Address to which data will be written.
 * \param[in] data Data to write to addr address.
 *
 * \note Do not mistake this function with the program ones.
 ****************************************************************************/
FS_T(WriteW)
static inline void FlashWriteW(uint32_t addr, uint16_t data) {
	*((volatile uint16_t*)addr) = data;
}

/************************************************************************//**
 * \brief Reads a byte from the specified address.
 *
 * \param[in] addr Address that will be read.
 *
 * \return Readed word.
 ****************************************************************************/
FS_T(Read)
static inline uint8_t FlashRead(uint32_t addr) {
	return *((volatile uint8_t*)addr);
}

/************************************************************************//**
 * \brief Reads a word from the specified address.
 *
 * \param[in] addr Address that will be read.
 *
 * \return Readed word.
 ****************************************************************************/
FS_T(ReadW)
static inline uint16_t FlashReadW(uint32_t addr) {
	return *((volatile uint16_t*)addr);
}

/************************************************************************//**
 * \brief Writes a command to the flash chip.
 * 
 * \param[in] The command data structure to write to the flash chip.
 ****************************************************************************/
FS_T(WriteCmd)
static inline void FlashWriteCmd(FlashCmd cmd) {
	FlashWrite(cmd.addr, cmd.data);
}

/// Helper macro for writing commands. Takes the command and an auxiliar
/// variable used as an iterator.
#define FLASH_WRITE_CMD(cmd, iterator)	do {					\
	for (iterator = 0; iterator < cmd ## _CYC; iterator++) {	\
		FlashWrite(cmd[iterator].addr, cmd[iterator].data);		\
	}															\
} while (0);

/************************************************************************//**
 * \brief Writes the flash unlock command to the flash chip. This command
 * must be used as part of other larger commands.
 ****************************************************************************/
FS_T(Unlock)
static inline void FlashUnlock(void) {
	uint8_t i;
	
	FLASH_WRITE_CMD(FLASH_UNLOCK, i);
}

/************************************************************************//**
 * \brief Writes the flash autoselect command to the flash chip. This command
 * must be used as part of other larger commands.
 ****************************************************************************/
FS_T(Autoselect)
static inline void FlashAutoselect(void) {
	uint8_t i;

	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_AUTOSEL, i);
}

/************************************************************************//**
 * \brief Sends the reset command, to return to array read mode.
 ****************************************************************************/
FS_T(Reset)
static inline void FlashReset(void) {
	uint8_t i;
	FLASH_WRITE_CMD(FLASH_RESET, i);
}

/************************************************************************//**
 * \brief Writes the manufacturer ID query command to the flash chip.
 *
 * \return The manufacturer ID code.
 ****************************************************************************/
FS_T(GetMainId)
uint8_t FlashGetManId(void);

/************************************************************************//**
 * \brief Writes the device ID query command to the flash chip.
 *
 * \param[out] devId The device ID code, consisting of 3 words.
 ****************************************************************************/
FS_T(GetDevId)
void FlashGetDevId(uint8_t devId[3]);

/************************************************************************//**
 * \brief Programs a word to the specified address.
 *
 * \param[in] addr The address to which data will be programmed.
 * \param[in] data Data to program to the specified address.
 *
 * \warning Doesn't poll until programming is complete
 ****************************************************************************/
FS_T(Prog)
void FlashProg(uint32_t addr, uint16_t data);

/************************************************************************//**
 * \brief Programs a buffer to the specified address range.
 *
 * \param[in] addr The address of the first word to be written
 * \param[in] data The data array to program to the specified address range.
 * \param[in] wLen The number of words to program, contained on data.
 * \return The number of words successfully programed.
 *
 * \note wLen must be less or equal than 16.
 * \note If addr-wLen defined range crosses a write-buffer boundary, all the
 *       requested words will not be written. To avoid this situation, it
 *       is advisable to write to addresses having the lower 4 bits (A1~A5)
 *       equal to 0.
 ****************************************************************************/
FS_T(WriteBuf)
uint8_t FlashWriteBuf(uint32_t addr, uint16_t data[], uint8_t wLen);

/************************************************************************//**
 * Enables the "Unlock Bypass" status, allowing to issue several commands
 * (like the Unlock Bypass Programm) using less write cycles.
 ****************************************************************************/
FS_T(UnlockBypass)
static inline void FlashUnlockBypass(void) {
	uint8_t i;

	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_UL_BYP, i);
}

// Warning: must be issued after a FlashUnlockBypass command
// Warning: doesn't poll until programming is complete
FS_T(UnlockProgram)
static inline void FlashUnlockProgram(uint32_t addr, uint8_t data) {
	// Write unlock bypass program command
	FlashWrite(addr, FLASH_UL_BYP_PROG[0]);
	// Write data
	FlashWrite(addr, data);
}


/************************************************************************//**
 * Ends the "Unlock Bypass" state, returning to default read mode.
 ****************************************************************************/
FS_T(UnlockBypassReset)
static inline void FlashUnlockBypassReset(void) {
	// Write reset command. Addresses are don't care
	FlashWrite(0, FLASH_UL_BYP_RST[0]);
	FlashWrite(0, FLASH_UL_BYP_RST[1]);
}

/************************************************************************//**
 * Erases the complete flash chip.
 *
 * \return '0' the if erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
FS_T(ChipErase)
uint8_t FlashChipErase(void);

/************************************************************************//**
 * Erases a complete flash sector, specified by addr parameter.
 *
 * \param[in] addr Address contained in the sector that will be erased.
 * \return '0' if the erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
FS_T(SectErase)
uint8_t FlashSectErase(uint32_t addr);

/************************************************************************//**
 * Erases a flash memory range.
 *
 * \param[in] addr Address base for the range to erase.
 * \param[in] len  Length of the range to erase
 * \return '0' if the erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
uint8_t FlashRangeErase(uint32_t addr, uint32_t len);

/************************************************************************//**
 * \brief Polls flash chip after a program operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address to which data has been written.
 * \param[in] data Data written to addr address.
 * \return 0 if OK, 1 if error during program operation.
 ****************************************************************************/
FS_T(DataPoll)
uint8_t FlashDataPoll(uint32_t addr, uint8_t data);

/************************************************************************//**
 * \brief Polls flash chip after an erase operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address contained in the erased zone.
 * \return 0 if OK, 1 if error during program operation.
 *
 * \warning Function erases the minimum memory range CONTAINING the
 * specified range. Due to the granularity of the flash sectors, it can (and
 * most likely will) erase more memory than requested. This is expected
 * behaviour, and programmer must be aware of this.
 * \warning Currently the function does not check if the input range covers
 * the sector/s containing the bootloader.
 ****************************************************************************/
FS_T(ErasePoll)
uint8_t FlashErasePoll(uint32_t addr);

FS_T(chip_erase)
void flash_chip_erase(void *ctx);

FS_T(sector_erase)
void flash_sector_erase(uint32_t addr, void *ctx);

FS_T(range_erase)
int flash_range_erase(uint32_t addr, uint32_t len);

FS_T(flash_write_buf)
int flash_write_buf(uint32_t addr, uint16_t *data, uint16_t wlen, void *ctx);

FS_T(poll_proc)
void flash_poll_proc(void);

FS_T(write_long)
int flash_write_long(uint32_t addr, uint16_t *data, uint16_t wlen);

void flash_completion_cb_set(completion_cb cb);

#ifdef __cplusplus
}
#endif

/** \} */

#endif /*_FLASH_H_*/

