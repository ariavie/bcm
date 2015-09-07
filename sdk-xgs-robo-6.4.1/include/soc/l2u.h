/*
 * $Id: l2u.h,v 1.11 Broadcom SDK $
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
 * File:        l2u.h
 * Purpose:     XGS3 L2 User table manipulation support
 */

#ifndef _L2U_H_
#define _L2U_H_

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
/* Provide a more reasonably named type from auto-generated type */
#define l2u_entry_t l2_user_entry_entry_t

/* More convenience */
#define _l2u_field_get soc_L2_USER_ENTRYm_field_get
#define _l2u_field_set soc_L2_USER_ENTRYm_field_set
#define _l2u_field32_get soc_L2_USER_ENTRYm_field32_get
#define _l2u_field32_set soc_L2_USER_ENTRYm_field32_set
#define _l2u_mac_addr_get soc_L2_USER_ENTRYm_mac_addr_get
#define _l2u_mac_addr_set soc_L2_USER_ENTRYm_mac_addr_set

/* Maximum number of BPDU addresses */
#define L2U_BPDU_COUNT 6

/* Valid fields in search key */
#define L2U_KEY_MAC             0x00000001
#define L2U_KEY_VLAN            0x00000002
#define L2U_KEY_PORT            0x00000004
#define L2U_KEY_MODID           0x00000008
#define L2U_KEY_MAC_MASK        0x00000010
#define L2U_KEY_VLAN_MASK       0x00000020

typedef struct l2u_key_s {
    uint32      flags;
    sal_mac_addr_t mac;
    int         vlan;
    int         port;
    int         modid;
    sal_mac_addr_t mac_mask;
    int         vlan_mask;
} l2u_key_t;

extern int soc_l2u_overlap_check(int unit, l2u_entry_t *entry, int *index);
extern int soc_l2u_search(int unit, l2u_entry_t *key, l2u_entry_t *result, int *index);
extern int soc_l2u_find_free_entry(int unit, l2u_entry_t *key, int *free_index);
extern int soc_l2u_insert(int unit, l2u_entry_t *entry, int index, int *index_used);
extern int soc_l2u_get(int unit, l2u_entry_t *entry, int index);
extern int soc_l2u_delete(int unit, l2u_entry_t *entry, int index, int *index_deleted);
extern int soc_l2u_delete_by_key(int unit, l2u_key_t *key);
extern int soc_l2u_delete_all(int unit);
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
#endif /* _L2U_H_ */
