// PRIVATE EYE (VAUGHAN)

#include "wf-menu.h"
#include "util.h"
#include "vdp.h"
#include "mw/megawifi.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/// When defined, fake data will be used for some WiFi operations
#define _FAKE_WIFI

/// Maximum length of an IP string, including the null termination
#define WF_IP_MAXLEN	16

/// Maximum number of dynamic menu items
#define WF_MENU_MAX_DYN_ITEMS 		20

/// Maximum length of a DNS server (not including null termination)
#define WF_NTP_SERV_MAXLEN          32

/// Macro to compute the number of items of a MenuItem variable
#define MENU_NITEMS(menuItems)	(sizeof(menuItems)/sizeof(MenuItem))

/// Macro to fill on MenuEntry structures: nItems, spacing, entPerPage, pages
#define MENU_ENTRY_NUMS(numItems, spacing)	(numItems),(spacing), \
	MENU_ITEM_NLINES/(spacing),((numItems) - 1)/(MENU_ITEM_NLINES/(spacing))

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

static const char stdContext[] = "[A]ccept, [B]ack";
static const char strScanContext[] = "[A]ccept, [B]ack, [C]: Rescan";
static const char oskQwertyContext[] = "A-OK, B-Del, C-Caps, S-Done";
static const char oskNumIpContext[] = "A-OK, B-Del, S-Done";
static const char strEmptyText[] = "<EMPTY>";
static const char strSsid[] = "SSID:    ";
static const char strPass[] = "PASS:    ";
static const char strIp[] =   "ADDRESS: ";
static const char strMask[] = "MASK:    ";
static const char strGw[] =   "GATEWAY: ";
static const char strDns1[] = "DNS1:    ";
static const char strDns2[] = "DNS2:    ";
//static const char strAct[] =  "SET AS ACTIVE";
static const char strIpAuto[] = "IP CONFIG: AUTOMATIC";
static const char strIpManual[] = "IP CONFIG: MANUAL";
static const char strScan[] = "SCAN IN PROGRESS, PLEASE WAIT...";
//static const char strScanFail[] = "SCAN FAILED!";
//static const char strCfgFail[] = "CONFIGURATION FAILED!";
static const char strTimeConfig[] = "TIME CONFIGURATION";
static const char strStartScan[] = "SCAN...";
static const char strSave[] = "SAVE!";
//static const char strOk[] = "OK";
static const char strDone[] = "DONE!";
static const char strFailed[] = "FAILED!";
static const char strWrongIp[] = "WRONG IP!";
static const char strErrSsid[] = "No valid SSID set!";
static const char strErrRange[] = "Invalid input range!";
static const char strNtpSrvInput[] = "Enter NTP server address";
static const char strErrApCfgSet[] = "Error setting SSID/password!";
static const char strErrIpCfgSet[] = "Error setting IP configuration!";
static const char strDsOff[] = "DAYLIGTH SAVINGS: OFF";
static const char strDsOn[] = "DAYLIGTH SAVINGS: ON";
const char strNetParLen[WF_NET_CFG_PARAMS] = {
	4, 4, 7, 4, 7, 4, 4
};

// Default network configuration
static const char strDefIp[] =     "192.168.1.64";
static const char strDefMask[] =   "255.255.255.0";
static const char strDefGw[] =     "192.168.1.1";
static const char strDefDns1[] =   "192.168.1.1";
static const char strDefDns2[] =   "8.8.8.8";
static const char strDefNtp1[] = "0.pool.ntp.org";
static const char strDefNtp2[] = "1.pool.ntp.org";
static const char strDefNtp3[] = "2.pool.ntp.org";


/// Time configuration data structure
typedef struct {
    /// Time update interval (seconds). Minimum is 15 seconds
    uint16_t delay_sec;
    /// Timezone (-11 to 13)
    int8_t tz;
    /// Daylight saving (0 or 1)
    int8_t dst;
    /// NTP servers buffer
    char ntpServ[(WF_NTP_SERV_MAXLEN + 1) * 3];
} WfTimeConf;

/// Module global menu data structure
typedef struct {
    /// Time configuration data structure
    WfTimeConf *tc;
	/// Menu items of the scanned WiFi APs.
	MenuItem *item;
	/// Data of the IP configuration entries being edited
	char ipPar[WF_IP_CFG_PARAMS][WF_IP_MAXLEN];
	/// Selected network configuration item (from 0 to 2)
	uint8_t selConfig;
	/// Number of scanned APs
	uint8_t aps;
    /// SSID buffer
	char ssid[33];
	/// Password being edited
	char pass[65];
} WfMenuData;

