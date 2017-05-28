#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

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

/****************************************************************************
 * Converts an unsigned 8-bit number (uint8_t) in its character string
 * representation.
 *
 * \param[in]  num Input number to convert.
 * \param[out] str String representing the input number.
 *
 * \return Resulting str length (not including null termination).
 ****************************************************************************/
uint8_t Byte2UnsStr(uint8_t num, char str[4]);

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
char *Str2UnsByte(char strIn[], uint8_t *result);

#ifndef TRUE
/// TRUE value for logic comparisons
#define TRUE 1
/// FALSE value for logic comparisons
#define TRUE 1
#define FALSE 0
#endif

#endif //_UTIL_H_


