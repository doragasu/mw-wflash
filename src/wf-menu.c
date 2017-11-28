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

/// \brief Macro to fill on MenuEntry structures:
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

/// IP parameters for the ipPar field of the 
typedef enum {
	WF_IP_ADDR,			///< IPv4 address
	WF_IP_MASK,			///< Network mask
	WF_IP_GATEWAY,		///< Gateway
	WF_IP_DNS1,			///< DNS, first entry
	WF_IP_DNS2,			///< DNS, second entry
	WF_IP_CFG_PARAMS	///< Number of IP parameters to configure
} WfIpConfItems;

/// IP address definition
typedef union {
	uint32_t addr;
	uint8_t byte[4];
} IpAddr;

/// Strings for the supported auth types
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
const char strIp[] =   "ADDRESS: ";
const char strMask[] = "MASK:    ";
const char strGw[] =   "GATEWAY: ";
const char strDns1[] = "DNS1:    ";
const char strDns2[] = "DNS2:    ";
const char strEdit[] = "EDIT";
const char strAct[] =  "SET AS ACTIVE";
const char strIpAuto[] = "IP CONFIG: AUTOMATIC";
const char strIpManual[] = "IP CONFIG: MANUAL";
const char strScan[] = "SCAN IN PROGRESS, PLEASE WAIT...";
const char strScanFail[] = "SCAN FAILED!";
const char strCfgFail[] = "CONFIGURATION FAILED!";
const char strDhcp[] = "AUTO";
const char strStartScan[] = "SCAN...";
const char strSave[] = "SAVE!";
const char strOk[] = "OK";
const char strDone[] = "DONE!";
const char strFailed[] = "FAILED!";
const char strWrongIp[] = "WRONG IP!";
const char *strNetPar[WF_NET_CFG_PARAMS] = {
	"SSID", "PASS", "IP", "MASK", "GATEWAY", "DNS1", "DNS2"
};
const char strNetParLen[WF_NET_CFG_PARAMS] = {
	4, 4, 7, 4, 7, 4, 4
};

const char strDefIp[] =     "192.168.1.64";
const char strDefMask[] =   "255.255.255.0";
const char strDefGw[] =     "192.168.1.1";
const char strDefDns1[] =   "192.168.1.1";
const char strDefDns2[] =   "8.8.8.8";

/// Module global menu data structure
typedef struct {
	/// Menu items of the scanned WiFi APs.
	MenuItem *item;
	/// Data of the IP configuration entries being edited
	char ipPar[WF_IP_CFG_PARAMS][WF_IP_MAXLEN];
	/// Selected network configuration item (from 0 to 2)
	uint8_t selConfig;
	/// Number of scanned APs
	uint8_t aps;
	/// SSID being edited
    MenuString ssid;
    /// Password being edited
    MenuString pass;
    /// SSID buffer
	char ssidBuf[33];
	/// Password being edited
	char passBuf[65];
} WfMenuData;

// Private prototypes
int MenuWiFiScan(void *m);
int MenuSsidLink(void *m);
int MenuConfEntrySet(void *m);
int MenuConfEntryCb(void* m);
static int MenuIpOskEnter(void *m);
static int MenuIpOskExit(void *m);
uint16_t MenuIpConfFillDhcp(MenuItem* item);
static int MenuApPassExitCb(void *m);
int MenuSsidCopySelected(void *m);
int MenuIpConfToggle(void *m);
int MenuConfDataEntryCb(void *m);
void MenuFillNetPar(MwIpCfg *ip);
void MenuFillDefaultNetPar(void);
static void MenuIpConfShow(MenuItem* item);
static void MenuIpConfHide(MenuItem* item);

/// Module global data (other than item buffers)
static WfMenuData *wd;

/// Convert an IPv4 address in string format to binary format
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

/****************************************************************************
 * IP configuration parameter entry menus
 *
 * Title: <several>
 *
 * AP password input screen.
 ****************************************************************************/
const MenuEntry ipSsidOsk = {
	MENU_TYPE_OSK_QWERTY,		    // QWERTY keyboard
	8,								// Margin
	MENU_STR("SSID"),			    // Title
	MENU_STR(oskQwertyContext),		// Left context
	MenuIpOskEnter,					// cbEntry
	MenuIpOskExit, 				    // cbExit
	.keyb = {
		MENU_STR("Enter access point SSID:"),
		MENU_EESTR(0),
		33,
	    33
	}
};

