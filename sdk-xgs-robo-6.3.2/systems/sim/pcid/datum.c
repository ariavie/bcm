/*
 * $Id: datum.c 1.7 Broadcom SDK $
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
 * The part of PCID that caches the register/memory values.
 *
 */

#include "pcid.h"
#include <soc/mem.h>

int
_soc_datum_read(pcid_info_t *pcid_info, int reg,
                uint32 selectors, uint32 addr, int words,
                uint32 *data)
{
    int index;
    soc_datum_t *d, **ht = reg ? pcid_info->reg_ht : pcid_info->mem_ht;

    index = SOC_HASH_EXTENDED_DATUM(selectors, addr);

    for (d = ht[index]; d != NULL; d = d->next) {
        if ((d->selectors == selectors) && (d->addr == addr)) {
            memcpy(data, d->data, words * 4);
            return SOC_E_NONE;
        }
    }
    return SOC_E_NOT_FOUND;
}

int
_soc_datum_write(pcid_info_t *pcid_info, int reg,
                 uint32 selectors, uint32 addr, int words,
                 uint32 *data)
{
    int index;
    soc_datum_t *d, **ht = reg ? pcid_info->reg_ht : pcid_info->mem_ht;

    index = SOC_HASH_EXTENDED_DATUM(selectors, addr);

    for (d = ht[index]; d != NULL; d = d->next) {
        if ((d->selectors == selectors) && (d->addr == addr)) {
            memcpy(d->data, data, words * 4);
            return SOC_E_NONE;
        }
    }
    return SOC_E_NOT_FOUND;
}

int
_soc_datum_add(pcid_info_t *pcid_info, int reg,
               uint32 selectors, uint32 addr, int words,
               uint32 *data)
{
    int index;
    soc_datum_t *d, **ht = reg ? pcid_info->reg_ht : pcid_info->mem_ht;

    index = SOC_HASH_EXTENDED_DATUM(selectors, addr);

    d = sal_alloc(sizeof(soc_datum_t), reg ? "rdatum_t" : "mdatum_t");
    if (NULL == d) {
        return SOC_E_MEMORY;
    }

    d->selectors = selectors;
    d->addr = addr;
    memcpy(d->data, data, words * 4);
    d->next = ht[index];
    ht[index] = d;
    return SOC_E_NONE;
}

int
soc_datum_reg_read(pcid_info_t *pcid_info, soc_regaddrinfo_t ainfo,
                   uint32 selectors, uint32 addr,
                   uint32 *data)
{
    int words;
    words = 1;
    if (SOC_REG_IS_64(pcid_info->unit, ainfo.reg)) {
        words = 2;
    }
    if(SOC_REG_IS_ABOVE_64(pcid_info->unit, ainfo.reg)) {
        words = SOC_REG_ABOVE_64_INFO(pcid_info->unit, ainfo.reg).size;
    }

    return _soc_datum_read(pcid_info, TRUE, selectors, addr, words, data);

}

int
soc_datum_reg_write(pcid_info_t *pcid_info, soc_regaddrinfo_t ainfo,
                    uint32 selectors, uint32 addr,
                    uint32 *data)
{
    int rv, words;
    uint64 mask;

    words = 1;
    if (SOC_REG_IS_64(pcid_info->unit, ainfo.reg)) {
        words = 2;
    }
    if(SOC_REG_IS_ABOVE_64(pcid_info->unit, ainfo.reg)) {
        words = SOC_REG_ABOVE_64_INFO(pcid_info->unit, ainfo.reg).size;
    }

    rv = _soc_datum_write(pcid_info, TRUE, selectors, addr, words, data);

    if (SOC_E_NOT_FOUND == rv) {
        /* Add new entry to the data tables */
        if ((!SOC_COUNTER_INVALID(pcid_info->unit, ainfo.reg)) &&
            (SOC_REG_IS_COUNTER(pcid_info->unit, ainfo.reg))) {
            COMPILER_64_SET(mask, 0, 1);
            COMPILER_64_SHL(mask,SOC_REG_INFO(pcid_info->unit,
                                              ainfo.reg).fields[0].len);
            COMPILER_64_SUB_32(mask, 1);
            data[1] &= COMPILER_64_HI(mask);
            data[0] &= COMPILER_64_LO(mask);
        }
        rv = _soc_datum_add(pcid_info, TRUE, selectors, addr, words, data);
    }

    /* Keep the error code for futher processing in the caller. */
    return rv;
}

int
soc_datum_mem_read(pcid_info_t *pcid_info,
                   uint32 selectors, uint32 addr,
                   uint32 *data)
{
    return _soc_datum_read(pcid_info, FALSE, selectors, addr,
                         SOC_MAX_MEM_WORDS, data);
}

int
soc_datum_mem_write(pcid_info_t *pcid_info,
                    uint32 selectors, uint32 addr,
                    uint32 *data)
{
    int rv;

    rv = _soc_datum_write(pcid_info, FALSE, selectors, addr,
                          SOC_MAX_MEM_WORDS, data);

    if (SOC_E_NOT_FOUND == rv) {
        rv = _soc_datum_add(pcid_info, FALSE, selectors, addr,
                           SOC_MAX_MEM_WORDS, data);
    }

    /* Keep the error code for futher processing in the caller. */
    return rv;
}
