/*
 * $Id: draco_arltest.c,v 1.106 Broadcom SDK $
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
 * ARL Tests.
 *
 * Insert/Lookup/Delete, hashing, bucket overflow tests.
 */


#ifdef BCM_ESW_SUPPORT
#include <soc/mem.h>
#include <soc/hash.h>
#include <soc/l2x.h>
#include <soc/l3x.h>
#include <appl/diag/system.h>
#include <appl/test/draco_arltest.h>
#include <shared/bsl.h>
#include "testlist.h"
#include <bcm/init.h>
#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/stack.h>
#include <bcm/link.h>
#include <bcm_int/esw/firebolt.h>
#if defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
#include <bcm_int/esw/triumph.h>
#endif /* BCM_TRIUMPH_SUPPORT */
#include <appl/diag/progress.h>

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
#ifdef BCM_XGS_SWITCH_SUPPORT

/*
 * L2 work structure
 */

STATIC draco_l2_test_t dl2_work[SOC_MAX_NUM_DEVICES];
#ifdef INCLUDE_L3
STATIC draco_l3_test_t dl3_work[SOC_MAX_NUM_DEVICES];
#endif /* INCLUDE_L3 */

sal_mac_addr_t dl_mac_src = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

#ifdef BCM_BRADLEY_SUPPORT
STATIC void
hbl2_setup(int unit, draco_l2_test_t *dw)
{
    draco_l2_testdata_t *ad;


    /* Hash */
    ad = &dw->dlp_l2_hash;

    ad->opt_count      = soc_mem_index_count(unit, L2Xm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Overflow */
    ad = &dw->dlp_l2_ov;

    ad->opt_count      = 2048;
    ad->opt_hash       = FB_L2_DEFAULT_HASH;
    ad->hash_count     = FB_HASH_COUNT;

    /* Lookup */
    ad = &dw->dlp_l2_lu;

    ad->opt_count      = soc_mem_index_count(unit, L2Xm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Delete by port */
    ad = &dw->dlp_l2_dp;

    ad->opt_count      = 20;  /* 20 GX PORTS */
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Delete by VLAN */
    ad = &dw->dlp_l2_dv;

    ad->opt_count      = soc_mem_index_count(unit, VLAN_TABm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;
}

#endif /* BCM_BRADLEY_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
STATIC void
fbl2_setup(int unit, draco_l2_test_t *dw)
{
    draco_l2_testdata_t *ad;


    /* Hash */
    ad = &dw->dlp_l2_hash;

    ad->opt_count      = soc_mem_index_count(unit, L2Xm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Overflow */
    ad = &dw->dlp_l2_ov;

    ad->opt_count      = 2048;
    ad->opt_hash       = FB_L2_DEFAULT_HASH;
    ad->hash_count     = FB_HASH_COUNT;

    /* Lookup */
    ad = &dw->dlp_l2_lu;

    ad->opt_count      = soc_mem_index_count(unit, L2Xm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Delete by port */
    ad = &dw->dlp_l2_dp;

    ad->opt_count      = 24;  /* 24 GE ports + N XE PORTS */
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Delete by VLAN */
    ad = &dw->dlp_l2_dv;

    ad->opt_count      = soc_mem_index_count(unit, VLAN_TABm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;
}

#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_TRX_SUPPORT
STATIC void
trl2_setup(int unit, draco_l2_test_t *dw)
{
    draco_l2_testdata_t *ad;

    /* Hash */
    ad = &dw->dlp_l2_hash;

    ad->opt_count      = soc_mem_index_count(unit, L2Xm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Overflow */
    ad = &dw->dlp_l2_ov;

    ad->opt_count      = 2048;
    ad->opt_hash       = FB_L2_DEFAULT_HASH;
    ad->hash_count     = FB_HASH_COUNT;

    /* Lookup */
    ad = &dw->dlp_l2_lu;

    ad->opt_count      = soc_mem_index_count(unit, L2Xm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Delete by port */
    ad = &dw->dlp_l2_dp;

    ad->opt_count      = 49;  /* 49 GE ports + N XE PORTS */
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;

    /* Delete by VLAN */
    ad = &dw->dlp_l2_dv;

    ad->opt_count      = soc_mem_index_count(unit, VLAN_TABm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->hash_count     = FB_HASH_COUNT;
}

#endif /* BCM_TRX_SUPPORT */


STATIC void
dl2_setup(int unit, draco_l2_test_t *dw)
{
    draco_l2_testdata_t *ad;

    if (dw->dlw_set_up) {
        return;
    }

    dw->dlw_set_up = TRUE;
    dw->dlw_unit = unit;

    /* Hash */
    ad = &dw->dlp_l2_hash;

    ad->unit           = unit;
    ad->opt_count      = 8191;
    ad->opt_verbose    = FALSE;
    ad->opt_reset      = TRUE;
    ad->opt_hash       = XGS_HASH_CRC16_LOWER;
    ad->opt_base_vid   = 0;
    ad->opt_vid_inc    = 1;
    ad->opt_mac_inc    = 1;
    ENET_SET_MACADDR(ad->opt_base_mac, dl_mac_src);
    ad->hash_count = XGS_HASH_COUNT;

    /* Overflow */
    ad = &dw->dlp_l2_ov;

    ad->unit           = unit;
    ad->opt_count      = 1024;
    ad->opt_verbose    = FALSE;
    ad->opt_reset      = TRUE;
    ad->opt_hash       = DRACO_ARL_DEFAULT_HASH;
    ad->opt_base_vid   = 0;
    ad->opt_vid_inc    = 1;
    ad->opt_mac_inc    = 1;
    ENET_SET_MACADDR(ad->opt_base_mac, dl_mac_src);
    ad->hash_count = XGS_HASH_COUNT;

    /* Lookup */
    ad = &dw->dlp_l2_lu;

    ad->unit           = unit;
    ad->opt_count      = 8191;
    ad->opt_verbose    = FALSE;
    ad->opt_reset      = TRUE;
    ad->opt_hash       = XGS_HASH_CRC16_LOWER;
    ad->opt_base_vid   = 0;
    ad->opt_vid_inc    = 1;
    ad->opt_mac_inc    = 1;
    ENET_SET_MACADDR(ad->opt_base_mac, dl_mac_src);
    ad->hash_count = XGS_HASH_COUNT;

    /* Delete by port */
    ad = &dw->dlp_l2_dp;

    ad->unit           = unit;
    ad->opt_count      = 12;  /* Only 12 relevant ports? */
    ad->opt_verbose    = FALSE;
    ad->opt_reset      = TRUE;
    ad->opt_hash       = XGS_HASH_CRC16_LOWER;
    ad->opt_base_vid   = 0;
    ad->opt_vid_inc    = 1;
    ad->opt_mac_inc    = 1;
    ENET_SET_MACADDR(ad->opt_base_mac, dl_mac_src);
    ad->hash_count = XGS_HASH_COUNT;

    /* Delete by VLAN */
    ad = &dw->dlp_l2_dv;

    ad->unit           = unit;
    ad->opt_count      = 4096;
    ad->opt_verbose    = FALSE;
    ad->opt_reset      = TRUE;
    ad->opt_hash       = XGS_HASH_CRC16_LOWER;
    ad->opt_base_vid   = 0;
    ad->opt_vid_inc    = 1;
    ad->opt_mac_inc    = 1;
    ENET_SET_MACADDR(ad->opt_base_mac, dl_mac_src);
    ad->hash_count = XGS_HASH_COUNT;

#ifdef BCM_BRADLEY_SUPPORT
    /* Bradley adjustments */
    if (SOC_IS_HB_GW(unit)) {
        hbl2_setup(unit, dw);
    }
#endif /* BCM_BRADLEY_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
    /* Firebolt adjustments */
    if (SOC_IS_FB_FX_HX(unit)) {
        fbl2_setup(unit, dw);
    }
#endif /* BCM_FIREBOLT_SUPPORT */
#ifdef BCM_TRX_SUPPORT
    /* Triumph adjustments */
    if (SOC_IS_TRX(unit)) {
        trl2_setup(unit, dw);
    }
#endif /* BCM_TRX_SUPPORT */
}


/*
 * Test initialization routine used for all L2 tests
 */

STATIC int
draco_l2_test_init(int unit, draco_l2_testdata_t *ad, args_t *a)
{
    int                         rv = -1;
    parse_table_t               pt;
    uint32                      hash_read;
    uint32                      l2_aux_hash_control;

    parse_table_init(unit, &pt);

    parse_table_add(&pt, "Count", PQ_INT|PQ_DFL, 0, &ad->opt_count, NULL);
    parse_table_add(&pt, "Verbose", PQ_BOOL|PQ_DFL, 0, &ad->opt_verbose, NULL);
    parse_table_add(&pt, "Reset", PQ_BOOL|PQ_DFL, 0, &ad->opt_reset, NULL);
    parse_table_add(&pt, "Hash", PQ_INT|PQ_DFL, 0, &ad->opt_hash, NULL);
    parse_table_add(&pt, "BaseVID", PQ_INT|PQ_DFL, 0, &ad->opt_base_vid, NULL);
    parse_table_add(&pt, "VidIncrement", PQ_INT|PQ_DFL, 0,
                    &ad->opt_vid_inc, NULL);
    parse_table_add(&pt, "BaseMac",     PQ_DFL|PQ_MAC,  0,
                    &ad->opt_base_mac,  NULL);
    parse_table_add(&pt, "MacIncrement", PQ_INT|PQ_DFL, 0,
                    &ad->opt_mac_inc, NULL);

    /* Test the obvious parsings before wasting time with malloc */
    if (parse_arg_eq(a, &pt) < 0) {
        test_error(unit,
                   "%s: Error: Invalid option: %s\n", ARG_CMD(a),
                   ARG_CUR(a) ? ARG_CUR(a) : "*");
        goto done;
    }

    if (ad->opt_count < 1) {
        test_error(unit, "Illegal count %d\n", ad->opt_count);
        goto done;
    }

    if (ad->opt_hash >= ad->hash_count) {
        test_error(unit, "Illegal hash selection %d\n", ad->opt_hash);
        goto done;
    }

    if (ad->opt_base_vid >= (1 << 12)) {
        test_error(unit, "Out of range VLAN ID selection %d\n",
                   ad->opt_base_vid);
        goto done;
    }

    /*
     * Re-initialize chip to ensure ARL and software ARL table are clear
     * at start of test.
     */

    if (ad->opt_reset) {
        BCM_IF_ERROR_RETURN(bcm_linkscan_enable_set(unit, 0));
        if (soc_reset_init(unit) < 0) {
            test_error(unit, "SOC initialization failed\n");
            goto done;
        }

        if (soc_misc_init(unit) < 0) {
            test_error(unit, "ARL initialization failed\n");
            goto done;
        }

        if (soc_mmu_init(unit) < 0) {
            test_error(unit, "MMU initialization failed\n");
            goto done;
        }

        if (SOC_FUNCTIONS(unit)->soc_age_timer_set(unit, 0, 0) < 0) {
            test_error(unit, "Could not disable age timers\n");
            goto done;
        }

        if (mbcm_init(unit) < 0) { /* Needed for "delete by" tests */
            test_error(unit, "mBCM initialization failed\n");
            goto done;
        }

    }

    if (READ_HASH_CONTROLr(unit, &hash_read) < 0) {
        test_error(unit, "Hash select read failed\n");
        goto done;
    }

    ad->save_hash_control = hash_read;

    hash_read = ad->opt_hash;

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        hash_read = ad->save_hash_control;
        soc_reg_field_set(unit, HASH_CONTROLr, &hash_read,
                          L2_AND_VLAN_MAC_HASH_SELECTf, ad->opt_hash);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    if (WRITE_HASH_CONTROLr(unit, hash_read) < 0) {
        test_error(unit, "Hash select setting failed\n");
        goto done;
    }

    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (READ_L2_AUX_HASH_CONTROLr(unit, &l2_aux_hash_control) < 0) {
            test_error(unit, "L2 AUX hash control read failed\n");
            goto done;
        }
    
        ad->save_l2_aux_hash_control = l2_aux_hash_control;
    
        soc_reg_field_set(unit, L2_AUX_HASH_CONTROLr, 
                          &l2_aux_hash_control, HASH_SELECTf, FB_HASH_LSB);
    
        if ((SAL_BOOT_SIMULATION) && (!SAL_BOOT_QUICKTURN)) {
            soc_reg_field_set(unit, L2_AUX_HASH_CONTROLr, 
                              &l2_aux_hash_control, ENABLEf, 0);
        }
    
        if (WRITE_L2_AUX_HASH_CONTROLr(unit, l2_aux_hash_control) < 0) {
            test_error(unit, "L2 AUX hash control setting failed\n");
            goto done;
        }
    }

    rv = 0;

 done:

    parse_arg_eq_done(&pt);
    return rv;
}

/* Individual test init wrappers */
int
draco_l2_hash_test_init(int unit, args_t *a, void **p)
{
    draco_l2_test_t        *dw = &dl2_work[unit];
    draco_l2_testdata_t    *dp = &dw->dlp_l2_hash;
    int                    rv;

    dl2_setup(unit, dw);

    /* Set working data to hash */
    dw->dlp_l2_cur = dp;

    if ((rv = draco_l2_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
draco_l2_ov_test_init(int unit, args_t *a, void **p)
{
    draco_l2_test_t        *dw = &dl2_work[unit];
    draco_l2_testdata_t    *dp = &dw->dlp_l2_ov;
    int                    rv;

    dl2_setup(unit, dw);

    /* Set working data to overflow */
    dw->dlp_l2_cur = dp;

    if ((rv = draco_l2_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
draco_l2_lu_test_init(int unit, args_t *a, void **p)
{
    draco_l2_test_t        *dw = &dl2_work[unit];
    draco_l2_testdata_t    *dp = &dw->dlp_l2_lu;
    int                    rv;

    dl2_setup(unit, dw);

    /* Set working data to lookup */
    dw->dlp_l2_cur = dp;

    if ((rv = draco_l2_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
draco_l2_dp_test_init(int unit, args_t *a, void **p)
{
    draco_l2_test_t        *dw = &dl2_work[unit];
    draco_l2_testdata_t    *dp = &dw->dlp_l2_dp;
    int                    rv;

    dl2_setup(unit, dw);

    /* Set working data to delete by port */
    dw->dlp_l2_cur = dp;

    if ((rv = draco_l2_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
draco_l2_dv_test_init(int unit, args_t *a, void **p)
{
    draco_l2_test_t        *dw = &dl2_work[unit];
    draco_l2_testdata_t    *dp = &dw->dlp_l2_dv;
    int                    rv;

    dl2_setup(unit, dw);

    /* Set working data to delete by VLAN */
    dw->dlp_l2_cur = dp;

    if ((rv = draco_l2_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

/*
 * Utility routines for ARL testing
 */

void
draco_l2_time_start(draco_l2_testdata_t *ad)
{
    ad->tm = SAL_TIME_DOUBLE();
}

void
draco_l2_time_end(draco_l2_testdata_t *ad)
{
    ad->tm = SAL_TIME_DOUBLE() - ad->tm;
    if (ad->opt_verbose) {
        cli_out("    time: %"COMPILER_DOUBLE_FORMAT" msec\n", ad->tm * 1000);
    }
}


#if defined(BCM_FIREBOLT_SUPPORT)
static int
draco_l2_bucket_search(int unit, draco_l2_testdata_t *ad, int bucket,
                        l2x_entry_t *expect)
{
    l2x_entry_t chip_entry;
    int ix, mem_table_index;
    int rv = -1; /* Assume failed unless we find it */
    int validf;

    validf = VALIDf;

    for (ix = 0; ix < SOC_L2X_BUCKET_SIZE; ix++) {
        mem_table_index = (bucket * SOC_L2X_BUCKET_SIZE) + ix;

        if (soc_mem_read(unit, L2Xm, MEM_BLOCK_ANY,
                             mem_table_index, &chip_entry) < 0) {
            test_error(unit,
                       "Read ARL failed at bucket %d, offset %d\n",
                       bucket, ix);
            break;
        }

        if (!soc_L2Xm_field32_get(unit, &chip_entry, validf)) {
            /* Valid bit unset, entry blank */
            continue;
        }

        if (soc_mem_compare_key(unit, L2Xm, expect, &chip_entry) == 0) {
            /* Found the matching entry */
            rv = 0;
            break;
        }
    }

    return rv;
}

/*
 * Test of L2X hashing
 *
 *   This test tries a number of keys against one of the hashing functions,
 *   checking a software hash against the hardware hash, then searching the
 *   bucket to find the entry after inserting.
 *
 */
int
fb_l2_test_hash(int unit, args_t *a, void *p)
{
    draco_l2_testdata_t         *ad = p;
    l2x_entry_t         entry;
    int r, rv = 0;
    int soft_bucket;
    int ix;
    int hash = ad->opt_hash;
    uint8 key[XGS_HASH_KEY_SIZE];
    int iterations;
    bcm_l2_addr_t addr;
    int vid_inc = ad->opt_vid_inc;
    int mac_inc = ad->opt_mac_inc;
#ifdef BCM_TRIUMPH_SUPPORT
    int ext_index_max = -1;
#endif /* BCM_TRIUMPH_SUPPORT */

    COMPILER_REFERENCE(a);

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        ext_index_max = SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max;
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = -1;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    bcm_l2_addr_t_init(&addr, ad->opt_base_mac, ad->opt_base_vid);

    if (ad->opt_verbose) {
        cli_out("Starting ARL hash test\n");
    }

    iterations = ad->opt_count;

    for (ix = 0; ix < iterations; ix++) {
#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(unit)) {
            int num_bits;

            _bcm_tr_l2_to_l2x(unit, &entry, &addr, TRUE);
            num_bits = soc_tr_l2x_base_entry_to_key(unit, &entry, key);
            soft_bucket = soc_tr_l2x_hash(unit, hash, num_bits, &entry, key);
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            _bcm_fb_l2_to_l2x(unit, &entry, &addr);
            soc_draco_l2x_base_entry_to_key(unit, &entry, key);
            soft_bucket = soc_fb_l2_hash(unit, hash, key);
        }

        if (ad->opt_verbose) {
            cli_out("Inserting ");
            soc_mem_entry_dump(unit, L2Xm, &entry);
            cli_out("\n");
            cli_out("into bucket 0x%x\n", soft_bucket);
        }

        if ((r = bcm_l2_addr_add(unit, &addr)) < 0) {
            if (r == SOC_E_FULL) {
                /* Bucket overflow, just pass on */
                goto incr;
            } else {
                test_error(unit,
                           "ARL insert failed at bucket %d\n", soft_bucket);
                rv = -1;
                goto done;
            }
        }

        /* Now we search for the entry */

        /* Only do a quick check vs. expected bucket here */
        if (draco_l2_bucket_search(unit, ad, soft_bucket, &entry) < 0) {
            test_error(unit,
                       "ARL entry with key "
                       "0x%02x%02x%02x%02x%02x%02x%02x%01x "
                       "not found in predicted bucket %d\n",
                       key[7], key[6], key[5], key[4],
                       key[3], key[2], key[1], (key[0] >> 4) & 0xf,
                       soft_bucket);
        }

        if (bcm_l2_addr_delete(unit, addr.mac, addr.vid) < 0) {
            test_error(unit, "ARL delete failed at bucket %d\n",
                       soft_bucket);
            rv = -1;
            goto done;
        }

    incr:
        addr.vid += vid_inc;
        if (addr.vid > FB_L2_VID_MAX) {
            addr.vid = 1;
        }
        increment_macaddr(addr.mac, mac_inc);
    }

 done:
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = ext_index_max;
    }
#endif /* BCM_TRIUMPH_SUPPORT */
    return rv;
}

/*
 * Test of ARL overflow behavior
 *
 *   This test fills each bucket of the ARL, then inserts another entry to see
 *   that the last entry fails to insert.
 *
 */

int
fb_l2_test_ov(int unit, args_t *a, void *p)
{
    draco_l2_testdata_t         *ad = p;
    l2x_entry_t         entry, result, entry_tmp[SOC_L2X_BUCKET_SIZE];
    int                 ix, jx, r, idx, rv = 0;
    int bucket = 0;
    uint32 hash = ad->opt_hash;
    bcm_l2_addr_t addr, addr_tmp;
    int vid_inc = ad->opt_vid_inc;
    int iter = ad->opt_count;
    uint8 key[XGS_HASH_KEY_SIZE];
#ifdef BCM_TRIUMPH_SUPPORT
    int ext_index_max = -1;
#endif /* BCM_TRIUMPH_SUPPORT */

    COMPILER_REFERENCE(a);

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        ext_index_max = SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max;
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = -1;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    bcm_l2_addr_t_init(&addr, ad->opt_base_mac, ad->opt_base_vid);

    if (hash != FB_HASH_LSB) {
        if (ad->opt_verbose) {
            cli_out("Resetting hash selection to LSB\n");
        }

        hash = ad->save_hash_control;
        soc_reg_field_set(unit, HASH_CONTROLr, &hash,
                          L2_AND_VLAN_MAC_HASH_SELECTf, FB_HASH_LSB);

        if (WRITE_HASH_CONTROLr(unit, hash) < 0) {
            test_error(unit, "Hash select setting failed\n");
            goto done;
        }

        ad->opt_hash = hash = FB_HASH_LSB;
    }

    if (iter > soc_mem_index_count(unit, L2Xm)) {
        iter = soc_mem_index_count(unit, L2Xm);
    }

    while (iter--) {
        for (ix = 0; ix < SOC_L2X_BUCKET_SIZE; ix++) {
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit)) {
                _bcm_tr_l2_to_l2x(unit, &(entry_tmp[ix]), &addr, TRUE);

                if (ix == 0) {
                    int num_bits;

                    num_bits = soc_tr_l2x_base_entry_to_key
                        (unit, &(entry_tmp[ix]), key);
                    bucket = soc_tr_l2x_hash(unit, hash, num_bits,
                                             &(entry_tmp[ix]), key);
    
                    if (ad->opt_verbose) {
                        cli_out("Filling bucket %d\n", bucket);
                    }
                }
            } else
#endif /* BCM_TRX_SUPPORT */
            {
                _bcm_fb_l2_to_l2x(unit, &(entry_tmp[ix]), &addr);

                if (ix == 0) {
                    soc_draco_l2x_base_entry_to_key(unit, &(entry_tmp[ix]), key);
                    bucket = soc_fb_l2_hash(unit, hash, key);
    
                    if (ad->opt_verbose) {
                        cli_out("Filling bucket %d\n", bucket);
                    }
                }
            }

            if ((r = bcm_l2_addr_add(unit, &addr)) < 0) {
                if (r == SOC_E_FULL) {
                    /* Already full, stop wasting time */
                    break;
                } else {
                    test_error(unit,
                               "ARL insert failed at bucket %d\n", bucket);
                    rv = -1;
                    goto done;
                }
            }

            /* key for LSB is the MAC address, so we must keep it constant */
            addr.vid += vid_inc;
            if (addr.vid > DRACO_L2_VID_MAX) {
                addr.vid = 1;
            }
        }

        if (ad->opt_verbose) {
            cli_out("Inserting %dth entry in bucket %d, should fail\n",
                    (SOC_L2X_BUCKET_SIZE + 1), bucket);
        }

#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(unit)) {
            _bcm_tr_l2_to_l2x(unit, &entry, &addr, TRUE);
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            _bcm_fb_l2_to_l2x(unit, &entry, &addr);
        }

        if ((r = bcm_l2_addr_add(unit, &addr)) < 0) {
            if (r != SOC_E_FULL) {
                test_error(unit,
                           "ARL insert failed\n");
                rv = -1;
                goto done;
            }
        } else {
            test_error(unit, "ARL insert to full bucket succeeded\n");
            rv = -1;
            goto done;
        }

        if (ad->opt_verbose) {
            cli_out("Verifying entries present\n");
        }

        /* Verify bucket contains our added entries */
        for (jx = 0; jx < ix; jx++) {
            if (draco_l2_bucket_search(unit, ad, bucket,
                                          &(entry_tmp[jx])) < 0) {
                test_error(unit, "ARL entry missing at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
            if (soc_mem_search(unit, L2Xm, MEM_BLOCK_ANY, &idx, 
                               &entry_tmp[jx], &result, 0) < 0) {
                test_error(unit, "ARL entry missing at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
            if (bucket != (idx / SOC_L2X_BUCKET_SIZE)) {
                test_error(unit, "ARL entry inserted into wrong bucket"
                           " Expected %d Actual %d\n", bucket, idx);
                rv = -1;
                goto done;
            }
        }

        if (ad->opt_verbose) {
            cli_out("Cleaning bucket %d\n", bucket);
        }

        /* Remove the entries that we added */
        for (jx = 0; jx < ix; jx++) {
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit)) {
                _bcm_tr_l2_from_l2x(unit, &addr_tmp, &(entry_tmp[jx]));
            } else
#endif /* BCM_TRX_SUPPORT */
            {
                _bcm_fb_l2_from_l2x(unit, &addr_tmp, &(entry_tmp[jx]));
            }
            if (bcm_l2_addr_delete(unit, addr_tmp.mac, addr_tmp.vid) < 0) {
                test_error(unit, "ARL delete failed at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        /* We want the increment to change buckets by one */
        increment_macaddr(addr.mac, 1);
    }

 done:
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = ext_index_max;
    }
#endif /* BCM_TRIUMPH_SUPPORT */
    return rv;
}

/*
 * Test of ARL lookup
 *
 *   This test loops a given number of times.  It inserts a single ARL
 *   entry, looks up the ARL entry, and deletes the ARL entry.
 *   It is possible to run this test while heavy switching is going on.
 */

int
fb_l2_test_lu(int unit, args_t *a, void *p)
{
    draco_l2_testdata_t *ad = p;
    l2x_entry_t         entry, entry_tmp;
    int                 i, r, rv = -1;
    int                 found_times, bucket_full_times;
    int                 index;
    bcm_l2_addr_t addr;
    int vid_inc = ad->opt_vid_inc;
    int mac_inc = ad->opt_mac_inc;
#ifdef BCM_TRIUMPH_SUPPORT
    int ext_index_max = -1;
#endif /* BCM_TRIUMPH_SUPPORT */

    COMPILER_REFERENCE(a);

    if (! soc_feature(unit, soc_feature_l2_lookup_cmd)) {
        test_error(unit, "L2 lookup not supported by chip (unit %d)\n", unit);
        return -1;
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        ext_index_max = SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max;
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = -1;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    bcm_l2_addr_t_init(&addr, ad->opt_base_mac, ad->opt_base_vid);

    draco_l2_time_start(ad);

    found_times = 0;
    bucket_full_times = 0;

    for (i = 1; i <= ad->opt_count; i++) {

#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(unit)) {
            _bcm_tr_l2_to_l2x(unit, &entry, &addr, TRUE);
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            _bcm_fb_l2_to_l2x(unit, &entry, &addr);
        }

        if ((r = bcm_l2_addr_add(unit, &addr)) < 0) {
            if (r == SOC_E_FULL) {
                /* Bucket overflow, just pass on */
                bucket_full_times++;
                goto inc;
            } else {
                test_error(unit,
                           "ARL insert failed on loop %d: %s\n",
                           i, soc_errmsg(r));
                break;
            }
        }

        if ((r = soc_mem_search(unit, L2Xm, MEM_BLOCK_ANY, &index,
                                &entry, &entry_tmp, 0)) < 0) {
            test_error(unit,
                       "ARL lookup failure on loop %d: %s\n",
                       i, soc_errmsg(r));
            break;
        } else  {
            found_times++;
        }

        if ((r = bcm_l2_addr_delete(unit, addr.mac, addr.vid)) < 0) {
            test_error(unit,
                       "ARL delete failed on loop %d: %s\n",
                       i, soc_errmsg(r));
            break;
        }

        if ((r = soc_mem_search(unit, L2Xm, MEM_BLOCK_ANY, &index,
                                &entry, &entry_tmp, 0)) < 0) {
            if (r != SOC_E_NOT_FOUND) {
                test_error(unit,
                           "ARL lookup failure on loop %d: %s\n",
                           i, soc_errmsg(r));
                break;
            }
            /* else, OK */
        } else  {
            test_error(unit,
                       "ARL lookup succeeded after delete on loop %d\n", i);
            break;
        }

    inc:
        addr.vid += vid_inc;
        if (addr.vid > DRACO_L2_VID_MAX) {
            addr.vid = 1;
        }
        increment_macaddr(addr.mac, mac_inc);
    }

    draco_l2_time_end(ad);

    if (i > ad->opt_count) {            /* All iterations passed */
        if ((bucket_full_times + found_times) != ad->opt_count) {
            test_error(unit,
                       "Lookup succeeded only %d times out of %d\n",
                       found_times, ad->opt_count);
        } else {
            rv = 0;                     /* Test passed */

            if (ad->opt_verbose) {
                cli_out("Passed:  %d lookups, %d bucket overflows\n",
                        found_times, bucket_full_times);
            }
        }
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = ext_index_max;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    return rv;
}

/*
 * Test of ARL delete by port
 *
 * This test will insert a collection of entries with different ports,
 * then try to delete them by port reference.
 *
 */

int
fb_l2_test_dp(int unit, args_t *a, void *p)
{
    draco_l2_testdata_t *ad = p;
    l2x_entry_t         *entries = NULL;
    int                 i, r, rv = 0;
    int                 bucket_full_times;
    int                 *bucket_full = NULL;
    bcm_l2_addr_t       addr;
    int                 vid_inc = ad->opt_vid_inc;
    int                 mac_inc = ad->opt_mac_inc;
    int                 modid, modid_hi;
    uint8               key[XGS_HASH_KEY_SIZE];
#ifdef BCM_TRIUMPH_SUPPORT
    int                 ext_index_max = -1;
#endif /* BCM_TRIUMPH_SUPPORT */

    COMPILER_REFERENCE(a);

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        ext_index_max = SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max;
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = -1;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    bcm_l2_addr_t_init(&addr, ad->opt_base_mac, ad->opt_base_vid);
    if ((r = bcm_stk_my_modid_get(unit, &modid)) < 0) {
        test_error(unit,
                   "Modid retrieval failed: %s\n",
                   soc_errmsg(r));
        rv = -1;
        goto done;
    }
    modid_hi = modid + 1;

    entries =
        sal_alloc(sizeof(l2x_entry_t) * SOC_MAX_NUM_PORTS, "L2 entries");
    if (!entries) {
        test_error(unit,
                   "Memory allocation failure\n");
        rv = -1;
        goto done;
    }
    sal_memset(entries, 0, sizeof(l2x_entry_t) * SOC_MAX_NUM_PORTS);

    bucket_full = sal_alloc(sizeof(int) * SOC_MAX_NUM_PORTS,
                            "L2 full buckets");
    if (!bucket_full) {
        test_error(unit,
                   "Memory allocation failure\n");
        rv = -1;
        goto done;
    }
    sal_memset(bucket_full, 0, sizeof(int) * SOC_MAX_NUM_PORTS);

    bucket_full_times = 0;

    if (ad->opt_verbose) {
        cli_out("Inserting port entries\n");
    }

    draco_l2_time_start(ad);

    PBMP_E_ITER(unit, i) {
        if (SOC_PORT_MOD1(unit, i)) {
            addr.modid = modid_hi;
        } else {
            addr.modid = modid;
        }
        addr.port = i & 0x1f;
#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(unit)) {
            _bcm_tr_l2_to_l2x(unit, &(entries[i]), &addr, FALSE);
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            _bcm_fb_l2_to_l2x(unit, &(entries[i]), &addr);
        }

        if (ad->opt_verbose) {
            cli_out("Inserting port %d entry\n", i);
        }

        if ((r = bcm_l2_addr_add(unit, &addr)) < 0) {
            if (r == SOC_E_FULL) {
                bucket_full_times++;
                bucket_full[i] = 1;
                continue;
            } else {
                test_error(unit,
                           "ARL insert failed on port entry %d: %s\n",
                           i, soc_errmsg(r));
                rv = -1;
                goto done;
            }
        } else {
            bucket_full[i] = 0;
        }

        addr.vid += vid_inc;
        if (addr.vid > DRACO_L2_VID_MAX) {
            addr.vid = 1;
        }
        increment_macaddr(addr.mac, mac_inc);
    }

    PBMP_E_ITER(unit, i) {
        l2x_entry_t *cur_entry = &(entries[i]);
        int local_modid;
        int port;

        port = i & 0x1f;

        if (SOC_PORT_MOD1(unit, i)) {
             local_modid = modid_hi;
        } else {
            local_modid = modid;
        }
        if (!bucket_full[i]) {
            if (ad->opt_verbose) {
                cli_out("Attempting to delete by port %d\n", i);
            }

            r = bcm_l2_addr_delete_by_port(unit, local_modid, port,
                                                 BCM_L2_DELETE_STATIC);
            if (r < 0) {
                test_error(unit,
                           "ARL delete by port unsuccessful on modid %d, port %d\n",
                           local_modid, port);
                rv = -1;
                goto done;
            } else {
                /* Delete claims to be successful, check */
                int soft_bucket;

#ifdef BCM_TRX_SUPPORT
                if (SOC_IS_TRX(unit)) {
                    int num_bits;
                    num_bits = soc_tr_l2x_base_entry_to_key(unit, cur_entry,
                                                            key);
                    soft_bucket = soc_tr_l2x_hash(unit, ad->opt_hash, num_bits,
                                                  cur_entry, key);
                } else
#endif
                {
                    soc_draco_l2x_base_entry_to_key(unit, cur_entry, key);
                    soft_bucket = soc_fb_l2_hash(unit, ad->opt_hash, key);
                }

                /* We should not find the entry when searching its bucket */
                if (draco_l2_bucket_search(unit, ad,
                                              soft_bucket, cur_entry) == 0) {
                    test_error(unit,
                               "ARL delete by port failed on port %d\n", i);
                    rv = -1;
                    goto done;
                }
            }
        }
    }

    draco_l2_time_end(ad);

 done:
    if (bucket_full) {
        sal_free(bucket_full);
    }
    if (entries) {
        sal_free(entries);
    }
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = ext_index_max;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    return rv;
}

/*
 * Test of ARL delete by VLAN
 *
 * This test will insert a collection of entries with different VLAN ids,
 * then try to delete them by VLAN.
 *
 */

int
fb_l2_test_dv(int unit, args_t *a, void *p)
{
    draco_l2_testdata_t *ad = p;
    int                 count = ad->opt_count;
    l2x_entry_t         *entries = NULL;
    int                 i, vid, r = 0, rv = 0;
    int                 bucket_full_times;
    int                 *bucket_full = NULL;
    bcm_l2_addr_t       addr;
    int                 mac_inc = ad->opt_mac_inc;
    int                 vid_inc = ad->opt_vid_inc;
    uint8               key[XGS_HASH_KEY_SIZE];
#ifdef BCM_TRIUMPH_SUPPORT
    int                 ext_index_max = -1;
#endif /* BCM_TRIUMPH_SUPPORT */

    COMPILER_REFERENCE(a);

    entries = sal_alloc(sizeof(l2x_entry_t) * count, "L2 entries");
    if (!entries) {
        test_error(unit,
                   "Memory allocation failure\n");
        rv = -1;
        goto done;
    }
    sal_memset(entries, 0, sizeof(l2x_entry_t) * count);

    bucket_full = sal_alloc(sizeof(int) * count, "L2 full buckets");
    if (!bucket_full) {
        test_error(unit,
                   "Memory allocation failure\n");
        rv = -1;
        goto done;
    }
    sal_memset(bucket_full, 0, sizeof(int) * count);

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        ext_index_max = SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max;
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = -1;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    bcm_l2_addr_t_init(&addr, ad->opt_base_mac, ad->opt_base_vid);

    draco_l2_time_start(ad);

    bucket_full_times = 0;

    if (ad->opt_verbose) {
        cli_out("Inserting VLAN entries\n");
    }

    for (i = 0; i < count; i++) {
#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(unit)) {
            _bcm_tr_l2_to_l2x(unit, &(entries[i]), &addr, TRUE);
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            _bcm_fb_l2_to_l2x(unit, &(entries[i]), &addr);
        }

        if (ad->opt_verbose) {
            cli_out("Inserting VLAN %d entry\n", addr.vid);
        }

        if ((r = bcm_l2_addr_add(unit, &addr)) < 0) {
            if (r == SOC_E_FULL) {
                bucket_full_times++;
                bucket_full[i] = 1;
                continue;
            } else {
                test_error(unit,
                           "ARL insert failed on VLAN entry %d: %s\n",
                           i, soc_errmsg(r));
                rv = -1;
                goto done;
            }
        } else {
            bucket_full[i] = 0;
        }

        addr.vid += vid_inc;
        if (addr.vid > DRACO_L2_VID_MAX) {
            addr.vid = 1;
        }
        increment_macaddr(addr.mac, mac_inc);
    }

    vid = ad->opt_base_vid;

    for (i = 0; i < count; i++) {
        l2x_entry_t *cur_entry = &(entries[i]);
        if (!bucket_full[i]) {
            if (ad->opt_verbose) {
                cli_out("Attempting to delete by VLAN %d\n", vid);
            }

            r = bcm_l2_addr_delete_by_vlan(unit, vid, BCM_L2_DELETE_STATIC);
            if (r < 0) {
                test_error(unit,
                           "ARL delete by VLAN unsuccessful on VLAN %d\n", vid);
                rv = -1;
                goto done;
            } else {
                /* Delete claims to be successful, check */
                int soft_bucket;

#ifdef BCM_TRX_SUPPORT
                if (SOC_IS_TRX(unit)) {
                    int num_bits;

                    num_bits = soc_tr_l2x_base_entry_to_key(unit, cur_entry,
                                                            key);
                    soft_bucket = soc_tr_l2x_hash(unit, ad->opt_hash, num_bits,
                                                  cur_entry, key);
                } else
#endif
                {
                    soc_draco_l2x_base_entry_to_key(unit, cur_entry, key);
                    soft_bucket = soc_fb_l2_hash(unit, ad->opt_hash, key);
                }
                /* We should not find the entry when searching its bucket */
                if (draco_l2_bucket_search(unit, ad,
                                              soft_bucket, cur_entry) == 0) {
                    test_error(unit,
                               "ARL delete by VLAN failed on VLAN %d\n",
                               vid);
                    rv = -1;
                    goto done;
                }
            }
        }

        vid += vid_inc;
        if (vid > DRACO_L2_VID_MAX) {
            vid = 1;
        }
    }

    draco_l2_time_end(ad);

 done:
    if (bucket_full) {
        sal_free(bucket_full);
    }
    if (entries) {
        sal_free(entries);
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm)) {
        SOC_PERSIST(unit)->memState[EXT_L2_ENTRYm].index_max = ext_index_max;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    return rv;
}

/*
 * Test clean-up routine used for all ARL tests
 */

int
fb_l2_test_done(int unit, void *p)
{
    draco_l2_testdata_t *ad = p;

    if (ad == NULL) {
	return 0;
    }

    /* Check if empty at the end of the test */
    if (ad->opt_reset) {
        int rv, ix;
        int index_min = soc_mem_index_min(unit, L2Xm);
        int index_max = soc_mem_index_max(unit, L2Xm);
        uint32 *buf = 0;
        uint32 *ent;
        uint32 count;

        buf = soc_cm_salloc(unit,
                            SOC_MEM_TABLE_BYTES(unit, L2Xm),
                            "l2x_test");
        if (!buf) {
            test_error(unit, "Memory allocation failed\n");
            return (-1);
        }

        if ((rv = soc_mem_read_range(unit, L2Xm, MEM_BLOCK_ANY,
                          index_min, index_max, buf)) < 0) {
            test_error(unit, "Memory DMA of L2X entries failed\n");
            return (-1);
        }

        count = soc_mem_index_count(unit, L2Xm);
        for (ix = 0; ix < count; ix++) {
            ent = soc_mem_table_idx_to_pointer(unit, L2Xm,
                                               uint32 *, buf, ix);
            if (soc_L2Xm_field32_get(unit, ent, VALIDf)) {
                test_error(unit, "L2 table not empty after test entry = %d\n",
                                ix);
                soc_mem_entry_dump(unit, L2Xm, ent);
                return (-1);
            }
        }

        soc_cm_sfree(unit, buf);
    }

    if (WRITE_HASH_CONTROLr(unit, ad->save_hash_control) < 0) {
        test_error(unit, "Hash select restore failed\n");
    }

    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (WRITE_L2_AUX_HASH_CONTROLr(unit, ad->save_l2_aux_hash_control) < 0) {
            test_error(unit, "L2 AUX hash control restore failed\n");
        }
    }

    return 0;
}
#endif /* BCM_FIREBOLT_SUPPORT */

/*
 * Test of ARL hashing
 *
 *   This test tries a number of keys against one of the hashing functions,
 *   checking a software hash against the hardware hash, then searching the
 *   bucket to find the entry after inserting.
 *
 */
int
draco_l2_test_hash(int unit, args_t *a, void *p)
{
    int rv = 0;

    COMPILER_REFERENCE(a);

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
    if (SOC_IS_FBX(unit)) {
        return fb_l2_test_hash(unit, a, p);
    }
#endif /* BCM_FIREBOLT_SUPPORT || BCM_BRADLEY_SUPPORT */

    return rv;
}


/*
 * Test of ARL overflow behavior
 *
 *   This test fills each bucket of the ARL, then inserts another entry to see
 *   that the last entry fails to insert.
 *
 */

int
draco_l2_test_ov(int unit, args_t *a, void *p)
{
    int rv = 0;

    COMPILER_REFERENCE(a);

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
    if (SOC_IS_FBX(unit)) {
        return fb_l2_test_ov(unit, a, p);
    }
#endif /* BCM_FIREBOLT_SUPPORT || BCM_BRADLEY_SUPPORT */

    return rv;
}

/*
 * Test of ARL lookup
 *
 *   This test loops a given number of times.  It inserts a single ARL
 *   entry, looks up the ARL entry, and deletes the ARL entry.
 *   It is possible to run this test while heavy switching is going on.
 */

int
draco_l2_test_lu(int unit, args_t *a, void *p)
{
    int         rv = -1;

    COMPILER_REFERENCE(a);

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
    if (SOC_IS_FBX(unit)) {
        return fb_l2_test_lu(unit, a, p);
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    if (! soc_feature(unit, soc_feature_arl_lookup_cmd)) {
        test_error(unit, "ARL lookup not supported by chip (unit %d)\n", unit);
        return -1;
    }

    return rv;
}

/*
 * Test of ARL delete by port
 *
 * This test will insert a collection of entries with different ports,
 * then try to delete them by port reference.
 *
 */

int
draco_l2_test_dp(int unit, args_t *a, void *p)
{
    int rv = 0;

    COMPILER_REFERENCE(a);
#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
    if (SOC_IS_FBX(unit)) {
        return fb_l2_test_dp(unit, a, p);
    }
#endif /* BCM_FIREBOLT_SUPPORT */

    return rv;
}

/*
 * Test of ARL delete by VLAN
 *
 * This test will insert a collection of entries with different VLAN ids,
 * then try to delete them by VLAN.
 *
 */

int
draco_l2_test_dv(int unit, args_t *a, void *p)
{
    int rv = 0;

    COMPILER_REFERENCE(a);
#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
    if (SOC_IS_FBX(unit)) {
        return fb_l2_test_dv(unit, a, p);
    }
#endif /* BCM_FIREBOLT_SUPPORT */

    return rv;
}

/*
 * Test clean-up routine used for all ARL tests
 */

int
draco_l2_test_done(int unit, void *p)
{
    draco_l2_testdata_t *ad = p;

    if (ad == NULL) {
	return 0;
    }

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
    if (SOC_IS_FBX(unit)) {
        return fb_l2_test_done(unit, p);
    }
#endif /* BCM_FIREBOLT_SUPPORT */

    return 0;
}



#ifdef INCLUDE_L3
#ifdef BCM_FIREBOLT_SUPPORT
STATIC void
fbl3_setup(int unit, draco_l3_test_t *dw)
{
    draco_l3_testdata_t *ad;


    /* Hash */
    ad = &dw->dlp_l3_hash;

    ad->opt_count      = soc_mem_index_count(unit, L3_ENTRY_IPV6_MULTICASTm);
    ad->opt_hash       = FB_HASH_CRC16_LOWER;
    ad->opt_dual_hash  = -1;
    ad->hash_count     = FB_HASH_COUNT;
    sal_memset (ad->opt_src_ip6, 0, sizeof(ad->opt_src_ip6));
    sal_memset (ad->opt_base_ip6, 0, sizeof(ad->opt_base_ip6));
    ad->opt_base_ip6[15] = 1;
    ad->opt_src_ip6[15] = 1;
    ad->opt_ip6_inc =  1;
    ad->opt_src_ip6_inc =  1;
    ad->opt_base_vrf_id = 0;
    ad->opt_vrf_id_inc = 1;

    /* Overflow */
    ad = &dw->dlp_l3_ov;

    ad->opt_count      = soc_mem_index_count(unit, L3_ENTRY_IPV6_MULTICASTm);
    ad->opt_hash       = FB_L3_DEFAULT_HASH;
    ad->opt_dual_hash  = -1;
    ad->hash_count     = FB_HASH_COUNT;
    sal_memset (ad->opt_src_ip6, 0, sizeof(ad->opt_src_ip6));
    sal_memset (ad->opt_base_ip6, 0, sizeof(ad->opt_base_ip6));
    ad->opt_base_ip6[15] = 1;
    ad->opt_src_ip6[15] = 1;
    ad->opt_ip6_inc =  1;
    ad->opt_src_ip6_inc =  1;
    ad->opt_base_vrf_id = 0;
    ad->opt_vrf_id_inc = 1;
}

#endif /* BCM_FIREBOLT_SUPPORT */

STATIC void
dl3_setup(int unit, draco_l3_test_t *dw)
{
    draco_l3_testdata_t *ad;
    soc_mem_t           l3xm = INVALIDm;
    soc_mem_t           l3x_validm = INVALIDm;

    if (dw->dlw_set_up) {
        return;
    }
#ifdef BCM_FIREBOLT_SUPPORT
    /* Firebolt adjustments */
    if (SOC_IS_FBX(unit)) {
        l3xm = L3_ENTRY_IPV6_MULTICASTm;
        l3x_validm = L3_ENTRY_IPV6_MULTICASTm;
    }
#endif /* BCM_FIREBOLT_SUPPORT */

    dw->dlw_set_up = TRUE;
    dw->dlw_unit = unit;

    /* Hash */
    ad = &dw->dlp_l3_hash;

    ad->unit           = unit;
    ad->opt_count      = SOC_MEM_IS_VALID(unit, l3xm) ?
        soc_mem_index_count(unit, l3xm) : 0;
    ad->opt_verbose    = FALSE;
    ad->opt_reset      = TRUE;
    ad->opt_hash       = XGS_HASH_CRC16_LOWER;
    ad->opt_ipmc_enable = TRUE;
    ad->opt_key_src_ip = 1;
    ad->opt_base_ip    = 0;
    ad->opt_ip_inc     = 1;
    ad->opt_src_ip     = 0;
    ad->opt_src_ip_inc = 1;
    ad->opt_base_vid   = 0;
    ad->opt_vid_inc    = 1;
    ad->opt_mac_inc    = 1;
    ad->ipv6           = 0;
    ad->hash_count     = XGS_HASH_COUNT;
    ENET_SET_MACADDR(ad->opt_base_mac, dl_mac_src);

    /* Overflow */
    ad = &dw->dlp_l3_ov;

    ad->unit           = unit;
    ad->opt_count      = SOC_MEM_IS_VALID(unit, l3x_validm) ?
        soc_mem_index_count(unit, l3x_validm) : 0;
    ad->opt_verbose    = FALSE;
    ad->opt_reset      = TRUE;
    ad->opt_hash       = DRACO_ARL_DEFAULT_HASH;
    ad->opt_ipmc_enable = TRUE;
    ad->opt_key_src_ip = 1;
    ad->opt_base_ip    = 0;
    ad->opt_ip_inc     = 1;
    ad->opt_src_ip     = 0;
    ad->opt_src_ip_inc = 1;
    ad->opt_base_vid   = 0;
    ad->opt_vid_inc    = 1;
    ad->opt_mac_inc    = 1;
    ad->ipv6           = 0;
    ad->hash_count     = XGS_HASH_COUNT;
    ENET_SET_MACADDR(ad->opt_base_mac, dl_mac_src);

#ifdef BCM_FIREBOLT_SUPPORT
    /* Firebolt adjustments */
    if (SOC_IS_FBX(unit)) {
        fbl3_setup(unit, dw);
    }
#endif /* BCM_FIREBOLT_SUPPORT */
}

STATIC int
_xgs_l3_hash_save(int unit, draco_l3_testdata_t *ad)
{
    SOC_IF_ERROR_RETURN(_soc_mem_cmp_l3x_sync(unit));
    ad->save_ipmc_config = SOC_CONTROL(unit)->hash_key_config;
    return SOC_E_NONE;
}

STATIC int
_xgs_l3_hash_set(int unit, draco_l3_testdata_t *ad)
{
    ad->set_ipmc_config = ad->save_ipmc_config;
    if (ad->opt_ipmc_enable) {
        ad->set_ipmc_config |= L3X_IPMC_ENABLE;
    } else {
        ad->set_ipmc_config &= ~L3X_IPMC_ENABLE;
    }
    /* Turn off PIM_DM_SM mode */
    ad->set_ipmc_config &= ~L3X_IPMC_SIP;
    if (ad->opt_key_src_ip) {
        ad->set_ipmc_config &= ~L3X_IPMC_SIP0;
    } else {
        ad->set_ipmc_config |= L3X_IPMC_SIP0;
    }
    

    if (ad->set_ipmc_config != ad->save_ipmc_config) {
        SOC_IF_ERROR_RETURN(_soc_mem_cmp_l3x_set(unit, ad->set_ipmc_config));
    }
    return SOC_E_NONE;
}

/*
 * Test initialization routine used for all L3 tests
 */

STATIC int
draco_l3_test_init(int unit, draco_l3_testdata_t *ad, args_t *a)
{
    int                 rv = -1, dual = 0;
    parse_table_t       pt;
    uint32              hash_read = 0;
    uint32              dual_hash_read = 0;

    parse_table_init(unit, &pt);

    parse_table_add(&pt, "Count", PQ_INT|PQ_DFL, 0, &ad->opt_count, NULL);
    parse_table_add(&pt, "Verbose", PQ_BOOL|PQ_DFL, 0, &ad->opt_verbose, NULL);
    parse_table_add(&pt, "Reset", PQ_BOOL|PQ_DFL, 0, &ad->opt_reset, NULL);
    parse_table_add(&pt, "Hash", PQ_INT|PQ_DFL, 0, &ad->opt_hash, NULL);
    if (soc_feature(unit, soc_feature_dual_hash)) {
        parse_table_add(&pt, "DualHash", PQ_INT|PQ_DFL, 0, &ad->opt_dual_hash, NULL);
        parse_table_add(&pt, "DualEnable", PQ_INT|PQ_DFL, 0, &dual, NULL);
    }
    parse_table_add(&pt, "IPMCEnable", PQ_BOOL|PQ_DFL, 0,
                    &ad->opt_ipmc_enable, NULL);
    parse_table_add(&pt, "SrcIPKey", PQ_BOOL|PQ_DFL, 0,
                    &ad->opt_key_src_ip, NULL);
    if (ad->ipv6) {
        parse_table_add(&pt, "BaseIP", PQ_IP6|PQ_DFL, 0, &ad->opt_base_ip6, NULL);
        parse_table_add(&pt, "BIPIncrement", PQ_INT|PQ_DFL, 0,
                        &ad->opt_ip6_inc, NULL);
        parse_table_add(&pt, "SourceIP", PQ_IP6|PQ_DFL, 0,
                        &ad->opt_src_ip6, NULL);
        parse_table_add(&pt, "SrcIPIncrement", PQ_INT|PQ_DFL, 0,
                        &ad->opt_src_ip6_inc, NULL);
    } else {
        parse_table_add(&pt, "BaseIP", PQ_IP|PQ_DFL, 0, &ad->opt_base_ip, NULL);
        parse_table_add(&pt, "BIPIncrement", PQ_INT|PQ_DFL, 0,
                        &ad->opt_ip_inc, NULL);
        parse_table_add(&pt, "SourceIP", PQ_IP|PQ_DFL, 0, &ad->opt_src_ip, NULL);
        parse_table_add(&pt, "SrcIPIncrement", PQ_INT|PQ_DFL, 0,
                        &ad->opt_src_ip_inc, NULL);
    }
    parse_table_add(&pt, "BaseVID", PQ_HEX|PQ_DFL, 0, &ad->opt_base_vid, NULL);
    parse_table_add(&pt, "VIDIncrement", PQ_INT|PQ_DFL, 0,
                    &ad->opt_vid_inc, NULL);
    parse_table_add(&pt, "BaseMac", PQ_DFL|PQ_MAC, 0, &ad->opt_base_mac, NULL);
    parse_table_add(&pt, "MacIncrement", PQ_INT|PQ_DFL, 0,
                    &ad->opt_mac_inc, NULL);

    parse_table_add(&pt, "BaseVRFID", PQ_INT|PQ_DFL, 0,
                    &ad->opt_base_vrf_id, NULL);
    parse_table_add(&pt, "VRFIDIncrement", PQ_INT|PQ_DFL, 0,
                    &ad->opt_vrf_id_inc, NULL);

    /* Test the obvious parsings before wasting time with malloc */
    if (parse_arg_eq(a, &pt) < 0) {
        test_error(unit,
                   "%s: Error: Invalid option: %s\n", ARG_CMD(a),
                   ARG_CUR(a) ? ARG_CUR(a) : "*");
        goto done;
    }

    if (ad->opt_count < 1) {
        test_error(unit, "Illegal count %d\n", ad->opt_count);
        goto done;
    }

    if (ad->opt_hash >= ad->hash_count) {
        test_error(unit, "Illegal hash selection %d\n", ad->opt_hash);
        goto done;
    }

    if (dual == 1) {
        if (ad->opt_dual_hash >= ad->hash_count) {
            test_error(unit, "Illegal dual hash selection %d\n", ad->opt_dual_hash);
            goto done;
        }
    } else {
        ad->opt_dual_hash = -1;
    }

    /*
     * Re-initialize chip to ensure ARL and software ARL table are clear
     * at start of test.
     */

    if (ad->opt_reset) {
        BCM_IF_ERROR_RETURN(bcm_linkscan_enable_set(unit, 0));
        if (soc_reset_init(unit) < 0) {
            test_error(unit, "SOC initialization failed\n");
            goto done;
        }

        if (soc_misc_init(unit) < 0) {
            test_error(unit, "ARL initialization failed\n");
            goto done;
        }

        if (soc_mmu_init(unit) < 0) {
            test_error(unit, "MMU initialization failed\n");
            goto done;
        }

        /* Must initialize mbcm for L3 functions below */
        if (bcm_init(unit) < 0) {
            test_error(unit, "BCM layer initialization failed\n");
            goto done;
        }

        if (bcm_l3_init(unit) < 0) {
            test_error(unit, "L3 initialization failed\n");
            goto done;
        }

        if (SOC_FUNCTIONS(unit)->soc_age_timer_set(unit, 0, 0) < 0) {
            test_error(unit, "Could not disable age timers\n");
            goto done;
        }
    }

    if (READ_HASH_CONTROLr(unit, &hash_read) < 0) {
        test_error(unit, "Hash select read failed\n");
        goto done;
    }
    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (READ_L3_AUX_HASH_CONTROLr(unit, &dual_hash_read) < 0) {
            test_error(unit, "Dual Hash select read failed\n");
            goto done;
        }
    }

    ad->save_hash_control = hash_read;
    ad->save_dual_hash_control = dual_hash_read;
    hash_read = ad->opt_hash;
    dual_hash_read = ad->opt_dual_hash;
#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        hash_read = ad->save_hash_control;
        soc_reg_field_set(unit, HASH_CONTROLr, &hash_read,
                          L3_HASH_SELECTf, ad->opt_hash);
        if (soc_feature(unit, soc_feature_dual_hash)) {
            dual_hash_read = ad->save_dual_hash_control;
            if (ad->opt_dual_hash != -1) {
                soc_reg_field_set(unit, L3_AUX_HASH_CONTROLr, &dual_hash_read,
                                  HASH_SELECTf, ad->opt_dual_hash);
                soc_reg_field_set(unit, L3_AUX_HASH_CONTROLr, &dual_hash_read,
                                  ENABLEf, 1);
            } else {
                soc_reg_field_set(unit, L3_AUX_HASH_CONTROLr, &dual_hash_read,
                                  ENABLEf, 0);
            }
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    if (WRITE_HASH_CONTROLr(unit, hash_read) < 0) {
        test_error(unit, "Hash select set failed\n");
        goto done;
    }
    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (WRITE_L3_AUX_HASH_CONTROLr(unit, dual_hash_read) < 0) {
            test_error(unit, "Dual Hash select set failed\n");
            goto done;
        }
    }

    if (!SOC_IS_FBX(unit)) {
        if (_xgs_l3_hash_save(unit, ad) < 0) {
            test_error(unit, "hash state save failed");
            goto done;
        }
        if (_xgs_l3_hash_set(unit, ad) < 0) {
            test_error(unit, "hash state set failed");
            goto done;
        }

        if (_soc_mem_cmp_l3x_sync(unit) < 0) {
            test_error(unit, "Unable to sync L3 config\n");
            goto done;
        }
    }

    rv = 0;

 done:

    parse_arg_eq_done(&pt);
    return rv;
}

/* Individual test init wrappers */
int
draco_l3_hash_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_hash;
    int                    rv;

    dl3_setup(unit, dw);

    /* Set working data to hash */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
draco_l3_ov_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_ov;
    int                    rv;

    dl3_setup(unit, dw);

    /* Set working data to overflow */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

#ifdef BCM_XGS3_SWITCH_SUPPORT
/* Individual test init wrappers */
int
fb_l3_hash_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_hash;
    int                    rv;

    dl3_setup(unit, dw);
    dp->ipv6 = 0;

    /* Set working data to hash */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
fb_l3_ov_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_ov;
    int                    rv;

    dl3_setup(unit, dw);
    dp->ipv6 = 0;

    /* Set working data to overflow */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
fb_l3ip6_hash_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_hash;
    int                    rv;

    dl3_setup(unit, dw);
    dp->ipv6 = 1;

    /* Set working data to hash */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
fb_l3ip6_ov_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_ov;
    int                    rv;

    dl3_setup(unit, dw);
    dp->ipv6 = 1;

    /* Set working data to overflow */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
fb_l3_ipv6_hash_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_hash;
    int                    rv;

    dl3_setup(unit, dw);
    dp->ipv6 = 1;

    /* Set working data to hash */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

int
fb_l3_ipv6_ov_test_init(int unit, args_t *a, void **p)
{
    draco_l3_test_t        *dw = &dl3_work[unit];
    draco_l3_testdata_t    *dp = &dw->dlp_l3_ov;
    int                    rv;

    dl3_setup(unit, dw);
    dp->ipv6 = 1;

    /* Set working data to overflow */
    dw->dlp_l3_cur = dp;

    if ((rv = draco_l3_test_init(unit, dp, a)) < 0) {
        return rv;
    } else {
        *p = dp;
    }

    return 0;
}

#ifdef BCM_FIREBOLT_SUPPORT
static int
fb_l3_bucket_search(int unit, draco_l3_testdata_t *ad,
                    int bucket, uint32 *expect, int dual, int bank)
{
    uint32          entry[SOC_MAX_MEM_WORDS];
    int             i, ix, mem_table_index;
    int             num_vbits;
    int             num_ent_per_bucket, iter_count;
    soc_mem_t       mem;
    int             valid_ix;
    soc_field_t     valid_f[] = {VALIDf, VALID_0f, VALID_1f, VALID_2f, VALID_3f}; 
    int rv = -1; /* Assume failed unless we find it */

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
        int key_type;

        key_type = soc_mem_field32_get(unit, L3_ENTRY_ONLYm, 
                                       expect, KEY_TYPEf);
        switch (key_type) {
            case TR_L3_HASH_KEY_TYPE_V4UC:
                valid_ix = 0;
                num_vbits = 1;
                mem = L3_ENTRY_IPV4_UNICASTm;
                break;
            case TR_L3_HASH_KEY_TYPE_V4MC:
                valid_ix = 1;
                num_vbits = 2;
                mem = L3_ENTRY_IPV4_MULTICASTm;
                break;
            case TR_L3_HASH_KEY_TYPE_V6UC:
                valid_ix = 1;
                num_vbits = 2;
                mem = L3_ENTRY_IPV6_UNICASTm;
                break;
            case TR_L3_HASH_KEY_TYPE_V6MC:
                valid_ix = 1;
                num_vbits = 4;
                mem = L3_ENTRY_IPV6_MULTICASTm;
                break;
            default: 
                return -1;
        }
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
    if (soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, expect, V6f)) {
        valid_ix = 1;
        if (soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, expect, IPMCf)) {
            mem = L3_ENTRY_IPV6_MULTICASTm;
            num_vbits = 4;
        } else {
            mem = L3_ENTRY_IPV6_UNICASTm;
            num_vbits = 2;
        }
    } else {
        num_vbits = 1;
        valid_ix = 0;
        if (soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, expect, IPMCf)) {
            mem = L3_ENTRY_IPV4_MULTICASTm;
        } else {
            mem = L3_ENTRY_IPV4_UNICASTm;
        }
    }

    num_ent_per_bucket = SOC_L3X_BUCKET_SIZE(unit) / num_vbits;
    if (dual == TRUE) {
        iter_count = num_ent_per_bucket / 2;
    } else {
        iter_count = num_ent_per_bucket;
    }

    for (ix = 0; ix < iter_count; ix++) {
        mem_table_index = (bucket * num_ent_per_bucket) + ix;
        if (bank) {
            mem_table_index += (num_ent_per_bucket / 2);
        }

        if (soc_mem_read(unit, mem, MEM_BLOCK_ANY,
                             mem_table_index, entry) < 0) {
            test_error(unit,
                       "Read ARL failed at bucket %d, offset %d\n",
                       bucket, ix);
            break;
        }

        for(i = 0; i < num_vbits; i++) {
            if (!soc_mem_field32_get(unit, mem, entry, valid_f[valid_ix + i])) {
                break;
            }
        }
        if (i != num_vbits) {
            /* Valid bit not set, blank entry */
            continue;
        }

        if (soc_mem_compare_key(unit, mem, expect, entry) == 0) {
            /* Found the matching entry */
            rv = 0;
            break;
        }
    }

    return rv;
}

int
_fb_l3ip4ucast_test_hash(int unit, args_t *a, void *p)
{
    draco_l3_testdata_t         *ad = p;
    uint32                      entry[SOC_MAX_MEM_WORDS];
    uint32                      entry_lkup[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV4_UNICASTm;
    ip_addr_t                   dst_ip;
    ip_addr_t                   dst_ip_inc;
    bcm_vlan_t                  vid;
    int                         vid_inc;
    int                         r, rv = 0;
    int                         soft_bucket;
    int                         hard_index;
    int                         hard_bucket;
    int                         ix;
    int                         iterations;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0;
    int                         dual = FALSE;
    int                         bank, banks, bank_count = 1;

    COMPILER_REFERENCE(a);
    vrf_id_max = SOC_VRF_MAX(unit);

    dst_ip = ad->opt_base_ip; 
    dst_ip_inc = ad->opt_ip_inc; 
    vid = ad->opt_base_vid;
    vid_inc = ad->opt_vid_inc;
    
    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;

    if (ad->opt_verbose) {
        cli_out("Starting L3 hash test\n");
    }

    iterations = ad->opt_count;
    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (ad->opt_dual_hash != -1) {
            dual = TRUE;
            bank_count = 2;
        }
    }

    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT 
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
        soc_mem_field32_set(unit, mem, entry, KEY_TYPEf, 
                            TR_L3_HASH_KEY_TYPE_V4UC);
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, entry, V6f, 0);
        soc_mem_field32_set(unit, mem, entry, IPMCf, 0);
    }
    soc_mem_field32_set(unit, mem, entry, VALIDf, 1);
    if (vrf_id_max) {
        soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
    }

    for (ix = 0; ix < iterations; ix++) {
        for (bank = 0; bank < bank_count; bank++) {
            soc_mem_field_set(unit, mem, entry, IP_ADDRf, &dst_ip);
            if (SOC_MEM_FIELD_VALID(unit, mem, VLAN_IDf)) {
                soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
            }
            if (vrf_id_max) {
                soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
            } 
            soft_bucket = soc_fb_l3x2_entry_hash(unit, entry);
            banks = 0x3;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            if (dual == TRUE) {
                soft_bucket = soc_fb_l3x_bank_entry_hash(unit, bank, entry);
                banks = (bank == 0) ? 2 : 1;
            }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
    
            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, &entry);
                cli_out("\n");
                if (dual) {
                    cli_out("into bucket 0x%x (bank %d)\n",
                            soft_bucket, bank);
                } else {
                   cli_out("into bucket 0x%x\n", soft_bucket);
                }
            }

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            r = soc_mem_bank_insert(unit, mem, banks, COPYNO_ALL, entry, NULL);
#else  /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            r = soc_mem_insert(unit, mem, COPYNO_ALL, entry);
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            if (SOC_FAILURE(r)) {
                if (r == SOC_E_FULL) {
                    /* Bucket overflow, just pass on */
                    continue;
                } else {
                    test_error(unit, "L3 insert failed at bucket %d\n", soft_bucket);
                    rv = -1;
                    goto done;
                }
            }

            /* Now we search for the entry */
            hard_index = 0;
            if (soc_mem_search(unit, mem, COPYNO_ALL,
                               &hard_index, entry, entry_lkup, 0) < 0) {
                test_error(unit, "Entry lookup failed\n");
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }
    
            hard_bucket = (hard_index >> (SOC_L3X_BUCKET_SIZE(unit) == 16 ? 4 : 3));
			    /* entries per bucket */
            if (soft_bucket != hard_bucket) {
                test_error(unit,
                           "Software Hash %d disagrees with  Hardware Hash %d\n",
                           soft_bucket, hard_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }
    
            /* Only do a quick check vs. expected bucket here */
            if (fb_l3_bucket_search(unit, ad, soft_bucket, entry, dual, bank) < 0) {
                test_error(unit, "Entry not found in bucket %d\n", soft_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                rv = -1;
                goto done;
            }
    
            /* Clean up afterward */
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", soft_bucket);
                rv = -1;
                goto done;
            }
    
        }
        dst_ip += dst_ip_inc;
        vid += vid_inc;
        if (vid > DRACO_L3_VID_MAX) {
            vid = 1;
        }
        if (vrf_id_max) {
            vrf_id += vrf_id_inc;
            if (vrf_id > vrf_id_max) {
                vrf_id = 0;
            }
        } 
    }

 done:
    return rv;
}

int
_fb_l3ip4mcast_test_hash(int unit, args_t *a, void *p)
{
    draco_l3_testdata_t         *ad = p;
    uint32                      entry[SOC_MAX_MEM_WORDS];
    uint32                      entry_lkup[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV4_MULTICASTm;
    ip_addr_t                   src_ip;
    ip_addr_t                   dst_ip;
    ip_addr_t                   src_ip_inc;
    ip_addr_t                   dst_ip_inc;
    bcm_vlan_t                  vid;
    int                         vid_inc;
    int                         r, rv = 0;
    int                         soft_bucket;
    int                         hard_index;
    int                         hard_bucket;
    int                         ix;
    int                         iterations;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0;
    int                         ent_per_bucket;
    int                         dual = FALSE;
    int                         bank, banks, bank_count = 1;

    COMPILER_REFERENCE(a);
    vrf_id_max = SOC_VRF_MAX(unit);

    if (ad->opt_key_src_ip) {
        src_ip_inc = ad->opt_src_ip_inc;
        src_ip = ad->opt_src_ip;
    } else {
        src_ip_inc = 0;
        src_ip = 0;
    }
    dst_ip = ad->opt_base_ip; 
    dst_ip_inc = ad->opt_ip_inc; 
    vid = ad->opt_base_vid;
    vid_inc = ad->opt_vid_inc;

    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;

    /*  Invalid IPMC/Unicast address checks required */

    if (ad->opt_verbose) {
        cli_out("Starting L3 hash test\n");
    }

    iterations = ad->opt_count;
    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (ad->opt_dual_hash != -1) {
            dual = TRUE;
            bank_count = 2;
        }
    }

    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) { 
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_0f,
                            TR_L3_HASH_KEY_TYPE_V4MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_1f,
                            TR_L3_HASH_KEY_TYPE_V4MC);
        soc_mem_field32_set(unit, mem, entry, VALID_0f, 1);
        soc_mem_field32_set(unit, mem, entry, VALID_1f, 1);
    } else
#endif
    {
        soc_mem_field32_set(unit, mem, entry, V6f, 0);
        soc_mem_field32_set(unit, mem, entry, IPMCf, 1);
        soc_mem_field32_set(unit, mem, entry, VALIDf, 1);
    }
    if (vrf_id_max) {
        soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
    } 
    for (ix = 0; ix < iterations; ix++) {
        for (bank = 0; bank < bank_count; bank++) {
            soc_mem_field_set(unit, mem, entry, GROUP_IP_ADDRf, &dst_ip);
            soc_mem_field_set(unit, mem, entry, SOURCE_IP_ADDRf, &src_ip);
            soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
            if (vrf_id_max) {
                soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
            } 
    
            soft_bucket = soc_fb_l3x2_entry_hash(unit, entry);
            banks = 0x3;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            if (dual == TRUE) {
                soft_bucket = soc_fb_l3x_bank_entry_hash(unit, bank, entry);
                banks = (bank == 0) ? 2 : 1;
            }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
    
            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, &entry);
                if (dual) {
                    cli_out("into bucket 0x%x (bank %d)",
                            soft_bucket, bank);
                } else {
                   cli_out("into bucket 0x%x", soft_bucket);
                }
            }

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            r = soc_mem_bank_insert(unit, mem, banks, COPYNO_ALL, entry, NULL);
#else  /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            r = soc_mem_insert(unit, mem, COPYNO_ALL, entry);
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            if (SOC_FAILURE(r)) {
                if (r == SOC_E_FULL) {
                    /* Bucket overflow, just pass on */
                    continue;
                } else {
                    test_error(unit,
                               "L3 insert failed at bucket %d\n", soft_bucket);
                    rv = -1;
                    goto done;
                }
            }
    
            /* Now we search for the entry */
            hard_index = 0;
            if (soc_mem_search(unit, mem, COPYNO_ALL,
                               &hard_index, entry, entry_lkup, 0) < 0) {
                test_error(unit, "Entry lookup failed\n");
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }

#ifdef BCM_TRIUMPH_SUPPORT
            if (SOC_IS_TR_VL(unit)) {
                ent_per_bucket = 8;
            } else
#endif
            if (SOC_L3X_BUCKET_SIZE(unit) == 16) {
                ent_per_bucket = 16;
            } else {
                ent_per_bucket = 8;
            }
            hard_bucket = (hard_index / ent_per_bucket); /* entries per bucket */
            if (soft_bucket != hard_bucket) {
                test_error(unit,
                           "Software Hash %d disagrees with  Hardware Hash %d\n",
                           soft_bucket, hard_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }
    
            /* Only do a quick check vs. expected bucket here */
            if (fb_l3_bucket_search(unit, ad, soft_bucket, entry, dual, bank) < 0) {
                test_error(unit, "Entry not found in bucket %d\n", soft_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                rv = -1;
                goto done;
            }
    
            /* Clean up afterward */
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", soft_bucket);
                rv = -1;
                goto done;
            }
    
            src_ip += src_ip_inc;
            dst_ip += dst_ip_inc;
            vid += vid_inc;
            if (vid > DRACO_L3_VID_MAX) {
                vid = 1;
            }
            if (vrf_id_max) {
                vrf_id += vrf_id_inc;
                if (vrf_id > vrf_id_max) {
                    vrf_id = 0;
                }
            }
        }
    }

 done:
    return rv;
}

int
_fb_l3ip6ucast_test_hash(int unit, args_t *a, void *p)
{
    draco_l3_testdata_t         *ad = p;
    uint32                      entry[SOC_MAX_MEM_WORDS];
    uint32                      entry_lkup[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV6_UNICASTm;
    ip6_addr_t                  dst_ip;
    uint32                      ip_tmp[2];
    int                         dst_ip_inc;
    int                         r, rv = 0;
    int                         soft_bucket;
    int                         hard_index;
    int                         hard_bucket;
    int                         ix;
    int                         iterations;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0;
    int                         dual = FALSE;
    int                         bank, banks, bank_count = 1;

    COMPILER_REFERENCE(a);

    vrf_id_max = SOC_VRF_MAX(unit);

    memcpy(dst_ip, ad->opt_base_ip6, sizeof(ip6_addr_t));
    dst_ip_inc = ad->opt_ip6_inc;
    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;

    if (ad->opt_verbose) {
        cli_out("Starting L3 hash test\n");
    }

    iterations = ad->opt_count;
    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (ad->opt_dual_hash != -1) {
            dual = TRUE;
            bank_count = 2;
        }
    }

    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) { 
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_0f,
                            TR_L3_HASH_KEY_TYPE_V6UC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_1f,
                            TR_L3_HASH_KEY_TYPE_V6UC);
        if (vrf_id_max) {
            soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
        }
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, entry, V6_0f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_1f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_0f, 0);
        soc_mem_field32_set(unit, mem, entry, IPMC_1f, 0);
#if  defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
        defined(BCM_SCORPION_SUPPORT)
        if (vrf_id_max) {
            soc_mem_field32_set(unit, mem, entry, VRF_ID_0f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_1f, 1);
        }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
    }
    soc_mem_field32_set(unit, mem, entry, VALID_0f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_1f, 1);
    for (ix = 0; ix < iterations; ix++) {
        for (bank = 0; bank < bank_count; bank++) {
            ip_tmp[1] = ((dst_ip[8] << 24) | (dst_ip[9] << 16) |
                         (dst_ip[10] << 8) | (dst_ip[11] << 0));
            ip_tmp[0] = ((dst_ip[12] << 24) | (dst_ip[13] << 16) |
                         (dst_ip[14] << 8) | (dst_ip[15] << 0));
            soc_mem_field_set(unit, mem, entry,
                              IP_ADDR_LWR_64f, ip_tmp);
    
            ip_tmp[1] = ((dst_ip[0] << 24) | (dst_ip[1] << 16) |
                         (dst_ip[2] << 8) | (dst_ip[3] << 0));
            ip_tmp[0] = ((dst_ip[4] << 24) | (dst_ip[5] << 16) |
                         (dst_ip[6] << 8) | (dst_ip[7] << 0));
            soc_mem_field_set(unit, mem, entry,
                              IP_ADDR_UPR_64f, ip_tmp);

#ifdef BCM_TRIUMPH_SUPPORT
            if (soc_feature(unit, soc_feature_l3_entry_key_type)) { 
                if (vrf_id_max) {
                    soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
                } 
            } else
#endif /* BCM_TRIUMPH_SUPPORT */
            {
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
                if (vrf_id_max) {
                    soc_mem_field32_set(unit, mem, entry, VRF_ID_0f, vrf_id);
                    soc_mem_field32_set(unit, mem, entry, VRF_ID_1f, vrf_id);
                } 
#endif /*  BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
            }
            soft_bucket = soc_fb_l3x2_entry_hash(unit, entry);
            banks = 0x3;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            if (dual == TRUE) {
                soft_bucket = soc_fb_l3x_bank_entry_hash(unit, bank, entry);
                banks = (bank == 0) ? 2 : 1;
            }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
    
            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, &entry);
                if (dual) {
                    cli_out("into bucket 0x%x (bank %d)", soft_bucket, bank);
                } else {
                   cli_out("into bucket 0x%x", soft_bucket);
                }
            }
    
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            r = soc_mem_bank_insert(unit, mem, banks, COPYNO_ALL, entry, NULL);
#else  /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            r = soc_mem_insert(unit, mem, COPYNO_ALL, entry);
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            if (SOC_FAILURE(r)) {
                if (r == SOC_E_FULL) {
                    /* Bucket overflow, just pass on */
                    continue;
                } else {
                    test_error(unit, "L3 insert failed at bucket %d\n", soft_bucket);
                    rv = -1;
                    goto done;
                }
            }
    
            /* Now we search for the entry */
            hard_index = 0;
            if (soc_mem_search(unit, mem, COPYNO_ALL,
                               &hard_index, entry, entry_lkup, 0) < 0) {
                test_error(unit, "Entry lookup failed\n");
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }
    
            hard_bucket = (hard_index >> (SOC_L3X_BUCKET_SIZE(unit) == 16 ? 3 : 2)); 
			    /* entries per bucket */
            if (soft_bucket != hard_bucket) {
                test_error(unit,
                           "Software Hash %d disagrees with  Hardware Hash %d\n",
                           soft_bucket, hard_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }
    
            /* Only do a quick check vs. expected bucket here */
            if (fb_l3_bucket_search(unit, ad, soft_bucket, entry, dual, bank) < 0) {
                test_error(unit, "Entry not found in bucket %d\n", soft_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                rv = -1;
                goto done;
            }
    
            /* Clean up afterward */
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", soft_bucket);
                rv = -1;
                goto done;
            }
            increment_ip6addr(dst_ip, 15, dst_ip_inc);
            if (vrf_id_max) {
                vrf_id += vrf_id_inc;
                if (vrf_id > vrf_id_max) {
                    vrf_id = 0;
                }
            } 
        }
    }

 done:
    return rv;
}

int
_fb_l3ip6mcast_test_hash(int unit, args_t *a, void *p)
{
    draco_l3_testdata_t         *ad = p;
    uint32                      entry[SOC_MAX_MEM_WORDS];
    uint32                      entry_lkup[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV6_MULTICASTm;
    ip6_addr_t                  src_ip;
    ip6_addr_t                  dst_ip;
    uint32                      ip_tmp[2];
    int                         src_ip_inc;
    int                         dst_ip_inc;
    bcm_vlan_t                  vid;
    int                         vid_inc;
    int                         r, rv = 0;
    int                         soft_bucket;
    int                         hard_index;
    int                         hard_bucket;
    int                         ix;
    int                         iterations;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0; 
    int                         dual = FALSE;
    int                         bank, banks, bank_count = 1;

    COMPILER_REFERENCE(a);
    vrf_id_max = SOC_VRF_MAX(unit);

    if (ad->opt_key_src_ip) {
        memcpy(src_ip, ad->opt_src_ip6, sizeof(ip6_addr_t));
        src_ip_inc = ad->opt_src_ip6_inc;
    } else {
        sal_memset (src_ip, 0, sizeof(src_ip));
        src_ip_inc = 0;
    }
    memcpy(dst_ip, ad->opt_base_ip6, sizeof(ip6_addr_t));
    dst_ip_inc = ad->opt_ip6_inc;
    vid = ad->opt_base_vid;
    vid_inc = ad->opt_vid_inc;
    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;

    /*  Invalid IPMC/Unicast address checks required */

    if (ad->opt_verbose) {
        cli_out("Starting L3 hash test\n");
    }

    iterations = ad->opt_count;
    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (ad->opt_dual_hash != -1) {
            dual = TRUE;
            bank_count = 2;
        }
    }

    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) { 
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_0f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_1f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_2f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_3f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        if (vrf_id_max) {
            soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
        }
    } else 
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, entry, V6_0f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_1f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_2f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_3f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_0f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_1f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_2f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_3f, 1);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||\
    defined(BCM_SCORPION_SUPPORT)
        if (vrf_id_max) {
            soc_mem_field32_set(unit, mem, entry, VRF_ID_0f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_1f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_2f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_3f, 1);
         }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
    }
    soc_mem_field32_set(unit, mem, entry, VALID_0f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_1f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_2f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_3f, 1);
    for (ix = 0; ix < iterations; ix++) {
        for (bank = 0; bank < bank_count; bank++) {
            dst_ip[0] = 0;
            ip_tmp[1] = ((dst_ip[8] << 24) | (dst_ip[9] << 16) |
                         (dst_ip[10] << 8) | (dst_ip[11] << 0));
            ip_tmp[0] = ((dst_ip[12] << 24) | (dst_ip[13] << 16) |
                         (dst_ip[14] << 8) | (dst_ip[15] << 0));
            soc_mem_field_set(unit, mem, entry,
                              GROUP_IP_ADDR_LWR_64f, ip_tmp);
    
            ip_tmp[1] = ((dst_ip[0] << 24) | (dst_ip[1] << 16) |
                         (dst_ip[2] << 8) | (dst_ip[3] << 0));
            ip_tmp[0] = ((dst_ip[4] << 24) | (dst_ip[5] << 16) |
                         (dst_ip[6] << 8) | (dst_ip[7] << 0));
            soc_mem_field_set(unit, mem, entry,
                              GROUP_IP_ADDR_UPR_56f, ip_tmp);
    
            ip_tmp[1] = ((src_ip[8] << 24) | (src_ip[9] << 16) |
                         (src_ip[10] << 8) | (src_ip[11] << 0));
            ip_tmp[0] = ((src_ip[12] << 24) | (src_ip[13] << 16) |
                         (src_ip[14] << 8) | (src_ip[15] << 0));
            soc_mem_field_set(unit, mem, entry,
                              SOURCE_IP_ADDR_LWR_64f, ip_tmp);
    
            ip_tmp[1] = ((src_ip[0] << 24) | (src_ip[1] << 16) |
                         (src_ip[2] << 8) | (src_ip[3] << 0));
            ip_tmp[0] = ((src_ip[4] << 24) | (src_ip[5] << 16) |
                         (src_ip[6] << 8) | (src_ip[7] << 0));
            soc_mem_field_set(unit, mem, entry,
                              SOURCE_IP_ADDR_UPR_64f, ip_tmp);


#ifdef BCM_TRIUMPH_SUPPORT
            if (soc_feature(unit, soc_feature_l3_entry_key_type)) { 
                soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
                if (vrf_id_max) {
                    soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
                } 
            } else
#endif /* BCM_TRIUMPH_SUPPORT */
            {
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_0f, vid);
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_1f, vid);
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_2f, vid);
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_3f, vid);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
                if (vrf_id_max) {
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_0f, vrf_id);
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_1f, vrf_id);
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_2f, vrf_id);
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_3f, vrf_id);
                } 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
            }
            soft_bucket = soc_fb_l3x2_entry_hash(unit, entry);
            banks = 0x3;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            if (dual == TRUE) {
                soft_bucket = soc_fb_l3x_bank_entry_hash(unit, bank, entry);
                banks = (bank == 0) ? 2 : 1;
            }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */

            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, &entry);
                if (dual) {
                    cli_out("into bucket 0x%x (bank %d)", soft_bucket, bank);
                } else {
                   cli_out("into bucket 0x%x", soft_bucket);
                }
            }
    
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
            defined(BCM_TRX_SUPPORT)
            r = soc_mem_bank_insert(unit, mem, banks, COPYNO_ALL, entry, NULL);
