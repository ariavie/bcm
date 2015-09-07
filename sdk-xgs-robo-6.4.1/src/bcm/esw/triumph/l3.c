/*
 * $Id: l3.c,v 1.89 Broadcom SDK $
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
 * Purpose:     Triumph L3 function implementations
 */


#include <soc/defs.h>

#include <assert.h>

#include <sal/core/libc.h>
#if defined(BCM_TRIUMPH_SUPPORT)  && defined(INCLUDE_L3)

#include <shared/util.h>
#include <soc/mem.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/l3x.h>
#include <soc/lpm.h>
#include <soc/tnl_term.h>

#include <bcm/l3.h>
#include <bcm/tunnel.h>
#include <bcm/debug.h>
#include <bcm/error.h>
#include <bcm/stack.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw_dispatch.h>
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
#include <bcm_int/esw/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

/*
 * Function:
 *      _bcm_tr_defip_init
 * Purpose:
 *      Initialize L3 DEFIP table for triumph devices.
 * Parameters:
 *      unit    - (IN)  SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr_defip_init(int unit)
{
    soc_mem_t mem_v4;      /* IPv4 Route table memory.             */
    soc_mem_t mem_v6;      /* IPv6 Route table memory.             */
    soc_mem_t mem_v6_128;  /* IPv6 full prefix route table memory. */
    int index_min, index_max;
#if defined(BCM_KATANA_SUPPORT)
    int ipv6_lpm_128b_enable;
#endif

    /* Get memory for IPv4 entries. */
    BCM_IF_ERROR_RETURN(_bcm_tr_l3_defip_mem_get(unit, 0, 0, &mem_v4));

    /* Initialize IPv4 entries lookup engine. */
    if (L3_DEFIPm == mem_v4) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm_init(unit));
    } else {
        BCM_IF_ERROR_RETURN(_bcm_tr_ext_lpm_init(unit, mem_v4));
    }

    /* Get memory for IPv6 entries. */
    BCM_IF_ERROR_RETURN
        (_bcm_tr_l3_defip_mem_get(unit, BCM_L3_IP6, 0, &mem_v6));

    /* Initialize IPv6 entries lookup engine. */
    if (L3_DEFIPm == mem_v6) {
        if (mem_v4 != mem_v6) {
            BCM_IF_ERROR_RETURN(soc_fb_lpm_init(unit));
        }
    } else {
        BCM_IF_ERROR_RETURN(_bcm_tr_ext_lpm_init(unit, mem_v6));
    }

    /* Get memory for IPv6 entries with prefix length > 64. */
    BCM_IF_ERROR_RETURN
        (_bcm_tr_l3_defip_mem_get(unit, BCM_L3_IP6,
                                  BCM_XGS3_L3_IPV6_PREFIX_LEN,
                                  &mem_v6_128));

    /* Initialize IPv6 entries lookup engine. */
    if (mem_v6 != mem_v6_128) {
        if (SOC_IS_TRIDENT2(unit)) {  
            return BCM_E_NONE;
        }
#if defined(BCM_KATANA_SUPPORT)
        ipv6_lpm_128b_enable =
            soc_property_get(unit, spn_IPV6_LPM_128B_ENABLE, 0);
        if (SOC_IS_KATANA(unit) && (ipv6_lpm_128b_enable)) {
            /* Initialize pair memory for IPv6 entries with prefix length > 64. */
            BCM_IF_ERROR_RETURN(_bcm_kt_defip_pair128_init(unit));
            BCM_IF_ERROR_RETURN(_bcm_trx_defip_128_init(unit));
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(_bcm_trx_defip_128_init(unit));
        }
    }

    /* Update defip size in l3 book keeping if ESM present */
    if (soc_feature(unit, soc_feature_esm_support) && (L3_DEFIPm != mem_v4)) {
        index_min = soc_mem_index_min(unit, mem_v4);
        index_max = soc_mem_index_max(unit, mem_v4);
        BCM_XGS3_L3_DEFIP_TBL_SIZE(unit) = index_max - index_min + 1;
    }
    return (BCM_E_NONE);
}


/*
 * Function:
 *      _bcm_tr_defip_deinit
 * Purpose:
 *      De-initialize L3 DEFIP table for triumph devices.
 * Parameters:
 *      unit    - (IN)  SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr_defip_deinit(int unit)
{
    soc_mem_t mem_v4;      /* IPv4 Route table memory.             */
    soc_mem_t mem_v6;      /* IPv6 Route table memory.             */
    soc_mem_t mem_v6_128;  /* IPv6 full prefix route table memory. */
#if defined(BCM_KATANA_SUPPORT)
    int ipv6_lpm_128b_enable;
#endif

    /* Get memory for IPv4 entries. */
    BCM_IF_ERROR_RETURN(_bcm_tr_l3_defip_mem_get(unit, 0, 0, &mem_v4));

    /* Initialize IPv4 entries lookup engine. */
    if (L3_DEFIPm == mem_v4) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm_deinit(unit));
    } else {
        BCM_IF_ERROR_RETURN(_bcm_tr_ext_lpm_deinit(unit, mem_v4));
    }

    /* Get memory for IPv6 entries. */
    BCM_IF_ERROR_RETURN
        (_bcm_tr_l3_defip_mem_get(unit, BCM_L3_IP6, 0, &mem_v6));

    /* Initialize IPv6 entries lookup engine. */
    if ((L3_DEFIPm == mem_v6) && (mem_v4 != mem_v6)) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm_deinit(unit));
    } else {
        BCM_IF_ERROR_RETURN(_bcm_tr_ext_lpm_deinit(unit, mem_v6));
    }

    /* Get memory for IPv6 entries with prefix length > 64. */
    BCM_IF_ERROR_RETURN
        (_bcm_tr_l3_defip_mem_get(unit, BCM_L3_IP6,
                                  BCM_XGS3_L3_IPV6_PREFIX_LEN,
                                  &mem_v6_128));

    /* Initialize IPv6 entries lookup engine. */
    if (mem_v6 != mem_v6_128) {
#if defined(BCM_KATANA_SUPPORT)
        ipv6_lpm_128b_enable =
            soc_property_get(unit, spn_IPV6_LPM_128B_ENABLE, 0);
        if (SOC_IS_KATANA(unit) && (ipv6_lpm_128b_enable)) {
            BCM_IF_ERROR_RETURN(_bcm_kt_defip_pair128_deinit(unit));
            BCM_IF_ERROR_RETURN(_bcm_trx_defip_128_deinit(unit));
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(_bcm_trx_defip_128_deinit(unit));
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr_l3_ingress_interface_get
 * Purpose:
 *      Get ingress interface config.
 * Parameters:
 *      unit     - (IN) BCM unit number.
 *      iif      - (IN) Interface configuration.
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_ingress_interface_get(int unit, _bcm_l3_ingress_intf_t *iif)
{

#ifdef BCM_TRIDENT2_SUPPORT
    uint32 index;
    void *entries[1];
    iif_profile_entry_t l3_iif_profile;
#endif
    iif_entry_t entry;       /* HW entry buffer.        */
    int intf_id;             /* Interface id.           */
    int rv;                  /* Operation return status.*/
    int hw_map_idx=0;
    int no_trust_idx=0;

    
    /* Input parameters sanity check. */
    if (NULL == iif) {
        return (BCM_E_PARAM);
    }
    if ((iif->intf_id > soc_mem_index_max(unit, L3_IIFm)) || 
        (iif->intf_id < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    intf_id = iif->intf_id;
    sal_memset(iif, 0, sizeof(_bcm_l3_ingress_intf_t));
    sal_memcpy(&entry,  soc_mem_entry_null(unit, L3_IIFm),
               sizeof(iif_entry_t));

    /* Read interface config from hw. */
    rv = soc_mem_read(unit, L3_IIFm, MEM_BLOCK_ANY, intf_id, (uint32 *)&entry);
    BCM_IF_ERROR_RETURN(rv);

    /* Get class id value. */
    iif->if_class = soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry, CLASS_IDf);

    /* Get vrf id value. */
    iif->vrf = soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry, VRFf);

    /* Get vrf id value. */
    if (soc_mem_field_valid(unit, L3_IIFm, ALLOW_GLOBAL_ROUTEf)) {
        if (0 == soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry,
                                     ALLOW_GLOBAL_ROUTEf)) {
            iif->flags |= BCM_VLAN_L3_VRF_GLOBAL_DISABLE; 
        }
    }


#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) { /* Get l3 iif profile entry */
        entries[0] = &l3_iif_profile;
        index = soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry,
                                    L3_IIF_PROFILE_INDEXf); 
        (void)_bcm_l3_iif_profile_entry_get(unit, index, 1, entries);
        /* Get vrf id value. */
        if (soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                &l3_iif_profile, ALLOW_GLOBAL_ROUTEf)) {
            iif->flags |= BCM_L3_INGRESS_GLOBAL_ROUTE; 
        }

        if (soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                &l3_iif_profile, URPF_DEFAULTROUTECHECKf)) {
            iif->flags |= BCM_L3_INGRESS_URPF_DEFAULT_ROUTE_CHECK; 
        }

        if (0 == soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                     &l3_iif_profile, IPV4L3_ENABLEf)) {
            iif->flags |= BCM_L3_INGRESS_ROUTE_DISABLE_IP4_UCAST; 
        }

        if (0 == soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                     &l3_iif_profile, IPMCV4_ENABLEf)) {
            iif->flags |= BCM_L3_INGRESS_ROUTE_DISABLE_IP4_MCAST; 
        }

        if (0 == soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                     &l3_iif_profile, IPV6L3_ENABLEf)) {
            iif->flags |= BCM_L3_INGRESS_ROUTE_DISABLE_IP6_UCAST; 
        }

        if (0 == soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                     &l3_iif_profile, IPMCV6_ENABLEf)) {
            iif->flags |= BCM_L3_INGRESS_ROUTE_DISABLE_IP6_MCAST; 
        }

        if (soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                &l3_iif_profile, UNKNOWN_IPV4_MC_TOCPUf)) {
            iif->flags |= BCM_L3_INGRESS_UNKNOWN_IP4_MCAST_TOCPU; 
        }

        if (soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                &l3_iif_profile, UNKNOWN_IPV6_MC_TOCPUf)) {
            iif->flags |= BCM_L3_INGRESS_UNKNOWN_IP6_MCAST_TOCPU; 
        }

        if (soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                &l3_iif_profile, ICMP_REDIRECT_TOCPUf)) {
            iif->flags |= BCM_L3_INGRESS_ICMP_REDIRECT_TOCPU; 
        }

        if (soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                &l3_iif_profile, L3_OVERRIDE_IPMC_DO_VLANf)) {
            iif->flags |= BCM_L3_INGRESS_IPMC_DO_VLAN_DISABLE; 
        }
 
        /* Retrieve IP option Profile Index */
        if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
            iif->ip4_options_profile_id = soc_mem_field32_get(unit, L3_IIFm,
                                   (uint32 *)&entry, IP_OPTION_PROFILE_INDEXf);
        }
        if (soc_feature(unit, soc_feature_nat) && 
                        SOC_MEM_FIELD_VALID(unit, L3_IIFm, SRC_REALM_IDf)) {
            /* Get the nat source realm id value. */
            iif->nat_realm_id = soc_mem_field32_get(unit, L3_IIFm, 
                                               (uint32 *)&entry, SRC_REALM_IDf);
        }
    } 
