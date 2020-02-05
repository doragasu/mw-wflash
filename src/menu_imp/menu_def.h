/* Menu definitions */

#ifndef _MENU_DEF_H_
#define _MENU_DEF_H_

#include "menu_str.h"

#define MENU_STATUS_MAX_CHR	12

/// back_level is set to this value, go back to root menu
#define MENU_BACK_ALL		7

enum PACKED menu_msg_flags {
	MENU_MSG_BUTTON_YES = 1,
	MENU_MSG_BUTTON_NO = 2,
	MENU_MSG_BUTTON_OK = 4,
	MENU_MSG_BUTTON_CANCEL = 8,
	MENU_MSG_MODAL = 8
};

struct menu_data;
struct menu_entry_instance;

typedef int (*menu_callback)(struct menu_entry_instance *instance);

enum PACKED menu_type {
	MENU_TYPE_ITEM = 0,
	MENU_TYPE_OSK,
	MENU_TYPE_MSG,
	MENU_TYPE_MAX
};

enum PACKED menu_osk_type {
	MENU_TYPE_OSK_QWERTY = 0,
	MENU_TYPE_OSK_NUMERIC,
	MENU_TYPE_OSK_NUMERIC_NEG,
	MENU_TYPE_OSK_IPV4,
	MENU_TYPE_OSK_MAX
};

enum menu_button {
	MENU_BUTTON_A,
	MENU_BUTTON_B,
	MENU_BUTTON_C,
	MENU_BUTTON_START,
	MENU_BUTTON_MAX
};

struct menu_item {
	struct menu_str caption;
	struct menu_entry *next;
	menu_callback entry_cb;
	struct {
		uint8_t not_selectable:1;
		uint8_t alt_color:1;
		uint8_t hidden:1;
		uint8_t secure:1;
		uint8_t draw_empty:1;
	};
	uint8_t offset;
};

struct menu_item_entry {
	struct {
		uint8_t n_items;
		uint8_t pages;
		uint8_t items_per_page:5;
		uint8_t back_levels:3;
		uint8_t spacing:4;
		enum menu_h_align align:2;
	};
	struct menu_item *item;
};

/// On Screen Keyboard menu entry. When menu enters, if inout_str is not NULL
/// and is not an empty string, the editable string will be filled with it.
/// Otherwise, if default_str is not empty, it will be used. If default_str is
/// also empty, editable string will be empty.
/// User edited string is copied in menu->tmp_str. If inout_str exists, the
/// string will also be copied to inout_str.
struct menu_osk_entry {
	struct menu_str tmp;
	struct menu_str caption;
	struct menu_str default_str;
	struct menu_str *inout_str;
	uint8_t offset;
	uint8_t line_len;
	enum menu_osk_type osk_type;
};

struct menu_msg_entry {
	struct menu_str caption;
	uint16_t tout_frames;
	enum menu_msg_flags flags;
	uint16_t addr;
	uint8_t width;
	uint8_t height;
};

/// About callbacks:
/// * enter_cb is run on menu entering, after copying the struct menu_entry to
///   RAM, so the callback can perform initialization routines when needed.
///   The callback must return 0, or the new menu will not be entered.
/// * exit_cb is run on menu exiting.
/// * c_button_cb is run when C button is pressed while in the menu.
struct menu_entry {
	enum menu_type type;
	uint8_t margin;
	struct menu_str title;
	struct menu_str left_context;
	menu_callback enter_cb;
	menu_callback exit_cb;
	menu_callback periodic_cb;
	menu_callback action_cb;
	menu_callback c_button_cb;
	union {
		struct menu_item_entry *item_entry;
		struct menu_osk_entry *osk_entry;
		struct menu_msg_entry *msg_entry;
	};
};

/// Holds the data required for a menu_entry instance
struct menu_entry_instance {
	struct menu_entry *entry;
	struct menu_entry_instance *prev;
	uint8_t sel_item;
	uint8_t sel_page;
};

#endif /*_MENU_DEF_H_*/

