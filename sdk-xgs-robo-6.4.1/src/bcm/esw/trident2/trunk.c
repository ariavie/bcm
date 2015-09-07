/*
 * $Id: trunk.c,v 1.10 Broadcom SDK $
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
 * File:        trunk.c
 * Purpose:     Trident2 resilient hashing and VP LAG implementation.
 *              VP LAG code is enclosed in INCLUDE_L3.
 */

#include <soc/defs.h>
#include <sal/core/libc.h>
#include <shared/bsl.h>
#ifdef BCM_TRIDENT2_SUPPORT

#include <soc/drv.h>
#include <soc/trident2.h>

#include <bcm/error.h>
#include <bcm/trunk.h>

#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/niv.h>
#include <bcm_int/esw/extender.h>
#include <bcm_int/esw/trident2.h>

/* Bookkeeping info for Higig resilient hashing */

typedef struct _td2_hg_rh_info_s {
    SHR_BITDCL *hg_rh_flowset_block_bitmap; /* Each block corresponds to
                                               64 entries */
    uint32 hg_rh_rand_seed; /* The seed for pseudo-random number generator */
} _td2_hg_rh_info_t;

STATIC _td2_hg_rh_info_t *_td2_hg_rh_info[BCM_MAX_NUM_UNITS];

#define _BCM_HG_RH_FLOWSET_BLOCK_USED_GET(_u_, _idx_) \
    SHR_BITGET(_td2_hg_rh_info[_u_]->hg_rh_flowset_block_bitmap, _idx_)
#define _BCM_HG_RH_FLOWSET_BLOCK_USED_SET(_u_, _idx_) \
    SHR_BITSET(_td2_hg_rh_info[_u_]->hg_rh_flowset_block_bitmap, _idx_)
#define _BCM_HG_RH_FLOWSET_BLOCK_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(_td2_hg_rh_info[_u_]->hg_rh_flowset_block_bitmap, _idx_)
#define _BCM_HG_RH_FLOWSET_BLOCK_USED_SET_RANGE(_u_, _idx_, _count_) \
    SHR_BITSET_RANGE(_td2_hg_rh_info[_u_]->hg_rh_flowset_block_bitmap, _idx_, _count_)
#define _BCM_HG_RH_FLOWSET_BLOCK_USED_CLR_RANGE(_u_, _idx_, _count_) \
    SHR_BITCLR_RANGE(_td2_hg_rh_info[_u_]->hg_rh_flowset_block_bitmap, _idx_, _count_)
#define _BCM_HG_RH_FLOWSET_BLOCK_TEST_RANGE(_u_, _idx_, _count_, _result_) \
    SHR_BITTEST_RANGE(_td2_hg_rh_info[_u_]->hg_rh_flowset_block_bitmap, _idx_, _count_, _result_)

/* Bookkeeping info for LAG resilient hashing */

typedef struct _td2_lag_rh_info_s {
    int num_lag_rh_flowset_blocks;
    SHR_BITDCL *lag_rh_flowset_block_bitmap; /* Each block corresponds to
                                                64 entries */
    uint32 lag_rh_rand_seed; /* The seed for pseudo-random number generator */
} _td2_lag_rh_info_t;

STATIC _td2_lag_rh_info_t *_td2_lag_rh_info[BCM_MAX_NUM_UNITS];

#define _BCM_LAG_RH_FLOWSET_BLOCK_USED_GET(_u_, _idx_) \
    SHR_BITGET(_td2_lag_rh_info[_u_]->lag_rh_flowset_block_bitmap, _idx_)
#define _BCM_LAG_RH_FLOWSET_BLOCK_USED_SET(_u_, _idx_) \
    SHR_BITSET(_td2_lag_rh_info[_u_]->lag_rh_flowset_block_bitmap, _idx_)
#define _BCM_LAG_RH_FLOWSET_BLOCK_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(_td2_lag_rh_info[_u_]->lag_rh_flowset_block_bitmap, _idx_)
#define _BCM_LAG_RH_FLOWSET_BLOCK_USED_SET_RANGE(_u_, _idx_, _count_) \
    SHR_BITSET_RANGE(_td2_lag_rh_info[_u_]->lag_rh_flowset_block_bitmap, _idx_, _count_)
#define _BCM_LAG_RH_FLOWSET_BLOCK_USED_CLR_RANGE(_u_, _idx_, _count_) \
    SHR_BITCLR_RANGE(_td2_lag_rh_info[_u_]->lag_rh_flowset_block_bitmap, _idx_, _count_)
#define _BCM_LAG_RH_FLOWSET_BLOCK_TEST_RANGE(_u_, _idx_, _count_, _result_) \
    SHR_BITTEST_RANGE(_td2_lag_rh_info[_u_]->lag_rh_flowset_block_bitmap, _idx_, _count_, _result_)

#ifdef INCLUDE_L3

/* VP LAG software state */

typedef struct _td2_vp_lag_group_s {
    int vp_id; /* The VP value allocated to VP LAG */
    int has_member; /* Indicates if VP LAG has members */
    int non_uc_index; /* Index of the non-unicast member */
} _td2_vp_lag_group_t;

typedef struct _td2_vp_lag_info_s {
    int max_vp_lags; /* Maximum number of VP LAGs */
    SHR_BITDCL *vp_lag_used_bitmap; /* Bitmap of used VP LAG IDs */
    SHR_BITDCL *vp_lag_egr_member_bitmap; /* Bitmap of used entries of
                                             EGR_VPLAG_MEMBER table */
    _td2_vp_lag_group_t *group_info; /* Array of VP LAG group info */
} _td2_vp_lag_info_t;

STATIC _td2_vp_lag_info_t *_td2_vp_lag_info[BCM_MAX_NUM_UNITS];

#define VP_LAG_INIT_CHECK(_u_) \
        if (NULL == _td2_vp_lag_info[_u_]) { return BCM_E_INIT; }

#define MAX_VP_LAGS(_u_) (_td2_vp_lag_info[_u_]->max_vp_lags)

#define VP_LAG_USED_GET(_u_, _idx_) \
    SHR_BITGET(_td2_vp_lag_info[_u_]->vp_lag_used_bitmap, _idx_)
#define VP_LAG_USED_SET(_u_, _idx_) \
    SHR_BITSET(_td2_vp_lag_info[_u_]->vp_lag_used_bitmap, _idx_)
#define VP_LAG_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(_td2_vp_lag_info[_u_]->vp_lag_used_bitmap, _idx_)
 
#define VP_LAG_EGR_MEMBER_USED_GET(_u_, _idx_) \
    SHR_BITGET(_td2_vp_lag_info[_u_]->vp_lag_egr_member_bitmap, _idx_)
#define VP_LAG_EGR_MEMBER_USED_SET(_u_, _idx_) \
    SHR_BITSET(_td2_vp_lag_info[_u_]->vp_lag_egr_member_bitmap, _idx_)
#define VP_LAG_EGR_MEMBER_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(_td2_vp_lag_info[_u_]->vp_lag_egr_member_bitmap, _idx_)
#define VP_LAG_EGR_MEMBER_USED_SET_RANGE(_u_, _idx_, _count_) \
    SHR_BITSET_RANGE(_td2_vp_lag_info[_u_]->vp_lag_egr_member_bitmap, _idx_, _count_)
#define VP_LAG_EGR_MEMBER_USED_CLR_RANGE(_u_, _idx_, _count_) \
    SHR_BITCLR_RANGE(_td2_vp_lag_info[_u_]->vp_lag_egr_member_bitmap, _idx_, _count_)
#define VP_LAG_EGR_MEMBER_TEST_RANGE(_u_, _idx_, _count_, _result_) \
    SHR_BITTEST_RANGE(_td2_vp_lag_info[_u_]->vp_lag_egr_member_bitmap, _idx_, _count_, _result_)

#define VP_LAG_GROUP_INFO(_u_, _idx_) (_td2_vp_lag_info[_u_]->group_info[_idx_])

#endif /* INCLUDE_L3 */

int
bcm_td2_trunk_init(int unit)
{

    uint32 flags = 0; 
    bcm_trunk_t tid;
    bcm_port_t port;
    bcm_gport_t gport;     
    uint16 dev_id;
    uint8 rev_id;
    bcm_trunk_info_t trunk_info;
    int member_count;
    bcm_trunk_member_t *member_array = NULL; 
    bcm_trunk_chip_info_t ta_info;
    int rv = BCM_E_NONE;

    soc_cm_get_id(unit, &dev_id, &rev_id);
    
    if (BCM56855_DEVICE_ID == dev_id) {
        SOC_IF_ERROR_RETURN
                (bcm_esw_trunk_chip_info_get(unit, &ta_info));            
          
        flags |= BCM_TRUNK_FLAG_WITH_ID;
        tid = ta_info.trunk_fabric_id_max;
        SOC_IF_ERROR_RETURN
                (bcm_esw_trunk_create(unit, flags, &tid));
        bcm_trunk_info_t_init(&trunk_info);
        trunk_info.dlf_index = BCM_TRUNK_UNSPEC_INDEX;
        trunk_info.mc_index = BCM_TRUNK_UNSPEC_INDEX;
        trunk_info.ipmc_index = BCM_TRUNK_UNSPEC_INDEX; 
        trunk_info.psc = BCM_TRUNK_PSC_SRCDSTMAC;

        member_count = 0;
       
        member_array = sal_alloc(sizeof(bcm_trunk_member_t) *
                        BCM_TRUNK_FABRIC_MAX_PORTCNT, "member array");   
        if (NULL == member_array) {
            return BCM_E_RESOURCE;
        }        

        for (port = SOC_TD2_56855_LOOPBACK_HIGIG_PORT_START; 
             port < SOC_TD2_56855_LOOPBACK_HIGIG_PORT_END + 1; port++) {         
            if (member_count >= BCM_TRUNK_FABRIC_MAX_PORTCNT) {
                sal_free(member_array);
                return BCM_E_PARAM;
            }
            bcm_trunk_member_t_init(&member_array[member_count]);
            BCM_GPORT_LOCAL_SET(gport, port);
            member_array[member_count].gport = gport;
            member_count++;
        } 

        rv = bcm_esw_trunk_set(unit, tid, &trunk_info, 
                               member_count, member_array);   
        sal_free(member_array);
    }

    return rv;
}


