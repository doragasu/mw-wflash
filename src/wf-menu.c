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

/// IP address definition
typedef union {
	uint32_t addr;
	uint8_t byte[4];
} IpAddr;

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

const char security[][4] = {
	"OPEN", "WEP ", "WPA1", "WPA2", "WPA ", "??? "
};

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
const char strScan[] = "SCAN IN PROGRESS, PLEASE WAIT...";
const char strScanFail[] = "SCAN FAILED!";
const char strCfgFail[] = "CONFIGURATION FAILED!";
const char strDhcp[] = "AUTO";
const char strOk[] = "OK";

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

/// Selected network configuration item (from 0 to 2)
static uint8_t selConfig;
/// Pool for dynamically created strings
static char dynPool[WF_MENU_MAX_DYN_ITEMS * WF_MENU_AVG_STR_LEN];
/// Pool for dynamically created items
static MenuItem dynItems[WF_MENU_MAX_DYN_ITEMS];

uint32_t MenuIpStr2Bin(char ip[]) {
	IpAddr addr;
	int i;

	for (i = 0; i < 4; i++) {
		if ((ip = Str2UnsByte(ip, &addr.byte[i])) == NULL) return 0;
		ip++;
	}
	return addr.addr;
}

/// Fills dynItems with network parameters. Does NOT fill callbacks,
/// next entries or flags
int MenuIpConfFill(uint8_t *startItem, uint16_t *offset, 
		uint8_t numConfig) {
	MwIpCfg *ip;
	uint8_t strLen;

	// A return value of 0 means that there is an error.
	if (MwIpCfgGet(numConfig, &ip) != MW_OK) return 1;

	/// \todo Filling data is pretty regular and should be done in a loop
	/// IP
	/// \todo Implement automatic (DHCP) settings
	strLen  = MenuStrCpy(dynPool + (*offset), strIp, 0);
	strLen += MenuBin2IpStr(ip->addr, dynPool + (*offset) + strLen);
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = strLen;
	(*offset) += strLen + 1;
	// MASK
	strLen  = MenuStrCpy(dynPool + (*offset), strMask, 0);
	strLen += MenuBin2IpStr(ip->mask, dynPool + (*offset) + strLen);
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = strLen;
	(*offset) += strLen + 1;
	// GATEWAY
	strLen  = MenuStrCpy(dynPool + (*offset), strGw, 0);
	strLen += MenuBin2IpStr(ip->gateway, dynPool + (*offset) + strLen);
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = strLen;
	(*offset) += strLen + 1;
	// DNS1
	strLen  = MenuStrCpy(dynPool + (*offset), strDns1, 0);
	strLen += MenuBin2IpStr(ip->dns1, dynPool + (*offset) + strLen);
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = strLen;
	(*offset) += strLen + 1;
	// DNS2
	strLen  = MenuStrCpy(dynPool + (*offset), strDns1, 0);
	strLen += MenuBin2IpStr(ip->dns1, dynPool + (*offset) + strLen);
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = strLen;
	(*offset) += strLen + 1;

	return 0;
}

/// Fills dynItems with default network parameters. Does NOT fill callbacks,
/// next entries or flags
uint16_t MenuIpConfFillDhcp(uint8_t *startItem, uint16_t *offset) {
	uint8_t strLen;

	// IP
	strLen  = MenuStrCpy(dynPool + (*offset), strIp, 0);
	strLen += MenuStrCpy(dynPool + (*offset) + strLen, strDhcp, 0);
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = strLen;
	*offset += strLen + 1;
	// MASK
	dynPool[(*offset)] = '\0';
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = 0;
	// GATEWAY
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = 0;
	// DNS1
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = 0;
	// DNS2
	dynItems[(*startItem)].caption.string = dynPool + (*offset);
	dynItems[(*startItem)++].caption.length = 0;

	return 0;
}

int MenuNetConfEntryBackCb(void *m) {
	UNUSED_PARAM(m);

	return 0;
}


