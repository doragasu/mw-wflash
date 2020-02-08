#include <string.h>
#include <stdlib.h>
#include "menu_main.h"
#include "menu_net.h"
#include "menu_txt.h"
#include "menu_dl.h"
#include "menu_gtag.h"
#include "../globals.h"
#include "../sysfsm.h"
#include "../menu_imp/menu.h"
#include "../menu_imp/menu_msg.h"
#include "../mw/megawifi.h"
#include "../rom_head.h"

#define NTP_SERV_MAXLEN 	32
#define TIMEZONE_MAXLEN		32

/// Item offsets for the NTP configuration menu
enum {
	MENU_NTP_SERV1 = 1,
	MENU_NTP_SERV2 = 2,
	MENU_NTP_SERV3 = 3,
	MENU_NTP_TIMEZONE = 6
};

static int menu_tz_validate(struct menu_entry_instance *instance)
{
	struct menu_str *text = &instance->entry->osk_entry->tmp;
	int err = 0;

	if (text->length < 3) {
		menu_msg("INVALID TIMEZONE", "Must have at least 3 characters", 0, 5 * 60);
		err = 1;
	}

	return err;
}

const struct menu_entry menu_ntp_tz_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.action_cb = menu_tz_validate,
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Time zone string (e.g. \"UTC+1\"):"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = TIMEZONE_MAXLEN
	}
};

static int menu_non_empty_validate(struct menu_entry_instance *instance)
{
	if ('\0' == *instance->entry->osk_entry->tmp.str) {
		menu_msg("INVALID INPUT", "Input cannot be empty", 0, 5 * 60);
		return 1;
	}

	return 0;
}

const struct menu_entry menu_ntp_serv_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.action_cb = menu_non_empty_validate,
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Enter NTP server:"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = 32
	}
};

static int menu_ntp_save(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	const char *servers[3];
	const char *tz;

	for (int i = 0; i < 3; i++) {
		servers[i] = item[i + MENU_NTP_SERV1].caption.str;
	}
	tz = item[MENU_NTP_TIMEZONE].caption.str;

	if (MW_ERR_NONE != mw_sntp_cfg_set(tz, servers)) {
		menu_msg("ERROR", "Failed to save configuration!", 0, 5 * 60);
		return 1;
	}

	menu_back(1);

	return 0;
}

static int menu_ntp_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	char *server[3] = {0};
	char *tz = NULL;
	enum mw_err err;
	int i;

	err = mw_sntp_cfg_get(&tz, server);
	if (err) {
		menu_msg("ERROR", "Failed to get time configuration", 0, 180);
		return 1;
	}

	for (i = 0; i < 3 && server[i]; i++) {
		menu_str_replace(&item[MENU_NTP_SERV1 + i].caption, server[i]);
	}
	for (; i < 3; i++) {
		menu_str_replace(&item[MENU_NTP_SERV1 + i].caption, "");
	}
	if (tz) {
		menu_str_replace(&item[MENU_NTP_TIMEZONE].caption, tz);
	}

	return 0;
}

const struct menu_entry ntp_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = 8,
	.title = MENU_STR_RO("TIME CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = menu_ntp_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(11, 1, MENU_H_ALIGN_LEFT, 1) {
		{
			.caption = MENU_STR_RO("TIME SERVERS:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(NTP_SERV_MAXLEN),
			.next = (struct menu_entry*)&menu_ntp_serv_osk
		},
		{
			.caption = MENU_STR_EMPTY(NTP_SERV_MAXLEN),
			.next = (struct menu_entry*)&menu_ntp_serv_osk
		},
		{
			.caption = MENU_STR_EMPTY(NTP_SERV_MAXLEN),
			.next = (struct menu_entry*)&menu_ntp_serv_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("Time zone string:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(TIMEZONE_MAXLEN),
			.next = (struct menu_entry*)&menu_ntp_tz_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("DONE!"),
			.entry_cb = menu_ntp_save
		}
	} MENU_ITEM_ENTRY_END
};

static int root_menu_cb(struct menu_entry_instance *instance)
{
	UNUSED_PARAM(instance);

	menu_back(3);

	return 0;
}

static int default_cfg_cb(struct menu_entry_instance *instance)
{
	enum mw_err err;

	err = mw_factory_settings();
	if (MW_ERR_NONE == err) {
		menu_msg("SUCCESS!", "Factory configuration restored", 0, 0);
		mw_sleep(120);
		menu_msg_close();
	} else {
		menu_msg("ERROR!", "Factory restore failed", 0, 0);
		mw_sleep(180);
		menu_msg_close();
	}

	root_menu_cb(instance);

	return 0;
}

const struct menu_entry defaults_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("CONFIRM FACTORY SETTINGS"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.item_entry = MENU_ITEM_ENTRY(5, 2, MENU_H_ALIGN_CENTER, 1) {
		{
			.caption = MENU_STR_RO("THIS WILL DELETE ALL USER SETTINGS!"),
			.alt_color = TRUE,
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("ARE YOU SURE?"),
			.alt_color = TRUE,
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE,
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("NO, TAKE ME OUT!"),
			.entry_cb = root_menu_cb
		},
		{
			.caption = MENU_STR_RO("YES, DELETE EVERYTHING"),
			.entry_cb = default_cfg_cb
		},
	} MENU_ITEM_ENTRY_END
};

const struct menu_entry advanced_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("ADVANCED CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.item_entry = MENU_ITEM_ENTRY(2, 3, MENU_H_ALIGN_CENTER, 1) {
		{
			.caption = MENU_STR_RO("TIME CONFIGURATION"),
			.next = (struct menu_entry*)&ntp_menu
		},
		{
			.caption = MENU_STR_RO("RESET TO DEFAULTS"),
			.next = (struct menu_entry*)&defaults_menu
		},
	} MENU_ITEM_ENTRY_END
};

/// Fill slot names with SSIDs
static int config_menu_enter_cb(struct menu_entry_instance *instance)
{
	int i;
	char *ssid;

	struct menu_item *item = instance->entry->item_entry->item;

	for (i = 0; i < MW_NUM_CFG_SLOTS; i++) {
		if (MW_ERR_NONE == mw_ap_cfg_get(i, &ssid, NULL) &&
				ssid[0] != '\0') {
			menu_str_append(&item[i].caption, ssid);
		}
	}

	return 0;
}

const struct menu_entry config_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = config_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(5, 2, MENU_H_ALIGN_LEFT, 1) {
		{
			.caption = MENU_STR_RW("1: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&net_menu
		},
		{
			.caption = MENU_STR_RW("2: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&net_menu
		},
		{
			.caption = MENU_STR_RW("3: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&net_menu
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("ADVANCED"),
			.next = (struct menu_entry*)&advanced_menu
		}
	} MENU_ITEM_ENTRY_END
};

static int game_boot_cb(struct menu_entry_instance *instance)
{
	UNUSED_PARAM(instance);

	sf_boot(SF_ENTRY_POINT_ADDR, FALSE);

	return 0;
}

static int dl_menu_set_cb(struct menu_entry_instance *instance)
{
	int i;
	char *ssid;
	unsigned int configs = 0;
	unsigned int last_valid_cfg;
	struct menu_item *item =
		&instance->entry->item_entry->item[instance->sel_item];

	for (i = 0; i < MW_NUM_CFG_SLOTS; i++) {
		if (MW_ERR_NONE == mw_ap_cfg_get(i, &ssid, NULL)) {
			if (ssid[0]) {
				configs++;
				last_valid_cfg = i;
			}
		} else {
			return 1;
		}
	}

	switch (configs) {
	case 0:
		item->next = NULL;
		menu_msg("NOT CONFIGURED", "Configure a WiFi "
				"and try again!", 0, 0);
		break;

	case 1:
		mw_log("ONE CONFIG");
		instance->sel_item = last_valid_cfg;
		item->next = (struct menu_entry*)&download_start_menu;
		break;

	default:
		item->next = (struct menu_entry*)&download_menu;
		break;
	}

	return 0;
}

static int main_menu_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;

	// If no game intalled (no valid boot addr), leave boot game
	// entry as not selectable
	if (!GL_ENTRY_POINT_ADDR || 0xFFFFFFFF == GL_ENTRY_POINT_ADDR ||
			0x20202020 == GL_ENTRY_POINT_ADDR) {
		goto out;
	}

	item->alt_color = FALSE;
	item->not_selectable = FALSE;
	menu_str_replace(&item->caption, "START GAME");

out:
	return 0;
}

static int id_get(char *id)
{
	uint8_t *bssid;
	int err = 0;

	bssid = mw_bssid_get(MW_IF_STATION);
	if (bssid) {
		for (int i = 0; i < 6; i++) {
			uint8_to_hex_str(bssid[i], &id[i<<1]);
		}
	} else {
		strcpy(id, "UNKNOWN");
		err = 1;
	}

	return err;
}

static int about_menu_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	struct menu_str *id_str = &item[2].caption;
	char id[13] = "UNKNOWN";

	id_get(id);
	menu_str_append(id_str, id);

	return 0;
}

const struct menu_entry about_menu ROM_DATA(about_menu) = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("ABOUT"),
	.left_context = MENU_STR_RO(ITEM_BACK_STR),
	.enter_cb = about_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(15, 1, MENU_H_ALIGN_CENTER, 1) {
		{
			.caption = MENU_STR_RW("MegaWiFi loader"
					STR(GL_VER_MAJOR) "." STR(GL_VER_MINOR),
					40),
			.alt_color = TRUE,
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RW("Cart ID: ", 22),
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE,
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE,
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("This is part of MegaWiFi project"),
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE,
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE,
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("Code and hardware: @doragasu"),
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("Font and logo: @Manu_Segura_"),
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("Music and SFX: @DavidBonus"),
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("Uses sound player from Shiru"),
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE,
			.not_selectable = TRUE
		},
		{
			.hidden = TRUE,
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_RO("(c) 1985alternativo, 2020"),
			.not_selectable = TRUE
		}
	} MENU_ITEM_ENTRY_END
};

const struct menu_entry main_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("MegaWiFi loader by doragasu"),
	.left_context = MENU_STR_RO("Select an option"),
	.enter_cb = main_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(5, 4, MENU_H_ALIGN_CENTER, 1) {
		{
			.caption = MENU_STR_RW("NO GAME INSTALLED", 40),
			.not_selectable = TRUE,
			.alt_color = TRUE,
			.entry_cb = game_boot_cb
		},
		{
			.caption = MENU_STR_RO("DOWNLOAD MODE"),
//			.next = (struct menu_entry*)&download_menu,
			.entry_cb = dl_menu_set_cb
		},
		{
			.caption = MENU_STR_RO("CONFIGURATION"),
			.next = (struct menu_entry*)&config_menu
		},
		{
			.caption = MENU_STR_RO("GAMERTAGS"),
			.next = (struct menu_entry*)&gamertag_menu
		},
		{
			.caption = MENU_STR_RO("ABOUT"),
			.next = (struct menu_entry*)&about_menu
		}
	} MENU_ITEM_ENTRY_END
};

