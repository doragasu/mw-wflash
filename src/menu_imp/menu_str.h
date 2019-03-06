#ifndef _MENU_STR_H_
#define _MENU_STR_H_

#include "../util.h"
#include "menu_scr.h"

struct menu_str {
	char *str;	///< Pointer to string
	uint8_t length;	///< String length, not including null termination
	/// Allocated space minus 1. Set to 0 for read only strings
	uint8_t max_length;
};

/// \note Strings can be considered read-only, read-write and empty. An empty
/// string is also read-write, but has not yet data available:
/// - Read-only: length > 0, max_length == 0.
/// - Read-write: length > 0, max_length > 0.
/// - Empty: length == 0, max_length > 0.

/// Build a read only menu string
#define MENU_STR_RO(string) {			\
	.length = sizeof(string) - 1,		\
	.max_length = 0,			\
	.str = string				\
}

/// Build an editable (read/write) menu string.
/// \note string must be const, and will be copied to RAM during init.
#define MENU_STR_RW(string, max_len) {		\
	.length = sizeof(string) - 1,		\
	.max_length = max_len,			\
	.str = string				\
}

/// Build an empty, editable menu string.
/// \note RAM will be allocated during initialization.
#define MENU_STR_EMPTY(max_len) {		\
	.length = 0,				\
	.max_length = max_len,			\
	.str = NULL				\
}

/// Build an empty, editable menu string, with a buffer of the specified
/// length statically allocated.
#define MENU_STR_BUF(max_len) {			\
	.length = 0,				\
	.max_length = max_len,			\
	.str = (char[max_len + 1]){0}		\
}

/// NULL menu string
#define MENU_STR_NULL {.length = 0, .max_length = 0, .str = NULL}

/// Alignment for the menu_str_line() function.
enum PACKED menu_h_align {
	MENU_H_ALIGN_LEFT = 0,
	MENU_H_ALIGN_CENTER,
	MENU_H_ALIGN_RIGHT,
	MENU_H_ALIGN_MAX
};

/************************************************************************//**
 * Check if input is a read-only string.
 *
 * \param[in] str String to check.
 *
 * \return true if input is a read-only string
 ****************************************************************************/
static inline int menu_str_is_ro(const struct menu_str *str)
{
	return (str->length && !str->max_length);
}

/************************************************************************//**
 * Check if input is a read/write string.
 *
 * \param[in] str String to check.
 *
 * \return true if input is a read/write string
 ****************************************************************************/
static inline int menu_str_is_rw(const struct menu_str *str)
{
	return (str->length && str->max_length);
}

/************************************************************************//**
 * Check if input is an empty string.
 *
 * \param[in] str String to check.
 *
 * \return true if input is an empty string
 ****************************************************************************/
static inline int menu_str_is_empty(const struct menu_str *str)
{
	return (!str->length && str->max_length);
}

/************************************************************************//**
 * Draw a single character at specified position.
 *
 * \param[in] chr        Character to draw.
 * \param[in] x          Horizontal coordinate for the string.
 * \param[in] y          Vertical coordinate for the string.
 * \param[in] char_color Character color.
 * \param[in] loc        Location of the screen in which to write.
 *
 * \note Checks for DMA not to be active before writing to VDP.
 ****************************************************************************/
static inline void menu_str_char(char chr, uint8_t x, uint8_t y, uint8_t char_color,
		enum menu_placement loc)
{
	VdpDmaWait();
	VdpDrawChars(VDP_PLANEA_ADDR, x + loc, y, char_color, 1, &chr);
}

/************************************************************************//**
 * Draw a menu string at specified position.
 *
 * Character strings are drawn until null termination is found, or max_len
 * is reached.
 *
 * \param[in] str        Menu string to write.
 * \param[in] x          Horizontal coordinate for the string.
 * \param[in] y          Vertical coordinate for the string.
 * \param[in] max_len    Maximum number of characters to draw.
 * \param[in] text_color Text color.
 * \param[in] loc        Location of the screen in which to write.
 *
 * \note Checks for DMA not to be active before writing to VDP.
 ****************************************************************************/
