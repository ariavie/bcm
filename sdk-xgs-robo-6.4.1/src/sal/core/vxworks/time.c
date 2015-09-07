/*
 * $Id: time.c,v 1.11 Broadcom SDK $
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
 * File: 	time.c
 * Purpose:	Time management
 */

#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef VXWORKS
#include <sysLib.h>
#endif

#include <assert.h>
#include <sal/core/time.h>
#include <sal/core/boot.h>
#include <sal/appl/vxworks/hal.h>
/*
 * Function:
 *	_sal_usec_to_ticks
 * Purpose:
 *	Internal routine to convert microseconds to VxWorks clock ticks.
 * Parameters:
 *	usec - time period in microseconds
 * Returns:
 *	ticks - VxWorks clock ticks
 * Notes:
 *	Rounds up: 0 usec maps to 0 ticks; >0 usec maps to >0 ticks.
 */

int
_sal_usec_to_ticks(uint32 usec)
{
    int		divisor;

    divisor = SECOND_USEC / sysClkRateGet();

    return (usec + divisor - 1) / divisor;
}

#ifdef COMPILER_HAS_DOUBLE

/*
 * Function:
 *	sal_time_double
 * Purpose:
 *	Returns the time since boot in seconds.
 * Returns:
 *	Double precision floating-point seconds.
 * Notes:
 *	Useful for benchmarking.
 *	Not safe for use in interrupt routines since it uses floating point.
 *	Made to be as accurate as possible on the given platform.
 */

double
sal_time_double(void)
{
      double       rate = 0.0;   
      UINT32       tbu, tbl, freq;   
     
    if (platform_info->f_timestamp_get && 
        (platform_info->f_timestamp_get(&tbu,&tbl,&freq) == OK) ) {
        rate = 1.0 / freq;   
        return rate * (tbu * 4294967296.0 + tbl);   
    } else {   
        /* Precision limited to the OS clock period (typically 0.01 sec) */
        struct timespec	tv;

        clock_gettime(CLOCK_REALTIME, &tv);

        return (tv.tv_sec + tv.tv_nsec * 0.000000001);
    }
}

#endif /* COMPILER_HAS_DOUBLE */

/*
 * Function:
 *	sal_time_usecs
 * Purpose:
 *	Returns the relative time in microseconds modulo 2^32.
 * Returns:
 *	Time in microseconds modulo 2^32
 * Notes:
 *	The precision is limited to the VxWorks clock period
 *	(typically 10000 usec.)
 */

sal_usecs_t
sal_time_usecs(void)
{
#if defined(COMPILER_HAS_LONGLONG)   
      /* provide more accurate microsecond timing using the PPC timebase */   
     
      UINT32              tbu, tbl,freq;   
      unsigned long long  tb;   
    
    if (platform_info->f_timestamp_get && 
        (platform_info->f_timestamp_get(&tbu,&tbl,&freq) == OK) ) {
         tb = tbu;   
         tb <<= 32;   
         tb |= tbl;   

        if (freq/SECOND_USEC) {
            return (tb / (freq/SECOND_USEC));
        } else if (freq/SECOND_MSEC) {
            return ((tb / (freq/SECOND_MSEC)) * MILLISECOND_USEC);
        } 
    }             
#endif     
    {
    struct timespec	ltv;

    clock_gettime(CLOCK_REALTIME, &ltv);

    return (ltv.tv_sec * 1000000 + ltv.tv_nsec / 1000);
    }
}

/*
 * Function:
 *	sal_time
 * Purpose:
 *	Return the current time in seconds since 00:00, Jan 1, 1970.
 * Returns:
 *	Time in seconds
 * Notes:
 *	This routine must be implemented so it is safe to call from
 *	an interrupt routine.  It is used for timestamping and other
 *	purposes.
 */

sal_time_t
sal_time(void)
{
    struct timespec	tv;

    clock_gettime(CLOCK_REALTIME, &tv);

    return tv.tv_sec;
}
