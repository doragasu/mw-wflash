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

/// Length of the wflash buffer
#define WFLASH_BUFLEN	WF_MAX_DATALEN

/// TCP port to use (set to Megadrive release year ;-)
#define WFLASH_PORT 	1989

/// Command buffer
static char cmdBuf[WFLASH_BUFLEN];

/// Sets background to RED, prints message and loops forever
void Panic(char msg[]) {
	VdpRamWrite(VDP_CRAM_WR, 0x00, VDP_COLOR_RED);
	VdpDrawText(VDP_PLANEA_ADDR, 1, 1, VDP_TXT_COL_CYAN, msg);
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

	// Poll status each 100 ms	
	do {
		for (loop = 5; loop > 0; loop--) VdpVBlankWait();
		stat = MwSysStatGet();
	} while((stat != NULL) && (stat->sys_stat < 4));
// TODO: Use ONLINE flag (it looks like big/little endian reverse bit fields
//	} while((stat != NULL) && (!stat->online));

	if (stat == NULL) return -1;

	// Obtain IP address and print it
	i = stat->cfg;
	if (MwIpCfgGet(i, &ip) != MW_OK) Panic("COULD NOT GET IP!");
	VdpDrawText(VDP_PLANEA_ADDR, 1, 2, VDP_TXT_COL_WHITE, "IP:");
	i = 5 + VdpDrawDec(VDP_PLANEA_ADDR, 5, 2, VDP_TXT_COL_CYAN, ip->addr>>24);
	VdpDrawText(VDP_PLANEA_ADDR, i++, 2, VDP_TXT_COL_WHITE, ".");
	i += VdpDrawDec(VDP_PLANEA_ADDR, i, 2, VDP_TXT_COL_CYAN, ip->addr>>16);
	VdpDrawText(VDP_PLANEA_ADDR, i++, 2, VDP_TXT_COL_WHITE, ".");
	i += VdpDrawDec(VDP_PLANEA_ADDR, i, 2, VDP_TXT_COL_CYAN, ip->addr>>8);
	VdpDrawText(VDP_PLANEA_ADDR, i++, 2, VDP_TXT_COL_WHITE, ".");
	i += VdpDrawDec(VDP_PLANEA_ADDR, i, 2, VDP_TXT_COL_CYAN, ip->addr);

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
	VdpDrawText(VDP_PLANEA_ADDR, 12, 1, VDP_TXT_COL_WHITE, "- MW RTOS");
	i = 22 + VdpDrawDec(VDP_PLANEA_ADDR, 22, 1, VDP_TXT_COL_CYAN, verMajor);
	VdpDrawText(VDP_PLANEA_ADDR, i++, 1, VDP_TXT_COL_WHITE, ".");
	i += VdpDrawDec(VDP_PLANEA_ADDR, i, 1, VDP_TXT_COL_CYAN, verMinor) + 1;
	ToUpper(variant);
	VdpDrawText(VDP_PLANEA_ADDR, i, 1, VDP_TXT_COL_CYAN, variant);

	return 0;
}

/// Global initialization
void Init(void) {
	// Initialize VDP
	VdpInit();
	// Initialize gamepad
	GpInit();

	// Print program version
	VdpDrawText(VDP_PLANEA_ADDR, 1, 1, VDP_TXT_COL_WHITE, "WFLASH 0.1");
	// Initialize MegaWiFi
	if (MegaWifiInit()) Panic("MEGAWIFI?");
}

/// Entry point
int main(void) {
	// Initialization
	Init();
	VdpDrawText(VDP_PLANEA_ADDR, 1, 3, VDP_TXT_COL_MAGENTA, "READY!");
	while (1) {
		// Bind port 1989 for command server
		MwTcpBind(WF_CHANNEL, WFLASH_PORT);
		// Wait until we have a client connection
		if (WaitUserInteraction());
		// Got client connection!!!
		SfInit();
		// Run command parser
		while (!SfCycle());
		/// Error or socket disconnected
		MwTcpDisconnect(WF_CHANNEL);
		VdpLineClear(VDP_PLANEA_ADDR, 3);
		VdpDrawText(VDP_PLANEA_ADDR, 1, 3, VDP_TXT_COL_MAGENTA,
				"DISCONNECTED!");
	}
	return 0;
}

/** \} */

