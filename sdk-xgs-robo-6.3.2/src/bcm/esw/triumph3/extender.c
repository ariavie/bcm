/*
 * $Id: extender.c 1.11 Broadcom SDK $
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
 * File:    extender.c
 * Purpose: Implements Port Extension APIs for Triumph3.
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)

#include <soc/mem.h>
#include <soc/hash.h>
#include <soc/l2x.h>

#include <bcm/error.h>

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph3.h>

/*
 * Software book keeping for Port Extension information
 */
typedef struct _bcm_tr3_extender_port_info_s {
    uint32 flags;
    bcm_gport_t port;
    uint16 extended_port_vid;
    bcm_extender_pcp_de_select_t pcp_de_select;
    uint8 pcp;
    uint8 de;
    bcm_vlan_t match_vlan;
} _bcm_tr3_extender_port_info_t;

typedef struct _bcm_tr3_extender_bookkeeping_s {
    _bcm_tr3_extender_port_info_t *port_info; /* VP state */
} _bcm_tr3_extender_bookkeeping_t;

STATIC _bcm_tr3_extender_bookkeeping_t _bcm_tr3_extender_bk_info[BCM_MAX_NUM_UNITS];

#define EXTENDER_INFO(unit) (&_bcm_tr3_extender_bk_info[unit])
#define EXTENDER_PORT_INFO(unit, vp) (&EXTENDER_INFO(unit)->port_info[vp])

/*
 * Function:
 *      _bcm_tr3_extender_port_cnt_update
 * Purpose:
 *      Update port's VP count.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      gport - (IN) GPORT ID.
 *      vp    - (IN) Virtual port number.
 *      incr  - (IN) If TRUE, increment VP count, else decrease VP count.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr3_extender_port_cnt_update(int unit, bcm_gport_t gport,
        int vp, int incr)
{
    int mod_out, port_out, tgid_out, id_out;
    bcm_port_t local_member_array[SOC_MAX_NUM_PORTS];
    int local_member_count;
    int idx;
    int mod_local;
    _bcm_port_info_t *port_info;

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, gport, &mod_out, 
                                &port_out, &tgid_out, &id_out));
    if (-1 != id_out) {
        return BCM_E_PARAM;
    }

    /* Update the physical port's SW state. If associated with a trunk,
     * update each local physical port's SW state.
     */

    if (BCM_TRUNK_INVALID != tgid_out) {

        BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid_out, 
                    SOC_MAX_NUM_PORTS, local_member_array, &local_member_count));

        for (idx = 0; idx < local_member_count; idx++) {
            _bcm_port_info_access(unit, local_member_array[idx], &port_info);
            if (incr) {
                port_info->vp_count++;
            } else {
                port_info->vp_count--;
            }
        }
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &mod_local));
        if (mod_local) {
            if (soc_feature(unit, soc_feature_sysport_remap)) { 
                BCM_XLATE_SYSPORT_S2P(unit, &port_out); 
            }
            _bcm_port_info_access(unit, port_out, &port_info);
            if (incr) {
                port_info->vp_count++;
            } else {
                port_info->vp_count--;
            }
        }
    }

    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_tr3_extender_reinit
 * Purpose:
 *      Warm boot recovery for the Extender module
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_tr3_extender_reinit(int unit)
{
    int rv = BCM_E_NONE;
    int stable_size;
    uint8 *ing_nh_buf = NULL;
    ing_l3_next_hop_entry_t *ing_nh_entry;
    uint8 *egr_nh_buf = NULL;
    egr_l3_next_hop_entry_t *egr_nh_entry;
    int i, index_min, index_max;
    uint32 entry_type, vp, trunk_bit;
    bcm_trunk_t tgid;
    bcm_module_t modid, mod_out;
    bcm_port_t port_num, port_out;
    int lower_th, higher_th;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* Recover extender virtual ports from Ingress and
     * Egress Next Hop tables
     */
    ing_nh_buf = soc_cm_salloc(unit,
            SOC_MEM_TABLE_BYTES(unit, ING_L3_NEXT_HOPm),
            "Ing Next Hop buffer");
    if (NULL == ing_nh_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    index_min = soc_mem_index_min(unit, ING_L3_NEXT_HOPm);
    index_max = soc_mem_index_max(unit, ING_L3_NEXT_HOPm);
    rv = soc_mem_read_range(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
            index_min, index_max, ing_nh_buf);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    egr_nh_buf = soc_cm_salloc(unit,
            SOC_MEM_TABLE_BYTES(unit, EGR_L3_NEXT_HOPm),
            "Egr Next Hop buffer");
    if (NULL == egr_nh_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    index_min = soc_mem_index_min(unit, EGR_L3_NEXT_HOPm);
    index_max = soc_mem_index_max(unit, EGR_L3_NEXT_HOPm);
    rv = soc_mem_read_range(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY,
            index_min, index_max, egr_nh_buf);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    /* Get multicast E-VID range */
    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchExtenderMulticastHigherThreshold, &higher_th));

    for (i = index_min; i <= index_max; i++) {
        egr_nh_entry = soc_mem_table_idx_to_pointer
            (unit, EGR_L3_NEXT_HOPm, egr_l3_next_hop_entry_t *, 
             egr_nh_buf, i);

        /* Check entry type */
        entry_type = soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                ENTRY_TYPEf);
        if (entry_type != 2) {
            /* Not extender virtual port entry type */
            continue;
        }

        /* Check that ETAG action is ADD */
        if (2 != soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                    SD_TAG__VNTAG_ACTIONSf)) {
            continue;
        }

        /* Recover VP */
        vp = soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                SD_TAG__DVPf);
        if ((stable_size == 0) || SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
            rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeExtender);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        } else {
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                /* VP bitmap is recovered by virtual_init */
                continue;
            }
        }

        /* Recover ETAG VID */
        EXTENDER_PORT_INFO(unit, vp)->extended_port_vid =
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                SD_TAG__VNTAG_DST_VIFf);

        /* Recover physical trunk or port */
        ing_nh_entry = soc_mem_table_idx_to_pointer
            (unit, ING_L3_NEXT_HOPm, ing_l3_next_hop_entry_t *, 
             ing_nh_buf, i);
        trunk_bit = soc_ING_L3_NEXT_HOPm_field32_get(unit, ing_nh_entry,
                Tf);
        if (trunk_bit) {
            tgid = soc_ING_L3_NEXT_HOPm_field32_get(unit, ing_nh_entry,
                    TGIDf);
            BCM_GPORT_TRUNK_SET(EXTENDER_PORT_INFO(unit, vp)->port, tgid);
        } else {
            modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, ing_nh_entry,
                    MODULE_IDf);
            port_num = soc_ING_L3_NEXT_HOPm_field32_get(unit, ing_nh_entry,
                    PORT_NUMf);
            rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                    modid, port_num, &mod_out, &port_out);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            BCM_GPORT_MODPORT_SET(EXTENDER_PORT_INFO(unit, vp)->port,
                    mod_out, port_out);
        }

        /* Recover multicast flag */
        if (EXTENDER_PORT_INFO(unit, vp)->extended_port_vid >= lower_th &&
            EXTENDER_PORT_INFO(unit, vp)->extended_port_vid <= higher_th) {
            EXTENDER_PORT_INFO(unit, vp)->flags |= BCM_EXTENDER_PORT_MULTICAST;
        }

        /* Recover ETAG PCP and DE info */
        switch (soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                    SD_TAG__ETAG_PCP_DE_SOURCEf)) {
            case 2:
                EXTENDER_PORT_INFO(unit, vp)->pcp_de_select =
                    BCM_EXTENDER_PCP_DE_SELECT_DEFAULT;
                EXTENDER_PORT_INFO(unit, vp)->pcp = 
                    soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                            SD_TAG__ETAG_PCPf);
                EXTENDER_PORT_INFO(unit, vp)->de = 
                    soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                            SD_TAG__ETAG_DEf);
                break;
            case 3:
                EXTENDER_PORT_INFO(unit, vp)->pcp_de_select =
                    BCM_EXTENDER_PCP_DE_SELECT_PHB;
                break;
            default:
                rv = BCM_E_INTERNAL;
                goto cleanup;
        }

        /* Recover match_vlan */
        EXTENDER_PORT_INFO(unit, vp)->match_vlan =
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                    SD_TAG__SD_TAG_VIDf);

        if (stable_size == 0) {
            /* In the Port module, a port's VP count is not recovered in 
             * level 1 Warm Boot.
             */
            rv = _bcm_tr3_extender_port_cnt_update(unit,
                    EXTENDER_PORT_INFO(unit, vp)->port, vp, TRUE);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    }

cleanup:
    if (ing_nh_buf) {
        soc_cm_sfree(unit, ing_nh_buf);
    }
    if (egr_nh_buf) {
        soc_cm_sfree(unit, egr_nh_buf);
    }

    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      _bcm_tr3_extender_free_resources
 * Purpose:
 *      Free all allocated tables and memory
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */
STATIC void
_bcm_tr3_extender_free_resources(int unit)
{
    if (EXTENDER_INFO(unit)->port_info) {
        sal_free(EXTENDER_INFO(unit)->port_info);
        EXTENDER_INFO(unit)->port_info = NULL;
    }
}

