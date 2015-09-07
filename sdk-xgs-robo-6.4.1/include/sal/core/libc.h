/*
 * $Id: libc.h,v 1.17 Broadcom SDK $
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
 * File: 	libc.h
 * Purpose: 	Some C library functions to remove dependencies
 *		of the driver on external libraries.
 *
 * The compile-time flag -DRTOS_STRINGS should be used if your RTOS
 * supports the standard <string.h> routines.  The RTOS library or
 * built-in versions of these routines are likely to be much more
 * efficient than the stand-in versions below.
 */

#ifndef _SAL_LIBC_H
#define _SAL_LIBC_H

#ifdef VXWORKS
#include <sys/types.h>		/* VxWorks needs this for stdarg */
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#endif

#include <sal/types.h>
#include <stdarg.h>

#ifndef NULL
#define	NULL	0
#endif

#if defined(VXWORKS) || defined(UNIX) || defined(__KERNEL__)
#define RTOS_STRINGS
#endif

#if defined(RTOS_STRINGS) && defined(__KERNEL__)
#include <linux/string.h>
#else
#include <string.h>
#endif

#ifdef RTOS_STRINGS


#define sal_strlen		strlen
#define sal_strcpy		strcpy
#define sal_strncpy		strncpy
#define sal_strcmp		strcmp
#define sal_strstr              strstr
#define sal_strcat              strcat
#define sal_strncat             strncat
#define sal_strncmp             strncmp
#define sal_strchr              strchr
#define sal_strrchr             strrchr
#define sal_strspn              strspn
#define sal_strtok              strtok
#define sal_strtoul             strtoul
#define sal_toupper             toupper
#define sal_tolower             tolower
#define sal_atoi                atoi

#define sal_abort               abort 
#define sal_fgets               fgets 
#define sal_fgetc               fgetc 
#define sal_fputs               fputs 
#define sal_fputc               fputc 
#define sal_fwrite              fwrite 
#define sal_fprintf             fprintf 
#define sal_fflush              fflush 
#define sal_vfprintf            vfprintf 



#if defined(memcpy)
void *sal_memcpy_wrapper(void *, const void *, size_t);
#define sal_memcpy		sal_memcpy_wrapper
#else
#define sal_memcpy		memcpy
#endif

#define sal_memset		memset

#else /* !RTOS_STRINGS */

extern int sal_strlen(const char *s);
extern char *sal_strcpy(char *, const char *);
extern char *sal_strncpy(char *, const char *, size_t);
extern int sal_strcmp(const char *, const char *);

extern void *sal_memcpy(void *, const void *, size_t);
extern void *sal_memset(void *, int, size_t);
extern char *sal_strstr(const char *haystack, const char *needle);
extern char *sal_strcat(const char *dest, const char *src);
extern char *sal_strncat(const char *dest, const char *src, size_t n);
extern int sal_strncmp(const char *s1, const char *s2, size_t n);
extern char *sal_strchr(const char *s, int c);
extern char *sal_strrchr(const char *s, int c);
extern size_t sal_strspn(const char *s, const char *accept);
 /* Should be avoided, not thread safe !!! */
extern char *sal_strtok(char *str, const char *delim);
extern unsigned long int sal_strtoul(const char *nptr, 
        char **endptr, int base);
extern int sal_toupper(int c);
extern int sal_tolower(int c);
extern int sal_atoi(const char *nptr);

#endif /* !RTOS_STRINGS */

/* Always use our version of memcmp, since it is broken 
 * in certain OS-versions (e..g Linux 2.4.18)
 */
extern int sal_memcmp(const void *, const void *, size_t);

/* Always use our version of strdup, strndup, which uses 
 * sal_alloc() instead of malloc(), and can be freed by sal_free()
 */
extern char *sal_strdup(const char *s);
extern char *sal_strndup(const char *s, size_t);

extern char *sal_strtok_r(char *s1, const char *delim, char **s2);

extern int sal_ctoi(const char *s, char **end);	/* C constant to integer */
extern void sal_itoa(char *buf, uint32 num,
		     int base, int caps, int prec);

#ifdef COMPILER_HAS_DOUBLE
extern void sal_ftoa(char *buf, double f, int decimals);
#endif

extern int sal_vsnprintf(char *buf, size_t bufsize,
			 const char *fmt, va_list ap)
    COMPILER_ATTRIBUTE ((format (printf, 3, 0)));
extern int sal_vsprintf(char *buf, const char *fmt, va_list ap)
    COMPILER_ATTRIBUTE ((format (printf, 2, 0)));
extern int sal_snprintf(char *buf, size_t bufsize, const char *fmt, ...)
    COMPILER_ATTRIBUTE ((format (printf, 3, 4)));
extern int sal_sprintf(char *buf, const char *fmt, ...)
    COMPILER_ATTRIBUTE ((format (printf, 2, 3)));
extern void sal_free_safe(void *ptr); 

#endif	/* !_SAL_LIBC_H */
