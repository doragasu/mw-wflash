/************************************************************************//**
 * \brief Basic VDP handling routines. This module implements basic VDP
 * related routines for:
 * - VDP initialization.
 * - Font loading and colour text drawing on planes. 
 * No sprites or any other fancy stuff.
 *
 * \author Jesús Alonso (@doragasu)
 * \date 2017
 ****************************************************************************/
#include "vdp.h"
#include "font.h"
#include "util.h"

/// VDP shadow register values.
static uint8_t vdpRegShadow[VDP_REG_MAX];

/// Mask used to build control port data for VDP RAM writes.
/// Bits 15 and 14: CD1 and CD0.
/// Bits 7 to 4: CD5 to CD2.
const uint16_t cdMask[VDP_RAM_TYPE_MAX] = {
	0x0000,	// VRAM read
	0x4000,	// VRAM write
	0x0020,	// CRAM read
	0xC000,	// CRAM write
	0x0010,	// VSRAM read
	0x4010,	// VSRAM write
	0x0030,	// VRAM read, 8-bit (undocumented)
};

/************************************************************************//**
 * Write to an VDP register, updating the register shadow value.
 *
 * \param[in] reg   Register number (using VdpReg enumerate recommended).
 * \param[in] value Value to write to the VDP register.
 ****************************************************************************/
static inline void VdpRegWrite(uint8_t reg, uint8_t value) {
	if (reg >= VDP_REG_MAX) return;
	vdpRegShadow[reg] = value;

	VDP_CTRL_PORT_W = 0x8000 | (reg<<8) | value;
}

/************************************************************************//**
 * VDP Initialization. Call this function once before using this module.
 ****************************************************************************/
void VdpInit(void) {
	// Mode 1:
	// - No 8 left pixels blank
	// - No HINT
	VdpRegWrite(VDP_REG_MODE1, 0x04);
	// Mode 2:
	// - Display disabled
	// - VBlank int disabled
	// - DMA disabled
	// - 224 lines
	// - Megadrive mode
	VdpRegWrite(VDP_REG_MODE2, 0x14);
	// Name table for PLANE A set to 0x2000
	VdpRegWrite(VDP_REG_PLANEA_NT, VDP_PLANEA_ADDR>>10);
	// Name table for WINDOW set to 0x6000
	VdpRegWrite(VDP_REG_WIN_NT, VDP_WIN_ADDR>>10);
	// Name table for PLANE B set to 0x4000
	VdpRegWrite(VDP_REG_PLANEB_NT, VDP_PLANEB_ADDR>>13);
	// Empty sprite attribute table
	VdpRegWrite(VDP_REG_SPR_T, 0x00);
	// Background color palette 0, color 0
	VdpRegWrite(VDP_REG_BGCOL, 0x00);
	// Mode 3:
	// - External interrupt disable
	// - No vertical tile scroll
	// - Plane horizontal scroll
	VdpRegWrite(VDP_REG_MODE3, 0x00);
	// Mode 4:
	// - 40 tiles per line
	// - VSync signal
	// - HSync signal
	// - Standard color data generation
	// - No shadow/highlight mode
	// - No interlace
	VdpRegWrite(VDP_REG_MODE4, 0x81);
	// Horizontal scroll data address
	VdpRegWrite(VDP_REG_HSCROLL, VDP_HSCROLL_ADDR>>10);
	// Set auto-increment to 2
	VdpRegWrite(VDP_REG_INCR, 0x02);
	// Set plane sizes to 128x32 cells
	VdpRegWrite(VDP_REG_PSIZE, 0x03);
	// Window H and V positions
	VdpRegWrite(VDP_REG_WIN_HPOS, 0x00);
	VdpRegWrite(VDP_REG_WIN_VPOS, 0x00);

	// Clear VRAM
	VdpVRamClear(0, 32768);

	// Load font three times, to be able to use three different colors
	VdpFontLoad(font, fontChars, 0, 1, 0);
	VdpFontLoad(font, fontChars, fontChars * 32, 2, 0);
	VdpFontLoad(font, fontChars, 2 * fontChars * 32, 3, 0);

	// Set background and font colors
	VdpRamRwPrep(VDP_CRAM_WR, 0);
	VDP_DATA_PORT_W = VDP_COLOR_BLACK;
	VDP_DATA_PORT_W = VDP_COLOR_WHITE;
	VDP_DATA_PORT_W = VDP_COLOR_CYAN;
	VDP_DATA_PORT_W = VDP_COLOR_MAGENTA;

	// Set  scroll to 0
	VDP_DATA_PORT_W = 0;
	VdpRamRwPrep(VDP_VSRAM_WR, 0);
	VDP_DATA_PORT_W = 0;
	VDP_DATA_PORT_W = 0;

	// Enable display
	VdpRegWrite(VDP_REG_MODE2, 0x54);
}