#else  /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            r = soc_mem_insert(unit, mem, COPYNO_ALL, entry);
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
            if (SOC_FAILURE(r)) {
                if (r == SOC_E_FULL) {
                    /* Bucket overflow, just pass on */
                    continue;
                } else {
                    test_error(unit,
                               "L3 insert failed at bucket %d\n", soft_bucket);
                    rv = -1;
                    goto done;
                }
            }
    
            /* Now we search for the entry */
            hard_index = 0;
            if (soc_mem_search(unit, mem, COPYNO_ALL,
                               &hard_index, entry, entry_lkup, 0) < 0) {
                test_error(unit, "Entry lookup failed\n");
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }
    
            hard_bucket = (hard_index >> (SOC_L3X_BUCKET_SIZE(unit) == 16 ? 2 : 1)); 
			    /* entries per bucket */
            if (soft_bucket != hard_bucket) {
                test_error(unit,
                           "Software Hash %d disagrees with  Hardware Hash %d\n",
                           soft_bucket, hard_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                goto done;
            }
    
            /* Only do a quick check vs. expected bucket here */
            if (fb_l3_bucket_search(unit, ad, soft_bucket, entry, dual, bank) < 0) {
                test_error(unit, "Entry not found in bucket %d\n", soft_bucket);
                soc_mem_entry_dump(unit, mem, entry);
                rv = -1;
                goto done;
            }
    
            /* Clean up afterward */
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", soft_bucket);
                rv = -1;
                goto done;
            }
    
            increment_ip6addr(src_ip, 15, src_ip_inc);
            increment_ip6addr(dst_ip, 15, dst_ip_inc);
            vid += vid_inc;
            if (vid > DRACO_L3_VID_MAX) {
                vid = 1;
            }
            if (vrf_id_max) {
                vrf_id += vrf_id_inc;
                if (vrf_id > vrf_id_max) {
                    vrf_id = 0;
                }
            }
        }
    }

 done:
    return rv;
}

