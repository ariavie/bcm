/*
 * $Id: topology.h,v 1.12 Broadcom SDK $
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
 * File:        topology.h
 * Purpose:     
 */

#ifndef   _STKTASK_TOPOLOGY_H_
#define   _STKTASK_TOPOLOGY_H_

#include <sdk_config.h>
#include <appl/cpudb/cpudb.h>

#ifndef TOPO_MODIDS_MAX
#define TOPO_MODIDS_MAX           128
#endif
#ifndef TOPO_DEFAULT_WEIGHT
#define TOPO_DEFAULT_WEIGHT          1
#endif
#define TOPO_CXN_UNKNOWN          0xff


/* Flag:  Force callback even if topology doesn't change */
#define TOPO_F_FORCE_AP_CALL 0x1

/*
 * tp_tx_cxns[src][dest] is the stack port index (relative to
 * DB in context) of the port in "src" which should be used
 * to reach "dest".  src and dest are "topo indexes", that
 * is just the ordinal index in which the CPUs appear when
 * iterating through the DB.
 *
 * tp_rx_cxns[dest][src] indicates the stack port index
 * in "dest" on which packets from CPU "src" should be expected.
 * This is used for programming blocking registers, etc, in
 * fabrics.
 */

typedef struct topo_info_s topo_info_t;
struct topo_info_s {
    uint8 *tp_tx_cxns;   /* "by hand" 2D array, num_cpu x num_cpu. */
    uint8 *tp_rx_cxns;
};

typedef struct topo_stk_port_s topo_stk_port_t;
struct topo_stk_port_s {
    uint8 flags; /* TOPO_CUT_PORT */
    int tx_mod_num;
    uint8 tx_mods[TOPO_MODIDS_MAX];
    int rx_mod_num;
    uint8 rx_mods[TOPO_MODIDS_MAX];
};

#define TOPO_CUT_PORT 0x1

typedef struct topo_cpu_s topo_cpu_t;
struct topo_cpu_s {
    /* Info recorded from local entry of completed DB or from topo pkt */
    cpudb_entry_t local_entry;

    uint8 version;
    uint16 master_seq_num;
    uint8 flags;
    topo_stk_port_t tp_stk_port[CPUDB_CXN_MAX];
    int num_cpus;
    uint16 ap_cookie;
};

typedef int (*topology_mod_id_assign_f)(cpudb_ref_t db_ref);

/****************************************************************
 *
 * Topology application function APIs.
 *
 ****************************************************************/

/* In topology.c:  Generic topology processing */

extern int topology_create(cpudb_ref_t db_ref);
extern int topology_destroy(cpudb_ref_t db_ref);
extern int topology_mod_ids_assign(cpudb_ref_t db_ref);
extern int topo_reserved_modid_set(uint32 modid, int enable);
extern int topo_reserved_modid_get(uint32 *modid);
extern int topology_mod_id_assign_function(topology_mod_id_assign_f func);

extern int topo_tx_port_get(
    cpudb_ref_t db_ref,
    cpudb_entry_t *src_entry,
    cpudb_entry_t *dest_entry,
    int *stk_idx);

/*
 * Some useful topology iterator macros
 *
 * topo_cpu_t *topo_info;
 * cpudb_stk_port_t *stk_p;
 */

/* Iterate over stack port info including flags and CPU cxns */
#define TOPO_FOREACH_STK_PORT_INFO(stk_p, topo_info, sp_idx)                \
    for (sp_idx = 0, stk_p = &(topo_info)->local_entry.stk_ports[0];        \
         sp_idx < (topo_info)->local_entry.base.num_stk_ports;              \
         stk_p = &(topo_info)->local_entry.base.stk_ports[++sp_idx])

/* Iterate over base stack port info (unit, port) */
#define TOPO_FOREACH_STK_PORT(port_p, topo_info, sp_idx)                    \
    for (sp_idx = 0, port_p = &(topo_info)->local_entry.base.stk_ports[0];  \
         sp_idx < (topo_info)->local_entry.base.num_stk_ports;              \
         port_p = &(topo_info)->local_entry.base.stk_ports[++sp_idx])

#define TOPO_FOREACH_TX_MODID(mod_id, topo_info, sp_idx, idx)               \
    for (idx = 0, mod_id = (topo_info)->tp_stk_port[sp_idx].tx_mods[0];     \
         idx < (topo_info)->tp_stk_port[sp_idx].tx_mod_num;                 \
         mod_id = (topo_info)->tp_stk_port[sp_idx].tx_mods[++idx])

#define TOPO_FOREACH_RX_MODID(mod_id, topo_info, sp_idx, idx)               \
    for (idx = 0, mod_id = (topo_info)->tp_stk_port[sp_idx].rx_mods[0];     \
         idx < (topo_info)->tp_stk_port[sp_idx].rx_mod_num;                 \
         mod_id = (topo_info)->tp_stk_port[sp_idx].rx_mods[++idx])

#endif /* _STKTASK_TOPOLOGY_H_ */
