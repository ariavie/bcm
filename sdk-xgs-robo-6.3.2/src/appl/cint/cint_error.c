/*
 * $Id: cint_error.c 1.12 Broadcom SDK $
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
 * File:        cint_error.c
 * Purpose:     CINT error handling functions
 *
 */

#include "cint_config.h"
#include "cint_porting.h"
#include "cint_error.h"

const char* 
cint_error_name(cint_error_t err)
{
    switch(err) {                       
#define CINT_ERROR_LIST_ENTRY(_entry) case CINT_E_##_entry : return #_entry ; 
#include "cint_error_entry.h"

        default: return "<UNKNOWN>";
    }
}       


void 
cint_internal_error(const char* f, int l, const char* fmt, ...)
{
    va_list args; 
    va_start(args, fmt); 
    
    CINT_PRINTF("*** internal error (%s:%d): ", f, l); 
    CINT_VPRINTF(fmt, args); 
    CINT_PRINTF("\n"); 
    va_end(args); 
    cint_errno = CINT_E_INTERNAL; 
}       

static void 
__cint_vmsg(const char* f, int l, const char* type, const char* fmt, va_list args)
     COMPILER_ATTRIBUTE((format (printf, 4, 0)));

static void 
__cint_vmsg(const char* f, int l, const char* type, const char* fmt, va_list args)
{
    if(f) {
        CINT_PRINTF("** %s:%d: %s: ", f, l, type); 
    }
    else if(l != 0) {
        CINT_PRINTF("** %d: %s: ", l, type); 
    }
    else {
        CINT_PRINTF("** %s: ", type); 
    }
    CINT_VPRINTF(fmt, args); 
    CINT_PRINTF("\n"); 
}

void
cint_error(const char* f, int l, cint_error_t e, const char* fmt, ...)
{
    va_list args; 
    va_start(args, fmt); 
    __cint_vmsg(f, l, "error", fmt, args); 
    va_end(args); 
    cint_errno = e; 
}

int
cint_ast_error(const cint_ast_t* ast, cint_error_t e, const char* fmt, ...)
{
    va_list args; 
    va_start(args, fmt); 
    __cint_vmsg(ast ? ast->file : NULL, 
                ast ? ast->line : 0, 
                "error", fmt, args); 
    va_end(args);
    cint_errno = e;

    return e; 
}       
    
int 
cint_ast_warn(const cint_ast_t* ast, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt); 
    __cint_vmsg(ast ? ast->file : NULL, 
                ast ? ast->line : 0,
                "warning", fmt, args); 
    va_end(args); 
    return 0; 
}
   
void 
cint_warn(const char* f, int l, const char* fmt, ...)
{
    va_list args; 
    va_start(args, fmt); 
    __cint_vmsg(f, l, "warning", fmt, args); 
    va_end(args); 
}
