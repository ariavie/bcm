/*
 * $Id: 172ec9039af55b66cefe88605e72d1a6c8f49544 $
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
 */

#include <phymod/phymod_symbols.h>

static size_t
phymod_strlcpy(char *dest, const char *src, size_t cnt)
{
    char *ptr = dest;
    size_t copied = 0;

    while (*src && (cnt > 1)) {
	*ptr++ = *src++;
	cnt--;
	copied++;
    }
    *ptr = '\0';

    return copied;
}

/*
 * Function:
 *	phymod_symbol_field_filter
 * Purpose:
 *	Callback for Filtering fields based on current data view.
 * Parameters:
 *	symbol - symbol information
 *	fnames - list of all field names for this device
 *	encoding - key for decoding overlay
 *	cookie - context data
 * Returns:
 *      Non-zero if field should be filtered out (not displayed)
 * Notes:
 *      The filter key has the following syntax:
 *
 *        {[<keysrc>]:<keyfield>:<keyval>[|<keyval> ... ]}
 *
 *      Ideally the keysrc is the same data entry which is
 *      being decoded, and in this case it can left out, e.g.:
 *
 *        {:KEY_TYPEf:1}
 *
 *      This example encoding means that if KEY_TYPEf=1, then
 *      this field is valid for this view.
 *
 *      Note that a filed can be for multiple views, e.g.:
 *
 *        {:KEY_TYPEf:1|3}
 *
 *      This example encoding means that this field is valid
 *      if KEY_TYPEf=1 or KEY_TYPEf=3.
 *
 *      The special <keyval>=-1 means that this field is valid
 *      even if there is no context (cookie=NULL).
 *
 *      Note that this filter code does NOT support a <keysrc>
 *      which is different from the current data entry.
 */
int 
phymod_symbol_field_filter(const phymod_symbol_t *symbol, const char **fnames,
                              const char *encoding, void *cookie)
{
#if PHYMOD_CONFIG_INCLUDE_FIELD_NAMES == 1
    uint32_t *data = (uint32_t *)cookie;
    uint32_t val[PHYMOD_MAX_REG_WSIZE];
    phymod_field_info_t finfo; 
    char tstr[128];
    char *keyfield, *keyvals;
    char *ptr;
    int kval = -1;

    /* Do not filter if no (or unknown) encoding */
    if (encoding == NULL || *encoding != '{') {
        return 0;
    }

    /* Do not filter if encoding cannot be parsed */
    phymod_strlcpy(tstr, encoding, sizeof(tstr));
    ptr = tstr;
    if ((ptr = PHYMOD_STRCHR(ptr, ':')) == NULL) {
        return 0;
    }
    *ptr++ = 0;
    keyfield = ptr;
    if ((ptr = PHYMOD_STRCHR(ptr, ':')) == NULL) {
        return 0;
    }
    *ptr++ = 0;
    keyvals = ptr;

    /* Only show default view if no context */
    if (data == NULL) {
        return (PHYMOD_STRSTR(keyvals, "-1") == NULL) ? 1 : 0;
    }

    /* Look for <keyfield> in data entry */
    PHYMOD_SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo, fnames) {

        if (finfo.name && PHYMOD_STRCMP(finfo.name, keyfield) == 0) {
            /* Get normalized field value */
            PHYMOD_MEMSET(val, 0, sizeof(val));
            phymod_field_get(data, finfo.minbit, finfo.maxbit, val);
            kval = val[0];
            break;
        }

    } PHYMOD_SYMBOL_FIELDS_ITER_END(); 

    /* Check if current key matches any <keyval> in encoding */
    ptr = keyvals;
    while (ptr) {
        if (PHYMOD_STRTOUL(ptr, NULL, 0) == kval) {
            return 0;
        }
        if ((ptr = PHYMOD_STRCHR(ptr, '|')) != NULL) {
            ptr++;
        }
    }
#endif

    /* No match - filter this field */
    return 1; 
}
