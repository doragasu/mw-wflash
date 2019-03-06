/// \todo
/// * Frame number masking for triggering timers on different frames
///   (e.g. on even or odd frames).

#include <string.h>
#include <setjmp.h>
#include "loop.h"
#include "mpool.h"
#include "vdp.h"

enum loop_check {
	LOOP_CHECK_FUNCS = 0,
	LOOP_CHECK_TIMERS
};

struct loop_data {
	struct loop_func **f;
	struct loop_timer **t;
	jmp_buf *env;
	enum loop_check check;
	int8_t func_max;
	int8_t timer_max;
	int8_t idx;
	int8_t vblank;
	uint16_t frame;
	int exit;
};

static struct loop_data *d = NULL;

int loop_init(uint8_t max_func, uint8_t max_timer)
{
	if (d) {
		// Already initialized
		return 0;
	}

	mp_init(0);

	d = mp_alloc(sizeof(struct loop_data));
	memset(d, 0, sizeof(struct loop_data));
	// Allocate pointers for the func/timer structures
	d->f = mp_alloc(max_func  * sizeof(struct loop_func*));
	d->t = mp_alloc(max_timer * sizeof(struct loop_timer*));
	memset(d->f, 0, max_func  * sizeof(struct loop_func*));
	memset(d->t, 0, max_timer * sizeof(struct loop_timer*));
	d->func_max = max_func;
	d->timer_max = max_timer;
	d->vblank = VDP_CTRL_PORT_W & VDP_STAT_VBLANK;

	return 0;
}

int loop_func_add(struct loop_func *func)
{
	int i;

	for (i = 0; i < d->func_max && d->f[i]; i++);
	if (i == d->func_max) return 1;

	d->f[i] = func;
	return 0;
}

int loop_func_del(struct loop_func *func)
{
	struct loop_func *f;
	int i;

	for (i = 0; (f = d->f[i]); i++) {
		if (f == func) {
			// Mark function to be deleted during loop
			f->to_delete = 1;
			return 0;
		}
	}
	return 1;
}

int loop_timer_add(struct loop_timer *timer)
{
	int i;

	for (i = 0; i < d->timer_max && d->t[i]; i++);
	if (i == d->timer_max) return 1;

	d->t[i] = timer;
	return 0;
}

int loop_timer_del(struct loop_timer *timer)
{
	struct loop_timer *t;
	int i;

	for (i = 0; (t = d->t[i]); i++) {
		if (t == timer) {
			// Mark function to be deleted during loop
			t->to_delete = 1;
			return 0;
		}
	}
	return 1;
}

static int frame_update(void)
{
	int vblank = VDP_CTRL_PORT_W & VDP_STAT_VBLANK;
	int rc = 0;

	if (!d->vblank && vblank) {
		rc = 1;
		d->frame++;
	}
	d->vblank = vblank;
	return rc;
}

static void delete_func(int i)
{
	while (d->f[i + 1]) {
		d->f[i] = d->f[i + 1];
		i++;
	}
	d->f[i] = NULL;
}

static void run_funcs(void)
{
	struct loop_func *f;

	while (d->idx < d->func_max && (f = d->f[d->idx])) {
		if (f->to_delete) {
			f->to_delete = 0;
			delete_func(d->idx);
		} else {
			d->idx++;
			if (!f->disabled) {
				f->func_cb(f);
			}
		}
	}
}

static void delete_timer(int i)
{
	while (d->t[i + 1]) {
		d->t[i] = d->t[i + 1];
		i++;
	}
	d->t[i] = NULL;
}

static void update_timer(struct loop_timer *t)
{
	if (t->frames) {
		t->count++;
		if (t->count >= t->frames) {
			if (t->auto_reload) {
				t->count = 0;
			} else {
				t->frames = 0;
			}
			t->timer_cb(t);
		}
	}
}

static void check_timers(void)
{
	struct loop_timer *t;

	while (d->idx < d->timer_max && (t = d->t[d->idx])) {
		if (t->to_delete) {
			t->to_delete = 0;
			delete_timer(d->idx);
		} else {
			d->idx++;
			update_timer(t);
		}
	}
}

int loop(void)
{
	while (!d->exit) {
		if (LOOP_CHECK_FUNCS == d->check) {
			run_funcs();
			d->idx = 0;
		}
		if (frame_update()) {
			d->check = LOOP_CHECK_TIMERS;
			check_timers();
			d->idx = 0;
			d->check = LOOP_CHECK_FUNCS;
		}
	}

	return d->exit;
}

void loop_deinit(void)
{
	if (!d) return;

	mp_free_to(d);
	d = NULL;
}

int loop_pend(void)
{
	jmp_buf env;
	jmp_buf *prev;
	int returned;

	prev = d->env;
	returned = setjmp(env);
	if (!returned) {
		d->env = &env;
		loop();
		// Loop call above should not return
	}
	// Restore environment before the setjmp() call
	d->env = prev;

	return returned;
}

void loop_post(int return_value)
{
	if (d->env) {
		longjmp(*d->env, return_value);
	}
}

void loop_end(int return_value)
{
	d->exit = return_value;
}