/*
 * Function:
 *      bcm_tr3_extender_init
 * Purpose:
 *      Initialize the Extender module.
 * Parameters:
 *      unit - SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_extender_init(int unit)
{
    int num_vp;
    int rv = BCM_E_NONE;

    sal_memset(EXTENDER_INFO(unit), 0,
            sizeof(_bcm_tr3_extender_bookkeeping_t));

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    if (NULL == EXTENDER_INFO(unit)->port_info) {
        EXTENDER_INFO(unit)->port_info = sal_alloc
            (sizeof(_bcm_tr3_extender_port_info_t) * num_vp,
             "extender_port_info");
        if (NULL == EXTENDER_INFO(unit)->port_info) {
            _bcm_tr3_extender_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(EXTENDER_INFO(unit)->port_info, 0,
            sizeof(_bcm_tr3_extender_port_info_t) * num_vp);

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        /* Warm Boot recovery */
        rv = _bcm_tr3_extender_reinit(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_tr3_extender_free_resources(unit);
        }
    } else
#endif /* BCM_WARM_BOOT_SUPPORT */
    {
        /* Initialize ETAG VID multicast range */
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_set(unit,
                    bcmSwitchExtenderMulticastLowerThreshold, 0x1000));
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_set(unit,
                    bcmSwitchExtenderMulticastHigherThreshold, 0x3fff));
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_extender_cleanup
 * Purpose:
 *      Detach the Extender module.
 * Parameters:
 *      unit - SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_extender_cleanup(int unit)
{
    _bcm_tr3_extender_free_resources(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_extender_nh_info_set
 * Purpose:
 *      Get a next hop index and configure next hop tables.
 * Parameters:
 *      unit       - (IN) SOC unit number. 
 *      extender_port  - (IN) Pointer to Extender port structure. 
 *      vp         - (IN) Virtual port number. 
 *      drop       - (IN) Drop indication. 
 *      nh_index   - (IN/OUT) Next hop index. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr3_extender_nh_info_set(int unit, bcm_extender_port_t *extender_port,
        int vp, int drop, int *nh_index)
{
    int rv;
    uint32 nh_flags;
    bcm_l3_egress_t nh_info;
    egr_l3_next_hop_entry_t egr_nh;
    uint8 egr_nh_entry_type;
    int etag_dot1p_mapping_ptr;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int id;
    int ing_nh_port;
    int ing_nh_module;
    int ing_nh_trunk;
    ing_l3_next_hop_entry_t ing_nh;
    initial_ing_l3_next_hop_entry_t initial_ing_nh;

    /* Get a next hop index */

    if (extender_port->flags & BCM_EXTENDER_PORT_REPLACE) {
        if ((*nh_index > soc_mem_index_max(unit, EGR_L3_NEXT_HOPm)) ||
                (*nh_index < soc_mem_index_min(unit, EGR_L3_NEXT_HOPm)))  {
            return BCM_E_PARAM;
        }
    } else {
        /*
         * Allocate a next-hop entry. By calling bcm_xgs3_nh_add()
         * with _BCM_L3_SHR_WRITE_DISABLE flag, a next-hop index is
         * allocated but nothing is written to hardware. The "nh_info"
         * in this case is not used, so just set to all zeros.
         */
        bcm_l3_egress_t_init(&nh_info);

        nh_flags = _BCM_L3_SHR_MATCH_DISABLE | _BCM_L3_SHR_WRITE_DISABLE;
        rv = bcm_xgs3_nh_add(unit, nh_flags, &nh_info, nh_index);
        BCM_IF_ERROR_RETURN(rv);
    }

    /* Write EGR_L3_NEXT_HOP entry */

    if (extender_port->flags & BCM_EXTENDER_PORT_REPLACE) {
        /* Read the existing egress next_hop entry */
        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                *nh_index, &egr_nh);
        BCM_IF_ERROR_RETURN(rv);

        /* Be sure that the existing entry is programmed to SD-tag */
        egr_nh_entry_type = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);
        if (egr_nh_entry_type != 0x2) { /* != SD-tag */
            return BCM_E_PARAM;
        }

        /* Get ETAG priority mapping profile pointer */
        etag_dot1p_mapping_ptr = soc_EGR_L3_NEXT_HOPm_field32_get(unit,
                &egr_nh, SD_TAG__ETAG_DOT1P_MAPPING_PTRf);
    } else {
        egr_nh_entry_type = 0x2; /* SD-tag */
        etag_dot1p_mapping_ptr = 0;
    }

    sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
            ENTRY_TYPEf, egr_nh_entry_type);
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
            SD_TAG__DVPf, vp);
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
            SD_TAG__HG_HDR_SELf, 1);
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
            SD_TAG__VNTAG_DST_VIFf, extender_port->extended_port_vid);
    if (extender_port->pcp_de_select == BCM_EXTENDER_PCP_DE_SELECT_DEFAULT) {
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                SD_TAG__ETAG_PCP_DE_SOURCEf, 2);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                SD_TAG__ETAG_PCPf, extender_port->pcp);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                SD_TAG__ETAG_DEf, extender_port->de);
    } else if (extender_port->pcp_de_select == BCM_EXTENDER_PCP_DE_SELECT_PHB) {
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                SD_TAG__ETAG_PCP_DE_SOURCEf, 3);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                SD_TAG__ETAG_DOT1P_MAPPING_PTRf, etag_dot1p_mapping_ptr);
    } else {
        return BCM_E_PARAM;
    }
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
            SD_TAG__VNTAG_ACTIONSf, 2);
    /* The field SD_TAG_VID is not used for Port Extension.
     * Store match_vlan here for warm boot recovery.
     */
    if (extender_port->match_vlan >= BCM_VLAN_INVALID) {
        return BCM_E_PARAM;
    }
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
            SD_TAG__SD_TAG_VIDf, extender_port->match_vlan);
    rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm,
            MEM_BLOCK_ALL, *nh_index, &egr_nh);
    if (rv < 0) {
        goto cleanup;
    }

    /* Resolve gport */

    rv = _bcm_esw_gport_resolve(unit, extender_port->port, &mod_out, 
            &port_out, &trunk_id, &id);
    if (rv < 0) {
        goto cleanup;
    }

    ing_nh_port = -1;
    ing_nh_module = -1;
    ing_nh_trunk = -1;

    if (BCM_GPORT_IS_TRUNK(extender_port->port)) {
        ing_nh_module = -1;
        ing_nh_port = -1;
        ing_nh_trunk = trunk_id;
    } else {
        ing_nh_module = mod_out;
        ing_nh_port = port_out;
        ing_nh_trunk = -1;
    }

    /* Write ING_L3_NEXT_HOP entry */

    sal_memset(&ing_nh, 0, sizeof(ing_l3_next_hop_entry_t));

    if (ing_nh_trunk == -1) {
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, PORT_NUMf, ing_nh_port);
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, MODULE_IDf, ing_nh_module);
    } else {    
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, Tf, 1);
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, TGIDf, ing_nh_trunk);
    }

    soc_mem_field32_set(unit, ING_L3_NEXT_HOPm, &ing_nh, DROPf, drop);
    soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
            &ing_nh, ENTRY_TYPEf, 0x2); /* L2 DVP */

    if (soc_mem_field_valid(unit, ING_L3_NEXT_HOPm, DVP_ATTRIBUTE_1_INDEXf)) {
        uint32 mtu_profile_index;

        rv = _bcm_tr3_mtu_profile_index_get(unit, 0x3fff, &mtu_profile_index);
        if (rv < 0) {
            goto cleanup;
        }
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm, &ing_nh,
                DVP_ATTRIBUTE_1_INDEXf, mtu_profile_index);
    } else {
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm, &ing_nh,
                MTU_SIZEf, 0x3fff);
    }

    rv = soc_mem_write (unit, ING_L3_NEXT_HOPm,
            MEM_BLOCK_ALL, *nh_index, &ing_nh);
    if (rv < 0) {
        goto cleanup;
    }

    /* Write INITIAL_ING_L3_NEXT_HOP entry */

    sal_memset(&initial_ing_nh, 0, sizeof(initial_ing_l3_next_hop_entry_t));
    if (ing_nh_trunk == -1) {
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, PORT_NUMf, ing_nh_port);
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, MODULE_IDf, ing_nh_module);
    } else {
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, Tf, 1);
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, TGIDf, ing_nh_trunk);
    }
    rv = soc_mem_write(unit, INITIAL_ING_L3_NEXT_HOPm,
            MEM_BLOCK_ALL, *nh_index, &initial_ing_nh);
    if (rv < 0) {
        goto cleanup;
    }

    return rv;

