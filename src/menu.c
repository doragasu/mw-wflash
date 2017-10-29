/************************************************************************//**
 * \brief Menu implementation. This menu allows to define menu data
 * structures, and alow allows to display them and handle options.
 *
 * \author	Jesús Alonso (doragasu)
 * \date	2017
 ****************************************************************************/

// NOTES:
// Each menu screen is defined by one MenuEntry and one or more MenuItems.
// The MenuEntry defines the screen layout, and the MenuItems define the
// items that appear in the menu.
//
// MenuEntry and MenuItems shall be const structures to preserver RAM. When
// a menu is entered, these structures that define the menu are copied from
// ROM to RAM. This is enough to draw simple (static) menus, but more dynamic
// menus require that the copied menu data is modified before the menu is
// displayed. For this the module implements several callback functions, that
// are called in several situations, and can modify the menu data as needed:
// - MenuEntry::entry: called when the menu is entered.
// - MenuEntry::exit:  called when the menu is exited. Invalidates the menu
//   exit if the callback returns FALSE.
// - MenuEntry::cBut:  called when C button is pressed.
//
// In addition to the "standard" menus, based on MenuEntry and MenuItems, this
// module implements On Screen Keyboard (OSK) menus that can be used for the
// user to input data. Supported OSK menus are:
// - MENU_OSK_OSK_QWERTY: QWERTY type keyboard, to enter text, numbers and
//   symbols.
// - MENU_OSK_OSK_NUMERIC: Keyboard allowing entering only 0~9 digits.
// - MENU_OSK_OSK_IPV4: Menu allowing to enter IPv4 addresses only.
// These OSK menus allow to define callbacks to validate input data.
//
// In addition to the menu data, there are several strings that can be set:
// - MenuMessage() overlays a simple "message box" to the current menu.
// - MenuStatStrSet() sets the status string on the bottom right of the
//   screen.
// - MenuPanic() is used for critical error notifications.
//
// Menu transitions are tracked using an internal Menu variable. This allows
// the user to navigate the menus going forward/back. This has forced to code
// a memory pool management module in order to maintain the menu structures.
// Memory is allocated while navigating deeper into menus, and is freed when
// navigating menus back. Note that this might be dangerous because reaching
// the deeper levels of the menu structures might exhaust the memory pool.
// The pool floor is tracked when entering a menu, and freed when exiting it.
// It might be possible freeing the memory used for the current menu just
// before entering next, to keep the memory footprint low, but this might
// complicate to a great extent going backwards from a menu to the previous
// one.
#include "menu.h"
#include "gamepad.h"
#include "mpool.h"
#include <string.h>
#include "util.h"

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
#define MENU_OSK_QWERTY_ROWS		4
/// Number of columns of the QWERTY menu
#define MENU_OSK_QWERTY_COLS		11

/// Obtains the number of scroll steps for the menu scroll function
#define MENU_SCROLL_NSTEPS	(sizeof(scrDelta))

#define MenuGetCurrentItemNum()		(md->me->selItem + \
		md->me->selPage * md->me->mEntry.mItem.entPerPage)

/// Returns the number of items on current page
#define MenuNumPageItems()	\
		(md->me->selPage == md->me->mEntry.mItem.pages? \
		md->me->mEntry.mItem.nItems - (md->me->mEntry.mItem.entPerPage * \
		md->me->mEntry.mItem.pages):md->me->mEntry.mItem.entPerPage)

/// Alphanumeric menu definition
static const char qwerty[2 * MENU_OSK_QWERTY_ROWS][MENU_OSK_QWERTY_COLS] = {{
	//   0   1   2   3   4   5   6   7   8   9  10
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

/// Reverse keyboard lookup table, to get QWERTY keyboard coordinates from
/// the ascii character (subtracting the ' ' character and indexing in this
/// table
static const MenuOskCoord qwertyRev[96] = {
	// SP       !        "        #        $        &        '        (
	{0,4,0}, {1,0,0}, {1,2,10},{1,0,2}, {1,0,3}, {1,0,4}, {1,0,6}, {0,2,10},
	// (        )        *        +        ,        -        .        /
	{1,0,8}, {1,0,9}, {1,0,7}, {0,3,10},{0,3,7}, {0,0,10},{0,3,8}, {0,3,9},
	// 0        1        2        3        4        5        6        7
	{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3} ,{0,0,4}, {0,0,5}, {0,0,6}, {0,0,7},
	// 8        9        :        ;        <        =        >        ?
	{0,0,8}, {0,0,9}, {1,2,9}, {0,2,9} ,{1,3,7}, {1,3,10},{1,3,8}, {1,3,9},
	// @        A        B        C        D        E        F        G
	{1,0,1}, {0,2,0}, {0,3,4}, {0,3,2}, {0,2,2}, {0,1,2}, {0,2,3}, {0,2,4},
	// H        I        J        K        L        M        N        O
	{0,2,5}, {0,1,7}, {0,2,6}, {0,2,7}, {0,2,8}, {0,3,6}, {0,3,5}, {0,1,8},
	// P        Q        R        S        T        U        V        W
	{0,1,9}, {0,1,0}, {0,1,3}, {0,2,1}, {0,1,4}, {0,1,6}, {0,3,3}, {0,1,1},
	// X        Y        Z        [         \\        ]      ^        _
	{0,3,1}, {0,1,5}, {0,3,0}, {1,7,15},{1,7,15},{1,7,15},{1,0,5}, {1,0,10},
	// `        a        b        c        d        e        f        g
	{1,7,15},{1,2,0}, {1,3,4}, {1,3,2}, {1,2,2}, {1,1,2}, {1,2,3}, {1,2,4},
	// h        i        j        k        l        m        n        o
	{1,2,5}, {1,1,7}, {1,2,6}, {1,2,7}, {1,2,8}, {1,3,6}, {1,3,5}, {1,1,8},
	// p        q        r        s        t        u        v        w
	{1,1,9}, {1,1,0}, {1,1,3}, {1,2,1}, {1,1,4}, {1,1,6}, {1,3,3}, {1,1,1},
	// x        y        z        {        |        }        ~      CURSOR
	{1,3,1}, {1,1,5}, {1,3,0}, {1,7,15},{1,7,15},{1,7,15},{1,7,15},{1,7,15}
};

/// Supported virtual menu actions
typedef enum {
	MENU_OSK_FUNC_CANCEL = 0,	///< Cancel edit
	MENU_OSK_FUNC_DEL,			///< Delete character
	MENU_OSK_FUNC_LEFT,			///< Move to left
	MENU_OSK_FUNC_RIGHT,		///< Move to right
	MENU_OSK_FUNC_DONE,			///< Done edit
	MENU_OSK_NUM_FUNCS			///< Number of special function keys
} MenuOskSpecialFuncs;

/// Special function key labels
static const char qwertyFunc[MENU_OSK_NUM_FUNCS][4] = {
	"BACK", "DEL", " <-", " ->", "DONE"
};

/// Space key label
static const char qwertySpace[] = "[SPACE]";

/// Number of columns of the IP OSK menu
#define MENU_OSK_IP_COLS	3
/// Number of rows of the IP OSK menu
#define MENU_OSK_IP_ROWS	4

/// IP entry menu definition
static const char ip[MENU_OSK_IP_ROWS][MENU_OSK_IP_COLS] = {
	{'7', '8', '9'},
	{'4', '5', '6'},
	{'1', '2', '3'},
	{'.', '0', '.'}
};

/// Number of columns of the IP OSK menu
#define MENU_OSK_NUM_COLS	3
/// Number of rows of the IP OSK menu
#define MENU_OSK_NUM_ROWS	4

/// Numeric entry menu definition
static const char num[MENU_OSK_NUM_ROWS][MENU_OSK_NUM_COLS] = {
	{'7', '8', '9'},
	{'4', '5', '6'},
	{'1', '2', '3'},
	{' ', '0', ' '}
};

/// Dynamic data needed to display the menus
static Menu *md;

/************************************************************************//**
 * Sets background to red, writes a panic message and loops forever.
 *
 * \param[in] errStr String with the error that causes the panic status
 * \param[in] len    Length in characters of errStr string.
 ****************************************************************************/
void MenuPanic(char errStr[], uint8_t len) {
	const MenuString error = {.string = errStr, .length = len, .flags = 0};

	// Set background to red and write panic message
	VdpRamWrite(VDP_CRAM_WR, 0x00, VDP_COLOR_RED);
	MenuMessage(error, 0);

	while (1);
}

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
	const MenuEntry *m = &md->me->mEntry;

	memcpy(md->rContext.string, statStr.string, statStr.length + 1);
	md->rContext.length = statStr.length;
	// Redraw context string
	MenuClearLines(MENU_LINE_CONTEXT, MENU_LINE_CONTEXT, 0);
	VdpDrawText(VDP_PLANEA_ADDR, m->margin, MENU_LINE_CONTEXT,
			MENU_COLOR_CONTEXT_L, m->lContext.length, m->lContext.string);
	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(md->rContext, MENU_H_ALIGN_RIGHT,
			m->margin), MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_R,
			statStr.length, md->rContext.string);
}

