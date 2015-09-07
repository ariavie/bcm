/*
 * $Id: rx.c,v 1.141 Broadcom SDK $
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
 * File:        rx.c
 * Purpose:     Receive packet mechanism
 * Requires:
 *
 * See sdk/doc/txrx.txt and pkt.txt for
 * information on the RX API and implementation.
 *
 * Quick overview:
 *
 *     Packet buffer allocation/deallocation is user configurable.
 *     This expects to be given monolithic (single block) buffers.
 *     When "HANDLED_OWNED" is returned by a handler, that means
 *     that the data buffer is stolen, not the packet structure.
 *
 *     Callback functions may be registered in interrupt or non-
 *     interrupt mode.  Non-interrupt is preferred.
 *
 *     Interrupt load is limited by setting overall rate limit
 *     (bcm_rx_rate_burst_set/get).
 *
 *     If a packet is not serviced in interrupt mode, it is queued
 *     based on its COS.
 *
 *     Each queue has a rate limit (bcm_rx_cos_rate_set/get) which
 *     controls the number of callbacks that will be made for the queue.
 *     The non-interrupt thread services these queues from highest to
 *     lowest and will discard packets in the queue when they exceed
 *     the queue's rate limit.
 *
 *     Packets handled at interrupt level are still accounted for in
 *     the COS rate limiting.
 *
 *     A channel is:
 *          Physically:  A separate hardware DMA process
 *          Logically:  A collection of COS bundled together.
 *     Rate limiting per channel is no longer supported (replaced
 *     by COS queue rate limiting).
 *
 *     Channels may be enabled and disabled separately from starting RX
 *     running.  However, stopping RX disables all channels.
 *
 *     Packets are started in groups called "chains", each of which
 *     is controlled by a "DV" (DMA-descriptor vector).
 *
 *     Updates to the handler linked list need to be synchronized
 *     both with thread packet processing (mutex) and interrupt
 *     packet processing (spl).
 *
 *     If no real callouts are registered (other than internal discard)
 *     don't bother starting DVs, nor queuing input pkts into cos queues.
 */

#include <shared/bsl.h>

#include <shared/alloc.h>
#include <sal/core/libc.h>
#include <soc/dma.h>
#include <soc/drv.h>
#include <soc/higig.h>
#if defined(BCM_BRADLEY_SUPPORT)
#include <soc/bradley.h>
#endif

#include <bcm_int/common/multicast.h>
#include <bcm_int/common/rx.h>

#ifdef BCM_ESW_SUPPORT
#ifdef BCM_XGS3_SWITCH_SUPPORT
#include <bcm_int/esw/port.h>
#endif /* BCM_XGS3_SWITCH_SUPPORT */
#include <bcm_int/esw/trunk.h>
#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
#include <bcm_int/esw/mpls.h>
#include <bcm_int/esw/mim.h>
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/vpn.h>
#include <bcm_int/esw/triumph.h>
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */
#if defined(INCLUDE_RCPU) && defined(BCM_XGS3_SWITCH_SUPPORT)
#include <bcm_int/esw/rcpu.h>
#endif
#ifdef BCM_TRX_SUPPORT
#include <bcm_int/esw/trx.h>
#endif /* BCM_TRX_SUPPORT */
#endif /* BCM_ESW_SUPPORT */

#ifdef BCM_DPP_SUPPORT
#include <soc/dpp/PPD/ppd_api_trap_mgmt.h>
#include <bcm_int/dpp/utils.h>
#include <bcm_int/dpp/rx.h>
#include <sal/core/libc.h>
#include <sal/appl/io.h>
#endif /*BCM_DPP_SUPPORT*/

#include <bcm_int/control.h>
#include <bcm/error.h>
#include <bcm/rx.h>
#include <bcm/stack.h>
#include <bcm_int/api_xlate_port.h>

#ifdef BCM_SBX_CMIC_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif

#ifdef BCM_RPC_SUPPORT
#include <bcm_int/rpc/rlink.h>
#endif

#ifdef BCM_ARAD_SUPPORT
#define BCM_RX_COS_ARAD   (64)
#endif 

#if defined(BROADCOM_DEBUG)
#define RX_ERROR(message_)      LOG_ERROR(BSL_LS_BCM_RX, message_)
#define RX_WARN(message_)       LOG_WARN(BSL_LS_BCM_RX, message_)
#define RX_INFO(message_)       LOG_INFO(BSL_LS_BCM_RX, message_)
#define RX_VERBOSE(message_)    LOG_VERBOSE(BSL_LS_BCM_RX, message_)
#define RX_PRINT(message_)      bsl_printf message_
#else

#define RX_ERROR(message_)
#define RX_WARN(message_)
#define RX_INFO(message_)
#define RX_VERBOSE(message_)
#define RX_PRINT(message_)
#endif  /* defined(BROADCOM_DEBUG) */



/* The main control structure for RX subsystem. */
rx_ctl_t          *rx_ctl[BCM_CONTROL_MAX];
int                rx_spl;

rx_control_t rx_control;
#ifdef BCM_SBX_CMIC_SUPPORT
extern void         sbx_rx_parse_pkt(int unit, bcm_pkt_t *pkt);
extern void         sbx_rx_parse_pkt_ext(int unit, bcm_pkt_t *pkt, uint32 erh_type, int fe_unit);
#endif

#if defined(BCM_RPC_SUPPORT) || defined(BCM_RCPU_SUPPORT)
static int         rx_common_spl;
#define RX_COMMON_INTR_LOCK            rx_common_spl = sal_splhi()
#define RX_COMMON_INTR_UNLOCK          sal_spl(rx_common_spl)

bcm_pkt_t *pkt_freelist;
/* ONLY modify pktlist_count BEFORE starting RX */
int bcm_rx_pktlist_count = BCM_RX_PKTLIST_COUNT_DEFAULT;

int
bcm_rx_remote_pkt_alloc(int len, bcm_pkt_t **pkt)
{
    uint8 *data;
    int rv;

    RX_COMMON_INTR_LOCK;
    if (pkt_freelist != NULL) {
        *pkt = pkt_freelist;
        pkt_freelist = (*pkt)->next;
    }
    RX_COMMON_INTR_UNLOCK;
    if (*pkt == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(*pkt, 0, sizeof(**pkt));

    data = NULL;
    rv = bcm_rx_pool_alloc(-1, len, 0, (void*)&data);
    if (BCM_E_NONE != rv) {
        bcm_rx_remote_pkt_free(*pkt);
        *pkt = NULL;
        return rv;
    }
    BCM_PKT_ONE_BUF_SETUP(*pkt, data, len);
    (*pkt)->alloc_ptr = data;
    return BCM_E_NONE;
}

int
bcm_rx_remote_pkt_free(bcm_pkt_t *pkt)
{
    if (pkt == NULL) {
        return BCM_E_INTERNAL;
    }
    if (pkt->alloc_ptr != NULL) {
        bcm_rx_pool_free(-1, pkt->alloc_ptr);
        pkt->alloc_ptr = NULL;
    }
    RX_COMMON_INTR_LOCK;
    pkt->next = pkt_freelist;
    pkt_freelist = pkt;
    RX_COMMON_INTR_UNLOCK;

    return BCM_E_NONE;
}
#endif  /* BCM_RPC_SUPPORT */

/* Standard BCM transport pointer structure */
bcm_trans_ptr_t bcm_trans_ptr = {
    bcm_rx_alloc,
    bcm_rx_free,
    bcm_pkt_rx_alloc,
    bcm_pkt_rx_free,
    bcm_rx_register,
    bcm_rx_unregister,
    bcm_tx_pkt_setup,
    bcm_tx,
    bcm_tx_list,
    bcm_tx_array,
    bcm_tx_pkt_l2_map,        /* Not yet fully implemented */
    0                         /* Default unit for alloc/free */
};

#if (defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined (BCM_ARAD_SUPPORT))

STATIC int
_bcm_rx_default_scheduler(int unit, int *sched_unit, 
                          bcm_cos_queue_t *sched_cosq, int *sched_count);

#define _Q_DEC_TOKENS(q) if ((q)->pps > 0) ((q)->tokens)--;

/*
 * This should be called each time a packet is handled, owned, etc.
 * It counts when all packets for a DV are processed; it
 * changes the DV state and notifies the thread at that point.
 */
#define MARK_PKT_PROCESSED(_pkt, unit, dv)                              \
    do {                                                                \
        if (RX_IS_LOCAL(unit) || RX_IS_RCPU(unit)) {                    \
            MARK_PKT_PROCESSED_LOCAL(unit, dv);                         \
        } else {                                                        \
            bcm_rx_remote_pkt_free(_pkt);                               \
        }                                                               \
    } while (0)

/*
 * As above, no local checking
 */
#define MARK_PKT_PROCESSED_LOCAL(unit, dv)                              \
    do {                                                                \
        int pkts_processed;                                             \
        RX_INTR_LOCK;                                                   \
        pkts_processed = ++DV_PKTS_PROCESSED(dv);                       \
        RX_INTR_UNLOCK;                                                 \
        if (pkts_processed == RX_PPC(unit)) {                           \
            DV_STATE(dv) = DV_S_NEEDS_FILL;                             \
            RX_THREAD_NOTIFY(unit);                                     \
        }                                                               \
    } while (0)

#ifndef BCM_RPC_SUPPORT
#undef  MARK_PKT_PROCESSED
#define MARK_PKT_PROCESSED(_pkt, unit,  dv)                             \
        MARK_PKT_PROCESSED_LOCAL(unit,  dv)
#endif  /* BCM_RPC_SUPPORT */

/* Sleep .5 seconds when not running. */
#define NON_RUNNING_SLEEP 500000

static int _rx_chan_run_count;

int bcm_rx_token_check_us = BCM_RX_TOKEN_CHECK_US_DEFAULT;

/* Check if given quantity is < cur sleep secs; set to that value if so */
#define BASE_SLEEP_VAL                                               \
    ((_rx_chan_run_count > 0) ? bcm_rx_token_check_us : NON_RUNNING_SLEEP)

/* Set sleep to base value */
#define INIT_SLEEP    rx_control.sleep_cur = BASE_SLEEP_VAL

/* Lower sleep time if val is < current */
#define SLEEP_MIN_SET(val)                                           \
    (rx_control.sleep_cur = ((val) < rx_control.sleep_cur) ?         \
     (val) : rx_control.sleep_cur)

/*
 * Boolean:
 *     Is discard the only callout registered?
 */
#define INTR_CALLOUTS(unit) \
    (rx_ctl[unit]->hndlr_intr_cnt != 0)
#define NON_INTR_CALLOUTS(unit) \
    (rx_ctl[unit]->hndlr_cnt != 0)
#define NO_REAL_CALLOUTS(unit) \
    (rx_ctl[unit]->hndlr_intr_cnt == 0 && rx_ctl[unit]->hndlr_cnt == 0)

/* Number of packets allocated for a channel */
#define RX_CHAN_PKT_COUNT(unit, chan) (RX_PPC(unit) * RX_CHAINS(unit, chan))

#define RX_DEFAULT_ALLOC    bcm_rx_pool_alloc
#define RX_DEFAULT_FREE     bcm_rx_pool_free

static int         rx_dv_count;     /* Debug: count number of DVs used */


/* Forward declarations */
STATIC INLINE void     rx_done_chain(int unit, dv_t *dv);
STATIC void        rx_pkt_thread(void *param);
STATIC int         rx_thread_start(int unit);
STATIC void        rx_process_packet(int unit, bcm_pkt_t *pkt);
#ifdef BCM_RCPU_SUPPORT
STATIC void        rx_rcpu_process_packet(int unit, bcm_pkt_t *pkt);
#endif /* BCM_RCPU_SUPPORT */
STATIC INLINE void     rx_intr_process_packet(int unit, dv_t *dv,
                                     dcb_t *dcb, bcm_pkt_t *pkt);
STATIC bcm_rx_t    rx_discard_packet(int unit, bcm_pkt_t *pkt,
                                     void *cookie);

STATIC int         rx_channel_dv_setup(int unit, int chan);
STATIC void        rx_channel_shutdown(int unit, int chan);
STATIC void        rx_user_cfg_check(int unit);
STATIC void        rx_dcb_per_pkt_init(int unit);
STATIC int         rx_discard_handler_setup(int unit, rx_ctl_t *rx_ctrl_ptr);

STATIC INLINE void     rx_dv_fill(int unit, int chan, dv_t *dv);
STATIC INLINE dv_t    *rx_dv_alloc(int unit, int chan, int dv_idx);
STATIC INLINE void     rx_dv_dealloc(int unit, int chan, int dv_idx);
STATIC void        rx_queue_init(int unit, rx_ctl_t *rx_ctrl_ptr);

STATIC void        rx_init_all_tokens(int unit);
STATIC void        rx_cleanup(int unit);

STATIC void        rx_default_parser(int unit,  bcm_pkt_t *pkt);

/* Default data for configuring RX system */
static bcm_rx_cfg_t _rx_dflt_cfg = {
    RX_PKT_SIZE_DFLT,       /* packet alloc size */
    RX_PPC_DFLT,            /* Packets per chain */
    RX_PPS_DFLT,            /* Default pkt rate, global (all COS, one unit) */
    0,                      /* Burst */
    {                       /* Just configure channel 1 */
        {0},                /* Channel 0 is usually TX */
        {                   /* Channel 1, default RX */
            RX_CHAINS_DFLT, /* DV count (number of chains) */
            1000,           /* Default pkt rate, DEPRECATED */
            0,              /* No flags */
            0xff            /* COS bitmap channel to receive */
        }
    },
    RX_DEFAULT_ALLOC,       /* alloc function */
    RX_DEFAULT_FREE,        /* free function */
    0                       /* flags */
};

#if defined(BCM_RPC_SUPPORT) || defined(BCM_RCPU_SUPPORT)
static bcm_pkt_t *pktlist_alloc;
#endif

/****************************************************************
 *
 * User visible API routines:
 *     bcm_rx_init                 Init,
 *     bcm_rx_start                     Setup,
 *     bcm_rx_stop                          teardown,
 *     bcm_rx_cfg_get                            get configuration
 *     bcm_rx_cfg_init             Reinitial user configuration
 *     bcm_rx_register             Register/unregister handlers
 *     bcm_rx_unregister
 *     bcm_rx_queue_register
 *     bcm_rx_queue_unregister
 *     bcm_rx_cosq_mapping_size_get
 *     bcm_rx_cosq_mapping_set
 *     bcm_rx_cosq_mapping_get
 *     bcm_rx_cosq_mapping_delete
 *     bcm_rx_channels_running
 *     bcm_rx_alloc                Gateways to packet alloc/free a buffer
 *     bcm_rx_free
 *     bcm_rx_rate_get/set         Get/set interrupt load rate limits
 *     bcm_rx_cos_rate_get/set     Get/set COS rate limits
 *     bcm_rx_cos_burst_get/set    Get/set burst rate limits
 *     bcm_rx_cos_max_len_get/set  Get/set max q len limits
 *
 * Since we demand doing an RX stop to change some parts of the
 * configuration (like pkts/chain setting), the DVs must be
 * aborted and deallocated on stop.
 *
 ****************************************************************/


/* 
 * Function:
 *      _bcm_rx_ctrl_lock
 * Purpose:
 *      Lock rx control structures for all units.
 * Parameters: 
 *      None.
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_rx_ctrl_lock(void) 
{
    int unit;          /* Unit iterator. */

    _BCM_RX_SYSTEM_LOCK;

    for (unit = 0; unit < BCM_CONTROL_MAX; unit++) {
        /* Skip rx uninitialized units. */
        if (!RX_IS_SETUP(unit)) {
            continue;
        }
        RX_LOCK(unit);
    }
    return (BCM_E_NONE);
}

/* 
 * Function:
 *      _bcm_rx_ctrl_unlock
 * Purpose:
 *      Unlock rx control structures for all units.
 * Parameters: 
 *      None.
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_rx_ctrl_unlock(void) 
{
    int unit;          /* Unit iterator. */

    for (unit = 0; unit < BCM_CONTROL_MAX; unit++) {
        /* Skip rx uninitialized units. */
        if (!RX_IS_SETUP(unit)) {
            continue;
        }
        RX_UNLOCK(unit);
    }

    _BCM_RX_SYSTEM_UNLOCK;
        
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_rx_unit_list_update
 * Purpose:
 *      Update list of units with rx started.
 * Parameters: 
 *      None.
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_rx_unit_list_update(void) 
{
    int unit;              /* Unit iterator.                  */
    int prev_unit;         /* Last unit with rx initialized.  */
    int rv = BCM_E_NONE;   /* Operation return status.        */

    /* Lock all units rx control structure. */ 
    _bcm_rx_ctrl_lock();

    /* Reset first unit in the list & max cos queue number. */
    rx_control.rx_unit_first = prev_unit = -1;
    rx_control.system_cosq_max = -1;

    for (unit = 0; unit < BCM_CONTROL_MAX; unit++) {
        /* Reset next unit on initialized units. */
        if (RX_IS_SETUP(unit)) {
            rx_ctl[unit]->next_unit = -1;
        }

        /* Skip rx not started units. */
        if (!RX_UNIT_STARTED(unit)) {
            continue;
        }

        /* Fill rx started units into a linked list .*/
        if (-1 == prev_unit) {
            rx_control.rx_unit_first = unit;
        } else {
            rx_ctl[prev_unit]->next_unit = unit;
        }
        prev_unit = unit;
        rx_ctl[unit]->next_unit = -1;

        if (RX_QUEUE_MAX(unit) > rx_control.system_cosq_max) {
            rx_control.system_cosq_max = RX_QUEUE_MAX(unit);
        }
    }

    /* Unlock all units rx control structure. */ 
    _bcm_rx_ctrl_unlock(); 
    return (rv);
}


/*
 * Function:
 *      bcm_common_rx_sched_register
 * Purpose:
 *      Rx scheduler registration function. 
 * Parameters:
 *      unit       - (IN) Unused. 
 *      sched_cb   - (IN) Rx scheduler routine.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_sched_register(int unit, bcm_rx_sched_cb sched_cb)
{
    /* Input parameters check. */
    if (NULL == sched_cb) {
        return (BCM_E_PARAM);
    }

    _bcm_rx_ctrl_lock(); 

    rx_control.rx_sched_cb = sched_cb; 

    _bcm_rx_ctrl_unlock(); 

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_rx_sched_unregister
 * Purpose:
 *      Rx scheduler de-registration function. 
 * Parameters:
 *      unit  - (IN) Unused. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_sched_unregister(int unit)
{
    _bcm_rx_ctrl_lock(); 

    rx_control.rx_sched_cb = _BCM_RX_SCHED_DEFAULT_CB;

    _bcm_rx_ctrl_unlock(); 
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_rx_unit_next_get
 * Purpose:
 *      Rx started units iteration routine.
 * Parameters:
 *      unit       - (IN)  BCM device number. 
 *      unit_next  - (OUT) Next attached unit with started rx.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_unit_next_get(int unit, int *unit_next)
{
    /* Input parameters check. */
    if (NULL == unit_next) {
        return (BCM_E_PARAM);
    }

    if (!RX_IS_SETUP(unit)) {
        *unit_next = -1;
    } else {
        RX_LOCK(unit);
        if (!RX_UNIT_STARTED(unit)) {
            *unit_next = -1;
        } else {
            *unit_next = rx_ctl[unit]->next_unit;
        }
        RX_UNLOCK(unit);
    }
    return (BCM_E_NONE);
}


/*
 * Function:
 *      bcm_common_rx_queue_max_get
 * Purpose:
 *      Get maximum cos queue number for the device.
 * Parameters:
 *      unit    - (IN) BCM device number. 
 *      cosq    - (OUT) Maximum queue priority.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_queue_max_get(int unit, bcm_cos_queue_t  *cosq)
{
    /* Input parameters check. */
    if (NULL == cosq) {
        return (BCM_E_PARAM);
    }

    if (!SOC_UNIT_VALID(unit)) {
        *cosq = NUM_CPU_COSQ_DEF;
    } else {

        *cosq = NUM_CPU_COSQ(unit) - 1;
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_rx_queue_packet_count_get
 * Purpose:
 *      Get number of packets awaiting processing in the specific device/queue.
 * Parameters:
 *      unit         - (IN) BCM device number. 
 *      cosq         - (IN) Queue priority.
 *      packet_count - (OUT) Number of packets awaiting processing. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_queue_packet_count_get(int unit, bcm_cos_queue_t cosq, int *packet_count)
{
    /* Input parameters check. */
    if (NULL == packet_count) {
        return (BCM_E_PARAM);
    }
    
    if (0 == RX_IS_SETUP(unit)) {
        return (BCM_E_INIT);
    }

    if (cosq > RX_QUEUE_MAX(unit)) {
        return (BCM_E_PARAM);
    }

    RX_LOCK(unit);
    if (RX_UNIT_STARTED(unit)) {
        *packet_count = RX_QUEUE(unit, cosq)->count;
    } else {
        *packet_count = 0;
    }
    RX_UNLOCK(unit);
    return (BCM_E_NONE);
}
/*
 * Function:
 *      bcm_common_rx_queue_rate_limit_status
 * Purpose:
 *      Get number of packet that can be rx scheduled 
 *      until system hits queue rx rate limit. 
 * Parameters:
 *      unit           - (IN) BCM device number. 
 *      cosq           - (IN) Queue priority.
 *      packet_tokens  - (OUT)Maximum number of packets that can be  scheduled.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_queue_rate_limit_status_get(int unit, bcm_cos_queue_t cosq, 
                                       int *packet_tokens)
{
    /* Input parameters check. */
    if (NULL == packet_tokens) {
        return (BCM_E_PARAM);
    }

    if (0 == RX_IS_SETUP(unit)) {
        return (BCM_E_INIT);
    }

    if (cosq > RX_QUEUE_MAX(unit)) {
        return (BCM_E_PARAM);
    }

    RX_LOCK(unit);
    if (RX_UNIT_STARTED(unit)) {
        if (RX_QUEUE(unit, cosq)->pps > 0) {
            *packet_tokens = RX_QUEUE(unit, cosq)->tokens;
        } else {
            *packet_tokens = BCM_RX_SCHED_ALL_PACKETS;
        }
    } else {
        /* Don't schedule anything for this device. */ 
        *packet_tokens = 0;  
    }
    RX_UNLOCK(unit);
    return (BCM_E_NONE);
}

#ifdef BCM_RPC_SUPPORT
/*
 * Function:
 *      _bcm_rx_sl_mode_update
 * Purpose:
 *      Update packet structures for SL mode
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Right now, affects all packet structures.  This will be a
 *      problem if packets are "on the fly".
 */

int
_bcm_rx_sl_mode_update(int unit) {
    int sl_mode;
    int i;

    if (unit < 0 || unit >= BCM_CONTROL_MAX) {  /* Bad unit number */
        return BCM_E_PARAM;
    }

    if (rx_ctl[unit] == NULL) { /* Init not yet done */
        return BCM_E_NONE;
    }

    sl_mode = (SOC_UNIT_VALID(unit) && SOC_SL_MODE(unit))
        && !RX_IGNORE_SL(unit);

    for (i = 0; i < bcm_rx_pktlist_count; i++) {
        if (sl_mode) {
            pktlist_alloc[i].flags |= BCM_PKT_F_SLTAG;
        } else {
            pktlist_alloc[i].flags &= ~BCM_PKT_F_SLTAG;
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_RPC_SUPPORT */

/*
 * Function:
 *      bcm_rx_init
 * Purpose:
 *      Software initialization for RX API
 * Parameters:
 *      unit - Unit to init
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Allocates rx control structure
 *      Copies default config into active config
 *      Adds discard handler
 */

int
_bcm_common_rx_init(int unit)
{
    int rv;
    rx_ctl_t *rx_ctrl_ptr;        
    int queue_size;

    if (rx_ctl[unit] != NULL) {
        if (_bcm_common_rx_active(unit)) {
            if ((rv = _bcm_common_rx_stop(unit, NULL)) < 0) {
                RX_ERROR((BSL_META_U
                          (unit, "Error in RX Init: RX Stop returned %s\n"),
                          bcm_errmsg(rv)));
                return rv;
            }
        }
        return BCM_E_NONE;
    }

#if defined(BCM_RPC_SUPPORT) || defined(BCM_RCPU_SUPPORT)
    if (pktlist_alloc == NULL) { /* Set up remote packet free list */
        int i;
        int len = bcm_rx_pktlist_count * sizeof(bcm_pkt_t);

        pktlist_alloc = sal_alloc(len, "RX pkt list");
        if (pktlist_alloc == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(pktlist_alloc, 0, len);

        for (i = 0; i < bcm_rx_pktlist_count; i++) {
            sal_memset(&pktlist_alloc[i], 0, sizeof(bcm_pkt_t));
            pktlist_alloc[i].next = &pktlist_alloc[i + 1];
        }
        pktlist_alloc[bcm_rx_pktlist_count - 1].next = NULL;
        pkt_freelist = pktlist_alloc;
    }
#endif

    rx_ctrl_ptr = sal_alloc(sizeof(rx_ctl_t), "rx_ctl");
    if (rx_ctrl_ptr == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(rx_ctrl_ptr, 0, sizeof(rx_ctl_t));
    sal_memcpy(&rx_ctrl_ptr->user_cfg, &_rx_dflt_cfg, sizeof(bcm_rx_cfg_t));

    rv = _bcm_common_rx_queue_max_get(unit, &rx_ctrl_ptr->queue_max);
    if (BCM_FAILURE(rv)) {
        sal_free(rx_ctrl_ptr);
        return (rv);
    }

    if((rx_ctrl_ptr->queue_max+1) > BCM_RX_COS){
        sal_free(rx_ctrl_ptr);
        return BCM_E_INTERNAL;
    }
    
    queue_size = sizeof(rx_queue_t) * BCM_RX_COS;
    rx_ctrl_ptr->pkt_queue =  sal_alloc(queue_size, "pkt_queue");
    if (rx_ctrl_ptr->pkt_queue == NULL) {
        sal_free(rx_ctrl_ptr);
        return BCM_E_MEMORY;
    }
    sal_memset(rx_ctrl_ptr->pkt_queue, 0, queue_size);
    rx_queue_init(unit, rx_ctrl_ptr);

    rx_ctrl_ptr->rx_mutex = sal_mutex_create("RX_MUTEX");
    rv = rx_discard_handler_setup(unit, rx_ctrl_ptr);
    if (BCM_FAILURE(rv)) {
        sal_mutex_destroy(rx_ctrl_ptr->rx_mutex);
        sal_free(rx_ctrl_ptr->pkt_queue);
        sal_free(rx_ctrl_ptr);
        return (rv);
    } 

    if (!bcm_rx_pool_setup_done()) {
        RX_INFO((BSL_META_U(unit, "RX: Starting rx pool\n")));
        rv = (bcm_rx_pool_setup(-1,
                      rx_ctrl_ptr->user_cfg.pkt_size + sizeof(void *)));
        if (BCM_FAILURE(rv)) {
            sal_free((void *)rx_ctrl_ptr->rc_callout);
            sal_mutex_destroy(rx_ctrl_ptr->rx_mutex);
            sal_free(rx_ctrl_ptr->pkt_queue);
            sal_free(rx_ctrl_ptr);
            return (rv);
        } 
    }

    if (NULL == rx_control.system_lock) {                            
        rx_control.system_lock = sal_mutex_create("RX system lock"); 
        if (NULL == rx_control.system_lock) {
            bcm_rx_pool_cleanup();
            sal_free((void *)rx_ctrl_ptr->rc_callout);
            sal_mutex_destroy(rx_ctrl_ptr->rx_mutex);
            sal_free(rx_ctrl_ptr->pkt_queue);
            sal_free(rx_ctrl_ptr);
            return (BCM_E_MEMORY);
        }
    }                                                                

    if (NULL == rx_control.start_lock) {                            
        rx_control.start_lock = sal_mutex_create("RX start lock"); 
        if (NULL == rx_control.start_lock) {
            bcm_rx_pool_cleanup();
            sal_free((void *)rx_ctrl_ptr->rc_callout);
            sal_mutex_destroy(rx_ctrl_ptr->rx_mutex);
            sal_mutex_destroy(rx_control.system_lock);
            sal_free(rx_ctrl_ptr->pkt_queue);
            sal_free(rx_ctrl_ptr);
            return (BCM_E_MEMORY);
        }
    }                                                                

    rx_ctrl_ptr->next_unit = -1;
    _BCM_RX_SYSTEM_LOCK;
    rx_ctl[unit] = rx_ctrl_ptr;
    _BCM_RX_SYSTEM_UNLOCK; 
    RX_INFO((BSL_META_U(unit, "RX: Initialized unit %d\n"), unit));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_rx_shutdown
 * Purpose:
 *      Shutdown threads, free up resources without touching
 *      hardware
 * Parameters:
 *      unit - StrataXGS unit number
 * Returns:
 *      BCM_E_XXX
 */
int _bcm_rx_shutdown(int unit)
{
    rx_callout_t *callout_p;
    rx_callout_t *next_callout_p;

    if (0 == RX_IS_SETUP(unit)) {
        return (BCM_E_NONE);
    }

    _BCM_RX_START_LOCK;
    _BCM_RX_SYSTEM_LOCK;

    if (RX_UNIT_STARTED(unit))
    {
        bcm_rx_stop(unit, NULL);
    }

    rx_cleanup(unit);

    for (callout_p = (void *) (rx_ctl[unit]->rc_callout); callout_p != NULL;)
    {
        next_callout_p = (void *) callout_p->rco_next;

        sal_free(callout_p);

        callout_p = next_callout_p;
    }

    rx_ctl[unit]->rc_callout = NULL;

    sal_mutex_destroy(rx_ctl[unit]->rx_mutex);

    sal_free(rx_ctl[unit]->pkt_queue);
    sal_free(rx_ctl[unit]);
    rx_ctl[unit] = NULL;

#if defined(BCM_RPC_SUPPORT) || defined(BCM_RCPU_SUPPORT)
    if (pktlist_alloc != NULL) {
        bcm_pkt_t *pkt, *next_pkt;

        pkt = pkt_freelist;
        while (pkt != NULL) {
            next_pkt = pkt->next;
            if (pkt->alloc_ptr != NULL) {
                bcm_rx_remote_pkt_free(pkt);
                pkt->alloc_ptr = NULL;
            }
            pkt = next_pkt;
        }
        sal_free(pktlist_alloc);
        pktlist_alloc = NULL;
    }
    pkt_freelist = NULL;
#endif
       

    rx_control.system_flags |= BCM_RX_CTRL_ACTIVE_UNITS_UPDATE;

    _BCM_RX_SYSTEM_UNLOCK;
    _BCM_RX_START_UNLOCK;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_rx_cfg_init
 * Purpose:
 *      Re-initialize the user level configuration
 * Parameters:
 *      unit - StrataXGS unit number
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Can't use if currently running.  Should be called before
 *      doing a simple modification of the RX configuration in case
 *      the previous user has left it in a strange state.
 */

int
_bcm_common_rx_cfg_init(int unit)
{
    RX_INIT_CHECK(unit);

    if (RX_UNIT_STARTED(unit)) {
        return BCM_E_BUSY;
    }

    sal_memcpy(&rx_ctl[unit]->user_cfg, &_rx_dflt_cfg, sizeof(bcm_rx_cfg_t));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_rx_start
 * Purpose:
 *      Initialize and configure the RX subsystem for a given unit
 * Parameters:
 *      unit - Unit to configure
 *      cfg - Configuration to use.  See include/bcm/rx.h
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Starts the packet receive thread if not already running.
 *      cfg may be null:  Use default config.
 *      alloc/free in cfg may be null:  Use default alloc/free functions
 */

int
_bcm_common_rx_start(int unit, bcm_rx_cfg_t *cfg)
{  
    
    int rv = BCM_E_NONE; 
    int chan; 

    RX_INIT_CHECK(unit); 

    if (RX_UNIT_STARTED(unit)) { 
        RX_INFO((BSL_META_U(unit, "RX start %d:  Already started\n"), unit));
        return BCM_E_BUSY;
    }

    _BCM_RX_START_LOCK;
    if (cfg) {  /* Use default if config not specified. */
        if ((cfg->pkt_size == 0) || (cfg->pkts_per_chain == 0)) {
            _BCM_RX_START_UNLOCK;
            return BCM_E_PARAM;
        }
        sal_memcpy(&rx_ctl[unit]->user_cfg, cfg, sizeof(bcm_rx_cfg_t));
        if (cfg->rx_alloc == NULL) {
            rx_ctl[unit]->user_cfg.rx_alloc = RX_DEFAULT_ALLOC;
        }
        if (cfg->rx_free == NULL) {
            rx_ctl[unit]->user_cfg.rx_free = RX_DEFAULT_FREE;
        }
        rx_user_cfg_check(unit);
    }

    RX_INFO((BSL_META_U(unit, "RX: Starting unit %d\n"), unit));
    
    if (rx_ctl[unit]->rx_parser == NULL) {
        rx_ctl[unit]->rx_parser = rx_default_parser;
    }

#if defined(BCM_RPC_SUPPORT) || defined(BCM_RCPU_SUPPORT)
    if (rx_ctl[unit]->user_cfg.rx_alloc != RX_DEFAULT_ALLOC ||
            rx_ctl[unit]->user_cfg.rx_free != RX_DEFAULT_FREE) {
        LOG_WARN(BSL_LS_BCM_RX,
                 (BSL_META_U(unit,
                             BSL_META_U
                              (unit, "RX WARNING: Not using rx_pool alloc/free with %s.\n")),
                  RX_IS_RCPU(unit)? "RCPU" : "RPC"));
    }
#endif

    if (RX_IS_LOCAL(unit)) {
        rx_dcb_per_pkt_init(unit);
    }

    rx_init_all_tokens(unit);
    rx_ctl[unit]->pkts_since_start = 0;
    rx_ctl[unit]->pkts_owned = 0;

    if (RX_IS_LOCAL(unit) && SOC_UNIT_VALID(unit)) {
        int         first_rx_chan = -1;

        /* Set up each channel */
        FOREACH_SETUP_CHANNEL(unit, chan) {
            rv = rx_channel_dv_setup(unit, chan);
            if (rv < 0) {
                RX_ERROR((BSL_META_U
                          (unit, "RX: Error on setup unit %d, chan %d\n"),
                          unit, chan));
                rx_cleanup(unit);
                return rv;
            }
            
            rx_ctl[unit]->chan_running |= (1 << chan);
            ++_rx_chan_run_count;
            /* get the first RX channel */
            if (first_rx_chan == -1) {
                first_rx_chan = chan;
            }
        }

        /*
         * RX cos based DMA
         */
        if (soc_feature(unit, soc_feature_cos_rx_dma) && 
                first_rx_chan != -1) {
            /* enable all cosq on the first Rx channel */
            rv = _bcm_common_rx_queue_channel_set(unit, -1, first_rx_chan);
            if (BCM_FAILURE(rv)) {
                _BCM_RX_START_UNLOCK;
                return rv;
            }
            if(!soc_feature(unit, soc_feature_cmicm)) {
                soc_pci_write(unit, CMIC_CONFIG, 
                               (soc_pci_read(unit, CMIC_CONFIG) |
                               CC_COS_QUALIFIED_DMA_RX_EN));
            }
        }
    }

    RX_INTR_LOCK;
    if (!rx_control.thread_running) {
        rx_control.rx_unit_first = -1;
        rx_control.system_cosq_max = -1;
        rx_control.thread_running = TRUE;
        RX_INTR_UNLOCK;
        /* Start up the thread */
        if ((rv = rx_thread_start(unit)) < 0) {
            rx_control.thread_running = FALSE;
            rx_cleanup(unit);
            _BCM_RX_START_UNLOCK;
            return rv;
        }
    } else {
        RX_INTR_UNLOCK;
    }

    rx_ctl[unit]->flags |= BCM_RX_F_STARTED;

#if defined(INCLUDE_RCPU) && defined(BCM_ESW_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
    if (RX_IS_RCPU(unit)) {
        soc_rcpu_tocpu_cb_register(unit, _bcm_esw_rcpu_tocpu_rx);
    }
#endif
    /* 
     * Make sure to update the list after
     * new unit was marked active. 
     */
    _BCM_RX_SYSTEM_LOCK;
    rx_control.system_flags |= BCM_RX_CTRL_ACTIVE_UNITS_UPDATE;
    _BCM_RX_SYSTEM_UNLOCK;
    _BCM_RX_START_UNLOCK;
    return rv;
}

/*
 * Function:
 *      bcm_rx_stop
 * Purpose:
 *      Stop RX for the given unit; saves current configuration
 * Parameters:
 *      unit - The unit to stop
 *      cfg - OUT Configuration copied to this parameter
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      This signals the thread to exit.
 */

int
_bcm_common_rx_stop(int unit, bcm_rx_cfg_t *cfg)
{
    int i;

    RX_INIT_CHECK(unit);

    _BCM_RX_START_LOCK;
    RX_INFO((BSL_META_U(unit, "RX: Stopping unit %d\n"), unit));

    if (cfg != NULL) {    /* Save configuration */
        sal_memcpy(cfg, &rx_ctl[unit]->user_cfg, sizeof(bcm_rx_cfg_t));
    }

#if defined(INCLUDE_RCPU) && defined(BCM_ESW_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
    if (RX_IS_RCPU(unit)) {
        soc_rcpu_tocpu_cb_unregister(unit);
    }
#endif /* INCLUDE_RCPU */

    /*
     * If no units are active, signal thread to stop and wait to
     * see it exit; simple semaphore
     */
    RX_INTR_LOCK;
    for (i = 0; i < BCM_CONTROL_MAX; i++) {
        if (!RX_IS_SETUP(i) || i == unit) {
            continue;
        }
        if (rx_ctl[i]->flags & BCM_RX_F_STARTED) {
            /* Some other unit is active */
            rx_ctl[unit]->flags &= ~BCM_RX_F_STARTED;
            RX_INTR_UNLOCK;
            _BCM_RX_SYSTEM_LOCK;
            rx_control.system_flags |= BCM_RX_CTRL_ACTIVE_UNITS_UPDATE;
            _BCM_RX_SYSTEM_UNLOCK;
            _BCM_RX_START_UNLOCK;
            return BCM_E_NONE;
        }
    }

    /* Okay, no one else is running.  Kill thread */
    if (rx_control.thread_running) {
        rx_control.thread_exit_complete = FALSE;
        rx_control.thread_running = FALSE;
        RX_INTR_UNLOCK;
        RX_THREAD_NOTIFY(unit);
        for (i = 0; i < 10; i++) {
            if (rx_control.thread_exit_complete) {
                break;
            }
            /* Sleep a bit to allow thread to stop */
            sal_usleep(500000);
        }
        if (!rx_control.thread_exit_complete) {
            LOG_WARN(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U
                                  (unit, "RX %d: Thread %p running after signaled "
                                  "to stop; \nDVs may not be cleaned up.\n")),
                      unit, (void *)rx_control.rx_tid));
        } else {
            rx_control.rx_tid = NULL;
        }
    } else {
        RX_INTR_UNLOCK;
    }

    rx_ctl[unit]->flags &= ~BCM_RX_F_STARTED;
    _BCM_RX_SYSTEM_LOCK;
    rx_control.system_flags |= BCM_RX_CTRL_ACTIVE_UNITS_UPDATE;
    _BCM_RX_SYSTEM_UNLOCK;
    _BCM_RX_START_UNLOCK;

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_common_rx_clear
 * Purpose:
 *      Clear all RX info
 * Returns:
 *      BCM_E_NONE
 */

int
_bcm_common_rx_clear(int unit)
{
    for (unit = 0; unit < BCM_CONTROL_MAX; unit++)
    {
        BCM_IF_ERROR_RETURN(_bcm_rx_shutdown(unit));
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_rx_cfg_get
 * Purpose:
 *      Check if init done; get the current RX configuration
 * Parameters:
 *      unit - Strata device ID
 *      cfg - OUT Configuration copied to this parameter.  May be NULL
 * Returns:
 *      BCM_E_INIT if not running on unit
 *      BCM_E_NONE if running on unit
 *      < 0 BCM_E_XXX error code
 * Notes:
 */

int
_bcm_common_rx_cfg_get(int unit, bcm_rx_cfg_t *cfg)
{
    RX_INIT_CHECK(unit);

    /* Copy config */
    if (cfg != NULL) {
        sal_memcpy(cfg, &rx_ctl[unit]->user_cfg, sizeof(bcm_rx_cfg_t));
    }

    return (RX_UNIT_STARTED(unit)) ? BCM_E_NONE : BCM_E_INIT;
}

/* Install callback handle */ 
static int
_bcm_common_rx_callback_install(int unit, const char * name, rx_callout_t *rco, 
                             uint8 priority, uint32 flags)
{
    volatile rx_callout_t    *list_rco, *prev_rco;
    int                       i;
    
    RX_LOCK(unit);
    RX_INTR_LOCK;

    /* Need to double check duplicate callback */
    for (list_rco = rx_ctl[unit]->rc_callout; list_rco;
         list_rco = list_rco->rco_next) {
        if (list_rco->rco_function == rco->rco_function &&
            list_rco->rco_priority == rco->rco_priority) {
            if (((list_rco->rco_flags & BCM_RCO_F_INTR) == 
                        (rco->rco_flags & BCM_RCO_F_INTR)) &&
                (list_rco->rco_cookie == rco->rco_cookie)) {
                /* get duplicate handle, update cosq */
                for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                     if (SHR_BITGET(rco->rco_cos, i)) {
                         SHR_BITSET(list_rco->rco_cos, i);
                     }
                }
                RX_INTR_UNLOCK;
                RX_UNLOCK(unit);
                sal_free ((void *)rco);
                return BCM_E_NONE;
            }
            LOG_VERBOSE(BSL_LS_BCM_RX,
                        (BSL_META_U(unit,
                                    BSL_META_U
                                     (unit, "RX: %s registered with diff params\n")),
                         name));
                                                                                                          
            RX_INTR_UNLOCK;
            RX_UNLOCK(unit);
            sal_free((void *)rco);
            return BCM_E_PARAM;
        }
    }
    
    /*
     * Find correct place to insert handler, this code assumes that
     * the discard handler has been registered on init.  Handlers
     * of the same priority are placed in the list in the order
     * they are registered
     */

    prev_rco = NULL;
    for (list_rco = rx_ctl[unit]->rc_callout; list_rco;
         list_rco = list_rco->rco_next) {
        if (list_rco->rco_priority < priority) {
            break;
        }

        prev_rco = list_rco;
    }

    if (prev_rco) {                     /* Insert after prev_rco */
        rco->rco_next = prev_rco->rco_next;
        prev_rco->rco_next = rco;
    } else {                            /* Insert first */
        rco->rco_next = list_rco;
        rx_ctl[unit]->rc_callout = rco;
    }

    if (flags & BCM_RCO_F_INTR) {
        rx_ctl[unit]->hndlr_intr_cnt++;
    } else {
        rx_ctl[unit]->hndlr_cnt++;
    }
    RX_INTR_UNLOCK;
    RX_UNLOCK(unit);

    LOG_VERBOSE(BSL_LS_BCM_RX,
                (BSL_META_U(unit,
                            BSL_META_U(unit, "RX: %s registered %s%s.\n")),
                 name,
                 prev_rco ? "after " : "first",
                 prev_rco ? prev_rco->rco_name : ""));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_rx_queue_register
 * Purpose:
 *      Register an application callback for the specified CPU queue
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      name - constant character string for debug purposes.
 *      cosq - CPU cos queue
 *      callback - callback function pointer.
 *      priority - priority of handler in list (0 is lowest priority).
 *      cookie - cookie passed to driver when packet arrives.
 *      flags - Register for interrupt or non-interrupt callback
 * Returns:
 *      BCM_E_NONE - callout registered.
 *      BCM_E_MEMORY - memory allocation failed.
 */

int
_bcm_common_rx_queue_register(int unit, const char *name, bcm_cos_queue_t cosq, 
                          bcm_rx_cb_f callback, uint8 priority, void *cookie, 
                          uint32 flags)
{
    volatile rx_callout_t      *rco;
    volatile rx_callout_t      *list_rco;
    int i;

    /*
     * If the caller wants to use the same callback function for different
     * queues, they have to call multiple times. Note that in the driver, 
     * we keep track of the queues using a bitmap so that only one RCO entry 
     * is needed for each callback.
     */
    if (callback == NULL) {
        return BCM_E_PARAM;
    }

    /* Check unit */
    RX_INIT_CHECK(unit);

    /* Check cosq number */
    if ((cosq != BCM_RX_COS_ALL) &&
        (cosq < 0 || cosq > RX_QUEUE_MAX(unit))) {
        return BCM_E_PARAM;
    }

    RX_INFO((BSL_META_U
             (unit, "RX: Registering %s on %d, cosq 0x%x flags 0x%x%s\n"),
             name, unit, cosq, flags, flags & BCM_RCO_F_INTR ? "(intr)" : ""));

#if defined(BCM_RPC_SUPPORT)
    if (RX_IS_REMOTE(unit) && !RX_IS_RCPU(unit)) {
        int rv;

        /* Always try to set up tunnel connection; multiple connects okay. */
        rv = bcm_rlink_rx_connect(unit);
        if (rv < 0) {
            LOG_WARN(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U
                                  (unit, "RX: rlink connect unit %d returned %d: %s\n")),
                      unit, rv, bcm_errmsg(rv)));
        }
    }
#endif
    /* 
     * In the case of re-installing a callback with the same priority, 
     * unlike _bcm_common_rx_register(), this API will just update the cosq 
     * bitmap for the callback if other parameters are the same. This allows 
     * multiple cosq to be added to same callback via calling 
     * _bcm_common_rx_queue_register() multiple times.
     */

     RX_LOCK(unit);
     RX_INTR_LOCK;
     for (list_rco = rx_ctl[unit]->rc_callout; list_rco;
          list_rco = list_rco->rco_next) {
          uint32 flag_int = flags & BCM_RCO_F_INTR;

          if (list_rco->rco_function == callback &&
              list_rco->rco_priority == priority) {
              if ((list_rco->rco_flags & BCM_RCO_F_INTR) == flag_int &&
                  list_rco->rco_cookie == cookie) {
                  if (cosq == BCM_RX_COS_ALL) {
                      for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                           SHR_BITSET(list_rco->rco_cos, i);
                      }
                  } else {
                      SHR_BITSET(list_rco->rco_cos, cosq);

                      /* Support legacy cosq input via flags */
                      if (flags & BCM_RCO_F_ALL_COS) {
                          for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                              SHR_BITSET(list_rco->rco_cos, i);
                          }
                      } else {
                          for (i = 0; i < 16; i++) {
                              if ((flags & BCM_RCO_F_COS_ACCEPT_MASK) &
                                      BCM_RCO_F_COS_ACCEPT(i)) {
                                  SHR_BITSET(list_rco->rco_cos, i);
                              }
                          }
                      }
                  }

                  RX_INTR_UNLOCK;
                  RX_UNLOCK(unit);
                  return BCM_E_NONE;
              }
              LOG_VERBOSE(BSL_LS_BCM_RX,
                          (BSL_META_U(unit,
                                      BSL_META_U
                                       (unit, "RX: %s registered with diff params\n")),
                           name));

              RX_INTR_UNLOCK;
              RX_UNLOCK(unit);
              return BCM_E_PARAM;
          }
     }

    RX_INTR_UNLOCK;
    RX_UNLOCK(unit);

    /* Alloc a callback struct */
    if ((rco = sal_alloc(sizeof(*rco), "rx_callout")) == NULL) {
        return(BCM_E_MEMORY);
    }

    /* Init the callback struct */
    SETUP_RCO(rco, name, callback, priority, cookie, NULL, flags);

    if (cosq == BCM_RX_COS_ALL) {
        for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
             SHR_BITSET(rco->rco_cos, i);
        }
    } else {
        SHR_BITSET(rco->rco_cos, cosq);

        /* Support legacy cosq input via flags */
        if (flags & BCM_RCO_F_ALL_COS) {
            for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                SHR_BITSET(rco->rco_cos, i);
            }
        } else {
            for (i = 0; i < 16; i++) {
                if ((flags & BCM_RCO_F_COS_ACCEPT_MASK) & 
                        BCM_RCO_F_COS_ACCEPT(i)) {
                    SHR_BITSET(rco->rco_cos, i);
                }
            }
        }
    }

    return _bcm_common_rx_callback_install(unit, name, (rx_callout_t *)rco, 
                                       priority, flags);
}

/*
 * Function:
 *      bcm_rx_register
 * Purpose:
 *      Register an upper layer driver
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      chan - DMA channel number
 *      name - constant character string for debug purposes.
 *      priority - priority of handler in list (0 is lowest priority).
 *      f - function to call for that driver.
 *      cookie - cookie passed to driver when packet arrives.
 *      flags - Register for interrupt or non-interrupt callback
 * Returns:
 *      BCM_E_NONE - callout registered.
 *      BCM_E_MEMORY - memory allocation failed.
 * Notes:
 *      Refer bcm_rx_queue_register() if cosq is bigger than 16.
 */

int
_bcm_common_rx_register(int unit, const char *name, bcm_rx_cb_f callback,
                uint8 priority, void *cookie, uint32 flags)
{
    volatile rx_callout_t      *rco, *list_rco;
    int i;

    RX_INIT_CHECK(unit);

    if (NULL == callback) {
        return BCM_E_PARAM;
    }

    RX_INFO((BSL_META_U
             (unit, "RX: Registering %s on %d, flags 0x%x%s\n"),
             name, unit, flags, flags & BCM_RCO_F_INTR ? "(intr)" : ""));

    if (!(flags & BCM_RCO_F_COS_ACCEPT_MASK) &&
            !(flags & BCM_RCO_F_ALL_COS)) {
        LOG_WARN(BSL_LS_BCM_RX,
                 (BSL_META_U(unit,
                             BSL_META_U
                              (unit,
                              "RX unit %d: Registering callback with no COS accepted.\n")),
                  unit));
        LOG_WARN(BSL_LS_BCM_RX,
                 (BSL_META_U(unit,
                             BSL_META_U(unit, "    Callbacks will not occur to %s\n")),
                  name));
    }

#if defined(BCM_RPC_SUPPORT)
    if (RX_IS_REMOTE(unit) && !RX_IS_RCPU(unit)) {
        int rv;

        /* Always try to set up tunnel connection; multiple connects okay. */
        rv = bcm_rlink_rx_connect(unit);
        if (rv < 0) {
            LOG_WARN(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U
                                  (unit, "RX: rlink connect unit %d returned %d: %s\n")),
                      unit, rv, bcm_errmsg(rv)));
        }
    }
#endif

    RX_LOCK(unit);
    RX_INTR_LOCK; 
    /* First check if already registered */
    for (list_rco = rx_ctl[unit]->rc_callout; list_rco;
         list_rco = list_rco->rco_next) {
        if (list_rco->rco_function == callback &&
            list_rco->rco_priority == priority) {
            if (list_rco->rco_flags == flags &&
                list_rco->rco_cookie == cookie) {
                RX_INTR_UNLOCK;
                RX_UNLOCK(unit);
                return BCM_E_NONE;
            }
            LOG_VERBOSE(BSL_LS_BCM_RX,
                        (BSL_META_U(unit,
                                    BSL_META_U
                                     (unit, "RX: %s registered with diff params\n")),
                         name));

            RX_INTR_UNLOCK;
            RX_UNLOCK(unit);
            return BCM_E_PARAM;
        }
    }

    RX_INTR_UNLOCK;
    RX_UNLOCK(unit);

    if ((rco = sal_alloc(sizeof(*rco), "rx_callout")) == NULL) {
        return(BCM_E_MEMORY);
    }
    SETUP_RCO(rco, name, callback, priority, cookie, NULL, flags);
    /* Older implementation used only rco_flags field to carry
     * cos information. Since there are now devices which support > 32
     * cos queues, rco_cos bitmap has been added. Add the specified
     * cos to rco_cos here.
     */
    if (flags & BCM_RCO_F_ALL_COS) {
        for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
            SETUP_RCO_COS_SET(rco, i);
        }
    } else {
        for (i = 0; i < 16; i++) {
            if ((flags & BCM_RCO_F_COS_ACCEPT_MASK) & BCM_RCO_F_COS_ACCEPT(i)) {
                SETUP_RCO_COS_SET(rco, i);
            }
        }
    }

    return _bcm_common_rx_callback_install(unit, name, (rx_callout_t *)rco, 
                                 priority, flags);
}

/* Common function for bcm_rx_unregister and bcm_rx_queue_unregister */
static int
_bcm_common_rx_callback_unregister(int unit, bcm_rx_cb_f callback, 
             uint8 priority,  bcm_cos_queue_t cosq)
{
    volatile rx_callout_t *rco;
    volatile rx_callout_t *prev_rco = NULL;
    const char *name;
    uint32 flags;
    int i;

    RX_INIT_CHECK(unit);

    if ((cosq != BCM_RX_COS_ALL) &&
        (cosq < 0 || cosq > RX_QUEUE_MAX(unit))) {
         return BCM_E_PARAM;
    } 

    RX_LOCK(unit);
    RX_INTR_LOCK;
    for (rco = rx_ctl[unit]->rc_callout; rco; rco = rco->rco_next) {
        if (rco->rco_function == callback && rco->rco_priority == priority) {
            break;
        }
        prev_rco = rco;
    }

    if (!rco) {
        RX_INTR_UNLOCK;
        RX_UNLOCK(unit);
        return BCM_E_NOT_FOUND;
    }

    if (cosq != BCM_RX_COS_ALL) {
        SHR_BITCLR(rco->rco_cos, cosq);
        for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
             if (SHR_BITGET(rco->rco_cos, i)) {
                 /* Don't delete callback if any cosq still associated */
                 RX_INTR_UNLOCK;
                 RX_UNLOCK(unit);
                 return BCM_E_NONE;
             }
        }
    }
    name = rco->rco_name;
    flags = rco->rco_flags;

    if (!prev_rco) {  /* First elt on list */
        rx_ctl[unit]->rc_callout = rco->rco_next;
    } else {          /* skip current */
        prev_rco->rco_next = rco->rco_next;
    }

    if (flags & BCM_RCO_F_INTR) {
        rx_ctl[unit]->hndlr_intr_cnt--;
    } else {
        rx_ctl[unit]->hndlr_cnt--;
    }
    RX_INTR_UNLOCK;
    RX_UNLOCK(unit);

#if defined(BCM_RPC_SUPPORT)
    if (RX_IS_REMOTE(unit) && !RX_IS_RCPU(unit)) {
        /* Check if time to unregister */
        int rv;

        if (rx_ctl[unit]->rc_callout == NULL) { /* No callouts, disconnect */
            rv = bcm_rlink_rx_disconnect(unit);
            if (rv < 0) {
                LOG_WARN(BSL_LS_BCM_RX,
                         (BSL_META_U(unit,
                                     BSL_META_U
                                      (unit,
                                      "RX: rlink disconnect unit %d returned %d: %s\n")),
                          unit, rv, bcm_errmsg(rv)));
            }
        }
    }
#endif

    RX_INFO((BSL_META_U(unit, "RX: Unregistered %s on %d\n"), name, unit));
    sal_free((void *)rco);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_rx_unregister
 * Purpose:
 *      De-register a callback function
 * Parameters:
 *      unit - Unit reference
 *      priority - Priority of registered callback
 *      callback - The function being registered
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Run through linked list looking for match of function and priority
 */

int
_bcm_common_rx_unregister(int unit, bcm_rx_cb_f callback, uint8 priority)
{
    return _bcm_common_rx_callback_unregister(unit, callback, priority, 
                                           BCM_RX_COS_ALL);
}

/*
 * Function:
 *      bcm_rx_queue_unregister
 * Purpose:
 *      Unregister a callback function
 * Parameters:
 *      unit - Unit reference
 *      cosq - CPU cos queue
 *      priority - Priority of registered callback
 *      callback - The function being registered
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_common_rx_queue_unregister(int unit, bcm_cos_queue_t cosq,
                            bcm_rx_cb_f callback, uint8 priority)
{
    return _bcm_common_rx_callback_unregister (unit, callback, priority, cosq);
}

/*
 * Function:
 *      bcm_rx_reasons_get
 * Purpose:
 *      Get all supported reasons for rx packets
 * Parameters:
 *      unit - Unit reference
 *      reasons - rx packet "reasons" bitmap
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_common_rx_reasons_get(int unit, bcm_rx_reasons_t *reasons)
{
    uint32 ix;
    uint32 max_index = 32;
    soc_rx_reason_t *map;
    soc_rx_reason_t **map_array;
    int maps_index = 0;

    if (!SOC_UNIT_VALID(unit)) {
        
        return BCM_E_INTERNAL;
    }

    BCM_RX_REASON_CLEAR_ALL(*reasons);

    if (soc_feature(unit, soc_feature_dcb_reason_hi)) {
        max_index = 64;
    }

    map_array = SOC_DCB_RX_REASON_MAPS(unit);
    while ((map = map_array[maps_index++]) != NULL) {
        for (ix = 0; ix < max_index ; ix++) {
            if (map[ix] != (soc_rx_reason_t)socRxReasonInvalid && 
                map[ix] != (soc_rx_reason_t)socRxReasonCount) {
                BCM_RX_REASON_SET(*reasons, (bcm_rx_reason_t)map[ix]);
            }
        }
    }

#ifdef  BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        /* BPDU is a separate bit in the DCB, so it is pasted in */
        BCM_RX_REASON_SET(*reasons, bcmRxReasonBpdu);
    }
#endif
    return BCM_E_NONE;
}

/*  
 * Function:
 *      bcm_rx_queue_channel_set
 * Purpose:
 *      Assign a RX channel to a cosq 
 * Parameters:
 *      unit - Unit reference
 *      queue_id - CPU cos queue index (0 - (max cosq - 1)) 
 *                                      (Negative for all)
 *      chan_id - channel index (0 - (BCM_RX_CHANNELS-1)), -1 for no channel
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_queue_channel_set(int unit, bcm_cos_queue_t queue_id, 
                                 bcm_rx_chan_t chan_id)
{
    uint32 ix, bit;
    uint32 chan_id_max = BCM_RX_CHANNELS; 
    uint32 reg_index,reg_base, reg_addr, reg_val;
#ifdef BCM_CMICM_SUPPORT
    int cmc;
    int pci_cmc = SOC_PCI_CMC(unit);
    uint32 chan_off;
    int startq = 0;
    int numq, endq, countq;
    int start_chan_id;
#ifdef BCM_KATANA_SUPPORT
    uint32 i = 0;
#endif

    if (0 != SOC_CMCS_NUM(unit)) {
        chan_id_max *= SOC_CMCS_NUM(unit);
    }
#endif

    if (SOC_WARM_BOOT(unit)) {
        return BCM_E_NONE;
    }
    if (!soc_feature(unit, soc_feature_cos_rx_dma)) {
        /* not DMA set up */
        return BCM_E_CONFIG;
    } else if (-1 == chan_id) {
        if (-1 == queue_id) {
            /* May not disable all queues */
            return BCM_E_PARAM;
        } else if (soc_feature(unit, soc_feature_cmicm)) {
            /* May not disable queues on CMICm devices */
            return BCM_E_PARAM;
        } /* Else, OK to disable queue */
    } else if ((chan_id < 0) || (chan_id >= chan_id_max)) {
        /* Verify the chan id */
        return BCM_E_PARAM;
    } else if (queue_id >= NUM_CPU_COSQ(unit)) {
        /* Verify queue_id */
        return BCM_E_PARAM;
    }

#ifdef BCM_CMICM_SUPPORT /* For now, we also have to support CMICe */
    if(soc_feature(unit, soc_feature_cmicm)) {
        /* We institute the normalizing assumption that the
         * channels are numbered first in the PCI CMC, then in order of the
         * ARM CMCs.  This is why we have the shuffling steps below.
         */
        if (chan_id < BCM_RX_CHANNELS) {
            cmc = pci_cmc;
        } else {
            cmc = SOC_ARM_CMC(unit, ((chan_id / BCM_RX_CHANNELS) - 1));
            /* compute start queue number for any CMC */
            startq = NUM_CPU_ARM_COSQ(unit, pci_cmc);
            for (ix = 0; ix < cmc; ix++) {
                startq += (ix != pci_cmc) ? NUM_CPU_ARM_COSQ(unit, ix) : 0;
            }
        }

        numq = NUM_CPU_ARM_COSQ(unit, cmc);
        start_chan_id = (cmc != pci_cmc) ? cmc * BCM_RX_CHANNELS : 0;
        
        if (queue_id < 0) { /* All COS Queues */
            /* validate the queue range of CMC is in the valid range
             * for this CMC
             */
            SHR_BITCOUNT_RANGE(CPU_ARM_QUEUE_BITMAP(unit, cmc),
                               countq, startq, numq);
            if (numq != countq) {
                return BCM_E_PARAM;
            }

            /* We know chan_id != -1 from the parameter check at the
             * start of the function */
            endq = startq + NUM_CPU_ARM_COSQ(unit, cmc);
            for (ix = start_chan_id; ix < (start_chan_id + BCM_RX_CHANNELS); ix++) {
                /* set CMIC_CMCx_CHy_COS_CTRL_RX_0/1 based on CMC's start
                 * and end queue number
                 */
                chan_off = ix % BCM_RX_CHANNELS;
                reg_val = 0;
                if (ix == (uint32)chan_id) {
                    reg_val = (endq < 32) ?
                              ((uint32)1 << endq) - 1 : 0xffffffff;
                    reg_val &= (startq < 32) ?
                               ~(((uint32)1 << startq) - 1) : 0;
                }

                reg_addr = CMIC_CMCx_CHy_COS_CTRL_RX_0_OFFSET(cmc, chan_off);
                /* Incoporate the reserved queues (if any on this device)
                 * into the CMIC programming,  */
                reg_val |= CPU_ARM_RSVD_QUEUE_BITMAP(unit,cmc)[0];
                soc_pci_write(unit, reg_addr, reg_val);
                reg_addr = CMIC_CMCx_CHy_COS_CTRL_RX_1_OFFSET(cmc, chan_off);

#if defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_KATANAX(unit)) {
                    /*
                     * Katana queues have been disabled to prevent packets
                     * that cannot egress from reaching the CMIC. At this
                     * point those queues can be enabled.
                     */
                    for (i = startq; i < endq; i++) {
                        reg_val = 0;
                        soc_reg_field_set(unit,
                            THDO_QUEUE_DISABLE_CFG2r, &reg_val, QUEUE_NUMf, i);
                        SOC_IF_ERROR_RETURN(
                            WRITE_THDO_QUEUE_DISABLE_CFG2r(unit, reg_val));
                        reg_val = 0;
                        soc_reg_field_set(unit,
                            THDO_QUEUE_DISABLE_CFG1r, &reg_val, QUEUE_WRf, 1);
                        SOC_IF_ERROR_RETURN(
                            WRITE_THDO_QUEUE_DISABLE_CFG1r(unit, reg_val));
                    }
                }
#endif /*BCM_KATANA_SUPPORT */
                reg_val = 0;               
                if (ix == (uint32)chan_id) {
                    reg_val = ((endq >= 32) && (endq < 64)) ?
                              (((uint32)1 << (endq % 32)) - 1) : 
                              ((endq < 32) ? 0 : 0xffffffff);
                    reg_val &= (startq >= 32) ?
                               ~(((uint32)1 << (startq % 32)) - 1) : 0xffffffff;
                }
                /* Incoporate the reserved queues (if any on this device)
                 * into the CMIC programming,  */
                reg_val |= CPU_ARM_RSVD_QUEUE_BITMAP(unit,cmc)[1];
                soc_pci_write(unit, reg_addr, reg_val);
            }
        } else {
            endq = startq + NUM_CPU_ARM_COSQ(unit, cmc);
            if (queue_id < startq || queue_id >= endq) {
                return BCM_E_PARAM;
            } 
            if (!SHR_BITGET(CPU_ARM_QUEUE_BITMAP(unit, cmc), queue_id)) {
                /* This queue is in use by some other CMC, reject */
                return BCM_E_PARAM;
            }
            for (ix = start_chan_id;
                 ix < (start_chan_id + BCM_RX_CHANNELS); ix++) {
                chan_off = ix % BCM_RX_CHANNELS;
                reg_addr = (queue_id < 32) ?
                    CMIC_CMCx_CHy_COS_CTRL_RX_0_OFFSET(cmc, chan_off) :
                    CMIC_CMCx_CHy_COS_CTRL_RX_1_OFFSET(cmc, chan_off);
                reg_val = soc_pci_read(unit, reg_addr);
                if (ix == (uint32)chan_id) {
                    reg_val |= ((uint32)1 << (queue_id % 32));
                } else { /* Clear for all other channels */
                    reg_val &= ~((uint32)1 << (queue_id % 32));
                }
                /* Incoporate the reserved queues (if any on this device)
                 * into the CMIC programming,  */
                reg_val |=
                    CPU_ARM_RSVD_QUEUE_BITMAP(unit,cmc)[queue_id / 32];
                soc_pci_write(unit, reg_addr, reg_val);

#if defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_KATANAX(unit)) {
                    /*
                     * Katana queues have been disabled to prevent packets
                     * that cannot egress from reaching the CMIC. At this
                     * point those queues can be enabled.
                     */
                    reg_val = 0;
                    soc_reg_field_set(unit, THDO_QUEUE_DISABLE_CFG2r, &reg_val,
                                      QUEUE_NUMf, queue_id);
                    SOC_IF_ERROR_RETURN(
                            WRITE_THDO_QUEUE_DISABLE_CFG2r(unit, reg_val));
                    reg_val = 0;
                    soc_reg_field_set(unit, THDO_QUEUE_DISABLE_CFG1r, &reg_val,
                                      QUEUE_WRf, 1);
                    SOC_IF_ERROR_RETURN(
                            WRITE_THDO_QUEUE_DISABLE_CFG1r(unit, reg_val));
                }
#endif /*BCM_KATANA_SUPPORT */
            }
        }
    } else
#endif /* CMICM Support */
    {
        if (chan_id >= 0) {
            if (SOC_KNET_MODE(unit)) {
                /* Accept all DMA channels configured for Rx */
                reg_val = soc_pci_read(unit, CMIC_DMA_CTRL);
                reg_val &= DC_DIRECTION_MASK(chan_id);
                if (reg_val == DC_MEM_TO_SOC(chan_id)) {
                    return BCM_E_NOT_FOUND;
                }
            } else if (!RX_CHAN_USED(unit, chan_id)) {
                return BCM_E_NOT_FOUND;
            }
        }
        if (SOC_IS_TRX(unit)) {
            reg_base = CMIC_RX_COS_CONTROL_0;
        } else {
            reg_base = CMIC_RX_COS_CONTROL;
        }
        reg_index = NUM_CPU_COSQ(unit) / 8; 

        /* Negative queue_id represents all COS queue */
        if (queue_id < 0) {
            /* We know chan_id != -1 from the parameter check at the
             * start of the function */
            for (ix = 0; ix < reg_index; ix++) {
                reg_addr = reg_base + (4 * ix);
                reg_val  = (0xff << RCC_COS_MAP_SHFT(chan_id));
                soc_pci_write(unit, reg_addr, reg_val); 
            }
        } else {
            reg_addr = reg_base + (4 * (queue_id / 8));
            reg_val = soc_pci_read(unit, reg_addr);
            bit = 1 << (queue_id % 8);
            for (ix = 0; ix < BCM_RX_CHANNELS; ix++) { 
                if (ix == (uint32)chan_id) {
                    reg_val |= (bit << RCC_COS_MAP_SHFT(ix));
                } else {
                    /* a cosq can associate with exact one channel */
                    reg_val &= ~(bit << RCC_COS_MAP_SHFT(ix));
                }
            }
            soc_pci_write(unit, reg_addr, reg_val);
        }
    }
    return BCM_E_NONE;
}

/*  
 * Function:
 *      bcm_rx_queue_channel_get
 * Purpose:
 *      Get the associated rx channel with a given cosq
 * Parameters:
 *      unit - Unit reference
 *      queue_id - CPU cos queue index (0 - (max cosq - 1)) 
 *      chan_id - channel index (0 - (BCM_RX_CHANNELS-1))
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_common_rx_queue_channel_get(int unit, bcm_cos_queue_t queue_id, 
                             bcm_rx_chan_t *chan_id)
{
    uint32 reg_base, reg_addr, reg_val;
    uint32 ix, bit;
#ifdef BCM_CMICM_SUPPORT
    int cmc;
    int chan_offset;
    int pci_cmc = SOC_PCI_CMC(unit);
    uint32 chan_id_max = BCM_RX_CHANNELS * SOC_CMCS_NUM(unit);
#endif

    if (!SOC_UNIT_VALID(unit)) {
        
        return BCM_E_INTERNAL;
    }
    if (!RX_INIT_DONE(unit)) {
        return BCM_E_INIT;
    }

    *chan_id = -1;
    if (!soc_feature(unit, soc_feature_cos_rx_dma)) {
        /* not DMA set up */
        return BCM_E_CONFIG;
    } else if (queue_id < 0 || queue_id >= NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

#ifdef BCM_CMICM_SUPPORT /* For now, we also have to support CMICe */
    if(soc_feature(unit, soc_feature_cmicm)) {
        for (ix = 0; ix < chan_id_max; ix++) { 
            if (ix < BCM_RX_CHANNELS) {
                cmc = pci_cmc;
            } else {
                cmc = SOC_ARM_CMC(unit, ((ix / BCM_RX_CHANNELS) - 1));
            }
            chan_offset = ix % BCM_RX_CHANNELS;
            reg_addr = (queue_id < 32) ? 
                        CMIC_CMCx_CHy_COS_CTRL_RX_0_OFFSET(cmc,chan_offset) :
                        CMIC_CMCx_CHy_COS_CTRL_RX_1_OFFSET(cmc,chan_offset);
            bit = 1 << (queue_id % 32);
            reg_val = soc_pci_read(unit, reg_addr);
            if (reg_val & bit) {
                if (*chan_id == -1) {
                    *chan_id = ix;
                } else {
                    LOG_WARN(BSL_LS_BCM_RX,
                             (BSL_META_U(unit,
                                         BSL_META_U
                                          (unit,
                                          "rx_queue_channel_get: Warning: "
                                          "Multiple channels assigned to the "
                                          "COS 0x%x for unit %d\n")),
                              queue_id, unit));
                    return BCM_E_INTERNAL;
                }
            }
        }
    } else
#endif /* CMICM Support */
    {
        if (SOC_IS_TRX(unit)) {
            reg_base = CMIC_RX_COS_CONTROL_0;
        } else {
            reg_base = CMIC_RX_COS_CONTROL;
        }

        reg_addr = reg_base + (4 * (queue_id / 8));
        reg_val  = soc_pci_read(unit, reg_addr);

        bit = 1 << (queue_id % 8);
        for (ix = 0; ix < BCM_RX_CHANNELS; ix++) {
             if (reg_val & (bit << RCC_COS_MAP_SHFT(ix))) {
                 if (*chan_id == -1) {
                     *chan_id = ix;
                 } else {
                     LOG_WARN(BSL_LS_BCM_RX,
                              (BSL_META_U(unit,
                                          BSL_META_U
                                           (unit,
                                           "rx_queue_channel_get: Warning: "
                                           "Multiple channels assigned to "
                                           "the COS 0x%x for unit %d\n")), 
                               queue_id, unit));
                     return BCM_E_INTERNAL;
                 }
             }
        }
    }
    return ((*chan_id == -1)? BCM_E_NOT_FOUND : BCM_E_NONE);
}

/*
 * Function:
 *      bcm_rx_active
 * Purpose:
 *      Return boolean as to whether unit is running
 * Parameters:
 *      unit - StrataXGS to check
 * Returns:
 *      Boolean:   TRUE if unit is running.
 * Notes:
 *
 */

int
_bcm_common_rx_active(int unit)
{
    return (RX_UNIT_STARTED(unit)) != 0;
}


/*
 * Function:
 *      bcm_rx_running_channels_get
 * Purpose:
 *      Returns a bitmap indicating which channels are active
 * Parameters:
 *      unit       - Which unit to operate on
 * Returns:
 *      Bitmap of active channels
 * Notes:
 */

int
_bcm_common_rx_channels_running(int unit, uint32 *channels)
{
    if (!RX_INIT_DONE(unit)) {
        return BCM_E_INIT;
    }
    *channels = rx_ctl[unit]->chan_running;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_rx_alloc
 * Purpose:
 *      Gateway to configured RX allocation function
 * Parameters:
 *      unit - Unit reference
 *      pkt_size - Packet size, see notes.
 *      flags - Used to set up packet flags
 * Returns:
 *      Pointer to new packet buffer or NULL if cannot alloc memory
 * Notes:
 *      Although the packet size is normally configured per unit,
 *      the option of using a different size is given here.  If
 *      pkt_size <= 0, then the default packet size for the unit
 *      is used.
 */

int
_bcm_common_rx_alloc(int unit, int pkt_size, uint32 flags, void **buf)
{
    if (!RX_INIT_DONE(unit)) {
        *buf = NULL;                    
        return BCM_E_INIT;
    }

    if (pkt_size <= 0) {
        pkt_size = rx_ctl[unit]->user_cfg.pkt_size;
    }

    return rx_ctl[unit]->user_cfg.rx_alloc(unit, pkt_size, flags, buf);
}


/*
 * Function:
 *      bcm_rx_free
 * Purpose:
 *      Gateway to configured RX free function.  Generally, packet
 *      buffer was allocated with bcm_rx_alloc.
 * Parameters:
 *      unit - Unit reference
 *      pkt - Packet to free
 * Returns:
 * Notes:
 *      In particular, packets stolen from RX with BCM_RX_HANDLED_OWNED
 *      should use this to free packets.
 */

int
_bcm_common_rx_free(int unit, void *pkt_data)
{
#if defined(BROADCOM_DEBUG)
    if (rx_ctl[unit] == NULL || !rx_ctl[unit]->user_cfg.rx_free) {
        return BCM_E_MEMORY;
    }
#endif /* BROADCOM_DEBUG */

    if (pkt_data != NULL) {
        rx_ctl[unit]->user_cfg.rx_free(unit, pkt_data);
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_rx_free_enqueue
 * Purpose:
 *      Queue a packet to be freed by the RX thread.
 * Parameters:
 *      unit - Unit reference
 *      pkt - Packet to free
 * Returns:
 * Notes:
 *      This may be called in interrupt context to queue
 *      a packet to be freed.
 *
 *      Assumes pkt_data is 32-bit aligned.
 *      Uses the first word of the freed data as a "next" pointer
 *      for the free list.
 */

#define PKT_TO_FREE_NEXT(data) (((void **)data)[0])
int
_bcm_common_rx_free_enqueue(int unit, void *pkt_data)
{
    if (pkt_data == NULL) {
        return BCM_E_PARAM;
    }

    if (!RX_INIT_DONE(unit) || rx_control.rx_tid == NULL) {
        return BCM_E_INIT;
    }

    RX_INTR_LOCK;
    PKT_TO_FREE_NEXT(pkt_data) = (void *)(rx_ctl[unit]->free_list);
    rx_ctl[unit]->free_list = pkt_data;
    RX_INTR_UNLOCK;

    RX_THREAD_NOTIFY(unit);

    return BCM_E_NONE;
}


/****************************************************************
 *
 * Global (all COS) and per COS rate limiting configuration
 *
 ****************************************************************/


/*
 * Functions:
 *      bcm_rx_burst_set, get; bcm_rx_rate_set, get
 *      bcm_rx_cos_burst_set, get; bcm_rx_cos_rate_set, get;
 *      bcm_rx_cos_max_len_set, get
 * Purpose:
 *      Get/Set the global and per COS limits:
 *           rate:      Packets/second
 *           burst:     Packets (max tokens in bucket)
 *           max_len:   Packets (max permitted in queue).
 * Parameters:
 *      unit - Unit reference
 *      cos - For per COS functions, which COS queue affected
 *      pps - Rate in packets per second (OUT for get functions)
 *      burst - Burst rate for the system in packets (OUT for get functions)
 *      max_q_len - Burst rate for the system in packets (OUT for get functions)
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      PPS must be >= 0 and
 *      Max queue length must be >= 0;
 *          otherwise param error.
 *
 *      PPS == 0 -> rate limiting disabled.
 *      max_q_len == 0 -> no limit on queue length (not recommended)
 */

int
_bcm_common_rx_rate_set(int unit, int pps)
{
    RX_INIT_CHECK(unit);

    if (pps < 0) {
        return BCM_E_PARAM;
    }
    RX_PPS(unit) = pps;

    return BCM_E_NONE;
}

int
_bcm_common_rx_rate_get(int unit, int *pps)
{
    RX_INIT_CHECK(unit);

    if (pps) {
        *pps = RX_PPS(unit);
    }

    return BCM_E_NONE;
}

int 
_bcm_common_rx_cpu_rate_set(int unit, int pps)
{
    if (pps < 0) {
        return BCM_E_PARAM;
    }
    rx_control.system_pps = pps;

    return BCM_E_NONE;
}

int 
_bcm_common_rx_cpu_rate_get(int unit, int *pps)
{
    if (pps) {
        *pps = rx_control.system_pps;
    } 
    return BCM_E_NONE;
}


int
_bcm_common_rx_burst_set(int unit, int burst)
{
    RX_INIT_CHECK(unit);

    RX_BURST(unit) = burst;
    RX_TOKENS(unit) = burst;

    return BCM_E_NONE;
}

int
_bcm_common_rx_burst_get(int unit, int *burst)
{
    RX_INIT_CHECK(unit);

    if (burst) {
        *burst = RX_BURST(unit);
    }
    return BCM_E_NONE;
}

int
_bcm_common_rx_cos_rate_set(int unit, int cos, int pps)
{
    int i;

    if (!LEGAL_COS(cos) || pps < 0) {
        return BCM_E_PARAM;
    }

    RX_INIT_CHECK(unit);
    if (cos == BCM_RX_COS_ALL) {
        for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
            RX_COS_PPS(unit, i) = pps;
        }
    } else {
        RX_COS_PPS(unit, cos) = pps;
    }

    return BCM_E_NONE;
}

int
_bcm_common_rx_cos_rate_get(int unit, int cos, int *pps)
{
    RX_INIT_CHECK(unit);
    if (pps) {
        *pps = RX_COS_PPS(unit, cos);
    }

    return BCM_E_NONE;
}

int
_bcm_common_rx_cos_burst_set(int unit, int cos, int burst)
{
    rx_queue_t *queue;
    int i;

    if (!LEGAL_COS(cos)) {
        return BCM_E_PARAM;
    }

    RX_INIT_CHECK(unit);
    if (cos == BCM_RX_COS_ALL) {
        for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
            queue = RX_QUEUE(unit, i);
            queue->burst = burst;
            queue->tokens = burst;
        }
    } else {
        queue = RX_QUEUE(unit, cos);
        queue->burst = burst;
        queue->tokens = burst;
    }

    return BCM_E_NONE;
}

int
_bcm_common_rx_cos_burst_get(int unit, int cos, int *burst)
{
    RX_INIT_CHECK(unit);
    if (burst) {
        *burst = RX_COS_BURST(unit, cos);
    }

    return BCM_E_NONE;
}

int
_bcm_common_rx_cos_max_len_set(int unit, int cos, int max_q_len)
{
    if (!LEGAL_COS(cos) || max_q_len < 0) {
        return BCM_E_PARAM;
    }

    RX_INIT_CHECK(unit);
    if (cos == BCM_RX_COS_ALL) {
        for (cos = 0; cos <= RX_QUEUE_MAX(unit); cos++) {
            RX_COS_MAX_LEN(unit, cos) = max_q_len;
        }
    } else {
        RX_COS_MAX_LEN(unit, cos) = max_q_len;
    }

    return BCM_E_NONE;
}

int
_bcm_common_rx_cos_max_len_get(int unit, int cos, int *max_q_len)
{
    RX_INIT_CHECK(unit);
    if (max_q_len) {
        *max_q_len = RX_COS_MAX_LEN(unit, cos);
    }

    return BCM_E_NONE;
}

/****************************************************************
 *
 * The non-interrupt thread
 *
 ****************************************************************/

STATIC int
rx_thread_start(int unit)
{
    int priority;

    /* Timer/Event semaphore thread sleeping on. */
    if (NULL == rx_control.pkt_notify) {
        rx_control.pkt_notify = sal_sem_create("RX pkt ntfy", sal_sem_BINARY, 0);
        if (NULL == rx_control.pkt_notify) {
            return (BCM_E_MEMORY);
        }
        rx_control.pkt_notify_given = FALSE;
    }

    /* RX start/stop on one of the units protection mutex. */
    if (NULL == rx_control.system_lock) {
        rx_control.system_lock = sal_mutex_create("RX system lock");
        if (NULL == rx_control.system_lock) {
            sal_sem_destroy(rx_control.pkt_notify);
            return (BCM_E_MEMORY);
        }
    }

    if (SOC_UNIT_VALID(unit)) {
        priority = soc_property_get(unit,
                                    spn_BCM_RX_THREAD_PRI,
                                    RX_THREAD_PRI_DFLT);
    } else {
        priority = RX_THREAD_PRI_DFLT;
    }

    if (NULL == rx_control.rx_sched_cb) {
        rx_control.rx_sched_cb = _BCM_RX_SCHED_DEFAULT_CB;
    }

    /* Start rx thread. */
    rx_control.rx_tid = sal_thread_create("bcmRX",
                                          SAL_THREAD_STKSZ,
                                          priority,
                                          rx_pkt_thread, NULL);
    /* Thread creation error handling. */
    if (NULL == rx_control.rx_tid) {
        sal_sem_destroy(rx_control.pkt_notify);
        sal_mutex_destroy(rx_control.system_lock);
        sal_mutex_destroy(rx_control.start_lock);
        rx_control.pkt_notify = NULL;
        rx_control.system_lock = NULL;
        return BCM_E_MEMORY;
    }
    return BCM_E_NONE;
}


/*
 * Calculate number of tokens that should be added to a bucket
 *
 * Add pps tokens per second to the bucket (up to max_burst).
 * Find the number of us per token (assuming pps is much less than 1M).
 *
 *     (us per token) = 1000000 us/sec / (pps tokens/sec)
 *
 * For example, for 2000 pps, add one token every 500 us.  We then
 * check the elapsed time since the last addition and divide that
 * by us per token.
 *
 * That is:
 *
 *     (tokens to add) = (elapsed us since last add) / (us per token)
 *                     = (cur - last) / (1000000 / pps)
 *                     = ((cur - last) * pps) / 1000000
 *
 * Note:  When pps > 10000, there can be an integer overflow from
 * the multiplication.  We fix this by changing order of the calculation
 * depending on the size of pps.
 */

#define us_PER_TOKEN(pps) (1000000 / (pps))

static sal_usecs_t last_fill_check = 0;    /* Last time tokens checked */

/*
 * Function:
 *      _token_update
 * Purpose:
 *      Update the tokens for one bucket
 * Parameters:
 *      cur_time - current time in us
 *      pps - rate limit for bucket; 0 means disabled
 *      burst - burst limit for bucket
 *      token_bucket - (IN/OUT) bucket to be updated
 *      last_fill - (IN/OUT) last time this bucket was updated
 * Returns:
 *      BCM_E_XXX
 */

STATIC INLINE void
_token_update(sal_usecs_t cur_time, int pps, int burst,
              volatile int *token_bucket, sal_usecs_t *last_fill)
{
    int new_tokens;
    int max_add;
    int bucket_top;
    int time_diff;

    max_add = bucket_top = pps > burst ? pps : burst;

    time_diff = SAL_USECS_SUB(cur_time, *last_fill);
    if (time_diff < 0) {
        /* In case the clock has changed, update last fill to cur_time */
        *last_fill = cur_time;
        return;
    }
    if (time_diff == 0) {
        return;
    }

    /* Cap the number of additions by the fraction of a second refreshed */
    if (time_diff < 1000000) {
        max_add = pps / (1000000/time_diff);
    }

    /* Not enough time passed to add tokens at this rate */
    if (max_add == 0) {
        return;
    }

    /* We use commutative multiplication law to avoid integer 
       overflow for the following calculation 
            time_diff * pps / 1000000
       If time gap is bigger than two regular refill times
       (2 * BCM_RX_TOKEN_CHECK_US_DEFAULT) = 200000 micro
           we use (time_diff / 10000)
       For high  rate flows (over 1000 pps) we use pps / 100
    */ 
    if (*token_bucket < bucket_top) {
        new_tokens = ((time_diff > (BCM_RX_TOKEN_CHECK_US_DEFAULT << 1)) ? \
                      ((pps < 1000) ?  (((time_diff / 10000) * pps) / 100) : \
                       ((time_diff / 10000) * (pps / 100))) : \
                       ((pps < 1000) ?  ((time_diff * pps) / 1000000) : \
                       ((time_diff * (pps / 100)) / 10000)));

        if (new_tokens > max_add) {
            new_tokens = max_add;
        }
        if (new_tokens > 0) {
            *token_bucket += new_tokens;
            if (*token_bucket > bucket_top) {
                *token_bucket = bucket_top;
            }
            *last_fill = cur_time;
        }
    }
}

/* Add tokens to all active token buckets */
STATIC INLINE void
_all_tokens_update(sal_usecs_t cur_time)
{
    int cos, unit;
    rx_queue_t *queue;

    for (unit = 0; unit < BCM_CONTROL_MAX; unit++) {
        if (RX_IS_SETUP(unit)) {
            for (cos = 0; cos <= RX_QUEUE_MAX(unit); cos++) {
                queue = RX_QUEUE(unit, cos);
                if (queue->pps > 0) {
                    _token_update(cur_time, queue->pps, queue->burst,
                                  &queue->tokens, &queue->last_fill);
                }
            }
            if (RX_PPS(unit) > 0) {
                _token_update(cur_time, RX_PPS(unit), RX_BURST(unit),
                              &rx_ctl[unit]->tokens, &rx_ctl[unit]->last_fill);
            }
        }
    }

    /* Check system wide rate limiting */
    if (rx_control.system_pps > 0) {
        _token_update(cur_time, rx_control.system_pps, 0,
                      &rx_control.system_tokens,
                      &rx_control.system_last_fill);
    }

    last_fill_check = cur_time;
}

STATIC void
rx_update_tokens(void)
{
    sal_usecs_t cur_time;

    /* Get current time. */
    cur_time = sal_time_usecs();

    RX_INTR_LOCK;

    /* Protection agains clock moving back. */
    if (SAL_USECS_SUB(cur_time, last_fill_check) < 0) {
        last_fill_check = cur_time;
    }

    /* Time based tokens refill check. */
    if (SAL_USECS_SUB(cur_time, last_fill_check) >= bcm_rx_token_check_us) {
        _all_tokens_update(cur_time);
    }

    RX_INTR_UNLOCK;
}

/* Free all buffers listed in pending free list */
STATIC void
rx_free_queued(int unit)
{
    void *free_list, *next_free;

    /* Steal list of pkts to be freed for unit */
    RX_INTR_LOCK;
    free_list = (bcm_pkt_t *)rx_ctl[unit]->free_list;
    rx_ctl[unit]->free_list = NULL;
    RX_INTR_UNLOCK;

    while (free_list) {
        next_free = PKT_TO_FREE_NEXT(free_list);
        bcm_rx_free(unit, free_list);
        free_list = next_free;
    }
}

/*
 * Thread support functions
 */

/* Start a chain and update the tokens */
STATIC INLINE int
rx_chain_start(int unit, int chan, dv_t *dv)
{
    int rv = BCM_E_NONE;

    LOG_VERBOSE(BSL_LS_BCM_RX,
                (BSL_META_U(unit,
                            BSL_META_U(unit, "RX: Starting %d/%d/%d\n")),
                 unit, chan, DV_INDEX(dv)));

    if (!RX_INIT_DONE(unit) || !rx_control.thread_running) {
        /* Revert state in case scheduled */
        DV_STATE(dv) = DV_S_FILLED;
        return BCM_E_NONE;
    }

    if (!SOC_UNIT_VALID(unit)) {
        
        return BCM_E_INTERNAL;
    }

    /* Start the DV */
    DV_STATE(dv) = DV_S_ACTIVE;
    DV_ABORT_CLEANUP(dv) = soc_feature(unit, soc_feature_rxdma_cleanup);

    if ((rv = soc_dma_start(unit, chan, dv)) < 0) {
        DV_STATE(dv) = DV_S_ERROR;
        RX_ERROR((BSL_META_U(unit, "RX: Could not start dv, u %d, chan %d\n"),
                  unit, chan));
    }

    return rv;
}

/*
 * The DV (chain) given is ready
 * to go.  If rate limiting allows that, start the chain.  If
 * not, schedule the chain in the future.
 *
 * This is implemented by:  First updating the global (all COS) and
 * per channel token buckets.  If a bucket is negative, this
 * indicates a time in the future when the DV can be scheduled,
 * based on the pkts/sec of the limit.
 *
 * Calculate both (global and channel) start times and take the
 * later one.
 *
 * NOTE:  This routine should only be called from inside of
 * the rx thread b/c it affects variables that that thread
 * depends on for timing.
 */
#define USEC_DELAY(tokens, ppc, pps) \
    ((-(tokens) + (ppc)) * us_PER_TOKEN(pps))
STATIC INLINE int
rx_chain_start_or_sched(int unit, int chan, dv_t *dv)
{
    int global_usecs = 0;
    int system_usecs = 0;
    sal_usecs_t cur_time;

    LOG_VERBOSE(BSL_LS_BCM_RX,
                (BSL_META_U(unit,
                            BSL_META_U(unit, "RX: Chain. glob tok %d.\n")),
                 RX_TOKENS(unit)));

    RX_INTR_LOCK;

    /*
     * Decrement the token buckets for global limits.
     * If negative, we must schedule the packet in the
     * future.  The time to schedule is based on the number of usecs
     * needed to get the bucket up to pkts/chain again.
     */
    if (rx_control.system_pps > 0) { /* System rate limiting */
        rx_control.system_tokens -= RX_PPC(unit);
        if (rx_control.system_tokens < 0) {
            system_usecs = USEC_DELAY(rx_control.system_tokens,
                                      RX_PPC(unit), rx_control.system_pps);
        }
    }
    if (RX_PPS(unit)) { /* Global rate limiting is on */
        RX_TOKENS(unit) -= RX_PPC(unit);
        if (RX_TOKENS(unit) < 0) {
            global_usecs = USEC_DELAY(rx_ctl[unit]->tokens,
                                      RX_PPC(unit), RX_PPS(unit));
        }
    }

    RX_INTR_UNLOCK;

    /* Use the maximum delay */
    if (global_usecs < system_usecs) {
        global_usecs = system_usecs;
    }

    if (!global_usecs) { /* Can start immediately */
        return rx_chain_start(unit, chan, dv);
    } else {

        /***** The DV must be scheduled into the future *****/

        DV_STATE(dv) = DV_S_SCHEDULED;
        cur_time = sal_time_usecs();
        DV_SCHED_TIME(dv) = cur_time;
        DV_TIME_DIFF(dv) = global_usecs;
        SLEEP_MIN_SET(global_usecs);
        RX_INFO((BSL_META_U
                 (unit, "RX: Scheduling %d/%d/%d in %d us; "
                  "cur %u; sleep %u\n"), unit, chan, DV_INDEX(dv),
                 global_usecs, cur_time, rx_control.sleep_cur));
    }

    return BCM_E_NONE;
}

/*
 * Based on the state of the DV, carry out an action.
 * If it is scheduled, check if it may be started.
 *
 * NOTE:  This routine should only be called from inside of
 * the rx thread b/c it affects variables that that thread
 * depends on for timing.
 */

STATIC INLINE int
rx_update_dv(int unit, int chan, dv_t *dv)
{
    sal_usecs_t cur_usecs;
    int sched_usecs;
    int rv = BCM_E_NONE;

    /* If no real callouts, don't bother starting DVs */
    if (!SOC_KNET_MODE(unit) &&
        (!rx_control.thread_running || NO_REAL_CALLOUTS(unit))) {
        if (DV_STATE(dv) == DV_S_SCHEDULED) {
            DV_STATE(dv) = DV_S_FILLED;
        }
        return BCM_E_NONE;
    }

    assert(dv);

    switch (DV_STATE(dv)) {
    case DV_S_NEEDS_FILL:
        rx_dv_fill(unit, chan, dv);
        /* If fill succeeded, try to schedule/start; otherwise break. */
        if (DV_STATE(dv) != DV_S_FILLED) {
            break;
        }
        /* Fall thru:  Ready to be scheduled/started */
    case DV_S_FILLED: /* See if we can start or schedule this DV */
         rv = rx_chain_start_or_sched(unit, chan, dv);
        break;
    case DV_S_SCHEDULED:
        cur_usecs = sal_time_usecs();

        /* How much time before next schedule? */
        sched_usecs = DV_FUTURE_US(dv, cur_usecs);

        if (sched_usecs <= 0) {
            /* Start the dv */
            RX_INFO((BSL_META_U
                     (unit, "RX: Starting scheduled %d/%d/%d, diff %d "
                      "@ %u\n"), unit, chan, DV_INDEX(dv),
                     sched_usecs, cur_usecs));
            rv = rx_chain_start(unit, chan, dv);
        } else {
            /* Still not ready to be started; set sleep min time */
            RX_INFO((BSL_META_U
                     (unit, "RX: %d/%d/%d not ready at %u, diff %d\n"),
                      unit, chan, DV_INDEX(dv), cur_usecs, sched_usecs));
            SLEEP_MIN_SET(sched_usecs);
        }
        break;
    default: /* Don't worry about other states here. */
        break;
    }

    return rv;
}

/*
 * Function:
 *      rx_thread_dv_check
 * Purpose:
 *      Check if any DVs need refilling, starting or scheduling
 * Parameters:
 *      unit - unit to examine
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      To avoid starvation, a "current" channel is maintained that
 *      rotates through the possible channels.
 *
 *      Does nothing for remote units, but safe to call on them.
 */
#define _CUR_CHAN(unit) (rx_ctl[unit]->cur_chan)

STATIC void
rx_thread_dv_check(int unit)
{
    int chan;
    int i, j;

    if (RX_IS_REMOTE(unit) || !SOC_UNIT_VALID(unit)) {
        return;
    }

    chan = _CUR_CHAN(unit);

    for (j = 0; j < BCM_RX_CHANNELS; j++) {
        if (RX_CHAN_RUNNING(unit, chan)) {
            for (i = 0; i < RX_CHAINS(unit, chan); i++) {
                rx_update_dv(unit, chan, RX_DV(unit, chan, i));
            }
        }
        chan = (chan + 1) % BCM_RX_CHANNELS;
    }

    _CUR_CHAN(unit) = (_CUR_CHAN(unit) + 1) % BCM_RX_CHANNELS;
}

#undef _CUR_CHAN
/*
 * Function:
 *      rx_thread_pkts_process
 * Purpose:
 *      Process all pending packets for a particular COS
 * Parameters:
 *      unit  - (IN) BCM device number. 
 *      cos   - (IN) Queue to read packets from. 
 *      count - (IN) 
 * Returns:
 *       BCM_E_XXX
 */

STATIC int
rx_thread_pkts_process(int unit, int cos, int count)
{
    bcm_pkt_t *pkt_list = NULL;
    bcm_pkt_t *next_pkt;
    rx_queue_t *queue;
    int drop_all = FALSE;             /* Drop all flag.           */
    int rv = BCM_E_NONE;              /* Operation return status. */

    /*
     * Steal the queue's packets; if rate limiting on, just take as many
     * as can be handled.
     */
    queue = RX_QUEUE(unit, cos);

    if ((count > queue->count) || (count < 0)) {
        count = queue->count;
    }

    if (0 == count) {
        return (BCM_E_NONE);
    }

    if ((queue->pps > 0) && (count > queue->tokens)) {
        
        _Q_STEAL_ALL(queue, pkt_list);
        drop_all = TRUE;
        /*  count = queue->tokens; */
    } else {
    _Q_STEAL(queue, pkt_list, count);
    }

    if (pkt_list == NULL) {
        return (BCM_E_NONE);   /* Strange but Queue is empty.*/
    }

    /* Process packets */
    while (pkt_list) {
#if defined(BCM_RXP_DEBUG)
        bcm_rx_pool_own(pkt_list->alloc_ptr, "rx_dequeue");
#endif
        next_pkt = pkt_list->_next;

        /* Make sure scheduler properly enforces queue rate limiting. */
        if (queue->pps > 0 && (FALSE == drop_all)) {
            if (queue->tokens > 0) {
                queue->tokens--;
            } 
        }

        /* Process 1 packet from the queue. */

        if (drop_all) {
            if (RX_IS_RCPU(unit)) {
                bcm_pkt_rx_free(unit, pkt_list);    /* Free the packet */
            } else {
                MARK_PKT_PROCESSED(pkt_list, unit, pkt_list->_dv);
            }
            queue->rate_disc++;
        } else {
#ifdef BCM_RCPU_SUPPORT
            if (RX_IS_RCPU(unit)) {
                rx_rcpu_process_packet(unit, pkt_list);
            } else 
#endif /* BCM_RCPU_SUPPORT */
            {
                rx_process_packet(unit, pkt_list);
            }
        }

        pkt_list = next_pkt;

        _BCM_RX_CHECK_THREAD_DONE;
    }
#if 0
    LOG_DEBUG(BSL_LS_BCM_RX,
              (BSL_META_U(unit,
                          "Processed (%d) packets"),
               count));
#endif
    return rv;
}

/*
 * Clean up Queues for a RCPU unit.
 */
STATIC INLINE void
rx_rcpu_cleanup_queues(int unit)
{
    int i;
    bcm_pkt_t *pkt_list = NULL;
    bcm_pkt_t *next_pkt;
    rx_queue_t *queue;

    rx_free_queued(unit);
    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
        queue = RX_QUEUE(unit, i);
        _Q_STEAL_ALL(queue, pkt_list);
        while (pkt_list) {
            next_pkt = pkt_list->_next;
            bcm_pkt_free(unit, pkt_list);
            pkt_list = next_pkt;
        }
    }
}

/*
 * Clean up queues on thread exit.  The queued packets still belong
 * to DVs, so they'll be freed when rx_dv_dealloc is called.
 */
STATIC INLINE void
rx_cleanup_queues(int unit)
{
    int i;
    rx_queue_t *queue;

    rx_free_queued(unit);
    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
        queue = RX_QUEUE(unit, i);
        if (!_Q_EMPTY(queue)) {
            queue->count = 0;
            queue->head = NULL;
            queue->tail = NULL;
        }
    }
}

/* Called on error or thread exit to clean up DMA, etc. */
STATIC void
rx_cleanup(int unit)
{
    int chan;

    if (RX_IS_RCPU(unit)) {
        rx_rcpu_cleanup_queues(unit);
        return;
    }

    if (rx_ctl[unit] != NULL) {
        if (SOC_UNIT_VALID(unit)) {
            /* For each RX channel, abort running DVs, dealloc DVs,
             * and mark not running */
            FOREACH_SETUP_CHANNEL(unit, chan) {
                /* Abort running DVs on this RX channel*/
                if (soc_dma_abort_channel_total(unit, chan) < 0) {
                    LOG_WARN(BSL_LS_BCM_RX,
                             (BSL_META_U(unit,
                                         BSL_META_U
                                          (unit, "RX: Error aborting DMA channel %d\n")),
                              chan));
                }
                /* Sleep .1 sec to allow abort to complete and
                 * pending pkts thru */
                sal_usleep(100000);

                rx_channel_shutdown(unit, chan);
            }
        }

        rx_cleanup_queues(unit);
    }
}

/*
 * Function:
 *      _bcm_rx_default_scheduler
 * Purpose:
 *      Schedule number of packets to be processed 
 *      on a specific unit, queue pair.
 * Parameters:
 *      unit         - (IN)  BCM device number. 
 *      sched_unit   - (OUT) Device to process packets. 
 *      sched_cosq   - (OUT) Queue to process packets. 
 *      sched_count  - (OUT) Number of packets to process.
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_rx_default_scheduler(int unit, int *sched_unit, 
                          bcm_cos_queue_t *sched_cosq, int *sched_count) 
{
    int pkt_in_queue;         /* Number of packets waiting.   */ 
    int pkt_limit;            /* Max packets to schedule due  */ 
                              /* to rate limiting.            */ 
    static bcm_cos_queue_t cosq_idx = -1; /* Queue iterator.  */
    static int unit_next = -1;/* Bcm device iterator.         */
    
    
    /* Get maximum cosq supported by the system. */
    if ((-1 == unit_next) &&
        (cosq_idx == -1)) { 
        /* Queues & units iteration initialization. */
        cosq_idx = rx_control.system_cosq_max;
        unit_next = unit;
    } else {
        /* Proceed to the next unit. */
        BCM_IF_ERROR_RETURN(_bcm_common_rx_unit_next_get(unit_next, &unit_next));
        /* If no other unit found proceed to the next queue.*/
        if (-1 == unit_next) {
            unit_next = unit;
            cosq_idx--;
        }
    }

    for (; cosq_idx >= 0; cosq_idx--) {
        /* Check if thread exit request was issued. */
        _BCM_RX_CHECK_THREAD_DONE;

        /* Check if queue is supported by the system. */
        for(;unit_next != -1;) {
            /* Check if thread exit request was issued. */
            _BCM_RX_CHECK_THREAD_DONE;

            /* Check if queue is supported by the device. */ 
            if (cosq_idx > RX_QUEUE_MAX(unit_next)) {
                BCM_IF_ERROR_RETURN(_bcm_common_rx_unit_next_get(unit_next, &unit_next));
                continue;
            }

            /* Get number of packets awaiting processing. */
            BCM_IF_ERROR_RETURN
                (_bcm_common_rx_queue_packet_count_get(unit_next, 
                                                   cosq_idx, &pkt_in_queue));
            /* Queue rate limiting enforcement. */
            if (pkt_in_queue) {
                BCM_IF_ERROR_RETURN
                    (_bcm_common_rx_queue_rate_limit_status_get(unit_next, cosq_idx, 
                                                            &pkt_limit));
                *sched_unit = unit_next;
                *sched_cosq = cosq_idx;
                if (BCM_RX_SCHED_ALL_PACKETS == pkt_limit) {
                    *sched_count = pkt_in_queue;
                } else if (pkt_limit < pkt_in_queue) {
                    *sched_count = pkt_limit;;
                }  else {
                    *sched_count = pkt_in_queue;
                }
                return (BCM_E_NONE);
            }
            BCM_IF_ERROR_RETURN(_bcm_common_rx_unit_next_get(unit_next, &unit_next));
        }
        unit_next = unit;
    }

    /* Prep for the next scheduling cycle. */
    cosq_idx = -1;   
    unit_next = -1;   
    /* Done indicator. */
    *sched_count = 0;

    return (BCM_E_NONE);
}

/*
 * Function:
 *      rx_pkt_thread
 * Purpose:
 *      User level thread that handles packets and DV refills
 * Parameters:
 *      param    - Unit number for this thread
 * Notes:
 *      This can get awoken for one or more of the following reasons:
 *      1.  Descriptor(s) are done (packets ready to process)
 *      2.  RX stop is called ending processing
 *      3.  Interrupt processing has handled packets and DV may be
 *          ready for refill.
 *      4.  Periodic check in case memory has freed up.
 *
 *      When a DV is scheduled to start, it may change the
 * next waking time of this thread by setting sleep_cur.
 */


STATIC void
rx_pkt_thread(void *param)
{
    int unit = 0;
    int unit_update_idx;
    bcm_cos_queue_t cosq; 
    int count;

    RX_INFO((BSL_META_U(unit, "RX:  Packet thread starting\n")));

    INIT_SLEEP;
    /* Sleep on semaphore. */
    while (rx_control.thread_running) {

        /* Refill buckets */
        rx_update_tokens();

        /* Make sure scheduler callback was registered. */
        if (NULL == rx_control.rx_sched_cb) {
            break;
        }

        /* Lock system rx start/stop mechanism. */
        _BCM_RX_SYSTEM_LOCK;
        
        /* Check if system active units list update is required.  */
        if (rx_control.system_flags & BCM_RX_CTRL_ACTIVE_UNITS_UPDATE) {
            _bcm_rx_unit_list_update(); 
            rx_control.system_flags &= ~BCM_RX_CTRL_ACTIVE_UNITS_UPDATE;
        }

        unit_update_idx =  rx_control.rx_unit_first;

        /* Schedule number of packets to be processed. */
        while (BCM_SUCCESS(rx_control.rx_sched_cb(rx_control.rx_unit_first, 
                                                  &unit, &cosq, &count))) {
            /* Check for  completion. */
            if (!count) {
                break;
            }
            /* check unit */
            if (unit < 0 || unit >= BCM_CONTROL_MAX) {
                break;
            }
            /* Process packets based on scheduler allocation. */
            if (BCM_FAILURE(rx_thread_pkts_process(unit, cosq, count))) {
                RX_INFO((BSL_META_U
                         (unit, "Packet rx failed - check the scheduler.\n")));
            }
            /* 
             *  Free queued packets and check DVs for all units 
             *  in rx started list before the scheduled one.
             */
            for (;-1 != unit_update_idx;) {
                rx_free_queued(unit_update_idx);
                rx_thread_dv_check(unit_update_idx);
                if (unit_update_idx == unit) {
                    break;
                }
                _bcm_common_rx_unit_next_get(unit_update_idx, &unit_update_idx);
            }
            _BCM_RX_CHECK_THREAD_DONE;
        }
              
        /* 
         *  Free queued packets and check DVs for all units 
         *  in rx started list after last scheduled unit. 
         */
        for (;-1 != unit_update_idx;) {
            /* Free queued packets and check DVs */
            rx_free_queued(unit_update_idx);
            rx_thread_dv_check(unit_update_idx);
            _bcm_common_rx_unit_next_get(unit_update_idx, &unit_update_idx);
        }

        /* Unlock system rx start/stop mechanism. */
        _BCM_RX_SYSTEM_UNLOCK;

        _BCM_RX_CHECK_THREAD_DONE;

        SLEEP_MIN_SET(BASE_SLEEP_VAL);

        sal_sem_take(rx_control.pkt_notify, rx_control.sleep_cur);
        rx_control.pkt_notify_given = FALSE;

        INIT_SLEEP;
    }

    for (unit = 0; unit < BCM_CONTROL_MAX; unit++) {
        if (RX_IS_LOCAL(unit) || RX_IS_RCPU(unit)) {
            rx_cleanup(unit);
        }
    }
    rx_control.thread_exit_complete = TRUE;
    RX_INFO((BSL_META_U(unit, "RX: Packet thread exitting\n")));

    sal_thread_exit(0);
}

/****************************************************************
 *
 * Interrupt handling routines:
 *     Packet done
 *     Chain done
 *
 ****************************************************************/


/* Multiple DCB DMA fillin routine */

STATIC INLINE void
multi_dcb_fillin(int unit, bcm_pkt_t *pkt, dv_t *dv, int idx)
{
#ifdef BCM_XGS_SUPPORT
    uint8 *vlan_store;
    uint16 vlan_ctl_tag;

    if (SOC_IS_XGS12_FABRIC(unit)) { /* Handle HiGig header and VLAN tag */

        /* Put XGS hdr into pkt->_higig */
        sal_memcpy(pkt->_higig, SOC_DV_HG_HDR(dv, idx),
                   sizeof(soc_higig_hdr_t));

        /* Put the VLAN info either into pkt or pkt->_vtag as per flags */
        vlan_ctl_tag = soc_higig_field_get(unit,
                                           (soc_higig_hdr_t *)pkt->_higig,
                                           HG_vlan_tag);
        vlan_store = BCM_PKT_VLAN_PTR(pkt);
        vlan_store[0] = 0x81;
        vlan_store[1] = 0x00;
        vlan_store[2] = vlan_ctl_tag >> 8;
        vlan_store[3] = vlan_ctl_tag & 0xff;

        return;
    }
#endif /* BCM_XGS_SUPPORT */

    if (BCM_PKT_HAS_SLTAG(pkt)) {
        /* SL tag was DMA'd to DV buffer; copy to pkt->_sltag */
        sal_memcpy(pkt->_sltag, SOC_DV_SL_TAG(dv, idx), sizeof(uint32));
    }

    if (BCM_PKT_RX_VLAN_TAG_STRIP(pkt)) {
        /* VLAN tag was DMA'd to DV buffer; copy to pkt->_vtag */
        sal_memcpy(pkt->_vtag, SOC_DV_VLAN_TAG(dv, idx), sizeof(uint32));
    }
}

/* Move data around if it was DMA'd in a single data buffer */

STATIC INLINE void
single_dcb_fillin(int unit, bcm_pkt_t *pkt)
{
    uint8 tmp_buf[20];

    /* Single buffer was used for packet; move data as necessary */
#ifdef BCM_XGS_SUPPORT
    uint16 vlan_ctl_tag;
    uint8 *pkt_ptr;

    if (SOC_IS_XGS12_FABRIC(unit)) { /* Handle HiGig header and VLAN tag */
        sal_memcpy(pkt->_higig, pkt->_pkt_data.data, 12);
        pkt->_pkt_data.data += 12;  /* 12 hg hdr */
        if (!BCM_PKT_RX_VLAN_TAG_STRIP(pkt)) {
            /* Move L2 addrs back 4 bytes; add VLAN data into pkt */
            sal_memcpy(tmp_buf, pkt->_pkt_data.data, 12);
            pkt->_pkt_data.data -= 4;  /* Add space back for vlan */
            sal_memcpy(pkt->_pkt_data.data, tmp_buf, 12);
            pkt_ptr = pkt->_pkt_data.data;
            pkt_ptr[12] = 0x81;
            pkt_ptr[13] = 0x00;
            vlan_ctl_tag = soc_higig_field_get(unit,
                                               (soc_higig_hdr_t *)pkt->_higig,
                                               HG_vlan_tag);

            pkt_ptr[14] = vlan_ctl_tag >> 8;
            pkt_ptr[15] = vlan_ctl_tag & 0xff;
        }

        /* Note:  XGS and SL combinations not currently supported.  */
        return;
    }
#endif

    /* No HG header */
    if (BCM_PKT_HAS_SLTAG(pkt)) { /* Strip SL tag */
        sal_memcpy(pkt->_sltag, &pkt->_pkt_data.data[16], 4);
        sal_memcpy(tmp_buf, pkt->_pkt_data.data, 16);
        pkt->_pkt_data.data += 4;  /* Eliminated 4 byte SL tag */
        sal_memcpy(pkt->_pkt_data.data, tmp_buf, 16);
    }
    
    if (BCM_PKT_RX_VLAN_TAG_STRIP(pkt)) { /* Strip VLAN tag */
        sal_memcpy(pkt->_vtag, &pkt->_pkt_data.data[12], 4);
        sal_memcpy(tmp_buf, pkt->_pkt_data.data, 12);
        pkt->_pkt_data.data += 4;  /* Eliminated 4 byte VLAN tag */
        sal_memcpy(pkt->_pkt_data.data, tmp_buf, 12);
    } 
}


/*
 * Function:
 *      rx_done_packet
 * Purpose:
 *      Process a packet done done notification.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      dv - pointer to dv structure active
 *      dcb - pointer to dcb completed.
 * Returns:
 *      Nothing.
 * Notes:
 *      This routine is called in interrupt context
 *
 *      We count on the non-interrupt thread to update
 *      the tokens for each queue.
 *
 *      Only called for local units.
 */

STATIC void
rx_done_packet(int unit, dv_t *dv, dcb_t *dcb)
{
    volatile bcm_pkt_t *pkt;
    int pkt_dcb_idx;
    int flags, chan;
    uint32 count;

    chan = DV_CHANNEL(dv);
    pkt_dcb_idx = SOC_DCB_PTR2IDX(unit, dcb, dv->dv_dcb) /
        RX_DCB_PER_PKT(unit, chan);
    pkt = DV_PKT(dv, pkt_dcb_idx);
    assert(pkt_dcb_idx == pkt->_idx);

    /* for jumbo packet */
    flags = SOC_DCB_INTRINFO(unit, dcb, 0, &count);
    if (0 == (flags & SOC_DCB_INFO_PKTEND)) {
        if (RX_CHAN_FLAGS(unit, chan) & BCM_RX_F_OVERSIZED_OK) {
            /* Accept truncated packet */
            pkt->flags |= BCM_RX_TRUNCATED;
        } else {
            /* No matter DCB mode is multi or single mode, the DV of packet
               will be free if the packet is !end & pkt_size > dma_len */
            rx_ctl[unit]->dcb_errs++;
            MARK_PKT_PROCESSED_LOCAL(unit, dv);
            return;
        }
    }
#ifdef INCLUDE_KNET
    /* Mask off indicator for kernel processing done */
    count &= ~SOC_DCB_KNET_DONE;
    if (count == 0) {
        MARK_PKT_PROCESSED_LOCAL(unit, dv);
        return;
    }
#endif

    rx_ctl[unit]->tot_pkts++;
    rx_ctl[unit]->pkts_since_start++;
    if (!rx_control.thread_running) {
        /* Thread is not running; Ignore packet */
        rx_ctl[unit]->thrd_not_running++;
        if (++DV_PKTS_PROCESSED(dv) == RX_PPC(unit)) {
            DV_STATE(dv) = DV_S_NEEDS_FILL;
        }
        return;
    }

    if (RX_CHAN_RUNNING(unit, chan)) {
        /* Do not process packet if an error is noticed. */
#ifdef  BCM_XGS3_SWITCH_SUPPORT
        /*
         * XGS 3 RX error as a result of previously aborting RX DMA 
         * First DCB in the RX packet must have Start bit set
         */
        int     abort_cleanup_done;

        if (DV_ABORT_CLEANUP(dv)) {
            dcb_t   *sop_dcb;

            DV_ABORT_CLEANUP(dv) = 0;

            sop_dcb = SOC_DCB_IDX2PTR(unit,
                              dv->dv_dcb,
                              pkt_dcb_idx * RX_DCB_PER_PKT(unit, chan));
            if (SOC_DCB_RX_START_GET(unit, sop_dcb)) {
                abort_cleanup_done = 1;
            } else {
                abort_cleanup_done = 0;
            }
        } else {
            abort_cleanup_done = 1;
        }
#else
        #define abort_cleanup_done   1
#endif
        if (!SOC_DCB_RX_ERROR_GET(unit, dcb) && abort_cleanup_done) {
            rx_intr_process_packet(unit, dv, dcb, (bcm_pkt_t *)pkt);
        } else {
            rx_ctl[unit]->dcb_errs++;
            MARK_PKT_PROCESSED_LOCAL(unit, dv);
        }
    } else {/* If not active, mark the packet as handled */
        rx_ctl[unit]->not_running++;
        MARK_PKT_PROCESSED_LOCAL(unit, dv);
    }
}


/*
 * Function:
 *      rx_done_chain
 * Purpose:
 *      Handle chain done interrupt
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      dv - pointer to dv completed.
 * Returns:
 *      Nothing
 * Notes:
 */

STATIC void
rx_done_chain(int unit, dv_t *dv)
{
    if (DV_STATE(dv) == DV_S_ACTIVE) {
        DV_STATE(dv) = DV_S_CHN_DONE;
    }

    if (DV_STATE(dv) == DV_S_NEEDS_FILL) {
        int chan = DV_CHANNEL(dv);
        rx_update_tokens();
        rx_dv_fill(unit, chan, dv);
        if (DV_STATE(dv) != DV_S_FILLED) {
            RX_THREAD_NOTIFY(unit);
        } else {
            (void)rx_chain_start_or_sched(unit, chan, dv);
        }
    }

    /* Might implement stall here */
}


STATIC INLINE void
pkt_len_get(int unit, int chan, bcm_pkt_t *pkt, dv_t *dv)
{
    int len = 0;
    int i;

    for (i = (pkt->_idx * RX_DCB_PER_PKT(unit, chan));
         i < (pkt->_idx + 1) * RX_DCB_PER_PKT(unit, chan); i++) {
        len += SOC_DCB_XFERCOUNT_GET(unit,
                     SOC_DCB_IDX2PTR(unit, dv->dv_dcb, i));
    }

    pkt->tot_len = len;
    pkt->pkt_len = len;

    /* Don't include some tags in "pkt_len" */
#ifdef  BCM_XGS_SUPPORT
    if (BCM_PKT_HAS_HGHDR(pkt)) {
        /*
         * HiGig pkts (Hercules only):  Do not include header length;
         * but, VLAN is inserted into the packet, so need to add that
         * back in.
         */
        pkt->pkt_len -= (sizeof(soc_higig_hdr_t) - sizeof(uint32));
    }
#endif  /* BCM_XGS_SUPPORT */
    if (BCM_PKT_HAS_SLTAG(pkt)) {
        pkt->pkt_len -= sizeof(uint32);
    }
    if (BCM_PKT_RX_CRC_STRIP(pkt)) {
        pkt->pkt_len -= sizeof(uint32);
    }
    if (BCM_PKT_RX_VLAN_TAG_STRIP(pkt)) {
        pkt->pkt_len -= sizeof(uint32);
    }
}

#ifdef BCM_XGS3_SWITCH_SUPPORT
#ifdef BCM_HIGIG2_SUPPORT

STATIC INLINE void
rx_higig2_gport_resolve(int unit, bcm_gport_t vp, int physical,
                        bcm_gport_t *gport)
{
    bcm_gport_t     l_gport = BCM_GPORT_INVALID;

    if (!physical) {
#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
        /* MPLS/MiM virtual port unicast */
#if defined(BCM_SCORPION_SUPPORT)
        if (SOC_IS_SC_CQ(unit) &&
            soc_feature(unit, soc_feature_subport)){
            int grp;
            if (BCM_SUCCESS(_bcm_tr_subport_group_find(unit,
                                                       vp, &grp))) {
                if (grp != -1) {  	
                    BCM_GPORT_SUBPORT_PORT_SET(l_gport, vp);
                }
            }
        } else
#endif /* BCM_SCORPION_SUPPORT */
        if (SOC_IS_TR_VL(unit)) {
            if (_bcm_vp_used_get(unit, vp, _bcmVpTypeMpls)) {
                BCM_GPORT_MPLS_PORT_ID_SET(l_gport, vp);
            } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeMim)) {
                BCM_GPORT_MIM_PORT_ID_SET(l_gport, vp);
            } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeSubport)) {
                BCM_GPORT_SUBPORT_PORT_SET(l_gport, vp);
            } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeWlan)) {
                BCM_GPORT_WLAN_PORT_ID_SET(l_gport, vp);
            }
        }
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */
    } else {
        BCM_GPORT_MODPORT_SET(l_gport, (vp >> 8) & 0xff, vp & 0xff);
    }

    *gport = l_gport;
}

#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
STATIC INLINE void
rx_higig2_vpn_resolve(int unit, bcm_vlan_t vni, bcm_vlan_t *vpn)
{
    if (SOC_IS_TR_VL(unit)) {
        if (_bcm_vfi_used_get(unit, vni, _bcmVfiTypeMpls)) {
            _BCM_VPN_SET(*vpn, _BCM_VPN_TYPE_VFI, vni);
        } else if (_bcm_vfi_used_get(unit, vni, _bcmVfiTypeMim)) {
            _BCM_VPN_SET(*vpn, _BCM_VPN_TYPE_VFI, vni);
        } else if (_bcm_vfi_used_get(unit, vni, _bcmVfiTypeVxlan)) {
            _BCM_VPN_SET(*vpn, _BCM_VPN_TYPE_VFI, vni);
        }
    }
}
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */

#if defined(INCLUDE_L3) && defined(BCM_XGS3_SUPPORT)
STATIC INLINE void
rx_higig2_encap_resolve(int unit, uint32 repl_id, bcm_if_t *encap)
{
    
    if (soc_feature(unit, soc_feature_remote_encap)) {
        /* TR3 only has next hop encoding */
        *encap = repl_id + BCM_XGS3_EGRESS_IDX_MIN;
    } else if (SOC_IS_TRIUMPH2(unit) || SOC_IS_VALKYRIE2(unit) ||
               SOC_IS_APOLLO(unit) || SOC_IS_TD_TT(unit) ||
               SOC_IS_KATANA(unit)) {
        if (0 != (repl_id & (1 << 14))) { 
            *encap = (repl_id & ((1 << 14) - 1)) + BCM_XGS3_EGRESS_IDX_MIN;
        } else {
            /* Just an L3 interface */
            *encap = repl_id;
        }
    } else {
        /* Legacy devices only have L3 interfaces */
        *encap = repl_id;
    }
}
#endif /* INCLUDE_L3 && BCM_XGS3_SUPPORT */
#endif /* BCM_HIGIG2_SUPPORT */


/*
 * Function:
 *      rx_higig_info_decode
 * Purpose:
 *      Decomposes the info in Higig headers and puts them into the
 *      bmc_pkt_t informational members.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      pkt - pointer to completed DCB.
 * Returns:
 *      Nothing.
 */
void
rx_higig_info_decode(int unit, bcm_pkt_t *pkt)
{
    soc_higig_hdr_t *xgh;
    xgh = (soc_higig_hdr_t *)(pkt->_higig);

#ifdef BCM_HIGIG2_SUPPORT
    if (soc_feature(unit, soc_feature_higig2)) {
        /* CMIC RX packets are in HiGig2 format if the device supports it */
        soc_higig2_hdr_t *xgh2;
        bcm_gport_t raw_port, gport;
        int physical = FALSE;
        int remote_encap = FALSE;
#if defined(BCM_BRADLEY_SUPPORT)
        int  mc_index;
        int bcast_size, mcast_size, ipmc_size;
#endif /* BCM_BRADLEY_SUPPORT */
#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
        bcm_vlan_t vni = 0, vpn=0;
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */
#if defined(INCLUDE_L3) && defined(BCM_XGS3_SUPPORT)
        uint32 repl_id;
#endif /* INCLUDE_L3 && BCM_XGS3_SUPPORT */

        if (SOC_REMOTE_ENCAP(unit) && 
            soc_feature(unit, soc_feature_remote_encap)) {
            remote_encap = TRUE;
        }

        xgh2 = (soc_higig2_hdr_t *)(pkt->_higig);

        /* Start with the FRC fields */
        pkt->prio_int = soc_higig2_field_get(unit, xgh2, HG_tc);
        pkt->multicast_group = soc_higig2_field_get(unit, xgh2, HG_mgid);
        pkt->stk_load_balancing_number =
            soc_higig2_field_get(unit, xgh2, HG_lbid);
        switch (soc_higig2_field_get(unit, xgh2, HG_dp)) {
        case 0:
            pkt->color = bcmColorGreen;
            break;
        case 3:
            pkt->color = bcmColorYellow;
            break;
        case 1:
            pkt->color = bcmColorRed;
            break;
        default:
            pkt->color = bcmColorBlack;
            break;
        }
        /* Other FRC fields come from DCB values, so don't overwrite them */

        switch (soc_higig2_field_get(unit, xgh2, HG_ppd_type)) {
        case 0:      /* PPD0 */
            if (soc_higig2_field_get(unit, xgh2, HG_ingress_tagged)) {
                pkt->rx_untagged &= ~BCM_PKT_OUTER_UNTAGGED;
            } else {
                pkt->rx_untagged |= BCM_PKT_OUTER_UNTAGGED;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_mirror)) {
                pkt->stk_flags |= BCM_PKT_STK_F_MIRROR;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_l3)) {
                pkt->flags |= BCM_PKT_F_ROUTED;
            }
            pkt->vlan = soc_higig2_field_get(unit, xgh2, HG_vlan_id);
            pkt->vlan_pri = soc_higig2_field_get(unit, xgh2, HG_vlan_pri);
            pkt->vlan_cfi = soc_higig2_field_get(unit, xgh2, HG_vlan_cfi);
            pkt->pfm = soc_higig2_field_get(unit, xgh2, HG_pfm);
            if (soc_higig2_field_get(unit, xgh2, HG_preserve_dscp)) {
                pkt->stk_flags |= BCM_PKT_STK_F_PRESERVE_DSCP;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_preserve_dot1p)) {
                pkt->stk_flags |= BCM_PKT_STK_F_PRESERVE_PKT_PRIO;
            }
#if defined(INCLUDE_L3) && defined(BCM_XGS3_SUPPORT)
            if (remote_encap) {
                pkt->stk_flags |= BCM_PKT_STK_F_ENCAP_ID;
                repl_id =
                    soc_higig2_field_get(unit, xgh2, HG_replication_id);
                rx_higig2_encap_resolve(unit, repl_id, pkt->stk_encap_id);
            }
#endif /* INCLUDE_L3 && BCM_XGS3_SUPPORT */
            break;
        case 1:      /* PPD1 */
            pkt->stk_classification_tag =
                soc_higig2_field_get(unit, xgh2, HG_ctag);
            pkt->stk_flags |= BCM_PKT_STK_F_CLASSIFICATION_TAG;
            pkt->vlan = soc_higig2_field_get(unit, xgh2, HG_vlan_id);
            pkt->vlan_pri = soc_higig2_field_get(unit, xgh2, HG_vlan_pri);
            pkt->vlan_cfi = soc_higig2_field_get(unit, xgh2, HG_vlan_cfi);
            pkt->pfm = soc_higig2_field_get(unit, xgh2, HG_pfm);
            break;
        case 2:      /* PPD2 */
#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
            vni = soc_higig2_field_get(unit, xgh2, HG_vni);
            rx_higig2_vpn_resolve(unit, vni, &vpn);
            if (_BCM_VPN_IS_SET(vpn)) {
                pkt->vlan = vpn;
            }
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */
#if defined(INCLUDE_L3) && defined(BCM_XGS3_SUPPORT)
            if (remote_encap) {
                pkt->stk_flags |= BCM_PKT_STK_F_ENCAP_ID;
                repl_id =
                    soc_higig2_field_get(unit, xgh2, HG_replication_id);
                rx_higig2_encap_resolve(unit, repl_id, pkt->stk_encap_id);
            }
#endif /* INCLUDE_L3 && BCM_XGS3_SUPPORT */
            if (soc_higig2_field_get(unit, xgh2, HG_multipoint)) {
                switch (soc_higig2_field_get(unit, xgh2, HG_fwd_type)) {
                case 4:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_L2_MULTICAST;
                    break;
                case 5:
                    pkt->stk_forward =
                        BCM_PKT_STK_FORWARD_L2_MULTICAST_UNKNOWN;
                    break;
                case 2:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_L3_MULTICAST;
                    break;
                case 3:
                    pkt->stk_forward =
                        BCM_PKT_STK_FORWARD_L3_MULTICAST_UNKNOWN;
                    break;
                case 6:
                    pkt->stk_forward =
                        BCM_PKT_STK_FORWARD_L2_UNICAST_UNKNOWN;
                    break;
                case 7:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_BROADCAST;
                    break;
                default:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_COUNT;
                    break;
                }
                if (!remote_encap) {
                    pkt->multicast_group =
                        soc_higig2_field_get(unit, xgh2, HG_dst_vp);
                }
            } else {
                switch (soc_higig2_field_get(unit, xgh2, HG_fwd_type)) {
                case 0:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_CPU;
                    break;
                case 4:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_L2_UNICAST;
                    break;
                case 2:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_L3_UNICAST;
                    break;
                default:
                    pkt->stk_forward = BCM_PKT_STK_FORWARD_COUNT;
                    break;
                }
                if (!remote_encap) {
                    raw_port = soc_higig2_field_get(unit, xgh2, HG_dst_vp);
                    physical =
                        soc_higig2_field_get(unit, xgh2, HG_dst_type) != 0;
                    rx_higig2_gport_resolve(unit, raw_port,
                                            physical, &gport);
                    if (gport != BCM_GPORT_INVALID) {
                        pkt->dst_gport = gport;
                        pkt->stk_flags |= BCM_PKT_STK_F_DST_PORT;
                    }
                }
            }
            raw_port = soc_higig2_field_get(unit, xgh2, HG_src_vp);
            physical = soc_higig2_field_get(unit, xgh2, HG_src_type) != 0;
            rx_higig2_gport_resolve(unit, raw_port, physical, &gport);
            if (gport != BCM_GPORT_INVALID) {
                pkt->src_gport = gport;
                pkt->stk_flags |= BCM_PKT_STK_F_SRC_PORT;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_mirror)) {
                pkt->stk_flags |= BCM_PKT_STK_F_MIRROR;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_donot_modify)) {
                pkt->stk_flags |= BCM_PKT_STK_F_DO_NOT_MODIFY;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_donot_learn)) {
                pkt->stk_flags |= BCM_PKT_STK_F_DO_NOT_LEARN;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_lag_failover)) {
                pkt->stk_flags |= BCM_PKT_STK_F_TRUNK_FAILOVER;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_protection_status)) {
                pkt->stk_flags |= BCM_PKT_STK_F_FAILOVER;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_preserve_dscp)) {
                pkt->stk_flags |= BCM_PKT_STK_F_PRESERVE_DSCP;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_preserve_dot1p)) {
                pkt->stk_flags |= BCM_PKT_STK_F_PRESERVE_PKT_PRIO;
            }
            break;
        case 3:      /* PPD3 */
            raw_port = soc_higig2_field_get(unit, xgh2, HG_src_vp);
            physical = soc_higig2_field_get(unit, xgh2, HG_src_type) != 0;
            rx_higig2_gport_resolve(unit, raw_port, physical, &gport);
            if (gport != BCM_GPORT_INVALID) {
                pkt->src_gport = gport;
                pkt->stk_flags |= BCM_PKT_STK_F_SRC_PORT;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_preserve_dscp)) {
                pkt->stk_flags |= BCM_PKT_STK_F_PRESERVE_DSCP;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_preserve_dot1p)) {
                pkt->stk_flags |= BCM_PKT_STK_F_PRESERVE_PKT_PRIO;
            }
            if (soc_higig2_field_get(unit, xgh2, HG_donot_learn)) {
                pkt->stk_flags |= BCM_PKT_STK_F_DO_NOT_LEARN;
            }
            if (soc_higig2_field_get(unit, xgh2,
                                    HG_data_container_type) == 1) {
                /* Offload overlay */
                pkt->stk_classification_tag =
                    soc_higig2_field_get(unit, xgh2, HG_ctag);
                pkt->stk_flags |= BCM_PKT_STK_F_CLASSIFICATION_TAG;
                if (soc_higig2_field_get(unit, xgh2, HG_deferred_drop)) {
                    pkt->stk_flags |= BCM_PKT_STK_F_DEFERRED_DROP;
                }
                if (soc_higig2_field_get(unit, xgh2,
                                        HG_deferred_change_pkt_pri)) {
                    pkt->stk_flags |= BCM_PKT_STK_F_DEFERRED_CHANGE_PKT_PRIO;
                    pkt->stk_pkt_prio =
                        soc_higig2_field_get(unit, xgh2, HG_new_pkt_pri);
                }
                if (soc_higig2_field_get(unit, xgh2,
                                        HG_deferred_change_dscp)) {
                    pkt->stk_flags |= BCM_PKT_STK_F_DEFERRED_CHANGE_DSCP;
                    pkt->stk_dscp =
                        soc_higig2_field_get(unit, xgh2, HG_new_dscp);
                }
                switch (soc_higig2_field_get(unit, xgh2, HG_vxlt_done)) {
                case 0:
                    pkt->stk_flags |= BCM_PKT_STK_F_VLAN_TRANSLATE_NONE;
                    break;
                case 1:
                    pkt->stk_flags |= BCM_PKT_STK_F_VLAN_TRANSLATE_UNCHANGED;
                    break;
                case 3:
                    pkt->stk_flags |= BCM_PKT_STK_F_VLAN_TRANSLATE_CHANGED;
                    break;
                default:
                    /* Unknown type, no change */
                    break;
                }
            } /* Else unknown overlay, nothing to parse */
            break;
        default:
            /* Unknown type, nothing to parse */
            break;
        }

#if defined(BCM_BRADLEY_SUPPORT)
        if ((SOC_HIGIG_OP_IPMC == pkt->opcode) ||
            (SOC_HIGIG_OP_MC == pkt->opcode)) {
            /* Resolve multicast group */
            /* We can ignore the following return code because it connot
             * return anything but SOC_E_NONE */
            (void) soc_hbx_higig2_mcast_sizes_get(unit, &bcast_size,
                                                  &mcast_size, &ipmc_size);

            mc_index = pkt->multicast_group;
            if (SOC_HIGIG_OP_IPMC == pkt->opcode) {
                mc_index -= mcast_size;
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
                if (SOC_IS_TRX(unit)) {
                    bcm_multicast_t group;
                    int rv;
                    rv = _bcm_tr_multicast_ipmc_group_type_get(unit,
                                                               mc_index,
                                                               &group);
                    if (BCM_E_NONE == rv) {
                        pkt->multicast_group = group;
                    } else {
                        /* Default to L3 IPMC type */
                        _BCM_MULTICAST_GROUP_SET(pkt->multicast_group,
                                                 _BCM_MULTICAST_TYPE_L3,
                                                 mc_index);
                    }
                } else
#endif /* BCM_TRIUMPH_SUPPORT && INCLUDE_L3 */
                {
                    _BCM_MULTICAST_GROUP_SET(pkt->multicast_group,
                                             _BCM_MULTICAST_TYPE_L3,
                                             mc_index);
                }
            } else { /* SOC_HIGIG_OP_MC */
                mc_index -= bcast_size;
                _BCM_MULTICAST_GROUP_SET(pkt->multicast_group,
                                         _BCM_MULTICAST_TYPE_L2, mc_index);
            }
        }
#endif /* BCM_BRADLEY_SUPPORT */
    } else
#endif /* BCM_HIGIG2_SUPPORT */
    {
        /* Higig decoding */
        pkt->prio_int = soc_higig_field_get(unit, xgh, HG_cos);
        pkt->pfm = soc_higig_field_get(unit, xgh, HG_pfm);
        switch (soc_higig_field_get(unit, xgh, HG_cng)) {
        case 0:
            pkt->color = bcmColorGreen;
            break;
        case 3:
            pkt->color = bcmColorYellow;
            break;
        case 1:
            pkt->color = bcmColorRed;
            break;
        default:
            pkt->color = bcmColorBlack;
            break;
        }
        if (soc_higig_field_get(unit, xgh, HG_hdr_format) == 0) {
            /* Overlay 1 */
            if (SOC_IS_HB_GW(unit)) {
                if (soc_higig_field_get(unit, xgh, HG_donot_modify)) {
                    pkt->stk_flags |= BCM_PKT_STK_F_DO_NOT_MODIFY;
                }
                if (soc_higig_field_get(unit, xgh, HG_donot_learn)) {
                    pkt->stk_flags |= BCM_PKT_STK_F_DO_NOT_LEARN;
                }
                if (soc_higig_field_get(unit, xgh, HG_lag_failover)) {
                    pkt->stk_flags |= BCM_PKT_STK_F_TRUNK_FAILOVER;
                }
            }
            if (soc_higig_field_get(unit, xgh, HG_mirror)) {
                pkt->stk_flags |= BCM_PKT_STK_F_MIRROR;
            }
            if (soc_higig_field_get(unit, xgh, HG_l3)) {
                pkt->flags |= BCM_PKT_F_ROUTED;
            }
        } else if (soc_higig_field_get(unit, xgh, HG_hdr_format) == 1) {
            /* Overlay 2 */
            pkt->stk_classification_tag =
                soc_higig_field_get(unit, xgh, HG_ctag);
            pkt->stk_flags |= BCM_PKT_STK_F_CLASSIFICATION_TAG;
        } /* Else reserved format */

        if ((pkt->rx_untagged) && (pkt->flags & BCM_RX_MIRRORED)) {
            pkt->vlan = soc_higig_field_get(unit, xgh, HG_vlan_id);
            pkt->vlan_pri = soc_higig_field_get(unit, xgh, HG_vlan_pri);
            pkt->vlan_cfi = soc_higig_field_get(unit, xgh, HG_vlan_cfi);
        }
    }
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#if defined(INCLUDE_RCPU) && defined(BCM_ESW_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
extern int rx_rcpu_base_info_parse(bcm_pkt_t *rx_pkt);
extern int rx_rcpu_intr_process_packet(bcm_pkt_t *pkt, int *runit);
#endif 

STATIC void 
rx_default_parser(int unit,  bcm_pkt_t *pkt)
{
    bcm_dma_chan_t  chan;
    int    idx, runit;
    dv_t *dv ;
    dcb_t *dcb ;
    
    dv = pkt->_dv;
    dcb = pkt->_dcb;
    idx = pkt->_idx;
    chan = DV_CHANNEL(dv);

    /* If single buffer used to DMA packet, may need cleanup */    
    if (RX_CHAN_FLAGS(unit, chan) & BCM_RX_F_MULTI_DCB) {
        multi_dcb_fillin(unit, pkt, dv, idx);
    } else if (!SOC_IS_RCPU_UNIT(unit)) {
        single_dcb_fillin(unit, pkt);
    }
        
    runit = unit;

    if (SOC_IS_RCPU_UNIT(unit)) {
#if defined(INCLUDE_RCPU) && defined(BCM_ESW_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
        rx_rcpu_intr_process_packet(pkt, &runit);
#endif
    } else if (SOC_IS_XGS_SWITCH(unit)) { /* Get XGS HiGig info from DMA desc */
#ifdef BCM_ESW_SUPPORT
#ifdef BCM_XGS_SUPPORT
        int src_port_tgid;
        pkt->opcode = SOC_DCB_RX_OPCODE_GET(unit, dcb);
        pkt->dest_mod = SOC_DCB_RX_DESTMOD_GET(unit, dcb);
        pkt->dest_port = SOC_DCB_RX_DESTPORT_GET(unit, dcb);
        pkt->src_mod = SOC_DCB_RX_SRCMOD_GET(unit, dcb);
        src_port_tgid = SOC_DCB_RX_SRCPORT_GET(unit, dcb);
        pkt->rx_decap_tunnel = 
            (bcm_rx_decap_tunnel_t)SOC_DCB_RX_DECAP_TUNNEL_GET(unit, dcb);
        if (!soc_feature(unit, soc_feature_trunk_group_overlay) &&
            (src_port_tgid & BCM_TGID_TRUNK_INDICATOR(unit))) {
            pkt->src_trunk = src_port_tgid & BCM_TGID_PORT_TRUNK_MASK(unit);
            pkt->flags |= BCM_PKT_F_TRUNK;
            pkt->src_port = -1;
        } else {
            pkt->src_port = src_port_tgid;
            pkt->src_trunk = -1;
            pkt->flags &= ~BCM_PKT_F_TRUNK;
#ifdef BCM_XGS3_SWITCH_SUPPORT
            if (soc_feature(unit, soc_feature_trunk_extended)) {
                int rv = BCM_E_NONE;
                bcm_trunk_t tid;

                tid = -1;
                rv = _bcm_esw_trunk_port_property_get(unit, pkt->src_mod,
                                                      src_port_tgid, &tid);
                if (BCM_SUCCESS(rv) && (tid != -1)) {
                    pkt->src_trunk = tid;
                    pkt->flags |= BCM_PKT_F_TRUNK;
                }
            }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
        }
        pkt->src_gport = BCM_GPORT_INVALID;
        pkt->dst_gport = BCM_GPORT_INVALID;
        pkt->cos = SOC_DCB_RX_COS_GET(unit, dcb);

        pkt->prio_int = BCM_PKT_VLAN_PRI(pkt);
        pkt->vlan = BCM_PKT_VLAN_ID(pkt);

#ifdef BCM_XGS3_SWITCH_SUPPORT
        if (SOC_IS_XGS3_SWITCH(unit)) {
            bcm_vlan_t outer_vid;
            int hg_hdr_size;
            int rv = BCM_E_NONE;
            _bcm_gport_dest_t dest;

            if (SOC_DCB_RX_MIRROR_GET(unit, dcb)) {
                pkt->flags |= BCM_RX_MIRRORED;
            } 
            pkt->rx_classification_tag = SOC_DCB_RX_CLASSTAG_GET(unit, dcb);
            outer_vid = SOC_DCB_RX_OUTER_VID_GET(unit, dcb);
            if (outer_vid) {
                /* Don't overwrite if we don't have info */
                pkt->vlan = outer_vid;
            }
            pkt->vlan_pri = SOC_DCB_RX_OUTER_PRI_GET(unit, dcb);
            pkt->vlan_cfi = SOC_DCB_RX_OUTER_CFI_GET(unit, dcb);
            pkt->inner_vlan = SOC_DCB_RX_INNER_VID_GET(unit, dcb);
            pkt->inner_vlan_pri = SOC_DCB_RX_INNER_PRI_GET(unit, dcb);
            pkt->inner_vlan_cfi = SOC_DCB_RX_INNER_CFI_GET(unit, dcb);
#ifdef BCM_TRX_SUPPORT
            pkt->rx_outer_tag_action =
                _BCM_TRX_TAG_ACTION_DECODE(SOC_DCB_RX_OUTER_TAG_ACTION_GET(unit, dcb));
            pkt->rx_inner_tag_action =
                _BCM_TRX_TAG_ACTION_DECODE(SOC_DCB_RX_INNER_TAG_ACTION_GET(unit, dcb));
            pkt->rx_l3_intf = SOC_DCB_RX_L3_INTF_GET(unit, dcb);
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_HIGIG2_SUPPORT
            if (soc_feature(unit, soc_feature_higig2)) {
                hg_hdr_size = sizeof(soc_higig2_hdr_t);
            } else
#endif /* BCM_HIGIG2_SUPPORT */
            {
                hg_hdr_size = sizeof(soc_higig_hdr_t);
            }
            /* Put XGS hdr into pkt->_higig */
            sal_memcpy(pkt->_higig, SOC_DCB_MHP_GET(unit, dcb),
                       hg_hdr_size);
#ifdef  LE_HOST
            {
                /* Higig field accessors expect network byte ordering,
                 * so we must reverse the bytes on LE hosts */
                int word;
                uint32 *hg_data = (uint32 *) pkt->_higig;
                for (word = 0; word < BYTES2WORDS(hg_hdr_size); word++) {
                    hg_data[word] = _shr_swap32(hg_data[word]);
                }
            }
#endif
            rx_higig_info_decode(unit, pkt);

            if (BCM_GPORT_INVALID == pkt->src_gport) {
                /* Not set by the Higig2 logic */
                _bcm_gport_dest_t_init(&dest);
                if (-1 != pkt->src_trunk) {
                    dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
                    dest.tgid = pkt->src_trunk;
                } else {
                    dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                    dest.modid = pkt->src_mod;
                    dest.port = pkt->src_port;
                }
                rv = _bcm_esw_gport_construct(unit, &dest, &(pkt->src_gport));
                if (BCM_FAILURE(rv)) {
                    pkt->src_gport = BCM_GPORT_INVALID;
                }
            }

            if (BCM_GPORT_INVALID == pkt->dst_gport) {
                /* Not set by the Higig2 logic */
                _bcm_gport_dest_t_init(&dest);
                dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                dest.modid = pkt->dest_mod;
                dest.port = pkt->dest_port;
                rv = _bcm_esw_gport_construct(unit, &dest,
                                              &(pkt->dst_gport));
                if (BCM_FAILURE(rv)) {
                    pkt->dst_gport = BCM_GPORT_INVALID;
                }
            }
        }
        pkt->rx_matched = SOC_DCB_RX_MATCHRULE_GET(unit, dcb);
#endif

        if (soc_feature(unit, soc_feature_rx_timestamp)) {
           /* Get time stamp value for TS protocol packets or OAM DM */
            pkt->rx_timestamp = SOC_DCB_RX_TIMESTAMP_GET(unit, dcb); 
        }

        if (soc_feature(unit, soc_feature_rx_timestamp_upper)) {
            /* Get upper 32-bit of time stamp value for OAM DM */
            pkt->rx_timestamp_upper = SOC_DCB_RX_TIMESTAMP_UPPER_GET(unit, dcb);
        }
    } else if (SOC_IS_XGS12_FABRIC(unit)) {
        soc_higig_hdr_t *xgh;

        xgh = (soc_higig_hdr_t *)(pkt->_higig);
        pkt->opcode = xgh->overlay1.opcode;
        pkt->dest_mod = xgh->overlay1.dst_mod |
                        (xgh->hgp_overlay1.dst_mod_5 << 5) |
                        (xgh->hgp_overlay1.dst_mod_6 << 6);
        pkt->dest_port = xgh->overlay1.dst_port;
        pkt->src_mod = xgh->overlay1.src_mod |
                        (xgh->hgp_overlay1.src_mod_5 << 5) |
                        (xgh->hgp_overlay1.src_mod_6 << 6);
        pkt->src_port = xgh->overlay1.src_port;
        pkt->cos = SOC_DCB_RX_COS_GET(unit, dcb);
        pkt->prio_int = xgh->overlay1.cos;
        pkt->rx_untagged = !xgh->hgp_overlay1.ingress_tagged;
        pkt->vlan = xgh->overlay1.vlan_id_lo | (xgh->overlay1.vlan_id_hi << 8);
#endif
#endif /* BCM_ESW_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
    } else if (SOC_UNIT_VALID(unit) && SOC_IS_SIRIUS(unit)) {
        pkt->opcode = SOC_DCB_RX_OPCODE_GET(unit, dcb);
        pkt->dest_mod = SOC_DCB_RX_DESTMOD_GET(unit, dcb);
        pkt->dest_port = SOC_DCB_RX_DESTPORT_GET(unit, dcb);
        pkt->src_mod = SOC_DCB_RX_SRCMOD_GET(unit, dcb);
	pkt->cos = 0;
	pkt->prio_int = BCM_PKT_VLAN_PRI(pkt);
#endif
    } else if (SOC_UNIT_VALID(unit)) {
        /* All Strata and Strata 2 devices report src port as ing port */
        pkt->src_port = SOC_DCB_RX_SRCPORT_GET(unit, dcb);
        pkt->cos = SOC_DCB_RX_COS_GET(unit, dcb);
        pkt->prio_int = BCM_PKT_VLAN_PRI(pkt);
        pkt->vlan = BCM_PKT_VLAN_ID(pkt);
    }
    
    pkt->rx_unit = runit;
    
#ifdef BCM_ESW_SUPPORT
    if ((SOC_UNIT_VALID(unit)) && !SOC_IS_RCPU_UNIT(unit)) {
        pkt->rx_reason = SOC_DCB_RX_REASON_GET(unit, dcb);
        SOC_DCB_RX_REASONS_GET(unit, dcb, &pkt->rx_reasons);
    }
#endif

#ifdef BCM_ARAD_SUPPORT
#ifdef BCM_ARAD_PARSE_PACKET_IN_INTERRUPT_CONTEXT
    if (SOC_IS_ARAD(unit)) {
         _bcm_dpp_rx_packet_parse(unit, pkt, 0);         
    }
#endif /*BCM_ARAD_PARSE_PACKET_IN_INTERRUPT_CONTEXT*/
#endif /*BCM_ARAD_SUPPORT*/

}

/*
 * Function:
 *      rx_intr_process_packet
 * Purpose:
 *      Processes a received packet in interrupt context
 *      calling out to the handlers until
 *      the packet is consumed.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      dcb - pointer to completed DCB.
 * Returns:
 *      Nothing.
 * Notes:
 *      THIS ROUTINE IS CALLED AT INTERRUPT LEVEL.
 */

STATIC INLINE void
rx_intr_process_packet(int unit, dv_t *dv, dcb_t *dcb, bcm_pkt_t *pkt)
{
    volatile rx_callout_t *rco;
    bcm_rx_t              handler_rc;
    bcm_dma_chan_t        chan;
    rx_queue_t           *queue;
    int                   handled = FALSE;
    
    pkt->_dcb = (void *)dcb;
    chan = DV_CHANNEL(dv);
    
    pkt_len_get(unit, chan, (bcm_pkt_t *)pkt, dv);
    pkt->rx_port = SOC_DCB_RX_INGPORT_GET(unit, dcb);
    pkt->dma_channel = chan;
    pkt->rx_unit = pkt->unit = unit;
     
    if (!SOC_IS_XGS12_FABRIC(unit)) {
        /* Need this for some HiGig processing, so do it first */
        pkt->rx_untagged = SOC_DCB_RX_UNTAGGED_GET(unit, dcb);
    }
    
	/* to get correct cos */
    if (SOC_IS_RCPU_UNIT(unit)) {
#if defined(INCLUDE_RCPU) && defined(BCM_ESW_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
        if (!(RX_CHAN_FLAGS(unit, chan) & BCM_RX_F_MULTI_DCB)) {
            single_dcb_fillin(unit, pkt);
        }
        if (BCM_SUCCESS(rx_rcpu_base_info_parse(pkt))) {
            pkt->cos = SOC_DCB_RX_COS_GET(unit, (dcb_t *)(pkt->_dcb));
        }
#endif
#ifdef BCM_ARAD_SUPPORT
#ifdef BCM_ARAD_PARSE_PACKET_IN_INTERRUPT_CONTEXT
    } else if (SOC_IS_ARAD(unit)) {
         _bcm_dpp_rx_packet_cos_parse(unit, pkt);
#endif 
#endif 
    } else {
        pkt->cos = SOC_DCB_RX_COS_GET(unit, dcb);
#ifdef BCM_SIRIUS_SUPPORT    
        if (SOC_IS_SIRIUS(unit)) {
            pkt->cos = 0;
        } 
#endif 
    }

    RX_CHAN_CTL(unit, chan).rpkt++;
    RX_CHAN_CTL(unit, chan).rbyte += pkt->tot_len;
    
    queue = RX_QUEUE(unit, pkt->cos);

   
    if (INTR_CALLOUTS(unit)) {
        /* Check if bandwidth available for this pkt */
        if (queue->pps > 0 && queue->tokens <= 0) {
            /* Rate limiting on and not enough tokens */
            queue->rate_disc++;
            MARK_PKT_PROCESSED_LOCAL(unit, dv);
            return;
        }
        
        if ((rx_ctl[unit]->user_cfg.flags & BCM_RX_F_PKT_UNPARSED) != 
        	BCM_RX_F_PKT_UNPARSED) {
            if (rx_ctl[unit]->rx_parser != NULL) {
                rx_ctl[unit]->rx_parser(unit, pkt);
            }
        }
        
        /* Loop through registered drivers looking for intr handlers. */
        for (rco = rx_ctl[unit]->rc_callout; rco; rco = rco->rco_next) {
            BCM_API_XLATE_PORT_DECL(rx_port);
            BCM_API_XLATE_PORT_DECL(rx_port_orig);

            if (!(rco->rco_flags & BCM_RCO_F_INTR)) {
                continue;
            }
            if (!(SHR_BITGET(rco->rco_cos, pkt->cos))) {
                /* callback is not interested in this COS */
                continue;
            }

            /* Since pkt->rx_port is an int8, it cannot be passed directly */
            BCM_API_XLATE_PORT_SAVE(rx_port_orig, pkt->rx_port);
            BCM_API_XLATE_PORT_ASSIGN(rx_port, rx_port_orig);
            BCM_API_XLATE_PORT_P2A(unit, &rx_port);
            BCM_API_XLATE_PORT_ASSIGN(pkt->rx_port, rx_port);

            handler_rc = rco->rco_function(unit, pkt, rco->rco_cookie);

            BCM_API_XLATE_PORT_RESTORE(pkt->rx_port, rx_port_orig);

            switch (handler_rc) {
            case BCM_RX_NOT_HANDLED:
                break;                      /* Next callout */
            case BCM_RX_HANDLED:            /* Account for the packet */
                _Q_DEC_TOKENS(queue);
                MARK_PKT_PROCESSED_LOCAL(unit, dv);
                handled = TRUE;
                rco->rco_pkts_handled++;
                break;
            case BCM_RX_HANDLED_OWNED:      /* Packet stolen */
                _Q_DEC_TOKENS(queue);
                pkt->_pkt_data.data = NULL;  /* Mark as stolen */
                pkt->alloc_ptr = NULL;  /* Mark as stolen */
                MARK_PKT_PROCESSED_LOCAL(unit, dv);
                handled = TRUE;
                rx_ctl[unit]->pkts_owned++;
                rco->rco_pkts_owned++;
                break;
            default: /* Treated as NOT_HANDLED */
                rx_ctl[unit]->bad_hndlr_rv++;
                break;
            }

            if (handled) {
                break;
            }
        }
    }

    if (!handled) {  /* Not processed, enqueue for non-interrupt handling. */
        if ((queue->max_len > 0) && (queue->count < queue->max_len)) {
            if (NON_INTR_CALLOUTS(unit)) {
                if ((rx_ctl[unit]->user_cfg.flags & BCM_RX_F_PKT_UNPARSED) != 
                	BCM_RX_F_PKT_UNPARSED) {
                    if (rx_ctl[unit]->rx_parser != NULL) {
                        rx_ctl[unit]->rx_parser(unit, pkt);
                    }
                }
                _Q_ENQUEUE(queue, pkt);
                RX_THREAD_NOTIFY(unit);
            } else {
                MARK_PKT_PROCESSED_LOCAL(unit, dv);
                rx_ctl[unit]->no_hndlr++;
            }
        } else { /* No space; discard */
            MARK_PKT_PROCESSED_LOCAL(unit, dv);
            queue->qlen_disc++;
        }
    } else {
	/* 
	 * perserving these flags that were conditionally set
	 * in function bcm_pkt_flags_init(int unit, bcm_pkt_t *pkt, uint32 flags) 
	 */
	pkt->flags &= (BCM_PKT_F_HGHDR | BCM_PKT_F_SLTAG);
    }
}

int
rcpu_rx_pkt_enqueue(int unit, bcm_pkt_t *pkt)
{
    rx_queue_t           *queue;

    if (!RX_UNIT_STARTED(unit) || !rx_control.thread_running) {
        return BCM_E_PARAM;
    }
 
    queue = RX_QUEUE(unit, pkt->cos);

    if ((queue->max_len > 0) && (queue->count < queue->max_len)) {
        _Q_ENQUEUE_LOCK(queue, pkt);
        RX_THREAD_NOTIFY(unit);
    } else { /* No space; discard */
        queue->qlen_disc++;
        return BCM_E_RESOURCE;
    }

    return BCM_E_NONE;
}

#if defined(BCM_RPC_SUPPORT)

int bcm_rx_tunnel_count;

/*
 * Function:
 *      bcm_rx_remote_pkt_enqueue
 * Purpose:
 *      Enqueue a remote packet for normal RX processing
 * Parameters:
 *      unit          - The BCM unit in which queue the pkt is placed
 *      pkt           - The packet to enqueue
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_common_rx_remote_pkt_enqueue(int unit, bcm_pkt_t *pkt)
{
    rx_queue_t           *queue;

    if (!RX_IS_SETUP(unit) || RX_IS_LOCAL(unit) || RX_IS_RCPU(unit)) {
        return BCM_E_PARAM;
    }

    queue = RX_QUEUE(unit, pkt->cos);

    if ((queue->max_len > 0) && (queue->count < queue->max_len)) {
        _Q_ENQUEUE_LOCK(queue, pkt);
        RX_THREAD_NOTIFY(unit);
    } else { /* No space; discard */
        queue->qlen_disc++;
        return BCM_E_RESOURCE;
    }

    bcm_rx_tunnel_count++;

    return BCM_E_NONE;
}
#else
int
_bcm_common_rx_remote_pkt_enqueue(int unit, bcm_pkt_t *pkt)
{
    return BCM_E_UNAVAIL;
}
#endif  /* BCM_RPC_SUPPORT */

#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)

/*
 * Function:
 *      fill_in_pkt_vpn_id 
 * Purpose:
 *      Fill pkt->vlan with vpn id if pkt->src_gport is a VPLS virtual port.
 *      This function is necessary since the rx_higig_info_decode function is
 *      not always able to derive vpn id from Higig2 header. Triumph does not
 *      set PPD2 header's VNI field, and PPD3 header doesn't have a VNI field.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      pkt - The packet to process.
 * Returns:
 *      Nothing.
 * Notes:
 *      This function needs to read SOURCE_VPm table to obtain vpn id.
 *      Hence, it's called in non-interrupt context by rx_process_packet.
 */

STATIC void
fill_in_pkt_vpn_id(int unit, bcm_pkt_t *pkt)
{
    int src_vp;
    source_vp_entry_t svp_entry;
    int rv;
    int vfi;

    if (BCM_GPORT_IS_MPLS_PORT(pkt->src_gport)) {
        src_vp = BCM_GPORT_MPLS_PORT_ID_GET(pkt->src_gport);
        if (!_BCM_VPN_IS_SET(pkt->vlan)) {
            rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ALL, src_vp, &svp_entry);
            if (BCM_SUCCESS(rv)) {
                if (0x1 == 
                    soc_SOURCE_VPm_field32_get(unit, &svp_entry, ENTRY_TYPEf)) {
                    vfi = soc_SOURCE_VPm_field32_get(unit, &svp_entry, VFIf);
                    if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMpls)) {
                        _BCM_VPN_SET(pkt->vlan, _BCM_VPN_TYPE_VFI, vfi);
                    } 
                } 
            }
        }
    }

    return;
}

#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */

#ifdef BCM_RCPU_SUPPORT
/*
 * Function:
 *      rx_rcpu_process_packet
 * Purpose:
 *      Processes a received RCPU packet in non-interrupt context
 *      calling out to the handlers until the packet is consumed.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      rx_pkt - The packet to process
 * Returns:
 *      Nothing.
 * Notes:
 *      Assumes discard handler is registered with lowest priority
 */

STATIC void
rx_rcpu_process_packet(int unit, bcm_pkt_t *pkt)
{
    volatile rx_callout_t *rco;
    bcm_rx_t              handler_rc;
    int                   handled = FALSE;
    int i;

    assert(pkt);

    RX_LOCK(unit);  /* Can't modify handler list while doing callbacks */
    /* Loop through registered drivers until packet consumed */
    for (rco = rx_ctl[unit]->rc_callout; rco; rco = rco->rco_next) {
        if (rco->rco_flags & BCM_RCO_F_INTR) { /* Non interrupt context */
            continue;
        }
        if (!(SHR_BITGET(rco->rco_cos, pkt->cos))) {
            /* callback is not interested in this COS */
            continue;
        }

        handler_rc = rco->rco_function(unit, pkt, rco->rco_cookie);

        switch (handler_rc) {
        case BCM_RX_NOT_HANDLED:
            break;                      /* Next callout */
        case BCM_RX_HANDLED:
            handled = TRUE;
            bcm_rx_remote_pkt_free(pkt); /* Free the packet */
            LOG_VERBOSE(BSL_LS_BCM_RX,
                        (BSL_META_U(unit,
                                    BSL_META_U(unit, "rx: pkt handled by %s\n")),
                         rco->rco_name));
            rco->rco_pkts_handled++;
            break;
        case BCM_RX_HANDLED_OWNED:
            handled = TRUE;
            /*
             * Make the data buffer point to NULL
             *     As a result, in pkt_free, only the pkt structure is freed
             *     The data buffer itself is not.
             */
            for (i = 0; i < pkt->blk_count; i++) {
                pkt->pkt_data[i].data = NULL;
            }
            pkt->alloc_ptr = NULL;
            bcm_rx_remote_pkt_free(pkt);
            LOG_VERBOSE(BSL_LS_BCM_RX,
                        (BSL_META_U(unit,
                                    BSL_META_U(unit, "rx: pkt owned by %s\n")),
                         rco->rco_name));
            rx_ctl[unit]->pkts_owned++;
            rco->rco_pkts_owned++;
            break;
        default:
            LOG_WARN(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U
                                  (unit,
                                  "rx_process_packet: unit=%d: "
                                  "Invalid callback return value=%d\n")),
                      unit, handler_rc));
            break;
        }

        if (handled) {
            break;
        }
    }
    RX_UNLOCK(unit);

    if (!handled) {
        /* Internal error as discard should have handled packet */
        RX_ERROR((BSL_META_U
                  (unit,
                   "bcm_rx_process_packet: No handler processed packet: "
                   "Unit %d Port %s\n"),
                  unit, SOC_PORT_NAME(unit, pkt->rx_port)));
        bcm_rx_remote_pkt_free(pkt);
    }
}
#endif /* BCM_RCPU_SUPPORT */
/*
 * Function:
 *      rx_process_packet
 * Purpose:
 *      Processes a received packet in non-interrupt context
 *      calling out to the handlers until the packet is consumed.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      rx_pkt - The packet to process
 * Returns:
 *      Nothing.
 * Notes:
 *      Assumes discard handler is registered with lowest priority
 */

STATIC void
rx_process_packet(int unit, bcm_pkt_t *pkt)
{
    volatile rx_callout_t *rco;
    bcm_rx_t              handler_rc;
    dv_t                 *dv;
    int                   idx;
    int                   handled = FALSE;

    assert(pkt);

    dv = pkt->_dv;
    idx = pkt->_idx;
    if (RX_IS_LOCAL(unit) || RX_IS_RCPU(unit)) {
        assert(pkt == DV_PKT(dv, idx));
    }

#ifdef BCM_ARAD_SUPPORT
#ifndef BCM_ARAD_PARSE_PACKET_IN_INTERRUPT_CONTEXT
    if (SOC_UNIT_VALID(unit) && SOC_IS_ARAD(unit)) {
         _bcm_dpp_rx_packet_parse(unit, pkt, 1);
	}
#endif /*BCM_ARAD_PARSE_PACKET_IN_INTERRUPT_CONTEXT*/
#endif /*BCM_ARAD_SUPPORT*/

#ifdef  BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourcePacket, bslSeverityNormal, unit)) {
        /* Dump the packet info */
        RX_INFO((BSL_META_U(unit, "rx_process_packet: packet in\n")));
        if ((RX_IS_LOCAL(unit) && SOC_UNIT_VALID(unit))
            || RX_IS_RCPU(unit)) {
            if (bsl_check(bslLayerSoc, bslSourceDma, bslSeverityVerbose, unit)) {
                soc_dma_dump_dv(unit, "rx dv: ", dv);
            }
        }
    }
#endif  /* BROADCOM_DEBUG */

    /*    coverity[overrun-local : FALSE]    */
    if (!NON_INTR_CALLOUTS(unit)) {
        /* No one to talk to.  Mark processed and return */
        MARK_PKT_PROCESSED(pkt, unit, dv);
        rx_ctl[unit]->no_hndlr++;
        return;
    }

#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
    if (SOC_UNIT_VALID(unit) && SOC_IS_TR_VL(unit)) { 
        fill_in_pkt_vpn_id(unit, pkt);
    }
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */

    RX_LOCK(unit);  /* Can't modify handler list while doing callbacks */

#ifdef BCM_SBX_CMIC_SUPPORT
    /*
     * SOC layer for unit 1 is not instantiated when unit 1 is remote. 
     * This has been observed in a debug/test environment only thus far.
     * However, unit 1 does get processed by the RX code.
     * This code has a lot of history and supports running on many "platforms"
     * so the coder should exercise caution in making changes. 
     * Checking SOC_UNIT_VALID(unit) would be prudent in additional code paths.
     */
    if (SOC_UNIT_VALID(unit)) {
		if (SOC_IS_SIRIUS(unit)) {
			if (SOC_SBX_CFG(unit)->uInterfaceProtocol == SOC_SBX_IF_PROTOCOL_SBX) {
			    if (SOC_SBX_CFG(unit)->parse_rx_erh) {
				sbx_rx_parse_pkt(unit, pkt);
			    }
			}
		} else if (SOC_IS_CALADAN3(pkt->unit)) {
		  /* CMIC interface on C3 is treated as a normal port, and therefore there is ERH */
		  pkt->_sbx_hdr_len = 0;
		}
        /* Note the use of different "unit" designations below in this conditional!
         * For "L2 learn" messages coming up from Sirius system the unit == 1 
         * and pkt->unit == 0! This is all rather overly complex.
         */ 

	} else if (RX_IS_REMOTE(unit) && SOC_IS_CALADAN3(pkt->unit)) {
      if (SOC_SBX_CONTROL(unit)->ucode_erh == SOC_SBX_G3P1_ERH_ARAD) {
	pkt->_sbx_hdr_len = SOC_SBX_G3P1_ERH_LEN_ARAD;
	sal_memcpy(pkt->_sbx_rh, pkt->pkt_data->data, pkt->_sbx_hdr_len);
	sbx_rx_parse_pkt_ext(unit, pkt, SOC_SBX_G3P1_ERH_ARAD, unit);
      } else {
        pkt->_sbx_hdr_len = SOC_SBX_G2P3_ERH_LEN_SIRIUS;
        pkt->pkt_len -= SOC_SBX_G2P3_ERH_LEN_SIRIUS;
        sal_memcpy(pkt->_sbx_rh, pkt->pkt_data->data, pkt->_sbx_hdr_len);
        sbx_rx_parse_pkt_ext(unit, pkt, SOC_SBX_G2P3_ERH_SIRIUS, pkt->unit);
      }
    }
#endif

    /* Loop through registered drivers until packet consumed */
    for (rco = rx_ctl[unit]->rc_callout; rco; rco = rco->rco_next) {
        BCM_API_XLATE_PORT_DECL(rx_port);
        BCM_API_XLATE_PORT_DECL(rx_port_orig);

        if (rco->rco_flags & BCM_RCO_F_INTR) { /* Non interrupt context */
            continue;
        }
        if (!(SHR_BITGET(rco->rco_cos, pkt->cos))) {
            /* callback is not interested in this COS */
            continue;
        }

        /* Since pkt->rx_port is an int8, it cannot be passed directly */
        BCM_API_XLATE_PORT_SAVE(rx_port_orig, pkt->rx_port);
        BCM_API_XLATE_PORT_SAVE(rx_port, rx_port_orig);
        BCM_API_XLATE_PORT_P2A(unit, &rx_port);
        BCM_API_XLATE_PORT_SAVE(pkt->rx_port, rx_port);

        handler_rc = rco->rco_function(unit, pkt, rco->rco_cookie);

        BCM_API_XLATE_PORT_RESTORE(pkt->rx_port, rx_port_orig);

        switch (handler_rc) {
        case BCM_RX_NOT_HANDLED:
            break;                      /* Next callout */
        case BCM_RX_HANDLED:
            handled = TRUE;
            MARK_PKT_PROCESSED(pkt, unit, dv);
            LOG_VERBOSE(BSL_LS_BCM_RX,
                        (BSL_META_U(unit,
                                    BSL_META_U(unit, "rx: pkt handled by %s\n")),
                         rco->rco_name));
            rco->rco_pkts_handled++;
            break;
        case BCM_RX_HANDLED_OWNED:
            handled = TRUE;
            pkt->_pkt_data.data = NULL;
            pkt->alloc_ptr = NULL;
            MARK_PKT_PROCESSED(pkt, unit, dv);
            LOG_VERBOSE(BSL_LS_BCM_RX,
                        (BSL_META_U(unit,
                                    BSL_META_U(unit, "rx: pkt owned by %s\n")),
                         rco->rco_name));
            rx_ctl[unit]->pkts_owned++;
            rco->rco_pkts_owned++;
            break;
        default:
            LOG_WARN(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U
                                  (unit,
                                  "rx_process_packet: unit=%d: "
                                  "Invalid callback return value=%d\n")),
                      unit, handler_rc));
            break;
        }

        if (handled) {
            break;
        }
    }
    /* 
     * perserving these flags that were conditionally set
     * in function bcm_pkt_flags_init(int unit, bcm_pkt_t *pkt, uint32 flags) 
     */
    pkt->flags &= (BCM_PKT_F_HGHDR | BCM_PKT_F_SLTAG);
    RX_UNLOCK(unit);

    if (!handled) {
        /* Internal error as discard should have handled packet */
        if (SOC_UNIT_VALID(unit)) {
            RX_ERROR((BSL_META_U
                      (unit,
                       "bcm_rx_process_packet: No handler processed packet: "
                       "Unit %d Port %s\n"),
                      unit, SOC_PORT_NAME(unit, pkt->rx_port)));
        } else {
            RX_ERROR((BSL_META_U
                      (unit,
                       "bcm_rx_process_packet: No handler processed packet: "
                       "Unit %d Port %d\n"),
                      unit, pkt->rx_port));
        }
        MARK_PKT_PROCESSED(pkt, unit, dv);
    }
}

/****************************************************************/


/****************************************************************
 *
 * Setup routines
 *
 ****************************************************************/

/*
 * Function:
 *      rx_channel_dv_setup
 * Purpose:
 *      Set up DVs and packets for a channel according to current config
 * Parameters:
 *      unit, chan - what's being configured
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      This is only called during the init (bcm_rx_start)
 *      Local units only.
 */

STATIC int
rx_channel_dv_setup(int unit, int chan)
{
    int i, j;
    dv_t *dv;
    int rv;
    bcm_pkt_t *all_pkts;
    bcm_pkt_t *pkt;
    uint32 dma_flags = 0;

    /* If channel is not configured for RX, configure it. */
    switch (soc_dma_chan_dvt_get(unit, chan)) {
    case DV_RX: /* Already RX - check flags */
        if (RX_CHAN_FLAGS(unit, chan) & BCM_RX_F_OVERSIZED_OK) {
            /* Force reconfiguration */
            dma_flags = SOC_DMA_F_MBM | SOC_DMA_F_INTR_ON_DESC;
            LOG_VERBOSE(BSL_LS_BCM_RX,
                        (BSL_META_U(unit,
                                    BSL_META_U(unit, "rx: accept oversized pkts\n"))));
        }
        break;
    case DV_NONE: /* Need to configure the channel */
        dma_flags = SOC_DMA_F_MBM;
        break;
    default: /* Problem */
        RX_ERROR((BSL_META_U(unit, "Incompatible channel setup for %d/%d\n"),
                  unit, chan));
        return BCM_E_PARAM;
    }

    /* Check if we need to (re)configure Rx DMA channel */
    if (dma_flags) {
        rv = soc_dma_chan_config(unit, chan, DV_RX, dma_flags);
        if (rv < 0) {
            RX_ERROR((BSL_META_U
                      (unit, "Could not configure channel %d on %d\n"),
                      chan, unit));
            return rv;
        }
    }

    /* Allocate the packet structures we need for the DVs */
    if (RX_CHAN_PKTS(unit, chan) == NULL) {
        all_pkts = sal_alloc(sizeof(bcm_pkt_t) *
                             RX_CHAN_PKT_COUNT(unit, chan), "rx_pkts");
        if (all_pkts == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(all_pkts, 0, sizeof(bcm_pkt_t) * RX_PPC(unit) *
                   RX_CHAINS(unit, chan));
        RX_CHAN_PKTS(unit, chan) = all_pkts;
    }

    /* Set up packets and allocate DVs */
    for (i = 0; i < RX_CHAINS(unit, chan); i++) {
        if ((dv = rx_dv_alloc(unit, chan, i)) == NULL) {
            for (j = 0; j < i; j++) {
                rx_dv_dealloc(unit, chan, j);
                RX_DV(unit, chan, i) = NULL;
            }
            sal_free(RX_CHAN_PKTS(unit, chan));
            RX_CHAN_PKTS(unit, chan) = NULL;
            return BCM_E_MEMORY;
        }
        for (j = 0; j < RX_PPC(unit); j++) {
            pkt = RX_PKT(unit, chan, i, j);
            pkt->_idx = j;
            pkt->_dv = dv;
            pkt->unit = unit;
            pkt->pkt_data = &pkt->_pkt_data;
            pkt->blk_count = 1;
            if ((SOC_UNIT_VALID(unit) && SOC_IS_XGS12_FABRIC(unit))
                    && !RX_IGNORE_HG(unit)) {
                BCM_PKT_HGHDR_REQUIRE(pkt);
            }
            if ((SOC_UNIT_VALID(unit) && SOC_SL_MODE(unit))
                && !RX_IGNORE_SL(unit)) {
                BCM_PKT_SLTAG_REQUIRE(pkt);
            }
        }

        DV_STATE(dv) = DV_S_NEEDS_FILL;
        RX_DV(unit, chan, i) = dv;
    }

    return BCM_E_NONE;
}


STATIC int
rx_discard_handler_setup(int unit, rx_ctl_t *rx_ctrl_ptr)
{
    volatile rx_callout_t *rco;
    int i;

    if ((rco = sal_alloc(sizeof(*rco), "rx_callout")) == NULL) {
        return BCM_E_MEMORY;
    }
    SETUP_RCO(rco, "Discard", rx_discard_packet, BCM_RX_PRIO_MIN, NULL, NULL,
              BCM_RCO_F_ALL_COS);   /* Accept any cos; non-interrupt */
    for (i = 0; i <= rx_ctrl_ptr->queue_max; i++) {
        SETUP_RCO_COS_SET(rco, i);
    }
    rx_ctrl_ptr->rc_callout = rco;

    return BCM_E_NONE;
}

/*
 * Check the channel configuration passed in by the user for legal values
 */

STATIC void
rx_user_cfg_check(int unit)
{
    int chan;
    uint32 cos_bmp = 0;
    int chan_count = 0;
    bcm_rx_cfg_t *cfg;
    int cos;
    rx_queue_t *queue;

    cfg = &rx_ctl[unit]->user_cfg;

    if (RX_IS_LOCAL(unit) || RX_IS_RCPU(unit)) {
        FOREACH_SETUP_CHANNEL(unit, chan) {
            if (RX_CHAINS(unit, chan) < 0) {
                LOG_WARN(BSL_LS_BCM_RX,
                         (BSL_META_U(unit,
                                     BSL_META_U
                                      (unit, "rx_config %d %d: Warning: chains < 0.")),
                          unit, chan));
                RX_CHAINS(unit, chan) = 0;
            } else {
                chan_count++;
                if (RX_CHAINS(unit, chan) > RX_CHAINS_MAX) {
                    LOG_WARN(BSL_LS_BCM_RX,
                             (BSL_META_U(unit,
                                         BSL_META_U
                                          (unit, "rx_config %d %d: Warning: "
                                          "Bad chain cnt %d.  Now %d.\n")),
                              unit, chan,
                              RX_CHAINS(unit, chan), RX_CHAINS_MAX));
                    RX_CHAINS(unit, chan) = RX_CHAINS_MAX;
                }
            }
        }
        if (cfg->pkts_per_chain <= 0 ||
            cfg->pkts_per_chain > RX_PPC_MAX) {
            LOG_WARN(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U
                                  (unit, "rx_config: Warning: bad pkts/chn %d. Now %d.\n")),
                      cfg->pkts_per_chain, RX_PPC_DFLT));
            cfg->pkts_per_chain = RX_PPC_DFLT;
        }

        if (SOC_UNIT_VALID(unit) &&
            (SOC_IS_XGS(unit) || SOC_IS_SIRIUS(unit))) {
            /* Assumes all XGS devices support 8 COS */
            /* Check if all COS are accounted for and no overlap */
            FOREACH_SETUP_CHANNEL(unit, chan) {
                if (cos_bmp & RX_CHAN_COS(unit, chan)) {
                    LOG_WARN(BSL_LS_BCM_RX,
                             (BSL_META_U(unit,
                                         BSL_META_U
                                          (unit, "rx_config: Warning: COS overlap may not "
                                          "function correctly, unit %d, channel %d\n")),
                              unit, chan));
                }
                cos_bmp |= RX_CHAN_COS(unit, chan);
            }
            if ((cos_bmp = (0xff & ~cos_bmp))) {
                LOG_WARN(BSL_LS_BCM_RX,
                         (BSL_META_U(unit,
                                     BSL_META_U
                                      (unit, "rx_config: Warning: Not mapping "
                                      "COS 0x%x for unit %d\n")), cos_bmp, unit));
            }
        } else {
            if (chan_count > 1) {
                LOG_WARN(BSL_LS_BCM_RX,
                         (BSL_META_U(unit,
                                     BSL_META_U
                                      (unit, "rx_config: Warning: Activating more "
                                      "than one channel on non-xgs system\n"))));
            }
        }
    }
    /*    coverity[overrun-local : FALSE]    */

    /* coverity[overrun-local] */
    if (RX_PPS(unit) < 0) {
        RX_PPS(unit) = 0;
    }

    for (cos = 0; cos <= RX_QUEUE_MAX(unit); cos++) {
        queue = RX_QUEUE(unit, cos);
        if (queue->pps < 0) {
            queue->pps = 0;
        }
        if (queue->max_len < 0) {
            queue->max_len = 0;
        }
    }

}

/****************************************************************
 *
 * Tear down routines
 *
 ****************************************************************/

/*
 * Function:
 *      rx_channel_shutdown
 * Purpose:
 *      Shutdown a channel, deallocating DVs and packets
 * Parameters:
 *      unit, chan - what's being configured
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      This is only called when thread is brought down (bcm_rx_stop)
 *      or on an error
 *
 *      Must only be called on local units.
 */

STATIC void
rx_channel_shutdown(int unit, int chan)
{
    int i;
    bcm_pkt_t *pkt;

    _rx_chan_run_count--;
    rx_ctl[unit]->chan_running &= ~(1 << chan);
    if (RX_CHAN_USED(unit, chan)) {
        /* Deallocate all packets */
        if (RX_CHAN_PKTS(unit, chan) != NULL) {
            /* Free any buffers held by packets */
            for (i = 0; i < RX_CHAN_PKT_COUNT(unit, chan); i++) {
                pkt = &RX_CHAN_PKTS(unit, chan)[i];
                if (pkt->alloc_ptr != NULL) {
                    bcm_rx_free(unit, pkt->alloc_ptr);
                    pkt->_pkt_data.data = NULL;
                    pkt->alloc_ptr = NULL;
                }
            }
            sal_free(RX_CHAN_PKTS(unit, chan));
            RX_CHAN_PKTS(unit, chan) = NULL;
        }
        if (SOC_UNIT_VALID(unit)) {
            /* Deallocate the DVs */
            for (i = 0; i < RX_CHAINS(unit, chan); i++) {
                rx_dv_dealloc(unit, chan, i);
            }
        }
    }
}

/****************************************************************
 *
 * Info/calculation routines
 *
 ****************************************************************/

/*
 * Calculate number of DCBs per packet based on system cfg.
 * The MACs are always set up separately.  The VLAN is skipped
 * on Hercules.
 *
 * The following always get a DCB:
 *    HiGig header
 *    SL tag
 *    Everything past the SL tag
 *
 * If the NO_VTAG flag is given, then an extra DCB is needed for that.
 * In that case, we also need a DCB for the MAC addresses.
 *
 */

STATIC void
rx_dcb_per_pkt_init(int unit)
{
    int chan;
    int dcb;

    dcb = 3;
    if (SOC_UNIT_VALID(unit) && SOC_SL_MODE(unit)) {
        dcb += 1;       /* sl mode stack tag */
    }

    for (chan = 0; chan < BCM_RX_CHANNELS; chan++) {
        if (RX_CHAN_FLAGS(unit, chan) & BCM_RX_F_MULTI_DCB) {
            RX_CHAN_CTL(unit, chan).dcb_per_pkt = dcb;
        } else {
            RX_CHAN_CTL(unit, chan).dcb_per_pkt = 1;
        }
    }
}


/****************************************************************
 *
 * DV manipulation functions
 *     rx_dv_add_pkt     Add a packet to a DV
 *     rx_dv_fill        Prepare a DV to be sent to the SOC layer
 *     rx_dv_alloc       Allocate an RX DV
 *     rx_dv_dealloc     Free an RX DV
 *
 ****************************************************************/

/* Set up a DV for multiple DCBs (pkt scatter) according to packet format */
STATIC INLINE int
rx_multi_dv_pkt(int unit, volatile bcm_pkt_t *pkt, dv_t *dv, int idx)
{
    int bytes_used;
    uint8 *data_ptr;

#ifdef  BCM_XGS_SUPPORT
    /* HIGIG HEADER: Always into DV struct if present */
    if (BCM_PKT_HAS_HGHDR(pkt)) {
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDRX(unit, dv,
                                          (sal_vaddr_t)SOC_DV_HG_HDR(dv, idx),
                                          sizeof(soc_higig_hdr_t), 0));
    }
#endif

    /* Set up DCB for DMAC and SMAC:  Always into packet */
    SOC_IF_ERROR_RETURN(SOC_DCB_ADDRX(unit, dv,
                                      (sal_vaddr_t)BCM_PKT_DMAC(pkt),
                                      2 * sizeof(bcm_mac_t), 0));
    bytes_used = 2 * sizeof(bcm_mac_t);

    /* Setup DCB for VLAN:  Either into DV struct, or pkt as per flags */
    if (!SOC_IS_XGS12_FABRIC(unit)) {
        /* Assume VLAN tag goes into packet. */
        data_ptr = &pkt->_pkt_data.data[12];
        if (BCM_PKT_RX_VLAN_TAG_STRIP(pkt)) {
            /* Oops, VLAN tag must be placed in non-pkt buffer */
            data_ptr = SOC_DV_VLAN_TAG(dv, idx);
        } else {
            /* Okay, VLAN in pkt.  Adjust byte count for pkt buffer */
            bytes_used += sizeof(uint32); /* Used for VLAN in pkt */
        }
        /* data_ptr now points to where VLAN tag should be DMA'd */
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDRX(unit, dv,
                                          (sal_vaddr_t)data_ptr,
                                          sizeof(uint32), 0));
    } else { /* Adjust bytes used to add space for VLAN tag if not stripped */
        if (!BCM_PKT_RX_VLAN_TAG_STRIP(pkt)) { /* Save space for VLAN tag */
            bytes_used += sizeof(uint32);
        }
    }

    /* Setup DCB for SL TAG when present:  Always put into DV struct */
    if (BCM_PKT_HAS_SLTAG(pkt)) {
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDRX(unit, dv,
                                          (sal_vaddr_t)SOC_DV_SL_TAG(dv, idx),
                                          sizeof(uint32), 0));
    }

    /* Setup DCB for PAYLOAD + CRC:  Always into packet */
    SOC_IF_ERROR_RETURN(SOC_DCB_ADDRX(unit, dv,
                              (sal_vaddr_t)&(pkt->_pkt_data.data[bytes_used]),
                              pkt->_pkt_data.len - bytes_used,
                              0));

    return BCM_E_NONE;
}


/*
 * Set up a packet in a DV.  Use packet scatter for parts of packet
 */

STATIC INLINE int
rx_dv_add_pkt(int unit, volatile bcm_pkt_t *pkt, int idx, dv_t *dv)
{
    if (RX_CHAN_FLAGS(unit, DV_CHANNEL(dv)) & BCM_RX_F_MULTI_DCB) {
        BCM_IF_ERROR_RETURN(rx_multi_dv_pkt(unit, pkt, dv, idx));
    } else {
        /* Setup single DCB for all headers + PAYLOAD + CRC */
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDRX(unit, dv,
                                          (sal_vaddr_t)(pkt->_pkt_data.data),
                                          pkt->_pkt_data.len,
                                          0));
    }

    soc_dma_desc_end_packet(dv);

    return BCM_E_NONE;
}


/*
 * Function:
 *      rx_dv_fill
 * Purpose:
 *      Try to fill a DV with any packets it needs
 * Parameters:
 *      unit
 *      chan
 *      dv
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If successful changes the state of the DV.
 *      If memory error occurs, the state of the DV is not
 *      changed, but an error is not returned.
 */

STATIC INLINE void
rx_dv_fill(int unit, int chan, dv_t *dv)
{
    int         i;
    volatile bcm_pkt_t   *pkt;
    rx_dv_info_t   *save_info;
    static int warned = 0;
    void *pkt_data;
    int rv;

    pkt_data = NULL;
    save_info = DV_INFO(dv);  /* Save info before resetting dv */
    soc_dma_dv_reset(DV_RX, dv);
#ifdef BCM_RX_REFILL_FROM_INTR
    dv->dv_flags |= DV_F_NOTIFY_CHN;
#endif
    dv->_DV_INFO = save_info;  /* Restore info to DV */

    assert(DV_STATE(dv) == DV_S_NEEDS_FILL);

    for (i = 0; i < RX_PPC(unit); i++) {
        pkt = DV_PKT(dv, i);
        if (pkt->_pkt_data.data == NULL) {
            /* No pkt buffer; it needs to be allocated; use dflt size */
            rv = bcm_rx_alloc(unit, -1, RX_CHAN_FLAGS(unit, chan), &pkt_data);
            if (BCM_FAILURE(rv)) {/* Failed to allocate a pkt for this DV. */
                if (!warned) {
                    warned = 1;
                    LOG_WARN(BSL_LS_BCM_RX,
                             (BSL_META_U(unit,
                                         BSL_META_U
                                          (unit, "RX: Failed to allocate mem\n"))));
                }
                RX_CHAN_CTL(unit, chan).mem_fail++;
                return; /* Not an error, try again later */
            }
            pkt->_pkt_data.data = pkt_data;
            pkt->alloc_ptr = pkt_data;
            pkt->_pkt_data.len = rx_ctl[unit]->user_cfg.pkt_size;
        } else {
            pkt->_pkt_data.data = pkt->alloc_ptr;
        }

        /* Set up CRC stripping */
        if (RX_CHAN_FLAGS(unit, chan) & BCM_RX_F_CRC_STRIP) {
            pkt->flags |= BCM_RX_CRC_STRIP;
        } else {
            pkt->flags &= ~BCM_RX_CRC_STRIP;
        }

        /* Set up Vlan Tag stripping */
        if (RX_CHAN_FLAGS(unit, chan) & BCM_RX_F_VTAG_STRIP) {
            pkt->flags |= BCM_PKT_F_NO_VTAG;
        } else {
            pkt->flags &= ~BCM_PKT_F_NO_VTAG;
        }

        /* Set up the packet in the DCBs of the DV */
        if ((rv = rx_dv_add_pkt(unit, pkt, i, dv)) < 0) {
            DV_STATE(dv) = DV_S_ERROR;
            LOG_WARN(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U
                                  (unit, "Failed to add pkt %d to dv on unit %d: %s\n")),
                      i, unit, bcm_errmsg(rv)));
            break;
        }
    }

    if (DV_STATE(dv) != DV_S_ERROR) { /* Mark as ready to be started */
        DV_STATE(dv) = DV_S_FILLED;
        DV_PKTS_PROCESSED(dv) = 0;
    }

    return;
}


/*
 * Allocate and clear a DV (DMA chain control unit)
 * Local units only
 */
STATIC INLINE dv_t *
rx_dv_alloc(int unit, int chan, int dv_idx)
{
    dv_t *dv;
    rx_dv_info_t *dv_info;
    int clr_len;
    
    if ((dv = soc_dma_dv_alloc(unit, DV_RX, RX_DCB_PER_DV(unit, chan)))
                                  == NULL) {
        return NULL;
    }
    
    /* not a queued dv */
    if (dv->_DV_INFO == NULL) {
	    if ((dv_info = sal_alloc(sizeof(rx_dv_info_t), "dv_info")) == NULL) {
	    	soc_dma_dv_free(unit, dv);
	        return NULL;
	    }
	} else {
		dv_info = dv->_DV_INFO;
	}
    sal_memset(dv_info, 0, sizeof(rx_dv_info_t));

    /* DCBs MUST start off 0 */
    clr_len = SOC_DCB_SIZE(unit) * RX_DCB_PER_DV(unit, chan);
    sal_memset(dv->dv_dcb, 0, clr_len);

    /* Set up and install the DV and its info structure */
    dv->dv_done_chain = rx_done_chain;
    dv->dv_done_packet  = rx_done_packet;

    dv_info->idx = dv_idx;
    dv_info->chan = chan;
    dv_info->state = DV_S_NEEDS_FILL;
    dv->_DV_INFO = dv_info;

    DV_RX_IDX_SET(dv, rx_dv_count++);  /* Debug: Indicate DVs index */

    return dv;
}

/* Free a DV and any packets it controls */
STATIC INLINE void
rx_dv_dealloc(int unit, int chan, int dv_idx)
{
    dv_t *dv;
    rx_dv_info_t *dv_info;
    
    dv = RX_DV(unit, chan, dv_idx);

    if (dv) {
    	dv_info = DV_INFO(dv);
        RX_DV(unit, chan, dv_idx) = NULL;
        soc_dma_dv_free(unit, dv);
        if (!soc_dma_dv_valid(dv)) {
        	if (dv_info) {
        	    sal_free(dv_info);
        	}
        }
        
    }
}

void 
rx_dv_free_cb(int unit, dv_t *dv) 
{ 
    rx_dv_info_t *dv_info;
	if (unit) {
		;
	}	
    if (dv) {
    	if (dv->dv_done_chain == rx_done_chain) {
	    	dv_info = DV_INFO(dv);
	        if (dv_info) {
		        sal_free(dv_info);
		    }
		}
	}
    
}

/****************************************************************
 *
 * Rate limiting code
 *
 ****************************************************************/

/*
 * Function:
 *      rx_init_all_tokens
 * Purpose:
 *      Initialize all RX token buckets
 * Parameters:
 *      unit - Strata XGS unit number
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Includes top level limit, per channel limits and per cos limits.
 */

STATIC void
rx_init_all_tokens(int unit)
{
    int cos;
    sal_usecs_t cur_time;
    rx_queue_t *queue;

    cur_time = sal_time_usecs();
    RX_TOKENS(unit) = RX_BURST(unit);
    rx_ctl[unit]->last_fill = cur_time;

    last_fill_check = sal_time_usecs();
    for (cos = 0; cos <= RX_QUEUE_MAX(unit); cos++) {
        queue = RX_QUEUE(unit, cos);
        queue->tokens = queue->burst;
        queue->last_fill = cur_time;
    }

    rx_control.system_tokens = rx_control.system_pps;
    rx_control.system_last_fill = cur_time;
}


/* Initialize the queues. */
STATIC void
rx_queue_init(int unit, rx_ctl_t *rx_ctrl_ptr)
{
    int cos;
    rx_queue_t *queue;

    for (cos = 0; cos <= rx_ctrl_ptr->queue_max; cos++) {
        queue = rx_ctrl_ptr->pkt_queue + cos;
        queue->head = NULL;
        queue->tail = NULL;
        queue->count = 0;
        queue->max_len = RX_Q_MAX_DFLT;
    }
}

/****************************************************************
 *
 * Other functions accessed through RX subsystem:
 *     Discard packet handler:  Always registered first
 *     Default alloc/free routines:  Provided for default config
 *
 ****************************************************************/

/*
 * Function:
 *      rx_discard_packet
 * Purpose:
 *      Lowest priority registered handler that discards packet.
 * Parameters:
 *      unit - StrataSwitch Unit number.
 *      pkt - The packet to handle
 *      cookie - (Not used)
 * Returns:
 *      bcm_rx_handled
 */

STATIC bcm_rx_t
rx_discard_packet(int unit, bcm_pkt_t *pkt, void *cookie)
{
    int chan;

    COMPILER_REFERENCE(cookie);
    chan = pkt->dma_channel;

    ++(rx_ctl[unit]->chan_ctl[chan].dpkt);
    rx_ctl[unit]->chan_ctl[chan].dbyte += pkt->tot_len;

    return(BCM_RX_HANDLED);
}

#if defined(BROADCOM_DEBUG)

static char *dv_state_names[] = DV_STATE_STRINGS;

/*
 * Function:
 *      bcm_rx_show
 * Purpose:
 *      Show RX information for the specified device.
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      Nothing.
 */

int
_bcm_common_rx_show(int unit)
{
    volatile rx_callout_t      *rco;
    int chan;
    dv_t *dv;
    int i;
    sal_usecs_t cur_time;
    rx_queue_t *queue;

    RX_UNIT_CHECK(unit);

    if (!RX_INIT_DONE(unit)) {
        RX_PRINT(("Initializing RX subsystem (%d)\n",
                  bcm_rx_init(unit)));
    }

    cur_time = sal_time_usecs();
    RX_PRINT(("RX Info @ time=%u: %sstarted. Last fill %u. "
              "Thread is %srunning.\n"
              "    +verbose for more info\n",
              cur_time,
              RX_UNIT_STARTED(unit) ? "" : "not ",
              last_fill_check,
              rx_control.thread_running ? "" : "not "));

    RX_PRINT(("    Pkt Size %d. Pkts/Chain %d. All COS PPS %d. Burst %d."
              " Flags %x.\n",
              RX_PKT_SIZE(unit), RX_PPC(unit), RX_PPS(unit), RX_BURST(unit),
              RX_USER_FLAGS(unit)));
    RX_PRINT(("    Sys PPS %d. Sys tokens %d. Sys fill %u.\n",
              rx_control.system_pps, rx_control.system_tokens,
              rx_control.system_last_fill));
    RX_PRINT(("    Cntrs:  Pkts %d. Last start %d. Tunnel %d. Owned %d.\n"
              "        Bad Hndlr %d. No Hndlr %d. Not Running %d.\n"
              "        Thrd Not Running %d. DCB Errs %d.\n",
              rx_ctl[unit]->tot_pkts,
              rx_ctl[unit]->pkts_since_start,
              rx_ctl[unit]->tunnelled,
              rx_ctl[unit]->pkts_owned,
              rx_ctl[unit]->bad_hndlr_rv,
              rx_ctl[unit]->no_hndlr,
              rx_ctl[unit]->not_running,
              rx_ctl[unit]->thrd_not_running,
              rx_ctl[unit]->dcb_errs));
    LOG_VERBOSE(BSL_LS_BCM_RX,
                (BSL_META_U(unit,
                            BSL_META_U
                             (unit, "    Tokens %d. Sleep %d.\n"
                             "    Thread: %p. run %d. exitted %d. pri %d.\n")),
                 RX_TOKENS(unit),
                 rx_control.sleep_cur,
                 (void *)rx_control.rx_tid,
                 rx_control.thread_running,
                 rx_control.thread_exit_complete,
                 SOC_UNIT_VALID(unit) ? soc_property_get(unit,
                 spn_BCM_RX_THREAD_PRI,
                 RX_THREAD_PRI_DFLT) : 0));

    RX_PRINT(("  Registered callbacks:\n"));
    /* Display callouts and priority in order */
    for (rco = rx_ctl[unit]->rc_callout; rco; rco = rco->rco_next) {
        RX_PRINT(("        %-10s Priority=%3d%s. "
                  "Argument=0x%x. COS 0x%x%08x.\n",
                  rco->rco_name, (uint32)rco->rco_priority,
                  (rco->rco_flags & BCM_RCO_F_INTR) ? " Interrupt" : "",
                  PTR_TO_INT(rco->rco_cookie),
                  rco->rco_cos[1], rco->rco_cos[0]));
        RX_PRINT(("        %10s Packets handled %u, owned %u.\n", " ",
                  rco->rco_pkts_handled,
                  rco->rco_pkts_owned));
    }

    RX_PRINT(("  Channel Info\n"));

    FOREACH_SETUP_CHANNEL(unit, chan) {
        RX_PRINT(("    Chan %d is %s: Chains %d. COS 0x%x. DCB/pkt %d\n",
                  chan,
                  RX_CHAN_RUNNING(unit, chan) ? "running" : "setup",
                  RX_CHAINS(unit, chan),
                  RX_CHAN_CFG(unit, chan).cos_bmp,
                  RX_CHAN_CTL(unit, chan).dcb_per_pkt));
        RX_PRINT(("        rpkt %d. rbyte %d. dpkt %d. dbyte %d. "
                  "mem fail %d flags %x.\n",
                  RX_CHAN_CTL(unit, chan).rpkt, RX_CHAN_CTL(unit, chan).rbyte,
                  RX_CHAN_CTL(unit, chan).dpkt, RX_CHAN_CTL(unit, chan).dbyte,
                  RX_CHAN_CTL(unit, chan).mem_fail, RX_CHAN_FLAGS(unit, chan)));

        /* Display all DVs allocated */
        if (bsl_check(bslLayerSoc, bslSourceCommon, bslSeverityVerbose, unit)) {
            for (i = 0; i < RX_CHAINS(unit, chan); i++) {
                dv = RX_DV(unit, chan, i);
                if (dv == NULL || !soc_dma_dv_valid(dv)) {
                    RX_PRINT(("        DV %d (%p) is not valid\n", 
                              i, (void *)dv));
                } else {
                    RX_PRINT(("        DV %d (%d, %p) %s. chan %d. "
                              "pkt cnt %d. pkt[%d] %p\n",
                              i, DV_INDEX(dv), (void *)dv,
                              dv_state_names[DV_STATE(dv)],
                              DV_CHANNEL(dv),
                              DV_PKTS_PROCESSED(dv),
                              DV_PKTS_PROCESSED(dv),
                              (void *)DV_PKT(dv, DV_PKTS_PROCESSED(dv))));
                    if (DV_STATE(dv) == DV_S_SCHEDULED) {
                        RX_PRINT(("            Sched at %u for %d; "
                                  "future %d\n",
                                  DV_SCHED_TIME(dv),
                                  DV_TIME_DIFF(dv),
                                  DV_FUTURE_US(dv, cur_time)));
                    }
                }
            }
        }
    }

    RX_PRINT(("  Queue Info\n"));

    /* Display queue info */
    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
#if 0
        if (i < 16) {
            /* legacy support */
            if (!(BCM_RCO_F_COS_ACCEPT(i) & BCM_RCO_F_COS_ACCEPT_MASK)) {
                /* Skip unused queues */
                continue;
            }
        }
#endif
        queue = RX_QUEUE(unit, i);
         RX_PRINT(("    Queue %d: PPS %d. CurPkts %d. TotPkts %d. "
                   "Disc rate %d, qlen %d.\n",
                   i, queue->pps,
                   queue->count,
                   queue->tot_pkts,
                   queue->rate_disc,
                   queue->qlen_disc));
         LOG_VERBOSE(BSL_LS_BCM_RX,
                     (BSL_META_U(unit,
                                 BSL_META_U(unit, "        Tokens %d. Fill %u. Max %d. "
                                  "Brst %d. Head %p. Tail %p.\n")),
                      queue->tokens,
                      queue->last_fill,
                      queue->max_len,
                      queue->burst,
                      (void *)queue->head,
                      (void *)queue->tail));
    }

    return BCM_E_NONE;
}

#endif  /* BROADCOM_DEBUG */

#endif /* (defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT)) */

/*
 * Function:
 *      bcm_rx_cfg_t_init
 * Purpose:
 *      Initialize the RX configuration structure.
 * Parameters:
 *      rx_cfg - Pointer to RX configuration structure.
 * Returns:
 *      NONE
 */
void
bcm_rx_cfg_t_init(bcm_rx_cfg_t *rx_cfg)
{
    if (rx_cfg != NULL) {
        sal_memset(rx_cfg, 0, sizeof (*rx_cfg));
    }
    return;
}


/*
 * Function:
 *      bcm_rx_trap_config_t_init
 * Purpose:
 *      Initialize the bcm_rx_trap_config_t structure.
 * Parameters:
 *      rx_trap - Pointer to bcm_rx_trap_config_t structure.
 * Returns:
 *      NONE
 */
void
bcm_rx_trap_config_t_init(bcm_rx_trap_config_t *rx_trap)
{
    if (rx_trap != NULL) {
        sal_memset(rx_trap, 0, sizeof (*rx_trap));
    }
    return;
}


/*
 * Function:
 *      bcm_rx_snoop_config_t_init
 * Purpose:
 *      Initialize the bcm_rx_snoop_config_t structure.
 * Parameters:
 *      rx_snoop - Pointer to bcm_rx_snoop_config_t structure.
 * Returns:
 *      NONE
 */
void
bcm_rx_snoop_config_t_init(bcm_rx_snoop_config_t *rx_snoop)
{
    if (rx_snoop != NULL) {
        sal_memset(rx_snoop, 0, sizeof (*rx_snoop));
    }
    return;
}
