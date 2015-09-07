/*
 * $Id: socintf.c 1.7 Broadcom SDK $
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
 * The part of PCID that interfaces to sockets
 *
 * Requires:
 *     Socket library
 *     Verinet functions (write_command, etc)
 *     pli_getreg_service
 *     pli_setreg_service
 *
 *     
 * Provides:
 *     pcid_setup_socket
 *     pcid_wait_for_cnxn
 *     pcid_process_request
 *     
 */

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "pcid.h"
#include "mem.h"
#include "cmicsim.h"
#include "dma.h"
#include "pli.h"

int
pcid_setup_socket(int port)
{
    struct sockaddr_in serv_addr;
    int i;
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	
	perror("server: can't open stream socket");
	exit(1);
    }

    /*
     * Set socket option to reuse local address. This is supposed
     * to have the effect of freeing up the local address.
     */

    i = 1;
    if (setsockopt(sockfd, SOL_SOCKET,
                   SO_REUSEADDR, (char *) &i, 4) < 0) {
	perror("setsockopt");
    }

    /*
     * Set up server address...
     */

    memset((void *) &serv_addr,0x0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(port);

    /*
     * Bind our local address & port
     */

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	perror("server: can't bind local address");
	exit(1);
    }

    /*
     * Only process one connection at a time.
     */

    listen(sockfd, 1);

    return sockfd;
}

int pcid_wait_for_cnxn(int sockfd)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    int newsockfd;

    for (;;) {
	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

	if (newsockfd >= 0) {
	    break;
	}

	if (errno != EINTR) {
	    perror("accept");
	    exit(1);
	}
    }

    return newsockfd;
}

void pcid_close_cnxn(int sockfd)
{
    close(sockfd);
}