int MenuNetConfEntryAcceptCb(void *m) {
	Menu *md = (Menu*)m;
	UNUSED_PARAM(m);
	// IP configuration
	MwIpCfg ipCfg;
	MenuString tmpStr;
	int i = 2;


	// Save the configuration to WiFi module
	if (MwApCfgSet(selConfig, dynItems[0].caption.string,
				dynItems[1].caption.string) != MW_OK) {
		tmpStr.string = (char*)strCfgFail;
		tmpStr.length = sizeof(strCfgFail) - 1;
		MenuMessage(tmpStr, 120);
		return FALSE;
	}
	ipCfg.addr = MenuIpStr2Bin(dynItems[i++].caption.string);
	ipCfg.mask = MenuIpStr2Bin(dynItems[i++].caption.string);
	ipCfg.gateway = MenuIpStr2Bin(dynItems[i++].caption.string);
	ipCfg.dns1 = MenuIpStr2Bin(dynItems[i++].caption.string);
	ipCfg.dns2 = MenuIpStr2Bin(dynItems[i++].caption.string);
	if (MwIpCfgSet(selConfig, &ipCfg) != MW_OK) {
		tmpStr.string = (char*)strCfgFail;
		tmpStr.length = sizeof(strCfgFail) - 1;
		MenuMessage(tmpStr, 120);
		return FALSE;
	}
	// Go back two menu levels
	md->level -= 2;

	return TRUE;
}

int MenuNetConfEntryCb(void *m) {
	UNUSED_PARAM(m);
	const Menu *md = (Menu*)m;
	uint16_t offset;
	const uint8_t pLevel = md->level - 1;
//	uint8_t item;
	char ssidBuf[32 + 1];
	uint8_t i;
	uint8_t dhcp = FALSE;

	// Fill in scanned SSID, that must be still available in the dynamic
	// entries of the previous menu
//	item = md->selPage[pLevel] * md->me[pLevel]->item.entPerPage +
//		   md->selItem[pLevel];
	// Copy the SSID to a temporal buffer before putting it in the final
	// destination, because if SSID was the first entry and we copy it
	// directly, because of the preceding "SSID: " text, most likely it will
	// overwrite itself.
	i = 0;
	MenuStrCpy(ssidBuf, dynItems[pLevel].caption.string, 0);
	offset  = MenuStrCpy(dynPool, strSsid, 0);
	offset += MenuStrCpy(dynPool + offset, ssidBuf, 0);
	dynItems[i].caption.string = dynPool;
	dynItems[i].caption.length = offset++;
	/// \todo Will have to fill this to handle DHCP/manual configuration
	dynItems[i].cb = NULL;	
	dynItems[i].next = NULL;
	dynItems[i].flags.selectable = 1;
	dynItems[i++].flags.alt_color = 0;
	
	// Fill IP configuration of the curren entry. If not configured, fill
	// with default configuration (DHCP).
	if (MenuIpConfFill(&i, &offset, selConfig)) {
		dhcp = TRUE;
		MenuIpConfFillDhcp(&i, &offset);
	}
	// Add [BLANK] entry
	dynPool[offset] = '\0';
	dynItems[i].caption.string = dynPool + offset++;
	dynItems[i].caption.length = 0;
	// Add OK entry
	dynItems[i].caption.string = dynPool + offset;
	dynItems[i].caption.length = MenuStrCpy(dynPool + offset, strOk, 0);
	offset += dynItems[i++].caption.length + 1;
	// Add BACK entry
	dynItems[i].caption.string = dynPool + offset;
	dynItems[i].caption.length = MenuStrCpy(dynPool + offset, strOk, 0);
	offset += dynItems[i++].caption.length + 1;
	// Fill remaining item fields
	// - For SSID and IP
	for (i = 0; i < 2; i++) {
		dynItems[i].cb = NULL;
		/// \todo Fill this with correct value
		dynItems[i].next = NULL;
		dynItems[i].flags.selectable = 1;
		dynItems[i].flags.alt_color = 0;
	}
	// - For the remaining IP configuration fields
	for (; i < 6; i++) {
		dynItems[i].cb = NULL;
		/// \todo Fill this with correct value
		dynItems[i].next = NULL;
		dynItems[i].flags.selectable = ~dhcp;
		dynItems[i].flags.alt_color = dhcp;
	}
	// For the [BLANK] string
	dynItems[i].cb = NULL;
	dynItems[i].next = NULL;
	dynItems[i].flags.selectable = 0;
	dynItems[i++].flags.alt_color = 0;
	// For the OK string
	dynItems[i].cb = MenuNetConfEntryAcceptCb;
	dynItems[i].next = NULL;
	dynItems[i].flags.selectable = 1;
	dynItems[i++].flags.alt_color = 0;
	// For the BACK string
	dynItems[i].cb = NULL;
	dynItems[i].next = NULL;
	dynItems[i].flags.selectable = 1;
	dynItems[i++].flags.alt_color = 0;
	// For the OK and back strings:
	for (; i < 9; i++) {
		dynItems[i].next = NULL;
		dynItems[i].flags.selectable = 1;
		dynItems[i].flags.alt_color = 0;
	}
	return 0;
}

