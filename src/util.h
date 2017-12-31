#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>
// Not very happy about adding this header here...
#include "mw/megawifi.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

/// Section attribute definition for variables and functions. Examples:
/// - int a SECTION(data);
/// - void foo(void) SECTION(text);
#define SECTION(name)	__attribute__((section(#name)))

/// Remove compiler warnings when not using a function parameter
#define UNUSED_PARAM(x)		(void)x

#if !defined(MAX)
/// Returns the maximum of two numbers
#define MAX(a, b)	((a)>(b)?(a):(b))
#endif
#if !defined(MIN)
/// Returns the minimum of two numbers
#define MIN(a, b)	((a)<(b)?(a):(b))
#endif

/// Swaps bytes from a word (16 bit)
#define ByteSwapWord(w)	(uint16_t)((((uint16_t)(w))>>8) | (((uint16_t)(w))<<8))

/// Swaps bytes from a dword (32 bits)
#define ByteSwapDWord(dw)	(uint32_t)((((uint32_t)(dw))>>24) |               \
		((((uint32_t)(dw))>>8) & 0xFF00) | ((((uint32_t)(dw)) & 0xFF00)<<8) | \
	  	(((uint32_t)(dw))<<24))

static inline void ToUpper(char str[]) {
	uint16_t i;
	for (i = 0; str[i] != '\0'; i++) {
		if ((str[i] >= 'a') && (str[i] <= 'z'))
			str[i] -= 'a' - 'A';
	}
}

/************************************************************************//**
 * \brief Converts an unsigned 8-bit number (uint8_t) in its character
 * string representation.
 *
 * \param[in]  num Input number to convert.
 * \param[out] str String representing the input number.
 *
 * \return Resulting str length (not including null termination).
 ****************************************************************************/
uint8_t Byte2UnsStr(uint8_t num, char str[4]);

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
char *Str2UnsByte(char strIn[], uint8_t *result);

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
int Long2Str(long num, char str[], int bufLen, int padLen, char padChr);

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
MwMsgSysStat *ApJoinWait(uint16_t retries, uint16_t frmPoll);

#ifndef TRUE
/// TRUE value for logic comparisons
#define TRUE 1
/// FALSE value for logic comparisons
#define TRUE 1
#define FALSE 0
#endif

#endif //_UTIL_H_


