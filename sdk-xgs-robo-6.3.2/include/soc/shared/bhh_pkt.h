/* 
 * $Id: bhh_pkt.h 1.12 Broadcom SDK $
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
 * File:        bhh_pkt.h
 * Purpose:     BHH Packet Format definitions
 *              common to SDK and uKernel.
 */

#ifndef   _SOC_SHARED_BHH_PKT_H_
#define   _SOC_SHARED_BHH_PKT_H_

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif

#include <shared/pack.h>
#include <shared/bhh.h>


/*******************************************
 * Pack/Unpack Macros
 * 
 * Data is packed/unpacked in/from Network byte order
 */
#define SHR_BHH_ENCAP_PACK_U8(_buf, _var)     _SHR_PACK_U8(_buf, _var)
#define SHR_BHH_ENCAP_PACK_U16(_buf, _var)    _SHR_PACK_U16(_buf, _var)
#define SHR_BHH_ENCAP_PACK_U32(_buf, _var)    _SHR_PACK_U32(_buf, _var)
#define SHR_BHH_ENCAP_UNPACK_U8(_buf, _var)   _SHR_UNPACK_U8(_buf, _var)
#define SHR_BHH_ENCAP_UNPACK_U16(_buf, _var)  _SHR_UNPACK_U16(_buf, _var)
#define SHR_BHH_ENCAP_UNPACK_U32(_buf, _var)  _SHR_UNPACK_U32(_buf, _var)


/******************************************
 * Network Packet Format definitions
 *
 * Note: LENGTH is specified in bytes unless noted otherwise
 */

/* BHH Control Packet lengths */
#define SHR_BHH_HEADER_LENGTH                4
#define SHR_BHH_ACH_LENGTH                   4
#define SHR_BHH_ACH_TYPE_LENGTH              2

/* Associated Channel Header */
#define SHR_BHH_ACH_FIRST_NIBBLE             0x1
#define SHR_BHH_ACH_VERSION                  0x0
#define SHR_BHH_ACH_CHANNEL_TYPE             0x8902


/* MPLS */
#define SHR_BHH_MPLS_ROUTER_ALERT_LABEL      1
#define SHR_BHH_MPLS_GAL_LABEL               13
#define SHR_BHH_MPLS_PW_LABEL                0xFF


/* MPLS */
#define SHR_BHH_MPLS_LABEL_LENGTH            4
#define SHR_BHH_MPLS_ROUTER_ALERT_LABEL      1
#define SHR_BHH_MPLS_GAL_LABEL               13
#define SHR_BHH_MPLS_LABEL_LABEL_LENGTH      3  /* Label field in MPLS */
#define SHR_BHH_MPLS_LABEL_MASK              0xFFFFF000

#define SHR_BHH_MPLS_IS_BOS(_label)                     \
     (((_label)[2]) & 0x1)
#define SHR_BHH_MPLS_IS_GAL(_label)                     \
    (((_label)[0] == 0x00) && ((_label)[1] == 0x00) &&  \
     ((((_label)[2]) & 0xF0) == 0xD0))

/* Ether Type */
#define SHR_BHH_L2_ETYPE_VLAN                  0x8100
#define SHR_BHH_L2_ETYPE_MPLS_UCAST            0x8847
#define SHR_BHH_L2_HEADER_ETYPE_OFFSET         16
#define SHR_BHH_L2_HEADER_TAGGED_LENGTH        18     
#define SHR_BHH_L2_HEADER_TAGGED_ETYPE_OFFSET  16

/* Ethernet Frame */
#define SHR_BHH_ENET_UNTAGGED_MIN_PKT_SIZE     64
#define SHR_BHH_ENET_TAGGED_MIN_PKT_SIZE       68
#define SHR_BHH_ENET_IS_TAGGED(_pkt)           \
    (((_pkt)[12] == 0x81) && ((_pkt)[13] == 0x00))

/* BHH op-code */
#define SHR_BHH_OPCODE_LEN                     1
#define SHR_BHH_OPCODE_LM_PREFIX              42
#define SHR_BHH_OPCODE_DM_PREFIX              44
#define SHR_BHH_OPCODE_LM_MASK                0xFE
#define SHR_BHH_OPCODE_DM_MASK                0xFC


/*************************************
 * BHH Control Packet Format
 */

/*
 * BHH Y.1731 Header
 */
typedef struct shr_bhh_header_s {
    /* Mandatory */
#ifdef LE_HOST
    uint32 tlv_offset:8,
           flags_period:3,
           flags_rsvd:4,
           flags_rdi:1,
           opcode:8,
           version:5,
           mel:3;
#else
    uint32 mel:3,
           version:5,
           opcode:8,
           flags_rdi:1,
           flags_rsvd:4,
           flags_period:3,
           tlv_offset:8;
#endif  /* LE_HOST */
} shr_bhh_header_t;

typedef struct shr_bhh_mpls_header_s {
#ifdef LE_HOST
    uint32 ttl:8,
	s:1,
	exp:3,
	label:20;
#else
    uint32 label:20,
	exp:3,
	s:1,
	ttl:8;
#endif
} shr_bhh_mpls_header_t;

#endif /* _SOC_SHARED_BHH_PKT_H_ */