/************************************************************************//**
 * Draw requested item page.
 *
 * \param[in] chrOff Character offset in plane to draw menu
 ****************************************************************************/
void MenuDrawItemPage(uint8_t chrOff) {
	// Current menu
	const MenuEntry *m = &md->me->mEntry;
	// Loop control
	uint8_t i;
	// Line to draw text in
	uint8_t line;
	// Number of items to draw in this page
	uint8_t pageItems;
	// Number of the item to draw
	uint8_t item;
	// Color used to draw the current item
	uint8_t color;

	// Clear previously drawn items
	MenuClearLines(MENU_LINE_ITEM_FIRST, MENU_LINE_ITEM_LAST, chrOff);
	// Get the number of items to draw on current page
	pageItems = MenuNumPageItems();
	// Keep selected item unless it points to a non existing option
	if (md->me->selItem >= pageItems)
		md->me->selItem =  pageItems - 1;
	// Draw menu items in page
	for (i = 0, line = MENU_LINE_ITEM_FIRST, item = md->me->selPage *
			m->mItem.entPerPage; i < pageItems; i++,
			line += m->mItem.spacing, item++) {
		if (i == md->me->selItem) {
			color = MENU_COLOR_ITEM_SEL;
		} else {
			color = m->mItem.item[item].alt_color?MENU_COLOR_ITEM_ALT:
				MENU_COLOR_ITEM;
		}
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MenuStrAlign(
			m->mItem.item[item].caption, m->mItem.align, m->margin), line,
			color, m->mItem.item[item].caption.length,
			m->mItem.item[item].caption.string);
	}
	// Draw page number and total, if number of pages greater than 1
	if (m->mItem.pages > 0) {
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			MENU_DEF_RIGHT_MARGIN - 1, MENU_LINE_PAGER, MENU_COLOR_PAGER,
			m->mItem.pages + 1);
		VdpDrawText(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			MENU_DEF_RIGHT_MARGIN - 2, MENU_LINE_PAGER, MENU_COLOR_PAGER, 1,
			"/");
		VdpDrawDec(VDP_PLANEA_ADDR, chrOff + MENU_LINE_CHARS_TOTAL - 
			MENU_DEF_RIGHT_MARGIN - 3, MENU_LINE_PAGER, MENU_COLOR_PAGER,
			md->me->selPage + 1);
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
 * Draws the current key on the edit field, with the plane offset and
 * color indicated.
 *
 * \param[in] offset    Character offset in which to draw the menu.
 * \param[in] textColor Text color definition as defined in vdp module.
 ****************************************************************************/
void MenuOskDrawEditKey(uint16_t offset, uint8_t textColor) {
	// Current menu entry
	const MenuEntry *m = &md->me->mEntry;
	char c;

	switch (m->type) {
		case MENU_TYPE_OSK_QWERTY:
			// Check if we are in a special key
			if (md->coord.col == MENU_OSK_QWERTY_COLS) c = 0x7F;
			else if (md->coord.row == MENU_OSK_QWERTY_ROWS) c = ' ';
			else
				c = qwerty[md->coord.caps * 4 + md->coord.row][md->coord.col];
			break;
			
		case MENU_TYPE_OSK_NUMERIC:
			if (md->coord.col == MENU_OSK_NUM_COLS) c = 0x7F;
			else c = num[md->coord.row][md->coord.col];
			break;

		case MENU_TYPE_OSK_IPV4:
			if (md->coord.col == MENU_OSK_IP_COLS) c = 0x7F;
			else c = ip[md->coord.row][md->coord.col];
			break;
	}
	// Draw the selected key with corresponding color
	VdpDrawText(VDP_PLANEA_ADDR, offset + ((MENU_LINE_CHARS_TOTAL -
			m->keyb.maxLen)>>1) + md->me->selItem, MENU_LINE_OSK_DATA,
			textColor, 1, &c);
}

