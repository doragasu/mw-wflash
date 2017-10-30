#ifndef _WF_MENU_H_
#define _WF_MENU_H_

#include "menu.h"
#include "mpool.h"

void WfMenuInit(MenuString statStr);

/// Converts an IP address in uint32_t binary representation to
/// string representation. Returns resulting string length.
uint8_t MenuBin2IpStr(uint32_t addr, char str[]);

#endif

