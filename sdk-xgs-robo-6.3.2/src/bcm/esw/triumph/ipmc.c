/*
 * $Id: ipmc.c 1.94.2.2 Broadcom SDK $
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
 * Purpose:     Tracks and manages IPMC tables.
 */

#ifdef INCLUDE_L3

#include <soc/l3x.h>
#if defined(BCM_BRADLEY_SUPPORT)
#include <soc/bradley.h>
#endif /* BCM_BRADLEY_SUPPORT */

#include <bcm/error.h>
#include <bcm/ipmc.h>

#include <bcm_int/esw/ipmc.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/bradley.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/multicast.h>

#ifdef BCM_TRIUMPH2_SUPPORT
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
#include <soc/trident2.h>
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
#include <bcm_int/esw/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */

#include <bcm_int/esw_dispatch.h>

#define TR_IPMC_NO_SRC_CHECK_PORT(unit) ((SOC_IS_TD_TT(unit) || \
                                          SOC_IS_TRIUMPH3(unit) || \
                                          SOC_IS_KATANAX(unit)) ? 0x7f : \
                                          ((SOC_IS_ENDURO(unit) || \
                                           SOC_IS_HURRICANEX(unit)) ? 0x1f : 0x3f))

/*
 * Function:
 *      _bcm_tr_ipmc_l3entry_list_add
 * Purpose:
 *      Add a L3 entry to IPMC group's linked list of L3 entries
 * Parameters:
 *      unit       - (IN) BCM device number.
 *      ipmc_index - (IN) IPMC group ID.
 *      _bcm_l3_cfg_t - (IN) l3 config structure
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr_ipmc_l3entry_list_add(int unit, int ipmc_index,
                               _bcm_l3_cfg_t l3cfg)
{
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;
   
    ipmc_l3entry = sal_alloc(sizeof(_bcm_esw_ipmc_l3entry_t), "IPMC L3 entry");
    if (ipmc_l3entry == NULL) {
        return BCM_E_MEMORY;
    }
    ipmc_l3entry->ip6 = (l3cfg.l3c_flags & BCM_L3_IP6) ? 1 : 0;
    ipmc_l3entry->l3info.flags = l3cfg.l3c_flags;
    ipmc_l3entry->l3info.vrf = l3cfg.l3c_vrf;
    ipmc_l3entry->l3info.ip_addr = l3cfg.l3c_ipmc_group;
    ipmc_l3entry->l3info.src_ip_addr = l3cfg.l3c_src_ip_addr;
    sal_memcpy(ipmc_l3entry->l3info.ip6, l3cfg.l3c_ip6, BCM_IP6_ADDRLEN);
    sal_memcpy(ipmc_l3entry->l3info.sip6, l3cfg.l3c_sip6, BCM_IP6_ADDRLEN);
    ipmc_l3entry->l3info.vid = l3cfg.l3c_vid;
    ipmc_l3entry->l3info.prio = l3cfg.l3c_prio;
    ipmc_l3entry->l3info.ipmc_ptr = l3cfg.l3c_ipmc_ptr;
    ipmc_l3entry->l3info.lookup_class = l3cfg.l3c_lookup_class;
    ipmc_l3entry->l3info.rp_id = l3cfg.l3c_rp_id;
    ipmc_l3entry->next = IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list;
    IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list = ipmc_l3entry;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_ipmc_l3entry_list_del
 * Purpose:
 *      Delete a L3 entry from IPMC group's linked list of L3 entries
 * Parameters:
 *      unit       - (IN) BCM device number.
 *      ipmc_index - (IN) IPMC group ID.
 *      _bcm_l3_cfg_t - (IN) l3 config structure
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr_ipmc_l3entry_list_del(int unit, int ipmc_index,
                              _bcm_l3_cfg_t l3cfg)
{
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;
    _bcm_esw_ipmc_l3entry_t *prev_ipmc_l3entry = NULL;
   
    ipmc_l3entry = IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list;
    while (ipmc_l3entry != NULL) {
        if ((ipmc_l3entry->l3info.ip_addr == l3cfg.l3c_ip_addr) &&
            (ipmc_l3entry->l3info.src_ip_addr == l3cfg.l3c_src_ip_addr) && 
            (!sal_memcmp(ipmc_l3entry->l3info.ip6, l3cfg.l3c_ip6, BCM_IP6_ADDRLEN)) &&
            (!sal_memcmp(ipmc_l3entry->l3info.sip6, l3cfg.l3c_sip6, BCM_IP6_ADDRLEN)) &&
            (ipmc_l3entry->l3info.vid == l3cfg.l3c_vid) &&
            (ipmc_l3entry->l3info.vrf == l3cfg.l3c_vrf)) {
           if (ipmc_l3entry == IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list) {
               IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list = ipmc_l3entry->next;
           } else {
               /* 
                * In the following line of code, Coverity thinks the
                * prev_ipmc_l3entry pointer may still be NULL when 
                * dereferenced. This situation will never occur because 
                * if ipmc_l3entry is not pointing to the head of the 
                * linked list, prev_ipmc_l3entry would not be NULL.
                */
               /* coverity[var_deref_op : FALSE] */
               prev_ipmc_l3entry->next = ipmc_l3entry->next;
           }
           sal_free(ipmc_l3entry);
           break;
        }
        prev_ipmc_l3entry = ipmc_l3entry;
        ipmc_l3entry = ipmc_l3entry->next;
    }

    if (ipmc_l3entry == NULL) {
        return BCM_E_NOT_FOUND;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_tr_ipmc_l3entry_list_update(int unit, int ipmc_index,
                                 _bcm_l3_cfg_t l3cfg)
{
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;
   
    ipmc_l3entry = IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list;
    while (ipmc_l3entry != NULL) {
        if ((ipmc_l3entry->l3info.ip_addr == l3cfg.l3c_ip_addr) &&
            (ipmc_l3entry->l3info.src_ip_addr == l3cfg.l3c_src_ip_addr) && 
            (!sal_memcmp(ipmc_l3entry->l3info.ip6, l3cfg.l3c_ip6, BCM_IP6_ADDRLEN)) &&
            (!sal_memcmp(ipmc_l3entry->l3info.sip6, l3cfg.l3c_sip6, BCM_IP6_ADDRLEN)) &&
            (ipmc_l3entry->l3info.vid == l3cfg.l3c_vid)) {
            /* set new values */
            ipmc_l3entry->l3info.flags = l3cfg.l3c_flags;
            ipmc_l3entry->l3info.vrf = l3cfg.l3c_vrf;
            ipmc_l3entry->l3info.prio = l3cfg.l3c_prio;
            ipmc_l3entry->l3info.ipmc_ptr = l3cfg.l3c_ipmc_ptr;
            ipmc_l3entry->l3info.lookup_class = l3cfg.l3c_lookup_class;
            ipmc_l3entry->l3info.rp_id = l3cfg.l3c_rp_id;
            break;
        }
        ipmc_l3entry = ipmc_l3entry->next;
    }

    if (ipmc_l3entry == NULL) {
        return BCM_E_NOT_FOUND;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_ipmc_l3entry_list_size_get
 * Purpose:
 *      Get the number of L3 entries in IPMC group's linked list.
 * Parameters:
 *      unit       - (IN) BCM device number.
 *      ipmc_index - (IN) IPMC group ID.
 *      size       - (OUT) Number of L3 entries.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr_ipmc_l3entry_list_size_get(int unit, int ipmc_index,
        int *size)
{
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;

    *size = 0;
    ipmc_l3entry = IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list;
    while (ipmc_l3entry != NULL) {
        (*size)++;
        ipmc_l3entry = ipmc_l3entry->next;
    }

    return BCM_E_NONE;
}

#ifdef BCM_TRIUMPH2_SUPPORT
/*
 * Function:
 *      _tr2_ipmc_glp_get
 * Purpose:
 *      Fill source information to bcm_ipmc_addr_t struct.
 */
STATIC int
_tr2_ipmc_glp_get(int unit, bcm_ipmc_addr_t *ipmc, ipmc_1_entry_t *entry)

{
    int                 mod, port_tgid, is_trunk, rv = BCM_E_NONE;
    int                 no_src_check = FALSE;

    is_trunk = soc_L3_IPMC_1m_field32_get(unit, entry, Tf);
    mod = soc_L3_IPMC_1m_field32_get(unit, entry, MODULE_IDf);
    port_tgid = soc_L3_IPMC_1m_field32_get(unit, entry, PORT_NUMf);
    if (is_trunk) {
        if ((port_tgid == TR_IPMC_NO_SRC_CHECK_PORT(unit)) &&
                (mod == SOC_MODID_MAX(unit))) {
            no_src_check = TRUE;
        } else {
            mod = 0;
            port_tgid = soc_L3_IPMC_1m_field32_get(unit, entry, TGIDf);
        }
    }
    if (no_src_check) {
        ipmc->ts = 0;
        ipmc->mod_id = -1;
        ipmc->port_tgid = -1;
        ipmc->flags |= BCM_IPMC_SOURCE_PORT_NOCHECK;
    } else if (is_trunk) {
        ipmc->ts = 1;
        ipmc->mod_id = 0;
        ipmc->port_tgid = port_tgid;
    } else {
        bcm_module_t    mod_in, mod_out;
        bcm_port_t      port_in, port_out;

        mod_in = mod;
        port_in = port_tgid;
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                    mod_in, port_in,
                                    &mod_out, &port_out));
        ipmc->ts = 0;
        ipmc->mod_id = mod_out;
        ipmc->port_tgid = port_out;
    }
    return rv;
}
#endif

/*
 * Function:
 *      _tr_ipmc_info_get
 * Purpose:
 *      Fill information in bcm_ipmc_addr_t struct.
 */

STATIC int
_tr_ipmc_info_get(int unit, int ipmc_index, bcm_ipmc_addr_t *ipmc, 
                  ipmc_entry_t *entry, uint8 do_l3_lkup, 
                  _bcm_esw_ipmc_l3entry_t *use_ipmc_l3entry)
{
    int           mod = -1, port_tgid = -1, is_trunk = 0;
    int           no_src_check = FALSE;

    ipmc->v = soc_L3_IPMCm_field32_get(unit, entry, VALIDf);

    if (soc_mem_field_valid(unit, L3_IPMCm, PORT_NUMf)) {
        is_trunk = soc_L3_IPMCm_field32_get(unit, entry, Tf);
        mod = soc_L3_IPMCm_field32_get(unit, entry, MODULE_IDf);
        port_tgid = soc_L3_IPMCm_field32_get(unit, entry, PORT_NUMf);
        if (is_trunk) {
            if ((port_tgid == TR_IPMC_NO_SRC_CHECK_PORT(unit)) &&
                    (mod == SOC_MODID_MAX(unit))) {
                no_src_check = TRUE;
            } else {
                mod = 0;
                port_tgid = soc_L3_IPMCm_field32_get(unit, entry, TGIDf);
            }
        }
        if (no_src_check) {
            ipmc->ts = 0;
            ipmc->mod_id = -1;
            ipmc->port_tgid = -1;
            ipmc->flags |= BCM_IPMC_SOURCE_PORT_NOCHECK;
        } else if (is_trunk) {
            ipmc->ts = 1;
            ipmc->mod_id = 0;
            ipmc->port_tgid = port_tgid;
        } else {
            bcm_module_t    mod_in, mod_out;
            bcm_port_t      port_in, port_out;

            mod_in = mod;
            port_in = port_tgid;
            BCM_IF_ERROR_RETURN
                (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                        mod_in, port_in,
                                        &mod_out, &port_out));
            ipmc->ts = 0;
            ipmc->mod_id = mod_out;
            ipmc->port_tgid = port_out;
        }
    }

    if ((ipmc->v) && (do_l3_lkup)) {
        _bcm_l3_cfg_t l3cfg;
        _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;

        if (use_ipmc_l3entry) {
            /* use the passed in l3 info */
            ipmc_l3entry = use_ipmc_l3entry;
        } else {
            /* Note: this simply picks up the first l3 info */
            ipmc_l3entry = IPMC_GROUP_INFO(unit, ipmc_index)->l3entry_list;
            if (NULL == ipmc_l3entry) {
                /* No entries in Multicast host table */
                return BCM_E_EMPTY;
            }
        }
        sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
        l3cfg.l3c_flags = BCM_L3_IPMC;
        l3cfg.l3c_vrf = ipmc_l3entry->l3info.vrf;
        l3cfg.l3c_vid = ipmc_l3entry->l3info.vid;

        if (ipmc_l3entry->ip6) {
            ipmc->flags |= BCM_IPMC_IP6;
        } else {
            ipmc->flags &= ~BCM_IPMC_IP6;
        }
        if (ipmc->flags & BCM_IPMC_HIT_CLEAR) {
            l3cfg.l3c_flags |= BCM_L3_HIT_CLEAR;
        }
        
        /* need to get current l3 info from h/w like hit bits */
        if (ipmc->flags & BCM_IPMC_IP6) {
            sal_memcpy(ipmc->s_ip6_addr, &ipmc_l3entry->l3info.sip6, BCM_IP6_ADDRLEN);
            sal_memcpy(ipmc->mc_ip6_addr, &ipmc_l3entry->l3info.ip6, BCM_IP6_ADDRLEN);
            sal_memcpy(l3cfg.l3c_sip6, &ipmc_l3entry->l3info.sip6, BCM_IP6_ADDRLEN);
            sal_memcpy(l3cfg.l3c_ip6, &ipmc_l3entry->l3info.ip6, BCM_IP6_ADDRLEN);
            l3cfg.l3c_flags |= BCM_L3_IP6;
            BCM_IF_ERROR_RETURN(mbcm_driver[unit]->mbcm_l3_ip6_get(unit, &l3cfg));
        } else {
            ipmc->s_ip_addr = ipmc_l3entry->l3info.src_ip_addr;
            ipmc->mc_ip_addr = ipmc_l3entry->l3info.ipmc_group;
            l3cfg.l3c_src_ip_addr = ipmc_l3entry->l3info.src_ip_addr;
            l3cfg.l3c_ipmc_group = ipmc_l3entry->l3info.ipmc_group;
            BCM_IF_ERROR_RETURN(mbcm_driver[unit]->mbcm_l3_ip4_get(unit, &l3cfg));
        }
        
        if (l3cfg.l3c_flags & BCM_L3_HIT) {
            ipmc->flags |= BCM_IPMC_HIT;
        }

        if (l3cfg.l3c_flags & BCM_IPMC_POST_LOOKUP_RPF_CHECK) {
            ipmc->flags |= BCM_IPMC_POST_LOOKUP_RPF_CHECK;
            ipmc->l3a_intf = l3cfg.l3c_intf;
            if (l3cfg.l3c_flags & BCM_IPMC_RPF_FAIL_DROP) {
                ipmc->flags |= BCM_IPMC_RPF_FAIL_DROP;
            }
            if (l3cfg.l3c_flags & BCM_IPMC_RPF_FAIL_TOCPU) {
                ipmc->flags |= BCM_IPMC_RPF_FAIL_TOCPU;
            }
        }

        if (ipmc_l3entry->l3info.flags & BCM_L3_RPE) {
            ipmc->cos = ipmc_l3entry->l3info.prio;
            ipmc->flags |= BCM_IPMC_SETPRI;
        } else {
            ipmc->cos = -1;
            ipmc->flags &= ~BCM_IPMC_SETPRI;
        }
        ipmc->group = ipmc_index;
        ipmc->lookup_class = ipmc_l3entry->l3info.lookup_class;
        ipmc->vrf = ipmc_l3entry->l3info.vrf;
        ipmc->vid = ipmc_l3entry->l3info.vid;
        ipmc->rp_id = ipmc_l3entry->l3info.rp_id;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _tr_ipmc_write
 * Purpose:
 *      Write an ipmc entry from bcm_ipmc_addr_t struct.
 */

STATIC int
_tr_ipmc_write(int unit, int ipmc_id, bcm_ipmc_addr_t *ipmc)
{
    int                 rv;
    ipmc_entry_t        entry;
    int                 mod, port_tgid, is_trunk, no_src_check = FALSE;
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    ipmc_1_entry_t      entry_1;
    sal_memset(&entry_1, 0, sizeof(ipmc_1_entry_t));
#endif

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, ipmc_id, &entry));

    soc_L3_IPMCm_field32_set(unit, &entry, VALIDf, ipmc->v);

#ifdef BCM_KATANA_SUPPORT
    if (soc_mem_field_valid(unit,L3_IPMCm,MMU_MC_REDIRECTION_PTRf)) {
        soc_mem_field32_set(unit, L3_IPMCm, &entry, 
                            MMU_MC_REDIRECTION_PTRf, ipmc_id);
 
    }
#endif

    if ((ipmc->flags & BCM_IPMC_SOURCE_PORT_NOCHECK) ||
        (ipmc->port_tgid < 0)) {                        /* no source port */
        no_src_check = TRUE;
        is_trunk = 0;
        mod = SOC_MODID_MAX(unit);
        port_tgid = TR_IPMC_NO_SRC_CHECK_PORT(unit);
    } else if (ipmc->ts) {                              /* trunk source port */
        is_trunk = 1;
        mod = 0;
        port_tgid = ipmc->port_tgid;
    } else {                                            /* source port */
        bcm_module_t    mod_in, mod_out;
        bcm_port_t      port_in, port_out;

        mod_in = ipmc->mod_id;
        port_in = ipmc->port_tgid;
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET,
                                    mod_in, port_in,
                                    &mod_out, &port_out));
        /* Check parameters, since above is an application callback */
        if (!SOC_MODID_ADDRESSABLE(unit, mod_out)) {
            return BCM_E_BADID;
        }
        if (!SOC_PORT_ADDRESSABLE(unit, port_out)) {
            return BCM_E_PORT;
        }
        is_trunk = 0;
        mod = mod_out;
        port_tgid = port_out;
    }

    if (is_trunk) {
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
        if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
            soc_L3_IPMC_1m_field32_set(unit, &entry_1, Tf, 1);
            soc_L3_IPMC_1m_field32_set(unit, &entry_1, TGIDf, port_tgid);
        } else