/************************************************************************//**
 * Draw on screen keybaoard function keys.
 *
 * \param[in] offset    Character offset in which to draw the menu.
 ****************************************************************************/
void MenuDrawOskFunc(uint16_t offset) {
	uint8_t i, color, cols;

	cols = 0;
	switch(md->me->mEntry.type) {
		case MENU_TYPE_OSK_QWERTY:
			cols = MENU_OSK_QWERTY_COLS;
			break;

		case MENU_TYPE_OSK_NUMERIC:
			cols = MENU_OSK_NUM_COLS;
			break;

		case MENU_TYPE_OSK_IPV4:
			cols = MENU_OSK_IP_COLS;
			break;
	}
	for (i = 0; i < MENU_OSK_NUM_FUNCS; i++) {
		if (md->coord.col == cols && md->coord.row == i)
			color = MENU_COLOR_ITEM_SEL;
		else color = MENU_COLOR_ITEM;
		VdpDrawText(VDP_PLANEA_ADDR, offset + 2 * MENU_OSK_QWERTY_COLS +
				+ 6 + 2, MENU_LINE_OSK_KEYS + 2 * i,
				color, 4, (char*)qwertyFunc[i]);
	}
}

/************************************************************************//**
 * Draws the currently active On Screen Keyboard.
 *
 * \param[in] offset Character offset in which to draw the menu
 ****************************************************************************/
void MenuDrawOsk(uint16_t offset) {
	uint8_t i, j;
	uint8_t color;

	// Current menu entry
	const MenuEntry *m = &md->me->mEntry;

	// Draw the field name to edit
	VdpDrawText(VDP_PLANEA_ADDR, offset + m->margin, MENU_LINE_OSK_FIELD,
			MENU_COLOR_OSK_FIELD, m->keyb.fieldName.length,
			m->keyb.fieldName.string);

	// Draw the field data to edit, centered with respect to the maximum
	// size of the data string.
	VdpDrawText(VDP_PLANEA_ADDR, offset + ((MENU_LINE_CHARS_TOTAL -
			m->keyb.maxLen)>>1), MENU_LINE_OSK_DATA, MENU_COLOR_OSK_DATA,
			md->str.length, md->str.string);
	// Draw the selected key with corresponding color
	MenuOskDrawEditKey(offset, MENU_COLOR_ITEM_SEL);

	// Draw the on screen keyboard
	switch (m->type) {
		case MENU_TYPE_OSK_QWERTY:	// Draw QWERTY OSK
			for (i = 0; i < MENU_OSK_QWERTY_ROWS; i++) {
				for (j = 0; j < MENU_OSK_QWERTY_COLS; j++) {
					if (md->coord.row == i && md->coord.col == j)
						color = MENU_COLOR_ITEM_SEL;
					else color = MENU_COLOR_ITEM;
					VdpDrawText(VDP_PLANEA_ADDR, offset - 3 +
						((MENU_LINE_CHARS_TOTAL - 2*MENU_OSK_QWERTY_COLS)>>1) +
						2 * j, MENU_LINE_OSK_KEYS + 2 * i, color, 1,
						(char*)&qwerty[i + md->coord.caps * 4][j]);
				}
			}
			// Draw space bar
			if (md->coord.row == MENU_OSK_QWERTY_ROWS && md->coord.col != 
					MENU_OSK_QWERTY_COLS)
					color = MENU_COLOR_ITEM_SEL;
			else color = MENU_COLOR_ITEM;
			VdpDrawText(VDP_PLANEA_ADDR, offset +
					((sizeof(qwertySpace) - 1)) - 3 +
					((MENU_LINE_CHARS_TOTAL - 2*MENU_OSK_QWERTY_COLS)>>1) ,
					MENU_LINE_OSK_KEYS + 2 * MENU_OSK_QWERTY_ROWS,
					color, sizeof(qwertySpace) - 1,
					(char*)qwertySpace);
			// Draw special keys
			MenuDrawOskFunc(offset);
			break;

		case MENU_TYPE_OSK_NUMERIC:	// Draw numeric OSK
			for (i = 0; i < MENU_OSK_NUM_ROWS; i++) {
				for (j = 0; j < MENU_OSK_NUM_COLS; j++) {
					if (md->coord.row == i && md->coord.col == j)
						color = MENU_COLOR_ITEM_SEL;
					else color = MENU_COLOR_ITEM;
					VdpDrawText(VDP_PLANEA_ADDR, offset - 3 +
						((MENU_LINE_CHARS_TOTAL - 2 * MENU_OSK_NUM_COLS)>>1) +
						2 * j, MENU_LINE_OSK_KEYS + 2 * i, color, 1,
						(char*)&num[i][j]);
				}
			}
			// Draw special keys
			MenuDrawOskFunc(offset);
			break;

		case MENU_TYPE_OSK_IPV4:	// Draw IPv4 OSK
			for (i = 0; i < MENU_OSK_IP_ROWS; i++) {
				for (j = 0; j < MENU_OSK_IP_COLS; j++) {
					if (md->coord.row == i && md->coord.col == j)
						color = MENU_COLOR_ITEM_SEL;
					else color = MENU_COLOR_ITEM;
					VdpDrawText(VDP_PLANEA_ADDR, offset - 3 +
						((MENU_LINE_CHARS_TOTAL - 2 * MENU_OSK_IP_COLS)>>1) +
						2 * j, MENU_LINE_OSK_KEYS + 2 * i, color, 1,
						(char*)&ip[i][j]);
				}
			}
			// Draw special keys
			MenuDrawOskFunc(offset);
			break;

	}
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
	const MenuEntry *m = &md->me->mEntry;
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
	VdpDrawText(VDP_PLANEA_ADDR, offset + MenuStrAlign(md->rContext,
			MENU_H_ALIGN_RIGHT, MENU_DEF_RIGHT_MARGIN),
			MENU_LINE_CONTEXT, MENU_COLOR_CONTEXT_R, md->rContext.length,
			md->rContext.string);
	// Depending on menu type, draw menu page contents
	switch (m->type) {
		case MENU_TYPE_ITEM:
			// Draw page (including selected item)
			MenuDrawItemPage(offset);
			break;

		case MENU_TYPE_OSK_QWERTY:
			// Copy string, place cursor at the end, and enter menu
			MenuStringCopy(&md->str, &m->keyb.fieldData);
			md->me->selItem = md->str.length;
			// Select the "DONE" item
			md->coord.caps = 0;
			md->coord.row = MENU_OSK_QWERTY_ROWS;
			md->coord.col = MENU_OSK_QWERTY_COLS;
			MenuDrawOsk(offset);
			break;

		case MENU_TYPE_OSK_NUMERIC:
			MenuStringCopy(&md->str, &m->keyb.fieldData);
			md->me->selItem = md->str.length;
			// Select the "DONE" item
			md->coord.caps = 0;
			md->coord.row = MENU_OSK_NUM_ROWS;
			md->coord.col = MENU_OSK_NUM_COLS;
			MenuDrawOsk(offset);
			break;

		case MENU_TYPE_OSK_IPV4:
			MenuStringCopy(&md->str, &m->keyb.fieldData);
			md->me->selItem = md->str.length;
			// Select the "DONE" item
			md->coord.caps = 0;
			md->coord.row = MENU_OSK_IP_ROWS;
			md->coord.col = MENU_OSK_IP_COLS;
			MenuDrawOsk(offset);
		default:
			break;
	}

	// X-scroll plane to show drawn menu
	MenuXScroll(direction);
	// Clear screen zone that has been hidden
	MenuClearLines(0, MENU_NLINES_TOTAL, offset);
}