const MenuEntry ipAddrOsk = {
	MENU_TYPE_OSK_IPV4,			    // IPv4 entry
	8,								// Margin
	MENU_STR("IP ADDRESS"),			// Title
	MENU_STR(oskNumIpContext),		// Left context
	MenuIpOskEnter,					// cbEntry
	MenuIpOskExit, 				    // cbExit
	.keyb = {
		MENU_STR("Enter IPv4 address:"),
		MENU_EESTR(0),
		15,
		15
	}
};

const MenuEntry ipMaskOsk = {
	MENU_TYPE_OSK_IPV4,			    // IPv4 entry
	8,								// Margin
	MENU_STR("NET MASK"),			// Title
	MENU_STR(oskNumIpContext),		// Left context
	MenuIpOskEnter,					// cbEntry
	MenuIpOskExit, 				    // cbExit
	.keyb = {
		MENU_STR("Enter subnet mask:"),
		MENU_EESTR(0),
		15,
		15
	}
};

const MenuEntry ipGwOsk = {
	MENU_TYPE_OSK_IPV4,			    // IPv4 entry
	8,								// Margin
	MENU_STR("GATEWAY"),			// Title
	MENU_STR(oskNumIpContext),		// Left context
	MenuIpOskEnter,					// cbEntry
	MenuIpOskExit, 				    // cbExit
	.keyb = {
		MENU_STR("Enter Gateway:"),
		MENU_EESTR(0),
		15,
		15
	}
};

const MenuEntry ipDns1Osk = {
	MENU_TYPE_OSK_IPV4,			    // IPv4 entry
	8,								// Margin
	MENU_STR("PRIMARY DNS"),		// Title
	MENU_STR(oskNumIpContext),		// Left context
	MenuIpOskEnter,					// cbEntry
	MenuIpOskExit, 				    // cbExit
	.keyb = {
		MENU_STR("Enter primary DNS address:"),
		MENU_EESTR(0),
		15,
		15
	}
};

const MenuEntry ipDns2Osk = {
	MENU_TYPE_OSK_IPV4,			    // IPv4 entry
	8,								// Margin
	MENU_STR("SECONDARY DNS"),		// Title
	MENU_STR(oskNumIpContext),		// Left context
	MenuIpOskEnter,					// cbEntry
	MenuIpOskExit, 				    // cbExit
	.keyb = {
		MENU_STR("Enter secondary DNS address:"),
		MENU_EESTR(0),
		15,
		15
	}
};

/// Menu entries for the network configuration menu
enum {
	MENU_NET_CONF_SSID = 0,		///< SSID entry
	MENU_NET_CONF_PASS,			///< Password entry (hidden text)
	MENU_NET_CONF_SCAN,		    ///< IP type (AUTOMATIC/MANUAL)
	MENU_NET_CONF_IP_TYPE,		///< IP type (AUTOMATIC/MANUAL)
	MENU_NET_CONF_IP,			///< IP address
	MENU_NET_CONF_MASK,			///< Net mask
	MENU_NET_CONF_GATEWAY,		///< Gateway address
	MENU_NET_CONF_DNS1,			///< Primary DNS
	MENU_NET_CONF_DNS2,			///< Secondary DNS
	MENU_NET_CONF_OK,			///< Save configuration
	MENU_NET_CONF_NUM_ENTRIES	///< Number of menu entries
};

/// Sets the MenuString data of the IP address configuration to be edited
static int MenuIpOskEnter(void *m) {
	Menu *md = (Menu*)m;
    int sel;

    // Set text caption depending on selected entry
    sel = md->me->prev->selItem;
    switch (sel) {
        case MENU_NET_CONF_SSID:
            md->me->mEntry.keyb.fieldData.string = wd->ssid.string;
            md->me->mEntry.keyb.fieldData.length = strlen(wd->ssid.string);
            break;

        case MENU_NET_CONF_PASS:
            break;

        case MENU_NET_CONF_SCAN:
            break;
                
        case MENU_NET_CONF_IP_TYPE:
            break;

        case MENU_NET_CONF_IP:
        case MENU_NET_CONF_MASK:
        case MENU_NET_CONF_GATEWAY:
        case MENU_NET_CONF_DNS1:
        case MENU_NET_CONF_DNS2:
            md->me->mEntry.keyb.fieldData.string = wd->ipPar[sel -
                MENU_NET_CONF_IP];
            md->me->mEntry.keyb.fieldData.length = strlen(
                    md->me->mEntry.keyb.fieldData.string);
            break;

        case MENU_NET_CONF_OK:
        case MENU_NET_CONF_NUM_ENTRIES:
        default:
            break;
    }

    return TRUE;
}

