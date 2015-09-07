/*
 * $Id: phymod_debug.h,v 1.1.2.4 Broadcom SDK $
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

#ifndef __PHYMOD_DEBUG_H__
#define __PHYMOD_DEBUG_H__

#include <phymod/phymod.h>

extern uint32_t phymod_dbg_mask;
extern uint32_t phymod_dbg_addr;
extern uint32_t phymod_dbg_lane;

extern int
phymod_debug_check(uint32_t flags, const phymod_access_t *pa);

/*
 * The PHYMOD_VDBG macro can be used by PHYMOD driver implementations
 * to print verbose debug message which can be filtered by a mask
 * and a PHY address parameter.
 *
 * Examples of the call syntax:
 *
 *   PHYMOD_VDBG(DBG_SPD, pa, ("link status %d\n", link));
 *   PHYMOD_VDBG(DBG_AN, NULL, ("autoneg status 0x%x\n", an_stat));
 *
 * Please note the use of parentheses.
 *
 * The DBG_SPD and DBG_AN parameters should each be a bit mask for
 * filtering speed-related messages (see #defines below).
 *
 * The 'pa' parameter is a pointer to a phymod_access_t from which the
 * PHY address is pulled. If this pointer is NULL, the message will be
 * shown for any PHY address.
 */

/*
 * Bits [31:20] are reserved for common debug output flags.  A driver
 * may use bits [19:0] for implementation-specific debug output.
 */

#define DBG_AN          (1L << 26) /* Auto-negotiation */
#define DBG_SPD         (1L << 27) /* Speed */
#define DBG_LNK         (1L << 28) /* Link */
#define DBG_CFG         (1L << 29) /* Configuration */
#define DBG_ACC         (1L << 30) /* Register access */
#define DBG_DRV         (1L << 31) /* Driver code outside PHYMOD */


#define PHYMOD_VDBG(flags_, pa_, stuff_) \
    if (phymod_debug_check(flags_, pa_)) \
        PHYMOD_DEBUG_ERROR(stuff_)

#endif /* __PHYMOD_DEBUG_H__ */
