/*
 * $Id: proxy-service.c 1.8 Broadcom SDK $
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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sal/core/thread.h>

#include "proxy-service.h"

/*
 * Function: _proxy_thread
 *
 * Purpose:
 *    Implements the proxy data loop
 * Parameters:
 *    ctrl - proxy control structure
 * Returns:
 *    Nothing
 */
static void
_proxy_thread(proxy_ctrl_t* ctrl)
{
    unsigned char* data = malloc(ctrl->max_data_size); 
    assert(data); 
    assert(ctrl->input_cb); 

    for (;;) {
	unsigned int len = ctrl->max_data_size; 
	memset(data, 0, sizeof(data)); 
	
	/* Receive packets from the given input callback */
	if (ctrl->input_cb(ctrl, data, &len) >= 0) {
	    /* Send it to the output callback */
	    if (ctrl->output_cb) {
		ctrl->output_cb(ctrl, data, &len); 
	    }
	}
	
	if (ctrl->exit) {
            if (ctrl->exit_cb) {
		ctrl->exit_cb(ctrl, NULL, 0); 
            }
            free(data);
	    ctrl->exit = 0; 
	    return; 
	}
    }
}

/*
 * Function: proxy_service_start
 *
 * Purpose:
 *    Start a proxy thread/loop
 * Parameters:
 *    ctrl - proxy control structure
 *    fork - indicates whether a new thread should be created. 
 *       TRUE: run loop in a new thread.
 *       FALSE: Run loop (blocking)
 * Returns:
 *    Nothing
 */
int 
proxy_service_start(proxy_ctrl_t* ctrl, int fork)
{
    if (fork) {
	sal_thread_create("_proxy_thread", 0, 0, 
			  (void (*)(void*))_proxy_thread, ctrl); 
    } else {
	_proxy_thread(ctrl); 
    }
    return 0; 
}
