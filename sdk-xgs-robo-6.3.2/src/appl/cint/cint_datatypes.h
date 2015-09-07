/*
 * $Id: cint_datatypes.h 1.21 Broadcom SDK $
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
 * File:        cint_datatypes.h
 * Purpose:     CINT datatype interfaces
 *
 */

#ifndef __CINT_DATATYPES_H__
#define __CINT_DATATYPES_H__

#include "cint_ast.h"
#include "cint_types.h"
#include "cint_error.h"

/*
 * Generic datatype description structure
 */

#define CINT_DATATYPE_F_ATOMIC          0x1
#define CINT_DATATYPE_F_STRUCT          0x2
#define CINT_DATATYPE_F_ENUM            0x4
#define CINT_DATATYPE_F_FUNC            0x8
#define CINT_DATATYPE_F_CONSTANT        0x10
#define CINT_DATATYPE_F_FUNC_DYNAMIC    0x20
#define CINT_DATATYPE_F_FUNC_POINTER    0x40
#define CINT_DATATYPE_F_TYPEDEF         0x80
#define CINT_DATATYPE_F_ITERATOR        0x100
#define CINT_DATATYPE_F_MACRO           0x200
#define CINT_DATATYPE_F_FUNC_VARARG     0x400
#define CINT_DATATYPE_FLAGS_FUNC        (CINT_DATATYPE_F_FUNC | CINT_DATATYPE_F_FUNC_DYNAMIC)
#define CINT_DATATYPE_FLAGS_TYPE        (CINT_DATATYPE_F_ATOMIC | CINT_DATATYPE_F_STRUCT | CINT_DATATYPE_F_ENUM | CINT_DATATYPE_F_FUNC_POINTER )

typedef struct cint_datatype_s {
    
    /* Flags for this datatype */
    unsigned int flags; 

    /* 
     * The description of this datatype
     */ 
    cint_parameter_desc_t desc; 
    
    /*
     * Pointer to the description of the basetype
     */
    union {
        cint_atomic_type_t* ap; 
        cint_struct_type_t* sp; 
        cint_enum_type_t* ep; 
        cint_constants_t* cp; 
        cint_function_t* fp; 
        cint_function_pointer_t* fpp; 
        cint_custom_iterator_t* ip; 
        cint_custom_macro_t* mp; 
        void* p; 
    } basetype;  

    /*
     * Original type name. May be different from basetype name based on aliases or typedefs. 
     * Also used for temporary storage.
     */
    char type[CINT_CONFIG_MAX_VARIABLE_NAME]; 

    /*
     * Custom print and assignment vectors which can be used with this datatype. 
     * These are different from the atomic type definition as it this type
     * may still be an aggregate and may be treated or referenced as such in 
     * addition to the custom assignment and print options. 
     */
    cint_atomic_type_t* cap; 

    /* 
     * Used when identifying when custom operations should be used on a type 
     * defined as an array of another type. 
     *
     * For example, consider an atomic type defined to be a char[6] with custom
     * input and output functions.  For a variable of this type we could always
     * use the custom functions.  However, if we represented an array of this
     * type we would have to store the custom functions and only use them after
     * we dereferenced the entity sufficiently.  Storing the dimension count 
     * (one in this case) of the base type allows us to do this. 
     *
     */
    int type_num_dimensions;
} cint_datatype_t; 


/*
 * Use for structures created by hand
 */
#define CINT_STRUCT_TYPE_DEFINE(_struct) \
 { \
     #_struct, \
     sizeof(_struct), \
     __cint_struct_members__##_struct, \
     __cint_maddr__##_struct \
 }


extern int
cint_datatype_size(const cint_datatype_t* dt); 

extern int
cint_datatype_find(const char* basetype, cint_datatype_t* dt); 

extern int
cint_datatype_enum_find(const char* enumid, cint_datatype_t* dt, int* value); 

typedef int (*cint_datatype_traverse_t)(void* cookie, const cint_datatype_t* dt); 

extern int
cint_datatype_traverse(int flags, cint_datatype_traverse_t cb, void* cookie); 

extern char*
cint_datatype_format(const cint_datatype_t* dt, int alloc); 

extern char*
cint_datatype_format_pd(const cint_parameter_desc_t* pd, int alloc); 

extern void cint_datatype_clear(void); 
extern int cint_datatype_add_atomics(cint_atomic_type_t* types); 
extern int cint_datatype_add_data(cint_data_t* data, void *dlhandle);
extern int cint_datatype_add_function(cint_function_t* f); 
extern int cint_datatype_add_structure(cint_struct_type_t* s); 
extern int cint_datatype_add_enumeration(cint_enum_type_t* cet); 

extern int cint_datatype_constant_find(const char* name, int* c); 
extern int cint_datatype_checkall(int print); 
extern void cint_datatype_clear_structure(cint_struct_type_t* structure);
extern void cint_datatype_clear_function(cint_function_t* function);
extern void cint_datatype_clear_enumeration(cint_enum_type_t* enumeration);

#endif /* __CINT_DATATYPES_H__ */
