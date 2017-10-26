/************************************************************************//**
 * \brief Wireless Flash memory manager for MegaWiFi cartridges. Allows
 * remotely programming MegaWiFi catridges. It also allows performing some
 * flash memory management routienes, like erasing the cartridge, reading
 * data, reading flash identifiers, etc.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2017
 * \defgroup wflash main
 * \{
 ****************************************************************************/
#include "vdp.h"
#include "sysfsm.h"
#include "util.h"
#include "cmds.h"
#include "gamepad.h"
#include "mw/megawifi.h"
#include "wf-menu.h"
#include "menu.h"
#include <string.h>

/// Length of the wflash buffer
#define WFLASH_BUFLEN	WF_MAX_DATALEN

/// TCP port to use (set to Megadrive release year ;-)
#define WFLASH_PORT 	1989

/// Command buffer
static char cmdBuf[WFLASH_BUFLEN];

/// Waits until start is pressed (to boot flashed ROM) or a client connects
int WaitUserInteraction(void) {
	MwSockStat s;
	int8_t loop;

	// Poll for connections each 100 ms
	do {
		// If START button is pressed, boot the flashed ROM
		if (!(GpRead() & GP_START_MASK)) SfBoot(SF_ENTRY_POINT_ADDR);
		for (loop = 5; loop > 0; loop--) VdpVBlankWait();
		s = MwSockStatGet(WF_CHANNEL);
	} while (s != (MwSockStat)MW_ERROR && s == MW_SOCK_TCP_LISTEN);
	return (MW_SOCK_TCP_EST == s)?0:-1;
}

/// MegaWiFi initialization
int MegaWifiInit(void){
	uint8_t verMajor, verMinor, loop;
	char *variant;
	char strBuf[20];
	MenuString stat;

	// Initialize MegaWiFi
	MwInit(cmdBuf, WFLASH_BUFLEN);
	// Wait a bit and take module out of resest
	VdpVBlankWait();
	VdpVBlankWait();
	MwModuleStart();
	for (loop = 119; loop > 0; loop--) VdpVBlankWait();
	UartResetFifos();
	
	stat.string = strBuf;
	// Try detecting MegaWiFi module by requesting version
	if (MwVersionGet(&verMajor, &verMinor, &variant) != MW_OK) {
		// Set menu status string to show Megawifi was not found
		stat.length = MenuStrCpy(strBuf, "MegaWiFi?", 20 - 1);
		strBuf[20 - 1] = '\0';	// Ensure null termination
		MenuStatStrSet(stat);
		return -1;
	}
	// Set menu status string to show readed version
	stat.length = MenuStrCpy(strBuf, "MW RTOS ", 0);
	stat.length += Byte2UnsStr(verMajor, strBuf + stat.length);
	strBuf[stat.length++] = '.';
	stat.length += Byte2UnsStr(verMinor, strBuf + stat.length);
	MenuStatStrSet(stat);
	return 0;
}

/// Global initialization
void Init(void) {
	const MenuString str = MENU_STR("Detecting WiFi...");
	// Initialize VDP
	VdpInit();
	// Initialize gamepad
	GpInit();
	// Initialize menu system
	WfMenuInit(str);
	// Initialize MegaWifi
	MegaWifiInit();
}

/// Entry point
int main(void) {
	MwMsgSysStat *stat;
	MwIpCfg *ip;
	MwState prevStat = MW_ST_INIT;
	char statBuf[16];
	MenuString statStr;
	uint8_t pad;

	// Initialization
	Init();
	statStr.string = statBuf;
	while (1) {
		/// \todo 1. Parse communications events.
		// 2. Wait for VBlank.
		VdpVBlankWait();
		// 3. Read controller and perform non-menu related actions.
		pad = GpPressed();
		// 4. If pad pressed, perform menu related actions. Else check
		//    connection status.
		if (0xFF != pad) MenuButtonAction(pad);
		else {
			stat = MwSysStatGet();
			// Find if connection has just been established
			if ((stat != NULL) && (stat->sys_stat >= MW_ST_READY) &&
					(prevStat < MW_ST_READY)) {
				// Connection established! Print IP in the status string
				if (MwIpCfgGet(stat->cfg, &ip) != MW_OK)
					MenuPanic("COULD NOT GET IP!", 17);
				statStr.length = MenuBin2IpStr(ip->addr, statBuf);
				MenuStatStrSet(statStr);
			} else if ((stat != NULL) && (stat->sys_stat < MW_ST_READY)
					&& (prevStat >= MW_ST_READY)) {
				// Connection lost! Inform in the status string
				statStr.length = MenuStrCpy(statBuf, "DISCONNECTED!", 16 - 1);
				statBuf[16 - 1] = '\0';
				MenuStatStrSet(statStr);
			}
		}
	}
	return 0;
}

/** \} */

