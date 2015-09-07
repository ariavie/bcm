/*
 * $Id: cint_variables.h,v 1.9 Broadcom SDK $
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
 * File:        cint_variables.h
 * Purpose:     CINT variable interfaces
 *
 */

#ifndef __CINT_VARIABLES_H__
#define __CINT_VARIABLES_H__

/*
 * Generic variable instance structure
 */

#include "cint_types.h"
#include "cint_error.h"
#include "cint_interpreter.h"

#define CINT_VARIABLE_F_AUTO            0x1
#define CINT_VARIABLE_F_SDATA           0x2
#define CINT_VARIABLE_F_SNAME           0x4
#define CINT_VARIABLE_F_CONST           0x8
#define CINT_VARIABLE_F_VOLATILE        0x10
#define CINT_VARIABLE_F_NODESTROY       0x20
#define CINT_VARIABLE_F_AUTOCAST        0x40
#define CINT_VARIABLE_F_CSTRING         0x80

typedef struct cint_variable_s {
    
    /* Private */
    struct cint_variable_s* next; 
    struct cint_variable_s* prev; 


    /* Variable Flags */
    unsigned int flags; 

    /* The name of the variable */
    char* name; 
    
    cint_datatype_t dt; 
    
    void* data; 
    int size; 
    
    /* 
     * When arrays are converted to pointers their dimension data is saved here
     * and the source is set to zero. 
     */
    int cached_num_dimensions;

} cint_variable_t; 


/*
 * Initialize variable management
 */
extern int cint_variable_init(void); 

/*
 * Start a new local variable scope. 
 * All variables from previous local scopes are also available. 
 */
extern int cint_variable_lscope_push(const char* msg); 

/*
 * End the existing local variable scope
 */
extern int cint_variable_lscope_pop(const char* msg); 

/*
 * Start a new variable scope. 
 */
extern int cint_variable_scope_push(const char* msg); 

/*
 * End an existing variable scope. 
 */
extern int cint_variable_scope_pop(const char* msg); 

/*
 * Variable List for given scope
 */
extern cint_variable_t* cint_variable_scope(int); 

/*
 * Allocate a variable 
 */
extern cint_variable_t* cint_variable_alloc(void); 

/*
 * Free a variable 
 */
extern int cint_variable_free(cint_variable_t* v); 

/*
 * Find a variable. All scopes or current scope only. 
 */
extern cint_variable_t* cint_variable_find(const char* name, int cscope); 

/*
 * Add a variable to the current scope or global scope. 
 */
extern int cint_variable_add(cint_variable_t* v); 
extern int cint_variable_list_remove(cint_variable_t* v); 
extern int cint_variable_rename(cint_variable_t* v, const char *name);

/*
 * Delete a variable from the current scope. 
 */
extern int cint_variable_delete(const char* name); 

/*
 * Clear and delete all variables with the AUTO flag in the current scope
 */
extern int cint_variable_auto_save(cint_variable_t* v); 
extern int cint_variable_auto_clear(void); 

extern int cint_variable_clear(void); 
extern int cint_variable_dump(void); 

#endif /* __CINT_VARIABLES_H__ */
