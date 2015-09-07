/*
 * $Id: l3.c 1.29.2.1 Broadcom SDK $
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
 * File:        l3.c
 * Purpose:     Triumph2 L3 function implementations
 */


#include <soc/defs.h>

#include <assert.h>

#include <sal/core/libc.h>
#if defined(BCM_TRIUMPH2_SUPPORT)  && defined(INCLUDE_L3)

#include <shared/util.h>
#include <soc/mem.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/l3x.h>
#include <soc/lpm.h>
#include <soc/tnl_term.h>
#include <soc/defragment.h>

#include <bcm/l3.h>
#include <bcm/tunnel.h>
#include <bcm/debug.h>
#include <bcm/error.h>
#include <bcm/stack.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw_dispatch.h>
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT*/

/* L3 ECMP table defragmentation buffer info */
typedef struct _bcm_tr2_l3_ecmp_defragment_buffer_s {
    int base_ptr; /* Starting index of the buffer used
                     in ECMP table defragmentation */
    int size;     /* Size of the buffer */
} _bcm_tr2_l3_ecmp_defragment_buffer_t;

STATIC _bcm_tr2_l3_ecmp_defragment_buffer_t
       *_bcm_tr2_l3_ecmp_defragment_buffer_info[BCM_MAX_NUM_UNITS];

