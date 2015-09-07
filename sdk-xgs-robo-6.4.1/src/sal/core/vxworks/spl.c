/*
 * $Id: spl.c,v 1.7 Broadcom SDK $
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
 * File: 	spl.c
 * Purpose:	Interrupt Blocking
 */

#include <vxWorks.h>
#include <sysLib.h>
#include <intLib.h>

static int _int_locked = 0;

/*
 * Function:
 *	sal_spl
 * Purpose:
 *	Set interrupt level.
 * Parameters:
 *	level - interrupt level to set.
 * Returns:
 *	Value of interrupt level in effect prior to the call.
 * Notes:
 *	Used most often to restore interrupts blocked by sal_splhi.
 */

int
sal_spl(int level)
{
    int	il;
    _int_locked--;
#if VX_VERSION >= 64
    intUnlock(level);
    il = 0;
#else
    il = intUnlock(level);
#endif
    return il;
}

/*
 * Function:
 *	sal_splhi
 * Purpose:
 *	Set interrupt mask to highest level.
 * Parameters:
 *	None
 * Returns:
 *	Value of interrupt level in effect prior to the call.
 */

int
sal_splhi(void)
{
    int il;
    il = intLock();
    _int_locked++;
    return (il);
}

/*
 * Function:
 *	sal_int_locked
 * Purpose:
 *	Return TRUE if interrupts have been blocked via splhi.
 * Parameters:
 *	None
 * Returns:
 *	Boolean
 */

int
sal_int_locked(void)
{
    return _int_locked;
}

/*
 * Function:
 *	sal_int_context
 * Purpose:
 *	Return TRUE if running in interrupt context.
 * Parameters:
 *	None
 * Returns:
 *	Boolean
 */

int
sal_int_context(void)
{
    return INT_CONTEXT();
}

/*
 * Function:
 *	sal_no_sleep
 * Purpose:
 *	Return TRUE if current context cannot sleep.
 * Parameters:
 *	None
 * Returns:
 *	Boolean
 */

int
sal_no_sleep(void)
{
    return sal_int_locked();
}
