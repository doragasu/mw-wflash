#ifndef _MENU_H_
#define _MENU_H_

#include "menu_def.h"

/// Compute the number of items fitting one page
#define MENU_ITEMS_PER_PAGE(items, spacing)			\
	MIN(1 + MENU_ITEM_LINES/(spacing), items)

/// Compute the number of pages needed for a menu item entry
#define MENU_PAGES(items, spacing)				\
	(1 + (items - 1)/MENU_ITEMS_PER_PAGE(items, spacing))

/// Macro to ease building menu item entries
#define MENU_ITEM_ENTRY(items, item_spacing, item_align)		\
	(struct menu_item_entry*)&(const struct menu_item_entry) {	\
	.n_items = items,						\
	.spacing = item_spacing,					\
	.items_per_page = MENU_ITEMS_PER_PAGE(items, item_spacing),	\
	.pages = MENU_PAGES(items, item_spacing),			\
	.align = item_align,						\
	.item = (struct menu_item*)(const struct menu_item[items])

#define MENU_ITEM_ENTRY_END }

#define MENU_OSK_ENTRY (struct menu_osk_entry*)&(const struct menu_osk_entry)

/************************************************************************//**
 * Initializes menu subsystem, and sets the status string.
 *
 * \param[in] root   Top level menu entry.
 * \param[in] status Status string.
 ****************************************************************************/
void menu_init(const struct menu_entry *root, struct menu_str *status);

/************************************************************************//**
 * Sets the status string and updates it on the screen.
 *
 * \param[in] status Status string.
 ****************************************************************************/
void menu_stat_str_set(struct menu_str *status);

/************************************************************************//**
 * Updates the menus.
 *
 * This function updates the menu according to gamepad input. Must be called
 * once per frame, preferably at the beginning of the vertical blanking
 * interval.
 *
 * \param[in] gp_press Key press events, in format SACBRLDU (see gamepad.h).
 ****************************************************************************/
void menu_update(uint8_t gp_press);

/************************************************************************//**
 * Enter the specified menu entry.
 *
 * \param[in] next Menu entry to be entered.
 *
 * \note This function is rarely needed since menu navigation is automatically
 * handling using the definitions in the menu_item structures. The notable
 * exception to this rule are message boxes.
 ****************************************************************************/
void menu_enter(const struct menu_entry *next);

void menu_msg(const char *title, const char *caption,
		enum menu_msg_flags flags, uint16_t wait_frames);

/************************************************************************//**
 * Go back one or more menu levels.
 *
 * \param[in] levels Number of levels to go back.
 *
 * \note a levels value of 0, is interpreted as 1 level.
 ****************************************************************************/
void menu_back(int levels);

/************************************************************************//**
 * Go back to the root menu.
 ****************************************************************************/
void menu_reset(void);

#endif /*_MENU_H_*/

