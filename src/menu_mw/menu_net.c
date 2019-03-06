#include <string.h>
#include "menu_net.h"
#include "menu_txt.h"
#include "../menu_imp/menu.h"
#include "../menu_imp/menu_msg.h"
#include "../mw/megawifi.h"
#include "../mpool.h"
#include "../util.h"
#include "../loop.h"

#define MENU_NET_DATA_OFF	10

enum {
	MENU_NET_SSID = 0,
	MENU_NET_PASS,
	MENU_NET_SCAN,
	MENU_NET_IP_CONFIG,
	MENU_NET_IP,
	MENU_NET_MASK,
	MENU_NET_GW,
	MENU_NET_DNS1,
	MENU_NET_DNS2,
	MENU_NET_SAVE
};

struct test_data {
	struct loop_timer tim;
	struct menu_entry_instance *instance;
};

static const char * const security[] = {
	"OPEN  ", "WEP   ", "WPA   ", "WPA2  ", "WPA1/2", "UNSUP."
};

static struct test_data test = {};

static const char *net_security_str(enum mw_security sec)
{
	const char *sec_str;

	if (sec >= MW_SEC_UNKNOWN) {
		sec_str = security[MW_SEC_UNKNOWN];
	} else {
		sec_str = security[sec];
	}

	return sec_str;
}

static int net_menu_reset(struct menu_entry_instance *instance)
{
	UNUSED_PARAM(instance);

	menu_reset();

	return 0;
}

static void net_menu_test_cb(struct loop_timer *t)
{
	UNUSED_PARAM(t);

	/// \todo spinner by reusing the timer
	int slot = test.instance->prev->prev->prev->sel_item;
	struct menu_item *item = test.instance->entry->item_entry->item;
	enum mw_err err;

	err = mw_ap_assoc(slot);
	if (MW_ERR_NONE != err) {
		menu_str_append(&item[0].caption, "ERROR!");
		goto end;
	}
	mw_sleep(60 * 5);
	menu_str_append(&item[0].caption, "DONE!");
	item[1].hidden = FALSE;
	menu_item_draw(MENU_PLACE_CENTER);

	err = mw_tcp_connect(1, "www.duck.com", "443", NULL);
	if (MW_ERR_NONE != err) {
		menu_str_append(&item[1].caption, "ERROR!");
		goto end;
	}
	menu_str_append(&item[1].caption, "DONE!");
	mw_tcp_disconnect(1);

end:
	mw_ap_disassoc();
	item[2].hidden = FALSE;
	item[2].entry_cb = net_menu_reset;
	menu_item_draw(MENU_PLACE_CENTER);
	loop_timer_del(&test.tim);
}

static int net_menu_test_run(struct menu_entry_instance *instance)
{
	test.tim.timer_cb = net_menu_test_cb;
	test.instance = instance;
	loop_timer_add(&test.tim);
	loop_timer_start(&test.tim, 1);

	return 0;
}

const struct menu_entry net_menu_test = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("WIFI CONFIGURATION TEST"),
	.enter_cb = net_menu_test_run,
	.left_context = MENU_STR_EMPTY(2),
	.item_entry = MENU_ITEM_ENTRY(3, 4, MENU_H_ALIGN_CENTER) {
		{
			.caption = MENU_STR_RW("Associating to AP... ", 27),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_RW("Connecting to duck.com... ", 32),
			.not_selectable = TRUE,
			.alt_color = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("TEST FINISHED"),
			.hidden = TRUE,
			.entry_cb = net_menu_reset
		},
	} MENU_ITEM_ENTRY_END
};

const struct menu_entry net_menu_done = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("CONFIGURATION COMPLETE!"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.item_entry = MENU_ITEM_ENTRY(2, 4, MENU_H_ALIGN_CENTER) {
		{
			.caption = MENU_STR_RO("TEST"),
			.next = (struct menu_entry*)&net_menu_test
		},
		{
			.caption = MENU_STR_RO("DONE!"),
			.entry_cb = net_menu_reset
		}
	} MENU_ITEM_ENTRY_END
};

// Copy selected SSID to previous menu level
static int net_menu_scan_sel(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	char *ssid = item[instance->sel_item].caption.str;
	struct menu_item *prev = instance->prev->entry->item_entry->item;
	struct menu_item *dst = &prev[MENU_NET_SSID];
	// Skip "SSID: ", overwrite anything else
	dst->caption.length = dst->offset;
	menu_str_append(&dst->caption, ssid + 12);

	// Selection complete, go back a level
	menu_back(1);

	return 0;
}

static void net_menu_ap_fill(int n_item, struct menu_item_entry *entry,
		const struct mw_ap_data *ap)
{
	struct menu_str *line = &entry->item[n_item].caption;
	char signal[5];
	const char *security;
	int line_length;
	int rssi_length;

	// Line length: 4 (RSSI) + 6 (security) + sssid_len + 3 (blanks, null)
	line_length = 4 + 6 + ap->ssid_len + 2;
	line->str = mp_alloc(line_length + 1);
	if (line->str) {
		line->max_length = line_length;
		security = net_security_str(ap->auth);
		rssi_length = int8_to_str(ap->rssi, signal);
		for (int i = rssi_length; i < 4; i++) {
			menu_str_append(line, " ");
		}
		menu_str_append(line, signal);
		menu_str_append(line, " ");
		menu_str_append(line, security);
		menu_str_append(line, " ");
		menu_str_append(line, ap->ssid);
		line->str[line->length] = '\0';
		entry->item[n_item].entry_cb = net_menu_scan_sel;
	}
}

