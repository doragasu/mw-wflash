#include "util.h"
#include "vdp.h"

/************************************************************************//**
 * \brief Converts an unsigned 8-bit number (uint8_t) in its character
 * string representation.
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

/************************************************************************//**
 * \brief Converts a character string representing an 8-bit unsigned number,
 * to its binary (uint8_t) representation.
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
    // Special case: number is zero
    if (*strIn < '0' || *strIn > '9') return strIn;
	// Determine number length (up to 4 characters)
	for (i = 0; (i < 4) && (strIn[i] >= '0') && (strIn[i] <= '9'); i++);
	
	switch (i) {
		// If number is 3 characters, the number fits in 8 bits only if
		// lower than 256
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

/************************************************************************//**
 * \brief Converts an integer to a character string.
 *
 * \param[in]  num  Number to convert to string.
 * \param[out] str  String that will hold the converted number.
 * \param[in]  bufLen Length of str buffer.
 * \param[in]  padLen Length of the padding to introduce. 0 for no padding.
 * \param[in]  padChr Character used for padding (typically '0' or ' ').
 *
 * \return Number of characters written to str buffer, not including the
 * null termination. 0 if string does not fin in the buffer and has not
 * been converted.
 *
 * \warning Function uses lots of divisions. Maybe it is not the best of the
 * ideas using it in a game loop.
 ****************************************************************************/
int Long2Str(long num, char str[], int bufLen, int padLen, char padChr) {
    int i = 0;
    int j;
    int rem;
    int len = 0;

    // Obtain string length
    for (rem = num, len = 0; rem; len++, rem /= 10);
    // if number is 0 or negative, increase length by 1
    if (len == 0) {
        len++;
        str[i++] = '0';
    }
    if (num < 0) {
        len++;
        num = -num;
        str[i++] = '-';
    }
    // Check number fits in buffer
    if (((len + 1) > bufLen) || ((padLen + 1) > bufLen)) return 0;
    for (; i < (padLen - len); i++) str[i] = padChr;

    // Perform the conversion in reverse order
    padLen = MAX(padLen, len);
    str[padLen] = '\0';
    for (j = padLen - 1;j >= i; j--) {
        str[j] = '0' + num % 10;
        num = num / 10;
    }

    return padLen;
}


/************************************************************************//**
 * \brief Waits until module has joined an AP, an error occurs or specified
 *        retries and frames expire.
 *
 * \param[in] retries Number of times to retry waiting for AP to join.
 * \param[in] frmPoll Number of frames to wait between state polls to the
 *            WiFi module.
 *
 * \return Module status if module it has joined AP, NULL if error or
 * specified retries and frames expire.
 *
 * \note If you do not want the function to block, call it without retries
 * and with zero frmPoll: ApJoinWait(0, 0);
 ****************************************************************************/
MwMsgSysStat *ApJoinWait(uint16_t retries, uint16_t frmPoll) {
    uint16_t frm = 0;
    uint16_t tried;
	MwMsgSysStat *stat;

    for (tried = 0; tried <= retries; tried++) {
    	stat = MwSysStatGet();
    	// Find if connection has just been established
    	if ((stat != NULL) && (stat->sys_stat >= MW_ST_READY)) {
            return stat;
    	}
        for (frm = 0; frm < frmPoll; frm++) VdpVBlankWait();
	}
    return NULL;
}