#endif

    /* Get Trust QoS Map Id Value. */
    if (BCM_VLAN_INVALID > iif->intf_id) {
       iif->qos_map_id = 0;
    } else {
        if (soc_mem_field_valid(unit, L3_IIFm, TRUST_DSCP_PTRf)) {
           hw_map_idx = soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry,
                                            TRUST_DSCP_PTRf);
        }
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TRIDENT2(unit)) {
            hw_map_idx = soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                             &l3_iif_profile, TRUST_DSCP_PTRf);
        }
#endif
        if (soc_mem_field_valid(unit, L3_IIFm, USE_VINTF_CTR_IDXf)) {
            no_trust_idx = ((soc_mem_index_count(unit, DSCP_TABLEm) / 64)) - 1;
        } else {
            no_trust_idx = 63;
        }

        if (hw_map_idx != no_trust_idx) {
             if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
                 BCM_IF_ERROR_RETURN
                   (_bcm_tr_qos_idx2id(unit, hw_map_idx, 0x3,
                        &iif->qos_map_id));
             } else {
#ifdef BCM_TRIUMPH2_SUPPORT
                  BCM_IF_ERROR_RETURN
                     (_bcm_tr2_qos_idx2id(unit, hw_map_idx, 0x3,
                        &iif->qos_map_id));
#endif /*BCM_TRIUMPH2_SUPPORT*/
             }
             iif->flags |= BCM_L3_INGRESS_DSCP_TRUST; 
        }
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_mem_field_valid(unit, L3_IIFm, IPMC_L3_IIFf)) {
         iif->ipmc_intf_id = soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry, IPMC_L3_IIFf);
    }
#endif    

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_mem_field_valid(unit, L3_IIFm, URPF_DEFAULTROUTECHECKf)) {
         if (0 == soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry,
                                  URPF_DEFAULTROUTECHECKf)) {
              iif->flags |= BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE; 
         }
    }
    if (soc_mem_field_valid(unit, L3_IIFm, URPF_MODEf)) {
         iif->urpf_mode = soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry,
                             URPF_MODEf); 
    }
#endif

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        if (0 == soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                     &l3_iif_profile,
                                     URPF_DEFAULTROUTECHECKf)) {
            iif->flags |= BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE; 
        }
        iif->urpf_mode = soc_mem_field32_get(unit, L3_IIF_PROFILEm, 
                                     &l3_iif_profile, URPF_MODEf);
    }
#endif
    /* Set interface id. */
    iif->intf_id = intf_id;

    return (BCM_E_NONE);
}

#ifdef BCM_TRIDENT2_SUPPORT
/*
 * Function:
 *      _bcm_l3_iif_profile_recover
 * Purpose:
 *      Add entry into profile index table. 
 * Parameters:
 *      unit     - (IN) BCM unit number.
 *      iif      - (IN) Interface configuration.
 *      profile_index - (IN) Index into Profile table
 * Returns:
 *      BCM_E_XXX.
 */

int
_bcm_l3_iif_profile_recover(int unit, iif_profile_entry_t *l3_iif_profile,
                            uint8 profile_index)
{
    uint32 index;
    void *entries[1];
    int rv = BCM_E_NONE;

    /* Input parameters sanity check. */
    if (NULL == l3_iif_profile) {
        return (BCM_E_PARAM);
    }   
    
    entries[0] = l3_iif_profile;
    index = profile_index;

    /* Create new profile entry if not existent or updating failed */
    rv = _bcm_l3_iif_profile_sw_state_set(unit, entries, 1, index);
    return rv;
}

/*
 * Function:
 *      _bcm_l3_iif_profile_add
 * Purpose:
 *      Add entry into profile index table. 
 * Parameters:
 *      unit     - (IN) BCM unit number.
 *      iif      - (IN) Interface configuration.
 *      profile_index - (OUT) Index into Profile table
 * Returns:
 *      BCM_E_XXX.
 */

