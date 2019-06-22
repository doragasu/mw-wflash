#include <string.h>
#include "menu_gtag.h"
#include "menu_txt.h"
#include "../menu_imp/menu.h"
#include "../menu_imp/menu_msg.h"
#include "../mw/megawifi.h"
#include "../mpool.h"
#include "../util.h"

enum {
	MENU_GTE_NICKNAME = 0,
	MENU_GTE_NICKNAME_DATA,
	MENU_GTE_EMPTY1,
	MENU_GTE_SECRET,
	MENU_GTE_SECRET_DATA,
	MENU_GTE_EMPTY2,
	MENU_GTE_TAGLINE,
	MENU_GTE_TAGLINE_DATA,
	MENU_GTE_EMPTY3,
	MENU_GTE_SAVE
};

static const struct menu_entry menu_gte_nickname_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("NICKNAME"),
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Enter your user name:"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = MW_GT_NICKNAME_MAX - 1
	}
};

static const struct menu_entry menu_gte_secret_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("PASSWORD"),
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Enter your password:"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = MW_GT_SECURITY_MAX - 1
	}
};

static const struct menu_entry menu_gte_tagline_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("TAGLINE"),
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Enter a cool tagline:"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = MW_GT_TAGLINE_MAX - 1
	}
};

/// Fill slot names with SSIDs
static int gte_menu_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	uint8_t slot = instance->prev->sel_item;
	struct mw_gamertag *gamertag;

	gamertag = mw_gamertag_get(slot);
	menu_str_replace(&item[MENU_GTE_NICKNAME_DATA].caption,
			gamertag->nickname);
	menu_str_replace(&item[MENU_GTE_SECRET_DATA].caption,
			gamertag->security);
	menu_str_replace(&item[MENU_GTE_TAGLINE_DATA].caption,
			gamertag->tagline);

	return 0;
}

static int gte_menu_save(struct menu_entry_instance *instance)
{
	struct mw_gamertag gamertag;
	struct menu_item *item = instance->entry->item_entry->item;
	uint8_t slot = instance->prev->sel_item;
	enum mw_err err;

	strcpy(gamertag.nickname, item[MENU_GTE_NICKNAME_DATA].caption.str);
	strcpy(gamertag.security, item[MENU_GTE_SECRET_DATA].caption.str);
	strcpy(gamertag.tagline, item[MENU_GTE_TAGLINE_DATA].caption.str);

	err = mw_gamertag_set(slot, &gamertag);
	if (err != MW_ERR_NONE) {
		menu_msg("ERROR", "Failed to save configuration!", 0, 60 * 5);
		return 1;
	} else {
		menu_msg("DONE", "Gamertag updated", 0, 60 * 5);
	}

	return 0;
}

const struct menu_entry gamertag_entry_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = 4,
	.title = MENU_STR_RO("GAMERTAG"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = gte_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(10, 2, MENU_H_ALIGN_LEFT) {
		{
			.caption = MENU_STR_RO("NICKNAME:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(MW_GT_NICKNAME_MAX),
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&menu_gte_nickname_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("PASSWORD:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(MW_GT_SECURITY_MAX),
			.secure = TRUE,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&menu_gte_secret_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("TAG LINE:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_EMPTY(MW_GT_TAGLINE_MAX),
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&menu_gte_tagline_osk
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("SAVE!"),
			.entry_cb = gte_menu_save,
		},
	} MENU_ITEM_ENTRY_END
};

/// Fill slot names with SSIDs
static int gamertag_menu_enter_cb(struct menu_entry_instance *instance)
{
	int i;
	struct mw_gamertag *gamertag;
	struct menu_item *item = instance->entry->item_entry->item;

	for (i = 0; i < MW_NUM_CFG_SLOTS; i++) {
		if ((gamertag = mw_gamertag_get(i))) {
			menu_str_append(&item[i].caption, gamertag->nickname);
		}
	}

	return 0;
}

const struct menu_entry gamertag_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("GAMERTAGS"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = gamertag_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(3, 3, MENU_H_ALIGN_LEFT) {
		{
			.caption = MENU_STR_RW("1: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&gamertag_entry_menu
		},
		{
			.caption = MENU_STR_RW("2: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&gamertag_entry_menu
		},
		{
			.caption = MENU_STR_RW("3: ", 36),
			.offset = 3,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&gamertag_entry_menu
		}
	} MENU_ITEM_ENTRY_END
};


