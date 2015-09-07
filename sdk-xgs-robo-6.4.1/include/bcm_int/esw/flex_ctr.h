/*
 * $Id: flex_ctr.h,v 1.46 Broadcom SDK $
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
 * File:        flex_ctr.h
 * Purpose:     Contains all internal function and structure required for
 *              flex counter.
 */

#ifndef __BCM_FLEX_CTR_H__
#define __BCM_FLEX_CTR_H__

#include <sal/core/libc.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#include <bcm/l2.h>
#include <bcm/error.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/l2.h>
#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/mpls.h>
#include <bcm_int/esw/mim.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/trident.h>

/*
 ADDITIONAL INFO
 Packet attributes are of three types
 1) Packet Attributes                i.e. uncompressed mode
 2) UDF attributes                   i.e. udf mode
 3) Compressed Packet Attributes     i.e. compressed mode
*/

typedef enum bcm_stat_flex_packet_attr_type_e {
    bcmStatFlexPacketAttrTypeUncompressed=0,
    bcmStatFlexPacketAttrTypeCompressed=1,
    bcmStatFlexPacketAttrTypeUdf=2
}bcm_stat_flex_packet_attr_type_t;

/*
 ========================================================================
 Total 12 packets attributes are supported in ingress packet flow direction
 This includes

 1) cng             (2b: Pre FP packet color)
 2) IFP_CNG         (2b: Post FP packet color)
 3) INT_PRI         (4b: Packet internal priority)

 ######################################################################
 ****** In compressed mode, attributes(cng,IFP_CNG,INT_PRI) are
 ****** grouped into PRI_CNG_MAP map table.
 ######################################################################

 4) VLAN_FORMAT     (2b:Incoming PacketInner&Outer Vlan tag present indicators)
 5) OUTER_DOT1P     (3b: Incoming packet outer vlan tag priority.)
 6) INNER_DOT1P     (3b: Incoming packet inner vlan tag priority.)
 ######################################################################
 ****** In compressed mode, attributes(VLAN_FORMAT,OUTER_DOT1P,INNER_DOT1P) are
 ****** grouped into PKT_PRI_MAP map table
 ######################################################################

 7) INGRESS_PORT    (6b: Packet local ingress port.)
 ######################################################################
 ****** In compressed mode, attributes(INGRESS_PORT) are
 ****** grouped into PORT_MAP map table
 ######################################################################

 8) TOS             (8b: Incoming IP packet TOS byte.)
 ######################################################################
 ****** In compressed mode, attributes(TOS) are
 ****** grouped into TOS_MAP map table
 ######################################################################

 9) PACKET_RESOLUTION(6b: IFP packet resolution vector.)
 10)SVP_TYPE         (1b:SVP(SourceVirtualPortNumber)is network port indication)
 11)DROP             (1b: Drop indication.)
 ######################################################################
 ****** In compressed mode, attributes(PACKET_RESOLUTION,SVP_TYPE,DROP) are
 ****** grouped into PKT_RES_MAP map table
 ######################################################################

 12)IP Packet         (1b: Packet is IP.)
 Total 12 Packet attributes with 39 bits
 ========================================================================
*/

typedef struct bcm_stat_flex_ing_pkt_attr_bits_s {
    /*
    Below are default bits for uncompressed format.
    Need to be set for compressed format
    Total Bits :(2+2+4)+(2+3+3)+(6)+(8)+(6+1+1) + (1)
               : 8     + 8     + 6 + 8 + 8      + 1
               : 39 bits
    */
    /* PRI_CNF Group for compressed format */
    uint8   cng;            /* 2 for uncompressed frmt   */
    uint8   cng_pos;
    uint8   cng_mask;

    uint8   ifp_cng;              /* 2 for uncompressed format */
    uint8   ifp_cng_pos;
    uint8   ifp_cng_mask;

    uint8   int_pri;              /* 4 for uncompressed format */
    uint8   int_pri_pos;
    uint8   int_pri_mask;

    /* PKT_PRI vlan Group for compressed format */
    uint8   vlan_format;          /* 2 for uncompressed format */
    uint8   vlan_format_pos;
    uint8   vlan_format_mask;
    uint8   outer_dot1p;          /* 3 for uncompressed format */
    uint8   outer_dot1p_pos;
    uint8   outer_dot1p_mask;
    uint8   inner_dot1p;          /* 3 for uncompressed format */
    uint8   inner_dot1p_pos;
    uint8   inner_dot1p_mask;

    /* Port Group for compressed format */
    uint8   ing_port;             /* 6 for uncompressed format */
    uint8   ing_port_pos;
    uint8   ing_port_mask;

    /* TOS Group for compressed format  */
    uint8   tos_dscp;                  /* 8 for uncompressed format */
    uint8   tos_dscp_pos;
    uint8   tos_dscp_mask;
    uint8   tos_ecn;                  /* 8 for uncompressed format */
    uint8   tos_ecn_pos;
    uint8   tos_ecn_mask;
    /* PKT_RESOLTION Group for compressed format */
    uint8   pkt_resolution;       /* 6 for uncompressed format */
    uint8   pkt_resolution_pos;
    uint8   pkt_resolution_mask;

    uint8   svp_type;             /* 1 for uncompressed format */
    uint8   svp_type_pos;
    uint8   svp_type_mask;

    uint8   drop;                 /* 1 for uncompressed format */
    uint8   drop_pos;
    uint8   drop_mask;

    uint8   ip_pkt;               /* 1 for uncompressed format */
    uint8    ip_pkt_pos;
    uint8    ip_pkt_mask;
}bcm_stat_flex_ing_pkt_attr_bits_t;


