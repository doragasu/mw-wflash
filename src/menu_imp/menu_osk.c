#include <string.h>
#include "menu_int.h"
#include "menu_osk.h"
#include "../vdp.h"
#include "../gamepad.h"

/// \todo We are constantly reading the menu type index. Maybe it should be
/// wiser to have it as an input parameter of functions requiring it.

/// Number of rows of the QWERTY menu
#define MENU_OSK_QWERTY_ROWS		4
/// Number of columns of the QWERTY menu
#define MENU_OSK_QWERTY_COLS		11

/// Alphanumeric menu definition
static const char qwerty_layout[2 * MENU_OSK_QWERTY_ROWS * MENU_OSK_QWERTY_COLS] = {
//       0    1    2    3    4    5    6    7    8    9   10
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[',
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', '+',
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', ']',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', '=',
};

/// Supported virtual menu actions
enum menu_osk_special_func{
	MENU_OSK_FUNC_CANCEL = 0,	///< Cancel edit
	MENU_OSK_FUNC_DEL,		///< Delete character
	MENU_OSK_FUNC_LEFT,		///< Move to left
	MENU_OSK_FUNC_RIGHT,		///< Move to right
	MENU_OSK_FUNC_ENTER,		///< Done edit
	MENU_OSK_NUM_FUNCS		///< Number of special function keys
};

/// Space key label
static const char qwerty_space_str[] = "[SPACE]";

#define MENU_OSK_SPACE_OFFSET	((sizeof(qwerty_space) - 1)) - 3 +	\
	((MENU_LINE_CHARS - 2 * MENU_OSK_QWERTY_COLS)>>1) 

/// Number of columns of the IP OSK menu
#define MENU_OSK_IPV4_COLS	3
/// Number of rows of the IP OSK menu
#define MENU_OSK_IPV4_ROWS	4

/// IP entry menu definition
static const char ipv4_layout[MENU_OSK_IPV4_ROWS * MENU_OSK_IPV4_COLS] = {
	'7', '8', '9',
	'4', '5', '6',
	'1', '2', '3',
	'.', '0', MENU_OSK_KEY_NONE
};

/// Number of columns of the IP OSK menu
#define MENU_OSK_NUMERIC_COLS	3
/// Number of rows of the IP OSK menu
#define MENU_OSK_NUMERIC_ROWS	4

/// Numeric entry menu definition
static const char numeric_layout[MENU_OSK_NUMERIC_ROWS *
MENU_OSK_NUMERIC_COLS] = {
	'7', '8', '9',
	'4', '5', '6',
	'1', '2', '3',
	MENU_OSK_KEY_NONE, '0', MENU_OSK_KEY_NONE
};

/// Number of columns of the IP OSK menu
#define MENU_OSK_NUMERIC_NEG_COLS	3
/// Number of rows of the IP OSK menu
#define MENU_OSK_NUMERIC_NEG_ROWS	4

/// Numeric entry menu definition
static const char numeric_neg_layout[MENU_OSK_NUMERIC_NEG_ROWS *
MENU_OSK_NUMERIC_NEG_COLS] = {
	'7', '8', '9',
	'4', '5', '6',
	'1', '2', '3',
	'-', '0', MENU_OSK_KEY_NONE
};

struct menu_special_fun {
	const int num;
	const uint8_t keycode[MENU_OSK_NUM_FUNCS];
	const char * const layout[MENU_OSK_NUM_FUNCS];
};

struct menu_osk_flags {
	uint8_t space:1;
	uint8_t shift:1;
	uint8_t reserved:6;
};

struct menu_osk {
	const uint8_t rows;
	const uint8_t cols;
	const uint8_t offset;
	const struct menu_osk_flags flags;
	const char *layout;
};

/// Special function key labels
static const struct menu_special_fun special_func = {
	5,
	{MENU_OSK_KEY_BACK, MENU_OSK_KEY_DEL, MENU_OSK_KEY_LEFT,
		MENU_OSK_KEY_RIGHT, MENU_OSK_KEY_ENTER},
	{"BACK", "DEL", " <-", " ->", "DONE"}
};

