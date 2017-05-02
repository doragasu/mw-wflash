#include "wf-menu.h"
#include "util.h"

#define MENU_NITEMS(menuItems)	(sizeof(menuItems)/sizeof(MenuItem))

/// Helper to fill entry data in MenuEntry structure
/// (nItems, spacing, entPerPage, pages)
#define MENU_ENTRY_NUMS(numItems, spacing)	(numItems),(spacing), \
	MENU_ITEM_NLINES/(spacing),(numItems)/(MENU_ITEM_NLINES/(spacing))

const char stdContext[] = "[A]ccept, [B]ack";

char editableStr1[32];	// QWERTY string
char editableStr2[10];	// Number
char editableStr3[15];	// IP

MenuItem editable[] = { {
		{editableStr1, 0}, 
		NULL,
		NULL,
		{1, MENU_EDITABLE_QWERTY, 1}
	},{
		{editableStr2, 0},
		NULL,
		NULL,
		{1, MENU_EDITABLE_NUMERIC, 1}
	},{
		{editableStr3, 0},
		NULL,
		NULL,
		{1, MENU_EDITABLE_IP, 1}
	}
};

const MenuEntry editTest = {
	MENU_STR("EDIT MENU TEST"),		// Title
	MENU_STR(stdContext),			// Left context
	editable,						// Item
	NULL,							// cbEntry
	NULL,							// cbExit
	// nItems, spacing, entPerPage, pages
	MENU_ENTRY_NUMS(MENU_NITEMS(editable), 3),
	5,								// margin
	{MENU_H_ALIGN_LEFT}				// align
};

const MenuItem lA1Item[] = { {
	MENU_STR("LA1 TEST1"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	},{
	MENU_STR("LA1 TEST2"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	},{
	MENU_STR("LA1 TEST3"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	},{
	MENU_STR("LA1 TEST4"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}
};

#define MENU_LA1_ITEMS	(sizeof(lA1Item)/sizeof(MenuItem))

const MenuEntry lA1Entry = {
	MENU_STR("LEVEL A-1 MENU"),		// Title
	MENU_STR(stdContext),			// Left context
	lA1Item,						// Item
	NULL,							// cbEntry
	NULL,							// cbExit
	// nItems, spacing, entPerPage, pages
	MENU_ENTRY_NUMS(MENU_NITEMS(lA1Item), 3),
	1,								// margin
	{MENU_H_ALIGN_CENTER}			// align
};

const MenuItem rootItem[] = { {
		MENU_STR("START"),			///< Caption
		(MenuEntry*)&lA1Entry,		///< Next
		NULL,						///< Callback
		{1, MENU_EDITABLE_NO, 1}						///< Selectable, editable, Enabled
	}, {
		MENU_STR("CONFIGURATION"),
		(MenuEntry*)&editTest,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("3CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("4CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("5CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("6CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("7CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("8CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("9CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("10CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("11CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("12CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("13CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("14CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}, {
		MENU_STR("15CONFIGURATION"),
		NULL,
		NULL,
		{1, MENU_EDITABLE_NO, 1}
	}
};
#define MENU_ROOT_ITEMS	(sizeof(rootItem)/sizeof(MenuItem))

const MenuEntry rootMenu = {
	MENU_STR("WFLASH BOOTLOADER"),	// Title
	MENU_STR(stdContext),			// Left context
	rootItem,						// Item
	NULL,							// cbEntry
	NULL,							// cbExit
	// nItems, spacing, entPerPage, pages
	MENU_ENTRY_NUMS(MENU_NITEMS(rootItem), 2),
	1,								// margin
	{MENU_H_ALIGN_CENTER}			// align
};

void WfMenuInit(MenuString statStr) {
	MenuInit(&rootMenu, statStr);
}

