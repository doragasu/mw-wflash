#include "menu_dl.h"
#include "menu_txt.h"
#include "../mw/megawifi.h"
#include "../menu_imp/menu.h"

/// Empty menu, data will be manually written on the screen
const struct menu_entry download_start_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("DOWNLOAD MODE"),
	.left_context = MENU_STR_RO("[B]ack"),
	.enter_cb = download_mode_menu_cb,
	.item_entry = MENU_ITEM_ENTRY(3, 2, MENU_H_ALIGN_CENTER) {
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
		if (MW_ERR_NONE == mw_ap_cfg_get(i, &ssid, NULL)) {
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
	.item_entry = MENU_ITEM_ENTRY(3, 2, MENU_H_ALIGN_LEFT) {
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

