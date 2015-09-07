/*
 * $Id: ipmc.c 1.4 Broadcom SDK $
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
 * File:        ipmc.c
 * Purpose:     Trident2 PIM-BIDIR implementation.
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/profile_mem.h>

#include <bcm/error.h>
#include <bcm/ipmc.h>
#include <bcm/l3.h>

#include <bcm_int/esw/trident2.h>

typedef struct _bcm_td2_l3_iif_s {
    bcm_if_t l3_iif;
    struct _bcm_td2_l3_iif_s *next;
} _bcm_td2_l3_iif_t;

typedef struct _bcm_td2_rp_s {
    int valid; /* Indicates if this rendezvous point is valid */
    int ref_count; /* Number of IPMC entries referring to
                      this rendezvous point */
    _bcm_td2_l3_iif_t *l3_iif_list; /* Linked list of active L3 IIFs
                                       belonging to this rendezvous point */
} _bcm_td2_rp_t;

typedef struct _bcm_td2_pim_bidir_s {
    int num_rp; /* Number of rendezvous points */
    _bcm_td2_rp_t *rp_info; /* Rendezvous points info */
    soc_profile_mem_t *active_l3_iif_profile; /* Active L3 IIF profile */
    SHR_BITDCL *ipmc_tcam_bitmap; /* Bitmap of valid IPMC TCAM entries */
} _bcm_td2_pim_bidir_t;

STATIC _bcm_td2_pim_bidir_t *_bcm_td2_pim_bidir_info[BCM_MAX_NUM_UNITS];

#define IPMC_NUM_RP(unit) \
        (_bcm_td2_pim_bidir_info[unit]->num_rp)

#define IPMC_RP_INFO(unit, rp_id) \
        (&_bcm_td2_pim_bidir_info[unit]->rp_info[rp_id])

#define IPMC_ACTIVE_L3_IIF_PROFILE(unit) \
        (_bcm_td2_pim_bidir_info[unit]->active_l3_iif_profile)

#define IPMC_TCAM_BITMAP(unit) \
        (_bcm_td2_pim_bidir_info[unit]->ipmc_tcam_bitmap)

#ifdef BCM_WARM_BOOT_SUPPORT
STATIC int _bcm_td2_ipmc_pim_bidir_recover(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      bcm_td2_ipmc_pim_bidir_detach
 * Purpose:
 *      Free data structures related to PIM-BIDIR.
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_pim_bidir_detach(int unit)
{
    int rv = BCM_E_NONE;
    _bcm_td2_pim_bidir_t *pim_bidir_info = NULL;
    int i;
    _bcm_td2_l3_iif_t *current_ptr, *next_ptr;

    if (NULL != _bcm_td2_pim_bidir_info[unit]) {
        pim_bidir_info = _bcm_td2_pim_bidir_info[unit];

        /* Free rendezvous point info array */
        if (NULL != pim_bidir_info->rp_info) {
            for (i = 0; i < pim_bidir_info->num_rp; i++) {
                /* Free linked list of active L3 IIFs */
                current_ptr = pim_bidir_info->rp_info[i].l3_iif_list;
                while (NULL != current_ptr) {
                    next_ptr = current_ptr->next;
                    sal_free(current_ptr);
                    current_ptr = next_ptr;
                }
                pim_bidir_info->rp_info[i].l3_iif_list = NULL;
            }
            sal_free(pim_bidir_info->rp_info);
            pim_bidir_info->rp_info = NULL;
        }

        if (NULL != pim_bidir_info->active_l3_iif_profile) {
            (void) soc_profile_mem_destroy(unit,
                    pim_bidir_info->active_l3_iif_profile);
            sal_free(pim_bidir_info->active_l3_iif_profile);
            pim_bidir_info->active_l3_iif_profile = NULL;
        }

        if (NULL != pim_bidir_info->ipmc_tcam_bitmap) {
            sal_free(pim_bidir_info->ipmc_tcam_bitmap);
            pim_bidir_info->ipmc_tcam_bitmap = NULL;
        }

        sal_free(_bcm_td2_pim_bidir_info[unit]);
        _bcm_td2_pim_bidir_info[unit] = NULL;
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_pim_bidir_init
 * Purpose:
 *      Initialize data structures related to PIM-BIDIR.
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_pim_bidir_init(int unit)
{
    int rv = BCM_E_NONE;
    _bcm_td2_pim_bidir_t *pim_bidir_info = NULL;
    soc_mem_t profile_mem;
    int entry_words;
    void *entry_ptr;
    uint32 profile_index;
    int ipmc_tcam_bitmap_length;
    int i;
    iif_entry_t l3_iif_entry;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        _bcm_td2_pim_bidir_info[unit] = sal_alloc(sizeof(_bcm_td2_pim_bidir_t),
                "PIM BIDIR info");
        if (NULL == _bcm_td2_pim_bidir_info[unit]) {
            bcm_td2_ipmc_pim_bidir_detach(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_bcm_td2_pim_bidir_info[unit], 0, sizeof(_bcm_td2_pim_bidir_t));
    pim_bidir_info = _bcm_td2_pim_bidir_info[unit];

    /* Allocate an array to keep track of rendezvous point info */
    pim_bidir_info->num_rp = soc_mem_field_length(unit,
            ING_ACTIVE_L3_IIF_PROFILEm, RPA_ID_PROFILEf);
    pim_bidir_info->rp_info = sal_alloc(pim_bidir_info->num_rp *
            sizeof(_bcm_td2_rp_t), "RP info array");
    if (NULL == pim_bidir_info->rp_info) {
        bcm_td2_ipmc_pim_bidir_detach(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(pim_bidir_info->rp_info, 0,
            pim_bidir_info->num_rp * sizeof(_bcm_td2_rp_t));

    /* Allocate active L3 IIF profile */
    pim_bidir_info->active_l3_iif_profile =
        sal_alloc(sizeof(soc_profile_mem_t), "Active L3 IIF Profile");
    if (NULL == pim_bidir_info->active_l3_iif_profile) {
        bcm_td2_ipmc_pim_bidir_detach(unit);
        return BCM_E_MEMORY;
    }
    soc_profile_mem_t_init(pim_bidir_info->active_l3_iif_profile);

    /* Initialize the active L3 IIF profile */
    profile_mem = ING_ACTIVE_L3_IIF_PROFILEm;
    entry_words = sizeof(ing_active_l3_iif_profile_entry_t) / sizeof(uint32);
    rv = soc_profile_mem_create(unit, &profile_mem, &entry_words, 1,
                                pim_bidir_info->active_l3_iif_profile);
    if (BCM_FAILURE(rv)) {
        bcm_td2_ipmc_pim_bidir_detach(unit);
        return rv;
    }

    if (!SOC_WARM_BOOT(unit)) {
        /* Initially, all L3_IIF entries point to an active L3 IIF profile
         * that contains no valid rendezvous points.
         */ 
        entry_ptr = soc_mem_entry_null(unit, ING_ACTIVE_L3_IIF_PROFILEm);
        rv = soc_profile_mem_add(unit, pim_bidir_info->active_l3_iif_profile,
                &entry_ptr, 1, &profile_index);
        if (BCM_FAILURE(rv)) {
            bcm_td2_ipmc_pim_bidir_detach(unit);
            return rv;
        }
        for (i = soc_mem_index_min(unit, L3_IIFm);
                i <= soc_mem_index_max(unit, L3_IIFm); i++) {
            rv = READ_L3_IIFm(unit, MEM_BLOCK_ANY, i, &l3_iif_entry);
            if (BCM_FAILURE(rv)) {
                bcm_td2_ipmc_pim_bidir_detach(unit);
                return rv;
            }
            soc_L3_IIFm_field32_set(unit, &l3_iif_entry,
                    ACTIVE_L3_IIF_PROFILE_INDEXf, profile_index);
            rv = WRITE_L3_IIFm(unit, MEM_BLOCK_ALL, i, &l3_iif_entry);
            if (BCM_FAILURE(rv)) {
                bcm_td2_ipmc_pim_bidir_detach(unit);
                return rv;
            }
        }
        SOC_PROFILE_MEM_REFERENCE(unit, pim_bidir_info->active_l3_iif_profile,
                profile_index, soc_mem_index_count(unit, L3_IIFm) - 1);

    } 

    /* Allocate a bitmap indicating which IP_MULTICAST_TCAM entry is valid */
    ipmc_tcam_bitmap_length = soc_mem_index_count(unit, IP_MULTICAST_TCAMm);
    pim_bidir_info->ipmc_tcam_bitmap =
        sal_alloc(SHR_BITALLOCSIZE(ipmc_tcam_bitmap_length), "IPMC TCAM bitmap");
    if (NULL == pim_bidir_info->ipmc_tcam_bitmap) {
        bcm_td2_ipmc_pim_bidir_detach(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(pim_bidir_info->ipmc_tcam_bitmap, 0,
            SHR_BITALLOCSIZE(ipmc_tcam_bitmap_length));

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        rv = _bcm_td2_ipmc_pim_bidir_recover(unit);
        if (BCM_FAILURE(rv)) {
            bcm_td2_ipmc_pim_bidir_detach(unit);
            return rv;
        }
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_create
 * Purpose:
 *      Create a rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      flags - BCM_IPMC_RP_xxx flags.
 *      rp_id - (IN/OUT) Rendezvous point ID.
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_create(
    int unit, 
    uint32 flags, 
    int *rp_id)
{
    int rv = BCM_E_NONE;
    int i;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    if (NULL == rp_id) {
        return BCM_E_PARAM;
    }

    if (flags & BCM_IPMC_RP_WITH_ID) {
        if ((*rp_id < 0) || (*rp_id >= IPMC_NUM_RP(unit))) {
            return BCM_E_PARAM;
        }
        if (IPMC_RP_INFO(unit, *rp_id)->valid) {
            return BCM_E_EXISTS;
        }
        IPMC_RP_INFO(unit, *rp_id)->valid = 1;
    } else {
        for (i = 0; i < IPMC_NUM_RP(unit); i++) {
            if (!IPMC_RP_INFO(unit, i)->valid) {
                IPMC_RP_INFO(unit, i)->valid = 1;
                *rp_id = i;
                break;
            }
        }
        if (i == IPMC_NUM_RP(unit)) {
            /* No available rendezvous point ID */
            return BCM_E_RESOURCE;
        }
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_destroy
 * Purpose:
 *      Destroy a rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID.
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_destroy(
    int unit, 
    int rp_id)
{
    int rv = BCM_E_NONE;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    if ((rp_id < 0) || (rp_id >= IPMC_NUM_RP(unit))) {
        return BCM_E_PARAM;
    }

    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        return BCM_E_NOT_FOUND;
    }

    if (IPMC_RP_INFO(unit, rp_id)->ref_count > 0) {
        /* RP ID is still being referenced by IPMC entries. */
        return BCM_E_BUSY;
    }
    
    /* Delete all active L3 interfaces associated with this RP ID */
    BCM_IF_ERROR_RETURN(bcm_td2_ipmc_rp_delete_all(unit, rp_id));

    IPMC_RP_INFO(unit, rp_id)->valid = 0;

    return rv;
}

/*
 * Function:
 *      _bcm_td2_ipmc_rp_add
 * Purpose:
 *      Add an active L3 interface to a rendezvous point.
 * Parameters:
 *      unit  - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 *      intf_id - Active L3 interface ID
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_td2_ipmc_rp_add(
    int unit, 
    int rp_id, 
    bcm_if_t intf_id)
{
    int rv = BCM_E_NONE;
    iif_entry_t l3_iif_entry;
    uint32 profile_index, new_profile_index;
    ing_active_l3_iif_profile_entry_t profile_entry;
    int rp_bitmap_length;
    SHR_BITDCL *rp_bitmap = NULL;
    void *entry_ptr;

    /* Get L3 IIF's current active L3 interface profile */
    SOC_IF_ERROR_RETURN
        (READ_L3_IIFm(unit, MEM_BLOCK_ANY, intf_id, &l3_iif_entry));
    profile_index = soc_L3_IIFm_field32_get(unit, &l3_iif_entry,
            ACTIVE_L3_IIF_PROFILE_INDEXf);
    SOC_IF_ERROR_RETURN
        (READ_ING_ACTIVE_L3_IIF_PROFILEm(unit, MEM_BLOCK_ANY, profile_index,
                                         &profile_entry));

    /* Get a bitmap of the rendezvous points L3 IIF currently belongs to */
    rp_bitmap_length = soc_mem_field_length(unit, ING_ACTIVE_L3_IIF_PROFILEm,
            RPA_ID_PROFILEf);
    rp_bitmap = sal_alloc(SHR_BITALLOCSIZE(rp_bitmap_length), "RP bitmap");
    if (NULL == rp_bitmap) {
        return BCM_E_MEMORY;
    }
    soc_ING_ACTIVE_L3_IIF_PROFILEm_field_get(unit, &profile_entry,
            RPA_ID_PROFILEf, rp_bitmap);

    /* Check if L3 IIF already belongs to the given rendezvous point */
    if (SHR_BITGET(rp_bitmap, rp_id)) {
        sal_free(rp_bitmap);
        return BCM_E_EXISTS;
    }

    /* Add L3 IIF to the given rendezvous point */
    SHR_BITSET(rp_bitmap, rp_id);

    /* Add a new active L3 interface profile */
    soc_ING_ACTIVE_L3_IIF_PROFILEm_field_set(unit, &profile_entry,
            RPA_ID_PROFILEf, rp_bitmap);
    entry_ptr = &profile_entry;
    rv = soc_profile_mem_add(unit, IPMC_ACTIVE_L3_IIF_PROFILE(unit),
            &entry_ptr, 1, &new_profile_index);
    if (SOC_FAILURE(rv)) {
        sal_free(rp_bitmap);
        return rv;
    }
    soc_L3_IIFm_field32_set(unit, &l3_iif_entry, ACTIVE_L3_IIF_PROFILE_INDEXf,
            new_profile_index);
    rv = WRITE_L3_IIFm(unit, MEM_BLOCK_ALL, intf_id, &l3_iif_entry);
    if (SOC_FAILURE(rv)) {
        sal_free(rp_bitmap);
        return rv;
    }

    /* Delete the old active L3 interface profile */
    rv = soc_profile_mem_delete(unit, IPMC_ACTIVE_L3_IIF_PROFILE(unit),
            profile_index);
    if (SOC_FAILURE(rv)) {
        sal_free(rp_bitmap);
        return rv;
    }

    sal_free(rp_bitmap);
    return rv;
}

/*
 * Function:
 *      _bcm_td2_ipmc_rp_delete
 * Purpose:
 *      Delete an active L3 interface from a rendezvous point.
 * Parameters:
 *      unit  - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 *      intf_id - Active L3 interface ID
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_td2_ipmc_rp_delete(
    int unit, 
    int rp_id, 
    bcm_if_t intf_id)
{
    int rv = BCM_E_NONE;
    iif_entry_t l3_iif_entry;
    uint32 profile_index, new_profile_index;
    ing_active_l3_iif_profile_entry_t profile_entry;
    int rp_bitmap_length;
    SHR_BITDCL *rp_bitmap = NULL;
    void *entry_ptr;

    /* Get L3 IIF's current active L3 interface profile */
    SOC_IF_ERROR_RETURN
        (READ_L3_IIFm(unit, MEM_BLOCK_ANY, intf_id, &l3_iif_entry));
    profile_index = soc_L3_IIFm_field32_get(unit, &l3_iif_entry,
            ACTIVE_L3_IIF_PROFILE_INDEXf);
    SOC_IF_ERROR_RETURN
        (READ_ING_ACTIVE_L3_IIF_PROFILEm(unit, MEM_BLOCK_ANY, profile_index,
                                         &profile_entry));

    /* Get a bitmap of the rendezvous points L3 IIF currently belongs to */
    rp_bitmap_length = soc_mem_field_length(unit, ING_ACTIVE_L3_IIF_PROFILEm,
            RPA_ID_PROFILEf);
    rp_bitmap = sal_alloc(SHR_BITALLOCSIZE(rp_bitmap_length), "RP bitmap");
    if (NULL == rp_bitmap) {
        return BCM_E_MEMORY;
    }
    soc_ING_ACTIVE_L3_IIF_PROFILEm_field_get(unit, &profile_entry,
            RPA_ID_PROFILEf, rp_bitmap);

    /* Make sure L3 IIF belongs to the given rendezvous point */
    if (!SHR_BITGET(rp_bitmap, rp_id)) {
        sal_free(rp_bitmap);
        return BCM_E_NOT_FOUND;
    }

    /* Delete L3 IIF from the given rendezvous point */
    SHR_BITCLR(rp_bitmap, rp_id);

    /* Add a new active L3 interface profile */
    soc_ING_ACTIVE_L3_IIF_PROFILEm_field_set(unit, &profile_entry,
            RPA_ID_PROFILEf, rp_bitmap);
    entry_ptr = &profile_entry;
    rv = soc_profile_mem_add(unit, IPMC_ACTIVE_L3_IIF_PROFILE(unit),
            &entry_ptr, 1, &new_profile_index);
    if (SOC_FAILURE(rv)) {
        sal_free(rp_bitmap);
        return rv;
    }
    soc_L3_IIFm_field32_set(unit, &l3_iif_entry, ACTIVE_L3_IIF_PROFILE_INDEXf,
            new_profile_index);
    rv = WRITE_L3_IIFm(unit, MEM_BLOCK_ALL, intf_id, &l3_iif_entry);
    if (SOC_FAILURE(rv)) {
        sal_free(rp_bitmap);
        return rv;
    }

    /* Delete the old active L3 interface profile */
    rv = soc_profile_mem_delete(unit, IPMC_ACTIVE_L3_IIF_PROFILE(unit),
            profile_index);
    if (SOC_FAILURE(rv)) {
        sal_free(rp_bitmap);
        return rv;
    }

    sal_free(rp_bitmap);
    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_set
 * Purpose:
 *      Set active L3 interfaces for a rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 *      intf_count - Number of elements in intf_array
 *      intf_array - Array of active L3 interfaces
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_set(
    int unit, 
    int rp_id, 
    int intf_count, 
    bcm_if_t *intf_array)
{
    int rv = BCM_E_NONE;
    SHR_BITDCL *intf_bitmap = NULL;
    SHR_BITDCL *still_active_bitmap = NULL;
    int num_l3_iif;
    int i;
    _bcm_td2_l3_iif_t *prev_ptr, *current_ptr;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (rp_id < 0 || rp_id >= IPMC_NUM_RP(unit)) {
        return BCM_E_PARAM;
    }
    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        return BCM_E_CONFIG;
    }
    if (intf_count > 0 && intf_array == NULL) {
        return BCM_E_PARAM;
    }

    /* Convert intf_array to a bitmap */
    num_l3_iif = soc_mem_index_count(unit, L3_IIFm);
    intf_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_l3_iif),
            "L3 interface bitmap");
    if (NULL == intf_bitmap) {
        return BCM_E_MEMORY;
    }
    sal_memset(intf_bitmap, 0, SHR_BITALLOCSIZE(num_l3_iif));
    for (i = 0; i < intf_count; i++) {
        if (intf_array[i] < 0 || intf_array[i] >= num_l3_iif) { 
            rv = BCM_E_PARAM;
            goto cleanup;
        }
        SHR_BITSET(intf_bitmap, intf_array[i]);
    }

    /* Traverse rendezvous point's existing linked list of active L3 IIFs.
     * Delete the L3 IIFs that are no longer active, i.e. not in intf_array.
     * Keep track of L3 IIFs that remain active in still_active_bitmap.
     */
    still_active_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_l3_iif),
            "Still active L3 interface bitmap");
    if (NULL == still_active_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(still_active_bitmap, 0, SHR_BITALLOCSIZE(num_l3_iif));
    prev_ptr = NULL;
    current_ptr = IPMC_RP_INFO(unit, rp_id)->l3_iif_list;
    while (NULL != current_ptr) {
        if (SHR_BITGET(intf_bitmap, current_ptr->l3_iif)) {
            /* This L3 IIF remains active. */
            SHR_BITSET(still_active_bitmap, current_ptr->l3_iif);
            prev_ptr = current_ptr;
            current_ptr = current_ptr->next;
        } else {
            /* Delete inactive L3 IIF from rendezvous point */
            rv = _bcm_td2_ipmc_rp_delete(unit, rp_id, current_ptr->l3_iif);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            if (current_ptr == IPMC_RP_INFO(unit, rp_id)->l3_iif_list) {
                IPMC_RP_INFO(unit, rp_id)->l3_iif_list = current_ptr->next;
                sal_free(current_ptr);
                current_ptr = IPMC_RP_INFO(unit, rp_id)->l3_iif_list;
            } else {
                prev_ptr->next = current_ptr->next;
                sal_free(current_ptr);
                current_ptr = prev_ptr->next;
            }
        }
    }

    /* Traverse intf_array. Add the L3 IIFs that are not already active,
     * i.e. not in still_active_bitmap.
     */
    for (i = 0; i < intf_count; i++) {
        if (SHR_BITGET(still_active_bitmap, intf_array[i])) {
            /* This L3 IIF is already active. */
            continue;
        }
        rv = _bcm_td2_ipmc_rp_add(unit, rp_id, intf_array[i]);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        current_ptr = sal_alloc(sizeof(_bcm_td2_l3_iif_t), "Active L3 IIF");
        if (NULL == current_ptr) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        current_ptr->l3_iif = intf_array[i];
        current_ptr->next = IPMC_RP_INFO(unit, rp_id)->l3_iif_list;
        IPMC_RP_INFO(unit, rp_id)->l3_iif_list = current_ptr;
    }

cleanup:
    if (NULL != intf_bitmap) {
        sal_free(intf_bitmap);
    }
    if (NULL != still_active_bitmap) {
        sal_free(still_active_bitmap);
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_get
 * Purpose:
 *      Get active L3 interfaces for a rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 *      intf_max - Number of elements in intf_array
 *      intf_array - (OUT) Array of active L3 interfaces
 *      intf_count - (OUT) Number of elements returned in intf_array
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_get(
    int unit, 
    int rp_id, 
    int intf_max, 
    bcm_if_t *intf_array, 
    int *intf_count)
{
    int rv = BCM_E_NONE;
    _bcm_td2_l3_iif_t *current_ptr;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (rp_id < 0 || rp_id >= IPMC_NUM_RP(unit)) {
        return BCM_E_PARAM;
    }
    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        return BCM_E_CONFIG;
    }
    if ((intf_max > 0 && intf_array == NULL) ||
        (intf_max == 0 && intf_array != NULL)) {
        return BCM_E_PARAM;
    }
    if (NULL == intf_count) {
        return BCM_E_PARAM;
    }

    /* Traverse rendezvous point's linked list of active L3 IIFs. */
    *intf_count = 0;
    current_ptr = IPMC_RP_INFO(unit, rp_id)->l3_iif_list;
    while (NULL != current_ptr) {
        if (NULL != intf_array) {
            intf_array[*intf_count] = current_ptr->l3_iif;
        }
        (*intf_count)++;
        if (*intf_count == intf_max) {
            break;
        }
        current_ptr = current_ptr->next;
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_add
 * Purpose:
 *      Add an active L3 interface to a rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 *      intf_id - L3 interface ID
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_add(
    int unit, 
    int rp_id, 
    bcm_if_t intf_id)
{
    int rv = BCM_E_NONE;
    _bcm_td2_l3_iif_t *current_ptr;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (rp_id < 0 || rp_id >= IPMC_NUM_RP(unit)) {
        return BCM_E_PARAM;
    }
    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        return BCM_E_CONFIG;
    }
    if (intf_id < 0 || intf_id > soc_mem_index_max(unit, L3_IIFm)) {
        return BCM_E_PARAM;
    }

    /* Add intf_id to rendezvous point */
    BCM_IF_ERROR_RETURN(_bcm_td2_ipmc_rp_add(unit, rp_id, intf_id));

    /* Insert intf_id to head of rendezvous point's linked list of L3 IIFs */
    current_ptr = sal_alloc(sizeof(_bcm_td2_l3_iif_t), "Active L3 IIF");
    if (NULL == current_ptr) {
        return BCM_E_MEMORY;
    }
    current_ptr->l3_iif = intf_id;
    current_ptr->next = IPMC_RP_INFO(unit, rp_id)->l3_iif_list;
    IPMC_RP_INFO(unit, rp_id)->l3_iif_list = current_ptr;

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_delete
 * Purpose:
 *      Delete an active L3 interface from a rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 *      intf_id - L3 interface ID
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_delete(
    int unit, 
    int rp_id, 
    bcm_if_t intf_id)
{
    int rv = BCM_E_NONE;
    _bcm_td2_l3_iif_t *current_ptr, *prev_ptr;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (rp_id < 0 || rp_id >= IPMC_NUM_RP(unit)) {
        return BCM_E_PARAM;
    }
    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        return BCM_E_CONFIG;
    }
    if (intf_id < 0 || intf_id > soc_mem_index_max(unit, L3_IIFm)) {
        return BCM_E_PARAM;
    }

    /* Traverse rendezvous point's linked list of active L3 IIFs to
     * find and delete the given intf_id.
     */
    prev_ptr = NULL;
    current_ptr = IPMC_RP_INFO(unit, rp_id)->l3_iif_list;
    while (NULL != current_ptr) {
        if (current_ptr->l3_iif == intf_id) {
            BCM_IF_ERROR_RETURN(_bcm_td2_ipmc_rp_delete(unit, rp_id, intf_id));
            if (current_ptr == IPMC_RP_INFO(unit, rp_id)->l3_iif_list) {
                IPMC_RP_INFO(unit, rp_id)->l3_iif_list = current_ptr->next;
            } else {
                /* 
                 * In the following line of code, Coverity thinks the
                 * prev_ptr pointer may still be NULL when 
                 * dereferenced. This situation will never occur because 
                 * if current_ptr is not pointing to the head of the 
                 * linked list, prev_ptr would not be NULL.
                 */
                /* coverity[var_deref_op : FALSE] */
                prev_ptr->next = current_ptr->next;
            }
            sal_free(current_ptr);
            break;
        } else {
            prev_ptr = current_ptr;
            current_ptr = current_ptr->next;
        }
    }
    if (NULL == current_ptr) {
        return BCM_E_NOT_FOUND;
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_delete_all
 * Purpose:
 *      Delete all active L3 interfaces from a rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_delete_all(
    int unit, 
    int rp_id) 
{
    return bcm_td2_ipmc_rp_set(unit, rp_id, 0, NULL);
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_ref_count_incr
 * Purpose:
 *      Increment reference count of rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_ref_count_incr(
    int unit, 
    int rp_id) 
{
    int rv = BCM_E_NONE;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    if ((rp_id < 0) || (rp_id >= IPMC_NUM_RP(unit))) {
        return BCM_E_PARAM;
    }

    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        return BCM_E_NOT_FOUND;
    }

    IPMC_RP_INFO(unit, rp_id)->ref_count++;

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_ref_count_decr
 * Purpose:
 *      Decrement reference count of rendezvous point.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_rp_ref_count_decr(
    int unit, 
    int rp_id) 
{
    int rv = BCM_E_NONE;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    if ((rp_id < 0) || (rp_id >= IPMC_NUM_RP(unit))) {
        return BCM_E_PARAM;
    }

    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        return BCM_E_NOT_FOUND;
    }

    if (IPMC_RP_INFO(unit, rp_id)->ref_count == 0) {
        return BCM_E_INTERNAL;
    }

    IPMC_RP_INFO(unit, rp_id)->ref_count--;

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_range_add
 * Purpose:
 *      Add a range of PIM-BIDIR IPMC addresses.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      range_id - (IN/OUT) Range ID
 *      range - IPMC address range info
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_range_add(
    int unit, 
    int *range_id, 
    bcm_ipmc_range_t *range)
{
    int rv = BCM_E_NONE;
    int tcam_index = 0;
    int i;
    ip_multicast_tcam_entry_t tcam_entry;
    uint32 group_addr[4], group_addr_mask[4];

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (NULL == range_id || NULL == range) {
        return BCM_E_PARAM;
    }
    if (range->flags & BCM_IPMC_RANGE_WITH_ID) {
        if (*range_id < 0 ||
            *range_id > soc_mem_index_max(unit, IP_MULTICAST_TCAMm)) {
            return BCM_E_PARAM;
        }
    }
    if (!(range->flags & BCM_IPMC_RANGE_PIM_BIDIR)) {
        /* On Trident2, only PIM-BIDIR eligible addresses can be specified.
         * Addresses not eligible for PIM-BIDIR cannot be specified.
         */
        return BCM_E_PARAM;
    }
    if (range->priority != 0) {
        /* On Trident2, only PIM-BIDIR eligible addresses can be specified.
         * Hence, there is no need to specify relative priority among the
         * ranges of PIM-BIDIR eligible addresses.
         */
        return BCM_E_PARAM;
    }
    if ((range->vrf > SOC_VRF_MAX(unit)) ||
        (range->vrf < BCM_L3_VRF_DEFAULT)) {
        return BCM_E_PARAM;
    }

    /* Get index to IP_MULTICAST_TCAM */
    if (range->flags & BCM_IPMC_RANGE_REPLACE) {
        if (!(range->flags & BCM_IPMC_RANGE_WITH_ID)) {
            return BCM_E_PARAM;
        }
        tcam_index = *range_id;
        if (!SHR_BITGET(IPMC_TCAM_BITMAP(unit), tcam_index)) {
            return BCM_E_NOT_FOUND;
        }
    } else {
        if (range->flags & BCM_IPMC_RANGE_WITH_ID) {
            tcam_index = *range_id;
            if (SHR_BITGET(IPMC_TCAM_BITMAP(unit), tcam_index)) {
                return BCM_E_EXISTS;
            }
            SHR_BITSET(IPMC_TCAM_BITMAP(unit), tcam_index);
        } else {
            /* Find a free TCAM index */
            for (i = 0; i < soc_mem_index_count(unit, IP_MULTICAST_TCAMm); i++) {
                if (!SHR_BITGET(IPMC_TCAM_BITMAP(unit), i)) {
                    SHR_BITSET(IPMC_TCAM_BITMAP(unit), i);
                    tcam_index = i;
                    *range_id = tcam_index;
                    break;
                }
            }
            if (i == soc_mem_index_count(unit, IP_MULTICAST_TCAMm)) {
                return BCM_E_RESOURCE;
            }
        }
    }

    /* Write entry to IP_MULTICAST_TCAM */
    sal_memset(&tcam_entry, 0, sizeof(tcam_entry));
    soc_IP_MULTICAST_TCAMm_field32_set(unit, &tcam_entry, VALIDf, 1);
    soc_IP_MULTICAST_TCAMm_field32_set(unit, &tcam_entry, VRFf,
            range->vrf);
    soc_IP_MULTICAST_TCAMm_field32_set(unit, &tcam_entry, VRF_MASKf,
            range->vrf_mask);
    if (range->flags & BCM_IPMC_RANGE_IP6) {
        soc_IP_MULTICAST_TCAMm_field32_set(unit, &tcam_entry,
                MODEf, 1);
        soc_IP_MULTICAST_TCAMm_field32_set(unit, &tcam_entry,
                MODE_MASKf, 1);
        soc_mem_ip6_addr_set(unit, IP_MULTICAST_TCAMm, &tcam_entry,
                GROUP_ADDRf, range->mc_ip6_addr, 0);
        soc_mem_ip6_addr_set(unit, IP_MULTICAST_TCAMm, &tcam_entry,
                GROUP_ADDR_MASKf, range->mc_ip6_addr_mask, 0);
    } else {
        soc_IP_MULTICAST_TCAMm_field32_set(unit, &tcam_entry,
                MODEf, 0);
        soc_IP_MULTICAST_TCAMm_field32_set(unit, &tcam_entry,
                MODE_MASKf, 1);
        sal_memset(group_addr, 0, sizeof(group_addr));
        group_addr[0] = range->mc_ip_addr;
        soc_IP_MULTICAST_TCAMm_field_set(unit, &tcam_entry,
                GROUP_ADDRf, group_addr);
        sal_memset(group_addr_mask, 0, sizeof(group_addr_mask));
        group_addr_mask[0] = range->mc_ip_addr_mask;
        soc_IP_MULTICAST_TCAMm_field_set(unit, &tcam_entry,
                GROUP_ADDR_MASKf, group_addr_mask);
    }
    SOC_IF_ERROR_RETURN(WRITE_IP_MULTICAST_TCAMm(unit, MEM_BLOCK_ALL,
                tcam_index, &tcam_entry));

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_range_delete
 * Purpose:
 *      Delete a range of PIM-BIDIR IPMC addresses.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      range_id - Range ID
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_range_delete(
    int unit, 
    int range_id)
{
    int rv = BCM_E_NONE;
    int tcam_index;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (range_id < 0 ||
            range_id > soc_mem_index_max(unit, IP_MULTICAST_TCAMm)) {
        return BCM_E_PARAM;
    }

    /* Delete the TCAM entry referenced by range_id */
    tcam_index = range_id;
    if (!SHR_BITGET(IPMC_TCAM_BITMAP(unit), tcam_index)) {
        return BCM_E_NOT_FOUND;
    }
    SOC_IF_ERROR_RETURN(WRITE_IP_MULTICAST_TCAMm(unit, MEM_BLOCK_ALL,
                tcam_index, soc_mem_entry_null(unit, IP_MULTICAST_TCAMm)));
    SHR_BITCLR(IPMC_TCAM_BITMAP(unit), tcam_index);

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_range_delete_all
 * Purpose:
 *      Delete all ranges of PIM-BIDIR IPMC addresses.
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_range_delete_all(
    int unit)
{
    int rv = BCM_E_NONE;
    int ipmc_tcam_bitmap_length;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Clear IP Multicast TCAM table */
    SOC_IF_ERROR_RETURN
        (soc_mem_clear(unit, IP_MULTICAST_TCAMm, MEM_BLOCK_ALL, 0));

    /* Clear bitmap indicating which IP_MULTICAST_TCAM entry is valid */
    ipmc_tcam_bitmap_length = soc_mem_index_count(unit, IP_MULTICAST_TCAMm);
    sal_memset(IPMC_TCAM_BITMAP(unit), 0,
            SHR_BITALLOCSIZE(ipmc_tcam_bitmap_length));

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_range_get
 * Purpose:
 *      Get a range of PIM-BIDIR IPMC addresses.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      range_id - Range ID
 *      range - (OUT) Range info
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_range_get(
    int unit, 
    int range_id, 
    bcm_ipmc_range_t *range)
{
    int rv = BCM_E_NONE;
    int tcam_index;
    ip_multicast_tcam_entry_t tcam_entry;
    uint32 group_addr[4], group_addr_mask[4];

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (range_id < 0 ||
        range_id > soc_mem_index_max(unit, IP_MULTICAST_TCAMm)) {
        return BCM_E_PARAM;
    }
    if (NULL == range) {
        return BCM_E_PARAM;
    }

    /* Read the TCAM entry referenced by range_id */
    tcam_index = range_id;
    if (!SHR_BITGET(IPMC_TCAM_BITMAP(unit), tcam_index)) {
        return BCM_E_NOT_FOUND;
    }
    SOC_IF_ERROR_RETURN
        (READ_IP_MULTICAST_TCAMm(unit, MEM_BLOCK_ANY, tcam_index, &tcam_entry));
    if (!soc_IP_MULTICAST_TCAMm_field32_get(unit, &tcam_entry, VALIDf)) {
        return BCM_E_INTERNAL;
    }

    /* Fill in the bcm_ipmc_range_t structure */
    bcm_ipmc_range_t_init(range);
    range->flags |= BCM_IPMC_RANGE_PIM_BIDIR;
    if (soc_IP_MULTICAST_TCAMm_field32_get(unit, &tcam_entry, MODEf)) {
        /* IPv6 entry */
        range->flags |= BCM_IPMC_RANGE_IP6;
        soc_mem_ip6_addr_get(unit, IP_MULTICAST_TCAMm, &tcam_entry,
                GROUP_ADDRf, range->mc_ip6_addr, 0);
        soc_mem_ip6_addr_get(unit, IP_MULTICAST_TCAMm, &tcam_entry,
                GROUP_ADDR_MASKf, range->mc_ip6_addr_mask, 0);
    } else {
        /* IPv4 entry */
        soc_IP_MULTICAST_TCAMm_field_get(unit, &tcam_entry, GROUP_ADDRf,
                group_addr);
        soc_IP_MULTICAST_TCAMm_field_get(unit, &tcam_entry, GROUP_ADDR_MASKf,
                group_addr_mask);
        range->mc_ip_addr = group_addr[0];
        range->mc_ip_addr_mask = group_addr_mask[0];
    }
    range->vrf = soc_IP_MULTICAST_TCAMm_field32_get(unit, &tcam_entry,
            VRFf);
    range->vrf_mask = soc_IP_MULTICAST_TCAMm_field32_get(unit, &tcam_entry,
            VRF_MASKf);

    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_range_size_get
 * Purpose:
 *      Get the number of PIM-BIDIR IPMC address ranges supported by the device.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      size - (OUT) Number of ranges 
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_range_size_get(
    int unit, 
    int *size)
{
    int rv = BCM_E_NONE;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    /* Input check */
    if (NULL == size) {
        return BCM_E_PARAM;
    }

    *size = soc_mem_index_count(unit, IP_MULTICAST_TCAMm);

    return rv;
}

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Function:
 *      bcm_td2_ipmc_pim_bidir_scache_size_get
 * Purpose:
 *      Get the required scache size for PIM-BIDIR feature.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      size - (OUT) Number of bytes
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_pim_bidir_scache_size_get(
    int unit, 
    uint32 *size)
{
    int num_rp;

    *size = 0;
    num_rp = soc_mem_field_length(unit, ING_ACTIVE_L3_IIF_PROFILEm,
            RPA_ID_PROFILEf);
    *size += SHR_BITALLOCSIZE(num_rp);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_ipmc_pim_bidir_sync
 * Purpose:
 *      Record PIM-BIDIR persistent info into the scache.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_td2_ipmc_pim_bidir_sync(
    int unit, 
    uint8 **scache_ptr)
{
    int num_rp, i;
    SHR_BITDCL *rp_bitmap = NULL;

    /* Create a bitmap of valid rendezvous points */
    num_rp = soc_mem_field_length(unit, ING_ACTIVE_L3_IIF_PROFILEm,
            RPA_ID_PROFILEf);
    rp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_rp), "RP bitmap");
    if (NULL == rp_bitmap) {
        return BCM_E_MEMORY;
    }
    sal_memset(rp_bitmap, 0, SHR_BITALLOCSIZE(num_rp));
    for (i = 0; i < num_rp; i++) {
        if (IPMC_RP_INFO(unit, i)->valid) {
            SHR_BITSET(rp_bitmap, i);
        }
    }

    /* Store the bitmap of valid rendezvous points into scache */
    sal_memcpy((*scache_ptr), rp_bitmap, SHR_BITALLOCSIZE(num_rp));
    (*scache_ptr) += SHR_BITALLOCSIZE(num_rp);

    sal_free(rp_bitmap);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_ipmc_pim_bidir_scache_recover
 * Purpose:
 *      Recover PIM-BIDIR info from scache.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_ipmc_pim_bidir_scache_recover(int unit, uint8 **scache_ptr) 
{
    int num_rp, i;
    SHR_BITDCL *rp_bitmap = NULL;

    /* Recover a bitmap of valid rendezvous points */
    num_rp = soc_mem_field_length(unit, ING_ACTIVE_L3_IIF_PROFILEm,
            RPA_ID_PROFILEf);
    rp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_rp), "RP bitmap");
    if (NULL == rp_bitmap) {
        return BCM_E_MEMORY;
    }
    sal_memcpy(rp_bitmap, (*scache_ptr), SHR_BITALLOCSIZE(num_rp));
    for (i = 0; i < num_rp; i++) {
        if (SHR_BITGET(rp_bitmap, i)) {
            IPMC_RP_INFO(unit, i)->valid = 1;
        }
    }
    (*scache_ptr) += SHR_BITALLOCSIZE(num_rp);

    sal_free(rp_bitmap);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ipmc_pim_bidir_recover
 * Purpose:
 *      Recover PIM-BIDIR info from hardware.
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_td2_ipmc_pim_bidir_recover(int unit)
{
    int rv = BCM_E_NONE;
    int buf_size;
    uint8 *l3_iif_buf = NULL;
    int i;
    iif_entry_t *l3_iif_entry_ptr = NULL;
    int profile_index;
    ing_active_l3_iif_profile_entry_t profile_entry;
    int rp_bitmap_length;
    SHR_BITDCL *rp_bitmap = NULL;
    int rp_id;
    _bcm_td2_l3_iif_t *current_ptr;
    uint8 *tcam_buf = NULL;
    ip_multicast_tcam_entry_t *tcam_entry_ptr = NULL;

    /* Read L3 IIF table */
    buf_size = sizeof(iif_entry_t) * soc_mem_index_count(unit, L3_IIFm);
    l3_iif_buf = soc_cm_salloc(unit, buf_size, "L3 IIF buffer");
    if (NULL == l3_iif_buf) {
        rv = BCM_E_MEMORY;
        goto clean_up;
    }
    rv = soc_mem_read_range(unit, L3_IIFm, MEM_BLOCK_ANY,
            soc_mem_index_min(unit, L3_IIFm),
            soc_mem_index_max(unit, L3_IIFm), l3_iif_buf);
    if (SOC_FAILURE(rv)) {
        goto clean_up;
    }

    /* Traverse each entry of L3 IIF table */
    for (i = soc_mem_index_min(unit, L3_IIFm);
            i <= soc_mem_index_max(unit, L3_IIFm); i++) {
        l3_iif_entry_ptr = soc_mem_table_idx_to_pointer(unit, L3_IIFm,
                iif_entry_t *, l3_iif_buf, i);

        /* Recover active L3 IIF profile reference count */
        profile_index = soc_L3_IIFm_field32_get(unit, l3_iif_entry_ptr,
                ACTIVE_L3_IIF_PROFILE_INDEXf); 
        SOC_PROFILE_MEM_REFERENCE(unit, IPMC_ACTIVE_L3_IIF_PROFILE(unit),
                profile_index, 1);
        SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, IPMC_ACTIVE_L3_IIF_PROFILE(unit),
                profile_index, 1);

        /* Get a bitmap of the rendezvous points L3 IIF belongs to */
        rv = READ_ING_ACTIVE_L3_IIF_PROFILEm(unit, MEM_BLOCK_ANY,
                profile_index, &profile_entry);
        if (SOC_FAILURE(rv)) {
            goto clean_up;
        }
        rp_bitmap_length = soc_mem_field_length(unit,
                ING_ACTIVE_L3_IIF_PROFILEm, RPA_ID_PROFILEf);
        if (NULL == rp_bitmap) {
            rp_bitmap = sal_alloc(SHR_BITALLOCSIZE(rp_bitmap_length),
                    "RP bitmap");
            if (NULL == rp_bitmap) {
                rv = BCM_E_MEMORY;
                goto clean_up;
            }
        }
        soc_ING_ACTIVE_L3_IIF_PROFILEm_field_get(unit, &profile_entry,
                RPA_ID_PROFILEf, rp_bitmap);

        /* For each rendezvous point the L3 IIF belongs to,
         * insert the L3 IIF into the rendezvous point's linked list
         * of L3 IIFs.
         */
        for (rp_id = 0; rp_id < rp_bitmap_length; rp_id++) {
            if (SHR_BITGET(rp_bitmap, rp_id)) {
                IPMC_RP_INFO(unit, rp_id)->valid = 1;
                current_ptr = sal_alloc(sizeof(_bcm_td2_l3_iif_t),
                        "Active L3 IIF");
                if (NULL == current_ptr) {
                    rv = BCM_E_MEMORY;
                    goto clean_up;
                }
                current_ptr->l3_iif = i;
                current_ptr->next = IPMC_RP_INFO(unit, rp_id)->l3_iif_list;
                IPMC_RP_INFO(unit, rp_id)->l3_iif_list = current_ptr;
            }
        }
    }

    /* Recover bitmap of valid IPMC TCAM entries */
    buf_size = sizeof(ip_multicast_tcam_entry_t) *
        soc_mem_index_count(unit, IP_MULTICAST_TCAMm);
    tcam_buf = soc_cm_salloc(unit, buf_size, "IP Multicast TCAM buffer");
    if (NULL == tcam_buf) {
        rv = BCM_E_MEMORY;
        goto clean_up;
    }
    rv = soc_mem_read_range(unit, IP_MULTICAST_TCAMm, MEM_BLOCK_ANY,
            soc_mem_index_min(unit, IP_MULTICAST_TCAMm),
            soc_mem_index_max(unit, IP_MULTICAST_TCAMm), tcam_buf);
    if (SOC_FAILURE(rv)) {
        goto clean_up;
    }
    for (i = soc_mem_index_min(unit, IP_MULTICAST_TCAMm);
            i <= soc_mem_index_max(unit, IP_MULTICAST_TCAMm); i++) {
        tcam_entry_ptr = soc_mem_table_idx_to_pointer(unit, IP_MULTICAST_TCAMm,
                ip_multicast_tcam_entry_t *, tcam_buf, i);
        if (soc_IP_MULTICAST_TCAMm_field32_get(unit, tcam_entry_ptr, VALIDf)) {
            SHR_BITSET(IPMC_TCAM_BITMAP(unit), i);
        }
    }

clean_up:
    if (NULL != l3_iif_buf) {
        soc_cm_sfree(unit, l3_iif_buf);
    }
    if (NULL != rp_bitmap) {
        sal_free(rp_bitmap);
    }
    if (NULL != tcam_buf) {
        soc_cm_sfree(unit, tcam_buf);
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_ipmc_rp_ref_count_recover
 * Purpose:
 *      Increment reference count of rendezvous point. If the rendezvous point
 *      is invalid, set it to valid.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      rp_id - Rendezvous point ID
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      During warm boot recovery, the IPMC address table is traversed to
 *      recover how many IPMC address entries are referencing the rendezvous
 *      points. This function is called to update the reference count.
 *      The traversal of IPCM address table occurs before recovery of
 *      rendezvous point info from scache. Hence, when this function is
 *      called, it's possible that the rendezvous point is not yet marked
 *      valid.
 */
int 
bcm_td2_ipmc_rp_ref_count_recover(
    int unit, 
    int rp_id) 
{
    int rv = BCM_E_NONE;

    if (NULL == _bcm_td2_pim_bidir_info[unit]) {
        return BCM_E_INIT;
    }

    if ((rp_id < 0) || (rp_id >= IPMC_NUM_RP(unit))) {
        return BCM_E_PARAM;
    }

    if (!IPMC_RP_INFO(unit, rp_id)->valid) {
        IPMC_RP_INFO(unit, rp_id)->valid = 1;
    }

    IPMC_RP_INFO(unit, rp_id)->ref_count++;

    return rv;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_td2_ipmc_pim_bidir_sw_dump
 * Purpose:
 *     Displays PIM-BIDIR information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_td2_ipmc_pim_bidir_sw_dump(int unit)
{
    int rv;
    int i, j;
    int num_profiles, ref_count;

    soc_cm_print("    PIM-BIDIR Rendezvous Points Size: %d\n",
            IPMC_NUM_RP(unit));

    soc_cm_print("    Allocated Rendezvous Points (ID:reference count):");
    for (i = 0, j = 0; i < IPMC_NUM_RP(unit); i++) {
        if (!IPMC_RP_INFO(unit, i)->valid) {
            continue;
        }
        if (!(j % 10)) {
            soc_cm_print("\n    ");
        }
        soc_cm_print("  %2d:%-6d", i, IPMC_RP_INFO(unit, i)->ref_count);
        j++;
    }
    soc_cm_print("\n");

    soc_cm_print("    Active L3 IIF profile (index:reference count):");
    num_profiles = soc_mem_index_count(unit, ING_ACTIVE_L3_IIF_PROFILEm);
    for (i = 0, j = 0; i < num_profiles; i++) {
        rv = soc_profile_mem_ref_count_get(unit,
                IPMC_ACTIVE_L3_IIF_PROFILE(unit), i, &ref_count);
        if (SOC_FAILURE(rv)) {
            continue;
        }
        if (0 == ref_count) {
            continue;
        }
        if (!(j % 10)) {
            soc_cm_print("\n    ");
        }
        soc_cm_print("  %4d:%-4d", i, ref_count);
        j++;
    }
    soc_cm_print("\n");

    return;
}

#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */
