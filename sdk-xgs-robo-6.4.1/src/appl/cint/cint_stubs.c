/*
 * $Id: cint_stubs.c,v 1.1 Broadcom SDK $
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
 * File:        cint_stubs.c
 * Purpose:     portability stub definitions
 *
 */

#include "cint_stubs.h"

#if CINT_CONFIG_INCLUDE_STUBS == 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
cint_stub_printf(const char *format, ...)
{
    int rv;

    va_list ap;
    va_start(ap, format);
    rv = cint_stub_vprintf(format, ap);
    va_end(ap);

    return rv;
}

int
cint_stub_vprintf(const char *format, va_list ap)
{
    return vprintf(format, ap);
}

void *
cint_stub_malloc(size_t size)
{
    return malloc(size);
}

void *
cint_stub_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void
cint_stub_free(void *ptr)
{
    free(ptr);
}

char *
cint_stub_strrchr(const char *s, int c)
{
    return strrchr(s,c);
}

int
cint_stub_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

char *
cint_stub_strstr(const char *s1, const char *s2)
{
    return strstr(s1, s2);
}

void *
cint_stub_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

void *
cint_stub_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

int
cint_stub_sprintf(char *str, const char *format, ...)
{
    int rv;

    va_list ap;
    va_start(ap, format);
    rv = vsprintf(str, format, ap);
    va_end(ap);

    return rv;
}

int
cint_stub_snprintf(char *str, size_t size, const char *format, ...)
{
    int rv;

    va_list ap;
    va_start(ap, format);
    rv = cint_stub_vsnprintf(str, size, format, ap);
    va_end(ap);

    return rv;
}

int
cint_stub_vsnprintf(char *str, size_t size,
                    const char *format, va_list ap)
{
    return vsnprintf(str, size, format, ap);
}

char *
cint_stub_strcat(char *dest, const char *src)
{
    return strcat(dest, src);
}

char *
cint_stub_strdup(const char *s)
{
    return strdup(s);
}

size_t
cint_stub_strlen(const char *s)
{
    return strlen(s);
}

char *
cint_stub_strncpy(char *dest, const char *src, size_t n)
{
    return strncpy(dest, src, n);
}

void
cint_stub_fatal_error(const char *msg)
{
    fputs(msg, stderr);
    exit(2);
}

FILE *
cint_stub_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

int
cint_stub_fclose(FILE *stream)
{
    return fclose(stream);
}

size_t
cint_stub_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fread(ptr, size, nmemb, stream);
}

int
cint_stub_ferror(FILE *stream)
{
    return ferror(stream);
}

int
cint_stub_getc(FILE *stream)
{
    return getc(stream);
}

#endif /* CINT_CONFIG_CINT_STUBS */
