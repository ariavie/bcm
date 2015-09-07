/*
 * $Id: io.c 1.23 Broadcom SDK $
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
 * SAL I/O abstraction
 */

#include <sal/core/libc.h>
#include <sal/core/alloc.h>
#include <sal/core/spl.h>
#include "sal/appl/editline/editline.h"

#include <soc/cm.h>

#include <sal/types.h>
#include <sal/core/thread.h>
#include <sal/core/dpc.h>

#include <sal/appl/io.h>

#include <soc/debug.h>

#include <stdarg.h>

#ifdef ECOS_LOGGING_SUPPORT
/* Logging mechanism for Mozart */
#include <cyg/logging/logging.h>
#endif /* ECOS_LOGGING_SUPPORT */


#ifndef SDK_CONFIG_SAL_READLINE
/*
 * Read input line, providing Emacs-based command history editing.
 */

char *
sal_readline(char *prompt, char *buf, int bufsize, char *defl)
{
    char *s, *full_prompt, *cont_prompt;
#ifdef INCLUDE_EDITLINE
    extern void rl_prompt_set(CONST char *prompt);
#else
    char *t;
#endif
    int len;

    if (bufsize == 0)
        return NULL;

    cont_prompt = prompt[0] ? "? " : "";
    full_prompt = sal_alloc(sal_strlen(prompt) + (defl ? sal_strlen(defl) : 0) + 8, __FILE__);
    sal_strcpy(full_prompt, prompt);
    if (defl)
	sal_sprintf(full_prompt + sal_strlen(full_prompt), "[%s] ", defl);

#ifdef INCLUDE_EDITLINE

    printk_file("%s", full_prompt);
    s = readline(full_prompt);

#else /* !INCLUDE_EDITLINE */

    t = sal_alloc(bufsize, __FILE__);    
#if defined(KEYSTONE) && defined(__ECOS)
    diag_printf("%s", full_prompt);
#else
    printk("%s", full_prompt);
#endif

    if ((s = sal_console_gets(t, bufsize)) == 0) {
      sal_free(t);
    } else {
	    s[bufsize - 1] = 0;
	    if ((t = strchr(s, '\n')) != 0) {
	      *t = 0;
        /* Replace garbage characters with spaces */
      }
      for (t = s; *t; t++) {
        if (*t < 32 && *t != 7 && *t != 9) {
          *t = ' ';
        }
      }
    }

#endif /* !INCLUDE_EDITLINE */

    if (s == 0) {                       /* Handle Control-D */
        buf[0] = 0;
	/* EOF */
	buf = 0;
	goto done;
    } else {
	printk_file("%s\n", s);
    }

    len = 0;
    if (s[0] == 0) {
	if (defl && buf != defl) {
            if ((len = sal_strlen(defl)) >= bufsize) {
                len = bufsize - 1;
            }
	    sal_memcpy(buf, defl, len);
	}
    } else {
	if ((len = sal_strlen(s)) >= bufsize) {
            len = bufsize - 1;
            printk("WARNING: input line truncated to %d chars\n", len);
        }
	sal_memcpy(buf, s, len);
    }
    buf[len] = 0;

    sal_free(s);

    /*
     * If line ends in a backslash, perform a continuation prompt.
     */

    if (sal_strlen(buf) != 0) {
        /*
         * Ensure that there is atleast one character available
         */
        s = buf + sal_strlen(buf) - 1;
        if (*s == '\\' && sal_readline(cont_prompt, s, bufsize - (s - buf), 0) == 0) {
            buf = 0;
        }
    }

 done:
#ifdef INCLUDE_EDITLINE
    rl_prompt_set(NULL);
#endif
    sal_free(full_prompt);
    
    return buf;
}
#endif /* !SDK_CONFIG_SAL_READLINE */

int sal_readchar(const char *prompt)
{
#ifdef INCLUDE_EDITLINE
    extern int readchar(const char *prompt);
#else
    char	buf[64];
#endif

#ifdef INCLUDE_EDITLINE
    return(readchar(prompt));
#else
    printk("%s", prompt);
    if (NULL == (sal_console_gets(buf, sizeof(buf)))) {
	return(EOF);
    } else {
	return(buf[0]);
    }
#endif
}

