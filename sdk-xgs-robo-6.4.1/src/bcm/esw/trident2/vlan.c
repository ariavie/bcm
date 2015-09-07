/*
 * $Id: vlan.c,v 1.3 Broadcom SDK $
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
 * File:    vlan.c
 * Purpose: Handle trident2 specific vlan features:
 *             Manages VP VLAN membership tables.
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3) 

#include <soc/mem.h>
#include <soc/hash.h>
#include <soc/ptable.h>
#include <soc/mcm/memacc.h>
#include <bcm/error.h>

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/api_xlate_port.h>

#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/ipmc.h>

#define FLAG2HW_STP_STATE(_f) \
	((_f) == BCM_VLAN_GPORT_ADD_STP_DISABLE ? 0: \
	 (_f) == BCM_VLAN_GPORT_ADD_STP_BLOCK ?   1: \
	 (_f) == BCM_VLAN_GPORT_ADD_STP_LEARN ?   2: 3) 

/*
 * Function:
 *      bcm_td2_ing_vp_vlan_membership_add
 * Purpose:
 *      create a ING_VP_VLAN_MEMBERSHIP entry
 *      
 * Parameters:
 *      unit - (IN) BCM device number
 *      vp   - (IN) VP number
 *      vlan - (IN) VLAN 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_ing_vp_vlan_membership_add(int unit, int vp, bcm_vlan_t vlan, uint32 flags)
{
    int rv = BCM_E_NONE;
    ing_vp_vlan_membership_entry_t ivvm_ent;
    ing_vp_vlan_membership_entry_t ivvm_ent_result;
    int index;

    sal_memset(&ivvm_ent, 0, sizeof(ivvm_ent));

    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VPf, vp);
    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VLANf, vlan);
    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, SP_TREEf, 
              FLAG2HW_STP_STATE(flags & BCM_VLAN_GPORT_ADD_STP_MASK));
    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VALIDf, 1);

    rv = soc_mem_search(unit, ING_VP_VLAN_MEMBERSHIPm, MEM_BLOCK_ANY, 
                 &index, &ivvm_ent, &ivvm_ent_result, 0);

    if (rv == SOC_E_NONE) {
        /* found the entry, update the data field and write back  */
        soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent_result, SP_TREEf, 
              FLAG2HW_STP_STATE(flags & BCM_VLAN_GPORT_ADD_STP_MASK));
        rv = soc_mem_write(unit, ING_VP_VLAN_MEMBERSHIPm,
                          MEM_BLOCK_ALL, index, &ivvm_ent_result);
        
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;  /* error return */
    } else {
        /* not found, add to the table */ 
        rv = soc_mem_insert(unit, ING_VP_VLAN_MEMBERSHIPm, 
                       MEM_BLOCK_ALL, &ivvm_ent);
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_egr_vp_vlan_membership_add
 * Purpose:
 *      create a EGR_VP_VLAN_MEMBERSHIP entry
 *      
 * Parameters:
 *      unit - (IN) BCM device number
 *      vp   - (IN) VP number
 *      vlan - (IN) VLAN 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_egr_vp_vlan_membership_add(int unit, int vp, bcm_vlan_t vlan,uint32 flags)
{
    int rv = BCM_E_NONE;
    egr_vp_vlan_membership_entry_t evvm_ent;
    egr_vp_vlan_membership_entry_t evvm_ent_result;
    int index;

    sal_memset(&evvm_ent, 0, sizeof(evvm_ent));

    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VPf, vp);
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VLANf, vlan);
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VALIDf, 1);
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, SP_TREEf, 
              FLAG2HW_STP_STATE(flags & BCM_VLAN_GPORT_ADD_STP_MASK));
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, UNTAGf, 
              (flags & BCM_VLAN_PORT_UNTAGGED)? 1:0);

    rv = soc_mem_search(unit, EGR_VP_VLAN_MEMBERSHIPm, MEM_BLOCK_ANY, 
                 &index, &evvm_ent, &evvm_ent_result, 0);

    if (rv == SOC_E_NONE) {
        /* found the entry, update  */
        soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, 
              &evvm_ent_result, SP_TREEf, 
              FLAG2HW_STP_STATE(flags & BCM_VLAN_GPORT_ADD_STP_MASK));
        soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, 
               &evvm_ent_result, UNTAGf, 
              (flags & BCM_VLAN_PORT_UNTAGGED)? 1:0);
        rv = soc_mem_write(unit, EGR_VP_VLAN_MEMBERSHIPm,
                          MEM_BLOCK_ALL, index, &evvm_ent_result);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;  /* error return */
    } else {
        /* not found, add to the table */ 
        rv = soc_mem_insert(unit, EGR_VP_VLAN_MEMBERSHIPm, 
                       MEM_BLOCK_ALL, &evvm_ent);
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_ing_vp_vlan_membership_delete
 * Purpose:
 *      delete a ING_VP_VLAN_MEMBERSHIP entry
 *      
 * Parameters:
 *      unit - (IN) BCM device number
 *      vp   - (IN) VP number
 *      vlan - (IN) VLAN 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_ing_vp_vlan_membership_delete(int unit, int vp, bcm_vlan_t vlan)
{
    int rv = BCM_E_NONE;
    ing_vp_vlan_membership_entry_t ivvm_ent;

    sal_memset(&ivvm_ent, 0, sizeof(ivvm_ent));

    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VPf, vp);
    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VLANf, vlan);
    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VALIDf, 1);

    rv = soc_mem_delete_return_old(unit, ING_VP_VLAN_MEMBERSHIPm, 
                    MEM_BLOCK_ALL, &ivvm_ent, &ivvm_ent);

    return rv;
}

/*
 * Function:
 *      bcm_td2_egr_vp_vlan_membership_delete
 * Purpose:
 *      delete a EGR_VP_VLAN_MEMBERSHIP entry
 *
 * Parameters:
 *      unit - (IN) BCM device number
 *      vp   - (IN) VP number
 *      vlan - (IN) VLAN
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_egr_vp_vlan_membership_delete(int unit, int vp, bcm_vlan_t vlan)
{
    int rv = BCM_E_NONE;
    egr_vp_vlan_membership_entry_t evvm_ent;

    sal_memset(&evvm_ent, 0, sizeof(evvm_ent));

    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VPf, vp);
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VLANf, vlan);
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VALIDf, 1);

    rv = soc_mem_delete_return_old(unit, EGR_VP_VLAN_MEMBERSHIPm,
                    MEM_BLOCK_ALL, &evvm_ent, &evvm_ent);

    return rv;
}


/*
 * Function:
 *      bcm_td2_ing_vp_vlan_membership_get
 * Purpose:
 *      get the STP state of this ING_VP_VLAN_MEMBERSHIP entry
 *      
 * Parameters:
 *      unit - (IN) BCM device number
 *      vp   - (IN) VP number
 *      vlan - (IN) VLAN 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_ing_vp_vlan_membership_get(int unit, int vp, bcm_vlan_t vlan, int *flags)
{
    ing_vp_vlan_membership_entry_t ivvm_ent;
    ing_vp_vlan_membership_entry_t ivvm_ent_result;
    int idx;
    int sp_state;

    *flags = 0;
    sal_memset(&ivvm_ent, 0, sizeof(ivvm_ent));

    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VPf, vp);
    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VLANf, vlan);
    soc_ING_VP_VLAN_MEMBERSHIPm_field32_set(unit, &ivvm_ent, VALIDf, 1);

    BCM_IF_ERROR_RETURN(soc_mem_search(unit, ING_VP_VLAN_MEMBERSHIPm, 
                         MEM_BLOCK_ALL, &idx,
                         &ivvm_ent, &ivvm_ent_result, 0));

    sp_state = soc_ING_VP_VLAN_MEMBERSHIPm_field32_get(unit, &ivvm_ent_result,
                                              SP_TREEf);
    if (sp_state == PVP_STP_DISABLED) {
        *flags = BCM_VLAN_GPORT_ADD_STP_DISABLE;
    } else if (sp_state == PVP_STP_LEARNING) {
        *flags = BCM_VLAN_GPORT_ADD_STP_LEARN;
    } else if (sp_state == PVP_STP_BLOCKING) {
        *flags = BCM_VLAN_GPORT_ADD_STP_BLOCK;
    } else {
        *flags = BCM_VLAN_GPORT_ADD_STP_FORWARD;
    }   
    
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_egr_vp_vlan_membership_get
 * Purpose:
 *      get the STP state of this EGR_VP_VLAN_MEMBERSHIP entry
 *      
 * Parameters:
 *      unit - (IN) BCM device number
 *      vp   - (IN) VP number
 *      vlan - (IN) VLAN 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_egr_vp_vlan_membership_get(int unit, int vp, bcm_vlan_t vlan, int *flags)
{
    egr_vp_vlan_membership_entry_t evvm_ent;
    egr_vp_vlan_membership_entry_t evvm_ent_result;
    int idx;
    int sp_state;
    int untag;

    *flags = 0;
    sal_memset(&evvm_ent, 0, sizeof(evvm_ent));

    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VPf, vp);
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VLANf, vlan);
    soc_EGR_VP_VLAN_MEMBERSHIPm_field32_set(unit, &evvm_ent, VALIDf, 1);

    BCM_IF_ERROR_RETURN(soc_mem_search(unit, EGR_VP_VLAN_MEMBERSHIPm, 
                         MEM_BLOCK_ALL, &idx,
                         &evvm_ent, &evvm_ent_result, 0));

    sp_state = soc_EGR_VP_VLAN_MEMBERSHIPm_field32_get(unit, &evvm_ent_result,
                                              SP_TREEf);
    if (sp_state == PVP_STP_DISABLED) {
        *flags = BCM_VLAN_GPORT_ADD_STP_DISABLE;
    } else if (sp_state == PVP_STP_LEARNING) {
        *flags = BCM_VLAN_GPORT_ADD_STP_LEARN;
    } else if (sp_state == PVP_STP_BLOCKING) {
        *flags = BCM_VLAN_GPORT_ADD_STP_BLOCK;
    } else {
        *flags = BCM_VLAN_GPORT_ADD_STP_FORWARD;
    }   
    untag = soc_EGR_VP_VLAN_MEMBERSHIPm_field32_get(unit, &evvm_ent_result,
                                              UNTAGf);
    *flags |= untag? BCM_VLAN_PORT_UNTAGGED:0; 
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_ing_vp_vlan_membership_get_all
 * Purpose:
 *      For the given vlan, get all the virtual ports and flags from
 *      ING_VP_VLAN_MEMBERSHIP table.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN.
 *      vp_bitmap - (OUT) Bitmap of virtual ports.
 *      arr_size  - (IN) Number of elements in flags_arr.
 *      flags_arr - (OUT) Array of flags.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_ing_vp_vlan_membership_get_all(int unit, bcm_vlan_t vlan,
        SHR_BITDCL *vp_bitmap, int arr_size, int *flags_arr)
{
    int rv = BCM_E_NONE;
    soc_mem_t mem;
    int chunk_size, num_chunks, chunk_index;
    int alloc_size;
    uint8 *buf = NULL;
    int i, entry_index_min, entry_index_max;
    uint32 *buf_entry;
    int vp;
    int sp_state;

    if (NULL == vp_bitmap) {
        return BCM_E_PARAM;
    }
    if (arr_size != soc_mem_index_count(unit, SOURCE_VPm)) {
        return BCM_E_PARAM;
    } 
    if (NULL == flags_arr) {
        return BCM_E_PARAM;
    } 

    mem = ING_VP_VLAN_MEMBERSHIPm;

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, mem) / chunk_size;
    if (soc_mem_index_count(unit, mem) % chunk_size) {
        num_chunks++;
    }

    alloc_size = sizeof(ing_vp_vlan_membership_entry_t) * chunk_size;
    buf = soc_cm_salloc(unit, alloc_size, "ING_VP_VLAN_MEMBERSHIP buffer");
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

        /* Read each entry of the chunk to extract VP and flags */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            buf_entry = soc_mem_table_idx_to_pointer(unit,
                    mem, uint32 *, buf, i);

            if (soc_mem_field32_get(unit, mem, buf_entry, VALIDf) == 0) {
                continue;
            }
            if (soc_mem_field32_get(unit, mem, buf_entry, VLANf) != vlan) {
                continue;
            }

            vp = soc_mem_field32_get(unit, mem, buf_entry, VPf);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                continue;
            }
            SHR_BITSET(vp_bitmap, vp);

            sp_state = soc_mem_field32_get(unit, mem, buf_entry, SP_TREEf);
            if (sp_state == PVP_STP_DISABLED) {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_DISABLE;
            } else if (sp_state == PVP_STP_LEARNING) {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_LEARN;
            } else if (sp_state == PVP_STP_BLOCKING) {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_BLOCK;
            } else {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_FORWARD;
            }   
        }
    }

