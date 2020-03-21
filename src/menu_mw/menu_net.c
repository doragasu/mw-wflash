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

#define MENU_NET_PHY_DEFAULT	MW_PHY_11BGN

enum {
	MENU_NET_SCAN = 0,
	MENU_NET_SSID,
	MENU_NET_PASS,
	MENU_NET_ADVANCED,
	MENU_NET_SAVE
};

enum {
	MENU_ADV_IP_CONFIG,
	MENU_ADV_IP,
	MENU_ADV_MASK,
	MENU_ADV_GW,
	MENU_ADV_DNS1,
	MENU_ADV_DNS2,
	MENU_ADV_MSG1,
	MENU_ADV_MSG2,
	MENU_ADV_PHY,
	MENU_ADV_OK
};

struct menu_net_adv_data {
	struct mw_ip_cfg ip;
	uint8_t phy;
};

struct menu_net_data {
	struct menu_net_adv_data cfg;
	struct menu_net_adv_data tmp;
};

static const char * const security[] = {
	"OPEN  ", "WEP   ", "WPA   ", "WPA2  ", "WPA1/2", "UNSUP."
};

static struct menu_net_data *d;

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

static int net_menu_done_cb(struct menu_entry_instance *instance)
{
	UNUSED_PARAM(instance);

	menu_back(MENU_BACK_ALL);

	return 0;
}

