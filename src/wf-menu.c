#include "wf-menu.h"
#include "util.h"
#include "vdp.h"
#include "mw/megawifi.h"
#include <string.h>

#define _FAKE_WIFI

#define WF_IP_MAXLEN	16

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
	(MenuItem*)(item), MENU_ENTRY_NUMS(MENU_NITEMS(item),(spacing))

/// (Network parameters that can be configured
typedef enum {
	WF_NET_SSID = 0,		///< WiFi AP SSID
	WF_NET_PASS,			///< WiFi AP password
	WF_NET_IP,				///< IPv4 address
	WF_NET_MASK,			///< Network mask
	WF_NET_GATEWAY,			///< Gateway
	WF_NET_DNS1,			///< DNS, first entry
	WF_NET_DNS2,			///< DNS, second entry
	WF_NET_CFG_PARAMS		///< Number of network parameters to configure
} WfNetConfItems;

/// IP address definition
typedef union {
	uint32_t addr;
	uint8_t byte[4];
} IpAddr;

////int TestCb(void* md) {
////	Menu *m = (Menu*)md;
////	const MenuEntry* me = m->me[m->level];
////	uint8_t i;
////
////	VdpDrawText(VDP_PLANEA_ADDR, 1, 1, MENU_COLOR_OSK_DATA, me->title.length,
////			me->title.string);
////
////	for (i = 0; i < 120; i++) VdpVBlankWait();
////
////	return TRUE;
//////	return FALSE;
////}

const char security[][4] = {
	"OPEN", "WEP ", "WPA1", "WPA2", "WPA ", "??? "
};

/// Offset used on the network options emu
#define MENU_NET_VALUE_OFFSET	9

const char stdContext[] = "[A]ccept, [B]ack";
const char strScanContext[] = "[A]ccept, [B]ack, [C]: Rescan";
const char oskQwertyContext[] = "A-OK, B-Del, C-Caps, S-Done";
const char oskNumIpContext[] = "A-OK, B-Del, S-Done";
const char strEmptyText[] = "<EMPTY>";
const char strSsid[] = "SSID:    ";
const char strPass[] = "PASS:    ";
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
const char *strNetPar[WF_NET_CFG_PARAMS] = {
	"SSID", "PASS", "IP", "MASK", "GATEWAY", "DNS1", "DNS2"
};
const char strNetParLen[WF_NET_CFG_PARAMS] = {
	4, 4, 2, 4, 7, 4, 4
};

////char editableIp[16] = "192.168.1.60";
////char editableNum[9] = "123456";
////
////const MenuEntry numTest = {
////	MENU_TYPE_OSK_NUMERIC,
////	1,
////	MENU_STR("NUMERIC MENU TEST"),
////	MENU_STR(oskNumIpContext),
////	NULL,
////	TestCb,
////	.keyb = {
////		MENU_STR("Enter number:"),
////		{editableNum, 6},
////		8,
////		8
////	}
////};
////
////const MenuEntry ipTest = {
////	MENU_TYPE_OSK_IPV4,
////	1,
////	MENU_STR("IP MENU TEST"),
////	MENU_STR(oskNumIpContext),
////	NULL,
////	MenuIpValidate,
////	.keyb = {
////		MENU_STR("Enter IP address:"),
////		{editableIp, 12},
////		15,
////		15
////	}
////};
////
////char editableStr[32] = "Edit me!";
////
////const MenuEntry editTest = {
////	MENU_TYPE_OSK_QWERTY,			// Item list type
////	1,								// Margin
////	MENU_STR("EDIT MENU TEST"),		// Title
////	MENU_STR(oskQwertyContext),		// Left context
////	NULL,							// cbEntry
////	NULL,							// cbExit
////	.keyb = {
////		MENU_STR("Input string test:"),
////		{editableStr, 8},
////		32,
////		32
////	}
////};
////
////const MenuItem confItem[] = { {
////	MENU_STR("String input test"),
////		&editTest,
////		NULL,
////		{1, 0}
////	},{
////	MENU_STR("IP entry test"),
////		&ipTest,
////		NULL,
////		{1, 0}
////	},{
////	MENU_STR("Numeric entry test"),
////		&numTest,
////		NULL,
////		{1, 0}
////	},{
////	MENU_STR("Unused"),
////		NULL,
////		NULL,
////		{1, 1}
////	}
////};


