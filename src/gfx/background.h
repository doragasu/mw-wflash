#ifndef _BACKGROUND_H_
#define _BACKGROUND_H_

#include "../util.h"
#include "font.h"

#define LOGO_TILES_ADDR	(3 * FONT_NCHARS * 32)
#define LED_TILES_ADDR	(LOGO_TILES_ADDR + 2 * LOGO_NUM_TILES * 8 * 2)

void bg_init(void);

void bg_deinit(void);

void bg_hscroll(int16_t px);

void bg_led_draw(uint16_t plane_addr, uint8_t plane_width, uint8_t x, uint8_t y, uint8_t pal);

#endif /*_BACKGROUND_H_*/

