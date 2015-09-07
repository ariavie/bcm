/* 
 * $Id: api_mode.c 1.2 Broadcom SDK $
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
 * File:        api_mode.c
 * Purpose:     API mode grammar implementation
 *
 */

#include "sal/core/alloc.h"
#include "sal/core/libc.h"
#include "sal/appl/io.h"
#include "tokenizer.h"
#include "context.h"
#include "completion.h"
#include "api_mode_yy.h"
#include "api_grammar.tab.h"


/*
 * API Mode grammar handler functions
 */


/* allocate a grammar node */
STATIC api_mode_arg_t *
alloc_node(api_mode_parse_t *prs,
           const char *value, int kind,
           api_mode_token_t *token, const cint_datatype_t *dt)
{
    api_mode_arg_t *arg;

    arg = sal_alloc(sizeof(*arg),"api_mode");
    if (!arg) {
        return NULL;
    }
    sal_memset(arg,0,sizeof(*arg));
    arg->kind = kind;
    arg->value = sal_strdup(value);
    arg->token = token;
    arg->dt = dt;
    arg->mm = prs->root;
    prs->root = arg;
    if (prs->base == NULL) {
        prs->base = arg;
    }

    return arg;
}

/* free a grammar node */
STATIC void
free_node(api_mode_arg_t *arg)
{
    sal_free((void *)arg->value);
    sal_memset(arg, 0, sizeof(*arg));
    sal_free(arg);
}

/* free a list of grammar nodes */
STATIC void
free_node_list(api_mode_arg_t *arg)
{
    api_mode_arg_t *next; 
    while (arg) {
        next = arg->mm;
        free_node(arg);
        arg = next;
    }
}

/* return a string corresponding to a node kind id */
STATIC char *
node_kind(int x)
{
    char *s = "???";
    
    switch (x) {
        case IDENT: s="IDENT"; break;
        case KEY: s="KEY"; break;
        case CONSTANT: s="CONSTANT"; break;
        case AGGREGATE: s="{}"; break;
        case PROMPT: s="PROMPT"; break;
        case EMPTY: s="<>"; break;
        case KEY_VALUE: s="="; break;
        case RANGE: s="RANGE"; break;
        case ITEM: s="ITEM"; break;
        case ASSIGN: s="ASSIGN"; break;
        case VALUE: s="VALUE"; break;
        case PRINT: s="PRINT"; break;
        case VAR: s="VAR"; break;
        case CREATE: s="CREATE"; break;
        case '!': s="INFO"; break;
        case '=': s="="; break;
        case ';': s=";"; break;
        case '?': s="?"; break;
        case '.': s="."; break;
        case ',': s=","; break;
        case '{': s="{"; break;
        case '}': s="}"; break;
    }

    return s;
}

/* print node information */
STATIC void
show_node(int indent, api_mode_arg_t *arg)
{
    sal_printf("%*s%s %s %s\n",
               indent*2,"",
               node_kind(arg->kind),
               arg->value,
               arg->flags&IS_FIRST?"+":"");
}

/* set the parent for a list of child nodes */
STATIC void
set_parent(api_mode_arg_t *arg, api_mode_arg_t *parent)
{
    for (; arg; arg=arg->next) {
        arg->parent = parent;
    }
}

/* print node information for a tree of nodes */
void
api_mode_show(int indent, api_mode_arg_t *start)
{
    api_mode_arg_t *arg;

    for (arg=start; arg; arg=arg->next) {
        show_node(indent,arg);
        if (arg->sub) {
            api_mode_show(indent+1, arg->sub);
        }
    }
}

/* allocate a node of a certain kind with a value */
api_mode_arg_t *
api_mode_node(void *prs, const char *value, int kind)
{
    return alloc_node((api_mode_parse_t *)prs, value, kind, NULL, NULL);
}

/* execute a grammar tree via a parser callback */
api_mode_arg_t *
api_mode_execute(void *p, api_mode_arg_t *arg)
{
    api_mode_parse_t *prs = (api_mode_parse_t *)p;

    if (prs->verbose) {
        prs->result = prs->callback(arg, prs->user_data);
    }
    
    return arg;
}

/* set node flags */
api_mode_arg_t *
api_mode_mark(api_mode_arg_t *arg, int flag)
{
    arg->flags |= flag;

    return arg;
}

/* set 'sec' as a sublist of 'pri' */
api_mode_arg_t *
api_mode_sub(api_mode_arg_t *pri, api_mode_arg_t *sec)
{
    pri->sub = sec;
    set_parent(sec, pri);

    return pri;
}

/* create a key/value node */
api_mode_arg_t *
api_mode_key_value(api_mode_arg_t *key, api_mode_arg_t *value)
{
    key->kind = KEY_VALUE;
    api_mode_sub(key, value);
    return key;
}

/* create a range node */
api_mode_arg_t *
api_mode_range(api_mode_arg_t *from, api_mode_arg_t *to,
               api_mode_arg_t *times, api_mode_arg_t *incr)
{
    from->kind = RANGE;
    api_mode_append(incr, times);
    api_mode_append(times, to);
    api_mode_sub(from, to);
    return from;
}

/* append 'arg' to node list 'src' */
api_mode_arg_t *
api_mode_append(api_mode_arg_t *src, api_mode_arg_t *arg)
{
    api_mode_arg_t *target;

    for (target=src; target->next; target=target->next) {
        if (target == target->next) {
            (void)api_mode_unexpected();
            break;
        }
    }

    target->next = arg;
    if (src->parent != NULL && arg->parent == NULL) {
        arg->parent = src->parent;
    }

    return src;
}