#ifdef INCLUDE_EDITLINE
/*
 * Configure readline by passing it completion routine (for TAB
 * completion handling) and a list-possible routine (for ESC-? handling).
 */

void sal_readline_config(char *(*complete)(char *pathname, int *unique),
			 int (*list_possib)(char *pathname, char ***avp))
{
    extern char		*(*rl_complete)(char *, int *);
    extern int		(*rl_list_possib)(char *, char ***);

    rl_complete = complete;
    rl_list_possib = list_possib;
}
#endif /* INCLUDE_EDITLINE */

/*
 * printk
 *
 *  Synchronized printing facility which allows multiplexing output
 *  to console and/or file.
 */

#ifndef NO_FILEIO
#include <stdio.h>
static FILE *file_fp = 0;		/* Non-NULL indicates file opened */
static char *file_nm = NULL;		/* Non-NULL indicates file selected */
#endif /* !NO_FILEIO */

static uint32 current_dk = DK_ERR | DK_WARN; /* Start with errors enabled */

static int console_enabled = 1;

int printk_cons_enable(int enable)
{
    int old = console_enabled;
    console_enabled = enable;
    return old;
}

int printk_cons_is_enabled(void)
{
    return console_enabled;
}

int printk_cons(const char *fmt, ...)
{
    int retv;

    if (console_enabled) {
	va_list varg;
	va_start(varg, fmt);  
	retv = sal_vprintf(fmt, varg);
	va_end(varg);
    } else
	retv = 0;

    return retv;
}

char *printk_file_name(void)
{
#ifdef NO_FILEIO
    return "<no filesystem>";
#else
    return(file_nm);
#endif
}

int printk_file_open(char *filename, int append)
{
#ifndef NO_FILEIO
    if (file_nm) {
	printk_file_close();
    }
    if ((file_fp = sal_fopen(filename, append ? "a" : "w")) == 0) {
	perror("Error opening file");
	return -1;
    }
    file_nm = sal_strcpy((char *)sal_alloc(sal_strlen(filename) + 1, __FILE__), filename);
#endif /* NO_FILEIO */
    return 0;
}

int printk_file_close(void)
{
#ifndef NO_FILEIO
    if (! file_nm) {
	printk("File logging is not on\n");
	return -1;
    }
    if (file_fp) {
	sal_fclose(file_fp);
	file_fp = 0;
    }
    sal_free(file_nm);
    file_nm = NULL;
#endif
    return 0;
}

int printk_file_enable(int enable)
{
#ifdef NO_FILEIO
    return 0;
#else
    int old_enable = (file_fp != NULL);

    if (! old_enable && enable) {
	/* Enable */
	if (! file_nm) {
	    printk("Logging file not opened, can't enable\n");
	    return -1;
	}
	if ((file_fp = sal_fopen(file_nm, "a")) == 0) {
	    perror("Error opening file");
	    return -1;
	}
    }

    if (old_enable && ! enable) {
	if (file_fp) {
	    sal_fclose(file_fp);
	    file_fp = 0;
	}
	/* Note: file_nm remains valid; output can still be re-enabled */
    }
    return old_enable;
#endif
}

int printk_file_is_enabled(void)
{
#ifdef NO_FILEIO
    return 0;
#else
    return (file_fp != NULL);
#endif
}

void printk_file_dpc(void *a1, void *a2, void *a3, void *a4, void *a5)
{
#ifndef NO_FILEIO
    char	*fmt = (char *)a1;
    
    if (file_fp) {
	sal_fprintf(file_fp, fmt, a2, a3, a4, a5);
	sal_fflush(file_fp);
    }
#endif /* !NO_FILEIO */
}

int vprintk_file(const char *fmt, va_list ap)
{
    int	retv = 0;
#ifndef NO_FILEIO

    if (file_fp) {
	if (sal_int_context()) {
	    void *a1, *a2, *a3, *a4;
	    a1 = va_arg(ap, void *);
	    a2 = va_arg(ap, void *);
	    a3 = va_arg(ap, void *);
	    a4 = va_arg(ap, void *);
	    /* Passing fmt in lieue of fake owner to avoid wasting an arg */
	    retv = sal_dpc(printk_file_dpc, (void *)fmt, a1, a2, a3, a4);
	} else {
	    retv = sal_vfprintf(file_fp, fmt, ap);
	    sal_fflush(file_fp);
	}
    } else
	retv = 0;
#endif /* !NO_FILEIO */
    return(retv);

}    

