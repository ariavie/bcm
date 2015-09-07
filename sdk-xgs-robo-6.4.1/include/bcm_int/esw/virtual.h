/*
 * $Id: virtual.h,v 1.18 Broadcom SDK $
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
 * File:        virtual.h
 * Purpose:     Function declarations for VP / VFI resource management
 */

#ifndef _BCM_INT_RESOURCE_H_
#define _BCM_INT_RESOURCE_H_

typedef struct _bcm_virtual_bookkeeping_s {
    SHR_BITDCL  *vfi_bitmap;         /* Global VFI bitmap */
    SHR_BITDCL  *vp_bitmap;          /* Global Virtual Port bitmap */
    SHR_BITDCL  *mpls_vfi_bitmap;    /* MPLS VFI bitmap */
    SHR_BITDCL  *mpls_vp_bitmap;     /* MPLS Virtual Port bitmap */
    SHR_BITDCL  *mim_vfi_bitmap;     /* MIM VFI bitmap */
    SHR_BITDCL  *mim_vp_bitmap;      /* MIM Virtual Port bitmap */
    SHR_BITDCL  *l2gre_vfi_bitmap;   /* L2GRE VFI bitmap */
    SHR_BITDCL  *l2gre_vp_bitmap;    /* L2GRE Virtual Port bitmap */
    SHR_BITDCL  *vxlan_vfi_bitmap;   /* VXLAN VFI bitmap */
    SHR_BITDCL  *vxlan_vp_bitmap;    /* VXLAN Virtual Port bitmap */
    SHR_BITDCL  *subport_vp_bitmap;  /* Subport Virtual Port bitmap */
    SHR_BITDCL  *wlan_vp_bitmap;     /* WLAN Virtual Port bitmap */
    SHR_BITDCL  *trill_vp_bitmap;    /* TRILL Virtual Port bitmap */
    SHR_BITDCL  *vlan_vp_bitmap;     /* VLAN Virtual Port bitmap */
    SHR_BITDCL  *niv_vp_bitmap;      /* NIV Virtual Port bitmap */
    SHR_BITDCL  *extender_vp_bitmap; /* Extender Virtual Port bitmap */
    SHR_BITDCL  *vp_lag_vp_bitmap;   /* VP LAG Virtual Port bitmap */
} _bcm_virtual_bookkeeping_t;

extern _bcm_virtual_bookkeeping_t  _bcm_virtual_bk_info[BCM_MAX_NUM_UNITS];

#if defined(BCM_TRX_SUPPORT)
#include <bcm_int/esw/mbcm.h>

/****************************************************************
 *
 * Virtual resource management functions
 *
 ****************************************************************/
#if defined(INCLUDE_L3)
typedef enum _bcm_vp_type_s {
    _bcmVpTypeMpls,      /* MPLS VP */
    _bcmVpTypeMim,       /* MIM VP */
    _bcmVpTypeSubport,   /* Subport VP */
    _bcmVpTypeWlan,      /* WLAN VP */
    _bcmVpTypeTrill,     /* TRILL VP */
    _bcmVpTypeVlan,      /* VLAN VP */
    _bcmVpTypeNiv,       /* NIV VP */
    _bcmVpTypeL2Gre,     /* L2GRE VP */
    _bcmVpTypeVxlan,     /* VXLAN VP */
    _bcmVpTypeExtender,  /* Extender VP */
    _bcmVpTypeVpLag,     /* VP LAG VP */
    _bcmVpTypeAny        /* Any VP */
} _bcm_vp_type_e;

typedef enum _bcm_vfi_type_s {
    _bcmVfiTypeMpls,     /* MPLS VFI */
    _bcmVfiTypeMim,      /* MIM VFI */
    _bcmVfiTypeL2Gre,   /* L2GRE VFI */
    _bcmVfiTypeVxlan,   /* VXLAN VFI */
    _bcmVfiTypeAny       /* Any VFI */
} _bcm_vfi_type_e;

typedef enum _bcm_vp_ing_dvp_config_op_e {
    _bcmVpIngDvpConfigClear,  /* Clear ING_DVP_* tables                    */
    _bcmVpIngDvpConfigSet,    /* Set input data in ING_DVP_* tables        */
    _bcmVpIngDvpConfigUpdate  /* Update the input data in ING_DVP_* tables */
} _bcm_vp_ing_dvp_config_op_t;

extern int _bcm_virtual_init(int unit, soc_mem_t vp_mem, soc_mem_t vfi_mem);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_virtual_sync(int unit);
#endif
extern int _bcm_vp_alloc(int unit, int start, int end, int count, soc_mem_t vp_mem, 
                         _bcm_vp_type_e type, int *base_vp);
extern int _bcm_vp_free(int unit, _bcm_vp_type_e type, int count, int base_vp);
extern int _bcm_vp_used_set(int unit, int vp, _bcm_vp_type_e type);
extern int _bcm_vp_used_get(int unit, int vp, _bcm_vp_type_e type);
extern int _bcm_vp_default_cml_mode_get (int unit, int *cml_default_enable, int *cml_new, int *cml_move);
extern int _bcm_vfi_alloc(int unit, soc_mem_t vp_mem, _bcm_vfi_type_e type, int *vfi);
extern int _bcm_vfi_alloc_with_id(int unit, soc_mem_t vfi_mem, _bcm_vfi_type_e type, int vfi);
extern int _bcm_vfi_free(int unit, _bcm_vfi_type_e type, int vfi);
extern int _bcm_vfi_used_get(int unit, int vfi, _bcm_vfi_type_e type);
extern int _bcm_vfi_flex_stat_index_set(int unit, int vfi,
                               _bcm_vfi_type_e type, int fs_idx,uint32 flags);
extern int _bcm_vp_ing_dvp_config(int unit, _bcm_vp_ing_dvp_config_op_t op,
                                  int vp, int nh_index);

#endif /* INCLUDE_L3 */

#endif /* BCM_TRX_SUPPORT */
#endif  /* !_BCM_INT_RESOURCE_H_ */


