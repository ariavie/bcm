/*
 * $Id: mbox.h 1.2 Broadcom SDK $
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
 * Shared Memory Mailbox infrastruction - mbox communications & debug logging
 */

#ifndef   _BCM_COMMON_MBOX_H_
#define   _BCM_COMMON_MBOX_H_

#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/core/sync.h>

#define _BCM_MBOX_MAX_BUFFERS        (16)
#define _BCM_MBOX_MAX_LOG            (1024)

#define _BCM_MBOX_SDK_VERSION         (0x01000000)
#define _BCM_MBOX_UC_MIN_VERSION      (0x01000000)

#define _BCM_MBOX_RESPONSE_TIMEOUT_US (10000000)  /* 1 sec */

typedef enum _bcm_mbox_transport_type_e {
    _BCM_MBOX_MESSAGE,
    _BCM_MBOX_TUNNEL_TO,
    _BCM_MBOX_TUNNEL_OUT
} _bcm_mbox_transport_type_t;

typedef enum _bcm_mbox_mbox_status_e {
    _BCM_MBOX_MS_EMPTY,
    _BCM_MBOX_MS_CMD,
    _BCM_MBOX_MS_TUNNEL_TO,
    _BCM_MBOX_MS_TUNNEL_OUT,
    _BCM_MBOX_MS_RESP,
    _BCM_MBOX_MS_EVENT,
    _BCM_MBOX_MS_TUNNEL_IN,
    _BCM_MBOX_MS_PENDING
} _bcm_mbox_mailbox_status_t;


typedef struct _bcm_bs_internal_stack_mbox_s {
    volatile uint32 node_num;  /* clock number that the message is addressed to/from */
    volatile uint32 data_len;
    volatile uint8 data[1536];
} _bcm_bs_internal_stack_mbox_t;

typedef struct _bcm_bs_internal_stack_mboxes_s {
    volatile uint32 num_buffers;                     /* == BCM_MBOX_MAX_BUFFERS */
    volatile uint32 status[_BCM_MBOX_MAX_BUFFERS];     /* MBOX_STATUS_x value */
    volatile _bcm_bs_internal_stack_mbox_t mbox[_BCM_MBOX_MAX_BUFFERS]; /* contents of mbox */
} _bcm_bs_internal_stack_mboxes_t;

typedef struct _bcm_bs_internal_stack_log_s {
    volatile uint32 size;                         /* allocated size of buf (below)              */
    volatile uint32 head;                         /* host-read, updated by CMICm                */
    uint32 tail;                                  /* host-read/write.  not updated by CMICm     */
    volatile uint8 buf[4];                        /* '4' is placeholder.  Actual size is 'size' */
} _bcm_bs_internal_stack_log_t;

typedef struct _bcm_bs_internal_stack_state_s {
    /* network-byte-ordered pointers to the shared structures */
    volatile uint32 mbox_ptr;
    volatile uint32 log_ptr;

    _bcm_bs_internal_stack_mboxes_t * mboxes;
    _bcm_bs_internal_stack_log_t * log;

    /* Elements after this point are not accessed by firmware */
    int core_num;
    int is_running;

    sal_sem_t response_ready;
    uint8 *response_data;
    uint32 response_len;
} _bcm_bs_internal_stack_state_t;

typedef struct _bcm_bs_internal_comm_info_s {
    _bcm_bs_internal_stack_state_t *unit_state;

    sal_sem_t comm_available;
} _bcm_bs_internal_comm_info_t;



int _bcm_mbox_comm_init(int unit, int appl_type);

int _bcm_mbox_txrx(int unit, uint32 node_num, _bcm_mbox_transport_type_t transport,
                       uint8 *out_data, int out_len, uint8 *in_data, int *in_len);

int _bcm_mbox_debug_flag_set(uint32 flags);
int _bcm_mbox_debug_flag_get(uint32 *flags);


#endif /* _BCM_COMMON_MBOX_H_ */
