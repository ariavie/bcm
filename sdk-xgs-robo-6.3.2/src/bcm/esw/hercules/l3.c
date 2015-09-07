/*
 * $Id: l3.c 1.24 Broadcom SDK $
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
 * File: 	l3.c
 * Purpose: 	Tracks and manages l3 routing tables and interfaces.
 */

#ifdef INCLUDE_L3

#include <soc/drv.h>
#include <soc/mem.h>

#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/port.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/l3.h>

int
bcm_hercules_l3_tables_init(int unit)
{    
    return BCM_E_NONE;
}

int
bcm_hercules_l3_tables_cleanup(int unit)
{    
    return BCM_E_NONE;
}

int
bcm_hercules_l3_enable(int unit, int enable)
{
    return BCM_E_NONE;
}

int
bcm_hercules_l3_intf_get(int unit, _bcm_l3_intf_cfg_t *intf_info)
{    
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_intf_get_by_vid(int unit, _bcm_l3_intf_cfg_t *intf_info)
{    
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_intf_create(int unit, _bcm_l3_intf_cfg_t *intf_info)
{    
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_intf_id_create(int unit, _bcm_l3_intf_cfg_t *intf_info)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_intf_lookup(int unit, _bcm_l3_intf_cfg_t *l3i)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_intf_del(int unit, _bcm_l3_intf_cfg_t *intf_info)
{    
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_intf_del_all(int unit)
{    
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_get(int unit, int flag, _bcm_l3_cfg_t *l3cfg)
{    
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_add(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_del(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_del_prefix(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_del_intf(int unit, _bcm_l3_cfg_t *l3cfg, int negate)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_del_all(int unit)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_replace(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_conflict_get(int unit, bcm_l3_key_t *ipkey,
			     bcm_l3_key_t *cf_array, int cf_max, int *cf_count)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_defip_cfg_get(int unit, _bcm_defip_cfg_t *defip)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_defip_ecmp_get_all(int unit, _bcm_defip_cfg_t *defip,
           bcm_l3_route_t *path_array, int max_path, int *path_count)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_defip_add(int unit, _bcm_defip_cfg_t *defip)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_defip_del(int unit, _bcm_defip_cfg_t *defip)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_defip_del_intf(int unit, _bcm_defip_cfg_t *defip, int negate)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_defip_del_all(int unit)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_age(int unit, uint32 flags, bcm_l3_host_traverse_cb age_out,
                    void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_traverse(int unit, uint32 start, uint32 end,
                      bcm_l3_host_traverse_cb cb, void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_info(int unit, bcm_l3_info_t *l3info)
{
    l3info->l3info_max_intf = 0;
    l3info->l3info_max_host = 0;
    l3info->l3info_max_route = 0;
    l3info->l3info_occupied_intf = 0;
    l3info->l3info_occupied_host = 0;
    l3info->l3info_occupied_route = 0;
    l3info->l3info_max_lpm_block = 0;
    l3info->l3info_used_lpm_block = 0;

    /* Superseded values for backward compatibility */

    l3info->l3info_max_l3         = l3info->l3info_max_host;
    l3info->l3info_max_defip      = l3info->l3info_max_route;
    l3info->l3info_occupied_l3    = l3info->l3info_occupied_host;
    l3info->l3info_occupied_defip = l3info->l3info_occupied_route;

    return (BCM_E_NONE);
}

int
bcm_hercules_defip_age(int unit, bcm_l3_route_traverse_cb age_out,
                       void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_defip_traverse(int unit, uint32 start, uint32 end,
                      bcm_l3_route_traverse_cb trav_fn, void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l3_ip6_get(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_l3_ip6_add(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_l3_ip6_del(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_l3_ip6_del_prefix(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_l3_ip6_replace(int unit, _bcm_l3_cfg_t *l3cfg)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_l3_ip6_traverse(int unit, uint32 start, uint32 end,
                      bcm_l3_host_traverse_cb cb, void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_ip6_defip_cfg_get(int unit, _bcm_defip_cfg_t *defip)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ip6_defip_ecmp_get_all(int unit, _bcm_defip_cfg_t *defip,
           bcm_l3_route_t *path_array, int max_path, int *path_count)
{ 
    return (BCM_E_UNAVAIL);
} 


int
bcm_hercules_ip6_defip_traverse(int unit, uint32 start, uint32 end,
                      bcm_l3_route_traverse_cb trav_fn, void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_ip6_defip_add(int unit, _bcm_defip_cfg_t *defip)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ip6_defip_del(int unit, _bcm_defip_cfg_t *defip)
{
    return BCM_E_UNAVAIL;
}

#endif	/* INCLUDE_L3 */

int _bcm_esw_hercules_l3_not_empty;
