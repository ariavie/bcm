/*
 * $Id: nh_tx.h,v 1.11 Broadcom SDK $
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
 * File:        nh_tx.h
 * Purpose:     Header file for next hop transmit functions
 */

#ifndef   _CPUTRANS_NH_TX_H_
#define   _CPUTRANS_NH_TX_H_

#include <sdk_config.h>
#include <shared/idents.h>

#include <bcm/types.h>
#include <bcm/rx.h>

/* Next hop callback function prototype */
typedef int (*nh_tx_cb_f)(int unit, int port, uint8 *pkt_buf, int len,
                           void *cookie);

/* Next hop transmit function prototype */
typedef int (*nh_tx_tx_f)(int unit, int port, uint8 *pkt_buf, int len,
                           nh_tx_cb_f callback, void *cookie);

extern int nh_tx_setup(bcm_trans_ptr_t *trans_ptr);
extern int nh_tx_setup_done(void);

extern void nh_tx_l2_header_setup(uint8 *pkt_buf, uint16 vlan);

extern int nh_tx_dest_set(const bcm_mac_t mac, int dest_mod, int dest_port);
extern int nh_tx_dest_get(bcm_mac_t *mac, int *dest_mod, int *dest_port);
extern int nh_tx_dest_install(int install, int vid);

extern int nh_tx_src_set(int src_mod, int src_port);
extern int nh_tx_src_get(int *src_mod, int *src_port);

extern int nh_tx_trans_ptr_set(int unit, int port, bcm_trans_ptr_t *ptr);
extern int nh_tx_trans_ptr_get(int unit, int port, bcm_trans_ptr_t **ptr);

extern int nh_tx_src_mod_port_set(int unit, int port,
                                  int src_mod, int src_port);
extern int nh_tx_src_mod_port_get(int unit, int port,
                                  int *src_mod, int *src_port);

extern int nh_tx(int unit,
                 int port,
                 uint8 *pkt_buf,
                 int len,
                 int cos,
                 int vlan,
                 uint16 pkt_type,
                 uint32 ct_flags,
                 nh_tx_cb_f callback,
                 void *cookie);

extern void nh_pkt_setup(bcm_pkt_t *pkt, int cos, uint16 vlan,
                         uint16 pkt_type, uint32 ct_flags);
extern void nh_pkt_local_setup(bcm_pkt_t *pkt, int unit, int port);
extern void nh_pkt_final_setup(bcm_pkt_t *pkt, int unit, int port);
extern int nh_pkt_tx(bcm_pkt_t *pkt, nh_tx_cb_f callback, void *cookie);

extern int nh_tx_pkt_recognize(uint8 *pkt_buf, uint16 *pkt_type);

extern int nh_tx_local_mac_set(const bcm_mac_t new_mac);
extern int nh_tx_local_mac_get(bcm_mac_t *local_mac);

extern int nh_tx_snap_set(const bcm_mac_t new_mac, uint16 new_type,
                          uint16 new_local_type);
extern int nh_tx_snap_get(bcm_mac_t snap_mac, uint16 *snap_type,
                          uint16 *local_type);

extern int nh_tx_reset(int reset_to_defaults);

extern int nh_tx_unknown_modid_get(int *modid);
extern int nh_tx_unknown_modid_set(int modid);

#ifndef NH_TX_DEST_MAC_DEFAULT
#define NH_TX_DEST_MAC_DEFAULT      {0x00, 0x10, 0x18, 0xff, 0xff, 0xff}
#endif
#ifndef NH_TX_SNAP_MAC_DEFAULT
#define NH_TX_SNAP_MAC_DEFAULT      SHARED_BRCM_NTSW_SNAP_MAC
#endif
#ifndef NH_TX_SNAP_TYPE_DEFAULT
#define NH_TX_SNAP_TYPE_DEFAULT     SHARED_BRCM_NTSW_SNAP_TYPE
#endif
#define NH_TX_LOCAL_PKT_TYPE        SHARED_PKT_TYPE_NH_TX
#ifndef NH_TX_DEST_PORT_DEFAULT
#define NH_TX_DEST_PORT_DEFAULT     0
#endif
#ifndef NH_TX_DEST_MOD_DEFAULT
#define NH_TX_DEST_MOD_DEFAULT      0
#endif
#ifndef NH_TX_SRC_PORT_DEFAULT
#define NH_TX_SRC_PORT_DEFAULT      0
#endif
#ifndef NH_TX_SRC_MOD_DEFAULT
#define NH_TX_SRC_MOD_DEFAULT       31
#endif
#ifndef NH_TX_NUM_PKTS_DEFAULT
#define NH_TX_NUM_PKTS_DEFAULT      10
#endif


#endif /* _CPUTRANS_NH_TX_H_ */