// Private prototypes
static int MenuReset(void *m);
static int MenuWiFiTest(void *m);
int MenuWiFiScan(void *m);
int MenuConfSsidLoad(void *m);
int MenuConfEntrySet(void *m);
int MenuConfEntryCb(void* m);
static int MenuIpOskEnter(void *m);
static int MenuIpOskExit(void *m);
uint16_t MenuIpConfPrintNetPar(MenuItem* item);
//static int MenuApPassExitCb(void *m);
static int MenuSsidSet(void *m);
int MenuSsidCopySelected(void *m);
int MenuIpConfToggle(void *m);
int MenuConfDataEntryCb(void *m);
void MenuFillNetPar(MwIpCfg *ip);
void MenuFillDefaultNetPar(void);
static int MenuNtpEntryCb(void *m);
static void MenuIpConfShow(MenuItem* item);
static void MenuIpConfHide(MenuItem* item);
static int MenuNetSaveCb(void *m);
static int MenuDsToggle(void *m);
static int MenuNtpOskEntry(void *m);
static int MenuNtpOskExit(void *m);

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
 * WiFi configuration test log.
 *
 * Title: TESTING WIFI CONNECTION
 *
 * Shows the WiFi test log
 ****************************************************************************/
/// WiFi test log items
const MenuItem testLogItem[] = { {
		MENU_EESTR(0),	       		// Caption
		NULL,						// Next: none
		NULL,			            // Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}
};

/// WiFi test log entry data
const MenuEntry wifiTestLogEntry = {
	MENU_TYPE_ITEM,					// Menu type
	4,								// Margin
	MENU_STR("TESTING WIFI CONNECTION"),	// Title
	MENU_STR(stdContext),			// Left context
	MenuWiFiTest,					// entry callback
	NULL,							// exit callback
	NULL,							// cBut callback
	.mItem = {
		// rootItem, nItems, spacing, enPerPage, pages
		MENU_ENTRY_ITEM(testLogItem, 1),
		{MENU_H_ALIGN_LEFT}	    	// align
	}
};

/// Tests WiFi connectifity by connecting to the AP and trying to
/// synchronize the date and time.
static int MenuWiFiTest(void *m) {
    UNUSED_PARAM(m);

    // Connect to AP
    
    // Try to set date and time

    return FALSE;
}

/****************************************************************************
 * WiFi configuration test menu.
 *
 * Title: CONFIGURATION SAVED
 *
 * WiFi configuration saved, allow testing it.
 ****************************************************************************/
/// WiFi test menu item data
const MenuItem testItem[] = { {
		MENU_STR("DONE!"),			// Caption
		NULL,						// Next: none
		(void*)&MenuReset,			// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_STR("TEST"),
		(void*)&wifiTestLogEntry,
		NULL,
		{{1, 0, 0}}
	}
};

/// WiFi test menu entry data
const MenuEntry wifiTestEntry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("WFLASH BOOTLOADER"),	// Title
	MENU_STR(stdContext),			// Left context
	NULL,							// entry callback
	NULL,							// exit callback
	NULL,							// cBut callback
	.mItem = {
		// rootItem, nItems, spacing, enPerPage, pages
		MENU_ENTRY_ITEM(testItem, 3),
		{MENU_H_ALIGN_CENTER}		// align
	}
};

