/*
 * $Id: mos_msg_common.h,v 1.56 Broadcom SDK $
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
 * File:    mos_msg_common.h
 */

#ifndef _SOC_SHARED_MOS_MSG_COMMON_H
#define _SOC_SHARED_MOS_MSG_COMMON_H

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
#define CMICM_MSG_AREAS (0x00400000)
#define NUM_UCS         (CMICM_NUM_UCS)
#endif

#include <soc/shared/mos_intr_common.h>

/*
 * Common structure shared between ARMs, host at fixed location
 */

/* This is the format of the status word sent/recvd between the
   processors.  The ack bits is what limits the number of msgs to 16. */

#define MOS_MSG_STATUS_STATE(s) ((s) & 0x03)
#define MOS_MSG_STATUS_SENT_INDEX(s) ((((s)) >> 2) & 0x0f)
#define MOS_MSG_STATUS_ACK_INDEX(s) ((((s)) >> 6) & 0x0f)

#define MOS_MSG_STATUS_STATE_W(s, i) s = (s  & 0xfffffffc) | (i)
#define MOS_MSG_STATUS_SENT_INDEX_W(s, i) s = (s  & 0xffffffc3) | ((i)  << 2)
#define MOS_MSG_STATUS_ACK_INDEX_W(s, i) s = (s  & 0xfffffc3f) | ((i) << 6)

#define MOS_MSG_ACK_MASK(i) (1 << ((i) + 16))
#define MOS_MSG_ACK_BIT(s,i) (((s) >> ((i) + 16)) & 1)

#define MOS_MSG_ACK_BIT_SET(s,i) s |= MOS_MSG_ACK_MASK(i)
#define MOS_MSG_ACK_BIT_CLEAR(s,i) s &= ~MOS_MSG_ACK_MASK(i)

typedef uint32 mos_msg_host_status_t;

#define MOS_MSG_RESET_STATE 1     /* Used to force a reset */
#define MOS_MSG_INIT_STATE  2     /* Used to indicate initilaized */
#define MOS_MSG_READY_STATE 3     /* In normal mode */

/*  Message data format.  Messages can be raw 60-bit messages or have
    a predefined structure interpreted below.  It is up to the class
    owner to make the interpretation */
typedef union {
    uint32   words[2];
    struct {
        uint8    mclass;        /* Class of this message */
        uint8    subclass;      /* Subclass (or data for raw) */
        uint16   len;           /* Arbitrary data field (usually length) */
        uint32   data;          /* Arbitrary data field (usually phys addr) */
    } s;
} mos_msg_data_t;

/* For now, all of the interfaces have 16 pending messages */
#define NUM_MOS_MSG_SLOTS 16

/* Highest order bit (0x80) is used for the subclass REPLY message */
#define MOS_MSG_SUBCLASS_REPLY(subclass) (0x80 | (subclass))

/* _MOS_MSG_DMA_FLAG indicates a "long" message (DMA required) */
#define _MOS_MSG_DMA_FLAG                     0x40
#define MOS_MSG_DMA_SET(sublass)          ((sublass) | _MOS_MSG_DMA_FLAG)
#define MOS_MSG_DMA_CLEAR(sublass)        ((sublass) & ~_MOS_MSG_DMA_FLAG)
#define MOS_MSG_DMA_MSG(sublass)          ((sublass) & _MOS_MSG_DMA_FLAG)

/* If DMAs are requested then we must round the size up */
#define MOS_MSG_DMA_LEN(x) (((x) + 3) & ~3)

