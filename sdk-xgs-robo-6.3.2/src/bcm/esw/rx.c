/*
 * $Id: rx.c 1.212 Broadcom SDK $
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

/* We need to call top-level APIs on other units */
#ifdef BCM_HIDE_DISPATCHABLE
#undef BCM_HIDE_DISPATCHABLE
#endif

#include <shared/alloc.h>

#include <soc/drv.h>
#include <soc/higig.h>

#include <bcm/rx.h>
#include <bcm_int/common/rx.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/rx.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/control.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/katana.h>
#include <bcm_int/esw/katana2.h>

#include <bcm_int/api_xlate_port.h>

#if defined(BROADCOM_DEBUG)
#include <soc/debug.h>
#include <soc/cm.h>
#define RX_DEBUG(stuff)        soc_cm_debug stuff
#define RX_PRINT(stuff)        soc_cm_print stuff

#if 0
#define RX_VERY_VERBOSE(stuff) soc_cm_debug stuff
#else
#define RX_VERY_VERBOSE(stuff)
#endif

#else
#define RX_DEBUG(stuff)
#define RX_VERY_VERBOSE(stuff)
#endif  /* defined(BROADCOM_DEBUG) */

#include <bcm_int/rpc/rlink.h>

/*
 * Function:
 *      bcm_esw_rx_sched_register
 * Purpose:
 *      Rx scheduler registration function. 
 * Parameters:
 *      unit       - (IN) Unused. 
 *      sched_cb   - (IN) Rx scheduler routine.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_sched_register(int unit, bcm_rx_sched_cb sched_cb)
{
    return (_bcm_common_rx_sched_register(unit, sched_cb));
}

/*
 * Function:
 *      bcm_esw_rx_sched_unregister
 * Purpose:
 *      Rx scheduler de-registration function. 
 * Parameters:
 *      unit  - (IN) Unused. 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_sched_unregister(int unit)
{
    return (_bcm_common_rx_sched_unregister(unit));
}

/*
 * Function:
 *      bcm_esw_rx_unit_next_get
 * Purpose:
 *      Rx started units iteration routine.
 * Parameters:
 *      unit       - (IN)  BCM device number. 
 *      unit_next  - (OUT) Next attached unit with started rx.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_unit_next_get(int unit, int *unit_next)
{
    return (_bcm_common_rx_unit_next_get(unit, unit_next));
}

/*
 * Function:
 *      bcm_esw_rx_queue_max_get
 * Purpose:
 *      Get maximum cos queue number for the device.
 * Parameters:
 *      unit    - (IN) BCM device number. 
 *      cosq    - (OUT) Maximum queue priority.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_queue_max_get(int unit, bcm_cos_queue_t  *cosq)
{  
    return (_bcm_common_rx_queue_max_get(unit, cosq));
}

/*
 * Function:
 *      bcm_esw_rx_queue_packet_count_get
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
bcm_esw_rx_queue_packet_count_get(int unit, bcm_cos_queue_t cosq, int *packet_count)
{
    return (_bcm_common_rx_queue_packet_count_get(unit, cosq, packet_count));
}

/*
 * Function:
 *      bcm_esw_rx_queue_rate_limit_status_get
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
bcm_esw_rx_queue_rate_limit_status_get(int unit, bcm_cos_queue_t cosq, 
                                       int *packet_tokens)
{
    return (_bcm_common_rx_queue_rate_limit_status_get(unit, cosq, 
						       packet_tokens));
}


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
bcm_esw_rx_init(int unit)
{
    return (_bcm_common_rx_init(unit));
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
bcm_esw_rx_cfg_init(int unit)
{
    return (_bcm_common_rx_cfg_init(unit));
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
bcm_esw_rx_start(int unit, bcm_rx_cfg_t *cfg)
{
    return (_bcm_common_rx_start(unit, cfg));
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
bcm_esw_rx_stop(int unit, bcm_rx_cfg_t *cfg)
{
    return (_bcm_common_rx_stop(unit, cfg));
}


/*
 * Function:
 *      bcm_esw_rx_clear
 * Purpose:
 *      Clear all RX info
 * Returns:
 *      BCM_E_NONE
 */

int
bcm_esw_rx_clear(int unit)
{
    return (_bcm_common_rx_clear(unit));
}


int bcm_esw_rx_deinit(int unit)
{
    return _bcm_rx_shutdown(unit);
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
bcm_esw_rx_cfg_get(int unit, bcm_rx_cfg_t *cfg)
{
    return (_bcm_common_rx_cfg_get(unit, cfg));
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
bcm_esw_rx_queue_register(int unit, const char *name, bcm_cos_queue_t cosq, 
                          bcm_rx_cb_f callback, uint8 priority, void *cookie, 
                          uint32 flags)
{
    return (_bcm_common_rx_queue_register(unit, name, cosq, callback,
                                       priority, cookie, flags));
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
bcm_esw_rx_register(int unit, const char *name, bcm_rx_cb_f callback,
                uint8 priority, void *cookie, uint32 flags)
{
    return (_bcm_common_rx_register(unit, name, callback,
                                    priority, cookie, flags));
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
bcm_esw_rx_unregister(int unit, bcm_rx_cb_f callback, uint8 priority)
{
    return (_bcm_common_rx_unregister(unit, callback, priority));
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
bcm_esw_rx_queue_unregister(int unit, bcm_cos_queue_t cosq,
                            bcm_rx_cb_f callback, uint8 priority)
{
    return (_bcm_common_rx_queue_unregister(unit, cosq, callback, priority));
}

/*
 * Function:
 *      bcm_rx_cosq_mapping_size_get
 * Purpose:
 *      Get number of COSQ mapping entries
 * Parameters:
 *      unit - Unit reference
 *      size - (OUT) number of entries
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_cosq_mapping_size_get(int unit, int *size)
{
    if (size == NULL) {
        return BCM_E_PARAM;
    }

    if (SOC_UNIT_VALID(unit) && SOC_MEM_IS_VALID(unit, CPU_COS_MAPm)) {
        *size = soc_mem_index_count(unit, CPU_COS_MAPm);
        return BCM_E_NONE;
    }

    *size = 0;
    return BCM_E_UNAVAIL;
}

#ifdef BCM_TRX_SUPPORT

/* CPU_COS_MAP_KEY format for TRX */
#define _bcm_scorpion_rx_cpu_cosmap_key_max             39
#define _bcm_triumph_rx_cpu_cosmap_key_max              44
#define _bcm_triumph2_rx_cpu_cosmap_key_max             59
#define _bcm_trident_rx_cpu_cosmap_key_max              60
#define _bcm_katana_rx_cpu_cosmap_key_max               62
#define _bcm_enduro_rx_cpu_cosmap_key_max               50
#define _bcm_triumph_rx_cpu_max_cosq                    48
#define _bcm_trident2_rx_cpu_max_cosq                   44
#define _bcm_triumph3_rx_cpu_max_cosq                   45
#define _bcm_scorpion_rx_cpu_max_cosq                   32
#define _bcm_hurricane_rx_cpu_max_cosq                   8
#define _bcm_tr3_rx_ip_cpu_cosmap_key_max               90
#define _bcm_tr3_rx_ep_cpu_cosmap_key_max               15
#define _bcm_tr3_rx_nlf_cpu_cosmap_key_max               7
#define _bcm_tr3_rx_cpu_cosmap_key_overlays              3
#define _bcm_tr3_rx_cpu_cosmap_key_words                 3
#define _bcm_tr3_rx_cpu_cosmap_reason_type_mask        0x3
#define _bcm_rx_cpu_cosmap_key_words_max                 3 /* For now */



static bcm_rx_reason_t 
_bcm_trx_cpu_cos_map_key [] =
{
    bcmRxReasonBpdu,                    /* PROTOCOL_BPDU */
    bcmRxReasonIpmcReserved,            /* PROTOCOL_IPMC_RSVD */
    bcmRxReasonDhcp,                    /* PROTOCOL_DHCP */
    bcmRxReasonIgmp,                    /* PROTOCOL_IGMP */
    bcmRxReasonArp,                     /* PROTOCOL_ARP */
    bcmRxReasonUnknownVlan,             /* CPU_UVLAN. */
    bcmRxReasonSharedVlanMismatch,      /* PVLAN_MISMATCH */
    bcmRxReasonDosAttack,               /* CPU_DOS_ATTACK */
    bcmRxReasonParityError,             /* PARITY_ERROR */
    bcmRxReasonHigigControl,            /* MH_CONTROL */
    bcmRxReasonTtl1,                    /* TTL_1 */
    bcmRxReasonL3Slowpath,              /* IP_OPTIONS_PKT. */
    bcmRxReasonL2SourceMiss,            /* CPU_SLF */
    bcmRxReasonL2DestMiss,              /* CPU_DLF */
    bcmRxReasonL2Move,                  /* CPU_L2MOVE */
    bcmRxReasonL2Cpu,                   /* CPU_L2CPU */
    bcmRxReasonL2NonUnicastMiss,        /* PBT_NONUC_PKT */
    bcmRxReasonL3SourceMiss,            /* CPU_L3SRC_MISS. */
    bcmRxReasonL3DestMiss,              /* CPU_L3DST_MISS */
    bcmRxReasonL3SourceMove,            /* CPU_L3SRC_MOVE */
    bcmRxReasonMcastMiss,               /* CPU_MC_MISS */
    bcmRxReasonIpMcastMiss,             /* CPU_IPMC_MISS */
    bcmRxReasonL3HeaderError,           /* CPU_L3HDR_ERR */
    bcmRxReasonMartianAddr,             /* CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* CPU_TUNNEL_ERR */
    bcmRxReasonHigigHdrError,           /* HGHDR_ERROR */
    bcmRxReasonMcastIdxError,           /* MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* VFP */
    bcmRxReasonClassBasedMove,          /* CBSM_PREVENTED */
    bcmRxReasonL2LearnLimit,            /* MAC_LIMIT */
    bcmRxReasonE2eHolIbp,               /* E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* HG_HDR_TYPE1 */
    bcmRxReasonNhop,                    /* NHOP */
    bcmRxReasonUrpfFail,                /* URPF_FAILED */
    bcmRxReasonFilterMatch,             /* CPU_FFP */
    bcmRxReasonIcmpRedirect,            /* ICMP_REDIRECT */
    bcmRxReasonSampleSource,            /* CPU_SFLOW_SRC */
    bcmRxReasonSampleDest,              /* CPU_SFLOW_DST */
    bcmRxReasonL3MtuFail,               /* L3_MTU_CHECK_FAIL */

    /* Below are not available for Scorpion */
    bcmRxReasonMplsLabelMiss,           /* MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* MPLS_SEQ_NUM_FAIL */

    /* Below are not available Triumph */
    bcmRxReasonMplsCtrlWordError,       /* MPLS_CW_TYPE_NOT_ZERO */
    bcmRxReasonMmrp,                    /* PROTOCOL_MMRP */
    bcmRxReasonSrp,                     /* PROTOCOL_SRP */
    bcmRxReasonWlanSlowpathKeepalive,   /* CAPWAP_KEEPALIVE */
    bcmRxReasonWlanClientError,         /* WLAN_CLIENT_DATABASE_ERROR */
    bcmRxReasonWlanDot1xDrop,           /* WLAN_DOT1X_DROP */
    bcmRxReasonWlanSlowpath,            /* WLAN_CAPWAP_SLOWPATH */
    bcmRxReasonEncapHigigError,         /* EHG_NONHG */
    bcmRxReasonTunnelControl,           /* AMT_CONTROL_PKT */
    bcmRxReasonTimeSync,                /* TIME_SYNC_PKT */
    bcmRxReasonOAMSlowpath,             /* OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* OAM_ERROR */
    bcmRxReasonL2Marked,                /* PROTOCOL_L2_PKT */
    bcmRxReasonL3AddrBindFail,          /* MAC_BIND_FAIL */
    bcmRxReasonIpfixRateViolation       /* IPFIX_FLOW */
};

#ifdef BCM_ENDURO_SUPPORT
static bcm_rx_reason_t 
_bcm_enduro_cpu_cos_map_key [] =
{
    bcmRxReasonBpdu,                    /* PROTOCOL_BPDU */
    bcmRxReasonIpmcReserved,            /* PROTOCOL_IPMC_RSVD */
    bcmRxReasonDhcp,                    /* PROTOCOL_DHCP */
    bcmRxReasonIgmp,                    /* PROTOCOL_IGMP */
    bcmRxReasonArp,                     /* PROTOCOL_ARP */
    bcmRxReasonUnknownVlan,             /* CPU_UVLAN. */
    bcmRxReasonSharedVlanMismatch,      /* PVLAN_MISMATCH */
    bcmRxReasonDosAttack,               /* CPU_DOS_ATTACK */
    bcmRxReasonParityError,             /* PARITY_ERROR */
    bcmRxReasonHigigControl,            /* MH_CONTROL */
    bcmRxReasonTtl1,                    /* TTL_1 */
    bcmRxReasonL3Slowpath,              /* IP_OPTIONS_PKT. */
    bcmRxReasonL2SourceMiss,            /* CPU_SLF */
    bcmRxReasonL2DestMiss,              /* CPU_DLF */
    bcmRxReasonL2Move,                  /* CPU_L2MOVE */
    bcmRxReasonL2Cpu,                   /* CPU_L2CPU */
    bcmRxReasonL2NonUnicastMiss,        /* PBT_NONUC_PKT */
    bcmRxReasonL3SourceMiss,            /* CPU_L3SRC_MISS. */
    bcmRxReasonL3DestMiss,              /* CPU_L3DST_MISS */
    bcmRxReasonL3SourceMove,            /* CPU_L3SRC_MOVE */
    bcmRxReasonMcastMiss,               /* CPU_MC_MISS */
    bcmRxReasonIpMcastMiss,             /* CPU_IPMC_MISS */
    bcmRxReasonL3HeaderError,           /* CPU_L3HDR_ERR */
    bcmRxReasonMartianAddr,             /* CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* CPU_TUNNEL_ERR */
    bcmRxReasonHigigHdrError,           /* HGHDR_ERROR */
    bcmRxReasonMcastIdxError,           /* MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* VFP */
    bcmRxReasonClassBasedMove,          /* CBSM_PREVENTED */
    bcmRxReasonL2LearnLimit,            /* MAC_LIMIT */
    bcmRxReasonE2eHolIbp,               /* E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* HG_HDR_TYPE1 */
    bcmRxReasonNhop,                    /* NHOP */
    bcmRxReasonUrpfFail,                /* URPF_FAILED */
    bcmRxReasonFilterMatch,             /* CPU_FFP */
    bcmRxReasonIcmpRedirect,            /* ICMP_REDIRECT */
    bcmRxReasonSampleSource,            /* CPU_SFLOW_SRC */
    bcmRxReasonSampleDest,              /* CPU_SFLOW_DST */
    bcmRxReasonL3MtuFail,               /* L3_MTU_CHECK_FAIL */
    bcmRxReasonMplsLabelMiss,           /* MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* MPLS_SEQ_NUM_FAIL */
    bcmRxReasonMplsCtrlWordError,       /* MPLS_CW_TYPE_NOT_ZERO */
    bcmRxReasonTimeSync,                /* TIME_SYNC_PKT */
    bcmRxReasonOAMSlowpath,             /* OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* OAM_ERROR */
    bcmRxReasonOAMLMDM,                 /* OAM_LMDM */
    bcmRxReasonL2Marked                 /* PROTOCOL_L2_PKT */
};
#endif /* BCM_ENDURO_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
static bcm_rx_reason_t 
_bcm_trident_cpu_cos_map_key [] =
{
    bcmRxReasonBpdu,                    /* PROTOCOL_BPDU */
    bcmRxReasonIpmcReserved,            /* PROTOCOL_IPMC_RSVD */
    bcmRxReasonDhcp,                    /* PROTOCOL_DHCP */
    bcmRxReasonIgmp,                    /* PROTOCOL_IGMP */
    bcmRxReasonArp,                     /* PROTOCOL_ARP */
    bcmRxReasonUnknownVlan,             /* CPU_UVLAN. */
    bcmRxReasonSharedVlanMismatch,      /* PVLAN_MISMATCH */
    bcmRxReasonDosAttack,               /* CPU_DOS_ATTACK */
    bcmRxReasonParityError,             /* PARITY_ERROR */
    bcmRxReasonHigigControl,            /* MH_CONTROL */
    bcmRxReasonTtl1,                    /* TTL_1 */
    bcmRxReasonL3Slowpath,              /* IP_OPTIONS_PKT. */
    bcmRxReasonL2SourceMiss,            /* CPU_SLF */
    bcmRxReasonL2DestMiss,              /* CPU_DLF */
    bcmRxReasonL2Move,                  /* CPU_L2MOVE */
    bcmRxReasonL2Cpu,                   /* CPU_L2CPU */
    bcmRxReasonL2NonUnicastMiss,        /* PBT_NONUC_PKT */
    bcmRxReasonL3SourceMiss,            /* CPU_L3SRC_MISS. */
    bcmRxReasonL3DestMiss,              /* CPU_L3DST_MISS */
    bcmRxReasonL3SourceMove,            /* CPU_L3SRC_MOVE */
    bcmRxReasonMcastMiss,               /* CPU_MC_MISS */
    bcmRxReasonIpMcastMiss,             /* CPU_IPMC_MISS */
    bcmRxReasonL3HeaderError,           /* CPU_L3HDR_ERR */
    bcmRxReasonMartianAddr,             /* CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* CPU_TUNNEL_ERR */
    bcmRxReasonHigigHdrError,           /* HGHDR_ERROR */
    bcmRxReasonMcastIdxError,           /* MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* VFP */
    bcmRxReasonClassBasedMove,          /* CBSM_PREVENTED */
    bcmRxReasonVlanTranslate,           /* VXLT_MISS */
    bcmRxReasonE2eHolIbp,               /* E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* HG_HDR_TYPE1 */
    bcmRxReasonNhop,                    /* NHOP */
    bcmRxReasonUrpfFail,                /* URPF_FAILED */
    bcmRxReasonFilterMatch,             /* CPU_FFP */
    bcmRxReasonIcmpRedirect,            /* ICMP_REDIRECT */
    bcmRxReasonSampleSource,            /* CPU_SFLOW_SRC */
    bcmRxReasonSampleDest,              /* CPU_SFLOW_DST */
    bcmRxReasonL3MtuFail,               /* L3_MTU_CHECK_FAIL */
    bcmRxReasonMplsLabelMiss,           /* MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* MPLS_SEQ_NUM_FAIL */
    bcmRxReasonMplsCtrlWordError,       /* MPLS_CW_TYPE_NOT_ZERO */
    bcmRxReasonMmrp,                    /* PROTOCOL_MMRP */
    bcmRxReasonSrp,                     /* PROTOCOL_SRP */
    bcmRxReasonStation,                 /* MY_STATION */
    bcmRxReasonNiv,                     /* NIV_DROP_REASON_ENCODING */
    bcmRxReasonNiv,                     /*   -> */
    bcmRxReasonNiv,                     /* 3 bits */
    bcmRxReasonL3AddrBindFail,          /* MAC_BIND_FAIL */
    bcmRxReasonTunnelControl,           /* AMT_CONTROL_PKT */
    bcmRxReasonTimeSync,                /* TIME_SYNC_PKT */
    bcmRxReasonOAMSlowpath,             /* OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* OAM_ERROR */
    bcmRxReasonL2Marked,                /* PROTOCOL_L2_PKT */
    bcmRxReasonTrill,                   /* TRILL_DROP_REASON_ENCODING */
    bcmRxReasonTrill,                   /*   -> */
    bcmRxReasonTrill                    /* 3 bits */
};

/* From FORMAT NIV_CPU_OPCODE_ENCODING */
static bcm_rx_reason_t
_bcm_niv_cpu_opcode_encoding[] =
{
    bcmRxReasonNiv,              /* 0: NO_ERRORS
                                  * Base field, must match the entries above */
    bcmRxReasonNivPrioDrop,      /* 1:DOT1P_ADMITTANCE_DISCARD */
    bcmRxReasonNivInterfaceMiss, /* 2:VIF_LOOKUP_MISS */
    bcmRxReasonNivRpfFail,       /* 3:RPF_LOOKUP_MISS */
    bcmRxReasonNivTagInvalid,    /* 4:VNTAG_FORMAT_ERROR */
    bcmRxReasonNivTagDrop,       /* 5:VNTAG_PRESENT_DROP */
    bcmRxReasonNivUntagDrop      /* 6:VNTAG_NOT_PRESENT_DROP */
};

/* From FORMAT TRILL_CPU_OPCODE_ENCODING */
static bcm_rx_reason_t
_bcm_trill_cpu_opcode_encoding[] =
{
    bcmRxReasonTrill,            /* 0:NO_ERRORS
                                  * Base field, must match the entries above */
    bcmRxReasonTrillInvalid,     /* 1:TRILL_HDR_ERROR */
    bcmRxReasonTrillMiss,        /* 2:TRILL_LOOKUP_MISS */
    bcmRxReasonTrillRpfFail,     /* 3:TRILL_RPF_CHECK_FAIL */
    bcmRxReasonTrillSlowpath,    /* 4:TRILL_SLOWPATH */
    bcmRxReasonTrillCoreIsIs,    /* 5:TRILL_CORE_IS_IS_PKT */
    bcmRxReasonTrillTtl,         /* 6:TRILL_HOP_COUNT_CHECK_FAIL */
    bcmRxReasonInvalid           
};
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
/* From FORMAT CPU_COS_MAP_KEY (exclude common bits (bit 0 to 6) */
static bcm_rx_reason_t
_bcm_trident2_cpu_cos_map_key[] =
{
    bcmRxReasonBpdu,                    /* 07:PROTOCOL_BPDU */
    bcmRxReasonIpmcReserved,            /* 08:PROTOCOL_IPMC_RSVD */
    bcmRxReasonDhcp,                    /* 09:PROTOCOL_DHCP */
    bcmRxReasonIgmp,                    /* 10:PROTOCOL_IGMP */
    bcmRxReasonArp,                     /* 11:PROTOCOL_ARP */
    bcmRxReasonUnknownVlan,             /* 12:CPU_UVLAN. */
    bcmRxReasonSharedVlanMismatch,      /* 13:PVLAN_MISMATCH */
    bcmRxReasonDosAttack,               /* 14:CPU_DOS_ATTACK */
    bcmRxReasonParityError,             /* 15:PARITY_ERROR */
    bcmRxReasonHigigControl,            /* 16:MH_CONTROL */
    bcmRxReasonTtl1,                    /* 17:TTL_1 */
    bcmRxReasonL3Slowpath,              /* 18:IP_OPTIONS_PKT. */
    bcmRxReasonL2SourceMiss,            /* 19:CPU_SLF */
    bcmRxReasonL2DestMiss,              /* 20:CPU_DLF */
    bcmRxReasonL2Move,                  /* 21:CPU_L2MOVE */
    bcmRxReasonL2Cpu,                   /* 22:CPU_L2CPU */
    bcmRxReasonL2NonUnicastMiss,        /* 23:PBT_NONUC_PKT */
    bcmRxReasonL3SourceMiss,            /* 24:CPU_L3SRC_MISS. */
    bcmRxReasonL3DestMiss,              /* 25:CPU_L3DST_MISS */
    bcmRxReasonL3SourceMove,            /* 26:CPU_L3SRC_MOVE */
    bcmRxReasonMcastMiss,               /* 27:CPU_MC_MISS */
    bcmRxReasonIpMcastMiss,             /* 28:CPU_IPMC_MISS */
    bcmRxReasonL3HeaderError,           /* 29:CPU_L3HDR_ERR */
    bcmRxReasonMartianAddr,             /* 30:CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* 31:CPU_TUNNEL_ERR */
    bcmRxReasonHigigHdrError,           /* 32:HGHDR_ERROR */
    bcmRxReasonMcastIdxError,           /* 33:MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* 34:VFP */
    bcmRxReasonClassBasedMove,          /* 35:CBSM_PREVENTED */
    bcmRxReasonCongestionCnm,           /* 36:ICNM */
    bcmRxReasonE2eHolIbp,               /* 37:E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* 38:HG_HDR_TYPE1 */
    bcmRxReasonNhop,                    /* 39:NHOP */
    bcmRxReasonUrpfFail,                /* 40:URPF_FAILED */
    bcmRxReasonFilterMatch,             /* 41:CPU_FFP */
    bcmRxReasonIcmpRedirect,            /* 42:ICMP_REDIRECT */
    bcmRxReasonSampleSource,            /* 43:CPU_SFLOW_SRC */
    bcmRxReasonSampleDest,              /* 44:CPU_SFLOW_DST */
    bcmRxReasonL3MtuFail,               /* 45:L3_MTU_CHECK_FAIL */
    bcmRxReasonMplsLabelMiss,           /* 46:MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* 47:MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* 48:MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* 49:MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* 50:MPLS_SEQ_NUM_FAIL */
    bcmRxReasonMplsUnknownAch,          /* 51:MPLS_UNKNOWN_ACH_ERROR */
    bcmRxReasonMmrp,                    /* 52:PROTOCOL_MMRP */
    bcmRxReasonSrp,                     /* 53:PROTOCOL_SRP */
    bcmRxReasonTimesyncUnknownVersion,  /* 54:IEEE1588_UNKNOWN_VERSION */
    bcmRxReasonMplsRouterAlertLabel,    /* 55:MPLS_ALERT_LABEL */
    bcmRxReasonMplsIllegalReservedLabel,/* 56:MPLS_ILLEGAL_RESERVED_LABEL */
    bcmRxReasonVlanTranslate,           /* 57:VXLT_MISS */
    bcmRxReasonTunnelControl,           /* 58:AMT_CONTROL_PKT */
    bcmRxReasonTimeSync,                /* 59:TIME_SYNC */
    bcmRxReasonOAMSlowpath,             /* 60:OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* 61:OAM_ERROR */
    bcmRxReasonL2Marked,                /* 62:PROTOCOL_L2_PKT */
    bcmRxReasonL3AddrBindFail,          /* 63:MAC_BIND_FAIL */
    bcmRxReasonStation,                 /* 64:MY_STATION */
    bcmRxReasonNiv,                     /* 65:NIV_DROP_REASON_ENCODING */
    bcmRxReasonNiv,                     /* 66:  -> */
    bcmRxReasonNiv,                     /* 67:3 bits */
    bcmRxReasonTrill,                   /* 68:TRILL_DROP_REASON_ENCODING */
    bcmRxReasonTrill,                   /* 69:  -> */
    bcmRxReasonTrill,                   /* 70:3 bits */
    bcmRxReasonL2GreSipMiss,            /* 71:L2GRE_SIP_MISS */
    bcmRxReasonL2GreVpnIdMiss,          /* 72:L2GRE_VPNID_MISS */
    bcmRxReasonBfdSlowpath,             /* 73:BFD_SLOWPATH */
    bcmRxReasonBfd,                     /* 74:BFD_ERROR */
    bcmRxReasonOAMLMDM,                 /* 75:OAM_LMDM */
    bcmRxReasonCongestionCnmProxy,      /* 76:QCN_CNM_PRP */
    bcmRxReasonCongestionCnmProxyError, /* 77:QCN_CNM_PRP_DLF */
    bcmRxReasonVxlanSipMiss,            /* 78:VXLAN_SIP_MISS */
    bcmRxReasonVxlanVpnIdMiss,          /* 79:VXLAN_VN_ID_MISS */
    bcmRxReasonFcoeZoneCheckFail,       /* 80:FCOE_ZONE_CHECK_FAIL */
    bcmRxReasonNat,                     /* 81:NAT_DROP_REASON_ENCODING */
    bcmRxReasonNat,                     /* 82:  -> */
    bcmRxReasonNat,                     /* 83:3 bits */
    bcmRxReasonIpmcInterfaceMismatch    /* 84:CPU_IPMC_INTERFACE_MISMATCH */
};

/* From FORMAT NAT_CPU_OPCODE_ENCODING */
static bcm_rx_reason_t
_bcm_nat_cpu_opcode_encoding[] =
{
    bcmRxReasonNat,              /* 0:NOP
                                  * Base field, must match the entries above */
    bcmRxReasonTcpUdpNatMiss,    /* 1:NORMAL */
    bcmRxReasonIcmpNatMiss,      /* 2:ICMP */
    bcmRxReasonNatFragment,      /* 3:FRAGMEMT */
    bcmRxReasonNatMiss           /* 4:OTHER */
};
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
static bcm_rx_reason_t 
_bcm_katana_cpu_cos_map_key [] =
{
    bcmRxReasonBpdu,                    /* PROTOCOL_BPDU */
    bcmRxReasonIpmcReserved,            /* PROTOCOL_IPMC_RSVD */
    bcmRxReasonDhcp,                    /* PROTOCOL_DHCP */
    bcmRxReasonIgmp,                    /* PROTOCOL_IGMP */
    bcmRxReasonArp,                     /* PROTOCOL_ARP */
    bcmRxReasonUnknownVlan,             /* CPU_UVLAN. */
    bcmRxReasonSharedVlanMismatch,      /* PVLAN_MISMATCH */
    bcmRxReasonDosAttack,               /* CPU_DOS_ATTACK */
    bcmRxReasonParityError,             /* PARITY_ERROR */
    bcmRxReasonHigigControl,            /* MH_CONTROL */
    bcmRxReasonTtl1,                    /* TTL_1 */
    bcmRxReasonL3Slowpath,              /* IP_OPTIONS_PKT. */
    bcmRxReasonL2SourceMiss,            /* CPU_SLF */
    bcmRxReasonL2DestMiss,              /* CPU_DLF */
    bcmRxReasonL2Move,                  /* CPU_L2MOVE */
    bcmRxReasonL2Cpu,                   /* CPU_L2CPU */
    bcmRxReasonL2NonUnicastMiss,        /* PBT_NONUC_PKT */
    bcmRxReasonL3SourceMiss,            /* CPU_L3SRC_MISS. */
    bcmRxReasonL3DestMiss,              /* CPU_L3DST_MISS */
    bcmRxReasonL3SourceMove,            /* CPU_L3SRC_MOVE */
    bcmRxReasonMcastMiss,               /* CPU_MC_MISS */
    bcmRxReasonIpMcastMiss,             /* CPU_IPMC_MISS */
    bcmRxReasonL3HeaderError,           /* CPU_L3HDR_ERR */
    bcmRxReasonMartianAddr,             /* CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* CPU_TUNNEL_ERR */
    bcmRxReasonHigigHdrError,           /* HGHDR_ERROR */
    bcmRxReasonMcastIdxError,           /* MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* VFP */
    bcmRxReasonClassBasedMove,          /* CBSM_PREVENTED */
    bcmRxReasonVlanTranslate,           /* VXLT_MISS */
    bcmRxReasonE2eHolIbp,               /* E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* HG_HDR_TYPE1 */
    bcmRxReasonNhop,                    /* NHOP */
    bcmRxReasonUrpfFail,                /* URPF_FAILED */
    bcmRxReasonFilterMatch,             /* CPU_FFP */
    bcmRxReasonIcmpRedirect,            /* ICMP_REDIRECT */
    bcmRxReasonSampleSource,            /* CPU_SFLOW_SRC */
    bcmRxReasonSampleDest,              /* CPU_SFLOW_DST */
    bcmRxReasonL3MtuFail,               /* L3_MTU_CHECK_FAIL */
    bcmRxReasonMplsLabelMiss,           /* MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* MPLS_SEQ_NUM_FAIL */
    bcmRxReasonBfdSlowpath,             /* BFD_SLOWPATH */
    bcmRxReasonMmrp,                    /* PROTOCOL_MMRP */
    bcmRxReasonSrp,                     /* PROTOCOL_SRP */
    bcmRxReasonStation,                 /* MY_STATION */
    bcmRxReasonNiv,                     /* NIV_DROP_REASON_ENCODING */
    bcmRxReasonNiv,                     /*   -> */
    bcmRxReasonNiv,                     /* 3 bits */
    bcmRxReasonL3AddrBindFail,          /* MAC_BIND_FAIL */
    bcmRxReasonTunnelControl,           /* AMT_CONTROL_PKT */
    bcmRxReasonTimeSync,                /* TIME_SYNC_PKT */
    bcmRxReasonOAMSlowpath,             /* OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* OAM_ERROR */
    bcmRxReasonL2Marked,                /* PROTOCOL_L2_PKT */
    bcmRxReasonOAMLMDM,                 /* OAM_LMDM */
    bcmRxReasonInvalid,                 /* RESERVED_0 */
    bcmRxReasonL2LearnLimit,            /* MAC_LIMIT */
    bcmRxReasonBfd,                     /* BFD_ERROR_ENCODING */
    bcmRxReasonBfd,
};

static bcm_rx_reason_t
_bcm_bfd_cpu_opcode_encoding[] =
{
    bcmRxReasonBfd,        /* Base field, must match the entries above */
    bcmRxReasonBfdUnknownVersion,
    bcmRxReasonBfdInvalidVersion,
    /* bcmRxReasonBfdLookupFailure, use special case to handle */
    bcmRxReasonBfdInvalidPacket
};

#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT


static bcm_rx_reason_t 
_bcm_triumph3_ip_cpu_cos_map_key [] =
{
    bcmRxReasonUnknownVlan,             /* CPU_UVLAN. */
    bcmRxReasonL2SourceMiss,            /* CPU_SLF */
    bcmRxReasonL2DestMiss,              /* CPU_DLF */
    bcmRxReasonL2Move,                  /* CPU_L2MOVE */
    bcmRxReasonL2Cpu,                   /* CPU_L2CPU */
    bcmRxReasonSampleSource,            /* CPU_SFLOW_SRC */
    bcmRxReasonSampleDest,              /* CPU_SFLOW_DST */
    bcmRxReasonL3DestMiss,              /* CPU_L3DST_MISS */
    bcmRxReasonL3SourceMiss,            /* CPU_L3SRC_MISS. */
    bcmRxReasonL3SourceMove,            /* CPU_L3SRC_MOVE */
    bcmRxReasonMcastMiss,               /* CPU_MC_MISS */
    bcmRxReasonIpMcastMiss,             /* CPU_IPMC_MISS */
    bcmRxReasonL3HeaderError,           /* CPU_L3HDR_ERR */
    bcmRxReasonBpdu,                    /* PROTOCOL_BPDU */
    bcmRxReasonIpmcReserved,            /* PROTOCOL_IPMC_RSVD */
    bcmRxReasonDhcp,                    /* PROTOCOL_DHCP */
    bcmRxReasonIgmp,                    /* PROTOCOL_IGMP */
    bcmRxReasonArp,                     /* PROTOCOL_ARP */
    bcmRxReasonMmrp,                    /* PROTOCOL_MMRP */
    bcmRxReasonSrp,                     /* PROTOCOL_SRP */
    bcmRxReasonTunnelControl,           /* AMT_CONTROL_PKT */
    bcmRxReasonL2Marked,                /* PROTOCOL_L2_PKT */
    bcmRxReasonIcmpRedirect,            /* ICMP_REDIRECT */
    bcmRxReasonDosAttack,               /* CPU_DOS_ATTACK */
    bcmRxReasonMartianAddr,             /* CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* CPU_TUNNEL_ERR */
    bcmRxReasonInvalid,                 /* L3_SLOWPATH - duplicate */
    bcmRxReasonL3MtuFail,               /* L3_MTU_CHECK_FAIL */
    bcmRxReasonMcastIdxError,           /* MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* VFP */
    bcmRxReasonClassBasedMove,          /* CBSM_PREVENTED */
    bcmRxReasonL3AddrBindFail,          /* MAC_BIND_FAIL */
    bcmRxReasonMplsLabelMiss,           /* MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* MPLS_SEQ_NUM_FAIL */
    bcmRxReasonL2NonUnicastMiss,        /* PBT_NONUC_PKT */
    bcmRxReasonNhop,                    /* NHOP */
    bcmRxReasonStation,                 /* MY_STATION */
    bcmRxReasonVlanTranslate,           /* VXLT_MISS */
    bcmRxReasonTimeSync,                /* TIME_SYNC_PKT */
    bcmRxReasonOAMSlowpath,             /* OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* OAM_ERROR */
    bcmRxReasonIpfixRateViolation,      /* IPFIX_FLOW */
    bcmRxReasonL2LearnLimit,            /* MAC_LIMIT */
    bcmRxReasonEncapHigigError,         /* EHG_NONHG */
    bcmRxReasonRegexMatch,              /* FLOW_TRACKER */
    bcmRxReasonBfd,                     /* BFD_ERROR */
    bcmRxReasonBfdSlowpath,             /* BFD_SLOWPATH */
    bcmRxReasonFailoverDrop,            /* PROTECTION_DROP_DATA */
    bcmRxReasonSharedVlanMismatch,      /* PVLAN_MISMATCH */
    bcmRxReasonHigigControl,            /* MH_CONTROL */
    bcmRxReasonTtl1,                    /* TTL_1 */
    bcmRxReasonL3Slowpath,              /* IP_OPTIONS_PKT. */
    bcmRxReasonE2eHolIbp,               /* E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* HG_HDR_TYPE1 */
    bcmRxReasonUrpfFail,                /* URPF_FAILED */
    bcmRxReasonNivUntagDrop,            /* NIV_UNTAG_DROP */
    bcmRxReasonNivTagDrop,              /* NIV_TAG_DROP */
    bcmRxReasonNivTagInvalid,           /* NIV_TAG_INVALID */
    bcmRxReasonNivRpfFail,              /* NIV_RPF_FAIL */
    bcmRxReasonNivInterfaceMiss,        /* NIV_INTERFACE_MISS */
    bcmRxReasonNivPrioDrop,             /* NIV_PRIO_DROP */
    bcmRxReasonTrillName,               /* NICKNAME_TABLE_COPYTOCPU */
    bcmRxReasonTrillTtl,                /* TRILL_HOP_COUNT_CHECK_FAIL */
    bcmRxReasonTrillCoreIsIs,           /* TRILL_CORE_IS_IS_PKT */
    bcmRxReasonTrillSlowpath,           /* TRILL_SLOWPATH */
    bcmRxReasonTrillRpfFail,            /* TRILL_RPF_CHECK_FAIL */
    bcmRxReasonTrillMiss,               /* TRILL_LOOKUP_MISS */
    bcmRxReasonTrillInvalid,            /* TRILL_HDR_ERROR */
    bcmRxReasonOAMLMDM,                 /* OAM_LMDM */
    bcmRxReasonWlanSlowpathKeepalive,   /* CAPWAP_KEEPALIVE */
    bcmRxReasonWlanTunnelError,         /* WLAN_SHIM_HEADER_ERROR_TO_CPU */
    bcmRxReasonWlanSlowpath,            /* WLAN_CAPWAP_SLOWPATH */
    bcmRxReasonWlanDot1xDrop,           /* WLAN_DOT1X_DROP */
    bcmRxReasonMplsReservedEntropyLabel, /* ENTROPY_LABEL_IN_UNALLOWED_RANGE */
    bcmRxReasonCongestionCnmProxy,      /* QCN_CNM_PRP */
    bcmRxReasonCongestionCnmProxyError, /* QCN_CNM_PRP_DLF */
    bcmRxReasonMplsUnknownAch,          /* MPLS_UNKNOWN_ACH_TYPE */
    bcmRxReasonMplsLookupsExceeded,     /* MPLS_OUT_OF_LOOKUPS */
    bcmRxReasonMplsIllegalReservedLabel, /* MPLS_ILLEGAL_RESERVED_LABEL */
    bcmRxReasonMplsRouterAlertLabel,    /* MPLS_ALERT_LABEL */
    bcmRxReasonParityError,             /* PARITY_ERROR */
    bcmRxReasonHigigHdrError,           /* HGHDR_ERROR */
    bcmRxReasonFilterMatch,             /* CPU_FFP */
    bcmRxReasonTimesyncUnknownVersion,  /* IEEE1588_UNKNOWN_VERSION */
    bcmRxReasonCongestionCnm,           /* ICNM */
    bcmRxReasonL2GreSipMiss,            /* L2GRE_SIP_MISS */
    bcmRxReasonL2GreVpnIdMiss,          /* L2GRE_VPNID_MISS */
};

static bcm_rx_reason_t 
_bcm_triumph3_ep_cpu_cos_map_key [] =
{
    bcmRxReasonUnknownVlan,             /* CPUE_VLAN */
    bcmRxReasonStp,                     /* CPUE_STG */
    bcmRxReasonVlanTranslate,           /* CPUE_VXLT */
    bcmRxReasonTunnelError,             /* CPUE_TUNNEL */
    bcmRxReasonIpmc,                    /* CPUE_L3ERR */
    bcmRxReasonL3HeaderError,           /* CPUE_L3PKT_ERR */
    bcmRxReasonTtl,                     /* CPUE_TTL_DROP */
    bcmRxReasonL2MtuFail,               /* CPUE_MTU */
    bcmRxReasonHigigHdrError,           /* CPUE_HIGIG */
    bcmRxReasonSplitHorizon,            /* CPUE_PRUNE */
    bcmRxReasonNivPrune,                /* CPUE_NIV_DISCARD */
    bcmRxReasonVirtualPortPrune,        /* CPUE_SPLIT_HORIZON */
    bcmRxReasonInvalid,                 /* CPUE_EFP */
    bcmRxReasonNonUnicastDrop,          /* CPUE_MULTI_DEST */
    bcmRxReasonTrillPacketPortMismatch, /* CPUE_TRILL */
    bcmRxReasonInvalid,                 /* Reserved */
    bcmRxReasonInvalid,                 /* Reserved */
    bcmRxReasonInvalid,                 /* Reserved */
    bcmRxReasonInvalid,                 /* Reserved */
    bcmRxReasonInvalid,                 /* Reserved */
    bcmRxReasonTunnelControl,           /* AMT_CONTROL_PKT */
    bcmRxReasonL2Marked,                /* PROTOCOL_L2_PKT */
    bcmRxReasonIcmpRedirect,            /* ICMP_REDIRECT */
    bcmRxReasonDosAttack,               /* CPU_DOS_ATTACK */
    bcmRxReasonMartianAddr,             /* CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* CPU_TUNNEL_ERR */
    bcmRxReasonInvalid,                 /* L3_SLOWPATH - duplicate */
    bcmRxReasonL3MtuFail,               /* L3_MTU_CHECK_FAIL */
    bcmRxReasonMcastIdxError,           /* MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* VFP */
    bcmRxReasonClassBasedMove,          /* CBSM_PREVENTED */
    bcmRxReasonL3AddrBindFail,          /* MAC_BIND_FAIL */
    bcmRxReasonMplsLabelMiss,           /* MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* MPLS_SEQ_NUM_FAIL */
    bcmRxReasonL2NonUnicastMiss,        /* PBT_NONUC_PKT */
    bcmRxReasonNhop,                    /* NHOP */
    bcmRxReasonStation,                 /* MY_STATION */
    bcmRxReasonVlanTranslate,           /* VXLT_MISS */
    bcmRxReasonTimeSync,                /* TIME_SYNC_PKT */
    bcmRxReasonOAMSlowpath,             /* OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* OAM_ERROR */
    bcmRxReasonIpfixRateViolation,      /* IPFIX_FLOW */
    bcmRxReasonL2LearnLimit,            /* MAC_LIMIT */
    bcmRxReasonEncapHigigError,         /* EHG_NONHG */
    bcmRxReasonRegexMatch,              /* FLOW_TRACKER */
    bcmRxReasonBfd,                     /* BFD_ERROR */
    bcmRxReasonBfdSlowpath,             /* BFD_SLOWPATH */
    bcmRxReasonFailoverDrop,            /* PROTECTION_DROP_DATA */
    bcmRxReasonSharedVlanMismatch,      /* PVLAN_MISMATCH */
    bcmRxReasonHigigControl,            /* MH_CONTROL */
    bcmRxReasonTtl1,                    /* TTL_1 */
    bcmRxReasonL3Slowpath,              /* IP_OPTIONS_PKT. */
    bcmRxReasonE2eHolIbp,               /* E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* HG_HDR_TYPE1 */
    bcmRxReasonUrpfFail,                /* URPF_FAILED */
    bcmRxReasonNivUntagDrop,            /* NIV_UNTAG_DROP */
    bcmRxReasonNivTagDrop,              /* NIV_TAG_DROP */
    bcmRxReasonNivTagInvalid,           /* NIV_TAG_INVALID */
    bcmRxReasonNivRpfFail,              /* NIV_RPF_FAIL */
    bcmRxReasonNivInterfaceMiss,        /* NIV_INTERFACE_MISS */
    bcmRxReasonNivPrioDrop,             /* NIV_PRIO_DROP */
    bcmRxReasonTrillName,               /* NICKNAME_TABLE_COPYTOCPU */
    bcmRxReasonTrillTtl,                /* TRILL_HOP_COUNT_CHECK_FAIL */
    bcmRxReasonTrillCoreIsIs,           /* TRILL_CORE_IS_IS_PKT */
    bcmRxReasonTrillSlowpath,           /* TRILL_SLOWPATH */
    bcmRxReasonTrillRpfFail,            /* TRILL_RPF_CHECK_FAIL */
    bcmRxReasonTrillMiss,               /* TRILL_LOOKUP_MISS */
    bcmRxReasonTrillInvalid,            /* TRILL_HDR_ERROR */
    bcmRxReasonOAMLMDM,                 /* OAM_LMDM */
    bcmRxReasonWlanSlowpathKeepalive,   /* CAPWAP_KEEPALIVE */
    bcmRxReasonWlanTunnelError,         /* WLAN_SHIM_HEADER_ERROR_TO_CPU */
    bcmRxReasonWlanSlowpath,            /* WLAN_CAPWAP_SLOWPATH */
    bcmRxReasonWlanDot1xDrop,           /* WLAN_DOT1X_DROP */
    bcmRxReasonMplsReservedEntropyLabel, /* ENTROPY_LABEL_IN_UNALLOWED_RANGE */
    bcmRxReasonCongestionCnmProxy,      /* QCN_CNM_PRP */
    bcmRxReasonCongestionCnmProxyError, /* QCN_CNM_PRP_DLF */
    bcmRxReasonMplsUnknownAch,          /* MPLS_UNKNOWN_ACH_TYPE */
    bcmRxReasonMplsLookupsExceeded,     /* MPLS_OUT_OF_LOOKUPS */
    bcmRxReasonMplsIllegalReservedLabel, /* MPLS_ILLEGAL_RESERVED_LABEL */
    bcmRxReasonMplsRouterAlertLabel,    /* MPLS_ALERT_LABEL */
    bcmRxReasonParityError,             /* PARITY_ERROR */
    bcmRxReasonHigigHdrError,           /* HGHDR_ERROR */
    bcmRxReasonFilterMatch,             /* CPU_FFP */
    bcmRxReasonTimesyncUnknownVersion,  /* IEEE1588_UNKNOWN_VERSION */
    bcmRxReasonCongestionCnm,           /* ICNM */
    bcmRxReasonL2GreSipMiss,            /* L2GRE_SIP_MISS */
    bcmRxReasonL2GreVpnIdMiss,          /* L2GRE_VPNID_MISS */
};

static bcm_rx_reason_t 
_bcm_triumph3_nlf_cpu_cos_map_key [] =
{
    bcmRxReasonRegexAction,              /* SM_TOCPU */
    bcmRxReasonWlanClientMove,          /* WLAN_MOVE_TOCPU */
    bcmRxReasonWlanSourcePortMiss,      /* WLAN_SVP_MISS_TOCPU */
    bcmRxReasonWlanClientError,         /* WLAN_DATABASE_ERROR */
    bcmRxReasonWlanClientSourceMiss,    /* WLAN_CLIENT_DATABASE_SA_MISS */
    bcmRxReasonWlanClientDestMiss,      /* WLAN_CLIENT_DATABASE_DA_MISS */
    bcmRxReasonWlanMtu,                 /* WLAN_MTU_FAIL */
    bcmRxReasonL3DestMiss,              /* CPU_L3DST_MISS */
    bcmRxReasonL3SourceMiss,            /* CPU_L3SRC_MISS. */
    bcmRxReasonL3SourceMove,            /* CPU_L3SRC_MOVE */
    bcmRxReasonMcastMiss,               /* CPU_MC_MISS */
    bcmRxReasonIpMcastMiss,             /* CPU_IPMC_MISS */
    bcmRxReasonL3HeaderError,           /* CPU_L3HDR_ERR */
    bcmRxReasonBpdu,                    /* PROTOCOL_BPDU */
    bcmRxReasonIpmcReserved,            /* PROTOCOL_IPMC_RSVD */
    bcmRxReasonDhcp,                    /* PROTOCOL_DHCP */
    bcmRxReasonIgmp,                    /* PROTOCOL_IGMP */
    bcmRxReasonArp,                     /* PROTOCOL_ARP */
    bcmRxReasonMmrp,                    /* PROTOCOL_MMRP */
    bcmRxReasonSrp,                     /* PROTOCOL_SRP */
    bcmRxReasonTunnelControl,           /* AMT_CONTROL_PKT */
    bcmRxReasonL2Marked,                /* PROTOCOL_L2_PKT */
    bcmRxReasonIcmpRedirect,            /* ICMP_REDIRECT */
    bcmRxReasonDosAttack,               /* CPU_DOS_ATTACK */
    bcmRxReasonMartianAddr,             /* CPU_MARTIAN_ADDR */
    bcmRxReasonTunnelError,             /* CPU_TUNNEL_ERR */
    bcmRxReasonInvalid,                 /* L3_SLOWPATH - duplicate */
    bcmRxReasonL3MtuFail,               /* L3_MTU_CHECK_FAIL */
    bcmRxReasonMcastIdxError,           /* MCIDX_ERROR */
    bcmRxReasonVlanFilterMatch,         /* VFP */
    bcmRxReasonClassBasedMove,          /* CBSM_PREVENTED */
    bcmRxReasonL3AddrBindFail,          /* MAC_BIND_FAIL */
    bcmRxReasonMplsLabelMiss,           /* MPLS_LABEL_MISS */
    bcmRxReasonMplsInvalidAction,       /* MPLS_INVALID_ACTION */
    bcmRxReasonMplsInvalidPayload,      /* MPLS_INVALID_PAYLOAD */
    bcmRxReasonMplsTtl,                 /* MPLS_TTL_CHECK_FAIL */
    bcmRxReasonMplsSequenceNumber,      /* MPLS_SEQ_NUM_FAIL */
    bcmRxReasonL2NonUnicastMiss,        /* PBT_NONUC_PKT */
    bcmRxReasonNhop,                    /* NHOP */
    bcmRxReasonStation,                 /* MY_STATION */
    bcmRxReasonVlanTranslate,           /* VXLT_MISS */
    bcmRxReasonTimeSync,                /* TIME_SYNC_PKT */
    bcmRxReasonOAMSlowpath,             /* OAM_SLOWPATH */
    bcmRxReasonOAMError,                /* OAM_ERROR */
    bcmRxReasonIpfixRateViolation,      /* IPFIX_FLOW */
    bcmRxReasonL2LearnLimit,            /* MAC_LIMIT */
    bcmRxReasonEncapHigigError,         /* EHG_NONHG */
    bcmRxReasonRegexMatch,              /* FLOW_TRACKER */
    bcmRxReasonBfd,                     /* BFD_ERROR */
    bcmRxReasonBfdSlowpath,             /* BFD_SLOWPATH */
    bcmRxReasonFailoverDrop,            /* PROTECTION_DROP_DATA */
    bcmRxReasonSharedVlanMismatch,      /* PVLAN_MISMATCH */
    bcmRxReasonHigigControl,            /* MH_CONTROL */
    bcmRxReasonTtl1,                    /* TTL_1 */
    bcmRxReasonL3Slowpath,              /* IP_OPTIONS_PKT. */
    bcmRxReasonE2eHolIbp,               /* E2E_HOL_IBP */
    bcmRxReasonClassTagPackets,         /* HG_HDR_TYPE1 */
    bcmRxReasonUrpfFail,                /* URPF_FAILED */
    bcmRxReasonNivUntagDrop,            /* NIV_UNTAG_DROP */
    bcmRxReasonNivTagDrop,              /* NIV_TAG_DROP */
    bcmRxReasonNivTagInvalid,           /* NIV_TAG_INVALID */
    bcmRxReasonNivRpfFail,              /* NIV_RPF_FAIL */
    bcmRxReasonNivInterfaceMiss,        /* NIV_INTERFACE_MISS */
    bcmRxReasonNivPrioDrop,             /* NIV_PRIO_DROP */
    bcmRxReasonTrillName,               /* NICKNAME_TABLE_COPYTOCPU */
    bcmRxReasonTrillTtl,                /* TRILL_HOP_COUNT_CHECK_FAIL */
    bcmRxReasonTrillCoreIsIs,           /* TRILL_CORE_IS_IS_PKT */
    bcmRxReasonTrillSlowpath,           /* TRILL_SLOWPATH */
    bcmRxReasonTrillRpfFail,            /* TRILL_RPF_CHECK_FAIL */
    bcmRxReasonTrillMiss,               /* TRILL_LOOKUP_MISS */
    bcmRxReasonTrillInvalid,            /* TRILL_HDR_ERROR */
    bcmRxReasonOAMLMDM,                 /* OAM_LMDM */
    bcmRxReasonWlanSlowpathKeepalive,   /* CAPWAP_KEEPALIVE */
    bcmRxReasonWlanTunnelError,         /* WLAN_SHIM_HEADER_ERROR_TO_CPU */
    bcmRxReasonWlanSlowpath,            /* WLAN_CAPWAP_SLOWPATH */
    bcmRxReasonWlanDot1xDrop,           /* WLAN_DOT1X_DROP */
    bcmRxReasonMplsReservedEntropyLabel, /* ENTROPY_LABEL_IN_UNALLOWED_RANGE */
    bcmRxReasonCongestionCnmProxy,      /* QCN_CNM_PRP */
    bcmRxReasonCongestionCnmProxyError, /* QCN_CNM_PRP_DLF */
    bcmRxReasonMplsUnknownAch,          /* MPLS_UNKNOWN_ACH_TYPE */
    bcmRxReasonMplsLookupsExceeded,     /* MPLS_OUT_OF_LOOKUPS */
    bcmRxReasonMplsIllegalReservedLabel, /* MPLS_ILLEGAL_RESERVED_LABEL */
    bcmRxReasonMplsRouterAlertLabel,    /* MPLS_ALERT_LABEL */
    bcmRxReasonParityError,             /* PARITY_ERROR */
    bcmRxReasonHigigHdrError,           /* HGHDR_ERROR */
    bcmRxReasonFilterMatch,             /* CPU_FFP */
    bcmRxReasonTimesyncUnknownVersion,  /* IEEE1588_UNKNOWN_VERSION */
    bcmRxReasonCongestionCnm,           /* ICNM */
    bcmRxReasonL2GreSipMiss,            /* L2GRE_SIP_MISS */
    bcmRxReasonL2GreVpnIdMiss,          /* L2GRE_VPNID_MISS */
};

static bcm_rx_reason_t *_bcm_tr3_cpu_cos_map_overlays[_bcm_tr3_rx_cpu_cosmap_key_overlays] = {
    _bcm_triumph3_ip_cpu_cos_map_key,
    _bcm_triumph3_ep_cpu_cos_map_key,
    _bcm_triumph3_nlf_cpu_cos_map_key,
};

static uint32 _bcm_tr3_cpu_cos_map_maxs[_bcm_tr3_rx_cpu_cosmap_key_overlays] = {
    _bcm_tr3_rx_ip_cpu_cosmap_key_max,
    _bcm_tr3_rx_ep_cpu_cosmap_key_max,
    _bcm_tr3_rx_nlf_cpu_cosmap_key_max,
};

#endif /* BCM_TRIUMPH3_SUPPORT */

#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
/*
 * Function:
 *      _bcm_esw_rcmr_overlay_get
 * Purpose:
 *      Get all supported reasons for overlayed CPU cosq mapping 
 * Parameters:
 *      unit - Unit reference
 *      reasons - cpu cosq "reasons" mapping bitmap
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_rcmr_overlay_get(int unit, bcm_rx_reasons_t * reasons)
{
    uint32          sz, *sz_overlays;
    uint32          ix, ovx;
    bcm_rx_reason_t *cpu_cos_map_key, **cpu_cos_map_overlays;

    if (SOC_IS_TRIUMPH3(unit)) {
        cpu_cos_map_overlays = _bcm_tr3_cpu_cos_map_overlays;
        sz_overlays = _bcm_tr3_cpu_cos_map_maxs;
    } else {
        /* If the feature check passes, we should have an implementation */
        return BCM_E_INTERNAL;
    }

    for (ovx = 0; ovx < _bcm_tr3_rx_cpu_cosmap_key_overlays; ovx++) {
        cpu_cos_map_key = cpu_cos_map_overlays[ovx];
        sz = sz_overlays[ovx];
        for (ix = 0; ix < sz; ix++) {
             if (cpu_cos_map_key[ix] != bcmRxReasonInvalid) {
                 BCM_RX_REASON_SET(*reasons, cpu_cos_map_key[ix]);
             }
        }
    }
    return BCM_E_NONE;
}

static soc_field_t _bcm_tr3_reason_fields[_bcm_tr3_rx_cpu_cosmap_key_words] = {
    REASONS_KEY_LOWf,
    REASONS_KEY_MIDf,
    REASONS_KEY_HIGHf,
};

static soc_field_t _bcm_tr3_mask_fields[_bcm_tr3_rx_cpu_cosmap_key_words] = {
    REASONS_MASK_LOWf,
    REASONS_MASK_MIDf,
    REASONS_MASK_HIGHf,
};

STATIC int
_bcm_tr3_rx_cosq_mapping_get(int unit, int index,
                            bcm_rx_reasons_t *reasons, 
                            bcm_rx_reasons_t *reasons_mask,
                            uint8 *prio, uint8 *prio_mask,
                            uint32 *packet_type, uint32 * packet_type_mask,
                            bcm_cos_queue_t *cosq)
{
    cpu_cos_map_entry_t entry; 
    uint32 key[_bcm_rx_cpu_cosmap_key_words_max];
    uint32 mask[_bcm_rx_cpu_cosmap_key_words_max];
    uint32 maskwordlen[_bcm_rx_cpu_cosmap_key_words_max];
    uint32 keywordlen[_bcm_rx_cpu_cosmap_key_words_max];
    uint32 ix, ovx, word, words, key_len, mask_len, cur_len;
    uint32 sw_pkt_type_mask, sw_pkt_type_key;
    uint32 reason_type_mask, reason_type_key;
    uint32 maskbitset, keybitset;
    soc_field_t *reason_fields, *mask_fields;
    bcm_rx_reason_t *cpu_cos_map_key, **cpu_cos_map_overlays;

    if (SOC_IS_TRIUMPH3(unit)) {
        cpu_cos_map_overlays = _bcm_tr3_cpu_cos_map_overlays;
        reason_fields = _bcm_tr3_reason_fields;
        mask_fields = _bcm_tr3_mask_fields;
        words = _bcm_tr3_rx_cpu_cosmap_key_words;
    } else {
        
        return BCM_E_INTERNAL;
    }

    /* Verify the index */
    if (index < soc_mem_index_min(unit, CPU_COS_MAPm) ||
        index > soc_mem_index_max(unit, CPU_COS_MAPm)) {
        return BCM_E_PARAM;
    }

    /* NULL pointer check */
    if (reasons == NULL  || reasons_mask == NULL  || 
        prio == NULL || prio_mask == NULL || 
        packet_type == NULL || packet_type_mask == NULL || 
        cosq == NULL) {
        return BCM_E_PARAM;
    }
 
    /* Read the entry */
    SOC_IF_ERROR_RETURN
        (READ_CPU_COS_MAPm(unit, MEM_BLOCK_ANY, index, &entry));

    /* Return BCM_E_NOT_FOUND if invalid */
    if (soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, VALIDf) == 0) {
        return (BCM_E_NOT_FOUND);
    }        

    sal_memset(reasons, 0, sizeof(bcm_rx_reasons_t));
    sal_memset(reasons_mask, 0, sizeof(bcm_rx_reasons_t));

    *cosq = (bcm_cos_queue_t)
              soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, COSf);
    
    sw_pkt_type_mask = soc_mem_field32_get(unit, CPU_COS_MAPm, 
                                           &entry, SW_PKT_TYPE_MASKf);
    sw_pkt_type_key  = soc_mem_field32_get(unit, CPU_COS_MAPm, 
                                           &entry, SW_PKT_TYPE_KEYf);

    if (sw_pkt_type_mask == 0 && sw_pkt_type_key == 0) {
        /* all packets matched */
        *packet_type_mask = 0;
        *packet_type = 0;
    } else if (sw_pkt_type_mask == 2 && sw_pkt_type_key == 0) {
        /* Only non-switched packets */
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
        *packet_type = 0;
    } else if (sw_pkt_type_mask == 2 && sw_pkt_type_key == 2) {
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
        *packet_type = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
    } else if (sw_pkt_type_mask == 3 && sw_pkt_type_key == 2) {
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED | 
                             BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST;
        *packet_type = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
    } else if (sw_pkt_type_mask == 3 && sw_pkt_type_key == 3) {
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED |
                             BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST;
        *packet_type = BCM_RX_COSQ_PACKET_TYPE_SWITCHED |
                        BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST;
    }
 
    if (soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, MIRR_PKT_MASKf)) {
        *packet_type_mask |= BCM_RX_COSQ_PACKET_TYPE_MIRROR;
    }

    if (soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, MIRR_PKT_KEYf)) {
        *packet_type |= BCM_RX_COSQ_PACKET_TYPE_MIRROR;
    }

    reason_type_mask = soc_mem_field32_get(unit, CPU_COS_MAPm, 
                                           &entry, REASON_CODE_TYPE_MASKf);
    reason_type_key  = soc_mem_field32_get(unit, CPU_COS_MAPm, 
                                           &entry, REASON_CODE_TYPE_KEYf);

    if ((0 != reason_type_mask) && 
        (_bcm_tr3_rx_cpu_cosmap_reason_type_mask != reason_type_mask)) {
        /* This should be set to exact match or all */
        return BCM_E_INTERNAL;
    }

    switch (reason_type_key) {
    case 0:
        ovx = 0; /* IP overlay */
        break;
    case 2:
        ovx = 1; /* EP overlay */
        break;
    case 3:
        ovx = 2; /* NLF overlay */
        break;
    default:
        /* Not a valid choice on current devices */
        return BCM_E_INTERNAL;
    }
    cpu_cos_map_key = cpu_cos_map_overlays[ovx];

    *prio_mask = 
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, INT_PRI_MASKf);
    *prio = 
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, INT_PRI_KEYf);

    mask_len = key_len = 0;
    for (word = 0; word < words; word++) {
        mask[word] = 
            soc_mem_field32_get(unit, CPU_COS_MAPm, &entry,
                                mask_fields[word]);
        maskwordlen[word] =
            soc_mem_field_length(unit, CPU_COS_MAPm, mask_fields[word]);
        mask_len += maskwordlen[word];

        key[word] = 
            soc_mem_field32_get(unit, CPU_COS_MAPm, &entry,
                                reason_fields[word]);
        keywordlen[word] =
            soc_mem_field_length(unit, CPU_COS_MAPm, reason_fields[word]);
        key_len += keywordlen[word];

        if (keywordlen[word] != maskwordlen[word]) {
            
            return BCM_E_INTERNAL;
        }
    }

    if (key_len != mask_len) {
        
        return BCM_E_INTERNAL;
    }

    cur_len = word = 0;
    for (ix = 0; ix < key_len; ix++) {
        if (ix == (cur_len + keywordlen[word])) {
            /* Start of new word, advance values */
            cur_len += keywordlen[word];
            word++;
        }

        keybitset = key[word] & (1 << (ix - cur_len));
        maskbitset = mask[word] & (1 << (ix - cur_len));

        if (maskbitset) {
            BCM_RX_REASON_SET(*reasons_mask, cpu_cos_map_key[ix]);
        }
        if (keybitset) {
            BCM_RX_REASON_SET(*reasons, cpu_cos_map_key[ix]);
        }
    }

    return BCM_E_NONE;
}