const struct menu_entry net_menu_done = {
	.type = MENU_TYPE_ITEM,
	.margin = MENU_DEF_LEFT_MARGIN,
	.title = MENU_STR_RO("CONFIGURATION COMPLETE!"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.item_entry = MENU_ITEM_ENTRY(1, 4, MENU_H_ALIGN_CENTER, MENU_BACK_ALL) {
		{
			.caption = MENU_STR_RO("DONE"),
			.entry_cb = net_menu_done_cb
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
	data_len = mw_ap_scan(d->tmp.phy, &data, &n_aps);
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
	entry->back_levels = 1;
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

	dhcp = 'D' == item[MENU_ADV_IP_CONFIG].caption.str
		[MENU_NET_DATA_OFF + 1];

	return dhcp;
}

static int net_menu_adv_ok(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;

	if (!net_menu_is_dhcp(item)) {
		d->tmp.ip.addr.addr = ip_str_to_uint32(item[MENU_ADV_IP].
				caption.str + MENU_NET_DATA_OFF);
		d->tmp.ip.mask.addr = ip_str_to_uint32(item[MENU_ADV_MASK].
				caption.str + MENU_NET_DATA_OFF);
		d->tmp.ip.gateway.addr = ip_str_to_uint32(item[MENU_ADV_GW].
				caption.str + MENU_NET_DATA_OFF);
		d->tmp.ip.dns1.addr = ip_str_to_uint32(item[MENU_ADV_DNS1].
				caption.str + MENU_NET_DATA_OFF);
		d->tmp.ip.dns2.addr = ip_str_to_uint32(item[MENU_ADV_DNS2].
				caption.str + MENU_NET_DATA_OFF);
	}
	d->cfg = d->tmp;

	menu_back(1);

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
	struct menu_str *caption = &item[MENU_ADV_IP_CONFIG].caption;
	int offset = MENU_NET_DATA_OFF + 1;

	caption->length = offset + menu_str_buf_cpy(caption->str + offset,
			cfg, caption->max_length - offset);

	for (int i = MENU_ADV_IP; i <= MENU_ADV_DNS2; i++) {
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

static void net_menu_adv_phy_fill(struct menu_item *item, uint8_t phy)
{
	struct menu_str *comp = &item[MENU_ADV_PHY].caption;
	comp->length = 21;

	if (phy & 1) {
		comp->str[comp->length++] = 'b';
	}
	if (phy & 2) {
		comp->str[comp->length++] = 'g';
	}
	if (phy & 4) {
		comp->str[comp->length++] = 'n';
	}
	comp->str[comp->length] = '\0';
}

static int net_menu_phy_toggle(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;

	// Hacky implementation
	d->tmp.phy >>= 1;
	if (!d->tmp.phy) {
		d->tmp.phy = MW_PHY_11BGN; // 0b111
	}

	net_menu_adv_phy_fill(item, d->tmp.phy);

	menu_item_draw(MENU_PLACE_CENTER);

	// No menu transition here
	return TRUE;
}

static void net_menu_draw_default(struct menu_item *item, uint8_t phy)
{
	net_menu_ip_config(item, TRUE);
	menu_str_append(&item[MENU_ADV_IP].caption, DEF_IP_STR);
	menu_str_append(&item[MENU_ADV_MASK].caption, DEF_MASK_STR);
	menu_str_append(&item[MENU_ADV_GW].caption, DEF_GW_STR);
	menu_str_append(&item[MENU_ADV_DNS1].caption, DEF_DNS1_STR);
	menu_str_append(&item[MENU_ADV_DNS2].caption, DEF_DNS2_STR);

	net_menu_adv_phy_fill(item, phy);
}

static void net_menu_draw_cfg(struct menu_item *item,
		const struct menu_net_adv_data *cfg)
{
	struct menu_str *str;

	net_menu_ip_config(item, FALSE);
	str = &item[MENU_ADV_IP].caption;
	str->length += uint32_to_ip_str(cfg->ip.addr.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_ADV_MASK].caption;
	str->length += uint32_to_ip_str(cfg->ip.mask.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_ADV_GW].caption;
	str->length += uint32_to_ip_str(cfg->ip.gateway.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_ADV_DNS1].caption;
	str->length += uint32_to_ip_str(cfg->ip.dns1.addr, str->str +
			MENU_NET_DATA_OFF);
	str = &item[MENU_ADV_DNS2].caption;
	str->length += uint32_to_ip_str(cfg->ip.dns2.addr, str->str +
			MENU_NET_DATA_OFF);

	net_menu_adv_phy_fill(item, cfg->phy);
}

static int net_menu_adv_enter_cb(struct menu_entry_instance *instance)
{
	// Copy config to temporal, and draw menu fields
	d->tmp = d->cfg;
	if (d->tmp.ip.addr.addr) {
		net_menu_draw_cfg(instance->entry->item_entry->item, &d->tmp);
	} else {
		net_menu_draw_default(instance->entry->item_entry->item,
				d->tmp.phy);
	}

	return 0;
}

const struct menu_entry net_menu_advanced = {
	.type = MENU_TYPE_ITEM,
	.margin = 8,
	.title = MENU_STR_RO("ADVANCED NETWORK CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = net_menu_adv_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(10, 2, MENU_H_ALIGN_LEFT, 1) {
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
			.caption = MENU_STR_RO("Change this only if you suffer"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_RO(	"connectivity problems:"),
			.not_selectable = TRUE,
			.alt_color = TRUE
		},
		{
			.caption = MENU_STR_RW("COMPATIBILITY: 802.11", 26),
			.entry_cb = net_menu_phy_toggle
		},
		{
			.caption = MENU_STR_RO("OK"),
			.entry_cb = net_menu_adv_ok
		},
	} MENU_ITEM_ENTRY_END
};

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

static int net_menu_save(struct menu_entry_instance *instance)
{
	struct menu_item *item = instance->entry->item_entry->item;
	struct menu_item *item_ssid = &item[MENU_NET_SSID];
	struct menu_item *item_pass = &item[MENU_NET_PASS];
	char *ssid = item_ssid->caption.str + item_ssid->offset;
	char *pass = item_pass->caption.str + item_pass->offset;
	uint8_t slot = instance->prev->sel_item;

	if (mw_ap_cfg_set(slot, ssid, pass, d->cfg.phy) ||
			mw_ip_cfg_set(slot, &d->cfg.ip)) {
		menu_msg("ERROR", "Failed to save configuration!", 0, 60 * 5);
		return 1;
	}

	mw_def_ap_cfg(slot);

	return 0;
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

	// Will be automatically freed on menu exit
	d = mp_alloc(sizeof(struct menu_net_data));

	// Get AP config
	d->cfg.phy = MENU_NET_PHY_DEFAULT;
	err = mw_ap_cfg_get(slot, &ssid, &pass, &d->cfg.phy);
	if (!err && ssid) {
		menu_str_append(&item[MENU_NET_SSID].caption, ssid);
	}
	if (!err && pass) {
		menu_str_append(&item[MENU_NET_PASS].caption, pass);
	}

	// Get IP config
	err = mw_ip_cfg_get(slot, &ip_cfg);
	if (err || !ip_cfg || !ip_cfg->addr.addr) {
		// No config or DHCP
		memset(&d->cfg.ip, 0, sizeof(struct mw_ip_cfg));
	} else {
		// Manual config
		d->cfg.ip = *ip_cfg;
	}

	return 0;
}

const struct menu_entry net_menu = {
	.type = MENU_TYPE_ITEM,
	.margin = 8,
	.title = MENU_STR_RO("NETWORK CONFIGURATION"),
	.left_context = MENU_STR_RO(ITEM_LEFT_CTX_STR),
	.enter_cb = net_menu_enter_cb,
	.item_entry = MENU_ITEM_ENTRY(10, 2, MENU_H_ALIGN_LEFT, 1) {
		{
			.caption = MENU_STR_RO("SCAN..."),
			.next = (struct menu_entry*)&net_menu_scan,
		},
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
			.caption = MENU_STR_RO("ADVANCED..."),
			.next = (struct menu_entry*)&net_menu_advanced
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.not_selectable = TRUE,
			.hidden = TRUE
		},
		{
			.caption = MENU_STR_RO("SAVE!"),
			.entry_cb = net_menu_save,
			.next = (struct menu_entry*)&net_menu_done
		}
	} MENU_ITEM_ENTRY_END
};

