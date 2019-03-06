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
#include "menu_imp/menu_itm.h"

//#include "vdp.h"

/// System states
enum sf_stat {
	WF_IDLE,			///< Idle state, waiting for commands
	WF_DATA_WAIT,		///< Waiting for data to write to Flash
	WF_STATE_MAX		///< State limit. Do not use.
};

/// Local module data structure
struct sf_data {
	char *buf;		///< Command buffer
	int16_t buf_length;	///< Command buffer length
	uint32_t waddr;		///< Word address to which write
	uint32_t wrem;		///< Remaining words to write
	enum sf_stat s;		///< System status
};

/// Module local data
static struct sf_data d;

void sf_init(char *cmd_buf, int16_t buf_length)
{
	d.buf = cmd_buf;
	d.buf_length = buf_length;
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
static int sf_write(wf_buf *in, uint16_t len) {
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

static int sf_cmd_version_get(wf_buf *in, int16_t len)
{
	int ret = len;

	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("VERSION_GET"), 2, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
	// sanity checks
	if (0 == ByteSwapWord(in->cmd.len) &&
			WF_HEADLEN == len) {
		in->cmd.len = ByteSwapWord(2);
		in->cmd.cmd = WF_CMD_OK;
		in->cmd.data[0] = WF_VERSION_MAJOR;
		in->cmd.data[1] = WF_VERSION_MINOR;
	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("SENDING"), 3, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN + 2, 0);
	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("SENT"), 3, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
	} else {
		in->cmd.len = 0;
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_id_get(wf_buf *in, int len)
{
	int ret = len;

	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("ID_GET"), 2, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
	// sanity checks
	if (0 == ByteSwapWord(in->cmd.len) &&
			WF_HEADLEN == len) {
	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("QUERY FLASH"), 3, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
		in->cmd.data[0] = FlashGetManId();
		FlashGetDevId(in->cmd.data + 1);
		in->cmd.cmd = WF_CMD_OK;
		in->cmd.len = ByteSwapWord(4);
	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("SENDING"), 3, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN + 4, 0);
	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("SENT"), 3, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		in->cmd.len = 0;
	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("FRAME ERROR"), 3, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_echo(wf_buf *in, int16_t len)
{
	int ret = len;

	// sanity check
	if (len == ByteSwapWord(in->cmd.len) + WF_HEADLEN) {
		in->cmd.cmd = WF_CMD_OK;
		mw_send_sync(WF_CHANNEL, in->sdata, len, 0);
	} else {
		in->cmd.len = 0;
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_erase(wf_buf *in, int16_t len, struct menu_item *item)
{
	int ret = len;
	const int cmd_len = sizeof(struct wf_mem_range);

	// sanity check
	if (((cmd_len + WF_HEADLEN) == len) &&
			(cmd_len == ByteSwapWord(in->cmd.len))) {
		menu_str_replace(&item[2].caption, "ERASING...");
		menu_item_draw(MENU_PLACE_CENTER);
		if (!FlashRangeErase(ByteSwapDWord(in->cmd.mem.addr),
					ByteSwapDWord(in->cmd.mem.len))) {
			in->cmd.cmd = WF_CMD_OK;
		} else {
			in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
			ret = -1;
		}
	}
	in->cmd.len = 0;
	mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);

	return ret;
}

static int sf_cmd_program(wf_buf *in, int16_t len, struct menu_item *item)
{
	int ret = len;
	char addr[7];

	// sanity check
	if ((len == (ByteSwapWord(in->cmd.len) + WF_HEADLEN)) &&
			((ByteSwapDWord(in->cmd.mem.addr) +
			  ByteSwapDWord(in->cmd.mem.len)) <
			 FLASH_CHIP_LENGTH)) {
		// Acknowledge command and transition to WF_DATA_WAIT
		menu_str_replace(&item[2].caption, "PROGRAM: ");
		uint32_to_hex_str(ByteSwapDWord(in->cmd.mem.addr), addr, 6);
		menu_str_append(&item[2].caption, addr);
		menu_item_draw(MENU_PLACE_CENTER);
		
		in->cmd.cmd = WF_CMD_OK;
		d.s = WF_DATA_WAIT;
		d.waddr = ByteSwapDWord(in->cmd.mem.addr)>>1;
		d.wrem = ByteSwapDWord(in->cmd.mem.len)>>1;
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		ret = -1;
	}
	in->cmd.len = 0;
	mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);

	return ret;
}

static int sf_cmd_run(wf_buf *in, int len)
{
	int ret = len;
	uint32_t entry;

	in->cmd.len = ByteSwapWord(0);
	// sanity check
	if (((WF_HEADLEN + 4) == len) &&
			(4 == ByteSwapWord(in->cmd.len))) {
		// Boot program at specified address
		entry = ByteSwapDWord(in->cmd.dwdata[0]);
		in->cmd.cmd = WF_CMD_OK;
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		sf_boot(entry);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_autorun(wf_buf *in, int len)
{
	int ret = len;

	in->cmd.len = ByteSwapWord(0);
	// sanity check
	if ((WF_HEADLEN == len) &&
		(0 == ByteSwapWord(in->cmd.len))) {
		in->cmd.cmd = WF_CMD_OK;
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		sf_boot(SF_ENTRY_POINT_ADDR);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_bload_addr_get(wf_buf *in, int16_t len)
{
	int ret = len;

	in->cmd.len = ByteSwapWord(0);
	// sanity check
	if ((WF_HEADLEN == len) && (0 == ByteSwapWord(in->cmd.len))) {
		in->cmd.cmd = WF_CMD_OK;
		in->cmd.len = ByteSwapWord(4);
		in->cmd.dwdata[0] = ByteSwapDWord(SF_BOOTLOADER_ADDR);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN + 4, 0);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send_sync(WF_CHANNEL, in->sdata, WF_HEADLEN, 0);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_proc(wf_buf *in, int16_t len, struct menu_item *item)
{
	// NOTE: A write command should never be accompanied by data
	// payload, write command must be acknowledged befor client
	// starts sending data.
	switch (ByteSwapWord(in->cmd.cmd)) {
	// Get bootloader version
	case WF_CMD_VERSION_GET:
	menu_str_line_draw(&(struct menu_str)MENU_STR_RO("VERSION_GET"), 2, 1, MENU_H_ALIGN_CENTER, MENU_COLOR_ITEM, 0);
		len = sf_cmd_version_get(in, len);
		break;

	// Get Flash IDs
	case WF_CMD_ID_GET:
		len = sf_cmd_id_get(in, len);
		break;

	// Echo data (for debugging purposes)
	case WF_CMD_ECHO:
		len = sf_cmd_echo(in, len);
		break;

	// Erase flash
	case WF_CMD_ERASE:
		len = sf_cmd_erase(in, len, item);
		break;

	// Program flash
	case WF_CMD_PROGRAM:
		len = sf_cmd_program(in, len, item);
		break;

	// Run program from address
	case WF_CMD_RUN:
		len = sf_cmd_run(in, len);
		break;

	// Run program, determine start address automagically
	case WF_CMD_AUTORUN:
		len = sf_cmd_autorun(in, len);
		break;

	// Get bootloader start address
	case WF_CMD_BLOADER_START:
		len = sf_cmd_bload_addr_get(in, len);
		break;

	default:
		len = -1;
	}

	return len;
}

static int sf_data_write(wf_buf *in, uint16_t len, struct menu_item *item)
{
	uint16_t to_write;

	// Because command does not require confirmation at this point,
	// if we receive more data than remaining to write, it could be
	// because TCP has joined on a single datagram the data payload
	// and a command request sent immediately after the data. So
	// there is more data to process after we finish writing.
	//
	// FIXME: It could happen that what is received after the data
	// payload is a partial command (i.e. some command data is
	// still missing). Support for handling this case is not yet
	// implemented.
	to_write = MIN(len, d.wrem<<1);
	// Write data to Flash
	switch (sf_write(in, to_write)) {
	case -1: // Error
		menu_str_replace(&item[2].caption, "ERROR!");
		menu_item_draw(MENU_PLACE_CENTER);
		to_write = -1;
		break;

	case 1: // Finished
		// TODO: Draw position
		d.s = WF_IDLE;
		char temp[10];
		uint32_to_hex_str(len, temp, 4);
		temp[4] = ' ';
		uint32_to_hex_str(to_write + 5, temp, 4);
		temp[9] = '\0';
		menu_str_replace(&item[2].caption, temp);
//		item[2].caption.str[0] = '\0';
//		item[2].caption.length = 0;
		menu_item_draw(MENU_PLACE_CENTER);
		break;

	case 0:	// OK, more data pending
	default:
		break;
	}

	return to_write;
}

static int sf_data_proc(wf_buf *in, int16_t len, struct menu_item *item)
{
	switch (d.s) {
	case WF_IDLE:
		// Awaiting command
		len = sf_cmd_proc(in, len, item);
		break;

	case WF_DATA_WAIT:
		len = sf_data_write(in, len, item);
		break;

	default:
		len = -1;
		break;
	}

	return len;
}

enum mw_err sf_cycle(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	enum mw_err err;
	uint8_t ch;
	int16_t len = d.buf_length;
	int16_t processed;
	int offset = 0;
	wf_buf *in = (wf_buf*)d.buf;

	// Receive data
	err = mw_recv_sync(&ch, d.buf, &len, 0);
	if (err) {
		return err;
	}
	if (ch != SF_CHANNEL) {
		// Ignore frame
		// FIXME Maybe someone should be able to read this data
		return MW_ERR_NONE;
	}

	// If we have received no payload, maybe the connection has been lost
	if (len <= 0) {
		if (MW_SOCK_TCP_EST != mw_sock_stat_get(SF_CHANNEL)) {
			return MW_ERR;
		} else {
			return MW_ERR_NONE;
		}
	}

	// While there is data to process...
	while (len) {
		processed = sf_data_proc(in, len, item);
		if (processed > 0) {
			len -= processed;
			offset += processed;
			in = (wf_buf*)(d.buf + offset);
		} else {
			// Error, exit
			len = 0;
		}
	}

	return MW_ERR_NONE;
}

/************************************************************************//**
 * Boot from specified address.
 *
 * \param[in] addr Address from which to boot.
 * \todo Move this to sys.h
 ****************************************************************************/
void boot_addr(uint32_t addr);

void sf_boot(uint32_t addr) {
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
	mw_module_reset();

	// boot
	boot_addr(addr);
}

