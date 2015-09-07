/*
 * $Id: rlink.h 1.5 Broadcom SDK $
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
 * Remote Linkscan
 */

#ifndef	_BCM_INT_RLINK_H
#define	_BCM_INT_RLINK_H

#include <sdk_config.h>
#include <bcm/pkt.h>
#include <bcm/l2.h>

#ifdef	BCM_RPC_SUPPORT

#ifndef BCM_RLINK_RX_REMOTE_MAX_DEFAULT
#define BCM_RLINK_RX_REMOTE_MAX_DEFAULT {10, 10, 10, 20, 20, 20, 50, 80}
#endif

#ifndef BCM_RLINK_L2_REMOTE_MAX_DEFAULT
#define BCM_RLINK_L2_REMOTE_MAX_DEFAULT 50
#endif

typedef int (*bcm_rlink_l2_cb_f)(int unit, bcm_l2_addr_t *l2addr, int insert);

extern int	bcm_rlink_start(void);
extern int	bcm_rlink_stop(void);

extern void     bcm_rlink_device_clear(int unit);
extern int      bcm_rlink_device_detach(int unit);

extern void     bcm_rlink_server_clear(void);
extern void     bcm_rlink_client_clear(void);
extern void     bcm_rlink_clear(void);

extern int bcm_rlink_rx_connect(int unit);
extern int bcm_rlink_rx_disconnect(int unit);

extern void bcm_rlink_l2_cb_set(bcm_rlink_l2_cb_f l2_cb);
extern void bcm_rlink_l2_cb_get(bcm_rlink_l2_cb_f *l2_cb);

extern void bcm_rlink_l2_native_only_set(int native_only);
extern void bcm_rlink_l2_native_only_get(int *native_only);

extern int bcm_rlink_traverse_message(bcm_pkt_t *rx_pkt,
                                      uint8 *data_in, int len_in,
                                      uint8 *data_out, int len_out,
                                      int *len_out_actual);

extern int bcm_rlink_traverse_init(void);
extern int bcm_rlink_traverse_deinit(void);
extern int bcm_rlink_traverse_client_init(void);
extern int bcm_rlink_traverse_client_deinit(void);
extern int bcm_rlink_traverse_server_init(void);
extern int bcm_rlink_traverse_server_deinit(void);
extern int bcm_rlink_traverse_server_clear(void);
extern int bcm_rlink_traverse_client_clear(void);

#endif	/* BCM_RPC_SUPPORT */
#endif	/* _BCM_INT_RLINK_H */
