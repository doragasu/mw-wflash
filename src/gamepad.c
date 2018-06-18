#include "gamepad.h"

static uint8_t prev = 0xFF;

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

uint8_t GpPressed(void) {
	uint8_t retVal;
	uint8_t input = GpRead();

	retVal = input | ~prev;
	prev = input;
	return retVal;
}

