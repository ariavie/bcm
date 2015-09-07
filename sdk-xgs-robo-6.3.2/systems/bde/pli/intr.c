/*
 * $Id: intr.c 1.10 Broadcom SDK $
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
 * Interrupt controller task.
 * This task emulates an interrupt handler and callout mechanism.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>

#ifdef VXWORKS
#include <vxWorks.h>
#include <sockLib.h>
#endif

#include <sal/appl/sal.h>
#include <sal/appl/io.h>
#include <sal/core/thread.h>
#include <sal/core/sync.h>
#include <sal/core/spl.h>
#include <soc/debug.h>	

#include "verinet.h"

int
intr_int_context(void)
{
    int i;

    for (i = 0; i < N_VERINET_DEVICES; i++) {
	if (sal_thread_self() == verinet[i].intrDispatchThread) {
	    return 1;
	}
    }

    return 0;
}

static void
intr_dispatch(void *v_void)
{
    verinet_t *v = (verinet_t *)v_void;
    int s;

    debugk(DK_VERINET,
	   "intr_dispatch: running, sema=%p\n",
	   v->intrDispatchSema);

    while (!v->intrFinished) {
	if (sal_sem_take(v->intrDispatchSema, sal_sem_FOREVER)) {
	    printk("intr_dispatch: sal_sem_take failed\n");
	    continue;
	}

	/*
	 * In real hardware, interrupts aren't taken while at splhi.  We
	 * simulate this in our thread-based simulation simply by taking
	 * the semaphore for the duration of the call to the interrupt
	 * vector.  To be correct, sal_splhi() would actually have to
	 * suspend all other threads and spl() resume them.
	 */

	debugk(DK_VERINET,
	       "intr_dispatch: ISR=%p ISR_data=%p\n",
	       v->ISR, v->ISR_data);

	if (v->ISR) {
	    s = sal_splhi();
	    (*v->ISR)(v->ISR_data);
	    sal_spl(s);
	}
    }

    debugk(DK_VERINET, "intr_dispatch: thread exiting ....\n");
    sal_sem_destroy(v->intrDispatchSema);
    printk("INTC dispatcher shutdown.\n");
    v->intrDispatchThread = 0;
    sal_thread_exit(0);
}

/*
 * Process an interrupt request dispatched on the socket.
 */

void
intr_handler(void *v_void)
{
    verinet_t *v = (verinet_t *)v_void;
    uint32 cause;
    rpc_cmd_t cmd;
    int sockfd = v->intrFd;

    v->intrFinished = 0;

    /* Start off dispatch thread */

    v->intrDispatchSema =
	sal_sem_create("Interrupt-dispatch", sal_sem_BINARY, 0);

    if (!v->intrDispatchSema) {
	printk("intr_handler: failed to create dispatch sem\n");
	v->intrFinished = 1;
    } else {
	v->intrDispatchThread = sal_thread_create("Interrupt-dispatch",
						  SAL_THREAD_STKSZ, 100,
						  intr_dispatch, v);
	if (SAL_THREAD_ERROR == v->intrDispatchThread) {
	    printk("intr_handler: failed to create dispatch thread\n");
	    sal_sem_destroy(v->intrDispatchSema);
	    v->intrDispatchSema = 0;
	    v->intrFinished = 1;
	}
    }

    while (!v->intrFinished) {
	debugk(DK_VERINET, "intr_handler: wait...\n");

	if (wait_command(sockfd, &cmd) < 0) {
	    break;	/* Error message already printed */
	}

	debugk(DK_VERINET,
	       "intr_handler: request opcode 0x%x\n", cmd.opcode);

	switch(cmd.opcode) {

	case RPC_SEND_INTR:
	    /* Read cause */
	    cause = cmd.args[0];
	    make_rpc_intr_resp(&cmd,cause);
	    write_command(sockfd, &cmd);

	    if (v->intrSkipTest) {
		debugk(DK_VERINET, "Test interrupt received (ignoring)\n");
		v->intrSkipTest = 0;
		break;
	    }

	    /* Wake up dispatch thread */

	    debugk(DK_VERINET, "Wake dispatch\n");

	    sal_sem_give(v->intrDispatchSema);
	    break;

	case RPC_DISCONNECT:
	    v->intrFinished = 1;
	    v->intrWorkerExit++;
	    printk("intr handler received disconnect\n");
	    sal_sem_give(v->intrDispatchSema);
	    break;

	default:
	    /* Unknown opcode */
	    break;
	}
    }
    printk("Interrupt service shutdown.\n");
    close(sockfd);

    /* If dispatch thread around - wait for him to exit */

    if (v->intrDispatchSema) {
	sal_sem_give(v->intrDispatchSema);
    }

    sal_thread_exit(0);
}

void
intr_set_vector(verinet_t *v, pli_isr_t isr, void *isr_data)
{
    /*
     * Set interrupt vector
     */

    debugk(DK_VERINET,
	   "intr_set_vector: dev=%d isr=%p isr_data=%p\n",
	   (int)(v - verinet), isr, isr_data);

    v->ISR = isr;
    v->ISR_data = isr_data;
}

/*
 * start_isr_service: Sets up the socket and handles client requests
 * with a child thread per socket.
 */
void
intr_listener(void *v_void)
{
    verinet_t *v = (verinet_t *)v_void;
    int sockfd, newsockfd, one = 1;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("server: can't open stream socket");
	exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		   (char *)&one, sizeof (one)) < 0) {
	perror("setsockopt");
    }

    /*
     * Setup server address...
     */
    memset((void *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(0);	/* Pick any port */

    /*
     * Bind our local address
     */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	perror("dmac: unable to bind address");
	sal_thread_exit(0);
    }

    v->intPort = getsockport(sockfd);	/* Get port that was picked */

    /* listen for inbound homies ...*/
    listen(sockfd, 5);

    /* Notify intr_init that socket is listening */
    sal_sem_give(v->intrListening);

    printk("ISR dispatcher listening on port[%d]\n", v->intPort);

    while (!v->intrWorkerExit) {
	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0 && errno == EINTR) {
	    continue;
	}

	if (newsockfd < 0) {
	    perror("server: accept error");
	} else {
	    v->intrSkipTest = 1;

	    v->intrFd = newsockfd;

	    v->intrHandlerThread = sal_thread_create("Interrupt-handler",
						     SAL_THREAD_STKSZ, 100,
						     intr_handler, v);

	    if (SAL_THREAD_ERROR == v->intrHandlerThread) {
		printk("start_isr_service: thread creation error\n");
	    } else{
		debugk(DK_VERINET, "Interrupt request thread dispatched.\n");
	    }
	}
    }

    debugk(DK_VERINET, "INTC listener shutdown.\n");
    sal_thread_exit(0);
}

int
intr_init(verinet_t *v)
{
    if (v->intrInited) {
	return 0;
    }

    v->intrListening = sal_sem_create("intr listener", sal_sem_BINARY, 0);

    /*
     * Create Interrupt task
     */
    printk("Starting Interrupt service...\n");

    v->intrThread = sal_thread_create("Interrupt-listener",
				      SAL_THREAD_STKSZ, 100,
				      intr_listener, v);

    if (SAL_THREAD_ERROR == v->intrThread) {
	printk("ERROR: could not create interrupt listener task!\n");
	return -2;
    }

    sal_sem_take(v->intrListening, sal_sem_FOREVER);   /* Wait for listen() */
    sal_sem_destroy(v->intrListening);

    v->intrInited = 1;

    return 0;
}
