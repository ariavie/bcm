/*
 * $Id: enumgen.h,v 1.3 Broadcom SDK $
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
 * File: 	enumgen.h
 * Purpose: 	Defines a set of Macros for managing enums and associated
 *              string arrays.
 */
#ifndef _ENUMGEN_H
#define _ENUMGEN_H

/* Enum typedef and Enum name strings generation Macros */
#define SHR_G_ENUM_BEGIN(etype) typedef enum etype##e {
#define SHR_G_ENUM_END(etype) } etype##t
#define SHR_G_ENTRY(e)      SHR_G_MAKE_STR(e)
#define SHR_G_NAME_BEGIN(etype) {
#define SHR_G_NAME_END(etype) }
#define SHR_G_MAKE_ENUM(e) \
        SHR_G_ENUM_BEGIN(e) \
        SHR_G_ENTRIES(dont_care) \
        SHR_G_ENUM_END(e)


/* These set of macros helps to keep the defined enums in sync with their string
 * name array by requiring all modification in one place.
 * To create a enum of type enum_type_t and an associated string array
 * initializer ENUM_TYPE_NAME_INIALIZER use code described in template below
 * and modify it suitably. For example the template code below defines -
    typedef enum   enum_type_e {
                            enum_type_zero,
                            enum_type_entry1,
                            enum_type_entry2,
                            enum_type_count
                            }   enum_type_t ;                 

    #define ENUM_TYPE_NAME_INITIALIZER {    "zero"  ,           \
                                            "entry1"  ,         \
                                            "entry2"  ,         \
                                            "count"    }  ;

 */

#if 0 /* Template code begin */

#include "enumgen.h"

/* New additions are made below this line */
#define SHR_G_ENTRIES(PFX, DC)      /* DO NOT CHANGE */                     \
SHR_G_ENTRY(PFX, zero),             /* DO NOT CHANGE */                     \
SHR_G_ENTRY(PFX, entry1),           /* ===### Your entries here ###=== */   \
SHR_G_ENTRY(PFX, entry2),           /* ===### Your entries here ###=== */   \
SHR_G_ENTRY(PFX, count)             /* DO NOT CHANGE */
/* New additions are made above this line*/

/* Make the enums */
#undef  SHR_G_MAKE_STR              /* DO NOT CHANGE */
#define SHR_G_MAKE_STR(a)     a     /* DO NOT CHANGE */
SHR_G_MAKE_ENUM(enum_type_);        /* ===### Change enum_type_ ###=== */

/* Make the string array */
#undef  SHR_G_MAKE_STR              /* DO NOT CHANGE */
#define SHR_G_MAKE_STR(a)     #a    /* DO NOT CHANGE */
#define ENUM_TYPE_NAME_INITIALIZER  /* ===### Change ENUM_TYPE_ ###=== */   \
        SHR_G_NAME_BEGIN(dont_care) /* DO NOT CHANGE */                     \
        SHR_G_ENTRIES(, dont_care)  /* DO NOT CHANGE */                     \
        SHR_G_NAME_END(dont_care)   /* DO NOT CHANGE */

#endif /* Template code end */

#endif  /* !_ENUMGEN_H */
