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
#define TIMEZONE_MAXLEN 	32

/// Item offsets for the NTP configuration menu
enum {
	MENU_NTP_SERV1 = 1,
	MENU_NTP_SERV2 = 2,
	MENU_NTP_SERV3 = 3,
	MENU_NTP_TIMEZONE = 6
};

/// Advanced configuration items
enum {
	MENU_ADV_TIME_SERV_CAPTION = 0,
	MENU_ADV_TIME_SERV,
	MENU_ADV_EMTPY1,
	MENU_ADV_WIFI_CFG_CAPTION,
	MENU_ADV_WIFI_QOS,
	MENU_ADV_WIFI_AMSDU,
	MENU_ADV_WIFI_AMPDU,
	MENU_ADV_EMTPY2,
	MENU_ADV_WIFI_SAVE,
	MENU_ADV_N_ENTRIES
};

static void menu_on_off_set(struct menu_item *item, int value)
{
	if (value) {
		item->caption.str[item->offset] = 'N';
		item->caption.str[item->offset + 1] = '\0';
		item->caption.length = item->offset + 1;
	} else {
		item->caption.str[item->offset] = 'F';
		item->caption.str[item->offset + 1] = 'F';
		item->caption.str[item->offset + 2] = '\0';
		item->caption.length = item->offset + 2;
	}
}

static void menu_reboot(void)
{
	extern uint32_t dirty_dw;

	// Make sure we boot the loader again
	dirty_dw = MAGIC_WIFI_CONFIG;
	sf_boot(GL_BOOTLOADER_ADDR, TRUE);
}

static int menu_on_off_get(struct menu_item *item)
{
	return 'N' == item->caption.str[item->offset];
}

static int menu_on_off_toggle(struct menu_entry_instance *instance)
{
	struct menu_item *item =
		&instance->entry->item_entry->item[instance->sel_item];

	menu_on_off_set(item, !menu_on_off_get(item));

	menu_item_draw(MENU_PLACE_CENTER);

	return 0;
}

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

static int menu_non_empty_validate(struct menu_entry_instance *instance)
{
	int err = 0;

	if ('\0' == *instance->entry->osk_entry->tmp.str) {
		menu_msg("INVALID INPUT", "Input cannot be empty",
				0, 5 * 60);
		err = 1;
	}

	return err;
}

static int menu_domain_validate(struct menu_entry_instance *instance)
{
	struct menu_str *text = &instance->entry->osk_entry->tmp;
	int err = 0;

	if (menu_non_empty_validate(instance) || !strchr(text->str, '.')) {
		menu_msg("INVALID SERVER", "Please enter a DNS server",
				0, 5 * 60);
		err = 1;
	}

	return err;
}

const struct menu_entry menu_ntp_tz_osk ROM_DATA(menu_ntp_tz_osk) = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.action_cb = menu_tz_validate,
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO(	"Timezone (e.g. GMT-1, UTC, CET):"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = TIMEZONE_MAXLEN
	}
};

const struct menu_entry menu_ntp_serv_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.action_cb = menu_domain_validate,
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
	const char *tz_str;


	for (int i = 0; i < 3; i++) {
		servers[i] = item[i + MENU_NTP_SERV1].caption.str;
	}
	tz_str = item[MENU_NTP_TIMEZONE].caption.str;
	if (MW_ERR_NONE != mw_sntp_cfg_set(tz_str, servers) ||
			MW_ERR_NONE != mw_cfg_save()) {
		menu_msg("ERROR", "Failed to save configuration!", 0, 5 * 60);
		return 1;
	}

	menu_back(1);

	return 0;
}

static int menu_ntp_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	char *tz_str = NULL;
	char *server[3] = {0};
	enum mw_err err;
	int i;

	err = mw_sntp_cfg_get(&tz_str, server);
	if (err) {
		menu_msg("ERROR", "Failed to get time configuration", 0, 180);
		return 1;
	}

	menu_str_replace(&item[MENU_NTP_TIMEZONE].caption, tz_str);
	for (i = 0; i < 3 && server[i]; i++) {
		menu_str_replace(&item[MENU_NTP_SERV1 + i].caption, server[i]);
	}

	return 0;
}

const struct menu_entry ntp_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = 8,
	.title = MENU_STR_RO("TIME CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = menu_ntp_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(9, 2, MENU_H_ALIGN_LEFT, 1) {
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
			.caption = MENU_STR_RO("Timezone:"),
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
			.caption = MENU_STR_RO("SAVE!"),
			.entry_cb = menu_ntp_save
		}
	} MENU_ITEM_ENTRY_END
};

static int root_menu_cb(struct menu_entry_instance *instance)
{
	UNUSED_PARAM(instance);

	menu_back(MENU_BACK_ALL);

	return 0;
}

static int default_cfg_cb(struct menu_entry_instance *instance)
{
	enum mw_err err;
	UNUSED_PARAM(instance);

	err = mw_factory_settings();
	if (MW_ERR_NONE == err) {
		menu_msg("SUCCESS!", "Rebooting...", 0, 0);
		mw_sleep(180);
		menu_reboot();
	} else {
		menu_msg("ERROR!", "Factory restore failed", 0, 0);
	}

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

static int server_save_cb(struct menu_entry_instance *instance)
{
	struct menu_str *text = &instance->entry->osk_entry->tmp;
	int err = 0;

	if (MW_ERR_NONE != mw_def_server_set(text->str) ||
			MW_ERR_NONE != mw_cfg_save()) {
		menu_msg("SERVER CONFIGURATION", "Server config failed!", 0, 300);
		err = 1;
	}

	return err;
}

const struct menu_entry menu_def_serv_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("DEFAULT MEGAWIFI SERVER"),
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.action_cb = server_save_cb,
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("WARNING: change at your own risk!"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = 38
	}
};