/*
 * Function:
 *      _bcm_tr2_l3_tnl_term_add
 * Purpose:
 *      Add tunnel termination entry to the hw.
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      tnl_info - (IN)Tunnel terminator parameters. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_l3_tnl_term_add(int unit, uint32 *entry_ptr, bcm_tunnel_terminator_t *tnl_info)
{
    bcm_module_t mod_in, mod_out, my_mod;
    bcm_port_t port_in, port_out;
    _bcm_l3_ingress_intf_t iif;
    int tunnel, wlan;
    int rv;

    /* Program remote port */
    if ((tnl_info->type == bcmTunnelTypeWlanWtpToAc) || 
        (tnl_info->type == bcmTunnelTypeWlanAcToAc) ||
        (tnl_info->type == bcmTunnelTypeWlanWtpToAc6) ||
        (tnl_info->type == bcmTunnelTypeWlanAcToAc6)) {
        wlan = 1;
    } else {
        wlan = 0;
    }
    if (wlan) {
        if (tnl_info->flags & BCM_TUNNEL_TERM_WLAN_REMOTE_TERMINATE) {
            if (!BCM_GPORT_IS_MODPORT(tnl_info->remote_port)) {
                return BCM_E_PARAM;
            }
            mod_in = BCM_GPORT_MODPORT_MODID_GET(tnl_info->remote_port);
            port_in = BCM_GPORT_MODPORT_PORT_GET(tnl_info->remote_port); 
            BCM_IF_ERROR_RETURN
                (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET, mod_in, 
                                        port_in, &mod_out, &port_out));
            if (!SOC_MODID_ADDRESSABLE(unit, mod_out)) {
                return (BCM_E_BADID);
            }
            if (!SOC_PORT_ADDRESSABLE(unit, port_out)) {
                return (BCM_E_PORT);
            }
            soc_L3_TUNNELm_field32_set(unit, entry_ptr, REMOTE_TERM_GPPf,
                                       (mod_out << 6) | port_out);
        } else {
            /* Send to the local loopback port */
            rv = bcm_esw_stk_my_modid_get(unit, &my_mod);
            BCM_IF_ERROR_RETURN(rv);
            port_in = 54;
            mod_in = my_mod;
            rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET, mod_in, 
                                        port_in, &mod_out, &port_out);
            BCM_IF_ERROR_RETURN(rv);

            soc_L3_TUNNELm_field32_set(unit, entry_ptr, REMOTE_TERM_GPPf,
                                       (mod_out << 6) | port_out);
        }
        /* Program tunnel id */
        if (tnl_info->flags & BCM_TUNNEL_TERM_TUNNEL_WITH_ID) {
            if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
                return BCM_E_PARAM;
            }
            if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, TUNNEL_IDf)) {
                tunnel = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);
                soc_L3_TUNNELm_field32_set(unit, entry_ptr, TUNNEL_IDf,
                                           tunnel);
            }
        }

    }
    if ((tnl_info->type == bcmTunnelTypeAutoMulticast) ||
        (tnl_info->type == bcmTunnelTypeAutoMulticast6)) {
        /* Program L3_IIFm */
        if(SOC_MEM_FIELD_VALID(unit, L3_IIFm, IPMC_L3_IIFf)) {
            sal_memset(&iif, 0, sizeof(_bcm_l3_ingress_intf_t));
            iif.intf_id = tnl_info->vlan;

            rv = _bcm_tr_l3_ingress_interface_get(unit, &iif);
            BCM_IF_ERROR_RETURN(rv);
            iif.vrf = tnl_info->vrf;
#if defined(BCM_TRIDENT2_SUPPORT)
            if (soc_feature(unit, soc_feature_l3_iif_profile)) {
                iif.profile_flags |= _BCM_L3_IIF_PROFILE_DO_NOT_UPDATE;
            }
#endif /* BCM_TRIDENT2_SUPPORT */
            rv = _bcm_tr_l3_ingress_interface_set(unit, &iif);
            BCM_IF_ERROR_RETURN(rv);
        }

        /* Program tunnel id */
        if (tnl_info->flags & BCM_TUNNEL_TERM_TUNNEL_WITH_ID) {
            if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
                return BCM_E_PARAM;
            }
            if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, TUNNEL_IDf)) {
                tunnel = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);
                soc_L3_TUNNELm_field32_set(unit, entry_ptr, TUNNEL_IDf,
                                           tunnel);
            }
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_l3_tnl_term_entry_init
 * Purpose:
 *      Initialize soc tunnel terminator entry key portion.
 * Parameters:
 *      unit     - (IN)  BCM device number. 
 *      tnl_info - (IN)  BCM buffer with tunnel info.
 *      entry    - (OUT) SOC buffer with key filled in.  
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_l3_tnl_term_entry_init(int unit, bcm_tunnel_terminator_t *tnl_info,
                                soc_tunnel_term_t *entry)
{
    int       idx;                /* Entry iteration index.     */
    int       idx_max;            /* Entry widht.               */
    uint32    *entry_ptr;         /* Filled entry pointer.      */
    _bcm_tnl_term_type_t tnl_type;/* Tunnel type.               */
    int       rv;                 /* Operation return status.   */

    /* Input parameters check. */
    if ((NULL == tnl_info) || (NULL == entry)) {
        return (BCM_E_PARAM);
    }

    /* Get tunnel type & sub_type. */
    BCM_IF_ERROR_RETURN
        (_bcm_xgs3_l3_set_tnl_term_type(unit, tnl_info, &tnl_type));

    /* Reset destination structure. */
    sal_memset(entry, 0, sizeof(soc_tunnel_term_t));

    /* Set Destination/Source pair. */
    entry_ptr = (uint32 *)&entry->entry_arr[0];
    if (tnl_type.tnl_outer_hdr_ipv6 == 1) {
        /* Apply mask on source address. */
        rv = bcm_xgs3_l3_mask6_apply(tnl_info->sip6_mask, tnl_info->sip6);
        BCM_IF_ERROR_RETURN(rv);

        /* SIP [0-63] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[0], IP_ADDRf,
                             tnl_info->sip6, SOC_MEM_IP6_LOWER_ONLY);
        /* SIP [64-127] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[1], IP_ADDRf,
                             tnl_info->sip6, SOC_MEM_IP6_UPPER_ONLY);
        /* DIP [0-63] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[2], IP_ADDRf,
                             tnl_info->dip6, SOC_MEM_IP6_LOWER_ONLY);
        /* DIP [64-127] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[3], IP_ADDRf,
                             tnl_info->dip6, SOC_MEM_IP6_UPPER_ONLY);

        /* SIP MASK [0-63] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[0], IP_ADDR_MASKf,
                             tnl_info->sip6_mask, SOC_MEM_IP6_LOWER_ONLY);
        /* SIP MASK [64-127] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[1], IP_ADDR_MASKf,
                             tnl_info->sip6_mask, SOC_MEM_IP6_UPPER_ONLY);
        /* DIP MASK [0-63] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[2], IP_ADDR_MASKf,
                             tnl_info->dip6_mask, SOC_MEM_IP6_LOWER_ONLY);
        /* DIP MASK [64-127] */
        soc_mem_ip6_addr_set(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[3], IP_ADDR_MASKf,
                             tnl_info->dip6_mask, SOC_MEM_IP6_UPPER_ONLY);
    }  else if (tnl_type.tnl_outer_hdr_ipv6 == 0) {
        tnl_info->sip &= tnl_info->sip_mask;

        /* Set destination ip. */
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, DIPf, tnl_info->dip);

        /* Set source ip. */
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, SIPf, tnl_info->sip);

        /* Set destination subnet mask. */
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, DIP_MASKf,
                                   tnl_info->dip_mask);

        /* Set source subnet mask. */
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, SIP_MASKf,
                                   tnl_info->sip_mask);
    }
    /* Resolve number of entries hw entry occupies. */
    idx_max = (tnl_type.tnl_outer_hdr_ipv6 == 1) ? SOC_TNL_TERM_IPV6_ENTRY_WIDTH : \
              (tnl_type.tnl_outer_hdr_ipv6 == 0) ? SOC_TNL_TERM_IPV4_ENTRY_WIDTH : 0; 

    for (idx = 0; idx < idx_max; idx++) {
        entry_ptr = (uint32 *)&entry->entry_arr[idx];

        /* Set valid bit. */
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, VALIDf, 1);

        /* Set tunnel subtype. */
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, SUB_TUNNEL_TYPEf,
                                   tnl_type.tnl_sub_type);

        /* Set tunnel type. */
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, TUNNEL_TYPEf,
                                   tnl_type.tnl_auto);

        /* Set entry mode. */

    if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, MODEf)) {
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, MODEf,
                                   tnl_type.tnl_outer_hdr_ipv6);
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, MODE_MASKf, 1);
    } else if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, KEY_TYPEf)) {
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, KEY_TYPEf,
                tnl_type.tnl_outer_hdr_ipv6);
        soc_L3_TUNNELm_field32_set(unit, entry_ptr, KEY_TYPE_MASKf, 1);
    }

        if (0 == idx) {
            /* Set the PROTOCOL field */
            if ((tnl_info->type == bcmTunnelTypeIpAnyIn4) || 
                (tnl_info->type == bcmTunnelTypeIpAnyIn6))
            {
              /* Set PROTOCOL and PROTOCOL_MASK field to zero for IpAnyInx tunnel type*/
               soc_L3_TUNNELm_field32_set(unit, entry_ptr, PROTOCOLf, 0x0);
               soc_L3_TUNNELm_field32_set(unit, entry_ptr, PROTOCOL_MASKf, 0x0);
            } else {            
                soc_L3_TUNNELm_field32_set(unit, entry_ptr, PROTOCOLf,
                                           tnl_type.tnl_protocol);
                soc_L3_TUNNELm_field32_set(unit, entry_ptr, 
                                           PROTOCOL_MASKf, 0xff);
            }
        }
        
        if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, BFD_ENABLEf)) {
            soc_L3_TUNNELm_field32_set(unit, entry_ptr, BFD_ENABLEf, 0);
        }

        if ((tnl_info->type == bcmTunnelTypeWlanWtpToAc) ||
            (tnl_info->type == bcmTunnelTypeWlanAcToAc) ||
            (tnl_info->type == bcmTunnelTypeWlanWtpToAc6) ||
            (tnl_info->type == bcmTunnelTypeWlanAcToAc6)) { 

            /* Set the L4 ports - WLAN/AMT tunnels */
            if (0 == idx) {
                soc_L3_TUNNELm_field32_set(unit, entry_ptr, L4_DEST_PORTf,
                                           tnl_info->udp_dst_port);

                soc_L3_TUNNELm_field32_set(unit, entry_ptr,
                                           L4_DEST_PORT_MASKf, 0xffff);

                soc_L3_TUNNELm_field32_set(unit, entry_ptr, L4_SRC_PORTf,
                                           tnl_info->udp_src_port);
    
                soc_L3_TUNNELm_field32_set(unit, entry_ptr,
                                           L4_SRC_PORT_MASKf, 0xffff);
            }

            /* Set UDP tunnel type. */
            soc_L3_TUNNELm_field32_set(unit, entry_ptr, UDP_TUNNEL_TYPEf,
                                       tnl_type.tnl_udp_type);
            /* Ignore UDP checksum */
            soc_L3_TUNNELm_field32_set(unit, entry_ptr, IGNORE_UDP_CHECKSUMf,
   				       0x1);
        } else if (tnl_info->type == bcmTunnelTypeAutoMulticast) {

		/* Set UDP tunnel type. */
		soc_L3_TUNNELm_field32_set(unit, entry_ptr, UDP_TUNNEL_TYPEf,
					   tnl_type.tnl_udp_type);

		soc_L3_TUNNELm_field32_set(unit, entry_ptr, IGNORE_UDP_CHECKSUMf,
					   0x1);
		soc_L3_TUNNELm_field32_set(unit, entry_ptr, CTRL_PKTS_TO_CPUf,
                       0x1);
        } else if (tnl_info->type == bcmTunnelTypeAutoMulticast6) {

		/* Set UDP tunnel type. */
              soc_L3_TUNNELm_field32_set(unit, entry_ptr, UDP_TUNNEL_TYPEf,
						   tnl_type.tnl_udp_type);	
              soc_L3_TUNNELm_field32_set(unit, entry_ptr, IGNORE_UDP_CHECKSUMf,
						   0x1);
              soc_L3_TUNNELm_field32_set(unit, entry_ptr, CTRL_PKTS_TO_CPUf,
						   0x1);
        }

        /* Save vlan id for ipmc lookup.*/
        if((tnl_info->vlan) && SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, L3_IIFf)) {                 
            soc_L3_TUNNELm_field32_set(unit, entry_ptr, L3_IIFf, tnl_info->vlan);
        }

        /* Set GRE payload */
        if (tnl_type.tnl_gre) {
            /* GRE IPv6 payload is allowed. */
            soc_L3_TUNNELm_field32_set(unit, entry_ptr, GRE_PAYLOAD_IPV6f,
                                       tnl_type.tnl_gre_v6_payload);

            /* GRE IPv6 payload is allowed. */
            soc_L3_TUNNELm_field32_set(unit, entry_ptr, GRE_PAYLOAD_IPV4f,
                                       tnl_type.tnl_gre_v4_payload);
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_l3_tnl_term_entry_parse
 * Purpose:
 *      Parse tunnel terminator entry portion.
 * Parameters:
 *      unit     - (IN)  BCM device number. 
 *      entry    - (IN)  SOC buffer with tunne information.  
 *      tnl_info - (OUT) BCM buffer with tunnel info.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_l3_tnl_term_entry_parse(int unit, soc_tunnel_term_t *entry,
                                  bcm_tunnel_terminator_t *tnl_info)
{
    _bcm_tnl_term_type_t tnl_type;     /* Tunnel type information.   */
    uint32 *entry_ptr;                 /* Filled entry pointer.      */
    int tunnel_id;                     /* Tunnel ID */
    int remote_port;                   /* Remote port */
    bcm_module_t mod, mod_out, my_mod; /* Module IDs */
    bcm_port_t port, port_out;         /* Physical ports */

    /* Input parameters check. */
    if ((NULL == tnl_info) || (NULL == entry)) {
        return (BCM_E_PARAM);
    }

    /* Reset destination structure. */
    sal_memset(tnl_info, 0, sizeof(bcm_tunnel_terminator_t));
    sal_memset(&tnl_type, 0, sizeof(_bcm_tnl_term_type_t));

    entry_ptr = (uint32 *)&entry->entry_arr[0];

    /* Get valid bit. */
    if (!soc_L3_TUNNELm_field32_get(unit, entry_ptr, VALIDf)) {
        return (BCM_E_NOT_FOUND);
    }

    if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, MODEf)) {
        tnl_type.tnl_outer_hdr_ipv6 =
            soc_L3_TUNNELm_field32_get(unit, entry_ptr, MODEf);
    } else if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, KEY_TYPEf)) {
        tnl_type.tnl_outer_hdr_ipv6 =
            soc_L3_TUNNELm_field32_get(unit, entry_ptr, KEY_TYPEf);

    }
     
    /* Get Destination/Source pair. */
    if (tnl_type.tnl_outer_hdr_ipv6 == 1) {
        /* SIP [0-63] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[0], IP_ADDRf,
                             tnl_info->sip6, SOC_MEM_IP6_LOWER_ONLY);
        /* SIP [64-127] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[1], IP_ADDRf,
                             tnl_info->sip6, SOC_MEM_IP6_UPPER_ONLY);
        /* DIP [0-63] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[2], IP_ADDRf,
                             tnl_info->dip6, SOC_MEM_IP6_LOWER_ONLY);
        /* DIP [64-127] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[3], IP_ADDRf,
                             tnl_info->dip6, SOC_MEM_IP6_UPPER_ONLY);

        /* SIP MASK [0-63] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[0], IP_ADDR_MASKf,
                             tnl_info->sip6_mask, SOC_MEM_IP6_LOWER_ONLY);
        /* SIP MASK [64-127] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[1], IP_ADDR_MASKf,
                             tnl_info->sip6_mask, SOC_MEM_IP6_UPPER_ONLY);
        /* DIP MASK [0-63] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[2], IP_ADDR_MASKf,
                             tnl_info->dip6_mask, SOC_MEM_IP6_LOWER_ONLY);
        /* DIP MASK [64-127] */
        soc_mem_ip6_addr_get(unit, L3_TUNNELm, 
                             (uint32 *)&entry->entry_arr[3], IP_ADDR_MASKf,
                             tnl_info->dip6_mask, SOC_MEM_IP6_UPPER_ONLY);

    }  else if (tnl_type.tnl_outer_hdr_ipv6 == 0) {
        /* Get destination ip. */
        tnl_info->dip = soc_L3_TUNNELm_field32_get(unit, entry_ptr, DIPf);

        /* Get source ip. */
        tnl_info->sip = soc_L3_TUNNELm_field32_get(unit, entry_ptr, SIPf);

        /* Destination subnet mask. */
        tnl_info->dip_mask = BCM_XGS3_L3_IP4_FULL_MASK;

        /* Source subnet mask. */
        tnl_info->sip_mask = soc_L3_TUNNELm_field32_get(unit, entry_ptr, 
                                                        SIP_MASKf);
    }

    /* Get tunnel subtype. */
    tnl_type.tnl_sub_type = 
        soc_L3_TUNNELm_field32_get(unit, entry_ptr, SUB_TUNNEL_TYPEf);

    /* Get UDP tunnel type. */
    tnl_type.tnl_udp_type = 
        soc_L3_TUNNELm_field32_get(unit, entry_ptr, UDP_TUNNEL_TYPEf);

    /* Get tunnel type. */
    tnl_type.tnl_auto = 
        soc_L3_TUNNELm_field32_get(unit, entry_ptr, TUNNEL_TYPEf);

    /* Copy DSCP from outer header flag. */
    if (soc_L3_TUNNELm_field32_get(unit, entry_ptr, USE_OUTER_HDR_DSCPf)) {
        tnl_info->flags |= BCM_TUNNEL_TERM_USE_OUTER_DSCP;
    }
    /* Copy TTL from outer header flag. */
    if (soc_L3_TUNNELm_field32_get(unit, entry_ptr, USE_OUTER_HDR_TTLf)) {
        tnl_info->flags |= BCM_TUNNEL_TERM_USE_OUTER_TTL;
    }
    /* Keep inner header DSCP flag. */
    if (soc_L3_TUNNELm_field32_get(unit, entry_ptr,
                                   DONOT_CHANGE_INNER_HDR_DSCPf)) {
        tnl_info->flags |= BCM_TUNNEL_TERM_KEEP_INNER_DSCP;
    }

    soc_mem_pbmp_field_get(unit, L3_TUNNELm, entry_ptr, ALLOWED_PORT_BITMAPf,
                           &tnl_info->pbmp);

    /* Tunnel or IPMC lookup vlan id */
    tnl_info->vlan = soc_L3_TUNNELm_field32_get(unit, entry_ptr, IINTFf);

    /*  Get trust dscp per tunnel */ 
    if (soc_L3_TUNNELm_field32_get(unit, entry_ptr, USE_OUTER_HDR_DSCPf)) {
        tnl_info->flags |= BCM_TUNNEL_TERM_DSCP_TRUST;
    }

    /* Get the protocol field and make some decisions */
    tnl_type.tnl_protocol = soc_L3_TUNNELm_field32_get(unit, entry_ptr, 
                                                       PROTOCOLf);
    switch (tnl_type.tnl_protocol) {
        case 0x2F:
            tnl_type.tnl_gre = 1;
            break;
        case 0x67:
            tnl_type.tnl_pim_sm = 1;
        default:
            break;
    }
    /* Get gre IPv4 payload allowed. */
    tnl_type.tnl_gre_v4_payload = 
        soc_L3_TUNNELm_field32_get(unit, entry_ptr, GRE_PAYLOAD_IPV4f); 

    /* Get gre IPv6 payload allowed. */
    tnl_type.tnl_gre_v6_payload = 
        soc_L3_TUNNELm_field32_get(unit, entry_ptr, GRE_PAYLOAD_IPV6f);

    /* Get the L4 data */
    if (soc_mem_field_valid (unit, L3_TUNNELm, L4_SRC_PORTf)) {
        tnl_info->udp_src_port = soc_L3_TUNNELm_field32_get(unit, entry_ptr, 
                                                        L4_SRC_PORTf);
    }
    if (soc_mem_field_valid (unit, L3_TUNNELm, L4_DEST_PORTf)) {
        tnl_info->udp_dst_port = soc_L3_TUNNELm_field32_get(unit, entry_ptr, 
                                                        L4_DEST_PORTf);
    }
    /* Get the tunnel ID */
    if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, TUNNEL_IDf)) {
        tunnel_id = soc_L3_TUNNELm_field32_get(unit, entry_ptr, TUNNEL_IDf);
        if (tunnel_id) {
            BCM_GPORT_TUNNEL_ID_SET(tnl_info->tunnel_id, tunnel_id);
            tnl_info->flags |= BCM_TUNNEL_TERM_TUNNEL_WITH_ID;
        }
    }

    /* Get the remote port member */
    if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, REMOTE_TERM_GPPf)) {
        remote_port = soc_L3_TUNNELm_field32_get(unit, entry_ptr,
                                                 REMOTE_TERM_GPPf);
        mod = (remote_port >> 6) & 0x7F;
        port = remote_port & 0x3F;
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                    mod, port, &mod_out, &port_out));
        BCM_GPORT_MODPORT_SET(tnl_info->remote_port, mod_out, port_out);
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_mod));
        if (mod != my_mod) {
            tnl_info->flags |= BCM_TUNNEL_TERM_WLAN_REMOTE_TERMINATE;
        } 
    }
                                
    /* Get tunnel type, sub_type and protocol. */
    BCM_IF_ERROR_RETURN
        (_bcm_xgs3_l3_get_tnl_term_type(unit, tnl_info, &tnl_type));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_l3_ecmp_grp_get
 * Purpose:
 *      Get ecmp group next hop members by index.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      ecmp_grp - (IN)Ecmp group id to read. 
 *      ecmp_count - (IN)Maximum number of entries to read.
 *      nh_idx     - (OUT)Next hop indexes. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_l3_ecmp_grp_get(int unit, int ecmp_grp, int ecmp_group_size, int *nh_idx)
{
    int idx;                                /* Iteration index.              */
    int max_ent_count;                      /* Number of entries to read.    */
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.      */
    int one_entry_grp = TRUE;               /* Single next hop entry group.  */ 
    int rv = BCM_E_UNAVAIL;                 /* Operation return status.      */
    int ecmp_idx;

    /* Input parameters sanity check. */
    if ((NULL == nh_idx) || (ecmp_group_size < 1)) {
        return (BCM_E_PARAM);
    }

    /* Zero all next hop indexes first. */
    sal_memset(nh_idx, 0, ecmp_group_size * sizeof(int));
    sal_memset(hw_buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

    /* Get group base pointer. */
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, L3_ECMP_COUNTm,
                                          MEM_BLOCK_ANY, ecmp_grp, hw_buf));
    ecmp_idx = soc_mem_field32_get(unit, L3_ECMP_COUNTm, hw_buf, BASE_PTRf);

    /* Read zero based ecmp count. */
    rv = soc_mem_read(unit, L3_ECMP_COUNTm, MEM_BLOCK_ANY, ecmp_grp, hw_buf);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    max_ent_count = soc_mem_field32_get(unit, L3_ECMP_COUNTm, hw_buf, COUNTf);
    max_ent_count++; /* Count is zero based. */ 

    /* Read all the indexes from hw. */
    for (idx = 0; idx < max_ent_count; idx++) {

        /* Read next hop index. */
        rv = soc_mem_read(unit, L3_ECMPm, MEM_BLOCK_ANY, 
                          (ecmp_idx + idx), hw_buf);
        if (rv < 0) {
            break;
        }
        nh_idx[idx] = soc_mem_field32_get(unit, L3_ECMPm, 
                                          hw_buf, NEXT_HOP_INDEXf);
        /* Check if group contains . */ 
        if (idx && (nh_idx[idx] != nh_idx[0])) { 
            one_entry_grp = FALSE;
        }

        if (0 == soc_feature(unit, soc_feature_l3_dynamic_ecmp_group)) {
            /* Next hops popuplated in cycle,stop once you read first entry again */
            if (idx && (FALSE == one_entry_grp) && (nh_idx[idx] == nh_idx[0])) {
                nh_idx[idx] = 0;
                break;
            }
        } else {
             one_entry_grp = FALSE;
        }
    }
    /* Reset rest of the group if only 1 next hop is present. */
    if (one_entry_grp) {
       sal_memset(nh_idx + 1, 0, (ecmp_group_size - 1) * sizeof(int)); 
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr2_l3_ecmp_grp_add
 * Purpose:
 *      Add ecmp group next hop members, or reset ecmp group entry.  
 *      NOTE: Function always writes all the entries in ecmp group.
 *            If there is not enough nh indexes - next hops written
 *            in cycle. 
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      ecmp_grp   - (IN)ecmp group id to write.
 *      buf        - (IN)Next hop indexes or NULL for entry reset.
 *      max_paths - (IN) ECMP Max paths
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_l3_ecmp_grp_add(int unit, int ecmp_grp, void *buf, int max_paths)
{
    uint32 l3_ecmp[SOC_MAX_MEM_FIELD_WORDS];        /* l3_ecmp buf             */
    uint32 l3_ecmp_count[SOC_MAX_MEM_FIELD_WORDS];  /* l3_ecmp_count buf       */
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS];         /* l3_ecmp_count buf       */
    int ecmp_idx;                                   /* Ecmp table entry index. */
    int *nh_idx;                                    /* Ecmp group nh indexes.  */
    int nh_cycle_idx;
    int entry_type;
    uint32 reg_val, value;
    ing_l3_next_hop_entry_t ing_nh;
    _bcm_l3_tbl_op_t data;
    int max_grp_size = 0;                           /* Maximum ecmp group size.*/
    int idx = 0;                                    /* Iteration index.        */
    int rv = BCM_E_UNAVAIL;                         /* Operation return value. */
    int l3_ecmp_oif_flds[8] = {  L3_OIF_0f, 
                             L3_OIF_1f, 
                             L3_OIF_2f, 
                             L3_OIF_3f, 
                             L3_OIF_4f, 
                             L3_OIF_5f, 
                             L3_OIF_6f, 
                             L3_OIF_7f }; 
    int l3_ecmp_oif_type_flds[8] = {  L3_OIF_0_TYPEf, 
                             L3_OIF_1_TYPEf, 
                             L3_OIF_2_TYPEf, 
                             L3_OIF_3_TYPEf, 
                             L3_OIF_4_TYPEf, 
                             L3_OIF_5_TYPEf, 
                             L3_OIF_6_TYPEf, 
                             L3_OIF_7_TYPEf };

    /* Input parameters check. */
    if (NULL == buf) {
        return (BCM_E_PARAM);
    }

    /* Cast input buffer. */
    nh_idx = (int *) buf;
    max_grp_size = max_paths;

    if (BCM_XGS3_L3_ENT_REF_CNT(BCM_XGS3_L3_TBL_PTR(unit,
                                ecmp_grp), ecmp_grp)) {
        /* Group has already exists, get base ptr from group table */ 
        sal_memset (hw_buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, L3_ECMP_COUNTm,
                            MEM_BLOCK_ANY, ecmp_grp, hw_buf));
        ecmp_idx = soc_mem_field32_get(unit, L3_ECMP_COUNTm, hw_buf, BASE_PTRf);
    } else {  
        /* Get index to the first slot in the ECMP table
         * that can accomodate max_grp_size */
        data.width = max_paths;
        data.tbl_ptr = BCM_XGS3_L3_TBL_PTR(unit, ecmp); 
        data.oper_flags = _BCM_L3_SHR_TABLE_TRAVERSE_CONTROL; 
        data.entry_index = -1;
        rv = _bcm_xgs3_tbl_free_idx_get(unit, &data);
        if (BCM_FAILURE(rv)) {
            if (rv == BCM_E_FULL) {
                /* Defragment ECMP table */
                BCM_IF_ERROR_RETURN(bcm_tr2_l3_ecmp_defragment_no_lock(unit));
    
                /* Attempt to get free index again */
                BCM_IF_ERROR_RETURN(_bcm_xgs3_tbl_free_idx_get(unit, &data));
            } else {
                return rv;
            }
        }
        ecmp_idx = data.entry_index;
        BCM_XGS3_L3_ENT_REF_CNT_INC(data.tbl_ptr, data.entry_index, max_grp_size);
    }

    if (ecmp_idx >= BCM_XGS3_L3_ECMP_TBL_SIZE(unit)) {
        return BCM_E_FULL;
    }

    sal_memset (hw_buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

    /* Write all the indexes to hw. */
    for (idx = 0, nh_cycle_idx = 0; idx < max_grp_size; idx++, nh_cycle_idx++) {

        /* Set next hop index. */
        sal_memset (l3_ecmp, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

        /* If this is the last nhop then program black-hole. */
        if ( (!idx) && (!nh_idx[nh_cycle_idx]) ) {
              nh_cycle_idx = 0;
        } else  if (!nh_idx[nh_cycle_idx]) {
            break;
        }

        soc_mem_field32_set(unit, L3_ECMPm, l3_ecmp, NEXT_HOP_INDEXf,
                            nh_idx[nh_cycle_idx]);

        /* Write buffer to hw L3_ECMPm table. */
        rv = soc_mem_write(unit, L3_ECMPm, MEM_BLOCK_ALL, 
                           (ecmp_idx + idx), l3_ecmp);
        if (BCM_FAILURE(rv)) {
            break;
        }

        /* Write buffer to hw INITIAL_L3_ECMPm table. */
        rv = soc_mem_write(unit, INITIAL_L3_ECMPm, MEM_BLOCK_ALL, 
                                    (ecmp_idx + idx), l3_ecmp);
        if (BCM_FAILURE(rv)) {
            break;
        }
        
        if (soc_feature(unit, soc_feature_urpf)) {
            /* Check if URPF is enabled on device */
            BCM_IF_ERROR_RETURN(
                 READ_L3_DEFIP_RPF_CONTROLr(unit, &reg_val));
            if (reg_val) {
                if (idx < 8) {
                    BCM_IF_ERROR_RETURN (soc_mem_read(unit, 
                       ING_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                       nh_idx[idx], &ing_nh));

                    entry_type = soc_ING_L3_NEXT_HOPm_field32_get(unit, 
                                              &ing_nh, ENTRY_TYPEf);

                    if (entry_type == 0x0) {
                        value = soc_ING_L3_NEXT_HOPm_field32_get(unit, 
                                      &ing_nh, VLAN_IDf);
                        soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, 
                                      l3_ecmp_oif_type_flds[idx], entry_type);
                        soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, 
                                      l3_ecmp_oif_flds[idx], value);
                    } else if (entry_type == 0x1) {
                        value  = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, L3_OIFf);
                        soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, 
                                        l3_ecmp_oif_type_flds[idx], entry_type);
                        soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, 
                                        l3_ecmp_oif_flds[idx], value);
                    }
                    soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, URPF_COUNTf, idx);
                } else {
                    /* Inorder to avoid TRAP_TO_CPU, urpf_mode on L3_IIF/PORT must be set 
                     *  to STRICT_MODE / LOOSE_MODE */
                    soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, ECMP_GT8f , 1);
                }
            }
        }
    }

    if (BCM_SUCCESS(rv)) {
        /* mode 0 = 512 ecmp groups */
        if (!BCM_XGS3_L3_MAX_ECMP_MODE(unit)) { 
            /* Set Max Group Size. */
            sal_memset (l3_ecmp_count, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));
            soc_mem_field32_set(unit, L3_ECMP_COUNTm, l3_ecmp_count, COUNTf, max_grp_size - 1);

            rv = soc_mem_write(unit, L3_ECMP_COUNTm, MEM_BLOCK_ALL, 
                               (ecmp_grp+1), l3_ecmp_count);
            BCM_IF_ERROR_RETURN(rv);
        }
        /* Set zero based ecmp count. */
        if (!idx) {
            soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, COUNTf, idx);
        } else {
            soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, COUNTf, idx - 1);
        }

        /* Set group base pointer. */
        soc_mem_field32_set(unit, L3_ECMP_COUNTm, hw_buf, 
                            BASE_PTRf, ecmp_idx);

        rv = soc_mem_write(unit, L3_ECMP_COUNTm, MEM_BLOCK_ALL, 
                           ecmp_grp, hw_buf);

        BCM_IF_ERROR_RETURN(rv);

        rv = soc_mem_write(unit, INITIAL_L3_ECMP_COUNTm,
                           MEM_BLOCK_ALL, ecmp_grp, hw_buf);
        /* mode 1 = max possible ecmp groups */
        if (BCM_XGS3_L3_MAX_ECMP_MODE(unit)) {
            /* Save the max possible paths for this ECMP group in s/w */ 
            BCM_XGS3_L3_MAX_PATHS_PERGROUP_PTR(unit)[ecmp_grp] = max_paths;
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr2_l3_ecmp_grp_del
 * Purpose:
 *      Reset ecmp group next hop members
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      ecmp_grp   - (IN)ecmp group id to write.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_l3_ecmp_grp_del(int unit, int ecmp_grp, int max_grp_size)
{
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to write hw entry.*/
    ecmp_count_entry_t ecmp_count_entry;
    int ecmp_idx;               /* Ecmp table entry index. */
    int idx;                    /* Iteration index.        */
    int rv = BCM_E_UNAVAIL;     /* Operation return value. */
    _bcm_l3_tbl_op_t data;
    data.tbl_ptr = BCM_XGS3_L3_TBL_PTR(unit, ecmp); 

    /* Initialize ecmp entry. */
    sal_memset (hw_buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

    /* Calculate table index. */
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, L3_ECMP_COUNTm,
                MEM_BLOCK_ANY, ecmp_grp, &ecmp_count_entry));
    ecmp_idx = soc_mem_field32_get(unit, L3_ECMP_COUNTm, &ecmp_count_entry,
            BASE_PTRf);

    /* Write all the indexes to hw. */
    for (idx = 0; idx < max_grp_size; idx++) {
        /* Write buffer to hw. */
        rv = soc_mem_write(unit, L3_ECMPm, MEM_BLOCK_ALL, 
                           (ecmp_idx + idx), hw_buf);

        if (BCM_FAILURE(rv)) {
            return rv;
        }

        /* Write initial ecmp table. */
        if (SOC_MEM_IS_VALID(unit, INITIAL_L3_ECMPm)) {
            /* Write buffer to hw. */
            rv = soc_mem_write(unit, INITIAL_L3_ECMPm, MEM_BLOCK_ALL, 
                           (ecmp_idx + idx), hw_buf);
            if (BCM_FAILURE(rv)) {
                return rv;
            }
        }
    }

    /* Decrement ref count for the entries in ecmp table
     * Ref count for ecmp_group table is decremented in common table del func. */
    BCM_XGS3_L3_ENT_REF_CNT_DEC(data.tbl_ptr, ecmp_idx, max_grp_size);

    /* Set group base pointer. */
    ecmp_idx = ecmp_grp;

    rv = soc_mem_write(unit, L3_ECMP_COUNTm, MEM_BLOCK_ALL, 
                       ecmp_idx, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    if (!BCM_XGS3_L3_MAX_ECMP_MODE(unit)) {
        rv = soc_mem_write(unit, L3_ECMP_COUNTm, MEM_BLOCK_ALL, 
                (ecmp_idx+1), hw_buf);
        BCM_IF_ERROR_RETURN(rv);
    }

    rv = soc_mem_write(unit, INITIAL_L3_ECMP_COUNTm, MEM_BLOCK_ALL, 
                       ecmp_idx, hw_buf);
    
    if (BCM_XGS3_L3_MAX_ECMP_MODE(unit)) {
        /* Reset the max paths of the deleted group */
        BCM_XGS3_L3_MAX_PATHS_PERGROUP_PTR(unit)[ecmp_grp] = 0;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_tr2_l3_intf_urpf_default_route_set
 * Purpose:
 *      Set urpf_default_route enable flag for the L3 interface. 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      enable    - (IN)Enable. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr2_l3_intf_urpf_default_route_set(int unit, bcm_vlan_t vid, int enable)  
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/
    int ret_val;                /* Operation return value.  */

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;
    soc_mem_lock(unit, L3_IIFm);

    ret_val = _bcm_tr_l3_ingress_interface_get(unit, &iif);
    if (BCM_FAILURE(ret_val)) {
        soc_mem_unlock(unit, L3_IIFm);
        return (ret_val);
    }

    if (enable) {
       iif.flags &= ~BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE;
    } else {
       iif.flags |= BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE;
    }

    ret_val = _bcm_tr_l3_ingress_interface_set(unit, &iif);

    soc_mem_unlock(unit, L3_IIFm);

    return (ret_val);
}

/*
 * Function:
 *      _bcm_tr2_l3_intf_urpf_default_route_get
 * Purpose:
 *      Get the urpf_default_route_check enable flag for the specified L3 interface
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      enable    - (OUT)enable. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr2_l3_intf_urpf_default_route_get (int unit, bcm_vlan_t vid, int *enable)
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;

    BCM_IF_ERROR_RETURN(_bcm_tr_l3_ingress_interface_get(unit, &iif));

    *enable = (iif.flags & BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE) ? 0 : 1;
    return (BCM_E_NONE);
}
/*
 * Function:
 *      _bcm_tr2_l3_intf_urpf_mode_set
 * Purpose:
 *      Set the urpf_mode info for the specified L3 interface
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      urpf_mode       - (IN)urpf_mode. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr2_l3_intf_urpf_mode_set(int unit, bcm_vlan_t vid, bcm_vlan_urpf_mode_t urpf_mode)  
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/
    int ret_val;                /* Operation return value.  */

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }
   
    iif.intf_id = vid;
    soc_mem_lock(unit, L3_IIFm);

    ret_val = _bcm_tr_l3_ingress_interface_get(unit, &iif);
    if (BCM_FAILURE(ret_val)) {
        soc_mem_unlock(unit, L3_IIFm);
        return (ret_val);
    }

    iif.urpf_mode = urpf_mode;

    ret_val = _bcm_tr_l3_ingress_interface_set(unit, &iif);

    soc_mem_unlock(unit, L3_IIFm);

    return (ret_val);
}