/* Define the message classes and subclasses */
#define MOS_MSG_CLASS_SYSTEM            0
#define   MOS_MSG_SUBCLASS_SYSTEM_PING          1
#define   MOS_MSG_SUBCLASS_SYSTEM_APPL_INIT     2
#define   MOS_MSG_SUBCLASS_SYSTEM_LOG           3
#define   MOS_MSG_SUBCLASS_SYSTEM_INFO          4
#define   MOS_MSG_SUBCLASS_SYSTEM_INFO_REPLY    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_SUBCLASS_SYSTEM_INFO)
#define   MOS_MSG_SUBCLASS_SYSTEM_STATS         5
#define   MOS_MSG_SUBCLASS_SYSTEM_CONSOLE_IN    6
#define   MOS_MSG_SUBCLASS_SYSTEM_CONSOLE_OUT   7
#define   MOS_MSG_SUBCLASS_SYSTEM_PBUF          8
#define   MOS_MSG_SUBCLASS_SYSTEM_VERSION       9

/*
 * BFD
 */
#define MOS_MSG_CLASS_BFD                     1

/* BFD message subclasses */
#define _BFD_APPL_READY                       0x00
#define _BFD_INIT                             0x01
#define _BFD_UNINIT                           0x02
#define _BFD_SESS_SET                         0x03
#define _BFD_SESS_GET                         0x04
#define _BFD_SESS_DELETE                      0x05
#define _BFD_AUTH_SP_SET                      0x06
#define _BFD_AUTH_SP_GET                      0x07
#define _BFD_AUTH_SHA1_SET                    0x08
#define _BFD_AUTH_SHA1_GET                    0x09
#define _BFD_STAT_GET                         0x0a
#define _BFD_SESS_POLL                        0x0b
#define _BFD_SESS_DELETE_ALL                  0x0c
#define _BFD_EVENT_MASK_SET                   0x0d

#define MOS_MSG_SUBCLASS_BFD_APPL_READY             \
    _BFD_APPL_READY
#define MOS_MSG_SUBCLASS_BFD_INIT                   \
    MOS_MSG_DMA_SET(_BFD_INIT)
#define MOS_MSG_SUBCLASS_BFD_INIT_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_BFD_INIT)
#define MOS_MSG_SUBCLASS_BFD_UNINIT                 \
    _BFD_UNINIT
#define MOS_MSG_SUBCLASS_BFD_UNINIT_REPLY           \
    MOS_MSG_SUBCLASS_REPLY(_BFD_UNINIT)
#define MOS_MSG_SUBCLASS_BFD_SESS_SET               \
    MOS_MSG_DMA_SET(_BFD_SESS_SET)
#define MOS_MSG_SUBCLASS_BFD_SESS_SET_REPLY         \
    MOS_MSG_SUBCLASS_REPLY(_BFD_SESS_SET)
#define MOS_MSG_SUBCLASS_BFD_SESS_GET	\
    _BFD_SESS_GET
#define MOS_MSG_SUBCLASS_BFD_SESS_GET_REPLY         \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BFD_SESS_GET))
#define MOS_MSG_SUBCLASS_BFD_SESS_DELETE            \
    _BFD_SESS_DELETE
#define MOS_MSG_SUBCLASS_BFD_SESS_DELETE_REPLY      \
    MOS_MSG_SUBCLASS_REPLY(_BFD_SESS_DELETE)
#define MOS_MSG_SUBCLASS_BFD_AUTH_SP_SET            \
    MOS_MSG_DMA_SET(_BFD_AUTH_SP_SET)
#define MOS_MSG_SUBCLASS_BFD_AUTH_SP_SET_REPLY      \
    MOS_MSG_SUBCLASS_REPLY(_BFD_AUTH_SP_SET)
#define MOS_MSG_SUBCLASS_BFD_AUTH_SP_GET            \
    _BFD_AUTH_SP_GET
#define MOS_MSG_SUBCLASS_BFD_AUTH_SP_GET_REPLY      \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BFD_AUTH_SP_GET))
#define MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_SET          \
    MOS_MSG_DMA_SET(_BFD_AUTH_SHA1_SET)
#define MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_SET_REPLY    \
    MOS_MSG_SUBCLASS_REPLY(_BFD_AUTH_SHA1_SET)
#define MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_GET          \
    _BFD_AUTH_SHA1_GET
#define MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_GET_REPLY    \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BFD_AUTH_SHA1_GET))
#define MOS_MSG_SUBCLASS_BFD_STAT_GET               \
    MOS_MSG_DMA_SET(_BFD_STAT_GET)
