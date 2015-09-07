/*
 * $Id: multicast.c,v 1.139 Broadcom SDK $
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
 * File:    multicast.c
 * Purpose: Manages multicast functions
 */

#include <sal/core/libc.h>
#include <sal/core/time.h>
#include <shared/bsl.h>
#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>
#if defined(BCM_BRADLEY_SUPPORT)
#include <soc/bradley.h>
#endif /* BCM_BRADLEY_SUPPORT */

#include <bcm/l2.h>
#include <bcm/port.h>
#include <bcm/error.h>
#include <bcm/multicast.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/ipmc.h>
#if defined(BCM_TRX_SUPPORT)
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/trx.h>
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
#include <bcm_int/esw/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
#include <bcm_int/esw/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#include <bcm_int/esw/subport.h>
#endif /* BCM_KATANA2_SUPPORT */

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/esw/triumph3.h>

#define _BCM_MULTICAST_MTU_TABLE_OFFSET(unit) \
        ((SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit)) ? 512 : 8192)

#ifdef BCM_WARM_BOOT_SUPPORT
static int multicast_mode_set[BCM_MAX_NUM_UNITS];
#endif /* BCM_WARM_BOOT_SUPPORT */

#define _BCM_MULTICAST_L2_OR_FABRIC(_unit_, _group_) \
        (_BCM_MULTICAST_IS_L2(_group_) || SOC_IS_XGS3_FABRIC(_unit_))

#ifdef	BCM_XGS3_FABRIC_SUPPORT
typedef struct _bcm_fabric_multicast_info_s {
    int         mcast_size;
    int         ipmc_size;
    uint32      *mcast_used;
} _bcm_fabric_multicast_info_t;

static _bcm_fabric_multicast_info_t _bcm_fabric_mc_info[BCM_MAX_NUM_UNITS];

#define BCM_FABRIC_MCAST_BASE(unit) (0)
#define BCM_FABRIC_MCAST_MAX(unit) (_bcm_fabric_mc_info[unit].mcast_size - 1)
#define BCM_FABRIC_IPMC_BASE(unit) (_bcm_fabric_mc_info[unit].mcast_size)
#define BCM_FABRIC_IPMC_MAX(unit) \
        (_bcm_fabric_mc_info[unit].mcast_size + \
         _bcm_fabric_mc_info[unit].ipmc_size - 1)
#define BCM_FABRIC_MC_USED_BITMAP(unit) \
        (_bcm_fabric_mc_info[unit].mcast_used)
#endif	/* BCM_XGS3_FABRIC_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
/* Cache of MTU profile size regs */
STATIC soc_profile_reg_t *_bcm_mtu_profile[BCM_MAX_NUM_UNITS];
#endif

STATIC int multicast_initialized[BCM_MAX_NUM_UNITS];
#define MULTICAST_INIT_CHECK(unit) \
    if (!multicast_initialized[unit]) { \
        return BCM_E_INIT; \
    }

int
_bcm_esw_multicast_flags_to_group_type(uint32 flags)
{
    int group_type = 0;

    if (flags & BCM_MULTICAST_TYPE_L2) {
        group_type = _BCM_MULTICAST_TYPE_L2;
    } else if (flags & BCM_MULTICAST_TYPE_L3){
        group_type = _BCM_MULTICAST_TYPE_L3;
    } else if (flags & BCM_MULTICAST_TYPE_VPLS){
        group_type = _BCM_MULTICAST_TYPE_VPLS;
    } else if (flags & BCM_MULTICAST_TYPE_SUBPORT){
        group_type = _BCM_MULTICAST_TYPE_SUBPORT;
    } else if (flags & BCM_MULTICAST_TYPE_MIM){
        group_type = _BCM_MULTICAST_TYPE_MIM;
    } else if (flags & BCM_MULTICAST_TYPE_WLAN){
        group_type = _BCM_MULTICAST_TYPE_WLAN;
    } else if (flags & BCM_MULTICAST_TYPE_VLAN) { 
        group_type = _BCM_MULTICAST_TYPE_VLAN;
    } else if (flags & BCM_MULTICAST_TYPE_TRILL) {
        group_type = _BCM_MULTICAST_TYPE_TRILL;
    } else if (flags & BCM_MULTICAST_TYPE_NIV) {
        group_type = _BCM_MULTICAST_TYPE_NIV;
    } else if (flags & BCM_MULTICAST_TYPE_EGRESS_OBJECT) {
        group_type = _BCM_MULTICAST_TYPE_EGRESS_OBJECT;
    } else if (flags & BCM_MULTICAST_TYPE_L2GRE) {
        group_type = _BCM_MULTICAST_TYPE_L2GRE;
    } else if (flags & BCM_MULTICAST_TYPE_VXLAN) {
        group_type = _BCM_MULTICAST_TYPE_VXLAN;
    } else if (flags & BCM_MULTICAST_TYPE_EXTENDER) {
        group_type = _BCM_MULTICAST_TYPE_EXTENDER;
    }/* Else default to 0 above */

    return group_type;
}

uint32
_bcm_esw_multicast_group_type_to_flags(int group_type)
{
    uint32 flags = 0;

    switch (group_type) {
    case _BCM_MULTICAST_TYPE_L2:
        flags = BCM_MULTICAST_TYPE_L2;
        break;
    case _BCM_MULTICAST_TYPE_L3:
        flags = BCM_MULTICAST_TYPE_L3;
        break;
    case _BCM_MULTICAST_TYPE_VPLS:
        flags = BCM_MULTICAST_TYPE_VPLS;
        break;
    case _BCM_MULTICAST_TYPE_SUBPORT:
        flags = BCM_MULTICAST_TYPE_SUBPORT;
        break;
    case _BCM_MULTICAST_TYPE_MIM:
        flags = BCM_MULTICAST_TYPE_MIM;
        break;
    case _BCM_MULTICAST_TYPE_WLAN:
        flags = BCM_MULTICAST_TYPE_WLAN;
        break;
    case _BCM_MULTICAST_TYPE_VLAN:
        flags = BCM_MULTICAST_TYPE_VLAN;
        break;
    case _BCM_MULTICAST_TYPE_TRILL:
        flags = BCM_MULTICAST_TYPE_TRILL;
        break;
    case _BCM_MULTICAST_TYPE_NIV:
        flags = BCM_MULTICAST_TYPE_NIV;
        break;
    case _BCM_MULTICAST_TYPE_EGRESS_OBJECT:
        flags = BCM_MULTICAST_TYPE_EGRESS_OBJECT;
        break;
    case _BCM_MULTICAST_TYPE_L2GRE:
        flags = BCM_MULTICAST_TYPE_L2GRE;
        break;
    case _BCM_MULTICAST_TYPE_VXLAN:
        flags = BCM_MULTICAST_TYPE_VXLAN;
        break;
    case _BCM_MULTICAST_TYPE_EXTENDER:
        flags = BCM_MULTICAST_TYPE_EXTENDER;
        break;

    default:
        /* flags defaults to 0 above */
        break;
    }

    return flags;
}

#ifdef BCM_XGS3_FABRIC_SUPPORT
/*
 * Function:
 *      _bcm_esw_fabric_multicast_param_check
 * Purpose:
 *      Validate fabric multicast parameters and adjust HW index.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      group      - (IN) Multicast group ID.
 *      mc_id      - (IN/OUT)   multicast index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_fabric_multicast_param_check(int unit, bcm_multicast_t group,
                                      int *mc_index)
{
    if (_BCM_MULTICAST_IS_L2(group)) {
        /* L2 multicast groups start at index 1 */
        if ((*mc_index < BCM_FABRIC_MCAST_BASE(unit)) ||
            (*mc_index > BCM_FABRIC_MCAST_MAX(unit))) {
            return BCM_E_PARAM;
        }
    } else if (_BCM_MULTICAST_IS_SET(group)) {
        /* Non-L2 groups are offset */
        *mc_index += BCM_FABRIC_IPMC_BASE(unit);
        if ((*mc_index < BCM_FABRIC_IPMC_BASE(unit)) ||
            (*mc_index > BCM_FABRIC_IPMC_MAX(unit))) {
            return BCM_E_PARAM;
        }
    } else {
        return BCM_E_PARAM;
    }
    if (!SHR_BITGET(BCM_FABRIC_MC_USED_BITMAP(unit), *mc_index)) {
        return BCM_E_NOT_FOUND;
    }
    return BCM_E_NONE;
}
#endif	/* BCM_XGS3_FABRIC_SUPPORT */

/*
 * Function:
 *      _bcm_esw_multicast_ipmc_read
 * Purpose:
 *      Read L3 multicast distribuition ports.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   IPMC index.
 *      l2_pbmp    - (IN)   L2 distribution ports.
 *      l3_pbmp    - (IN)   L3 distribuition port. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_esw_multicast_ipmc_read(int unit, int ipmc_id, bcm_pbmp_t *l2_pbmp,
                             bcm_pbmp_t *l3_pbmp)
{
    uint32 entry[SOC_MAX_MEM_FIELD_WORDS];  /* hw entry bufffer. */

    /* Input parameters check. */
    if ((NULL == l2_pbmp) || (NULL == l3_pbmp)) {
        return (BCM_E_PARAM);
    }

    /* Table index sanity check. */
    if ((ipmc_id < soc_mem_index_min(unit, L3_IPMCm)) ||
        (ipmc_id > soc_mem_index_max(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    /* Read L3 ipmc table. */
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY,
                                     ipmc_id, entry));

    /* If entry is invalid - clear L2 & L3 bitmaps. */
    if (soc_mem_field32_get(unit, L3_IPMCm, entry, VALIDf) == 0) {
        BCM_PBMP_CLEAR(*l2_pbmp);
        BCM_PBMP_CLEAR(*l3_pbmp);
        return (BCM_E_NONE);
    }
    /* Extract L2 & L3 bitmaps. */
    soc_mem_pbmp_field_get(unit, L3_IPMCm, entry, L2_BITMAPf, l2_pbmp);
    soc_mem_pbmp_field_get(unit, L3_IPMCm, entry, L3_BITMAPf, l3_pbmp);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_esw_multicast_ipmc_write
 * Purpose:
 *      Write L3 multicast distribuition ports.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   IPMC index.
 *      l2_pbmp    - (IN)   L2 distribution ports.
 *      l3_pbmp    - (IN)   L3 distribuition port. 
 *      valid      - (IN)   Distribuition is valid bit.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_esw_multicast_ipmc_write(int unit, int ipmc_id, bcm_pbmp_t l2_pbmp,
                              bcm_pbmp_t l3_pbmp, int valid)
{
    uint32 entry[SOC_MAX_MEM_FIELD_WORDS];  /* hw entry bufffer. */
    int rv;
#if defined(BCM_KATANA_SUPPORT)
    int remap_id = 0;
#endif

    sal_memset(&entry, 0, sizeof(ipmc_entry_t));

    /* Currently there is no need to invalidate distribution. */
    if (0 == valid) {
        BCM_PBMP_CLEAR(l2_pbmp);
        BCM_PBMP_CLEAR(l3_pbmp);
    }

    /* Table index sanity check. */
    if ((ipmc_id < soc_mem_index_min(unit, L3_IPMCm)) ||
        (ipmc_id > soc_mem_index_max(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    /* Read / Modify / Write section. */
    soc_mem_lock(unit, L3_IPMCm);
    rv = soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, ipmc_id, entry);
    if (BCM_SUCCESS(rv)) {
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit)) {
            remap_id = soc_mem_field32_get(unit, L3_IPMCm, entry,
                                    MMU_MC_REDIRECTION_PTRf);
            if (soc_mem_field32_get(unit, L3_IPMCm, entry, VALIDf) == 0 || !valid) {
                /* Invalid entry, flush */
                sal_memset(&entry, 0, sizeof(ipmc_entry_t)); 
                if (remap_id) {
                     soc_mem_field32_set(unit, L3_IPMCm, entry,
                                    MMU_MC_REDIRECTION_PTRf, remap_id);
                }
            } 
            soc_mem_field32_set(unit, L3_IPMCm, entry, VALIDf, valid);
            if ((0 == soc_mem_field32_get(unit, L3_IPMCm, entry,
                                          MMU_MC_REDIRECTION_PTRf))) {
                soc_mem_field32_set(unit, L3_IPMCm, entry,
                                    MMU_MC_REDIRECTION_PTRf, ipmc_id);
            }
        } else
#endif
        {
            if (soc_mem_field32_get(unit, L3_IPMCm, entry, VALIDf) == 0 || !valid) {
                /* Invalid entry, flush */
                sal_memset(&entry, 0, sizeof(ipmc_entry_t));
            }
            soc_mem_field32_set(unit, L3_IPMCm, entry, VALIDf, valid); 
        }
    }

    soc_mem_pbmp_field_set(unit, L3_IPMCm, entry, L2_BITMAPf, &l2_pbmp);
    soc_mem_pbmp_field_set(unit, L3_IPMCm, entry, L3_BITMAPf, &l3_pbmp);

    rv = soc_mem_write(unit, L3_IPMCm, MEM_BLOCK_ALL, ipmc_id, &entry);

    soc_mem_unlock(unit, L3_IPMCm);

#if defined(BCM_BRADLEY_SUPPORT)
    if (BCM_SUCCESS(rv) && (SOC_IS_HBX(unit) || SOC_IS_TD_TT(unit))) {
        /* On these devices, the Higig packets use the L2MC table with
         * IPMC offset to get the L2 bitmap. */
        int		mc_base, mc_size, mc_index;
        l2mc_entry_t	l2mc;

        SOC_IF_ERROR_RETURN
            (soc_hbx_ipmc_size_get(unit, &mc_base, &mc_size));
        if (ipmc_id < 0 || ipmc_id > mc_size) {
            return BCM_E_PARAM;
        }
        mc_index = ipmc_id + mc_base;

        soc_mem_lock(unit, L2MCm);
        if (!valid) {
            rv = WRITE_L2MCm(unit, MEM_BLOCK_ALL, mc_index,
                    soc_mem_entry_null(unit, L2MCm));
        } else {
            rv = READ_L2MCm(unit, MEM_BLOCK_ANY, mc_index, &l2mc);
            if (rv >= 0) {
                soc_mem_pbmp_field_set(unit, L2MCm, &l2mc,
                        PORT_BITMAPf, &l2_pbmp);
                soc_mem_field32_set(unit, L2MCm, &l2mc, VALIDf, 1);
                rv = WRITE_L2MCm(unit, MEM_BLOCK_ALL, mc_index, &l2mc);
            }
        }
        soc_mem_unlock(unit, L2MCm);
    }
#endif /* BCM_BRADLEY_SUPPORT */

    return rv;
}

/*
 * Function:
 *      _bcm_esw_ipmc_egress_intf_add
 * Purpose:
 *      Helper function to add ipmc egress interface on different devices.
 * Parameters:
 *      unit       - (IN)  Device Number
 *      ipmc_id    - (IN)  ipmc id
 *      port       - (IN)  Port number
 *      nh_index   - (IN)  Next Hop index
 *      is_l3      - (IN)  Indicates if multicast group type is IPMC.
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_esw_ipmc_egress_intf_add(int unit, int ipmc_id, bcm_port_t port,
                          int nh_index, int is_l3)
{
#if defined (INCLUDE_L3)
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit) || SOC_IS_KATANA2(unit)) {
        return _bcm_tr3_ipmc_egress_intf_add(unit, ipmc_id, port, nh_index,
                                             is_l3);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) 
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_VALKYRIE2(unit) ||
        SOC_IS_APOLLO(unit) || SOC_IS_TD_TT(unit) ||
        SOC_IS_KATANA(unit)) {
        return _bcm_tr2_ipmc_egress_intf_add(unit, ipmc_id, port, nh_index);
    }
#endif
#if defined(BCM_FIREBOLT_SUPPORT)
    return _bcm_fb_ipmc_egress_intf_add(unit, ipmc_id, port, nh_index, is_l3);
#endif /* BCM_FIREBOLT_SUPPORT */
#endif /* INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      _bcm_esw_ipmc_egress_intf_delete
 * Purpose:
 *      Helper function to delete ipmc egress interface on different devices.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   ipmc id
 *      port       - (IN)  Port number
 *      if_max     - (IN)  Maximal interface
 *      nh_index   - (IN)  Next Hop index
 *      is_l3      - (IN)  Indicates if multicast group type is IPMC.
 * Returns:
 *      BCM_E_XXX
 */
int _bcm_esw_ipmc_egress_intf_delete(int unit, int ipmc_id,
                                 bcm_port_t port, int if_max,
                                 int nh_index, int is_l3)
{
#if defined (INCLUDE_L3)
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit) || SOC_IS_KATANA2(unit))     {
        return _bcm_tr3_ipmc_egress_intf_delete(unit, ipmc_id, port,
                                                if_max, nh_index, is_l3);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) 
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_VALKYRIE2(unit) ||
        SOC_IS_APOLLO(unit) || SOC_IS_TD_TT(unit) || SOC_IS_KATANA(unit)) {
        return _bcm_tr2_ipmc_egress_intf_delete(unit, ipmc_id, port, 
                                                if_max, nh_index);
    }
#endif
#if defined(BCM_FIREBOLT_SUPPORT)
    return _bcm_fb_ipmc_egress_intf_delete(unit, ipmc_id, port,
                                           if_max, nh_index, is_l3);
#endif /* BCM_FIREBOLT_SUPPORT */
 #endif /* INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_esw_multicast_sync
 * Purpose:
 *      Record Multicast module persisitent info for Level 2 Warm Boot
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_esw_multicast_sync(int unit)
{
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TRX(unit) && !SOC_IS_XGS3_FABRIC(unit)) {
        return _bcm_trx_multicast_sync(unit);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */


#if defined(BCM_XGS3_SWITCH_SUPPORT) 

/*
 * Function:
 *      _bcm_esw_multicast_l2_add
 * Purpose:
 *      Helper function to add a port to L2 multicast group
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_multicast_l2_add(int unit, bcm_multicast_t group, 
                          bcm_port_t port, bcm_if_t encap_id)
{
    l2mc_entry_t    l2mc_entry;
    bcm_pbmp_t      l2_pbmp;
    int             mc_index, gport_id, modid_local, idx, trunk_local_ports = 0;
    bcm_module_t    mod_out;
    bcm_port_t      port_out;
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    bcm_port_t      local_trunk_member_ports[SOC_MAX_NUM_PP_PORTS] = {0};
    int             max_num_ports = SOC_MAX_NUM_PP_PORTS;
#else
    bcm_port_t      local_trunk_member_ports[SOC_MAX_NUM_PORTS] = {0};
    int             max_num_ports = SOC_MAX_NUM_PORTS;
#endif
    bcm_trunk_t     trunk_id;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif

    modid_local = 0;

    mc_index = _BCM_MULTICAST_ID_GET(group);

    /* Table index sanity check. */
#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_fabric_multicast_param_check(unit, group, &mc_index));
    } else
#endif /* BCM_XGS3_FABRIC_SUPPORT */
    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L2MCm))) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out,
                                &trunk_id, &gport_id)); 

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        if (BCM_GPORT_IS_SET(port) &&
            _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit, port)) {
            if (mod_out == (my_modid + 1)) {
                pp_port = BCM_GPORT_SUBPORT_PORT_GET(port);
                if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                    (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport == FALSE) {
                return BCM_E_PORT;
            }
        }
    }
    if (is_local_subport) {
        port_out = pp_port;
    } else
#endif

    if (BCM_TRUNK_INVALID != trunk_id) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_trunk_local_members_get(unit, trunk_id, 
                                             max_num_ports,
                                             local_trunk_member_ports, 
                                             &trunk_local_ports));
        /* Convertion of system ports to physical ports done in */
        /* _bcm_esw_multicast_trunk_members_setup */
    } else {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_modid_is_local(unit, mod_out, &modid_local));
        if (TRUE != modid_local) {
            /* Only add this to replication set if destination is local */
            return BCM_E_PORT;
        }
        /* Convert system port to physical port */
        if (soc_feature(unit, soc_feature_sysport_remap)) {
            BCM_XLATE_SYSPORT_S2P(unit, &port_out);
        }
    }

    /* Add the port to the L2MC port bitmap */
    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, L2MCm, MEM_BLOCK_ANY, mc_index, &l2mc_entry));

    soc_mem_pbmp_field_get(unit, L2MCm, &l2mc_entry, PORT_BITMAPf,
                           &l2_pbmp);

    if (BCM_TRUNK_INVALID != trunk_id) {
        for (idx = 0; idx < trunk_local_ports; idx++) {
            BCM_PBMP_PORT_ADD(l2_pbmp, local_trunk_member_ports[idx]);
        }
    } else {
        BCM_PBMP_PORT_ADD(l2_pbmp, port_out);
    }
    soc_mem_pbmp_field_set(unit, L2MCm, &l2mc_entry, PORT_BITMAPf, &l2_pbmp);

    return soc_mem_write(unit, L2MCm, MEM_BLOCK_ALL, mc_index, &l2mc_entry);
}

/*
 * Function:
 *      _bcm_esw_multicast_l2_delete
 * Purpose:
 *      Helper function to delete a port from L2 multicast group
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_multicast_l2_delete(int unit, bcm_multicast_t group, 
                             bcm_port_t port, bcm_if_t encap_id)
{
    l2mc_entry_t    l2mc_entry;
    bcm_pbmp_t      l2_pbmp;
    int             mc_index, gport_id, modid_local, idx, trunk_local_ports = 0;
    bcm_module_t    mod_out;
    bcm_port_t      port_out;
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    bcm_port_t      local_trunk_member_ports[SOC_MAX_NUM_PP_PORTS] = {0};
    int             max_num_ports = SOC_MAX_NUM_PP_PORTS;
#else
    bcm_port_t      local_trunk_member_ports[SOC_MAX_NUM_PORTS] = {0};
    int             max_num_ports = SOC_MAX_NUM_PORTS;
#endif
    bcm_trunk_t     trunk_id;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif

    modid_local = 0;
    mc_index = _BCM_MULTICAST_ID_GET(group);

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_fabric_multicast_param_check(unit, group, &mc_index));
    } else
#endif /* BCM_XGS3_FABRIC_SUPPORT */
    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L2MCm))) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, 
                                &trunk_id, &gport_id)); 

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        if (BCM_GPORT_IS_SET(port) &&
            _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit, port)) {
            if (mod_out == (my_modid + 1)) {
                pp_port = BCM_GPORT_SUBPORT_PORT_GET(port);
                if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                    (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport == FALSE) {
                return BCM_E_PORT;
            }
        }
    }
    if (is_local_subport) {
        port_out = pp_port;
    } else
#endif

    if (BCM_TRUNK_INVALID != trunk_id) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_trunk_local_members_get(unit, trunk_id, 
                                             max_num_ports,
                                             local_trunk_member_ports, 
                                             &trunk_local_ports));
        /* Convertion of system ports to physical ports done in */
        /* _bcm_esw_trunk_local_members_get */
    } else {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_modid_is_local(unit, mod_out, &modid_local));
        if (TRUE != modid_local) {
            /* Only add this to replication set if destination is local */
            return BCM_E_PORT;
        }
        /* Convert system port to physical port */
        if (soc_feature(unit, soc_feature_sysport_remap)) {
            BCM_XLATE_SYSPORT_S2P(unit, &port_out);
        }
    }

    /* Delete the port from the L2MC port bitmap */
    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, L2MCm, MEM_BLOCK_ANY, mc_index, &l2mc_entry));

    soc_mem_pbmp_field_get(unit, L2MCm, &l2mc_entry, PORT_BITMAPf,
                           &l2_pbmp);


    if (BCM_TRUNK_INVALID != trunk_id) {
        for (idx = 0; idx < trunk_local_ports; idx++) {
            BCM_PBMP_PORT_REMOVE(l2_pbmp, local_trunk_member_ports[idx]);
        }
    } else {
        BCM_PBMP_PORT_REMOVE(l2_pbmp, port_out);
    }
    soc_mem_pbmp_field_set(unit, L2MCm, &l2mc_entry, PORT_BITMAPf, &l2_pbmp);

    return soc_mem_write(unit, L2MCm, MEM_BLOCK_ALL, mc_index, &l2mc_entry);
}

#if (defined(BCM_TRX_SUPPORT) || defined(BCM_XGS3_SWITCH_SUPPORT)) && defined(INCLUDE_L3) 
/*
 * Function:
 *      _bcm_esw_multicast_l2_get
 * Purpose:
 *      Helper function to get ports that are part of L2 multicast 
 * Parameters: 
 *      unit           - (IN) Device Number
 *      mc_index       - (IN) Multicast index
 *      port_max       - (IN) Number of entries in "port_array"
 *      port_array     - (OUT) List of ports
 *      encap_id_array - (OUT) List of encap identifiers
 *      port_count     - (OUT) Actual number of ports returned
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the input parameter port_max = 0, return in the output parameter
 *      port_count the total number of ports/encapsulation IDs in the 
 *      specified multicast group's replication list.
 */