STATIC uint32 entry_tmp[SOC_L3X_MAX_BUCKET_SIZE][SOC_MAX_MEM_WORDS];
int
_fb_l3ip4ucast_test_ov(int unit, args_t *a, void *p)
{
    uint32                      entry[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV4_UNICASTm;
    int                         num_ent_per_bucket;
    int                         num_vbits;
    int                         num_buckets; 
    ip_addr_t                   dst_ip;
    bcm_vlan_t                  vid;
    int                         vid_inc;
    int                         r, rv = 0;
    int                         bucket;
    int                         i, j;
    int                         iterations;
    draco_l3_testdata_t         *ad = p;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0;


    COMPILER_REFERENCE(a);
    vrf_id_max = SOC_VRF_MAX(unit);

    num_vbits = 1;
    num_ent_per_bucket = SOC_L3X_BUCKET_SIZE(unit) / num_vbits;
    num_buckets = soc_mem_index_count(unit, mem) / num_ent_per_bucket;
    iterations = (ad->opt_count > num_buckets) ? num_buckets : ad->opt_count;

    dst_ip = ad->opt_base_ip; 
    vid = ad->opt_base_vid;
    vid_inc = ad->opt_vid_inc;
    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;

    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
        soc_mem_field32_set(unit, mem, entry, KEY_TYPEf,
                            TR_L3_HASH_KEY_TYPE_V4UC);
    } else  
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, entry, V6f, 0);
        soc_mem_field32_set(unit, mem, entry, IPMCf, 0);
    }
    soc_mem_field32_set(unit, mem, entry, VALIDf, 1);
    if (vrf_id_max) { 
        soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
    }

    while (iterations--) {
        soc_mem_field_set(unit, mem, entry, IP_ADDRf, &dst_ip);
        if (SOC_MEM_FIELD_VALID(unit, mem, VLAN_IDf)) {
            soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
        }
        if (vrf_id_max) { 
            soc_mem_field32_set(unit, mem, entry, VRF_IDf, vrf_id);
        } 

        bucket = soc_fb_l3x2_entry_hash(unit, entry);

        for (i = 0; i < num_ent_per_bucket; i++) {
            memcpy(entry_tmp[i], entry, sizeof(entry));
            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, entry_tmp[i]);
                cli_out("into bucket 0x%x\n", bucket);
            }

            if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry_tmp[i])) < 0) {
                if (r == SOC_E_FULL) {
                    /* Already full, stop wasting time */
                    break;
                } else {
                    test_error(unit, "L3 insert failed at bucket %d\n", bucket);
                    rv = -1;
                    goto done;
                }
            }

            /* Increment key by number of buckets in the table */
            dst_ip += num_buckets;
            vid += vid_inc;
            if (vid > DRACO_L3_VID_MAX) {
                vid = 1;
            }
            if (vrf_id_max) { 
                vrf_id += vrf_id_inc;
                if (vrf_id > vrf_id_max) {
                    vrf_id = 0;
                }
            }
            soc_mem_field_set(unit, mem, entry, IP_ADDRf, &dst_ip);
            if (SOC_MEM_FIELD_VALID(unit, mem, VLAN_IDf)) {
                soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
            }
            if (vrf_id_max) { 
                soc_mem_field32_set(unit, mem, entry, VRF_IDf, vrf_id);
            } 
        }

        if (ad->opt_verbose) {
            cli_out("Inserting %d entry in bucket %d, should fail\n",
                    num_ent_per_bucket, bucket);
        }

        if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry)) < 0) {
            if (r != SOC_E_FULL) {
                test_error(unit, "L3 insert failed\n");
                rv = -1;
                goto done;
            }
        } else {
            test_error(unit, "L3 insert to full bucket succeeded\n");
            rv = -1;
            goto done;
        }

        if (ad->opt_verbose) {
            cli_out("Verifying installed entries\n");
        }

        /* Verify bucket contains added entries */
        for (j = 0; j < i; j++) {
            if (fb_l3_bucket_search(unit, ad, bucket, entry_tmp[j], FALSE, 0) < 0) {
                test_error(unit, "L3 entry missing at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        if (ad->opt_verbose) {
            cli_out("Cleaning bucket %d\n", bucket);
        }

        /* Remove the entries that we added */
        for (j = 0; j < i; j++) {
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry_tmp[j]) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        /* Do Next bucket */
        dst_ip += 1;
        vid += vid_inc;
        if (vid > DRACO_L3_VID_MAX) {
            vid = 1;
        }
        if (vrf_id_max) { 
            vrf_id += vrf_id_inc;
            if (vrf_id > vrf_id_max) {
                vrf_id = 0;
            }
        }
    }

 done:

    return rv;
}

int
_fb_l3ip4mcast_test_ov(int unit, args_t *a, void *p)
{
    uint32                      entry[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV4_MULTICASTm;
    int                         num_ent_per_bucket;
    int                         num_vbits;
    int                         num_buckets; 
    ip_addr_t                   src_ip;
    ip_addr_t                   dst_ip;
    int                         src_ip_inc;
    bcm_vlan_t                  vid;
    int                         vid_inc;
    int                         r, rv = 0;
    int                         bucket;
    int                         i, j;
    int                         iterations;
    draco_l3_testdata_t         *ad = p;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0;
    uint32 hash_read;


    COMPILER_REFERENCE(a);
    num_vbits = 1;
    
    vrf_id_max = SOC_VRF_MAX(unit);
#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TR_VL(unit)) {
        num_vbits = 2;
    }
#endif

    num_ent_per_bucket = SOC_L3X_BUCKET_SIZE(unit) / num_vbits;
    num_buckets = soc_mem_index_count(unit, mem) / num_ent_per_bucket;
    iterations = (ad->opt_count > num_buckets) ? num_buckets : ad->opt_count;

    if (ad->opt_key_src_ip) {
        src_ip_inc = ad->opt_src_ip_inc;
        src_ip = ad->opt_src_ip;
    } else {
        src_ip_inc = 0;
        src_ip = 0;
    }
    dst_ip = ad->opt_base_ip; 
    vid = ad->opt_base_vid;
    vid_inc = ad->opt_vid_inc;
    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;


    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_0f,
                            TR_L3_HASH_KEY_TYPE_V4MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_1f,
                            TR_L3_HASH_KEY_TYPE_V4MC);
        soc_mem_field32_set(unit, mem, entry, VALID_0f, 1);
        soc_mem_field32_set(unit, mem, entry, VALID_1f, 1);
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, entry, V6f, 0);
        soc_mem_field32_set(unit, mem, entry, IPMCf, 1);
        soc_mem_field32_set(unit, mem, entry, VALIDf, 1);
    }
    if (vrf_id_max) {
        soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
    } 
    while (iterations--) {
        soc_mem_field_set(unit, mem, entry, GROUP_IP_ADDRf, &dst_ip);
        soc_mem_field_set(unit, mem, entry, SOURCE_IP_ADDRf, &src_ip);
        soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
        if (vrf_id_max) {
            soc_mem_field32_set(unit, mem, entry, VRF_IDf, vrf_id);
        } 

        bucket = soc_fb_l3x2_entry_hash(unit, entry);

        for (i = 0; i < num_ent_per_bucket; i++) {
            memcpy(entry_tmp[i], entry, sizeof(entry));
            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, entry_tmp[i]);
                cli_out("into bucket 0x%x\n", bucket);
            }

            if (READ_HASH_CONTROLr(unit, &hash_read) < 0) {
                test_error(unit, "Hash select read failed\n");
                goto done;
            }

            if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry_tmp[i])) < 0) {
                if (r == SOC_E_FULL) {
                    /* Already full, stop wasting time */
                    break;
                } else {
                    test_error(unit, "L3 insert failed at bucket %d\n", bucket);
                    rv = -1;
                    goto done;
                }
            }

            if (READ_HASH_CONTROLr(unit, &hash_read) < 0) {
                test_error(unit, "Hash select read failed\n");
                goto done;
            }

            /* Increment key by number of buckets in the table */
            dst_ip += num_buckets;
            src_ip += src_ip_inc;
            vid += vid_inc;
            if (vid > DRACO_L3_VID_MAX) {
                vid = 1;
            }
            if (vrf_id_max) {
                vrf_id += vrf_id_inc;
                if (vrf_id > vrf_id_max) {
                    vrf_id = 0;
                }
            }
            soc_mem_field_set(unit, mem, entry, GROUP_IP_ADDRf, &dst_ip);
            soc_mem_field_set(unit, mem, entry, SOURCE_IP_ADDRf, &src_ip);
            soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
            if (vrf_id_max) {
                soc_mem_field32_set(unit, mem, entry, VRF_IDf, vrf_id);
            } 
        }

        if (ad->opt_verbose) {
            cli_out("Inserting %d entry in bucket %d, should fail\n",
                    num_ent_per_bucket, bucket);
        }

        if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry)) < 0) {
            if (r != SOC_E_FULL) {
                test_error(unit, "L3 insert failed\n");
                rv = -1;
                goto done;
            }
        } else {
            test_error(unit, "L3 insert to full bucket succeeded\n");
            rv = -1;
            goto done;
        }

        if (ad->opt_verbose) {
            cli_out("Verifying installed entries\n");
        }

        /* Verify bucket contains added entries */
        for (j = 0; j < i; j++) {
            if (fb_l3_bucket_search(unit, ad, bucket, entry_tmp[j], FALSE, 0) < 0) {
                test_error(unit, "L3 entry missing at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        if (ad->opt_verbose) {
            cli_out("Cleaning bucket %d\n", bucket);
        }

        /* Remove the entries that we added */
        for (j = 0; j < i; j++) {
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry_tmp[j]) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        /* Do Next bucket */
        dst_ip += 1;
        src_ip += src_ip_inc;
        vid += vid_inc;
        if (vid > DRACO_L3_VID_MAX) {
            vid = 1;
        }
        if (vrf_id_max) {
            vrf_id += vrf_id_inc;
            if (vrf_id > vrf_id_max) {
                vrf_id = 0;
            }
        }
    }

 done:

    return rv;
}

