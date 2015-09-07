/*
 * $Id: topo_int.h 1.14 Broadcom SDK $
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
 * File:        topo_int.h
 * Purpose:     
 */

#ifndef   _TOPO_INT_H_
#define   _TOPO_INT_H_

#include <appl/cpudb/debug.h>
#define _TOPO_DEBUG(flags, stuff) TKS_DEBUG(TKS_DBG_TOPOLOGY | (flags), stuff)
#define TOPO_ERR(stuff) _TOPO_DEBUG(TKS_DBG_ERR, stuff)
#define TOPO_WARN(stuff) _TOPO_DEBUG(TKS_DBG_WARN, stuff)
#define TOPO_VERB(stuff) _TOPO_DEBUG(TKS_DBG_VERBOSE, stuff)
#define TOPO_VVERB(stuff) _TOPO_DEBUG(TKS_DBG_TOPOV, stuff)

/* Accessors for topo information stored in DB structure as cookie */

#define TP_INFO(db_ref) ((topo_info_t *)((db_ref)->topo_cookie))

/*
 * The TX CXN matrix is ordered (src, dest) -> src-tx-stk-idx
 * The RX CXN matrix is ordered (dest, src) -> dest-rx-stk-idx
 *
 * src and dest are topology CPU indices
 */

#define TP_TX_CXN_MATRIX(db_ref)  (TP_INFO(db_ref)->tp_tx_cxns)
#define TP_TX_CXN_INIT(db_ref, bytes) \
    sal_memset(TP_TX_CXN_MATRIX(db_ref), TOPO_CXN_UNKNOWN, bytes)
#define TP_TX_CXN(db_ref, src_idx, dest_idx) \
    (TP_TX_CXN_MATRIX(db_ref)[db_ref->num_cpus * src_idx + dest_idx])

#define TP_RX_CXN_MATRIX(db_ref)  (TP_INFO(db_ref)->tp_rx_cxns)
#define TP_RX_CXN_INIT(db_ref, bytes) \
    sal_memset(TP_RX_CXN_MATRIX(db_ref), TOPO_CXN_UNKNOWN, bytes)
#define TP_RX_CXN(db_ref, src_idx, dest_idx) \
    (TP_RX_CXN_MATRIX(db_ref)[db_ref->num_cpus * src_idx + dest_idx])

#define TP_REACHABLE(db_ref, src_idx, dest_idx) \
         (TP_TX_CXN(db_ref, src_idx, dest_idx) != TOPO_CXN_UNKNOWN)

#define PACK_LONG _SHR_PACK_LONG
#define UNPACK_LONG _SHR_UNPACK_LONG

#endif /* _TOPO_INT_H_ */
