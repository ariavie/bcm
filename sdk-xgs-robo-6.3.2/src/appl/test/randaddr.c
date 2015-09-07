/*
 * $Id: randaddr.c 1.67 Broadcom SDK $
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
 * Random index memory test implemented via S-Channel table read/write.
 *
 * This test function is a variation on memrand for random memory
 * testing.  It was requested by Govind to do the following:
 *
 * Write every word in the memory each pass.
 * Write all then read all
 * Vary the pattern each write, for example, from all 1s to all 0s.
 * For different seeds, to order of writes to the memory should vary.
 *
 * Randomizing algorithm:
 *   Inputs: current value, original seed.
 *
 */

#include <sal/types.h>

#include <sal/appl/pci.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/progress.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/l2x.h>
#include <soc/memory.h>

#if defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT)
#include <soc/dpp/drv.h>
#include <soc/dfe/cmn/dfe_drv.h>
#endif /*defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT)*/

#include "testlist.h"

#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_POLAR_SUPPORT)
/*
 * Return true if a and b are relatively prime
 * Bad O(n) algorithm
 */
static uint32
relprime(uint32 a, uint32 b)
{
    uint32 i;
    uint32 min = a < b ? a : b;
    uint32 max = a < b ? b : a;

    if (max % min == 0) {
        return 0;
    }

    if ((a % 2 == 0) && (b % 2 == 0)) {
        return 0;
    }

    for (i = 3; i <= min / 2; i += 2) {
        if ((a % i == 0) && (b % i == 0)) {
            return 0;
        }
    }

    return 1;
}

/*
 * Return the first value greater than or equal to start that is
 * relatively prime to val.
 */
static uint32
get_rel_prime(uint32 val, uint32 start)
{
    while (!relprime(val, start)) {
        start++;
    }

    return start;
}


/*
 * Memory test status structure
 */

typedef struct rand_work_s {
    int was_debug_mode;
    int orig_enable;
    int saved_tcam_protect_write;
    int mem_scan;
    int scan_rate;
    sal_usecs_t scan_interval;
    int iters;                          /* Total iterations to perform */

    soc_mem_t mem;                      /* Memory to test */

    int copyno;                         /* copy to test (or COPYNO_ALL) */
    int copyno_total;                   /* Number of copies */

    int index_min;                      /* Lowest index in range to test */
    int index_max;                      /* Last index in range to test */
    int index_total;                    /* Size of index range (max-min+1) */
    int array_index_start;              /* Lowest array index in range to test */
    int array_index_end;                /* Last array index in range to test */

    uint32 data_start;                  /* initial data value */
    uint32 data;                        /* current data value */
    uint32 seed;                        /* seed */
    int ecc_as_data;                    /* treat ecc field as regular field */
    uint32 addr_incr;                   /* for internal random */
    uint32 first_addr;
    uint32 cur_addr;
} rand_work_t;

static rand_work_t      *rand_work[SOC_MAX_NUM_DEVICES];

#ifdef BCM_POLAR_SUPPORT
static int
_soc_mem_id_map(int unit, soc_mem_t mem, int *drv_mem){

    int table_name;
    
    switch(mem) {
        case GEN_MEMORYm:
            table_name = DRV_MEM_GEN;
            break;
        case L2_ARLm:
        case L2_MARLm:
        case L2_ARL_SWm:
        case L2_MARL_SWm:
            table_name = DRV_MEM_ARL_HW;
            break;
        case MARL_PBMPm:
            table_name = DRV_MEM_MCAST;
            break;
        case MSPT_TABm:
            table_name = DRV_MEM_MSTP;
            break;
        case VLAN_1Qm:
            table_name = DRV_MEM_VLAN;
            break;
        case VLAN2VLANm:
            table_name = DRV_MEM_VLANVLAN;
            break;
        case MAC2VLANm:
            table_name = DRV_MEM_MACVLAN;
            break;
        case PROTOCOL2VLANm:
            table_name = DRV_MEM_PROTOCOLVLAN;
            break;
        case FLOW2VLANm:
            table_name = DRV_MEM_FLOWVLAN;
            break;
        case CFP_TCAM_S0m:
        case CFP_TCAM_S1m:
        case CFP_TCAM_S2m:
        case CFP_TCAM_IPV4_SCm:
        case CFP_TCAM_IPV6_SCm:
        case CFP_TCAM_NONIP_SCm:
        case CFP_TCAM_CHAIN_SCm:
            table_name = DRV_MEM_TCAM_DATA;
            break;
        case CFP_TCAM_MASKm:
        case CFP_TCAM_IPV4_MASKm:
        case CFP_TCAM_IPV6_MASKm:
        case CFP_TCAM_NONIP_MASKm:
        case CFP_TCAM_CHAIN_MASKm:
            table_name = DRV_MEM_TCAM_MASK;
            break;
        case CFP_ACT_POLm:
            table_name = DRV_MEM_CFP_ACT;
            break;
        case CFP_METERm:
            table_name = DRV_MEM_CFP_METER;
            break;
        case CFP_STAT_IBm:
            table_name = DRV_MEM_CFP_STAT_IB;
            break;
        case CFP_STAT_OBm:
            table_name = DRV_MEM_CFP_STAT_OB;
            break;
        case EGRESS_VID_REMARKm:
            table_name = DRV_MEM_EGRVID_REMARK;
            break;
        default:
            printk("Unsupport memory table.\n");
            return -1;
    }
    *drv_mem = table_name;
    return SOC_E_NONE;
}
#endif /* BCM_POLAR_SUPPORT */

