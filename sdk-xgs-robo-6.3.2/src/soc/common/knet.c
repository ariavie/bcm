/*
 * $Id: knet.c 1.11 Broadcom SDK $
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
 * File: 	knet.c
 * Purpose: 	Kernel network control
 */

#include <soc/drv.h>
#include <soc/knet.h>

#ifdef INCLUDE_KNET

static soc_knet_vectors_t kvect;

#define KNET_OPEN       kvect.kcom.open
#define KNET_CLOSE      kvect.kcom.close
#define KNET_SEND       kvect.kcom.send
#define KNET_RECV       kvect.kcom.recv


typedef struct knet_cmd_ctrl_s {
    sal_mutex_t cmd_lock;
    sal_mutex_t msg_lock;
    sal_sem_t cmd_done;
    int opcode;
    int seqno;
    int resp_len;
    kcom_msg_t resp_msg;
} knet_cmd_ctrl_t;

static knet_cmd_ctrl_t knet_cmd_ctrl;

/*
 * Simple handler registration (emphasis on simple). 
 */
#define MAX_CALLBACKS 5

static struct callback_ctrl_s {
    soc_knet_rx_cb_t cb; 
    void* cookie; 
} _callback_ctrl[MAX_CALLBACKS]; 

static int knet_rx_thread_run = 0;

int
soc_knet_rx_register(soc_knet_rx_cb_t callback, void *cookie, uint32 flags)
{
    int idx; 

    for (idx = 0; idx < MAX_CALLBACKS; idx++) {
	if (_callback_ctrl[idx].cb == NULL) {
	    _callback_ctrl[idx].cb = callback; 
	    _callback_ctrl[idx].cookie = cookie; 
	    return SOC_E_NONE; 
	}
    }
    return SOC_E_RESOURCE; 
}

int
soc_knet_rx_unregister(soc_knet_rx_cb_t callback)
{
    int idx; 

    for (idx = 0; idx < MAX_CALLBACKS; idx++) {
	if (_callback_ctrl[idx].cb == callback) {
	    _callback_ctrl[idx].cb = NULL; 
	    _callback_ctrl[idx].cookie = NULL; 
	}
    }
    return SOC_E_NONE; 
}

int
soc_knet_handle_cmd_resp(kcom_msg_t *kmsg, unsigned int len, void *cookie)
{
    knet_cmd_ctrl_t *ctrl = (knet_cmd_ctrl_t *)cookie;

    if (kmsg->hdr.type != KCOM_MSG_TYPE_RSP) {
        /* Not handled */
        return 0;
    }
    if (kmsg->hdr.opcode != ctrl->opcode) {
        soc_cm_print("soc_knet_handle_cmd_resp: wrong opcode %d (expected %d)\n",
                     kmsg->hdr.opcode, ctrl->opcode);
    } else if (kmsg->hdr.seqno != ctrl->seqno) {
        soc_cm_print("soc_knet_handle_cmd_resp: wrong seq no %d (expected %d)\n",
                     kmsg->hdr.seqno, ctrl->seqno);
    } else {
        if (len > sizeof(ctrl->resp_msg)) {
            soc_cm_print("soc_knet_handle_cmd_resp: resp too long (%d bytes)\n",
                         len);
        } else {
            SOC_DEBUG_PRINT((DK_VERBOSE,
                             "soc_knet_handle_cmd_resp: got %d bytes\n", len));
            if (kmsg->hdr.status != 0) {
                SOC_DEBUG_PRINT((DK_VERBOSE,
                                 "soc_knet_handle_cmd_resp: status %d\n",
                                 kmsg->hdr.status));
            }
            sal_mutex_take(ctrl->msg_lock, sal_mutex_FOREVER);
            sal_memcpy(&ctrl->resp_msg, kmsg, len);
            ctrl->resp_len = len;
            sal_mutex_give(ctrl->msg_lock);
        }
        sal_sem_give(ctrl->cmd_done);
    }
    /* Handled */
    return 1;
}

