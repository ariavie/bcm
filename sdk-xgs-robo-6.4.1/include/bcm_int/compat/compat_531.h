/*
 * $Id: compat_531.h,v 1.1 Broadcom SDK $
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
 * RPC Compatibility with sdk-5.3.1 routines
 */

#ifndef _COMPAT_531_H_
#define _COMPAT_531_H_

#ifdef	BCM_RPC_SUPPORT

#include <bcm/types.h>

typedef struct bcm_compat531_l2_cache_addr_s {
    uint32              flags;          /* BCM_L2_CACHE_XXX flags */
    bcm_mac_t           mac;            /* dest MAC address to match */
    bcm_mac_t           mac_mask;       /* mask */
    bcm_vlan_t          vlan;           /* VLAN to match */
    bcm_vlan_t          vlan_mask;      /* mask */
    bcm_port_t          src_port;       /* ingress port to match (BCM5660x) */
    bcm_port_t          src_port_mask;  /* mask (must be 0 if not BCM5660x) */
    bcm_module_t        dest_modid;     /* switch destination module */
    bcm_port_t          dest_port;      /* switch destination port */
    bcm_trunk_t         dest_trunk;     /* switch destination trunk ID */
    int                 prio;           /* -1 to not set internal priority */
} bcm_compat531_l2_cache_addr_t;

extern int bcm_compat531_l2_cache_set(
    int unit, 
    int index, 
    bcm_compat531_l2_cache_addr_t *addr,
    int *index_used);

extern int bcm_compat531_l2_cache_get(
    int unit,
    int index,
    bcm_compat531_l2_cache_addr_t *addr);

#endif	/* BCM_RPC_SUPPORT */

#endif	/* !_COMPAT_531_H */