static const struct menu_osk osk[MENU_TYPE_OSK_MAX] = {
	{
		MENU_OSK_QWERTY_ROWS,
		MENU_OSK_QWERTY_COLS,
		((MENU_LINE_CHARS - 2 * MENU_OSK_QWERTY_COLS)>>1) - 3,
		{1, 1, 0},
		qwerty_layout
	},
	{
		MENU_OSK_NUMERIC_ROWS,
		MENU_OSK_NUMERIC_COLS,
		((MENU_LINE_CHARS - 2 * MENU_OSK_NUMERIC_COLS)>>1) - 3,
		{0, 0, 0},
		numeric_layout
	},
	{
		MENU_OSK_NUMERIC_NEG_ROWS,
		MENU_OSK_NUMERIC_NEG_COLS,
		((MENU_LINE_CHARS - 2 * MENU_OSK_NUMERIC_COLS)>>1) - 3,
		{0, 0, 0},
		numeric_neg_layout
	},
	{
		MENU_OSK_IPV4_ROWS,
		MENU_OSK_IPV4_COLS,
		((MENU_LINE_CHARS - 2 * MENU_OSK_NUMERIC_COLS)>>1) - 3,
		{0, 0, 0},
		ipv4_layout
	}
};

static char menu_osk_get_std_key(enum menu_osk_type i, uint8_t row, uint8_t col)
{
	return osk[i].layout[(row + menu->coord.caps * 4) * osk[i].cols + col];
}

static char menu_osk_get_current_key(void)
{
	enum menu_osk_type i = menu->instance->entry->osk_entry->osk_type;
	int8_t row = menu->coord.row;
	int8_t col = menu->coord.col;
	char key = MENU_OSK_KEY_NONE;

	if (col == osk[i].cols) {
		key = special_func.keycode[row];
	} else if (row == osk[i].rows) {
		key = ' ';
	} else {
		key = menu_osk_get_std_key(i, row, col);
	}

	return key;
}

/// Draw a key for the current keyboard
static void menu_osk_draw_std_key(enum menu_osk_type i, uint8_t row,
		uint8_t col, uint8_t color, enum menu_placement loc)
{
	unsigned char key;

	key = menu_osk_get_std_key(i, row, col);
	if (key <= MENU_OSK_PRINTABLE_MAX) {
		VdpDrawText(VDP_PLANEA_ADDR, loc + osk[i].offset + 2 * col ,
				MENU_LINE_OSK_KEYS + 2 * row, color, 1,
				(char*)&key, 0);
	}
}

/// Draw a special key for the current keyboard
static void menu_osk_draw_special_key(int key, uint8_t color,
		enum menu_placement loc)
{
	VdpDrawText(VDP_PLANEA_ADDR, loc + 2 * MENU_OSK_QWERTY_COLS +
			6 + 2, MENU_LINE_OSK_KEYS + 2 * key, color, 4,
			special_func.layout[key], '\0');
}

static void menu_osk_draw_space(uint8_t color, enum menu_placement loc)
{
	VdpDrawText(VDP_PLANEA_ADDR, loc +
			((sizeof(qwerty_space_str) - 1)) +
			osk[MENU_TYPE_OSK_QWERTY].offset,
			MENU_LINE_OSK_KEYS + 2 * MENU_OSK_QWERTY_ROWS,
			color, sizeof(qwerty_space_str) - 1,
			qwerty_space_str, '\0');
}

/// Draws current key in menu->coord
static void menu_osk_draw_key(uint8_t color, enum menu_placement loc)
{
	enum menu_osk_type i = menu->instance->entry->osk_entry->osk_type;
	uint8_t row = menu->coord.row;
	uint8_t col = menu->coord.col;
	uint8_t rows = osk[i].rows;
	uint8_t cols = osk[i].cols;

	if (col >= cols) {
		menu_osk_draw_special_key(row, color, loc);
	} else if (row >= rows) {
		menu_osk_draw_space(color,loc);
	} else {
		menu_osk_draw_std_key(i, row, col, color, loc);
	}
}

