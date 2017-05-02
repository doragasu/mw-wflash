/************************************************************************//**
 * \brief Menu implementation. This menu allows to define menu data
 * structures, and alow allows to display them and handle options.
 *
 * \author	Jesús Alonso (doragasu)
 * \date	2017
 ****************************************************************************/
#include "menu.h"
#include "gamepad.h"
#include <string.h>

/// Scroll direction to the left
#define MENU_SCROLL_DIR_LEFT	0
/// Scroll direction to the right
#define MENU_SCROLL_DIR_RIGHT	1

/// Number of characters horizontally separating each menu
#define MENU_SEPARATION_CHR		(VDP_SCREEN_WIDTH_PX/8)
/// Number of pixels separating each menu
#define MENU_SEPARATION_PX		VDP_SCREEN_WIDTH_PX

/// Number of steps to scroll a complete menu screen
const uint8_t scrDelta[] = {
	1, 1, 2, 2, 3, 5, 6, 7, 9, 11, 13, 16, 18, 21, 25, 28, 32, 36, 40, 44
};

/// Number of rows of the QWERTY menu
#define MENU_QWERTY_ROWS		4
/// Number of columns of the QWERTY menu
#define MENU_QWERTY_COLS		11

/// Alphanumeric menu definition
const char qwerty[2 * MENU_QWERTY_ROWS][MENU_QWERTY_COLS] = {{
		'1','2','3','4','5','6','7','8','9','0','-'
	},{
		'Q','W','E','R','T','Y','U','I','O','P','['
	},{
		'A','S','D','F','G','H','J','K','L',';','\'',
	},{
		'Z','X','C','V','B','N','M',',','.','/','+'
	},{
		'!','@','#','$','%','^','&','*','(',')','_'
	},{
		'q','w','e','r','t','y','u','i','o','p',']'
	},{
		'a','s','d','f','g','h','j','k','l',':','"'
	},{
		'z','x','c','v','b','n','m','<','>','?','='
	}
};

/// Obtains the number of scroll steps for the menu scroll function
#define MENU_SCROLL_NSTEPS	(sizeof(scrDelta))

#define MenuGetCurrentItemNum()		(md.selItem[md.level] + \
		md.selPage[md.level] * md.me[md.level]->item.entPerPage)

/// Returns the number of items on current page
#define MenuNumPageItems()	\
		(md.selPage[md.level] == md.me[md.level]->item.pages? \
		md.me[md.level]->item.nItems - (md.me[md.level]->item.entPerPage * \
		md.me[md.level]->item.pages):md.me[md.level]->item.entPerPage)

/// Dynamic data structure needed to display the menus
typedef struct {
	/// Menu entry for each menu level
	const MenuEntry *me[MENU_NLEVELS];
	/// Reserve space for the rContext string
	char rConStr[MENU_LINE_CHARS_TOTAL];
	MenuString rContext;		///< Right context string (bottom line)
	uint8_t level;				///< Current menu level
	/// Selected item, for each menu level. On keyboard menus, it holds
	/// the cursor position.
	uint8_t selItem[MENU_NLEVELS];
	/// Selected menu item page (minus 1). On keyb
	uint8_t selPage[MENU_NLEVELS];
} Menu;

/// Dynamic data needed to display the menus
static Menu md;

/************************************************************************//**
 * Compute line character position needed for requested horizontal alignment.
 *
 * \param[in] mStr   MenuString type text to align.
 * \param[in] align  Requested alignment.
 * \param[in] margin Margin to apply to left and right alignments (ignored
 *                   for center aligned strings).
 *
 * \return The horizontal position of the start of the string, for it to be
 * horizontal aligned as requested.
 ****************************************************************************/
uint8_t MenuStrAlign(MenuString mStr, MenuHAlign align, uint8_t margin) {
	switch (align) {
		case MENU_H_ALIGN_CENTER:
			return mStr.length + margin >= MENU_LINE_CHARS_TOTAL?margin:
				(MENU_LINE_CHARS_TOTAL - mStr.length)>>1;

		case MENU_H_ALIGN_RIGHT:
			return mStr.length + margin >= MENU_LINE_CHARS_TOTAL?margin:
				MENU_LINE_CHARS_TOTAL - margin - mStr.length;

		case MENU_H_ALIGN_LEFT:
		default:
			return margin;
	}
}