/*
 * Function:
 *      bcm_td2_hg_rh_deinit
 * Purpose:
 *      Deallocate Higig resilient hashing internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
void
bcm_td2_hg_rh_deinit(int unit)
{
    if (_td2_hg_rh_info[unit]) {
        if (_td2_hg_rh_info[unit]->hg_rh_flowset_block_bitmap) {
            sal_free(_td2_hg_rh_info[unit]->hg_rh_flowset_block_bitmap);
            _td2_hg_rh_info[unit]->hg_rh_flowset_block_bitmap = NULL;
        }
        sal_free(_td2_hg_rh_info[unit]);
        _td2_hg_rh_info[unit] = NULL;
    }
}

/*
 * Function:
 *      bcm_td2_hg_rh_init
 * Purpose:
 *      Initialize Higig resilient hashing internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_init(int unit)
{
    int num_blocks;

    if (_td2_hg_rh_info[unit] == NULL) {
        _td2_hg_rh_info[unit] = sal_alloc(sizeof(_td2_hg_rh_info_t),
                "_td2_hg_rh_info");
        if (_td2_hg_rh_info[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(_td2_hg_rh_info[unit], 0, sizeof(_td2_hg_rh_info_t));
    }

    /* Each bit in hg_rh_flowset_block_bitmap corresponds to a block of 64
     * resilient hashing flow set table entries.
     */
    num_blocks = soc_mem_index_count(unit, RH_HGT_FLOWSETm) / 64;
    if (_td2_hg_rh_info[unit]->hg_rh_flowset_block_bitmap == NULL) {
        _td2_hg_rh_info[unit]->hg_rh_flowset_block_bitmap = 
            sal_alloc(SHR_BITALLOCSIZE(num_blocks),
                    "hg_rh_flowset_block_bitmap");
        if (_td2_hg_rh_info[unit]->hg_rh_flowset_block_bitmap == NULL) {
            bcm_td2_hg_rh_deinit(unit);
            return BCM_E_MEMORY;
        }
        sal_memset(_td2_hg_rh_info[unit]->hg_rh_flowset_block_bitmap, 0,
                SHR_BITALLOCSIZE(num_blocks));
    }

    /* Set the seed for the pseudo-random number generator */
    _td2_hg_rh_info[unit]->hg_rh_rand_seed = sal_time_usecs();

    if (!SOC_WARM_BOOT(unit)) {
        /* Clear Higig resilient hashing group control table */
        SOC_IF_ERROR_RETURN
            (soc_mem_clear(unit, RH_HGT_GROUP_CONTROLm, MEM_BLOCK_ALL, 0));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_hg_rh_dynamic_size_encode
 * Purpose:
 *      Encode Higig trunk resilient hashing flow set size.
 * Parameters:
 *      dynamic_size - (IN) Number of flow sets.
 *      encoded_value - (OUT) Encoded value.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_dynamic_size_encode(int dynamic_size, int *encoded_value)
{   
    switch (dynamic_size) {
        case 64:
            *encoded_value = 1;
            break;
        case 128:
            *encoded_value = 2;
            break;
        case 256:
            *encoded_value = 3;
            break;
        case 512:
            *encoded_value = 4;
            break;
        case 1024:
            *encoded_value = 5;
            break;
        case 2048:
            *encoded_value = 6;
            break;
        case 4096:
            *encoded_value = 7;
            break;
        case 8192:
            *encoded_value = 8;
            break;
        case 16384:
            *encoded_value = 9;
            break;
        case 32768:
            *encoded_value = 10;
            break;
        case 65536:
            *encoded_value = 11;
            break;
        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_hg_rh_dynamic_size_decode
 * Purpose:
 *      Decode Higig trunk resilient hashing flow set size.
 * Parameters:
 *      encoded_value - (IN) Encoded value.
 *      dynamic_size - (OUT) Number of flow sets.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_hg_rh_dynamic_size_decode(int encoded_value, int *dynamic_size)
{   
    switch (encoded_value) {
        case 1:
            *dynamic_size = 64;
            break;
        case 2:
            *dynamic_size = 128;
            break;
        case 3:
            *dynamic_size = 256;
            break;
        case 4:
            *dynamic_size = 512;
            break;
        case 5:
            *dynamic_size = 1024;
            break;
        case 6:
            *dynamic_size = 2048;
            break;
        case 7:
            *dynamic_size = 4096;
            break;
        case 8:
            *dynamic_size = 8192;
            break;
        case 9:
            *dynamic_size = 16384;
            break;
        case 10:
            *dynamic_size = 32768;
            break;
        case 11:
            *dynamic_size = 65536;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_hg_rh_status_get
 * Purpose:
 *      Check if resilient hashing is enabled on any Higig trunk group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      rh_status - (OUT) Higig resilient hashing status
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_status_get(int unit, int *rh_status)
{
    bcm_trunk_chip_info_t chip_info;
    int i;
    int psc;
    int rv;

    *rh_status = FALSE;

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &chip_info));
    if (chip_info.trunk_fabric_id_min < 0) {
        /* There are no Higig trunk groups. */
        return BCM_E_NOT_FOUND;
    }

    for (i = chip_info.trunk_fabric_id_min;
            i <= chip_info.trunk_fabric_id_max; i++) {
        rv = bcm_esw_trunk_psc_get(unit, i, &psc);
        if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
            return rv;
        }
        if (psc == BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
            *rh_status = TRUE;
            break;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_hg_rh_free_resource
 * Purpose:
 *      Free resources for a Higig resilient hashing.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig group ID.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_free_resource(int unit, int hgtid)
{
    int rv = BCM_E_NONE;
    uint32 rval;
    rh_hgt_group_control_entry_t rh_hgt_group_control_entry;
    hg_trunk_group_entry_t hg_trunk_group_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int block_base_ptr;
    int num_blocks;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;

    SOC_IF_ERROR_RETURN(READ_ENHANCED_HASHING_CONTROLr(unit, &rval));
    if (0 == soc_reg_field_get(unit, ENHANCED_HASHING_CONTROLr, rval,
                RH_HGT_ENABLEf)) {
        /* Higig resilient hashing is not enabled chip-wide. */
        return BCM_E_NONE;
    }

    SOC_IF_ERROR_RETURN(READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                &hg_trunk_group_entry));
    if (0 == soc_HG_TRUNK_GROUPm_field32_get(unit,
                &hg_trunk_group_entry, ENHANCED_HASHING_ENABLEf)) {
        /* Either (1) resilient hashing is not enabled on this Higig trunk
         * group, or (2) resilient hahsing is enabled with no members.
         * In case (2), RH_HGT_GROUP_CONTROL.FLOW_SET_SIZE needs to be cleared. 
         */
        SOC_IF_ERROR_RETURN(WRITE_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ALL,
                    hgtid, soc_mem_entry_null(unit, RH_HGT_GROUP_CONTROLm)));
        return BCM_E_NONE;
    }

    /* Clear ENHANCED_HASHING_ENABLE in HG_TRUNK_GROUP */
    soc_HG_TRUNK_GROUPm_field32_set(unit, &hg_trunk_group_entry,
            ENHANCED_HASHING_ENABLEf, 0);
    SOC_IF_ERROR_RETURN(WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid,
                &hg_trunk_group_entry));

    /* Clear RH_HGT_GROUP_CONTROL entry */
    SOC_IF_ERROR_RETURN(READ_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY,
                hgtid, &rh_hgt_group_control_entry));
    entry_base_ptr = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
            &rh_hgt_group_control_entry, FLOW_SET_BASEf);
    flow_set_size = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
            &rh_hgt_group_control_entry, FLOW_SET_SIZEf);
    SOC_IF_ERROR_RETURN(WRITE_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ALL,
                hgtid, soc_mem_entry_null(unit, RH_HGT_GROUP_CONTROLm)));

    /* Clear flow set table entries */
    BCM_IF_ERROR_RETURN
        (_bcm_td2_hg_rh_dynamic_size_decode(flow_set_size, &num_entries));
    alloc_size = num_entries * sizeof(rh_hgt_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_HGT_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_write_range(unit, RH_HGT_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, buf_ptr);
        return rv;
    }
    soc_cm_sfree(unit, buf_ptr);

    /* Free flow set table resources */
    block_base_ptr = entry_base_ptr >> 6;
    num_blocks = num_entries >> 6; 
    _BCM_HG_RH_FLOWSET_BLOCK_USED_CLR_RANGE(unit, block_base_ptr, num_blocks);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_hg_rh_dynamic_size_set
 * Purpose:
 *      Set the dynamic size for a Higig resilient hashing group with
 *      no members, so it can be recovered from hardware during warm boot.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig group ID.
 *      dynamic_size - (IN) Number of flow sets.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_dynamic_size_set(int unit, int hgtid, int dynamic_size)
{
    hg_trunk_group_entry_t hg_trunk_group_entry;
    rh_hgt_group_control_entry_t rh_hgt_group_control_entry;
    int flow_set_size;

    SOC_IF_ERROR_RETURN(READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                &hg_trunk_group_entry));
    if (soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_trunk_group_entry,
            ENHANCED_HASHING_ENABLEf)) {
        /* Not expecting resilient hashing to be enabled */
        return BCM_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(READ_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY,
                hgtid, &rh_hgt_group_control_entry));
    BCM_IF_ERROR_RETURN
        (bcm_td2_hg_rh_dynamic_size_encode(dynamic_size, &flow_set_size));
    soc_RH_HGT_GROUP_CONTROLm_field32_set(unit, &rh_hgt_group_control_entry,
            FLOW_SET_SIZEf, flow_set_size);
    SOC_IF_ERROR_RETURN(WRITE_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ALL,
                hgtid, &rh_hgt_group_control_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_hg_rh_rand_get
 * Purpose:
 *      Get a random number between 0 and the given max value.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      rand_max - (IN) Maximum random number.
 *      rand_num - (OUT) Random number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      This procedure uses the pseudo-random number generator algorithm
 *      suggested by the C standard.
 */
STATIC int
_bcm_td2_hg_rh_rand_get(int unit, int rand_max, int *rand_num)
{
    int modulus;
    int rand_seed_shift;

    if (rand_max < 0) {
        return BCM_E_PARAM;
    }

    if (NULL == rand_num) {
        return BCM_E_PARAM;
    }

    /* Make sure the modulus does not exceed limit. For instance,
     * if the 32-bit rand_seed is shifted to the right by 16 bits before
     * the modulo operation, the modulus should not exceed 1 << 16.
     */
    modulus = rand_max + 1;
    rand_seed_shift = 16;
    if (modulus > (1 << (32 - rand_seed_shift))) {
        return BCM_E_PARAM;
    }

    _td2_hg_rh_info[unit]->hg_rh_rand_seed =
        _td2_hg_rh_info[unit]->hg_rh_rand_seed * 1103515245 + 12345;

    *rand_num = (_td2_hg_rh_info[unit]->hg_rh_rand_seed >> rand_seed_shift) %
                modulus;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_hg_rh_member_choose
 * Purpose:
 *      Choose a member of the Higig trunk group.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      num_members - (IN) Number of members to choose from.
 *      entry_count_arr   - (IN/OUT) An array keeping track of how many
 *                                   flow set entries have been assigned
 *                                   to each member.
 *      max_entry_count   - (IN/OUT) The maximum number of flow set entries
 *                                   that can be assigned to a member. 
 *      chosen_index      - (OUT) The index of the chosen member.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_hg_rh_member_choose(int unit, int num_members, int *entry_count_arr,
        int *max_entry_count, int *chosen_index)
{
    int member_index;
    int next_index;

    *chosen_index = 0;

    /* Choose a random member index */
    BCM_IF_ERROR_RETURN(_bcm_td2_hg_rh_rand_get(unit, num_members - 1,
                &member_index));

    if (entry_count_arr[member_index] < *max_entry_count) {
        entry_count_arr[member_index]++;
        *chosen_index = member_index;
    } else {
        /* The randomly chosen member has reached the maximum
         * flow set entry count. Choose the next member that
         * has not reached the maximum entry count.
         */
        next_index = (member_index + 1) % num_members;
        while (next_index != member_index) {
            if (entry_count_arr[next_index] < *max_entry_count) {
                entry_count_arr[next_index]++;
                *chosen_index = next_index;
                break;
            } else {
                next_index = (next_index + 1) % num_members;
            }
        }
        if (next_index == member_index) {
            /* All members have reached the maximum flow set entry
             * count. This scenario occurs when dividing the number of
             * flow set entries by the number of members results
             * in a non-zero remainder. The remainder flow set entries
             * will be distributed among members, at most 1 remainder
             * entry per member.
             */
            (*max_entry_count)++;
            if (entry_count_arr[member_index] < *max_entry_count) {
                entry_count_arr[member_index]++;
                *chosen_index = member_index;
            } else {
                /* It's possible that the member's entry count already equals
                 * to the incremented value of max_entry_count.
                 */
                next_index = (member_index + 1) % num_members;
                while (next_index != member_index) {
                    if (entry_count_arr[next_index] < *max_entry_count) {
                        entry_count_arr[next_index]++;
                        *chosen_index = next_index;
                        break;
                    } else {
                        next_index = (next_index + 1) % num_members;
                    }
                }
                if (next_index == member_index) {
                    return BCM_E_INTERNAL;
                }
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_hg_rh_set
 * Purpose:
 *      Configure a Higig resilient hashing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure, which contains
 *                      all resilient hashing eligible members of the group.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_set(int unit,
        int hgtid,
        _esw_trunk_add_info_t *add_info)
{
    int rv = BCM_E_NONE;
    int num_blocks;
    int total_blocks;
    int max_block_base_ptr;
    int block_base_ptr;
    int entry_base_ptr;
    SHR_BITDCL occupied;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int *entry_count_arr = NULL;
    int max_entry_count;
    int chosen_index;
    int i;
    rh_hgt_flowset_entry_t *flowset_entry;
    int index_min, index_max;
    rh_hgt_group_control_entry_t rh_hgt_group_control_entry;
    int flow_set_size;
    hg_trunk_group_entry_t hg_trunk_group_entry;

    if (add_info == NULL || add_info->psc != BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (add_info->num_ports == 0) {
        /* Write the dynamic_size to hardware, so it can be
         * recovered during warm boot.
         */
        BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_dynamic_size_set(unit,
                    hgtid, add_info->dynamic_size));

        return BCM_E_NONE;
    }

    /* Find a contiguous region of flow set table that's large
     * enough to hold the requested number of flow sets.
     * The flow set table is allocated in 64-entry blocks.
     */
    num_blocks = add_info->dynamic_size >> 6;
    total_blocks = soc_mem_index_count(unit, RH_HGT_FLOWSETm) >> 6;
    max_block_base_ptr = total_blocks - num_blocks;
    for (block_base_ptr = 0;
         block_base_ptr <= max_block_base_ptr;
         block_base_ptr++) {
        /* Check if the contiguous region of flow set table from
         * block_base_ptr to (block_base_ptr + num_blocks - 1) is free. 
         */
        _BCM_HG_RH_FLOWSET_BLOCK_TEST_RANGE(unit, block_base_ptr, num_blocks,
                occupied); 
        if (!occupied) {
            break;
        }
    }
    if (block_base_ptr > max_block_base_ptr) {
        /* A contiguous region of the desired size could not be found in
         * flow set table.
         */
        return BCM_E_RESOURCE;
    }

    /* Configure flow set table */
    alloc_size = add_info->dynamic_size * sizeof(rh_hgt_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_HGT_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);

    entry_count_arr = sal_alloc(sizeof(int) * add_info->num_ports,
            "RH entry count array");
    if (NULL == entry_count_arr) {
        soc_cm_sfree(unit, buf_ptr);
        return BCM_E_MEMORY;
    }
    sal_memset(entry_count_arr, 0, sizeof(int) * add_info->num_ports);
    max_entry_count = add_info->dynamic_size / add_info->num_ports;

    for (i = 0; i < add_info->dynamic_size; i++) {
        /* Choose a member of the Higig trunk group */
        rv = _bcm_td2_hg_rh_member_choose(unit, add_info->num_ports,
                entry_count_arr, &max_entry_count, &chosen_index);
        if (BCM_FAILURE(rv)) {
            soc_cm_sfree(unit, buf_ptr);
            sal_free(entry_count_arr);
            return rv;
        }

        /* Set flow set entry */
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_HGT_FLOWSETm, rh_hgt_flowset_entry_t *, buf_ptr, i);
        soc_mem_field32_set(unit, RH_HGT_FLOWSETm, flowset_entry, VALIDf, 1);
        soc_mem_field32_set(unit, RH_HGT_FLOWSETm, flowset_entry, EGRESS_PORTf,
                add_info->tp[chosen_index]);
    }

    entry_base_ptr = block_base_ptr << 6;
    index_min = entry_base_ptr;
    index_max = index_min + add_info->dynamic_size - 1;
    rv = soc_mem_write_range(unit, RH_HGT_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, buf_ptr);
        sal_free(entry_count_arr);
        return rv;
    }
    soc_cm_sfree(unit, buf_ptr);
    sal_free(entry_count_arr);

    _BCM_HG_RH_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr, num_blocks);

    /* Update Higig RH group control table */
    sal_memset(&rh_hgt_group_control_entry, 0,
            sizeof(rh_hgt_group_control_entry_t));
    soc_RH_HGT_GROUP_CONTROLm_field32_set(unit, &rh_hgt_group_control_entry,
            FLOW_SET_BASEf, entry_base_ptr);
    BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_dynamic_size_encode(
                add_info->dynamic_size, &flow_set_size));
    soc_RH_HGT_GROUP_CONTROLm_field32_set(unit, &rh_hgt_group_control_entry,
            FLOW_SET_SIZEf, flow_set_size);
    SOC_IF_ERROR_RETURN(WRITE_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ALL,
                hgtid, &rh_hgt_group_control_entry));

    /* Update HG_TRUNK_GROUP */
    SOC_IF_ERROR_RETURN(READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                &hg_trunk_group_entry));
    soc_HG_TRUNK_GROUPm_field32_set(unit, &hg_trunk_group_entry,
            ENHANCED_HASHING_ENABLEf, 1);
    SOC_IF_ERROR_RETURN(WRITE_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, hgtid,
                &hg_trunk_group_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_hg_rh_add
 * Purpose:
 *      Add a member to a Higig resilient hashing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure, which contains
 *                      all resilient hashing eligible members of the group,
 *                      including the member to be added.
 *      new_member - (IN) Member to add.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_add(int unit,
        int hgtid,
        _esw_trunk_add_info_t *add_info,
        bcm_trunk_member_t *new_member)
{
    int rv = BCM_E_NONE;
    bcm_port_t new_port;
    bcm_module_t new_mod;
    bcm_port_t tmp_port;
    int tmp_index = -1;
    int num_existing_members;
    int *member_index_arr = NULL;
    int i;
    rh_hgt_group_control_entry_t rh_hgt_group_control_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;
    int *entry_count_arr = NULL;
    rh_hgt_flowset_entry_t *flowset_entry;
    int egress_port;
    int member_index;
    int lower_bound, upper_bound;
    int threshold;
    int new_member_entry_count;
    int entry_index, next_entry_index;

    if (add_info == NULL ||
        add_info->psc != BCM_TRUNK_PSC_DYNAMIC_RESILIENT ||
        add_info->num_ports == 0) {
        return BCM_E_PARAM;
    }

    if (new_member == NULL) {
        return BCM_E_PARAM;
    }

    if (add_info->num_ports == 1) {
        /* Adding the first member is the same as setting one member */
       return bcm_td2_hg_rh_set(unit, hgtid, add_info);
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_gport_array_resolve(unit, TRUE, 1,
                &new_member->gport, &new_port, &new_mod));

    /* Swap new member to the last one for BCM_TRUNK_FLAG_MEMBER_SORT */
    if (add_info->flags & BCM_TRUNK_FLAG_MEMBER_SORT) {
        for (i = 0; i < add_info->num_ports; i++){
            if (i == (add_info->num_ports - 1)) {
                break;
            }
            if (new_port == add_info->tp[i]) {
                tmp_port = add_info->tp[i];
                add_info->tp[i] = add_info->tp[add_info->num_ports - 1];
                add_info->tp[add_info->num_ports - 1] = tmp_port;
                tmp_index = i;
                break;
            }    
        }
    }

    /* Check that the new member is the last member of the array */
    if (new_port != add_info->tp[add_info->num_ports - 1]) {
        return BCM_E_PARAM;
    }
    num_existing_members = add_info->num_ports - 1;

    /* Get member index for each existing member port */
    member_index_arr = sal_alloc(sizeof(int) * SOC_MAX_NUM_PORTS,
            "member index array");
    if (NULL == member_index_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    for (i = 0; i < SOC_MAX_NUM_PORTS; i++) {
        member_index_arr[i] = -1;
    }
    for (i = 0; i < num_existing_members; i++) {
        member_index_arr[add_info->tp[i]] = i;
    }

    /* Read all the flow set entries of this Higig resilient hash group,
     * and compute the number of entries currently assigned to each
     * existing member.
     */
    rv = READ_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY, hgtid,
            &rh_hgt_group_control_entry);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }
    entry_base_ptr = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
            &rh_hgt_group_control_entry, FLOW_SET_BASEf);
    flow_set_size = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
            &rh_hgt_group_control_entry, FLOW_SET_SIZEf);
    rv = _bcm_td2_hg_rh_dynamic_size_decode(flow_set_size, &num_entries);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_entries * sizeof(rh_hgt_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_HGT_FLOWSET entries");
    if (NULL == buf_ptr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_read_range(unit, RH_HGT_FLOWSETm, MEM_BLOCK_ANY,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_existing_members * sizeof(int);
    entry_count_arr = sal_alloc(alloc_size, "RH entry count array");
    if (NULL == entry_count_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(entry_count_arr, 0, alloc_size);

    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_HGT_FLOWSETm, rh_hgt_flowset_entry_t *, buf_ptr, i);
        if (!soc_mem_field32_get(unit, RH_HGT_FLOWSETm, flowset_entry,
                    VALIDf)) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        egress_port = soc_mem_field32_get(unit, RH_HGT_FLOWSETm, flowset_entry,
                EGRESS_PORTf);
        member_index = member_index_arr[egress_port];
        if (member_index == -1) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        entry_count_arr[member_index]++;
    }

    /* Check that the distribution of flow set entries among existing members
     * is balanced. For instance, if the number of flow set entries is 64, and
     * the number of existing members is 6, then every member should have
     * between 10 and 11 entries. 
     */ 
    lower_bound = num_entries / num_existing_members;
    upper_bound = (num_entries % num_existing_members) ?
                  (lower_bound + 1) : lower_bound;
    for (i = 0; i < num_existing_members; i++) {
        if (entry_count_arr[i] < lower_bound ||
                entry_count_arr[i] > upper_bound) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
    }

    /* Re-balance flow set entries from existing members to the new member.
     * For example, if the number of flow set entries is 64, and the number of
     * existing members is 6, then each existing member has between 10 and 11
     * entries. The entries should be re-assigned from the 6 existing members
     * to the new member such that each member will end up with between 9 and
     * 10 entries.
     */
    lower_bound = num_entries / add_info->num_ports;
    upper_bound = (num_entries % add_info->num_ports) ?
                  (lower_bound + 1) : lower_bound;
    threshold = upper_bound;
    new_member_entry_count = 0;
    while (new_member_entry_count < lower_bound) {
        /* Pick a random entry in the flow set buffer */
        rv = _bcm_td2_hg_rh_rand_get(unit, num_entries - 1, &entry_index);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        flowset_entry = soc_mem_table_idx_to_pointer(unit, RH_HGT_FLOWSETm,
                rh_hgt_flowset_entry_t *, buf_ptr, entry_index);
        egress_port = soc_mem_field32_get(unit, RH_HGT_FLOWSETm, flowset_entry,
                EGRESS_PORTf);
        member_index = member_index_arr[egress_port];
        if ((egress_port != new_port) &&
                (entry_count_arr[member_index] > threshold)) {
            soc_mem_field32_set(unit, RH_HGT_FLOWSETm, flowset_entry,
                    EGRESS_PORTf, new_port);
            entry_count_arr[member_index]--;
            new_member_entry_count++;
        } else {
            /* Either the member of the randomly chosen entry is
             * the same as the new member, or the member is an existing
             * member and its entry count has decreased to threshold.
             * In both cases, find the next entry that contains a
             * member that's not the new member and whose entry count
             * has not decreased to threshold.
             */
            next_entry_index = (entry_index + 1) % num_entries;
            while (next_entry_index != entry_index) {
                flowset_entry = soc_mem_table_idx_to_pointer(unit,
                        RH_HGT_FLOWSETm, rh_hgt_flowset_entry_t *, buf_ptr,
                        next_entry_index);
                egress_port = soc_mem_field32_get(unit, RH_HGT_FLOWSETm,
                        flowset_entry, EGRESS_PORTf);
                member_index = member_index_arr[egress_port];
                if ((egress_port != new_port) &&
                        (entry_count_arr[member_index] > threshold)) {
                    soc_mem_field32_set(unit, RH_HGT_FLOWSETm, flowset_entry,
                            EGRESS_PORTf, new_port);
                    entry_count_arr[member_index]--;
                    new_member_entry_count++;
                    break;
                } else {
                    next_entry_index = (next_entry_index + 1) % num_entries;
                }
            }
            if (next_entry_index == entry_index) {
                /* The entry count of all existing members has decreased
                 * to threshold. The entry count of the new member has
                 * not yet increased to lower_bound. Lower the threshold.
                 */
                threshold--;
            }
        }
    }

    /* Write re-balanced flow set entries to hardware */
    rv = soc_mem_write_range(unit, RH_HGT_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

cleanup:
    if (member_index_arr) {
        sal_free(member_index_arr);
    }
    if (buf_ptr) {
        soc_cm_sfree(unit, buf_ptr);
    }
    if (entry_count_arr) {
        sal_free(entry_count_arr);
    }

    /* Restore the order of trunk members */ 
    if (tmp_index != -1) {
        tmp_port = add_info->tp[tmp_index];
        add_info->tp[tmp_index] = add_info->tp[add_info->num_ports - 1];
        add_info->tp[add_info->num_ports - 1] = tmp_port;
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_hg_rh_delete
 * Purpose:
 *      Delete a member from a Higig resilient hashing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      hgtid - (IN) Higig trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure, which contains
 *                      all resilient hashing eligible members of the group,
 *                      except the member to be deleted.
 *      leaving_member - (IN) Member to delete.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_delete(int unit,
        int hgtid,
        _esw_trunk_add_info_t *add_info,
        bcm_trunk_member_t *leaving_member)
{
    int rv = BCM_E_NONE;
    bcm_port_t leaving_port;
    bcm_module_t leaving_mod;
    int num_remaining_members;
    int *member_index_arr = NULL;
    int i;
    rh_hgt_group_control_entry_t rh_hgt_group_control_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;
    int leaving_member_entry_count;
    int *entry_count_arr = NULL;
    rh_hgt_flowset_entry_t *flowset_entry;
    int egress_port;
    int member_index;
    int lower_bound, upper_bound;
    int threshold;
    int chosen_index;

    if (add_info == NULL || add_info->psc != BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (leaving_member == NULL) {
        return BCM_E_PARAM;
    }

    if (add_info->num_ports == 0) {
        /* Deleting the last member is the same as freeing all resources */
        BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_free_resource(unit, hgtid));

        /* Write the dynamic_size to hardware, so it can be recovered
         * during warm boot.
         */
        BCM_IF_ERROR_RETURN(bcm_td2_hg_rh_dynamic_size_set(unit, hgtid,
                    add_info->dynamic_size));

        return BCM_E_NONE;
    }

    /* Check that the leaving member is not a member of the array */
    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_gport_array_resolve(unit, TRUE, 1,
                &leaving_member->gport, &leaving_port, &leaving_mod));
    for (i = 0; i < add_info->num_ports; i++) {
        if (leaving_port == add_info->tp[i]) {
            return BCM_E_PARAM;
        }
    }
    num_remaining_members = add_info->num_ports;

    /* Get member index for each remaining member port */
    member_index_arr = sal_alloc(sizeof(int) * SOC_MAX_NUM_PORTS,
            "member index array");
    if (NULL == member_index_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    for (i = 0; i < SOC_MAX_NUM_PORTS; i++) {
        member_index_arr[i] = -1;
    }
    for (i = 0; i < num_remaining_members; i++) {
        member_index_arr[add_info->tp[i]] = i;
    }

    /* Read all the flow set entries of this Higig resilient hash group,
     * and compute the number of entries currently assigned to each
     * member, including the leaving member.
     */
    rv = READ_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY, hgtid,
            &rh_hgt_group_control_entry);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }
    entry_base_ptr = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
            &rh_hgt_group_control_entry, FLOW_SET_BASEf);
    flow_set_size = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
            &rh_hgt_group_control_entry, FLOW_SET_SIZEf);
    rv = _bcm_td2_hg_rh_dynamic_size_decode(flow_set_size, &num_entries);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_entries * sizeof(rh_hgt_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_HGT_FLOWSET entries");
    if (NULL == buf_ptr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_read_range(unit, RH_HGT_FLOWSETm, MEM_BLOCK_ANY,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_remaining_members * sizeof(int);
    entry_count_arr = sal_alloc(alloc_size, "RH entry count array");
    if (NULL == entry_count_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(entry_count_arr, 0, alloc_size);

    leaving_member_entry_count = 0;
    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_HGT_FLOWSETm, rh_hgt_flowset_entry_t *, buf_ptr, i);
        if (!soc_mem_field32_get(unit, RH_HGT_FLOWSETm, flowset_entry,
                    VALIDf)) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        egress_port = soc_mem_field32_get(unit, RH_HGT_FLOWSETm, flowset_entry,
                EGRESS_PORTf);
        if (egress_port == leaving_port) {
            leaving_member_entry_count++;
        } else {
            member_index = member_index_arr[egress_port];
            if (member_index == -1) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            entry_count_arr[member_index]++;
        }
    }

    /* Check that the distribution of flow set entries among all members
     * is balanced. For instance, if the number of flow set entries is 64, and
     * the number of members is 6, then every member should have
     * between 10 and 11 entries. 
     */ 
    lower_bound = num_entries / (num_remaining_members + 1);
    upper_bound = (num_entries % (num_remaining_members + 1)) ?
                  (lower_bound + 1) : lower_bound;
    for (i = 0; i < num_remaining_members; i++) {
        if (entry_count_arr[i] < lower_bound ||
                entry_count_arr[i] > upper_bound) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
    }
    if (leaving_member_entry_count < lower_bound ||
            leaving_member_entry_count > upper_bound) {
        rv = BCM_E_INTERNAL;
        goto cleanup;
    }

    /* Re-balance flow set entries from the leaving member to the remaining
     * members. For example, if the number of flow set entries is 64, and
     * the number of members is 6, then each member has between 10 and 11
     * entries. The entries should be re-assigned from the leaving member to
     * the remaining 5 members such that each remaining member will end up
     * with between 12 and 13 entries.
     */
    lower_bound = num_entries / num_remaining_members;
    upper_bound = (num_entries % num_remaining_members) ?
                  (lower_bound + 1) : lower_bound;
    threshold = lower_bound;
    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_HGT_FLOWSETm, rh_hgt_flowset_entry_t *, buf_ptr, i);
        egress_port = soc_mem_field32_get(unit, RH_HGT_FLOWSETm, flowset_entry,
                EGRESS_PORTf);
        if (egress_port != leaving_port) {
            continue;
        }

        /* Randomly choose a member among the remaining members */
        rv = _bcm_td2_hg_rh_member_choose(unit, num_remaining_members,
                entry_count_arr, &threshold, &chosen_index);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        soc_mem_field32_set(unit, RH_HGT_FLOWSETm, flowset_entry, EGRESS_PORTf,
                add_info->tp[chosen_index]);
    }

    /* Write re-balanced flow set entries to hardware */
    rv = soc_mem_write_range(unit, RH_HGT_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

cleanup:
    if (member_index_arr) {
        sal_free(member_index_arr);
    }
    if (buf_ptr) {
        soc_cm_sfree(unit, buf_ptr);
    }
    if (entry_count_arr) {
        sal_free(entry_count_arr);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_hg_rh_ethertype_set
 * Purpose:
 *      Set the Ethertypes that are eligible or ineligible for
 *      Higig resilient hashing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) BCM_TRUNK_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_count - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (IN) Array of Ethertypes.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_td2_hg_rh_ethertype_set(
    int unit, 
    uint32 flags, 
    int ethertype_count, 
    int *ethertype_array)
{
    uint32 control_reg;
    int i, j;
    rh_hgt_ethertype_eligibility_map_entry_t ethertype_entry;

    /* Input check */
    if (ethertype_count > soc_mem_index_count(unit,
                RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm)) {
        return BCM_E_RESOURCE;
    }

    /* Update ethertype eligibility control register */
    SOC_IF_ERROR_RETURN
        (READ_RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &control_reg));
    soc_reg_field_set(unit, RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr,
            &control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf,
            flags & BCM_TRUNK_DYNAMIC_ETHERTYPE_ELIGIBLE ? 1 : 0);
    soc_reg_field_set(unit, RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr,
            &control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf,
            flags & BCM_TRUNK_DYNAMIC_ETHERTYPE_INNER ? 1 : 0);
    SOC_IF_ERROR_RETURN
        (WRITE_RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, control_reg));

    /* Update Ethertype eligibility map table */
    for (i = 0; i < ethertype_count; i++) {
        sal_memset(&ethertype_entry, 0,
                sizeof(rh_hgt_ethertype_eligibility_map_entry_t));
        soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, VALIDf, 1);
        soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, ETHERTYPEf, ethertype_array[i]);
        SOC_IF_ERROR_RETURN(WRITE_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, i, &ethertype_entry));
    }

    /* Zero out remaining entries of Ethertype eligibility map table */
    for (j = i; j < soc_mem_index_count(unit,
                RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm); j++) {
        SOC_IF_ERROR_RETURN(WRITE_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, j, soc_mem_entry_null(unit,
                        RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm)));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_hg_rh_ethertype_get
 * Purpose:
 *      Get the Ethertypes that are eligible or ineligible for
 *      Higig resilient hashing.
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
bcm_td2_hg_rh_ethertype_get(
    int unit, 
    uint32 *flags, 
    int ethertype_max, 
    int *ethertype_array, 
    int *ethertype_count)
{
    uint32 control_reg;
    int i;
    int ethertype;
    rh_hgt_ethertype_eligibility_map_entry_t ethertype_entry;

    *ethertype_count = 0;

    /* Get flags */
    SOC_IF_ERROR_RETURN
        (READ_RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &control_reg));
    if (soc_reg_field_get(unit, RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr,
                control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf)) {
        *flags |= BCM_TRUNK_DYNAMIC_ETHERTYPE_ELIGIBLE;
    }
    if (soc_reg_field_get(unit, RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr,
                control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf)) {
        *flags |= BCM_TRUNK_DYNAMIC_ETHERTYPE_INNER;
    }

    /* Get Ethertypes */
    for (i = 0; i < soc_mem_index_count(unit,
                RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm); i++) {
        SOC_IF_ERROR_RETURN(READ_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ANY, i, &ethertype_entry));
        if (soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                    &ethertype_entry, VALIDf)) {
            ethertype = soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
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

/*
 * Function:
 *      bcm_td2_lag_rh_deinit
 * Purpose:
 *      Deallocate LAG resilient hashing internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
void
bcm_td2_lag_rh_deinit(int unit)
{
    if (_td2_lag_rh_info[unit]) {
        if (_td2_lag_rh_info[unit]->lag_rh_flowset_block_bitmap) {
            sal_free(_td2_lag_rh_info[unit]->lag_rh_flowset_block_bitmap);
            _td2_lag_rh_info[unit]->lag_rh_flowset_block_bitmap = NULL;
        }
        sal_free(_td2_lag_rh_info[unit]);
        _td2_lag_rh_info[unit] = NULL;
    }
}

/*
 * Function:
 *      bcm_td2_lag_rh_init
 * Purpose:
 *      Initialize LAG resilient hashing internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_init(int unit)
{
    int num_lag_rh_flowset_entries;

    if (_td2_lag_rh_info[unit] == NULL) {
        _td2_lag_rh_info[unit] = sal_alloc(sizeof(_td2_lag_rh_info_t),
                "_td2_lag_rh_info");
        if (_td2_lag_rh_info[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(_td2_lag_rh_info[unit], 0, sizeof(_td2_lag_rh_info_t));
    }

    /* The LAG and ECMP resilient hash features share the same flow set table.
     * The table can be configured in one of 3 modes:
     * - dedicated to LAG resilient hashing,
     * - dedicated to ECMP resilient hashing,
     * - split evenly between LAG and ECMP resilient hashing.
     * The mode was configured during Trident2 initialization.
     */
    num_lag_rh_flowset_entries = soc_mem_index_count(unit, RH_LAG_FLOWSETm);

    /* Each bit in lag_rh_flowset_block_bitmap corresponds to a block of 64
     * resilient hashing flow set table entries.
     */
    _td2_lag_rh_info[unit]->num_lag_rh_flowset_blocks =
        num_lag_rh_flowset_entries / 64;
    if (_td2_lag_rh_info[unit]->num_lag_rh_flowset_blocks > 0) {
        if (_td2_lag_rh_info[unit]->lag_rh_flowset_block_bitmap == NULL) {
            _td2_lag_rh_info[unit]->lag_rh_flowset_block_bitmap = 
                sal_alloc(SHR_BITALLOCSIZE(_td2_lag_rh_info[unit]->
                            num_lag_rh_flowset_blocks),
                        "lag_rh_flowset_block_bitmap");
            if (_td2_lag_rh_info[unit]->lag_rh_flowset_block_bitmap == NULL) {
                bcm_td2_lag_rh_deinit(unit);
                return BCM_E_MEMORY;
            }
            sal_memset(_td2_lag_rh_info[unit]->lag_rh_flowset_block_bitmap, 0,
                    SHR_BITALLOCSIZE(_td2_lag_rh_info[unit]->
                        num_lag_rh_flowset_blocks));
        }
    }

    /* Set the seed for the pseudo-random number generator */
    _td2_lag_rh_info[unit]->lag_rh_rand_seed = sal_time_usecs();

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_lag_rh_dynamic_size_encode
 * Purpose:
 *      Encode LAG trunk resilient hashing flow set size.
 * Parameters:
 *      dynamic_size - (IN) Number of flow sets.
 *      encoded_value - (OUT) Encoded value.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_dynamic_size_encode(int dynamic_size, int *encoded_value)
{   
    switch (dynamic_size) {
        case 64:
            *encoded_value = 1;
            break;
        case 128:
            *encoded_value = 2;
            break;
        case 256:
            *encoded_value = 3;
            break;
        case 512:
            *encoded_value = 4;
            break;
        case 1024:
            *encoded_value = 5;
            break;
        case 2048:
            *encoded_value = 6;
            break;
        case 4096:
            *encoded_value = 7;
            break;
        case 8192:
            *encoded_value = 8;
            break;
        case 16384:
            *encoded_value = 9;
            break;
        case 32768:
            *encoded_value = 10;
            break;
        case 65536:
            *encoded_value = 11;
            break;
        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_lag_rh_dynamic_size_decode
 * Purpose:
 *      Decode LAG trunk resilient hashing flow set size.
 * Parameters:
 *      encoded_value - (IN) Encoded value.
 *      dynamic_size - (OUT) Number of flow sets.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_lag_rh_dynamic_size_decode(int encoded_value, int *dynamic_size)
{   
    switch (encoded_value) {
        case 1:
            *dynamic_size = 64;
            break;
        case 2:
            *dynamic_size = 128;
            break;
        case 3:
            *dynamic_size = 256;
            break;
        case 4:
            *dynamic_size = 512;
            break;
        case 5:
            *dynamic_size = 1024;
            break;
        case 6:
            *dynamic_size = 2048;
            break;
        case 7:
            *dynamic_size = 4096;
            break;
        case 8:
            *dynamic_size = 8192;
            break;
        case 9:
            *dynamic_size = 16384;
            break;
        case 10:
            *dynamic_size = 32768;
            break;
        case 11:
            *dynamic_size = 65536;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_lag_rh_free_resource
 * Purpose:
 *      Free resources for a LAG resilient hashing.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      tid - (IN) LAG group ID.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_free_resource(int unit, int tid)
{
    int rv = BCM_E_NONE;
    trunk_group_entry_t trunk_group_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;
    int block_base_ptr;
    int num_blocks;

    SOC_IF_ERROR_RETURN(READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY,
                tid, &trunk_group_entry));
    if (0 == soc_TRUNK_GROUPm_field32_get(unit,
                &trunk_group_entry, ENHANCED_HASHING_ENABLEf)) {
        /* Either (1) resilient hashing is not enabled on this trunk
         * group, or (2) resilient hahsing is enabled with no members.
         * In case (2), TRUNK_GROUP.FLOW_SET_SIZE needs to be cleared. 
         */
        soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry,
                RH_FLOW_SET_SIZEf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid,
                    &trunk_group_entry));
        return BCM_E_NONE;
    }
    flow_set_size = soc_TRUNK_GROUPm_field32_get(unit,
            &trunk_group_entry, RH_FLOW_SET_SIZEf);
    entry_base_ptr = soc_TRUNK_GROUPm_field32_get(unit,
            &trunk_group_entry, RH_FLOW_SET_BASEf);

    /* Clear resilient hashing fields in TRUNK_GROUP */
    soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry,
            ENHANCED_HASHING_ENABLEf, 0);
    soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry,
            RH_FLOW_SET_BASEf, 0);
    soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry,
            RH_FLOW_SET_SIZEf, 0);
    SOC_IF_ERROR_RETURN(WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid,
                &trunk_group_entry));

    /* Clear flow set table entries */
    BCM_IF_ERROR_RETURN
        (_bcm_td2_lag_rh_dynamic_size_decode(flow_set_size, &num_entries));
    alloc_size = num_entries * sizeof(rh_lag_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_LAG_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_write_range(unit, RH_LAG_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, buf_ptr);
        return rv;
    }
    soc_cm_sfree(unit, buf_ptr);

    /* Free flow set table resources */
    block_base_ptr = entry_base_ptr >> 6;
    num_blocks = num_entries >> 6; 
    _BCM_LAG_RH_FLOWSET_BLOCK_USED_CLR_RANGE(unit, block_base_ptr, num_blocks);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_lag_rh_dynamic_size_set
 * Purpose:
 *      Set the dynamic size for a LAG resilient hashing group with
 *      no members, so it can be recovered from hardware during warm boot.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      tid - (IN) Trunk group ID.
 *      dynamic_size - (IN) Number of flow sets.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_dynamic_size_set(int unit, int tid, int dynamic_size)
{
    trunk_group_entry_t trunk_group_entry;
    int flow_set_size;

    SOC_IF_ERROR_RETURN(READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid,
                &trunk_group_entry));
    if (soc_TRUNK_GROUPm_field32_get(unit, &trunk_group_entry,
            ENHANCED_HASHING_ENABLEf)) {
        /* Not expecting resilient hashing to be enabled */
        return BCM_E_INTERNAL;
    }

    BCM_IF_ERROR_RETURN
        (bcm_td2_lag_rh_dynamic_size_encode(dynamic_size, &flow_set_size));
    soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry, RH_FLOW_SET_SIZEf,
            flow_set_size);
    SOC_IF_ERROR_RETURN(WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid,
                &trunk_group_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_lag_rh_rand_get
 * Purpose:
 *      Get a random number between 0 and the given max value.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      rand_max - (IN) Maximum random number.
 *      rand_num - (OUT) Random number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      This procedure uses the pseudo-random number generator algorithm
 *      suggested by the C standard.
 */
STATIC int
_bcm_td2_lag_rh_rand_get(int unit, int rand_max, int *rand_num)
{
    int modulus;
    int rand_seed_shift;

    if (rand_max < 0) {
        return BCM_E_PARAM;
    }

    if (NULL == rand_num) {
        return BCM_E_PARAM;
    }

    /* Make sure the modulus does not exceed limit. For instance,
     * if the 32-bit rand_seed is shifted to the right by 16 bits before
     * the modulo operation, the modulus should not exceed 1 << 16.
     */
    modulus = rand_max + 1;
    rand_seed_shift = 16;
    if (modulus > (1 << (32 - rand_seed_shift))) {
        return BCM_E_PARAM;
    }

    _td2_lag_rh_info[unit]->lag_rh_rand_seed =
        _td2_lag_rh_info[unit]->lag_rh_rand_seed * 1103515245 + 12345;

    *rand_num = (_td2_lag_rh_info[unit]->lag_rh_rand_seed >> rand_seed_shift) %
                modulus;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_lag_rh_member_choose
 * Purpose:
 *      Choose a member of the LAG trunk group.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      num_members - (IN) Number of members to choose from.
 *      entry_count_arr   - (IN/OUT) An array keeping track of how many
 *                                   flow set entries have been assigned
 *                                   to each member.
 *      max_entry_count   - (IN/OUT) The maximum number of flow set entries
 *                                   that can be assigned to a member. 
 *      chosen_index      - (OUT) The index of the chosen member.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_lag_rh_member_choose(int unit, int num_members, int *entry_count_arr,
        int *max_entry_count, int *chosen_index)
{
    int member_index;
    int next_index;

    *chosen_index = 0;

    /* Choose a random member index */
    BCM_IF_ERROR_RETURN(_bcm_td2_lag_rh_rand_get(unit, num_members - 1,
                &member_index));

    if (entry_count_arr[member_index] < *max_entry_count) {
        entry_count_arr[member_index]++;
        *chosen_index = member_index;
    } else {
        /* The randomly chosen member has reached the maximum
         * flow set entry count. Choose the next member that
         * has not reached the maximum entry count.
         */
        next_index = (member_index + 1) % num_members;
        while (next_index != member_index) {
            if (entry_count_arr[next_index] < *max_entry_count) {
                entry_count_arr[next_index]++;
                *chosen_index = next_index;
                break;
            } else {
                next_index = (next_index + 1) % num_members;
            }
        }
        if (next_index == member_index) {
            /* All members have reached the maximum flow set entry
             * count. This scenario occurs when dividing the number of
             * flow set entries by the number of members results
             * in a non-zero remainder. The remainder flow set entries
             * will be distributed among members, at most 1 remainder
             * entry per member.
             */
            (*max_entry_count)++;
            if (entry_count_arr[member_index] < *max_entry_count) {
                entry_count_arr[member_index]++;
                *chosen_index = member_index;
            } else {
                /* It's possible that the member's entry count already equals
                 * to the incremented value of max_entry_count.
                 */
                next_index = (member_index + 1) % num_members;
                while (next_index != member_index) {
                    if (entry_count_arr[next_index] < *max_entry_count) {
                        entry_count_arr[next_index]++;
                        *chosen_index = next_index;
                        break;
                    } else {
                        next_index = (next_index + 1) % num_members;
                    }
                }
                if (next_index == member_index) {
                    return BCM_E_INTERNAL;
                }
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_lag_rh_set
 * Purpose:
 *      Configure a LAG resilient hashing group.
 * Parameters:
 *      unit         - (IN) SOC unit number. 
 *      tid          - (IN) LAG trunk group ID.
 *      add_info     - (IN) Pointer to trunk add info structure.
 *      num_rh_ports - (IN) Number of resilient hashing eligible members in
 *                          the group.
 *      mod_array    - (IN) Array of module IDs of resilient hashing eligible
 *                          members.
 *      port_array   - (IN) Array of port IDs of resilient hashing eligible
 *                          members.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_set(int unit,
        int tid,
        _esw_trunk_add_info_t *add_info,
        int num_rh_ports,
        int *mod_array,
        int *port_array)
{
    int rv = BCM_E_NONE;
    int num_blocks;
    int total_blocks;
    int max_block_base_ptr;
    int block_base_ptr;
    int entry_base_ptr;
    SHR_BITDCL occupied;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int *entry_count_arr = NULL;
    int max_entry_count;
    int chosen_index;
    int i;
    rh_lag_flowset_entry_t *flowset_entry;
    int index_min, index_max;
    trunk_group_entry_t trunk_group_entry;
    int flow_set_size;

    if (add_info == NULL || add_info->psc != BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (num_rh_ports > 0 && (mod_array == NULL || port_array == NULL)) {
        return BCM_E_PARAM;
    }

    if (num_rh_ports == 0) {
        /* Write the dynamic_size to hardware, so it can be
         * recovered during warm boot.
         */
        BCM_IF_ERROR_RETURN(bcm_td2_lag_rh_dynamic_size_set(unit,
                    tid, add_info->dynamic_size));
        return BCM_E_NONE;
    }

    /* Find a contiguous region of flow set table that's large
     * enough to hold the requested number of flow sets.
     * The flow set table is allocated in 64-entry blocks.
     */
    num_blocks = add_info->dynamic_size >> 6;
    total_blocks = _td2_lag_rh_info[unit]->num_lag_rh_flowset_blocks;
    if (num_blocks > total_blocks) {
        return BCM_E_RESOURCE;
    }
    max_block_base_ptr = total_blocks - num_blocks;
    for (block_base_ptr = 0;
         block_base_ptr <= max_block_base_ptr;
         block_base_ptr++) {
        /* Check if the contiguous region of flow set table from
         * block_base_ptr to (block_base_ptr + num_blocks - 1) is free. 
         */
        _BCM_LAG_RH_FLOWSET_BLOCK_TEST_RANGE(unit, block_base_ptr, num_blocks,
                occupied); 
        if (!occupied) {
            break;
        }
    }
    if (block_base_ptr > max_block_base_ptr) {
        /* A contiguous region of the desired size could not be found in
         * flow set table.
         */
        return BCM_E_RESOURCE;
    }

    /* Configure flow set table */
    alloc_size = add_info->dynamic_size * sizeof(rh_lag_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_LAG_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);

    entry_count_arr = sal_alloc(sizeof(int) * num_rh_ports,
            "RH entry count array");
    if (NULL == entry_count_arr) {
        soc_cm_sfree(unit, buf_ptr);
        return BCM_E_MEMORY;
    }
    sal_memset(entry_count_arr, 0, sizeof(int) * num_rh_ports);
    max_entry_count = add_info->dynamic_size / num_rh_ports;

    for (i = 0; i < add_info->dynamic_size; i++) {
        /* Choose a member of the LAG */
        rv = _bcm_td2_lag_rh_member_choose(unit, num_rh_ports,
                entry_count_arr, &max_entry_count, &chosen_index);
        if (BCM_FAILURE(rv)) {
            soc_cm_sfree(unit, buf_ptr);
            sal_free(entry_count_arr);
            return rv;
        }

        /* Set flow set entry */
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_LAG_FLOWSETm, rh_lag_flowset_entry_t *, buf_ptr, i);
        soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry, VALIDf, 1);
        soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry, MODULE_IDf,
                mod_array[chosen_index]);
        soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry, PORT_NUMf,
                port_array[chosen_index]);
    }

    entry_base_ptr = block_base_ptr << 6;
    index_min = entry_base_ptr;
    index_max = index_min + add_info->dynamic_size - 1;
    rv = soc_mem_write_range(unit, RH_LAG_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, buf_ptr);
        sal_free(entry_count_arr);
        return rv;
    }
    soc_cm_sfree(unit, buf_ptr);
    sal_free(entry_count_arr);

    _BCM_LAG_RH_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr, num_blocks);

    /* Update TRUNK_GROUP */
    SOC_IF_ERROR_RETURN(READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid,
                &trunk_group_entry));
    soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry,
            ENHANCED_HASHING_ENABLEf, 1);
    soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry,
            RH_FLOW_SET_BASEf, entry_base_ptr);
    BCM_IF_ERROR_RETURN(bcm_td2_lag_rh_dynamic_size_encode(
                add_info->dynamic_size, &flow_set_size));
    soc_TRUNK_GROUPm_field32_set(unit, &trunk_group_entry,
            RH_FLOW_SET_SIZEf, flow_set_size);
    SOC_IF_ERROR_RETURN(WRITE_TRUNK_GROUPm(unit, MEM_BLOCK_ALL, tid,
                &trunk_group_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_lag_rh_add
 * Purpose:
 *      Add a member to a LAG resilient hashing group.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      tid      - (IN) LAG trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure.
 *      num_rh_ports - (IN) Number of resilient hashing eligible members in
 *                          the group, including the member to be added.
 *      mod_array    - (IN) Array of module IDs of resilient hashing eligible
 *                          members, including the member to be added.
 *      port_array   - (IN) Array of port IDs of resilient hashing eligible
 *                          members, including the member to be added.
 *      new_member   - (IN) Member to add.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_add(int unit,
        int tid,
        _esw_trunk_add_info_t *add_info,
        int num_rh_ports,
        int *mod_array,
        int *port_array,
        bcm_trunk_member_t *new_member)
{
    int rv = BCM_E_NONE;
    bcm_port_t new_port;
    bcm_module_t new_mod;
    int num_existing_members;
    int i, j;
    trunk_group_entry_t trunk_group_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;
    int *entry_count_arr = NULL;
    rh_lag_flowset_entry_t *flowset_entry;
    bcm_module_t mod;
    bcm_port_t port;
    int member_index = 0;
    int lower_bound, upper_bound;
    int threshold;
    int new_member_entry_count;
    int entry_index, next_entry_index;
    int is_new_member;

    if (add_info == NULL || add_info->psc != BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (num_rh_ports == 0 || mod_array == NULL || port_array == NULL) {
        return BCM_E_PARAM;
    }

    if (new_member == NULL) {
        return BCM_E_PARAM;
    }

    if (num_rh_ports == 1) {
        /* Adding the first member is the same as setting one member */
        return bcm_td2_lag_rh_set(unit, tid, add_info, num_rh_ports, mod_array,
                port_array);
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_gport_array_resolve(unit, FALSE, 1,
                &new_member->gport, &new_port, &new_mod));

    /* Swap new member to the last one for BCM_TRUNK_FLAG_MEMBER_SORT */
    if (add_info->flags & BCM_TRUNK_FLAG_MEMBER_SORT) {
        for (i = 0; i < num_rh_ports; i++){
            if (i == (num_rh_ports - 1)) {
                break;
            }
            if (new_mod == mod_array[i] &&
                new_port == port_array[i]) {
                mod = mod_array[i];
                port = port_array[i];
                mod_array[i] = mod_array[num_rh_ports - 1];
                port_array[i] = port_array[num_rh_ports - 1];
                mod_array[num_rh_ports - 1] = mod;
                port_array[num_rh_ports - 1] = port;
                break;
            }    
        }
    }

    /* Check that the new member is the last element of the array of
     * resilient hashing eligible members.
     */
    if (new_mod != mod_array[num_rh_ports - 1] ||
        new_port != port_array[num_rh_ports - 1]) {
        return BCM_E_PARAM;
    }
    num_existing_members = num_rh_ports - 1;

    /* Read all the flow set entries of this LAG resilient hash group,
     * and compute the number of entries currently assigned to each
     * existing member.
     */
    SOC_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &trunk_group_entry));
    entry_base_ptr = soc_TRUNK_GROUPm_field32_get(unit, &trunk_group_entry,
            RH_FLOW_SET_BASEf);
    flow_set_size = soc_TRUNK_GROUPm_field32_get(unit, &trunk_group_entry,
            RH_FLOW_SET_SIZEf);
    BCM_IF_ERROR_RETURN
        (_bcm_td2_lag_rh_dynamic_size_decode(flow_set_size, &num_entries));

    alloc_size = num_entries * sizeof(rh_lag_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_LAG_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_read_range(unit, RH_LAG_FLOWSETm, MEM_BLOCK_ANY,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_existing_members * sizeof(int);
    entry_count_arr = sal_alloc(alloc_size, "LAG RH entry count array");
    if (NULL == entry_count_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(entry_count_arr, 0, alloc_size);

    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_LAG_FLOWSETm, rh_lag_flowset_entry_t *, buf_ptr, i);
        if (!soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                    VALIDf)) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        mod = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                MODULE_IDf);
        port = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                PORT_NUMf);
        for (j = 0; j < num_existing_members; j++) {
            if (mod == mod_array[j] && port == port_array[j]) {
                break;
            }
        }
        if (j == num_existing_members) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        entry_count_arr[j]++;
    }

    /* Check that the distribution of flow set entries among existing members
     * is balanced. For instance, if the number of flow set entries is 64, and
     * the number of existing members is 6, then every member should have
     * between 10 and 11 entries. 
     */ 
    lower_bound = num_entries / num_existing_members;
    upper_bound = (num_entries % num_existing_members) ?
                  (lower_bound + 1) : lower_bound;
    for (i = 0; i < num_existing_members; i++) {
        if (entry_count_arr[i] < lower_bound ||
                entry_count_arr[i] > upper_bound) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
    }

    /* Re-balance flow set entries from existing members to the new member.
     * For example, if the number of flow set entries is 64, and the number of
     * existing members is 6, then each existing member has between 10 and 11
     * entries. The entries should be re-assigned from the 6 existing members
     * to the new member such that each member will end up with between 9 and
     * 10 entries.
     */
    lower_bound = num_entries / num_rh_ports;
    upper_bound = (num_entries % num_rh_ports) ?
                  (lower_bound + 1) : lower_bound;
    threshold = upper_bound;
    new_member_entry_count = 0;
    while (new_member_entry_count < lower_bound) {
        /* Pick a random entry in the flow set buffer */
        rv = _bcm_td2_lag_rh_rand_get(unit, num_entries - 1, &entry_index);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        flowset_entry = soc_mem_table_idx_to_pointer(unit, RH_LAG_FLOWSETm,
                rh_lag_flowset_entry_t *, buf_ptr, entry_index);
        mod = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                MODULE_IDf);
        port = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                PORT_NUMf);

        /* Determine if the randomly picked entry contains the new member.
         * If not, determine the existing member index.
         */
        if (mod == new_mod && port == new_port) {
            is_new_member = TRUE;
        } else {
            is_new_member = FALSE;
        }
        if (!is_new_member) {
            for (i = 0; i < num_existing_members; i++) {
                if (mod == mod_array[i] && port == port_array[i]) {
                    break;
                }
            }
            if (i == num_existing_members) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            member_index = i;
        }

        if (!is_new_member && (entry_count_arr[member_index] > threshold)) {
            soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry,
                    MODULE_IDf, new_mod);
            soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry,
                    PORT_NUMf, new_port);
            entry_count_arr[member_index]--;
            new_member_entry_count++;
        } else {
            /* Either the member of the randomly chosen entry is
             * the same as the new member, or the member is an existing
             * member and its entry count has decreased to threshold.
             * In both cases, find the next entry that contains a
             * member that's not the new member and whose entry count
             * has not decreased to threshold.
             */
            next_entry_index = (entry_index + 1) % num_entries;
            while (next_entry_index != entry_index) {
                flowset_entry = soc_mem_table_idx_to_pointer(unit,
                        RH_LAG_FLOWSETm, rh_lag_flowset_entry_t *, buf_ptr,
                        next_entry_index);
                mod = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                        MODULE_IDf);
                port = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                        PORT_NUMf);

                /* Determine if the entry contains the new member.
                 * If not, determine the existing member index.
                 */
                if (mod == new_mod && port == new_port) {
                    is_new_member = TRUE;
                } else {
                    is_new_member = FALSE;
                }
                if (!is_new_member) {
                    for (i = 0; i < num_existing_members; i++) {
                        if (mod == mod_array[i] && port == port_array[i]) {
                            break;
                        }
                    }
                    if (i == num_existing_members) {
                        rv = BCM_E_INTERNAL;
                        goto cleanup;
                    }
                    member_index = i;
                }

                if (!is_new_member &&
                        (entry_count_arr[member_index] > threshold)) {
                    soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry,
                            MODULE_IDf, new_mod);
                    soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry,
                            PORT_NUMf, new_port);
                    entry_count_arr[member_index]--;
                    new_member_entry_count++;
                    break;
                } else {
                    next_entry_index = (next_entry_index + 1) % num_entries;
                }
            }
            if (next_entry_index == entry_index) {
                /* The entry count of all existing members has decreased
                 * to threshold. The entry count of the new member has
                 * not yet increased to lower_bound. Lower the threshold.
                 */
                threshold--;
            }
        }
    }

    /* Write re-balanced flow set entries to hardware */
    rv = soc_mem_write_range(unit, RH_LAG_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

cleanup:
    if (buf_ptr) {
        soc_cm_sfree(unit, buf_ptr);
    }
    if (entry_count_arr) {
        sal_free(entry_count_arr);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_lag_rh_delete
 * Purpose:
 *      Delete a member from a LAG resilient hashing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      tid - (IN) LAG trunk group ID.
 *      add_info - (IN) Pointer to trunk add info structure.
 *      num_rh_ports - (IN) Number of resilient hashing eligible members in
 *                          the group, except the member to be deleted.
 *      mod_array    - (IN) Array of module IDs of resilient hashing eligible
 *                          members, except the member to be deleted.
 *      port_array   - (IN) Array of port IDs of resilient hashing eligible
 *                          members, except the member to be deleted.
 *      leaving_member - (IN) Member to delete.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_delete(int unit,
        int tid,
        _esw_trunk_add_info_t *add_info,
        int num_rh_ports,
        int *mod_array,
        int *port_array,
        bcm_trunk_member_t *leaving_member)
{
    int rv = BCM_E_NONE;
    bcm_port_t leaving_port;
    bcm_module_t leaving_mod;
    int num_remaining_members;
    int i, j;
    trunk_group_entry_t trunk_group_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;
    int leaving_member_entry_count;
    int *entry_count_arr = NULL;
    rh_lag_flowset_entry_t *flowset_entry;
    bcm_module_t mod;
    bcm_port_t port;
    int lower_bound, upper_bound;
    int threshold;
    int chosen_index;

    if (add_info == NULL || add_info->psc != BCM_TRUNK_PSC_DYNAMIC_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (num_rh_ports > 0 && (mod_array == NULL || port_array == NULL)) {
        return BCM_E_PARAM;
    }

    if (leaving_member == NULL) {
        return BCM_E_PARAM;
    }

    if (num_rh_ports == 0) {
        /* Deleting the last member is the same as freeing all resources */
        BCM_IF_ERROR_RETURN(bcm_td2_lag_rh_free_resource(unit, tid));

        /* Write the dynamic_size to hardware, so it can be recovered
         * during warm boot.
         */
        BCM_IF_ERROR_RETURN(bcm_td2_lag_rh_dynamic_size_set(unit, tid,
                    add_info->dynamic_size));

        return BCM_E_NONE;
    }

    /* Check that the leaving member is not a member of the array */
    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_gport_array_resolve(unit, FALSE, 1,
                &leaving_member->gport, &leaving_port, &leaving_mod));
    for (i = 0; i < num_rh_ports; i++) {
        if (leaving_mod == mod_array[i] && leaving_port == port_array[i]) {
            return BCM_E_PARAM;
        }
    }
    num_remaining_members = num_rh_ports;

    /* Read all the flow set entries of this LAG resilient hash group,
     * and compute the number of entries currently assigned to each
     * member, including the leaving member.
     */
    SOC_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tid, &trunk_group_entry));
    entry_base_ptr = soc_TRUNK_GROUPm_field32_get(unit, &trunk_group_entry,
            RH_FLOW_SET_BASEf);
    flow_set_size = soc_TRUNK_GROUPm_field32_get(unit, &trunk_group_entry,
            RH_FLOW_SET_SIZEf);
    BCM_IF_ERROR_RETURN
        (_bcm_td2_lag_rh_dynamic_size_decode(flow_set_size, &num_entries));

    alloc_size = num_entries * sizeof(rh_lag_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_LAG_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_read_range(unit, RH_LAG_FLOWSETm, MEM_BLOCK_ANY,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_remaining_members * sizeof(int);
    entry_count_arr = sal_alloc(alloc_size, "RH entry count array");
    if (NULL == entry_count_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(entry_count_arr, 0, alloc_size);

    leaving_member_entry_count = 0;
    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_LAG_FLOWSETm, rh_lag_flowset_entry_t *, buf_ptr, i);
        if (!soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                    VALIDf)) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        mod = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                MODULE_IDf);
        port = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                PORT_NUMf);
        if (mod == leaving_mod && port == leaving_port) {
            leaving_member_entry_count++;
        } else {
            for (j = 0; j < num_remaining_members; j++) {
                if (mod == mod_array[j] && port == port_array[j]) {
                    break;
                }
            }
            if (j == num_remaining_members) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            entry_count_arr[j]++;
        }
    }

    /* Check that the distribution of flow set entries among all members
     * is balanced. For instance, if the number of flow set entries is 64, and
     * the number of members is 6, then every member should have
     * between 10 and 11 entries. 
     */ 
    lower_bound = num_entries / (num_remaining_members + 1);
    upper_bound = (num_entries % (num_remaining_members + 1)) ?
                  (lower_bound + 1) : lower_bound;
    for (i = 0; i < num_remaining_members; i++) {
        if (entry_count_arr[i] < lower_bound ||
                entry_count_arr[i] > upper_bound) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
    }
    if (leaving_member_entry_count < lower_bound ||
            leaving_member_entry_count > upper_bound) {
        rv = BCM_E_INTERNAL;
        goto cleanup;
    }

    /* Re-balance flow set entries from the leaving member to the remaining
     * members. For example, if the number of flow set entries is 64, and
     * the number of members is 6, then each member has between 10 and 11
     * entries. The entries should be re-assigned from the leaving member to
     * the remaining 5 members such that each remaining member will end up
     * with between 12 and 13 entries.
     */
    lower_bound = num_entries / num_remaining_members;
    upper_bound = (num_entries % num_remaining_members) ?
                  (lower_bound + 1) : lower_bound;
    threshold = lower_bound;
    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_LAG_FLOWSETm, rh_lag_flowset_entry_t *, buf_ptr, i);
        mod = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                MODULE_IDf);
        port = soc_mem_field32_get(unit, RH_LAG_FLOWSETm, flowset_entry,
                PORT_NUMf);
        if (mod != leaving_mod || port != leaving_port) {
            continue;
        }

        /* Randomly choose a member among the remaining members */
        rv = _bcm_td2_lag_rh_member_choose(unit, num_remaining_members,
                entry_count_arr, &threshold, &chosen_index);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry, MODULE_IDf,
                mod_array[chosen_index]);
        soc_mem_field32_set(unit, RH_LAG_FLOWSETm, flowset_entry, PORT_NUMf,
                port_array[chosen_index]);
    }

    /* Write re-balanced flow set entries to hardware */
    rv = soc_mem_write_range(unit, RH_LAG_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

cleanup:
    if (buf_ptr) {
        soc_cm_sfree(unit, buf_ptr);
    }
    if (entry_count_arr) {
        sal_free(entry_count_arr);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_lag_rh_ethertype_set
 * Purpose:
 *      Set the Ethertypes that are eligible or ineligible for
 *      LAG resilient hashing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) BCM_TRUNK_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_count - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (IN) Array of Ethertypes.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_td2_lag_rh_ethertype_set(
    int unit, 
    uint32 flags, 
    int ethertype_count, 
    int *ethertype_array)
{
    uint32 control_reg;
    int i, j;
    rh_lag_ethertype_eligibility_map_entry_t ethertype_entry;

    /* Input check */
    if (ethertype_count > soc_mem_index_count(unit,
                RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm)) {
        return BCM_E_RESOURCE;
    }

    /* Update ethertype eligibility control register */
    SOC_IF_ERROR_RETURN
        (READ_RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &control_reg));
    soc_reg_field_set(unit, RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr,
            &control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf,
            flags & BCM_TRUNK_DYNAMIC_ETHERTYPE_ELIGIBLE ? 1 : 0);
    soc_reg_field_set(unit, RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr,
            &control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf,
            flags & BCM_TRUNK_DYNAMIC_ETHERTYPE_INNER ? 1 : 0);
    SOC_IF_ERROR_RETURN
        (WRITE_RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, control_reg));

    /* Update Ethertype eligibility map table */
    for (i = 0; i < ethertype_count; i++) {
        sal_memset(&ethertype_entry, 0,
                sizeof(rh_lag_ethertype_eligibility_map_entry_t));
        soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, VALIDf, 1);
        soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, ETHERTYPEf, ethertype_array[i]);
        SOC_IF_ERROR_RETURN(WRITE_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, i, &ethertype_entry));
    }

    /* Zero out remaining entries of Ethertype eligibility map table */
    for (j = i; j < soc_mem_index_count(unit,
                RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm); j++) {
        SOC_IF_ERROR_RETURN(WRITE_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, j, soc_mem_entry_null(unit,
                        RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm)));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_lag_rh_ethertype_get
 * Purpose:
 *      Get the Ethertypes that are eligible or ineligible for
 *      LAG resilient hashing.
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
bcm_td2_lag_rh_ethertype_get(
    int unit, 
    uint32 *flags, 
    int ethertype_max, 
    int *ethertype_array, 
    int *ethertype_count)
{
    uint32 control_reg;
    int i;
    int ethertype;
    rh_lag_ethertype_eligibility_map_entry_t ethertype_entry;

    *ethertype_count = 0;

    /* Get flags */
    SOC_IF_ERROR_RETURN
        (READ_RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &control_reg));
    if (soc_reg_field_get(unit, RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr,
                control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf)) {
        *flags |= BCM_TRUNK_DYNAMIC_ETHERTYPE_ELIGIBLE;
    }
    if (soc_reg_field_get(unit, RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr,
                control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf)) {
        *flags |= BCM_TRUNK_DYNAMIC_ETHERTYPE_INNER;
    }

    /* Get Ethertypes */
    for (i = 0; i < soc_mem_index_count(unit,
                RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm); i++) {
        SOC_IF_ERROR_RETURN(READ_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ANY, i, &ethertype_entry));
        if (soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                    &ethertype_entry, VALIDf)) {
            ethertype = soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
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

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Function:
 *      bcm_td2_hg_rh_recover
 * Purpose:
 *      Recover Higig resilient hashing software state.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      hgtid - (IN) Higig trunk ID.
 *      trunk_info - (OUT) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_hg_rh_recover(int unit, int hgtid, trunk_private_t *trunk_info)
{
    rh_hgt_group_control_entry_t rh_hgt_group_control_entry;
    hg_trunk_group_entry_t hg_trunk_group_entry;
    int flow_set_size;
    int entry_base_ptr;
    int num_entries;
    int block_base_ptr;
    int num_blocks;

    /* A Higig trunk group can be in one of 3 resilient hashing states:
     * (1) RH not enabled: RH_HGT_GROUP_CONTROL.FLOW_SET_SIZE = 0.
     * (2) RH enabled but without any members:
     *     RH_HGT_GROUP_CONTROL.FLOW_SET_SIZE is non-zero,
     *     HG_TRUNK_GROUP.ENHANCED_HASHING_ENABLE = 0.
     * (3) RH enabled with members:
     *     RH_HGT_GROUP_CONTROL.FLOW_SET_SIZE is non-zero,
     *     HG_TRUNK_GROUP.ENHANCED_HASHING_ENABLE = 1.
     */

    SOC_IF_ERROR_RETURN(READ_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY,
                hgtid, &rh_hgt_group_control_entry));
    flow_set_size = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
            &rh_hgt_group_control_entry, FLOW_SET_SIZEf);
    if (0 == flow_set_size) {
        /* Resilient hashing is not enabled on this Higig trunk group.
         * Nothing to do.
         */
        return BCM_E_NONE;
    }

    /* Recover dynamic size */
    BCM_IF_ERROR_RETURN
        (_bcm_td2_hg_rh_dynamic_size_decode(flow_set_size, &num_entries));
    trunk_info->dynamic_size = num_entries;

    /* Recover PSC */
    trunk_info->psc = BCM_TRUNK_PSC_DYNAMIC_RESILIENT;

    SOC_IF_ERROR_RETURN(READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid,
                &hg_trunk_group_entry));
    if (soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_trunk_group_entry,
                ENHANCED_HASHING_ENABLEf)) {
        /* Recover Higig resilient hashing flow set block usage */
        entry_base_ptr = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit,
                &rh_hgt_group_control_entry, FLOW_SET_BASEf);
        block_base_ptr = entry_base_ptr >> 6;
        num_blocks = num_entries >> 6; 
        _BCM_HG_RH_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr, num_blocks);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_lag_rh_recover
 * Purpose:
 *      Recover LAG resilient hashing software state.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      tid - (IN) Trunk group ID.
 *      trunk_info - (OUT) Pointer to trunk info.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_lag_rh_recover(int unit, int tid, trunk_private_t *trunk_info)
{
    trunk_group_entry_t trunk_group_entry;
    int flow_set_size;
    int entry_base_ptr;
    int num_entries;
    int block_base_ptr;
    int num_blocks;

    /* A trunk group can be in one of 3 resilient hashing states:
     * (1) RH not enabled: TRUNK_GROUP.RH_FLOW_SET_SIZE = 0.
     * (2) RH enabled but without any members:
     *     TRUNK_GROUP.RH_FLOW_SET_SIZE is non-zero,
     *     TRUNK_GROUP.ENHANCED_HASHING_ENABLE = 0.
     * (3) RH enabled with members:
     *     TRUNK_GROUP.RH_FLOW_SET_SIZE is non-zero,
     *     TRUNK_GROUP.ENHANCED_HASHING_ENABLE = 1.
     */

    SOC_IF_ERROR_RETURN(READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY,
                tid, &trunk_group_entry));
    flow_set_size = soc_TRUNK_GROUPm_field32_get(unit,
            &trunk_group_entry, RH_FLOW_SET_SIZEf);
    if (0 == flow_set_size) {
        /* Resilient hashing is not enabled on this LAG.
         * Nothing to do.
         */
        return BCM_E_NONE;
    }

    /* Recover dynamic size */
    BCM_IF_ERROR_RETURN
        (_bcm_td2_lag_rh_dynamic_size_decode(flow_set_size, &num_entries));
    trunk_info->dynamic_size = num_entries;

    /* Recover PSC */
    trunk_info->psc = BCM_TRUNK_PSC_DYNAMIC_RESILIENT;

    if (soc_TRUNK_GROUPm_field32_get(unit, &trunk_group_entry,
                ENHANCED_HASHING_ENABLEf)) {
        /* Recover LAG resilient hashing flow set block usage */
        entry_base_ptr = soc_TRUNK_GROUPm_field32_get(unit,
                &trunk_group_entry, RH_FLOW_SET_BASEf);
        block_base_ptr = entry_base_ptr >> 6;
        num_blocks = num_entries >> 6; 
        _BCM_LAG_RH_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr, num_blocks);
    }

    return BCM_E_NONE;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP

/*
 * Function:
 *     bcm_td2_hg_rh_sw_dump
 * Purpose:
 *     Displays Higig resilient hashing state maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void bcm_td2_hg_rh_sw_dump(int unit)
{
    int i, j;

    LOG_CLI((BSL_META_U(unit,
                        "Higig Resilient Hashing Info -\n")));

    /* Print Higig RH flowset table usage */
    LOG_CLI((BSL_META_U(unit,
                        "    Higig RH Flowset Table Blocks Used:")));
    j = 0;
    for (i = 0; i < soc_mem_index_count(unit, RH_HGT_FLOWSETm) >> 6; i++) {
        if (_BCM_HG_RH_FLOWSET_BLOCK_USED_GET(unit, i)) {
            j++;
            if (j % 15 == 1) {
                LOG_CLI((BSL_META_U(unit,
                                    "\n     ")));
            }
            LOG_CLI((BSL_META_U(unit,
                                " %4d"), i));
        }
    }
    LOG_CLI((BSL_META_U(unit,
                        "\n")));

    return;
}

/*
 * Function:
 *     bcm_td2_lag_rh_sw_dump
 * Purpose:
 *     Displays LAG resilient hashing state maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void bcm_td2_lag_rh_sw_dump(int unit)
{
    int i, j;

    LOG_CLI((BSL_META_U(unit,
                        "LAG Resilient Hashing Info -\n")));
    LOG_CLI((BSL_META_U(unit,
                        "    LAG RH Flowset Table Blocks Total: %d\n"),
             _td2_lag_rh_info[unit]->num_lag_rh_flowset_blocks));

    /* Print LAG RH flowset table usage */
    LOG_CLI((BSL_META_U(unit,
                        "    LAG RH Flowset Table Blocks Used:")));
    j = 0;
    for (i = 0; i < _td2_lag_rh_info[unit]->num_lag_rh_flowset_blocks; i++) {
        if (_BCM_LAG_RH_FLOWSET_BLOCK_USED_GET(unit, i)) {
            j++;
            if (j % 15 == 1) {
                LOG_CLI((BSL_META_U(unit,
                                    "\n     ")));
            }
            LOG_CLI((BSL_META_U(unit,
                                " %4d"), i));
        }
    }
    LOG_CLI((BSL_META_U(unit,
                        "\n")));

    return;
}

