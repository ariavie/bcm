%{
/*
 * $Id: api_grammar.y,v 1.2 Broadcom SDK $
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
 * File:        api_grammar.y
 * Purpose:     API mode parser
 *
 */
%}

/* API mode grammar */

%{
#include "api_mode_yy.h"
extern void api_mode_error(yyltype *loc, yyscan_t yyscanner,
                           void *parser, char const *s);
%}

%right '='

%define api.pure
%define api.push_pull "both"
%name-prefix="api_mode_"
%locations
%debug
%verbose
%error-verbose

%lex-param   { yyscan_t yyscanner }
%parse-param { yyscan_t yyscanner }
%parse-param { void *parser }

%token IDENT
%token KEY
%token CONSTANT
%token AGGREGATE
%token PROMPT
%token EMPTY
%token KEY_VALUE
%token RANGE
%token ITEM
%token ASSIGN
%token VALUE
%token PRINT
%token VAR
%token CREATE

%%

statements
    : statement
      { api_mode_execute(parser, $1); YYACCEPT; }
    | statements ';' statement
      { $$ = api_mode_append($1, api_mode_append($2,$3)); }
    ;

statement
    : executable
    | '!'                                       /* set info mode */
    | '!' executable
        { $$ = api_mode_append($1,$2); }        /* info */
    | identifiers '?'                           /* help */
        { $$ = api_mode_append($1,$2); }
    | PRINT selector_list                       /* print */
        { $$ = api_mode_append($1,$2); }
    | CREATE identifiers keys                   /* type constructor */
        { $$ =  api_mode_append($1,api_mode_append($2,$3)); }
    | VAR selector_list '=' argument           /* variable assignment */
        { $$ = api_mode_append($1,api_mode_append($2,$4)); }
    | VAR selectors PROMPT                      /* prompt-mode assignment */
        { $$ = api_mode_append($1,api_mode_append($2,$3)); }
    ;

executable
    : identifiers
    | identifiers PROMPT                        /* prompt */
        { $$ = api_mode_append($1,$2); }
    | identifiers positional_arguments          /* execute */
        { $$ = api_mode_append($1,$2); }
    | identifiers keyword_arguments             /* execute */
        { $$ = api_mode_append($1,$2); }
    ; 

identifiers
    : IDENT
        { $$ = api_mode_mark($1, IS_FIRST); }
    | identifiers IDENT
        { $$ = api_mode_append($1,$2); }
    ;

selectors
    : IDENT
        { $$ = api_mode_mark($1, IS_FIRST); }
    | selectors '.' IDENT
        { $$ = api_mode_sub_append($1,$3); }
    ;

selector_list
    : selectors
        { $$ = api_mode_mark($1, IS_FIRST); }
    | selector_list selectors
        { $$ = api_mode_append($1,$2); }
    ;

keys
    : KEY
        { $$ = api_mode_mark($1, IS_FIRST); }
    | keys KEY
        { $$ = api_mode_append($1,$2); }
    ;

positional_arguments
    : argument
        { $$ = api_mode_mark($1, IS_FIRST); }
    | positional_arguments argument
        { $$ = api_mode_append($1,$2); }
    ;

positional_item
    : ITEM
    | ITEM '-' ITEM
        { $$ = api_mode_range($1, $3, NULL, NULL); }
    | ITEM '*' CONSTANT
        { $$ = api_mode_range($1, NULL, NULL, $3); }
    | ITEM '-' ITEM '/' CONSTANT
        { $$ = api_mode_range($1, $3, NULL, $5); }
    | ITEM '*' ITEM '/' CONSTANT
        { $$ = api_mode_range($1, NULL, $3, $5); }
    ;

positional_list
    : positional_item
        { $$ = api_mode_mark($1, IS_FIRST); }
    | positional_list ',' positional_item
        { $$ = api_mode_append($1,$3); }
    ;

keyword_arguments
    : keyword_argument
        { $$ = api_mode_mark($1, IS_FIRST); }
    | keyword_arguments keyword_argument
        { $$ = api_mode_append($1,$2); }
    ;

keyword_argument
    : KEY '=' argument
        { $$ = api_mode_key_value($1,$3); }
    ;

argument
    : CONSTANT
    | VALUE
    | aggregate
    ;

aggregate
    : '{' aggregate_arguments '}'
        { $$ = api_mode_sub(api_mode_node(parser,"", AGGREGATE), $2); }
    | positional_list
        { $$ = api_mode_sub(api_mode_node(parser,"", AGGREGATE), $1); }
    ;

aggregate_arguments
    : keyword_arguments
    | positional_arguments
    | '='
        { $$ = api_mode_node(parser,"{=}", PROMPT); }
    ;

