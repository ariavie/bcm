/*
 * $Id: sync.h,v 1.9 Broadcom SDK $
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
 * File: 	sync.h
 * Purpose: 	SAL thread definitions
 *
 * Note: the SAL mutex abstraction is required to allow the same mutex
 * to be taken recursively by the same thread without deadlock.
 */

#ifndef _SAL_SYNC_H
#define _SAL_SYNC_H

typedef struct sal_mutex_s{
    char mutex_opaque_type;
} *sal_mutex_t;

typedef struct sal_sem_s{
    char sal_opaque_type;
} *sal_sem_t;

typedef struct sal_spinlock_s {
    char spinlock_opaque_type;
} *sal_spinlock_t;

#define sal_mutex_FOREVER	(-1)
#define sal_mutex_NOWAIT	0

#define sal_sem_FOREVER		(-1)
#define sal_sem_BINARY		1
#define sal_sem_COUNTING	0

extern sal_mutex_t sal_mutex_create(char *desc);
extern void sal_mutex_destroy(sal_mutex_t m);

/*#define BROADCOM_DEBUG_MUTEX*/

#ifndef BROADCOM_DEBUG_MUTEX
extern int sal_mutex_take(sal_mutex_t m, int usec);
extern int sal_mutex_give(sal_mutex_t m);
#endif

extern sal_sem_t sal_sem_create(char *desc, int binary, int initial_count);
extern void sal_sem_destroy(sal_sem_t b);
extern int sal_sem_take(sal_sem_t b, int usec);
extern int sal_sem_give(sal_sem_t b);
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
extern void
sal_sem_resource_usage_get(unsigned int *sem_curr, unsigned int *sem_max);
extern void
sal_mutex_resource_usage_get(unsigned int *mutex_curr, unsigned int *mutex_max);
#endif
#endif



#ifdef BROADCOM_DEBUG_MUTEX
extern void sal_mutex_dbg_dump(void);

extern int sal_mutex_take_bcm_debug(sal_mutex_t m, int usec, const char *take_loc, int line);
extern int sal_mutex_give_bcm_debug(sal_mutex_t m, const char *give_loc, int line);


#define sal_mutex_take(MUTEX, WAIT_TIME) sal_mutex_take_bcm_debug(MUTEX, WAIT_TIME, __FILE__, __LINE__)
#define sal_mutex_give(MUTEX) sal_mutex_give_bcm_debug(MUTEX, __FILE__ , __LINE__)


/*
extern int sal_mutex_take_bcm_debug(sal_mutex_t m, int usec, const char *take_loc);
extern int sal_mutex_give_bcm_debug(sal_mutex_t m, const char *);


#define sal_mutex_take(MUTEX, WAIT_TIME) sal_mtx_take_bcm_debug(MUTEX, WAIT_TIME, __FILE__)

#define sal_mutex_give(MUTEX) sal_mutex_give_bcm_debug(MUTEX, __LINE__ )
*/

#endif

/* SAL Global Lock - SDK internal use only */
extern sal_mutex_t         sal_global_lock;
int sal_global_lock_init(void);
#define SAL_GLOBAL_LOCK     sal_mutex_take(sal_global_lock, sal_mutex_FOREVER)
#define SAL_GLOBAL_UNLOCK   sal_mutex_give(sal_global_lock)

extern sal_spinlock_t sal_spinlock_create(char *desc);
extern int sal_spinlock_destroy(sal_spinlock_t lock);
extern int sal_spinlock_lock(sal_spinlock_t lock);
extern int sal_spinlock_unlock(sal_spinlock_t lock);

#endif	/* !_SAL_SYNC_H */