#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
STATIC int
addr_rand_common_clear(int unit, soc_mem_t mem, int copyno)
{
#ifdef INCLUDE_MEM_SCAN
    rand_work_t *rw = rand_work[unit];
#endif /* INCLUDE_MEM_SCAN */
    int rv;
    
    SOC_MEM_TEST_SKIP_CACHE_SET(unit, 1);
    if ((rv = soc_mem_parity_control(unit, mem, copyno, FALSE)) < 0) {
        test_error(unit, "Could not disable parity warnings on memory %s\n",
                   SOC_MEM_UFNAME(unit, mem));
        return -1;
    }
#ifdef INCLUDE_MEM_SCAN
    if ((rw->mem_scan = soc_mem_scan_running(unit, &rw->scan_rate,
                                             &rw->scan_interval)) < 0) {
        if (soc_mem_scan_stop(unit)) {
            return -1;
        }
    }
#endif /* INCLUDE_MEM_SCAN */
    return 0;
}

STATIC int
addr_rand_common_restore(int unit, soc_mem_t mem, int copyno)
{
#ifdef INCLUDE_MEM_SCAN
    rand_work_t *rw = rand_work[unit];
#endif /* INCLUDE_MEM_SCAN */

    SOC_MEM_TEST_SKIP_CACHE_SET(unit, 0);
    if (soc_mem_parity_restore(unit, mem, copyno) < 0) {
        test_error(unit, "Could not enable parity warnings on memory %s\n",
                   SOC_MEM_UFNAME(unit, mem));
        return -1;
    }
#ifdef INCLUDE_MEM_SCAN
    if (rw->mem_scan) {
        if (soc_mem_scan_start(unit, rw->scan_rate, rw->scan_interval)) {
            return -1;
        }
    }
#endif /* INCLUDE_MEM_SCAN */
    return 0;
}
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */

int
addr_rand_init(int unit, args_t *a, void **p)
{
    rand_work_t         *rw;
    char                *mem_name;
    char                *idx_start_str, *idx_end_str;
    char                *array_idx_start_str, *array_idx_end_str;
    int                 blk;
    int                 rv = -1;
    parse_table_t       pt;
#ifdef BCM_POLAR_SUPPORT
    uint32 cache_enable;
#endif /* BCM_POLAR_SUPPORT */

    rw = rand_work[unit];
    if (rw == NULL) {
        rw = sal_alloc(sizeof(rand_work_t), "randaddr");
        if (rw == NULL) {
            printk("%s: cannot allocate memory test data\n", ARG_CMD(a));
            return -1;
        }
        sal_memset(rw, 0, sizeof(rand_work_t));
        rand_work[unit] = rw;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt,  "Memory",     PQ_STRING, "",
                    &mem_name, NULL);
    parse_table_add(&pt,  "IndexStart", PQ_STRING, (void *) "min",
                    &idx_start_str, NULL);
    parse_table_add(&pt,  "IndexEnd",   PQ_STRING, (void *) "max",
                    &idx_end_str, NULL);
    parse_table_add(&pt,  "ArrayIndexStart", PQ_STRING, (void *) "min", &array_idx_start_str, NULL);
    parse_table_add(&pt,  "ArrayIndexEnd",   PQ_STRING, (void *) "max", &array_idx_end_str, NULL);
    parse_table_add(&pt,  "ITERations", PQ_INT, (void *) 10,
                    &rw->iters, NULL);
    parse_table_add(&pt,  "SEED",       PQ_INT, (void *) 0xdecade,
                    &rw->seed, NULL );
    parse_table_add(&pt,  "EccAsData",  PQ_BOOL, 0,
                    &rw->ecc_as_data, NULL );
    parse_table_add(&pt,  "InitialData", PQ_INT, (void *) 0xffffffff,
                    &rw->data_start, NULL );

    if (parse_arg_eq(a, &pt) < 0) {
        printk("%s: Invalid option: %s\n",
               ARG_CMD(a), ARG_CUR(a));
        goto done;
    }

    if (ARG_CNT(a) != 0) {
        printk("%s: extra options starting with \"%s\"\n",
               ARG_CMD(a), ARG_CUR(a));
        goto done;
    }

    if (mem_name == 0 || *mem_name == 0 ||
        parse_memory_name(unit,
                          &rw->mem,
                          mem_name,
                          &rw->copyno,
                          0) < 0) {
        test_error(unit,
                   "Missing or unknown memory name "
                   "(use listmem for list)\n");
        goto done;
    }

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        if (!soc_mem_is_valid(unit, rw->mem) ||
            soc_mem_is_readonly(unit, rw->mem) ||
            soc_robo_mem_index_max(unit, rw->mem) < 3) {
            test_error(unit,
                       "Cannot test memory %s with this command\n",
                       SOC_ROBO_MEM_UFNAME(unit, rw->mem));
            goto done;
        }
    } else
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (!soc_mem_is_valid(unit, rw->mem) ||
            soc_mem_is_readonly(unit, rw->mem) ||
            soc_mem_index_max(unit, rw->mem) < 3) {
            test_error(unit,
                       "Cannot test memory %s with this command\n",
                       SOC_MEM_UFNAME(unit, rw->mem));
            goto done;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
    }
#if defined(BCM_DFE_SUPPORT) || defined(BCM_DPP_SUPPORT)
    if (SOC_IS_SAND(unit) && (soc_mem_is_writeonly(unit, rw->mem) || soc_mem_is_signal(unit, rw->mem))) {
        test_error(unit, "Memory %s is writeonly/signal \n",
                   SOC_MEM_UFNAME(unit, rw->mem));
        goto done;
    }
#endif
#ifdef BCM_88650_A0
    if(SOC_IS_ARAD(unit)) {
        switch(rw->mem) {
            case BRDC_FSRD_FSRD_WL_EXT_MEMm:
            case IQM_MEM_8000000m:
            case NBI_TBINS_MEMm:
            case NBI_RBINS_MEMm:
            case PORT_WC_UCMEM_DATAm:
            case ECI_MEM_00010000m:
            case EGQ_CBMm:
            case EGQ_RDMUCm:

               test_error(unit, "Memory %s is invalid/readonly/writeonly/signal \n",
                       SOC_MEM_UFNAME(unit, rw->mem));

               goto done;
            default:
                break;
        }
    }
#endif

#ifdef BCM_88750_A0
    /*Filter dynamic memories*/
    if(SOC_IS_FE1600(unit)) {
        switch(rw->mem) {
        case RTP_RMHMTm:
        case RTP_CUCTm:
        case RTP_DUCTPm:
        case RTP_DUCTSm:
        case RTP_MEM_800000m:
        case RTP_MEM_900000m:
               test_error(unit, "Memory %s is invalid/readonly/writeonly/signal \n",
                       SOC_MEM_UFNAME(unit, rw->mem));
               goto done;
            default:
                break;
        }
    }