#endif
        {
            soc_L3_IPMCm_field32_set(unit, &entry, Tf, 1);
            soc_L3_IPMCm_field32_set(unit, &entry, TGIDf, port_tgid);
        }
    } else {
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
        if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
            soc_L3_IPMC_1m_field32_set(unit, &entry_1, MODULE_IDf, mod);
            soc_L3_IPMC_1m_field32_set(unit, &entry_1, PORT_NUMf, port_tgid);
            if (no_src_check) {
                soc_L3_IPMC_1m_field32_set(unit, &entry_1, Tf, 1);
            } else {
                soc_L3_IPMC_1m_field32_set(unit, &entry_1, Tf, 0);
            }
        } else
#endif
        {
            soc_L3_IPMCm_field32_set(unit, &entry, MODULE_IDf, mod);
            soc_L3_IPMCm_field32_set(unit, &entry, PORT_NUMf, port_tgid);
            if (no_src_check) {
                soc_L3_IPMCm_field32_set(unit, &entry, Tf, 1);
            } else {
                soc_L3_IPMCm_field32_set(unit, &entry, Tf, 0);
            }
        }
    }
    rv = soc_mem_write(unit, L3_IPMCm, MEM_BLOCK_ALL, ipmc_id, &entry);
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
        rv = soc_mem_write(unit, L3_IPMC_1m, MEM_BLOCK_ALL, ipmc_id, &entry_1);
    }
#endif
    return (rv);
}

/*
 * Function:
 *      _tr_ipmc_enable
 * Purpose:
 *      Enable/disable IPMC support.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      enable - TRUE: enable IPMC support.
 *               FALSE: disable IPMC support.
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_tr_ipmc_enable(int unit, int enable)
{
    int        port, do_vlan;
    bcm_pbmp_t port_pbmp;

    enable = enable ? 1 : 0;
    do_vlan = soc_property_get(unit, spn_IPMC_DO_VLAN, 1);

    BCM_PBMP_CLEAR(port_pbmp);
    BCM_PBMP_ASSIGN(port_pbmp, PBMP_PORT_ALL(unit));
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit) && soc_feature(unit, soc_feature_flex_port)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update(unit, &port_pbmp));
    }
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &port_pbmp);
    }
#endif

    PBMP_ITER(port_pbmp, port) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port,
                                      _bcmPortIpmcV4Enable, enable));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port,
                                      _bcmPortIpmcV6Enable, enable));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_set(unit, port,
                                      _bcmPortIpmcVlanKey,
                                      (enable && do_vlan) ? 1 : 0));
    }

#if defined(BCM_TRX_SUPPORT)
    if (soc_feature(unit, soc_feature_lport_tab_profile)) {
        /* Update LPORT Profile Table */
        BCM_IF_ERROR_RETURN
            (_bcm_lport_profile_field32_modify(unit, V4IPMC_ENABLEf, enable));
        BCM_IF_ERROR_RETURN
            (_bcm_lport_profile_field32_modify(unit, V6IPMC_ENABLEf, enable));
        BCM_IF_ERROR_RETURN
            (_bcm_lport_profile_field32_modify(unit, IPMC_DO_VLANf,
                                               (enable && do_vlan) ? 1 : 0));
    }
#endif /* BCM_TRX_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_ipmc_init
 * Purpose:
 *      Initialize the IPMC module and enable IPMC support.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      This function has to be called before any other IPMC functions.
 */