int
_fb_l3ip6ucast_test_ov(int unit, args_t *a, void *p)
{
    uint32                      entry[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV6_UNICASTm;
    int                         num_ent_per_bucket;
    int                         num_vbits;
    int                         num_buckets; 
    ip6_addr_t                  dst_ip;
    uint32                      ip_tmp[2];
    bcm_vlan_t                  vid;
    int                         vid_inc;
    int                         r, rv = 0;
    int                         bucket;
    int                         i, j;
    int                         iterations;
    draco_l3_testdata_t         *ad = p;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0; 

    COMPILER_REFERENCE(a);
    vrf_id_max = SOC_VRF_MAX(unit);

    num_vbits = 2;
    num_ent_per_bucket = SOC_L3X_BUCKET_SIZE(unit) / num_vbits;
    num_buckets = soc_mem_index_count(unit, mem) / num_ent_per_bucket;
    iterations = (ad->opt_count > num_buckets) ? num_buckets : ad->opt_count;

    memcpy(dst_ip, ad->opt_base_ip6, sizeof(ip6_addr_t));
    vid = ad->opt_base_vid;
    vid_inc = ad->opt_vid_inc;
    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;
    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_0f,
                            TR_L3_HASH_KEY_TYPE_V6UC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_1f,
                            TR_L3_HASH_KEY_TYPE_V6UC);
        if (vrf_id_max) { 
            soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
        }
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, entry, V6_0f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_1f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_0f, 0);
        soc_mem_field32_set(unit, mem, entry, IPMC_1f, 0);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
        if (vrf_id_max) { 
            soc_mem_field32_set(unit, mem, entry, VRF_ID_0f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_1f, 1);
        }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
    }
    soc_mem_field32_set(unit, mem, entry, VALID_0f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_1f, 1);
    while (iterations--) {
        ip_tmp[1] = ((dst_ip[8] << 24) | (dst_ip[9] << 16) |
                     (dst_ip[10] << 8) | (dst_ip[11] << 0));
        ip_tmp[0] = ((dst_ip[12] << 24) | (dst_ip[13] << 16) |
                     (dst_ip[14] << 8) | (dst_ip[15] << 0));
        soc_mem_field_set(unit, mem, entry,
                          IP_ADDR_LWR_64f, ip_tmp);

        ip_tmp[1] = ((dst_ip[0] << 24) | (dst_ip[1] << 16) |
                     (dst_ip[2] << 8) | (dst_ip[3] << 0));
        ip_tmp[0] = ((dst_ip[4] << 24) | (dst_ip[5] << 16) |
                     (dst_ip[6] << 8) | (dst_ip[7] << 0));
        soc_mem_field_set(unit, mem, entry,
                          IP_ADDR_UPR_64f, ip_tmp);
#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
            if (vrf_id_max) { 
                soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
            } 
        } else
#endif /* BCM_TRIUMPH_SUPPORT */
        {
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||\
    defined(BCM_SCORPION_SUPPORT)
            if (vrf_id_max) { 
                soc_mem_field32_set(unit, mem,entry, VRF_ID_0f, vrf_id);
                soc_mem_field32_set(unit, mem,entry, VRF_ID_1f, vrf_id);
            } 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
        }
        bucket = soc_fb_l3x2_entry_hash(unit, entry);

        for (i = 0; i < num_ent_per_bucket; i++) {
            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, entry);
                cli_out("into bucket 0x%x\n", bucket);
            }

            memcpy(entry_tmp[i], entry, sizeof(entry));
            if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry_tmp[i])) < 0) {
                if (r == SOC_E_FULL) {
                    /* Already full, stop wasting time */
                    break;
                } else {
                    test_error(unit, "L3 insert failed at bucket %d\n", bucket);
                    rv = -1;
                    goto done;
                }
            }

            /* Increment key by number of buckets in the table */
            increment_ip6addr(dst_ip, 15, num_buckets);
            vid += vid_inc;
            if (vid > DRACO_L3_VID_MAX) {
                vid = 1;
            }
            if (vrf_id_max) { 
                vrf_id += vrf_id_inc;
                if (vrf_id > vrf_id_max) {
                    vrf_id = 0;
                }
            }
            ip_tmp[1] = ((dst_ip[8] << 24) | (dst_ip[9] << 16) |
                         (dst_ip[10] << 8) | (dst_ip[11] << 0));
            ip_tmp[0] = ((dst_ip[12] << 24) | (dst_ip[13] << 16) |
                         (dst_ip[14] << 8) | (dst_ip[15] << 0));
            soc_mem_field_set(unit, mem, entry,
                              IP_ADDR_LWR_64f, ip_tmp);

            ip_tmp[1] = ((dst_ip[0] << 24) | (dst_ip[1] << 16) |
                         (dst_ip[2] << 8) | (dst_ip[3] << 0));
            ip_tmp[0] = ((dst_ip[4] << 24) | (dst_ip[5] << 16) |
                         (dst_ip[6] << 8) | (dst_ip[7] << 0));
            soc_mem_field_set(unit, mem, entry,
                              IP_ADDR_UPR_64f, ip_tmp);

            if (vrf_id_max) { 
#ifdef BCM_TRIUMPH_SUPPORT
                if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
                    soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
                } else
