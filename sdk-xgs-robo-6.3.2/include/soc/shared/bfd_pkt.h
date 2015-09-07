/* 
 * $Id: bfd_pkt.h 1.17 Broadcom SDK $
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
 * File:        bfd_pkt.h
 * Purpose:     BFD Packet Format definitions
 *              common to SDK and uKernel.
 */

#ifndef   _SOC_SHARED_BFD_PKT_H_
#define   _SOC_SHARED_BFD_PKT_H_

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif

#include <shared/pack.h>
#include <shared/bfd.h>


/*******************************************
 * Pack/Unpack Macros
 * 
 * Data is packed/unpacked in/from Network byte order
 */
#define SHR_BFD_ENCAP_PACK_U8(_buf, _var)     _SHR_PACK_U8(_buf, _var)
#define SHR_BFD_ENCAP_PACK_U16(_buf, _var)    _SHR_PACK_U16(_buf, _var)
#define SHR_BFD_ENCAP_PACK_U32(_buf, _var)    _SHR_PACK_U32(_buf, _var)
#define SHR_BFD_ENCAP_UNPACK_U8(_buf, _var)   _SHR_UNPACK_U8(_buf, _var)
#define SHR_BFD_ENCAP_UNPACK_U16(_buf, _var)  _SHR_UNPACK_U16(_buf, _var)
#define SHR_BFD_ENCAP_UNPACK_U32(_buf, _var)  _SHR_UNPACK_U32(_buf, _var)


/******************************************
 * Network Packet Format definitions
 *
 * Note: LENGTH is specified in bytes unless noted otherwise
 */

/* BFD Control Packet lengths */
#define SHR_BFD_CONTROL_HEADER_LENGTH        24  /* Mandatory */
#define SHR_BFD_AUTH_SP_HEADER_START_LENGTH  3
#define SHR_BFD_AUTH_SHA1_LENGTH             28
#define SHR_BFD_AUTH_SHA1_AUTH_KEY_OFFSET    8
/* Offsets are relative to the start of BFD PDU */
#define SHR_BFD_MY_DISCR_OFFSET              4
#define SHR_BFD_YOUR_DISCR_OFFSET            8

/* UDP Header */
#define SHR_BFD_UDP_HEADER_LENGTH            8
#define SHR_BFD_UDP_SINGLE_HOP_DEST_PORT     3784
#define SHR_BFD_UDP_MULTI_HOP_DEST_PORT      4784

/* IPv4 Header */
#define SHR_BFD_IPV4_VERSION                 4
#define SHR_BFD_IPV4_HEADER_LENGTH           20    /* Bytes */
#define SHR_BFD_IPV4_HEADER_LENGTH_WORDS     5     /* Words */
#define SHR_BFD_IPV4_HEADER_SA_LENGTH        4
#define SHR_BFD_IPV4_HEADER_SA_OFFSET        12
#define SHR_BFD_IPV4_HEADER_DA_OFFSET        16

/* IPv6 Header */
#define SHR_BFD_IPV6_VERSION                 6
#define SHR_BFD_IPV6_HEADER_LENGTH           40
#define SHR_BFD_IPV6_HEADER_SA_LENGTH        16
#define SHR_BFD_IPV6_HEADER_SA_OFFSET        8
#define SHR_BFD_IPV6_HEADER_DA_OFFSET        24

/* IP Protocols */
#define SHR_BFD_IP_PROTOCOL_UDP              17
#define SHR_BFD_IP_PROTOCOL_IPV4_ENCAP       4
#define SHR_BFD_IP_PROTOCOL_IPV6_ENCAP       41
#define SHR_BFD_IP_PROTOCOL_GRE              47

/* GRE */
#define SHR_BFD_GRE_HEADER_LENGTH            4

/* Associated Channel Header */
#define SHR_BFD_ACH_HEADER_LENGTH            4
#define SHR_BFD_ACH_FIRST_NIBBLE             0x1
#define SHR_BFD_ACH_VERSION                  0x0
#define SHR_BFD_ACH_CHANNEL_TYPE_RAW         0x0007
#define SHR_BFD_ACH_CHANNEL_TYPE_IPV4        0x0021
#define SHR_BFD_ACH_CHANNEL_TYPE_IPV6        0x0057
#define SHR_BFD_ACH_CHANNEL_TYPE_MPLS_TP_CC  0x0022  /* Continuity Check */
#define SHR_BFD_ACH_CHANNEL_TYPE_MPLS_TP_CV  0x0023  /* Connectivity Verif */
#define SHR_BFD_ACH_CHANNEL_TYPE_MPLS_TP_FM  0x0058  /* Fault Management */

#define SHR_BFD_ACH_CHANNEL_IS_MPLS_TP_CC(_ach)     \
    (((_ach)[2] == 0x00) && ((_ach)[3] == 0x22))
#define SHR_BFD_ACH_CHANNEL_IS_MPLS_TP_CV(_ach)     \
    (((_ach)[2] == 0x00) && ((_ach)[3] == 0x23))