static int MenuIpOskExit(void *m) {
	Menu *md = (Menu*)m;
    MenuString msg;
    int sel;

    // Get selected option
    sel = md->me->prev->selItem;

    // Validate selected text
    switch (sel) {
        case MENU_NET_CONF_SSID:
            break;

        case MENU_NET_CONF_PASS:
            break;

        case MENU_NET_CONF_SCAN:
            break;
                
        case MENU_NET_CONF_IP_TYPE:
            break;

        case MENU_NET_CONF_IP:
        case MENU_NET_CONF_MASK:
        case MENU_NET_CONF_GATEWAY:
        case MENU_NET_CONF_DNS1:
        case MENU_NET_CONF_DNS2:
            // Check entered string is a valid IP address
            if (!MenuIpValidate(m)) {
                msg.string = (char*)strWrongIp;
                msg.length = sizeof(strWrongIp) - 1;
                MenuMessage(msg, 60);
                return FALSE;
            }
            strcpy(wd->ipPar[sel - MENU_NET_CONF_IP], md->strBuf);
            MenuIpConfFillDhcp(md->me->prev->mEntry.mItem.item);
            break;

        case MENU_NET_CONF_OK:
        case MENU_NET_CONF_NUM_ENTRIES:
        default:
            break;
    }
    // Update menu items
    
    return TRUE;
}

/****************************************************************************
 * Password entry menu
 *
 * Title: PASSWORD
 *
 * AP password input screen.
 ****************************************************************************/
const MenuEntry apPass = {
	MENU_TYPE_OSK_QWERTY,			// Item list type
	1,								// Margin
	MENU_STR("PASSWORD"),			// Title
	MENU_STR(oskQwertyContext),		// Left context
	NULL,							// cbEntry
	MenuApPassExitCb,				// cbExit
	.keyb = {
		MENU_STR("Enter AP password:"),
		MENU_EESTR(32),
		32,
		32
	}
};

static int MenuApPassExitCb(void *m) {
	Menu *md = (Menu*)m;
    MenuString msg;

	// Set resulting SSID and password configuration
    if (MW_OK == MwApCfgSet(wd->selConfig, wd->ssidBuf,
                md->me->mEntry.keyb.fieldName.string)) {
        msg.string = (char*)strDone;
        msg.length = sizeof(strDone) - 1;
    } else {
        msg.string = (char*)strFailed;
        msg.length = sizeof(strFailed) - 1;
    }
    MenuMessage(msg, 30);
    // Go back an extra level
	MenuBack();

	return TRUE;
}

/****************************************************************************
 * WiFi APs menu
 *
 * This menu entry is not populated with items because they are dynamically
 * populated by the entry callback function.
 ****************************************************************************/
const MenuEntry confSsidSelEntry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("WIFI NETWORK"),		// Title
	MENU_STR(strScanContext),		// Left context
	MenuSsidLink,					// entry callback
	MenuSsidCopySelected,			// exit callback
	NULL,							// cBut callback
	.mItem = {
		NULL,						// item
		0,							// nItems
		2,							// spacing
		MENU_ITEM_NLINES/2,			// entPerPage
		0,							// pages
		{MENU_H_ALIGN_LEFT}			// align
	}
};

int MenuSsidLink(void *m) {
	Menu *md = (Menu*)m;
	MenuItemEntry *mie = &md->me->mEntry.mItem;

	// Fill scanned item data
	mie->item = wd->item;
	mie->nItems = wd->aps;
	mie->pages = mie->nItems / mie->entPerPage;
	if (0 != (mie->nItems % mie->entPerPage)) mie->entPerPage--;
	return TRUE;
}

