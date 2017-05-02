/************************************************************************//**
 * \brief Menu implementation. This menu allows to define menu data
 * structures, and alow allows to display them and handle options.
 *
 * To use this module, the program must define a menu structure with at the
 * very minimum a root MenuEntry, containing several MenuItem structures.
 *
 * The MenuEntry structure can partially automate menu navigation:
 * - UP/DOWN: Change selected option.
 * - LEFT/RIGHT: Change page (when there are several menu pages).
 * - A: Choses selected options.
 * - C: Goes back in the menu tree.
 *
 * When an option is chosen, depending on how the menu structure has been
 * defined, the following actions will be performed:
 * - MenuItem callback is called.
 * - Transition to the next MenuEntry is performed.
 *
 * When doing a MenuEntry transition, depending on how the menu structure
 * has been defined, the following actions will be performed:
 * - Exit callback of the exiting MenuEntry is executed.
 * - Entry callback of the entering MenuEntry is executed.
 *
 * \author	Jesús Alonso (doragasu)
 * \date	2017
 ****************************************************************************/
#ifndef _MENU_H_
#define _MENU_H_

#include <stdint.h>
#include "vdp.h"

/// Menu level, selected item number and pad status
typedef void(*MenuCb)(uint8_t level, uint8_t item, uint8_t padStatus);

#ifndef MENU_NLEVELS
/// Number of nested menu levels. Should be overriden by application
/// before including this file.
/// \warning Setting this to a value lower than needed will cause mass
/// destruction!
#define MENU_NLEVELS	4
#endif

/// Number of buttons supported
#define MENU_NBUTTONS		4

/// A button
#define MENU_BUTTON_A		0
/// B button
#define MENU_BUTTON_B		1
/// C button
#define MENU_BUTTON_C		2
/// START button
#define MENU_BUTTON_START	3
 
// Default actions if no menu callbacks provided:
// A, START: Accept selected option
// B, Nothing
// C, Back

/** \addtogroup MenuGeometry Dimension related definitions
 *  \note On the menu context, a line is not a "raster" line, but a character
 *  line (typically 1 line per 8 raster lines).
 *  \note X coordinate grows from 0 to right, while Y coordinate grows
 *  from 0 to the bottom of the screen.
 *  \{ */
/// Total number of lines on screen
#define MENU_NLINES_TOTAL		(VDP_SCREEN_HEIGHT_PX/8)
/// Total number of drawable characters per line
#define MENU_LINE_CHARS_TOTAL	(VDP_SCREEN_WIDTH_PX/8)
/// First line on top of the screen
#define MENU_LINE_FIRST			0
/// Last line on bottom of the screen
#define MENU_LINE_LAST			(MENU_NLINES_TOTAL - 1)
/// Line in which title will be displayed
#define MENU_LINE_TITLE			1
/// Line in which to draw context strings
#define MENU_LINE_CONTEXT		(MENU_LINE_LAST - 1)
/// Line in which to draw menu pages information
#define MENU_LINE_PAGER			(MENU_LINE_CONTEXT - 1)
/// First usable line to draw menu items
#define MENU_LINE_ITEM_FIRST	(MENU_LINE_TITLE + 3)
/// Last usable line to draw menu items
#define MENU_LINE_ITEM_LAST		(MENU_LINE_PAGER - 2)
/// Number of lines available to draw menu items
#define MENU_ITEM_NLINES		(MENU_LINE_ITEM_LAST-MENU_LINE_ITEM_FIRST+1)
/// Line to draw the field name on the on screen keyboard menus
#define MENU_LINE_OSK_FIELD		(MENU_LINE_TITLE + 3)
/// Line to draw the field data on the on screen keyboard menus
#define MENU_LINE_OSK_DATA		(MENU_LINE_OSK_CAPTION + 2)
/// Line to draw the upper line of the on screen keyboard
#define MENU_LINE_OSK_KEYBOARD	(MENU_LINE_OSK_DATA + 3)
/// Default margin
#define MENU_DEF_LEFT_MARGIN	1
/** \} */

/** \addtogroup MenuColor Color definitions for the menu
 *  \{ */