/************************************************************************//**
 * Clears requested lines of the screen. Each line is 40 characters wide.
 *
 * \param[in] first  First line to clear.
 * \param[in] last   Last line to clear.
 * \param[in] offset Horizontal plane offset of the screen.
 ****************************************************************************/
void MenuClearLines(uint8_t first, uint8_t last, uint8_t offset) {
	int8_t line;
	uint16_t addr;

	// Clear line by line from specified offset
	for (line = first, addr = VDP_PLANEA_ADDR + offset * 2 + first *
		(VDP_PLANE_HTILES<<1); line <= last; line++,
		addr += VDP_PLANE_HTILES<<1) {
		VdpDmaVRamFill(addr, MENU_SEPARATION_CHR * 2, 0);
		// Wait for DMA completion and set increment to 2
		VdpDmaWait();
	}
}

/************************************************************************//**
 * Sets the status text to display in the right context string space.
 *
 * \param[in] statStr MenuString with the status text to display in the right
 *                     context string space.
 ****************************************************************************/
void MenuStatStrSet(MenuString statStr) {
	// Current menu entry
	const MenuEntry *m = md.me[md.level];

	memcpy(md.rContext.string, statStr.string, statStr.length + 1);
	md.rContext.length = statStr.length;
	// Redraw context string
	MenuClearLines(MENU_LINE_CONTEXT, MENU_LINE_CONTEXT, 0);
	VdpDrawText(VDP_PLANEA_ADDR, m->margin, MENU_LINE_CONTEXT,
			MENU_COLOR_CONTEXT_L, statStr.length, m->lContext.string);
	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(md.rContext, MENU_H_ALIGN_RIGHT,
			m->margin), MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_R,
			statStr.length, md.rContext.string);
}

/************************************************************************//**
 * Draw requested item page.
 *
 * \param[in] chrOff Character offset in plane to draw menu
 ****************************************************************************/
void MenuDrawItemPage(uint8_t chrOff) {
	// Current menu
	const MenuEntry *m = md.me[md.level];
	// Loop control
	uint8_t i;
	// Line to draw text in
	uint8_t line;
	// Number of items to draw in this page
	uint8_t pageItems;
	// Number of the item to draw
	uint8_t item;

	// Clear previously drawn items
	MenuClearLines(MENU_LINE_ITEM_FIRST, MENU_LINE_ITEM_LAST, chrOff);
	// Get the number of items to draw on current page
	pageItems = MenuNumPageItems();
	// Keep selected item unless it points to a non existing option
	if (md.selItem[md.level] >= pageItems) md.selItem[md.level] = pageItems - 1;
	// Draw menu items in page
	for (i = 0, line = MENU_LINE_ITEM_FIRST, item = md.selPage[md.level]
			* m->item.entPerPage; i < pageItems; i++, line += m->item.spacing, item++) {
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MenuStrAlign(
			m->item.item[item].caption, m->item.align, m->margin), line,
			i == md.selItem[md.level]?MENU_COLOR_ITEM_SEL:MENU_COLOR_ITEM,
			m->item.item[item].caption.length,
			m->item.item[item].caption.string);
	}
	// Draw page number and total, if number of pages greater than 1
	if (m->item.pages > 0) {
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			MENU_DEF_RIGHT_MARGIN - 1, MENU_LINE_PAGER, MENU_COLOR_PAGER,
			m->item.pages + 1);
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			MENU_DEF_RIGHT_MARGIN - 2, MENU_LINE_PAGER, MENU_COLOR_PAGER, 1,
			"/");
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			MENU_DEF_RIGHT_MARGIN - 3, MENU_LINE_PAGER, MENU_COLOR_PAGER,
			md.selPage[md.level] + 1);
	}
}

/************************************************************************//**
 * Scroll the menu to the left/right for the new menu to be displayed.
 *
 * \param[in] direction Either MENU_SCROLL_DIR_LEFT (to scroll the menu to
 *            the left) or MENU_SCROLL_DIR_RIGHT (to scroll the menu to the
 *            right).
 ****************************************************************************/
