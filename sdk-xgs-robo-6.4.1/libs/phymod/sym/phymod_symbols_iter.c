/*
 * $Id: 1ebe9cc455f8774405f55c047261d6023c31fda8 $
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
 * PHYMOD Shell Utility: SYMOPS
 *
 * These utility functions provide all of the symbolic
 * register/memory encoding and decoding. 
 *
 */

#include <phymod/phymod_system.h>
#include <phymod/phymod_symbols.h>

static char *
_strtolower(char *dst, const char *src, int dmax)
{
    const char *s = src; 
    char *d = dst; 
    
    while (*s && --dmax) {
        *d = *s;
        if (*s >= 'A' && *s <= 'Z') {
            *d += ('a' - 'A'); 
        }
        s++;
        d++;
    }
    *d = 0;

    return dst; 
}

static int
_sym_match(phymod_symbols_iter_t *iter, const char *sym_name)
{
    char search_name[64]; 
    char symbol_name[64]; 

    _strtolower(search_name, iter->name, sizeof(search_name)); 
    _strtolower(symbol_name, sym_name, sizeof(symbol_name));

    switch(iter->matching_mode) {
    case PHYMOD_SYMBOLS_ITER_MODE_EXACT:
        if (PHYMOD_STRCMP(search_name, symbol_name) == 0) {
            /* Name matches */
            return 1;
        }
        break; 
    case PHYMOD_SYMBOLS_ITER_MODE_START:
        if (PHYMOD_STRNCMP(symbol_name, search_name,
                        PHYMOD_STRLEN(search_name)) == 0) {
            /* Name matches */
            return 1;
        }
        break;
    case PHYMOD_SYMBOLS_ITER_MODE_STRSTR:
        if (PHYMOD_STRSTR(symbol_name, search_name) != NULL) {
            /* Name matches */
            return 1;
        }
        break; 
    default:
        break;
    }
    return 0;
}

int
phymod_symbols_iter(phymod_symbols_iter_t *iter)
{       
    int count = 0; 
    int rc; 
    int match;
    uint32_t idx; 
    phymod_symbol_t s; 

    for (idx = 0; phymod_symbols_get(iter->symbols, idx, &s) >= 0; idx++) {

        if (s.name == 0) {
            /* Last */
            continue;
        }

        /* Check flags which must be present */
        if (iter->pflags && ((s.flags & iter->pflags) != iter->pflags)) {
            /* Symbol does not match */
            continue; 
        }

        /* Check flags which must be absent */
        if (iter->aflags && ((s.flags & iter->aflags) != 0)) {
            /* Symbol does not match */
            continue;
        }

        /* Check the name */
        if (PHYMOD_STRCMP("*", iter->name) != 0) {
            /* Not wildcarded */
            match = 0;
            if (_sym_match(iter, s.name)) {
                match = 1;
            }
#if PHYMOD_CONFIG_INCLUDE_ALIAS_NAMES == 1
            else if (s.ufname && _sym_match(iter, s.ufname)) {
                match = 1;
            }
            else if (s.alias && _sym_match(iter, s.alias)) {
                match = 1;
            }
#endif
            if (!match) {
                continue;
            }
        }

        /* Whew, name is okay */
        count++; 

        if ((rc = iter->function(&s, iter->vptr)) < 0) {
            return rc;
        }
    }
    
    return count; 
}
