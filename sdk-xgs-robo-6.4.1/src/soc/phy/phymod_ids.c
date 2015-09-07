/*
 * $Id: phymod_ids.c,v 1.1.2.2 Broadcom SDK $
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
 * File:        phymod_ids.c
 * Purpose:     These routines and structures are related to
 *              figuring out core and phy IDs correlating specific device
 */

#include <soc/drv.h>

/*
 * Function:
 *      soc_phymod_default_core_id_get
 * Purpose:
 *      Return the default PHY core_id and phy_ids for logical port
 * Parameters:
 *      unit - StrataSwitch unit number
 *      port - logical port numer
 *      max_phys - max size of phy_ids
 *      core_id - (OUT) returned core_id
 *      phy_ids - (OUT) returned phy_ids
 *  
 */

STATIC int
soc_phymod_default_core_id_get(int unit, int port, uint32 max_cores, uint32 max_phys,
                               uint32 *num_cores, uint32 *core_id, uint32 *phy_ids)
{
    int num_lanes_in_core = 4; /* This can be ovveride per unit, 
                                  in each unit must keep constant */
    soc_info_t *si;
    int phy_port;  
    int phy_ids_index = 0;
    int cid, pid;
    int in_core_offset;

    si = &SOC_INFO(unit);

    /* Calculate num of cores */
    (*num_cores) = (si->port_num_lanes[port] + num_lanes_in_core - 1) / num_lanes_in_core;

    /* Validate array sizes */
    if(max_cores < (*num_cores)) {
        return SOC_E_INTERNAL;
    }

    if(max_phys < (*num_cores) * num_lanes_in_core) {
        return SOC_E_INTERNAL;
    }

    /* Get first physical port */
    if (soc_feature(unit, soc_feature_logical_port_num)) {
        phy_port = si->port_l2p_mapping[port];
    } else {
        phy_port = port;
    }
    in_core_offset = SOC_PORT_BINDEX(unit, phy_port) % num_lanes_in_core;

    /* Set IDs */
    for(cid = 0 ; cid<(*num_cores) ; cid++) {
        core_id[cid] = in_core_offset / num_lanes_in_core + cid;

        for(pid=0 ; pid<num_lanes_in_core ; pid++) {
            /* core offset * max lanes in core + lane offset */
            phy_ids[phy_ids_index] = core_id[cid] * num_lanes_in_core + pid;
            phy_ids_index++;
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phymod_core_id_get
 * Purpose:
 *      Return the PHY core_id and phy_ids for logical port
 * Parameters:
 *      unit - StrataSwitch unit number
 *      port - logical port numer
 *      max_phys - max size of phy_ids
 *      core_id - (OUT) returned core_id
 *      phy_ids - (OUT) returned phy_ids
 *  
 * Note: 
 *      The function contains default indexing for core ids and phy ids
 *      Assuming there are maximum 4 lanes per core
 *      Devices that need other calculation required to implement inside their own indexing
 */
int
soc_phymod_core_id_get(int unit, int port, uint32 max_cores, uint32 max_phys,
                       uint32 *num_cores, uint32 *core_id, uint32 *phy_ids)
{
    return soc_phymod_default_core_id_get(unit, port, max_cores, max_phys,
                                          num_cores, core_id, phy_ids);
}

/*
 * Function:
 *      soc_phymod_broadcast_id_offset
 * Purpose:
 *      Return the first broadcast_id.
 * Parameters:
 *      unit - StrataSwitch unit number
 *      broadcast_id_offset - (OUT) first allowed broadcast if
 *  
 * Note: 
 *      The actual broadcast ID will be broadcast_id_offset + phy_id
 */
int 
soc_phymod_broadcast_id_offset(int unit, uint32 *broadcast_id_offset)
{
    *broadcast_id_offset = 400;

    return SOC_E_NONE;
}