/// Module global menu data structure
typedef struct {
	/// Data of the menu configuration entries being edited
	char *netParPtr[WF_NET_CFG_PARAMS][12];
	/// Selected network configuration item (from 0 to 2)
	uint8_t selConfig;
} WfMenuData;

/// Module global data (other than item buffers)
static WfMenuData *wd;

///// Pool for dynamically created strings
//static char dynPool[WF_MENU_MAX_DYN_ITEMS * WF_MENU_AVG_STR_LEN];
///// Pool for dynamically created items
//static MenuItem dynItems[WF_MENU_MAX_DYN_ITEMS];

uint32_t MenuIpStr2Bin(char ip[]) {
	IpAddr addr;
	int i;

	for (i = 0; i < 4; i++) {
		if ((ip = Str2UnsByte(ip, &addr.byte[i])) == NULL) return 0;
		ip++;
	}
	return addr.addr;
}

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

///// Fills dynItems with network parameters. Does NOT fill callbacks,
///// next entries or flags
//int MenuIpConfFill(uint8_t *startItem, uint16_t *offset, 
//		uint8_t numConfig) {
//	MwIpCfg *ip;
//	uint8_t strLen;
//
//	// A MwIpCfgGet() return value of 0 means that there is an error.
//	if (MwIpCfgGet(numConfig, &ip) != MW_OK) return 1;
//
//	/// \todo Filling data is pretty regular and should be done in a loop
//	/// IP
//	/// \todo Implement automatic (DHCP) settings
//	strLen  = MenuStrCpy(dynPool + (*offset), strIp, 0);
//	wd.netParPtr[*startItem] = dynPool + (*offset);		// Store param ptr.
//	strLen += MenuBin2IpStr(ip->addr, dynPool + (*offset) + strLen);
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = strLen;
//	(*offset) += strLen + 1;
//	// MASK
//	strLen  = MenuStrCpy(dynPool + (*offset), strMask, 0);
//	wd.netParPtr[*startItem] = dynPool + (*offset);		// Store param ptr.
//	strLen += MenuBin2IpStr(ip->mask, dynPool + (*offset) + strLen);
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = strLen;
//	(*offset) += strLen + 1;
//	// GATEWAY
//	strLen  = MenuStrCpy(dynPool + (*offset), strGw, 0);
//	wd.netParPtr[*startItem] = dynPool + (*offset);		// Store param ptr.
//	strLen += MenuBin2IpStr(ip->gateway, dynPool + (*offset) + strLen);
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = strLen;
//	(*offset) += strLen + 1;
//	// DNS1
//	strLen  = MenuStrCpy(dynPool + (*offset), strDns1, 0);
//	wd.netParPtr[*startItem] = dynPool + (*offset);		// Store param ptr.
//	strLen += MenuBin2IpStr(ip->dns1, dynPool + (*offset) + strLen);
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = strLen;
//	(*offset) += strLen + 1;
//	// DNS2
//	strLen  = MenuStrCpy(dynPool + (*offset), strDns1, 0);
//	wd.netParPtr[*startItem] = dynPool + (*offset);		// Store param ptr.
//	strLen += MenuBin2IpStr(ip->dns1, dynPool + (*offset) + strLen);
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = strLen;
//	(*offset) += strLen + 1;
//
//	return 0;
//}
//
///// Fills dynItems with default network parameters. Does NOT fill callbacks,
///// next entries or flags
//uint16_t MenuIpConfFillDhcp(uint8_t *startItem, uint16_t *offset) {
//	uint8_t strLen;
//
//	// IP
//	strLen  = MenuStrCpy(dynPool + (*offset), strIp, 0);
//	strLen += MenuStrCpy(dynPool + (*offset) + strLen, strDhcp, 0);
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = strLen;
//	*offset += strLen + 1;
//	// MASK
//	dynPool[(*offset)] = '\0';
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = 0;
//	// GATEWAY
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = 0;
//	// DNS1
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = 0;
//	// DNS1
//	dynItems[(*startItem)].caption.string = dynPool + (*offset);
//	dynItems[(*startItem)++].caption.length = 0;
//
//	return 0;
//}
//
//int MenuNetConfEntryBackCb(void *m) {
//	UNUSED_PARAM(m);
//
//	return 0;
//}
//
//
//int MenuNetConfEntryAcceptCb(void *m) {
//	Menu *md = (Menu*)m;
//	UNUSED_PARAM(m);
//	// IP configuration
//	MwIpCfg ipCfg;
//	MenuString tmpStr;
//	int i = 2;
//
//
//	// Save the configuration to WiFi module
//	if (MwApCfgSet(wd.selConfig, dynItems[0].caption.string +
//			sizeof(strSsid) - 1,
//				dynItems[1].caption.string) != MW_OK) {
//		tmpStr.string = (char*)strCfgFail;
//		tmpStr.length = sizeof(strCfgFail) - 1;
//		MenuMessage(tmpStr, 120);
//		return FALSE;
//	}
//	ipCfg.addr = MenuIpStr2Bin(dynItems[i++].caption.string);
//	ipCfg.mask = MenuIpStr2Bin(dynItems[i++].caption.string);
//	ipCfg.gateway = MenuIpStr2Bin(dynItems[i++].caption.string);
//	ipCfg.dns1 = MenuIpStr2Bin(dynItems[i++].caption.string);
//	ipCfg.dns2 = MenuIpStr2Bin(dynItems[i++].caption.string);
//	if (MwIpCfgSet(wd.selConfig, &ipCfg) != MW_OK) {
//		tmpStr.string = (char*)strCfgFail;
//		tmpStr.length = sizeof(strCfgFail) - 1;
//		MenuMessage(tmpStr, 120);
//		return FALSE;
//	}
//	// Go back two menu levels
//	md->level -= 2;
//
//	return TRUE;
//}
//int MenuNetParamEditCb(void *m) {
//	const Menu *md = (Menu*)m;
//	int item = md->selItem[md->level - 1];
//	MenuEntry *me = (MenuEntry*)md->me[md->level];
//
//	// Set menu title, data and content string (from previous menu entry)
//	// We have to fill:
//	// me->keyb.fieldName: <FIELD NAME>
//	// me->keyb.fieldData: <point to variable>
//	// me->keyb.maxLen:    <Set depending on field>
//	// me->keyb.lineLen:   <Set depending on field>
//	memcpy(dynPool, strNetPar[item], strNetParLen[item] + 1);
//	me->keyb.fieldName.string = dynPool;
//	me->keyb.fieldName.length = strNetParLen[item];
//	// TODO: CONTINUE HERE, PLAN WHICH IS THE BEST WAY TO RECOVER DATA
//	// MAYBE THE BEST WAY IS STORING POINTERS TO dynPool when filling
//	// data on previous step
//
////	switch 
//	return FALSE;
//}
//
////const MenuEntry ipTest = {
////	MENU_TYPE_OSK_IPV4,
////	1,
////	MENU_STR("IP MENU TEST"),
////	MENU_STR(oskNumIpContext),
////	NULL,
////	MenuIpValidate,
////	.keyb = {
////		MENU_STR("Enter IP address:"),
////		{editableIp, 12},
////		15,
////		15
////	}
////};
//
//MenuEntry editTest = {
//	MENU_TYPE_OSK_QWERTY,			// Item list type
//	1,								// Margin
//	MENU_STR(strEdit),				// Title
////	{NULL, 0},						// Title
//	MENU_STR(oskQwertyContext),		// Left context
//	MenuNetParamEditCb,				// cbEntry
//	NULL,							// cbExit
//	.keyb = {
//		{NULL, 0},
//		{NULL, 0},
//		0,
//		0
//	}
//};
//
//// Items:
//// 0: SSID
//// 1: PASS
//// 2: IP
//// 3: MASK
//// 4: GATEWAY
//// 5: DNS1
//// 6: DNS2
//int MenuNetConfEntryCb(void *m) {
//	UNUSED_PARAM(m);
//	const Menu *md = (Menu*)m;
//	uint16_t offset;
//	const uint8_t pLevel = md->level - 1;
//	uint8_t item;
//	char ssidBuf[32 + 9 + 1];
//	uint8_t i;
//	uint8_t dhcp = FALSE;
//
//	// Fill in scanned SSID, that must be still available in the dynamic
//	// entries of the previous menu
//	// TODO: Enhance function to work if we do not come from a WiFi scan
//	item = md->selPage[pLevel] * md->me[pLevel]->item.entPerPage +
//		   md->selItem[pLevel];
//	// Copy the SSID to a temporal buffer before putting it in the final
//	// destination, because if SSID was the first entry and we copy it
//	// directly, because of the preceding "SSID: " text, most likely it will
//	// overwrite itself.
//	i = 0;
//	// network parameter edit menu?
//	// Note: string includes SSID identifier text
//	MenuStrCpy(ssidBuf, dynItems[item].caption.string, 0);
//	offset = MenuStrCpy(dynPool, ssidBuf, 0);
//	wd.netParPtr[i] = dynPool + MENU_NET_VALUE_OFFSET;	// Store param ptr.
//	dynItems[i].caption.string = dynPool;
//	dynItems[i].caption.length = offset++;
//	dynItems[i].cb = NULL;	
//	dynItems[i].next = NULL;
//	dynItems[i].selectable = 1;
//	dynItems[i++].alt_color = 0;
//	// Add empty PASS entry
//	// TODO: Warning, maybe we should leave 64 + 1 bytes for a full password!
//	dynItems[i].caption.string = dynPool + offset;
//	offset += MenuStrCpy(dynPool + offset, strPass, 0);
//	wd.netParPtr[i] = dynPool + offset;					// Store param ptr.
//	offset += MenuStrCpy(dynPool + offset, strEmptyText, 0);
//	dynItems[i].caption.length = dynPool + offset -
//		dynItems[i].caption.string;
//	offset++;
//	dynItems[i].cb = NULL;
//	dynItems[i].next = NULL;
//	dynItems[i].selectable = 1;
//	dynItems[i++].alt_color = 0;
//	
//	// Fill IP configuration of the curren entry. If not configured, fill
//	// with default configuration (DHCP).
//	if (MenuIpConfFill(&i, &offset, wd.selConfig)) {
//		dhcp = TRUE;
//		MenuIpConfFillDhcp(&i, &offset);
//	}
//	// Add [BLANK] entry
//	dynPool[offset] = '\0';
//	dynItems[i].caption.string = dynPool + offset++;
//	dynItems[i].caption.length = 0;
//	// Add OK entry
//	dynItems[i].caption.string = dynPool + offset;
//	dynItems[i].caption.length = MenuStrCpy(dynPool + offset, strOk, 0);
//	offset += dynItems[i++].caption.length + 1;
//	// Add BACK entry
//	dynItems[i].caption.string = dynPool + offset;
//	dynItems[i].caption.length = MenuStrCpy(dynPool + offset, strOk, 0);
//	offset += dynItems[i++].caption.length + 1;
//	// Fill remaining item fields
//	// - For SSID and IP
//	for (i = 0; i < 2; i++) {
//		dynItems[i].cb = NULL;
//		/// \todo Fill this with correct value
//		dynItems[i].next = NULL;
//		dynItems[i].selectable = 1;
//		dynItems[i].alt_color = 0;
//	}
//	// - For the remaining IP configuration fields
//	for (; i < 6; i++) {
//		dynItems[i].cb = NULL;
//		/// \todo Fill this with correct value
//		dynItems[i].next = NULL;
//		dynItems[i].selectable = ~dhcp;
//		dynItems[i].alt_color = dhcp;
//	}
//	// For the [BLANK] string
//	dynItems[i].cb = NULL;
//	dynItems[i].next = NULL;
//	dynItems[i].selectable = 0;
//	dynItems[i++].alt_color = 0;
//	// For the OK string
//	dynItems[i].cb = MenuNetConfEntryAcceptCb;
//	dynItems[i].next = NULL;
//	dynItems[i].selectable = 1;
//	dynItems[i++].alt_color = 0;
//	// For the BACK string
//	dynItems[i].cb = NULL;
//	dynItems[i].next = NULL;
//	dynItems[i].selectable = 1;
//	dynItems[i++].alt_color = 0;
//	// For the OK and back strings:
//	for (; i < 9; i++) {
//		dynItems[i].next = NULL;
//		dynItems[i].selectable = 1;
//		dynItems[i].alt_color = 0;
//	}
//	return 0;
//}
//
//const MenuEntry MenuIpCfgEntry = {
//	MENU_TYPE_ITEM,					// Menu type
//	8,								// Margin
//	MENU_STR("NETWORK CONFIGURATION"),	// Title
//	MENU_STR(stdContext),			// Left context
//	MenuNetConfEntryCb,				// entry callback
//	NULL,							// exit callback
//	NULL,							// cBut callback
//	.item = {
//		dynItems,					// item
//		10,							// nItems
//		2,							// spacing
//		10,							// entPerPage
//		0,							// pages
//		{MENU_H_ALIGN_LEFT}			// align
//	}
//};

