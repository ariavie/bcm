/*
 * $Id: qos.c,v 1.32 Broadcom SDK $
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
 * All Rights Reserved.$
 *
 * QoS module
 */
#include <sal/core/libc.h>

#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/l2u.h>
#include <soc/util.h>
#include <soc/debug.h>
#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/port.h>
#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/ipmc.h>
#include <bcm/qos.h>
#include <bcm/stack.h>
#include <bcm/topo.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/xgs3.h>
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
#include <bcm_int/esw/triumph.h>
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

/* Initialize the QoS module. */
int 
bcm_esw_qos_init(int unit)
{
    int rv = BCM_E_NONE;

#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {

        rv = bcm_tr2_qos_init(unit);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_init(unit);
    }
#endif
#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) ||
        SOC_IS_TRIUMPH3(unit)) {
        egr_map_mh_entry_t mapmh;
        soc_mem_t mem;
        uint32 idx, max;

        mem = EGR_MAP_MHm;
        idx = soc_mem_index_min(unit, mem);
        max = soc_mem_index_max(unit, mem);
        sal_memset(&mapmh, 0, sizeof(mapmh));
        soc_mem_lock(unit, mem);
        while (BCM_SUCCESS(rv) && idx < max) {
            soc_mem_field32_set(unit, mem, &mapmh, HG_TCf, idx & 0xf);
            rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, idx, &mapmh);
            idx++;
        }
        soc_mem_unlock(unit, mem);
    }
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIDENT2(unit)) {
        rv = bcm_td2_qos_init(unit);
    }
#endif
    return rv;
}

/* Detach the QoS module. */
int 
bcm_esw_qos_detach(int unit)
{
    int rv = BCM_E_NONE;
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = bcm_tr2_qos_detach(unit);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_detach(unit);
    }
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIDENT2(unit)) {
        rv = bcm_td2_qos_detach(unit);
    }
#endif

    return rv;
}

/* Create a QoS map profile */
int 
bcm_esw_qos_map_create(int unit, uint32 flags, int *map_id)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_HURRICANE_SUPPORT)
    if ((SOC_IS_HURRICANEX(unit)) && (flags & BCM_QOS_MAP_MPLS)) {
        return rv;
    }
#endif

#if defined(BCM_GREYHOUND_SUPPORT)
    if ((SOC_IS_GREYHOUND(unit)) && (flags & BCM_QOS_MAP_MPLS)) {
        return rv;
    }
#endif


#ifdef BCM_TRIDENT2_SUPPORT
    if ((QOS_FLAGS_ARE_FCOE(flags) || (flags & BCM_QOS_MAP_L2_VLAN_ETAG)) 
        && SOC_IS_TRIDENT2(unit)) {
        return bcm_td2_qos_map_create(unit, flags, map_id);
    }
#endif

#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = bcm_tr2_qos_map_create(unit, flags, map_id);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_map_create(unit, flags, map_id);
    }
#endif
    return rv;
}

/* Destroy a QoS map profile */
int 
bcm_esw_qos_map_destroy(int unit, int map_id)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)&& (QOS_MAP_IS_FCOE(map_id) 
        || QOS_MAP_IS_L2_VLAN_ETAG(map_id))) {
        return bcm_td2_qos_map_destroy(unit, map_id);
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = bcm_tr2_qos_map_destroy(unit, map_id);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_map_destroy(unit, map_id);
    }
#endif
    return rv;
}

/* Add an entry to a QoS map */
int 
bcm_esw_qos_map_add(int unit, uint32 flags, bcm_qos_map_t *map, int map_id)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)&& (QOS_MAP_IS_FCOE(map_id) 
        || QOS_MAP_IS_L2_VLAN_ETAG(map_id))) {
        return bcm_td2_qos_map_add(unit, flags, map, map_id);
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = bcm_tr2_qos_map_add(unit, flags, map, map_id);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_map_add(unit, flags, map, map_id);
    }
#endif
    return rv;
}