/* append 'arg' to child node list of 'src' */
api_mode_arg_t *
api_mode_sub_append(api_mode_arg_t *src, api_mode_arg_t *arg)
{
    if (src->sub == NULL) {
        src->sub = arg;
    } else {
        api_mode_append(src->sub, arg);
    }
    set_parent(arg, src);

    return src;
}

/* display a parse error */
void
api_mode_error(yyltype *loc, yyscan_t yyscanner, void *p, char const *s)
{
    api_mode_parse_t *prs = (api_mode_parse_t *)p;

    COMPILER_REFERENCE(loc);
    COMPILER_REFERENCE(yyscanner);
    sal_printf("%s\n", s);
    prs->done = 1;
}

/* return the next node after 'arg'. if arg is the last child node,
   recurse back up to the next parent node. */
api_mode_arg_t *
api_mode_arg_next(api_mode_arg_t *arg)
{
    api_mode_arg_t *next = NULL;

    if (arg->next) {
        next = arg->next;
    } else if (arg->parent) {
        next = api_mode_arg_next(arg->parent);
    }

    return next;
}


/* Pull parser lex function -
   not used, but still referenced by the pull parser */

int
api_mode_lex(YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner)
{
    COMPILER_REFERENCE(lvalp);
    COMPILER_REFERENCE(llocp);
    COMPILER_REFERENCE(scanner);

    return api_mode_unexpected();
}

/* show scanner tokens for diagnostic purposes */
STATIC void
show_scanner_tokens(yyscan_t scanner, api_mode_context_t *ctx)
{
    int i;

    for (i=0; i<scanner->tok.len; i++) {
        sal_printf("%s ", node_kind(ctx->info[i].grammar_type));
    }
    sal_printf("\n");
}
   
/*
 *
 * API Mode parser
 *
 */

/*
  parse the output of the scanner
 */
STATIC int
api_mode_parser(yyscan_t scanner, api_mode_context_t *ctx, int flags,
                api_mode_arg_cb_t cb, void *user_data)
{
    int status;
    api_mode_pstate *ps;
    enum yytokentype code;
    api_mode_parse_t parser;
    api_mode_arg_t *yylval;
    yyltype_t yylloc;
    api_mode_token_t *token;
    const cint_datatype_t *dt;

    /* Setup parser context */
    sal_memset(&parser, 0, sizeof(parser));

    if (flags & PARSE_VERBOSE) {
        parser.verbose = 1;
    }

    if (flags & PARSE_DEBUG) {
        api_mode_debug = 1;
    }

    parser.callback = cb;
    parser.user_data = user_data;
    scanner->idx = 0;
    status = 0;

    if (api_mode_debug) {
        show_scanner_tokens(scanner, ctx);
    }
    /* Call scanner and parser */
    ps = api_mode_pstate_new();
    do {
        if (scanner->idx < scanner->tok.len) {
            token = &scanner->tok.token[scanner->idx];
            code = ctx->info[scanner->idx].grammar_type;
            dt = ctx->info[scanner->idx].dt;
            yylval = alloc_node(&parser, token->str, code, token, dt);
            if (yylval == NULL) {
                status = API_MODE_E_FAIL;
                break;
            }
            /* location */
            yylloc.first_line   = token->first_line;
            yylloc.first_column = token->first_column;
            yylloc.last_line    = token->last_line;
            yylloc.last_column  = token->last_column;
            /* next token */
            scanner->idx++;
        } else {
            /* no more tokens; signal end-of-input */
            code = 0;
            yylval = NULL;
            yylloc.first_line   = -1;
            yylloc.first_column = -1;
            yylloc.last_line    = -1;
            yylloc.last_column  = -1;
        }

        status = api_mode_push_parse(ps, code, &yylval, &yylloc,
                                     scanner, &parser);
        
    } while (status == YYPUSH_MORE);
    api_mode_pstate_delete(ps);

    /* free resources */
    free_node_list(parser.root);

    if (scanner->idx < scanner->tok.len) {
        printf("api_mode_parser: %d tokens left over\n",
               scanner->tok.len-scanner->idx);
    }
    
    return status;
}

/*
  Parse an API mode string. This is the top level interface to the
  API mode parser.

  The string 'input' is parsed, and the callback is called when the
  parse input is exhausted.

  In practice, this function is called either in some sort of
  completion context, where the callback is used to facilitate command
  or argument completion, or an execution context, where the callback
  is used to execute the parsed string.

  In principle, it's possible to reuse parse information from a
  completion, it's not clear that it's worth the complexity, so
  completion and execution are considered separate contexts, where
  completion just facilitates building up the command line, and
  execution parses the final result. This assumes that parsing is not
  very expensive. If it is, then some sort of caching between completion
  and execution will be needed.
  
 */
int
api_mode_parse_string(const char *input, int flags,
                      api_mode_arg_cb_t cb,
                      void *user_data)
{
    int rv = API_MODE_E_FAIL;
    api_mode_scanner_t scan;
    api_mode_context_t ctx;
    api_mode_completion_type_t ctype;

    rv = api_mode_tokenizer(input, &scan.tok);
    if (!rv) {
        /* tokenized OK, now contextualize */
        if (scan.tok.len > 0) {
            rv = api_mode_contextualizer(&scan.tok, &ctx);
            if (!rv) {
                /* ignore errors at this point, because if the parser errors
                   out, don't pass the command to the shell. */
                ctype = api_mode_completion_set(api_mode_completion_none);
                (void)api_mode_parser(&scan, &ctx, flags, cb, user_data);
                (void)api_mode_completion_set(ctype);
            }
            api_mode_contextualizer_free(&ctx);
        }
    }

    (void)api_mode_tokenizer_free(&scan.tok);

    return rv;
}

/* called for unexpected conditions */
int
api_mode_unexpected(void)
{
    sal_printf("Unexpected condition.\n");

    return API_MODE_E_INTERNAL;
}

