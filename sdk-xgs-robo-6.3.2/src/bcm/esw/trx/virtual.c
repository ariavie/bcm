/*
 * $Id: virtual.c 1.70 Broadcom SDK $
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
 * File:    virtual.c
 * Purpose: Manages VP / VFI resources
 */
#if defined(INCLUDE_L3)
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#include <soc/mcm/allenum.h>

#include <sal/compiler.h>

#include <bcm/error.h>
#include <bcm/types.h>
#include <bcm/module.h>
#include <bcm_int/esw/virtual.h>
#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm_int/esw/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
#include <bcm_int/esw/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
#include <bcm_int/esw/enduro.h>
#endif /* BCM_ENDURO_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

/*
 * Global virtual book keeping info
 */
_bcm_virtual_bookkeeping_t  _bcm_virtual_bk_info[BCM_MAX_NUM_UNITS] = {{ 0 }};
STATIC sal_mutex_t _virtual_mutex[BCM_MAX_NUM_UNITS] = {NULL};

#define VIRTUAL_INFO(_unit_)   (&_bcm_virtual_bk_info[_unit_])

/*
 * VFI table usage bitmap operations
 */
#define _BCM_VIRTUAL_VFI_USED_GET(_u_, _vfi_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->vfi_bitmap, (_vfi_))
#define _BCM_VIRTUAL_VFI_USED_SET(_u_, _vfi_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->vfi_bitmap, (_vfi_))
#define _BCM_VIRTUAL_VFI_USED_CLR(_u_, _vfi_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->vfi_bitmap, (_vfi_))

/*
 * Virtual Port usage bitmap operations
 */
#define _BCM_VIRTUAL_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->vp_bitmap, (_vp_))
#define _BCM_VIRTUAL_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->vp_bitmap, (_vp_))
#define _BCM_VIRTUAL_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->vp_bitmap, (_vp_))

/*
 * MPLS VFI table usage bitmap operations
 */
#define _BCM_MPLS_VFI_USED_GET(_u_, _vfi_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->mpls_vfi_bitmap, (_vfi_))
#define _BCM_MPLS_VFI_USED_SET(_u_, _vfi_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->mpls_vfi_bitmap, (_vfi_))
#define _BCM_MPLS_VFI_USED_CLR(_u_, _vfi_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->mpls_vfi_bitmap, (_vfi_))

/*
 * MPLS Virtual Port usage bitmap operations
 */
#define _BCM_MPLS_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->mpls_vp_bitmap, (_vp_))
#define _BCM_MPLS_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->mpls_vp_bitmap, (_vp_))
#define _BCM_MPLS_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->mpls_vp_bitmap, (_vp_))

/*
 * MIM VFI table usage bitmap operations
 */
#define _BCM_MIM_VFI_USED_GET(_u_, _vfi_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->mim_vfi_bitmap, (_vfi_))
#define _BCM_MIM_VFI_USED_SET(_u_, _vfi_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->mim_vfi_bitmap, (_vfi_))
#define _BCM_MIM_VFI_USED_CLR(_u_, _vfi_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->mim_vfi_bitmap, (_vfi_))

/*
 * MIM Virtual Port usage bitmap operations
 */
#define _BCM_MIM_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->mim_vp_bitmap, (_vp_))
#define _BCM_MIM_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->mim_vp_bitmap, (_vp_))
#define _BCM_MIM_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->mim_vp_bitmap, (_vp_))

/*
 * L2GRE VFI usage bitmap operations
 */
#define _BCM_L2GRE_VFI_USED_GET(_u_, _vfi_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->l2gre_vfi_bitmap, (_vfi_))
#define _BCM_L2GRE_VFI_USED_SET(_u_, _vfi_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->l2gre_vfi_bitmap, (_vfi_))
#define _BCM_L2GRE_VFI_USED_CLR(_u_, _vfi_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->l2gre_vfi_bitmap, (_vfi_))

/*
 * L2GRE Virtual Port usage bitmap operations
 */
#define _BCM_L2GRE_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->l2gre_vp_bitmap, (_vp_))
#define _BCM_L2GRE_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->l2gre_vp_bitmap, (_vp_))
#define _BCM_L2GRE_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->l2gre_vp_bitmap, (_vp_))


/*
 * VXLAN VFI usage bitmap operations
 */
#define _BCM_VXLAN_VFI_USED_GET(_u_, _vfi_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->vxlan_vfi_bitmap, (_vfi_))
#define _BCM_VXLAN_VFI_USED_SET(_u_, _vfi_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->vxlan_vfi_bitmap, (_vfi_))
#define _BCM_VXLAN_VFI_USED_CLR(_u_, _vfi_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->vxlan_vfi_bitmap, (_vfi_))

/*
 * VXLAN Virtual Port usage bitmap operations
 */
#define _BCM_VXLAN_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->vxlan_vp_bitmap, (_vp_))
#define _BCM_VXLAN_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->vxlan_vp_bitmap, (_vp_))
#define _BCM_VXLAN_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->vxlan_vp_bitmap, (_vp_))

/* 
 * Virtual resource lock
 */
#define VIRTUAL_LOCK(unit) \
        sal_mutex_take(_virtual_mutex[unit], sal_mutex_FOREVER); 

#define VIRTUAL_UNLOCK(unit) \
        sal_mutex_give(_virtual_mutex[unit]); 

/*
 * Subport Virtual Port usage bitmap operations
 */
#define _BCM_SUBPORT_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->subport_vp_bitmap, (_vp_))
#define _BCM_SUBPORT_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->subport_vp_bitmap, (_vp_))
#define _BCM_SUBPORT_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->subport_vp_bitmap, (_vp_))

/*
 * WLAN Virtual Port usage bitmap operations
 */
#define _BCM_WLAN_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->wlan_vp_bitmap, (_vp_))
#define _BCM_WLAN_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->wlan_vp_bitmap, (_vp_))
#define _BCM_WLAN_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->wlan_vp_bitmap, (_vp_))

   /*
     * TRILL Virtual Port usage bitmap operations
     */
#define _BCM_TRILL_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->trill_vp_bitmap, (_vp_))
#define _BCM_TRILL_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->trill_vp_bitmap, (_vp_))
#define _BCM_TRILL_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->trill_vp_bitmap, (_vp_))
/*
 * VLAN Virtual Port usage bitmap operations
 */
#define _BCM_VLAN_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->vlan_vp_bitmap, (_vp_))
#define _BCM_VLAN_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->vlan_vp_bitmap, (_vp_))
#define _BCM_VLAN_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->vlan_vp_bitmap, (_vp_))

/*
 * NIV Virtual Port usage bitmap operations
 */
#define _BCM_NIV_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->niv_vp_bitmap, (_vp_))
#define _BCM_NIV_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->niv_vp_bitmap, (_vp_))
#define _BCM_NIV_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->niv_vp_bitmap, (_vp_))

/*
 * Extender Virtual Port usage bitmap operations
 */
#define _BCM_EXTENDER_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->extender_vp_bitmap, (_vp_))
#define _BCM_EXTENDER_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->extender_vp_bitmap, (_vp_))
#define _BCM_EXTENDER_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->extender_vp_bitmap, (_vp_))

/*
 * VP LAG Virtual Port usage bitmap operations
 */
#define _BCM_VP_LAG_VP_USED_GET(_u_, _vp_) \
        SHR_BITGET(VIRTUAL_INFO(_u_)->vp_lag_vp_bitmap, (_vp_))
#define _BCM_VP_LAG_VP_USED_SET(_u_, _vp_) \
        SHR_BITSET(VIRTUAL_INFO((_u_))->vp_lag_vp_bitmap, (_vp_))
#define _BCM_VP_LAG_VP_USED_CLR(_u_, _vp_) \
        SHR_BITCLR(VIRTUAL_INFO((_u_))->vp_lag_vp_bitmap, (_vp_))

/*
 * Function:
 *      _bcm_virtual_free_resource
 * Purpose:
 *      Free all allocated resources
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */
STATIC void
_bcm_virtual_free_resource(int unit, _bcm_virtual_bookkeeping_t *virtual_info)
{
    if (!virtual_info) {
        return;
    }

    if (_virtual_mutex[unit]) {
        sal_mutex_destroy(_virtual_mutex[unit]);
        _virtual_mutex[unit] = NULL;
    } 

    if (virtual_info->vfi_bitmap) {
        sal_free(virtual_info->vfi_bitmap);
        virtual_info->vfi_bitmap = NULL;
    }

    if (virtual_info->vp_bitmap) {
        /* Before we free the VP bitmap, unregister it from the
         * SOC layer L2X thread. */
        soc_l2x_cml_vp_bitmap_set(unit, NULL);
        sal_free(virtual_info->vp_bitmap);
        virtual_info->vp_bitmap = NULL;
    }

    if (virtual_info->mpls_vfi_bitmap) {
        sal_free(virtual_info->mpls_vfi_bitmap);
        virtual_info->mpls_vfi_bitmap = NULL;
    }

    if (virtual_info->mpls_vp_bitmap) {
        sal_free(virtual_info->mpls_vp_bitmap);
        virtual_info->mpls_vp_bitmap = NULL;
    }

    if (virtual_info->mim_vfi_bitmap) {
        sal_free(virtual_info->mim_vfi_bitmap);
        virtual_info->mim_vfi_bitmap = NULL;
    }

    if (virtual_info->mim_vp_bitmap) {
        sal_free(virtual_info->mim_vp_bitmap);
        virtual_info->mim_vp_bitmap = NULL;
    }

    if (virtual_info->subport_vp_bitmap) {
        sal_free(virtual_info->subport_vp_bitmap);
        virtual_info->subport_vp_bitmap = NULL;
    }

    if (virtual_info->wlan_vp_bitmap) {
        sal_free(virtual_info->wlan_vp_bitmap);
        virtual_info->wlan_vp_bitmap = NULL;
    }

    if (virtual_info->trill_vp_bitmap) {
        sal_free(virtual_info->trill_vp_bitmap);
        virtual_info->trill_vp_bitmap = NULL;
    }

    if (virtual_info->vlan_vp_bitmap) {
        sal_free(virtual_info->vlan_vp_bitmap);
        virtual_info->vlan_vp_bitmap = NULL;
    }

    if (virtual_info->niv_vp_bitmap) {
        sal_free(virtual_info->niv_vp_bitmap);
        virtual_info->niv_vp_bitmap = NULL;
    }

    if (virtual_info->l2gre_vfi_bitmap) {
        sal_free(virtual_info->l2gre_vfi_bitmap);
        virtual_info->l2gre_vfi_bitmap = NULL;
    }

    if (virtual_info->l2gre_vp_bitmap) {
        sal_free(virtual_info->l2gre_vp_bitmap);
        virtual_info->l2gre_vp_bitmap = NULL;
    }

    if (virtual_info->vxlan_vfi_bitmap) {
        sal_free(virtual_info->vxlan_vfi_bitmap);
        virtual_info->vxlan_vfi_bitmap = NULL;
    }

    if (virtual_info->vxlan_vp_bitmap) {
        sal_free(virtual_info->vxlan_vp_bitmap);
        virtual_info->vxlan_vp_bitmap = NULL;
    }

    if (virtual_info->extender_vp_bitmap) {
        sal_free(virtual_info->extender_vp_bitmap);
        virtual_info->extender_vp_bitmap = NULL;
    }

    if (virtual_info->vp_lag_vp_bitmap) {
        sal_free(virtual_info->vp_lag_vp_bitmap);
        virtual_info->vp_lag_vp_bitmap = NULL;
    }
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_VERSION_1_1                SOC_SCACHE_VERSION(1,1)
#define BCM_WB_VERSION_1_2                SOC_SCACHE_VERSION(1,2)
#define BCM_WB_VERSION_1_3                SOC_SCACHE_VERSION(1,3)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_3

/*
 * Function:
 *      _bcm_esw_virtual_sync
 * Purpose:
 *      - Copy the s/w state of 'Virtual' module to external
 *        memory.
 *      - Not called during Warm Boot.
 * Parameters:
 *      unit    -  (IN) Device number.
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_virtual_sync(int unit)
{
    int                 stable_size, alloc_sz = 0;
    int                 num_vp, num_vfi;
    uint8               *virtual_bitmap;
    soc_scache_handle_t scache_handle;  /* SCache reference number        */

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    if ((0 == stable_size) || (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit))) {
        return BCM_E_NONE;
    }
    if ((!SOC_MEM_IS_VALID(unit, VFIm)) ||
        (!SOC_MEM_IS_VALID(unit, SOURCE_VPm))) {
        /* ===============================================       */
        /* Either VFI not present or SOURCE_VP not present.      */
        /* Possible with some chips(hurricane) so not continuing */
        /* ===============================================       */

        /* if (SOC_IS_HURRICANE(unit)) */
        return BCM_E_NONE;
    }

    num_vfi = soc_mem_index_count(unit, VFIm);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    alloc_sz += SHR_BITALLOCSIZE(num_vp);
    alloc_sz += SHR_BITALLOCSIZE(num_vfi);
    /* mpls vp */
    alloc_sz += SHR_BITALLOCSIZE(num_vp);
    /* mpls vfi */
    alloc_sz += SHR_BITALLOCSIZE(num_vfi);
    /* mim vp */
    alloc_sz += SHR_BITALLOCSIZE(num_vp);
    /* mim vfi */
    alloc_sz += SHR_BITALLOCSIZE(num_vfi);
    /* subport vp */
    alloc_sz += SHR_BITALLOCSIZE(num_vp);
    /* vlan vp */
    if (soc_feature(unit, soc_feature_vlan_vp)) {
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
    }
    /* niv vp */
    if (soc_feature(unit, soc_feature_niv)) {
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
    }
    /* trill vp */
    if (soc_feature(unit, soc_feature_trill)) {
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
    }
    /* l2gre vp */
    if (soc_feature(unit, soc_feature_l2gre)) {
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
    }
    /* extender vp */
    if (soc_feature(unit, soc_feature_port_extension)) {
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
    }
    /* vp lag vp */
    if (soc_feature(unit, soc_feature_vp_lag)) {
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
    }

    /* vxlan vfi & vp */
    if (soc_feature(unit, soc_feature_vxlan)) {
        alloc_sz += SHR_BITALLOCSIZE(num_vfi); 
        alloc_sz += SHR_BITALLOCSIZE(num_vp); 
    } 


    SOC_SCACHE_HANDLE_SET(scache_handle,
                          unit, BCM_MODULE_VIRTUAL, 0);

    if (stable_size > alloc_sz) {
        SOC_IF_ERROR_RETURN (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                             alloc_sz, &virtual_bitmap, 
                             BCM_WB_DEFAULT_VERSION, NULL));
        
        VIRTUAL_LOCK(unit);
        sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->vp_bitmap,
                   SHR_BITALLOCSIZE(num_vp));
        virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
        sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->vfi_bitmap,
                   SHR_BITALLOCSIZE(num_vfi));
        virtual_bitmap += SHR_BITALLOCSIZE(num_vfi);
        sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->mpls_vp_bitmap,
                   SHR_BITALLOCSIZE(num_vp));
        virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
        sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->mpls_vfi_bitmap,
                   SHR_BITALLOCSIZE(num_vfi));
        virtual_bitmap += SHR_BITALLOCSIZE(num_vfi);
        sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->mim_vp_bitmap,
                   SHR_BITALLOCSIZE(num_vp));
        virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
        sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->mim_vfi_bitmap,
                   SHR_BITALLOCSIZE(num_vfi));
        virtual_bitmap += SHR_BITALLOCSIZE(num_vfi);
        sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->subport_vp_bitmap,
                   SHR_BITALLOCSIZE(num_vp));
        
        if (soc_feature(unit, soc_feature_vlan_vp)) {
            virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
            sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->vlan_vp_bitmap, 
                       SHR_BITALLOCSIZE(num_vp));
        }

        if (soc_feature(unit, soc_feature_niv)) {
            virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
            sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->niv_vp_bitmap,
                           SHR_BITALLOCSIZE(num_vp));
        }

        if (soc_feature(unit, soc_feature_trill)) {
                virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->trill_vp_bitmap,
                          SHR_BITALLOCSIZE(num_vp));
        }

        if (soc_feature(unit, soc_feature_l2gre)) {
                virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->l2gre_vp_bitmap,
                          SHR_BITALLOCSIZE(num_vp));
        }

        if (soc_feature(unit, soc_feature_port_extension)) {
            virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
            sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->extender_vp_bitmap,
                           SHR_BITALLOCSIZE(num_vp));
        }

        if (soc_feature(unit, soc_feature_vp_lag)) {
            virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
            sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->vp_lag_vp_bitmap,
                           SHR_BITALLOCSIZE(num_vp));
        }

        /* 
         *  BCM_WB_VERSION_1_3
         */

        /* Store VXLAN vfi/vp BITMAP */
        /* 
         *  VXLAN vfi/vp BITMAP points to a struct array with a base type of uint32.
         *  sized to hold the maximum number of VFI / VP
         * 
         *  uint32[(size of (SOURCE_VPm) + 32) / 32]     *vxlan_ip_tnl_bitmap
         */

        if (soc_feature(unit, soc_feature_vxlan)) {
            virtual_bitmap += SHR_BITALLOCSIZE(num_vfi);
            sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->vxlan_vfi_bitmap,
                           SHR_BITALLOCSIZE(num_vfi));
            virtual_bitmap += SHR_BITALLOCSIZE(num_vp);
            sal_memcpy(virtual_bitmap, VIRTUAL_INFO(unit)->vxlan_vp_bitmap,
                           SHR_BITALLOCSIZE(num_vp));            
        }

        VIRTUAL_UNLOCK(unit);
 
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_virtual_reinit
 * Purpose:
 *      - Top level reinit func for Virtual module.
 *      - Do nothing for Unsynchronized (level 1) and Limited 
 *        scache (level 1.5) based recovery.
 *      - Extended scache (level 2) based recovery:
 *        a. Cold Boot: Allocate external memory required to store 
 *                 module specific s/w state.
 *        b. Warm Boot: Recover the s/w state from external memory.
 * Parameters:
 *      unit    -  (IN) Device number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_virtual_reinit(int unit) {

    int rv = BCM_E_NONE;
    int32  stable_size = 0;
    uint32 num_vp, num_vfi;
    uint32 alloc_sz = 0;
    uint8  *virtual_bitmap;
    uint16 recovered_ver;
    uint32  virtual_scache_size;
    soc_scache_handle_t scache_handle;  /* SCache reference number */

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    if ((0 == stable_size) || (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit))) {
    /* Unsynchronized or Limited scache support based recovery */
        return rv;
    } else { 
    /* Extended scache support available */

        num_vfi = soc_mem_index_count(unit, VFIm);
        num_vp = soc_mem_index_count(unit, SOURCE_VPm);
        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_VIRTUAL, 0);
    
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
        alloc_sz += SHR_BITALLOCSIZE(num_vfi);
        /* mpls vp */
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
        /* mpls vfi */
        alloc_sz += SHR_BITALLOCSIZE(num_vfi);
        /* mim vp */
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
        /* mim vfi */
        alloc_sz += SHR_BITALLOCSIZE(num_vfi);
        /* subport vp */
        alloc_sz += SHR_BITALLOCSIZE(num_vp);
        /* vlan vp */
        if (soc_feature(unit, soc_feature_vlan_vp)) {
            alloc_sz += SHR_BITALLOCSIZE(num_vp);
        }
        /* niv vp */
        if (soc_feature(unit, soc_feature_niv)) {
            alloc_sz += SHR_BITALLOCSIZE(num_vp);
        }
        /* trill vp */
        if (soc_feature(unit, soc_feature_trill)) {
            alloc_sz += SHR_BITALLOCSIZE(num_vp);
        }
        /* l2gre vp */
        if (soc_feature(unit, soc_feature_l2gre)) {
            alloc_sz += SHR_BITALLOCSIZE(num_vp);
        }
        /* extender vp */
        if (soc_feature(unit, soc_feature_port_extension)) {
            alloc_sz += SHR_BITALLOCSIZE(num_vp);
        }
        /* vp lag vp */
        if (soc_feature(unit, soc_feature_vp_lag)) {
            alloc_sz += SHR_BITALLOCSIZE(num_vp);
        }
        /* vxlan vfi & vp */
        if (soc_feature(unit, soc_feature_vxlan)) {
            alloc_sz += SHR_BITALLOCSIZE(num_vfi); 
            alloc_sz += SHR_BITALLOCSIZE(num_vp); 
        }  
       
        if (SOC_WARM_BOOT(unit)) {
        /* Warm Boot */
            if (stable_size > alloc_sz) {
                SOC_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle,
                                    FALSE, alloc_sz, &virtual_bitmap, 
                                    BCM_WB_DEFAULT_VERSION, &recovered_ver));
                sal_memcpy(VIRTUAL_INFO(unit)->vp_bitmap, virtual_bitmap,
                          SHR_BITALLOCSIZE(num_vp));
                virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                sal_memcpy(VIRTUAL_INFO(unit)->vfi_bitmap, virtual_bitmap,
                          SHR_BITALLOCSIZE(num_vfi));
                virtual_bitmap += SHR_BITALLOCSIZE(num_vfi);
                sal_memcpy(VIRTUAL_INFO(unit)->mpls_vp_bitmap, virtual_bitmap,
                          SHR_BITALLOCSIZE(num_vp));
                virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                sal_memcpy(VIRTUAL_INFO(unit)->mpls_vfi_bitmap, virtual_bitmap,
                          SHR_BITALLOCSIZE(num_vfi));
                virtual_bitmap += SHR_BITALLOCSIZE(num_vfi);
                sal_memcpy(VIRTUAL_INFO(unit)->mim_vp_bitmap, virtual_bitmap,
                          SHR_BITALLOCSIZE(num_vp));
                virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                sal_memcpy(VIRTUAL_INFO(unit)->mim_vfi_bitmap, virtual_bitmap,
                          SHR_BITALLOCSIZE(num_vfi));
                virtual_bitmap += SHR_BITALLOCSIZE(num_vfi);
                sal_memcpy(VIRTUAL_INFO(unit)->subport_vp_bitmap, virtual_bitmap,
                          SHR_BITALLOCSIZE(num_vp));

                virtual_scache_size = 0;
                if (recovered_ver >= BCM_WB_VERSION_1_1) {
                    if (soc_feature(unit, soc_feature_vlan_vp)) {
                            virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                            sal_memcpy(VIRTUAL_INFO(unit)->vlan_vp_bitmap, virtual_bitmap,
                                      SHR_BITALLOCSIZE(num_vp));
                    }

                    if (soc_feature(unit, soc_feature_niv)) {
                            virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                            sal_memcpy(VIRTUAL_INFO(unit)->niv_vp_bitmap, virtual_bitmap,
                                      SHR_BITALLOCSIZE(num_vp));
                    }
                  
                    if (soc_feature(unit, soc_feature_trill)) {
                            virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                            sal_memcpy(VIRTUAL_INFO(unit)->trill_vp_bitmap, virtual_bitmap,
                                      SHR_BITALLOCSIZE(num_vp));
                    }
                } else {
                    if (soc_feature(unit, soc_feature_vlan_vp)) {
                            virtual_scache_size += SHR_BITALLOCSIZE(num_vp); 
                    }
                    if (soc_feature(unit, soc_feature_niv)) {
                            virtual_scache_size += SHR_BITALLOCSIZE(num_vp); 
                    }
                    if (soc_feature(unit, soc_feature_trill)) {
                            virtual_scache_size += SHR_BITALLOCSIZE(num_vp); 
                    }
                }

                if (recovered_ver >= BCM_WB_VERSION_1_2) {
                    if (soc_feature(unit, soc_feature_l2gre)) {
                            virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                            sal_memcpy(VIRTUAL_INFO(unit)->l2gre_vp_bitmap, virtual_bitmap,
                                       SHR_BITALLOCSIZE(num_vp));
                    }
                    if (soc_feature(unit, soc_feature_port_extension)) {
                            virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                            sal_memcpy(VIRTUAL_INFO(unit)->extender_vp_bitmap, virtual_bitmap,
                                      SHR_BITALLOCSIZE(num_vp));
                    }
                    if (soc_feature(unit, soc_feature_vp_lag)) {
                            virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                            sal_memcpy(VIRTUAL_INFO(unit)->vp_lag_vp_bitmap, virtual_bitmap,
                                      SHR_BITALLOCSIZE(num_vp));
                    }
                } else {
                    if (soc_feature(unit, soc_feature_l2gre)) {
                        virtual_scache_size += SHR_BITALLOCSIZE(num_vp); 
                    }
                    if (soc_feature(unit, soc_feature_port_extension)) {
                        virtual_scache_size += SHR_BITALLOCSIZE(num_vp); 
                    }
                    if (soc_feature(unit, soc_feature_vp_lag)) {
                        virtual_scache_size += SHR_BITALLOCSIZE(num_vp); 
                    }
                }

                if (recovered_ver >= BCM_WB_VERSION_1_3) {
                    if (soc_feature(unit, soc_feature_vxlan)) {
                        virtual_bitmap += SHR_BITALLOCSIZE(num_vfi); 
                        sal_memcpy(VIRTUAL_INFO(unit)->vxlan_vfi_bitmap, virtual_bitmap,
                                   SHR_BITALLOCSIZE(num_vfi));                        
                        virtual_bitmap += SHR_BITALLOCSIZE(num_vp); 
                        sal_memcpy(VIRTUAL_INFO(unit)->vxlan_vp_bitmap, virtual_bitmap,
                                   SHR_BITALLOCSIZE(num_vp));
                    }
                } else {
                    if (soc_feature(unit, soc_feature_vxlan)) {
                        virtual_scache_size += SHR_BITALLOCSIZE(num_vfi); 
                        virtual_scache_size += SHR_BITALLOCSIZE(num_vp); 
                    }
                } 

                if (virtual_scache_size > 0) {
                    SOC_IF_ERROR_RETURN
                        (soc_scache_realloc(unit, scache_handle, virtual_scache_size));
                }
                 
            } else {
                rv = BCM_E_INTERNAL; /* stable_size < alloc_size */
            }
        } else {
        /* Cold Boot */
            if (stable_size > alloc_sz) {
                rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
                                             alloc_sz, &virtual_bitmap, 
                                             BCM_WB_DEFAULT_VERSION, NULL);
                if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                    return rv;
                }
            }
        }
    }
    return rv;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      _bcm_virtual_init
 * Purpose:
 *      Internal function for initializing virtual resource management
 * Parameters:
 *      unit    -  (IN) Device number.
 * Returns:
 *      BCM_X_XXX
 */
