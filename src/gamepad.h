/************************************************************************//**
 * \brief Genesis/Megadrive routines for reading the Gamepad.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2017
 * \note   Currently only 3-button gamepad for port A is supported. The
 *         routine is also compatible with 6-button pads as long as the
 *         GpRead() routine is called only once per frame.
 * \defgroup Gamepad gamepad
 * \{
 ****************************************************************************/
#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#include <stdint.h>

/** \addtogroup GpRegAddrs GpRegAddrs
 *  \brief Gamepad related register addresses.
 *  \{ */
/// Version number address
#define GP_REG_VERSION_ADDR		0xA10001

/// Port A, data register address
#define GP_REG_PORTA_DATA_ADDR	0xA10003
/// Port B, data register address
#define GP_REG_PORTB_DATA_ADDR	0xA10005
/// Port C, data register address
#define GP_REG_PORTC_DATA_ADDR	0xA10007

/// Port A, control register address
#define GP_REG_PORTA_CTRL_ADDR	0xA10009
/// Port B, control register address
#define GP_REG_PORTB_CTRL_ADDR	0xA1000B
/// Port C, control register address
#define GP_REG_PORTC_CTRL_ADDR	0xA1000D
/** \} */


/** \addtogroup GpRegs GpRegs
 *  \brief Gamepad related registers.
 *  \{ */
/// Gamepad hardware version number
#define GP_REG_VERSION			(*((volatile uint8_t*)GP_REG_VERSION_ADDR))

/// Port A, data register
#define GP_REG_PORTA_DATA		(*((volatile uint8_t*)GP_REG_PORTA_DATA_ADDR))
/// Port B, data register
#define GP_REG_PORTB_DATA		(*((volatile uint8_t*)GP_REG_PORTB_DATA_ADDR))
/// Port C, data register
#define GP_REG_PORTC_DATA		(*((volatile uint8_t*)GP_REG_PORTC_DATA_ADDR))

/// Port A, control register
#define GP_REG_PORTA_CTRL		(*((volatile uint8_t*)GP_REG_PORTA_CTRL_ADDR))
/// Port B, control register
#define GP_REG_PORTB_CTRL		(*((volatile uint8_t*)GP_REG_PORTB_CTRL_ADDR))
/// Port C, control register
#define GP_REG_PORTC_CTRL		(*((volatile uint8_t*)GP_REG_PORTC_CTRL_ADDR))
/** \} */

/** \addtogroup GpMasks GpMasks
 *  \brief Masks used to filter the cross and buttons.
 *  \{ */
#define GP_START_MASK			0x80	///< Start button
#define GP_A_MASK				0x40	///< A button
#define GP_B_MASK				0x10	///< B button
#define GP_C_MASK				0x20	///< C button
#define GP_RIGHT_MASK			0x08	///< Right direction
#define GP_LEFT_MASK			0x04	///< Left direction
#define GP_DOWN_MASK			0x02	///< Down direction
#define GP_UP_MASK				0x01	///< Up direction
/** \} */


/************************************************************************//**
 * Module initialization. Call this routine before using any other in this
 * module.
 ****************************************************************************/
#define GpInit()	do{GP_REG_PORTA_CTRL = 0x40;}while(0)

/************************************************************************//**
 * Read gamepad. Currently only 3-button pads on port A are read.
 *
 * \return Pad status in format SACBRLDU (START, A, C, B, RIGHT, LEFT,
 *         DOWN, UP). For each bit, a '1' means that the button/direction
 *         is not pressed. A '0' means that the button/direction is pressed.
 *         Masks can be used to filter returned data (see GpMasks).
 ****************************************************************************/
uint8_t GpRead(void);

#endif /*_GAMEPAD_H_*/

/** \} */

