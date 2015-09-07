/*
 * $Id: mem_table_test.h,v 1.4 Broadcom SDK $
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
 * Generic Hash Memory Test definitions.
 *
 */

#ifndef __TEST_TABLE_GENERIC__H
#define __TEST_TABLE_GENERIC__H

#ifdef BCM_XGS_SWITCH_SUPPORT
#ifdef BCM_ISM_SUPPORT

/*
 * Generic Hashing Test Data Structure
 */
typedef struct test_generic_hash_testdata_s {
    int unit;
    char *mem_str;
    soc_mem_t mem;
    int copyno;
    
    int opt_count; /* entries for hash, buckets for overflow */
    int opt_verbose;
    int opt_reset;
    
    char *opt_key_base;
    int opt_key_incr;    
    int32 opt_banks;
    char *opt_hash_offsets;
    uint8 offset_count;
    uint8 offsets[_SOC_ISM_MAX_BANKS];
    uint32 restore;
    uint8 config_banks[_SOC_ISM_MAX_BANKS];
    uint8 cur_offset[_SOC_ISM_MAX_BANKS];
} test_generic_hash_testdata_t;

typedef struct test_generic_hash_test_s {
    uint8 setup_done;
    test_generic_hash_testdata_t testdata;
    test_generic_hash_testdata_t *ctp;
    int unit;
} test_generic_hash_test_t;

#endif /* BCM_ISM_SUPPORT */
#endif /* BCM_XGS_SWITCH_SUPPORT */

#endif /*!__TEST_TABLE_GENERIC__H */
