/*
 * $Id: ipmc.c,v 1.14 Broadcom SDK $
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
 * File: 	ipmc.c
 * Purpose: 	Tracks and manages IPMC tables.
 */

#ifdef INCLUDE_L3

#include <soc/drv.h>
#include <soc/mem.h>

#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/hercules.h>

int
bcm_hercules_ipmc_init(int unit)
{
    if (SOC_IS_HERCULES15(unit)) {
	return soc_mem_clear(unit, MEM_IPMCm, MEM_BLOCK_ALL, 0);
    }

#ifdef BCM_HUMV_SUPPORT
    if (SOC_IS_HUMV(unit)) {
	ipmc_entry_t    ent;
	int	        i, imin, imax;

	imin = soc_mem_index_min(unit, L3_IPMCm);
	imax = soc_mem_index_max(unit, L3_IPMCm);

        sal_memset(&ent, 0, sizeof(ent));
        soc_L3_IPMCm_field32_set(unit, &ent, VALIDf, 1);
        for (i = imin; i <= imax; i++) {
            SOC_IF_ERROR_RETURN
                (WRITE_L3_IPMCm(unit, SOC_BLOCK_ALL, i, &ent));
	}
	return BCM_E_NONE;
    }
#endif

    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_detach(int unit)
{
    /* Just clear tables, as done in init */
    return bcm_hercules_ipmc_init(unit);
}

int
bcm_hercules_ipmc_get(int unit, int index, bcm_ipmc_addr_t *ipmc)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_lookup(int unit, int *index, bcm_ipmc_addr_t *ipmc)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_add(int unit, bcm_ipmc_addr_t *ipmc)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_put(int unit, int index, bcm_ipmc_addr_t *ipmc)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_delete(int unit, bcm_ipmc_addr_t *ipmc)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_delete_all(int unit)
{
    return bcm_hercules_ipmc_init(unit);
}

int
bcm_hercules_ipmc_age(int unit, uint32 flags, bcm_ipmc_traverse_cb age_out,
                      void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_ipmc_traverse(int unit, uint32 flags, bcm_ipmc_traverse_cb cb, 
                           void *user_data)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_ipmc_enable(int unit, int enable)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_src_port_check(int unit, int enable)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_src_ip_search(int unit, int enable)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_egress_port_set(int unit, bcm_port_t port, 
                                const bcm_mac_t mac,  int untag, 
                                bcm_vlan_t vid, int ttl_thresh)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_egress_port_get(int unit, bcm_port_t port, sal_mac_addr_t mac,
                         int *untag, bcm_vlan_t *vid, int *ttl_thresh)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_repl_init(int unit)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_repl_detach(int unit)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_repl_get(int unit, int index, bcm_port_t port, 
                           bcm_vlan_vector_t vlan_vec)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_repl_add(int unit, int index, bcm_port_t port, 
                           bcm_vlan_t vlan)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_repl_delete(int unit, int index, bcm_port_t port, 
                              bcm_vlan_t vlan)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_repl_delete_all(int unit, int index, bcm_port_t port)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_egress_intf_add(int unit, int index, bcm_port_t port, 
                                  bcm_l3_intf_t *l3_intf)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_ipmc_egress_intf_delete(int unit, int index, bcm_port_t port, 
                                     bcm_l3_intf_t *l3_intf)
{
    return BCM_E_UNAVAIL;
}
#endif	/* INCLUDE_L3 */

int _bcm_esw_hercules_ipmc_not_empty;