int
bcm_tr_ipmc_init(int unit)
{
    egr_ipmc_entry_t egr_entry;
#if defined(BCM_TRIUMPH2_SUPPORT)
    ipmc_remap_entry_t remap_entry;
#endif
    _bcm_esw_ipmc_t  *info = IPMC_INFO(unit);
    int i, rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(bcm_tr_ipmc_detach(unit));
    BCM_IF_ERROR_RETURN(_tr_ipmc_enable(unit, TRUE));

    IPMC_GROUP_NUM(unit) = soc_mem_index_count(unit, L3_IPMCm);
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
        soc_info_t *si;
        int member_count;
        int port, phy_port, mmu_port;

        /* Each replication group is statically allocated a region
         * in REPL_HEAD table. The size of the region depends on the
         * maximum number of valid ports. Thus, the max number of
         * replication groups is limited to number of REPL_HEAD entries
         * divided by the max number of valid ports.
         */
        si = &SOC_INFO(unit);
        member_count = 0;
        PBMP_ITER(SOC_CONTROL(unit)->repl_eligible_pbmp, port) {
            phy_port = si->port_l2p_mapping[port];
            mmu_port = si->port_p2m_mapping[phy_port];
            if ((mmu_port == 57) || (mmu_port == 59) || (mmu_port == 61) || (mmu_port == 62)) {
                /* No replication on MMU ports 57, 59, 61 and 62 */
                continue;
            }
            member_count++;
        }
        if (member_count > 0) {
            IPMC_GROUP_NUM(unit) =
                soc_mem_index_count(unit, MMU_REPL_HEAD_TBLm) / member_count;
            if (IPMC_GROUP_NUM(unit) > soc_mem_index_count(unit, L3_IPMCm)) {
                IPMC_GROUP_NUM(unit) = soc_mem_index_count(unit, L3_IPMCm);
            }
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_BRADLEY_SUPPORT
    if (SOC_REG_FIELD_VALID(unit, MC_CONTROL_5r, SHARED_TABLE_IPMC_SIZEf)) {
        int ipmc_base, ipmc_size;
        
        SOC_IF_ERROR_RETURN
            (soc_hbx_ipmc_size_get(unit, &ipmc_base, &ipmc_size));

        if (IPMC_GROUP_NUM(unit) > ipmc_size) {
            /* Reduce to fix allocated table space */
            IPMC_GROUP_NUM(unit) = ipmc_size;
        }
    }
#endif

    info->ipmc_count = 0;

    info->ipmc_group_info =
        sal_alloc(IPMC_GROUP_NUM(unit) * sizeof(_bcm_esw_ipmc_group_info_t),
                  "IPMC group info");
    if (info->ipmc_group_info == NULL) {
        return (BCM_E_MEMORY);
    }
    sal_memset(info->ipmc_group_info, 0, 
               IPMC_GROUP_NUM(unit) * sizeof(_bcm_esw_ipmc_group_info_t));

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_pim_bidir)) {
        rv = bcm_td2_ipmc_pim_bidir_init(unit);
        if (BCM_FAILURE(rv)) {
            sal_free(info->ipmc_group_info);
            info->ipmc_group_info = NULL;
            return rv;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Initialize EGR_IPMC entries to have L3_PAYLOAD == 1 */
    sal_memset(&egr_entry, 0, sizeof(egr_entry));
    if (soc_mem_field_valid(unit, EGR_IPMCm, L3_PAYLOADf)) {
        soc_EGR_IPMCm_field32_set(unit, &egr_entry, L3_PAYLOADf, 0);
    }
    if (soc_mem_field_valid(unit, EGR_IPMCm, REPLICATION_TYPEf)) {
        soc_EGR_IPMCm_field32_set(unit, &egr_entry, REPLICATION_TYPEf, 0);
    }
    if (soc_mem_field_valid(unit, EGR_IPMCm, DONT_PRUNE_VLANf)) {
        soc_EGR_IPMCm_field32_set(unit, &egr_entry, DONT_PRUNE_VLANf, 0);
    }

    for (i = 0; i < IPMC_GROUP_NUM(unit); i++) {
#if defined(BCM_TRIUMPH2_SUPPORT)
        if (SOC_MEM_IS_VALID(unit, L3_IPMC_REMAPm)) {
            /* Initialize identity mapping */
            sal_memset(&remap_entry, 0, sizeof(remap_entry));
            soc_L3_IPMC_REMAPm_field32_set(unit, &remap_entry, L3MC_INDEXf, i);
            rv = WRITE_L3_IPMC_REMAPm(unit, MEM_BLOCK_ALL, i, &remap_entry);
            if (rv < 0) {
                sal_free(info->ipmc_group_info);
                info->ipmc_group_info = NULL;
                return rv;
            }
        }
#endif
        if (SOC_MEM_IS_VALID(unit, EGR_IPMCm)) {
            rv = WRITE_EGR_IPMCm(unit, MEM_BLOCK_ALL, i, &egr_entry);
            if (rv < 0) {
                sal_free(info->ipmc_group_info);
                info->ipmc_group_info = NULL;
                return rv;
            }
        }
    }
    info->ipmc_initialized = TRUE;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_ipmc_detach
 * Purpose:
 *      Detach the IPMC module.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_detach(int unit)
{
    _bcm_esw_ipmc_t    *info = IPMC_INFO(unit);
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry; 
    int i, rv;

    if (info->ipmc_initialized) {
        /* Delete all IPMC entries. BCM_E_NOT_FOUND may be
         * returned by bcm_tr_ipmc_delete_all since L3 module
         * clears L3 tables during initialization.
         */
        rv = bcm_tr_ipmc_delete_all(unit);
        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
            return rv;
        }

        BCM_IF_ERROR_RETURN(_tr_ipmc_enable(unit, FALSE));

        if (info->ipmc_group_info != NULL) {
            for (i = 0; i < IPMC_GROUP_NUM(unit); i++) {
                ipmc_l3entry = IPMC_GROUP_INFO(unit, i)->l3entry_list;
                while (ipmc_l3entry != NULL) {
                    IPMC_GROUP_INFO(unit, i)->l3entry_list = ipmc_l3entry->next;
                    sal_free(ipmc_l3entry);
                    ipmc_l3entry = IPMC_GROUP_INFO(unit, i)->l3entry_list;
                }
            }
            sal_free(info->ipmc_group_info);
            info->ipmc_group_info = NULL;
        }

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_pim_bidir)) {
            BCM_IF_ERROR_RETURN(bcm_td2_ipmc_pim_bidir_detach(unit));
        }
#endif /* BCM_TRIDENT2_SUPPORT */

        info->ipmc_initialized = FALSE;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_ipmc_get
 * Purpose:
 *      Get an IPMC entry by index.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      index - The index number.
 *      ipmc - (OUT) IPMC entry information.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_get(int unit, int index, bcm_ipmc_addr_t *ipmc)
{
    ipmc_entry_t        ipmc_entry;
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    ipmc_1_entry_t      ipmc_1_entry;
#endif

    IPMC_INIT(unit);
    IPMC_ID(unit, index);

    if (IPMC_USED_ISSET(unit, index)) {
        BCM_IF_ERROR_RETURN
            (soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, index, &ipmc_entry));
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
        if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_read(unit, L3_IPMC_1m, MEM_BLOCK_ANY, index, 
                              &ipmc_1_entry));
            BCM_IF_ERROR_RETURN
                (_tr2_ipmc_glp_get(unit, ipmc, &ipmc_1_entry));
        }
#endif
        BCM_IF_ERROR_RETURN
            (_tr_ipmc_info_get(unit, index, ipmc, &ipmc_entry, 1, NULL));

        ipmc->group = index;

        return BCM_E_NONE;
    } 

    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm_tr_ipmc_lookup
 * Purpose:
 *      Look up an IPMC entry by sip, mcip and vid
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      index - (OUT) The index number.
 *      ipmc - (IN, OUT) IPMC entry information.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_lookup(int unit, int *index, bcm_ipmc_addr_t *ipmc)
{
    ipmc_entry_t   ipmc_entry;
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    ipmc_1_entry_t ipmc_1_entry;
#endif
    _bcm_l3_cfg_t  l3cfg;
    int            ipmc_id;

    IPMC_INIT(unit);

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
    l3cfg.l3c_vid = ipmc->vid;
    l3cfg.l3c_flags = BCM_L3_IPMC;
    l3cfg.l3c_vrf = ipmc->vrf;
    if (ipmc->flags & BCM_IPMC_HIT_CLEAR) {
        l3cfg.l3c_flags |= BCM_L3_HIT_CLEAR;
    } 
    if (ipmc->flags & BCM_IPMC_IP6) {
        sal_memcpy(l3cfg.l3c_sip6, ipmc->s_ip6_addr, BCM_IP6_ADDRLEN);
        sal_memcpy(l3cfg.l3c_ip6, ipmc->mc_ip6_addr, BCM_IP6_ADDRLEN);
        l3cfg.l3c_flags |= BCM_L3_IP6;
        ipmc->flags |= BCM_IPMC_IP6;
        BCM_IF_ERROR_RETURN(mbcm_driver[unit]->mbcm_l3_ip6_get(unit, &l3cfg));
    } else {
        l3cfg.l3c_src_ip_addr = ipmc->s_ip_addr;
        l3cfg.l3c_ipmc_group = ipmc->mc_ip_addr;
        BCM_IF_ERROR_RETURN(mbcm_driver[unit]->mbcm_l3_ip4_get(unit, &l3cfg));
        ipmc->flags &= ~BCM_IPMC_IP6;
    }

    ipmc_id = l3cfg.l3c_ipmc_ptr;
    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, ipmc_id, &ipmc_entry));

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
        BCM_IF_ERROR_RETURN
            (soc_mem_read(unit, L3_IPMC_1m, MEM_BLOCK_ANY, ipmc_id,
                          &ipmc_1_entry));
        BCM_IF_ERROR_RETURN
            (_tr2_ipmc_glp_get(unit, ipmc, &ipmc_1_entry));
    }
#endif

    BCM_IF_ERROR_RETURN
        (_tr_ipmc_info_get(unit, ipmc_id, ipmc, &ipmc_entry, 0, NULL));
    
    if (ipmc->v) {
        ipmc->group = ipmc_id;
        ipmc->lookup_class = l3cfg.l3c_lookup_class;
        ipmc->rp_id = l3cfg.l3c_rp_id;
        if (l3cfg.l3c_flags & BCM_L3_HIT) {
            ipmc->flags |= BCM_IPMC_HIT;
        }
        if (l3cfg.l3c_flags & BCM_L3_RPE) {
            ipmc->cos =  l3cfg.l3c_prio;
            ipmc->flags |= BCM_IPMC_SETPRI;
        } else {
            ipmc->cos = -1;
            ipmc->flags &= ~BCM_IPMC_SETPRI;
        }
        if (l3cfg.l3c_flags & BCM_IPMC_POST_LOOKUP_RPF_CHECK) {
            ipmc->flags |= BCM_IPMC_POST_LOOKUP_RPF_CHECK;
            ipmc->l3a_intf = l3cfg.l3c_intf;
            if (l3cfg.l3c_flags & BCM_IPMC_RPF_FAIL_DROP) {
                ipmc->flags |= BCM_IPMC_RPF_FAIL_DROP;
            }
            if (l3cfg.l3c_flags & BCM_IPMC_RPF_FAIL_DROP) {
                ipmc->flags |= BCM_IPMC_RPF_FAIL_TOCPU;
            }
        }
    }    
    if (index != NULL) {
        *index = ipmc_id;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_ipmc_add
 * Purpose:
 *      Add a new entry to the L3 table.
 * Parameters:
 *      unit - (IN) BCM device number.
 *      ipmc - (IN) IPMC entry information.
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr_ipmc_add(int unit, bcm_ipmc_addr_t *ipmc)
{
    _bcm_l3_cfg_t       l3cfg; /* L3 ipmc entry.           */
    int                 rv;    /* Operation return status. */

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));

    l3cfg.l3c_vid = ipmc->vid;
    l3cfg.l3c_flags = BCM_L3_IPMC;
    l3cfg.l3c_vrf = ipmc->vrf;
    l3cfg.l3c_lookup_class = ipmc->lookup_class;
    if (ipmc->flags & BCM_IPMC_SETPRI) {
        l3cfg.l3c_flags |= BCM_L3_RPE;
        l3cfg.l3c_prio = ipmc->cos;
    }
    if (ipmc->flags & BCM_IPMC_IP6) {
        if (!BCM_IP6_MULTICAST(ipmc->mc_ip6_addr)) {
            return BCM_E_PARAM;
        }
        sal_memcpy(l3cfg.l3c_sip6, ipmc->s_ip6_addr, BCM_IP6_ADDRLEN);
        sal_memcpy(l3cfg.l3c_ip6, ipmc->mc_ip6_addr, BCM_IP6_ADDRLEN);
        l3cfg.l3c_flags |= BCM_L3_IP6;
    } else {
        if (!BCM_IP4_MULTICAST(ipmc->mc_ip_addr)) {
            return BCM_E_PARAM;
        }
        l3cfg.l3c_src_ip_addr = ipmc->s_ip_addr;
        l3cfg.l3c_ipmc_group = ipmc->mc_ip_addr;
    }

    l3cfg.l3c_ipmc_ptr = ipmc->group;
    l3cfg.l3c_flags |=  BCM_L3_HIT;
    l3cfg.l3c_vid = ipmc->vid;
    l3cfg.l3c_rp_id = ipmc->rp_id;

    if (ipmc->flags & BCM_IPMC_REPLACE) {
        l3cfg.l3c_flags |= BCM_L3_REPLACE;
    }

    if (ipmc->flags & BCM_IPMC_POST_LOOKUP_RPF_CHECK) {
        l3cfg.l3c_intf   = ipmc->l3a_intf;
        l3cfg.l3c_flags |= BCM_IPMC_POST_LOOKUP_RPF_CHECK;
        if (ipmc->flags & BCM_IPMC_RPF_FAIL_DROP) {
            l3cfg.l3c_flags |= BCM_IPMC_RPF_FAIL_DROP;
        }
        if (ipmc->flags & BCM_IPMC_RPF_FAIL_TOCPU) {
            l3cfg.l3c_flags |= BCM_IPMC_RPF_FAIL_TOCPU;
        }
    }

    if (ipmc->flags & BCM_IPMC_IP6) {
        rv = mbcm_driver[unit]->mbcm_l3_ip6_add(unit, &l3cfg);
    } else {
        rv = mbcm_driver[unit]->mbcm_l3_ip4_add(unit, &l3cfg);
    }

    if (BCM_SUCCESS(rv)) {
        rv = _bcm_tr_ipmc_l3entry_list_add(unit, ipmc->group, 
                                           l3cfg); 
    }
    return (rv);
}