/*
 ========================================================================
 Total 12 packets attributes are supported in egress packet flow direction
 This includes

 1) cng            (2b: Post FP packet color)
 2) INT_PRI        (4b: Packet internal priority)
 ######################################################################
 ****** In compressed mode, attributes(cng,INT_PRI) are
 ****** grouped into PRI_CNG_MAP map table.
 ######################################################################

 3) VLAN_FORMAT    (2b: OutgoingPacketInner&Outer Vlan tag present indicators.)
 4) OUTER_DOT1P    (3b: Incoming packet outer vlan tag priority.)
 5) INNER_DOT1P    (3b: Incoming packet inner vlan tag priority.)
 ######################################################################
 ****** In compressed mode, attributes(VLAN_FORMAT,OUTER_DOT1P,INNER_DOT1P) are
 ****** grouped into PKT_PRI_MAP map table
 ######################################################################

 6) EGRESS_PORT     (6b: Packet local egress port.)
 ######################################################################
 ****** In compressed mode, attributes(EGRESS_PORT) are
 ****** grouped into PORT_MAP map table
 ######################################################################

 7) TOS             (8b: Outgoing IP packet TOS byte.)
 ######################################################################
 ****** In compressed mode, attributes(TOS) are
 ****** grouped into TOS_MAP map table
 ######################################################################

 8) PACKET_RESOLUTION(1b: IFP packet resolution vector.)
 9)SVP_TYPE          (1b: SVP(SourceVirtualPortNumber) is NetworkPortindication)
 10)DVP_TYPE         (1b: DVP(DestinationVirtualPortNumber) is
                          NetworkPortIndication.)
 11)DROP             (1b: Drop indication.)
 ######################################################################
 ****** In compressed mode, attributes(PACKET_RESOLUTION,SVP_TYPE,DVP,DROP) are
 ****** grouped into PKT_RES_MAP map table
 12)IP Packet         (1b: Packet is IP.)
 Total 12 Packet attributes with 33 bits
 ========================================================================
*/
typedef struct bcm_stat_flex_egr_pkt_attr_bits_s {
    /*
    Below are default bits for uncompressed
    format. Need to be set for compressed format
    Total Bits :(2+4)+(2+3+3)+(6)+(8)+(1+1+1+1) + (1)
               : 6     + 8   + 6 + 8 + 4      + 1
               : 33 bits
    */

    /* PRI_CNF Group for compressed format */
    uint8   cng;                  /* 2 for uncompressed frmt   */
    uint8   cng_pos;
    uint8   cng_mask;

    uint8   int_pri;              /* 4 for uncompressed format */
    uint8   int_pri_pos;
    uint8   int_pri_mask;

    /* PKT_PRI vlan Group for compressed format */
    uint8   vlan_format;         /* 2 for uncompressed format  */
    uint8   vlan_format_pos;
    uint8   vlan_format_mask;

    uint8   outer_dot1p;         /* 3 for uncompressed format  */
    uint8   outer_dot1p_pos;
    uint8   outer_dot1p_mask;

    uint8   inner_dot1p;         /* 3 for uncompressed format   */
    uint8   inner_dot1p_pos;
    uint8   inner_dot1p_mask;

    /* Port Group for compressed format */
    uint8   egr_port;            /* 6 for uncompressed format  */
    uint8   egr_port_pos;
    uint8   egr_port_mask;

    /* TOS Group for compressed format */
    uint8   tos_dscp;                 /* 8 for uncompressed format  */
    uint8   tos_dscp_pos;
    uint8   tos_dscp_mask;
    uint8   tos_ecn;                 /* 8 for uncompressed format  */
    uint8   tos_ecn_pos;
    uint8   tos_ecn_mask;

    /* PKT_RESOLTION Group for compressed format               */
    uint8   pkt_resolution;     /* 1 for uncompressed format   */
    uint8   pkt_resolution_pos;
    uint8   pkt_resolution_mask;

    uint8   svp_type;           /* 1 for uncompressed format   */
    uint8   svp_type_pos;
    uint8   svp_type_mask;

    uint8   dvp_type;           /* 1 for uncompressed format   */
    uint8   dvp_type_pos;
    uint8   dvp_type_mask;

    uint8   drop;               /* 1 for uncompressed format   */
    uint8   drop_pos;
    uint8   drop_mask;

    uint8   ip_pkt;             /* 1 for uncompressed format   */
    uint8   ip_pkt_pos;
    uint8   ip_pkt_mask;
}bcm_stat_flex_egr_pkt_attr_bits_t;

/*
 There are two UDF (UserDefinedFields) chunks each contains one 16-bit UDF bit.
 Each UDF chunk has a valid bit associated with it. UDF attributes are 35-bits
 and contains following fields
 1) UDF0 (16-bits)
 2) UDF1 (16-bits)
 3) UDF_VALID0 (1-bit)
 4) UDF_VALID1 (1-bit)
 5) Drop (1-bit)
 For a packet, one or both of the udf valid bits might be invalid..

 UDF configuration happens separately and for configuring flex-counter we need
 below two udf(16b) masks only
*/


typedef struct bcm_stat_flex_udf_pkt_attr_bits_s {
    uint16  udf0;
    uint16  udf1;
}bcm_stat_flex_udf_pkt_attr_bits_t;

/* For UncompressedIngressMode,PacketAttributes bits are fixed i.e.cng 2 bits */
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS            0x1
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IFP_CNG_ATTR_BITS        0x2
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS        0x4
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS    0x8
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS    0x10
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS    0x20
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INGRESS_PORT_ATTR_BITS   0x40
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS       0x80
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS        0x100
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS 0x200
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS       0x400
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_DROP_ATTR_BITS           0x800
#define BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS         0x1000

/* For UncompressedEgressMode,PacketAttributes bits are fixed i.e.cng 2 bits */

#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS            0x1
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS        0x2
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS    0x4
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS    0x8
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS    0x10
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_EGRESS_PORT_ATTR_BITS    0x20
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS       0x40
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS        0x80
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS 0x100
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS       0x200
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DVP_TYPE_ATTR_BITS       0x400
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DROP_ATTR_BITS           0x800
#define BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS         0x1000

typedef struct bcm_stat_flex_offset_table_entry_s {
    uint8    offset;
    uint8    count_enable;
}bcm_stat_flex_offset_table_entry_t;

typedef struct bcm_stat_flex_ing_uncmprsd_attr_selectors_s {
    uint32                             uncmprsd_attr_bits_selector;
    uint8                              total_counters;
    bcm_stat_flex_offset_table_entry_t offset_table_map[256];
}bcm_stat_flex_ing_uncmprsd_attr_selectors_t;

typedef struct bcm_stat_flex_egr_uncmprsd_attr_selectors_s {
    uint32                             uncmprsd_attr_bits_selector;
    uint8                              total_counters;
    bcm_stat_flex_offset_table_entry_t offset_table_map[256];
}bcm_stat_flex_egr_uncmprsd_attr_selectors_t;


/*
   ###################################################
   Ingress side compressed mode maps
   ###################################################

   PRI_CNG_MAP[2^(cng:2b + ifp_cng:2b +int_pri:4b=8)=256]  *  8
   PKT_PRI_MAP[2^(VLAN_FORMAT:2b + OUTER_DOT1P:3b + INNER_DOT1P:3b=8)=256] * 8
   PORT_MAP[2^(INGRESS_PORT:6b=6)=64] * 8
   TOS_MAP[2^(TOS:8b=8)=256] * 8
   PKT_RES_MAP[2^(PACKET_RESOLUTION:6b + SVP_TYPE:1b + DROP:1b=8)=256]  * 8

*/
typedef uint8 bcm_stat_flex_ing_cmprsd_pri_cnf_attr_map_t[256]; /*256 * 8 */
typedef uint8 bcm_stat_flex_ing_cmprsd_pkt_pri_attr_map_t[256]; /*256 * 8 */
typedef uint8 bcm_stat_flex_ing_cmprsd_port_attr_map_t[170];    /*64  * 8 */
                                                                /*TD2:128*8 */
                                                                /*KT2:170*8 */
typedef uint8 bcm_stat_flex_ing_cmprsd_tos_attr_map_t[256];     /*256 * 8 */
                                                         /* KT:256 KT2:1024 */
typedef uint8 bcm_stat_flex_ing_cmprsd_pkt_res_attr_map_t[1024]; /*256 * 8 */