/*
 * Function:
 *      _bcm_tr2_l3_intf_urpf_mode_get
 * Purpose:
 *      Get the urpf_mode info for the specified L3 interface
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      urpf_mode       - (OUT) urpf_mode
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr2_l3_intf_urpf_mode_get(int unit, bcm_vlan_t vid, bcm_vlan_urpf_mode_t *urpf_mode)
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;

    BCM_IF_ERROR_RETURN(_bcm_tr_l3_ingress_interface_get(unit, &iif));

    *urpf_mode = iif.urpf_mode;
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_buffer_init
 * Purpose:
 *      Initialize L3 ECMP table defragmentation buffer.
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_tr2_l3_ecmp_defragment_buffer_init(int unit)
{
    if (NULL == _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]) {
        _bcm_tr2_l3_ecmp_defragment_buffer_info[unit] =
            sal_alloc(sizeof(_bcm_tr2_l3_ecmp_defragment_buffer_t),
                    "l3 ecmp defragmentation buffer info");
        if (NULL == _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_bcm_tr2_l3_ecmp_defragment_buffer_info[unit], 0,
            sizeof(_bcm_tr2_l3_ecmp_defragment_buffer_t));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_buffer_deinit
 * Purpose:
 *      De-initialize L3 ECMP table defragmentation buffer.
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      None.
 */