/// Requires the string metadata to be previously copied, only loads the
/// string contents when necessary.
static void MenuStringLoad(MenuString *dst, const MenuString *org) {
	// Only load editable strings (const strings can be kept at ROM)
	if (org->editable) {
		// Flags and length were already loaded
//		dst->flags = org->flags;
//		dst->length = org->length;
		// Allocate memory for the string buffer
		dst->string = MpAlloc(org->length);
		// Copy the string data unless empty
		memset(dst->string, 0, org->length);
		if (!org->empty) memcpy(dst->string, org->string, org->length);
	}
}

/// Loads menu items into a loaded MenuEntity
static void MenuItemsLoad(MenuEntry *dst, const MenuEntry *menu) {
	size_t len;
	int i;

	// Copy all items (excepting MenuString contents)
	len = dst->mItem.nItems * sizeof(MenuItem);
	dst->mItem.item = MpAlloc(len);
	memcpy(dst->mItem.item, menu->mItem.item, len);
	// Load MenuStrings
	MenuStringLoad(&dst->title, &menu->title);
	MenuStringLoad(&dst->lContext, &menu->lContext);
	
	for (i = 0; i < dst->mItem.nItems; i++) {
		MenuStringLoad(&dst->mItem.item[i].caption,
				&menu->mItem.item[i].caption);
	}
}

/// Loads the specified menu, and adds it to the menu list. Currently active
/// menu when this function is called, is added to the new menu ->prev entry.
MenuEntity *MenuLoad(const MenuEntry *menu) {
	MenuEntity *tmp;

	// Allocate  for the new menu entry
	if (!(tmp = MpAlloc(sizeof(MenuEntity)))) return NULL;
	memset(tmp, 0, sizeof(MenuEntity));
	tmp->prev = md->me;
	md->me = tmp;
	// Copy the menu entry
	md->me->mEntry = *menu;
	// Alloc and copy entry data depending on menu type
	switch (md->me->mEntry.type) {
		case MENU_TYPE_ITEM:
			// Copy items
			MenuItemsLoad(&md->me->mEntry, menu);
			break;

		case MENU_TYPE_OSK_QWERTY:
		case MENU_TYPE_OSK_NUMERIC:
		case MENU_TYPE_OSK_IPV4:
			// Load MenuStrings
			MenuStringLoad(&md->me->mEntry.keyb.fieldName,
						   &menu->keyb.fieldName);
			MenuStringLoad(&md->me->mEntry.keyb.fieldData,
						   &menu->keyb.fieldData);
			break;
	}
	return tmp;
}

void MenuUnload(void) {
	MenuEntity *tmp;

	tmp = md->me;
	md->me = md->me->prev;
	MpFreeTo(tmp);
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
	// Initialize memory pool used for the menus
	MpInit();
	// Allocate memory for the global MenuData structure
	md = MpAlloc(sizeof(Menu));
	/// \todo Allocate memory for the required configuration structures,
	/// and maybe remove strBuf and str from Menu structure
	// Zero module data
	memset((void*)md, 0, sizeof(Menu));
	// Load root menu and set menu entities
	md->me = NULL;
	MenuLoad(root);
	// Set string to point to string data and copy input context
	md->rContext = rContext;
	md->rContext.string = md->rConStr;
	strncpy(md->rConStr, rContext.string, rContext.length);

	VdpRamWrite(VDP_VRAM_WR, VDP_HSCROLL_ADDR, 0);
	// Initialize temp string
	md->str.string = md->strBuf;

	// Draw root menu
	MenuDraw(MENU_SCROLL_DIR_LEFT);
}

/************************************************************************//**
 * Advances page number to the next one (but does not draw the page).
 ****************************************************************************/
static inline void MenuNextPage(void) {
	md->me->selPage++;
	if (md->me->selPage > md->me->mEntry.mItem.pages)
		md->me->selPage = 0;
}

/************************************************************************//**
 * Sets page number to the previous one (but does not draw the page).
 ****************************************************************************/
static inline void MenuPrevPage(void) {
	md->me->selPage = md->me->selPage?md->me->selPage - 1:
		md->me->mEntry.mItem.pages;
}

/************************************************************************//**
 * Draw current item for MENU_TYPE_ITEM menus, with specified color.
 *
 * \param[in] txtColor Color used to draw the item.
 ****************************************************************************/
static inline void MenuDrawCurrentItem(uint8_t txtColor) {
	uint8_t line, item;
	const MenuEntry *m = &md->me->mEntry;

	line = MENU_LINE_ITEM_FIRST + md->me->selItem * m->mItem.spacing;
	item = md->me->selItem + md->me->selPage * m->mItem.entPerPage;
	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(m->mItem.item[item].caption,
			m->mItem.align, m->margin), line, txtColor,
			m->mItem.item[item].caption.length,
			m->mItem.item[item].caption.string);
}

