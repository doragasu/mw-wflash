/************************************************************************//**
 * \brief Menu implementation. This menu allows to define menu data
 * structures, and alow allows to display them and handle options.
 *
 * \author	Jesús Alonso (doragasu)
 * \date	2017
 ****************************************************************************/
#include "menu.h"
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

typedef struct {
	const MenuEntry *root;		///< Root menu entry
	const MenuEntry *current;	///< Current menu entry
	/// Reserve space for the rContext string
	char rConStr[MENU_LINE_CHARS_TOTAL];
	MenuString rContext;		///< Right context string (bottom line)
	uint8_t level;				///< Current menu level
	/// Selected item, for each menu level
	uint8_t selItem[MENU_NLEVELS];
	/// Selected menu item page
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
				MENU_LINE_CHARS_TOTAL - margin - mStr.length - 1;

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
		VdpDmaWait();
	}
}

void MenuStatStrSet(MenuString statStr) {
	// Current menu entry
	const MenuEntry *m = md.current;

	memcpy(md.rContext.string, statStr.string, statStr.length);
	md.rContext.length = statStr.length;
	// Redraw context string
	MenuClearLines(MENU_LINE_CONTEXT, MENU_LINE_CONTEXT, 0);
	VdpDrawText(VDP_PLANEA_ADDR, m->margin, MENU_LINE_CONTEXT,
			MENU_COLOR_CONTEXT_L, MENU_LINE_CHARS_TOTAL,
			m->lContext.string);
	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(md.rContext, MENU_H_ALIGN_RIGHT,
			m->margin), MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_R,
			MENU_LINE_CHARS_TOTAL, md.rContext.string);
}

/// \param[in] chrOff Character offset in plane to draw menu
void MenuDrawPage(uint8_t chrOff) {
	// Current menu
	const MenuEntry *m = md.current;
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
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MenuStrAlign(
			m->item[i].caption, m->align, m->margin), line,
			i == md.selItem[md.level]?MENU_COLOR_ITEM_SEL:MENU_COLOR_ITEM,
			MENU_LINE_CHARS_TOTAL, m->item[i].caption.string);
	}
	// Draw page number and total, if number of pages greater than 1
	if (m->pages > 1) {
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			m->margin - 1, MENU_LINE_PAGER, MENU_COLOR_PAGER, m->pages + 1);
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			m->margin - 2, MENU_LINE_PAGER, MENU_COLOR_PAGER, 1, "/");
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			m->margin - 3, MENU_LINE_PAGER, MENU_COLOR_PAGER, m->pages + 1);
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
	const MenuEntry *m = md.current;
	uint16_t offset;

	offset = direction == MENU_SCROLL_DIR_LEFT?MENU_SEPARATION_CHR:
		(VDP_PLANE_HTILES - MENU_SEPARATION_CHR);
	// Draw title string out of screen
	VdpDrawText(VDP_PLANEA_ADDR, offset + MenuStrAlign(m->title,
			MENU_H_ALIGN_CENTER, 0), MENU_LINE_TITLE, MENU_COLOR_TITLE,
			MENU_LINE_CHARS_TOTAL, m->title.string);
	// Draw context strings
	VdpDrawText(VDP_PLANEA_ADDR, offset + m->margin,
			MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_L, MENU_LINE_CHARS_TOTAL,
			m->lContext.string);
	VdpDrawText(VDP_PLANEA_ADDR, offset +
			MenuStrAlign(md.rContext, MENU_H_ALIGN_RIGHT, m->margin),
			MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_R, MENU_LINE_CHARS_TOTAL,
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
	memcpy(md.rConStr, rContext.string, rContext.length);
	md.rContext.length = rContext.length;
	// Set root and curren menu entries
	md.root = root;
	md.current = root;
	// Set context string
	strncpy(md.rConStr, rContext.string, MENU_LINE_CHARS_TOTAL);
	md.rContext = rContext;
	VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR, 0);

	// Draw root menu
	MenuDraw(MENU_SCROLL_DIR_LEFT);
}

