/*
 * $Id: ipmc.h,v 1.25 Broadcom SDK $
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
 * This file contains IPMC definitions internal to the BCM library.
 */

#ifndef _BCM_INT_IPMC_H
#define _BCM_INT_IPMC_H

#include <bcm/types.h>
#include <bcm/ipmc.h>

#ifdef	INCLUDE_L3

typedef struct _bcm_esw_ipmc_l3entry_info_s
{
    uint32        flags;        /* See <bcm/l3.h> BCM_L3_* */
    bcm_vrf_t     vrf;          /* Virtual Router Instance (EZ only) */
    bcm_ip_t      ip_addr;      /* IPv4 address */
    bcm_ip_t      src_ip_addr;  /* Source IPv4 address (for IPMC) */
    bcm_ip6_t     ip6;          /* IPv6 address */
    bcm_ip6_t     sip6;         /* IPv6 Source address (for IPMC) */
    bcm_vlan_t    vid;          /* VLAN ID (for IPMC 5695 only) */
    int           prio;         /* Priority */
    int           ipmc_ptr;     /* 5690 index to IPMC table */
    int           lookup_class; /* Classification lookup class id. */
    int           rp_id;        /* Rendezvous point ID */
#define ipmc_group   ip_addr
} _bcm_esw_ipmc_l3entry_info_t;

typedef struct _bcm_esw_ipmc_l3entry_s
{
    int l3index;
    int ip6;
    _bcm_esw_ipmc_l3entry_info_t l3info;
    struct _bcm_esw_ipmc_l3entry_s *next;
} _bcm_esw_ipmc_l3entry_t;

typedef struct _bcm_esw_ipmc_group_info_s
{
    int ref_count; /* Reference count is incremented when a L3 entry pointing
                      to this IPMC group is added, or when bcm_multicast_create
                      allocates this IPMC index. */
    _bcm_esw_ipmc_l3entry_t *l3entry_list; /* Linked list of L3 entries 
                                               sharing this IPMC group */
} _bcm_esw_ipmc_group_info_t;

typedef struct _bcm_esw_ipmc_s
{
    int ipmc_initialized;
    int ipmc_size;
    int ipmc_count;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    _bcm_esw_ipmc_group_info_t *ipmc_group_info;
    int ipmc_valid_as_hit; /* L3_IPMC "valid" field is used as hit-bit (ER)*/
#endif /* XGS3 devices */
#if defined(BCM_XGS12_SWITCH_SUPPORT)
    SHR_BITDCL  *ipmc_allocated; 
    int         *ipmc_l3_index;
#endif /* XGS1 or XGS2 devices */
} _bcm_esw_ipmc_t;


/* IPMC info structure to be used by multiple source files */
extern _bcm_esw_ipmc_t esw_ipmc_info[BCM_MAX_NUM_UNITS];

/* MACROS to manipulate IPMC info structure */
#define IPMC_GROUP_INFO(unit, n) \
        (&esw_ipmc_info[unit].ipmc_group_info[n])

#define IPMC_USED_SET(unit, n)                                   \
        {                                                        \
            if (esw_ipmc_info[unit].ipmc_group_info[n].ref_count == 0) { \
                esw_ipmc_info[unit].ipmc_count++;               \
            }                                                    \
            esw_ipmc_info[unit].ipmc_group_info[n].ref_count++; \
        }

#define IPMC_USED_CLR(unit, n)                                   \
        {                                                        \
            esw_ipmc_info[unit].ipmc_group_info[n].ref_count--; \
            if (esw_ipmc_info[unit].ipmc_group_info[n].ref_count == 0) { \
                esw_ipmc_info[unit].ipmc_count--;               \
            }                                                    \
        }

#define IPMC_USED_ISSET(unit, n) \
        (esw_ipmc_info[unit].ipmc_group_info[n].ref_count > 0)

