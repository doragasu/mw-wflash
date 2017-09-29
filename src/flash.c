/************************************************************************//**
 * \brief This module allows to manage (mainly read and write) from flash
 * memory chips such as S29GL032.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2015
 ****************************************************************************/

#include "flash.h"
#include "util.h"

#include "vdp.h"

/// Obtains a sector address from an address. Current chip does not actually
//require any calculations to get SA.
/// \todo Support several flash chips
#define FLASH_SA_GET(addr)		(addr)

/// Number of shifts for addresses of saddr array
#define FLASH_SADDR_SHIFT	8

/// Top address of the Flash chip, plus 1, shifted FLASH_SADDR_SHIFT times.
#define FLASH_SADDR_MAX		(FLASH_CHIP_LENGTH>>FLASH_SADDR_SHIFT)

/// Sector addresses, shifted FLASH_SADDR_SHIFTS times to the right
/// Note not all the sectors are the same length (depending on top boot
/// or bottom boot flash configuration).
const static uint16_t saddr[] = {
	0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700,
	0x0800, 0x0900, 0x0A00, 0x0B00, 0x0C00, 0x0D00, 0x0E00, 0x0F00,
	0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x1600, 0x1700,
	0x1800, 0x1900, 0x1A00, 0x1B00, 0x1C00, 0x1D00, 0x1E00, 0x1F00,
	0x2000, 0x2100, 0x2200, 0x2300, 0x2400, 0x2500, 0x2600, 0x2700,
	0x2800, 0x2900, 0x2A00, 0x2B00, 0x2C00, 0x2D00, 0x2E00, 0x2F00,
	0x3000, 0x3100, 0x3200, 0x3300, 0x3400, 0x3500, 0x3600, 0x3700,
	0x3800, 0x3900, 0x3A00, 0x3B00, 0x3C00, 0x3D00, 0x3E00, 0x3F00,
	0x3F20, 0x3F40, 0x3F60, 0x3F80, 0x3FA0, 0x3FC0, 0x3FE0
};

/// Returns the sector number corresponding to address input
#define FLASH_NSECT		(sizeof(saddr) / sizeof(uint16_t))

/************************************************************************//**
 * \brief Polls flash chip after a program operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address to which data has been written.
 * \param[in] data Data written to addr address.
 * \return 0 if OK, 1 if error during program operation.
 ****************************************************************************/
uint8_t FlashDataPoll(uint32_t addr, uint8_t data) {
	uint8_t read;

	// Poll while DQ7 != data(7) and DQ5 == 0 and DQ1 == 0
	do {
		read = FlashRead(addr);
	} while (((data ^ read) & 0x80) && ((read & 0x22) == 0));

	// DQ7 must be tested after another read, according to datasheet
//	if (((data ^ read) & 0x80) == 0) return 0;
	read = FlashRead(addr);
	if (((data ^ read) & 0x80) == 0) return 0;
	// Data has not been programmed, return with error. If DQ5 is set, also
	// issue a reset command to return to array read mode
	if (read & 0x20) {
		FlashReset();
	}
	// If DQ1 is set, issue the write-to-buffer-abort-reset command.
	if (read & 0x02) {
		FlashUnlock();
		FlashReset();
	}
	return 1;
}

/************************************************************************//**
 * \brief Polls flash chip after an erase operation, and returns when the
 * program operation ends, or when there is an error.
 *
 * \param[in] addr Address contained in the erased zone.
 * \return 1 if OK, 0 if error during program operation.
 ****************************************************************************/
uint8_t FlashErasePoll(uint32_t addr) {
	uint16_t read;

	// Wait until DQ7 or DQ5 are set
	do {
		read = FlashRead(addr);
	} while (!(read & 0xA0));

	// If DQ5 is set, an error has occurred. Also a reset command needs to
	// be sent to return to array read mode.
	if (!(read & 0x80)) return 0;

	FlashReset();
	return 1;
	//return (read & 0x80) != 0;
}

/**
 * Public functions
 */

/************************************************************************//**
 * \brief Module initialization. Configures the 68k bus.
 ****************************************************************************/
void FlashInit(void){
}

/************************************************************************//**
 * \brief Writes the manufacturer ID query command to the flash chip.
 *
 * \return The manufacturer ID code.
 ****************************************************************************/
uint8_t FlashGetManId(void) {
	uint8_t retVal;

	// Obtain manufacturer ID and reset interface to return to array read.
	FlashAutoselect();
	retVal = FlashRead(FLASH_MANID_RD[0]);
	FlashReset();

	return retVal;
}

/************************************************************************//**
 * \brief Writes the device ID query command to the flash chip.
 *
 * \param[out] devId The device ID code, consisting of 3 words.
 ****************************************************************************/
void FlashGetDevId(uint8_t devId[3]) {
	// Obtain device ID and reset interface to return to array read.
	FlashAutoselect();
	devId[0] = FlashRead(FLASH_DEVID_RD[0]);
	devId[1] = FlashRead(FLASH_DEVID_RD[1]);
	devId[2] = FlashRead(FLASH_DEVID_RD[2]);
	FlashReset();
}