cleanup:
    if (!(extender_port->flags & BCM_EXTENDER_PORT_REPLACE)) {
        (void) bcm_xgs3_nh_del(unit, _BCM_L3_SHR_WRITE_DISABLE, *nh_index);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_extender_nh_info_delete
 * Purpose:
 *      Free next hop index and clear next hop tables.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      nh_index - (IN) Next hop index. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr3_extender_nh_info_delete(int unit, int nh_index)
{
    egr_l3_next_hop_entry_t egr_nh;
    ing_l3_next_hop_entry_t ing_nh;
    initial_ing_l3_next_hop_entry_t initial_ing_nh;

    /* Clear EGR_L3_NEXT_HOP entry */
    sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                                   MEM_BLOCK_ALL, nh_index, &egr_nh));

    /* Clear ING_L3_NEXT_HOP entry */
    sal_memset(&ing_nh, 0, sizeof(ing_l3_next_hop_entry_t));
    BCM_IF_ERROR_RETURN(soc_mem_write (unit, ING_L3_NEXT_HOPm,
                                   MEM_BLOCK_ALL, nh_index, &ing_nh));

    /* Clear INITIAL_ING_L3_NEXT_HOP entry */
    sal_memset(&initial_ing_nh, 0, sizeof(initial_ing_l3_next_hop_entry_t));
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, INITIAL_ING_L3_NEXT_HOPm,
                                   MEM_BLOCK_ALL, nh_index, &initial_ing_nh));

    /* Free the next-hop index. */
    BCM_IF_ERROR_RETURN
        (bcm_xgs3_nh_del(unit, _BCM_L3_SHR_WRITE_DISABLE, nh_index));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_extender_match_add
 * Purpose:
 *      Add match criteria for Extender VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      extender_port - (IN) Pointer to VLAN virtual port structure. 
 *      vp - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr3_extender_match_add(int unit, bcm_extender_port_t *extender_port,
        int vp)
{
    vlan_xlate_entry_t    vent, old_vent;
    int                   key_type;
    bcm_module_t          mod_out;
    bcm_port_t            port_out;
    bcm_trunk_t           trunk_out;
    int                   tmp_id;
    bcm_vlan_action_set_t action;
    uint32                profile_idx;
    int                   rv;
    bcm_trunk_t           trunk_id;
    int                   idx;
    int                   mod_local;
    int                   num_local_ports;
    bcm_port_t            local_ports[SOC_MAX_NUM_PORTS];
    int                   local_member_count;
    bcm_port_t            local_member_array[SOC_MAX_NUM_PORTS];
    int                   port_key_type_vif_vlan, port_key_type_vif;
    int                   port_key_type_a, port_key_type_b;
    int                   use_port_a, use_port_b;
    int                   vt_enable;

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));

    soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);

    if (extender_port->match_vlan == BCM_VLAN_NONE) {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF, &key_type));
    } else {
        if (!BCM_VLAN_VALID(extender_port->match_vlan)) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF_VLAN, &key_type));
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__VLANf,
                extender_port->match_vlan);
    }
    soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, key_type);

    if (extender_port->extended_port_vid >=
            (1 << soc_mem_field_length(unit, VLAN_XLATEm, VIF__SRC_VIFf))) {
        return BCM_E_PARAM;
    }
    soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__SRC_VIFf,
            extender_port->extended_port_vid);

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_mem_field_valid(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, extender_port->port,
                                &mod_out, &port_out, &trunk_out, &tmp_id));
    if (BCM_GPORT_IS_TRUNK(extender_port->port)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__TGIDf, trunk_out);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__PORT_NUMf, port_out);
    }

    soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__MPLS_ACTIONf, 0x1); /* SVP */
    soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__SOURCE_VPf, vp);

    bcm_vlan_action_set_t_init(&action);
    if (BCM_VLAN_VALID(extender_port->match_vlan)) {
        action.dt_outer = bcmVlanActionCopy;
        action.dt_inner = bcmVlanActionDelete;
        action.ot_outer = bcmVlanActionReplace;
    } else {
        action.ot_outer_prio = bcmVlanActionReplace;
        action.ut_outer = bcmVlanActionAdd;
    }
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_add(unit, &action, &profile_idx));
    soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__TAG_ACTION_PROFILE_PTRf,
            profile_idx);
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_mem_field_valid(unit, VLAN_XLATEm, VIF__VLAN_ACTION_VALIDf)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__VLAN_ACTION_VALIDf, 1);
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    rv = soc_mem_insert_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
            &vent, &old_vent);
    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &old_vent,
                VIF__TAG_ACTION_PROFILE_PTRf);       
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    BCM_IF_ERROR_RETURN(rv);

    num_local_ports = 0;
    if (BCM_GPORT_IS_TRUNK(extender_port->port)) {
        trunk_id = BCM_GPORT_TRUNK_GET(extender_port->port);
        rv = _bcm_trunk_id_validate(unit, trunk_id);
        if (BCM_FAILURE(rv)) {
            return BCM_E_PORT;
        }
        rv = _bcm_esw_trunk_local_members_get(unit, trunk_id,
                SOC_MAX_NUM_PORTS, local_member_array, &local_member_count);
        if (BCM_FAILURE(rv)) {
            return BCM_E_PORT;
        }
        for (idx = 0; idx < local_member_count; idx++) {
            local_ports[num_local_ports++] = local_member_array[idx];
        }
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, extender_port->port, &mod_out, &port_out,
                                    &trunk_id, &tmp_id)); 
        if ((trunk_id != -1) || (tmp_id != -1)) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &mod_local));
        if (mod_local) {
            local_ports[num_local_ports++] = port_out;
        }
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_pt_vtkey_type_value_get(unit,
                VLXLT_HASH_KEY_TYPE_VIF_VLAN, &port_key_type_vif_vlan));
    BCM_IF_ERROR_RETURN(_bcm_esw_pt_vtkey_type_value_get(unit,
                VLXLT_HASH_KEY_TYPE_VIF, &port_key_type_vif));
    for (idx = 0; idx < num_local_ports; idx++) {
        BCM_IF_ERROR_RETURN(_bcm_esw_port_config_get(unit, local_ports[idx],
                    _bcmPortVTKeyTypeFirst, &port_key_type_a));
        BCM_IF_ERROR_RETURN(_bcm_esw_port_config_get(unit, local_ports[idx],
                    _bcmPortVTKeyPortFirst, &use_port_a));
        BCM_IF_ERROR_RETURN(_bcm_esw_port_config_get(unit, local_ports[idx],
                    _bcmPortVTKeyTypeSecond, &port_key_type_b));
        BCM_IF_ERROR_RETURN(_bcm_esw_port_config_get(unit, local_ports[idx],
                    _bcmPortVTKeyPortSecond, &use_port_b));

        if (BCM_VLAN_VALID(extender_port->match_vlan)) {
            if ((port_key_type_a != port_key_type_vif_vlan) &&
                    (port_key_type_b != port_key_type_vif_vlan)) {
                BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                            local_ports[idx], _bcmPortVTKeyTypeFirst,
                            port_key_type_vif_vlan));
                BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                            local_ports[idx], _bcmPortVTKeyPortFirst, 1));
                if (port_key_type_a == port_key_type_vif) {
                    /* The first slot contained VIF key type, which
                     * was just overwritten by VIF_VLAN key type. Hence,
                     * the VIF key type needs to be moved to second slot.
                     */
                    BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                                local_ports[idx], _bcmPortVTKeyTypeSecond,
                                port_key_type_a));
                    BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                                local_ports[idx], _bcmPortVTKeyPortSecond,
                                use_port_a));
                }
            }
        } else {
            if ((port_key_type_a != port_key_type_vif) &&
                    (port_key_type_b != port_key_type_vif)) {
                if (port_key_type_a != port_key_type_vif_vlan) {
                    BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                                local_ports[idx], _bcmPortVTKeyTypeFirst,
                                port_key_type_vif));
                    BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                                local_ports[idx], _bcmPortVTKeyPortFirst, 1));
                } else {
                    BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                                local_ports[idx], _bcmPortVTKeyTypeSecond,
                                port_key_type_vif));
                    BCM_IF_ERROR_RETURN(_bcm_esw_port_config_set(unit,
                                local_ports[idx], _bcmPortVTKeyPortSecond, 1));
                }
            }
        }

        /* Enable ingress VLAN translation */
        BCM_IF_ERROR_RETURN
            (_bcm_esw_port_config_get(unit, local_ports[idx],
                                      _bcmPortVlanTranslate, &vt_enable));
        if (!vt_enable) {
            BCM_IF_ERROR_RETURN
                (_bcm_esw_port_config_set(unit, local_ports[idx],
                                          _bcmPortVlanTranslate, 1));
        }

        if (BCM_VLAN_VALID(extender_port->match_vlan)) {
            /* Enable egress VLAN translation */
            SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                        EGR_VLAN_CONTROL_1r, local_ports[idx], VT_ENABLEf, 1));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_extender_match_delete
 * Purpose:
 *      Delete match criteria for Extender VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vp - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr3_extender_match_delete(int unit, int vp)
{
    vlan_xlate_entry_t vent, old_vent;
    int                key_type;
    bcm_module_t       mod_out;
    bcm_port_t         port_out;
    bcm_trunk_t        trunk_out;
    int                tmp_id;
    uint32             profile_idx;
    int                rv;

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));

    if (BCM_VLAN_VALID(EXTENDER_PORT_INFO(unit, vp)->match_vlan)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF_VLAN, &key_type));
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__VLANf,
                EXTENDER_PORT_INFO(unit, vp)->match_vlan);
    } else {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF, &key_type));
    }
    soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, key_type);

    soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__SRC_VIFf,
            EXTENDER_PORT_INFO(unit, vp)->extended_port_vid);

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_mem_field_valid(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, EXTENDER_PORT_INFO(unit, vp)->port,
                                &mod_out, &port_out, &trunk_out, &tmp_id));
    if (BCM_GPORT_IS_TRUNK(EXTENDER_PORT_INFO(unit, vp)->port)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__TGIDf, trunk_out);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__PORT_NUMf, port_out);
    }

    rv = soc_mem_delete_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
            &vent, &old_vent);
    if ((rv == SOC_E_NONE) &&
            soc_VLAN_XLATEm_field32_get(unit, &old_vent, VALIDf)) {
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &old_vent,
                                                  VIF__TAG_ACTION_PROFILE_PTRf);       
        /* Delete the old vlan action profile entry */
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_extender_port_add
 * Purpose:
 *      Create an Extender port on Controlling Bridge.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      extender_port - (IN/OUT) Extender port information
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr3_extender_port_add(int unit,
                         bcm_extender_port_t *extender_port)
{
    int mode;
    int vp;
    int rv = BCM_E_NONE;
    int num_vp;
    int nh_index = 0;
    ing_dvp_table_entry_t dvp_entry;
    source_vp_entry_t svp_entry;
    int cml_default_enable=0, cml_default_new=0, cml_default_move=0;

    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_egress_mode_get(unit, &mode));
    if (!mode) {
        soc_cm_debug(DK_L3, "L3 egress mode must be set first\n");
        return BCM_E_DISABLED;
    }

    if (!(extender_port->flags & BCM_EXTENDER_PORT_REPLACE)) {
        /* Create new Extender VP */

        if (extender_port->flags & BCM_EXTENDER_PORT_WITH_ID) {
            if (!BCM_GPORT_IS_EXTENDER_PORT(extender_port->extender_port_id)) {
                return BCM_E_PARAM;
            }
            vp = BCM_GPORT_EXTENDER_PORT_ID_GET(extender_port->extender_port_id);
            if (_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                return BCM_E_EXISTS;
            } else {
                rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeExtender);
                if (rv < 0) {
                    return rv;
                }
            }
        } else {
            /* allocate a new VP index */
            num_vp = soc_mem_index_count(unit, SOURCE_VPm);
            rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm,
                    _bcmVpTypeExtender, &vp);
            if (rv < 0) {
                return rv;
            }
        }

        /* Configure next hop tables */
        rv = _bcm_tr3_extender_nh_info_set(unit, extender_port, vp, 0,
                                              &nh_index);
        if (rv < 0) {
            goto cleanup;
        }

        /* Configure DVP table */
        sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
        soc_ING_DVP_TABLEm_field32_set(unit, &dvp_entry, NEXT_HOP_INDEXf,
                                       nh_index);
        rv = WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp_entry);
        if (rv < 0) {
            goto cleanup;
        }

        /* Configure SVP table */
        sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
        soc_SOURCE_VPm_field32_set(unit, &svp_entry, ENTRY_TYPEf, 3);

        /* Set the CML */
        rv = _bcm_vp_default_cml_mode_get(unit, &cml_default_enable,
                &cml_default_new, &cml_default_move);
        if (rv < 0) {
            goto cleanup;
        }
        if (cml_default_enable) {
            /* Set the CML to default values */
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_NEWf,
                    cml_default_new);
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_MOVEf,
                    cml_default_move);
        } else {
            /* Set the CML to PVP_CML_SWITCH by default (hw learn and forward) */
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_NEWf, 0x8);
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_MOVEf, 0x8);
        }

        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp_entry);
        if (rv < 0) {
            goto cleanup;
        }

        /* Configure SOURCE_VP_2 table */
#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_mem_is_valid(unit, SOURCE_VP_2m)) {
            source_vp_2_entry_t svp_2_entry;

            sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
            soc_SOURCE_VP_2m_field32_set(unit, &svp_2_entry, PARSE_USING_SGLP_TPIDf, 1);
            rv = WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, vp, &svp_2_entry);
            if (rv < 0) {
                goto cleanup;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */

        /* Configure ingress VLAN translation table for unicast VPs */
        if (!(extender_port->flags & BCM_EXTENDER_PORT_MULTICAST)) {
            rv = _bcm_tr3_extender_match_add(unit, extender_port, vp);
            if (rv < 0) {
                goto cleanup;
            }
        }

        /* Increment port's VP count */
        rv = _bcm_tr3_extender_port_cnt_update(unit, extender_port->port, vp,
                TRUE);
        if (rv < 0) {
            goto cleanup;
        }

    } else { /* Replace properties of existing Extender VP */

        if (!(extender_port->flags & BCM_EXTENDER_PORT_WITH_ID)) {
            return BCM_E_PARAM;
        }
        if (!BCM_GPORT_IS_EXTENDER_PORT(extender_port->extender_port_id)) {
            return BCM_E_PARAM;
        }
        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(extender_port->extender_port_id);
        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
            return BCM_E_PARAM;
        }

        /* For existing Extender vp, NH entry already exists */
        BCM_IF_ERROR_RETURN
            (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry));
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry,
                NEXT_HOP_INDEXf);

        /* Update existing next hop entries */
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_extender_nh_info_set(unit, extender_port, vp, 0,
                                           &nh_index));

        /* Delete old ingress VLAN translation entry,
         * install new ingress VLAN translation entry
         */
        if (!(EXTENDER_PORT_INFO(unit, vp)->flags &
                    BCM_EXTENDER_PORT_MULTICAST)) {
            BCM_IF_ERROR_RETURN(_bcm_tr3_extender_match_delete(unit, vp));
        }
        if (!(extender_port->flags & BCM_EXTENDER_PORT_MULTICAST)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_extender_match_add(unit, extender_port, vp));
        }

        /* Decrement old port's VP count, increment new port's VP count */
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_extender_port_cnt_update(unit,
                EXTENDER_PORT_INFO(unit, vp)->port, vp, FALSE));
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_extender_port_cnt_update(unit,
                EXTENDER_PORT_INFO(unit, vp)->port, vp, TRUE));
    }

    /* Set Extender VP software state */
    EXTENDER_PORT_INFO(unit, vp)->flags = extender_port->flags;
    EXTENDER_PORT_INFO(unit, vp)->port = extender_port->port;
    EXTENDER_PORT_INFO(unit, vp)->extended_port_vid =
        extender_port->extended_port_vid;
    EXTENDER_PORT_INFO(unit, vp)->pcp_de_select = extender_port->pcp_de_select;
    EXTENDER_PORT_INFO(unit, vp)->pcp = extender_port->pcp;
    EXTENDER_PORT_INFO(unit, vp)->de = extender_port->de;
    EXTENDER_PORT_INFO(unit, vp)->match_vlan = extender_port->match_vlan;

    BCM_GPORT_EXTENDER_PORT_ID_SET(extender_port->extender_port_id, vp);

    return rv;

cleanup:
    if (!(extender_port->flags & BCM_EXTENDER_PORT_REPLACE)) {
        (void) _bcm_vp_free(unit, _bcmVpTypeExtender, 1, vp);
        _bcm_tr3_extender_nh_info_delete(unit, nh_index);

        sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
        (void)WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp_entry);

        sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
        (void)WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp_entry);

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_mem_is_valid(unit, SOURCE_VP_2m)) {
            source_vp_2_entry_t svp_2_entry;

            sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
            (void)WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, vp, &svp_2_entry);
        }
#endif /* BCM_TRIDENT2_SUPPORT */

        if (!(extender_port->flags & BCM_EXTENDER_PORT_MULTICAST)) {
            (void) _bcm_tr3_extender_match_delete(unit, vp);
        }
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_extender_port_delete
 * Purpose:
 *      Destroy a Extender virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) Extender GPORT ID.
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr3_extender_port_delete(int unit, bcm_gport_t gport)
{
    int vp;
    source_vp_entry_t svp_entry;
    int nh_index;
    ing_dvp_table_entry_t dvp_entry;

    if (!BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        return BCM_E_PARAM;
    }

    vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
        return BCM_E_NOT_FOUND;
    }

    /* Delete ingress VLAN translation entry */
    if (!(EXTENDER_PORT_INFO(unit, vp)->flags & BCM_EXTENDER_PORT_MULTICAST)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_extender_match_delete(unit, vp));
    }

    /* Clear SVP entry */
    sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
    BCM_IF_ERROR_RETURN
        (WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp_entry));

    /* Clear SOURCE_VP_2 entry */
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_mem_is_valid(unit, SOURCE_VP_2m)) {
        source_vp_2_entry_t svp_2_entry;

        sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
        BCM_IF_ERROR_RETURN
            (WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, vp, &svp_2_entry));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Clear DVP entry */
    BCM_IF_ERROR_RETURN
        (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry));
    nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry, NEXT_HOP_INDEXf);
    sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
    BCM_IF_ERROR_RETURN
        (WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp_entry));

    /* Clear next hop entries and free next hop index */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_extender_nh_info_delete(unit, nh_index));

    /* Decrement port's VP count */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_extender_port_cnt_update(unit,
                                          EXTENDER_PORT_INFO(unit, vp)->port,
                                          vp, FALSE));
    /* Free VP */
    BCM_IF_ERROR_RETURN
        (_bcm_vp_free(unit, _bcmVpTypeExtender, 1, vp));

    /* Clear Extender VP software state */
    sal_memset(EXTENDER_PORT_INFO(unit, vp), 0,
            sizeof(_bcm_tr3_extender_port_info_t));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_extender_port_delete_all
 * Purpose:
 *      Destroy all Extender virtual ports.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr3_extender_port_delete_all(int unit)
{
    int i;
    bcm_gport_t extender_port_id;

    for (i = soc_mem_index_min(unit, SOURCE_VPm);
         i <= soc_mem_index_max(unit, SOURCE_VPm);
         i++) {
        if (_bcm_vp_used_get(unit, i, _bcmVpTypeExtender)) {
            BCM_GPORT_EXTENDER_PORT_ID_SET(extender_port_id, i);
            BCM_IF_ERROR_RETURN
                (bcm_tr3_extender_port_delete(unit, extender_port_id));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_extender_port_get
 * Purpose:
 *      Get Extender virtual port info.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      extender_port - (IN/OUT) Pointer to Extender virtual port structure. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr3_extender_port_get(int unit, bcm_extender_port_t *extender_port)
{
    int vp;

    if (!BCM_GPORT_IS_EXTENDER_PORT(extender_port->extender_port_id)) {
        return BCM_E_PARAM;
    }

    vp = BCM_GPORT_EXTENDER_PORT_ID_GET(extender_port->extender_port_id);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
        return BCM_E_NOT_FOUND;
    }

    bcm_extender_port_t_init(extender_port);
    extender_port->flags = EXTENDER_PORT_INFO(unit, vp)->flags;
    BCM_GPORT_EXTENDER_PORT_ID_SET(extender_port->extender_port_id, vp);
    extender_port->port = EXTENDER_PORT_INFO(unit, vp)->port;
    extender_port->extended_port_vid = EXTENDER_PORT_INFO(unit, vp)->
                                       extended_port_vid;
    extender_port->pcp_de_select = EXTENDER_PORT_INFO(unit, vp)->pcp_de_select;
    extender_port->pcp = EXTENDER_PORT_INFO(unit, vp)->pcp;
    extender_port->de = EXTENDER_PORT_INFO(unit, vp)->de;
    extender_port->match_vlan = EXTENDER_PORT_INFO(unit, vp)->match_vlan;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_extender_port_traverse
 * Purpose:
 *      Traverse all Extender ports and call supplied callback routine.
 * Parameters:
 *      unit      - (IN) Device Number
 *      cb        - (IN) User-provided callback
 *      user_data - (IN/OUT) Cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_extender_port_traverse(int unit, bcm_extender_port_traverse_cb cb, 
                                  void *user_data)
{
    int i;
    bcm_extender_port_t extender_port;

    for (i = soc_mem_index_min(unit, SOURCE_VPm);
         i <= soc_mem_index_max(unit, SOURCE_VPm);
         i++) {
        if (_bcm_vp_used_get(unit, i, _bcmVpTypeExtender)) {
            bcm_extender_port_t_init(&extender_port);
            BCM_GPORT_EXTENDER_PORT_ID_SET(extender_port.extender_port_id, i);
            BCM_IF_ERROR_RETURN(bcm_tr3_extender_port_get(unit, &extender_port));
            BCM_IF_ERROR_RETURN(cb(unit, &extender_port, user_data));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_extender_port_resolve
 * Purpose:
 *      Get the modid, port, trunk values for a Extender virtual port
 * Parameters:
 *      unit     - (IN) BCM device number
 *      gport    - (IN) Global port identifier
 *      modid    - (OUT) Module ID
 *      port     - (OUT) Port number
 *      trunk_id - (OUT) Trunk ID
 *      id       - (OUT) Virtual port ID
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr3_extender_port_resolve(int unit, bcm_gport_t extender_port_id,
                          bcm_module_t *modid, bcm_port_t *port,
                          bcm_trunk_t *trunk_id, int *id)

{
    int rv = BCM_E_NONE, nh_index, vp;
    ing_l3_next_hop_entry_t ing_nh;
    ing_dvp_table_entry_t dvp;

    if (!BCM_GPORT_IS_EXTENDER_PORT(extender_port_id)) {
        return (BCM_E_BADID);
    }

    vp = BCM_GPORT_EXTENDER_PORT_ID_GET(extender_port_id);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
        return BCM_E_NOT_FOUND;
    }
    BCM_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
    nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp,
                                              NEXT_HOP_INDEXf);
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                      nh_index, &ing_nh));

    if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, ENTRY_TYPEf) != 0x2) {
        /* Entry type is not L2 DVP */
        return BCM_E_NOT_FOUND;
    }
    if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
        *trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
    } else {
        *modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
        *port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
    }
    *id = vp;
    return rv;
}

/*
 * Purpose:
 *	Add a downstream forwarding entry
 * Parameters:
 *	unit - (IN) Device Number
 *	forward_entry - (IN) Forwarding entry
 */
int
bcm_tr3_extender_forward_add(int unit, bcm_extender_forward_t *forward_entry)
{
    int rv = BCM_E_NONE;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t tgid_out;
    int id_out;
    l2_entry_1_entry_t l2_entry;
    int lower_th, higher_th;

    if (forward_entry->name_space > 0xfff) {
        return BCM_E_PARAM;
    }

    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
        if (forward_entry->extended_port_vid > 0xfff) {
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit,
                    forward_entry->dest_port,
                    &mod_out, &port_out, &tgid_out, &id_out));
        if (-1 != id_out) {
            return BCM_E_PARAM;
        }

        sal_memset(&l2_entry, 0, sizeof(l2_entry));
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, VALIDf, 1);
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, KEY_TYPEf,
                SOC_MEM_KEY_L2_ENTRY_1_PE_VID_PE_VID);
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__NAMESPACEf,
                forward_entry->name_space);
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__ETAG_VIDf,
                forward_entry->extended_port_vid);
        if (-1 != tgid_out) {
            BCM_IF_ERROR_RETURN(_bcm_trunk_id_validate(unit, tgid_out));
            soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__DEST_TYPEf,
                    1);
            soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__TGIDf,
                    tgid_out);
        } else {
            soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__MODULE_IDf,
                    mod_out);
            soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__PORT_NUMf,
                    port_out);
        }
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, STATIC_BITf, 1);

    } else { /* Multicast forward entry */
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastHigherThreshold, &higher_th));
        if ((forward_entry->extended_port_vid < lower_th) ||
               (forward_entry->extended_port_vid > higher_th)) {
            return BCM_E_PARAM;
        }
        if (!_BCM_MULTICAST_IS_L2(forward_entry->dest_multicast)) {
            return BCM_E_PARAM;
        }

        sal_memset(&l2_entry, 0, sizeof(l2_entry));
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, VALIDf, 1);
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, KEY_TYPEf,
                SOC_MEM_KEY_L2_ENTRY_1_PE_VID_PE_VID);
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__NAMESPACEf,
                forward_entry->name_space);
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__ETAG_VIDf,
                forward_entry->extended_port_vid);
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__L2MC_PTRf,
                _BCM_MULTICAST_ID_GET(forward_entry->dest_multicast));
        soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, STATIC_BITf, 1);
    }

    soc_mem_lock(unit, L2_ENTRY_1m);

    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_REPLACE)) {
        rv = soc_mem_insert(unit, L2_ENTRY_1m, MEM_BLOCK_ALL, &l2_entry);
        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, L2_ENTRY_1m);
            return rv;
        }
    } else { /* Replace existing entry */
        rv = soc_mem_delete(unit, L2_ENTRY_1m, MEM_BLOCK_ALL, &l2_entry);
        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, L2_ENTRY_1m);
            return rv;
        }
        rv = soc_mem_insert(unit, L2_ENTRY_1m, MEM_BLOCK_ALL, &l2_entry);
        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, L2_ENTRY_1m);
            return rv;
        }
    }

    soc_mem_unlock(unit, L2_ENTRY_1m);

    return rv;
}

/*
 * Purpose:
 *	Delete a downstream forwarding entry
 * Parameters:
 *      unit - (IN) Device Number
 *      forward_entry - (IN) Forwarding entry
 */
int
bcm_tr3_extender_forward_delete(int unit,
        bcm_extender_forward_t *forward_entry)
{
    int rv = BCM_E_NONE;
    l2_entry_1_entry_t l2_entry;
    int lower_th, higher_th;

    if (forward_entry->name_space > 0xfff) {
        return BCM_E_PARAM;
    }
    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
        if (forward_entry->extended_port_vid > 0xfff) {
            return BCM_E_PARAM;
        }
    } else { /* Multicast forward entry */
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastHigherThreshold, &higher_th));
        if ((forward_entry->extended_port_vid < lower_th) ||
               (forward_entry->extended_port_vid > higher_th)) {
            return BCM_E_PARAM;
        }
    }

    sal_memset(&l2_entry, 0, sizeof(l2_entry));
    soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, KEY_TYPEf,
            SOC_MEM_KEY_L2_ENTRY_1_PE_VID_PE_VID);
    soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__NAMESPACEf,
            forward_entry->name_space);
    soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__ETAG_VIDf,
            forward_entry->extended_port_vid);
    soc_mem_lock(unit, L2_ENTRY_1m);
    rv = soc_mem_delete(unit, L2_ENTRY_1m, MEM_BLOCK_ALL, &l2_entry);
    soc_mem_unlock(unit, L2_ENTRY_1m);

    return rv;
}

/*
 * Purpose:
 *	Delete all downstream forwarding entries
 * Parameters:
 *	unit - Device Number
 */