/************************************************************************//**
 * Perform menu actions for a MENU_TYPE_ITEM menu, depending in the pressed
 * key.
 *
 * \param[in] input Input key changes.
 ****************************************************************************/
void MenuItemAction(uint8_t input) {
	uint16_t tmp, i;
	const MenuEntry *m = &md->me->mEntry;

	// Parse buttons before movement
	if (input & GP_A_MASK) {
		// If defined run callback, and perform item actions if authorized
		tmp = MenuGetCurrentItemNum();
		if ((m->mItem.item[tmp].cb) && (!m->mItem.item[tmp].cb(&md))) return;
		// Accept selected menu option
		if (m->mItem.item[tmp].next) {
			// Call exit callback before exiting menu
			if (m->exit) m->exit(&md);
			// Load next menu:
			if (!MenuLoad(m->mItem.item[tmp].next))
				MenuPanic("MENU LEVELS EXHAUSTED!", 22);
			// Call menu entry callback
			if (md->me->mEntry.entry) md->me->mEntry.entry(&md);
			// Select page and item
			for (i = 0; !md->me->mEntry.mItem.item[i].selectable; i++);
			md->me->selItem = i;
			md->me->selPage = 0;
			// Draw menu
			MenuDraw(MENU_SCROLL_DIR_LEFT);
		}
	} else if (input & GP_B_MASK) {
		// Only unload if we are not at the root menu
		if (md->me->prev) {
			// If there is exit callback, execute it
			if (m->exit) m->exit(&md);
			MenuUnload();
			// Call menu entry callback
			if (md->me->mEntry.entry) md->me->mEntry.entry(&md);
			MenuDraw(MENU_SCROLL_DIR_RIGHT);
		}
	} else if (input & GP_UP_MASK) {
		// Go up a menu item, and continue while item is not selectable
		do {
			if (md->me->selItem) {
				md->me->selItem--;
			} else {
				// Go to previous page and select last item
				MenuPrevPage();
				md->me->selItem = MenuNumPageItems() - 1;
			}
		} while (md->me->mEntry.mItem.item[md->me->selPage *
				md->me->mEntry.mItem.entPerPage + md->me->selItem].
				selectable == 0);
		MenuDrawItemPage(0);
	} else if (input & GP_DOWN_MASK) {
		// Go down a menu item
		tmp = MenuNumPageItems() - 1;
		// Advance once, and continue advancing while item not selectable
		do {
			if (md->me->selItem < tmp) {
				md->me->selItem++;
			} else {
				// Advance to next page, and select first item
				MenuNextPage();
				md->me->selItem = 0;
			}
		} while (md->me->mEntry.mItem.item[md->me->selPage *
				md->me->mEntry.mItem.entPerPage + md->me->selItem].
				selectable == 0);
		MenuDrawItemPage(0);
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

/************************************************************************//**
 * Draw current QWERTY OSK key, in specified color.
 *
 * \param[in] color Color used to draw the current key.
 ****************************************************************************/
void MenuOskQwertyDrawCurrent(uint8_t color) {
	uint8_t i;
	// Determine if we have to draw a special key
	if (md->coord.col >= MENU_OSK_QWERTY_COLS) {
		i = md->coord.row;
		VdpDrawText(VDP_PLANEA_ADDR, 2 * MENU_OSK_QWERTY_COLS + 6 + 2,
				MENU_LINE_OSK_KEYS + 2 * i, color, 4,
				(char*)qwertyFunc[i]);
	// Determine if we have to draw the space bar
	} else if (md->coord.row >= MENU_OSK_QWERTY_ROWS) {
		VdpDrawText(VDP_PLANEA_ADDR, ((sizeof(qwertySpace) - 1)) - 3 +
				((MENU_LINE_CHARS_TOTAL - 2*MENU_OSK_QWERTY_COLS)>>1) ,
				MENU_LINE_OSK_KEYS + 2 * MENU_OSK_QWERTY_ROWS, color
				, sizeof(qwertySpace) - 1, (char*)qwertySpace);
	} else {
		// Draw normal character
		i = md->coord.caps * 4 + md->coord.row;
		VdpDrawText(VDP_PLANEA_ADDR, ((MENU_LINE_CHARS_TOTAL - 2 *
				MENU_OSK_QWERTY_COLS)>>1) + 2 * md->coord.col - 3,
				MENU_LINE_OSK_KEYS + 2 * md->coord.row, color, 1,
				(char*)&qwerty[i][md->coord.col]);
	}
}

/************************************************************************//**
 * Adds specified character to the edited string using an on-screen
 * keyboard.
 *
 * \param[in] c Character to add to the edited string.
 ****************************************************************************/
void MenuAddChar(char c) {
	const MenuEntry *m = &md->me->mEntry;

	// If we are at the end of the string, and there is room, add the
	// character. If not at the end of the string, edit current caracter
	// and advance one position.
	if (md->me->selItem == md->str.length) {
		if (md->me->selItem < m->keyb.maxLen) {
			/// \todo check if last position, and move to "DONE" in that case
			MenuOskDrawEditKey(0, MENU_COLOR_OSK_DATA);
			md->str.string[md->me->selItem++] = c;
			md->str.length = md->me->selItem;
			MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
		}
	} else {
		// We are in the middle of the string, advance to next character
		MenuOskDrawEditKey(0, MENU_COLOR_OSK_DATA);
		md->str.string[md->me->selItem++] = c;
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	}
}

/************************************************************************//**
 * Deletes previous character edited on the on-screen keyboard.
 ****************************************************************************/
void MenuOskKeyDel(void) {
	const MenuEntry *m = &md->me->mEntry;

	// If currently at origin, key cannot be deleted
	if (!md->me->selItem) return;
	// If we are at the last character, just clear it.
	if (md->me->selItem == md->str.length) {
		// Clear current character
		VdpDrawText(VDP_PLANEA_ADDR, ((MENU_LINE_CHARS_TOTAL -
				m->keyb.maxLen)>>1) + md->me->selItem, MENU_LINE_OSK_DATA,
				MENU_COLOR_OSK_DATA, 1, " ");
		md->me->selItem--;
	} else {
		// Not at the end of the string, copy all characters from this
		// position, 1 character to the left
		// First clear last character
		VdpDrawText(VDP_PLANEA_ADDR, ((MENU_LINE_CHARS_TOTAL -
				m->keyb.maxLen)>>1) + md->str.length - 1, MENU_LINE_OSK_DATA,
				MENU_COLOR_OSK_DATA, 1, " ");
		strncpy(md->str.string + md->me->selItem - 1, md->str.string +
				md->me->selItem, md->str.length - md->me->selItem);
		md->str.string[md->str.length] = '\0';
		md->me->selItem--;
	}
	md->str.length--;
	MenuDrawOsk(0);
}

/************************************************************************//**
 * Commits and end the string edition using an on-screen keyboard.
 ****************************************************************************/
void MenuOskDone(void) {
	const MenuEntry *m = &md->me->mEntry;

	// Add null termination to string
	md->strBuf[md->str.length] = '\0';	
	// If exit callback defined, run it and perform transition if allowed.
	if ((m->exit) && (!m->exit(&md))) return;
	// Copy temporal string to menu entry string and scroll back
	MenuStringCopy((MenuString*)&m->keyb.fieldData, &md->str);
	// Free menu resources and go back to the previous menu
	MenuUnload();
	MenuDraw(MENU_SCROLL_DIR_RIGHT);
}

/************************************************************************//**
 * Performs a cursor shift to the left for on-screen keyboards.
 ****************************************************************************/
void MenuOskEditLeft(void) {
	const MenuEntry *m = &md->me->mEntry;

	// If we are at the origin, ignore key
	if (md->me->selItem == 0) return;
	// Clear current character
	VdpDrawText(VDP_PLANEA_ADDR, ((MENU_LINE_CHARS_TOTAL -
			m->keyb.maxLen)>>1) + md->me->selItem, MENU_LINE_OSK_DATA,
			MENU_COLOR_OSK_DATA, 1, " ");
	md->me->selItem--;
	MenuDrawOsk(0);
}

/************************************************************************//**
 * Performs a cursor shift to the right for on-screen keyboards.
 ****************************************************************************/
void MenuOskEditRight(void) {
	// If we are at the end, ignore key
	if (md->me->selItem == md->str.length) return;
	md->me->selItem++;
	MenuDrawOsk(0);
}

/************************************************************************//**
 * Handles key presses for the virtual keyboards.
 ****************************************************************************/
void MenuOskKeyPress(void) {
	uint8_t cols;

	switch (md->me->mEntry.type) {
		case MENU_TYPE_OSK_QWERTY:
			cols = MENU_OSK_QWERTY_COLS;
			break;

		case MENU_TYPE_OSK_NUMERIC:
			cols = MENU_OSK_NUM_COLS;
			break;

		case MENU_TYPE_OSK_IPV4:
			cols = MENU_OSK_IP_COLS;
			break;

		default:
			cols = 0;
	}
	// Find if a special key has been pressed
	if (md->coord.col == cols) {
		// Parse special virtual keys
		switch (md->coord.row) {
			case MENU_OSK_FUNC_CANCEL:
				// Free menu resources and go to the previous menu
				MenuUnload();
				MenuDraw(MENU_SCROLL_DIR_RIGHT);
				break;

			case MENU_OSK_FUNC_DEL:
				MenuOskKeyDel();
				break;

			case MENU_OSK_FUNC_LEFT:
				MenuOskEditLeft();
				break;

			case MENU_OSK_FUNC_RIGHT:
				MenuOskEditRight();
				break;

			case MENU_OSK_FUNC_DONE:
				MenuOskDone();
				break;

		}
	} else {
		// Parse normal key depending on keyboard type
		switch (md->me->mEntry.type) {
			case MENU_TYPE_OSK_QWERTY:
			   	if (md->coord.row == MENU_OSK_QWERTY_ROWS) {
					// Space pressed
					MenuAddChar(' ');
				} else {
					// Normal character pressed
					MenuAddChar(qwerty[md->coord.caps * 4 +
							md->coord.row][md->coord.col]);
				}
				break;

			case MENU_TYPE_OSK_NUMERIC:
				MenuAddChar(num[md->coord.row][md->coord.col]);
				break;

		case MENU_TYPE_OSK_IPV4:
			MenuAddChar(ip[md->coord.row][md->coord.col]);
			break;
		} // switch()
	}
}

/************************************************************************//**
 * Menu navigation through QWERTY virtual keyboard.
 *
 * \param[in] input Pad button changes in the format returned by GpPressed(),
 *                  but inverted (not).
 ****************************************************************************/
void MenuOskQwertyActions(uint8_t input) {
	if (input & GP_A_MASK) {
		MenuOskKeyPress();
	} else if (input & GP_B_MASK) {
		// Delete current character
		MenuOskKeyDel();
	} else if (input & GP_C_MASK) {
		md->coord.caps ^= 1;
		MenuDrawOsk(0);
	} else if (input & GP_START_MASK) {
		MenuOskDone();
	} else if (input & GP_UP_MASK) {
		// Draw current key with not selected color
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM);
		// Decrement row and draw key as selected.
		md->coord.row = md->coord.row?md->coord.row - 1:MENU_OSK_QWERTY_ROWS;
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_DOWN_MASK) {
		// Draw current key with not selected color
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM);
		// Increment row and draw key as selected.
		md->coord.row = md->coord.row == MENU_OSK_QWERTY_ROWS?0:
			md->coord.row + 1;
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_LEFT_MASK) {
		// Draw current key with not selected color
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM);
		// Decrement col and draw key as selected.
		// Special case: if we are on the last line, change between space
		// and bottom special key
		if (md->coord.row == MENU_OSK_QWERTY_ROWS) {
			md->coord.col = md->coord.col == MENU_OSK_QWERTY_COLS?0:
				MENU_OSK_QWERTY_COLS;
		} else {
			md->coord.col = md->coord.col?md->coord.col - 1:
				MENU_OSK_QWERTY_COLS;
		}
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_RIGHT_MASK) {
		// Draw current key with not selected color
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM);
		// Increment row and draw key as selected.
		// Special case: if we are on the last line, change between space
		// and bottom special key
		if (md->coord.row == MENU_OSK_QWERTY_ROWS) {
			md->coord.col = md->coord.col == MENU_OSK_QWERTY_COLS?0:
				MENU_OSK_QWERTY_COLS;
		} else {
			md->coord.col = md->coord.col == MENU_OSK_QWERTY_COLS?0:
				md->coord.col + 1;
		}
		MenuOskQwertyDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	}
}

