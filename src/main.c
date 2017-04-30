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

/// Sets background to RED, prints message and loops forever
/// \todo scroll to the origin before drawing
void Panic(char msg[]) {
	VdpRamWrite(VDP_CRAM_WR, 0x00, VDP_COLOR_RED);
	VdpDrawText(VDP_PLANEA_ADDR, 1, 1, VDP_TXT_COL_CYAN,
			SF_LINE_MAXCHARS, msg);
	MwModuleReset();
	while(1);
}

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
	} while (s != MW_ERROR && s == MW_SOCK_TCP_LISTEN);
	return (MW_SOCK_TCP_EST == s)?0:-1;
}

/// Waits forever until module joins an AP or error
int WaitApJoin(void) {
	MwMsgSysStat* stat;
	MwIpCfg *ip;
	uint8_t i;
	int8_t loop;
	char statBuf[16];
	MenuString statStr;

	statStr.string = statBuf;
	// Poll status each 100 ms	
	do {
		for (loop = 5; loop > 0; loop--) VdpVBlankWait();
		stat = MwSysStatGet();
	} while((stat != NULL) && (stat->sys_stat < 4));
// TODO: Use ONLINE flag (it looks like big/little endian reverse bit fields
//	} while((stat != NULL) && (!stat->online));

	if (stat == NULL) return -1;

	// Obtain IP address and fill status info with result
	i = stat->cfg;
	if (MwIpCfgGet(i, &ip) != MW_OK) {
		strcpy(statBuf, "DISCONNECTED!");
		statStr.length = 13;
		MenuStatStrSet(statStr);
		return 0;
	}
	i =  Byte2UnsStr(ip->addr>>24, statBuf);
	statBuf[i++] = '.';
	i += Byte2UnsStr(ip->addr>>16, statBuf + i);
	statBuf[i++] = '.';
	i += Byte2UnsStr(ip->addr>>8, statBuf + i);
	statBuf[i++] = '.';
	i += Byte2UnsStr(ip->addr, statBuf + i);
	statBuf[i] = '\0';
	statStr.length = i;

	return 0;
}

/// MegaWiFi initialization
int MegaWifiInit(void){
	uint8_t verMajor, verMinor, i, loop;
	char *variant;

	// Initialize MegaWiFi
	MwInit(cmdBuf, WFLASH_BUFLEN);
	// Wait a bit and take module out of resest
	VdpVBlankWait();
	VdpVBlankWait();
	MwModuleStart();
	for (loop = 119; loop > 0; loop--) VdpVBlankWait();
	UartResetFifos();
	// Wait until module has joined AP
	if (WaitApJoin()) Panic("COULD NOT JOIN AP!");
	
	// Try detecting MegaWiFi module by requesting version
	if (MwVersionGet(&verMajor, &verMinor, &variant) != MW_OK) {
		return -1;
	}
	// Draw detected RTOS version
	VdpDrawText(VDP_PLANEA_ADDR, 12, 1, VDP_TXT_COL_WHITE,
			SF_LINE_MAXCHARS, "- MW RTOS");
	i = 22 + VdpDrawDec(VDP_PLANEA_ADDR, 22, 1, VDP_TXT_COL_CYAN, verMajor);
	VdpDrawText(VDP_PLANEA_ADDR, i++, 1, VDP_TXT_COL_WHITE,
			SF_LINE_MAXCHARS, ".");
	i += VdpDrawDec(VDP_PLANEA_ADDR, i, 1, VDP_TXT_COL_CYAN, verMinor) + 1;
	ToUpper(variant);
	VdpDrawText(VDP_PLANEA_ADDR, i, 1, VDP_TXT_COL_CYAN,
			SF_LINE_MAXCHARS, variant);

	return 0;
}

/// Global initialization
void Init(void) {
	const MenuString str = {"IDLE", 4};
	// Initialize VDP
	VdpInit();
	// Initialize gamepad
	GpInit();
	// Initialize menu system
	WfMenuInit(str);
}

/// Entry point
int main(void) {
	uint8_t pad;
	// Initialization
	Init();
	while (1) {
		/// \todo 1. Parse communications events.
		// 2. Wait for VBlank.
		VdpVBlankWait();
		// 3. Read controller and perform non-menu related actions.
		pad = GpPressed();
		// 4. Perform menu related actions.
		MenuButtonAction(pad);
		VdpDrawHex(VDP_PLANEA_ADDR, 1, 1, VDP_TXT_COL_CYAN, pad);

	}
//	while (1) { // Bind port 1989 for command server MwTcpBind(WF_CHANNEL, WFLASH_PORT); // Wait until we have a client connection
//		WaitUserInteraction();
//		// Got client connection!!!
//		SfInit();
//		// Run command parser
//		while (!SfCycle());
//		/// Error or socket disconnected
//		MwTcpDisconnect(WF_CHANNEL);
//		VdpLineClear(VDP_PLANEA_ADDR, 3);
//		VdpDrawText(VDP_PLANEA_ADDR, 1, 3, VDP_TXT_COL_MAGENTA,
//				SF_LINE_MAXCHARS, "DISCONNECTED!");
//	}
	return 0;
}

/** \} */

