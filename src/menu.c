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

#define MENU_SCROLL_NSTEPS	(sizeof(scrDelta))

#define MenuGetCurrentItemNum()		(md.selItem[md.level] + \
		md.selPage[md.level] * md.me[md.level]->entPerPage)

/// Returns the number of items on current page
#define MenuNumPageItems()	(md.selPage[md.level] == md.me[md.level]->pages? \
		md.me[md.level]->nItems - (md.me[md.level]->entPerPage * \
		md.me[md.level]->pages):md.me[md.level]->entPerPage)


typedef struct {
	/// Menu entry for each menu level
	const MenuEntry *me[MENU_NLEVELS];
	/// Reserve space for the rContext string
	char rConStr[MENU_LINE_CHARS_TOTAL];
	MenuString rContext;		///< Right context string (bottom line)
	uint8_t level;				///< Current menu level
	/// Selected item, for each menu level
	uint8_t selItem[MENU_NLEVELS];
	/// Selected menu item page (minus 1)
	uint8_t selPage[MENU_NLEVELS];
} Menu;

Menu md;

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

/// \param[in] chrOff Character offset in plane to draw menu
void MenuDrawPage(uint8_t chrOff) {
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
			* m->entPerPage; i < pageItems; i++, line += m->spacing, item++) {
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MenuStrAlign(
			m->item[item].caption, m->align, m->margin), line,
			i == md.selItem[md.level]?MENU_COLOR_ITEM_SEL:MENU_COLOR_ITEM,
			m->item[item].caption.length, m->item[item].caption.string);
	}
	// Draw page number and total, if number of pages greater than 1
	if (m->pages > 0) {
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			m->margin - 1, MENU_LINE_PAGER, MENU_COLOR_PAGER, m->pages + 1);
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			m->margin - 2, MENU_LINE_PAGER, MENU_COLOR_PAGER, 1, "/");
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			m->margin - 3, MENU_LINE_PAGER, MENU_COLOR_PAGER,
			md.selPage[md.level] + 1);
	}
}

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

/// \note The function assumes the screen half in which it will draw the
/// menu, is already clear. It also clears the screen half that falls
/// outside of the screen, when it switches menu.
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
	VdpDrawText(VDP_PLANEA_ADDR, offset + m->margin,
			MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_L, m->lContext.length,
			m->lContext.string);
	VdpDrawText(VDP_PLANEA_ADDR, offset +
			MenuStrAlign(md.rContext, MENU_H_ALIGN_RIGHT, m->margin),
			MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_R, md.rContext.length,
			md.rContext.string);
	// Draw page (including selected item)
	MenuDrawPage(offset);
	// X-scroll plane to show drawn menu
	MenuXScroll(direction);
	// Clear screen zone that has been hidden
	MenuClearLines(0, MENU_NLINES_TOTAL, offset);
}

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
	if (md.selPage[md.level] > md.me[md.level]->pages)
		md.selPage[md.level] = 0;
}

/// Sets page number to the previous one (but does not draw the page)
static inline void MenuPrevPage(void) {
	md.selPage[md.level] = md.selPage[md.level]?md.selPage[md.level] - 1:
		md.me[md.level]->pages;
}

static inline void MenuDrawCurrentItem(uint8_t txtColor) {
	uint8_t line, item;
	const MenuEntry *m = md.me[md.level];

	line = MENU_LINE_ITEM_FIRST + md.selItem[md.level] * m->spacing;
	item = md.selItem[md.level] + md.selPage[md.level] * m->entPerPage;
	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(m->item[item].caption, m->align,
			m->margin), line, txtColor, m->item[item].caption.length,
			m->item[item].caption.string);
}

void MenuButtonAction(uint8_t input) {
	uint8_t tmp;
	const MenuEntry *m = md.me[md.level];

	input = ~input;
	// Parse buttons before movement
	if (input & GP_A_MASK) {
		// Accept selected menu option
		VdpDrawText(VDP_PLANEA_ADDR, 4, 1,MENU_COLOR_CONTEXT_R, 1, "A");
		tmp = MenuGetCurrentItemNum();
		if (m->item[tmp].next) {
			md.me[md.level + 1] = m->item[tmp].next;
			// Level up!
			md.level++;
			md.selItem[md.level] = 0;
			md.selPage[md.level] = 0;
			MenuDraw(MENU_SCROLL_DIR_LEFT);
		}
	} else if (input & GP_C_MASK) {
		// Go back one menu level
		VdpDrawText(VDP_PLANEA_ADDR, 4, 1,MENU_COLOR_CONTEXT_R, 1, "C");
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
			MenuDrawPage(0);
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
			MenuDrawPage(0);
		}
	} else if (input & GP_LEFT_MASK) {
		// Change to previous page
		MenuPrevPage();
		MenuDrawPage(0);
	} else if (input & GP_RIGHT_MASK) {
		// Change to next page
		MenuNextPage();
		MenuDrawPage(0);
	}
}