int
soc_knet_cmd_req(kcom_msg_t *kmsg, unsigned int len, unsigned int buf_size)
{
    int rv;
    knet_cmd_ctrl_t *ctrl = &knet_cmd_ctrl;

    if (!knet_rx_thread_run) {
        return SOC_E_UNAVAIL;
    }

    sal_mutex_take(ctrl->cmd_lock, sal_mutex_FOREVER);

    kmsg->hdr.type = KCOM_MSG_TYPE_CMD;
    kmsg->hdr.seqno = 0;

    ctrl->opcode = kmsg->hdr.opcode;

    rv = KNET_SEND(KCOM_CHAN_KNET, kmsg, len, buf_size);
    if (rv < 0) {
        soc_cm_print("soc_knet_cmd_req: command failed\n");
    } else if (rv > 0) {
        /* Synchronous response - no need to wait */
    } else if (sal_sem_take(ctrl->cmd_done, 2000000)) {
        soc_cm_print("soc_knet_cmd_req: command timeout\n");
        rv = SOC_E_TIMEOUT;
    } else {
        SOC_DEBUG_PRINT((DK_VERBOSE,
                         "soc_knet_cmd_req: command OK\n"));
        len = ctrl->resp_len;
        if (len > buf_size) {
            SOC_DEBUG_PRINT((DK_VERBOSE,
                             "soc_knet_cmd_req: oversized response "
                             "(%d bytes, max %d)\n",
                             len, buf_size));
            len = buf_size;
        }
        sal_mutex_take(ctrl->msg_lock, sal_mutex_FOREVER);
        sal_memcpy(kmsg, &ctrl->resp_msg, len);
        sal_mutex_give(ctrl->msg_lock);
        switch (kmsg->hdr.status) {
        case KCOM_E_NONE:
            rv = SOC_E_NONE;
            break;
        case KCOM_E_PARAM:
            rv = SOC_E_PARAM;
            break;
        case KCOM_E_RESOURCE:
            rv = SOC_E_RESOURCE;
            break;
        case KCOM_E_NOT_FOUND:
            rv = SOC_E_NOT_FOUND;
            break;
        default:
            rv = SOC_E_FAIL;
            break;
        }
    }

    sal_mutex_give(ctrl->cmd_lock);

    return rv;
}

int
soc_knet_handle_rx(kcom_msg_t *kmsg, unsigned int len)
{
    int unit;
    int idx;
    int rv = 0;

    /* Check for valid header */
    if (len < sizeof(kcom_msg_hdr_t)) {
        return SOC_E_INTERNAL;
    }

    /* Check for valid unit */
    unit = kmsg->hdr.unit;
    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }

    for (idx = 0; idx < MAX_CALLBACKS; idx++) {
	if (_callback_ctrl[idx].cb != NULL) {
	    rv = _callback_ctrl[idx].cb(kmsg, len, _callback_ctrl[idx].cookie);
            if (rv > 0) {
                /* Message handled */
                return SOC_E_NONE; 
            }
	}
    }
    if (rv == 0) {
        soc_cm_print("soc_knet_handle_rx: unhandled (type=%d, opcode=%d)\n",
                     kmsg->hdr.type, kmsg->hdr.opcode);
    }
    return SOC_E_NOT_FOUND;
}

void
soc_knet_rx_thread(void *context)
{
    kcom_msg_t kmsg;
    int len;

    while (knet_rx_thread_run) {
        if ((len = KNET_RECV(KCOM_CHAN_KNET, &kmsg, sizeof(kmsg))) < 0) {
            soc_cm_print("knet rx error - thread aborting\n");
            knet_rx_thread_run = 0;
            return;
        }
        soc_knet_handle_rx(&kmsg, len);
    }
}

int
soc_knet_config(soc_knet_vectors_t *vect)
{
    if (vect == NULL) {
        return SOC_E_PARAM;
    }

    kvect = *vect;

    return SOC_E_NONE;
}

int
soc_eth_knet_hw_config(int unit, int type, int chan, uint32 flags, uint32 value)
{
    unsigned int len;
    kcom_msg_eth_hw_config_t kmsg;

    sal_memset(&kmsg, 0, sizeof(kmsg));
    kmsg.hdr.opcode = KCOM_M_ETH_HW_CONFIG;
    kmsg.hdr.unit = unit;
    kmsg.config.type = type;
    kmsg.config.chan = chan;
    kmsg.config.flags = flags;
    kmsg.config.value = value;

    len = sizeof(kmsg);

    return soc_knet_cmd_req((kcom_msg_t *)&kmsg, len, sizeof(kmsg));
}

int
soc_knet_hw_reset(int unit, int channel)
{
    unsigned int len;
    kcom_msg_hw_reset_t kmsg;

    sal_memset(&kmsg, 0, sizeof(kmsg));
    kmsg.hdr.opcode = KCOM_M_HW_RESET;
    kmsg.hdr.unit = unit;
    if (channel >= 0) {
        kmsg.channels = (1 << channel);
    }
    len = sizeof(kmsg);

    return soc_knet_cmd_req((kcom_msg_t *)&kmsg, len, sizeof(kmsg));
}

int
soc_knet_hw_init(int unit)
{
    unsigned int len;
    kcom_msg_hw_init_t kmsg;

    sal_memset(&kmsg, 0, sizeof(kmsg));
    kmsg.hdr.opcode = KCOM_M_HW_INIT;
    kmsg.hdr.unit = unit;
    kmsg.dcb_size = SOC_DCB_SIZE(unit);
    kmsg.dcb_type = SOC_DCB_TYPE(unit);
    len = sizeof(kmsg);

    return soc_knet_cmd_req((kcom_msg_t *)&kmsg, len, sizeof(kmsg));
}