uint16_t MenuIpConfFillDhcp(uint8_t *startItem, MenuItem* item, MwIpCfg *ip) {
	int i = *startItem;
	char addr[16];

	addr[15] = '\0';
	// IP
	MenuBin2IpStr(ip->addr, addr);
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			addr, MW_SSID_MAXLEN);
	i++;
	// MASK
	MenuBin2IpStr(ip->mask, addr);
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			addr, MW_SSID_MAXLEN);
	i++;
	// GATEWAY
	MenuBin2IpStr(ip->gateway, addr);
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			addr, MW_SSID_MAXLEN);
	i++;
	// DNS1
	MenuBin2IpStr(ip->dns1, addr);
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			addr, MW_SSID_MAXLEN);
	i++;
	// DNS1
	MenuBin2IpStr(ip->dns2, addr);
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			addr, MW_SSID_MAXLEN);
	i++;

	*startItem = i;

	return 0;
}

int MenuWiFiScan(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;
	MenuString str;
	char *apData;
	uint16_t pos;
	MwApData apd;
	int16_t dataLen;
	uint8_t i;
	uint16_t strPos;

	// Clear previously drawn items, and print the WiFi scan message
	str.string = (char*)strScan;
	str.length = sizeof(strScan) - 1;
	MenuMessage(str, 0);

	// Disconnect from network
	MwApLeave();
	// Scan networks
	if ((dataLen = MwApScan(&apData)) == MW_ERROR) {
		str.string = (char*)strScanFail;	
		str.length = sizeof(strScanFail) - 1;
		MenuMessage(str, 120);
		return FALSE;
	}
	str.string = (char*)"SCAN OK!!!";
	str.length = 10;
	MenuMessage(str, 120);
	// Scan complete, fill in MenuItem information.
//	pos = 0;
//	for (i = 0, pos = 0, strPos = 0; (pos = MwApFillNext(apData, pos, &apd,
//			dataLen) > 0) && (i < WF_MENU_MAX_DYN_ITEMS); i++) {
//		// Fill a dynEntry. Format is: signal_strength(3) auth(4) SSID(29)
//		/// \todo check we do not overflow string buffer
//		dynItems[i].caption.string = dynPool + strPos;
//		if (apd.auth < MW_AUTH_OPEN || apd.auth > MW_AUTH_UNKNOWN)
//			apd.auth = MW_AUTH_UNKNOWN;
//		Byte2UnsStr(apd.str, dynPool + strPos);
//		strPos += 4;
//		strPos += MenuStrCpy(dynPool + strPos, security[(uint8_t)apd.auth], 0);
//		strPos += MenuStrCpy(dynPool + strPos, apd.ssid, apd.ssidLen);
//		dynPool[strPos++] = '\0';
//		dynItems[i].caption.length = dynPool + strPos -
//			dynItems[i].caption.string;
//
//		dynItems[i].next = &MenuIpCfgEntry;
//		dynItems[i].cb = NULL;
//		dynItems[i].selectable = 1;
//		dynItems[i].alt_color = 0;
//	}

	
	return 0;
}