void MenuXScroll(uint8_t direction) {
	int8_t i;
	uint16_t addr, xScroll, offset;
	// Scrolling loop
	for (i = MENU_SCROLL_NSTEPS - 1, xScroll = 0; i >= 0; i--) {
		// Wait for VBlank
		VdpVBlankWait();
		// Compute and write new scroll value, taking into account direction
		xScroll += direction == MENU_SCROLL_DIR_LEFT?-scrDelta[i]:scrDelta[i];
		VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR, xScroll);
	}
	// Copy menu to the zone that has been hidden
	offset = direction == MENU_SCROLL_DIR_LEFT?MENU_SEPARATION_CHR * 2:
		(VDP_PLANE_HTILES - MENU_SEPARATION_CHR) * 2;

	for (i = MENU_NLINES_TOTAL - 1, addr = VDP_PLANEA_ADDR; i >= 0; i--,
			addr += VDP_PLANE_HTILES * 2) {
		VdpDmaVRamCopy(addr + offset, addr, MENU_SEPARATION_CHR * 2);
		VdpDmaWait();
	}
	// Return scroll to original position
	VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR, 0);
}

/************************************************************************//**
 * Draws the currently active On Screen Keyboard.
 *
 * \param[in] offset Character offset in which to draw the menu
 ****************************************************************************/
void DrawOsk(uint16_t offset) {
	// Current menu entry
	const MenuEntry *m = md.me[md.level];

	// Draw the field name to edit
	VdpDrawText(VDP_PLANEA_ADDR, offset + m->margin, MENU_LINE_OSK_FIELD,
			MENU_COLOR_OSK_FIELD, m->keyb.fieldName.length,
			m->keyb.fieldName.string);

	// Draw the field data to edit, centered with respect to the maximum
	// size of the data string.
	VdpDrawText(VDP_PLANEA_ADDR, offset + ((MENU_LINE_CHARS_TOTAL -
			m->keyb.maxLen)>>1), MENU_LINE_OSK_DATA, MENU_COLOR_OSK_DATA,
			m->keyb.fieldData.length, m->keyb.fieldData.string);
}

/************************************************************************//**
 * Draws the menu set on the current menu level. The menu is drawn out fo the
 * visible screen, and the plane is scrolled on the specified direction for
 * the new menu to appear.
 *
 * \param[in] direction Either MENU_SCROLL_DIR_LEFT (to scroll the menu to
 *            the left) or MENU_SCROLL_DIR_RIGHT (to scroll the menu to the
 *            right).
 *
 * \note The function assumes the screen half in which it will draw the
 * menu, is already clear. It also clears the screen half that falls
 * outside of the screen, when it switches menus.
 ****************************************************************************/
void MenuDraw(uint8_t direction) {
	// Current menu entry
	const MenuEntry *m = md.me[md.level];
	uint16_t offset;

	offset = direction == MENU_SCROLL_DIR_LEFT?MENU_SEPARATION_CHR:
		(VDP_PLANE_HTILES - MENU_SEPARATION_CHR);
	// Draw title string out of screen
	VdpDrawText(VDP_PLANEA_ADDR, offset + MenuStrAlign(m->title,
			MENU_H_ALIGN_CENTER, 0), MENU_LINE_TITLE, MENU_COLOR_TITLE,
			m->title.length, m->title.string);
	// Draw context strings
	VdpDrawText(VDP_PLANEA_ADDR, offset + MENU_DEF_LEFT_MARGIN,
			MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_L, m->lContext.length,
			m->lContext.string);
	VdpDrawText(VDP_PLANEA_ADDR, offset + MenuStrAlign(md.rContext,
			MENU_H_ALIGN_RIGHT, MENU_DEF_RIGHT_MARGIN),
			MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_R, md.rContext.length,
			md.rContext.string);
	// Depending on menu type, draw menu page contents
	switch (m->type) {
		case MENU_TYPE_ITEM:
			// Draw page (including selected item)
			MenuDrawItemPage(offset);
			break;

		case MENU_TYPE_OSK_QWERTY:
		case MENU_TYPE_OSK_NUMERIC:
		case MENU_TYPE_OSK_IPV4:
			DrawOsk(offset);

		default:
			break;
	}

	// X-scroll plane to show drawn menu
	MenuXScroll(direction);
	// Clear screen zone that has been hidden
	MenuClearLines(0, MENU_NLINES_TOTAL, offset);
}

/************************************************************************//**
 * Module initialization. Call this function before using any other one from
 * this module. This function initialzes the menu subsystem and displays the
 * root menu.
 *
 * \param[in] root     Pointer to the root menu entry.
 * \param[in] rContext MenuString with the text to display in the right
 *                     context string.
 ****************************************************************************/