/// Draw special keys
static void menu_osk_draw_special_keys(enum menu_placement loc)
{
	int8_t row;
	uint8_t color;
	int8_t sel_row = -1;
	int i = menu->instance->entry->osk_entry->osk_type;

	if (menu->coord.col == osk[i].cols) {
		sel_row = menu->coord.row;
	}

	for (row = 0; row < MENU_OSK_NUM_FUNCS; row++) {
		color = row == sel_row?MENU_COLOR_ITEM_SEL:MENU_COLOR_ITEM;
		menu_osk_draw_special_key(row, color, loc);
	}
}

static void menu_osk_draw_std_keys(enum menu_placement loc)
{
	uint8_t i = menu->instance->entry->osk_entry->osk_type;
	uint8_t rows = osk[i].rows;
	uint8_t cols = osk[i].cols;
	uint8_t key_idx;
	uint8_t row, col;

	for (row = 0, key_idx = 0; row < rows; row++) {
		for (col = 0; col < cols; col++, key_idx++) {
			menu_osk_draw_std_key(i, row, col,
					MENU_COLOR_ITEM, loc);
		}
	}

	// Redraw selected key if applicable
	row = menu->coord.row;
	col = menu->coord.col;
	if (row < rows && col < cols) {
		menu_osk_draw_std_key(i, row, col, MENU_COLOR_ITEM_SEL, loc);
	}
}

static void menu_osk_draw_keys(enum menu_placement loc)
{
	struct menu_osk_entry *entry = menu->instance->entry->osk_entry;
	uint8_t i = entry->osk_type;

	// Draw keys
	menu_osk_draw_std_keys(loc);
	menu_osk_draw_special_keys(loc);
	
	if (osk[i].flags.space) {
		menu_osk_draw_space(loc, menu->coord.row >= osk[i].rows?
				MENU_COLOR_ITEM_SEL:MENU_COLOR_ITEM);
	}
}

static void menu_osk_draw_cursor(enum menu_placement loc)
{
	VdpDrawChars(VDP_PLANEA_ADDR, loc + menu->instance->entry->margin +
			menu->cursor, MENU_LINE_OSK_DATA,
			MENU_COLOR_ITEM_SEL, 1, &(char){MENU_OSK_KEY_CURSOR});
}

static void menu_osk_draw(enum menu_placement loc)
{
	struct menu_osk_entry *entry = menu->instance->entry->osk_entry;
	struct menu_str *str = &entry->tmp;

	// Draw caption and input data
	menu_str_line_draw(&entry->caption, MENU_LINE_OSK_FIELD, 0,
			MENU_H_ALIGN_CENTER, MENU_COLOR_OSK_FIELD, loc);
	if (str->length) {
		menu_str_line_draw(str, MENU_LINE_OSK_DATA,
				menu->instance->entry->margin,
				MENU_H_ALIGN_LEFT, MENU_COLOR_OSK_DATA, loc);
	}
	menu_osk_draw_cursor(loc);
	
	// Draw keys
	menu_osk_draw_keys(MENU_PLACE_CENTER);
}

static void menu_osk_shift(void)
{
	enum menu_osk_type i = menu->instance->entry->osk_entry->osk_type;

	if (osk[i].flags.shift) {
		menu->coord.caps ^= 1;
		menu_osk_draw_std_keys(MENU_PLACE_CENTER);
	}
}

static void menu_osk_row_move(int8_t value)
{
	enum menu_osk_type i = menu->instance->entry->osk_entry->osk_type;
	int8_t row = menu->coord.row;
	int8_t col = menu->coord.col;
	uint8_t rows_total = osk[i].rows + osk[i].flags.space;
	uint8_t done = FALSE;
	unsigned char key;

	if (col == osk[i].cols) {
		// We are in special keys
		row += value;
		if (row == MENU_OSK_NUM_FUNCS) {
			row = 0;
		} else if (row < 0) {
			row = MENU_OSK_NUM_FUNCS - 1;
		}
		done = TRUE;
	}
	while (!done) {
		// We are in standard key or space
		row += value;
		if (row >= rows_total) {
			row = 0;
		} else if (row < 0) {
			row = rows_total - 1;
		}
		if (!done) {
			key = menu_osk_get_std_key(i, row, menu->coord.col);
			if (key != MENU_OSK_KEY_NONE) {
				done = TRUE;
			}
		}
	}
	menu->coord.row = row;
}

