/*
 * $Id: l2cache.c,v 1.80 Broadcom SDK $
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
 * L2 Cache - Layer 2 BPDU and overflow address cache
 */

#include <sal/core/libc.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/l2u.h>

#include <bcm/l2.h>
#include <bcm/error.h>

#include <bcm_int/esw/port.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/vpn.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/l2.h>
#include <bcm_int/esw/stack.h>
#ifdef BCM_TRIDENT_SUPPORT
#include <bcm_int/esw/trident.h>
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */

#include <bcm_int/esw_dispatch.h>

#ifdef BCM_XGS3_SWITCH_SUPPORT

/*
 * Function:
 *      _bcm_l2_cache_gport_resolve
 * Purpose:
 *      Convert destination in a GPORT format to modid port pair or trunk id
 * Parameters:
 *      unit - Unit number
 *      l2caddr - (IN/OUT) Hardware-independent L2 entry
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_l2_cache_gport_resolve(int unit, bcm_l2_cache_addr_t *l2caddr)
{
    bcm_port_t      port;
    bcm_module_t    modid;
    bcm_trunk_t     tgid;
    int             id;


    if (BCM_GPORT_IS_SET(l2caddr->dest_port)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit, l2caddr->dest_port, 
                                                   &modid, &port, &tgid, &id));

#if defined(BCM_KATANA2_SUPPORT)
        if (_BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
            l2caddr->dest_port)) {
        } else
#endif
        {
            /* MPLS MIM and WLAN GPORTS are not currently supported. */
            if (-1 != id) {
                return (BCM_E_UNAVAIL);
            }
        }
        /* Trunk GPORT */
        if (BCM_TRUNK_INVALID != tgid) {
            l2caddr->flags |= BCM_L2_CACHE_TRUNK;
            l2caddr->dest_trunk = tgid;
        } else {    /* MODPORT GPORT */
            l2caddr->dest_port = port;
            l2caddr->dest_modid = modid;
        }
    }
    if (BCM_GPORT_IS_SET(l2caddr->src_port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, l2caddr->src_port, 
                                                   &port));
        l2caddr->src_port = port;
    }


    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_l2_cache_gport_construct
 * Purpose:
 *      Constract a GPORT destination format from modid port pair or trunk id
 * Parameters:
 *      unit - Unit number
 *      l2caddr - (IN/OUT) Hardware-independent L2 entry
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_l2_cache_gport_construct(int unit, bcm_l2_cache_addr_t *l2caddr)
{
    bcm_gport_t         gport;
    _bcm_gport_dest_t   gport_dst;

     _bcm_gport_dest_t_init(&gport_dst);

     if (l2caddr->flags & BCM_L2_CACHE_TRUNK) {
         gport_dst.gport_type = BCM_GPORT_TYPE_TRUNK;
         gport_dst.tgid = l2caddr->dest_trunk;
     } else {
         gport_dst.gport_type = BCM_GPORT_TYPE_MODPORT;
         gport_dst.modid = l2caddr->dest_modid;
         gport_dst.port = l2caddr->dest_port;
     }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_gport_construct(unit, &gport_dst, &gport));

    l2caddr->dest_port = gport;

    gport_dst.gport_type = BCM_GPORT_TYPE_MODPORT;
    gport_dst.port = l2caddr->src_port;
    BCM_IF_ERROR_RETURN(
        bcm_esw_stk_my_modid_get(unit, &gport_dst.modid));
    if (gport_dst.modid < 0) {
        return BCM_E_UNAVAIL;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_gport_construct(unit, &gport_dst, &gport));

    l2caddr->src_port = gport;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_l2_from_l2u
 * Purpose:
 *      Convert L2 User table entry to a hardware-independent L2 cache entry.
 * Parameters:
 *      unit - Unit number
 *      l2caddr - (OUT) Hardware-independent L2 entry
 *      l2u_entry - Firebolt L2 User entry
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_l2_cache_from_l2u(int unit, 
                       bcm_l2_cache_addr_t *l2caddr, l2u_entry_t *l2u_entry)
{
    l2u_entry_t     l2u_mask_entry;
    bcm_module_t    mod_in = 0, mod_out;
    bcm_port_t      port_in = 0, port_out;
    uint32          mask[SOC_MAX_MEM_WORDS];
    int             skip_l2u, isGport;
    bcm_vlan_t      vlan;

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);

    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {

        if (!_l2u_field32_get(unit, l2u_entry, VALIDf)) {
            return BCM_E_NOT_FOUND;
        }

        sal_memset(l2caddr, 0, sizeof (*l2caddr));
        sal_memset(&l2u_mask_entry, 0, sizeof (l2u_mask_entry));

        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, KEYf)) {
            _l2u_field_get(unit, l2u_entry, MASKf, mask);
            _l2u_field_set(unit, &l2u_mask_entry, KEYf, mask);
        } else {
            _l2u_field_get(unit, l2u_entry, MASKf, (uint32 *)&l2u_mask_entry);
            /* key fields start from bit 1 */
            l2u_mask_entry.entry_data[2] = l2u_mask_entry.entry_data[1] >> 31; 
            l2u_mask_entry.entry_data[1] =
                (l2u_mask_entry.entry_data[1] << 1) | 
                (l2u_mask_entry.entry_data[0] >> 31); 
            l2u_mask_entry.entry_data[0] = l2u_mask_entry.entry_data[0] << 1;
        }

        _l2u_mac_addr_get(unit, l2u_entry, MAC_ADDRf, l2caddr->mac);
        _l2u_mac_addr_get(unit, &l2u_mask_entry, MAC_ADDRf, l2caddr->mac_mask);

        vlan = _l2u_field32_get(unit, l2u_entry, VLAN_IDf);
        l2caddr->vlan_mask = _l2u_field32_get(unit, &l2u_mask_entry, VLAN_IDf);

