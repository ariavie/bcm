/*
 * $Id: tx.h,v 1.5 Broadcom SDK $
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
 * File:        tx.h
 * Purpose:     
 */

#ifndef   _BCM_INT_TX_H_
#define   _BCM_INT_TX_H_

#include <bcm/pkt.h>

/* This is all the extra information associated with a DV for RX */
typedef struct tx_dv_info_s {
    volatile bcm_pkt_t *pkt[SOC_DV_PKTS_MAX];
    int                 pkt_count;
    volatile uint8      pkt_done_cnt;
    bcm_pkt_cb_f        chain_done_cb;
    void               *cookie;
} tx_dv_info_t;

/*
 * TX DV INFO structure maintains information about a TX chain.
 * This includes:  Per packet callbacks and the chain done callback.
 *
 * Packets are processed in the order they return if any packet in
 * the chain requires a call back.  If none does, then only a chain
 * done callback is setup/made.
 *
 * A "current" packet is maintained in the order the packet's are
 * set up.  The packet-done interrupts will occur in this order.
 *
 * TX DV info macros:
 *     
 *    TX_INFO_SET(dv, val)      Assign the TX info ptr for a DV
 *    TX_INFO(dv)               Access the TX info ptr of a DV
 *    TX_INFO_PKT_ADD(dv)       Add pkt ptr to info struct; advance count
 *    TX_INFO_CUR_PKT(dv)       Pointer to "current" pkt done being processed
 *    TX_INFO_PKT_MARK_DONE(dv) Advances "current" packet.
 */

#define TX_INFO_SET(dv, val)      ((dv)->dv_public1.ptr = val)
#define TX_INFO(dv)               ((tx_dv_info_t *)((dv)->dv_public1.ptr))
#define TX_INFO_PKT_ADD(dv, pkt) \
    TX_INFO(dv)->pkt[TX_INFO(dv)->pkt_count++] = (pkt)
#define TX_INFO_CUR_PKT(dv)       (TX_INFO(dv)->pkt[TX_INFO(dv)->pkt_done_cnt])
#define TX_INFO_PKT_MARK_DONE(dv) ((TX_INFO(dv)->pkt_done_cnt)++)

#define TX_DV_NEXT(dv)            ((eth_dv_t *)((dv)->dv_public2.ptr))
#define TX_DV_NEXT_SET(dv, dv_next) \
    ((dv)->dv_public2.ptr = (void *)(dv_next))

/*
 * Define:  TX_EXTRA_DCB_COUNT:
 *      The number of extra DCBs needed to accomodate portions of the
 * packet such as the HiGig header, SL tag, breaking up a block due
 * to VLAN tag insertion, etc.
 */
#define TX_EXTRA_DCB_COUNT 3

/* Service Provider TPID Definition : 
 *  1. 802.1q : 0x8100 (802.1Q defined already)
 *  2. Device default : 0x9100/0x9200
 *  3. 802.1ad recommend : 0x88A8
 */
#define SP_TPID_CHIP_DEFAULT1   0x9100
#define SP_TPID_CHIP_DEFAULT2   0x9200
#define SP_TPID_1AD_DEFAULT     0x88A8

#define SP_TPID_CHECK(_in_tag,_tpid)     \
            (((_tpid) != 0) && (((uint16)(_in_tag)) == (_tpid)))
#define SP_TPID_DEFAULT_CHECK(_in_tag)       \
            (SP_TPID_CHECK((_in_tag), SP_TPID_CHIP_DEFAULT1) ||   \
                SP_TPID_CHECK((_in_tag), SP_TPID_CHIP_DEFAULT2) ||   \
                SP_TPID_CHECK((_in_tag), SP_TPID_1AD_DEFAULT))

extern int bcm_robo_tx_deinit(int unit);

#endif /* _BCM_INT_TX_H_ */