/// Copy selected SSID entry to internal wd structure
int MenuSsidCopySelected(void *m) {
    int i;
	Menu *md = (Menu*)m;
	MenuItem *item = &md->me->mEntry.mItem.item[md->me->selItem];

    // Copy SSID data
    wd->ssid.string = wd->ssidBuf;
    wd->ssid.length = item->caption.length - 8;
    for (i = 0; i < wd->ssid.length; i++)
        wd->ssid.string[i] = item->caption.string[8 + i];
    wd->ssid.string[i] = '\0';

    return TRUE;
}

/// Fills wd->netPar with default IP configuration data
void MenuFillDefaultNetPar(void) {
    strcpy(wd->ipPar[WF_IP_ADDR], strDefIp);
    strcpy(wd->ipPar[WF_IP_MASK], strDefMask);
    strcpy(wd->ipPar[WF_IP_GATEWAY], strDefGw);
    strcpy(wd->ipPar[WF_IP_DNS1], strDefDns1);
    strcpy(wd->ipPar[WF_IP_DNS2], strDefDns2);
}

/// Fills wd->netPar with IP configuration data
void MenuFillNetPar(MwIpCfg *ip) {
	int i = WF_NET_IP;

	MenuBin2IpStr(ip->addr, wd->ipPar[i++]);
	MenuBin2IpStr(ip->mask, wd->ipPar[i++]);
	MenuBin2IpStr(ip->gateway, wd->ipPar[i++]);
	MenuBin2IpStr(ip->dns1, wd->ipPar[i++]);
	MenuBin2IpStr(ip->dns2, wd->ipPar[i++]);
}

/// Fills network configuration entries
uint16_t MenuIpConfFillDhcp(MenuItem* item) {
	int i = 0;

    // SSID and PASS
    if (!wd->ssid.length) {
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				strEmptyText, 0);
		i++;
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				strEmptyText, 0);
		i++;
	} else {
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				wd->ssid.string, MW_SSID_MAXLEN);
		i++;
		item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
				wd->pass.string, MW_SSID_MAXLEN);
		i++;
    }

    // Skip scan and manual/auto toggle options
    i += 2;
	// IP
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			wd->ipPar[WF_IP_ADDR], MW_SSID_MAXLEN);
	i++;
	// MASK
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			wd->ipPar[WF_IP_MASK], MW_SSID_MAXLEN);
	i++;
	// GATEWAY
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			wd->ipPar[WF_IP_GATEWAY], MW_SSID_MAXLEN);
	i++;
	// DNS1
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			wd->ipPar[WF_IP_DNS1], MW_SSID_MAXLEN);
	i++;
	// DNS2
	item[i].caption.length += MenuStrCpy(item[i].caption.string + 9,
			wd->ipPar[WF_IP_DNS2], MW_SSID_MAXLEN);

	return 0;
}

/// Show IP configuration items
static void MenuIpConfShow(MenuItem* item) {
	int i;

    for (i = MENU_NET_CONF_IP; i < MENU_NET_CONF_OK; i++) {
    	item[i].hide = FALSE;
    	item[i].selectable = TRUE;
    }
}

/// Hide IP configuration items
static void MenuIpConfHide(MenuItem* item) {
	int i;

    for (i = MENU_NET_CONF_IP; i < MENU_NET_CONF_OK; i++) {
    	item[i].hide = TRUE;
    	item[i].selectable = FALSE;
    }
}

#ifdef _FAKE_WIFI
/// \brief Fake scan data. Field order is:
/// - Auth
/// - channel
/// - str
/// - ssidLen
/// - ssid
static const char fakeScanData[] = {
	0, 1, 25, 3, 'A', 'P', '1',
	0, 2, 50, 3, 'A', 'P', '2',
	0, 3, 75, 3, 'A', 'P', '3'
};
#define _FAKE_WIFI_APS		3
#endif

