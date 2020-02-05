#ifndef _LOGO_H_
#define _LOGO_H_

#include "../util.h"

#define LOGO_TILE_WIDTH		32
#define LOGO_TILE_HEIGHT	32
#define LOGO_NUM_TILES		126

extern const uint32_t logo_tiles[LOGO_NUM_TILES * 8];
extern const uint16_t logo_map[LOGO_TILE_WIDTH * LOGO_TILE_HEIGHT];
extern const uint16_t logo_pal[16];

#endif /*_LOGO_H_*/