/// Go back to the root menu. Do not call this from the root menu itself!
static int MenuReset(void *m) {
	Menu *md = (Menu*)m;
    MenuEntity *me;

    me = md->me->prev;

    while (me->prev != NULL) {
        me = me->prev;
        MenuUnlink();
    }
    MenuBack(TRUE);


    return FALSE;
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

const MenuEntry ipPassOsk = {
	MENU_TYPE_OSK_QWERTY,		    // QWERTY keyboard
	8,								// Margin
	MENU_STR("PASSWORD"),		    // Title
	MENU_STR(oskQwertyContext),		// Left context
	MenuIpOskEnter,					// cbEntry
	MenuIpOskExit, 				    // cbExit
	.keyb = {
		MENU_STR("Enter access point password:"),
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
            md->me->mEntry.keyb.fieldData.string = wd->ssid;
            md->me->mEntry.keyb.fieldData.length = strlen(wd->ssid);
            break;

        case MENU_NET_CONF_PASS:
            // Password data is always set as empty
            md->me->mEntry.keyb.fieldData.string[0] = '\0';
            md->me->mEntry.keyb.fieldData.length = 0;
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

/// Exit network configuration on-screen keyboard and save entered data
static int MenuIpOskExit(void *m) {
	Menu *md = (Menu*)m;
    MenuString msg;
    char *dst = NULL;
    int sel;

    // Get selected option
    sel = md->me->prev->selItem;

    // Validate selected text
    switch (sel) {
        case MENU_NET_CONF_SSID:
            dst = wd->ssid;
            break;

        case MENU_NET_CONF_PASS:
            dst = wd->pass;
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
            dst = wd->ipPar[sel - MENU_NET_CONF_IP];
            break;

        case MENU_NET_CONF_OK:
        case MENU_NET_CONF_NUM_ENTRIES:
        default:
            break;
    }
    // Update menu items
    if (dst) strcpy(dst, md->strBuf);
    MenuIpConfPrintNetPar(md->me->prev->mEntry.mItem.item);
    
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
	MenuConfSsidLoad,				// entry callback
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

int MenuConfSsidLoad(void *m) {
	Menu *md = (Menu*)m;
	MenuItemEntry *mie = &md->me->mEntry.mItem;

	// Fill scanned item data
	mie->item = wd->item;
	mie->nItems = wd->aps;
	mie->pages = (mie->nItems - 1) / mie->entPerPage;
//	if (0 != (mie->nItems % mie->entPerPage)) mie->entPerPage--;
	return TRUE;
}

/// Copy selected SSID entry to internal wd structure
int MenuSsidCopySelected(void *m) {
    int i;
    uint16_t len;
	Menu *md = (Menu*)m;
	MenuItem *item = &md->me->mEntry.mItem.item[md->me->selItem];

    // Copy SSID data
    len = item->caption.length - 8;
    for (i = 0; i < len; i++)
        wd->ssid[i] = item->caption.string[8 + i];
    wd->ssid[i] = '\0';

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

/// Fills network configuration entries, using data in wd structure.
uint16_t MenuIpConfPrintNetPar(MenuItem* item) {
	int i = 0;
    int j;
    int len;

    // SSID and PASS
    if (!wd->ssid[0]) {
		item[i].caption.length = 9 + MenuStrCpy(item[i].caption.string + 9,
				strEmptyText, 0);
	} else {
		item[i].caption.length = 9 + MenuStrCpy(item[i].caption.string + 9,
				wd->ssid, MW_SSID_MAXLEN);
    }
	i++;
    if (!wd->pass[0]) {
		item[i].caption.length = 9 + MenuStrCpy(item[i].caption.string + 9,
				strEmptyText, 0);
    } else {
        // Password copied as asterisks
        len = MIN(strlen(wd->pass), 20);
        for (j = 9; j < (len + 9); j++) item[i].caption.string[j] = '*';
        item[i].caption.string[j] = '\0';
        item[i].caption.length = j;
    }
	i++;

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
	0, 3, 75, 3, 'A', 'P', '3',
	0, 2, 60, 3, 'A', 'P', '4',
	0, 2, 70, 3, 'A', 'P', '5',
	0, 2, 80, 3, 'A', 'P', '6',
	0, 2, 90, 3, 'A', 'P', '7',
	0, 2, 91, 3, 'A', 'P', '8',
	0, 2, 92, 3, 'A', 'P', '9',
	0, 2, 53, 4, 'A', 'P', '1', '0',
	0, 2, 58, 4, 'A', 'P', '1', '1',
	0, 2, 64, 4, 'A', 'P', '1', '2',
};
#define _FAKE_WIFI_APS	12
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

		item[i].next = NULL;
		item[i].cb = (void*)&MenuSsidSet;
		item[i].selectable = 1;
		item[i].alt_color = 0;
	}
	// Store scanned data, that will be loaded by the entry callback
	wd->item = item;
	wd->aps = aps;
	return TRUE;
}

/// Set SSID previously selected in the scan menu
static int MenuSsidSet(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = &md->me->mEntry.mItem.item[md->me->selItem +
        md->me->selPage * md->me->mEntry.mItem.entPerPage];
    MenuString str;
    int len, i;

    // Store chosen SSID data
    len = item->caption.length - 8;
    for (i = 0; i < len; i++)
        wd->ssid[i] = item->caption.string[8 + i];
    wd->ssid[i] = '\0';

    // Update SSID label and go back a level
    MenuIpConfPrintNetPar(md->me->prev->mEntry.mItem.item);
    str.string = (char*)strDone;
    str.length = sizeof(strDone) - 1;
    MenuMessage(str, 30);

	MenuBack(FALSE);

	return FALSE;
}

/****************************************************************************
 * Network configuration entry menu
 *
 * Title: NETWORK CONFIGURATION
 *
 * Screen with SSID, password and IP configuration entry fields.
 ****************************************************************************/
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
		(void*)&ipPassOsk,			// Next
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
		(void*)&wifiTestEntry,		// Next
		&MenuNetSaveCb,		        // Callback
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
	MenuBack(TRUE);

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
        wd->ssid[0] = '\0';
        wd->pass[0] = '\0';
        error = TRUE;
    } else {
        strcpy(wd->ssid, ssid);
        strcpy(wd->pass, pass);
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
    	MenuIpConfPrintNetPar(item);
    } else {
		MenuFillNetPar(ip);
    	MenuIpConfPrintNetPar(item);
        MenuIpConfShow(item);
	}
	return 0;
}

/// Toggle IP configuration between auto (DHCP) and manual
int MenuIpConfToggle(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;

	if (item[MENU_NET_CONF_IP_TYPE].caption.string == strIpAuto) {
		// Toggle IP configuration to manual
		item[MENU_NET_CONF_IP_TYPE].caption.string = (char*)strIpManual;
		// Fill in IP configuration data
		MenuIpConfPrintNetPar(item);
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

/// Save network configuration (ssid, password, ip config)
static int MenuNetSaveCb(void *m) {
	Menu *md = (Menu*)m;
    MwIpCfg ip;
    MenuString str;

    // Sanity check, test if SSID provided
    if (!wd->ssid[0]) {
        str.string = (char*)strErrSsid;
        str.length = sizeof(strErrSsid) - 1;
        MenuMessage(str, 60);
        return FALSE;
    }
    // It is assumed that IP configuration is valid since a default valid one
    // is provided and edited configuration is checked before accepted. Also
    // it is not mandatory to provide a password (to support open APs).
   
    // Try setting AP configuration
    if (MW_OK != MwApCfgSet(wd->selConfig, wd->ssid, wd->pass)) {
        str.string = (char*)strErrApCfgSet;
        str.length = sizeof(strErrApCfgSet) - 1;
        MenuMessage(str, 60);
//        return FALSE;
        return TRUE;
    }
    // If IP configuration set to automatic, set IP parameters to 0.
    // else convert IP strings to binary data and set IP configuration.
    if (md->me->mEntry.mItem.item[MENU_NET_CONF_IP_TYPE].caption.string ==
            strIpAuto) {
        memset(wd->ipPar, 0, sizeof(wd->ipPar));
    } else {
        ip.addr = MenuIpStr2Bin(wd->ipPar[WF_IP_ADDR]);
        ip.mask = MenuIpStr2Bin(wd->ipPar[WF_IP_MASK]);
        ip.gateway = MenuIpStr2Bin(wd->ipPar[WF_IP_GATEWAY]);
        ip.dns1 = MenuIpStr2Bin(wd->ipPar[WF_IP_DNS1]);
        ip.dns2 = MenuIpStr2Bin(wd->ipPar[WF_IP_DNS2]);
    }
    
    if (MW_OK != MwIpCfgSet(wd->selConfig, &ip)) {
        str.string = (char*)strErrIpCfgSet;
        str.length = sizeof(strErrIpCfgSet) - 1;
        MenuMessage(str, 60);
//        return FALSE;
        return TRUE;
    }
    str.string = (char*)strDone;
    str.length = sizeof(strDone) - 1;

    // No transition to the next menu
//    MenuBack(TRUE);
    return TRUE;
}

/****************************************************************************
 * NTP configuration
 *
 * Title: TIME CONFIGURATION
 *
 * NTP configuration (servers, update interval)
 ****************************************************************************/
/// NTP server URL entry
const MenuEntry ntpSrvOsk = {
	MENU_TYPE_OSK_QWERTY,		    // QWERTY keyboard
	8,								// Margin
	MENU_STR("NTP SERVER"),	        // Title
	MENU_STR(oskQwertyContext),		// Left context
	MenuNtpOskEntry,				// cbEntry
	MenuNtpOskExit,         	    // cbExit
	.keyb = {
		MENU_STR(strNtpSrvInput),
		MENU_EESTR(0),
		33,
	    33
	}
};

/// Timezone keyboard entry
const MenuEntry ntpTzOsk = {
	MENU_TYPE_OSK_NUMERIC_NEG,	    // Numeric keyboard
	4,								// Margin
	MENU_STR("TIME ZONE"),	        // Title
	MENU_STR(oskQwertyContext),		// Left context
	MenuNtpOskEntry,	        	// cbEntry
	MenuNtpOskExit, 	            // cbExit
	.keyb = {
		MENU_STR("Time zone offset (-11 to 13):"),
		MENU_EESTR(0),
		3,
	    3
	}
};

/// Update interval keyboard entry
const MenuEntry ntpIntervalOsk = {
	MENU_TYPE_OSK_NUMERIC,		    // Numeric keyboard
	4,								// Margin
	MENU_STR("UPDATE INTERVAL"),    // Title
	MENU_STR(oskQwertyContext),		// Left context
	MenuNtpOskEntry,	        	// cbEntry
	MenuNtpOskExit, 	            // cbExit
	.keyb = {
		MENU_STR("Update interval (15+ seconds):"),
		MENU_EESTR(0),
		6,
	    6
	}
};

/// Constants for the time option menu strings
enum {
    MENU_TIMECFG_NTPSRV1 = 1,   ///< NTP server 1 string
    MENU_TIMECFG_NTPSRV2,       ///< NTP server 2 string
    MENU_TIMECFG_NTPSRV3,       ///< NTP server 3 string
    MENU_TIMECFG_TZ = 6,        ///< Time zone
    MENU_TIMECFG_INTERVAL = 11  ///< Update interval
};

/// Configuration items for the time options
const MenuItem ntpConfItem[] = {
	{
        MENU_STR("SNTP SERVERS:"),
        NULL,
        NULL,
        {{0, 1, 0}}
    }, {
		MENU_EESTR(WF_NTP_SERV_MAXLEN),
		(void*)&ntpSrvOsk,		    // Next
		NULL,	            		// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_EESTR(WF_NTP_SERV_MAXLEN),
		(void*)&ntpSrvOsk,		    // Next
		NULL,	            		// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_EESTR(WF_NTP_SERV_MAXLEN),
		(void*)&ntpSrvOsk,		    // Next
		NULL,	            		// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_EESTR(0),              // Empty entry
		NULL,
		NULL,
		{{0, 0, 1}}
	}, {
        MENU_STR("TIME ZONE:"),
        NULL,
        NULL,
        {{0, 1, 0}}
    }, {
		MENU_EESTR(3),
		(void*)&ntpTzOsk,		    // Next
		NULL,	            		// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_EESTR(0),              // Empty entry
		NULL,
		NULL,
		{{0, 0, 1}}
	}, {
        MENU_STR(strDsOff),
        NULL,                       // Next: empty
        MenuDsToggle,
        {{1, 0, 0}}
    }, {
		MENU_EESTR(0),              // Empty entry
		NULL,
		NULL,
		{{0, 0, 1}}
	}, {
        MENU_STR("UPDATE INTERVAL:"),
        NULL,
        NULL,
        {{0, 1, 0}}
    }, {
		MENU_EESTR(6),
		(void*)&ntpIntervalOsk,	    // Next
		NULL,	            		// Callback
		{{1, 0, 0}}					// Selectable, alt_color, hide
	}, {
		MENU_EESTR(0),              // Empty entry
		NULL,
		NULL,
		{{0, 0, 1}}
	}, {
		MENU_EESTR(0),              // Empty entry
		NULL,
		NULL,
		{{0, 0, 1}}
	}, {
        MENU_STR(strSave),
        NULL,
        NULL,
        {{1, 0, 0}}
	}
};

/// Configuration slot selection menu entry
const MenuEntry ntpConfEntry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR(strTimeConfig),	    // Title
	MENU_STR(stdContext),			// Left context
	MenuNtpEntryCb, 				// entry
	NULL,							// exit
	NULL,							// cBut callback
	.mItem = {
		// rootItem, nItems, spacing, enPerPage, pages
		MENU_ENTRY_ITEM(ntpConfItem, 1),
		{MENU_H_ALIGN_LEFT}			// align
	}
};

static int MenuNtpOskEntry(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->prev->mEntry.mItem.item;
    int selItem = md->me->prev->selItem;

    switch (selItem) {
        case MENU_TIMECFG_NTPSRV1:
        case MENU_TIMECFG_NTPSRV2:
        case MENU_TIMECFG_NTPSRV3:
        case MENU_TIMECFG_TZ:
        case MENU_TIMECFG_INTERVAL:
            md->me->mEntry.keyb.fieldData = item[selItem].caption;
            break;
    }

    return TRUE;
}

static int MenuNtpOskExit(void *m) {
	Menu *md = (Menu*)m;
    MenuString str;
    MenuItem *item = md->me->prev->mEntry.mItem.item;
    int selItem = md->me->prev->selItem;
    long num;

    switch (selItem) {
        case MENU_TIMECFG_NTPSRV1:
        case MENU_TIMECFG_NTPSRV2:
        case MENU_TIMECFG_NTPSRV3:
        case MENU_TIMECFG_INTERVAL:
            // Check minimum value of 15 seconds
            num = atol(md->strBuf);
            if (num < 15) {
                str.string = (char*)strErrRange;
                str.length = sizeof(strErrRange) - 1;
                MenuMessage(str, 60);
                return FALSE;
            } else {
                md->str.length = Long2Str(num, md->str.string,
                        MENU_STR_MAX_LEN + 1, 0, 0);
                item[selItem].caption.length = md->str.length;
            }
            break;

        case MENU_TIMECFG_TZ:
            // Check timezone is between -11 and +13
            num = atol(md->strBuf);
            if ((num < -11) || (num > 13)) {
                str.string = (char*)strErrRange;
                str.length = sizeof(strErrRange) - 1;
                MenuMessage(str, 60);
                return FALSE;
            } else {
                md->str.length = Long2Str(num, md->str.string,
                        MENU_STR_MAX_LEN + 1, 0, 0);
                item[selItem].caption.length = md->str.length;
            }
            break;
    }

    return TRUE;
}

/// Callback for the time configuration menu entry
int MenuNtpEntryCb(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;

    // Allocate memory for the time configuration structure
    wd->tc = MpAlloc(sizeof(WfTimeConf));
    memset(wd->tc, 0, sizeof(WfTimeConf));
    // Load configuration. Currently only default configuration is supported.
    item[1].caption.length = MenuStrCpy(item[1].caption.string, strDefNtp1,
            WF_NTP_SERV_MAXLEN);
    item[2].caption.length = MenuStrCpy(item[2].caption.string, strDefNtp2,
            WF_NTP_SERV_MAXLEN);
    item[3].caption.length = MenuStrCpy(item[3].caption.string, strDefNtp3,
            WF_NTP_SERV_MAXLEN);
    item[6].caption.string[0] =  '0';
    item[6].caption.string[1] = '\0';
    item[6].caption.length = 1;
    item[11].caption.length = MenuStrCpy(item[11].caption.string, "300", 0);
//    wd->tc->tz = 0;
//    wd->tc->dst = 0;
    wd->tc->delay_sec = 300;

    // Copy NTP data to destination strings
    
    return TRUE;
}

/// Toggle daylight saving option
static int MenuDsToggle(void *m) {
	Menu *md = (Menu*)m;
	MenuItem *item = md->me->mEntry.mItem.item;
    int selItem = md->me->selItem;

    if (item[selItem].caption.string == strDsOff) {
        item[selItem].caption.string = (char*)strDsOn;
        item[selItem].caption.length = sizeof(strDsOn) - 1;
    } else {
        item[selItem].caption.string = (char*)strDsOff;
        item[selItem].caption.length = sizeof(strDsOff) - 1;
    }
    MenuDrawItemPage(0);

    // No transition to next entry
    return FALSE;
}

/****************************************************************************
 * Configuration items
 *
 * Title: WIFI CONFIGURATION
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
	}, {
        MENU_EESTR(0),
        NULL,
        NULL,
        {{0, 0, 1}}
    }, {
        MENU_STR(strTimeConfig),
        (void*)&ntpConfEntry,
        NULL,
        {{1, 0, 0}}
    }
};

/// Configuration slot selection menu entry
const MenuEntry confEntry = {
	MENU_TYPE_ITEM,					// Menu type
	1,								// Margin
	MENU_STR("TIME CONFIGURATION"),	// Title
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
}