#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#ifdef INCLUDE_L3

/*
 * Function:
 *      bcm_td2_vp_lag_deinit
 * Purpose:
 *      Deallocate internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
void
bcm_td2_vp_lag_deinit(int unit)
{
    if (_td2_vp_lag_info[unit]) {
        if (_td2_vp_lag_info[unit]->vp_lag_used_bitmap) {
            sal_free(_td2_vp_lag_info[unit]->vp_lag_used_bitmap);
            _td2_vp_lag_info[unit]->vp_lag_used_bitmap = NULL;
        }
        if (_td2_vp_lag_info[unit]->vp_lag_egr_member_bitmap) {
            sal_free(_td2_vp_lag_info[unit]->vp_lag_egr_member_bitmap);
            _td2_vp_lag_info[unit]->vp_lag_egr_member_bitmap = NULL;
        }
        if (_td2_vp_lag_info[unit]->group_info) {
            sal_free(_td2_vp_lag_info[unit]->group_info);
            _td2_vp_lag_info[unit]->group_info = NULL;
        }
        sal_free(_td2_vp_lag_info[unit]);
        _td2_vp_lag_info[unit] = NULL;
    }
}

/*
 * Function:
 *      bcm_td2_vp_lag_init
 * Purpose:
 *      Initialize internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_vp_lag_init(int unit)
{
    int rv = BCM_E_NONE;
    int egr_member_table_size;

    bcm_td2_vp_lag_deinit(unit);

    if (NULL == _td2_vp_lag_info[unit]) {
        _td2_vp_lag_info[unit] = sal_alloc(sizeof(_td2_vp_lag_info_t),
                "VP LAG info");
        if (NULL == _td2_vp_lag_info[unit]) {
            bcm_td2_vp_lag_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_td2_vp_lag_info[unit], 0, sizeof(_td2_vp_lag_info_t));

    /* Get maximum number of VP LAGs configured by user */
    MAX_VP_LAGS(unit) = soc_property_get(unit, spn_MAX_VP_LAGS,
            soc_mem_index_count(unit, EGR_VPLAG_GROUPm));
    if (0 == MAX_VP_LAGS(unit)) {
        /* User configured zero VP LAGS, nothing more to do. */
        return BCM_E_NONE;
    }

    /* Allocate a bitmap tracking which VP LAG IDs are used */
    if (NULL == _td2_vp_lag_info[unit]->vp_lag_used_bitmap) {
        _td2_vp_lag_info[unit]->vp_lag_used_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(MAX_VP_LAGS(unit)), "VP LAG used bitmap");
        if (NULL == _td2_vp_lag_info[unit]->vp_lag_used_bitmap) {
            bcm_td2_vp_lag_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_td2_vp_lag_info[unit]->vp_lag_used_bitmap, 0,
            SHR_BITALLOCSIZE(MAX_VP_LAGS(unit)));

    /* Allocate a bitmap tracking which EGR_VPLAG_MEMBER table entries are used */
    egr_member_table_size = soc_mem_index_count(unit, EGR_VPLAG_MEMBERm);
    if (NULL == _td2_vp_lag_info[unit]->vp_lag_egr_member_bitmap) {
        _td2_vp_lag_info[unit]->vp_lag_egr_member_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(egr_member_table_size),
                    "EGR_VPLAG_MEMBER table used bitmap");
        if (NULL == _td2_vp_lag_info[unit]->vp_lag_egr_member_bitmap) {
            bcm_td2_vp_lag_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_td2_vp_lag_info[unit]->vp_lag_egr_member_bitmap, 0,
            SHR_BITALLOCSIZE(egr_member_table_size));


    /* Allocate an array containing info for each VP LAG */
    if (NULL == _td2_vp_lag_info[unit]->group_info) {
        _td2_vp_lag_info[unit]->group_info = sal_alloc
            (MAX_VP_LAGS(unit) * sizeof(_td2_vp_lag_group_t),
             "VP LAG group info array");
        if (NULL == _td2_vp_lag_info[unit]->group_info) {
            bcm_td2_vp_lag_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_td2_vp_lag_info[unit]->group_info, 0,
            MAX_VP_LAGS(unit) * sizeof(_td2_vp_lag_group_t));

    return rv;
}