static void menu_osk_row_change(int8_t value)
{
	menu_osk_draw_key(MENU_COLOR_ITEM, MENU_PLACE_CENTER);
	menu_osk_row_move(value);
	menu_osk_draw_key(MENU_COLOR_ITEM_SEL, MENU_PLACE_CENTER);
}

static void menu_osk_col_move(int8_t value)
{
	enum menu_osk_type i = menu->instance->entry->osk_entry->osk_type;
	int8_t row = menu->coord.row;
	int8_t col = menu->coord.col;
	uint8_t rows = osk[i].rows;
	uint8_t cols = osk[i].cols;
	uint8_t done = FALSE;
	uint8_t rows_total = rows + osk[i].flags.space;
	uint8_t row_max;

	if (col >= cols) {
		// Adjust row if there are more special keys than
		// standard key rows
		if (row >= rows_total) {
			row = rows_total - 1;
		}
	}

	if (row >= rows) {
		// If we are in space, jump to special keys, if we are
		// in special keys jump to space
		if (col >= cols) {
			col = 0;
		} else {
			col = cols;
		}
		done = TRUE;
	}
	while (!done) {
		col += value;
		if (col > cols) {
			col = 0;
		} else if (col < 0) {
			col = cols;
		}
		row_max = col == cols?MENU_OSK_NUM_FUNCS:rows +
			osk[i].flags.space;
		if (row >= row_max) {
			row = row_max;
		}
		if (col == cols) {
			if (row >= MENU_OSK_NUM_FUNCS) {
				row = MENU_OSK_NUM_FUNCS;
			}
			done = TRUE;
		} else {
			if (row >= rows_total) {
				row = rows_total - 1;
			}
			if (MENU_OSK_KEY_NONE != (uint8_t)
				menu_osk_get_std_key(i, row, col)) {
				done = TRUE;
			}
		}
	}
	menu->coord.col = col;
	menu->coord.row = row;
}

static void menu_osk_col_change(int8_t value)
{
	menu_osk_draw_key(MENU_COLOR_ITEM, MENU_PLACE_CENTER);
	menu_osk_col_move(value);
	menu_osk_draw_key(MENU_COLOR_ITEM_SEL, MENU_PLACE_CENTER);
}

static void menu_osk_add_char(char chr)
{
	struct menu_str *tmp = &menu->instance->entry->osk_entry->tmp;

	if (menu->cursor < tmp->max_length) {
		// Make room and add the new character
		for (int i = tmp->length; i > menu->cursor; i--) {
			tmp->str[i] = tmp->str[i - 1];
		}
		tmp->str[menu->cursor] = chr;
		tmp->length++;
		menu->cursor++;

		menu_str_line_draw(tmp, MENU_LINE_OSK_DATA,
				MENU_DEF_LEFT_MARGIN, MENU_H_ALIGN_LEFT,
				MENU_COLOR_OSK_DATA, MENU_PLACE_CENTER);
		menu_osk_draw_cursor(MENU_PLACE_CENTER);
	}

}

static void menu_osk_del_char(void)
{
	struct menu_str *tmp = &menu->instance->entry->osk_entry->tmp;

	if (menu->cursor) {
		menu->cursor--;
		tmp->length--;
		for (int i = menu->cursor; i < tmp->length; i++) {
			tmp->str[i] = tmp->str[i + 1];
		}
		menu_str_line_draw(tmp, MENU_LINE_OSK_DATA,
				MENU_DEF_LEFT_MARGIN, MENU_H_ALIGN_LEFT,
				MENU_COLOR_OSK_DATA, MENU_PLACE_CENTER);
		menu_osk_draw_cursor(MENU_PLACE_CENTER);
	}
}

static void menu_osk_move_chr(int value)
{
	struct menu_str *tmp = &menu->instance->entry->osk_entry->tmp;

	menu->cursor += value;
	if (menu->cursor < 0) {
		menu->cursor = 0;
	} else if (menu->cursor > tmp->length) {
		menu->cursor = tmp->length;
	}
	menu_str_line_draw(tmp, MENU_LINE_OSK_DATA,
			MENU_DEF_LEFT_MARGIN, MENU_H_ALIGN_LEFT,
			MENU_COLOR_OSK_DATA, MENU_PLACE_CENTER);
	menu_osk_draw_cursor(MENU_PLACE_CENTER);
}