void MenuInit(const MenuEntry *root, MenuString rContext) {
	// Zero module data
	memset((void*)&md, 0, sizeof(Menu));
	// Set string to point to string data
	md.rContext.string = md.rConStr;
	memcpy(md.rConStr, rContext.string, rContext.length + 1);
	md.rContext.length = rContext.length;
	// Set root and curren menu entries
	md.me[0] = root;
	// Set context string
	strncpy(md.rConStr, rContext.string, MENU_LINE_CHARS_TOTAL);
	md.rContext = rContext;
	VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR, 0);

	// Draw root menu
	MenuDraw(MENU_SCROLL_DIR_LEFT);
}

/// Advances page number to the next one (but does not draw the page)
static inline void MenuNextPage(void) {
	md.selPage[md.level]++;
	if (md.selPage[md.level] > md.me[md.level]->item.pages)
		md.selPage[md.level] = 0;
}

/// Sets page number to the previous one (but does not draw the page)
static inline void MenuPrevPage(void) {
	md.selPage[md.level] = md.selPage[md.level]?md.selPage[md.level] - 1:
		md.me[md.level]->item.pages;
}

/// Draws current item on the menu screen
static inline void MenuDrawCurrentItem(uint8_t txtColor) {
	uint8_t line, item;
	const MenuEntry *m = md.me[md.level];

	line = MENU_LINE_ITEM_FIRST + md.selItem[md.level] * m->item.spacing;
	item = md.selItem[md.level] + md.selPage[md.level] * m->item.entPerPage;
	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(m->item.item[item].caption,
			m->item.align, m->margin), line, txtColor,
			m->item.item[item].caption.length,
			m->item.item[item].caption.string);
}

/************************************************************************//**
 * Obtains the buttons pressed as input, and performs the corresponding
 * actions depending on the button press (item change, menu change, callback
 * execution, etc.).
 *
 * \param[in] input Menu actions, as obtained from a call to GpPressed().
 *
 * \todo Currently working only for MENU_TYPE_ITEM menus.
 ****************************************************************************/
void MenuButtonAction(uint8_t input) {
	uint8_t tmp;
	const MenuEntry *m = md.me[md.level];

	input = ~input;
	// Parse buttons before movement
	if (input & GP_A_MASK) {
		// Accept selected menu option
		tmp = MenuGetCurrentItemNum();
		if (m->item.item[tmp].next) {
			md.me[md.level + 1] = m->item.item[tmp].next;
			// Level up!
			md.level++;
			md.selItem[md.level] = 0;
			md.selPage[md.level] = 0;
			MenuDraw(MENU_SCROLL_DIR_LEFT);
		}
	} else if (input & GP_B_MASK) {
		// Go back one menu level
		if (md.level) {
			md.level--;
			MenuDraw(MENU_SCROLL_DIR_RIGHT);
		}
	} else if (input & GP_UP_MASK) {
		// Go up a menu item
		if (md.selItem[md.level]) {
			// Draw currently selected item with non-selected color
			MenuDrawCurrentItem(MENU_COLOR_ITEM);
			// Draw previous item with selected color
			md.selItem[md.level]--;
			MenuDrawCurrentItem(MENU_COLOR_ITEM_SEL);
		} else {
			// Go to previous page and select last item
			MenuPrevPage();
			md.selItem[md.level] = MenuNumPageItems() - 1;
			MenuDrawItemPage(0);
		}
	} else if (input & GP_DOWN_MASK) {
		// Go down a menu item
		tmp = MenuNumPageItems() - 1;
		if (md.selItem[md.level] < tmp) {
			// Draw currently selected item with non-selected color
			MenuDrawCurrentItem(MENU_COLOR_ITEM);
			// Draw next item with selected color
			md.selItem[md.level]++;
			MenuDrawCurrentItem(MENU_COLOR_ITEM_SEL);
		} else {
			// Advance to next page, and select first item
			MenuNextPage();
			md.selItem[md.level] = 0;
			MenuDrawItemPage(0);
		}
	} else if (input & GP_LEFT_MASK) {
		// Change to previous page
		MenuPrevPage();
		MenuDrawItemPage(0);
	} else if (input & GP_RIGHT_MASK) {
		// Change to next page
		MenuNextPage();
		MenuDrawItemPage(0);
	}
}