int
_bcm_tr3_rx_cosq_mapping_set(int unit, int index,
                            bcm_rx_reasons_t reasons, bcm_rx_reasons_t
                            reasons_mask,
                            uint8 int_prio, uint8 int_prio_mask,
                            uint32 packet_type, uint32 packet_type_mask,
                            bcm_cos_queue_t cosq)
{
    bcm_rx_reason_t  ridx;
    bcm_rx_reasons_t reasons_remain;
    uint32 key[_bcm_tr3_rx_cpu_cosmap_key_words];
    uint32 mask[_bcm_tr3_rx_cpu_cosmap_key_words];
    uint32 maskwordlen[_bcm_tr3_rx_cpu_cosmap_key_words];
    uint32 keywordlen[_bcm_tr3_rx_cpu_cosmap_key_words];
    uint32 word, words, key_len, mask_len, cur_len;
    int32 ovx, ovx_max;
    int reason_set;
    cpu_cos_map_entry_t entry; 
    uint8 sw_pkt_type_key = 0;
    uint8 sw_pkt_type_mask = 0;
    uint32 reason_type_mask = 0;
    uint32 reason_type_key = 0;
    uint32 *sz_overlays, bit;
    soc_field_t *reason_fields, *mask_fields;
    bcm_rx_reason_t *cpu_cos_map_key, **cpu_cos_map_overlays;

    if (SOC_IS_TRIUMPH3(unit)) {
        cpu_cos_map_overlays = _bcm_tr3_cpu_cos_map_overlays;
        sz_overlays = _bcm_tr3_cpu_cos_map_maxs;
        reason_fields = _bcm_tr3_reason_fields;
        mask_fields = _bcm_tr3_mask_fields;
        words = _bcm_tr3_rx_cpu_cosmap_key_words;
        ovx_max = _bcm_tr3_rx_cpu_cosmap_key_overlays - 1;
    } else {
        
        return BCM_E_INTERNAL;
    }

    /* Verify the index */
    if (index < soc_mem_index_min(unit, CPU_COS_MAPm) ||
        index > soc_mem_index_max(unit, CPU_COS_MAPm)) {
        return BCM_E_PARAM;
    }

    /* Verify the cosq */
    if (SOC_IS_TRIUMPH3(unit) &&
        soc_feature(unit, soc_feature_cmic_reserved_queues) && 
        cosq >= _bcm_triumph3_rx_cpu_max_cosq) {
        return BCM_E_PARAM;
    }

    if (SOC_IS_HELIX4(unit) && 
        cosq >= _bcm_trident2_rx_cpu_max_cosq) {
        return BCM_E_PARAM;
    }

    /* Verify the packet type */

    if (packet_type & BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST) {
        sw_pkt_type_key |= 1;
    }
    if (packet_type & BCM_RX_COSQ_PACKET_TYPE_SWITCHED) {
        sw_pkt_type_key |= 2;
    }
    if (packet_type_mask & BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST) {
        sw_pkt_type_mask |= 1;
    }
    if (packet_type_mask & BCM_RX_COSQ_PACKET_TYPE_SWITCHED) {
        sw_pkt_type_mask |= 2;
    }
    sw_pkt_type_key &= sw_pkt_type_mask;

    if ((sw_pkt_type_mask == 0x1) || 
        ((sw_pkt_type_mask != 0) && (sw_pkt_type_key == 0x1))) { 
        /* Hw doesn't support these cases */
        return BCM_E_PARAM;
    }

    reasons_remain = reasons_mask;

    mask_len = key_len = 0;
    for (word = 0; word < words; word++) {
        mask[word] = 0;
        maskwordlen[word] =
            soc_mem_field_length(unit, CPU_COS_MAPm, mask_fields[word]);
        mask_len += maskwordlen[word];

        key[word] = 0;
        keywordlen[word] =
            soc_mem_field_length(unit, CPU_COS_MAPm, reason_fields[word]);
        key_len += keywordlen[word];

        if (keywordlen[word] != maskwordlen[word]) {
            
            return BCM_E_INTERNAL;
        }
    }

    if (key_len != mask_len) {
        
        return BCM_E_INTERNAL;
    }

    bit = 0;
    reason_set = FALSE;
    for (ovx = ovx_max; ovx >= 0; ovx--) {
        if (reason_set) {
            if (ovx > 0) {
                continue;
            } else {
                bit = _bcm_tr3_cpu_cos_map_maxs[1];
            }            
        } else {
            bit = 0;
        }

        cpu_cos_map_key = cpu_cos_map_overlays[ovx];

        /* Start this loop where left off from previous */
        for (; bit < sz_overlays[ovx]; bit++) {
            /* Find reason being set */
            ridx  = cpu_cos_map_key[bit];
            if (!BCM_RX_REASON_GET(reasons_mask, ridx)) {
                continue;
            }

            cur_len = 0;
            for (word = 0; word < words; word++) {
                if (bit < (cur_len + keywordlen[word])) {
                    break;
                } else {
                    cur_len += keywordlen[word];
                }
            }
            if (word == words) {
                
                return BCM_E_INTERNAL;
            }

            mask[word] |=  1 << (bit - cur_len);
            if (BCM_RX_REASON_GET(reasons, ridx)) {
                key[word] |=  1 << (bit - cur_len);
            }

            /* clean the bit of reasons_remain */
            BCM_RX_REASON_CLEAR(reasons_remain, ridx);

            if (!reason_set) {
                reason_set = TRUE;
                switch (ovx) {
                case 0:
                    if (bit < _bcm_tr3_cpu_cos_map_maxs[1]) {
                        /* First bit in the overlayed range, must be IP */
                        reason_type_mask = 0x3;
                    } else {
                        /* First bit in the common range, no restriction */
                        reason_type_mask = 0;
                    }
                    reason_type_key = 0; /* IP overlay */
                    break;
                case 1:
                    reason_type_mask = 0x3;
                    reason_type_key = 2; /* EP overlay */
                    break;
                case 2:
                    reason_type_mask = 0x3;
                    reason_type_key = 3; /* NLF overlay */
                    break;
                default:
                    return BCM_E_INTERNAL;
                }
            }
        }
    }

    /* check whether there are reasons unsupported or unmappped */
    for (ridx = bcmRxReasonInvalid; ridx < bcmRxReasonCount; ridx++) {
        if (BCM_RX_REASON_GET(reasons_remain, ridx)) {
            return BCM_E_PARAM;
        }
    }

    /* Now zero entry */
    sal_memset (&entry, 0, sizeof(cpu_cos_map_entry_t));

    /* Program the key and mask */
    for (word = 0; word < words; word++) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            reason_fields[word], key[word]);   

        /* program the mask field */
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            mask_fields[word], mask[word]);   
    }

    /* Program the reasons type */ 
    if (0 != reason_type_mask) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            REASON_CODE_TYPE_MASKf, reason_type_mask);   
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            REASON_CODE_TYPE_KEYf, reason_type_key);   
    }

    /* Program the packet type */ 
    if (packet_type_mask & (BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST |
                         BCM_RX_COSQ_PACKET_TYPE_SWITCHED)) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            SW_PKT_TYPE_MASKf, sw_pkt_type_mask);   
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            SW_PKT_TYPE_KEYf, sw_pkt_type_key);   
    }

    if (packet_type_mask & BCM_RX_COSQ_PACKET_TYPE_MIRROR) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            MIRR_PKT_MASKf, 1);   
        if (packet_type & BCM_RX_COSQ_PACKET_TYPE_MIRROR) {
            soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                                MIRR_PKT_KEYf, 1);   
        }
    }

    /* Handle priority when int_prio_mask != 0 */
    if (int_prio_mask) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            INT_PRI_KEYf, (int_prio & 0xf));   
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            INT_PRI_MASKf, (int_prio_mask & 0xf));   
    }

    soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, COSf, cosq);
    soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, VALIDf, 1);

    /* write to memory */
    SOC_IF_ERROR_RETURN(WRITE_CPU_COS_MAPm(unit, MEM_BLOCK_ANY, 
                                           index, &entry));

    return BCM_E_NONE;
}