int
bcm_tr3_extender_forward_delete_all(int unit)
{
    int rv = BCM_E_NONE;
    int seconds, enabled;
    l2_entry_1_entry_t match_mask;
    l2_entry_1_entry_t match_data;
    l2_bulk_entry_t l2_bulk;
    int match_mask_index, match_data_index;
    uint32 rval;
    int field_len;

    SOC_IF_ERROR_RETURN
        (SOC_FUNCTIONS(unit)->soc_age_timer_get(unit, &seconds, &enabled));
    if (enabled) {
        SOC_IF_ERROR_RETURN(soc_tr3_l2_bulk_age_stop(unit));
    }

    soc_mem_lock(unit, L2_ENTRY_1m);

    /* Set match mask and data */
    sal_memset(&match_mask, 0, sizeof(match_mask));
    sal_memset(&match_data, 0, sizeof(match_data));

    soc_mem_field32_set(unit, L2_ENTRY_1m, &match_mask, VALIDf, 1);
    soc_mem_field32_set(unit, L2_ENTRY_1m, &match_data, VALIDf, 1);

    soc_mem_field32_set(unit, L2_ENTRY_1m, &match_mask, WIDEf, 1);
    soc_mem_field32_set(unit, L2_ENTRY_1m, &match_data, WIDEf, 0);

    field_len = soc_mem_field_length(unit, L2_ENTRY_1m, KEY_TYPEf);
    soc_mem_field32_set(unit, L2_ENTRY_1m, &match_mask, KEY_TYPEf,
            (1 << field_len) - 1);
    soc_mem_field32_set(unit, L2_ENTRY_1m, &match_data, KEY_TYPEf,
            SOC_MEM_KEY_L2_ENTRY_1_PE_VID_PE_VID);

    sal_memset(&l2_bulk, 0, sizeof(l2_bulk));
    sal_memcpy(&l2_bulk, &match_mask, sizeof(match_mask));
    match_mask_index = 1;
    rv = WRITE_L2_BULKm(unit, MEM_BLOCK_ALL, match_mask_index, &l2_bulk);
    if (BCM_SUCCESS(rv)) {
        sal_memset(&l2_bulk, 0, sizeof(l2_bulk));
        sal_memcpy(&l2_bulk, &match_data, sizeof(match_data));
        match_data_index = 0;
        rv = WRITE_L2_BULKm(unit, MEM_BLOCK_ALL, match_data_index, &l2_bulk);
    }

    /* Set bulk control */
    rval = 0;
    soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, L2_MOD_FIFO_RECORDf, 0);
    soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, ACTIONf, 1);
    soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, BURST_ENTRIESf, 
                      _SOC_TR3_L2_BULK_BURST_MAX);
    soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, ENTRY_WIDTHf, 0);
    soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, NUM_ENTRIESf, 
                      soc_mem_index_count(unit, L2_ENTRY_1m));
    soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, EXTERNAL_L2_ENTRYf, 0);
    if (BCM_SUCCESS(rv)) {
        rv = WRITE_L2_BULK_CONTROLr(unit, rval);
    }
    if (BCM_SUCCESS(rv)) {
        rv = soc_tr3_l2_port_age(unit, L2_BULK_CONTROLr, INVALIDr);
    }

    soc_mem_unlock(unit, L2_ENTRY_1m);

    if(enabled) {
        SOC_IF_ERROR_RETURN(soc_tr3_l2_bulk_age_start(unit, seconds));
    }

    return rv;
}

/*
 * Purpose:
 *      Get a downstream forwarding entry
 * Parameters:
 *      unit - (IN) Device Number
 *      forward_entry - (IN/OUT) Forwarding entry
 */
int
bcm_tr3_extender_forward_get(int unit, bcm_extender_forward_t *forward_entry)
{
    int rv = BCM_E_NONE;
    l2_entry_1_entry_t l2_entry, l2_entry_out;
    int idx;
    _bcm_gport_dest_t dest;
    int l2mc_index;
    int lower_th, higher_th;

    if (forward_entry->name_space > 0xfff) {
        return BCM_E_PARAM;
    }
    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
        if (forward_entry->extended_port_vid > 0xfff) {
            return BCM_E_PARAM;
        }
    } else { /* Multicast forward entry */
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastHigherThreshold, &higher_th));
        if ((forward_entry->extended_port_vid < lower_th) ||
               (forward_entry->extended_port_vid > higher_th)) {
            return BCM_E_PARAM;
        }
    }

    sal_memset(&l2_entry, 0, sizeof(l2_entry));
    soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, KEY_TYPEf,
            SOC_MEM_KEY_L2_ENTRY_1_PE_VID_PE_VID);
    soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__NAMESPACEf,
            forward_entry->name_space);
    soc_L2_ENTRY_1m_field32_set(unit, &l2_entry, PE_VID__ETAG_VIDf,
            forward_entry->extended_port_vid);
    soc_mem_lock(unit, L2_ENTRY_1m);
    rv = soc_mem_search(unit, L2_ENTRY_1m, MEM_BLOCK_ALL, &idx, &l2_entry,
            &l2_entry_out, 0);
    soc_mem_unlock(unit, L2_ENTRY_1m);

    if (SOC_SUCCESS(rv)) {
        if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
            if (soc_L2_ENTRY_1m_field32_get(unit, &l2_entry_out,
                        PE_VID__DEST_TYPEf)) {
                dest.tgid = soc_L2_ENTRY_1m_field32_get(unit, &l2_entry_out,
                        PE_VID__TGIDf);
                dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
            } else {
                dest.modid = soc_L2_ENTRY_1m_field32_get(unit, &l2_entry_out,
                        PE_VID__MODULE_IDf);
                dest.port = soc_L2_ENTRY_1m_field32_get(unit, &l2_entry_out,
                        PE_VID__PORT_NUMf);
                dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
            }
            BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &dest,
                        &(forward_entry->dest_port)));
        } else {
            l2mc_index = soc_L2_ENTRY_1m_field32_get(unit, &l2_entry_out,
                    PE_VID__L2MC_PTRf);
            _BCM_MULTICAST_GROUP_SET(forward_entry->dest_multicast,
                    _BCM_MULTICAST_TYPE_L2, l2mc_index);
        }
    }

    return rv;
}

/*
 * Purpose:
 *      Traverse all valid downstream forwarding entries and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per forwarding entry.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_extender_forward_traverse(int unit,
                             bcm_extender_forward_traverse_cb cb,
                             void *user_data)
{
    int rv = BCM_E_NONE;
    int chunk_entries, chunk_bytes;
    uint8 *l2_tbl_chunk = NULL;
    int chunk_idx_min, chunk_idx_max;
    int ent_idx;
    l2_entry_1_entry_t *l2_entry;
    bcm_extender_forward_t forward_entry;
    int l2mc_index;
    _bcm_gport_dest_t dest;
    int lower_th, higher_th;

    /* Get ETAG VID multicast range */
    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchExtenderMulticastHigherThreshold, &higher_th));

    chunk_entries = soc_property_get(unit, spn_L2DELETE_CHUNKS,
                                 L2_MEM_CHUNKS_DEFAULT);
    chunk_bytes = 4 * SOC_MEM_WORDS(unit, L2_ENTRY_1m) * chunk_entries;
    l2_tbl_chunk = soc_cm_salloc(unit, chunk_bytes, "extender forward traverse");
    if (NULL == l2_tbl_chunk) {
        return BCM_E_MEMORY;
    }
    for (chunk_idx_min = soc_mem_index_min(unit, L2_ENTRY_1m); 
         chunk_idx_min <= soc_mem_index_max(unit, L2_ENTRY_1m); 
         chunk_idx_min += chunk_entries) {

        /* Read a chunk of L2 table */
        sal_memset(l2_tbl_chunk, 0, chunk_bytes);
        chunk_idx_max = chunk_idx_min + chunk_entries - 1;
        if (chunk_idx_max > soc_mem_index_max(unit, L2_ENTRY_1m)) {
            chunk_idx_max = soc_mem_index_max(unit, L2_ENTRY_1m);
        }
        rv = soc_mem_read_range(unit, L2_ENTRY_1m, MEM_BLOCK_ANY,
                                chunk_idx_min, chunk_idx_max, l2_tbl_chunk);
        if (SOC_FAILURE(rv)) {
            break;
        }
        for (ent_idx = 0;
             ent_idx <= (chunk_idx_max - chunk_idx_min);
             ent_idx++) {
            l2_entry = soc_mem_table_idx_to_pointer(unit, L2_ENTRY_1m,
                    l2_entry_1_entry_t *, l2_tbl_chunk, ent_idx);

            if (soc_L2_ENTRY_1m_field32_get(unit, l2_entry, VALIDf) == 0) {
                continue;
            }
            if (soc_L2_ENTRY_1m_field32_get(unit, l2_entry, KEY_TYPEf) != 
                    SOC_MEM_KEY_L2_ENTRY_1_PE_VID_PE_VID) {
                continue;
            }

            /* Get fields of forwarding entry */
            bcm_extender_forward_t_init(&forward_entry);
            forward_entry.name_space = soc_L2_ENTRY_1m_field32_get(unit,
                    l2_entry, PE_VID__NAMESPACEf);
            forward_entry.extended_port_vid = soc_L2_ENTRY_1m_field32_get(unit,
                    l2_entry, PE_VID__ETAG_VIDf);
            if ((forward_entry.extended_port_vid >= lower_th) &&
                (forward_entry.extended_port_vid <= higher_th)) {
                /* Multicast downstream forwarding entry */
                forward_entry.flags |= BCM_EXTENDER_FORWARD_MULTICAST;
                l2mc_index = soc_L2_ENTRY_1m_field32_get(unit, l2_entry,
                        PE_VID__L2MC_PTRf);
                _BCM_MULTICAST_GROUP_SET(forward_entry.dest_multicast,
                        _BCM_MULTICAST_TYPE_L2, l2mc_index);
            } else {
                if (soc_L2_ENTRY_1m_field32_get(unit, l2_entry,
                            PE_VID__DEST_TYPEf)) {
                    dest.tgid = soc_L2_ENTRY_1m_field32_get(unit, l2_entry,
                            PE_VID__TGIDf);
                    dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
                } else {
                    dest.modid = soc_L2_ENTRY_1m_field32_get(unit, l2_entry,
                            PE_VID__MODULE_IDf);
                    dest.port = soc_L2_ENTRY_1m_field32_get(unit, l2_entry,
                            PE_VID__PORT_NUMf);
                    dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                }
                rv = _bcm_esw_gport_construct(unit, &dest,
                        &(forward_entry.dest_port));
                if (BCM_FAILURE(rv)) {
                    break;
                }
            }

            /* Invoke callback */
            rv = cb(unit, &forward_entry, user_data);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
        if (BCM_FAILURE(rv)) {
            break;
        }
    }
    soc_cm_sfree(unit, l2_tbl_chunk);

    return rv;
}

