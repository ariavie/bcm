/*
 * $Id: time.h,v 1.7 Broadcom SDK $
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
 * File:     time.h
 * Purpose:     SAL time definitions
 *
 * Microsecond Time Routines
 *
 *  The main clock abstraction is sal_usecs_t, an unsigned 32-bit
 *  counter that increments every microsecond and wraps around every
 *  4294.967296 seconds (~71.5 minutes)
 */

#ifndef _SAL_TIME_H
#define _SAL_TIME_H

#include <sal/types.h>

/*
 * Constants
 */

#define MILLISECOND_USEC        (1000)
#define SECOND_USEC            (1000000)
#define MINUTE_USEC            (60 * SECOND_USEC)
#define HOUR_USEC            (60 * MINUTE_USEC)

#define SECOND_MSEC            (1000)
#define MINUTE_MSEC            (60 * SECOND_MSEC)

#define SAL_USECS_TIMESTAMP_ROLLOVER    ((uint32)(0xFFFFFFFF))    

typedef unsigned long sal_time_t;
typedef uint32 sal_usecs_t;

void sal_time_usecs_register(sal_usecs_t (*f)(void));
void sal_time_register(sal_time_t (*f)(void));

/* Relative time in microseconds modulo 2^32 */
extern sal_usecs_t sal_time_usecs(void);

/* Absolute time in seconds ala Unix time, interrupt-safe */
extern sal_time_t sal_time(void);

#ifdef COMPILER_HAS_DOUBLE

void sal_time_double_register(double (*f)(void));

/*
 * Time since boot in seconds as a double precision value, useful for
 * benchmarking.  Precision is platform-dependent and may be limited to
 * scheduler clock period.  On a Mousse it uses the timebase and has a
 * precision of 80 ns (0.00000008).
 */
extern double sal_time_double(void);
#define SAL_TIME_DOUBLE() sal_time_double()

#else /* COMPILER_HAS_DOUBLE */
#define SAL_TIME_DOUBLE() sal_time()
#endif


/*
 * SAL_USECS_SUB
 *
 *  Subtracts two sal_usecs_t and returns signed integer microseconds.
 *  Can be used as a comparison because it returns:
 *
 *    integer < 0 if A < B
 *    integer > 0 if A > B
 *    integer = 0 if A == B
 *
 * SAL_USECS_ADD
 *
 *  Adds usec to time.  Wrap-around is OK as long as the total
 *  amount added is less than 0x7fffffff.
 */

#define SAL_USECS_SUB(_t2, _t1)        (((int) ((_t2) - (_t1))))
#define SAL_USECS_ADD(_t, _us)        ((_t) + (_us))

#endif    /* !_SAL_TIME_H */
