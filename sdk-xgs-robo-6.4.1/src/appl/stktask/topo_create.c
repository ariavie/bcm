/* 
 * $Id: topo_create.c,v 1.4 Broadcom SDK $
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
 * File:        topo_create.c
 * Purpose:     Local topology calculator
 *
 */

#include <assert.h>
#include <bcm/error.h>
#include <bcm/types.h>
#include <appl/cpudb/cpudb.h>
#include <appl/stktask/topology.h>
#include <appl/stktask/topology.h>
#include "topo_int.h"

/*
 * Function:
 *     _cpudb_entry_modid_get(cpudb_entry_t *entry,
 *                            uint8 *modp, int max, int *lenp)
 * Purpose:
 *     Fill an array of modids assigned to a CPU entry
 * Parameters:
 *     entry - (IN) CPUDB entry
 *     modp  - (OUT) array of modids to fill
 *     max   - (IN) maximum length of array
 *     lenp  - (OUT) actual length of array
 * Returns:
 *     BCM_E_NONE     - no errors
 *     BCM_E_FULL     - too many modids for CPUDB
 *     BCM_E_PARAM    - db_ref NULL, db_ref not fully constructed
 *     BCM_E_MEMORY   - not enough memory or system resources
 *     BCM_E_FAIL     - topology calculation failed (CPUDB incomplete)
 *     BCM_E_RESOURCE - not enough modids available for topology
 * Notes:
 *     Modids must already be assigned.
 *
 *     Doesn't bother with the stackport flags, as it's only used
 *     internally to set the cpudb sp_info flags.
 *
 */

STATIC int
_cpudb_entry_modid_get(cpudb_entry_t *entry, uint8 *modp, int max, int *lenp)
{
    int i;
    int count;
    int mod, nmod;

    count = 0;
    for (i = 0; i < entry->base.num_units; i++) {
        mod = entry->mod_ids[i];
        for (nmod = 0; nmod < entry->base.mod_ids_req[i]; nmod++) {
            if (count < max) {
                *modp++ = mod + nmod;
                count++;
            } else {
                return BCM_E_FULL;
            }
        }
    }
    *lenp = count;

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_stack_topo_cpu(cpudb_ref_t db_ref, topo_cpu_t *tp_cpu)
 * Purpose:
 *     Calculate local topology from db_ref, returning topo_cpu
 *     and updating db_ref.
 * Parameters:
 *     db_ref - (IN/OUT) CPU database
 *     tp_cpu - (OUT) Calculated topology
 * Returns:
 *     BCM_E_NONE     - no errors
 *     BCM_E_FULL     - too many modids for CPUDB
 *     BCM_E_PARAM    - db_ref NULL, db_ref not fully constructed
 *     BCM_E_MEMORY   - not enough memory or system resources
 *     BCM_E_FAIL     - topology calculation failed (CPUDB incomplete)
 *     BCM_E_RESOURCE - not enough modids available for topology
 * Notes:
 *     Modids must already be assigned.
 *
 *     Doesn't bother with the stackport flags, as it's only used
 *     internally to set the cpudb sp_info flags.
 *
 */

STATIC int
_bcm_stack_topo_cpu(cpudb_ref_t db_ref, topo_cpu_t *tp_cpu)
{
    cpudb_entry_t *local, *entry;
    int sp, dst, mcount, mleft, mtotal;
    uint8 *mptr;
    int rv = BCM_E_FAIL;

    if (!cpudb_valid(db_ref)) {
        return BCM_E_PARAM;
    }

    if (tp_cpu == NULL) {
        return BCM_E_PARAM;
    }

    if (db_ref->topo_cookie == NULL) {
        return BCM_E_PARAM;
    }

    local = db_ref->local_entry;
    sal_memset(tp_cpu, 0, sizeof(*tp_cpu));
    tp_cpu->local_entry = *local;
    tp_cpu->master_seq_num = db_ref->master_entry->base.dseq_num;
    tp_cpu->num_cpus = db_ref->num_cpus;

    dst=0;
    
    CPUDB_FOREACH_ENTRY(db_ref, entry) {
        if (dst != local->topo_idx) {

            /* Get the TX stack port on this CPU that points to dst */
            sp = TP_TX_CXN(db_ref, local->topo_idx, dst);

            /* Append the modids associated with this stack port */
            mtotal = tp_cpu->tp_stk_port[sp].tx_mod_num;
            mleft = COUNTOF(tp_cpu->tp_stk_port[sp].tx_mods) - mtotal;
            if (mleft <= 0) {
                rv = BCM_E_FULL;
                break;
            }
            mptr = tp_cpu->tp_stk_port[sp].tx_mods + mtotal;
            rv = _cpudb_entry_modid_get(entry, mptr, mleft, &mcount);
            if (BCM_FAILURE(rv)) {
                break;
            }
            tp_cpu->tp_stk_port[sp].tx_mod_num += mcount;

            
            entry->dest_mod = entry->mod_ids[entry->base.dest_unit];
            entry->dest_port = entry->base.dest_port;
            entry->tx_unit = local->base.stk_ports[sp].unit;
            entry->tx_port = local->base.stk_ports[sp].port;
            entry->flags |= (CPUDB_F_TX_KNOWN|CPUDB_F_DEST_KNOWN);

            /* Get the RX stack port on this CPU that points to dst */
            sp = TP_RX_CXN(db_ref, local->topo_idx, dst);

            /* Append the modids associated with this stack port */
            mtotal = tp_cpu->tp_stk_port[sp].rx_mod_num;
            mleft = COUNTOF(tp_cpu->tp_stk_port[sp].rx_mods) - mtotal;
            if (mleft <= 0) {
                rv = BCM_E_FULL;
                break;
            }
            mptr = tp_cpu->tp_stk_port[sp].rx_mods + mtotal;
            rv = _cpudb_entry_modid_get(entry, mptr, mleft, &mcount);
            if (BCM_FAILURE(rv)) {
                break;
            }
            tp_cpu->tp_stk_port[sp].rx_mod_num += mcount;

        } else {
            /* local entry */
            local->dest_mod = local->mod_ids[local->base.dest_unit];
            local->dest_port = local->base.dest_port;
            local->tx_unit = local->base.dest_unit;
            local->tx_port = local->base.dest_port;
            local->flags |= (CPUDB_F_TX_KNOWN|CPUDB_F_DEST_KNOWN);

            if (db_ref->num_cpus == 1) {
                /* Nothing more to do */
                rv = BCM_E_NONE;
            }
        }
        dst++; /* Next dst id */
    }

    return rv;
}

/*
 * Function:
 *     bcm_stack_topo_create(cpudb_ref_t db_ref, topo_cpu_t *tp_cpu)
 * Purpose:
 *     Calculate local topology from db_ref, returning topo_cpu
 *     and updating db_ref.
 * Parameters:
 *     db_ref - (IN/OUT) CPU database
 *     tp_cpu - (OUT) Calculated topology
 * Returns:
 *     BCM_E_NONE     - no errors
 *     BCM_E_FULL     - too many modids for CPUDB
 *     BCM_E_PARAM    - db_ref NULL, db_ref not fully constructed
 *     BCM_E_MEMORY   - not enough memory or system resources
 *     BCM_E_FAIL     - topology calculation failed (CPUDB incomplete)
 *     BCM_E_RESOURCE - not enough modids available for topology
 * Notes:
 *     Does not perform any device programming.
 */

int
bcm_stack_topo_create(cpudb_ref_t db_ref, topo_cpu_t *tp_cpu)
{
    BCM_IF_ERROR_RETURN(topology_mod_ids_assign(db_ref));
    BCM_IF_ERROR_RETURN(topology_create(db_ref));
    BCM_IF_ERROR_RETURN(_bcm_stack_topo_cpu(db_ref,tp_cpu));

    return BCM_E_NONE;
}
