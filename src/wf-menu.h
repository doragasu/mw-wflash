#ifndef _WF_MENU_H_
#define _WF_MENU_H_

#include "menu.h"
#include "util.h"

#define MENU_STR(string)	{string, sizeof(string) - 1}

/// Helper to fill entry data in MenuEntry structure
#define MENU_ENTRY_NUMS(numItems, spacing)	(numItems),(spacing), \
	MENU_ITEM_NLINES/(spacing),(numItems)/(MENU_ITEM_NLINES/(spacing))

const MenuItem rootItem[] = { {
		MENU_STR("START"),
		NULL,
		{1, 1}
	}, {
		MENU_STR("CONFIGURATION"),
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

#endif

