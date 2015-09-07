/*
 * $Id: hash_mem_test.c,v 1.6 Broadcom SDK $
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
 *
 * Generic memory table Tests.
 *
 * Insert/Lookup/Delete, hashing, bucket overflow tests.
 */

#include <soc/mem.h>
#include <appl/diag/system.h>
#include "testlist.h"
#include <bcm/init.h>
#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/link.h>
#include <soc/l2x.h>
#include <soc/ism.h>
#include <appl/diag/mem_table_test.h>
#include <shared/bsl.h>
#ifdef BCM_XGS_SWITCH_SUPPORT
#ifdef BCM_ISM_SUPPORT
#include <bcm_int/esw/triumph.h>

STATIC test_generic_hash_test_t hash_test[SOC_MAX_NUM_DEVICES];

STATIC void
_test_generic_hash_setup(int unit, test_generic_hash_test_t *ht)
{
    test_generic_hash_testdata_t *htd;
    soc_mem_t tmem = L2_ENTRY_1m;
    
    if (ht->setup_done) {
        return;
    }
    ht->setup_done = TRUE;
    ht->unit = unit;
    
    htd = &ht->testdata;
    htd->mem = tmem;
    htd->mem_str = sal_strdup("l2_entry_1");
    htd->opt_count = 100; /*soc_mem_index_count(unit, tmem);*/
    htd->opt_verbose = FALSE;
    htd->opt_reset = FALSE;
    htd->opt_key_base = sal_strdup("0");
    htd->opt_key_incr = 1;
    htd->opt_banks = -1; /* All applicable banks by default */
    htd->opt_hash_offsets = sal_strdup("");
}

STATIC int
_test_generic_hash_init(int unit, test_generic_hash_testdata_t *htd,
                        args_t *a)
{
    int rv = -1, val;
    parse_table_t pt;
    char *offset, *buf = NULL;
    char *tokstr;
    
    parse_table_init(unit, &pt);
    
    parse_table_add(&pt, "Mem", PQ_DFL|PQ_STRING|PQ_STATIC, 0, &htd->mem_str, NULL);
    parse_table_add(&pt, "Count", PQ_INT|PQ_DFL, 0, &htd->opt_count, NULL);
    parse_table_add(&pt, "Verbose", PQ_BOOL|PQ_DFL, 0, &htd->opt_verbose, NULL);
    parse_table_add(&pt, "Reset", PQ_BOOL|PQ_DFL, 0, &htd->opt_reset, NULL);
    parse_table_add(&pt, "Key base", PQ_DFL|PQ_STRING|PQ_STATIC, 0, &htd->opt_key_base, NULL);
    parse_table_add(&pt, "Key incr", PQ_INT|PQ_DFL, 0, &htd->opt_key_incr, NULL);
    parse_table_add(&pt, "Banks", PQ_HEX|PQ_DFL, 0, &htd->opt_banks, NULL);
    parse_table_add(&pt, "Bank offsets", PQ_DFL|PQ_STRING|PQ_STATIC, 0, &htd->opt_hash_offsets, NULL);
    
    if (htd->opt_count < 1) {
        test_error(unit, "Illegal count %d\n", htd->opt_count);
        goto done;
    }
    
    if (parse_arg_eq(a, &pt) < 0 || ARG_CNT(a) != 0) {
        test_error(unit,
                   "%s: Invalid option: %s\n", ARG_CMD(a),
                   ARG_CUR(a) ? ARG_CUR(a) : "*");
        parse_arg_eq_done(&pt);
        return -1;
    }

    if (parse_memory_name(unit, &htd->mem, htd->mem_str,
                          &htd->copyno, 0) < 0) {
        test_error(unit, "Memory \"%s\" is invalid\n",
                   htd->mem_str);
        parse_arg_eq_done(&pt);
        return -1;
    }
    
    if (!isint(htd->opt_key_base)) {
        test_error(unit, "Key val is not valid\n");
        parse_arg_eq_done(&pt);
        return -1;
    }
    
    htd->offset_count = 0;
    
    if ((buf = sal_alloc(ARGS_BUFFER, "bank_offsets")) == NULL) {
        test_error(unit, "Unable to allocate memory for bank offsets\n");
        return -1;
    }
    
    strncpy(buf, htd->opt_hash_offsets, ARGS_BUFFER);/* Don't destroy input string */
    buf[ARGS_BUFFER - 1] = 0;        
    
    offset = sal_strtok_r(buf, ",", &tokstr);
    while (offset != 0) {
        val = parse_integer(offset);
        if (val > 64) {
            val = 64;
        } 
        htd->offsets[htd->offset_count] = (uint8)val;
        htd->offset_count++;
        offset = sal_strtok_r(NULL, ",", &tokstr);
    }
    
    /*
     * Re-initialize chip to ensure tables are clear
     * at start of test.
     */

    if (htd->opt_reset) {
        rv = bcm_linkscan_enable_set(unit, 0);
        if (rv && rv != BCM_E_UNIT) {
            test_error(unit, "Linkscan disabling failed\n");
            goto done;
        }
        if (soc_reset_init(unit) < 0) {
            test_error(unit, "SOC initialization failed\n");
            goto done;
        }

        if (soc_misc_init(unit) < 0) {
            test_error(unit, "MISC initialization failed\n");
            goto done;
        }

        if (soc_mmu_init(unit) < 0) {
            test_error(unit, "MMU initialization failed\n");
            goto done;
        }

        if (mbcm_init(unit) < 0) { 
            test_error(unit, "BCM initialization failed\n");
            goto done;
        }
    }

    rv = 0;

done:
    if (buf) {
        sal_free(buf);
    }
    parse_arg_eq_done(&pt);
    return rv;
}

