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
#include "mw/megawifi.h"
#include "menu_imp/menu.h"

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
#define SF_BOOTLOADER_ADDR	(*((uint32_t*)0x000004))

/************************************************************************//**
 * Module initialization. Call this function before using this module.
 ****************************************************************************/
void sf_init(char *cmd_buf, int16_t buf_length,
		struct menu_entry_instance *instance);

/************************************************************************//**
 * Start the command parser.
 *
 * \return 0 if OK, non-zero if error.
 ****************************************************************************/
void sf_start(void);

/************************************************************************//**
 * Clear environment and boot from specified address.
 *
 * \param[in] addr Address from which to boot.
 * \param[in] quick When true, do not wait any frames before performing boot
 ****************************************************************************/
void sf_boot(uint32_t addr, int quick);

#endif /*_SYSFSM_H_*/

/** \} */

