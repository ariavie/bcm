/*
 * $Id: cint_operators.h 1.14 Broadcom SDK $
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
 * File:        cint_operators.h
 * Purpose:     CINT operator interfaces
 *
 */

#ifndef __CINT_OPERATORS_H__
#define __CINT_OPERATORS_H__

#include "cint_config.h"
#include "cint_types.h"

/*
 * Operator Flags
 */
#define CINT_OPERATOR_F_ACCEPT_INTEGRAL         0x1
#define CINT_OPERATOR_F_ACCEPT_DOUBLE           0x2
#define CINT_OPERATOR_F_ACCEPT_POINTER          0x4
#define CINT_OPERATOR_F_ACCEPT_ALL              0x8

#define CINT_OPERATOR_F_LEFT                    0x100
#define CINT_OPERATOR_F_RIGHT                   0x200
#define CINT_OPERATOR_F_TYPE_CHECK              0x400
#define CINT_OPERATOR_F_OPTIONAL                0x800
#define CINT_OPERATOR_F_LOGICAL                 0x1000


#define CINT_OPERATOR_FLAGS_UNARY CINT_OPERATOR_F_RIGHT 
#define CINT_OPERATOR_FLAGS_IUNARY CINT_OPERATOR_FLAGS_UNARY | CINT_OPERATOR_F_ACCEPT_INTEGRAL
#define CINT_OPERATOR_FLAGS_AUNARY CINT_OPERATOR_FLAGS_IUNARY | CINT_OPERATOR_F_ACCEPT_DOUBLE
#define CINT_OPERATOR_FLAGS_PUNARY CINT_OPERATOR_FLAGS_AUNARY | CINT_OPERATOR_F_ACCEPT_POINTER

#define CINT_OPERATOR_FLAGS_BINARY CINT_OPERATOR_F_LEFT | CINT_OPERATOR_F_RIGHT | CINT_OPERATOR_F_TYPE_CHECK
#define CINT_OPERATOR_FLAGS_IBINARY CINT_OPERATOR_FLAGS_BINARY | CINT_OPERATOR_F_ACCEPT_INTEGRAL
#define CINT_OPERATOR_FLAGS_ABINARY CINT_OPERATOR_FLAGS_IBINARY | CINT_OPERATOR_F_ACCEPT_DOUBLE
#define CINT_OPERATOR_FLAGS_PBINARY CINT_OPERATOR_FLAGS_ABINARY | CINT_OPERATOR_F_ACCEPT_POINTER
     

typedef enum cint_operator_e {
    
#define CINT_OPERATOR_LIST_ENTRY(name, _entry, f) cintOp##_entry ,

#include "cint_op_entry.h"

    cintOpLast

} cint_operator_t; 


/*
 * Operator operand types
 */
typedef enum cint_operand_type_e {
    
    cintOperandInt,
    cintOperandUInt,
    cintOperandLong,
    cintOperandULong,
    cintOperandLongLong,
    cintOperandULongLong,
    cintOperandDouble,
    cintOperandPointer

} cint_operand_type_t; 
    
/*
 * Evaluate an operator AST. 
 */
struct cint_ast_s; 
struct cint_variable_s; 

extern struct cint_variable_s* 
cint_eval_operator(struct cint_ast_s* opnode); 

extern cint_operator_t
cint_operator_type(struct cint_ast_s* ast); 

struct cint_variable_s*
cint_eval_operator_internal(struct cint_ast_s* ast, cint_operator_t op, 
                            struct cint_variable_s* left, 
                            struct cint_variable_s* right); 

extern const char* 
cint_operator_name(cint_operator_t op);

#endif /* __CINT_OPERATORS_H__ */
