/*
 * $Id: mcast.c,v 1.14 Broadcom SDK $
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
 * File:        mcast.c
 * Purpose:     Tracks and manages L2 Multicast tables.
 *
 * NOTE: These are empty stubs because the standard bcm_mcast layer
 * calls are not intended for programming a fabric chip.
 * Fabric device mcast programming should use the bcm_mcast_bitmap_* APIs.
 */

#include <soc/drv.h>
#include <soc/mem.h>

#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>


int
bcm_hercules_mcast_addr_add_w_l2mcindex(int unit,bcm_mcast_addr_t *mcaddr)
{
  return (BCM_E_UNAVAIL);
}

int
bcm_hercules_mcast_addr_remove_w_l2mcindex(int unit, bcm_mcast_addr_t *mcaddr)
{
  return (BCM_E_UNAVAIL);
}

int
bcm_hercules_mcast_port_add(int unit, bcm_mcast_addr_t *mcaddr)
{
  return (BCM_E_UNAVAIL);
}

int
bcm_hercules_mcast_port_remove(int unit, bcm_mcast_addr_t *mcaddr)
{
  return (BCM_E_UNAVAIL);
}

int
bcm_hercules_mcast_addr_add(int unit, bcm_mcast_addr_t *mcaddr)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_mcast_addr_remove(int unit, sal_mac_addr_t mac, bcm_vlan_t vid)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_mcast_port_get(int unit,
                            sal_mac_addr_t mac, bcm_vlan_t vid,
                            bcm_mcast_addr_t *mcaddr)
{
    return (BCM_E_UNAVAIL);
}

int
_bcm_hercules_mcast_detach(int unit)
{
    return BCM_E_NONE;
}

int
bcm_hercules_mcast_init(int unit)
{
    if (SOC_IS_HERCULES15(unit)) {
        return soc_mem_clear(unit, MEM_MCm, MEM_BLOCK_ALL, 0);
    }

    return BCM_E_UNAVAIL;
}
