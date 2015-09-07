/*
 * prototypes for functions defined in bcmstdlib.c
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
 * $Id: bcmstdlib.h,v 1.2 Broadcom SDK $:
 */

/*
 * bcmstdlib.h file should be used only to construct an OSL or alone without any OSL
 * It should not be used with any orbitarary OSL's as there could be a conflict
 * with some of the routines defined here.
*/

#ifndef	_BCMSTDLIB_H
#define	_BCMSTDLIB_H

#include "typedefs.h"
#include "bcmdefs.h"

#if !defined(vxworks) && (!defined(_WIN32) || defined(EFI)) && !defined(_CFE_)

typedef int FILE;
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)

/* i/o functions */
extern int fputc(int c, FILE *stream);
extern void putc(int c);
/* extern int putc(int c, FILE *stream); */
#define putchar(c) putc(c)
extern int fputs(const char *s, FILE *stream);
extern int puts(const char *s);
extern int getc(void);
extern bool keypressed(void);

/* string functions */
#define PRINTF_BUFLEN	512
extern int printf(const char *fmt, ...);
extern int BCMROMFN(sprintf)(char *buf, const char *fmt, ...);

extern char *BCMROMFN(index)(const char *s, int c);

/* For EFI, use some of the common EFI Driver Library functions
 * to reduce final size and improve efficiency
 */
#ifndef EFI
extern int BCMROMFN(strcmp)(const char *s1, const char *s2);
extern size_t BCMROMFN(strlen)(const char *s);
extern char *BCMROMFN(strcpy)(char *dest, const char *src);
extern char *BCMROMFN(strstr)(const char *s, const char *find);
extern char *BCMROMFN(strncpy)(char *dest, const char *src, size_t n);
extern char *BCMROMFN(strcat)(char *d, const char *s);
#endif /* EFI */

extern int BCMROMFN(strncmp)(const char *s1, const char *s2, size_t n);
extern char *BCMROMFN(strchr)(const char *str, int c);
extern char *BCMROMFN(strrchr)(const char *str, int c);
extern size_t BCMROMFN(strspn)(const char *s1, const char *s2);
extern size_t BCMROMFN(strcspn)(const char *s1, const char *s2);
extern unsigned long BCMROMFN(strtoul)(const char *cp, char **endp, int base);
#define strtol(nptr, endptr, base) ((long)strtoul((nptr), (endptr), (base)))
#define	atoi(s)	((int)(strtoul((s), NULL, 10)))

extern void *BCMROMFN(memmove)(void *dest, const void *src, size_t n);
extern void *BCMROMFN(memchr)(const void *s, int c, size_t n);

extern int BCMROMFN(vsprintf)(char *buf, const char *fmt, va_list ap);
/* mem functions */
#ifndef EFI
/* For EFI, using EFIDriverLib versions */
/* Cannot use memmem in ROM because of character array initialization wiht "" in gcc */
extern void *memset(void *dest, int c, size_t n);
/* Cannot use memcpy in ROM because of structure assignmnets in gcc */
extern void *memcpy(void *dest, const void *src, size_t n);
extern int BCMROMFN(memcmp)(const void *s1, const void *s2, size_t n);

/* bcopy, bcmp, and bzero */
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
#define	bzero(b, len)		memset((b), '\0', (len))
#endif /* EFI */

extern unsigned long rand(void);

#endif /* !defined(vxworks) && !defined(_WIN32) && !defined(_CFE_) */

extern int BCMROMFN(snprintf)(char *str, size_t n, char const *fmt, ...);
extern int BCMROMFN(vsnprintf)(char *buf, size_t size, const char *fmt, va_list ap);

#endif 	/* _BCMSTDLIB_H */
