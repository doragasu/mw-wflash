#ifndef _MENU_OSK_H_
#define _MENU_OSK_H_

#include "menu_def.h"

enum menu_osk_ret {
	MENU_OSK_ACTION_NONE = 0,
	MENU_OSK_ACTION_DONE,
	MENU_OSK_ACTION_CANCEL,
	MENU_OSK_ACTION_MAX
};

/// Keycodes for special characters, including space key
#define MENU_OSK_KEY_NONE    0xFF
#define MENU_OSK_KEY_BACK    0xFE
#define MENU_OSK_KEY_DEL     0xFD
#define MENU_OSK_KEY_LEFT    0xFC
#define MENU_OSK_KEY_RIGHT   0xFB
#define MENU_OSK_KEY_ENTER   0xFA
#define MENU_OSK_KEY_CURSOR  0x7F
#define MENU_OSK_KEY_SPACE   ' '

/// Only characters lower or equal to this value are printable
#define MENU_OSK_PRINTABLE_MAX 	127

/************************************************************************//**
 * Enters an OSK menu.
 *
 * Renders the complete OSK screen corresponding to the current menu
 * OSK entry.
 ****************************************************************************/
void menu_osk_enter(void);

/************************************************************************//**
 * Updates the on-screen-keyboard menu according to gamepad input.
 *
 * Must be called once per frame, preferably at the beginning of the vertical
 * blanking interval.
 *
 * \param[in]  gp_press Key press events, in format SACBRLDU (see gamepad.h).
 *
 * \return Action required due to the user menu input:
 * - MENU_OSK_ACTION_NONE: No action required.
 * - MENU_OSK_ACTION_DONE: Text editing complete.
 * - MENU_OSK_ACTION_CANCEL: User has cancelled the input dialog.
 ****************************************************************************/
enum menu_osk_ret menu_osk_update(uint8_t gp_press);

#endif /*_MENU_OSK_H_*/

