/*
* $Id: 2b252d8af3a2867e82458c52ae6219e34d0bab7b $
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
* This file contains the mpls table 
*/
#ifndef _BCM_INT_ROBO_MPLS_H
#define _BCM_INT_ROBO_MPLS_H

typedef struct _bcm_robo_mpls_switch_entry_s {
    bcm_mpls_label_t            ing_vp_mpls_label;/* for the key , it is the 20bits label*/
    bcm_mpls_label_t            egr_vp_mpls_label;/* for swap, it is the 32bits including label,S exp,TTL*/
    bcm_port_t                  ing_port_id;
    bcm_mpls_switch_action_t    action;           /* POP or SWAP*/
    int32                       egr_intf_index;   /* valid for SWAP action */
    bcm_vpn_t                   vpn;              /* valid for POP action */
    struct _bcm_robo_mpls_switch_entry_s *p_pre;
    struct _bcm_robo_mpls_switch_entry_s *p_next;
} _bcm_robo_mpls_switch_entry_t;

typedef enum _mpls_port_type_e{
    MPLS_PORT_TYPE_UNI,
    MPLS_PORT_TYPE_NNI
} _mpls_port_type_t;

typedef struct _bcm_robo_mpls_port_s {
    int32                       port_index;
    bcm_port_t                  port_id;
    bcm_vlan_t                  vlan_id;
    bcm_mpls_label_t            match_vc_mpls_label;/* 20bits label,valid for NNI port*/
    bcm_mpls_label_t            egr_vc_mpls_label;  /* 32 bits including label,S,exp and TTL*/
    _mpls_port_type_t           port_type;
    bcm_mpls_port_match_t       criteria;
    int32                       egr_intf_index;
    struct _bcm_robo_mpls_port_s *p_pre;
    struct _bcm_robo_mpls_port_s *p_next;
} _bcm_robo_mpls_port_t;

typedef struct _bcm_robo_mpls_vpws_vpn_s {
    bcm_vpn_t                   vpn;
    int32                       uni_port_index;
    int32                       nni_port_index;
    struct _bcm_robo_mpls_vpws_vpn_s *p_pre;
    struct _bcm_robo_mpls_vpws_vpn_s *p_next;
}_bcm_robo_mpls_vpws_vpn_t;

#define _MPLS_ROBO_MAX_ASSCO_DATA 6
#define _MPLS_ROBO_MAX_KEY_LEN  8
#define _MPLS_ROBO_MAX_ENCAP_LEN 32
typedef struct _bcm_robo_mpls_lookup_entry_s {
    int32                       uni_port_index;
    int32                       nni_port_index;
    int32                       l3_intf_index;
    bcm_mpls_label_t            vp_mpls_label;
    bcm_port_t                  port;
    bcm_vpn_t                   vpn;
    uint8                       key[_MPLS_ROBO_MAX_KEY_LEN];     /* lookup key */
    uint32                      idx;					   
    uint8                       data[_MPLS_ROBO_MAX_ASSCO_DATA]; /* associcate data */
    uint8                       encap[_MPLS_ROBO_MAX_ENCAP_LEN];
    struct _bcm_robo_mpls_lookup_entry_s *p_pre;
    struct _bcm_robo_mpls_lookup_entry_s *p_next;
}_bcm_robo_mpls_lookup_entry_t;

#endif

