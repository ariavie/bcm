/*
 * $Id: console.c 1.11 Broadcom SDK $
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
 * File: 	console.c
 * Purpose:	User TTY I/O
 */

#include <stdarg.h>
#include <sal/core/sync.h>
#include <sal/core/thread.h>
#include <sal/appl/io.h>

#ifdef SAL_NO_TTY
#define	SAL_THREAD_NAME_PRINT_DISABLE
#else
/* These routines are provided by the TTY module */
extern int tty_printf(const char* fmt, ...);
extern int tty_vprintf(const char* fmt, va_list vargs);
#endif


/*
 * sal_console_init
 */

int
sal_console_init(void)
{
    return 0;
}

/*
 * Function:
 *	sal_console_info
 * Purpose:
 *	Get console information
 * Parameters:
 *	info - console info
 * Returns:
 *      0 on success
 *      -1 if stdin is not a tty
 */

int
sal_console_info_get(sal_console_info_t *info)
{
    return -1;
}

/*
 * Function:
 *	sal_console_write
 * Purpose:
 *	Write characters to console
 * Parameters:
 *	buf - buffer to write
 *	count - number of characters in buffer
 */

int
sal_console_write(const void *buf, int count)
{
    return 0;
}

/*
 * Function:
 *	sal_console_read
 * Purpose:
 *	Read characters from console
 * Parameters:
 *	buf - buffer to fill
 *	count - number of characters to read
 */

int
sal_console_read(void *buf, int count)
{
    return 0;
}

/*
 * Function:
 *	sal_console_gets
 * Purpose:
 *	Read line from console
 * Parameters:
 *	buf - input buffer
 *	size - size of input buffer
 */

extern char *tty_gets(char *, int);

char *
sal_console_gets(char *buf, int size)
{
    return tty_gets(buf, size);
}

#ifndef SDK_CONFIG_SAL_VPRINTF
/*
 * sal_vprintf
 *
 *   This is the base routine upon which all standard printing is built.
 */

int sal_vprintf(const char *fmt, va_list varg)
{
    int			retv = 0;
#ifndef	SAL_THREAD_NAME_PRINT_DISABLE
    char		thread_name[80];
    sal_thread_t	thread;

    thread = sal_thread_self();
    thread_name[0] = 0;

    if (thread != sal_thread_main_get()) {
	sal_thread_name(thread, thread_name, sizeof (thread_name));
    }
#endif	/* !SAL_THREAD_NAME_PRINT_DISABLE */

#ifndef	SAL_THREAD_NAME_PRINT_DISABLE
    if (thread_name[0] != 0) {
	(void) tty_printf("[%s]", thread_name);
    }
#endif	/* !SAL_THREAD_NAME_PRINT_DISABLE */

#ifndef SAL_NO_TTY
    retv = tty_vprintf(fmt, varg);
#endif

    return retv;
}
#endif /* !SDK_CONFIG_SAL_VPRINTF */

int sal_printf(const char *fmt, ...)
{
    int retv;
    va_list varg;
    va_start(varg, fmt);
    retv = sal_vprintf(fmt, varg);
    va_end(varg);
    return retv;
}
