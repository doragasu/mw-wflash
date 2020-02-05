#include "menu_int.h"
#include "menu_itm.h"
#include "../mpool.h"
#include "../gamepad.h"
#include "../util.h"
#include "../snd/sound.h"

static void menu_draw_pager(enum menu_placement loc)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	// Worst case lenth: 3 + 1 + 3 + null_termination
	char pager[8];
	uint8_t i;

	i = uint8_to_str(instance->sel_page + 1, pager);
	pager[i++] = '/';
	i += uint8_to_str(entry->pages, pager + i);
	/// \todo Should we replace this with menu_line_draw()?
	VdpDmaWait();
	VdpDrawText(VDP_PLANEA_ADDR, loc + MENU_LINE_CHARS - 
			MENU_DEF_RIGHT_MARGIN - i, MENU_LINE_PAGER,
			MENU_COLOR_PAGER, i, pager, ' ');
}

void menu_item_clear(enum menu_placement loc)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	uint8_t line = MENU_LINE_ITEM_FIRST;
	uint8_t lines = entry->items_per_page;
	uint8_t spacing = entry->spacing;

	for (int i = 0; i < lines; i++, line += spacing) {
		menu_str_line_clear(loc, line);
	}
}

static void menu_item_single(struct menu_item *item, uint8_t line,
		uint8_t color, enum menu_placement loc)
{
	struct menu_entry_instance *instance = menu->instance;
	struct menu_str str;

	str.str = mp_alloc(item->caption.max_length);
	str.max_length = item->caption.max_length;
	str.length = 0;
	menu_str_cpy(&str, &item->caption);
	if (str.length <= item->offset && item->draw_empty) {
		menu_str_append(&str, "<EMPTY>");
	} else if (item->secure) {
		// Mask field with asterisks
		for (int i = item->offset; i < str.length; i++) {
			str.str[i] = '*';
		}
	}
	menu_str_line_draw(&str, line, instance->entry->margin,
			instance->entry->item_entry->align, color, loc);

	mp_free_to(str.str);
}

void menu_item_draw(enum menu_placement loc)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	struct menu_item *item;
	uint8_t line = MENU_LINE_ITEM_FIRST;
	uint8_t num_items;
	uint8_t top_item;
	uint8_t color;
	int i;

	top_item = instance->sel_page * entry->items_per_page;
	num_items = MIN(entry->items_per_page, entry->n_items - top_item);

	// Only draw non-empty lines
	for (i = 0; i < num_items; i++, line += entry->spacing) {
		item = &entry->item[top_item + i];
		if (item->hidden) {
			menu_str_line_clear(loc, line);
		} else {
			if (instance->sel_item == (top_item + i)) {
				color = MENU_COLOR_ITEM_SEL;
			} else if (item->alt_color) {
				color = MENU_COLOR_ITEM_ALT;
			} else {
				color = MENU_COLOR_ITEM;
			}
			menu_item_single(item, line, color, loc);
		}
	}
	// Clear empty lines
	for (; i < entry->items_per_page; i++, line += entry->spacing) {
		menu_str_line_clear(loc, line);
	}

	// Draw page number if there is more than one page
	if (entry->pages > 1) {
		menu_draw_pager(loc);
	}
}

/// Return the line number of the current selection
static uint8_t menu_item_current_line_num(void)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	uint8_t page_off = instance->sel_page * entry->items_per_page;

	return MENU_LINE_ITEM_FIRST + entry->spacing *
		(instance->sel_item - page_off);
}

static void menu_item_change(enum menu_placement loc, uint8_t item_num)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	uint8_t page_num = item_num / entry->items_per_page;
	uint8_t line;

	if (page_num != instance->sel_page) {
		instance->sel_page = page_num;
		instance->sel_item = item_num;
		menu_item_draw(loc);
	} else if (item_num != instance->sel_item) {
		line = menu_item_current_line_num();
		menu_item_single(&entry->item[instance->sel_item], line,
				MENU_COLOR_ITEM, loc);
		instance->sel_item = item_num;
		line = menu_item_current_line_num();
		menu_item_single(&entry->item[instance->sel_item], line,
				MENU_COLOR_ITEM_SEL, loc);
	}
}

