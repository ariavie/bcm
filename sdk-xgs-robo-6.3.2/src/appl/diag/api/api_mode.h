/* 
 * $Id: api_mode.h 1.3 Broadcom SDK $
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
 * File:        api_mode.h
 * Purpose:     API mode header
 */

#ifndef   _API_MODE_H_
#define   _API_MODE_H_

#include "tokenizer.h"
#include "cint_datatypes.h"
#include "shared/error.h"

#define API_MODE_E_NONE         _SHR_E_NONE
#define API_MODE_E_INTERNAL     _SHR_E_INTERNAL
#define API_MODE_E_MEMORY       _SHR_E_MEMORY
#define API_MODE_E_UNIT         _SHR_E_UNIT
#define API_MODE_E_PARAM        _SHR_E_PARAM
#define API_MODE_E_EMPTY        _SHR_E_EMPTY
#define API_MODE_E_FULL         _SHR_E_FULL
#define API_MODE_E_NOT_FOUND    _SHR_E_NOT_FOUND
#define API_MODE_E_EXISTS       _SHR_E_EXISTS
#define API_MODE_E_TIMEOUT      _SHR_E_TIMEOUT
#define API_MODE_E_BUSY         _SHR_E_BUSY
#define API_MODE_E_FAIL         _SHR_E_FAIL
#define API_MODE_E_DISABLED     _SHR_E_DISABLED
#define API_MODE_E_BADID        _SHR_E_BADID
#define API_MODE_E_RESOURCE     _SHR_E_RESOURCE
#define API_MODE_E_CONFIG       _SHR_E_CONFIG
#define API_MODE_E_UNAVAIL      _SHR_E_UNAVAIL
#define API_MODE_E_INIT         _SHR_E_INIT
#define API_MODE_E_PORT         _SHR_E_PORT

#define API_MODE_SUCCESS(_rv) _SHR_E_SUCCESS(_rv) 
#define API_MODE_FAILURE(_rv) _SHR_E_FAILURE(_rv) 
#define API_MODE_IF_ERROR_RETURN(_rv) _SHR_E_IF_ERROR_RETURN(_rv) 

#define HELP '?'
#define INFO '!'

#define IS_FIRST     0x0001
#define IS_VAR       0x0002
#define IS_TYPE      0x0004

typedef struct api_mode_arg_s {
    int flags;
    int kind;
    const char *value;
    api_mode_token_t *token;
    const cint_datatype_t *dt;
    struct api_mode_arg_s *sub;         /* composite or keyword:value */
    struct api_mode_arg_s *parent;      /* parent of sub */
    struct api_mode_arg_s *next;        /* next in arg list */
    struct api_mode_arg_s *mm;          /* next in memory management list */
} api_mode_arg_t;

typedef int (*api_mode_arg_cb_t)(api_mode_arg_t *arg, void *user_data);

typedef struct api_mode_parse_s {
    int verbose;
    int done;
    int result;
    api_mode_arg_t *root;
    api_mode_arg_t *base;
    api_mode_arg_cb_t callback;
    void *user_data;
} api_mode_parse_t;

extern int api_mode_debug;

extern int api_mode_token(const char *s, void *parser,
                          int kind, api_mode_arg_t **arg);
extern int api_mode_get(void *p, char *buf, int max_size);

extern api_mode_arg_t *api_mode_node(void *prs, const char *value, int kind);
extern api_mode_arg_t *api_mode_execute(void *prs, api_mode_arg_t *arg);
extern api_mode_arg_t *api_mode_mark(api_mode_arg_t *arg, int flag);
extern api_mode_arg_t *api_mode_sub(api_mode_arg_t *pri, api_mode_arg_t *sec);
extern api_mode_arg_t *api_mode_key_value(api_mode_arg_t *key,
                                          api_mode_arg_t *value);
extern api_mode_arg_t *api_mode_range(api_mode_arg_t *from,
                                      api_mode_arg_t *to,
                                      api_mode_arg_t *times,
                                      api_mode_arg_t *incr);
extern api_mode_arg_t *api_mode_append(api_mode_arg_t *src,
                                       api_mode_arg_t *arg);
extern api_mode_arg_t *api_mode_sub_append(api_mode_arg_t *src,
                                           api_mode_arg_t *arg);

extern api_mode_arg_t *api_mode_arg_next(api_mode_arg_t *arg);

#define PARSE_VERBOSE 0x0001
#define PARSE_DEBUG   0x0002
#define SCAN_DEBUG    0x0004

extern int api_mode_parse_string(const char *input, int flags,
                                 api_mode_arg_cb_t cb, void *user_data);
extern void api_mode_show(int indent, api_mode_arg_t *start);
extern int api_mode_process_command(int u, char *s);
extern int api_mode_unexpected(void);

#endif /* _API_MODE_H_ */