static inline void menu_str_pos_draw(const struct menu_str *str, uint8_t x,
		uint8_t y, int max_len, uint8_t text_color,
		enum menu_placement loc)
{
	VdpDmaWait();
	VdpDrawChars(VDP_PLANEA_ADDR, x + loc, y, text_color,
			MIN(str->length, max_len), str->str);
}

/************************************************************************//**
 * Draw a menu string using a full screen line.
 *
 * Line space not used by the string is filled with blanks.
 *
 * \param[in] str        Menu string to write.
 * \param[in] line       Line in which to write the string.
 * \param[in] margin     Text margin (for left/right aligns)
 * \param[in] align      Text alignment.
 * \param[in] text_color Text color.
 * \param[in] loc        Location of the screen in which to write.
 *
 * \note Checks for DMA not to be active before writing to VDP.
 ****************************************************************************/
void menu_str_line_draw(const struct menu_str *str, uint8_t line,
		uint8_t margin, enum menu_h_align align,
		uint8_t text_color, enum menu_placement loc);

/************************************************************************//**
 * Copy a menu string.
 *
 * \param[out] dst Destination menu string.
 * \param[in]  src Source menu string.
 *
 * \note String buffer for dst needs maxlen + 1 bytes.
 ****************************************************************************/
void menu_str_cpy(struct menu_str *dst, const struct menu_str *src);

/************************************************************************//**
 * Copy a null terminated string into a buffer.
 *
 * \param[out] dst Destination buffer.
 * \param[in]  src Source string.
 * \param[in]  max_len Maximum number of characters to copy.
 *
 * \return The number of copied characters (not including null termination)
 ****************************************************************************/
int menu_str_buf_cpy(char *dst, const char *src, unsigned int max_len);

/************************************************************************//**
 * Copy a null terminated string at the end of a struct menu_str.
 *
 * \param[out] dst    Destination struct menu_str.
 * \param[in]  append Source string to copy.
 ****************************************************************************/
static inline void menu_str_append(struct menu_str *dst, const char *append)
{
	dst->length += menu_str_buf_cpy(dst->str + dst->length, append,
			dst->max_length - dst->length);
}

/************************************************************************//**
 * Replaces the string in the specified menu string, with a new null
 * terminated string.
 *
 * \param[out] dst     Destination struct menu_str.
 * \param[in]  replace Null terminated string to replace the old one.
 ****************************************************************************/
static inline void menu_str_replace(struct menu_str *dst, const char *replace)
{
	dst->length = menu_str_buf_cpy(dst->str, replace, dst->max_length);
}

/************************************************************************//**
 * Clear a complete line of text, at the specified location.
 *
 * \param[in] loc  Location of the screen to clear.
 * \param[in] line Line number to clear.
 *
 * \note Checks for DMA not to be active before writing to VDP.
 * \warning Leaves DMA active when returns.
 ****************************************************************************/
static inline void menu_str_line_clear(enum menu_placement loc, int line)
{
	VdpDmaWait();
	VdpLineClear(VDP_PLANEA_ADDR + loc, line);
}

/************************************************************************//**
 * Copy a complete line of text, at the specified location.
 *
 * \param[in] dst  Location of screen from which to copy line.
 * \param[in] src  Location of the screen to which line is copied.
 * \param[in] line Line number to copy.
 *
 * \note Checks for DMA not to be active before writing to VDP.
 * \warning Leaves DMA active when returns.
 ****************************************************************************/
static inline void menu_str_line_copy(enum menu_placement dst, enum menu_placement src,
		uint8_t line)
{
	VdpDmaWait();
	VdpDmaVRamCopy(VDP_PLANEA_ADDR + 2 * (src + VDP_PLANE_HTILES * line),
			VDP_PLANEA_ADDR + 2 * (dst + VDP_PLANE_HTILES * line),
			2 * MENU_LINE_CHARS);
}

#endif /*_MENU_STR_H_*/

