/*
 * $Id: 1b457e775b06a38af35aa4f97d6aa7e120bf81fb $
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
 * PHYMOD Symbol Field Routines
 *
 *
 ******************************************************************************/

#include <phymod/phymod_symbols.h>

/*
 * Function:
 *	phymod_field_get
 * Purpose:
 *	Extract multi-word field value from multi-word register/memory.
 * Parameters:
 *	entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer where to store extracted field value
 * Returns:
 *      Pointer to extracted field value.
 */
uint32_t *
phymod_field_get(const uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
{
    int i, wp, bp, len;

    bp = sbit;
    len = ebit - sbit + 1;

    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (bp) {
            fbuf[i] = (entbuf[wp++] >> bp) & ((1 << (32 - bp)) - 1);
            if (len > (32 - bp)) {
                fbuf[i] |= entbuf[wp] << (32 - bp);
            }
        } else {
            fbuf[i] = entbuf[wp++];
        }
        if (len < 32) {
            fbuf[i] &= ((1 << len) - 1);
        }
    }

    return fbuf;
}

/*
 * Function:
 *	phymod_field_set
 * Purpose:
 *	Assign multi-word field value in multi-word register/memory.
 * Parameters:
 *	entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer with new field value
 * Returns:
 *      Nothing.
 */
int
phymod_field_set(uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
{
    uint32_t mask;
    int i, wp, bp, len;

    if (fbuf == NULL) {
        return -1;
    }

    bp = sbit;
    len = ebit - sbit + 1;
    if (len <= 0) {
        return -1;
    }

    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (bp) {
            if (len < 32) {
                mask = (1 << len) - 1;
            } else {
                mask = ~0;
            }
            entbuf[wp] &= ~(mask << bp);
            entbuf[wp++] |= fbuf[i] << bp;
            if (len > (32 - bp)) {
                entbuf[wp] &= ~(mask >> (32 - bp));
                entbuf[wp] |= fbuf[i] >> (32 - bp) & ((1 << bp) - 1);
            }
        } else {
            if (len < 32) {
                mask = (1 << len) - 1;
                entbuf[wp] &= ~mask;
                entbuf[wp++] |= (fbuf[i] & mask) << bp;
            } else {
                entbuf[wp++] = fbuf[i];
            }
        }
    }
    return 0;
}

uint32_t *
phymod_field_info_decode(uint32_t* fp, phymod_field_info_t* finfo, const char** fnames)
{
    if(!fp) {
        return NULL; 
    }

    if(finfo) {
        /*
         * Single or Double Word Descriptor?
         */
        if(PHYMOD_SYMBOL_FIELD_EXT(*fp)) {
            /* Double Word */
            finfo->fid = PHYMOD_SYMBOL_FIELD_EXT_ID_GET(*fp); 
            finfo->maxbit = PHYMOD_SYMBOL_FIELD_EXT_MAX_GET(*(fp+1)); 
            finfo->minbit = PHYMOD_SYMBOL_FIELD_EXT_MIN_GET(*(fp+1)); 
        }       
        else {
            /* Single Word */
            finfo->fid = PHYMOD_SYMBOL_FIELD_ID_GET(*fp); 
            finfo->maxbit = PHYMOD_SYMBOL_FIELD_MAX_GET(*fp); 
            finfo->minbit = PHYMOD_SYMBOL_FIELD_MIN_GET(*fp); 
        }       

        if(fnames) {
            finfo->name = fnames[finfo->fid]; 
        }
        else {
            finfo->name = NULL; 
        }       
    }

    if(PHYMOD_SYMBOL_FIELD_LAST(*fp)) {
        return NULL; 
    }

    if(PHYMOD_SYMBOL_FIELD_EXT(*fp)) {
        return fp+2; 
    }

    return fp+1; 
}

uint32_t
phymod_field_info_count(uint32_t* fp)
{    
    int count = 0; 
    while(fp) {
        fp = phymod_field_info_decode(fp, NULL, NULL); 
        count++; 
    }   
    return count; 
}
