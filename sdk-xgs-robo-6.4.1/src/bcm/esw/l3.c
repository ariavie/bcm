/*
 * $Id: l3.c,v 1.331 Broadcom SDK $
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
 * File:    l3.c
 * Purpose: Manages L3 interface table, forwarding table, routing table
 */

#include <shared/bsl.h>

#include <soc/defs.h>
#include <bcm/error.h>
#include <shared/bsl.h>
#ifdef INCLUDE_L3
#include <sal/core/libc.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>
#include <soc/l3x.h>
#include <soc/hash.h>


#include <bcm/l3.h>
#include <bcm/debug.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/virtual.h>

#ifdef  BCM_XGS_SWITCH_SUPPORT
#include <bcm_int/esw/firebolt.h>
#if defined(BCM_TRIUMPH_SUPPORT)
#include <bcm_int/esw/triumph.h>
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#endif  /* BCM_XGS_SWITCH_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm_int/esw/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/flex_ctr.h>

/*
 * Special IP addresses
 */
#define INADDR_ANY        (uint32)0x00000000
#define INADDR_LOOPBACK   (uint32)0x7F000001
#define INADDR_NONE       (uint32)0xffffffff

#ifdef BCM_KATANA_SUPPORT
#define _BCM_QOS_MAP_TYPE_MASK                     0x3ff
#define _BCM_QOS_MAP_SHIFT                            10
#define _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE   5
#define _BCM_QOS_MAP_ING_QUEUE_OFFSET_MAX              7
#endif
/*
 * L3 book keeping global info
 */
_bcm_l3_bookkeeping_t   _bcm_l3_bk_info[BCM_MAX_NUM_UNITS];
static int              l3_internal_initialized = 0;

#define L3_INIT(unit) \
    if (!soc_feature(unit, soc_feature_l3)) { \
        return BCM_E_UNAVAIL; \
    } else if (!_bcm_l3_bk_info[unit].l3_initialized) { return BCM_E_INIT; }

int
_bcm_l3_reinit(int unit)
{
    _bcm_l3_bookkeeping_t *l3;
  
    l3 = &_bcm_l3_bk_info[unit];
    sal_memset(l3, 0, sizeof(_bcm_l3_bookkeeping_t));
   
    if (soc_property_get(unit, spn_L3_DISABLE_ADD_TO_ARL, 0)) {
        BCM_L3_BK_FLAG_SET(unit, BCM_L3_BK_DISABLE_ADD_TO_ARL);
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if ((SOC_IS_FB_FX_HX(unit) && !SOC_IS_RAPTOR(unit)) ||
        SOC_IS_TR_VL(unit) || SOC_IS_HB_GW(unit) || 
        SOC_IS_SC_CQ(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_xgs3_l3_reinit(unit));
    }
#endif

    l3->l3_initialized = 1;
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_esw_l3_gport_construct
 * Purpose:
 *      Constructs a gport out of given parameter for L3 Module.
 * Parameters:
 *      unit - SOC device unit number
 *      port_tgid - Physical port 
 *      modid - Module ID
 *      tgid  - Trunk group ID
 *      flags  - L3 Flags
 *      gport  - (Out) gport constructed from given arguments
 * Returns:
 *      BCM_E_XXX              
 */

int
_bcm_esw_l3_gport_construct(int unit, bcm_port_t port, bcm_module_t modid,
                            bcm_trunk_t tgid, uint32 flags, bcm_port_t *gport) 
{
    int                 isGport, rv;
    _bcm_gport_dest_t   dest;
    bcm_module_t        mymodid;
#if defined(BCM_KATANA2_SUPPORT)
    bcm_port_t          pp_port;
    int                 is_local_subport = 0;
#endif

    if (NULL == gport) {
        return BCM_E_PARAM;
    }

    if (flags & BCM_L3_TGID) {
        if (BCM_FAILURE(_bcm_trunk_id_validate(unit, tgid))) {
            return BCM_E_PARAM;
        }
    } else if ((port < 0) && (modid < 0)){
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));

    /* if output should not be gport then do nothing*/
    if (!isGport) {
        return BCM_E_NONE;
    }

    if (BCM_GPORT_IS_WLAN_PORT(port)) {
        return BCM_E_NONE;
    } 


#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        rv = bcm_esw_stk_my_modid_get(unit, &mymodid);
        /* For LinkPHY/SubportPktTag pp_ports mymodid+1 is used */
        if ((modid == mymodid + 1) && !(flags & BCM_L3_TGID)) {
            BCM_IF_ERROR_RETURN(_bcm_kt2_modport_to_pp_port_get(unit,
                modid, port, &pp_port));
            /*For KT2, pp_port >=42 are used for LinkPHY/SubportPktTag subport.
            * Get the subport info associated with the pp_port and form the
            * subport_gport */
            if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                is_local_subport = 1;
            }
        }
    }
    if (is_local_subport) {
        /* construct LinkPHY/SubportPkttag subport gport */
        _BCM_KT2_SUBPORT_PORT_ID_SET(*gport, pp_port);
        if (BCM_PBMP_MEMBER(
            SOC_INFO(unit).linkphy_pp_port_pbm, pp_port)) {
            _BCM_KT2_SUBPORT_PORT_TYPE_SET(*gport,
                _BCM_KT2_SUBPORT_TYPE_LINKPHY);
        } else if (BCM_PBMP_MEMBER(
            SOC_INFO(unit).subtag_pp_port_pbm, pp_port)) {
            _BCM_KT2_SUBPORT_PORT_TYPE_SET(*gport,
                _BCM_KT2_SUBPORT_TYPE_SUBTAG);
        } else {
            return BCM_E_PORT;
        }
    } else
#endif
    {
        _bcm_gport_dest_t_init(&dest);

        if (flags & BCM_L3_TGID) {
            dest.tgid = tgid;
            dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
        } else {
            /* Stacking ports should be encoded as devport */
            if (IS_ST_PORT(unit, port)) {
                rv = bcm_esw_stk_my_modid_get(unit, &mymodid);
                if (BCM_E_UNAVAIL == rv) {
                    dest.gport_type = _SHR_GPORT_TYPE_DEVPORT;
                } else {
                    dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                    dest.modid = modid;
                }
            } else {
                dest.modid = modid;
                dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
            }
            dest.port = port;
        }
        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_construct(unit, &dest, gport));
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_esw_l3_gport_resolve
 * Purpose:
 *      Resolves a gport for L3 Module.
 * Parameters:
 *      unit - SOC device unit number
 *      gport - gport provided 
 *      port  - (Out) Physical port decoded from gport
 *      modid - (Out) Module ID decodec from gport 
 *      tgid  - (Out) Trunk group ID decoded from gport
 *      gport  - (Out) gport constructed from given arguments
 * Returns:
 *      BCM_E_XXX              
 */
int
_bcm_esw_l3_gport_resolve(int unit, bcm_gport_t gport, bcm_port_t *port, 
                          bcm_module_t *modid, bcm_trunk_t *tgid, uint32 *flags)
{
    int id;
    bcm_trunk_t     local_tgid;
    bcm_module_t    local_modid;
    bcm_port_t      local_port;

    if ((NULL == port) || (NULL == modid) || (NULL == tgid) || 
        (NULL == flags)) {
        return BCM_E_PARAM;
    }
   
    if (BCM_GPORT_IS_BLACK_HOLE(gport)) {
         *port = gport;
         return BCM_E_NONE;
    } else {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_resolve(unit, gport,
            &local_modid, &local_port, &local_tgid, &id));
    }

#if defined(BCM_KATANA2_SUPPORT)
    if (BCM_GPORT_IS_SET(gport) &&
        _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT (unit, gport)) {
    } else
#endif
    if (-1 != id) {
        return BCM_E_PARAM;
    }
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit) &&
       ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) ||
        (BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(gport)))) {
        *modid = local_modid;
        *port = local_port;
        *tgid = local_tgid; /* overlaying extended queue id in tgid */
    } else
#endif 
    if (BCM_TRUNK_INVALID != local_tgid) {
        *flags |= BCM_L3_TGID;
        *tgid = local_tgid;
    } else {
        *modid = local_modid;
        *port = local_port;
    }

    return BCM_E_NONE;
}

#if defined(BCM_XGS3_SWITCH_SUPPORT)
#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_1                SOC_SCACHE_VERSION(1,1)
#define BCM_WB_VERSION_1_2                SOC_SCACHE_VERSION(1,2)
#define BCM_WB_VERSION_1_3                SOC_SCACHE_VERSION(1,3)
#define BCM_WB_VERSION_1_4                SOC_SCACHE_VERSION(1,4)
#define BCM_WB_VERSION_1_5                SOC_SCACHE_VERSION(1,5)
#define BCM_WB_VERSION_1_6                SOC_SCACHE_VERSION(1,6)
#define BCM_WB_VERSION_1_7                SOC_SCACHE_VERSION(1,7)
#define BCM_WB_VERSION_1_8                SOC_SCACHE_VERSION(1,8)
#define BCM_WB_VERSION_1_9                SOC_SCACHE_VERSION(1,9)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_9

/*
 * Function:
 *      _bcm_esw_l3_sync
 * Purpose:
 *      Record L3 module persisitent info for Level 2 Warm Boot
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_esw_l3_sync(int unit)
{
    soc_scache_handle_t scache_handle;
    uint8 *l3_scache_ptr;
    int alloc_sz = 2 * sizeof(int32); /* Egress, HostAdd mode */
    int32 l3EgressMode = 0;
    int32 l3HostAddMode = 0;
#ifdef BCM_TRIUMPH_SUPPORT
    int32 l3IntfMapMode = 0;
    int32 l3IngressMode = 0;
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    int idx;
#endif

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
        alloc_sz  += sizeof(int32); /* Ingress mode */
        alloc_sz  += sizeof(int32); /* Interface Map mode */
    }
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
    /* For extended scache warmboot mode (level-2) only */
    if (soc_feature(unit, soc_feature_l3_dynamic_ecmp_group) &&
        BCM_XGS3_L3_MAX_ECMP_MODE(unit)                      &&
        !SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
        alloc_sz += sizeof(uint16) *
                    BCM_XGS3_L3_ECMP_MAX_GROUPS(unit);
    }
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_ecmp_dlb)) {
        int ecmp_dlb_alloc_sz;

        BCM_IF_ERROR_RETURN
            (bcm_tr3_ecmp_dlb_wb_alloc_size_get(unit, &ecmp_dlb_alloc_sz));
        alloc_sz += ecmp_dlb_alloc_sz;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf) ||
                soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTR_0f)) {
            int wb_alloc_sz;

            BCM_IF_ERROR_RETURN
                (bcm_tr2_l3_ecmp_defragment_buffer_wb_alloc_size_get(unit,
                                                                     &wb_alloc_sz));
            alloc_sz += wb_alloc_sz;
        }
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    if (BCM_WB_DEFAULT_VERSION >= BCM_WB_VERSION_1_6) {
        if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
            int wb_alloc_sz;
            BCM_IF_ERROR_RETURN
                (_bcm_td2_l3_ip4_options_profile_scache_len_get(unit, &wb_alloc_sz));
                alloc_sz += wb_alloc_sz;
        }
    }

    if (soc_feature(unit, soc_feature_l3_ingress_interface)) { 
        alloc_sz += SHR_BITALLOCSIZE(BCM_XGS3_L3_ING_IF_TBL_SIZE(unit));
    }
#endif

    /* ECMP max paths info */
    alloc_sz += sizeof(int);

    /* Determine ECMP_GROUP entry number in 32-bits words */
    if (soc_feature(unit, soc_feature_l3)) {
        alloc_sz += SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS);
    }

#ifdef BCM_TRIDENT2_SUPPORT
    /* Determine ECMP_GROUP resorting flags*/
    if (soc_feature(unit, soc_feature_l3)) {
        alloc_sz += SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS);
    }
 #endif
 
    L3_INIT(unit);

    SOC_SCACHE_HANDLE_SET(scache_handle,  unit, BCM_MODULE_L3, 0);
    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 alloc_sz, &l3_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL));

    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_egress_mode_get(unit, &l3EgressMode));
    sal_memcpy(l3_scache_ptr, &l3EgressMode, sizeof(l3EgressMode));
    l3_scache_ptr += sizeof(l3EgressMode);

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
        BCM_IF_ERROR_RETURN(bcm_xgs3_l3_ingress_mode_get(unit, &l3IngressMode));
        sal_memcpy(l3_scache_ptr, &l3IngressMode, sizeof(l3IngressMode));
        l3_scache_ptr += sizeof(l3IngressMode);
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_host_as_route_return_get(unit, &l3HostAddMode));
    sal_memcpy(l3_scache_ptr, &l3HostAddMode, sizeof(l3HostAddMode));
    l3_scache_ptr += sizeof(l3HostAddMode);

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
       BCM_IF_ERROR_RETURN(bcm_xgs3_l3_ingress_intf_map_get(unit, &l3IntfMapMode));
       sal_memcpy(l3_scache_ptr, &l3IntfMapMode,  sizeof(l3IntfMapMode));
       l3_scache_ptr += sizeof(l3IntfMapMode);
    }
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
    /* For extended scache warmboot mode (level-2) only */
    if (soc_feature(unit, soc_feature_l3_dynamic_ecmp_group) &&
        BCM_XGS3_L3_MAX_ECMP_MODE(unit)                      &&
        !SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
        for(idx=0; idx<BCM_XGS3_L3_ECMP_MAX_GROUPS(unit); idx++) {
            sal_memcpy(l3_scache_ptr,
                       &(BCM_XGS3_L3_MAX_PATHS_PERGROUP_PTR(unit)[idx]),
                       sizeof(uint16));
            l3_scache_ptr += sizeof(uint16);
        }
    }
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_ecmp_dlb)) {
        BCM_IF_ERROR_RETURN(bcm_tr3_ecmp_dlb_sync(unit, &l3_scache_ptr));
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf) ||
            soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTR_0f)) {
        if (!soc_feature(unit, soc_feature_l3_no_ecmp)) {
            BCM_IF_ERROR_RETURN
                (bcm_tr2_l3_ecmp_defragment_buffer_sync(unit, &l3_scache_ptr));
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_ip4_options_sync(unit, &l3_scache_ptr));
    }

#endif /* BCM_TRIDENT2_SUPPORT */

    sal_memcpy(l3_scache_ptr, &BCM_XGS3_L3_ECMP_MAX_PATHS(unit), sizeof(int));
    l3_scache_ptr += sizeof(int);

    /* Store ECMP_GRP valid/invalid bit to Scache */
    if (soc_feature(unit, soc_feature_l3) && !soc_feature(unit, soc_feature_l3_no_ecmp)) {
        int i;
        _bcm_l3_tbl_t *ecmp_tbl_ptr = BCM_XGS3_L3_TBL_PTR(unit, ecmp_grp);
        SHR_BITDCL egbitmap[_SHR_BITDCLSIZE(BCM_L3_MAX_ECMP_GROUPS)] = {0};

        for (i = 0; i <= ecmp_tbl_ptr->idx_max; i++) {
            if (BCM_XGS3_L3_ENT_REF_CNT(ecmp_tbl_ptr, i) > 0) {
                SHR_BITSET(egbitmap, i);
            }
        }

        sal_memcpy(l3_scache_ptr, egbitmap, SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS));  
        l3_scache_ptr += SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS);

    }
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
        sal_memcpy(l3_scache_ptr, BCM_XGS3_L3_ING_IF_INUSE(unit), 
            SHR_BITALLOCSIZE(BCM_XGS3_L3_ING_IF_TBL_SIZE(unit)));
        l3_scache_ptr += SHR_BITALLOCSIZE(BCM_XGS3_L3_ING_IF_TBL_SIZE(unit));
    }
#endif
    
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_l3) && !soc_feature(unit, soc_feature_l3_no_ecmp)) {
        int i; 
        _bcm_l3_tbl_t *ecmp_tbl_ptr = BCM_XGS3_L3_TBL_PTR(unit, ecmp_grp);
        SHR_BITDCL egflgbitmap[_SHR_BITDCLSIZE(BCM_L3_MAX_ECMP_GROUPS)] = {0};

        for (i = 0; i <= ecmp_tbl_ptr->idx_max; i++) {
            if (BCM_XGS3_L3_ENT_REF_CNT(ecmp_tbl_ptr, i) > 0 &&
                 (BCM_XGS3_L3_ECMP_GROUP_FLAGS(unit, i) > 0)) {
                SHR_BITSET(egflgbitmap, i);
            }
        }
        
        sal_memcpy(l3_scache_ptr, egflgbitmap, SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS));  
        l3_scache_ptr += SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS);
    }
#endif

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_l3_warm_boot_alloc
 * Purpose:
 *      Allocate persistent info memory for L3 module - Level 2 Warm Boot
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_l3_warm_boot_alloc(int unit)
{
    int rv;
    int alloc_sz = 2 * sizeof(int32); /* Egress, Host modes */
    uint8 *l3_scache_ptr;
    soc_scache_handle_t scache_handle;

#ifdef BCM_TRIUMPH2_SUPPORT
    BCM_XGS3_L3_MAX_ECMP_MODE(unit) = (SOC_IS_TRIUMPH3(unit)) ? TRUE :
                           soc_property_get(unit, spn_L3_MAX_ECMP_MODE, 0);
#endif

    if (BCM_WB_DEFAULT_VERSION >= BCM_WB_VERSION_1_1) {
#ifdef BCM_TRIUMPH_SUPPORT 
        if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
            alloc_sz += sizeof(int32); /* Ingress mode */
        }
#endif /* BCM_TRIUMPH_SUPPORT */
    }
    
    if (BCM_WB_DEFAULT_VERSION >= BCM_WB_VERSION_1_2) {
#ifdef BCM_TRIUMPH_SUPPORT 
        if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
            alloc_sz += sizeof(int32); /* intfMap mode */
        }
#endif /* BCM_TRIUMPH_SUPPORT */
    }
    
    if (BCM_WB_DEFAULT_VERSION >= BCM_WB_VERSION_1_3) {
#ifdef BCM_TRIUMPH2_SUPPORT
        /* For extended scache warmboot mode (level-2) only */
        if (soc_feature(unit, soc_feature_l3_dynamic_ecmp_group) &&
            BCM_XGS3_L3_MAX_ECMP_MODE(unit)                      &&
            !SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
            alloc_sz += sizeof(uint16) *
                BCM_XGS3_L3_ECMP_MAX_GROUPS(unit);
        }
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_dlb)) {
            int ecmp_dlb_alloc_sz;

            BCM_IF_ERROR_RETURN
                (bcm_tr3_ecmp_dlb_wb_alloc_size_get(unit, &ecmp_dlb_alloc_sz));
            alloc_sz += ecmp_dlb_alloc_sz;
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf) ||
                soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTR_0f)) {
            int wb_alloc_sz;

            BCM_IF_ERROR_RETURN
                (bcm_tr2_l3_ecmp_defragment_buffer_wb_alloc_size_get(unit,
                                                                     &wb_alloc_sz));
            alloc_sz += wb_alloc_sz;
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    }
    if (BCM_WB_DEFAULT_VERSION >= BCM_WB_VERSION_1_6) {
#ifdef BCM_TRIDENT2_SUPPORT 
        if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
            int wb_alloc_sz;
            BCM_IF_ERROR_RETURN
                (_bcm_td2_l3_ip4_options_profile_scache_len_get(unit, &wb_alloc_sz));
                alloc_sz += wb_alloc_sz;
        }
#endif /* BCM_TRIUMPH_SUPPORT */
    }

#ifdef BCM_TRIDENT2_SUPPORT 
    if (soc_feature(unit, soc_feature_l3_ingress_interface)){
        alloc_sz +=
            SHR_BITALLOCSIZE(soc_mem_index_count(unit, L3_IIFm));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* to save max ecmp paths info */
    alloc_sz += sizeof(int);

    /* each bit save per L3_ECMP_GROUP entry valid/invalid */
    if (soc_feature(unit, soc_feature_l3)) {
        alloc_sz += SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS);
    }
 
#ifdef BCM_TRIDENT2_SUPPORT   
    /* ECMP group resorting flag */
    if (soc_feature(unit, soc_feature_l3)) {
        alloc_sz += SHR_BITALLOCSIZE(BCM_L3_MAX_ECMP_GROUPS);
    }
#endif

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_L3, 0);

    /* This function is only used for Cold Boot */
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
                                 alloc_sz, &l3_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL);

    if (BCM_E_NOT_FOUND == rv) {
        /* Proceed with Level 1 Warm Boot */
        rv = BCM_E_NONE;
    }

    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */
#endif /* defined(BCM_XGS3_SWITCH_SUPPORT) */

/*
 * Function:
 *      bcm_l3_init
 * Purpose:
 *      Initialize the L3 table, default IP table and the L3 interface table.
 * Parameters:
 *      unit - SOC device unit number
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_MEMORY            Cannot allocate memory
 *      BCM_E_INTERNAL          Chip access failure
 * Notes:
 *      This function has to be called before any other L3 functions.
 */

int
bcm_esw_l3_init(int unit)
{
    _bcm_l3_bookkeeping_t *l3;

    if (!soc_feature(unit, soc_feature_l3)) {
        return BCM_E_UNAVAIL;
    }

    if (!l3_internal_initialized) {
        sal_memset(_bcm_l3_bk_info, 0,
                   BCM_MAX_NUM_UNITS * sizeof(_bcm_l3_bookkeeping_t));
        l3_internal_initialized = 1;
    }

    l3 = &_bcm_l3_bk_info[unit];

    if (soc_property_get(unit, spn_L3_DISABLE_ADD_TO_ARL, 0)) {
        BCM_L3_BK_FLAG_SET(unit, BCM_L3_BK_DISABLE_ADD_TO_ARL);
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        return (_bcm_l3_reinit(unit));
    }
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    else {
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_warm_boot_alloc(unit));
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
#endif /* BCM_WARM_BOOT_SUPPORT */

   BCM_IF_ERROR_RETURN(mbcm_driver[unit]->mbcm_l3_tables_init(unit));

   if (!BCM_XGS3_L3_NH_CNT(unit)) {
       BCM_XGS3_L3_NH_CNT(unit) = 1;
   }

   l3->l3_initialized = 1;

   return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_esw_l3_intf_create
 * Purpose:
 *     Create an L3 interface
 * Parameters:
 *     unit  - StrataXGS unit number
 *     intf  - interface info: l3a_mac_addr - MAC address;
 *             l3a_vid - VLAN ID;
 *             flag BCM_L3_ADD_TO_ARL: add mac address to arl table
 *                                     with static bit and L3 bit set.
 *             flag BCM_L3_WITH_ID: use specified interface ID l3a_intf.
 *             flag BCM_L3_REPLACE: overwrite if interface already exists.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The VLAN table should be set up properly before this call.
 *      The L3 interface ID is automatically assigned unless a specific
 *      ID is requested with the BCM_L3_WITH_ID flag.
 */

int
_bcm_esw_l3_intf_create(int unit, bcm_l3_intf_t *intf)
{
    _bcm_l3_intf_cfg_t l3i;
    bcm_l3_intf_t find_if;
    int rv;

    /* Input parameters check. */
    if (NULL == intf) {
        return (BCM_E_PARAM);
    }

    if (((intf->l3a_vrf > SOC_VRF_MAX(unit)) ||
        (intf->l3a_vrf < BCM_L3_VRF_DEFAULT)) &&
         !soc_feature(unit, soc_feature_fp_based_routing)) {
        return (BCM_E_PARAM);
    }

    if (BCM_MAC_IS_ZERO(intf->l3a_mac_addr)) {
        return (BCM_E_PARAM);
    }

    if (!BCM_VLAN_VALID(intf->l3a_vid)) {
        return (BCM_E_PARAM);
    }

    if (!BCM_TTL_VALID(intf->l3a_ttl)) {
        return (BCM_E_PARAM);
    }

    if (intf->l3a_group) {
        return (BCM_E_PARAM);
    }

    if ((intf->l3a_group > SOC_INTF_CLASS_MAX(unit)) ||
        (intf->l3a_group < 0)) {
        return (BCM_E_PARAM);
    }

    if ((0 == SOC_IS_TRX(unit)) && (intf->l3a_inner_vlan)) {
        return (BCM_E_PARAM);
    }

    sal_memset(&l3i, 0, sizeof(_bcm_l3_intf_cfg_t));
    sal_memcpy(l3i.l3i_mac_addr, intf->l3a_mac_addr, sizeof(bcm_mac_t));
    l3i.l3i_vid = intf->l3a_vid;
    l3i.l3i_inner_vlan = intf->l3a_inner_vlan;
    l3i.l3i_flags = intf->l3a_flags;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    l3i.l3i_vrf = intf->l3a_vrf;
    l3i.l3i_group = intf->l3a_group;
    l3i.l3i_ttl = intf->l3a_ttl;
    l3i.l3i_mtu = intf->l3a_mtu;
    l3i.l3i_tunnel_idx = intf->l3a_tunnel_idx;
    sal_memcpy(&l3i.vlan_qos, &intf->vlan_qos, sizeof(bcm_l3_intf_qos_t));
    sal_memcpy(&l3i.inner_vlan_qos, &intf->inner_vlan_qos, sizeof(bcm_l3_intf_qos_t));
    sal_memcpy(&l3i.dscp_qos, &intf->dscp_qos, sizeof(bcm_l3_intf_qos_t));
#if defined(BCM_TRIUMPH3_SUPPORT)
    l3i.l3i_class = intf->l3a_intf_class;
#endif /* BCM_TRIUMPH3_SUPPORT */
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    l3i.l3i_ip4_options_profile_id = intf->l3a_ip4_options_profile_id;
    l3i.l3i_nat_realm_id = intf->l3a_nat_realm_id;
    l3i.l3i_intf_flags = intf->l3a_intf_flags;
#endif /* BCM_TRIDENT2_SUPPORT */

    /* L3 interface ID given */
    if (intf->l3a_flags & BCM_L3_WITH_ID) {
        /* Initilize interface lookup key. */
        bcm_l3_intf_t_init(&find_if);

        /* Set interface index. */
        find_if.l3a_intf_id = intf->l3a_intf_id;

        /* Read interface info by index. */
        rv = bcm_esw_l3_intf_get(unit, &find_if);

        if (rv == BCM_E_NONE) {
            /* BCM_L3_REPLACE flag must be set for interface update. */
            if (!(intf->l3a_flags & BCM_L3_REPLACE)) {
               return BCM_E_EXISTS;
            }

            /* Remove old interface layer 2 address if required. */
            if (BCM_L3_INTF_ARL_GET(unit, intf->l3a_intf_id)) {
                BCM_IF_ERROR_RETURN 
                    (bcm_esw_l2_addr_delete(unit, find_if.l3a_mac_addr,
                                         find_if.l3a_vid));
                BCM_L3_INTF_ARL_CLR(unit, find_if.l3a_intf_id);
            }
        } else if (rv != BCM_E_NOT_FOUND) { /* Other error */
            return rv;
        }
        /* Set interface index. */
        l3i.l3i_index = intf->l3a_intf_id;
        rv = mbcm_driver[unit]->mbcm_l3_intf_id_create(unit, &l3i);
    } else {
        rv = mbcm_driver[unit]->mbcm_l3_intf_create(unit, &l3i);
    }

    if (rv < 0) {
        return (rv);
    }

    if (!(intf->l3a_flags & BCM_L3_WITH_ID)) {
        intf->l3a_intf_id = l3i.l3i_index;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_esw_l3_intf_create
 * Purpose:
 *     Create an L3 interface
 * Parameters:
 *     unit  - StrataXGS unit number
 *     intf  - interface info: l3a_mac_addr - MAC address;
 *             l3a_vid - VLAN ID;
 *             flag BCM_L3_ADD_TO_ARL: add mac address to arl table
 *                                     with static bit and L3 bit set.
 *             flag BCM_L3_WITH_ID: use specified interface ID l3a_intf.
 *             flag BCM_L3_REPLACE: overwrite if interface already exists.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The VLAN table should be set up properly before this call.
 *      The L3 interface ID is automatically assigned unless a specific
 *      ID is requested with the BCM_L3_WITH_ID flag.
 */
int
bcm_esw_l3_intf_create(int unit, bcm_l3_intf_t *intf)
{
    int rv;

    L3_INIT(unit);
    L3_LOCK(unit);

    rv = _bcm_esw_l3_intf_create(unit, intf);

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *     bcm_esw_l3_intf_delete
 * Purpose:
 *     Delete L3 interface based on L3 interface ID
 * Parameters:
 *     unit  - StrataXGS unit number
 *     intf  - interface structure with L3 interface ID as input
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_esw_l3_intf_delete(int unit, bcm_l3_intf_t *intf)
{
    _bcm_l3_intf_cfg_t l3i;
    int rv;

    L3_INIT(unit);

    if (NULL == intf) {
        return (BCM_E_PARAM);
    } 

    sal_memset(&l3i, 0, sizeof(_bcm_l3_intf_cfg_t));
    l3i.l3i_index = intf->l3a_intf_id;

    L3_LOCK(unit);
    rv = mbcm_driver[unit]->mbcm_l3_intf_delete(unit, &l3i);
    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *     bcm_esw_l3_intf_find
 * Purpose:
 *     Find the L3 intf number based on (MAC, VLAN)
 * Parameters:
 *     unit  - StrataXGS unit number
 *     intf  - (IN) interface (MAC, VLAN), (OUT)intf number
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_esw_l3_intf_find(int unit, bcm_l3_intf_t *intf)
{
    _bcm_l3_intf_cfg_t l3i;
    int rv;

    L3_INIT(unit);

    if (NULL == intf) {
        return (BCM_E_PARAM);
    } 

    if ((BCM_MAC_IS_MCAST(intf->l3a_mac_addr)) || 
        (BCM_MAC_IS_ZERO(intf->l3a_mac_addr))) {
        return (BCM_E_PARAM);
    }

    if (!BCM_VLAN_VALID(intf->l3a_vid)) {
        return (BCM_E_PARAM);
    }

    sal_memset(&l3i, 0, sizeof(_bcm_l3_intf_cfg_t));
    sal_memcpy(l3i.l3i_mac_addr, intf->l3a_mac_addr, sizeof(bcm_mac_t));
    l3i.l3i_vid = intf->l3a_vid;

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_intf_lookup(unit, &l3i);

    L3_UNLOCK(unit);

    intf->l3a_intf_id = l3i.l3i_index;

    return rv;
}

/*
 * Function:
 *     bcm_esw_l3_intf_get
 * Purpose:
 *     Given the L3 interface number, return the MAC and VLAN
 * Parameters:
 *     unit  - StrataXGS unit number
 *     intf  - (IN) L3 interface; (OUT)VLAN ID, 802.3 MAC for this L3 intf
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_esw_l3_intf_get(int unit, bcm_l3_intf_t *intf)
{
    _bcm_l3_intf_cfg_t l3i;
    int rv;

    L3_INIT(unit);

    if (NULL == intf) {
        return (BCM_E_PARAM);
    } 

    sal_memset(&l3i, 0, sizeof(_bcm_l3_intf_cfg_t));
    l3i.l3i_index = intf->l3a_intf_id;

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_intf_get(unit, &l3i);

    L3_UNLOCK(unit);
    if (rv < 0) {
        return (rv);
    }

    sal_memcpy(intf->l3a_mac_addr, l3i.l3i_mac_addr, sizeof(bcm_mac_t));
    intf->l3a_vid = l3i.l3i_vid;
    intf->l3a_inner_vlan = l3i.l3i_inner_vlan;
    intf->l3a_tunnel_idx = l3i.l3i_tunnel_idx;
    intf->l3a_flags = l3i.l3i_flags;
    intf->l3a_vrf = l3i.l3i_vrf;
    intf->l3a_ttl = l3i.l3i_ttl;
    intf->l3a_mtu = l3i.l3i_mtu;
    intf->l3a_group = l3i.l3i_group;
    sal_memcpy(&intf->vlan_qos, &l3i.vlan_qos, sizeof(bcm_l3_intf_qos_t));
    sal_memcpy(&intf->inner_vlan_qos, &l3i.inner_vlan_qos, sizeof(bcm_l3_intf_qos_t));
    sal_memcpy(&intf->dscp_qos, &l3i.dscp_qos, sizeof(bcm_l3_intf_qos_t));
#ifdef BCM_TRIDENT2_SUPPORT
    intf->l3a_ip4_options_profile_id = l3i.l3i_ip4_options_profile_id;
    intf->l3a_nat_realm_id = l3i.l3i_nat_realm_id;
    intf->l3a_intf_flags = l3i.l3i_intf_flags;
#endif /* BCM_TRIDENT2_SUPPORT */


    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_esw_l3_intf_find_vlan
 * Purpose:
 *     Find the L3 interface by VLAN
 * Parameters:
 *     unit - StrataXGS unit number
 *     intf  - (IN) VID, (OUT)L3 intf info
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_esw_l3_intf_find_vlan(int unit, bcm_l3_intf_t *intf)
{
    int rv;
    _bcm_l3_intf_cfg_t l3i;

    L3_INIT(unit);

    if (NULL == intf) {
        return (BCM_E_PARAM);
    } 

    if (!BCM_VLAN_VALID(intf->l3a_vid)) {
        return (BCM_E_PARAM);
    }

    sal_memset(&l3i, 0, sizeof(_bcm_l3_intf_cfg_t));
    l3i.l3i_vid = intf->l3a_vid;

    L3_LOCK(unit);
    rv = mbcm_driver[unit]->mbcm_l3_intf_get_by_vid(unit, &l3i);

    L3_UNLOCK(unit);
    if (rv < 0) {
        return (rv);
    }

    intf->l3a_intf_id = l3i.l3i_index;
    sal_memcpy(intf->l3a_mac_addr, l3i.l3i_mac_addr, sizeof(bcm_mac_t));
    intf->l3a_tunnel_idx = l3i.l3i_tunnel_idx;
    intf->l3a_flags = l3i.l3i_flags;
    intf->l3a_vrf = l3i.l3i_vrf;
    intf->l3a_ttl = l3i.l3i_ttl;
    intf->l3a_mtu = l3i.l3i_mtu;
    intf->l3a_group = l3i.l3i_group;
#ifdef BCM_TRIDENT2_SUPPORT
    intf->l3a_intf_flags = l3i.l3i_intf_flags;
#endif /* BCM_TRIDENT2_SUPPORT */

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_l3_intf_delete_all
 * Purpose:
 *      Delete all L3 interfaces
 * Parameters:
 *      unit - SOC device unit number
 * Returns:
 *      BCM_E_XXXX
 */
int
bcm_esw_l3_intf_delete_all(int unit)
{
    int rv;
    L3_INIT(unit);
    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_intf_delete_all(unit);

    L3_UNLOCK(unit);

    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_find
 * Purpose:
 *      Find an entry from the L3 host table given IP address.
 * Parameters:
 *      unit - SOC device unit number
 *      info - (Out) Pointer to memory for bcm_l3_host_t.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      1) The hit returned may be either L3SH or L3DH or Both
 *      2) Flag set to BCM_L3_IP6 for IPv6, default is IPv4
 */

int
bcm_esw_l3_host_find(int unit, bcm_l3_host_t *info)
{
    _bcm_l3_cfg_t l3cfg;
    int           rt;

    L3_INIT(unit);

    if (NULL == info) {
        return (BCM_E_PARAM);
    }

    /* Vrf is in device supported range check. */
    if ((info->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (info->l3a_vrf < BCM_L3_VRF_DEFAULT)) {
        return (BCM_E_PARAM);
    }
  
    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, info->l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
    l3cfg.l3c_flags = info->l3a_flags;
    l3cfg.l3c_vrf = info->l3a_vrf;

    L3_LOCK(unit);
    if (info->l3a_flags & BCM_L3_IP6) {
        sal_memcpy(l3cfg.l3c_ip6, info->l3a_ip6_addr, BCM_IP6_ADDRLEN);
        rt = mbcm_driver[unit]->mbcm_l3_ip6_get(unit, &l3cfg);
    } else {
        l3cfg.l3c_ip_addr = info->l3a_ip_addr;
        rt = mbcm_driver[unit]->mbcm_l3_ip4_get(unit, &l3cfg);
    }
    L3_UNLOCK(unit);

    if (rt < 0) {
        return (rt);
    }

    info->l3a_flags = l3cfg.l3c_flags;
    if (l3cfg.l3c_flags & BCM_L3_IPMC) {
        info->l3a_ipmc_ptr = l3cfg.l3c_ipmc_ptr;
    }
    sal_memcpy(info->l3a_nexthop_mac, l3cfg.l3c_mac_addr, sizeof(bcm_mac_t));
    info->l3a_intf = l3cfg.l3c_intf;
    info->l3a_port_tgid = l3cfg.l3c_port_tgid;
    info->l3a_modid = l3cfg.l3c_modid;

    /* For devices that support the overlaid_address_range, if the RPE
       field is '0', the 4 bits of PRI are the upper 4 bits
       (of total 10 bits) of the classId, the lower 6 come from classId
       as usual, for rest of the cases, the classID and PRI are copied
       over directly.
    */
    if (soc_feature(unit, soc_feature_overlaid_address_class) && 
               !BCM_L3_RPE_SET(l3cfg.l3c_flags)) {
        info->l3a_lookup_class = ((l3cfg.l3c_prio & 0xF) << 6);
        info->l3a_lookup_class |= (l3cfg.l3c_lookup_class & 0x3F);
    } else {
        info->l3a_lookup_class = l3cfg.l3c_lookup_class;
        info->l3a_pri = l3cfg.l3c_prio;
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_wlan) && 
        BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, l3cfg.l3c_intf)) {
        /* Get DVP using the egress object ID and construct a gport */
        egr_l3_next_hop_entry_t next_hop;
        _bcm_gport_dest_t dest;
        int nh_idx = l3cfg.l3c_intf - BCM_XGS3_DVP_EGRESS_IDX_MIN;
        BCM_IF_ERROR_RETURN
            (READ_EGR_L3_NEXT_HOPm(unit, MEM_BLOCK_ANY, nh_idx, &next_hop));
        dest.wlan_id = soc_EGR_L3_NEXT_HOPm_field32_get(unit, &next_hop, DVPf);
        dest.gport_type = _SHR_GPORT_TYPE_WLAN_PORT;
        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_construct(unit, &dest, &(info->l3a_port_tgid)));
    } else
#endif
    {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_l3_gport_construct(unit, info->l3a_port_tgid, 
                                         info->l3a_modid, info->l3a_port_tgid, 
                                         info->l3a_flags, 
                                         &(info->l3a_port_tgid)));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_esw_l3_host_add
 * Purpose:
 *      Add an entry to the L3 table.
 * Parameters:
 *      unit - SOC unit number
 *      info - Pointer to bcm_l3_host_t containing fields related to IP table.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *  1. Flag set to BCM_L3_IP6 for IPv6, default is IPv4
 *
 *  2. If BCM_L3_REPLACE flag is set, replace exisiting L3 entry
 *     with new information, otherwise, add only if key does not
 *     exist.
 *     On XGS, the DEFIP table points to the L3 entry, so this
 *     function will affect both L3 switching entry and routes.
 *     The affected route is one whose next hop IP address
 *     is the key (2nd parameter of this function).
 */

int
bcm_esw_l3_host_add(int unit, bcm_l3_host_t *info)
{
    int rv;
    _bcm_l3_cfg_t l3cfg;
    bcm_l3_host_t info_orig, info_local;
    bcm_l3_route_t  info_route;

    L3_INIT(unit);

    if (NULL == info) {
        return (BCM_E_PARAM);
    }

    /* Copy provided structure to local so it can be modified. */
    sal_memcpy(&info_local, info, sizeof(bcm_l3_host_t));

    /* Vrf is in device supported range check. */
    if ((info_local.l3a_vrf > SOC_VRF_MAX(unit)) || 
        (info_local.l3a_vrf < BCM_L3_VRF_DEFAULT)) {
        return (BCM_E_PARAM);
    }
  
    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, info_local.l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    /* FP lookup class range check. */
    /* These are the 3 classID validity checks:
     
       1. If the device is a TR3, the 10 bit classID is natively supported.
          and hence we check against the same.
     
       2. For devices that support the overlaid_address_range, if the RPE
          field is '0', the 4 bits of PRI are the upper 4 bits
          (of total 10 bits) of the classId, the lower 6 come from classId
          as usual, we hence need to check the value range accordingly. 
     
       3. For rest of the cases classID is restricted to 6 bits and we
          need to check accordingly.
    */

    if (SOC_IS_TRIUMPH3(unit)) {
        if ((info_local.l3a_lookup_class > SOC_EXT_ADDR_CLASS_MAX(unit)) ||
            (info_local.l3a_lookup_class < 0)) {
                    return (BCM_E_PARAM);
        }
    } else if (soc_feature(unit, soc_feature_overlaid_address_class) && 
               !BCM_L3_RPE_SET(info_local.l3a_flags)) {
                   if ((info_local.l3a_lookup_class > 
                        SOC_OVERLAID_ADDR_CLASS_MAX(unit)) ||
                      (info_local.l3a_lookup_class < 0)) {
                        return (BCM_E_PARAM);
            }
    } else if ((info_local.l3a_lookup_class > SOC_ADDR_CLASS_MAX(unit)) || 
               (info_local.l3a_lookup_class < 0)) {
                        return (BCM_E_PARAM);
    }

    if(!BCM_PRIORITY_VALID(info_local.l3a_pri)) {
        return (BCM_E_PARAM);
    }
    if (BCM_GPORT_IS_SET(info_local.l3a_port_tgid)) {
        
        /* l3a_port_tgid can contain either port or trunk */
        BCM_IF_ERROR_RETURN(
            _bcm_esw_l3_gport_resolve(unit, info_local.l3a_port_tgid, 
                                      &(info_local.l3a_port_tgid),
                                      &(info_local.l3a_modid), 
                                      &(info_local.l3a_port_tgid), 
                                      &(info_local.l3a_flags)));
    } else {
        PORT_DUALMODID_VALID(unit, info_local.l3a_port_tgid);
    }
    /* Check if host entry already exists. */
    L3_LOCK(unit);
    info_orig = info_local;
    rv = bcm_esw_l3_host_find(unit, &info_orig);
    if ((BCM_SUCCESS(rv)) && (0 == (info_local.l3a_flags & BCM_L3_REPLACE))) {
        /* If so BCM_L3_REPLACE flag must be set.  */
        L3_UNLOCK(unit);
        return (BCM_E_EXISTS);
    } else if ((BCM_FAILURE(rv)) && (BCM_E_NOT_FOUND != rv)) {
        L3_UNLOCK(unit);
        return (rv);
    }

    /* Initialize mbcm structure. */
    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
    sal_memcpy(l3cfg.l3c_mac_addr, info_local.l3a_nexthop_mac, sizeof(bcm_mac_t));
    l3cfg.l3c_intf = info_local.l3a_intf;
    l3cfg.l3c_modid = info_local.l3a_modid;
    l3cfg.l3c_port_tgid = info_local.l3a_port_tgid;
    l3cfg.l3c_flags = info_local.l3a_flags;
    l3cfg.l3c_vrf = info_local.l3a_vrf;

    /* If the chip uses the 'soc_feature_overlaid_address_class' feature and
       if the RPE is '0' the upper 4 bits of the classId need to be copied
       from the PRI field, the lower bits get copied to the classId field as, 
       for all other cases, the classID and PRI are assigned directly */
    if (soc_feature(unit, soc_feature_overlaid_address_class) && 
        (!BCM_L3_RPE_SET(info_local.l3a_flags))) {
            l3cfg.l3c_prio = ((info_local.l3a_lookup_class & 0x3C0) >> 6);
            l3cfg.l3c_lookup_class = (info_local.l3a_lookup_class & 0x3F);
    } else {
        l3cfg.l3c_prio = info_local.l3a_pri;
        l3cfg.l3c_lookup_class = info_local.l3a_lookup_class;
    }

    if (info_local.l3a_flags & BCM_L3_IP6) {
        sal_memcpy(l3cfg.l3c_ip6, info_local.l3a_ip6_addr, BCM_IP6_ADDRLEN);
        /* Add host entry - no prior entry found */
        if (BCM_FAILURE(rv)) {
            rv = mbcm_driver[unit]->mbcm_l3_ip6_add(unit, &l3cfg);
        } else {
            /* Remove Route entry and add to Host table */
            if (info_orig.l3a_flags & BCM_L3_HOST_AS_ROUTE) {

               /* Entry exists within ROUTE table */
               l3cfg.l3c_flags &=  ~BCM_L3_HOST_AS_ROUTE;

               /* Add entry to HOST Table */
               rv = mbcm_driver[unit]->mbcm_l3_ip6_add(unit, &l3cfg);

               if (BCM_SUCCESS(rv)) {

                  /* Init ROUTE entry */
                  sal_memset(&info_route, 0, sizeof(bcm_l3_route_t));
                  sal_memcpy(info_route.l3a_nexthop_mac, l3cfg.l3c_mac_addr, sizeof(bcm_mac_t));
                  info_route.l3a_vrf = l3cfg.l3c_vrf;
                  info_route.l3a_flags = BCM_L3_IP6;
                  sal_memcpy(info_route.l3a_ip6_net, l3cfg.l3c_ip6,
                                            sizeof(bcm_ip6_t));
                  bcm_ip6_mask_create(info_route.l3a_ip6_mask, 128);
                  info_route.l3a_intf = l3cfg.l3c_intf;
                  info_route.l3a_modid = l3cfg.l3c_modid;
                  info_route.l3a_port_tgid = l3cfg.l3c_port_tgid;
                  info_route.l3a_pri = l3cfg.l3c_prio;
                  info_route.l3a_lookup_class = l3cfg.l3c_lookup_class;

                  /* Delete ROUTE entry */
                  rv = bcm_esw_l3_route_delete(unit, &info_route);
                  if (BCM_SUCCESS(rv)) {
                     /* Special return value - as entry existed within ROUTE table */
#if defined(BCM_FIREBOLT_SUPPORT)
                     (void) bcm_xgs3_l3_host_as_route_return_get(unit, &rv);
#endif /* BCM_FIREBOLT_SUPPORT  */
                  }
               } else {
                  /* Update ROUTE entry */
                  l3cfg.l3c_flags |=  BCM_L3_HOST_AS_ROUTE;
                  rv = mbcm_driver[unit]->mbcm_l3_ip6_replace(unit, &l3cfg);
               }
            } else {
                 /* Replace prior host entry found */
                  rv = mbcm_driver[unit]->mbcm_l3_ip6_replace(unit, &l3cfg);
            }
        }
    } else {
        l3cfg.l3c_ip_addr = info_local.l3a_ip_addr;
        /* Add host entry - no prior entry found */
        if (BCM_FAILURE(rv)) {
            rv = mbcm_driver[unit]->mbcm_l3_ip4_add(unit, &l3cfg);
        } else {
            /* Remove Route entry and add to Host table */
            if (info_orig.l3a_flags & BCM_L3_HOST_AS_ROUTE) {

               /* Entry exists within ROUTE table */
               l3cfg.l3c_flags &=  ~BCM_L3_HOST_AS_ROUTE;

               /* Add entry to HOST Table */
               rv = mbcm_driver[unit]->mbcm_l3_ip4_add(unit, &l3cfg);

               if (BCM_SUCCESS(rv)) {

                  /* Init ROUTE entry */
                  sal_memset(&info_route, 0, sizeof(bcm_l3_route_t));
                  sal_memcpy(info_route.l3a_nexthop_mac, l3cfg.l3c_mac_addr, sizeof(bcm_mac_t));
                  info_route.l3a_vrf = l3cfg.l3c_vrf;
                  info_route.l3a_subnet = l3cfg.l3c_ip_addr;
                  info_route.l3a_ip_mask = bcm_ip_mask_create(32);
                  info_route.l3a_intf = l3cfg.l3c_intf;
                  info_route.l3a_modid = l3cfg.l3c_modid;
                  info_route.l3a_port_tgid = l3cfg.l3c_port_tgid;
                  info_route.l3a_pri = l3cfg.l3c_prio;
                  info_route.l3a_lookup_class = l3cfg.l3c_lookup_class;

                  /* Delete ROUTE entry */
                  rv = bcm_esw_l3_route_delete(unit, &info_route);
                  if (BCM_SUCCESS(rv)) {
                     /* Special return value - as entry existed within ROUTE table */
#if defined(BCM_FIREBOLT_SUPPORT)
                     (void) bcm_xgs3_l3_host_as_route_return_get(unit, &rv);
#endif /* BCM_FIREBOLT_SUPPORT  */
                  }
               } else {
                  /* Update ROUTE entry */
                  l3cfg.l3c_flags |=  BCM_L3_HOST_AS_ROUTE;
                  rv = mbcm_driver[unit]->mbcm_l3_ip4_replace(unit, &l3cfg);
               }
            } else {
                 /* Replace prior host entry found */
                 rv = mbcm_driver[unit]->mbcm_l3_ip4_replace(unit, &l3cfg);
            }
        }
    }
    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_delete
 * Purpose:
 *      Delete an entry from the L3 table.
 * Parameters:
 *      unit - SOC device unit number
 *      ip_addr - Pointer to host structure containing
 *                destination IP address.
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_INTERNAL          Chip access failure
 *      BCM_E_INIT              Unit Not initialized yet
 *      BCM_E_NOT_FOUND      Cannot find a match
 * Notes:
 *     Flag set to BCM_L3_IP6 for IPv6, default is IPv4
 */

int
bcm_esw_l3_host_delete(int unit, bcm_l3_host_t *ip_addr)
{
    _bcm_l3_cfg_t l3cfg;
    bcm_l3_host_t info;
    int rt;

    L3_INIT(unit);

    if (NULL == ip_addr) {
        return (BCM_E_PARAM);
    }

    /* Vrf is in device supported range check. */
    if ((ip_addr->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (ip_addr->l3a_vrf < BCM_L3_VRF_DEFAULT)) {
        return (BCM_E_PARAM);
    }
  
    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, ip_addr->l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    bcm_l3_host_t_init(&info);
    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
    info.l3a_vrf = ip_addr->l3a_vrf;

    L3_LOCK(unit);
    if (ip_addr->l3a_flags & BCM_L3_IP6) {
        info.l3a_flags   = BCM_L3_IP6;
        sal_memcpy(info.l3a_ip6_addr, ip_addr->l3a_ip6_addr, BCM_IP6_ADDRLEN);
        rt = bcm_esw_l3_host_find(unit, &info);
        if (rt != BCM_E_NONE) {
            L3_UNLOCK(unit);
            return rt;
        }

        sal_memcpy(l3cfg.l3c_ip6,
                   ip_addr->l3a_ip6_addr, BCM_IP6_ADDRLEN);
        l3cfg.l3c_flags = info.l3a_flags;
        l3cfg.l3c_vrf = ip_addr->l3a_vrf;
        rt = mbcm_driver[unit]->mbcm_l3_ip6_delete(unit, &l3cfg);
    } else {
        info.l3a_ip_addr = ip_addr->l3a_ip_addr;
        rt = bcm_esw_l3_host_find(unit, &info);
        if (rt != BCM_E_NONE) {
            L3_UNLOCK(unit);
            return rt;
        }

        l3cfg.l3c_ip_addr = ip_addr->l3a_ip_addr;
        l3cfg.l3c_flags = info.l3a_flags;  /* BCM_L3_L2TOCPU might be set */
        l3cfg.l3c_vrf = ip_addr->l3a_vrf;
        rt = mbcm_driver[unit]->mbcm_l3_ip4_delete(unit, &l3cfg);
    }
    L3_UNLOCK(unit);
    return rt;
}

/*
 * Function:
 *      bcm_esw_l3_host_delete_by_network
 * Purpose:
 *      Delete all L3 entries that match the IP address mask
 * Parameters:
 *      unit    - SOC device unit number
 *      net_addr - Pointer to route structure containing the network
 *                address to be matched against
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_INTERNAL          Chip access failure
 *      BCM_E_INIT              Unit Not initialized yet
 * Notes:
 *     Flag set to BCM_L3_IP6 for IPv6, default is IPv4
 */

int
bcm_esw_l3_host_delete_by_network(int unit, bcm_l3_route_t *net_addr)
{
    _bcm_l3_cfg_t l3cfg;
    int rv;

    L3_INIT(unit);

    if (NULL == net_addr) {
        return (BCM_E_PARAM);
    }

    /* Vrf is in device supported range check. */
    if ((net_addr->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (net_addr->l3a_vrf < BCM_L3_VRF_DEFAULT)) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, net_addr->l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
    l3cfg.l3c_vrf        = net_addr->l3a_vrf;

    L3_LOCK(unit);
    if (net_addr->l3a_flags & BCM_L3_IP6) {
        sal_memcpy(l3cfg.l3c_ip6,
                   net_addr->l3a_ip6_net, BCM_IP6_ADDRLEN);
        sal_memcpy(l3cfg.l3c_ip6_mask,
                   net_addr->l3a_ip6_mask, BCM_IP6_ADDRLEN);
        l3cfg.l3c_flags = BCM_L3_IP6;
        rv = mbcm_driver[unit]->mbcm_l3_ip6_delete_prefix(unit, &l3cfg);
    } else { 
        l3cfg.l3c_ip_addr    = net_addr->l3a_subnet;
        l3cfg.l3c_ip_mask    = net_addr->l3a_ip_mask;
        rv = mbcm_driver[unit]->mbcm_l3_ip4_delete_prefix(unit, &l3cfg);
    } 
    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_delete_by_interface
 * Purpose:
 *      Delete all L3 entries that match the L3 interface
 * Parameters:
 *      unit - SOC device unit number
 *      info - The host structure containing the to be matched interface ID
 *             (IN)l3a_intf_id member
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_INIT              Unit Not initialized yet
 * Notes:
 *   - Flag set to BCM_L3_NEGATE to delete all host entries
 *     that do NOT match the L3 interface.
 */
int
bcm_esw_l3_host_delete_by_interface(int unit, bcm_l3_host_t *info)
{
    int rv;
    _bcm_l3_cfg_t l3cfg;
    int negate;

    L3_INIT(unit);

    if (NULL == info) {
        return (BCM_E_PARAM);
    }

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));

    l3cfg.l3c_intf = info->l3a_intf;
    negate = info->l3a_flags & BCM_L3_NEGATE ? 1 : 0;

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_delete_intf(unit, &l3cfg, negate);

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_delete_all
 * Purpose:
 *      Delete all entries from the L3 table.
 * Parameters:
 *      unit - SOC device unit number
 *      info - Pointer to host structure containing the flag
 *             indicating IPv4 or IPv6
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_INTERNAL          Chip access failure
 *      BCM_E_INIT              Unit Not initialized yet
 */
int
bcm_esw_l3_host_delete_all(int unit, bcm_l3_host_t *info)
{
    int rv;
    L3_INIT(unit);

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_delete_all(unit);

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_conflict_get
 * Purpose:
 *      Given a IP address, return conflicts in the L3 table.
 * Parameters:
 *      unit     - SOC unit number.
 *      ipkey    - IP address to test conflict condition
 *      cf_array - (OUT) arrary of conflicting addresses(at most 8)
 *      cf_max   - max number of conflicts wanted
 *      cf_count    - (OUT) actual # of conflicting addresses
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_host_conflict_get(int unit, bcm_l3_key_t *ipkey, bcm_l3_key_t *cf_array,
                    int cf_max, int *cf_count)
{
    int rv;
    L3_INIT(unit);

    if (NULL == ipkey) {
        return (BCM_E_PARAM);
    } 

    /* Vrf is in device supported range check. */
    if ((ipkey->l3k_vrf > SOC_VRF_MAX(unit)) || 
        (ipkey->l3k_vrf < BCM_L3_VRF_DEFAULT)) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, ipkey->l3k_flags)) {
        return (BCM_E_UNAVAIL);
    }

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_conflict_get(unit, ipkey, cf_array, 
                                                 cf_max, cf_count);
    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_age
 * Purpose:
 *      Age out the L3 entry by clearing the HIT bit when appropriate,
 *      the L3 host entry itself is removed if HIT bit is not set.
 * Parameters:
 *      unit - SOC device unit number
 *      flags - The criteria used to age out L3 table.
 *      age_cb - Call back routine.
 *      user_data  - (IN) User provided cookie for callback.
 * Returns:
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_INIT              Unit Not initialized yet
 */

int
bcm_esw_l3_host_age(int unit, uint32 flags, bcm_l3_host_traverse_cb age_cb, 
                    void *user_data)
{
    int rv;
    L3_INIT(unit);

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, flags)) {
        return (BCM_E_UNAVAIL);
    }

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_age(unit, flags, age_cb, user_data);

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_traverse
 * Purpose:
 *      Go through all valid L3 entries, and call the callback function
 *      at each entry
 * Parameters:
 *      unit - SOC device unit number
 *      flags - BCM_L3_IP6
 *      start - Callback called after this many L3 entries
 *      end   - Callback called before this many L3 entries
 *      cb    - User supplied callback function
 *      user_data - User supplied cookie used in parameter in callback function
 * Returns:
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_INIT              Unit Not initialized yet
 */

int
bcm_esw_l3_host_traverse(int unit, uint32 flags,
                         uint32 start, uint32 end,
                         bcm_l3_host_traverse_cb cb, void *user_data)
{
    int rv;

    L3_INIT(unit);

    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, flags)) {
        return (BCM_E_UNAVAIL);
    }

    L3_LOCK(unit);

    if (flags & BCM_L3_IP6) {
        rv = mbcm_driver[unit]->mbcm_l3_ip6_traverse(unit,
                                start, end, cb, user_data);
    } else {
        rv = mbcm_driver[unit]->mbcm_l3_ip4_traverse(unit,
                                start, end, cb, user_data);
    }

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_host_invalidate_entry
 * Purpose:
 *      Given a IP address, invalidate the L3 entry without
 *      clearing the entry information, so that the entry can be
 *      turned back to valid without resetting all the information.
 * Parameters:
 *      unit     - SOC unit number.
 *      ipaddr   - IP address to test conflict condition
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_host_invalidate_entry(int unit, bcm_ip_t ipaddr)
{
#ifdef BCM_XGS12_SWITCH_SUPPORT
    int rv;
#endif  /* BCM_XGS_SWITCH_SUPPORT */
    L3_INIT(unit);

#ifdef BCM_XGS12_SWITCH_SUPPORT
    if (SOC_IS_XGS12_SWITCH(unit)) {
        L3_LOCK(unit);
        rv = bcm_xgs_l3_invalidate_entry(unit, ipaddr);
        L3_UNLOCK(unit);
        return (rv);
    }
#endif  /* BCM_XGS_SWITCH_SUPPORT */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_l3_host_validate_entry
 * Purpose:
 *      Given a IP address, validate the L3 entry without
 *      resetting the entry information.
 * Parameters:
 *      unit     - SOC unit number.
 *      ipaddr   - IP address to test conflict condition
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_host_validate_entry(int unit, bcm_ip_t ipaddr)
{
#ifdef BCM_XGS12_SWITCH_SUPPORT
    int rv;
#endif  /* BCM_XGS_SWITCH_SUPPORT */
    L3_INIT(unit);

#ifdef BCM_XGS12_SWITCH_SUPPORT
    if (SOC_IS_XGS12_SWITCH(unit)) {
        L3_LOCK(unit);
        rv = bcm_xgs_l3_validate_entry(unit, ipaddr);
        L3_UNLOCK(unit);
        return (rv);
    }
#endif  /* BCM_XGS_SWITCH_SUPPORT */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_l3_route_add
 * Purpose:
 *      Add an IP route to the DEFIP table.
 * Parameters:
 *      unit - SOC device unit number
 *      info - Pointer to bcm_l3_route_t containing all valid fields.
 * Returns:
 *      BCM_E_XXX
 * Note:
 *  1. In addition to the route info (i.e. the network number,
 *     nexthop MAC addr, L3 intf, port, modid, or trunk ID),
 *     on XGS chips, one must specify the nexthop IP addr.
 *     This is used as key to find entry in the L3 table.
 *
 *  2. XGS ECMP route must have BCM_L3_MULTIPATH flag set.
 *     ECMP is supported on all XGS devices.
 *
 *  3. BCM_L3_DEFIP_LOCAL flag is used, then packets that matches
 *     LPM table will be sent to CPU unchanged (XGS only).
 *     BCM_L3_DEFIP_LOCAL flag is used for last hop direct-connected
 *     subnet (e.g where workstations are attached to router).
 *     BCM_L3_DEFIP_LOCAL and BCM_L3_MULTIPATH can not be both set.
 *     When BCM_L3_DEFIP_LOCAL flag is used, nexthop IP addr
 *     is not needed.
 */

int
bcm_esw_l3_route_add(int unit, bcm_l3_route_t *info)
{
    int max_prefix_length;
    _bcm_defip_cfg_t defip;
    uint8 ip6_zero[BCM_IP6_ADDRLEN] = {0};
    int rv;
    bcm_l3_route_t  info_local;

    L3_INIT(unit);

    if (NULL == info) {
        return (BCM_E_PARAM);
    }

    /* Copy provided structure to local so it can be modified. */
    sal_memcpy(&info_local, info, sizeof(bcm_l3_route_t));

    if ((info_local.l3a_vrf > SOC_VRF_MAX(unit)) || 
        (info_local.l3a_vrf < BCM_L3_VRF_GLOBAL)) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, info_local.l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    if ((info_local.l3a_lookup_class > SOC_ADDR_CLASS_MAX(unit)) || 
        (info_local.l3a_lookup_class < 0)) {
        return (BCM_E_PARAM);
    }

    if(!BCM_PRIORITY_VALID(info_local.l3a_pri)) {
        return (BCM_E_PARAM);
    }

    if (BCM_GPORT_IS_SET(info_local.l3a_port_tgid)) {

        /* l3a_port_tgid can contain either port or trunk */
        BCM_IF_ERROR_RETURN(
            _bcm_esw_l3_gport_resolve(unit, info_local.l3a_port_tgid, 
                                      &(info_local.l3a_port_tgid),
                                      &(info_local.l3a_modid), 
                                      &(info_local.l3a_port_tgid), 
                                      &(info_local.l3a_flags)));
    } else {
        PORT_DUALMODID_VALID(unit, info_local.l3a_port_tgid);
    }

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));

    sal_memcpy(defip.defip_mac_addr, info_local.l3a_nexthop_mac,
               sizeof(bcm_mac_t));
    defip.defip_intf       = info_local.l3a_intf;
    defip.defip_modid      = info_local.l3a_modid;
    defip.defip_port_tgid  = info_local.l3a_port_tgid;
    defip.defip_vid        = info_local.l3a_vid;
    defip.defip_flags      = info_local.l3a_flags;
    defip.defip_prio       = info_local.l3a_pri;
    defip.defip_vrf        = info_local.l3a_vrf;
    defip.defip_tunnel_option = (info_local.l3a_tunnel_option & 0xffff);
    defip.defip_mpls_label = info_local.l3a_mpls_label;
    defip.defip_lookup_class      = info_local.l3a_lookup_class;

    L3_LOCK(unit);
    if (info_local.l3a_flags & BCM_L3_IP6) {
        max_prefix_length = 
            soc_feature(unit, soc_feature_lpm_prefix_length_max_128) ? 128 : 64;
        if (bcm_ip6_mask_length(info_local.l3a_ip6_mask) == 0 &&
            sal_memcmp(info_local.l3a_ip6_net, ip6_zero, BCM_IP6_ADDRLEN) != 0) {
            L3_UNLOCK(unit);
            return (BCM_E_PARAM);
        }
        sal_memcpy(defip.defip_ip6_addr, info_local.l3a_ip6_net, BCM_IP6_ADDRLEN);
        defip.defip_sub_len = bcm_ip6_mask_length(info_local.l3a_ip6_mask);
        if (defip.defip_sub_len > max_prefix_length) {
            L3_UNLOCK(unit);
            return (BCM_E_PARAM);
        } 
        rv = mbcm_driver[unit]->mbcm_ip6_defip_add(unit, &defip);
    } else {
        if (info_local.l3a_ip_mask == 0 && info_local.l3a_subnet != 0) {
            L3_UNLOCK(unit);
            return (BCM_E_PARAM);
        }
        defip.defip_ip_addr    = info_local.l3a_subnet & info_local.l3a_ip_mask;
        defip.defip_sub_len    = bcm_ip_mask_length(info_local.l3a_ip_mask);
        defip.defip_nexthop_ip = info_local.l3a_nexthop_ip;
        rv = mbcm_driver[unit]->mbcm_ip4_defip_add(unit, &defip);
    }
    L3_UNLOCK(unit);

    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_route_delete
 * Purpose:
 *      Delete an entry from the Default IP Router table.
 * Parameters:
 *      unit - SOC device unit number
 *      info - Pointer to bcm_l3_route_t structure with valid IP subnet & mask.
 * Returns:
 *      BCM_E_XXX.
 * Note:
 *  1. For non-ECMP routes, it suffices to specify the network number
 *     in order to delete the route, since there is only one path.
 *
 *  2. For ECMP routes, in addition to specifying the network number,
 *     one must specify which ECMP path is to be deleted.
 *     This is done by specifying the following parameters:
 *     nexthop MAC addr, L3 intf, port, modid, next hop IP addr.
 *
 *     This is how these parameters are used to identify the ECMP path:
 *        next hop IP addr | nexthop MAC addr |
 *        (L3 intf, port, modid)
 *     Unspecified nexthop IP or MAC addr must be set to 0.
 *
 *     XGS ECMP route must have BCM_L3_MULTIPATH flag set.
 *     ECMP is supported on all XGS devices.
 */

int
bcm_esw_l3_route_delete(int unit, bcm_l3_route_t *info)
{
    int max_prefix_length;
    _bcm_defip_cfg_t defip;
    int rv;
    bcm_l3_route_t info_local;

    L3_INIT(unit);

    if (NULL == info) {
        return (BCM_E_PARAM);
    }

    /* Copy provided structure to local so it can be modified. */
    sal_memcpy(&info_local, info, sizeof(bcm_l3_route_t));

    if ((info_local.l3a_vrf > SOC_VRF_MAX(unit)) || 
        (info_local.l3a_vrf < BCM_L3_VRF_GLOBAL)) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, info_local.l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));

    defip.defip_vid        = info_local.l3a_vid;
    defip.defip_flags      = info_local.l3a_flags;
    defip.defip_vrf        = info_local.l3a_vrf;

    if (info_local.l3a_flags & BCM_L3_MULTIPATH) {
         if (BCM_GPORT_IS_SET(info_local.l3a_port_tgid)) {
              /* l3a_port_tgid can contain either port or trunk */
              BCM_IF_ERROR_RETURN(
                   _bcm_esw_l3_gport_resolve(unit, info_local.l3a_port_tgid, 
                                  &(info_local.l3a_port_tgid),
                                  &(info_local.l3a_modid), 
                                  &(info_local.l3a_port_tgid), 
                                  &(info_local.l3a_flags)));
         } 

        sal_memcpy(defip.defip_mac_addr, info_local.l3a_nexthop_mac,
                   sizeof(bcm_mac_t));
        defip.defip_intf      = info_local.l3a_intf;
        defip.defip_port_tgid = info_local.l3a_port_tgid;
        defip.defip_modid     = info_local.l3a_modid;
        defip.defip_flags      = info_local.l3a_flags;
    }

    L3_LOCK(unit);
    if (info_local.l3a_flags & BCM_L3_IP6) {
        max_prefix_length = 
            soc_feature(unit, soc_feature_lpm_prefix_length_max_128) ? 128 : 64;
        sal_memcpy(defip.defip_ip6_addr, info_local.l3a_ip6_net, BCM_IP6_ADDRLEN);
        defip.defip_sub_len    = bcm_ip6_mask_length(info_local.l3a_ip6_mask);
        if (defip.defip_sub_len > max_prefix_length) {
            L3_UNLOCK(unit);
            return (BCM_E_PARAM);
        } 
        rv = mbcm_driver[unit]->mbcm_ip6_defip_delete(unit, &defip);
    } else {
        defip.defip_ip_addr    = info_local.l3a_subnet & info_local.l3a_ip_mask;
        defip.defip_sub_len    = bcm_ip_mask_length(info_local.l3a_ip_mask);
        defip.defip_nexthop_ip = info_local.l3a_nexthop_ip;

        rv = mbcm_driver[unit]->mbcm_ip4_defip_delete(unit, &defip);
    }

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_route_get
 * Purpose:
 *      Get an entry from the DEF IP table.
 * Parameters:
 *      unit - SOC device unit number
 *      info - (OUT)Pointer to bcm_l3_route_t for return information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *    - Application has to pass IP address, IP mask for the network
 *    - If the route is local, then the BCM_L3_DEFIP_LOCAL flag is
 *      set, and next hop MAC, port/tgid, interface, MODID fields
 *      are all cleared.
 */

int
bcm_esw_l3_route_get(int unit, bcm_l3_route_t *info)
{
    int max_prefix_length;
    _bcm_defip_cfg_t defip;
    int rv;

    L3_INIT(unit);

    if (NULL == info) {
        return BCM_E_PARAM;
    }

    if ((info->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (info->l3a_vrf < BCM_L3_VRF_GLOBAL)) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, info->l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));
    defip.defip_flags = info->l3a_flags;
    defip.defip_vrf     = info->l3a_vrf;

    L3_LOCK(unit);
    if (info->l3a_flags & BCM_L3_IP6) {
        max_prefix_length = 
            soc_feature(unit, soc_feature_lpm_prefix_length_max_128) ? 128 : 64;
        sal_memcpy(defip.defip_ip6_addr, info->l3a_ip6_net, BCM_IP6_ADDRLEN);
        defip.defip_sub_len    = bcm_ip6_mask_length(info->l3a_ip6_mask);
        if (defip.defip_sub_len > max_prefix_length) {
            L3_UNLOCK(unit);
            return (BCM_E_PARAM);
        } 
        rv = mbcm_driver[unit]->mbcm_ip6_defip_cfg_get(unit, &defip);
    } else {
        defip.defip_ip_addr = info->l3a_subnet & info->l3a_ip_mask;
        defip.defip_sub_len = bcm_ip_mask_length(info->l3a_ip_mask);
        rv = mbcm_driver[unit]->mbcm_ip4_defip_cfg_get(unit, &defip);
    }

    L3_UNLOCK(unit);
    if (rv < 0) {
        return rv;
    }

    sal_memcpy(info->l3a_nexthop_mac, defip.defip_mac_addr,
               sizeof(bcm_mac_t));
    info->l3a_nexthop_ip = defip.defip_nexthop_ip;
    info->l3a_intf = defip.defip_intf;
    info->l3a_port_tgid = defip.defip_port_tgid;
    info->l3a_modid = defip.defip_modid;
    info->l3a_lookup_class = defip.defip_lookup_class;
    info->l3a_flags = defip.defip_flags;
    info->l3a_mpls_label = defip.defip_mpls_label;
    info->l3a_tunnel_option = defip.defip_tunnel_option;

    BCM_IF_ERROR_RETURN(
        _bcm_esw_l3_gport_construct(unit, info->l3a_port_tgid, info->l3a_modid, 
                                    info->l3a_port_tgid, info->l3a_flags, 
                                    &(info->l3a_port_tgid)));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_l3_route_find
 * Purpose:
 *      Get a Longest Prefix Matched entry from the DEF IP table for 
 *      a host address.
 * Parameters:
 *      unit  - SOC device unit number
 *      dest  - (IN) Given host address.
 *      route - (OUT)Pointer to bcm_l3_route_t for return information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *    - Application has to pass IP address and flag for IP version. 
 *    
 */
int
bcm_esw_l3_route_find(int unit, bcm_l3_host_t *host, bcm_l3_route_t *route)
{
    int max_prefix_length;
    _bcm_defip_cfg_t defip;
    int rv;

    L3_INIT(unit);

    if (NULL == host || NULL == route) {
        return BCM_E_PARAM;
    }

    if ((host->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (host->l3a_vrf < BCM_L3_VRF_GLOBAL)) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, host->l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));
    defip.defip_flags = host->l3a_flags;
    defip.defip_vrf   = host->l3a_vrf;

    L3_LOCK(unit);
    if (host->l3a_flags & BCM_L3_IP6) {
        max_prefix_length = 128;
        sal_memcpy(defip.defip_ip6_addr, host->l3a_ip6_addr, BCM_IP6_ADDRLEN);
        defip.defip_sub_len = max_prefix_length;
        rv = bcm_xgs3_defip_find(unit, &defip);
    } else {
        defip.defip_ip_addr = host->l3a_ip_addr;
        defip.defip_sub_len = 32;
        rv = bcm_xgs3_defip_find(unit, &defip);
    }

    L3_UNLOCK(unit);
    if (rv < 0) {
        return rv;
    }

    sal_memcpy(route->l3a_nexthop_mac, defip.defip_mac_addr,
               sizeof(bcm_mac_t));
    route->l3a_nexthop_ip = defip.defip_nexthop_ip;
    route->l3a_intf = defip.defip_intf;
    route->l3a_port_tgid = defip.defip_port_tgid;
    route->l3a_modid = defip.defip_modid;
    route->l3a_lookup_class = defip.defip_lookup_class;
    route->l3a_flags = defip.defip_flags;
    route->l3a_mpls_label = defip.defip_mpls_label;
    route->l3a_tunnel_option = (uint32)defip.defip_index;
    route->l3a_vrf = defip.defip_vrf;
    if (defip.defip_flags & BCM_L3_IP6) {
        sal_memcpy(route->l3a_ip6_net, defip.defip_ip6_addr, BCM_IP6_ADDRLEN);
        bcm_ip6_mask_create(route->l3a_ip6_mask, defip.defip_sub_len);
    } else {
        route->l3a_subnet = defip.defip_ip_addr;
        route->l3a_ip_mask = bcm_ip_mask_create(defip.defip_sub_len);
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_l3_gport_construct(unit, route->l3a_port_tgid, route->l3a_modid, 
                                    route->l3a_port_tgid, route->l3a_flags, 
                                    &(route->l3a_port_tgid)));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_l3_subnet_route_find
 * Purpose:
 *      Get a Longest Prefix Matched entry from the DEF IP table for a subnet.
 * Parameters:
 *      unit  - SOC device unit number
 *      input - (IN) Given subnet.
 *      route - (OUT)Pointer to bcm_l3_route_t for return information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *    - Application has to pass IP address, IP mask for the network, and 
 *    - flag for IP version.
 *    
 */
int
bcm_esw_l3_subnet_route_find(int unit, bcm_l3_route_t *input, 
                             bcm_l3_route_t *route)
{
    int max_prefix_length;
    _bcm_defip_cfg_t defip;
    int rv;

    L3_INIT(unit);

    if (NULL == input || NULL == route) {
        return BCM_E_PARAM;
    }

    if ((input->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (input->l3a_vrf < BCM_L3_VRF_GLOBAL)) {
        return (BCM_E_PARAM);
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, input->l3a_flags)) {
        return (BCM_E_UNAVAIL);
    }

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));
    defip.defip_flags = input->l3a_flags;
    defip.defip_vrf   = input->l3a_vrf;

    L3_LOCK(unit);
    if (input->l3a_flags & BCM_L3_IP6) {
        bcm_ip6_t mask;
        max_prefix_length = 128;
        sal_memcpy(defip.defip_ip6_addr, input->l3a_ip6_net, BCM_IP6_ADDRLEN);
        sal_memcpy(mask, input->l3a_ip6_mask, BCM_IP6_ADDRLEN);
        defip.defip_sub_len = bcm_ip6_mask_length(mask);
        if (defip.defip_sub_len > max_prefix_length) {
            L3_UNLOCK(unit);
            return (BCM_E_PARAM);
        } 
        rv = bcm_xgs3_defip_find(unit, &defip);
    } else {
        defip.defip_ip_addr = input->l3a_subnet & input->l3a_ip_mask;
        defip.defip_sub_len = bcm_ip_mask_length(input->l3a_ip_mask);
        rv = bcm_xgs3_defip_find(unit, &defip);
    }

    L3_UNLOCK(unit);
    if (rv < 0) {
        return rv;
    }

    sal_memcpy(route->l3a_nexthop_mac, defip.defip_mac_addr,
               sizeof(bcm_mac_t));
    route->l3a_nexthop_ip = defip.defip_nexthop_ip;
    route->l3a_intf = defip.defip_intf;
    route->l3a_port_tgid = defip.defip_port_tgid;
    route->l3a_modid = defip.defip_modid;
    route->l3a_lookup_class = defip.defip_lookup_class;
    route->l3a_flags = defip.defip_flags;
    route->l3a_mpls_label = defip.defip_mpls_label;
    route->l3a_tunnel_option = (uint32)defip.defip_index;
    route->l3a_vrf = defip.defip_vrf;
    if (defip.defip_flags & BCM_L3_IP6) {
        sal_memcpy(route->l3a_ip6_net, defip.defip_ip6_addr, BCM_IP6_ADDRLEN);
        bcm_ip6_mask_create(route->l3a_ip6_mask, defip.defip_sub_len);
    } else {
        route->l3a_subnet = defip.defip_ip_addr;
        route->l3a_ip_mask = bcm_ip_mask_create(defip.defip_sub_len);
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_l3_gport_construct(unit, route->l3a_port_tgid, route->l3a_modid, 
                                    route->l3a_port_tgid, route->l3a_flags, 
                                    &(route->l3a_port_tgid)));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_esw_l3_route_multipath_get
 * Purpose:
 *      Get all paths for a route, useful for ECMP route,
 *      For non-ECMP route, there is only one path returned.
 * Parameters:
 *      unit       - (IN) SOC device unit number
 *      the_route  - (IN) route's net/mask
 *      path_array - (OUT) Array of all ECMP paths
 *      path_count - (OUT) Actual number of ECMP paths
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_esw_l3_route_multipath_get(int unit, bcm_l3_route_t *the_route,
       bcm_l3_route_t *path_array, int max_path, int *path_count)
{
    int max_prefix_length;
    _bcm_defip_cfg_t defip;
    int rv;

    L3_INIT(unit);

    if ((NULL == the_route) || (max_path < 1)) {
        return BCM_E_PARAM;
    }

    if ((the_route->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (the_route->l3a_vrf < BCM_L3_VRF_GLOBAL)) {
        return (BCM_E_PARAM);
    }

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));
    defip.defip_flags = the_route->l3a_flags;
    defip.defip_vrf = the_route->l3a_vrf;

    L3_LOCK(unit);
    if (the_route->l3a_flags & BCM_L3_IP6) {
        max_prefix_length = 
            soc_feature(unit, soc_feature_lpm_prefix_length_max_128) ? 128 : 64;
        sal_memcpy(defip.defip_ip6_addr, the_route->l3a_ip6_net, BCM_IP6_ADDRLEN);
        defip.defip_sub_len    = bcm_ip6_mask_length(the_route->l3a_ip6_mask);
        if (defip.defip_sub_len > max_prefix_length) {
            L3_UNLOCK(unit);
            return (BCM_E_PARAM);
        } 
        rv = mbcm_driver[unit]->mbcm_ip6_defip_ecmp_get_all(unit, &defip,
                                 path_array, max_path, path_count);
    } else {
        defip.defip_ip_addr = the_route->l3a_subnet & the_route->l3a_ip_mask;
        defip.defip_sub_len = bcm_ip_mask_length(the_route->l3a_ip_mask);
        rv = mbcm_driver[unit]->mbcm_ip4_defip_ecmp_get_all(unit, &defip,
                                 path_array, max_path, path_count);
    }
    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_route_delete_by_interface
 * Purpose:
 *      Delete all entries from the DEF IP table which have
 *      L3 interface number intf.
 * Parameters:
 *      unit - SOC device unit number
 *      intf - The L3 interface number
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_UNIT              Illegal unit number
 *      BCM_E_INIT              Unit Not initialized yet
 * Notes:
 *   - Flag set to BCM_L3_IP6 for IPv6, default is IPv4
 *   - Flag set to BCM_L3_NEGATE to delete all host entries
 *     that do NOT match the L3 interface.
 */

int
bcm_esw_l3_route_delete_by_interface(int unit, bcm_l3_route_t *info)
{
    int rv;
    _bcm_defip_cfg_t defip;

    L3_INIT(unit);

    if (NULL == info) {
        return (BCM_E_PARAM);
    }

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));
    defip.defip_intf = info->l3a_intf;
    defip.defip_flags = info->l3a_flags;

    L3_LOCK(unit);

    if (info->l3a_flags & BCM_L3_NEGATE) {
        rv = mbcm_driver[unit]->mbcm_defip_delete_intf(unit, &defip, 1);
    } else {
        rv = mbcm_driver[unit]->mbcm_defip_delete_intf(unit, &defip, 0);
    }

    L3_UNLOCK(unit);

    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_route_delete_all
 * Purpose:
 *      Remove all defip entries for a specified unit.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 * Returns:
 *      BCM_E_NONE - delete successful.
 *      BCM_E_XXX - delete failed.
 */

int
bcm_esw_l3_route_delete_all(int unit, bcm_l3_route_t *info)
{
    int rv;
    L3_INIT(unit);

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_defip_delete_all(unit);

    L3_UNLOCK(unit);

    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_route_traverse
 * Purpose:
 *      Find entries from the DEF IP table.
 * Parameters:
 *      unit - SOC device unit number
 *      flags - BCM_L3_IP6
 *      start - Starting point of interest.
 *      end - Ending point of interest.
 *      trav_fn - User callback function, called once per defip entry.
 *      user_data - User supplied cookie used in parameter in callback function
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The hit returned may be either L3SH or L3DH or Both
 */
int
bcm_esw_l3_route_traverse(int unit, uint32 flags,
                          uint32 start, uint32 end,
                          bcm_l3_route_traverse_cb trav_fn, void *user_data)
{
    int rv;

    L3_INIT(unit);
    L3_LOCK(unit);

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, flags)) {
        L3_UNLOCK(unit);
        return (BCM_E_UNAVAIL);
    }

    if (flags & BCM_L3_IP6) {
        rv = mbcm_driver[unit]->mbcm_ip6_defip_traverse(unit,
                                start, end, trav_fn, user_data);
    } else {
        rv = mbcm_driver[unit]->mbcm_ip4_defip_traverse(unit,
                                start, end, trav_fn, user_data);
    }

    L3_UNLOCK(unit);
    return (rv);
}

/* to be deprecated */
int
bcm_esw_l3_defip_traverse(int unit, bcm_l3_route_traverse_cb trav_fn,
                      uint32 start, uint32 end)
{
    int rv;
    L3_INIT(unit);

    if (start > end) {
        return (BCM_E_PARAM);
    }

    L3_LOCK(unit);

    rv  = mbcm_driver[unit]->mbcm_ip4_defip_traverse(unit,
                                     start, end, trav_fn, NULL);
    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_route_age
 * Purpose:
 *      Age on DEFIP routing table, clear HIT bit of entry if set
 * Parameters:
 *      unit       - (IN) BCM device unit number.
 *      age_out    - (IN) Call back routine.
 *      user_data  - (IN) User provided cookie for callback.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_route_age(int unit, uint32 flags, bcm_l3_route_traverse_cb age_out,
                     void *user_data)
{
    int rv;
    L3_INIT(unit);

    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_defip_age(unit, age_out, user_data);

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *    bcm_esw_l3_route_max_ecmp_set
 * Purpose:
 *    Set the maximum ECMP paths allowed for a route
 *    for chips that supports ECMP
 * Parameters:
 *    unit - SOC device unit number
 *    max  - MAX number of paths for ECMP (number must be power of 2)
 * Returns:
 *      BCM_E_XXX
 * Note:
 *    There is a hardware limit on the number of ECMP paths,
 *    for bcm_ip_mask_length it is 32, BCM5665/50/73/74 is 8.
 *    The above parameter must be within the hardware limit.
 *    This function can be called before ECMP routes are added,
 *    normally at the beginning.  Once ECMP routes exist, cannot be reset.
 */
int
bcm_esw_l3_route_max_ecmp_set(int unit, int max)
{
    int rv = BCM_E_UNAVAIL;
    L3_INIT(unit);
    L3_LOCK(unit);

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        rv = bcm_xgs3_max_ecmp_set(unit, max);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *    bcm_esw_l3_route_max_ecmp_get
 * Purpose:
 *    Get the maximum ECMP paths allowed for a route
 *    for chips that supports ECMP
 * Parameters:
 *    unit - SOC device unit number
 *    max  - (OUT)MAX number of paths for ECMP (number must be power of 2)
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_route_max_ecmp_get(int unit, int *max)
{
    int rv = BCM_E_UNAVAIL;
    L3_INIT(unit);

    if (NULL == max) {
        return (BCM_E_PARAM);
    }

    *max = 0;

    L3_LOCK(unit);
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        rv = bcm_xgs3_max_ecmp_get(unit, max);
    }
#endif   /* BCM_XGS3_SWITCH_SUPPORT */
    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_l3_cleanup
 * Purpose:
 *      Detach L3 function for a unit, clean up all tables and data structures
 * Parameters:
 *      unit - StrataSwitch Unit #.
 * Returns:
 *      BCM_E_NONE - detach successful.
 *      BCM_E_XXX - detach failed.
 */

int
bcm_esw_l3_cleanup(int unit)
{
    int rv = BCM_E_NONE;

    if ((0 == l3_internal_initialized) || 
        (0 == _bcm_l3_bk_info[unit].l3_initialized)) {
        if (NULL != mbcm_driver[unit]) {
            rv = mbcm_driver[unit]->mbcm_l3_tables_cleanup(unit);
        }
        return rv;
    }

    L3_LOCK(unit);
    rv = bcm_esw_ipmc_detach(unit);
    if (BCM_FAILURE(rv)) {
        L3_UNLOCK(unit);
        return (rv);
    }
    
    /*no subport feature in Hurricane*/
    if(!(SOC_IS_HURRICANEX(unit)||SOC_IS_GREYHOUND(unit))) {
        rv = bcm_esw_subport_cleanup(unit);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }
    }/*SOC_IS_HURRICANE*/

    rv = bcm_esw_mpls_cleanup(unit);
    if (BCM_FAILURE(rv)) {
        L3_UNLOCK(unit);
        return (rv);
    }

    if(!(SOC_IS_HURRICANEX(unit)||SOC_IS_GREYHOUND(unit))) {
        rv = bcm_esw_wlan_detach(unit);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }
    }/*SOC_IS_HURRICANE*/

    if(!(SOC_IS_HURRICANEX(unit)||SOC_IS_GREYHOUND(unit))) {
        rv = bcm_esw_mim_detach(unit);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }
    }

    if (0 == SOC_HW_ACCESS_DISABLE(unit)) {
        rv = bcm_esw_l3_route_delete_all(unit, NULL);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }
        rv = bcm_esw_l3_host_delete_all(unit, NULL);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }
        rv = bcm_esw_l3_intf_delete_all(unit);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }
    }

    rv = mbcm_driver[unit]->mbcm_l3_tables_cleanup(unit);
    _bcm_l3_bk_info[unit].l3_initialized = 0;
    L3_UNLOCK(unit);

    return(rv);
}

/*
 * Function:
 *      bcm_esw_l3_enable_set
 * Purpose:
 *      Enable/disable L3 function for a unit without clearing any L3 tables
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      enable - 1 for enable, 0 for disable
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_esw_l3_enable_set(int unit, int enable)
{
    L3_INIT(unit);
    L3_LOCK(unit);

    mbcm_driver[unit]->mbcm_l3_enable(unit, enable);

    L3_UNLOCK(unit);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_l3_info
 * Purpose:
 *      Get the status of hardware.
 * Parameters:
 *      unit - SOC device unit number
 *      l3_info - (OUT) Available hardware's L3 information.
 * Returns:
 *      BCM_E_XXX.
 */

int
bcm_esw_l3_info(int unit, bcm_l3_info_t *l3info)
{
    int rv;
    L3_INIT(unit);

    if (l3info == NULL) {
        return (BCM_E_PARAM);
    }
    L3_LOCK(unit);

    rv = mbcm_driver[unit]->mbcm_l3_info_get(unit, l3info);

    L3_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_ingress_create
 * Purpose:
 *      Create an Ingress Interface object.
 * Parameters:
 *      unit    - (IN)  bcm device.
 *      flags   - (IN)  BCM_L3_INGRESS_REPLACE: replace existing.
 *                          BCM_L3_INGRESS_WITH_ID: intf argument is given.
 *                          BCM_L3_INGRESS_GLOBAL_ROUTE : 
 *                          BCM_L3_INGRESS_DSCP_TRUST : 
 *      ing_intf     - (IN) Ingress Interface information.
 *      intf_id    - (OUT) L3 Ingress interface id pointing to Ingress object.
 *                      This is an IN argument if either BCM_L3_INGRESS_REPLACE
 *                      or BCM_L3_INGRESS_WITH_ID are given in flags.
 * Returns:
 *      BCM_E_XXX
*/

int 
bcm_esw_l3_ingress_create(int unit, bcm_l3_ingress_t *ing_intf, bcm_if_t *intf_id)
{
    int rv = BCM_E_UNAVAIL;
#if defined (BCM_TRIUMPH_SUPPORT)
    if (soc_feature(unit,soc_feature_l3_ingress_interface)) {
         L3_LOCK(unit);
         rv =  bcm_xgs3_l3_ingress_create(unit, ing_intf, intf_id);
         L3_UNLOCK(unit);
    }
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_ingress_destroy
 * Purpose:
 *      Destroy an Ingress Interface object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      intf_id    - (IN) L3 Ingress interface id pointing to Ingress object.
 * Returns:
 *      BCM_E_XXX
 */

int 
bcm_esw_l3_ingress_destroy(int unit, bcm_if_t intf_id)
{
    int rv = BCM_E_UNAVAIL;
#if defined (BCM_TRIUMPH_SUPPORT)
    if (soc_feature(unit,soc_feature_l3_ingress_interface)) {
         L3_LOCK(unit);
         rv =  bcm_xgs3_l3_ingress_destroy(unit, intf_id);
         L3_UNLOCK(unit);
}
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_ingress_find
 * Purpose:
 *      Find an Ingress Interface object.     
 * Parameters:
 *      unit       - (IN) bcm device.
 *      ing_intf        - (IN) Ingress Interface information.
 *      intf_id       - (OUT) L3 Ingress interface id pointing to Ingress object.
 * Returns:
 *      BCM_E_XXX
 */

int 
bcm_esw_l3_ingress_find(int unit, bcm_l3_ingress_t *ing_intf, bcm_if_t *intf_id)
{
    int rv = BCM_E_UNAVAIL;
#if defined (BCM_TRIUMPH_SUPPORT)
    if (soc_feature(unit,soc_feature_l3_ingress_interface)) {
         L3_LOCK(unit);
         rv =  bcm_xgs3_l3_ingress_find(unit, ing_intf, intf_id);
         L3_UNLOCK(unit);
    }
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_ingress_get
 * Purpose:
 *      Get an Ingress Interface object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      intf_id    - (IN) L3 Ingress interface id pointing to Ingress object.
 *      ing_intf  - (OUT) Ingress Interface information.
 * Returns:
 *      BCM_E_XXX
 */

int 
bcm_esw_l3_ingress_get(int unit, bcm_if_t intf_id, bcm_l3_ingress_t *ing_intf)
{
    int rv = BCM_E_UNAVAIL;
#if defined (BCM_TRIUMPH_SUPPORT)
    if (soc_feature(unit,soc_feature_l3_ingress_interface)) {
         L3_LOCK(unit);
         rv =  bcm_xgs3_l3_ingress_get(unit, intf_id, ing_intf);
         L3_UNLOCK(unit);
}
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_ingress_traverse
 * Purpose:
 *      Goes through ijgress interface objects table and runs the user callback
 *      function at each valid ingress objects entry passing back the
 *      information for that object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      trav_fn    - (IN) Callback function. 
 *      user_data  - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_ingress_traverse(int unit, 
                           bcm_l3_ingress_traverse_cb trav_fn, void *user_data)
{
    int rv = BCM_E_UNAVAIL;
#if defined (BCM_TRIUMPH_SUPPORT)
    if (soc_feature(unit,soc_feature_l3_ingress_interface)) {
         L3_LOCK(unit);
         rv =  bcm_xgs3_l3_ingress_traverse(unit, trav_fn, user_data);
         L3_UNLOCK(unit);
}
#endif
    return rv;
}
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_egress_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given egress interface.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static 
bcm_error_t _bcm_esw_l3_egress_stat_get_table_info(
           int                        unit,
           bcm_if_t                   intf_id,
           uint32                     *num_of_tables,
           bcm_stat_flex_table_info_t *table_info)
{
    bcm_l3_egress_t egr_intf={0};
    int index=0;

    (*num_of_tables) = 0;
    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return BCM_E_UNAVAIL;
    }
    BCM_IF_ERROR_RETURN(bcm_esw_l3_egress_get(unit, intf_id, &egr_intf));
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_wlan) && egr_intf.encap_id > 0) {
        index = intf_id - BCM_XGS3_DVP_EGRESS_IDX_MIN ;
    } else
#endif
        {
           index = intf_id - BCM_XGS3_EGRESS_IDX_MIN ;
        }
    table_info[*num_of_tables].table=EGR_L3_NEXT_HOPm;
    table_info[*num_of_tables].index=index;
    table_info[*num_of_tables].direction=bcmStatFlexDirectionEgress;
    (*num_of_tables)++;
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_l3_ingress_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given ingress interface.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a ingress L3 object.
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static 
bcm_error_t _bcm_esw_l3_ingress_stat_get_table_info(
            int                        unit,
            bcm_if_t                   intf_id,
            uint32                     *num_of_tables,
            bcm_stat_flex_table_info_t *table_info)
{
    bcm_l3_ingress_t ing_intf={0 };
    bcm_l3_intf_t    l3_intf={0};

    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return BCM_E_UNAVAIL;
    }
    if (intf_id > BCM_VLAN_MAX) {
        BCM_IF_ERROR_RETURN(bcm_esw_l3_ingress_get(unit, intf_id, &ing_intf));
        /* Although above check should return error for invalid intf_id */
#ifdef BCM_XGS3_SWITCH_SUPPORT
        if (!SHR_BITGET(BCM_XGS3_L3_ING_IF_INUSE(unit), intf_id)) {
            return BCM_E_NOT_FOUND;
        }
#endif
    } else {
        /* Check whether it is valid vlan or not */
        l3_intf.l3a_vid=intf_id;
        BCM_IF_ERROR_RETURN(bcm_esw_l3_intf_find_vlan(unit,&l3_intf));
    }
    table_info[*num_of_tables].table=L3_IIFm;
    table_info[*num_of_tables].index=intf_id;
    table_info[*num_of_tables].direction=bcmStatFlexDirectionIngress;
    (*num_of_tables)++;
    return BCM_E_NONE;
}
#endif

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
STATIC _bcm_flex_stat_t
_bcm_esw_l3_stat_to_flex_stat(bcm_l3_stat_t stat)
{
    _bcm_flex_stat_t flex_stat;

    switch (stat) {
    case bcmL3StatInPackets:
        flex_stat = _bcmFlexStatIngressPackets;
        break;
    case bcmL3StatInBytes:
        flex_stat = _bcmFlexStatIngressBytes;
        break;
    case bcmL3StatOutPackets:
        flex_stat = _bcmFlexStatEgressPackets;
        break;
    case bcmL3StatOutBytes:
        flex_stat = _bcmFlexStatEgressBytes;
        break;
    default:
        flex_stat = _bcmFlexStatNum;
    }

    return flex_stat;
}
#endif

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_egress_stat_attach
 * Description:
 *      Attach counters entries to the given L3 Egress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC bcm_error_t _bcm_esw_l3_egress_stat_attach(
            int      unit, 
            bcm_if_t intf_id, 
            uint32   stat_counter_id)
{
    soc_mem_t                  table=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     pool_number=0;
    uint32                     base_index=0;
    bcm_stat_flex_mode_t       offset_mode=0;
    bcm_stat_object_t          object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t      group_mode= bcmStatGroupModeSingle;
    uint32                     count=0;
    uint32                     actual_num_tables=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    _bcm_esw_stat_get_counter_id_info(
                  stat_counter_id,
                  &group_mode,&object,&offset_mode,&pool_number,&base_index);
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit,group_mode));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_table_info(
                        unit,object,1,&actual_num_tables,&table,&direction));

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_egress_stat_get_table_info(
                        unit, intf_id,&num_of_tables,&table_info[0]));
    for (count=0; count < num_of_tables ; count++) {
         if ((table_info[count].direction == direction) &&
             (table_info[count].table == table) ) {
              if (direction == bcmStatFlexDirectionIngress) {
                  return _bcm_esw_stat_flex_attach_ingress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
              } else {
                  return _bcm_esw_stat_flex_attach_egress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
              } 
         }
    }
    return BCM_E_NOT_FOUND;
}
#endif

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
STATIC int
_bcm_esw_l3_egr_flex_stat_hw_index_set(int unit, _bcm_flex_stat_handle_t fsh,
                                     int fs_idx, void *cookie) 
{
    bcm_if_t  intf_id;
    int index;
    egr_l3_next_hop_entry_t egr_nh;
    int rv;
    uint32 handle = _BCM_FLEX_STAT_HANDLE_WORD_GET(fsh, 0);   
    bcm_l3_egress_t egr_intf={0};
    int entry_type;
    soc_mem_info_t *memp;
    soc_field_t ctr_field;
    soc_field_t use_ctr_field;

    intf_id = (bcm_if_t)handle;
    rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(bcm_esw_l3_egress_get(unit, intf_id, &egr_intf));

    if (soc_feature(unit, soc_feature_wlan) && egr_intf.encap_id > 0) {
        index = intf_id - BCM_XGS3_DVP_EGRESS_IDX_MIN;
    } else {
        index = intf_id - BCM_XGS3_EGRESS_IDX_MIN;
    }

    rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY,index,&egr_nh);

    if (BCM_SUCCESS(rv)) {
        entry_type = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                           ENTRY_TYPEf);
        memp = &SOC_MEM_INFO(unit, EGR_L3_NEXT_HOPm);
        if (memp->views != NULL) {
            if (sal_strcmp(memp->views[entry_type],"L3") == 0) {
                ctr_field = L3__VINTF_CTR_IDXf;
                use_ctr_field = L3__USE_VINTF_CTR_IDXf; 
            } else if (sal_strcmp(memp->views[entry_type],"PROXY") == 0) {
                ctr_field = PROXY__VINTF_CTR_IDXf;
                use_ctr_field = PROXY__USE_VINTF_CTR_IDXf; 
            } else if (sal_strcmp(memp->views[entry_type],"MPLS") == 0) {
                ctr_field = MPLS__VINTF_CTR_IDXf;
                use_ctr_field = MPLS__USE_VINTF_CTR_IDXf; 
            } else if (sal_strcmp(memp->views[entry_type],"SD_TAG") == 0) {
                ctr_field = SD_TAG__VINTF_CTR_IDXf;
                use_ctr_field = SD_TAG__USE_VINTF_CTR_IDXf; 
            } else if (sal_strcmp(memp->views[entry_type],"MIM") == 0) {
                ctr_field = MIM__VINTF_CTR_IDXf;
                use_ctr_field = MIM__USE_VINTF_CTR_IDXf; 
            } else {
                return BCM_E_UNAVAIL;
            }
        } else {
            ctr_field = L3__VINTF_CTR_IDXf;
            use_ctr_field = L3__USE_VINTF_CTR_IDXf; 
        }

        if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, use_ctr_field)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            use_ctr_field, fs_idx > 0 ? 1 : 0);
        }
        if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, ctr_field)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            ctr_field, fs_idx);
        }
        rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ALL, index,
                                   &egr_nh);
    }
    return rv;
}