const MenuEntry MenuIpCfgEntry = {
	MENU_TYPE_ITEM,					// Menu type
	8,								// Margin
	MENU_STR("NETWORK CONFIGURATION"),	// Title
	MENU_STR(stdContext),			// Left context
	MenuNetConfEntryCb,				// cbEntry
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

/// Converts an IP address in uint32_t binary representation to
/// string representation. Returns resulting string length.
uint8_t MenuBin2IpStr(uint32_t addr, char str[]) {
	uint8_t i;

	i  = Byte2UnsStr(addr>>24, str);
	str[i++] = '.';
	i += Byte2UnsStr(addr>>16, str + i);
	str[i++] = '.';
	i += Byte2UnsStr(addr>>8, str + i);
	str[i++] = '.';
	i += Byte2UnsStr(addr, str + i);
	str[i] = '\0';
	return i;
}

int MenuWiFiScan(void *m) {
	UNUSED_PARAM(m);
	MenuString scanMenuStr = {(char*)strScan, sizeof(strScan) - 1};
	char *apData;
	uint16_t pos;
	MwApData apd;
	int16_t dataLen;
	uint8_t i;
	uint16_t strPos;

	// Clear previously drawn items, and print the WiFi scan message
	MenuClearLines(11, 13, 0);

	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(scanMenuStr, MENU_H_ALIGN_CENTER,
			0), 12, MENU_COLOR_ITEM_ALT, scanMenuStr.length,(char*)strScan);

	// Scan networks
	if ((dataLen = MwApScan(&apData)) == MW_ERROR) {
		MenuClearLines(11, 13, 0);

		scanMenuStr.string = (char*)strScanFail;	
		scanMenuStr.length = sizeof(strScanFail) - 1;
		MenuMessage(scanMenuStr, 120);
		return -1;
	}
	// Scan complete, fill in information.
	pos = 0;
	for (i = 0, pos = 0, strPos = 0; (pos = MwApFillNext(apData, pos, &apd,
			dataLen) > 0) && (i < WF_MENU_MAX_DYN_ITEMS); i++) {
		// Fill a dynEntry. Format is: signal_strength(3) auth(4) SSID(29)
		/// \todo check we do not overflow string buffer
		dynItems[i].caption.string = dynPool + strPos;
		if (apd.auth < MW_AUTH_OPEN || apd.auth > MW_AUTH_UNKNOWN)
			apd.auth = MW_AUTH_UNKNOWN;
		Byte2UnsStr(apd.str, dynPool + strPos);
		strPos += 4;
		strPos += MenuStrCpy(dynPool + strPos, security[(uint8_t)apd.auth], 0);
		strPos += MenuStrCpy(dynPool + strPos, apd.ssid, apd.ssidLen);
		dynPool[strPos++] = '\0';
		dynItems[i].caption.length = dynPool + strPos -
			dynItems[i].caption.string;

		dynItems[i].next = &MenuIpCfgEntry;
		dynItems[i].cb = NULL;
		dynItems[i].flags.selectable = 1;
		dynItems[i].flags.alt_color = 0;
	}

	
	return 0;
}

