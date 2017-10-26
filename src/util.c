#include "util.h"

/****************************************************************************
 * Converts an unsigned 8-bit number (uint8_t) in its character string
 * representation.
 *
 * \param[in]  num Input number to convert.
 * \param[out] str String representing the input number.
 *
 * \return Resulting str length (not including null termination).
 ****************************************************************************/
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

/****************************************************************************
 * Converts a character string representing an 8-bit unsigned number, to its
 * binary (uint8_t) representation.
 *
 * \param[in]  strIn  Input string with the number to convert.
 * \param[out] result Converted number will be left here.
 *
 * \return Pointer to the end of the number received in strIn parameter, or
 * NULL if the strIn does not contain a valid string representation of an
 * uint8_t type.
 ****************************************************************************/
char *Str2UnsByte(char strIn[], uint8_t *result) {
	uint8_t i;

	*result = 0;

	// Skip leading zeros
	while (*strIn == '0') strIn++;
	// Determine number length (up to 4 characters)
	for (i = 0; (i < 4) && (strIn[i] >= '0') && (strIn[i] <= '9'); i++);
	
	switch (i) {
		// If number is 3 characters, the number fits in 8 bits only if
		// lower than 255
		case 3:
			if ((strIn[0] > '2') || ((strIn[0] == '2') && ((strIn[1] > '5') ||
						((strIn[1] == '5') && (strIn[2] > '5')))))
				return NULL;
			else {
				*result = ((*strIn) - '0') * 100;
				strIn++;
			}
			// fallthrough
		case 2:
			*result += ((*strIn) - '0') * 10;
			strIn++;
			// fallthrough
		case 1:
			*result += (*strIn) - '0';
			strIn++;
			break;

		// If length is 4 or more, number does not fit in 8 bits.
		default:
			return NULL;
	}
	return strIn;
}