STATIC int     
_bcm_esw_multicast_l2_get(int unit, bcm_multicast_t group, int port_max,
                             bcm_gport_t *port_array, bcm_if_t *encap_id_array, 
                             int *port_count)
{
    int             mc_index, i, rv;
    l2mc_entry_t    l2mc_entry;
    bcm_pbmp_t      l2_pbmp;
    bcm_port_t      port_iter;

    i = 0;
    rv = BCM_E_NONE;

    mc_index = _BCM_MULTICAST_ID_GET(group);

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_fabric_multicast_param_check(unit, group, &mc_index));
    } else
#endif /* BCM_XGS3_FABRIC_SUPPORT */
    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L2MCm))) {
        return BCM_E_PARAM;
    }

    soc_mem_lock(unit, L2MCm);
    rv = soc_mem_read(unit, L2MCm, MEM_BLOCK_ANY, mc_index, &l2mc_entry);
    soc_mem_unlock(unit, L2MCm);
    BCM_IF_ERROR_RETURN(rv);

    soc_mem_pbmp_field_get(unit, L2MCm, &l2mc_entry, PORT_BITMAPf, &l2_pbmp);
    PBMP_ITER(l2_pbmp, port_iter) {
        if ((port_max > 0) && (i >= port_max)) {
            break;
        }
        if (NULL != port_array) {
#if defined(BCM_KATANA2_SUPPORT)
            /* For KT2, pp_port >=42 are used for LinkPHY/SubportPktTag subport.
             * Get the subport info associated with the pp_port and form the subport_gport 
             * subport gport should be returned in the port_array for pp_port >=42*/

            if ((soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) &&
                (port_iter >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                (port_iter <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                _BCM_KT2_SUBPORT_PORT_ID_SET(port_array[i], port_iter);
                if (BCM_PBMP_MEMBER(
                    SOC_INFO(unit).linkphy_pp_port_pbm, port_iter)) {
                    _BCM_KT2_SUBPORT_PORT_TYPE_SET(port_array[i],
                        _BCM_KT2_SUBPORT_TYPE_LINKPHY);
                } else if (BCM_PBMP_MEMBER(
                    SOC_INFO(unit).subtag_pp_port_pbm, port_iter)) {
                    _BCM_KT2_SUBPORT_PORT_TYPE_SET(port_array[i],
                        _BCM_KT2_SUBPORT_TYPE_SUBTAG);
                } else {
                    return BCM_E_PORT;
                }
            } else
#endif /* BCM_KATANA2_SUPPORT */
            {
                BCM_IF_ERROR_RETURN
                (bcm_esw_port_gport_get(unit, port_iter, (port_array + i)));
            }
        }

        if (NULL != encap_id_array) {
            encap_id_array[i] = BCM_IF_INVALID;
        }
        i++;
    }
    *port_count = i;
    return BCM_E_NONE;
}
#endif /* #if (defined(BCM_TRX_SUPPORT) || defined(BCM_XGS3_SWITCH_SUPPORT)) && defined(INCLUDE_L3) */

/*
 * Function:
 *      _bcm_esw_multicast_l2_create
 * Purpose:
 *      Helper function to allocate a multicast group index for l2
 * Parameters:
 *      unit       - (IN)   Device Number
 *      flags      - (IN)   BCM_MULTICAST_*
 *      group      - (OUT)  Group ID
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_multicast_l2_create(int unit, uint32 flags, bcm_multicast_t *group)
{
    int             mc_index;
    int             is_set = FALSE;
    l2mc_entry_t    l2mc_entry;

    if (flags & BCM_MULTICAST_WITH_ID) {
        if (_BCM_MULTICAST_IS_SET(*group) && !_BCM_MULTICAST_IS_L2(*group)) {
            /* Group ID doesn't match creation flag type */
            return BCM_E_PARAM;
        }
        mc_index = _BCM_MULTICAST_ID_GET(*group);
        if ((mc_index < 0) ||
            (mc_index >= soc_mem_index_count(unit, L2MCm))) {
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN(_bcm_xgs3_l2mc_index_is_set(unit, mc_index, &is_set));
        if (is_set) {
            return BCM_E_EXISTS;
        }
        
        BCM_IF_ERROR_RETURN(_bcm_xgs3_l2mc_id_alloc(unit, mc_index));
    } else {
        BCM_IF_ERROR_RETURN(_bcm_xgs3_l2mc_free_index(unit, &mc_index));
    }
    sal_memset(&l2mc_entry, 0, sizeof(l2mc_entry));
    soc_mem_field32_set(unit, L2MCm, &l2mc_entry, VALIDf, 1);
    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, L2MCm, MEM_BLOCK_ALL, mc_index, &l2mc_entry));
    _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_L2, mc_index);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_multicast_l2_destroy
 * Purpose:
 *      Helper function to destroy L2 multicast 
 * Parameters: 
 *      unit           - (IN) Device Number
 *      group          - (IN) Multicast group to destroy
 * Returns:
 *      BCM_E_XXX
 */

STATIC int     
_bcm_esw_multicast_l2_destroy(int unit, bcm_multicast_t group)
{
    int         mc_index;

    mc_index = _BCM_MULTICAST_ID_GET(group);

    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L2MCm))) {
        return BCM_E_PARAM;
    }

    /* Clear the HW entry */
    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, L2MCm, MEM_BLOCK_ALL, 
                      mc_index, soc_mem_entry_null(unit, L2MCm)));
    
    /* Free the L2MC index */
    return _bcm_xgs3_l2mc_id_free(unit, mc_index);
}

#if defined (INCLUDE_L3)

/*
 * Function:
 *      _bcm_esw_multicast_egress_mapped_trunk_member_find
 * Purpose:
 *      Find the trunk member the encap id resides on.
 * Parameters:
 *      unit  - (IN) Device Number
 *      group - (IN) Multicast group ID
 *      port  - (IN) GPORT ID
 *      encap_id - (IN) Encap identifiers
 *      trunk_member - (OUT) Trunk member the encap id resides on
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_multicast_egress_mapped_trunk_member_find(int unit,
        bcm_multicast_t group, bcm_gport_t port, bcm_if_t encap_id,
        bcm_gport_t *trunk_member)
{
    int rv = BCM_E_NONE;
    bcm_trunk_t trunk_id;
    bcm_trunk_info_t tinfo;
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    bcm_port_t      local_trunk_port_array[SOC_MAX_NUM_PP_PORTS];
    int             max_num_ports = SOC_MAX_NUM_PP_PORTS;
#else
    bcm_port_t      local_trunk_port_array[SOC_MAX_NUM_PORTS];
    int             max_num_ports = SOC_MAX_NUM_PORTS;
#endif
    int num_local_trunk_ports;
    int port_count;
    bcm_gport_t *port_array;
    bcm_if_t *encap_id_array;
    int match;
    int i, j;
    bcm_port_t local_port;
    int mc_index;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE;
#endif
    if (NULL == trunk_member) {
        return BCM_E_PARAM;
    }

    *trunk_member = port;

    if (0 == _BCM_MULTICAST_IS_L3(group)) {
        /* Software trunk resolution is available only for IPMC groups */
        return BCM_E_NONE;
    }
    mc_index = _BCM_MULTICAST_ID_GET(group);
    if ((mc_index < 0) || (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    if (!BCM_GPORT_IS_TRUNK(port)) {
        return BCM_E_NONE;
    }

    if (BCM_IF_INVALID == encap_id) {
        return BCM_E_NONE;
    }

    /* Get trunk property and local member ports */
    trunk_id = BCM_GPORT_TRUNK_GET(port);
    BCM_IF_ERROR_RETURN
        (bcm_esw_trunk_get(unit, trunk_id, &tinfo, 0, NULL, NULL));
    if (!(tinfo.flags & BCM_TRUNK_FLAG_IPMC_CLEAVE)) {
        return BCM_E_NONE;
    }
    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, trunk_id,
                max_num_ports, local_trunk_port_array,
                &num_local_trunk_ports));

    /* Get all ports and encap_ids of the multicast group */
    BCM_IF_ERROR_RETURN(bcm_esw_multicast_egress_get(unit, group, 0,
                NULL, NULL, &port_count));
    if (0 == port_count) {
        return BCM_E_NOT_FOUND;
    }
    port_array = sal_alloc(sizeof(bcm_gport_t) * port_count, "port_array");
    if (NULL == port_array) {
        return BCM_E_MEMORY;
    }
    encap_id_array = sal_alloc(sizeof(bcm_if_t) * port_count, "encap_id_array");
    if (NULL == encap_id_array) {
        sal_free(port_array);
        return BCM_E_MEMORY;
    }
    rv = bcm_esw_multicast_egress_get(unit, group, port_count,
            port_array, encap_id_array, &port_count);
    if (BCM_FAILURE(rv)) {
        sal_free(port_array);
        sal_free(encap_id_array);
        return rv;
    }

    /* Search for the trunk member the encap id resides on */
    match = FALSE;
    for (i = 0; i < port_count; i++) {
        if (encap_id == encap_id_array[i]) {
#if defined(BCM_KATANA2_SUPPORT)
            is_local_subport = FALSE;
            if (soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) {
                if (BCM_GPORT_IS_SET(port_array[i]) &&
                    _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                    port_array[i])) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport) {
                local_port = _BCM_KT2_SUBPORT_PORT_ID_GET(port_array[i]);
            } else
#endif
            {
                rv = bcm_esw_port_local_get(unit, port_array[i], &local_port);
                if (BCM_FAILURE(rv)) {
                    sal_free(port_array);
                    sal_free(encap_id_array);
                    return rv;
                }
            }
            for (j = 0; j < num_local_trunk_ports; j++) {
                if (local_trunk_port_array[j] == local_port) {
                    match = TRUE;
                    break;
                } 
            }
            if (match) {
                break;
            }
        }
    }

    if (match) {
        *trunk_member = port_array[i];
    } else {
        rv = BCM_E_NOT_FOUND;
    }

    sal_free(port_array);
    sal_free(encap_id_array);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_multicast_egress_encap_id_to_trunk_member_map
 * Purpose:
 *      Maps encap id to a trunk group member. This is performed
 *      if (1) the given group is an IPMC group, (2) the port in port_array is 
 *      a trunk gport, and (3) IPMC trunk resolution is disabled for that trunk
 *      group in hardware by setting the BCM_TRUNK_FLAG_IPMC_CLEAVE flag in
 *      bcm_trunk_info_t. The hash fields used for trunk group member
 *      selection are specified by ipmc_psc field in bcm_trunk_info_t.
 *      The valid values for ipmc_psc are BCM_TRUNK_PSC_IPSA, IPDA, EGRESS_VID,
 *      and RANDOM.
 * Parameters:
 *      unit       - (IN) Device Number
 *      group      - (IN) Multicast group ID
 *      port_count     - (IN) Number of ports in replication list
 *      port_array     - (IN) List of GPORT Identifiers
 *      encap_id_array - (IN) List of encap identifiers
 *      mapped_port_array - (OUT) List of mapped GPORT identifiers
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_multicast_egress_encap_id_to_trunk_member_map(int unit,
        bcm_multicast_t group, int port_count,
        bcm_gport_t *port_array, bcm_if_t *encap_id_array,
        bcm_gport_t *mapped_port_array)
{
    int rv = BCM_E_NONE;
    bcm_ipmc_addr_t ipmc_data;
    uint8 *hash_key = NULL;
    uint8 *hash_key_ptr = NULL;
    int hash_key_size, max_hash_key_size;
    int i, j;
    bcm_trunk_t trunk_id;
    bcm_trunk_info_t tinfo;
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
        bcm_port_t      local_trunk_port_array[SOC_MAX_NUM_PP_PORTS];
        int             max_num_ports = SOC_MAX_NUM_PP_PORTS;
#else
        bcm_port_t      local_trunk_port_array[SOC_MAX_NUM_PORTS];
        int             max_num_ports = SOC_MAX_NUM_PORTS;
#endif
    int num_local_trunk_ports;
    bcm_l3_intf_t l3_intf;
    uint32 hash;
    bcm_port_t mapped_port;
    int mc_index;
    int random_num;
    int optimize_for_space;
    bcm_if_t *temp_encap_array = NULL;
    int *sublist_id_array = NULL;
    int temp_encap_count;
    int sublist_max, sublist_count = 0;
    int first_free_sublist_id, next_free_sublist_id;
    int selected_sublist_id;
    int sublist0_trunk_member_index;
    int trunk_member_index;
    int temp_encap_index;

    if (port_count < 0) {
        return BCM_E_PARAM;
    }

    if (0 == port_count) {
        /* Nothing to do */
        return BCM_E_NONE;
    }

    if (NULL == port_array) {
        return BCM_E_PARAM;
    }

    /* Copy port_array to mapped_port_array */
    sal_memcpy(mapped_port_array, port_array,
            sizeof(bcm_gport_t) * port_count);

    if (0 == _BCM_MULTICAST_IS_L3(group)) {
        /* Software trunk resolution is available only for IPMC groups */
        return BCM_E_NONE;
    }
    mc_index = _BCM_MULTICAST_ID_GET(group);
    if ((mc_index < 0) || (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    if (NULL == encap_id_array) {
        /* For IPMC groups, encap_id_array should not be NULL */
        return BCM_E_PARAM;
    }

    /* Allocate hash key. The hash key contains at most two IPv6 address,
     * a VLAN ID, and a random integer.
     */
    max_hash_key_size = 2 * sizeof(bcm_ip6_t) + sizeof(bcm_vlan_t) +
                        sizeof(int);
    hash_key = sal_alloc(max_hash_key_size, "hash key");
    if (NULL == hash_key) {
        return BCM_E_MEMORY;
    }
    sal_memset(hash_key, 0, max_hash_key_size);

    /* Iterate through mapped_port_array */
    for (i = 0; i < port_count; i++) {
        if (!BCM_GPORT_IS_TRUNK(mapped_port_array[i])) {
            continue;
        }

        if (BCM_IF_INVALID == encap_id_array[i]) {
            continue;
        }

        trunk_id = BCM_GPORT_TRUNK_GET(mapped_port_array[i]);
        rv = bcm_esw_trunk_get(unit, trunk_id, &tinfo, 0, NULL, NULL);
        if (BCM_FAILURE(rv)) {
            goto done;
        }
        if (!(tinfo.flags & BCM_TRUNK_FLAG_IPMC_CLEAVE)) {
            /* IPMC trunk resolution is enabled in hardware,
             * do not perform software trunk resolution.
             */
            continue;
        }

        /* Get local members of the trunk group */
        rv = _bcm_esw_trunk_local_members_get(unit, trunk_id,
                max_num_ports, local_trunk_port_array,
                &num_local_trunk_ports);
        if (BCM_FAILURE(rv)) {
            goto done;
        }

        /* Compute hash */

        sal_memset(hash_key, 0, max_hash_key_size);
        hash_key_ptr = hash_key;

        if ((tinfo.ipmc_psc & BCM_TRUNK_PSC_IPSA) ||
                (tinfo.ipmc_psc & BCM_TRUNK_PSC_IPDA)) {
            ipmc_data.flags = 0;
            rv = bcm_esw_ipmc_get_by_index(unit, _BCM_MULTICAST_ID_GET(group),
                    &ipmc_data);
            if (BCM_FAILURE(rv)) {
                goto done;
            }
            if (ipmc_data.flags & BCM_IPMC_IP6) {
                if (tinfo.ipmc_psc & BCM_TRUNK_PSC_IPSA) {
                    sal_memcpy(hash_key_ptr, &ipmc_data.s_ip6_addr,
                            sizeof(ipmc_data.s_ip6_addr));
                    hash_key_ptr += sizeof(ipmc_data.s_ip6_addr);
                }
                if (tinfo.ipmc_psc & BCM_TRUNK_PSC_IPDA) {
                    sal_memcpy(hash_key_ptr, &ipmc_data.mc_ip6_addr,
                            sizeof(ipmc_data.mc_ip6_addr));
                    hash_key_ptr += sizeof(ipmc_data.mc_ip6_addr);
                }
            } else {
                if (tinfo.ipmc_psc & BCM_TRUNK_PSC_IPSA) {
                    sal_memcpy(hash_key_ptr, &ipmc_data.s_ip_addr,
                            sizeof(ipmc_data.s_ip_addr));
                    hash_key_ptr += sizeof(ipmc_data.s_ip_addr);
                }
                if (tinfo.ipmc_psc & BCM_TRUNK_PSC_IPDA) {
                    sal_memcpy(hash_key_ptr, &ipmc_data.mc_ip_addr,
                            sizeof(ipmc_data.mc_ip_addr));
                    hash_key_ptr += sizeof(ipmc_data.mc_ip_addr);
                }
            }
        }

        if (tinfo.ipmc_psc & BCM_TRUNK_PSC_EGRESS_VID) {
            if (!BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, encap_id_array[i])) {
                bcm_l3_intf_t_init(&l3_intf);
                l3_intf.l3a_intf_id = encap_id_array[i];
                rv = bcm_esw_l3_intf_get(unit, &l3_intf);
                if (BCM_FAILURE(rv)) {
                    goto done;
                }
                sal_memcpy(hash_key_ptr, &l3_intf.l3a_vid,
                        sizeof(bcm_vlan_t));
                hash_key_ptr += sizeof(bcm_vlan_t);
            }
        }

        if (tinfo.ipmc_psc & BCM_TRUNK_PSC_RANDOM) {
            random_num = sal_time_usecs();
            sal_memcpy(hash_key_ptr, &random_num, sizeof(int));
            hash_key_ptr += sizeof(int);
        }

        hash_key_size = hash_key_ptr - hash_key;
        if (hash_key_size > 0) {
            hash = _shr_crc32(0, hash_key, hash_key_size);
        } else {
            hash = 0;
        }

        optimize_for_space = FALSE;
#if defined(BCM_TRIUMPH2_SUPPORT) 
        if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit) || SOC_IS_TRIDENT(unit) ||
                SOC_IS_KATANA(unit)) {
            int available_percent, threshold;

            rv = bcm_esw_switch_control_get(unit,
                    bcmSwitchIpmcReplicationAvailabilityThreshold, &threshold);
            if (BCM_FAILURE(rv)) {
                goto done;
            }
            if (threshold > 0) {
                rv = bcm_tr2_ipmc_repl_availability_get(unit, &available_percent);
                if (BCM_FAILURE(rv)) {
                    goto done;
                }
                if (available_percent < threshold) {
                    /* IPMC replication table available space has fallen below the
                     * configured threshold. Conservation of IPMC replication table
                     * space now takes precedence over hashing when assigning encap 
                     * ids to trunk members.
                     */
                    optimize_for_space = TRUE;
                }
            }
        } 
#endif  /* BCM_TRIUMPH2_SUPPORT */

        if (!optimize_for_space) {
            mapped_port = local_trunk_port_array[hash % num_local_trunk_ports];
            rv = bcm_esw_port_gport_get(unit, mapped_port,
                    &mapped_port_array[i]);
            if (BCM_FAILURE(rv)) {
                goto done;
            }
            continue;
        } 

        /* Assign encap ids to trunk members with conservation of IPMC
         * replication table space as highest priority.
         * First, gather all encap ids with the same trunk ID as the
         * current trunk group.
         */

        if (NULL == temp_encap_array) {
            temp_encap_array = sal_alloc(sizeof(bcm_if_t) * port_count,
                    "temp encap array");
            if (NULL == temp_encap_array) {
                rv = BCM_E_MEMORY;
                goto done;
            }
        }
        sal_memset(temp_encap_array, 0, sizeof(bcm_if_t) * port_count);

        if (NULL == sublist_id_array) {
            sublist_id_array = sal_alloc(sizeof(int) * port_count,
                    "sublist id array");
            if (NULL == sublist_id_array) {
                rv = BCM_E_MEMORY;
                goto done;
            }
        }
        sal_memset(sublist_id_array, 0, sizeof(int) * port_count);

        temp_encap_count = 0;
        for (j = i; j < port_count; j++) {
            if (!BCM_GPORT_IS_TRUNK(mapped_port_array[j])) {
                continue;
            }
            if (BCM_IF_INVALID == encap_id_array[j]) {
                continue;
            }
            if (BCM_GPORT_TRUNK_GET(mapped_port_array[j]) != trunk_id) {
                continue;
            }
            temp_encap_array[temp_encap_count++] = encap_id_array[j];
        }

        /* Split the current trunk group's list of encap ids into sublists,
         * such that the sublists match existing replication lists,
         * enabling sharing of IPMC replication table entries.
         * It's not necessary to generate more sublists than there are
         * trunk members, since mapping multiple sublists to a trunk member
         * would effectively join those sublists.
         */

        sublist_max = num_local_trunk_ports;
        for (j = 0; j < temp_encap_count; j++) {
            /* Initialize each encap id's sublist id to be
             * the invalid value of sublist_max.
             */
            sublist_id_array[j] = sublist_max;
        }

#if defined(BCM_TRIUMPH2_SUPPORT) 
        if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit) || SOC_IS_TRIDENT(unit) ||
                SOC_IS_KATANA(unit)) {

            rv = bcm_tr2_ipmc_repl_list_split(unit, temp_encap_count,
                    temp_encap_array, sublist_id_array, 
                    sublist_max, &sublist_count);
            if (BCM_FAILURE(rv)) {
                goto done;
            }
        } 
#endif  /* BCM_TRIUMPH2_SUPPORT */

        /* If the number of sublists used, given by sublist_count,
         * is less than the max number of sublists allowed,
         * given by sublist_max, the encap ids that are not yet
         * a member of any sublist are assigned to the remaining
         * free sublists.
         * If there are no free sublist left, the encap ids that are
         * not yet a member of any sublist are assigned to an
         * existing sublist selected by hash.
         */
        if (sublist_count < sublist_max) {
            first_free_sublist_id = sublist_count;
            next_free_sublist_id = first_free_sublist_id;
            for (j = 0; j < temp_encap_count; j++) {
                if (sublist_id_array[j] == sublist_max) {
                    sublist_id_array[j] = next_free_sublist_id;
                    if ((next_free_sublist_id + 1) == sublist_max) {
                        /* wrap around to the first free sublist id */
                        next_free_sublist_id = first_free_sublist_id;
                    } else {
                        next_free_sublist_id++;
                    }
                }
            }
        } else if (sublist_count == sublist_max) {
            selected_sublist_id = hash % sublist_max;
            for (j = 0; j < temp_encap_count; j++) {
                if (sublist_id_array[j] == sublist_max) {
                    sublist_id_array[j] = selected_sublist_id;
                }
            }
        } else {
            /* sublist_count should never be greater than sublist_max */
            rv = BCM_E_INTERNAL;
            goto done;
        }

        /* The sublists will be assigned to trunk members in a round-robin
         * fashion, with the starting trunk member selected by hash.
         * Sublist 0 will be assigned to a local trunk member selected
         * by hashing. Then sublist 1 will be assigned to the
         * next trunk member in local_trunk_port_array, and so on.
         */
        sublist0_trunk_member_index = hash % num_local_trunk_ports;
        temp_encap_index = 0;
        for (j = i; j < port_count; j++) {
            if (!BCM_GPORT_IS_TRUNK(mapped_port_array[j])) {
                continue;
            }
            if (BCM_IF_INVALID == encap_id_array[j]) {
                continue;
            }
            if (BCM_GPORT_TRUNK_GET(mapped_port_array[j]) != trunk_id) {
                continue;
            }
            if (temp_encap_array[temp_encap_index] != encap_id_array[j]) {
                /* They should always match */
                rv = BCM_E_INTERNAL;
                goto done;
            }

            trunk_member_index = (sublist0_trunk_member_index +
                sublist_id_array[temp_encap_index]) % num_local_trunk_ports;

            mapped_port = local_trunk_port_array[trunk_member_index];
            rv = bcm_esw_port_gport_get(unit, mapped_port,
                    &mapped_port_array[j]);
            if (BCM_FAILURE(rv)) {
                goto done;
            }
            temp_encap_index++;
        }
    }

done:
    if (hash_key) {
        sal_free(hash_key);
    }
    if (temp_encap_array) {
        sal_free(temp_encap_array);
    }
    if (sublist_id_array) {
        sal_free(sublist_id_array);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_esw_multicast_l3_group_check
 * Purpose:
 *      Helper function to validate L3 or Virtual multicast group
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      is_l3     - (OUT) L3 Group identifier for Firebolt
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_multicast_l3_group_check(int unit, bcm_multicast_t group, int *is_l3)
{
    if (_BCM_MULTICAST_IS_L3(group) 
        || _BCM_MULTICAST_IS_TRILL(group)
        || _BCM_MULTICAST_IS_VXLAN(group)) {
        if (NULL != is_l3) {    /* If caller cares about this information */
            *is_l3 = 1;
        }        
    } else if (!SOC_IS_TRX(unit)) {
        /* Pre TX devices don't support Virutal multicast groups */
        return (BCM_E_PARAM);
    } else if (!_BCM_MULTICAST_IS_VPLS(group) &&
               !_BCM_MULTICAST_IS_MIM(group) &&
               !_BCM_MULTICAST_IS_WLAN(group) &&
               !_BCM_MULTICAST_IS_VLAN(group) &&
               !_BCM_MULTICAST_IS_NIV(group) &&
               !_BCM_MULTICAST_IS_L2GRE(group) &&
               !_BCM_MULTICAST_IS_SUBPORT(group) &&
               !_BCM_MULTICAST_IS_EGRESS_OBJECT(group) &&
               !_BCM_MULTICAST_IS_EXTENDER(group)) {
        return (BCM_E_PARAM);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_esw_multicast_l3_add
 * Purpose:
 *      Helper function to add a port to L3 or Virtual multicast group
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_multicast_l3_add(int unit, bcm_multicast_t group, 
                          bcm_gport_t port, bcm_if_t encap_id)

{
    int mc_index, is_l3, gport_id, idx, trunk_local_ports = 0; 
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int modid_local, rv = BCM_E_NONE; 
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    bcm_port_t      local_trunk_member_ports[SOC_MAX_NUM_PP_PORTS] = {0};
    int             max_num_ports = SOC_MAX_NUM_PP_PORTS;
#else
    bcm_port_t      local_trunk_member_ports[SOC_MAX_NUM_PORTS] = {0};
    int             max_num_ports = SOC_MAX_NUM_PORTS;
#endif
    bcm_pbmp_t l2_pbmp, l3_pbmp, old_pbmp, *new_pbmp;
    int if_max = BCM_MULTICAST_PORT_MAX;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    /* Size of "L3 Next hop table is limited to 2K on dagger(Enduro variant).
     * On Hurricane2 and Ranger+ max replication interface is limited by
     * number of EGR_L3_Intfm.
     */

    int nexthop_count = 0, egr_intf_count = 0;
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE2(unit)
        || SOC_IS_TRIUMPH3(unit)) {
        nexthop_count =  soc_mem_index_count(unit, ING_L3_NEXT_HOPm);
        egr_intf_count = soc_mem_index_count(unit, EGR_L3_INTFm);
        if_max = (nexthop_count < egr_intf_count)?
             ((if_max < nexthop_count)? if_max : nexthop_count):
             ((if_max < egr_intf_count)?  if_max : egr_intf_count);
    }
#endif /* BCM_ENDURO_SUPPORT || BCM_TRIUMPH3_SUPPORT || BCM_HURRICANE2_SUPPORT*/

    is_l3 = 0;      /* Needed to support Firebolt style of L3 Multicast */
    trunk_local_ports = 0; 
    modid_local = FALSE;

    mc_index = _BCM_MULTICAST_ID_GET(group);

    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_multicast_l3_group_check(unit, group, &is_l3));

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_trill)) {
        if (_BCM_MULTICAST_IS_TRILL(group)) {
             BCM_IF_ERROR_RETURN
                (bcm_td_trill_egress_add(unit, group));
        }
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, 
                                &trunk_id, &gport_id));

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        if (BCM_GPORT_IS_SET(port) &&
            _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit, port)) {
            if (mod_out == (my_modid + 1)) {
                pp_port = BCM_GPORT_SUBPORT_PORT_GET(port);
                if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                    (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport == FALSE) {
                return BCM_E_PORT;
            }
        }
    }
    if (is_local_subport) {
        port_out = pp_port;
    } else
#endif

    if (BCM_TRUNK_INVALID != trunk_id) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_trunk_local_members_get(unit, trunk_id, 
                                             max_num_ports,
                                             local_trunk_member_ports, 
                                             &trunk_local_ports));
        /* Convertion of system ports to physical ports done in */
        /* _bcm_esw_trunk_local_members_get */
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &modid_local));
        if (TRUE != modid_local) {
            /* Only add this to replication set if destination is local */
            return BCM_E_PORT;
        }

        /* Convert system port to physical port */
        if (soc_feature(unit, soc_feature_sysport_remap)) {
            BCM_XLATE_SYSPORT_S2P(unit, &port_out);
        }
    }

#ifdef BCM_TRIUMPH3_SUPPORT 
    if (SOC_IS_TRIUMPH3(unit)) {
        int use_axp_port = 0;
        bcm_l3_egress_t l3_egr;

        if (_BCM_MULTICAST_IS_WLAN(group)) {
            /* Triumph3 requires WLAN replications to be configured on
             * AXP port.
             */
            use_axp_port = 1;
        } else if (_BCM_MULTICAST_IS_L3(group)) {
            /* Triumph3 requires IPMC replications destined for WLAN
             * virtual ports to be configured on AXP port.
             */
            if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, encap_id)) {

                bcm_l3_egress_t_init(&l3_egr);
                rv = bcm_esw_l3_egress_get(unit, encap_id, &l3_egr);
                if (BCM_SUCCESS(rv)) { 
                    if (BCM_GPORT_IS_WLAN_PORT(l3_egr.port)) {
                        use_axp_port = 1;
                    }
                }
            }
        }

        if (use_axp_port) {
            trunk_id = BCM_TRUNK_INVALID;
            port_out = AXP_PORT(unit, SOC_AXP_NLF_WLAN_ENCAP);
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    if (BCM_IF_INVALID != encap_id) {
        /* Add all local ports in trunk to the multicast egress interface */
        if (BCM_TRUNK_INVALID != trunk_id) {
            for (idx = 0; idx < trunk_local_ports; idx++) {
                rv = _bcm_esw_ipmc_egress_intf_add(unit, mc_index, 
                                               local_trunk_member_ports[idx],
                                               encap_id, is_l3);
                if (BCM_FAILURE(rv)) {
                    while (idx--) {
                        /* Error code ignored because we already in error */
                        (void) _bcm_esw_ipmc_egress_intf_delete(unit,mc_index, 
                                                    local_trunk_member_ports[idx],
                                                    if_max, 
                                                    encap_id, is_l3);
                    }
                    return rv;
                }
            }
        } else {
            /* Add  the required port to the multicast egress interface */
            BCM_IF_ERROR_RETURN(
                _bcm_esw_ipmc_egress_intf_add(unit, mc_index, port_out, 
                                              encap_id, is_l3));
        }
        new_pbmp = &l3_pbmp;
    } else {
        if (!_BCM_MULTICAST_IS_VPLS(group)) {
            /* Updating the L2 bitmap */
            new_pbmp = &l2_pbmp;
        } else {
            return BCM_E_PARAM;
        }
    }     

    /* Add port to the IPMC_L3_BITMAP or L2_BITMAP as decided above */
    rv = _bcm_esw_multicast_ipmc_read(unit, mc_index, &l2_pbmp, &l3_pbmp);

    if (BCM_SUCCESS(rv)) {
        BCM_PBMP_ASSIGN(old_pbmp, *new_pbmp);
        if (BCM_TRUNK_INVALID != trunk_id) {
            for (idx = 0; idx < trunk_local_ports; idx++) {
                BCM_PBMP_PORT_ADD(*new_pbmp, local_trunk_member_ports[idx]);
            }
        } else {
            BCM_PBMP_PORT_ADD(*new_pbmp, port_out);
        }
        if (BCM_PBMP_NEQ(old_pbmp, *new_pbmp)) {
            rv = _bcm_esw_multicast_ipmc_write(unit, mc_index, l2_pbmp, 
                                               l3_pbmp, TRUE);
        }
    } 
    if (BCM_FAILURE(rv) && (encap_id != BCM_IF_INVALID)) {
        if (BCM_TRUNK_INVALID != trunk_id) {
             for (idx = 0; idx < trunk_local_ports; idx++) {
                 (void) _bcm_esw_ipmc_egress_intf_delete(unit,
                             mc_index, local_trunk_member_ports[idx],
                             if_max, encap_id, is_l3);
             }
        } else {
           (void) _bcm_esw_ipmc_egress_intf_delete(unit, mc_index, port_out, 
                                   if_max, encap_id, is_l3);
        }
    }

    return rv;
}

/*
 * Function:
 *      _bcm_esw_multicast_l3_delete
 * Purpose:
 *      Helper function to delete a port from L3 or Virtual multicast group
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_multicast_l3_delete(int unit, bcm_multicast_t group, 
                             bcm_gport_t port, bcm_if_t encap_id)

{
    int mc_index, is_l3, gport_id, i, idx, trunk_local_ports = 0; 
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int modid_local, local_count, port_count, rv = BCM_E_NONE; 
    bcm_port_t    temp_port;
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    bcm_port_t    local_trunk_member_ports[SOC_MAX_NUM_PP_PORTS] = {0};
    int           max_num_ports = SOC_MAX_NUM_PP_PORTS;
#else
    bcm_port_t    local_trunk_member_ports[SOC_MAX_NUM_PORTS] = {0};
    int           max_num_ports = SOC_MAX_NUM_PORTS;
#endif
    bcm_pbmp_t l2_pbmp, l3_pbmp, old_pbmp;
    bcm_gport_t *port_array;
    bcm_if_t *encap_array;
    int if_max = BCM_MULTICAST_PORT_MAX;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    /* Size of "L3 Next hop table is limited to 2K on dagger(Enduro variant).
     * On Hurricane2 and Ranger+ max replication interface is limited by 
     * number of EGR_L3_Intfm.
     */

    int nexthop_count = 0, egr_intf_count = 0;
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE2(unit) 
        || SOC_IS_TRIUMPH3(unit)) {
        nexthop_count =  soc_mem_index_count(unit, ING_L3_NEXT_HOPm);
        egr_intf_count = soc_mem_index_count(unit, EGR_L3_INTFm);
        if_max = (nexthop_count < egr_intf_count)?
                 ((if_max < nexthop_count)? if_max : nexthop_count):
                 ((if_max < egr_intf_count)? if_max : egr_intf_count);
    }
#endif /* BCM_ENDURO_SUPPORT || BCM_TRIUMPH3_SUPPORT || BCM_HURRICANE2_SUPPORT*/

#ifdef BCM_GREYHOUND_SUPPORT
    if (soc_feature(unit, soc_feature_l3mc_use_egress_next_hop)) {
        /* To assign proper if_max to avoid delete operation got unexpect 
         * parameter validation error due to if_max is default at 4096.
         *
         * GH has egress logic changed for IPMC replication to reach 
         *  EGR_L3_NEXT_HOPm before EGR_L3_INTFm. But the maximum replication 
         *  still limited by EGR_L3_INTFm.
         */
        if_max = soc_mem_index_count(unit, EGR_L3_INTFm);
    }
#endif /* BCM_GREYHOUND_SUPPORT */
    
    is_l3 = 0;      /* Needed to support Firebolt style of L3 Multicast */
    trunk_local_ports = 0; 
    modid_local = FALSE;
    port_count = local_count = 0;

    mc_index = _BCM_MULTICAST_ID_GET(group);

    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_multicast_l3_group_check(unit, group, &is_l3));

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_trill)) {
        if (_BCM_MULTICAST_IS_TRILL(group)) {
             BCM_IF_ERROR_RETURN
                (bcm_td_trill_egress_delete(unit, group));
        }
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, 
                                &trunk_id, &gport_id)); 

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        if (BCM_GPORT_IS_SET(port) &&
            _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit, port)) {
            if (mod_out == (my_modid + 1)) {
                pp_port = BCM_GPORT_SUBPORT_PORT_GET(port);
                if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                    (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport == FALSE) {
                return BCM_E_PORT;
            }
        }
    }
    if (is_local_subport) {
        port_out = pp_port;
    } else
#endif

    if (BCM_TRUNK_INVALID != trunk_id) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_trunk_local_members_get(unit, trunk_id, 
                                             max_num_ports,
                                             local_trunk_member_ports, 
                                             &trunk_local_ports));
        /* Convertion of system ports to physical ports done in */
        /* _bcm_esw_trunk_local_members_get */
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &modid_local));
        if (TRUE != modid_local) {
            /* Only add this to replication set if destination is local */
            return BCM_E_PORT;
        }

        /* Convert system port to physical port */
        if (soc_feature(unit, soc_feature_sysport_remap)) {
            BCM_XLATE_SYSPORT_S2P(unit, &port_out);
        }
    }

#ifdef BCM_TRIUMPH3_SUPPORT 
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit)) {
        if (_BCM_MULTICAST_IS_WLAN(group)) {
            trunk_id = BCM_TRUNK_INVALID;
            port_out = AXP_PORT(unit, SOC_AXP_NLF_WLAN_ENCAP);
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    if (encap_id == BCM_IF_INVALID) {
        /* Remove L2 port */
        if (_BCM_MULTICAST_IS_VPLS(group)) {
            return BCM_E_PARAM;
        }

        soc_mem_lock(unit, L3_IPMCm);
        /* Delete the port from the IPMC L2_BITMAP */
        rv = _bcm_esw_multicast_ipmc_read(unit, mc_index, 
                                          &l2_pbmp, &l3_pbmp);
        if (BCM_SUCCESS(rv)) {
            BCM_PBMP_ASSIGN(old_pbmp, l2_pbmp);
            if (BCM_TRUNK_INVALID != trunk_id) {
                for (idx = 0; idx < trunk_local_ports; idx++) {
                    BCM_PBMP_PORT_REMOVE(l2_pbmp,
                                         local_trunk_member_ports[idx]);
                }
            } else {
                BCM_PBMP_PORT_REMOVE(l2_pbmp, port_out);
            }
            if (BCM_PBMP_NEQ(old_pbmp, l2_pbmp)) {
                rv = _bcm_esw_multicast_ipmc_write(unit, mc_index,
                                        l2_pbmp, l3_pbmp, TRUE);
            } else {
                rv = BCM_E_NOT_FOUND;
            }
        }
        soc_mem_unlock(unit, L3_IPMCm);
        return rv;
    }

    /*
     * Walk through the list of egress interfaces for this group.
     * Check if the interface getting deleted is the ONLY
     * instance of the local port in the replication list.
     * If so, we can delete the port from the IPMC L3_BITMAP.
     */
    port_array = sal_alloc(if_max * sizeof(bcm_gport_t),
                           "mcast port array");
    if (port_array == NULL) {
        return BCM_E_MEMORY;
    }
    encap_array = sal_alloc(if_max * sizeof(bcm_if_t),
                            "mcast encap array");
    if (encap_array == NULL) {
        sal_free (port_array);
        return BCM_E_MEMORY;
    }

    rv = bcm_esw_multicast_egress_get(unit, group, if_max, 
                                      port_array, encap_array, &port_count);
    if (BCM_FAILURE(rv)) {
        sal_free (port_array);
        sal_free (encap_array);
        return (rv);
    } 

    for (i = 0; i < port_count ; i++) {
        if (encap_id == encap_array[i]) {
            /* Skip the one we're about to delete */
            continue;
        }

#if defined(BCM_KATANA2_SUPPORT)
        is_local_subport = FALSE;
        if (soc_feature(unit, soc_feature_linkphy_coe) ||
            soc_feature(unit, soc_feature_subtag_coe)) {
            if (BCM_GPORT_IS_SET(port_array[i]) &&
                _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                port_array[i])) {
                is_local_subport = TRUE;
            }
        }
        if (is_local_subport) {
            temp_port = _BCM_KT2_SUBPORT_PORT_ID_GET(port_array[i]);
        } else
#endif
        {
            (void) bcm_esw_port_local_get(unit, port_array[i], &temp_port);
        }

        if (temp_port == port_out) {
            local_count = 1;
            break;
        } else {
            for (idx = 0; idx < trunk_local_ports; idx++) {
                if (temp_port == local_trunk_member_ports[idx]) {
                       local_count = 1;
                       break;
                    }
            }
            if (local_count) {
                break;
            }
        }
    }
    sal_free(port_array);
    sal_free(encap_array);

    /* Delete the port from the IPMC L3_BITMAP if local_count is 0
     * and if the chip does not support IPMC cut-through.
     * For chips that support IPMC cut-through (indicated by the existence
     * of L3_IPMC.REPL_HEAD_BASE_PTR field), the L3_BITMAP and the
     * REPL_HEAD_BASE_PTR fields need to be updated atomically, which
     * is done within _bcm_esw_ipmc_egress_intf_delete function.
     */
    if (!local_count && !soc_mem_field_valid(unit, L3_IPMCm,
                REPL_HEAD_BASE_PTRf)) {
        soc_mem_lock(unit, L3_IPMCm);
        rv = _bcm_esw_multicast_ipmc_read(unit, mc_index, 
                                          &l2_pbmp, &l3_pbmp);
        if (BCM_SUCCESS(rv)) {
            if (BCM_TRUNK_INVALID != trunk_id) {
                for (idx = 0; idx < trunk_local_ports; idx++) {
                    BCM_PBMP_PORT_REMOVE(l3_pbmp,
                                         local_trunk_member_ports[idx]);
                }
            } else {
                BCM_PBMP_PORT_REMOVE(l3_pbmp, port_out);
            }
            rv = _bcm_esw_multicast_ipmc_write(unit, mc_index, l2_pbmp,
                                               l3_pbmp, TRUE);
        }
        soc_mem_unlock(unit, L3_IPMCm);
        BCM_IF_ERROR_RETURN(rv);
    }

    if (BCM_TRUNK_INVALID != trunk_id) {
        for (idx = 0; idx < trunk_local_ports; idx++) {
            rv =  _bcm_esw_ipmc_egress_intf_delete(unit, mc_index,
                                local_trunk_member_ports[idx], if_max, 
                                               encap_id, is_l3);
            if (rv < 0) {
                while (idx--) {
                    (void) _bcm_esw_ipmc_egress_intf_add(unit, mc_index,
                                local_trunk_member_ports[idx], encap_id, is_l3);
                }
                return rv;
            }
        }
    } else {
        rv =  _bcm_esw_ipmc_egress_intf_delete(unit, mc_index, port_out,
                                       if_max, encap_id, is_l3);
    }

    return rv;
}


/*
 * Function:
 *      _bcm_esw_multicast_l3_get
 * Purpose:
 *      Helper function to get ports that are part of L3 or Virtual multicast 
 * Parameters: 
 *      unit           - (IN) Device Number
 *      mc_index       - (IN) Multicast index
 *      port_max       - (IN) Number of entries in "port_array"
 *      port_array     - (OUT) List of ports
 *      encap_id_array - (OUT) List of encap identifiers
 *      port_count     - (OUT) Actual number of ports returned
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the input parameter port_max = 0, return in the output parameter
 *      port_count the total number of ports/encapsulation IDs in the 
 *      specified multicast group's replication list.
 */

STATIC int     
_bcm_esw_multicast_l3_get(int unit, bcm_multicast_t group, int port_max,
                             bcm_gport_t *port_array, bcm_if_t *encap_id_array, 
                             int *port_count)
{
    int             mc_index, i, count, rv;
    bcm_pbmp_t      ether_higig_pbmp, l2_pbmp, l3_pbmp;
    bcm_port_t      port_iter;
    bcm_if_t        *local_id_array;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE;
#endif

    i= 0;
    rv = BCM_E_NONE;
    local_id_array = NULL;

    mc_index = _BCM_MULTICAST_ID_GET(group);
    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_multicast_l3_group_check(unit, group, NULL));

    if (port_max > 0) {
        local_id_array = sal_alloc(sizeof(bcm_if_t) * port_max,
                                   "local array of interfaces");
        if (NULL == local_id_array) {
            return BCM_E_MEMORY;
        }
        sal_memset(local_id_array, 0x00, sizeof(bcm_if_t) * port_max);
    }

    /* Collect list of GPORTs for each front-panel and Higig port. */
    *port_count = 0;
    BCM_PBMP_ASSIGN(ether_higig_pbmp, PBMP_E_ALL(unit));
    BCM_PBMP_OR(ether_higig_pbmp, PBMP_HG_ALL(unit));

    /* CPU ports are valid for replication on TD2 */
    if(SOC_IS_TRIDENT2(unit)) {
        BCM_PBMP_OR(ether_higig_pbmp, PBMP_CMIC(unit));
    }

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_subtag_coe) ||
        soc_feature(unit, soc_feature_linkphy_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &ether_higig_pbmp);
    }