#endif /* BCM_TRIUMPH_SUPPORT */
                {
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_0f, vrf_id);
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_1f, vrf_id);
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
                }
            } 
        }

        if (ad->opt_verbose) {
            cli_out("Inserting %d entry in bucket %d, should fail\n",
                    num_ent_per_bucket, bucket);
        }

        if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry)) < 0) {
            if (r != SOC_E_FULL) {
                test_error(unit, "L3 insert failed\n");
                rv = -1;
                goto done;
            }
        } else {
            test_error(unit, "L3 insert to full bucket succeeded\n");
            rv = -1;
            goto done;
        }

        if (ad->opt_verbose) {
            cli_out("Verifying installed entries\n");
        }

        /* Verify bucket contains added entries */
        for (j = 0; j < i; j++) {
            if (fb_l3_bucket_search(unit, ad, bucket, entry_tmp[j], FALSE, 0) < 0) {
                test_error(unit, "L3 entry missing at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        if (ad->opt_verbose) {
            cli_out("Cleaning bucket %d\n", bucket);
        }

        /* Remove the entries that we added */
        for (j = 0; j < i; j++) {
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry_tmp[j]) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        /* Do Next bucket */
        increment_ip6addr(dst_ip, 15, 1);
        vid += vid_inc;
        if (vid > DRACO_L3_VID_MAX) {
            vid = 1;
        }
        if (vrf_id_max) { 
            vrf_id += vrf_id_inc;
            if (vrf_id > vrf_id_max) {
                vrf_id = 0;
            }
        }
    }

 done:

    return rv;
}

