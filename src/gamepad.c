/************************************************************************//**
 * \brief Genesis/Megadrive routines for reading the Gamepad.
 *
 * \author Jesús Alonso (doragasu)
 * \date   2017
 * \note   Currently only 3-button gamepad for port A is supported. The
 *         routine is also compatible with 6-button pads as long as the
 *         GpRead() routine is called only once per frame.
 ****************************************************************************/
#include "gamepad.h"

/************************************************************************//**
 * Read gamepad. Currently only 3-button pads on port A are read.
 *
 * \return Pad status in format SACBRLDU (START, A, C, B, RIGHT, LEFT,
 *         DOWN, UP). For each bit, a '1' means that the button/direction
 *         is not pressed. A '0' means that the button/direction is pressed.
 *         Masks can be used to filter returned data (see GpMasks).
 ****************************************************************************/
uint8_t GpRead(void) {
	uint8_t ret;

	// Read START and A buttons
	ret = GP_REG_PORTA_DATA;
	// Prepare to read cross, B and C
	GP_REG_PORTA_DATA = 0x40;
	// While data gets ready, shift START and A to their position
	ret = (ret & 0x30)<<2;
	// Read cross, B and C
	ret |= GP_REG_PORTA_DATA & 0x3F;
	// Put back to normal
	GP_REG_PORTA_DATA = 0x00;

	return ret;
}

