/************************************************************************//**
 * \brief System controller for wflash. Keeps the system status and
 * processes incoming events, to perform the requested actions.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2017
 ****************************************************************************/
#include <string.h>
#include "sysfsm.h"
#include "cmds.h"
#include "flash.h"
#include "util.h"
#include "loop.h"
#include "menu_imp/menu_itm.h"

//#include "vdp.h"

/// System states
//enum sf_stat {
//	WF_IDLE = 0,		///< Idle state, waiting for commands
//	WD_CMD_WAIT,		///< Waiting for a command
//	WF_DATA_WAIT,		///< Waiting for data to write to Flash
//	WF_FLASHING,		///< Using flash chip
//	WF_STATE_MAX		///< State limit. Do not use.
//};

const char * const lsd_err[] = {
	"FRAMING ERROR",
	"INVALID CHANNEL",
	"FRAME TOO LONG",
	"IN PROGRESS",
	"UNDEFINED ERROR",
	"COMPLETE",
	"BUSY"
};

static const char *get_lsd_err(enum lsd_status stat)
{
	return lsd_err[stat - LSD_STAT_ERR_FRAMING];
}

/// Local module data structure
struct sf_data {
	char *buf[2];		///< Command buffer (double buffered)
	uint32_t addr;		///< Address to which write
	int32_t rem_recv;	///< Remaining bytes to receive
	int32_t rem_write;	///< Remaining bytes to write
	struct loop_func f;	///< Loop function for flash polling
//	uint16_t pos;		///< Position in receive buffer
	int16_t buf_length;	///< Command buffer length
	uint16_t recvd[2];	///< Number of bytes received on each buffer
	uint16_t to_write;	///< Number of bytes from buffer to write
	/// Menu instance for text drawing
	struct menu_entry_instance *instance;
	uint8_t next_idx;	///< Next empty frame
	uint8_t avail_idx;	///< Next ready frame
	uint8_t avail_frames;	///< Available (filled) frames
	struct {
		uint8_t busy_flash:1;	///< Flash is erasing/writing data
		uint8_t busy_recv:1;	///< We are receiving data
	};
};

// Local callback function prototypes
static void cmd_recv_cb(enum lsd_status stat, uint8_t ch,
		char *data, uint16_t len, void *ctx);
static void flash_done_cb(int err, void *ctx);
static void data_recv_cb(enum lsd_status stat, uint8_t ch,
		char *data, uint16_t len, void *ctx);

/// Module local data
static struct sf_data d;

static void flash_poll_cb(struct loop_func *f)
{
	UNUSED_PARAM(f);
	flash_poll_proc();
}

void sf_init(char *cmd_buf, int16_t buf_length,
		struct menu_entry_instance *instance)
{
	d.buf[0] = cmd_buf;
	d.buf[1] = cmd_buf + buf_length;
	d.buf_length = buf_length;
	d.instance = instance;
	d.f.func_cb = flash_poll_cb;
	d.f.disabled = TRUE;
	flash_completion_cb_set(flash_done_cb);
}

// If context is not NULL, command reception is not restarted
static void send_complete_cb(enum lsd_status stat, void *ctx)
{
	if (LSD_STAT_COMPLETE == stat && !ctx) {
		// Command complete, receive another one
		sf_start();
	}
}

static int sf_cmd_version_get(wf_buf *in, int16_t len)
{
	int ret = len;

	// sanity checks
	if (0 == ByteSwapWord(in->cmd.len) &&
			WF_HEADLEN == len) {
		in->cmd.len = ByteSwapWord(2);
		in->cmd.cmd = WF_CMD_OK;
		in->cmd.data[0] = WF_VERSION_MAJOR;
		in->cmd.data[1] = WF_VERSION_MINOR;
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN + 2,
				NULL, send_complete_cb);
	} else {
		in->cmd.len = 0;
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_id_get(wf_buf *in, int len)
{
	int ret = len;

	// sanity checks
	if (0 == ByteSwapWord(in->cmd.len) &&
			WF_HEADLEN == len) {
		in->cmd.data[0] = FlashGetManId();
		FlashGetDevId(in->cmd.data + 1);
		in->cmd.cmd = WF_CMD_OK;
		in->cmd.len = ByteSwapWord(4);
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN + 4,
				NULL, send_complete_cb);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		in->cmd.len = 0;
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
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
		mw_send(WF_CHANNEL, in->sdata, len, NULL, send_complete_cb);
	} else {
		in->cmd.len = 0;
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
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
	mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN, NULL, send_complete_cb);

	return ret;
}

static void sf_err_print(const char *err)
{
	struct menu_str str = {.str = (char*)err};
	str.length = strlen(err);
	menu_str_line_draw(&str, 3, 0, MENU_H_ALIGN_CENTER, 0, 0);
}

static int frame_check(enum lsd_status stat, char *buf, uint8_t ch,
		int16_t len, lsd_recv_cb retry_cb)
{
	if (LSD_STAT_COMPLETE != stat) {
		sf_err_print(get_lsd_err(stat));
		return 1;
	}
	if (ch != SF_CHANNEL) {
		sf_err_print("INVALID CHANNEL!");
		mw_recv(buf, d.buf_length, NULL, retry_cb);
		return 1;
	}

	if (len <= 0) {
		if (MW_SOCK_TCP_EST != mw_sock_stat_get(SF_CHANNEL)) {
			// Connection lost
			sf_err_print("CONNECTION LOST!");
			return 1;
		} else {
			// No data to process, return error but try again
			mw_recv(buf, d.buf_length, NULL, retry_cb);
			return 1;
		}
	}

	return 0;
}