#define SHR_BFD_ACH_CHANNEL_IS_MPLS_TP_FM(_ach)     \
    (((_ach)[2] == 0x00) && ((_ach)[3] == 0x58))

#define SHR_BFD_ACH_CHANNEL_MPLS_TP_CC_SET(_ach)    \
    do {                                            \
        (_ach)[2] = 0x00;                           \
        (_ach)[3] = 0x22;                           \
    } while (0)

#define SHR_BFD_ACH_CHANNEL_MPLS_TP_CV_SET(_ach)    \
    do {                                            \
        (_ach)[2] = 0x00;                           \
        (_ach)[3] = 0x23;                           \
    } while (0)

/* MPLS Fault Management Message */
#define SHR_BFD_MPLS_FM_HEADER_LENGTH        4
#define SHR_BFD_MPLS_FM_VERSION              1
#define SHR_BFD_MPLS_FM_MSG_TYPE_AIS         1  /* Alarm Indication Signal */
#define SHR_BFD_MPLS_FM_MSG_TYPE_LKR         2  /* Lock Report */
#define SHR_BFD_MPLS_FM_FLAGS_L           0x02  /* Link Down Indication */

#define SHR_BFD_MPLS_FM_IS_AIS(_msg)         \
    (((_msg)[1]) == 1)
#define SHR_BFD_MPLS_FM_IS_LKR(_msg)         \
    (((_msg)[1]) == 2)
#define SHR_BFD_MPLS_FM_IS_LDI(_msg)         \
    (SHR_BFD_MPLS_FM_IS_AIS(_msg) && (((_msg)[2]) & SHR_BFD_MPLS_FM_FLAGS_L))

/* MPLS */
#define SHR_BFD_MPLS_LABEL_LENGTH            4
#define SHR_BFD_MPLS_ROUTER_ALERT_LABEL      1
#define SHR_BFD_MPLS_GAL_LABEL               13
#define SHR_BFD_MPLS_LABEL_LABEL_LENGTH      3  /* Label field in MPLS */

#define SHR_BFD_MPLS_IS_BOS(_label)                     \
     (((_label)[2]) & 0x1)
#define SHR_BFD_MPLS_IS_GAL(_label)                     \
    (((_label)[0] == 0x00) && ((_label)[1] == 0x00) &&  \
     ((((_label)[2]) & 0xF0) == 0xD0))

/* L2 Header */
#define SHR_BFD_L2_ETYPE_IPV4                  0x0800
#define SHR_BFD_L2_ETYPE_IPV6                  0x86DD
#define SHR_BFD_L2_ETYPE_MPLS_UCAST            0x8847
#define SHR_BFD_L2_HEADER_TAGGED_LENGTH        18     
#define SHR_BFD_L2_HEADER_TAGGED_ETYPE_OFFSET  16

/* Ethernet Frame */
#define SHR_BFD_ENET_UNTAGGED_MIN_PKT_SIZE     64
#define SHR_BFD_ENET_TAGGED_MIN_PKT_SIZE       68
#define SHR_BFD_ENET_IS_TAGGED(_pkt)           \
    (((_pkt)[12] == 0x81) && ((_pkt)[13] == 0x00))


/*************************************
 * BFD Control Packet Format
 */

/*
 * BFD Simple Password Authentication
 */
typedef struct shr_bfd_auth_header_sp_s {
    uint8   key_id;
    char    password[_SHR_BFD_AUTH_SIMPLE_PASSWORD_KEY_LENGTH];
} shr_bfd_auth_header_sp_t;

/*
 * BFD SHA1 Authentication
 */
typedef struct shr_bfd_auth_header_sha1_s {
    uint8   key_id;
    uint8   reserved;
    uint32  sequence;
    uint8   key[_SHR_BFD_AUTH_SHA1_KEY_LENGTH];
} shr_bfd_auth_header_sha1_t;

/*
 * BFD Authentication Header
 */
typedef struct shr_bfd_auth_header_s {
    uint8   type;
    uint8   length;
    union {
        shr_bfd_auth_header_sp_t    sp;
        shr_bfd_auth_header_sha1_t  sha1;
    } data;
} shr_bfd_auth_header_t;

/*
 * BFD Control Packet Header
 */
typedef struct shr_bfd_header_s {
    /* Mandatory */
#ifdef LE_HOST
    uint32  length:8,
            detect_multiplier:8,
            multipoint:1,
            demand_mode:1,
            auth_present:1,
            cpi:1,
            final:1,
            poll:1,
            state:2,
            diag:5,
            version: 3;
#else
    uint32  version: 3,
            diag:5,
            state:2,
            poll:1,
            final:1,
            cpi:1,
            auth_present:1,
            demand_mode:1,
            multipoint:1,
            detect_multiplier:8,
            length:8;
#endif  /* LE_HOST */
        
    uint32  my_discriminator;
    uint32  your_discriminator;
    uint32  desired_min_tx_interval;
    uint32  required_min_rx_interval;
    uint32  required_min_echo_rx_interval;

    /* Optional BFD Authentication header may be present */
} shr_bfd_header_t;

#endif /* _SOC_SHARED_BFD_PKT_H_ */