STATIC int
_bcm_esw_l3_ingr_flex_stat_hw_index_set(int unit, _bcm_flex_stat_handle_t fsh,
                                     int fs_idx, void *cookie)
{
    int index;
    int rv;
    uint32 handle = _BCM_FLEX_STAT_HANDLE_WORD_GET(fsh, 0);
    iif_entry_t       l3iif_entry;

    index = (int)handle;  /* interface_id */
    rv = BCM_E_NONE;

    rv = soc_mem_read(unit, L3_IIFm, MEM_BLOCK_ANY,index,&l3iif_entry);

    if (BCM_SUCCESS(rv)) {
        if (soc_mem_field_valid(unit, L3_IIFm,
                                    USE_VINTF_CTR_IDXf)) {
            soc_mem_field32_set(unit, L3_IIFm, &l3iif_entry,
                            USE_VINTF_CTR_IDXf,
                            fs_idx > 0 ? 1 : 0);
        }
        soc_mem_field32_set(unit, L3_IIFm, &l3iif_entry,
                            VINTF_CTR_IDXf, fs_idx);
        rv = soc_mem_write(unit, L3_IIFm, MEM_BLOCK_ALL, index,
                                   &l3iif_entry);
    }
    return rv;
}

#endif

/*
 * Function:
 *      bcm_esw_l3_egress_stat_attach
 * Description:
 *      Attach counters entries to the given L3 Egress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_egress_stat_attach(
            int      unit,
            bcm_if_t intf_id,
            uint32   stat_counter_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_egress_stat_attach(unit,intf_id,stat_counter_id);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        _bcm_flex_stat_type_t fs_type;
        uint32 fs_inx;
        uint32 flag;
        int rv;

        fs_type = _BCM_FLEX_STAT_TYPE(stat_counter_id);
        fs_inx  = _BCM_FLEX_STAT_COUNT_INX(stat_counter_id);

        if (!((fs_type == _bcmFlexStatTypeEgressGport) && fs_inx)) {
            return BCM_E_PARAM;
        }

        flag = _BCM_FLEX_STAT_HW_EGRESS;

        rv = _bcm_esw_flex_stat_enable_set(unit, fs_type,
                             _bcm_esw_l3_egr_flex_stat_hw_index_set,
                                      INT_TO_PTR(flag),
                                      intf_id, TRUE,fs_inx);
        return rv;
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_egress_stat_detach
 * Description:
 *      Detach  counters entries to the given L3 Egress interface. 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC
bcm_error_t _bcm_esw_l3_egress_stat_detach(
            int      unit, 
            bcm_if_t intf_id)
{
    uint32                     count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_error_t                rv[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = 
                                  {BCM_E_FAIL};
    uint32                     flag[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = {0};

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_egress_stat_get_table_info(
                   unit, intf_id,&num_of_tables,&table_info[0]));

    for (count=0; count < num_of_tables ; count++) {
         if (table_info[count].direction == bcmStatFlexDirectionIngress) {
             rv[bcmStatFlexDirectionIngress]= 
                   _bcm_esw_stat_flex_detach_ingress_table_counters(
                        unit, table_info[count].table,table_info[count].index);
             flag[bcmStatFlexDirectionIngress] = 1;
         } else {
             rv[bcmStatFlexDirectionEgress] = 
                   _bcm_esw_stat_flex_detach_egress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
             flag[bcmStatFlexDirectionIngress] = 1;
         }
    }
    if ((rv[bcmStatFlexDirectionIngress] == BCM_E_NONE) ||
        (rv[bcmStatFlexDirectionEgress] == BCM_E_NONE)) {
         return BCM_E_NONE;
    }
    if (flag[bcmStatFlexDirectionIngress] == 1) {
        return rv[bcmStatFlexDirectionIngress];
    }
    if (flag[bcmStatFlexDirectionEgress] == 1) {
        return rv[bcmStatFlexDirectionEgress];
    }
    return BCM_E_NOT_FOUND;
}
#endif

/*
 * Function:
 *      bcm_esw_l3_egress_stat_detach
 * Description:
 *      Detach  counters entries to the given L3 Egress interface.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_egress_stat_detach(
            int      unit,
            bcm_if_t intf_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_egress_stat_detach(unit,intf_id);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint32 flag;
        int rv;

        flag = _BCM_FLEX_STAT_HW_EGRESS;

        rv = _bcm_esw_flex_stat_enable_set(unit, _bcmFlexStatTypeEgressGport,
                             _bcm_esw_l3_egr_flex_stat_hw_index_set,
                                      INT_TO_PTR(flag),
                                      intf_id, FALSE,1);
        return rv;
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_egress_stat_counter_get
 * Description:
 *      Get the specified counter statistic for a L3 egress interface 
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC
bcm_error_t _bcm_esw_l3_egress_stat_counter_get(int              unit, 
                                                int              sync_mode, 
                                                bcm_if_t         intf_id, 
                                                bcm_l3_stat_t    stat, 
                                                uint32           num_entries, 
                                                uint32           *counter_indexes, 
                                                bcm_stat_value_t *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    if ((stat == bcmL3StatOutPackets) ||
        (stat == bcmL3StatOutBytes)) {
         direction = bcmStatFlexDirectionEgress;
    } else {
         /* direction = bcmStatFlexDirectionIngress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3StatOutPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
    
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_egress_stat_get_table_info(
                        unit, intf_id,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count=0; index_count < num_entries ; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_get(
                                      unit, sync_mode,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }
    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      _bcm_esw_l3_egress_counter_get
 * Description:
 *      Get the specified counter statistic for a L3 egress interface
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int _bcm_esw_l3_egress_counter_get(int              unit,
                                   int              sync_mode,
                                   bcm_if_t         intf_id,
                                   bcm_l3_stat_t    stat,
                                   uint32           num_entries,
                                   uint32           *counter_indexes,
                                   bcm_stat_value_t *counter_values)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_egress_stat_counter_get(unit, sync_mode, 
                                                   intf_id, 
                                                   stat, num_entries, 
                                                   counter_indexes,
                                                   counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint64 val;
        int rv = BCM_E_NONE;

        if (!(stat == bcmL3StatOutPackets ||
              stat == bcmL3StatOutBytes)) {
            return BCM_E_PARAM;
        }   
        rv =  _bcm_esw_flex_stat_get(unit,
                                     sync_mode,
                                     _bcmFlexStatTypeEgressGport,
                                     intf_id,
                                     _bcm_esw_l3_stat_to_flex_stat(stat), 
                                     &val);

        if (stat == bcmL3StatOutPackets) {
            counter_values->packets = COMPILER_64_LO(val);
        } else {
            COMPILER_64_SET(counter_values->bytes,
                      COMPILER_64_HI(val),
                      COMPILER_64_LO(val));
        }
        return rv;

    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_l3_egress_stat_counter_get
 * Description:
 *      Get the specified counter statistic for a L3 egress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_egress_stat_counter_get(
            int              unit,
            bcm_if_t         intf_id,
            bcm_l3_stat_t    stat,
            uint32           num_entries,
            uint32           *counter_indexes,
            bcm_stat_value_t *counter_values)
{
    return _bcm_esw_l3_egress_counter_get(unit, 0, intf_id, stat,
                                        num_entries, 
                                        counter_indexes,
                                        counter_values);   
}

/*
 * Function:
 *      bcm_esw_l3_egress_stat_counter_sync_get
 * Description:
 *      retrieve set of counter statistics for a L3 egress interface
 *      sw accumulated counters synced with hw count.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_egress_stat_counter_sync_get(int              unit,
                                            bcm_if_t         intf_id,
                                            bcm_l3_stat_t    stat,
                                            uint32           num_entries,
                                            uint32           *counter_indexes,
                                            bcm_stat_value_t *counter_values)
{
    return _bcm_esw_l3_egress_counter_get(unit, 1, intf_id, stat,
                                          num_entries, counter_indexes,
                                          counter_values);   
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_egress_stat_counter_set
 * Description:
 *      Set the specified counter statistic for a L3 egress interface 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (IN) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */
STATIC
bcm_error_t _bcm_esw_l3_egress_stat_counter_set(
            int              unit, 
            bcm_if_t         intf_id, 
            bcm_l3_stat_t    stat, 
            uint32           num_entries, 
            uint32           *counter_indexes, 
            bcm_stat_value_t *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    if ((stat == bcmL3StatOutPackets) ||
        (stat == bcmL3StatOutBytes)) {
         direction = bcmStatFlexDirectionEgress;
    } else {
         /* direction = bcmStatFlexDirectionIngress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3StatOutPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_egress_stat_get_table_info(
                        unit, intf_id,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count=0; index_count < num_entries ; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_set(
                                      unit,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
            }
         }
    }
    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      bcm_esw_l3_egress_stat_counter_set
 * Description:
 *      Set the specified counter statistic for a L3 egress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (IN) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */
int bcm_esw_l3_egress_stat_counter_set(
            int              unit,
            bcm_if_t         intf_id,
            bcm_l3_stat_t    stat,
            uint32           num_entries,
            uint32           *counter_indexes,
            bcm_stat_value_t *counter_values)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_egress_stat_counter_set(unit,intf_id,stat,
                      num_entries,counter_indexes,counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint64 val;
        int rv = BCM_E_NONE;

        if (!(stat == bcmL3StatOutPackets ||
              stat == bcmL3StatOutBytes)) {
            return BCM_E_PARAM;
        }

        if (stat == bcmL3StatOutPackets) {
            COMPILER_64_SET(val,0,counter_values->packets);
        } else {
            COMPILER_64_SET(val,
                      COMPILER_64_HI(counter_values->bytes),
                      COMPILER_64_LO(counter_values->bytes));
        }
        rv =  _bcm_esw_flex_stat_set(unit,
                _bcmFlexStatTypeEgressGport,
                intf_id,
                _bcm_esw_l3_stat_to_flex_stat(stat), val);

       return rv;

    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_l3_egress_stat_id_get
 * Description:
 *      Get stat counter id associated with given egress interface id
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      Stat_counter_id  - (OUT) Stat Counter ID
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_egress_stat_id_get(
            int             unit,
            bcm_if_t        intf_id, 
            bcm_l3_stat_t   stat, 
            uint32          *stat_counter_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                     index=0;
    uint32                     num_stat_counter_ids=0;

    if ((stat == bcmL3StatInPackets) ||
        (stat == bcmL3StatInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         direction = bcmStatFlexDirectionEgress;
    }
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_egress_stat_get_table_info(
                        unit,intf_id,&num_of_tables,&table_info[0]));
    for (index=0; index < num_of_tables ; index++) {
         if (table_info[index].direction == direction)
             return _bcm_esw_stat_flex_get_counter_id(
                                  unit, 1, &table_info[index],
                                  &num_stat_counter_ids,stat_counter_id);
    }
    return BCM_E_NOT_FOUND;
#else
    return BCM_E_UNAVAIL;
#endif
}


#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_ingress_stat_attach
 * Description:
 *      Attach counters entries to the given L3 Ingress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a Ingress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC
bcm_error_t _bcm_esw_l3_ingress_stat_attach(
            int      unit, 
            bcm_if_t intf_id, 
            uint32   stat_counter_id)
{
    soc_mem_t                  table=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     pool_number=0;
    uint32                     base_index=0;
    bcm_stat_flex_mode_t       offset_mode=0;
    bcm_stat_object_t          object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t      group_mode= bcmStatGroupModeSingle;
    uint32                     count=0;
    uint32                     actual_num_tables=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    _bcm_esw_stat_get_counter_id_info(
                  stat_counter_id,
                  &group_mode,&object,&offset_mode,&pool_number,&base_index);
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit,group_mode));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_table_info(
                        unit,object,1,&actual_num_tables,&table,&direction));

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_ingress_stat_get_table_info(
                        unit, intf_id,&num_of_tables,&table_info[0]));
    for (count=0; count < num_of_tables ; count++) {
         if ((table_info[count].direction == direction) &&
             (table_info[count].table == table) ) {
              if (direction == bcmStatFlexDirectionIngress) {
                  return _bcm_esw_stat_flex_attach_ingress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
              } else {
                  return _bcm_esw_stat_flex_attach_egress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
              } 
         }
    }
    return BCM_E_NOT_FOUND;
}
#endif

/*
 * Function:
 *      bcm_esw_l3_ingress_stat_attach
 * Description:
 *      Attach counters entries to the given L3 Ingress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a Ingress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.  *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_ingress_stat_attach(
            int      unit,
            bcm_if_t intf_id,
            uint32   stat_counter_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_ingress_stat_attach(unit,intf_id,stat_counter_id);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        _bcm_flex_stat_type_t fs_type;
        uint32 fs_inx;
        uint32 flag;
        int rv;
        bcm_l3_ingress_t ing_intf={0 };
        bcm_l3_intf_t    l3_intf={0};

        if (intf_id > BCM_VLAN_MAX) {
            BCM_IF_ERROR_RETURN(bcm_esw_l3_ingress_get(unit, intf_id,
                        &ing_intf));
            /* Although above check should return error for invalid intf_id */
#ifdef BCM_XGS3_SWITCH_SUPPORT
            if (!SHR_BITGET(BCM_XGS3_L3_ING_IF_INUSE(unit), intf_id)) {
                return BCM_E_NOT_FOUND;
            }
#endif
        } else {
            /* Check whether it is valid vlan or not */
            l3_intf.l3a_vid=intf_id;
            BCM_IF_ERROR_RETURN(bcm_esw_l3_intf_find_vlan(unit,&l3_intf));
        }

        fs_type = _BCM_FLEX_STAT_TYPE(stat_counter_id);
        fs_inx  = _BCM_FLEX_STAT_COUNT_INX(stat_counter_id);

        if (!((fs_type == _bcmFlexStatTypeGport) && fs_inx)) {
            return BCM_E_PARAM;
        }

        flag = _BCM_FLEX_STAT_HW_INGRESS;

        rv = _bcm_esw_flex_stat_enable_set(unit, fs_type,
                             _bcm_esw_l3_ingr_flex_stat_hw_index_set,
                                      INT_TO_PTR(flag),
                                      intf_id, TRUE,fs_inx);
        return rv;
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }

}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_ingress_stat_detach
 * Description:
 *      Detach  counters entries to the given L3 Ingress interface. 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC
bcm_error_t _bcm_esw_l3_ingress_stat_detach(
            int      unit, 
            bcm_if_t intf_id)
{
    uint32                     count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_error_t                rv[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = 
                                  {BCM_E_FAIL};
    uint32                     flag[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = {0};

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_ingress_stat_get_table_info(
                        unit, intf_id,&num_of_tables,&table_info[0]));

    for (count=0; count < num_of_tables ; count++) {
         if (table_info[count].direction == bcmStatFlexDirectionIngress) {
             rv[bcmStatFlexDirectionIngress]= 
                   _bcm_esw_stat_flex_detach_ingress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
             flag[bcmStatFlexDirectionIngress] = 1;
         } else {
             rv[bcmStatFlexDirectionEgress] = 
                   _bcm_esw_stat_flex_detach_egress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
             flag[bcmStatFlexDirectionIngress] = 1;
         }
    }
    if ((rv[bcmStatFlexDirectionIngress] == BCM_E_NONE) ||
        (rv[bcmStatFlexDirectionEgress] == BCM_E_NONE)) {
         return BCM_E_NONE;
    }
    if (flag[bcmStatFlexDirectionIngress] == 1) {
        return rv[bcmStatFlexDirectionIngress];
    }
    if (flag[bcmStatFlexDirectionEgress] == 1) {
        return rv[bcmStatFlexDirectionEgress];
    }
    return BCM_E_NOT_FOUND;
}
#endif

/*
 * Function:
 *      bcm_esw_l3_ingress_stat_detach
 * Description:
 *      Detach  counters entries to the given L3 Ingress interface.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

int bcm_esw_l3_ingress_stat_detach(
            int      unit,
            bcm_if_t intf_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_ingress_stat_detach(unit,intf_id);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint32 flag;
        int rv;

        flag = _BCM_FLEX_STAT_HW_INGRESS;

        rv = _bcm_esw_flex_stat_enable_set(unit, _bcmFlexStatTypeGport,
                             _bcm_esw_l3_ingr_flex_stat_hw_index_set,
                                      INT_TO_PTR(flag),
                                      intf_id, FALSE,1);
        return rv;
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_ingress_stat_counter_get
 * Description:
 *      Get the specified counter statistic for a L3 ingress interface 
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      intf_id          - (IN) Interface id of a ingress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC
bcm_error_t _bcm_esw_l3_ingress_stat_counter_get(
            int              unit, 
            int              sync_mode,
            bcm_if_t         intf_id, 
            bcm_l3_stat_t    stat, 
            uint32           num_entries, 
            uint32           *counter_indexes, 
            bcm_stat_value_t *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    if ((stat == bcmL3StatInPackets) ||
        (stat == bcmL3StatInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3StatInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
    
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_ingress_stat_get_table_info(
                        unit, intf_id,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
        if (table_info[table_count].direction == direction) {
            for (index_count=0; index_count < num_entries ; index_count++) {
                 BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_get(
                                     unit, sync_mode,
                                     table_info[table_count].index,
                                     table_info[table_count].table,
                                     byte_flag,
                                     counter_indexes[index_count],
                                     &counter_values[index_count]));
            }
         }
    }
    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      _bcm_esw_l3_ingress_flex_stat_counter_get
 * Description:
 *      Get the specified counter statistic for a L3 ingress interface
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count
 *      intf_id          - (IN) Interface id of a ingress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int _bcm_esw_l3_ingress_flex_stat_counter_get(
            int              unit,
            int              sync_mode,
            bcm_if_t         intf_id,
            bcm_l3_stat_t    stat,
            uint32           num_entries,
            uint32           *counter_indexes,
            bcm_stat_value_t *counter_values)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_ingress_stat_counter_get(unit, sync_mode, 
                                                    intf_id, stat,
                                                    num_entries, 
                                                    counter_indexes, 
                                                    counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint64 val;
        int rv = BCM_E_NONE;

        if (!(stat == bcmL3StatInPackets ||
              stat == bcmL3StatInBytes)) {
            return BCM_E_PARAM;
        }
        rv =  _bcm_esw_flex_stat_get(unit, sync_mode,
                                     _bcmFlexStatTypeGport,
                                     intf_id,
                                     _bcm_esw_l3_stat_to_flex_stat(stat),
                                     &val);

        if (stat == bcmL3StatInPackets) {
            counter_values->packets = COMPILER_64_LO(val);
        } else {
            COMPILER_64_SET(counter_values->bytes,
                      COMPILER_64_HI(val),
                      COMPILER_64_LO(val));
        }
        return rv;

    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}


/*
 * Function:
 *      bcm_esw_l3_ingress_stat_counter_get
 * Description:
 *      Get the specified counter statistic for a L3 ingress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a ingress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */
int bcm_esw_l3_ingress_stat_counter_get(
             int              unit,
             bcm_if_t         intf_id,
             bcm_l3_stat_t    stat,
             uint32           num_entries,
             uint32           *counter_indexes,
             bcm_stat_value_t *counter_values)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_ingress_flex_stat_counter_get(unit, 0, intf_id, stat,
                                                   num_entries,
                                                   counter_indexes, 
                                                   counter_values);

    return rv;
}



/*
 * Function:
 *      bcm_esw_l3_ingress_stat_counter_sync_get
 * Description:
 *      Get the specified counter statisitic for a L3 ingress interface
 *      sw accumulated counters synced with hw count.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a ingress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */
int bcm_esw_l3_ingress_stat_counter_sync_get(
             int              unit,
             bcm_if_t         intf_id,
             bcm_l3_stat_t    stat,
             uint32           num_entries,
             uint32           *counter_indexes,
             bcm_stat_value_t *counter_values)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_ingress_flex_stat_counter_get(unit, 1, intf_id, stat,
                                                   num_entries,
                                                   counter_indexes, 
                                                   counter_values);

    return rv;
}


#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_ingress_stat_counter_set
 * Description:
 *      Set the specified counter statistic for a L3 ingress interface 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (IN) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */

STATIC
bcm_error_t _bcm_esw_l3_ingress_stat_counter_set(
            int              unit, 
            bcm_if_t         intf_id, 
            bcm_l3_stat_t    stat, 
            uint32           num_entries, 
            uint32           *counter_indexes, 
            bcm_stat_value_t *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    if ((stat == bcmL3StatInPackets) ||
        (stat == bcmL3StatInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3StatInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_ingress_stat_get_table_info(
                        unit, intf_id,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count=0; index_count < num_entries ; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_set(
                                      unit,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }
    return BCM_E_NONE;
}
#endif

#ifdef BCM_KATANA_SUPPORT
/*
 * Function:
 *      _bcm_kt_extended_queue_config
 * Description:
 *      Set the cosq id and the profile index for subscriber port in 
 *      ING_L3_NEXT_HOP/ING_QUEUE_MAP
 * Parameters:
 *      unit             - (IN) unit number
 *      flags            - (IN) Flag to indicate EH_TAG_TYPE 
 *      queue_id         - (IN) CosQ id derived from gport 
 *      profile_index    - (IN) value ING_QUEUE_OFFSET_MAPPING_TABLE profile 
 *      intf             - (IN) L3 interface id pointing to Egress object.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */

STATIC
int _bcm_kt_extended_queue_config(int unit, uint32 flags, 
                      int queue_id, int profile_index, bcm_if_t *intf) 
{ 
    int idx = 0;
    ing_queue_map_entry_t ing_queue_entry;
    ing_l3_next_hop_entry_t in_entry;
    int old_queue_id = 0;
    int eh_tag_type = 1;   /*EH_QUEUE_TAG contains actual COSQ value */
    int mem = ING_L3_NEXT_HOPm;
    if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, *intf)) {
        idx = *intf - BCM_XGS3_EGRESS_IDX_MIN;
    } else {
        idx = *intf - BCM_XGS3_DVP_EGRESS_IDX_MIN;
    }
    sal_memset(&in_entry, 0, BCM_XGS3_L3_ENT_SZ(unit, nh));
    /* Read ingress next hop entry. */
    BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_READ(unit, mem, idx, &in_entry));
    if (flags & BCM_L3_QUEUE_MAP) {
        /*  ING_QUEUE_MAP needs to be configured */          
        sal_memset(&ing_queue_entry, 0, sizeof(ing_queue_map_entry_t));
        soc_mem_field32_set(unit, ING_QUEUE_MAPm, 
                    &ing_queue_entry, QUEUE_SET_BASEf, queue_id);
        soc_mem_field32_set(unit, ING_QUEUE_MAPm, &ing_queue_entry, 
                             QUEUE_OFFSET_PROFILE_INDEXf, profile_index);
        BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_WRITE(unit, ING_QUEUE_MAPm, 
                                            queue_id, &ing_queue_entry));
        eh_tag_type = 2; /* Get BASE QUEUE from ING_QUEUE_MAP */
    }
    if ((flags & BCM_L3_REPLACE) && (0 == queue_id)) {
        if (SOC_MEM_FIELD_VALID(unit, mem, EH_QUEUE_TAGf)) {
            old_queue_id = soc_mem_field32_get(unit, mem, &in_entry,
                                             EH_QUEUE_TAGf);
            if (old_queue_id > 0) {
            /*  ING_QUEUE_MAP needs to be reset */
                sal_memset(&ing_queue_entry, 0, sizeof(ing_queue_map_entry_t));
                BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_WRITE(unit, ING_QUEUE_MAPm,
                                          old_queue_id, &ing_queue_entry));
            }
        }
        eh_tag_type = 0;  /* No HQOS */
    }
    if (SOC_MEM_FIELD_VALID(unit, mem, EH_TAG_TYPEf)) {
        soc_mem_field32_set(unit, mem, &in_entry, EH_TAG_TYPEf,
                            eh_tag_type & 0x3);
    }
    if (SOC_MEM_FIELD_VALID(unit, mem, EH_QUEUE_TAGf)) {
         soc_mem_field32_set(unit, mem, &in_entry, EH_QUEUE_TAGf, queue_id);
    }
    /* Write ingress next hop entry. */
    BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_WRITE(unit, mem, idx, &in_entry));
    return BCM_E_NONE;
}
#endif /* BCM_KATANA_SUPPORT */

/*
 * Function:
 *      bcm_esw_l3_ingress_stat_counter_set
 * Description:
 *      Set the specified counter statistic for a L3 ingress interface
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a egress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (IN) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */

int bcm_esw_l3_ingress_stat_counter_set(
            int              unit,
            bcm_if_t         intf_id,
            bcm_l3_stat_t    stat,
            uint32           num_entries,
            uint32           *counter_indexes,
            bcm_stat_value_t *counter_values)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_ingress_stat_counter_set(unit,intf_id,stat,
                      num_entries,counter_indexes,counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint64 val;
        int rv = BCM_E_NONE;

        if (!(stat == bcmL3StatInPackets ||
              stat == bcmL3StatInBytes)) {
            return BCM_E_PARAM;
        }

        if (stat == bcmL3StatInPackets) {
            COMPILER_64_SET(val,0,counter_values->packets);
        } else {
            COMPILER_64_SET(val,
                      COMPILER_64_HI(counter_values->bytes),
                      COMPILER_64_LO(counter_values->bytes));
        }
        rv =  _bcm_esw_flex_stat_set(unit,
                _bcmFlexStatTypeGport,
                intf_id,
                _bcm_esw_l3_stat_to_flex_stat(stat), val);
       return rv;
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_l3_egress_stat_id_get
 * Description:
 *      Get stat counter id associated with given ingress interface id
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      intf_id          - (IN) Interface id of a ingress L3 object.
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      Stat_counter_id  - (OUT) Stat Counter ID
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_ingress_stat_id_get(
            int             unit,
            bcm_if_t        intf_id,
            bcm_l3_stat_t stat, 
            uint32          *stat_counter_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                     index=0;
    uint32                     num_stat_counter_ids=0;

    if ((stat == bcmL3StatInPackets) ||
        (stat == bcmL3StatInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         direction = bcmStatFlexDirectionEgress;
    }
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_ingress_stat_get_table_info(
                        unit,intf_id,&num_of_tables,&table_info[0]));
    for (index=0; index < num_of_tables ; index++) {
         if (table_info[index].direction == direction)
             return _bcm_esw_stat_flex_get_counter_id(
                                  unit, 1, &table_info[index],
                                  &num_stat_counter_ids,stat_counter_id);
    }
    return BCM_E_NOT_FOUND;
#else
    return BCM_E_UNAVAIL;
#endif
}




/*
 * Function:
 *      bcm_esw_l3_egress_create
 * Purpose:
 *      Create an Egress forwarding object.
 * Parameters:
 *      unit    - (IN)  bcm device.
 *      flags   - (IN)  BCM_L3_REPLACE: replace existing.
 *                      BCM_L3_WITH_ID: intf argument is given.
 *      egr     - (IN) Egress forwarding destination.
 *      intf    - (OUT) L3 interface id pointing to Egress object.
 *                      This is an IN argument if either BCM_L3_REPLACE
 *                      or BCM_L3_WITH_ID are given in flags.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_create(int unit, uint32 flags, bcm_l3_egress_t *egr, 
                         bcm_if_t *intf)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_KATANA_SUPPORT
    int extended_queue_id = 0;
#endif
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        bcm_l3_egress_t egr_local;
        int vp_routing = FALSE;

        /* Input parameters check. */
        if ((NULL == egr) || (NULL == intf)) {
            return (BCM_E_PARAM);
        }

        /* Copy provided structure to local so it can be modified. */
        sal_memcpy(&egr_local, egr, sizeof(bcm_l3_egress_t));

        if (egr_local.vlan >= BCM_VLAN_INVALID) {
            return (BCM_E_PARAM);
        }
#ifdef BCM_KATANA_SUPPORT        
        if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(egr_local.port)) ||
            (BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(egr_local.port))) {
            if ((((egr_local.qos_map_id >> _BCM_QOS_MAP_SHIFT) !=
                       _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE)) ||
                 ((egr_local.qos_map_id & _BCM_QOS_MAP_TYPE_MASK) > 
                                     _BCM_QOS_MAP_ING_QUEUE_OFFSET_MAX)) {
                return (BCM_E_PARAM);
            }
        }
#endif
        if (BCM_GPORT_IS_BLACK_HOLE(egr_local.port)) {
             return BCM_E_PARAM;
        } else if (BCM_GPORT_IS_SET(egr_local.port)) {
            if (BCM_GPORT_IS_NIV_PORT(egr_local.port) ||
                BCM_GPORT_IS_EXTENDER_PORT(egr_local.port) ||
                BCM_GPORT_IS_VLAN_PORT(egr_local.port)) {
#ifdef BCM_TRIDENT2_SUPPORT
                if (soc_feature(unit, soc_feature_virtual_port_routing)) {
                    vp_routing = TRUE;
                } else
#endif /* BCM_TRIDENT2_SUPPORT */
                {
                    return BCM_E_UNAVAIL;
                }
            }
            if (BCM_GPORT_IS_WLAN_PORT(egr_local.port) || (vp_routing)) {
                /* Resolve the gport and store the VP in egr->encap_id */
                rv = _bcm_esw_gport_resolve(unit, egr_local.port, 
                                            &(egr_local.module), 
                                            &(egr_local.port), 
                                            &(egr_local.trunk), 
                                            &(egr_local.encap_id));  
                if (-1 != egr_local.trunk) {
                    egr_local.flags |= BCM_L3_TGID;
                }
            } else {
                rv = _bcm_esw_l3_gport_resolve(unit, egr_local.port, 
                                               &(egr_local.port), 
                                               &(egr_local.module), 
                                               &(egr_local.trunk), 
                                               &(egr_local.flags));
            }
            BCM_IF_ERROR_RETURN(rv);
        } else {
            PORT_DUALMODID_VALID(unit, egr_local.port);
        }
		
        if((egr_local.flags & BCM_L3_IPMC) && 
            !BCM_MAC_IS_ZERO(egr_local.mac_addr)) {
            if (!soc_feature(unit, soc_feature_ipmc_use_configured_dest_mac)) {
                return BCM_E_CONFIG;
            }
        }
		
        L3_LOCK(unit);
        if (soc_feature(unit, soc_feature_mpls)) {
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
            rv = bcm_tr_mpls_lock(unit);
            if ((rv != BCM_E_INIT) && BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */
        }
#ifdef BCM_KATANA_SUPPORT
        if (soc_feature(unit, soc_feature_extended_queueing)) {
            if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(egr->port)) ||
                (BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(egr->port))) {
                extended_queue_id = egr_local.trunk;
                egr_local.trunk = 0;
            }
        }
#endif
        rv = bcm_xgs3_l3_egress_create(unit, flags, &egr_local, intf);
        if (soc_feature(unit, soc_feature_mpls)) {
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
            bcm_tr_mpls_unlock(unit);
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */
        }
#ifdef BCM_KATANA_SUPPORT
        if (BCM_SUCCESS(rv)) {
            if (soc_feature(unit, soc_feature_extended_queueing)) {
                if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(egr->port)) ||
                    (BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(egr->port))) {
                    rv = _bcm_kt_extended_queue_config(unit, egr_local.flags, 
                                                       extended_queue_id, 
                          (egr_local.qos_map_id & _BCM_QOS_MAP_TYPE_MASK), intf); 
                } else {
                    /* Reset extended queue configuration, if any */
                    if (egr_local.flags & BCM_L3_REPLACE) {
                        rv = _bcm_kt_extended_queue_config(unit, egr_local.flags,
                                                           0, 0, intf);
                    }
                }
            }
        }
#endif
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_egress_destroy
 * Purpose:
 *      Destroy an Egress forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      intf    - (IN) L3 interface id pointing to Egress object.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_destroy(int unit, bcm_if_t intf)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        if (soc_feature(unit, soc_feature_mpls)) {
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
           rv =  bcm_tr_mpls_lock(unit);
            if ((rv != BCM_E_INIT) && BCM_FAILURE(rv)) {
               L3_UNLOCK(unit);
               return rv;
           }
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */
        }
         rv = bcm_xgs3_l3_egress_destroy(unit, intf);
         if (soc_feature(unit, soc_feature_mpls)) {
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
             bcm_tr_mpls_unlock(unit);
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */
         }
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_get
 * Purpose:
 *      Get an Egress forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      intf    - (IN) L3 interface id pointing to Egress object.
 *      egr     - (OUT) Egress forwarding destination.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_get (int unit, bcm_if_t intf, bcm_l3_egress_t *egr) 
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)

    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_get (unit, intf, egr);
        L3_UNLOCK(unit);
        if (BCM_FAILURE(rv)) {
            return rv;
        }

#ifdef BCM_TRX_SUPPORT
        if (egr->encap_id > 0 && egr->encap_id < BCM_XGS3_EGRESS_IDX_MIN) {
            /* encap_id contains a virtual port value */
            int vp;
            vp = egr->encap_id;
            if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv)) {
                BCM_GPORT_NIV_PORT_ID_SET(egr->port, vp);
            } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                BCM_GPORT_EXTENDER_PORT_ID_SET(egr->port, vp);
            } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                BCM_GPORT_VLAN_PORT_ID_SET(egr->port, vp);
            } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeWlan)) {
                BCM_GPORT_WLAN_PORT_ID_SET(egr->port, vp);
            } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
                BCM_GPORT_VXLAN_PORT_ID_SET(egr->port, vp);
            } else {
                return BCM_E_INTERNAL;
            }
            egr->module = 0;
            egr->trunk = 0;
            egr->flags &= ~BCM_L3_TGID;
            egr->encap_id = 0;
        } else if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, egr->encap_id)) {
            /* encap_id contains a L2 egress object ID */
            egr->port = 0;
            egr->module = 0;
            egr->trunk = 0;
            egr->flags &= ~BCM_L3_TGID;
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            rv = _bcm_esw_l3_gport_construct(unit, egr->port, egr->module, 
                    egr->trunk, egr->flags, &(egr->port));
        }
        if ((intf - BCM_XGS3_EGRESS_IDX_MIN) == BCM_XGS3_L3_BLACK_HOLE_NH_IDX) {
             egr->port = BCM_GPORT_BLACK_HOLE;
        }

    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_find
 * Purpose:
 *      Find an egress forwarding object.     
 * Parameters:
 *      unit       - (IN) bcm device.
 *      egr        - (IN) Egress object properties to match.  
 *      intf       - (OUT) L3 interface id pointing to egress object
 *                         if found. 
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_find(int unit, bcm_l3_egress_t *egr, 
                                 bcm_if_t *intf)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        bcm_l3_egress_t egr_local;
        int vp_routing = FALSE;

        /* Input parameters check. */
        if ((NULL == egr) || (NULL == intf)) {
            return (BCM_E_PARAM);
        }

        /* Copy provided structure to local so it can be modified. */
        sal_memcpy(&egr_local, egr, sizeof(bcm_l3_egress_t));

        if (BCM_GPORT_IS_SET(egr_local.port)) {
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_virtual_port_routing) &&
                (BCM_GPORT_IS_NIV_PORT(egr_local.port) ||
                 BCM_GPORT_IS_EXTENDER_PORT(egr_local.port) ||
                 BCM_GPORT_IS_VLAN_PORT(egr_local.port))) {
                vp_routing = TRUE;
            }
#endif 
            if (BCM_GPORT_IS_WLAN_PORT(egr_local.port) || (vp_routing)) {
                /* Resolve the WLAN gport and store the VP in egr->encap_id */
                rv = _bcm_esw_gport_resolve(unit, egr_local.port, 
                                            &(egr_local.module), 
                                            &(egr_local.port), 
                                            &(egr_local.trunk), 
                                            &(egr_local.encap_id));  
                if (-1 != egr_local.trunk) {
                    egr_local.flags |= BCM_L3_TGID;
                }
            } else {
                rv = _bcm_esw_l3_gport_resolve(unit, egr_local.port, 
                                               &(egr_local.port), 
                                               &(egr_local.module), 
                                               &(egr_local.trunk), 
                                               &(egr_local.flags));
            }
            BCM_IF_ERROR_RETURN(rv); 
        } else {
            PORT_DUALMODID_VALID(unit, egr_local.port);
        }
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_find(unit, &egr_local, intf);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_egress_traverse
 * Purpose:
 *      Goes through egress objects table and runs the user callback
 *      function at each valid egress objects entry passing back the
 *      information for that object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      trav_fn    - (IN) Callback function. 
 *      user_data  - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_traverse(int unit, 
                           bcm_l3_egress_traverse_cb trav_fn, void *user_data)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_traverse(unit, trav_fn, user_data);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_max_create 
 * Purpose:
 *      Create an Egress Multipath forwarding object with specified path-width
 * Parameters:
 *      unit       - (IN) bcm device.
 *      flags      - (IN) BCM_L3_REPLACE: replace existing.
 *                        BCM_L3_WITH_ID: intf argument is given.
 *      max_paths - (IN) configurable path-width
 *      intf_count - (IN) Number of elements in intf_array.
 *      intf_array - (IN) Array of Egress forwarding objects.
 *      mpintf     - (OUT) L3 interface id pointing to Egress multipath object.
 *                         This is an IN argument if either BCM_L3_REPLACE
 *                          or BCM_L3_WITH_ID are given in flags.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_egress_multipath_max_create(int unit, uint32 flags, int max_paths,
                                   int intf_count, bcm_if_t *intf_array, bcm_if_t *mpintf)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_TRX(unit) && soc_feature(unit, soc_feature_l3)) {
         L3_LOCK(unit);
         rv = bcm_xgs3_l3_egress_multipath_max_create(unit, flags, 0, max_paths, intf_count,
                                                              intf_array, mpintf);
         L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_create 
 * Purpose:
 *      Create an Egress Multipath forwarding object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      flags      - (IN) BCM_L3_REPLACE: replace existing.
 *                        BCM_L3_WITH_ID: intf argument is given.
 *      intf_count - (IN) Number of elements in intf_array.
 *      intf_array - (IN) Array of Egress forwarding objects.
 *      mpintf     - (OUT) L3 interface id pointing to Egress multipath object.
 *                         This is an IN argument if either BCM_L3_REPLACE
 *                          or BCM_L3_WITH_ID are given in flags.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_multipath_create(int unit, uint32 flags, int intf_count,
                                   bcm_if_t *intf_array, bcm_if_t *mpintf)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_multipath_create(unit, flags, 0, intf_count,
                                                   intf_array, mpintf);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_destroy
 * Purpose:
 *      Destroy an Egress Multipath forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      mpintf  - (IN) L3 interface id pointing to Egress multipath object.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_multipath_destroy(int unit, bcm_if_t mpintf) 
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);

        rv = bcm_xgs3_l3_egress_multipath_destroy(unit, mpintf);

        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_get
 * Purpose:
 *      Get an Egress Multipath forwarding object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      mpintf     - (IN) L3 interface id pointing to Egress multipath object.
 *      intf_size  - (IN) Size of allocated entries in intf_array.
 *      intf_array - (OUT) Array of Egress forwarding objects.
 *      intf_count - (OUT) Number of entries of intf_count actually filled in.
 *                      This will be a value less than or equal to the value.
 *                      passed in as intf_size unless intf_size is 0.  If
 *                      intf_size is 0 then intf_array is ignored and
 *                      intf_count is filled in with the number of entries
 *                      that would have been filled into intf_array if
 *                      intf_size was arbitrarily large.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_multipath_get(int unit, bcm_if_t mpintf, int intf_size,
                                bcm_if_t *intf_array, int *intf_count)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_multipath_get(unit, mpintf, intf_size,
                                                intf_array, intf_count);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_add
 * Purpose:
 *      Add an Egress forwarding object to an Egress Multipath 
 *      forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      mpintf  - (IN) L3 interface id pointing to Egress multipath object.
 *      intf    - (IN) L3 interface id pointing to Egress forwarding object.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_multipath_add(int unit, bcm_if_t mpintf, bcm_if_t intf)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_multipath_add(unit, mpintf, intf);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_delete
 * Purpose:
 *      Delete an Egress forwarding object to an Egress Multipath 
 *      forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      mpintf  - (IN) L3 interface id pointing to Egress multipath object
 *      intf    - (IN) L3 interface id pointing to Egress forwarding object
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_multipath_delete(int unit, bcm_if_t mpintf, bcm_if_t intf)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_multipath_delete(unit, mpintf, intf);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_find
 * Purpose:
 *      Find an egress multipath forwarding object.     
 * Parameters:
 *      unit       - (IN) bcm device.
 *      intf_count - (IN) Number of elements in intf_array. 
 *      intf_array - (IN) Array of egress forwarding objects.  
 *      intf       - (OUT) L3 interface id pointing to egress multipath object
 *                         if found. 
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_multipath_find(int unit, int intf_count, bcm_if_t
                                 *intf_array, bcm_if_t *mpintf)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_multipath_find(unit, intf_count,
                                                intf_array, mpintf);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_multipath_traverse
 * Purpose:
 *      Goes through multipath egress objects table and runs the user callback
 *      function at each multipath egress objects entry passing back the
 *      information for that multipath object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      trav_fn    - (IN) Callback function. 
 *      user_data  - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_multipath_traverse(int unit, 
                          bcm_l3_egress_multipath_traverse_cb trav_fn,
                          void *user_data)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_multipath_traverse(unit, trav_fn, user_data);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      _bcm_esw_l3_egress_ecmp_create 
 * Purpose:
 *      Create or modify an Egress ECMP forwarding object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      ecmp       - (INOUT) ECMP group info.
 *      intf_count - (IN) Number of elements in intf_array.
 *      intf_array - (IN) Array of Egress forwarding objects.
 *      op         - (IN) Member operation: SET, ADD, DELETE, or REPLACE.
 *      count      - (IN) Number of elements in intf.
 *      intf       - (IN) Egress forwarding objects to add, delete, or replace.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_l3_egress_ecmp_create(int unit, bcm_l3_egress_ecmp_t *ecmp,
        int intf_count, bcm_if_t *intf_array, int op, int count, bcm_if_t *intf)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    int rv2 = BCM_E_NONE;

    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {

        if (ecmp->dynamic_mode == BCM_L3_ECMP_DYNAMIC_MODE_NORMAL ||
            ecmp->dynamic_mode == BCM_L3_ECMP_DYNAMIC_MODE_ASSIGNED ||
            ecmp->dynamic_mode == BCM_L3_ECMP_DYNAMIC_MODE_OPTIMAL) {
            if (!soc_feature(unit, soc_feature_ecmp_dlb)) {
               return BCM_E_UNAVAIL;
            }
        } else if (ecmp->dynamic_mode == BCM_L3_ECMP_DYNAMIC_MODE_RESILIENT) {
            if (!soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
               return BCM_E_UNAVAIL;
            }
        }

        L3_LOCK(unit);

        if (ecmp->max_paths > 0) {
            rv = bcm_xgs3_l3_egress_multipath_max_create(unit, ecmp->flags, 
                ecmp->ecmp_group_flags, ecmp->max_paths, intf_count, intf_array,
                &ecmp->ecmp_intf);
        } else {
            rv = bcm_xgs3_l3_egress_multipath_create(unit, ecmp->flags, 
                    ecmp->ecmp_group_flags, intf_count, intf_array, &ecmp->ecmp_intf);
        }
        
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return rv;
        }

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_dlb)) {
            rv = bcm_tr3_l3_egress_ecmp_dlb_create(unit, ecmp, intf_count,
                                                   intf_array);
        } 
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
            rv = bcm_td2_l3_egress_ecmp_rh_create(unit, ecmp, intf_count,
                              intf_array, op, count, intf);
        } 
#endif /* BCM_TRIUMPH3_SUPPORT */

        /*
         * One of above two hashing features should be active at a time.
         *
         * Free/Destroy the software state created for ecmp object if
         * there is error in feature specific multipath state creation.
         * Return the main error coming from feature specific
         * routines. Error coming from multipath destroy is very rare
         * as the object that was just created is being destroyed.
         * But even if there is some error, display a message.
         */
        if (BCM_FAILURE(rv)) {
            rv2 = bcm_xgs3_l3_egress_multipath_destroy(unit, ecmp->ecmp_intf); 
        }

        if (BCM_FAILURE(rv2)) {
            LOG_ERROR(BSL_LS_BCM_COMMON,
                      (BSL_META_U(unit,
                                  "%s: multipath destroy failed : %d \n"), FUNCTION_NAME(),  rv2));
        }
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_create 
 * Purpose:
 *      Create or modify an Egress ECMP forwarding object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      ecmp       - (INOUT) ECMP group info.
 *      intf_count - (IN) Number of elements in intf_array.
 *      intf_array - (IN) Array of Egress forwarding objects.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_create(int unit, bcm_l3_egress_ecmp_t *ecmp,
        int intf_count, bcm_if_t *intf_array)
{
    int rv;

    if (soc_feature(unit, soc_feature_ecmp_resilient_hash) &&
        (ecmp->flags & BCM_L3_ECMP_RH_REPLACE)) {
        int alloc_size, old_intf_count;
        bcm_if_t *old_intf_array = NULL;
        int i;

        /* Input check */
        if (intf_count > 0 && intf_array == NULL) {
            return (BCM_E_PARAM);
        }

        if (!BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, ecmp->ecmp_intf)) {
            return (BCM_E_PARAM);
        }

        for (i = 0; i < intf_count; i++) {
            if (!BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf_array[i])) {
                return (BCM_E_PARAM);
            }
        }

        /* Allocate interface array */
        alloc_size = sizeof(bcm_if_t) * BCM_XGS3_L3_ECMP_MAX(unit);
        old_intf_array = sal_alloc(alloc_size, "old intf array");
        if (NULL == old_intf_array) {
            return BCM_E_MEMORY;
        }
        sal_memset(old_intf_array, 0, alloc_size);

        L3_LOCK(unit);

        /* Get ECMP group's existing interface array */
        rv = bcm_esw_l3_egress_ecmp_get(unit, ecmp, BCM_XGS3_L3_ECMP_MAX(unit),
                old_intf_array, &old_intf_count);
        if (BCM_FAILURE(rv)) {
            sal_free(old_intf_array);
            L3_UNLOCK(unit);
            return rv;
        }

        /* Check if there is room in ECMP group */
        if ((old_intf_count == ecmp->max_paths) && 
            (intf_count > old_intf_count)) {
            sal_free(old_intf_array);
            L3_UNLOCK(unit);
            return BCM_E_FULL;
        }

        if (intf_count > ecmp->max_paths) {
            sal_free(old_intf_array);
            L3_UNLOCK(unit);
            return BCM_E_RESOURCE;
        }

        /* Update ECMP group */
        ecmp->flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
        rv = _bcm_esw_l3_egress_ecmp_create(unit, ecmp, intf_count, intf_array,
                BCM_L3_ECMP_MEMBER_OP_REPLACE, old_intf_count, old_intf_array);

        sal_free(old_intf_array);

        L3_UNLOCK(unit);
    } else {
        rv = _bcm_esw_l3_egress_ecmp_create(unit, ecmp, intf_count, intf_array,
                BCM_L3_ECMP_MEMBER_OP_SET, 0, NULL);
    }

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_destroy
 * Purpose:
 *      Destroy an Egress ECMP forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      ecmp    - (IN) ECMP group info.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_destroy(int unit, bcm_l3_egress_ecmp_t *ecmp) 
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);

        rv = bcm_xgs3_l3_egress_multipath_destroy(unit, ecmp->ecmp_intf);

