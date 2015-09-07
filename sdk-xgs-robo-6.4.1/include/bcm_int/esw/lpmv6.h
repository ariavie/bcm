/*
 * $Id: 864272108af8ca144ef0244d94a464e721eef224 $
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
 * File:        lpmv6.h
 * Purpose:     Function declarations for LPMV6 Internal functions.
 */

#ifndef _BCM_INT_LPMV6_H
#define _BCM_INT_LPMV6_H

#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/l3.h>

#define _BCM_DEFIP_TCAM_DEPTH    1024
#define _BCM_DEFIP_TCAM_NUM      8

/* L3 LPM state */
#define BCM_DEFIP_PAIR128_HASH_SZ             (0x6)
#define BCM_DEFIP_PAIR128(_unit_)             l3_defip_pair128[(_unit_)]
#define BCM_DEFIP_PAIR128_ARR(_unit_)         BCM_DEFIP_PAIR128((_unit_))->entry_array
#define BCM_DEFIP_PAIR128_IDX_MAX(_unit_)     BCM_DEFIP_PAIR128((_unit_))->idx_max
#define BCM_DEFIP_PAIR128_USED_COUNT(_unit_)  BCM_DEFIP_PAIR128((_unit_))->used_count
#define BCM_DEFIP_PAIR128_TOTAL(_unit_)       BCM_DEFIP_PAIR128((_unit_))->total_count
#define BCM_DEFIP_PAIR128_URPF_OFFSET(_unit_) BCM_DEFIP_PAIR128((_unit_))->urpf_offset
#define BCM_DEFIP_PAIR128_ENTRY_SET(_unit_, _idx_, _plen_, _hash_)          \
            BCM_DEFIP_PAIR128_ARR((_unit_))[(_idx_)].prefix_len = (_plen_); \
            BCM_DEFIP_PAIR128_ARR((_unit_))[(_idx_)].entry_hash = (_hash_)

typedef struct _bcm_defip_pair128_entry_s {
    uint16 prefix_len; /* Route entry  prefix length.*/
    uint16 entry_hash; /* Route entry key hash.      */
} _bcm_defip_pair128_entry_t;

typedef struct _bcm_defip_pair128_table_s {
    _bcm_defip_pair128_entry_t *entry_array; /* Cam entries array.      */
    uint16 idx_max;                   /* Last cam entry index.             */
    uint16 used_count;                /* Used cam entry count.             */
    uint16 total_count;               /* Total number of available entries.*/
    uint16 urpf_offset;               /* Src lookup start offset.          */
} _bcm_defip_pair128_table_t;

#if defined(INCLUDE_L3)

/* LPM */
extern int _bcm_l3_lpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, 
                           int *nh_ecmp_idx);
extern int _bcm_l3_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, 
                           int nh_ecmp_idx);
extern int _bcm_l3_lpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg);
extern int _bcm_l3_lpm_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data);
extern int _bcm_l3_defip_init(int unit);
extern int _bcm_l3_defip_deinit(int unit);
extern int _bcm_l3_defip_urpf_enable(int unit, int enable);
extern int _bcm_defip_pair128_deinit(int unit);
extern int _bcm_l3_defip_pair128_get(int unit, _bcm_defip_cfg_t *lpm_cfg,
                                     int *nh_ecmp_idx);
extern int _bcm_l3_defip_pair128_add(int unit, _bcm_defip_cfg_t *lpm_cfg,
                                     int nh_ecmp_idx);
extern int _bcm_l3_defip_pair128_del(int unit, _bcm_defip_cfg_t *lpm_cfg);
extern int _bcm_l3_defip_pair128_update_match(int unit, 
                                              _bcm_l3_trvrs_data_t *trv_data);
extern int _bcm_l3_scaled_lpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg,
                                  int *nh_ecmp_idx);
extern int _bcm_l3_scaled_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg,
                                  int nh_ecmp_idx);
extern int _bcm_l3_scaled_lpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg);
extern int _bcm_l3_lpm128_ripple_entries(int unit);
extern int _bcm_l3_lpm_ripple_entries(int unit, _bcm_defip_cfg_t *lpm_cfg,
                                      int new_nh_ecmp_idx);
extern int _bcm_l3_scaled_lpm_update_match(int unit, 
                                           _bcm_l3_trvrs_data_t *trv_data);
extern int _bcm_defip_pair128_field_cache_init(int unit);

#endif /* INCLUDE_L3 */
#endif
