/*
 * $Id: rcpu.h 1.22 Broadcom SDK $
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
 */

#ifndef _BCM_INT_RCPU_H
#define _BCM_INT_RCPU_H

#include <soc/rcpu.h>

#ifdef INCLUDE_RCPU

extern int _bcm_esw_rcpu_init(int unit);
extern int _bcm_esw_rcpu_deinit(int unit);
extern int _bcm_rcpu_tx(int unit, bcm_pkt_t *pkt, void *cookie);
extern int _bcm_rcpu_tx_list(int unit, bcm_pkt_t *pkt,
                              bcm_pkt_cb_f all_done_cb, void *cookie);
extern int _bcm_rcpu_tx_array(int unit, bcm_pkt_t **pkt, int count, 
                              bcm_pkt_cb_f all_done_cb, void *cookie);
extern int 
_bcm_esw_rcpu_tocpu_rx(int unit, uint8 *pkt, int pkt_len, rcpu_encap_info_t *info);

typedef struct bcm_rcpu_cfg_s {
    soc_rcpu_cfg_t rcpu_cfg;
    soc_pbmp_t     pbmp;       /* remote CMIC RX port bitmap       */
    uint32         dot1pri[8]; /* 802.1p priority to be used for each COS */
    uint32         mh_tc[8];   /* Module header traffic class to be used for each COS */
    uint32         cpu_tc[8];  /* CPU traffic class to be used for each COS */      
} bcm_rcpu_cfg_t;

#define RCPU_MAX_BUF_LEN        (RX_PKT_SIZE_DFLT + 32 + 64)

#define BCM_RCPU_CFG(_unit)    (&BCM_RCPU_CONTROL(_unit)->cfg)

#define BCM_RCPU_CFG_LMAC(_unit)       \
            BCM_RCPU_CONTROL(_unit)->cfg.rcpu_cfg.dst_mac
#define BCM_RCPU_CFG_SRC_MAC(_unit)       \
            BCM_RCPU_CONTROL(_unit)->cfg.rcpu_cfg.src_mac

#define BCM_RCPU_F_INITED       0x10000

typedef struct bcm_rcpu_control_s {
    bcm_rcpu_cfg_t      cfg;
    sal_mutex_t         lock;
    uint32              flags;
} _bcm_rcpu_control_t;

#define BCM_RCPU_MAX_UNITS       BCM_UNITS_MAX 
extern _bcm_rcpu_control_t      *_bcm_rcpu_control[BCM_RCPU_MAX_UNITS];

#define BCM_RCPU_CONTROL(_unit)     _bcm_rcpu_control[_unit]
#define BCM_RCPU_UNIT_VALID(_unit)  ((_unit) >= 0 && \
				     (_unit) < BCM_RCPU_MAX_UNITS && \
				     BCM_RCPU_CONTROL(_unit) != 0)

#define	RCPU_UNIT_LOCK(unit)   sal_mutex_take(BCM_RCPU_CONTROL((unit))->lock, \
                                               sal_mutex_FOREVER)
#define	RCPU_UNIT_UNLOCK(unit) sal_mutex_give(BCM_RCPU_CONTROL((unit))->lock)

#define BCM_RCPU_RX_PRIO            BCM_RX_PRIO_MAX

#endif /* INCLUDE_RCPU */

#endif /* _BCM_INT_RCPU_H */