#endif
    BCM_PBMP_ITER(ether_higig_pbmp, port_iter) {
        if (port_max > 0) {
            rv = bcm_esw_ipmc_egress_intf_get(unit, mc_index, port_iter,
                                             port_max - *port_count, 
                                             local_id_array, 
                                             &count);
        } else {
            rv = bcm_esw_ipmc_egress_intf_get(unit, mc_index, port_iter,
                                             0,
                                             NULL, 
                                             &count);
        }
        if (BCM_FAILURE(rv)) {
            sal_free((void *)local_id_array);
            return rv;
        }
#if defined(BCM_KATANA2_SUPPORT)
        /* For KT2, pp_port >=42 are used for LinkPHY/SubportPktTag subport.
         * Get the subport info associated with the pp_port and
         * form the subport_gport.
         */ 
        is_local_subport = FALSE;
        if ((soc_feature(unit, soc_feature_linkphy_coe) ||
            soc_feature(unit, soc_feature_subtag_coe)) &&
            (port_iter >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
            (port_iter <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
            is_local_subport = TRUE;
        }
#endif

        for (i = 0; i < count; i++) {
            /* Copy data to provided arrays */
            if ((NULL != encap_id_array) && (NULL != local_id_array)) {
                if (soc_feature(unit, soc_feature_l3mc_use_egress_next_hop)) {
                    /* Translate the HW index to encap_id */
                    encap_id_array[*port_count + i] = local_id_array[i] +
                            BCM_XGS3_EGRESS_IDX_MIN;
                } else {
                    encap_id_array[*port_count + i] = local_id_array[i];
                }
            }
            if (NULL != port_array) {
                /* Convert to GPORT values */
#if defined(BCM_KATANA2_SUPPORT)
                if (is_local_subport) {

                    _BCM_KT2_SUBPORT_PORT_ID_SET(port_array[*port_count + i],
                            port_iter);
                    if (BCM_PBMP_MEMBER(
                        SOC_INFO(unit).linkphy_pp_port_pbm, port_iter)) {
                        _BCM_KT2_SUBPORT_PORT_TYPE_SET(
                            port_array[*port_count + i],
                            _BCM_KT2_SUBPORT_TYPE_LINKPHY);
                    } else if (BCM_PBMP_MEMBER(
                        SOC_INFO(unit).subtag_pp_port_pbm, port_iter)) {
                        _BCM_KT2_SUBPORT_PORT_TYPE_SET(
                            port_array[*port_count + i],
                            _BCM_KT2_SUBPORT_TYPE_SUBTAG);
                    } else {
                        sal_free((void *)local_id_array);
                        return BCM_E_PORT;
                    }
                } else
#endif /* BCM_KATANA2_SUPPORT */
                {
                    rv = bcm_esw_port_gport_get(unit, port_iter, 
                                  &(port_array[(*port_count + i)]));
                    if (BCM_FAILURE(rv)) {
                        sal_free((void *)local_id_array);
                        return rv;
                    }
                }
            }
        }
        *port_count += count;
        if ((port_max > 0) && (*port_count == port_max)) {
            break;
        }
    }

    /* Check for L2 ports */
    rv =_bcm_esw_multicast_ipmc_read(unit, mc_index, &l2_pbmp, &l3_pbmp);
    if (BCM_FAILURE(rv)) {
        sal_free((void *)local_id_array);
        return rv;
    }

    BCM_PBMP_ITER(l2_pbmp, port_iter) {
        if ((port_max > 0) && (*port_count == port_max)) {
            break;
        }
        if (NULL != port_array) {
#if defined(BCM_KATANA2_SUPPORT)
            /* For KT2, pp_port >=42 are used for LinkPHY/SubportPktTag subport.
             * Get the subport info associated with the pp_port and
             * form the subport_gport.
             */ 
            is_local_subport = FALSE;
            if ((soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) &&
                (port_iter >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                (port_iter <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                is_local_subport = TRUE;
            }

            if (is_local_subport) {
                _BCM_KT2_SUBPORT_PORT_ID_SET(port_array[*port_count],
                    port_iter);
                if (BCM_PBMP_MEMBER(
                    SOC_INFO(unit).linkphy_pp_port_pbm, port_iter)) {
                    _BCM_KT2_SUBPORT_PORT_TYPE_SET(port_array[*port_count],
                        _BCM_KT2_SUBPORT_TYPE_LINKPHY);
                } else if (BCM_PBMP_MEMBER(
                    SOC_INFO(unit).subtag_pp_port_pbm, port_iter)) {
                    _BCM_KT2_SUBPORT_PORT_TYPE_SET(port_array[*port_count],
                        _BCM_KT2_SUBPORT_TYPE_SUBTAG);
                } else {
                    sal_free((void *)local_id_array);
                    return BCM_E_PORT;
                }
    
            } else
#endif /* BCM_KATANA2_SUPPORT */
            {
                rv = bcm_esw_port_gport_get(unit, port_iter, 
                                        &(port_array[*port_count]));

                if (BCM_FAILURE(rv)) {
                    sal_free((void *)local_id_array);
                    return rv;
                }
            }
        }
        if (NULL != encap_id_array) {
            encap_id_array[*port_count] = BCM_IF_INVALID;
        }
        *port_count += 1;
    }

    if (local_id_array) {
        sal_free((void *)local_id_array);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_multicast_l3_create
 * Purpose:
 *      Helper function to allocate a multicast group index for L3 or Virtual
 * Parameters:
 *      unit       - (IN)   Device Number
 *      flags      - (IN)   BCM_MULTICAST_*
 *      group      - (OUT)  Group ID
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_multicast_l3_create(int unit, uint32 flags, bcm_multicast_t *group)
{
    int                 mc_index, rv;
    int                 is_set = FALSE;
    egr_ipmc_entry_t    egr_ipmc;
    bcm_pbmp_t          active, l2_pbmp, l3_pbmp;
    uint32              type;
    uint32 entry[SOC_MAX_MEM_FIELD_WORDS];  /* hw entry bufffer. */

    /* Check if IPMC module is initialized */
    IPMC_INIT(unit);

    rv  = BCM_E_NONE;
    type = flags & BCM_MULTICAST_TYPE_MASK;

    if (flags & BCM_MULTICAST_WITH_ID) {
        mc_index = _BCM_MULTICAST_ID_GET(*group);
        if ((mc_index < 0) ||
            (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
            return BCM_E_PARAM;
        }
        rv = bcm_xgs3_ipmc_id_is_set(unit, mc_index, &is_set);
        if (BCM_SUCCESS(rv)) {
            if (is_set) {
                return BCM_E_EXISTS;
            }
        } else {
            return rv;
        }
        rv = bcm_xgs3_ipmc_id_alloc(unit, mc_index);
        if (BCM_FAILURE(rv)) {
            return rv;
        }
    } else {
        /* Allocate an IPMC index */
        BCM_IF_ERROR_RETURN(bcm_xgs3_ipmc_create(unit, &mc_index));
    }

    /*Per IPMC Group Attributes */
    sal_memset(&egr_ipmc, 0, sizeof(egr_ipmc));

    if ((type & _BCM_VIRTUAL_TYPES_MASK) && 
        (type != BCM_MULTICAST_TYPE_WLAN)) {
        if (soc_mem_field_valid(unit, EGR_IPMCm, REPLICATION_TYPEf)) {
            soc_EGR_IPMCm_field32_set(unit, &egr_ipmc,
                                      REPLICATION_TYPEf, 0x1);
            /* Next-hop indexes from MMU */
        }
        if (soc_mem_field_valid(unit, EGR_IPMCm, DONT_PRUNE_VLANf)) {
            soc_EGR_IPMCm_field32_set(unit, &egr_ipmc, DONT_PRUNE_VLANf, 1);
        }
    }

    /* Commit the values to HW */
    if (SOC_MEM_IS_VALID(unit, EGR_IPMCm)) {
        rv = WRITE_EGR_IPMCm(unit, MEM_BLOCK_ALL, mc_index, &egr_ipmc);
        if (BCM_FAILURE(rv)) {
            (void) bcm_xgs3_ipmc_id_free(unit, mc_index);
            return rv;
        }
    }

    /* Add stack ports to L2 PBMP */
    BCM_PBMP_CLEAR(l2_pbmp);
    BCM_PBMP_CLEAR(l3_pbmp);
    SOC_PBMP_STACK_ACTIVE_GET(unit, active);
    BCM_PBMP_OR(l2_pbmp, active);
    BCM_PBMP_REMOVE(l2_pbmp, SOC_PBMP_STACK_INACTIVE(unit));

    rv = _bcm_esw_multicast_ipmc_write(unit, mc_index, l2_pbmp,
                                       l3_pbmp, TRUE);
    if (BCM_FAILURE(rv)) {
        sal_memset(&egr_ipmc, 0, sizeof(egr_ipmc));
        if (SOC_MEM_IS_VALID(unit, EGR_IPMCm)) {
            (void) WRITE_EGR_IPMCm(unit, MEM_BLOCK_ALL, mc_index, 
                                   &egr_ipmc);
            (void) bcm_xgs3_ipmc_id_free(unit, mc_index);
            return rv;
        }
    }

    if (type == BCM_MULTICAST_TYPE_L3) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_L3,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_VPLS) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_VPLS,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_MIM) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_MIM,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_WLAN) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_WLAN,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_SUBPORT) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_SUBPORT,
                                 mc_index);
    }else if (type == BCM_MULTICAST_TYPE_TRILL) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_TRILL,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_VLAN) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_VLAN,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_NIV) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_NIV,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_EGRESS_OBJECT) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_EGRESS_OBJECT,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_L2GRE) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_L2GRE,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_VXLAN) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_VXLAN,
                                 mc_index);
    } else if (type == BCM_MULTICAST_TYPE_EXTENDER) {
        _BCM_MULTICAST_GROUP_SET(*group, _BCM_MULTICAST_TYPE_EXTENDER,
                                 mc_index);
    }

    if (soc_feature(unit, soc_feature_trill)) {
        if (type == BCM_MULTICAST_TYPE_TRILL) {
            if (SOC_MEM_FIELD_VALID(unit, L3_IPMCm, REMOVE_SGLP_FROM_L3_BITMAPf)) {
                rv = soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, mc_index, entry);
                if (BCM_SUCCESS(rv)) {
                    soc_mem_field32_set(unit, L3_IPMCm, entry, 
                                    REMOVE_SGLP_FROM_L3_BITMAPf, 0x1);
                    rv = soc_mem_write(unit, L3_IPMCm, MEM_BLOCK_ALL, mc_index, &entry);
                }
            }
        }
    }

    if (BCM_SUCCESS(rv) && SOC_IS_TRX(unit)) {
        /* Record IPMC types for reference */
        rv = _bcm_tr_multicast_ipmc_group_type_set(unit, *group);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_esw_multicast_l3_destroy
 * Purpose:
 *      Helper function to destroy L3 and Virtual multicast 
 * Parameters: 
 *      unit           - (IN) Device Number
 *      group          - (IN) Multicast group to destroy
 * Returns:
 *      BCM_E_XXX
 */

STATIC int     
_bcm_esw_multicast_l3_destroy(int unit, bcm_multicast_t group)
{
    int                 is_l3, mc_index;
    bcm_port_t          port_iter;
    bcm_pbmp_t          ether_higig_pbmp, l2_pbmp, l3_pbmp;
    egr_ipmc_entry_t    egr_ipmc;

    /* Check if IPMC module is initialized */
    IPMC_INIT(unit);

    is_l3 = 0;
    
    BCM_IF_ERROR_RETURN(_bcm_esw_multicast_l3_group_check(unit, group, &is_l3));

    mc_index = _BCM_MULTICAST_ID_GET(group);
    if ((mc_index < 0) || (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    /* Decrement ref count by calling 'free' */
    BCM_IF_ERROR_RETURN(bcm_xgs3_ipmc_id_free(unit, mc_index));

    if (IPMC_USED_ISSET(unit, mc_index)) {
        /* Group is in use, correct the ref count
         * decremented by above 'free' and return error. */
        IPMC_USED_SET(unit, mc_index)                                   \
        return BCM_E_BUSY;
    } else { /* Ref count is zero. Group is not being used. */
        /* Clear the replication set */
        _bcm_esw_multicast_ipmc_read(unit, mc_index, &l2_pbmp, &l3_pbmp);
        if(is_l3) {
           BCM_PBMP_ASSIGN(ether_higig_pbmp, l3_pbmp);
        } else {
           BCM_PBMP_ASSIGN(ether_higig_pbmp, l2_pbmp);
        }
        BCM_PBMP_OR(ether_higig_pbmp, PBMP_HG_ALL(unit));
        /* CPU ports are valid for replication on TD2 */
        if(SOC_IS_TRIDENT2(unit)) {
            BCM_PBMP_OR(ether_higig_pbmp, PBMP_CMIC(unit));
        }

#if defined(BCM_KATANA2_SUPPORT)
        if (soc_feature(unit, soc_feature_subtag_coe) ||
            soc_feature(unit, soc_feature_linkphy_coe)) {
           _bcm_kt2_subport_pbmp_update(unit, &ether_higig_pbmp);
       }
#endif

        BCM_PBMP_ITER(ether_higig_pbmp, port_iter) {
            BCM_IF_ERROR_RETURN(
                 _bcm_esw_ipmc_egress_intf_set(unit, mc_index, port_iter, 0, 
                                               NULL, is_l3, FALSE));
        }

        /* Clear the IPMC related tables */
        BCM_PBMP_CLEAR(l2_pbmp);
        BCM_PBMP_CLEAR(l3_pbmp);

        BCM_IF_ERROR_RETURN(
        _bcm_esw_multicast_ipmc_write(unit, mc_index, l2_pbmp, l3_pbmp, FALSE));

        if (SOC_MEM_IS_VALID(unit, EGR_IPMCm)) {
            sal_memset(&egr_ipmc, 0, sizeof(egr_ipmc));
            BCM_IF_ERROR_RETURN
                (WRITE_EGR_IPMCm(unit, MEM_BLOCK_ALL, mc_index, &egr_ipmc));
        }

        /* Free IPMC type record */
        if (SOC_IS_TRX(unit)) {
            BCM_IF_ERROR_RETURN(
                _bcm_tr_multicast_ipmc_group_type_set(unit, mc_index));
        }
    }
    
    return BCM_E_NONE;
}
#endif /* INCLUDE_L3 */

/*
 * Function:
 *      _bcm_esw_multicast_type_validate
 * Purpose:
 *      Helper function to validate multicast type support 
 * Parameters:
 *      unit       - (IN)   Device Number
 *      type       - (IN)   BCM_MULTICAST_TYPE_*
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_multicast_type_validate(int unit, uint32 type)
{
    int rv; 

    /* Only single multicast type is allowed per API call */
    /* therefore number of bits on indicating Multicast type must equal to 1 */
    if (1 != _shr_popcount(type)) {
        return BCM_E_PARAM;
    }

    rv = BCM_E_PARAM; /* return error by default */

    switch (type) {
        case BCM_MULTICAST_TYPE_L2: 
            rv = BCM_E_NONE;
            break;
        case BCM_MULTICAST_TYPE_L3: 
            if (soc_feature(unit, soc_feature_ip_mcast_repl)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_VPLS:
            if (soc_feature(unit, soc_feature_mpls)) {
                rv = BCM_E_NONE;
            } else {
                rv = BCM_E_UNAVAIL;
            }
            break;
        case BCM_MULTICAST_TYPE_SUBPORT:
            if (soc_feature(unit, soc_feature_subport)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_MIM:
            if (soc_feature(unit, soc_feature_mim)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_WLAN:
            if (soc_feature(unit, soc_feature_wlan)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_VLAN:
            if (soc_feature(unit, soc_feature_vlan_vp)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_TRILL:
            if (soc_feature(unit, soc_feature_trill)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_NIV:
            if (soc_feature(unit, soc_feature_niv)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_EGRESS_OBJECT:
            if (soc_feature(unit, soc_feature_mpls)) {
                rv = BCM_E_NONE;
            }
            break;
        case BCM_MULTICAST_TYPE_L2GRE:
            if (soc_feature(unit, soc_feature_l2gre)) {
                rv = BCM_E_NONE;
            } else {
                rv = BCM_E_UNAVAIL;
            }
            break;
        case BCM_MULTICAST_TYPE_VXLAN:
            if (soc_feature(unit, soc_feature_vxlan)) {
                rv = BCM_E_NONE;
            } else {
                rv = BCM_E_UNAVAIL;
            }
            break;
        case BCM_MULTICAST_TYPE_EXTENDER:
            if (soc_feature(unit, soc_feature_port_extension)) {
                rv = BCM_E_NONE;
            }
            break;

        default:
            rv = BCM_E_PARAM;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_multicast_create
 * Purpose:
 *      Helper function to allocate a multicast group index
 * Parameters:
 *      unit       - (IN)   Device Number
 *      flags      - (IN)   BCM_MULTICAST_*
 *      group      - (OUT)  Group ID
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_multicast_create(int unit, uint32 flags, bcm_multicast_t *group)
{
    uint32 type;

    type = flags & BCM_MULTICAST_TYPE_MASK;

    BCM_IF_ERROR_RETURN(
        _bcm_esw_multicast_type_validate(unit, type));

    if (type == BCM_MULTICAST_TYPE_L2) {
        /* Create L2 multicast group */
        return _bcm_esw_multicast_l2_create(unit, flags, group);
    } else {
#if defined(INCLUDE_L3)
        /* Create L3/Virtual multicast group */
        return _bcm_esw_multicast_l3_create(unit, flags, group);
#endif /* INCLUDE_L3 */
    }

    return BCM_E_UNAVAIL;
}


/* Macro to free memory and return code instead of using goto */

#define BCM_MULTICAST_MEM_FREE_RETURN(_r_)              \
    if (local_trunk_port_array) {                       \
        for (j = 0; j < port_count; j++) {       \
            if (local_trunk_port_array[j]) {            \
                sal_free(local_trunk_port_array[j]);    \
            }                                           \
        }                                               \
        sal_free(local_trunk_port_array);               \
    }                                                   \
    if (local_port_array) {                             \
        sal_free(local_port_array);                     \
    }                                                   \
    if (trunk_local_ports) {                            \
        sal_free(trunk_local_ports);                    \
    }                                                   \
    if (temp_encap_list) {                              \
        sal_free(temp_encap_list);                      \
        temp_encap_list = NULL;                         \
    }                                                   \
    return _r_;


#if defined(BCM_XGS3_SWITCH_SUPPORT) && defined(INCLUDE_L3)
/*
 * Function:
 *      _bcm_esw_multicast_egress_set
 * Purpose:
 *      Helper function to assign the complete set of egress GPORTs in the
 *      replication list for the specified multicast index.
 * Parameters:
 *      unit       - (IN) Device Number
 *      group      - (IN) Multicast group ID
 *      port_count   - (IN) Number of ports in replication list
 *      port_array   - (IN) List of GPORT Identifiers
 *      encap_id_array - (IN) List of encap identifiers
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_multicast_egress_set(int unit, bcm_multicast_t group, int port_count,
                            bcm_gport_t *port_array, bcm_if_t *encap_id_array)
{
    int mc_index, i, j, rv = BCM_E_NONE;
    int gport_id, *temp_encap_list = NULL;
    bcm_port_t *local_port_array = NULL, **local_trunk_port_array = NULL;
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    int max_num_ports = SOC_MAX_NUM_PP_PORTS;
#else
    int max_num_ports = SOC_MAX_NUM_PORTS;
#endif
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_pbmp_t l2_pbmp, l3_pbmp;
    l2mc_entry_t l2mc_entry; 
    bcm_trunk_t trunk_id;
    soc_mem_t   mem;
    int idx;
    int modid_local = 0;
    int   *trunk_local_ports = NULL;
#if defined (INCLUDE_L3)
    int list_count, is_l3 = 0, is_vpls = 0;
    bcm_pbmp_t mc_eligible_pbmp, e_pbmp;
    bcm_port_t port_iter;
    int trunk_local_found;
#endif /* INCLUDE_L3 */
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif

    /* Input parameters check. */
    mc_index = _BCM_MULTICAST_ID_GET(group);
    mem = (_BCM_MULTICAST_L2_OR_FABRIC(unit, group)) ? L2MCm : L3_IPMCm; 
    /* Table index sanity check. */
#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_fabric_multicast_param_check(unit, group, &mc_index));
    } else
#endif /* BCM_XGS3_FABRIC_SUPPORT */
    if ((mc_index < 0) || (mc_index >= soc_mem_index_count(unit, mem))) {
        return (BCM_E_PARAM);
    }

    if (port_count > 0) {
        if (NULL == port_array) {
            return (BCM_E_PARAM);
        }
        if (!_BCM_MULTICAST_L2_OR_FABRIC(unit, group) && 
            (NULL == encap_id_array)) {
            return (BCM_E_PARAM);
        }
    } else if (port_count < 0) {
        return (BCM_E_PARAM);
    }   

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        /* The group was already validated above */
    } else
#endif /* BCM_XGS3_FABRIC_SUPPORT */
    if (!SOC_IS_TRX(unit)) {
        if ((0 == _BCM_MULTICAST_IS_L2(group)) && 
            (0 == _BCM_MULTICAST_IS_L3(group))) {
            return (BCM_E_PARAM);
        }
    } else {
#if defined (INCLUDE_L3)
        rv = _bcm_esw_multicast_l3_group_check(unit, group, NULL);
#endif /* INCLUDE_L3 */
        if (BCM_FAILURE(rv) && (0 == _BCM_MULTICAST_IS_L2(group))) {
            return BCM_E_PARAM;
        }
    }

    /* Convert GPORT array into local port numbers */
    if (port_count > 0) {
        local_port_array =
            sal_alloc(sizeof(bcm_port_t) * port_count, "local_port array");
        if (NULL == local_port_array) {
            BCM_MULTICAST_MEM_FREE_RETURN(BCM_E_MEMORY);
        }
        
        temp_encap_list =
            sal_alloc(sizeof(int) * port_count, "temp_encap_list");
        if (NULL == temp_encap_list) {
            BCM_MULTICAST_MEM_FREE_RETURN(rv);
        }

        trunk_local_ports =
            sal_alloc(sizeof(int) * port_count, "trunk_local_ports");
        if (NULL == trunk_local_ports) {
            BCM_MULTICAST_MEM_FREE_RETURN(BCM_E_MEMORY);
        }

        local_trunk_port_array =
            sal_alloc(sizeof(bcm_port_t *) * port_count,
                      "local_trunk_port_array pointers");
        if (NULL == local_trunk_port_array) {
            BCM_MULTICAST_MEM_FREE_RETURN(BCM_E_MEMORY);
        }

        for (i = 0; i < port_count; i++) {
            local_trunk_port_array[i] =
                sal_alloc(sizeof(bcm_port_t) * max_num_ports,
                          "local_trunk_port_array");
            if (NULL == local_trunk_port_array[i]) {
                BCM_MULTICAST_MEM_FREE_RETURN(BCM_E_MEMORY);
            }
        }

        for (i = 0; i < port_count; i++) {
            trunk_local_ports[i] = 0;
            for (idx = 0; idx < max_num_ports; idx++) {
                local_trunk_port_array[i][idx] = 0;
            }
        }
    }

    for (i = 0; i < port_count ; i++) {
        rv = _bcm_esw_gport_resolve(unit, port_array[i],
                                    &mod_out, &port_out,
                                    &trunk_id, &gport_id); 
        if (BCM_FAILURE(rv)) {
            BCM_MULTICAST_MEM_FREE_RETURN(rv);
        }

#if defined(BCM_KATANA2_SUPPORT)
        is_local_subport = FALSE;
        if (soc_feature(unit, soc_feature_linkphy_coe) ||
            soc_feature(unit, soc_feature_subtag_coe)) {
            rv = bcm_esw_stk_my_modid_get(unit, &my_modid);
            if (BCM_FAILURE(rv)) {
                BCM_MULTICAST_MEM_FREE_RETURN(rv);
            }
            if (BCM_GPORT_IS_SET(port_array[i]) &&
                _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                    port_array[i])) {
                if (mod_out == (my_modid + 1)) {
                    pp_port = BCM_GPORT_SUBPORT_PORT_GET(port_array[i]);
                    if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                        (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                        is_local_subport = TRUE;
                    }
                }
                if (is_local_subport == FALSE) {
                    BCM_MULTICAST_MEM_FREE_RETURN(BCM_E_PORT);
                }
            }
        }
        if (is_local_subport) {
            local_port_array[i] = pp_port;
        } else
#endif

        if (BCM_TRUNK_INVALID != trunk_id) {
            rv = _bcm_esw_trunk_local_members_get(unit, trunk_id,
                    max_num_ports, local_trunk_port_array[i],
                    &trunk_local_ports[i]);
            if (BCM_FAILURE(rv)) {
                BCM_MULTICAST_MEM_FREE_RETURN(rv);
            }
            /* Record the trunk ID to use later */
            BCM_GPORT_TRUNK_SET(local_port_array[i], trunk_id);
        } else {
            rv = _bcm_esw_modid_is_local(unit, mod_out, &modid_local);
            if (BCM_FAILURE(rv)) {
                BCM_MULTICAST_MEM_FREE_RETURN(rv);
            }

            /* Convert system local_port to physical local_port */ 
            if (soc_feature(unit, soc_feature_sysport_remap)) { 
                BCM_XLATE_SYSPORT_S2P(unit, &port_out); 
            }

            if (TRUE != modid_local) {
                BCM_MULTICAST_MEM_FREE_RETURN(BCM_E_PARAM);
            }
            local_port_array[i] = port_out;
        }
    }

    BCM_PBMP_CLEAR(l2_pbmp);
    BCM_PBMP_CLEAR(l3_pbmp);
    if (_BCM_MULTICAST_L2_OR_FABRIC(unit, group)) {
        for (i = 0; i < port_count; i++) {
            if (BCM_GPORT_IS_TRUNK(local_port_array[i])) {
                for (idx = 0; idx < trunk_local_ports[i]; idx++) {
                    BCM_PBMP_PORT_ADD(l2_pbmp,
                                      local_trunk_port_array[i][idx]);
                }
            } else {
                BCM_PBMP_PORT_ADD(l2_pbmp, local_port_array[i]);
            }
        }

        /* Update the L2MC port bitmap */
        soc_mem_lock(unit, L2MCm);
        rv = soc_mem_read(unit, L2MCm, MEM_BLOCK_ANY, mc_index, &l2mc_entry);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, L2MCm);
            BCM_MULTICAST_MEM_FREE_RETURN(rv);
        }
        soc_mem_pbmp_field_set(unit, L2MCm, &l2mc_entry, PORT_BITMAPf,
                               &l2_pbmp);
        rv = soc_mem_write(unit, L2MCm, MEM_BLOCK_ALL,
                           mc_index, &l2mc_entry);
        soc_mem_unlock(unit, L2MCm);
#if defined(INCLUDE_L3)
    } else {
        if (_BCM_MULTICAST_IS_L3(group)) {
            is_l3 = 1;
        }
        if (0 == port_count) {
            /* Clearing the replication set */
            list_count = 0;
            BCM_PBMP_CLEAR(e_pbmp);
            BCM_PBMP_ASSIGN(e_pbmp, PBMP_E_ALL(unit));
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TRIDENT2(unit)) {
                BCM_PBMP_PORT_ADD(e_pbmp,CMIC_PORT(unit));	
            }
#endif	    
#if defined(BCM_KATANA2_SUPPORT)
            if (soc_feature(unit, soc_feature_subtag_coe) ||
                soc_feature(unit, soc_feature_linkphy_coe)) {
               _bcm_kt2_subport_pbmp_update(unit, &e_pbmp);
           }
#endif

            BCM_PBMP_ITER(e_pbmp, port_iter) {
                rv = _bcm_esw_ipmc_egress_intf_set(unit, mc_index, port_iter,
                                        list_count, NULL, is_l3, FALSE);
                if (BCM_FAILURE(rv)) {
                    BCM_MULTICAST_MEM_FREE_RETURN(rv);
                }
            }
        } else {
            if (_BCM_MULTICAST_IS_VPLS(group)) {
                is_vpls = 1;
            }

            /* 
             * For each port, walk through the list of GPORTs
             * and collect the ones that match the port.
             */

            /* Valid SOC ports plus the CMIC port */
            BCM_PBMP_ASSIGN(mc_eligible_pbmp, PBMP_PORT_ALL(unit));
            BCM_PBMP_PORT_ADD(mc_eligible_pbmp, CMIC_PORT(unit));
#if defined(BCM_KATANA2_SUPPORT)
            if (soc_feature(unit, soc_feature_subtag_coe) ||
                soc_feature(unit, soc_feature_linkphy_coe)) {
               _bcm_kt2_subport_pbmp_update(unit, &mc_eligible_pbmp);
           }
#endif
            BCM_PBMP_ITER(mc_eligible_pbmp, port_iter) {
                list_count = 0;
                for (i = 0; i < port_count ; i++) {
                    if (BCM_GPORT_IS_TRUNK(local_port_array[i])) {
                        trunk_local_found = 0;
                        for (idx = 0; idx < trunk_local_ports[i]; idx++) {
                            if (local_trunk_port_array[i][idx] == port_iter) {
                                trunk_local_found = 1;
                                break;
                            }
                        }
                        if (!trunk_local_found) {
                            continue;
                        }   
                    } else {
                        if (local_port_array[i] != port_iter) {
                            continue;
                        }
                    }
                    if (encap_id_array[i] == BCM_IF_INVALID) {
                        if (!is_vpls) {
                            /* Add port to L2 pbmp */
                            BCM_PBMP_PORT_ADD(l2_pbmp, port_iter);
                        } else {
                            rv = BCM_E_PARAM;
                            BCM_MULTICAST_MEM_FREE_RETURN(rv);
                        }
                    } else {
                        if (soc_feature(unit, 
                                soc_feature_l3mc_use_egress_next_hop)) {
                            /* _bcm_esw_ipmc_egress_intf_set() processing the 
                             * real HW index to indicate egress NH object.
                             */
                            temp_encap_list[list_count] = encap_id_array[i] - 
                                     BCM_XGS3_EGRESS_IDX_MIN;
                        } else {
                            temp_encap_list[list_count] = encap_id_array[i];
                        }
                        list_count++;
                    }
                }
                if ((0 == list_count) && (CMIC_PORT(unit) == port_iter)) {
                    /* Don't install L3 ports on CMIC */
                    continue;
                }
                rv = _bcm_esw_ipmc_egress_intf_set(unit, mc_index, port_iter,
                                                   list_count, temp_encap_list, is_l3, TRUE);
                if (BCM_FAILURE(rv)) {
                    BCM_MULTICAST_MEM_FREE_RETURN(rv);
                }
                if (list_count) {
                    BCM_PBMP_PORT_ADD(l3_pbmp, port_iter);
                }
            }
        }
        rv = _bcm_esw_multicast_ipmc_write(unit, mc_index, l2_pbmp,
                                           l3_pbmp, TRUE);
#endif  /* INCLUDE_L3 */
    }

    BCM_MULTICAST_MEM_FREE_RETURN(rv);
}
#endif


#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef	BCM_XGS3_FABRIC_SUPPORT
/*
 * Function:
 *      _bcm_esw_fabric_multicast_init
 * Purpose:
 *      Initialize the multicast module on XGS3 fabric devices.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
STATIC int 
_bcm_esw_fabric_multicast_init(int unit)
{
    int	mc_base, mc_size;
    int	ipmc_base, ipmc_size, alloc_size;
    int l2mc_table_size = soc_mem_index_count(unit, L2MCm);

    SOC_IF_ERROR_RETURN
        (soc_hbx_mcast_size_get(unit, &mc_base, &mc_size));
    SOC_IF_ERROR_RETURN
        (soc_hbx_ipmc_size_get(unit, &ipmc_base, &ipmc_size));

    _bcm_fabric_mc_info[unit].mcast_size = mc_size;
    _bcm_fabric_mc_info[unit].ipmc_size = ipmc_size;

    if (NULL != _bcm_fabric_mc_info[unit].mcast_used) {
        sal_free(_bcm_fabric_mc_info[unit].mcast_used);
        _bcm_fabric_mc_info[unit].mcast_used = NULL;
    }
    alloc_size = SHR_BITALLOCSIZE(l2mc_table_size);
    _bcm_fabric_mc_info[unit].mcast_used = 
        sal_alloc(alloc_size, "Fabric MC entries used");
    if (NULL == _bcm_fabric_mc_info[unit].mcast_used) {
        return BCM_E_MEMORY;
    }
    sal_memset(_bcm_fabric_mc_info[unit].mcast_used, 0, alloc_size);
    
#ifdef BCM_WARM_BOOT_SUPPORT
    multicast_mode_set[unit] = TRUE;

    /* Recover the used multicast entries */
    if (SOC_WARM_BOOT(unit)) {
        l2mc_entry_t *l2mc_entry, *l2mc_table;
        int index, index_min, index_max, l2mc_tbl_sz;

        /*
         * Go through L2MC table to find valid entries
         */
        index_min = soc_mem_index_min(unit, L2MCm);
        index_max = soc_mem_index_max(unit, L2MCm);

        l2mc_tbl_sz = sizeof(l2mc_entry_t) * l2mc_table_size;
        l2mc_table = soc_cm_salloc(unit, l2mc_tbl_sz, "l2mc tbl dma");
        if (l2mc_table == NULL) {
            sal_free(BCM_FABRIC_MC_USED_BITMAP(unit));
            return BCM_E_MEMORY;
        }

        memset((void *)l2mc_table, 0, l2mc_tbl_sz);
        if (soc_mem_read_range(unit, L2MCm, MEM_BLOCK_ANY,
                               index_min, index_max, l2mc_table) < 0) {
            sal_free(BCM_FABRIC_MC_USED_BITMAP(unit));
            soc_cm_sfree(unit, l2mc_table);
            return SOC_E_INTERNAL;
        }

        for (index = index_min; index <= index_max; index++) {

            l2mc_entry = soc_mem_table_idx_to_pointer(unit, L2MCm,
                                                      l2mc_entry_t *,
                                                      l2mc_table, index);

            if (!soc_mem_field32_get(unit, L2MCm, l2mc_entry, VALIDf)) {
                continue;
            }

            SHR_BITSET(BCM_FABRIC_MC_USED_BITMAP(unit), index);
        }

        soc_cm_sfree(unit, l2mc_table);
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_fabric_multicast_detach
 * Purpose:
 *      Detach the multicast module on XGS3 fabric devices.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
STATIC int
_bcm_esw_fabric_multicast_detach(int unit)
{
    _bcm_fabric_mc_info[unit].mcast_size = 0;
    _bcm_fabric_mc_info[unit].ipmc_size = 0;

    if (NULL != _bcm_fabric_mc_info[unit].mcast_used) {
        sal_free(_bcm_fabric_mc_info[unit].mcast_used);
        _bcm_fabric_mc_info[unit].mcast_used = NULL;
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_fabric_multicast_create
 * Purpose:
 *      Create a multicast group on XGS3 fabric devices.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
STATIC int 
_bcm_esw_fabric_multicast_create(int unit, uint32 flags, bcm_multicast_t *group)
{
    uint32 type;
    int search_index, fabric_index = -1;
    int index_min, index_max, rv;
    l2mc_entry_t	l2mc;

    /* On switch devices, we verify that the particular multicast group
     * type is valid and supported on the device.
     * We Must Not Do This on Fabrics!
     * Fabric devices must be able to pass all group types.  We translate
     * the type here to what is appropriate on the specific device HW.
     */
    type = flags & BCM_MULTICAST_TYPE_MASK;
    if (1 != _shr_popcount(type)) {
        return BCM_E_PARAM;
    }

    if (type == BCM_MULTICAST_TYPE_L2) {
        index_min = BCM_FABRIC_MCAST_BASE(unit);
        index_max =  BCM_FABRIC_MCAST_MAX(unit);
        if (flags & BCM_MULTICAST_WITH_ID) {
            if (!_BCM_MULTICAST_IS_L2(*group)) {
                /* Group ID doesn't match creation flag type */
                return BCM_E_PARAM;
            }
        }
    } else {
        index_min = BCM_FABRIC_IPMC_BASE(unit);
        index_max = BCM_FABRIC_IPMC_MAX(unit);
        if (flags & BCM_MULTICAST_WITH_ID) {
            if (_BCM_MULTICAST_IS_L2(*group)) {
                /* Group ID doesn't match creation flag type */
                return BCM_E_PARAM;
            }
        }
    }

    if (flags & BCM_MULTICAST_WITH_ID) {
        fabric_index = _BCM_MULTICAST_ID_GET(*group) + index_min;
        if ((fabric_index < index_min) || (fabric_index > index_max)) {
            /* Group index out of bounds */
            return BCM_E_PARAM;
        }
        if (SHR_BITGET(BCM_FABRIC_MC_USED_BITMAP(unit),
                       fabric_index)) {
            /* Requested group has already been created */
            return BCM_E_EXISTS;
        }
    } else {
        for (search_index = index_min; search_index <= index_max;
             search_index++) {
            if (!SHR_BITGET(BCM_FABRIC_MC_USED_BITMAP(unit),
                            search_index)) {
                fabric_index = search_index;
                break;
            }
        }
        if (-1 == fabric_index) {
            /* Out of free groups */
            return BCM_E_RESOURCE;
        }
    }

    /* Mark HW entry as valid */
    sal_memset(&l2mc, 0, sizeof(l2mc));
    soc_mem_field32_set(unit, L2MCm, &l2mc, VALIDf, 1);
    soc_mem_lock(unit, L2MCm);
    rv = WRITE_L2MCm(unit, MEM_BLOCK_ALL, fabric_index, &l2mc);
    soc_mem_unlock(unit, L2MCm);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    /* Record group allocation */
    SHR_BITSET(BCM_FABRIC_MC_USED_BITMAP(unit), fabric_index);

    _BCM_MULTICAST_GROUP_SET(*group,
                             (type == BCM_MULTICAST_TYPE_L2) ?
                             _BCM_MULTICAST_TYPE_L2 : _BCM_MULTICAST_TYPE_L3,
                             fabric_index - index_min);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_fabric_multicast_destroy
 * Purpose:
 *      Destroy a multicast group on XGS3 fabric devices.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
STATIC int 
_bcm_esw_fabric_multicast_destroy(int unit, bcm_multicast_t group)
{
    int fabric_index = -1;
    int index_min, index_max, rv;
    l2mc_entry_t	l2mc;

    if (_BCM_MULTICAST_IS_L2(group)) {
        index_min = BCM_FABRIC_MCAST_BASE(unit);
        index_max =  BCM_FABRIC_MCAST_MAX(unit);
    } else {
        /* All non-L2 types are the same on fabric devices */
        index_min = BCM_FABRIC_IPMC_BASE(unit);
        index_max = BCM_FABRIC_IPMC_MAX(unit);
    }
    fabric_index = _BCM_MULTICAST_ID_GET(group) + index_min;

    if ((fabric_index < index_min) || (fabric_index > index_max)) {
        /* Group index out of bounds */
        return BCM_E_PARAM;
    }
    if (!SHR_BITGET(BCM_FABRIC_MC_USED_BITMAP(unit), fabric_index)) {
        /* Requested group has not been created */
        return BCM_E_NOT_FOUND;
    }

    /* Mark HW entry as invalid */
    sal_memset(&l2mc, 0, sizeof(l2mc));
    soc_mem_lock(unit, L2MCm);
    rv = WRITE_L2MCm(unit, MEM_BLOCK_ALL, fabric_index, &l2mc);
    soc_mem_unlock(unit, L2MCm);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    /* Clear group allocation */
    SHR_BITCLR(BCM_FABRIC_MC_USED_BITMAP(unit), fabric_index);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_fabric_multicast_group_get
 * Purpose:
 *      Retrieve the flags associated with a multicast group.
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) Multicast group ID
 *      flags - (OUT) BCM_MULTICAST_*
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
STATIC int 
_bcm_esw_fabric_multicast_group_get(int unit, bcm_multicast_t group,
                                    uint32 *flags)
{
    int mc_index;
    uint32 type_flags;

    /* Check if this is a known group */
    mc_index = _BCM_MULTICAST_ID_GET(group);
    BCM_IF_ERROR_RETURN
        (_bcm_esw_fabric_multicast_param_check(unit, group, &mc_index));

    /* Since it's known, it's either L2 or L3 */
    if ((mc_index < BCM_FABRIC_MCAST_BASE(unit)) ||
        (mc_index > BCM_FABRIC_MCAST_MAX(unit))) {
        type_flags = BCM_MULTICAST_TYPE_L2;
    } else {
        type_flags = BCM_MULTICAST_TYPE_L3;
    }

    *flags = (type_flags | BCM_MULTICAST_WITH_ID);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_fabric_multicast_group_traverse
 * Purpose:
 *      Iterate over the defined multicast groups of the type
 *      specified in 'flags'.  If all types are desired, use
 *      MULTICAST_TYPE_MASK.
 * Parameters:
 *      unit - (IN) Unit number.
 *      trav_fn - (IN) Callback function.
 *      flags - (IN) BCM_MULTICAST_*
 *      user_data - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
_bcm_esw_fabric_multicast_group_traverse(int unit,
                                 bcm_multicast_group_traverse_cb_t trav_fn, 
                                         uint32 flags, void *user_data)
{
    int search_index;
    int index_min, index_max;
    uint32 group_flags;
    bcm_multicast_t group;

    /* L2 groups */
    index_min = BCM_FABRIC_MCAST_BASE(unit);
    index_max =  BCM_FABRIC_MCAST_MAX(unit);
    group_flags = BCM_MULTICAST_TYPE_L2 | BCM_MULTICAST_WITH_ID;

    for (search_index = index_min; search_index <= index_max;
         search_index++) {
        if (SHR_BITGET(BCM_FABRIC_MC_USED_BITMAP(unit), search_index)) {
            _BCM_MULTICAST_GROUP_SET(group, _BCM_MULTICAST_TYPE_L2,
                                     search_index);
            BCM_IF_ERROR_RETURN
                ((*trav_fn)(unit, group, group_flags, user_data));
        }
    }
        
    /* L3 groups */
    index_min = BCM_FABRIC_IPMC_BASE(unit);
    index_max = BCM_FABRIC_IPMC_MAX(unit);
    group_flags = BCM_MULTICAST_TYPE_L3 | BCM_MULTICAST_WITH_ID;

    for (search_index = index_min; search_index <= index_max;
         search_index++) {
        if (SHR_BITGET(BCM_FABRIC_MC_USED_BITMAP(unit), search_index)) {
            _BCM_MULTICAST_GROUP_SET(group, _BCM_MULTICAST_TYPE_L3,
                                     search_index - index_min);
            BCM_IF_ERROR_RETURN
                ((*trav_fn)(unit, group, group_flags, user_data));
        }
    }

    return BCM_E_NONE;
}
#endif	/* BCM_XGS3_FABRIC_SUPPORT */

/*
 * Function:
 *      bcm_esw_multicast_init
 * Purpose:
 *      Initialize the multicast module.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_multicast_init(int unit)
{
#ifdef BCM_WARM_BOOT_SUPPORT
    multicast_mode_set[unit] = FALSE;
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        /* Fabrics must track the multicast groups here, since they
         * are not recorded in the mcast or IPMC modules. */
        BCM_IF_ERROR_RETURN(_bcm_esw_fabric_multicast_init(unit));
        multicast_initialized[unit] = TRUE;
        return BCM_E_NONE;
    }
#endif /* BCM_XGS3_FABRIC_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        multicast_initialized[unit] = TRUE;
        return BCM_E_NONE;
    }
#endif /*BCM_SHADOW_SUPPORT*/

#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TRX(unit)) {
        BCM_IF_ERROR_RETURN(bcm_trx_multicast_init(unit));
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
    if (SOC_REG_IS_VALID(unit, IPMC_L3_MTUr)) {
        soc_reg_t reg;
        uint64 rval64, *rval64s[1];
        uint32 pindex;
        /* Create profile for IPMC_L3_MTU register */
        if (_bcm_mtu_profile[unit] == NULL) {
            _bcm_mtu_profile[unit] =
                sal_alloc(sizeof(soc_profile_reg_t), "MTU size Profile Reg");
            if (_bcm_mtu_profile[unit] == NULL) {
                return BCM_E_MEMORY;
            }
            soc_profile_reg_t_init(_bcm_mtu_profile[unit]);
            reg = IPMC_L3_MTUr;
            BCM_IF_ERROR_RETURN
                (soc_profile_reg_create(unit, &reg, 1, _bcm_mtu_profile[unit]));
            /* Add a default entry for backward comaptability */
            COMPILER_64_ZERO(rval64);
            COMPILER_64_SET(rval64, 0, 0x3fff);
            rval64s[0] = &rval64;
            BCM_IF_ERROR_RETURN
                (soc_profile_reg_add(unit, _bcm_mtu_profile[unit], rval64s,
                                     1, &pindex)); 
        }
    }
#endif

    multicast_initialized[unit] = TRUE;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_esw_multicast_detach
 * Purpose:
 *      Shut down (uninitialize) the multicast module.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_multicast_detach(int unit)
{
#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        /* Fabrics must track the multicast groups here, since they
         * are not recorded in the mcast or IPMC modules. */
        BCM_IF_ERROR_RETURN(_bcm_esw_fabric_multicast_detach(unit));
        multicast_initialized[unit] = FALSE;
        return BCM_E_NONE;
    }
#endif /* BCM_XGS3_FABRIC_SUPPORT */

#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TRX(unit)) {
        BCM_IF_ERROR_RETURN(bcm_trx_multicast_detach(unit));
        multicast_initialized[unit] = FALSE;
        return BCM_E_NONE;
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */

    multicast_initialized[unit] = FALSE;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_multicast_create
 * Purpose:
 *      Allocate a multicast group index
 * Parameters:
 *      unit       - (IN)   Device Number
 *      flags      - (IN)   BCM_MULTICAST_*
 *      group      - (OUT)  Group ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_create(int unit, uint32 flags, bcm_multicast_t *group)
{
    int rv = BCM_E_UNAVAIL;

    MULTICAST_INIT_CHECK(unit);

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        return _bcm_esw_fabric_multicast_create(unit, flags, group);
    }
#endif /* BCM_XGS3_FABRIC_SUPPORT */

#if defined(BCM_XGS3_SWITCH_SUPPORT) 
    if (SOC_IS_XGS3_SWITCH(unit)) {
        rv = _bcm_esw_multicast_create(unit, flags, group);

#if defined(BCM_WARM_BOOT_SUPPORT) && defined(INCLUDE_L3)
        if (BCM_SUCCESS(rv) && !multicast_mode_set[unit]) {
            /* Notify IPMC module that we're operating in multicast mode
             * for Warm Boot purposes */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_ipmc_repl_wb_flags_set(unit,
                                                 _BCM_IPMC_WB_MULTICAST_MODE,
                                                 _BCM_IPMC_WB_MULTICAST_MODE));
            multicast_mode_set[unit] = TRUE;
        }
#endif /* BCM_WARM_BOOT_SUPPORT && INCLUDE_L3 */
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_multicast_destroy
 * Purpose:
 *      Free a multicast group index
 * Parameters:
 *      unit       - (IN) Device Number
 *      group      - (IN) Group ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_destroy(int unit, bcm_multicast_t group)
{
    MULTICAST_INIT_CHECK(unit);

    if (!_BCM_MULTICAST_IS_SET(group)) {
        return BCM_E_PARAM;
    }

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        return _bcm_esw_fabric_multicast_destroy(unit, group);
    }
#endif /* BCM_XGS3_FABRIC_SUPPORT */

#if defined(BCM_XGS3_SWITCH_SUPPORT) 
    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (_BCM_MULTICAST_IS_L2(group)) {
            return _bcm_esw_multicast_l2_destroy(unit, group);
        } else {
#if defined(INCLUDE_L3)
            return _bcm_esw_multicast_l3_destroy(unit, group);
#endif
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_l3_encap_get
 * Purpose:
 *      Get the Encap ID for L3.
 * Parameters:
 *      unit  - (IN) Unit number.
 *      group - (IN) Multicast group ID.
 *      port  - (IN) Physical port.
 *      intf  - (IN) L3 interface ID.
 *      encap_id - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_multicast_l3_encap_get(int unit, bcm_multicast_t group,
                               bcm_gport_t port, bcm_if_t intf,
                               bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        /* Encap ID is not used for fabric devices in XGS3 */
        *encap_id = BCM_IF_INVALID;
        return BCM_E_NONE;
    }
#endif	/* BCM_XGS3_FABRIC_SUPPORT */

#if defined(BCM_XGS3_SWITCH_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (BCM_GPORT_IS_VLAN_PORT(port) ||
            BCM_GPORT_IS_NIV_PORT(port) ||
            BCM_GPORT_IS_EXTENDER_PORT(port)) {
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_virtual_port_routing)) {
                return bcm_td2_multicast_l3_vp_encap_get(unit, group, port,
                        intf, encap_id);
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                return BCM_E_UNAVAIL;
            }
        }
        
#ifdef BCM_GREYHOUND_SUPPORT
        if (soc_feature(unit, soc_feature_l3mc_use_egress_next_hop)) {
            /* intf for this feature must be egress_nh index */
            if (!BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf)){
                return BCM_E_PARAM;
            }
            return bcm_esw_multicast_egress_object_encap_get(unit, 
                    group, intf, encap_id);
        }
#endif /* BCM_GREYHOUND_SUPPORT */

        /* For L3 IPMC, encap_id is simply the L3 interface ID */
        *encap_id = intf;
        return BCM_E_NONE;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL; 
}

/*
 * Function:
 *      bcm_multicast_l2_encap_get
 * Purpose:
 *      Get the Encap ID for L2.
 * Parameters:
 *      unit     - (IN) Unit number.
 *      group    - (IN) Multicast group ID.
 *      port     - (IN) Physical port.
 *      vlan     - (IN) Vlan.
 *      encap_id - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_multicast_l2_encap_get(int unit, bcm_multicast_t group,
                               bcm_gport_t port, bcm_vlan_t vlan,
                               bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_XGS3_SWITCH_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        /* Encap ID is not used for L2 in XGS3 */
        *encap_id = BCM_IF_INVALID;
        return BCM_E_NONE;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL; 
}

/*
 * Function:
 *      bcm_multicast_trill_encap_get
 * Purpose:
 *      Get the Encap ID for a TRILL port.
 * Parameters:
 *      unit         - (IN) Unit number.
 *      group        - (IN) Multicast group ID.
 *      port         - (IN) Physical port.
 *      intf          - (IN) L3 interface ID.
 *      encap_id     - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_trill_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                 bcm_if_t intf, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_trill)) { 
        /* For TRILL IPMC, encap_id is simply the L3 interface ID */
        *encap_id = intf;
        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_vpls_encap_get
 * Purpose:
 *      Get the Encap ID for a MPLS port.
 * Parameters:
 *      unit         - (IN) Unit number.
 *      group        - (IN) Multicast group ID.
 *      port         - (IN) Physical port.
 *      mpls_port_id - (IN) MPLS port ID.
 *      encap_id     - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_vpls_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                 bcm_gport_t mpls_port_id, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH2(unit) ||
            SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit) ||
            SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
        return bcm_tr2_multicast_vpls_encap_get(unit, group, port, 
                                               mpls_port_id, encap_id);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TR_VL(unit) && !SOC_IS_HURRICANEX(unit)) {
        return bcm_tr_multicast_vpls_encap_get(unit, group, port, 
                                               mpls_port_id, encap_id);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_subport_encap_get
 * Purpose:
 *      Get the Encap ID for a subport.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      group     - (IN) Multicast group ID.
 *      port      - (IN) Physical port.
 *      subport   - (IN) Subport ID.
 *      encap_id  - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_subport_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                    bcm_gport_t subport, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit) ||
        SOC_IS_TRIUMPH3(unit)) {
        return bcm_tr2_multicast_subport_encap_get(unit, group, port, 
                                                  subport, encap_id);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit) &&
        !SOC_IS_XGS3_FABRIC(unit)) {
        return bcm_tr_multicast_subport_encap_get(unit, group, port, 
                                                  subport, encap_id);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_mim_encap_get
 * Purpose:
 *      Get the Encap ID for MiM.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      group     - (IN) Multicast group ID.
 *      port      - (IN) Physical port.
 *      mim_port  - (IN) MiM port ID.
 *      encap_id  - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_mim_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                bcm_gport_t mim_port, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_mim)) {
        return bcm_tr2_multicast_mim_encap_get(unit, group, port, 
                                              mim_port, encap_id);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_wlan_encap_get
 * Purpose:
 *      Get the Encap ID for WLAN.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      group     - (IN) Multicast group ID.
 *      port      - (IN) Physical port.
 *      wlan_port - (IN) WLAN port ID.
 *      encap_id  - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_wlan_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                 bcm_gport_t wlan_port, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_wlan)) {
        return bcm_tr2_multicast_wlan_encap_get(unit, group, port, 
                                               wlan_port, encap_id);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_vlan_encap_get
 * Purpose:
 *      Get the Encap ID for VLAN virtual port.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      group     - (IN) Multicast group ID.
 *      port      - (IN) Physical port.
 *      vlan_port - (IN) VLAN port ID.
 *      encap_id  - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_vlan_encap_get(int unit, bcm_multicast_t group,
                                 bcm_gport_t port, bcm_gport_t vlan_port_id,
                                 bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_vlan_vp)) {
        int vp;
        ing_dvp_table_entry_t dvp;

        if (!BCM_GPORT_IS_VLAN_PORT(vlan_port_id)) {
            return BCM_E_PARAM;
        }
        vp = BCM_GPORT_VLAN_PORT_ID_GET(vlan_port_id); 
        if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

        /* Next-hop index is used for multicast replication */
        *encap_id = (bcm_if_t) soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        if (!SOC_IS_ENDURO(unit)) {
            *encap_id += BCM_XGS3_DVP_EGRESS_IDX_MIN;
        }
        return BCM_E_NONE;
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_niv_encap_get
 * Purpose:
 *      Get the Encap ID for NIV virtual port.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      group       - (IN) Multicast group ID.
 *      port        - (IN) Physical port.
 *      niv_port_id - (IN) NIV port ID.
 *      encap_id    - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_niv_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                bcm_gport_t niv_port_id, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int vp;
        ing_dvp_table_entry_t dvp;

        if (!BCM_GPORT_IS_NIV_PORT(niv_port_id)) {
            return BCM_E_PARAM;
        }
        vp = BCM_GPORT_NIV_PORT_ID_GET(niv_port_id); 
        if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

        /* Next-hop index is used for multicast replication */
        *encap_id = (bcm_if_t) soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        *encap_id += BCM_XGS3_DVP_EGRESS_IDX_MIN;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_multicast_egress_object_encap_get
 * Purpose:
 *      Get the Encap ID for Egress_object
 * Parameters:
 *      unit        - (IN) Unit number.
 *      group       - (IN) Multicast group ID.
 *      intf          - (IN) Egress Object ID.
 *      encap_id    - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_egress_object_encap_get(int unit, bcm_multicast_t group, bcm_if_t intf,
                                bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
#if defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_l3)) {
        if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf)) {
            if (soc_feature(unit, soc_feature_l3mc_use_egress_next_hop)) {
                *encap_id = intf;
            } else {
                *encap_id = (intf - BCM_XGS3_EGRESS_IDX_MIN) + 
                        BCM_XGS3_DVP_EGRESS_IDX_MIN;
            }
        } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, intf)) {
            *encap_id = intf;
        } else if (BCM_XGS3_PROXY_EGRESS_IDX_VALID(unit, intf)) {
            *encap_id = (intf - BCM_XGS3_PROXY_EGRESS_IDX_MIN) + BCM_XGS3_DVP_EGRESS_IDX_MIN;
        } else {
            return BCM_E_PARAM;
        }
        return BCM_E_NONE;
    }
