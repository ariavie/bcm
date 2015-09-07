/*
 * $Id: ipfix.h,v 1.11 Broadcom SDK $
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
 */

#ifndef _BCM_INT_IPFIX_H
#define _BCM_INT_IPFIX_H

#include <bcm/types.h>
#include <soc/profile_mem.h>

#define _BCM_IPFIX_CB_MAX 3

/* Index into array of profile structures */
typedef enum _bcm_ipfix_profile_id_e {
    _BCM_IPFIX_PROFILE_ID_INVALID = -1,
    _BCM_IPFIX_PROFILE_ID_ING_DSCP_XLATE_TABLE = 0,
    _BCM_IPFIX_PROFILE_ID_ING_IPV4_MASK_SET_A,
    _BCM_IPFIX_PROFILE_ID_ING_IPV4_MASK_SET_B,
    _BCM_IPFIX_PROFILE_ID_ING_IPV6_MASK_SET_A,
    _BCM_IPFIX_PROFILE_ID_ING_IPV6_MASK_SET_B,
    _BCM_IPFIX_PROFILE_ID_EGR_DSCP_XLATE_TABLE,
    _BCM_IPFIX_PROFILE_ID_EGR_IPV4_MASK_SET_A,
    _BCM_IPFIX_PROFILE_ID_EGR_IPV6_MASK_SET_A,
    _BCM_IPFIX_PROFILE_ID_ING_PORT_RECORD_LIMIT_SET,
    _BCM_IPFIX_PROFILE_ID_ING_MAXIMUM_LIVE_TIME_SET,
    _BCM_IPFIX_PROFILE_ID_ING_MINIMUM_LIVE_TIME_SET,
    _BCM_IPFIX_PROFILE_ID_ING_MAXIMUM_IDLE_AGE_SET,
    _BCM_IPFIX_PROFILE_ID_ING_SAMPLING_LIMIT_SET,
    _BCM_IPFIX_PROFILE_ID_EGR_PORT_RECORD_LIMIT_SET,
    _BCM_IPFIX_PROFILE_ID_EGR_MAXIMUM_LIVE_TIME_SET,
    _BCM_IPFIX_PROFILE_ID_EGR_MINIMUM_LIVE_TIME_SET,
    _BCM_IPFIX_PROFILE_ID_EGR_MAXIMUM_IDLE_AGE_SET,
    _BCM_IPFIX_PROFILE_ID_EGR_SAMPLING_LIMIT_SET,
    _BCM_IPFIX_PROFILE_ID_COUNT
} _bcm_ipfix_profile_id_t;

typedef enum _bcm_ipfix_profile_type_e {
    _BCM_IPFIX_PROFILE_TYPE_NONE = 0,
    _BCM_IPFIX_PROFILE_TYPE_REG,
    _BCM_IPFIX_PROFILE_TYPE_MEM
} _bcm_ipfix_profile_type_t;

typedef struct _bcm_ipfix_profile_def_s {
    _bcm_ipfix_profile_type_t type;
    soc_field_t index_field;  /* associated field id in ING_IPFIX_PROFILEm */
    struct {
        _bcm_ipfix_profile_id_t profile_id;
        int reg_mem_id[2];    /* soc_reg_t or soc_mem_t */
        int entry_words[2];   /* word size for memory entry */
    } stage[2];               /* one for ingress, one for egress */
} _bcm_ipfix_profile_def_t;

typedef struct _bcm_ipfix_id_def_s {
    soc_reg_t port_config[2];
    soc_mem_t profile[2];
    soc_mem_t export_fifo[2];
    soc_reg_t age_control[2];
    soc_reg_t mirror_control[2];
    soc_field_t mtp_index[4];
} _bcm_ipfix_id_def_t;

typedef struct _bcm_ipfix_flags_def_s {
    uint32 bit_mask;
    soc_field_t field;
} _bcm_ipfix_flags_def_t;

typedef enum _bcm_ipfix_callback_state_e {
    _BCM_IPFIX_CALLBACK_STATE_INVALID = 0,
    _BCM_IPFIX_CALLBACK_STATE_ACTIVE,
    _BCM_IPFIX_CALLBACK_STATE_REGISTERED,
    _BCM_IPFIX_CALLBACK_STATE_UNREGISTERED
} _bcm_ipfix_callback_state_t;

typedef struct _bcm_ipfix_callback_entry_s {
    VOL _bcm_ipfix_callback_state_t state;
    bcm_ipfix_callback_t callback;
    void *userdata;
} _bcm_ipfix_callback_entry_t;

typedef struct _bcm_ipfix_ctrl_s {
    /* per unit profiles array is indexed by _bcm_ipfix_profile_id_t */
    union {
        soc_profile_reg_t reg;
        soc_profile_mem_t mem;
    } profiles[_BCM_IPFIX_PROFILE_ID_COUNT];
    SHR_BITDCL *rate_used_bmp;
    int rate_used_count;
    _bcm_ipfix_callback_entry_t callback_entry[_BCM_IPFIX_CB_MAX];
    sal_thread_t pid;          /* export fifo processing thread process id */
    VOL sal_usecs_t interval;  /* export fifo processing polling interval */
    int dma_chan[2];           /* export fifo DMA channel number */
} _bcm_ipfix_ctrl_t;

#define _BCM_IPFIX_RATE_MIRROR_COUNT 4

extern void bcm_ipfix_dump_export_entry(int unit, bcm_ipfix_data_t *data);
extern int bcm_ipfix_export_fifo_control(int unit, sal_usecs_t interval);
extern int _bcm_tr_ipfix_rate_id_check(int unit, bcm_ipfix_rate_id_t rate_id);
extern int _bcm_esw_ipfix_deinit(int unit);

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_ipfix_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_ipfix_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#endif /* !_BCM_INT_IPFIX_H */