/// Select the previous item
static void menu_item_prev(void)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	uint8_t item_num = instance->sel_item;

	do {
		if (0 == item_num) {
			item_num = entry->n_items - 1;
		} else {
			item_num--;
		}
	} while(entry->item[item_num].not_selectable);
	menu_item_change(MENU_PLACE_CENTER, item_num);
}

/// Select the next item
static void menu_item_next(void)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	uint8_t item_num = instance->sel_item;

	do {
		item_num++;
		if (item_num == entry->n_items) {
			item_num = 0;
		}
	} while(entry->item[item_num].not_selectable);

	menu_item_change(MENU_PLACE_CENTER, item_num);
}

/// Select the next page
static void menu_item_page_up(void)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	uint8_t page_num = instance->sel_page;
	uint8_t item_num;

	page_num++;
	if (page_num >= entry->pages) {
		page_num = 0;
	}
	item_num = page_num * entry->items_per_page;

	while(entry->item[item_num].not_selectable) {
		item_num++;
		if (item_num == entry->n_items) {
			item_num = 0;
		}
	}
	menu_item_change(MENU_PLACE_CENTER, item_num);
	psgfx_play(SFX_MENU_PAGE);
}

/// Select the previous page
static void menu_item_page_down(void)
{
	struct menu_item_entry *entry = menu->instance->entry->item_entry;
	struct menu_entry_instance *instance = menu->instance;
	uint8_t page_num = instance->sel_page;
	uint8_t item_num;

	if (0 == page_num) {
		page_num = entry->pages - 1;
	} else {
		page_num--;
	}
	item_num = page_num * entry->items_per_page;
	while(entry->item[item_num].not_selectable) {
		item_num++;
		if (item_num == entry->n_items) {
			item_num = 0;
		}
	}
	menu_item_change(MENU_PLACE_CENTER, item_num);
	psgfx_play(SFX_MENU_PAGE);
}

static struct menu_entry *menu_accept(struct menu_entry_instance *instance)
{
	struct menu_item_entry *entry = instance->entry->item_entry;
	menu_callback cb = entry->item[instance->sel_item].entry_cb;
	int err = FALSE;
	struct menu_entry *next = NULL;
	uint8_t sel_item = instance->sel_item;

	if (cb) {
		err = cb(instance);
	}
       	if (!err) {
		next = entry->item[sel_item].next;
	}

	return next;
}

int menu_item_update(uint8_t gp_press, struct menu_entry **next)
{
	struct menu_entry_instance *instance = menu->instance;
	int back = 0;

	*next = NULL;

	// Note: if several keys are simultaneously pressed,
	// only one is parsed
	if (gp_press & GP_A_MASK) {
		*next = menu_accept(instance);
	} else if (gp_press & GP_B_MASK) {
		back = instance->entry->item_entry->back_levels;
	} else if (gp_press & GP_C_MASK) {
		if (instance->entry->c_button_cb) {
			instance->entry->c_button_cb(instance);
		}
	} else if (gp_press & GP_DOWN_MASK) {
		menu_item_next();
		psgfx_play(SFX_MENU_ITEM);
	} else if (gp_press & GP_UP_MASK) {
		menu_item_prev();
		psgfx_play(SFX_MENU_ITEM);
	} else if (gp_press & GP_RIGHT_MASK) {
		menu_item_page_up();
	} else if (gp_press & GP_LEFT_MASK) {
		menu_item_page_down();
	}

	return back;
}

int menu_item_enter(void)
{
	int err = 0;

	struct menu_entry_instance *instance = menu->instance;
	struct menu_item_entry *entry = instance->entry->item_entry;

	if (entry->item[instance->sel_item].not_selectable) {
		menu_item_next();
	}

	if (instance->entry->action_cb) {
		err = instance->entry->action_cb(instance);
	}

	menu_str_line_clear(MENU_PLACE_CENTER, MENU_LINE_PAGER);
	menu_item_draw(MENU_PLACE_CENTER);

	return err;
}