/*
 * Function:
 *      _bcm_tr_ipmc_del
 * Purpose:
 *      Remove an  entry from the L3 table.
 * Parameters:
 *      unit - (IN) BCM device number.
 *      ipmc - (IN)IPMC entry information.
 *      modify_l3entry_list - (IN) Controls whether to modify IPMC group's
 *                                 L3 entry list.
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr_ipmc_del(int unit, bcm_ipmc_addr_t *ipmc, int modify_l3entry_list)
{
    _bcm_l3_cfg_t       l3cfg; /* L3 ipmc entry.           */
    int                 rv;    /* Operation return status. */
    int                 ipmc_index = 0;

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));

    l3cfg.l3c_vid = ipmc->vid;
    l3cfg.l3c_flags = BCM_L3_IPMC;
    l3cfg.l3c_vrf = ipmc->vrf;
    l3cfg.l3c_vid = ipmc->vid;

    if (ipmc->flags & BCM_IPMC_IP6) {
        if (!BCM_IP6_MULTICAST(ipmc->mc_ip6_addr)) {
            return BCM_E_PARAM;
        }
        sal_memcpy(l3cfg.l3c_sip6, ipmc->s_ip6_addr, BCM_IP6_ADDRLEN);
        sal_memcpy(l3cfg.l3c_ip6, ipmc->mc_ip6_addr, BCM_IP6_ADDRLEN);
        l3cfg.l3c_flags |= BCM_L3_IP6;
    } else {
        if (!BCM_IP4_MULTICAST(ipmc->mc_ip_addr)) {
            return BCM_E_PARAM;
        }
        l3cfg.l3c_src_ip_addr = ipmc->s_ip_addr;
        l3cfg.l3c_ipmc_group = ipmc->mc_ip_addr;
    }

    if (ipmc->flags & BCM_IPMC_IP6) {
        rv = mbcm_driver[unit]->mbcm_l3_ip6_get(unit, &l3cfg);
        if (BCM_SUCCESS(rv)) {
            ipmc_index = l3cfg.l3c_ipmc_ptr;
            rv = mbcm_driver[unit]->mbcm_l3_ip6_delete(unit, &l3cfg);
        }
    } else {
        rv = mbcm_driver[unit]->mbcm_l3_ip4_get(unit, &l3cfg);
        if (BCM_SUCCESS(rv)) {
            ipmc_index = l3cfg.l3c_ipmc_ptr;
            rv = mbcm_driver[unit]->mbcm_l3_ip4_delete(unit, &l3cfg);
        }
    }

    if (BCM_SUCCESS(rv) && modify_l3entry_list) {
        rv = _bcm_tr_ipmc_l3entry_list_del(unit, ipmc_index, l3cfg); 
    }

    return (rv);
}

/*
 * Function:
 *      _bcm_tr_ipmc_src_port_compare
 * Purpose:
 *      Compare the IPMC source port parameters.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      ipmc_index - IPMC index to be shared.
 *      ipmc - IPMC address entry info.
 *      match - (OUT) Match indication.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr_ipmc_src_port_compare(int unit, int ipmc_index,
        bcm_ipmc_addr_t *ipmc, int *match)
{
    ipmc_entry_t entry;
    ipmc_1_entry_t entry_1;
    int no_src_check, is_trunk, tgid, mod, port;
    int t_f, tgid_f, mod_f, port_f;

    if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
        BCM_IF_ERROR_RETURN
            (soc_mem_read(unit, L3_IPMC_1m, MEM_BLOCK_ANY, ipmc_index,
                          &entry_1));
    } else {
        BCM_IF_ERROR_RETURN
            (soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, ipmc_index,
                          &entry));
    }

    no_src_check = FALSE;
    is_trunk = 0;
    tgid = -1;
    mod = -1;
    port = -1;
    if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
        t_f    = soc_L3_IPMC_1m_field32_get(unit, &entry_1, Tf);
        tgid_f = soc_L3_IPMC_1m_field32_get(unit, &entry_1, TGIDf);
        mod_f  = soc_L3_IPMC_1m_field32_get(unit, &entry_1, MODULE_IDf);
        port_f = soc_L3_IPMC_1m_field32_get(unit, &entry_1, PORT_NUMf);
    } else {
        t_f    = soc_L3_IPMCm_field32_get(unit, &entry, Tf);
        tgid_f = soc_L3_IPMCm_field32_get(unit, &entry, TGIDf);
        mod_f  = soc_L3_IPMCm_field32_get(unit, &entry, MODULE_IDf);
        port_f = soc_L3_IPMCm_field32_get(unit, &entry, PORT_NUMf);
    }

    if ((t_f == 1) && (mod_f == SOC_MODID_MAX(unit)) &&
            (port_f == TR_IPMC_NO_SRC_CHECK_PORT(unit))) {
        no_src_check = TRUE;
    } else if (t_f == 1) {
        is_trunk = 1;
        tgid = tgid_f;
    } else {
        mod = mod_f;
        port = port_f;
    }

    *match = FALSE;
    if ((ipmc->flags & BCM_IPMC_SOURCE_PORT_NOCHECK) ||
            (ipmc->port_tgid < 0)) {                        /* no source port */
        if (no_src_check) {
            *match = TRUE;
        }
    } else if (ipmc->ts) {                              /* trunk source port */
        if (is_trunk && (tgid == ipmc->port_tgid)) {
            *match = TRUE;
        }
    } else {                                            /* source port */
        bcm_module_t    mod_in, mod_out;
        bcm_port_t      port_in, port_out;

        mod_in = ipmc->mod_id;
        port_in = ipmc->port_tgid;
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET,
                                     mod_in, port_in,
                                     &mod_out, &port_out));
        /* Check parameters, since above is an application callback */
        if (!SOC_MODID_ADDRESSABLE(unit, mod_out)) {
            return BCM_E_BADID;
        }
        if (!SOC_PORT_ADDRESSABLE(unit, port_out)) {
            return BCM_E_PORT;
        }

        if ((mod == mod_out) && (port == port_out)) {
            *match = TRUE;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_ipmc_src_port_consistency_check
 * Purpose:
 *      When multiple IPMC address entries share the same IPMC
 *      index, they must also have the same port parameter
 *      for the purpose of IPMC source port checking. This
 *      procedure makes sure this is the case.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      ipmc_index - IPMC index to be shared.
 *      ipmc - IPMC address entry info.
 *      already_used - Indicates if the given IPMC address entry is
 *                     already using the ipmc index to be shared.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr_ipmc_src_port_consistency_check(int unit, int ipmc_index,
        bcm_ipmc_addr_t *ipmc, int already_used)
{
    int l3entry_list_size;
    int match;

    BCM_IF_ERROR_RETURN(_bcm_tr_ipmc_l3entry_list_size_get(unit,
                ipmc_index, &l3entry_list_size));
    if ((already_used && (l3entry_list_size > 1)) ||
            (!already_used && (l3entry_list_size > 0))) {
        /* If there are IPMC address entries other than the given IPMC address
         * entry that are pointing the given ipmc index, verify that the IPMC
         * source port check parameters are the same. If not, the ipmc index
         * cannot be shared.
         */ 
        BCM_IF_ERROR_RETURN(_bcm_tr_ipmc_src_port_compare(unit,
                    ipmc_index, ipmc, &match));
        if (!match) {
            return BCM_E_PARAM;
        } 
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_ipmc_add
 * Purpose:
 *      Add a new entry to the IPMC table.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      ipmc - IPMC entry information.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_add(int unit, bcm_ipmc_addr_t *ipmc)
{

    bcm_ipmc_addr_t ipmc_lookup_data;
    int             old_ipmc_index;
    int new_entry, rv;
#ifdef BCM_TRIDENT2_SUPPORT
    int             old_rp_id;
#endif /* BCM_TRIDENT2_SUPPORT */

    IPMC_INIT(unit);

    /* Check if IPMC entry already exists */
    ipmc_lookup_data = *ipmc;
    rv = bcm_tr_ipmc_lookup(unit, &old_ipmc_index, &ipmc_lookup_data);
#ifdef BCM_TRIDENT2_SUPPORT
    old_rp_id = ipmc_lookup_data.rp_id;
#endif /* BCM_TRIDENT2_SUPPORT */
    if (BCM_SUCCESS(rv)) {
        if (!(ipmc->flags & BCM_IPMC_REPLACE)) {
           return (BCM_E_EXISTS);
        } else {
            new_entry = FALSE;
        }
    } else {
        /* Return if error occured. */
        if (rv != BCM_E_NOT_FOUND) {
            return (rv);
        }
        new_entry = TRUE;
    }

    if (new_entry) { 
        BCM_IF_ERROR_RETURN(_bcm_tr_ipmc_src_port_consistency_check(unit,
                    ipmc->group, ipmc, 0));

        /* Increment the reference count of the given ipmc_index. */
        BCM_IF_ERROR_RETURN(bcm_xgs3_ipmc_id_alloc(unit, ipmc->group));
    } else {
        if (ipmc->group != old_ipmc_index) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr_ipmc_src_port_consistency_check(unit,
                                                         ipmc->group,
                                                         ipmc, 0));
            /* Increment the reference count of the given ipmc_index. */
            BCM_IF_ERROR_RETURN(bcm_xgs3_ipmc_id_alloc(unit, ipmc->group));

            /* Decrement the reference count of the old ipmc_index. */
            BCM_IF_ERROR_RETURN(bcm_xgs3_ipmc_id_free(unit, old_ipmc_index));
            if (!IPMC_USED_ISSET(unit, old_ipmc_index)) {
                /* Reference count should not be zero yet. */
                return BCM_E_INTERNAL;
            }
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_tr_ipmc_src_port_consistency_check(unit,
                                                         ipmc->group,
                                                         ipmc, 1));
        }
    }

    ipmc->v = (ipmc->flags & BCM_IPMC_ADD_DISABLED) ? 0 : 1;

    if (new_entry) {
        /* Write L3_IPMC table entry. */
        rv = _tr_ipmc_write(unit, ipmc->group, ipmc);
        if (BCM_FAILURE(rv)) {
            bcm_xgs3_ipmc_id_free(unit, ipmc->group);
            return (rv);
        }

        /* Add new L3 table entry */
        rv = _bcm_tr_ipmc_add(unit, ipmc);
        if (BCM_FAILURE(rv)) {
            bcm_xgs3_ipmc_id_free(unit, ipmc->group);
            if (!IPMC_USED_ISSET(unit, ipmc->group)) {
                /* Reference count should not be zero yet. */
                return BCM_E_INTERNAL;
            }
            return (rv);
        }
    } else {
        /* Update existing IPMC entry */
        rv = bcm_tr_ipmc_put(unit, ipmc->group, ipmc);
        if (BCM_FAILURE(rv)) {
            _bcm_tr_ipmc_del(unit, ipmc, TRUE);
            bcm_xgs3_ipmc_id_free(unit, ipmc->group);
            return rv;
        }
    }

#ifdef BCM_TRIDENT2_SUPPORT
    /* Update reference count of rendezvous point */
    if (soc_feature(unit, soc_feature_pim_bidir)) {
        if (new_entry) {
            if (ipmc->rp_id != BCM_IPMC_RP_ID_INVALID) {
                BCM_IF_ERROR_RETURN
                    (bcm_td2_ipmc_rp_ref_count_incr(unit, ipmc->rp_id));
            }
        } else {
            if (old_rp_id != ipmc->rp_id) {
                if (ipmc->rp_id != BCM_IPMC_RP_ID_INVALID) {
                    BCM_IF_ERROR_RETURN
                        (bcm_td2_ipmc_rp_ref_count_incr(unit, ipmc->rp_id));
                }
                if (old_rp_id != BCM_IPMC_RP_ID_INVALID) {
                    BCM_IF_ERROR_RETURN
                        (bcm_td2_ipmc_rp_ref_count_decr(unit, old_rp_id));
                }
            }
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_ipmc_put
 * Purpose:
 *      Overwrite an entry in the IPMC table.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      index - Table index to overwrite.
 *      data - IPMC entry information.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_put(int unit, int index, bcm_ipmc_addr_t *ipmc)
{
    _bcm_l3_cfg_t   l3cfg;
    int old_ipmc_index;

    IPMC_INIT(unit);
    IPMC_ID(unit, index);

    BCM_IF_ERROR_RETURN(_tr_ipmc_write(unit, index, ipmc));

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));

    l3cfg.l3c_vid = ipmc->vid;
    l3cfg.l3c_flags = BCM_L3_IPMC;
    l3cfg.l3c_vrf = ipmc->vrf;
    if (ipmc->flags & BCM_IPMC_IP6) {
        sal_memcpy(l3cfg.l3c_sip6, ipmc->s_ip6_addr, BCM_IP6_ADDRLEN);
        sal_memcpy(l3cfg.l3c_ip6, ipmc->mc_ip6_addr, BCM_IP6_ADDRLEN);
        l3cfg.l3c_flags |= BCM_L3_IP6;
        BCM_IF_ERROR_RETURN
            (mbcm_driver[unit]->mbcm_l3_ip6_get(unit, &l3cfg));
    } else {
        l3cfg.l3c_src_ip_addr = ipmc->s_ip_addr;
        l3cfg.l3c_ipmc_group = ipmc->mc_ip_addr;
        BCM_IF_ERROR_RETURN
            (mbcm_driver[unit]->mbcm_l3_ip4_get(unit, &l3cfg));
    }
    old_ipmc_index = l3cfg.l3c_ipmc_ptr;

    if (!(ipmc->flags & BCM_IPMC_SETPRI)) {
        l3cfg.l3c_flags &= ~BCM_L3_RPE;
        l3cfg.l3c_prio = 0;
    } else {
        l3cfg.l3c_flags |= BCM_L3_RPE;
        l3cfg.l3c_prio = ipmc->cos;
    }
    l3cfg.l3c_lookup_class = ipmc->lookup_class;
    l3cfg.l3c_ipmc_ptr = ipmc->group;
    l3cfg.l3c_rp_id = ipmc->rp_id;

    if (ipmc->flags & BCM_IPMC_POST_LOOKUP_RPF_CHECK) {
        l3cfg.l3c_intf = ipmc->l3a_intf;
        l3cfg.l3c_flags |= BCM_IPMC_POST_LOOKUP_RPF_CHECK;
        if (ipmc->flags & BCM_IPMC_RPF_FAIL_DROP) {
            l3cfg.l3c_flags |= BCM_IPMC_RPF_FAIL_DROP;
        }
        if (ipmc->flags & BCM_IPMC_RPF_FAIL_TOCPU) {
            l3cfg.l3c_flags |= BCM_IPMC_RPF_FAIL_TOCPU;
        }
    } 

    BCM_IF_ERROR_RETURN
        (bcm_xgs3_l3_replace(unit, &l3cfg));

    if (old_ipmc_index != ipmc->group) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr_ipmc_l3entry_list_add(unit, ipmc->group,
                                           l3cfg)); 
        BCM_IF_ERROR_RETURN
            (_bcm_tr_ipmc_l3entry_list_del(unit, old_ipmc_index,
                                           l3cfg)); 
    } else {
            BCM_IF_ERROR_RETURN
                (_bcm_tr_ipmc_l3entry_list_update(unit, old_ipmc_index,
                                                  l3cfg));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_ipmc_delete
 * Purpose:
 *      Delete an entry from the IPMC table.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      data - IPMC entry information.
 *      modify_l3entry_list - Control whether to modify IPMC group's linked
 *                            list of L3 entries.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If BCM_IPMC_KEEP_ENTRY is true, the entry valid bit is cleared
 *      but the entry is not deleted from the table.
 */

STATIC int
_bcm_tr_ipmc_delete(int unit, bcm_ipmc_addr_t *ipmc, int modify_l3entry_list)
{
    _bcm_l3_cfg_t       l3cfg;
    int                 ipmc_id;
#ifdef BCM_TRIDENT2_SUPPORT
    int                 rp_id;
#endif /* BCM_TRIDENT2_SUPPORT */

    IPMC_INIT(unit);

    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
    l3cfg.l3c_vid = ipmc->vid;
    l3cfg.l3c_flags = BCM_L3_IPMC;
    l3cfg.l3c_vrf = ipmc->vrf;
    if (ipmc->flags & BCM_IPMC_IP6) {
        sal_memcpy(l3cfg.l3c_sip6, ipmc->s_ip6_addr, BCM_IP6_ADDRLEN);
        sal_memcpy(l3cfg.l3c_ip6, ipmc->mc_ip6_addr, BCM_IP6_ADDRLEN);
        l3cfg.l3c_flags |= BCM_L3_IP6;
        BCM_IF_ERROR_RETURN(
            mbcm_driver[unit]->mbcm_l3_ip6_get(unit, &l3cfg));
    } else {
        l3cfg.l3c_src_ip_addr = ipmc->s_ip_addr;
        l3cfg.l3c_ipmc_group = ipmc->mc_ip_addr;
        BCM_IF_ERROR_RETURN(
            mbcm_driver[unit]->mbcm_l3_ip4_get(unit, &l3cfg));
    }
    ipmc_id = l3cfg.l3c_ipmc_ptr;
#ifdef BCM_TRIDENT2_SUPPORT
    rp_id = l3cfg.l3c_rp_id;
#endif /* BCM_TRIDENT2_SUPPORT */

    if (!(ipmc->flags & BCM_IPMC_KEEP_ENTRY)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr_ipmc_del(unit, ipmc, modify_l3entry_list));

        bcm_xgs3_ipmc_id_free(unit, ipmc_id);
        if (!IPMC_USED_ISSET(unit, ipmc_id)) {
            /* Reference count should not be zero yet. */
            return BCM_E_INTERNAL;
        }

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_pim_bidir)) {
            if (rp_id != BCM_IPMC_RP_ID_INVALID) {
                BCM_IF_ERROR_RETURN
                    (bcm_td2_ipmc_rp_ref_count_decr(unit, rp_id));
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_ipmc_delete
 * Purpose:
 *      Delete an entry from the IPMC table.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      data - IPMC entry information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If BCM_IPMC_KEEP_ENTRY is true, the entry valid bit is cleared
 *      but the entry is not deleted from the table.
 */

int
bcm_tr_ipmc_delete(int unit, bcm_ipmc_addr_t *ipmc)
{
    return _bcm_tr_ipmc_delete(unit, ipmc, TRUE);
}

/*
 * Function:
 *      bcm_tr_ipmc_delete_all
 * Purpose:
 *      Delete all entries from the IPMC table.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_delete_all(int unit)
{
    int i, rv = BCM_E_NONE;
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;
    _bcm_l3_cfg_t l3cfg;

    IPMC_INIT(unit);

    IPMC_LOCK(unit);
    for (i = 0; i < IPMC_GROUP_NUM(unit); i++) {
        if (IPMC_USED_ISSET(unit, i)) {
            ipmc_l3entry = IPMC_GROUP_INFO(unit, i)->l3entry_list;
            while (ipmc_l3entry != NULL) {
                sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
                l3cfg.l3c_vid = ipmc_l3entry->l3info.vid;
                l3cfg.l3c_flags = BCM_L3_IPMC;
                l3cfg.l3c_vrf = ipmc_l3entry->l3info.vrf;
                if (ipmc_l3entry->l3info.flags & BCM_L3_IP6) {
                    sal_memcpy(l3cfg.l3c_sip6, ipmc_l3entry->l3info.sip6, BCM_IP6_ADDRLEN);
                    sal_memcpy(l3cfg.l3c_ip6, ipmc_l3entry->l3info.ip6, BCM_IP6_ADDRLEN);
                    l3cfg.l3c_flags |= BCM_L3_IP6;
                } else {
                    l3cfg.l3c_src_ip_addr = ipmc_l3entry->l3info.src_ip_addr;
                    l3cfg.l3c_ipmc_group = ipmc_l3entry->l3info.ip_addr;
                }
                rv = bcm_xgs3_l3_del(unit, &l3cfg);
                if (rv < 0) {
                    IPMC_UNLOCK(unit);
                    return rv;
                }
#ifdef BCM_TRIDENT2_SUPPORT
                if (soc_feature(unit, soc_feature_pim_bidir)) {
                    if (ipmc_l3entry->l3info.rp_id != BCM_IPMC_RP_ID_INVALID) {
                        rv = bcm_td2_ipmc_rp_ref_count_decr(unit,
                                ipmc_l3entry->l3info.rp_id);
                        if (rv < 0) {
                            IPMC_UNLOCK(unit);
                            return rv;
                        }
                    }
                }
#endif /* BCM_TRIDENT2_SUPPORT */
                IPMC_GROUP_INFO(unit, i)->l3entry_list = ipmc_l3entry->next;
                sal_free(ipmc_l3entry);
                ipmc_l3entry = IPMC_GROUP_INFO(unit, i)->l3entry_list;
            }

            /* For IPMC groups that have non-zero reference counts,
             * decrease reference count to 1, meaning the IPMC group
             * is only referenced by bcm_multicast_egress APIs.
             */
            IPMC_USED_ONE(unit, i);
        }
    }

    IPMC_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      bcm_tr_ipmc_age
 * Purpose:
 *      Age out the ipmc entry by clearing the HIT bit when appropriate,
 *      the ipmc entry itself is removed if HIT bit is not set.
 * Parameters:
 *      unit       -  (IN) BCM device number.
 *      flags      -  (IN) The criteria used to age out ipmc table.
 *                         IPv6/IPv4
 *      age_cb     -  (IN) Call back routine.
 *      user_data  -  (IN) User provided cookie for callback.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr_ipmc_age(int unit, uint32 flags, bcm_ipmc_traverse_cb age_cb,
                void *user_data)
{
    int idx;                   /* Ipmc table iteration index. */
    bcm_ipmc_addr_t entry;     /* Ipmc entry iterator.        */
    int rv = BCM_E_NONE;       /* Operation return status.    */

    ipmc_entry_t   ipmc_entry;
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    ipmc_1_entry_t ipmc_1_entry;
#endif
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;
    _bcm_esw_ipmc_l3entry_t *prev_ipmc_l3entry = NULL;

    IPMC_INIT(unit);
    IPMC_LOCK(unit);

    for (idx = 0; idx < IPMC_GROUP_NUM(unit); idx++) {
        if (IPMC_USED_ISSET(unit, idx)) {

            rv = soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, idx, &ipmc_entry);
            if (BCM_FAILURE(rv)) {
                goto age_done;
            }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
            if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
                rv = soc_mem_read(unit, L3_IPMC_1m, MEM_BLOCK_ANY, idx, 
                                  &ipmc_1_entry);
                if (BCM_FAILURE(rv)) {
                    goto age_done;
                }
            }
#endif /* BCM_TRIUMPH2_SUPPORT */

            ipmc_l3entry = IPMC_GROUP_INFO(unit, idx)->l3entry_list;
            while (ipmc_l3entry != NULL) {
                sal_memset(&entry, 0, sizeof(bcm_ipmc_addr_t));
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
                if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
                    rv = _tr2_ipmc_glp_get(unit, &entry, &ipmc_1_entry);
                    if (BCM_FAILURE(rv)) {
                        goto age_done;
                    }
                }
#endif /* BCM_TRIUMPH2_SUPPORT */
                rv = _tr_ipmc_info_get(unit, idx, &entry, &ipmc_entry, 1, ipmc_l3entry);
                if (BCM_FAILURE(rv)) {
                    goto age_done;
                }

                /* Make sure update only ipv4 or ipv6 entries. */
                if ((flags & BCM_IPMC_IP6) != (entry.flags & BCM_IPMC_IP6)) {
                    prev_ipmc_l3entry = ipmc_l3entry;
                    ipmc_l3entry = ipmc_l3entry->next;
                    continue;
                }

                if (entry.flags & BCM_IPMC_HIT) {
                    _bcm_l3_cfg_t l3cfg;
                    /* Clear hit bit on used entry (by doing a lookup !!) */
                    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
                    l3cfg.l3c_flags |= BCM_L3_HIT_CLEAR;
                    l3cfg.l3c_vid = ipmc_l3entry->l3info.vid;
                    l3cfg.l3c_flags |= BCM_L3_IPMC;
                    l3cfg.l3c_vrf = ipmc_l3entry->l3info.vrf;
                    l3cfg.l3c_vid = ipmc_l3entry->l3info.vid;

                    if (ipmc_l3entry->ip6) {
                        sal_memcpy(l3cfg.l3c_sip6, &ipmc_l3entry->l3info.sip6, 
                                   BCM_IP6_ADDRLEN);
                        sal_memcpy(l3cfg.l3c_ip6, &ipmc_l3entry->l3info.ip6, 
                                   BCM_IP6_ADDRLEN);
                        l3cfg.l3c_flags |= BCM_L3_IP6;
                        rv = (mbcm_driver[unit]->mbcm_l3_ip6_get(unit, &l3cfg));
                    } else {
                        l3cfg.l3c_src_ip_addr = ipmc_l3entry->l3info.src_ip_addr;
                        l3cfg.l3c_ipmc_group = ipmc_l3entry->l3info.ipmc_group;
                        rv = (mbcm_driver[unit]->mbcm_l3_ip4_get(unit, &l3cfg));
                    }
                    if (BCM_FAILURE(rv)) {
                        goto age_done;
                    }
                    prev_ipmc_l3entry = ipmc_l3entry;
                    ipmc_l3entry = ipmc_l3entry->next;
                } else {
                    /* Delete IPMC L3 entry. Inhibit modification of IPMC group's
                     * l3entry_list by _bcm_tr_ipmc_delete.
                     */
                    rv = _bcm_tr_ipmc_delete(unit, &entry, FALSE);
                    if (BCM_FAILURE(rv)) {
                        goto age_done;
                    }

                    /* Delete from IPMC group's l3entry_list */
                    if (ipmc_l3entry == IPMC_GROUP_INFO(unit, idx)->l3entry_list) {
                        IPMC_GROUP_INFO(unit, idx)->l3entry_list = ipmc_l3entry->next;
                        sal_free(ipmc_l3entry);
                        ipmc_l3entry = IPMC_GROUP_INFO(unit, idx)->l3entry_list;
                    } else {
                        /* 
                         * In the following line of code, Coverity thinks the
                         * prev_ipmc_l3entry pointer may still be NULL when 
                         * dereferenced. This situation will never occur because 
                         * if ipmc_l3entry is not pointing to the head of the 
                         * linked list, prev_ipmc_l3entry would not be NULL.
                         */
                        /* coverity[var_deref_op : FALSE] */
                        prev_ipmc_l3entry->next = ipmc_l3entry->next;
                        sal_free(ipmc_l3entry);
                        ipmc_l3entry = prev_ipmc_l3entry->next;
                    }

                    /* Invoke user callback. */
                    if (NULL != age_cb) {
                        _BCM_MULTICAST_GROUP_SET(entry.group,
                                _BCM_MULTICAST_TYPE_L3, entry.group);
                        rv = (*age_cb)(unit, &entry, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
                        if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                            goto age_done;
                        }
#endif
                    }
                }
            }
        }
    }

age_done:
    IPMC_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_tr_ipmc_traverse
 * Purpose:
 *      Go through all valid ipmc entries, and call the callback function
 *      at each entry
 * Parameters:
 *      unit      - (IN) BCM device number.
 *      flags     - (IN) The criteria used to age out ipmc table.
 *      cb        - (IN) User supplied callback function.
 *      user_data - (IN) User supplied cookie used in parameter
 *                       in callback function.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr_ipmc_traverse(int unit, uint32 flags, bcm_ipmc_traverse_cb cb,
                     void *user_data)
{
    int idx;                   /* Ipmc table iteration index. */
    bcm_ipmc_addr_t entry;     /* Ipmc entry iterator.        */
    int rv = BCM_E_NONE;       /* Operation return status.    */

    ipmc_entry_t   ipmc_entry;
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    ipmc_1_entry_t ipmc_1_entry;
#endif
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;

    IPMC_INIT(unit);
    IPMC_LOCK(unit);

    for (idx = 0; idx < IPMC_GROUP_NUM(unit); idx++) {
        if (IPMC_USED_ISSET(unit, idx)) {

            rv = soc_mem_read(unit, L3_IPMCm, MEM_BLOCK_ANY, idx, &ipmc_entry);
            if (BCM_FAILURE(rv)) {
                goto traverse_done;
            }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
            if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
                rv = soc_mem_read(unit, L3_IPMC_1m, MEM_BLOCK_ANY, idx, 
                        &ipmc_1_entry);
                if (BCM_FAILURE(rv)) {
                    goto traverse_done;
                }
            }
#endif /* BCM_TRIUMPH2_SUPPORT */

            ipmc_l3entry = IPMC_GROUP_INFO(unit, idx)->l3entry_list;
            while (ipmc_l3entry != NULL) {
                sal_memset(&entry, 0, sizeof(bcm_ipmc_addr_t));
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
                if (SOC_MEM_IS_VALID(unit, L3_IPMC_1m)) {
                    rv = _tr2_ipmc_glp_get(unit, &entry, &ipmc_1_entry);
                    if (BCM_FAILURE(rv)) {
                        goto traverse_done;
                    }
                }
#endif /* BCM_TRIUMPH2_SUPPORT */
                rv = _tr_ipmc_info_get(unit, idx, &entry, &ipmc_entry, 1, ipmc_l3entry);
                if (BCM_FAILURE(rv)) {
                    goto traverse_done;
                }

                /* Make sure update only ipv4 or ipv6 entries. */
                if ((flags & BCM_IPMC_IP6) != (entry.flags & BCM_IPMC_IP6)) {
                    ipmc_l3entry = ipmc_l3entry->next;
                    continue;
                }

                /* Get the pointer to next ipmc_l3entry before callback,
                 * since callback may delete the current ipmc_l3entry.
                 */
                ipmc_l3entry = ipmc_l3entry->next;

                /* Invoke user callback. */
                _BCM_MULTICAST_GROUP_SET(entry.group,
                                _BCM_MULTICAST_TYPE_L3, entry.group);
                rv = (*cb)(unit, &entry, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
                if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                    goto traverse_done;
                }
#endif
            }
        }
    }

traverse_done:
    IPMC_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_tr_ipmc_enable
 * Purpose:
 *      Enable or disable IPMC chip functions.
 * Parameters:
 *      unit - Unit number
 *      enable - TRUE to enable; FALSE to disable
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_enable(int unit, int enable)
{
    IPMC_INIT(unit);

    return _tr_ipmc_enable(unit, enable);
}


/*
 * Function:
 *      bcm_tr_ipmc_src_port_check
 * Purpose:
 *      Enable or disable Source Port checking in IPMC lookups.
 * Parameters:
 *      unit - Unit number
 *      enable - TRUE to enable; FALSE to disable
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_src_port_check(int unit, int enable)
{
    IPMC_INIT(unit);

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_tr_ipmc_src_ip_search
 * Purpose:
 *      Enable or disable Source IP significance in IPMC lookups.
 * Parameters:
 *      unit - Unit number
 *      enable - TRUE to enable; FALSE to disable
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_src_ip_search(int unit, int enable)
{
    IPMC_INIT(unit);

    if (enable) {
        return BCM_E_NONE;  /* always on */
    } else {
        return BCM_E_FAIL;  /* cannot be disabled */
    }
}

/*
 * Function:
 *      bcm_tr_ipmc_egress_port_set
 * Purpose:
 *      Configure the IP Multicast egress properties
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - port to config.
 *      mac  - MAC address.
 *      untag - 1: The IP multicast packet is transmitted as untagged packet.
 *              0: The IP multicast packet is transmitted as tagged packet
 *              with VLAN tag vid.
 *      vid  - VLAN ID.
 *      ttl  - 1 to disable the TTL decrement, 0 otherwise.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_egress_port_set(int unit, bcm_port_t port,
                            const bcm_mac_t mac, int untag,
                            bcm_vlan_t vid, int ttl)
{
    uint32      cfg2;

#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
        SOC_PBMP_MEMBER(SOC_INFO(unit).linkphy_pp_port_pbm, port)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
        SOC_PBMP_MEMBER(SOC_INFO(unit).subtag_pp_port_pbm, port))) {
    } else
#endif
    if (!IS_PORT(unit, port)) {
        return BCM_E_BADID;
    }

    if (!SOC_PBMP_PORT_VALID(port)) {
        return BCM_E_BADID;
    }

    SOC_IF_ERROR_RETURN(READ_EGR_IPMC_CFG2r(unit, port, &cfg2));

    soc_reg_field_set(unit, EGR_IPMC_CFG2r, &cfg2,
                      UNTAGf, untag ? 1 : 0);
    soc_reg_field_set(unit, EGR_IPMC_CFG2r, &cfg2,
                      VIDf, vid);

    
    SOC_IF_ERROR_RETURN(WRITE_EGR_IPMC_CFG2r(unit, port, cfg2));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_ipmc_egress_port_get
 * Purpose:
 *      Return the IP Multicast egress properties
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - port to config.
 *      mac - (OUT) MAC address.
 *      untag - (OUT) 1: The IP multicast packet is transmitted as
 *                       untagged packet.
 *                    0: The IP multicast packet is transmitted as tagged
 *                       packet with VLAN tag vid.
 *      vid - (OUT) VLAN ID.
 *      ttl_thresh - (OUT) Drop IPMC packets if TTL <= ttl_thresh.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr_ipmc_egress_port_get(int unit, bcm_port_t port, sal_mac_addr_t mac,
                            int *untag, bcm_vlan_t *vid, int *ttl_thresh)
{
    uint32              cfg2;
#if defined(BCM_KATANA2_SUPPORT)
    if (IS_LP_PORT(unit, port) || IS_SUBTAG_PORT(unit, port)) {
    } else
#endif
    if (!SOC_PBMP_PORT_VALID(port) || !IS_PORT(unit, port)) {
        return BCM_E_BADID;
    }

    SOC_IF_ERROR_RETURN(READ_EGR_IPMC_CFG2r(unit, port, &cfg2));

    *untag = soc_reg_field_get(unit, EGR_IPMC_CFG2r, cfg2, UNTAGf);
    *vid = soc_reg_field_get(unit, EGR_IPMC_CFG2r, cfg2, VIDf);
    *ttl_thresh = -1;

    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Recover IPMC group count, IPMC group info, and PIM-BIDIR
 * rendezvous point reference count by traversing IPMC address table
 */
STATIC int
_bcm_tr_ipmc_table_recover(int unit)
{
    int rv = BCM_E_NONE;
    soc_mem_t v4mc_mem, v6mc_mem;
    int v4mc_entry_size, v6mc_entry_size;
    int v4mc_key_type, v6mc_key_type;
    soc_field_t l3mc_index_field, rpe_field;
    soc_field_t vrf_field, group_ip_field, source_ip_field;
    soc_field_t l3_iif_field, pri_field, class_id_field, rpa_id_field;
    soc_field_t source_ip_hi_field, source_ip_lo_field;
    int chunk_size, num_chunks, chunk_index;
    int entry_index_min, entry_index_max;
    uint8 *v4mc_buf = NULL;
    uint8 *v6mc_buf = NULL;
    int i;
    uint32 *v4mc_entry;
    uint32 *v6mc_entry;
    int ipmc_ptr;
    _bcm_l3_cfg_t l3cfg;

    if (SOC_MEM_IS_VALID(unit, L3_ENTRY_2m)) {
        v4mc_mem = L3_ENTRY_2m;
        v4mc_entry_size = sizeof(l3_entry_2_entry_t);
        v4mc_key_type = 6;
        l3mc_index_field = IPV4MC__L3MC_INDEXf;
        rpe_field = IPV4MC__RPEf;
        vrf_field = IPV4MC__VRF_IDf;
        group_ip_field = IPV4MC__GROUP_IP_ADDRf;
        source_ip_field = IPV4MC__SOURCE_IP_ADDRf;
        l3_iif_field = IPV4MC__L3_IIFf;
        pri_field = IPV4MC__PRIf;
        class_id_field = IPV4MC__CLASS_IDf;
        rpa_id_field = INVALIDf;
    } else {
        v4mc_mem = L3_ENTRY_IPV4_MULTICASTm;
        v4mc_entry_size = sizeof(l3_entry_ipv4_multicast_entry_t);
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TRIDENT2(unit)) {
            v4mc_key_type = TD2_L3_HASH_KEY_TYPE_V4MC;
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        {
            v4mc_key_type = TR_L3_HASH_KEY_TYPE_V4MC;
        }
        l3mc_index_field = L3MC_INDEXf;
        rpe_field = RPEf;
        vrf_field = VRF_IDf;
        group_ip_field = GROUP_IP_ADDRf;
        source_ip_field = SOURCE_IP_ADDRf;
        l3_iif_field = L3_IIFf;
        pri_field = PRIf;
        class_id_field = CLASS_IDf;
        rpa_id_field = RPA_IDf;
    }

    /* Traverse IPv4 multicast table in chunks to avoid BCM_E_MEMORY */
    chunk_size = 1024;
    num_chunks = (soc_mem_index_count(unit, v4mc_mem) + chunk_size - 1) /
                 chunk_size;
    v4mc_buf = soc_cm_salloc(unit, v4mc_entry_size * chunk_size,
            "v4mc buf dma");
    if (v4mc_buf == NULL) {
        return BCM_E_MEMORY;
    }
    for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {
        /* Read a chunk of entries */
        entry_index_min = chunk_index * chunk_size;
        entry_index_max = entry_index_min + chunk_size - 1;
        if (entry_index_max > soc_mem_index_max(unit, v4mc_mem)) {
            entry_index_max = soc_mem_index_max(unit, v4mc_mem);
        }
        rv = soc_mem_read_range(unit, v4mc_mem, MEM_BLOCK_ANY,
                entry_index_min, entry_index_max, v4mc_buf);
        if (SOC_FAILURE(rv)) {
            soc_cm_sfree(unit, v4mc_buf);
            return rv;
        }

        /* Read each entry of the chunk */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            v4mc_entry = soc_mem_table_idx_to_pointer(unit, v4mc_mem,
                    uint32 *, v4mc_buf, i);

            if (v4mc_key_type != soc_mem_field32_get(unit, v4mc_mem,
                        v4mc_entry, KEY_TYPE_0f)) {
                continue;
            }
            if (v4mc_key_type != soc_mem_field32_get(unit, v4mc_mem,
                        v4mc_entry, KEY_TYPE_1f)) {
                continue;
            }
            if (0 == soc_mem_field32_get(unit, v4mc_mem, v4mc_entry,
                        VALID_0f)) {
                continue;
            }
            if (0 == soc_mem_field32_get(unit, v4mc_mem, v4mc_entry,
                        VALID_1f)) {
                continue;
            }

            /* Update IPMC group count and IPMC group reference count */
            ipmc_ptr = soc_mem_field32_get(unit, v4mc_mem, v4mc_entry,
                    l3mc_index_field);
            IPMC_USED_SET(unit, ipmc_ptr);

            /* Insert into IPMC group's linked list of L3 entries */
            sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
            l3cfg.l3c_flags |= BCM_L3_IPMC;
            l3cfg.l3c_flags |=  BCM_L3_HIT;
            if (soc_mem_field32_get(unit, v4mc_mem, v4mc_entry, rpe_field)) {
                l3cfg.l3c_flags |=  BCM_L3_RPE;
            }
            l3cfg.l3c_vrf = soc_mem_field32_get(unit, v4mc_mem,
                    v4mc_entry, vrf_field);
            l3cfg.l3c_ipmc_group = soc_mem_field32_get(unit, v4mc_mem,
                    v4mc_entry, group_ip_field);
            l3cfg.l3c_src_ip_addr = soc_mem_field32_get(unit, v4mc_mem,
                    v4mc_entry, source_ip_field);
            l3cfg.l3c_vid = soc_mem_field32_get(unit, v4mc_mem,
                    v4mc_entry, l3_iif_field);
            l3cfg.l3c_prio = soc_mem_field32_get(unit, v4mc_mem,
                    v4mc_entry, pri_field);
            l3cfg.l3c_ipmc_ptr = soc_mem_field32_get(unit, v4mc_mem,
                    v4mc_entry, l3mc_index_field);
            l3cfg.l3c_lookup_class = soc_mem_field32_get(unit, v4mc_mem,
                    v4mc_entry, class_id_field);
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_mem_field_valid(unit, L3_ENTRY_IPV4_MULTICASTm, RPA_IDf)) {
                l3cfg.l3c_rp_id = soc_mem_field32_get(unit, v4mc_mem,
                        v4mc_entry, rpa_id_field);
                if (l3cfg.l3c_rp_id == 0 && (l3cfg.l3c_vid != 0 ||
                            soc_mem_field32_get(unit, v4mc_mem, v4mc_entry,
                                EXPECTED_L3_IIFf) != 0)) {
                    /* If RPA_ID field value is 0, it could mean (1) this IPMC
                     * entry is not a PIM-BIDIR entry, or (2) this entry is a
                     * PIM-BIDIR entry and is associated with rendezvous point
                     * 0. If either the L3 IIF in the key or the expected L3
                     * IIF in the data is not zero, this entry is not a
                     * PIM-BIDIR entry.
                     */ 
                    l3cfg.l3c_rp_id = BCM_IPMC_RP_ID_INVALID;
                } else {
                    /* Recover rendezvous point's reference count */
                    rv = bcm_td2_ipmc_rp_ref_count_recover(unit,
                            l3cfg.l3c_rp_id);
                    if (BCM_FAILURE(rv)) {
                        soc_cm_sfree(unit, v4mc_buf);
                        return rv;
                    }
                }
            }
#endif /* BCM_TRIDENT2_SUPPORT */
            rv = _bcm_tr_ipmc_l3entry_list_add(unit, ipmc_ptr, l3cfg);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, v4mc_buf);
                return rv;
            }
        }
    }
    soc_cm_sfree(unit, v4mc_buf);

    /*
     * Deal with IPv6 multicast table now
     */
    if (SOC_MEM_IS_VALID(unit, L3_ENTRY_4m)) {
        v6mc_mem = L3_ENTRY_4m;
        v6mc_entry_size = sizeof(l3_entry_4_entry_t);
        v6mc_key_type = 7;
        l3mc_index_field = IPV6MC__L3MC_INDEXf;
        rpe_field = IPV6MC__RPEf;
        vrf_field = IPV6MC__VRF_IDf;
        source_ip_hi_field = IPV6MC__SOURCE_IP_ADDR_UPR_64f;
        source_ip_lo_field = IPV6MC__SOURCE_IP_ADDR_LWR_64f;
        l3_iif_field = IPV6MC__L3_IIFf;
        pri_field = IPV6MC__PRIf;
        class_id_field = IPV6MC__CLASS_IDf;
        rpa_id_field = INVALIDf;
    } else {
        v6mc_mem = L3_ENTRY_IPV6_MULTICASTm;
        v6mc_entry_size = sizeof(l3_entry_ipv6_multicast_entry_t);
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TRIDENT2(unit)) {
            v6mc_key_type = TD2_L3_HASH_KEY_TYPE_V6MC;
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        {
            v6mc_key_type = TR_L3_HASH_KEY_TYPE_V6MC;
        }
        l3mc_index_field = L3MC_INDEXf;
        rpe_field = RPEf;
        vrf_field = VRF_IDf;
        source_ip_hi_field = SOURCE_IP_ADDR_UPR_64f;
        source_ip_lo_field = SOURCE_IP_ADDR_LWR_64f;
        l3_iif_field = L3_IIFf;
        pri_field = PRIf;
        class_id_field = CLASS_IDf;
        rpa_id_field = RPA_IDf;
    }

    /* Traverse IPv6 multicast table in chunks to avoid BCM_E_MEMORY */
    chunk_size = 1024;
    num_chunks = (soc_mem_index_count(unit, v6mc_mem) + chunk_size - 1) /
                 chunk_size;
    v6mc_buf = soc_cm_salloc(unit, v6mc_entry_size * chunk_size,
            "v6mc buf dma");
    if (v6mc_buf == NULL) {
        return BCM_E_MEMORY;
    }
    for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {
        /* Read a chunk of entries */
        entry_index_min = chunk_index * chunk_size;
        entry_index_max = entry_index_min + chunk_size - 1;
        if (entry_index_max > soc_mem_index_max(unit, v6mc_mem)) {
            entry_index_max = soc_mem_index_max(unit, v6mc_mem);
        }
        rv = soc_mem_read_range(unit, v6mc_mem, MEM_BLOCK_ANY,
                entry_index_min, entry_index_max, v6mc_buf);
        if (SOC_FAILURE(rv)) {
            soc_cm_sfree(unit, v6mc_buf);
            return rv;
        }

        /* Read each entry of the chunk */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            v6mc_entry = soc_mem_table_idx_to_pointer(unit, v6mc_mem,
                    uint32 *, v6mc_buf, i);

            if (v6mc_key_type != soc_mem_field32_get(unit, v6mc_mem,
                        v6mc_entry, KEY_TYPE_0f)) {
                continue;
            }
            if (v6mc_key_type != soc_mem_field32_get(unit, v6mc_mem,
                        v6mc_entry, KEY_TYPE_1f)) {
                continue;
            }
            if (v6mc_key_type != soc_mem_field32_get(unit, v6mc_mem,
                        v6mc_entry, KEY_TYPE_2f)) {
                continue;
            }
            if (v6mc_key_type != soc_mem_field32_get(unit, v6mc_mem,
                        v6mc_entry, KEY_TYPE_3f)) {
                continue;
            }
            if (0 == soc_mem_field32_get(unit, v6mc_mem, v6mc_entry,
                        VALID_0f)) {
                continue;
            }
            if (0 == soc_mem_field32_get(unit, v6mc_mem, v6mc_entry,
                        VALID_1f)) {
                continue;
            }
            if (0 == soc_mem_field32_get(unit, v6mc_mem, v6mc_entry,
                        VALID_2f)) {
                continue;
            }
            if (0 == soc_mem_field32_get(unit, v6mc_mem, v6mc_entry,
                        VALID_3f)) {
                continue;
            }

            /* Update IPMC group count and IPMC group reference count */
            ipmc_ptr = soc_mem_field32_get(unit, v6mc_mem, v6mc_entry,
                    l3mc_index_field);
            IPMC_USED_SET(unit, ipmc_ptr);

            /* Insert into IPMC group's linked list of L3 entries */
            sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
            l3cfg.l3c_flags |= BCM_IPMC_IP6;
            l3cfg.l3c_flags |= BCM_L3_IPMC;
            l3cfg.l3c_flags |=  BCM_L3_HIT;
            if (soc_mem_field32_get(unit, v6mc_mem, v6mc_entry, rpe_field)) {
                l3cfg.l3c_flags |= BCM_L3_RPE;
            }
            l3cfg.l3c_vrf = soc_mem_field32_get(unit, v6mc_mem,
                    v6mc_entry, vrf_field);
            if (SOC_MEM_IS_VALID(unit, L3_ENTRY_4m)) {
                soc_mem_ip6_addr_get(unit, v6mc_mem, v6mc_entry, 
                        IPV6MC__GROUP_IP_ADDR_LWR_96f, l3cfg.l3c_ip6, 
                        SOC_MEM_IP6_LOWER_96BIT);
                soc_mem_ip6_addr_get(unit, v6mc_mem, v6mc_entry, 
                        IPV6MC__GROUP_IP_ADDR_UPR_24f, l3cfg.l3c_ip6, 
                        SOC_MEM_IP6_BITS_119_96);
            } else {
                soc_mem_ip6_addr_get(unit, v6mc_mem, v6mc_entry, 
                        GROUP_IP_ADDR_LWR_64f, l3cfg.l3c_ip6, 
                        SOC_MEM_IP6_LOWER_ONLY);
                soc_mem_ip6_addr_get(unit, v6mc_mem, v6mc_entry, 
                        GROUP_IP_ADDR_UPR_56f, l3cfg.l3c_ip6, 
                        SOC_MEM_IP6_UPPER_ONLY);
            }
            l3cfg.l3c_ip6[0] = 0xff;    /* Set entry to multicast*/ 
            soc_mem_ip6_addr_get(unit, v6mc_mem, v6mc_entry, 
                    source_ip_lo_field, l3cfg.l3c_sip6, 
                    SOC_MEM_IP6_LOWER_ONLY);
            soc_mem_ip6_addr_get(unit, v6mc_mem, v6mc_entry, 
                    source_ip_hi_field, l3cfg.l3c_sip6, 
                    SOC_MEM_IP6_UPPER_ONLY);
            l3cfg.l3c_vid = soc_mem_field32_get(unit, v6mc_mem,
                    v6mc_entry, l3_iif_field);
            l3cfg.l3c_prio = soc_mem_field32_get(unit, v6mc_mem,
                    v6mc_entry, pri_field);
            l3cfg.l3c_ipmc_ptr = soc_mem_field32_get(unit, v6mc_mem,
                    v6mc_entry, l3mc_index_field);
            l3cfg.l3c_lookup_class = soc_mem_field32_get(unit, v6mc_mem,
                    v6mc_entry, class_id_field);
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_mem_field_valid(unit, L3_ENTRY_IPV6_MULTICASTm, RPA_IDf)) {
                l3cfg.l3c_rp_id = soc_mem_field32_get(unit, v6mc_mem,
                        v6mc_entry, rpa_id_field);
                if (l3cfg.l3c_rp_id == 0 && (l3cfg.l3c_vid != 0 ||
                            soc_mem_field32_get(unit, v6mc_mem, v6mc_entry,
                                EXPECTED_L3_IIFf) != 0)) {
                    /* If RPA_ID field value is 0, it could mean (1) this IPMC
                     * entry is not a PIM-BIDIR entry, or (2) this entry is a
                     * PIM-BIDIR entry and is associated with rendezvous point
                     * 0. If either the L3 IIF in the key or the expected L3
                     * IIF in the data is not zero, this entry is not a
                     * PIM-BIDIR entry.
                     */ 
                    l3cfg.l3c_rp_id = BCM_IPMC_RP_ID_INVALID;
                } else {
                    /* Update rendezvous point's reference count */
                    rv = bcm_td2_ipmc_rp_ref_count_recover(unit,
                            l3cfg.l3c_rp_id);
                    if (BCM_FAILURE(rv)) {
                        soc_cm_sfree(unit, v6mc_buf);
                        return rv;
                    }
                }
            }