/// Title text color
#define MENU_COLOR_TITLE		VDP_TXT_COL_CYAN
/// Left context string line color
#define MENU_COLOR_CONTEXT_L	VDP_TXT_COL_CYAN
/// Right context string line color
#define MENU_COLOR_CONTEXT_R	VDP_TXT_COL_MAGENTA
/// Color definition for the pager field
#define MENU_COLOR_PAGER		VDP_TXT_COL_CYAN
/// Unselected item color
#define MENU_COLOR_ITEM			VDP_TXT_COL_WHITE
/// Selected item color
#define MENU_COLOR_ITEM_SEL		VDP_TXT_COL_MAGENTA
/// Color to draw the field name on the on screen keyboard menus
#define MENU_COLOR_OSK_FIELD	VDP_TXT_COL_MAGENTA
/** \} */

/// Supported alignment for menu items
typedef enum {
	MENU_H_ALIGN_CENTER = 0,	///< Center align (default)
	MENU_H_ALIGN_LEFT,			///< Left align
	MENU_H_ALIGN_RIGHT			///< Right align
} MenuHAlign;

/// Suported On Screen Keyboard types
typedef enum {
	MENU_TYPE_ITEM = 0,			///< Item list menu
	MENU_TYPE_OSK_QWERTY,		///< QWERTY-like keyboard
	MENU_TYPE_OSK_NUMERIC,		///< Numeric only keyboard
	MENU_TYPE_OSK_IPV4			///< IPv4 address entry keyboard
} MenuType;

/// Macro to help filling MenuString structures
#define MENU_STR(string)	{(char*)(string), sizeof(string) - 1}

/// String definition for menus, including its properties
typedef struct {
	char *string;
	uint8_t length;
} MenuString;

/// Menu Item.
/// \note All menu items from a single menu entry, must be declared
/// in a single MenuItem array.
typedef struct {
	MenuString caption;			///< Menu item text (editable)
	const void *next;			///< Next MenuEntry (if item accepted)
	const MenuCb cb;			///< Callback to run if option chosen
	struct {
		uint8_t selectable:1;	///< Selectable item
		uint8_t enabled:1;		///< Enabled item
	} flags;					///< Menu item flags
} MenuItem;

/// Private items for item list Menu Entres.
typedef struct {
	const MenuItem *item;		///< Pointer to Menu Item array
	const int8_t nItems;		///< Number of menu entries
	const uint8_t spacing;		///< Line spacing between options
	const uint8_t entPerPage;	///< Maximum entries per page
	const uint8_t pages;		///< Number of entry pages minus 1
	const struct {
		MenuHAlign align:2;		///< Alignment for items
	};
} MenuItemEntry;


/// Private items for On Screen Keyboard Menu Entries
typedef struct {
	const MenuString fieldName;	///< Field name
	MenuString fieldData;		///< Editable field data
	const uint8_t maxLen;		///< Maximum length of fieldData string
	const uint8_t lineLen;		///< Maximum line length
} MenuOskEntry;

/// Menu entry, supporting the different menu entry types that can be used.
typedef struct {
	const uint8_t type;			///< Menu type
	const uint8_t margin;		///< Margin
	const MenuString title;		///< Menu title
	const MenuString lContext;	///< Left context string (bottom line)
	const MenuCb entry;			///< Callback for menu entry
	const MenuCb exit;			///< Callback for menu exit
	union {
		MenuItemEntry item;		///< Item list entries
		MenuOskEntry keyb;		///< On screen keyboard entries
	};
} MenuEntry;

/************************************************************************//**
 * Module initialization. Call this function before using any other one from
 * this module. This function initialzes the menu subsystem and displays the
 * root menu.
 *
 * \param[in] root    Pointer to the root menu entry.
 * \param[in] statStr MenuString with the status text to display in the right
 *                     context string space.
 ****************************************************************************/
void MenuInit(const MenuEntry *root, MenuString statStr);

/************************************************************************//**
 * Sets the status text to display in the right context string space.
 *
 * \param[in] statStr MenuString with the status text to display in the right
 *                     context string space.
 ****************************************************************************/
void MenuStatStrSet(MenuString statStr);

/************************************************************************//**
 * Obtains the buttons pressed as input, and performs the corresponding
 * actions depending on the button press (item change, menu change, callback
 * execution, etc.).
 *
 * \param[in] input Menu actions, as obtained from a call to GpPressed().
 *
 * \todo Currently working only for MENU_TYPE_ITEM menus.
 ****************************************************************************/
void MenuButtonAction(uint8_t input);

void MenuForceLoad(MenuEntry *me);

#endif /*_MENU_H_*/