/************************************************************************//**
 * Draws text on a plane.
 *
 * \param[in] planeAddr Address in VRAM of the plane used to draw text.
 * \param[in] x         Horizontal text coordinate.
 * \param[in] y         Vertical text coordinate.
 * \param[in] txtColor  Text colour (see VdpTextColors).
 * \param[in] maxChars	Maximum number of characters to write
 * \param[in] text      Null terminated text to write to the plane.
 ****************************************************************************/
void VdpDrawText(uint16_t planeAddr, uint8_t x, uint8_t y, uint8_t txtColor,
		uint8_t maxChars, char text[]) {
	uint16_t offset;
	uint16_t i;

	// Calculate nametable offset and prepare VRAM writes
	offset = planeAddr + 2 *(x + y * VDP_PLANE_HTILES);
	VdpRamRwPrep(VDP_VRAM_WR, offset);

	for (i = 0; text[i] && (i < maxChars); i++) {
		VDP_DATA_PORT_W = text[i] - ' ' + txtColor;
	}
}

/************************************************************************//**
 * Draws an 8-bit hexadecimal number on a plane.
 *
 * \param[in] planeAddr Address in VRAM of the plane used to draw text.
 * \param[in] x         Horizontal text coordinate.
 * \param[in] y         Vertical text coordinate.
 * \param[in] txtColor  Text colour (see VdpTextColors).
 * \param[in] num       Number to draw on the plane in hexadecimal format.
 ****************************************************************************/
void VdpDrawHex(uint16_t planeAddr, uint8_t x, uint8_t y, uint8_t txtColor,
		uint8_t num) {
	uint16_t offset;
	uint8_t tmp;

	// Calculate nametable offset and prepare VRAM writes
	offset = planeAddr + 2 * (x + y * VDP_PLANE_HTILES);
	VdpRamRwPrep(VDP_VRAM_WR, offset);

	// Write hex byte
	tmp = num>>4;
	VDP_DATA_PORT_W = txtColor + (tmp > 9?tmp - 10 + 0x21:tmp + 0x10);
	tmp = num & 0xF;
	VDP_DATA_PORT_W = txtColor + (tmp > 9?tmp - 10 + 0x21:tmp + 0x10);
}

/************************************************************************//**
 * Clears (sets to 0) the specified VRAM range.
 *
 * \param[in] addr VRAM address to clear.
 * \param[in] wlen Length in words of the range to clear.
 ****************************************************************************/
void VdpVRamClear(uint16_t addr, uint16_t wlen) {
	uint16_t i;

	VdpRamRwPrep(VDP_VRAM_WR, addr);

	for (i = 0; i < wlen; i++) VDP_DATA_PORT_W = 0;
}

/************************************************************************//**
 * Draws an 8-bit decimal number on a plane.
 *
 * \param[in] planeAddr Address in VRAM of the plane used to draw text.
 * \param[in] x         Horizontal text coordinate.
 * \param[in] y         Vertical text coordinate.
 * \param[in] txtColor  Text colour (see VdpTextColors).
 * \param[in] num       Number to draw on the plane in hexadecimal format.
 *
 * \return Number of characters used by the drawn number.
 ****************************************************************************/
uint8_t VdpDrawDec(uint16_t planeAddr, uint8_t x, uint8_t y, uint8_t txtColor,
		uint8_t num) {
	uint16_t offset;
	uint8_t len, i;
	char str[4];

	// Calculate nametable offset and prepare VRAM writes
	offset = planeAddr + 2 * (x + y * VDP_PLANE_HTILES);
	VdpRamRwPrep(VDP_VRAM_WR, offset);

	len = Byte2UnsStr(num, str);
	for (i = 0; i < len; i++) VDP_DATA_PORT_W = txtColor + 0x10 - '0' + str[i];

	return i;
}

/************************************************************************//**
 * Loads a 1bpp font on the VRAM, setting specified foreground and
 * background colours.
 *
 * \param[in] font  Array containing the 1bpp font (8 bytes per character).
 * \param[in] chars Number of characters contained in font.
 * \param[in] addr  VRAM Address to load the font in.
 * \param[in] fgcol Foreground colour, in CRAM colour format.
 * \param[in] bgcol Background colour, in CRAM colour format.
 ****************************************************************************/
void VdpFontLoad(const uint32_t font[], uint8_t chars, uint16_t addr,
		uint8_t fgcol, uint8_t bgcol) {
	uint32_t line;
	uint32_t scratch;
	int16_t i;
    int8_t j, k;

	// Prepare write
	VdpRamRwPrep(VDP_VRAM_WR, addr);
	// Note font is 1bpp, and it expanded to 4bpp during load

	// Each char takes two DWORDs	
	for (i = 0; i < 2 * chars; i++) {
		line = font[i];
		// Process a 32-bit font input
		for (j = 0; j < 4; j++) {
			scratch = 0;
			// Process 8 bits to create 1 VDP DWORD
			for (k = 0; k < 8; k++) {
				scratch<<=4;
				scratch |= line & 1?fgcol:bgcol;
				line>>=1;
			}
			// Write 32-bit value to VDP
			VDP_DATA_PORT_DW = scratch;
		}
	}
}

void VdpDma(uint32_t src, uint16_t dst, uint16_t wLen, uint16_t mem) {
	uint32_t cmd;	// Command word

	// Write transfer length
	VdpRegWrite(VDP_REG_DMALEN1, wLen);
	VdpRegWrite(VDP_REG_DMALEN2, wLen>>8);
	// Write source
	VdpRegWrite(VDP_REG_DMASRC1, src);
	VdpRegWrite(VDP_REG_DMASRC2, src>>8);
	VdpRegWrite(VDP_REG_DMASRC3, src>>16);
	// Write command and start DMA
	cmd = (dst>>14) | (mem & 0xFF) | (((dst & 0x3FFF) | (mem & 0xFF00))<<16);
	VDP_CTRL_PORT_DW = cmd;
}

void VdpDmaVRamFill(uint16_t dst, uint16_t len, uint16_t fill) {
	uint32_t cmd;	// Command word

	// Set auto-increment to 1 byte
	VdpRegWrite(VDP_REG_INCR, 0x01);
	// Write transfer length
	VdpRegWrite(VDP_REG_DMALEN1, len);
	VdpRegWrite(VDP_REG_DMALEN2, len>>8);
	// Enable DMA fill
	VdpRegWrite(VDP_REG_DMASRC3, VDP_DMA_FILL);
	// Write destination address
	cmd = (dst>>14) | VDP_DMA_FILL | (((dst & 0x7FFF) | 0x4000)<<16);
	VDP_CTRL_PORT_DW = cmd;
	// Write fill data
	VDP_DATA_PORT_W = fill;
	// Restore auto-increment
	VdpRegWrite(VDP_REG_INCR, 0x02);
}

void VdpDmaVRamCopy(uint16_t src, uint16_t dst, uint16_t len) {
	uint32_t cmd;	// Command word

	// Set auto-increment to 1 byte
	VdpRegWrite(VDP_REG_INCR, 0x01);
	// Write transfer length
	VdpRegWrite(VDP_REG_DMALEN1, len);
	VdpRegWrite(VDP_REG_DMALEN2, len>>8);
	// Write source
	VdpRegWrite(VDP_REG_DMASRC1, src);
	VdpRegWrite(VDP_REG_DMASRC2, src>>8);
	VdpRegWrite(VDP_REG_DMASRC3, VDP_DMA_COPY);
	// Write destination and start transfer
	cmd = (dst>>14) | VDP_DMA_COPY | ((dst & 0x3FFF)<<16);
	VDP_CTRL_PORT_DW = cmd;
	// Restore auto-increment
	VdpRegWrite(VDP_REG_INCR, 0x02);
}

void VdpLineClear(uint16_t planeAddr, uint8_t line) {
	uint16_t start;
	int8_t i;

	// Calculate nametable offset and prepare VRAM writes
	start = planeAddr + 2 * (line * VDP_PLANE_HTILES);
	VdpRamRwPrep(VDP_VRAM_WR, start);

	// Clear 32 characters (30 should be enough but just in case)
	for (i = 31; i >= 0; i--) VDP_DATA_PORT_W = 0;
}

void VdpVBlankWait(void) {
	while ((VDP_CTRL_PORT_W & VDP_STAT_VBLANK));
	while ((VDP_CTRL_PORT_W & VDP_STAT_VBLANK) == 0);
}

