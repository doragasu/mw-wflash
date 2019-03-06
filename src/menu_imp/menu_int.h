#ifndef _MENU_INTERNAL_H_
#define _MENU_INTERNAL_H_

#include "menu_def.h"

#define MENU_STAT_CHANGE(new_stat)	menu->stat = new_stat

/// Internal menu state
enum menu_stat {
	MENU_STAT_IDLE = 0,
	MENU_STAT_ENTERING,
	MENU_STAT_EXITING,
	MENU_STAT_BUSY,
	MENU_STAT_MAX
};

/// Coordinates of a key for an OSK
struct menu_osk_coord {
	uint8_t caps:1;	///< Set to 1 for CAPS key
	uint8_t row:3;	///< Keyboard row
	uint8_t col:4;	///< Keyboard column
};

/// Holds the data required for a menu instance
struct menu_instance {
	enum menu_stat stat;			///< The menu status
	uint16_t offset;			///< Offset for animations
	uint16_t anim_step;			///< Step used for animations
	struct menu_entry_instance *instance;	///< Current menu instance
	struct menu_str right_context;		///< Context string, right side
	char context_buf[MENU_LINE_CHARS];	///< Context string buffer
	struct menu_osk_coord coord;		///< Coords for OSK menus
	int8_t cursor;				///< Cursor position
	uint8_t level;				///< Menu level
};

/// The global menu instance
extern struct menu_instance *menu;

#endif /*_MENU_INTERNAL_H_*/

