/* 
 * $Id: traverse.h,v 1.2.10.1 Broadcom SDK $
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
 * File:        traverse.h
 * Purpose:     RLINK traverse client/server/control support functions
 */

#ifndef   _TRAVERSE_H_
#define   _TRAVERSE_H_

#include <bcm/types.h>
#include <bcm/pkt.h>
#include <appl/cpudb/cpudb.h>
#include "rlink.h"

typedef struct bcm_rlink_traverse_data_s {
    int                             unit;       /* traverse unit */
    uint32                          c_id;       /* client ID */
    uint32                          s_id;       /* server ID */
    bcm_pkt_t                       *rx_pkt;
    uint8                           *rx_buf;    /* receive buffer */
    uint8                           *rx_ptr;    /* rx buffer current */
    int                             rx_len;     /* rx buf length */
    uint8                           *tx_buf;    /* tx buffer */
    uint8                           *tx_ptr;    /* tx buffer current */
    int                             tx_len;     /* tx buffer length */
    void                            *parent;    /* parent client/server */
} bcm_rlink_traverse_data_t;

/*
  Called by a client traverse implementation to start a remote traverse.
  Initializes and registers a traverse request record.
  Does *not* send any RLINK messages to the RLINK server.
 */
extern int bcm_rlink_traverse_request_start(int unit,
                                            bcm_rlink_traverse_data_t *req,
                                            uint32 *key);

/*
  Gets a traverse reply. If there are no rxeplies, and the remote
  traverse has not started, or signaled completion, then issue a request
  for traverse data and wait for a response.
 */
extern int bcm_rlink_traverse_reply_get(int unit,
                                        bcm_rlink_traverse_data_t *req);

/*
  Indicate that this request is done. If the traverse server has not
  indicated completion, then issue a done message.
 */
extern int bcm_rlink_traverse_request_done(int unit,
                                           int rv,
                                           bcm_rlink_traverse_data_t *data);


/* Server side implementation APIs */

/*
  Check to see if there's enough room for a reply. If not,
  send out current data and wait for a resonse.
 */
extern int bcm_rlink_traverse_reply_check(bcm_rlink_traverse_data_t *data,
                                          int size);

/*
  Flush any outstanding callback data.
 */
extern int bcm_rlink_traverse_reply_done(bcm_rlink_traverse_data_t *data,
                                         int rv);

/*
  Handle a traverse request
 */
extern int bcm_rlink_traverse_request(rlink_type_t type,
                                      cpudb_key_t cpu,
                                      bcm_pkt_t *pkt, uint8 *data, int len);

extern int bcm_rlink_traverse_device_clear(int unit);
#endif /* _TRAVERSE_H_ */