#endif  /* INCLUDE_L3 */
#endif /* BCM_TRIDENT_SUPPORT || BCM_GREYHOUND_SUPPORT */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_multicast_l2gre_encap_get
 * Purpose:
 *      Get the Encap ID for L2GRE virtual port
 * Parameters:
 *      unit        - (IN) Unit number.
 *      group       - (IN) Multicast group ID.
 *      port        - (IN) Physical port.
 *      l2gre_port_id - (IN) L2GRE port ID.
 *      encap_id    - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_l2gre_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                bcm_gport_t l2gre_port_id, bcm_if_t *encap_id)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_l2gre)) {
        int vp;
        ing_dvp_table_entry_t dvp;

        if (!BCM_GPORT_IS_L2GRE_PORT(l2gre_port_id)) {
            return BCM_E_PARAM;
        }
        /* Get Multicast DVP index */
        vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port_id); 
        if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
            return BCM_E_PARAM;
        }
        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
            return BCM_E_PARAM;
        }
        /* Multicast DVP will never point to ECMP group */
        BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
        if (soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMPf)) {
            return BCM_E_PARAM;
        }

        /* Next-hop index is used for multicast replication */
        *encap_id = (bcm_if_t) soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        *encap_id += BCM_XGS3_DVP_EGRESS_IDX_MIN;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcm_esw_multicast_vxlan_encap_get
 * Purpose:
 *      Get the Encap ID for VXLAN virtual port
 * Parameters:
 *      unit        - (IN) Unit number.
 *      group       - (IN) Multicast group ID.
 *      port        - (IN) Physical port.
 *      vxlan_port_id - (IN) L2GRE port ID.
 *      encap_id    - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_vxlan_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                bcm_gport_t vxlan_port_id, bcm_if_t *encap_id)
{
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_vxlan)) {
        int vp;
        ing_dvp_table_entry_t dvp;

        if (!BCM_GPORT_IS_VXLAN_PORT(vxlan_port_id)) {
            return BCM_E_PARAM;
        }

        /* Get Multicast DVP index */
        vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port_id); 
        if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
            return BCM_E_PARAM;
        }

        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            return BCM_E_PARAM;
        }

        /* Multicast DVP will never point to ECMP group */
        BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

        /* Next-hop index is used for multicast replication */
        *encap_id = (bcm_if_t) soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        *encap_id += BCM_XGS3_DVP_EGRESS_IDX_MIN;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_extender_encap_get
 * Purpose:
 *      Get the Encap ID for Extender virtual port.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      group       - (IN) Multicast group ID.
 *      port        - (IN) Physical port.
 *      extender_port_id - (IN) Extender port ID.
 *      encap_id    - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_extender_encap_get(int unit, bcm_multicast_t group,
        bcm_gport_t port, bcm_gport_t extender_port_id, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {
        int vp;
        ing_dvp_table_entry_t dvp;

        if (!BCM_GPORT_IS_EXTENDER_PORT(extender_port_id)) {
            return BCM_E_PARAM;
        }
        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(extender_port_id); 
        if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN
            (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

        /* Next-hop index is used for multicast replication */
        *encap_id = (bcm_if_t) soc_ING_DVP_TABLEm_field32_get(unit, &dvp,
                NEXT_HOP_INDEXf);
        *encap_id += BCM_XGS3_DVP_EGRESS_IDX_MIN;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_mac_encap_get
 * Purpose:
 *      Get the Encap ID for MAC virtual port.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      group       - (IN) Multicast group ID.
 *      port        - (IN) Physical port.
 *      mac_port_id - (IN) MAC port ID.
 *      encap_id    - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_esw_multicast_mac_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                bcm_gport_t mac_port_id, bcm_if_t *encap_id)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_mac_virtual_port)) {
        int vp;
        ing_dvp_table_entry_t dvp;

        if (!BCM_GPORT_IS_MAC_PORT(mac_port_id)) {
            return BCM_E_PARAM;
        }
        vp = BCM_GPORT_MAC_PORT_ID_GET(mac_port_id); 
        if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

        /* Next-hop index is used for multicast replication */
        *encap_id = (bcm_if_t) soc_ING_DVP_TABLEm_field32_get(unit, &dvp,
                NEXT_HOP_INDEXf);
        *encap_id += BCM_XGS3_DVP_EGRESS_IDX_MIN;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_egress_add
 * Purpose:
 *      Add a GPORT to the replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_egress_add(int unit, bcm_multicast_t group, 
                             bcm_gport_t port, bcm_if_t encap_id)
{
    int rv=BCM_E_NONE;

    MULTICAST_INIT_CHECK(unit);

   /* Check whether the given multicast group is available */
    rv = bcm_esw_multicast_group_is_free(unit, group);
    if (rv != BCM_E_EXISTS) {
       return BCM_E_UNAVAIL;
    }

#if defined(BCM_XGS3_SWITCH_SUPPORT) 
    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (_BCM_MULTICAST_L2_OR_FABRIC(unit, group)) {
            return _bcm_esw_multicast_l2_add(unit, group, port, encap_id);
        } else {
#if defined(INCLUDE_L3)
            /* If (1) the given group is an IPMC group, (2) the given port is
             * a trunk gport, and (3) IPMC trunk resolution is disabled for the
             * trunk group in hardware, then encap_id is added to only one
             * member of the trunk group. The following procedure finds which
             * trunk member the encap_id is mapped to.
             */
            bcm_gport_t mapped_port;
            BCM_IF_ERROR_RETURN
                (_bcm_esw_multicast_egress_encap_id_to_trunk_member_map(unit,
                    group, 1, &port, &encap_id, &mapped_port));
            return _bcm_esw_multicast_l3_add(unit, group, mapped_port, encap_id);
#endif /* INCLUDE_L3 */
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_egress_delete
 * Purpose:
 *      Delete GPORT from the replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_egress_delete(int unit, bcm_multicast_t group, 
                                bcm_gport_t port, bcm_if_t encap_id)
{
    int rv=BCM_E_NONE;

    MULTICAST_INIT_CHECK(unit);

    /* Check whether the given multicast group is available */
    rv = bcm_esw_multicast_group_is_free(unit, group);
    if (rv != BCM_E_EXISTS) {
       return BCM_E_UNAVAIL;
    }

#if defined(BCM_XGS3_SWITCH_SUPPORT) 
    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (_BCM_MULTICAST_L2_OR_FABRIC(unit, group)) {
            return _bcm_esw_multicast_l2_delete(unit, group, port, encap_id);
        } else {
#if defined(INCLUDE_L3)
            /* If (1) the given group is an IPMC group, (2) the given port is
             * a trunk gport, and (3) IPMC trunk resolution is disabled for
             * the trunk group in hardware, then encap_id resides on only one
             * member of the trunk group. The following procedure finds which
             * trunk member the encap_id resides on.
             */
            bcm_gport_t mapped_port;
            BCM_IF_ERROR_RETURN
                (_bcm_esw_multicast_egress_mapped_trunk_member_find(unit,
                    group, port, encap_id, &mapped_port));
            return _bcm_esw_multicast_l3_delete(unit, group, mapped_port, encap_id);
#endif /* INCLUDE_L3 */
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_multicast_egress_delete_all
 * Purpose:
 *      Delete all replications for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_egress_delete_all(int unit, bcm_multicast_t group)
{ 
    int rv=BCM_E_NONE;

    MULTICAST_INIT_CHECK(unit);

    /* Check whether the given multicast group is available */
    rv = bcm_esw_multicast_group_is_free(unit, group);
    if (rv != BCM_E_EXISTS) {
       return BCM_E_UNAVAIL;
    }

    return bcm_esw_multicast_egress_set(unit, group, 0, NULL, NULL);
}

/*  
 * Function:
 *      bcm_multicast_egress_set
 * Purpose:
 *      Assign the complete set of egress GPORTs in the
 *      replication list for the specified multicast index.
 * Parameters:
 *      unit       - (IN) Device Number
 *      group      - (IN) Multicast group ID
 *      port_count - (IN) Number of ports in replication list
 *      port_array - (IN) List of GPORT Identifiers
 *      encap_id_array - (IN) List of encap identifiers
 * Returns:
 *      BCM_E_XXX
 */     
int     
bcm_esw_multicast_egress_set(int unit, bcm_multicast_t group, int port_count,
                             bcm_gport_t *port_array, bcm_if_t *encap_id_array)
{
    int rv=BCM_E_NONE;

    MULTICAST_INIT_CHECK(unit);

    /* Check whether the given multicast group is available */
    rv = bcm_esw_multicast_group_is_free(unit, group);
    if (rv != BCM_E_EXISTS) {
       return BCM_E_UNAVAIL;
    }

#if defined(BCM_XGS3_SWITCH_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        /* If (1) the given group is an IPMC group, (2) the given port is a
         * trunk gport, and (3) IPMC trunk resolution is disabled for the
         * trunk group in hardware, then each encap_id is added to only one
         * member in trunk group. The following procedure maps the encap_ids
         * to trunk members.
         */
        int rv;
        bcm_gport_t *mapped_port_array = NULL;
        if (port_count > 0) {
            mapped_port_array = sal_alloc(sizeof(bcm_gport_t) * port_count,
                    "resolved port array");
            if (NULL == mapped_port_array) {
                return BCM_E_MEMORY;
            }
            rv = _bcm_esw_multicast_egress_encap_id_to_trunk_member_map(unit,
                    group, port_count, port_array, encap_id_array,
                    mapped_port_array);
            if (BCM_FAILURE(rv)) {
                sal_free(mapped_port_array);
                return rv;
            }
        }

        rv = _bcm_esw_multicast_egress_set(unit, group, port_count, 
                mapped_port_array, encap_id_array);

        if (mapped_port_array) {
            sal_free(mapped_port_array);
        }

        return rv;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}   

#if (defined(BCM_TRX_SUPPORT) || defined(BCM_XGS3_SWITCH_SUPPORT)) && defined(INCLUDE_L3)
/*
 * Function:
 *      _bcm_esw_multicast_egress_get
 * Purpose:
 *      Helper function to API 
 * Parameters: 
 *      unit           - (IN) Device Number
 *      mc_index       - (IN) Multicast index
 *      port_max       - (IN) Number of entries in "port_array"
 *      port_array     - (OUT) List of ports
 *      encap_id_array - (OUT) List of encap identifiers
 *      port_count     - (OUT) Actual number of ports returned
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the input parameter port_max = 0, return in the output parameter
 *      port_count the total number of ports/encapsulation IDs in the 
 *      specified multicast group's replication list.
 */
STATIC int     
_bcm_esw_multicast_egress_get(int unit, bcm_multicast_t group, int port_max,
                              bcm_gport_t *port_array,
                              bcm_if_t *encap_id_array, 
                              int *port_count)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT) 
    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (_BCM_MULTICAST_L2_OR_FABRIC(unit, group)) {
            return _bcm_esw_multicast_l2_get(unit, group, port_max, port_array, 
                                             encap_id_array, port_count);
        } else {
#if defined(INCLUDE_L3)
            return _bcm_esw_multicast_l3_get(unit, group, port_max, port_array, 
                                             encap_id_array, port_count);
#endif /* INCLUDE_L3 */
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    return BCM_E_UNAVAIL;
}
#endif

/*
 * Function:
 *      bcm_multicast_egress_get
 * Purpose:
 *      Retrieve a set of egress multicast GPORTs in the
 *      replication list for the specified multicast index.
 * Parameters: 
 *      unit           - (IN) Device Number
 *      mc_index       - (IN) Multicast index
 *      port_max       - (IN) Number of entries in "port_array"
 *      port_array     - (OUT) List of ports
 *      encap_id_array - (OUT) List of encap identifiers
 *      port_count     - (OUT) Actual number of ports returned
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the input parameter port_max = 0, return in the output parameter
 *      port_count the total number of ports/encapsulation IDs in the 
 *      specified multicast group's replication list.
 */
int     
bcm_esw_multicast_egress_get(int unit, bcm_multicast_t group, int port_max,
                             bcm_gport_t *port_array,
                             bcm_if_t *encap_id_array, 
                             int *port_count)
{
    int rv = BCM_E_UNAVAIL;
#if (defined(BCM_TRX_SUPPORT) || defined(BCM_XGS3_SWITCH_SUPPORT)) && defined(INCLUDE_L3)
    int i;
    bcm_module_t mod_out, my_modid;
    bcm_port_t port_in, port_out;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE;
#endif

    MULTICAST_INIT_CHECK(unit);

    /* Check whether the given multicast group is available */
    rv = bcm_esw_multicast_group_is_free(unit, group);
    if (rv != BCM_E_EXISTS) {
       return BCM_E_UNAVAIL;
    }

    /* port_array and encap_id_array are allowed to be NULL */
    if ((NULL == port_count) || (port_max < 0) ){
        return BCM_E_PARAM;
    }

    /* If port_max = 0, port_array and encap_id_array must be NULL */
    if (port_max == 0) {
        if ((NULL != port_array) || (NULL != encap_id_array)) {
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_multicast_egress_get(unit, group, port_max, port_array,
                                      encap_id_array, port_count));

    if (NULL != port_array) {
        /* Convert GPORT_LOCAL format to GPORT_MODPORT format */
        if (bcm_esw_stk_my_modid_get(unit, &my_modid) < 0) {
            return BCM_E_INTERNAL;
        }
        for (i = 0; i < *port_count; i++) {

#if defined(BCM_KATANA2_SUPPORT)
            /* For LinkPHY/SubTag subport return in subport_gport format*/
            if (soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) {
                is_local_subport = FALSE;
                if (BCM_GPORT_IS_SET(port_array[i]) &&
                    _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                    port_array[i])) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport) {
            } else
#endif
            {
                BCM_IF_ERROR_RETURN(
                    bcm_esw_port_local_get(unit, port_array[i], &port_in));
        
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                            my_modid, port_in, &mod_out, &port_out));
                BCM_GPORT_MODPORT_SET(port_array[i], mod_out, port_out);
            }
        }
    }

    rv = BCM_E_NONE;
#endif

    return rv;
}   



int
bcm_esw_multicast_repl_set(int unit, int mc_index, bcm_port_t port,
                      bcm_vlan_vector_t vlan_vec)
{
    return BCM_E_UNAVAIL;
}

int
bcm_esw_multicast_repl_get(int unit, int index, bcm_port_t port,
                      bcm_vlan_vector_t vlan_vec)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_multicast_group_get
 * Purpose:
 *      Retrieve the flags associated with a multicast group.
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) Multicast group ID
 *      flags - (OUT) BCM_MULTICAST_*
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_multicast_group_get(int unit, bcm_multicast_t group, uint32 *flags)
{
    MULTICAST_INIT_CHECK(unit);

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        return _bcm_esw_fabric_multicast_group_get(unit, group, flags);
    }
#endif	/* BCM_XGS3_FABRIC_SUPPORT */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TRX(unit)) {
        return bcm_trx_multicast_group_get(unit, group, flags);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
#if defined(BCM_XGS3_SWITCH_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        return bcm_xgs3_multicast_group_get(unit, group, flags);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_multicast_group_is_free
 * Purpose:
 *      Request if the given multicast group is available on the
 *      device
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) Multicast group ID.
 * Returns:
 *      BCM_E_NONE - multicast group is valid and available on the device
 *      BCM_E_EXISTS - multicast group is valid but already in use
 *                     on this device
 *      BCM_E_PARAM - multicast group is not valid on this device 
 * Notes:
 */
