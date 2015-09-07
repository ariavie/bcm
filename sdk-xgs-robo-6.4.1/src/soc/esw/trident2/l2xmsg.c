/*
 * $Id: l2xmsg.c,v 1.1 Broadcom SDK $
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
 * File:	l2xmsg.c
 * Purpose:	Provide needed routines for L2 overflow.
 */

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/time.h>
#include <shared/bsl.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#include <soc/ism_hash.h>
#include <soc/trident2.h>

#ifdef BCM_TRIDENT2_SUPPORT

int
soc_td2_l2_overflow_disable(int unit)
{
    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, IL2LU_INTR_ENABLEr, REG_PORT_ANY,
                                L2_LEARN_INSERT_FAILUREf, 0));
    /* Mark as inactive */
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->l2_overflow_active = FALSE;
    SOC_CONTROL_UNLOCK(unit);
    /* But don't disable interrupt as its shared by various IL2LU events */
    return SOC_E_NONE;
}

int
soc_td2_l2_overflow_enable(int unit)
{
    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, IL2LU_INTR_ENABLEr, REG_PORT_ANY,
                                L2_LEARN_INSERT_FAILUREf, 1));
    /* Mark as active */
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->l2_overflow_active = TRUE;
    SOC_CONTROL_UNLOCK(unit);
    /* Enable interrupt */
    (void)soc_cmicm_intr1_enable(unit, _SOC_TD2_L2LU_INTR_MASK);
    return SOC_E_NONE;
}

int
soc_td2_l2_overflow_fill(int unit, uint8 zeros_or_ones)
{
    l2_learn_insert_failure_entry_t entry;
    
    if (zeros_or_ones) {
        sal_memset(&entry, 0xffffffff, sizeof(entry));
        SOC_IF_ERROR_RETURN
            (WRITE_L2_LEARN_INSERT_FAILUREm(unit, COPYNO_ALL, 0, &entry));
    } else {
        SOC_IF_ERROR_RETURN
            (soc_mem_clear(unit, L2_LEARN_INSERT_FAILUREm, COPYNO_ALL, FALSE));
    }    
    return SOC_E_NONE;
}

void
soc_td2_l2_overflow_interrupt_handler(int unit)
{
    l2x_entry_t l2x_entry;

    if (!SOC_CONTROL(unit)->l2_overflow_active) {
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "Received L2 overflow event with no app handler or"   \
                              " processing inactive !!\n")));
    }
    if (soc_td2_l2_overflow_disable(unit)) {
        return;
    }

    if (READ_L2_LEARN_INSERT_FAILUREm(unit, COPYNO_ALL, 0, &l2x_entry)) {
        return;
    }

    /* Callback */
	soc_l2x_callback(unit, SOC_L2X_ENTRY_OVERFLOW, NULL, &l2x_entry);    
}

int
soc_td2_l2_overflow_stop(int unit)
{
    if (!SOC_CONTROL(unit)->l2_overflow_enable) {
        return SOC_E_NONE;
    }
    SOC_IF_ERROR_RETURN(soc_td2_l2_overflow_fill(unit, 1));
    SOC_IF_ERROR_RETURN(soc_td2_l2_overflow_disable(unit));
    return SOC_E_NONE;
}

int
soc_td2_l2_overflow_start(int unit)
{
    if (!SOC_CONTROL(unit)->l2_overflow_enable) {
        return SOC_E_NONE;
    }
    if (SOC_CONTROL(unit)->l2_overflow_active) {
        return SOC_E_NONE;
    }
    SOC_IF_ERROR_RETURN(soc_td2_l2_overflow_fill(unit, 0));
    SOC_IF_ERROR_RETURN(soc_td2_l2_overflow_enable(unit));
    return SOC_E_NONE;
}

#endif /* BCM_TRIUMPH3_SUPPORT */
