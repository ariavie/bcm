/*
 * $Id: dma.h,v 1.6 Broadcom SDK $
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
 * File:        dma.h
 * Purpose:     
 */

#ifndef   _PCID_DMA_H_
#define   _PCID_DMA_H_

#include <soc/defs.h>
#include <soc/dcb.h>
#include <sys/types.h>

#include "pcid.h"

extern uint32 pcid_dcb_fetch(pcid_info_t *pcid_info, uint32 addr, dcb_t *dcb);
extern uint32 pcid_dcb_store(pcid_info_t *pcid_info, uint32 addr, dcb_t *dcb);
extern void pcid_dma_tx_start(pcid_info_t *pcid_info, int ch);
extern void pcid_dma_cmicm_tx_start(pcid_info_t *pcid_info, int ch);
extern void pcid_dma_rx_start(pcid_info_t *pcid_info, int ch);
extern void pcid_dma_cmicm_rx_start(pcid_info_t *pcid_info, int ch);
extern void pcid_dma_rx_check(pcid_info_t *pcid_info, int chan);
extern void pcid_dma_cmicm_rx_check(pcid_info_t *pcid_info, int chan);
extern void pcid_dma_start_chan(pcid_info_t *pcid_info, int ch);
extern void pcid_dma_stat_write(pcid_info_t *pcid_info, uint32 value);
extern void pcid_dma_ctrl_write(pcid_info_t *pcid_info, uint32 value);
extern void pcid_dma_cmicm_ctrl_write(pcid_info_t *pcid_info, uint32 reg, uint32 value);
extern void pcid_dma_stop_all(pcid_info_t *pcid_info);

/* Call back function type for tx */
typedef int (*pcid_tx_cb_f)(pcid_info_t *pcid_info, uint8 *pkt_data, int eop);
extern void pcid_dma_tx_cb_set(pcid_info_t *pcid_info, pcid_tx_cb_f fun);
void _pcid_loop_tx_cb(pcid_info_t *pcid_info, dcb_t *dcb, int chan);

extern uint32
pcid_reg_read(pcid_info_t *pcid_info, uint32 addr);
extern void
pcid_reg_write(pcid_info_t *pcid_info, uint32 addr, uint32 value);
extern void
pcid_reg_or_write(pcid_info_t* pcid_info, uint32 addr, uint32 or_value);
extern void
pcid_reg_and_write(pcid_info_t* pcid_info, uint32 addr, uint32 and_value);

#endif /* _PCID_DMA_H_ */
