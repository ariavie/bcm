/*
 * $Id: console.c,v 1.7 Broadcom SDK $
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

#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>

#define __PROTOTYPE_5_0		/* Get stdarg prototypes for logMsg */
#include <vxWorks.h>
#include <logLib.h>
#include <tyLib.h>
#include <intLib.h>
#include <selectLib.h>
#include <ioLib.h>

#include <sal/core/libc.h>
#include <sal/core/sync.h>
#include <sal/core/thread.h>
#include <sal/core/spl.h>
#include <sal/appl/io.h>

static sal_mutex_t console_mutex;

/*
 * Function:
 *	sal_console_init
 * Purpose:
 *	Initialize the SAL console module
 */

int
sal_console_init(void)
{
    console_mutex = sal_mutex_create("console mutex");

    /* ^X is too dangerous; change monitor trap character to ^\ */
    tyMonitorTrapSet('\\' & 037);

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
#if	defined(TIOCGWINSZ)
    struct winsize	W;
#endif	/* defined(TIOCGWINSZ) */

    if (!isatty(ioTaskStdGet(0,STD_IN))) {
        return -1;
    }
    if (info == NULL) {
        return 0;
    }
    memset(info, 0, sizeof(*info));
#if	defined(TIOCGWINSZ)
    if (ioctl(ioTaskStdGet(0,STD_IN), TIOCGWINSZ, &W) >= 0) {
	info->cols = (int)W.ws_col;
	info->rows = (int)W.ws_row;
    }
#endif	/* defined(TIOCGWINSZ) */
    return 0;
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
    return write(ioTaskStdGet(0,STD_OUT), (void *)buf, count);
}

/*
 * Function:
 *	sal_console_read
 * Purpose:
 *	Read characters from console
 * Parameters:
 *	buf - buffer to fill
 *	count - number of characters to read
 * Notes:
 *      This function is currently only used by readline
 *      to read one character at a time.
 */

jmp_buf TTYget_jb;
void (*TTYget_orig_sigint)(int);

STATIC void TTYget_sighandler(int reason)
{
    longjmp(TTYget_jb, 1);
}

int
sal_console_read(void *buf, int count)
{
    int n;

    /*
     * VxWorks read does not abort with EINTR if a SIGINT is received,
     * so a signal handler and setjmp are used to emulate it.
     */

    TTYget_orig_sigint = signal(SIGINT, TTYget_sighandler);

    if (setjmp(TTYget_jb)) {
        /* Stuff Ctrl-C in buffer to make readline happy */
	((char*)buf)[0] = 'C' & 0x1f;
        n = 1;
    } else {
	n = read(ioTaskStdGet(0,STD_IN), buf, count);
    }

    signal(SIGINT, TTYget_orig_sigint);

    return n;
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

char *
sal_console_gets(char *buf, int size)
{
    char *p = fgets(buf, size, stdin);
    if (p == NULL) {
	clearerr(stdin);
    }
    return p;
}

/*
 * Function:
 *	sal_vprintf_direct
 * Purpose:
 *	Do a vprintf directly to the serial port, bypassing the OS
 * Notes:
 *	Intended for debugging, and especially for interrupt routines.
 */

#ifdef HAVE_DIRECT_SERIAL
static int
sal_vprintf_direct(const char *fmt, va_list varg)
{
    char		buf[256];
    int			r;

    r = sal_vsnprintf(buf, sizeof (buf), fmt, varg);

    sysSerialPrintString(buf);

    return r;
}
#endif /* HAVE_DIRECT_SERIAL */

#ifndef SDK_CONFIG_SAL_VPRINTF
/*
 * Function:
 *	sal_vprintf
 * Purpose:
 *	The base routine upon which all standard printing is built.
 * Parameters:
 *	fmt - printf-style format string
 *	varg - printf-style vararg list
 */

int
sal_vprintf(const char *fmt, va_list varg)
{
    int			retv;

    if (INT_CONTEXT() || sal_int_locked()) {
#ifdef HAVE_DIRECT_SERIAL
	retv = sal_vprintf_direct(fmt, varg);
#else /* !HAVE_DIRECT_SERIAL */
	unsigned int a1, a2, a3, a4, a5, a6;

	a1 = va_arg(varg, unsigned int);
	a2 = va_arg(varg, unsigned int);
	a3 = va_arg(varg, unsigned int);
	a4 = va_arg(varg, unsigned int);
	a5 = va_arg(varg, unsigned int);
	a6 = va_arg(varg, unsigned int);

	/* Note: logMsg allows only 6 arguments */
	retv = logMsg((char *) fmt, a1, a2, a3, a4, a5, a6);
#endif /* !HAVE_DIRECT_SERIAL */
    } else {
#ifndef	SAL_THREAD_NAME_PRINT_DISABLE
	char		thread_name[SAL_THREAD_NAME_MAX_LEN];
	sal_thread_t	thread;

	thread = sal_thread_self();
	thread_name[0] = 0;

	if (thread != sal_thread_main_get()) {
	    sal_thread_name(thread, thread_name, sizeof (thread_name));
	}
#endif	/* !SAL_THREAD_NAME_PRINT_DISABLE */

	sal_mutex_take(console_mutex, sal_mutex_FOREVER);

#ifndef	SAL_THREAD_NAME_PRINT_DISABLE
	if (thread_name[0] != 0) {
	    (void) printf("[%s]", thread_name);
	}
#endif	/* !SAL_THREAD_NAME_PRINT_DISABLE */

	retv = vprintf(fmt, varg);

	fflush(stdout);

	sal_mutex_give(console_mutex);
    }

    return retv;
}
#endif /* !SDK_CONFIG_SAL_VPRINTF */

/*
 * Function:
 *	sal_printf
 * Purpose:
 *	Regular printf routine based on sal_vprintf
 * Parameters:
 *	fmt - printf-style format string
 *	... - printf-style argument list
 */

int
sal_printf(const char *fmt, ...)
{
    int retv;
    va_list varg;
    va_start(varg, fmt);
    retv = sal_vprintf(fmt, varg);
    va_end(varg);
    return retv;
}
