/*
 * $Id: sal.h,v 1.21 Broadcom SDK $
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
 * File:     sal.h
 * Purpose:     SAL Support definitions
 */

#ifndef _SAL_H
#define _SAL_H

#include <sal/types.h>
#include <sal/core/thread.h>
#include <sal/core/time.h>

/*
 * Include the system types so that things like uint32_t are always
 * available and consistent.
 */
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#endif

#if defined(UNIX) && !defined(__STRICT_ANSI__)
#define sal_strncasecmp strncasecmp
#define sal_strcasecmp strcasecmp
#else
extern int sal_strncasecmp(const char *s1, const char *s2, size_t n);
extern int sal_strcasecmp(const char *s1, const char *s2);
#endif


/*
 * SAL Initialization
 */

int    sal_appl_init(void);
/*
 * Console Initialization
 */
void    sal_readline_init(void);
void    sal_readline_term(void);

/*
 * SAL Date Routines
 */

int    sal_date_set(sal_time_t *val);
int    sal_date_get(sal_time_t *val);

/*
 * SAL Booting/Rebooting routines.
 */

void    sal_reboot(void);

/*
 * SAL Shell routine - to escape to shell on whatever platform.
 */

void    sal_shell(void);

/* 
 * LED Support, platform specific routine to set led value.
 *  sal_led: sets the state of one or more discrete LEDs; bit 0 is first LED
 *  sal_led_string: sets the state of an alphanumeric display, if any.
 */

extern uint32    sal_led(uint32 led);
extern void    sal_led_string(const char *s);

/*
 * Watchdog timer, platform specific
 */

extern int    sal_watchdog_arm(uint32 usec);

/* 
 * Memory Testing 
 */

extern int    sal_memory_check(uint32 addr);

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
extern void sal_dma_alloc_resource_usage_get(unsigned int *alloc_curr,
                                             unsigned int *alloc_max);
#endif
#endif

/* 
 * Random number generation 
 */

#define SAL_RAND_MIN    0
#define SAL_RAND_MAX    0x7fffffff

extern int    sal_rand(void);
extern void    sal_srand(unsigned int seed);

/*
 * Compatibility macros
 */

#ifdef __KERNEL__
#include <linux/kernel.h>
#define strtol simple_strtol 
#define strtoul simple_strtoul 
#endif

/*
 * Some time functions only available on UNIX/vxWorks
 * For now other systems just won't use these.
 */
#if defined(UNIX) || defined(VXWORKS)
#define sal_mktime mktime
#define sal_localtime localtime
#define sal_strftime strftime
#endif

/*
 * Cause abnormal program termination
 */
#if defined(UNIX) || defined(VXWORKS)
#define sal_abort abort
#else
void sal_abort(void);
#endif


#endif    /* !_SAL_H */