#endif

    if (rw->copyno == COPYNO_ALL) {
        rw->copyno_total = 0;
        SOC_MEM_BLOCK_ITER(unit, rw->mem, blk) {
            rw->copyno_total += 1;
        }
    } else {
        rw->copyno_total = 1;
        if (!SOC_MEM_BLOCK_VALID(unit, rw->mem, rw->copyno)) {
#ifdef BCM_POLAR_SUPPORT
            if (SOC_IS_POLAR(unit)) {
                test_error(unit,
                           "Copy number out of range for memory %s\n",
                           SOC_ROBO_MEM_UFNAME(unit, rw->mem));
            } else
#endif /* BCM_POLAR_SUPPORT */
            {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                test_error(unit,
                           "Copy number out of range for memory %s\n",
                           SOC_MEM_UFNAME(unit, rw->mem));
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
            }
            goto done;
        }
    }

    rw->index_min = parse_memory_index(unit, rw->mem, idx_start_str);
    rw->index_max = parse_memory_index(unit, rw->mem, idx_end_str);
    rw->array_index_start = parse_memory_array_index(unit, rw->mem, array_idx_start_str);
    rw->array_index_end = parse_memory_array_index(unit, rw->mem, array_idx_end_str);
    if (rw->array_index_start > rw->array_index_end ) {
        unsigned temp_ai = rw->array_index_start;
        rw->array_index_start = rw->array_index_end;
        rw->array_index_end = temp_ai;
        printk("WARNING: switching start and end array indices to %u-%u\n", rw->array_index_start, rw->array_index_end);
    }
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FB_FX_HX(unit)) {
        if (rw->index_min < soc_mem_index_min(unit, rw->mem)) {
            rw->index_min = soc_mem_index_min(unit, rw->mem);
        }
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    rw->index_total = rw->index_max - rw->index_min + 1;

    if (rw->index_total <= 1 || rw->copyno_total < 1) {
        test_error(unit,
                   "Min copyno/index must be less than max copyno/index\n");
        goto done;
    }

    /* Place MMU in debug mode if testing a CBP memory */
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    if (soc_mem_is_debug(unit, rw->mem) &&
        (rw->was_debug_mode = soc_mem_debug_set(unit, 0)) < 0) {
        test_error(unit, "Could not put MMU in debug mode\n");
        goto done;
    }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */

    /* Disable non-atomic TCAM write handling */
    rw->saved_tcam_protect_write = SOC_CONTROL(unit)->tcam_protect_write;
    SOC_CONTROL(unit)->tcam_protect_write = FALSE;

#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_DFE(unit)) {
        SOC_MEM_TEST_SKIP_CACHE_SET(unit, 1);
        /* Disable any parity control (herc) */
        if (soc_mem_parity_control(unit, rw->mem, rw->copyno, FALSE) < 0) {
            test_error(unit, "Could not disable parity warnings on memory %s\n",
                   SOC_MEM_UFNAME(unit, rw->mem));
            goto done;
        }
    } else 
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        /* Do nothing for parity control */
    } else
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (addr_rand_common_clear(unit, rw->mem, rw->copyno)) {
            goto done;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
    }

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        DRV_MEM_CACHE_GET(unit, rw->mem, &cache_enable);
        if (cache_enable) {
            printk("WARNING: Caching is enabled on memory %s\n",
                   SOC_ROBO_MEM_UFNAME(unit, rw->mem));
        }
    } else
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (soc_mem_cache_get(unit, rw->mem,
                              rw->copyno == COPYNO_ALL ?
                              MEM_BLOCK_ALL : rw->copyno)) {
            printk("WARNING: Caching is enabled on memory %s.%s\n",
                   SOC_MEM_UFNAME(unit, rw->mem),
                   SOC_BLOCK_NAME(unit, rw->copyno));
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
    }

#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    if (soc_mem_cpu_write_control(unit, rw->mem, rw->copyno, TRUE,
                                  &rw->orig_enable) < 0) {
        test_error(unit, "Could not enable exclusive cpu write on memory %s\n",
                   SOC_MEM_UFNAME(unit, rw->mem));
        goto done;
    }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */

    /*
     * Turn off L2 task to keep it from going crazy if L2 memory is
     * being tested.
     */

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_arl_hashed)) {
        (void)soc_l2x_stop(unit);
    }
