/// Screen related definitions
#include "../vdp.h"

/** \addtogroup MenuGeometry Dimension related definitions
 *  \note On the menu context, a line is not a "raster" line, but a character
 *  line (typically 1 line per 8 raster lines).
 *  \note X coordinate grows from 0 to right, while Y coordinate grows
 *  from 0 to the bottom of the screen.
 *  \{ */
/// Total number of lines on screen
#define MENU_LINES			(VDP_SCREEN_HEIGHT_PX/8)
/// Total number of drawable characters per line
#define MENU_LINE_CHARS			(VDP_SCREEN_WIDTH_PX/8)
/// First line on top of the screen
#define MENU_LINE_FIRST			0
/// Last line on bottom of the screen
#define MENU_LINE_LAST			(MENU_LINES - 1)
/// Line in which title will be displayed
#define MENU_LINE_TITLE			1
/// Line in which to draw context strings
#define MENU_LINE_CONTEXT		(MENU_LINE_LAST - 1)
/// Line in which to draw menu pages information
#define MENU_LINE_PAGER			(MENU_LINE_CONTEXT - 1)
/// First usable line to draw menu items
#define MENU_LINE_ITEM_FIRST		(MENU_LINE_TITLE + 3)
/// Last usable line to draw menu items
#define MENU_LINE_ITEM_LAST		(MENU_LINE_PAGER - 2)
/// Number of lines available to draw menu items
#define MENU_ITEM_LINES			(MENU_LINE_ITEM_LAST-MENU_LINE_ITEM_FIRST+1)
/// Line to draw the field name on the on screen keyboard menus
#define MENU_LINE_OSK_FIELD		(MENU_LINE_TITLE + 5)
/// Line to draw the field data on the on screen keyboard menus
#define MENU_LINE_OSK_DATA		(MENU_LINE_OSK_FIELD + 3)
/// Line to draw the upper line of the on screen keyboard
#define MENU_LINE_OSK_KEYS		(MENU_LINE_OSK_DATA + 6)
/// Default left margin
#define MENU_DEF_LEFT_MARGIN	1
/// Default right margin
#define MENU_DEF_RIGHT_MARGIN	1
/** \} */

/** \addtogroup MenuColor Color definitions for the menu
 *  \{ */
/// Title text color
#define MENU_COLOR_TITLE		VDP_TXT_COL_CYAN
/// Left context string line color
#define MENU_COLOR_CONTEXT_L		VDP_TXT_COL_CYAN
/// Right context string line color
#define MENU_COLOR_CONTEXT_R		VDP_TXT_COL_MAGENTA
/// Color definition for the pager field
#define MENU_COLOR_PAGER		VDP_TXT_COL_CYAN
/// Unselected item color
#define MENU_COLOR_ITEM			VDP_TXT_COL_WHITE
/// Unselected item, alternate color
#define MENU_COLOR_ITEM_ALT		VDP_TXT_COL_CYAN
/// Selected item color
#define MENU_COLOR_ITEM_SEL		VDP_TXT_COL_MAGENTA
/// Color to draw the field name on the on screen keyboard menus
#define MENU_COLOR_OSK_FIELD		VDP_TXT_COL_WHITE
/// Color to draw the field name on the on screen keyboard menus
#define MENU_COLOR_OSK_DATA		VDP_TXT_COL_CYAN
/** \} */

/// Maximum string length
#define MENU_STR_MAX_LEN	38

/// Number of characters horizontally separating each menu
#define MENU_SEPARATION_CHR		(VDP_SCREEN_WIDTH_PX/8)

/// Screen buffer location for string drawing. Not to be mistaken with
/// alignment (menu_h_align).
enum menu_placement {
	/// [88 ~ 127]
	MENU_PLACE_LEFT = VDP_PLANE_HTILES - MENU_SEPARATION_CHR,
	/// [0 ~ 39]
	MENU_PLACE_CENTER = 0,
	/// [40 ~ 79]
	MENU_PLACE_RIGHT = MENU_SEPARATION_CHR
};