/// \note This entry is not const because the number of entries is unknown
/// until the scan is performed
MenuEntry confSsidSelEntry = {
	MENU_TYPE_ITEM,					// Menu type
	8,								// Margin
	MENU_STR("WIFI NETWORK"),		// Title
	MENU_STR(stdContext),			// Left context
	MenuWiFiScan,					// cbEntry
	NULL,							// cbExit
	.item = {
		dynItems,					// item
		0,							// nItems
		2,							// spacing
		0,							// entPerPage
		0,							// pages
		{MENU_H_ALIGN_LEFT}			// align
	}
};

int MenuConfSetActive(void *m) {
	UNUSED_PARAM(m);

	return 0;
}

int MenuConfDataEntryCb(void *m) {
	UNUSED_PARAM(m);
	char *ssid;
	uint16_t offset;
	uint8_t i;
	uint8_t strLen;
	uint8_t error = FALSE;

	// It is assumed that the configuration item is stored on previous level
	// data, as it corresponds to the selected option.
	i = 0;
	// Get the SSID
	if ((MwApCfgGet(selConfig, &ssid, NULL) != MW_OK) || (*ssid == '\0')) {
		// Configuration request failed, fill all items as empty
		// SSID
		error = TRUE;
		strLen  = MenuStrCpy(dynPool, strSsid, 0);
		strLen += MenuStrCpy(dynPool + strLen, strEmptyText, 0);
		dynItems[i].caption.string = dynPool;
		dynItems[i++].caption.length = strLen;
		offset = strLen + 1;
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
	// If no error, fill IP configuration
	if (error || MenuIpConfFill(&i, &offset, selConfig)) {
			error = TRUE;
			MenuIpConfFillDhcp(&i, &offset);
	}
	// [BLANK]
	dynItems[i].caption.length = sizeof(strEdit) - 1;
	*(dynPool + offset) = '\0';
	dynItems[i].cb = NULL;
	dynItems[i].next = NULL;
	dynItems[i].flags.selectable = 0;
	dynItems[i].flags.alt_color = 0;
	dynItems[i++].caption.length = 0;
	// EDIT
	dynItems[i].caption.string = dynPool + offset;
	dynItems[i].caption.length = sizeof(strEdit) - 1;
	dynItems[i].cb = MenuWiFiScan;
	dynItems[i].next = NULL;
	dynItems[i].flags.selectable = 1;
	dynItems[i++].flags.alt_color = 0;
	offset += 1 + MenuStrCpy(dynPool + offset, strEdit, 0);
	// SET AS ACTIVE
	dynItems[i].caption.string = dynPool + offset;
	dynItems[i].caption.length = sizeof(strAct) - 1;
	dynItems[i].next = NULL;
	if (error) {
		dynItems[i].flags.selectable = 0;
		dynItems[i].flags.alt_color = 1;
		dynItems[i].cb = NULL;
	} else {
		dynItems[i].flags.selectable = 1;
		dynItems[i].flags.alt_color = 0;
		dynItems[i].cb = MenuConfSetActive;
	}
	offset += 1 + MenuStrCpy(dynPool + offset, strAct, 0);
	// Fill remaining fields
	for (i = 0; i < 6; i++) {
		dynItems[i].cb = NULL;
		dynItems[i].next = NULL;
		dynItems[i].flags.selectable = 0;
		dynItems[i].flags.alt_color = 1;
	}
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
	MENU_STR("NETWORK CONFIGURATION"),	// Title
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

/// Sets the selected menu entry configuration variable
int MenuConfEntrySet(void *m) {
	Menu *md = (Menu*)m;

	selConfig = md->selItem[md->level];
	
	return 1;
}

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
		dynItems[i].cb = MenuConfEntrySet;
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
			if (copied == 32) dynPool[offset + 32] = '\0';
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

