#include "menu_dl.h"
#include "../menu_imp/menu.h"

/// Empty menu, data will be manually written on the screen
const struct menu_entry download_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("DOWNLOAD MODE"),
	.left_context = MENU_STR_RO("[B]ack"),
	.action_cb = download_mode_cb,
	.item_entry = MENU_ITEM_ENTRY(3, 2, MENU_H_ALIGN_CENTER) {
		{
			.caption = MENU_STR_RW("Associating to access point...", 38),
			.not_selectable = TRUE
		},
		{
			.caption = MENU_STR_NULL
		},
		{
			.caption = MENU_STR_EMPTY(14),
			.not_selectable = TRUE,
			.alt_color = TRUE
		}
	} MENU_ITEM_ENTRY_END
};

