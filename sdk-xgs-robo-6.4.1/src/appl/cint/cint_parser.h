/*
 * $Id: cint_parser.h,v 1.10 Broadcom SDK $
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
 * File:        cint_parser.h
 * Purpose:     CINT parser interfaces
 *
 */

/*******************************************************************************
 *
 * CINT C Parser Interface
 *
 * This is the object-oriented interface to the lexical scanner
 * and generated parser state machine. 
 *
 ******************************************************************************/

#ifndef __CINT_CPARSER_H__
#define __CINT_CPARSER_H__

#include "cint_config.h"
#include "cint_ast.h"

typedef struct cint_cparser_s {

    /* Private Implementation Members */
    cint_ast_t* result; 
    void* scanner; 
    void* parser; 
    int error; 

} cint_cparser_t; 


/*
 * Create a C Parser Instance
 */
extern cint_cparser_t* cint_cparser_create(void); 


/*
 * Set the input file handle
 */
int cint_cparser_start_handle(cint_cparser_t* cp, void* handle); 

/*
 * Set the input string
 */
int cint_cparser_start_string(cint_cparser_t* cp, const char* string); 

/*
 * Parse one C translation unit from the current input handle.  
 */
cint_ast_t* cint_cparser_parse(cint_cparser_t* cp); 

/* 
 * Parse one C translation unit from the given input string. 
 */
cint_ast_t* cint_cparser_parse_string(const char* string); 

/*
 * Retrieve the error code from the last request for parsing
 */
int cint_cparser_error(cint_cparser_t* cp); 

/*
 * Destroy a C Parser Instance
 */
int cint_cparser_destroy(cint_cparser_t* cp); 

/*
 * Handle a parser fatal error
 */
extern void cint_cparser_fatal_error(char *msg);

#if CINT_CONFIG_INCLUDE_PARSER_READLINE == 1
extern int cint_cparser_input_readline(void *in, char* buf,
                                       int* result, int max_size, int prompt);
#else

/*
 * Default character input
 */
extern int cint_cparser_input_default(void *in, char* buf,
                                      int* result, int max_size, int prompt);

/*
 * Default character input with echo
 */
extern int cint_cparser_input_default_echo(void *in,
                                           char* buf,
                                           int* result, int max_size,
                                           int prompt, int echo);

#endif /* CINT_CONFIG_INCLUDE_PARSER_READLINE */

/*
   Interfaces to library functions.  Declaring here avoids painful
   interactions with other header files and helps keep track of
   portability interfaces.
*/

extern void *cint_cparser_alloc(unsigned int size);
extern void *cint_cparser_realloc(void *ptr,  unsigned int size);
extern void cint_cparser_free(void *ptr);
extern void *cint_cparser_memcpy(void *dst, const void *src, int len);
extern void *cint_cparser_memset(void *dst, int c, int len);
extern void cint_cparser_message(const char *msg, int len);
extern int cint_cparser_interactive(void); 
extern int cint_cparser_include(int level);
extern const char *cint_cparser_set_prompt(const char *prompt);

#endif /* __CINT_CPARSER_H__ */
    