STATIC int
_bcm_l3_iif_profile_add(int unit, _bcm_l3_ingress_intf_t *iif, 
                        uint8 *profile_index)
{
    uint32 index;
    uint8 value;                /* Enable/disable boolean. */
    void *entries[1];
    iif_entry_t iif_entry;      /* HW iif_entry buffer.        */
    iif_profile_entry_t l3_iif_profile;
    int hw_map_idx=0;
    int no_trust_idx=0;
    int rv = BCM_E_NONE;

    /* Input parameters sanity check. */
    if ((NULL == iif) || (NULL == profile_index)) {
        return (BCM_E_PARAM);
    }

    *profile_index = 0;

    if (SOC_IS_TRIDENT2(unit)) {
        /* Read interface entry from hw. */
        rv = soc_mem_read(unit, L3_IIFm, MEM_BLOCK_ANY, iif->intf_id,
                          (uint32 *)&iif_entry);

        if (BCM_SUCCESS(rv)) {
            /* Get profile_index from interface entry*/
            index = soc_mem_field32_get(unit, L3_IIFm,
                                        (uint32 *)&iif_entry,
                                        L3_IIF_PROFILE_INDEXf);

            if (iif->profile_flags & _BCM_L3_IIF_PROFILE_DO_NOT_UPDATE) {
                *profile_index = (uint8)index;
                return BCM_E_NONE;
            }
        }

        entries[0] = &l3_iif_profile;
        sal_memset(&l3_iif_profile, 0, sizeof(iif_profile_entry_t));

        /* Set Trust QoS Map Id Value. */
        if (iif->flags & BCM_L3_INGRESS_DSCP_TRUST) {
            BCM_IF_ERROR_RETURN
                 (_bcm_tr2_qos_id2idx(unit, iif->qos_map_id, &hw_map_idx));
            soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                                TRUST_DSCP_PTRf, hw_map_idx);
        } else {
            if (BCM_VLAN_INVALID > iif->intf_id) {
                soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                                TRUST_DSCP_PTRf, 127);
            } else {
                /* Trident2 : 7 bit*/
                no_trust_idx = ((soc_mem_index_count(unit, DSCP_TABLEm) / 64)) - 1;
                soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile, 
                                    TRUST_DSCP_PTRf, no_trust_idx);
            }
        }

        /* Set URPF. */
        value  = (iif->flags & BCM_L3_INGRESS_URPF_DEFAULT_ROUTE_CHECK) ?  1 : 0;
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile, 
                                    URPF_DEFAULTROUTECHECKf, value);
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                             URPF_MODEf, iif->urpf_mode); 

        /* Set enable global route value. */
        value  = (iif->flags & BCM_L3_INGRESS_GLOBAL_ROUTE) ?  1 : 0;
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            ALLOW_GLOBAL_ROUTEf, value); 

        /* enable IPv4 and IPv6 Unicast and MC */
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            IPV4L3_ENABLEf,
                            (iif->flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP4_UCAST)
                            ? 0 : 1); 
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            IPMCV4_ENABLEf,
                            (iif->flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP4_MCAST)
                            ? 0 : 1); 
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            IPV6L3_ENABLEf,
                            (iif->flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP6_UCAST)
                            ? 0 : 1); 
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            IPMCV6_ENABLEf,
                            (iif->flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP6_MCAST)
                            ? 0 : 1); 

        /* Set ICMP redirect to CPU */
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            ICMP_REDIRECT_TOCPUf,
                            (iif->flags & BCM_L3_INGRESS_ICMP_REDIRECT_TOCPU)
                            ? 1 : 0);

        /* Set Unknown IPMC to CPU */
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            UNKNOWN_IPV4_MC_TOCPUf,
                            (iif->flags & BCM_L3_INGRESS_UNKNOWN_IP4_MCAST_TOCPU)
                            ? 1 : 0);

        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            UNKNOWN_IPV6_MC_TOCPUf,
                            (iif->flags & BCM_L3_INGRESS_UNKNOWN_IP6_MCAST_TOCPU)
                            ? 1 : 0); 

        /* set L3_OVERRIDE_IPMC_DO_VLAN */
        soc_mem_field32_set(unit, L3_IIF_PROFILEm, &l3_iif_profile,
                            L3_OVERRIDE_IPMC_DO_VLANf,
                            (iif->flags & BCM_L3_INGRESS_IPMC_DO_VLAN_DISABLE)
                            ? 1 : 0);

        if (BCM_SUCCESS(rv)) {
            /* If profile entry exist and not shared, update it */
            rv = _bcm_l3_iif_profile_entry_update(unit, entries, &index);
            if (BCM_SUCCESS(rv)) {
                *profile_index = (uint8)index;
                return BCM_E_NONE;
            }
        }

        /* Create new profile entry if not existent or updating failed */
        rv = _bcm_l3_iif_profile_entry_add(unit, entries, 1, &index);
        if (BCM_SUCCESS(rv)) {
            *profile_index = (uint8)index;
        }
    }
    return rv;

}
#endif /* BCM_TRIDENT2_SUPPORT */


/*
 * Function:
 *      _bcm_tr_l3_ingress_interface_set
 * Purpose:
 *      Set ingress interface config.
 * Parameters:
 *      unit     - (IN) BCM unit number.
 *      iif      - (IN) Interface configuration.
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_ingress_interface_set(int unit, _bcm_l3_ingress_intf_t *iif)
{
    iif_entry_t entry, iif_entry;     /* HW entry buffer.        */
    uint8 value = 0;                  /* Enable/disable boolean. */
#ifdef BCM_TRIDENT2_SUPPORT
    uint8 old_profile_index = 0;
#endif
    int hw_map_idx=0;
    int rv = BCM_E_NONE;
#ifdef BCM_TRIUMPH2_SUPPORT
    int mode=0;
#endif
    int bit_num=0;


    /* Input parameters sanity check. */
    if (NULL == iif) {
        return (BCM_E_PARAM);
    }
    if ((iif->intf_id > soc_mem_index_max(unit, L3_IIFm)) || 
        (iif->intf_id < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    sal_memcpy(&entry,  soc_mem_entry_null(unit, L3_IIFm),
               sizeof(iif_entry_t));
    sal_memcpy(&iif_entry,  soc_mem_entry_null(unit, L3_IIFm),
               sizeof(iif_entry_t));

    /* Set class id value. */
    soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, CLASS_IDf,
                        iif->if_class);

    /* Set vrf id value. */
    soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, VRFf, iif->vrf);

    /* Set Trust QoS Map Id Value. */
    if (iif->flags & BCM_L3_INGRESS_DSCP_TRUST) {
         if (SOC_MEM_FIELD_VALID(unit, L3_IIFm, TRUST_DSCP_PTRf)) {
             if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
                 BCM_IF_ERROR_RETURN
                     (_bcm_tr_qos_id2idx(unit, iif->qos_map_id, &hw_map_idx));
             } else {
#ifdef BCM_TRIUMPH2_SUPPORT
                 BCM_IF_ERROR_RETURN
                     (_bcm_tr2_qos_id2idx(unit, iif->qos_map_id, &hw_map_idx));
#endif /* BCM_TRIUMPH2_SUPPORT */
             }
             soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, 
                        TRUST_DSCP_PTRf, hw_map_idx);
         }
    } else {
             if (BCM_VLAN_INVALID > iif->intf_id) {
                 if (soc_mem_field_valid(unit, L3_IIFm, TRUST_DSCP_PTRf)) {
                     bit_num = soc_mem_field_length(unit, L3_IIFm, TRUST_DSCP_PTRf);
                     switch (bit_num) {
                         case _BCM_TR_L3_TRUST_DSCP_PTR_BIT_SIZE:
                              /* DSCP_TABLE size vary across Chariot and Enduro, but Do not trust value is identical */
                              soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, 
                                              TRUST_DSCP_PTRf, 63);
                              break;
                         case _BCM_TD_L3_TRUST_DSCP_PTR_BIT_SIZE:
                              /* Trident and Triumph3 */
                             
                             if (SOC_IS_TRIUMPH3(unit)) {
                                 soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, 
                                                     TRUST_DSCP_PTRf, 127);
                             } else {
                                 soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, 
                                                     TRUST_DSCP_PTRf, 0);
                             }
                             break;

                         default:
                             break;
                     }
                 }
             } else {
                if (soc_mem_field_valid(unit, L3_IIFm, TRUST_DSCP_PTRf)) {
                    bit_num = soc_mem_field_length(unit, L3_IIFm, TRUST_DSCP_PTRf);
                    switch (bit_num) {
                        case _BCM_TR_L3_TRUST_DSCP_PTR_BIT_SIZE:
                             /* DSCP_TABLE size vary across Chariot and Enduro, but Do not trust value is identical */
                             soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, 
                                             TRUST_DSCP_PTRf, 63);
                             break;
                        case _BCM_TD_L3_TRUST_DSCP_PTR_BIT_SIZE:
                             /* Trident and Triumph3 */
                            soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, 
                                               TRUST_DSCP_PTRf, 127);
                             break;

                        default:
                              break;
                    }
                }
             }
    }
    /* Set identity mapping for IPMC L3 IIF */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_mem_field_valid(unit, L3_IIFm, IPMC_L3_IIFf)) {
        rv = bcm_xgs3_l3_ingress_mode_get(unit, &mode);
        if (rv < 0) {
             return rv;
        }
        if (mode) {  
            soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, IPMC_L3_IIFf, 
                            iif->ipmc_intf_id);
        } else {
            soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, IPMC_L3_IIFf, 
                            iif->intf_id);
        }
    }
#endif
#ifdef BCM_TRIDENT_SUPPORT
    if (soc_mem_field_valid(unit, L3_IIFm, URPF_DEFAULTROUTECHECKf)) {
         value  = (iif->flags & BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE) ?  0 : 1;
         soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                             URPF_DEFAULTROUTECHECKf, value); 
    }
    if (soc_mem_field_valid(unit, L3_IIFm, URPF_MODEf)) {
         soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                             URPF_MODEf, iif->urpf_mode); 
    }
#endif

    /* Set enable global route value. */
    if (soc_mem_field_valid(unit, L3_IIFm, ALLOW_GLOBAL_ROUTEf)) {
        value  = (iif->flags & BCM_VLAN_L3_VRF_GLOBAL_DISABLE) ?  0 : 1;
        soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                            ALLOW_GLOBAL_ROUTEf, value); 
    }

