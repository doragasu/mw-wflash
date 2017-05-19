#include "wf-menu.h"
#include "util.h"
#include "vdp.h"
#include "mw/megawifi.h"
#include <string.h>

/// Maximum number of dynamic menu items
#define WF_MENU_MAX_DYN_ITEMS 		20

/// Average string length, for dynamic strings
#define WF_MENU_AVG_STR_LEN 		24

/// Macro to compute the number of items of a MenuItem variable
#define MENU_NITEMS(menuItems)	(sizeof(menuItems)/sizeof(MenuItem))

/// Macro to fill on MenuEntry structures: nItems, spacing, entPerPage, pages
#define MENU_ENTRY_NUMS(numItems, spacing)	(numItems),(spacing), \
	MENU_ITEM_NLINES/(spacing),(numItems)/(MENU_ITEM_NLINES/(spacing))

/// Macro to fill on MenuEntry structures:
/// rootItem, nItems, spacing, enPerPage, pages
#define MENU_ENTRY_ITEM(item, spacing)	\
	(item), MENU_ENTRY_NUMS(MENU_NITEMS(item),(spacing))

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
const char strEmptyText[] = "<EMPTY>";
const char strSsid[] = "SSID:    ";
const char strIp[] =   "IP:      ";
const char strMask[] = "MASK:    ";
const char strGw[] =   "GATEWAY: ";
const char strDns1[] = "DNS1:    ";
const char strDns2[] = "DNS2:    ";
const char strEdit[] = "EDIT";
const char strAct[] =  "SET AS ACTIVE";

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
	MenuIpValidate,
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

const MenuItem confItem[] = { {
	MENU_STR("String input test"),
		&editTest,
		NULL,
		{1, 0}
	},{
	MENU_STR("IP entry test"),
		&ipTest,
		NULL,
		{1, 0}
	},{
	MENU_STR("Numeric entry test"),
		&numTest,
		NULL,
		{1, 0}
	},{
	MENU_STR("Unused"),
		NULL,
		NULL,
		{1, 1}
	}
};

/// Pool for dynamically created strings
static char dynPool[WF_MENU_MAX_DYN_ITEMS * WF_MENU_AVG_STR_LEN];
/// Pool for dynamically created items
static MenuItem dynItems[WF_MENU_MAX_DYN_ITEMS];

uint16_t MenuStrCpy(char dst[], const char src[], uint16_t maxLen) {
	uint16_t i;

	// If maxLen is 0, no maxLen specified, so use the maximum possible value
	if (!maxLen) maxLen--;

	for (i = 0; (i < maxLen) && (src[i] != '\0'); i++)
		dst[i] = src[i];

	return i;
}

int MenuConfDataEntryCb(void *m) {
	const Menu *me = (Menu*)m;
	char *ssid;
	MwIpCfg *ip;
	uint8_t offset;
	uint8_t confItem;
	uint8_t i;
	uint8_t strLen;
	uint8_t error = FALSE;

	// It is assumed that the configuration item is stored on previous level
	// data, as it corresponds to the selected option.
	confItem = me->selItem[me->level - 1];
	i = 0;
	// Get the SSID
	if ((MwApCfgGet(confItem, &ssid, NULL) != MW_OK) || (*ssid == '\0')) {
		// Configuration request failed, fill all items as empty
		// SSID
		strLen  = MenuStrCpy(dynPool, strSsid, 0);
		strLen += MenuStrCpy(dynPool + strLen, strEmptyText, 0);
		dynItems[i].caption.string = dynPool;
		dynItems[i++].caption.length = strLen;
		offset = strLen + 1;
		error = TRUE;
	} else {
		strLen  = MenuStrCpy(dynPool, strSsid, 0);
		strLen += MenuStrCpy(dynPool + strLen, ssid, MW_SSID_MAXLEN);
		// If SSID is 32 bytes, it might not be null terminated
		if (dynPool[strLen] != '\0') {
			strLen++;
			dynPool[strLen] = '\0';
		}
		dynItems[i].caption.string = dynPool;
		dynItems[i++].caption.length = strLen;
		offset = strLen + 1;
	}
	if (error || (MwIpCfgGet(confItem, &ip) != MW_OK))
		// IP
		strLen  = MenuStrCpy(dynPool + offset, strIp, 0);
		strLen += MenuStrCpy(dynPool + offset + strLen, strEmptyText, 0);
		dynItems[i].caption.string = dynPool + offset;
		dynItems[i++].caption.length = strLen;
		offset += strLen + 1;
		// MASK
		*dynPool = '\0';
		dynItems[i].caption.string = dynPool + offset;
		dynItems[i++].caption.length = 0;
		// GATEWAY
		dynItems[i].caption.string = dynPool + offset;
		dynItems[i++].caption.length = 0;
		// DNS1
		dynItems[i].caption.string = dynPool + offset;
		dynItems[i++].caption.length = 0;
		// DNS2
		dynItems[i].caption.string = dynPool + offset;
		dynItems[i++].caption.length = 0;
	} else {
		// Request IP configuration
		
	}
	// [BLANK]
	// EDIT
	// SET AS DEFAULT
	
	return 0;
}

