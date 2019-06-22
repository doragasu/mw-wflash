/************************************************************************//**
 * \brief MeGaWiFi command message definitions. Contains the definition of
 *        the command codes and the data structures conforming the command
 *        message queries and responses.
 *
 * \author Jesus Alonso (doragasu)
 * \date 2015
 *
 * \defgroup MwMsg mwmsg
 * \{
 ****************************************************************************/
#ifndef _MW_MSG_H_
#define _MW_MSG_H_

#include <stdint.h>
#include "../util.h"

/// Maximum buffer length (bytes)
#define MW_MSG_MAX_BUFLEN	512

/// Command header length (command code and data length fields).
#define MW_CMD_HEADLEN		(2 * sizeof(uint16_t))

/// Maximum data length contained inside command buffer.
#define MW_CMD_MAX_BUFLEN	(MW_MSG_MAX_BUFLEN - MW_CMD_HEADLEN)

/// Maximum SSID length (including '\0').
#define MW_SSID_MAXLEN		32
/// Maximum password length (including '\0').
#define MW_PASS_MAXLEN		64

/// Supported commands.
enum PACKED mw_command {
	MW_CMD_OK		=   0,	///< OK command reply
	MW_CMD_VERSION      	=   1,	///< Get firmware version
	MW_CMD_ECHO		=   2,	///< Echo data
	MW_CMD_AP_SCAN		=   3,	///< Scan for access points
	MW_CMD_AP_CFG		=   4,	///< Configure access point
	MW_CMD_AP_CFG_GET   	=   5,	///< Get access point configuration
	MW_CMD_IP_CURRENT 	=   6,	///< Get current IPv4 configuration
	MW_CMD_RESERVED		=   7,
	MW_CMD_IP_CFG		=   8,	///< Configure IPv4
	MW_CMD_IP_CFG_GET	=   9,	///< Get IPv4 configuration
	MW_CMD_DEF_AP_CFG	=  10,	///< Set default AP configuration
	MW_CMD_DEF_AP_CFG_GET	=  11,	///< Get default AP configuration
	MW_CMD_AP_JOIN		=  12,	///< Join access point
	MW_CMD_AP_LEAVE		=  13,	///< Leave previously joined AP
	MW_CMD_TCP_CON		=  14,	///< Connect TCP socket
	MW_CMD_TCP_BIND		=  15,	///< Bind TCP socket to port
	MW_CMD_TCP_ACCEPT	=  16,	///< Accept incomint TCP connection
	MW_CMD_TCP_DISC		=  17,	///< Disconnect and free TCP socket
	MW_CMD_UDP_SET		=  18,	///< Configure UDP socket
	MW_CMD_UDP_CLR		=  19,	///< Clear and free UDP socket
	MW_CMD_SOCK_STAT	=  20,	///< Get socket status
	MW_CMD_PING		=  21,	///< Ping host
	MW_CMD_SNTP_CFG		=  22,	///< Configure SNTP service
	MW_CMD_SNTP_CFG_GET     =  23,  ///< Get SNTP configuration
	MW_CMD_DATETIME		=  24,	///< Get date and time
	MW_CMD_DT_SET       	=  25,	///< Set date and time
	MW_CMD_FLASH_WRITE	=  26,	///< Write to WiFi module flash
	MW_CMD_FLASH_READ	=  27,	///< Read from WiFi module flash
	MW_CMD_FLASH_ERASE	=  28,	///< Erase sector from WiFi flash
	MW_CMD_FLASH_ID 	=  29,	///< Get WiFi flash chip identifiers
	MW_CMD_SYS_STAT		=  30,	///< Get system status
	MW_CMD_DEF_CFG_SET	=  31,	///< Set default configuration
	MW_CMD_HRNG_GET		=  32,	///< Gets random numbers
	MW_CMD_ERROR		= 255	///< Error command reply
};

/// Supported security protocols
enum PACKED mw_security {
	MW_SEC_OPEN = 0,
	MW_SEC_WEP,
	MW_SEC_WPA_PSK,
	MW_SEC_WPA2_PSK,
	MW_SEC_WPA_WPA2_PSK,
	MW_SEC_UNKNOWN
};

/// IPv4 address
union ip_addr {
	uint32_t addr;
	uint8_t byte[4];
};

/// TCP/UDP address message
struct mw_msg_in_addr{
	char dst_port[6];	///< TCP destination port string
	char src_port[6];	///< TCP source port string
	uint8_t channel;	///< LSD channel used for communications
	/// Data payload
	char dst_addr[MW_CMD_MAX_BUFLEN - 6 - 6 - 1];
};

/// TCP/UDP address message
typedef struct {
	char dst_port[6];	///< TCP destination port string
	char src_port[6];	///< TCP source port string
	uint8_t channel;	///< LSD channel used for communications
	/// Data payload
	char dstAddr[MW_CMD_MAX_BUFLEN - 6 - 6 - 1];
} MwMsgInAddr;

/// IP configuration parameters
struct mw_ip_cfg {
	union ip_addr addr;	///< Host IP address in binary format
	union ip_addr mask;	///< Subnet mask in binary IP format
	union ip_addr gateway;	///< Gateway IP address in binary format
	union ip_addr dns1;	///< DNS server 1 IP address in binary format
	union ip_addr dns2;	///< DNS server 2 IP address in binary format
};

/// IP configuration parameters
typedef struct {
	uint32_t addr;		///< Host IP address in binary format
	uint32_t mask;		///< Subnet mask in binary IP format
	uint32_t gateway;	///< Gateway IP address in binary format
	uint32_t dns1;		///< DNS server 1 IP address in binary format
	uint32_t dns2;		///< DNS server 2 IP address in binary format
} MwIpCfg;

/// \brief AP configuration message
/// \warning If ssid length is MW_SSID_MAXLEN, the string will not be NULL
///          terminated. Also if pass length equals MW_PASS_MAXLEN, pass
//           string will not be NULL terminated.
struct mw_msg_ap_cfg {
	uint8_t cfg_num;		///< Configuration number
	char ssid[MW_SSID_MAXLEN];	///< SSID string
	char pass[MW_PASS_MAXLEN];	///< Password string
};

/// \brief AP configuration message
/// \warning If ssid length is MW_SSID_MAXLEN, the string will not be NULL
///          terminated. Also if pass length equals MW_PASS_MAXLEN, pass
//           string will not be NULL terminated.
typedef struct {
	uint8_t cfgNum;			///< Configuration number
	char ssid[MW_SSID_MAXLEN];	///< SSID string
	char pass[MW_PASS_MAXLEN];	///< Password string
} MwMsgApCfg;

/// IP configuration message
struct mw_msg_ip_cfg {
	uint8_t cfg_slot;	///< Configuration slot
	uint8_t reserved[3];	///< Reserved (set to 0)
	struct mw_ip_cfg ip;	///< IPv4 configuration data
};

/// IP configuration message
typedef struct {
	uint8_t cfgNum;		///< Configuration number
	uint8_t reserved[3];	///< Reserved (set to 0)
	MwIpCfg ip;		///< IPv4 configuration data
} MwMsgIpCfg;

/// SNTP and timezone configuration
struct mw_msg_sntp_cfg {
	uint16_t up_delay;	///< Update delay in seconds (min: 15)
	int8_t tz;		///< Timezone (from -11 to 13)
	uint8_t dst;		///< Daylight savines (set to 1 to add 1 hour)
	/// Up to 3 NTP server URLs, separated by a NULL character. A double
	/// NULL marks the end of the server list.
	char servers[MW_CMD_MAX_BUFLEN - 4];
};

/// SNTP and timezone configuration
typedef struct {
	uint16_t upDelay;	///< Update delay in seconds (min: 15)
	int8_t tz;		///< Timezone (from -11 to 13)
	uint8_t dst;		///< Daylight savines (set to 1 to add 1 hour)
	/// Up to 3 NTP server URLs, separated by a NULL character. A double
	/// NULL marks the end of the server list.
	char servers[MW_CMD_MAX_BUFLEN - 4];
} MwMsgSntpCfg;

/// Date and time message
struct mw_msg_date_time {
	uint32_t dt_bin[2];	///< Number of seconds since Epoch (64-bit)
	/// Date and time in textual format
	char dt_str[MW_CMD_MAX_BUFLEN - sizeof(uint64_t)];
};

/// Date and time message
typedef struct {
	uint32_t dtBin[2];	///< Number of seconds since Epoch (64-bit)
	/// Date and time in textual format
	char dtStr[MW_CMD_MAX_BUFLEN - sizeof(uint64_t)];
} MwMsgDateTime;

/// Flash memory address and data
struct mw_msg_flash_data {
	uint32_t addr;		///< Flash memory address
	/// Data associated to the address
	uint8_t data[MW_CMD_MAX_BUFLEN - sizeof(uint32_t)];
};

/// Flash memory address and data
typedef struct {
	uint32_t addr;		///< Flash memory address
	/// Data associated to the address
	uint8_t data[MW_CMD_MAX_BUFLEN - sizeof(uint32_t)];
} MwMsgFlashData;

/// Flash memory block
struct mw_msg_flash_range {
	uint32_t addr;		///< Start address
	uint16_t len;		///< Length of the block
};

/// Flash memory block
typedef struct {
	uint32_t addr;		///< Start address
	uint16_t len;		///< Length of the block
} MwMsgFlashRange;

/// Bind message data
struct mw_msg_bind {
	uint32_t reserved;	///< Reserved, set to 0
	uint16_t port;		///< Port to bind to
	uint8_t  channel;	///< Channel used for the socket bound to port
};

/// Bind message data
typedef struct {
	uint32_t reserved;	///< Reserved, set to 0
	uint16_t port;		///< Port to bind to
	uint8_t  channel;	///< Channel used for the socket bound to port
} MwMsgBind;

