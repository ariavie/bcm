/*
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
 * $Id: typedefs.h 1.3 Broadcom SDK $
 */

#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

/* Define 'SITE_TYPEDEFS' in the compile to include a site specific
 * typedef file "site_typedefs.h".
 *
 * If 'SITE_TYPEDEFS' is not defined, then the "Inferred Typedefs"
 * section of this file makes inferences about the compile environment
 * based on defined symbols and possibly compiler pragmas.
 *
 * Following these two sections is the "Default Typedefs"
 * section. This section is only prcessed if 'USE_TYPEDEF_DEFAULTS' is
 * defined. This section has a default set of typedefs and a few
 * proprocessor symbols (TRUE, FALSE, NULL, ...).
 */

#ifdef SITE_TYPEDEFS

/*
 * Site Specific Typedefs
 *
 */

#include "site_typedefs.h"

#else

/*
 * Inferred Typedefs
 *
 */

/* Infer the compile environment based on preprocessor symbols and pramas.
 * Override type definitions as needed, and include configuration dependent
 * header files to define types.
 */

#ifdef __cplusplus

#define TYPEDEF_BOOL
#ifndef FALSE
#define FALSE	false
#endif
#ifndef TRUE
#define TRUE	true
#endif

#else	/* ! __cplusplus */

#if defined(_WIN32)

#define TYPEDEF_BOOL
typedef	unsigned char	bool;			/* consistent w/BOOL */

#endif /* _WIN32 */

#endif	/* ! __cplusplus */

/* use the Windows ULONG_PTR type when compiling for 64 bit */
#if defined(_WIN64) && !defined(EFI)
#include <basetsd.h>
#define TYPEDEF_UINTPTR
typedef ULONG_PTR uintptr;
#elif defined(__x86_64__)
#define TYPEDEF_UINTPTR
typedef unsigned long long int uintptr;
#endif

#if defined(_HNDRTE_) && !defined(_HNDRTE_SIM_)
#define _NEED_SIZE_T_
#endif

#if defined(_MINOSL_)
#define _NEED_SIZE_T_
#endif

#if defined(EFI) && !defined(_WIN64)
#define _NEED_SIZE_T_
#endif

#if defined(_NEED_SIZE_T_)
typedef long unsigned int size_t;
#endif

#ifdef __DJGPP__
typedef long unsigned int size_t;
#endif /* __DJGPP__ */

#ifdef _MSC_VER	/* Microsoft C */
#define TYPEDEF_INT64
#define TYPEDEF_UINT64
typedef signed __int64	int64;
typedef unsigned __int64 uint64;
#endif

#if defined(MACOSX)
#define TYPEDEF_BOOL
#endif

#if defined(__NetBSD__)
#define TYPEDEF_ULONG
#endif

#if defined(vxworks)
#define TYPEDEF_USHORT
#endif

#ifdef	linux
#define TYPEDEF_UINT
#define TYPEDEF_USHORT
#define TYPEDEF_ULONG
#ifdef __KERNEL__
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19))
#define TYPEDEF_BOOL
#endif	/* >= 2.6.19 */
#endif	/* __KERNEL__ */
#endif	/* linux */

#if defined(__ECOS)
#define TYPEDEF_BOOL
#endif

#if !defined(linux) && !defined(vxworks) && !defined(_WIN32) && !defined(_CFE_) && \
!defined(_HNDRTE_) && !defined(_MINOSL_) && !defined(__DJGPP__) && \
!defined(__IOPOS__) && !defined(__ECOS)
#define TYPEDEF_UINT
#define TYPEDEF_USHORT
#endif

#if defined(vxworks)
/* Do not support the int64 type in VxWorks */
#define TYPEDEF_INT64
#endif

/* Do not support the (u)int64 types with strict ansi for GNU C */
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
#define TYPEDEF_INT64
#define TYPEDEF_UINT64
#endif

/* ICL accepts unsigned 64 bit type only, and complains in ANSI mode
 * for singned or unsigned
 */
#if defined(__ICL)

#define TYPEDEF_INT64

#if defined(__STDC__)
#define TYPEDEF_UINT64
#endif

#endif /* __ICL */

#if !defined(_WIN32) && !defined(_CFE_) && !defined(_HNDRTE_) && !defined(_MINOSL_) && \
	!defined(__DJGPP__) && !defined(__IOPOS__)

/* pick up ushort & uint from standard types.h */
#if defined(linux) && defined(__KERNEL__)

#include <linux/types.h>	/* sys/types.h and linux/types.h are oil and water */

#else

#if defined(__ECOS)
#include <pkgconf/infra.h>
#include <cyg/infra/cyg_type.h>
#include <stdarg.h>
#endif

#include <sys/types.h>

#endif /* linux && __KERNEL__ */

#endif /* !_WIN32 && !PMON && !_CFE_ && !_HNDRTE_  && !_MINOSL_ && !__DJGPP__ */

#if defined(MACOSX)

#ifdef __BIG_ENDIAN__
#define IL_BIGENDIAN
#else
#ifdef IL_BIGENDIAN
#error "IL_BIGENDIAN was defined for a little-endian compile"
#endif
#endif /* __BIG_ENDIAN__ */

#if !defined(__cplusplus)

#if defined (__i386__)
typedef unsigned char bool;
#else
typedef unsigned int bool;
#endif
#define TYPE_BOOL 1
enum {
    false	= 0,
    true	= 1
};

#if defined(KERNEL)
#include <IOKit/IOTypes.h>
#endif /* KERNEL */

#endif /* __cplusplus */

#endif /* MACOSX */

#if defined(vxworks)
#include <private/cplusLibP.h>
#endif

/* use the default typedefs in the next section of this file */
#define USE_TYPEDEF_DEFAULTS

#endif /* SITE_TYPEDEFS */


/*
 * Default Typedefs
 *
 */

#ifdef USE_TYPEDEF_DEFAULTS
#undef USE_TYPEDEF_DEFAULTS

#ifndef TYPEDEF_BOOL
typedef	/* @abstract@ */ unsigned char	bool;
#endif

/* define uchar, ushort, uint, ulong */

#ifndef TYPEDEF_UCHAR
typedef unsigned char	uchar;
#endif

#ifndef TYPEDEF_USHORT
typedef unsigned short	ushort;
#endif

#ifndef TYPEDEF_UINT
typedef unsigned int	uint;
#endif

#ifndef TYPEDEF_ULONG
typedef unsigned long	ulong;
#endif

/* define [u]int8/16/32/64, uintptr */

#ifndef _SAL_TYPES_H

#ifndef TYPEDEF_UINT8
typedef unsigned char	uint8;
#endif

#ifndef TYPEDEF_UINT16
typedef unsigned short	uint16;
#endif

#ifndef TYPEDEF_UINT32
typedef unsigned int	uint32;
#endif

#ifndef TYPEDEF_UINT64
typedef unsigned long long uint64;
#endif

#ifndef TYPEDEF_UINTPTR
typedef unsigned int	uintptr;
#endif

#ifndef TYPEDEF_INT8
typedef signed char	int8;
#endif

#ifndef TYPEDEF_INT16
typedef signed short	int16;
#endif

#ifndef TYPEDEF_INT32
typedef signed int	int32;
#endif

#ifndef TYPEDEF_INT64
typedef signed long long int64;
#endif

#endif /* !_SAL_TYPES_H */

/* define float32/64, float_t */

#ifndef TYPEDEF_FLOAT32
typedef float		float32;
#endif

#ifndef TYPEDEF_FLOAT64
typedef double		float64;
#endif


/*
 * abstracted floating point type allows for compile time selection of
 * single or double precision arithmetic.  Compiling with -DFLOAT32
 * selects single precision; the default is double precision.
 */

#ifndef TYPEDEF_FLOAT_T

#if defined(FLOAT32)
typedef float32 float_t;
#else /* default to double precision floating point */
typedef float64 float_t;
#endif

#endif /* TYPEDEF_FLOAT_T */

/* define macro values */

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1  /* TRUE */
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef OFF
#define	OFF	0
#endif

#ifndef ON
#define	ON	1  /* ON = 1 */
#endif

#define	AUTO	(-1) /* Auto = -1 */

/* define PTRSZ, INLINE */

#ifndef PTRSZ
#define	PTRSZ	sizeof(char*)
#endif

#ifndef INLINE

#ifdef _MSC_VER

#define INLINE __inline

#elif defined(__GNUC__)

#define INLINE __inline__

#else

#define INLINE

#endif /* _MSC_VER */

#endif /* INLINE */

#undef TYPEDEF_BOOL
#undef TYPEDEF_UCHAR
#undef TYPEDEF_USHORT
#undef TYPEDEF_UINT
#undef TYPEDEF_ULONG
#undef TYPEDEF_UINT8
#undef TYPEDEF_UINT16
#undef TYPEDEF_UINT32
#undef TYPEDEF_UINT64
#undef TYPEDEF_UINTPTR
#undef TYPEDEF_INT8
#undef TYPEDEF_INT16
#undef TYPEDEF_INT32
#undef TYPEDEF_INT64
#undef TYPEDEF_FLOAT32
#undef TYPEDEF_FLOAT64
#undef TYPEDEF_FLOAT_T

#endif /* USE_TYPEDEF_DEFAULTS */

/* 
 * Including the bcmdefs.h here, to make sure everyone including typedefs.h
 * gets this automatically
*/
#include "bcmdefs.h"

#endif /* _TYPEDEFS_H_ */