// Menu structure:
// SSID:    <data>
// IP:      <MANUAL,AUTO>
// MASK:    <DATA>
// GATEWAY: <DATA>
// DNS1:    <DATA>
// DNS2:    <DATA>
// [BLANK]
// EDIT
// SET AS DEFAULT
const MenuEntry confEntryData = {
	MENU_TYPE_ITEM,					// Menu type
	8,								// Margin
	MENU_STR("CONNECTION"),			// Title
	MENU_STR(stdContext),			// Left context
	MenuConfDataEntryCb,			// cbEntry
	NULL,							// cbExit
	.item = {
		dynItems,					// item
		9,							// nItems
		2,							// spacing
		9,							// entPerPage
		0,							// pages
		{MENU_H_ALIGN_LEFT}			// align
	}
};

/// Fills confEntry items
/// \todo Add a marker to default configuration
int MenuConfEntryCb(void* m) {
	int8_t i;
	uint16_t offset;
	uint16_t copied;
	char *ssid;
	UNUSED_PARAM(m);

	// Load MegaWiFi configurations and fill entries with available SSIDs
	for (i = 0, offset = 0; i < 3; i++) {
		dynItems[i].caption.string = dynPool + offset;
		dynItems[i].cb = NULL;
		dynItems[i].next = &confEntryData;
		dynItems[i].flags.selectable = 1;
		dynItems[i].flags.alt_color = 0;
		dynPool[offset++] = '1' + i;
		dynPool[offset++] = ':';
		dynPool[offset++] = ' ';

		// Obtains configurations and fill SSIDs
		if ((MwApCfgGet(i, &ssid, NULL) != MW_OK) || (*ssid == '\0')) {
			strcpy(dynPool + offset, "<EMPTY>");
			dynItems[i].caption.length = 10;
			offset += 8;
		} else {
			copied = MenuStrCpy(dynPool + offset, ssid, 32);
			if (copied == 32) *(dynPool + offset + 32) = '\0';
			dynItems[i].caption.length = copied;
			offset += copied + 1;
		}
	}

	return 0;
}

const MenuEntry confEntry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("CONFIGURATION"),		// Title
	MENU_STR(stdContext),			// Left context
	MenuConfEntryCb,				// cbEntry
	NULL,							// cbExit
	.item = {
		dynItems,					// item
		3,							// nItems
		3,							// spacing
		3,							// entPerPage
		0,							// pages
		{MENU_H_ALIGN_LEFT}			// align
	}
};

/// Root menu items
const MenuItem rootItem[] = { {
		MENU_STR("START"),			///< Caption
		NULL,						///< Next
		NULL,						///< Callback
		{1, 1}						///< Selectable, alt_color
	}, {
		MENU_STR("CONFIGURATION"),
		&confEntry,
		NULL,
		{1, 0}
	}
};

/// Root menu
const MenuEntry rootMenu = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("WFLASH BOOTLOADER"),	// Title
	MENU_STR(stdContext),			// Left context
	NULL,							// cbEntry
	NULL,							// cbExit
	.item = {
		// rootItem, nItems, spacing, enPerPage, pages
		MENU_ENTRY_ITEM(rootItem, 2),
		{MENU_H_ALIGN_CENTER}		// align
	}
};

void WfMenuInit(MenuString statStr) {
	MenuInit(&rootMenu, statStr);
}