int 
bcm_esw_multicast_group_is_free(int unit, bcm_multicast_t group)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    int             mc_index;
    int             rv = BCM_E_PARAM;
    int             is_set;

    mc_index = _BCM_MULTICAST_ID_GET(group);

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        rv = _bcm_esw_fabric_multicast_param_check(unit, group, &mc_index);
        /* Remap return codes */
        if (BCM_E_NOT_FOUND == rv) {
            /* Multicast group is available, return without error */
            rv = BCM_E_NONE;
        } else if (BCM_E_NONE == rv) {
            /* Multicast group is already configured */
            rv = BCM_E_EXISTS;
        }
    } else 
#endif	/* BCM_XGS3_FABRIC_SUPPORT */
    if (_BCM_MULTICAST_IS_L2(group)) {
        rv = _bcm_xgs3_l2mc_index_is_set(unit, mc_index, &is_set);
        if (BCM_SUCCESS(rv)) {
            if (is_set) {
                rv = BCM_E_EXISTS;
            }
        }
#if defined(INCLUDE_L3)
    } else if (_BCM_MULTICAST_IS_SET(group)) {
        uint32 flags;
        uint8 group_type;

        group_type = _BCM_MULTICAST_TYPE_GET(group);
        flags = _bcm_esw_multicast_group_type_to_flags(group_type);
        /* Check if this group type is supported on this device. */
        BCM_IF_ERROR_RETURN(_bcm_esw_multicast_type_validate(unit, flags));

        rv = bcm_xgs3_ipmc_id_is_set(unit, mc_index, &is_set);
        if (BCM_SUCCESS(rv)) {
            if (is_set) {
                rv = BCM_E_EXISTS;
            }
        }
#endif /* INCLUDE_L3 */
    }

    return rv; 
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_XGS3_SWITCH_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_multicast_group_free_range_get
 * Purpose:
 *      Retrieve the minimum and maximum unallocated multicast groups
 *      for a given multicast type.
 * Parameters:
 *      unit - (IN) Unit number.
 *      type_flag - (IN) One of BCM_MULTICAST_TYPE_*.
 *      group_min - (OUT) Minimum available multicast group of specified type.
 *      group_max - (OUT) Maximum available multicast group of specified type.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_multicast_group_free_range_get(int unit, uint32 type_flag, 
                                       bcm_multicast_t *group_min,
                                       bcm_multicast_t *group_max)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    bcm_multicast_t dev_min, dev_max, group, free_min, free_max;
    int             index_min, index_max, group_type, min_group_index;
    int             rv = BCM_E_PARAM;
    uint32          type;

    type = type_flag & BCM_MULTICAST_TYPE_MASK;
    if (1 != _shr_popcount(type)) {
        return BCM_E_PARAM;
    }

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        if (type_flag == BCM_MULTICAST_TYPE_L2) {
            index_min = BCM_FABRIC_MCAST_BASE(unit);
            index_max =  BCM_FABRIC_MCAST_MAX(unit);
            min_group_index = 1;
        } else {
            index_min = BCM_FABRIC_IPMC_BASE(unit);
            index_max = BCM_FABRIC_IPMC_MAX(unit);
            min_group_index = 0;
        }
    } else 
#endif	/* BCM_XGS3_FABRIC_SUPPORT */
    if (type_flag == BCM_MULTICAST_TYPE_L2) {
        index_min = soc_mem_index_min(unit, L2MCm);
        index_max = soc_mem_index_max(unit, L2MCm);
        min_group_index = 1;
    } else {
        index_min = soc_mem_index_min(unit, L3_IPMCm);
        index_max = soc_mem_index_max(unit, L3_IPMCm);
        min_group_index = 0;
    }

    group_type = _bcm_esw_multicast_flags_to_group_type(type_flag);
    _BCM_MULTICAST_GROUP_SET(dev_min, group_type, min_group_index);
    _BCM_MULTICAST_GROUP_SET(dev_max, group_type, index_max - index_min);


    free_min = free_max = 0; /* Invalid multicast group */
    for (group = dev_min; group <= dev_max; group++) {
        rv = bcm_esw_multicast_group_is_free(unit, group);
        if (BCM_SUCCESS(rv)) {
            if (0 == free_min) {
                free_min = group;
            }
            free_max = group;
        } else if (BCM_E_EXISTS == rv) {
            /* Nothing to do but clear the error */
            rv = BCM_E_NONE;
        } else {
            /* Real error, return */
            break;
        }
    }

    if (BCM_SUCCESS(rv)) {
        if (0 == free_min) {
            /* No available groups of this type */
            return BCM_E_NOT_FOUND;
        } else {
            /* Copy the results */
            *group_min = free_min;
            *group_max = free_max;
        }
    }

    return rv; 
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_XGS3_SWITCH_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_multicast_group_traverse
 * Purpose:
 *      Iterate over the defined multicast groups of the type
 *      specified in 'flags'.  If all types are desired, use
 *      MULTICAST_TYPE_MASK.
 * Parameters:
 *      unit - (IN) Unit number.
 *      trav_fn - (IN) Callback function.
 *      flags - (IN) BCM_MULTICAST_*
 *      user_data - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_multicast_group_traverse(int unit,
                                 bcm_multicast_group_traverse_cb_t trav_fn, 
                                 uint32 flags, void *user_data)
{
    MULTICAST_INIT_CHECK(unit);

#ifdef BCM_XGS3_FABRIC_SUPPORT
    if (SOC_IS_XGS3_FABRIC(unit)) {
        return _bcm_esw_fabric_multicast_group_traverse(unit, trav_fn,
                                                        flags, user_data);
    }
#endif	/* BCM_XGS3_FABRIC_SUPPORT */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_TRX(unit)) {
        return bcm_trx_multicast_group_traverse(unit, trav_fn,
                                                flags, user_data);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
#if defined(BCM_XGS3_SWITCH_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        return bcm_xgs3_multicast_group_traverse(unit, trav_fn,
                                                 flags, user_data);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

