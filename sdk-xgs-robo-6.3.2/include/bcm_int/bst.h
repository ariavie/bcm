/*
 * $Id: bst.h 1.7 Broadcom SDK $
 *
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

#ifndef _BCM_BST_H
#define _BCM_BST_H

#include <soc/defs.h>
#include <soc/types.h>

typedef enum {
    _bcmResourceDevice  = 0,
    _bcmResourceEgrPool,
    _bcmResourceEgrMCastPool,
    _bcmResourceIngPool,
    _bcmResourcePortPool,
    _bcmResourcePriGroupShared,
    _bcmResourcePriGroupHeadroom,
    _bcmResourceUcast,
    _bcmResourceMcast,
    _bcmResourceMaxCount
} _bcm_bst_resource_t;

typedef enum _bcm_bst_cb_ret_e {
    _BCM_BST_RV_OK  = 0,
    _BCM_BST_RV_RETRY,
    _BCM_BST_RV_ERROR
} _bcm_bst_cb_ret_t;

/*
 * _bcm_bst_device_index_resolve callback is used if device
 * bst API uses the common implementation. This callback queries
 * the device implementation to resolve the gport, cosq to
 * bst resource, along with the index ranges. also device implementation
 * can return a reason code retry to call tresolve again if the
 * indexes are not contiguous or to map multiple resources.
 */
typedef _bcm_bst_cb_ret_t (*_bcm_bst_device_index_resolve)(int unit,
                                         bcm_gport_t gport, bcm_cos_queue_t cosq,
                                         uint32 flags, _bcm_bst_resource_t *res,
                                         int *pipe, int *from_hw_index, 
                                         int *to_hw_index, int *rcb_data, 
                                         void **user_data, int *bcm_rv);

typedef int (*_bcm_bst_device_index_map)(int unit,
            int res, int port, int index, bcm_gport_t *gport, bcm_cos_t *cosq);

typedef struct _bcm_bst_device_handlers_s {
    _bcm_bst_device_index_resolve resolve_index;
    _bcm_bst_device_index_map     reverse_resolve_index;
} _bcm_bst_device_handlers_t;

extern int _bcm_bst_attach(int unit, _bcm_bst_device_handlers_t *cbs);

extern int _bcm_bst_detach(int unit);

extern int
_bcm_bst_cmn_profile_set(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                         uint32 flags, bcm_cosq_bst_profile_t *profile);

extern int
_bcm_bst_cmn_profile_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                         uint32 flags, bcm_cosq_bst_profile_t *profile);

extern int _bcm_bst_cmn_control_set(int unit, bcm_switch_control_t type, int arg);

extern int _bcm_bst_cmn_control_get(int unit, bcm_switch_control_t type, int *arg);

extern int
_bcm_bst_cmn_stat_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                        uint32 flags, uint64 *pstat);

extern int
_bcm_bst_cmn_stat_traverse(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                           uint32 flags, bcm_cosq_bst_stat_traverse_cb cb, 
                           void *user_data);

#endif /* _BCM_BST_H */