int printk_file(const char *fmt, ...)
{
#ifdef NO_FILEIO
    return 0;
#else
    int retv;

    if (file_fp) {
	va_list varg;
	va_start(varg, fmt); 
	retv = vprintk_file(fmt, varg);
	va_end(varg);
    } else
	retv = 0;

    return retv;
#endif
}

int vprintk(const char *fmt, va_list ap)
{
    int retv = 0;

#ifndef NO_FILEIO
    if (file_fp) {
	va_list ap_copy;
	va_copy(ap_copy, ap);	/* Avoid consuming same arg list twice. */
	retv = vprintk_file(fmt,ap_copy);
        va_end(ap_copy); 
    }
#endif

    if (console_enabled)
	retv = sal_vprintf(fmt,ap);

    return retv;
}

int printk(const char *fmt, ...)
{
    int retv;
    va_list varg;
    va_start(varg, fmt);  
    retv = vprintk(fmt, varg);
    va_end(varg);
    return retv;
}

/*
 * debugk
 *
 *  Debug printing facility (also synchronized with locks like printk).
 *  Specify one or more debug message classes in the call.  The message is
 *  displayed only if the message class is enabled via debugk_select().
 */

uint32 debugk_select(uint32 dk)
{
    uint32 old_dk = current_dk;
    current_dk = dk;
    return old_dk;
}

uint32 debugk_enable(uint32 dk)
{
    uint32 old_dk = current_dk;
    current_dk |= dk;
    return old_dk;
}

uint32 debugk_disable(uint32 dk)
{
    uint32 old_dk = current_dk;
    current_dk &= ~dk;
    return old_dk;
}

int debugk_check(uint32 dk)
{
    return ((current_dk & dk) == dk);
}

int vdebugk(uint32 dk, const char *fmt, va_list varg)
{
    int retv;

    if (debugk_check(dk)) {
        retv = vprintk(fmt, varg);
    } else {
        retv = 0;
    }
  
#ifdef ECOS_LOGGING_SUPPORT

    /* 
     * Logging mechanism for Mozart
     */
    {
        /*
         * To minimize code modification in SDK side and 
         * also to keep the dependency chain clean,
         * constants are not passed between mozart and SDK. 
         */
#define LOG_LVL_ERROR       (0)
#define LOG_LVL_WARNING     (1)
#define LOG_LVL_INFO        (2)
#define LOG_LVL_DEBUG       (3)
#define LOG_CAT_NONE        (32)
                
        loglevel_t level;
        logcat_t i;
        uint32 k = dk;
       
        /* Check level */
        level = LOG_LVL_DEBUG;
        if (k & DK_VERBOSE) {
            level = LOG_LVL_DEBUG; /* Verbose here is more debugging */
            k &= ~DK_VERBOSE;
        }
        if (k & DK_WARN) {
            level = LOG_LVL_WARNING;
            k &= ~DK_WARN;
        }
        if (k & DK_ERR) {
            level = LOG_LVL_ERROR;
            k &= ~DK_ERR;
        }
        
        /*
         * Since soc_cm_debug(VERBOSE/DEBUG) calls on SDK are too frequent 
         * (and sometimes unnecessary), logging are only enabled for
         * error and warning levels. 
         */
        if (level == LOG_LVL_DEBUG) {
            return retv;
        }
        
        /* Log message for each category found */
        for(i=0; i<32; i++) {
            if ((k == 0 && i == 0) || (k & 1)) {
                cyg_syslogv(level, ((k == 0)? LOG_CAT_NONE : i), fmt, varg);
            }
            
            if (k == 0) {
                break;
            }
            
            k >>= 1;
        }
    }
#endif /* ECOS_LOGGING_SUPPORT */

    return retv;
}

int debugk(uint32 dk, const char *fmt, ...)
{
    int retv;
    va_list varg;
    va_start(varg, fmt);
    retv = vdebugk(dk, fmt, varg);
    va_end(varg);
    return retv;
}

