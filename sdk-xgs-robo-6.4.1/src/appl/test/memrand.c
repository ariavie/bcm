/*
 * $Id: memrand.c,v 1.63 Broadcom SDK $
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
 * The test algorithm from Gil Winograd (Broadcom in Irvine) is:
 *
 * for ( a long time ) {
 *   generate 4 random addresses: a0, a1, a2, a3
 *   generate 4 random data words: d0, d1, d2, d3
 *   for ( i = 0 to 3 ) {
 *     write d0 to address a0
 *     write d1 to address a1
 *     write d2 to address a2
 *     write d3 to address a3
 *     read a0 into variable v0
 *     read a1 into variable v1
 *     read a2 into variable v2
 *     read a3 into variable v3
 *
 *     verify that all reads were correct
 *     if ( verification fails ) {
 *       print a0, a1, a2, a3
 *       print d0, d1, d2, d3
 *       print v0, v1, v2, v3
 *     }
 *   }
 * }
 */

#include <sal/types.h>
#include <shared/bsl.h>
#include <sal/appl/pci.h>
#include <sal/appl/sal.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/progress.h>

#include <soc/mem.h>
#include <soc/l2x.h>
#include <soc/memory.h>

#ifdef BCM_DPP_SUPPORT
#include <soc/dpp/drv.h>
#endif /* BCM_DPP_SUPPORT */
#ifdef BCM_DFE_SUPPORT
#include <soc/dfe/cmn/dfe_drv.h>
#endif /* BCM_DFE_SUPPORT */

#include "testlist.h"

#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined (BCM_POLAR_SUPPORT)
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
    int iters;                /* Total iterations to perform */

    soc_mem_t mem;            /* Memory to test */

    int copyno;                /* copy to test (or COPYNO_ALL) */
    int copyno_total;            /* Number of copies */

    int index_min;            /* Lowest index in range to test */
    int index_max;            /* Last index in range to test */
    int index_total;            /* Size of index range (max-min+1) */
    unsigned array_index_start;        /* Lowest array index in range to test */
    unsigned array_index_end;        /* Last array index in range to test */

    int continue_on_error;              /* Do not abort test due to errors */
    int error_max;                      /* Abort test after N errors */

    uint32 seed;            /* 0 = don't set */
    int ecc_as_data;                    /* treat ecc field as regular field */
} rand_work_t;

static rand_work_t    *rand_work[SOC_MAX_NUM_DEVICES];

#define GROUP_SIZE    4

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
            cli_out("Unsupport memory table.\n");
            return -1;
    }
    *drv_mem = table_name;
    return SOC_E_NONE;
}
#endif /* BCM_POLAR_SUPPORT */

#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
STATIC int
mem_rand_common_clear(int unit, soc_mem_t mem, int copyno)
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
                                             &rw->scan_interval)) > 0) {
        if (soc_mem_scan_stop(unit)) {
            return -1;
        }
    }
#endif /* INCLUDE_MEM_SCAN */
    return 0;
}

