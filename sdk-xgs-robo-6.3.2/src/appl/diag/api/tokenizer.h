/* 
 * $Id: tokenizer.h 1.1 Broadcom SDK $
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
 * File:        tokenizer.h
 * Purpose:     API mode tokenizer
 */

#ifndef   _TOKENIZER_H_
#define   _TOKENIZER_H_

#include "cint_interpreter.h"

typedef enum api_mode_token_type_e {
    API_MODE_TOKEN_TYPE_NONE,
    API_MODE_TOKEN_TYPE_VALUE,
    API_MODE_TOKEN_TYPE_QUOTE,
    API_MODE_TOKEN_TYPE_SEPARATOR,
    API_MODE_TOKEN_TYPE_COMMENT,
    API_MODE_TOKEN_TYPE_ERROR
} api_mode_token_type_t;

typedef enum api_mode_token_error_e {
    API_MODE_TOKEN_ERROR_NONE,
    API_MODE_TOKEN_ERROR_INIT,
    API_MODE_TOKEN_ERROR_ESCAPE,
    API_MODE_TOKEN_ERROR_QUOTE,
    API_MODE_TOKEN_ERROR_CHAR,
    API_MODE_TOKEN_ERROR_UNEXPECTED,
    API_MODE_TOKEN_ERROR_STATE,
    API_MODE_TOKEN_ERROR_LAST
} api_mode_token_error_t;

typedef struct api_mode_token_s {
    const char *str;                    /* NUL terminated token string */
    int first_line;                     /* source buffer start line */
    int first_column;                   /* source buffer start column */
    int last_line;                      /* source buffer end line */
    int last_column;                    /* source buffer end column */
    int ident;                          /* indentifier? */
    api_mode_token_type_t token_type;   /* tokenizer type */
} api_mode_token_t;

typedef struct api_mode_tokens_s {
    int len;                            /* number of valid tokens */
    api_mode_token_error_t error;       /* error state */
    char *buf;                          /* token data memory buffer */
    api_mode_token_t *token;            /* array of 'len' tokens */
} api_mode_tokens_t;


extern int api_mode_tokenizer(const char *input, api_mode_tokens_t *tokens);
extern int api_mode_tokenizer_free(api_mode_tokens_t *tokens);
extern const char *api_mode_tokenizer_error_str(api_mode_token_error_t error);

#endif /* _TOKENIZER_H_ */
