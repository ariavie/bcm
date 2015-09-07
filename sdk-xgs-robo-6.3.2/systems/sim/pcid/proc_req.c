/*
 * $Id: proc_req.c 1.13 Broadcom SDK $
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
 * File:        proc_req.c
 * Purpose:
 * Requires:    
 */


#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <soc/mem.h>
#include <soc/hash.h>

#include <soc/cmic.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/schanmsg.h>
#include <sal/appl/io.h>
#include <bde/pli/verinet.h>

#include "pcid.h"
#include "mem.h"
#include "cmicsim.h"
#include "dma.h"
#include "pli.h"

STATIC rpc_cmd_t cmd;
/*
 * Handles an incoming client request
 * We block until the client has something to send us and then 
 * we do something over the socket descriptor.
 * Returns 1 when a command is processed, 0 on disconnect.
 */

int pcid_process_request(pcid_info_t *pcid_info, int sockfd,
                    struct timeval *tmout)
{
    char buf[32];
    uint32 regval;
    int r, unit, finished = 0;

    unit = pcid_info->unit;
    r = get_command(sockfd, tmout, &cmd);

    if (r == 0) {
	return PR_NO_REQUEST;	/* No request; return to poll later */
    }

    if (r < 0) {		/* Error; cause polling loop to exit */
	return PR_ERROR;
    }

    switch (cmd.opcode) {
	/*
	 * We only handle requests: anything else is a programming
	 * error. Once we are here, all parameters have been marshalled
	 * and we simply need to call the correct routine (see pli_cmd.c).
	 */

    case RPC_SCHAN_REQ:
        {       
            schan_msg_t msg; 
            int i; 
            int dw_write, dw_read; 
            extern int schan_op(pcid_info_t *pcid_info, int unit, schan_msg_t* data); 

            dw_write = cmd.args[0]; 
            dw_read = cmd.args[1]; 
        
            for(i = 0; i < dw_write; i++) {
                msg.dwords[i] = cmd.args[i+2]; 
            }
            if (!(pcid_info->schan_cb) || 
                (i = (pcid_info->schan_cb)(pcid_info, unit, &msg))) {
                i = schan_op(pcid_info, unit, &msg); 
            }
            make_rpc(&cmd, RPC_SCHAN_RESP, RPC_OK, 0); 
            cmd.argcount = dw_read+1; 
            cmd.args[0] = i; 
            for(i = 0; i < dw_read; i++) {
                cmd.args[i+1] = msg.dwords[i]; 
            }
            if (write_command(sockfd, &cmd) != RPC_OK) {
                pcid_info->opt_rpc_error = 1;
            }
            break; 
        }       

    case RPC_GETREG_REQ: /* PIO read of register */
	/* Get a register from our model ... */
	/* cmd.args[1] is register type (verinet.h) */

	regval = pli_getreg_service(pcid_info, unit, cmd.args[1], cmd.args[0]);
      
	strcpy(buf, "********");
	make_rpc_getreg_resp(&cmd,
			     0,
			     regval,
			     buf);
    if (write_command(sockfd, &cmd) != RPC_OK) {
        pcid_info->opt_rpc_error = 1;
    }

	break;

    case RPC_SETREG_REQ: /* PIO write to register */
	/* Set a register on our model... */
        /* coverity[stack_use_overflow] */
	pli_setreg_service(pcid_info, unit, cmd.args[2], cmd.args[0],
                           cmd.args[1]);
	make_rpc_setreg_resp(&cmd, cmd.args[1]);
    if (write_command(sockfd, &cmd) != RPC_OK) {
        pcid_info->opt_rpc_error = 1;
    }
	break;
      
    case RPC_IOCTL_REQ: /* Change something inside the model */
        if (pcid_info->ioctl) {
            pcid_info->ioctl(pcid_info, cmd.args);
        }
	break;
       
    case RPC_SHUTDOWN: /* Change something inside the model */
        if (pcid_info->ioctl) {
            cmd.args[0] = BCM_SIM_DEACTIVATE;
            pcid_info->ioctl(pcid_info, cmd.args);
        }
	break;
       
    case RPC_REGISTER_CLIENT:
	/* Setup port for DMA r/w requests to client and interrupts ...*/
	/* Connect to client interrupt and DMA ports and test*/
        printk("Received register client request\n");
	regval = RPC_OK;

	pcid_info->client = register_client(sockfd,
				 cmd.args[0],
				 cmd.args[1],
				 cmd.args[2]);

	if (pcid_info->client == 0) {
	    finished = 1;
	    break;
	}

	/*
	 * TEST: Write to DMA address 0 on client.
	 */
	r = dma_writemem(pcid_info->client->dmasock,
			      0,
			      0xbabeface);
      
	/*
	 * TEST: Read from DMA address 0 on client.
	 */
	r = dma_readmem(pcid_info->client->dmasock,0, &regval);
      
	/* TEST: Result should be the same, if not FAIL! */
	if(regval != 0xbabeface){
	    printk("ERROR: regval = 0x%x\n", regval);
	    r = RPC_FAIL;
	}
	
	/*
	 * TEST: Send an interrupt to the client.
	 */
	r = send_interrupt(pcid_info->client->intsock, 0);
        if (r < 0) {
            debugk(DK_ERR, "RPC error: soc_internal_send_int failed. \n");
            pcid_info->opt_rpc_error = 1;
        } else {
	    r = RPC_OK;
        }

	/* Send Registration status back ... */
	make_rpc_register_resp(&cmd, r);
	if (write_command(pcid_info->client->piosock, &cmd) != RPC_OK) {
        pcid_info->opt_rpc_error = 1;
    }

	debugk(DK_VERBOSE,
	       "Client registration: 0x%x DMA=0x%x, INT=0x%x, PIO=0x%x -OK\n",
	       pcid_info->client->inetaddr,
	       pcid_info->client->dmasock,
	       pcid_info->client->intsock,
	       pcid_info->client->piosock);

	break;

    case RPC_DISCONNECT:
	/* First disconnect the interrupt and dma sockets on client */

	unregister_client(pcid_info->client);

	finished = 1;
	break;

    default:
	/* Unknown opcode */
	break;
    }

    return (finished ? PR_ALL_DONE : PR_REQUEST_HANDLED);
}
