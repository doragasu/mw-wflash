#include <string.h>
#include "menu.h"
#include "menu_int.h"
#include "../util.h"
#include "../gamepad.h"
#include "menu_msg.h"

enum {
	BUTTON_YES = 0,
	BUTTON_NO,
	BUTTON_OK,
	BUTTON_CANCEL,
	BUTTON_MAX
};

//static const char *button[BUTTON_MAX] = {"YES", "NO", "OK", "CANCEL"};
static const uint8_t button_length[BUTTON_MAX] = {3, 2, 2, 6};

static uint8_t buttons_length_get(const struct menu_msg_entry *entry)
{
	uint8_t buttons_length = 0;
	
	if (entry->flags & MENU_MSG_BUTTON_YES) {
		buttons_length += button_length[BUTTON_YES];
	}
	if (entry->flags & MENU_MSG_BUTTON_NO) {
		if (buttons_length) {
			buttons_length++;
		}
		buttons_length += button_length[BUTTON_NO];
	}
	if (entry->flags & MENU_MSG_BUTTON_OK) {
		if (buttons_length) {
			buttons_length++;
		}
		buttons_length += button_length[BUTTON_OK];
	}
	if (entry->flags & MENU_MSG_BUTTON_CANCEL) {
		if (buttons_length) {
			buttons_length++;
		}
		buttons_length += button_length[BUTTON_CANCEL];
	}

	return buttons_length;
}

static void frame_draw(uint16_t addr, uint8_t hor_length,
		uint8_t ver_length)
{
	int i;

	VdpDmaWait();
	VdpDmaVRamFill(addr + 2, hor_length, 2, 0);
	VdpDmaWait();
	VdpDmaVRamFill(addr, hor_length - 1, 2, 0x5F00);
	VdpDmaWait();
	VdpRamWrite(VDP_VRAM_WR, addr, 0x5F);
	addr += 2 * VDP_PLANE_HTILES;

	for (i = 0; i < ver_length; i++, addr += 2 * VDP_PLANE_HTILES) {
		VdpRamWrite(VDP_VRAM_WR, addr, 0x5F);
		VdpDmaVRamFill(addr + 2, 2 * (hor_length - 2), 1, 0x00);
		VdpDmaWait();
		VdpRamWrite(VDP_VRAM_WR, addr + 2 * (hor_length - 1), 0x5F);
	}
		
	VdpDmaWait();
	VdpDmaVRamFill(addr + 2, hor_length, 2, 0);
	VdpDmaWait();
	VdpDmaVRamFill(addr, hor_length - 1, 2, 0x5F00);
	VdpDmaWait();
	VdpRamWrite(VDP_VRAM_WR, addr, 0x5F);
}

//static void buttons_draw(uint8_t selected, const struct menu_msg_entry *entry)
//{
//	uint8_t buttons_length = buttons_length_get(entry);
//}
//

static void rect_copy(uint16_t org_addr, uint16_t dst_addr,
		int16_t hor_length, uint16_t ver_length)
{
	for (uint16_t i = 0; i < (ver_length + 2); i++) {
		VdpDmaWait();
		VdpDmaVRamCopy(org_addr, dst_addr, hor_length * 2);
		org_addr += VDP_PLANE_HTILES * 2;
		dst_addr += VDP_PLANE_HTILES * 2;
	}
}

void menu_msg_restore(void)
{
	struct menu_msg_entry *msg = menu->instance->entry->msg_entry;

	rect_copy(msg->addr + 2 * MENU_LINE_CHARS, msg->addr, msg->width,
			msg->height);
}

void menu_msg_enter(void)
{
	struct menu_entry *entry = menu->instance->entry;
	struct menu_msg_entry *msg = entry->msg_entry;
	uint8_t but_length = buttons_length_get(entry->msg_entry);
	if (!msg->width) {

		msg->width = 4 + MAX(but_length, MAX(entry->title.length,
				entry->msg_entry->caption.length));
	}
	if (!msg->height) {
		msg->height = but_length?7:5;
	}

	uint8_t x_caption = (MENU_LINE_CHARS -
			entry->msg_entry->caption.length) / 2;
	uint8_t x = (MENU_LINE_CHARS - msg->width) / 2;
	uint8_t y = (MENU_LINES - msg->height) / 2;

	if (!msg->addr) {
		msg->addr = VDP_PLANEA_ADDR + (y * VDP_PLANE_HTILES + x) * 2;
	}

	// Copy dialog area to the unused screen zone
	rect_copy(msg->addr, msg->addr + 2 * MENU_LINE_CHARS, msg->width,
			msg->height);

	// Draw borders
	frame_draw(msg->addr, msg->width, msg->height);

	// Draw title
	menu_str_pos_draw(&entry->title, x + 2, y, msg->width - 4,
			MENU_COLOR_ITEM_ALT, MENU_PLACE_CENTER);

	// Draw caption
	menu_str_pos_draw(&entry->msg_entry->caption, x_caption, y + 2,
			msg->width - 4, MENU_COLOR_ITEM_ALT, MENU_PLACE_CENTER);

	// Draw buttons


}

static enum menu_msg_action button_action(void)
{
	// TODO
	return MENU_MSG_ACTION_YES_OK;
}

static void button_move(int8_t value)
{
	// TODO
	UNUSED_PARAM(value);
}

enum menu_msg_action menu_msg_update(uint8_t gp_press)
{
	UNUSED_PARAM(gp_press);
	struct menu_entry_instance *instance = menu->instance;
	struct menu_msg_entry *entry = instance->entry->msg_entry;
	enum menu_msg_action action = MENU_MSG_ACTION_NONE;

	// Check timeout
	if (entry->tout_frames) {
		entry->tout_frames--;
		if (!entry->tout_frames) {
			return MENU_MSG_ACTION_TIMEOUT;
		}
	}

	if (gp_press & GP_A_MASK) {
		if (!(entry->flags & MENU_MSG_MODAL)) {
			action = button_action();
		}
	} else if (gp_press & GP_B_MASK) {
		if (!(entry->flags & MENU_MSG_MODAL)) {
			action = MENU_MSG_ACTION_CLOSE;
		}
	} else if (gp_press & GP_C_MASK) {
		if (instance->entry->c_button_cb) {
			instance->entry->c_button_cb(instance);
		}
	} else if (gp_press & GP_RIGHT_MASK) {
		button_move(1);
	} else if (gp_press & GP_LEFT_MASK) {
		button_move(-1);
	}
	return action;
}

void menu_msg_close(void)
{
	menu_back(1);
}

