/*
 * $Id: topo_pkt.h,v 1.5 Broadcom SDK $
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
 * File:        topo_pkt.h
 * Purpose:     
 */

#ifndef   _STKTASK_TOPO_PKT_H_
#define   _STKTASK_TOPO_PKT_H_

#include <sdk_config.h>

/****************************************************************
 *
 * Topology packet format description
 *
 * See topology.txt for more information.
 *
 ****************************************************************/

#define TOPO_PKT_VERSION              0

/* These are relative to start of topo packet, post CPUTRANS header */
#define TOPO_VER_OFS                  0
#define TOPO_RSVD_OFS                 (TOPO_VER_OFS + sizeof(uint32))
#define TOPO_MSEQ_NUM_OFS             (TOPO_RSVD_OFS + sizeof(uint32))
#define TOPO_BASE_LEN_OFS             (TOPO_MSEQ_NUM_OFS + sizeof(uint32))
#define TOPO_FLAGS_OFS                (TOPO_BASE_LEN_OFS + sizeof(uint32))
#define TOPO_MOD_IDS_OFS              (TOPO_FLAGS_OFS + sizeof(uint32))
#define TOPO_HEADER_BYTES(_units)  \
    (TOPO_MOD_IDS_OFS + (_units) * sizeof(uint32))
#define TOPO_CMP_START_OFS            TOPO_MOD_IDS_OFS

#ifndef TOPO_PKT_BYTES_MAX
#define TOPO_PKT_BYTES_MAX (1500) /* Max supported topo pkt size */
#endif

#define BCM_ST_TOPO_CLIENT_ID SHARED_CLIENT_ID_TOPOLOGY

/* Topo packet and stack task related API functions */

extern int topo_pkt_handle_init(uint32 topo_atp_flags);
extern int topo_pkt_expect_set(int enable);
extern int bcm_stack_topo_update(cpudb_ref_t db_ref);
extern int bcm_stack_topo_create(cpudb_ref_t db_ref, topo_cpu_t *tp_cpu);

extern int topo_pkt_gen(
    cpudb_ref_t db_ref,
    cpudb_entry_t *entry,
    uint8 *pkt_data,
    int len,
    int *packed_bytes);

extern int topo_pkt_parse(
    cpudb_ref_t db_ref,
    cpudb_entry_t *entry,
    uint8 *topo_data,
    int len,
    topo_cpu_t *tp_cpu,
    int *ap_data_ofs);

extern int topo_delay_set(int delay_us);
extern int topo_delay_get(int *delay_us);

extern int topo_master_delay_set(int delay_us);
extern int topo_master_delay_get(int *delay_us);

#endif /* _STKTASK_TOPO_PKT_H_ */