#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, KEY_TYPEf) &&
            _l2u_field32_get(unit, l2u_entry, KEY_TYPEf)) {
#ifdef BCM_MPLS_SUPPORT
            if (soc_feature(unit, soc_feature_mpls) &&
               (_bcm_vfi_used_get(unit, vlan, _bcmVfiTypeMpls))) {
                    _BCM_VPN_SET(vlan, _BCM_VPN_TYPE_VFI, vlan);
            } else
#endif /* BCM_MPLS_SUPPORT */
            {
                _BCM_VPN_SET(vlan, _BCM_VPN_TYPE_VFI, vlan);
            }
        }
#endif /* TRX && L3 */
        l2caddr->vlan = vlan;

        l2caddr->prio = _l2u_field32_get(unit, l2u_entry, PRIf);

        if (_l2u_field32_get(unit, l2u_entry, RPEf)) {
            l2caddr->flags |= BCM_L2_CACHE_SETPRI;
        }

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) \
 || defined(BCM_RAVEN_SUPPORT)
        if ((SOC_IS_FIREBOLT2(unit) || SOC_IS_TRX(unit)
             || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit)) &&
            _l2u_field32_get(unit, l2u_entry, DO_NOT_LEARN_MACSAf)) {
            l2caddr->flags |= BCM_L2_CACHE_LEARN_DISABLE;
        }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT || BCM_RAVEN_SUPPORT */

        if (_l2u_field32_get(unit, l2u_entry, CPUf)) {
            l2caddr->flags |= BCM_L2_CACHE_CPU;
        }

        if (_l2u_field32_get(unit, l2u_entry, BPDUf)) {
            l2caddr->flags |= BCM_L2_CACHE_BPDU;
        }

        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, L2_PROTOCOL_PKTf)) {
            if (_l2u_field32_get(unit, l2u_entry, L2_PROTOCOL_PKTf)) {
                l2caddr->flags |= BCM_L2_CACHE_PROTO_PKT;
            }
        }

        if (_l2u_field32_get(unit, l2u_entry, DST_DISCARDf)) {
            l2caddr->flags |= BCM_L2_CACHE_DISCARD;
        }

        if (soc_feature(unit, soc_feature_trunk_group_overlay)) {
            if (_l2u_field32_get(unit, l2u_entry, Tf)) {
                l2caddr->flags |= BCM_L2_CACHE_TRUNK;
                l2caddr->dest_trunk = _l2u_field32_get(unit, l2u_entry, TGIDf);
            } else {
                mod_in = _l2u_field32_get(unit, l2u_entry, MODULE_IDf);
                port_in = _l2u_field32_get(unit, l2u_entry, PORT_NUMf);
            }
        } else {
            mod_in = _l2u_field32_get(unit, l2u_entry, MODULE_IDf);
            port_in = _l2u_field32_get(unit, l2u_entry, PORT_TGIDf);
            if (port_in & BCM_TGID_TRUNK_INDICATOR(unit)) {
                l2caddr->flags |= BCM_L2_CACHE_TRUNK;
                l2caddr->dest_trunk = BCM_MODIDf_TGIDf_TO_TRUNK(unit, mod_in,
                                                                port_in);
            }
        }
        if (!(l2caddr->flags & BCM_L2_CACHE_TRUNK)) {
            if (!SOC_MODID_ADDRESSABLE(unit, mod_in)) {
                return BCM_E_BADID;
            }
            BCM_IF_ERROR_RETURN(_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                                       mod_in, port_in, &mod_out,
                                                       &port_out));
            l2caddr->dest_modid = mod_out;
            l2caddr->dest_port = port_out;
        }
        BCM_IF_ERROR_RETURN(
            bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));

        if (isGport) {
            BCM_IF_ERROR_RETURN(
                _bcm_l2_cache_gport_construct(unit, l2caddr));
        }

        if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f)) {
            if (_l2u_field32_get(unit, l2u_entry, L3f)) {
                l2caddr->flags |= BCM_L2_CACHE_L3;
            }
        } else if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f)) {
            if (_l2u_field32_get(unit, l2u_entry, RESERVED_0f)) {
                l2caddr->flags |= BCM_L2_CACHE_L3;
            }
        }

        if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, MIRRORf)) {
            if (_l2u_field32_get(unit, l2u_entry, MIRRORf)) {
                l2caddr->flags |= BCM_L2_CACHE_MIRROR;
            }
        }

#if defined(BCM_TRX_SUPPORT) 
        if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, CLASS_IDf)) {
            l2caddr->lookup_class = _l2u_field32_get(unit, l2u_entry, CLASS_IDf);
        }
#endif /* BCM_TRX_SUPPORT */

        return BCM_E_NONE;
    }
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_fb_l2_to_l2u
 * Purpose:
 *      Convert a hardware-independent L2 cache entry to a L2 User table entry.
 * Parameters:
 *      unit - Unit number
 *      l2u_entry - (OUT) Firebolt L2 User entry
 *      l2caddr - Hardware-independent L2 entry
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_l2_cache_to_l2u(int unit, 
                     l2u_entry_t *l2u_entry, bcm_l2_cache_addr_t *l2caddr)
{
    l2u_entry_t     l2u_mask_entry;
    bcm_module_t    mod_in, mod_out;
    bcm_port_t      port_in, port_out;
    uint32          mask[SOC_MAX_MEM_WORDS];
    soc_field_t     port_field = 0;
    int             skip_l2u, isGport;
    bcm_vlan_t      vlan;
    int             int_pri_max;

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);
    
    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {

        /* If VPN specified do not perform vlan id check */
        if (!_BCM_VPN_IS_SET(l2caddr->vlan)) {
            VLAN_CHK_ID(unit, l2caddr->vlan);
            VLAN_CHK_ID(unit, l2caddr->vlan_mask);
        }
        if (l2caddr->flags & BCM_L2_CACHE_SETPRI) {
            int_pri_max = (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? 15 : 7;
            if (l2caddr->prio < 0 || l2caddr->prio > int_pri_max) {
                return BCM_E_PARAM;
            }
        }

        if (BCM_GPORT_IS_SET(l2caddr->dest_port) ||
            BCM_GPORT_IS_SET(l2caddr->src_port)) {
            BCM_IF_ERROR_RETURN(_bcm_l2_cache_gport_resolve(unit, l2caddr));
            isGport = 1;
        } else {
            isGport = 0;
        }

        sal_memset(l2u_entry, 0, sizeof (*l2u_entry));
        sal_memset(&l2u_mask_entry, 0, sizeof (l2u_mask_entry));

        _l2u_field32_set(unit, l2u_entry, VALIDf, 1);

        vlan = l2caddr->vlan;
        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, KEY_TYPEf)) {
            if (_BCM_VPN_IS_SET(l2caddr->vlan)) {
                _l2u_field32_set(unit, l2u_entry, KEY_TYPEf, 1);
                _BCM_VPN_GET(vlan, _BCM_VPN_TYPE_VFI, l2caddr->vlan);
            }
            _l2u_field32_set(unit, &l2u_mask_entry, KEY_TYPEf, 1);

            if (l2caddr->flags & BCM_L2_CACHE_PROTO_PKT) {
                if (soc_mem_field_valid(unit, L2_USER_ENTRYm, 
                                        L2_PROTOCOL_PKTf)) {
                    _l2u_field32_set(unit, l2u_entry, L2_PROTOCOL_PKTf, 1);
                } else {
                  return BCM_E_UNAVAIL;
                }
            }

        } else {
            if (_BCM_VPN_IS_SET(l2caddr->vlan)) {
                return BCM_E_CONFIG;
            }
        }

        _l2u_field32_set(unit, l2u_entry, VLAN_IDf, vlan);
        _l2u_field32_set(unit, &l2u_mask_entry, VLAN_IDf, l2caddr->vlan_mask);

        _l2u_mac_addr_set(unit, l2u_entry, MAC_ADDRf, l2caddr->mac);
        _l2u_mac_addr_set(unit, &l2u_mask_entry, MAC_ADDRf, l2caddr->mac_mask);

        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, KEYf)) {
            _l2u_field_get(unit, &l2u_mask_entry, KEYf, mask);
        } else {
            /* key fields start from bit 1 */
            mask[0] = (l2u_mask_entry.entry_data[1] << 31) | 
                (l2u_mask_entry.entry_data[0] >> 1); 
            mask[1] = (l2u_mask_entry.entry_data[2] << 31) | 
                (l2u_mask_entry.entry_data[1] >> 1); 
        }
        _l2u_field_set(unit, l2u_entry, MASKf, mask);

        if (l2caddr->flags & BCM_L2_CACHE_SETPRI) {
            _l2u_field32_set(unit, l2u_entry, PRIf, l2caddr->prio);
            _l2u_field32_set(unit, l2u_entry, RPEf, 1);
        }

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) \
 || defined(BCM_RAVEN_SUPPORT)
        if ((SOC_IS_FIREBOLT2(unit) || SOC_IS_TRX(unit)
             || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit)) &&
            (l2caddr->flags & BCM_L2_CACHE_LEARN_DISABLE)) {
            _l2u_field32_set(unit, l2u_entry, DO_NOT_LEARN_MACSAf, 1);
        }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT || BCM_RAVEN_SUPPORT */

        if (l2caddr->flags & BCM_L2_CACHE_CPU) {
            _l2u_field32_set(unit, l2u_entry, CPUf, 1);
        }

        if (l2caddr->flags & BCM_L2_CACHE_BPDU) {
            _l2u_field32_set(unit, l2u_entry, BPDUf, 1);
        }

        if (l2caddr->flags & BCM_L2_CACHE_DISCARD) {
            _l2u_field32_set(unit, l2u_entry, DST_DISCARDf, 1);
        }

        if (soc_feature(unit, soc_feature_trunk_group_overlay)) {
            if (l2caddr->flags & BCM_L2_CACHE_TRUNK) {
                _l2u_field32_set(unit, l2u_entry, Tf, 1);
                _l2u_field32_set(unit, l2u_entry, TGIDf, l2caddr->dest_trunk);
            } else {
                port_field = PORT_NUMf;
            }
        } else {
            if (l2caddr->flags & BCM_L2_CACHE_TRUNK) {
                _l2u_field32_set(unit, l2u_entry, MODULE_IDf,
                                 BCM_TRUNK_TO_MODIDf(unit,
                                                     l2caddr->dest_trunk));
                _l2u_field32_set(unit, l2u_entry, PORT_TGIDf,
                                 BCM_TRUNK_TO_TGIDf(unit,
                                                    l2caddr->dest_trunk));
            } else {
                port_field = PORT_TGIDf;
            }
        }

        if (!(l2caddr->flags & BCM_L2_CACHE_TRUNK)) {
            mod_in = l2caddr->dest_modid;
            port_in = l2caddr->dest_port;
            if (!isGport) {
            PORT_DUALMODID_VALID(unit, port_in);
            }
            BCM_IF_ERROR_RETURN
                (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET,
                                        mod_in, port_in, &mod_out, &port_out));
            if (!SOC_MODID_ADDRESSABLE(unit, mod_out)) {
                return BCM_E_BADID;
            }
            if (!SOC_PORT_ADDRESSABLE(unit, port_out)) {
                return BCM_E_PORT;
            }
            _l2u_field32_set(unit, l2u_entry, MODULE_IDf, mod_out);
            _l2u_field32_set(unit, l2u_entry, port_field, port_out);
        }

        if (l2caddr->flags & BCM_L2_CACHE_L3) {
            if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f)) {
                _l2u_field32_set(unit, l2u_entry, L3f, 1);
            } else if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm,
                                           RESERVED_0f)) {
                _l2u_field32_set(unit, l2u_entry, RESERVED_0f, 1);
            }
        }

        if (l2caddr->flags & BCM_L2_CACHE_MIRROR) {
            if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, MIRRORf)) {
                _l2u_field32_set(unit, l2u_entry, MIRRORf, 1);
            }
        }

#if defined(BCM_TRX_SUPPORT) 
        if (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, CLASS_IDf)) {
            _l2u_field32_set(unit, l2u_entry, CLASS_IDf, l2caddr->lookup_class);
        }
#endif /* BCM_TRX_SUPPORT */

        return BCM_E_NONE;
    }
    return BCM_E_UNAVAIL;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 *      bcm_esw_l2_cache_init
 * Purpose:
 *      Initialize the L2 cache
 * Parameters:
 *      unit - device number
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Clears all entries and preloads a few BCM_L2_CACHE_BPDU
 *      entries to match previous generation of devices.
 */
int
bcm_esw_l2_cache_init(int unit)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int     skip_l2u;

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_ism_memory)) {
        return bcm_tr3_l2_cache_init(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);

    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {
        bcm_l2_cache_addr_t addr;
        l2u_entry_t entry;
        int index;

        if (!SAL_BOOT_QUICKTURN && !SAL_BOOT_XGSSIM) {
            SOC_IF_ERROR_RETURN
                (soc_mem_clear(unit, L2_USER_ENTRYm, COPYNO_ALL, TRUE));
        }
        
        bcm_l2_cache_addr_t_init(&addr);
        addr.flags = BCM_L2_CACHE_CPU | BCM_L2_CACHE_BPDU;
        /* Set default BPDU addresses (01:80:c2:00:00:00) */
        ENET_SET_MACADDR(addr.mac, _soc_mac_spanning_tree);
        ENET_SET_MACADDR(addr.mac_mask, _soc_mac_all_ones);
        BCM_IF_ERROR_RETURN (bcm_esw_stk_my_modid_get(unit, &addr.dest_modid));
        addr.dest_port = CMIC_PORT(unit);
        BCM_IF_ERROR_RETURN(_bcm_l2_cache_to_l2u(unit, &entry, &addr));
        SOC_IF_ERROR_RETURN(soc_l2u_insert(unit, &entry, -1, &index));

        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, L2_PROTOCOL_PKTf)) {
            soc_mem_field32_set(unit, L2_USER_ENTRYm,
                                      &entry, L2_PROTOCOL_PKTf, 1);
            if (!soc_feature(unit, soc_feature_l2_no_vfi)) {
                soc_mem_field32_set(unit, L2_USER_ENTRYm, &entry, KEY_TYPEf, 1);
            }
            if (soc_mem_field_valid(unit, L2_USER_ENTRYm, VFI_MASKf)) {
               soc_mem_field32_set(unit, L2_USER_ENTRYm, &entry, VFI_MASKf, 0);
            }
            SOC_IF_ERROR_RETURN(soc_l2u_insert(unit, &entry, -1, &index));
        }

        if (!SAL_BOOT_QUICKTURN && !SAL_BOOT_XGSSIM) {
            /* Set 01:80:c2:00:00:10 */
            addr.mac[5] = 0x10;
            BCM_IF_ERROR_RETURN(_bcm_l2_cache_to_l2u(unit, &entry, &addr));
            SOC_IF_ERROR_RETURN(soc_l2u_insert(unit, &entry, -1, &index));
            /* Set 01:80:c2:00:00:0x */
            addr.mac[5] = 0x00;
            addr.mac_mask[5] = 0xf0;
            BCM_IF_ERROR_RETURN(_bcm_l2_cache_to_l2u(unit, &entry, &addr));
            SOC_IF_ERROR_RETURN(soc_l2u_insert(unit, &entry, -1, &index));
            /* Set 01:80:c2:00:00:2x */
            addr.mac[5] = 0x20;
            BCM_IF_ERROR_RETURN(_bcm_l2_cache_to_l2u(unit, &entry, &addr));
            SOC_IF_ERROR_RETURN(soc_l2u_insert(unit, &entry, -1, &index));
        }

        return BCM_E_NONE;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcm_esw_l2_cache_size_get
 * Purpose:
 *      Get number of L2 cache entries
 * Parameters:
 *      unit - device number
 *      size - (OUT) number of entries in cache
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l2_cache_size_get(int unit, int *size)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int skip_l2u;

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);

    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {
        *size = soc_mem_index_count(unit, L2_USER_ENTRYm);

        return 0;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_l2_cache_set
 * Purpose:
 *      Set an L2 cache entry
 * Parameters:
 *      unit - device number
 *      index - l2 cache entry number (or -1)
 *              If -1 is given then the entry used will be the first
 *              available entry if the l2 cache address mac_mask
 *              field is all ones, or the last available entry if
 *              the mac_mask field has some zeros.  Cache entries
 *              are matched from lowest entry to highest entry.
 *      addr - l2 cache address
 *      index_used - (OUT) l2 cache entry number actually used
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l2_cache_set(int unit, int index, bcm_l2_cache_addr_t *addr,
                 int *index_used)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int skip_l2u;
    l2u_entry_t l2u_entry;
    bcm_l2_cache_addr_t l2_addr;

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }
    L2_INIT(unit);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_ism_memory)) {
        return bcm_tr3_l2_cache_set(unit, index, addr, index_used);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);

    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {
        if (index < -1 || index > soc_mem_index_max(unit, L2_USER_ENTRYm)) {
            return BCM_E_PARAM;
        }

        sal_memcpy(&l2_addr, addr, sizeof(bcm_l2_cache_addr_t));

        if ((l2_addr.lookup_class > SOC_ADDR_CLASS_MAX(unit)) || 
            (l2_addr.lookup_class < 0)) {
            return (BCM_E_PARAM);
        }

        /* ESW architecture can't support multiple destinations */
        if (BCM_PBMP_NOT_NULL(l2_addr.dest_ports)) {
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN(
            _bcm_l2_cache_to_l2u(unit, &l2u_entry, &l2_addr));
        
#ifdef BCM_TRIDENT_SUPPORT
        if (soc_mem_is_valid(unit, MY_STATION_TCAMm) &&
            (addr->flags & BCM_L2_CACHE_L3)) {
            int rv;
            if (index == -1) {
                rv = bcm_td_l2cache_myStation_lookup(unit, &l2_addr, &index);
                if (BCM_FAILURE(rv)) {
                    /* find the first free index in L2_USER_ENTRY */
                    BCM_IF_ERROR_RETURN
                        (soc_l2u_find_free_entry(unit, &l2u_entry, &index));
                }
            }

            rv = bcm_td_l2cache_myStation_set(unit, index, &l2_addr);
        }
#endif /* BCM_TRIDENT_SUPPORT */

        SOC_IF_ERROR_RETURN
            (soc_l2u_insert(unit, &l2u_entry, index, index_used));

        return BCM_E_NONE;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_l2_cache_get
 * Purpose:
 *      Get an L2 cache entry
 * Parameters:
 *      unit - device number
 *      index - l2 cache entry index
 *      size - (OUT) l2 cache address
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l2_cache_get(int unit, int index, bcm_l2_cache_addr_t *addr)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int skip_l2u;
    l2u_entry_t l2u_entry;

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    L2_INIT(unit)

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_ism_memory)) {
        return bcm_tr3_l2_cache_get(unit, index, addr);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);

    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {
        if (index < 0 || index > soc_mem_index_max(unit, L2_USER_ENTRYm)) {
            return BCM_E_PARAM;
        }

        SOC_IF_ERROR_RETURN(
            soc_l2u_get(unit, &l2u_entry, index));

        BCM_IF_ERROR_RETURN(
            _bcm_l2_cache_from_l2u(unit, addr, &l2u_entry));

#ifdef BCM_TRIDENT_SUPPORT
        if (soc_mem_is_valid(unit, MY_STATION_TCAMm) &&
            (addr->flags & BCM_L2_CACHE_L3)) {
            BCM_IF_ERROR_RETURN
                (bcm_td_l2cache_myStation_get(unit, index, addr));
        }
#endif /* BCM_TRIDENT_SUPPORT */

        return BCM_E_NONE;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_l2_cache_delete
 * Purpose:
 *      Clear an L2 cache entry
 * Parameters:
 *      unit - device number
 *      index - l2 cache entry index
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l2_cache_delete(int unit, int index)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int skip_l2u, rv;

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    L2_INIT(unit);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_ism_memory)) {
        return bcm_tr3_l2_cache_delete(unit, index);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);

    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {
        if (index < 0 || index > soc_mem_index_max(unit, L2_USER_ENTRYm)) {
            return BCM_E_PARAM;
        }

        rv = BCM_E_NONE;
        soc_mem_lock(unit, L2_USER_ENTRYm);
#ifdef BCM_TRIDENT_SUPPORT
        if (soc_mem_is_valid(unit, MY_STATION_TCAMm)) {
            l2u_entry_t l2u_entry;
            rv = soc_l2u_get(unit, &l2u_entry, index);
            if (BCM_SUCCESS(rv))  {
                if ((SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f) &&
                     _l2u_field32_get(unit, &l2u_entry, L3f)) ||
                    (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f) &&
                     _l2u_field32_get(unit, &l2u_entry, RESERVED_0f))) {
                     rv = bcm_td_l2cache_myStation_delete(unit, index);
                } else if (SOC_IS_KATANAX(unit)) {
	                   /* Katana doesn't has L3,RESERVED_0 fields
                              so ignoring return value ! */
                           bcm_td_l2cache_myStation_delete(unit, index);
                } /* Nothing can be done..!! */
            }
        }