STATIC int
mem_rand_common_restore(int unit, soc_mem_t mem, int copyno)
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
mem_rand_init(int unit, args_t *a, void **p)
{
    rand_work_t        *rw;
    char        *mem_name;
    char        *idx_start_str, *idx_end_str;
    char        *array_idx_start_str, *array_idx_end_str;
    int            blk;
    int            rv = -1;
    parse_table_t    pt;
#ifdef BCM_POLAR_SUPPORT
    uint32 cache_enable;
#endif /* BCM_POLAR_SUPPORT */

    rw = rand_work[unit];
    if (rw == NULL) {
    rw = sal_alloc(sizeof(rand_work_t), "memrand");
    if (rw == NULL) {
        cli_out("%s: cannot allocate memory test data\n", ARG_CMD(a));
        return -1;
    }
    sal_memset(rw, 0, sizeof(rand_work_t));
    rand_work[unit] = rw;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt,  "Memory",    PQ_STRING, "",
            &mem_name, NULL);
    parse_table_add(&pt,  "IndexStart",    PQ_STRING, (void *) "min",
            &idx_start_str, NULL);
    parse_table_add(&pt,  "IndexEnd",    PQ_STRING, (void *) "max",
            &idx_end_str, NULL);
    parse_table_add(&pt,  "ArrayIndexStart", PQ_STRING, (void *) "min", &array_idx_start_str, NULL);
    parse_table_add(&pt,  "ArrayIndexEnd", PQ_STRING, (void *) "max", &array_idx_end_str, NULL);
    parse_table_add(&pt,  "ITERations", PQ_INT, (void *) 1500,
            &rw->iters, NULL);
    parse_table_add(&pt,  "IGnoreErrors", PQ_BOOL, 0,
                    &rw->continue_on_error, NULL);
    parse_table_add(&pt,  "ErrorMax",   PQ_INT, (void *) 1,
                    &rw->error_max, NULL);
    parse_table_add(&pt,  "SEED",    PQ_INT, (void *) 0xdecade,
            &rw->seed, NULL );
    parse_table_add(&pt,  "EccAsData",  PQ_BOOL, 0,
                &rw->ecc_as_data, NULL );

    if (parse_arg_eq(a, &pt) < 0) {
    cli_out("%s: Invalid option: %s\n",
            ARG_CMD(a), ARG_CUR(a));
    goto done;
    }

    if (ARG_CNT(a) != 0) {
    cli_out("%s: extra options starting with \"%s\"\n",
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

    if (!soc_mem_is_valid(unit, rw->mem)) {
#ifdef BCM_POLAR_SUPPORT
        if (SOC_IS_POLAR(unit)) {
            test_error(unit,
                   "Cannot test memory %s:  Invalid memory.\n",
                   SOC_ROBO_MEM_UFNAME(unit, rw->mem));
        } else
#endif /* BCM_POLAR_SUPPORT */
        {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
            test_error(unit,
                   "Cannot test memory %s:  Invalid memory.\n",
                   SOC_MEM_UFNAME(unit, rw->mem));
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
        }
        goto done;
    }
    if (soc_mem_is_readonly(unit, rw->mem)) {
#ifdef BCM_POLAR_SUPPORT
        if (SOC_IS_POLAR(unit)) {
            test_error(unit,
                   "Cannot test memory %s:  Readonly.\n",
                   SOC_ROBO_MEM_UFNAME(unit, rw->mem));
        } else
#endif /* BCM_POLAR_SUPPORT */
        {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
            test_error(unit,
                   "Cannot test memory %s:  Readonly.\n",
                   SOC_MEM_UFNAME(unit, rw->mem));
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
        }
        goto done;
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

#if defined (BCM_DFE_SUPPORT)
    if(SOC_IS_DFE(unit)) 
    { 
        int rv;
        int is_filtered = 0;
        
        rv = MBCM_DFE_DRIVER_CALL(unit, mbcm_dfe_drv_test_mem_filter, (unit, rw->mem, &is_filtered));
        if (rv != SOC_E_NONE)
        {
            return rv;
        }
        if (is_filtered)
        {
               test_error(unit, "Memory %s is invalid/readonly/writeonly/signal \n",
                       SOC_MEM_UFNAME(unit, rw->mem));
               goto done;
        }
    }
#endif /*BCM_DFE_SUPPORT*/

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        if (soc_robo_mem_index_max(unit, rw->mem) < 3) {
        test_error(unit,
               "Cannot test memory %s:  Too few entries.\n",
               SOC_ROBO_MEM_UFNAME(unit, rw->mem));
        goto done;
        }
    } else
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (soc_mem_index_max(unit, rw->mem) < 3) {
           if (soc_feature(unit,soc_feature_esm_support)) {
              if ((rw->mem == L3_DEFIPm && SOC_MEM_IS_ENABLED(unit, L3_DEFIPm)) ||
                  (rw->mem == L3_DEFIP_ONLYm && SOC_MEM_IS_ENABLED(unit, L3_DEFIP_ONLYm)) ||
                  (rw->mem == L3_DEFIP_DATA_ONLYm && SOC_MEM_IS_ENABLED(unit, L3_DEFIP_DATA_ONLYm)) ||
                  (rw->mem == L3_DEFIP_HIT_ONLYm && SOC_MEM_IS_ENABLED(unit, L3_DEFIP_DATA_ONLYm)) ||
                  (rw->mem == L3_DEFIP_PAIR_128_DATA_ONLYm && SOC_MEM_IS_ENABLED(unit, L3_DEFIP_PAIR_128_DATA_ONLYm)) ||
                  (rw->mem == L3_DEFIP_PAIR_128_HIT_ONLYm && SOC_MEM_IS_ENABLED(unit, L3_DEFIP_PAIR_128_HIT_ONLYm))) {
                  /* The above internal memories will not be supported if External TCAM is present.
                  These internal memories are set to 0 and hence can not run the test on these memories.*/
                  return BCM_E_UNAVAIL;
              } else {
                  test_error(unit,
                  "Cannot test memory %s:  Too few entries.\n",
                  SOC_MEM_UFNAME(unit, rw->mem));
                  goto done;
                }
           } else {
              test_error(unit,
                   "Cannot test memory %s:  Too few entries.\n",
                   SOC_MEM_UFNAME(unit, rw->mem));
              goto done;
             }
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
    }

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
        cli_out("WARNING: switching start and end array indices to %u-%u\n", rw->array_index_start, rw->array_index_end);
    }
    rw->index_total = rw->index_max - rw->index_min + 1;

    if (rw->index_total < 1 || rw->copyno_total < 1) {
    test_error(unit,
           "Min copyno/index must be less than max copyno/index\n");
    goto done;
    }
    
    if (rw->index_total < GROUP_SIZE) {
    test_error(unit,
           "Num of indexes must be greater than group size (%d)\n",GROUP_SIZE);
    goto done;
    }

    /* Place MMU in debug mode if testing an MMU memory */
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
        if (mem_rand_common_clear(unit, rw->mem, rw->copyno)) {
            goto done;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
    }

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        DRV_MEM_CACHE_GET(unit, rw->mem, &cache_enable);
        if (cache_enable) {
            cli_out("WARNING: Caching is enabled on memory %s\n",
                    SOC_ROBO_MEM_UFNAME(unit, rw->mem));
        }
    } else
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
        if (soc_mem_cache_get(unit, rw->mem,
                  rw->copyno == COPYNO_ALL ?
                  MEM_BLOCK_ALL : rw->copyno)) {
        cli_out("WARNING: Caching is enabled on memory %s.%s\n",
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

    if (rw->seed) {
    sal_srand(rw->seed);
    }

    *p = rw;

    rv = 0;

 done:
    parse_arg_eq_done(&pt);
    return rv;
}

/*
 * Memory Random Index Test
 */

static uint32
randword(void)
{
    uint32        word = 0;
    int            i;

    /*
     * This should make all 32 bits fairly random
     */

    for (i = 0; i < 3; i++) {
    /* Rotate left 7 */
    word = (word << 7) | ((word >> 25) & 0x7f);
    /* Add some random bits */
    word ^= (uint32) sal_rand();
    }

    return word;
}

static int
randnum(int maxplus1)
{
    return (sal_rand() >> 4) % maxplus1;
}

#undef TEST_INJECT_READ_ERROR

static void
dump_group(char *pfx,
       int dw,
       int *addrs,
       uint32 datas[GROUP_SIZE][SOC_MAX_MEM_WORDS])
{
    int            i, word;

    cli_out("%s", pfx);

    for (i = 0; i < GROUP_SIZE; i++) {
    cli_out("    Index[0x%08x] =", addrs[i]);

    for (word = 0; word < dw; word++)
        cli_out(" 0x%08x", datas[i][word]);

    cli_out("\n");
    }
}

int
mem_rand(int unit, args_t *a, void *p)
{
    rand_work_t        *rw = p;
    uint32        mask[SOC_MAX_MEM_WORDS];
    uint32        tcammask[SOC_MAX_MEM_WORDS];
    uint32        eccmask[SOC_MAX_MEM_WORDS];
    uint32              forcemask[SOC_MAX_MEM_WORDS];
    uint32              forcedata[SOC_MAX_MEM_WORDS];
    uint32              accum_tcammask, accum_forcemask;
    int            iter, i, dw, word, copyno;
    unsigned            array_index;
    int            addr[GROUP_SIZE];
    uint32        data[GROUP_SIZE][SOC_MAX_MEM_WORDS];
    uint32        verf[GROUP_SIZE][SOC_MAX_MEM_WORDS];
    char        status[160];
    int                 rv = 0;
    int                 error_count;

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

    progress_init(rw->iters * rw->copyno_total * (rw->array_index_end - rw->array_index_start + 1), 3, 0);

    SOC_MEM_BLOCK_ITER(unit, rw->mem, copyno) {
        if (rw->copyno != COPYNO_ALL && rw->copyno != copyno) {
            continue;
        }
        for (array_index = rw->array_index_start; array_index <= rw->array_index_end; ++array_index) {
            if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
#ifdef BCM_POLAR_SUPPORT
                if (SOC_IS_POLAR(unit)) {
                    sal_sprintf(status,
                                "Running %d iterations on %s[%u-%u].[%d-%d]",
                                rw->iters,
                                SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                rw->array_index_start, rw->array_index_end,
                                rw->index_min, rw->index_max);
                } else
#endif /* BCM_POLAR_SUPPORT */
                {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                    sal_sprintf(status,
                                "Running %d iterations on %s[%u-%u].%s[%d-%d]",
                                rw->iters,
                                SOC_MEM_UFNAME(unit, rw->mem),
                                rw->array_index_start, rw->array_index_end,
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

            error_count = 0;
            for (iter = 0; iter < rw->iters; iter++) {
                uint32     miscomp;
                int        j;

                /* Generate unique random addresses */

                for (i = 0; i < GROUP_SIZE; i++) {
                    addr[i] = rw->index_min + randnum(rw->index_total);
                    if (soc_mem_test_skip(unit, rw->mem, addr[i])) {
                        i--;
                        continue;
                    }
                    for (j = 0; j < i; j++)
                        if (addr[i] == addr[j]) {
                            i--;
                            break;
                        }
                    }

            /* Generate random data */

                for (i = 0; i < GROUP_SIZE; i++) {
                    for (word = 0; word < dw; word++) {
                        data[i][word] = randword() & mask[word];
                    }
                }
                if (accum_tcammask) {
                    /* data read back has dependency on mask */
                    if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
                       (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU)) {
                        for (i = 0; i < GROUP_SIZE; i++) {
                            for (word = 0; word < dw; word++) {
                                data[i][word] &= ~tcammask[word];
                            }
                        }
                    } else if (soc_feature(unit, soc_feature_xy_tcam)) {
                        for (i = 0; i < GROUP_SIZE; i++) {
                            for (word = 0; word < dw; word++) {
                                data[i][word] |= tcammask[word];
                            }
                        }
                    } 
                }
                if (accum_forcemask) {
                    for (i = 0; i < GROUP_SIZE; i++) {
                        for (word = 0; word < dw; word++) {
                            data[i][word] &= ~forcemask[word];
                            data[i][word] |= forcedata[word];
                        }
                    }
                }

                /* Perform writes (as quickly as possible) */

                for (i = 0; i < GROUP_SIZE; i++)
#ifdef BCM_POLAR_SUPPORT
                    if (SOC_IS_POLAR(unit)) {
                        int table_name;
                        if(_soc_mem_id_map(unit, rw->mem, &table_name) == SOC_E_NONE) {
                            if (DRV_MEM_WRITE(unit, table_name, addr[i], 1, data[i]) < 0) {
                                cli_out("Write ERROR: table %s[%d] iteration %d\n",
                                        SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                        addr[i], iter);
                                if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                    cli_out("At array index %u.\n", array_index);
                                }
                                goto break_all;
                            }
                        }
                    } else
#endif /* BCM_POLAR_SUPPORT */
                    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                        if ((rv = soc_mem_array_write(unit, rw->mem, array_index, copyno,
                                                addr[i], data[i])) < 0) {
                            cli_out("Write ERROR: table %s.%s[%d] iteration %d\n",
                                    SOC_MEM_UFNAME(unit, rw->mem),
                                    SOC_BLOCK_NAME(unit, copyno),
                                    addr[i], iter);
                            if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                cli_out("At array index %u.\n", array_index);
                            }
                            goto break_all;
                        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                    }

                /* Perform reads (as quickly as possible) */

                for (i = 0; i < GROUP_SIZE; i++) {
#ifdef BCM_POLAR_SUPPORT
                    if (SOC_IS_POLAR(unit)) {
                        int table_name;
                        if(_soc_mem_id_map(unit, rw->mem, &table_name) == SOC_E_NONE) {
                            if (DRV_MEM_READ(unit, table_name, addr[i], 1, verf[i]) < 0) {
                                cli_out("Read ERROR: table %s[%d] iteration %d\n",
                                        SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                        addr[i], iter);
                                if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                    cli_out("At array index %u.\n", array_index);
                                }
                                goto break_all;
                            }
                        }
                    } else
#endif /* BCM_POLAR_SUPPORT */
                    {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                        if ((rv = soc_mem_array_read(unit, rw->mem, array_index, copyno,
                                               addr[i], verf[i])) < 0) {
                            cli_out("Read ERROR: table %s.%s[%d] iteration %d\n",
                                    SOC_MEM_UFNAME(unit, rw->mem),
                                    SOC_BLOCK_NAME(unit, copyno),
                                    addr[i], iter);
                            if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                cli_out("At array index %u.\n", array_index);
                            }
                            goto break_all;
                        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                    }

#ifdef TEST_INJECT_READ_ERROR
                    if (addr[i] == 7)
                        verf[0][0] ^= 0x00010000;
#endif
                }

                /*
                 * Verify data
                 */

                miscomp = FALSE;
                for (i = 0; i < GROUP_SIZE; i++)
                    for (word = 0; word < dw; word++) {
                        if (!((verf[i][word] ^ data[i][word]) & mask[word])) {
                            continue;
                        }
                        miscomp = TRUE;
#ifdef BCM_POLAR_SUPPORT
                        if (SOC_IS_POLAR(unit)) {
                            cli_out("Compare ERROR: table %s[%d] iteration %d\n",
                                    SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                    addr[i], iter+1);
                        } else
#endif /* BCM_POLAR_SUPPORT */
                        {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                            cli_out("Compare ERROR: table %s.%s[%d] iteration %d\n",
                                    SOC_MEM_UFNAME(unit, rw->mem),
                                    SOC_BLOCK_NAME(unit, copyno),
                                    addr[i], iter+1);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                        }
                        if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                            cli_out("At array index %u.\n", array_index);
                        }
                    }

                /*
                 * Process miscompare(s)
                 */

                if (miscomp) {
                    soc_pci_analyzer_trigger(unit);

                    dump_group("Write data:\n", dw, addr, data);
                    dump_group("Read data:\n", dw, addr, verf);

                    for (i = 0; i < GROUP_SIZE; i++) {
                        for (word = 0; word < dw; word++)
                            data[i][word] ^= verf[i][word];
#ifdef BCM_POLAR_SUPPORT
                        if (SOC_IS_POLAR(unit)) {
                            int table_name;
                            if(_soc_mem_id_map(unit, rw->mem, &table_name) == SOC_E_NONE) {
                                if (DRV_MEM_READ(unit, table_name, addr[i], 1, verf[i]) < 0) {
                                    cli_out("Read ERROR: table %s[%d]\n",
                                            SOC_ROBO_MEM_UFNAME(unit, rw->mem),
                                            addr[i]);
                                    if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                        cli_out("At array index %u.\n", array_index);
                                    }
                                    goto break_all;
                                }
                            }
                        } else
#endif /* BCM_POLAR_SUPPORT */
                        {
#if defined (BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
                            if ((rv = soc_mem_array_read(unit, rw->mem, array_index, copyno,
                                                   addr[i], verf[i])) < 0) {
                                cli_out("Read ERROR: table %s.%s[%d]\n",
                                        SOC_MEM_UFNAME(unit, rw->mem),
                                        SOC_BLOCK_NAME(unit, copyno),
                                        addr[i]);
                                if (rw->array_index_start != 0 || rw->array_index_end != rw->array_index_start) {
                                    cli_out("At array index %u.\n", array_index);
                                }
                                goto break_all;
                            }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_DFE_SUPPORT || BCM_PETRA_SUPPORT || BCM_CALADAN3_SUPPORT */
                        }
                    }

                    dump_group("Difference:\n", dw, addr, data);
                    dump_group("Re-read results:\n", dw, addr, verf);

                    test_error(unit, "\n");

                    error_count++;
                }

                if (!rw->continue_on_error && error_count > rw->error_max) {
                    break;
                }
                progress_report(1);
            }
        }
    }

 break_all:
    progress_done();

    return rv;
}

int
mem_rand_done(int unit, void *p)
{
    rand_work_t        *rw = p;
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
        if (mem_rand_common_restore(unit, rw->mem, rw->copyno)) {
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
