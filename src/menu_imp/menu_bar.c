#include <string.h>
#include "menu_bar.h"

struct menu_bar_data {
	struct menu_str *str;
	uint32_t delta;
	bool reverse;
};

static struct menu_bar_data mb;

void menu_bar_init(struct menu_str *str, uint16_t max, bool reverse)
{
	mb.str = str;
	mb.delta = (((uint32_t)(str->max_length))<<24) / (uint32_t)max;
	mb.reverse = reverse;
}

void menu_bar_update(uint16_t pos)
{
	uint8_t len;
	uint8_t n_fill;
	uint8_t max_length = mb.str->max_length;

	len = ((uint32_t)pos * mb.delta)>>24;
	len = MIN(len, max_length);

	n_fill = mb.reverse ? max_length - len : len;
	// Filled characters
	memset(mb.str->str, 127, n_fill);
	// Empty characters
	memset(mb.str->str + n_fill, ' ', max_length - n_fill);
	mb.str->length = max_length;
}

