#ifndef _SOUND_H_
#define _SOUND_H_

#include <stdint.h>

/// Sound effects for menus
enum menu_sfx {
	SFX_MENU_ENTER    = 3,	///< Enter a new menu
	SFX_MENU_BACK     = 2,	///< Go back a menu level
	SFX_MENU_PAGE     = 0,	///< Change item page
	SFX_MENU_ITEM     = 4,	///< Change item (up/down)
	SFX_MENU_KEY_MOVE = 5,	///< Change key (OSK)
	SFX_MENU_KEY_TYPE = 3,	///< Add character (OSK)
	SFX_MENU_KEY_DEL  = 6,	///< Delete a character (OSK)
	SFX_MENU_TOGGLE   = 6	///< Menu toggle
};

/// Module initialization
uint16_t sound_init(const uint8_t *tfc_data, const uint8_t *psg_data);
/// Module deinitialization
void sound_deinit(void);

// PSG player functions
uint16_t psgfx_init(const uint8_t *data);
void psgfx_deinit(void);
void psgfx_play(uint16_t num);

// TFC player functions
void tfc_init(const uint8_t *data);
void tfc_play(uint16_t play);

extern const uint8_t menu_01_00_data[];
extern const uint8_t ojete_data[];
extern const uint8_t sfx_data[];
extern const uint8_t uwpsgfx_data[];

#endif /*_SOUND_H_*/

