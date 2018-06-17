/************************************************************************//**
 * \brief System controller for wflash. Keeps the system status and
 * processes incoming events, to perform the requested actions.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2017
 * \todo Module currently does not support checksum/CRC
 * \defgroup sysfsm sysfsm
 * \{
 ****************************************************************************/

#ifndef _SYSFSM_H_
#define _SYSFSM_H_

#include <stdint.h>
#include "menu.h"

/// Default channel to use for MegaWiFi communications
#define SF_CHANNEL      1

/// Default port to use for MegaWiFi communications
#define SF_PORT         1989

/// Maximum number of characters to draw per line
#define SF_LINE_MAXCHARS	(VDP_SCREEN_WIDTH_PX/8 - 1)

/// Entry point address is stored at the beginning of the NOTES section of
/// the cartridge header
#define SF_ENTRY_POINT_ADDR	(*((uint32_t*)0x0001C8))

/// Bootloader address is currently the 68000 start entry
#define SF_BOOTLOADER_ADDR  (*((uint32_t*)0x000004))

/************************************************************************//**
 * Module initialization. Call this function before using this module.
 ****************************************************************************/
void SfInit(void);

/************************************************************************//**
 * Perform one cycle of the system state machine.
 *
 * \return 0 if OK, non-zero if error.
 ****************************************************************************/
int SfCycle(Menu *md);

/************************************************************************//**
 * Clear environment and boot from specified address.
 *
 * \param[in] addr Address from which to boot.
 ****************************************************************************/
void SfBoot(uint32_t addr);

#endif /*_SYSFSM_H_*/

/** \} */