#define MOS_MSG_SUBCLASS_BFD_STAT_GET_REPLY         \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BFD_STAT_GET))
#define MOS_MSG_SUBCLASS_BFD_SESS_POLL              \
    _BFD_SESS_POLL
#define MOS_MSG_SUBCLASS_BFD_SESS_POLL_REPLY        \
    MOS_MSG_SUBCLASS_REPLY(_BFD_SESS_POLL)
#define MOS_MSG_SUBCLASS_BFD_SESS_DELETE_ALL        \
    _BFD_SESS_DELETE_ALL
#define MOS_MSG_SUBCLASS_BFD_SESS_DELETE_ALL_REPLY  \
    MOS_MSG_SUBCLASS_REPLY(_BFD_SESS_DELETE_ALL)
#define MOS_MSG_SUBCLASS_BFD_EVENT_MASK_SET         \
    _BFD_EVENT_MASK_SET
#define MOS_MSG_SUBCLASS_BFD_EVENT_MASK_SET_REPLY   \
    MOS_MSG_SUBCLASS_REPLY(_BFD_EVENT_MASK_SET)


#define   MOS_MSG_CLASS_BFD_EVENT       2


/*
 * 1588/PTP Message Class
 */
#define MOS_MSG_CLASS_1588              (3)
#define   MOS_MSG_SUBCLASS_MBOX_APPL_READY              (0)
#define   MOS_MSG_SUBCLASS_MBOX_CONFIG                  (1)
#define   MOS_MSG_SUBCLASS_MBOX_CMDRESP                 (2)
#define   MOS_MSG_SUBCLASS_MBOX_TUNNEL_OUT              (3)
#define   MOS_MSG_SUBCLASS_MBOX_TUNNEL_TO               (4)
#define   MOS_MSG_SUBCLASS_MBOX_TUNNEL_IN               (5) 
#define   MOS_MSG_SUBCLASS_MBOX_EVENT_BASE              (32)

#define   MOS_MSG_MBOX_APPL_UNKNOWN  (0)
#define   MOS_MSG_MBOX_APPL_BS       (1)
#define   MOS_MSG_MBOX_APPL_1588     (2)


#define MOS_MSG_CLASS_CES               4
#define _CES_APPL_READY              0
#define _CES_CRM_INIT                0x01 /* Init hello message */
#define _CES_CRM_CONFIG              0x02 /* Common Clock Configuration */
#define _CES_CRM_RCLOCK_CONFIG       0x03 /* Per service diff configuration */
#define _CES_CRM_STATUS              0x04 /* Per port configuration */

#define   MOS_MSG_SUBCLASS_CES_APPL_READY              _CES_APPL_READY
#define   MOS_MSG_SUBCLASS_CES_CRM_INIT                MOS_MSG_DMA_SET(_CES_CRM_INIT)
#define   MOS_MSG_SUBCLASS_CES_CRM_INIT_REPLY          MOS_MSG_SUBCLASS_REPLY(_CES_CRM_INIT)
#define   MOS_MSG_SUBCLASS_CES_CRM_CONFIG              MOS_MSG_DMA_SET(_CES_CRM_CONFIG)
#define   MOS_MSG_SUBCLASS_CES_CRM_CONFIG_REPLY        MOS_MSG_SUBCLASS_REPLY(_CES_CRM_CONFIG)
#define   MOS_MSG_SUBCLASS_CES_CRM_RCLOCK_CONFIG       MOS_MSG_DMA_SET(_CES_CRM_RCLOCK_CONFIG)
#define   MOS_MSG_SUBCLASS_CES_CRM_RCLOCK_CONFIG_REPLY MOS_MSG_SUBCLASS_REPLY(_CES_CRM_RCLOCK_CONFIG)
#define   MOS_MSG_SUBCLASS_CES_CRM_STATUS              _CES_CRM_STATUS
#define   MOS_MSG_SUBCLASS_CES_CRM_STATUS_REPLY        MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_CES_CRM_STATUS))

