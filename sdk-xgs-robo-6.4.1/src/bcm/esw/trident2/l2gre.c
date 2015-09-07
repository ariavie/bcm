/*
 * $Id: l2gre.c,v 1.1 Broadcom SDK $
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
 * File:    l2gre.c
 * Purpose: TD2 L2GRE enhancements
 */
#include <soc/defs.h>

#include <assert.h>

#include <sal/core/libc.h>
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/hash.h>
#include <soc/debug.h>
#include <bcm/types.h>
#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/l2gre.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/common/multicast.h>


#define L2GRE_INFO(_unit_)   (_bcm_tr3_l2gre_bk_info[_unit_])

/*
 * Function:
 *		_bcm_td2_mpls_match_vlan_entry_update
 * Purpose:
 *		Update L2GRE Match Vlan_xlate  Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vlan_xlate_entry_t
 *  OUT :  vlan_xlate_entry_t

 */

STATIC int
_bcm_td2_l2gre_match_vxlate_entry_update(int unit, vlan_xlate_entry_t *vent, 
                                                                     vlan_xlate_entry_t *return_ent)
{
    uint32  vp, key_type, value;

    /* Check if Key_Type identical */
    key_type = soc_VLAN_XLATEm_field32_get (unit, vent, KEY_TYPEf);
    value = soc_VLAN_XLATEm_field32_get (unit, return_ent, KEY_TYPEf);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    /* Retain original Keys -- Update data only */
    soc_VLAN_XLATEm_field32_set(unit, return_ent, XLATE__MPLS_ACTIONf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, XLATE__DISABLE_VLAN_CHECKSf, 1);
    vp = soc_VLAN_XLATEm_field32_get (unit, vent, XLATE__SOURCE_VPf);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, XLATE__SOURCE_VPf, vp);

   return BCM_E_NONE;
}


