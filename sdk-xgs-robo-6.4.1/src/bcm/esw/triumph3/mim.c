/*
 * $Id: mim.c,v 1.10 Broadcom SDK $
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
 * File:    mim.c
 * Purpose: Manages MiM functions for triumph3
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/hash.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>
#include <soc/triumph.h>

#include <bcm/error.h>
#include <bcm/mim.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw/mim.h>
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/triumph3.h>

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/api_xlate_port.h>


#define L3_INFO(unit)    (&_bcm_l3_bk_info[unit])
#define MIM_INFO(_unit_) (&_bcm_tr2_mim_bk_info[_unit_])
#define VPN_ISID(_unit_, _vfi_)  \
        (_bcm_tr2_mim_bk_info[_unit_].vpn_info[_vfi_].isid)

/*
 * Function:
 *      _bcm_tr3_mim_match_add
 * Purpose: 
 *      Add MiM match parameters
 *
 * Parameters: 
 *      unit       - (IN) bcm device
 *      mim_port   - (IN) mim port
 *      vp         - (IN) vp
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_tr3_mim_match_add(int unit, bcm_mim_port_t *mim_port, int vp)
{
    int rv = BCM_E_NONE, i, gport_id;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    bcm_mac_t mac_mask;


    rv = _bcm_esw_gport_resolve(unit, mim_port->port, &mod_out,
                                &port_out, &trunk_id, &gport_id);
    BCM_IF_ERROR_RETURN(rv);

    if ((mim_port->criteria == BCM_MIM_PORT_MATCH_PORT_VLAN) ||
        (mim_port->criteria == BCM_MIM_PORT_MATCH_PORT_VLAN_STACKED)) {
        vlan_xlate_extd_entry_t vent;
        soc_mem_t xlate_mem;

        sal_memset(&vent, 0, sizeof(vent));
        xlate_mem = VLAN_XLATE_EXTDm;
        soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_0f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_1f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_mem_field32_set(unit, xlate_mem, &vent, VALID_0f, 0x1);
        soc_mem_field32_set(unit, xlate_mem, &vent, VALID_1f, 0x1);
        /* vlan xlate data */
        soc_mem_field32_set(unit, xlate_mem, &vent, MPLS_ACTIONf, 0x1); /* SVP */
        soc_mem_field32_set(unit, xlate_mem, &vent, DISABLE_VLAN_CHECKSf, 1);
        soc_mem_field32_set(unit, xlate_mem, &vent, SOURCE_VPf, vp);

        
        /* vlan xlate key */
        if (mim_port->criteria == BCM_MIM_PORT_MATCH_PORT_VLAN) {
            if (!BCM_VLAN_VALID(mim_port->match_vlan)) {
                return BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, xlate_mem, &vent, OVIDf,
                                        mim_port->match_vlan);
            MIM_INFO(unit)->port_info[vp].flags |= 
                                     _BCM_MIM_PORT_TYPE_ACCESS_PORT_VLAN;
            MIM_INFO(unit)->port_info[vp].match_vlan = mim_port->match_vlan;

        } else {
            if (!BCM_VLAN_VALID(mim_port->match_vlan) || 
                !BCM_VLAN_VALID(mim_port->match_inner_vlan)) {
                return BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_0f,
                                        TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
            soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_1f,
                                        TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
            soc_mem_field32_set(unit, xlate_mem, &vent, VALID_0f, 0x1);
            soc_mem_field32_set(unit, xlate_mem, &vent, VALID_1f, 0x1);
            soc_mem_field32_set(unit, xlate_mem, &vent, OVIDf,
                                        mim_port->match_vlan);
            soc_mem_field32_set(unit, xlate_mem, &vent, IVIDf,
                                        mim_port->match_inner_vlan);
            MIM_INFO(unit)->port_info[vp].flags |= 
                             _BCM_MIM_PORT_TYPE_ACCESS_PORT_VLAN_STACKED;
            MIM_INFO(unit)->port_info[vp].match_vlan = mim_port->match_vlan;
            MIM_INFO(unit)->port_info[vp].match_inner_vlan = 
                             mim_port->match_inner_vlan;
        }

        if (BCM_GPORT_IS_TRUNK(mim_port->port)) {
            soc_mem_field32_set(unit, xlate_mem, &vent, Tf, 1);
            soc_mem_field32_set(unit, xlate_mem, &vent, TGIDf, trunk_id);
        } else {
            soc_mem_field32_set(unit, xlate_mem, &vent, MODULE_IDf, mod_out);
            soc_mem_field32_set(unit, xlate_mem, &vent, PORT_NUMf, port_out);
        }
        rv = soc_mem_insert(unit, xlate_mem, MEM_BLOCK_ALL, &vent);
        BCM_IF_ERROR_RETURN(rv);
        _bcm_tr2_mim_port_match_count_adjust(unit, vp, 1);

    } else if (mim_port->criteria == BCM_MIM_PORT_MATCH_PORT) {
        if (BCM_GPORT_IS_TRUNK(mim_port->port)) {
            rv = _bcm_tr2_mim_match_trunk_add(unit, trunk_id, vp);
            if (rv >= 0) {
                MIM_INFO(unit)->port_info[vp].flags |= 
                    _BCM_MIM_PORT_TYPE_ACCESS_PORT_TRUNK;
                MIM_INFO(unit)->port_info[vp].index = trunk_id;
            }
            BCM_IF_ERROR_RETURN(rv);
        } else {
            source_trunk_map_modbase_entry_t modbase_entry;
            int mod_id_idx=0; /* Module Id */
            int isLocal;

            BCM_IF_ERROR_RETURN(_bcm_esw_modid_is_local(unit, mod_out, &isLocal));
            BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod_id_idx));

            if ((!isLocal) && (mod_out != BCM_MODID_INVALID)) {
                mod_id_idx = mod_out;
            }

            BCM_IF_ERROR_RETURN
            (soc_mem_read(unit, SOURCE_TRUNK_MAP_MODBASEm, MEM_BLOCK_ANY,
                mod_id_idx, &modbase_entry));

            i = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_MODBASEm,
                    &modbase_entry, BASEf) + port_out;
            rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm,
                    i, SOURCE_VPf, vp);
            BCM_IF_ERROR_RETURN(rv);

            rv = soc_mem_field32_modify(unit, PORT_TABm, port_out,
                    PORT_OPERATIONf, 0x1); /* L2_SVP */
            BCM_IF_ERROR_RETURN(rv);

            MIM_INFO(unit)->port_info[vp].flags |= 
                             _BCM_MIM_PORT_TYPE_ACCESS_PORT;
            MIM_INFO(unit)->port_info[vp].index = i;
        }
    } else if ((mim_port->criteria == BCM_MIM_PORT_MATCH_LABEL)) {
        mpls_entry_entry_t ment;

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        if (BCM_GPORT_IS_TRUNK(mim_port->port)) {
            soc_MPLS_ENTRYm_field32_set(unit, &ment, Tf, 1);
            soc_MPLS_ENTRYm_field32_set(unit, &ment, TGIDf, trunk_id);
        } else {
            soc_MPLS_ENTRYm_field32_set(unit, &ment, MODULE_IDf, mod_out);
            soc_MPLS_ENTRYm_field32_set(unit, &ment, PORT_NUMf, port_out);
        }
        soc_MPLS_ENTRYm_field32_set(unit, &ment, MPLS_LABELf,
                                    mim_port->match_label);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, SOURCE_VPf, vp);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, MPLS_ACTION_IF_BOSf,
                                    0x1); /* L2 SVP */
        soc_MPLS_ENTRYm_field32_set(unit, &ment, MPLS_ACTION_IF_NOT_BOSf,
                                    0x0); /* INVALID */

        rv = soc_mem_insert(unit, MPLS_ENTRYm, MEM_BLOCK_ALL, &ment);
        BCM_IF_ERROR_RETURN(rv);
        MIM_INFO(unit)->port_info[vp].flags |= 
                         _BCM_MIM_PORT_TYPE_ACCESS_LABEL;
        MIM_INFO(unit)->port_info[vp].match_label = mim_port->match_label;

    } else if ((mim_port->criteria == BCM_MIM_PORT_MATCH_TUNNEL_VLAN_SRCMAC)) {
        mpls_entry_extd_entry_t ment;
        soc_mem_t mpls_mem;
        uint64 temp_mac;

        /* Configure peer  Network Virtual port ( NVP) */
        if (!BCM_VLAN_VALID(mim_port->match_tunnel_vlan)) {
            return BCM_E_PARAM;
        }

        sal_memset(&ment, 0, sizeof(ment));
        mpls_mem = MPLS_ENTRY_EXTDm;
        soc_mem_field32_set(unit, mpls_mem, &ment, KEY_TYPE_0f, 0x17); /* MIM NVP */
        soc_mem_field32_set(unit, mpls_mem, &ment, KEY_TYPE_1f, 0x17);
        soc_mem_field32_set(unit, mpls_mem, &ment, VALID_0f, 1);
        soc_mem_field32_set(unit, mpls_mem, &ment, VALID_1f, 1);

        /* MIM_NVP key */
        soc_mem_field32_set(unit, mpls_mem, &ment, MIM_NVP__BVIDf,
                                    mim_port->match_tunnel_vlan);
        soc_mem_mac_addr_set(unit, mpls_mem, &ment, MIM_NVP__BMACSAf, 
                             mim_port->match_tunnel_srcmac);

        /* MIM_NVP data */ 
        if (BCM_GPORT_IS_TRUNK(mim_port->port)) {
            soc_mem_field32_set(unit, mpls_mem, &ment, MIM_NVP__Tf, 1);
            soc_mem_field32_set(unit, mpls_mem, &ment, MIM_NVP__TGIDf, trunk_id);
        } else {
            soc_mem_field32_set(unit, mpls_mem, &ment, MIM_NVP__MODULE_IDf, mod_out);
            soc_mem_field32_set(unit, mpls_mem, &ment, MIM_NVP__PORT_NUMf, port_out);
        }
        if (mim_port->flags & BCM_MIM_PORT_TYPE_PEER) {
            soc_mem_t tunnel_mem;
            soc_tunnel_term_t tnl_entry;
            uint32 index;

            sal_memset(&tnl_entry, 0, sizeof(tnl_entry));
            tunnel_mem = L3_TUNNELm;
           
            /* MIM L3 TUNNEL Type */
            soc_mem_field32_set(unit, tunnel_mem, &tnl_entry, KEY_TYPEf, 0x3);
 
            /* MIM L3 TUNNEL KEY BMACSA+BVID */
            soc_mem_mac_addr_set(unit, tunnel_mem, &tnl_entry, MIM__BMACSAf, 
                                                mim_port->match_tunnel_srcmac);
            soc_mem_field32_set(unit, tunnel_mem, &tnl_entry, MIM__BVIDf,
                                                 mim_port->match_tunnel_vlan);

            /* Enable ISID,BMACSA, BVID lookup in mpls entry table */
            soc_mem_field32_set(unit, tunnel_mem, &tnl_entry, 
                MIM__ISID_LOOKUP_KEY_CONTROLf, 1);

            sal_memset(&mac_mask, 0xff, sizeof(mac_mask));
            soc_mem_field32_set(unit, tunnel_mem, 
                        &tnl_entry, MIM__SGLP_MASKf, 0xffff);
            soc_mem_field32_set(unit, tunnel_mem, 
                        &tnl_entry, MIM__BVID_MASKf, 0xfff);
            soc_mem_mac_addr_set(unit, tunnel_mem, 
                        &tnl_entry, MIM__BMACSA_MASKf, mac_mask);
            soc_mem_field32_set(unit, tunnel_mem, 
                        &tnl_entry, MIM__KEY_TYPE_MASKf, 0x3);
            soc_mem_field32_set(unit,  tunnel_mem, &tnl_entry, VALIDf, 1);

            rv = soc_tunnel_term_insert(unit, &tnl_entry, &index);
            BCM_IF_ERROR_RETURN(rv);
            MIM_INFO(unit)->port_info[vp].flags |= 
                             _BCM_MIM_PORT_TYPE_PEER;
        } else {
            MIM_INFO(unit)->port_info[vp].flags |= 
                             _BCM_MIM_PORT_TYPE_NETWORK;
        }
        soc_mem_field32_set(unit, mpls_mem, &ment, MIM_NVP__SVPf, vp);
       
        rv = soc_mem_insert(unit, mpls_mem, MEM_BLOCK_ALL, &ment);
        BCM_IF_ERROR_RETURN(rv);
        SAL_MAC_ADDR_TO_UINT64(mim_port->match_tunnel_srcmac, temp_mac);
        SAL_MAC_ADDR_FROM_UINT64
           (MIM_INFO(unit)->port_info[vp].match_tunnel_srcmac, temp_mac); 
        MIM_INFO(unit)->port_info[vp].match_tunnel_vlan = 
                         mim_port->match_tunnel_vlan;
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_mim_match_delete
 * Purpose: 
 *      Delete MIM match parameters
 *
 * Parameters: 
 *      unit       - (IN) bcm device
 *      vp         - (IN) virtual port
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

int
_bcm_tr3_mim_match_delete(int unit, int vp)
{
    int port, rv;

    if (MIM_INFO(unit)->port_info[vp].flags & 
        _BCM_MIM_PORT_TYPE_ACCESS_PORT_VLAN) {
        vlan_xlate_extd_entry_t vent;
        soc_mem_t   xlate_mem;
        sal_memset(&vent, 0, sizeof(vent));

        xlate_mem = VLAN_XLATE_EXTDm;
        soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_0f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_1f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_mem_field32_set(unit, xlate_mem, &vent, VALID_0f, 0x1);
        soc_mem_field32_set(unit, xlate_mem, &vent, VALID_1f, 0x1);

        /* vlan xlate key */
        soc_mem_field32_set(unit, xlate_mem, &vent, OVIDf,
                                    MIM_INFO(unit)->port_info[vp].match_vlan);
        if (MIM_INFO(unit)->port_info[vp].modid != -1) {
            soc_mem_field32_set(unit, xlate_mem, &vent, MODULE_IDf, 
                                        MIM_INFO(unit)->port_info[vp].modid);
            soc_mem_field32_set(unit, xlate_mem, &vent, PORT_NUMf, 
                                        MIM_INFO(unit)->port_info[vp].port);
        } else {
            soc_mem_field32_set(unit, xlate_mem, &vent, Tf, 1);
            soc_mem_field32_set(unit, xlate_mem, &vent, TGIDf, 
                                        MIM_INFO(unit)->port_info[vp].tgid);
        }
        rv = soc_mem_delete(unit, xlate_mem, MEM_BLOCK_ANY, &vent);
        if (rv != BCM_E_NOT_FOUND) {
            _bcm_tr2_mim_port_match_count_adjust(unit, vp, -1);
            return rv; /* Match has been deleted by bcm_port_match_delete */
        }

    } else if (MIM_INFO(unit)->port_info[vp].flags & 
               _BCM_MIM_PORT_TYPE_ACCESS_PORT_VLAN_STACKED) {
        vlan_xlate_extd_entry_t vent;
        soc_mem_t   xlate_mem;
        sal_memset(&vent, 0, sizeof(vent));

        xlate_mem = VLAN_XLATE_EXTDm;
        soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_0f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_IVID_OVID);
        soc_mem_field32_set(unit, xlate_mem, &vent, KEY_TYPE_1f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_mem_field32_set(unit, xlate_mem, &vent, VALID_0f, 0x1);
        soc_mem_field32_set(unit, xlate_mem, &vent, VALID_1f, 0x1);

        /* vlan xlate key */
        soc_mem_field32_set(unit, xlate_mem, &vent, OVIDf,
                                    MIM_INFO(unit)->port_info[vp].match_vlan);
        soc_mem_field32_set(unit, xlate_mem, &vent, IVIDf,
                              MIM_INFO(unit)->port_info[vp].match_inner_vlan);
        if (MIM_INFO(unit)->port_info[vp].modid != -1) {
            soc_mem_field32_set(unit, xlate_mem, &vent, MODULE_IDf, 
                                        MIM_INFO(unit)->port_info[vp].modid);
            soc_mem_field32_set(unit, xlate_mem, &vent, PORT_NUMf, 
                                        MIM_INFO(unit)->port_info[vp].port);
        } else {
            soc_mem_field32_set(unit, xlate_mem, &vent, Tf, 1);
            soc_mem_field32_set(unit, xlate_mem, &vent, TGIDf, 
                                        MIM_INFO(unit)->port_info[vp].tgid);
        }
        rv = soc_mem_delete(unit, xlate_mem, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);   

    } else if (MIM_INFO(unit)->port_info[vp].flags & 
               _BCM_MIM_PORT_TYPE_ACCESS_PORT) {
        rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                    MIM_INFO(unit)->port_info[vp].index, 
                                    SOURCE_VPf, 0);        
        BCM_IF_ERROR_RETURN(rv);
        port = MIM_INFO(unit)->port_info[vp].index & SOC_PORT_ADDR_MAX(unit);
        rv = soc_mem_field32_modify(unit, PORT_TABm, port,
                                    PORT_OPERATIONf, 0x0); /* NORMAL */
        BCM_IF_ERROR_RETURN(rv);

    } else if(MIM_INFO(unit)->port_info[vp].flags & 
              _BCM_MIM_PORT_TYPE_ACCESS_PORT_TRUNK) {
        int trunk_id;
        trunk_id = MIM_INFO(unit)->port_info[vp].index;
        rv = _bcm_tr_mim_match_trunk_delete(unit, trunk_id, vp);
        BCM_IF_ERROR_RETURN(rv);
    } else if (MIM_INFO(unit)->port_info[vp].flags & 
               _BCM_MIM_PORT_TYPE_ACCESS_LABEL) {
        mpls_entry_entry_t ment; 
        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        if (MIM_INFO(unit)->port_info[vp].modid != -1) {
            soc_MPLS_ENTRYm_field32_set(unit, &ment, MODULE_IDf, 
                                        MIM_INFO(unit)->port_info[vp].modid);
            soc_MPLS_ENTRYm_field32_set(unit, &ment, PORT_NUMf, 
                                        MIM_INFO(unit)->port_info[vp].port);
        } else {
            soc_MPLS_ENTRYm_field32_set(unit, &ment, Tf, 1);
            soc_MPLS_ENTRYm_field32_set(unit, &ment, TGIDf, 
                                        MIM_INFO(unit)->port_info[vp].tgid);
        }
        soc_MPLS_ENTRYm_field32_set(unit, &ment, MPLS_LABELf,
                                    MIM_INFO(unit)->port_info[vp].match_label);
        rv = soc_mem_delete(unit, MPLS_ENTRYm, MEM_BLOCK_ANY, &ment);
        BCM_IF_ERROR_RETURN(rv);

    } else if (MIM_INFO(unit)->port_info[vp].flags & 
               (_BCM_MIM_PORT_TYPE_NETWORK | _BCM_MIM_PORT_TYPE_PEER)) {
        mpls_entry_extd_entry_t ment;
        soc_mem_t mpls_mem;

        sal_memset(&ment, 0, sizeof(ment));
        mpls_mem = MPLS_ENTRY_EXTDm;
        soc_mem_field32_set(unit, mpls_mem, &ment, KEY_TYPE_0f, 0x17); /* MIM NVP */
        soc_mem_field32_set(unit, mpls_mem, &ment, KEY_TYPE_1f, 0x17);
        soc_mem_field32_set(unit, mpls_mem, &ment, VALID_0f, 1);
        soc_mem_field32_set(unit, mpls_mem, &ment, VALID_1f, 1);
        
        /* MIM_NVP key */
        soc_mem_field32_set(unit, mpls_mem, &ment, MIM_NVP__BVIDf,
                               MIM_INFO(unit)->port_info[vp].match_tunnel_vlan);
        soc_mem_mac_addr_set(unit, mpls_mem, &ment, MIM_NVP__BMACSAf, 
                             MIM_INFO(unit)->port_info[vp].match_tunnel_srcmac);
        rv = soc_mem_delete(unit, mpls_mem, MEM_BLOCK_ALL, &ment);
        BCM_IF_ERROR_RETURN(rv);
       
        /* Delete L3_Tunnel MIM entry for peer port */ 
        if (MIM_INFO(unit)->port_info[vp].flags & _BCM_MIM_PORT_TYPE_PEER) {
            soc_mem_t tunnel_mem;
            soc_tunnel_term_t tnl_entry;

            sal_memset(&tnl_entry, 0, sizeof(tnl_entry));
            tunnel_mem = L3_TUNNELm;
           
            /* MIM L3 TUNNEL Type */
            soc_mem_field32_set(unit, tunnel_mem, &tnl_entry, KEY_TYPEf, 0x3);
 
            /* MIM L3 TUNNEL KEY BMACSA+BVID */
            soc_mem_mac_addr_set(unit, tunnel_mem, &tnl_entry, MIM__BMACSAf, 
                                                MIM_INFO(unit)->port_info[vp].match_tunnel_srcmac);
            soc_mem_field32_set(unit, tunnel_mem, &tnl_entry, MIM__BVIDf,
                                                 MIM_INFO(unit)->port_info[vp].match_tunnel_vlan);
            rv = soc_tunnel_term_delete(unit, &tnl_entry);
            BCM_IF_ERROR_RETURN(rv);
        }
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_tr3_mim_peer_port_config_add
 * Purpose: 
 *      Add ISID and VP for peer ports.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      mim_port   - (IN) mim port
 *      vp         - (IN) virtual port
 *      vpn        - (IN) VPN ID (VFI)
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_tr3_mim_peer_port_config_add(int unit, bcm_mim_port_t *mim_port, 
                                  int vp, bcm_mim_vpn_t vpn) 
{
    mpls_entry_extd_entry_t ment;
    egr_vlan_xlate_entry_t egr_vlan_xlate_entry;
    int index, vfi, rv = BCM_E_NONE;

    /* Need an ISID-SVP entry in MPLS_ENTRY */
    _BCM_MIM_VPN_GET(vfi, _BCM_MIM_VPN_TYPE_MIM, vpn);

    sal_memset(&ment, 0, sizeof(ment));
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, KEY_TYPE_0f, 0x19); /* MIM_ISID_SVP */
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, KEY_TYPE_1f, 0x19); /* MIM_ISID_SVP */
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, VALID_0f, 0x1);
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, VALID_1f, 0x1);
    /* MIM_ISID_SVP key */ 
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, MIM_ISID__ISIDf, 
                                VPN_ISID(unit, vfi));

    /* MIM_ISID_SVP data */ 
    
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, MIM_ISID__VFIf, vfi);
    rv = soc_mem_search(unit, MPLS_ENTRY_EXTDm, MEM_BLOCK_ANY, &index,
                        &ment, &ment, 0);
    if (rv == SOC_E_NONE) {
        return BCM_E_EXISTS;
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    }
    BCM_IF_ERROR_RETURN(soc_mem_insert(unit, MPLS_ENTRY_EXTDm, MEM_BLOCK_ALL, &ment));

    /* Also need the reverse entry in the egress */
    sal_memset(&egr_vlan_xlate_entry, 0, sizeof(egr_vlan_xlate_entry_t));
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, KEY_TYPEf, 
                                    0x3);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, VALIDf, 0x1);
    /* MIM_ISID_DVP key */
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, MIM_ISID__VFIf, 
                                    vfi);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, 
                                    MIM_ISID__DVPf, vp);
    /* MIM_ISID_DVP data */
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry,  
                                    MIM_ISID__ISIDf, VPN_ISID(unit, vfi));

    /* Deal with egress SD tag actions */
    if (mim_port->flags & BCM_MIM_PORT_EGRESS_SERVICE_VLAN_TAGGED) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_mim_egr_vxlt_sd_tag_actions(unit, mim_port, 
                                                  &egr_vlan_xlate_entry));
    }

    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        &egr_vlan_xlate_entry, &egr_vlan_xlate_entry, 0);
    if (rv == SOC_E_NONE) {
        return BCM_E_EXISTS;
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    }
    rv = soc_mem_insert(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &egr_vlan_xlate_entry);

    return rv;
}

