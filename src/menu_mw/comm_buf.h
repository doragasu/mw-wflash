#ifndef _COMM_BUF_H_
#define _COMM_BUF_H_

#include "../cmds.h"

/// Length of the wflash buffer
#define MW_BUFLEN	WF_MAX_DATALEN

/// Internal command buffer, exported for the main module to use it in
/// initialization routines. Size is double MW_BUFLEN to be able to use
/// double buffer. Also has two additional words to be able to accomodate
/// two extra bytes when receiving frames with an odd number of bytes.
extern char cmd_buf[2 * MW_BUFLEN + 4];

#endif /*_COMM_BUF_H_*/

