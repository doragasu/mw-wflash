/*********************************************************************++*//**
 * \brief WFlash command definitions
 *
 * \author Jesús Alonso (doragasu)
 * \date 2017
 * \addtogroup WFlash
 * \{
 ****************************************************************************/
#ifndef _CMDS_H_
#define _CMDS_H_

#include <stdint.h> 
/// Major number of the commands implementation
#define WF_VERSION_MAJOR	0x00
/// Minor number of the commands implementation
#define WF_VERSION_MINOR	0x01

/// Maximum payload length
//#define WF_MAX_DATALEN	1152
#define WF_MAX_DATALEN	1440
/// Header lenght
#define WF_HEADLEN		4
/// MegaWiFi channel
#define WF_CHANNEL		1

/// Available commands
enum {
	WF_CMD_VERSION_GET = 0,		///< Get bootloader version
	WF_CMD_ECHO,				///< Echo data
	WF_CMD_ID_GET,				///< Get flash chip ID
	WF_CMD_ERASE,				///< Erase range
	WF_CMD_PROGRAM,				///< Program data
	WF_CMD_READ,				///< Read data
	WF_CMD_RUN,					///< Run from address
	WF_CMD_AUTORUN,				///< Run from entry point in cart header
	WF_CMD_MAX					///< Maximum command value delimiter
};

/// OK reply code to a command
#define WF_CMD_OK			0
/// ERROR reply code to a command
#define WF_CMD_ERROR		1

/// Memory range definition
typedef struct {
	uint32_t addr;	///< Start address of the range
	uint32_t len;	///< Length of the memory range
} WfMemRange;

/// Command definition
typedef struct {
	uint16_t cmd;	///< Command code
	uint16_t len;	///< Command length
	union {
		/// 8-bit data
		uint8_t data[WF_MAX_DATALEN - 4];
		/// 16-bit data
		uint16_t wdata [(WF_MAX_DATALEN - 4) / 2];
		/// 32-bit data
		uint32_t dwdata [(WF_MAX_DATALEN - 4) / 4];
		/// Memory range
		WfMemRange mem;
	};
} WfCmd;

/// Command buffer. Allows accessing the buffer as a command or as raw data.
typedef union {
	uint8_t data[WF_MAX_DATALEN];			///< 8-bit data
	uint16_t wdata[WF_MAX_DATALEN / 2];		///< 16-bit data
	uint32_t dwdata[WF_MAX_DATALEN / 4];	///< 32-bit data
	WfCmd cmd;								///< Command
} WfBuf;

#endif /*_CMDS_H_*/

/** \} */