#define MOS_MSG_CLASS_PKT_BCN           5
#define   MOS_MSG_SUBCLASS_PKT_BCN_APPL_READY   0
#define   MOS_MSG_SUBCLASS_PKT_BCN_CONFIG       1
#define   MOS_MSG_SUBCLASS_PKT_BCN_DESTROY      2
#define   MOS_MSG_SUBCLASS_PKT_BCN_GET          3
#define   MOS_MSG_SUBCLASS_PKT_BCN_START        4
#define   MOS_MSG_SUBCLASS_PKT_BCN_STOP         5

#define   MOS_MSG_SUBCLASS_PKT_BCN_CONFIG_REPLY   1
#define   MOS_MSG_SUBCLASS_PKT_BCN_DESTROY_REPLY  2
#define   MOS_MSG_SUBCLASS_PKT_BCN_GET_REPLY      3
#define   MOS_MSG_SUBCLASS_PKT_BCN_START_REPLY    4
#define   MOS_MSG_SUBCLASS_PKT_BCN_STOP_REPLY     5

#define MOS_MSG_CLASS_OANM              6
#define   MOS_MSG_SUBCLASS_OANM_APPL_READY         0


/*
 * BHH
 */
#define MOS_MSG_CLASS_BHH                     7

/* BHH message subclasses */
#define _BHH_APPL_READY                       0x00
#define _BHH_INIT                             0x01
#define _BHH_UNINIT                           0x02
#define _BHH_SESS_SET                         0x05
#define _BHH_SESS_GET                         0x06
#define _BHH_SESS_DELETE                      0x07
#define _BHH_LOOPBACK_ADD                     0x08
#define _BHH_LOOPBACK_GET                     0x09
#define _BHH_LOOPBACK_DELETE                  0x0a
#define _BHH_EVENT_MASK_SET                   0x0b
#define _BHH_STAT_GET                         0x0c
#define _BHH_SESS_ENABLE                      0x0d
#define _BHH_LOSS_MEASUREMENT_ADD             0x0e
#define _BHH_LOSS_MEASUREMENT_GET             0x0f
#define _BHH_LOSS_MEASUREMENT_DELETE          0x10
#define _BHH_DELAY_MEASUREMENT_ADD            0x11
#define _BHH_DELAY_MEASUREMENT_GET            0x12
#define _BHH_DELAY_MEASUREMENT_DELETE         0x13

#define MOS_MSG_SUBCLASS_BHH_APPL_READY             \
    _BHH_APPL_READY
#define MOS_MSG_SUBCLASS_BHH_INIT                   \
    MOS_MSG_DMA_SET(_BHH_INIT)
#define MOS_MSG_SUBCLASS_BHH_INIT_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_BHH_INIT)
#define MOS_MSG_SUBCLASS_BHH_UNINIT                 \
    _BHH_UNINIT
#define MOS_MSG_SUBCLASS_BHH_UNINIT_REPLY           \
    MOS_MSG_SUBCLASS_REPLY(_BHH_UNINIT)
#define MOS_MSG_SUBCLASS_BHH_SESS_SET               \
    MOS_MSG_DMA_SET(_BHH_SESS_SET)
#define MOS_MSG_SUBCLASS_BHH_SESS_SET_REPLY         \
    MOS_MSG_SUBCLASS_REPLY(_BHH_SESS_SET)
#define MOS_MSG_SUBCLASS_BHH_SESS_GET               \
    _BHH_SESS_GET
#define MOS_MSG_SUBCLASS_BHH_SESS_GET_REPLY         \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BHH_SESS_GET))
#define MOS_MSG_SUBCLASS_BHH_SESS_DELETE            \
    _BHH_SESS_DELETE
#define MOS_MSG_SUBCLASS_BHH_SESS_DELETE_REPLY      \
    MOS_MSG_SUBCLASS_REPLY(_BHH_SESS_DELETE)