#ifdef BCM_TRIDENT2_SUPPORT
    /* TD2 L3_IIF_PROFILE  and IP Option Profile*/
    if (SOC_IS_TRIDENT2(unit)) {
#define _BCM_TD2_IP_OPTIONS_PROFILE_MASK  0x3ff   /* 10 bits width */
#define _BCM_TD2_IP_OPTIONS_BITS_WIDTH    8  /* IP options size = 8 (0 - 256) */
        if (soc_feature(unit, soc_feature_nat) &&
                        SOC_MEM_FIELD_VALID(unit, L3_IIFm, SRC_REALM_IDf)) {
            if ((iif->nat_realm_id < 0) || (iif->nat_realm_id > 3)) {
                return BCM_E_PARAM;
            }
            /* Set the nat source realm id value. */
            soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry, SRC_REALM_IDf,
                                iif->nat_realm_id);
        }
        /* Read interface entry from hw. */
        rv = soc_mem_read(unit, L3_IIFm, MEM_BLOCK_ANY, iif->intf_id,
                          (uint32 *)&iif_entry);

        if (BCM_SUCCESS(rv)) {
            /* Get profile_index from interface entry*/
            old_profile_index = soc_mem_field32_get(unit, L3_IIFm,
                                                    (uint32 *)&iif_entry,
                                                    L3_IIF_PROFILE_INDEXf);
        }

        rv = _bcm_l3_iif_profile_add(unit, iif, &value);
        if (BCM_FAILURE(rv)) {
            return rv;
        }
        if ((value < soc_mem_index_min(unit, L3_IIF_PROFILEm)) ||
            (value > soc_mem_index_max(unit, L3_IIF_PROFILEm))) {
            _bcm_l3_iif_profile_entry_delete(unit, value);
            return BCM_E_FAIL;
        }
        if (soc_mem_field_valid(unit, L3_IIFm, L3_IIF_PROFILE_INDEXf)) {
            soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                                L3_IIF_PROFILE_INDEXf, value);
        }

        /* Program IP option Profile Index */
        if (soc_feature(unit, soc_feature_l3_ip4_options_profile)) {
            int index;

            /* Get the hardware index for the profile */
            rv = _bcm_td2_l3_ip4_options_profile_id2idx(unit, 
                                                iif->ip4_options_profile_id,
                                                &index);
            if (BCM_SUCCESS(rv)) {
                /* Program the upper 2 bits */
                index &= _BCM_TD2_IP_OPTIONS_PROFILE_MASK;
                /*
                * COVERITY
                *
                * The operands don't affect result. It is kept intentionally
                * as a defensive default for future development.
                */
                /* coverity[result_independent_of_operands] */
                index = index >> _BCM_TD2_IP_OPTIONS_BITS_WIDTH;
                soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                                    IP_OPTION_PROFILE_INDEXf, index);
            }
        }
    }
#endif

    /* Write  updated entry back to hw.*/
    rv = soc_mem_write(unit, L3_IIFm, MEM_BLOCK_ALL, iif->intf_id, 
                          (uint32 *)&entry);
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        if (BCM_SUCCESS(rv)) {
            /* delete old entry if its valid and different than new entry*/
            if ((value != old_profile_index) && old_profile_index) {
                _bcm_l3_iif_profile_entry_delete(unit, old_profile_index);
            }
        } else {
            /* if hw write fails for new entry then old entry is still vaild*/
            /* we may have to remove the new entry which was added above. */
            _bcm_l3_iif_profile_entry_delete(unit, value);
        }
    }
#endif

    return rv;
}

/*
 * Function:
 *      _bcm_tr_l3_ingress_interface_clr
 * Purpose:
 *      Set ingress interface config.
 * Parameters:
 *      unit     - (IN) BCM unit number.
 *      iif      - (IN) Interface configuration.
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_ingress_interface_clr(int unit, int intf_id)
{
    int bit_num;
    iif_entry_t entry;          /* HW entry buffer.        */
#ifdef BCM_TRIDENT2_SUPPORT
    uint8 index;
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Input parameters sanity check. */
    if ((intf_id > soc_mem_index_max(unit, L3_IIFm)) || 
        (intf_id < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        BCM_IF_ERROR_RETURN(
            soc_mem_read(unit, L3_IIFm, MEM_BLOCK_ANY, intf_id, (uint32 *)&entry));
        index = soc_mem_field32_get(unit, L3_IIFm, (uint32 *)&entry,
                                    L3_IIF_PROFILE_INDEXf); 
        (void)_bcm_l3_iif_profile_entry_delete(unit, index);
    }
#endif

    /* Restore the IIF entry to its defaults */
    sal_memcpy(&entry,  soc_mem_entry_null(unit, L3_IIFm),
               sizeof(iif_entry_t));

    /* Set enable global route value. */
    if (soc_mem_field_valid(unit, L3_IIFm, ALLOW_GLOBAL_ROUTEf)) {
        soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                            ALLOW_GLOBAL_ROUTEf, 1);
    }

    /* Set identity mapping for IPMC L3 IIF */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_mem_field_valid(unit, L3_IIFm, IPMC_L3_IIFf)) {
        soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                            IPMC_L3_IIFf, intf_id);
    }
#endif

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_mem_field_valid(unit, L3_IIFm, URPF_DEFAULTROUTECHECKf)) {
         soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                             URPF_DEFAULTROUTECHECKf, 1);
    }
#endif

    /* Set appropriate default TRUST_DSCP_PTR mapping */
    if (soc_mem_field_valid(unit, L3_IIFm, TRUST_DSCP_PTRf)) {
        bit_num = soc_mem_field_length(unit, L3_IIFm, TRUST_DSCP_PTRf);

        switch (bit_num) {
            case _BCM_TR_L3_TRUST_DSCP_PTR_BIT_SIZE:
                /* DSCP_TABLE size vary across Chariot and Enduro,
                 * but Do not trust value is identical */
                soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                                    TRUST_DSCP_PTRf, 63);
                break;
            case _BCM_TD_L3_TRUST_DSCP_PTR_BIT_SIZE:
                /* Trident and Triumph3 */
                if ((BCM_VLAN_INVALID < intf_id) || SOC_IS_TRIUMPH3(unit)) {
                    soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                                        TRUST_DSCP_PTRf, 127);
                } else {
                    /* For Trident, if intf_id < BCM_VLAN_INVALID */
                    soc_mem_field32_set(unit, L3_IIFm, (uint32 *)&entry,
                                        TRUST_DSCP_PTRf, 0);
                }
                break;
            default:
                break;
        }
    }

    /* Write  updated entry back to hw.*/
    return  soc_mem_write(unit, L3_IIFm, MEM_BLOCK_ALL, intf_id, 
                          (uint32 *)&entry);
}

/*
 * Function:
 *      _bcm_tr_l3_intf_class_set
 * Purpose:
 *      Set classigication class to the interface. 
 * Parameters:
 *      unit       - (IN) SOC unit number.
 *      vid        - (IN) vlan id.
 *      intf_class - (IN) Interface class id.
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_intf_class_set(int unit, bcm_vlan_t vid, uint32 intf_class)
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/
    int ret_val;                /* Operation return value.  */

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    if (intf_class> SOC_INTF_CLASS_MAX(unit)) {
        return (BCM_E_PARAM);
    }
    
    iif.intf_id = vid;
    soc_mem_lock(unit, L3_IIFm);

    ret_val = _bcm_tr_l3_ingress_interface_get(unit, &iif);
    if (BCM_FAILURE(ret_val)) {
        soc_mem_unlock(unit, L3_IIFm);
        return (ret_val);
    }

    iif.if_class = intf_class;
#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit, soc_feature_l3_iif_profile)) {
        iif.profile_flags |= _BCM_L3_IIF_PROFILE_DO_NOT_UPDATE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    ret_val = _bcm_tr_l3_ingress_interface_set(unit, &iif);

    soc_mem_unlock(unit, L3_IIFm);

    return (ret_val);
}