cleanup:
    if (buf) {
        soc_cm_sfree(unit, buf);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_egr_vp_vlan_membership_get_all
 * Purpose:
 *      For the given vlan, get all the virtual ports and flags from
 *      EGR_VP_VLAN_MEMBERSHIP table.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN.
 *      vp_bitmap - (OUT) Bitmap of virtual ports.
 *      arr_size  - (IN) Number of elements in flags_arr.
 *      flags_arr - (OUT) Array of flags.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_egr_vp_vlan_membership_get_all(int unit, bcm_vlan_t vlan,
        SHR_BITDCL *vp_bitmap, int arr_size, int *flags_arr)
{
    int rv = BCM_E_NONE;
    soc_mem_t mem;
    int chunk_size, num_chunks, chunk_index;
    int alloc_size;
    uint8 *buf = NULL;
    int i, entry_index_min, entry_index_max;
    uint32 *buf_entry;
    int vp;
    int sp_state;
    int untag;

    if (NULL == vp_bitmap) {
        return BCM_E_PARAM;
    }
    if (arr_size != soc_mem_index_count(unit, SOURCE_VPm)) {
        return BCM_E_PARAM;
    } 
    if (NULL == flags_arr) {
        return BCM_E_PARAM;
    } 

    mem = EGR_VP_VLAN_MEMBERSHIPm;

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, mem) / chunk_size;
    if (soc_mem_index_count(unit, mem) % chunk_size) {
        num_chunks++;
    }

    alloc_size = sizeof(egr_vp_vlan_membership_entry_t) * chunk_size;
    buf = soc_cm_salloc(unit, alloc_size, "EGR_VP_VLAN_MEMBERSHIP buffer");
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

        /* Read each entry of the chunk to extract VP and flags */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            buf_entry = soc_mem_table_idx_to_pointer(unit,
                    mem, uint32 *, buf, i);

            if (soc_mem_field32_get(unit, mem, buf_entry, VALIDf) == 0) {
                continue;
            }
            if (soc_mem_field32_get(unit, mem, buf_entry, VLANf) != vlan) {
                continue;
            }

            vp = soc_mem_field32_get(unit, mem, buf_entry, VPf);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                continue;
            }
            SHR_BITSET(vp_bitmap, vp);

            sp_state = soc_mem_field32_get(unit, mem, buf_entry, SP_TREEf);
            if (sp_state == PVP_STP_DISABLED) {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_DISABLE;
            } else if (sp_state == PVP_STP_LEARNING) {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_LEARN;
            } else if (sp_state == PVP_STP_BLOCKING) {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_BLOCK;
            } else {
                flags_arr[vp] = BCM_VLAN_GPORT_ADD_STP_FORWARD;
            }   

            untag = soc_mem_field32_get(unit, mem, buf_entry, UNTAGf);
            if (untag) {
                flags_arr[vp] |= BCM_VLAN_PORT_UNTAGGED;
            }
        }
    }

