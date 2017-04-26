#include "util.h"

uint8_t Byte2UnsStr(uint8_t num, char str[4]) {
	uint8_t i = 0;
	uint8_t tmp;

	// Compute digits and write decimal number
	// On 3 digit numbers, first one can only be 1 or 2. Take advantage of
	// this to avoid division (TODO test if this is really faster).
	if (num > 199) {
		str[i++] = '2';
		num -= 200;
	} else if (num > 99) {
		str[i++] = '1';
		num -= 100;
	}
	
	tmp = num / 10;
	if (tmp) str[i++] = '0' + tmp;
	str[i++] = '0' + num % 10;
	str[i] = '\0';

	return i;
}