static void flash_action(void)
{
	if (!d.busy_recv && (d.rem_recv > 0) && d.avail_frames < 2) {
		d.busy_recv = TRUE;
		mw_recv(d.buf[d.next_idx], d.buf_length, NULL, data_recv_cb);
	}
	if (!d.busy_flash && (d.rem_write) && d.avail_frames) {
		d.busy_flash = TRUE;
		d.to_write = MIN(d.recvd[d.avail_idx], d.rem_write);
		flash_write_long(d.addr, (uint16_t*)d.buf[d.avail_idx],
				d.to_write / 2);
		loop_func_enable(&d.f);
	}
}

static void flash_done_cb(int err, void *ctx)
{
	UNUSED_PARAM(ctx);
	uint16_t remaining;

	if (err) {
		// Programming failed!
		loop_func_del(&d.f);
		sf_err_print("PROGRAMMING FAILED!");
		// TODO Cancel reception of remaining data
		return;
	}
	d.rem_write -= d.to_write;
	if (d.rem_write <= 0) {
		// We are done. If there are remaining bytes, they are from
		// a new command following the data transfer
		loop_func_del(&d.f);
		remaining = d.recvd[d.avail_idx] - d.to_write;
		if (remaining > 0) {
			// Got next command, process it
			cmd_recv_cb(LSD_STAT_COMPLETE, SF_CHANNEL,
					d.buf[d.avail_idx] + d.to_write,
					remaining, NULL);
		} else {
                       // Clean end, restart command parser
                       sf_start();
		}
	} else {
		// More data to come
		d.avail_frames--;
		d.busy_flash = FALSE;
		d.avail_idx ^= 1;
		d.addr += d.to_write;
		loop_func_disable(&d.f);
	
		flash_action();
	}
}

static void data_recv_cb(enum lsd_status stat, uint8_t ch,
		char *data, uint16_t len, void *ctx)
{
	UNUSED_PARAM(ctx);
	int err;

	err = frame_check(stat, data, ch, len, data_recv_cb);
	if (err) {
		loop_func_del(&d.f);
		d.rem_recv = d.rem_write = -1;
		// TODO Cancel writing
		return;
	}

	d.busy_recv = FALSE;
	d.recvd[d.next_idx] = len;
	d.avail_frames++;
	d.rem_recv -= len;
	d.next_idx ^= 1;

	flash_action();
}

static int sf_cmd_program(wf_buf *in, int16_t len, struct menu_item *item)
{
	int ret = len;

	// sanity check
	if ((len == (ByteSwapWord(in->cmd.len) + WF_HEADLEN)) &&
			((ByteSwapDWord(in->cmd.mem.addr) +
			  ByteSwapDWord(in->cmd.mem.len)) <
			 FLASH_CHIP_LENGTH)) {
		// Acknowledge command and start data reception
		menu_str_replace(&item[2].caption, "PROGRAM: ");
		item[2].caption.length +=
			uint32_to_hex_str(ByteSwapDWord(in->cmd.mem.addr),
					item[2].caption.str + 9, 6);
		menu_item_draw(MENU_PLACE_CENTER);
		
		in->cmd.len = 0;
		in->cmd.cmd = WF_CMD_OK;
		d.addr = ByteSwapDWord(in->cmd.mem.addr);
		d.rem_recv = ByteSwapDWord(in->cmd.mem.len);
		d.rem_write = d.rem_recv;
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				(void*)1, send_complete_cb);
		// Start data reception and program
		// For the first data frame, use buffer 1, since buffer
		// 0 is in use for the command response
		d.next_idx = 1;
		d.avail_idx = 1;
		d.avail_frames = 0;
		d.busy_flash = FALSE;
		d.busy_recv = FALSE;
		loop_func_add(&d.f);
		loop_func_disable(&d.f);

		flash_action();
	} else {
		sf_err_print("PROGRAM CMD ERROR!");
		in->cmd.len = 0;
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		ret = -1;
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
	}

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
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
		sf_boot(entry);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
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
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
		sf_boot(SF_ENTRY_POINT_ADDR);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
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
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN + 4,
				NULL, send_complete_cb);
	} else {
		in->cmd.cmd = ByteSwapWord(WF_CMD_ERROR);
		mw_send(WF_CHANNEL, in->sdata, WF_HEADLEN,
				NULL, send_complete_cb);
		ret = -1;
	}

	return ret;
}

static int sf_cmd_proc(wf_buf *in, int16_t len)
{
	struct menu_item *item = d.instance->entry->item_entry->item;
	// NOTE: A write command should never be accompanied by data
	// payload, write command must be acknowledged befor client
	// starts sending data.
	switch (ByteSwapWord(in->cmd.cmd)) {
	// Get bootloader version
	case WF_CMD_VERSION_GET:
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

static void cmd_recv_cb(enum lsd_status stat, uint8_t ch,
		char *data, uint16_t len, void *ctx)
{
	UNUSED_PARAM(ctx);
	int err;

	err = frame_check(stat, data, ch, len, cmd_recv_cb);

	if (!err) {
		sf_cmd_proc((wf_buf*)data, len);
	}
}

void sf_start(void)
{
	mw_recv(d.buf[0], d.buf_length, NULL, cmd_recv_cb);
}

static void boot_timer_cb(struct loop_timer *t)
{
	UNUSED_PARAM(t);

	loop_post(1);
}

/************************************************************************//**
 * Boot from specified address.
 *
 * \param[in] addr Address from which to boot.
 * \todo Move this to sys.h
 ****************************************************************************/
void boot_addr(uint32_t addr);

void sf_boot(uint32_t addr) {
	struct loop_timer t = {
		.timer_cb = boot_timer_cb,
		.frames = 2
	};
	int i;

	// Wait between 1 and 2 frames for the message to be sent
	loop_timer_add(&t);
	loop_pend();
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

