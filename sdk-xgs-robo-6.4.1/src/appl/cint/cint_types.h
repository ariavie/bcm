/*
 * $Id: cint_types.h,v 1.21 Broadcom SDK $
 * 
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
 * File:        cint_types.h
 * Purpose:     CINT type system interface
 *
 */

#ifndef __CINT_TYPES_H__
#define __CINT_TYPES_H__

#include "cint_config.h"

/*
 * Atomic data types structure
 */

#define CINT_ATOMIC_TYPE_F_CHAR           0x1
#define CINT_ATOMIC_TYPE_F_SHORT          0x2
#define CINT_ATOMIC_TYPE_F_INT            0x4
#define CINT_ATOMIC_TYPE_F_LONG           0x8
#define CINT_ATOMIC_TYPE_F_LONGLONG       0x10
#define CINT_ATOMIC_TYPE_F_DOUBLE         0x20
#define CINT_ATOMIC_TYPE_FLAGS_MASK       0xFF  /* First Byte reserved for type flags */


#define _AF(_type) CINT_ATOMIC_TYPE_F_##_type
#define CINT_ATOMIC_TYPE_FLAGS_CSI      ( _AF(CHAR) | _AF(SHORT) | _AF(INT) )
#define CINT_ATOMIC_TYPE_FLAGS_CSIL     ( _AF(CHAR) | _AF(SHORT) | _AF(INT) | _AF(LONG) )
#define CINT_ATOMIC_TYPE_FLAGS_INTEGRAL ( CINT_ATOMIC_TYPE_FLAGS_CSI | _AF(LONG) | _AF(LONGLONG) )
#define CINT_ATOMIC_TYPE_FLAGS_ARITH    ( CINT_ATOMIC_TYPE_FLAGS_INTEGRAL | _AF(DOUBLE) )

#define CINT_ATOMIC_TYPE_F_SIGNED         0x100
#define CINT_ATOMIC_TYPE_F_UNSIGNED       0x200
#define CINT_ATOMIC_TYPE_F_ABSTRACT       0x400
#define CINT_ATOMIC_TYPE_F_CAP_ONLY       0x800

typedef enum cint_atomic_format_e {
    cintAtomicFormatCintPrintVariable, 
    cintAtomicFormatCintPrintf, 
    cintAtomicFormatCintLast
} cint_atomic_format_t; 

typedef struct cint_atomic_type_s {
    const char* name; 
    int size; 
    unsigned int flags; 
    int (*format)(void* p, char* dst, int size, cint_atomic_format_t format); 
    int (*assign)(void* p, const char* expr); 
} cint_atomic_type_t; 



/******************************************************************************
 *
 * Autogen Description Structures
 *
 *****************************************************************************/

#define CINT_PARAM_IN           0x100 /* pointer is input */
#define CINT_PARAM_OUT          0x200 /* pointer is output */
#define CINT_PARAM_INOUT        (CINT_PARAM_IN|CINT_PARAM_OUT)
#define CINT_PARAM_VP           0x400 /* param is array base */
#define CINT_PARAM_VL           0x800 /* param is array length */
#define CINT_PARAM_IDX          0x0ff /* VP/VL assoc. param idx */

/*
 * Description of a named parameter or structure member. 
 */
typedef struct cint_parameter_desc_s {
    const char* basetype; 
    const char* name; 
    int pcount; 
    /* 
     * Specifies a single dimension array.  When the type is registered
     * with the interpreter it is converted to num_dimensions and 
     * dimensions.
     */
    int array; 
    unsigned int flags;
    int num_dimensions;
    int dimensions[CINT_CONFIG_ARRAY_DIMENSION_LIMIT];
} cint_parameter_desc_t; 

typedef struct cint_fparams_s {
    void* rv; 
    void* args[CINT_CONFIG_MAX_FPARAMS]; 
} cint_fparams_t; 

/*
 * Function description structure
 */

struct cint_ast_s; 

typedef void (*cint_fpointer_t)(void); 

typedef int (*cint_wrapper_f)(cint_fparams_t* params); 

