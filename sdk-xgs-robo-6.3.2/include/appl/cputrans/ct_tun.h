/*
 * $Id: ct_tun.h 1.4 Broadcom SDK $
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
 * File:        ct_tun.h
 * Purpose:     
 */

#ifndef   _CPUTRANS_CT_TUN_H_
#define   _CPUTRANS_CT_TUN_H_

#include <bcm/rx.h>

#include <appl/cpudb/cpudb.h>
#include <appl/cputrans/atp.h>

#if defined(BCM_RPC_SUPPORT)

/*
 * Transmit function to call to tunnel data.  REQUIRED.  pkt_data does not
 * include any transport header (eg no CPUTRANS_HEADER).  But other
 * encapsulation data (source port, original COS, etc) is included.
 */

typedef void (*ct_tunnel_done_cb_f)(uint8 *pkt_data, void *cookie, int rv);

/*
 * Tunnel filter.  Optional.  If present (set), this function is
 * called before tunnelling the packet.
 */

typedef bcm_cpu_tunnel_mode_t (*ct_rx_tunnel_filter_f)(int unit,
                                                       bcm_pkt_t *pkt);

extern int ct_rx_tunnel_filter_set(ct_rx_tunnel_filter_f t_cb);
extern int ct_rx_tunnel_filter_get(ct_rx_tunnel_filter_f *t_cb);

typedef int (*ct_rx_tunnel_direct_f)(int unit, bcm_pkt_t *pkt,
                                     cpudb_key_t *cpu);

extern int ct_rx_tunnel_direct_set(ct_rx_tunnel_direct_f dif_fn);
extern int ct_rx_tunnel_direct_get(ct_rx_tunnel_direct_f *dif_fn);

extern int ct_rx_tunnel_mode_default_set(int mode);
extern bcm_cpu_tunnel_mode_t ct_rx_tunnel_mode_default_get(void);
extern int ct_rx_tunnel_priority_set(int priority);
extern int ct_rx_tunnel_priority_get(void);

extern void ct_rx_tunnelled_pkt_handler(cpudb_key_t src_key,
                                        uint8 *payload, int len);

extern bcm_cpu_tunnel_mode_t ct_rx_tunnel_check(int unit, bcm_pkt_t *pkt,
                                                int *no_ack, int *check_cpu,
                                                cpudb_key_t *cpu);

extern int ct_rx_tunnel_header_bytes_get(void);
extern uint8 *ct_rx_tunnel_pkt_pack(bcm_pkt_t *pkt, uint8 *buf);
extern int ct_rx_tunnel_pkt_unpack(uint8 *pkt_data, int pkt_len,
                                   bcm_pkt_t *pkt, uint8 **payload_start,
                                   int *payload_len);

/****************************************************************
 *
 * Packet TX tunnelling code
 *
 ****************************************************************/

extern int ct_tx_tunnel_setup(void);
extern int ct_tx_tunnel_stop(void);

extern int ct_tx_tunnel(bcm_pkt_t *pkt,
                        int dest_unit,   /* BCM unit */
                        int remote_port,
                        uint32 flags,  /* See include/bcm/pkt.h */
                        bcm_cpu_tunnel_mode_t mode);

extern int ct_tx_tunnel_forward(uint8 *pkt_data, int len);

#endif /* BCM_RPC_SUPPORT */

#endif /* !_CPUTRANS_CT_TUN_H_ */