int
soc_knet_check_version(int unit)
{
    int rv;
    unsigned int len;
    kcom_msg_version_t kmsg;

    sal_memset(&kmsg, 0, sizeof(kmsg));
    kmsg.hdr.opcode = KCOM_M_VERSION;
    kmsg.hdr.unit = unit;
    kmsg.version = KCOM_VERSION;
    len = sizeof(kmsg);

    rv = soc_knet_cmd_req((kcom_msg_t *)&kmsg, len, sizeof(kmsg));

    if (SOC_SUCCESS(rv) && kmsg.version != KCOM_VERSION) {
        rv = SOC_E_INTERNAL;
    }

    return rv;
}

int
soc_knet_irq_mask_set(int unit, uint32 addr, uint32 mask)
{
    if (kvect.irq_mask_set) {
        return kvect.irq_mask_set(unit, addr, mask);
    }
    soc_pci_write(unit, addr, mask);
    return SOC_E_NONE;
}

int
soc_knet_cleanup(void)
{
    soc_knet_rx_unregister(soc_knet_handle_cmd_resp);

    if (knet_cmd_ctrl.cmd_lock != NULL) {
        sal_mutex_destroy(knet_cmd_ctrl.cmd_lock);
    }
    if (knet_cmd_ctrl.msg_lock != NULL) {
        sal_mutex_destroy(knet_cmd_ctrl.msg_lock);
    }
    if (knet_cmd_ctrl.cmd_done != NULL) {
        sal_sem_destroy(knet_cmd_ctrl.cmd_done);
    }
    return SOC_E_NONE;
}

int
soc_knet_init(int unit)
{
    int rv;
    void *knet_chan;
    sal_thread_t rxthr;
    knet_cmd_ctrl_t *ctrl = &knet_cmd_ctrl;
    kcom_msg_string_t kmsg;

    if (KNET_OPEN == NULL) {
        /* KCOM vectors not assigned */
        SOC_DEBUG_PRINT((DK_VERBOSE,
                         "soc_knet_init: No KCOM vectors\n"));
        return SOC_E_CONFIG;
    }

    if (!SOC_IS_XGS3_SWITCH(unit) &&         
        !(soc_cm_get_dev_type(unit) & SOC_ETHER_DEV_TYPE)) {        
        /* Only XGS3-style DCBs are supported */
        /* ROBO KNET is supported */        
        return SOC_E_UNAVAIL;
    }

    if (knet_rx_thread_run) {
        /* Already initialized */
        return SOC_E_NONE;
    }

    ctrl->cmd_lock = sal_mutex_create("KNET CMD");
    if (ctrl->cmd_lock == NULL) {
        soc_knet_cleanup();
        return SOC_E_RESOURCE;
    }
    ctrl->msg_lock = sal_mutex_create("KNET MSG");
    if (ctrl->msg_lock == NULL) {
        soc_knet_cleanup();
        return SOC_E_RESOURCE;
    }
    ctrl->cmd_done = sal_sem_create("KNET CMD", 1, 0);
    if (ctrl->cmd_done == NULL) {
        soc_knet_cleanup();
        return SOC_E_RESOURCE;
    }

    /* Check if kernel messaging is initialized */
    knet_chan = KNET_OPEN(KCOM_CHAN_KNET);
    if (knet_chan == NULL) {
        SOC_DEBUG_PRINT((DK_VERBOSE,
                         "knet open failed\n"));
        soc_knet_cleanup();
        return SOC_E_FAIL;
    }

    /* Try sending string event to see if kernel module is present */
    sal_memset(&kmsg, 0, sizeof(kmsg));
    kmsg.hdr.opcode = KCOM_M_STRING;
    kmsg.hdr.type = KCOM_MSG_TYPE_EVT;
    sal_strcpy(kmsg.val, "soc_knet_init");
    rv = KNET_SEND(KCOM_CHAN_KNET, &kmsg, sizeof(kmsg), sizeof(kmsg));
    if (rv < 0) {
        SOC_DEBUG_PRINT((DK_VERBOSE,
                         "knet init failed\n"));
        soc_knet_cleanup();
        return SOC_E_FAIL;
    }

    /* Register command response handler */
    soc_knet_rx_register(soc_knet_handle_cmd_resp, ctrl, 0);

    /* Start message handler */
    knet_rx_thread_run = 1;
    rxthr = sal_thread_create("SOC KNET RX", 0, 0, soc_knet_rx_thread, NULL);
    if (rxthr == NULL) {
        soc_cm_print("knet rx thread create failed\n");
        soc_knet_cleanup();
        return SOC_E_FAIL;
    }

    rv = soc_knet_check_version(unit);
    if (SOC_FAILURE(rv)) {
        soc_cm_print("knet version check failed\n");
        soc_knet_cleanup();
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}

#endif /* INCLUDE_KNET */

int _soc_knet_not_empty;
