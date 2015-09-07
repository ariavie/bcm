/*
 * $Id: draco_arltest.h 1.16 Broadcom SDK $
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
 * Draco ARL Test definitions.
 *
 */

#ifndef __DRACO_ARLTEST__H
#define __DRACO_ARLTEST__H

/*
 * Draco ARL Test Data Structure
 */

typedef struct draco_l2_testdata_s {
    int                 unit;
    int                 opt_count;
    int                 opt_verbose;
    int                 opt_reset;
    int                 opt_hash;
    int                 opt_base_vid;
    int                 opt_vid_inc;         /* VID increment */
    sal_mac_addr_t      opt_base_mac;
    int                 opt_mac_inc;         /* Mac increment */
    COMPILER_DOUBLE     tm;
    uint32              save_hash_control;
    int                 hash_count;          /* Hash Alg count */
    int                 save_l2_aux_hash_control;
} draco_l2_testdata_t;

#define DRACO_L2_VID_MAX        (0x7ff) 
#define DRACO_L3_VID_MAX        (0xfff) 
#define DRACO_ARL_DEFAULT_HASH  XGS_HASH_LSB

#define FB_L2_VID_MAX            (0xfff)
#define FB_L2_DEFAULT_HASH      FB_HASH_LSB
#define FB_L3_DEFAULT_HASH      FB_HASH_LSB

#define MAX_VRF_ID              (0x3f)
#define FB2_VRF_ID_MAX          MAX_VRF_ID
#define TR_VRF_ID_MAX           (0x7ff)

typedef struct draco_l3_testdata_s {
    int                 unit;
    int                 opt_count;
    int                 opt_verbose;
    int                 opt_reset;
    int                 opt_hash;
    int                 opt_dual_hash;
    int                 opt_ipmc_enable;
    int                 opt_key_src_ip;      /* Source IP in upper IPMC key */
    ip_addr_t           opt_base_ip;
    int                 opt_ip_inc;          /* IP addr increment */
    ip_addr_t           opt_src_ip;
    int                 opt_src_ip_inc;      /* Source IP addr increment */
    int                 opt_base_vid;
    int                 opt_vid_inc;         /* VID increment */
    sal_mac_addr_t      opt_base_mac;
    int                 opt_mac_inc;         /* Mac increment */
    ip6_addr_t          opt_base_ip6;
    ip6_addr_t          opt_src_ip6;
    int                 opt_ip6_inc;
    int                 opt_src_ip6_inc;
    int                 ipv6;                /* Test uses IPv6 args */
    COMPILER_DOUBLE     tm;
    int                 save_hash_control;
    int                 save_dual_hash_control;
    uint32              save_ipmc_config;
    uint32              set_ipmc_config;
    int                 hash_count;          /* Hash Alg count */
    int                 opt_base_vrf_id;     /* VRF ID */ 
    int                 opt_vrf_id_inc;      /* VRF ID Increment */ 
} draco_l3_testdata_t;

/* L2/L3 test structure, to retain test settings between runs */
typedef struct draco_l2_test_s {
    int                 dlw_set_up;     /* TRUE if dl2_setup() done */
    draco_l2_testdata_t dlp_l2_hash;    /* L2 hash Parameters */
    draco_l2_testdata_t dlp_l2_ov;      /* L2 overflow Parameters */
    draco_l2_testdata_t dlp_l2_lu;      /* L2 lookup Parameters */
    draco_l2_testdata_t dlp_l2_dp;      /* L2 delete by port Parameters */
    draco_l2_testdata_t dlp_l2_dv;      /* L2 delete by VLAN Parameters */
    draco_l2_testdata_t *dlp_l2_cur;    /* Current L2 test parameters */
    int                 dlw_unit;       /* Unit # */
} draco_l2_test_t;

typedef struct draco_l3_test_s {
    int                 dlw_set_up;     /* TRUE if dl3_setup() done */
    draco_l3_testdata_t dlp_l3_hash;    /* L3 hash Parameters */
    draco_l3_testdata_t dlp_l3_ov;      /* L3 overflow Parameters */
    draco_l3_testdata_t *dlp_l3_cur;    /* Current L3 test parameters */
    int                 dlw_unit;       /* Unit # */
} draco_l3_test_t;

#endif /*!__DRACO_ARLTEST__H */
