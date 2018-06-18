/************************************************************************//**
 * \brief System controller for wflash. Keeps the system status and
 * processes incoming events, to perform the requested actions.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2017
 ****************************************************************************/
#include "sysfsm.h"
#include "cmds.h"
#include "flash.h"
#include "util.h"
#include "mw/megawifi.h"

//#include "vdp.h"

/// System states
typedef enum {
	WF_IDLE,			///< Idle state, waiting for commands
	WF_DATA_WAIT,		///< Waiting for data to write to Flash
	WF_STATE_MAX		///< State limit. Do not use.
} SfStat;

/// Local module data structure
typedef struct {
	uint32_t waddr;		///< Word address to which write
	uint32_t wrem;		///< Remaining words to write
	SfStat s;			///< System status
} SfData;

/// Module local data
static SfData d;

void SfInit(void) {
	d.s = WF_IDLE;
}

/************************************************************************//**
 * Write data to flash chip.
 *
 * \param[in] in  Input data buffer to write to cart.
 * \param[in] len Length of the data buffer to write to cart.
 *
 * \return 0 if OK but more data is pending. 1 if finished. -1 if error.
 ****************************************************************************/
static int SfWrite(WfBuf *in, uint16_t len) {
	uint16_t i, wlen;
	uint8_t wwrite;

	// Convert byte length into word length
	wlen = len>>1;

	if (wlen > d.wrem) {
		return -1;
	}

	// Write not aligned data
	wwrite = FLASH_CHIP_WBUFLEN - (d.waddr & (FLASH_CHIP_WBUFLEN - 1));
	wwrite = MIN(wwrite, wlen);
	if (FlashWriteBuf(d.waddr<<1, in->wdata, wwrite) != wwrite) return -1;
	d.waddr += wwrite;
	// Write received buffer in blocks
	for (i = wwrite; i < wlen; i += wwrite, d.waddr += wwrite) {
		wwrite = MIN(FLASH_CHIP_WBUFLEN, wlen - i);
		if (FlashWriteBuf(d.waddr<<1, in->wdata + i, wwrite) != wwrite) {
			return -1;
		}

	}
	// Update remaining words, and check if finished
	d.wrem  -= wlen;
	if (!d.wrem) return 1;
	return 0;
}