cleanup:
    if (buf) {
        soc_cm_sfree(unit, buf);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_ing_vp_vlan_membership_delete_all
 * Purpose:
 *      For the given vlan, delete all the virtual ports from
 *      ING_VP_VLAN_MEMBERSHIP table.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_ing_vp_vlan_membership_delete_all(int unit, bcm_vlan_t vlan)
{
    int rv = BCM_E_NONE;
    soc_mem_t mem;
    int chunk_size, num_chunks, chunk_index;
    int alloc_size;
    uint8 *buf = NULL;
    int i, entry_index_min, entry_index_max;
    uint32 *buf_entry;
    int vp;

    mem = ING_VP_VLAN_MEMBERSHIPm;

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, mem) / chunk_size;
    if (soc_mem_index_count(unit, mem) % chunk_size) {
        num_chunks++;
    }

    alloc_size = sizeof(ing_vp_vlan_membership_entry_t) * chunk_size;
    buf = soc_cm_salloc(unit, alloc_size, "ING_VP_VLAN_MEMBERSHIP buffer");
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

        /* Read each entry of the chunk */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            buf_entry = soc_mem_table_idx_to_pointer(unit,
                    mem, uint32 *, buf, i);

            if (soc_mem_field32_get(unit, mem, buf_entry, VALIDf) == 0) {
                continue;
            }
            if (soc_mem_field32_get(unit, mem, buf_entry, VLANf) != vlan) {
                continue;
            }

            vp = soc_mem_field32_get(unit, mem, buf_entry, VPf);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                continue;
            }
            rv = bcm_td2_ing_vp_vlan_membership_delete(unit, vp, vlan);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    }

