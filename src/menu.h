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
 * \author	 Jesús Alonso (doragasu)
 * \date	 2017
 * \defgroup menu menu
 * \{
 ****************************************************************************/
#ifndef _MENU_H_
#define _MENU_H_

#include <stdint.h>
#include "vdp.h"

/// Reason causing the execution of a menu callback function.
typedef enum {
	MENU_CB_REASON_ENTRY = 0,	///< Menu entry
	MENU_CB_REASON_EXIT,		///< Menu exit
	MENU_CB_REASON_ITEM			///< Item selected
} MenuCbReason;


/************************************************************************//**
 * Callback function definition, used on several menu actions.
 *
 * \param[inout] md Pointer to the MenuData entry with menu information
 *
 * \return For menu exit and menu item callbacks, the return value is used
 * to validate the menu transition. Return TRUE to allow the menu change, or
 * FALSE to prevent it. For other callbacks, the return value is ignored.
 ****************************************************************************/
typedef int(*MenuCb)(void* md);

#ifndef MENU_NLEVELS
/// Number of nested menu levels. Can be overriden by application by
/// defining this constant in the makefile.
/// \warning Setting this to a value lower than needed will cause mass
/// destruction!
#define MENU_NLEVELS	10
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
#define MENU_COLOR_CONTEXT_L	VDP_TXT_COL_CYAN
/// Right context string line color
#define MENU_COLOR_CONTEXT_R	VDP_TXT_COL_MAGENTA
/// Color definition for the pager field
#define MENU_COLOR_PAGER		VDP_TXT_COL_CYAN
/// Unselected item color
#define MENU_COLOR_ITEM			VDP_TXT_COL_WHITE
/// Unselected item, alternate color
#define MENU_COLOR_ITEM_ALT		VDP_TXT_COL_CYAN
/// Selected item color
#define MENU_COLOR_ITEM_SEL		VDP_TXT_COL_MAGENTA
/// Color to draw the field name on the on screen keyboard menus
#define MENU_COLOR_OSK_FIELD	VDP_TXT_COL_WHITE
/// Color to draw the field name on the on screen keyboard menus
#define MENU_COLOR_OSK_DATA		VDP_TXT_COL_CYAN
/** \} */

/// Maximum string length
#define MENU_STR_MAX_LEN	32

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

/// Macro to help filling MenuString structures. The string is set as not
/// editable, not empty.
#define MENU_STR(str)	{				\
	.string = (char*)(str),				\
	.length = sizeof(str) - 1,			\
	.editable = FALSE,					\
	.empty = FALSE,						\
}

/// Macro to help filling MenuString structures. The string is set as
/// editable, not empty (empty string).
#define MENU_ESTR(str, maxLen)	{		\
	.string = (char*)(str),				\
	.length = maxLen,					\
	.editable = TRUE,					\
	.empty = FALSE,						\
}

/// Macro to help filling MenuString structures. The string is set as
/// editable, empty (empty editable string).
#define MENU_EESTR(maxLen)	{			\
	.string = NULL,						\
	.length = 0,						\
	.editable = TRUE,					\
	.empty = TRUE,						\
}

/// String definition for menus, including its properties. Other than its
/// length, MenuStrings can have several properties:
/// - Editable: String can be edited. When loading an editable string
///   from a menu, its contents are copied to RAM up to length bytes. If
///   not editable, string is maintained in ROM.
/// - Empty: This property is only used with editable strings. If true, no
//    contents from the string are copied to RAM, only memory up to length is
//    reserved.
typedef struct {
	char *string;
	uint8_t length;
	union {
		uint8_t flags;
		struct {
			uint8_t editable:1;	// String can be edited if TRUE
			uint8_t empty:1;	// String is empty if TRUE
		};
	};
} MenuString;

/// Menu Item.
/// \note All menu items from a single menu entry, must be declared
/// in a single MenuItem array.
typedef struct {
	MenuString caption;			///< Menu item text (editable)
	void *next;					///< Next MenuEntry (if item accepted)
	MenuCb cb;					///< Callback to run if option chosen
	union {
		struct {
			uint8_t selectable:1;	///< Selectable item
			uint8_t alt_color:1;	///< Print with alternate color
			uint8_t hide:1;			///< Hide text
//			uint8_t edit_off:5;		///< Offset to start editing text
		};
		uint8_t flags;			///< Menu item flags
	};
} MenuItem;

/// Private items for item list Menu Entres.
typedef struct {
	MenuItem *item;			///< Pointer to Menu Item array
	int8_t nItems;			///< Number of menu entries
	uint8_t spacing;		///< Line spacing between options
	uint8_t entPerPage;		///< Maximum entries per page
	uint8_t pages;			///< Number of entry pages minus 1
	struct {
		MenuHAlign align:2;	///< Alignment for items
	};
} MenuItemEntry;


/// Private items for On Screen Keyboard Menu Entries
typedef struct {
	MenuString fieldName;	///< Field name
	MenuString fieldData;	///< Editable field data
	uint8_t maxLen;			///< Maximum length of fieldData string
	uint8_t lineLen;		///< Maximum line length
} MenuOskEntry;

/// Menu entry, supporting the different menu entry types that can be used.
/// Entries are defined as const to be on ROM, and copied to a MenuEntity
/// structure before displaying the menu entry data.
typedef struct {
	uint8_t type;				///< Menu type
	uint8_t margin;				///< Margin
	MenuString title;			///< Menu title
	MenuString lContext;		///< Left context string (bottom line)
	MenuCb entry;				///< Callback for menu entry
	MenuCb exit;				///< Callback for menu exit
	MenuCb cBut;				///< C button callback
	union {
		MenuItemEntry mItem;	///< Item list entries
		MenuOskEntry keyb;		///< On screen keyboard entries
	};
} MenuEntry;

/// Menu entity, contains the menu live data (that will be directly displayed
/// on screen.
typedef struct menuEntity {
	MenuEntry mEntry;			///< Menu entry copy (on RAM)
	struct menuEntity *prev;	///< Menu entity of the previous menu
	uint8_t selItem;			///< Selected item in the menu
	uint8_t selPage;			///< Selected menu page
} MenuEntity;

/// Coordinates of selected keyboard item
typedef struct {
	uint8_t caps:1;		///< Set to 1 to toggle caps key
	uint8_t row:3;		///< Keyboard row
	uint8_t col:4;		///< Keyboard column
} MenuOskCoord;

/// Dynamic data structure needed to display the menus
typedef struct {
	/// Menu entry for the root menu
//	MenuEntity *root;
	/// Menu entry for the currently displayed menu
	MenuEntity *me;
	MenuString rContext;		///< Right context string (bottom line)
	/// Reserve space for the rContext string
	char rConStr[MENU_LINE_CHARS_TOTAL];
	/// Coordinates of selected keyboard item
	MenuOskCoord coord;
	/// Temporal string for data entry
	MenuString str;
	char strBuf[MENU_STR_MAX_LEN + 1];
} Menu;

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
 * Obtains the changes of buttons pressed as input, and performs the
 * corresponding actions depending on the button press (item change, menu
 * change, callback execution, etc.).
 *
 * \param[in] input Key press changes, as obtained by GpPressed() function.
 ****************************************************************************/
void MenuButtonAction(uint8_t input);

/************************************************************************//**
 * Forces loading specified menu entry.
 *
 * \param[in] me    Pointer to MenuEntry to load.
 * \param[in] level Menu level to load the new entry in. If set to 0, the
 *                  level next to the current menu level will be used.
 *                  Otherwise the specified level will be set.
 *
 * \warning When setting a menu level (other than 0), the caller must make
 *          sure that all previous levels are correctly populated and
 *          handled.
 ****************************************************************************/
void MenuForceLoad(MenuEntry *me, uint8_t level);

void MenuUnlink(void);

/************************************************************************//**
 * Goes back one menu level, unless in the root menu.
 ****************************************************************************/
void MenuBack(void);

/************************************************************************//**
 * Draw a message "box" over the current menu. The function allows choosing
 * if the message is kept during a fixed amount of frames, or if it is
 * indefinitely kept.
 *
 * \param[out] str    String to print on the message "box"
 * \param[in]  frames Number of frames to keep the message box. If set to 0,
 *                    the message will be kept until manually cleared.
 ****************************************************************************/
void MenuMessage(MenuString str, uint16_t frames);

/************************************************************************//**
 * Copy a MenuString.
 *
 * \param[out] dst Destination MenuString.
 * \param[in]  src Source string.
 ****************************************************************************/
void MenuStringCopy(MenuString *dst, const MenuString *src);

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
uint16_t MenuStrCpy(char dst[], const char src[], uint16_t maxLen);

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
int MenuIpValidate(void *md);

/************************************************************************//**
 * Sets background to red, writes a panic message and loops forever.
 *
 * \param[in] errStr String with the error that causes the panic status
 * \param[in] len    Length in characters of errStr string.
 ****************************************************************************/
void MenuPanic(char errStr[], uint8_t len);

/************************************************************************//**
 * Clears requested lines of the screen. Each line is 40 characters wide.
 *
 * \param[in] first  First line to clear.
 * \param[in] last   Last line to clear.
 * \param[in] offset Horizontal plane offset of the screen.
 ****************************************************************************/
void MenuClearLines(uint8_t first, uint8_t last, uint8_t offset);

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
uint8_t MenuStrAlign(MenuString mStr, MenuHAlign align, uint8_t margin);

/************************************************************************//**
 * Draw requested item page.
 *
 * \param[in] chrOff Character offset in plane to draw menu
 ****************************************************************************/
void MenuDrawItemPage(uint8_t chrOff);

#endif /*_MENU_H_*/
/** \} */

