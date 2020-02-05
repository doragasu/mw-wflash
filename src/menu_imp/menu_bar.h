#ifndef _MENU_BAR_H_
#define _MENU_BAR_H_

#include <stdint.h>
#include <stdbool.h>
#include "menu_str.h"

/************************************************************************//**
 * Initializes the bar to draw.
 *
 * The bar is rendered into the specified menu_str structure, and uses all
 * the available space in that string.
 *
 * \param[in] str     Menu string to write the bar to.
 * \param[in] max     Maximum value, that represents 100% of the bar.
 * \param[in] reverse Maximum value, that represents 100% of the bar.
 ****************************************************************************/
void menu_bar_init(struct menu_str *str, uint16_t max, bool reverse);

/************************************************************************//**
 * Updates the menu_str structure associated to the bar, with the specified
 * value.
 *
 * \param[in] pos Position (relative to max) of the bar.
 ****************************************************************************/
void menu_bar_update(uint16_t pos);

#endif /*_MENU_BAR_H_*/