typedef struct bcm_stat_flex_ing_cmprsd_attr_selectors_s {
    bcm_stat_flex_ing_pkt_attr_bits_t               pkt_attr_bits;
    bcm_stat_flex_ing_cmprsd_pri_cnf_attr_map_t     pri_cnf_attr_map;
    bcm_stat_flex_ing_cmprsd_pkt_pri_attr_map_t     pkt_pri_attr_map;
    bcm_stat_flex_ing_cmprsd_port_attr_map_t        port_attr_map;
    bcm_stat_flex_ing_cmprsd_tos_attr_map_t         tos_attr_map;
    bcm_stat_flex_ing_cmprsd_pkt_res_attr_map_t     pkt_res_attr_map;
    uint8                                           total_counters;
    bcm_stat_flex_offset_table_entry_t              offset_table_map[256];
}bcm_stat_flex_ing_cmprsd_attr_selectors_t;

/*
   ###################################################
   Egress side compressed mode maps
   ###################################################

   PRI_CNG_MAP[2^(cng:2b + int_pri:4b=6)=64]  *  8
   PKT_PRI_MAP[2^(VLAN_FORMAT:2b + OUTER_DOT1P:3b + INNER_DOT1P:3b=8)=256] * 8
   PORT_MAP[2^(EGRESS_PORT:6b=6)=64] * 8
   TOS_MAP[2^(TOS:8b=8)=256] * 8
   PKT_RES_MAP[2^(PACKET_RESOLUTION:1b +
                  SVP_TYPE:1b +
                  DVP_TYPE:1b +
                  DROP:1b=4)=16]  * 8
*/
typedef uint8 bcm_stat_flex_egr_cmprsd_pri_cnf_attr_map_t[64];
typedef uint8 bcm_stat_flex_egr_cmprsd_pkt_pri_attr_map_t[256];
typedef uint8 bcm_stat_flex_egr_cmprsd_port_attr_map_t[170]; /* KT:64,KT2:169 */
typedef uint8 bcm_stat_flex_egr_cmprsd_tos_attr_map_t[256];
typedef uint8 bcm_stat_flex_egr_cmprsd_pkt_res_attr_map_t[256];

typedef struct bcm_stat_flex_egr_cmprsd_attr_selectors_s {
    bcm_stat_flex_egr_pkt_attr_bits_t           pkt_attr_bits;
    bcm_stat_flex_egr_cmprsd_pri_cnf_attr_map_t pri_cnf_attr_map;
    bcm_stat_flex_egr_cmprsd_pkt_pri_attr_map_t pkt_pri_attr_map;
    bcm_stat_flex_egr_cmprsd_port_attr_map_t    port_attr_map;
    bcm_stat_flex_egr_cmprsd_tos_attr_map_t     tos_attr_map;
    bcm_stat_flex_egr_cmprsd_pkt_res_attr_map_t pkt_res_attr_map;
    uint8                                       total_counters;
    bcm_stat_flex_offset_table_entry_t          offset_table_map[256];
}bcm_stat_flex_egr_cmprsd_attr_selectors_t;

typedef struct bcm_stat_flex_udf_pkt_attr_selectors_s {
    bcm_stat_flex_udf_pkt_attr_bits_t    udf_pkt_attr_bits;
}bcm_stat_flex_udf_pkt_attr_selectors_t;


typedef    struct bcm_stat_flex_ing_attr_s {
    bcm_stat_flex_packet_attr_type_t                packet_attr_type;
    /* union of all possible ingress packet attributes selectors
    i.e. uncompressed,compressed and udf */
    bcm_stat_flex_ing_uncmprsd_attr_selectors_t     uncmprsd_attr_selectors;
    bcm_stat_flex_ing_cmprsd_attr_selectors_t       cmprsd_attr_selectors;
    bcm_stat_flex_udf_pkt_attr_selectors_t          udf_pkt_attr_selectors;
}bcm_stat_flex_ing_attr_t;

typedef    struct bcm_stat_flex_egr_attr_s {
    bcm_stat_flex_packet_attr_type_t                packet_attr_type;
    /* union of all possible egress packet attributes selectors i.e.
    uncompressed,compressed and udf */
    bcm_stat_flex_egr_uncmprsd_attr_selectors_t     uncmprsd_attr_selectors;
    bcm_stat_flex_egr_cmprsd_attr_selectors_t       cmprsd_attr_selectors;
    bcm_stat_flex_udf_pkt_attr_selectors_t          udf_pkt_attr_selectors;
}bcm_stat_flex_egr_attr_t;

typedef    struct bcm_stat_flex_attr_s {
    bcm_stat_flex_direction_t   direction;
    bcm_stat_flex_ing_attr_t    ing_attr;
    bcm_stat_flex_egr_attr_t    egr_attr;
}bcm_stat_flex_attr_t;

/* flex counter mode. */
/* MSB bit will indicate ingress or egress mode. */
typedef uint32  bcm_stat_flex_mode_t;


typedef struct bcm_stat_flex_counter_value_s {
    uint32    pkt_counter_value;
    uint64    byte_counter_value;
    uint64    pkt64_counter_value;
}bcm_stat_flex_counter_value_t;

typedef struct bcm_stat_flex_ctr_offset_info_s {
    uint8    all_counters_flag;
    uint32   offset_index; /* Valid only when all_counters_flag is false=0*/
}bcm_stat_flex_ctr_offset_info_t;


/*  FLEX COUNTER COMMON DECLARATION */

#define BCM_STAT_FLEX_COUNTER_MAX_DIRECTION     2
#define BCM_STAT_FLEX_COUNTER_MAX_MODE          4
#define BCM_STAT_FLEX_COUNTER_MAX_POOL          16
#define BCM_STAT_FLEX_COUNTER_MAX_INGRESS_TABLE 16
#define BCM_STAT_FLEX_COUNTER_MAX_EGRESS_TABLE  8
#define BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE   32
#define BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS    8


typedef struct bcm_stat_flex_table_mode_info_t {
    soc_mem_t            soc_mem_v;
    bcm_stat_flex_mode_t mode;
}bcm_stat_flex_table_mode_info_t;


/* POOL NUMBERS for INGRESS Tables */


#define FLEX_COUNTER_DEFAULT_PORT_TABLE_POOL_NUMBER                        0
#define FLEX_COUNTER_DEFAULT_VFP_POLICY_TABLE_POOL_NUMBER                  1
/* VFI and VLAN sharing same pool id */
#define FLEX_COUNTER_DEFAULT_VFI_TABLE_POOL_NUMBER                         2
#define FLEX_COUNTER_DEFAULT_VLAN_TABLE_POOL_NUMBER                        2
#define FLEX_COUNTER_DEFAULT_VXLAN_TABLE_POOL_NUMBER                       2

/* VLAN_XLATE and MPLS_TUNNEL_LABEL sharing same pool id */
#define FLEX_COUNTER_DEFAULT_VLAN_XLATE_TABLE_POOL_NUMBER                  3
#define FLEX_COUNTER_DEFAULT_MPLS_ENTRY_TABLE_POOL_NUMBER                  3

/* MPLS_ENTRY_VC_LABEL and VRF can share same pool */
#define FLEX_COUNTER_DEFAULT_VRF_TABLE_POOL_NUMBER                         4

/* Source VP and L3_iif sharing same pool id */
#define FLEX_COUNTER_DEFAULT_L3_IIF_TABLE_POOL_NUMBER                      5
#define FLEX_COUNTER_DEFAULT_SOURCE_VP_TABLE_POOL_NUMBER                   5