/*
 * Function:
 *      _bcm_tr_l3_intf_global_route_enable_get
 * Purpose:
 *      Get the global route enable flag for the specified L3 interface
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      enable    - (OUT)enable. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_intf_global_route_enable_get(int unit, bcm_vlan_t vid, int *enable)
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;

    BCM_IF_ERROR_RETURN(_bcm_tr_l3_ingress_interface_get(unit, &iif));

    *enable = (iif.flags & BCM_VLAN_L3_VRF_GLOBAL_DISABLE) ? 0 : 1;
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr_l3_intf_global_route_enable_set
 * Purpose:
 *      Set global route enable flag for the L3 interface. 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      enable    - (IN)Enable. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_intf_global_route_enable_set(int unit, bcm_vlan_t vid, int enable)  
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
       iif.flags &= ~BCM_VLAN_L3_VRF_GLOBAL_DISABLE;
    } else {
       iif.flags |= BCM_VLAN_L3_VRF_GLOBAL_DISABLE;
    }

    ret_val = _bcm_tr_l3_ingress_interface_set(unit, &iif);

    soc_mem_unlock(unit, L3_IIFm);

    return (ret_val);
}

#if defined (BCM_TRIDENT2_SUPPORT)
/*
 * Function:
 *      _bcm_tr_l3_intf_nat_realm_id_get
 * Purpose:
 *      Get the src_realm_id for the specified L3 interface
 * Parameters:
 *      unit            - (IN)SOC unit number.
 *      vid             - (IN)Vlan id. 
 *      nat_realm_id    - (OUT)realm_id. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_intf_nat_realm_id_get(int unit, bcm_vlan_t vid, int *realm_id)
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/

    if (!soc_feature(unit, soc_feature_nat)) {
        return BCM_E_UNAVAIL;
    }

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;

    BCM_IF_ERROR_RETURN(_bcm_tr_l3_ingress_interface_get(unit, &iif));

    *realm_id = iif.nat_realm_id;
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr_l3_intf_nat_realm_id_set
 * Purpose:
 *      Bind interface to the VRF & update if info.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      vrf       - (OUT)Vrf. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_intf_nat_realm_id_set(int unit, bcm_vlan_t vid, int realm_id)  
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/
    int ret_val;                /* Operation return value.  */

    if (!soc_feature(unit, soc_feature_nat)) {
        return BCM_E_UNAVAIL;
    }

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    if ((realm_id < 0) || (realm_id > 3)) {
        return (BCM_E_PARAM);
    }
    
    iif.intf_id = vid;
    soc_mem_lock(unit, L3_IIFm);

    ret_val = _bcm_tr_l3_ingress_interface_get(unit, &iif);
    if (BCM_FAILURE(ret_val)) {
        soc_mem_unlock(unit, L3_IIFm);
        return (ret_val);
    }

    iif.nat_realm_id = realm_id;
    if (soc_feature(unit, soc_feature_l3_iif_profile)) {
        iif.profile_flags |= _BCM_L3_IIF_PROFILE_DO_NOT_UPDATE;
    }

    ret_val = _bcm_tr_l3_ingress_interface_set(unit, &iif);

    soc_mem_unlock(unit, L3_IIFm);

    return (ret_val);
}
#endif /* BCM_TRIDENT2_SUPPORT */


/*
 * Function:
 *      _bcm_tr_l3_intf_vrf_get
 * Purpose:
 *      Get the VRF,flags info for the specified L3 interface
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      vrf       - (OUT)Vrf. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_intf_vrf_get(int unit, bcm_vlan_t vid, bcm_vrf_t *vrf)
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;

    BCM_IF_ERROR_RETURN(_bcm_tr_l3_ingress_interface_get(unit, &iif));

    *vrf = iif.vrf;
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr_l3_intf_vrf_bind
 * Purpose:
 *      Bind interface to the VRF & update if info.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vid       - (IN)Vlan id. 
 *      vrf       - (OUT)Vrf. 
 * Returns:
 *      BCM_E_XXX.
 */
int
_bcm_tr_l3_intf_vrf_bind(int unit, bcm_vlan_t vid, bcm_vrf_t vrf)  
{
    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/
    int ret_val;                /* Operation return value.  */

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    if (vrf > SOC_VRF_MAX(unit)) {
        return (BCM_E_PARAM);
    }
    
    iif.intf_id = vid;
    soc_mem_lock(unit, L3_IIFm);

    ret_val = _bcm_tr_l3_ingress_interface_get(unit, &iif);
    if (BCM_FAILURE(ret_val)) {
        soc_mem_unlock(unit, L3_IIFm);
        return (ret_val);
    }

    iif.vrf = vrf;
#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit, soc_feature_l3_iif_profile)) {
        iif.profile_flags |= _BCM_L3_IIF_PROFILE_DO_NOT_UPDATE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    ret_val = _bcm_tr_l3_ingress_interface_set(unit, &iif);

    soc_mem_unlock(unit, L3_IIFm);

    return (ret_val);
}

/*
 * Function:
 *      _bcm_tr_l3_defip_mem_get
 * Purpose:
 *      Resolve route table memory for a given route entry.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      flags      - (IN)IPv6/IPv4 route.
 *      plen       - (IN)Prefix length.
 *      mem        - (OUT)Route table memory.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr_l3_defip_mem_get(int unit, uint32 flags, int plen, soc_mem_t *mem)
{
#if defined(BCM_VALKYRIE_SUPPORT) || defined(BCM_ENDURO_SUPPORT) || \
    defined(BCM_APOLLO_SUPPORT) || defined(BCM_VALKYRIE2_SUPPORT) || \
    defined(BCM_TRIDENT_SUPPORT)
    if (!soc_feature(unit, soc_feature_esm_support)) {
        if (flags & BCM_L3_IP6) {
#ifdef BCM_KATANA2_SUPPORT
            if (SOC_IS_KATANA2(unit)) {
                *mem = L3_DEFIPm;
            } else
#endif /* BCM_KATANA2_SUPPORT */
            if (plen > 64) {
                *mem = L3_DEFIP_128m;
            } else {  /* IPv6 prefix length < 64 bit. */
                *mem = L3_DEFIPm;
            }
        } else { /* IPv4 route. */
            *mem = L3_DEFIPm;
        }
        return (BCM_E_NONE);
    }
