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

/// Default values for VDP registers
const static uint8_t vdpRegDefaults[19] = {
	// Mode 1:
	// - No 8 left pixels blank
	// - No HINT
	0x04,
	// Mode 2:
	// - Display disabled
	// - VBlank int disabled
	// - DMA disabled
	// - 224 lines
	// - Megadrive mode
	0x04,
	// Name table for PLANE A
	VDP_PLANEA_ADDR>>10,
	// Name table for WINDOW
	VDP_WIN_ADDR>>10,
	// Name table for PLANE B
	VDP_PLANEB_ADDR>>13,
	// Empty sprite attribute table
	0x00,
	// Empty sprite pattern generator
	0x00,
	// Background color palette 0, color 0
	0x00,
	// Unused
	0x00,
	// Unused
	0x00,
	// H-Interrupt register
	0xFF,
	// Mode 3:
	// - External interrupt disable
	// - No vertical tile scroll
	// - Plane horizontal scroll
	0x00,
	// Mode 4:
	// - 40 tiles per line
	// - VSync signal
	// - HSync signal
	// - Standard color data generation
	// - No shadow/highlight mode
	// - No interlace
	0x81,
	// Horizontal scroll data address
	VDP_HSCROLL_ADDR>>10,
	// Nametable pattern generator
	0x00,
	// Set auto-increment to 2
	0x02,
	// Set plane sizes to 128x32 cells
	0x03,
	// Window H and V positions
	0x00, 0x00
};

/************************************************************************//**
 * Write to an VDP register, updating the register shadow value.
 *
 * \param[in] reg   Register number (using VdpReg enumerate recommended).
 * \param[in] value Value to write to the VDP register.
 ****************************************************************************/
static inline void VdpRegWrite(uint8_t reg, uint8_t value) {
	vdpRegShadow[reg] = value;
	VDP_CTRL_PORT_W = 0x8000 | (reg<<8) | value;
}

/************************************************************************//**
 * VDP Initialization. Call this function once before using this module.
 ****************************************************************************/
void VdpInit(void) {
	uint8_t i;

	for (i = 0; i < sizeof(vdpRegDefaults); i++)
		VdpRegWrite(i, vdpRegDefaults[i]);

	// Clear VRAM
	/// \bug I do not know why, but VRam Fill does not work. Maybe you
	/// have to wait until configuration is somehow applied.
	VdpDmaVRamFill(0, 0, 0);
	VdpDmaWait();

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

	// Set auto-increment to 2
	VdpRegWrite(VDP_REG_INCR, 0x02);
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

	// Set auto increment
	VdpRegWrite(VDP_REG_INCR, 0x02);
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

	// Set auto increment
	VdpRegWrite(VDP_REG_INCR, 0x02);
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

	// Set auto increment
	VdpRegWrite(VDP_REG_INCR, 0x02);
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

/************************************************************************//**
 * Fills specified VRAM region.
 *
 * \param[in] dst  Start VRAM memory address to fill.
 * \param[in] len  Length in bytes of the VRAM zone to fill.
 * \param[in] fill Byte to write to the filled zone.
 *
 * \warning This function starts the DMA fill but does not wait for the fill
 * operation to complete. During fill operation, only VDP status register,
 * H/V counter and PSG registers can be accessed. If you need to access any
 * other VDP register, call VdpDmaWait() first.
 ****************************************************************************/
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
}

/************************************************************************//**
 * Copies from VRAM to VRAM a specified region.
 *
 * \param[in] src Start VRAM memory address to copy.
 * \param[in] dst Destination VRAM address of the copy operation.
 * \param[in] len Length in bytes of the zone to copy.
 *
 * \warning This function starts the DMA copy but does not wait for the copy
 * operation to complete. During copy operation, only VDP status register,
 * H/V counter and PSG registers can be accessed. If you need to access any
 * other VDP register, call VdpDmaWait() first.
 ****************************************************************************/
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
}

/************************************************************************//**
 * Clears (sets to 0) the plane line.
 *
 * \param[in] planeAddr Address in VRAM of the plane to clear.
 * \param[in] line      Line number to clear.
 *
 * \warning This function starts the DMA fill but does not wait for the fill
 * operation to complete. During fill operation, only VDP status register,
 * H/V counter and PSG registers can be accessed. If you need to access any
 * other VDP register, call VdpDmaWait() first.
 ****************************************************************************/
void VdpLineClear(uint16_t planeAddr, uint8_t line) {
	uint16_t start;

	// Calculate nametable offset and prepare VRAM writes
	start = planeAddr + 2 * (line * VDP_PLANE_HTILES);

	VdpDmaVRamFill(start, 32 * 2, 0);
}

/************************************************************************//**
 * Waits until the beginning of the next VBLANK cycle.
 ****************************************************************************/
void VdpVBlankWait(void) {
	while ((VDP_CTRL_PORT_W & VDP_STAT_VBLANK));
	while ((VDP_CTRL_PORT_W & VDP_STAT_VBLANK) == 0);
}