/// MwState Possible states of the system state machine.
enum mw_state {
	MW_ST_INIT = 0,		///< Initialization state.
	MW_ST_IDLE,		///< Idle state, until connected to an AP.
	MW_ST_AP_JOIN,		///< Trying to join an access point.
	MW_ST_SCAN,		///< Scanning access points.
	MW_ST_READY,		///< Connected to The Internet.
	MW_ST_TRANSPARENT,	///< Transparent communication state.
	MW_ST_MAX			///< Limit number for state machine.
};

/// MwState Possible states of the system state machine.
typedef enum mw_state MwState;

/// Socket status.
enum mw_sock_stat {
	MW_SOCK_NONE = 0,	///< Unused socket.
	MW_SOCK_TCP_LISTEN,	///< Socket bound and listening.
	MW_SOCK_TCP_EST,	///< TCP socket, connection established.
	MW_SOCK_UDP_READY	///< UDP socket ready for sending/receiving
};

/// Socket status.
typedef enum mw_sock_stat MwSockStat;

/// System status
union mw_msg_sys_stat {
	uint32_t st_flags;		///< Accesses all the flags at once
	struct {
		enum mw_state sys_stat:8;	///< System status
		uint8_t online:1;	///< Module is connected to the Internet
		uint8_t cfg_ok:1;	///< Configuration OK
		uint8_t dt_ok:1;	///< Date and time synchronized at least once
		uint8_t cfg:2;		///< Network configuration set
		uint16_t reserved:3;	///< Reserved flags
		uint16_t ch_ev:16;	///< Channel flags with the pending event
	};
};

/// System status
typedef union {
	uint32_t st_flags;		///< Accesses all the flags at once
	struct {
		MwState sys_stat:8;	///< System status
		uint8_t online:1;	///< Module is connected to the Internet
		uint8_t cfg_ok:1;	///< Configuration OK
		uint8_t dt_ok:1;	///< Date and time synchronized at least once
		uint8_t cfg:2;		///< Network configuration set
		uint16_t reserved:3;	///< Reserved flags
		uint16_t ch_ev:16;	///< Channel flags with the pending event
	};
} MwMsgSysStat;

/// Command sent to system FSM
typedef union mw_cmd {
	char packet[MW_CMD_MAX_BUFLEN + 2 * sizeof(uint16_t)];
	struct {
		uint16_t cmd;			///< Command code
		uint16_t data_len;		///< Data length
		// If datalen is nonzero, additional command data goes here until
		// filling datalen bytes.
		union {
			uint8_t ch;		///< Channel number for channel related requests
			/// RAW data in uint8_t format
			uint8_t data[MW_CMD_MAX_BUFLEN];
			/// RAW data in uint32_t format
			uint32_t dw_data[MW_CMD_MAX_BUFLEN / sizeof(uint32_t)];
			struct mw_msg_in_addr in_addr;		///< Internet address
			struct mw_msg_ap_cfg ap_cfg;		///< Access Point configuration
			struct mw_msg_ip_cfg ip_cfg;		///< IP configuration
			struct mw_msg_sntp_cfg sntp_cfg;	///< SNTP client configuration
			struct mw_msg_date_time date_time;	///< Date and time message
			struct mw_msg_flash_data fl_data;	///< Flash memory data
			struct mw_msg_flash_range fl_range;	///< Flash memory range
			struct mw_msg_bind bind;		///< Bind message
			union mw_msg_sys_stat sys_stat;		///< System status
			uint16_t fl_sect;	///< Flash sector
			uint32_t fl_id;		///< Flash IDs
			uint16_t rnd_len;	///< Length of the random buffer to fill
		};
	};
} mw_cmd;

/// Command sent to system FSM
typedef struct {
	uint16_t cmd;			///< Command code
	uint16_t datalen;		///< Data length
	// If datalen is nonzero, additional command data goes here until
	// filling datalen bytes.
	union {
		uint8_t ch;		///< Channel number for channel related requests
		/// RAW data in uint8_t format
		uint8_t data[MW_CMD_MAX_BUFLEN];
		/// RAW data in uint32_t format
		uint32_t dwData[MW_CMD_MAX_BUFLEN / sizeof(uint32_t)];
		MwMsgInAddr inAddr;	///< Internet address
		MwMsgApCfg apCfg;	///< Access Point configuration
		MwMsgIpCfg ipCfg;	///< IP configuration
		MwMsgSntpCfg sntpCfg;	///< SNTP client configuration
		MwMsgDateTime datetime;	///< Date and time message
		MwMsgFlashData flData;	///< Flash memory data
		MwMsgFlashRange flRange;///< Flash memory range
		MwMsgBind bind;		///< Bind message
		MwMsgSysStat sysStat;	///< System status
		uint16_t flSect;	///< Flash sector
		uint32_t flId;		///< Flash IDs
		uint16_t rndLen;	///< Length of the random buffer to fill
	};
} MwCmd;

#endif //_MW_MSG_H_

/** \} */

