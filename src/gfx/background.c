#include "../vdp.h"
#include "../loop.h"
#include "logo.h"
#include "led.h"
#include "background.h"

static int16_t hpos;
static int16_t vpos;

static void scroll_cb(struct loop_timer *t)
{
	UNUSED_PARAM(t);

	VdpDmaWait();
	VdpRamWrite(VDP_VSRAM_WR, 2, vpos++);
	VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR + 2, hpos++);
}

static struct loop_timer scroll_timer = {
	.timer_cb = scroll_cb,
	.frames = 2,
	.auto_reload = TRUE
};

void bg_init(void)
{
	VdpDisable();
	VdpTilesLoad(logo_tiles, LOGO_TILES_ADDR, LOGO_NUM_TILES * 8 * 2);
	VdpTilesLoad(led_tiles, LED_TILES_ADDR, LED_NUM_TILES * 8 * 2);
	VdpPalLoad(logo_pal, 1);
	VdpPalLoad(led_green_pal, 2);
	VdpPalLoad(led_green_pal, 3);
	for (int i = 3; i >= 0; i--) {
		VdpPalFadeOut(3);
	}
	for (uint8_t i = 0; i < 4; i++) {
		VdpMapLoad(logo_map, VDP_PLANEB_ADDR + i * (32 * 2), 32, 32,
				128, 0x120, 1);
	}
	VdpEnable();

	loop_timer_add(&scroll_timer);
}

void bg_deinit(void)
{
	loop_timer_del(&scroll_timer);
}

void bg_hscroll(int16_t px)
{
	hpos += px;
	VdpDmaWait();
	VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR + 2, hpos);
}

void bg_led_draw(uint16_t plane_addr, uint8_t plane_width, uint8_t x, uint8_t y, uint8_t pal)
{
	uint16_t offset = plane_addr + 2 * (x + plane_width * y);
	uint16_t tile_addr = 3 * FONT_NCHARS + LOGO_NUM_TILES;

	VdpRamRwPrep(VDP_VRAM_WR, offset);
	VDP_DATA_PORT_W = tile_addr++ + (pal<<13);
	VDP_DATA_PORT_W = tile_addr++ + (pal<<13);
	offset += 2 * plane_width;
	VdpRamRwPrep(VDP_VRAM_WR, offset);
	VDP_DATA_PORT_W = tile_addr++ + (pal<<13);
	VDP_DATA_PORT_W = tile_addr   + (pal<<13);
}