#ifdef BCM_TRIUMPH3_SUPPORT
        if (BCM_SUCCESS(rv)) {
            if (soc_feature(unit, soc_feature_ecmp_dlb)) {
                rv = bcm_tr3_l3_egress_ecmp_dlb_destroy(unit, ecmp);
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
        if (BCM_SUCCESS(rv)) {
            if (soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
                rv = bcm_td2_l3_egress_ecmp_rh_destroy(unit, ecmp->ecmp_intf);
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */

        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_get
 * Purpose:
 *      Get an Egress ECMP forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      ecmp    - (INOUT) ECMP group info.
 *      intf_size  - (IN) Size of allocated entries in intf_array.
 *      intf_array - (OUT) Array of Egress forwarding objects.
 *      intf_count - (OUT) Number of entries of intf_count actually filled in.
 *                      This will be a value less than or equal to the value.
 *                      passed in as intf_size unless intf_size is 0.  If
 *                      intf_size is 0 then intf_array is ignored and
 *                      intf_count is filled in with the number of entries
 *                      that would have been filled into intf_array if
 *                      intf_size was arbitrarily large.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_get(int unit, bcm_l3_egress_ecmp_t *ecmp, int intf_size,
                                bcm_if_t *intf_array, int *intf_count)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    int  grp_idx = 0;

    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {

        L3_LOCK(unit);

        rv = bcm_xgs3_l3_egress_multipath_get(unit, ecmp->ecmp_intf, intf_size,
                                                intf_array, intf_count);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return rv;
        }

        ecmp->flags = 0;
        rv = bcm_xgs3_l3_egress_ecmp_max_paths_get(unit, ecmp->ecmp_intf,
                &ecmp->max_paths);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return rv;
        }

        /* Get ecmp group index. */
        grp_idx = ecmp->ecmp_intf - BCM_XGS3_MPATH_EGRESS_IDX_MIN; 
        if (BCM_XGS3_L3_ECMP_GROUP_FLAGS(unit, grp_idx)) {
            ecmp->ecmp_group_flags = BCM_L3_ECMP_PATH_NO_SORTING; 
        } else {    
            ecmp->ecmp_group_flags = 0;
        }
        ecmp->dynamic_mode = 0;
        ecmp->dynamic_size = 0;
        ecmp->dynamic_age = 0;
        ecmp->dynamic_load_exponent = 0;
        ecmp->dynamic_expected_load_exponent = 0;

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_dlb)) {
            rv = bcm_tr3_l3_egress_ecmp_dlb_get(unit, ecmp);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
            rv = bcm_td2_l3_egress_ecmp_rh_get(unit, ecmp);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_add
 * Purpose:
 *      Add an Egress forwarding object to an Egress ECMP 
 *      forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      ecmp    - (IN) ECMP group info.
 *      intf    - (IN) L3 interface id pointing to Egress forwarding object.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_add(int unit, bcm_l3_egress_ecmp_t *ecmp, bcm_if_t intf)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        int alloc_size;
        bcm_if_t *intf_array;
        int intf_count;

        /* Input check */
        if (!BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, ecmp->ecmp_intf)) {
            return (BCM_E_PARAM);
        }
        if (!BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf)) {
            return (BCM_E_PARAM);
        }

        /* Allocate interface array */
        alloc_size = sizeof(bcm_if_t) * BCM_XGS3_L3_ECMP_MAX(unit);
        intf_array = sal_alloc(alloc_size, "intf array");
        if (NULL == intf_array) {
            return BCM_E_MEMORY;
        }
        sal_memset(intf_array, 0, alloc_size);

        L3_LOCK(unit);

        /* Get ECMP group's existing interface array */
        rv = bcm_esw_l3_egress_ecmp_get(unit, ecmp, BCM_XGS3_L3_ECMP_MAX(unit),
                intf_array, &intf_count);
        if (BCM_FAILURE(rv)) {
            sal_free(intf_array);
            L3_UNLOCK(unit);
            return rv;
        }

        /* Check if there is room in ECMP group */
        if (intf_count == ecmp->max_paths) {
            sal_free(intf_array);
            L3_UNLOCK(unit);
            return BCM_E_FULL;
        }

        intf_array[intf_count] = intf;

        /* Update ECMP group */
        ecmp->flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
        rv = _bcm_esw_l3_egress_ecmp_create(unit, ecmp, intf_count + 1,
                intf_array, BCM_L3_ECMP_MEMBER_OP_ADD, 1, &intf);

        sal_free(intf_array);

        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_delete
 * Purpose:
 *      Delete an Egress forwarding object to an Egress ECMP
 *      forwarding object.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      ecmp    - (IN) ECMP group info.
 *      intf    - (IN) L3 interface id pointing to Egress forwarding object
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_delete(int unit, bcm_l3_egress_ecmp_t *ecmp,
        bcm_if_t intf)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        int alloc_size;
        bcm_if_t *intf_array;
        int intf_count;
        int i, j;

        /* Input check */
        if (!BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, ecmp->ecmp_intf)) {
            return (BCM_E_PARAM);
        }
        if (!BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf)) {
            return (BCM_E_PARAM);
        }

        /* Allocate interface array */
        alloc_size = sizeof(bcm_if_t) * BCM_XGS3_L3_ECMP_MAX(unit);
        intf_array = sal_alloc(alloc_size, "intf array");
        if (NULL == intf_array) {
            return BCM_E_MEMORY;
        }
        sal_memset(intf_array, 0, alloc_size);

        L3_LOCK(unit);

        /* Get ECMP group's existing interface array */
        rv = bcm_esw_l3_egress_ecmp_get(unit, ecmp, BCM_XGS3_L3_ECMP_MAX(unit),
                intf_array, &intf_count);
        if (BCM_FAILURE(rv)) {
            sal_free(intf_array);
            L3_UNLOCK(unit);
            return rv;
        }

        /* Search for given intf */
        for (i = 0; i < intf_count; i++) {
            if (intf_array[i] == intf) {
                break;
            }
        }
        if (i == intf_count) {
            /* Not found */
            sal_free(intf_array);
            L3_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        }

        /* Shift remaining elements of intf_array */
        for (j = i; j < intf_count - 1; j++) {
            intf_array[j] = intf_array[j+1];
        }

        /* Update ECMP group */
        ecmp->flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
        rv = _bcm_esw_l3_egress_ecmp_create(unit, ecmp, intf_count - 1,
                intf_array, BCM_L3_ECMP_MEMBER_OP_DELETE, 1, &intf);

        sal_free(intf_array);

        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_find
 * Purpose:
 *      Find an egress ECMP forwarding object.     
 * Parameters:
 *      unit       - (IN) bcm device.
 *      intf_count - (IN) Number of elements in intf_array. 
 *      intf_array - (IN) Array of egress forwarding objects.  
 *      ecmp       - (OUT) ECMP group info.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_find(int unit, int intf_count,
        bcm_if_t *intf_array, bcm_l3_egress_ecmp_t *ecmp)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {

        L3_LOCK(unit);

        rv = bcm_xgs3_l3_egress_multipath_find(unit, intf_count,
                                                intf_array, &ecmp->ecmp_intf);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return rv;
        }

        ecmp->flags = 0;
        rv = bcm_xgs3_l3_egress_ecmp_max_paths_get(unit, ecmp->ecmp_intf,
                &ecmp->max_paths);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return rv;
        }
        ecmp->ecmp_group_flags = 0;
        ecmp->dynamic_mode = 0;
        ecmp->dynamic_size = 0;
        ecmp->dynamic_age = 0;
        ecmp->dynamic_load_exponent = 0;
        ecmp->dynamic_expected_load_exponent = 0;

#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_dlb)) {
            rv = bcm_tr3_l3_egress_ecmp_dlb_get(unit, ecmp);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
            rv = bcm_td2_l3_egress_ecmp_rh_get(unit, ecmp);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_traverse
 * Purpose:
 *      Goes through ECMP egress objects table and runs the user callback
 *      function at each ECMP egress objects entry passing back the
 *      information for that ECMP object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      trav_fn    - (IN) Callback function. 
 *      user_data  - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_traverse(int unit, 
                          bcm_l3_egress_ecmp_traverse_cb trav_fn,
                          void *user_data)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);
        rv = bcm_xgs3_l3_egress_ecmp_traverse(unit, trav_fn, user_data);
        L3_UNLOCK(unit);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_member_status_set
 * Purpose:
 *      Set ECMP dynamic load balancing member status.
 * Parameters:
 *      unit - (IN) Unit number.
 *      intf - (IN) L3 Interface ID pointing to an Egress forwarding object.
 *      status - (IN) ECMP member status.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_member_status_set(
    int unit, 
    bcm_if_t intf,
    int status)
{
    int rv = BCM_E_UNAVAIL;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_ecmp_dlb)) {
        L3_LOCK(unit);
        rv = bcm_tr3_l3_egress_ecmp_member_status_set(unit, intf, status);
        L3_UNLOCK(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_member_status_get
 * Purpose:
 *      Get ECMP member status.
 * Parameters:
 *      unit - (IN) Unit number.
 *      intf - (IN) L3 Interface ID pointing to an Egress forwarding object.
 *      status - (OUT) ECMP member status.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_member_status_get(
    int unit, 
    bcm_if_t intf,
    int *status)
{
    int rv = BCM_E_UNAVAIL;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_ecmp_dlb)) {
        L3_LOCK(unit);
        rv = bcm_tr3_l3_egress_ecmp_member_status_get(unit, intf, status);
        L3_UNLOCK(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_ethertype_set
 * Purpose:
 *      Set the Ethertypes that are eligible or ineligible for
 *      ECMP dynamic load balancing or resilient hashing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) BCM_L3_ECMP_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_count - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (IN) Array of Ethertypes.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_ethertype_set(
    int unit, 
    uint32 flags, 
    int ethertype_count, 
    int *ethertype_array)
{
    int rv;

    L3_LOCK(unit);

    rv = BCM_E_UNAVAIL;

    if (flags & BCM_L3_ECMP_DYNAMIC_ETHERTYPE_RESILIENT) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
            rv = bcm_td2_l3_egress_ecmp_rh_ethertype_set(unit, flags,
                    ethertype_count, ethertype_array);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
    } else {
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_dlb)) {
            rv = bcm_tr3_l3_egress_ecmp_dlb_ethertype_set(unit, flags,
                    ethertype_count, ethertype_array);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
    }

    L3_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_egress_ecmp_ethertype_get
 * Purpose:
 *      Get the Ethertypes that are eligible or ineligible for
 *      ECMP dynamic load balancing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (INOUT) BCM_L3_ECMP_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_max - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (OUT) Array of Ethertypes.
 *      ethertype_count - (OUT) Number of elements returned in ethertype_array.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_egress_ecmp_ethertype_get(
    int unit, 
    uint32 *flags, 
    int ethertype_max, 
    int *ethertype_array, 
    int *ethertype_count)
{
    int rv;

    L3_LOCK(unit);

    rv = BCM_E_UNAVAIL;

    if (*flags & BCM_L3_ECMP_DYNAMIC_ETHERTYPE_RESILIENT) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
            rv = bcm_td2_l3_egress_ecmp_rh_ethertype_get(unit, flags,
                    ethertype_max, ethertype_array, ethertype_count);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
    } else {
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_ecmp_dlb)) {
            rv = bcm_tr3_l3_egress_ecmp_dlb_ethertype_get(unit, flags,
                    ethertype_max, ethertype_array, ethertype_count);
            if (BCM_FAILURE(rv)) {
                L3_UNLOCK(unit);
                return rv;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
    }

    L3_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_ip6_prefix_map_get
 * Purpose:
 *      Get a list of IPv6 96 bit prefixes which are mapped to ipv4 lookup
 *      space.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      map_size   - (IN) Size of allocated entries in ip6_array.
 *      ip6_array  - (OUT) Array of mapped prefixes.
 *      ip6_count  - (OUT) Number of entries of ip6_array actually filled in.
 *                      This will be a value less than or equal to the value.
 *                      passed in as map_size unless map_size is 0.  If
 *                      map_size is 0 then ip6_array is ignored and
 *                      ip6_count is filled in with the number of entries
 *                      that would have been filled into ip6_array if
 *                      map_size was arbitrarily large.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_ip6_prefix_map_get(int unit, int map_size, 
                              bcm_ip6_t *ip6_array, int *ip6_count)
{
    int rv = BCM_E_UNAVAIL;

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, BCM_L3_IP6)) {
        return (BCM_E_UNAVAIL);
    }
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    L3_LOCK(unit);
    rv = bcm_xgs3_l3_ip6_prefix_map_get(unit, map_size, 
                                          ip6_array, ip6_count);
    L3_UNLOCK(unit);
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}
/*
 * Function:
 *      bcm_esw_l3_ip6_prefix_map_add
 * Purpose:
 *      Create an IPv6 prefix map into IPv4 entry. In case Ipv6 traffic
 *      destination or source IP address matches upper 96 bits of
 *      translation entry. traffic will be routed/switched  based on
 *      lower 32 bits of destination/source IP address treated as IPv4 address.
 * Parameters:
 *      unit     - (IN)  bcm device.
 *      ip6_addr - (IN)  New IPv6 translation address.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_ip6_prefix_map_add(int unit, bcm_ip6_t ip6_addr) 
{
    int rv = BCM_E_UNAVAIL;

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, BCM_L3_IP6)) {
        return (BCM_E_UNAVAIL);
    }
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    L3_LOCK(unit);
    rv = bcm_xgs3_l3_ip6_prefix_map_add(unit, ip6_addr);
    L3_UNLOCK(unit);
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_ip6_prefix_map_delete
 * Purpose:
 *      Destroy an IPv6 prefix map entry.
 * Parameters:
 *      unit     - (IN)  bcm device.
 *      ip6_addr - (IN)  IPv6 translation address.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_ip6_prefix_map_delete(int unit, bcm_ip6_t ip6_addr)
{
    int rv = BCM_E_UNAVAIL;

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, BCM_L3_IP6)) {
        return (BCM_E_UNAVAIL);
    }
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    L3_LOCK(unit);
    rv = bcm_xgs3_l3_ip6_prefix_map_delete(unit, ip6_addr);
    L3_UNLOCK(unit);
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_ip6_prefix_map_delete_all
 * Purpose:
 *      Flush all IPv6 prefix maps.
 * Parameters:
 *      unit     - (IN)  bcm device.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_ip6_prefix_map_delete_all(int unit)
{
    int rv = BCM_E_UNAVAIL;

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, BCM_L3_IP6)) {
        return (BCM_E_UNAVAIL);
    }

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    L3_LOCK(unit);
    rv = bcm_xgs3_l3_ip6_prefix_map_delete_all(unit);
    L3_UNLOCK(unit);
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_vrrp_add
 * Purpose:
 *      Add VRID for the given VSI. Adding a VRID using this API means
 *      the physical node has become the master for the virtual router
 * Parameters:
 *      unit - (IN) Unit number.
 *      vlan - (IN) VLAN/VSI
 *      vrid - (IN) VRID - Virtual router ID to be added
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_l3_vrrp_add(int unit, bcm_vlan_t vlan, uint32 vrid)
{
    bcm_l2_addr_t l2addr;            /* Layer 2 address for interface. */
    int rv;                          /* Operation return status.       */
    bcm_mac_t mac = {0x00, 0x00, 0x5e, 0x00, 0x01, 0x00};/* Mac address.*/

    if (!BCM_VLAN_VALID(vlan)) {
        return (BCM_E_PARAM);
    }

    if (vrid > 0xff) {
        return (BCM_E_PARAM);
    }
    mac[5] = vrid;

    /* Set l2 address info. */
    bcm_l2_addr_t_init(&l2addr, mac, vlan);

#if defined(BCM_MIRAGE_SUPPORT)
    if (soc_feature(unit, soc_feature_fp_routing_mirage)) {
        /* Set l2 entry flags. */
        l2addr.flags = BCM_L2_STATIC | BCM_L2_REPLACE_DYNAMIC;

        /* Set l2 entry port to CPU port. */
        l2addr.port = BCM_MIRAGE_L3_PORT;
    } else 
#endif /* BCM_MIRAGE_SUPPORT */
    {
        /* Set l2 entry flags. */
        l2addr.flags = BCM_L2_L3LOOKUP | BCM_L2_STATIC | BCM_L2_REPLACE_DYNAMIC;

        /* Set l2 entry port to CPU port. */
        l2addr.port = CMIC_PORT(unit); 
    }

    /* Set l2 entry module id to local module. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &l2addr.modid));

    /* Overwrite existing entry if any. */
    bcm_esw_l2_addr_delete(unit, mac, vlan);

    /* Add entry to l2 table. */
    rv = bcm_esw_l2_addr_add(unit, &l2addr);

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_vrrp_delete
 * Purpose:
 *      Delete VRID for a particulat VLAN/VSI
 * Parameters:
 *      unit - (IN) Unit number.
 *      vlan - (IN) VLAN/VSI
 *      vrid - (IN) VRID - Virtual router ID to be deleted
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_l3_vrrp_delete(int unit, bcm_vlan_t vlan, uint32 vrid)
{
    bcm_l2_addr_t l2addr;            /* Layer 2 address for interface. */
    int rv;                          /* Operation return status.       */
    bcm_mac_t mac = {0x00, 0x00, 0x5e, 0x00, 0x01, 0x00};/* Mac address.*/

    if (!BCM_VLAN_VALID(vlan)) {
        return (BCM_E_PARAM);
    }

    if (vrid > 0xff) {
        return (BCM_E_PARAM);
    }
    mac[5] = vrid;

    /* Set l2 address info. */
    bcm_l2_addr_t_init(&l2addr, mac, vlan);

    /* Overwrite existing entry if any. */
    rv = bcm_esw_l2_addr_delete(unit, mac, vlan);
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_vrrp_delete_all
 * Purpose:
 *      Delete all the VRIDs for a particular VLAN/VSI
 * Parameters:
 *      unit - (IN) Unit number.
 *      vlan - (IN) VLAN/VSI
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_l3_vrrp_delete_all(int unit, bcm_vlan_t vlan)
{
    int idx;     /* vr id iteration index.   */ 
    int rv;      /* Operation return status. */

    if (!BCM_VLAN_VALID(vlan)) {
        return (BCM_E_PARAM);
    }

    for (idx = 0; idx < 0x100; idx++) {
        rv = bcm_esw_l3_vrrp_delete(unit, vlan, idx);
        if (BCM_FAILURE(rv) && (BCM_E_NOT_FOUND != rv)) {
            return (rv);
        }
    }
    return (BCM_E_NONE);
}


/*
 * Function:
 *      bcm_esw_l3_vrrp_get
 * Purpose:
 *      Get all the VRIDs for which the physical node is master for
 *      the virtual routers on the given VLAN/VSI
 * Parameters:
 *      unit - (IN) Unit number.
 *      vlan - (IN) VLAN/VSI
 *      alloc_size - (IN) Size of vrid_array
 *      vrid_array - (OUT) Pointer to the array to which the VRIDs will be copied
 *      count - (OUT) Number of VRIDs copied
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_l3_vrrp_get(int unit, bcm_vlan_t vlan, int alloc_size, 
                    int *vrid_array, int *count)
{
    bcm_l2_addr_t l2addr;             /* Layer 2 address for interface. */
    int idx;                          /* vr id iteration index.         */ 
    int tmp_cnt;                      /* Valie entry count.             */ 
    int rv;                           /* Operation return status.       */
    bcm_mac_t mac = {0x00, 0x00, 0x5e, 0x00, 0x01, 0x00};/* Mac address.*/

    if (!BCM_VLAN_VALID(vlan)) {
        return (BCM_E_PARAM);
    }

    if (NULL == count) {
        return (BCM_E_PARAM);
    }

    tmp_cnt = 0;
    for (idx = 0; idx < 0x100; idx++) {
        if ( alloc_size > tmp_cnt) {
            mac[5] = idx;

            /* Set l2 address info. */
            bcm_l2_addr_t_init(&l2addr, mac, vlan);

            /* Overwrite existing entry if any. */
            rv = bcm_esw_l2_addr_get(unit, mac, vlan, &l2addr);
            if (BCM_SUCCESS(rv)) {
                if (NULL != vrid_array) {
                    vrid_array[tmp_cnt] = idx;  
                }
                tmp_cnt++;
            }
        }
    }
    *count = tmp_cnt;
    return (BCM_E_NONE);
}


#if defined(BCM_TRIUMPH2_SUPPORT)
#define BCM_VRF_STAT_VALID(unit, vrf) \
    if (!soc_feature(unit, soc_feature_gport_service_counters)) { \
        return BCM_E_UNAVAIL; \
    } else if (((vrf) > SOC_VRF_MAX(unit)) || \
               ((vrf) < BCM_L3_VRF_DEFAULT)) { return (BCM_E_PARAM); }

STATIC int
_bcm_vrf_flex_stat_hw_index_set(int unit, _bcm_flex_stat_handle_t fsh,
                                int fs_idx, void *cookie)
{
    uint32 handle = _BCM_FLEX_STAT_HANDLE_WORD_GET(fsh, 0);
    COMPILER_REFERENCE(cookie);
    if (SOC_MEM_FIELD_VALID(unit, VRFm, USE_SERVICE_CTR_IDXf)) {
        BCM_IF_ERROR_RETURN(
            soc_mem_field32_modify(unit, VRFm, handle,
                                  USE_SERVICE_CTR_IDXf, (fs_idx!=0) ? 1:0));
    }
    return soc_mem_field32_modify(unit, VRFm, handle,
                                  SERVICE_CTR_IDXf, fs_idx);
}

STATIC _bcm_flex_stat_t
_bcm_esw_l3_vrf_stat_to_flex_stat(bcm_l3_vrf_stat_t stat)
{
    _bcm_flex_stat_t flex_stat;

    switch (stat) {
    case bcmL3VrfStatIngressPackets:
        flex_stat = _bcmFlexStatIngressPackets;
        break;
    case bcmL3VrfStatIngressBytes:
        flex_stat = _bcmFlexStatIngressBytes;
        break;
    default:
        flex_stat = _bcmFlexStatNum;
    }

    return flex_stat;
}

/* Requires "idx" variable */
#define BCM_VRF_STAT_ARRAY_VALID(unit, nstat, value_arr) \
    for (idx = 0; idx < nstat; idx++) { \
        if (NULL == value_arr + idx) { \
            return (BCM_E_PARAM); \
        } \
    }

STATIC int
_bcm_l3_vrf_stat_array_convert(int unit, int nstat,
                               bcm_l3_vrf_stat_t *stat_arr, 
                               _bcm_flex_stat_t *fs_arr)
{
    int idx;

    if ((nstat <= 0) || (nstat > _bcmFlexStatEgressPackets)) {
        /* VRF only has ingress support, so the number of ingress types
         * is the first egress type. */
        return BCM_E_PARAM;
    }

    if (NULL == stat_arr) {
        return (BCM_E_PARAM);
    }

    for (idx = 0; idx < nstat; idx++) {
        fs_arr[idx] = _bcm_esw_l3_vrf_stat_to_flex_stat(stat_arr[idx]);
    }
    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_vrf_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given VRF(Virtual Router Interface) id.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) VRF(Virtual Router Interface) id
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static 
bcm_error_t _bcm_esw_l3_vrf_stat_get_table_info(
            int                        unit,
            bcm_vrf_t                  vrf,
            uint32                     *num_of_tables,
            bcm_stat_flex_table_info_t *table_info)
{
    (*num_of_tables)=0;

    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_l3_256_defip_table)) {
        return BCM_E_UNAVAIL; /* VRF stats not valid for Ranger & Ranger + */
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
    L3_INIT(unit);
    if ((vrf > SOC_VRF_MAX(unit)) || (vrf < BCM_L3_VRF_DEFAULT)) {
         return (BCM_E_PARAM);
    }
    table_info[*num_of_tables].table=VRFm;
    table_info[*num_of_tables].index=vrf;
    table_info[*num_of_tables].direction=bcmStatFlexDirectionIngress;
    (*num_of_tables)++;
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}
#endif

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_vrf_stat_attach
 * Description:
 *      Attach counters entries to the given  VRF 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

STATIC bcm_error_t _bcm_esw_l3_vrf_stat_attach(
            int       unit, 
            bcm_vrf_t vrf, 
            uint32    stat_counter_id)
{
    soc_mem_t                  table=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     pool_number=0;
    uint32                     base_index=0;
    bcm_stat_flex_mode_t       offset_mode=0;
    bcm_stat_object_t          object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t      group_mode= bcmStatGroupModeSingle;
    uint32                     count=0;
    uint32                     actual_num_tables=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    _bcm_esw_stat_get_counter_id_info(
                  stat_counter_id,
                  &group_mode,&object,&offset_mode,&pool_number,&base_index);
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit,group_mode));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_table_info(
                        unit,object,1,&actual_num_tables,&table,&direction));

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_get_table_info(
                        unit, vrf,&num_of_tables,&table_info[0]));
    for (count=0; count < num_of_tables ; count++) {
         if ((table_info[count].direction == direction) &&
             (table_info[count].table == table) ) {
              if (direction == bcmStatFlexDirectionIngress) {
                  return _bcm_esw_stat_flex_attach_ingress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
              } else {
                  return _bcm_esw_stat_flex_attach_egress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
              } 
         }
    }
    return BCM_E_NOT_FOUND;
}
#endif

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
/*
 * Function:
 *      _tr2_l3_vrf_stat_attach
 * Description:
 *      Attach counters entries to the given VRF for TR2/Trident device
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

STATIC bcm_error_t _tr2_l3_vrf_stat_attach(
            int       unit,
            bcm_vrf_t vrf,
            uint32    stat_counter_id,
            int enable)
{

    _bcm_flex_stat_type_t fs_type;
    uint32 fs_inx;
    int rv;

    fs_type = _BCM_FLEX_STAT_TYPE(stat_counter_id);
    fs_inx  = _BCM_FLEX_STAT_COUNT_INX(stat_counter_id);

    if (!((fs_type == _bcmFlexStatTypeVrf) && fs_inx)) {
        return BCM_E_PARAM;
    }
    BCM_VRF_STAT_VALID(unit, vrf);

    L3_LOCK(unit);
    rv = _bcm_esw_flex_stat_enable_set(unit, fs_type,
                         _bcm_vrf_flex_stat_hw_index_set,
                                  INT_TO_PTR(_BCM_FLEX_STAT_HW_INGRESS),
                                  vrf, enable,fs_inx);
    L3_UNLOCK(unit);
    return rv;
}
#endif

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_attach
 * Description:
 *      Attach counters entries to the given  VRF
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

int bcm_esw_l3_vrf_stat_attach(
            int       unit,
            bcm_vrf_t vrf,
            uint32    stat_counter_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_vrf_stat_attach(unit,vrf,stat_counter_id);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        return _tr2_l3_vrf_stat_attach(unit,vrf,stat_counter_id,TRUE);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_vrf_stat_detach
 * Description:
 *      Detach   counters entries to the given VRF
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC bcm_error_t _bcm_esw_l3_vrf_stat_detach(
            int       unit, 
            bcm_vrf_t vrf)
{
    uint32                     count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_error_t                rv[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = 
                                  {BCM_E_FAIL};
    uint32                     flag[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = {0};

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_get_table_info(
                        unit, vrf,&num_of_tables,&table_info[0]));

    for (count=0; count < num_of_tables ; count++) {
         if (table_info[count].direction == bcmStatFlexDirectionIngress) {
             rv[bcmStatFlexDirectionIngress]= 
                   _bcm_esw_stat_flex_detach_ingress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
             flag[bcmStatFlexDirectionIngress] = 1;
         } else {
             rv[bcmStatFlexDirectionEgress] = 
                   _bcm_esw_stat_flex_detach_egress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
             flag[bcmStatFlexDirectionIngress] = 1;
         }
    }
    if ((rv[bcmStatFlexDirectionIngress] == BCM_E_NONE) ||
        (rv[bcmStatFlexDirectionEgress] == BCM_E_NONE)) {
         return BCM_E_NONE;
    }
    if (flag[bcmStatFlexDirectionIngress] == 1) {
        return rv[bcmStatFlexDirectionIngress];
    }
    if (flag[bcmStatFlexDirectionEgress] == 1) {
        return rv[bcmStatFlexDirectionEgress];
    }
    return BCM_E_NOT_FOUND;
}
#endif

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_detach
 * Description:
 *      Detach   counters entries to the given VRF
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_vrf_stat_detach(
            int       unit,
            bcm_vrf_t vrf)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_vrf_stat_detach(unit,vrf);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        return _tr2_l3_vrf_stat_attach(unit,vrf,
                   _BCM_FLEX_STAT_COUNT_ID(_bcmFlexStatTypeVrf,1),
                   FALSE);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_vrf_stat_counter_get
 * Description:
 * Get L3 VRF counter value for specified VRF statistic type 
 * if sync_mode is set, sync the sw accumulated count
 * with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      vrf              - (IN) Virtual router instance
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC bcm_error_t _bcm_esw_l3_vrf_stat_counter_get(
            int               unit, 
            int               sync_mode,
            bcm_vrf_t         vrf, 
            bcm_l3_vrf_stat_t stat, 
            uint32            num_entries, 
            uint32            *counter_indexes, 
            bcm_stat_value_t  *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    if ((stat == bcmL3VrfStatIngressPackets) ||
        (stat == bcmL3VrfStatIngressBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3VrfStatIngressPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
    
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_get_table_info(
                        unit, vrf,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count=0; index_count < num_entries ; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_get(
                                      unit, sync_mode,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }
    return BCM_E_NONE;
}


#endif 
/*
 * Function:
 *      _bcm_esw_l3_vrf_flex_stat_counter_get
 * Description:
 * Get L3 VRF counter value for specified VRF statistic type
 *if sync_mode is set, sync the sw accumulated count
 *with hw count value first, else return sw count. 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      vrf              - (IN) Virtual router instance
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int _bcm_esw_l3_vrf_flex_stat_counter_get(
            int               unit,
            int               sync_mode,
            bcm_vrf_t         vrf,
            bcm_l3_vrf_stat_t stat,
            uint32            num_entries,
            uint32            *counter_indexes,
            bcm_stat_value_t  *counter_values)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_vrf_stat_counter_get(unit, sync_mode, vrf, stat,
                      num_entries, counter_indexes, counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint64 val;
        int rv = BCM_E_NONE;
 
        rv = _bcm_esw_l3_vrf_stat_get(unit, sync_mode, vrf, stat, &val);

        if (stat == bcmL3VrfStatIngressPackets) {
            counter_values->packets = COMPILER_64_LO(val);
        } else {
            COMPILER_64_SET(counter_values->bytes,
                      COMPILER_64_HI(val),
                      COMPILER_64_LO(val));
        }
        return rv;

    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}



/*
 * Function:
 *      bcm_esw_l3_vrf_stat_counter_get
 * Description:
 * Get L3 VRF counter value for specified VRF statistic type
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_vrf_stat_counter_get(
            int               unit,
            bcm_vrf_t         vrf,
            bcm_l3_vrf_stat_t stat,
            uint32            num_entries,
            uint32            *counter_indexes,
            bcm_stat_value_t  *counter_values)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_vrf_flex_stat_counter_get(unit, 0, vrf, stat, num_entries,
                                         counter_indexes, counter_values);
    return rv;
}    


/*
 * Function:
 *      bcm_esw_l3_vrf_stat_counter_sync_get
 * Description:
 * Get L3 VRF counter value for specified VRF statistic type
 * sw accumulated counters synced with hw count.
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_vrf_stat_counter_sync_get(
            int               unit,
            bcm_vrf_t         vrf,
            bcm_l3_vrf_stat_t stat,
            uint32            num_entries,
            uint32            *counter_indexes,
            bcm_stat_value_t  *counter_values)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_vrf_flex_stat_counter_get(unit, 1, vrf, stat, num_entries,
                                         counter_indexes, counter_values);
    return rv;
}
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_esw_l3_vrf_stat_counter_set
 * Description:
 *      Set L3 VRF counter value for specified VRF statistic type 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */
STATIC bcm_error_t _bcm_esw_l3_vrf_stat_counter_set(
            int               unit, 
            bcm_vrf_t         vrf, 
            bcm_l3_vrf_stat_t stat, 
            uint32            num_entries, 
            uint32            *counter_indexes, 
            bcm_stat_value_t  *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    if ((stat == bcmL3VrfStatIngressPackets) ||
        (stat == bcmL3VrfStatIngressBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3VrfStatIngressPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
    
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_get_table_info(
                        unit, vrf,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count=0; index_count < num_entries ; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_set(
                                      unit,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }
    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_counter_set
 * Description:
 *      Set L3 VRF counter value for specified VRF statistic type
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */
int bcm_esw_l3_vrf_stat_counter_set(
            int               unit,
            bcm_vrf_t         vrf,
            bcm_l3_vrf_stat_t stat,
            uint32            num_entries,
            uint32            *counter_indexes,
            bcm_stat_value_t  *counter_values)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_l3_vrf_stat_counter_set(unit,vrf,stat,
                      num_entries,counter_indexes,counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        uint64 val;
        int rv = BCM_E_NONE;

        if (stat == bcmL3VrfStatIngressPackets) {
            COMPILER_64_SET(val,0,counter_values->packets);
        } else {
            COMPILER_64_SET(val,
                      COMPILER_64_HI(counter_values->bytes),
                      COMPILER_64_LO(counter_values->bytes));
        }
        rv = bcm_esw_l3_vrf_stat_set(unit,vrf,stat,val);

        return rv;

    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }

}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_id_get
 * Description:
 *      Get stat counter id associated with given VRF
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vrf              - (IN) Virtual router instance
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      Stat_counter_id  - (OUT) Stat Counter ID
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_vrf_stat_id_get(
            int             unit,
            bcm_vrf_t       vrf, 
            bcm_l3_vrf_stat_t stat, 
            uint32          *stat_counter_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                     index=0;
    uint32                     num_stat_counter_ids=0;

    if ((stat == bcmL3VrfStatIngressPackets) ||
        (stat == bcmL3VrfStatIngressBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         direction = bcmStatFlexDirectionEgress;
    }
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_get_table_info(
                        unit,vrf,&num_of_tables,&table_info[0]));
    for (index=0; index < num_of_tables ; index++) {
         if (table_info[index].direction == direction)
             return _bcm_esw_stat_flex_get_counter_id(
                                  unit, 1, &table_info[index],
                                  &num_stat_counter_ids,stat_counter_id);
    }
    return BCM_E_NOT_FOUND;
#else
    return BCM_E_UNAVAIL;
#endif
}





/*
 * Function:
 *      bcm_esw_l3_vrf_stat_enable_set
 * Purpose:
 *      Enable/disable packet and byte counters for the selected VRF.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      enable - (IN) Non-zero to enable counter collection, zero to disable.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_enable_set(int unit, bcm_vrf_t vrf, int enable)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int                        rv = BCM_E_UNAVAIL;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_stat_object_t          object=bcmStatObjectIngPort;
    uint32                     num_entries=0;
    uint32                     num_stat_counter_ids=0;
    uint32                     stat_counter_id[
                                     BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]={0};
#endif /* BCM_KATANA_SUPPORT */
    L3_INIT(unit);
   if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);

    L3_LOCK(unit);
    rv = _bcm_esw_flex_stat_enable_set(unit, _bcmFlexStatTypeVrf,
                                       _bcm_vrf_flex_stat_hw_index_set,
                                       NULL, vrf, enable,0);
    L3_UNLOCK(unit);
    return rv;
   } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (enable) { 
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_get_table_info(
                            unit,vrf,&num_of_tables,&table_info[0]));
        if (num_of_tables != 1) {
                return BCM_E_INTERNAL;
        }
        if (table_info[0].direction != bcmStatFlexDirectionIngress) {
            return BCM_E_INTERNAL;
        }
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_ingress_object(
                            unit,table_info[0].table,
                            table_info[0].index,NULL,&object)); 
        BCM_IF_ERROR_RETURN(bcm_esw_stat_group_create(
                            unit,object,bcmStatGroupModeSingle,
                            &stat_counter_id[table_info[0].direction],
                            &num_entries));
        BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_attach(
                            unit,vrf,stat_counter_id[table_info[0].direction])); 
    } else {
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_get_table_info(
                            unit,vrf,&num_of_tables,&table_info[0]));
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_counter_id(
                            unit, num_of_tables,&table_info[0],
                            &num_stat_counter_ids,&stat_counter_id[0])); 
        if (num_stat_counter_ids != 1) {
            return BCM_E_INTERNAL;
        }
        BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_detach(unit,vrf));
        BCM_IF_ERROR_RETURN(bcm_esw_stat_group_destroy(
                            unit,
                            stat_counter_id[bcmStatFlexDirectionIngress]));
    }
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_KATANA_SUPPORT */
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      _bcm_esw_l3_vrf_stat_get
 * Purpose:
 *      Get 64-bit counter value for specified VRF statistic type.
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      sync_mode - (IN) hwcount is to be synced to sw count 
 *      vrf       - (IN) Virtual router instance.
 *      stat      - (IN) Type of the counter to retrieve.
 *      val       - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_esw_l3_vrf_stat_get(int unit, int sync_mode,
                         bcm_vrf_t vrf, bcm_l3_vrf_stat_t stat, uint64 *val)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int              rv=BCM_E_UNAVAIL;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif
    L3_INIT(unit);
   if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_get(unit, sync_mode, _bcmFlexStatTypeVrf, vrf,
                           _bcm_esw_l3_vrf_stat_to_flex_stat(stat), val);
    L3_UNLOCK(unit);
    return rv;
   } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_counter_get(unit, sync_mode, vrf,
                                                         stat, 1, 
                                                         &counter_indexes, 
                                                         &counter_values));
    if (stat == bcmL3VrfStatIngressPackets) {
        COMPILER_64_SET(*val,
                        COMPILER_64_HI(counter_values.packets64),
                        COMPILER_64_LO(counter_values.packets64));
    } else {
        COMPILER_64_SET(*val,
                        COMPILER_64_HI(counter_values.bytes),
                        COMPILER_64_LO(counter_values.bytes));
    }
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}


/*
 * Function:
 *      bcm_esw_l3_vrf_stat_get
 * Purpose:
 *      Get 64-bit counter value for specified VRF statistic type.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_get(int unit, bcm_vrf_t vrf, bcm_l3_vrf_stat_t stat, 
                        uint64 *val)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_vrf_stat_get(unit, 0, vrf, stat, val);

    return rv;
}


/*
 * Function:
 *      bcm_esw_l3_vrf_stat_sync_get
 * Purpose:
 *       Get 64-bit counter value for specified VRF statistic type.
 *       sw accumulated counters synced with hw count.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_sync_get(int unit, bcm_vrf_t vrf, bcm_l3_vrf_stat_t stat, 
                        uint64 *val)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_vrf_stat_get(unit, 1, vrf, stat, val);

    return rv;
} 



/*
 * Function:
 *      _bcm_esw_l3_vrf_stat_get32
 * Purpose:
 *      Get lower 32-bit counter value for specified VRF statistic
 *      type.
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      sync_mode   - (IN) hwcount is to be synced to sw count 
 *      vrf         - (IN) Virtual router instance.
 *      stat        - (IN) Type of the counter to retrieve.
 *      val         - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_esw_l3_vrf_stat_get32(int unit, int sync_mode, bcm_vrf_t vrf, 
                          bcm_l3_vrf_stat_t stat, uint32 *val)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif
   if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    L3_INIT(unit);
    BCM_VRF_STAT_VALID(unit, vrf);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_get32(unit, sync_mode, _bcmFlexStatTypeVrf, vrf,
                          _bcm_esw_l3_vrf_stat_to_flex_stat(stat), val);
    L3_UNLOCK(unit);
    return rv;
   }
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_vrf_stat_counter_get(unit, sync_mode, vrf,
                                                         stat, 1, 
                                                         &counter_indexes, 
                                                         &counter_values));
    if (stat == bcmL3VrfStatIngressPackets) {
        *val = counter_values.packets;
    } else {
        /* Ignoring Hi bytes value */
        *val = COMPILER_64_LO(counter_values.bytes);
    }
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_get32
 * Purpose:
 *      Get lower 32-bit counter value for specified VRF statistic
 *      type.
 * Parameters:
 *      unit - (IN) Unit number.
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      vrf - (IN) Virtual router instance.
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_get32(int unit,bcm_vrf_t vrf, 
                          bcm_l3_vrf_stat_t stat, uint32 *val)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_vrf_stat_get32(unit, 0, vrf, stat, val);

    return rv;
}


/*
 * Function:
 *      bcm_esw_l3_vrf_stat_sync_get32
 * Purpose:
 *      Get lower 32-bit counter value for specified VRF statistic
 *      type.
 *      sw accumulated counters synced with hw count.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_sync_get32(int unit,bcm_vrf_t vrf, 
                          bcm_l3_vrf_stat_t stat, uint32 *val)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_vrf_stat_get32(unit, 1, vrf, stat, val);

    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_set
 * Purpose:
 *      Set 64-bit counter value for specified VRF statistic type.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (IN) New counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_set(int unit, bcm_vrf_t vrf, bcm_l3_vrf_stat_t stat, 
                        uint64 val)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif
    L3_INIT(unit);
   if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_set(unit, _bcmFlexStatTypeVrf, vrf,
                           _bcm_esw_l3_vrf_stat_to_flex_stat(stat), val);
    L3_UNLOCK(unit);
    return rv;
   } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (stat == bcmL3VrfStatIngressPackets) {
        counter_values.packets = COMPILER_64_LO(val);
    } else {
        COMPILER_64_SET(counter_values.bytes,
                        COMPILER_64_HI(val),
                        COMPILER_64_LO(val));
    }
    BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_counter_set(
                        unit,vrf,stat,1, &counter_indexes, &counter_values));
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_set32
 * Purpose:
 *      Set lower 32-bit counter value for specified VRF statistic
 *      type.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (IN) New counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_set32(int unit, bcm_vrf_t vrf, 
                          bcm_l3_vrf_stat_t stat, uint32 val)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif
    L3_INIT(unit);
   if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_set32(unit, _bcmFlexStatTypeVrf, vrf,
                            _bcm_esw_l3_vrf_stat_to_flex_stat(stat), val);
    L3_UNLOCK(unit);
    return rv;
   } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (stat == bcmL3VrfStatIngressPackets) {
        counter_values.packets = val;
    } else {
        /* Ignoring high value */
        COMPILER_64_SET(counter_values.bytes,0,val);
    }
    BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_counter_set(
                        unit,vrf, stat, 1, &counter_indexes, &counter_values));
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_multi_get
 * Purpose:
 *      Get 64-bit counter value for multiple VRF statistic types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (OUT) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_multi_get(int unit, bcm_vrf_t vrf, int nstat, 
                              bcm_l3_vrf_stat_t *stat_arr,
                              uint64 *value_arr)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int rv = BCM_E_UNAVAIL;
    _bcm_flex_stat_t fs_arr[_bcmFlexStatNum]; /* Normalize stats */
    int                 idx;       /* Statistics iteration index. */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif

    L3_INIT(unit);

  if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);
    BCM_IF_ERROR_RETURN
        (_bcm_l3_vrf_stat_array_convert(unit, nstat, stat_arr, fs_arr));
    BCM_VRF_STAT_ARRAY_VALID(unit, nstat, value_arr);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_multi_get(unit, _bcmFlexStatTypeVrf, vrf,
                                      nstat, fs_arr, value_arr);
    L3_UNLOCK(unit);
    return rv; 
  } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    for (idx=0;idx < nstat ; idx ++) {
         BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_counter_get( 
                             unit, vrf, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if (stat_arr[idx] == bcmL3VrfStatIngressPackets) {
             COMPILER_64_SET(value_arr[idx],
                             COMPILER_64_HI(counter_values.packets64),
                             COMPILER_64_LO(counter_values.packets64));
         } else {
             COMPILER_64_SET(value_arr[idx],
                             COMPILER_64_HI(counter_values.bytes),
                             COMPILER_64_LO(counter_values.bytes));
         }
    }
    return  BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_multi_get32
 * Purpose:
 *      Get lower 32-bit counter value for multiple VRF statistic
 *      types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (OUT) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_multi_get32(int unit, bcm_vrf_t vrf, int nstat, 
                                bcm_l3_vrf_stat_t *stat_arr,
                                uint32 *value_arr)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int rv = BCM_E_UNAVAIL;
    _bcm_flex_stat_t fs_arr[_bcmFlexStatNum]; /* Normalize stats */
    int                 idx;       /* Statistics iteration index. */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif

    L3_INIT(unit);
  if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);
    BCM_IF_ERROR_RETURN
        (_bcm_l3_vrf_stat_array_convert(unit, nstat, stat_arr, fs_arr));
    BCM_VRF_STAT_ARRAY_VALID(unit, nstat, value_arr);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_multi_get32(unit, _bcmFlexStatTypeVrf, vrf,
                                        nstat, fs_arr, value_arr);
    L3_UNLOCK(unit);
    return rv;
  } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    for (idx=0;idx < nstat ; idx ++) {
         BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_counter_get(
                             unit, vrf, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if (stat_arr[idx] == bcmL3VrfStatIngressPackets) {
             value_arr[idx] = counter_values.packets;
         } else {
             value_arr[idx] = COMPILER_64_LO(counter_values.bytes);
         }
    }
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_multi_set
 * Purpose:
 *      Set 64-bit counter value for multiple VRF statistic types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (IN) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_multi_set(int unit, bcm_vrf_t vrf, int nstat, 
                              bcm_l3_vrf_stat_t *stat_arr, 
                              uint64 *value_arr)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int rv = BCM_E_UNAVAIL;
    _bcm_flex_stat_t fs_arr[_bcmFlexStatNum]; /* Normalize stats */
    int                 idx;       /* Statistics iteration index. */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif

    L3_INIT(unit);
  if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);
    BCM_IF_ERROR_RETURN
        (_bcm_l3_vrf_stat_array_convert(unit, nstat, stat_arr, fs_arr));
    BCM_VRF_STAT_ARRAY_VALID(unit, nstat, value_arr);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_multi_set(unit, _bcmFlexStatTypeVrf, vrf,
                                      nstat, fs_arr, value_arr);
    L3_UNLOCK(unit);
    return rv;
  } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    for (idx=0;idx < nstat ; idx ++) {
         if (stat_arr[idx] == bcmL3VrfStatIngressPackets) {
             counter_values.packets = COMPILER_64_LO(value_arr[idx]);
         } else {
             COMPILER_64_SET(counter_values.bytes,
                             COMPILER_64_HI(value_arr[idx]),
                             COMPILER_64_LO(value_arr[idx]));
         }
         BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_counter_set( 
                             unit, vrf, stat_arr[idx], 1, 
                             &counter_indexes, &counter_values));
    }
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_l3_vrf_stat_multi_set32
 * Purpose:
 *      Set lower 32-bit counter value for multiple VRF statistic
 *      types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      vrf - (IN) Virtual router instance.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (IN) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_vrf_stat_multi_set32(int unit, bcm_vrf_t vrf, int nstat,
                                bcm_l3_vrf_stat_t *stat_arr, 
                                uint32 *value_arr)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    int rv = BCM_E_UNAVAIL;
    _bcm_flex_stat_t fs_arr[_bcmFlexStatNum]; /* Normalize stats */
    int                 idx;       /* Statistics iteration index. */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    uint32           counter_indexes=0;
    bcm_stat_value_t counter_values={0};
#endif

    L3_INIT(unit);
 if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    BCM_VRF_STAT_VALID(unit, vrf);
    BCM_IF_ERROR_RETURN
        (_bcm_l3_vrf_stat_array_convert(unit, nstat, stat_arr, fs_arr));
    BCM_VRF_STAT_ARRAY_VALID(unit, nstat, value_arr);
    L3_LOCK(unit);

    rv = _bcm_esw_flex_stat_multi_set32(unit, _bcmFlexStatTypeVrf, vrf,
                                        nstat, fs_arr, value_arr);
    L3_UNLOCK(unit);
    return rv;
  } 
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    for (idx=0;idx < nstat ; idx ++) {
         if (stat_arr[idx] == bcmL3VrfStatIngressPackets) {
             counter_values.packets = value_arr[idx];
         } else {
             COMPILER_64_SET(counter_values.bytes,0,value_arr[idx]);
         }
         BCM_IF_ERROR_RETURN(bcm_esw_l3_vrf_stat_counter_set(
                             unit, vrf, stat_arr[idx], 1, 
                             &counter_indexes, &counter_values));
    }
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH2_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_l3_source_bind_enable_set
 * Purpose:
 *      Enable or disable l3 source binding checks on an ingress port.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) Packet ingress port.
 *      enable - (IN) Non-zero to enable l3 source binding checks,
 *               zero to disable.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_source_bind_enable_set(int unit, bcm_port_t port, int enable)
{
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    int key_type_value;
    int key_type;
    _bcm_port_config_t key_config, port_config;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    if (enable) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_get(unit, port,
                                _bcmPortVTKeyTypeSecond, &key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_get(unit,key_type_value,&key_type));

        if (key_type == VLXLT_HASH_KEY_TYPE_OVID) {
            /* Overwrite OVID if it's in second slot */
            key_config = _bcmPortVTKeyTypeSecond;
            port_config = _bcmPortVTKeyPortSecond;
        } else { /* Otherwise just use first slot */
            key_config = _bcmPortVTKeyTypeFirst;
            port_config = _bcmPortVTKeyPortFirst;
        }

        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_value_get(unit,
                     VLXLT_HASH_KEY_TYPE_HPAE,&key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port, key_config,
                                      key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_value_get(unit,
                     VLXLT_HASH_KEY_TYPE_OTAG,&key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port, port_config,
                                      key_type_value));
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_get(unit, port,
                               _bcmPortVTKeyTypeSecond, &key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_get(unit,key_type_value,&key_type));
        if (key_type == VLXLT_HASH_KEY_TYPE_HPAE) {
            key_config = _bcmPortVTKeyTypeSecond;
            port_config = _bcmPortVTKeyPortSecond;
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_esw_port_config_get(unit, port,
                                          _bcmPortVTKeyTypeFirst,
                                          &key_type_value));
            BCM_IF_ERROR_RETURN
                (_bcm_esw_pt_vtkey_type_get(unit,
                                 key_type_value,&key_type));
            if (key_type == VLXLT_HASH_KEY_TYPE_HPAE) {
                key_config = _bcmPortVTKeyTypeFirst;
                port_config = _bcmPortVTKeyPortFirst;
            } else {
                /* Already not enabled */
                return BCM_E_NONE;
            }
        }
        /* Return to OVID usage */
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_value_get(unit,
                     VLXLT_HASH_KEY_TYPE_OVID,&key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port, key_config,
                                      key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_value_get(unit,
                     VLXLT_HASH_KEY_TYPE_OTAG,&key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port, port_config,
                                      key_type_value));
    }
    return BCM_E_NONE; 
#else
    return BCM_E_UNAVAIL; 
#endif
}

/*
 * Function:
 *      bcm_eswl3_source_bind_enable_get
 * Purpose:
 *      Retrieve whether l3 source binding checks are performed on an
 *      ingress port.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) Packet ingress port.
 *      enable - (OUT) Non-zero if l3 source binding checks are enabled,
 *               zero if disabled.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_source_bind_enable_get(int unit, bcm_port_t port, int *enable)
{
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    int key_type;
    int key_type_value;
    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_port_config_get(unit, port,
                                  _bcmPortVTKeyTypeSecond, &key_type_value));
    BCM_IF_ERROR_RETURN
        (_bcm_esw_pt_vtkey_type_get(unit,key_type_value,&key_type));

    if (key_type == VLXLT_HASH_KEY_TYPE_HPAE) {
        *enable = TRUE;
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_get(unit, port,
                                _bcmPortVTKeyTypeFirst, &key_type_value));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_get(unit,key_type_value,&key_type));
        if (key_type == VLXLT_HASH_KEY_TYPE_HPAE) {
            *enable = TRUE;
        } else {
            *enable = FALSE;
        }
    }

    return BCM_E_NONE; 
#else
    return BCM_E_UNAVAIL; 
#endif
}
 
#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int 
_tr3_l3_source_bind_add(int unit, bcm_l3_source_bind_t *info)
{
    int             tmp_id, rv, idx = 0;
    bcm_module_t    modid_out;
    bcm_port_t      port_out;
    bcm_trunk_t     trunk_out;
    vlan_xlate_extd_entry_t vxent, res_vxent;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    if (info->flags & BCM_L3_SOURCE_BIND_IP6) {
        /* Not supported yet */
        return BCM_E_UNAVAIL;
    }

    sal_memset(&vxent, 0, sizeof(vlan_xlate_extd_entry_t));
    sal_memset(&res_vxent, 0, sizeof(vlan_xlate_extd_entry_t));

    /* MAC_IP_BIND lookup key type */
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_0f, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_1f, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, KEY_TYPE_0f,
                              TR3_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, KEY_TYPE_1f,
                              TR3_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent,
                              MAC_IP_BIND__SIPf, info->ip);

    rv = soc_mem_search(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, &idx, 
                        &vxent, &res_vxent, 0);

    if (BCM_SUCCESS(rv)) {
        if (soc_VLAN_XLATE_EXTDm_field32_get(unit, &res_vxent, VALID_0f)) {
            /* Valid entry found */
            if ((info->flags & BCM_L3_SOURCE_BIND_REPLACE) == 0) {
                /* Entry exists! */
                return BCM_E_EXISTS;
            }
        }
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    }

    if (info->port == BCM_GPORT_INVALID) {
        /* wild card */
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent,
                                  MAC_IP_BIND__MODULE_IDf, 0x7f);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent,
                                  MAC_IP_BIND__Tf, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent,
                                  MAC_IP_BIND__PORT_NUMf, 0x3f);
    } else if (BCM_GPORT_IS_SET(info->port)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, info->port, &modid_out,
                                    &port_out, &trunk_out, &tmp_id));
        if (-1 != tmp_id) {
            return BCM_E_PARAM;
        }
        if (BCM_TRUNK_INVALID != trunk_out) {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent,
                                      MAC_IP_BIND__Tf, 1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, 
                    MAC_IP_BIND__MODULE_IDf, (trunk_out >> 6) & 0x1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, 
                    MAC_IP_BIND__PORT_NUMf, trunk_out & 0x3f);
        } else {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, 
                     MAC_IP_BIND__MODULE_IDf, modid_out);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, 
                     MAC_IP_BIND__PORT_NUMf, port_out);
        }
    } else {
        return BCM_E_PORT;
    }

    soc_VLAN_XLATE_EXTDm_mac_addr_set(unit, &vxent,
                               MAC_IP_BIND__MAC_ADDRf, info->mac);

    if (soc_feature(unit, soc_feature_ipfix_rate) && info->rate_id != 0) {
        soc_VLAN_XLATE_EXTDm_field32_set
            (unit, &vxent, MAC_IP_BIND__IPFIX_FLOW_METER_IDf, info->rate_id);
    }

    rv = soc_mem_insert(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, &vxent);

    if ((rv == SOC_E_EXISTS) &&
        (info->flags & BCM_L3_SOURCE_BIND_REPLACE)) {
        rv = BCM_E_NONE;
    }

    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
STATIC int 
_tr2_l3_source_bind_add(int unit, bcm_l3_source_bind_t *info)
{
    int             tmp_id, rv, idx = 0;
    bcm_module_t    modid_out;
    bcm_port_t      port_out;
    bcm_trunk_t     trunk_out;
    vlan_mac_entry_t vment, res_vment;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    if (info->flags & BCM_L3_SOURCE_BIND_IP6) {
        /* Not supported yet */
        return BCM_E_UNAVAIL;
    }

    sal_memset(&vment, 0, sizeof(vlan_mac_entry_t));
    sal_memset(&res_vment, 0, sizeof(vlan_mac_entry_t));

    /* MAC_IP_BIND lookup key type */
    soc_VLAN_MACm_field32_set(unit, &vment, VALIDf, 1);
    soc_VLAN_MACm_field32_set(unit, &vment, KEY_TYPEf,
                              TR_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_MACm_field32_set(unit, &vment,
                              MAC_IP_BIND__SIPf, info->ip);

    rv = soc_mem_search(unit, VLAN_MACm, MEM_BLOCK_ALL, &idx, 
                        &vment, &res_vment, 0);

    if (BCM_SUCCESS(rv)) {
        if (soc_VLAN_MACm_field32_get(unit, &res_vment, VALIDf)) {
            /* Valid entry found */
            if ((info->flags & BCM_L3_SOURCE_BIND_REPLACE) == 0) {
                /* Entry exists! */
                return BCM_E_EXISTS;
            }
        }
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    }

    if (info->port == BCM_GPORT_INVALID) {
        /* wild card */
        soc_VLAN_MACm_field32_set(unit, &vment,
                                  MAC_IP_BIND__SRC_MODIDf, 0x7f);
        soc_VLAN_MACm_field32_set(unit, &vment,
                                  MAC_IP_BIND__SRC_Tf, 1);
        soc_VLAN_MACm_field32_set(unit, &vment,
                                  MAC_IP_BIND__SRC_PORTf, 0x3f);
    } else if (BCM_GPORT_IS_SET(info->port)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, info->port, &modid_out,
                                    &port_out, &trunk_out, &tmp_id));
        if (-1 != tmp_id) {
            return BCM_E_PARAM;
        }
        if (BCM_TRUNK_INVALID != trunk_out) {
            soc_VLAN_MACm_field32_set(unit, &vment,
                                      MAC_IP_BIND__SRC_Tf, 1);
            soc_VLAN_MACm_field32_set(unit, &vment, MAC_IP_BIND__SRC_MODIDf,
                                      (trunk_out >> 6) & 0x1);
            soc_VLAN_MACm_field32_set(unit, &vment, MAC_IP_BIND__SRC_PORTf,
                                      trunk_out & 0x3f);
        } else {
            soc_VLAN_MACm_field32_set(unit, &vment, MAC_IP_BIND__SRC_MODIDf,
                                      modid_out);
            soc_VLAN_MACm_field32_set(unit, &vment, MAC_IP_BIND__SRC_PORTf,
                                      port_out);
        }
    } else {
        return BCM_E_PORT;
    }

    soc_VLAN_MACm_mac_addr_set(unit, &vment,
                               MAC_IP_BIND__HPAE_MAC_ADDRf, info->mac);

    if (soc_feature(unit, soc_feature_ipfix_rate) && info->rate_id != 0) {
        soc_VLAN_MACm_field32_set
            (unit, &vment, MAC_IP_BIND__IPFIX_FLOW_METER_IDf, info->rate_id);
    }

    rv = soc_mem_insert(unit, VLAN_MACm, MEM_BLOCK_ALL, &vment);

    if ((rv == SOC_E_EXISTS) &&
        (info->flags & BCM_L3_SOURCE_BIND_REPLACE)) {
        rv = BCM_E_NONE;
    }

    return rv;
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      bcm_esw_l3_source_bind_add
 * Purpose:
 *      Add or replace an L3 source binding.
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN) L3 source binding information
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_l3_source_bind_add(int unit, bcm_l3_source_bind_t *info)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr3_l3_source_bind_add(unit,info));
        return BCM_E_NONE;
    } 
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr2_l3_source_bind_add(unit,info));
        return BCM_E_NONE;
    } 
#endif
    return BCM_E_UNAVAIL; 
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_tr3_l3_source_bind_delete(int unit, bcm_l3_source_bind_t *info)
{
    vlan_xlate_extd_entry_t vxent;
    int rv;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    if (info->flags & BCM_L3_SOURCE_BIND_IP6) {
        /* Not supported yet */
        return BCM_E_UNAVAIL;
    }

    sal_memset(&vxent, 0, sizeof(vxent));

    /* MAC_IP_BIND key type */
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_0f, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_1f, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, KEY_TYPE_0f,
                              TR3_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, KEY_TYPE_1f,
                              TR3_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent,
                              MAC_IP_BIND__SIPf, info->ip);

    rv = soc_mem_delete(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, &vxent);

    if (rv == SOC_E_NOT_FOUND) {
        rv = SOC_E_NONE;
    }
    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
