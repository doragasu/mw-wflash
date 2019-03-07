#include <string.h>
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
static const uint16_t saddr[] = {
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

enum PACKED poll_type {
	FLASH_POLL_NONE = 0,
	FLASH_POLL_DATA,
	FLASH_POLL_SECT,
	FLASH_POLL_RANGE,
	FLASH_POLL_CHIP,
	FLASH_POLL_MAX
};

struct write_long_data {
	uint16_t *buf;
	uint32_t addr;
	uint16_t buf_wlength;
	uint16_t wpos;
};

struct poll_data {
	completion_cb cb;
	void *ctx;
	union {
		uint32_t addr;
		struct {
			// For sector range erase
			uint16_t cur_sect;
			uint16_t fin_sect;
		};
	};
	struct write_long_data write;
	uint8_t data;
	enum poll_type type;
};

static struct poll_data poll;

/// Returns the sector number corresponding to address input
#define FLASH_NSECT		(sizeof(saddr) / sizeof(uint16_t))

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

void FlashInit(void){
	memset(&poll, 0, sizeof(struct poll_data));
}

uint8_t FlashGetManId(void) {
	uint8_t retVal;

	// Obtain manufacturer ID and reset interface to return to array read.
	FlashAutoselect();
	retVal = FlashRead(FLASH_MANID_RD[0]);
	FlashReset();

	return retVal;
}

void FlashGetDevId(uint8_t devId[3]) {
	// Obtain device ID and reset interface to return to array read.
	FlashAutoselect();
	devId[0] = FlashRead(FLASH_DEVID_RD[0]);
	devId[1] = FlashRead(FLASH_DEVID_RD[1]);
	devId[2] = FlashRead(FLASH_DEVID_RD[2]);
	FlashReset();
}

void FlashProg(uint32_t addr, uint16_t data) {
	uint8_t i;

	// Unlock and write program command
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_PROG, i);
	// Write data
	FlashWrite(addr, data);
}

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

uint8_t FlashChipErase(void) {
	uint8_t i;

	// Unlock and write chip erase sequence
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_CHIP_ERASE, i);
	// Poll until erase complete
	return FlashErasePoll(1);
}

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

uint8_t FlashRangeErase(uint32_t addr, uint32_t len) {
	// Index
	uint8_t i, j;
	// Shifted address to compare with 
	uint16_t caddr = addr>>FLASH_SADDR_SHIFT;
	uint16_t clen = (len - 1)>>FLASH_SADDR_SHIFT;

	if (!len) return 0;
	if ((addr + len) > (FLASH_SADDR_MAX<<FLASH_SADDR_SHIFT)) return 1;

	// Find sector containing the initial address
	for (i = FLASH_NSECT - 1; (caddr < saddr[i]) && i; i--);

	// Find sector containing the end address
	for (j = FLASH_NSECT - 1; ((caddr + clen) < saddr[j]) && j; j--);

	// Special case: erase full chip
	if ((0 == i) && ((FLASH_NSECT - 1 ) == j)) {
		return FlashChipErase();
	}

	for (; i <= j; i++) {
		if (!FlashSectErase(((uint32_t)saddr[i])<<FLASH_SADDR_SHIFT)) {
			return 2;
		}
	}

	return 0;
}

void flash_chip_erase(void *ctx)
{
	uint8_t i;

	// Unlock and write chip erase sequence
	FlashUnlock();
	FLASH_WRITE_CMD(FLASH_CHIP_ERASE, i);
	poll.type = FLASH_POLL_CHIP;
	poll.addr = 1;
	poll.data = 0xFF;
	poll.ctx = ctx;
}

void flash_sector_erase(uint32_t addr, void *ctx)
{
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
	poll.type = FLASH_POLL_SECT;
	poll.addr = addr;
	poll.data = 0xFF;
	poll.ctx = ctx;
}

static void range_erase_cb(int err, void *ctx)
{
	if (err) {
		poll.cb = ctx;
		if (poll.cb) {
			poll.cb(1, NULL);
		}
		return;
	}
	poll.cur_sect++;
	if (poll.cur_sect <= poll.fin_sect) {
		// Relaunch sector erase with new sector
		flash_sector_erase((uint32_t)saddr[poll.cur_sect]<<
				FLASH_SADDR_SHIFT, ctx);
	} else {
		poll.cb = ctx;
		if (poll.cb) {
			poll.cb(0, NULL);
		}
	}
}

int flash_range_erase(uint32_t addr, uint32_t len)
{
	// Index
	uint8_t i, j;
	// Shifted address to compare with 
	uint16_t caddr = addr>>FLASH_SADDR_SHIFT;
	uint16_t clen = (len - 1)>>FLASH_SADDR_SHIFT;
	void *ctx;

	if (!len) return 0;
	if ((addr + len) > (FLASH_SADDR_MAX<<FLASH_SADDR_SHIFT)) {
		return 1;
	}

	// Find sector containing the initial address
	for (i = FLASH_NSECT - 1; (caddr < saddr[i]) && i; i--);

	// Find sector containing the end address
	for (j = FLASH_NSECT - 1; ((caddr + clen) < saddr[j]) && j; j--);

	// Special case: erase full chip
	if ((0 == i) && ((FLASH_NSECT - 1 ) == j)) {
		flash_chip_erase(NULL);
	} else {
		ctx = poll.cb;
		poll.cb = range_erase_cb;
		poll.type = FLASH_POLL_SECT;
		// Store end sector address
		poll.cur_sect = i;
		poll.fin_sect = j;
		flash_sector_erase(((uint32_t)saddr[i])<<FLASH_SADDR_SHIFT,
				ctx);
	}

	return 0;
}

int flash_write_buf(uint32_t addr, uint16_t *data, uint16_t wlen, void *ctx)
{
	// Sector address
	uint32_t sa;
	// Number of words to write
	uint8_t wc;
	// Index
	uint8_t i;

	// Obtain the sector address
	sa = FLASH_SA_GET(addr);
	// Compute the number of words to write minus 1. Maximum number is 15,
	// but without crossing a write-buffer page
	wc = MIN(wlen, FLASH_CHIP_WBUFLEN - ((addr>>1) &
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
	poll.type = FLASH_POLL_DATA;
	poll.addr = addr - 1;
	poll.data = data[i - 1];
	poll.ctx = ctx;

	// Return number of elements (words) to write
	return i;
}

static void flash_write_long_cb(int err, void *ctx)
{
	int written;
	struct write_long_data *w = &poll.write;

	if (err) {
		goto out;
	}
	if (poll.write.wpos >= poll.write.buf_wlength) {
		goto out;
	}
	written = flash_write_buf(w->addr, w->buf, w->buf_wlength - w->wpos,
			ctx);
	if (written <= 0) {
		err = 1;
		goto out;
	}

	poll.write.wpos += written;
	poll.write.addr += 2 * written;
	poll.write.buf += written;

	return;


out:
	poll.type = FLASH_POLL_NONE;
	poll.cb = ctx;
	if (poll.cb) {
		poll.cb(err, ctx);
	}
}

int flash_write_long(uint32_t addr, uint16_t *data, uint16_t wlen)
{
	void *ctx = poll.cb;
	int written;

	poll.cb = flash_write_long_cb;
	poll.write.addr = addr;
	poll.write.buf = data;
	poll.write.buf_wlength = wlen;
	poll.write.wpos = 0;
	written = flash_write_buf(addr, data, wlen, ctx);
	if (written <= 0) {
		return written;
	}
	poll.write.wpos += written;
	poll.write.addr += 2 * written;
	poll.write.buf += written;

	return 0;
}

void flash_poll_proc(void)
{
	uint8_t read;
	int err = 0;

	if (!poll.type) {
		return;
	}

	read = FlashRead(poll.addr);
	if ((read & 0x80) == (poll.data & 0x80)) {
		goto complete;
	}
	if (read & 0x20) {
		read = FlashRead(poll.addr);
		if ((read & 0x80) == (poll.data & 0x80)) {
			goto complete;
		} else {
			err = 1;
			FlashReset();
			goto complete;
		}
	}

	return;

complete:
	poll.type = FLASH_POLL_NONE;
	if (poll.cb) {
		poll.cb(err, poll.ctx);
	}
}

void flash_completion_cb_set(completion_cb cb)
{
	poll.cb = cb;
}

