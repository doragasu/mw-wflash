#include "wf-menu.h"
#include "util.h"

#define MENU_STR(string)	{string, sizeof(string) - 1}

/// Helper to fill entry data in MenuEntry structure
#define MENU_ENTRY_NUMS(numItems, spacing)	(numItems),(spacing), \
	MENU_ITEM_NLINES/(spacing),(numItems)/(MENU_ITEM_NLINES/(spacing))

/// Status string buffer
static char statBuf[16];
/// Status string
static MenuString statStr = {statBuf, 0};

const MenuItem rootItem[] = { {
		MENU_STR("START"),			///< Caption
		NULL,						///< Callback
		NULL,						///< Next
		{1, 1}						///< Selectable, Enabled
	}, {
		MENU_STR("CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}
};
#define MENU_ROOT_ITEMS	2
//#define MENU_ROOT_ITEMS	(sizeof(rootItem)/sizeof(MenuItem))

const MenuEntry rootMenu = {
	MENU_STR("WFLASH BOOTLOADER"),
	MENU_STR("LEFT CONTEXT"),
	rootItem,
	NULL,
	NULL,
	MENU_ENTRY_NUMS(MENU_ROOT_ITEMS, 2),
	1,
	{MENU_H_ALIGN_CENTER}
};

void WfMenuInit(MenuString statStr) {
	MenuInit(&rootMenu, statStr);
}