STATIC int 
_tr2_l3_source_bind_delete(int unit, bcm_l3_source_bind_t *info)
{
    vlan_mac_entry_t vment;
    int rv; 

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    if (info->flags & BCM_L3_SOURCE_BIND_IP6) {
        /* Not supported yet */
        return BCM_E_UNAVAIL;
    }

    sal_memset(&vment, 0, sizeof(vment));

    /* MAC_IP_BIND key type */
    soc_VLAN_MACm_field32_set(unit, &vment, VALIDf, 1);
    soc_VLAN_MACm_field32_set(unit, &vment, KEY_TYPEf,
                              TR_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_MACm_field32_set(unit, &vment,
                              MAC_IP_BIND__SIPf, info->ip);

    rv = soc_mem_delete(unit, VLAN_MACm, MEM_BLOCK_ALL, &vment);

    if (rv == SOC_E_NOT_FOUND) {
        rv = SOC_E_NONE;
    }
    return rv;
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      bcm_esw_l3_source_bind_delete
 * Purpose:
 *      Remove an existing L3 source binding.
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN) L3 source binding information
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_l3_source_bind_delete(int unit, bcm_l3_source_bind_t *info)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr3_l3_source_bind_delete(unit,info));
        return BCM_E_NONE;
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr2_l3_source_bind_delete(unit,info));
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;

}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int 
_tr3_l3_source_bind_delete_all(int unit)
{
    int idx, imin, imax, nent, vmbytes, rv;
    vlan_xlate_extd_entry_t * vxtab, *vxtabp;
    
    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    imin = soc_mem_index_min(unit, VLAN_XLATE_EXTDm);
    imax = soc_mem_index_max(unit, VLAN_XLATE_EXTDm);
    nent = soc_mem_index_count(unit, VLAN_XLATE_EXTDm);
    vmbytes = soc_mem_entry_words(unit, VLAN_XLATE_EXTDm);
    vmbytes = WORDS2BYTES(vmbytes);
    vxtab = soc_cm_salloc(unit, nent * sizeof(*vxtab), "vlan_xlate_extd");

    if (vxtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    soc_mem_lock(unit, VLAN_XLATE_EXTDm);
    rv = soc_mem_read_range(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY,
                            imin, imax, vxtab);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
        soc_cm_sfree(unit, vxtab);
        return rv; 
    }
    
    for(idx = 0; idx < nent; idx++) {
        vxtabp = soc_mem_table_idx_to_pointer(unit, VLAN_XLATE_EXTDm,
                                              vlan_xlate_extd_entry_t *, 
                                              vxtab, idx);

        if ((0 == soc_VLAN_XLATE_EXTDm_field32_get(unit, vxtabp, VALID_0f)) ||
            (TR3_VLXLT_HASH_KEY_TYPE_HPAE !=
             soc_VLAN_XLATE_EXTDm_field32_get(unit, vxtabp, KEY_TYPE_0f))) {
            continue;
        }

        rv = soc_mem_delete(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, vxtabp);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
            soc_cm_sfree(unit, vxtab);
            return rv; 
        }
    }
    
    soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
    soc_cm_sfree(unit, vxtab);
    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
STATIC int 
_tr2_l3_source_bind_delete_all(int unit)
{
    int idx, imin, imax, nent, vmbytes, rv;
    vlan_mac_entry_t * vmtab, *vmtabp;
    
    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    imin = soc_mem_index_min(unit, VLAN_MACm);
    imax = soc_mem_index_max(unit, VLAN_MACm);
    nent = soc_mem_index_count(unit, VLAN_MACm);
    vmbytes = soc_mem_entry_words(unit, VLAN_MACm);
    vmbytes = WORDS2BYTES(vmbytes);
    vmtab = soc_cm_salloc(unit, nent * sizeof(*vmtab), "vlan_mac");

    if (vmtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    soc_mem_lock(unit, VLAN_MACm);
    rv = soc_mem_read_range(unit, VLAN_MACm, MEM_BLOCK_ANY,
                            imin, imax, vmtab);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_MACm);
        soc_cm_sfree(unit, vmtab);
        return rv; 
    }
    
    for(idx = 0; idx < nent; idx++) {
        vmtabp = soc_mem_table_idx_to_pointer(unit, VLAN_MACm,
                                              vlan_mac_entry_t *, 
                                              vmtab, idx);

        if ((0 == soc_VLAN_MACm_field32_get(unit, vmtabp, VALIDf)) ||
            (TR_VLXLT_HASH_KEY_TYPE_HPAE !=
             soc_VLAN_MACm_field32_get(unit, vmtabp, KEY_TYPEf))) {
            continue;
        }

        rv = soc_mem_delete(unit, VLAN_MACm, MEM_BLOCK_ALL, vmtabp);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, VLAN_MACm);
            soc_cm_sfree(unit, vmtab);
            return rv; 
        }
    }
    
    soc_mem_unlock(unit, VLAN_MACm);
    soc_cm_sfree(unit, vmtab);
    return rv;
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      bcm_esw_l3_source_bind_delete_all
 * Purpose:
 *      Remove all existing L3 source bindings.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_l3_source_bind_delete_all(int unit)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr3_l3_source_bind_delete_all(unit));
        return BCM_E_NONE;
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr2_l3_source_bind_delete_all(unit));
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_tr3_l3_source_bind_hw_entry_to_sw_info(int unit,
                                        vlan_xlate_extd_entry_t *vxent,
                                        bcm_l3_source_bind_t *info)
{
    int             t_bit;
    bcm_module_t    modid;
    bcm_port_t      port;

    bcm_l3_source_bind_t_init(info);
    info->ip = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxent, 
                      MAC_IP_BIND__SIPf);
    if (soc_feature(unit, soc_feature_ipfix_rate)) {
        info->rate_id = soc_VLAN_XLATE_EXTDm_field32_get
            (unit, vxent, MAC_IP_BIND__IPFIX_FLOW_METER_IDf);
    }
    soc_VLAN_XLATE_EXTDm_mac_addr_get(unit, vxent,
                               MAC_IP_BIND__MAC_ADDRf, info->mac);

    /* Port analysis */

    port = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxent,
                                           MAC_IP_BIND__PORT_NUMf);
    modid = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxent,
                                      MAC_IP_BIND__MODULE_IDf);
    t_bit = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxent,
                                    MAC_IP_BIND__Tf);

    if (t_bit == 1) {
        if ((modid == 0x7f) && (port == 0x3f)) {
            info->port = BCM_GPORT_TYPE_BLACK_HOLE; /* Wild card */
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_esw_l3_gport_construct(unit, 0, 0,
                                   (port & 0x3f) | ((modid & 0x1) << 6),
                                             BCM_L3_TGID, &(info->port)));
        }
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_l3_gport_construct(unit, modid, port,
                                         0, 0, &(info->port)));
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
STATIC int
_tr2_l3_source_bind_hw_entry_to_sw_info(int unit,
                                            vlan_mac_entry_t *vment,
                                            bcm_l3_source_bind_t *info)
{
    int             t_bit;
    bcm_module_t    modid;
    bcm_port_t      port;

    bcm_l3_source_bind_t_init(info);
    info->ip = soc_VLAN_MACm_field32_get(unit, vment, MAC_IP_BIND__SIPf);
    if (soc_feature(unit, soc_feature_ipfix_rate)) {
        info->rate_id = soc_VLAN_MACm_field32_get
            (unit, vment, MAC_IP_BIND__IPFIX_FLOW_METER_IDf);
    }
    soc_VLAN_MACm_mac_addr_get(unit, vment,
                               MAC_IP_BIND__HPAE_MAC_ADDRf, info->mac);

    /* Port analysis */

    port = soc_VLAN_MACm_field32_get(unit, vment,
                                           MAC_IP_BIND__SRC_PORTf);
    modid = soc_VLAN_MACm_field32_get(unit, vment,
                                      MAC_IP_BIND__SRC_MODIDf);
    t_bit = soc_VLAN_MACm_field32_get(unit, vment,
                                    MAC_IP_BIND__SRC_Tf);

    if (t_bit == 1) {
        if ((modid == 0x7f) && (port == 0x3f)) {
            info->port = BCM_GPORT_TYPE_BLACK_HOLE; /* Wild card */
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_esw_l3_gport_construct(unit, 0, 0,
                                   (port & 0x3f) | ((modid & 0x1) << 6),
                                             BCM_L3_TGID, &(info->port)));
        }
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_l3_gport_construct(unit, modid, port,
                                         0, 0, &(info->port)));
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_tr3_l3_source_bind_get(int unit, bcm_l3_source_bind_t *info)
{
    int             rv, idx = 0;
    vlan_xlate_extd_entry_t vxent, res_vxent;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    if (info->flags & BCM_L3_SOURCE_BIND_IP6) {
        /* Not supported yet */
        return BCM_E_UNAVAIL;
    }

    sal_memset(&vxent, 0, sizeof(vlan_xlate_extd_entry_t));
    sal_memset(&res_vxent, 0, sizeof(vlan_xlate_extd_entry_t));

    /* MAC_IP_BIND lookup key type */
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_0f, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_1f, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, KEY_TYPE_0f,
                              TR3_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, KEY_TYPE_1f,
                              TR3_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent,
                              MAC_IP_BIND__SIPf, info->ip);

    soc_mem_lock(unit, VLAN_XLATE_EXTDm);
    rv = soc_mem_search(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, &idx,
                        &vxent, &res_vxent, 0);
    soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
    BCM_IF_ERROR_RETURN(rv);

    return _tr3_l3_source_bind_hw_entry_to_sw_info(unit, &res_vxent,
                                                       info);
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
STATIC int 
_tr2_l3_source_bind_get(int unit, bcm_l3_source_bind_t *info)
{
    int             rv, idx = 0;
    vlan_mac_entry_t vment, res_vment;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    if (info->flags & BCM_L3_SOURCE_BIND_IP6) {
        /* Not supported yet */
        return BCM_E_UNAVAIL;
    }

    sal_memset(&vment, 0, sizeof(vlan_mac_entry_t));
    sal_memset(&res_vment, 0, sizeof(vlan_mac_entry_t));

    /* MAC_IP_BIND lookup key type */
    soc_VLAN_MACm_field32_set(unit, &vment, VALIDf, 1);
    soc_VLAN_MACm_field32_set(unit, &vment, KEY_TYPEf,
                              TR_VLXLT_HASH_KEY_TYPE_HPAE);
    soc_VLAN_MACm_field32_set(unit, &vment,
                              MAC_IP_BIND__SIPf, info->ip);

    soc_mem_lock(unit, VLAN_MACm);
    rv = soc_mem_search(unit, VLAN_MACm, MEM_BLOCK_ALL, &idx, 
                        &vment, &res_vment, 0);
    soc_mem_unlock(unit, VLAN_MACm);
    BCM_IF_ERROR_RETURN(rv);

    return _tr2_l3_source_bind_hw_entry_to_sw_info(unit, &res_vment,
                                                       info);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      bcm_esw_l3_source_bind_get
 * Purpose:
 *      Retrieve the details of an existing L3 source binding.
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN/OUT) L3 source binding information
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_l3_source_bind_get(int unit, bcm_l3_source_bind_t *info)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr3_l3_source_bind_get(unit,info));
        return BCM_E_NONE;
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr2_l3_source_bind_get(unit,info));
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int 
_tr3_l3_source_bind_traverse(int unit, bcm_l3_source_bind_traverse_cb cb, 
                                void *user_data)
{
    int idx, imin, imax, nent, vmbytes, rv;
    vlan_xlate_extd_entry_t * vxtab, *vxtabp;
    bcm_l3_source_bind_t info;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    /* Input parameters check. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    imin = soc_mem_index_min(unit, VLAN_XLATE_EXTDm);
    imax = soc_mem_index_max(unit, VLAN_XLATE_EXTDm);
    nent = soc_mem_index_count(unit, VLAN_XLATE_EXTDm);
    vmbytes = soc_mem_entry_words(unit, VLAN_XLATE_EXTDm);
    vmbytes = WORDS2BYTES(vmbytes);
    vxtab = soc_cm_salloc(unit, nent * sizeof(*vxtab), "vlan_xlate_extd");

    if (vxtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    soc_mem_lock(unit, VLAN_XLATE_EXTDm);
    rv = soc_mem_read_range(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY,
                            imin, imax, vxtab);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
        soc_cm_sfree(unit, vxtab);
        return rv; 
    }
    
    for(idx = 0; idx < nent; idx++) {
        vxtabp = soc_mem_table_idx_to_pointer(unit, VLAN_XLATE_EXTDm,
                                              vlan_xlate_extd_entry_t *, 
                                              vxtab, idx);

        if ((0 == soc_VLAN_XLATE_EXTDm_field32_get(unit, vxtabp, VALID_0f)) ||
            (TR3_VLXLT_HASH_KEY_TYPE_HPAE !=
             soc_VLAN_XLATE_EXTDm_field32_get(unit, vxtabp, KEY_TYPE_0f))) {
            continue;
        }

        rv = _tr3_l3_source_bind_hw_entry_to_sw_info(unit, vxtabp,
                                                         &info);

        if (BCM_SUCCESS(rv)) {
            /* Call traverse callback with the data. */
            rv = cb(unit, &info, user_data);
        }

        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
            soc_cm_sfree(unit, vxtab);
            return rv; 
        }
    }
    
    soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
    soc_cm_sfree(unit, vxtab);
    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
STATIC int 
_tr2_l3_source_bind_traverse(int unit, bcm_l3_source_bind_traverse_cb cb, 
                                void *user_data)
{
    int idx, imin, imax, nent, vmbytes, rv;
    vlan_mac_entry_t * vmtab, *vmtabp;
    bcm_l3_source_bind_t info;

    if (!soc_feature(unit, soc_feature_ip_source_bind)) {
        return BCM_E_UNAVAIL;
    }

    /* Input parameters check. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    imin = soc_mem_index_min(unit, VLAN_MACm);
    imax = soc_mem_index_max(unit, VLAN_MACm);
    nent = soc_mem_index_count(unit, VLAN_MACm);
    vmbytes = soc_mem_entry_words(unit, VLAN_MACm);
    vmbytes = WORDS2BYTES(vmbytes);
    vmtab = soc_cm_salloc(unit, nent * sizeof(*vmtab), "vlan_mac");

    if (vmtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    soc_mem_lock(unit, VLAN_MACm);
    rv = soc_mem_read_range(unit, VLAN_MACm, MEM_BLOCK_ANY,
                            imin, imax, vmtab);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_MACm);
        soc_cm_sfree(unit, vmtab);
        return rv; 
    }
    
    for(idx = 0; idx < nent; idx++) {
        vmtabp = soc_mem_table_idx_to_pointer(unit, VLAN_MACm,
                                              vlan_mac_entry_t *, 
                                              vmtab, idx);

        if ((0 == soc_VLAN_MACm_field32_get(unit, vmtabp, VALIDf)) ||
            (TR_VLXLT_HASH_KEY_TYPE_HPAE !=
             soc_VLAN_MACm_field32_get(unit, vmtabp, KEY_TYPEf))) {
            continue;
        }

        rv = _tr2_l3_source_bind_hw_entry_to_sw_info(unit, vmtabp,
                                                         &info);

        if (BCM_SUCCESS(rv)) {
            /* Call traverse callback with the data. */
            rv = cb(unit, &info, user_data);
        }

        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, VLAN_MACm);
            soc_cm_sfree(unit, vmtab);
            return rv; 
        }
    }
    
    soc_mem_unlock(unit, VLAN_MACm);
    soc_cm_sfree(unit, vmtab);
    return rv;
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      bcm_esw_l3_source_bind_traverse
 * Purpose:
 *      Traverse through the L3 source bindings and run callback at
 *      each defined binding.
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) Call back routine.
 *      user_data - (IN) User provided cookie for callback.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_l3_source_bind_traverse(int unit, bcm_l3_source_bind_traverse_cb cb,
                                void *user_data)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr3_l3_source_bind_traverse(unit,cb,user_data));
        return BCM_E_NONE;
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit)) {
        BCM_IF_ERROR_RETURN
            (_tr2_l3_source_bind_traverse(unit,cb,user_data));
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;
}

#ifdef BCM_WARM_BOOT_SUPPORT
int
_bcm_l3_cleanup(int unit)
{

    return(mbcm_driver[unit]->mbcm_l3_tables_cleanup(unit));
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      bcm_esw_l3_host_stat_attach
 * Description:
 *      Attach counters entries to the given L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description  
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 */
int bcm_esw_l3_host_stat_attach(
            int unit,
            bcm_l3_host_t *info,
            uint32 stat_counter_id)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return _bcm_td2_l3_host_stat_attach(unit, info, stat_counter_id);
    } else
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_l3_host_stat_attach(unit, info, stat_counter_id);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_l3_host_stat_detach
 * Description:
 *      Detach counters entries to the given L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info         - (IN) L3 host description
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_host_stat_detach(
            int unit,
            bcm_l3_host_t *info)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {    
        return _bcm_td2_l3_host_stat_detach(unit, info);
    } else
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_l3_host_stat_detach(unit, info);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      _bcm_esw_l3_host_stat_counter_get
 * Description:
 *      Get counter statistic values for specific L3 host entry
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.  
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int _bcm_esw_l3_host_stat_counter_get(
            int unit,
            int sync_mode,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values)
{

#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) { 
        return _bcm_td2_l3_host_stat_counter_get(unit, sync_mode, info, 
                    stat, num_entries, counter_indexes, counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_l3_host_stat_counter_get(unit, sync_mode, info, 
                    stat, num_entries, counter_indexes, counter_values);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}


/*
 * Function:
 *      bcm_esw_l3_host_stat_counter_get
 * Description:
 *      Get counter statistic values for specific L3 host entry
 *      
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_host_stat_counter_get(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values)
{
     int rv = BCM_E_UNAVAIL;

     rv = _bcm_esw_l3_host_stat_counter_get(unit, 0, info, stat, 
                                            num_entries, counter_indexes, 
                                            counter_values);

     return rv;
}


/*
 * Function:
 *      bcm_esw_l3_host_stat_counter_sync_get
 * Description:
 *      Get counter statistic values for specific L3 host entry
 *      sw accumulated counters synced with hw count.
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_host_stat_counter_sync_get(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values)
{
     int rv = BCM_E_UNAVAIL;

     rv = _bcm_esw_l3_host_stat_counter_get(unit, 1, info, stat, 
                                            num_entries, counter_indexes, 
                                            counter_values);

     return rv;
}     

/*
 * Function:
 *      bcm_esw_l3_host_stat_counter_set
 * Description:
 *      Set counter statistic values for specific L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (IN) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_host_stat_counter_set(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return _bcm_td2_l3_host_stat_counter_set(unit, info, stat,
                    num_entries, counter_indexes, counter_values);
    } else
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_l3_host_stat_counter_set(unit, info, stat,
                    num_entries, counter_indexes, counter_values);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_l3_host_stat_id_get
 * Description:
 *      Get stat counter id associated with given L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      Stat_counter_id  - (OUT) Stat Counter ID
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_l3_host_stat_id_get(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 *stat_counter_id)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) { 
        return _bcm_td2_l3_host_stat_id_get(unit, info, stat, stat_counter_id);
    } else
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_l3_host_stat_id_get(unit, info, stat, stat_counter_id);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_attach
 * Description:
 *      Attach counters entries to the given  l3 route defip index . Allocate
 *      flex counter and assign counter index and offset for the index pointed
 *      by l3 defip index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 Route Info
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
    
int  bcm_esw_l3_route_stat_attach (
                            int                 unit,
                            bcm_l3_route_t      *info,
                            uint32              stat_counter_id)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
        rv = _bcm_td2_l3_route_stat_attach (
                    unit, info, stat_counter_id);

        L3_UNLOCK(unit);
    } 
#endif
    return rv;
}


/*
 * Function:
 *      bcm_esw_l3_route_stat_detach
 * Description:
 *      Detach counter entries to the given  l3 route index. 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 Route Info
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_route_stat_detach (
                            int                 unit,
                            bcm_l3_route_t      *info)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
        rv = _bcm_td2_l3_route_stat_detach (unit, info);
        L3_UNLOCK(unit);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      _bcm_esw_l3_route_stat_counter_get
 *
 * Description:
 *  Get l3 route counter value for specified l3 route index
 *  if sync_mode is set, sync the sw accumulated count
 *  with hw count value first, else return sw count.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count
 *      info             - (IN)  L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  _bcm_esw_l3_route_stat_counter_get(
                            int                 unit, 
                            int                 sync_mode,
                            bcm_l3_route_t      *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
        rv = _bcm_td2_l3_route_stat_counter_get (
                    unit, sync_mode, info, stat, num_entries, 
                    counter_indexes, counter_values);
        L3_UNLOCK(unit);
    } 
#endif
    return rv;
}


/*
 * Function:
 *      bcm_esw_l3_route_stat_counter_get
 *
 * Description:
 *  Get l3 route counter value for specified l3 route index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN)  L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_route_stat_counter_get(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_route_stat_counter_get(unit, 0, info, stat,
                                            num_entries, counter_indexes, 
                                            counter_values);
    return rv;                                            
}



/*
 * Function:
 *      bcm_esw_l3_route_stat_counter_sync_get
 *
 * Description:
 *  Get l3 route counter value for specified l3 route index
 *  sw accumulated counters synced with hw count.
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN)  L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_route_stat_counter_sync_get(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_esw_l3_route_stat_counter_get(unit, 1, info, stat,
                                            num_entries, counter_indexes, 
                                            counter_values);
    return rv;                                     
}    
/*
 * Function:
 *      bcm_esw_l3_route_stat_counter_set
 *
 * Description:
 *  Set l3 route counter value for specified l3 route index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_route_stat_counter_set(
                            int                 unit, 
                            bcm_l3_route_t      *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
    int rv = BCM_E_UNAVAIL;

    if (counter_indexes == NULL || counter_values == NULL) {
        return BCM_E_PARAM;
    }

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
        rv = _bcm_td2_l3_route_stat_counter_set (
                    unit, info, stat, num_entries, 
                    counter_indexes, counter_values);
        L3_UNLOCK(unit);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_multi_get
 *
 * Description:
 *  Get Multiple l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 Route Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

    
int  bcm_esw_l3_route_stat_multi_get(
                            int                 unit, 
                            bcm_l3_route_t      *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint64              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
        rv = _bcm_td2_l3_route_stat_multi_get (
                    unit, info, nstat, stat_arr, value_arr);
        L3_UNLOCK(unit);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_multi_get32
 *
 * Description:
 *  Get 32bit l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 Route Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_route_stat_multi_get32(
                            int                 unit, 
                            bcm_l3_route_t      *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint32              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
       rv = _bcm_td2_l3_route_stat_multi_get32 (
                    unit, info, nstat, stat_arr, value_arr);
        L3_UNLOCK(unit);
    } 
#endif
    return rv;
}

/*
 * Function:
 *     bcm_esw_l3_route_stat_multi_set
 *
 * Description:
 *  Set l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_route_stat_multi_set(
                            int                 unit, 
                            bcm_l3_route_t      *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint64              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
        rv = _bcm_td2_l3_route_stat_multi_set (
                    unit, info, nstat, stat_arr, value_arr);
        L3_UNLOCK(unit);
    } 
#endif
    return rv;

}

/*
 * Function:
 *      bcm_esw_l3_route_stat_multi_set32
 *
 * Description:
 *  Set 32bit l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

int  bcm_esw_l3_route_stat_multi_set32(
                            int                 unit, 
                            bcm_l3_route_t      *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint32              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        L3_LOCK(unit);
        rv = _bcm_td2_l3_route_stat_multi_set32 (
                    unit, info, nstat, stat_arr, value_arr);
        L3_UNLOCK(unit);
    } 
#endif
    return rv;
}

/*
 * Function: 
 *      bcm_esw_l3_route_stat_id_get
 *
 * Description: 
 *      Get Stat Counter if associated with given l3 route index
 *
 * Parameters: 
 *      unit             - (IN) bcm device
 *      info             - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

int  bcm_esw_l3_route_stat_id_get(
                            int                 unit,
                            bcm_l3_route_t      *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              *stat_counter_id) 
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_route_stat_id_get (
                    unit, info, stat, stat_counter_id);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_enable_set
 * Purpose:
 *      Enable/disable packet and byte counters for the selected route
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN) Route info
 *      enable - (IN) Non-zero to enable counter collection, zero to disable.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_route_stat_enable_set(int unit, bcm_l3_route_t *info, int enable)
{
    int     rv = BCM_E_UNAVAIL;

    /* Old Flex API interfaces are not supported for L3 Route */
    
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_get
 * Purpose:
 *      Get 64-bit counter value for specified L3 Route Types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN) Route info
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_route_stat_get(int unit, bcm_l3_route_t *info, bcm_l3_stat_t stat,
                        uint64 *val)
{
    int     rv = BCM_E_UNAVAIL;

    /* Old Flex API interfaces are not supported for L3 route */
    
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_get32
 * Purpose:
 *      Get 32-bit counter value for specified L3 Route Types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN) Route info
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_route_stat_get32(int unit, bcm_l3_route_t *info, bcm_l3_stat_t stat,
                        uint32 *val)
{
    int     rv = BCM_E_UNAVAIL;

    /* Old Flex API interfaces are not supported for L3 route */
    
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_set
 * Purpose:
 *      Get 64-bit counter value for specified L3 Route Types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN) Route info
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_route_stat_set(int unit, bcm_l3_route_t *info, bcm_l3_stat_t stat,
                        uint64 val)
{
    int     rv = BCM_E_UNAVAIL;

    /* Old Flex API interfaces are not supported for L3 route */
    
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_route_stat_set32
 * Purpose:
 *      Get 32-bit counter value for specified L3 Route Types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      info - (IN) Route info
 *      stat - (IN) Type of the counter to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_l3_route_stat_set32(int unit, bcm_l3_route_t *info, bcm_l3_stat_t stat,
                        uint32 val)
{
    int     rv = BCM_E_UNAVAIL;

    /* Old Flex API interfaces are not supported for L3 route */
    
    return rv;
}


/*
 * Function:
 *      bcm_esw_l3_ip4_options_profile_create
 * Purpose:
 *      Create an IP option handling profile.
 * Parameters:
 *      unit    - (IN)  bcm device.
 *      flags   - (IN)  BCM_L3_WITH_ID: ip4_options_profile_id argument given.
 *      default_action - (IN) Default action for options in the profile
 *      ip4_options_profile_id - (OUT) L3 IP options profile ID
 *                      This is an IN argument if BCM_L3_WITH_ID is given in 
 *                      flag.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_ip4_options_profile_create(int unit, uint32 flags, 
                         bcm_l3_ip4_options_action_t default_action, 
                         int *ip4_options_profile_id)
{
    int rv = BCM_E_UNAVAIL;
    if (ip4_options_profile_id == NULL) {
        return BCM_E_PARAM;
    }
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
        rv = _bcm_td2_l3_ip4_options_profile_create(unit, flags, 
                         default_action, ip4_options_profile_id);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_ip4_options_profile_destroy
 * Purpose:
 *      Delete a IP option handling profile.
 * Parameters:
 *      unit    - (IN)  bcm device.
 *      ip4_options_profile_id - (IN) L3 IP options profile ID
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_ip4_options_profile_destroy(int unit, int ip4_options_profile_id)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
        rv = _bcm_td2_l3_ip4_options_profile_destroy(unit, ip4_options_profile_id);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return (rv);
}


/*
 * Function:
 *      bcm_esw_l3_ip4_options_action_set
 * Purpose:
 *      Update action for given IP action and Profile ID 
 * Parameters:
 *      unit                   - (IN)  bcm device.
 *      ip4_options_profile_id - (IN)  IP options profile ID
 *      ip4_option             - (IN)  New Action for IP Options
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_ip4_options_action_set(int unit,
                         int ip4_options_profile_id, 
                         int ip4_option, 
                         bcm_l3_ip4_options_action_t action)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
        rv = _bcm_td2_l3_ip4_options_profile_action_set(unit, ip4_options_profile_id,
                         ip4_option, action);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      bcm_esw_l3_ip4_options_action_get
 * Purpose:
 *      Retrive action for given IP action and Profile ID 
 * Parameters:
 *      unit                   - (IN)  bcm device.
 *      ip4_options_profile_id - (IN)  IP options profile ID
 *      ip4_option             - (OUT) Action for IP Options
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_l3_ip4_options_action_get(int unit,
                         int ip4_options_profile_id, 
                         int ip4_option, 
                         bcm_l3_ip4_options_action_t *action)
{
    int rv = BCM_E_UNAVAIL;
    if (action == NULL) {
        return BCM_E_PARAM;
    }
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
        rv = _bcm_td2_l3_ip4_options_profile_action_get(unit, ip4_options_profile_id, 
                         ip4_option, action);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      _bcm_esw_l3_egress_reference_get
 * Purpose:
 *      Get the reference count of the following object
 *      
 *      Egress object       ECMP = 0
 *      Multipath object    ECMP = 1
 *
 * Parameters:
 *      unit       - (IN) bcm device.
 *      mpintf     - (IN) L3 interface id pointing to Egress nh/multipath object
 *      ecmp       - (IN) Flag to indicate the object is multicast or not
 *      ref_count  - (OUT) The number of references to the L3 Table
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_esw_l3_egress_reference_get(int unit, bcm_if_t mpintf, int ecmp,
                                 uint32 * ref_count)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    int offset = 0;
    uint32 object_id;

    if (SOC_IS_XGS3_SWITCH(unit) && soc_feature(unit, soc_feature_l3)) {
        L3_LOCK(unit);

        if (ecmp != 0) {
            /* Multipath object */
            offset = BCM_XGS3_MPATH_EGRESS_IDX_MIN;
        } else {
            /* None-Multipaht object */
            if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, mpintf)) {
                /* Egress object */
                offset = BCM_XGS3_EGRESS_IDX_MIN;
            } else if ( BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, mpintf)) {
                /* DVP Egress object */
                offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
            }
        }

        object_id = mpintf - offset;

        _bcm_xgs3_nh_ref_cnt_get(unit, object_id, ecmp, ref_count);

        L3_UNLOCK(unit);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return (BCM_E_NONE);
}

#endif  /* INCLUDE_L3 */

