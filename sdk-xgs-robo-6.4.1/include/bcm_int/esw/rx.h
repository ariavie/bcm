/*
 * $Id: rx.h,v 1.35 Broadcom SDK $
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
 * File:        rx.h
 * Purpose:     Internal structures and definitions for RX module
 */

#ifndef   _BCM_RX_H_
#define   _BCM_RX_H_

#include <sdk_config.h>
#include <soc/dma.h>
#include <bcm/error.h>
#include <bcm/rx.h>
#include <bcm_int/common/rx.h>

/* This is the tunnel header description */
#define TUNNEL_RX_HEADER_LEN_OFS             0    /* uint16 */
#define TUNNEL_RX_HEADER_REASON_OFS          2    /* uint32 */
#define TUNNEL_RX_HEADER_RX_UNIT_OFS         6
#define TUNNEL_RX_HEADER_RX_PORT_OFS         7
#define TUNNEL_RX_HEADER_RX_CPU_COS_OFS      8
#define TUNNEL_RX_HEADER_RX_UNTAGGED_OFS     9
#define TUNNEL_RX_HEADER_COS_OFS             10
#define TUNNEL_RX_HEADER_PRIO_INT_OFS        11
#define TUNNEL_RX_HEADER_SRC_PORT_OFS        12
#define TUNNEL_RX_HEADER_SRC_MOD_OFS         13
#define TUNNEL_RX_HEADER_DEST_PORT_OFS       14
#define TUNNEL_RX_HEADER_DEST_MOD_OFS        15
#define TUNNEL_RX_HEADER_OPCODE_OFS          16
#define TUNNEL_RX_HEADER_BYTES               17

extern int _bcm_rx_shutdown(int unit);
extern int bcm_esw_rx_deinit(int unit);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_rx_sync(int unit);
#endif

#endif  /* !_BCM_RX_H_ */