#endif
    if (flags & BCM_L3_IP6) {
        if (plen > 64) {
            *mem = L3_DEFIP_128m;
            if (soc_feature(unit, soc_feature_esm_support) &&
                SOC_MEM_IS_ENABLED(unit, EXT_IPV6_128_DEFIPm)) {
                if (soc_mem_index_count(unit, EXT_IPV6_128_DEFIPm)) {
                    *mem = EXT_IPV6_128_DEFIPm;
                }
            } 
        } else {  /* IPv6 prefix length < 64 bit. */
            if (soc_feature(unit, soc_feature_esm_support) && 
                (SOC_MEM_IS_ENABLED(unit, EXT_IPV6_128_DEFIPm) ||
                 SOC_MEM_IS_ENABLED(unit, EXT_IPV6_64_DEFIPm))) {
                if (soc_mem_index_count(unit, EXT_IPV6_128_DEFIPm)) {
                    *mem = EXT_IPV6_128_DEFIPm;
                } else if (soc_mem_index_count(unit, EXT_IPV6_64_DEFIPm)) {
                    *mem = EXT_IPV6_64_DEFIPm;
                } else { /* Defaults to internal memory even if soc feature esm
                         is enabled, but device not configured to use ext mem */
                    *mem = L3_DEFIPm;
                }
            } else {
                *mem = L3_DEFIPm;
            }
        }
    } else { /* IPv4 route. */
        *mem = L3_DEFIPm; 
        if (soc_feature(unit, soc_feature_esm_support) &&
            SOC_MEM_IS_ENABLED(unit, EXT_IPV4_DEFIPm)) {
            if (soc_mem_index_count(unit, EXT_IPV4_DEFIPm)) {
                *mem = EXT_IPV4_DEFIPm;
            }
        } 
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr_l3_enable
 * Purpose:
 *      Resolve route table memory for a given route entry.
 * Parameters:
 *      unit       - (IN)BCM device number.
 *      port       - (IN)Port number.
 *      flags      - (IN)IPv6/IPv4 flag.
 *      enable     - (IN)Enable /Disable 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr_l3_enable(int unit, bcm_port_t port, uint32 flags, int enable)
{
    int rv;
    soc_field_t fields[2];
    uint32 values[2] = {1, 0};
    uint32 rval;

    if (!soc_feature(unit, soc_feature_esm_support)) {
        /* No esm on this devices. */
        return (BCM_E_NONE);
    }

    BCM_IF_ERROR_RETURN(READ_ESM_KEYGEN_CTLr(unit, &rval));

    if (flags & BCM_L3_IP6) {
        fields[0] = IPV6_FWD_MODEf;
        fields[1] = IPV6_128_ENf;

        if (enable) {
            
            values[0] = 2; /* L3 forwarding mode */
            /* Enable 128 bit forwarding if 128 bit table is present.*/
            if (soc_mem_index_count(unit, EXT_IPV6_128_DEFIPm)) {
                values[1] = 1;
            }
        } else {
            values[0] = 1; /* use L2 to forward IPv6 packet */
        }
        rv = soc_reg_fields32_modify(unit, ESM_MODE_PER_PORTr, port,
                                     2, fields, values);
    } else {
        fields[0] = IPV4_FWD_MODEf;
        if (enable) {
            
            values[0] = 2; /* L3 forwarding mode */
        } else {
            values[0] = 1; /* use L2 to forward IPv4 packet */
        }
        rv = soc_reg_fields32_modify(unit, ESM_MODE_PER_PORTr, port, 
                                     1, fields, values);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr_l3_clear_hit
 * Purpose:
 *      Clear hit bit on l3 entry
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      mem       - (IN)L3 table memory.
 *      l3cfg     - (IN)l3 entry info. 
 *      l3x_entry - (IN)l3 entry filled hw buffer.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr_l3_clear_hit(int unit, soc_mem_t mem,
                       _bcm_l3_cfg_t *l3cfg, void *l3x_entry)
{
    uint32 *buf_p;                /* HW buffer address.       */ 
    int mcast;                    /* Entry is multicast flag. */ 
    int ipv6;                     /* Entry is IPv6 flag.      */
    int idx;                      /* Iterator index.          */
    soc_field_t hitf[] = { HIT_0f, HIT_1f, HIT_2f, HIT_3f };

    /* Input parameters check */
    if ((NULL == l3cfg) || (NULL == l3x_entry)) {
        return (BCM_E_PARAM);
    }

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);
    mcast = (l3cfg->l3c_flags & BCM_L3_IPMC);

    /* Init memory pointers. */ 
    buf_p = (uint32 *)l3x_entry;

    /* If entry was not hit  there is nothing to clear */
    if (!(l3cfg->l3c_flags & BCM_L3_HIT)) {
        return (BCM_E_NONE);
    }

    /* Reset entry hit bit in hw. */
    if (ipv6 && mcast) {
        /* IPV6 multicast entry hit reset. */
        for (idx = 1; idx < 4; idx++) {
            soc_mem_field32_set(unit, mem, buf_p, hitf[idx], 0);
        }
    } else if (ipv6 || mcast) {
        /* Reset IPV6 unicast  hit bit. */
        for (idx = 1; idx < 2; idx++) {
            soc_mem_field32_set(unit, mem, buf_p, hitf[idx], 0);
        }
    }

    /* Reset hit bit. */
    soc_mem_field32_set(unit, mem, buf_p, hitf[0], 0);

    /* Write entry back to hw. */
    return BCM_XGS3_MEM_WRITE(unit, mem, l3cfg->l3c_hw_index, buf_p);
}

/*
 * Function:
 *      _bcm_tr_l3_ipmc_ent_init
 * Purpose:
 *      Set GROUP/SOURCE/VID/IMPC flag in the entry.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      l3x_entry - (IN/OUT) IPMC entry to fill. 
 *      l3cfg     - (IN) Api IPMC data. 
 * Returns:
 *    BCM_E_XXX
 */
void
_bcm_tr_l3_ipmc_ent_init(int unit, uint32 *buf_p,
                             _bcm_l3_cfg_t *l3cfg)
{
    soc_mem_t mem;                     /* IPMC table memory.    */
    int ipv6;                          /* IPv6 entry indicator. */
    int idx;                           /* Iteration index.      */
    soc_field_t typef[] = { KEY_TYPE_0f, KEY_TYPE_1f, 
                            KEY_TYPE_2f, KEY_TYPE_3f };
    soc_field_t validf[] = { VALID_0f, VALID_1f, VALID_2f, VALID_3f };

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    if (ipv6) {
        /* Set group address. */
        soc_mem_ip6_addr_set(unit, mem, buf_p, GROUP_IP_ADDR_LWR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_LOWER_ONLY);
        l3cfg->l3c_ip6[0] = 0x0;    /* Don't write ff entry already mcast. */
        soc_mem_ip6_addr_set(unit, mem, buf_p, GROUP_IP_ADDR_UPR_56f, 
                             l3cfg->l3c_ip6, SOC_MEM_IP6_UPPER_ONLY);
        l3cfg->l3c_ip6[0] = 0xff;    /* Restore The entry  */

        /* Set source  address. */
        soc_mem_ip6_addr_set(unit, mem, buf_p, SOURCE_IP_ADDR_LWR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_LOWER_ONLY);
        soc_mem_ip6_addr_set(unit, mem, buf_p, SOURCE_IP_ADDR_UPR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_UPPER_ONLY);

        for (idx = 0; idx < 4; idx++) {
            /* Set key type */
            soc_mem_field32_set(unit, mem, buf_p, typef[idx], 
                                TR_L3_HASH_KEY_TYPE_V6MC);

            /* Set entry valid bit. */
            soc_mem_field32_set(unit, mem, buf_p, validf[idx], 1);

        }
    } else {
        /* Set group id. */
        soc_mem_field32_set(unit, mem, buf_p, GROUP_IP_ADDRf,
                            l3cfg->l3c_ip_addr);

        /* Set source address. */
        soc_mem_field32_set(unit, mem, buf_p, SOURCE_IP_ADDRf,
                            l3cfg->l3c_src_ip_addr);

        for (idx = 0; idx < 2; idx++) {
            /* Set key type */
            soc_mem_field32_set(unit, mem, buf_p, typef[idx], 
                                TR_L3_HASH_KEY_TYPE_V4MC);

            /* Set entry valid bit. */
            soc_mem_field32_set(unit, mem, buf_p, validf[idx], 1);
        }
    }

    /* Set vlan id or L3_IIF. */
    if (l3cfg->l3c_vid < 4096) {
        soc_mem_field32_set(unit, mem, buf_p, VLAN_IDf, l3cfg->l3c_vid);
    } else if (soc_mem_field_valid(unit, mem, L3_IIFf)) {
        soc_mem_field32_set(unit, mem, buf_p, L3_IIFf, l3cfg->l3c_vid);
    }

    /* Set virtual router id. */
    if (SOC_MEM_FIELD_VALID(unit, mem, VRF_IDf)) {
        soc_mem_field32_set(unit, mem, buf_p, VRF_IDf, l3cfg->l3c_vrf);
    }
    return;
}

/*
 * Function:
 *      _bcm_tr_l3_ipmc_ent_parse
 * Purpose:
 *      Service routine used to parse hw l3 ipmc entry to api format.
 * Parameters:
 *      unit      - (IN)SOC unit number. 
 *      l3cfg     - (IN/OUT)l3 entry parsing destination buf.
 *      l3x_entry - (IN/OUT)hw buffer.
 * Returns:
 *      void
 */
STATIC INLINE void
_bcm_tr_l3_ipmc_ent_parse(int unit, _bcm_l3_cfg_t *l3cfg,
                          l3_entry_ipv6_multicast_entry_t *l3x_entry)
{
    soc_mem_t mem;                     /* IPMC table memory.    */
    uint32 *buf_p;                     /* HW buffer address.    */
    int ipv6;                          /* IPv6 entry indicator. */
    int idx;                           /* Iteration index.      */
    soc_field_t hitf[] = {HIT_1f, HIT_2f, HIT_3f };

    buf_p = (uint32 *)l3x_entry;

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    /* Mark entry as multicast & clear rest of the flags. */
    l3cfg->l3c_flags = BCM_L3_IPMC;
    if (ipv6) {
       l3cfg->l3c_flags |= BCM_L3_IP6;
    }

    /* Read hit value. */
    if(soc_mem_field32_get(unit, mem, buf_p, HIT_0f)) { 
        l3cfg->l3c_flags |= BCM_L3_HIT;
    } else if (ipv6) {
        /* Get hit bit. */
        for (idx = 0; idx < 3; idx++) {
            if(soc_mem_field32_get(unit, mem, buf_p, hitf[idx])) { 
                l3cfg->l3c_flags |= BCM_L3_HIT;
                break;
            }
        }
    }

    /* Set ipv6 group address to multicast. */
    if (ipv6) {
        l3cfg->l3c_ip6[0] = 0xff;   /* Set entry ip to mcast address. */
    }

    /* Read priority override */
    if(soc_mem_field32_get(unit, mem, buf_p, RPEf)) { 
        l3cfg->l3c_flags |= BCM_L3_RPE;
    }

    /* Read destination discard bit. */
    if(soc_mem_field32_get(unit, mem, buf_p, DST_DISCARDf)) { 
        l3cfg->l3c_flags |= BCM_L3_DST_DISCARD;
    }

    /* Read Virtual Router Id. */
    if(!(SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit))) { 
        l3cfg->l3c_vrf = soc_mem_field32_get(unit, mem, buf_p, VRF_IDf);
    }

    /* Pointer to ipmc replication table. */
    l3cfg->l3c_ipmc_ptr = soc_mem_field32_get(unit, mem, buf_p, L3MC_INDEXf);

    /* Classification lookup class id. */
    l3cfg->l3c_lookup_class = soc_mem_field32_get(unit, mem, buf_p, CLASS_IDf);

    /* Read priority value. */
    l3cfg->l3c_prio = soc_mem_field32_get(unit, mem, buf_p, PRIf);
    return;
}

/*
 * Function:
 *      _bcm_tr_l3_ipmc_get
 * Purpose:
 *      Get l3 multicast entry.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      l3cfg     - (IN/OUT)Group/Source key & Get result buffer.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_tr_l3_ipmc_get(int unit, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t l3x_key;    /* Lookup key buffer.    */
    l3_entry_ipv6_multicast_entry_t l3x_entry;  /* Search result buffer. */
    int clear_hit;                              /* Clear hit flag.       */
    soc_mem_t mem;                              /* IPMC table memory.    */
    int ipv6;                                   /* IPv6 entry indicator. */
    int rv;                                     /* Return value.         */

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    /*  Zero buffers. */
    sal_memcpy(&l3x_key, soc_mem_entry_null(unit, mem), 
                   soc_mem_entry_words(unit,mem) * 4);
    sal_memcpy(&l3x_entry, soc_mem_entry_null(unit, mem), 
                   soc_mem_entry_words(unit,mem) * 4);

    /* Check if clear hit bit is required. */
    clear_hit = l3cfg->l3c_flags & BCM_L3_HIT_CLEAR;

    /* Lookup Key preparation. */
    _bcm_tr_l3_ipmc_ent_init(unit, (uint32 *)&l3x_key, l3cfg);

    /* Perform hw lookup. */
    MEM_LOCK(unit, mem);
    rv = soc_mem_search(unit, mem, MEM_BLOCK_ANY, &l3cfg->l3c_hw_index,
                        &l3x_key, &l3x_entry, 0);
    MEM_UNLOCK(unit, mem);
    BCM_XGS3_LKUP_IF_ERROR_RETURN(rv, BCM_E_NOT_FOUND);

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit)) {
        soc_mem_t mem_y;
        l3_entry_ipv6_multicast_entry_t l3x_entry_y;
        uint32 hit;
        mem_y = ipv6 ? L3_ENTRY_IPV6_MULTICAST_Ym : L3_ENTRY_IPV4_MULTICAST_Ym;
        BCM_IF_ERROR_RETURN
            (BCM_XGS3_MEM_READ(unit, mem_y, l3cfg->l3c_hw_index,
                               &l3x_entry_y));
        hit = soc_mem_field32_get(unit, mem, &l3x_entry, HIT_0f);
        hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_0f);
        soc_mem_field32_set(unit, mem, &l3x_entry, HIT_0f, hit);
        hit = soc_mem_field32_get(unit, mem, &l3x_entry, HIT_1f);
        hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_1f);
        soc_mem_field32_set(unit, mem, &l3x_entry, HIT_1f, hit);
        if (ipv6) {
            hit = soc_mem_field32_get(unit, mem, &l3x_entry, HIT_2f);
            hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_2f);
            soc_mem_field32_set(unit, mem, &l3x_entry, HIT_2f, hit);
            hit = soc_mem_field32_get(unit, mem, &l3x_entry, HIT_3f);
            hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_3f);
            soc_mem_field32_set(unit, mem, &l3x_entry, HIT_3f, hit);
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Extract buffer information. */
    _bcm_tr_l3_ipmc_ent_parse(unit, l3cfg, &l3x_entry);

    /* Clear the HIT bit */
    if (clear_hit) {
        BCM_IF_ERROR_RETURN(_bcm_tr_l3_clear_hit(unit, mem, l3cfg, &l3x_entry));
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr_l3_ipmc_add
 * Purpose:
 *      Add l3 multicast entry.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      l3cfg     - (IN/OUT)Group/Source key.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_tr_l3_ipmc_add(int unit, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t l3x_entry;  /* Write entry buffer.   */
    soc_mem_t mem;                              /* IPMC table memory.    */
    uint32 *buf_p;                              /* HW buffer address.    */
    int ipv6;                                   /* IPv6 entry indicator. */
    int idx;                                    /* Iteration index.      */
    int idx_max;                                /* Iteration index max.  */
    int rv;                                     /* Return value.         */
    soc_field_t hitf[] = { HIT_0f, HIT_1f, HIT_2f, HIT_3f };

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    /*  Zero buffers. */
    buf_p = (uint32 *)&l3x_entry; 
    sal_memcpy(buf_p, soc_mem_entry_null(unit, mem), 
                   soc_mem_entry_words(unit,mem) * 4);

   /* Prepare entry to write. */
    _bcm_tr_l3_ipmc_ent_init(unit, (uint32 *)&l3x_entry, l3cfg);

    /* Set priority override bit. */
    if (l3cfg->l3c_flags & BCM_L3_RPE) {
        soc_mem_field32_set(unit, mem, buf_p, RPEf, 1);
    }

    /* Set destination discard. */
    if (l3cfg->l3c_flags & BCM_L3_DST_DISCARD) {
        soc_mem_field32_set(unit, mem, buf_p, DST_DISCARDf, 1);
    }
	
    if(!(SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit))) { 
        /* Virtual router id. */
        soc_mem_field32_set(unit, mem, buf_p, VRF_IDf, l3cfg->l3c_vrf);
    }	

    /* Set priority. */
    soc_mem_field32_set(unit, mem, buf_p, PRIf, l3cfg->l3c_prio);

    /* Pointer to ipmc table. */
    soc_mem_field32_set(unit, mem, buf_p, L3MC_INDEXf, l3cfg->l3c_ipmc_ptr);

    /* Classification lookup class id. */
    soc_mem_field32_set(unit, mem, buf_p, CLASS_IDf, l3cfg->l3c_lookup_class);

    idx_max = (ipv6) ? 4 : 2;
    for (idx = 0; idx < idx_max; idx++) {
        /* Set hit bit. */
        if (l3cfg->l3c_flags & BCM_L3_HIT) {
            soc_mem_field32_set(unit, mem, buf_p, hitf[idx], 1);
        }
    }

    /* Write entry to the hw. */
    MEM_LOCK(unit, mem);
    /* Handle replacement. */
    if (BCM_XGS3_L3_INVALID_INDEX != l3cfg->l3c_hw_index) {
        rv = BCM_XGS3_MEM_WRITE(unit, mem, l3cfg->l3c_hw_index, buf_p);
    } else {
        rv = soc_mem_insert(unit, mem, MEM_BLOCK_ANY, (void *)buf_p);
    }

    /* Increment number of ipmc routes. */
    if (BCM_SUCCESS(rv) && (BCM_XGS3_L3_INVALID_INDEX == l3cfg->l3c_hw_index)) {
        (ipv6) ? BCM_XGS3_L3_IP6_IPMC_CNT(unit)++ : \
                 BCM_XGS3_L3_IP4_IPMC_CNT(unit)++;
    }
    /*    coverity[overrun-local : FALSE]    */
    MEM_UNLOCK(unit, mem);
    return rv;
}