/*
 * Function:
 *      bcm_tr3_etag_ethertype_set
 * Purpose:
 *      Set ETAG Ethertype.
 * Parameters:
 *      unit      - (IN) BCM device number
 *      ethertype - (IN) Ethertype
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_etag_ethertype_set(int unit, int ethertype)
{

    if (ethertype < 0 || ethertype > 0xffff) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, PE_ETHERTYPEr,
                                REG_PORT_ANY, ETHERTYPEf, ethertype));
    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, PE_ETHERTYPEr,
                                REG_PORT_ANY, ENABLEf, ethertype ? 1 : 0));

    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, EGR_PE_ETHERTYPEr,
                                REG_PORT_ANY, ETHERTYPEf, ethertype));
    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, EGR_PE_ETHERTYPEr,
                                REG_PORT_ANY, ENABLEf, ethertype ? 1 : 0));

    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, EGR_PE_ETHERTYPE_2r,
                                REG_PORT_ANY, ETHERTYPEf, ethertype));
    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, EGR_PE_ETHERTYPE_2r,
                                REG_PORT_ANY, ENABLEf, ethertype ? 1 : 0));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_etag_ethertype_get
 * Purpose:
 *      Get ETAG Ethertype.
 * Parameters:
 *      unit      - (IN) BCM device number
 *      ethertype - (OUT) Ethertype
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_etag_ethertype_get(int unit, int *ethertype)
{
    uint32 rval;

    if (ethertype == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(READ_PE_ETHERTYPEr(unit, &rval));
    if (soc_reg_field_get(unit, PE_ETHERTYPEr, rval, ENABLEf)) {
        *ethertype = soc_reg_field_get(unit, PE_ETHERTYPEr, rval, ETHERTYPEf);
    } else {
        *ethertype = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_extender_untagged_add
 * Purpose:
 *      Set Extender VP tagging/untagging status by adding
 *      a (Extender VP, VLAN) egress VLAN translation entry.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN ID. 
 *      vp   - (IN) Virtual port number.
 *      flags - (IN) Untagging indication.
 *      egr_vt_added - (OUT) Indicates if (VP, VLAN) added to egress VLAN
 *                           translation table.
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr3_extender_untagged_add(int unit, bcm_vlan_t vlan, int vp, int flags,
        int *egr_vt_added)
{
    egr_vlan_xlate_entry_t vent, old_vent;
    bcm_vlan_action_set_t action;
    uint32 profile_idx;
    int rv = BCM_E_NONE;

    *egr_vt_added = FALSE;

    sal_memset(&vent, 0, sizeof(egr_vlan_xlate_entry_t));
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, ENTRY_TYPEf, 1);
    } else if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, KEY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 1);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, DVPf, vp);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, vlan);

    if (!BCM_VLAN_VALID(EXTENDER_PORT_INFO(unit, vp)->match_vlan)) {
        if (!(flags & BCM_VLAN_PORT_UNTAGGED)) {
            /* No need to insert an egress vlan translation entry
             * to remove the outer tag.
             */
            return BCM_E_NONE;
        }

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_mem_field_valid(unit, EGR_VP_VLAN_MEMBERSHIPm, UNTAGf)) {
            if (flags & BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP) {
                /* The UNTAG field of EGR_VP_VLAN_MEMBERSHIP table will
                 * be configured with untagging status. It would be redundant
                 * to insert an egress VLAN translation entry.
                 */
                return BCM_E_NONE;
            }
        } 
#endif /* BCM_TRIDENT2_SUPPORT */

        bcm_vlan_action_set_t_init(&action);
        action.dt_outer = bcmVlanActionDelete;
        action.ot_outer = bcmVlanActionDelete;
    } else {
        /* Extender port's match_vlan is valid. It needs to be inserted
         * into the packet using an egress vlan translation entry.
         */
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, NEW_OVIDf,
                EXTENDER_PORT_INFO(unit, vp)->match_vlan);
        bcm_vlan_action_set_t_init(&action);
        action.dt_outer = bcmVlanActionReplace;
        action.ot_outer = bcmVlanActionReplace;
        if (flags & BCM_VLAN_PORT_UNTAGGED) {
            action.dt_inner = bcmVlanActionNone;
            action.ot_inner = bcmVlanActionNone;
        } else {
            action.dt_inner = bcmVlanActionCopy;
            action.ot_inner = bcmVlanActionCopy;
        }
    }
    BCM_IF_ERROR_RETURN
        (_bcm_trx_egr_vlan_action_profile_entry_add(unit, &action, &profile_idx));
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, TAG_ACTION_PROFILE_PTRf,
            profile_idx);

    rv = soc_mem_insert_return_old(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL,
                                   &vent, &old_vent);
    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, profile_idx);
    }

    *egr_vt_added = TRUE;

    return rv;
}

/*
 * Function:
 *      bcm_tr3_extender_untagged_delete
 * Purpose:
 *      Delete (Extender VP, VLAN) egress VLAN translation entry.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN ID. 
 *      vp   - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr3_extender_untagged_delete(int unit, bcm_vlan_t vlan, int vp)
{
    egr_vlan_xlate_entry_t vent, old_vent;
    uint32 profile_idx;
    int rv;

    sal_memset(&vent, 0, sizeof(egr_vlan_xlate_entry_t));
    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, ENTRY_TYPEf, 1);
    } else if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, KEY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 1);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, DVPf, vp);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, vlan);

    rv = soc_mem_delete_return_old(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL,
                                   &vent, &old_vent);
    if ((rv == SOC_E_NONE) &&
            soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vent, VALIDf)) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, profile_idx);
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_extender_untagged_get
 * Purpose:
 *      Get tagging/untagging status of a Extender virtual port by
 *      reading the (Extender VP, VLAN) egress vlan translation entry.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN to remove virtual port from.
 *      vp   - (IN) Virtual port number.
 *      flags - (OUT) Untagging status of the Extender virtual port.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_extender_untagged_get(int unit, bcm_vlan_t vlan, int vp,
                                  int *flags)
{
    egr_vlan_xlate_entry_t vent, res_vent;
    int idx;
    uint32 profile_idx;
    int rv;
    bcm_vlan_action_set_t action;

    *flags = 0;
    
    sal_memset(&vent, 0, sizeof(egr_vlan_xlate_entry_t));
    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, ENTRY_TYPEf, 1);
    } else if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, KEY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 1);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, DVPf, vp);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, vlan);

    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &idx,
                                   &vent, &res_vent, 0);
    if ((rv == SOC_E_NONE) &&
            soc_EGR_VLAN_XLATEm_field32_get(unit, &res_vent, VALIDf)) {
        profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &res_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        _bcm_trx_egr_vlan_action_profile_entry_get(unit, &action, profile_idx);

        if (!BCM_VLAN_VALID(EXTENDER_PORT_INFO(unit, vp)->match_vlan)) {
            if (bcmVlanActionDelete == action.ot_outer) {
                *flags = BCM_VLAN_PORT_UNTAGGED;
            }
        } else {
            if (bcmVlanActionNone == action.ot_inner) {
                *flags = BCM_VLAN_PORT_UNTAGGED;
            }
        }
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_niv_port_untagged_vlan_set
 * Purpose:
 *      Set the default VLAN ID for an Extender port.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      port - Extender gport.
 *      vid -  VLAN ID used for packets that ingress without a VLAN tag.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_extender_port_untagged_vlan_set(int unit, bcm_port_t port,
        bcm_vlan_t vid)
{
    int                rv = BCM_E_NONE;
    int                vp;
    vlan_xlate_entry_t key_data, entry_data;
    int                key_type;
    bcm_module_t       mod_out;
    bcm_port_t         port_out;
    bcm_trunk_t        trunk_out;
    int                tmp_id;
    int                entry_index;

    /* Get VP */
    if (BCM_GPORT_IS_EXTENDER_PORT(port)) {
        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(port);
    } else {
        return BCM_E_PORT;
    }

    /* Construct lookup key */
    sal_memset(&key_data, 0, sizeof(vlan_xlate_entry_t));

    if (BCM_VLAN_VALID(EXTENDER_PORT_INFO(unit, vp)->match_vlan)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF_VLAN, &key_type));
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__VLANf,
                EXTENDER_PORT_INFO(unit, vp)->match_vlan);
    } else {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF, &key_type));
    }
    soc_VLAN_XLATEm_field32_set(unit, &key_data, KEY_TYPEf, key_type);

    soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__SRC_VIFf,
            EXTENDER_PORT_INFO(unit, vp)->extended_port_vid);

    if (soc_mem_field_valid(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, SOURCE_TYPEf, 1);
    }
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, EXTENDER_PORT_INFO(unit, vp)->port,
                                &mod_out, &port_out, &trunk_out, &tmp_id));
    if (BCM_GPORT_IS_TRUNK(EXTENDER_PORT_INFO(unit, vp)->port)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__TGIDf, trunk_out);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__PORT_NUMf, port_out);
    }

    /* Lookup existing entry */
    SOC_IF_ERROR_RETURN(soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
                &entry_index, &key_data, &entry_data, 0));

    /* Replace entry's new VLAN */
    soc_VLAN_XLATEm_field32_set(unit, &entry_data, VIF__NEW_OVIDf, vid);

    /* Insert new entry */
    rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &entry_data);
    if (rv == SOC_E_EXISTS) {
        rv = BCM_E_NONE;
    } else {
        return BCM_E_INTERNAL;
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_extender_port_untagged_vlan_get
 * Purpose:
 *      Get the default VLAN ID for an Extender port.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      port - Extender gport.
 *      vid  - (OUT) VLAN ID used for packets that ingress without a VLAN tag.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_extender_port_untagged_vlan_get(int unit, bcm_port_t port,
        bcm_vlan_t *vid)
{
    int                rv = BCM_E_NONE;
    int                vp;
    vlan_xlate_entry_t key_data, entry_data;
    int                key_type;
    bcm_module_t       mod_out;
    bcm_port_t         port_out;
    bcm_trunk_t        trunk_out;
    int                tmp_id;
    int                entry_index;

    /* Get VP */
    if (BCM_GPORT_IS_EXTENDER_PORT(port)) {
        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(port);
    } else {
        return BCM_E_PORT;
    }

    /* Construct lookup key */
    sal_memset(&key_data, 0, sizeof(vlan_xlate_entry_t));

    if (BCM_VLAN_VALID(EXTENDER_PORT_INFO(unit, vp)->match_vlan)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF_VLAN, &key_type));
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__VLANf,
                EXTENDER_PORT_INFO(unit, vp)->match_vlan);
    } else {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF, &key_type));
    }
    soc_VLAN_XLATEm_field32_set(unit, &key_data, KEY_TYPEf, key_type);

    soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__SRC_VIFf,
            EXTENDER_PORT_INFO(unit, vp)->extended_port_vid);

    if (soc_mem_field_valid(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, SOURCE_TYPEf, 1);
    }
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, EXTENDER_PORT_INFO(unit, vp)->port,
                                &mod_out, &port_out, &trunk_out, &tmp_id));
    if (BCM_GPORT_IS_TRUNK(EXTENDER_PORT_INFO(unit, vp)->port)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__TGIDf, trunk_out);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__PORT_NUMf, port_out);
    }

    /* Lookup existing entry */
    SOC_IF_ERROR_RETURN(soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
                &entry_index, &key_data, &entry_data, 0));

    /* Get entry's VLAN */
    *vid = soc_VLAN_XLATEm_field32_get(unit, &entry_data, VIF__NEW_OVIDf);

    return rv;
}