/************************************************************************//**
 * Draw current item for MENU_TYPE_OSK_IPV4 menus, with specified color.
 *
 * \param[in] txtColor Color used to draw the item.
 ****************************************************************************/
void MenuOskIpDrawCurrent(uint8_t textColor) {
	uint8_t i;

	// Determine if we have to draw a special key
	if (md->coord.col >= MENU_OSK_IP_COLS) {
		i = md->coord.row;
		VdpDrawText(VDP_PLANEA_ADDR, 2 * MENU_OSK_QWERTY_COLS + 6 + 2,
				MENU_LINE_OSK_KEYS + 2 * i, textColor, 4,
				(char*)qwertyFunc[i]);
	} else {
		// Draw normal character
		i = md->coord.row;
		VdpDrawText(VDP_PLANEA_ADDR, ((MENU_LINE_CHARS_TOTAL - 2 *
				MENU_OSK_IP_COLS)>>1) + 2 * md->coord.col - 3,
				MENU_LINE_OSK_KEYS + 2 * md->coord.row, textColor, 1,
				(char*)&ip[i][md->coord.col]);
	}
}

/************************************************************************//**
 * Menu navigation through IP virtual keyboard.
 *
 * \param[in] input Pad button changes in the format returned by GpPressed(),
 *                  but inverted (not).
 ****************************************************************************/