#define FLEX_COUNTER_DEFAULT_L3_TUNNEL_TABLE_POOL_NUMBER                   6
#define FLEX_COUNTER_DEFAULT_FCOE_TABLE_POOL_NUMBER                        6

/* L3_ENTRY_2/4 + EXT_IPV4_UCAST_WIDE/EXT_IPV6_UCAST_WIDE share same pool id */
#define FLEX_COUNTER_DEFAULT_L3_ENTRY_TABLE_POOL_NUMBER                    7
#define FLEX_COUNTER_DEFAULT_L3_ROUTE_TABLE_POOL_NUMBER                    7

#define FLEX_COUNTER_DEFAULT_EXT_FP_POLICY_TABLE_POOL_NUMBER               8

/* POOL MASKS for INGRESS Tables */
#define FLEX_COUNTER_POOL_USED_BY_PORT_TABLE                               0x1
#define FLEX_COUNTER_POOL_USED_BY_VFP_POLICY_TABLE                         0x2
#define FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE                               0x4
#define FLEX_COUNTER_POOL_USED_BY_VFI_TABLE                                0x8
#define FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE                         0x10
#define FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE                         0x20
#define FLEX_COUNTER_POOL_USED_BY_VRF_TABLE                                0x40
#define FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE                             0x80
#define FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE                          0x100
#define FLEX_COUNTER_POOL_USED_BY_L3_TUNNEL_TABLE                          0x200
#define FLEX_COUNTER_POOL_USED_BY_L3_ENTRY_TABLE                           0x400
#define FLEX_COUNTER_POOL_USED_BY_EXT_FP_POLICY_TABLE                      0x800
#define FLEX_COUNTER_POOL_USED_BY_VXLAN_TABLE                              0x1000
#define FLEX_COUNTER_POOL_USED_BY_FCOE_TABLE                               0x2000
#define FLEX_COUNTER_POOL_USED_BY_VSAN_TABLE                               0x4000
#define FLEX_COUNTER_POOL_USED_BY_L3_ROUTE_TABLE                           0x8000

/* POOL NUMBERS for EGRESS Tables */
#define FLEX_COUNTER_DEFAULT_EGR_L3_NEXT_HOP_TABLE_POOL_NUMBER             0
#define FLEX_COUNTER_DEFAULT_EGR_VFI_TABLE_POOL_NUMBER                     1
#define FLEX_COUNTER_DEFAULT_EGR_PORT_TABLE_POOL_NUMBER                    2
#define FLEX_COUNTER_DEFAULT_EGR_VLAN_TABLE_POOL_NUMBER                    3
#define FLEX_COUNTER_DEFAULT_EGR_VLAN_XLATE_TABLE_POOL_NUMBER              4
#define FLEX_COUNTER_DEFAULT_EGR_DVP_ATTRIBUTE_1_TABLE_POOL_NUMBER         5
#define FLEX_COUNTER_DEFAULT_L3_NAT_TABLE_POOL_NUMBER                      7

#if defined(BCM_TRIDENT2_SUPPORT)
#define FLEX_COUNTER_TD2_DEFAULT_EGR_VLAN_XLATE_TABLE_POOL_NUMBER          0
#define FLEX_COUNTER_TD2_DEFAULT_EGR_DVP_ATTRIBUTE_1_TABLE_POOL_NUMBER     1
#define FLEX_COUNTER_TD2_DEFAULT_L3_NAT_TABLE_POOL_NUMBER                  2
#endif

/* POOL MASKS for EGRESS Tables */
#define FLEX_COUNTER_POOL_USED_BY_EGR_L3_NEXT_HOP_TABLE                    0x1
#define FLEX_COUNTER_POOL_USED_BY_EGR_VFI_TABLE                            0x2
#define FLEX_COUNTER_POOL_USED_BY_EGR_PORT_TABLE                           0x4
#define FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_TABLE                           0x8
#define FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_XLATE_TABLE                     0x10
#define FLEX_COUNTER_POOL_USED_BY_EGR_DVP_ATTRIBUTE_1_TABLE                0x20
#define FLEX_COUNTER_POOL_USED_BY_EGR_L3_NAT_TABLE                         0x40


#define TABLE_INDEPENDENT_POOL_MASK  0x80000000

/* ================================================================= */
/* Pkt Resolution bits for Triumph3 is 6 bits so max value is 64     */
/* Below values are just index and nothing to do with actual value   */
/* Actual chip specific values will be determined  in init function  */
/* ================================================================= */
#define _UNKNOWN_PKT       0       /* unknown packet */
#define _CONTROL_PKT       1       /* pkt(12, 13) == 0x8808 */
#define _BPDU_PKT          2       /* L2 USER ENTRY table BPDU bit */
#define _L2BC_PKT          3       /* L2 Broadcast pkt */
#define _L2UC_PKT          4       /* L2 Unicast pkt */
#define _L2DLF_PKT         5       /* L2 destination lookup failure */
#define _UNKNOWN_IPMC_PKT  6       /* Unknown IP Multicast pkt */
#define _KNOWN_IPMC_PKT    7       /* Known IP Multicast pkt */
#define _KNOWN_L2MC_PKT    8       /* Known L2 multicast pkt */
#define _UNKNOWN_L2MC_PKT  9       /* Unknown L2 multicast pkt */
#define _KNOWN_L3UC_PKT    10       /* Known L3 Unicast pkt */
#define _UNKNOWN_L3UC_PKT  11       /* Unknown L3 Unicast pkt */
#define _KNOWN_MPLS_PKT    12       /* Known MPLS pkt */
#define _KNOWN_MPLS_L3_PKT 13       /* Known L3 MPLS pkt */
#define _KNOWN_MPLS_L2_PKT 14       /* Known L2 MPLS pkt */
#define _UNKNOWN_MPLS_PKT  15       /* Unknown MPLS pkt */
#define _KNOWN_MIM_PKT     16       /* Known MIM pkt */
#define _UNKNOWN_MIM_PKT   17       /* Unknown MIM pkt */
#define _KNOWN_MPLS_MULTICAST_PKT    18       /* Known MPLS multicast pkt */

/* Additional Control Packets */
#define _OAM_PKT           19       /* OAM packet that needs to be 
                                       terminated locally */
#define _BFD_PKT           20       /* BFD packet that needs to be 
                                       terminated locally */
#define _ICNM_PKT          21       /* ICNM packet that needs to be 
                                       terminated locally */
#define _1588_PKT          22       /* ICNM packet that needs to be 
                                       terminated locally */
#define _KNOWN_TRILL_PKT   23       /* Known EgressRbridgeTRILL transit pkt */
#define _UNKNOWN_TRILL_PKT 24       /* UnKnown EgressRbridgeTRILL transit pkt */
#define _KNOWN_NIV_PKT     25       /* Known DestinationVifDownstream pkt */
#define _UNKNOWN_NIV_PKT   26       /* UnKnown DestinationVifDownstream pkt */
#define _KNOWN_L2GRE_PKT   27       /* Known L2 Gre pkt */
#define _KNOWN_VXLAN_PKT   28       /* Known VXLAN pkt */
#define _KNOWN_FCOE_PKT    29       /* Known FCoE pkt */
#define _UNKNOWN_FCOE_PKT  30       /* UnKnown FCoE pkt */



