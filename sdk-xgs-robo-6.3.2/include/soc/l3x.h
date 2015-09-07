/*
 * $Id: l3x.h 1.16 Broadcom SDK $
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
 * File:        l3x.h
 * Purpose:     Draco L3X hardware table manipulation support
 */

#ifndef _L3X_H_
#define _L3X_H_

#include <soc/macipadr.h>
#include <soc/hash.h>

#ifdef BCM_XGS_SWITCH_SUPPORT

#if defined(INCLUDE_L3)
#define SOC_MEM_L3X_ENTRY_VALID             (1 << 0)
#define SOC_MEM_L3X_ENTRY_MOVED             (1 << 1)
#define SOC_MEM_L3X_ENTRY_MOVE_FAIL         (1 << 2)

/*
 *  Dual hash l3x entry info.
 */
typedef struct soc_mem_l3x_entry_info_s {
    int index;       /* Entry start index in the bucket.    */
    int size;        /* Entry size - number of base memory  */
                     /* slots entry occupies.               */
    soc_mem_t mem;   /* View name.                          */
    uint8 flags;     /* Entry flags - valid/move attempted. */
} soc_mem_l3x_entry_info_t;

/*
 *  Dual hash l3x half bucket entries map.
 */
typedef struct soc_mem_l3x_bucket_map_s {
    int size;          /* Bucket map size  (#valid entries).         */            
    int valid_count;   /* Count of valid base entries in the bucket. */
    int total_count;   /* Total number of base entries in the bucket.*/
    soc_mem_t base_mem;/* Base memory/view name.                     */
    soc_mem_t base_idx;/* Bucket start index in base memory.         */
    soc_mem_l3x_entry_info_t *entry_arr;
                      /* Bucket entries information  array.          */
} soc_mem_l3x_bucket_map_t;
#endif /* INCLUDE_L3 */

extern int soc_l3x_init(int unit);

/* These defines should be replaced with unit-based accessors someday */
#define SOC_L3X_MAX_BUCKET_SIZE  16
#define SOC_L3X_BUCKET_SIZE(_u)  ((SOC_IS_RAVEN(_u) || SOC_IS_TR_VL(_u)) ? 16 : 8)
#define SOC_L3X_IP_MULTICAST(i)	 (((uint32)(i) & 0xf0000000) == 0xe0000000)
extern int soc_l3x_entries(int unit);	/* Warning: very slow */

extern int soc_l3x_lock(int unit);
extern int soc_l3x_unlock(int unit);

/*
 * L3 software-based sanity routines
 */

#endif	/* BCM_XGS_SWITCH_SUPPORT */

#ifdef	BCM_FIREBOLT_SUPPORT

extern int soc_fb_l3x_bank_insert(int unit, uint8 banks,
                                  l3_entry_ipv6_multicast_entry_t *entry);
extern int soc_fb_l3x_bank_delete(int unit, uint8 banks,
                                  l3_entry_ipv6_multicast_entry_t *entry);
extern int soc_fb_l3x_bank_lookup(int unit, uint8 banks,
                                  l3_entry_ipv6_multicast_entry_t *key,
                                  l3_entry_ipv6_multicast_entry_t *result,
                                  int *index_ptr);

extern int soc_fb_l3x_insert(int unit, l3_entry_ipv6_multicast_entry_t *entry);
extern int soc_fb_l3x_delete(int unit, l3_entry_ipv6_multicast_entry_t *entry);
extern int soc_fb_l3x_lookup(int unit, l3_entry_ipv6_multicast_entry_t *key,
                             l3_entry_ipv6_multicast_entry_t *result,
                             int *index_ptr);

/*
 * We must use SOC_MEM_INFO here, because we may redefine the
 * index_max due to configuration, but the SCHAN msg uses the maximum size.
 */
#define SOC_L3X_PERR_PBM_POS(unit, mem) \
        ((_shr_popcount(SOC_MEM_INFO(unit, mem).index_max) + \
            soc_mem_entry_bits(unit, mem)) %32)
#endif	/* BCM_FIREBOLT_SUPPORT */

#ifdef	BCM_EASYRIDER_SUPPORT

#define SOC_ER_L3V6_BUCKET_SIZE	 4
#define soc_er_l3v4_insert(unit, entry) \
        soc_mem_insert((unit),  L3_ENTRY_V4m, COPYNO_ALL, (entry))
#define soc_er_l3v6_insert(unit, entry) \
        soc_mem_insert((unit),  L3_ENTRY_V6m, COPYNO_ALL, (entry))
#define soc_er_l3v4_delete(unit, entry) \
        soc_mem_delete((unit),  L3_ENTRY_V4m, COPYNO_ALL, (entry))
#define soc_er_l3v6_delete(unit, entry) \
        soc_mem_delete((unit),  L3_ENTRY_V6m, COPYNO_ALL, (entry))
extern int soc_er_l3v4_lookup(int unit, l3_entry_v4_entry_t *key,
                              l3_entry_v4_entry_t *result, int *index_ptr);
extern int soc_er_l3v6_lookup(int unit, l3_entry_v6_entry_t *key,
                              l3_entry_v6_entry_t *result, int *index_ptr);

#endif	/* BCM_EASYRIDER_SUPPORT */

#endif	/* !_L3X_H_ */
