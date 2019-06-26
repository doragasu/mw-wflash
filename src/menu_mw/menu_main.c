#include <string.h>
#include <stdlib.h>
#include "menu_main.h"
#include "menu_net.h"
#include "menu_txt.h"
#include "menu_dl.h"
#include "menu_gtag.h"
#include "../sysfsm.h"
#include "../menu_imp/menu.h"
#include "../menu_imp/menu_msg.h"
#include "../mw/megawifi.h"

#define NTP_SERV_MAXLEN 	32

/// Item offsets for the NTP configuration menu
enum {
	MENU_NTP_SERV1 = 1,
	MENU_NTP_SERV2 = 2,
	MENU_NTP_SERV3 = 3,
	MENU_NTP_TIMEZONE = 6,
	MENU_NTP_UPDATE_INTERVAL = 9,
	MENU_NTP_DAYLIGHT_SAVING = 11
};


static int menu_tz_validate(struct menu_entry_instance *instance)
{
	struct menu_str *text = &instance->entry->osk_entry->tmp;
	char *endptr = NULL;
	long number = strtol(text->str, &endptr, 10);
	int err = 0;

	if ('\0' == *text->str) {
		err = 1;
	}
	if (*endptr != '\0') {
		err = 1;
	}
	if (!err) {
		err = !IN_RANGE(number, -11, 13);
	}
	if (err) {
		menu_msg("INVALID TIMEZONE", "Invalid timezone "
				"(-11 to 13)", 0, 5 * 60);
	}

	return err;
}

const struct menu_entry menu_ntp_tz_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.action_cb = menu_tz_validate,
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO(	"Time zone (-11 to 13):"),
		.osk_type = MENU_TYPE_OSK_NUMERIC_NEG,
		.line_len = 3
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

const struct menu_entry menu_ntp_interval_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.action_cb = menu_non_empty_validate,
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Update interval (15+ seconds):"),
		.osk_type = MENU_TYPE_OSK_NUMERIC,
		.line_len = 4
	}
};

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



static int menu_on_off_status(const struct menu_item *item)
{
	int offset = item->offset + 1;

	return item->caption.str[offset] == 'N'?1:0;
}

static int menu_ntp_save(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	char *servers[3];
	uint16_t up_delay;
	int8_t timezone;
	int8_t dst;

	for (int i = 0; i < 3; i++) {
		servers[i] = item[i + 1].caption.str;
	}
	up_delay = atoi(item[MENU_NTP_UPDATE_INTERVAL].caption.str);
	timezone = atoi(item[MENU_NTP_TIMEZONE].caption.str);
	dst = menu_on_off_status(&item[MENU_NTP_DAYLIGHT_SAVING]);

	if (MW_ERR_NONE != mw_sntp_cfg_set((const char**)servers, up_delay,
				timezone, dst)) {
		menu_msg("ERROR", "Failed to save configuration!", 0, 5 * 60);
		return 1;
	}

	menu_back(1);

	return 0;
}

static void menu_on_off_draw(struct menu_item *item, int8_t on)
{
	if (on) {
		item->caption.str[item->offset + 1] = 'N';
		item->caption.length = item->offset + 2;
	} else {
		item->caption.str[item->offset + 1] = 'F';
		item->caption.str[item->offset + 2] = 'F';
		item->caption.length = item->offset + 3;
	}
}

static int menu_on_off_toggle(struct menu_entry_instance *instance)
{
	struct menu_item *item = &instance->entry->item_entry->
		item[instance->sel_item];

	menu_on_off_draw(item, 'F' == item->caption.str[item->offset + 1]);
	menu_item_draw(MENU_PLACE_CENTER);

	return 0;
}

static int menu_ntp_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	char *server[3] = {0};
	uint16_t update_delay;
	int8_t timezone;
	int8_t dst;
	enum mw_err err;
	int i;

	err = mw_sntp_cfg_get(server, &update_delay, &timezone, &dst);
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
	item[MENU_NTP_UPDATE_INTERVAL].caption.length = uint16_to_str(update_delay,
			item[MENU_NTP_UPDATE_INTERVAL].caption.str);
	item[MENU_NTP_TIMEZONE].caption.length = int8_to_str(timezone,
			item[MENU_NTP_TIMEZONE].caption.str);
	menu_on_off_draw(&item[MENU_NTP_DAYLIGHT_SAVING], dst);

	return 0;
}

const struct menu_entry ntp_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = 8,
	.title = MENU_STR_RO("TIME CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = menu_ntp_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(16, 1, MENU_H_ALIGN_LEFT) {
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
			.caption = MENU_STR_RO("Time zone:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(3),
			.next = (struct menu_entry*)&menu_ntp_tz_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("Update interval:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(4),
			.next = (struct menu_entry*)&menu_ntp_interval_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RW("Daylight saving: OXX", 20),
			.offset = 17,
			.entry_cb = menu_on_off_toggle
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
	.item_entry = MENU_ITEM_ENTRY(5, 2, MENU_H_ALIGN_CENTER) {
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
	.item_entry = MENU_ITEM_ENTRY(2, 3, MENU_H_ALIGN_CENTER) {
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
	.item_entry = MENU_ITEM_ENTRY(5, 2, MENU_H_ALIGN_LEFT) {
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

static int main_menu_enter_cb(struct menu_entry_instance *instance)
{
	// Enable START option only if a flashed game is detected
	if (SF_ENTRY_POINT_ADDR && SF_ENTRY_POINT_ADDR != 0x20202020) {
		instance->entry->item_entry->item[0].not_selectable = FALSE;
		instance->entry->item_entry->item[0].alt_color = FALSE;
	}

	return 0;
}

static int game_boot_cb(struct menu_entry_instance *instance)
{
	UNUSED_PARAM(instance);

	sf_boot(SF_ENTRY_POINT_ADDR);

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

const struct menu_entry main_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("MegaWiFi bootloader by doragasu"),
	.left_context = MENU_STR_RO("Select an option"),
	.enter_cb = main_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(4, 4, MENU_H_ALIGN_CENTER) {
		{
			.caption = MENU_STR_RO("START!"),
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
		}
	} MENU_ITEM_ENTRY_END
};

