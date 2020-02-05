#include "sound.h"
#include "../loop.h"
#include "../util.h"
#include "../vdp.h"

static int rate_60hz;
static int frame_count;

void psgfx_frame(void);
void tfc_frame(void);

ROM_TEXT(player_cb)
static void player_cb(struct loop_timer *t)
{
	UNUSED_PARAM(t);

	// If running at 60 Hz, skip one in six updates
	if (!rate_60hz || (rate_60hz && (frame_count % 6))) {
		tfc_frame();
		psgfx_frame();
	}
	frame_count++;
}

static struct loop_timer player_timer = {
	.timer_cb = player_cb,
	.frames = 1,
	.auto_reload = TRUE
};

ROM_TEXT(sound_init)
uint16_t sound_init(const uint8_t *tfc_data, const uint8_t *psg_data)
{
	uint16_t rc;

	rate_60hz = VdpIs60Hz();
	loop_timer_add(&player_timer);
	rc = psgfx_init(psg_data);
	tfc_init(tfc_data);
	tfc_play(TRUE);

	return rc;
}

ROM_TEXT(sound_deinit)
void sound_deinit(void)
{
	tfc_play(FALSE);
	psgfx_deinit();
	loop_timer_del(&player_timer);
}
