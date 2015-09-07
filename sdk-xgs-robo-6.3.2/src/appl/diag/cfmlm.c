/*
 * $Id: cfmlm.c 1.12 Broadcom SDK $
 * $Copyright: Copyright 2012 Broadcom Corporation.
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.  
 *  
 * Except as expressly set forth in the Authorized License,
 *  
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *  
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 *
 * Linemodule CLI access
 */

#if defined(VXWORKS)
#if defined(BMW) || defined(NSX) || defined(METROCORE)

#include "vxWorks.h"
#include "stdio.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "sockLib.h"
#include "ioLib.h"
#include "netinet/in.h"
#include <appl/diag/cmdlist.h>

#define TELNET_PORT 23

extern int sysSlotIdGet();
extern int atoi(const char *);

/* Debug Stuff */
UINT32 saddr = (192 << 24) | (168 << 16) | (0 << 8) | (1 << 0);
int useslotid = 1;

char lm_console_usage[] =
    "Parameters: <lm slot>\n\tConnect to LM slot.\n";

cmd_result_t
lm_console(int unit, args_t *a)
{
    int retval;
    char   buf[4];
    struct sockaddr_in  sa;
    int lineModSock;
    fd_set rfds;
    char *c;
    UINT32 laddr = (192 << 24) | (168 << 16) | (0 << 8) | (0 << 0);

    if ((c = ARG_GET(a)) == NULL) {
        return(ERROR);
    } else {
        printf("IP addr %d.%d.%d.%d\n",
        (saddr >> 24 ) & 0x000000FF,
        (saddr >> 16 ) & 0x000000FF,
        (saddr >> 8 ) & 0x000000FF,
        (saddr >> 0 ) & 0x000000FF
        );

        laddr |= (sysSlotIdGet() ? (1 << 8) : 0);
        laddr |= ((atoi(c)) & 0x000000FF);
        printf("slot %s IP addr %d.%d.%d.%d\n", c,
        (laddr >> 24 ) & 0x000000FF,
        (laddr >> 16 ) & 0x000000FF,
        (laddr >> 8 ) & 0x000000FF,
        (laddr >> 0 ) & 0x000000FF
        );
        if (useslotid) {
            saddr = laddr;
        }
        if (((laddr & 0x000000FF) < 3 ) || ((laddr & 0x000000FF) > 10 )) {
                printk("Slot ID should be 3 .. 10\n");
                return(ERROR);
        }
    }


    if ((lineModSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Socket Create failed\n");
        return (ERROR);
    }

    /* connect to Telnet port */
    bzero ((char *)&sa, sizeof(sa));
    sa.sin_family      = AF_INET;
    sa.sin_port        = htons(TELNET_PORT);
    sa.sin_addr.s_addr = htonl(saddr);
    if (connect(lineModSock, (struct sockaddr *)&sa, sizeof(sa)) < 0 )
    {
        printf("connect failed\n");
        return (ERROR);
    }

    printf("connect success %d\n", lineModSock);


    while(1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(lineModSock, &rfds);
        retval = select(lineModSock + 1, &rfds, NULL, NULL, NULL);

        if (retval < 0) break;

        if (FD_ISSET(0, &rfds)) {
            if (read(0, buf, 1)) {
                write(lineModSock, buf, 1);
            } else {
                break;
            }
        } else if (FD_ISSET(lineModSock, &rfds)) {
            if (read(lineModSock, buf, 1)) {
                write(0, buf, 1);
            } else {
                break;
            }
        }
    }

    close(lineModSock);

    return(OK);
}

#endif /* defined(BMW) || defined(NSX) || defined(METROCORE) */
#endif /* defined(VXWORKS) */

int _bcm_diag_cfmlm_not_empty;