#endif /* BCM_XGS_SWITCH_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit) && (rw->mem == EGR_PERQ_XMT_COUNTERSm)) {
        rv = soc_reg_field32_modify(unit, EGR_EDB_HW_CONTROLr, REG_PORT_ANY,
                                    XGS_COUNTER_COMPAT_MODEf, 1);
       if (rv != SOC_E_NONE) {
           goto done;
       }
    }
    if (SOC_IS_SHADOW(unit) && SOC_BLOCK_IS_CMP(unit,
        SOC_MEM_BLOCK_MIN(unit, rw->mem), SOC_BLK_MS_ISEC)) {
        rv = soc_reg_field32_modify(unit, ISEC_MASTER_CTRLr, 1,
                                    XGS_COUNTER_COMPAT_MODEf, 1);
        if (rv != SOC_E_NONE) {
            return -1;
       }
        rv = soc_reg_field32_modify(unit, ISEC_MASTER_CTRLr, 5,
                                    XGS_COUNTER_COMPAT_MODEf, 1);
      if (rv != SOC_E_NONE) {
           return -1;
       }
    }
    if (SOC_IS_SHADOW(unit) && SOC_BLOCK_IS_CMP(unit,
        SOC_MEM_BLOCK_MIN(unit, rw->mem), SOC_BLK_MS_ESEC)) {
        rv = soc_reg_field32_modify(unit, ESEC_MASTER_CTRLr, 1,
                                    XGS_COUNTER_COMPAT_MODEf, 1);
        if (rv != SOC_E_NONE) {
            return -1;
        }
        rv = soc_reg_field32_modify(unit, ESEC_MASTER_CTRLr, 5,
                                    XGS_COUNTER_COMPAT_MODEf, 1);
        if (rv != SOC_E_NONE) {
           return -1;
        }
    }
