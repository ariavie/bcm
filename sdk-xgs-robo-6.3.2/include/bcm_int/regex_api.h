/*
 * $Id: regex_api.h 1.8 Broadcom SDK $
 *
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
 */
#ifndef _RE_H_
#define _RE_H_

#if defined(INCLUDE_REGEX)

#define DFA_TRAVERSE_START      0x1
#define DFA_TRAVERSE_END        0x2
#define DFA_STATE_FINAL         0x4

typedef enum {
        REGEX_ERROR_NONE            = 0,
        REGEX_ERROR_INVALID_CLASS   = -1,
        REGEX_ERROR_EXPANSION_FAIL  = -2,
        REGEX_ERROR_NO_POST         = -3,
        REGEX_ERROR_NO_DFA          = -4,
        REGEX_ERROR_NO_NFA          = -5,
        REGEX_ERROR_NO_MEMORY       = -6,
        REGEX_ERROR                 = -7
} regex_error;

typedef enum {
    REGEX_CB_OK    = 0,
    REGEX_CB_ABORT,
} regex_cb_error_t;

#define BCM_TR3_REGEX_CFLAG_EXPAND_LCUC     0x01
#define BCM_TR3_REGEX_CFLAG_EXPAND_UC       0x02
#define BCM_TR3_REGEX_CFLAG_EXPAND_LC       0x04

/*
 * Call user provided callback to map a DFA State to HW.
 */
typedef regex_cb_error_t (*regex_dfa_state_cb)(unsigned int flags, 
                            int match_idx, int in_state, int from_c, int to_c, 
                            int to_state, int num_dfa_state, void *user_data);


int bcm_regex_compile(char **re, unsigned int *res_flags, 
                      int num_pattern, unsigned int cflags, void** dfa);

int bcm_regex_dfa_traverse(void *dfa, regex_dfa_state_cb compile_dfa_cb,
                            void *user_data);

int bcm_regex_dfa_free(void *dfa);

extern void _bcm_regex_sw_dump(int unit);

#endif /* INCLUDE_REGEX */

#endif /* _RE_H_ */