#define MOS_MSG_SUBCLASS_BHH_STAT_GET               \
    MOS_MSG_DMA_SET(_BHH_STAT_GET)
#define MOS_MSG_SUBCLASS_BHH_STAT_GET_REPLY         \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BHH_STAT_GET))
#define MOS_MSG_SUBCLASS_BHH_SESS_ENABLE              \
    MOS_MSG_DMA_SET(_BHH_SESS_ENABLE)
#define MOS_MSG_SUBCLASS_BHH_SESS_ENABLE_REPLY        \
    MOS_MSG_SUBCLASS_REPLY(_BHH_SESS_ENABLE)
#define MOS_MSG_SUBCLASS_BHH_LOOPBACK_ADD        \
    MOS_MSG_DMA_SET(_BHH_LOOPBACK_ADD)
#define MOS_MSG_SUBCLASS_BHH_LOOPBACK_ADD_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(_BHH_LOOPBACK_ADD)
#define MOS_MSG_SUBCLASS_BHH_LOOPBACK_DELETE        \
    MOS_MSG_DMA_SET(_BHH_LOOPBACK_DELETE)
#define MOS_MSG_SUBCLASS_BHH_LOOPBACK_DELETE_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(_BHH_LOOPBACK_DELETE)
#define MOS_MSG_SUBCLASS_BHH_LOOPBACK_GET        \
    _BHH_LOOPBACK_GET
#define MOS_MSG_SUBCLASS_BHH_LOOPBACK_GET_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BHH_LOOPBACK_GET))
#define MOS_MSG_SUBCLASS_BHH_EVENT_MASK_SET         \
    _BHH_EVENT_MASK_SET
#define MOS_MSG_SUBCLASS_BHH_EVENT_MASK_SET_REPLY   \
    MOS_MSG_SUBCLASS_REPLY(_BHH_EVENT_MASK_SET)
#define MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_ADD        \
    MOS_MSG_DMA_SET(_BHH_LOSS_MEASUREMENT_ADD)
#define MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_ADD_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(_BHH_LOSS_MEASUREMENT_ADD)
#define MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_DELETE        \
    MOS_MSG_DMA_SET(_BHH_LOSS_MEASUREMENT_DELETE)
#define MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_DELETE_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(_BHH_LOSS_MEASUREMENT_DELETE)
#define MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_GET        \
    _BHH_LOSS_MEASUREMENT_GET
#define MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_GET_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BHH_LOSS_MEASUREMENT_GET))
#define MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_ADD        \
    MOS_MSG_DMA_SET(_BHH_DELAY_MEASUREMENT_ADD)
#define MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_ADD_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(_BHH_DELAY_MEASUREMENT_ADD)
#define MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_DELETE        \
    MOS_MSG_DMA_SET(_BHH_DELAY_MEASUREMENT_DELETE)
#define MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_DELETE_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(_BHH_DELAY_MEASUREMENT_DELETE)
#define MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_GET        \
    _BHH_DELAY_MEASUREMENT_GET
#define MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_GET_REPLY     \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_BHH_DELAY_MEASUREMENT_GET))

#define   MOS_MSG_CLASS_BHH_EVENT       8

/*
 * LLM
 */
#define MOS_MSG_CLASS_LLM                     9

/* LLM message subclasses */
#define _LLM_APPL_READY                       0x00
#define _LLM_INIT                             0x01
#define _LLM_UNINIT                           0x02
#define _LLM_PON_ATT_MAC_BITMAP_SET           0x03
#define _LLM_PON_ATT_MAC_BITMAP_GET           0x04
#define _LLM_APP_INFO_GET                     0x05
#define _LLM_APP_POINTER_DB_GET               0x06
#define _LLM_APP_PON_ID_DB_GET                0x07
#define _LLM_PON_ATT_MAC_BITMAP_INCREASE      0x08
#define _LLM_PON_ATT_MAC_BITMAP_DECREASE      0x09
#define _LLM_PON_ATT_SERVICE_ENABLE           0x0a
#define _LLM_PON_ATT_UPDATE                   0x0b
#define _LLM_PON_ATT_MAC_RX_MODE_SET          0x0c
#define _LLM_PON_ATT_MAC_MOVE_SET             0x0d
#define _LLM_PON_ATT_MAC_MOVE_GET             0x0e




