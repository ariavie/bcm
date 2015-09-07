/*
 * $Id: datum.c,v 1.6 Broadcom SDK $
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

#define MAX_BROADCAST_WRITE_SIZE (SOC_MAX_NUM_BLKS - 1)
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
    int rv = SOC_E_NONE, words;
    uint64 mask;
    int nof_blocks = 1, blocks[MAX_BROADCAST_WRITE_SIZE], i, unit = pcid_info->unit;

    words = 1;
    if (SOC_REG_IS_64(unit, ainfo.reg)) {
        words = 2;
    }
    if(SOC_REG_IS_ABOVE_64(unit, ainfo.reg)) {
        words = SOC_REG_ABOVE_64_INFO(unit, ainfo.reg).size;
    }


    blocks[0] = selectors;
#ifdef BCM_JERICHO_SUPPORT
    if (SOC_IS_JERICHO(unit)) {
        /* If this is a broadcast block, write to the broadcast members instead of to the broadcast block */
        soc_block_t br_block = ainfo.block;
        if (br_block >= 0 && SOC_BLOCK_IS_BROADCAST(unit, br_block)) {
            assert(SOC_BLOCK2SCH(unit, br_block) == selectors);
            nof_blocks = SOC_BLOCK_BROADCAST_SIZE(unit, br_block);
            assert(nof_blocks > 1 && nof_blocks <= MAX_BROADCAST_WRITE_SIZE);
           /* fill the blocks array with the schannel block IDs */
            for (i = 0; i < nof_blocks; ++i) {
                blocks[i] = SOC_BLOCK2SCH(unit, SOC_BLOCK_BROADCAST_MEMBER(unit, br_block, i));
            }
        }
    }
#endif /* BCM_JERICHO_SUPPORT */

    for (i = 0; i < nof_blocks && rv == SOC_E_NONE; ++i) { /* perform write for each block */

        rv = _soc_datum_write(pcid_info, TRUE, blocks[i], addr, words, data);
        if (rv == SOC_E_NOT_FOUND) {
            /* Add new entry to the data tables */
            if ((!SOC_COUNTER_INVALID(unit, ainfo.reg)) &&
                (SOC_REG_IS_COUNTER(unit, ainfo.reg))) {
                COMPILER_64_SET(mask, 0, 1);
                COMPILER_64_SHL(mask,SOC_REG_INFO(unit,
                                                  ainfo.reg).fields[0].len);
                COMPILER_64_SUB_32(mask, 1);
                data[1] &= COMPILER_64_HI(mask);
                data[0] &= COMPILER_64_LO(mask);
            }
            rv = _soc_datum_add(pcid_info, TRUE, blocks[i], addr, words, data);
        }
    }

    /* Keep the error code for further processing by the caller. */
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
    int rv = SOC_E_NONE;
    int nof_blocks = 1, blocks[MAX_BROADCAST_WRITE_SIZE], i;
#ifdef BCM_JERICHO_SUPPORT
    int unit = pcid_info->unit;
#endif /* BCM_JERICHO_SUPPORT */

    blocks[0] = selectors;
#ifdef BCM_JERICHO_SUPPORT
    if (SOC_IS_JERICHO(unit)) { /* handle broadcast blocks */
      int blk;
        /* Find the block of the given CMIC block */
        for (blk = 0; ; ++blk) {
            if (SOC_BLOCK_TYPE(unit, blk) < 0) {
                return INVALIDm;
            } else if (SOC_BLOCK2OFFSET(unit, blk) == selectors) {
                break;
            }
        }
        /* If this is a broadcast block, write to the broadcast members instead of to the broadcast block */
        if (SOC_BLOCK_IS_BROADCAST(unit, blk)) {
            nof_blocks = SOC_BLOCK_BROADCAST_SIZE(unit, blk);
            assert(nof_blocks > 1 && nof_blocks <= MAX_BROADCAST_WRITE_SIZE);
           /* fill the blocks array with the schannel block IDs */
            for (i = 0; i < nof_blocks; ++i) {
                blocks[i] = SOC_BLOCK2SCH(unit, SOC_BLOCK_BROADCAST_MEMBER(unit, blk, i));
            }
        }
    }
#endif /* BCM_JERICHO_SUPPORT */

    for (i = 0; i < nof_blocks && rv == SOC_E_NONE; ++i) { /* perform write for each block */
        rv = _soc_datum_write(pcid_info, FALSE, blocks[i], addr, SOC_MAX_MEM_WORDS, data);
        if (SOC_E_NOT_FOUND == rv) {
            rv = _soc_datum_add(pcid_info, FALSE, blocks[i], addr, SOC_MAX_MEM_WORDS, data);
        }
    }

    /* Keep the error code for futher processing in the caller. */
    return rv;
}