/*
 * Function:
 *        _bcm_td2_l2gre_match_vxlate_entry_set
 * Purpose:
 *       Set L2GRE Match Vxlate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  l2gre_port
 *  IN :  vlan_xlate_entry_t
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_l2gre_match_vxlate_entry_set(int unit, bcm_l2gre_port_t *l2gre_port, vlan_xlate_entry_t *vent)
{
    vlan_xlate_entry_t return_vent;
    int rv = BCM_E_UNAVAIL;
    int index;

    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        vent, &return_vent, 0);

    if (rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
              _bcm_td2_l2gre_match_vxlate_entry_update (unit, vent, &return_vent));
         rv = soc_mem_write(unit, VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           &return_vent);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {
         if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
             return BCM_E_NOT_FOUND;
         } else {
              rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, vent);
         }
    }

    return rv;
}


/*
 * Function:
 *      _bcm_td2_l2gre_match_add
 * Purpose:
 *      Assign match criteria of an L2GRE port
 * Parameters:
 *      unit    - (IN) Device Number
 *      l2gre_port - (IN) l2gre port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_td2_l2gre_match_add(int unit, bcm_l2gre_port_t *l2gre_port, int vp, bcm_vpn_t vpn)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);
    int rv = BCM_E_NONE;
    bcm_module_t mod_out = -1;
    bcm_port_t port_out = -1;
    bcm_trunk_t trunk_id = -1;
    vlan_xlate_entry_t vent;
    int    mod_id_idx=0; /* Module Id */
    int    src_trk_idx=0;  /*Source Trunk table index.*/
    int    gport_id=-1;

    rv = _bcm_tr3_l2gre_port_resolve(unit, l2gre_port->l2gre_port_id, -1, &mod_out,
                    &port_out, &trunk_id, &gport_id);
    BCM_IF_ERROR_RETURN(rv);

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod_id_idx));

    if (l2gre_port->criteria == BCM_L2GRE_PORT_MATCH_PORT_VLAN ) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
        if (!BCM_VLAN_VALID(l2gre_port->match_vlan)) {
             return BCM_E_PARAM;
        }
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                        TR_VLXLT_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf, 
                                        l2gre_port->match_vlan);
        l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VLAN;
        l2gre_info->match_key[vp].match_vlan = l2gre_port->match_vlan;

        if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            l2gre_info->match_key[vp].trunk_id = trunk_id;
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            l2gre_info->match_key[vp].port = port_out;
            l2gre_info->match_key[vp].modid = mod_out;
        }
        rv = _bcm_td2_l2gre_match_vxlate_entry_set(unit, l2gre_port, &vent);
        BCM_IF_ERROR_RETURN(rv);
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
              bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
        }
    } else if (l2gre_port->criteria ==
                            BCM_L2GRE_PORT_MATCH_PORT_INNER_VLAN) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
        if (!BCM_VLAN_VALID(l2gre_port->match_inner_vlan)) {
            return BCM_E_PARAM;
        }
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                    TR_VLXLT_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf,
                                    l2gre_port->match_inner_vlan);
        l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_INNER_VLAN;
        l2gre_info->match_key[vp].match_inner_vlan = l2gre_port->match_inner_vlan;

        if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            l2gre_info->match_key[vp].trunk_id = trunk_id;
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            l2gre_info->match_key[vp].port = port_out;
            l2gre_info->match_key[vp].modid = mod_out;
        }
        rv = _bcm_td2_l2gre_match_vxlate_entry_set(unit, l2gre_port, &vent);
        BCM_IF_ERROR_RETURN(rv);
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
             bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
        }
    } else if (l2gre_port->criteria == 
                            BCM_L2GRE_PORT_MATCH_PORT_VLAN_STACKED) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
         soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
         if (!BCM_VLAN_VALID(l2gre_port->match_vlan) || 
                !BCM_VLAN_VALID(l2gre_port->match_inner_vlan)) {
              return BCM_E_PARAM;
         }
         soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                   TR_VLXLT_HASH_KEY_TYPE_IVID_OVID);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf, 
                   l2gre_port->match_vlan);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf, 
                   l2gre_port->match_inner_vlan);
         l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_STACKED;
         l2gre_info->match_key[vp].match_vlan = l2gre_port->match_vlan;
         l2gre_info->match_key[vp].match_inner_vlan = l2gre_port->match_inner_vlan;
         if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            l2gre_info->match_key[vp].trunk_id = trunk_id;
         } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            l2gre_info->match_key[vp].port = port_out;
            l2gre_info->match_key[vp].modid = mod_out;
         }
         rv = _bcm_td2_l2gre_match_vxlate_entry_set(unit, l2gre_port, &vent);
         BCM_IF_ERROR_RETURN(rv);
         if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
             bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
         }
    } else if (l2gre_port->criteria ==
                                            BCM_L2GRE_PORT_MATCH_VLAN_PRI) {
              soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
              soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
              soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                                 TR_VLXLT_HASH_KEY_TYPE_PRI_CFI);
              /* match_vlan : Bits 12-15 contains VLAN_PRI + CFI, vlan=BCM_E_NONE */
              soc_VLAN_XLATEm_field32_set(unit, &vent, OTAGf, 
                                                 l2gre_port->match_vlan);
              l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_PRI;
              l2gre_info->match_key[vp].match_vlan = l2gre_port->match_vlan;

              if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
                   soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
                   soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
                        l2gre_info->match_key[vp].trunk_id = trunk_id;
              } else {
                   soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
                   soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
                   l2gre_info->match_key[vp].port = port_out;
                   l2gre_info->match_key[vp].modid = mod_out;
              }
              rv = _bcm_td2_l2gre_match_vxlate_entry_set(unit, l2gre_port, &vent);
              BCM_IF_ERROR_RETURN(rv);
              if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
                   bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
              }
    }else if (l2gre_port->criteria == BCM_L2GRE_PORT_MATCH_PORT) {
        if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
             rv = bcm_tr3_l2gre_match_trunk_add(unit, trunk_id, vp);
             if (rv >= 0) {
                   l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_TRUNK;
                   l2gre_info->match_key[vp].trunk_id = trunk_id;
             }
             BCM_IF_ERROR_RETURN(rv);       
        } else {
            int is_local;

            BCM_IF_ERROR_RETURN
                ( _bcm_esw_modid_is_local(unit, mod_out, &is_local));

            /* Get index to source trunk map table */
            BCM_IF_ERROR_RETURN(
                   _bcm_esw_src_mod_port_table_index_get(unit, mod_out,
                     port_out, &src_trk_idx));

            rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                        src_trk_idx, SOURCE_VPf, vp);
            BCM_IF_ERROR_RETURN(rv);
            /* Enable SVP Mode */
            if (soc_mem_field_valid(unit, SOURCE_TRUNK_MAP_TABLEm, SVP_VALIDf) ) {
                rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm,
                                        src_trk_idx, SVP_VALIDf, 0x1);
                BCM_IF_ERROR_RETURN(rv);
            }
            if (is_local) {
                rv = soc_mem_field32_modify(unit, PORT_TABm, port_out,
                                        PORT_OPERATIONf, 0x1); /* L2_SVP */
                BCM_IF_ERROR_RETURN(rv);

                /* Set TAG_ACTION_PROFILE_PTR */
                rv = bcm_tr3_l2gre_port_untagged_profile_set(unit, port_out);
                BCM_IF_ERROR_RETURN(rv);
            }

            l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_PORT;
            l2gre_info->match_key[vp].index = src_trk_idx;
        }
    }else if (l2gre_port->criteria == BCM_L2GRE_PORT_MATCH_VPNID) {
        mpls_entry_entry_t ment;
        int tunnel_idx=-1;
        uint32 tunnel_sip;

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VPNID;
        if (!BCM_GPORT_IS_TUNNEL(l2gre_port->match_tunnel_id)) {
            return BCM_E_PARAM;
        }

        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(l2gre_port->match_tunnel_id);
        tunnel_sip = l2gre_info->l2gre_tunnel_term[tunnel_idx].sip;
        l2gre_info->match_key[vp].match_tunnel_index = tunnel_idx;

        if (l2gre_port->flags & BCM_L2GRE_PORT_MULTICAST) {
              return BCM_E_NONE;
        }

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_SIP__SIPf, 
                                    tunnel_sip);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_SIP__SVPf, vp);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_L2GRE_KEY_TYPE_TUNNEL);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

        rv = bcm_tr3_l2gre_match_tunnel_entry_set(unit, &ment);
        BCM_IF_ERROR_RETURN(rv);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_l2gre_match_delete
 * Purpose:
 *      Delete match criteria of an L2GRE port
 * Parameters:
 *      unit    - (IN) Device Number
 *      l2gre_port - (IN) L2GRE port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_td2_l2gre_match_delete(int unit,  int vp)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);
    int rv=BCM_E_NONE;
    vlan_xlate_entry_t vent;
    bcm_trunk_t trunk_id;
    int    src_trk_idx=0;   /*Source Trunk table index.*/
    int    mod_id_idx=0;   /* Module_Id */
    int port=0, tunnel_index = -1;
    mpls_entry_entry_t ment;

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod_id_idx));

    if  (l2gre_info->match_key[vp].flags == _BCM_L2GRE_PORT_MATCH_TYPE_VLAN) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                    TR_VLXLT_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf,
                                    l2gre_info->match_key[vp].match_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        l2gre_info->match_key[vp].modid);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        l2gre_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);
        
        (void) bcm_tr3_l2gre_match_clear (unit, vp);
         bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);

    } else if  (l2gre_info->match_key[vp].flags == 
                     _BCM_L2GRE_PORT_MATCH_TYPE_INNER_VLAN) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                    VLXLT_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf,
                                    l2gre_info->match_key[vp].match_inner_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        l2gre_info->match_key[vp].modid);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        l2gre_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);
        
        (void) bcm_tr3_l2gre_match_clear (unit, vp);
        bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);
    }else if (l2gre_info->match_key[vp].flags == 
                    _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_STACKED) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                    TR_VLXLT_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf,
                                    l2gre_info->match_key[vp].match_vlan);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf,
                              l2gre_info->match_key[vp].match_inner_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        l2gre_info->match_key[vp].modid);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        l2gre_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);

        (void) bcm_tr3_l2gre_match_clear (unit, vp);
        bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);

    } else if	(l2gre_info->match_key[vp].flags ==
                                               _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_PRI) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                                      TR_VLXLT_HASH_KEY_TYPE_PRI_CFI);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OTAGf,
                                                       l2gre_info->match_key[vp].match_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                                      l2gre_info->match_key[vp].modid);
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                                      l2gre_info->match_key[vp].port);
        } else {
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                                      l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);

        (void) bcm_tr3_l2gre_match_clear (unit, vp);
        bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);
    }else if  (l2gre_info->match_key[vp].flags == 
                    _BCM_L2GRE_PORT_MATCH_TYPE_PORT) {
         int is_local;

         src_trk_idx = l2gre_info->match_key[vp].index;
         rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                  src_trk_idx, SOURCE_VPf, 0);
         BCM_IF_ERROR_RETURN(rv);
         /* Disable SVP Mode */
         if (soc_mem_field_valid(unit, SOURCE_TRUNK_MAP_TABLEm, SVP_VALIDf) ) {
                rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm,
                                        src_trk_idx, SVP_VALIDf, 0x0);
                BCM_IF_ERROR_RETURN(rv);
         }

         BCM_IF_ERROR_RETURN(_bcm_esw_src_modid_port_get( unit, src_trk_idx,
                                                          &mod_id_idx, &port));
         BCM_IF_ERROR_RETURN
             ( _bcm_esw_modid_is_local(unit, mod_id_idx, &is_local));

         if (is_local) {
            rv = soc_mem_field32_modify(unit, PORT_TABm, port,
                                       PORT_OPERATIONf, 0x0); /* NORMAL */
            BCM_IF_ERROR_RETURN(rv);

            /* Reset TAG_ACTION_PROFILE_PTR */
            rv = bcm_tr3_l2gre_port_untagged_profile_reset(unit, port);
            BCM_IF_ERROR_RETURN(rv);
         }
         (void) bcm_tr3_l2gre_match_clear (unit, vp);

    } else if  (l2gre_info->match_key[vp].flags == 
                  _BCM_L2GRE_PORT_MATCH_TYPE_TRUNK) {
         trunk_id = l2gre_info->match_key[vp].trunk_id;
         rv = bcm_tr3_l2gre_match_trunk_delete(unit, trunk_id, vp);
         BCM_IF_ERROR_RETURN(rv);

        (void) bcm_tr3_l2gre_match_clear (unit, vp);
    } else if (l2gre_info->match_key[vp].flags == 
                  _BCM_L2GRE_PORT_MATCH_TYPE_VPNID) {

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        tunnel_index = l2gre_info->match_key[vp].match_tunnel_index;
        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_VPNID__SIPf, 
                                    l2gre_info->l2gre_tunnel_term[tunnel_index].sip);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_L2GRE_KEY_TYPE_TUNNEL);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

        rv = bcm_tr3_l2gre_match_tunnel_entry_reset(unit, &ment);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l2gre_eline_vp_configure
 * Purpose:
 *      Configure L2GRE ELINE virtual port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_l2gre_eline_vp_configure (int unit, int vfi_index, int active_vp, 
                  source_vp_entry_t *svp, int tpid_enable, bcm_l2gre_port_t  *l2gre_port)
{
    int rv = BCM_E_NONE;
    source_vp_2_entry_t svp_2_entry;

    /* Set SOURCE_VP */
    soc_SOURCE_VPm_field32_set(unit, svp, CLASS_IDf,
                                l2gre_port->if_class);

    if (l2gre_port->flags & BCM_L2GRE_PORT_NETWORK) {
         soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf, 1);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_SOURCEf, _BCM_L2GRE_TPID_SVP_BASED);
    } else {
         soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf, 0);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_SOURCEf, _BCM_L2GRE_TPID_SGLP_BASED);
         /*  Configure IPARS Parser to signal to IMPLS Parser - Applicable only for HG2 PPD2 packets 
               to reconstruct TPID state based on HG header information - Only for VXLAN Access ports */
         sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
         soc_SOURCE_VP_2m_field32_set(unit, &svp_2_entry, PARSE_USING_SGLP_TPIDf, 1);
         rv = WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, active_vp, &svp_2_entry);
         if (rv < 0) {
             return rv;
         }
    }

    if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TAGGED) {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 1);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_ENABLEf, tpid_enable);
    } else {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 0);
    }

    soc_SOURCE_VPm_field32_set(unit, svp, DISABLE_VLAN_CHECKSf, 1);
    soc_SOURCE_VPm_field32_set(unit, svp, 
                           ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_VFI); /* VFI Type */
    soc_SOURCE_VPm_field32_set(unit, svp, VFIf, vfi_index);

    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, active_vp, svp);
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l2gre_elan_vp_configure
 * Purpose:
 *      Configure L2GRE ELAN virtual port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_l2gre_elan_vp_configure (int unit, int vfi_index, int vp, 
                  source_vp_entry_t *svp, int tpid_enable, bcm_l2gre_port_t  *l2gre_port)
{
    int rv = BCM_E_NONE;
    int cml_default_enable=0, cml_default_new=0, cml_default_move=0;
    source_vp_2_entry_t svp_2_entry;

    /* Set SOURCE_VP */
    soc_SOURCE_VPm_field32_set(unit, svp, CLASS_IDf,
                         l2gre_port->if_class);

    if (vfi_index == _BCM_L2GRE_VFI_INVALID) {
        soc_SOURCE_VPm_field32_set(unit, svp, 
                                   ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_INVALID); /* INVALID */
    } else {
        /* Initialize VP parameters */
        soc_SOURCE_VPm_field32_set(unit, svp, 
                                   ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_VFI); /* VFI Type */
    }
    if (l2gre_port->flags & BCM_L2GRE_PORT_NETWORK) {
           soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf, 1);
           soc_SOURCE_VPm_field32_set(unit, svp, TPID_SOURCEf, _BCM_L2GRE_TPID_SVP_BASED);
    } else {
           soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf, 0);
           soc_SOURCE_VPm_field32_set(unit, svp, TPID_SOURCEf, _BCM_L2GRE_TPID_SGLP_BASED);
           /*  Configure IPARS Parser to signal to IMPLS Parser - Applicable only for HG2 PPD2 packets 
                to reconstruct TPID state based on HG header information - Only for VXLAN Access ports */
           sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
           soc_SOURCE_VP_2m_field32_set(unit, &svp_2_entry, PARSE_USING_SGLP_TPIDf, 1);
           rv = WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, vp, &svp_2_entry);
           if (rv < 0) {
              return rv;
           }
    }
    soc_SOURCE_VPm_field32_set(unit, svp, VFIf, vfi_index);

    if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TAGGED) {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 1);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_ENABLEf, tpid_enable);
    } else {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 0);
    }
    rv = _bcm_vp_default_cml_mode_get (unit,
                       &cml_default_enable, &cml_default_new, 
                       &cml_default_move);
    if (rv < 0) {
         return rv;
    }
    if (cml_default_enable) {
        /* Set the CML to default values */
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_NEWf, cml_default_new);
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_MOVEf, cml_default_move);
    } else {
        /* Set the CML to PVP_CML_SWITCH by default (hw learn and forward) */
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_NEWf, 0x8);
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_MOVEf, 0x8);
    }
    if (soc_mem_field_valid(unit, SOURCE_VPm, DISABLE_VLAN_CHECKSf)) {
          soc_SOURCE_VPm_field32_set(unit, svp, DISABLE_VLAN_CHECKSf, 0x1);
    }

    BCM_IF_ERROR_RETURN
           (WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, svp));

    return rv;
}

#else /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */
int bcm_esw_td2_l2gre_not_empty;
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */
