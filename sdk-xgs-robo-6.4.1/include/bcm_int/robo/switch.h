/*
 * $Id: switch.h,v 1.10 Broadcom SDK $
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
 * This file contains internal definitions for switch module to the BCM library.
 */

#ifndef _BCM_INT_SWITCH_H
#define _BCM_INT_SWITCH_H

#define _BCM_SWITCH_DOS_COMMON_GROUP 0x1
#define _BCM_SWITCH_DOS_TCP_GROUP    0x2

/* TB's new design on IGMP/MLD snnoping mode setting definition.
 * The difference from previous ROBO device for IGMP and MLD are :
 *  - IGMP and MLD can only be set together on TB.
 *  - IGMP/MLD snooping can be disable on TB.
 *
 *  Assign the symbol definition as general logical type for further usage.
 */
#define  _BCM_IGMPMLD_MODE_NONE          0
#define  _BCM_IGMPMLD_MODE_CPU_TRAP      1
#define  _BCM_IGMPMLD_MODE_CPU_SNOOP     2
#define  _BCM_IGMPMLD_MODE_MAXID        _BCM_IGMPMLD_MODE_CPU_SNOOP


#define _BCM_SWITCH_IGMP_MLD_TYPES(type)    \
            (((type) == bcmSwitchIgmpPktToCpu) || \
            ((type) == bcmSwitchIgmpPktDrop) || \
            ((type) == bcmSwitchIgmpQueryToCpu) || \
            ((type) == bcmSwitchIgmpQueryDrop) || \
            ((type) == bcmSwitchIgmpReportLeaveToCpu) || \
            ((type) == bcmSwitchIgmpReportLeaveDrop) || \
            ((type) == bcmSwitchIgmpUnknownToCpu) || \
            ((type) == bcmSwitchIgmpUnknownDrop) || \
            ((type) == bcmSwitchMldPktToCpu) || \
            ((type) == bcmSwitchMldPktDrop) || \
            ((type) == bcmSwitchMldQueryToCpu) || \
            ((type) == bcmSwitchMldQueryDrop) || \
            ((type) == bcmSwitchMldReportDoneToCpu) || \
            ((type) == bcmSwitchMldReportDoneDrop) || \
            ((type) == bcmSwitchIgmpQueryFlood) || \
            ((type) == bcmSwitchIgmpReportLeaveFlood) || \
            ((type) == bcmSwitchIgmpUnknownFlood) || \
            ((type) == bcmSwitchMldQueryFlood) || \
            ((type) == bcmSwitchMldReportDoneFlood))

extern int _bcm_robo_switch_init(int unit);

#endif  /* !_BCM_INT_SWITCH_H */

