#include "wf-menu.h"
#include "util.h"

#define MENU_STR(string)	{string, sizeof(string) - 1}

/// Helper to fill entry data in MenuEntry structure
/// (nItems, spacing, entPerPage, pages)
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
//#define MENU_ROOT_ITEMS	1
#define MENU_ROOT_ITEMS	2
//#define MENU_ROOT_ITEMS	(sizeof(rootItem)/sizeof(MenuItem))

const MenuEntry rootMenu = {
	MENU_STR("WFLASH BOOTLOADER"),	// Title
	MENU_STR("LEFT CONTEXT"),		// Left context
	rootItem,						// Item
	NULL,							// cbEntry
	NULL,							// cbExit
	// nItems, spacing, entPerPage, pages
	MENU_ENTRY_NUMS(MENU_ROOT_ITEMS, 2), // nItems, spacing
	1,								// margin
	{MENU_H_ALIGN_CENTER}			// align
};

void WfMenuInit(MenuString statStr) {
	MenuInit(&rootMenu, statStr);
}