/* Generic hash test init */
int
test_generic_hash_init(int unit, args_t *a, void **p)
{
    test_generic_hash_test_t *ht = &hash_test[unit];
    test_generic_hash_testdata_t *htd = &ht->testdata;
    int rv;
    
    _test_generic_hash_setup(unit, ht);
    
    ht->ctp = htd;
    
    if ((rv = _test_generic_hash_init(unit, htd, a)) < 0) {
        return rv;
    } else {
        *p = htd;
    }
    return 0;
}

/*
 * Test of hash memory
 *
 *   This test tries a number of keys against one of the hashing functions,
 *   checking a software hash against the hardware hash, then searching the
 *   bucket to find the entry after inserting.
 *
 */
int
test_generic_hash(int unit, args_t *a, void *p)
{
    int ix, index, rc = 0;
    int incr, iter, hw_index;
    uint8 *is = NULL, *isptr, inc_mode_hw = 0;
    uint8 tmp, inc_mode = 0, num_flds, key[64] = {0};
    uint8 banks[_SOC_ISM_MAX_BANKS], req_bcount = 0, bcount;
    uint32 opres, full = 0;
    uint32 entry[SOC_MAX_MEM_WORDS] = {0};
    uint32 rentry[SOC_MAX_MEM_WORDS] = {0};
    uint32 ifail = 0, sfail = 0, dfail = 0;
    uint32 fvalue[SOC_MAX_MEM_WORDS] = {0};    
    uint32 *buff = NULL, *bptr, sskip = 0;
    uint32 bmask = -1, sizes[_SOC_ISM_MAX_BANKS] = {0};
    soc_field_t lsbf, keyf[4] = {0};
    test_generic_hash_testdata_t *htd = p;
    
    COMPILER_REFERENCE(a);
    
    if (htd->opt_count > soc_mem_index_count(unit, htd->mem)) {
        htd->opt_count = soc_mem_index_count(unit, htd->mem);
    }
    iter = htd->opt_count;
    incr = htd->opt_key_incr;
    if (!incr) {
        iter = 1;
    }
    if (incr > 10) {
        /* Artificially limiting the increment value to max 100 */
        incr = htd->opt_key_incr = 10; 
    }
    if (htd->opt_verbose) {
        cli_out("Starting generic hash test: iter: %d, inc: %d\n",
                iter, incr);
    }    
    parse_long_integer((uint32*)key, sizeof(key)/sizeof(uint32), htd->opt_key_base);
    /* prepare entry and get s/w insert index */
    rc = soc_gen_entry_from_key(unit, htd->mem, key, entry);
    if (rc) {
        test_error(unit, "Could not generate entry from key: Test aborted.\n");
        return 0;
    }
    /* Fix the key_types to be the same */
    if (soc_mem_field_valid(unit, htd->mem, KEY_TYPE_0f)) {
        soc_mem_field32_set(unit, htd->mem, entry, KEY_TYPE_1f, 
                            soc_mem_field32_get(unit, htd->mem, entry, KEY_TYPE_0f));
        if (soc_mem_field_valid(unit, htd->mem, KEY_TYPE_2f)) {
            soc_mem_field32_set(unit, htd->mem, entry, KEY_TYPE_2f, 
                                soc_mem_field32_get(unit, htd->mem, entry, KEY_TYPE_0f));
            soc_mem_field32_set(unit, htd->mem, entry, KEY_TYPE_3f, 
                                soc_mem_field32_get(unit, htd->mem, entry, KEY_TYPE_0f));
        }
    }
    rc = soc_generic_get_hash_key(unit, htd->mem, entry, keyf, &lsbf, &num_flds);
    if (rc) {
        test_error(unit, "Could not retreive key fields from entry: Test aborted.\n");
        return 0;
    }
    
    rc = soc_ism_get_banks_for_mem(unit, htd->mem, banks, sizes, &bcount);
    if (rc || !bcount) {
        test_error(unit, "No banks configured for this memory: Test aborted.\n");
        return 0;
    }
    if (htd->opt_verbose) {
        cli_out("Num of configured banks: %d\n", bcount);
    }
    if (htd->opt_banks != -1) {
        bmask = 0;
        for (ix = 0; ix < bcount; ix++) {
            bmask |= 1 << banks[ix];
        }
        if (htd->opt_verbose) {
            cli_out("Supported banks mask: 0x%x\n", bmask);
        }
        if (!(htd->opt_banks & bmask)) {
            test_error(unit, "Invalid input banks mask: 0x%4x\n", htd->opt_banks);
            return 0;
        }
        if ((htd->opt_banks | bmask) != bmask) {
            test_error(unit, "Invalid input banks mask: 0x%4x\n", htd->opt_banks);
            return 0;
        }
        cli_out("Num of requested banks: %d\n", req_bcount);
    }
    htd->restore = 0;
    if (htd->offset_count) {
        if (htd->opt_verbose) {
            cli_out("Requested offsets: %d\n", htd->offset_count);
        }
        if (htd->offset_count > bcount) {
            test_error(unit, "Offsets count(%d) does not match banks count(%d).\n",
                       htd->offset_count, bcount);
            htd->offset_count = bcount;
        }
        for (ix = 0; ix < htd->offset_count; ix++) {
            if (htd->offsets[ix] >= 48) {
                inc_mode++; /* lsb mode */
            }
        }
        /* retreive, set and save offsets */
        for (ix = 0; ix < htd->offset_count; ix++) {
            rc = soc_ism_hash_offset_config_get(unit, banks[ix], &htd->cur_offset[ix]);
            if (rc) {
                test_error(unit, "Error retreiving offset for %s: bank: %d\n",
                           htd->mem_str, banks[ix]);
                return 0;
            }
            if (inc_mode && (htd->offsets[ix] < 48)) {
                /* Artificially bump the offset to lsb */
                if (htd->opt_verbose) {
                    cli_out("Updating offset for bank %d from %d to 48\n", 
                            banks[ix], htd->offsets[ix]);
                }
                htd->offsets[ix] = 48;
            }
            if (htd->offsets[ix] == htd->cur_offset[ix]) {
                continue;
            }
            rc = soc_ism_hash_offset_config(unit, banks[ix], htd->offsets[ix]);
            if (rc) {
                test_error(unit, "Error setting offset for %s: bank: %d\n",
                           htd->mem_str, banks[ix]);
                return 0;
            }
            htd->restore |= 1 << banks[ix];
            htd->config_banks[ix] = banks[ix];
            if (htd->opt_verbose) {
                cli_out("Set offset %d in place of %d for bank %d\n", 
                        htd->offsets[ix], htd->cur_offset[ix], banks[ix]);
            }
        }
    }
    
    /* determine inc_mode again from h/w config as something already with lsb 
       offset might not have been modified above */
    if (htd->opt_banks != -1) {
        for (ix = 0; ix < _SOC_ISM_MAX_BANKS; ix++) {
            if (htd->opt_banks & (1 << ix)) {
                rc = soc_ism_hash_offset_config_get(unit, ix, &tmp);
                if (rc) {
                    test_error(unit, "Error retreiving offset for %s: bank: %d\n",
                               htd->mem_str, ix);
                    return 0;
                }
                if (tmp >= 48) {
                    inc_mode_hw++;
                }
                req_bcount++;
            }
        }
    } else {
        for (ix = 0; ix < bcount; ix++) {
            rc = soc_ism_hash_offset_config_get(unit, banks[ix], &tmp);
            if (rc) {
                test_error(unit, "Error retreiving offset for %s: bank: %d\n",
                           htd->mem_str, banks[ix]);
                return 0;
            }
            if (tmp >= 48) {
                inc_mode_hw++;
            }
        }
    }
    if ((inc_mode && (inc_mode != inc_mode_hw)) || 
        (inc_mode_hw && (inc_mode_hw != req_bcount && inc_mode_hw != bcount))) {
        test_error(unit, "Conflicting or unsupported offset config detected: Test aborted.\n");
        return 0;
    }
    if (!inc_mode && inc_mode_hw) {
        inc_mode = inc_mode_hw;
    }
    
    if (!inc_mode) {
        soc_mem_field_get(unit, htd->mem, entry, keyf[0], fvalue);
    } else {
        soc_mem_field_get(unit, htd->mem, entry, lsbf, fvalue);
    }
    
    if ((buff = sal_alloc(iter * SOC_MAX_MEM_WORDS * sizeof(uint32), 
                          "hash_test_buff")) == NULL) {
        test_error(unit, "Buffer allocation failure: Test aborted.\n");
        return 0;
    }
    bptr = buff;
    if ((is = sal_alloc(iter * sizeof(uint8), "hash_test_stat")) == NULL) {
        test_error(unit, "Status buffer allocation failure: Test aborted.\n");
        return 0;
    }
    isptr = is;
    for (ix = 0; ix < iter; ix++) {
        rc = soc_generic_hash(unit, htd->mem, entry, htd->opt_banks, TABLE_INSERT_CMD_MSG,
                              &index, &opres, NULL, NULL);
        if (rc) {
            test_error(unit, "Error calculating hash: Test aborted.\n");
            goto done;
        }
        /* Insert in h/w */
        rc = soc_mem_generic_insert(unit, htd->mem, MEM_BLOCK_ANY, htd->opt_banks,
                                    entry, NULL, &hw_index);
        if (rc && ((rc != SOC_E_FULL) && (rc != SOC_E_EXISTS))) {
            test_error(unit, "Error in h/w insert: Test aborted.\n");
            goto done;
        } else {
            isptr[ix] = 1;
            rc = 0;
        }
        if ((opres == SCHAN_GEN_RESP_TYPE_FULL) && (hw_index == -1)) {
            cli_out("Bucket full for iter: %d\n", ix);
            isptr[ix] = 0;
            full++;
            bptr += (sizeof(entry) / sizeof(uint32));
            continue;
        }
        /* Compare indexes */
        if (index == hw_index) {
            if (htd->opt_verbose) {
                cli_out("Matching s/w and h/w insert(%d) index: %d\n",
                        ix, index);
            }
            /* Search for the entry, should be found */
            if (soc_mem_search(unit, htd->mem, MEM_BLOCK_ALL, &hw_index, 
                               entry, rentry, 0) < 0) {
                test_error(unit, "Search failed at index %d\n", index);
                rc = -1;
                break;
            }
            if (index != hw_index) {
                test_error(unit, "Mismatch in s/w and h/w search index: %d vs %d\n", index, hw_index);
                sfail++;
            } else {
                if (htd->opt_verbose) {
                    cli_out("Matching s/w and h/w search(%d) index: %d\n",
                            ix, index);
                }
            }
        } else {
            cli_out("Mismatch in s/w and h/w insert index: %d vs %d\n",
                    index, hw_index);
            ifail++;
            sskip++;
        }        
        sal_memcpy(bptr, entry, sizeof(entry));
        bptr += ((sizeof(entry) / sizeof(uint32)));
        
        if (!inc_mode) {
            if (soc_mem_field_valid(unit, htd->mem, KEY_TYPEf)) {
                fvalue[0] += incr << soc_mem_field_length(unit, htd->mem, KEY_TYPEf);
                soc_mem_field_set(unit, htd->mem, entry, keyf[0], fvalue);
            } else {
                fvalue[0] += incr << soc_mem_field_length(unit, htd->mem, KEY_TYPE_0f);
                soc_mem_field_set(unit, htd->mem, entry, keyf[0], fvalue);
            }
        } else {
            fvalue[0] += incr;
            fvalue[0] &= ((1 << soc_mem_field_length(unit, htd->mem, lsbf)) - 1);            
            soc_mem_field_set(unit, htd->mem, entry, lsbf, fvalue);
        }
    }    
    /* Delete entries */
    bptr = buff;
    isptr = is;
    for (ix = 0; ix < iter; ix++) {
        if (!isptr[ix]) {
            bptr += ((sizeof(entry) / sizeof(uint32)));
            continue;
        }
        if (soc_mem_delete(unit, htd->mem, MEM_BLOCK_ALL, bptr) < 0) {
            test_error(unit, "Delete failed for entry %d\n", ix);
            dfail++;
        }
        bptr += ((sizeof(entry) / sizeof(uint32)));
    }
    cli_out("Insert result: %d iter(s) passed out of %d iter(s), "
            "%d iter(s) skipped.\n", 
            iter-ifail-full, iter, full);
    cli_out("Search result: %d iter(s) passed out of %d iter(s), "
            "%d iter(s) skipped.\n", 
            iter-sfail-full-sskip, iter, full+sskip);
    cli_out("Delete result: %d entr(y/ies) deleted.\n",
            iter-dfail-full);
done:
    if (buff) {
        sal_free(buff);
    }
    if (is) {
        sal_free(is);
    }
    return rc;
}