#define IPMC_USED_ONE(unit, n)                                           \
        {                                                                \
            if (esw_ipmc_info[unit].ipmc_group_info[n].ref_count != 0) { \
                esw_ipmc_info[unit].ipmc_group_info[n].ref_count = 1;    \
            }                                                            \
        }

#define IPMC_ID(unit, id) \
        if ((id < 0) || (id >= esw_ipmc_info[unit].ipmc_size)) \
            { return BCM_E_PARAM; }

#define IPMC_INIT(unit) \
        if (!soc_feature(unit, soc_feature_ip_mcast)) { \
            return BCM_E_UNAVAIL; \
        } else if (!esw_ipmc_info[unit].ipmc_initialized) { \
            return BCM_E_INIT; \
        }

#define IPMC_INFO(unit)      (&esw_ipmc_info[unit])
#define IPMC_GROUP_NUM(unit) (esw_ipmc_info[unit].ipmc_size)

#define IPMC_LOCK(unit) \
        soc_mem_lock(unit, L3_IPMCm)
#define IPMC_UNLOCK(unit) \
        soc_mem_unlock(unit, L3_IPMCm)

#define IPMC_VALID_AS_HIT(unit) (esw_ipmc_info[unit].ipmc_valid_as_hit)

extern int _bcm_esw_ipmc_stk_update(int unit, uint32 flags);

typedef struct _bcm_repl_list_info_s {
    int index;
    int hash;
    int list_size;
    int refcount;
    struct _bcm_repl_list_info_s *next;
} _bcm_repl_list_info_t;

#define	_BCM_VLAN_VEC_HASH(vec)	        _shr_crc32b(0, (uint8 *)vec, \
                                                    BCM_VLAN_COUNT)

extern int _bcm_esw_ipmc_egress_intf_set(int unit, int mc_index, 
                                         bcm_port_t port, int if_count, 
                                         bcm_if_t *if_array, int is_l3,
                                         int check_port);
extern int bcm_esw_ipmc_egress_intf_get(int unit, int mc_index,
                                        bcm_port_t port, int if_max,
                                        bcm_if_t *if_array, int *if_count);
extern int bcm_esw_ipmc_get_by_index(int unit, int index,
                                     bcm_ipmc_addr_t *data);

#ifdef BCM_WARM_BOOT_SUPPORT
#define _BCM_IPMC_WB_REPL_LIST                  0x1
#define _BCM_IPMC_WB_MULTICAST_MODE             0x2
#define _BCM_IPMC_WB_IPMC_GROUP_TYPE_MULTICAST  0x4
#define _BCM_IPMC_WB_L2MC_GROUP_TYPE_MULTICAST  0x8
#define _BCM_IPMC_WB_FLAGS_ALL                  0xf

extern int _bcm_esw_ipmc_repl_wb_flags_set(int unit, uint8 flags,
                                           uint8 flags_mask);
extern int _bcm_esw_ipmc_repl_wb_flags_get(int unit, uint8 flags_mask,
                                           uint8 *flags);
extern int _bcm_esw_ipmc_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_ipmc_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int _bcm_esw_ipmc_idx_ret_type_set(int unit, int arg);
extern int _bcm_esw_ipmc_idx_ret_type_get(int unit, int *arg);
extern int _bcm_esw_ipmc_repl_threshold_set(int unit, int arg);
extern int _bcm_esw_ipmc_repl_threshold_get(int unit, int *arg);

extern bcm_error_t
_bcm_esw_ipmc_stat_counter_get(int              unit,
                               int              sync_mode,
                               bcm_ipmc_addr_t  *info,
                               bcm_ipmc_stat_t  stat,
                               uint32           num_entries,
                               uint32           *counter_indexes,
                               bcm_stat_value_t *counter_values);
#else
#define	_bcm_esw_ipmc_stk_update(_u,_f)	BCM_E_NONE
#endif	/* INCLUDE_L3 */

#endif	/* !_BCM_INT_IPMC_H */