#endif /* BCM_TRIDENT2_SUPPORT */
            rv = _bcm_tr_ipmc_l3entry_list_add(unit, ipmc_ptr, l3cfg);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, v6mc_buf);
                return rv;
            }
        }
    }
    soc_cm_sfree(unit, v6mc_buf);

    return rv;
}

/*
 * Reload IPMC state on TR class of devices
 */
int
_bcm_tr_ipmc_reinit(int unit)
{
    _bcm_esw_ipmc_t *info = IPMC_INFO(unit);
    int rv = BCM_E_NONE;
    int i;
    uint8 flags;
    int l3_min, l3_max, l3_ipmc_tbl_sz;
    uint8 *l3_ipmc_table = NULL;
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;
    ipmc_entry_t *l3_ipmc_entry;

    info->ipmc_group_info = NULL;

    IPMC_GROUP_NUM(unit) = soc_mem_index_count(unit, L3_IPMCm);
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
        soc_info_t *si;
        int member_count;
        int port, phy_port, mmu_port;

        /* Each replication group is statically allocated a region
         * in REPL_HEAD table. The size of the region depends on the
         * maximum number of valid ports. Thus, the max number of
         * replication groups is limited to number of REPL_HEAD entries
         * divided by the max number of valid ports.
         */
        si = &SOC_INFO(unit);
        member_count = 0;
        PBMP_ITER(SOC_CONTROL(unit)->repl_eligible_pbmp, port) {
            phy_port = si->port_l2p_mapping[port];
            mmu_port = si->port_p2m_mapping[phy_port];
            if ((mmu_port == 57) || (mmu_port == 59) || (mmu_port == 61) || (mmu_port == 62)) {
                /* No replication on MMU ports 57, 59, 61 and 62 */
                continue;
            }
            member_count++;
        }
        if (member_count > 0) {
            IPMC_GROUP_NUM(unit) =
                soc_mem_index_count(unit, MMU_REPL_HEAD_TBLm) / member_count;
            if (IPMC_GROUP_NUM(unit) > soc_mem_index_count(unit, L3_IPMCm)) {
                IPMC_GROUP_NUM(unit) = soc_mem_index_count(unit, L3_IPMCm);
            }
        }
    } 
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_BRADLEY_SUPPORT
    if (SOC_REG_FIELD_VALID(unit, MC_CONTROL_5r, SHARED_TABLE_IPMC_SIZEf)) {
        int ipmc_base, ipmc_size;
        
        SOC_IF_ERROR_RETURN
            (soc_hbx_ipmc_size_get(unit, &ipmc_base, &ipmc_size));

        if (IPMC_GROUP_NUM(unit) > ipmc_size) {
            /* Reduce to fix allocated table space */
            IPMC_GROUP_NUM(unit) = ipmc_size;
        }
    }
#endif /* BCM_BRADLEY_SUPPORT */

    info->ipmc_count = 0;

    info->ipmc_group_info =
        sal_alloc(IPMC_GROUP_NUM(unit) * sizeof(_bcm_esw_ipmc_group_info_t),
                  "IPMC group info");
    if (info->ipmc_group_info == NULL) {
        rv = BCM_E_MEMORY;
        goto ret_err;
    }
    sal_memset(info->ipmc_group_info, 0, 
               IPMC_GROUP_NUM(unit) * sizeof(_bcm_esw_ipmc_group_info_t));

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_pim_bidir)) {
        rv = bcm_td2_ipmc_pim_bidir_init(unit);
        if (BCM_FAILURE(rv)) {
            goto ret_err;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Traverse IPMC address table to recover IPMC group count,
     * IPMC group info, and PIM-BIDIR rendezvous point reference count.
     */
    rv = _bcm_tr_ipmc_table_recover(unit);
    if (BCM_FAILURE(rv)) {
        goto ret_err;
    }

    /* Recover multicast mode from HW cache */
    rv = _bcm_esw_ipmc_repl_wb_flags_get(unit,
                                         _BCM_IPMC_WB_MULTICAST_MODE,
                                         &flags);
    if (flags) {
        /*
         * Increase reference count for all defined mulitcast groups now
         */
        l3_min = soc_mem_index_min(unit, L3_IPMCm);
        l3_max = soc_mem_index_max(unit, L3_IPMCm);
        l3_ipmc_tbl_sz = sizeof(ipmc_entry_t) * \
            (l3_max - l3_min + 1);
        l3_ipmc_table = soc_cm_salloc(unit, l3_ipmc_tbl_sz,
                                      "L3 ipmc tbl dma");
        if (l3_ipmc_table == NULL) {
            rv = BCM_E_MEMORY;
            goto ret_err;
        }
        if ((rv = soc_mem_read_range(unit, L3_IPMCm,
                                     MEM_BLOCK_ANY,
                                     l3_min, l3_max, l3_ipmc_table)) < 0) {
            soc_cm_sfree(unit, l3_ipmc_table);
            goto ret_err;
        }

        for (i = l3_min; i <= l3_max; i++) {
            l3_ipmc_entry = soc_mem_table_idx_to_pointer(unit,
                                                         L3_IPMCm,
                                                         ipmc_entry_t *,
                                                         l3_ipmc_table, i);
            if (0 == soc_L3_IPMCm_field32_get(unit, l3_ipmc_entry,
                                              VALIDf)) {
                continue;
            }
        
            /* It's a multicast group we need to note. */
            IPMC_USED_SET(unit, i);
        }
        soc_cm_sfree(unit, l3_ipmc_table);
    }

    /*
     * Recover replication state
     */
#if defined(BCM_TRIUMPH3_SUPPORT) 
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
        rv = _bcm_tr3_ipmc_repl_reload(unit);
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
 if (SOC_IS_KATANA2(unit)) {
        rv = _bcm_kt2_ipmc_repl_reload(unit);
    } else
#endif /* BCM_KATANA2_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit) || SOC_IS_TRIDENT(unit) ||
            SOC_IS_KATANA(unit)) {
        rv = _bcm_tr2_ipmc_repl_reload(unit);
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
    {
        rv = _bcm_xgs3_ipmc_repl_reload(unit);
    }

ret_err:
    if (BCM_FAILURE(rv)) {

        if (info->ipmc_group_info != NULL) {
            for (i = 0; i < IPMC_GROUP_NUM(unit); i++) {
                ipmc_l3entry = IPMC_GROUP_INFO(unit, i)->l3entry_list;
                while (ipmc_l3entry != NULL) {
                    IPMC_GROUP_INFO(unit, i)->l3entry_list = ipmc_l3entry->next;
                    sal_free(ipmc_l3entry);
                    ipmc_l3entry = IPMC_GROUP_INFO(unit, i)->l3entry_list;
                }
            }
            sal_free(info->ipmc_group_info);
            info->ipmc_group_info = NULL;
        }
    } else {
        info->ipmc_initialized = TRUE;
    }

    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_tr_ipmc_sw_dump
 * Purpose:
 *     Displays IPMC information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_tr_ipmc_sw_dump(int unit)
{
    int                   i, j;
    _bcm_esw_ipmc_t      *ipmc_info;
    _bcm_esw_ipmc_l3entry_t *ipmc_l3entry;

    /*
     * xgs3_ipmc_info
     */

    ipmc_info = IPMC_INFO(unit);

    soc_cm_print("  XGS3 IPMC Info -\n");
    soc_cm_print("    Init        : %d\n", ipmc_info->ipmc_initialized);
    soc_cm_print("    Size        : %d\n", IPMC_GROUP_NUM(unit));
    soc_cm_print("    Count       : %d\n", ipmc_info->ipmc_count);

    soc_cm_print("    Alloc index :");
    if (ipmc_info->ipmc_group_info != NULL) {
        for (i = 0, j = 0; i < IPMC_GROUP_NUM(unit); i++) {
            /* If not set, skip print */
            if (!IPMC_USED_ISSET(unit, i)) {
                continue;
            }
            if (!(j % 10)) {
                soc_cm_print("\n    ");
            }
            soc_cm_print("  %5d", i);
            j++;
        }
    }
    soc_cm_print("\n");

    soc_cm_print("    Reference count (index:value) :");
    if (ipmc_info->ipmc_group_info != NULL) {
        for (i = 0, j = 0; i < IPMC_GROUP_NUM(unit); i++) {
            if (!IPMC_USED_ISSET(unit, i)) {
                continue;
            }
            if (!(j % 4)) {
                soc_cm_print("\n    ");
            }
            soc_cm_print("  %5d:%-5d", i, IPMC_GROUP_INFO(unit, i)->ref_count);
            j++;
        }
    }
    soc_cm_print("\n");

    soc_cm_print("    IP6 (index:value) :");
    if (ipmc_info->ipmc_group_info != NULL) {
        for (i = 0, j = 0; i < IPMC_GROUP_NUM(unit); i++) {
            ipmc_l3entry = IPMC_GROUP_INFO(unit, i)->l3entry_list;
            while (ipmc_l3entry != NULL) {
                if (ipmc_l3entry->ip6 == 1) {
                    if (!(j % 4)) {
                        soc_cm_print("\n    ");
                    }
                    soc_cm_print("  %5d:%-5d", i, ipmc_l3entry->ip6);
                    j++;
                }
                ipmc_l3entry = ipmc_l3entry->next;
            }
        }
    }
    soc_cm_print("\n");

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_pim_bidir)) {
        _bcm_td2_ipmc_pim_bidir_sw_dump(unit);
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* IPMC replication info is elsewhere */
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit) || SOC_IS_KATANA2(unit)) {
        _bcm_tr3_ipmc_repl_sw_dump(unit);
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit) || SOC_IS_TRIDENT(unit) ||
            SOC_IS_KATANA(unit)) {
        _bcm_tr2_ipmc_repl_sw_dump(unit);
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
    {
        _bcm_xgs3_ipmc_repl_sw_dump(unit);
    }

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif  /* INCLUDE_L3 */

int _bcm_tr_firebolt_ipmc_not_empty;