void MenuOskIpActions(uint8_t input) {
	uint8_t limit;

	if (input & GP_A_MASK) {
		MenuOskKeyPress();
	} else if (input & GP_B_MASK) {
		// Delete current character
		MenuOskKeyDel();
	} else if (input & GP_C_MASK) {
		// Nothing to do for IP keyboards.
	} else if (input & GP_START_MASK) {
		/// \todo check IP is valid before accepting
		MenuOskDone();
	} else if (input & GP_UP_MASK) {
		// Draw current key with not selected color
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM);
		// Decrement row and draw key as selected. Note that if we are on last
		// row, the limit changes
		limit = md->coord.col < MENU_OSK_IP_COLS?MENU_OSK_IP_ROWS:
			MENU_OSK_NUM_FUNCS;
			md->coord.row = md->coord.row?md->coord.row - 1:limit - 1;
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_DOWN_MASK) {
		// Draw current key with not selected color
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM);
		// Increment row and draw key as selected.
		limit = md->coord.col < MENU_OSK_IP_COLS?MENU_OSK_IP_ROWS:
			MENU_OSK_NUM_FUNCS;
		md->coord.row = md->coord.row < limit - 1?md->coord.row + 1:0;
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_LEFT_MASK) {
		// Draw current key with not selected color
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM);
		// Decrement col if not on the last column
		if (md->coord.row < MENU_OSK_IP_ROWS) {
			md->coord.col = md->coord.col?md->coord.col - 1:MENU_OSK_IP_COLS;
		}
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_RIGHT_MASK) {
		// Draw current key with not selected color
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM);
		// Increment row if not on last line
		if (md->coord.row < MENU_OSK_IP_ROWS) {
			md->coord.col = md->coord.col == MENU_OSK_IP_COLS?0:
				md->coord.col + 1;
		}
		MenuOskIpDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	}
}

/************************************************************************//**
 * Draw current item for MENU_TYPE_OSK_NUMERIC menus, with specified color.
 *
 * \param[in] txtColor Color used to draw the item.
 ****************************************************************************/
void MenuOskNumDrawCurrent(uint8_t textColor) {
	uint8_t i;

	// Determine if we have to draw a special key
	if (md->coord.col >= MENU_OSK_NUM_COLS) {
		i = md->coord.row;
		VdpDrawText(VDP_PLANEA_ADDR, 2 * MENU_OSK_QWERTY_COLS + 6 + 2,
				MENU_LINE_OSK_KEYS + 2 * i, textColor, 4,
				(char*)qwertyFunc[i]);
	} else {
		// Draw normal character
		i = md->coord.row;
		VdpDrawText(VDP_PLANEA_ADDR, ((MENU_LINE_CHARS_TOTAL - 2 *
				MENU_OSK_NUM_COLS)>>1) + 2 * md->coord.col - 3,
				MENU_LINE_OSK_KEYS + 2 * md->coord.row, textColor, 1,
				(char*)&num[i][md->coord.col]);
	}
}

/************************************************************************//**
 * Parses actions for the numeric virtual keyboard.
 *
 * \param[in] input Key press changes, as obtained by GpPressed() function.
 ****************************************************************************/
void MenuOskNumActions(uint8_t input) {
	uint8_t limit;

	if (input & GP_A_MASK) {
		MenuOskKeyPress();
	} else if (input & GP_B_MASK) {
		// Delete current character
		MenuOskKeyDel();
	} else if (input & GP_C_MASK) {
		// Nothing to do for numeric keyboards.
	} else if (input & GP_START_MASK) {
		/// \todo check number is in range before accepting
		MenuOskDone();
	} else if (input & GP_UP_MASK) {
		// Draw current key with not selected color
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM);
		// Decrement row and draw key as selected. Note that if we are on last
		// row, the limit changes
		limit = md->coord.col < MENU_OSK_NUM_COLS?MENU_OSK_NUM_ROWS:
			MENU_OSK_NUM_FUNCS;
		if (md->coord.row) {
			md->coord.row = md->coord.row - 1;
		} else {
			md->coord.row = limit - 1;
			if (md->coord.col < MENU_OSK_NUM_COLS) md->coord.col = 1;
		}
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_DOWN_MASK) {
		// Draw current key with not selected color
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM);
		// Increment row and draw key as selected.
		if (md->coord.col < MENU_OSK_NUM_COLS) {
			if (md->coord.row < (MENU_OSK_NUM_ROWS - 1)) {
				md->coord.row++;
				if (md->coord.row >= (MENU_OSK_NUM_ROWS - 1))
					md->coord.col = 1;
			} else md->coord.row = 0;
		} else {
			if (md->coord.row < (MENU_OSK_NUM_FUNCS - 1))
					md->coord.row++;
			else md->coord.row = 0;
		}
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_LEFT_MASK) {
		// Draw current key with not selected color
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM);
		// Decrement col if not on the last column
		if (md->coord.col < MENU_OSK_NUM_COLS) {
			// We are on numeric keys
			if ((!md->coord.col) ||
					(md->coord.row >= (MENU_OSK_NUM_ROWS - 1)))
				md->coord.col = MENU_OSK_NUM_COLS;
			else md->coord.col--;
		} else {
			// We are on special keys
			if (md->coord.row < MENU_OSK_NUM_ROWS) {
				if (md->coord.row < (MENU_OSK_NUM_ROWS - 1))
					md->coord.col--;
				else md->coord.col = 1;
			}
		}
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	} else if (input & GP_RIGHT_MASK) {
		// Draw current key with not selected color
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM);
		// Increment row if not on last line
		if (md->coord.col < MENU_OSK_NUM_COLS) {
			// We are on numeric keys
			if ((md->coord.col < MENU_OSK_NUM_COLS) &&
					(md->coord.row < MENU_OSK_NUM_ROWS - 1))
				md->coord.col++;
			else md->coord.col = MENU_OSK_NUM_COLS;
		} else {
			// We are on special keys
			if (md->coord.row < MENU_OSK_NUM_ROWS) {
				if (md->coord.row < (MENU_OSK_NUM_ROWS - 1))
					md->coord.col = 0;
				else md->coord.col = 1;
			}
		}
		MenuOskNumDrawCurrent(MENU_COLOR_ITEM_SEL);
		// Draw on the edited item the newly selected character
		MenuOskDrawEditKey(0, MENU_COLOR_ITEM_SEL);
	}
}

