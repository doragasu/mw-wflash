#include <string.h>
#include "menu.h"
#include "menu_int.h"
#include "menu_itm.h"
#include "menu_osk.h"
#include "menu_msg.h"
#include "../mpool.h"
#include "../gamepad.h"

/// The global menu instance
struct menu_instance *menu;

/// Number of steps to scroll a complete menu screen
const uint8_t anim_delta[] = {
	44, 40, 36, 32, 28, 25, 21, 18, 16, 13, 11, 9, 7, 6, 5, 3, 2, 2, 1, 1
};

#define MENU_ANIM_STEPS		(sizeof(anim_delta)/sizeof(uint8_t))

#define MENU_SCROLL(offset)	do {				\
	VdpDmaWait();						\
	VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR, offset);	\
} while(0)

static void *alloc_cpy(const void *org, size_t length)
{
	void *cpy;

	cpy = mp_alloc(length);
	if (cpy) {
		memcpy(cpy, org, length);
	}

	return cpy;
}

/// Copies a menu screen area
static void menu_copy(enum menu_placement dst, enum menu_placement org)
{
	for (int i = 0; i < MENU_LINES; i++) {
		menu_str_line_copy(dst, org, i);
	}
}

/// Clears a menu screen area
static void menu_clear(enum menu_placement loc)
{
	for (int i = 0; i < MENU_LINES; i++) {
		menu_str_line_clear(loc, i);
	}
}

static void menu_draw_context(enum menu_placement loc)
{
	struct menu_entry *entry = menu->instance->entry;

	// Draw title
	menu_str_line_draw(&entry->title, MENU_LINE_TITLE, 0,
			MENU_H_ALIGN_CENTER, MENU_COLOR_TITLE, loc);
	// Draw left and right context
	menu_str_line_draw(&entry->left_context, MENU_LINE_CONTEXT,
			MENU_DEF_LEFT_MARGIN, MENU_H_ALIGN_LEFT,
			MENU_COLOR_CONTEXT_L, loc);
	menu_str_pos_draw(&menu->right_context, loc + MENU_LINE_CHARS -
			entry->margin - menu->right_context.length,
			MENU_LINE_CONTEXT, MENU_STATUS_MAX_CHR,
			MENU_COLOR_CONTEXT_R, loc);
}

static void menu_alloc_items(struct menu_item_entry *entry)
{
	struct menu_str *caption;

	for (int i = 0; i < entry->n_items; i++) {
		caption = &entry->item[i].caption;
		// Do not copy R/O strings
		if (menu_str_is_rw(caption)) {
			caption->str = alloc_cpy(caption->str,
					caption->max_length + 1);
		}
	}
}

static void menu_alloc_osk(struct menu_osk_entry *entry)
{
	// Copy input string to temporal buffer, for it to be edited
	// No memory is allocated for this operation
	menu_str_cpy(&entry->tmp, &entry->default_str);
}

static void menu_alloc_msg(struct menu_msg_entry *entry)
{
	struct menu_str *caption = &entry->caption;

	// Only allocate and copy string if it is R/W
	if (menu_str_is_rw(caption)) {
		caption->str = alloc_cpy(caption->str, caption->max_length + 1);
	}
}

static struct menu_entry *menu_alloc_cpy(const struct menu_entry *entry)
{
	struct menu_entry *copy;
	size_t len;

	len = sizeof(struct menu_entry);
	copy = alloc_cpy(entry, len);

	switch (entry->type) {
	case MENU_TYPE_ITEM:
		// Make room for item entry and items
		copy->item_entry = alloc_cpy(entry->item_entry,
				sizeof(struct menu_item_entry));
		len = entry->item_entry->n_items * sizeof(struct menu_item);
		copy->item_entry->item =
			alloc_cpy(entry->item_entry->item, len);
		break;

	case MENU_TYPE_OSK:
		// Make room for OSK entry and edit buffer
		copy->osk_entry = alloc_cpy(entry->osk_entry,
				sizeof(struct menu_osk_entry));
		copy->osk_entry->tmp.str =
			mp_alloc(copy->osk_entry->line_len + 1);
		copy->osk_entry->tmp.max_length = copy->osk_entry->line_len;
		copy->osk_entry->tmp.length = 0;
		break;

	case MENU_TYPE_MSG:
		// Make room for message entry
		copy->msg_entry = alloc_cpy(entry->msg_entry,
				sizeof(struct menu_msg_entry));
		break;

	default:
		break;
	}

	// Allocate required memory for menu strings
	switch (copy->type)
	{
	case MENU_TYPE_ITEM:
		menu_alloc_items(copy->item_entry);
		break;

	case MENU_TYPE_OSK:
		menu_alloc_osk(copy->osk_entry);
		break;

	case MENU_TYPE_MSG:
		menu_alloc_msg(copy->msg_entry);
		break;
		
	default:
		break;
	}

	return copy;
}

void menu_init(const struct menu_entry *root, struct menu_str *status)
{
	menu = mp_calloc(sizeof(struct menu_instance));
	menu->right_context.str = menu->context_buf;
	menu->right_context.max_length = MENU_STATUS_MAX_CHR;
	menu_str_cpy(&menu->right_context, status);

	// Enter root menu
	menu_enter(root);
}

void menu_stat_str_set(struct menu_str *status)
{
	menu_str_cpy(&menu->right_context, status);
	menu_draw_context(MENU_PLACE_CENTER);
}

static void menu_animate(void)
{
	menu->offset += MENU_STAT_ENTERING == menu->stat?
		-anim_delta[menu->anim_step]:anim_delta[menu->anim_step];
	MENU_SCROLL(menu->offset);
	menu->anim_step++;
	if (menu->anim_step >= MENU_ANIM_STEPS ) {
		MENU_STAT_CHANGE(MENU_STAT_IDLE);
	}
}

