/************************************************************************//**
 * \brief Menu implementation. This menu allows to define menu data
 * structures, and alow allows to display them and handle options.
 *
 * \author	Jesús Alonso (doragasu)
 * \date	2017
 ****************************************************************************/
#include "menu.h"
#include <string.h>

/// Number of characters horizontally separating each menu
#define MENU_SEPARATION_CHR		64
/// Number of pixels separating each menu
#define MENU_SEPARATION_PX		(MENU_SEPARATION_CHR*8)

typedef struct {
	MenuEntry *root;		///< Root menu entry
	MenuEntry *current;		///< Current menu entry
	/// Reserve space for the rContext string
	char rConStr[MENU_LINE_CHARS_TOTAL];
	MenuString rContext;	///< Right context string (bottom line)
	int8_t menuXPos;		///< X coord. of menu start (0 or 64)
	uint8_t level;			///< Current menu level
	/// Selected item, for each menu level
	uint8_t selItem[MENU_NLEVELS];
	uint8_t curPage;		///< Current menu item page
	uint8_t totPage;		///< Total number of menu item pages
} Menu;

void MenuPaletteFadeIn(void) {
}

void MenuPaletteFadeOut(void) {
}

Menu md;

void MenuInit(MenuEntry *root, MenuString rContext) {
	memset((void*)&md, 0, sizeof(Menu));
	md.rContext.string = md.rConStr;
	md.root = root;
	md.rContext = rContext;
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

void MenuClearPage(void) {
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

	pageItems = md.curPage == m->pages - 1?m->nItems - (m->entPerPage *
			m->pages - 1):m->entPerPage;
	// Draw menu items in page
	for (i = 0, line = MENU_LINE_ITEM_FIRST; i < pageItems; i++,
			line += m->spacing) {
		// TODO: Change color of selected item
		VdpDrawText(VDP_PLANEA_ADDR, md.menuXPos + MenuStrAlign(
			m->item[i].caption, m->align, m->margin), line, MENU_COLOR_ITEM,
			MENU_LINE_CHARS_TOTAL, m->item[i].caption.string);
	}
	// Draw page number and total
	
}

/// \note The function assumes the screen half in which it will draw the
/// menu, is already clear. It also clears the screen half that falls
/// outside of the screen, when it switches menu.
void MenuDraw(void) {
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
	// Draw page depending on selItem
	MenuDrawPage();
	// Draw selected item
	// X-scroll plane to show drawn menu
	// Clear screen zone that has been hidden
}

