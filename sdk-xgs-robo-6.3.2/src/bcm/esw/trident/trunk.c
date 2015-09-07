/*
 * $Id: trunk.c 1.84 Broadcom SDK $
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
 * Trident Front Panel and Higig Trunking
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#ifdef BCM_TRIDENT_SUPPORT 

#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/bradley.h>

#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/link.h>   /* LAG failover support */
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/trident2.h>

#define _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT  16
#define _BCM_TD_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT 16

#ifdef BCM_KATANA_SUPPORT
#define _BCM_KATANA_FP_TRUNK_RTAG1_6_MAX_PORTCNT     8
#define _BCM_KATANA_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT 4
#endif

/* -------------------------------- */
/* Trunk membership info            */
/* -------------------------------- */
typedef struct {
    uint16 num_ports;
    uint16 *modport;
    uint32 *member_flags;
#ifdef BCM_WARM_BOOT_SUPPORT
    uint8 recovered_from_scache;
#endif /* BCM_WARM_BOOT_SUPPORT */
} _trident_member_info_t;

typedef struct {
    _trident_member_info_t *member_info;
} _trident_trunk_member_info_t;

STATIC _trident_trunk_member_info_t *_trident_trunk_member_info[BCM_MAX_NUM_UNITS];

#define MEMBER_INFO(_unit_, _tid_) (_trident_trunk_member_info[_unit_]->member_info[_tid_])

/* -------------------------------- */
/* Module and port to trunk mapping */
/* -------------------------------- */
STATIC uint16 *_trident_mod_port_to_tid_map[BCM_MAX_NUM_UNITS];

/* ----------------------------------------------------------------- */
/* Bookkeeping info on which entries of trunk member tables are used */
/* ----------------------------------------------------------------- */
typedef struct _trident_trunk_member_bookkeeping_s {
    SHR_BITDCL *fp_trunk_member_bitmap;
    SHR_BITDCL *hg_trunk_member_bitmap;
} _trident_trunk_member_bookkeeping_t;

STATIC _trident_trunk_member_bookkeeping_t *_trident_trunk_member_bk[BCM_MAX_NUM_UNITS];

#define TRUNK_MEMBER_INFO(_unit_) (_trident_trunk_member_bk[_unit_])

#define _BCM_FP_TRUNK_MEMBER_USED_GET(_u_, _idx_) \
    SHR_BITGET(TRUNK_MEMBER_INFO(_u_)->fp_trunk_member_bitmap, _idx_)
#define _BCM_FP_TRUNK_MEMBER_USED_SET(_u_, _idx_) \
    SHR_BITSET(TRUNK_MEMBER_INFO(_u_)->fp_trunk_member_bitmap, _idx_)
#define _BCM_FP_TRUNK_MEMBER_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(TRUNK_MEMBER_INFO(_u_)->fp_trunk_member_bitmap, _idx_)
#define _BCM_FP_TRUNK_MEMBER_USED_SET_RANGE(_u_, _idx_, _count_) \
    SHR_BITSET_RANGE(TRUNK_MEMBER_INFO(_u_)->fp_trunk_member_bitmap, _idx_, _count_)
#define _BCM_FP_TRUNK_MEMBER_USED_CLR_RANGE(_u_, _idx_, _count_) \
    SHR_BITCLR_RANGE(TRUNK_MEMBER_INFO(_u_)->fp_trunk_member_bitmap, _idx_, _count_)
#define _BCM_FP_TRUNK_MEMBER_TEST_RANGE(_u_, _idx_, _count_, _result_) \
    SHR_BITTEST_RANGE(TRUNK_MEMBER_INFO(_u_)->fp_trunk_member_bitmap, _idx_, _count_, _result_)
                                                                                
#define _BCM_HG_TRUNK_MEMBER_USED_GET(_u_, _idx_) \
    SHR_BITGET(TRUNK_MEMBER_INFO(_u_)->hg_trunk_member_bitmap, _idx_)
#define _BCM_HG_TRUNK_MEMBER_USED_SET(_u_, _idx_) \
    SHR_BITSET(TRUNK_MEMBER_INFO(_u_)->hg_trunk_member_bitmap, _idx_)
#define _BCM_HG_TRUNK_MEMBER_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(TRUNK_MEMBER_INFO(_u_)->hg_trunk_member_bitmap, _idx_)
#define _BCM_HG_TRUNK_MEMBER_USED_SET_RANGE(_u_, _idx_, _count_) \
    SHR_BITSET_RANGE(TRUNK_MEMBER_INFO(_u_)->hg_trunk_member_bitmap, _idx_, _count_)
#define _BCM_HG_TRUNK_MEMBER_USED_CLR_RANGE(_u_, _idx_, _count_) \
    SHR_BITCLR_RANGE(TRUNK_MEMBER_INFO(_u_)->hg_trunk_member_bitmap, _idx_, _count_)
#define _BCM_HG_TRUNK_MEMBER_TEST_RANGE(_u_, _idx_, _count_, _result_) \
    SHR_BITTEST_RANGE(TRUNK_MEMBER_INFO(_u_)->hg_trunk_member_bitmap, _idx_, _count_, _result_)

/* ------------------------------------------------- */
/* Bookkeeping info for Higig dynamic load balancing */
/* ------------------------------------------------- */
typedef struct _trident_hg_dlb_bookkeeping_s {
    /* Higig DLB parameters */
    SHR_BITDCL *hg_dlb_id_used_bitmap;
    SHR_BITDCL *hg_dlb_flowset_block_bitmap; /* Each block corresponds to
                                                512 entries */
#ifdef BCM_TRIUMPH3_SUPPORT
    SHR_BITDCL *hg_dlb_member_id_used_bitmap;
#endif /* BCM_TRIUMPH3_SUPPORT */
    int hg_dlb_sample_rate;
    int hg_dlb_tx_load_min_th;
    int hg_dlb_tx_load_max_th;
    int hg_dlb_qsize_min_th;
    int hg_dlb_qsize_max_th;
    uint8 *hg_dlb_load_weight;
    soc_profile_mem_t *hg_dlb_quality_map_profile;
#ifdef BCM_WARM_BOOT_SUPPORT
    uint8 recovered_from_scache; /* Indicates if the following members of this
                                    structure have been recovered from scached:
                                    hg_dlb_sample_rate,
                                    hg_dlb_tx_load_min_th,
                                    hg_dlb_tx_load_max_th,
                                    hg_dlb_tx_qsize_min_th,
                                    hg_dlb_tx_qsize_max_th,
                                    hg_dlb_load_weight */
#endif /* BCM_WARM_BOOT_SUPPORT */
} _trident_hg_dlb_bookkeeping_t;

STATIC _trident_hg_dlb_bookkeeping_t *_trident_hg_dlb_bk[BCM_MAX_NUM_UNITS];

#define HG_DLB_INFO(_unit_) (_trident_hg_dlb_bk[_unit_])

#define _BCM_HG_DLB_ID_USED_GET(_u_, _idx_) \
    SHR_BITGET(HG_DLB_INFO(_u_)->hg_dlb_id_used_bitmap, _idx_)
#define _BCM_HG_DLB_ID_USED_SET(_u_, _idx_) \
    SHR_BITSET(HG_DLB_INFO(_u_)->hg_dlb_id_used_bitmap, _idx_)
#define _BCM_HG_DLB_ID_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(HG_DLB_INFO(_u_)->hg_dlb_id_used_bitmap, _idx_)

#define _BCM_HG_DLB_FLOWSET_BLOCK_USED_GET(_u_, _idx_) \
    SHR_BITGET(HG_DLB_INFO(_u_)->hg_dlb_flowset_block_bitmap, _idx_)
#define _BCM_HG_DLB_FLOWSET_BLOCK_USED_SET(_u_, _idx_) \
    SHR_BITSET(HG_DLB_INFO(_u_)->hg_dlb_flowset_block_bitmap, _idx_)
#define _BCM_HG_DLB_FLOWSET_BLOCK_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(HG_DLB_INFO(_u_)->hg_dlb_flowset_block_bitmap, _idx_)
#define _BCM_HG_DLB_FLOWSET_BLOCK_USED_SET_RANGE(_u_, _idx_, _count_) \
    SHR_BITSET_RANGE(HG_DLB_INFO(_u_)->hg_dlb_flowset_block_bitmap, _idx_, _count_)
#define _BCM_HG_DLB_FLOWSET_BLOCK_USED_CLR_RANGE(_u_, _idx_, _count_) \
    SHR_BITCLR_RANGE(HG_DLB_INFO(_u_)->hg_dlb_flowset_block_bitmap, _idx_, _count_)
#define _BCM_HG_DLB_FLOWSET_BLOCK_TEST_RANGE(_u_, _idx_, _count_, _result_) \
    SHR_BITTEST_RANGE(HG_DLB_INFO(_u_)->hg_dlb_flowset_block_bitmap, _idx_, _count_, _result_)

#ifdef BCM_TRIUMPH3_SUPPORT
#define _BCM_HG_DLB_MEMBER_ID_USED_GET(_u_, _idx_) \
    SHR_BITGET(HG_DLB_INFO(_u_)->hg_dlb_member_id_used_bitmap, _idx_)
#define _BCM_HG_DLB_MEMBER_ID_USED_SET(_u_, _idx_) \
    SHR_BITSET(HG_DLB_INFO(_u_)->hg_dlb_member_id_used_bitmap, _idx_)
#define _BCM_HG_DLB_MEMBER_ID_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(HG_DLB_INFO(_u_)->hg_dlb_member_id_used_bitmap, _idx_)
#endif /* BCM_TRIUMPH3_SUPPORT */

/* ---------------------------- */
/* Higig trunk override profile */
/* ---------------------------- */
STATIC soc_profile_mem_t *_trident_hg_trunk_override_profile[BCM_MAX_NUM_UNITS];

/* ----------------------- */
/* Hardware Trunk Failover */
/* ----------------------- */

/* For the devices which support hardware trunk failover, the failover
 * configuration must be retained if it is one of the standard models 
 * for the retrieval function to use the same encoding as the set function.
 * 
 * Note: The trunk members stored in _trident_hw_tinfo_t's modport array do not 
 *       include those members with the BCM_TRUNK_MEMBER_EGRESS_DISABLE set.
 */

typedef struct {
    uint16        num_ports;
    uint8         *psc;
    uint16        *modport;
    uint32        *flags;
} _trident_hw_tinfo_t;

typedef struct {
    _trident_hw_tinfo_t *hw_tinfo;
} _trident_trunk_hwfail_t;

STATIC _trident_trunk_hwfail_t *_trident_trunk_hwfail[BCM_MAX_NUM_UNITS];

/* ----------------------- */
/* Software Trunk Failover */
/* ----------------------- */

/* Even without hardware failover support, given the high speed link
 * change notification possible with trident devices, a port that has been
 * indicated as link down can cause the trunk_group table to be immediately
 * updated with the link down port removed.  This works well for two cases:
 * (1) this is a higig trunk, or (2) the local device is the only device
 * in the system.
 * 
 * Note: The trunk members stored in _trident_tinfo_t's modport array do not 
 *       include those members with the BCM_TRUNK_MEMBER_EGRESS_DISABLE set.
 *       If a trunk member port is down, it'll be removed from_trident_tinfo_t's
 *       modport array by _bcm_trident_trunk_swfailover_trigger. 
 */
typedef struct {
    uint8         rtag;
    uint16        num_ports;
    uint16        *modport;
    uint32        *member_flags;
    uint32        flags;
} _trident_tinfo_t;

typedef struct {
    uint8         mymodid;
    bcm_trunk_t   trunk[SOC_MAX_NUM_PORTS]; /* If port is not a member of any
                                               trunk group, trunk = 0; else
                                               trunk = trunk id + 1. */
    _trident_tinfo_t *tinfo;
} _trident_trunk_swfail_t;

STATIC _trident_trunk_swfail_t *_trident_trunk_swfail[BCM_MAX_NUM_UNITS];

/* Function prototypes */

STATIC int _bcm_trident_trunk_set_port_property(int unit,
        bcm_module_t mod, bcm_port_t port, int tid);
STATIC int _bcm_trident_trunk_fabric_port_set(int unit,
        bcm_trunk_t hgtid, pbmp_t old_ports, pbmp_t new_ports);
STATIC int _bcm_trident_nuc_tpbm_get(int unit,
        int num_ports, bcm_module_t *tm, SHR_BITDCL *nuc_tpbm);
STATIC int _bcm_trident_trunk_block_mask(int unit, int nuc_type,
        pbmp_t old_ports, pbmp_t new_ports, pbmp_t nuc_ports,
        SHR_BITDCL *my_modid_bmap, uint32 member_count,
        uint32 flags);
STATIC int _bcm_trident_hg_dlb_sample_rate_thresholds_set(int unit,
        int sample_rate, int min_th, int max_th);
STATIC int _bcm_trident_hg_dlb_qsize_thresholds_set(int unit,
        int min_th, int max_th);
STATIC int _bcm_trident_hg_dlb_quality_assign(int unit, int tx_load_percent,
        uint32 *entry_arr);

/* ----------------------------------------------------------------------------- 
 *
 * Routines for initialization and de-initialization
 *
 * ----------------------------------------------------------------------------- 
 */

/*
 * Function:
 *      _bcm_trident_trunk_member_info_deinit
 * Purpose:
 *      Deallocate trunk membership info.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void
_bcm_trident_trunk_member_info_deinit(int unit)
{
    int trunk_num_groups;
    int i;

    if (_trident_trunk_member_info[unit]) {
        if (_trident_trunk_member_info[unit]->member_info) {
            trunk_num_groups = soc_mem_index_count(unit, TRUNK_GROUPm) +
                soc_mem_index_count(unit, HG_TRUNK_GROUPm);
            for (i = 0; i < trunk_num_groups; i++) {
                if (MEMBER_INFO(unit, i).modport) {
                    sal_free(MEMBER_INFO(unit, i).modport);
                    MEMBER_INFO(unit, i).modport = NULL;
                }
                if (MEMBER_INFO(unit, i).member_flags) {
                    sal_free(MEMBER_INFO(unit, i).member_flags);
                    MEMBER_INFO(unit, i).member_flags = NULL;
                }
            }
            sal_free(_trident_trunk_member_info[unit]->member_info);
            _trident_trunk_member_info[unit]->member_info = NULL;
        }
        sal_free(_trident_trunk_member_info[unit]);
        _trident_trunk_member_info[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_trident_trunk_member_info_init
 * Purpose:
 *      Allocate and initialize trunk member info.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_trident_trunk_member_info_init(int unit)
{
    int trunk_num_groups;

    if (_trident_trunk_member_info[unit] == NULL) {
        _trident_trunk_member_info[unit] =
            sal_alloc(sizeof(_trident_trunk_member_info_t), "trunk member info");
        if (_trident_trunk_member_info[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_member_info[unit], 0,
            sizeof(_trident_trunk_member_info_t));

    trunk_num_groups = soc_mem_index_count(unit, TRUNK_GROUPm) +
                       soc_mem_index_count(unit, HG_TRUNK_GROUPm);
    if (_trident_trunk_member_info[unit]->member_info == NULL) {
        _trident_trunk_member_info[unit]->member_info =
            sal_alloc(sizeof(_trident_member_info_t) * trunk_num_groups,
                    "member info");
        if (_trident_trunk_member_info[unit]->member_info == NULL) {
            _bcm_trident_trunk_member_info_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_member_info[unit]->member_info, 0, 
               sizeof(_trident_member_info_t) * trunk_num_groups);
                                                                                
    /* Note: The maximum number of member ports in a trunk group is 256 in Trident.
     *       But it would be quite wasteful to allocate 256 entries to each 
     *       _trident_trunk_member_info[unit]->member_info[x].modport/member_flags arrays.
     *       These arrays will be allocated as member ports are added to trunk group.
     */

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_mod_port_map_deinit
 * Purpose:
 *      Deinitialize a software cache for the SOURCE_TRUNK_MAP_TABLE.
 * Parameters:
 *      unit     -  (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void
_bcm_trident_trunk_mod_port_map_deinit(int unit) 
{
    if (_trident_mod_port_to_tid_map[unit]) {
        sal_free(_trident_mod_port_to_tid_map[unit]);
        _trident_mod_port_to_tid_map[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_trident_trunk_mod_port_map_init
 * Purpose:
 *      Initialize a software cache for the SOURCE_TRUNK_MAP_TABLE.
 * Parameters:
 *      unit     -  (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_trident_trunk_mod_port_map_init(int unit) 
{
    if (_trident_mod_port_to_tid_map[unit] == NULL) {
        _trident_mod_port_to_tid_map[unit] = 
            sal_alloc(sizeof(uint16) * _BCM_XGS3_TRUNK_MOD_PORT_MAP_IDX_COUNT(unit),
                    "_trident_mod_port_to_tid_map"); 
        if (_trident_mod_port_to_tid_map[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_mod_port_to_tid_map[unit], 0, 
            sizeof(uint16) * _BCM_XGS3_TRUNK_MOD_PORT_MAP_IDX_COUNT(unit));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_member_bk_deinit
 * Purpose:
 *      Deallocate _trident_trunk_member_bk
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void
_bcm_trident_trunk_member_bk_deinit(int unit) 
{
    if (_trident_trunk_member_bk[unit]) {
        if (_trident_trunk_member_bk[unit]->fp_trunk_member_bitmap) {
            sal_free(_trident_trunk_member_bk[unit]->fp_trunk_member_bitmap);
            _trident_trunk_member_bk[unit]->fp_trunk_member_bitmap = NULL;
        }
        if (_trident_trunk_member_bk[unit]->hg_trunk_member_bitmap) {
            sal_free(_trident_trunk_member_bk[unit]->hg_trunk_member_bitmap);
            _trident_trunk_member_bk[unit]->hg_trunk_member_bitmap = NULL;
        }
        sal_free(_trident_trunk_member_bk[unit]);
        _trident_trunk_member_bk[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_trident_trunk_member_bk_init
 * Purpose:
 *      Allocate and initialize _trident_trunk_member_bk
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_trident_trunk_member_bk_init(int unit) 
{
    int fp_trunk_member_tbl_size;
    int hg_trunk_member_tbl_size;

    if (_trident_trunk_member_bk[unit] == NULL) {
        _trident_trunk_member_bk[unit] = 
            sal_alloc(sizeof(_trident_trunk_member_bookkeeping_t), "_trident_trunk_member_bk");
        if (_trident_trunk_member_bk[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_member_bk[unit], 0,
            sizeof(_trident_trunk_member_bookkeeping_t));

    fp_trunk_member_tbl_size = soc_mem_index_count(unit, TRUNK_MEMBERm);
    if (_trident_trunk_member_bk[unit]->fp_trunk_member_bitmap == NULL) {
        _trident_trunk_member_bk[unit]->fp_trunk_member_bitmap = 
            sal_alloc(SHR_BITALLOCSIZE(fp_trunk_member_tbl_size), "fp_trunk_member_bitmap");
        if (_trident_trunk_member_bk[unit]->fp_trunk_member_bitmap == NULL) {
            _bcm_trident_trunk_member_bk_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_member_bk[unit]->fp_trunk_member_bitmap, 0,
            SHR_BITALLOCSIZE(fp_trunk_member_tbl_size));

    hg_trunk_member_tbl_size = soc_mem_index_count(unit, HG_TRUNK_MEMBERm);
    if (_trident_trunk_member_bk[unit]->hg_trunk_member_bitmap == NULL) {
        _trident_trunk_member_bk[unit]->hg_trunk_member_bitmap = 
            sal_alloc(SHR_BITALLOCSIZE(hg_trunk_member_tbl_size), "hg_trunk_member_bitmap");
        if (_trident_trunk_member_bk[unit]->hg_trunk_member_bitmap == NULL) {
            _bcm_trident_trunk_member_bk_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_member_bk[unit]->hg_trunk_member_bitmap, 0,
            SHR_BITALLOCSIZE(hg_trunk_member_tbl_size));

    /* Reserve entry 0 of HG_TRUNK_MEMBERm, as invalid Higig trunk groups have
     * their HG_TRUNK_GROUP table entry's BASE_PTR and TG_SIZE fields set to 0.
     */ 
    if (!SOC_WARM_BOOT(unit)) {
        SOC_IF_ERROR_RETURN
            (WRITE_HG_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL, 0,
                                    soc_mem_entry_null(unit, HG_TRUNK_MEMBERm)));
    }

    if (!SOC_IS_KATANAX(unit)) {
        /* Katana's HG_TRUNK_MEMBER table only has 8 members.
         * It would be too expensive to reserve entry 0.
         */
        _BCM_HG_TRUNK_MEMBER_USED_SET(unit, 0);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_quality_map_profile_init
 * Purpose:
 *      Initialize Higig DLB quality mapping profile.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_trident_hg_dlb_quality_map_profile_init(int unit) 
{
    soc_profile_mem_t *profile;
    soc_mem_t mem;
    int entry_words;
    int entries_per_profile;
    int tx_load_percent;
    int alloc_size;
    uint32 *entry_arr;
    int rv = BCM_E_NONE;
    void *entries;
    uint32 base_index = 0;
    int port_count;
    bcm_port_t port;
    uint32 quantize_control_reg;
    int i;

    if (NULL == _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile) {
        _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile =
            sal_alloc(sizeof(soc_profile_mem_t),
                      "HG DLB Quality Map Profile Mem");
        if (NULL == _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile) {
            return BCM_E_MEMORY;
        }
    } else {
        soc_profile_mem_destroy(unit,
                _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile);
    }
    profile = _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile;
    soc_profile_mem_t_init(profile);

    if (soc_mem_is_valid(unit, DLB_HGT_PORT_QUALITY_MAPPINGm)) {
        mem = DLB_HGT_PORT_QUALITY_MAPPINGm;
        entry_words = sizeof(dlb_hgt_port_quality_mapping_entry_t) /
            sizeof(uint32);
    } else {
        mem = DLB_HGT_QUALITY_MAPPINGm;
        entry_words = sizeof(dlb_hgt_quality_mapping_entry_t) / sizeof(uint32);
    }
    SOC_IF_ERROR_RETURN
        (soc_profile_mem_create(unit, &mem, &entry_words, 1, profile));

    entries_per_profile = 64;
    if (!SOC_WARM_BOOT(unit)) {
        alloc_size = entries_per_profile * entry_words * sizeof(uint32);
        entry_arr = sal_alloc(alloc_size, "HG DLB Quality Map entries");
        if (entry_arr == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(entry_arr, 0, alloc_size);

        /* Assign quality in the entry array, with 100% weight for port load,
         * 0% for queue size.
         */
        tx_load_percent = 100;
        rv = _bcm_trident_hg_dlb_quality_assign(unit, tx_load_percent,
                entry_arr);
        if (BCM_FAILURE(rv)) {
            sal_free(entry_arr);
            return rv;
        }

        entries = entry_arr;
        rv = soc_profile_mem_add(unit, profile, &entries, entries_per_profile,
                &base_index);
        sal_free(entry_arr);
        if (SOC_FAILURE(rv)) {
            return rv;
        }

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
            int member_id;
            dlb_hgt_quality_control_entry_t quality_control_entry;
            soc_field_t profile_ptr_f;

            for (member_id = 0;
                 member_id < soc_mem_index_count(unit,
                        DLB_HGT_QUALITY_CONTROLm);
                 member_id++) {
                BCM_IF_ERROR_RETURN
                    (READ_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ANY,
                                                   member_id,
                                                   &quality_control_entry));
                profile_ptr_f = soc_mem_field_valid(unit,
                        DLB_HGT_QUALITY_CONTROLm, PROFILE_PTRf) ?
                        PROFILE_PTRf : QMT_PROFILE_PTRf;
                soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                        &quality_control_entry, profile_ptr_f,
                        base_index / entries_per_profile);
                BCM_IF_ERROR_RETURN
                    (WRITE_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ALL,
                                                    member_id,
                                                    &quality_control_entry));
            }  

            for (i = 0; i < entries_per_profile; i++) {
                SOC_PROFILE_MEM_REFERENCE(unit, profile, base_index + i,
                        member_id - 1);
            }

            HG_DLB_INFO(unit)->
                hg_dlb_load_weight[base_index/entries_per_profile] =
                    tx_load_percent;
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            port_count = 0;
            PBMP_ALL_ITER(unit, port) {
                BCM_IF_ERROR_RETURN
                    (READ_DLB_HGT_QUANTIZE_CONTROLr(unit, port,
                                                    &quantize_control_reg));
                soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
                        &quantize_control_reg,
                        PORT_QUALITY_MAPPING_PROFILE_PTRf,
                        base_index / entries_per_profile);
                BCM_IF_ERROR_RETURN
                    (WRITE_DLB_HGT_QUANTIZE_CONTROLr(unit, port,
                                                     quantize_control_reg));
                port_count++;
            }  

            for (i = 0; i < entries_per_profile; i++) {
                SOC_PROFILE_MEM_REFERENCE(unit, profile, base_index + i,
                        port_count - 1);
            }

            HG_DLB_INFO(unit)->
                hg_dlb_load_weight[base_index/entries_per_profile] =
                    tx_load_percent;
        } 
    } else { /* Warm boot, recover reference counts and entries_per_set */
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
            int member_id;
            dlb_hgt_quality_control_entry_t quality_control_entry;
            soc_field_t profile_ptr_f;

            for (member_id = 0;
                 member_id < soc_mem_index_count(unit,
                        DLB_HGT_QUALITY_CONTROLm);
                 member_id++) {
                BCM_IF_ERROR_RETURN
                    (READ_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ANY,
                                                   member_id,
                                                   &quality_control_entry));
                profile_ptr_f = soc_mem_field_valid(unit,
                        DLB_HGT_QUALITY_CONTROLm, PROFILE_PTRf) ?
                        PROFILE_PTRf : QMT_PROFILE_PTRf;
                base_index = soc_DLB_HGT_QUALITY_CONTROLm_field32_get(unit,
                        &quality_control_entry, profile_ptr_f);

                base_index *= entries_per_profile;
                for (i = 0; i < entries_per_profile; i++) {
                    SOC_PROFILE_MEM_REFERENCE(unit, profile, base_index + i, 1);
                    SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, profile,
                            base_index + i, entries_per_profile);
                }
            }  
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            PBMP_ALL_ITER(unit, port) {
                BCM_IF_ERROR_RETURN(READ_DLB_HGT_QUANTIZE_CONTROLr(unit, port,
                            &quantize_control_reg));
                base_index = soc_reg_field_get(unit, DLB_HGT_QUANTIZE_CONTROLr,
                        quantize_control_reg, PORT_QUALITY_MAPPING_PROFILE_PTRf);
                base_index *= entries_per_profile;
                for (i = 0; i < entries_per_profile; i++) {
                    SOC_PROFILE_MEM_REFERENCE(unit, profile, base_index + i, 1);
                    SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, profile,
                            base_index + i, entries_per_profile);
                }
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_deinit
 * Purpose:
 *      Deallocate _trident_hg_dlb_bk
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void
_bcm_trident_hg_dlb_deinit(int unit) 
{
    int rv;
    soc_field_t dlb_refresh_f;
    uint32 arb_control_reg;

    if (_trident_hg_dlb_bk[unit]) {
        if (_trident_hg_dlb_bk[unit]->hg_dlb_id_used_bitmap) {
            sal_free(_trident_hg_dlb_bk[unit]->hg_dlb_id_used_bitmap);
            _trident_hg_dlb_bk[unit]->hg_dlb_id_used_bitmap = NULL;
        }
        if (_trident_hg_dlb_bk[unit]->hg_dlb_flowset_block_bitmap) {
            sal_free(_trident_hg_dlb_bk[unit]->hg_dlb_flowset_block_bitmap);
            _trident_hg_dlb_bk[unit]->hg_dlb_flowset_block_bitmap = NULL;
        }
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
            if (_trident_hg_dlb_bk[unit]->hg_dlb_member_id_used_bitmap) {
                sal_free(_trident_hg_dlb_bk[unit]->hg_dlb_member_id_used_bitmap);
                _trident_hg_dlb_bk[unit]->hg_dlb_member_id_used_bitmap = NULL;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
        if (_trident_hg_dlb_bk[unit]->hg_dlb_load_weight) {
            sal_free(_trident_hg_dlb_bk[unit]->hg_dlb_load_weight);
            _trident_hg_dlb_bk[unit]->hg_dlb_load_weight = NULL;
        }
        if (_trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile) {
            soc_profile_mem_destroy(unit,
                    _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile);
            sal_free(_trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile);
            _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile = NULL;
        }

        sal_free(_trident_hg_dlb_bk[unit]);
        _trident_hg_dlb_bk[unit] = NULL;
    }

    /* Disable DLB HGT refresh */
    dlb_refresh_f = soc_reg_field_valid(unit, AUX_ARB_CONTROL_2r,
            DLB_HGT_REFRESH_ENABLEf) ? DLB_HGT_REFRESH_ENABLEf :
            DLB_REFRESH_ENABLEf;
    rv = READ_AUX_ARB_CONTROL_2r(unit, &arb_control_reg);
    if (BCM_SUCCESS(rv)) {
        soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &arb_control_reg,
                dlb_refresh_f, 0);
        if (soc_reg_field_valid(unit, AUX_ARB_CONTROL_2r,
                    DLB_HGT_256NS_REFRESH_ENABLEf)) {
            soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &arb_control_reg,
                    DLB_HGT_256NS_REFRESH_ENABLEf, 0);
        } else if (soc_reg_field_valid(unit, AUX_ARB_CONTROL_2r,
                    DLB_1US_TICK_ENABLEf)) {
            soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &arb_control_reg,
                    DLB_1US_TICK_ENABLEf, 0);
        }
        (void)WRITE_AUX_ARB_CONTROL_2r(unit, arb_control_reg);
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_reg_field_valid(unit, SW2_HW_CONTROLr, ENABLE_HGT_DLBf)) {
        uint32 sw2_hw_control_reg;

        rv = READ_SW2_HW_CONTROLr(unit, &sw2_hw_control_reg);
        if (BCM_SUCCESS(rv)) {
            soc_reg_field_set(unit, SW2_HW_CONTROLr, &sw2_hw_control_reg,
                    ENABLE_HGT_DLBf, 0);
            (void)WRITE_SW2_HW_CONTROLr(unit, sw2_hw_control_reg);
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

}

/*
 * Function:
 *      _bcm_trident_hg_dlb_init
 * Purpose:
 *      Allocate and initialize _trident_hg_dlb_bk
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_trident_hg_dlb_init(int unit) 
{
    int num_hg_dlb_id;
    int hg_flowset_tbl_size;
    int total_num_blocks;
    int rv = BCM_E_NONE;
    uint32 quantize_control_reg;
    bcm_port_t port;
    uint32 measure_update_control_reg;
    soc_field_t dlb_refresh_f;
    uint32 arb_control_reg;
    int num_quality_map_profiles = 0;

    if (_trident_hg_dlb_bk[unit] == NULL) {
        _trident_hg_dlb_bk[unit] = sal_alloc(
                sizeof(_trident_hg_dlb_bookkeeping_t), "_trident_hg_dlb_bk");
        if (_trident_hg_dlb_bk[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_hg_dlb_bk[unit], 0,
            sizeof(_trident_hg_dlb_bookkeeping_t));

    num_hg_dlb_id = soc_mem_index_count(unit, DLB_HGT_GROUP_CONTROLm);
    if (_trident_hg_dlb_bk[unit]->hg_dlb_id_used_bitmap == NULL) {
        _trident_hg_dlb_bk[unit]->hg_dlb_id_used_bitmap = 
            sal_alloc(SHR_BITALLOCSIZE(num_hg_dlb_id), "hg_dlb_id_used_bitmap");
        if (_trident_hg_dlb_bk[unit]->hg_dlb_id_used_bitmap == NULL) {
            _bcm_trident_hg_dlb_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_hg_dlb_bk[unit]->hg_dlb_id_used_bitmap, 0,
            SHR_BITALLOCSIZE(num_hg_dlb_id));

    if (soc_mem_is_valid(unit, DLB_HGT_FLOWSET_PORTm)) {
        hg_flowset_tbl_size = soc_mem_index_count(unit, DLB_HGT_FLOWSET_PORTm);
    } else {
        hg_flowset_tbl_size = soc_mem_index_count(unit, DLB_HGT_FLOWSETm);
    }
    /* Each bit in hg_dlb_flowset_block_bitmap corresponds to a block of 512
     * flow set table entries.
     */
    total_num_blocks = hg_flowset_tbl_size / 512;
    if (_trident_hg_dlb_bk[unit]->hg_dlb_flowset_block_bitmap == NULL) {
        _trident_hg_dlb_bk[unit]->hg_dlb_flowset_block_bitmap = 
            sal_alloc(SHR_BITALLOCSIZE(total_num_blocks),
                    "hg_dlb_flowset_block_bitmap");
        if (_trident_hg_dlb_bk[unit]->hg_dlb_flowset_block_bitmap == NULL) {
            _bcm_trident_hg_dlb_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_hg_dlb_bk[unit]->hg_dlb_flowset_block_bitmap, 0,
            SHR_BITALLOCSIZE(total_num_blocks));

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
        int num_hg_dlb_member_id;
        soc_field_t profile_ptr_f;

        num_hg_dlb_member_id = soc_mem_index_count(unit,
                DLB_HGT_MEMBER_ATTRIBUTEm);
        if (_trident_hg_dlb_bk[unit]->hg_dlb_member_id_used_bitmap == NULL) {
            _trident_hg_dlb_bk[unit]->hg_dlb_member_id_used_bitmap = 
                sal_alloc(SHR_BITALLOCSIZE(num_hg_dlb_member_id),
                        "hg_dlb_member_id_used_bitmap");
            if (_trident_hg_dlb_bk[unit]->hg_dlb_member_id_used_bitmap == NULL) {
                _bcm_trident_hg_dlb_deinit(unit);
                return BCM_E_MEMORY;
            }
        }
        sal_memset(_trident_hg_dlb_bk[unit]->hg_dlb_member_id_used_bitmap, 0,
                SHR_BITALLOCSIZE(num_hg_dlb_member_id));

        profile_ptr_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_CONTROLm,
                PROFILE_PTRf) ? PROFILE_PTRf : QMT_PROFILE_PTRf;
        num_quality_map_profiles = 1 << soc_mem_field_length(unit,
                DLB_HGT_QUALITY_CONTROLm, profile_ptr_f);
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        num_quality_map_profiles = 1 << soc_reg_field_length(unit,
                DLB_HGT_QUANTIZE_CONTROLr, PORT_QUALITY_MAPPING_PROFILE_PTRf);
    }

    if (_trident_hg_dlb_bk[unit]->hg_dlb_load_weight == NULL) {
        _trident_hg_dlb_bk[unit]->hg_dlb_load_weight = 
            sal_alloc(num_quality_map_profiles * sizeof(uint8),
                    "hg_dlb_load_weight");
        if (_trident_hg_dlb_bk[unit]->hg_dlb_load_weight == NULL) {
            _bcm_trident_hg_dlb_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_hg_dlb_bk[unit]->hg_dlb_load_weight, 0,
            num_quality_map_profiles * sizeof(uint8));

    /* Initialize port quality mapping profile */
    rv = _bcm_trident_hg_dlb_quality_map_profile_init(unit);
    if (BCM_FAILURE(rv)) {
        _bcm_trident_hg_dlb_deinit(unit);
        return rv;
    }

    if (!SOC_WARM_BOOT(unit)) {
        /* Set Higig DLB sampling period and port load thresholds */
        rv = _bcm_trident_hg_dlb_sample_rate_thresholds_set(unit, 1000000,
                1250, 8750);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

        /* Configure Higig DLB port load EWMA weight */
        rv = _bcm_trident_hg_dlb_config_set(unit,
                bcmSwitchFabricTrunkDynamicEgressBytesExponent, 0);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

        /* Configure Higig DLB port queue size EWMA weight */
        rv = _bcm_trident_hg_dlb_config_set(unit,
                bcmSwitchFabricTrunkDynamicQueuedBytesExponent, 0);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

        /* Configure Higig DLB port load cap control */
        rv = _bcm_trident_hg_dlb_config_set(unit,
                bcmSwitchFabricTrunkDynamicEgressBytesDecreaseReset, 0);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

        /* Configure Higig DLB port queue size cap control */
        rv = _bcm_trident_hg_dlb_config_set(unit,
                bcmSwitchFabricTrunkDynamicQueuedBytesDecreaseReset, 0);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

        /* Configure Higig DLB port queue size thresholds */
        rv = _bcm_trident_hg_dlb_qsize_thresholds_set(unit, 0, 0);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

        /* Disable link status override by software */
        if (soc_mem_is_valid(unit, DLB_HGT_LINK_CONTROLm)) {
            rv = soc_mem_clear(unit, DLB_HGT_LINK_CONTROLm, MEM_BLOCK_ALL, 0);
        } else {
            rv = soc_mem_clear(unit, DLB_HGT_MEMBER_SW_STATEm, MEM_BLOCK_ALL, 0);
        }
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

        /* Disable all HGT DLB groups */
        if (SOC_MEM_IS_VALID(unit, HGT_DLB_CONTROLm)) {
            rv = soc_mem_clear(unit, HGT_DLB_CONTROLm, MEM_BLOCK_ALL, 0);
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }
        } else {
            rv = soc_mem_clear(unit, DLB_HGT_GROUP_CONTROLm, MEM_BLOCK_ALL, 0);
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }
        }

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
            int member_id;
            dlb_hgt_quality_control_entry_t quality_control_entry;
            soc_field_t enable_credit_collect_f;

            /* Clear group membership */
            rv = soc_mem_clear(unit, DLB_HGT_GROUP_MEMBERSHIPm, MEM_BLOCK_ALL, 0);
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }

            /* Clear port<->member mapping tables */
            rv = soc_mem_clear(unit, DLB_HGT_PORT_MEMBER_MAPm, MEM_BLOCK_ALL, 0);
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }
            rv = soc_mem_clear(unit, DLB_HGT_MEMBER_ATTRIBUTEm, MEM_BLOCK_ALL, 0);
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }

            /* Clear per member threshold scaling factor and 
             * quality measure update control.
             */
            for (member_id = 0;
                    member_id < soc_mem_index_count(unit,
                        DLB_HGT_QUALITY_CONTROLm);
                    member_id++) {
                rv = READ_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ANY,
                        member_id, &quality_control_entry);
                if (SOC_FAILURE(rv)) {
                    _bcm_trident_hg_dlb_deinit(unit);
                    return rv;
                }
                soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                        &quality_control_entry, ENABLE_AVG_CALCf, 0);
                soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                        &quality_control_entry, ENABLE_QUALITY_UPDATEf, 0);
                enable_credit_collect_f = soc_mem_field_valid(unit,
                        DLB_HGT_QUALITY_CONTROLm, ENABLE_CREDIT_COLLECTIONf) ?
                       ENABLE_CREDIT_COLLECTIONf : ENB_CREDIT_COLLECTIONf;
                soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                        &quality_control_entry, enable_credit_collect_f, 0);
                soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                        &quality_control_entry, LOADING_SCALING_FACTORf, 0);
                soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                        &quality_control_entry, QSIZE_SCALING_FACTORf, 0);
                rv = WRITE_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ALL,
                        member_id, &quality_control_entry);
                if (SOC_FAILURE(rv)) {
                    _bcm_trident_hg_dlb_deinit(unit);
                    return rv;
                }
            }
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            /* Clear per port threshold scaling factor and 
             * quality measure update control.
             */
            PBMP_ALL_ITER(unit, port) {
                rv = READ_DLB_HGT_QUANTIZE_CONTROLr(unit, port,
                        &quantize_control_reg);
                if (BCM_FAILURE(rv)) {
                    _bcm_trident_hg_dlb_deinit(unit);
                    return rv;
                }
                soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
                        &quantize_control_reg,
                        PORT_LOADING_THRESHOLD_SCALING_FACTORf, 0);
                soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
                        &quantize_control_reg,
                        PORT_QSIZE_THRESHOLD_SCALING_FACTORf, 0);
                rv = WRITE_DLB_HGT_QUANTIZE_CONTROLr(unit, port,
                        quantize_control_reg);
                if (BCM_FAILURE(rv)) {
                    _bcm_trident_hg_dlb_deinit(unit);
                    return rv;
                }

                rv = READ_DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr(unit,
                        port, &measure_update_control_reg);
                if (BCM_FAILURE(rv)) {
                    _bcm_trident_hg_dlb_deinit(unit);
                    return rv;
                }
                soc_reg_field_set(unit,
                        DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                        &measure_update_control_reg,
                        ENABLE_MEASURE_COLLECTIONf, 0);
                soc_reg_field_set(unit,
                        DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                        &measure_update_control_reg,
                        ENABLE_MEASURE_AVERAGE_CALCULATIONf, 0);
                soc_reg_field_set(unit,
                        DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                        &measure_update_control_reg,
                        ENABLE_PORT_QUALITY_UPDATEf, 0);
                rv = WRITE_DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr(unit,
                        port, measure_update_control_reg);
                if (BCM_FAILURE(rv)) {
                    _bcm_trident_hg_dlb_deinit(unit);
                    return rv;
                }
            }
        }

        /* Enable DLB HGT refresh */
        dlb_refresh_f = soc_reg_field_valid(unit, AUX_ARB_CONTROL_2r,
            DLB_HGT_REFRESH_ENABLEf) ? DLB_HGT_REFRESH_ENABLEf :
            DLB_REFRESH_ENABLEf;
        rv = READ_AUX_ARB_CONTROL_2r(unit, &arb_control_reg);
        if (BCM_SUCCESS(rv)) {
            soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &arb_control_reg,
                    dlb_refresh_f, 1);
            if (soc_reg_field_valid(unit, AUX_ARB_CONTROL_2r,
                        DLB_HGT_256NS_REFRESH_ENABLEf)) {
                soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &arb_control_reg,
                                  DLB_HGT_256NS_REFRESH_ENABLEf, 1);
            } else if (soc_reg_field_valid(unit, AUX_ARB_CONTROL_2r,
                        DLB_1US_TICK_ENABLEf)) {
                soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &arb_control_reg,
                        DLB_1US_TICK_ENABLEf, 1);
            }
            rv = WRITE_AUX_ARB_CONTROL_2r(unit, arb_control_reg);
        }
        if (BCM_FAILURE(rv)) {
            _bcm_trident_hg_dlb_deinit(unit);
            return rv;
        }

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_reg_field_valid(unit, SW2_HW_CONTROLr, ENABLE_HGT_DLBf)) {
            uint32 sw2_hw_control_reg;

            rv = READ_SW2_HW_CONTROLr(unit, &sw2_hw_control_reg);
            if (BCM_SUCCESS(rv)) {
                soc_reg_field_set(unit, SW2_HW_CONTROLr, &sw2_hw_control_reg,
                        ENABLE_HGT_DLBf, 1);
                rv = WRITE_SW2_HW_CONTROLr(unit, sw2_hw_control_reg);
            }
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        /* Configure Ethertype eligibility */
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_mem_is_valid(unit, DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm)) {
            uint32 measure_control_reg;

            rv = soc_mem_clear(unit, DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm,
                    MEM_BLOCK_ALL, 0);
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }
            rv = READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit,
                    &measure_control_reg);
            if (SOC_SUCCESS(rv)) {
                soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                        &measure_control_reg,
                        ETHERTYPE_ELIGIBILITY_CONFIGf, 0);
                soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                        &measure_control_reg,
                        INNER_OUTER_ETHERTYPE_SELECTIONf, 0);
                rv = WRITE_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit,
                        measure_control_reg);
            }
            if (BCM_FAILURE(rv)) {
                _bcm_trident_hg_dlb_deinit(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

    }

    return rv;
}

/*
 * Function:
 *      _bcm_trident_hg_trunk_override_profile_deinit
 * Purpose:
 *      Deallocate _trident_hg_trunk_override_profile
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC void
_bcm_trident_hg_trunk_override_profile_deinit(int unit)
{
    if (_trident_hg_trunk_override_profile[unit]) {
        (void)soc_profile_mem_destroy(unit, _trident_hg_trunk_override_profile[unit]);
        sal_free(_trident_hg_trunk_override_profile[unit]);
        _trident_hg_trunk_override_profile[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_trident_hg_trunk_override_profile_init
 * Purpose:
 *      Allocate and initialize _trident_hg_trunk_override_profile
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_trident_hg_trunk_override_profile_init(int unit)
{
    soc_mem_t mem;
    int       entry_words;
    void      *entry_ptr;
    int       ref_count;
    uint32    index;
    int       i;
    uint32    profile_ptr;
    vlan_tab_entry_t vlan_tab_entry;
    l2mc_entry_t l2mc_entry;
    ipmc_entry_t ipmc_entry;
    ifp_redirection_profile_entry_t ifp_redirection_profile_entry;

    if (_trident_hg_trunk_override_profile[unit] == NULL) {
        _trident_hg_trunk_override_profile[unit] = 
            sal_alloc(sizeof(soc_profile_mem_t), "HG Trunk Override Profile Mem");
        if (_trident_hg_trunk_override_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    } else {
        soc_profile_mem_destroy(unit, _trident_hg_trunk_override_profile[unit]);
    }
    soc_profile_mem_t_init(_trident_hg_trunk_override_profile[unit]);

    /* Create profile table cache (or re-init if it already exists) */
    mem = ING_HIGIG_TRUNK_OVERRIDE_PROFILEm;
    entry_words = sizeof(ing_higig_trunk_override_profile_entry_t) / sizeof(uint32);
    SOC_IF_ERROR_RETURN
        (soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                _trident_hg_trunk_override_profile[unit]));

    if (!SOC_WARM_BOOT(unit)) {
        /* By default, VLAN, L2MC, IPMC, and IFP Redirection Profile tables' Higig
         * trunk override profile pointer is 0. Add a profile at index 0 of
         * ING_HIGIG_TRUNK_OVERRIDE_PROFILE table with the reference count equal
         * to the number of entries in VLAN, L2MC, IPMC, and IFP Redirection
         * Profile tables.
         */
        entry_ptr = soc_mem_entry_null(unit, ING_HIGIG_TRUNK_OVERRIDE_PROFILEm);
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_add(unit, _trident_hg_trunk_override_profile[unit],
                                 &entry_ptr, 1, &index));
        ref_count = soc_mem_index_count(unit, VLAN_TABm);
        ref_count += soc_mem_index_count(unit, L2MCm);
        ref_count += soc_mem_index_count(unit, L3_IPMCm);
        ref_count += soc_mem_index_count(unit, IFP_REDIRECTION_PROFILEm);
        SOC_PROFILE_MEM_REFERENCE(unit, _trident_hg_trunk_override_profile[unit],
                index, ref_count - 1);
    } else { /* Warm boot, recover reference counts and entries_per_set */

        /* Recover from VLAN_TAB */
        for (i = soc_mem_index_min(unit, VLAN_TABm);
             i <= soc_mem_index_max(unit, VLAN_TABm);
             i++) {
            SOC_IF_ERROR_RETURN
                (READ_VLAN_TABm(unit, MEM_BLOCK_ANY, i, &vlan_tab_entry));
            profile_ptr = soc_VLAN_TABm_field32_get(unit, &vlan_tab_entry,
                    HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
        }

        /* Recover from L2MC */
        for (i = soc_mem_index_min(unit, L2MCm);
             i <= soc_mem_index_max(unit, L2MCm);
             i++) {
            SOC_IF_ERROR_RETURN
                (READ_L2MCm(unit, MEM_BLOCK_ANY, i, &l2mc_entry));
            profile_ptr = soc_L2MCm_field32_get(unit, &l2mc_entry,
                    HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
        }

        /* Recover from IPMC */
        for (i = soc_mem_index_min(unit, L3_IPMCm);
             i <= soc_mem_index_max(unit, L3_IPMCm);
             i++) {
            SOC_IF_ERROR_RETURN
                (READ_L3_IPMCm(unit, MEM_BLOCK_ANY, i, &ipmc_entry));
            profile_ptr = soc_L3_IPMCm_field32_get(unit, &ipmc_entry,
                    HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
        }

        /* Recover from IFP_REDIRECTION_PROFILE */
        for (i = soc_mem_index_min(unit, IFP_REDIRECTION_PROFILEm);
             i <= soc_mem_index_max(unit, IFP_REDIRECTION_PROFILEm);
             i++) {
            SOC_IF_ERROR_RETURN
                (READ_IFP_REDIRECTION_PROFILEm(unit, MEM_BLOCK_ANY, i,
                                               &ifp_redirection_profile_entry));
            profile_ptr = soc_IFP_REDIRECTION_PROFILEm_field32_get(unit,
                    &ifp_redirection_profile_entry,
                    HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit,
                    _trident_hg_trunk_override_profile[unit],
                    profile_ptr, 1);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_deinit
 * Purpose:
 *      Deallocate _trident_trunk_hwfail
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void
_bcm_trident_trunk_hwfailover_deinit(int unit)
{
    int trunk_num_groups;
    int i;

    if (_trident_trunk_hwfail[unit]) {
        if (_trident_trunk_hwfail[unit]->hw_tinfo) {
            trunk_num_groups = soc_mem_index_count(unit, TRUNK_GROUPm) +
                soc_mem_index_count(unit, HG_TRUNK_GROUPm);
            for (i = 0; i < trunk_num_groups; i++) {
                if (_trident_trunk_hwfail[unit]->hw_tinfo[i].modport) {
                    sal_free(_trident_trunk_hwfail[unit]->hw_tinfo[i].modport);
                    _trident_trunk_hwfail[unit]->hw_tinfo[i].modport = NULL;
                }
                if (_trident_trunk_hwfail[unit]->hw_tinfo[i].psc) {
                    sal_free(_trident_trunk_hwfail[unit]->hw_tinfo[i].psc);
                    _trident_trunk_hwfail[unit]->hw_tinfo[i].psc = NULL;
                }
                if (_trident_trunk_hwfail[unit]->hw_tinfo[i].flags) {
                    sal_free(_trident_trunk_hwfail[unit]->hw_tinfo[i].flags);
                    _trident_trunk_hwfail[unit]->hw_tinfo[i].flags = NULL;
                }
            }
            sal_free(_trident_trunk_hwfail[unit]->hw_tinfo);
            _trident_trunk_hwfail[unit]->hw_tinfo = NULL;
        }
        sal_free(_trident_trunk_hwfail[unit]);
        _trident_trunk_hwfail[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_init
 * Purpose:
 *      Allocate and initialize _trident_trunk_hwfail
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_trident_trunk_hwfailover_init(int unit)
{
    int trunk_num_groups;

    if (_trident_trunk_hwfail[unit] == NULL) {
        _trident_trunk_hwfail[unit] = sal_alloc(sizeof(_trident_trunk_hwfail_t), 
                "_trident_trunk_hwfail");
        if (_trident_trunk_hwfail[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_hwfail[unit], 0, sizeof(_trident_trunk_hwfail_t));

    trunk_num_groups = soc_mem_index_count(unit, TRUNK_GROUPm) +
                       soc_mem_index_count(unit, HG_TRUNK_GROUPm);
    if (_trident_trunk_hwfail[unit]->hw_tinfo == NULL) {
        _trident_trunk_hwfail[unit]->hw_tinfo =
            sal_alloc(sizeof(_trident_hw_tinfo_t) * trunk_num_groups,
                    "_trident_trunk_hwfail_hw_tinfo");
        if (_trident_trunk_hwfail[unit]->hw_tinfo == NULL) {
            _bcm_trident_trunk_hwfailover_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_hwfail[unit]->hw_tinfo, 0, 
               sizeof(_trident_hw_tinfo_t) * trunk_num_groups);
                                                                                
    /* Note: The maximum number of member ports in a trunk group is 256 in Trident.
     *       But it would be quite wasteful to allocate 256 entries to each 
     *       _trident_trunk_hwfail[unit]->hw_tinfo[x].modport/psc/flags arrays.
     *       These arrays will be allocated as member ports are added to trunk group.
     */

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_swfailover_deinit
 * Purpose:
 *      Deallocate _trident_trunk_swfail
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void
_bcm_trident_trunk_swfailover_deinit(int unit)
{
    int trunk_num_groups;
    int i;

    if (_trident_trunk_swfail[unit]) {
        if (_trident_trunk_swfail[unit]->tinfo) {
            trunk_num_groups = soc_mem_index_count(unit, TRUNK_GROUPm) +
                soc_mem_index_count(unit, HG_TRUNK_GROUPm);
            for (i = 0; i < trunk_num_groups; i++) {
                if (_trident_trunk_swfail[unit]->tinfo[i].modport) {
                    sal_free(_trident_trunk_swfail[unit]->tinfo[i].modport);
                    _trident_trunk_swfail[unit]->tinfo[i].modport = NULL;
                }
            }
            sal_free(_trident_trunk_swfail[unit]->tinfo);
            _trident_trunk_swfail[unit]->tinfo = NULL;
        }
        sal_free(_trident_trunk_swfail[unit]);
        _trident_trunk_swfail[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_trident_trunk_swfailover_init
 * Purpose:
 *      Allocate and initialize _trident_trunk_swfail
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_trident_trunk_swfailover_init(int unit)
{
    int trunk_num_groups;

    if (_trident_trunk_swfail[unit] == NULL) {
        _trident_trunk_swfail[unit] = sal_alloc(sizeof(_trident_trunk_swfail_t), 
                "_trident_trunk_swfail");
        if (_trident_trunk_swfail[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_swfail[unit], 0, sizeof(_trident_trunk_swfail_t));

    trunk_num_groups = soc_mem_index_count(unit, TRUNK_GROUPm) +
                       soc_mem_index_count(unit, HG_TRUNK_GROUPm);
    if (_trident_trunk_swfail[unit]->tinfo == NULL) {
        _trident_trunk_swfail[unit]->tinfo =
            sal_alloc(sizeof(_trident_tinfo_t) * trunk_num_groups,
                    "_trident_trunk_swfail_tinfo");
        if (_trident_trunk_swfail[unit]->tinfo == NULL) {
            _bcm_trident_trunk_swfailover_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_trident_trunk_swfail[unit]->tinfo, 0, 
               sizeof(_trident_tinfo_t) * trunk_num_groups);
                                                                                
    /* Note: The maximum number of member ports in a trunk group is 256 in Trident.
     *       But it would be quite wasteful to allocate 256 entries to each 
     *       _trident_trunk_swfail[unit]->tinfo[x].modport array. The modport
     *       array will be allocated as member ports are added to trunk group.
     */

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_deinit
 * Purpose:
 *      Deallocate global data structures
 * Parameters:
 *      unit - (IN)SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
void
_bcm_trident_trunk_deinit(int unit)
{
    _bcm_trident_trunk_member_info_deinit(unit);

    _bcm_trident_trunk_mod_port_map_deinit(unit);

    _bcm_trident_trunk_member_bk_deinit(unit);

    if (soc_feature(unit, soc_feature_hg_dlb)) {
        _bcm_trident_hg_dlb_deinit(unit);
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_lag_dlb)) {
        bcm_tr3_lag_dlb_deinit(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    _bcm_trident_hg_trunk_override_profile_deinit(unit);

    _bcm_trident_trunk_hwfailover_deinit(unit);

    _bcm_trident_trunk_swfailover_deinit(unit);

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
        bcm_td2_hg_rh_deinit(unit);
    }
    if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
        bcm_td2_lag_rh_deinit(unit);
    }
#endif /* BCM_TRIDENT2_SUPPORT */

}

/*
 * Function:
 *      _bcm_trident_trunk_init
 * Purpose:
 *      Initialize global data structures
 * Parameters:
 *      unit - (IN)SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_init(int unit)
{
    int rv;

    _bcm_trident_trunk_deinit(unit);

    rv = _bcm_trident_trunk_member_info_init(unit);
    if (BCM_FAILURE(rv)) {
        _bcm_trident_trunk_deinit(unit);
        return rv;
    }

    rv = _bcm_trident_trunk_mod_port_map_init(unit);
    if (BCM_FAILURE(rv)) {
        _bcm_trident_trunk_deinit(unit);
        return rv;
    }

    rv = _bcm_trident_trunk_member_bk_init(unit);
    if (BCM_FAILURE(rv)) {
        _bcm_trident_trunk_deinit(unit);
        return rv;
    }

    if (soc_feature(unit, soc_feature_hg_dlb)) {
        rv = _bcm_trident_hg_dlb_init(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_trunk_deinit(unit);
            return rv;
        }
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_lag_dlb)) {
        rv = bcm_tr3_lag_dlb_init(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_trunk_deinit(unit);
            return rv;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    rv = _bcm_trident_hg_trunk_override_profile_init(unit);
    if (BCM_FAILURE(rv)) {
        _bcm_trident_trunk_deinit(unit);
        return rv;
    }

    rv = _bcm_trident_trunk_hwfailover_init(unit);
    if (BCM_FAILURE(rv)) {
        _bcm_trident_trunk_deinit(unit);
        return rv;
    }

    rv = _bcm_trident_trunk_swfailover_init(unit);
    if (BCM_FAILURE(rv)) {
        _bcm_trident_trunk_deinit(unit);
        return rv;
    }

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
        rv = bcm_td2_hg_rh_init(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_trunk_deinit(unit);
            return rv;
        }
    }

    if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
        rv = bcm_td2_lag_rh_init(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_trident_trunk_deinit(unit);
            return rv;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    return BCM_E_NONE;
}

/* ----------------------------------------------------------------------------- 
 *
 * Routines for hardware failover
 *
 * ----------------------------------------------------------------------------- 
 */

static soc_field_t _hg_trunk_failover_portf[8] = {
    PORT0f, PORT1f, PORT2f, PORT3f,
    PORT4f, PORT5f, PORT6f, PORT7f
};


#if defined(BCM_KATANA2_SUPPORT)
static soc_field_t _hg_trunk_failover_pp_portf[16]= {
    PP_PORT0f, PP_PORT1f, PP_PORT2f, PP_PORT3f,
    PP_PORT4f, PP_PORT5f, PP_PORT6f, PP_PORT7f,
    PP_PORT8f, PP_PORT9f, PP_PORT10f, PP_PORT11f,
    PP_PORT12f, PP_PORT13f, PP_PORT14f, PP_PORT15f
};
static soc_field_t _hg_trunk_failover_physical_portf[16]= {
    PHYSICAL_PORT0f, PHYSICAL_PORT1f, PHYSICAL_PORT2f, PHYSICAL_PORT3f,
    PHYSICAL_PORT4f, PHYSICAL_PORT5f, PHYSICAL_PORT6f, PHYSICAL_PORT7f,
    PHYSICAL_PORT8f, PHYSICAL_PORT9f, PHYSICAL_PORT10f, PHYSICAL_PORT11f,
    PHYSICAL_PORT12f, PHYSICAL_PORT13f, PHYSICAL_PORT14f, PHYSICAL_PORT15f
};
#endif /* defined(BCM_KATANA2_SUPPORT) */


/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_hg_write
 * Purpose:
 *      Set Higig trunk hardware failover config.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      failport - (IN) Fail port.
 *      rtag     - (IN) Port selection criteria for failover port list.
 *      hw_count - (IN) Number of ports in failover port list.
 *      ftp      - (IN) Pointer to failover port list.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_hwfailover_hg_write(int unit,
        bcm_port_t failport, int rtag,
        int hw_count, bcm_port_t *ftp)
{
    hg_trunk_failover_set_entry_t     fail_entry;
    hg_trunk_failover_enable_entry_t  failover_enable_entry;
    int           ix;
    bcm_pbmp_t    fail_ports;
    int    failover_max_portcnt;
    
    failover_max_portcnt = 1 << soc_mem_field_length(unit,
                HG_TRUNK_FAILOVER_SETm, FAILOVER_SET_SIZEf);

    sal_memset(&fail_entry, 0, sizeof(fail_entry));

    if (hw_count) { /* Enabling HG trunk failover */
        soc_HG_TRUNK_FAILOVER_SETm_field32_set(unit, &fail_entry,
                FAILOVER_SET_SIZEf, hw_count - 1);
        soc_HG_TRUNK_FAILOVER_SETm_field32_set(unit, &fail_entry,
                RTAGf, rtag);

        for (ix = 0; ix < failover_max_portcnt; ix++) {
#if defined(BCM_KATANA2_SUPPORT)
            if (SOC_IS_KATANA2(unit)) {
                soc_HG_TRUNK_FAILOVER_SETm_field32_set(unit, &fail_entry,
                        _hg_trunk_failover_pp_portf[ix], ftp[ix % hw_count]);
                soc_HG_TRUNK_FAILOVER_SETm_field32_set(unit, &fail_entry,
                        _hg_trunk_failover_physical_portf[ix], ftp[ix % hw_count]);

            } else
#endif /*  (defined(BCM_KATANA2_SUPPORT)  */
            { 
                soc_HG_TRUNK_FAILOVER_SETm_field32_set(unit, &fail_entry,
                        _hg_trunk_failover_portf[ix], ftp[ix % hw_count]);
            }
        }
        SOC_IF_ERROR_RETURN
            (WRITE_HG_TRUNK_FAILOVER_SETm(unit, MEM_BLOCK_ALL, failport,
                                          &fail_entry));
    }

    /* Update HG trunk mode selection bitmap */

    SOC_IF_ERROR_RETURN
        (READ_HG_TRUNK_FAILOVER_ENABLEm(unit, MEM_BLOCK_ANY, 0,
                                        &failover_enable_entry));
    SOC_PBMP_CLEAR(fail_ports);
    soc_mem_pbmp_field_get(unit, HG_TRUNK_FAILOVER_ENABLEm,
            &failover_enable_entry, BITMAPf, &fail_ports);

    if (hw_count) {
        SOC_PBMP_PORT_ADD(fail_ports, failport);
    } else {
        SOC_PBMP_PORT_REMOVE(fail_ports, failport);
    }

    soc_mem_pbmp_field_set(unit, HG_TRUNK_FAILOVER_ENABLEm,
            &failover_enable_entry, BITMAPf, &fail_ports);

    SOC_IF_ERROR_RETURN
        (WRITE_HG_TRUNK_FAILOVER_ENABLEm(unit, MEM_BLOCK_ALL, 0,
                                         &failover_enable_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_hg_read
 * Purpose:
 *      Get trunk hardware failover config.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      failport - (IN) Port ID of the fail port.
 *      array_size  - (IN) Max number of ports in failover port list.
 *      rtag        - (OUT) Pointer to failover rtag.
 *      ftp         - (OUT) Pointer to port IDs in failover port list.
 *      ftm         - (OUT) Pointer to module IDs in failover port list.
 *      array_count - (OUT) Number of ports in failover port list.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_hwfailover_hg_read(int unit,
        bcm_port_t failport, int array_size,
        int *rtag, bcm_port_t *ftp, bcm_module_t *ftm,
        int *array_count)
{
    hg_trunk_failover_enable_entry_t  failover_enable_entry;
    hg_trunk_failover_set_entry_t     fail_entry;
    bcm_pbmp_t    fail_ports;
    int           ix, hw_count;

    SOC_IF_ERROR_RETURN
        (READ_HG_TRUNK_FAILOVER_ENABLEm(unit, MEM_BLOCK_ANY, 0,
                                        &failover_enable_entry));
    SOC_PBMP_CLEAR(fail_ports);
    soc_mem_pbmp_field_get(unit, HG_TRUNK_FAILOVER_ENABLEm,
            &failover_enable_entry, BITMAPf, &fail_ports);

    if (SOC_PBMP_MEMBER(fail_ports, failport)) {
        SOC_IF_ERROR_RETURN
            (READ_HG_TRUNK_FAILOVER_SETm(unit, MEM_BLOCK_ALL, failport,
                                         &fail_entry));
        hw_count = 1 + soc_mem_field32_get(unit, HG_TRUNK_FAILOVER_SETm,
                &fail_entry, FAILOVER_SET_SIZEf);
        if ((hw_count > array_size) && (array_size != 0)) {
            hw_count = array_size;
        }
        for (ix = 0; ix < hw_count; ix++) {
            if (NULL != ftm) {
                ftm[ix] = 0;
            }
            if (NULL != ftp) {
#if defined(BCM_KATANA2_SUPPORT)
                if (SOC_IS_KATANA2(unit)) {
                    ftp[ix] = soc_mem_field32_get(unit, HG_TRUNK_FAILOVER_SETm,
                            &fail_entry,
                            _hg_trunk_failover_pp_portf[ix]);
                    ftp[ix] = soc_mem_field32_get(unit, HG_TRUNK_FAILOVER_SETm,
                            &fail_entry,
                            _hg_trunk_failover_physical_portf[ix]);
                } else
#endif /*  (defined(BCM_KATANA2_SUPPORT)  */
                { 
                    ftp[ix] = soc_mem_field32_get(unit, HG_TRUNK_FAILOVER_SETm,
                            &fail_entry,
                            _hg_trunk_failover_portf[ix]);
                }
            }
        }
        if (NULL != array_count) {
            *array_count = hw_count;
        }
        if (NULL != rtag) {
            *rtag = soc_mem_field32_get(unit, HG_TRUNK_FAILOVER_SETm,
                                        &fail_entry, RTAGf);
        }
    } else {
        if (NULL != array_count) {
            *array_count = 0;
        }
        if (NULL != rtag) {
            *rtag = 0;
        }
    }
    return BCM_E_NONE;
}

static soc_field_t _trunk_failover_portf[8] = {
    PORT0f, PORT1f, PORT2f, PORT3f,
    PORT4f, PORT5f, PORT6f, PORT7f
};

static soc_field_t _trunk_failover_modulef[8] = {
    MODULE0f, MODULE1f, MODULE2f, MODULE3f,
    MODULE4f, MODULE5f, MODULE6f, MODULE7f
};

/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_write
 * Purpose:
 *      Set front panel trunk hardware failover config.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      failport - (IN) Fail port.
 *      rtag     - (IN) Port selection criteria for failover port list.
 *      hw_count - (IN) Number of ports in failover port list.
 *      ftp      - (IN) Pointer to failover port list port ID.
 *      ftm      - (IN) Pointer to failover port list module ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_hwfailover_write(int unit,
        bcm_port_t failport, int rtag,
        int hw_count, bcm_port_t *ftp, bcm_module_t *ftm)
{
    port_lag_failover_set_entry_t  fail_entry;
    uint64 val64;
    int    ix, mirror_enable;
    int    failover_max_portcnt;
    soc_reg_t lag_failover_status_reg;
    soc_reg_t lag_failover_config_reg;
    int    old_failover_en, new_failover_en;
    int    old_link_status_sel, new_link_status_sel;
    int    old_reset_flow_control, new_reset_flow_control;

    lag_failover_status_reg = SOC_REG_IS_VALID(unit, LAG_FAILOVER_STATUSr) ?
              LAG_FAILOVER_STATUSr: XLMAC_LAG_FAILOVER_STATUSr;
    SOC_IF_ERROR_RETURN
        (soc_reg_get(unit, lag_failover_status_reg, failport, 0, &val64));
    if (soc_reg64_field32_get(unit, lag_failover_status_reg, val64,
                LAG_FAILOVER_LOOPBACKf)) {
        /*
         * Already in failover mode.  Do not change HW settings!
         * This should be dealt with as part of the port link processing.
         */
        if (hw_count == 0) {
            /* Notify linkscan of port removal from trunk */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_link_failover_set(unit, failport, FALSE));
            return BCM_E_NONE;
        } else {
            return BCM_E_PORT;
        }
    }

    if (hw_count) { /* Enabling trunk failover */
        /* We must be in directed mirroring mode to use this feature */
        BCM_IF_ERROR_RETURN
            (bcm_esw_switch_control_get(unit, bcmSwitchDirectedMirroring,
                                        &mirror_enable));
        if (!mirror_enable) {
            return BCM_E_CONFIG;
        }

        sal_memset(&fail_entry, 0, sizeof(fail_entry));
        soc_mem_field32_set(unit, PORT_LAG_FAILOVER_SETm,
                (uint32 *)&fail_entry,
                FAILOVER_SET_SIZEf, hw_count - 1);
        soc_mem_field32_set(unit, PORT_LAG_FAILOVER_SETm,
                (uint32 *)&fail_entry, RTAGf, rtag);

        failover_max_portcnt = 1 << soc_mem_field_length(unit,
                PORT_LAG_FAILOVER_SETm, FAILOVER_SET_SIZEf);
        for (ix = 0; ix < failover_max_portcnt; ix++) {
            soc_mem_field32_set(unit, PORT_LAG_FAILOVER_SETm,
                    (uint32 *)&fail_entry,
                    _trunk_failover_portf[ix],
                    ftp[ix % hw_count]);
            soc_mem_field32_set(unit, PORT_LAG_FAILOVER_SETm,
                    (uint32 *)&fail_entry,
                    _trunk_failover_modulef[ix],
                    ftm[ix % hw_count]);
        }
        SOC_IF_ERROR_RETURN
            (WRITE_PORT_LAG_FAILOVER_SETm(unit, MEM_BLOCK_ALL, failport,
                                          &fail_entry));
    }

    lag_failover_config_reg = SOC_REG_IS_VALID(unit, LAG_FAILOVER_CONFIGr) ?
              LAG_FAILOVER_CONFIGr: XLMAC_CTRLr;
    SOC_IF_ERROR_RETURN
        (soc_reg_get(unit, lag_failover_config_reg, failport, 0, &val64));

    old_failover_en = soc_reg64_field32_get(unit, lag_failover_config_reg,
            val64, LAG_FAILOVER_ENf);
    new_failover_en = (hw_count != 0) ? 1 : 0;

    if (soc_reg_field_valid(unit, lag_failover_config_reg,
                LINK_STATUS_SELECTf)) {
        old_link_status_sel = soc_reg64_field32_get(unit,
                lag_failover_config_reg, val64, LINK_STATUS_SELECTf);
        new_link_status_sel = (hw_count != 0) ? 1 : 0;
    } else {
        old_link_status_sel = 0;
        new_link_status_sel = 0;
    }

    if (old_failover_en != new_failover_en ||
        old_link_status_sel != new_link_status_sel) {
        soc_reg64_field32_set(unit, lag_failover_config_reg, &val64,
                LAG_FAILOVER_ENf, new_failover_en);
        if (soc_reg_field_valid(unit, lag_failover_config_reg,
                    LINK_STATUS_SELECTf)) {
            soc_reg64_field32_set(unit, lag_failover_config_reg, &val64,
                    LINK_STATUS_SELECTf, new_link_status_sel);
        }
        SOC_IF_ERROR_RETURN
            (soc_reg_set(unit, lag_failover_config_reg, failport, 0, val64));
    }

    if (SOC_REG_IS_VALID(unit, XLMAC_RX_LSS_CTRLr)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_get(unit, XLMAC_RX_LSS_CTRLr, failport, 0, &val64));
        old_reset_flow_control = soc_reg64_field32_get(unit, XLMAC_RX_LSS_CTRLr,
                val64, RESET_FLOW_CONTROL_TIMERS_ON_LINK_DOWNf);
        new_reset_flow_control = (hw_count != 0) ? 1 : 0;
        if (old_reset_flow_control != new_reset_flow_control) {
            soc_reg64_field32_set(unit, XLMAC_RX_LSS_CTRLr, &val64,
                    RESET_FLOW_CONTROL_TIMERS_ON_LINK_DOWNf, new_reset_flow_control);
            SOC_IF_ERROR_RETURN
                (soc_reg_set(unit, XLMAC_RX_LSS_CTRLr, failport, 0, val64));
        }
    }

    /* Notify linkscan of failover status */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_link_failover_set(unit, failport, (hw_count != 0)));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_read
 * Purpose:
 *      Get front-panel trunk hardware failover config.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      failport - (IN) Port ID of the fail port.
 *      array_size  - (IN) Max number of ports in failover port list.
 *      rtag        - (OUT) Pointer to fail port RTAG. 
 *      ftp         - (OUT) Pointer to port IDs in failover port list.
 *      ftm         - (OUT) Pointer to module IDs in failover port list.
 *      array_count - (OUT) Number of ports in failover port list.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_hwfailover_read(int unit,
        bcm_port_t failport, int array_size,
        int *rtag, bcm_port_t *ftp, bcm_module_t *ftm,
        int *array_count)
{
    port_lag_failover_set_entry_t fail_entry;
    uint64 val64;
    int    ix, hw_count, enable = FALSE;
    soc_reg_t lag_failover_config_reg;

    lag_failover_config_reg = SOC_REG_IS_VALID(unit, LAG_FAILOVER_CONFIGr) ?
              LAG_FAILOVER_CONFIGr: XLMAC_CTRLr;
    SOC_IF_ERROR_RETURN
        (soc_reg_get(unit, lag_failover_config_reg, failport, 0, &val64));
    enable = soc_reg64_field32_get(unit, lag_failover_config_reg, val64,
            LAG_FAILOVER_ENf);

    if (enable) {
        SOC_IF_ERROR_RETURN
            (READ_PORT_LAG_FAILOVER_SETm(unit, MEM_BLOCK_ALL, failport,
                                         &fail_entry));
        hw_count = 1 + soc_mem_field32_get(unit, PORT_LAG_FAILOVER_SETm,
                &fail_entry,
                FAILOVER_SET_SIZEf);
        if ((hw_count > array_size) && (array_size != 0)) {
            hw_count = array_size;
        }
        for (ix = 0; ix < hw_count; ix++) {
            if (NULL != ftp) {
                ftp[ix] = soc_mem_field32_get(unit, PORT_LAG_FAILOVER_SETm,
                        &fail_entry,
                        _trunk_failover_portf[ix]);
            }
            if (NULL != ftm) {
                ftm[ix] = soc_mem_field32_get(unit, PORT_LAG_FAILOVER_SETm,
                        &fail_entry,
                        _trunk_failover_modulef[ix]);
            }
        }
        if (NULL != rtag) {
            *rtag = soc_mem_field32_get(unit, PORT_LAG_FAILOVER_SETm,
                        &fail_entry, RTAGf);
        }
        if (NULL != array_count) {
            *array_count = hw_count;
        }
    } else {
        if (NULL != array_count) {
            *array_count = 0;
        }
        if (NULL != rtag) {
            *rtag = 0;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_trident_trunk_psc_to_rtag
 * Purpose:
 *      Convert PSC to RTAG.
 * Parameters:
 *      unit - (IN) StrataSwitch PCI device unit number (driver internal).
 *      psc  - (OUT) Port selection criteria.
 *      rtag - (OUT) Pointer to RTAG.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_trident_trunk_psc_to_rtag(int unit, int psc, int *rtag)
{
    switch (psc) {
    case BCM_TRUNK_PSC_SRCMAC:
    case BCM_TRUNK_PSC_DSTMAC:
    case BCM_TRUNK_PSC_SRCDSTMAC:
    case BCM_TRUNK_PSC_SRCIP:
    case BCM_TRUNK_PSC_DSTIP:
    case BCM_TRUNK_PSC_SRCDSTIP:
        /* These BCM_TRUNK_PSC_ values match the hardware RTAG values */
        *rtag = psc;
        break;
    case BCM_TRUNK_PSC_PORTINDEX:
        if (!soc_feature(unit, soc_feature_port_trunk_index)) {
            return BCM_E_PARAM;
        }
        *rtag = 7;
        break;
    case BCM_TRUNK_PSC_PORTFLOW:
        if (!soc_feature(unit, soc_feature_port_flow_hash)) {
            return BCM_E_PARAM;
        }
        *rtag = 7;
        break;
    case BCM_TRUNK_PSC_DYNAMIC:
    case BCM_TRUNK_PSC_DYNAMIC_ASSIGNED:
    case BCM_TRUNK_PSC_DYNAMIC_OPTIMAL:
    case BCM_TRUNK_PSC_DYNAMIC_RESILIENT:
        *rtag = 7;
        break;
    default:
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_set
 * Purpose:
 *      Derive hardware failover port list, and
 *      set trunk hardware failover config.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      hg_trunk - (IN) Indicate if trunk group is a Higig trunk.
 *      port     - (IN) Port ID of the fail port.
 *      modid    - (IN) Module ID of the fail port.
 *      psc      - (IN) Port selection criteria for failover port list.
 *      flags    - (IN) BCM_TRUNK_FLAG_FAILOVER_xxx
 *      count    - (IN) Number of ports in failover port list.
 *      ftp      - (IN) Pointer to port IDs in failover port list.
 *      ftm      - (IN) Pointer to module IDs in failover port list.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_hwfailover_set(int unit,
        bcm_trunk_t tid, int hg_trunk,
        bcm_port_t port, bcm_module_t modid,
        int psc, uint32 flags, int count, 
        bcm_port_t *ftp, bcm_module_t *ftm)
{
    bcm_trunk_chip_info_t chip_info;
    int rv = BCM_E_NONE;
    int ix, tix, tid_ix, tports, next, local, mod_is_local, hw_count, rtag;
    uint8 tp_mod;
    uint16 fail_modport;
    bcm_gport_t gport;
    bcm_port_t local_port;
    int failover_max_portcnt;
    bcm_port_t *port_list = NULL;
    bcm_module_t *modid_list = NULL;
    _trident_hw_tinfo_t *hwft;
    
    /* Need the HW port number for the table write later */
    if (hg_trunk) {
        fail_modport = local_port = port;
    } else {
        BCM_GPORT_MODPORT_SET(gport, modid, port);
        BCM_IF_ERROR_RETURN
            (bcm_esw_port_local_get(unit, gport, &local_port));
        fail_modport = (modid << 8) | port;
    }

    BCM_IF_ERROR_RETURN(_bcm_trident_trunk_psc_to_rtag(unit, psc, &rtag));

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    tid_ix = tid + (hg_trunk ? chip_info.trunk_group_count : 0);
    hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid_ix]);

    /* Where is our port in the trunk list? */
    tports = hwft->num_ports;
    for (tix = 0; tix < tports; tix++) {
        if (fail_modport == hwft->modport[tix]) {
            break;
        }
    }

    if ((0 != count) && (tix == tports)) {
        /* Port not in trunk! */
        return BCM_E_NOT_FOUND;
    }

    if (hg_trunk) {
        failover_max_portcnt = 1 << soc_mem_field_length(unit,
                HG_TRUNK_FAILOVER_SETm, FAILOVER_SET_SIZEf);
    } else {
        failover_max_portcnt = 1 << soc_mem_field_length(unit,
                PORT_LAG_FAILOVER_SETm, FAILOVER_SET_SIZEf);
    }
    port_list = sal_alloc(sizeof(bcm_port_t) * failover_max_portcnt,
            "failover port list");
    if (port_list == NULL) {
        return BCM_E_MEMORY;
    }
    modid_list = sal_alloc(sizeof(bcm_module_t) * failover_max_portcnt,
            "failover module list");
    if (modid_list == NULL) {
        sal_free(port_list);
        return BCM_E_MEMORY;
    }

    if (flags) {
        switch (flags) {
            case BCM_TRUNK_FLAG_FAILOVER_NEXT_LOCAL:
                next = TRUE;
                local = TRUE;
                break;
            case BCM_TRUNK_FLAG_FAILOVER_NEXT:
                next = TRUE;
                local = FALSE;
                break;
            case BCM_TRUNK_FLAG_FAILOVER_ALL_LOCAL:
                next = FALSE;
                local = TRUE;
                break;
            case BCM_TRUNK_FLAG_FAILOVER_ALL:
                next = FALSE;
                local = FALSE;
                break;
            default:
                /* Illegal flags setting */
                sal_free(port_list);
                sal_free(modid_list);
                return BCM_E_PARAM;
        }

        hw_count = 0;

        for (ix = ((tix + 1) % tports);  /* Start after fail port */
                ix != tix;                  /* Stop when we get back */
                ix = ((ix + 1) % tports)) { /* Cycle through trunk port list */
            tp_mod = hwft->modport[ix] >> 8;
            if (local && !hg_trunk) {
                rv = _bcm_esw_modid_is_local(unit, tp_mod, &mod_is_local);
                if (BCM_FAILURE(rv)) {
                    sal_free(port_list);
                    sal_free(modid_list);
                    return rv;
                }
                if (!mod_is_local) {
                    continue;
                }
            }
            port_list[hw_count] = hwft->modport[ix] & 0xff;
            modid_list[hw_count] = tp_mod;
            hw_count++;
            if (hw_count == failover_max_portcnt) {
                break;
            }
            if (next) {
                break;
            }
        }
    } else {
        hw_count = 0;

        if (count) {
            for (ix = 0; ix < count && ix < failover_max_portcnt; ix++) {
                port_list[ix] = ftp[ix];
                modid_list[ix] = ftm[ix];
            }
            hw_count = ix;
        }
    }

    if (hg_trunk) {
        rv = _bcm_trident_trunk_hwfailover_hg_write(unit, local_port, rtag,
                hw_count, port_list);
    } else {
        rv = _bcm_trident_trunk_hwfailover_write(unit, local_port, rtag,
                hw_count, port_list, modid_list);
    }
    sal_free(port_list);
    sal_free(modid_list);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if (tix < hwft->num_ports) {
        hwft->flags[tix] = flags;
        hwft->psc[tix] = psc;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_trident_trunk_hwfailover_get
 * Purpose:
 *      Get trunk hardware failover config.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      hg_trunk - (IN) Indicate if trunk group is a Higig trunk.
 *      port     - (IN) Port ID of the fail port.
 *      modid    - (IN) Module ID of the fail port.
 *      psc         - (OUT) Port selection criteria for failover port list.
 *      flags       - (OUT) BCM_TRUNK_FLAG_FAILOVER_xxx
 *      array_size  - (IN) Max number of ports in failover port list.
 *      ftp         - (OUT) Pointer to port IDs in failover port list.
 *      ftm         - (OUT) Pointer to module IDs in failover port list.
 *      array_count - (OUT) Number of ports in failover port list.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_hwfailover_get(int unit,
        bcm_trunk_t tid, int hg_trunk,
        bcm_port_t port, bcm_module_t modid,
        int *psc, uint32 *flags, int array_size,
        bcm_port_t *ftp, bcm_module_t *ftm, 
        int *array_count)
{
    bcm_trunk_chip_info_t chip_info;
    int tix, tid_ix, tports;
    uint16 fail_modport;
    bcm_gport_t gport;
    bcm_port_t local_port;
    _trident_hw_tinfo_t *hwft;

    /* Need the HW port number for the table write later */
    if (hg_trunk) {
        fail_modport = local_port = port;
    } else {
        BCM_GPORT_MODPORT_SET(gport, modid, port);
        BCM_IF_ERROR_RETURN
            (bcm_esw_port_local_get(unit, gport, &local_port));
        fail_modport = (modid << 8) | port;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    tid_ix = tid + (hg_trunk ? chip_info.trunk_group_count : 0);
    hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid_ix]);

    /* Where is our port in the trunk list? */
    tports = hwft->num_ports;
    for (tix = 0; tix < tports; tix++) {
        if (fail_modport == hwft->modport[tix]) {
            break;
        }
    }

    if (tix == tports) {
        /* Port not in trunk! */
        return BCM_E_NOT_FOUND;
    }

    *psc = hwft->psc[tix];

    if (hwft->flags[tix]) {
        /* Just return the flags value */
        *flags = hwft->flags[tix];
        *array_count = 0;
    } else {
        /* Access the HW to get the list */
        *flags = 0;
        if (hg_trunk) {
            BCM_IF_ERROR_RETURN
                (_bcm_trident_trunk_hwfailover_hg_read(unit, local_port,
                                                    array_size, NULL,
                                                    ftp, ftm, array_count));
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_trident_trunk_hwfailover_read(unit, local_port,
                                                    array_size, NULL,
                                                    ftp, ftm, array_count));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_failover_set
 * Purpose:
 *      Disable hardware failover for old trunk ports, and
 *      configure hardware failover for new trunk ports.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      hg_trunk - (IN) Indicate if trunk group is a Higig trunk
 *      add_info - (IN) Pointer to trunk add info structure.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_failover_set(int unit, 
        bcm_trunk_t tid, 
        int hg_trunk,
        _esw_trunk_add_info_t *add_info)
{
    bcm_trunk_chip_info_t chip_info;
    int rv = BCM_E_NONE;
    int tix, mod_is_local, tid_ix;
    bcm_port_t   tp_port;
    bcm_module_t tp_mod = -1;
    _trident_hw_tinfo_t *hwft;
    bcm_port_t *port_array = NULL;
    bcm_module_t *mod_array = NULL;
    int num_hwft_ports;
    int failover_flags;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    tid_ix = tid + (hg_trunk ? chip_info.trunk_group_count : 0);
    hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid_ix]);

    failover_flags = 0;
    if (add_info != NULL) {
        if (add_info->flags & BCM_TRUNK_FLAG_FAILOVER_NEXT) {
            failover_flags = BCM_TRUNK_FLAG_FAILOVER_NEXT;
        } else if (add_info->flags & BCM_TRUNK_FLAG_FAILOVER_NEXT_LOCAL) {
            failover_flags = BCM_TRUNK_FLAG_FAILOVER_NEXT_LOCAL;
        } else if (add_info->flags & BCM_TRUNK_FLAG_FAILOVER_ALL) {
            failover_flags = BCM_TRUNK_FLAG_FAILOVER_ALL;
        } else if (add_info->flags & BCM_TRUNK_FLAG_FAILOVER_ALL_LOCAL) {
            failover_flags = BCM_TRUNK_FLAG_FAILOVER_ALL_LOCAL;
        }
    }

    if ((NULL == add_info) || (0 != failover_flags)) {
        /* Disable hardware failover for old trunk ports */
        for (tix = 0; tix < hwft->num_ports; tix++) {
            if (!hg_trunk) {
                tp_mod = hwft->modport[tix] >> 8;
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_modid_is_local(unit, tp_mod, &mod_is_local));
                if (!mod_is_local) {
                    continue;
                }
            }
            tp_port = hwft->modport[tix] & 0xff;
            BCM_IF_ERROR_RETURN
                (_bcm_trident_trunk_hwfailover_set(unit, tid, hg_trunk,
                                                   tp_port, tp_mod,
                                                   hwft->psc[tix],
                                                   0, 0, NULL, NULL));
        }
    }
    /* Else, updating trunk with failover_flags == 0,
     * do not change failover configuration */

    if (hwft->modport) {
        sal_free(hwft->modport);
        hwft->modport = NULL;
    }
    if (hwft->psc) {
        sal_free(hwft->psc);
        hwft->psc = NULL;
    }
    if (hwft->flags) {
        sal_free(hwft->flags);
        hwft->flags = NULL;
    }
    hwft->num_ports = 0;

    if (NULL == add_info) {
        /* We're deleting the trunk */
        return BCM_E_NONE;
    }

    /* Record new trunk data */

    if (hwft->modport == NULL) {
        hwft->modport = sal_alloc(sizeof(uint16) * add_info->num_ports, "hw_tinfo_modport");
        if (hwft->modport == NULL) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(hwft->modport, 0, sizeof(uint16) * add_info->num_ports);

    if (hwft->psc == NULL) {
        hwft->psc = sal_alloc(sizeof(uint8) * add_info->num_ports, "hw_tinfo_psc");
        if (hwft->psc == NULL) {
            sal_free(hwft->modport);
            hwft->modport = NULL;
            return BCM_E_MEMORY;
        }
    }
    sal_memset(hwft->psc, 0, sizeof(uint8) * add_info->num_ports);

    if (hwft->flags == NULL) {
        hwft->flags = sal_alloc(sizeof(uint32) * add_info->num_ports, "hw_tinfo_flags");
        if (hwft->flags == NULL) {
            sal_free(hwft->modport);
            hwft->modport = NULL;
            sal_free(hwft->psc);
            hwft->psc = NULL;
            return BCM_E_MEMORY;
        }
    }
    sal_memset(hwft->flags, 0, sizeof(uint32) * add_info->num_ports);

    if (mod_array == NULL) {
        mod_array = sal_alloc(sizeof(bcm_module_t) * add_info->num_ports, "mod_array");
        if (mod_array == NULL) {
            sal_free(hwft->modport);
            hwft->modport = NULL;
            sal_free(hwft->psc);
            hwft->psc = NULL;
            sal_free(hwft->flags);
            hwft->flags = NULL;
            return BCM_E_MEMORY;
        }
    }
    sal_memset(mod_array, 0, sizeof(bcm_module_t) * add_info->num_ports);
    
    if (port_array == NULL) {
        port_array = sal_alloc(sizeof(bcm_port_t) * add_info->num_ports, "port_array");
        if (port_array == NULL) {
            sal_free(hwft->modport);
            hwft->modport = NULL;
            sal_free(hwft->psc);
            hwft->psc = NULL;
            sal_free(hwft->flags);
            hwft->flags = NULL;
            sal_free(mod_array);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(port_array, 0, sizeof(bcm_port_t) * add_info->num_ports);

    num_hwft_ports = 0;
    for (tix = 0; tix < add_info->num_ports; tix++) {
        if (add_info->member_flags[tix] & BCM_TRUNK_MEMBER_EGRESS_DISABLE) {
            continue;
        }
        hwft->flags[num_hwft_ports] = failover_flags;
        hwft->psc[num_hwft_ports] = add_info->psc;
        hwft->modport[num_hwft_ports] = add_info->tp[tix];
        port_array[num_hwft_ports] = add_info->tp[tix];
        if (!hg_trunk) {
            hwft->modport[num_hwft_ports] |= (add_info->tm[tix] << 8);
            mod_array[num_hwft_ports] = add_info->tm[tix];
        }
        num_hwft_ports++; 
    }
    hwft->num_ports = num_hwft_ports;

    /* No failover flags in trunk_set means no failover changes */
    if (0 != failover_flags) {
        /* Configure hardware failover for new trunk ports */
        for (tix = 0; tix < hwft->num_ports; tix++) {
            if (!hg_trunk) {
                rv = _bcm_esw_modid_is_local(unit, hwft->modport[tix] >> 8,
                        &mod_is_local);
                if (BCM_FAILURE(rv)) {
                    sal_free(mod_array);
                    sal_free(port_array);
                    return rv;
                }
                if (!mod_is_local) {
                    continue;
                }
            }
            rv = _bcm_trident_trunk_hwfailover_set(unit, tid, hg_trunk, 
                    hwft->modport[tix] & 0xff,
                    hwft->modport[tix] >> 8,
                    hwft->psc[tix],
                    hwft->flags[tix],
                    hwft->num_ports,
                    port_array,
                    mod_array);
            if (BCM_FAILURE(rv)) {
                sal_free(mod_array);
                sal_free(port_array);
                return rv;
            }
        }
    }

    sal_free(mod_array);
    sal_free(port_array);

    return BCM_E_NONE;
}

/* ----------------------------------------------------------------------------- 
 *
 * Routines for software failover
 *
 * ----------------------------------------------------------------------------- 
 */

/*
 * Function:
 *      _bcm_trident_trunk_swfailover_fabric_set
 * Purpose:
 *      Set Higig trunk software failover config.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig trunk ID.
 *      rtag  - (IN) Port selection criteria. 
 *      ports - (IN) Higig trunk member ports.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_swfailover_fabric_set(int unit,
        bcm_trunk_t hgtid,
        int rtag,
        bcm_pbmp_t ports)
{
    bcm_trunk_chip_info_t chip_info;
    _trident_trunk_swfail_t *swf;
    _trident_tinfo_t        *swft;
    bcm_port_t              port;
    int                     i;
    int fabric_trunk_swf_tid_offset = 0;

    if (_trident_trunk_swfail[unit] == NULL) {
        return BCM_E_INIT;
    }

    swf = _trident_trunk_swfail[unit];

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    swft = &swf->tinfo[hgtid + chip_info.trunk_group_count];
    fabric_trunk_swf_tid_offset = chip_info.trunk_group_count + 1;

    swft->rtag = rtag;
    SOC_PBMP_COUNT(ports, swft->num_ports);

    if (swft->modport) {
        sal_free(swft->modport);
        swft->modport = NULL;
    }

    if (swft->num_ports > 0) {
        swft->modport = sal_alloc(sizeof(uint16) * swft->num_ports,
                "swfail_tinfo_modport");
        if (swft->modport == NULL) {
            return BCM_E_MEMORY;
        }

        i = 0;
        SOC_PBMP_ITER(ports, port) {
            swft->modport[i++] = port;
        }
    }

    PBMP_ALL_ITER(unit, port) {
        if (BCM_PBMP_MEMBER(ports, port)) {
            swf->trunk[port] = hgtid + fabric_trunk_swf_tid_offset;
        } else if (swf->trunk[port] ==
                (hgtid + fabric_trunk_swf_tid_offset)) {
            swf->trunk[port] = 0;       /* remove stale pointers to tid */
        }
    }
    return BCM_E_NONE;
}   

/*
 * Function:
 *      _bcm_trident_trunk_swfailover_fp_set
 * Purpose:
 *      Set front panel trunk software failover config.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      tid    - (IN) Trunk ID.
 *      rtag   - (IN) Port selection criteria. 
 *      nports - (IN) Number of trunk members. 
 *      mods   - (IN) Trunk member module ID array.
 *      ports  - (IN) Trunk member port ID array.
 *      member_flags - (IN) Trunk member flags array.
 *      flags  - (IN) Trunk group flags.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_swfailover_fp_set(int unit,
        bcm_trunk_t tid,
        int rtag,
        int nports,
        int *mods,
        int *ports,
        uint32 *member_flags,
        uint32 flags)
{
    _trident_trunk_swfail_t *swf;
    _trident_tinfo_t        *swft;
    bcm_pbmp_t          localports;
    bcm_port_t          port;
    bcm_module_t        my_modid;
    int                 i;

    if (_trident_trunk_swfail[unit] == NULL) {
        return BCM_E_INIT;
    }

    BCM_IF_ERROR_RETURN
        (bcm_esw_stk_my_modid_get(unit, &my_modid));

    swf = _trident_trunk_swfail[unit];
    swft = &swf->tinfo[tid];

    swf->mymodid = my_modid;
    swft->rtag = rtag;
    swft->num_ports = nports;
    swft->flags = flags;

    if (swft->modport) {
        sal_free(swft->modport);
        swft->modport = NULL;
    }
    if (swft->member_flags) {
        sal_free(swft->member_flags);
        swft->member_flags = NULL;
    }

    BCM_PBMP_CLEAR(localports);
    if (swft->num_ports > 0) {
        swft->modport = sal_alloc(sizeof(uint16) * swft->num_ports,
                "swfail_tinfo_modport");
        if (swft->modport == NULL) {
            return BCM_E_MEMORY;
        }
        swft->member_flags = sal_alloc(sizeof(uint32) * swft->num_ports,
                "swfail_tinfo_member_flags");
        if (swft->member_flags == NULL) {
            sal_free(swft->modport);
            swft->modport = NULL;
            return BCM_E_MEMORY;
        }

        for (i = 0; i < nports; i++) {
            swft->modport[i] = (mods[i] << 8) | ports[i];
            if (mods[i] == my_modid) {
                BCM_PBMP_PORT_ADD(localports, ports[i]);
            }
            swft->member_flags[i] = member_flags[i];
        }
    }

    PBMP_ALL_ITER(unit, port) {
        if (BCM_PBMP_MEMBER(localports, port)) {
            swf->trunk[port] = tid + 1;
        } else if (swf->trunk[port] == tid + 1) {
            swf->trunk[port] = 0;       /* remove stale pointers to tid */
        }
    }
    return BCM_E_NONE;
}

#ifdef BCM_TRUNK_FAILOVER_SUPPORT

/*
 * Function:
 *      _bcm_trident_trunk_fabric_swfailover_trigger
 * Purpose:
 *      Delete failed port from Higig trunk config.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      port  - (IN) Failed port number. 
 *      hgtid - (IN) Higig trunk ID.
 *      swf   - (IN) Pointer to software failover info.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_trident_trunk_fabric_swfailover_trigger(int unit,
        bcm_port_t port,
        int hgtid,
        _trident_trunk_swfail_t *swf)
{
    bcm_trunk_chip_info_t chip_info;
    _trident_tinfo_t *swft;
    int i, j, nmp;
    hg_trunk_group_entry_t  hg_trunk_group_entry;
    hg_trunk_member_entry_t hg_trunk_member_entry;
    hg_trunk_bitmap_entry_t hg_trunk_bitmap_entry;
    int old_base_ptr, old_tg_size, old_rtag;
    int max_base_ptr, base_ptr;
    int num_entries, old_num_entries;
    pbmp_t old_trunk_pbmp, new_trunk_pbmp;
    SHR_BITDCL occupied;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    swft = &swf->tinfo[hgtid + chip_info.trunk_group_count];

    /* update the _trident_trunk_swfail entry */
    nmp = swft->num_ports;
    for (i = j = 0; i < nmp; i++) {
        if (swft->modport[i] != port) {
            if (i != j) {
                swft->modport[j] = swft->modport[i];
            }
            j += 1;
        }
    }

    if (j == nmp) {
        return BCM_E_NONE; /* no changes, so no need to write out */
    }

    swft->num_ports = nmp = j;

    /* Update Higig trunk tables */

    SOC_IF_ERROR_RETURN
        (READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid, &hg_trunk_group_entry));
    old_base_ptr = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_trunk_group_entry, BASE_PTRf);
    old_tg_size = 1 + soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_trunk_group_entry, TG_SIZEf);
    old_rtag = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_trunk_group_entry, RTAGf);

    if (nmp == 0) {
        soc_HG_TRUNK_GROUPm_field32_set(unit, &hg_trunk_group_entry, BASE_PTRf, 0);
        soc_HG_TRUNK_GROUPm_field32_set(unit, &hg_trunk_group_entry, TG_SIZEf, 0);
        soc_HG_TRUNK_GROUPm_field32_set(unit, &hg_trunk_group_entry, RTAGf, swft->rtag);
        SOC_IF_ERROR_RETURN
            (WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid, &hg_trunk_group_entry));
    } else {

        /* Find a contiguous region of HG_TRUNK_MEMBERm table that's large
         * enough to hold all of the Higig trunk group's member ports.
         */
        num_entries = nmp;

        /* If RTAG is 1-6, Trident A0 and Katana ignore the TG_SIZE field and 
         * assume the Higig trunk group occupies FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT
         * contiguous entries in HG_TRUNK_MEMBER table.
         */
        if (swft->rtag >= 1 && swft->rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
            if(SOC_IS_KATANAX(unit)) {
                num_entries = _BCM_KATANA_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
            } else
#endif
            if (soc_feature(unit,
                        soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
                num_entries = _BCM_TD_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
            }
        } 

        max_base_ptr = soc_mem_index_count(unit, HG_TRUNK_MEMBERm) - num_entries;
        for (base_ptr = 0; base_ptr <= max_base_ptr; base_ptr++) {
            /* Check if the contiguous region of HG_TRUNK_MEMBERm table from
             * index base_ptr to (base_ptr + num_entries - 1) is free. 
             */
            _BCM_HG_TRUNK_MEMBER_TEST_RANGE(unit, base_ptr, num_entries, occupied); 
            if (!occupied) {
                break;
            }
        }

        if (base_ptr > max_base_ptr) {
            /* A contiguous region of the desired size could not be found in
             * HG_TRUNK_MEMBERm table.
             */
            return BCM_E_RESOURCE;
        }

        for (i = 0; i < num_entries; i++) {
            sal_memset(&hg_trunk_member_entry, 0, sizeof(hg_trunk_member_entry_t));
            soc_HG_TRUNK_MEMBERm_field32_set
                (unit, &hg_trunk_member_entry, PORT_NUMf, swft->modport[i%nmp] & 0xff);
            SOC_IF_ERROR_RETURN
                (WRITE_HG_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL, base_ptr + i,
                                        &hg_trunk_member_entry)); 
        }

        /* Set hg_trunk_member_bitmap */
        _BCM_HG_TRUNK_MEMBER_USED_SET_RANGE(unit, base_ptr, num_entries);

        soc_HG_TRUNK_GROUPm_field32_set
            (unit, &hg_trunk_group_entry, BASE_PTRf, base_ptr);
        soc_HG_TRUNK_GROUPm_field32_set
            (unit, &hg_trunk_group_entry, TG_SIZEf, nmp-1);
        soc_HG_TRUNK_GROUPm_field32_set
            (unit, &hg_trunk_group_entry, RTAGf, swft->rtag);
        SOC_IF_ERROR_RETURN
            (WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid, &hg_trunk_group_entry));
    }

    SOC_IF_ERROR_RETURN
        (READ_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, hgtid, &hg_trunk_bitmap_entry));
    SOC_PBMP_CLEAR(old_trunk_pbmp);
    soc_mem_pbmp_field_get(unit, HG_TRUNK_BITMAPm, &hg_trunk_bitmap_entry,
            HIGIG_TRUNK_BITMAPf, &old_trunk_pbmp);

    SOC_PBMP_ASSIGN(new_trunk_pbmp, old_trunk_pbmp);
    SOC_PBMP_PORT_REMOVE(new_trunk_pbmp, port);

    soc_mem_pbmp_field_set(unit, HG_TRUNK_BITMAPm, &hg_trunk_bitmap_entry,
            HIGIG_TRUNK_BITMAPf, &new_trunk_pbmp);
    SOC_IF_ERROR_RETURN
        (WRITE_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ALL, hgtid, &hg_trunk_bitmap_entry));

    BCM_IF_ERROR_RETURN
        (_bcm_trident_trunk_fabric_port_set(unit,
                                            hgtid,
                                            old_trunk_pbmp,
                                            new_trunk_pbmp));

    /* Clear the previously used region in HG_TRUNK_MEMBERm and hg_trunk_member_bitmap. */
    old_num_entries = old_tg_size;
    if (old_rtag >= 1 && old_rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
        if(SOC_IS_KATANAX(unit)) {
            old_num_entries = _BCM_KATANA_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
        } else
#endif
        if (soc_feature(unit, soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
            old_num_entries = _BCM_TD_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
        }
    } 
    _BCM_HG_TRUNK_MEMBER_USED_CLR_RANGE(unit, old_base_ptr, old_num_entries);
    for (i = 0; i < old_num_entries; i++) {
        sal_memset(&hg_trunk_member_entry, 0, sizeof(hg_trunk_member_entry_t));
        SOC_IF_ERROR_RETURN
            (WRITE_HG_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL, old_base_ptr + i,
                                    soc_mem_entry_null(unit, HG_TRUNK_MEMBERm))); 
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_swfailover_nonuc_update
 * Purpose:
 *      Update non-unicast trunk block mask table for
 *      software failover.
 * Parameters:
 *      unit           - (IN) SOC unit number. 
 *      num_ports      - (IN) Number of ports remaining in the trunk group,
 *                            excluding the failed port. 
 *      swft           - (IN) Software failover trunk group info, excluding
 *                            the failed port.
 *      mymodid        - (IN) Local module ID.
 *      old_trunk_pbmp - (IN) Trunk port bitmap before removing failed port.
 *      new_trunk_pbmp - (IN) Trunk port bitmap after removing failed port.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_swfailover_nonuc_update(int unit,
        int num_ports,
        _trident_tinfo_t *swft,
        int mymodid,
        pbmp_t old_trunk_pbmp,
        pbmp_t new_trunk_pbmp)
{
    int          rv = BCM_E_NONE;
    bcm_module_t *mods = NULL;
    int          i;
    SHR_BITDCL   *nuc_tpbm = NULL;
    SHR_BITDCL   *ipmc_tpbm = NULL;
    SHR_BITDCL   *l2mc_tpbm = NULL;
    SHR_BITDCL   *bcast_tpbm = NULL;
    SHR_BITDCL   *dlf_tpbm = NULL;
    SHR_BITDCL   *ipmc_my_modid_bmap = NULL;
    SHR_BITDCL   *l2mc_my_modid_bmap = NULL;
    SHR_BITDCL   *bcast_my_modid_bmap = NULL;
    SHR_BITDCL   *dlf_my_modid_bmap = NULL;
    pbmp_t       ipmc_trunk_pbmp;
    pbmp_t       l2mc_trunk_pbmp;
    pbmp_t       bcast_trunk_pbmp;
    pbmp_t       dlf_trunk_pbmp;
    uint32       ipmc_member_count;
    uint32       l2mc_member_count;
    uint32       bcast_member_count;
    uint32       dlf_member_count;
    bcm_port_t   loc_port;

    if (num_ports > 0) {
        /* Derive a member bitmap of non-unicast eligible members */
        mods = sal_alloc(sizeof(bcm_module_t) * num_ports, "mods");
        if (mods == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        for (i = 0; i < num_ports; i++) {
            mods[i] = swft->modport[i] >> 8;
        }

        nuc_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "nuc_tpbm");
        if (nuc_tpbm == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(nuc_tpbm, 0, SHR_BITALLOCSIZE(num_ports));
        rv = _bcm_trident_nuc_tpbm_get(unit, num_ports, mods, nuc_tpbm);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }

        /* Copy the non-unicast eligible member bitmap to IPMC, L2MC,
         * broadcast, and DLF eligible member bitmaps.
         */
        ipmc_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "ipmc_tpbm");
        if (ipmc_tpbm == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memcpy(ipmc_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(num_ports));

        l2mc_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "l2mc_tpbm");
        if (l2mc_tpbm == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memcpy(l2mc_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(num_ports));

        bcast_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "bcast_tpbm");
        if (bcast_tpbm == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memcpy(bcast_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(num_ports));

        dlf_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "dlf_tpbm");
        if (dlf_tpbm == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memcpy(dlf_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(num_ports));

        /* Iterate through the trunk member list to get port bitmaps
         * of IPMC/L2MC/BCAST/DLF eligible local members.
         */
        ipmc_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(num_ports),
                "ipmc_my_modid_bmap");
        if (ipmc_my_modid_bmap == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(ipmc_my_modid_bmap, 0, SHR_BITALLOCSIZE(num_ports));

        l2mc_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(num_ports),
                "l2mc_my_modid_bmap");
        if (l2mc_my_modid_bmap == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(l2mc_my_modid_bmap, 0, SHR_BITALLOCSIZE(num_ports));

        bcast_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(num_ports),
                "bcast_my_modid_bmap");
        if (bcast_my_modid_bmap == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(bcast_my_modid_bmap, 0, SHR_BITALLOCSIZE(num_ports));

        dlf_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(num_ports),
                "dlf_my_modid_bmap");
        if (dlf_my_modid_bmap == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(dlf_my_modid_bmap, 0, SHR_BITALLOCSIZE(num_ports));

        SOC_PBMP_CLEAR(ipmc_trunk_pbmp);
        SOC_PBMP_CLEAR(l2mc_trunk_pbmp);
        SOC_PBMP_CLEAR(bcast_trunk_pbmp);
        SOC_PBMP_CLEAR(dlf_trunk_pbmp);
        ipmc_member_count = 0;
        l2mc_member_count = 0;
        bcast_member_count = 0;
        dlf_member_count = 0;

        for (i= 0; i < num_ports; i++) {
            if (swft->member_flags[i] &
                    BCM_TRUNK_MEMBER_IPMC_EGRESS_DISABLE) {
                SHR_BITCLR(ipmc_tpbm, i);
            }
            if (swft->member_flags[i] &
                    BCM_TRUNK_MEMBER_L2MC_EGRESS_DISABLE) {
                SHR_BITCLR(l2mc_tpbm, i);
            }
            if (swft->member_flags[i] &
                    BCM_TRUNK_MEMBER_BCAST_EGRESS_DISABLE) {
                SHR_BITCLR(bcast_tpbm, i);
            }
            if (swft->member_flags[i] &
                    BCM_TRUNK_MEMBER_DLF_EGRESS_DISABLE) {
                SHR_BITCLR(dlf_tpbm, i);
            }

            if (mymodid == (swft->modport[i] >> 8)) {
                /* local port */
                loc_port = swft->modport[i] & 0xff;
                if (SHR_BITGET(ipmc_tpbm, i)) {
                    SOC_PBMP_PORT_ADD(ipmc_trunk_pbmp, loc_port);
                    SHR_BITSET(ipmc_my_modid_bmap, ipmc_member_count);
                }
                if (SHR_BITGET(l2mc_tpbm, i)) {
                    SOC_PBMP_PORT_ADD(l2mc_trunk_pbmp, loc_port);
                    SHR_BITSET(l2mc_my_modid_bmap, l2mc_member_count);
                }
                if (SHR_BITGET(bcast_tpbm, i)) {
                    SOC_PBMP_PORT_ADD(bcast_trunk_pbmp, loc_port);
                    SHR_BITSET(bcast_my_modid_bmap, bcast_member_count);
                }
                if (SHR_BITGET(dlf_tpbm, i)) {
                    SOC_PBMP_PORT_ADD(dlf_trunk_pbmp, loc_port);
                    SHR_BITSET(dlf_my_modid_bmap, dlf_member_count);
                }
            }

            if (SHR_BITGET(ipmc_tpbm, i)) {
                /* Eligible for IPMC forwarding */
                ipmc_member_count++;
            }
            if (SHR_BITGET(l2mc_tpbm, i)) {
                /* Eligible for L2MC forwarding */
                l2mc_member_count++;
            }
            if (SHR_BITGET(bcast_tpbm, i)) {
                /* Eligible for broadcast forwarding */
                bcast_member_count++;
            }
            if (SHR_BITGET(dlf_tpbm, i)) {
                /* Eligible for DLF forwarding */
                dlf_member_count++;
            }
        }

        /* Update trunk block masks */
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_IPMC,
                old_trunk_pbmp,
                new_trunk_pbmp,
                ipmc_trunk_pbmp,
                ipmc_my_modid_bmap,
                ipmc_member_count,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_L2MC,
                old_trunk_pbmp,
                new_trunk_pbmp,
                l2mc_trunk_pbmp,
                l2mc_my_modid_bmap,
                l2mc_member_count,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_BCAST,
                old_trunk_pbmp,
                new_trunk_pbmp,
                bcast_trunk_pbmp,
                bcast_my_modid_bmap,
                bcast_member_count,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_DLF,
                old_trunk_pbmp,
                new_trunk_pbmp,
                dlf_trunk_pbmp,
                dlf_my_modid_bmap,
                dlf_member_count,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    } else {
        SOC_PBMP_CLEAR(ipmc_trunk_pbmp);
        SOC_PBMP_CLEAR(l2mc_trunk_pbmp);
        SOC_PBMP_CLEAR(bcast_trunk_pbmp);
        SOC_PBMP_CLEAR(dlf_trunk_pbmp);
        rv = _bcm_trident_trunk_block_mask(unit, 
                TRUNK_BLOCK_MASK_TYPE_IPMC,
                old_trunk_pbmp,
                new_trunk_pbmp,   
                ipmc_trunk_pbmp,
                NULL,
                0,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        rv = _bcm_trident_trunk_block_mask(unit, 
                TRUNK_BLOCK_MASK_TYPE_L2MC,
                old_trunk_pbmp,
                new_trunk_pbmp,   
                l2mc_trunk_pbmp,
                NULL,
                0,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        rv = _bcm_trident_trunk_block_mask(unit, 
                TRUNK_BLOCK_MASK_TYPE_BCAST,
                old_trunk_pbmp,
                new_trunk_pbmp,   
                bcast_trunk_pbmp,
                NULL,
                0,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        rv = _bcm_trident_trunk_block_mask(unit, 
                TRUNK_BLOCK_MASK_TYPE_DLF,
                old_trunk_pbmp,
                new_trunk_pbmp,   
                dlf_trunk_pbmp,
                NULL,
                0,
                swft->flags);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    }

cleanup:
    if (mods) {
        sal_free(mods);
    }                          
    if (nuc_tpbm) {
        sal_free(nuc_tpbm);
    }
    if (ipmc_tpbm) {
        sal_free(ipmc_tpbm);
    }
    if (l2mc_tpbm) {
        sal_free(l2mc_tpbm);
    }
    if (bcast_tpbm) {
        sal_free(bcast_tpbm);
    }
    if (dlf_tpbm) {
        sal_free(dlf_tpbm);
    }
    if (ipmc_my_modid_bmap) {
        sal_free(ipmc_my_modid_bmap);
    }
    if (l2mc_my_modid_bmap) {
        sal_free(l2mc_my_modid_bmap);
    }
    if (bcast_my_modid_bmap) {
        sal_free(bcast_my_modid_bmap);
    }
    if (dlf_my_modid_bmap) {
        sal_free(dlf_my_modid_bmap);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_trident_trunk_swfailover_trigger
 * Purpose:
 *      Delete failed port from trunk config.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      ports_active - (IN) Port bitmap of active ports. 
 *      ports_status - (IN) Bitmap of ports status. 
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_swfailover_trigger(int unit,
        bcm_pbmp_t ports_active,
        bcm_pbmp_t ports_status)
{
    bcm_trunk_chip_info_t chip_info;
    bcm_port_t           port;
    int                  tid;
    _trident_trunk_swfail_t *swf;
    _trident_tinfo_t        *swft;
    int fabric_trunk_swf_tid_offset;

    swf = _trident_trunk_swfail[unit];

    if (swf == NULL) {
        return BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    fabric_trunk_swf_tid_offset = chip_info.trunk_group_count + 1;

    PBMP_ITER(ports_active, port) {
        if (BCM_PBMP_MEMBER(ports_status, port)) {
            continue;   /* link up */
        }

        /* link down */
        tid = swf->trunk[port];
        swf->trunk[port] = 0;
        if (tid <= 0) {
            continue;   /* already removed */
        }        

        if (tid >= 1 && tid <= chip_info.trunk_group_count) {
            pbmp_t               new_trunk_pbmp;
            pbmp_t               old_trunk_pbmp;
            uint16               modport;
            int                  i, j, nmp, rv;
            int                  uc_nmp;
            int                  old_base_ptr, old_tg_size, old_rtag;
            int                  max_base_ptr, base_ptr;
            int                  num_entries, old_num_entries;
            SHR_BITDCL           occupied;
            trunk_member_entry_t trunk_member_entry;
            trunk_group_entry_t  tg_entry;
            trunk_bitmap_entry_t trunk_bitmap_entry;
            uint16               *uc_modport = NULL;

            tid = tid - 1;
            swft = &swf->tinfo[tid];
            SOC_PBMP_CLEAR(new_trunk_pbmp);

            /* update _trident_trunk_swfail: remove matching ports, update size */

            modport = (swf->mymodid << 8) | port;
            nmp = swft->num_ports;

            for (i = j = 0; i < nmp; i++) {
                if (swft->modport[i] != modport) {
                    if (i != j) {
                        swft->modport[j] = swft->modport[i];
                        swft->member_flags[j] = swft->member_flags[i];
                    }
                    j += 1;
                }
            }
            if (j == nmp) {
                continue;       /* no changes, so no need to write out */
            }
            swft->num_ports = nmp = j;

            /* Get unicast egress trunk member ports */
            uc_modport = sal_alloc(sizeof(uint16) * nmp, "uc_modport");
            if (uc_modport == NULL) {
                return BCM_E_MEMORY;
            }
            uc_nmp = 0;
            for (i = 0; i < nmp; i++) {
                if (swft->member_flags[i] &
                        BCM_TRUNK_MEMBER_UNICAST_EGRESS_DISABLE) {
                    continue;
                }
                uc_modport[uc_nmp++] = swft->modport[i];
            }

            /* Convert _trident_trunk_swfail into trunk_group, trunk_member,
             * and trunk_bitmap tables
             */
            rv = READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &tg_entry);
            if (BCM_FAILURE(rv)) {
                sal_free(uc_modport);
                return rv;
            }
            old_base_ptr = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry,
                    BASE_PTRf);
            old_tg_size = 1 + soc_TRUNK_GROUPm_field32_get(unit, &tg_entry,
                    TG_SIZEf);
            old_rtag = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, RTAGf);

            if (uc_nmp == 0) {
                soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, BASE_PTRf, 0);
                soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, TG_SIZEf, 0);
                soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, RTAGf, 0);
                rv = WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid, &tg_entry);
                if (BCM_FAILURE(rv)) {
                    sal_free(uc_modport);
                    return rv;
                }
            } else {
                /* Find a contiguous region of TRUNK_MEMBERm table that's large
                 * enough to hold all of the trunk group's member ports.
                 */
                num_entries = uc_nmp;

                /* If RTAG is 1-6, Trident A0 and Katana ignore the TG_SIZE field 
                 * and assume the trunk group occupies FP_TRUNK_RTAG1_6_MAX_PORTCNT
                 * contiguous entries in TRUNK_MEMBER table.
                 */
                if (swft->rtag >= 1 && swft->rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
                    if(SOC_IS_KATANAX(unit)) {
                        num_entries = _BCM_KATANA_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
                    } else
#endif
                    if (soc_feature(unit,
                                soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
                        num_entries = _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
                    }
                } 

                max_base_ptr = soc_mem_index_count(unit, TRUNK_MEMBERm) - num_entries;
                for (base_ptr = 0; base_ptr <= max_base_ptr; base_ptr++) {
                    /* Check if the contiguous region of TRUNK_MEMBERm table from
                     * index base_ptr to (base_ptr + num_entries - 1) is free. 
                     */
                    _BCM_FP_TRUNK_MEMBER_TEST_RANGE(unit, base_ptr, num_entries, occupied); 
                    if (!occupied) {
                        break;
                    }
                }

                if (base_ptr > max_base_ptr) {
                    /* A contiguous region of the desired size could not be found in
                     * TRUNK_MEMBERm table.
                     */
                    sal_free(uc_modport);
                    return BCM_E_RESOURCE;
                }

                for (i = 0; i < num_entries; i++) {
                    sal_memset(&trunk_member_entry, 0, sizeof(trunk_member_entry_t));
                    soc_TRUNK_MEMBERm_field32_set(unit, &trunk_member_entry,
                            MODULE_IDf, uc_modport[i%uc_nmp] >> 8);
                    soc_TRUNK_MEMBERm_field32_set(unit, &trunk_member_entry,
                            PORT_NUMf, uc_modport[i%uc_nmp] & 0xff);
                    rv = WRITE_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL, base_ptr + i,
                            &trunk_member_entry); 
                    if (BCM_FAILURE(rv)) {
                        sal_free(uc_modport);
                        return rv;
                    }
                }

                /* Set fp_trunk_member_bitmap */
                _BCM_FP_TRUNK_MEMBER_USED_SET_RANGE(unit, base_ptr, num_entries);

                soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, BASE_PTRf, base_ptr);
                soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, TG_SIZEf, uc_nmp-1);
                soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, RTAGf, swft->rtag);
                rv = WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid, &tg_entry);
                if (BCM_FAILURE(rv)) {
                    sal_free(uc_modport);
                    return rv;
                }
            }

            sal_free(uc_modport);
            uc_modport = NULL;

            rv = READ_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, tid, &trunk_bitmap_entry);
            if (BCM_FAILURE(rv)) {
                return rv;
            }
            SOC_PBMP_CLEAR(old_trunk_pbmp);
            soc_mem_pbmp_field_get(unit, TRUNK_BITMAPm, &trunk_bitmap_entry,
                    TRUNK_BITMAPf, &old_trunk_pbmp);

            SOC_PBMP_ASSIGN(new_trunk_pbmp, old_trunk_pbmp);
            SOC_PBMP_PORT_REMOVE(new_trunk_pbmp, port);
            soc_mem_pbmp_field_set(unit, TRUNK_BITMAPm, &trunk_bitmap_entry,
                    TRUNK_BITMAPf, &new_trunk_pbmp);
            rv = WRITE_TRUNK_BITMAPm(unit, MEM_BLOCK_ALL, tid, &trunk_bitmap_entry);
            if (BCM_FAILURE(rv)) {
                return rv;
            }

            /* Update SOURCE_TRUNK_MAP table */
            rv = _bcm_trident_trunk_set_port_property(unit, swf->mymodid, port, -1);
            if (BCM_FAILURE(rv)) {
                return rv;
            }

            /* Clear the previously used region in TRUNK_MEMBERm and fp_trunk_member_bitmap. */
            if (old_rtag != 0) {
                old_num_entries = old_tg_size;
                if (old_rtag >= 1 && old_rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
                    if (SOC_IS_KATANAX(unit)) {
                        old_num_entries = _BCM_KATANA_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
                    } else
#endif
                    if (soc_feature(unit,
                                soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
                        old_num_entries = _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
                    }
                }

                _BCM_FP_TRUNK_MEMBER_USED_CLR_RANGE(unit, old_base_ptr, old_num_entries);
                for (i = 0; i < old_num_entries; i++) {
                    rv = WRITE_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL, old_base_ptr + i,
                            soc_mem_entry_null(unit, TRUNK_MEMBERm)); 
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }
                }
            }

            /* update non uc trunk block mask table */
            rv = _bcm_trident_trunk_swfailover_nonuc_update(unit, nmp, swft,
                    swf->mymodid, old_trunk_pbmp, new_trunk_pbmp);
            if (BCM_FAILURE(rv)) {
                return rv;
            }

        } else if ((tid >= fabric_trunk_swf_tid_offset) && 
                (tid <= (chip_info.trunk_fabric_id_max + 1))) {
            /* higig trunk */
            BCM_IF_ERROR_RETURN(
                    _bcm_trident_trunk_fabric_swfailover_trigger
                    (unit,
                     port,
                     tid - fabric_trunk_swf_tid_offset,
                     swf));
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRUNK_FAILOVER_SUPPORT */

/* ----------------------------------------------------------------------------- 
 *
 * Routines for front panel trunking 
 *
 * ----------------------------------------------------------------------------- 
 */

/*
 * Function:
 *      _bcm_trident_lag_slb_free_resource
 * Purpose:
 *      Free resources for a LAG static load balancing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      tid - (IN) Trunk group ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_lag_slb_free_resource(int unit, int tid)
{
    trunk_group_entry_t tg_entry;
    int base_ptr;
    int tg_size;
    int rtag;
    int num_entries;
 
    SOC_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &tg_entry));
    base_ptr = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry,
            BASE_PTRf);
    tg_size = 1 + soc_TRUNK_GROUPm_field32_get(unit, &tg_entry,
            TG_SIZEf);
    rtag = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry,
            RTAGf);

    if (rtag != 0) {
        num_entries = tg_size;
        if (rtag >= 1 && rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
            if (SOC_IS_KATANAX(unit)) {
                num_entries = _BCM_KATANA_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
            } else
#endif
            if (soc_feature(unit,
                        soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
                num_entries = _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
            }
        } 

        _BCM_FP_TRUNK_MEMBER_USED_CLR_RANGE(unit, base_ptr, num_entries);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_lag_slb_set
 * Purpose:
 *      Configure a LAG static load balancing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      tid   - (IN) Trunk group ID.
 *      num_uc_egr_ports - (IN) Number of trunk member ports eligible for
 *                              unicast egress
 *      uc_egr_hwmod  - (IN) Array of module IDs
 *      uc_egr_hwport - (IN) Array of port IDs
 *      t_info        - (IN) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_lag_slb_set(int unit, int tid, int num_uc_egr_ports,
        int *uc_egr_hwmod, int *uc_egr_hwport, trunk_private_t *t_info)
{
    int i;
    int num_entries;
    int max_base_ptr;
    int base_ptr;
    SHR_BITDCL occupied;
    trunk_member_entry_t trunk_member_entry;
    trunk_group_entry_t  tg_entry;

    if (num_uc_egr_ports == 0) {
        SOC_IF_ERROR_RETURN
            (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &tg_entry));
        soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, BASE_PTRf, 0);
        soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, TG_SIZEf, 0);
        soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, RTAGf, 0);
        SOC_IF_ERROR_RETURN
            (WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid, &tg_entry));
        return BCM_E_NONE;
    }

    /* Find a contiguous region of TRUNK_MEMBERm table that's large
     * enough to hold all of the trunk group's egress member ports.
     */

    num_entries = num_uc_egr_ports;

    /* If RTAG is 1-6, Trident A0 and Katana ignore the TG_SIZE field and 
     * assume the trunk group occupies FP_TRUNK_RTAG1_6_MAX_PORTCNT
     * contiguous entries in TRUNK_MEMBER table.
     */
    if (t_info->rtag >= 1 && t_info->rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANAX(unit)) {
            num_entries = _BCM_KATANA_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
        } else
#endif
        if (soc_feature(unit,
                    soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
            num_entries = _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
        }
    } 

    max_base_ptr = soc_mem_index_count(unit, TRUNK_MEMBERm) - num_entries;
    for (base_ptr = 0; base_ptr <= max_base_ptr; base_ptr++) {
        /* Check if the contiguous region of TRUNK_MEMBERm table from
         * index base_ptr to (base_ptr + num_entries - 1) is free. 
         */
        _BCM_FP_TRUNK_MEMBER_TEST_RANGE(unit, base_ptr, num_entries, occupied); 
        if (!occupied) {
            break;
        }
    }

    if (base_ptr > max_base_ptr) {
        /* A contiguous region of the desired size could not be found in
         * TRUNK_MEMBERm table.
         */
        return BCM_E_RESOURCE;
    }

    for (i = 0; i < num_entries; i++) {
        sal_memset(&trunk_member_entry, 0, sizeof(trunk_member_entry_t));
        soc_TRUNK_MEMBERm_field32_set
            (unit, &trunk_member_entry, MODULE_IDf,
             uc_egr_hwmod[i % num_uc_egr_ports]);
        soc_TRUNK_MEMBERm_field32_set
            (unit, &trunk_member_entry, PORT_NUMf,
             uc_egr_hwport[i % num_uc_egr_ports]);
        SOC_IF_ERROR_RETURN(WRITE_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL,
                    base_ptr + i, &trunk_member_entry)); 
    }

    /* Set fp_trunk_member_bitmap */
    _BCM_FP_TRUNK_MEMBER_USED_SET_RANGE(unit, base_ptr, num_entries);

    /* Update TRUNK_GROUP table */

    SOC_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &tg_entry));
    soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, BASE_PTRf, base_ptr);
    soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, TG_SIZEf,
            num_uc_egr_ports-1);
    soc_TRUNK_GROUPm_field32_set(unit, &tg_entry, RTAGf, t_info->rtag);
    SOC_IF_ERROR_RETURN
        (WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid, &tg_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_block_mask
 * Purpose:
 *      Update the trunk block mask table.
 *      Old ports are cleared and new ports are added to each entry.
 *      Each hash index location entry is owned by a specific unit/modid
 *      across all units in a stack. Hash index location entry owned by
 *      local unit/modid is provided by my_modid_bmap. member_count is
 *      the number of NUC forwarding eligible ports in the trunk group.
 *      Each eligible entry also has a rolling port that is not masked
 *      from among the new NUC forwarding eligible ports (nuc_ports).
 *      This is what spreads non-unicast traffic across multiple ports.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      nuc_type - (IN) Type of non-unicast: IPMC, L2MC, BCAST, or DLF.
 *      old_ports - (IN) Old local trunk member ports.
 *      new_ports - (IN) New local trunk member ports.
 *      nuc_ports - (IN) New local trunk member ports eligible for non-unicast.
 *      my_modid_bmap - (IN) Bitmap indicating which non-unicast eligible trunk
 *                           member port is local.
 *      member_count  - (IN) Number of new non-unicast eligible trunk member
 *                           ports, including both local and remote members.
 *      flags - (IN) Trunk group flags.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_block_mask(int unit,
        int nuc_type,
        pbmp_t old_ports,
        pbmp_t new_ports,
        pbmp_t nuc_ports,
        SHR_BITDCL *my_modid_bmap,
        uint32 member_count,
        uint32 flags)
{
    bcm_port_t port;
    int        localcount, localports[SOC_MAX_NUM_PORTS];
    int        all_local;
    int        i, imin, imax, rv;
    int        index_count; 
    soc_mem_t  mem;
    bcm_pbmp_t pbmp, old_pbmp;
    nonucast_trunk_block_mask_entry_t mask_entry;
    int        region_size;

    rv = BCM_E_NONE;

    localcount = 0;
    all_local = FALSE;
    if (SOC_PBMP_NOT_NULL(nuc_ports)) {
        PBMP_ITER(nuc_ports, port) {
            localports[localcount++] = port;
        }
        if (localcount == member_count) {
            all_local = TRUE;
        }
    }

    /* The NONUCAST_TRUNK_BLOCK_MASK table is divided into 4 regions:
     * IPMC, L2MC, broadcast, and unknown unicast/multicast.
     */
    mem = NONUCAST_TRUNK_BLOCK_MASKm;
    region_size = soc_mem_index_count(unit, mem) / 4;
    if (nuc_type == TRUNK_BLOCK_MASK_TYPE_IPMC) {
        imin = 0;
    } else if (nuc_type == TRUNK_BLOCK_MASK_TYPE_L2MC) {
        imin = region_size;
    } else if (nuc_type == TRUNK_BLOCK_MASK_TYPE_BCAST) {
        imin = region_size * 2;
    } else if (nuc_type == TRUNK_BLOCK_MASK_TYPE_DLF) {
        imin = region_size * 3;
    } else {
        return BCM_E_PARAM;
    }
    imax = imin + region_size - 1;
    soc_mem_lock(unit, mem);
    index_count = 0;
    for (i = imin; i <= imax; i++) {
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, i, &mask_entry);
        if (rv < 0) {
            break;
        }
        soc_mem_pbmp_field_get(unit, mem, &mask_entry, BLOCK_MASKf, &pbmp);
        SOC_PBMP_ASSIGN(old_pbmp, pbmp);
        SOC_PBMP_REMOVE(pbmp, old_ports);
        if (nuc_type == TRUNK_BLOCK_MASK_TYPE_IPMC &&
                (flags & BCM_TRUNK_FLAG_IPMC_CLEAVE)) {
            /* If the flag BCM_TRUNK_FLAG_IPMC_CLEAVE is set,
             * IPMC trunk resolution is disabled, and the bits in
             * pbmp corresponding to new_ports are not set.
             */
        } else {
            SOC_PBMP_OR(pbmp, new_ports);
            if (localcount > 0) {
                if (all_local || (member_count > BCM_SWITCH_TRUNK_MAX_PORTCNT)) {
                    if (SHR_BITGET(my_modid_bmap,
                                   ((i % region_size) % member_count))) {
                        SOC_PBMP_PORT_REMOVE(pbmp,
                                localports[index_count % localcount]);
                        index_count++;
                    }
                } else {
                    /* Not all trunk members are local, and number of
                     * members is BCM_SWITCH_TRUNK_MAX_PORTCNT or less.
                     * We need to conservatively configure
                     * NONUCAST_TRUNK_BLOCK_MASK table to interoperate
                     * with pre-Trident XGS3 devices.
                     */
                    if ((i % 16) == 0) {
                        index_count = 0;
                    }
                    if (SHR_BITGET(my_modid_bmap, ((i % 16) % member_count))) {
                        SOC_PBMP_PORT_REMOVE(pbmp,
                                localports[index_count % localcount]);
                        index_count++;
                    }
                }
            }
        }
        if (SOC_PBMP_EQ(pbmp, old_pbmp)) {
            continue;
        }
        soc_mem_pbmp_field_set(unit, mem, &mask_entry, BLOCK_MASKf, &pbmp);
        rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, i, &mask_entry);
        if (rv < 0) {
            break;
        }
    }
    soc_mem_unlock(unit, mem);

    return rv;
}

/*
 * Function:
 *      _bcm_trident_trunk_mod_port_map_set
 * Purpose:
 *      Set  (mod, port) to  trunk id map.
 *      Map implements a software cache for the SOURCE_TRUNK_MAP_TABLE.
 * Parameters:
 *      unit     -  (IN) BCM device number. 
 *      modid    -  (IN) Member port module id.  
 *      port     -  (IN) Member port number.  
 *      trunk_id -  (IN) Trunk number or negative value for reset.  
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_trident_trunk_mod_port_map_set(int unit,
        bcm_module_t modid, 
        bcm_port_t port,
        bcm_trunk_t trunk_id)  
{
    int   idx;               /* Map array index. */

    if (NULL == _trident_mod_port_to_tid_map[unit]) {
        return (BCM_E_INIT);
    }

    /* Trunk id range check, only 16 bits are allocated per trunk ID in
     * _trident_mod_port_to_tid_map
     */
    if (trunk_id >= 0xffff) {
        return (BCM_E_INTERNAL);
    }

    idx = modid * SOC_MAX_NUM_PORTS + port;
    if (idx >= _BCM_XGS3_TRUNK_MOD_PORT_MAP_IDX_COUNT(unit)) { 
        return (BCM_E_PARAM);
    }

    _trident_mod_port_to_tid_map[unit][idx] = (trunk_id < 0) ? 0 : (++trunk_id);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_trident_trunk_mod_port_map_get
 * Purpose:
 *      Read mapping (mod, port) to trunk id from software cache 
 *      for the SOURCE_TRUNK_MAP_TABLE.
 * Parameters:
 *      unit     -  (IN) BCM device number. 
 *      modid    -  (IN) Member port module id.  
 *      port     -  (IN) Member port number.  
 *      trunk_id -  (OUT) Trunk number or negative value for reset.  
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_trident_trunk_mod_port_map_get(int unit,
        bcm_module_t modid, 
        bcm_port_t port,
        bcm_trunk_t *trunk_id)  
{
    int              idx;      /* Map array index.         */
    bcm_module_t     mymodid;  /* Local module id.         */
    int              isLocal;  /* Local module id flag.    */
    int              rv;       /* Operation return status. */

    if (NULL == _trident_mod_port_to_tid_map[unit]) {
        return (BCM_E_INIT);
    }

    rv = _bcm_esw_modid_is_local(unit, modid, &isLocal);
    BCM_IF_ERROR_RETURN(rv);

    if (isLocal) {
        rv = bcm_esw_stk_my_modid_get(unit, &mymodid);
        BCM_IF_ERROR_RETURN(rv);
        if (mymodid != modid) {
            modid = mymodid;
            port += 32;
        }
    }

    idx = modid * SOC_MAX_NUM_PORTS + port;
    if (idx >= _BCM_XGS3_TRUNK_MOD_PORT_MAP_IDX_COUNT(unit)) { 
        return (BCM_E_PARAM);
    }

    *trunk_id = _trident_mod_port_to_tid_map[unit][idx] - 1;

    return (_trident_mod_port_to_tid_map[unit][idx]) ? BCM_E_NONE : BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_trident_trunk_set_port_property
 * Purpose:
 *      Mark a mod/port pair as being in a particular trunk (or not)
 *      A negative tid sets the mod/port back to non-trunk status.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      mod  - (IN) Module ID.
 *      port - (IN) Port number.
 *      tid  - (IN) Trunk group ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_set_port_property(int unit,
        bcm_module_t mod,
        bcm_port_t port,
        int tid)
{
    int       ptype_get, ptype_set, tid_get, tid_set;
    int       idx;
    soc_mem_t mem;
    source_trunk_map_table_entry_t stm;
    egr_gpp_attributes_entry_t ega;
    int       rv;

    if (tid < 0) {
        ptype_set = 0;                /* normal port */
        tid_set = 0;
    } else {
        ptype_set = 1;                /* trunk port */
        tid_set = tid;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_src_mod_port_table_index_get(unit, mod, port, &idx));

    mem = SOURCE_TRUNK_MAP_TABLEm;
    soc_mem_lock(unit, mem);
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, idx, &stm);
    if (BCM_SUCCESS(rv)) {
        ptype_get = soc_mem_field32_get(unit, mem, &stm, PORT_TYPEf);
        tid_get = soc_mem_field32_get(unit, mem, &stm, TGIDf);
        if ((ptype_get != ptype_set) || (tid_get != tid_set)) {
            soc_mem_field32_set(unit, mem, &stm, PORT_TYPEf, ptype_set);
            soc_mem_field32_set(unit, mem, &stm, TGIDf, tid_set);
            rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, idx, &stm);
        }
    }
    soc_mem_unlock(unit, mem);

    if(!SOC_IS_KATANAX(unit)) {
        mem = EGR_GPP_ATTRIBUTESm;
        soc_mem_lock(unit, mem);
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, idx, &ega);
        if (BCM_SUCCESS(rv)) {
            ptype_get = soc_mem_field32_get(unit, mem, &ega, ISTRUNKf);
            tid_get = soc_mem_field32_get(unit, mem, &ega, TGIDf);
            if ((ptype_get != ptype_set) || (tid_get != tid_set)) {
                soc_mem_field32_set(unit, mem, &ega, ISTRUNKf, ptype_set);
                soc_mem_field32_set(unit, mem, &ega, TGIDf, tid_set);
                rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, idx, &ega);
            }
        }
        soc_mem_unlock(unit, mem);
    }
    /* Update SW module, port to tid mapping. */
    if (BCM_SUCCESS(rv)) {
        rv = _bcm_trident_trunk_mod_port_map_set(unit, mod, port, tid);  
    }

    return rv;
}

/*
 * Function:
 *      _bcm_trident_trunk_get_port_property
 * Purpose:
 *      Get trunk group ID for a mod/port pair.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      mod  - (IN) Module ID.
 *      port - (IN) Port number.
 *      tid  - (OUT) Trunk group ID.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_get_port_property(int unit,
        bcm_module_t mod,
        bcm_port_t port,
        int *tid)
{
    return  _bcm_trident_trunk_mod_port_map_get(unit, mod, port, tid);  
}

/*
 * Function:
 *      _bcm_trident_nuc_tpbm_get
 * Purpose:
 *      Get non-unicast panel trunk members.
 * Parameters:
 *      unit      - (IN) SOC unit number. 
 *      num_ports - (IN) Number of ports in trunk group.
 *      tm        - (IN) Pointer to array of module IDs.
 *      nuc_tpbm  - (OUT) Bitmap of ports eligible for non-unicast.
 * Returns:
 *      BCM_E_xxx
 */
STATIC
int _bcm_trident_nuc_tpbm_get(int unit,
                      int num_ports,
                      bcm_module_t *tm,
                      SHR_BITDCL *nuc_tpbm)
{
    int    rv = BCM_E_NONE;
    int    i, mod = -1;
    uint32 mod_type;
    int    all_equal = 1;

    SHR_BITDCL *xgs12_tpbm = NULL;
    SHR_BITDCL *xgs3_tpbm = NULL;
    SHR_BITDCL *unknown_tpbm = NULL;

    SHR_BITDCL xgs12_tpbm_result;
    SHR_BITDCL xgs3_tpbm_result;
    SHR_BITDCL unknown_tpbm_result;

    xgs12_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "xgs12_tpbm");
    if (xgs12_tpbm == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(xgs12_tpbm, 0, SHR_BITALLOCSIZE(num_ports));

    xgs3_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "xgs3_tpbm");
    if (xgs3_tpbm == NULL) {
        sal_free(xgs12_tpbm);
        return BCM_E_MEMORY;
    }
    sal_memset(xgs3_tpbm, 0, SHR_BITALLOCSIZE(num_ports));

    unknown_tpbm = sal_alloc(SHR_BITALLOCSIZE(num_ports), "unknown_tpbm");
    if (unknown_tpbm == NULL) {
        sal_free(xgs12_tpbm);
        sal_free(xgs3_tpbm);
        return BCM_E_MEMORY;
    }
    sal_memset(unknown_tpbm, 0, SHR_BITALLOCSIZE(num_ports));

    SHR_BITSET(nuc_tpbm, 0);
    for (i = 0; i < num_ports; i++) {
        if (i == 0) {
            mod = tm[i];
        } else if (mod != tm[i]) {
            all_equal = 0;
        }
        rv = _bcm_switch_module_type_get(unit, tm[i], &mod_type);
        if (BCM_FAILURE(rv)) {
            sal_free(xgs12_tpbm);
            sal_free(xgs3_tpbm);
            sal_free(unknown_tpbm);
            return rv;
        }
        switch(mod_type) {
            case BCM_SWITCH_MODULE_XGS1:
                /* passthru */
            case BCM_SWITCH_MODULE_XGS2:
                SHR_BITSET(xgs12_tpbm, i);
                break;
            case BCM_SWITCH_MODULE_XGS3:
                SHR_BITSET(xgs3_tpbm, i);
                break;
            case BCM_SWITCH_MODULE_UNKNOWN:
                /* passthru */
            default:
                SHR_BITSET(unknown_tpbm, i);
                break;
        }
    }

    /* tpbm_result will be 0 if all bits in the range are 0 */
    SHR_BITTEST_RANGE(xgs12_tpbm, 0, num_ports, xgs12_tpbm_result);
    SHR_BITTEST_RANGE(xgs3_tpbm, 0, num_ports, xgs3_tpbm_result);
    SHR_BITTEST_RANGE(unknown_tpbm, 0, num_ports, unknown_tpbm_result);

    if (all_equal) {
        SHR_BITSET_RANGE(nuc_tpbm, 0, num_ports);
    } else if (xgs12_tpbm_result || unknown_tpbm_result) {
        SHR_BITSET(nuc_tpbm, 0);
    } else if (xgs3_tpbm_result) {
        SHR_BITCOPY_RANGE(nuc_tpbm, 0, xgs3_tpbm, 0, num_ports);
    }

    sal_free(xgs12_tpbm);
    sal_free(xgs3_tpbm);
    sal_free(unknown_tpbm);

    return rv;
}

/* Macro to free memory and return code */

#define _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(_r_) \
    if (nuc_tpbm) {                                    \
        sal_free(nuc_tpbm);                            \
    }                                                  \
    if (ipmc_tpbm) {                                   \
        sal_free(ipmc_tpbm);                           \
    }                                                  \
    if (l2mc_tpbm) {                                   \
        sal_free(l2mc_tpbm);                           \
    }                                                  \
    if (bcast_tpbm) {                                  \
        sal_free(bcast_tpbm);                          \
    }                                                  \
    if (dlf_tpbm) {                                    \
        sal_free(dlf_tpbm);                            \
    }                                                  \
    if (ipmc_my_modid_bmap) {                          \
        sal_free(ipmc_my_modid_bmap);                  \
    }                                                  \
    if (l2mc_my_modid_bmap) {                          \
        sal_free(l2mc_my_modid_bmap);                  \
    }                                                  \
    if (bcast_my_modid_bmap) {                         \
        sal_free(bcast_my_modid_bmap);                 \
    }                                                  \
    if (dlf_my_modid_bmap) {                          \
        sal_free(dlf_my_modid_bmap);                  \
    }                                                  \
    if (hwmod) {                                       \
        sal_free(hwmod);                               \
    }                                                  \
    if (hwport) {                                      \
        sal_free(hwport);                              \
    }                                                  \
    if (egr_hwmod) {                                   \
        sal_free(egr_hwmod);                           \
    }                                                  \
    if (egr_hwport) {                                  \
        sal_free(egr_hwport);                          \
    }                                                  \
    if (egr_member_flags) {                            \
        sal_free(egr_member_flags);                    \
    }                                                  \
    if (egr_scaling_factor) {                          \
        sal_free(egr_scaling_factor);                  \
    }                                                  \
    if (egr_load_weight) {                             \
        sal_free(egr_load_weight);                     \
    }                                                  \
    if (uc_egr_hwmod) {                                \
        sal_free(uc_egr_hwmod);                        \
    }                                                  \
    if (uc_egr_hwport) {                               \
        sal_free(uc_egr_hwport);                       \
    }                                                  \
    if (uc_egr_scaling_factor) {                       \
        sal_free(uc_egr_scaling_factor);               \
    }                                                  \
    if (uc_egr_load_weight) {                          \
        sal_free(uc_egr_load_weight);                  \
    }                                                  \
    if (leaving_members) {                             \
        sal_free(leaving_members);                     \
    }                                                  \
    if (staying_members) {                             \
        sal_free(staying_members);                     \
    }                                                  \
    if (joining_members) {                             \
        sal_free(joining_members);                     \
    }                                                  \
    return _r_;                             

/*
 * Function:
 *      _bcm_trident_trunk_fp_set
 * Purpose:
 *      Set front panel trunk members.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure.
 *      t_info   - (IN) Pointer to trunk info.
 *      op       - (IN) Type of operation: TRUNK_MEMBER_OP_SET, ADD or DELETE.
 *      member   - (IN) Member to add or delete.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_fp_modify(int unit,
        bcm_trunk_t tid,
        _esw_trunk_add_info_t *add_info,
        trunk_private_t *t_info,
        int op,
        bcm_trunk_member_t *member)
{
    int                  rv = BCM_E_NONE;
    SHR_BITDCL           *nuc_tpbm = NULL;
    SHR_BITDCL           *ipmc_tpbm = NULL;
    SHR_BITDCL           *l2mc_tpbm = NULL;
    SHR_BITDCL           *bcast_tpbm = NULL;
    SHR_BITDCL           *dlf_tpbm = NULL;
    SHR_BITDCL           *ipmc_my_modid_bmap = NULL;
    SHR_BITDCL           *l2mc_my_modid_bmap = NULL;
    SHR_BITDCL           *bcast_my_modid_bmap = NULL;
    SHR_BITDCL           *dlf_my_modid_bmap = NULL;
    int                  *hwmod = NULL;
    int                  *hwport = NULL;
    int                  num_egr_ports = 0;
    int                  *egr_hwmod = NULL;
    int                  *egr_hwport = NULL;
    uint32               *egr_member_flags = NULL;
    int                  *egr_scaling_factor = NULL;
    int                  *egr_load_weight = NULL;
    int                  num_uc_egr_ports = 0;
    int                  *uc_egr_hwmod = NULL;
    int                  *uc_egr_hwport = NULL;
    int                  *uc_egr_scaling_factor = NULL;
    int                  *uc_egr_load_weight = NULL;
    int                  negr_disable_port = 0;
    int                  num_uc_egr_disable_port = 0;
    pbmp_t               new_trunk_pbmp;
    pbmp_t               ipmc_trunk_pbmp;
    pbmp_t               l2mc_trunk_pbmp;
    pbmp_t               bcast_trunk_pbmp;
    pbmp_t               dlf_trunk_pbmp;
    uint32               ipmc_member_count;
    uint32               l2mc_member_count;
    uint32               bcast_member_count;
    uint32               dlf_member_count;
    int                  mod, port, mod_out, port_out;
    bcm_gport_t          gport;
    bcm_port_t           loc_port;
    int                  i;
    trunk_bitmap_entry_t trunk_bitmap_entry;
    pbmp_t               old_trunk_pbmp;
    int                  j, k, remove_trunk_port;
    int                  num_leaving_members = 0;
    bcm_gport_t          *leaving_members = NULL;
    int                  match_leaving_members;
    int                  num_staying_members = 0;
    bcm_gport_t          *staying_members = NULL;
    int                  match_staying_members;
    int                  num_joining_members = 0;
    bcm_gport_t          *joining_members = NULL;
    int                  match_joining_members;
    bcm_trunk_chip_info_t chip_info;
    int                  is_local;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (add_info->num_ports > chip_info.trunk_ports_max ||
            add_info->num_ports < 1) {
        return BCM_E_PARAM;
    }
 
    /* On Trident A0, the max number of trunk members for RTAG 1-6 is smaller
     * than for RTAG 7.
     */
    if (soc_feature(unit, soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
        if (t_info->rtag >= 1 && t_info->rtag <= 6) {
            if (add_info->num_ports > _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT) {
                return BCM_E_PARAM;
            }
        }
    }

    /* Audit unsupported flag combination */
    for (i = 0; i < add_info->num_ports; i++) {
        if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_EGRESS_DISABLE) {
            if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_INGRESS_DISABLE) {
                return BCM_E_PARAM;
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_UNICAST_EGRESS_DISABLE) {
                return BCM_E_PARAM;
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_IPMC_EGRESS_DISABLE) {
                return BCM_E_PARAM;
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_L2MC_EGRESS_DISABLE) {
                return BCM_E_PARAM;
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_BCAST_EGRESS_DISABLE) {
                return BCM_E_PARAM;
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_DLF_EGRESS_DISABLE) {
                return BCM_E_PARAM;
            }
        }
        if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_IPMC_EGRESS_DISABLE) {
            if (add_info->ipmc_index != BCM_TRUNK_UNSPEC_INDEX) {
                return BCM_E_PARAM;
            }
        }
        if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_L2MC_EGRESS_DISABLE) {
            if (add_info->mc_index != BCM_TRUNK_UNSPEC_INDEX) {
                return BCM_E_PARAM;
            }
        }
        if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_BCAST_EGRESS_DISABLE) {
            if (add_info->dlf_index != BCM_TRUNK_UNSPEC_INDEX) {
                return BCM_E_PARAM;
            }
        }
        if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_DLF_EGRESS_DISABLE) {
            if (add_info->dlf_index != BCM_TRUNK_UNSPEC_INDEX) {
                return BCM_E_PARAM;
            }
        }
    }

    if (add_info->dlf_index >= add_info->num_ports) {
        return BCM_E_PARAM;
    }

    /* Derive a member bitmap of non-unicast eligible members */
    nuc_tpbm = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports), "nuc_tpbm");
    if (nuc_tpbm == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(nuc_tpbm, 0, SHR_BITALLOCSIZE(add_info->num_ports));
    if (add_info->dlf_index == -1) {
        rv = _bcm_trident_nuc_tpbm_get(unit, add_info->num_ports, 
                add_info->tm, nuc_tpbm);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
    } else {
        SHR_BITSET(nuc_tpbm, add_info->dlf_index);
    }

    /* Copy the non-unicast eligible member bitmap to IPMC, L2MC, broadcast,
     * and DLF eligible member bitmaps.
     */
    ipmc_tpbm = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports), "ipmc_tpbm");
    if (ipmc_tpbm == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memcpy(ipmc_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(add_info->num_ports));

    l2mc_tpbm = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports), "l2mc_tpbm");
    if (l2mc_tpbm == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memcpy(l2mc_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(add_info->num_ports));

    bcast_tpbm = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports), "bcast_tpbm");
    if (bcast_tpbm == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memcpy(bcast_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(add_info->num_ports));

    dlf_tpbm = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports), "dlf_tpbm");
    if (dlf_tpbm == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memcpy(dlf_tpbm, nuc_tpbm, SHR_BITALLOCSIZE(add_info->num_ports));

    /* Iterate through the trunk member list to get module ID and port
     * number of each member, a port bitmap of local members, and
     * port bitmaps of IPMC/L2MC/BCAST/DLF eligible local members.
     */
    hwmod = sal_alloc(sizeof(int) * (add_info->num_ports), "hwmod");
    if (hwmod == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(hwmod, 0, sizeof(int) * (add_info->num_ports));

    hwport = sal_alloc(sizeof(int) * (add_info->num_ports), "hwport");
    if (hwport == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(hwport, 0, sizeof(int) * (add_info->num_ports));

    ipmc_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports),
            "ipmc_my_modid_bmap");
    if (ipmc_my_modid_bmap == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(ipmc_my_modid_bmap, 0, SHR_BITALLOCSIZE(add_info->num_ports));

    l2mc_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports),
            "l2mc_my_modid_bmap");
    if (l2mc_my_modid_bmap == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(l2mc_my_modid_bmap, 0, SHR_BITALLOCSIZE(add_info->num_ports));

    bcast_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports),
            "bcast_my_modid_bmap");
    if (bcast_my_modid_bmap == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(bcast_my_modid_bmap, 0, SHR_BITALLOCSIZE(add_info->num_ports));

    dlf_my_modid_bmap = sal_alloc(SHR_BITALLOCSIZE(add_info->num_ports),
            "dlf_my_modid_bmap");
    if (dlf_my_modid_bmap == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(dlf_my_modid_bmap, 0, SHR_BITALLOCSIZE(add_info->num_ports));

    SOC_PBMP_CLEAR(new_trunk_pbmp);
    SOC_PBMP_CLEAR(ipmc_trunk_pbmp);
    SOC_PBMP_CLEAR(l2mc_trunk_pbmp);
    SOC_PBMP_CLEAR(bcast_trunk_pbmp);
    SOC_PBMP_CLEAR(dlf_trunk_pbmp);
    ipmc_member_count = 0;
    l2mc_member_count = 0;
    bcast_member_count = 0;
    dlf_member_count = 0;
    negr_disable_port = 0;
    num_uc_egr_disable_port = 0;

    for (i = 0; i < add_info->num_ports; i++) {
        mod = add_info->tm[i];
        port = add_info->tp[i];
        rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET,
                                    mod, port,
                                    &mod_out, &port_out);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
        if (!SOC_MODID_ADDRESSABLE(unit, mod_out)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_BADID);
        }
        if (!SOC_PORT_ADDRESSABLE(unit, port_out)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_PORT);
        }
        hwmod[i] = mod_out;
        hwport[i] = port_out;

        if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_EGRESS_DISABLE) {
            SHR_BITCLR(ipmc_tpbm, i);
            SHR_BITCLR(l2mc_tpbm, i);
            SHR_BITCLR(bcast_tpbm, i);
            SHR_BITCLR(dlf_tpbm, i);
            negr_disable_port++;
        } else {
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_UNICAST_EGRESS_DISABLE) {
                num_uc_egr_disable_port++;
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_IPMC_EGRESS_DISABLE) {
                SHR_BITCLR(ipmc_tpbm, i);
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_L2MC_EGRESS_DISABLE) {
                SHR_BITCLR(l2mc_tpbm, i);
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_BCAST_EGRESS_DISABLE) {
                SHR_BITCLR(bcast_tpbm, i);
            }
            if (add_info->member_flags[i] &
                    BCM_TRUNK_MEMBER_DLF_EGRESS_DISABLE) {
                SHR_BITCLR(dlf_tpbm, i);
            }
        }

        BCM_GPORT_MODPORT_SET(gport, mod_out, port_out);
        if (BCM_SUCCESS(bcm_esw_port_local_get(unit, gport, &loc_port))) { 
            if (!IS_E_PORT(unit, loc_port)) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_PARAM);
            }
            SOC_PBMP_PORT_ADD(new_trunk_pbmp, loc_port);
            if (SHR_BITGET(ipmc_tpbm, i)) {
                SOC_PBMP_PORT_ADD(ipmc_trunk_pbmp, loc_port);
                SHR_BITSET(ipmc_my_modid_bmap, ipmc_member_count);
            }
            if (SHR_BITGET(l2mc_tpbm, i)) {
                SOC_PBMP_PORT_ADD(l2mc_trunk_pbmp, loc_port);
                SHR_BITSET(l2mc_my_modid_bmap, l2mc_member_count);
            }
            if (SHR_BITGET(bcast_tpbm, i)) {
                SOC_PBMP_PORT_ADD(bcast_trunk_pbmp, loc_port);
                SHR_BITSET(bcast_my_modid_bmap, bcast_member_count);
            }
            if (SHR_BITGET(dlf_tpbm, i)) {
                SOC_PBMP_PORT_ADD(dlf_trunk_pbmp, loc_port);
                SHR_BITSET(dlf_my_modid_bmap, dlf_member_count);
            }
        }

        if (SHR_BITGET(ipmc_tpbm, i)) {
            /* Eligible for IPMC forwarding */
            ipmc_member_count++;
        }
        if (SHR_BITGET(l2mc_tpbm, i)) {
            /* Eligible for L2MC forwarding */
            l2mc_member_count++;
        }
        if (SHR_BITGET(bcast_tpbm, i)) {
            /* Eligible for broadcast forwarding */
            bcast_member_count++;
        }
        if (SHR_BITGET(dlf_tpbm, i)) {
            /* Eligible for DLF forwarding */
            dlf_member_count++;
        }
    }

    /* Figure out which trunk ports are eligible for egress and
     * unicast egress
     */
    num_egr_ports = add_info->num_ports - negr_disable_port;
    if (num_egr_ports > 0) {
        egr_hwmod = sal_alloc(sizeof(bcm_module_t) * num_egr_ports,
                "egr_hwmod");
        if (egr_hwmod == NULL) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
        }
        egr_hwport = sal_alloc(sizeof(bcm_port_t) * num_egr_ports,
                "egr_hwport");
        if (egr_hwport == NULL) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
        }
        egr_member_flags = sal_alloc(sizeof(uint32) * num_egr_ports,
                "egr_member_flags");
        if (egr_member_flags == NULL) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
        }
        egr_scaling_factor = sal_alloc(sizeof(int) * num_egr_ports,
                "egr_scaling_factor");
        if (egr_scaling_factor == NULL) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
        }
        egr_load_weight = sal_alloc(sizeof(int) * num_egr_ports,
                "egr_load_weight");
        if (egr_load_weight == NULL) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
        }
        for (i = 0, j = 0; i < add_info->num_ports; i++) {
            if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_EGRESS_DISABLE) {
                continue;
            }
            egr_hwmod[j] = hwmod[i];
            egr_hwport[j] = hwport[i];
            egr_member_flags[j] = add_info->member_flags[i];
            egr_scaling_factor[j] = add_info->dynamic_scaling_factor[i];
            egr_load_weight[j] = add_info->dynamic_load_weight[i];
            j++;
        }

        num_uc_egr_ports = num_egr_ports - num_uc_egr_disable_port;
        if (num_uc_egr_ports > 0 ) {
            uc_egr_hwmod = sal_alloc(sizeof(bcm_module_t) * num_uc_egr_ports,
                    "uc_egr_hwmod");
            if (uc_egr_hwmod == NULL) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
            }
            uc_egr_hwport = sal_alloc(sizeof(bcm_port_t) * num_uc_egr_ports,
                    "uc_egr_hwport");
            if (uc_egr_hwport == NULL) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
            }
            uc_egr_scaling_factor = sal_alloc(sizeof(int) * num_uc_egr_ports,
                    "uc_egr_scaling_factor");
            if (uc_egr_scaling_factor == NULL) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
            }
            uc_egr_load_weight = sal_alloc(sizeof(int) * num_uc_egr_ports,
                    "uc_egr_load_weight");
            if (uc_egr_load_weight == NULL) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
            }
            for (i = 0, k = 0; i < j; i++) {
                if (egr_member_flags[i] &
                        BCM_TRUNK_MEMBER_UNICAST_EGRESS_DISABLE) {
                    continue;
                }
                uc_egr_hwmod[k] = egr_hwmod[i];
                uc_egr_hwport[k] = egr_hwport[i];
                uc_egr_scaling_factor[k] = egr_scaling_factor[i];
                uc_egr_load_weight[k] = egr_load_weight[i];
                k++;
            }
        }
    }

    /* Configure static load balancing */
    if (t_info->in_use) {
        rv = _bcm_trident_lag_slb_free_resource(unit, tid);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
    }
    rv = _bcm_trident_lag_slb_set(unit, tid, num_uc_egr_ports,
            uc_egr_hwmod, uc_egr_hwport, t_info); 
    if (BCM_FAILURE(rv)) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    /* Configure dynamic load balancing */
    if (soc_feature(unit, soc_feature_lag_dlb)) {
        if (t_info->in_use) {
            rv = bcm_tr3_lag_dlb_free_resource(unit, tid);
            if (BCM_FAILURE(rv)) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
            }
        }
        rv = bcm_tr3_lag_dlb_set(unit, tid, add_info, num_uc_egr_ports,
                uc_egr_hwmod, uc_egr_hwport, uc_egr_scaling_factor,
                uc_egr_load_weight);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    /* Configure resilient hashing */
    if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
        bcm_module_t del_mod;
        bcm_port_t del_port;

        if (op == TRUNK_MEMBER_OP_SET) {
            if (t_info->in_use) {
                rv = bcm_td2_lag_rh_free_resource(unit, tid);
                if (BCM_FAILURE(rv)) {
                    _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                }
            } 
            if (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                rv = bcm_td2_lag_rh_set(unit, tid, add_info,
                        num_uc_egr_ports, uc_egr_hwmod, uc_egr_hwport);
                if (BCM_FAILURE(rv)) {
                    _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                }
            }
        } else if (op == TRUNK_MEMBER_OP_ADD) {
            if (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                /* Add the new member if it's eligible for resilient hashing */
                if (!(member->flags & BCM_TRUNK_MEMBER_EGRESS_DISABLE) &&
                    !(member->flags & BCM_TRUNK_MEMBER_UNICAST_EGRESS_DISABLE)) {
                    rv = bcm_td2_lag_rh_add(unit, tid, add_info,
                            num_uc_egr_ports, uc_egr_hwmod, uc_egr_hwport,
                            member);
                    if (BCM_FAILURE(rv)) {
                        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                    }
                }
            }
        } else if (op == TRUNK_MEMBER_OP_DELETE) {
            if (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
                    mod = MEMBER_INFO(unit, tid).modport[i] >> 8;
                    port = MEMBER_INFO(unit, tid).modport[i] & 0xff;
                    rv = _bcm_esw_trunk_gport_array_resolve(unit, FALSE, 1,
                            &member->gport, &del_port, &del_mod);
                    if (BCM_FAILURE(rv)) {
                        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                    }
                    if (mod == del_mod && port == del_port) {
                        break;
                    }
                }
                if (i == MEMBER_INFO(unit, tid).num_ports) {
                    /* The member to be deleted is not found. */
                    _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_NOT_FOUND);
                }
                /* Delete the member if it was resilient hashing eligible */
                if (!(MEMBER_INFO(unit, tid).member_flags[i] &
                            BCM_TRUNK_MEMBER_EGRESS_DISABLE) &&
                    !(MEMBER_INFO(unit, tid).member_flags[i] &
                        BCM_TRUNK_MEMBER_UNICAST_EGRESS_DISABLE)) {
                    rv = bcm_td2_lag_rh_delete(unit, tid, add_info,
                            num_uc_egr_ports, uc_egr_hwmod, uc_egr_hwport,
                            member);
                    if (BCM_FAILURE(rv)) {
                        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                    }
                }
            }
        } else {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_PARAM);
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Update TRUNK_BITMAP table */
    rv = READ_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, tid, &trunk_bitmap_entry);
    if (BCM_FAILURE(rv)) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
    }
    SOC_PBMP_CLEAR(old_trunk_pbmp);
    soc_mem_pbmp_field_get(unit, TRUNK_BITMAPm, &trunk_bitmap_entry,
            TRUNK_BITMAPf, &old_trunk_pbmp);
    soc_mem_pbmp_field_set(unit, TRUNK_BITMAPm, &trunk_bitmap_entry,
            TRUNK_BITMAPf, &new_trunk_pbmp);
    rv = WRITE_TRUNK_BITMAPm(unit, MEM_BLOCK_ALL, tid, &trunk_bitmap_entry);
    if (BCM_FAILURE(rv)) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
    }

    /* Update source trunk map configuration */
    for (i = 0; i < add_info->num_ports; i++) {
        mod = hwmod[i%add_info->num_ports];
        port = hwport[i%add_info->num_ports];

        if (add_info->member_flags[i] & BCM_TRUNK_MEMBER_INGRESS_DISABLE) {
            rv = _bcm_trident_trunk_set_port_property(unit, mod, port, -1);
        } else {
            rv = _bcm_trident_trunk_set_port_property(unit, mod, port, tid);
        }
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
    }    

    /* Update the trunk block mask */
    if (SOC_PBMP_NOT_NULL(new_trunk_pbmp) ||
            SOC_PBMP_NOT_NULL(old_trunk_pbmp)) {
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_IPMC,
                old_trunk_pbmp,
                new_trunk_pbmp,
                ipmc_trunk_pbmp,
                ipmc_my_modid_bmap,
                ipmc_member_count,
                add_info->flags);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_L2MC,
                old_trunk_pbmp,
                new_trunk_pbmp,
                l2mc_trunk_pbmp,
                l2mc_my_modid_bmap,
                l2mc_member_count,
                add_info->flags);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_BCAST,
                old_trunk_pbmp,
                new_trunk_pbmp,
                bcast_trunk_pbmp,
                bcast_my_modid_bmap,
                bcast_member_count,
                add_info->flags);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
        rv = _bcm_trident_trunk_block_mask(unit,
                TRUNK_BLOCK_MASK_TYPE_DLF,
                old_trunk_pbmp,
                new_trunk_pbmp,
                dlf_trunk_pbmp,
                dlf_my_modid_bmap,
                dlf_member_count,
                add_info->flags);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
    }

    /* Harware trunk failover */
    if (soc_feature(unit, soc_feature_port_lag_failover)) {
        rv = _bcm_trident_trunk_failover_set(unit, tid, FALSE, add_info);
        if (BCM_FAILURE(rv)) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }
    }

    /* Software trunk failover */
    rv = _bcm_trident_trunk_swfailover_fp_set(unit,
                                           tid,
                                           t_info->rtag,
                                           num_egr_ports,
                                           egr_hwmod,
                                           egr_hwport,
                                           egr_member_flags,
                                           add_info->flags);
    if (BCM_FAILURE(rv)) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
    }

    /* Figure out which members are leaving, staying or joining
     * the trunk group
     */
    if (t_info->in_use) {
        if (MEMBER_INFO(unit, tid).num_ports > 0) {
            leaving_members = sal_alloc(sizeof(bcm_gport_t) *
                    MEMBER_INFO(unit, tid).num_ports, "leaving members");
            if (leaving_members == NULL) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
            }
            sal_memset(leaving_members, 0,
                    sizeof(bcm_gport_t) * MEMBER_INFO(unit, tid).num_ports);

            staying_members = sal_alloc(sizeof(bcm_gport_t) *
                    MEMBER_INFO(unit, tid).num_ports, "staying members");
            if (staying_members == NULL) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
            }
            sal_memset(staying_members, 0,
                    sizeof(bcm_gport_t) * MEMBER_INFO(unit, tid).num_ports);

            for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
                mod = MEMBER_INFO(unit, tid).modport[i] >> 8;
                port = MEMBER_INFO(unit, tid).modport[i] & 0xff;
                BCM_GPORT_MODPORT_SET(gport, mod, port);

                /* Invalidate port only if it is not a part from new trunk group */
                remove_trunk_port = TRUE;
                for (j = 0; j < add_info->num_ports; j++) {
                    if (mod == add_info->tm[j] && port == add_info->tp[j]) {
                        remove_trunk_port = FALSE;
                    }
                }
                if (remove_trunk_port) {
                    /* Add port to array containing members leaving the trunk
                     * group if it is different from previous leaving members
                     */
                    match_leaving_members = FALSE;
                    for (k = 0; k < num_leaving_members; k++) {
                        if (gport == leaving_members[k]) {
                            match_leaving_members = TRUE;
                            break;
                        }
                    }
                    if (!match_leaving_members) {
                        leaving_members[num_leaving_members++] = gport;
                    }

                    rv = _bcm_trident_trunk_set_port_property(unit, mod, port, -1);
                    if (BCM_FAILURE(rv)) {
                        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                    }
                    if (soc_feature(unit, soc_feature_port_lag_failover)) {
                        /* If deleted port is local, clear HW failover info */
                        rv = _bcm_esw_modid_is_local(unit, mod, &is_local);
                        if (BCM_FAILURE(rv)) {
                            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                        }
                        if (is_local) {
                            rv = _bcm_trident_trunk_hwfailover_set(unit, tid,
                                    FALSE, port, mod, add_info->psc,
                                    0, 0, NULL, NULL);
                            if (BCM_FAILURE(rv)) {
                                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
                            }
                        }
                    }
                } else {
                    /* Add port to array containing members staying in the trunk
                     * group if it is different from previous staying members
                     */
                    match_staying_members = FALSE;
                    for (k = 0; k < num_staying_members; k++) {
                        if (gport == staying_members[k]) {
                            match_staying_members = TRUE;
                            break;
                        }
                    }
                    if (!match_staying_members) {
                        staying_members[num_staying_members++] = gport;
                    }

                }
            }
        }
    }

    joining_members = sal_alloc(sizeof(bcm_gport_t) * add_info->num_ports,
            "joining members");
    if (joining_members == NULL) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }
    sal_memset(joining_members, 0, sizeof(bcm_gport_t) * add_info->num_ports);
    for (i = 0; i < add_info->num_ports; i++) {
        BCM_GPORT_MODPORT_SET(gport, hwmod[i], hwport[i]); 
        match_staying_members = FALSE;
        for (j = 0; j < num_staying_members; j++) {
            if (gport == staying_members[j]) {
                match_staying_members = TRUE;
                break;
            }
        }
        if (!match_staying_members) {
            match_joining_members = FALSE;
            for (j = 0; j < num_joining_members; j++) {
                if (gport == joining_members[j]) {
                    match_joining_members = TRUE;
                    break;
                }
            }
            if (!match_joining_members) {
                joining_members[num_joining_members++] = gport;
            }
        }
    }
 
    /* Migrate trunk properties from old members to new members */
    rv = _bcm_xgs3_trunk_property_migrate(unit,
                num_leaving_members, leaving_members,
                num_staying_members, staying_members,
                num_joining_members, joining_members);
    if (BCM_FAILURE(rv)) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
    }

    /* Save new trunk member info */
    if (MEMBER_INFO(unit, tid).modport) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
    }
    if (MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).member_flags);
        MEMBER_INFO(unit, tid).member_flags = NULL;
    }

    MEMBER_INFO(unit, tid).modport = sal_alloc(
            sizeof(uint16) * add_info->num_ports, "member info modport");
    if (NULL == MEMBER_INFO(unit, tid).modport) {
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }

    MEMBER_INFO(unit, tid).member_flags = sal_alloc(
            sizeof(uint32) * add_info->num_ports, "member info flags");
    if (NULL == MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
        _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(BCM_E_MEMORY);
    }

    for (i = 0; i < add_info->num_ports; i++) {
        MEMBER_INFO(unit, tid).modport[i] = ((hwmod[i] & 0xff) << 8) |
                                            (hwport[i] & 0xff);
        MEMBER_INFO(unit, tid).member_flags[i] = add_info->member_flags[i];
    }
    MEMBER_INFO(unit, tid).num_ports = add_info->num_ports;

    /* Call Trill module to clear states for local leaving members, joining members  */
#ifdef INCLUDE_L3
    if (soc_feature(unit, soc_feature_trill)) {
        bcm_port_t local_member;
        bcm_port_t local_member_arr[SOC_MAX_NUM_PORTS];
        int num_local_members;
        int nh_index = 0; 

        /* check if Trunk ID is in use by TRILL, VXLAN, L2-GRE */
        rv = _bcm_xgs3_trunk_nh_store_get(unit, tid, &nh_index);
        if (nh_index == 0) {
            _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
        }

        num_local_members = 0;
        for (i = 0; i < num_leaving_members; i++) {
            if (BCM_SUCCESS(bcm_esw_port_local_get(unit, leaving_members[i],
                            &local_member))) {
                local_member_arr[num_local_members++] = local_member;
            }
        }
        if (num_local_members > 0) {
            rv = _bcm_esw_trill_trunk_member_delete(unit, tid,
                    num_local_members, local_member_arr);
            if (BCM_FAILURE(rv)) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
            }
        }

        num_local_members = 0;
        for (i = 0; i < num_joining_members; i++) {
            if (BCM_SUCCESS(bcm_esw_port_local_get(unit, joining_members[i],
                            &local_member))) {
                local_member_arr[num_local_members++] = local_member;
            }
        }
        if (num_local_members > 0) {
            rv = _bcm_esw_trill_trunk_member_add(unit, tid,
                    num_local_members, local_member_arr);
            if (BCM_FAILURE(rv)) {
                _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
            }
        }
    }
#endif /* INCLUDE_L3 */

    _BCM_TRIDENT_TRUNK_FP_SET_FREE_MEM_RETURN(rv);
}

/*
 * Function:
 *      bcm_trident_trunk_fp_destroy
 * Purpose:
 *      Remove front-panel trunk group.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      t_info   - (IN) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_fp_destroy(int unit,
        bcm_trunk_t tid,
        trunk_private_t *t_info)
{
    trunk_group_entry_t         tg_entry;
    trunk_bitmap_entry_t        trunk_bitmap_entry;
    trunk_member_entry_t        trunk_member_entry;
    pbmp_t                      old_trunk_pbmp, new_trunk_pbmp;
    int                         old_base_ptr, old_tg_size, old_rtag;
    int                         old_num_entries;
    bcm_module_t                mod;
    bcm_port_t                  port;
    bcm_gport_t                 gport;
    int                         i, j;
    int                         num_leaving_members = 0;
    bcm_gport_t                 *leaving_members = NULL;
    int                         match_leaving_members;
    int                         rv = BCM_E_NONE;
    pbmp_t                      ipmc_trunk_pbmp;
    pbmp_t                      l2mc_trunk_pbmp;
    pbmp_t                      bcast_trunk_pbmp;
    pbmp_t                      dlf_trunk_pbmp;

    /* Clear dynamic load balancing */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_lag_dlb)) {
        BCM_IF_ERROR_RETURN(bcm_tr3_lag_dlb_free_resource(unit, tid));
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    /* Clear resilient hashing configuration */
    if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
        BCM_IF_ERROR_RETURN(bcm_td2_lag_rh_free_resource(unit, tid));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Get old trunk bitmap */
    SOC_IF_ERROR_RETURN
        (READ_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, tid, &trunk_bitmap_entry));
    SOC_PBMP_CLEAR(old_trunk_pbmp);
    soc_mem_pbmp_field_get(unit, TRUNK_BITMAPm, &trunk_bitmap_entry,
            TRUNK_BITMAPf, &old_trunk_pbmp);

    /* Get old trunk group entry */
    SOC_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &tg_entry));
    old_base_ptr = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, BASE_PTRf);
    old_tg_size = 1 + soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, TG_SIZEf);
    old_rtag = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, RTAGf);

    /* Clear TRUNK_BITMAP table */
    SOC_IF_ERROR_RETURN
        (WRITE_TRUNK_BITMAPm(unit, MEM_BLOCK_ALL, tid,
                             soc_mem_entry_null(unit, TRUNK_BITMAPm)));

    /* Clear TRUNK_GROUP table */
    SOC_IF_ERROR_RETURN
        (WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid,
                            soc_mem_entry_null(unit, TRUNK_GROUPm)));

    /* Clear TRUNK_MEMBER table */
    if (old_rtag != 0) {
        old_num_entries = old_tg_size;
        if (old_rtag >= 1 && old_rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
            if (SOC_IS_KATANAX(unit)) {
                old_num_entries = _BCM_KATANA_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
            } else
#endif
            if (soc_feature(unit,
                        soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
                old_num_entries = _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
            }
        } 

        _BCM_FP_TRUNK_MEMBER_USED_CLR_RANGE(unit, old_base_ptr, old_num_entries);

        for (i = 0; i < old_num_entries; i++) {
            sal_memset(&trunk_member_entry, 0, sizeof(trunk_member_entry_t));
            rv = WRITE_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL, old_base_ptr + i,
                    &trunk_member_entry); 
            if (BCM_FAILURE(rv)) {
                return rv;
            }
        }
    }

    /* Update source port to trunk mapping */

    if (MEMBER_INFO(unit, tid).num_ports > 0) {
        leaving_members = sal_alloc(sizeof(bcm_gport_t) *
                MEMBER_INFO(unit, tid).num_ports, "leaving members");
        if (leaving_members == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(leaving_members, 0,
                sizeof(bcm_gport_t) * MEMBER_INFO(unit, tid).num_ports);

        for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
            mod = MEMBER_INFO(unit, tid).modport[i] >> 8;
            port = MEMBER_INFO(unit, tid).modport[i] & 0xff;
            BCM_GPORT_MODPORT_SET(gport, mod, port);

            match_leaving_members = FALSE;
            for (j = 0; j < num_leaving_members; j++) {
                if (gport == leaving_members[j]) {
                    match_leaving_members = TRUE;
                    break;
                }
            }
            if (!match_leaving_members) {
                leaving_members[num_leaving_members++] = gport;
            }

            rv = _bcm_trident_trunk_set_port_property(unit, mod, port, -1);
            if (BCM_FAILURE(rv)) {
                sal_free(leaving_members);
                return rv;
            }
        }
    }

    /*
     * Remove ports from the block mask table
     */
    if (SOC_PBMP_NOT_NULL(old_trunk_pbmp)) {
        SOC_PBMP_CLEAR(new_trunk_pbmp);
        SOC_PBMP_CLEAR(ipmc_trunk_pbmp);
        SOC_PBMP_CLEAR(l2mc_trunk_pbmp);
        SOC_PBMP_CLEAR(bcast_trunk_pbmp);
        SOC_PBMP_CLEAR(dlf_trunk_pbmp);
        rv = _bcm_trident_trunk_block_mask(unit, TRUNK_BLOCK_MASK_TYPE_IPMC,
                old_trunk_pbmp, new_trunk_pbmp, ipmc_trunk_pbmp,
                NULL, 0, t_info->flags);
        if (BCM_FAILURE(rv)) {
            sal_free(leaving_members);
            return rv;
        }
        rv = _bcm_trident_trunk_block_mask(unit, TRUNK_BLOCK_MASK_TYPE_L2MC,
                old_trunk_pbmp, new_trunk_pbmp, l2mc_trunk_pbmp,
                NULL, 0, t_info->flags);
        if (BCM_FAILURE(rv)) {
            sal_free(leaving_members);
            return rv;
        }
        rv = _bcm_trident_trunk_block_mask(unit, TRUNK_BLOCK_MASK_TYPE_BCAST,
                old_trunk_pbmp, new_trunk_pbmp, bcast_trunk_pbmp,
                NULL, 0, t_info->flags);
        if (BCM_FAILURE(rv)) {
            sal_free(leaving_members);
            return rv;
        }
        rv = _bcm_trident_trunk_block_mask(unit, TRUNK_BLOCK_MASK_TYPE_DLF,
                old_trunk_pbmp, new_trunk_pbmp, dlf_trunk_pbmp,
                NULL, 0, t_info->flags);
        if (BCM_FAILURE(rv)) {
            sal_free(leaving_members);
            return rv;
        }
    }

    /*
     * Remove ports from failover
     */
    if (soc_feature(unit, soc_feature_port_lag_failover)) {
        rv = _bcm_trident_trunk_failover_set(unit, tid, FALSE, NULL);
        if (BCM_FAILURE(rv)) {
            sal_free(leaving_members);
            return rv;
        }
    }

    rv = _bcm_trident_trunk_swfailover_fp_set(unit,
                                              tid,
                                              0,
                                              0,
                                              NULL,
                                              NULL,
                                              NULL,
                                              0);
    if (BCM_FAILURE(rv)) {
        sal_free(leaving_members);
        return rv;
    }

    /* Clear trunk properties from members */
    rv = _bcm_xgs3_trunk_property_migrate(unit,
                num_leaving_members, leaving_members,
                0, NULL, 0, NULL);
    if (BCM_FAILURE(rv)) {
        sal_free(leaving_members);
        return rv;
    }

    /* Clear trunk member info */
    if (MEMBER_INFO(unit, tid).modport) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
    }
    if (MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).member_flags);
        MEMBER_INFO(unit, tid).member_flags = NULL;
    }
    MEMBER_INFO(unit, tid).num_ports = 0;

    t_info->in_use = FALSE;

    sal_free(leaving_members);
    return rv;
}

/*
 * Function:
 *	bcm_trident_trunk_fp_get
 * Purpose:
 *      Return trunk information of given front panel trunk ID.
 *      If t_data->num_ports = 0, return number of trunk members in
 *      t_data->num_ports.
 * Parameters:
 *      unit   - (IN) StrataSwitch PCI device unit number (driver internal).
 *      tid    - (IN) Trunk ID.
 *      t_data - (OUT) Place to store returned trunk add info.
 *      t_info - (IN) Trunk info.
 * Returns:
 *      BCM_E_NONE              Success.
 *      BCM_E_XXX
 */
STATIC int
_bcm_trident_trunk_fp_get(int unit,
        bcm_trunk_t tid,
        _esw_trunk_add_info_t *t_data,
        trunk_private_t *t_info)
{
    int i;
    bcm_module_t mod;
    bcm_port_t port;

    if (t_data->num_ports == 0) {
        t_data->num_ports = MEMBER_INFO(unit, tid).num_ports;
    } else {
        if (MEMBER_INFO(unit, tid).num_ports <= t_data->num_ports) {
            t_data->num_ports = MEMBER_INFO(unit, tid).num_ports;
        } 

        for (i = 0; i < t_data->num_ports; i++) {
            mod = MEMBER_INFO(unit, tid).modport[i] >> 8;
            port = MEMBER_INFO(unit, tid).modport[i] & 0xff;
            BCM_IF_ERROR_RETURN
                (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, mod, port,
                                        &t_data->tm[i], &t_data->tp[i]));
            t_data->member_flags[i] = MEMBER_INFO(unit, tid).member_flags[i];

            t_data->dynamic_scaling_factor[i] = 0;
            t_data->dynamic_load_weight[i] = 0;
            if ((t_data->psc == BCM_TRUNK_PSC_DYNAMIC) ||
                (t_data->psc == BCM_TRUNK_PSC_DYNAMIC_ASSIGNED) ||
                (t_data->psc == BCM_TRUNK_PSC_DYNAMIC_OPTIMAL)) {
                if (!(t_data->member_flags[i] &
                      BCM_TRUNK_MEMBER_EGRESS_DISABLE) && 
                    !(t_data->member_flags[i] &
                      BCM_TRUNK_MEMBER_UNICAST_EGRESS_DISABLE)) { 
#ifdef BCM_TRIUMPH3_SUPPORT 
                    BCM_IF_ERROR_RETURN(bcm_tr3_lag_dlb_member_attr_get(unit,
                                mod, port,
                                &t_data->dynamic_scaling_factor[i],
                                &t_data->dynamic_load_weight[i]));
#endif /* BCM_TRIUMPH3_SUPPORT */
                }
            } 
        }
    }

    return BCM_E_NONE;
}

/* ----------------------------------------------------------------------------- 
 *
 * Routines for Higig trunking 
 *
 * ----------------------------------------------------------------------------- 
 */

static soc_field_t port_loading_threshold_fields[] = {
    PORT_LOADING_THRESHOLD_1f,
    PORT_LOADING_THRESHOLD_2f,
    PORT_LOADING_THRESHOLD_3f,
    PORT_LOADING_THRESHOLD_4f,
    PORT_LOADING_THRESHOLD_5f,
    PORT_LOADING_THRESHOLD_6f,
    PORT_LOADING_THRESHOLD_7f  
};

/*
 * Function:
 *      _bcm_trident_hg_dlb_sample_rate_thresholds_set
 * Purpose:
 *      Set Higig DLB port load sample period and thresholds.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      sample_rate - (IN) Number of samplings per second.
 *      min_th      - (IN) Minimum port load threshold, in mbps.
 *      max_th      - (IN) Maximum port load threshold, in mbps.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_sample_rate_thresholds_set(int unit, int sample_rate,
        int min_th, int max_th)
{
    int num_time_units;
    uint32 measure_control_reg;
    int max_th_bytes;
    int th_increment;
    dlb_hgt_glb_quantize_thresholds_entry_t thresholds_entry;
    int i;
    int th_bytes[7];

    if (sample_rate <= 0 || min_th < 0 || max_th < 0) {
        return BCM_E_PARAM;
    }
    
    if (min_th > max_th) {
        max_th = min_th;
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
        /* Compute sampling period in units of 1us:
         *     number of 1us time units = 10^6 / sample_rate
         */ 
        num_time_units = 1000000 / sample_rate;
        if (num_time_units < 1 || num_time_units > 255) {
            /* Hardware limits the sampling period to 1 to 255 time units. */ 
            return BCM_E_PARAM;
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        /* Compute sampling period in units of 256ns:
         *     number of 256ns time units = 10^9 / 256 / sample_rate
         *                                = 3906250 / sample_rate
         */ 
        num_time_units = 3906250 / sample_rate;
        if (num_time_units < 2 || num_time_units > 255) {
            /* Hardware limits the sampling period to 2 to 255 time units. */ 
            return BCM_E_PARAM;
        }
    }
    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
            &measure_control_reg, SAMPLING_PERIODf, num_time_units);
    SOC_IF_ERROR_RETURN
        (WRITE_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, measure_control_reg));
    HG_DLB_INFO(unit)->hg_dlb_sample_rate = sample_rate;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
        dlb_hgt_quantize_threshold_entry_t quantize_threshold_entry;
        soc_mem_t quantize_threshold_m;

        /* Compute threshold in bytes per sampling period:
         * bytes per 1us = (million bits per sec) * 10^6 / 8 * 10^(-6),
         * bytes per sampling period = (bytes per 1us) * (number of 1us 
         *                             time units in sampling period)
         *                           = mbps * num_time_units / 8
         */
        max_th_bytes = max_th * num_time_units / 8;

        if (max_th_bytes > 0xffffff) {
            /* Hardware limits port load threshold to 24-bits */
            return BCM_E_PARAM;
        }

        /* Use min threshold as LOADING_THRESHOLD 0, and max threshold as
         * LOADING_THRESHOLD 6. LOADING_THRESHOLD 1 to 5 will be
         * evenly spread out between min and max thresholds.
         */
        th_increment = (max_th - min_th) / 6;
        quantize_threshold_m = soc_mem_is_valid(unit,
                DLB_HGT_PLA_QUANTIZE_THRESHOLDm) ?
                DLB_HGT_PLA_QUANTIZE_THRESHOLDm : DLB_HGT_QUANTIZE_THRESHOLDm;
        for (i = 0; i < 7; i++) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, quantize_threshold_m, MEM_BLOCK_ANY, i,
                              &quantize_threshold_entry));
            th_bytes[i] = (min_th + i * th_increment) * num_time_units / 8;
            soc_mem_field32_set(unit, quantize_threshold_m,
                    &quantize_threshold_entry, LOADING_THRESHOLDf, th_bytes[i]);
            SOC_IF_ERROR_RETURN
                (soc_mem_write(unit, quantize_threshold_m, MEM_BLOCK_ALL, i,
                               &quantize_threshold_entry));
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        /* Compute threshold in bytes per sampling period:
         * bytes per 256ns = (million bits per sec) * 10^6 / 8 * 256 * 10^(-9),
         * bytes per sampling period = (bytes per 256ns) * (number of 256ns
         *                             time units in sampling period)
         *                           = mbps * 256 * num_time_units / 8000
         *
         * Note: The portion of the formula, (mbps * 256 * num_time_units), may
         *       overflow when mbps exceeds 33000 and num_time_units equals to
         *       to its max value of 255. Hence, the formula will be rewritten
         *       as ((mbps * 256) / 8000) * num_time_units.
         */
        max_th_bytes = ((max_th << 8) / 8000) * num_time_units;

        if (max_th_bytes > 0xffff) {
            /* Hardware limits port load threshold to 16-bits */
            return BCM_E_PARAM;
        }

        /* Use min threshold as PORT_LOADING_THRESHOLD_1, and max threshold as
         * PORT_LOADING_THRESHOLD_7. PORT_LOADING_THRESHOLD 2 to 6 will be
         * evenly spread out between min and max thresholds.
         */
        th_increment = (max_th - min_th) / 6;
        SOC_IF_ERROR_RETURN
            (READ_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm(unit, MEM_BLOCK_ANY, 0,
                                                   &thresholds_entry));
        for (i = 0; i < 7; i++) {
            th_bytes[i] = (((min_th + i * th_increment) << 8) / 8000) *
                num_time_units;
            soc_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm_field32_set
                (unit, &thresholds_entry, port_loading_threshold_fields[i],
                 th_bytes[i]);
        }
        SOC_IF_ERROR_RETURN
            (WRITE_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm(unit, MEM_BLOCK_ALL, 0,
                                                    &thresholds_entry));
    }

    HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th = min_th;
    HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th = max_th;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_sample_rate_set
 * Purpose:
 *      Set Higig dynamic load balancing sample rate.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      sample_rate - (IN) Number of samplings per second.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_sample_rate_set(int unit, int sample_rate)
{
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_sample_rate_thresholds_set(unit,
                sample_rate, HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th,
                HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_tx_load_min_th_set
 * Purpose:
 *      Set Higig DLB port load minimum threshold.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      min_th - (IN) Minimum port loading threshold.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_tx_load_min_th_set(int unit, int min_th)
{
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_sample_rate_thresholds_set(unit,
                HG_DLB_INFO(unit)->hg_dlb_sample_rate, min_th,
                HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_tx_load_max_th_set
 * Purpose:
 *      Set Higig DLB port load maximum threshold.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      max_th - (IN) Maximum port loading threshold.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_tx_load_max_th_set(int unit, int max_th)
{
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_sample_rate_thresholds_set(unit,
                HG_DLB_INFO(unit)->hg_dlb_sample_rate,
                HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th,
                max_th));
    return BCM_E_NONE;
}

static soc_field_t port_qsize_threshold_fields[] = {
    PORT_QSIZE_THRESHOLD_1f,
    PORT_QSIZE_THRESHOLD_2f,
    PORT_QSIZE_THRESHOLD_3f,
    PORT_QSIZE_THRESHOLD_4f,
    PORT_QSIZE_THRESHOLD_5f,
    PORT_QSIZE_THRESHOLD_6f,
    PORT_QSIZE_THRESHOLD_7f 
};

/*
 * Function:
 *      _bcm_trident_hg_dlb_qsize_thresholds_set
 * Purpose:
 *      Set Higig DLB queue size thresholds.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      min_th - (IN) Minimum queue size threshold, in bytes.
 *      max_th - (IN) Maximum queue size threshold, in bytes.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_qsize_thresholds_set(int unit, int min_th, int max_th)
{
    int max_th_cells;
    int th_increment;
    dlb_hgt_glb_quantize_thresholds_entry_t thresholds_entry;
    int i;
    int th_cells[7];

    if (min_th < 0 || max_th < 0) {
        return BCM_E_PARAM;
    }

    if (min_th > max_th) {
        max_th = min_th;
    }

    /* Convert threshold to number of cells */
    max_th_cells = max_th / 208;

    if (max_th_cells > 0xffff) {
        /* Hardware limits queue size threshold to 16-bits */
        return BCM_E_PARAM;
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
        dlb_hgt_quantize_threshold_entry_t quantize_threshold_entry;
        soc_mem_t quantize_threshold_m;

        /* Use min threshold as QSIZE_THRESHOLD 0, and max threshold as
         * QSIZE_THRESHOLD 6. QSIZE_THRESHOLD 1 to 5 will be
         * evenly spread out between min and max thresholds.
         */
        th_increment = (max_th - min_th) / 6;
        quantize_threshold_m = soc_mem_is_valid(unit,
                DLB_HGT_PLA_QUANTIZE_THRESHOLDm) ?
                DLB_HGT_PLA_QUANTIZE_THRESHOLDm : DLB_HGT_QUANTIZE_THRESHOLDm;
        for (i = 0; i < 7; i++) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, quantize_threshold_m, MEM_BLOCK_ANY, i,
                              &quantize_threshold_entry));
            th_cells[i] = (min_th + i * th_increment) / 208;
            soc_mem_field32_set(unit, quantize_threshold_m,
                    &quantize_threshold_entry, QSIZE_THRESHOLDf, th_cells[i]);
            SOC_IF_ERROR_RETURN
                (soc_mem_write(unit, quantize_threshold_m, MEM_BLOCK_ALL, i,
                               &quantize_threshold_entry));
        }
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        /* Use min threshold as PORT_QSIZE_THRESHOLD_1, and max threshold as
         * PORT_QSIZE_THRESHOLD_7. PORT_QSIZE_THRESHOLD 2 to 6 will be
         * evenly spread out between min and max thresholds.
         */
        th_increment = (max_th - min_th) / 6;
        SOC_IF_ERROR_RETURN
            (READ_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm(unit, MEM_BLOCK_ANY, 0,
                                                   &thresholds_entry));
        for (i = 0; i < 7; i++) {
            th_cells[i] = (min_th + i * th_increment) >> 7;
            soc_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm_field32_set
                (unit, &thresholds_entry, port_qsize_threshold_fields[i],
                 th_cells[i]);
        }
        SOC_IF_ERROR_RETURN
            (WRITE_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm(unit, MEM_BLOCK_ALL, 0,
                                                    &thresholds_entry));
    }
    HG_DLB_INFO(unit)->hg_dlb_qsize_min_th = min_th;
    HG_DLB_INFO(unit)->hg_dlb_qsize_max_th = max_th;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_qsize_min_th_set
 * Purpose:
 *      Set Higig DLB queue size minimum threshold.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      min_th - (IN) Minimum queue size threshold, in bytes.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_qsize_min_th_set(int unit, int min_th)
{
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_qsize_thresholds_set(unit,
                min_th, HG_DLB_INFO(unit)->hg_dlb_qsize_max_th));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_qsize_max_th_set
 * Purpose:
 *      Set Higig DLB queue size maximum threshold.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      max_th - (IN) Maximum queue size threshold, in bytes.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_qsize_max_th_set(int unit, int max_th)
{
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_qsize_thresholds_set(unit,
                HG_DLB_INFO(unit)->hg_dlb_qsize_min_th, max_th));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_tx_load_weight_set
 * Purpose:
 *      Set Higig DLB port load weight.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      weight - (IN) Port load weight.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_tx_load_weight_set(int unit, int weight)
{
    uint32 measure_control_reg;
    soc_field_t load_weight_f;

    if (weight < 0 || 
        weight > 0xf) {
        /* Hardware limits port load weight to 4 bits */
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                PORT_LOADING_WEIGHTf)) {
        load_weight_f = PORT_LOADING_WEIGHTf;
    } else {
        load_weight_f = WEIGHT_LOADINGf;
    }
    soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        &measure_control_reg, load_weight_f, weight);
    SOC_IF_ERROR_RETURN
        (WRITE_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, measure_control_reg));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_tx_load_weight_get
 * Purpose:
 *      Get Higig DLB port load weight.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      weight - (OUT) Port load weight.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_tx_load_weight_get(int unit, int *weight)
{
    uint32 measure_control_reg;
    soc_field_t load_weight_f;

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                PORT_LOADING_WEIGHTf)) {
        load_weight_f = PORT_LOADING_WEIGHTf;
    } else {
        load_weight_f = WEIGHT_LOADINGf;
    }
    *weight = soc_reg_field_get(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        measure_control_reg, load_weight_f);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_qsize_weight_set
 * Purpose:
 *      Set Higig DLB queue size weight.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      weight - (IN) Queue size weight.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_qsize_weight_set(int unit, int weight)
{
    uint32 measure_control_reg;
    soc_field_t qsize_weight_f;

    if (weight < 0 || 
        weight > 0xf) {
        /* Hardware limits queue size weight to 4 bits */
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                PORT_QSIZE_WEIGHTf)) {
        qsize_weight_f = PORT_QSIZE_WEIGHTf;
    } else {
        qsize_weight_f = WEIGHT_QSIZEf;
    }
    soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        &measure_control_reg, qsize_weight_f, weight);
    SOC_IF_ERROR_RETURN
        (WRITE_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, measure_control_reg));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_qsize_weight_get
 * Purpose:
 *      Get Higig DLB queue size weight.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      weight - (OUT) Queue size weight.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_qsize_weight_get(int unit, int *weight)
{
    uint32 measure_control_reg;
    soc_field_t qsize_weight_f;

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                PORT_QSIZE_WEIGHTf)) {
        qsize_weight_f = PORT_QSIZE_WEIGHTf;
    } else {
        qsize_weight_f = WEIGHT_QSIZEf;
    }
    *weight = soc_reg_field_get(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        measure_control_reg, qsize_weight_f);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_tx_load_cap_set
 * Purpose:
 *      Set Higig DLB port load cap control.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      cap - (IN) Port load cap control.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_tx_load_cap_set(int unit, int cap)
{
    uint32 measure_control_reg;
    soc_field_t cap_load_f;

    if (cap < 0 || 
        cap > 1) {
        /* Hardware limits port load cap control to 1 bit */
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                CAP_LOADING_AVERAGEf)) {
        cap_load_f = CAP_LOADING_AVERAGEf; 
    } else {
        cap_load_f = CAP_LOADING_AVGf;
    }
    soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        &measure_control_reg, cap_load_f, cap);
    SOC_IF_ERROR_RETURN
        (WRITE_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, measure_control_reg));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_tx_load_cap_get
 * Purpose:
 *      Get Higig DLB port load cap control.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      cap - (OUT) Cap control.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_tx_load_cap_get(int unit, int *cap)
{
    uint32 measure_control_reg;
    soc_field_t cap_load_f;

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                CAP_LOADING_AVERAGEf)) {
        cap_load_f = CAP_LOADING_AVERAGEf; 
    } else {
        cap_load_f = CAP_LOADING_AVGf;
    }
    *cap= soc_reg_field_get(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        measure_control_reg, cap_load_f);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_qsize_cap_set
 * Purpose:
 *      Set Higig DLB queue size cap control.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      cap - (IN) Queue size cap control.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_qsize_cap_set(int unit, int cap)
{
    uint32 measure_control_reg;
    soc_field_t cap_qsize_f;

    if (cap < 0 || 
        cap > 1) {
        /* Hardware limits queue size cap control to 1 bit */
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                CAP_QSIZE_AVERAGEf)) {
        cap_qsize_f = CAP_QSIZE_AVERAGEf; 
    } else {
        cap_qsize_f = CAP_QSIZE_AVGf; 
    }
    soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        &measure_control_reg, cap_qsize_f, cap);
    SOC_IF_ERROR_RETURN
        (WRITE_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, measure_control_reg));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_qsize_cap_get
 * Purpose:
 *      Get Higig DLB queue size cap control.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      cap - (OUT) Cap control.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_qsize_cap_get(int unit, int *cap)
{
    uint32 measure_control_reg;
    soc_field_t cap_qsize_f;

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (SOC_REG_FIELD_VALID(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                CAP_QSIZE_AVERAGEf)) {
        cap_qsize_f = CAP_QSIZE_AVERAGEf; 
    } else {
        cap_qsize_f = CAP_QSIZE_AVGf; 
    }
    *cap= soc_reg_field_get(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
        measure_control_reg, cap_qsize_f);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_config_set
 * Purpose:
 *      Set per-chip Higig dynamic load balancing configuration parameters.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      type - (IN) Configuration parameter type.
 *      arg  - (IN) Configuration paramter value.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_hg_dlb_config_set(int unit, bcm_switch_control_t type, int arg)
{
    switch (type) {
        case bcmSwitchFabricTrunkDynamicSampleRate:
            return _bcm_trident_hg_dlb_sample_rate_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicEgressBytesExponent:
            return _bcm_trident_hg_dlb_tx_load_weight_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicQueuedBytesExponent:
            return _bcm_trident_hg_dlb_qsize_weight_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicEgressBytesDecreaseReset:
            return _bcm_trident_hg_dlb_tx_load_cap_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicQueuedBytesDecreaseReset:
            return _bcm_trident_hg_dlb_qsize_cap_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicEgressBytesMinThreshold:
            return _bcm_trident_hg_dlb_tx_load_min_th_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicEgressBytesMaxThreshold:
            return _bcm_trident_hg_dlb_tx_load_max_th_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicQueuedBytesMinThreshold:
            return _bcm_trident_hg_dlb_qsize_min_th_set(unit, arg);

        case bcmSwitchFabricTrunkDynamicQueuedBytesMaxThreshold:
            return _bcm_trident_hg_dlb_qsize_max_th_set(unit, arg);

        default:
            return BCM_E_PARAM;
    }
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_config_get
 * Purpose:
 *      Get per-chip Higig dynamic load balancing configuration parameters.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      type - (IN) Configuration parameter type.
 *      arg  - (OUT) Configuration paramter value.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_hg_dlb_config_get(int unit, bcm_switch_control_t type, int *arg)
{
    switch (type) {
        case bcmSwitchFabricTrunkDynamicSampleRate:
            *arg = HG_DLB_INFO(unit)->hg_dlb_sample_rate;
            break;

        case bcmSwitchFabricTrunkDynamicEgressBytesExponent:
            return _bcm_trident_hg_dlb_tx_load_weight_get(unit, arg);

        case bcmSwitchFabricTrunkDynamicQueuedBytesExponent:
            return _bcm_trident_hg_dlb_qsize_weight_get(unit, arg);

        case bcmSwitchFabricTrunkDynamicEgressBytesDecreaseReset:
            return _bcm_trident_hg_dlb_tx_load_cap_get(unit, arg);

        case bcmSwitchFabricTrunkDynamicQueuedBytesDecreaseReset:
            return _bcm_trident_hg_dlb_qsize_cap_get(unit, arg);

        case bcmSwitchFabricTrunkDynamicEgressBytesMinThreshold:
            *arg = HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th;
            break;

        case bcmSwitchFabricTrunkDynamicEgressBytesMaxThreshold:
            *arg = HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th;
            break;

        case bcmSwitchFabricTrunkDynamicQueuedBytesMinThreshold:
            *arg = HG_DLB_INFO(unit)->hg_dlb_qsize_min_th;
            break;

        case bcmSwitchFabricTrunkDynamicQueuedBytesMaxThreshold:
            *arg = HG_DLB_INFO(unit)->hg_dlb_qsize_max_th;
            break;

        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_quality_assign
 * Purpose:
 *      Assign quality based on port loading and queue size.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 *      tx_load_percent - (IN) Percent weight of port loading in determing
 *                             port quality. The remaining percentage is
 *                             the weight of queue size.
 *      entry_arr - (IN) Array of 64 quality map table entries.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_trident_hg_dlb_quality_assign(int unit, int tx_load_percent,
        uint32 *entry_arr)
{
    int quantized_tx_load;
    int quantized_qsize;
    int quality;
    int entry_index;

    if (entry_arr == NULL) {
        return BCM_E_PARAM;
    }

    for (quantized_tx_load = 0; quantized_tx_load < 8; quantized_tx_load++) {
        for (quantized_qsize = 0; quantized_qsize < 8; quantized_qsize++) {
            quality = (quantized_tx_load * tx_load_percent +
                    quantized_qsize * (100 - tx_load_percent)) / 100;
            entry_index = (quantized_tx_load << 3) + quantized_qsize;
            if (soc_mem_is_valid(unit, DLB_HGT_PORT_QUALITY_MAPPINGm)) {
                soc_DLB_HGT_PORT_QUALITY_MAPPINGm_field32_set(unit,
                        &entry_arr[entry_index], ASSIGNED_QUALITYf, quality);
            } else if (soc_mem_field_valid(unit, DLB_HGT_QUALITY_MAPPINGm,
                        ASSIGNED_QUALITYf)) {
                soc_DLB_HGT_QUALITY_MAPPINGm_field32_set(unit,
                        &entry_arr[entry_index], ASSIGNED_QUALITYf, quality);
            } else {
                soc_DLB_HGT_QUALITY_MAPPINGm_field32_set(unit,
                        &entry_arr[entry_index], QUALITYf, quality);
            }
        }
    }

    return BCM_E_NONE;
}

#ifdef BCM_TRIUMPH3_SUPPORT
/*
 * Function:
 *      _bcm_tr3_hg_dlb_member_quality_map_set
 * Purpose:
 *      Set per-member Higig DLB quality mapping table.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      member_id - (IN) Member ID.
 *      tx_load_percent - (IN) Percent weight of port loading in determing
 *                             port quality. The remaining percentage is
 *                             the weight of queue size.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_hg_dlb_member_quality_map_set(int unit, int member_id,
        int tx_load_percent)
{
    soc_profile_mem_t *profile;
    soc_mem_t mem;
    int entry_bytes;
    int entries_per_profile;
    int alloc_size;
    uint32 *entry_arr;
    int rv = BCM_E_NONE;
    void *entries;
    uint32 base_index;
    uint32 old_base_index = 0;
    dlb_hgt_quality_control_entry_t quality_control_entry;
    soc_field_t profile_ptr_f;

    profile = _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile;

    entries_per_profile = 64;
    entry_bytes = sizeof(dlb_hgt_quality_mapping_entry_t);
    alloc_size = entries_per_profile * entry_bytes;
    entry_arr = sal_alloc(alloc_size, "HG DLB Quality Map entries");
    if (entry_arr == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(entry_arr, 0, alloc_size);

    /* Assign quality in the entry array */
    rv = _bcm_trident_hg_dlb_quality_assign(unit, tx_load_percent, entry_arr);
    if (BCM_FAILURE(rv)) {
        sal_free(entry_arr);
        return rv;
    }

    mem = DLB_HGT_QUALITY_MAPPINGm;
    soc_mem_lock(unit, mem);

    /* Add profile */
    entries = entry_arr;
    rv = soc_profile_mem_add(unit, profile, &entries, entries_per_profile,
            &base_index);
    sal_free(entry_arr);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }

    /* Update member profile pointer */
    rv = READ_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ANY, member_id,
            &quality_control_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }
    profile_ptr_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_CONTROLm,
            PROFILE_PTRf) ? PROFILE_PTRf : QMT_PROFILE_PTRf;
    old_base_index = entries_per_profile * soc_mem_field32_get(unit,
            DLB_HGT_QUALITY_CONTROLm, &quality_control_entry, profile_ptr_f);
    soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit, &quality_control_entry, 
            profile_ptr_f, base_index / entries_per_profile);
    rv = WRITE_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ALL, member_id,
            &quality_control_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }

    /* Delete old profile */
    rv = soc_profile_mem_delete(unit, profile, old_base_index);
    soc_mem_unlock(unit, mem);

    HG_DLB_INFO(unit)->hg_dlb_load_weight[base_index/entries_per_profile] =
        tx_load_percent;

    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      _bcm_trident_hg_dlb_quality_map_set
 * Purpose:
 *      Set per-port Higig DLB quality mapping table.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      port - (IN) Port number.
 *      tx_load_percent - (IN) Percent weight of port loading in determing
 *                             port quality. The remaining percentage is
 *                             the weight of queue size.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_quality_map_set(int unit, bcm_port_t port,
        int tx_load_percent)
{
    soc_profile_mem_t *profile;
    int entries_per_profile;
    int alloc_size;
    uint32 *entry_arr;
    int rv;
    void *entries;
    uint32 base_index;
    uint32 old_base_index;
    uint32 quantize_control_reg;
    soc_mem_t mem;

    profile = _trident_hg_dlb_bk[unit]->hg_dlb_quality_map_profile;

    entries_per_profile = 64;
    alloc_size = entries_per_profile *
        sizeof(dlb_hgt_port_quality_mapping_entry_t);
    entry_arr = sal_alloc(alloc_size, "HG DLB Quality Map entries");
    if (entry_arr == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(entry_arr, 0, alloc_size);

    /* Assign quality in the entry array */
    rv = _bcm_trident_hg_dlb_quality_assign(unit, tx_load_percent, entry_arr);
    if (BCM_FAILURE(rv)) {
        sal_free(entry_arr);
        return rv;
    }

    mem = DLB_HGT_PORT_QUALITY_MAPPINGm;
    soc_mem_lock(unit, mem);
    entries = entry_arr;
    rv = soc_profile_mem_add(unit, profile, &entries, entries_per_profile,
            &base_index);
    sal_free(entry_arr);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }

    rv = READ_DLB_HGT_QUANTIZE_CONTROLr(unit, port, &quantize_control_reg);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }
    old_base_index = entries_per_profile * soc_reg_field_get(unit,
            DLB_HGT_QUANTIZE_CONTROLr, quantize_control_reg,
            PORT_QUALITY_MAPPING_PROFILE_PTRf);
    soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
            &quantize_control_reg, PORT_QUALITY_MAPPING_PROFILE_PTRf,
            base_index / entries_per_profile);
    rv = WRITE_DLB_HGT_QUANTIZE_CONTROLr(unit, port, quantize_control_reg);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }

    rv = soc_profile_mem_delete(unit, profile, old_base_index);

    soc_mem_unlock(unit, mem);

    HG_DLB_INFO(unit)->hg_dlb_load_weight[base_index/entries_per_profile] =
        tx_load_percent;

    return rv;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_id_alloc
 * Purpose:
 *      Get a free Higig DLB ID and mark it used.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      hgtid - (IN) Higig trunk ID.
 *      dlb_id - (OUT) Allocated DLB ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_id_alloc(int unit, int hgtid, int *dlb_id)
{
    int i;

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_id_equal_hg_tid)) {
        if (_BCM_HG_DLB_ID_USED_GET(unit, hgtid)) {
            return BCM_E_EXISTS;
        }
        _BCM_HG_DLB_ID_USED_SET(unit, hgtid);
        *dlb_id = hgtid;
        return BCM_E_NONE;
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        for (i = 0; i < soc_mem_index_count(unit, DLB_HGT_GROUP_CONTROLm); i++) {
            if (!_BCM_HG_DLB_ID_USED_GET(unit, i)) {
                _BCM_HG_DLB_ID_USED_SET(unit, i);
                *dlb_id = i;
                return BCM_E_NONE;
            }
        }
    }

    return BCM_E_RESOURCE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_id_free
 * Purpose:
 *      Free a Higig DLB ID.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      dlb_id - (IN) DLB ID to be freed.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_id_free(int unit, int dlb_id)
{
    if (dlb_id < 0 || dlb_id > soc_mem_index_max(unit, DLB_HGT_GROUP_CONTROLm)) {
        return BCM_E_PARAM;
    }

    _BCM_HG_DLB_ID_USED_CLR(unit, dlb_id);

    return BCM_E_NONE;
}

#ifdef BCM_TRIUMPH3_SUPPORT
/*
 * Function:
 *      _bcm_tr3_hg_dlb_member_id_alloc
 * Purpose:
 *      Get a free Higig DLB Member ID and mark it used.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      member_id - (OUT) Allocated Member ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_hg_dlb_member_id_alloc(int unit, int *member_id)
{
    int i;

    for (i = 0; i < soc_mem_index_count(unit, DLB_HGT_MEMBER_ATTRIBUTEm); i++) {
        if (!_BCM_HG_DLB_MEMBER_ID_USED_GET(unit, i)) {
            _BCM_HG_DLB_MEMBER_ID_USED_SET(unit, i);
            *member_id = i;
            return BCM_E_NONE;
        }
    }

    return BCM_E_RESOURCE;
}

/*
 * Function:
 *      _bcm_tr3_hg_dlb_member_id_free
 * Purpose:
 *      Free a Higig DLB Member ID.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      member_id - (IN) Member ID to be freed.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_hg_dlb_member_id_free(int unit, int member_id)
{
    if (member_id < 0 || member_id > soc_mem_index_max(unit,
                DLB_HGT_MEMBER_ATTRIBUTEm)) {
        return BCM_E_PARAM;
    }

    _BCM_HG_DLB_MEMBER_ID_USED_CLR(unit, member_id);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_hg_dlb_member_array_alloc
 * Purpose:
 *      Allocate member IDs for each Higig trunk member.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      add_info - (IN) Pointer to trunk add info structure.
 *      member_id_array - (OUT) Array of member IDs.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_hg_dlb_member_array_alloc(int unit,
        _esw_trunk_add_info_t *add_info,
        int *member_id_array)
{
    int i;
    dlb_hgt_port_member_map_entry_t port_member_map_entry;
    port_tab_entry_t ptab;
    dlb_hgt_member_attribute_entry_t member_attr_entry;
    dlb_hgt_quality_control_entry_t quality_control_entry;
    soc_field_t enable_credit_collect_f;

    for (i = 0; i < add_info->num_ports; i++) {
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_PORT_MEMBER_MAPm(unit, MEM_BLOCK_ANY,
                    add_info->tp[i], &port_member_map_entry));
        if (soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                    &port_member_map_entry, VALIDf)) {
            member_id_array[i] =
                soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                        &port_member_map_entry, MEMBER_IDf);
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_hg_dlb_member_id_alloc(unit,
                        &member_id_array[i]));

            /* Set member map entry */ 
            soc_DLB_HGT_PORT_MEMBER_MAPm_field32_set(unit,
                    &port_member_map_entry, VALIDf, 1);
            soc_DLB_HGT_PORT_MEMBER_MAPm_field32_set(unit,
                    &port_member_map_entry, MEMBER_IDf, member_id_array[i]);
            if (soc_mem_field_valid(unit, DLB_HGT_PORT_MEMBER_MAPm, EMBEDDED_HGf)) {
                SOC_IF_ERROR_RETURN
                    (READ_PORT_TABm(unit, MEM_BLOCK_ANY, add_info->tp[i], &ptab));
                if (soc_PORT_TABm_field32_get(unit, &ptab, PORT_TYPEf) == 3) {
                    /* Port is an embedded Higig port */
                    soc_DLB_HGT_PORT_MEMBER_MAPm_field32_set(unit,
                            &port_member_map_entry, EMBEDDED_HGf, 1);
                } else {
                    soc_DLB_HGT_PORT_MEMBER_MAPm_field32_set(unit,
                            &port_member_map_entry, EMBEDDED_HGf, 0);
                }
            }
            SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_PORT_MEMBER_MAPm(unit,
                        MEM_BLOCK_ALL, add_info->tp[i],
                        &port_member_map_entry));

            /* Set member attribute entry */
            SOC_IF_ERROR_RETURN(READ_DLB_HGT_MEMBER_ATTRIBUTEm(unit,
                        MEM_BLOCK_ANY, member_id_array[i],
                        &member_attr_entry));
            soc_DLB_HGT_MEMBER_ATTRIBUTEm_field32_set(unit, &member_attr_entry,
                    PORT_NUMf, add_info->tp[i]);
            SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_MEMBER_ATTRIBUTEm(unit,
                        MEM_BLOCK_ALL, member_id_array[i],
                        &member_attr_entry));

            /* Set member quality mapping profile */
            BCM_IF_ERROR_RETURN(_bcm_tr3_hg_dlb_member_quality_map_set(unit,
                        member_id_array[i], add_info->dynamic_load_weight[i]));

            /* Enable member quality control */
            SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUALITY_CONTROLm(unit,
                        MEM_BLOCK_ANY, member_id_array[i],
                        &quality_control_entry));
            soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                    &quality_control_entry, ENABLE_AVG_CALCf, 1);
            soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                    &quality_control_entry, ENABLE_QUALITY_UPDATEf, 1);
            enable_credit_collect_f = soc_mem_field_valid(unit,
                    DLB_HGT_QUALITY_CONTROLm, ENABLE_CREDIT_COLLECTIONf) ?
                ENABLE_CREDIT_COLLECTIONf : ENB_CREDIT_COLLECTIONf;
            soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                    &quality_control_entry, enable_credit_collect_f, 1);
            soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                    &quality_control_entry, LOADING_SCALING_FACTORf,
                    add_info->dynamic_scaling_factor[i]);
            soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
                    &quality_control_entry, QSIZE_SCALING_FACTORf, 
                    add_info->dynamic_scaling_factor[i]);
            SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_QUALITY_CONTROLm(unit,
                        MEM_BLOCK_ALL, member_id_array[i],
                        &quality_control_entry));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_hg_dlb_member_free_resource
 * Purpose:
 *      Clear state and free resources associated with a Higig DLB member.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      member_id - (IN) Member ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_hg_dlb_member_free_resource(int unit, int member_id)
{
    dlb_hgt_quality_control_entry_t quality_control_entry;
    soc_field_t enable_credit_collect_f;
    dlb_hgt_member_attribute_entry_t member_attribute_entry;
    int port_num;

    /* Disable member quality control */
    SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ANY,
                member_id, &quality_control_entry));
    soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
            &quality_control_entry, ENABLE_AVG_CALCf, 0);
    soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
            &quality_control_entry, ENABLE_QUALITY_UPDATEf, 0);
    enable_credit_collect_f = soc_mem_field_valid(unit,
            DLB_HGT_QUALITY_CONTROLm, ENABLE_CREDIT_COLLECTIONf) ?
        ENABLE_CREDIT_COLLECTIONf : ENB_CREDIT_COLLECTIONf;
    soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit,
            &quality_control_entry, enable_credit_collect_f, 0);
    soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit, &quality_control_entry,
            LOADING_SCALING_FACTORf, 0);
    soc_DLB_HGT_QUALITY_CONTROLm_field32_set(unit, &quality_control_entry,
            QSIZE_SCALING_FACTORf, 0);
    SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ALL,
                member_id, &quality_control_entry));

    /* Revert member quality mapping profile to default */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_hg_dlb_member_quality_map_set(unit, member_id, 100));

    /* Clear member attribute entry */
    SOC_IF_ERROR_RETURN(READ_DLB_HGT_MEMBER_ATTRIBUTEm(unit, MEM_BLOCK_ANY,
                member_id, &member_attribute_entry));
    port_num = soc_DLB_HGT_MEMBER_ATTRIBUTEm_field32_get(unit,
            &member_attribute_entry, PORT_NUMf);
    SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_MEMBER_ATTRIBUTEm(unit, MEM_BLOCK_ALL,
                member_id, soc_mem_entry_null(unit, DLB_HGT_MEMBER_ATTRIBUTEm)));

    /* Clear member map entry */
    SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_PORT_MEMBER_MAPm(unit, MEM_BLOCK_ALL,
                port_num, soc_mem_entry_null(unit, DLB_HGT_PORT_MEMBER_MAPm)));

    /* Free member ID */
    BCM_IF_ERROR_RETURN(_bcm_tr3_hg_dlb_member_id_free(unit, member_id));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_hg_dlb_member_attr_get
 * Purpose:
 *      Get attributes of a Higig DLB member.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      port - (IN) Port number of the member.
 *      scaling_factor - (OUT) Member's scaling factor.
 *      load_weight    - (OUT) Member's load weight.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_hg_dlb_member_attr_get(int unit, bcm_port_t port,
        int *scaling_factor, int *load_weight)
{
    dlb_hgt_port_member_map_entry_t port_member_map_entry;
    int member_id;
    dlb_hgt_quality_control_entry_t quality_control_entry;
    soc_field_t profile_ptr_f;
    int profile_ptr = 0;

    /* Find member ID of the given module and port */
    SOC_IF_ERROR_RETURN(READ_DLB_HGT_PORT_MEMBER_MAPm(unit, MEM_BLOCK_ANY,
                port, &port_member_map_entry));
    if (!soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit, &port_member_map_entry,
                VALIDf)) {
        return BCM_E_NOT_FOUND;
    }
    member_id = soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
            &port_member_map_entry, MEMBER_IDf);

    /* Get member's scaling factor and load weight */
    SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUALITY_CONTROLm(unit, MEM_BLOCK_ANY,
                member_id, &quality_control_entry));
    *scaling_factor = soc_DLB_HGT_QUALITY_CONTROLm_field32_get(unit,
            &quality_control_entry, LOADING_SCALING_FACTORf);
    profile_ptr_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_CONTROLm,
            PROFILE_PTRf) ? PROFILE_PTRf : QMT_PROFILE_PTRf;
    profile_ptr = soc_DLB_HGT_QUALITY_CONTROLm_field32_get(unit,
            &quality_control_entry, profile_ptr_f);
    *load_weight = HG_DLB_INFO(unit)->hg_dlb_load_weight[profile_ptr];

    return BCM_E_NONE;
}

#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      _bcm_trident_hg_dlb_dynamic_size_encode
 * Purpose:
 *      Encode Higig trunk dynamic load balancing flow set size.
 * Parameters:
 *      dynamic_size - (IN) Number of flow sets.
 *      encoded_value - (OUT) Encoded value.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_dynamic_size_encode(int dynamic_size,
        int *encoded_value)
{   
    switch (dynamic_size) {
        case 512:
            *encoded_value = 1;
            break;
        case 1024:
            *encoded_value = 2;
            break;
        case 2048:
            *encoded_value = 3;
            break;
        case 4096:
            *encoded_value = 4;
            break;
        case 8192:
            *encoded_value = 5;
            break;
        case 16384:
            *encoded_value = 6;
            break;
        case 32768:
            *encoded_value = 7;
            break;
        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_dynamic_size_decode
 * Purpose:
 *      Decode Higig trunk dynamic load balancing flow set size.
 * Parameters:
 *      encoded_value - (IN) Encoded value.
 *      dynamic_size - (OUT) Number of flow sets.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_dynamic_size_decode(int encoded_value,
        int *dynamic_size)
{   
    switch (encoded_value) {
        case 1:
            *dynamic_size = 512;
            break;
        case 2:
            *dynamic_size = 1024;
            break;
        case 3:
            *dynamic_size = 2048;
            break;
        case 4:
            *dynamic_size = 4096;
            break;
        case 5:
            *dynamic_size = 8192;
            break;
        case 6:
            *dynamic_size = 16384;
            break;
        case 7:
            *dynamic_size = 32768;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_free_resource
 * Purpose:
 *      Free resources for a Higig dynamic load balancing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig group ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_free_resource(int unit, int hgtid)
{
    int rv = BCM_E_NONE;
    hgt_dlb_control_entry_t hgt_dlb_control_entry;
    int dlb_enable, dlb_id;
    dlb_hgt_group_control_entry_t dlb_hgt_group_control_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int block_base_ptr;
    int num_blocks;

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_id_equal_hg_tid)) {
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY,
                    hgtid, &dlb_hgt_group_control_entry));
        dlb_enable = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                &dlb_hgt_group_control_entry, GROUP_ENABLEf);
        dlb_id = hgtid;
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        SOC_IF_ERROR_RETURN(READ_HGT_DLB_CONTROLm(unit, MEM_BLOCK_ANY, hgtid,
                    &hgt_dlb_control_entry));
        dlb_enable = soc_HGT_DLB_CONTROLm_field32_get(unit,
                &hgt_dlb_control_entry, GROUP_ENABLEf);
        dlb_id = soc_HGT_DLB_CONTROLm_field32_get(unit,
                &hgt_dlb_control_entry, DLB_IDf);
    }
    if (!dlb_enable) {
        return BCM_E_NONE;
    }

    /* Clear ENHANCED_HASHING_ENABLE in HG_TRUNK_GROUP */
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_mem_field_valid(unit, HG_TRUNK_GROUPm, ENHANCED_HASHING_ENABLEf)) {
        hg_trunk_group_entry_t hg_trunk_group_entry;

        SOC_IF_ERROR_RETURN(READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                    &hg_trunk_group_entry));
        soc_HG_TRUNK_GROUPm_field32_set(unit, &hg_trunk_group_entry,
                ENHANCED_HASHING_ENABLEf, 0);
        SOC_IF_ERROR_RETURN(WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid,
                    &hg_trunk_group_entry));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Clear HGT_DLB_CONTROL entry */
    if (soc_mem_is_valid(unit, HGT_DLB_CONTROLm)) {
        SOC_IF_ERROR_RETURN(WRITE_HGT_DLB_CONTROLm(unit, MEM_BLOCK_ALL, hgtid,
                    soc_mem_entry_null(unit, HGT_DLB_CONTROLm)));
    }

    /* Clear DLB_HGT_GROUP_CONTROL entry */
    SOC_IF_ERROR_RETURN(READ_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY, dlb_id,
                &dlb_hgt_group_control_entry));
    entry_base_ptr = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
            &dlb_hgt_group_control_entry, FLOW_SET_BASEf);
    flow_set_size = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
            &dlb_hgt_group_control_entry, FLOW_SET_SIZEf);
    SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ALL,
                dlb_id, soc_mem_entry_null(unit, DLB_HGT_GROUP_CONTROLm)));

    /* Free flow set table resources */
    BCM_IF_ERROR_RETURN
        (_bcm_trident_hg_dlb_dynamic_size_decode(flow_set_size, &num_entries));
    block_base_ptr = entry_base_ptr >> 9;
    num_blocks = num_entries >> 9; 
    _BCM_HG_DLB_FLOWSET_BLOCK_USED_CLR_RANGE(unit, block_base_ptr, num_blocks);

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
        dlb_hgt_group_membership_entry_t dlb_hgt_group_membership_entry;
        int member_bitmap_width, alloc_size;
        int i;
        SHR_BITDCL *member_bitmap = NULL;
        SHR_BITDCL *status_bitmap = NULL;
        SHR_BITDCL *override_bitmap = NULL;
        dlb_hgt_member_sw_state_entry_t dlb_hgt_member_sw_state_entry;

        /* Get membership and clear each member's state */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_GROUP_MEMBERSHIPm(unit, MEM_BLOCK_ANY,
                    dlb_id, &dlb_hgt_group_membership_entry));
        member_bitmap_width = soc_mem_field_length(unit,
            DLB_HGT_GROUP_MEMBERSHIPm, MEMBER_BITMAPf);
        alloc_size = SHR_BITALLOCSIZE(member_bitmap_width);
        member_bitmap = sal_alloc(alloc_size, "DLB HGT member bitmap"); 
        if (NULL == member_bitmap) {
            return BCM_E_MEMORY;
        }
        soc_DLB_HGT_GROUP_MEMBERSHIPm_field_get(unit,
            &dlb_hgt_group_membership_entry, MEMBER_BITMAPf, member_bitmap);
        for (i = 0; i < member_bitmap_width; i++) {
            if (SHR_BITGET(member_bitmap, i)) {
                rv = _bcm_tr3_hg_dlb_member_free_resource(unit, i);
                if (BCM_FAILURE(rv)) {
                    sal_free(member_bitmap);
                    return rv;
                }
            }
        }

        /* Clear member software state */
        rv = READ_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ANY, 0,
                &dlb_hgt_member_sw_state_entry);
        if (BCM_FAILURE(rv)) {
            sal_free(member_bitmap);
            return rv;
        }

        status_bitmap = sal_alloc(alloc_size, "DLB HGT member status bitmap"); 
        if (NULL == status_bitmap) {
            sal_free(member_bitmap);
            return BCM_E_MEMORY;
        }
        soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit,
                &dlb_hgt_member_sw_state_entry, MEMBER_BITMAPf, status_bitmap);
        SHR_BITREMOVE_RANGE(status_bitmap, member_bitmap, 0, member_bitmap_width,
                status_bitmap);
        soc_DLB_HGT_MEMBER_SW_STATEm_field_set(unit,
                &dlb_hgt_member_sw_state_entry, MEMBER_BITMAPf, status_bitmap);

        override_bitmap = sal_alloc(alloc_size, "DLB HGT member override bitmap"); 
        if (NULL == override_bitmap) {
            sal_free(member_bitmap);
            sal_free(status_bitmap);
            return BCM_E_MEMORY;
        }
        soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit,
                &dlb_hgt_member_sw_state_entry, OVERRIDE_MEMBER_BITMAPf, override_bitmap);
        SHR_BITREMOVE_RANGE(override_bitmap, member_bitmap, 0, member_bitmap_width,
                override_bitmap);
        soc_DLB_HGT_MEMBER_SW_STATEm_field_set(unit,
                &dlb_hgt_member_sw_state_entry, OVERRIDE_MEMBER_BITMAPf, override_bitmap);

        rv = WRITE_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ALL, 0,
                    &dlb_hgt_member_sw_state_entry);
        sal_free(member_bitmap);
        sal_free(status_bitmap);
        sal_free(override_bitmap);
        if (BCM_FAILURE(rv)) {
            return rv;
        }

        /* Clear group membership */
        SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_GROUP_MEMBERSHIPm(unit,
                    MEM_BLOCK_ALL, dlb_id, soc_mem_entry_null(unit,
                        DLB_HGT_GROUP_MEMBERSHIPm)));
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        dlb_hgt_group_membership_entry_t dlb_hgt_group_membership_entry;
        bcm_pbmp_t port_map, negated_port_map;
        bcm_port_t port;
        uint32 quantize_control;
        uint32 measure_update_control;
        dlb_hgt_link_control_entry_t dlb_hgt_link_control_entry;
        bcm_pbmp_t sw_port_state, override_port_map;

        SOC_IF_ERROR_RETURN(READ_DLB_HGT_GROUP_MEMBERSHIPm(unit, MEM_BLOCK_ANY,
                    dlb_id, &dlb_hgt_group_membership_entry));
        soc_mem_pbmp_field_get(unit, DLB_HGT_GROUP_MEMBERSHIPm,
                &dlb_hgt_group_membership_entry, PORT_MAPf, &port_map);
        BCM_PBMP_ITER(port_map, port) {
            /* Disable quality measure update */
            SOC_IF_ERROR_RETURN
                (READ_DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr(unit,
                    port, &measure_update_control));
            soc_reg_field_set(unit, DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                    &measure_update_control, ENABLE_MEASURE_COLLECTIONf, 0);
            soc_reg_field_set(unit, DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                    &measure_update_control, ENABLE_MEASURE_AVERAGE_CALCULATIONf, 0);
            soc_reg_field_set(unit, DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                    &measure_update_control, ENABLE_PORT_QUALITY_UPDATEf, 0);
            SOC_IF_ERROR_RETURN
                (WRITE_DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr(unit,
                    port, measure_update_control));

            /* Revert member quality mapping profile to default */
            BCM_IF_ERROR_RETURN
                (_bcm_trident_hg_dlb_quality_map_set(unit, port, 100));

            /* Clear threshold scaling factors */
            SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUANTIZE_CONTROLr(unit, port,
                        &quantize_control));
            soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
                    &quantize_control, PORT_LOADING_THRESHOLD_SCALING_FACTORf, 0);
            soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
                    &quantize_control, PORT_QSIZE_THRESHOLD_SCALING_FACTORf, 0);
            SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_QUANTIZE_CONTROLr(unit, port,
                        quantize_control));
        }

        /* Clear member software state */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_LINK_CONTROLm(unit, MEM_BLOCK_ANY,
                    0, &dlb_hgt_link_control_entry));

        soc_mem_pbmp_field_get(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_PORT_STATEf, &sw_port_state);
        BCM_PBMP_NEGATE(negated_port_map, port_map);
        BCM_PBMP_AND(sw_port_state, negated_port_map);
        soc_mem_pbmp_field_set(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_PORT_STATEf, &sw_port_state);

        soc_mem_pbmp_field_get(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_OVERRIDE_PORT_MAPf,
                &override_port_map);
        BCM_PBMP_AND(override_port_map, negated_port_map);
        soc_mem_pbmp_field_set(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_OVERRIDE_PORT_MAPf,
                &override_port_map);

        SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_LINK_CONTROLm(unit, MEM_BLOCK_ALL,
                    0, &dlb_hgt_link_control_entry));

        /* Clear group membership */
        SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_GROUP_MEMBERSHIPm(unit,
                    MEM_BLOCK_ALL, dlb_id, soc_mem_entry_null(unit,
                        DLB_HGT_GROUP_MEMBERSHIPm)));
    }

    /* Free DLB ID */
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_id_free(unit, dlb_id));

    return rv;
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_set
 * Purpose:
 *      Configure a Higig dynamic load balancing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_set(int unit,
        int hgtid,
        _esw_trunk_add_info_t *add_info)
{
    int dlb_enable;
    int dlb_id;
    int num_blocks;
    int total_blocks;
    int max_block_base_ptr;
    int block_base_ptr;
    SHR_BITDCL occupied;
    int entry_base_ptr;
    int num_entries_per_block;
    int alloc_size;
    uint32 *block_ptr = NULL;
    int i, k;
    int index_min, index_max;
    int rv = BCM_E_NONE;
    dlb_hgt_group_control_entry_t dlb_hgt_group_control_entry;
    int flow_set_size;
    int dlb_mode;
    hgt_dlb_control_entry_t hgt_dlb_control_entry;
    int *member_id_array = NULL;
    soc_mem_t flowset_mem;
    int flowset_entry_size;
    SHR_BITDCL *member_bitmap = NULL;
    SHR_BITDCL *override_bitmap = NULL;

    dlb_enable = (add_info->psc == BCM_TRUNK_PSC_DYNAMIC) ||
        (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_ASSIGNED) ||
        (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_OPTIMAL);
    if (!dlb_enable) {
        return BCM_E_NONE;
    }

    /* Allocate a free DLB ID */
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_id_alloc(unit, hgtid, &dlb_id));

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
        int member_bitmap_width;
        dlb_hgt_group_membership_entry_t dlb_hgt_group_membership_entry;
        dlb_hgt_member_sw_state_entry_t dlb_hgt_member_sw_state_entry;

        /* Allocate member IDs */
        member_id_array = sal_alloc(sizeof(int) * add_info->num_ports,
                "Higig DLB Member IDs");
        if (NULL == member_id_array) {
            rv = BCM_E_MEMORY;
            goto error;
        }
        rv = _bcm_tr3_hg_dlb_member_array_alloc(unit, add_info,
                member_id_array);
        if (BCM_FAILURE(rv)) {
            goto error;
        }

        /* Set group membership */
        member_bitmap_width = soc_mem_field_length(unit,
                DLB_HGT_GROUP_MEMBERSHIPm, MEMBER_BITMAPf);
        alloc_size = SHR_BITALLOCSIZE(member_bitmap_width);
        member_bitmap = sal_alloc(alloc_size, "DLB HGT member bitmap"); 
        if (NULL == member_bitmap) {
            rv = BCM_E_MEMORY;
            goto error;
        }
        sal_memset(member_bitmap, 0, alloc_size);
        for (i = 0; i < add_info->num_ports; i++) {
            SHR_BITSET(member_bitmap, member_id_array[i]);
        }
        sal_memset(&dlb_hgt_group_membership_entry, 0,
                sizeof(dlb_hgt_group_membership_entry_t));
        soc_DLB_HGT_GROUP_MEMBERSHIPm_field_set(unit,
                &dlb_hgt_group_membership_entry, MEMBER_BITMAPf, member_bitmap);
        rv = WRITE_DLB_HGT_GROUP_MEMBERSHIPm(unit, MEM_BLOCK_ALL, dlb_id,
                &dlb_hgt_group_membership_entry);
        if (BCM_FAILURE(rv)) {
            goto error;
        }

        /* Disable member state software override */
        rv = READ_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ANY,
                0, &dlb_hgt_member_sw_state_entry);
        if (BCM_FAILURE(rv)) {
            goto error;
        }
        override_bitmap = sal_alloc(alloc_size, "DLB HGT member override bitmap"); 
        if (NULL == override_bitmap) {
            rv = BCM_E_MEMORY;
            goto error;
        }
        soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit,
            &dlb_hgt_member_sw_state_entry, OVERRIDE_MEMBER_BITMAPf,
            override_bitmap);
        SHR_BITREMOVE_RANGE(override_bitmap, member_bitmap, 0, member_bitmap_width,
                override_bitmap);
        soc_DLB_HGT_MEMBER_SW_STATEm_field_set(unit,
            &dlb_hgt_member_sw_state_entry, OVERRIDE_MEMBER_BITMAPf,
            override_bitmap);
        rv = WRITE_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ALL,
                0, &dlb_hgt_member_sw_state_entry);
        if (BCM_FAILURE(rv)) {
            goto error;
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        uint32 quantize_control;
        uint32 measure_update_control;
        dlb_hgt_group_membership_entry_t dlb_hgt_group_membership_entry;
        bcm_pbmp_t port_map, negated_port_map, override_port_map;
        dlb_hgt_link_control_entry_t dlb_hgt_link_control_entry;

        BCM_PBMP_CLEAR(port_map);
        for (i = 0; i < add_info->num_ports; i++) {
            /* Set port quality mapping profile */
            rv = _bcm_trident_hg_dlb_quality_map_set(unit,
                    add_info->tp[i], add_info->dynamic_load_weight[i]);
            if (BCM_FAILURE(rv)) {
                goto error;
            }

            /* Set port threshold scaling factors */
            rv = READ_DLB_HGT_QUANTIZE_CONTROLr(unit, add_info->tp[i],
                    &quantize_control);
            if (BCM_FAILURE(rv)) {
                goto error;
            }
            soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
                    &quantize_control, PORT_LOADING_THRESHOLD_SCALING_FACTORf,
                    add_info->dynamic_scaling_factor[i]);
            soc_reg_field_set(unit, DLB_HGT_QUANTIZE_CONTROLr,
                    &quantize_control, PORT_QSIZE_THRESHOLD_SCALING_FACTORf,
                    add_info->dynamic_scaling_factor[i]);
            rv = WRITE_DLB_HGT_QUANTIZE_CONTROLr(unit, add_info->tp[i],
                    quantize_control);
            if (BCM_FAILURE(rv)) {
                goto error;
            }

            /* Enable quality measure update */
            rv = READ_DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr(unit,
                    add_info->tp[i], &measure_update_control);
            if (BCM_FAILURE(rv)) {
                goto error;
            }
            soc_reg_field_set(unit, DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                    &measure_update_control, ENABLE_MEASURE_COLLECTIONf, 1);
            soc_reg_field_set(unit, DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                    &measure_update_control, ENABLE_MEASURE_AVERAGE_CALCULATIONf, 1);
            soc_reg_field_set(unit, DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr,
                    &measure_update_control, ENABLE_PORT_QUALITY_UPDATEf, 1);
            rv = WRITE_DLB_HGT_PORT_QUALITY_MEASURE_UPDATE_CONTROLr(unit,
                    add_info->tp[i], measure_update_control);
            if (BCM_FAILURE(rv)) {
                goto error;
            }

            /* Set group port bitmap */
            BCM_PBMP_PORT_ADD(port_map, add_info->tp[i]);
        }

        /* Set group membership */
        sal_memset(&dlb_hgt_group_membership_entry, 0,
                sizeof(dlb_hgt_group_membership_entry_t));
        soc_mem_pbmp_field_set(unit, DLB_HGT_GROUP_MEMBERSHIPm,
                &dlb_hgt_group_membership_entry, PORT_MAPf, &port_map);
        rv = WRITE_DLB_HGT_GROUP_MEMBERSHIPm(unit, MEM_BLOCK_ALL, dlb_id,
                &dlb_hgt_group_membership_entry);
        if (BCM_FAILURE(rv)) {
            goto error;
        }

        /* Disable member state software override */
        rv = READ_DLB_HGT_LINK_CONTROLm(unit, MEM_BLOCK_ANY, 0,
                    &dlb_hgt_link_control_entry);
        if (BCM_FAILURE(rv)) {
            goto error;
        }
        soc_mem_pbmp_field_get(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_OVERRIDE_PORT_MAPf,
                &override_port_map);
        BCM_PBMP_NEGATE(negated_port_map, port_map);
        BCM_PBMP_AND(override_port_map, negated_port_map);
        soc_mem_pbmp_field_set(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_OVERRIDE_PORT_MAPf,
                &override_port_map);
        rv = WRITE_DLB_HGT_LINK_CONTROLm(unit, MEM_BLOCK_ALL, 0,
                    &dlb_hgt_link_control_entry);
        if (BCM_FAILURE(rv)) {
            goto error;
        }
    }

    /* Find a contiguous region of flow set table that's large
     * enough to hold the requested number of flow sets.
     * The flow set table is allocated in 512-entry blocks.
     */
    if (soc_mem_is_valid(unit, DLB_HGT_FLOWSET_PORTm)) {
        flowset_mem = DLB_HGT_FLOWSET_PORTm;
        flowset_entry_size = sizeof(dlb_hgt_flowset_port_entry_t);
    } else {
        flowset_mem = DLB_HGT_FLOWSETm;
        flowset_entry_size = sizeof(dlb_hgt_flowset_entry_t);
    }
    num_blocks = add_info->dynamic_size >> 9;
    total_blocks = soc_mem_index_count(unit, flowset_mem) >> 9;
    max_block_base_ptr = total_blocks - num_blocks;
    for (block_base_ptr = 0;
         block_base_ptr <= max_block_base_ptr;
         block_base_ptr++) {
        /* Check if the contiguous region of flow set table from
         * block_base_ptr to (block_base_ptr + num_blocks - 1) is free. 
         */
        _BCM_HG_DLB_FLOWSET_BLOCK_TEST_RANGE(unit, block_base_ptr, num_blocks,
                occupied); 
        if (!occupied) {
            break;
        }
    }
    if (block_base_ptr > max_block_base_ptr) {
        /* A contiguous region of the desired size could not be found in
         * flow set table.
         */
        rv = BCM_E_RESOURCE;
        goto error;
    }

    /* Configure flow set table */
    entry_base_ptr = block_base_ptr << 9;
    num_entries_per_block = 512;
    alloc_size = num_entries_per_block * flowset_entry_size;
    block_ptr = soc_cm_salloc(unit, alloc_size,
            "Block of DLB_HGT_FLOWSET entries");
    if (NULL == block_ptr) {
        rv = BCM_E_MEMORY;
        goto error;
    }
    sal_memset(block_ptr, 0, alloc_size);
    for (i = 0; i < num_blocks; i++) {
        for (k = 0; k < num_entries_per_block; k++) {
            if (soc_mem_is_valid(unit, DLB_HGT_FLOWSET_PORTm)) {
                dlb_hgt_flowset_port_entry_t *flowset_port_entry;

                flowset_port_entry = soc_mem_table_idx_to_pointer(unit,
                        DLB_HGT_FLOWSET_PORTm, dlb_hgt_flowset_port_entry_t *,
                        block_ptr, k);
                soc_DLB_HGT_FLOWSET_PORTm_field32_set(unit, flowset_port_entry,
                        VALIDf, 1);
                soc_DLB_HGT_FLOWSET_PORTm_field32_set(unit, flowset_port_entry,
                        PORT_NUMf, add_info->tp[(i * num_entries_per_block + k)
                        % add_info->num_ports]);
            } else {
                dlb_hgt_flowset_entry_t *flowset_entry;

                flowset_entry = soc_mem_table_idx_to_pointer(unit,
                        DLB_HGT_FLOWSETm, dlb_hgt_flowset_entry_t *,
                        block_ptr, k);
                soc_DLB_HGT_FLOWSETm_field32_set(unit, flowset_entry, VALIDf, 1);
                soc_DLB_HGT_FLOWSETm_field32_set(unit, flowset_entry, MEMBER_IDf,
                        member_id_array[(i * num_entries_per_block + k)
                        % add_info->num_ports]);
            }
        }
        index_min = entry_base_ptr + i * num_entries_per_block;
        index_max = index_min + num_entries_per_block - 1;
        rv = soc_mem_write_range(unit, flowset_mem, MEM_BLOCK_ALL,
                index_min, index_max, block_ptr);
        if (SOC_FAILURE(rv)) {
            goto error;
        }
    }
    _BCM_HG_DLB_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr, num_blocks);

    /* Update Higig DLB group control table */
    sal_memset(&dlb_hgt_group_control_entry, 0,
            sizeof(dlb_hgt_group_control_entry_t));
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_mem_field_valid(unit, DLB_HGT_GROUP_CONTROLm, GROUP_ENABLEf)) {
        soc_DLB_HGT_GROUP_CONTROLm_field32_set(unit, &dlb_hgt_group_control_entry,
                GROUP_ENABLEf, 1);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    soc_DLB_HGT_GROUP_CONTROLm_field32_set(unit, &dlb_hgt_group_control_entry,
            ENABLE_OPTIMAL_CANDIDATE_UPDATEf, 1);
    soc_DLB_HGT_GROUP_CONTROLm_field32_set(unit, &dlb_hgt_group_control_entry,
            FLOW_SET_BASEf, entry_base_ptr);

    rv = _bcm_trident_hg_dlb_dynamic_size_encode(add_info->dynamic_size,
                                                 &flow_set_size);
    if (SOC_FAILURE(rv)) {
        goto error;
    }
    soc_DLB_HGT_GROUP_CONTROLm_field32_set(unit, &dlb_hgt_group_control_entry,
            FLOW_SET_SIZEf, flow_set_size);

    switch (add_info->psc) {
        case BCM_TRUNK_PSC_DYNAMIC:
            dlb_mode = 0;
            break;
        case BCM_TRUNK_PSC_DYNAMIC_ASSIGNED:
            dlb_mode = 1;
            break;
        case BCM_TRUNK_PSC_DYNAMIC_OPTIMAL:
            dlb_mode = 2;
            break;
        default:
            rv = BCM_E_PARAM;
            goto error;
    }
#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_mem_field_valid(unit, DLB_HGT_GROUP_CONTROLm,
                MEMBER_ASSIGNMENT_MODEf)) {
        soc_DLB_HGT_GROUP_CONTROLm_field32_set(unit,
                &dlb_hgt_group_control_entry,
                MEMBER_ASSIGNMENT_MODEf, dlb_mode);
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        soc_DLB_HGT_GROUP_CONTROLm_field32_set(unit,
                &dlb_hgt_group_control_entry,
                PORT_ASSIGNMENT_MODEf, dlb_mode);
    }

    soc_DLB_HGT_GROUP_CONTROLm_field32_set(unit, &dlb_hgt_group_control_entry,
            INACTIVITY_DURATIONf, add_info->dynamic_age);

    rv = WRITE_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ALL, dlb_id,
                                      &dlb_hgt_group_control_entry);
    if (SOC_FAILURE(rv)) {
        goto error;
    }

    /* Update Higig DLB control table */
    if (soc_mem_is_valid(unit, HGT_DLB_CONTROLm)) {
        sal_memset(&hgt_dlb_control_entry, 0, sizeof(hgt_dlb_control_entry_t));
        soc_HGT_DLB_CONTROLm_field32_set
            (unit, &hgt_dlb_control_entry, DLB_IDf, dlb_id);
        soc_HGT_DLB_CONTROLm_field32_set
            (unit, &hgt_dlb_control_entry, GROUP_ENABLEf, 1);
        rv = WRITE_HGT_DLB_CONTROLm(unit, MEM_BLOCK_ALL, hgtid,
                &hgt_dlb_control_entry);
        if (SOC_FAILURE(rv)) {
            goto error;
        }
    }
    
    /* Update HG_TRUNK_GROUP */
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_mem_field_valid(unit, HG_TRUNK_GROUPm, ENHANCED_HASHING_ENABLEf)) {
        hg_trunk_group_entry_t hg_trunk_group_entry;

        rv = READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                &hg_trunk_group_entry);
        if (SOC_FAILURE(rv)) {
            goto error;
        }
        soc_HG_TRUNK_GROUPm_field32_set(unit, &hg_trunk_group_entry,
                ENHANCED_HASHING_ENABLEf, 1);
        rv = WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid,
                &hg_trunk_group_entry);
        if (SOC_FAILURE(rv)) {
            goto error;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

error:
    if (NULL != member_id_array) {
        sal_free(member_id_array);
    }
    if (NULL != member_bitmap) {
        sal_free(member_bitmap);
    }
    if (NULL != override_bitmap) {
        sal_free(override_bitmap);
    }
    if (NULL != block_ptr) { 
        soc_cm_sfree(unit, block_ptr);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_trident_hg_slb_free_resource
 * Purpose:
 *      Free resources for a Higig static load balancing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig trunk group ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_slb_free_resource(int unit, int hgtid)
{
    hg_trunk_group_entry_t hg_trunk_group_entry;
    int base_ptr;
    int tg_size;
    int rtag;
    int num_entries;

    SOC_IF_ERROR_RETURN
        (READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                              &hg_trunk_group_entry));
    base_ptr = soc_HG_TRUNK_GROUPm_field32_get(unit,
            &hg_trunk_group_entry, BASE_PTRf);
    tg_size = 1 + soc_HG_TRUNK_GROUPm_field32_get(unit,
            &hg_trunk_group_entry, TG_SIZEf);
    rtag = soc_HG_TRUNK_GROUPm_field32_get(unit,
            &hg_trunk_group_entry, RTAGf);

    num_entries = tg_size;
    if (rtag >= 1 && rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANAX(unit)) {
            num_entries = _BCM_KATANA_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
        } else
#endif
        if (soc_feature(unit,
                    soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
            num_entries = _BCM_TD_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
        }
    }

    _BCM_HG_TRUNK_MEMBER_USED_CLR_RANGE(unit, base_ptr, num_entries);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_slb_set
 * Purpose:
 *      Configure a Higig static load balancing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure.
 *      t_info   - (IN) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_slb_set(int unit,
        int hgtid,
        _esw_trunk_add_info_t *add_info,
        trunk_private_t *t_info)
{
    int num_entries;
    int max_base_ptr;
    int base_ptr;
    SHR_BITDCL occupied;
    int i;
    hg_trunk_member_entry_t hg_trunk_member_entry;
    hg_trunk_group_entry_t  hg_trunk_group_entry;

    /* Find a contiguous region of HG_TRUNK_MEMBER table that's large
     * enough to hold all of the Higig trunk group's member ports.
     */

    num_entries = add_info->num_ports;

    /* If RTAG is 1-6, Trident A0 and Katana ignore the TG_SIZE field and assume 
     * the Higig trunk group occupies FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT
     * contiguous entries in HG_TRUNK_MEMBER table.
     */
    if (t_info->rtag >= 1 && t_info->rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
        if(SOC_IS_KATANAX(unit)) {
            num_entries = _BCM_KATANA_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
        } else
#endif
        if (soc_feature(unit,
                    soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
            num_entries = _BCM_TD_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
        }
    } 

    max_base_ptr = soc_mem_index_count(unit, HG_TRUNK_MEMBERm) - num_entries;
    for (base_ptr = 0; base_ptr <= max_base_ptr; base_ptr++) {
        /* Check if the contiguous region of HG_TRUNK_MEMBERm table from
         * index base_ptr to (base_ptr + num_entries - 1) is free. 
         */
        _BCM_HG_TRUNK_MEMBER_TEST_RANGE(unit, base_ptr, num_entries, occupied); 
        if (!occupied) {
            break;
        }
    }

    if (base_ptr > max_base_ptr) {
        /* A contiguous region of the desired size could not be found in
         * HG_TRUNK_MEMBER table.
         */
        return BCM_E_RESOURCE;
    }

    for (i = 0; i < num_entries; i++) {
        sal_memset(&hg_trunk_member_entry, 0, sizeof(hg_trunk_member_entry_t));
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            soc_HG_TRUNK_MEMBERm_field32_set(unit, &hg_trunk_member_entry,
                    PHYSICAL_PORTf, add_info->tp[i % add_info->num_ports]);
            soc_HG_TRUNK_MEMBERm_field32_set(unit, &hg_trunk_member_entry,
                    PP_PORTf, add_info->tp[i % add_info->num_ports]);
        } else 
#endif /*  (defined(BCM_KATANA2_SUPPORT)  */
        {
            soc_HG_TRUNK_MEMBERm_field32_set(unit, &hg_trunk_member_entry,
                    PORT_NUMf, add_info->tp[i % add_info->num_ports]);
        }

        SOC_IF_ERROR_RETURN
            (WRITE_HG_TRUNK_MEMBERm(unit, MEM_BLOCK_ALL, base_ptr + i,
                                    &hg_trunk_member_entry)); 
    }

    /* Set hg_trunk_member_bitmap */
    _BCM_HG_TRUNK_MEMBER_USED_SET_RANGE(unit, base_ptr, num_entries);

    /* Update Higig trunk base_ptr, tg_size, and rtag */
    SOC_IF_ERROR_RETURN(READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
            &hg_trunk_group_entry));
    soc_HG_TRUNK_GROUPm_field32_set
        (unit, &hg_trunk_group_entry, BASE_PTRf, base_ptr);
    soc_HG_TRUNK_GROUPm_field32_set
        (unit, &hg_trunk_group_entry, TG_SIZEf, add_info->num_ports-1);
    soc_HG_TRUNK_GROUPm_field32_set
        (unit, &hg_trunk_group_entry, RTAGf, t_info->rtag);
    SOC_IF_ERROR_RETURN
        (WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid, &hg_trunk_group_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trident_trunk_fabric_port_set
 * Purpose:
 *      Update port table for Higig trunks.
 * Parameters:
 *      unit      - (IN) SOC unit number. 
 *      hgtid     - (IN) Higig trunk group ID.
 *      old_ports - (IN) Higig trunk group's old port bitmap.
 *      new_ports - (IN) Higig trunk group's new port bitmap.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_fabric_port_set(int unit,
        bcm_trunk_t hgtid,
        pbmp_t old_ports,
        pbmp_t new_ports)
{
    bcm_port_t		port;
    int			ctid;

    PBMP_HG_ITER(unit, port) {
        if (BCM_PBMP_MEMBER(new_ports, port)) {
            ctid = hgtid;
        } else if (BCM_PBMP_MEMBER(old_ports, port)) {
            ctid = -1;
            if (soc_feature(unit, soc_feature_hg_trunk_failover)) {
                /* Clear HW failover info for deleted port */
                BCM_IF_ERROR_RETURN
                    (_bcm_trident_trunk_hwfailover_set(unit, hgtid, TRUE,
                                                       port, -1,
                                                       BCM_TRUNK_PSC_DEFAULT,
                                                       0, 0, NULL, NULL));
            }
        } else {
            continue;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port,
                                      _bcmPortHigigTrunkId, ctid));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trident_trunk_fabric_modify
 * Purpose:
 *      Set, add or delete trunk members.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      hgtid    - (IN) Higig trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure.
 *      t_info   - (IN) Pointer to trunk info.
 *      op       - (IN) Type of operation: TRUNK_MEMBER_OP_SET, ADD or DELETE.
 *      member   - (IN) Member to add or delete.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_fabric_modify(int unit,
        bcm_trunk_t hgtid,
        _esw_trunk_add_info_t *add_info,
        trunk_private_t *t_info,
        int op,
        bcm_trunk_member_t *member)
{
    bcm_trunk_chip_info_t chip_info;
    int          i, local_id;
    bcm_gport_t  gport;
    bcm_trunk_t  trunk, tid;
    pbmp_t       old_trunk_pbmp, new_trunk_pbmp;
    hg_trunk_bitmap_entry_t hg_trunk_bitmap_entry;
    int          auto_include_disable = 0;
    int          rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));

    if (add_info->num_ports > chip_info.trunk_fabric_ports_max ||
            add_info->num_ports < 1) {
        return BCM_E_PARAM;
    }
 
    /* On Trident, the max number of trunk members for RTAG 1-6 is smaller
     * than for RTAG 7.
     */
    if (soc_feature(unit, soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
        if (t_info->rtag >= 1 && t_info->rtag <= 6) {
            if (add_info->num_ports > _BCM_TD_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT) {
                return BCM_E_PARAM;
            }
        }
    }

    tid = hgtid + chip_info.trunk_group_count;

    SOC_PBMP_CLEAR(new_trunk_pbmp);
    for (i = 0; i < add_info->num_ports; i++) {
        /* if given modid is -1 devport was used */
        if (BCM_MODID_INVALID != add_info->tm[i]) {
            BCM_GPORT_MODPORT_SET(gport, add_info->tm[i], add_info->tp[i]);

            BCM_IF_ERROR_RETURN
                (_bcm_esw_gport_resolve(unit, gport, &add_info->tm[i],
                                        &add_info->tp[i], &trunk, &local_id));
        } 

        if (!SOC_PORT_VALID(unit, add_info->tp[i])) {
            return BCM_E_PORT;
        }
        if (!IS_ST_PORT(unit, add_info->tp[i])) {
            return BCM_E_PARAM;
        }
        SOC_PBMP_PORT_ADD(new_trunk_pbmp, add_info->tp[i]);
    }

    /* Configure static load balancing */
    if (t_info->in_use) {
        BCM_IF_ERROR_RETURN(_bcm_trident_hg_slb_free_resource(unit, hgtid));
    }
    BCM_IF_ERROR_RETURN(_bcm_trident_hg_slb_set(unit, hgtid, add_info,
                    t_info));

    /* Configure dynamic load balancing, if enabled */
    if (soc_feature(unit, soc_feature_hg_dlb)) {
        if (t_info->in_use) {
            BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_free_resource(unit, hgtid));
        } 
        BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_set(unit, hgtid, add_info));
    }

#ifdef BCM_TRIDENT2_SUPPORT
    /* Configure resilient hashing, if enabled */
    if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
        if (op == TRUNK_MEMBER_OP_SET) {
            if (t_info->in_use) {
                BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_free_resource(unit, hgtid));
            } 
            if (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_set(unit, hgtid, add_info));
            }
        } else if (op == TRUNK_MEMBER_OP_ADD) {
            if (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_add(unit, hgtid, add_info,
                            member));
            }
        } else if (op == TRUNK_MEMBER_OP_DELETE) {
            if (add_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_delete(unit, hgtid, add_info,
                            member));
            }
        } else {
            return BCM_E_PARAM;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Update Higig trunk bitmap */

    SOC_IF_ERROR_RETURN
        (READ_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, hgtid, &hg_trunk_bitmap_entry));
    SOC_PBMP_CLEAR(old_trunk_pbmp);
    soc_mem_pbmp_field_get(unit, HG_TRUNK_BITMAPm, &hg_trunk_bitmap_entry,
            HIGIG_TRUNK_BITMAPf, &old_trunk_pbmp);

    soc_mem_pbmp_field_set(unit, HG_TRUNK_BITMAPm, &hg_trunk_bitmap_entry,
            HIGIG_TRUNK_BITMAPf, &new_trunk_pbmp);
    SOC_IF_ERROR_RETURN
        (WRITE_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ALL, hgtid, &hg_trunk_bitmap_entry));

    BCM_IF_ERROR_RETURN
        (_bcm_trident_trunk_fabric_port_set(unit,
                                            hgtid,
                                            old_trunk_pbmp,
                                            new_trunk_pbmp));

    if (soc_feature(unit, soc_feature_hg_trunk_failover)) {
        BCM_IF_ERROR_RETURN
            (_bcm_trident_trunk_failover_set(unit, hgtid, TRUE, add_info));
    }

    BCM_IF_ERROR_RETURN
        (_bcm_trident_trunk_swfailover_fabric_set(unit,
                                                  hgtid,
                                                  t_info->rtag,
                                                  new_trunk_pbmp));

    if (MEMBER_INFO(unit, tid).modport) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
    }
    if (MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).member_flags);
        MEMBER_INFO(unit, tid).member_flags = NULL;
    }

    MEMBER_INFO(unit, tid).modport = sal_alloc(
            sizeof(uint16) * add_info->num_ports, "member info modport");
    if (NULL == MEMBER_INFO(unit, tid).modport) {
        return BCM_E_MEMORY;
    }

    MEMBER_INFO(unit, tid).member_flags = sal_alloc(
            sizeof(uint32) * add_info->num_ports, "member info flags");
    if (NULL == MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
        return BCM_E_MEMORY;
    }

    for (i = 0; i < add_info->num_ports; i++) {
        /* Save new trunk member info */
        MEMBER_INFO(unit, tid).modport[i] = add_info->tp[i];
        MEMBER_INFO(unit, tid).member_flags[i] = add_info->member_flags[i];
    }
    MEMBER_INFO(unit, tid).num_ports = add_info->num_ports;

    /* On Trident, changing Higig trunk membership requires
     * updating the modport map table.
     */ 
    rv = bcm_esw_switch_control_get(unit,
            bcmSwitchFabricTrunkAutoIncludeDisable, &auto_include_disable);
    if (BCM_FAILURE(rv)) {
        if (BCM_E_UNAVAIL == rv) {
            auto_include_disable = 0;
        } else {
            return rv;
        }
    }
    if (!auto_include_disable) {
        BCM_IF_ERROR_RETURN(bcm_td_stk_modport_map_update(unit));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trident_trunk_fabric_destroy
 * Purpose:
 *      Remove fabric trunk group.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      hgtid    - (IN) Higig trunk group ID.
 *      t_info   - (IN) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_fabric_destroy(int unit,
        bcm_trunk_t hgtid,
        trunk_private_t *t_info)
{
    bcm_trunk_chip_info_t chip_info;
    pbmp_t old_trunk_pbmp, new_trunk_pbmp;
    hg_trunk_bitmap_entry_t hg_trunk_bitmap_entry;
    bcm_trunk_t tid;
    int auto_include_disable = 0;
    int rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    tid = hgtid + chip_info.trunk_group_count;

    /* Clear DLB configuration */
    if (soc_feature(unit, soc_feature_hg_dlb)) {
        BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_free_resource(unit, hgtid));
    }

#ifdef BCM_TRIDENT2_SUPPORT
    /* Clear resilient hashing configuration */
    if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
        BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_free_resource(unit, hgtid));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    BCM_IF_ERROR_RETURN(_bcm_trident_hg_slb_free_resource(unit, hgtid));

    if (MEMBER_INFO(unit, tid).modport) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
    }
    if (MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).member_flags);
        MEMBER_INFO(unit, tid).member_flags = NULL;
    }
    MEMBER_INFO(unit, tid).num_ports = 0;

    /* Get old Higig trunk bitmap */
    SOC_IF_ERROR_RETURN
        (READ_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, hgtid, &hg_trunk_bitmap_entry));
    SOC_PBMP_CLEAR(old_trunk_pbmp);
    soc_mem_pbmp_field_get(unit, HG_TRUNK_BITMAPm, &hg_trunk_bitmap_entry,
            HIGIG_TRUNK_BITMAPf, &old_trunk_pbmp);

    /* Clear Higig trunk bitmap */
    SOC_IF_ERROR_RETURN
        (WRITE_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ALL, hgtid,
                                soc_mem_entry_null(unit, HG_TRUNK_BITMAPm)));

    /* Clear Higig trunk group table */
    SOC_IF_ERROR_RETURN
        (WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid,
                               soc_mem_entry_null(unit, HG_TRUNK_GROUPm)));

    SOC_PBMP_CLEAR(new_trunk_pbmp);

    BCM_IF_ERROR_RETURN
        (_bcm_trident_trunk_fabric_port_set(unit,
                                            hgtid,
                                            old_trunk_pbmp,
                                            new_trunk_pbmp));

    if (soc_feature(unit, soc_feature_hg_trunk_failover)) {
        BCM_IF_ERROR_RETURN
            (_bcm_trident_trunk_failover_set(unit, hgtid, TRUE, NULL));
    }

    BCM_IF_ERROR_RETURN
        (_bcm_trident_trunk_swfailover_fabric_set(unit,
                                                  hgtid,
                                                  0,
                                                  new_trunk_pbmp));

    /* On Trident, changing Higig trunk membership requires
     * updating the modport map table.
     */ 
    rv = bcm_esw_switch_control_get(unit,
            bcmSwitchFabricTrunkAutoIncludeDisable, &auto_include_disable);
    if (BCM_FAILURE(rv)) {
        if (BCM_E_UNAVAIL == rv) {
            auto_include_disable = 0;
        } else {
            return rv;
        }
    }
    if (!auto_include_disable) {
        BCM_IF_ERROR_RETURN(bcm_td_stk_modport_map_update(unit));
    }

    t_info->in_use = FALSE;
    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_trident_trunk_fabric_get
 * Purpose:
 *      Return a fabric trunk information of given fabric trunk ID.
 *      If t_data->num_ports = 0, return number of trunk members in
 *      t_data->num_ports.
 * Parameters:
 *      unit   - (IN) StrataSwitch PCI device unit number (driver internal).
 *      tid    - (IN) Trunk ID.
 *      t_data - (OUT) Place to store returned trunk add info.
 *      t_info - (IN) Trunk info.
 * Returns:
 *      BCM_E_NONE              Success.
 *      BCM_E_XXX
 */
STATIC int
_bcm_trident_trunk_fabric_get(int unit,
        bcm_trunk_t hgtid,
        _esw_trunk_add_info_t *t_data,
        trunk_private_t *t_info)
{
    bcm_trunk_chip_info_t chip_info;
    bcm_trunk_t tid;
    bcm_module_t modid;
    bcm_port_t port;
    int i;
    uint32 quantize_control_reg;
    int profile_ptr;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    tid = hgtid + chip_info.trunk_group_count;

    if (t_data->num_ports == 0) {
        t_data->num_ports = MEMBER_INFO(unit, tid).num_ports;
    } else {
        if (MEMBER_INFO(unit, tid).num_ports <= t_data->num_ports) {
            t_data->num_ports = MEMBER_INFO(unit, tid).num_ports;
        } 

        if (BCM_FAILURE(bcm_esw_stk_my_modid_get(unit, &modid))) {
            modid = 0;
        }

        for (i = 0; i < t_data->num_ports; i++) {
            port = MEMBER_INFO(unit, tid).modport[i] & 0xff;
            BCM_IF_ERROR_RETURN(_bcm_esw_stk_modmap_map(unit,
                        BCM_STK_MODMAP_GET, modid, port,
                        &t_data->tm[i], &t_data->tp[i]));

            /* Get per trunk member DLB attributes */
            t_data->dynamic_scaling_factor[i] = 0;
            t_data->dynamic_load_weight[i] = 0;
            if ((t_data->psc == BCM_TRUNK_PSC_DYNAMIC) ||
                (t_data->psc == BCM_TRUNK_PSC_DYNAMIC_ASSIGNED) ||
                (t_data->psc == BCM_TRUNK_PSC_DYNAMIC_OPTIMAL)) {
#ifdef BCM_TRIUMPH3_SUPPORT 
                if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
                    BCM_IF_ERROR_RETURN(_bcm_tr3_hg_dlb_member_attr_get(unit,
                                port,
                                &t_data->dynamic_scaling_factor[i],
                                &t_data->dynamic_load_weight[i]));
                } else
#endif /* BCM_TRIUMPH3_SUPPORT */
                {
                    SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUANTIZE_CONTROLr(unit,
                                port, &quantize_control_reg));
                    t_data->dynamic_scaling_factor[i] = soc_reg_field_get(unit,
                            DLB_HGT_QUANTIZE_CONTROLr, quantize_control_reg,
                            PORT_LOADING_THRESHOLD_SCALING_FACTORf);
                    profile_ptr = soc_reg_field_get(unit, 
                            DLB_HGT_QUANTIZE_CONTROLr, quantize_control_reg,
                            PORT_QUALITY_MAPPING_PROFILE_PTRf);
                    t_data->dynamic_load_weight[i] =
                        HG_DLB_INFO(unit)->hg_dlb_load_weight[profile_ptr];
                }
            } 
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_fabric_find
 * Description:
 *      Get fabric trunk id that contains the given Higig port
 * Parameters:
 *      unit    - Unit number
 *      port    - Port number
 *      tid     - (OUT) Fabric trunk id
 * Returns:
 *      BCM_E_xxxx
 */
int
_bcm_trident_trunk_fabric_find(int unit,
        bcm_port_t port,
        int *tid)
{
    bcm_trunk_chip_info_t chip_info;
    int                   num_hg_trunk_groups;
    int                   hgtid;
    hg_trunk_bitmap_entry_t hg_trunk_bitmap_entry;
    pbmp_t                trunk_pbmp;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (chip_info.trunk_fabric_id_min == -1) {
        return BCM_E_NOT_FOUND;
    } else {
        num_hg_trunk_groups = chip_info.trunk_fabric_id_max - 
            chip_info.trunk_fabric_id_min + 1;
    }

    for (hgtid = 0; hgtid < num_hg_trunk_groups; hgtid++) {
        SOC_IF_ERROR_RETURN
            (READ_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, hgtid, &hg_trunk_bitmap_entry));
        SOC_PBMP_CLEAR(trunk_pbmp);
        soc_mem_pbmp_field_get(unit, HG_TRUNK_BITMAPm, &hg_trunk_bitmap_entry,
                HIGIG_TRUNK_BITMAPf, &trunk_pbmp);
        if (SOC_PBMP_MEMBER(trunk_pbmp, port)) {
            *tid = hgtid + chip_info.trunk_fabric_id_min;
            return BCM_E_NONE;
        }
    }
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_trident_trunk_override_mcast_set
 * Purpose:
 *      Set Higig trunk override in L2MC table.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      hgtid  - (IN) Higig trunk group ID.
 *      idx    - (IN) L2MC table index.
 *      enable - (IN) Higig trunk override enable.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_override_mcast_set(int unit,
        bcm_trunk_t hgtid,
        int idx,
        int enable)
{
    l2mc_entry_t l2mc_entry;
    ing_higig_trunk_override_profile_entry_t hgo_profile_entry;

    int    rv;
    uint32 old_profile_ptr, new_profile_ptr;
    uint32 hgo_bit;
    uint32 hgo_bitmap;
    void   *entry_ptr;

    /* Get old profile entry */

    soc_mem_lock(unit, L2MCm);
    rv = READ_L2MCm(unit, MEM_BLOCK_ANY, idx, &l2mc_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, L2MCm);
        return rv;
    }

    old_profile_ptr = soc_L2MCm_field32_get(unit, &l2mc_entry,
            HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);
    rv = READ_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm(unit, MEM_BLOCK_ANY,
            old_profile_ptr, &hgo_profile_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, L2MCm);
        return rv;
    }

    /* Add new profile entry */ 
    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        hgo_bitmap = soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf);
    }
    hgo_bit = 1 << hgtid;
    if (enable) {
        hgo_bitmap |= hgo_bit;
    } else {
        hgo_bitmap &= ~hgo_bit;
    }
    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_set(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_set(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, hgo_bitmap);
    }
    entry_ptr = &hgo_profile_entry;
    rv = soc_profile_mem_add(unit, _trident_hg_trunk_override_profile[unit], 
            &entry_ptr, 1, &new_profile_ptr);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, L2MCm);
        return rv;
    }

    soc_L2MCm_field32_set(unit, &l2mc_entry, HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf,
            new_profile_ptr);
    rv = WRITE_L2MCm(unit, MEM_BLOCK_ALL, idx, &l2mc_entry);
    soc_mem_unlock(unit, L2MCm);
    if (SOC_FAILURE(rv)) {
        return rv;
    }

    /* Delete old profile entry */
    rv = soc_profile_mem_delete(unit, _trident_hg_trunk_override_profile[unit], 
            old_profile_ptr);

    return rv;
}

/*
 * Function:
 *      _bcm_trident_trunk_override_mcast_get
 * Purpose:
 *      Get Higig trunk override for L2MC group.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      hgtid  - (IN) Higig trunk group ID.
 *      idx    - (IN) L2MC index.
 *      enable - (OUT) Pointer to Higig trunk override enable.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_override_mcast_get(int unit,
        bcm_trunk_t hgtid,
        int idx,
        int *enable)
{
    l2mc_entry_t l2mc_entry;
    ing_higig_trunk_override_profile_entry_t hgo_profile_entry;

    int    profile_ptr;
    uint32 hgo_bit;
    uint32 hgo_bitmap;

    SOC_IF_ERROR_RETURN
        (READ_L2MCm(unit, MEM_BLOCK_ANY, idx, &l2mc_entry));

    profile_ptr = soc_L2MCm_field32_get(unit, &l2mc_entry,
            HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);

    SOC_IF_ERROR_RETURN
        (READ_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm(unit, MEM_BLOCK_ANY,
                                                profile_ptr, &hgo_profile_entry));
    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        hgo_bitmap = soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf);
    }

    hgo_bit = 1 << hgtid;
    *enable = (hgo_bitmap & hgo_bit) ? 1 : 0;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_override_ipmc_set
 * Purpose:
 *      Set Higig trunk override for IPMC group.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      hgtid  - (IN) Higig trunk group ID.
 *      idx    - (IN) IPMC index.
 *      enable - (IN) Higig trunk override enable.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_override_ipmc_set(int unit,
        bcm_trunk_t hgtid,
        int idx,
        int enable)
{
    ipmc_entry_t ipmc_entry;
    ing_higig_trunk_override_profile_entry_t hgo_profile_entry;

    int    rv;
    uint32 old_profile_ptr, new_profile_ptr;
    uint32 hgo_bit;
    uint32 hgo_bitmap;
    void   *entry_ptr;
    int    mc_base = 0, mc_size, mc_index;

    /* Get old profile entry */

    soc_mem_lock(unit, L3_IPMCm);
    rv = READ_L3_IPMCm(unit, MEM_BLOCK_ANY, idx, &ipmc_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, L3_IPMCm);
        return rv;
    }

    old_profile_ptr = soc_L3_IPMCm_field32_get(unit, &ipmc_entry,
            HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);
    rv = READ_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm(unit, MEM_BLOCK_ANY,
            old_profile_ptr, &hgo_profile_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, L3_IPMCm);
        return rv;
    }

    /* Add new profile entry */ 
    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        hgo_bitmap = soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf);
    }
    hgo_bit = 1 << hgtid;
    if (enable) {
        hgo_bitmap |= hgo_bit;
    } else {
        hgo_bitmap &= ~hgo_bit;
    }

    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_set(unit, 
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_set(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, hgo_bitmap);
    }

    entry_ptr = &hgo_profile_entry;
    rv = soc_profile_mem_add(unit, _trident_hg_trunk_override_profile[unit], 
            &entry_ptr, 1, &new_profile_ptr);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, L3_IPMCm);
        return rv;
    }

    soc_L3_IPMCm_field32_set(unit, &ipmc_entry, HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf,
            new_profile_ptr);
    rv = WRITE_L3_IPMCm(unit, MEM_BLOCK_ALL, idx, &ipmc_entry);
    soc_mem_unlock(unit, L3_IPMCm);
    if (SOC_FAILURE(rv)) {
        return rv;
    }

    /* Delete old profile entry */
    SOC_IF_ERROR_RETURN
        (soc_profile_mem_delete(unit, _trident_hg_trunk_override_profile[unit], 
                                old_profile_ptr));

    /* In Trident, for IPMC packets from Higig ports, the L2 port bitmap is 
     * derived from L2MC table. Hence, the Higig trunk override in L2MC table
     * also needs to be updated.
     */
    if (SOC_REG_FIELD_VALID(unit, MC_CONTROL_5r, SHARED_TABLE_L2MC_SIZEf) &&
        SOC_REG_FIELD_VALID(unit, MC_CONTROL_5r, SHARED_TABLE_IPMC_SIZEf)) { 
        SOC_IF_ERROR_RETURN(soc_hbx_ipmc_size_get(unit, &mc_base, &mc_size));
        if (idx >= mc_size) {
            return BCM_E_PARAM;
        }
        mc_index = idx + mc_base;
        SOC_IF_ERROR_RETURN
            (_bcm_trident_trunk_override_mcast_set(unit, hgtid, mc_index, enable)); 
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_override_ipmc_get
 * Purpose:
 *      Get Higig trunk override for IPMC group.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      hgtid  - (IN) Higig trunk group ID.
 *      idx    - (IN) IPMC index.
 *      enable - (OUT) Pointer to Higig trunk override enable.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_override_ipmc_get(int unit,
        bcm_trunk_t hgtid,
        int idx,
        int *enable)
{
    ipmc_entry_t ipmc_entry;
    ing_higig_trunk_override_profile_entry_t hgo_profile_entry;

    int    profile_ptr;
    uint32 hgo_bit;
    uint32 hgo_bitmap;

    SOC_IF_ERROR_RETURN
        (READ_L3_IPMCm(unit, MEM_BLOCK_ANY, idx, &ipmc_entry));

    profile_ptr = soc_L3_IPMCm_field32_get(unit, &ipmc_entry,
            HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);

    SOC_IF_ERROR_RETURN
        (READ_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm(unit, MEM_BLOCK_ANY,
                                                profile_ptr, &hgo_profile_entry));

    if (SOC_IS_TD2_TT2(unit)) {            
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        hgo_bitmap = soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf);
    }
    hgo_bit = 1 << hgtid;
    *enable = (hgo_bitmap & hgo_bit) ? 1 : 0;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_override_vlan_set
 * Purpose:
 *      Set Higig trunk override in VLAN table.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      hgtid  - (IN) Higig trunk group ID.
 *      idx    - (IN) VLAN table index.
 *      enable - (IN) Higig trunk override enable.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_override_vlan_set(int unit,
        bcm_trunk_t hgtid,
        int idx,
        int enable)
{
    vlan_tab_entry_t vlan_tab_entry;
    ing_higig_trunk_override_profile_entry_t hgo_profile_entry;

    int    rv;
    uint32 old_profile_ptr, new_profile_ptr;
    uint32 hgo_bit;
    uint32 hgo_bitmap;
    void   *entry_ptr;

    /* Get old profile entry */

    soc_mem_lock(unit, VLAN_TABm);
    rv = READ_VLAN_TABm(unit, MEM_BLOCK_ANY, idx, &vlan_tab_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    old_profile_ptr = soc_VLAN_TABm_field32_get(unit, &vlan_tab_entry,
            HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);
    rv = READ_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm(unit, MEM_BLOCK_ANY,
            old_profile_ptr, &hgo_profile_entry);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    /* Add new profile entry */ 
    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        hgo_bitmap = soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf);
    }
    hgo_bit = 1 << hgtid;
    if (enable) {
        hgo_bitmap |= hgo_bit;
    } else {
        hgo_bitmap &= ~hgo_bit;
    }

    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_set(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_set(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, hgo_bitmap);
    }
    entry_ptr = &hgo_profile_entry;
    rv = soc_profile_mem_add(unit, _trident_hg_trunk_override_profile[unit], 
            &entry_ptr, 1, &new_profile_ptr);
    if (SOC_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    soc_VLAN_TABm_field32_set(unit, &vlan_tab_entry, HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf,
            new_profile_ptr);
    rv = WRITE_VLAN_TABm(unit, MEM_BLOCK_ALL, idx, &vlan_tab_entry);
    soc_mem_unlock(unit, VLAN_TABm);
    if (SOC_FAILURE(rv)) {
        return rv;
    }

    /* Delete old profile entry */
    rv = soc_profile_mem_delete(unit, _trident_hg_trunk_override_profile[unit], 
            old_profile_ptr);

    return rv;
}

/*
 * Function:
 *      _bcm_trident_trunk_override_vlan_get
 * Purpose:
 *      Get Higig trunk override for VLAN table.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      hgtid  - (IN) Higig trunk group ID.
 *      idx    - (IN) VLAN table index.
 *      enable - (OUT) Pointer to Higig trunk override enable.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_trident_trunk_override_vlan_get(int unit,
        bcm_trunk_t hgtid,
        int idx,
        int *enable)
{
    vlan_tab_entry_t vlan_tab_entry;
    ing_higig_trunk_override_profile_entry_t hgo_profile_entry;

    int    profile_ptr;
    uint32 hgo_bit;
    uint32 hgo_bitmap;

    SOC_IF_ERROR_RETURN
        (READ_VLAN_TABm(unit, MEM_BLOCK_ANY, idx, &vlan_tab_entry));

    profile_ptr = soc_VLAN_TABm_field32_get(unit, &vlan_tab_entry,
            HIGIG_TRUNK_OVERRIDE_PROFILE_PTRf);

    SOC_IF_ERROR_RETURN
        (READ_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm(unit, MEM_BLOCK_ANY,
                                                profile_ptr, &hgo_profile_entry));

    if (SOC_IS_TD2_TT2(unit)) {
        soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf, &hgo_bitmap);
    } else {
        hgo_bitmap = soc_ING_HIGIG_TRUNK_OVERRIDE_PROFILEm_field32_get(unit,
            &hgo_profile_entry, HIGIG_TRUNK_OVERRIDE_BITMAPf);
    }

    hgo_bit = 1 << hgtid;
    *enable = (hgo_bitmap & hgo_bit) ? 1 : 0;

    return BCM_E_NONE;
}

/* ----------------------------------------------------------------------------- 
 *
 * Routines shared by front panel and Higig trunking 
 *
 * ----------------------------------------------------------------------------- 
 */

/*
 * Function:
 *      bcm_trident_trunk_destroy
 * Purpose:
 *      Remove trunk group.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      t_info   - (IN) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_trident_trunk_destroy(int unit,
        bcm_trunk_t tid,
        trunk_private_t *t_info)
{
    bcm_trunk_chip_info_t chip_info;
    int                   hgtid;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (chip_info.trunk_fabric_id_min >= 0 &&
        tid >= chip_info.trunk_fabric_id_min) {        /* fabric trunk */
        hgtid = tid - chip_info.trunk_fabric_id_min;
        return _bcm_trident_trunk_fabric_destroy(unit, hgtid, t_info);
    }

    /* Front panel trunks */
    return _bcm_trident_trunk_fp_destroy(unit, tid, t_info);
}

/*
 * Function:
 *      _trident_trunk_add_info_member_alloc
 * Purpose:
 *      Allocate member_flags, tp, and tm of _esw_trunk_add_info_t.
 * Parameters:
 *      add_info  - (IN) Pointer to trident trunk add info structure.
 *      num_ports - (IN) Number of member ports.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_trident_trunk_add_info_member_alloc(_esw_trunk_add_info_t *add_info,
        int num_ports)
{
    int rv = BCM_E_NONE;

    add_info->member_flags = sal_alloc(sizeof(uint32) * num_ports,
            "_trident_trunk_add_info_member_flags");
    if (add_info->member_flags == NULL) {
        rv = BCM_E_MEMORY;
        goto error;
    }
    sal_memset(add_info->member_flags, 0, sizeof(uint32) * num_ports);

    add_info->tp = sal_alloc(sizeof(bcm_port_t) * num_ports,
            "_trident_trunk_add_info_tp");
    if (add_info->tp == NULL) {
        rv = BCM_E_MEMORY;
        goto error;
    }
    sal_memset(add_info->tp, 0, sizeof(bcm_port_t) * num_ports);

    add_info->tm = sal_alloc(sizeof(bcm_module_t) * num_ports,
            "_trident_trunk_add_info_tm");
    if (add_info->tm == NULL) {
        rv = BCM_E_MEMORY;
        goto error;
    }
    sal_memset(add_info->tm, 0, sizeof(bcm_module_t) * num_ports);

    add_info->dynamic_scaling_factor = sal_alloc(sizeof(int) * num_ports,
            "_trident_trunk_add_info_dynamic_scaling_factor");
    if (add_info->dynamic_scaling_factor == NULL) {
        rv = BCM_E_MEMORY;
        goto error;
    }
    sal_memset(add_info->dynamic_scaling_factor, 0, sizeof(int) * num_ports);

    add_info->dynamic_load_weight = sal_alloc(sizeof(int) * num_ports,
            "_trident_trunk_add_info_dynamic_load_weight");
    if (add_info->dynamic_load_weight == NULL) {
        rv = BCM_E_MEMORY;
        goto error;
    }
    sal_memset(add_info->dynamic_load_weight, 0, sizeof(int) * num_ports);

    return rv;

error:
    if (NULL != add_info->member_flags) {
        sal_free(add_info->member_flags);
    }
    if (NULL != add_info->tp) {
        sal_free(add_info->tp);
    }
    if (NULL != add_info->tm) {
        sal_free(add_info->tm);
    }
    if (NULL != add_info->dynamic_scaling_factor) {
        sal_free(add_info->dynamic_scaling_factor);
    }
    if (NULL != add_info->dynamic_load_weight) {
        sal_free(add_info->dynamic_load_weight);
    }

    return rv;
}

/*
 * Function:
 *      _trident_trunk_add_info_member_free
 * Purpose:
 *      Free member_flags, tp, and tm of _esw_trunk_add_info_t.
 * Parameters:
 *      add_info  - (IN) Pointer to trident trunk add info structure.
 * Returns:
 *      BCM_E_xxx
 */
STATIC void 
_trident_trunk_add_info_member_free(_esw_trunk_add_info_t *add_info)
{
    if (NULL != add_info->member_flags) {
        sal_free(add_info->member_flags);
    }
    if (NULL != add_info->tp) {
        sal_free(add_info->tp);
    }
    if (NULL != add_info->tm) {
        sal_free(add_info->tm);
    }
    if (NULL != add_info->dynamic_scaling_factor) {
        sal_free(add_info->dynamic_scaling_factor);
    }
    if (NULL != add_info->dynamic_load_weight) {
        sal_free(add_info->dynamic_load_weight);
    }
}

/*
 * Function:
 *      _bcm_trident_trunk_modify
 * Purpose:
 *      Set, add or delete trunk members.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      add_info - (IN) Pointer to trident trunk add info structure.
 *      t_info   - (IN) Pointer to trunk info.
 *      op       - (IN) Type of operation: TRUNK_MEMBER_OP_SET, ADD or DELETE.
 *      member   - (IN) Member to add or delete.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_modify(int unit,
        bcm_trunk_t tid,
        _esw_trunk_add_info_t *add_info,
        trunk_private_t *t_info,
        int op,
        bcm_trunk_member_t *member)
{
    bcm_trunk_chip_info_t chip_info;
    int                   hgtid;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));

    if (chip_info.trunk_fabric_id_min >= 0 &&
            tid >= chip_info.trunk_fabric_id_min) {
        /* Higig trunks */
        hgtid = tid - chip_info.trunk_fabric_id_min;
        return _bcm_trident_trunk_fabric_modify(unit, hgtid, add_info, t_info,
                op, member);
    } 

    /* Front panel trunks */
    return _bcm_trident_trunk_fp_modify(unit, tid, add_info, t_info,
            op, member);
}

/*
 * Function:
 *      _bcm_trident_hg_dlb_status_get
 * Purpose:
 *      Check if dynamic load balancing is enabled on any Higig trunk group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      dlb_status - (OUT) Higig DLB status
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_dlb_status_get(int unit, int *dlb_status)
{
    int rv = BCM_E_NONE;
    bcm_trunk_chip_info_t chip_info;
    int i;
    int psc;

    *dlb_status = FALSE;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (chip_info.trunk_fabric_id_min < 0) {
        /* There are no Higig trunk groups. */
        return BCM_E_NOT_FOUND;
    }

    for (i = chip_info.trunk_fabric_id_min;
            i <= chip_info.trunk_fabric_id_max; i++) {
        rv = bcm_esw_trunk_psc_get(unit, i, &psc);
        if (rv == BCM_E_NOT_FOUND) {
            continue;
        }
        if ((psc == BCM_TRUNK_PSC_DYNAMIC) ||
            (psc == BCM_TRUNK_PSC_DYNAMIC_ASSIGNED) ||
            (psc == BCM_TRUNK_PSC_DYNAMIC_OPTIMAL)) {
            *dlb_status = TRUE;
            break;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_clear
 * Purpose:
 *      Clear existing trunk group members if any.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) Trunk group ID.
 *      t_info   - (IN) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_trunk_clear(int unit,
        bcm_trunk_t tid,
        trunk_private_t *t_info)
{
    bcm_trunk_chip_info_t chip_info;
    int higig_trunk;
#ifdef BCM_TRIDENT2_SUPPORT
    int hgtid;
#endif

    if (t_info->in_use) {
        BCM_IF_ERROR_RETURN(bcm_trident_trunk_destroy(unit, tid, t_info));
    }

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (chip_info.trunk_fabric_id_min >= 0 &&
            tid >= chip_info.trunk_fabric_id_min) {
        higig_trunk = 1;
#ifdef BCM_TRIDENT2_SUPPORT
        hgtid = tid - chip_info.trunk_fabric_id_min;
#endif
    } else {
        higig_trunk = 0;
    }

    if (higig_trunk) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
            if (t_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                /* Write the dynamic_size to hardware, so it can be recovered
                 * during warm boot.
                 */
                BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_dynamic_size_set(unit, hgtid,
                            t_info->dynamic_size));
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
    } else {
#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
            if (t_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
                /* Write the dynamic_size to hardware, so it can be recovered
                 * during warm boot.
                 */
                BCM_IF_ERROR_RETURN(bcm_td2_lag_rh_dynamic_size_set(unit, tid,
                            t_info->dynamic_size));
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trident_trunk_modify
 * Purpose:
 *      Set, add or delete trunk members.
 * Parameters:
 *      unit       - (IN) SOC unit number. 
 *      tid        - (IN) Trunk group ID.
 *      trunk_info - (IN) Pointer to user provided trunk info.
 *      member_count - (IN) Number of trunk members.
 *      member_array - (IN) Array of trunk members.
 *      t_info       - (IN) Pointer to internal trunk info.
 *      op           - (IN) Type of operation: TRUNK_MEMBER_OP_SET, ADD or DELETE.
 *      member       - (IN) Member to add or delete.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_trident_trunk_modify(int unit,
        bcm_trunk_t tid,
        bcm_trunk_info_t *trunk_info,
        int member_count,
        bcm_trunk_member_t *member_array,
        trunk_private_t *t_info,
        int op,
        bcm_trunk_member_t *member)
{
    int dlb_enable=0, rh_enable=0;
    bcm_trunk_chip_info_t chip_info;
    int rv = BCM_E_NONE;
    int dynamic_size_encode;
    int dlb_status;
    _esw_trunk_add_info_t add_info;
    int i;
    int higig_trunk;

    if (trunk_info->psc <= 0) {
        trunk_info->psc = BCM_TRUNK_PSC_DEFAULT;
    }

    if ((trunk_info->psc == BCM_TRUNK_PSC_DYNAMIC) ||
        (trunk_info->psc == BCM_TRUNK_PSC_DYNAMIC_ASSIGNED) ||
        (trunk_info->psc == BCM_TRUNK_PSC_DYNAMIC_OPTIMAL)) {
        dlb_enable = 1;
    } else if (trunk_info->psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
        rh_enable = 1;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_trident_trunk_psc_to_rtag(unit, trunk_info->psc, &(t_info->rtag)));
    t_info->psc = trunk_info->psc;

    t_info->flags = trunk_info->flags;

    if ((trunk_info->dlf_index != trunk_info->mc_index) ||
        (trunk_info->dlf_index != trunk_info->ipmc_index)) {
        return BCM_E_PARAM;
    }
    t_info->dlf_index_spec =
        t_info->dlf_index_used =
        t_info->mc_index_spec = 
        t_info->mc_index_used =
        t_info->ipmc_index_spec =
        t_info->ipmc_index_used = trunk_info->dlf_index;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (chip_info.trunk_fabric_id_min >= 0 &&
            tid >= chip_info.trunk_fabric_id_min) {
        higig_trunk = 1;
    } else {
        higig_trunk = 0;
    }

    if (!higig_trunk) {
        /* Front panel trunks */

        if (dlb_enable) {
#ifdef BCM_TRIUMPH3_SUPPORT
            if (soc_feature(unit, soc_feature_lag_dlb)) {
                /* Verify dynamic_size */
                BCM_IF_ERROR_RETURN(bcm_tr3_lag_dlb_dynamic_size_encode(
                            trunk_info->dynamic_size, &dynamic_size_encode));
                t_info->dynamic_size = trunk_info->dynamic_size;

                /* Verify dynamic_age */
                if (trunk_info->dynamic_age > 0xffff) {
                    return BCM_E_PARAM;
                }
                t_info->dynamic_age = trunk_info->dynamic_age;

                /* Verify dynamic exponents */
                if (trunk_info->dynamic_load_exponent > 0xf) {
                    return BCM_E_PARAM;
                }
                t_info->dynamic_load_exponent =
                    trunk_info->dynamic_load_exponent;

                if (trunk_info->dynamic_expected_load_exponent > 0xf) {
                    return BCM_E_PARAM;
                }
                t_info->dynamic_expected_load_exponent =
                    trunk_info->dynamic_expected_load_exponent;

            } else
#endif /* BCM_TRIUMPH3_SUPPORT */
            {
                /* Dynamic load balancing not supported for LAGs */
                return BCM_E_PARAM;
            }
        } else if (rh_enable) {
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
                /* Verify dynamic_size */
                BCM_IF_ERROR_RETURN(bcm_td2_lag_rh_dynamic_size_encode(
                            trunk_info->dynamic_size, &dynamic_size_encode));
                t_info->dynamic_size = trunk_info->dynamic_size;
            } else 
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                /* Resilient hashing not supported for LAGs */
                return BCM_E_PARAM;
            }
        }

    } else {
        /* Higig trunk */

        if (dlb_enable) {
            if (soc_feature(unit, soc_feature_hg_dlb)) {
                /* Verify dynamic_size */
                BCM_IF_ERROR_RETURN(_bcm_trident_hg_dlb_dynamic_size_encode(
                            trunk_info->dynamic_size, &dynamic_size_encode));
                t_info->dynamic_size = trunk_info->dynamic_size;

                /* Verify dynamic_age */
                if ((trunk_info->dynamic_age < 16) ||
                        (trunk_info->dynamic_age > 0x7fff)) {
                    return BCM_E_PARAM;
                }
                t_info->dynamic_age = trunk_info->dynamic_age;
            } else {
                /* Dynamic load balancing not supported for Higig trunks */
                return BCM_E_PARAM;
            }

#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
                int rh_status;

                BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_status_get(unit, &rh_status));
                if (rh_status) {
                    /* Higig DLB and resilient hashing cannot be enabled
                     * at the same time.
                     */
                    return BCM_E_PARAM;
                }
            }

            if (SOC_REG_IS_VALID(unit, ENHANCED_HASHING_CONTROLr)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                            ENHANCED_HASHING_CONTROLr, REG_PORT_ANY,
                            RH_HGT_ENABLEf, 0));
            }
#endif /* BCM_TRIDENT2_SUPPORT */

        } else if (rh_enable) {

#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
                /* Verify dynamic_size */
                BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_dynamic_size_encode(
                            trunk_info->dynamic_size, &dynamic_size_encode));
                t_info->dynamic_size = trunk_info->dynamic_size;
            } else 
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                /* Resilient hashing not supported for Higig trunks */
                return BCM_E_PARAM;
            }

            if (soc_feature(unit, soc_feature_hg_dlb)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_trident_hg_dlb_status_get(unit, &dlb_status));
                if (dlb_status) {
                    /* DLB and resilient hashing cannot be enabled
                     * at the same time.
                     */
                    return BCM_E_PARAM;
                }
            }

#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_REG_IS_VALID(unit, ENHANCED_HASHING_CONTROLr)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                            ENHANCED_HASHING_CONTROLr, REG_PORT_ANY,
                            RH_HGT_ENABLEf, 1));
            }
#endif /* BCM_TRIDENT2_SUPPORT */
        }
    }

    if (member_count < 1) {
        /* Member count is less than 1 when the operation is
         * TRUNK_MEMBER_OP_SET with zero members or TRUNK_MEMBER_OP_DELETE
         * on the final member of the group. In either case,
         * clear the existing trunk member ports if any.
         */
        BCM_IF_ERROR_RETURN(_bcm_trident_trunk_clear(unit, tid, t_info));
    } else {
        sal_memset(&add_info, 0, sizeof(_esw_trunk_add_info_t));
        add_info.flags = trunk_info->flags;
        add_info.num_ports = member_count;
        add_info.psc = trunk_info->psc;
        add_info.ipmc_psc = trunk_info->ipmc_psc;
        add_info.dlf_index = trunk_info->dlf_index;
        add_info.mc_index = trunk_info->mc_index;
        add_info.ipmc_index = trunk_info->ipmc_index;
        add_info.dynamic_size = trunk_info->dynamic_size;
        add_info.dynamic_age = trunk_info->dynamic_age;
        add_info.dynamic_load_exponent = trunk_info->dynamic_load_exponent;
        add_info.dynamic_expected_load_exponent =
            trunk_info->dynamic_expected_load_exponent;

        BCM_IF_ERROR_RETURN
            (_trident_trunk_add_info_member_alloc(&add_info, member_count));
        for (i = 0; i < member_count; i++) {
            add_info.member_flags[i] = member_array[i].flags;
            add_info.tp[i] = member_array[i].gport;
            add_info.tm[i] = -1;
            add_info.dynamic_scaling_factor[i] =
                member_array[i].dynamic_scaling_factor;
            add_info.dynamic_load_weight[i] =
                member_array[i].dynamic_load_weight;
        }

        /* Convert member gport to module and port */
        rv = _bcm_esw_trunk_gport_array_resolve(unit,
                higig_trunk,
                member_count,
                add_info.tp,
                add_info.tp,
                add_info.tm);
        if (BCM_FAILURE(rv)) {
            _trident_trunk_add_info_member_free(&add_info);
            return rv;
        }

        rv = _bcm_trident_trunk_modify(unit, tid, &add_info, t_info, op, member);
        _trident_trunk_add_info_member_free(&add_info);
    }

    return rv;
}

/*
 * Function:
 *	_bcm_trident_trunk_get
 * Purpose:
 *      Return a trunk information of given trunk ID.
 * Parameters:
 *      unit   - (IN) StrataSwitch PCI device unit number (driver internal).
 *      tid    - (IN) Trunk ID.
 *      t_data - (OUT) Place to store returned trunk add info.
 *      t_info - (IN) Trunk info.
 * Returns:
 *      BCM_E_NONE              Success.
 *      BCM_E_XXX
 */
STATIC int
_bcm_trident_trunk_get(int unit,
        bcm_trunk_t tid,
        _esw_trunk_add_info_t *t_data,
        trunk_private_t *t_info)
{
    bcm_trunk_chip_info_t chip_info;
    int hgtid;

    t_data->flags = t_info->flags;
    t_data->psc = t_info->psc;
    t_data->ipmc_psc = t_info->ipmc_psc;
    t_data->dlf_index = t_info->dlf_index_used;
    t_data->mc_index = t_info->mc_index_used;
    t_data->ipmc_index = t_info->ipmc_index_used;
    t_data->dynamic_size = t_info->dynamic_size;
    t_data->dynamic_age = t_info->dynamic_age;
    t_data->dynamic_load_exponent =
        t_info->dynamic_load_exponent;
    t_data->dynamic_expected_load_exponent =
        t_info->dynamic_expected_load_exponent;

    if (!t_info->in_use) {
        t_data->num_ports = 0;
        return BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (chip_info.trunk_fabric_id_min >= 0 &&
        tid >= chip_info.trunk_fabric_id_min) {        /* fabric trunk */
        hgtid = tid - chip_info.trunk_fabric_id_min;
        return _bcm_trident_trunk_fabric_get(unit, hgtid, t_data, t_info);
    }

    /* Front panel trunk */
    return _bcm_trident_trunk_fp_get(unit, tid, t_data, t_info);
}

/*
 * Function:
 *	bcm_trident_trunk_get
 * Purpose:
 *      Return a trunk information of given trunk ID.
 * Parameters:
 *      unit   - (IN) StrataSwitch PCI device unit number (driver internal).
 *      tid    - (IN) Trunk ID.
 *      trunk_info   - (OUT) Place to store returned trunk info.
 *      member_max   - (IN) Size of member_array.
 *      member_array - (OUT) Array of trunk member structures.
 *      member_count - (OUT) Number of trunk members returned.
 *      t_info       - (IN) Internal trunk info.
 * Returns:
 *      BCM_E_NONE              Success.
 *      BCM_E_XXX
 */
int
bcm_trident_trunk_get(int unit,
        bcm_trunk_t tid,
        bcm_trunk_info_t *trunk_info,
        int member_max, 
        bcm_trunk_member_t *member_array,
        int *member_count,
        trunk_private_t *t_info)
{
    _esw_trunk_add_info_t add_info;
    int i;
    int rv = BCM_E_NONE;
    bcm_trunk_chip_info_t chip_info;
    int higig_trunk;

    sal_memset(&add_info, 0, sizeof(_esw_trunk_add_info_t));
    add_info.num_ports = member_max;
    if (member_max > 0) {
        BCM_IF_ERROR_RETURN
            (_trident_trunk_add_info_member_alloc(&add_info, member_max));
    }

    rv = _bcm_trident_trunk_get(unit, tid, &add_info, t_info);
    if (BCM_FAILURE(rv)) {
        if (member_max > 0) {
            _trident_trunk_add_info_member_free(&add_info);
        }
        return rv;
    }

    trunk_info->flags = add_info.flags;
    trunk_info->psc = add_info.psc;
    trunk_info->ipmc_psc = add_info.ipmc_psc;
    trunk_info->dlf_index = add_info.dlf_index;
    trunk_info->mc_index = add_info.mc_index;
    trunk_info->ipmc_index = add_info.ipmc_index;
    trunk_info->dynamic_size = add_info.dynamic_size;
    trunk_info->dynamic_age = add_info.dynamic_age;
    trunk_info->dynamic_load_exponent =
        add_info.dynamic_load_exponent;
    trunk_info->dynamic_expected_load_exponent =
        add_info.dynamic_expected_load_exponent;

    *member_count = add_info.num_ports;

    if (member_max > 0) {
        if (member_max < *member_count) {
            *member_count = member_max;
        }

        /* Construct member gport */
        rv = bcm_esw_trunk_chip_info_get(unit, &chip_info);
        if (BCM_FAILURE(rv)) {
            _trident_trunk_add_info_member_free(&add_info);
            return rv;
        }
        if (chip_info.trunk_fabric_id_min >= 0 &&
            tid >= chip_info.trunk_fabric_id_min) {
            higig_trunk = 1;
        } else {
            higig_trunk = 0;
        }
        rv = _bcm_esw_trunk_gport_construct(unit, higig_trunk,
                *member_count,
                add_info.tp, add_info.tm,
                add_info.tp);
        if (BCM_FAILURE(rv)) {
            _trident_trunk_add_info_member_free(&add_info);
            return rv;
        }

        for (i = 0; i < *member_count; i++) {
            bcm_trunk_member_t_init(&member_array[i]);
            member_array[i].flags = add_info.member_flags[i];
            member_array[i].gport = add_info.tp[i];
            member_array[i].dynamic_scaling_factor =
                add_info.dynamic_scaling_factor[i];
            member_array[i].dynamic_load_weight =
                add_info.dynamic_load_weight[i];
        }

        _trident_trunk_add_info_member_free(&add_info);
    }

    return rv;
}

/*
 * Function:
 *	bcm_trident_trunk_mcast_join
 * Purpose:
 *	Add the trunk group to existing MAC multicast entry.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      tid - Trunk Id.
 *      vid - Vlan ID.
 *      mac - MAC address.
 * Returns:
 *	BCM_E_XXX
 * Notes:
 *      Applications have to remove the MAC multicast entry and re-add in with
 *      new port bitmap to remove the trunk group from MAC multicast entry.
 */
int
bcm_trident_trunk_mcast_join(int unit,
        bcm_trunk_t tid,
        bcm_vlan_t vid,
        sal_mac_addr_t mac,
        trunk_private_t *t_info)
{
    trunk_bitmap_entry_t        trunk_bitmap_entry;
    bcm_mcast_addr_t            mc_addr;

    /* get the ports in the trunk group */
    SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, TRUNK_BITMAPm, MEM_BLOCK_ANY, tid,
                      &trunk_bitmap_entry));
    soc_mem_pbmp_field_get(unit, TRUNK_BITMAPm, &trunk_bitmap_entry,
                           TRUNK_BITMAPf, &mc_addr.pbmp);
    /* Set up the multicast address*/
    sal_memcpy(mc_addr.mac, mac, sizeof(sal_mac_addr_t));
    mc_addr.vid = vid;

    /* Add the trunk group's ports to the multicast group. */
    return bcm_esw_mcast_port_add(unit, &mc_addr);
}

/*
 * Function:
 *      bcm_trident_hg_dlb_member_status_set
 * Purpose:
 *      Set Higig DLB member status.
 * Parameters:
 *      unit   - (IN) SOC unit number.
 *      port   - (IN) Member port number.
 *      status - (IN) Member status.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_trident_hg_dlb_member_status_set(int unit, bcm_port_t port,
        int status)
{
    dlb_hgt_link_control_entry_t dlb_hgt_link_control_entry;
    bcm_pbmp_t sw_port_state, override_port_map;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
        dlb_hgt_port_member_map_entry_t port_member_map_entry;
        int member_id;
        dlb_hgt_member_sw_state_entry_t sw_state_entry;
        int member_bitmap_width, alloc_size;
        SHR_BITDCL *status_bitmap = NULL;
        SHR_BITDCL *override_bitmap = NULL;

        /* Get Higig port's DLB member ID */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_PORT_MEMBER_MAPm(unit, MEM_BLOCK_ANY,
                    port, &port_member_map_entry));
        if (soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                    &port_member_map_entry, VALIDf)) {
            member_id = soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                    &port_member_map_entry, MEMBER_IDf);
        } else {
            /* The given Higig port is not assigned a DLB member ID */
            return BCM_E_NOT_FOUND;
        }

        /* Get status bitmap and override bitmap */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ANY,
                    0, &sw_state_entry));
        member_bitmap_width = soc_mem_field_length(unit,
                DLB_HGT_MEMBER_SW_STATEm, MEMBER_BITMAPf);
        alloc_size = SHR_BITALLOCSIZE(member_bitmap_width);
        status_bitmap = sal_alloc(alloc_size, "DLB HGT member status bitmap"); 
        if (NULL == status_bitmap) {
            return BCM_E_MEMORY;
        }
        soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit, &sw_state_entry,
                MEMBER_BITMAPf, status_bitmap);

        override_bitmap = sal_alloc(alloc_size, "DLB HGT member override bitmap"); 
        if (NULL == override_bitmap) {
            sal_free(status_bitmap);
            return BCM_E_MEMORY;
        }
        soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit, &sw_state_entry,
                OVERRIDE_MEMBER_BITMAPf, override_bitmap);

        /* Update status and override bitmaps */
        switch (status) {
            case BCM_TRUNK_DYNAMIC_MEMBER_FORCE_DOWN:
                SHR_BITSET(override_bitmap, member_id);
                SHR_BITCLR(status_bitmap, member_id);
                break;
            case BCM_TRUNK_DYNAMIC_MEMBER_FORCE_UP:
                SHR_BITSET(override_bitmap, member_id);
                SHR_BITSET(status_bitmap, member_id);
                break;
            case BCM_TRUNK_DYNAMIC_MEMBER_HW:
                SHR_BITCLR(override_bitmap, member_id);
                SHR_BITCLR(status_bitmap, member_id);
                break;
            default:
                sal_free(status_bitmap);
                sal_free(override_bitmap);
                return BCM_E_PARAM;
        }

        /* Write status and override bitmaps to hardware */
        soc_DLB_HGT_MEMBER_SW_STATEm_field_set(unit, &sw_state_entry,
                MEMBER_BITMAPf, status_bitmap);
        soc_DLB_HGT_MEMBER_SW_STATEm_field_set(unit, &sw_state_entry,
                OVERRIDE_MEMBER_BITMAPf, override_bitmap);
        sal_free(status_bitmap);
        sal_free(override_bitmap);
        SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ALL,
                    0, &sw_state_entry));
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        /* Get status bitmap and override bitmap */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_LINK_CONTROLm(unit, MEM_BLOCK_ANY,
                    0, &dlb_hgt_link_control_entry));
        soc_mem_pbmp_field_get(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_PORT_STATEf, &sw_port_state);
        soc_mem_pbmp_field_get(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_OVERRIDE_PORT_MAPf,
                &override_port_map);

        /* Update status and override bitmaps */
        switch (status) {
            case BCM_TRUNK_DYNAMIC_MEMBER_FORCE_DOWN:
                BCM_PBMP_PORT_ADD(override_port_map, port);
                BCM_PBMP_PORT_REMOVE(sw_port_state, port);
                break;
            case BCM_TRUNK_DYNAMIC_MEMBER_FORCE_UP:
                BCM_PBMP_PORT_ADD(override_port_map, port);
                BCM_PBMP_PORT_ADD(sw_port_state, port);
                break;
            case BCM_TRUNK_DYNAMIC_MEMBER_HW:
                BCM_PBMP_PORT_REMOVE(override_port_map, port);
                BCM_PBMP_PORT_REMOVE(sw_port_state, port);
                break;
            default:
                return BCM_E_PARAM;
        }

        /* Write status and override bitmaps to hardware */
        soc_mem_pbmp_field_set(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_PORT_STATEf, &sw_port_state);
        soc_mem_pbmp_field_set(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_OVERRIDE_PORT_MAPf,
                &override_port_map);
        SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_LINK_CONTROLm(unit, MEM_BLOCK_ALL,
                    0, &dlb_hgt_link_control_entry));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trident_hg_dlb_member_status_get
 * Purpose:
 *      Get Higig DLB member status.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      port - (IN) Higig port number.
 *      status - (OUT) Member status.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_trident_hg_dlb_member_status_get(int unit, bcm_port_t port,
        int *status)
{
    int rv = BCM_E_NONE;
    dlb_hgt_link_control_entry_t dlb_hgt_link_control_entry;
    bcm_pbmp_t sw_port_state, override_port_map, port_pbmp;
    dlb_hgt_port_state_entry_t port_state_entry;
    bcm_pbmp_t hw_port_state;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
        dlb_hgt_port_member_map_entry_t port_member_map_entry;
        int member_id;
        dlb_hgt_member_sw_state_entry_t sw_state_entry;
        int member_bitmap_width, alloc_size;
        SHR_BITDCL *status_bitmap = NULL;
        SHR_BITDCL *override_bitmap = NULL;
        uint32 hw_state_reg, hw_state_reg_bitmap;

        /* Get Higig port's DLB member ID */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_PORT_MEMBER_MAPm(unit, MEM_BLOCK_ANY,
                    port, &port_member_map_entry));
        if (soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                    &port_member_map_entry, VALIDf)) {
            member_id = soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                    &port_member_map_entry, MEMBER_IDf);
        } else {
            /* The given Higig port is not assigned a DLB member ID */
            return BCM_E_NOT_FOUND;
        }

        /* Get status bitmap and override bitmap */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ANY,
                    0, &sw_state_entry));
        member_bitmap_width = soc_mem_field_length(unit,
                DLB_HGT_MEMBER_SW_STATEm, MEMBER_BITMAPf);
        alloc_size = SHR_BITALLOCSIZE(member_bitmap_width);
        status_bitmap = sal_alloc(alloc_size, "DLB HGT member status bitmap"); 
        if (NULL == status_bitmap) {
            return BCM_E_MEMORY;
        }
        soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit, &sw_state_entry,
                MEMBER_BITMAPf, status_bitmap);

        override_bitmap = sal_alloc(alloc_size, "DLB HGT member override bitmap"); 
        if (NULL == override_bitmap) {
            sal_free(status_bitmap);
            return BCM_E_MEMORY;
        }
        soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit, &sw_state_entry,
                OVERRIDE_MEMBER_BITMAPf, override_bitmap);

        /* Derive status from software override and status bitmaps or
         * hardware state bitmap.
         */
        if (SHR_BITGET(override_bitmap, member_id)) {
            if (SHR_BITGET(status_bitmap, member_id)) {
                *status = BCM_TRUNK_DYNAMIC_MEMBER_FORCE_UP;
            } else {
                *status = BCM_TRUNK_DYNAMIC_MEMBER_FORCE_DOWN;
            }
        } else {
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_mem_is_valid(unit, DLB_HGT_MEMBER_HW_STATEm)) {
                dlb_hgt_member_hw_state_entry_t hw_state_entry;
                SHR_BITDCL *hw_state_bitmap = NULL;

                rv = READ_DLB_HGT_MEMBER_HW_STATEm(unit, MEM_BLOCK_ANY, 0,
                        &hw_state_entry);
                if (BCM_FAILURE(rv)) {
                    sal_free(status_bitmap);
                    sal_free(override_bitmap);
                    return rv;
                }
                hw_state_bitmap = sal_alloc(alloc_size, "DLB HGT HW status bitmap"); 
                if (NULL == hw_state_bitmap) {
                    sal_free(status_bitmap);
                    sal_free(override_bitmap);
                    return BCM_E_MEMORY;
                }
                soc_DLB_HGT_MEMBER_HW_STATEm_field_get(unit, &hw_state_entry,
                        BITMAPf, hw_state_bitmap);
                if (SHR_BITGET(hw_state_bitmap, member_id)) {
                    *status = BCM_TRUNK_DYNAMIC_MEMBER_HW_UP;
                } else {
                    *status = BCM_TRUNK_DYNAMIC_MEMBER_HW_DOWN;
                }
                sal_free(hw_state_bitmap);
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                rv = READ_DLB_HGT_MEMBER_HW_STATEr(unit, &hw_state_reg); 
                if (BCM_FAILURE(rv)) {
                    sal_free(status_bitmap);
                    sal_free(override_bitmap);
                    return rv;
                }
                hw_state_reg_bitmap = soc_reg_field_get(unit,
                        DLB_HGT_MEMBER_HW_STATEr, hw_state_reg, BITMAPf);
                if (hw_state_reg_bitmap & (1 << member_id)) {
                    *status = BCM_TRUNK_DYNAMIC_MEMBER_HW_UP;
                } else {
                    *status = BCM_TRUNK_DYNAMIC_MEMBER_HW_DOWN;
                }
            }
        }

        sal_free(status_bitmap);
        sal_free(override_bitmap);

    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        /* Get status bitmap and override bitmap */
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_LINK_CONTROLm(unit, MEM_BLOCK_ANY,
                    0, &dlb_hgt_link_control_entry));
        soc_mem_pbmp_field_get(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_PORT_STATEf, &sw_port_state);
        soc_mem_pbmp_field_get(unit, DLB_HGT_LINK_CONTROLm,
                &dlb_hgt_link_control_entry, SW_OVERRIDE_PORT_MAPf,
                &override_port_map);

        /* Derive status from software override and status bitmaps or
         * hardware state bitmap.
         */
        BCM_PBMP_PORT_SET(port_pbmp, port);
        BCM_PBMP_AND(override_port_map, port_pbmp);
        BCM_PBMP_AND(sw_port_state, port_pbmp);
        if (BCM_PBMP_NOT_NULL(override_port_map)) {
            if (BCM_PBMP_NOT_NULL(sw_port_state)) {
                *status = BCM_TRUNK_DYNAMIC_MEMBER_FORCE_UP;
            } else {
                *status = BCM_TRUNK_DYNAMIC_MEMBER_FORCE_DOWN;
            }
        } else {
            SOC_IF_ERROR_RETURN(READ_DLB_HGT_PORT_STATEm(unit, MEM_BLOCK_ANY,
                        0, &port_state_entry)); 
            soc_mem_pbmp_field_get(unit, DLB_HGT_PORT_STATEm,
                    &port_state_entry, BITMAPf, &hw_port_state);
            BCM_PBMP_AND(hw_port_state, port_pbmp);
            if (BCM_PBMP_NOT_NULL(hw_port_state)) {
                *status = BCM_TRUNK_DYNAMIC_MEMBER_HW_UP;
            } else {
                *status = BCM_TRUNK_DYNAMIC_MEMBER_HW_DOWN;
            }
        }
    }

    return rv;
}

#ifdef BCM_TRIUMPH3_SUPPORT
/*
 * Function:
 *      bcm_tr3_hg_dlb_ethertype_set
 * Purpose:
 *      Set the Ethertypes that are eligible or ineligible for
 *      Higig dynamic load balancing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) BCM_TRUNK_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_count - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (IN) Array of Ethertypes.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_tr3_hg_dlb_ethertype_set(
    int unit, 
    uint32 flags, 
    int ethertype_count, 
    int *ethertype_array)
{
    uint32 measure_control_reg;
    int i, j;
    dlb_hgt_ethertype_eligibility_map_entry_t ethertype_entry;

    /* Input check */
    if (ethertype_count > soc_mem_index_count(unit,
                DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm)) {
        return BCM_E_RESOURCE;
    }

    /* Update quality measure control register */
    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
            &measure_control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf,
            flags & BCM_TRUNK_DYNAMIC_ETHERTYPE_ELIGIBLE ? 1 : 0);
    soc_reg_field_set(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
            &measure_control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf,
            flags & BCM_TRUNK_DYNAMIC_ETHERTYPE_INNER ? 1 : 0);
    SOC_IF_ERROR_RETURN
        (WRITE_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, measure_control_reg));

    /* Update Ethertype eligibility map table */
    for (i = 0; i < ethertype_count; i++) {
        sal_memset(&ethertype_entry, 0,
                sizeof(dlb_hgt_ethertype_eligibility_map_entry_t));
        soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, VALIDf, 1);
        soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, ETHERTYPEf, ethertype_array[i]);
        SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, i, &ethertype_entry));
    }

    /* Zero out remaining entries of Ethertype eligibility map table */
    for (j = i; j < soc_mem_index_count(unit,
                DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm); j++) {
        SOC_IF_ERROR_RETURN(WRITE_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, j, soc_mem_entry_null(unit,
                        DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm)));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_hg_dlb_ethertype_get
 * Purpose:
 *      Get the Ethertypes that are eligible or ineligible for
 *      Higig dynamic load balancing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (INOUT) BCM_TRUNK_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_max - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (OUT) Array of Ethertypes.
 *      ethertype_count - (OUT) Number of elements returned in ethertype_array.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_tr3_hg_dlb_ethertype_get(
    int unit, 
    uint32 *flags, 
    int ethertype_max, 
    int *ethertype_array, 
    int *ethertype_count)
{
    uint32 measure_control_reg;
    int i;
    int ethertype;
    dlb_hgt_ethertype_eligibility_map_entry_t ethertype_entry;

    *ethertype_count = 0;

    /* Get flags */
    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &measure_control_reg));
    if (soc_reg_field_get(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                measure_control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf)) {
        *flags |= BCM_TRUNK_DYNAMIC_ETHERTYPE_ELIGIBLE;
    }
    if (soc_reg_field_get(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
                measure_control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf)) {
        *flags |= BCM_TRUNK_DYNAMIC_ETHERTYPE_INNER;
    }

    /* Get Ethertypes */
    for (i = 0; i < soc_mem_index_count(unit,
                DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm); i++) {
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ANY, i, &ethertype_entry));
        if (soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                    &ethertype_entry, VALIDf)) {
            ethertype = soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                    &ethertype_entry, ETHERTYPEf);
            if (NULL != ethertype_array) {
                ethertype_array[*ethertype_count] = ethertype;
            }
            (*ethertype_count)++;
            if ((ethertype_max > 0) && (*ethertype_count == ethertype_max)) {
                break;
            }
        }
    }

    return BCM_E_NONE;
}

#endif /* BCM_TRIUMPH3_SUPPORT */

/* ----------------------------------------------------------------------------- 
 *
 * Warm Boot routines
 *
 * ----------------------------------------------------------------------------- 
 */
#ifdef BCM_WARM_BOOT_SUPPORT

void
_bcm_trident_hw_failover_num_ports_set(int unit, bcm_trunk_t tid,
                                       int hg_trunk, uint16 num_ports)
{
    bcm_trunk_chip_info_t chip_info;
    int tid_ix;

    (void) bcm_esw_trunk_chip_info_get(unit, &chip_info);
    tid_ix = tid + (hg_trunk ? chip_info.trunk_group_count : 0);
    _trident_trunk_hwfail[unit]->hw_tinfo[tid_ix].num_ports = num_ports;

    return;
}

uint16
_bcm_trident_hw_failover_num_ports_get(int unit, bcm_trunk_t tid,
                                       int hg_trunk)
{
    bcm_trunk_chip_info_t chip_info;
    int tid_ix;
    uint16 num_ports;

    (void) bcm_esw_trunk_chip_info_get(unit, &chip_info);
    tid_ix = tid + (hg_trunk ? chip_info.trunk_group_count : 0);
    num_ports = _trident_trunk_hwfail[unit]->hw_tinfo[tid_ix].num_ports;

    return num_ports;
}

int
_bcm_trident_hw_failover_flags_set(int unit, bcm_trunk_t tid,
                                   int port_index, int hg_trunk, uint8 flags)
{
    bcm_trunk_chip_info_t chip_info;
    int tid_ix;
    _trident_hw_tinfo_t *hwft;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    tid_ix = tid + (hg_trunk ? chip_info.trunk_group_count : 0);
    hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid_ix]);

    if (NULL == hwft->flags) {
        hwft->flags = sal_alloc(sizeof(uint32) * hwft->num_ports,
                "hw_tinfo_flags");
        if (hwft->flags == NULL) {
            return BCM_E_MEMORY;
        }
    }
    hwft->flags[port_index] = flags;

    return BCM_E_NONE;
}

uint8
_bcm_trident_hw_failover_flags_get(int unit, bcm_trunk_t tid,
                                   int port_index, int hg_trunk)
{
    bcm_trunk_chip_info_t chip_info;
    int tid_ix;
    uint8 flags;
    _trident_hw_tinfo_t *hwft;

    (void) bcm_esw_trunk_chip_info_get(unit, &chip_info);
    tid_ix = tid + (hg_trunk ? chip_info.trunk_group_count : 0);
    hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid_ix]);

    if (NULL != hwft->flags) {
        flags = _trident_trunk_hwfail[unit]->hw_tinfo[tid_ix].flags[port_index];
    } else {
        flags = 0;
    }

    return flags;
}

int
_bcm_trident_trunk_member_info_set(int unit, bcm_trunk_t tid,
                                uint16 num_ports,
                                uint16 *modport,
                                uint32 *member_flags)
{
    int i;

    if (MEMBER_INFO(unit, tid).modport) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
    }
    if (MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).member_flags);
        MEMBER_INFO(unit, tid).member_flags = NULL;
    }

    MEMBER_INFO(unit, tid).num_ports = num_ports;

    MEMBER_INFO(unit, tid).modport = sal_alloc(
            sizeof(uint16) * MEMBER_INFO(unit, tid).num_ports,
            "member info modport");
    if (NULL == MEMBER_INFO(unit, tid).modport) {
        return BCM_E_MEMORY;
    }

    MEMBER_INFO(unit, tid).member_flags = sal_alloc(
            sizeof(uint32) * MEMBER_INFO(unit, tid).num_ports,
            "member info flags");
    if (NULL == MEMBER_INFO(unit, tid).member_flags) {
        sal_free(MEMBER_INFO(unit, tid).modport);
        MEMBER_INFO(unit, tid).modport = NULL;
        return BCM_E_MEMORY;
    }

    for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
        MEMBER_INFO(unit, tid).modport[i] = modport[i];
        MEMBER_INFO(unit, tid).member_flags[i] = member_flags[i];
    }

    MEMBER_INFO(unit, tid).recovered_from_scache = 1;

    return BCM_E_NONE;
}

int
_bcm_trident_trunk_member_info_get(int unit, bcm_trunk_t tid,
                                uint16 array_size,
                                uint16 *modport,
                                uint32 *member_flags,
                                uint16 *num_ports)
{
    int i;

    if (NULL == num_ports) {
        return BCM_E_PARAM;
    }
    *num_ports = MEMBER_INFO(unit, tid).num_ports;

    if (array_size == 0) {
        return BCM_E_NONE;
    }

    for (i = 0; i < array_size; i++) {
        if (i == *num_ports) {
           break;
        } 
        modport[i] = MEMBER_INFO(unit, tid).modport[i];
        member_flags[i] = MEMBER_INFO(unit, tid).member_flags[i];
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_trident_trunk_rtag_to_psc(int unit, int rtag, int *psc)
{
    if (7 == rtag) {
        if (soc_feature(unit, soc_feature_port_flow_hash)) {
            *psc = BCM_TRUNK_PSC_PORTFLOW;
        } else if (soc_feature(unit, soc_feature_port_trunk_index)) {
            *psc = BCM_TRUNK_PSC_PORTINDEX;
        } else {
            return BCM_E_PARAM;
        }
    } else if (0 < rtag) {
        /* HW and SW values match */
        *psc = rtag;
    } else {
        /* Unknown rtag value */
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_trunk_mod_port_map_reinit
 * Purpose:
 *      Reinitialize the software cache for the SOURCE_TRUNK_MAP_TABLE.
 *      Recover trunk members with egress disable flag set.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      mod - (IN) Module ID.
 *      base_index - (IN) Module ID's base index.
 *      num_ports - (IN) Number of ports in the given module.
 * Returns:
 *      BCM_X_XXX
 * Notes:
 *      This function is called during stacking module reinit, since
 *      the number of ports in a given module is not known until then.
 */
int 
_bcm_trident_trunk_mod_port_map_reinit(int unit, bcm_module_t mod,
        int base_index, int num_ports) 
{
    bcm_port_t port;
    int	rv = BCM_E_NONE;
    source_trunk_map_table_entry_t stm; 
    int port_type;
    bcm_trunk_t tid;
    int array_size;
    bcm_module_t *mod_array = NULL;
    bcm_port_t *port_array = NULL;
    uint32 *flags_array = NULL;
    int i, found;

    /*
     * Recover from SOURCE_TRUNK_MAP tables.
     */
    for (port = 0; port < num_ports; port++) {

        rv = soc_mem_read(unit, SOURCE_TRUNK_MAP_TABLEm, MEM_BLOCK_ANY,
                base_index + port, &stm);
        if (BCM_FAILURE(rv)) {
            break;
        }

        port_type = soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm,
                PORT_TYPEf);
        if (port_type != 1) {
            continue;
        }

        tid = soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm, TGIDf);

        /* Recover source trunk map cache */
        rv = _bcm_trident_trunk_mod_port_map_set(unit, mod, port, tid);  
        if (BCM_FAILURE(rv)) {
            break;
        }

        /* Allocate arrays that can accommodate an additional member,
         * in case a new member needs to be added to the trunk group.
         */
        array_size = MEMBER_INFO(unit, tid).num_ports + 1;
        mod_array = sal_alloc(sizeof(bcm_module_t) * array_size, "mod_array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            break;
        }
        sal_memset(mod_array, 0, sizeof(bcm_module_t) * array_size); 

        port_array = sal_alloc(sizeof(bcm_port_t) * array_size, "port_array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            break;
        }
        sal_memset(port_array, 0, sizeof(bcm_port_t) * array_size); 

        flags_array = sal_alloc(sizeof(uint32) * array_size, "flags_array");
        if (flags_array == NULL) {
            rv = BCM_E_MEMORY;
            break;
        }
        sal_memset(flags_array, 0, sizeof(uint32) * array_size); 

        /* Try to find mod-port in the trunk group */
        found = 0; 
        for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
            mod_array[i] = MEMBER_INFO(unit, tid).modport[i] >> 8;
            port_array[i] = MEMBER_INFO(unit, tid).modport[i] & 0xff;
            flags_array[i] = MEMBER_INFO(unit, tid).member_flags[i];
            if (port_array[i] == port && mod_array[i] == mod) {
                found = 1;
                break;
            }
        }

        /* If the mod-port was not found, then add it with
         * EGRESS_DISABLE flag set.
         */  
        if (!found && !MEMBER_INFO(unit, tid).recovered_from_scache) {
            /* Find where to insert */
            for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
                if (port > port_array[i] || mod != mod_array[i]) {
                    continue;
                } 
                break;
            }

            /* Insert at the required location in sw state */
            mod_array[i] = mod;
            port_array[i] = port;
            flags_array[i] |= BCM_TRUNK_MEMBER_EGRESS_DISABLE;

            /* Copy remaining members as is */
            for (; i < MEMBER_INFO(unit, tid).num_ports; i++) {
                mod_array[i+1] = MEMBER_INFO(unit, tid).modport[i] >> 8;
                port_array[i+1] = MEMBER_INFO(unit, tid).modport[i] & 0xff;
                flags_array[i+1] = MEMBER_INFO(unit, tid).member_flags[i];
            }

            MEMBER_INFO(unit, tid).num_ports++;

            if (MEMBER_INFO(unit, tid).modport) {
                sal_free(MEMBER_INFO(unit, tid).modport);
                MEMBER_INFO(unit, tid).modport = NULL;
            }
            if (MEMBER_INFO(unit, tid).member_flags) {
                sal_free(MEMBER_INFO(unit, tid).member_flags);
                MEMBER_INFO(unit, tid).member_flags = NULL;
            }

            MEMBER_INFO(unit, tid).modport = sal_alloc(
                    sizeof(uint16) * MEMBER_INFO(unit, tid).num_ports,
                    "member info modport");
            if (NULL == MEMBER_INFO(unit, tid).modport) {
                rv = BCM_E_MEMORY;
                break;
            }

            MEMBER_INFO(unit, tid).member_flags = sal_alloc(
                    sizeof(uint32) * MEMBER_INFO(unit, tid).num_ports, "member info flags");
            if (NULL == MEMBER_INFO(unit, tid).member_flags) {
                sal_free(MEMBER_INFO(unit, tid).modport);
                MEMBER_INFO(unit, tid).modport = NULL;
                rv = BCM_E_MEMORY;
                break;
            }

            for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
                MEMBER_INFO(unit, tid).modport[i] = (mod_array[i] << 8) |
                    port_array[i];
                MEMBER_INFO(unit, tid).member_flags[i] = flags_array[i];
            }
        } 

        sal_free(mod_array);
        mod_array = NULL;
        sal_free(port_array);
        port_array = NULL;
        sal_free(flags_array);
        flags_array = NULL;
    }

    if (mod_array) {
        sal_free(mod_array);
    }
    if (port_array) {
        sal_free(port_array);
    }
    if (flags_array) {
        sal_free(flags_array);
    }

    return rv;
}

/*
 * Handle state recovery for front panel trunks
 */
int
_bcm_trident_trunk_lag_reinit(int unit, int ngroups_fp, trunk_private_t *trunk_info)
{
    int rv = BCM_E_NONE;
    int tid, tid_get;
    int ptype;
    trunk_group_entry_t tg_entry;
    int base_ptr, tg_size, rtag;
    bcm_module_t *egr_hw_mods;
    bcm_port_t *egr_hw_ports;
    uint32 *egr_member_flags;
    trunk_member_entry_t trunk_member_entry;
    source_trunk_map_table_entry_t stm;
    int stm_idx;
    int i, j, mod_is_local;
    int hwf_rtag, hwf_psc;
    _trident_hw_tinfo_t *hwft;
    int num_egr_ports;
    int num_entries;

    for (tid = 0; tid < ngroups_fp; tid++, trunk_info++) {

        SOC_IF_ERROR_RETURN
            (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &tg_entry));

        base_ptr = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, BASE_PTRf);
        tg_size = 1 + soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, TG_SIZEf);
        rtag = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, RTAGf);

        if (0 == rtag) {
            /* Invalid trunk group */
            continue;
        }

        /* Recover trunk_info */
        trunk_info->tid = tid;
        trunk_info->in_use = TRUE;
        BCM_IF_ERROR_RETURN
            (_bcm_trident_trunk_rtag_to_psc(unit, rtag, &trunk_info->psc));
        trunk_info->rtag = rtag;

        /* Recover trunk member table bookkeeping info */
        num_entries = tg_size;
        if (rtag >= 1 && rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
            if (SOC_IS_KATANAX(unit)) {
                num_entries = _BCM_KATANA_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
            } else
#endif
            if (soc_feature(unit,
                        soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
                num_entries = _BCM_TD_FP_TRUNK_RTAG1_6_MAX_PORTCNT;
            }
        } 
        _BCM_FP_TRUNK_MEMBER_USED_SET_RANGE(unit, base_ptr, num_entries);

        /* Recover trunk members */
        if (MEMBER_INFO(unit, tid).recovered_from_scache) {
            egr_hw_mods = sal_alloc(sizeof(bcm_module_t) *
                    MEMBER_INFO(unit, tid).num_ports, "egr_hw_mods");
            if (egr_hw_mods == NULL) {
                return BCM_E_MEMORY;
            }

            egr_hw_ports = sal_alloc(sizeof(bcm_port_t) * 
                    MEMBER_INFO(unit, tid).num_ports, "egr_hw_ports");
            if (egr_hw_ports == NULL) {
                sal_free(egr_hw_mods);
                return BCM_E_MEMORY;
            }

            egr_member_flags = sal_alloc(sizeof(uint32) *
                    MEMBER_INFO(unit, tid).num_ports, "egr_member_flags");
            if (egr_member_flags == NULL) {
                sal_free(egr_hw_mods);
                sal_free(egr_hw_ports);
                return BCM_E_MEMORY;
            }

            for (i = 0, j = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
                if (MEMBER_INFO(unit, tid).member_flags[i] &
                        BCM_TRUNK_MEMBER_EGRESS_DISABLE) {
                    continue;
                }
                egr_hw_mods[j] = MEMBER_INFO(unit, tid).modport[i] >> 8;
                egr_hw_ports[j] = MEMBER_INFO(unit, tid).modport[i] & 0xff;
                egr_member_flags[j] = MEMBER_INFO(unit, tid).member_flags[i];
                j++;
            }
            num_egr_ports = j;

        } else {
            /* In level 1 warm boot, not all egress trunk members can be
             * recovered from TRUNK_MEMBER table, as those members with
             * the UNICAST_EGRESS_DISABLE flag set will not be in
             * TRUNK_MEMBER table.
             */

            egr_hw_mods = sal_alloc(sizeof(bcm_module_t) * tg_size,
                    "egr_hw_mods");
            if (egr_hw_mods == NULL) {
                return BCM_E_MEMORY;
            }

            egr_hw_ports = sal_alloc(sizeof(bcm_port_t) * tg_size,
                    "egr_hw_ports");
            if (egr_hw_ports == NULL) {
                sal_free(egr_hw_mods);
                return BCM_E_MEMORY;
            }

            egr_member_flags = sal_alloc(sizeof(uint32) * tg_size,
                    "egr_member_flags");
            if (egr_member_flags == NULL) {
                sal_free(egr_hw_mods);
                sal_free(egr_hw_ports);
                return BCM_E_MEMORY;
            }

            for (i = 0; i < tg_size; i++) {
                rv = READ_TRUNK_MEMBERm(unit, MEM_BLOCK_ANY, base_ptr + i,
                        &trunk_member_entry);
                if (BCM_FAILURE(rv)) {
                    sal_free(egr_hw_mods);
                    sal_free(egr_hw_ports);
                    sal_free(egr_member_flags);
                    return rv;
                }
                egr_hw_mods[i] = soc_TRUNK_MEMBERm_field32_get
                    (unit, &trunk_member_entry, MODULE_IDf); 
                egr_hw_ports[i] = soc_TRUNK_MEMBERm_field32_get
                    (unit, &trunk_member_entry, PORT_NUMf); 
                egr_member_flags[i] = 0;

                /* Recreate INGRESS_DISABLE flag */
                rv = _bcm_esw_src_mod_port_table_index_get(unit, egr_hw_mods[i],
                        egr_hw_ports[i], &stm_idx);
                if (BCM_FAILURE(rv)) {
                    sal_free(egr_hw_mods);
                    sal_free(egr_hw_ports);
                    sal_free(egr_member_flags);
                    return rv;
                }
                rv = soc_mem_read(unit, SOURCE_TRUNK_MAP_TABLEm, MEM_BLOCK_ANY,
                        stm_idx, &stm);
                if (BCM_FAILURE(rv)) {
                    sal_free(egr_hw_mods);
                    sal_free(egr_hw_ports);
                    sal_free(egr_member_flags);
                    return rv;
                }
                ptype = soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm,
                        PORT_TYPEf);
                tid_get = soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm,
                        TGIDf);
                if (ptype != 1 || tid_get != tid) {
                    egr_member_flags[i] |= BCM_TRUNK_MEMBER_INGRESS_DISABLE;
                }
            }

            num_egr_ports = tg_size;

            if (MEMBER_INFO(unit, tid).modport) {
                sal_free(MEMBER_INFO(unit, tid).modport);
                MEMBER_INFO(unit, tid).modport = NULL;
            }
            if (MEMBER_INFO(unit, tid).member_flags) {
                sal_free(MEMBER_INFO(unit, tid).member_flags);
                MEMBER_INFO(unit, tid).member_flags = NULL;
            }

            MEMBER_INFO(unit, tid).num_ports = tg_size;
            MEMBER_INFO(unit, tid).modport = sal_alloc(
                    sizeof(uint16) * MEMBER_INFO(unit, tid).num_ports,
                    "member info modport");
            if (NULL == MEMBER_INFO(unit, tid).modport) {
                sal_free(egr_hw_mods);
                sal_free(egr_hw_ports);
                sal_free(egr_member_flags);
                return BCM_E_MEMORY;
            }

            MEMBER_INFO(unit, tid).member_flags = sal_alloc(
                    sizeof(uint32) * MEMBER_INFO(unit, tid).num_ports,
                    "member info flags");
            if (NULL == MEMBER_INFO(unit, tid).member_flags) {
                sal_free(egr_hw_mods);
                sal_free(egr_hw_ports);
                sal_free(egr_member_flags);
                sal_free(MEMBER_INFO(unit, tid).modport);
                MEMBER_INFO(unit, tid).modport = NULL;
                return BCM_E_MEMORY;
            }

            for (i = 0; i < MEMBER_INFO(unit, tid).num_ports; i++) {
                MEMBER_INFO(unit, tid).modport[i] =
                    (egr_hw_mods[i] << 8) | egr_hw_ports[i];
                MEMBER_INFO(unit, tid).member_flags[i] = egr_member_flags[i];
            }
        }

        /* Recover software trunk failover state */
        rv = _bcm_trident_trunk_swfailover_fp_set(unit, tid, rtag,
                num_egr_ports,
                egr_hw_mods,
                egr_hw_ports,
                egr_member_flags,
                trunk_info->flags);
        if (BCM_FAILURE(rv)) {
            sal_free(egr_hw_mods);
            sal_free(egr_hw_ports);
            sal_free(egr_member_flags);
            return rv;
        }

        /* Recover hardware trunk failover state. */
        if (soc_feature(unit, soc_feature_port_lag_failover)) {
            hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid]);
            hwft->num_ports = num_egr_ports;
            if (hwft->modport == NULL) {
                hwft->modport = sal_alloc(sizeof(uint16) * num_egr_ports,
                        "hw_tinfo_modport");
                if (hwft->modport == NULL) {
                    sal_free(egr_hw_mods);
                    sal_free(egr_hw_ports);
                    sal_free(egr_member_flags);
                    return BCM_E_MEMORY;
                }
            }
            sal_memset(hwft->modport, 0, sizeof(uint16) * num_egr_ports);

            if (hwft->psc == NULL) {
                hwft->psc = sal_alloc(sizeof(uint8) * num_egr_ports,
                        "hw_tinfo_psc");
                if (hwft->psc == NULL) {
                    sal_free(hwft->modport);
                    hwft->modport = NULL;
                    sal_free(egr_hw_mods);
                    sal_free(egr_hw_ports);
                    sal_free(egr_member_flags);
                    return BCM_E_MEMORY;
                }
            }
            sal_memset(hwft->psc, 0, sizeof(uint8) * num_egr_ports);

            if (hwft->flags == NULL) {
                /* If flags are not recovered from scache, allocate flags and
                 * set them to zero.
                 */
                hwft->flags = sal_alloc(sizeof(uint32) * num_egr_ports,
                        "hw_tinfo_flags");
                if (hwft->flags == NULL) {
                    sal_free(hwft->modport);
                    hwft->modport = NULL;
                    sal_free(hwft->psc);
                    hwft->psc = NULL;
                    sal_free(egr_hw_mods);
                    sal_free(egr_hw_ports);
                    sal_free(egr_member_flags);
                    return BCM_E_MEMORY;
                }
                sal_memset(hwft->flags, 0, sizeof(uint32) * num_egr_ports);
            }


            for (i = 0; i < num_egr_ports; i++) {
                hwft->modport[i] = (egr_hw_mods[i] << 8) | egr_hw_ports[i];
                rv = _bcm_esw_modid_is_local(unit, egr_hw_mods[i], &mod_is_local);
                if (BCM_FAILURE(rv)) {
                    sal_free(egr_hw_mods);
                    sal_free(egr_hw_ports);
                    sal_free(egr_member_flags);
                    return rv;
                }
                if (mod_is_local) { 
                    rv = _bcm_trident_trunk_hwfailover_read(unit,
                            egr_hw_ports[i], 0, &hwf_rtag, NULL, NULL, NULL);
                    if (BCM_FAILURE(rv)) {
                        sal_free(egr_hw_mods);
                        sal_free(egr_hw_ports);
                        sal_free(egr_member_flags);
                        return rv;
                    }
                    if (hwf_rtag > 0) {
                        rv = _bcm_trident_trunk_rtag_to_psc(unit, hwf_rtag,
                                &hwf_psc);
                        if (BCM_FAILURE(rv)) {
                            sal_free(egr_hw_mods);
                            sal_free(egr_hw_ports);
                            sal_free(egr_member_flags);
                            return rv;
                        }
                    } else {
                        hwf_psc = trunk_info->psc;
                    }
                    hwft->psc[i] = hwf_psc;
                }
            }
        }

        sal_free(egr_hw_mods);
        sal_free(egr_hw_ports);
        sal_free(egr_member_flags);

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_lag_dlb)) {
            BCM_IF_ERROR_RETURN
                (bcm_tr3_lag_dlb_group_recover(unit, tid, trunk_info));
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
            BCM_IF_ERROR_RETURN
                (bcm_td2_lag_rh_recover(unit, tid, trunk_info));
        }
#endif /* BCM_TRIDENT2_SUPPORT */

    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_lag_dlb)) {
        BCM_IF_ERROR_RETURN(bcm_tr3_lag_dlb_member_recover(unit));
        BCM_IF_ERROR_RETURN(bcm_tr3_lag_dlb_quality_parameters_recover(unit));
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_trident_hg_dlb_wb_alloc_size_get
 * Purpose:
 *      Get level 2 warm boot scache size for Higig DLB.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      size - (OUT) Allocation size.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_trident_hg_dlb_wb_alloc_size_get(int unit, int *size) 
{
    int alloc_size = 0;
    int num_elements = 0;
    soc_field_t profile_ptr_f;

    /* Allocate size for the following Higig DLB parameters: 
     * int hg_dlb_sample_rate;
     * int hg_dlb_tx_load_min_th;
     * int hg_dlb_tx_load_max_th;
     * int hg_dlb_qsize_min_th;
     * int hg_dlb_qsize_max_th;
     */
    alloc_size += sizeof(int) * 5;

    /* Allocate size of hg_dlb_load_weight array */
    if (soc_reg_field_valid(unit, DLB_HGT_QUANTIZE_CONTROLr,
                PORT_QUALITY_MAPPING_PROFILE_PTRf)) {
        num_elements = 1 << soc_reg_field_length(unit,
                DLB_HGT_QUANTIZE_CONTROLr, PORT_QUALITY_MAPPING_PROFILE_PTRf);
    } else {
        profile_ptr_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_CONTROLm,
                PROFILE_PTRf) ? PROFILE_PTRf : QMT_PROFILE_PTRf;
        num_elements = 1 << soc_mem_field_length(unit,
                DLB_HGT_QUALITY_CONTROLm, profile_ptr_f);
    }
    alloc_size += sizeof(uint8) * num_elements;

    *size = alloc_size;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trident_hg_dlb_sync
 * Purpose:
 *      Store Higig DLB parameters into scache.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_trident_hg_dlb_sync(int unit, uint8 **scache_ptr) 
{
    int value;
    int num_elements = 0;
    int i;
    uint8 load_weight;
    soc_field_t profile_ptr_f;

    /* Store the following Higig DLB parameters: 
     * int hg_dlb_sample_rate;
     * int hg_dlb_tx_load_min_th;
     * int hg_dlb_tx_load_max_th;
     * int hg_dlb_qsize_min_th;
     * int hg_dlb_qsize_max_th;
     */
    value = HG_DLB_INFO(unit)->hg_dlb_sample_rate;
    sal_memcpy((*scache_ptr), &value, sizeof(int));
    (*scache_ptr) += sizeof(int);

    value = HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th;
    sal_memcpy((*scache_ptr), &value, sizeof(int));
    (*scache_ptr) += sizeof(int);

    value = HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th;
    sal_memcpy((*scache_ptr), &value, sizeof(int));
    (*scache_ptr) += sizeof(int);

    value = HG_DLB_INFO(unit)->hg_dlb_qsize_min_th;
    sal_memcpy((*scache_ptr), &value, sizeof(int));
    (*scache_ptr) += sizeof(int);

    value = HG_DLB_INFO(unit)->hg_dlb_qsize_max_th;
    sal_memcpy((*scache_ptr), &value, sizeof(int));
    (*scache_ptr) += sizeof(int);

    /* Store hg_dlb_load_weight array */
    if (soc_reg_field_valid(unit, DLB_HGT_QUANTIZE_CONTROLr,
                PORT_QUALITY_MAPPING_PROFILE_PTRf)) {
        num_elements = 1 << soc_reg_field_length(unit,
                DLB_HGT_QUANTIZE_CONTROLr, PORT_QUALITY_MAPPING_PROFILE_PTRf);
    } else {
        profile_ptr_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_CONTROLm,
                PROFILE_PTRf) ? PROFILE_PTRf : QMT_PROFILE_PTRf;
        num_elements = 1 << soc_mem_field_length(unit,
                DLB_HGT_QUALITY_CONTROLm, profile_ptr_f);
    }
    for (i = 0; i < num_elements; i++) {
        load_weight = HG_DLB_INFO(unit)->hg_dlb_load_weight[i];
        sal_memcpy((*scache_ptr), &load_weight, sizeof(uint8));
        (*scache_ptr) += sizeof(uint8);
    }
        
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trident_hg_dlb_scache_recover
 * Purpose:
 *      Recover Higig DLB parameters from scache.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_trident_hg_dlb_scache_recover(int unit, uint8 **scache_ptr) 
{
    int value;
    int num_elements = 0;
    int i;
    uint8 load_weight;
    soc_field_t profile_ptr_f;

    /* Recover the following Higig DLB parameters: 
     * int hg_dlb_sample_rate;
     * int hg_dlb_tx_load_min_th;
     * int hg_dlb_tx_load_max_th;
     * int hg_dlb_qsize_min_th;
     * int hg_dlb_qsize_max_th;
     */
    sal_memcpy(&value, (*scache_ptr), sizeof(int));
    HG_DLB_INFO(unit)->hg_dlb_sample_rate = value;
    (*scache_ptr) += sizeof(int);

    sal_memcpy(&value, (*scache_ptr), sizeof(int));
    HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th = value;
    (*scache_ptr) += sizeof(int);

    sal_memcpy(&value, (*scache_ptr), sizeof(int));
    HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th = value;
    (*scache_ptr) += sizeof(int);

    sal_memcpy(&value, (*scache_ptr), sizeof(int));
    HG_DLB_INFO(unit)->hg_dlb_qsize_min_th = value;
    (*scache_ptr) += sizeof(int);

    sal_memcpy(&value, (*scache_ptr), sizeof(int));
    HG_DLB_INFO(unit)->hg_dlb_qsize_max_th = value;
    (*scache_ptr) += sizeof(int);

    /* Recover hg_dlb_load_weight array */
    if (soc_reg_field_valid(unit, DLB_HGT_QUANTIZE_CONTROLr,
                PORT_QUALITY_MAPPING_PROFILE_PTRf)) {
        num_elements = 1 << soc_reg_field_length(unit,
                DLB_HGT_QUANTIZE_CONTROLr, PORT_QUALITY_MAPPING_PROFILE_PTRf);
    } else {
        profile_ptr_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_CONTROLm,
                PROFILE_PTRf) ? PROFILE_PTRf : QMT_PROFILE_PTRf;
        num_elements = 1 << soc_mem_field_length(unit,
                DLB_HGT_QUALITY_CONTROLm, profile_ptr_f);
    }
    for (i = 0; i < num_elements; i++) {
        sal_memcpy(&load_weight, (*scache_ptr), sizeof(uint8));
        HG_DLB_INFO(unit)->hg_dlb_load_weight[i] = load_weight;
        (*scache_ptr) += sizeof(uint8);
    }
        
    HG_DLB_INFO(unit)->recovered_from_scache = TRUE;

    return BCM_E_NONE;
}

#ifdef BCM_TRIUMPH3_SUPPORT
/*
 * Function:
 *      _bcm_tr3_hg_dlb_member_recover
 * Purpose:
 *      Recover Higig DLB member ID usage.
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr3_hg_dlb_member_recover(int unit) 
{
    int i, member_id;
    dlb_hgt_port_member_map_entry_t port_member_map_entry;

    for (i = 0; i < soc_mem_index_count(unit, DLB_HGT_PORT_MEMBER_MAPm);
            i++) {
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_PORT_MEMBER_MAPm(unit,
                    MEM_BLOCK_ANY, i, &port_member_map_entry));
        if (soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                    &port_member_map_entry, VALIDf)) {
            member_id = soc_DLB_HGT_PORT_MEMBER_MAPm_field32_get(unit,
                    &port_member_map_entry, MEMBER_IDf);
            _BCM_HG_DLB_MEMBER_ID_USED_SET(unit, member_id);
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      _bcm_trident_hg_dlb_quality_parameters_recover
 * Purpose:
 *      Recover Higig DLB parameters used to derive a member's quality.
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trident_hg_dlb_quality_parameters_recover(int unit) 
{
    uint32 measure_control_reg;
    int num_time_units;
    dlb_hgt_glb_quantize_thresholds_entry_t thresholds_entry;
    int max_th_bytes;
    int min_th_bytes;
    int max_th_cells;
    int min_th_cells;
    int entries_per_profile;
    int base_index;
    int profile_ptr;
    dlb_hgt_port_quality_mapping_entry_t port_quality_mapping_entry;
    int quality;

    if (HG_DLB_INFO(unit)->recovered_from_scache) {
        /* If already recovered fro scache, do nothing */
        return BCM_E_NONE;
    }

    /* Recover sampling rate */
    SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit,
                &measure_control_reg));
    num_time_units = soc_reg_field_get(unit, DLB_HGT_QUALITY_MEASURE_CONTROLr,
            measure_control_reg, SAMPLING_PERIODf);
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
        /* Sampling rate = 10^6 / num_time_units */
        if (num_time_units > 0) {
            HG_DLB_INFO(unit)->hg_dlb_sample_rate = 1000000 / num_time_units;
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        /* Sampling rate = 10^9 ns / (256 ns * num_time_units)
         *               = 3906250 / num_time_units
         */
        if (num_time_units > 0) {
            HG_DLB_INFO(unit)->hg_dlb_sample_rate = 3906250 / num_time_units;
        }
    }

    /* Recover max and min load and queue size thresholds */
        /* Threshold (in mbps)
         *     = (bytes per sampling period) * 8 bits per byte /
         *       (sampling period in seconds) / 10^6
         *     = (bytes per sampling period) * 8 bits per byte /
         *       (num_time_units * 1 us per time unit * 10^-6) / 10^6
         *     = (bytes per sampling period) * 8 / num_time_units 
         */ 

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        dlb_hgt_pla_quantize_threshold_entry_t quantize_threshold_entry;

        SOC_IF_ERROR_RETURN(READ_DLB_HGT_PLA_QUANTIZE_THRESHOLDm(unit,
                    MEM_BLOCK_ANY, 0, &quantize_threshold_entry));
        min_th_bytes = soc_DLB_HGT_PLA_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, LOADING_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th = (min_th_bytes << 3) /
            num_time_units;

        min_th_cells = soc_DLB_HGT_PLA_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, QSIZE_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_qsize_min_th = min_th_cells * 208;

        SOC_IF_ERROR_RETURN(READ_DLB_HGT_PLA_QUANTIZE_THRESHOLDm(unit,
                    MEM_BLOCK_ANY, 6, &quantize_threshold_entry));
        max_th_bytes = soc_DLB_HGT_PLA_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, LOADING_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th = (max_th_bytes << 3) /
            num_time_units;

        max_th_cells = soc_DLB_HGT_PLA_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, QSIZE_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_qsize_max_th = max_th_cells * 208;

    } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        dlb_hgt_quantize_threshold_entry_t quantize_threshold_entry;

        SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUANTIZE_THRESHOLDm(unit,
                    MEM_BLOCK_ANY, 0, &quantize_threshold_entry));
        min_th_bytes = soc_DLB_HGT_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, LOADING_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th = (min_th_bytes << 3) /
            num_time_units;

        min_th_cells = soc_DLB_HGT_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, QSIZE_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_qsize_min_th = min_th_cells * 208;

        SOC_IF_ERROR_RETURN(READ_DLB_HGT_QUANTIZE_THRESHOLDm(unit,
                    MEM_BLOCK_ANY, 6, &quantize_threshold_entry));
        max_th_bytes = soc_DLB_HGT_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, LOADING_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th = (max_th_bytes << 3) /
            num_time_units;

        max_th_cells = soc_DLB_HGT_QUANTIZE_THRESHOLDm_field32_get
            (unit, &quantize_threshold_entry, QSIZE_THRESHOLDf); 
        HG_DLB_INFO(unit)->hg_dlb_qsize_max_th = max_th_cells * 208;
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        /* Threshold (in mbps)
         *     = (bytes per sampling period) * 8 bits per byte /
         *       (sampling period in seconds) / 10^6
         *     = (bytes per sampling period) * 8 bits per byte /
         *       (num_time_units * 256 ns per time unit * 10^-9) / 10^6
         *     = (bytes per sampling period) * 8000 / num_time_units / 256
         */ 
        SOC_IF_ERROR_RETURN(READ_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm(unit,
                    MEM_BLOCK_ANY, 0, &thresholds_entry));
        min_th_bytes = soc_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm_field32_get
            (unit, &thresholds_entry, PORT_LOADING_THRESHOLD_1f); 
        HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th =
            ((min_th_bytes << 3) * 1000 / num_time_units) >> 8;

        max_th_bytes = soc_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm_field32_get
            (unit, &thresholds_entry, PORT_LOADING_THRESHOLD_7f); 
        HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th = 
            ((max_th_bytes << 3) * 1000 / num_time_units) >> 8;

        min_th_cells = soc_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm_field32_get
            (unit, &thresholds_entry, PORT_QSIZE_THRESHOLD_1f); 
        HG_DLB_INFO(unit)->hg_dlb_qsize_min_th = min_th_cells * 208;

        max_th_cells = soc_DLB_HGT_GLB_QUANTIZE_THRESHOLDSm_field32_get
            (unit, &thresholds_entry, PORT_QSIZE_THRESHOLD_7f); 
        HG_DLB_INFO(unit)->hg_dlb_qsize_max_th = max_th_cells * 208;
    }

    /* Recover load weights */

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
        soc_field_t profile_ptr_f, quality_f;
        dlb_hgt_quality_mapping_entry_t quality_mapping_entry;

        entries_per_profile = 64;
        profile_ptr_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_CONTROLm,
                PROFILE_PTRf) ? PROFILE_PTRf : QMT_PROFILE_PTRf;
        quality_f = soc_mem_field_valid(unit, DLB_HGT_QUALITY_MAPPINGm,
                QUALITYf) ? QUALITYf : ASSIGNED_QUALITYf;
        for (profile_ptr = 0;
             profile_ptr < (1 << soc_mem_field_length(unit,
                        DLB_HGT_QUALITY_CONTROLm, profile_ptr_f));
             profile_ptr++) {
            base_index = profile_ptr * entries_per_profile;
            /* quality = (quantized_tx_load * tx_load_percent +
             *            quantized_qsize * (100 - tx_load_percent)) /
             *           100;
             * quality = ((quantized_tx_load - quantized_qsize) *
             *            tx_load_percent + quantized_qsize * 100) /
             *           100;
             * quality = (quantized_tx_load - quantized_qsize) *
             *           tx_load_percent / 100 + quantized_qsize;
             * tx_load_percent = (quality - quantized_qsize) * 100 /
             *                   (quantized_tx_load - quantized_qsize); 
             *
             * The index to DLB_HGT_QUALITY_MAPPING table consists of
             * base_index + quantized_tx_load * 8 + quantized_qsize,
             * where quantized_tx_load and quantized_qsize range from
             * 0 to 7.
             *
             * With quantized_tx_load = 7 and quantized_qsize = 0, the 
             * index to DLB_HGT_QUALITY_MAPPING table is
             * base_index + 56, and the tx_load_present expression
             * simplifies to quality * 100 / 7;
             */
            BCM_IF_ERROR_RETURN(READ_DLB_HGT_QUALITY_MAPPINGm(unit,
                        MEM_BLOCK_ANY, base_index + 56,
                        &quality_mapping_entry));
            quality = soc_DLB_HGT_QUALITY_MAPPINGm_field32_get(unit,
                    &port_quality_mapping_entry, quality_f);
            HG_DLB_INFO(unit)->hg_dlb_load_weight[profile_ptr] =
                quality * 100 / 7;
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        entries_per_profile = 64;
        for (profile_ptr = 0;
             profile_ptr < (1 << soc_reg_field_length(unit,
                        DLB_HGT_QUANTIZE_CONTROLr,
                        PORT_QUALITY_MAPPING_PROFILE_PTRf));
             profile_ptr++) {
            base_index = profile_ptr * entries_per_profile;
            /* quality = (quantized_tx_load * tx_load_percent +
             *            quantized_qsize * (100 - tx_load_percent)) / 100;
             * quality = ((quantized_tx_load - quantized_qsize) *
             *            tx_load_percent + quantized_qsize * 100) / 100;
             * quality = (quantized_tx_load - quantized_qsize) *
             *           tx_load_percent / 100 + quantized_qsize;
             * tx_load_percent = (quality - quantized_qsize) * 100 /
             *                   (quantized_tx_load - quantized_qsize); 
             *
             * The index to DLB_HGT_PORT_QUALITY_MAPPING table consists of
             * base_index + quantized_tx_load * 8 + quantized_qsize, where
             * quantized_tx_load and quantized_qsize range from 0 to 7.
             *
             * With quantized_tx_load = 7 and quantized_qsize = 0, the 
             * index to DLB_HGT_PORT_QUALITY_MAPPING table is
             * base_index + 56, and the tx_load_present expression
             * simplifies to quality * 100 / 7;
             */
            BCM_IF_ERROR_RETURN(READ_DLB_HGT_PORT_QUALITY_MAPPINGm(unit,
                        MEM_BLOCK_ANY, base_index + 56,
                        &port_quality_mapping_entry));
            quality = soc_DLB_HGT_PORT_QUALITY_MAPPINGm_field32_get(unit,
                    &port_quality_mapping_entry, ASSIGNED_QUALITYf);
            HG_DLB_INFO(unit)->hg_dlb_load_weight[profile_ptr] =
                quality * 100 / 7;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trident_hg_tg_size_field_forced
 * Purpose:
 *      Determine if the TG_SIZE field in HG_TRUNK_GROUP table is forced
 *      by hardware to be 0xf when RTAG field is less than or equal to 6.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_trident_hg_tg_size_field_forced(int unit)
{
    uint16 dev_id;
    uint8  rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    if ((dev_id == BCM56841_DEVICE_ID && rev_id >= BCM56841_B0_REV_ID) ||
        (dev_id == BCM56843_DEVICE_ID && rev_id >= BCM56843_B0_REV_ID) ||
        (dev_id == BCM56845_DEVICE_ID && rev_id >= BCM56845_B0_REV_ID)) {
        /* For these devices, the HG_TRUNK_GROUP.TG_SIZE field is forced
         * by harware to be 0xf when RTAG field is less than or equal to 6.
         */
       return TRUE;
    } else {
       return FALSE;
    } 
}

/*
 * Handle state recovery for fabric trunks
 */
int
_bcm_trident_trunk_fabric_reinit(int unit, int min_tid, int max_tid,
                          trunk_private_t *trunk_info)
{
    int rv = BCM_E_NONE;
    int tid, hgtid;
    hgt_dlb_control_entry_t hgt_dlb_control_entry;
    dlb_hgt_group_control_entry_t dlb_hgt_group_control_entry;
    hg_trunk_group_entry_t hg_tg_entry;
    hg_trunk_member_entry_t hg_trunk_member_entry;
    hg_trunk_bitmap_entry_t hg_trunk_bitmap_entry;
    int dlb_enable = 0, dlb_id;
    int entry_base_ptr, dynamic_size_encode, dlb_mode;
    int dynamic_size, dynamic_age;
    int block_base_ptr, num_blocks;
    int base_ptr, tg_size, rtag;
    bcm_port_t *hwport;
    int i;
    bcm_pbmp_t hg_trunk_pbmp;
    int hwf_rtag, hwf_psc;
    _trident_hw_tinfo_t *hwft;
    int num_entries;

    for (tid = min_tid; tid <= max_tid; tid++, trunk_info++) {

        hgtid = tid - min_tid;

        SOC_IF_ERROR_RETURN
            (READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                                  &hg_tg_entry));
        base_ptr = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_tg_entry,
                BASE_PTRf);
        if (_bcm_trident_hg_tg_size_field_forced(unit)) {
            tg_size = _bcm_trident_hw_failover_num_ports_get(unit, hgtid, TRUE);
            if (tg_size == 0) {
                /* Invalid Higig trunk group */
                continue;
            }
        } else {
            tg_size = 1 + soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_tg_entry,
                    TG_SIZEf);
            if ((0 == base_ptr) && (tg_size == 1)) {
                /* Invalid Higig trunk group */
                continue;
            }
        }

        rtag = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_tg_entry, RTAGf);

        /* Recover trunk info */
        trunk_info->tid = tid;
        trunk_info->in_use = TRUE;
        BCM_IF_ERROR_RETURN
            (_bcm_trident_trunk_rtag_to_psc(unit, rtag, &trunk_info->psc));
        trunk_info->rtag = rtag;
        /* trunk_info->flags already recovered from scache */

        /* Recover trunk member table bookkeeping info */
        num_entries = tg_size;
        if (rtag >= 1 && rtag <= 6) {
#ifdef BCM_KATANA_SUPPORT
            if (SOC_IS_KATANAX(unit)) {
                num_entries = _BCM_KATANA_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
            } else
#endif
            if (soc_feature(unit,
                        soc_feature_rtag1_6_max_portcnt_less_than_rtag7)) { 
                num_entries = _BCM_TD_FABRIC_TRUNK_RTAG1_6_MAX_PORTCNT;
            }
        } 
        _BCM_HG_TRUNK_MEMBER_USED_SET_RANGE(unit, base_ptr, num_entries);

        /* Recover Higig DLB group info */
        if (soc_feature(unit, soc_feature_hg_dlb)) {
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_hg_dlb_id_equal_hg_tid)) {
                SOC_IF_ERROR_RETURN(READ_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY,
                            hgtid, &dlb_hgt_group_control_entry));
                dlb_enable = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                        &dlb_hgt_group_control_entry, GROUP_ENABLEf);
                dlb_id = hgtid;
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                SOC_IF_ERROR_RETURN(READ_HGT_DLB_CONTROLm(unit, MEM_BLOCK_ANY,
                            hgtid, &hgt_dlb_control_entry));
                dlb_enable = soc_HGT_DLB_CONTROLm_field32_get(unit,
                        &hgt_dlb_control_entry, GROUP_ENABLEf);
                dlb_id = soc_HGT_DLB_CONTROLm_field32_get(unit,
                        &hgt_dlb_control_entry, DLB_IDf);
            }

            if (dlb_enable) {

                /* Mark Higig DLB group as used */
                _BCM_HG_DLB_ID_USED_SET(unit, dlb_id);

                SOC_IF_ERROR_RETURN(READ_DLB_HGT_GROUP_CONTROLm(unit,
                            MEM_BLOCK_ANY, dlb_id, &dlb_hgt_group_control_entry));

                /* Recover DLB mode */
#ifdef BCM_TRIUMPH3_SUPPORT
                if (soc_mem_field_valid(unit, DLB_HGT_GROUP_CONTROLm,
                            MEMBER_ASSIGNMENT_MODEf)) {
                    dlb_mode = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                            &dlb_hgt_group_control_entry, MEMBER_ASSIGNMENT_MODEf);
                } else
#endif /* BCM_TRIUMPH3_SUPPORT */
                {
                    dlb_mode = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                            &dlb_hgt_group_control_entry, PORT_ASSIGNMENT_MODEf);
                }
                switch (dlb_mode) {
                    case 0:
                        trunk_info->psc = BCM_TRUNK_PSC_DYNAMIC;
                        break;
                    case 1:
                        trunk_info->psc = BCM_TRUNK_PSC_DYNAMIC_ASSIGNED;
                        break;
                    case 2:
                        trunk_info->psc = BCM_TRUNK_PSC_DYNAMIC_OPTIMAL;
                        break;
                    default:
                        return BCM_E_INTERNAL;
                }

                /* Recover dynamic_size */
                dynamic_size_encode = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                        &dlb_hgt_group_control_entry, FLOW_SET_SIZEf);
                BCM_IF_ERROR_RETURN
                    (_bcm_trident_hg_dlb_dynamic_size_decode(dynamic_size_encode,
                                                             &dynamic_size));
                trunk_info->dynamic_size = dynamic_size;

                /* Recover dynamic_age */
                dynamic_age = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                        &dlb_hgt_group_control_entry, INACTIVITY_DURATIONf);
                trunk_info->dynamic_age = dynamic_age;

                /* Recover flow set table usage */
                entry_base_ptr = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                        &dlb_hgt_group_control_entry, FLOW_SET_BASEf);
                block_base_ptr = entry_base_ptr >> 9;
                num_blocks = trunk_info->dynamic_size >> 9; 
                _BCM_HG_DLB_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr,
                        num_blocks);
            } 
        }

#ifdef BCM_TRIDENT2_SUPPORT
        /* Recover Higig resilient hashing group info */
        if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
            BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_recover(unit, hgtid, trunk_info));
        }
#endif /* BCM_TRIDENT2_SUPPORT */

        /* Recover trunk member info */
        hwport = sal_alloc(sizeof(bcm_port_t) * tg_size, "hwport");
        if (hwport == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(hwport, 0, sizeof(bcm_port_t) * tg_size); 
        for (i = 0; i < tg_size; i++) {
            rv = READ_HG_TRUNK_MEMBERm(unit, MEM_BLOCK_ANY, base_ptr + i,
                    &hg_trunk_member_entry);
            if (BCM_FAILURE(rv)) {
                sal_free(hwport);
                return rv;
            }
            hwport[i] = soc_HG_TRUNK_MEMBERm_field32_get
                (unit, &hg_trunk_member_entry, PORT_NUMf); 
        }

        if (MEMBER_INFO(unit, tid).modport) {
            sal_free(MEMBER_INFO(unit, tid).modport);
            MEMBER_INFO(unit, tid).modport = NULL;
        }
        if (MEMBER_INFO(unit, tid).member_flags) {
            sal_free(MEMBER_INFO(unit, tid).member_flags);
            MEMBER_INFO(unit, tid).member_flags = NULL;
        }
        MEMBER_INFO(unit, tid).num_ports = tg_size;
        MEMBER_INFO(unit, tid).modport = sal_alloc(
                sizeof(uint16) * MEMBER_INFO(unit, tid).num_ports,
                "member info modport");
        if (NULL == MEMBER_INFO(unit, tid).modport) {
            sal_free(hwport);
            return BCM_E_MEMORY;
        }
        MEMBER_INFO(unit, tid).member_flags = sal_alloc(
                sizeof(uint32) * MEMBER_INFO(unit, tid).num_ports,
                "member info flags");
        if (NULL == MEMBER_INFO(unit, tid).member_flags) {
            sal_free(hwport);
            sal_free(MEMBER_INFO(unit, tid).modport);
            MEMBER_INFO(unit, tid).modport = NULL;
            return BCM_E_MEMORY;
        }
        for (i = 0; i < tg_size; i++) {
            MEMBER_INFO(unit, tid).modport[i] = hwport[i];
            MEMBER_INFO(unit, tid).member_flags[i] = 0;
        }

        /* Recover software trunk failover state */
        rv = READ_HG_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, hgtid,
                                   &hg_trunk_bitmap_entry);
        if (BCM_FAILURE(rv)) {
            sal_free(hwport);
            return rv;
        }
        soc_mem_pbmp_field_get(unit, HG_TRUNK_BITMAPm, &hg_trunk_bitmap_entry,
            HIGIG_TRUNK_BITMAPf, &hg_trunk_pbmp);
        rv = _bcm_trident_trunk_swfailover_fabric_set(unit, hgtid,
                trunk_info->rtag, hg_trunk_pbmp);
        if (BCM_FAILURE(rv)) {
            sal_free(hwport);
            return rv;
        }

        /* Recover hardware trunk failover state. */
        if (soc_feature(unit, soc_feature_hg_trunk_failover)) {
            hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid]);
            hwft->num_ports = tg_size;
            if (hwft->modport == NULL) {
                hwft->modport = sal_alloc(sizeof(uint16) * tg_size,
                        "hw_tinfo_modport");
                if (hwft->modport == NULL) {
                    sal_free(hwport);
                    return BCM_E_MEMORY;
                }
            }
            sal_memset(hwft->modport, 0, sizeof(uint16) * tg_size);

            if (hwft->psc == NULL) {
                hwft->psc = sal_alloc(sizeof(uint8) * tg_size,
                        "hw_tinfo_psc");
                if (hwft->psc == NULL) {
                    sal_free(hwft->modport);
                    hwft->modport = NULL;
                    sal_free(hwport);
                    return BCM_E_MEMORY;
                }
            }
            sal_memset(hwft->psc, 0, sizeof(uint8) * tg_size);

            if (hwft->flags == NULL) {
                /* If flags are not recovered from scache, allocate flags and
                 * set them to zero.
                 */
                hwft->flags = sal_alloc(sizeof(uint32) * tg_size,
                        "hw_tinfo_flags");
                if (hwft->flags == NULL) {
                    sal_free(hwft->modport);
                    hwft->modport = NULL;
                    sal_free(hwft->psc);
                    hwft->psc = NULL;
                    sal_free(hwport);
                    return BCM_E_MEMORY;
                }
                sal_memset(hwft->flags, 0, sizeof(uint32) * tg_size);
            }

            for (i = 0; i < tg_size; i++) {
                hwft->modport[i] = hwport[i];
                rv = _bcm_trident_trunk_hwfailover_hg_read(unit, hwport[i],
                        0, &hwf_rtag, NULL, NULL, NULL);
                if (BCM_FAILURE(rv)) {
                    sal_free(hwport);
                    return rv;
                }
                if (hwf_rtag != 0) {
                    rv = _bcm_trident_trunk_rtag_to_psc(unit, hwf_rtag,
                            &hwf_psc);
                    if (BCM_FAILURE(rv)) {
                        sal_free(hwport);
                        return rv;
                    }
                    hwft->psc[i] = hwf_psc;
                } else {
                    /* Use trunk group value as the default */
                    hwft->psc[i] = trunk_info->psc;
                }
            }
        }

        sal_free(hwport);
    }

    if (soc_feature(unit, soc_feature_hg_dlb)) {
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
            BCM_IF_ERROR_RETURN(_bcm_tr3_hg_dlb_member_recover(unit));
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        BCM_IF_ERROR_RETURN
            (_bcm_trident_hg_dlb_quality_parameters_recover(unit));
    }

    return rv;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP

/*
 * Function:
 *     _bcm_trident_hg_dlb_sw_dump
 * Purpose:
 *     Displays Higig DLB information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
STATIC void
_bcm_trident_hg_dlb_sw_dump(int unit)
{
    int i;
    soc_mem_t flowset_mem, quality_mapping_mem;
    int num_entries_per_profile;
    int num_profiles;
    int rv;
    int ref_count;

    soc_cm_print("Higig DLB Info -\n");

    /* Print Higig DLB group usage */
    soc_cm_print("    Higig DLB Groups Used:");
    for (i = 0; i < soc_mem_index_count(unit, DLB_HGT_GROUP_CONTROLm); i++) {
        if (_BCM_HG_DLB_ID_USED_GET(unit, i)) {
            soc_cm_print(" %d", i);
        }
    }
    soc_cm_print("\n");

    /* Print Higig DLB flowset table usage */
    soc_cm_print("    Higig DLB Flowset Table Blocks Used:");
    flowset_mem = soc_mem_is_valid(unit, DLB_HGT_FLOWSETm) ?
        DLB_HGT_FLOWSETm : DLB_HGT_FLOWSET_PORTm;
    for (i = 0; i < (soc_mem_index_count(unit, flowset_mem) >> 9); i++) {
        if (_BCM_HG_DLB_FLOWSET_BLOCK_USED_GET(unit, i)) {
            soc_cm_print(" %d", i);
        }
    }
    soc_cm_print("\n");

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_hg_dlb_member_id)) {
        /* Print Higig DLB member ID usage */
        soc_cm_print("    Higig DLB Member IDs Used:");
        for (i = 0; i < soc_mem_index_count(unit, DLB_HGT_MEMBER_ATTRIBUTEm); i++) {
            if (_BCM_HG_DLB_MEMBER_ID_USED_GET(unit, i)) {
                soc_cm_print(" %d", i);
            }
        }
        soc_cm_print("\n");
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    /* Print sample rate and threshold parameters */
    soc_cm_print("    Sample rate: %d per second\n",
            HG_DLB_INFO(unit)->hg_dlb_sample_rate);
    soc_cm_print("    Tx load min threshold: %d mbps\n",
            HG_DLB_INFO(unit)->hg_dlb_tx_load_min_th);
    soc_cm_print("    Tx load max threshold: %d mbps\n",
            HG_DLB_INFO(unit)->hg_dlb_tx_load_max_th);
    soc_cm_print("    Queue size min threshold: %d cells\n",
            HG_DLB_INFO(unit)->hg_dlb_qsize_min_th);
    soc_cm_print("    Queue size max threshold: %d cells\n",
            HG_DLB_INFO(unit)->hg_dlb_qsize_max_th);

    /* Print quality mapping profiles */
    soc_cm_print("    Quality mapping profiles:\n");
    num_entries_per_profile = 64;
    quality_mapping_mem = soc_mem_is_valid(unit, DLB_HGT_QUALITY_MAPPINGm) ?
        DLB_HGT_QUALITY_MAPPINGm : DLB_HGT_PORT_QUALITY_MAPPINGm;
    num_profiles = soc_mem_index_count(unit, quality_mapping_mem) /
        num_entries_per_profile;
    for (i = 0; i < num_profiles; i++) {
        soc_cm_print("      Profile %d: load weight %d percent, ",
                i, HG_DLB_INFO(unit)->hg_dlb_load_weight[i]);
        rv = soc_profile_mem_ref_count_get(unit, 
                HG_DLB_INFO(unit)->hg_dlb_quality_map_profile,
                i * num_entries_per_profile, &ref_count);
        if (SOC_FAILURE(rv)) {
            soc_cm_print("\n");
            continue;
        }
        soc_cm_print("ref count %d\n", ref_count);
    }

    return;
}

/*
 * Function:
 *     _bcm_trident_trunk_sw_dump
 * Purpose:
 *     Displays trunk information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_trident_trunk_sw_dump(int unit)
{
    int rv, i, j, tid_ix, tid_min, tid_max;
    bcm_trunk_chip_info_t   chip_info;
    _trident_trunk_swfail_t *swf;
    _trident_tinfo_t        *swft;
    int num_profiles;
    int ref_count;

    rv = bcm_esw_trunk_chip_info_get(unit, &chip_info);
    if (BCM_FAILURE(rv)) {
        soc_cm_print("Unable to retrieve unit %d chip info %d\n",
                     unit, rv);
        return;
    }

    if (chip_info.trunk_id_min < 0) {
        if (chip_info.trunk_fabric_id_min < 0) {
            /* No trunks!  Abort */
            soc_cm_print("No trunk groups on unit %d\n", unit);
            return;
        } else {
            tid_min = chip_info.trunk_fabric_id_min;
            tid_max = chip_info.trunk_fabric_id_max;
        }
    } else {
        tid_min = chip_info.trunk_id_min;
        if (chip_info.trunk_fabric_id_min < 0) {
            tid_max = chip_info.trunk_id_max;
        } else {
            tid_max = chip_info.trunk_fabric_id_max;
        }
    }

    /* Print software trunk failover states */

    swf = _trident_trunk_swfail[unit];
    if (NULL != swf) {
        soc_cm_print("Software failover parameters:\n");
        soc_cm_print("   Local modid:  %d\n", swf->mymodid);
        soc_cm_print("   Port:  Trunk\n");
        for (j = 0; j < SOC_MAX_NUM_PORTS; j++) {
            if (0 != swf->trunk[j]) {
                soc_cm_print("   %d:  %d\n",
                             j, swf->trunk[j]-1);
            }
        }
        soc_cm_print("   Trunk SW failover parameters\n");
        for (i = tid_min; i <= tid_max; i++) {
            if (i >= chip_info.trunk_fabric_id_min) {
                tid_ix = (i - chip_info.trunk_fabric_id_min) +
                    chip_info.trunk_group_count;
            } else {
                tid_ix = i;
            }
            swft = &swf->tinfo[tid_ix];
            if (0 != swft->num_ports) {
                soc_cm_print("   Trunk %d, index %d\n", i, tid_ix);
                soc_cm_print("      Flags value 0x%x\n", swft->flags);
                soc_cm_print("      RTAG value %d\n", swft->rtag);
                soc_cm_print("      Port count %d\n", swft->num_ports);
                for (j = 0; j < swft->num_ports; j++) {
                    soc_cm_print("      ModPort 0x%04x\n", swft->modport[j]);
                }
            }
        }
    } else {
        soc_cm_print("SW trunk failover not initialized on unit %d\n",
                     unit);
    }

    /* Print hardware trunk failover states */
    if (soc_feature(unit, soc_feature_port_lag_failover) ||
        soc_feature(unit, soc_feature_hg_trunk_failover)) {
        _trident_hw_tinfo_t *hwft;

        if (NULL == _trident_trunk_hwfail[unit]) {
            soc_cm_print("HW trunk failover not initialized on unit %d\n",
                         unit);
            return;
        }
        soc_cm_print("Trunk HW failover parameters\n");
        for (i = tid_min; i <= tid_max; i++) {
            if (i >= chip_info.trunk_fabric_id_min) {
                tid_ix = (i - chip_info.trunk_fabric_id_min) +
                    chip_info.trunk_group_count;
            } else {
                tid_ix = i;
            }
            hwft = &(_trident_trunk_hwfail[unit]->hw_tinfo[tid_ix]);
            if (0 != hwft->num_ports) {
                soc_cm_print("   Trunk %d, index %d\n", i, tid_ix);
                soc_cm_print("      Port count %d\n", hwft->num_ports);
                for (j = 0; j < hwft->num_ports; j++) {
                    soc_cm_print("      PSC value %d\n", hwft->psc[j]);
                    soc_cm_print("      Flags 0x%08x\n", hwft->flags[j]);
                    soc_cm_print("      ModPort 0x%04x\n", hwft->modport[j]);
                }
            }
        }
    }

    /* Print Higig trunk override profile info */
    soc_cm_print("Higig trunk override profile info\n");
    num_profiles = soc_mem_index_count(unit, ING_HIGIG_TRUNK_OVERRIDE_PROFILEm);
    for (i = 0; i < num_profiles; i++) {
        rv = soc_profile_mem_ref_count_get(unit,
                _trident_hg_trunk_override_profile[unit],
                i, &ref_count);
        if (SOC_FAILURE(rv)) {
            continue;
        }
        if (ref_count > 0) {
            soc_cm_print("   Higig trunk override profile %d ref count %d\n",
                    i, ref_count);
        }
    }

    /* Print trunk membership info */
    if (NULL != _trident_trunk_member_info[unit]) {
        soc_cm_print("Trunk membership info:\n");
        for (i = tid_min; i <= tid_max; i++) {
            if (i >= chip_info.trunk_fabric_id_min) {
                tid_ix = (i - chip_info.trunk_fabric_id_min) +
                    chip_info.trunk_group_count;
            } else {
                tid_ix = i;
            }

            if (0 != MEMBER_INFO(unit, tid_ix).num_ports) {
                soc_cm_print("   Trunk %d, index %d\n", i, tid_ix);
                soc_cm_print("      Port count %d\n",
                        MEMBER_INFO(unit, tid_ix).num_ports);
                for (j = 0; j < MEMBER_INFO(unit, tid_ix).num_ports; j++) {
                    soc_cm_print("      ModPort 0x%04x\n",
                            MEMBER_INFO(unit, tid_ix).modport[j]);
                    soc_cm_print("      Member flags 0x%x\n",
                            MEMBER_INFO(unit, tid_ix).member_flags[j]);
                }
            }
        }
    } else {
        soc_cm_print("Trunk membership info not initialized on unit %d\n",
                     unit);
    }

    if (soc_feature(unit, soc_feature_hg_dlb)) {
        _bcm_trident_hg_dlb_sw_dump(unit);
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_lag_dlb)) {
        bcm_tr3_lag_dlb_sw_dump(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
        bcm_td2_hg_rh_sw_dump(unit);
    }
    if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
        bcm_td2_lag_rh_sw_dump(unit);
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else /* BCM_TRIDENT_SUPPORT */

int _trident_trunk_not_empty;

#endif /* BCM_TRIDENT_SUPPORT */