#endif

    /*
     * Turn off FP.
     * Required when testing FP and METERING memories or tests will fail.
     * We don't care about the return value as the BCM layer may not have
     * been initialized at all.
     */
    (void)bcm_field_detach(unit);

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_esm_support)) {
        /* Some tables have dependency on the content of other table */
        switch (rw->mem) {
        case EXT_ACL360_TCAM_DATAm:
        case EXT_ACL360_TCAM_DATA_IPV6_SHORTm:
            rv = soc_mem_clear(unit, EXT_ACL360_TCAM_MASKm, 
                               MEM_BLOCK_ALL, TRUE);
            if (rv < 0) {
                test_error(unit, "Could not clear EXT_ACL360_TCAM_MASK\n");
                goto done;
            }
            break;
        case EXT_ACL432_TCAM_DATAm:
        case EXT_ACL432_TCAM_DATA_IPV6_LONGm:
        case EXT_ACL432_TCAM_DATA_L2_IPV4m:
        case EXT_ACL432_TCAM_DATA_L2_IPV6m:
            rv = soc_mem_clear(unit, EXT_ACL432_TCAM_MASKm, 
                               MEM_BLOCK_ALL, TRUE);
            if (rv < 0) {
                test_error(unit, "Could not clear EXT_ACL432_TCAM_MASK\n");
                goto done;
            }
            break;
        default:
            break;
        }
    }
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_etu_support)) {
        /* Some tables have dependency on the content of other table */
        switch (rw->mem) {
        case EXT_ACL480_TCAM_DATAm:
            if (SOC_FAILURE(soc_mem_clear(unit, EXT_ACL480_TCAM_MASKm,
                MEM_BLOCK_ALL, TRUE)))
            {
                test_error(unit, "Failed to clear EXT_ACL360_TCAM_MASKm\n");
                parse_arg_eq_done(&pt);
                return -1;
            }
            break;
        default:
            break;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    /* Choose a value for increment that is
       relatively prime to total index. */
    rw->addr_incr = get_rel_prime(rw->index_total, rw->seed *
                                  rw->index_total);
    rw->first_addr = (rw->addr_incr % rw->index_total) + rw->index_min;
    soc_cm_debug(DK_TESTS, "Running with simple seed.  Incr: %d. First 0x%x.\n",
                 rw->addr_incr, rw->first_addr);

    *p = rw;

    rv = 0;

 done:
    parse_arg_eq_done(&pt);
    return rv;
}

#define NEXT_ADDR(cur) \
  ((((cur) - rw->index_min + rw->addr_incr) % rw->index_total) + rw->index_min)


int
addr_rand(int unit, args_t *a, void *p)
{
    rand_work_t         *rw = p;
    uint32              mask[SOC_MAX_MEM_WORDS];
    uint32              tcammask[SOC_MAX_MEM_WORDS];
    uint32              eccmask[SOC_MAX_MEM_WORDS];
    uint32              forcemask[SOC_MAX_MEM_WORDS];
    uint32              forcedata[SOC_MAX_MEM_WORDS];
    uint32              accum_tcammask, accum_forcemask;
    int                 iter, i, dw, word, copyno;
    unsigned            array_index;
    uint32              data[SOC_MAX_MEM_WORDS];
    char                status[160];
    int                 count;
    uint32              miscomp;

    COMPILER_REFERENCE(a);

    dw = soc_mem_entry_words(unit, rw->mem);
#if defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    if(SOC_IS_DPP(unit) || SOC_IS_DFE(unit)) {
        soc_mem_datamask_rw_get(unit, rw->mem, mask);
    } else 
#endif
    {
        soc_mem_datamask_get(unit, rw->mem, mask);
    }
    soc_mem_tcammask_get(unit, rw->mem, tcammask);
    soc_mem_eccmask_get(unit, rw->mem, eccmask);
    soc_mem_forcedata_get(unit, rw->mem, forcemask, forcedata);
    accum_tcammask = 0;
    for (i = 0; i < dw; i++) {
        accum_tcammask |= tcammask[i];
    }
    accum_forcemask = 0;
    for (i = 0; i < dw; i++) {
        accum_forcemask |= forcemask[i];
    }
    if (!rw->ecc_as_data) {
        for (i = 0; i < dw; i++) {
            mask[i] &= ~eccmask[i];
        }
    }
    soc_mem_datamask_memtest(unit, rw->mem, mask);

    progress_init(rw->copyno_total * rw->iters * rw->index_total * (rw->array_index_end - rw->array_index_start + 1) * 2, 3, 0);

    SOC_MEM_BLOCK_ITER(unit, rw->mem, copyno) {
        if (rw->copyno != COPYNO_ALL && rw->copyno != copyno) {
            continue;
        }

        for (array_index = rw->array_index_start; array_index <= rw->array_index_end; ++array_index) {

            if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
#ifdef BCM_POLAR_SUPPORT
                if (SOC_IS_POLAR(unit)) {
                    sal_sprintf(status,
                                "Running %d iterations on %s[%d-%d]",
                                rw->iters,
                                SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                rw->index_min, rw->index_max);
                } else
#endif /* BCM_POLAR_SUPPORT */
                {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                    sal_sprintf(status,
                                "Running %d iterations on %s[%u].%s[%d-%d]",
                                rw->iters,
                                SOC_MEM_UFNAME(unit, rw->mem),
                                array_index,
                                SOC_BLOCK_NAME(unit, copyno),
                                rw->index_min, rw->index_max);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                }
            } else {
#ifdef BCM_POLAR_SUPPORT
                if (SOC_IS_POLAR(unit)) {
                    sal_sprintf(status,
                                "Running %d iterations on %s[%d-%d]",
                                rw->iters,
                                SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                rw->index_min, rw->index_max);
                } else
#endif /* BCM_POLAR_SUPPORT */
                {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                    sal_sprintf(status,
                                "Running %d iterations on %s.%s[%d-%d]",
                                rw->iters,
                                SOC_MEM_UFNAME(unit, rw->mem),
                                SOC_BLOCK_NAME(unit, copyno),
                                rw->index_min, rw->index_max);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                }
            }

            progress_status(status);

            for (iter = 0; iter < rw->iters; iter++) {
                /* Set up addr and data and write to all memory addresses */

                rw->cur_addr = rw->first_addr;
                rw->data = rw->data_start;

                count = 0;

                do {
                    count++;

                    if (soc_mem_entry_words(unit, rw->mem) % 2 == 0) {
                        rw->data = ~rw->data;
                    }
                    for (word = 0; word < dw; word++) {
                        data[word] = rw->data & mask[word];
                        rw->data = ~rw->data;
                    }
                    if (accum_tcammask) {
                        /* data read back has dependency on mask */
                        if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
                           (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU)) {
                            for (word = 0; word < dw; word++) {
                                data[word] &= ~tcammask[word];
                            }
                        } else if (soc_feature(unit, soc_feature_xy_tcam)) {
                            for (word = 0; word < dw; word++) {
                                data[word] |= tcammask[word];
                            }
                        }
                    }
                    if (accum_forcemask) {
                        for (word = 0; word < dw; word++) {
                            data[word] &= ~forcemask[word];
                            data[word] |= forcedata[word];
                        }
                    }

#ifdef BCM_POLAR_SUPPORT
                    if (SOC_IS_POLAR(unit)) {
                        int table_name;
                        if(_soc_mem_id_map(unit, rw->mem, &table_name) == SOC_E_NONE) {
                            if (DRV_MEM_WRITE(unit, table_name, rw->cur_addr, 1, data) < 0) {
                                printk("Write ERROR: table %s[%d] iteration %d\n",
                                       SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                       rw->cur_addr, iter);
                                if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                    printk("At array index %u.\n", array_index);
                                }
                                goto break_all;
                            }
                        }
                    } else
#endif /* BCM_POLAR_SUPPORT */
                    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                        if (soc_mem_array_write(unit, rw->mem, array_index, copyno,
                                          rw->cur_addr, data) < 0) {
                            printk("Write ERROR: table %s.%s[%d] iteration %d\n",
                                   SOC_MEM_UFNAME(unit, rw->mem),
                                   SOC_BLOCK_NAME(unit, copyno),
                                   rw->cur_addr, iter);
                            if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                printk("At array index %u.\n", array_index);
                            }
                            goto break_all;
                        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                    }

                    rw->cur_addr = NEXT_ADDR(rw->cur_addr);

                    progress_report(1);
                } while (rw->cur_addr != rw->first_addr);

                /* Reset addr and data and Verify values written */

                rw->cur_addr = rw->first_addr;
                rw->data = rw->data_start;

                miscomp = 0;

                if (count != rw->index_total) {
                    printk("WARNING:  number of writes != index total\n");
                }

                do {
                    if (soc_mem_entry_words(unit, rw->mem) % 2 == 0) {
                        rw->data = ~rw->data;
                    }
#ifdef BCM_POLAR_SUPPORT
                    if (SOC_IS_POLAR(unit)) {
                        int table_name;
                        if(_soc_mem_id_map(unit, rw->mem, &table_name) == SOC_E_NONE) {
                            if (DRV_MEM_READ(unit, table_name, rw->cur_addr, 1, data) < 0) {
                                printk("Read ERROR: table %s[%d] iteration %d\n",
                                       SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                       rw->cur_addr, iter);
                                if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                    printk("At array index %u.\n", array_index);
                                }
                                goto break_all;
                            }
                        }
                    } else
#endif /* BCM_POLAR_SUPPORT */
                    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                        if (soc_mem_array_read(unit, rw->mem, array_index, copyno,
                                         rw->cur_addr, data) < 0) {
                            printk("Read ERROR: table %s.%s[%d] iteration %d\n",
                                   SOC_MEM_UFNAME(unit, rw->mem),
                                   SOC_BLOCK_NAME(unit, copyno),
                                   rw->cur_addr, iter);
                            if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                printk("At array index %u.\n", array_index);
                            }
                            goto break_all;
                        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                    }

                    for (word = 0; word < dw; word++) {
                        uint32 comp_word = rw->data & mask[word];
                        if (accum_tcammask) {
                            /* data read back has dependency on mask */
                            if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM)||
                               (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU)) {
                                comp_word &= ~tcammask[word];
                            } else if (soc_feature(unit, soc_feature_xy_tcam)) {
                                comp_word |= tcammask[word];
                            }
                        }
                        if (accum_forcemask) {
                            comp_word &= ~forcemask[word];
                            comp_word |= forcedata[word];
                        }
                        if ((data[word] ^ comp_word) & mask[word]) {
                            soc_pci_analyzer_trigger(unit);

                            if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
#ifdef BCM_POLAR_SUPPORT
                                if (SOC_IS_POLAR(unit)) {
                                    printk("Compare ERROR: table %s[%u] iteration %d\n",
                                           SOC_ROBO_MEM_UFNAME(unit, rw->mem), array_index, iter+1);
                                } else
#endif /* BCM_POLAR_SUPPORT */
                                {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                                    printk("Compare ERROR: table %s[%u].%s iteration %d\n",
                                           SOC_MEM_UFNAME(unit, rw->mem), array_index,
                                           SOC_BLOCK_NAME(unit, copyno), iter+1);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                                }
                             } else {
#ifdef BCM_POLAR_SUPPORT
                                if (SOC_IS_POLAR(unit)) {
                                    printk("Compare ERROR: table %s iteration %d\n",
                                           SOC_ROBO_MEM_UFNAME(unit, rw->mem), iter+1);
                                } else
#endif /* BCM_POLAR_SUPPORT */
                                {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                                    printk("Compare ERROR: table %s.%s iteration %d\n",
                                           SOC_MEM_UFNAME(unit, rw->mem),
                                           SOC_BLOCK_NAME(unit, copyno), iter+1);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                                }
                            }
                            printk("Wrote word %d: 0x%08x.\n", word, comp_word);
                            printk("Read word %d:  0x%08x.\n", word, data[word]);
                            printk("Difference:    0x%08x.\n",
                                   comp_word ^ data[word]);
                            miscomp = 1;
                        }
                        rw->data = ~rw->data;
                    }

                    rw->cur_addr = NEXT_ADDR(rw->cur_addr);

                    progress_report(1);
                } while ((rw->cur_addr != rw->first_addr) && (!miscomp));

                if (miscomp) {
                    printk("Cycle aborted due to error\n");
                    test_error(unit, "\n");
                }
            }
        }
    }

 break_all:
    progress_done();

    return 0;
}

int
addr_rand_done(int unit, void *p)
{
    rand_work_t         *rw = p;
#ifdef BCM_SHADOW_SUPPORT
    int rv = SOC_E_NONE;
#endif

    if (rw) {
        /* Take MMU out of debug mode if testing a CBP memory */
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (soc_mem_is_debug(unit,rw->mem) &&
            rw->was_debug_mode >= 0 &&
            soc_mem_debug_set(unit, rw->was_debug_mode) < 0) {
            test_error(unit, "Could not restore previous MMU debug state\n");
            return -1;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */

        /* Restore non-atomic TCAM write handling status */
        SOC_CONTROL(unit)->tcam_protect_write = rw->saved_tcam_protect_write;

#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (soc_mem_cpu_write_control(unit, rw->mem, rw->copyno,
                                      rw->orig_enable, &rw->orig_enable) < 0) {
        test_error(unit, "Could not disable exclusive cpu write on memory "
                       "%s\n",
               SOC_MEM_UFNAME(unit, rw->mem));
        return -1;
    }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */

#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_DFE(unit)) {
        SOC_MEM_TEST_SKIP_CACHE_SET(unit, 0);
        if (soc_mem_parity_restore(unit, rw->mem, rw->copyno) < 0) {
            test_error(unit, "Could not enable parity warnings on memory %s\n",
               SOC_MEM_UFNAME(unit, rw->mem));
            return -1;
        }
    } else 
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        /* Do nothing for parity control */
    } else
#endif /* BCM_POLAR_SUPPORT */
    {    
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (addr_rand_common_restore(unit, rw->mem, rw->copyno)) {
            return -1;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
    }

#ifdef BCM_SHADOW_SUPPORT
        if (SOC_IS_SHADOW(unit) && (rw->mem == EGR_PERQ_XMT_COUNTERSm)) {
            rv = soc_reg_field32_modify(unit, EGR_EDB_HW_CONTROLr, REG_PORT_ANY,
                                        XGS_COUNTER_COMPAT_MODEf, 0);
           if (rv != SOC_E_NONE) {
               return -1;
           }
        }
        if (SOC_IS_SHADOW(unit) && SOC_BLOCK_IS_CMP(unit,
            SOC_MEM_BLOCK_MIN(unit, rw->mem), SOC_BLK_MS_ISEC)) {
            rv = soc_reg_field32_modify(unit, ISEC_MASTER_CTRLr, 1,
                                        XGS_COUNTER_COMPAT_MODEf, 0);
           if (rv != SOC_E_NONE) {
               return -1;
           }
            rv = soc_reg_field32_modify(unit, ISEC_MASTER_CTRLr, 5,
                                        XGS_COUNTER_COMPAT_MODEf, 0);
           if (rv != SOC_E_NONE) {
               return -1;
           }
        }
        if (SOC_IS_SHADOW(unit) && SOC_BLOCK_IS_CMP(unit,
            SOC_MEM_BLOCK_MIN(unit, rw->mem), SOC_BLK_MS_ESEC)) {
            rv = soc_reg_field32_modify(unit, ESEC_MASTER_CTRLr, 1,
                                        XGS_COUNTER_COMPAT_MODEf, 0);
           if (rv != SOC_E_NONE) {
               return -1;
           }
            rv = soc_reg_field32_modify(unit, ESEC_MASTER_CTRLr, 5,
                                        XGS_COUNTER_COMPAT_MODEf, 0);
           if (rv != SOC_E_NONE) {
               return -1;
           }
        }
#endif
    }

    return 0;
}

#endif /*  BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT || BCM_POLAR_SUPPORT*/
