/***********************************************************************
 *
 * $Id: bcm-uk-rtrans.c,v 1.9 Broadcom SDK $
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
 * This provides a simple implementation of a BCM C2C/NH transport driver. 
 * This driver uses a User/Kernel Proxy service to transmit and receive the
 * data. 
 * This driver is compiled for both the kernel and user space. 
 *
 **********************************************************************/

#include <shared/bsl.h>

#include <sal/core/alloc.h>
#include <sal/core/thread.h>

#include <bcm/error.h>

#include <soc/cm.h>

#include <bcm/tx.h>
#include <bcm/rx.h>
#include <assert.h>

#include <linux-uk-proxy.h>
#include <bcm-uk-trans.h>

/* 
 * This is the name of the proxy service this module will use 
 */
static char _input_proxy_service[] = "BCM UK PROXY REMOTE TRANSPORT"; 
static char _output_proxy_service[] = "BCM UK PROXY REMOTE TRANSPORT"; 


/* 
 * Receives packets from the HW, sends them to user space 
 */
static bcm_rx_t
_bcm_rx_cb(int unit, bcm_pkt_t* pkt, void* cookie)
{
    /* User space expects the bcm_pkt_t before the data */
    unsigned char data[LUK_MAX_DATA_SIZE]; 
    unsigned char* p = data; 
    
    /* The bcm_pkt_t structure is expected at the start of the data */
    /* The pointers will get fixed up in user space */
    memcpy(p, pkt, sizeof(*pkt)); 
    p+=sizeof(*pkt);     
    pkt->pkt_data->len = pkt->pkt_len; 
    memcpy(p, pkt->pkt_data->data, pkt->pkt_data->len); 
    
    /* Send it */    
    linux_uk_proxy_send(_output_proxy_service, data, pkt->pkt_data->len+sizeof(*pkt)+4); 
    /* Other people might want this at this point */
    return BCM_RX_NOT_HANDLED; 
}

/*
 * Function:
 *	_rx_handle
 * Purpose:
 *	Processes a received frame for the transport
 */
static int
_handle_user_tx(unsigned char* data, int len)
{
    bcm_pkt_t* pkt;
    unsigned char* p = data; 
    int l = len; 
    /* We received a packet from user space. Need to tx it */
    /* pkt structure is at the beginning of the packet */
    pkt = (bcm_pkt_t*)p; 
    p += sizeof(bcm_pkt_t); 
    l -= sizeof(bcm_pkt_t); 

    /* Set up the data block */
    pkt->pkt_data = &pkt->_pkt_data; 
    pkt->pkt_data->data = p; 
    pkt->pkt_data->len = l;     
    pkt->blk_count = 1; 
    
    bcm_tx(pkt->unit, pkt, NULL);     
    bcm_rx_free(0, data); 
    
    return 0; 
}


/*
 * Function:
 *	_rx_thread
 * Purpose:
 *	Receives and dispatches packets from the kernel proxy service. 
 */

typedef struct rx_thread_ctrl_s {
    const char* service; 
    int exit; 
} thread_ctrl_t; 

static void
_user_tx_thread(thread_ctrl_t* ctrl)
{
    unsigned char* data = NULL; 
    
    for(;;) {
	unsigned int len = LUK_MAX_DATA_SIZE; 
	if(!data) {
	    /* Transport client expects data to be allocated from the rx buffer pool */
	    bcm_rx_alloc(0, LUK_MAX_DATA_SIZE, 0, (void*)&data); 
	    assert(data); 
	}
	memset(data, 0, sizeof(data)); 

	if(linux_uk_proxy_recv(ctrl->service, data, &len) >= 0) {
	    _handle_user_tx(data, len); 
	    
	    data = NULL; 
	}
	
	if(ctrl->exit) {
	    ctrl->exit = 0; 
	    return; 
	}
    }
}
	
/* RX thread control */
static thread_ctrl_t _ctrl; 

int 
bcm_uk_rtrans_create(const char* service, bcm_trans_ptr_t** trans)
{   
    if(service) {
	strcpy(_input_proxy_service, service); 
	strcpy(_output_proxy_service, service); 
    }

    linux_uk_proxy_service_create(_input_proxy_service, 64, 0); 

    _ctrl.service = _input_proxy_service; 
    _ctrl.exit = 0; 

    sal_thread_create("BCM UK RT", 0, 0, (void (*)(void*))_user_tx_thread, &_ctrl); 

    /* Register an rx handler to send packets back to user */
    {	
	int rc; 
	if((rc = bcm_rx_register(0, "", _bcm_rx_cb, 250, NULL, BCM_RCO_F_ALL_COS)) < 0) {
	    LOG_CLI((BSL_META("register failed: %d (%s)\n"), rc, bcm_errmsg(rc))); 
	}
	else {
	    LOG_CLI((BSL_META("registered callback\n"))); 
	}
	if((rc = bcm_rx_start(0, NULL)) < 0) {
	    LOG_CLI((BSL_META("start failed: %d (%s)\n"), rc, bcm_errmsg(rc))); 
	}
	else {
	    LOG_CLI((BSL_META("rx started\n"))); 
	}
    }

    *trans = NULL; 
    return 0; 
}
    
	 
       














