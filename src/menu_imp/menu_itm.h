#ifndef _MENU_ITEM_H_
#define _MENU_ITEM_H_

#include "menu_def.h"

/************************************************************************//**
 * Initializes and enters a newly allocated menu entry.
 *
 * The allocated menu entry must be the one pointed by the current menu
 * instance.
 *
 * \return The value returned by the enter_cb, or 0 if no callback was run.
 ****************************************************************************/
int menu_item_enter(void);

/************************************************************************//**
 * Updates the menu item according to gamepad input.
 *
 * Must be called once per frame, preferably at the beginning of the vertical
 * blanking interval.
 *
 * \param[in]  gp_press Key press events, in format SACBRLDU (see gamepad.h).
 * \param[out] next     Pointer to the next menu entry, or NULL if no
 *             transition
 *
 * \return TRUE if user requested to go back a menu level, FALSE otherwise.
 ****************************************************************************/
int menu_item_update(uint8_t gp_press, struct menu_entry **next);

/************************************************************************//**
 * Draws the corresponding item page.
 *
 * Other than the items in the drawn page, nothing is modified on the screen.
 * \param[in] loc Location in which the menu will be cleared.
 *
 * \note This functions checks for DMA to be inactive on enter.
 * \warning DMA active when this function exits.
 ****************************************************************************/
void menu_item_draw(enum menu_placement loc);

/************************************************************************//**
 * Clears the text of a menu in the specified location
 *
 * Only the lines containing text are cleared. Other areas of the screen are
 * untouched.
 *
 * \param[in] loc Location in which the menu will be cleared.
 *
 * \note This functions checks for DMA to be inactive on enter.
 * \warning DMA active when this function exits.
 ****************************************************************************/
void menu_item_clear(enum menu_placement loc);

#endif /*_MENU_ITEM_H_*/

