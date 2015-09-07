/*
 * $Id: ctrans.h,v 1.2 Broadcom SDK $
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
 * File:        ctrans.h
 * Purpose:     
 */

#ifndef   _DIAG_CTRANS_H_
#define   _DIAG_CTRANS_H_

#include <bcm/types.h>
#include <appl/cpudb/cpudb.h>

#if defined(INCLUDE_LIB_CPUDB) && defined(INCLUDE_LIB_CPUTRANS) && \
    defined(INCLUDE_LIB_DISCOVER) && defined(INCLUDE_LIB_STKTASK)

extern const bcm_mac_t _bcast_mac;

#define MAX_DBS 10
extern cpudb_ref_t db_refs[MAX_DBS];
extern int cur_db;
extern int num_db;

#define PQ_KEY PQ_MAC

/* See stklink.h */
#define EVENT_LIST_NAMES \
    "INVALID",                    \
    "UNBlock",                    \
    "BLock",                      \
    "LinkUp",                     \
    "LinkDown",                   \
    "PKT",                        \
    "DRestart",                   \
    "DSuccess",                   \
    "DFailure",                   \
    "TSuccess",                   \
    "TFailure",                   \
    "ASuccess",                   \
    "AFailure",                   \
    "TimeOut",                    \
    "COMMFAIL",                   \
    NULL


#define PARSE_OBJ_CPUDB_STK_PORT                0
#define PARSE_OBJ_CPUDB_ENTRY                   1
#define PARSE_OBJ_MASTER_KEY                    2
#define PARSE_OBJ_LOCAL_KEY                     3
#define PARSE_OBJ_TOPO_STK_PORT                 4
#define PARSE_OBJ_TOPO_TX_MOD                   5
#define PARSE_OBJ_TOPO_RX_MOD                   6
#define PARSE_OBJ_TOPO_CPU                      7
#define PARSE_OBJ_TOPO_CLEAR                    8
#define PARSE_OBJ_TOPO_INSTALL                  9
#define PARSE_OBJ_STACK_INSTALL                 10
#define PARSE_OBJ_CPUDB_UNIT_MOD_IDS            11
#define PARSE_OBJ_LOCAL_ENTRY                   12
#define PARSE_OBJ_CPUDB_STK_PORT_STR            13

#define PARSE_OBJECT_NAMES \
    "cpudb_stk_port", \
    "cpudb_entry", \
    "master_key", \
    "local_key", \
    "topo_stk_port", \
    "topo_tx_mod",   \
    "topo_rx_mod",   \
    "topo_cpu",   \
    "topo_clear",      /* Command to clear data from parse_tp_cpu */ \
    "topo_install",    /* Command to install topology info  */ \
    "stack_install",   /* Call with current CPUDB */ \
    "cpudb_unit_mod_ids", \
    "local_entry", \
    "cpudb_stk_port_str", \
    NULL

#endif /* TKS libs defined */

#endif /* _DIAG_CTRANS_H_ */