/*
 * Function:
 *	bcm_td2_vp_lag_info_get
 * Purpose:
 *      Get VP LAG info.
 * Parameters:
 *      unit  - SOC unit number
 *      ngroups_vp_lag - (OUT) Number of VP LAGs
 *      nports_vp_lag  - (OUT) Maximum number of VPs per VP LAG
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_vp_lag_info_get(int unit, int *ngroups_vp_lag, int *nports_vp_lag)
{
    VP_LAG_INIT_CHECK(unit);

    *ngroups_vp_lag = MAX_VP_LAGS(unit);
    *nports_vp_lag = 1 << soc_mem_field_length(unit, EGR_VPLAG_GROUPm, COUNTf);

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_td2_tid_to_vp_lag_id
 * Purpose:
 *      Convert trunk ID to VP LAG ID.
 * Parameters:
 *      unit  - SOC unit number
 *      tid   - Trunk ID
 *      vp_lag_id - (OUT) VP LAG ID
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_tid_to_vp_lag_id(int unit, bcm_trunk_t tid, int *vp_lag_id)
{
    bcm_trunk_chip_info_t trunk_chip_info;

    if (tid < 0) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &trunk_chip_info));

    if (tid >= trunk_chip_info.vp_id_min && tid <= trunk_chip_info.vp_id_max) {
        *vp_lag_id = tid - trunk_chip_info.vp_id_min;
    } else {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_td2_vp_lag_create
 * Purpose:
 *      Create a VP LAG.
 * Parameters:
 *      unit  - SOC unit number
 *      flags - Flags
 *      tid   - (IN/OUT) Trunk ID
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_vp_lag_create(int unit, uint32 flags, bcm_trunk_t *tid)
{
    int rv = BCM_E_NONE;
    int vp_lag_id = 0;
    int i;
    int vp;
    egr_vplag_group_entry_t egr_vplag_group_entry;
    ecmp_count_entry_t l3_ecmp_count_entry;
    initial_l3_ecmp_group_entry_t initial_l3_ecmp_group_entry;
    ing_dvp_2_table_entry_t ing_dvp_2_entry;
    ing_dvp_table_entry_t ing_dvp_entry;
    bcm_trunk_chip_info_t trunk_chip_info;

    VP_LAG_INIT_CHECK(unit);

    if (flags & BCM_TRUNK_FLAG_WITH_ID) {
        BCM_IF_ERROR_RETURN(_bcm_td2_tid_to_vp_lag_id(unit, *tid, &vp_lag_id)); 
        if (VP_LAG_USED_GET(unit, vp_lag_id)) {
            return BCM_E_EXISTS;
        }
        VP_LAG_USED_SET(unit, vp_lag_id);
    } else if (flags & BCM_TRUNK_FLAG_VP) {
        for (i = 0; i < MAX_VP_LAGS(unit); i++) {
            if (!VP_LAG_USED_GET(unit, i)) {
                vp_lag_id = i;
                VP_LAG_USED_SET(unit, i);
                break;
            }
        }
        if (i == MAX_VP_LAGS(unit)) {
            return BCM_E_FULL;
        }
    } else {
        return BCM_E_PARAM;
    }

    /* In Trident2, VP LAGs and VPs share the same namespace.
     * A VP value needs to be allocated to represent a VP LAG, and this
     * VP value is not available to be used as a VP.
     */
    rv = _bcm_vp_alloc(unit, 0, soc_mem_index_count(unit, SOURCE_VPm) - 1,
            1, SOURCE_VPm, _bcmVpTypeVpLag, &vp);
    if (BCM_FAILURE(rv)) {
        VP_LAG_USED_CLR(unit, vp_lag_id);
        return rv;
    }
    VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id = vp;

    /* Trident2 supports up to 256 VP LAGs that are capable of performing
     * both unicast and multicast VP LAG resolution. Trident2 supports an
     * additional 768 VP LAGs that are capable of performing only unicast
     * VP LAG resolution. The VP LAG tables responsible for multicast
     * resolution are located in the egress pipeline; these tables
     * should be configured only for VP LAGs < 256. The VP LAGs tables
     * responsible for unicast resolution are located in the ingress
     * pipeline; these tables should be configured for all VP LAGs.
     */

    /* Configure EGR_VP_LAG_GROUP entry. Set base pointer and count
     * fields to all ones, indicating no members.
     */
    if (vp_lag_id < soc_mem_index_count(unit, EGR_VPLAG_GROUPm)) {
        SOC_IF_ERROR_RETURN(READ_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ANY, vp_lag_id,
                    &egr_vplag_group_entry));
        soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry,
                DVP_LAG_IDf, vp);
        soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry, BASE_PTRf,
                (1 << soc_mem_field_length(unit, EGR_VPLAG_GROUPm, BASE_PTRf)) - 1);
        soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry, COUNTf,
                (1 << soc_mem_field_length(unit, EGR_VPLAG_GROUPm, COUNTf)) - 1);
        rv = WRITE_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ALL, vp_lag_id,
                &egr_vplag_group_entry);
        if (BCM_FAILURE(rv)) {
            VP_LAG_USED_CLR(unit, vp_lag_id);
            (void) _bcm_vp_free(unit, _bcmVpTypeVpLag, 1, vp);
            return rv;
        }
    }

    /* Set base pointer and count fields in L3_ECMP_COUNT
     * to all ones, to indicate there are no members in this VP LAG.
     */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, vp_lag_id,
                &l3_ecmp_count_entry));
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, BASE_PTRf,
            (1 << soc_mem_field_length(unit, L3_ECMP_COUNTm, BASE_PTRf)) - 1);
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, COUNTf,
            (1 << soc_mem_field_length(unit, L3_ECMP_COUNTm, COUNTf)) - 1);
    rv = WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL, vp_lag_id,
            &l3_ecmp_count_entry);
    if (BCM_FAILURE(rv)) {
        VP_LAG_USED_CLR(unit, vp_lag_id);
        (void) _bcm_vp_free(unit, _bcmVpTypeVpLag, 1, vp);
        return rv;
    }

    /* Set base pointer and count fields in INITIAL_L3_ECMP_GROUP
     * to all ones, to indicate there are no members in this VP LAG.
     */
    SOC_IF_ERROR_RETURN(READ_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ANY,
                vp_lag_id, &initial_l3_ecmp_group_entry));
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            BASE_PTRf, (1 << soc_mem_field_length(unit, INITIAL_L3_ECMP_GROUPm,
                    BASE_PTRf)) - 1);
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            COUNTf, (1 << soc_mem_field_length(unit, INITIAL_L3_ECMP_GROUPm,
                    COUNTf)) - 1);
    rv = WRITE_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ALL,
            vp_lag_id, &initial_l3_ecmp_group_entry);
    if (BCM_FAILURE(rv)) {
        VP_LAG_USED_CLR(unit, vp_lag_id);
        (void) _bcm_vp_free(unit, _bcmVpTypeVpLag, 1, vp);
        return rv;
    }

    /* Configure ING_DVP_2_TABLE */
    SOC_IF_ERROR_RETURN(READ_ING_DVP_2_TABLEm(unit, MEM_BLOCK_ANY, vp,
                &ing_dvp_2_entry));
    soc_ING_DVP_2_TABLEm_field32_set(unit, &ing_dvp_2_entry,
            ENABLE_VPLAG_RESOLUTIONf, 1);
    soc_ING_DVP_2_TABLEm_field32_set(unit, &ing_dvp_2_entry,
            DVP_GROUP_PTRf, vp_lag_id);
    rv = WRITE_ING_DVP_2_TABLEm(unit, MEM_BLOCK_ALL, vp, &ing_dvp_2_entry);
    if (BCM_FAILURE(rv)) {
        VP_LAG_USED_CLR(unit, vp_lag_id);
        (void) _bcm_vp_free(unit, _bcmVpTypeVpLag, 1, vp);
        return rv;
    }

    /* Configure ING_DVP_TABLE */
    SOC_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp,
                &ing_dvp_entry));
    soc_ING_DVP_TABLEm_field32_set(unit, &ing_dvp_entry,
            ENABLE_VPLAG_RESOLUTIONf, 1);
    soc_ING_DVP_TABLEm_field32_set(unit, &ing_dvp_entry,
            DVP_GROUP_PTRf, vp_lag_id);
    rv = WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &ing_dvp_entry);
    if (BCM_FAILURE(rv)) {
        VP_LAG_USED_CLR(unit, vp_lag_id);
        (void) _bcm_vp_free(unit, _bcmVpTypeVpLag, 1, vp);
        return rv;
    }

    /* Convert VP LAG ID to trunk ID */
    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &trunk_chip_info));
    *tid = vp_lag_id + trunk_chip_info.vp_id_min;

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_td2_vp_lag_member_ingress_set
 * Purpose:
 *      Set members of a VP LAG in ingress pipeline.
 * Parameters:
 *      unit - SOC unit number.
 *      vp_lag_id - VP LAG ID.
 *      trunk_info - Information on the trunk group.
 *      vp_count - Number of VPs.
 *      vp_array - Array of VPs.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_vp_lag_member_ingress_set(int unit, int vp_lag_id,
        bcm_trunk_info_t *trunk_info, int vp_count, int *vp_array)
{
    _bcm_l3_tbl_op_t tbl_op_data;
    int base_ptr;
    int i;
    ing_dvp_table_entry_t ing_dvp_entry;
    int next_hop_index;
    ecmp_entry_t l3_ecmp_entry;
    initial_l3_ecmp_entry_t initial_l3_ecmp_entry;
    ecmp_count_entry_t l3_ecmp_count_entry;
    int old_base_ptr, old_vp_count;
    initial_l3_ecmp_group_entry_t initial_l3_ecmp_group_entry;

    /* Find a contiguous region of L3_ECMP table that's large
     * enough to hold all of the members.
     */
    sal_memset(&tbl_op_data, 0, sizeof(_bcm_l3_tbl_op_t));
    tbl_op_data.width = vp_count;
    tbl_op_data.tbl_ptr = BCM_XGS3_L3_TBL_PTR(unit, ecmp); 
    tbl_op_data.oper_flags = _BCM_L3_SHR_TABLE_TRAVERSE_CONTROL; 
    tbl_op_data.entry_index = -1;
    BCM_IF_ERROR_RETURN(_bcm_xgs3_tbl_free_idx_get(unit, &tbl_op_data));
    base_ptr = tbl_op_data.entry_index;
    BCM_XGS3_L3_ENT_REF_CNT_INC(tbl_op_data.tbl_ptr, base_ptr, vp_count);

    /* Write members to L3_ECMP and INITIAL_L3_ECMP tables */
    for (i = 0; i < vp_count; i++) {
        /* Get VP's next hop index */
        SOC_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY,
                    vp_array[i], &ing_dvp_entry));
        next_hop_index = soc_ING_DVP_TABLEm_field32_get(unit, &ing_dvp_entry,
                NEXT_HOP_INDEXf);

        /* Write L3_ECMP table */
        sal_memset(&l3_ecmp_entry, 0, sizeof(l3_ecmp_entry));
        soc_L3_ECMPm_field32_set(unit, &l3_ecmp_entry, DVPf,
                vp_array[i]);
        soc_L3_ECMPm_field32_set(unit, &l3_ecmp_entry, NEXT_HOP_INDEXf,
                next_hop_index);
        SOC_IF_ERROR_RETURN(WRITE_L3_ECMPm(unit, MEM_BLOCK_ALL,
                    base_ptr + i, &l3_ecmp_entry)); 

        /* Write INITIAL_L3_ECMP table */
        sal_memset(&initial_l3_ecmp_entry, 0, sizeof(initial_l3_ecmp_entry));
        soc_INITIAL_L3_ECMPm_field32_set(unit, &initial_l3_ecmp_entry,
                NEXT_HOP_INDEXf, next_hop_index);
        SOC_IF_ERROR_RETURN(WRITE_INITIAL_L3_ECMPm(unit, MEM_BLOCK_ALL,
                    base_ptr + i, &initial_l3_ecmp_entry)); 
    }

    /* Write base pointer and counter to L3_ECMP_COUNT table */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, vp_lag_id,
                &l3_ecmp_count_entry));
    old_base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit,
            &l3_ecmp_count_entry, BASE_PTRf);
    old_vp_count = 1 + soc_L3_ECMP_COUNTm_field32_get(unit,
            &l3_ecmp_count_entry, COUNTf);
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, BASE_PTRf,
            base_ptr);
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, COUNTf,
            vp_count - 1);
    SOC_IF_ERROR_RETURN(WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL, vp_lag_id,
                &l3_ecmp_count_entry));

    /* Write base pointer and counter to INITIAL_L3_ECMP_GROUP table */
    SOC_IF_ERROR_RETURN(READ_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ANY,
                vp_lag_id, &initial_l3_ecmp_group_entry));
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            BASE_PTRf, base_ptr);
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            COUNTf, vp_count - 1);
    SOC_IF_ERROR_RETURN(WRITE_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ALL,
                vp_lag_id, &initial_l3_ecmp_group_entry));

    /* If VP LAG had members, free old entries in L3_ECMP and
     * INITIAL_L3_ECMP tables. 
     */
    if (VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member) {
        BCM_XGS3_L3_ENT_REF_CNT_DEC(tbl_op_data.tbl_ptr, old_base_ptr,
                old_vp_count);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_td2_vp_lag_member_egress_set
 * Purpose:
 *      Set members of a VP LAG in egress pipeline.
 * Parameters:
 *      unit - SOC unit number.
 *      vp_lag_id - VP LAG ID.
 *      trunk_info - Information on the trunk group.
 *      vp_count - Number of VPs.
 *      vp_array - Array of VPs.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_vp_lag_member_egress_set(int unit, int vp_lag_id,
        bcm_trunk_info_t *trunk_info, int vp_count, int *vp_array)
{
    int num_entries;
    int max_base_ptr, base_ptr;
    SHR_BITDCL occupied;
    int i;
    egr_vplag_member_entry_t egr_member_entry;
    egr_vplag_group_entry_t egr_vplag_group_entry;
    egr_dvp_attribute_entry_t egr_dvp_attr_entry;
    int old_base_ptr, old_num_entries;

    if (trunk_info->dlf_index != BCM_TRUNK_UNSPEC_INDEX) {
        /* Only one member of the VP LAG is eligible to receive non-unicast 
         * traffic.
         */
        if (trunk_info->dlf_index >= vp_count) {
            return BCM_E_PARAM;
        }
        num_entries = 1;
    } else {
        num_entries = vp_count;
    }

    /* Find a contiguous region of EGR_VPLAG_MEMBER table that's large
     * enough to hold all of the members.
     */
    max_base_ptr = soc_mem_index_count(unit, EGR_VPLAG_MEMBERm) - num_entries;
    for (base_ptr = 0; base_ptr <= max_base_ptr; base_ptr++) {
        /* Check if the contiguous region from base_ptr to
         * (base_ptr + num_entries - 1) is free. 
         */
        VP_LAG_EGR_MEMBER_TEST_RANGE(unit, base_ptr, num_entries, occupied); 
        if (!occupied) {
            break;
        }
    }
    if (base_ptr > max_base_ptr) {
        /* A contiguous region of the desired size could not be found. */
        return BCM_E_RESOURCE;
    }

    /* Write members to EGR_VPLAG_MEMBER table */
    for (i = 0; i < num_entries; i++) {
        sal_memset(&egr_member_entry, 0, sizeof(egr_member_entry));
        if (trunk_info->dlf_index != BCM_TRUNK_UNSPEC_INDEX) {
            soc_EGR_VPLAG_MEMBERm_field32_set(unit, &egr_member_entry, DVPf,
                    vp_array[trunk_info->dlf_index]);
        } else {
            soc_EGR_VPLAG_MEMBERm_field32_set(unit, &egr_member_entry, DVPf,
                    vp_array[i]);
        }
        SOC_IF_ERROR_RETURN(WRITE_EGR_VPLAG_MEMBERm(unit, MEM_BLOCK_ALL,
                    base_ptr + i, &egr_member_entry)); 
    }

    /* Set vp_lag_egr_member_bitmap */
    VP_LAG_EGR_MEMBER_USED_SET_RANGE(unit, base_ptr, num_entries);

    /* Write base pointer and count to EGR_VPLAG_GROUP table */
    SOC_IF_ERROR_RETURN(READ_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ANY, vp_lag_id,
                &egr_vplag_group_entry));
    old_base_ptr = soc_EGR_VPLAG_GROUPm_field32_get(unit,
            &egr_vplag_group_entry, BASE_PTRf);
    old_num_entries = 1 + soc_EGR_VPLAG_GROUPm_field32_get(unit,
            &egr_vplag_group_entry, COUNTf);
    soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry, BASE_PTRf,
            base_ptr);
    soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry, COUNTf,
            num_entries - 1);
    SOC_IF_ERROR_RETURN(WRITE_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ALL, vp_lag_id,
                &egr_vplag_group_entry));

    /* If VP LAG had members, free old entries in EGR_VPLAG_MEMBER table */
    if (VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member) {
        VP_LAG_EGR_MEMBER_USED_CLR_RANGE(unit, old_base_ptr, old_num_entries);
    }

    /* Configure EGR_DVP_ATTRIBUTE for each vp */
    for (i = 0; i < vp_count; i++) {
        SOC_IF_ERROR_RETURN(READ_EGR_DVP_ATTRIBUTEm(unit, MEM_BLOCK_ANY,
                    vp_array[i], &egr_dvp_attr_entry));
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &egr_dvp_attr_entry,
                VP_TYPEf, 0);
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &egr_dvp_attr_entry,
                COMMON__ENABLE_VPLAG_SRC_PRUNINGf, TRUE);
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &egr_dvp_attr_entry,
                COMMON__ENABLE_VPLAG_RESOLUTIONf, TRUE);
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &egr_dvp_attr_entry,
                COMMON__VPLAG_GROUP_PTRf, vp_lag_id);
        SOC_IF_ERROR_RETURN(WRITE_EGR_DVP_ATTRIBUTEm(unit, MEM_BLOCK_ALL,
                    vp_array[i], &egr_dvp_attr_entry));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_td2_vp_lag_member_set
 * Purpose:
 *      Set members of a VP LAG.
 * Parameters:
 *      unit - SOC unit number.
 *      vp_lag_id - VP LAG ID.
 *      trunk_info - Information on the trunk group.
 *      member_count - Number of members.
 *      member_array - Array of members.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The existing ports in the VP LAG will be replaced with new ones.
 */
STATIC int
_bcm_td2_vp_lag_member_set(int unit, int vp_lag_id,
        bcm_trunk_info_t *trunk_info, int member_count,
        bcm_trunk_member_t *member_array)
{
    int rv = BCM_E_NONE;
    bcm_gport_t *vp_array = NULL;
    int i;
    bcm_gport_t gport;

    /* Convert member_array to vp_array. For each member,
     * convert its source VP into source VP LAG in hardware.
     */
    vp_array = sal_alloc(member_count * sizeof(int), "VP array");
    if (NULL == vp_array) {
        return BCM_E_MEMORY;
    }
    for (i = 0; i < member_count; i++) {
        gport = member_array[i].gport;
        if (BCM_GPORT_IS_VLAN_PORT(gport)) {
            vp_array[i] = BCM_GPORT_VLAN_PORT_ID_GET(gport);
            rv = _bcm_esw_vlan_port_source_vp_lag_set(unit, gport,
                    VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id);
            if (BCM_FAILURE(rv)) {
                sal_free(vp_array);
                return rv;
            }
        } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
            vp_array[i] = BCM_GPORT_NIV_PORT_ID_GET(gport);
            rv = _bcm_esw_niv_port_source_vp_lag_set(unit, gport,
                    VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id);
            if (BCM_FAILURE(rv)) {
                sal_free(vp_array);
                return rv;
            }
        } else if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
            vp_array[i] = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
            rv = _bcm_esw_extender_port_source_vp_lag_set(unit, gport,
                    VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id);
            if (BCM_FAILURE(rv)) {
                sal_free(vp_array);
                return rv;
            }
        } else {
            sal_free(vp_array);
            return BCM_E_PARAM;
        }
    }

    /* Set members in egress pipeline for VP LAGs < 256 */
    if (vp_lag_id < soc_mem_index_count(unit, EGR_VPLAG_GROUPm)) {
        rv = _bcm_td2_vp_lag_member_egress_set(unit, vp_lag_id, trunk_info,
                member_count, vp_array);
        if (BCM_FAILURE(rv)) {
            sal_free(vp_array);
            return rv;
        }
    } else {
        /* VP LAGs <= 256 can only be unicast-only VP LAGs */
        if (trunk_info->dlf_index != BCM_TRUNK_UNSPEC_INDEX) {
            sal_free(vp_array);
            return BCM_E_PARAM;
        }
    }

    /* Set members in ingress pipeline */
    rv = _bcm_td2_vp_lag_member_ingress_set(unit, vp_lag_id, trunk_info,
            member_count, vp_array);
    if (BCM_FAILURE(rv)) {
        sal_free(vp_array);
        return rv;
    }

    sal_free(vp_array);
    return rv;
}