/*
 * Function:
 *      _bcm_tr3_mim_peer_port_config_delete
 * Purpose: 
 *      Delete MPLS_ENTRy and EGR_VLAN_XLATE ISID/VFI instances
 *
 * Parameters: 
 *      unit       - (IN) bcm device
 *      vp         - (IN) vp
 *      vpn        - (IN) VPN Id ( VFI)
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_tr3_mim_peer_port_config_delete(int unit, int vp, bcm_mim_vpn_t vpn) 
{
    mpls_entry_extd_entry_t ment;
    egr_vlan_xlate_entry_t egr_vlan_xlate_entry;
    int vfi, rv = BCM_E_NONE;

    /* Delete the ISID-SVP entry in MPLS_ENTRY */
    _BCM_MIM_VPN_GET(vfi, _BCM_MIM_VPN_TYPE_MIM, vpn);

    sal_memset(&ment, 0, sizeof(ment));
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, KEY_TYPE_0f, 0x19); /* MIM_ISID_SVP */
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, KEY_TYPE_1f, 0x19); /* MIM_ISID_SVP */
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, VALID_0f, 0x1);
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, VALID_1f, 0x1);
    /* MIM_ISID_SVP key */ 
    soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, MIM_ISID__ISIDf, 
                                VPN_ISID(unit, vfi));
    
    rv = soc_mem_delete(unit, MPLS_ENTRY_EXTDm, MEM_BLOCK_ANY, &ment);
    BCM_IF_ERROR_RETURN(rv);

    /* Delete the reverse entry in the egress */
    sal_memset(&egr_vlan_xlate_entry, 0, sizeof(egr_vlan_xlate_entry_t));
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, KEY_TYPEf, 
                                    0x3);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, VALIDf, 0x1);
    /* MIM_ISID_DVP key */
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, MIM_ISID__VFIf, 
                                    vfi);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, 
                                    MIM_ISID__DVPf, vp);
    rv = soc_mem_delete(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY, 
                        &egr_vlan_xlate_entry);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_mim_lookup_id_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given MiM I-SID entry.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      lookup_id        - (IN) I-SID value
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_tr3_mim_lookup_id_stat_get_table_info(
                   int                        unit,
                   int                        lookup_id,
                   uint32                     *num_of_tables,
                   bcm_stat_flex_table_info_t *table_info)
{
    mpls_entry_extd_entry_t ment;
    int index, rv = BCM_E_NONE;
    soc_mem_t mpls_mem;

    /* Get MPLS_ENTRYm MIM_ISID Entry */
    sal_memset(&ment, 0, sizeof(ment));
#if defined(BCM_TRIDENT2_SUPPORT)    
    if (SOC_IS_TRIDENT2(unit)) {
        mpls_mem = MPLS_ENTRYm;
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, 0x2); /* MIM_ISID */
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 0x1);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        mpls_mem = MPLS_ENTRY_EXTDm;
        soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, 
                                         KEY_TYPE_0f, 0x18); /* MIM_ISID */
        soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, 
                                         KEY_TYPE_1f, 0x18); /* MIM_ISID */
        soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, VALID_0f, 0x1);
        soc_MPLS_ENTRY_EXTDm_field32_set(unit, &ment, VALID_1f, 0x1);
    }
    /* MIM_ISID key */ 
    soc_mem_field32_set(unit, mpls_mem, &ment, MIM_ISID__ISIDf, lookup_id);

    rv = soc_mem_search(unit, mpls_mem, MEM_BLOCK_ANY, &index,
                        &ment, &ment, 0);
    if (rv == BCM_E_NONE) {
        table_info[*num_of_tables].table=mpls_mem;
        table_info[*num_of_tables].index=index;
        table_info[*num_of_tables].direction=bcmStatFlexDirectionIngress;
        (*num_of_tables)++;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_tr3_mim_vpn_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given MiM VPN entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vpn              - (IN) MiM VPN
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_tr3_mim_vpn_stat_get_table_info(
                   int                        unit,
                   bcm_mim_vpn_t              vpn,
                   uint32                     *num_of_tables,
                   bcm_stat_flex_table_info_t *table_info)
{
    egr_vlan_xlate_entry_t egr_vlan_xlate_entry;
    int index, rv = BCM_E_NONE, vfi;


    /* Get EGR_VLAN_XLATEm MIM_ISID entry */
    sal_memset(&egr_vlan_xlate_entry, 0, sizeof(egr_vlan_xlate_entry_t));
#if defined(BCM_TRIDENT2_SUPPORT)    
    if (SOC_IS_TRIDENT2(unit)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, 
                                        ENTRY_TYPEf, 0x3); /* MIM_ISID view */
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry,
                                        KEY_TYPEf, 0x2); /* MIM_ISID view */
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, VALIDf, 0x1);
    /* key */ 
    _BCM_MIM_VPN_GET(vfi, _BCM_MIM_VPN_TYPE_MIM, vpn);

    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vlan_xlate_entry, MIM_ISID__VFIf, 
                                    vfi);

    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        &egr_vlan_xlate_entry, &egr_vlan_xlate_entry, 0);
    if (rv == SOC_E_NONE) {
        table_info[*num_of_tables].table=EGR_VLAN_XLATEm;
        table_info[*num_of_tables].index=index;
        table_info[*num_of_tables].direction=bcmStatFlexDirectionEgress;
        (*num_of_tables)++;
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_mim_peer_port_config_delete
 * Purpose: 
 *
 *
 * Parameters: 
 *      unit       - (IN) bcm device
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

#endif /* defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3) */