#if defined(INCLUDE_L3)
/*
 * Function:
 *      _bcm_esw_multicast_remap_group_set
 * Purpose:
 *      Set an IPMC remapping.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      from      - (IN) Multicast group ID to map from
 *      to        - (IN) Multicast group ID to map to
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
_bcm_esw_multicast_remap_group_set(int unit, bcm_multicast_t from, int to)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT)
    if (soc_feature(unit, soc_feature_ipmc_remap)) {

        rv = _bcm_esw_multicast_l3_group_check(unit, from, NULL);
        BCM_IF_ERROR_RETURN(rv);

        rv = _bcm_esw_multicast_l3_group_check(unit, to, NULL);
        BCM_IF_ERROR_RETURN(rv);

        rv = bcm_tr2_ipmc_remap_set(unit, from, to);
    }
#endif

    return rv;
}


/*
 * Function:
 *      _bcm_esw_multicast_remap_group_get
 * Purpose:
 *      Set an IPMC remapping.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      from      - (IN) Multicast group ID being mapped
 *      to        - (OUT) Pointer to Multicast group ID being mapped to,
 *                        using the same group type as 'from'.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
_bcm_esw_multicast_remap_group_get(int unit, bcm_multicast_t from, int *to)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT)
    if (soc_feature(unit, soc_feature_ipmc_remap)) {
        
        rv = _bcm_esw_multicast_l3_group_check(unit, from, NULL);
        if (BCM_SUCCESS(rv)) {
            rv = bcm_tr2_ipmc_remap_get(unit, from, to);
        }
    }
#endif

    return rv;
}
#endif

/*
 * Function:
 *      bcm_multicast_control_set
 * Purpose:
 *      Set multicast group control.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      group     - (IN) Multicast group ID.
 *      type      - (IN) Multicast group control type.
 *      arg       - (IN) Multicast group control argument.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_control_set(int unit,
        bcm_multicast_t group,
        bcm_multicast_control_t type,
        int arg)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_XGS3_SWITCH_SUPPORT) 
#if defined(INCLUDE_L3)
    int index; 
    mtu_values_entry_t l3_mtu_values_entry;
#endif

    MULTICAST_INIT_CHECK(unit);

    if (!SOC_IS_XGS3_SWITCH(unit)) {
        return BCM_E_UNAVAIL;
    }

#if defined(INCLUDE_L3)
    rv = (_bcm_esw_multicast_l3_group_check(unit, group, NULL));
#endif

    if (!_BCM_MULTICAST_IS_L2(group) && BCM_FAILURE(rv)) {
        return rv;
    }
    
    switch (type) {
        case bcmMulticastControlMtu:
            if (_BCM_MULTICAST_IS_L2(group)) {
                /* Not applicable to multicast types that uses a L2MC index.
                 * Only applicable to multicast types that uses a L3MC index.
                 */
                return BCM_E_PARAM;
            }
            if (SOC_MEM_IS_VALID(unit, L3_MTU_VALUESm)) {
#if defined(INCLUDE_L3) 
                index = _BCM_MULTICAST_MTU_TABLE_OFFSET(unit) +
                        _BCM_MULTICAST_ID_GET(group);
                if (!soc_mem_index_valid(unit, L3_MTU_VALUESm, index)) {
                    return BCM_E_PARAM;
                }

                SOC_IF_ERROR_RETURN(READ_L3_MTU_VALUESm(unit, MEM_BLOCK_ANY,
                            index, &l3_mtu_values_entry));

                if(!SOC_MEM_FIELD32_VALUE_FIT(unit, L3_MTU_VALUESm,
                            MTU_SIZEf, arg)) {
                    return BCM_E_PARAM;
                }
                soc_L3_MTU_VALUESm_field32_set(unit, &l3_mtu_values_entry,
                        MTU_SIZEf, arg);

                rv = WRITE_L3_MTU_VALUESm(unit, MEM_BLOCK_ALL, 
                                          index, &l3_mtu_values_entry);
#endif /* INCLUDE_L3 */
            } else if (SOC_MEM_FIELD_VALID(unit, L3_IPMCm, IPMC_MTU_INDEXf) && 
                       SOC_REG_IS_VALID(unit, IPMC_L3_MTUr)) {
#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
#if defined(INCLUDE_L3) 
                uint64 rval64, *rval64s[1];
                uint32 pindex, entry[SOC_MAX_MEM_WORDS];

                rval64s[0] = &rval64;
                COMPILER_64_ZERO(rval64);
                COMPILER_64_SET(rval64, 0, arg);
                index = _BCM_MULTICAST_ID_GET(group);
                if (!soc_mem_index_valid(unit, L3_IPMCm, index)) {
                    return BCM_E_PARAM;
                }
                BCM_IF_ERROR_RETURN
                    (soc_profile_reg_add(unit, _bcm_mtu_profile[unit], rval64s,
                                         1, &pindex));
                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, L3_IPMCm,
                                  MEM_BLOCK_ANY, index, &entry));
                soc_mem_field32_set(unit, L3_IPMCm, &entry,
                                    IPMC_MTU_INDEXf, pindex);
                BCM_IF_ERROR_RETURN
                    (soc_mem_write(unit, L3_IPMCm, MEM_BLOCK_ANY, index, 
                                   &entry));
#endif /* INCLUDE_L3 */
#endif
            } else {
                return BCM_E_UNAVAIL;
            }
            break;

        case bcmMulticastVpTrunkResolve:
            if (_BCM_MULTICAST_IS_L2(group)) {
                /* Not applicable to multicast types that uses a L2MC index. */
                return BCM_E_PARAM;
            }
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
            if (soc_feature(unit, soc_feature_vp_lag)) {
                egr_ipmc_entry_t egr_ipmc_entry;

                index = _BCM_MULTICAST_ID_GET(group);
                BCM_IF_ERROR_RETURN(READ_EGR_IPMCm(unit, MEM_BLOCK_ANY, index,
                            &egr_ipmc_entry));
                soc_EGR_IPMCm_field32_set(unit, &egr_ipmc_entry,
                        ENABLE_VPLAG_RESOLUTIONf, arg ? TRUE : FALSE);
                BCM_IF_ERROR_RETURN(WRITE_EGR_IPMCm(unit, MEM_BLOCK_ALL, index,
                            &egr_ipmc_entry));
            } else
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */
            {
                return BCM_E_PARAM;
            }
            break;

#if defined(INCLUDE_L3)
        case bcmMulticastRemapGroup:
            rv = _bcm_esw_multicast_remap_group_set(unit, group, arg);
            break;
#endif

        default:
            return BCM_E_PARAM;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_multicast_control_get
 * Purpose:
 *      Get multicast group control.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      group     - (IN) Multicast group ID.
 *      type      - (IN) Multicast group control type.
 *      arg       - (OUT) Multicast group control argument.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_control_get(int unit,
        bcm_multicast_t group,
        bcm_multicast_control_t type,
        int *arg)
{
    int rv = BCM_E_UNAVAIL;
    

#if defined(BCM_XGS3_SWITCH_SUPPORT) 
#if defined(INCLUDE_L3)
    mtu_values_entry_t l3_mtu_values_entry;
    int index;
#endif

    MULTICAST_INIT_CHECK(unit);

    if (!SOC_IS_XGS3_SWITCH(unit)) {
        return BCM_E_UNAVAIL;
    }

#if defined(INCLUDE_L3)
    rv = (_bcm_esw_multicast_l3_group_check(unit, group, NULL));
#endif

    if (!_BCM_MULTICAST_IS_L2(group) && BCM_FAILURE(rv)) {
        return rv;
    }

    switch (type) {
        case bcmMulticastControlMtu:
            if (_BCM_MULTICAST_IS_L2(group)) {
                /* Not applicable to multicast types that uses a L2MC index.
                 * Only applicable to multicast types that uses a L3MC index.
                 */
                return BCM_E_PARAM;
            }

            if (SOC_MEM_IS_VALID(unit, L3_MTU_VALUESm)) {
#if defined(INCLUDE_L3) 
                index = _BCM_MULTICAST_MTU_TABLE_OFFSET(unit) +
                        _BCM_MULTICAST_ID_GET(group);
                if (!soc_mem_index_valid(unit, L3_MTU_VALUESm, index)) {
                    return BCM_E_PARAM;
                }

                SOC_IF_ERROR_RETURN(READ_L3_MTU_VALUESm(unit, MEM_BLOCK_ANY,
                            index, &l3_mtu_values_entry));
                *arg = soc_L3_MTU_VALUESm_field32_get(unit,
                        &l3_mtu_values_entry, MTU_SIZEf);
                /* if no error assume success */
                rv = BCM_E_NONE;
#endif /* INCLUDE_L3 */
            } else if (SOC_MEM_FIELD_VALID(unit, L3_IPMCm, IPMC_MTU_INDEXf) && 
                       SOC_REG_IS_VALID(unit, IPMC_L3_MTUr)) {
#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
#if defined(INCLUDE_L3) 
                uint64 rval64, *rval64s[1];
                uint32 pindex, entry[SOC_MAX_MEM_WORDS];

                rval64s[0] = &rval64;
                index = _BCM_MULTICAST_ID_GET(group);
                if (!soc_mem_index_valid(unit, L3_IPMCm, index)) {
                    return BCM_E_PARAM;
                }
                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, L3_IPMCm,
                                  MEM_BLOCK_ANY, index, &entry));
                pindex = soc_mem_field32_get(unit, L3_IPMCm, &entry,
                                             IPMC_MTU_INDEXf);
                BCM_IF_ERROR_RETURN
                    (soc_profile_reg_get(unit, _bcm_mtu_profile[unit], pindex,
                                         1, rval64s));
                *arg = COMPILER_64_LO(rval64);
#endif /* INCLUDE_L3 */
#endif
            } else {
                return BCM_E_UNAVAIL;
            }
            break;

        case bcmMulticastVpTrunkResolve:
            if (_BCM_MULTICAST_IS_L2(group)) {
                /* Not applicable to multicast types that uses a L2MC index. */
                return BCM_E_PARAM;
            }
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
            if (soc_feature(unit, soc_feature_vp_lag)) {
                egr_ipmc_entry_t egr_ipmc_entry;

                index = _BCM_MULTICAST_ID_GET(group);
                BCM_IF_ERROR_RETURN(READ_EGR_IPMCm(unit, MEM_BLOCK_ANY, index,
                            &egr_ipmc_entry));
                *arg = soc_EGR_IPMCm_field32_get(unit, &egr_ipmc_entry,
                        ENABLE_VPLAG_RESOLUTIONf);
            } else
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */
            {
                return BCM_E_PARAM;
            }
            break;

#if defined(INCLUDE_L3)
        case bcmMulticastRemapGroup:
            rv = _bcm_esw_multicast_remap_group_get(unit, group, arg);
            break;
#endif

        default:
            return BCM_E_PARAM;
    }

#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return rv;
}

/*
 * Function:
 *      _bcm_esw_ipmc_subscriber_egress_intf_add
 * Purpose:
 *      Helper function to add ipmc egress interface on different devices.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   ipmc id
 *      port       - (IN)  Port number
 *      nh_index   - (IN)  Next Hop index
 *      subscriber_queue - (IN) Subscriber gport
 *      is_l3      - (IN)  Identifier of L2/L3 IPMC interface, relevant only for FB
 *      queue_count - (OUT) number of queues in the extended queue list
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_esw_ipmc_subscriber_egress_intf_add(int unit, int ipmc_id, 
                                         bcm_port_t port,
                                         int nh_index, 
                                         bcm_gport_t subscriber_queue,
                                         int is_l3,
                                         int *queue_count,
                                         int *q_count_increased)
{
#if defined (INCLUDE_L3)
#if defined(BCM_KATANA_SUPPORT) 
    int id;
    if (SOC_IS_KATANAX(unit)) {
        id = nh_index;
        return _bcm_kt_ipmc_subscriber_egress_intf_set(unit, ipmc_id, port, 1,
                                                &id, subscriber_queue, is_l3,
                                                queue_count, q_count_increased);
    }
#endif /* BCM_KATANA_SUPPORT */
#endif /* INCLUDE_L3 */
        return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_ipmc_subscriber_egress_intf_delete
 * Purpose:
 *      Helper function to add ipmc egress interface on different devices.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   ipmc id
 *      port       - (IN)  Port number
 *      nh_index   - (IN)  Next Hop index
 *      subscriber_queue - (IN) Subscriber gport
 *      is_l3      - (IN)  Identifier of L2/L3 IPMC interface, relevant only for FB
 *      queue_count - (OUT) number of queues in the extended queue list
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_esw_ipmc_subscriber_egress_intf_delete(int unit, int ipmc_id, 
                                            bcm_port_t port,
                                            int nh_index, 
                                            bcm_gport_t subscriber_queue, 
                                            int is_l3,
                                            int *queue_count,
                                            int *q_count_decreased)
{
#if defined (INCLUDE_L3)
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
        return _bcm_kt_ipmc_subscriber_egress_intf_delete(unit, ipmc_id, port, 
                                                nh_index, subscriber_queue,
                                                queue_count, q_count_decreased);
    }
#endif /* BCM_KATANA_SUPPORT */
#endif /* INCLUDE_L3 */
        return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_ipmc_subscriber_egress_intf_set
 * Purpose:
 *      Helper function to add ipmc egress interface on different devices.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   ipmc id
 *      port       - (IN)  Port number
 *      nh_index   - (IN)  Next Hop index
 *      subscriber_queue - (IN) Subscriber gport
 *      is_l3      - (IN)  Identifier of L2/L3 IPMC interface, relevant only for FB
 *      queue_count - (OUT) number of queues in the extended queue list
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_esw_ipmc_subscriber_egress_intf_set(int unit, int ipmc_id, 
                                         bcm_port_t port,
                                         int if_count,
                                         int *if_array, 
                                         bcm_gport_t subscriber_queue, 
                                         int is_l3,
                                         int *queue_count,
                                         int *q_count_increased)
{
#if defined (INCLUDE_L3)
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
        return _bcm_kt_ipmc_subscriber_egress_intf_set(unit, ipmc_id, port, 
                                                if_count, if_array, 
                                                subscriber_queue, is_l3,
                                                queue_count,
                                                q_count_increased);
    }
#endif /* BCM_KATANA_SUPPORT */
#endif /* INCLUDE_L3 */
        return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_ipmc_subscriber_egress_intf_get
 * Purpose:
 *      Helper function to add ipmc egress interface on different devices.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   ipmc id
 *      port       - (IN)  Port number
 *      nh_index   - (IN)  Next Hop index
 *      subscriber_queue - (IN) Subscriber gport
 *      is_l3      - (IN)  Identifier of L2/L3 IPMC interface, relevant only for FB
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_esw_ipmc_subscriber_egress_intf_get(int unit, bcm_multicast_t group,
                                         int port_count, 
                                         bcm_gport_t *port_array,
                                         bcm_if_t *encap_id_array,
                                         bcm_gport_t *subscriber_queue_array, 
                                         int *count) 
{
#if defined (INCLUDE_L3)
#if defined(BCM_KATANA_SUPPORT) 
    if (SOC_IS_KATANAX(unit)) {
        return _bcm_kt_ipmc_subscriber_egress_intf_get(unit, group, port_count,
                                                port_array, encap_id_array, 
                                                subscriber_queue_array, count);
    }
#endif /* BCM_KATANA_SUPPORT */
#endif /* INCLUDE_L3 */
        return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_ipmc_mmu_mc_remap_ptr
 * Purpose:
 *      Helper function to set/get ipmc mmu remap pointer
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   ipmc id
 *      remap_id - (IN/OUT) set/get remap id
 *      set          -  (IN) set/get input
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_esw_ipmc_mmu_mc_remap_ptr(int unit, int ipmc_id, 
                                           int *remap_id, int set)
{
#if defined (INCLUDE_L3)
#if defined(BCM_KATANA_SUPPORT) 
    if (SOC_IS_KATANAX(unit)) {
        return _bcm_kt_ipmc_mmu_mc_remap_ptr(unit, ipmc_id, 
                                             remap_id, set); 
    }
#endif /* BCM_KATANA_SUPPORT */
#endif /* INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_ipmc_set_remap_group
 * Purpose:
 *      Helper function to set/get ipmc mmu remap pointer
 * Parameters:
 *      unit       - (IN)   Device Number
 *      ipmc_id    - (IN)   ipmc id
 *      port      - (IN)  port id
 *      count          -  (IN) count
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_esw_ipmc_set_remap_group(int unit, int ipmc_id, 
                                       bcm_port_t port_out, 
                                       int count)
{
#if defined (INCLUDE_L3)
#if defined(BCM_KATANA_SUPPORT) 
    if (SOC_IS_KATANAX(unit)) {
        return _bcm_kt_ipmc_set_remap_group(unit, ipmc_id, 
                                    port_out, count); 
    }
#endif /* BCM_KATANA_SUPPORT */
#endif /* INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

#if defined(BCM_KATANA_SUPPORT)

/* Macro to free memory and return code instead of using goto */

#define BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(_r_)   \
    if (local_port_array) {                             \
        sal_free(local_port_array);                     \
    }                                                   \
    if (temp_encap_list) {                              \
        sal_free(temp_encap_list);                      \
        temp_encap_list = NULL;                         \
    }                                                   \
    return _r_;

/*
 * Function:
 *      _bcm_esw_multicast_egress_subscriber_set
 * Purpose:
 *      Set the  replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port_max - (IN) Maximum ports
 *      port_array      - (IN) GPORT array
 *      encap_array  - (IN) Encap ID array.
 *      subscriber_array - (IN) Subscriber array
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_esw_multicast_egress_subscriber_set(int unit, bcm_multicast_t group,
                                         int port_count, 
                                         bcm_gport_t *port_array,
                                         bcm_if_t *encap_id_array,
                                         bcm_gport_t *subscriber_queue_array) 
{
    int mc_index, i, rv = BCM_E_NONE;
    int gport_id, *temp_encap_list = NULL;
    bcm_port_t *local_port_array = NULL;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_pbmp_t l2_pbmp;
    l2mc_entry_t l2mc_entry; 
    bcm_trunk_t trunk_id;
    soc_mem_t   mem;
    int modid_local = 0;
#if defined (INCLUDE_L3)
    int list_count, is_vpls = 0;
    bcm_pbmp_t mc_eligible_pbmp, e_pbmp;
    bcm_port_t port_iter;
    int q_id = 0;
    int remap_index;
    int q_count = 0;
    int q_count_increased = 0;
#endif /* INCLUDE_L3 */
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif

    /* Input parameters check. */
    mc_index = _BCM_MULTICAST_ID_GET(group);
    mem = (_BCM_MULTICAST_L2_OR_FABRIC(unit, group)) ? L2MCm : L3_IPMCm; 
    
    /* BCM_XGS3_FABRIC_SUPPORT */
    if ((mc_index < 0) || (mc_index >= soc_mem_index_count(unit, mem))) {
        return (BCM_E_PARAM);
    }

    if (port_count > 0) {
        if (NULL == port_array) {
            return (BCM_E_PARAM);
        }
        if (!_BCM_MULTICAST_L2_OR_FABRIC(unit, group) && 
            (NULL == encap_id_array || NULL == subscriber_queue_array)) {
            return (BCM_E_PARAM);
        }
    } else {
        return (BCM_E_PARAM);
    }   

#if defined (INCLUDE_L3)
    rv = _bcm_esw_multicast_l3_group_check(unit, group, NULL);
#endif /* INCLUDE_L3 */
    if (BCM_FAILURE(rv) && (0 == _BCM_MULTICAST_IS_L2(group))) {
         return BCM_E_PARAM;
    }

    /* Convert GPORT array into local port numbers */
    if (port_count > 0) {
        local_port_array =
            sal_alloc(sizeof(bcm_port_t) * port_count, "local_port array");
        if (NULL == local_port_array) {
            BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(BCM_E_MEMORY);
        }
        
        temp_encap_list =
            sal_alloc(sizeof(int) * port_count, "temp_encap_list");
        if (NULL == temp_encap_list) {
            BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
        }
    }

    for (i = 0; i < port_count ; i++) {
        rv = _bcm_esw_gport_resolve(unit, port_array[i],
                                    &mod_out, &port_out,
                                    &trunk_id, &gport_id); 
        if (BCM_FAILURE(rv)) {
            BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
        }
#if defined(BCM_KATANA2_SUPPORT)
        is_local_subport = FALSE;
        if (soc_feature(unit, soc_feature_linkphy_coe) ||
            soc_feature(unit, soc_feature_subtag_coe)) {
            if (BCM_GPORT_IS_SET(port_array[i]) &&
                _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                    port_array[i])) {
                rv = bcm_esw_stk_my_modid_get(unit, &my_modid);
                if (BCM_FAILURE(rv)) {
                    BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
                }
                if (mod_out == (my_modid + 1)) {
                    pp_port = BCM_GPORT_SUBPORT_PORT_GET(port_array[i]);
                    if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                        (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                        is_local_subport = TRUE;
                    }
                }
                if (is_local_subport == FALSE) {
                    BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(BCM_E_PORT);
                }
            }
        }
        if (is_local_subport) {
            local_port_array[i] = pp_port;
        } else
#endif

        if (BCM_TRUNK_INVALID != trunk_id) {
            BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(BCM_E_PARAM);
        } else {
            rv = _bcm_esw_modid_is_local(unit, mod_out, &modid_local);
            if (BCM_FAILURE(rv)) {
                BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
            }

            if ((0 == IS_E_PORT(unit, port_out)) &&
                !(_BCM_MULTICAST_IS_L2(group) ||
                  (BCM_IF_INVALID == encap_id_array[i]))) {
                /* Stack ports should not perform interface replication */
                BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(BCM_E_PARAM);
            }
            if (TRUE != modid_local) {
                BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(BCM_E_PARAM);
            }
            local_port_array[i] = port_out;
        }
    }

    BCM_PBMP_CLEAR(l2_pbmp);
    if (_BCM_MULTICAST_L2_OR_FABRIC(unit, group)) {
        for (i = 0; i < port_count; i++) {
            BCM_PBMP_PORT_ADD(l2_pbmp, local_port_array[i]);
        }

        /* Update the L2MC port bitmap */
        soc_mem_lock(unit, L2MCm);
        rv = soc_mem_read(unit, L2MCm, MEM_BLOCK_ANY, mc_index, &l2mc_entry);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, L2MCm);
            BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
        }
        soc_mem_pbmp_field_set(unit, L2MCm, &l2mc_entry, PORT_BITMAPf,
                               &l2_pbmp);
        rv = soc_mem_write(unit, L2MCm, MEM_BLOCK_ALL,
                           mc_index, &l2mc_entry);
        soc_mem_unlock(unit, L2MCm);
#if defined(INCLUDE_L3)
    } else {
        if (_BCM_MULTICAST_IS_VPLS(group)) {
             is_vpls = 1;
        }

        BCM_PBMP_CLEAR(e_pbmp);
        BCM_PBMP_ASSIGN(e_pbmp, PBMP_E_ALL(unit));
        /* 
         * For each port, walk through the list of GPORTs
         * and collect the ones that match the port.
         */
        /* Valid SOC ports plus the CMIC port */
        BCM_PBMP_ASSIGN(mc_eligible_pbmp, PBMP_PORT_ALL(unit));
        BCM_PBMP_PORT_ADD(mc_eligible_pbmp, CMIC_PORT(unit));
#if defined(BCM_KATANA2_SUPPORT)
        if (soc_feature(unit, soc_feature_subtag_coe) ||
            soc_feature(unit, soc_feature_linkphy_coe)) {
            _bcm_kt2_subport_pbmp_update(unit, &mc_eligible_pbmp);
            _bcm_kt2_subport_pbmp_update(unit, &e_pbmp);
        }
#endif

        BCM_PBMP_ITER(mc_eligible_pbmp, port_iter) {
            list_count = 0;
            for (i = 0; i < port_count ; i++) {
                   if (local_port_array[i] != port_iter) {
                     continue;
                 } 
                 if (encap_id_array[i] == BCM_IF_INVALID) {
                     if (!is_vpls) {
                          /* Add port to L2 pbmp */
                          BCM_PBMP_PORT_ADD(l2_pbmp, port_iter);
                     } else {
                              rv = BCM_E_PARAM;
                            BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
                     }
                   } else if (0 == BCM_PBMP_MEMBER(e_pbmp, port_iter)) {
                        /* Must be a front-panel port to have an
                                      * interface replication list. */
                        BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(BCM_E_PARAM);
                    } else {
                        if (!list_count) {
                            q_id = subscriber_queue_array[i];
                        } else {
                            if (q_id != subscriber_queue_array[i]) {
                                BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(BCM_E_PARAM);
                            }
                        }
                        temp_encap_list[list_count] = encap_id_array[i];
                        list_count++;
                    }
                }
                
                if (list_count) {
                    rv = _bcm_esw_ipmc_mmu_mc_remap_ptr(unit, mc_index, &remap_index, FALSE);
                    if (BCM_FAILURE(rv)) {
                        BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
                    }

                    q_count_increased = 0;
                    rv = _bcm_esw_ipmc_subscriber_egress_intf_set(unit,
                             remap_index, port_iter, list_count,
                             temp_encap_list,q_id, TRUE, &q_count,
                             &q_count_increased);
                    if (BCM_FAILURE(rv)) {
                        BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
                    } else {
                        if (q_count_increased) {
                            _bcm_kt_ipmc_port_ext_queue_count_increment(unit,
                                mc_index, port_iter);
                        }
                        rv = _bcm_esw_ipmc_set_remap_group(unit, mc_index, 
                                                         port_iter, q_count); 
                        if (BCM_FAILURE(rv)) {
                            BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
                        } 
                    }
                }
            }
#endif  /* INCLUDE_L3 */
    }

    BCM_SUBSCRIBER_MULTICAST_MEM_FREE_RETURN(rv);
}