#ifdef BCM_TRIDENT2_SUPPORT

/*
 * Function:
 *      _bcm_td2_extender_match_vp_replace
 * Purpose:
 *      Replace VP value in Extender VP's match entry.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vp - (IN) Extender VP whose match entry is being replaced.
 *      new_vp - (IN) New VP value.
 *      old_vp - (OUT) Old VP value.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_td2_extender_match_vp_replace(int unit, int vp, int new_vp, int *old_vp)
{
    int                rv = BCM_E_NONE;
    vlan_xlate_entry_t key_data, entry_data;
    int                key_type;
    bcm_module_t       mod_out;
    bcm_port_t         port_out;
    bcm_trunk_t        trunk_out;
    int                tmp_id;
    int                entry_index;

    /* Construct lookup key */
    sal_memset(&key_data, 0, sizeof(vlan_xlate_entry_t));
    
    if (BCM_VLAN_VALID(EXTENDER_PORT_INFO(unit, vp)->match_vlan)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF_VLAN, &key_type));
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__VLANf,
                EXTENDER_PORT_INFO(unit, vp)->match_vlan);
    } else {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF, &key_type));
    }
    soc_VLAN_XLATEm_field32_set(unit, &key_data, KEY_TYPEf, key_type);
    soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__SRC_VIFf,
            EXTENDER_PORT_INFO(unit, vp)->extended_port_vid);
    if (soc_mem_field_valid(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, SOURCE_TYPEf, 1);
    }
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, EXTENDER_PORT_INFO(unit, vp)->port,
                                &mod_out, &port_out, &trunk_out, &tmp_id));
    if (BCM_GPORT_IS_TRUNK(EXTENDER_PORT_INFO(unit, vp)->port)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__TGIDf, trunk_out);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__PORT_NUMf, port_out);
    }

    /* Lookup existing entry */
    SOC_IF_ERROR_RETURN(soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
                &entry_index, &key_data, &entry_data, 0));

    /* Replace entry's VP value */
    *old_vp = soc_VLAN_XLATEm_field32_get(unit, &entry_data, VIF__SOURCE_VPf);
    soc_VLAN_XLATEm_field32_set(unit, &entry_data, VIF__SOURCE_VPf, new_vp);

    /* Insert new entry */
    rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &entry_data);
    if (rv == SOC_E_EXISTS) {
        rv = BCM_E_NONE;
    } else {
        return BCM_E_INTERNAL;
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_extender_port_source_vp_lag_set
 * Purpose:
 *      Set source VP LAG for an Extender virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) Extender virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_td2_extender_port_source_vp_lag_set(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
    int vp, old_vp;

    if (!BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        return BCM_E_PARAM;
    }
    vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
        return BCM_E_PARAM;
    }

    /* Set source VP LAG by replacing the SVP field in Extender VP's
     * match entry with the VP value representing the VP LAG.
     */
    BCM_IF_ERROR_RETURN
        (_bcm_td2_extender_match_vp_replace(unit, vp, vp_lag_vp, &old_vp));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_extender_port_source_vp_lag_clear
 * Purpose:
 *      Clear source VP LAG for an Extender virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) Extender virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_td2_extender_port_source_vp_lag_clear(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
    int vp, old_vp;

    if (!BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        return BCM_E_PARAM;
    }
    vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
        return BCM_E_PARAM;
    }

    /* Clear source VP LAG by replacing the SVP field in Extender VP's
     * match entry with the VP value.
     */
    BCM_IF_ERROR_RETURN
        (_bcm_td2_extender_match_vp_replace(unit, vp, vp, &old_vp));

    /* Check that the old VP value matches the VP value representing
     * the VP LAG.
     */
    if (old_vp != vp_lag_vp) {
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_extender_port_source_vp_lag_get
 * Purpose:
 *      Set source VP LAG for an Extender virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) Extender virtual port GPORT ID. 
 *      vp_lag_vp - (OUT) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_td2_extender_port_source_vp_lag_get(int unit, bcm_gport_t gport,
        int *vp_lag_vp)
{
    int                vp;
    vlan_xlate_entry_t key_data, entry_data;
    int                key_type;
    bcm_module_t       mod_out;
    bcm_port_t         port_out;
    bcm_trunk_t        trunk_out;
    int                tmp_id;
    int                entry_index;

    if (!BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        return BCM_E_PARAM;
    }
    vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
        return BCM_E_PARAM;
    }

    /* Construct match entry lookup key */
    sal_memset(&key_data, 0, sizeof(vlan_xlate_entry_t));

    if (BCM_VLAN_VALID(EXTENDER_PORT_INFO(unit, vp)->match_vlan)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF_VLAN, &key_type));
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__VLANf,
                EXTENDER_PORT_INFO(unit, vp)->match_vlan);
    } else {
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                    VLXLT_HASH_KEY_TYPE_VIF, &key_type));
    }
    soc_VLAN_XLATEm_field32_set(unit, &key_data, KEY_TYPEf, key_type);
    soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__SRC_VIFf,
            EXTENDER_PORT_INFO(unit, vp)->extended_port_vid);
    if (soc_mem_field_valid(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, SOURCE_TYPEf, 1);
    }
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, EXTENDER_PORT_INFO(unit, vp)->port,
                                &mod_out, &port_out, &trunk_out, &tmp_id));
    if (BCM_GPORT_IS_TRUNK(EXTENDER_PORT_INFO(unit, vp)->port)) {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__TGIDf, trunk_out);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &key_data, VIF__PORT_NUMf, port_out);
    }

    /* Lookup existing entry */
    SOC_IF_ERROR_RETURN(soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
                &entry_index, &key_data, &entry_data, 0));

    /* Get VP value representing VP LAG */
    *vp_lag_vp = soc_VLAN_XLATEm_field32_get(unit, &entry_data, VIF__SOURCE_VPf);
    if (!_bcm_vp_used_get(unit, *vp_lag_vp, _bcmVpTypeVpLag)) {
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_tr3_extender_sw_dump
 * Purpose:
 *     Displays Extender module information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
bcm_tr3_extender_sw_dump(int unit)
{
    int i, num_vp;

    soc_cm_print("\nSW Information Extender - Unit %d\n", unit);

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    soc_cm_print("\n  Port Info    : \n");
    for (i = 0; i < num_vp; i++) {
        if (EXTENDER_PORT_INFO(unit, i)->port == 0) {
            continue;
        }
        soc_cm_print("\n  Extender port vp = %d\n", i);
        soc_cm_print("Flags = 0x%x\n", EXTENDER_PORT_INFO(unit, i)->flags);
        soc_cm_print("Port = 0x%x\n", EXTENDER_PORT_INFO(unit, i)->port);
        soc_cm_print("ETAG VID = 0x%x\n",
                EXTENDER_PORT_INFO(unit, i)->extended_port_vid);
        soc_cm_print("PCP DE Select = 0x%x\n", 
                EXTENDER_PORT_INFO(unit, i)->pcp_de_select);
        soc_cm_print("Default PCP = 0x%x\n", EXTENDER_PORT_INFO(unit, i)->pcp);
        soc_cm_print("Default DE = 0x%x\n", EXTENDER_PORT_INFO(unit, i)->de);
        soc_cm_print("Match VLAN = 0x%x\n", EXTENDER_PORT_INFO(unit, i)->match_vlan);
    }

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */
