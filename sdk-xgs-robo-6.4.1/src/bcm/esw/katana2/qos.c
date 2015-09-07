/*
 * $Id: f2b99655c560f92519af804d44c60c3c648ce0aa $
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
 * File:        qos.c
 * Purpose:     Katana2 QoS functions
 */

#include <soc/mem.h>
#include <bcm/error.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/katana2.h>

#ifdef BCM_KATANA2_SUPPORT

#undef DSCP_CODE_POINT_CNT
#define DSCP_CODE_POINT_CNT 64
#undef DSCP_CODE_POINT_MAX
#define DSCP_CODE_POINT_MAX (DSCP_CODE_POINT_CNT - 1)
#define _BCM_QOS_MAP_CHUNK_DSCP  64

 /* Function:
 *      bcm_kt2_port_dscp_map_set
 * Purpose:
 *      Internal implementation for bcm_kt2_port_dscp_map_set
 * Parameters:
 *      unit - switch device
 *      port - switch port or -1 for global table
 *      srccp - src code point or -1
 *      mapcp - mapped code point or -1
 *      prio - priority value for mapped code point
 *              -1 to use port default untagged priority
 *              BCM_PRIO_RED    can be or'ed into the priority
 *              BCM_PRIO_YELLOW can be or'ed into the priority
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_kt2_port_dscp_map_set(int unit, bcm_port_t port, int srccp,
                            int mapcp, int prio, int cng)
{
    port_tab_entry_t         pent;
    void                     *entries[2];
    uint32                   old_profile_index = 0;
    int                      index = 0;
    int                      rv = BCM_E_NONE;
    dscp_table_entry_t       dscp_table[_BCM_QOS_MAP_CHUNK_DSCP];
    int                      offset = 0,i = 0;
    void                     *entry;

    /* Lock the port table */
    soc_mem_lock(unit, PORT_TABm); 

    /* Get profile index from port table. */
    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &pent);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, PORT_TABm);
        return rv;
    }

    old_profile_index =
        soc_mem_field32_get(unit, PORT_TABm, &pent, TRUST_DSCP_PTRf)
            * DSCP_CODE_POINT_CNT;

    sal_memset(dscp_table, 0, sizeof(dscp_table));

    /* Base index of table in hardware */

    entries[0] = &dscp_table;

    rv =  _bcm_dscp_table_entry_get(unit, old_profile_index,
            _BCM_QOS_MAP_CHUNK_DSCP, entries);
    offset = srccp;
    /* Modify what's needed */

    if (offset < 0) {
        for (i = 0; i <= DSCP_CODE_POINT_MAX; i++) {
             entry = &dscp_table[i];
             soc_DSCP_TABLEm_field32_set(unit, entry, DSCPf, mapcp);
             soc_DSCP_TABLEm_field32_set(unit, entry, PRIf, prio);
             soc_DSCP_TABLEm_field32_set(unit, entry, CNGf, cng);
        }
    } else {
        entry = &dscp_table[offset];
        soc_DSCP_TABLEm_field32_set(unit, entry, DSCPf, mapcp);
        soc_DSCP_TABLEm_field32_set(unit, entry, PRIf, prio);
        soc_DSCP_TABLEm_field32_set(unit, entry, CNGf, cng);
    }

    /* Add new chunk and store new HW index */

    rv = _bcm_dscp_table_entry_add(unit, entries, _BCM_QOS_MAP_CHUNK_DSCP, 
                                           (uint32 *)&index);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, PORT_TABm);
        return rv;
    }

    if (index != old_profile_index) {
        soc_mem_field32_set(unit, PORT_TABm, &pent, TRUST_DSCP_PTRf,
                            index / _BCM_QOS_MAP_CHUNK_DSCP);
        rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port,
                            &pent);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, PORT_TABm);
            return rv;
        }
    }

    rv = _bcm_dscp_table_entry_delete(unit, old_profile_index);
    /* Release port table lock */
    soc_mem_unlock(unit, PORT_TABm);
    return rv;
}

#endif /* BCM_KATANA2_SUPPORT */

