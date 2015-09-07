/*
 * $Id: robo_dino8.h,v 1.1 Broadcom SDK $
 *
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
 
#ifndef _ROBO_DINO8_H
#define _ROBO_DINO8_H


#define DRV_DINO8_MCAST_GROUP_NUM         256
#define DRV_DINO8_AGE_TIMER_MAX           1048575
#define DRV_DINO8_TRUNK_GROUP_NUM         4
#define DRV_DINO8_TRUNK_MAX_PORT_NUM      2
#define DRV_DINO8_MSTP_GROUP_NUM          1
#define DRV_DINO8_SEC_MAC_NUM_PER_PORT    24   /* only one EAP_DA/port allowed */
#define DRV_DINO8_COS_QUEUE_MAX_WEIGHT_VALUE  55
#define DRV_DINO8_AUTH_SUPPORT_PBMP       0x00000000
#define DRV_DINO8_RATE_CONTROL_SUPPORT_PBMP 0x0000000ff
#define DRV_DINO8_VLAN_ENTRY_NUM  4095
#define DRV_DINO8_BPDU_NUM    1
#define DRV_DINO8_AUTH_SEC_MODE DRV_SECURITY_VIOLATION_NONE

/* ------ DINO8 Storm Control related definition ------- */
#define DINO8_STORM_SUPPRESSION_DLF_MASK    0x20
#define DINO8_STORM_SUPPRESSION_BPDU_MASK   0x10
#define DINO8_STORM_SUPPRESSION_BROADCAST_MASK  0x0c
#define DINO8_STORM_SUPPRESSION_MULTICAST_MASK  0x02
#define DINO8_STORM_SUPPRESSION_UNICAST_MASK    0x01

#define DINO8_STORM_CONTROL_PKT_MASK (DRV_STORM_CONTROL_BCAST | \
                                        DRV_STORM_CONTROL_MCAST | \
                                        DRV_STORM_CONTROL_DLF)
#endif
