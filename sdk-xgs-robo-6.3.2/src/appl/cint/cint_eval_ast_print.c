/*
 * $Id: cint_eval_ast_print.c 1.10 Broadcom SDK $
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
 * File:        cint_eval_ast_print.c
 * Purpose:     CINT print function
 *
 */

#include "cint_eval_ast_print.h"
#include "cint_porting.h"
#include "cint_variables.h"
#include "cint_internal.h"
#include "cint_error.h"
#include "cint_debug.h"
#include "cint_eval_asts.h"


cint_variable_t*
cint_eval_ast_Print(cint_ast_t* ast)
{
    cint_variable_t* v = NULL; 

    if(ast) {
        
        switch(ast->utype.print.expression->ntype)
            {
                /*
                 * Make the output nicer for immediate strings and integers.
                 */
            case cintAstString:
                {
                    /* print immediate string value */
                    char *tmp, *nl; 

                    tmp = cint_cstring_value
                        (ast->utype.print.expression->utype.string.s);
                    CINT_PRINTF("%s", tmp);

                    /* Check if the string terminates in a newline. If
                       not, then issue one. */
                    nl=CINT_STRRCHR(tmp, '\n');
                    if (nl == NULL || nl[1] != 0) {
                        /*
                          nl is NULL if there were no newlines in the
                          string at all. nl[0] is non-zero if the
                          last newline in the string is not before the
                          terminating null.
                         */
                        CINT_PRINTF("\n");
                    }
                    CINT_FREE(tmp); 
                    break; 
                }
            case cintAstInteger:
                {
                    /* print immediate integer value */
                    CINT_PRINTF("%ld\n", ast->utype.print.expression->utype.integer.i); 
                    break; 
                }
#if CINT_CONFIG_INCLUDE_LONGLONGS == 1
            case cintAstLongLong:
                {
                    long long i =
                        ast->utype.print.expression->utype._longlong.i;
                    char buf1[50], *s1;
                    s1 = cint_lltoa(buf1, sizeof(buf1), i, 1, 10, 0);
                    /* print immediate integer value */
                    CINT_PRINTF("%s\n", s1); 
                    break; 
                }
#endif
            case cintAstType:
                {
                    /* Lookup the type and print information about it */
                    cint_datatype_t dt; 
                    if(cint_datatype_find(ast->utype.print.expression->utype.type.s, &dt) == CINT_E_NONE) {
                        switch((dt.flags & CINT_DATATYPE_FLAGS_TYPE))
                            {
                            case CINT_DATATYPE_F_ATOMIC:
                                {
                                    CINT_PRINTF("%s: atomic datatype, size %d bytes\n", dt.basetype.ap->name, dt.basetype.ap->size); 
                                    break; 
                                }
                            case CINT_DATATYPE_F_STRUCT:
                                {       
                                    const cint_parameter_desc_t* sm; 
                                    CINT_PRINTF("struct %s {\n", dt.basetype.sp->name); 
                                    for(sm = dt.basetype.sp->struct_members; sm->basetype; sm++) {
                                        cint_datatype_t dt;
                                        dt.desc = *sm; 
                                        CINT_PRINTF("    %s %s;\n", cint_datatype_format(&dt, 0), sm->name); 
                                    }
                                    CINT_PRINTF("}\n"); 
                                    CINT_PRINTF("size is %d bytes\n", dt.basetype.sp->size); 
                                    break; 
                                }
                            case CINT_DATATYPE_F_ENUM:
                                {
                                    const cint_enum_map_t* ep; 
                                    CINT_PRINTF("enum %s {\n", dt.basetype.ep->name); 
                                    for(ep = dt.basetype.ep->enum_map; ep->name; ep++) {
                                        CINT_PRINTF("    %s = %d\n", ep->name, ep->value);      
                                    }   
                                    CINT_PRINTF("}\n"); 
                                    break; 
                                }
                            case CINT_DATATYPE_F_FUNC_POINTER:
                                {
                                    cint_parameter_desc_t* p = dt.basetype.fpp->params;
                                    CINT_PRINTF("function pointer: '%s (*%s)", cint_datatype_format_pd(p, 0), dt.basetype.fpp->name);  
                                    cint_fparams_print(p+1); 
                                    CINT_PRINTF("'\n"); 
                                    break; 
                                }
                            case CINT_DATATYPE_F_TYPEDEF:
                                {
                                    /* Nothing yet */
                                    break; 
                                }
                            }   
                    }   
                else {
                        /* Should never get here */
                    }
                    break; 
                }
            default:
                {
                    v = cint_eval_ast(ast->utype.print.expression); 

                    if(v) {
                        cint_variable_print(v, 0, v->name);     
                    }   
                    else {
                        /* No error necessary -- an error would have been printed during the evaluation */
                    }
                    break; 
                }   
            }       
    }   

    return v; 
}