/*
 * Function:
 *      _bcm_tr_l3_ipmc_del
 * Purpose:
 *      Delete l3 multicast entry.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      l3cfg     - (IN)Group/Source deletion key.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_tr_l3_ipmc_del(int unit, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t l3x_entry;  /* Delete buffer.          */
    soc_mem_t mem;                              /* IPMC table memory.      */
    int ipv6;                                   /* IPv6 entry indicator.   */
    int rv;                                     /* Operation return value. */

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    /*  Zero entry buffer. */
    sal_memcpy(&l3x_entry, soc_mem_entry_null(unit, mem), 
               soc_mem_entry_words(unit,mem) * 4);


    /* Key preparation. */
    _bcm_tr_l3_ipmc_ent_init(unit, (uint32 *)&l3x_entry, l3cfg);

    /* Delete the entry from hw. */
    MEM_LOCK(unit, mem);

    rv = soc_mem_delete(unit, mem, MEM_BLOCK_ANY, (void *)&l3x_entry);

    /* Decrement number of ipmc routes. */
    if (BCM_SUCCESS(rv)) {
        (ipv6) ? BCM_XGS3_L3_IP6_IPMC_CNT(unit)-- : \
            BCM_XGS3_L3_IP4_IPMC_CNT(unit)--;
    }
    MEM_UNLOCK(unit, mem);
    return rv;
}

