/* 
 * $Id: 663227a48fe2fddd58eef3db46ed82861ebbc874 $
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
 * File:        context.h
 * Purpose:     API mode contextualizer interface
 */

#ifndef   _CONTEXT_H_
#define   _CONTEXT_H_

#include "tokenizer.h"
#include "api_mode_yy.h"
#include "api_grammar.tab.h"
#include "api_grammar.tab.h"
#include "cint_variables.h"

/* Return TRUE if 'c' is an uppercase letter */
#define UPPERCASE(c) (((c) >= 'A') && ((c) <= 'Z'))

#define CONTEXT_MAX_PFX 10
#define CONTEXT_MAX_BUFFER 256

typedef enum api_mode_context_qual_e {
    api_mode_context_qual_unknown,      /* Unknown grammar */
    api_mode_context_qual_function,     /* Function grammar */
    api_mode_context_qual_variable,     /* variable assignment grammar */
    api_mode_context_qual_print,        /* print grammar */
    api_mode_context_qual_create        /* type construction grammar */
} api_mode_context_qual_t;

typedef struct api_mode_private_s {
    const char *name;                   /* func name */
    enum yytokentype grammar_type;      /* grammar type */
    api_mode_context_qual_t qual;       /* qualification */
} api_mode_private_t;

/* Note - dt is a *copy*, not a pointer, because the dt passed to
   the traverse is constructed 'on the fly' and on the stack of the
   CINT traverse. */

typedef struct api_mode_cint_dt_db_entry_s {
    const char *name;                   /* CINT name */
    api_mode_private_t *private;        /* private API mode command */
    const cint_datatype_t dt;           /* CINT dt entry */
} api_mode_cint_dt_db_entry_t;

typedef struct api_mode_cint_dt_db_s {
    int count;                  /* total number of entries */
    int alloc;                  /* entried allocated */
    int idx;                    /* insertion index */
    int max;                    /* maximum length */
    api_mode_cint_dt_db_entry_t *entry;
} api_mode_cint_dt_db_t;

typedef struct api_mode_cint_var_db_s {
    int count;                  /* total number of entries */
    int alloc;                  /* entried allocated */
    int idx;                    /* insertion index */
    int max;                    /* maximum length */
    cint_variable_t *entry;
} api_mode_cint_var_db_t;

typedef struct api_mode_context_s {
    api_mode_context_qual_t qual; /* Context grammar qualification */
    char        *alloc;         /* allocation buffer */
    char        *match;         /* context match buffer */
    char        *join;          /* arg join pointer in match buffer */
    int         idx;            /* name table index */
    int         exact;          /* exact match */
    int         more;           /* overmatch */
    int         partial;        /* partial match */
    api_mode_cint_dt_db_entry_t *dt0; /* first DT match */
    api_mode_cint_dt_db_t *db;  /* function database */
    int         len;            /* number of info records */
    int         jlen;           /* join buffer length */
    int         mlen;           /* max match buffer length */
    int         num_ident;      /* number of leading identifiers */
    int         num_arg;        /* number of arguments */
    struct {
        enum yytokentype grammar_type;
        const cint_datatype_t *dt;
    } *info;
} api_mode_context_t;


extern int api_mode_contextualizer(api_mode_tokens_t *tokens,
                                   api_mode_context_t *ctx);

extern int api_mode_contextualizer_free(api_mode_context_t *ctx);

extern char *api_mode_bcm_prefix(char *buffer, int len, const char *name);

extern int api_mode_context_initialize(void);
extern int api_mode_context_uninitialize(void);

#endif /* _CONTEXT_H_ */
