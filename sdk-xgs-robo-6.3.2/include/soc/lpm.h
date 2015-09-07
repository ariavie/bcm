/*
 * $Id: lpm.h 1.16 Broadcom SDK $
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

#ifndef _LPM_H_
#define _LPM_H_
#include <shared/l3.h>

/* 
 *  Reserved VRF values 
 */
#define SOC_L3_VRF_OVERRIDE  _SHR_L3_VRF_OVERRIDE /* Matches before vrf */
                                                  /* specific entries.  */
#define SOC_L3_VRF_GLOBAL    _SHR_L3_VRF_GLOBAL   /* Matches after vrf  */
                                                  /* specific entries.  */
#define SOC_L3_VRF_DEFAULT   _SHR_L3_VRF_DEFAULT  /* Default vrf id.    */ 

#define SOC_APOLLO_L3_DEFIP_URPF_SIZE (0x800)
#define SOC_APOLLO_B0_L3_DEFIP_URPF_SIZE (0xC00)

#define SOC_LPM_LOCK(u)             soc_mem_lock(u, L3_DEFIPm)
#define SOC_LPM_UNLOCK(u)           soc_mem_unlock(u, L3_DEFIPm)

#define PRESERVE_HIT                TRUE 

#define FB_LPM_HASH_INDEX_NULL  (0xFFFF)
#define FB_LPM_HASH_IPV6_MASK   (0x8000)

#define _SOC_HASH_L3_VRF_GLOBAL   0
#define _SOC_HASH_L3_VRF_OVERRIDE 1
#define _SOC_HASH_L3_VRF_DEFAULT  2
#define _SOC_HASH_L3_VRF_SPECIFIC 3

#ifdef BCM_FIREBOLT_SUPPORT
#define IPV6_PFX_ZERO                33
#define IPV4_PFX_ZERO                (3 * (64 + 32 + 2 + 1)) 
#define MAX_PFX_ENTRIES              (2 * (IPV4_PFX_ZERO))
#define MAX_PFX_INDEX                (MAX_PFX_ENTRIES - 1)
#define SOC_LPM_INIT_CHECK(u)        (soc_lpm_state[(u)] != NULL)
#define SOC_LPM_STATE(u)             (soc_lpm_state[(u)])
#define SOC_LPM_STATE_START(u, pfx)  (soc_lpm_state[(u)][(pfx)].start)
#define SOC_LPM_STATE_END(u, pfx)    (soc_lpm_state[(u)][(pfx)].end)
#define SOC_LPM_STATE_PREV(u, pfx)  (soc_lpm_state[(u)][(pfx)].prev)
#define SOC_LPM_STATE_NEXT(u, pfx)  (soc_lpm_state[(u)][(pfx)].next)
#define SOC_LPM_STATE_VENT(u, pfx)  (soc_lpm_state[(u)][(pfx)].vent)
#define SOC_LPM_STATE_FENT(u, pfx)  (soc_lpm_state[(u)][(pfx)].fent)
#define SOC_LPM_PFX_IS_V4(u, pfx)   (pfx) >= IPV4_PFX_ZERO ? TRUE : FALSE  
#define SOC_LPM_PFX_IS_V6_64(u, pfx) (pfx) < IPV4_PFX_ZERO  ? TRUE : FALSE  

typedef struct soc_lpm_state_s {
    int start;  /* start index for this prefix length */
    int end;    /* End index for this prefix length */
    int prev;   /* Previous (Lo to Hi) prefix length with non zero entry count*/
    int next;   /* Next (Hi to Lo) prefix length with non zero entry count */
    int vent;   /* valid entries */
    int fent;   /* free entries */
} soc_lpm_state_t, *soc_lpm_state_p;
extern soc_lpm_state_p soc_lpm_state[SOC_MAX_NUM_DEVICES];

extern int soc_fb_lpm_init(int u);
extern int soc_fb_lpm_deinit(int u);
extern int soc_fb_lpm_insert(int unit, void *entry);
extern int soc_fb_lpm_delete(int u, void *key_data);
extern int soc_fb_lpm_match(int u, void *key_data, void *e, int *index_ptr);
extern int soc_fb_lpm_ipv4_delete_index(int u, int index);
extern int soc_fb_lpm_ipv6_delete_index(int u, int index);
extern int soc_fb_lpm_ip4entry0_to_0(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_ip4entry1_to_1(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_ip4entry0_to_1(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_ip4entry1_to_0(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_vrf_get(int unit, void *lpm_entry, int *vrf);

#ifdef BCM_KATANA_SUPPORT

typedef struct soc_kt_lpm_ipv6_cfg_s {
    uint32 sip_start_offset;
    uint32 dip_start_offset;
    uint32 depth;
} soc_kt_lpm_ipv6_cfg_t;

typedef struct soc_kt_lpm_info_s {
    soc_kt_lpm_ipv6_cfg_t ipv6_128b;
    soc_kt_lpm_ipv6_cfg_t ipv6_64b;
} soc_kt_lpm_ipv6_info_t;

extern soc_kt_lpm_ipv6_info_t *soc_kt_lpm_ipv6_info_get(int unit);
#endif
#ifdef BCM_WARM_BOOT_SUPPORT
extern int soc_fb_lpm_reinit(int unit, int idx, defip_entry_t *lpm_entry);
extern int soc_fb_lpm_reinit_done(int unit);
#else
#define soc_fb_lpm_reinit(u, t, s, c) (SOC_E_UNAVAIL)
#endif /* BCM_WARM_BOOT_SUPPORT */
  	 
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void soc_fb_lpm_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
#endif	/* BCM_FIREBOLT_SUPPORT */

#endif	/* !_LPM_H_ */