/*
 * Function:
 *      _bcm_tr_l3_ipmc_get_by_idx
 * Purpose:
 *      Get l3 multicast entry by entry index.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      dma_ptr  - (IN)Table pointer in dma. 
 *      idx       - (IN)Index to read. 
 *      l3cfg     - (IN/OUT)Entry data.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_tr_l3_ipmc_get_by_idx(int unit, void *dma_ptr,
                           int idx, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t *l3x_entry_p; /* Read buffer address.    */
    l3_entry_ipv6_multicast_entry_t l3x_entry;    /* Read buffer.            */
    soc_mem_t mem;                                /* IPMC table memory.      */
    uint32 ipv6;                                  /* IPv6 entry indicator.   */
    int clear_hit;                                /* Clear hit bit indicator.*/
    int key_type;

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    /* Check if clear hit is required. */
    clear_hit = l3cfg->l3c_flags & BCM_L3_HIT_CLEAR;

    if (NULL == dma_ptr) {             /* Read from hardware. */
        /* Zero buffers. */
        l3x_entry_p = &l3x_entry;
        sal_memcpy(l3x_entry_p, soc_mem_entry_null(unit, mem), 
                   soc_mem_entry_words(unit,mem) * 4);

        /* Read entry from hw. */
        BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_READ(unit, mem, idx, l3x_entry_p));
    } else {                    /* Read from dma. */
        l3x_entry_p =
            soc_mem_table_idx_to_pointer(unit, mem,
                                         l3_entry_ipv6_multicast_entry_t *,
                                         dma_ptr, idx);
    }
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit)) {
        soc_mem_t mem_y;
        l3_entry_ipv6_multicast_entry_t l3x_entry_y;
        uint32 hit;
        mem_y = ipv6 ? L3_ENTRY_IPV6_MULTICAST_Ym : L3_ENTRY_IPV4_MULTICAST_Ym;
        BCM_IF_ERROR_RETURN
            (BCM_XGS3_MEM_READ(unit, mem_y, idx, &l3x_entry_y));
        hit = soc_mem_field32_get(unit, mem, l3x_entry_p, HIT_0f);
        hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_0f);
        soc_mem_field32_set(unit, mem, l3x_entry_p, HIT_0f, hit);
        hit = soc_mem_field32_get(unit, mem, l3x_entry_p, HIT_1f);
        hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_1f);
        soc_mem_field32_set(unit, mem, l3x_entry_p, HIT_1f, hit);
        if (ipv6) {
            hit = soc_mem_field32_get(unit, mem, l3x_entry_p, HIT_2f);
            hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_2f);
            soc_mem_field32_set(unit, mem, l3x_entry_p, HIT_2f, hit);
            hit = soc_mem_field32_get(unit, mem, l3x_entry_p, HIT_3f);
            hit |= soc_mem_field32_get(unit, mem_y, &l3x_entry_y, HIT_3f);
            soc_mem_field32_set(unit, mem, l3x_entry_p, HIT_3f, hit);
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Ignore invalid entries. */
    if (!soc_mem_field32_get(unit, mem, l3x_entry_p, VALID_0f)) {
        return (BCM_E_NOT_FOUND);
    }


    key_type = soc_mem_field32_get(unit, L3_ENTRY_ONLYm,
                                   l3x_entry_p, KEY_TYPEf);

    switch (key_type) {
      case TR_L3_HASH_KEY_TYPE_V4MC:
          l3cfg->l3c_flags = BCM_L3_IPMC;
          break;
      case TR_L3_HASH_KEY_TYPE_V6UC:
          l3cfg->l3c_flags = BCM_L3_IP6;
          break;
      case TR_L3_HASH_KEY_TYPE_V6MC:
          l3cfg->l3c_flags = BCM_L3_IP6 | BCM_L3_IPMC;
          break;
      default:
          /* Catch all other key types, set flags to zero so that the 
           * following flag check is effective for all key types */
          l3cfg->l3c_flags = 0;  
          break;
    }

    /* Ignore protocol mismatch & multicast entries. */
    if ((ipv6  != (l3cfg->l3c_flags & BCM_L3_IP6)) ||
        (!(l3cfg->l3c_flags & BCM_L3_IPMC))) {
        return (BCM_E_NOT_FOUND);
    }

    /* Set index to l3cfg. */
    l3cfg->l3c_hw_index = idx;

    if (ipv6) {
        /* Get group address. */
        soc_mem_ip6_addr_get(unit, mem, l3x_entry_p, GROUP_IP_ADDR_LWR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_LOWER_ONLY);

        soc_mem_ip6_addr_get(unit, mem, l3x_entry_p, GROUP_IP_ADDR_UPR_56f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_UPPER_ONLY);

        /* Get source  address. */
        soc_mem_ip6_addr_get(unit, mem, l3x_entry_p, SOURCE_IP_ADDR_LWR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_LOWER_ONLY);
        soc_mem_ip6_addr_get(unit, mem, l3x_entry_p, SOURCE_IP_ADDR_UPR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_UPPER_ONLY);

        l3cfg->l3c_ip6[0] = 0xff;    /* Set entry to multicast*/
    } else {
        /* Get group id. */
        l3cfg->l3c_ip_addr =
            soc_mem_field32_get(unit, mem, l3x_entry_p, GROUP_IP_ADDRf);

        /* Get source address. */
        l3cfg->l3c_src_ip_addr =
            soc_mem_field32_get(unit, mem,  l3x_entry_p, SOURCE_IP_ADDRf);
    }

    /* Get L3_IIF or vlan id. */
    if (soc_mem_field_valid(unit, mem, L3_IIFf)) {
        l3cfg->l3c_vid = soc_mem_field32_get(unit, mem, l3x_entry_p, L3_IIFf);
    } else {
        l3cfg->l3c_vid = soc_mem_field32_get(unit, mem, l3x_entry_p, VLAN_IDf);
    }

    /* Parse entry data. */
    _bcm_tr_l3_ipmc_ent_parse(unit, l3cfg, l3x_entry_p);

    /* Clear the HIT bit */
    if (clear_hit) {
        BCM_IF_ERROR_RETURN(_bcm_tr_l3_clear_hit(unit, mem, l3cfg, l3x_entry_p));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr_l3_intf_mtu_set
 * Purpose:
 *      Set  L3 interface MTU value. 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      intf_info - (IN)Interface information.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr_l3_intf_mtu_set(int unit, _bcm_l3_intf_cfg_t *intf_info)
{
    mtu_values_entry_t mtu_value_buf;  /* Buffer to write mtu. */
    uint32  *mtu_value_buf_p;          /* Write buffer address.*/
    void *null_entry = soc_mem_entry_null(unit, L3_MTU_VALUESm);

    /* Input parameters check */
    if (NULL == intf_info) {
        return (BCM_E_PARAM);
    }

    if(!SOC_MEM_FIELD32_VALUE_FIT(unit, L3_MTU_VALUESm, 
                                  MTU_SIZEf, intf_info->l3i_mtu)) {
        return (BCM_E_PARAM);
    }
    if ((intf_info->l3i_index < soc_mem_index_min(unit, L3_MTU_VALUESm)) || 
        (intf_info->l3i_index > soc_mem_index_max(unit, L3_MTU_VALUESm))) {
        return (BCM_E_PARAM);
    }

    /* Reset the buffer to default value. */
    mtu_value_buf_p = (uint32 *)&mtu_value_buf;
    sal_memcpy(mtu_value_buf_p, null_entry, sizeof(mtu_values_entry_t));

    if (intf_info->l3i_mtu) {
        /* Set mtu. */
        soc_mem_field32_set(unit, L3_MTU_VALUESm, mtu_value_buf_p, 
                            MTU_SIZEf, intf_info->l3i_mtu);
    }

    /* MTU index to be based on L3 IIF Index */
    return BCM_XGS3_MEM_WRITE(unit, L3_MTU_VALUESm, 
                              intf_info->l3i_index, mtu_value_buf_p);
}

/*
 * Function:
 *      _bcm_tr_l3_intf_mtu_get
 * Purpose:
 *      Get  L3 interface MTU value. 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      intf_info - (IN/OUT)Interface information with updated mtu.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr_l3_intf_mtu_get(int unit, _bcm_l3_intf_cfg_t *intf_info)
{
    mtu_values_entry_t mtu_value_buf;  /* Buffer to write mtu. */
    uint32  *mtu_value_buf_p;          /* Write buffer address.           */

    /* Input parameters check */
    if (NULL == intf_info) {
        return (BCM_E_PARAM);
    }

    if ((intf_info->l3i_index < soc_mem_index_min(unit, L3_MTU_VALUESm)) || 
        (intf_info->l3i_index > soc_mem_index_max(unit, L3_MTU_VALUESm))) {
        return (BCM_E_PARAM);
    }
    
    /* Zero the buffer. */
    mtu_value_buf_p = (uint32 *)&mtu_value_buf;
    sal_memset(mtu_value_buf_p, 0, sizeof(mtu_values_entry_t));
    
    /* MTU index to be based on L3 IIF Index */
    /* Read mtu table entry by index. */
    BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_READ(unit, L3_MTU_VALUESm,
                                          intf_info->l3i_index, 
                                          mtu_value_buf_p));
    intf_info->l3i_mtu = 
        soc_mem_field32_get(unit, L3_MTU_VALUESm, mtu_value_buf_p, MTU_SIZEf);

    return (BCM_E_NONE);
}

#else /* BCM_TRIUMPH_SUPPORT && INCLUDE_L3 */
int bcm_esw_triumph_l3_not_empty;
#endif /* BCM_TRIUMPH_SUPPORT && INCLUDE_L3 */
