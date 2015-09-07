/*
 * $Id: idents.h 1.5 Broadcom SDK $
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
 * File:        idents.h
 * Purpose:     
 */

#ifndef   _SHARED_IDENTS_H_
#define   _SHARED_IDENTS_H_

#include <shared/types.h>

/* This is the Broadcom network switching SNAP MAC and type */
#define SHARED_BRCM_NTSW_SNAP_MAC        {0xaa, 0xaa, 0x03, 0x00, 0x10, 0x18}
#define SHARED_BRCM_NTSW_SNAP_TYPE       ((uint16) 1)

/*
 * These are local IDs used to differentiate pkts.  These are really
 * used in at least two different fields, but we share the name space
 * to keep them all unique.
 */

typedef enum shared_pkt_types_e {
    SHARED_PKT_TYPE_INVALID,
    SHARED_PKT_TYPE_PROBE,          /* Discovery, probe */
    SHARED_PKT_TYPE_ROUTING,        /* Discovery, routing */
    SHARED_PKT_TYPE_CONFIG,         /* Discovery, configuration */
    SHARED_PKT_TYPE_DISC_NH,        /* Discovery, next hop pkt */
    SHARED_PKT_TYPE_NEXT_HOP,       /* Next hop protocol */
    SHARED_PKT_TYPE_ATP,            /* Acknowledge/best effort transport */
    SHARED_PKT_TYPE_NH_TX,          /* Next hop packet */
    SHARED_PKT_TYPE_C2C,            /* CPU directed packets */
    SHARED_PKT_TYPE_COUNT           /* Last, please */
} shared_pkt_types_t;

#define SHARED_PKT_TYPE_NAMES \
    {                                               \
        "invalid",                                  \
        "probe",                                    \
        "routing",                                  \
        "config",                                   \
        "disc nexthop",                             \
        "next hop",                                 \
        "atp",                                      \
        "next hop tx",                              \
        "cpu2cpu",                                  \
        "unknown"                                   \
    }


/* Client IDs for various resources */

typedef enum shared_client_id_e {
    SHARED_CLIENT_ID_INVALID,
    SHARED_CLIENT_ID_DISC_CONFIG,          /* Discovery configuration */
    SHARED_CLIENT_ID_TOPOLOGY,             /* Topology packet */
    SHARED_CLIENT_ID_TUNNEL_RX_CONNECT,    /* Set up packet tunnel */
    SHARED_CLIENT_ID_TUNNEL_RX_PKT,        /* Tunnel a received packet */
    SHARED_CLIENT_ID_TUNNEL_RX_PKT_NO_ACK, /* Tunnel best effort */
    SHARED_CLIENT_ID_TUNNEL_TX_PKT,        /* Tunnel a packet for transmit */
    SHARED_CLIENT_ID_TUNNEL_TX_PKT_NO_ACK, /* Tunnel a packet for transmit */
    SHARED_CLIENT_ID_RPC,	           /* Remote Procedure call */
    SHARED_CLIENT_ID_RLINK,                /* Async event notification */
    SHARED_CLIENT_ID_COUNT                 /* Last, please */
} shared_client_id_t;

#define SHARED_CLIENT_ID_NAMES \
    {                                               \
        "invalid",                                  \
        "disc config",                              \
        "topology",                                 \
        "tunnel connect",                           \
        "tunnel rx pkt",                            \
        "tunnel rx pkt no ack",                     \
        "tunnel tx pkt",                            \
        "tunnel tx pkt no ack",                     \
        "rpc",                                      \
        "rlink async event",                        \
        "unknown"                                   \
    }

#endif /* _SHARED_IDENTS_H_ */