static enum menu_osk_ret menu_osk_done(struct menu_entry_instance *instance)
{
	struct menu_str *tmp = &menu->instance->entry->osk_entry->tmp;

	struct menu_osk_entry *entry = instance->entry->osk_entry;
	int err = FALSE;

	tmp->str[tmp->length] = '\0';
	if (instance->entry->action_cb) {
		err = instance->entry->action_cb(instance);
	}
	if (!err && entry->inout_str) {
		entry->inout_str->length = tmp->length + entry->offset;
		memcpy(entry->inout_str->str + entry->offset, tmp->str,
				tmp->length + 1);
		entry->inout_str->str[entry->inout_str->length] = '\0';
	}

	return err?MENU_OSK_ACTION_NONE:MENU_OSK_ACTION_DONE;
}

static enum menu_osk_ret menu_osk_key_input(char key)
{
	struct menu_str *tmp = &menu->instance->entry->osk_entry->tmp;
	enum menu_osk_ret action = MENU_OSK_ACTION_NONE;

	switch((uint8_t)key) {
	case MENU_OSK_KEY_NONE:
		break;

	case MENU_OSK_KEY_BACK:
		// Abort edit and go back
		tmp->length = 0;
		tmp->str[0] = '\0';
		action = MENU_OSK_ACTION_CANCEL;
		break;

	case MENU_OSK_KEY_DEL:
		menu_osk_del_char();
		break;

	case MENU_OSK_KEY_LEFT:
		menu_osk_move_chr(-1);
		break;

	case MENU_OSK_KEY_RIGHT:
		menu_osk_move_chr(1);
		break;

	case MENU_OSK_KEY_ENTER:
		action = menu_osk_done(menu->instance);
		break;

	default:
		menu_osk_add_char(key);
		break;

	}

	return action;
}

enum menu_osk_ret menu_osk_update(uint8_t gp_press)
{
	enum menu_osk_ret action = MENU_OSK_ACTION_NONE;

	// Note: if several keys are simultaneously pressed,
	// only one is parsed
	if (gp_press & GP_START_MASK) {
		action = menu_osk_key_input(MENU_OSK_KEY_ENTER);
	} else if (gp_press & GP_A_MASK) {
		action = menu_osk_key_input(menu_osk_get_current_key());
	} else if (gp_press & GP_B_MASK) {
		action = menu_osk_key_input(MENU_OSK_KEY_DEL);
	} else if (gp_press & GP_C_MASK) {
		menu_osk_shift();
	} else if (gp_press & GP_DOWN_MASK) {
		menu_osk_row_change(1);
	} else if (gp_press & GP_UP_MASK) {
		menu_osk_row_change(-1);
	} else if (gp_press & GP_RIGHT_MASK) {
		menu_osk_col_change(1);
	} else if (gp_press & GP_LEFT_MASK) {
		menu_osk_col_change(-1);
	}

	return action;
}

static void menu_iostr_inherit(void)
{
	struct menu_entry_instance *prev = menu->instance->prev;
	struct menu_item *item = &prev->entry->item_entry->item[prev->sel_item];
	struct menu_osk_entry *entry = menu->instance->entry->osk_entry;


	entry->inout_str = &item->caption;
	entry->offset = item->offset;
}

void menu_osk_enter(void)
{
	struct menu_osk_entry *entry = menu->instance->entry->osk_entry;
	struct menu_str *str = &entry->tmp;

	str->length = 0;
	str->max_length = MIN(entry->line_len, MENU_STR_MAX_LEN);
	if (!entry->inout_str) {
		// Inherit from previous menu level
		menu_iostr_inherit();
	}
	if (entry->inout_str->length > entry->offset) {
		str->length = menu_str_buf_cpy(str->str, entry->inout_str->str +
				entry->offset, str->max_length);
		menu->cursor = str->length;
	} else if (entry->default_str.length) {
		menu_str_cpy(str, &entry->default_str);
	}
	menu->cursor = str->length;
	menu->coord.caps = menu->coord.col = menu->coord.row = 0;

	menu_osk_draw(MENU_PLACE_CENTER);
}