int SfCycle(Menu *md) {
	MenuItem *item = &(md->me->mEntry.mItem.item[2]);
	int retVal = 0;
	WfBuf *in;
	uint16_t len, lenTmp;
	int ch;
	uint32_t entry;

	// Receive data
	if ((ch = MwRecv((uint8_t**)&in, &len, MW_DEF_MAX_LOOP_CNT)) == MW_ERROR)
		return MW_ERROR;
	if (ch != WF_CHANNEL) return MW_OK; 	// No data has been received

	// If we have received no payload, maybe the connection has been lost
	if (!len) {
		// Check socket status
		if (MW_SOCK_TCP_EST != MwSockStatGet(WF_CHANNEL)) return MW_ERROR;
		else return MW_OK;
	}

	// While there is data to process...
	while (len) {
		// Process received data depending on state
		switch (d.s) {
			case WF_IDLE:
				// Awaiting command
				// NOTE: A write command should never be accompanied by data
				// payload, write command must be acknowledged befor client
				// starts sending data.
				switch (ByteSwapWord(in->cmd.cmd)) {
					// Get bootloader version
					case WF_CMD_VERSION_GET:
						// sanity checks
						if (0 == ByteSwapWord(in->cmd.len) &&
								WF_HEADLEN == len) {
							in->cmd.len = ByteSwapWord(2);
							in->cmd.cmd = WF_CMD_OK;
							in->cmd.data[0] = WF_VERSION_MAJOR;
							in->cmd.data[1] = WF_VERSION_MINOR;
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN + 2);
						} else {
							in->cmd.len = 0;
							in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						}
						break;
	
					// Get Flash IDs
					case WF_CMD_ID_GET:
						// sanity checks
						if (0 == ByteSwapWord(in->cmd.len) &&
								WF_HEADLEN == len) {
							in->cmd.data[0] = FlashGetManId();
							FlashGetDevId(in->cmd.data + 1);
							in->cmd.cmd = WF_CMD_OK;
							in->cmd.len = ByteSwapWord(4);
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN + 4);
						} else {
							in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
							in->cmd.len = 0;
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						}
						break;
	
					// Echo data (for debugging purposes)
					case WF_CMD_ECHO:
						// sanity check
						if (len == ByteSwapWord(in->cmd.len) + WF_HEADLEN) {
							in->cmd.cmd = WF_CMD_OK;
							MwSend(WF_CHANNEL, in->data, len);
						} else {
							in->cmd.len = 0;
							in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						}
						break;
	
					// Erase flash
					case WF_CMD_ERASE:
						// sanity check
						in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
						if (((sizeof(WfMemRange) + WF_HEADLEN) == len) &&
							(sizeof(WfMemRange) == ByteSwapWord(in->cmd.len))) {
							item->caption.length = MenuStrCpy(
                                    item->caption.string, "ERASING...", 0);
                            MenuDrawItemPage(0);
							if (!FlashRangeErase(ByteSwapDWord(
									in->cmd.mem.addr), ByteSwapDWord(
										in->cmd.mem.len))) {
								in->cmd.cmd = WF_CMD_OK;
							}
						}
						in->cmd.len = 0;
						MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						break;
	
					// Program flash
					case WF_CMD_PROGRAM:
						// sanity check
						if ((len == (ByteSwapWord(in->cmd.len) + WF_HEADLEN)) &&
							((ByteSwapDWord(in->cmd.mem.addr) +
							  ByteSwapDWord(in->cmd.mem.len)) <
							 FLASH_CHIP_LENGTH)) {
							// Acknowledge command and transition to
							// WF_DATA_WAIT
							item->caption.length = MenuStrCpy(
                                    item->caption.string, "PROGRAM: ", 0);
                            item->caption.length += Uint32ToHexStr(
                                    ByteSwapDWord(in->cmd.mem.addr),
                                    item->caption.string +
                                    item->caption.length, 6);
                            MenuDrawItemPage(0);
							in->cmd.cmd = WF_CMD_OK;
							d.s = WF_DATA_WAIT;
							d.waddr = ByteSwapDWord(in->cmd.mem.addr)>>1;
							d.wrem = ByteSwapDWord(in->cmd.mem.len)>>1;
						} else in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
						in->cmd.len = 0;
						MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						break;
	
					// Run program from address
					case WF_CMD_RUN:
						in->cmd.len = ByteSwapWord(0);
						// sanity check
						if (((WF_HEADLEN + 4) == len) &&
								(4 == ByteSwapWord(in->cmd.len))) {
							// Boot program at specified address
							entry = ByteSwapDWord(in->cmd.dwdata[0]);
							in->cmd.cmd = WF_CMD_OK;
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
							SfBoot(entry);
						} else {
							in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						}
						break;

					case WF_CMD_AUTORUN:
						in->cmd.len = ByteSwapWord(0);
						// sanity check
						if ((WF_HEADLEN == len) &&
							(0 == ByteSwapWord(in->cmd.len))) {
							in->cmd.cmd = WF_CMD_OK;
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
							SfBoot(SF_ENTRY_POINT_ADDR);
						} else {
							in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						}
						break;
                    
                    // Get bootloader start address
                    case WF_CMD_BLOADER_START:
						in->cmd.len = ByteSwapWord(0);
						// sanity check
						if ((WF_HEADLEN == len) &&
							(0 == ByteSwapWord(in->cmd.len))) {
							in->cmd.cmd = WF_CMD_OK;
                            in->cmd.len = ByteSwapWord(4);
                            in->cmd.dwdata[0] = ByteSwapDWord(
                                    SF_BOOTLOADER_ADDR);
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN + 4);
						} else {
							in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
							MwSend(WF_CHANNEL, in->data, WF_HEADLEN);
						}
                        break;
	
					default:
						break;
				}
				// All the commands above either use all data, or error, so
				// no more data to process here.
				len = 0;
				break;
	
			case WF_DATA_WAIT:
				// Because command does not require confirmation at this point,
				// if we receive more data than remaining to write, it could be
				// because TCP has joined on a single datagram the data payload
				// and a command request sent immediately after the data. So
				// there is more data to process after we finish writing.
				//
				// TODO: It could happen that what is received after the data
				// payload is a partial command (i.e. some command data is
				// still missing). Support for handling this case is not yet
				// implemented.
				lenTmp = MIN(len, d.wrem<<1);
				len -= lenTmp;
				// Write data to Flash
				switch (SfWrite(in, lenTmp)) {
					case -1: // Error
						item->caption.length = MenuStrCpy(
                                item->caption.string, "ERROR!", 0);
                        MenuDrawItemPage(0);
						// fallthrough
					case 1:	 // Finished
						d.s = WF_IDLE;
//                        item->hide = TRUE;
//						VdpLineClear(VDP_PLANEA_ADDR, 7);
						break;
	
					case 0:	// OK, more data pending
					default:
						break;
				}
				in = (WfBuf*)(in->data + lenTmp);
				break;
	
			default:
				break;
		}// switch (d.s)
	}// while (len)
	return retVal;
}

/************************************************************************//**
 * Boot from specified address.
 *
 * \param[in] addr Address from which to boot.
 * \todo Move this to sys.h
 ****************************************************************************/
void BootAddr(uint32_t addr);

void SfBoot(uint32_t addr) {
    int i;

	// Wait between 1 and 2 frames for the message to be sent
	VdpVBlankWait();
	VdpVBlankWait();
	// Clear CRAM
	VdpRamRwPrep(VDP_CRAM_WR, 0);
	for (i = 64; i > 0; i--) VDP_DATA_PORT_W = 0;

	// Clear VRAM
	VdpRamRwPrep(VDP_VRAM_WR, 0);
	for (i = 32768; i > 0; i--) VDP_DATA_PORT_W = 0;

	// Clear VSRAM
	VdpRamRwPrep(VDP_VSRAM_WR, 0);
	for (i = 40; i > 0; i--) VDP_DATA_PORT_W = 0;
	
	// Put WiFi module in reset state
	MwModuleReset();

	// boot
	BootAddr(addr);
}