/// Scan for APs
int MenuWiFiScan(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;
	MenuString str;
	char *apData;
	uint16_t pos;
	MwApData apd;
	int16_t dataLen;
	uint8_t i;
	uint8_t aps;

	// Clear previously drawn items, and print the WiFi scan message
	str.string = (char*)strScan;
	str.length = sizeof(strScan) - 1;
	MenuMessage(str, 0);

	// Disconnect from network
	MwApLeave();
#ifdef _FAKE_WIFI
	aps = _FAKE_WIFI_APS;
	apData = (char*)fakeScanData;
	dataLen = sizeof(fakeScanData);
#else //_FAKE_WIFI
	// Scan networks
	if ((dataLen = MwApScan(&apData, &aps)) == MW_ERROR) {
		str.string = (char*)strScanFail;	
		str.length = sizeof(strScanFail) - 1;
		MenuMessage(str, 120);
		return FALSE;
	}
#endif //_FAKE_WIFI
	str.string = (char*)"SCAN OK!!!";
	str.length = 10;
	MenuMessage(str, 30);
	// Scan complete, fill in MenuItem information.
	// Allocate memory for the item descriptors
	item = MpAlloc(aps * sizeof(MenuItem));
	memset(item, 0, aps * sizeof(MenuItem));
	pos = 0;
	for (i = 0; ((pos = MwApFillNext(apData, pos, &apd,
			dataLen)) > 0) && (i < WF_MENU_MAX_DYN_ITEMS); i++) {
		// Fill a dynEntry. Format is: signal_strength(3) auth(4) SSID(29)
		/// \todo check we do not overflow string buffer
		// Allocate memory for SSID plus strength (4) plus security (5)
		// plus null termination
		item[i].caption.string = MpAlloc(apd.ssidLen + 9 + 1);
		// Copy strength
		item[i].caption.length = Byte2UnsStr(apd.str, item[i].caption.string);
		// Copy security type
		item[i].caption.string[item[i].caption.length++] = ' ';
		if ((apd.auth < MW_AUTH_OPEN) || (apd.auth > MW_AUTH_UNKNOWN))
			apd.auth = MW_AUTH_UNKNOWN;
		item[i].caption.length += MenuStrCpy(item[i].caption.string +
				item[i].caption.length, security[(int)apd.auth], 4);
		// Copy SSID
		item[i].caption.string[item[i].caption.length++] = ' ';
		memcpy(item[i].caption.string + item[i].caption.length, apd.ssid,
				apd.ssidLen);
		item[i].caption.length += apd.ssidLen;
		item[i].caption.string[item[i].caption.length] = '\0';

		item[i].next = (void*)&apPass;
		item[i].cb = NULL;
		item[i].selectable = 1;
		item[i].alt_color = 0;
	}
	// Store scanned data, that will be loaded by the entry callback
	wd->item = item;
	wd->aps = aps;
	return TRUE;
}

/****************************************************************************
 * Network configuration entry menu
 *
 * Title: NETWORK CONFIGURATION
 *
 * Screen with SSID, password and IP configuration entry fields.
 ****************************************************************************/
int MenuConfSetActive(void *m) {
	MenuString str;
	UNUSED_PARAM(m);

	// Set default config to selected entry
	if (MW_OK != MwDefApCfg(wd->selConfig)) {
		str.string = (char*)strFailed;
		str.length = sizeof(strFailed) - 1;
	} else {
		str.string = (char*)strDone;
		str.length = sizeof(strDone) - 1;
	}

	// Notify user and go back one menu level
	MenuMessage(str, 30);
	MenuUnlink();
	MenuBack();

	// Return false to avoid loading next entry
	return FALSE;
}

/// \brief Menu configuration data entry callback. Fills menu entries with
/// SSID information found.
int MenuConfDataEntryCb(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;
	char *ssid, *pass;
	MwIpCfg *ip;
	uint8_t error = FALSE;

	// Get the SSID and password
	if ((MwApCfgGet(wd->selConfig, &ssid, &pass) != MW_OK) ||
			(*ssid == '\0')) {
        wd->ssid.length    =   0;
        wd->ssid.string[0] = '\0';
        wd->pass.length    =   0;
        wd->pass.string[0] = '\0';
        error = TRUE;
    } else {
        wd->ssid.length = MenuStrCpy(wd->ssid.string, ssid, MW_SSID_MAXLEN);
        wd->ssid.string[MW_SSID_MAXLEN] = '\0';
        wd->pass.length = MenuStrCpy(wd->pass.string, pass, MW_PASS_MAXLEN);
        wd->pass.string[MW_PASS_MAXLEN] = '\0';
    }

	error = error || (MW_OK != MwIpCfgGet(wd->selConfig, &ip));
	// If error loading configuration, or configuration set to DHCP, set
	// label to automatic configuration. Else set to manual and display
	// loaded configuration
	if (error || !ip->addr) {
		item[MENU_NET_CONF_IP_TYPE].caption.string = (char*)strIpAuto;
		item[MENU_NET_CONF_IP_TYPE].caption.length = sizeof(strIpAuto) - 1;
	} else {
		item[MENU_NET_CONF_IP_TYPE].caption.string = (char*)strIpManual;
		item[MENU_NET_CONF_IP_TYPE].caption.length = sizeof(strIpManual) - 1;
	}
    // If error, set default IP configuration
    if (error) {
        MenuFillDefaultNetPar();
        MenuIpConfHide(item);
    	MenuIpConfFillDhcp(item);
    } else {
		MenuFillNetPar(ip);
    	MenuIpConfFillDhcp(item);
        MenuIpConfShow(item);
	}
	return 0;
}

