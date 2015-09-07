/*
 * $Id: 9e57dd82c2b3fe625a70f917e3e917cb06bfc71e $
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
 * File:    protect.c
 *
 * Purpose: 
 *
 * Functions:
 *      _bcm_ptp_enter_critical
 *      _bcm_ptp_exit_critical
 *      _bcm_ptp_intr_context
 *      _bcm_ptp_mutex_create
 *      _bcm_ptp_mutex_destroy
 *      _bcm_ptp_mutex_take
 *      _bcm_ptp_mutex_give
 *      _bcm_ptp_sem_create
 *      _bcm_ptp_sem_destroy
 *      _bcm_ptp_sem_take
 *      _bcm_ptp_sem_give
 */

#if defined(INCLUDE_PTP)

/* ---------------------------------------------------------------------------------------------- */
/* ABSTRACT: Allow work around for broken SAL semaphore and mutex issues                          */
/* ---------------------------------------------------------------------------------------------- */

#include <sal/core/time.h>

#include <bcm_int/common/ptp.h>

/* ============================= */
/* Critical section functions    */
/* ============================= */

int
_bcm_ptp_enter_critical(void)
{
    return sal_splhi();
}

int
_bcm_ptp_exit_critical(int level)
{
    return sal_spl(level);
}

int
_bcm_ptp_intr_context(void)
{
    return sal_int_context();
}


/* ============================= */
/* Mutex functions               */
/* ============================= */

_bcm_ptp_mutex_t
_bcm_ptp_mutex_create(char *desc)
{
    return sal_mutex_create(desc);
}

void
_bcm_ptp_mutex_destroy(_bcm_ptp_mutex_t m)
{
    sal_mutex_destroy(m);
}

int
_bcm_ptp_mutex_take(_bcm_ptp_mutex_t m, int usec)
{
    int rv = -1;
    sal_usecs_t wait_time = sal_time_usecs() + usec;

    while ((rv != 0) && (int32)(wait_time - sal_time_usecs()) > 0) {
        rv = sal_mutex_take(m, wait_time - sal_time_usecs());
    }

    return rv;
}

int
_bcm_ptp_mutex_give(_bcm_ptp_mutex_t m)
{
    return sal_mutex_give(m);
}


/* ============================= */
/* Semaphore functions           */
/* ============================= */

_bcm_ptp_sem_t
_bcm_ptp_sem_create(char *desc, int binary, int initial_count)
{
    return sal_sem_create(desc, binary, initial_count);
}

void
_bcm_ptp_sem_destroy(_bcm_ptp_sem_t b)
{
    sal_sem_destroy(b);
}

int
_bcm_ptp_sem_take(_bcm_ptp_sem_t b, int usec)
{
    int rv = -1;
    sal_usecs_t end_time = sal_time_usecs() + usec;
    int32 wait_time = usec;

    while ((rv != 0) && (wait_time > 0)) {
        rv = sal_sem_take(b, wait_time);
        wait_time = end_time - sal_time_usecs();
    }

    return rv;
}

int
_bcm_ptp_sem_give(_bcm_ptp_sem_t b)
{
    return sal_sem_give(b);
}

#endif /* INCLUDE_PTP */