void
bcm_tr2_l3_ecmp_defragment_buffer_deinit(int unit)
{
    if (NULL != _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]) {
        sal_free(_bcm_tr2_l3_ecmp_defragment_buffer_info[unit]);
        _bcm_tr2_l3_ecmp_defragment_buffer_info[unit] = NULL;
    }
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_buffer_set
 * Purpose:
 *      Set L3 ECMP table defragmentation buffer.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      size - (IN) Defragmentation buffer size.
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_tr2_l3_ecmp_defragment_buffer_set(int unit, int size)
{
    _bcm_l3_tbl_t *tbl_ptr;
    int old_base_ptr, old_size;
    int base_ptr;
    int i, j;
    int buffer_found;

    if (!soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf) &&
        !soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTR_0f)) {
        return BCM_E_UNAVAIL;
    }

    _bcm_esw_l3_lock(unit);

    tbl_ptr = BCM_XGS3_L3_TBL_PTR(unit, ecmp); 

    /* Free old defragmentation buffer */
    old_base_ptr = _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->base_ptr;
    old_size = _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->size;
    if (old_size > 0) {
        BCM_XGS3_L3_ENT_REF_CNT_DEC(tbl_ptr, old_base_ptr, old_size);
    }

    /* Fist, attempt to find new defragmentation buffer at end of ECMP table.
     * Such a buffer introduces less fragmentation than a buffer that's
     * located in the middle of the ECMP table.
     */
    for (i = tbl_ptr->idx_max + 1 - size; i < (tbl_ptr->idx_max + 1); i++) {
        if (BCM_XGS3_L3_ENT_REF_CNT(tbl_ptr, i)) {
            break;
        }
    }

    if (i == (tbl_ptr->idx_max + 1)) {
        /* An available buffer is found at end of ECMP table */
        base_ptr = tbl_ptr->idx_max + 1 - size;
    } else {
        /* Attempt to find new defragmentation buffer in ECMP table */
        buffer_found = FALSE;
        for (i = tbl_ptr->idx_min; i < (tbl_ptr->idx_max + 1 - size); i++) {
            for (j = i; j < (i + size); j++) {
                if (BCM_XGS3_L3_ENT_REF_CNT(tbl_ptr, j)) {
                    break;
                }
            }
            if (j == (i + size)) {
                buffer_found = TRUE;
                break;
            }
        }

        if (buffer_found) {
            base_ptr = i;
        } else {
            /* A free buffer of the requested size cannot be found.
             * Restore the old buffer.
             */
            if (old_size > 0) {
                BCM_XGS3_L3_ENT_REF_CNT_INC(tbl_ptr, old_base_ptr, old_size);
            }

            _bcm_esw_l3_unlock(unit);
            return BCM_E_RESOURCE;
        }
    }

    BCM_XGS3_L3_ENT_REF_CNT_INC(tbl_ptr, base_ptr, size);
    _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->base_ptr = base_ptr;
    _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->size = size;

    _bcm_esw_l3_unlock(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_buffer_get
 * Purpose:
 *      Get L3 ECMP table defragmentation buffer size.
 * Parameters:
 *      unit - (IN)  SOC unit number.
 *      size - (OUT) Defragmentation buffer size.
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_tr2_l3_ecmp_defragment_buffer_get(int unit, int *size)
{
    if (!soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf) &&
        !soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTR_0f)) {
        return BCM_E_UNAVAIL;
    }

    (*size) = _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->size;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_l3_ecmp_member_copy
 * Purpose:
 *      Copy ECMP table entry.
 * Parameters:
 *      unit - (IN)  SOC unit number.
 *      src_index - (IN) Index of the entry to be copied.
 *      dst_index - (IN) Destination index.
 * Returns:
 *      BCM_E_XXX.
 */
