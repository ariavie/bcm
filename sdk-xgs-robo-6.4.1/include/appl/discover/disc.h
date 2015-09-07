/*
 * $Id: disc.h,v 1.17 Broadcom SDK $
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
 * File:        disc.h
 * Purpose:     Stack Discovery Algorithm
 */

#ifndef   _DISCOVERY_DISC_H_
#define   _DISCOVERY_DISC_H_

#include <sdk_config.h>
#include <shared/idents.h>

#include <bcm/types.h>
#include <bcm/rx.h>

#include <appl/cputrans/next_hop.h>
#include <appl/cpudb/cpudb.h>

#define DISC_PKT_TYPE_PROBE     SHARED_PKT_TYPE_PROBE
#define DISC_PKT_TYPE_ROUTING   SHARED_PKT_TYPE_ROUTING
#define DISC_PKT_TYPE_CONFIG    SHARED_PKT_TYPE_CONFIG
#define DISC_NH_PKT_TYPE        SHARED_PKT_TYPE_DISC_NH
#define DISC_CONFIG_CLIENT_ID   SHARED_CLIENT_ID_DISC_CONFIG

#define DISCOVERY_VERSION_0 0
#define DISCOVERY_VERSION_1 1
#define DISCOVERY_VERSION_2 2
#define DISCOVERY_VERSION_DEFAULT DISCOVERY_VERSION_2

/*
 * Default values in microseconds for
 *    DISC_TIMEOUT_DEFAULT
 *         overall timeout (before discovery fails)
 *    DISC_CFG_TO_DEFAULT
 *         mark stk ports w/o probe pkts as inactive
 *    DISC_RETRX_DEFAULT
 *         retransmit max times (re-send more probe/route packets)
 *    DISC_RETRX_MIN_DEFAULT
 *         retransmit min times (min time between retrx)
 *    DISC_ABORT_TIMEOUT_DEFAULT
 *         abort timeout (how long to wait for thread to ACK abort)
 */

#ifndef DISC_TIMEOUT_DEFAULT
#define DISC_TIMEOUT_DEFAULT       (10 * 1000000)    /* 10 seconds */
#endif
#ifndef DISC_CFG_TO_DEFAULT
#define DISC_CFG_TO_DEFAULT        (1000000)         /* 1 second */
#endif
#ifndef DISC_RETRX_DEFAULT
#define DISC_RETRX_DEFAULT         (200000)          /* 0.2 second */
#endif

#ifndef DISC_RETRX_MIN_DEFAULT
#define DISC_RETRX_MIN_DEFAULT     (100000)          /* 0.1 second */
#endif
#ifndef DISC_ABORT_TIMEOUT_DEFAULT
#define DISC_ABORT_TIMEOUT_DEFAULT (1000000)         /* 1 second */
#endif

#ifndef TRANS_PTR_DEFAULT
#define TRANS_PTR_DEFAULT (&bcm_trans_ptr)
#endif

/* Flags for ATP config client; applications may make this "no-ACK" */
#ifndef DISC_CONFIG_ATP_FLAGS_DEFAULT
#define DISC_CONFIG_ATP_FLAGS_DEFAULT ATP_F_NEXT_HOP
#endif
extern uint32 disc_config_atp_flags;

/*
 * These are the discovery return values used by Stack Task.
 *
 * DISC_RESTART_NEW_SEQ is a return value from disc_start indicating a
 * hard restart is requested.  This can be returned, for example, when
 * discovery has given up on a stack port as being active, but then a
 * probe packet is received indicating the port is alive.
 *
 * DISC_RESTART_REQUEST is an indication from discovery that it needs
 * to be restarted, but not necessarily with a new sequence number.
 */
#define DISC_RESTART_REQUEST		1
#define DISC_RESTART_NEW_SEQ		2
#define DISC_PROBE_RECEIVED             3

typedef int (*disc_cb_t)(cpudb_ref_t db_ref, int status, void *user_data);


/*
 * A lower transmit delay is preferred when TTL is enabled, especially
 * in simplex systems.
 *
 * Note that the system must reach the maximum necessary TTL (and process
 * packets) before reaching timeout.  Timeout, PROBE_DELAY and
 * TTL max should be properly adjusted.
 */

extern int disc_retrx_min_us;

/*
 * Probe packet time-to-live is the number of hops a probe packet is
 * permitted to be forwarded.  If it is 0, it is disabled and the
 * packet will not be filtered before it reaches it's point of
 * origination.
 *
 * For systems with only duplex links, TTL can be set to 2.  Otherwise,
 * max TTL needs to be at least the number of CPUs in the system (in
 * particular for simplex ring topologies.)
 */

#ifndef DISC_TTL_MIN_DEFAULT
#define DISC_TTL_MIN_DEFAULT 2
#endif
#ifndef DISC_TTL_MAX_DEFAULT
#define DISC_TTL_MAX_DEFAULT CPUDB_CPU_MAX
#endif

typedef int (*disc_start_election_cb_t)(cpudb_ref_t db_ref);
typedef int (*disc_election_cb_t)(cpudb_ref_t db_ref, void *user_data);

extern int disc_start(cpudb_ref_t db_ref, disc_start_election_cb_t m_elect);
extern int disc_abort(int disc_rv, int timeout_us);

extern int disc_timeout_set(int timeout_us, int cfg_to_us, int retrx_us);
extern int disc_timeout_get(int *timeout_us, int *cfg_to_us, int *retrx_us);
extern int disc_trans_ptr_set(bcm_trans_ptr_t *trans_ptr);
extern int disc_trans_ptr_get(bcm_trans_ptr_t **trans_ptr);

extern int disc_cos_set(int cos);
extern int disc_cos_get(int *cos);
extern int disc_vlan_set(int vlan);
extern int disc_vlan_get(int *vlan);
extern int disc_version_set(int version);
extern int disc_version_get(int *version);
extern int disc_ttl_min_set(int ttl_min);
extern int disc_ttl_min_get(int *ttl_min);
extern int disc_ttl_max_set(int ttl_max);
extern int disc_ttl_max_get(int *ttl_max);
extern int disc_fallback_set(int enable);
extern int disc_fallback_get(int *enable);
extern int disc_pkt_type_get(uint8 *pkt_buf);
extern int disc_election_register(disc_election_cb_t m_elect,
                                  void *user_data);
extern int disc_election_unregister(disc_election_cb_t m_elect,
                                    void *user_data);
extern int disc_run(cpudb_ref_t db_ref);
extern int disc_register(disc_cb_t callback, void *user_data);
extern int disc_unregister(disc_cb_t callback, void *user_data);
extern int disc_init(void);
extern int disc_deinit(void);

#endif /* _DISCOVERY_DISC_H_ */
