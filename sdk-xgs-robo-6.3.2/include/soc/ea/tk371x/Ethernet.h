/*
 * $Id: Ethernet.h 1.1 Broadcom SDK $
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
 * File:     Ethernet.h
 * Purpose: 
 *
 */
 
#ifndef _SOC_EA_ETHERNET_H
#define _SOC_EA_ETHERNET_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkConfig.h>


/* pack structures */
#if defined(UNIX) || defined(VXWORKS) || defined(LINUX)
#pragma pack (1)
#endif

#ifdef TK_NEED_PKT_HEAD     
    #define TK_PK_HEAD_LEN      0
#endif

#define EthMinFrameSize             64
#define EthMinFrameSizeWithoutCrc   (EthMinFrameSize - 4)

/* Ethernet protocol type codes */
typedef enum {
    EthertypeArp        = 0x0806,
    EthertypeEapol      = 0x888E,
    EthertypeEapolOld   = 0x8180,
    EthertypeLoopback   = 0x9000,
    EthertypeMpcp       = 0x8808,
    EthertypeOam        = 0x8809,
    EthertypePppoeDisc  = 0x8863,
    EthertypePppoeSess  = 0x8864,
    EthertypeRarp       = 0x8035,
    EthertypeVlan       = 0x8100,
    EthertypeIp         = 0x0800,
} Ethertype;


/* MAC address */
typedef union {
    uint8   byte[6];
    uint16  word[3];
    uint8   u8[6];
} PACK MacAddr;

/* Basic Ethernet frame format */
typedef struct {
    MacAddr da;
    MacAddr sa;
#ifdef TK_NEED_PKT_HEAD
    uint8   pktDataHead[TK_PK_HEAD_LEN];
#endif
#ifdef TK_NEED_MGNT_VLAN
    uint16  vlanType;
    uint16  vlanInfo;
#endif
    uint16  type;
} PACK EthernetFrame;

/* VLAN tag */
typedef uint16 VlanTag;

/* VLAN header */
typedef struct {
    uint16  type;       /* EthertypeVlan */
    VlanTag tag;        /* priority + CFI + VID */
} PACK EthernetVlanData;


typedef struct {
    MacAddr Dst;
    MacAddr Src;
    uint16  VlanType;
    uint16  Vlan;
    uint16  Type;
} PACK VlanTaggedEthernetFrame;

#if defined(UNIX) || defined(VXWORKS) || defined(LINUX)
#pragma pack()
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_ETHERNET_H */