/*
 * Function:
 *	_bcm_td2_vp_lag_member_ingress_clear
 * Purpose:
 *      Clear members of a VP LAG in ingress pipeline.
 * Parameters:
 *      unit - SOC unit number.
 *      vp_lag_id - VP LAG ID.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_vp_lag_member_ingress_clear(int unit, int vp_lag_id)
{
    initial_l3_ecmp_group_entry_t initial_l3_ecmp_group_entry;
    ecmp_count_entry_t l3_ecmp_count_entry;
    int base_ptr, vp_count;
    int i;

    /* Set base pointer and count fields in INITIAL_L3_ECMP_GROUP
     * to all ones, to indicate that there are no members.
     */
    SOC_IF_ERROR_RETURN(READ_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ANY,
                vp_lag_id, &initial_l3_ecmp_group_entry));
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            BASE_PTRf, (1 << soc_mem_field_length(unit, INITIAL_L3_ECMP_GROUPm,
                    BASE_PTRf)) - 1);
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            COUNTf, (1 << soc_mem_field_length(unit, INITIAL_L3_ECMP_GROUPm,
                    COUNTf)) - 1);
    SOC_IF_ERROR_RETURN(WRITE_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ALL,
                vp_lag_id, &initial_l3_ecmp_group_entry));

    /* Set base pointer and count fields in L3_ECMP_COUNT
     * to all ones, to indicate that there are no members.
     */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, vp_lag_id,
                &l3_ecmp_count_entry));
    base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit, &l3_ecmp_count_entry,
            BASE_PTRf);
    vp_count = 1 + soc_L3_ECMP_COUNTm_field32_get(unit, &l3_ecmp_count_entry,
            COUNTf);
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, BASE_PTRf,
            (1 << soc_mem_field_length(unit, L3_ECMP_COUNTm, BASE_PTRf)) - 1);
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, COUNTf,
            (1 << soc_mem_field_length(unit, L3_ECMP_COUNTm, COUNTf)) - 1);
    SOC_IF_ERROR_RETURN(WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL, vp_lag_id,
                &l3_ecmp_count_entry));

    /* Clear INITIAL_L3_ECMP and L3_ECMP entries */
    for (i = 0; i < vp_count; i++) {
        SOC_IF_ERROR_RETURN(WRITE_INITIAL_L3_ECMPm(unit, MEM_BLOCK_ALL,
                    base_ptr + i, soc_mem_entry_null(unit, INITIAL_L3_ECMPm)));
        SOC_IF_ERROR_RETURN(WRITE_L3_ECMPm(unit, MEM_BLOCK_ALL,
                    base_ptr + i, soc_mem_entry_null(unit, L3_ECMPm)));
    }

    /* Decrement reference count of INITIAL_L3_ECMP and L3_ECMP entries */
    BCM_XGS3_L3_ENT_REF_CNT_DEC(BCM_XGS3_L3_TBL_PTR(unit, ecmp), base_ptr,
            vp_count);

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_td2_vp_lag_member_egress_clear
 * Purpose:
 *      Clear members of a VP LAG in egress pipeline.
 * Parameters:
 *      unit - SOC unit number.
 *      vp_lag_id - VP LAG ID.
 *      vp_count - Number of elements in vp_array.
 *      vp_array - Array of VPs in VP LAG.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_vp_lag_member_egress_clear(int unit, int vp_lag_id,
        int vp_count, int *vp_array)
{
    int i;
    egr_dvp_attribute_entry_t egr_dvp_attr_entry;
    egr_vplag_group_entry_t egr_vplag_group_entry;
    int base_ptr, num_entries;

    /* Clear EGR_DVP_ATTRIBUTE entry for each vp */
    for (i = 0; i < vp_count; i++) {
        SOC_IF_ERROR_RETURN(READ_EGR_DVP_ATTRIBUTEm(unit, MEM_BLOCK_ANY,
                    vp_array[i], &egr_dvp_attr_entry));
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &egr_dvp_attr_entry,
                COMMON__ENABLE_VPLAG_SRC_PRUNINGf, FALSE);
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &egr_dvp_attr_entry,
                COMMON__ENABLE_VPLAG_RESOLUTIONf, FALSE);
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &egr_dvp_attr_entry,
                COMMON__VPLAG_GROUP_PTRf, 0);
        SOC_IF_ERROR_RETURN(WRITE_EGR_DVP_ATTRIBUTEm(unit, MEM_BLOCK_ALL,
                    vp_array[i], &egr_dvp_attr_entry));
    }

    /* Set base pointer and count fields in EGR_VPLAG_GROUP
     * to all ones, to indicate that there are no members.
     */
    SOC_IF_ERROR_RETURN(READ_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ANY, vp_lag_id,
                &egr_vplag_group_entry));
    base_ptr = soc_EGR_VPLAG_GROUPm_field32_get(unit,
            &egr_vplag_group_entry, BASE_PTRf);
    num_entries = 1 + soc_EGR_VPLAG_GROUPm_field32_get(unit,
            &egr_vplag_group_entry, COUNTf);
    soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry, BASE_PTRf,
            (1 << soc_mem_field_length(unit, EGR_VPLAG_GROUPm, BASE_PTRf)) - 1);
    soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry, COUNTf,
            (1 << soc_mem_field_length(unit, EGR_VPLAG_GROUPm, COUNTf)) - 1);
    SOC_IF_ERROR_RETURN(WRITE_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ALL, vp_lag_id,
                &egr_vplag_group_entry));

    /* Clear EGR_VPLAG_MEMBER entries */
    for (i = 0; i < num_entries; i++) {
        SOC_IF_ERROR_RETURN(WRITE_EGR_VPLAG_MEMBERm(unit, MEM_BLOCK_ALL,
                    base_ptr + i, soc_mem_entry_null(unit, EGR_VPLAG_MEMBERm)));
    }

    VP_LAG_EGR_MEMBER_USED_CLR_RANGE(unit, base_ptr, num_entries);

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_td2_vp_lag_member_clear
 * Purpose:
 *      Clear members of a VP LAG.
 * Parameters:
 *      unit - SOC unit number.
 *      vp_lag_id - VP LAG ID.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_vp_lag_member_clear(int unit, int vp_lag_id)
{
    int rv = BCM_E_NONE;
    ecmp_entry_t l3_ecmp_entry;
    ecmp_count_entry_t l3_ecmp_count_entry;
    int base_ptr, vp_count;
    int *vp_array = NULL;
    int i;
    bcm_gport_t gport;

    /* Get VP LAG's members. For each member, convert its source VP LAG back
     * to source VP in hardware.
     */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, vp_lag_id,
                &l3_ecmp_count_entry));
    base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit, &l3_ecmp_count_entry,
            BASE_PTRf);
    vp_count = 1 + soc_L3_ECMP_COUNTm_field32_get(unit, &l3_ecmp_count_entry,
            COUNTf);
    if (vp_count < 1) {
        return BCM_E_INTERNAL;
    }
    vp_array = sal_alloc(vp_count * sizeof(int), "VP array");
    if (NULL == vp_array) {
        return BCM_E_MEMORY;
    }
    for (i = 0; i < vp_count; i++) {
        rv = READ_L3_ECMPm(unit, MEM_BLOCK_ANY, base_ptr + i,
                &l3_ecmp_entry);
        if (BCM_FAILURE(rv)) {
            sal_free(vp_array);
            return rv;
        }
        vp_array[i] = soc_L3_ECMPm_field32_get(unit, &l3_ecmp_entry, DVPf);

        /* Convert source VP LAG back to source VP in harware. */
        if (_bcm_vp_used_get(unit, vp_array[i], _bcmVpTypeVlan)) {
            BCM_GPORT_VLAN_PORT_ID_SET(gport, vp_array[i]);
            rv = _bcm_esw_vlan_port_source_vp_lag_clear(unit, gport,
                    VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id);
            if (BCM_FAILURE(rv)) {
                sal_free(vp_array);
                return rv;
            }
        } else if (_bcm_vp_used_get(unit, vp_array[i], _bcmVpTypeNiv)) {
            BCM_GPORT_NIV_PORT_ID_SET(gport, vp_array[i]);
            rv = _bcm_esw_niv_port_source_vp_lag_clear(unit, gport,
                    VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id);
            if (BCM_FAILURE(rv)) {
                sal_free(vp_array);
                return rv;
            }
        } else if (_bcm_vp_used_get(unit, vp_array[i], _bcmVpTypeExtender)) {
            BCM_GPORT_EXTENDER_PORT_ID_SET(gport, vp_array[i]);
            rv = _bcm_esw_extender_port_source_vp_lag_clear(unit, gport,
                    VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id);
            if (BCM_FAILURE(rv)) {
                sal_free(vp_array);
                return rv;
            }
        } else {
            sal_free(vp_array);
            return BCM_E_INTERNAL;
        }
    }

    /* Clear members in ingress pipeline */
    rv = _bcm_td2_vp_lag_member_ingress_clear(unit, vp_lag_id);
    if (BCM_FAILURE(rv)) {
        sal_free(vp_array);
        return rv;
    }

    /* Clear members in egress pipeline for VP LAGs < 256 */
    if (vp_lag_id < soc_mem_index_count(unit, EGR_VPLAG_GROUPm)) {
        rv = _bcm_td2_vp_lag_member_egress_clear(unit, vp_lag_id, vp_count,
                vp_array);
        if (BCM_FAILURE(rv)) {
            sal_free(vp_array);
            return rv;
        }
    }

    sal_free(vp_array);
    return rv;
}