#define _UNKNOWN_PKT_KATANA       0x00       /* unknown packet */
#define _CONTROL_PKT_KATANA       0x01       /* pkt(12, 13) == 0x8808 */
#define _BPDU_PKT_KATANA          0x02       /* L2 USER ENTRY table BPDU bit */
#define _L2BC_PKT_KATANA          0x03       /* L2 Broadcast pkt */
#define _L2UC_PKT_KATANA          0x04       /* L2 Unicast pkt */
#define _L2DLF_PKT_KATANA         0x05       /* L2 destination lookup failure */
#define _UNKNOWN_IPMC_PKT_KATANA  0x06       /* Unknown IP Multicast pkt */
#define _KNOWN_IPMC_PKT_KATANA    0x07       /* Known IP Multicast pkt */
#define _KNOWN_L2MC_PKT_KATANA    0x08       /* Known L2 multicast pkt */
#define _UNKNOWN_L2MC_PKT_KATANA  0x09       /* Unknown L2 multicast pkt */
#define _KNOWN_L3UC_PKT_KATANA    0x0a       /* Known L3 Unicast pkt */
#define _UNKNOWN_L3UC_PKT_KATANA  0x0b       /* Unknown L3 Unicast pkt */
#define _KNOWN_MPLS_PKT_KATANA    0x0c       /* Known MPLS pkt */
#define _KNOWN_MPLS_L3_PKT_KATANA 0x0d       /* Known L3 MPLS pkt */
#define _KNOWN_MPLS_L2_PKT_KATANA 0x0e       /* Known L2 MPLS pkt */
#define _UNKNOWN_MPLS_PKT_KATANA  0x0f       /* Unknown MPLS pkt */
#define _KNOWN_MIM_PKT_KATANA     0x10       /* Known MIM pkt */
#define _UNKNOWN_MIM_PKT_KATANA   0x11       /* Unknown MIM pkt */
#define _KNOWN_MPLS_MULTICAST_PKT_KATANA    0x12  /* Known MPLS multicast pkt */

#define _UNKNOWN_PKT_TR3       0       /* unknown packet */
#define _CONTROL_PKT_TR3       1       /* pkt(12, 13) == 0x8808 */
#define _OAM_PKT_TR3           2       /* OAM packet            */
#define _BFD_PKT_TR3           3       /* BFD packet            */
#define _BPDU_PKT_TR3          4       /* L2 USER ENTRY table BPDU bit */
#define _ICNM_PKT_TR3          5       /* ICNM packet            */
#define _1588_PKT_TR3          6       /* 1588 packet            */
#define _KNOWN_L2UC_PKT_TR3    8       /* Known destination L2 unicast packet*/
#define _L2UC_PKT_TR3          _KNOWN_L2UC_PKT_TR3
                                       /* L2UC_PKT_TR3: L2 Unicast pkt */
#define _UNKNOWN_L2UC_PKT_TR   9       /* Unknown destination L2 UnicastPacket*/
#define _L2DLF_PKT_TR3         _UNKNOWN_L2UC_PKT_TR
                                       /* L2 destination lookup failure */
#define _KNOWN_L2MC_PKT_TR3    10       /* Known L2 multicast pkt */
#define _UNKNOWN_L2MC_PKT_TR3  11       /* Unknown L2 multicast pkt */
#define _L2BC_PKT_TR3          12      /* L2 Broadcast pkt */
#define _KNOWN_L3UC_PKT_TR3    16       /* Known L3 Unicast pkt */
#define _UNKNOWN_L3UC_PKT_TR3  17       /* Unknown L3 Unicast pkt */
#define _KNOWN_IPMC_PKT_TR3    18       /* Known IP Multicast pkt */
#define _UNKNOWN_IPMC_PKT_TR3  19       /* Unknown IP Multicast pkt */
#define _KNOWN_MPLS_L2_PKT_TR3 24       /* Known L2 MPLS pkt */
#define _UNKNOWN_MPLS_PKT_TR3  25       /* Unknown MPLS pkt */
#define _KNOWN_MPLS_L3_PKT_TR3 26       /* Known L3 MPLS pkt */
#define _KNOWN_MPLS_PKT_TR3    28       /* Known MPLS pkt */
#define _KNOWN_MPLS_MULTICAST_PKT_TR3    29       /* Known MPLS multicast pkt */
#define _KNOWN_MIM_PKT_TR3     32       /* Known MIM pkt */
#define _UNKNOWN_MIM_PKT_TR3   33       /* Unknown MIM pkt */
#define _KNOWN_TRILL_PKT_TR3   40       /* Known EgressRbridgeTRILL Transit 
                                           pkt */
#define _UNKNOWN_TRILL_PKT_TR3 41       /* UnKnown EgressRbridgeTRILL Transit
                                           Pkt */
#define _KNOWN_NIV_PKT_TR3     48       /* Known DestinationVifDownstream pkt */
#define _UNKNOWN_NIV_PKT_TR3   49       /* UnKnown DestinationVifDownstream 
                                           pkt */
/* added in TD2 */
#define _KNOWN_L2GRE_PKT_TD2   50       /* Known L2 Gre pkt */
#define _KNOWN_VXLAN_PKT_TD2   51       /* Known VXLAN pkt */
#define _KNOWN_FCOE_PKT_TD2    52       /* Known FCoE pkt */
#define _UNKNOWN_FCOE_PKT_TD2  53       /* Unknown FCoE pkt */


#define BCM_FLEX_INGRESS_POOL               0x1
#define BCM_FLEX_EGRESS_POOL                0x2
#define BCM_FLEX_ALLOCATE_POOL              0x4
#define BCM_FLEX_DEALLOCATE_POOL            0x8
#define BCM_FLEX_SHARED_POOL                0x10
#define BCM_FLEX_NOT_SHARED_POOL            0x20

typedef struct  bcm_stat_flex_pool_stat_s {
    uint32  used_by_tables;
    SHR_BITDCL used_by_objects[_SHR_BITDCLSIZE(bcmStatObjectMaxValue)];
    uint32  used_entries;
    uint32  attached_entries;
}bcm_stat_flex_pool_stat_t;

/* ******************************************************************* */
/* counter-id-specific reference count will be added later i.e. after
   basic functionality starts working */
/* ******************************************************************* */
typedef struct  bcm_stat_flex_ingress_mode_s {
    uint32        available;
    uint32        reference_count;
    uint32        total_counters;
    bcm_stat_group_mode_t    group_mode;
    uint32                              flags;
    uint32                              num_selectors;
    bcm_stat_group_mode_attr_selector_t *attr_selectors;
    bcm_stat_flex_ing_attr_t ing_attr; 
}bcm_stat_flex_ingress_mode_t;
typedef struct  bcm_stat_flex_egress_mode_s {
    uint32        available;
    uint32        reference_count;
    uint32        total_counters;
    uint32        group_mode;
    uint32                              flags;
    uint32                              num_selectors;
    bcm_stat_group_mode_attr_selector_t *attr_selectors;
    bcm_stat_flex_egr_attr_t egr_attr;
}bcm_stat_flex_egress_mode_t;

typedef struct bcm_stat_flex_table_info_s {
    soc_mem_t                 table;
    uint32                    index;
    bcm_stat_flex_direction_t direction;
}bcm_stat_flex_table_info_t;

typedef struct  bcm_stat_flex_pool_attribute_t {
        int    module_type;
        int    pool_size;
        uint32 flags ;
        int    pool_id;
        int    offset;
}bcm_stat_flex_pool_attribute_t;

typedef enum bcm_stat_flex_group_mode_e {
   bcmStatGroupModeFlex1 = bcmStatGroupModeCng + 1,
   bcmStatGroupModeFlex2, 
   bcmStatGroupModeFlex3, 
   bcmStatGroupModeFlex4 
}bcm_stat_flex_group_mode_t;

/* BEGIN: NEW FLEX FEATURE RELATED STUFF */
#define BCM_STAT_FLEX_MAX_COUNTER 256
#define BCM_STAT_FLEX_MAX_SELECTORS 256

#define SHR_BITWID 32
#define BCM_STAT_FLEX_VALUE_SET(_array, _value)    \
        (((_array)[(_value) / SHR_BITWID]) |= (1U << ((_value) % SHR_BITWID)))

#define BCM_STAT_FLEX_VALUE_GET(_array, _value)    \
        (((_array)[(_value) / SHR_BITWID]) & (1U << ((_value) % SHR_BITWID)))

#define BCM_STAT_FLEX_BIT_ARRAY_SIZE 32
typedef enum bcm_stat_flex_attr_with_value_e {
              bcmStatFlexAttrPort=0,
              bcmStatFlexAttrTosDscp=1,
              bcmStatFlexAttrTosEcn=2,
              bcmStatFlexAttrSvp=3,
              bcmStatFlexAttrDvp=4,
              bcmStatFlexAttrMax=5
}bcm_stat_flex_attr_with_value_t;

typedef struct bcm_stat_flex_combine_attr_counter_s { 

#define BCM_STAT_FLEX_COLOR_GREEN                     0x00000001
#define BCM_STAT_FLEX_COLOR_YELLOW                    0x00000002
#define BCM_STAT_FLEX_COLOR_RED                       0x00000004
        uint32 pre_ifp_color_flags; 
        uint32 ifp_color_flags; 

#define BCM_STAT_FLEX_PRI0                                0x00000001
#define BCM_STAT_FLEX_PRI1                                0x00000002
#define BCM_STAT_FLEX_PRI2                                0x00000004
#define BCM_STAT_FLEX_PRI3                                0x00000008
#define BCM_STAT_FLEX_PRI4                                0x00000010
#define BCM_STAT_FLEX_PRI5                                0x00000020
#define BCM_STAT_FLEX_PRI6                                0x00000040
#define BCM_STAT_FLEX_PRI7                                0x00000080
#define BCM_STAT_FLEX_PRI8                                0x00000100
#define BCM_STAT_FLEX_PRI9                                0x00000200
#define BCM_STAT_FLEX_PRI10                               0x00000400
#define BCM_STAT_FLEX_PRI11                               0x00000800
#define BCM_STAT_FLEX_PRI12                               0x00001000
#define BCM_STAT_FLEX_PRI13                               0x00002000
#define BCM_STAT_FLEX_PRI14                               0x00004000
#define BCM_STAT_FLEX_PRI15                               0x00008000
        uint32 int_pri_flags; 

#define BCM_STAT_FLEX_VLAN_FORMAT_UNTAGGED                    0x00000001
#define BCM_STAT_FLEX_VLAN_FORMAT_INNER                       0x00000002
#define BCM_STAT_FLEX_VLAN_FORMAT_OUTER                       0x00000004
#define BCM_STAT_FLEX_VLAN_FORMAT_BOTH                        0x00000008
        uint32 vlan_format_flags; 

/* #define BCM_STAT_FLEX_PRI0..7 */
        uint32 outer_dot1p_flags; 
        uint32 inner_dot1p_flags; 

#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_PKT                    0x00000001
#define BCM_STAT_FLEX_PKT_TYPE_CONTROL_PKT                    0x00000002
#define BCM_STAT_FLEX_PKT_TYPE_OAM_PKT                        0x00000004
#define BCM_STAT_FLEX_PKT_TYPE_BFD_PKT                        0x00000008
#define BCM_STAT_FLEX_PKT_TYPE_BPDU_PKT                       0x00000010
#define BCM_STAT_FLEX_PKT_TYPE_ICNM_PKT                       0x00000020
#define BCM_STAT_FLEX_PKT_TYPE_PKT_IS_1588                    0x00000040
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2UC_PKT                 0x00000080
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2UC_PKT               0x00000100
#define BCM_STAT_FLEX_PKT_TYPE_L2BC_PKT                       0x00000200
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2MC_PKT                 0x00000400
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2MC_PKT               0x00000800
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_L3UC_PKT                 0x00001000
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L3UC_PKT               0x00002000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_IPMC_PKT                 0x00004000
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_IPMC_PKT               0x00008000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L2_PKT              0x00010000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L3_PKT              0x00020000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_PKT                 0x00040000
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MPLS_PKT               0x00080000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_MULTICAST_PKT       0x00100000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_MIM_PKT                  0x00200000
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MIM_PKT                0x00400000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_TRILL_PKT                0x00800000
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_TRILL_PKT              0x01000000
#define BCM_STAT_FLEX_PKT_TYPE_KNOWN_NIV_PKT                  0x02000000
#define BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_NIV_PKT                0x04000000
        uint32 pkt_resolution_flags; 
        uint32 pkt_resolution_high_flags; /* We might need it in future soon */

#define BCM_STAT_FLEX_DROP_ENABLE                             0x00000001
#define BCM_STAT_FLEX_DROP_DISABLE                            0x00000002
        uint32 drop_flags; 
#define BCM_STAT_FLEX_IP_PKT_ENABLE                           0x00000001
#define BCM_STAT_FLEX_IP_PKT_DISABLE                          0x00000002
        uint32 ip_pkt_flags ;
        /* Bit Value array for port,tos,svp & dvp considering max value as 1024
        =uint32=32 bits *                 size=BCM_STAT_FLEX_BIT_ARRAY_SIZE)*/
        uint32 value_array[bcmStatFlexAttrMax][BCM_STAT_FLEX_BIT_ARRAY_SIZE];
}bcm_stat_flex_combine_attr_counter_t ;

typedef struct bcm_stat_flex_attribute_s {
        uint32 pre_ifp_color; 
        uint32 ifp_color;     
        uint32 int_pri;    
        uint32 vlan_format;
        uint32 outer_dot1p;  
        uint32 inner_dot1p;  
        uint32  port;
        uint32  tos_dscp;
        uint32  tos_ecn;
        uint32 pkt_resolution;  
        uint32 svp;
        uint32 dvp;
        uint32 drop;
        uint32 ip_pkt;
        uint32 total_counters;
        bcm_stat_flex_combine_attr_counter_t  *combine_attr_counter;
}bcm_stat_flex_attribute_t;

#ifdef BCM_WARM_BOOT_SUPPORT
typedef struct bcm_stat_flex_group_mode_related_info_s {
    uint32 flags;
    uint32 total_counters;
    uint32 num_selectors;
    bcm_stat_group_mode_attr_selector_t attr_selectors
                                        [BCM_STAT_FLEX_MAX_SELECTORS];
}bcm_stat_flex_group_mode_related_info_t;
#endif




/* END: NEW FLEX FEATURE RELATED STUFF */

extern bcm_error_t _bcm_esw_stat_group_create (
                   int                   unit,
                   bcm_stat_object_t     object,
                   bcm_stat_group_mode_t group_mode,
                   uint32                *stat_counter_id,
                   uint32                *num_entries);
extern bcm_error_t _bcm_esw_stat_group_destroy(
                   int    unit,
                   uint32 stat_counter_id);
extern bcm_error_t _bcm_esw_stat_flex_get_ingress_mode_info(
                   int                          unit,
                   bcm_stat_flex_mode_t         mode,
                   bcm_stat_flex_ingress_mode_t *ingress_mode);
extern bcm_error_t _bcm_esw_stat_flex_get_egress_mode_info(
                   int                         unit,
                   bcm_stat_flex_mode_t        mode,
                   bcm_stat_flex_egress_mode_t *egress_mode);
extern bcm_error_t _bcm_esw_stat_flex_get_available_mode(
                   int                       unit,
                   bcm_stat_flex_direction_t direction,
                   bcm_stat_flex_mode_t      *mode);
extern bcm_error_t _bcm_esw_stat_flex_update_selector_keys_enable_fields(
                   int       unit,
                   soc_reg_t pkt_attr_selector_key_reg,
                   uint64    *pkt_attr_selector_key_reg_value,
                   uint32    ctr_pkt_attr_bit_position,
                   uint32    ctr_pkt_attr_total_bits,
                   uint8     pkt_attr_field_mask_v,
                   uint8     *ctr_current_bit_selector_position);
extern bcm_error_t _bcm_esw_stat_flex_update_offset_table(
                   int                                unit,
                   bcm_stat_flex_direction_t          direction,
                   soc_mem_t                          flex_ctr_offset_table_mem,
                   bcm_stat_flex_mode_t               mode,
                   uint32                             total_counters,
                   bcm_stat_flex_offset_table_entry_t offset_table_map[256]);
extern bcm_error_t _bcm_esw_stat_flex_egress_reserve_mode(
                   int                      unit,
                   bcm_stat_flex_mode_t     mode,
                   uint32                   total_counters,
                   bcm_stat_flex_egr_attr_t *egr_attr);
extern bcm_error_t _bcm_esw_stat_flex_ingress_reserve_mode(
                   int                      unit,
                   bcm_stat_flex_mode_t     mode,
                   uint32                   total_counters,
                   bcm_stat_flex_ing_attr_t *ing_attr);
extern bcm_error_t _bcm_esw_stat_flex_unreserve_mode(
                   int                       unit,
                   bcm_stat_flex_direction_t direction,
                   bcm_stat_flex_mode_t      mode);
extern 
bcm_error_t _bcm_esw_stat_flex_update_udf_selector_keys(
            int                                    unit,
            bcm_stat_flex_direction_t              direction,
            soc_reg_t                              pkt_attr_selector_key_reg,
            bcm_stat_flex_udf_pkt_attr_selectors_t *udf_pkt_attr_selectors,
            uint32                                 *total_udf_bits);
extern bcm_error_t _bcm_esw_stat_flex_init(int unit);
extern bcm_error_t _bcm_esw_stat_flex_cleanup(int unit);
extern bcm_error_t _bcm_esw_stat_flex_sync(int unit);
extern void _bcm_esw_stat_flex_callback(int unit);
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
/* Need to remove it later */
extern int _bcm_tr2_subport_gport_used(int unit, bcm_gport_t port);
#endif

extern bcm_error_t _bcm_esw_stat_flex_create_egress_mode (
                   int                      unit,
                   bcm_stat_flex_egr_attr_t *egr_attr,
                   bcm_stat_flex_mode_t     *mode);
extern bcm_error_t _bcm_esw_stat_flex_create_ingress_mode (
                   int                      unit,
                   bcm_stat_flex_ing_attr_t *ing_attr,
                   bcm_stat_flex_mode_t     *mode);
extern bcm_error_t _bcm_esw_stat_flex_delete_egress_mode(
                   int                  unit,
                   bcm_stat_flex_mode_t mode);
extern bcm_error_t _bcm_esw_stat_flex_delete_ingress_mode(
                   int                  unit,
                   bcm_stat_flex_mode_t mode);


extern bcm_error_t _bcm_esw_stat_flex_destroy_egress_table_counters(
                   int                  unit,
                   soc_mem_t            egress_table,
                   bcm_stat_object_t    object,
                   bcm_stat_flex_mode_t offset_mode,
                   uint32               base_idx,
                   uint32               pool_number);
extern bcm_error_t _bcm_esw_stat_flex_detach_egress_table_counters(
                   int       unit,
                   soc_mem_t egress_table,
                   uint32    index);
extern bcm_error_t _bcm_esw_stat_flex_destroy_ingress_table_counters(
                   int                  unit,
                   soc_mem_t            ingress_table,
                   bcm_stat_object_t    object,
                   bcm_stat_flex_mode_t offset_mode,
                   uint32               base_idx,
                   uint32               pool_number);
extern bcm_error_t _bcm_esw_stat_flex_detach_ingress_table_counters(
                   int       unit,
                   soc_mem_t ingress_table,
                   uint32    index);
extern bcm_error_t _bcm_esw_stat_flex_attach_egress_table_counters(
                   int                  unit,
                   soc_mem_t            egress_table,
                   uint32               index,
                   bcm_stat_flex_mode_t mode,
                   uint32               base_idx,
                   uint32               pool_number);
extern bcm_error_t _bcm_esw_stat_flex_create_egress_table_counters(
                   int                  unit,
                   soc_mem_t            egress_table,
                   bcm_stat_object_t    object,
                   bcm_stat_flex_mode_t mode,
                   uint32               *base_idx,
                   uint32               *pool_number);
extern bcm_error_t _bcm_esw_stat_flex_attach_ingress_table_counters_update(
                   int                  unit,
                   uint32               pool_number,
                   uint32               base_idx,
                   bcm_stat_flex_mode_t mode);  
extern bcm_error_t _bcm_esw_stat_flex_detach_ingress_table_counters_update(
                   int                  unit,
                   uint32               pool_number,
                   uint32               base_idx,
                   bcm_stat_flex_mode_t mode);  
extern bcm_error_t _bcm_esw_stat_flex_attach_ingress_table_counters1(
                   int                  unit,
                   soc_mem_t            ingress_table,
                   uint32               index,
                   bcm_stat_flex_mode_t mode,
                   uint32               base_idx,
                   uint32               pool_number,
                   void                 *ingress_entry_data1);
extern bcm_error_t _bcm_esw_stat_flex_attach_ingress_table_counters(
                   int                  unit,
                   soc_mem_t            ingress_table,
                   uint32               index,
                   bcm_stat_flex_mode_t mode,
                   uint32               base_idx,
                   uint32               pool_number);
extern bcm_error_t _bcm_esw_stat_flex_create_ingress_table_counters(
                   int                  unit,
                   soc_mem_t            ingress_table,
                   bcm_stat_object_t    object,
                   bcm_stat_flex_mode_t mode,
                   uint32               *base_idx,
                   uint32               *pool_number);
extern bcm_error_t _bcm_esw_stat_counter_get(
                   int              unit,
                   int              sync_mode,
                   uint32           index,
                   soc_mem_t        table,
                   uint32           byte_flag,
                   uint32           counter_index,
                   bcm_stat_value_t *value);
extern bcm_error_t _bcm_esw_stat_counter_set(
                   int              unit,
                   uint32           index,
                   soc_mem_t        table,
                   uint32           byte_flag,
                   uint32           counter_index,
                   bcm_stat_value_t *value);
extern bcm_error_t _bcm_esw_stat_counter_raw_get(
                   int              unit,
                   int              sync_mode,
                   uint32           stat_counter_id,
                   uint32           byte_flag,
                   uint32           counter_index,
                   bcm_stat_value_t *value);
extern bcm_error_t _bcm_esw_stat_counter_raw_set(
            int              unit,
            uint32           stat_counter_id,
            uint32           byte_flag,
            uint32           counter_index,
            bcm_stat_value_t *value);
extern bcm_error_t _bcm_esw_stat_flex_get_table_info(
                   int                       unit,
                   bcm_stat_object_t         object,
                   uint32                    expected_num_tables,
                   uint32                    *actual_num_tables,
                   soc_mem_t                 *table,
                   bcm_stat_flex_direction_t *direction);
extern bcm_error_t _bcm_esw_stat_flex_reset_group_mode(
                   int                       unit,
                   bcm_stat_flex_direction_t direction,
                   uint32                    offset_mode,
                   bcm_stat_group_mode_t     group_mode);
extern bcm_error_t _bcm_esw_stat_flex_set_group_mode(
                   int                       unit,
                   bcm_stat_flex_direction_t direction,
                   uint32                    offset_mode,
                   bcm_stat_group_mode_t     group_mode);
extern void  _bcm_esw_stat_flex_show_mode_info(int unit); 
extern bcm_error_t _bcm_esw_stat_flex_get_ingress_object(
                   int               unit,
                   soc_mem_t         ingress_table,
                   uint32            table_index,
                   void              *ingress_entry,
                   bcm_stat_object_t *object);
extern bcm_error_t _bcm_esw_stat_flex_get_egress_object(
                   int               unit,
                   soc_mem_t         egress_table,
                   uint32            table_index,
                   void              *ingress_entry,
                   bcm_stat_object_t *object);
extern bcm_error_t _bcm_esw_stat_flex_get_counter_id(
                   int                        unit,
                   uint32                     num_of_tables,
                   bcm_stat_flex_table_info_t *table_info,
                   uint32                     *num_stat_counter_ids,
                   uint32                     *stat_counter_id);
extern void _bcm_esw_stat_group_dump_info(
            int unit,
            int all_flag,
            bcm_stat_object_t object,
            bcm_stat_group_mode_t group);
extern void _bcm_esw_stat_get_counter_id(
            bcm_stat_group_mode_t group,
            bcm_stat_object_t     object,
            uint32                mode,
            uint32                pool_number,
            uint32                base_idx,
            uint32                *stat_counter_id);
extern void _bcm_esw_stat_get_counter_id_info(
            uint32                stat_counter_id,
            bcm_stat_group_mode_t *group,
            bcm_stat_object_t     *object,
            uint32                *mode,
            uint32                *pool_number,
            uint32                *base_idx);
extern void _bcm_esw_stat_flex_init_pkt_attr_bits(int unit);
extern void _bcm_esw_stat_flex_init_pkt_res_fields(int unit);
extern uint32 _bcm_esw_stat_flex_get_pkt_res_value(
              int unit,
              uint32 pkt_res_field);
extern bcm_error_t _bcm_esw_set_flex_counter_fields_values(int       unit,
                                                           uint32    index, 
                                                           soc_mem_t table,
                                                           void      *data,
                                                           uint32    offset_mode,
                                                           uint32    pool_number,
                                                           uint32    base_idx
);
extern bcm_error_t _bcm_esw_stat_validate_object(
                   int                       unit,
                   bcm_stat_object_t         object,
                   bcm_stat_flex_direction_t *direction);
extern bcm_error_t _bcm_esw_stat_validate_group(
                   int                   unit,
                   bcm_stat_group_mode_t group);
extern bcm_error_t _bcm_esw_stat_flex_pool_operation(
                   int                            unit,
                   bcm_stat_flex_pool_attribute_t *flex_pool_attribute);

extern bcm_error_t _bcm_esw_stat_id_get_all( int unit, bcm_stat_object_t object,
                                     int stat_max, uint32 *stat_array,
                                     int *stat_count);
extern int _bcm_esw_stat_flex_pool_info_multi_get(
    int unit,
    bcm_stat_flex_direction_t direction,
    uint32 num_pools,
    uint32 *actual_num_pools,
    bcm_stat_flex_pool_stat_info_t *flex_pool_stat);
/* Create Customized Stat Group mode for given Counter Attributes */
extern int _bcm_esw_stat_group_mode_id_create(
    int unit,
    uint32 flags,
    uint32 total_counters,
    uint32 num_selectors,
    bcm_stat_group_mode_attr_selector_t *attr_selectors,
    uint32 *mode_id);

/* Retrieves Customized Stat Group mode Attributes for given mode_id */
extern int _bcm_esw_stat_group_mode_id_get(
    int unit,
    uint32 mode_id,
    uint32 *flags,
    uint32 *total_counters,
    uint32 num_selectors,
    bcm_stat_group_mode_attr_selector_t *attr_selectors,
    uint32 *actual_num_selectors);

/* Destroys Customized Group mode */
extern int _bcm_esw_stat_group_mode_id_destroy(
    int unit,
    uint32 mode_id);

/* Associate an accounting object to customized group mode */
extern int _bcm_esw_stat_custom_group_create(
    int unit,
    uint32 mode_id,
    bcm_stat_object_t object,
    uint32 *stat_counter_id,
    uint32 *num_entries);


extern bcm_stat_flex_ing_pkt_attr_bits_t ing_pkt_attr_uncmprsd_bits_g;
extern bcm_stat_flex_ing_pkt_attr_bits_t ing_pkt_attr_cmprsd_bits_g;
extern bcm_stat_flex_egr_pkt_attr_bits_t egr_pkt_attr_uncmprsd_bits_g;
extern bcm_stat_flex_egr_pkt_attr_bits_t egr_pkt_attr_cmprsd_bits_g;
extern bcm_error_t _bcm_esw_stat_flex_update_ingress_flex_info(
            int                                 unit,
            bcm_stat_flex_mode_t                mode,
            uint32                              flags,
            uint32                              num_selectors,
            bcm_stat_group_mode_attr_selector_t *attr_selectors);
extern bcm_error_t _bcm_esw_stat_flex_update_egress_flex_info(
            int                                 unit,
            bcm_stat_flex_mode_t                mode,
            uint32                              flags,
            uint32                              num_selectors,
            bcm_stat_group_mode_attr_selector_t *attr_selectors);

#endif /* __BCM_FLEX_CTR_H__ */