STATIC int
_bcm_tr2_l3_ecmp_member_copy(int unit, int src_index, int dst_index)
{
    ecmp_entry_t ecmp_entry;
    initial_l3_ecmp_entry_t initial_ecmp_entry;

    if (src_index < 0 || src_index > soc_mem_index_max(unit, L3_ECMPm)) {
        return BCM_E_PARAM;
    }
    if (dst_index < 0 || dst_index > soc_mem_index_max(unit, L3_ECMPm)) {
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN
        (READ_L3_ECMPm(unit, MEM_BLOCK_ANY, src_index, &ecmp_entry));
    SOC_IF_ERROR_RETURN
        (WRITE_L3_ECMPm(unit, MEM_BLOCK_ALL, dst_index, &ecmp_entry));

    SOC_IF_ERROR_RETURN
        (READ_INITIAL_L3_ECMPm(unit, MEM_BLOCK_ANY, src_index,
                               &initial_ecmp_entry));
    SOC_IF_ERROR_RETURN
        (WRITE_INITIAL_L3_ECMPm(unit, MEM_BLOCK_ALL, dst_index,
                                &initial_ecmp_entry));

    /* Increment reference count for ECMP table entry at dst_index */
    BCM_XGS3_L3_ENT_REF_CNT_INC(BCM_XGS3_L3_TBL_PTR(unit, ecmp), dst_index, 1);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_l3_ecmp_member_clear
 * Purpose:
 *      Clear an ECMP table entry.
 * Parameters:
 *      unit  - (IN)  SOC unit number.
 *      index - (IN) Index of the entry to be cleared.
 * Returns:
 *      BCM_E_XXX.
 */
STATIC int
_bcm_tr2_l3_ecmp_member_clear(int unit, int index)
{
    if (index < 0 || index > soc_mem_index_max(unit, L3_ECMPm)) {
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(WRITE_L3_ECMPm(unit, MEM_BLOCK_ALL, index,
                soc_mem_entry_null(unit, L3_ECMPm)));

    SOC_IF_ERROR_RETURN(WRITE_INITIAL_L3_ECMPm(unit, MEM_BLOCK_ALL, index,
                soc_mem_entry_null(unit, INITIAL_L3_ECMPm)));

    /* Decrement reference count for the just cleared ECMP table entry */
    BCM_XGS3_L3_ENT_REF_CNT_DEC(BCM_XGS3_L3_TBL_PTR(unit, ecmp), index, 1);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_l3_ecmp_group_base_ptr_update
 * Purpose:
 *      Update the base pointer field of the given ECMP group.
 * Parameters:
 *      unit     - (IN) SOC unit number.
 *      group    - (IN) ECMP group.
 *      base_ptr - (IN) New base pointer.
 * Returns:
 *      BCM_E_XXX.
 */
STATIC int
_bcm_tr2_l3_ecmp_group_base_ptr_update(int unit, int group, int base_ptr)
{
    ecmp_count_entry_t ecmp_group_entry;
    uint32 initial_ecmp_group_entry[SOC_MAX_MEM_FIELD_WORDS];
    soc_mem_t initial_ecmp_group_mem;

    if (group < 0 || group > soc_mem_index_max(unit, L3_ECMP_COUNTm)) {
        return BCM_E_PARAM;
    }

    /* Read L3 ECMP group table */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, group,
                &ecmp_group_entry));
    
    /* Modify base pointer */
    if (soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf)) {
        soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_group_entry,
                BASE_PTRf, base_ptr);
    } else if (soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTR_0f)) {
        soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_group_entry,
                BASE_PTR_0f, base_ptr);
        soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_group_entry,
                BASE_PTR_1f, base_ptr);
        soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_group_entry,
                BASE_PTR_2f, base_ptr);
        soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_group_entry,
                BASE_PTR_3f, base_ptr);
    } else {
        return BCM_E_INTERNAL;
    }

    /* Write L3 ECMP group table */
    SOC_IF_ERROR_RETURN(WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL, group,
                &ecmp_group_entry));

    /* Read Initial L3 ECMP group table */
    if (soc_mem_is_valid(unit, INITIAL_L3_ECMP_COUNTm)) {
        initial_ecmp_group_mem = INITIAL_L3_ECMP_COUNTm;
    } else if (soc_mem_is_valid(unit, INITIAL_L3_ECMP_GROUPm)) {
        initial_ecmp_group_mem = INITIAL_L3_ECMP_GROUPm;
    } else {
        return BCM_E_INTERNAL;
    }
    SOC_IF_ERROR_RETURN(soc_mem_read(unit, initial_ecmp_group_mem,
                MEM_BLOCK_ANY, group, &initial_ecmp_group_entry));

    /* Modify base pointer */
    if (soc_mem_field_valid(unit, initial_ecmp_group_mem, BASE_PTRf)) {
        soc_mem_field32_set(unit, initial_ecmp_group_mem,
                &initial_ecmp_group_entry, BASE_PTRf, base_ptr);
    } else if (soc_mem_field_valid(unit, initial_ecmp_group_mem, BASE_PTR_0f)) {
        soc_mem_field32_set(unit, initial_ecmp_group_mem,
                &initial_ecmp_group_entry, BASE_PTR_0f, base_ptr);
        soc_mem_field32_set(unit, initial_ecmp_group_mem,
                &initial_ecmp_group_entry, BASE_PTR_1f, base_ptr);
        soc_mem_field32_set(unit, initial_ecmp_group_mem,
                &initial_ecmp_group_entry, BASE_PTR_2f, base_ptr);
        soc_mem_field32_set(unit, initial_ecmp_group_mem,
                &initial_ecmp_group_entry, BASE_PTR_3f, base_ptr);
    } else {
        return BCM_E_INTERNAL;
    }

    /* Write Initial L3 ECMP group table */
    SOC_IF_ERROR_RETURN(soc_mem_write(unit, initial_ecmp_group_mem,
                MEM_BLOCK_ALL, group, &initial_ecmp_group_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_no_lock
 * Purpose:
 *      Defragment L3 ECMP table. This procedure does not obtain
 *      L3 lock. It's assumed that L3 lock was obtained before
 *      invoking this procedure. 
 * Parameters:
 *      unit - (IN)  SOC unit number.
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_tr2_l3_ecmp_defragment_no_lock(int unit)
{
    _bcm_l3_tbl_t *ecmp_group_tbl_ptr;
    soc_defragment_block_t *block_array;
    soc_defragment_block_t reserved_block;
    soc_defragment_member_op_t member_op;
    soc_defragment_group_op_t group_op;
    int block_count;
    int i, ecmp_group_idx_increment;
    int rv;
    int max_paths;
    soc_field_t base_ptr_f;
    ecmp_count_entry_t ecmp_group_entry;

    if (!soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf) &&
        !soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTR_0f)) {
        return BCM_E_UNAVAIL;
    }

    ecmp_group_tbl_ptr = BCM_XGS3_L3_TBL_PTR(unit, ecmp_grp);

    block_array = sal_alloc(sizeof(soc_defragment_block_t) *
            (ecmp_group_tbl_ptr->idx_maxused + 1), "defragment block array");

    /* Get the ECMP table base_ptr and max group size of all the active
     * ECMP groups
     */
    block_count = 0;
    if (SOC_IS_TRIUMPH3(unit) || BCM_XGS3_L3_MAX_ECMP_MODE(unit)) {
        ecmp_group_idx_increment = _BCM_SINGLE_WIDE;
    } else {
        ecmp_group_idx_increment = _BCM_DOUBLE_WIDE;
    }
    if (soc_mem_field_valid(unit, L3_ECMP_COUNTm, BASE_PTRf)) {
        base_ptr_f = BASE_PTRf;
    } else {
        base_ptr_f = BASE_PTR_0f;
    }
    for (i = ecmp_group_tbl_ptr->idx_min;
            i <= ecmp_group_tbl_ptr->idx_maxused;
            i += ecmp_group_idx_increment) {

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_vp_lag)) {
            int is_vp_lag, is_used, has_member;
            rv = bcm_td2_vp_lag_status_get(unit, i, &is_vp_lag, &is_used,
                    &has_member);
            if (BCM_FAILURE(rv)) {
                sal_free(block_array);
                return rv;
            }
            if (is_vp_lag) {
                /* Skip VP LAG entries */
                continue;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */

        if (!BCM_XGS3_L3_ENT_REF_CNT(ecmp_group_tbl_ptr, i)) {
            continue;
        }

        rv = READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, i, &ecmp_group_entry);
        if (SOC_FAILURE(rv)) {
            sal_free(block_array);
            return rv;
        }
        block_array[block_count].base_ptr =
            soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_group_entry, base_ptr_f);
        rv = bcm_xgs3_l3_egress_ecmp_max_paths_get(unit,
                i + BCM_XGS3_MPATH_EGRESS_IDX_MIN, &max_paths);
        if (BCM_FAILURE(rv)) {
            sal_free(block_array);
            return rv;
        }
        block_array[block_count].size = max_paths;
        block_array[block_count].group = i;
        block_count++;
    }