///// \note This entry is not const because the number of entries is unknown
///// until the scan is performed
//MenuEntry confSsidSelEntry = {
//	MENU_TYPE_ITEM,					// Menu type
//	8,								// Margin
//	MENU_STR("WIFI NETWORK"),		// Title
//	MENU_STR(strScanContext),		// Left context
//	MenuWiFiScan,					// entry callback
//	NULL,							// exit callback
//	MenuWiFiScan,					// cBut callback
//	.item = {
//		dynItems,					// item
//		0,							// nItems
//		2,							// spacing
//		0,							// entPerPage
//		0,							// pages
//		{MENU_H_ALIGN_LEFT}			// align
//	}
//};

/// Set active configuration
int MenuConfSetActive(void *m) {
	UNUSED_PARAM(m);

	// Set default config to selected entry
	return MwDefApCfg(wd->selConfig);

	/// \todo Notify user? Go back a menu level?
	/// Maybe using MenuMessage()
	
}

int MenuConfDataEntryCb(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;
	char *ssid, *pass;
	MwIpCfg *ip;
	uint8_t i;
	uint8_t error = FALSE;

	i = 0;
	// Get the SSID and password
	if ((MwApCfgGet(wd->selConfig, &ssid, &pass) != MW_OK) || (*ssid == '\0')) {
		// Configuration request failed, fill all items as empty
		// SSID
		error = TRUE;
		
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				strEmptyText, 0);
		i++;
		// PASS
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				strEmptyText, 0);
		i++;
	} else {
		// SSID
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				ssid, MW_SSID_MAXLEN);
		i++;
		// PASS
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				pass, MW_SSID_MAXLEN);
		i++;
	}
	// If no error, fill IP configuration
	if (error || (MW_OK != MwIpCfgGet(wd->selConfig, &ip))) {
			error = TRUE;
	} else MenuIpConfFillDhcp(&i, item, ip);
	// [BLANK]
	// EDIT
	// SET AS ACTIVE
	i = 9;
	if (error) {
		item[i].selectable = 0;
		item[i].alt_color = 1;
		item[i].cb = NULL;
	} else {
		item[i].selectable = 1;
		item[i].alt_color = 0;
//		item[i].cb = MenuConfSetActive;
	}
	i++;
	return 0;
}