/// Network parameters menu
const MenuItem confNetPar[] = {
	{
		// Editable SSID
		MENU_ESTR(strSsid, 3 + 32 + 1),
		(void*)&ipSsidOsk,			// Next
		NULL,						// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		// Editable PASS
		MENU_ESTR(strPass, 3 + 32 + 1),
		NULL,						// Next
		NULL,						// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_STR(strStartScan),
		(void*)&confSsidSelEntry,	// Next
		MenuWiFiScan,				// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		// DHCP/MANUAL IP configuration
		MENU_EESTR(0),
		NULL,						// Next
		MenuIpConfToggle,			// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		// Editable IP
		MENU_ESTR(strIp, 9 + WF_IP_MAXLEN),
		(void*)&ipAddrOsk,			// Next
		NULL,						// Callback
		{{0, 0, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable netmask
		MENU_ESTR(strMask, 9 + WF_IP_MAXLEN),
		(void*)&ipMaskOsk,			// Next
		NULL,						// Callback
		{{0, 0, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable gateway
		MENU_ESTR(strGw, 9 + WF_IP_MAXLEN),
		(void*)&ipGwOsk,			// Next
		NULL,						// Callback
		{{0, 0, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable DNS1
		MENU_ESTR(strDns1, 9 + WF_IP_MAXLEN),
		(void*)&ipDns1Osk,			// Next
		NULL,						// Next
		{{0, 0, 1}}					// Selectable, alt_color, hide
	}, {
		// Editable DNS2
		MENU_ESTR(strDns2, 9 + WF_IP_MAXLEN),
		(void*)&ipDns2Osk,			// Next
		NULL,						// Callback
		{{0, 0, 1}}					// Selectable, alt_color, hide
	}, {
		MENU_STR(strSave),			// OK
		NULL,						// Next
		NULL,						// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}
};


/// Network configuration entry data
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

/// Toggle IP configuration between auto(DHCP) and manual
int MenuIpConfToggle(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;

	if (item[MENU_NET_CONF_IP_TYPE].caption.string == strIpAuto) {
		// Toggle IP configuration to manual
		item[MENU_NET_CONF_IP_TYPE].caption.string = (char*)strIpManual;
		// Fill in IP configuration data
		MenuIpConfFillDhcp(item);
        MenuIpConfShow(item);
	} else {
		// Toggle IP configuration to auto
		item[MENU_NET_CONF_IP_TYPE].caption.string = (char*)strIpAuto;
		// Hide IP related configuration entries
        MenuIpConfHide(item);
	}

	// Redraw page
	MenuDrawItemPage(0);

	return TRUE;
}

int MenuSetDefaultConf(void *m) {
	Menu *md = (Menu*)m;
	(void)md;

	/// \todo Set wd->selConfig as default
	

	return 0;
}

/****************************************************************************
 * Configuration items
 *
 * Title: CONFIGURATION SLOT
 *
 * Start game (currently disabled) and configuration entries.
 ****************************************************************************/
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

/// Configuration slot selection menu entry
const MenuEntry confEntry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("CONFIGURATION SLOT"),	// Title
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



/****************************************************************************
 * Root menu.
 *
 * Title: WFLASH BOOTLOADER
 *
 * Start game (currently disabled) and configuration entries.
 ****************************************************************************/
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

/// Module initalization
void WfMenuInit(MenuString statStr) {
    // Allocate local module data and initialize menu strings
	MenuInit(&rootMenu, statStr);
	wd = MpAlloc(sizeof(WfMenuData));
    memset(wd, 0, sizeof(WfMenuData));
    wd->ssid.string = wd->ssidBuf;
    wd->pass.string = wd->passBuf;
}

