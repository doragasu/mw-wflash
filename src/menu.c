/************************************************************************//**
 * \brief Menu implementation. This menu allows to define menu data
 * structures, and alow allows to display them and handle options.
 *
 * \author	Jesús Alonso (doragasu)
 * \date	2017
 ****************************************************************************/
#include "menu.h"
#include <string.h>

/// Initial value for quadratic scroll
#define MENU_SCROLL_DELTA_INIT	75
/// Number of steps to scroll a complete menu screen
#define MENU_SCROLL_NSTEPS		21
/// Scroll factor (fixed point value, using MENU_SCROLL_SHIFT)
#define MENU_SCROLL_FACTOR		7
/// Number of right shifts for the MENU_SCROLL_FACTOR fixed point value
#define MENU_SCROLL_SHIFT		3
/// Scroll direction to the left
#define MENU_SCROLL_DIR_LEFT	0
/// Scroll direction to the right
#define MENU_SCROLL_DIR_RIGHT	1

/// Number of characters horizontally separating each menu
#define MENU_SEPARATION_CHR		64
/// Number of pixels separating each menu
#define MENU_SEPARATION_PX		(MENU_SEPARATION_CHR*8)

typedef struct {
	MenuEntry *root;		///< Root menu entry
	MenuEntry *current;		///< Current menu entry
	uint16_t xScroll;		///< Scroll value, X axis
	int16_t  scrollDelta;	///< Amount to scroll on next step
	int8_t   scrollStep;	///< Scroll step
	/// Reserve space for the rContext string
	char rConStr[MENU_LINE_CHARS_TOTAL];
	MenuString rContext;	///< Right context string (bottom line)
	int8_t menuXPos;		///< X coord. of menu start (0 or 64)
	uint8_t level;			///< Current menu level
	/// Selected item, for each menu level
	uint8_t selItem[MENU_NLEVELS];
	/// Selected menu item page
	uint8_t selPage[MENU_NLEVELS];
} Menu;

Menu md;

void MenuInit(MenuEntry *root, MenuString rContext) {
	// Zero module data
	memset((void*)&md, 0, sizeof(Menu));
	// Set string to point to string data
	md.rContext.string = md.rConStr;
	// Set root and curren menu entries
	md.root = root;
	md.current = root;
	// Set context string
	strncpy(md.rConStr, rContext.string, MENU_LINE_CHARS_TOTAL);
	md.rContext = rContext;
	// Set scroll to second half, for the screen to be moved to the
	// first half when the first menu is drawn
	md.xScroll = MENU_SEPARATION_CHR * 8;
}

uint8_t MenuStrAlign(MenuString mStr, MenuHAlign align, uint8_t margin) {
	switch (align) {
		case MENU_H_ALIGN_CENTER:
			return mStr.length + margin >= MENU_LINE_CHARS_TOTAL?margin:
				((MENU_LINE_CHARS_TOTAL - mStr.length)>>1) - 1;

		case MENU_H_ALIGN_RIGHT:
			return mStr.length + margin >= MENU_LINE_CHARS_TOTAL?margin:
				MENU_LINE_CHARS_TOTAL - margin - mStr.length - 1;

		case MENU_H_ALIGN_LEFT:
		default:
			return margin;
	}
}

void MenuClearLines(uint8_t first, uint8_t last) {
	int8_t line;
	uint16_t addr;

	// Clear 64 characters out of the 128, line by line
	for (line = first, addr = VDP_PLANEA_ADDR + md.menuXPos + first *
		VDP_PLANE_HTILES; line <= last; line++, addr += VDP_PLANE_HTILES) {
		VdpVRamClear(addr, MENU_SEPARATION_CHR);
	}
}

void MenuDrawPage(void) {
	// Current menu
	MenuEntry *m = md.current;
	// Loop control
	uint8_t i;
	// Line to draw text in
	uint8_t line;
	// Number of items to draw in this page
	uint8_t pageItems;

	// Get the number of items to draw on current page
	pageItems = md.selPage[md.level] == m->pages - 1?m->nItems -
		(m->entPerPage * m->pages - 1):m->entPerPage;
	// Draw menu items in page
	for (i = 0, line = MENU_LINE_ITEM_FIRST; i < pageItems; i++,
			line += m->spacing) {
		VdpDrawText(VDP_PLANEA_ADDR, md.menuXPos + MenuStrAlign(
			m->item[i].caption, m->align, m->margin), line,
			i == md.selItem[md.level]?MENU_COLOR_ITEM_SEL:MENU_COLOR_ITEM,
			MENU_LINE_CHARS_TOTAL, m->item[i].caption.string);
	}
	// Draw page number and total
	VdpDrawDec(VDP_PLANEA_ADDR, md.menuXPos + MENU_LINE_CHARS_TOTAL - 
		m->margin - 1, MENU_LINE_PAGER, MENU_COLOR_PAGER, m->pages + 1);
	VdpDrawText(VDP_PLANEA_ADDR, md.menuXPos + MENU_LINE_CHARS_TOTAL - 
		m->margin - 2, MENU_LINE_PAGER, MENU_COLOR_PAGER, 1, "/");
	VdpDrawDec(VDP_PLANEA_ADDR, md.menuXPos + MENU_LINE_CHARS_TOTAL - 
		m->margin - 3, MENU_LINE_PAGER, MENU_COLOR_PAGER, m->pages + 1);
}

void MenuXScroll(uint8_t direction) {
	// Scrolling loop
	for (md.scrollStep = MENU_SCROLL_NSTEPS - 1,
		 md.scrollDelta = MENU_SCROLL_DELTA_INIT;
		 md.scrollStep >= 0; md.scrollStep--) {
		// Wait for VBlank
		VdpVBlankWait();
		// Compute and write new scroll value, taking into account direction
		md.xScroll += direction == MENU_SCROLL_DIR_LEFT?
			md.scrollDelta:-md.scrollDelta;
		VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR, md.xScroll);
		// Prepare next delta value
		md.scrollDelta = (md.scrollDelta * MENU_SCROLL_FACTOR)>>
			MENU_SCROLL_SHIFT;
	}
}

/// \note The function assumes the screen half in which it will draw the
/// menu, is already clear. It also clears the screen half that falls
/// outside of the screen, when it switches menu.
void MenuDraw(uint8_t direction) {
	// Current menu entry
	MenuEntry *m = md.current;

	// Draw title string out of screen
	VdpDrawText(VDP_PLANEA_ADDR, md.menuXPos + MenuStrAlign(m->title,
			MENU_H_ALIGN_CENTER, 0), MENU_LINE_TITLE, MENU_COLOR_TITLE,
			MENU_LINE_CHARS_TOTAL, m->title.string);
	// Draw context strings
	VdpDrawText(VDP_PLANEA_ADDR, md.menuXPos + m->margin, MENU_LINE_CONTEXT,
			MENU_COLOR_CONTEXT_L, MENU_LINE_CHARS_TOTAL, m->lContext.string);
	VdpDrawText(VDP_PLANEA_ADDR, md.menuXPos + MenuStrAlign(md.rContext,
			MENU_H_ALIGN_RIGHT, m->margin), MENU_LINE_CONTEXT,
			MENU_COLOR_CONTEXT_R, MENU_LINE_CHARS_TOTAL, md.rContext.string);
	// Draw page (including selected item)
	MenuDrawPage();
	// X-scroll plane to show drawn menu
	MenuXScroll(direction);
	// Advance page counter
	md.menuXPos ^= MENU_SEPARATION_CHR;
	// Clear screen zone that has been hidden
	MenuClearLines(0, MENU_NLINES_TOTAL);
}