#endif /* BCM_TRIDENT_SUPPORT */
        if (BCM_SUCCESS(rv)) {
            rv = soc_l2u_delete(unit, NULL, index, &index);
        }
        soc_mem_unlock(unit, L2_USER_ENTRYm);
        return rv;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_esw_l2_cache_delete_all
 * Purpose:
 *      Clear all L2 cache entries
 * Parameters:
 *      unit - device number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l2_cache_delete_all(int unit)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int skip_l2u, rv, index, index_max;
    l2u_entry_t entry;

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    L2_INIT(unit);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_ism_memory)) {
        return bcm_tr3_l2_cache_delete_all(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);

    if (soc_feature(unit, soc_feature_l2_user_table) && !skip_l2u) {
        index_max = soc_mem_index_max(unit, L2_USER_ENTRYm);

        rv = BCM_E_NONE;
        soc_mem_lock(unit, L2_USER_ENTRYm);
        for (index = 0; index <= index_max; index++) {
#ifdef BCM_TRIDENT_SUPPORT
            if (soc_mem_is_valid(unit, MY_STATION_TCAMm)) {
                rv = READ_L2_USER_ENTRYm(unit, MEM_BLOCK_ALL, index, &entry);
                if (BCM_SUCCESS(rv)) {
                    rv = bcm_td_l2cache_myStation_delete(unit, index);
                    /* Ignore entry not found case */
                    if (rv == BCM_E_NOT_FOUND) {
                        rv = BCM_E_NONE;
                    }
                }
            }
#endif /* BCM_TRIDENT_SUPPORT */
            if (BCM_SUCCESS(rv)) {
                sal_memset(&entry, 0, sizeof(entry));
                rv = WRITE_L2_USER_ENTRYm(unit, MEM_BLOCK_ALL, index, &entry);
            }
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
        soc_mem_unlock(unit, L2_USER_ENTRYm);
        return rv;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return BCM_E_UNAVAIL;
}