int
test_generic_hash_ov_init(int unit, args_t *a, void **p)
{
    test_generic_hash_test_t *ht = &hash_test[unit];
    test_generic_hash_testdata_t *htd = &ht->testdata;
    int rv;
    
    _test_generic_hash_setup(unit, ht);
    
    ht->ctp = htd;
    
    if ((rv = _test_generic_hash_init(unit, htd, a)) < 0) {
        return rv;
    } else {
        *p = htd;
    }
    return 0;
}

/*
 * Test of hash memory overflow behavior
 *
 *   This test fills each bucket, then inserts another entry to see
 *   that the last entry fails to insert.
 *
 */
int
test_generic_hash_ov(int unit, args_t *a, void *p)
{
    test_generic_hash_testdata_t *htd = p;
    
    COMPILER_REFERENCE(a);
    
    if (htd->opt_verbose) {
        cli_out("Starting generic hash overflow test\n");
    }
    return 0;
}

/*
 * Test clean-up routine used for generic hash tests
 */

int
test_generic_hash_done(int unit, void *p)
{
    uint8 ix;
    int rc = 0;
    test_generic_hash_testdata_t *htd = p;
    
    if (htd->restore) {
        for (ix = 0; ix < htd->offset_count; ix++) {
            if (htd->restore & (1 << htd->config_banks[ix])) {
                rc = soc_ism_hash_offset_config(unit, htd->config_banks[ix], 
                                                htd->cur_offset[ix]);
                if (rc) {
                    test_error(unit, "Error setting offset for %s: bank: %d\n",
                               htd->mem_str, htd->config_banks[ix]);
                }
                if (htd->opt_verbose) {
                    cli_out("Restore offset %d for bank %d\n", 
                            htd->cur_offset[ix], htd->config_banks[ix]);
                }
            }
        }
    }
    return rc;
}

#endif /* BCM_ISM_SUPPORT */
#endif /* BCM_XGS_SWITCH_SUPPORT */