typedef struct cint_function_s { 
    
    /* Function Name */
    const char* name;
    
    /* Real function address */
    cint_fpointer_t addr; 

    union {
        cint_wrapper_f wrapper;             /* Static Function Wrappers */
        struct cint_ast_s* statements;      /* Interpreter Functions */
    } body; 
    
    /* Function return value and parameter descriptions */
    cint_parameter_desc_t* params; 

} cint_function_t; 


/*
 * Function pointer description structure
 */

typedef struct cint_function_pointer_s {
    
    /* Function pointer type name */
    const char* name;                   
    /* Compiled function to use when referencing this type of function/callback */
    cint_fpointer_t cb; 

    cint_parameter_desc_t* params; 
    
    void* data; 

} cint_function_pointer_t; 

    
/*
 * Structure description structure
 */
typedef struct cint_struct_type_s {
    const char* name; 
    int size; 
    cint_parameter_desc_t* struct_members; 
    void* (*maddr)(void* p, int idx, struct cint_struct_type_s* parent); 
} cint_struct_type_t; 


/*
 * Enum value mapping structure
 */
typedef struct cint_enum_map_s {
    const char* name;
    int value;
} cint_enum_map_t; 

/*
 * Constants -- described as enum maps
 */
typedef cint_enum_map_t cint_constants_t; 

/*
 * Enum description structure
 */
typedef struct cint_enum_type_s {
    const char* name; 
    cint_enum_map_t* enum_map; 
} cint_enum_type_t; 

/*
 * Functional Language Extensions
 */
typedef struct cint_custom_iterator_s {
    const char* name; 
    struct cint_ast_s* (*handler)(const char* name, struct cint_ast_s* arguments, struct cint_ast_s* statements); 
} cint_custom_iterator_t; 

typedef struct cint_custom_macro_s {
    const char* name; 
    struct cint_ast_s* (*handler)(const char* name, struct cint_ast_s* arguments); 
} cint_custom_macro_t; 

/*
 * Data
 */
typedef struct cint_data_s {
    
    cint_atomic_type_t* atomics;
    cint_function_t* functions; 
    cint_struct_type_t* structures;
    cint_enum_type_t* enums; 
    cint_parameter_desc_t* typedefs; 
    cint_constants_t* constants; 
    cint_function_pointer_t* fpointers;
    cint_custom_iterator_t* iterators; 
    cint_custom_macro_t* macros; 

} cint_data_t; 


/*
 * Use when declaring data in a shared library
 */
#define CINT_LOAD_DATA_SYMBOL __cint_data_load
#define CINT_LOAD_DATA(cint_data_struct) cint_data_t* CINT_LOAD_DATA_SYMBOL = & cint_data_struct

extern int
cint_interpreter_callback(cint_function_pointer_t* cb, int nargs, int nreturn, ...); 



/***********************************************************************
 *
 * The following macros can be used to declare and initialize
 * CINT datatypes, structures, and wrappers. 
 *
 **********************************************************************/


/*
 * FUNCTION Wrapper Macros
 */

/* Start a function wrapper */
#define CINT_FWRAPPER_START    int __arg_index = 0

/* Extract a function argument */
#define CINT_FARG(_type, _name) _type _name = *(_type *)fparams->args[__arg_index++]

/* Set the return value */
#define CINT_FRET(_type) *(_type*)fparams->rv

/* VOID Parameter */
#define CINT_PARAM_VOID(name)   { "void", #name, 0, 0 }
#define CINT_ENTRY_LAST         { NULL }


#define CINT_FWRAPPER_END       /* Nothing currently */

#define CINT_FWRAPPER_ENTRY(_fname) { #_fname, (cint_fpointer_t) _fname, { __cint_fwrapper__##_fname }, __cint_parameters__##_fname }
#define CINT_FWRAPPER_NENTRY(_name, _fname) { _name, (cint_fpointer_t) _fname, { __cint_fwrapper__##_fname }, __cint_parameters__##_fname }
#define CINT_FWRAPPER_VARARG_ENTRY(_name, _fname) { _name, (cint_fpointer_t) _fname, { NULL }, NULL }

#include "cint_wrappers.h"

#endif /* __CINT_TYPES_H__ */

