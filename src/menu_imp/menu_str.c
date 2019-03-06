#include <string.h>
#include "menu_str.h"
#include "menu_int.h"

void menu_str_line_draw(const struct menu_str *str, uint8_t line,
		uint8_t margin, enum menu_h_align align,
		uint8_t text_color, enum menu_placement loc)
{
	char line_buf[MENU_LINE_CHARS];
	int i, j;
	int start, end;

	switch (align) {
	case MENU_H_ALIGN_CENTER:
		start = MAX((MENU_LINE_CHARS - MENU_DEF_LEFT_MARGIN -
			MENU_DEF_RIGHT_MARGIN - str->length) / 2 +
			MENU_DEF_LEFT_MARGIN, MENU_DEF_LEFT_MARGIN);
		end = MIN(start + str->length, MENU_LINE_CHARS -
				MENU_DEF_LEFT_MARGIN);
		break;

	case MENU_H_ALIGN_RIGHT:
		end = MENU_LINE_CHARS - margin;
		start = MAX(end - str->length, MENU_DEF_LEFT_MARGIN);
		break;

	case MENU_H_ALIGN_LEFT:
		// flowdown
	default:
		start = margin;
		end = MIN(start + str->length, MENU_LINE_CHARS);
	}

	for (i = 0; i < start; i++) line_buf[i] = ' ';
	for (j = 0; i < end; i++, j++) line_buf[i] = str->str[j];
	for (; i < MENU_LINE_CHARS; i++) line_buf[i] = ' ';

	VdpDmaWait();
	VdpDrawChars(VDP_PLANEA_ADDR, loc, line, text_color, MENU_LINE_CHARS,
			line_buf);
}

void menu_str_cpy(struct menu_str *dst, const struct menu_str *src)
{
	int len = src->length;

	if (dst->max_length) {
		len = MIN(src->length, dst->max_length);
	}

	memcpy(dst->str, src->str, len);
	dst->str[len] = '\0';
	dst->length = len;
}

int menu_str_buf_cpy(char *dst, const char *src, unsigned int max_len)
{
	unsigned int i;

	// If no max_len, use maximum unsigned value
	if (!max_len) {
		max_len--;
	}
	for (i = 0; i < max_len && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';

	return i;
}