static int net_menu_ap_scan(struct menu_entry_instance *instance)
{
	struct menu_item_entry *entry = instance->entry->item_entry;
	uint8_t n_aps = 0;
	char *data = NULL;
	int data_len;
	int pos = 0;
	struct mw_ap_data ap = {};
	int i;

	// Party time!
	menu_msg("NETWORK SCAN", "Scan in progress, please wait...",
			MENU_MSG_MODAL, 0);
	data_len = mw_ap_scan(&data, &n_aps);
	menu_msg_close();
	if (data_len < 0) {
		menu_msg("ERROR", "Scan failed!", 0, 60 * 5);
		return 1;
	}

	// Allocate memory for the menu items
	entry->item = mp_calloc(n_aps * sizeof(struct menu_item));
	if (!entry->item) {
		menu_msg("ERROR", "Out of memory!", 0, 60 * 5);
		return 1;
	}
	
	// Fill items
	entry->n_items = n_aps;
	entry->pages = MENU_PAGES(n_aps, 1);
	for (i = 0; i < n_aps && (pos = mw_ap_fill_next(data, pos, &ap,
					data_len)); i++) {
		net_menu_ap_fill(i, entry, &ap);
	}
	return 0;
}

/// This struct is mostly empty because it is populated in real time with
/// scan results.
static const struct menu_entry net_menu_scan = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("SELECT A WIFI ACCESS POINT"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = net_menu_ap_scan,
	.item_entry = (struct menu_item_entry*)&(const struct menu_item_entry) {
		.spacing = 1,
		.items_per_page = MENU_ITEM_LINES,
		.align = MENU_H_ALIGN_LEFT
	}
};

static int net_menu_is_dhcp(struct menu_item *item)
{
	int dhcp;

	dhcp = 'D' == item[MENU_NET_IP_CONFIG].caption.str
		[MENU_NET_DATA_OFF + 1];

	return dhcp;
}

static int net_menu_save(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	struct menu_item *item_ssid = &item[MENU_NET_SSID];
	struct menu_item *item_pass = &item[MENU_NET_PASS];
	char *ssid = item_ssid->caption.str + item_ssid->offset;
	char *pass = item_pass->caption.str + item_pass->offset;
	uint8_t slot = instance->prev->sel_item;
	struct mw_ip_cfg ip = {};

	if (!net_menu_is_dhcp(item)) {
		ip.addr.addr = ip_str_to_uint32(item[MENU_NET_IP].
				caption.str + MENU_NET_DATA_OFF);
		ip.mask.addr = ip_str_to_uint32(item[MENU_NET_MASK].
				caption.str + MENU_NET_DATA_OFF);
		ip.gateway.addr = ip_str_to_uint32(item[MENU_NET_GW].
				caption.str + MENU_NET_DATA_OFF);
		ip.dns1.addr = ip_str_to_uint32(item[MENU_NET_DNS1].
				caption.str + MENU_NET_DATA_OFF);
		ip.dns2.addr = ip_str_to_uint32(item[MENU_NET_DNS2].
				caption.str + MENU_NET_DATA_OFF);
	}

	if (mw_ap_cfg_set(slot, ssid, pass) || mw_ip_cfg_set(slot, &ip)) {
		menu_msg("ERROR", "Failed to save configuration!", 0, 60 * 5);
		return 1;
	}

	return 0;
}

static int net_menu_ip_validate(struct menu_entry_instance *instance)
{
	struct menu_str *tmp = &instance->entry->osk_entry->tmp;

	if (!ip_validate(tmp->str)) {
		menu_msg("INVALID IP", "Please enter a valid IPv4 address",
				0, 5 * 60);
		return 1;
	}

	return 0;
}

static const struct menu_entry menu_net_ipv4_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.left_context = MENU_STR_RO(NUMERIC_LEFT_CTX_STR),
	.action_cb = net_menu_ip_validate,
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Enter IP address:"),
		.osk_type = MENU_TYPE_OSK_IPV4,
		.line_len = 15
	}
};

static void net_menu_ip_config(struct menu_item *item, int dhcp)
{
	const char *cfg = dhcp?"DHCP":"MANUAL";
	struct menu_str *caption = &item[MENU_NET_IP_CONFIG].caption;
	int offset = MENU_NET_DATA_OFF + 1;

	caption->length = offset + menu_str_buf_cpy(caption->str + offset,
			cfg, caption->max_length - offset);

	for (int i = MENU_NET_IP; i <= MENU_NET_DNS2; i++) {
		item[i].hidden = dhcp;
		item[i].not_selectable = dhcp;
	}
}