static void menu_animation_start(void)
{
	// Enter the menu
	MENU_STAT_CHANGE(MENU_STAT_ENTERING);
	menu->anim_step = 0;
	// Copy current menu to the left shadow screen, and
	// clear previous menu
	if (menu->instance) {
		menu_copy(MENU_PLACE_LEFT, MENU_PLACE_CENTER);
		menu_item_clear(MENU_PLACE_CENTER);
	}

	// Move to the area of the copied menu
	menu->offset = VDP_SCREEN_WIDTH_PX;
	MENU_SCROLL(menu->offset);
}

void menu_enter(const struct menu_entry *next)
{
	struct menu_entry_instance *instance;
	struct menu_entry *entry;
	int err = FALSE;

	// Allocate and fill new menu instance
	instance = mp_alloc(sizeof(struct menu_entry_instance));

	// Draw new entry in the centered (not visible) area
	entry = menu_alloc_cpy(next);
	instance->entry = entry;
	instance->prev = menu->instance;
	if (instance->entry->enter_cb) {
		err = instance->entry->enter_cb(instance);
	}
	if (!err) {
		if (entry->type != MENU_TYPE_MSG) {
			menu_animation_start();
		}
		// Enter the menu
		menu->instance = instance;
		switch (entry->type) {
		case MENU_TYPE_ITEM:
			menu_draw_context(MENU_PLACE_CENTER);
			instance->sel_item = instance->sel_page = 0;
			menu_item_enter();
			break;

		case MENU_TYPE_OSK:
			menu_draw_context(MENU_PLACE_CENTER);
			menu_osk_enter();
			break;

		case MENU_TYPE_MSG:
			instance->sel_item = 0;
			menu_msg_enter();
			break;

		default:
			break;
		}
		menu->level++;
	} else {
		// Init failed, roll back
		mp_free_to(instance);
	}
}

static void menu_back_animation_start(void)
{
	MENU_STAT_CHANGE(MENU_STAT_EXITING);

	// Copy menu for scrolling, and clear previous one
	menu_copy(MENU_PLACE_RIGHT, MENU_PLACE_CENTER);
	menu->offset = -VDP_SCREEN_WIDTH_PX;
	MENU_SCROLL(menu->offset);
	menu_clear(MENU_PLACE_CENTER);
	menu->anim_step = 0;
}

void menu_back(int levels)
{
	struct menu_entry_instance *to_dealloc = NULL;
	enum menu_type type = menu->instance->entry->type;

	if (!menu->instance->prev) {
		return;
	}
	if (MENU_TYPE_MSG == type) {
		menu_msg_restore();
	} else {
		menu_back_animation_start();
	}

	menu->level -= levels;
	while (levels--) {
		to_dealloc = menu->instance;
		if (to_dealloc->entry->exit_cb) {
			to_dealloc->entry->exit_cb(to_dealloc);
		}
		menu->instance = to_dealloc->prev;
	}
	mp_free_to(to_dealloc);
	menu_draw_context(MENU_PLACE_CENTER);
	if (MENU_TYPE_MSG != type) {
		menu_item_enter();
	}
}

// FIXME Rewrite using menu_draw()
static void menu_update_item(uint8_t gp_press)
{
	struct menu_entry *next = NULL;
	int back = FALSE;

	back = menu_item_update(gp_press, &next);

	if (back) {
		menu_back(1);
	} else if (next) {
		menu_enter(next);
	}
}

void menu_reset(void)
{
	menu_back(menu->level - 1);
}

static void menu_update_osk(uint8_t gp_press)
{
	int action;

	action = menu_osk_update(gp_press);
	switch (action) {
	case MENU_OSK_ACTION_DONE:
		menu_back(1);
		break;

	case MENU_OSK_ACTION_CANCEL:
		menu_back(1);
		break;

	default:
		break;
	}
}

static void menu_update_msg(uint8_t gp_press)
{
	enum menu_msg_action action;

	action = menu_msg_update(gp_press);

	if (action) {
		menu_back(1);
	}
}

void menu_update(uint8_t gp_press)
{
	// Perform entry/exit animation when appropriate
	switch (menu->stat) {
	case MENU_STAT_IDLE:
		// Call corresponding menu item/osk update. The update function
		// returns the entry/exit event
		switch (menu->instance->entry->type) {
		case MENU_TYPE_ITEM:
			menu_update_item(gp_press);
			break;

		case MENU_TYPE_OSK:
			menu_update_osk(gp_press);
			break;

		case MENU_TYPE_MSG:
			menu_update_msg(gp_press);

		default:
			break;
		}
		
		
		// Perform entry/exit if requested
		break;

	case MENU_STAT_ENTERING:
		// flowdown
	case MENU_STAT_EXITING:
		menu_animate();
		break;

	case MENU_STAT_BUSY:
		break;

	default:
		break;

	}
}

void menu_msg(const char *title, const char *caption,
		enum menu_msg_flags flags, uint16_t wait_frames)
{
	struct menu_msg_entry msg = {
		.tout_frames = wait_frames,
		.flags = flags
	};

	struct menu_entry entry = {
		.type = MENU_TYPE_MSG,
		.msg_entry = &msg
	};

	// Set strings as R/W, for the menu initialization functions to
	// copy the strings (otherwise, only a pointer is taken)
	entry.title.str = (char*)title;
	entry.title.length = entry.title.max_length = strlen(title);
	entry.msg_entry->caption.str = (char*)caption;
	entry.msg_entry->caption.length = entry.msg_entry->caption.max_length =
		strlen(caption);

	menu_enter(&entry);
}

