#ifndef _MENU_DL_H_
#define _MENU_DL_H_
#include "../menu_imp/menu_def.h"

extern const struct menu_entry download_menu;

/// To be implemented by the module handling downloads
extern int download_mode_menu_cb(struct menu_entry_instance *instance);

#endif /*_MENU_DL_H_*/