/*
 * Function:
 *	bcm_td2_vp_lag_set
 * Purpose:
 *      Set properties and members of a VP LAG.
 * Parameters:
 *      unit - SOC unit number.
 *      tid - The trunk ID to be affected.
 *      trunk_info - Information on the trunk group.
 *      member_count - Number of members.
 *      member_array - Array of members.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_vp_lag_set(int unit, bcm_trunk_t tid, bcm_trunk_info_t *trunk_info,
        int member_count, bcm_trunk_member_t *member_array)
{
    bcm_trunk_chip_info_t trunk_chip_info;
    int vp_lag_id;

    VP_LAG_INIT_CHECK(unit);

    BCM_IF_ERROR_RETURN(_bcm_td2_tid_to_vp_lag_id(unit, tid, &vp_lag_id)); 
    if (!VP_LAG_USED_GET(unit, vp_lag_id)) {
        return BCM_E_NOT_FOUND;
    }

    if (NULL == trunk_info) {
        return BCM_E_PARAM;
    }
    if (trunk_info->psc != BCM_TRUNK_PSC_PORTFLOW) {
        /* Only RTAG7 is supported for VP LAG */
        return BCM_E_PARAM;
    }
    if ((trunk_info->dlf_index != trunk_info->mc_index) ||
        (trunk_info->dlf_index != trunk_info->ipmc_index)) {
        return BCM_E_PARAM;
    }
    if ((trunk_info->flags != 0) ||
        (trunk_info->ipmc_psc != 0) ||
        (trunk_info->dynamic_size != 0) ||
        (trunk_info->dynamic_age != 0) ||
        (trunk_info->dynamic_load_exponent != 0) ||
        (trunk_info->dynamic_expected_load_exponent != 0)) { 
        /* These fields should be zeroes */
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &trunk_chip_info));
    if (member_count > trunk_chip_info.vp_ports_max) {
        return BCM_E_PARAM;
    } else if (member_count > 0) {
        if (trunk_info->dlf_index >= member_count) {
            /* dlf_index out of range */
            return BCM_E_PARAM;
        }
        if (NULL == member_array) {
            return BCM_E_PARAM;
        }
    } else {
        if (NULL != member_array) {
            return BCM_E_PARAM;
        }
    }

    /* Clear existing members */
    if (VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member) {
        BCM_IF_ERROR_RETURN(_bcm_td2_vp_lag_member_clear(unit, vp_lag_id));
        VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member = FALSE;
    }

    /* Set new members */
    if (member_count > 0) {
        BCM_IF_ERROR_RETURN(_bcm_td2_vp_lag_member_set(unit, vp_lag_id,
                    trunk_info, member_count, member_array));
        VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member = TRUE;
    }

    VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index = trunk_info->dlf_index; 

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_td2_vp_lag_destroy
 * Purpose:
 *      Destroy a VP LAG.
 * Parameters:
 *      unit  - SOC unit number
 *      tid   - Trunk ID
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_vp_lag_destroy(int unit, bcm_trunk_t tid)
{
    int vp_lag_id;
    int vp;
    ecmp_count_entry_t l3_ecmp_count_entry;
    initial_l3_ecmp_group_entry_t initial_l3_ecmp_group_entry;
    ing_dvp_table_entry_t ing_dvp_entry;
    ing_dvp_2_table_entry_t ing_dvp_2_entry;
    egr_vplag_group_entry_t egr_vplag_group_entry;

    VP_LAG_INIT_CHECK(unit);

    BCM_IF_ERROR_RETURN(_bcm_td2_tid_to_vp_lag_id(unit, tid, &vp_lag_id)); 
    if (!VP_LAG_USED_GET(unit, vp_lag_id)) {
        return BCM_E_NOT_FOUND;
    }

    if (VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member) {
        BCM_IF_ERROR_RETURN(_bcm_td2_vp_lag_member_clear(unit, vp_lag_id));
        VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member = FALSE;
    }
    VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index = 0; 

    /* Get the VP used to represent this VP LAG */
    vp = VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id;

    /* Clear VP LAG info in ING_DVP_TABLE */
    SOC_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp,
                &ing_dvp_entry));
    soc_ING_DVP_TABLEm_field32_set(unit, &ing_dvp_entry,
            ENABLE_VPLAG_RESOLUTIONf, 0);
    soc_ING_DVP_TABLEm_field32_set(unit, &ing_dvp_entry,
            DVP_GROUP_PTRf, 0);
    SOC_IF_ERROR_RETURN(WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp,
                &ing_dvp_entry));

    /* Clear VP LAG info in ING_DVP_2_TABLE */
    SOC_IF_ERROR_RETURN(READ_ING_DVP_2_TABLEm(unit, MEM_BLOCK_ANY, vp,
                &ing_dvp_2_entry));
    soc_ING_DVP_2_TABLEm_field32_set(unit, &ing_dvp_2_entry,
            ENABLE_VPLAG_RESOLUTIONf, 0);
    soc_ING_DVP_2_TABLEm_field32_set(unit, &ing_dvp_2_entry,
            DVP_GROUP_PTRf, 0);
    SOC_IF_ERROR_RETURN(WRITE_ING_DVP_2_TABLEm(unit, MEM_BLOCK_ALL, vp,
                &ing_dvp_2_entry));

    /* Clear VP LAG info in INITIAL_L3_ECMP_GROUP entry */
    SOC_IF_ERROR_RETURN(READ_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ANY,
                vp_lag_id, &initial_l3_ecmp_group_entry));
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            BASE_PTRf, 0);
    soc_INITIAL_L3_ECMP_GROUPm_field32_set(unit, &initial_l3_ecmp_group_entry,
            COUNTf, 0);
    SOC_IF_ERROR_RETURN(WRITE_INITIAL_L3_ECMP_GROUPm(unit, MEM_BLOCK_ALL,
            vp_lag_id, &initial_l3_ecmp_group_entry));

    /* Clear VP LAG info in L3_ECMP_COUNT entry */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, vp_lag_id,
                &l3_ecmp_count_entry));
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, BASE_PTRf, 0);
    soc_L3_ECMP_COUNTm_field32_set(unit, &l3_ecmp_count_entry, COUNTf, 0);
    SOC_IF_ERROR_RETURN(WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL, vp_lag_id,
            &l3_ecmp_count_entry));

    /* Clear VP LAG info in EGR_VPLAG_GROUP entry */
    if (vp_lag_id < soc_mem_index_count(unit, EGR_VPLAG_GROUPm)) {
        SOC_IF_ERROR_RETURN(READ_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ANY,
                    vp_lag_id, &egr_vplag_group_entry));
        soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry,
                DVP_LAG_IDf, 0);
        soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry,
                BASE_PTRf, 0);
        soc_EGR_VPLAG_GROUPm_field32_set(unit, &egr_vplag_group_entry,
                COUNTf, 0);
        SOC_IF_ERROR_RETURN(WRITE_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ALL,
                    vp_lag_id, &egr_vplag_group_entry));
    }

    /* Free the VP used to represent the VP LAG */
    BCM_IF_ERROR_RETURN(_bcm_vp_free(unit, _bcmVpTypeVpLag, 1, vp));

    /* Free the VP LAG ID */
    VP_LAG_USED_CLR(unit, vp_lag_id);

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_td2_vp_lag_get
 * Purpose:
 *      Return information of given VP LAG.
 * Parameters:
 *      unit   - SOC unit number.
 *      tid    - Trunk ID.
 *      trunk_info   - (OUT) Place to store returned VP LAG info.
 *      member_max   - (IN) Size of member_array.
 *      member_array - (OUT) Place to store returned VP LAG members.
 *      member_count - (OUT) Place to store returned number of VP LAG members.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_vp_lag_get(int unit, bcm_trunk_t tid, bcm_trunk_info_t *trunk_info,
        int member_max, bcm_trunk_member_t *member_array, int *member_count)
{
    int vp_lag_id;
    ecmp_count_entry_t l3_ecmp_count_entry;
    ecmp_entry_t l3_ecmp_entry;
    int base_ptr, vp_count;
    int i;
    int vp;
    bcm_gport_t gport;

    VP_LAG_INIT_CHECK(unit);

    BCM_IF_ERROR_RETURN(_bcm_td2_tid_to_vp_lag_id(unit, tid, &vp_lag_id)); 
    if (!VP_LAG_USED_GET(unit, vp_lag_id)) {
        return BCM_E_NOT_FOUND;
    }

    if ((member_max > 0) && (NULL == member_array)) {
        return BCM_E_PARAM;
    }
    if ((member_max > 0) && (NULL == member_count)) {
        return BCM_E_PARAM;
    }

    if (NULL != trunk_info) {
        bcm_trunk_info_t_init(trunk_info);
        trunk_info->psc = BCM_TRUNK_PSC_PORTFLOW;
        trunk_info->dlf_index = VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index;
        trunk_info->mc_index = VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index;
        trunk_info->ipmc_index = VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index;
    }

    /* Get VP LAG members */
    if (VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member) {
        SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, vp_lag_id,
                    &l3_ecmp_count_entry));
        base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit, &l3_ecmp_count_entry,
                BASE_PTRf);
        vp_count = 1 + soc_L3_ECMP_COUNTm_field32_get(unit, &l3_ecmp_count_entry,
                COUNTf);
        if (vp_count < 1) {
            return BCM_E_INTERNAL;
        }

        if (member_max == 0) {
            if (NULL != member_count) {
                *member_count = vp_count;
            }
        } else {
            *member_count = 0;
            for (i = 0; i < vp_count && i < member_max; i++) {
                SOC_IF_ERROR_RETURN(READ_L3_ECMPm(unit, MEM_BLOCK_ANY,
                            base_ptr + i, &l3_ecmp_entry));
                vp = soc_L3_ECMPm_field32_get(unit, &l3_ecmp_entry, DVPf);

                /* Derive VP's GPORT ID */
                if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                    BCM_GPORT_VLAN_PORT_ID_SET(gport, vp);
                } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv)) {
                    BCM_GPORT_NIV_PORT_ID_SET(gport, vp);
                } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                    BCM_GPORT_EXTENDER_PORT_ID_SET(gport, vp);
                } else {
                    return BCM_E_INTERNAL;
                }

                bcm_trunk_member_t_init(&member_array[i]);
                member_array[i].gport = gport;
                (*member_count)++;
            }
        }
    } else { /* VP LAG has no members */
        if (NULL != member_count) {
            *member_count = 0;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_vp_lag_find
 * Description:
 *      Get trunk id that contains the given VP
 * Parameters:
 *      unit - SOC unit number
 *      gport - VP GPORT ID
 *      tid  - (OUT) Trunk id
 * Returns:
 *      BCM_E_xxxx
 */
int
bcm_td2_vp_lag_find(int unit, bcm_gport_t gport, bcm_trunk_t *tid)
{
    int vp_lag_vp;
    ing_dvp_table_entry_t ing_dvp_entry;
    int vp_lag_valid;
    int vp_lag_id;
    bcm_trunk_chip_info_t trunk_chip_info;

    VP_LAG_INIT_CHECK(unit);

    if (!BCM_GPORT_IS_SET(gport)) {
        return BCM_E_PORT;
    }

    /* Get the VP value representing VP LAG from VP's match entry, instead of
     * getting the VP LAG from EGR_DVP_ATTRIBUTE table, since only the
     * multicast capable VP LAGs can be retrieved from EGR_DVP_ATTRIBUTE.
     */
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_vlan_port_source_vp_lag_get(unit, gport, &vp_lag_vp));
    } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_niv_port_source_vp_lag_get(unit, gport, &vp_lag_vp));
    } else if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_extender_port_source_vp_lag_get(unit, gport, &vp_lag_vp));
    } else {
        return BCM_E_PORT;
    }

    /* Get VP LAG ID from ING_DVP_TABLE */
    SOC_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp_lag_vp,
                &ing_dvp_entry));
    vp_lag_valid = soc_ING_DVP_TABLEm_field32_get(unit, &ing_dvp_entry,
            ENABLE_VPLAG_RESOLUTIONf);
    if (!vp_lag_valid) {
        return BCM_E_INTERNAL;
    }
    vp_lag_id = soc_ING_DVP_TABLEm_field32_get(unit, &ing_dvp_entry,
            DVP_GROUP_PTRf);

    /* Convert VP LAG ID to trunk ID */
    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &trunk_chip_info));
    *tid = vp_lag_id + trunk_chip_info.vp_id_min;

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_td2_tid_to_vp_lag_vp
 * Purpose:
 *      Convert trunk ID to the VP value representing VP LAG.
 * Parameters:
 *      unit  - SOC unit number
 *      tid   - Trunk ID
 *      vp_lag_vp - (OUT) VP value
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_tid_to_vp_lag_vp(int unit, bcm_trunk_t tid, int *vp_lag_vp)
{
    int vp_lag_id;

    VP_LAG_INIT_CHECK(unit);

    BCM_IF_ERROR_RETURN(_bcm_td2_tid_to_vp_lag_id(unit, tid, &vp_lag_id)); 
    if (!VP_LAG_USED_GET(unit, vp_lag_id)) {
        return BCM_E_NOT_FOUND;
    }

    *vp_lag_vp = VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id;

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_td2_vp_lag_vp_to_tid
 * Purpose:
 *      Convert the VP value representing a VP LAG to trunk ID.
 * Parameters:
 *      unit      - SOC unit number
 *      vp_lag_vp - VP value
 *      tid       - (OUT) Trunk ID
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_vp_lag_vp_to_tid(int unit, int vp_lag_vp, bcm_trunk_t *tid)
{
    ing_dvp_table_entry_t ing_dvp_entry;
    int vp_lag_valid;
    int vp_lag_id;
    bcm_trunk_chip_info_t trunk_chip_info;

    VP_LAG_INIT_CHECK(unit);

    if (!_bcm_vp_used_get(unit, vp_lag_vp, _bcmVpTypeVpLag)) {
        return BCM_E_NOT_FOUND;
    }

    /* Get VP LAG ID from ING_DVP_TABLE */
    SOC_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp_lag_vp,
                &ing_dvp_entry));
    vp_lag_valid = soc_ING_DVP_TABLEm_field32_get(unit, &ing_dvp_entry,
            ENABLE_VPLAG_RESOLUTIONf);
    if (!vp_lag_valid) {
        return BCM_E_INTERNAL;
    }
    vp_lag_id = soc_ING_DVP_TABLEm_field32_get(unit, &ing_dvp_entry,
            DVP_GROUP_PTRf);

    /* Convert VP LAG ID to trunk ID */
    BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &trunk_chip_info));
    *tid = vp_lag_id + trunk_chip_info.vp_id_min;

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_td2_vp_lag_status_get
 * Purpose:
 *     Get VP LAG status.
 * Parameters:
 *     unit - SOC unit number
 *     vp_lag_id - VP LAG ID
 *     is_vp_lag - (OUT) Indicates if the given vp_lag_id is a valid VP LAG ID.
 *     is_used - (OUT) Indicates if the given vp_lag_id is being used.
 *     has_member - (OUT) Indicates if the given vp_lag_id has members.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_vp_lag_status_get(int unit, int vp_lag_id, int *is_vp_lag,
        int *is_used, int *has_member)
{
    if (NULL == is_vp_lag || NULL == is_used || NULL == has_member) {
        return BCM_E_PARAM;
    }

    *is_vp_lag = FALSE;
    *is_used = FALSE;
    *has_member = FALSE;

    if (vp_lag_id >= 0 && vp_lag_id < MAX_VP_LAGS(unit)) {
        *is_vp_lag = TRUE;
        if (VP_LAG_USED_GET(unit, vp_lag_id)) {
            *is_used = TRUE;
            if (VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member) {
                *has_member = TRUE;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_esw_num_vp_trunk_groups
 * Purpose:
 *     Get Number of VP LAG groups supported.
 * Parameters:
 *     unit - SOC unit number
 * Returns:
 *      Number of VP LAG groups supported.
 */
int _bcm_esw_num_vp_trunk_groups(int unit)
{
    return MAX_VP_LAGS(unit);
}

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Function:
 *      bcm_td2_vp_lag_reinit
 * Purpose:
 *      Recover VP LAG software state.
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_vp_lag_reinit(int unit)
{
    int rv = BCM_E_NONE;
    soc_mem_t mem;
    int entry_size;
    int chunk_size, num_chunks, chunk_index;
    int alloc_size;
    uint8 *buf = NULL;
    int i, j, entry_index_min, entry_index_max;
    uint32 *buf_entry;
    int vp_lag_id;
    int vp;
    ecmp_count_entry_t l3_ecmp_count_entry;
    ecmp_entry_t l3_ecmp_entry;
    int base_ptr;
    int count_field;
    int vp_count;
    egr_vplag_member_entry_t egr_member_entry;
    egr_vplag_group_entry_t egr_vplag_group_entry;
    int egr_base_ptr;
    int egr_vp_count;
    int non_uc_member;
    int stable_size;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* Traverse ING_DVP_TABLE to find valid VP LAGs */
    mem = ING_DVP_TABLEm;
    entry_size = sizeof(ing_dvp_table_entry_t);

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, mem) / chunk_size;
    if (soc_mem_index_count(unit, mem) % chunk_size) {
        num_chunks++;
    }

    alloc_size = entry_size * chunk_size;
    buf = soc_cm_salloc(unit, alloc_size, "ING_DVP_TABLE buffer");
    if (NULL == buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }

    for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {

        /* Get a chunk of entries */
        entry_index_min = chunk_index * chunk_size;
        entry_index_max = entry_index_min + chunk_size - 1;
        if (entry_index_max > soc_mem_index_max(unit, mem)) {
            entry_index_max = soc_mem_index_max(unit, mem);
        }
        rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY,
                entry_index_min, entry_index_max, buf);
        if (SOC_FAILURE(rv)) {
            goto cleanup;
        }

        /* Read each entry of the chunk to extract VP LAG info */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            buf_entry = soc_mem_table_idx_to_pointer(unit,
                    mem, uint32 *, buf, i);
            if (soc_mem_field32_get(unit, mem, buf_entry,
                        ENABLE_VPLAG_RESOLUTIONf) == 0) {
                continue;
            }
            vp_lag_id = soc_mem_field32_get(unit, mem, buf_entry,
                    DVP_GROUP_PTRf);
            if (vp_lag_id >= MAX_VP_LAGS(unit)) {
                /* The configuration property MAX_VP_LAGS was not
                 *  set correctly.
                 */
                rv = BCM_E_CONFIG;
                goto cleanup;
            }

            /* Recover bitmap of VP LAG IDs */
            VP_LAG_USED_SET(unit, vp_lag_id);

            /* Recover VP LAG's VP value */
            vp = entry_index_min + i;
            VP_LAG_GROUP_INFO(unit, vp_lag_id).vp_id = vp;
            if ((stable_size == 0) || SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
                rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeVpLag);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
            } else {
                /* In level 2 warm boot, the VP LAG VP bitmap should have been
                 * recovered by virtual_init.
                 */
                 if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVpLag)) {
                     rv = BCM_E_INTERNAL;
                     goto cleanup;
                 }
            }

            /* Recover if VP LAG has any members */
            rv = READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, vp_lag_id,
                    &l3_ecmp_count_entry);
            if (SOC_FAILURE(rv)) {
                goto cleanup;
            }
            base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit,
                    &l3_ecmp_count_entry, BASE_PTRf);
            count_field = soc_L3_ECMP_COUNTm_field32_get(unit,
                    &l3_ecmp_count_entry, COUNTf);
            vp_count = count_field + 1;
            if (base_ptr == ((1 << soc_mem_field_length(unit,
                                L3_ECMP_COUNTm, BASE_PTRf)) - 1) &&
                count_field == ((1 << soc_mem_field_length(unit,
                                L3_ECMP_COUNTm, COUNTf)) - 1)) {
                /* BASE_PTR and COUNT fields being all ones indicates
                 * this VP LAG has no members.
                 */
                VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member = FALSE;
            } else {
                VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member = TRUE;
            }

            if (!VP_LAG_GROUP_INFO(unit, vp_lag_id).has_member) {
                continue;
            }

            /* Recover non-unicast member index and bitmap of
             * used entries of EGR_VPLAG_MEMBER table
             */
            if (vp_lag_id >= soc_mem_index_count(unit, EGR_VPLAG_GROUPm)) {
                /* VP LAG is unicast only */
                VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index =
                    BCM_TRUNK_UNSPEC_INDEX;
            } else {
                rv = READ_EGR_VPLAG_GROUPm(unit, MEM_BLOCK_ANY, vp_lag_id,
                        &egr_vplag_group_entry);
                if (SOC_FAILURE(rv)) {
                    goto cleanup;
                }
                egr_base_ptr = soc_EGR_VPLAG_GROUPm_field32_get(unit,
                        &egr_vplag_group_entry, BASE_PTRf);
                egr_vp_count = 1 + soc_EGR_VPLAG_GROUPm_field32_get(unit,
                        &egr_vplag_group_entry, COUNTf);
                VP_LAG_EGR_MEMBER_USED_SET_RANGE(unit, egr_base_ptr,
                        egr_vp_count);
                if (egr_vp_count == 1 && vp_count > egr_vp_count) {
                    rv = READ_EGR_VPLAG_MEMBERm(unit, MEM_BLOCK_ANY,
                            egr_base_ptr, &egr_member_entry);
                    if (SOC_FAILURE(rv)) {
                        goto cleanup;
                    }
                    non_uc_member = soc_EGR_VPLAG_MEMBERm_field32_get(unit,
                            &egr_member_entry, DVPf);
                    for (j = 0; j < vp_count; j++) {
                        rv = READ_L3_ECMPm(unit, MEM_BLOCK_ANY, base_ptr + j,
                                &l3_ecmp_entry);
                        if (SOC_FAILURE(rv)) {
                            goto cleanup;
                        }
                        if (non_uc_member == soc_L3_ECMPm_field32_get(unit,
                                    &l3_ecmp_entry, DVPf)) {
                            VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index = j;
                            break;
                        }
                    }
                    if (j == vp_count) {
                        /* non_uc_member not found */
                        rv = BCM_E_INTERNAL;
                        goto cleanup;
                    }
                } else {
                    VP_LAG_GROUP_INFO(unit, vp_lag_id).non_uc_index =
                        BCM_TRUNK_UNSPEC_INDEX;
                }
            }
        }
    }

cleanup:
    if (buf) {
        soc_cm_sfree(unit, buf);
    }

    return rv;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     bcm_td2_vp_lag_sw_dump
 * Purpose:
 *     Displays VP LAG information maintained by software.
 * Parameters:
 *     unit - SOC unit number
 * Returns:
 *     None
 */
void
bcm_td2_vp_lag_sw_dump(int unit)
{
    bcm_trunk_chip_info_t trunk_chip_info;
    int i;

    LOG_CLI((BSL_META_U(unit,
                        "VP LAG Information:\n")));
    LOG_CLI((BSL_META_U(unit,
                        "  Max number of VP LAGs: %d\n"), MAX_VP_LAGS(unit)));

    (void)bcm_esw_trunk_chip_info_get(unit, &trunk_chip_info);
    LOG_CLI((BSL_META_U(unit,
                        "  VP LAG trunk ID min:   %d\n"), trunk_chip_info.vp_id_min));
    LOG_CLI((BSL_META_U(unit,
                        "  VP LAG trunk ID max:   %d\n"), trunk_chip_info.vp_id_max));
    LOG_CLI((BSL_META_U(unit,
                        "  VP LAG max members:    %d\n"), trunk_chip_info.vp_ports_max));

    for (i = 0; i < MAX_VP_LAGS(unit); i++) {
        if (VP_LAG_USED_GET(unit, i)) {
            LOG_CLI((BSL_META_U(unit,
                                "  VP LAG %d: vp_id = %d, "), i,
                     VP_LAG_GROUP_INFO(unit, i).vp_id));
            LOG_CLI((BSL_META_U(unit,
                                "has_member = %d, "),
                     VP_LAG_GROUP_INFO(unit, i).has_member));
            LOG_CLI((BSL_META_U(unit,
                                "non_uc_index = %d\n"),
                     VP_LAG_GROUP_INFO(unit, i).non_uc_index));
        }
    }

    return;
}

#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* INCLUDE_L3 */

#endif /* BCM_TRIDENT2_SUPPORT */