int 
_bcm_virtual_init(int unit, soc_mem_t vp_mem, soc_mem_t vfi_mem)
{
    int num_vfi, num_vp, num_wlan_vp=0, vp, rv = BCM_E_NONE;
    _bcm_virtual_bookkeeping_t *virtual_info = VIRTUAL_INFO(unit);

    num_vfi = soc_mem_index_count(unit, vfi_mem);
    num_vp = soc_mem_index_count(unit, vp_mem);
    if (soc_feature(unit, soc_feature_wlan)) {
        if (SOC_MEM_IS_VALID(unit, WLAN_SVP_TABLEm)) {
            num_wlan_vp = soc_mem_index_count(unit, WLAN_SVP_TABLEm);
        } else {
            num_wlan_vp = soc_mem_index_count(unit, SOURCE_VPm);
        }
    }
    /* 
     * Allocate resources 
     */
    if (NULL == _virtual_mutex[unit]) {
        _virtual_mutex[unit] = sal_mutex_create("virtual mutex");
        if (_virtual_mutex[unit] == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (NULL == virtual_info->vfi_bitmap) {
        virtual_info->vfi_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(num_vfi), "vfi_bitmap");
        if (virtual_info->vfi_bitmap == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (NULL == virtual_info->vp_bitmap) {
        virtual_info->vp_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(num_vp), "vp_bitmap");
        if (virtual_info->vp_bitmap == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (NULL == virtual_info->mpls_vfi_bitmap) {
        virtual_info->mpls_vfi_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(num_vfi), "mpls_vfi_bitmap");
        if (virtual_info->mpls_vfi_bitmap == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (NULL == virtual_info->mpls_vp_bitmap) {
        virtual_info->mpls_vp_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(num_vp), "mpls_vp_bitmap");
        if (virtual_info->mpls_vp_bitmap == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (NULL == virtual_info->mim_vfi_bitmap) {
        virtual_info->mim_vfi_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(num_vfi), "mim_vfi_bitmap");
        if (virtual_info->mim_vfi_bitmap == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (NULL == virtual_info->mim_vp_bitmap) {
        virtual_info->mim_vp_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(num_vp), "mim_vp_bitmap");
        if (virtual_info->mim_vp_bitmap == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (NULL == virtual_info->subport_vp_bitmap) {
        virtual_info->subport_vp_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(num_vp), "subport_vp_bitmap");
        if (virtual_info->subport_vp_bitmap == NULL) {
            _bcm_virtual_free_resource(unit, virtual_info);
            return BCM_E_MEMORY;
        }
    }

    if (soc_feature(unit, soc_feature_wlan)) {
        if (NULL == virtual_info->wlan_vp_bitmap) {
            virtual_info->wlan_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_wlan_vp), "wlan_vp_bitmap");
            if (virtual_info->wlan_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    if (soc_feature(unit, soc_feature_trill)) {
        if (NULL == virtual_info->trill_vp_bitmap) {
            virtual_info->trill_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_vp), "trill_vp_bitmap");
            if (virtual_info->trill_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    if (soc_feature(unit, soc_feature_vlan_vp)) {
        if (NULL == virtual_info->vlan_vp_bitmap) {
            virtual_info->vlan_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_vp), "vlan_vp_bitmap");
            if (virtual_info->vlan_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    if (soc_feature(unit, soc_feature_niv)) {
        if (NULL == virtual_info->niv_vp_bitmap) {
            virtual_info->niv_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_vp), "niv_vp_bitmap");
            if (virtual_info->niv_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    if (soc_feature(unit, soc_feature_l2gre)) {
        if (NULL == virtual_info->l2gre_vfi_bitmap) {
            virtual_info->l2gre_vfi_bitmap =
                   sal_alloc(SHR_BITALLOCSIZE(num_vfi), "l2gre_vfi_bitmap");
            if (virtual_info->l2gre_vfi_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }
    
    if (soc_feature(unit, soc_feature_l2gre)) {
        if (NULL == virtual_info->l2gre_vp_bitmap) {
            virtual_info->l2gre_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_vp), "l2gre_vp_bitmap");
            if (virtual_info->l2gre_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    if (soc_feature(unit, soc_feature_vxlan)) {
        if (NULL == virtual_info->vxlan_vfi_bitmap) {
            virtual_info->vxlan_vfi_bitmap =
                   sal_alloc(SHR_BITALLOCSIZE(num_vfi), "vxlan_vfi_bitmap");
            if (virtual_info->vxlan_vfi_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }
    
    if (soc_feature(unit, soc_feature_vxlan)) {
        if (NULL == virtual_info->vxlan_vp_bitmap) {
            virtual_info->vxlan_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_vp), "vxlan_vp_bitmap");
            if (virtual_info->vxlan_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    if (soc_feature(unit, soc_feature_port_extension)) {
        if (NULL == virtual_info->extender_vp_bitmap) {
            virtual_info->extender_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_vp), "extender_vp_bitmap");
            if (virtual_info->extender_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    if (soc_feature(unit, soc_feature_vp_lag)) {
        if (NULL == virtual_info->vp_lag_vp_bitmap) {
            virtual_info->vp_lag_vp_bitmap =
                sal_alloc(SHR_BITALLOCSIZE(num_vp), "vp_lag_vp_bitmap");
            if (virtual_info->vp_lag_vp_bitmap == NULL) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return BCM_E_MEMORY;
            }
        }
    }

    /*
     * Initialize 
     */ 
    sal_memset(virtual_info->vfi_bitmap, 0, SHR_BITALLOCSIZE(num_vfi));
    sal_memset(virtual_info->vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    sal_memset(virtual_info->mpls_vfi_bitmap, 0, SHR_BITALLOCSIZE(num_vfi));
    sal_memset(virtual_info->mpls_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    sal_memset(virtual_info->mim_vfi_bitmap, 0, SHR_BITALLOCSIZE(num_vfi));
    sal_memset(virtual_info->mim_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    sal_memset(virtual_info->subport_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    if (soc_feature(unit, soc_feature_wlan)) {
        sal_memset(virtual_info->wlan_vp_bitmap, 0, SHR_BITALLOCSIZE(num_wlan_vp));
    }	
    if (soc_feature(unit, soc_feature_trill)) {
        sal_memset(virtual_info->trill_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    }
    if (soc_feature(unit, soc_feature_vlan_vp)) {
        sal_memset(virtual_info->vlan_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    }
    if (soc_feature(unit, soc_feature_niv)) {
        sal_memset(virtual_info->niv_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    }
    if (soc_feature(unit, soc_feature_l2gre)) {
        sal_memset(virtual_info->l2gre_vfi_bitmap, 0, SHR_BITALLOCSIZE(num_vfi));
        sal_memset(virtual_info->l2gre_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    }
    if (soc_feature(unit, soc_feature_vxlan)) {
        sal_memset(virtual_info->vxlan_vfi_bitmap, 0, SHR_BITALLOCSIZE(num_vfi));
        sal_memset(virtual_info->vxlan_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    }
    if (soc_feature(unit, soc_feature_port_extension)) {
        sal_memset(virtual_info->extender_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    }
    if (soc_feature(unit, soc_feature_vp_lag)) {
        sal_memset(virtual_info->vp_lag_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    }
    /* HW retriction - mark VP index zero as used */
    rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeAny, &vp);
    if (vp != 0) {
        rv = BCM_E_INTERNAL;
    }
    if (BCM_FAILURE(rv)) {
        _bcm_virtual_free_resource(unit, virtual_info);
        return rv;
    }

    /* Now that the VP bitmap is initialized, register it with the
     * SOC layer L2X thread to use during freeze/thaw. */
    soc_l2x_cml_vp_bitmap_set(unit, virtual_info->vp_bitmap);

#ifdef BCM_WARM_BOOT_SUPPORT
    rv = _bcm_virtual_reinit(unit);
    if (rv != BCM_E_NONE) {
        _bcm_virtual_free_resource(unit, virtual_info);
        return rv;
    }

    if (SOC_WARM_BOOT(unit)) {
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_vlan_vp)) {
            /* Warm boot recovery of VLAN module's VLAN_VP_INFO is postponed to
             * here since it depends on the warm boot recovery of this module's
             * virtual_info.
             */
#ifdef BCM_ENDURO_SUPPORT
            if (SOC_IS_ENDURO(unit)) {
                rv = bcm_enduro_vlan_virtual_reinit(unit);
            } else
#endif /* BCM_ENDURO_SUPPORT */
            {
                rv = bcm_tr2_vlan_virtual_reinit(unit);
            } 
            if (BCM_FAILURE(rv)) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_vp_lag)) {
            /* Warm boot recovery of trunk module's VP LAG info is postponed to
             * here since it depends on the warm boot recovery of this module's
             * VP LAG VP bitmap.
             */
            rv = bcm_td2_vp_lag_reinit(unit);
            if (BCM_FAILURE(rv)) {
                _bcm_virtual_free_resource(unit, virtual_info);
                return rv;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */

    }

#endif /* BCM_WARM_BOOT_SUPPORT */

    return rv;
}

/*
 * Function:
 *      _bcm_vp_alloc
 * Purpose:
 *      Internal function for allocating a group of VPs
 * Parameters:
 *      unit    -  (IN) Device number.
 *      start   -  (IN) First VP index to allocate from 
 *      end     -  (IN) Last VP index to allocate from 
 *      count   -  (IN) How many consecutive VPs to allocate
 *      vp_mem  -  (IN) HW specific VP memory
 *      type    -  (IN) VP type
 *      base_vp -  (OUT) Base VP index
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_vp_alloc(int unit, int start, int end, int count, soc_mem_t vp_mem, 
              _bcm_vp_type_e type, int *base_vp)
{
    int num_vp = soc_mem_index_count(unit, vp_mem);
    int i, j;

    if ((end >= num_vp) || (start >= num_vp)) {
        return BCM_E_PARAM;
    }
    if (type == _bcmVpTypeWlan) {
        if (!soc_feature(unit, soc_feature_wlan)) {
            return BCM_E_PORT;
        }
    }

    /* vp_bitmap referenced by _BCM_VIRTUAL_VP_XXX macros, tracks
     * the usage of SOURCE_VP table because all vp types except wlan use the
     * same hardware table SOURCE_VP for vp assignment.  
     * Since Wlan vp uses a different table exclusively for vp assignment, 
     * the wlan_vp_bitmap alone should be sufficient to track wlan vp usage. 
     */
    VIRTUAL_LOCK(unit);
    if (type == _bcmVpTypeWlan) {
        for (i = start; i <= end; i += count) {
            for (j = 0; j < count; j++) {
                if (_BCM_WLAN_VP_USED_GET(unit, i + j)) {
                    break;
                }
            }
            if (j == count) {
                break;
            }
        }
        if (i <= end) {
            *base_vp = i;
            for (j = 0; j < count; j++) {
                _BCM_WLAN_VP_USED_SET(unit, i + j);
            }
            VIRTUAL_UNLOCK(unit);
            return BCM_E_NONE;
        }
        VIRTUAL_UNLOCK(unit);
        return BCM_E_RESOURCE;
    }

    /* handle all other vp types except wlan */
    for (i = start; i <= end; i += count) {
        for (j = 0; j < count; j++) {
            if (_BCM_VIRTUAL_VP_USED_GET(unit, i + j)) {
                break;
            }
        }
        if (j == count) {
            break;
        }
    }
    if (i <= end) {
        *base_vp = i;
        for (j = 0; j < count; j++) {
            _BCM_VIRTUAL_VP_USED_SET(unit, i + j);
            switch (type) {
            case _bcmVpTypeMpls:
                _BCM_MPLS_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeMim:
                _BCM_MIM_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeSubport:
                _BCM_SUBPORT_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeTrill:
                if (!soc_feature(unit, soc_feature_trill)) {
                    VIRTUAL_UNLOCK(unit);
                    return BCM_E_PORT;
                }
                _BCM_TRILL_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeVlan:
                if (!soc_feature(unit, soc_feature_vlan_vp)) {
                    VIRTUAL_UNLOCK(unit);
                    return BCM_E_PORT;
                }
                _BCM_VLAN_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeNiv:
                if (!soc_feature(unit, soc_feature_niv)) {
                    VIRTUAL_UNLOCK(unit);
                    return BCM_E_PORT;
                }
                _BCM_NIV_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeL2Gre:
                if (!soc_feature(unit, soc_feature_l2gre)) {
                    VIRTUAL_UNLOCK(unit);
                    return BCM_E_PORT;
                }
                _BCM_L2GRE_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeVxlan:
                if (!soc_feature(unit, soc_feature_vxlan)) {
                    VIRTUAL_UNLOCK(unit);
                    return BCM_E_PORT;
                }
                _BCM_VXLAN_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeExtender:
                if (!soc_feature(unit, soc_feature_port_extension)) {
                    VIRTUAL_UNLOCK(unit);
                    return BCM_E_PORT;
                }
                _BCM_EXTENDER_VP_USED_SET(unit, i + j);
                break;
            case _bcmVpTypeVpLag:
                if (!soc_feature(unit, soc_feature_vp_lag)) {
                    VIRTUAL_UNLOCK(unit);
                    return BCM_E_PORT;
                }
                _BCM_VP_LAG_VP_USED_SET(unit, i + j);
                break;
            default:
                break;
            }
        }
        VIRTUAL_UNLOCK(unit);
        return BCM_E_NONE;
    }
    VIRTUAL_UNLOCK(unit);
    return BCM_E_RESOURCE;
}

/*
 * Function:
 *      _bcm_vp_free
 * Purpose:
 *      Internal function for freeing a group of VPs
 * Parameters:
 *      unit    -  (IN) Device number
 *      type    -  (IN) VP type
 *      count   -  (IN) How many consecutive VPs to free
 *      base_vp -  (IN) Base VP index
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_vp_free(int unit, _bcm_vp_type_e type, int count, int base_vp)
{
    int i;
    VIRTUAL_LOCK(unit);
    for (i = 0; i < count; i++) {
        if (type != _bcmVpTypeWlan) {
            _BCM_VIRTUAL_VP_USED_CLR(unit, base_vp + i);
        }
        switch (type) {
        case _bcmVpTypeMpls:
            _BCM_MPLS_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeMim:
            _BCM_MIM_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeSubport:
            _BCM_SUBPORT_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeWlan:
            if (!soc_feature(unit, soc_feature_wlan)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_WLAN_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeTrill:
            if (!soc_feature(unit, soc_feature_trill)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_TRILL_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeVlan:
            if (!soc_feature(unit, soc_feature_vlan_vp)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_VLAN_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeNiv:
            if (!soc_feature(unit, soc_feature_niv)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_NIV_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeL2Gre:
            if (!soc_feature(unit, soc_feature_l2gre)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_L2GRE_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeVxlan:
            if (!soc_feature(unit, soc_feature_vxlan)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_VXLAN_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeExtender:
            if (!soc_feature(unit, soc_feature_port_extension)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_EXTENDER_VP_USED_CLR(unit, base_vp + i);
            break;
        case _bcmVpTypeVpLag:
            if (!soc_feature(unit, soc_feature_vp_lag)) {
                VIRTUAL_UNLOCK(unit);
                return BCM_E_PORT;
            }
            _BCM_VP_LAG_VP_USED_CLR(unit, base_vp + i);
            break;
        default:
            break;
        }
    }
    VIRTUAL_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_vp_used_set
 * Purpose:
 *      Mark the VP as used
 * Parameters:
 *      unit    -  (IN) Device number.
 *      vp      -  (IN) VP
 *      type   -  (IN) VP type
 * Returns:
 *      Boolean
 */
int
_bcm_vp_used_set(int unit, int vp, _bcm_vp_type_e type)
{
    int rv=BCM_E_NONE;
    VIRTUAL_LOCK(unit);

    if (type != _bcmVpTypeWlan) {
        _BCM_VIRTUAL_VP_USED_SET(unit, vp);
    }
    switch (type) {
         case _bcmVpTypeMpls:
              _BCM_MPLS_VP_USED_SET(unit, vp);
             break;
         case _bcmVpTypeMim:
              _BCM_MIM_VP_USED_SET(unit, vp);
             break;
         case _bcmVpTypeSubport:
              _BCM_SUBPORT_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeWlan:
              if (!soc_feature(unit, soc_feature_wlan)) {
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_WLAN_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeTrill:
              if (!soc_feature(unit, soc_feature_trill)) {
                  _BCM_VIRTUAL_VP_USED_CLR(unit, vp);
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_TRILL_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeVlan:
              if (!soc_feature(unit, soc_feature_vlan_vp)) {
                  _BCM_VIRTUAL_VP_USED_CLR(unit, vp);
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_VLAN_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeNiv:
              if (!soc_feature(unit, soc_feature_niv)) {
                  _BCM_VIRTUAL_VP_USED_CLR(unit, vp);
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_NIV_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeL2Gre:
              if (!soc_feature(unit, soc_feature_l2gre)) {
                  _BCM_VIRTUAL_VP_USED_CLR(unit, vp);
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_L2GRE_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeVxlan:
              if (!soc_feature(unit, soc_feature_vxlan)) {
                  _BCM_VIRTUAL_VP_USED_CLR(unit, vp);
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_VXLAN_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeExtender:
              if (!soc_feature(unit, soc_feature_port_extension)) {
                  _BCM_VIRTUAL_VP_USED_CLR(unit, vp);
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_EXTENDER_VP_USED_SET(unit, vp);
              break;
         case _bcmVpTypeVpLag:
              if (!soc_feature(unit, soc_feature_vp_lag)) {
                  _BCM_VIRTUAL_VP_USED_CLR(unit, vp);
                  VIRTUAL_UNLOCK(unit);
                  return BCM_E_PORT;
              }
              _BCM_VP_LAG_VP_USED_SET(unit, vp);
              break;
         default:
              break;
    }

    VIRTUAL_UNLOCK(unit);
    return rv;
}



/*
 * Function:
 *      _bcm_vp_used_get
 * Purpose:
 *      Check whether a VP is used or not
 * Parameters:
 *      unit    -  (IN) Device number.
 *      vp      -  (IN) VP
 * Returns:
 *      Boolean
 */
int
_bcm_vp_used_get(int unit, int vp, _bcm_vp_type_e type)
{
    int rv = BCM_E_NONE;

    
    switch (type) {
    case _bcmVpTypeMpls:
        if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
            return -1;
        }
        rv = _BCM_MPLS_VP_USED_GET(unit, vp);
        break;
    case _bcmVpTypeMim:
        if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
            return -1;
        }
        rv = _BCM_MIM_VP_USED_GET(unit, vp);
        break;
    case _bcmVpTypeSubport:
        if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
            return -1;
        }
        rv = _BCM_SUBPORT_VP_USED_GET(unit, vp);
        break;
    case _bcmVpTypeWlan:
        if (soc_feature(unit, soc_feature_wlan)) {
            if (SOC_MEM_IS_VALID(unit, WLAN_SVP_TABLEm)) {
                if (vp > soc_mem_index_count(unit, WLAN_SVP_TABLEm)) {
                    return -1;
                }
            } else if (SOC_MEM_IS_VALID(unit,SOURCE_VP_ATTRIBUTES_2m)) {
                if (vp > soc_mem_index_count(unit, SOURCE_VP_ATTRIBUTES_2m)) {
                    return -1;
                }
            } else {
                return -1;
            }
            rv = _BCM_WLAN_VP_USED_GET(unit, vp);
        }
        break;
    case _bcmVpTypeTrill:
        if (soc_feature(unit, soc_feature_trill)) {
            if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
                return -1;
            }
            rv = _BCM_TRILL_VP_USED_GET(unit, vp);
        }
        break;
    case _bcmVpTypeVlan:
        if (soc_feature(unit, soc_feature_vlan_vp)) {
            if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
                return -1;
            }
            rv = _BCM_VLAN_VP_USED_GET(unit, vp);
        }
        break;
    case _bcmVpTypeNiv:
        if (soc_feature(unit, soc_feature_niv)) {
            if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
                return -1;
            }
            rv = _BCM_NIV_VP_USED_GET(unit, vp);
        }
        break;
    case _bcmVpTypeL2Gre:
        if (soc_feature(unit, soc_feature_l2gre)) {
            if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
                return -1;
            }
            rv = _BCM_L2GRE_VP_USED_GET(unit, vp);
        }
        break;
    case _bcmVpTypeVxlan:
        if (soc_feature(unit, soc_feature_vxlan)) {
            if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
                return -1;
            }
            rv = _BCM_VXLAN_VP_USED_GET(unit, vp);
        }
        break;
    case _bcmVpTypeExtender:
        if (soc_feature(unit, soc_feature_port_extension)) {
            if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
                return -1;
            }
            rv = _BCM_EXTENDER_VP_USED_GET(unit, vp);
        }
        break;
    case _bcmVpTypeVpLag:
        if (soc_feature(unit, soc_feature_vp_lag)) {
            if (vp > soc_mem_index_count(unit, SOURCE_VPm)) {
                return -1;
            }
            rv = _BCM_VP_LAG_VP_USED_GET(unit, vp);
        }
        break;
    default:
        rv = _BCM_VIRTUAL_VP_USED_GET(unit, vp);
        break;
    }
    return rv;
}


/*
 * Function:
 *      _bcm_vp_default_cml_mode_get
 * Purpose:
 *      Get Gport default cml_enable
 * Parameters:
 *      unit    -  (IN) Device number.
 *      cml_default_enable  -  (OUT) default_cml_mode_enable
 *      cml_new  - (OUT) default cml_new
 *      cml_move - (OUT) default cml_move
 * Returns:
 *      Boolean
 */
int
_bcm_vp_default_cml_mode_get (int unit, int *cml_default_enable, int *cml_new, int *cml_move)
{
   int vp = 0;
   int rv = BCM_E_NONE;
   source_vp_entry_t svp;

    /* HW retriction - VP index zero is reserved */
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeAny)) {
        return BCM_E_UNAVAIL;
    }
    BCM_IF_ERROR_RETURN
          (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));

    if (soc_SOURCE_VPm_field32_get(unit, &svp, CLASS_IDf)) {
         *cml_new = soc_SOURCE_VPm_field32_get(unit, &svp, CML_FLAGS_NEWf);
         *cml_default_enable = 0x1;
    } else {
         *cml_new = 0x8; /* Default Value of cml */
    }

    if (soc_SOURCE_VPm_field32_get(unit, &svp, DVPf)) {
         *cml_move = soc_SOURCE_VPm_field32_get(unit, &svp, CML_FLAGS_MOVEf);
         *cml_default_enable = 0x1;
    } else {
         *cml_move = 0x8; /* Default Value of cml */
    }
    return rv;
}


/*
 * Function:
 *      _bcm_vfi_alloc
 * Purpose:
 *      Internal function for allocating a VFI
 * Parameters:
 *      unit    -  (IN) Device number.
 *      vfi_mem -  (IN) HW specific VFI memory
 *      type    -  (IN) VFI type
 *      vfi     -  (OUT) Base VP index
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_vfi_alloc(int unit, soc_mem_t vfi_mem, _bcm_vfi_type_e type, int *vfi)
{
    int i, num_vfi;
    num_vfi = soc_mem_index_count(unit, vfi_mem);
    VIRTUAL_LOCK(unit);
    for (i = 0; i < num_vfi; i++) {
        if (!_BCM_VIRTUAL_VFI_USED_GET(unit, i)) {
            break;
        }
    }
    if (i == num_vfi) {
        VIRTUAL_UNLOCK(unit);
        return BCM_E_RESOURCE;
    }
    *vfi = i;
    _BCM_VIRTUAL_VFI_USED_SET(unit, i);
    switch (type) {
    case _bcmVfiTypeMpls:
        _BCM_MPLS_VFI_USED_SET(unit, i);
        break;
    case _bcmVfiTypeMim:
        _BCM_MIM_VFI_USED_SET(unit, i);
        break;
    case _bcmVfiTypeL2Gre:
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_l2gre)) {
            _BCM_L2GRE_VFI_USED_SET(unit, i);
        }
#endif
        break;
    case _bcmVfiTypeVxlan:
#if defined(BCM_TRIDENT2_SUPPORT)
        if (soc_feature(unit, soc_feature_vxlan)) {
           _BCM_VXLAN_VFI_USED_SET(unit, i);
        }
#endif
        break;
    default:
        break;
    }
    VIRTUAL_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_vfi_alloc_with_id
 * Purpose:
 *      Internal function for allocating a VFI with a given ID
 * Parameters:
 *      unit    -  (IN) Device number.
 *      vfi_mem -  (IN) HW specific VFI memory
 *      type    -  (IN) VFI type
 *      vfi     -  (IN) VFI index
 * Returns:
 *      BCM_X_XXX
 */
 
int
_bcm_vfi_alloc_with_id(int unit, soc_mem_t vfi_mem, _bcm_vfi_type_e type, int vfi)
{
    int num_vfi;
    num_vfi = soc_mem_index_count(unit, vfi_mem);

    /* Check Valid range of VFI */
    if ( vfi < 0 || vfi >= num_vfi ) {
         return BCM_E_RESOURCE;
    }
   
    VIRTUAL_LOCK(unit);
    if (_BCM_VIRTUAL_VFI_USED_GET(unit, vfi)) {
         return BCM_E_EXISTS;
   }

    _BCM_VIRTUAL_VFI_USED_SET(unit, vfi);

    switch (type) {
    case _bcmVfiTypeMpls:
        _BCM_MPLS_VFI_USED_SET(unit, vfi);
        break;
    case _bcmVfiTypeMim:
        _BCM_MIM_VFI_USED_SET(unit, vfi);
        break;
    case _bcmVfiTypeL2Gre:
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_l2gre)) {
            _BCM_L2GRE_VFI_USED_SET(unit, vfi);
        }
#endif
        break;
    case _bcmVfiTypeVxlan:
#if defined(BCM_TRIDENT2_SUPPORT)
        if (soc_feature(unit, soc_feature_vxlan)) {
            _BCM_VXLAN_VFI_USED_SET(unit, vfi);
        }
#endif
        break;
    default:
        break;
    }
    VIRTUAL_UNLOCK(unit);
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_vfi_free
 * Purpose:
 *      Internal function for freeing a VFI
 * Parameters:
 *      unit    -  (IN) Device number.
 *      type    -  (IN) VFI type
 *      base_vfi - (IN) VFI index
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_vfi_free(int unit, _bcm_vfi_type_e type, int vfi)
{
    VIRTUAL_LOCK(unit);
    _BCM_VIRTUAL_VFI_USED_CLR(unit, vfi);
    switch (type) {
    case _bcmVfiTypeMpls:
        _BCM_MPLS_VFI_USED_CLR(unit, vfi);
        break;
    case _bcmVfiTypeMim:
        _BCM_MIM_VFI_USED_CLR(unit, vfi);
        break;
    case _bcmVfiTypeL2Gre:
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_l2gre)) {
            _BCM_L2GRE_VFI_USED_CLR(unit, vfi);
        }
#endif
        break;
    case _bcmVfiTypeVxlan:
#if defined(BCM_TRIDENT2_SUPPORT)
        if (soc_feature(unit, soc_feature_vxlan)) {
            _BCM_VXLAN_VFI_USED_CLR(unit, vfi);
        }
#endif
        break;
    default:
        break;
    }
    VIRTUAL_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_vfi_used_get
 * Purpose:
 *      Check whether a VFI is used or not 
 * Parameters:
 *      unit    -  (IN) Device number.
 *      vfi     -  (IN) VFI
 *      type    -  (IN) VFI type
 * Returns:
 *      Boolean
 */
int
_bcm_vfi_used_get(int unit, int vfi, _bcm_vfi_type_e type)
{
    int rv=0;
/* Removed Lock-Unlock - fn used in Interrupt context */
    switch (type) {
    case _bcmVfiTypeMpls:
        rv = _BCM_MPLS_VFI_USED_GET(unit, vfi);
        break;
    case _bcmVfiTypeMim:
        rv = _BCM_MIM_VFI_USED_GET(unit, vfi);
        break;
    case _bcmVfiTypeL2Gre:
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_l2gre)) {
            rv = _BCM_L2GRE_VFI_USED_GET(unit, vfi);
        }
#endif
        break;
    case _bcmVfiTypeVxlan:
#if defined(BCM_TRIDENT2_SUPPORT)
        if (soc_feature(unit, soc_feature_vxlan)) {
            rv = _BCM_VXLAN_VFI_USED_GET(unit, vfi);
        }
#endif
        break;
    default:
        rv = _BCM_VIRTUAL_VFI_USED_GET(unit, vfi);
        break;
    }
    return rv;
}

#ifdef BCM_TRIUMPH2_SUPPORT 
/*
 * Function:
 *      _bcm_vfi_flex_stat_index_set
 * Purpose:
 *      Associate a flexible stat with a VFI
 * Parameters:
 *      unit    -  (IN) Device number.
 *      vfi     -  (IN) VFI
 *      fs_idx  -  (IN) Flexible stat index
 * Returns:
 *      Boolean
 */
int
_bcm_vfi_flex_stat_index_set(int unit, int vfi, _bcm_vfi_type_e type,
                             int fs_idx,uint32 flags)
{
    int rv,rv1;
    vfi_entry_t vfi_entry;
    egr_vfi_entry_t egr_vfi_entry;

    rv = BCM_E_NONE;
    rv1 = BCM_E_NONE;
    VIRTUAL_LOCK(unit);
    if (_bcm_vfi_used_get(unit, vfi, type)) {
        if (flags & _BCM_FLEX_STAT_HW_INGRESS) {
            rv = soc_mem_read(unit, VFIm, MEM_BLOCK_ANY, vfi, &vfi_entry);
            if (BCM_SUCCESS(rv)) {
                if (soc_mem_field_valid(unit, VFIm, USE_SERVICE_CTR_IDXf)) {
                    soc_mem_field32_set(unit, VFIm, &vfi_entry,
                                    USE_SERVICE_CTR_IDXf, fs_idx > 0 ? 1 : 0);
                }
                soc_mem_field32_set(unit, VFIm, &vfi_entry, SERVICE_CTR_IDXf,
                                    fs_idx);
                rv = soc_mem_write(unit, VFIm, MEM_BLOCK_ANY, vfi, &vfi_entry);
            }
        }
        if (flags & _BCM_FLEX_STAT_HW_EGRESS) {
            rv1 = soc_mem_read(unit, EGR_VFIm, MEM_BLOCK_ANY, vfi,
                              &egr_vfi_entry);
            if (BCM_SUCCESS(rv1)) {
                if (soc_mem_field_valid(unit, EGR_VFIm,
                                        USE_SERVICE_CTR_IDXf)) {
                    soc_mem_field32_set(unit, EGR_VFIm, &egr_vfi_entry,
                                        USE_SERVICE_CTR_IDXf,
                                        fs_idx > 0 ? 1 : 0);
                }
                soc_mem_field32_set(unit, EGR_VFIm, &egr_vfi_entry,
                                    SERVICE_CTR_IDXf, fs_idx);
                rv1 = soc_mem_write(unit, EGR_VFIm, MEM_BLOCK_ANY, vfi,
                                   &egr_vfi_entry);
            }
        }
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    VIRTUAL_UNLOCK(unit);
    if (BCM_SUCCESS(rv1)) {
        return rv;
    } else {
        return rv1;
    }
}
#endif  /* BCM_TRIUMPH2_SUPPORT */

#else /* INCLUDE_L3 */
int _bcm_esw_trx_virtual_not_empty;
#endif  /* INCLUDE_L3 */
