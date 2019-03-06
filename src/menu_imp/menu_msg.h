#ifndef _MENU_MSG_H_
#define _MENU_MSG_H_

#include <stdint.h>
#include "menu_def.h"

enum menu_msg_action {
	MENU_MSG_ACTION_CLOSE = -2,
	MENU_MSG_ACTION_TIMEOUT = -1,
	MENU_MSG_ACTION_NONE = 0,
	MENU_MSG_ACTION_CANCEL = 1,
	MENU_MSG_ACTION_YES_OK = 2,
	MENU_MSG_ACTION_NO = 3
};

void menu_msg_enter(void);

enum menu_msg_action menu_msg_update(uint8_t gp_press);

void menu_msg_close(void);

void menu_msg_restore(void);

#endif /*_MENU_MSG_H_*/