/************************************************************************//**
 * Obtains the changes of buttons pressed as input, and performs the
 * corresponding actions depending on the button press (item change, menu
 * change, callback execution, etc.).
 *
 * \param[in] input Key press changes, as obtained by GpPressed() function.
 ****************************************************************************/
void MenuButtonAction(uint8_t input) {
	const MenuEntry *m = &md->me->mEntry;

	input = ~input;
	// Parse button presses depending on current menu type
	switch (m->type) {
		case MENU_TYPE_ITEM:
			MenuItemAction(input);
			break;

		case MENU_TYPE_OSK_QWERTY:
			MenuOskQwertyActions(input);
			break;

		case MENU_TYPE_OSK_NUMERIC:
			MenuOskNumActions(input);
			break;

		case MENU_TYPE_OSK_IPV4:
			MenuOskIpActions(input);
			break;
	}
}

/************************************************************************//**
 * Draw a message "box" over the current menu. The function allows choosing
 * if the message is kept during a fixed amount of frames, or if it is
 * indefinitely kept.
 *
 * \param[out] str    String to print on the message "box"
 * \param[in]  frames Number of frames to keep the message box. If set to 0,
 *                    the message will be kept until manually cleared.
 ****************************************************************************/
void MenuMessage(MenuString str, uint16_t frames) {
	MenuClearLines(11, 13, 0);
	VdpDrawText(VDP_PLANEA_ADDR, MenuStrAlign(str, MENU_H_ALIGN_CENTER, 0),
			12, MENU_COLOR_ITEM_ALT, str.length, str.string);
	if (frames) {
		VdpFramesWait(120);
		MenuDrawItemPage(0);
	}
}

/************************************************************************//**
 * Copy a MenuString.
 *
 * \param[out] dst Destination MenuString.
 * \param[in]  src Source string.
 ****************************************************************************/
void MenuStringCopy(MenuString *dst, const MenuString *src) {
	memcpy(dst->string, src->string, src->length);
	dst->length = src->length;
	dst->string[dst->length] = '\0';
}

/************************************************************************//**
 * Copy a null terminated character string. Allows to specify a maximum
 * length for the copy, and returns the number of characters copied.
 *
 * \param[out] dst    Destination string.
 * \param[in]  src    Source string.
 * \param[in]  maxLen Maximum length to copy.
 *
 * \return Number of characters copied (not including the null termination).
 * \warning If maxLen characters are copied before reaching the null
 * termination, copied dst string will not be null terminated.
 ****************************************************************************/
uint16_t MenuStrCpy(char dst[], const char src[], uint16_t maxLen) {
	uint16_t i;

	// If maxLen is 0, no maxLen specified, so use the maximum possible value
	if (!maxLen) maxLen--;

	for (i = 0; (i < maxLen) && (src[i] != '\0'); i++)
		dst[i] = src[i];

	if (i < maxLen) dst[i] = '\0';

	return i;
}

/************************************************************************//**
 * Evaluates if a string points to a number that can be stored in a
 * uint8_t type variable.
 *
 * \return The pointer to the character following the last digit of the
 *         number, if the string represents a number fittint in a uint_8.
 *         NULL if the string does not represent an uint8_t number.
 ****************************************************************************/
char *MenuNumIsU8(char num[]) {
	uint8_t i;

	// Skip leading zeros
	while (*num == '0') num++;
	// Determine number length (up to 4 characters)
	for (i = 0; (i < 4) && (num[i] >= '0') && (num[i] <= '9'); i++);
	
	switch (i) {
		// If number is 3 characters, the number fits in 8 bits only if
		// lower than 255
		case 3:
			if ((num[0] > '2') || ((num[0] == '2') && ((num[1] > '5') ||
						((num[1] == '5') && (num[2] > '5')))))
				return NULL;

		// If length is 2 or 1 characters, the number fits in 8 bits.
		// fallthrough
		case 2:
		case 1:
			return num + i;

		// If length is 4 or more, number does not fit in 8 bits.
		default:
			return NULL;
	}
}

/************************************************************************//**
 * IPv4 validate function. This function evaluates the data entered on the
 * input Menu structure, to guess if it corresponds to a valid IPv4.
 *
 * \param[in] md Pointer to the Menu structure containing the entered string
 *               to be evaluated.
 *
 * \return TRUE if the evaluated string corresponds to a valid IPv4. False
 *         otherwise.
 *
 * \note The intended use of this function is to be an exit callback for
 * MENU_TYPE_OSK_IPV4 menu entries, to force a valid IP address to be
 * entered in these menus.
 ****************************************************************************/
int MenuIpValidate(void *m) {
	Menu *md = (Menu*)m;
	char *str = md->strBuf;
	int8_t i;

	// Evaluate if we have 4 numbers fitting in a byte, separated by '.'
	if (!(str = MenuNumIsU8(md->strBuf))) return FALSE;

	for (i = 2; i >= 0; i--) {
		if (*str != '.') return FALSE;
		str++;
		if (!(str = MenuNumIsU8(str))) return FALSE;
	}

	if (*str != '\0') return FALSE;	
	return TRUE;
}