static int net_menu_ip_toggle(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	int dhcp = net_menu_is_dhcp(item);

	net_menu_ip_config(item, !dhcp);

	menu_item_draw(MENU_PLACE_CENTER);

	// No menu transition here
	return TRUE;
}

static void net_menu_draw_default(struct menu_item *item)
{
	net_menu_ip_config(item, TRUE);
	menu_str_append(&item[MENU_NET_IP].caption, DEF_IP_STR);
	menu_str_append(&item[MENU_NET_MASK].caption, DEF_MASK_STR);
	menu_str_append(&item[MENU_NET_GW].caption, DEF_GW_STR);
	menu_str_append(&item[MENU_NET_DNS1].caption, DEF_DNS1_STR);
	menu_str_append(&item[MENU_NET_DNS2].caption, DEF_DNS2_STR);
}

static void net_menu_draw_cfg(struct menu_item *item,
		const struct mw_ip_cfg *ip)
{
	struct menu_str *str;

	net_menu_ip_config(item, FALSE);
	str = &item[MENU_NET_IP].caption;
	str->length += uint32_to_ip_str(ip->addr.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_NET_MASK].caption;
	str->length += uint32_to_ip_str(ip->mask.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_NET_GW].caption;
	str->length += uint32_to_ip_str(ip->gateway.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_NET_DNS1].caption;
	str->length += uint32_to_ip_str(ip->dns1.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_NET_DNS2].caption;
	str->length += uint32_to_ip_str(ip->dns2.addr, str->str +
			MENU_NET_DATA_OFF);
}

/// Fill slot names with SSIDs
static int net_menu_enter_cb(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	uint8_t slot = instance->prev->sel_item;
	char *ssid = NULL;
	char *pass = NULL;
	struct mw_ip_cfg *ip_cfg = NULL;
	enum mw_err err;

	err = mw_ap_cfg_get(slot, &ssid, &pass);
	if (!err && ssid) {
		menu_str_append(&item[MENU_NET_SSID].caption, ssid);
	}
	if (!err && pass) {
		menu_str_append(&item[MENU_NET_PASS].caption, pass);
	}

	err = mw_ip_cfg_get(slot, &ip_cfg);
	if (err || !ip_cfg || !ip_cfg->addr.addr) {
		// No config or DHCP
		net_menu_draw_default(item);;
	} else {
		// Manual config
		net_menu_draw_cfg(item, ip_cfg);
	}

	return 0;
}

static const struct menu_entry menu_net_ssid_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("SSID"),
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Enter network SSID:"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = MW_SSID_MAXLEN
	}
};

static const struct menu_entry menu_net_pass_osk = {
	.type = MENU_TYPE_OSK,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("PASSWORD"),
	.left_context = MENU_STR_RO(QWERTY_LEFT_CTX_STR),
	.osk_entry = MENU_OSK_ENTRY {
		.caption = MENU_STR_RO("Enter network passsword:"),
		.osk_type = MENU_TYPE_OSK_QWERTY,
		.line_len = MW_PASS_MAXLEN
	}
};

const struct menu_entry net_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = 8,
	.title = MENU_STR_RO("NETWORK CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = net_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(10, 2, MENU_H_ALIGN_LEFT) {
		{
			.caption = MENU_STR_RW("SSID: ", 38),
			.offset = 6,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&menu_net_ssid_osk
		},
		{
			.caption = MENU_STR_RW("PASS: ", 70),
			.secure = TRUE,
			.offset = 6,
			.draw_empty = TRUE,
			.next = (struct menu_entry*)&menu_net_pass_osk
		},
		{
			.caption = MENU_STR_RO("SCAN..."),
			.next = (struct menu_entry*)&net_menu_scan,
		},
		{
			.caption = MENU_STR_RW("IP CONFIG: ", 17),
			.entry_cb = net_menu_ip_toggle
		},
		{
			.caption = MENU_STR_RW("ADDRESS:  ", 26),
			.offset = MENU_NET_DATA_OFF,
			.next = (struct menu_entry*)&menu_net_ipv4_osk
		},
		{
			.caption = MENU_STR_RW("MASK:     ", 26),
			.offset = MENU_NET_DATA_OFF,
			.next = (struct menu_entry*)&menu_net_ipv4_osk
		},
		{
			.caption = MENU_STR_RW("GATEWAY:  ", 26),
			.offset = MENU_NET_DATA_OFF,
			.next = (struct menu_entry*)&menu_net_ipv4_osk
		},
		{
			.caption = MENU_STR_RW("DNS1:     ", 26),
			.offset = MENU_NET_DATA_OFF,
			.next = (struct menu_entry*)&menu_net_ipv4_osk
		},
		{
			.caption = MENU_STR_RW("DNS2:     ", 26),
			.offset = MENU_NET_DATA_OFF,
			.next = (struct menu_entry*)&menu_net_ipv4_osk
		},
		{
			.caption = MENU_STR_RO("SAVE!"),
			.entry_cb = net_menu_save,
			.next = (struct menu_entry*)&net_menu_done
		},
	} MENU_ITEM_ENTRY_END
};