/************************************************************************//**
 * \brief Programs a word to the specified address.
 *
 * \param[in] addr The address to which data will be programmed.
 * \param[in] data Data to program to the specified address.
 *
 * \warning Doesn't poll until programming is complete
 ****************************************************************************/
void FlashProg(uint32_t addr, uint16_t data) {
	uint8_t i;

	// Unlock and write program command
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_PROG, i);
	// Write data
	FlashWrite(addr, data);
}

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
uint8_t FlashWriteBuf(uint32_t addr, uint16_t data[], uint8_t wLen) {
	// Sector address
	uint32_t sa;
	// Number of words to write
	uint8_t wc;
	// Index
	uint8_t i;

	// Check maximum write length
	if (wLen >  FLASH_CHIP_WBUFLEN) {
		return 0;
	}

	// Obtain the sector address
	sa = FLASH_SA_GET(addr);
	// Compute the number of words to write minus 1. Maximum number is 15,
	// but without crossing a write-buffer page
	wc = MIN(wLen, FLASH_CHIP_WBUFLEN - ((addr>>1) &
				(FLASH_CHIP_WBUFLEN - 1))) - 1;
	// Unlock and send Write to Buffer command
	FlashUnlock();
	FlashWriteW(sa, FLASH_WR_BUF[0]);
	// Write word count - 1
	FlashWriteW(sa, wc);

	// Write data to bufffer
	for (i = 0; i <= wc; i++, addr+=2) FlashWriteW(addr, data[i]);
	// Write program buffer command
	FlashWriteW(sa, FLASH_PRG_BUF[0]);
	// Poll until programming is complete
	if (FlashDataPoll(addr - 1, data[i - 1] & 0xFF)) {
		return 0;
	}

	// Return number of elements (words) written
	return i;
}

/************************************************************************//**
 * Enables the "Unlock Bypass" status, allowing to issue several commands
 * (like the Unlock Bypass Programm) using less write cycles.
 ****************************************************************************/
void FlashUnlockBypass(void) {
	uint8_t i;

	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_UL_BYP, i);
}

/************************************************************************//**
 * Ends the "Unlock Bypass" state, returning to default read mode.
 ****************************************************************************/
void FlashUnlockBypassReset(void) {
	// Write reset command. Addresses are don't care
	FlashWrite(0, FLASH_UL_BYP_RST[0]);
	FlashWrite(0, FLASH_UL_BYP_RST[1]);
}

/************************************************************************//**
 * Erases the complete flash chip.
 *
 * \return '0' the if erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
uint8_t FlashChipErase(void) {
	uint8_t i;

	// Unlock and write chip erase sequence
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_CHIP_ERASE, i);
	// Poll until erase complete
	return FlashErasePoll(1);
}

/************************************************************************//**
 * Erases a complete flash sector, specified by addr parameter.
 *
 * \param[in] addr Address contained in the sector that will be erased.
 * \return '0' if the erase operation completed successfully, '1' otherwise.
 ****************************************************************************/
uint8_t FlashSectErase(uint32_t addr) {
	// Sector address
	uint32_t sa;
	// Index
	uint8_t i;

	addr++;
	// Obtain the sector address
	sa = FLASH_SA_GET(addr);
	// Unlock and write sector address erase sequence
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_SEC_ERASE, i);
	// Write sector address 
	FlashWrite(sa, FLASH_SEC_ERASE_WR[0]);
	// Wait until erase starts (polling DQ3)
	while (!(FlashRead(sa) & 0x08));
	// Poll until erase complete
	return FlashErasePoll(addr);
}

/************************************************************************//**
 * Erases a flash memory range.
 *
 * \param[in] addr Address base for the range to erase.
 * \param[in] len  Length of the range to erase
 * \return '0' if the erase operation completed successfully, '1' otherwise.
 *
 * \warning Function erases the minimum memory range CONTAINING the
 * specified range. Due to the granularity of the flash sectors, it can (and
 * most likely will) erase more memory than requested. This is expected
 * behaviour, and programmer must be aware of this.
 * \warning Currently the function does not check if the input range covers
 * the sector/s containing the bootloader.
 ****************************************************************************/
uint8_t FlashRangeErase(uint32_t addr, uint32_t len) {
	// Index
	uint8_t i, j;
	// Shifted address to compare with 
	uint16_t caddr = addr>>FLASH_SADDR_SHIFT;
	uint16_t clen = (len - 1)>>FLASH_SADDR_SHIFT;

	if (!len) return 0;
	if ((addr + len) >= (FLASH_SADDR_MAX<<FLASH_SADDR_SHIFT)) return 1;

	// Find sector containing the initial address
	for (i = FLASH_NSECT - 1; caddr < saddr[i]; i--);
	// Find sector containing the end address
	for (j = FLASH_NSECT - 1; (caddr + clen) < saddr[j]; j--);

	for (; i <= j; i++) {
		if (!FlashSectErase((saddr[i])<<FLASH_SADDR_SHIFT)) return 2;
	}

	return 0;
}

