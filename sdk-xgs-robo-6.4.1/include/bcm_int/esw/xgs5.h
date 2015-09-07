/* 
 * $Id: xgs5.h,v 1.6 Broadcom SDK $
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
 * File:        xgs5.h
 * Purpose:     Definitions for XGS5 systems.
 */

#ifndef   _BCM_INT_XGS5_H_
#define   _BCM_INT_XGS5_H_

#if defined(INCLUDE_BFD)

#include <soc/mcm/allenum.h>
#include <soc/tnl_term.h>
#include <bcm/tunnel.h>
#include <bcm/bfd.h>

/*
 * Local RX DMA channel
 *
 * This is the channel number local to the uC (0..3).
 * Each uController application needs to use a different RX DMA channel.
 */
#define BCM_XGS5_BFD_RX_CHANNEL    1


/*
 * Device Specific HW Definitions
 */

/* Device programming routines */
typedef struct bcm_xgs5_bfd_hw_calls_s {
    int (*l3_tnl_term_entry_init)(int unit, 
                                  bcm_tunnel_terminator_t *tnl_info,
                                  soc_tunnel_term_t *entry);
    int (*mpls_lock)(int unit);
    void (*mpls_unlock)(int unit);
} bcm_xgs5_bfd_hw_calls_t;

/* L2 Table */
typedef struct bcm_xgs5_bfd_l2_table_s {
    soc_mem_t    mem;
    soc_field_t  key_type;
    uint32       bfd_key_type;
    soc_field_t  valid;
    soc_field_t  static_bit;
    soc_field_t  session_id_type;
    soc_field_t  your_discr;
    soc_field_t  label;
    soc_field_t  session_index;
    soc_field_t  cpu_queue_class;
    soc_field_t  remote;
    soc_field_t  dst_module;
    soc_field_t  dst_port;
    soc_field_t  int_pri;
} bcm_xgs5_bfd_l2_table_t;

/* L3 IPv4 Unicast Table */
typedef struct bcm_xgs5_bfd_l3_ipv4_table_s {
    soc_mem_t    mem;
    soc_field_t  vrf_id;
    soc_field_t  ip_addr;
    soc_field_t  key_type;
    soc_field_t  local_address;
    soc_field_t  bfd_enable;
} bcm_xgs5_bfd_l3_ipv4_table_t;

/* L3 IPv6 Unicast Table */
typedef struct bcm_xgs5_bfd_l3_ipv6_table_s {
    soc_mem_t    mem;
    soc_field_t  ip_addr_lwr_64;
    soc_field_t  ip_addr_upr_64;
    soc_field_t  key_type_0;
    soc_field_t  key_type_1;
    soc_field_t  vrf_id;
    soc_field_t  local_address;
    soc_field_t  bfd_enable;
} bcm_xgs5_bfd_l3_ipv6_table_t;

/* L3 Tunnel Table */
typedef struct bcm_xgs5_bfd_l3_tunnel_table_s {
    soc_mem_t    mem;
    soc_field_t  bfd_enable;
} bcm_xgs5_bfd_l3_tunnel_table_t;

/* MPLS Table */
typedef struct bcm_xgs5_bfd_mpls_table_s {
    soc_mem_t    mem;
    soc_field_t  valid;
    soc_field_t  key_type;
    uint32       key_type_value;
    soc_field_t  mpls_label;
    soc_field_t  session_id_type;
    soc_field_t  bfd_enable;
    soc_field_t  cw_check_ctrl;
    soc_field_t  pw_cc_type;
    soc_field_t  mpls_action_if_bos;
    soc_field_t  l3_iif;
    soc_field_t  decap_use_ttl;
} bcm_xgs5_bfd_mpls_table_t;

/* HW Definitions */
typedef struct bcm_xgs5_bfd_hw_defs_t {
    bcm_xgs5_bfd_hw_calls_t         *hw_call;    /* Chip programming */
    bcm_xgs5_bfd_l2_table_t         *l2;         /* L2 Memory Table */
    bcm_xgs5_bfd_l3_ipv4_table_t    *l3_ipv4;    /* L3 IPv4 UC Table */
    bcm_xgs5_bfd_l3_ipv6_table_t    *l3_ipv6;    /* L3 IPv6 UC Table */
    bcm_xgs5_bfd_l3_tunnel_table_t  *l3_tunnel;  /* L3 Tunnel Table */
    bcm_xgs5_bfd_mpls_table_t       *mpls;       /* MPLS Table */
} bcm_xgs5_bfd_hw_defs_t;


/* Functions */
extern
int bcmi_xgs5_bfd_init(int unit,
                       bcm_esw_bfd_drv_t *drv,
                       bcm_xgs5_bfd_hw_defs_t *hw_defs);
extern
int bcmi_xgs5_bfd_detach(int unit);
extern
int bcmi_xgs5_bfd_endpoint_create(int unit,
                                  bcm_bfd_endpoint_info_t *endpoint_info);
extern
int bcmi_xgs5_bfd_endpoint_get(int unit, bcm_bfd_endpoint_t endpoint, 
                               bcm_bfd_endpoint_info_t *endpoint_info);
extern
int bcmi_xgs5_bfd_endpoint_destroy(int unit,
                                   bcm_bfd_endpoint_t endpoint);
extern
int bcmi_xgs5_bfd_endpoint_destroy_all(int unit);
extern
int bcmi_xgs5_bfd_endpoint_poll(int unit, bcm_bfd_endpoint_t endpoint);
extern
int bcmi_xgs5_bfd_event_register(int unit,
                                 bcm_bfd_event_types_t event_types, 
                                 bcm_bfd_event_cb cb, void *user_data);
extern
int bcmi_xgs5_bfd_event_unregister(int unit,
                                   bcm_bfd_event_types_t event_types, 
                                   bcm_bfd_event_cb cb);
extern
int bcmi_xgs5_bfd_endpoint_stat_get(int unit,
                                    bcm_bfd_endpoint_t endpoint, 
                                    bcm_bfd_endpoint_stat_t *ctr_info,
                                    uint8 clear);
extern
int bcmi_xgs5_bfd_auth_sha1_set(int unit, int index,
                                bcm_bfd_auth_sha1_t *sha1);
extern
int bcmi_xgs5_bfd_auth_sha1_get(int unit, int index,
                                bcm_bfd_auth_sha1_t *sha1);
extern
int bcmi_xgs5_bfd_auth_simple_password_set(int unit, int index, 
                                           bcm_bfd_auth_simple_password_t *sp);
extern
int bcmi_xgs5_bfd_auth_simple_password_get(int unit, int index, 
                                           bcm_bfd_auth_simple_password_t *sp);

#ifdef BCM_WARM_BOOT_SUPPORT
extern
int bcmi_xgs5_bfd_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern
void bcmi_xgs5_bfd_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* INCLUDE_BFD */

#endif /* _BCM_INT_XGS5_H_ */