int
_fb_l3ip6mcast_test_ov(int unit, args_t *a, void *p)
{
    uint32                      entry[SOC_MAX_MEM_WORDS];
    soc_mem_t                   mem = L3_ENTRY_IPV6_MULTICASTm;
    int                         num_ent_per_bucket;
    int                         num_vbits;
    int                         num_buckets; 
    ip6_addr_t                  src_ip;
    ip6_addr_t                  dst_ip;
    uint32                      ip_tmp[2];
    int                         src_ip_inc;
    bcm_vlan_t                  vid;
    int                         vid_inc;
    int                         r, rv = 0;
    int                         bucket;
    int                         i, j;
    int                         iterations;
    draco_l3_testdata_t         *ad = p;
    int                         vrf_id;
    int                         vrf_id_inc;
    int                         vrf_id_max = 0;

    COMPILER_REFERENCE(a);
    vrf_id_max = SOC_VRF_MAX(unit);

    num_vbits = 4;
    num_ent_per_bucket = SOC_L3X_BUCKET_SIZE(unit) / num_vbits;
    num_buckets = soc_mem_index_count(unit, mem) / num_ent_per_bucket;
    iterations = (ad->opt_count > num_buckets) ? num_buckets : ad->opt_count;

    if (ad->opt_key_src_ip) {
        memcpy(src_ip, ad->opt_src_ip6, sizeof(ip6_addr_t));
        src_ip_inc = ad->opt_src_ip6_inc;
    } else {
        sal_memset (src_ip, 0, sizeof(src_ip));
        src_ip_inc = 0;
    }
    memcpy(dst_ip, ad->opt_base_ip6, sizeof(ip6_addr_t));
    vid = ad->opt_base_vid;
    vid_inc = ad->opt_vid_inc;
    vrf_id = ad->opt_base_vrf_id;
    vrf_id_inc = ad->opt_vrf_id_inc;

    sal_memset (entry, 0, sizeof(entry));
#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_0f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_1f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_2f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        soc_mem_field32_set(unit, mem, entry, KEY_TYPE_3f,
                            TR_L3_HASH_KEY_TYPE_V6MC);
        if (vrf_id_max) { 
            soc_mem_field32_set(unit, mem, entry, VRF_IDf, 1);
        } 
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, entry, V6_0f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_1f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_2f, 1);
        soc_mem_field32_set(unit, mem, entry, V6_3f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_0f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_1f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_2f, 1);
        soc_mem_field32_set(unit, mem, entry, IPMC_3f, 1);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
        if (vrf_id_max) { 
            soc_mem_field32_set(unit, mem, entry, VRF_ID_0f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_1f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_2f, 1);
            soc_mem_field32_set(unit, mem, entry, VRF_ID_3f, 1);
        } 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
    }
    soc_mem_field32_set(unit, mem, entry, VALID_0f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_1f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_2f, 1);
    soc_mem_field32_set(unit, mem, entry, VALID_3f, 1);
    while (iterations--) {
        dst_ip[0] = 0;
        ip_tmp[1] = ((dst_ip[8] << 24) | (dst_ip[9] << 16) |
                     (dst_ip[10] << 8) | (dst_ip[11] << 0));
        ip_tmp[0] = ((dst_ip[12] << 24) | (dst_ip[13] << 16) |
                     (dst_ip[14] << 8) | (dst_ip[15] << 0));
        soc_mem_field_set(unit, mem, entry,
                          GROUP_IP_ADDR_LWR_64f, ip_tmp);

        ip_tmp[1] = ((dst_ip[0] << 24) | (dst_ip[1] << 16) |
                     (dst_ip[2] << 8) | (dst_ip[3] << 0));
        ip_tmp[0] = ((dst_ip[4] << 24) | (dst_ip[5] << 16) |
                     (dst_ip[6] << 8) | (dst_ip[7] << 0));
        soc_mem_field_set(unit, mem, entry,
                          GROUP_IP_ADDR_UPR_56f, ip_tmp);

        ip_tmp[1] = ((src_ip[8] << 24) | (src_ip[9] << 16) |
                     (src_ip[10] << 8) | (src_ip[11] << 0));
        ip_tmp[0] = ((src_ip[12] << 24) | (src_ip[13] << 16) |
                     (src_ip[14] << 8) | (src_ip[15] << 0));
        soc_mem_field_set(unit, mem, entry,
                          SOURCE_IP_ADDR_LWR_64f, ip_tmp);

        ip_tmp[1] = ((src_ip[0] << 24) | (src_ip[1] << 16) |
                     (src_ip[2] << 8) | (src_ip[3] << 0));
        ip_tmp[0] = ((src_ip[4] << 24) | (src_ip[5] << 16) |
                     (src_ip[6] << 8) | (src_ip[7] << 0));
        soc_mem_field_set(unit, mem, entry,
                          SOURCE_IP_ADDR_UPR_64f, ip_tmp);

#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
            soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
            if (vrf_id_max) { 
                soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
            } 
        } else
#endif /* BCM_TRIUMPH_SUPPORT */
        {
            soc_mem_field32_set(unit, mem, entry, VLAN_ID_0f, vid);
            soc_mem_field32_set(unit, mem, entry, VLAN_ID_1f, vid);
            soc_mem_field32_set(unit, mem, entry, VLAN_ID_2f, vid);
            soc_mem_field32_set(unit, mem, entry, VLAN_ID_3f, vid);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
            if (vrf_id_max) { 
                soc_mem_field32_set(unit, mem,entry, VRF_ID_0f, vrf_id);
                soc_mem_field32_set(unit, mem,entry, VRF_ID_1f, vrf_id);
                soc_mem_field32_set(unit, mem,entry, VRF_ID_2f, vrf_id);
                soc_mem_field32_set(unit, mem,entry, VRF_ID_3f, vrf_id);
            } 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
        }
        bucket = soc_fb_l3x2_entry_hash(unit, entry);

        for (i = 0; i < num_ent_per_bucket; i++) {
            if (ad->opt_verbose) {
                cli_out("Inserting ");
                soc_mem_entry_dump(unit, mem, entry);
                cli_out("into bucket 0x%x\n", bucket);
            }

            memcpy(entry_tmp[i], entry, sizeof(entry));
            if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry_tmp[i])) < 0) {
                if (r == SOC_E_FULL) {
                    /* Already full, stop wasting time */
                    break;
                } else {
                    test_error(unit, "L3 insert failed at bucket %d\n", bucket);
                    rv = -1;
                    goto done;
                }
            }

            /* Increment key by number of buckets in the table */
            increment_ip6addr(dst_ip, 15, num_buckets);
            increment_ip6addr(src_ip, 15, src_ip_inc);
            vid += vid_inc;
            if (vid > DRACO_L3_VID_MAX) {
                vid = 1;
            }
            if (vrf_id_max) { 
                vrf_id += vrf_id_inc;
                if (vrf_id > vrf_id_max) {
                    vrf_id = 0;
                }
            }  
            dst_ip[0] = 0;
            ip_tmp[1] = ((dst_ip[8] << 24) | (dst_ip[9] << 16) |
                         (dst_ip[10] << 8) | (dst_ip[11] << 0));
            ip_tmp[0] = ((dst_ip[12] << 24) | (dst_ip[13] << 16) |
                         (dst_ip[14] << 8) | (dst_ip[15] << 0));
            soc_mem_field_set(unit, mem, entry,
                              GROUP_IP_ADDR_LWR_64f, ip_tmp);

            ip_tmp[1] = ((dst_ip[0] << 24) | (dst_ip[1] << 16) |
                         (dst_ip[2] << 8) | (dst_ip[3] << 0));
            ip_tmp[0] = ((dst_ip[4] << 24) | (dst_ip[5] << 16) |
                         (dst_ip[6] << 8) | (dst_ip[7] << 0));
            soc_mem_field_set(unit, mem, entry,
                              GROUP_IP_ADDR_UPR_56f, ip_tmp);

            ip_tmp[1] = ((src_ip[8] << 24) | (src_ip[9] << 16) |
                         (src_ip[10] << 8) | (src_ip[11] << 0));
            ip_tmp[0] = ((src_ip[12] << 24) | (src_ip[13] << 16) |
                         (src_ip[14] << 8) | (src_ip[15] << 0));
            soc_mem_field_set(unit, mem, entry,
                              SOURCE_IP_ADDR_LWR_64f, ip_tmp);

            ip_tmp[1] = ((src_ip[0] << 24) | (src_ip[1] << 16) |
                         (src_ip[2] << 8) | (src_ip[3] << 0));
            ip_tmp[0] = ((src_ip[4] << 24) | (src_ip[5] << 16) |
                         (src_ip[6] << 8) | (src_ip[7] << 0));
            soc_mem_field_set(unit, mem, entry,
                              SOURCE_IP_ADDR_UPR_64f, ip_tmp);
#ifdef BCM_TRIUMPH_SUPPORT
            if (soc_feature(unit, soc_feature_l3_entry_key_type)) {
                soc_mem_field32_set(unit, mem, entry, VLAN_IDf, vid);
                if (vrf_id_max) { 
                    soc_mem_field32_set(unit, mem,entry, VRF_IDf, vrf_id);
                }
            } else 
#endif /* BCM_TRIUMPH_SUPPORT */
            {
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_0f, vid);
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_1f, vid);
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_2f, vid);
                soc_mem_field32_set(unit, mem, entry, VLAN_ID_3f, vid);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
                if (vrf_id_max) { 
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_0f, vrf_id);
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_1f, vrf_id);
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_2f, vrf_id);
                    soc_mem_field32_set(unit, mem,entry, VRF_ID_3f, vrf_id);
                } 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
            }
        }

        if (ad->opt_verbose) {
            cli_out("Inserting %d entry in bucket %d, should fail\n",
                    num_ent_per_bucket, bucket);
        }
        if ((r = soc_mem_insert(unit, mem, COPYNO_ALL, entry)) < 0) {
            if (r != SOC_E_FULL) {
                test_error(unit, "L3 insert failed\n");
                rv = -1;
                goto done;
            }
        } else {
            test_error(unit, "L3 insert to full bucket succeeded\n");
            rv = -1;
            goto done;
        }

        if (ad->opt_verbose) {
            cli_out("Verifying installed entries\n");
        }

        /* Verify bucket contains added entries */
        for (j = 0; j < i; j++) {
            if (fb_l3_bucket_search(unit, ad, bucket, entry_tmp[j], FALSE, 0) < 0) {
                test_error(unit, "L3 entry missing at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        if (ad->opt_verbose) {
            cli_out("Cleaning bucket %d\n", bucket);
        }

        /* Remove the entries that we added */
        for (j = 0; j < i; j++) {
            if (soc_mem_delete(unit, mem, COPYNO_ALL, entry_tmp[j]) < 0) {
                test_error(unit, "L3 delete failed at bucket %d\n", bucket);
                rv = -1;
                goto done;
            }
        }

        /* Do Next bucket */
        increment_ip6addr(dst_ip, 15, 1);
        increment_ip6addr(src_ip, 15, src_ip_inc);
        vid += vid_inc;
        if (vid > DRACO_L3_VID_MAX) {
            vid = 1;
        }
        if (vrf_id_max) { 
            vrf_id += vrf_id_inc;
            if (vrf_id > vrf_id_max) {
                vrf_id = 0;
            }
        }
    }

 done:

    return rv;
}

#endif

/*
 * Test of L3 hashing
 *
 *   This test tries a number of keys against one of the hashing functions,
 *   checking a software hash against the hardware hash, then searching the
 *   bucket to find the entry after inserting.
 *
 */
int
fb_l3_test_hash(int unit, args_t *a, void *p)
{
    draco_l3_testdata_t         *ad = p;
    int                         ipv6_enable = ad->ipv6;
#ifdef BCM_FIREBOLT_SUPPORT
    int                         ipmc_enable = ad->opt_ipmc_enable;
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
    if (ipv6_enable) {
        if (ipmc_enable) {
            return (_fb_l3ip6mcast_test_hash(unit, a, p));
        } else {
            return (_fb_l3ip6ucast_test_hash(unit, a, p));
        }
    } else {
        if (ipmc_enable) {
            return (_fb_l3ip4mcast_test_hash(unit, a, p));
        } else {
            return (_fb_l3ip4ucast_test_hash(unit, a, p));
        }
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    return SOC_E_NONE;
}

/*
 * Test of L3 overflow behavior
 *
 *   This test fills each bucket of the L3, then inserts another entry to see
 *   that the last entry fails to insert.
 *
 */

int
fb_l3_test_ov(int unit, args_t *a, void *p)
{
    draco_l3_testdata_t         *ad = p;
    int                         ipv6_enable = ad->ipv6;
    uint32                      hash = ad->opt_hash;
#ifdef BCM_FIREBOLT_SUPPORT
    int                         ipmc_enable = ad->opt_ipmc_enable;
#endif /* BCM_FIREBOLT_SUPPORT */

    if (hash != FB_HASH_LSB) {
        if (ad->opt_verbose) {
            cli_out("Resetting hash selection to LSB\n");
        }

        hash = ad->save_hash_control;
        soc_reg_field_set(unit, HASH_CONTROLr, &hash,
                          L3_HASH_SELECTf, FB_HASH_LSB);

        if (WRITE_HASH_CONTROLr(unit, hash) < 0) {
            test_error(unit, "Hash select setting failed\n");
            return(-1);
        }

        ad->opt_hash = hash = FB_HASH_LSB;
    }

#ifdef BCM_FIREBOLT_SUPPORT
    if (ipv6_enable) {
        if (ipmc_enable) {
            return (_fb_l3ip6mcast_test_ov(unit, a, p));
        } else {
            return (_fb_l3ip6ucast_test_ov(unit, a, p));
        }
    } else {
        if (ipmc_enable) {
            return (_fb_l3ip4mcast_test_ov(unit, a, p));
        } else {
            return (_fb_l3ip4ucast_test_ov(unit, a, p));
        }
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    return SOC_E_NONE;
}

int
fb_l3_test_done(int unit, void *p)
{
    draco_l3_testdata_t *ad = p;
    soc_mem_t tmem = INVALIDm;

    if (ad == NULL) {
	return 0;
    }

    if (SOC_IS_FBX(unit)) {
        tmem = L3_ENTRY_IPV4_UNICASTm;
    } else {
        return (-1);
    }

    /* Check if empty at the end of the test */
    if (ad->opt_reset) {
        int rv, ix;
        int index_min = soc_mem_index_min(unit, tmem);
        int index_max = soc_mem_index_max(unit, tmem);
        uint32 *buf = 0;
        uint32 *ent = 0;
        uint32 count;

        buf = soc_cm_salloc(unit,
                            SOC_MEM_TABLE_BYTES(unit, tmem),
                            "fb_l3_test");
        if (!buf) {
            test_error(unit, "Memory allocation failed\n");
            return (-1);
        }

        if ((rv = soc_mem_read_range(unit, tmem,
                        MEM_BLOCK_ANY, index_min, index_max, buf)) < 0) {
            test_error(unit, "Memory DMA of L3 Entry failed\n");
            return (-1);
        }

        count = soc_mem_index_count(unit, tmem);
        for (ix = 0; ix < count; ix++) {
            ent = soc_mem_table_idx_to_pointer(unit, tmem, uint32 *, buf, ix);
            if (soc_mem_field32_get(unit, tmem, ent, VALIDf)) {
                test_error(unit, "L3 table not empty after test ent = %d\n",
                                ix);
                soc_mem_entry_dump(unit, tmem, ent);
                return (-1);
            }
        }

        soc_cm_sfree(unit, buf);
    }

    if (WRITE_HASH_CONTROLr(unit, ad->save_hash_control) < 0) {
        test_error(unit, "Hash select restore failed\n");
    }
    if (soc_feature(unit, soc_feature_dual_hash)) {
        if (ad->save_dual_hash_control != ad->opt_dual_hash) {
            if (WRITE_L3_AUX_HASH_CONTROLr(unit,
                                           ad->save_dual_hash_control) < 0) {
                test_error(unit, "Dual Hash select restore failed\n");
            }
        }
    }

    return 0;
}
#endif

#endif /* INCLUDE_L3 */

#endif /* BCM_XGS_SWITCH_SUPPORT */
#else
/*ISO C forbids an empty source file */
#include <bcm/types.h>
#endif /* BCM_ESW_SUPPORT */