#ifdef BCM_TRIDENT2_SUPPORT
    /* In Trident2, the VP LAG and ECMP features share the same group and
     * member tables. Defragmentation of the member table also requires
     * the group base_ptr and group size of all the active VP LAGs.
     */
    if (soc_feature(unit, soc_feature_vp_lag)) {
        bcm_trunk_chip_info_t trunk_chip_info;
        int min_vp_lag_id, max_vp_lag_id;
        int is_vp_lag, is_used, has_member;

        rv = bcm_esw_trunk_chip_info_get(unit, &trunk_chip_info);
        if (BCM_FAILURE(rv)) {
            sal_free(block_array);
            return rv;
        }
        if (trunk_chip_info.vp_id_min != -1 &&
                trunk_chip_info.vp_id_max != -1) {
            min_vp_lag_id = 0;
            max_vp_lag_id = trunk_chip_info.vp_id_max -
                trunk_chip_info.vp_id_min;
            for (i = min_vp_lag_id; i <= max_vp_lag_id; i++) {
                rv = bcm_td2_vp_lag_status_get(unit, i, &is_vp_lag, &is_used,
                        &has_member);
                if (BCM_FAILURE(rv)) {
                    sal_free(block_array);
                    return rv;
                }
                if (!is_vp_lag) {
                    sal_free(block_array);
                    return BCM_E_INTERNAL;
                }
                if (!is_used) {
                    continue;
                }
                if (!has_member) {
                    continue;
                }

                rv = READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, i,
                        &ecmp_group_entry);
                if (SOC_FAILURE(rv)) {
                    sal_free(block_array);
                    return rv;
                }
                block_array[block_count].base_ptr =
                    soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_group_entry,
                            BASE_PTRf);
                block_array[block_count].size = 1 +
                    soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_group_entry,
                            COUNTf);
                block_array[block_count].group = i;
                block_count++;
            }
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Get the defragmentation buffer base_ptr and size */
    reserved_block.base_ptr =
        _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->base_ptr;
    reserved_block.size =
        _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->size;
    reserved_block.group = -1;

    /* Set functions to copy and clear ECMP member entries */
    member_op.member_copy = _bcm_tr2_l3_ecmp_member_copy;
    member_op.member_clear = _bcm_tr2_l3_ecmp_member_clear;

    /* Set function to update ECMP group's base_ptr */
    group_op.group_base_ptr_update = _bcm_tr2_l3_ecmp_group_base_ptr_update;

    /* Call defragmentation routine */
    rv = soc_defragment(unit, block_count, block_array, &reserved_block,
            soc_mem_index_count(unit, L3_ECMPm), &member_op, &group_op);
    if (SOC_FAILURE(rv)) {
        sal_free(block_array);
        return rv;
    }

    sal_free(block_array);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment
 * Purpose:
 *      Defragment L3 ECMP table.
 * Parameters:
 *      unit - (IN)  SOC unit number.
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_tr2_l3_ecmp_defragment(int unit)
{
    int rv;

    _bcm_esw_l3_lock(unit);

    rv = bcm_tr2_l3_ecmp_defragment_no_lock(unit);

    _bcm_esw_l3_unlock(unit);

    return rv;
}

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_buffer_wb_alloc_size_get
 * Purpose:
 *      Get level 2 warm boot scache size for ECMP defragmentation
 *      buffer parameters.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      size - (OUT) Allocation size.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_l3_ecmp_defragment_buffer_wb_alloc_size_get(int unit, int *size) 
{
    *size = 0;

    /* Allocate for buffer base_ptr */
    *size += sizeof(int);

    /* Allocate for buffer size */
    *size += sizeof(int);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_buffer_sync
 * Purpose:
 *      Store ECMP defragmentation buffer parameters into scache.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_l3_ecmp_defragment_buffer_sync(int unit, uint8 **scache_ptr) 
{
    int value;

    /* Store buffer base_ptr */
    value = _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->base_ptr;
    sal_memcpy((*scache_ptr), &value, sizeof(int));
    (*scache_ptr) += sizeof(int);

    /* Store buffer base_ptr */
    value = _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->size;
    sal_memcpy((*scache_ptr), &value, sizeof(int));
    (*scache_ptr) += sizeof(int);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_l3_ecmp_defragment_buffer_recover
 * Purpose:
 *      Recover ECMP defragment buffer parameters from scache.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_l3_ecmp_defragment_buffer_recover(int unit, uint8 **scache_ptr) 
{
    int value;

    /* Recover defragmentation buffer base_ptr */
    sal_memcpy(&value, (*scache_ptr), sizeof(int));
    _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->base_ptr = value;
    (*scache_ptr) += sizeof(int);

    /* Recover defragmentation buffer size */
    sal_memcpy(&value, (*scache_ptr), sizeof(int));
    _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->size = value;
    (*scache_ptr) += sizeof(int);

    return BCM_E_NONE;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     bcm_tr2_l3_ecmp_defragment_buffer_sw_dump
 * Purpose:
 *     Displays ECMP defragmentation buffer information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
bcm_tr2_l3_ecmp_defragment_buffer_sw_dump(int unit)
{
    soc_cm_print("  ECMP Defragment Buffer: base_ptr = %d, size = %d\n",
            _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->base_ptr,
            _bcm_tr2_l3_ecmp_defragment_buffer_info[unit]->size);
    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
int bcm_esw_triumph2_l3_not_empty;
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */

