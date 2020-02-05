#include "../vdp.h"
#include "../loop.h"
#include "../font.h"
#include "logo.h"
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
	VdpTilesLoad(logo_tiles, 3 * fontChars * 32, LOGO_NUM_TILES * 8 * 2);
	VdpPalLoad(logo_pal, 1);
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

