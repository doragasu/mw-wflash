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

/// Entry point address is stored at the beginning of the NOTES section of
/// the cartridge header
#define SF_ENTRY_POINT_ADDR	(*((uint32_t*)0x1C8))

/************************************************************************//**
 * Module initialization. Call this function before using this module.
 ****************************************************************************/
void SfInit(void);

/************************************************************************//**
 * Perform one cycle of the system state machine.
 *
 * \return 0 if OK, non-zero if error.
 ****************************************************************************/
int SfCycle(void);

/************************************************************************//**
 * Clear environment and boot from specified address.
 *
 * \param[in] addr Address from which to boot.
 ****************************************************************************/
void SfBoot(uint32_t addr);

#endif /*_SYSFSM_H_*/

/** \} */

