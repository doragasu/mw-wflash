/************************************************************************//**
 * \brief 1985 Channel
 * \author Jesús Alonso (doragasu)
 * \date   2019
 * \defgroup 1985ch main
 * \{
 ****************************************************************************/
#include <string.h>
#include "vdp.h"
#include "util.h"
#include "gamepad.h"
#include "mpool.h"
#include "mw/megawifi.h"
#include "menu_imp/menu.h"
#include "menu_mw/menu_main.h"
#include "loop.h"
#include "sysfsm.h"
#include "snd/sound.h"
#include "gfx/background.h"

/// Length of the wflash buffer
#define MW_BUFLEN	1440

/// TCP port to use (set to Megadrive release year ;-)
#define MW_CH_PORT 	1985

/// Maximum number of loop functions
#define MW_MAX_LOOP_FUNCS	2

/// Maximun number of loop timers
#define MW_MAX_LOOP_TIMERS	4

/// Holds data required for the download process
struct download_menu_data {
	/// Loop timer running the download operation
	struct loop_timer timer;
	/// Menu instance
	struct menu_entry_instance *instance;
};

/// Command buffer (double buffered)
static char cmd_buf[2 * MW_BUFLEN];

/// Local data for the download operation
static struct download_menu_data dl;

/// Function that performs the download in a loop_timer context
static void download_mode_frame_cb(struct loop_timer *t)
{
	struct download_menu_data *d = container_of(t,
			struct download_menu_data, timer);
	struct menu_item_entry *entry = d->instance->entry->item_entry;
	struct menu_item *item = entry->item;
	uint8_t slot = d->instance->prev->sel_item;
	struct mw_ip_cfg *ip;
	enum mw_err err = FALSE;
	char ip_addr[16];

	err = mw_ap_assoc(slot);
	if (!err) {
		err = mw_ap_assoc_wait(30 * 60);
	}
	if (!err) {
		err = mw_ip_current(&ip);
		uint32_to_ip_str(ip->addr.addr, ip_addr);
	}
	if (!err) {
		err = mw_tcp_bind(SF_CHANNEL, SF_PORT);
	}
	if (!err) {
		menu_str_replace(&item[0].caption, "Associated. IP: ");
		menu_str_append(&item[0].caption, ip_addr);
		menu_item_draw(MENU_PLACE_CENTER);

		err = mw_sock_conn_wait(SF_CHANNEL, 0);
	}
	if (!err) {
		menu_str_replace(&item[0].caption, "Connected to client!");
		menu_item_draw(MENU_PLACE_CENTER);
		sf_init(cmd_buf, MW_BUFLEN, d->instance);
		sf_start();
//		menu_str_replace(&item[0].caption, "FINISHED");
	}
	loop_timer_del(t);
}

/// This callback will be run when user enters DOWNLOAD MODE
/// \note This should be moved to menu_mw/menu_dl.c, but since it needs to see
/// cmd_buf, we are leaving it here by now.
int download_mode_menu_cb(struct menu_entry_instance *instance)
{
	// Launch a one shot timer to do the work, and return for
	// the menu to scroll
	memset(&dl.timer, 0, sizeof(struct loop_timer));
	dl.timer.frames = 1;
	dl.timer.timer_cb = download_mode_frame_cb;
	dl.instance = instance;

	loop_timer_add(&dl.timer);

	return 0;
}

static void idle_cb(struct loop_func *f)
{
	UNUSED_PARAM(f);
	mw_process();
}

/// Run once per frame
static void frame_cb(struct loop_timer *t)
{
	UNUSED_PARAM(t);
	uint8_t pad_ev;

	// Read controller and update menu
	pad_ev = ~gp_pressed();
	menu_update(pad_ev);
}

/// MegaWiFi initialization
static void megawifi_init_cb(struct loop_timer  *t)
{
	uint8_t ver_major = 0, ver_minor = 0;
	char *variant;
	char str_buf[20];
	struct menu_str stat;
	enum mw_err err;

	// megawifi_init_cb is run only once. Use idle_cb from now on
	t->timer_cb = frame_cb;
	stat.str = str_buf;

	// Try detecting the module
	err = mw_detect(&ver_major, &ver_minor, &variant);

	if (MW_ERR_NONE != err) {
		// Set menu status string to show Megawifi was not found
		stat.length = menu_str_buf_cpy(str_buf, "MegaWiFi?", 20 - 1);
	} else {
		// Set menu status string to show readed version
		stat.length = menu_str_buf_cpy(str_buf, "MW RTOS ", 20 - 1);
		stat.length += uint8_to_str(ver_major,
				str_buf + stat.length);
		str_buf[stat.length++] = '.';
		stat.length += uint8_to_str(ver_minor,
				str_buf + stat.length);
	}
	str_buf[20 - 1] = '\0';	// Ensure null termination
	menu_stat_str_set(&stat);
}

/// Loop run while idle
static void main_loop_init(void)
{
	static struct loop_timer frame_timer = {
		.timer_cb = megawifi_init_cb,
		.frames = 1,
		.auto_reload = TRUE
	};
	static struct loop_func megawifi_loop = {
		.func_cb = idle_cb
	};

	loop_init(MW_MAX_LOOP_FUNCS, MW_MAX_LOOP_TIMERS);
	loop_timer_add(&frame_timer);
	loop_func_add(&megawifi_loop);
}

/// Global initialization
static void init(void)
{
	// Initialize memory pool
	mp_init(0);
	// Initialize VDP
	VdpInit();
	// Initialize gamepad
	gp_init();
	// Initialize game loop
	main_loop_init();
	// Initialize MegaWiFi
	mw_init(cmd_buf, MW_BUFLEN);
	// Initialize menu system
	menu_init(&main_menu, &(struct menu_str)MENU_STR_RO("Init..."));
	// Initializes sound, starts the song
	sound_init(menu_01_00_data, sfx_data);
	// Initialize scrolling background layer
	bg_init();
}

/// Entry point
int entry_point(uint16_t hard)
{
	UNUSED_PARAM(hard);

	init();

	// Enter game loop (should never return)
	loop();

	return 1;
}

/** \} */