// Menu structure:
// SSID:    <data>
// PASS:	<data>
// IP:      <MANUAL,AUTO>
// MASK:    <DATA>
// GATEWAY: <DATA>
// DNS1:    <DATA>
// DNS2:    <DATA>
// [BLANK]
// EDIT
// SET AS DEFAULT
/// Network parameters menu
const MenuItem confNetPar[] = {
	{
		// Editable SSID
		MENU_ESTR(strSsid, 3 + 32 + 1),
		NULL,						// Next
		NULL,						// Callback
		{{0, 1, 0}}					// Selectable, alt_color, hide
	}, {
		// Editable PASS
		MENU_ESTR(strPass, 3 + 32 + 1),
		NULL,						// Next
		NULL,						// Callback
		{{0, 1, 0}}					// Selectable, alt_color, hide
	}, {
		// Editable IP
		MENU_ESTR(strIp, 9 + WF_IP_MAXLEN),
		NULL,						// Next
		NULL,						// Callback
		{{0, 1, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable netmask
		MENU_ESTR(strMask, 9 + WF_IP_MAXLEN),
		NULL,						// Next
		NULL,						// Callback
		{{0, 1, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable gateway
		MENU_ESTR(strGw, 9 + WF_IP_MAXLEN),
		NULL,						// Next
		NULL,						// Callback
		{{0, 1, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable DNS1
		MENU_ESTR(strDns1, 9 + WF_IP_MAXLEN),
		NULL,						// Next
		NULL,						// Callback
		{{0, 1, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable DNS2
		MENU_ESTR(strDns2, 9 + WF_IP_MAXLEN),
		NULL,						// Next
		NULL,						// Callback
		{{0, 1, 1}}					// Selectable, alt_color, hide
	}, {
		MENU_EESTR(0),				// [EMPTY]
		NULL,						// Next
		NULL,						// Callback
		{{0, 0, 1}}					// Selectable, alt_color, hide
	}, {
		MENU_ESTR(strEdit, 9 + 5),	// EDIT
		MenuWiFiScan,
		NULL,						// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_ESTR(strAct, 9 + 14),	// Set as active
		NULL,						// Next
		NULL,						// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}
};



const MenuEntry confEntryData = {
	MENU_TYPE_ITEM,					// Menu type
	8,								// Margin
	MENU_STR("NETWORK CONFIGURATION"),	// Title
	MENU_STR(stdContext),			// Left context
	MenuConfDataEntryCb,			// entry callback
	NULL,							// exit callback
	NULL,							// cBut callback
	.mItem = {
		// rootItem, nItems, spacing, enPerPage, pages
		MENU_ENTRY_ITEM(confNetPar, 2),
		{MENU_H_ALIGN_LEFT}			// align
	}
};

/// Sets the selected menu entry configuration variable
int MenuConfEntrySet(void *m) {
	Menu *md = (Menu*)m;

	wd->selConfig = md->me->prev->selItem;
	
	return 1;
}

/// Fills confEntry items
/// \todo Add a marker to default configuration
int MenuConfEntryCb(void* m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;
	int8_t i;
#ifndef _FAKE_WIFI
	uint16_t copied;
	char *ssid;
#endif

	// Load MegaWiFi configurations and fill entries with available SSIDs
	for (i = 0; i < 3; i++) {
		item[i].caption.string[0] = '1' + i;
		item[i].caption.string[1] = ':';
		item[i].caption.string[2] = ' ';

#ifdef _FAKE_WIFI
		// Fake AP data
		strcpy(item[i].caption.string + 3, "FAKE_AP_1");
		item[i].caption.string[11] += i;
		item[i].caption.length = 12;
#else
		// Obtains configurations and fill SSIDs
		if ((MwApCfgGet(i, &ssid, NULL) != MW_OK) || (*ssid == '\0')) {
			strcpy(item[i].caption.string + 3, "<EMPTY>");
			item[i].caption.length = 10;
		} else {
			copied = MenuStrCpy(item[i].caption.string + 3, ssid, 32);
			if (copied == 32) item[i].caption.string[3 + 32] = '\0';
			item[i].caption.length = copied + 3;
		}
#endif //_FAKE_WIFI
	}

	return 0;
}

/// Supported configuration items
const MenuItem confItem[] = {
	{
		// Default caption (will be dynamically modified)
		MENU_ESTR(strEmptyText, 3 + 32 + 1),
		(void*)&confEntryData,		// Next
		MenuConfEntrySet,			// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_ESTR(strEmptyText, 3 + 32 + 1),
		(void*)&confEntryData,		// Next
		MenuConfEntrySet,			// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_ESTR(strEmptyText, 3 + 32 + 1),
		(void*)&confEntryData,		// Next
		MenuConfEntrySet,			// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}
};


const MenuEntry confEntry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("CONFIGURATION"),		// Title
	MENU_STR(stdContext),			// Left context
	MenuConfEntryCb,				// entry
	NULL,							// exit
	NULL,							// cBut callback
	.mItem = {
		// rootItem, nItems, spacing, enPerPage, pages
		MENU_ENTRY_ITEM(confItem, 3),
		{MENU_H_ALIGN_LEFT}			// align
	}
};

/// Root menu items
const MenuItem rootItem[] = { {
		MENU_STR("START"),			// Caption
		NULL,						// Next: none (yet ;-)
		NULL,						// Callback
		{{1, 1, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_STR("CONFIGURATION"),
		(void*)&confEntry,			// Next: Configuration entry
		NULL,
		{{1, 0, 0}}
	}
};

/// Root menu
const MenuEntry rootMenu = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("WFLASH BOOTLOADER"),	// Title
	MENU_STR(stdContext),			// Left context
	NULL,							// entry callback
	NULL,							// exit callback
	NULL,							// cBut callback
	.mItem = {
		// rootItem, nItems, spacing, enPerPage, pages
		MENU_ENTRY_ITEM(rootItem, 2),
		{MENU_H_ALIGN_CENTER}		// align
	}
};

void WfMenuInit(MenuString statStr) {
	wd = MpAlloc(sizeof(WfMenuData));
	MenuInit(&rootMenu, statStr);
}

