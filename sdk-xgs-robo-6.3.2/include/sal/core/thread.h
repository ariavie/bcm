/*
 * $Id: thread.h 1.15 Broadcom SDK $
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
 * File: 	thread.h
 * Purpose: 	SAL thread definitions
 */

#ifndef _SAL_THREAD_H
#define _SAL_THREAD_H

#include <sdk_config.h>

typedef struct sal_thread_s{
    char thread_opaque_type;
} *sal_thread_t;

#define SAL_THREAD_ERROR	((sal_thread_t) -1)
#ifndef SAL_THREAD_STKSZ
#define	SAL_THREAD_STKSZ	16384	/* Default Stack Size */
#endif

extern sal_thread_t sal_thread_create(char *, int, int, 
				  void (f)(void *), void *);
extern int sal_thread_destroy(sal_thread_t);
extern sal_thread_t sal_thread_self(void);
extern char* sal_thread_name(sal_thread_t thread,
				 char *thread_name, int thread_name_size);
extern void sal_thread_exit(int);
extern void sal_thread_yield(void);
extern void sal_thread_main_set(sal_thread_t thread);
extern sal_thread_t sal_thread_main_get(void);


#define sal_msleep(x)    sal_usleep((x) * 1000)
extern void sal_sleep(int sec);
extern void sal_usleep(unsigned int usec);
extern void sal_udelay(unsigned int usec);

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
extern void
sal_thread_resource_usage_get(
                unsigned int *sal_thread_count_curr,
                unsigned int *sal_stack_size_curr,
                unsigned int *sal_thread_count_max,
                unsigned int *sal_stack_size_max);
#endif
#endif

#endif	/* !_SAL_THREAD_H */
