/* 
 * $Id: cint_ast_entry.h 1.5 Broadcom SDK $
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
 * File:        cint_ast_entry.h
 * Purpose:     Declare AST entries based on CINT_AST_LIST_ENTRY
 */

#ifndef CINT_AST_LIST_ENTRY
#error CINT_AST_LIST_ENTRY needs definition
#endif

CINT_AST_LIST_ENTRY(Integer)
#if CINT_CONFIG_INCLUDE_DOUBLES == 1
CINT_AST_LIST_ENTRY(Double)
#endif
#if CINT_CONFIG_INCLUDE_LONGLONGS == 1
CINT_AST_LIST_ENTRY(LongLong)
#endif
CINT_AST_LIST_ENTRY(String)
CINT_AST_LIST_ENTRY(Type)
CINT_AST_LIST_ENTRY(Identifier)
CINT_AST_LIST_ENTRY(Declaration)
CINT_AST_LIST_ENTRY(Initializer)
CINT_AST_LIST_ENTRY(Operator)
CINT_AST_LIST_ENTRY(Function) 
CINT_AST_LIST_ENTRY(Elist)
CINT_AST_LIST_ENTRY(While)
CINT_AST_LIST_ENTRY(For)
CINT_AST_LIST_ENTRY(Continue)
CINT_AST_LIST_ENTRY(Break)
CINT_AST_LIST_ENTRY(Return)
CINT_AST_LIST_ENTRY(If)
CINT_AST_LIST_ENTRY(Print)
CINT_AST_LIST_ENTRY(FunctionDef)
CINT_AST_LIST_ENTRY(StructureDef)
CINT_AST_LIST_ENTRY(Switch)
CINT_AST_LIST_ENTRY(Case)
CINT_AST_LIST_ENTRY(Cint)
CINT_AST_LIST_ENTRY(Enumerator)
CINT_AST_LIST_ENTRY(EnumDef)
CINT_AST_LIST_ENTRY(Empty)

#undef CINT_AST_LIST_ENTRY