/* bcm_esw_qos_map_multi_get */
int
bcm_esw_qos_map_multi_get(int unit, uint32 flags,
                          int map_id, int array_size, 
                          bcm_qos_map_t *array, int *array_count)
{
    int rv = BCM_E_UNAVAIL;

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)&& QOS_MAP_IS_L2_VLAN_ETAG(map_id)) {
        return bcm_td2_qos_map_multi_get(unit, flags, map_id, array_size, array, 
                                       array_count);
    }
#endif
    
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = bcm_tr2_qos_map_multi_get(unit, flags, map_id, array_size, array, 
                                       array_count);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_map_multi_get(unit, flags, map_id, array_size, array, 
                                       array_count);
    }
#endif
    return rv;
}

/* Clear an entry from a QoS map */
int 
bcm_esw_qos_map_delete(int unit, uint32 flags, bcm_qos_map_t *map, int map_id)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)&& (QOS_MAP_IS_FCOE(map_id) 
        || QOS_MAP_IS_L2_VLAN_ETAG(map_id))) {
        return bcm_td2_qos_map_delete(unit, flags, map, map_id);
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = bcm_tr2_qos_map_delete(unit, flags, map, map_id);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_map_delete(unit, flags, map, map_id);
    }
#endif
    return rv;
}

/* Attach a QoS map to an object (Gport) */
int 
bcm_esw_qos_port_map_set(int unit, bcm_gport_t port, int ing_map, int egr_map)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit) 
        && (QOS_MAP_IS_FCOE(ing_map) || QOS_MAP_IS_L2_VLAN_ETAG(ing_map) 
        || QOS_MAP_IS_FCOE(egr_map))) {
        return bcm_td2_qos_port_map_set(unit, port, ing_map, egr_map);
    }
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {

        rv = bcm_tr2_qos_port_map_set(unit, port, ing_map, egr_map);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_port_map_set(unit, port, ing_map, egr_map);
    }
#endif
    return rv;
}

/* bcm_qos_port_map_get */
int
bcm_esw_qos_port_map_get(int unit, bcm_gport_t port, 
                         int *ing_map, int *egr_map)
{
    return BCM_E_UNAVAIL;
}

/* bcm_qos_port_vlan_map_set */
int
bcm_esw_qos_port_vlan_map_set(int unit,  bcm_port_t port, bcm_vlan_t vid,
                              int ing_map, int egr_map)
{
    int rv = BCM_E_UNAVAIL;
    
#if defined(BCM_TRIUMPH2_SUPPORT)
    if ( SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD_TT(unit)) {
        rv = bcm_tr2_qos_port_vlan_map_set(unit, port, vid, ing_map, egr_map);
    }
#endif

    return rv;
}

/* bcm_qos_port_vlan_map_get */
int
bcm_esw_qos_port_vlan_map_get(int unit, bcm_port_t port, bcm_vlan_t vid,
                              int *ing_map, int *egr_map)
{
    int rv = BCM_E_UNAVAIL;
    
#if defined(BCM_TRIUMPH2_SUPPORT)
    if ( SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD_TT(unit)) {
        rv = bcm_tr2_qos_port_vlan_map_get(unit, port, vid, ing_map, egr_map);
    }
#endif
    return rv;

}

/* bcm_qos_multi_get */
int
bcm_esw_qos_multi_get(int unit, int array_size, int *map_ids_array, 
                      int *flags_array, int *array_count)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = bcm_tr2_qos_multi_get(unit, array_size, map_ids_array, 
                                   flags_array, array_count);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = bcm_tr_qos_multi_get(unit, array_size, map_ids_array, 
                                   flags_array, array_count);
    }
#endif
    return rv;
}

#ifdef BCM_WARM_BOOT_SUPPORT
int
_bcm_esw_qos_sync(int unit)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        rv = _bcm_tr2_qos_sync(unit);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        rv = _bcm_tr_qos_sync(unit);
    }
#endif
    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
void
_bcm_esw_qos_sw_dump(int unit)
{
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)||
        SOC_IS_GREYHOUND(unit)) {
        _bcm_tr2_qos_sw_dump(unit);
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        _bcm_tr_qos_sw_dump(unit);
    }
#endif
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