#define MOS_MSG_SUBCLASS_LLM_APPL_READY             \
    _LLM_APPL_READY

#define MOS_MSG_SUBCLASS_LLM_INIT                   \
    MOS_MSG_DMA_SET(_LLM_INIT)
#define MOS_MSG_SUBCLASS_LLM_INIT_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_INIT)

#define MOS_MSG_SUBCLASS_LLM_UNINIT                 \
    _LLM_UNINIT
#define MOS_MSG_SUBCLASS_LLM_UNINIT_REPLY           \
    MOS_MSG_SUBCLASS_REPLY(_LLM_UNINIT)

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_SET                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_BITMAP_SET)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_SET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_PON_ATT_MAC_BITMAP_SET)

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_GET                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_BITMAP_GET)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_GET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_BITMAP_GET))

#define MOS_MSG_SUBCLASS_LLM_APP_INFO_GET                   \
    MOS_MSG_DMA_SET(_LLM_APP_INFO_GET)
#define MOS_MSG_SUBCLASS_LLM_APP_INFO_GET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_LLM_APP_INFO_GET))

#define MOS_MSG_SUBCLASS_LLM_POINTER_DB_INFO_GET                   \
    MOS_MSG_DMA_SET(_LLM_APP_POINTER_DB_GET)
#define MOS_MSG_SUBCLASS_LLM_POINTER_DB_INFO_GET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_LLM_APP_POINTER_DB_GET))

#define MOS_MSG_SUBCLASS_LLM_PON_ID_DB_GET                   \
    MOS_MSG_DMA_SET(_LLM_APP_PON_ID_DB_GET)
#define MOS_MSG_SUBCLASS_LLM_PON_ID_DB_GET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_LLM_APP_PON_ID_DB_GET))

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_INCREASE                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_BITMAP_INCREASE)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_INCREASE_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_PON_ATT_MAC_BITMAP_INCREASE)

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_DECREASE                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_BITMAP_DECREASE)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_BITMAP_DECREASE_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_PON_ATT_MAC_BITMAP_DECREASE)


#define MOS_MSG_SUBCLASS_LLM_PON_ATT_SERVICE_ENABLE                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_SERVICE_ENABLE)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_SERVICE_ENABLE_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_PON_ATT_SERVICE_ENABLE)

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_UPDATE                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_UPDATE)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_UPDATE_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_PON_ATT_UPDATE)

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_RX_MODE_SET                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_RX_MODE_SET)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_RX_MODE_SET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_PON_ATT_MAC_RX_MODE_SET)

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_MOVE_SET                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_MOVE_SET)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_MOVE_SET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(_LLM_PON_ATT_MAC_MOVE_SET)

#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_MOVE_GET                   \
    MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_MOVE_GET)
#define MOS_MSG_SUBCLASS_LLM_PON_ATT_MAC_MOVE_GET_REPLY             \
    MOS_MSG_SUBCLASS_REPLY(MOS_MSG_DMA_SET(_LLM_PON_ATT_MAC_MOVE_GET))




/*
 * BroadSync Message Class
 */
#define MOS_MSG_CLASS_BS               10


/*
 * LLM EVENT listiner
 */
#define MOS_MSG_CLASS_LLM_EVENT        11
                  
/* LLM event subclasses */
#define _LLM_NOTIFICATION                     0x00
#define _LLM_EVENT_MAC_MOVE                   0x01

#define MOS_MSG_SUBCLASS_LLM_NOTIFICATION                   \
    _LLM_NOTIFICATION

#define MOS_MSG_SUBCLASS_LLM_EVENT_MAC_MOVE                 \
    MOS_MSG_DMA_SET(_LLM_EVENT_MAC_MOVE)

/*
 * Test interface
 */
#define MOS_MSG_CLASS_TEST              12
#define   MOS_MSG_SUBCLASS_TEST_APPL_READY      0
#define   MOS_MSG_SUBCLASS_TEST_COUNT           1
#define   MOS_MSG_SUBCLASS_TEST_RUN             2

/*
 * PSCAN application
 */
#define MOS_MSG_CLASS_PSCAN             13
#define MOS_MSG_SUBCLASS_PSCAN_APPL_READY       0
#define MOS_MSG_SUBCLASS_PSCAN_SCAN_DELAY       1
#define MOS_MSG_SUBCLASS_PSCAN_PORT_CONFIG      2
#define MOS_MSG_SUBCLASS_PSCAN_PORT_BITMAP      MOS_MSG_DMA_SET(3)
#define MOS_MSG_SUBCLASS_PSCAN_PORT_BITMAP_REPLY 4

#define MOS_MSG_CLASS_PSCAN_EVENT       14
#define MOS_MSG_SUBCLASS_PSCAN_PORT_ALERT       0

#define MOS_MSG_CLASS_VERSION           15

/* Notes:
 *  - Changing MOS_MSG_CLASS numbers will break backwards compatibility.
 *  - Please contact stakeholders before modifying MOS_MSG_CLASS sections
 *  - MAX_MOS_MSG_CLASS should == the value of the highest MOS_MSG_CLASS
 */

#define MAX_MOS_MSG_CLASS               15

#define MOS_MSG_INCR(val) (((val) + 1) & (NUM_MOS_MSG_SLOTS - 1))

/* Message area per processor pair/direction */
typedef struct {
    mos_msg_host_status_t status;
    mos_msg_data_t data[NUM_MOS_MSG_SLOTS];
} mos_msg_area_t;

#define MOS_MSG_HOST_SDK 0
#define MOS_MSG_HOST_UC0 1
#define MOS_MSG_HOST_UC1 2

typedef struct {
    uint32 config;
    uint32 clk_ctrl;
    uint32 heartbeat_ctrl;
    uint32 heartbeat_down_duration;
    uint32 heartbeat_up_duration;
} mos_msg_bs_reg_cache_t;

/* Structure for containing 32/48/64-bit hardware timestamp */
typedef struct {
    uint32 ts_hi;
    uint32 ts_lo;
} mos_msg_hw_ts_t;

/* Area for storage of most recent timestamps from timestamp FIFO
 * together with system (PTP / Broadsync) time
 */
#define MOS_MSG_MAX_TS_EVENTS (19)
typedef struct {
    uint32 capture_mask;
    uint32 full_time_seconds_hi;
    uint32 full_time_seconds_low;
    uint32 full_time_nanoseconds;

    mos_msg_hw_ts_t ts0_timestamp;
    mos_msg_hw_ts_t ts1_timestamp;

    mos_msg_hw_ts_t event_timestamp[MOS_MSG_MAX_TS_EVENTS];
    mos_msg_hw_ts_t prev_event_timestamp[MOS_MSG_MAX_TS_EVENTS];
} mos_msg_timestamps_t;

/*  Fixed memory locations */

/* The host-to UC message areas are first */
#define MSG_AREA_HOST_TO_ARM(base, arm) ((base) + (sizeof(mos_msg_area_t) * (arm)))

/* The ARM areas are next with the uC to host area taking the slot that
   would be for self messages */
#define MSG_AREA_ARM_TO_HOST(base, arm) ((base) + (sizeof(mos_msg_area_t) * (NUM_UCS + (NUM_UCS * (arm)) + (arm))))
#define MSG_AREA_ARM_TO_ARM(base, FROM, TO) ((base) + (sizeof(mos_msg_area_t) * (NUM_UCS  + (NUM_UCS * (FROM)) + (TO))))

#define MSG_AREA_END(base) ((base) + sizeof(mos_msg_area_t) * 2 * (NUM_UCS+1) * NUM_UCS)

#endif
