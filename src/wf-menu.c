#include "wf-menu.h"
#include "util.h"
#include "vdp.h"

#define MENU_NITEMS(menuItems)	(sizeof(menuItems)/sizeof(MenuItem))

/// Helper to fill entry data in MenuEntry structure
/// (nItems, spacing, entPerPage, pages)
#define MENU_ENTRY_NUMS(numItems, spacing)	(numItems),(spacing), \
	MENU_ITEM_NLINES/(spacing),(numItems)/(MENU_ITEM_NLINES/(spacing))

int TestCb(void* md) {
	Menu *m = (Menu*)md;
	const MenuEntry* me = m->me[m->level];
	uint8_t i;

	VdpDrawText(VDP_PLANEA_ADDR, 1, 1, MENU_COLOR_OSK_DATA, me->title.length,
			me->title.string);

	for (i = 0; i < 120; i++) VdpVBlankWait();

	return TRUE;
//	return FALSE;
}

const char stdContext[] = "[A]ccept, [B]ack";
const char oskQwertyContext[] = "A-OK, B-Del, C-Caps, S-Done";
const char oskNumIpContext[] = "A-OK, B-Del, S-Done";


char editableIp[16] = "192.168.1.60";
char editableNum[9] = "123456";

const MenuEntry numTest = {
	MENU_TYPE_OSK_NUMERIC,
	1,
	MENU_STR("NUMERIC MENU TEST"),
	MENU_STR(oskNumIpContext),
	NULL,
	TestCb,
	.keyb = {
		MENU_STR("Enter number:"),
		{editableNum, 6},
		8,
		8
	}
};

const MenuEntry ipTest = {
	MENU_TYPE_OSK_IPV4,
	1,
	MENU_STR("IP MENU TEST"),
	MENU_STR(oskNumIpContext),
	NULL,
	TestCb,
	.keyb = {
		MENU_STR("Enter IP address:"),
		{editableIp, 12},
		15,
		15
	}
};

char editableStr[32] = "Edit me!";

const MenuEntry editTest = {
	MENU_TYPE_OSK_QWERTY,			// Item list type
	1,								// Margin
	MENU_STR("EDIT MENU TEST"),		// Title
	MENU_STR(oskQwertyContext),		// Left context
	NULL,							// cbEntry
	NULL,							// cbExit
	.keyb = {
		MENU_STR("Input string test:"),
		{editableStr, 8},
		32,
		32
	}
};

const MenuItem lA1Item[] = { {
	MENU_STR("String input test"),
		&editTest,
		NULL,
		{1, 1}
	},{
	MENU_STR("IP entry test"),
		&ipTest,
		NULL,
		{1, 1}
	},{
	MENU_STR("Numeric entry test"),
		&numTest,
		NULL,
		{1, 1}
	},{
	MENU_STR("Unused"),
		NULL,
		NULL,
		{1, 1}
	}
};

#define MENU_LA1_ITEMS	(sizeof(lA1Item)/sizeof(MenuItem))

const MenuEntry lA1Entry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("LEVEL A-1 MENU"),		// Title
	MENU_STR(stdContext),			// Left context
	NULL,							// cbEntry
	NULL,							// cbExit
	.item = {
		lA1Item,					// Item
		// nItems, spacing, entPerPage, pages
		MENU_ENTRY_NUMS(MENU_NITEMS(lA1Item), 3),
		{MENU_H_ALIGN_CENTER}		// align
	}
};

const MenuItem rootItem[] = { {
		MENU_STR("START"),			///< Caption
		&lA1Entry,					///< Next
		NULL,						///< Callback
		{1, 1}						///< Selectable, Enabled
	}, {
		MENU_STR("CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("3NOT SELECTABLE"),
		(MenuEntry*)&ipTest,
		NULL,
		{0, 1}
	}, {
		MENU_STR("4CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("5NOT SELECTABLE"),
		NULL,
		NULL,
		{0, 1}
	}, {
		MENU_STR("6NOT SELECTABLE"),
		NULL,
		NULL,
		{0, 1}
	}, {
		MENU_STR("7CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("8CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("9CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("10CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("11CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("12CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("13CONFIGURATION"),
		NULL,
		NULL,
		{1, 1}
	}, {
		MENU_STR("14NOT SELECTABLE"),
		NULL,
		NULL,
		{0, 1}
	}, {
		MENU_STR("15NOT SELECTABLE"),
		NULL,
		NULL,
		{0, 1}
	}
};
#define MENU_ROOT_ITEMS	(sizeof(rootItem)/sizeof(MenuItem))

const MenuEntry rootMenu = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("WFLASH BOOTLOADER"),	// Title
	MENU_STR(stdContext),			// Left context
	NULL,							// cbEntry
	NULL,							// cbExit
	.item = {
		rootItem,					// Item
		// nItems, spacing, entPerPage, pages
		MENU_ENTRY_NUMS(MENU_NITEMS(rootItem), 2),
		{MENU_H_ALIGN_CENTER}		// align
	}
};

void WfMenuInit(MenuString statStr) {
	MenuInit(&rootMenu, statStr);
}

