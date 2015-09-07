/*
 * $Id: cint_internal.h,v 1.24 Broadcom SDK $
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
 * File:        cint_internal.h
 * Purpose:     CINT internal interfaces
 *
 */

#ifndef __CINT_INTERNAL_H__
#define __CINT_INTERNAL_H__

#include "cint_config.h"
#include "cint_porting.h"
#include "cint_types.h"
#include "cint_error.h"
#include "cint_variables.h"

extern cint_error_t
cint_variable_create(cint_variable_t** rvar, 
                     const char* vname, 
                     const cint_parameter_desc_t* desc,
                     unsigned vflags, 
                     void* addr); 


/*
 * Char operations
 */
extern cint_variable_t*
cint_auto_char(char c); 

extern char
cint_char_value(cint_variable_t* v); 

extern cint_variable_t* 
cint_char_set(cint_variable_t* v, char c); 

/*
 * Short operations
 */

extern cint_variable_t*
cint_auto_short(short s); 

extern short
cint_short_value(cint_variable_t* v); 

extern cint_variable_t*
cint_short_set(cint_variable_t* v, short s); 


/*
 * Integer operations
 */
extern cint_variable_t*
cint_auto_integer(int i); 

extern int
cint_integer_value(cint_variable_t* v); 

extern cint_variable_t*
cint_integer_set(cint_variable_t* v, int i); 

/* Unsigned treatements */
#define cint_uinteger_set(_v, i) cint_integer_set(_v, i)
#define cint_uinteger_value(_v) ((unsigned int)cint_integer_value(_v))
extern cint_variable_t* cint_auto_uinteger(unsigned int i); 

/*
 * Long integer operations
 */
extern cint_variable_t*
cint_auto_long(long i); 

extern long
cint_long_value(cint_variable_t* v); 

extern cint_variable_t*
cint_long_set(cint_variable_t* v, long i); 

/* Unsigned treatments */
#define cint_ulong_value(_v) ((unsigned long)cint_long_value(_v))
#define cint_ulong_set(_v, _i) cint_long_set(_v,_i)
extern cint_variable_t* cint_auto_ulong(unsigned long i); 


/*
 * Long Long integer operations
 */
#if CINT_CONFIG_INCLUDE_LONGLONGS == 1
#define CINT_LONGLONG long long
#else
#define CINT_LONGLONG long
#endif

extern cint_variable_t*
cint_auto_longlong(CINT_LONGLONG i); 

extern CINT_LONGLONG 
cint_longlong_value(cint_variable_t* v); 

extern cint_variable_t*
cint_longlong_set(cint_variable_t* v, CINT_LONGLONG i); 

/* Unsigned treatements */
#define cint_ulonglong_value(_v) ((unsigned CINT_LONGLONG) cint_longlong_value(_v))
#define cint_ulonglong_set(_v, _i) cint_longlong_set(_v, _i); 
extern cint_variable_t* cint_auto_ulonglong(unsigned CINT_LONGLONG i); 




/*
 * Double operations
 */
#if CINT_CONFIG_INCLUDE_DOUBLES == 1
/* Use real double type */
#define CINT_DOUBLE double
#else
#define CINT_DOUBLE int
#endif

extern cint_variable_t*
cint_auto_double(CINT_DOUBLE d); 

extern CINT_DOUBLE
cint_double_value(cint_variable_t* v); 

extern cint_variable_t*
cint_double_set(cint_variable_t* v, CINT_DOUBLE d); 

extern int
cint_logical_value(cint_variable_t* v); 

#define cint_auto_logical cint_auto_integer

extern void*
cint_pointer_value(cint_variable_t* v); 

extern char*
cint_cstring_value(const char* s);

extern int
cint_variable_print(cint_variable_t* v, int indent, const char* vname); 

extern int
cint_variable_printf(cint_variable_t* v); 

extern int
cint_chartoi(const char* str, int* rv); 

extern int
cint_ctoi(const char* str, long* rv); 

#if CINT_CONFIG_INCLUDE_LONGLONGS == 1
extern int
cint_ctolli(const char* str, long long* rv); 
extern char *
cint_lltoa(char *buf, int len, unsigned long long num, int sign, int base, int prec);
#endif

#if CINT_CONFIG_INCLUDE_DOUBLES == 1
extern int
cint_ctod(const char* str, double* rv); 
#endif

extern int
cint_is_integer(cint_variable_t* v); 

extern int
cint_is_enum(cint_variable_t* v); 

extern cint_variable_t*
cint_auto_pointer(const char* type, void* value); 

extern int
cint_is_pointer(cint_variable_t* v); 

extern int 
cint_is_vpointer(cint_variable_t* v); 

extern unsigned
cint_atomic_flags(cint_variable_t* v); 

extern int
cint_constant_find(const char* name, int* c); 

extern int
cint_type_compatible(cint_datatype_t* dt1, cint_datatype_t* dt2); 

extern int
cint_type_check(cint_datatype_t* dt1, cint_datatype_t* dt2); 

int
cint_eval_type_list(cint_ast_t* ast, char* basetype, int len,
                    unsigned* flags, int is_variable_declaration);

extern cint_variable_t*
cint_auto_string(const char* s); 

extern cint_variable_t*
cint_variable_clone(cint_variable_t* v); 

extern cint_variable_t*
cint_variable_address(cint_variable_t* v); 

extern char*
cint_strlcpy(char* dst, const char* src, int size); 

extern void*
cint_maddr_dynamic_struct_t(void*p, int mnum, cint_struct_type_t* parent);     

extern int
cint_parameter_count(cint_parameter_desc_t* pd); 

extern int
cint_parameter_void(cint_parameter_desc_t* pd); 

extern void
cint_fparams_print(cint_parameter_desc_t* pd); 

extern int
cint_snprintf_ex(char** dst, int* size, const char* fmt, ...)
     COMPILER_ATTRIBUTE((format (printf, 3, 4))); 

#if CINT_CONFIG_INCLUDE_CINT_LOAD == 1
extern void
cint_ast_cint_cmd_unload(void *handle);
#endif

extern void cint_variable_data_copy(cint_variable_t* dst,
                                    cint_variable_t* src);

extern int 
cint_array_combine_dimensions(cint_parameter_desc_t * type,                           
                              const cint_parameter_desc_t * parameter);

#endif /* __CINT_INTERNAL_H__ */