#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRX_SUPPORT
/* Each device has 1 cos map key table and zero or more encoding tables */
STATIC int
_bcm_rx_reason_table_get(int unit,
                         bcm_rx_reason_t **cpu_cos_map_key,
                         int *key_table_len,
                         bcm_rx_reason_t **cpu_opcode_encoding,
                         int *encoding_table_len,
                         int *encoding_table_count)
{
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        cpu_opcode_encoding[0] = _bcm_niv_cpu_opcode_encoding;
        encoding_table_len[0] = sizeof(_bcm_niv_cpu_opcode_encoding) /
            sizeof(_bcm_niv_cpu_opcode_encoding[0]);
        cpu_opcode_encoding[1] = _bcm_trill_cpu_opcode_encoding;
        encoding_table_len[1] = sizeof(_bcm_trill_cpu_opcode_encoding) /
            sizeof(_bcm_trill_cpu_opcode_encoding[0]);
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            cpu_opcode_encoding[2] = _bcm_nat_cpu_opcode_encoding;
            encoding_table_len[2] = sizeof(_bcm_nat_cpu_opcode_encoding) /
                sizeof(_bcm_nat_cpu_opcode_encoding[0]);
            *encoding_table_count = 3;
            *cpu_cos_map_key = _bcm_trident2_cpu_cos_map_key;
            *key_table_len = sizeof(_bcm_trident2_cpu_cos_map_key) /
                sizeof(_bcm_trident2_cpu_cos_map_key[0]);
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        {
            *encoding_table_count = 2;
            *cpu_cos_map_key = _bcm_trident_cpu_cos_map_key;
            *key_table_len = sizeof(_bcm_trident_cpu_cos_map_key) /
                sizeof(_bcm_trident_cpu_cos_map_key[0]);
        }
        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
        cpu_opcode_encoding[0] = _bcm_bfd_cpu_opcode_encoding;
        encoding_table_len[0] = sizeof(_bcm_bfd_cpu_opcode_encoding) /
            sizeof(_bcm_bfd_cpu_opcode_encoding[0]);
        *encoding_table_count = 1;
        *cpu_cos_map_key = _bcm_katana_cpu_cos_map_key;
        *key_table_len = sizeof(_bcm_katana_cpu_cos_map_key) /
            sizeof(_bcm_katana_cpu_cos_map_key[0]);
        return BCM_E_NONE;
    }
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
        *encoding_table_count = 0;
        *cpu_cos_map_key = _bcm_enduro_cpu_cos_map_key;
        *key_table_len = sizeof(_bcm_enduro_cpu_cos_map_key) /
            sizeof(_bcm_enduro_cpu_cos_map_key[0]);
        return BCM_E_NONE;
    }
#endif /* BCM_ENDURO_SUPPORT */


    *encoding_table_count = 0;
    *cpu_cos_map_key = _bcm_trx_cpu_cos_map_key;
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit)) {
        *key_table_len = _bcm_triumph2_rx_cpu_cosmap_key_max;
    } else if (SOC_IS_TR_VL(unit)) {
        *key_table_len = _bcm_triumph_rx_cpu_cosmap_key_max;
    } else if (SOC_IS_SC_CQ(unit)) {
        *key_table_len = _bcm_scorpion_rx_cpu_cosmap_key_max;
    } else {
        return BCM_E_UNAVAIL;
    }
    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      bcm_rx_cosq_mapping_reasons_get
 * Purpose:
 *      Get all supported reasons for CPU cosq mapping  
 * Parameters:
 *      unit - Unit reference
 *      reasons - cpu cosq "reasons" mapping bitmap
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_rx_cosq_mapping_reasons_get(int unit, bcm_rx_reasons_t * reasons)
{
#ifdef BCM_TRX_SUPPORT
    bcm_rx_reason_t *cpu_cos_map_key, *cpu_opcode_enc[3];
    int key_table_len, enc_table_len[3], enc_count;
    int key_idx, table_idx, enc_idx;

    if (reasons == NULL) {
        return BCM_E_PARAM;
    }

    BCM_RX_REASON_CLEAR_ALL(*reasons);

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_rx_reason_overlay)) {
        return _bcm_esw_rcmr_overlay_get(unit, reasons);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    if (SOC_IS_TRX(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_rx_reason_table_get(unit, &cpu_cos_map_key, &key_table_len,
                                      cpu_opcode_enc, enc_table_len,
                                      &enc_count));

        for (key_idx = 0; key_idx < key_table_len; key_idx++) {
            if (cpu_cos_map_key[key_idx] != bcmRxReasonInvalid) {
                BCM_RX_REASON_SET(*reasons, cpu_cos_map_key[key_idx]);
            }
        }
        for (table_idx = 0; table_idx < enc_count; table_idx++) {
            for (enc_idx = 0; enc_idx < enc_table_len[table_idx]; enc_idx++) {
                if (cpu_opcode_enc[table_idx][enc_idx] != bcmRxReasonInvalid) {
                    BCM_RX_REASON_SET(*reasons,
                                      cpu_opcode_enc[table_idx][enc_idx]);
                }
            }
        }

        return BCM_E_NONE;
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_rx_reasons_get (int unit, bcm_rx_reasons_t *reasons)
{
    return (_bcm_common_rx_reasons_get(unit, reasons));
}

static soc_reg_t _rx_redirect_reg[3][2] = {
    { CMIC_PKT_REASONr, CMIC_PKT_REASON_HIr },
    { CMIC_PKT_REASON_DIRECTr, CMIC_PKT_REASON_DIRECT_HIr },
    { CMIC_PKT_REASON_MINIr, CMIC_PKT_REASON_MINI_HIr }
};

#ifdef BCM_CMICM_SUPPORT
static soc_reg_t _cmicm_rx_redirect_reg[3][2] = {
    {CMIC_PKT_REASON_0_TYPEr, CMIC_PKT_REASON_1_TYPEr},
    {CMIC_PKT_REASON_DIRECT_0_TYPEr, CMIC_PKT_REASON_DIRECT_1_TYPEr},
    {CMIC_PKT_REASON_MINI_0_TYPEr, CMIC_PKT_REASON_MINI_0_TYPEr}
};
#endif

int
bcm_esw_rx_redirect_reasons_set(int unit, bcm_rx_redirect_t mode, bcm_rx_reasons_t reasons)
{
    uint32 ix, i, max_index = 32;
    uint32 addr, rval, rval_hi = 0;
    soc_rx_reason_t *map;
    uint8 set = 0;
    soc_reg_t cmic_reg;

    if (!SOC_UNIT_VALID(unit)) { 
        return BCM_E_UNIT;
    }
    if ((mode < 0) || (mode > (int)_SHR_RX_REDIRECT_MAX)) {
        return SOC_E_PARAM;
    }
    if ((map = (SOC_DCB_RX_REASON_MAPS(unit))[0]) == NULL) {
        return SOC_E_INTERNAL;
    }

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        addr = soc_reg_addr(unit, _cmicm_rx_redirect_reg[mode][0], REG_PORT_ANY, 0);
    } else
#endif
    {
        addr = soc_reg_addr(unit, _rx_redirect_reg[mode][0], REG_PORT_ANY, 0);
    }
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, addr, &rval));
    if (soc_feature(unit, soc_feature_dcb_reason_hi)) {
        max_index = 64;
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm)) {
            addr = soc_reg_addr(unit, _cmicm_rx_redirect_reg[mode][1], REG_PORT_ANY, 0);
        } else
#endif
        {
            addr = soc_reg_addr(unit, _rx_redirect_reg[mode][1], REG_PORT_ANY, 0);
        }
        SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, addr, &rval_hi));
    }
    
    for (ix = 0; ix < max_index ; ix++) {
        if (map[ix] != socRxReasonInvalid && 
               map[ix] != socRxReasonCount) {
            if (BCM_RX_REASON_GET(reasons, (bcm_rx_reason_t)map[ix])) {
                set++;
                if (ix < 32) {
                    rval |= (1 << ix);
                } else {
                    rval_hi |= (1 << (ix-32));
                }
            } else {
                set++;
                if (ix < 32) {
                    rval &= ~(1 << ix);
                } else {
                    rval_hi &= ~(1 << (ix-32));
                }
            }
        }
    }
    
    if (set) {
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm)) {
            cmic_reg = _cmicm_rx_redirect_reg[mode][0];
        } else
#endif
        {
            cmic_reg = _rx_redirect_reg[mode][0];
        }
        for (i = 0; i < SOC_REG_NUMELS(unit, cmic_reg); i++) {
             addr = soc_reg_addr(unit, cmic_reg, REG_PORT_ANY, i);
             SOC_IF_ERROR_RETURN(soc_pci_write(unit, addr, rval));
        }
        if (soc_feature(unit, soc_feature_dcb_reason_hi)) {
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm)) {
                cmic_reg = _cmicm_rx_redirect_reg[mode][1];
            } else
#endif
            {
                cmic_reg = _rx_redirect_reg[mode][1];
            }
            for (i = 0; i < SOC_REG_NUMELS(unit, cmic_reg); i++) {
                addr = soc_reg_addr(unit, cmic_reg, REG_PORT_ANY, i);
                SOC_IF_ERROR_RETURN(soc_pci_write(unit, addr, rval_hi));
            }
        }
    }
    return SOC_E_NONE;
}

int
bcm_esw_rx_redirect_reasons_get(int unit, 
                                bcm_rx_redirect_t mode, 
                                bcm_rx_reasons_t *reasons)
{
    uint32 ix, max_index = 32;
    uint32 addr, rval;
    uint32 rval_hi = 0;
    soc_rx_reason_t *map;

    if (!SOC_UNIT_VALID(unit)) { 
        return BCM_E_UNIT;
    }
    if ((mode < 0) || (mode > (int)_SHR_RX_REDIRECT_MAX)) {
        return SOC_E_PARAM;
    }
    if ((map = (SOC_DCB_RX_REASON_MAPS(unit))[0]) == NULL) {
        return SOC_E_INTERNAL;
    }

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        addr = soc_reg_addr(unit, _cmicm_rx_redirect_reg[mode][0], REG_PORT_ANY, 0);
    } else
#endif
    {
        addr = soc_reg_addr(unit, _rx_redirect_reg[mode][0], REG_PORT_ANY, 0);
    }
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, addr, &rval));
    if (soc_feature(unit, soc_feature_dcb_reason_hi)) {
        max_index = 64;
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm)) {
            addr = soc_reg_addr(unit, _cmicm_rx_redirect_reg[mode][1], REG_PORT_ANY, 0);
        } else
#endif
        {
            addr = soc_reg_addr(unit, _rx_redirect_reg[mode][1], REG_PORT_ANY, 0);
        }
        SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, addr, &rval_hi));
    }
    BCM_RX_REASON_CLEAR_ALL(*reasons);
    for (ix = 0; ix < max_index ; ix++) {
        if (map[ix] != socRxReasonInvalid && 
               map[ix] != socRxReasonCount) {
            if (ix < 32) {
                if (rval & (1 << ix)) {
                    BCM_RX_REASON_SET(*reasons, (bcm_rx_reason_t)map[ix]);
                }
            } else {
                if (rval_hi & (1 << (ix - 32))) {
                    BCM_RX_REASON_SET(*reasons, (bcm_rx_reason_t)map[ix]);
                }
            }
        }
    }
    return SOC_E_NONE;
}

#ifdef BCM_TRX_SUPPORT
/* Delete an entry of CPU_COS_MAPm */
int
_bcm_trx_rx_cosq_mapping_delete(int unit, int index)
{
    cpu_cos_map_entry_t entry; 

    if (index < soc_mem_index_min(unit, CPU_COS_MAPm) ||
        index > soc_mem_index_max(unit, CPU_COS_MAPm)) {
        return BCM_E_PARAM;
    }

    /* Now zero entry */
    sal_memset (&entry, 0, sizeof(cpu_cos_map_entry_t));

    soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, VALIDf, 0);

    /* write to memory */
    SOC_IF_ERROR_RETURN(WRITE_CPU_COS_MAPm(unit, MEM_BLOCK_ANY, 
                            index, &entry));
    return BCM_E_NONE;
}

/* Get the rx cosq mapping */
int
_bcm_trx_rx_cosq_mapping_get(int unit, int index,
                            bcm_rx_reasons_t *reasons, 
                            bcm_rx_reasons_t *reasons_mask,
                            uint8 *prio, uint8 *prio_mask,
                            uint32 *packet_type, uint32 * packet_type_mask,
                            bcm_cos_queue_t *cosq)
{
    bcm_rx_reason_t *cpu_cos_map_key, *cpu_opcode_enc[3];
    int key_table_len, enc_table_len[3], enc_count;
    int word_idx, bit_idx, key_idx, table_idx, word_count, bit_count;
    uint32 mask[3], key[3], enc_mask[3], enc_key[3], field_size[3];
    bcm_rx_reason_t base_enc;
    cpu_cos_map_entry_t entry; 
    uint32 sw_pkt_type_mask, sw_pkt_type_key;
    int enc_bit = 0;

    BCM_IF_ERROR_RETURN
        (_bcm_rx_reason_table_get(unit, &cpu_cos_map_key, &key_table_len,
                                  cpu_opcode_enc, enc_table_len, &enc_count));

    /* Verify the index */
    if (index < soc_mem_index_min(unit, CPU_COS_MAPm) ||
        index > soc_mem_index_max(unit, CPU_COS_MAPm)) {
        return BCM_E_PARAM;
    }

    /* NULL pointer check */
    if (reasons == NULL  || reasons_mask == NULL  || 
        prio == NULL || prio_mask == NULL || 
        packet_type == NULL || packet_type_mask == NULL || 
        cosq == NULL) {
        return BCM_E_PARAM;
    }
 
    /* Read the entry */
    SOC_IF_ERROR_RETURN(READ_CPU_COS_MAPm(unit, MEM_BLOCK_ANY, index, &entry));

    /* Return BCM_E_NOT_FOUND if invalid */
    if (soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, VALIDf) == 0) {
        return (BCM_E_NOT_FOUND);
    }        

    sal_memset(reasons, 0, sizeof(bcm_rx_reasons_t));
    sal_memset(reasons_mask, 0, sizeof(bcm_rx_reasons_t));

    *cosq = (bcm_cos_queue_t)
              soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, COSf);
    
    sw_pkt_type_mask = soc_mem_field32_get(unit, CPU_COS_MAPm, 
                                           &entry, SW_PKT_TYPE_MASKf);
    sw_pkt_type_key  = soc_mem_field32_get(unit, CPU_COS_MAPm, 
                                           &entry, SW_PKT_TYPE_KEYf);

    if (sw_pkt_type_mask == 0 && sw_pkt_type_key == 0) {
        /* all packets matched */
        *packet_type_mask = 0;
        *packet_type = 0;
    } else if (sw_pkt_type_mask == 2 && sw_pkt_type_key == 0) {
        /* Only non-switched packets */
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
        *packet_type = 0;
    } else if (sw_pkt_type_mask == 2 && sw_pkt_type_key == 2) {
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
        *packet_type = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
    } else if (sw_pkt_type_mask == 3 && sw_pkt_type_key == 2) {
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED | 
                             BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST;
        *packet_type = BCM_RX_COSQ_PACKET_TYPE_SWITCHED;
    } else if (sw_pkt_type_mask == 3 && sw_pkt_type_key == 3) {
        *packet_type_mask = BCM_RX_COSQ_PACKET_TYPE_SWITCHED |
                             BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST;
        *packet_type = BCM_RX_COSQ_PACKET_TYPE_SWITCHED |
                        BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST;
    }
 
    if (soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, MIRR_PKT_MASKf)) {
        *packet_type_mask |= BCM_RX_COSQ_PACKET_TYPE_MIRROR;
    }

    if (soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, MIRR_PKT_KEYf)) {
        *packet_type |= BCM_RX_COSQ_PACKET_TYPE_MIRROR;
    }

    *prio_mask = 
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, INT_PRI_MASKf);
    *prio = 
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, INT_PRI_KEYf);

    mask[0] =
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, REASONS_MASK_LOWf);
    key[0] =
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, REASONS_KEY_LOWf);
    field_size[0] =
        soc_mem_field_length(unit, CPU_COS_MAPm, REASONS_KEY_LOWf);
    word_count = 1;
    if (soc_mem_field_valid(unit, CPU_COS_MAPm, REASONS_MASK_MIDf)) {
        mask[word_count] =
            soc_mem_field32_get(unit, CPU_COS_MAPm,
                                &entry, REASONS_MASK_MIDf);
        key[word_count] =
            soc_mem_field32_get(unit, CPU_COS_MAPm,
                                &entry, REASONS_KEY_MIDf);
        field_size[word_count] =
            soc_mem_field_length(unit, CPU_COS_MAPm, REASONS_KEY_MIDf);
        word_count++;
    }
    mask[word_count] =
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, REASONS_MASK_HIGHf);
    key[word_count] =
        soc_mem_field32_get(unit, CPU_COS_MAPm, &entry, REASONS_KEY_HIGHf);
    field_size[word_count] =
        soc_mem_field_length(unit, CPU_COS_MAPm, REASONS_KEY_HIGHf);
    word_count++;

    sal_memset(enc_mask, 0, sizeof(enc_mask));
    sal_memset(enc_key, 0, sizeof(enc_key));
    for (word_idx = 0; word_idx < word_count; word_idx++) {
        bit_count = field_size[word_idx];

        for (bit_idx = 0; bit_idx < bit_count; bit_idx++) {
            uint32 maskbitset;
            uint32 keybitset;

            maskbitset = mask[word_idx] & (1 << bit_idx);
            keybitset = key[word_idx] & (1 << bit_idx);
            key_idx = word_idx * 32 + bit_idx;
            if (key_idx >= key_table_len) {
                break;
            }

            for (table_idx = 0; table_idx < enc_count; table_idx++) {
                base_enc = cpu_opcode_enc[table_idx][0];
                if (cpu_cos_map_key[key_idx] != base_enc) {
                    continue;
                }
                if (key_idx == 0 || cpu_cos_map_key[key_idx - 1] != base_enc) {
                    enc_bit = 0;
                }
                if (maskbitset) { /* should always be set */
                    enc_mask[table_idx] |= (1 << enc_bit);
                }
                if (keybitset) {
                    enc_key[table_idx] |= (1 << enc_bit);
                }
                enc_bit++;
                break;
            }
            if (table_idx == enc_count) { /* not in any encode table */
                if (maskbitset) {
                    BCM_RX_REASON_SET(*reasons_mask, cpu_cos_map_key[key_idx]);
                }
                if (keybitset) {
                    BCM_RX_REASON_SET(*reasons, cpu_cos_map_key[key_idx]);
                }
            }
        }
    }

    for (table_idx = 0; table_idx < enc_count; table_idx++) {
        if (enc_mask[table_idx] == 0 && enc_key[table_idx] == 0) {
            continue;
        }
        BCM_RX_REASON_SET(*reasons_mask, cpu_opcode_enc[table_idx][0]);
        BCM_RX_REASON_SET(*reasons,
                          cpu_opcode_enc[table_idx][enc_key[table_idx]]);
    }
             
    return BCM_E_NONE;
}

/* Set the rx cosq mapping */
int
_bcm_trx_rx_cosq_mapping_set(int unit, int index,
                            bcm_rx_reasons_t reasons, bcm_rx_reasons_t
                            reasons_mask,
                            uint8 int_prio, uint8 int_prio_mask,
                            uint32 packet_type, uint32 packet_type_mask,
                            bcm_cos_queue_t cosq)
{
    bcm_rx_reason_t *cpu_cos_map_key, *cpu_opcode_enc[3];
    int key_table_len, enc_table_len[3], enc_count;
    int word_idx, bit_idx, key_idx, table_idx, enc_idx, word_count;
    uint32 mask[3], key[3], enc_key, field_size[3];
    bcm_rx_reason_t  reason_enum, enc_enum;
    bcm_rx_reasons_t reasons_remain;
    cpu_cos_map_entry_t entry; 
    uint8 sw_pkt_type_key = 0;
    uint8 sw_pkt_type_mask = 0;
    
    BCM_IF_ERROR_RETURN
        (_bcm_rx_reason_table_get(unit, &cpu_cos_map_key, &key_table_len,
                                  cpu_opcode_enc, enc_table_len, &enc_count));

    /* Verify COSQ */
    if (SOC_IS_TD_TT(unit)) {
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            if (cosq >= _bcm_trident2_rx_cpu_max_cosq) {
                return BCM_E_PARAM;
            }
        }
        if (cosq >= _bcm_triumph_rx_cpu_max_cosq) {
            return BCM_E_PARAM;
        }
#endif /* BCM_TRIDENT_SUPPORT */
    } else if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
               SOC_IS_VALKYRIE2(unit)) {
        if (cosq >= _bcm_triumph_rx_cpu_max_cosq) {
            return BCM_E_PARAM;
        }
#ifdef BCM_KATANA_SUPPORT
    } else if (SOC_IS_KATANAX(unit)) {
        if (SOC_IS_KATANA2(unit)) {
            if (cosq >= _bcm_trident2_rx_cpu_max_cosq) {
                return BCM_E_PARAM;
            }
        }
        if (cosq >= _bcm_triumph_rx_cpu_max_cosq) {
            return BCM_E_PARAM;
        }
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_ENDURO_SUPPORT
    } else if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
#if defined(BCM_HURRICANE_SUPPORT)
        if (SOC_IS_HURRICANEX(unit)) {
            if (cosq >= _bcm_hurricane_rx_cpu_max_cosq) {
                return BCM_E_PARAM;
            }            
        } else
#endif /* BCM_HURRICANE_SUPPORT */
        if (cosq >= _bcm_triumph_rx_cpu_max_cosq) {
            return BCM_E_PARAM;
        }
#endif /* BCM_ENDURO_SUPPORT */
    } else if (SOC_IS_TR_VL(unit)) {
        if (cosq >= _bcm_triumph_rx_cpu_max_cosq) {
            return BCM_E_PARAM;
        }
    } else if (SOC_IS_SC_CQ(unit)) {
        if (cosq >= _bcm_scorpion_rx_cpu_max_cosq) {
            return BCM_E_PARAM;
        }
    } else {
        return BCM_E_UNAVAIL;
    }

    /* Verify the index */
    if (index < soc_mem_index_min(unit, CPU_COS_MAPm) ||
        index > soc_mem_index_max(unit, CPU_COS_MAPm)) {
        return BCM_E_PARAM;
    }

    /* Verify the packet type */

    if (packet_type & BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST) {
        sw_pkt_type_key |= 1;
    }
    if (packet_type & BCM_RX_COSQ_PACKET_TYPE_SWITCHED) {
        sw_pkt_type_key |= 2;
    }
    if (packet_type_mask & BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST) {
        sw_pkt_type_mask |= 1;
    }
    if (packet_type_mask & BCM_RX_COSQ_PACKET_TYPE_SWITCHED) {
        sw_pkt_type_mask |= 2;
    }
    sw_pkt_type_key &= sw_pkt_type_mask;

    if ((sw_pkt_type_mask == 0x1) || 
        ((sw_pkt_type_mask != 0) && (sw_pkt_type_key == 0x1))) { 
        /* Hw doesn't support these cases */
        return BCM_E_PARAM;
    }

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
       if (BCM_RX_REASON_GET(reasons, bcmRxReasonMplsCtrlWordError)){
          BCM_RX_REASON_CLEAR(reasons,bcmRxReasonMplsCtrlWordError);
          BCM_RX_REASON_CLEAR(reasons_mask,bcmRxReasonMplsCtrlWordError);
          BCM_RX_REASON_SET(reasons,bcmRxReasonBfd);
          BCM_RX_REASON_SET(reasons_mask,bcmRxReasonBfd);
       }
       if (BCM_RX_REASON_GET(reasons, bcmRxReasonBfdLookupFailure)) {
           BCM_RX_REASON_CLEAR(reasons, bcmRxReasonBfdLookupFailure);
           BCM_RX_REASON_SET(reasons, bcmRxReasonBfdInvalidVersion);
       }
       if (BCM_RX_REASON_GET(reasons_mask, bcmRxReasonBfdLookupFailure)) {
           BCM_RX_REASON_CLEAR(reasons_mask, bcmRxReasonBfdLookupFailure);
           BCM_RX_REASON_SET(reasons_mask, bcmRxReasonBfdInvalidVersion);
       }
    }

#endif /* BCM_KATANA_SUPPORT */

    field_size[0] =
        soc_mem_field_length(unit, CPU_COS_MAPm, REASONS_KEY_LOWf);
    word_count = 1;
    if (soc_mem_field_valid(unit, CPU_COS_MAPm, REASONS_MASK_MIDf)) {
        field_size[word_count] =
            soc_mem_field_length(unit, CPU_COS_MAPm, REASONS_KEY_MIDf);
        word_count++;
    }
    field_size[word_count] =
        soc_mem_field_length(unit, CPU_COS_MAPm, REASONS_KEY_HIGHf);
    word_count++;

    reasons_remain = reasons_mask;
    enc_enum = -1;
    enc_key = 0;
    word_idx = 0;
    bit_idx = 0;
    sal_memset(mask, 0, sizeof(mask));
    sal_memset(key, 0, sizeof(key));
    for (key_idx = 0; key_idx < key_table_len; key_idx++, bit_idx++) {
        if (bit_idx == field_size[word_idx]) {
            bit_idx = 0;
            word_idx++;
            if (word_idx == word_count) {
                /* Trap implementation error */
                return BCM_E_INTERNAL;
            }
        }
 
        reason_enum = cpu_cos_map_key[key_idx];
        if (reason_enum == enc_enum) { /* following bit of encoding */
            mask[word_idx] |= 1 << bit_idx;
            key[word_idx] |= (enc_key & 1) << bit_idx;
            enc_key >>= 1;
            continue;
        }

        /* Find reason being set */
        if (!BCM_RX_REASON_GET(reasons_mask, reason_enum)) {
            continue;
        }
        BCM_RX_REASON_CLEAR(reasons_remain, reason_enum);

        for (table_idx = 0; table_idx < enc_count; table_idx++) {
            if (reason_enum != cpu_opcode_enc[table_idx][0]) {
                continue;
            }

            enc_enum = reason_enum;
            for (enc_idx = 1; enc_idx < enc_table_len[table_idx]; enc_idx++) {
                reason_enum = cpu_opcode_enc[table_idx][enc_idx];
                if (!BCM_RX_REASON_GET(reasons, reason_enum)) {
                    BCM_RX_REASON_CLEAR(reasons_remain, reason_enum);
                    continue;
                } else if (enc_key != 0) { /* multiple reasons supplied */
                    return BCM_E_PARAM;
                }
                BCM_RX_REASON_CLEAR(reasons_remain, reason_enum);
                enc_key = enc_idx;
            }
            mask[word_idx] |= 1 << bit_idx;
            key[word_idx] |= (enc_key & 1) << bit_idx;
            enc_key >>= 1;
            break;
        }
        if (table_idx == enc_count) {
            mask[word_idx] |= 1 << bit_idx;
            if (BCM_RX_REASON_GET(reasons, reason_enum)) {
                key[word_idx] |= 1 << bit_idx;
            }
        }
    }

    /* check whether there are reasons unsupported */
    if (!BCM_RX_REASON_IS_NULL(reasons_remain)) {
        return BCM_E_PARAM;
    }

    /* Now zero entry */
    sal_memset (&entry, 0, sizeof(cpu_cos_map_entry_t));

    /* Program the key and mask when reasons_mask is not NULL */
    if (!BCM_RX_REASON_IS_NULL(reasons_mask)) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, REASONS_MASK_LOWf,
                            mask[0]);   
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, REASONS_KEY_LOWf,
                            key[0]);
        word_count = 1;
        if (soc_mem_field_valid(unit, CPU_COS_MAPm, REASONS_MASK_MIDf)) {
            soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, REASONS_MASK_MIDf,
                                mask[word_count]);   
            soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, REASONS_KEY_MIDf,
                                key[word_count]);
            word_count++;
        }
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, REASONS_MASK_HIGHf,
                            mask[word_count]);   
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, REASONS_KEY_HIGHf,
                            key[word_count]);
    }

    /* Program the packet type */ 

    if (packet_type_mask & (BCM_RX_COSQ_PACKET_TYPE_NON_UNICAST |
                         BCM_RX_COSQ_PACKET_TYPE_SWITCHED)) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            SW_PKT_TYPE_MASKf, sw_pkt_type_mask);   
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            SW_PKT_TYPE_KEYf, sw_pkt_type_key);   
    }

    if (packet_type_mask & BCM_RX_COSQ_PACKET_TYPE_MIRROR) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            MIRR_PKT_MASKf, 1);   
        if (packet_type & BCM_RX_COSQ_PACKET_TYPE_MIRROR) {
            soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                                MIRR_PKT_KEYf, 1);   
        }
    }

    /* Handle priority when int_prio_mask != 0 */
    if (int_prio_mask) {
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            INT_PRI_KEYf, (int_prio & 0xf));   
        soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, 
                            INT_PRI_MASKf, (int_prio_mask & 0xf));   
    }

    soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, COSf, cosq);
    soc_mem_field32_set(unit, CPU_COS_MAPm, &entry, VALIDf, 1);

    /* write to memory */
    SOC_IF_ERROR_RETURN(WRITE_CPU_COS_MAPm(unit, MEM_BLOCK_ANY, 
                                           index, &entry));

    return BCM_E_NONE;
}
#endif /* BCM_TRX_SUPPORT */

/*
 * Function:
 *      bcm_rx_cosq_mapping_set
 * Purpose:
 *      Set the COSQ mapping to map qualified packets to the a CPU cos queue.
 * Parameters: 
 *      unit - Unit reference
 *      index - Index into COSQ mapping table (0 is lowest match priority)
 *      reasons - packet "reasons" bitmap
 *      reasons_mask - mask for packet "reasons" bitmap
 *      int_prio - internal priority value
 *      int_prio_mask - mask for internal priority value
 *      packet_type - packet type bitmap (BCM_RX_COSQ_PACKET_TYPE_*)
 *      packet_type_mask - mask for packet type bitmap
 *      cosq - CPU cos queue
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_cosq_mapping_set(int unit, int index, 
                            bcm_rx_reasons_t reasons,
                            bcm_rx_reasons_t reasons_mask, 
                            uint8 int_prio, uint8 int_prio_mask,
                            uint32 packet_type, uint32 packet_type_mask,
                            bcm_cos_queue_t cosq)
{
#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_rx_reason_overlay)) {
       return _bcm_tr3_rx_cosq_mapping_set(unit, index,
                                           reasons, reasons_mask,
                                           int_prio, int_prio_mask, 
                                           packet_type, packet_type_mask,
                                           cosq);
   }
#endif
#ifdef BCM_TRX_SUPPORT
   if (SOC_IS_TRX(unit)) {
       return _bcm_trx_rx_cosq_mapping_set(unit, index,
                                           reasons, reasons_mask,
                                           int_prio, int_prio_mask, 
                                           packet_type, packet_type_mask,
                                           cosq);
   }
#endif
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_rx_cosq_mapping_get
 * Purpose:
 *      Get the COSQ mapping at the specified index
 * Parameters:
 *      unit - Unit reference
 *      index - Index into COSQ mapping table (0 is lowest match priority)
 *      reasons - packet "reasons" bitmap
 *      reasons_mask - mask for packet "reasons" bitmap
 *      int_prio - internal priority value
 *      int_prio_mask - mask for internal priority value
 *      packet_type - packet type bitmap (BCM_RX_COSQ_PACKET_TYPE_*)
 *      packet_type_mask - mask for packet type bitmap 
 *      cosq - CPU cos queue 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_cosq_mapping_get(int unit, int index, 
                            bcm_rx_reasons_t *reasons, bcm_rx_reasons_t *reasons_mask, 
                            uint8 *int_prio, uint8 *int_prio_mask,
                            uint32 *packet_type, uint32 *packet_type_mask,
                            bcm_cos_queue_t *cosq)
{
#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_rx_reason_overlay)) {
       return _bcm_tr3_rx_cosq_mapping_get(unit, index, reasons, reasons_mask,
                                           int_prio, int_prio_mask, 
                                           packet_type, packet_type_mask,
                                           cosq);
   }
#endif
#ifdef BCM_TRX_SUPPORT
   if (SOC_IS_TRX(unit)) {
       return _bcm_trx_rx_cosq_mapping_get(unit, index, reasons, reasons_mask,
                                           int_prio, int_prio_mask, 
                                           packet_type, packet_type_mask,
                                           cosq);
   }
#endif
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_rx_cosq_mapping_delete
 * Purpose:
 *      Delete the COSQ mapping at the specified index
 * Parameters: 
 *      unit - Unit reference
 *      index - Index into COSQ mapping table 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_cosq_mapping_delete(int unit, int index)
{
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        return _bcm_trx_rx_cosq_mapping_delete(unit, index);
    }
#endif
    return BCM_E_UNAVAIL;
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
 *      chan_id - channel index (0 - (BCM_RX_CHANNELS-1))
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_rx_queue_channel_set (int unit, bcm_cos_queue_t queue_id, 
                              bcm_rx_chan_t chan_id)
{
#ifdef BCM_CMICM_SUPPORT
    if (chan_id >= BCM_RX_CHANNELS) {
        /* API access is constrained to only the PCI host channels. */
        return BCM_E_PARAM;
    }
#endif

    return (_bcm_common_rx_queue_channel_set(unit, queue_id, 
	  				     chan_id));
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
bcm_esw_rx_queue_channel_get(int unit, bcm_cos_queue_t queue_id, 
                             bcm_rx_chan_t *chan_id)
{
    return (_bcm_common_rx_queue_channel_get(unit, queue_id, 
	  				     chan_id));
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
bcm_esw_rx_active(int unit)
{
    return (_bcm_common_rx_active(unit));
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
bcm_esw_rx_channels_running(int unit, uint32 *channels)
{
    return (_bcm_common_rx_channels_running(unit, channels));
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
bcm_esw_rx_alloc(int unit, int pkt_size, uint32 flags, void **buf)
{
    return (_bcm_common_rx_alloc(unit, pkt_size, flags, buf));
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
bcm_esw_rx_free(int unit, void *pkt_data)
{
    return (_bcm_common_rx_free(unit, pkt_data));
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
int
bcm_esw_rx_free_enqueue(int unit, void *pkt_data)
{
    return (_bcm_common_rx_free_enqueue(unit, pkt_data));
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
bcm_esw_rx_rate_set(int unit, int pps)
{
    RX_INIT_CHECK(unit);

    if (pps < 0) {
        return BCM_E_PARAM;
    }
    RX_PPS(unit) = pps;

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
    if (RX_IS_LOCAL(unit) && SOC_UNIT_VALID(unit)) {
        if (soc_feature(unit, soc_feature_packet_rate_limit)) {
#ifdef BCM_TRIUMPH2_SUPPORT
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit)) {
                return bcm_rx_cos_rate_set(unit, BCM_RX_COS_ALL, pps);
            } else if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                int burst;
                BCM_IF_ERROR_RETURN(bcm_esw_rx_burst_get(unit, &burst));
#if defined(BCM_TRIDENT2_SUPPORT)
                if (SOC_IS_TD2_TT2(unit)) {
                    return mbcm_driver[unit]->mbcm_port_rate_egress_set
                        (unit, CMIC_PORT(unit), pps, burst,
                         _BCM_PORT_RATE_PPS_MODE);
                } else
#endif /* BCM_TRIDENT2_SUPPORT */
                if (SOC_IS_TD_TT(unit)) {
                    return bcm_tr_port_pps_rate_egress_set(unit, CMIC_PORT(unit),
                                                       (uint32)pps, (uint32)burst);
                }
#if defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_KATANAX(unit)) {
                    return bcm_kt_port_pps_rate_egress_set(unit, CMIC_PORT(unit),
                                                       (uint32)pps, (uint32)burst);
                }
#endif /* BCM_KATANA_SUPPORT */
            }
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit)) {
#ifdef BCM_TRIUMPH3_SUPPORT
                if (SOC_IS_TRIUMPH3(unit)) {
                    int qcnt;
                    for (qcnt = 0; qcnt <= RX_QUEUE_MAX(unit); qcnt++) {
                        return bcm_tr3_cosq_port_pps_set(unit, 
                                                CMIC_PORT(unit), qcnt, pps);
                    }
                } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
                {
                    return bcm_tr_cosq_port_pps_set(unit, CMIC_PORT(unit),
                                               -1, pps);
                }
            }
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            if (SOC_IS_FIREBOLT2(unit)) {
                return bcm_fb2_cosq_port_pps_set(unit, CMIC_PORT(unit),
                                                 -1, pps);
            }
#endif /* BCM_FIREBOLT2_SUPPORT */
        }
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

    return BCM_E_NONE;
}

int
bcm_esw_rx_rate_get(int unit, int *pps)
{
    return (_bcm_common_rx_rate_get(unit, pps));
}

int 
bcm_esw_rx_cpu_rate_set(int unit, int pps)
{
    return (_bcm_common_rx_cpu_rate_set(unit, pps));
}

int 
bcm_esw_rx_cpu_rate_get(int unit, int *pps)
{
    return (_bcm_common_rx_cpu_rate_get(unit, pps));
}


int
bcm_esw_rx_burst_set(int unit, int burst)
{
    RX_INIT_CHECK(unit);

    RX_BURST(unit) = burst;
    RX_TOKENS(unit) = burst;

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
    if (RX_IS_LOCAL(unit) && SOC_UNIT_VALID(unit)) {
        if (soc_feature(unit, soc_feature_packet_rate_limit)) {
#ifdef BCM_TRIUMPH2_SUPPORT
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit) || SOC_IS_KATANAX(unit)) {
                return bcm_rx_cos_burst_set(unit, BCM_RX_COS_ALL, burst);
            } else 
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                int pps;
                BCM_IF_ERROR_RETURN(bcm_esw_rx_rate_get(unit, &pps));
                return mbcm_driver[unit]->mbcm_port_rate_egress_set
                    (unit, CMIC_PORT(unit), pps, burst,
                     _BCM_PORT_RATE_PPS_MODE);
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            if (SOC_IS_TD_TT(unit)) {
                int pps;
                BCM_IF_ERROR_RETURN(bcm_esw_rx_rate_get(unit, &pps));
                return bcm_tr_port_pps_rate_egress_set(unit, CMIC_PORT(unit),
                                                       (uint32)pps, (uint32)burst);
            }
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit)) {
#ifdef BCM_TRIUMPH3_SUPPORT
                if (SOC_IS_TRIUMPH3(unit)) {
                    int pps;
                    BCM_IF_ERROR_RETURN(bcm_esw_rx_rate_get(unit, &pps));
                    return mbcm_driver[unit]->mbcm_port_rate_egress_set
                        (unit, CMIC_PORT(unit), pps, burst,
                         _BCM_PORT_RATE_PPS_MODE);
                } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
                {
                    return bcm_tr_cosq_port_burst_set(unit, CMIC_PORT(unit), -1,
                                                  burst);
                }
            }
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            if (SOC_IS_FIREBOLT2(unit)) {
                return bcm_fb2_cosq_port_burst_set(unit, CMIC_PORT(unit), -1,
                                                   burst);
            }
#endif /* BCM_FIREBOLT2_SUPPORT */
        }
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

    return BCM_E_NONE;
}

int
bcm_esw_rx_burst_get(int unit, int *burst)
{
    return (_bcm_common_rx_burst_get(unit, burst));
}


int
bcm_esw_rx_cos_rate_set(int unit, int cos, int pps)
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

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
    if (RX_IS_LOCAL(unit) && SOC_UNIT_VALID(unit)) {
        if (soc_feature(unit, soc_feature_packet_rate_limit)) {
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
#ifdef BCM_TRIDENT2_SUPPORT
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_td2_cosq_port_pps_set(unit, CMIC_PORT(unit), i,
                                                      pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_td2_cosq_port_pps_set(unit, CMIC_PORT(unit), cos,
                                                  pps));
                }
                return BCM_E_NONE;
#else
                return BCM_E_UNAVAIL;
#endif
            } else 
            if (SOC_IS_TD_TT(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_td_cosq_port_pps_set(unit, CMIC_PORT(unit), i,
                                                      pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_td_cosq_port_pps_set(unit, CMIC_PORT(unit), cos,
                                                  pps));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_tr3_cosq_port_pps_set(unit, CMIC_PORT(unit), i,
                                                      pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_tr3_cosq_port_pps_set(unit, CMIC_PORT(unit), cos,
                                                  pps));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_tr2_cosq_port_pps_set(unit, CMIC_PORT(unit), i,
                                                      pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_tr2_cosq_port_pps_set(unit, CMIC_PORT(unit), cos,
                                                  pps));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_HURRICANE_SUPPORT)
            if (SOC_IS_HURRICANEX(unit)) {
                return BCM_E_NONE;
            }
#endif
#ifdef BCM_KATANA_SUPPORT
            if (SOC_IS_KATANA(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_kt_cosq_port_pps_set(unit, CMIC_PORT(unit), i,
                                                      pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_kt_cosq_port_pps_set(unit, CMIC_PORT(unit), cos,
                                                  pps));
                }
                return BCM_E_NONE;
            }
#endif
#ifdef BCM_KATANA2_SUPPORT
            if (SOC_IS_KATANA2(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_kt2_cosq_port_pps_set(unit, CMIC_PORT(unit), i,
                                                      pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_kt2_cosq_port_pps_set(unit, CMIC_PORT(unit), cos,
                                                  pps));
                }
                return BCM_E_NONE;
            }
#endif
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_tr_cosq_port_pps_set(unit, CMIC_PORT(unit), i,
                                                      pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_tr_cosq_port_pps_set(unit, CMIC_PORT(unit), cos,
                                                  pps));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            if (SOC_IS_FIREBOLT2(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_fb2_cosq_port_pps_set(unit, CMIC_PORT(unit),
                                                       i, pps));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_fb2_cosq_port_pps_set(unit, CMIC_PORT(unit),
                                                   cos, pps));
                }
                return BCM_E_NONE;
            }
 #endif /* BCM_FIREBOLT2_SUPPORT */
       }
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

    return BCM_E_NONE;
}

int
bcm_esw_rx_cos_rate_get(int unit, int cos, int *pps)
{
    return (_bcm_common_rx_cos_rate_get(unit, cos, pps));
}

int
bcm_esw_rx_cos_burst_set(int unit, int cos, int burst)
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

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
    if (RX_IS_LOCAL(unit) && SOC_UNIT_VALID(unit)) {
        if (soc_feature(unit, soc_feature_packet_rate_limit)) {
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
#ifdef BCM_TRIDENT2_SUPPORT
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_td2_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                        i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_td2_cosq_port_burst_set(unit, CMIC_PORT(unit), cos,
                                                    burst));
                }
                return BCM_E_NONE;
#else
                return BCM_E_UNAVAIL;
#endif
            } else 
            if (SOC_IS_TD_TT(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_td_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                        i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_td_cosq_port_burst_set(unit, CMIC_PORT(unit), cos,
                                                    burst));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_tr3_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                        i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_tr3_cosq_port_burst_set(unit, CMIC_PORT(unit), cos,
                                                    burst));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_tr2_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                        i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_tr2_cosq_port_burst_set(unit, CMIC_PORT(unit), cos,
                                                    burst));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
             if (SOC_IS_KATANA(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_kt_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                        i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_kt_cosq_port_burst_set(unit, CMIC_PORT(unit), cos,
                                                    burst));
                }
                return BCM_E_NONE;
            }
#endif
#ifdef BCM_KATANA2_SUPPORT
             if (SOC_IS_KATANA2(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_kt2_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                        i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_kt2_cosq_port_burst_set(unit, CMIC_PORT(unit), cos,
                                                    burst));
                }
                return BCM_E_NONE;
            }
#endif

#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_tr_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                        i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_tr_cosq_port_burst_set(unit, CMIC_PORT(unit), cos,
                                                    burst));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            if (SOC_IS_FIREBOLT2(unit)) {
                if (cos == BCM_RX_COS_ALL) {
                    for (i = 0; i <= RX_QUEUE_MAX(unit); i++) {
                        BCM_IF_ERROR_RETURN
                            (bcm_fb2_cosq_port_burst_set(unit,
                                                         CMIC_PORT(unit),
                                                         i, burst));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (bcm_fb2_cosq_port_burst_set(unit, CMIC_PORT(unit),
                                                     cos, burst));
                }
                return BCM_E_NONE;
            }
#endif /* BCM_FIREBOLT2_SUPPORT */
        }
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

    return BCM_E_NONE;
}

int
bcm_esw_rx_cos_burst_get(int unit, int cos, int *burst)
{
    return (_bcm_common_rx_cos_burst_get(unit, cos, burst));
}

int
bcm_esw_rx_cos_max_len_set(int unit, int cos, int max_q_len)
{
    return (_bcm_common_rx_cos_max_len_set(unit, cos, max_q_len));
}

int
bcm_esw_rx_cos_max_len_get(int unit, int cos, int *max_q_len)
{
    return (_bcm_common_rx_cos_max_len_get(unit, cos, max_q_len));
}

/****************************************************************
 *
 * RX Control
 *
 ****************************************************************/

STATIC int
_bcm_esw_rx_chan_flag_set(int unit, uint32 flag, int value)
{
    int chan;

    FOREACH_SETUP_CHANNEL(unit, chan) {
        if (value) {
            RX_CHAN_FLAGS(unit, chan) |= flag;
        } else {
            RX_CHAN_FLAGS(unit, chan) &= ~flag;
        }
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_esw_rx_chan_flag_get(int unit, uint32 flag, int *value)
{
    int chan;

    FOREACH_SETUP_CHANNEL(unit, chan) {
        *value = RX_CHAN_FLAGS(unit, chan) & flag;
        break;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_esw_rx_user_flag_set(int unit, uint32 flag, int value)
{
    if (value) {
        RX_USER_FLAGS(unit) |= flag;
    } else {
        RX_USER_FLAGS(unit) &= ~flag;
    }

    return BCM_E_NONE;
}


STATIC int
_bcm_esw_rx_user_flag_get(int unit, uint32 flag, int *value)
{
    *value = RX_USER_FLAGS(unit) & flag;

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_rx_control_get(int unit, bcm_rx_control_t type, int *value)
 * Description:
 *      Get the status of specified RX feature.
 * Parameters:
 *      unit - Device number
 *      type - RX control parameter
 *      value - (OUT) Current value of control parameter
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_UNAVAIL - Functionality not available
 */

int
bcm_esw_rx_control_get(int unit, bcm_rx_control_t type, int *value)
{
    int rv = BCM_E_UNAVAIL;

    switch (type) {
    case bcmRxControlCRCStrip:
        rv = _bcm_esw_rx_chan_flag_get(unit, BCM_RX_F_CRC_STRIP, value);
        break;
    case bcmRxControlVTagStrip:
        rv = _bcm_esw_rx_chan_flag_get(unit, BCM_RX_F_VTAG_STRIP, value);
        break;
    case bcmRxControlRateStall:
        rv = _bcm_esw_rx_chan_flag_get(unit, BCM_RX_F_RATE_STALL, value);
        break;
    case bcmRxControlMultiDCB:
        rv = _bcm_esw_rx_chan_flag_get(unit, BCM_RX_F_MULTI_DCB, value);
        break;
    case bcmRxControlOversizedOK:
        rv = _bcm_esw_rx_chan_flag_get(unit, BCM_RX_F_OVERSIZED_OK, value);
        break;
    case bcmRxControlIgnoreHGHeader:
        rv = _bcm_esw_rx_user_flag_get(unit, BCM_RX_F_IGNORE_HGHDR, value);
        break;
    case bcmRxControlIgnoreSLHeader:
        rv = _bcm_esw_rx_user_flag_get(unit, BCM_RX_F_IGNORE_SLTAG, value);
        break;
    default:
        /* unsupported flag */
        break;
    }
    return rv;
}

/*
 * Function:
 *      bcm_rx_control_set(int unit, bcm_rx_control_t type, int value)
 * Description:
 *      Enable/Disable specified RX feature.
 * Parameters:
 *      unit - Device number
 *      type - RX control parameter
 *      value - new value of control parameter
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_UNAVAIL - Functionality not available
 *
 */

int
bcm_esw_rx_control_set(int unit, bcm_rx_control_t type, int value)
{
    int rv = BCM_E_UNAVAIL;

    switch (type) {
    case bcmRxControlCRCStrip:
        rv = _bcm_esw_rx_chan_flag_set(unit, BCM_RX_F_CRC_STRIP, value);
        break;
    case bcmRxControlVTagStrip:
        rv = _bcm_esw_rx_chan_flag_set(unit, BCM_RX_F_VTAG_STRIP, value);
        break;
    case bcmRxControlRateStall:
        rv = _bcm_esw_rx_chan_flag_set(unit, BCM_RX_F_RATE_STALL, value);
        break;
    case bcmRxControlMultiDCB:
        rv = _bcm_esw_rx_chan_flag_set(unit, BCM_RX_F_MULTI_DCB, value);
        break;
    case bcmRxControlOversizedOK:
        rv = _bcm_esw_rx_chan_flag_set(unit, BCM_RX_F_OVERSIZED_OK, value);
        break;
    case bcmRxControlIgnoreHGHeader:
        rv = _bcm_esw_rx_user_flag_set(unit, BCM_RX_F_IGNORE_HGHDR, value);
        break;
    case bcmRxControlIgnoreSLHeader:
        rv = _bcm_esw_rx_user_flag_set(unit, BCM_RX_F_IGNORE_SLTAG, value);
        break;
    default:
        /* unsupported flag */
        break;
    }
    return rv;
}

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
bcm_esw_rx_remote_pkt_enqueue(int unit, bcm_pkt_t *pkt)
{
    return (_bcm_common_rx_remote_pkt_enqueue(unit, pkt));
}



#if defined(BROADCOM_DEBUG)

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
bcm_esw_rx_show(int unit)
{
    return _bcm_common_rx_show(unit);
}

#endif  /* BROADCOM_DEBUG */
