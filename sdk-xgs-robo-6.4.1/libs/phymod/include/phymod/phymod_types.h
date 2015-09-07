/*
 * $Id: phymod_types.h,v 1.1.2.2 Broadcom SDK $
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
 * Basic type definitions.
 */

#ifndef __PHYMOD_TYPES_H__
#define __PHYMOD_TYPES_H__

#include <phymod/phymod_config.h>

#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__func__)
#endif

#if PHYMOD_CONFIG_DEFINE_UINT8_T == 1
typedef PHYMOD_CONFIG_TYPE_UINT8_T uint8_t; 
#endif

#if PHYMOD_CONFIG_DEFINE_UINT16_T == 1
typedef PHYMOD_CONFIG_TYPE_UINT16_T uint16_t; 
#endif

#if PHYMOD_CONFIG_DEFINE_UINT32_T == 1
typedef PHYMOD_CONFIG_TYPE_UINT32_T uint32_t; 
#endif

#if PHYMOD_CONFIG_DEFINE_SIZE_T == 1
typedef PHYMOD_CONFIG_TYPE_SIZE_T size_t; 
#endif

#if PHYMOD_CONFIG_DEFINE_PRIx32 == 1
#define PRIx32 PHYMOD_CONFIG_MACRO_PRIx32
#endif

#if PHYMOD_CONFIG_DEFINE_PRIu32 == 1
#define PRIu32 PHYMOD_CONFIG_MACRO_PRIu32
#endif

#if PHYMOD_CONFIG_DEFINE_ERROR_CODES
typedef enum {
    PHYMOD_E_NONE       = 0,
    PHYMOD_E_INTERNAL   = -1,
    PHYMOD_E_MEMORY     = -2,
    PHYMOD_E_IO         = -3,
    PHYMOD_E_PARAM      = -4,
    PHYMOD_E_CORE       = -5,
    PHYMOD_E_PHY        = -6,
    PHYMOD_E_BUSY       = -7,
    PHYMOD_E_FAIL       = -8,
    PHYMOD_E_TIMEOUT    = -9,
    PHYMOD_E_RESOURCE   = -10,
    PHYMOD_E_CONFIG     = -11,
    PHYMOD_E_UNAVAIL    = -12,
    PHYMOD_E_INIT       = -13,
    PHYMOD_E_LIMIT      = -14           /* Must come last */
} phymod_error_t;
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef VOLATILE
#define VOLATILE volatile
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef COUNTOF
#define COUNTOF(ary) ((int) (sizeof(ary) / sizeof((ary)[0])))
#endif

#ifndef PTR2INT
#define PTR2INT(_p) ((int)((long)(_p)))
#endif

#ifndef INT2PTR
#define INT2PTR(_i) ((void *)((long)(_i)))
#endif

#define LSHIFT32(_val, _cnt) ((uint32_t)(_val) << (_cnt))

#ifndef COMPILER_REFERENCE
#define COMPILER_REFERENCE(_a) ((void)(_a))
#endif

#ifndef BYTES2BITS
#define BYTES2BITS(x) ((x) * 8)
#endif

#ifndef BYTES2WORDS
#define BYTES2WORDS(x) (((x) + 3) / 4)
#endif

#ifndef WORDS2BITS
#define WORDS2BITS(x) ((x) * 32)
#endif

#ifndef WORDS2BYTES
#define WORDS2BYTES(x) ((x) * 4)
#endif


/* These must be moved */
#ifndef __F_MASK
#define __F_MASK(w) \
        (((uint32_t)1 << w) - 1)
#endif

#ifndef __F_GET
#define __F_GET(d,o,w) \
        (((d) >> o) & __F_MASK(w))
#endif

#ifndef __F_SET
#define __F_SET(d,o,w,v) \
        (d = ((d & ~(__F_MASK(w) << o)) | (((v) & __F_MASK(w)) << o)))
#endif

#ifndef __F_ENCODE

/* Encode a value of a given width at a given offset. Performs compile-time error checking on the value */
/* To ensure it fits within the given width */
#define __F_ENCODE(v,o,w) \
        ( ((v & __F_MASK(w)) == v) ? /* Value fits in width */ ( (uint32_t)(v) << o ) : /* Value does not fit -- compile time error */ 1 << 99)

#endif

typedef uint32_t phy_id_t;

typedef uint32_t core_id_t;


#endif /* __PHYMOD_TYPES_H__ */