static int config_advanced_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	char *server = mw_def_server_get();

	if (server) {
		menu_str_replace(&item[MENU_ADV_TIME_SERV].caption, server);
	}

	struct mw_wifi_adv_cfg *wifi = mw_wifi_adv_cfg_get();
	if (wifi) {
		menu_on_off_set(&item[MENU_ADV_WIFI_QOS], wifi->qos_enable);
		menu_on_off_set(&item[MENU_ADV_WIFI_AMSDU],
				wifi->amsdu_rx_enable);
		menu_on_off_set(&item[MENU_ADV_WIFI_AMPDU],
				wifi->ampdu_rx_enable);
	}

	return 0;
}

static int menu_adv_cfg_save(struct menu_entry_instance *instance)
{
	struct mw_wifi_adv_cfg *wifi_ptr = mw_wifi_adv_cfg_get();
	struct mw_wifi_adv_cfg wifi;
	struct menu_item *item = instance->entry->item_entry->item;

	if (!wifi_ptr) {
		goto err;
	}
	wifi = *wifi_ptr;

	// Configure WiFi according to QoS, AMPDU, AMSDU options
	wifi.qos_enable = menu_on_off_get(&item[MENU_ADV_WIFI_QOS]);
	// NOTE: default values taken from esp_wifi.h
	if (menu_on_off_get(&item[MENU_ADV_WIFI_AMPDU])) {
		wifi.ampdu_rx_enable = 1;
		wifi.rx_ba_win = 6;	// Default BA windows size
		wifi.rx_max_single_pkt_len = 1600;
	} else {
		wifi.ampdu_rx_enable = 0;
		wifi.rx_ba_win = 0;
		wifi.rx_max_single_pkt_len = 1600 - 524;
	}
	if (menu_on_off_get(&item[MENU_ADV_WIFI_AMSDU])) {
		wifi.amsdu_rx_enable = 1;
		wifi.rx_max_single_pkt_len = 3000;
	} else {
		// rx_max_single_pkt_len as defined by AMPDU option
		wifi.amsdu_rx_enable = 0;
	}

	if (mw_wifi_adv_cfg_set(&wifi) || mw_cfg_save()) {
		goto err;
	}

	menu_msg("SUCCESS!", "Rebooting...", 0, 0);
	mw_sleep(180);
	menu_reboot();

	return 0;

err:
	menu_msg("CONFIGURATION ERROR", "Failed to set config", 0, 0);
	return 1;
}

const struct menu_entry advanced_menu ROM_DATA(advanced_menu) = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("ADVANCED CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = config_advanced_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(MENU_ADV_N_ENTRIES, 2, MENU_H_ALIGN_CENTER, 1) {
		{
			.caption = MENU_STR_RO("DEFAULT SERVER:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(40),
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&menu_def_serv_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("ADVANCED WIFI CONFIGURATION"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_RW("QoS: OFF", 10),
			.offset = 6,
			.entry_cb = menu_on_off_toggle
		},
		{
			.caption = MENU_STR_RW("AMSDU: OFF", 12),
			.offset = 8,
			.entry_cb = menu_on_off_toggle
		},
		{
			.caption = MENU_STR_RW("AMPDU: OFF", 12),
			.offset = 8,
			.entry_cb = menu_on_off_toggle
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("SAVE!"),
			.entry_cb = menu_adv_cfg_save
		}
	} MENU_ITEM_ENTRY_END
};

/// Fill slot names with SSIDs
static int config_menu_enter_cb(struct menu_entry_instance *instance)
{
	int i;
	char *ssid;

	struct menu_item *item = instance->entry->item_entry->item;

	for (i = 0; i < MW_NUM_CFG_SLOTS; i++) {
		if (MW_ERR_NONE == mw_ap_cfg_get(i, &ssid, NULL, NULL) &&
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
	.item_entry = MENU_ITEM_ENTRY(8, 2, MENU_H_ALIGN_LEFT, 1) {
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
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("TIME CONFIGURATION"),
			.next = (struct menu_entry*)&ntp_menu
		},
		{
			.caption = MENU_STR_RO("ADVANCED"),
			.next = (struct menu_entry*)&advanced_menu
		},
		{
			.caption = MENU_STR_RO("RESET TO DEFAULTS"),
			.next = (struct menu_entry*)&defaults_menu
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
		if (MW_ERR_NONE == mw_ap_cfg_get(i, &ssid, NULL, NULL)) {
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

const struct menu_entry about_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("ABOUT"),
	.left_context = MENU_STR_RO(ITEM_BACK_STR),
	.enter_cb = about_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(15, 1, MENU_H_ALIGN_CENTER, 1) {
		{
			.caption = MENU_STR_RW("MegaWiFi loader "
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
			.caption = MENU_STR_RO("Logo: @Manu_Segura_"),
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