/*
 * Function:
 *      _bcm_esw_multicast_egress_subscriber_get
 * Purpose:
 *      Get the  replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port_max - (IN) Maximum ports
 *      port_array      - (OUT) GPORT array
 *      encap_array  - (OUT) Encap ID array.
 *      subscriber_array - (OUT) Subscriber array
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_esw_multicast_egress_subscriber_get(int unit, bcm_multicast_t group,
                                         int port_max, bcm_gport_t *port_array,
                                         bcm_if_t *encap_id_array,
                                         bcm_gport_t *queue_array,
                                         int *port_count)
{
    int             mc_index, i, rv;
    int             count = 0;
    bcm_port_t      port;
    int             remap_index;

    i= 0;
    rv = BCM_E_NONE;

    mc_index = _BCM_MULTICAST_ID_GET(group);
    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

#if defined (INCLUDE_L3)
    BCM_IF_ERROR_RETURN(_bcm_esw_multicast_l3_group_check(unit, group, NULL));
#endif /* INCLUDE_L3 */ 
    if (port_max <= 0) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
    (_bcm_esw_ipmc_mmu_mc_remap_ptr(unit, mc_index, &remap_index, FALSE));
    BCM_IF_ERROR_RETURN
    (_bcm_esw_ipmc_subscriber_egress_intf_get(unit, remap_index, port_max, 
                                    port_array, encap_id_array, 
                                    queue_array, &count)); 

    *port_count = count;    
    for (i = 0; i < count; i++) {
        if (NULL != port_array && NULL != queue_array) {
           /* Convert to GPORT values */
#ifdef BCM_KATANA2_SUPPORT
             if (SOC_IS_KATANA2(unit)) {
                 BCM_IF_ERROR_RETURN
                     (bcm_kt2_cosq_port_get(unit, queue_array[i], &port));
             } else 
#endif /* BCM_KATANA2_SUPPORT */
             {
                 BCM_IF_ERROR_RETURN
                     (bcm_kt_cosq_port_get(unit, queue_array[i], &port));
             }
             BCM_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_SET(queue_array[i],
                                                 queue_array[i]);
             queue_array[i] |= (port << 16);
             BCM_IF_ERROR_RETURN
                (bcm_esw_port_gport_get(unit, port, (port_array + i)));
        }
        if (i == port_max) {
            *port_count = port_max;
            break;
        }
    }

    return rv;    
}

/*
 * Function:
 *      _bcm_esw_multicast_egress_subscriber_add
 * Purpose:
 *      Add a GPORT to the replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 *      subscriber_queue - (IN) Subscriber gport
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_esw_multicast_egress_subscriber_add(int unit, bcm_multicast_t group,
                                         bcm_gport_t port,
                                         bcm_if_t encap_id,
                                         bcm_gport_t subscriber_queue)
{
    int mc_index, is_l3, gport_id;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int modid_local, rv = BCM_E_NONE; 
    bcm_pbmp_t l2_pbmp, l3_pbmp, old_pbmp, *new_pbmp;
    int remap_index;
    int q_count = 0;
    int q_count_increased = 0;
    int q_count_decreased = 0;
    int if_max = BCM_MULTICAST_PORT_MAX;
#if defined(BCM_ENDURO_SUPPORT)
    uint16              dev_id;
    uint8               rev_id;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif

    if (SOC_IS_ENDURO(unit)) {
        soc_cm_get_id(unit, &dev_id, &rev_id);

        if ((dev_id == BCM56230_DEVICE_ID) || (dev_id == BCM56231_DEVICE_ID)){
            /* 
             * Because the size of "L3 Next Hop Table" 
             * is reduced to 2K only on Dagger (Enduro Variant), 
             * the if_max needs to be changed to the size of "L3 Next Hop 
             * Table (2K)" instead of "BCM_MULTICAST_PORT_MAX(4K)".
             */ 
            if_max = soc_mem_index_count(unit, ING_L3_NEXT_HOPm);
        }
    }
#endif /* BCM_ENDURO_SUPPORT */

    is_l3 = 0;      
    modid_local = FALSE;

    mc_index = _BCM_MULTICAST_ID_GET(group);

    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

    if (!BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(subscriber_queue)) {
        return BCM_E_PARAM;
    }

#if defined (INCLUDE_L3)
    BCM_IF_ERROR_RETURN(_bcm_esw_multicast_l3_group_check(unit, group, &is_l3));
#endif /* INCLUDE_L3 */

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, 
                                &trunk_id, &gport_id));
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        if (BCM_GPORT_IS_SET(port) &&
            _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit, port)) {
            if (mod_out == (my_modid + 1)) {
                pp_port = BCM_GPORT_SUBPORT_PORT_GET(port);
                if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                    (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport == FALSE) {
                return BCM_E_PORT;
            }
        }
    }
    if (is_local_subport) {
        port_out = pp_port;
    } else
#endif

    if (BCM_TRUNK_INVALID != trunk_id) {
        return BCM_E_PARAM;
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &modid_local));

        if ((!IS_E_PORT(unit, port_out)) && (BCM_IF_INVALID != encap_id)) {
            /* Stack ports should not perform interface replication */
            return BCM_E_PORT;
        }
        if (TRUE != modid_local) {
            /* Only add this to replication set if destination is local */
            return BCM_E_PORT;
        }
        /* Convert system port to physical port */
        if (soc_feature(unit, soc_feature_sysport_remap)) {
            BCM_XLATE_SYSPORT_S2P(unit, &port_out);
        }
    }

    if (BCM_IF_INVALID != encap_id) {
        /* Add  the required port to the multicast egress interface */
        BCM_IF_ERROR_RETURN(_bcm_esw_ipmc_mmu_mc_remap_ptr(unit,
            mc_index, &remap_index, FALSE));

        rv = _bcm_esw_ipmc_subscriber_egress_intf_add(unit, remap_index,
                 port_out, encap_id, subscriber_queue, is_l3, &q_count,
                 &q_count_increased);
        if (BCM_SUCCESS(rv)) {
            new_pbmp = &l3_pbmp;
#if defined(INCLUDE_L3)
            if (q_count_increased) {
                _bcm_kt_ipmc_port_ext_queue_count_increment(unit,
                    mc_index, port_out);
            }
#endif
        } else {
            return rv;
        }
    } else {
        if (!_BCM_MULTICAST_IS_VPLS(group)) {
            /* Updating the L2 bitmap */
            new_pbmp = &l2_pbmp;
        } else {
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_ipmc_set_remap_group(unit, mc_index, port_out, q_count));

    rv = _bcm_esw_multicast_ipmc_read(unit, mc_index, &l2_pbmp, &l3_pbmp);

    if (BCM_SUCCESS(rv) && (BCM_IF_INVALID == encap_id)) {
        BCM_PBMP_ASSIGN(old_pbmp, *new_pbmp);
        BCM_PBMP_PORT_ADD(*new_pbmp, port_out);
        
        if (BCM_PBMP_NEQ(old_pbmp, *new_pbmp)) {

            /* Inside ipmc_write update num_copies, int, ext and 
             * redirection pointer need to manage the work around
             */
            rv = _bcm_esw_multicast_ipmc_write(unit, mc_index, l2_pbmp, 
                                               l3_pbmp, TRUE);
        }
    }

    if (BCM_FAILURE(rv)) {
        (void) _bcm_esw_ipmc_subscriber_egress_intf_delete(unit, mc_index, 
                                   port_out, if_max, 
                                   encap_id, is_l3, &q_count,
                                   &q_count_decreased);
    }

    return rv;
}


/*
 * Function:
 *      _bcm_esw_multicast_egress_subscriber_delete
 * Purpose:
 *      Delete a GPORT from the replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 *      subscriber_queue - (IN) Subscriber gport
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_esw_multicast_egress_subscriber_delete(int unit, bcm_multicast_t group,
                                            bcm_gport_t port,
                                            bcm_if_t encap_id,
                                            bcm_gport_t subscriber_queue) 
{
    int mc_index, is_l3, gport_id, i;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int modid_local, local_count, port_count, rv = BCM_E_NONE; 
    bcm_port_t    temp_port;    
    bcm_pbmp_t l2_pbmp, l3_pbmp, old_pbmp;
    bcm_gport_t *port_array;
    bcm_if_t *encap_array;
    int *q_array;
    int remap_index;
    int q_count = 0;
    int q_count_decreased = 0;
    int if_max = BCM_MULTICAST_PORT_MAX;
#if defined(BCM_ENDURO_SUPPORT)
    uint16              dev_id;
    uint8               rev_id;
#if defined(BCM_KATANA2_SUPPORT)
    int is_local_subport = FALSE, my_modid = 0, pp_port;
#endif

    if (SOC_IS_ENDURO(unit)) {
        soc_cm_get_id(unit, &dev_id, &rev_id);

        if ((dev_id == BCM56230_DEVICE_ID) || (dev_id == BCM56231_DEVICE_ID)){
            /* 
             * Because the size of "L3 Next Hop Table" 
             * is reduced to 2K only on Dagger (Enduro Variant), 
             * the if_max needs to be changed to the size of "L3 Next Hop 
             * Table (2K)" instead of "BCM_MULTICAST_PORT_MAX(4K)".
             */ 
            if_max = soc_mem_index_count(unit, ING_L3_NEXT_HOPm);
        }
    }
#endif /* BCM_ENDURO_SUPPORT */

    is_l3 = 0;      
    modid_local = FALSE;
    port_count = local_count = 0;

    mc_index = _BCM_MULTICAST_ID_GET(group);

    if ((mc_index < 0) ||
        (mc_index >= soc_mem_index_count(unit, L3_IPMCm))) {
        return BCM_E_PARAM;
    }

#if defined (INCLUDE_L3)
    BCM_IF_ERROR_RETURN(_bcm_esw_multicast_l3_group_check(unit, group, &is_l3));
#endif /* INCLUDE_L3 */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, 
                                &trunk_id, &gport_id)); 
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        if (BCM_GPORT_IS_SET(port) &&
            _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit, port)) {
            if (mod_out == (my_modid + 1)) {
                pp_port = BCM_GPORT_SUBPORT_PORT_GET(port);
                if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                    (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport == FALSE) {
                return BCM_E_PORT;
            }
        }
    }
    if (is_local_subport) {
        port_out = pp_port;
    } else
#endif

    if (BCM_TRUNK_INVALID != trunk_id) {
        return BCM_E_PARAM;
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &modid_local));

        if ((!IS_E_PORT(unit, port_out)) && (BCM_IF_INVALID != encap_id)) {
            /* Stack ports should not perform interface replication */
            return BCM_E_PORT;
        }
        if (TRUE != modid_local) {
            /* Only add this to replication set if destination is local */
            return BCM_E_PORT;
        }
        /* Convert system port to physical port */
        if (soc_feature(unit, soc_feature_sysport_remap)) {
            BCM_XLATE_SYSPORT_S2P(unit, &port_out);
        }
    }

    if (encap_id == BCM_IF_INVALID) {
        /* Remove L2 port */
        if (_BCM_MULTICAST_IS_VPLS(group)) {
            return BCM_E_PARAM;
        }

        soc_mem_lock(unit, L3_IPMCm);
        /* Delete the port from the IPMC L2_BITMAP */
        rv = _bcm_esw_multicast_ipmc_read(unit, mc_index, 
                                          &l2_pbmp, &l3_pbmp);
        if (BCM_SUCCESS(rv)) {
            BCM_PBMP_ASSIGN(old_pbmp, l2_pbmp);

            BCM_PBMP_PORT_REMOVE(l2_pbmp, port_out);

            if (BCM_PBMP_NEQ(old_pbmp, l2_pbmp)) {
                rv = _bcm_esw_multicast_ipmc_write(unit, mc_index,
                                        l2_pbmp, l3_pbmp, TRUE);
            } else {
                rv = BCM_E_NOT_FOUND;
            }
        }
        soc_mem_unlock(unit, L3_IPMCm);
        return rv;
    }

    /*
     * Walk through the list of egress interfaces for this group.
     * Check if the interface getting deleted is the ONLY
     * instance of the local port in the replication list.
     * If so, we can delete the port from the IPMC L3_BITMAP.
     */
    port_array = sal_alloc(if_max * sizeof(bcm_gport_t),
                           "mcast port array");
    if (port_array == NULL) {
        return BCM_E_MEMORY;
    }
    encap_array = sal_alloc(if_max * sizeof(bcm_if_t),
                            "mcast encap array");
    if (encap_array == NULL) {
        sal_free (port_array);
        return BCM_E_MEMORY;
    }
    q_array = sal_alloc(if_max * sizeof(bcm_if_t),
                            "mcast q array");
    if (q_array == NULL) {
        sal_free (port_array);
        sal_free (encap_array);
        return BCM_E_MEMORY;
    }

    sal_memset(q_array, 0, (if_max * sizeof(bcm_if_t)));
    rv = bcm_esw_multicast_egress_get(unit, group, if_max, 
                                      port_array, encap_array, &port_count);

    if (BCM_FAILURE(rv)) {
        sal_free (port_array);
        sal_free (encap_array);
        sal_free (q_array);
        return (rv);
    } 

    for (i = 0; i < port_count ; i++) {
        if (encap_id == encap_array[i]) {
            /* Skip the one we're about to delete */
            continue;
        }
#if defined(BCM_KATANA2_SUPPORT)
        if (soc_feature(unit, soc_feature_linkphy_coe) ||
            soc_feature(unit, soc_feature_subtag_coe)) {
            is_local_subport = FALSE;
            if (BCM_GPORT_IS_SET(port_array[i]) &&
                _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                port_array[i])) {
                is_local_subport = TRUE;
            }
        }
        if (is_local_subport) {
            temp_port = _BCM_KT2_SUBPORT_PORT_ID_GET(port_array[i]);
        } else
#endif
        {
            (void) bcm_esw_port_local_get(unit, port_array[i], &temp_port);
        }

        if (temp_port == port_out) {
            local_count = 1;
            break;
        } 
    }

    /* verify if there are any other interfaces on this port 
     * from extended queue list */

    if (!local_count) {
        rv = _bcm_esw_multicast_egress_subscriber_get(unit, group, 
                                          (if_max - 1), 
                                          port_array, encap_array, 
                                          q_array, &port_count);
        if (BCM_FAILURE(rv)) {
            sal_free (port_array);
            sal_free (encap_array);
            sal_free (q_array);
            return (rv);
        } 

        for (i = 0; i < port_count ; i++) {
            if (encap_id == encap_array[i]) {
                /* Skip the one we're about to delete */
                continue;
            }
#if defined(BCM_KATANA2_SUPPORT)
            if (soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) {
                is_local_subport = FALSE;
                if (BCM_GPORT_IS_SET(port_array[i]) &&
                    _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                    port_array[i])) {
                    is_local_subport = TRUE;
                }
            }
            if (is_local_subport) {
                temp_port = _BCM_KT2_SUBPORT_PORT_ID_GET(port_array[i]);
            } else
#endif
            {
                (void) bcm_esw_port_local_get(unit, port_array[i], &temp_port);
            }

            if (temp_port == port_out) {
                local_count = 1;
                break;
            } 
        }
    }
        
    sal_free(port_array);
    sal_free(encap_array);
    sal_free (q_array);

    BCM_IF_ERROR_RETURN
        (_bcm_esw_ipmc_mmu_mc_remap_ptr(unit, mc_index, &remap_index, FALSE));
   
    rv = _bcm_esw_ipmc_subscriber_egress_intf_delete(unit,
             remap_index, port_out, encap_id, subscriber_queue,
             is_l3, &q_count, &q_count_decreased);
    if (BCM_SUCCESS(rv)) {
#if defined(INCLUDE_L3)
        if (q_count_decreased) {
            _bcm_kt_ipmc_port_ext_queue_count_decrement(unit,
                mc_index, port_out);
        }
#endif
        BCM_IF_ERROR_RETURN(_bcm_esw_ipmc_set_remap_group(unit, mc_index, 
                                                        port_out, q_count)); 
    }
   
    return rv;
}

#endif /* BCM_KATANA_SUPPORT */

/*
 * Function:
 *      bcm_esw_multicast_egress_subscriber_add
 * Purpose:
 *      Add a GPORT to the replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 *      subscriber_queue - (IN) Subscriber gport
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_egress_subscriber_add(int unit, bcm_multicast_t group,
                                        bcm_gport_t port,
                                        bcm_if_t encap_id,
                                        bcm_gport_t subscriber_queue)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_KATANAX(unit)) {    
       return _bcm_esw_multicast_egress_subscriber_add(unit, group, port, 
                                                       encap_id, subscriber_queue);
    }
#endif /* BCM_KATANA_SUPPORT && INCLUDE_L3 */
        return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_multicast_egress_subscriber_delete
 * Purpose:
 *      Delete a GPORT from the replication list
 *      for the specified multicast index.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      port      - (IN) GPORT Identifier
 *      encap_id  - (IN) Encap ID.
 *      subscriber_queue - (IN) Subscriber gport
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_multicast_egress_subscriber_delete(int unit, bcm_multicast_t group,
                                           bcm_gport_t port,
                                           bcm_if_t encap_id,
                                           bcm_gport_t subscriber_queue) 
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_KATANAX(unit)) {    
        return _bcm_esw_multicast_egress_subscriber_delete(unit, group, port, 
                                                    encap_id, subscriber_queue);
    }
#endif /* BCM_KATANA_SUPPORT && INCLUDE_L3 */
            return BCM_E_UNAVAIL;

}

/*  
 * Function:
 *      bcm_esw_multicast_egress_subscriber_set
 * Purpose:
 *      Assign the complete set of egress GPORTs in the
 *      replication list for the specified multicast index.
 * Parameters:
 *      unit       - (IN) Device Number
 *      group      - (IN) Multicast group ID
 *      port_count - (IN) Number of ports in replication list
 *      port_array - (IN) List of GPORT Identifiers
 *      encap_id_array - (IN) List of encap identifiers
 *      subscriber_queue_array - (IN) Subscriber array
 * Returns:
 *      BCM_E_XXX
 */     
int
bcm_esw_multicast_egress_subscriber_set(int unit, bcm_multicast_t group,
                                        int port_count, bcm_gport_t *port_array,
                                        bcm_if_t *encap_id_array,
                                        bcm_gport_t *subscriber_queue_array) 
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_L3)
        if (SOC_IS_KATANAX(unit)) {            
            return _bcm_esw_multicast_egress_subscriber_set(unit, group, 
                                                    port_count, port_array, 
                                                    encap_id_array, 
                                                    subscriber_queue_array);
        }
#endif /* BCM_KATANA_SUPPORT && INCLUDE_L3 */
        return BCM_E_UNAVAIL;  

}

/*  
 * Function:
 *      bcm_esw_multicast_egress_subscriber_get
 * Purpose:
 *      Get the complete set of egress GPORTs in the
 *      replication list for the specified multicast index.
 * Parameters:
 *      unit       - (IN) Device Number
 *      group      - (IN) Multicast group ID
 *      port_count - (IN) Number of ports in replication list
 *      port_array - (OUT) List of GPORT Identifiers
 *      encap_id_array - (OUT) List of encap identifiers
  *      subscriber_queue_array - (OUT) Subscriber array
 * Returns:
 *      BCM_E_XXX
 */ 
int
bcm_esw_multicast_egress_subscriber_get(int unit, bcm_multicast_t group,
                                        int port_max,
                                        bcm_gport_t *port_array,
                                        bcm_if_t *encap_id_array,
                                        bcm_gport_t *subscriber_queue_array,
                                        int *port_count)
{
    MULTICAST_INIT_CHECK(unit);

#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_L3)
    if (SOC_IS_KATANAX(unit)) {            
        return _bcm_esw_multicast_egress_subscriber_get(unit, group, 
                                                        port_max, port_array, 
                                                        encap_id_array, 
                                                        subscriber_queue_array,
                                                        port_count);
    }
#endif /* BCM_KATANA_SUPPORT && INCLUDE_L3 */
            return BCM_E_UNAVAIL;  
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *      _bcm_esw_multicast_sw_dump_cb
 * Purpose:
 *      Callback to print Multicast info
 * Parameters:
 *      unit       - (IN)   Device Number
 *      group      - (IN)   Group ID
 *      flags      - (OUT)  BCM_MULTICAST_*
 *      user_data  - (IN)   User data to be passed to callback function.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_multicast_sw_dump_cb(int unit, bcm_multicast_t group, 
                              uint32 flags, void *user_data)
{
    LOG_CLI((BSL_META_U(unit,
                        "    0x%08x    0x%08x     "), group, flags));
    if (flags & BCM_MULTICAST_TYPE_L2) {
        LOG_CLI((BSL_META_U(unit,
                            " L2")));
    }
    if (flags & BCM_MULTICAST_TYPE_L3) {
        LOG_CLI((BSL_META_U(unit,
                            " L3")));
    }
    if (flags & BCM_MULTICAST_TYPE_VPLS) {
        LOG_CLI((BSL_META_U(unit,
                            " VPLS")));
    }
    if (flags & BCM_MULTICAST_TYPE_SUBPORT) {
        LOG_CLI((BSL_META_U(unit,
                            " SUBPORT")));
    }
    if (flags & BCM_MULTICAST_TYPE_MIM) {
        LOG_CLI((BSL_META_U(unit,
                            " MIM")));
    }
    if (flags & BCM_MULTICAST_TYPE_WLAN) {
        LOG_CLI((BSL_META_U(unit,
                            " WLAN")));
    }
    if (flags & BCM_MULTICAST_TYPE_VLAN) {
        LOG_CLI((BSL_META_U(unit,
                            " VLAN")));
    }
    if (flags & BCM_MULTICAST_TYPE_TRILL) {
        LOG_CLI((BSL_META_U(unit,
                            " TRILL")));
    }
    if (flags & BCM_MULTICAST_TYPE_NIV) {
        LOG_CLI((BSL_META_U(unit,
                            " NIV")));
    }
    if (flags &  BCM_MULTICAST_TYPE_EGRESS_OBJECT) {
        LOG_CLI((BSL_META_U(unit,
                            " MPLS P2MP")));
    }
    if (flags &  BCM_MULTICAST_TYPE_L2GRE) {
        LOG_CLI((BSL_META_U(unit,
                            " L2GRE")));
    }
    if (flags &  BCM_MULTICAST_TYPE_VXLAN) {
        LOG_CLI((BSL_META_U(unit,
                            " VXLAN")));
    }
    if (flags & BCM_MULTICAST_TYPE_EXTENDER) {
        LOG_CLI((BSL_META_U(unit,
                            " EXTENDER")));
    }
    if (flags & BCM_MULTICAST_WITH_ID) {
        LOG_CLI((BSL_META_U(unit,
                            " ID")));
    }
    LOG_CLI((BSL_META_U(unit,
                        "\n")));
    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_multicast_sw_dump
 * Purpose:
 *     Displays Multicast information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 * Note:
 *    The multicast info is really handled by other modules, but
 *    this is a handy summary of the information.
 *    Dump of l2_data is skipped.
 */
void
_bcm_multicast_sw_dump(int unit)
{
    int rv;

    LOG_CLI((BSL_META_U(unit,
                        "\nSW Information Multicast - Unit %d\n"), unit));
    LOG_CLI((BSL_META_U(unit,
                        "    Initialized: %d\n"), multicast_initialized[unit]));
    LOG_CLI((BSL_META_U(unit,
                        "    Groups:       Flag value:     Flags:\n")));

    rv = bcm_esw_multicast_group_traverse(unit,
                                          &_bcm_esw_multicast_sw_dump_cb,
                                          BCM_MULTICAST_TYPE_MASK, NULL);

    if (BCM_FAILURE(rv)) {
        LOG_CLI((BSL_META_U(unit,
                            "\n  *** Multicast traverse error ***: %s\n"),
                 bcm_errmsg(rv)));
    }

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
#if defined(INCLUDE_L3)
    if (SOC_REG_IS_VALID(unit, IPMC_L3_MTUr) && (NULL != _bcm_mtu_profile)) {
        int i, num_entries, ref_count;
        uint64 rval64, *rval64s[1];

        rval64s[0] = &rval64;
        num_entries = SOC_REG_NUMELS(unit, IPMC_L3_MTUr);
        LOG_CLI((BSL_META_U(unit,
                            "  IPMC_L3_MTU\n")));
        LOG_CLI((BSL_META_U(unit,
                            "    Number of entries: %d\n"), num_entries));
        LOG_CLI((BSL_META_U(unit,
                            "    Index RefCount -  IPMC_L3_MTU\n")));
        
        for (i = 0; i < num_entries; i++) {
            rv = soc_profile_reg_ref_count_get(unit,
                                               _bcm_mtu_profile[unit],
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                LOG_CLI((BSL_META_U(unit,
                                    " *** Error retrieving profile reference: %d ***\n"),
                         rv));
                break;
            }
            if (ref_count <= 0) {
                continue;
            }
            rv = soc_profile_reg_get(unit, _bcm_mtu_profile[unit], i,
                                     1, rval64s);
            if (SOC_FAILURE(rv)) {
                LOG_CLI((BSL_META_U(unit,
                                    " *** Error retrieving profile value: %d ***\n"),
                         rv));
                break;
            }
            LOG_CLI((BSL_META_U(unit,
                                "  %5d %8d       0x%08x\n"),
                     i, ref_count, COMPILER_64_LO(rval64)));
        }
        LOG_CLI((BSL_META_U(unit,
                            "\n")));
    }
#endif /* INCLUDE_L3 */
#endif

#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3) 
    if (soc_feature(unit, soc_feature_virtual_port_routing)) {
        bcm_td2_multicast_l3_vp_sw_dump(unit);
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