cleanup:
    if (buf) {
        soc_cm_sfree(unit, buf);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_egr_vp_vlan_membership_delete_all
 * Purpose:
 *      For the given vlan, delete all the virtual ports from
 *      EGR_VP_VLAN_MEMBERSHIP table.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_egr_vp_vlan_membership_delete_all(int unit, bcm_vlan_t vlan)
{
    int rv = BCM_E_NONE;
    soc_mem_t mem;
    int chunk_size, num_chunks, chunk_index;
    int alloc_size;
    uint8 *buf = NULL;
    int i, entry_index_min, entry_index_max;
    uint32 *buf_entry;
    int vp;

    mem = EGR_VP_VLAN_MEMBERSHIPm;

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, mem) / chunk_size;
    if (soc_mem_index_count(unit, mem) % chunk_size) {
        num_chunks++;
    }

    alloc_size = sizeof(egr_vp_vlan_membership_entry_t) * chunk_size;
    buf = soc_cm_salloc(unit, alloc_size, "EGR_VP_VLAN_MEMBERSHIP buffer");
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

        /* Read each entry of the chunk */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            buf_entry = soc_mem_table_idx_to_pointer(unit,
                    mem, uint32 *, buf, i);

            if (soc_mem_field32_get(unit, mem, buf_entry, VALIDf) == 0) {
                continue;
            }
            if (soc_mem_field32_get(unit, mem, buf_entry, VLANf) != vlan) {
                continue;
            }

            vp = soc_mem_field32_get(unit, mem, buf_entry, VPf);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) &&
                !_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                continue;
            }
            rv = bcm_td2_egr_vp_vlan_membership_delete(unit, vp, vlan);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    }

cleanup:
    if (buf) {
        soc_cm_sfree(unit, buf);
    }

    return rv;
}


#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */
