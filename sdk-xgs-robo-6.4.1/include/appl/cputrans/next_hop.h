/*
 * $Id: next_hop.h,v 1.6 Broadcom SDK $
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
 * File:        next_hop.h
 * Purpose:     Header file for next hop transport
 */

#ifndef   _CPUTRANS_NEXT_HOP_H_
#define   _CPUTRANS_NEXT_HOP_H_

#include <sdk_config.h>
#include <bcm/rx.h>

#include <appl/cputrans/nh_tx.h>
#include <appl/cpudb/cpudb.h>

#ifndef NEXT_HOP_MTU_DEFAULT
#define NEXT_HOP_MTU_DEFAULT         2048
#endif

/* Max number of supported callbacks */
#ifndef NEXT_HOP_CALLBACK_MAX
#define NEXT_HOP_CALLBACK_MAX        10
#endif

#define NEXT_HOP_PKT_TYPE            SHARED_PKT_TYPE_NEXT_HOP

/*
 * Keeping track of sequence numbers per CPU:
 *     SEQ_NUM_TRACK:  Number of sequence numbers to track per CPU
 *     SEQ_NUM_MAX:    Max sequence number to put in a packet
 *
 * These parameters are very important to the inner workings of
 * next hop.  The NO_TRACK is the number of sequence numbers recorded
 * per CPU.  If a copy of a packet is received, it is discarded b/c
 * its sequence number is in this list.  If the list is not long enough,
 * a packet could be incorrectly forwarded and sent up the stack.
 *
 * The trade off is the amount of space and time it requires to check
 * if an incoming packet has been seen before.
 *
 * The SEQ_NUM_MAX must be large enough so that rollover cannot result
 * in a new packet being mistaken for an old packet.
 */

#ifndef NEXT_HOP_SEQ_NUM_TRACK
#define NEXT_HOP_SEQ_NUM_TRACK                  16
#endif
#ifndef NEXT_HOP_SEQ_NUM_MAX
#define NEXT_HOP_SEQ_NUM_MAX                    (8 * 1024)
#endif

#ifndef NEXT_HOP_RX_QUEUE_SIZE
#define NEXT_HOP_RX_QUEUE_SIZE                  32
#endif
#ifndef NEXT_HOP_TX_QUEUE_SIZE
#define NEXT_HOP_TX_QUEUE_SIZE                  32
#endif

#ifndef NEXT_HOP_RX_PRIORITY_DEFAULT
#define NEXT_HOP_RX_PRIORITY_DEFAULT            105	/* bit more than atp */
#endif
#ifndef NEXT_HOP_THREAD_PRIORITY_DEFAULT
#define NEXT_HOP_THREAD_PRIORITY_DEFAULT        100
#endif
#ifndef NEXT_HOP_TRANS_PTR_DEFAULT
#define NEXT_HOP_TRANS_PTR_DEFAULT              (&bcm_trans_ptr)
#endif

/*
 * Typedef:
 *      next_hop_rx_callback_f
 * Purpose:
 *      Next hop broadcast application packet handler callback.
 * Parameters:
 *      src_key       - The source key of the originating CPU
 *      rx_unit       - Unit on which packet received
 *      rx_port       - Port on which packet received
 *      pkt_buf       - Packet buffer including all headers
 *      len           - Total length of pkt in bytes
 * Returns:
 *      BCM_RX_HANDLED       - Done with packet; no more processing
 *      BCM_RX_HANDLED_OWNED - Done with packet; do not free pkt buffer
 *      Otherwise, continue processing.
 * Notes:
 *      Use NEXT_HOP_PAYLOAD_OFFSET to index to start of data.
 *      src_key should be treated as const.
 */

typedef bcm_rx_t (*next_hop_rx_callback_f)(cpudb_key_t src_key, int mplx_num,
                                           int rx_unit, int rx_port,
                                           uint8 *pkt_buf, int len,
                                           void *cookie);


/*
 * Typedef:
 *      next_hop_tx_callback_f
 * Purpose:
 *      Next hop broadcast transmit function callback 
 * Parameters:
 *      return_code    - Success or failure code of transmit, BCM_E_XXX
 *      pkt_buf        - The buffer that was transmitted.
 *      cookie         - Cookie passed with transmit call.
 * Notes:
 *      Use for async transmits.
 *      For all transmits, provide NEXT_HOP_PAYLOAD_OFFSET bytes at
 *      the start of the packet for lower layers.
 */
typedef void (*next_hop_tx_callback_f)(int return_code,
                                       uint8 *pkt_buf,
                                       void *cookie);

extern int next_hop_start(const cpudb_base_t *local_base);
extern int next_hop_update(const cpudb_base_t *local_base);
extern int next_hop_running(void);
extern int next_hop_config_set(bcm_trans_ptr_t *trans_ptr,
                               int thread_priority,
                               int rx_priority);
extern int next_hop_config_get(bcm_trans_ptr_t **trans_ptr,
                               int *thread_priority,
                               int *rx_priority);

extern int next_hop_stop(void);

extern int next_hop_port_add(int unit, int port, int duplex);
extern int next_hop_port_remove(int unit, int port);

extern int next_hop_num_ports_get(void);
extern int next_hop_port_get(int idx, int *unit, int *port, int *duplex);

extern int next_hop_max_entries_get(void);
extern const cpudb_key_t *next_hop_key_get(int idx);

extern bcm_pkt_t * next_hop_pkt_create(uint8 *pkt_buf,
                                       int len,
                                       int cos,
                                       int vlan,
                                       int seg_len,
                                       uint32 ct_flags,
                                       int mplx_num,
                                       const cpudb_key_t nh_dest_key,
                                       int *tot_segs,
                                       int *rvp);

extern int next_hop_pkt_update(bcm_pkt_t *pkt,
                               int mplx_num,
                               const cpudb_key_t nh_dest_key);

extern int next_hop_pkt_send(bcm_pkt_t *pkt,
                             next_hop_tx_callback_f callback,
                             void *cookie);

extern int next_hop_pkt_destroy(bcm_pkt_t *pkt);

extern int next_hop_buffer_init(uint8 *hdr_buf,
                                int mplx_num,
                                const cpudb_key_t nh_dest_key);

extern int next_hop_tx(uint8 *pkt_buf,
                       int len,
                       int cos,
                       int vlan,
                       int seg_len,
                       uint32 ct_flags,
                       int mplx_num,
                       const cpudb_key_t nh_dest_key,
                       next_hop_tx_callback_f callback,
                       void *cookie);

extern int next_hop_register(next_hop_rx_callback_f callback, void *cookie,
                             int mplx_num);
extern int next_hop_unregister(next_hop_rx_callback_f callback,
                               int mplx_num);

extern int next_hop_key_invalidate(cpudb_key_t key);

extern int next_hop_mtu_get(int *mtu);
extern int next_hop_mtu_set(int mtu);

extern int next_hop_lru_enable_get(void);
extern void next_hop_lru_enable_set(int lru);


extern int next_hop_cos_set(int cos);
extern int next_hop_cos_get(int *cos);
extern int next_hop_vlan_set(int vlan);
extern int next_hop_vlan_get(int *vlan);

extern int next_hop_queue_size_set(int rx_size, int tx_size);
extern int next_hop_queue_size_get(int *rx_size, int *tx_size);

#if defined(BROADCOM_DEBUG)
void next_hop_dump(void);
#endif  /* BROADCOM_DEBUG */

#endif /* _CPUTRANS_NEXT_HOP_H_ */
