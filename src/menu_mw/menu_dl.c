#include "menu_dl.h"
#include "menu_txt.h"
#include "comm_buf.h"
#include "../globals.h"
#include "../sysfsm.h"
#include "../mw/megawifi.h"
#include "../menu_imp/menu.h"
#include "../menu_imp/menu_itm.h"
#include "../gfx/background.h"
#include "../snd/sound.h"

static int reboot_cb(struct menu_entry_instance *instance)
{
	UNUSED_PARAM(instance);
	extern uint32_t dirty_dw;

	// Make sure we boot the loader again
	dirty_dw = MAGIC_WIFI_CONFIG;
	sf_boot(GL_BOOTLOADER_ADDR, TRUE);

	return 1;
}

static void conn_err(struct menu_entry_instance *instance)
{
	struct menu_item_entry *entry = instance->entry->item_entry;
	struct menu_item *item = entry->item;
	struct menu_str *context = &instance->entry->left_context;

	menu_str_replace(&item[0].caption, "Connection error!");
	menu_str_replace(&item[2].caption, "BACK");
	menu_item_draw(MENU_PLACE_CENTER);
	mw_ap_disassoc();
	context->str = ITEM_ACCEPT_STR;
	context->length = context->max_length = sizeof(ITEM_ACCEPT_STR) - 1;
	item[1].entry_cb = reboot_cb;
	menu_redraw_context();
}

static int download_mode_menu_cb(struct menu_entry_instance *instance)
{
	struct menu_item_entry *entry = instance->entry->item_entry;
	struct menu_item *item = entry->item;
	uint8_t ap_slot = instance->prev->sel_item;
	enum mw_err err = FALSE;
	char ip_addr[16] = {0};
	struct mw_ip_cfg *ip = NULL;

	err = mw_ap_assoc(ap_slot);

	if (!err) {
		err = mw_ap_assoc_wait(39 * 60);
		if (!err) {
			menu_str_replace(&item[0].caption, "Connecting to server...");
			menu_item_draw(MENU_PLACE_CENTER);
		}
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
		sf_init(cmd_buf, MW_BUFLEN, instance);
		sf_start();
		instance->entry->periodic_cb = NULL;
		sound_deinit();
//		bg_deinit();
	}

	if (err) {
		conn_err(instance);
	}

	return err;
}

/// Empty menu, data will be manually written on the screen
const struct menu_entry download_start_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("DOWNLOAD MODE"),
	.left_context = MENU_STR_RO(WAIT_STR),
	.periodic_cb = download_mode_menu_cb,
	.item_entry = MENU_ITEM_ENTRY(3, 2, MENU_H_ALIGN_CENTER, 0) {
		{
			.caption = MENU_STR_RW("Associating to access "
					"point...", 38),
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_NULL
		},
		{
			.caption = MENU_STR_EMPTY(15),
			.not_selectable = TRUE,
			.alt_color = TRUE
		}
	} MENU_ITEM_ENTRY_END
};

static int download_menu_select_default_cb(struct menu_entry_instance *instance)
{
	int ap;

	// Select the last saved slot by default
	ap = mw_def_ap_cfg_get();
	if (ap < 0) {
		return 1;
	}
	instance->sel_item = ap;

	return 0;
}

/// Fill slot names with SSIDs
static int download_menu_enter_cb(struct menu_entry_instance *instance)
{
	int i;
	char *ssid;
	int err = 0;
	int configs = 0;
	struct menu_item *item = instance->entry->item_entry->item;

	for (i = 0; i < MW_NUM_CFG_SLOTS; i++) {
		if (MW_ERR_NONE == mw_ap_cfg_get(i, &ssid, NULL, NULL)) {
			if (ssid[0]) {
				menu_str_append(&item[i].caption, ssid);
				configs++;
			} else {
				item[i].alt_color = TRUE;
				item[i].not_selectable = TRUE;
			}
		} else {
			err = 1;
			goto out;
		}
	}

	if (!configs) {
		err = TRUE;
		menu_msg("NO NETWORK CONFIGURED!", "Configure a network"
				" and try again", 0, 0);
	}

out:
	return err;
}

const struct menu_entry download_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("SELECT NETWORK TO USE"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = download_menu_enter_cb,
	.action_cb = download_menu_select_default_cb,
	.item_entry = MENU_ITEM_ENTRY(3, 2, MENU_H_ALIGN_LEFT, 1) {
		{
			.caption = MENU_STR_RW("1: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&download_start_menu
		},
		{
			.caption = MENU_STR_RW("2: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&download_start_menu
		},
		{
			.caption = MENU_STR_RW("3: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&download_start_menu
		}
	} MENU_ITEM_ENTRY_END
};

