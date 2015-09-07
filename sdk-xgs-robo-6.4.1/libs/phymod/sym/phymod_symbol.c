/*
 * $Id: 33a8a6e5e41295eec330c77df5619da8d6296b87 $
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

/*******************************************************************************
 *
 * PHYMOD Symbol Routines
 *
 *
 ******************************************************************************/

#include <phymod/phymod_symbols.h>

#if PHYMOD_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

static const void *
__phymod_symbol_find(const char *name, const void *table, int size, int entry_size)
{
    int i; 
    phymod_symbol_t *sym;
    unsigned char *ptr = (unsigned char*)table; 

    if (table == NULL) {
        return NULL;
    }
    
    for (i = 0; (sym = (phymod_symbol_t*)(ptr)) && (i < size); i++) {
	if (PHYMOD_STRCMP(sym->name, name) == 0) {
	    return (void*) sym; 
	}
#if PHYMOD_CONFIG_INCLUDE_ALIAS_NAMES == 1
	if (sym->ufname && PHYMOD_STRCMP(sym->ufname, name) == 0) {
	    return (void*) sym; 
	}
	if (sym->alias && PHYMOD_STRCMP(sym->alias, name) == 0) {
	    return (void*) sym; 
	}
#endif
	ptr += entry_size; 
    }

    return NULL; 
}
	

const phymod_symbol_t *
phymod_symbol_find(const char *name, const phymod_symbol_t *table, int size)
{
    return (phymod_symbol_t*) __phymod_symbol_find(name, table, size, sizeof(phymod_symbol_t)); 
}


int 
phymod_symbols_find(const char *name, const phymod_symbols_t *symbols, phymod_symbol_t *rsym)
{
    const phymod_symbol_t *s = NULL; 

    if (rsym == NULL) return -1; 

    if ((symbols->symbols) && (s = phymod_symbol_find(name, symbols->symbols, symbols->size))) {
	*rsym = *s;
	return 0; 
    }
    return -1;
}

#endif
