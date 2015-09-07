/*
 * $Id: memtest.c,v 1.46 Broadcom SDK $
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
 * Memory Test Kernel
 *
 * Streamlined module designed for inclusion in the SOC driver for
 * performing power-on memory tests.
 *
 * This module is also used by the main SOC diagnostics memory tests,
 * fronted by user interface code.
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>

#include <soc/cm.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#ifdef BCM_BRADLEY_SUPPORT
#include <soc/bradley.h>
#endif
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <soc/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <soc/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
#include <soc/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
#include <soc/enduro.h>
#endif /* BCM_ENDURO_SUPPORT */
#if defined(BCM_HURRICANE_SUPPORT)
#include <soc/hurricane.h>
#endif /* BCM_HURRICANE_SUPPORT */
#if defined(BCM_HURRICANE2_SUPPORT)
#include <soc/hurricane2.h>
#endif /* BCM_HURRICANE2_SUPPORT */
#if defined(BCM_GREYHOUND_SUPPORT)
#include <soc/greyhound.h>
#endif /* BCM_GREYHOUND_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
#include <soc/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_PETRA_SUPPORT)
#include <soc/dpp/drv.h>
#endif /* BCM_PETRA_SUPPORT */
#if defined(BCM_DFE_SUPPORT)
#include <soc/dfe/cmn/dfe_drv.h>
#endif /* BCM_DFE_SUPPORT */

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_POLAR_SUPPORT)
/*
 * Table memory test
 */

#define PAT_PLAIN    0    /* Keep writing same word */
#define PAT_XOR      1    /* XOR 0xffffffff between entries */
#define PAT_RANDOM   2    /* Add 0xdeadbeef between words */
#define PAT_INCR     3    /* Add 1 between words */

STATIC void
fillpat(uint32 *seed, uint32 *mask, uint32 *buf, int pat, int dw)
{
    int            i;

    switch (pat) {
    case PAT_PLAIN:
    default:
    for (i = 0; i < dw; i++) {
        buf[i] = (*seed) & mask[i];
    }
    break;
    case PAT_XOR:
    for (i = 0; i < dw; i++) {
        buf[i] = (*seed) & mask[i];
    }
    (*seed) ^= 0xffffffff;
    break;
    case PAT_RANDOM:
    for (i = 0; i < dw; i++) {
        buf[i] = (*seed) & mask[i];
        (*seed) += 0xdeadbeef;
    }
    break;
    case PAT_INCR:
    for (i = 0; i < dw; i++) {
        buf[i] = (*seed) & mask[i];
    }
    (*seed) += 1;
    break;
    }
}

/*
 * Function:     soc_mem_datamask_memtest
 * Purpose:      Patch a bit mask for a memory entry under test
 * Returns:      SOC_E_xxx
 */
void
soc_mem_datamask_memtest(int unit, soc_mem_t mem, uint32 *buf)
{
#ifdef BCM_SCORPION_SUPPORT
    if (SOC_IS_SC_CQ(unit)) {
        switch (mem) {
        case FP_TCAM_Xm:
            soc_mem_field32_set(unit, FP_TCAM_Xm, buf, IPBMf, 0x1fff);
            soc_mem_field32_set(unit, FP_TCAM_Xm, buf, IPBM_MASKf, 0x1fff);
            break;
        case FP_TCAM_Ym:
            soc_mem_field32_set(unit, FP_TCAM_Ym, buf, IPBMf, 0x1fffe000);
            soc_mem_field32_set(unit, FP_TCAM_Ym, buf, IPBM_MASKf, 0x1fffe000);
            break;
        default:
            break;
        }
    }
#endif
#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        switch (mem) {
        case VLAN_1Qm:
            soc_mem_field32_set(unit, VLAN_1Qm, buf, FORWARD_MAPf_ROBO, 0x1bf);
            soc_mem_field32_set(unit, VLAN_1Qm, buf, UNTAG_MAPf_ROBO, 0x1bf);
            break;
        case CFP_ACT_POLm:
            soc_mem_field32_set(unit, CFP_ACT_POLm, buf, DST_MAP_OBf_ROBO, 0xff);
            soc_mem_field32_set(unit, CFP_ACT_POLm, buf, DST_MAP_IBf_ROBO, 0xff);
            break;
        case CFP_METERm:
            soc_mem_field32_set(unit, CFP_METERm, buf, RATE_REFRESH_ENf_ROBO, 0x0);
            soc_mem_field32_set(unit, CFP_METERm, buf, CURR_QUOTAf_ROBO, 0x0);
            break;
        case CFP_TCAM_CHAIN_MASKm:
        case CFP_TCAM_IPV4_MASKm:
        case CFP_TCAM_IPV6_MASKm:
        case CFP_TCAM_NONIP_MASKm:
            soc_mem_field32_set(unit, CFP_TCAM_IPV4_MASKm, buf, VALID_Rf_ROBO, 0x0);
            break;
        default:
            break;
        }
    }
#endif
}

/*
 * memtest_fill
 *
 *   parm:         Test parameters
 *   mem:       soc_mem_t of memory to test
 *   copyno:       which copy of memory to test
 *   seed:         test pattern start value
 *   pat:          test pattern function
 *
 *   Returns 0 on success, -1 on error.
 */

#define FOREACH_INDEX \
  for (index = index_start; \
    index_start <= index_end ? index <= index_end : index >= index_end; \
    index += index_step)

STATIC int
memtest_fill(int unit, soc_mem_test_t *parm, unsigned array_index, int copyno,
                 uint32 *seed, int pat)
{
    uint32               buf[SOC_MAX_MEM_WORDS];
    uint32               mask[SOC_MAX_MEM_WORDS];
    uint32               tcammask[SOC_MAX_MEM_WORDS];
    uint32               eccmask[SOC_MAX_MEM_WORDS];
    uint32               forcemask[SOC_MAX_MEM_WORDS];
    uint32               forcedata[SOC_MAX_MEM_WORDS];
    uint32               accum_tcammask, accum_forcemask;
    soc_mem_t            mem = parm->mem;
    int                  index, dw, rv, i;
    int                  index_start = parm->index_start;
    int                  index_end = parm->index_end;
    int                  index_step;

    index_step = parm->index_step;



    dw = soc_mem_entry_words(unit, mem);
    soc_mem_datamask_get(unit, mem, mask);
    soc_mem_tcammask_get(unit, mem, tcammask);
    soc_mem_eccmask_get(unit, mem, eccmask);
    soc_mem_forcedata_get(unit, mem, forcemask, forcedata);
    accum_tcammask = 0;
    for (i = 0; i < dw; i++) {
        accum_tcammask |= tcammask[i];
    }
    accum_forcemask = 0;
    for (i = 0; i < dw; i++) {
        accum_forcemask |= forcemask[i];
    }
    if (!parm->ecc_as_data) {
        for (i = 0; i < dw; i++) {
            mask[i] &= ~eccmask[i];
        }
    }
    soc_mem_datamask_memtest(unit, mem, mask);

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    if (parm->array_index_start != 0 || parm->array_index_end != parm->array_index_start) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "  FILL %s[%u-%u].%s[%d-%d]\n"),
                     SOC_MEM_UFNAME(unit, mem),
                     parm->array_index_start, parm->array_index_end,
                     SOC_BLOCK_NAME(unit, copyno),
                     index_start, index_end));
    } else 
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    {
#ifdef BCM_POLAR_SUPPORT
        if (SOC_IS_POLAR(unit)) {
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "  FILL %s[%d-%d]\n"),
                         SOC_ROBO_MEM_UFNAME(unit, mem),
                         index_start, index_end));
        } else
#endif /* BCM_POLAR_SUPPORT */
        {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "  FILL %s.%s[%d-%d]\n"),
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno),
                         index_start, index_end));
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
        }
    }

    if (bsl_check(bslLayerSoc, bslSourceSocmem, bslSeverityNormal, unit)) {
    LOG_CLI((BSL_META_U(unit,
                        "   MASK")));
    for (i = 0; i < dw; i++) {
        LOG_CLI((BSL_META_U(unit,
                            " 0x%08x"), mask[i]));
    }
    LOG_CLI((BSL_META_U(unit,
                        "\n")));
        if (accum_tcammask) {
            LOG_CLI((BSL_META_U(unit,
                                "   TCAM MASK")));
            for (i = 0; i < dw; i++) {
                LOG_CLI((BSL_META_U(unit,
                                    " 0x%08x"), tcammask[i]));
            }
            LOG_CLI((BSL_META_U(unit,
                                "\n")));
        }
    }

    FOREACH_INDEX {
    if (soc_mem_test_skip(unit, parm->mem, index)) {
        continue;
    }
    fillpat(seed, mask, buf, pat, dw);
        if (accum_tcammask) {
            /* data read back has dependency on mask */
            if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
                 (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU)) {
                for (i = 0; i < dw; i++) {
                    buf[i] &= ~tcammask[i];
                }
            } else if (soc_feature(unit, soc_feature_xy_tcam)) {
                for (i = 0; i < dw; i++) {
                    buf[i] |= tcammask[i];
                }
            }
        }
        if (accum_forcemask) {
            for (i = 0; i < dw; i++) {
                buf[i] &= ~forcemask[i];
                buf[i] |= forcedata[i];
            }
        }
    if ((rv = (*parm->write_cb)(parm, array_index, copyno, index, buf)) < 0) {
        return rv;
    }
    }

    return SOC_E_NONE;
}

/*
 * memtest_verify
 *
 *   parm:         Test parameters
 *   mem:       soc_mem_t of memory to test
 *   copyno:       which copy of memory to test
 *   seed:         test pattern start value
 *   incr:         test pattern increment value
 *
 *   Returns 0 on success, -1 on error.
 */

STATIC int
memtest_verify(int unit, soc_mem_test_t *parm, unsigned array_index, int copyno,
               uint32 *seed, int pat)
{
    uint32        buf[SOC_MAX_MEM_WORDS];
    uint32        mask[SOC_MAX_MEM_WORDS];
    uint32        tcammask[SOC_MAX_MEM_WORDS];
    uint32        eccmask[SOC_MAX_MEM_WORDS];
    uint32        forcemask[SOC_MAX_MEM_WORDS];
    uint32        forcedata[SOC_MAX_MEM_WORDS];
    uint32        accum_tcammask, accum_forcemask;
    uint32        cmp[SOC_MAX_MEM_WORDS];
    soc_mem_t     mem = parm->mem;
    int           index, dw, rv, i;
    int           index_start = parm->index_start;
    int           index_end = parm->index_end;
    int           index_step;
    int           read_cnt;

    index_step = parm->index_step;
    for (i=0;i<SOC_MAX_MEM_WORDS;i++) {
        mask[i] = 0;
        tcammask[i] = 0;
        eccmask[i] = 0;
    }
    dw = soc_mem_entry_words(unit, mem);
#if defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    if(SOC_IS_DPP(unit) || SOC_IS_DFE(unit)) {
        soc_mem_datamask_rw_get(unit, mem, mask);
    } else 
#endif 
    {
        soc_mem_datamask_get(unit, mem, mask);
    }
    soc_mem_tcammask_get(unit, mem, tcammask);
    soc_mem_eccmask_get(unit, mem, eccmask);
    soc_mem_forcedata_get(unit, mem, forcemask, forcedata);
    accum_tcammask = 0;
    for (i = 0; i < dw; i++) {
        accum_tcammask |= tcammask[i];
    }
    accum_forcemask = 0;
    for (i = 0; i < dw; i++) {
        accum_forcemask |= forcemask[i];
    }
    if (!parm->ecc_as_data) {
        for (i = 0; i < dw; i++) {
            mask[i] &= ~eccmask[i];
        }
    }
    soc_mem_datamask_memtest(unit, mem, mask);

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    if (parm->array_index_start != 0 || parm->array_index_end != parm->array_index_start) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "  VERIFY %s[%u-%u].%s[%d-%d] Reading %d times\n"),
                     SOC_MEM_UFNAME(unit, mem),
                     parm->array_index_start, parm->array_index_end,
                     SOC_BLOCK_NAME(unit, copyno),
                     index_start, index_end,
                     parm->read_count));
    } else 
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    {
#ifdef BCM_POLAR_SUPPORT
        if (SOC_IS_POLAR(unit)) {
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "  VERIFY %s[%d-%d] Reading %d times\n"),
                         SOC_ROBO_MEM_UFNAME(unit, mem),
                         index_start, index_end,
                         parm->read_count));
        } else
#endif /* BCM_POLAR_SUPPORT */
        {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "  VERIFY %s.%s[%d-%d] Reading %d times\n"),
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno),
                         index_start, index_end,
                         parm->read_count));
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
        }
    }

    FOREACH_INDEX {
    if (soc_mem_test_skip(unit, parm->mem, index)) {
        continue;
    }
    fillpat(seed, mask, cmp, pat, dw);
        if (accum_tcammask) {
            /* data read back has dependency on mask */
            if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
               (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU)) {
                for (i = 0; i < dw; i++) {
                    cmp[i] &= ~tcammask[i];
                }
            } else if (soc_feature(unit, soc_feature_xy_tcam)) {
                for (i = 0; i < dw; i++) {
                    cmp[i] |= tcammask[i];
                }
            }
        }
        if (accum_forcemask) {
            for (i = 0; i < dw; i++) {
                cmp[i] &= ~forcemask[i];
                cmp[i] |= forcedata[i];
            }
        }

    for (read_cnt = 0; read_cnt < parm->read_count; read_cnt++) {
        if ((rv = (*parm->read_cb)(parm, array_index, copyno, index, buf)) < 0) {
        return rv;
        }

        for (i = 0; i < dw; i++) {
        if ((buf[i] ^ cmp[i]) & mask[i]) {
            break;
        }
        }

        if (i < dw) {
        parm->err_count++;
        if ((*parm->miscompare_cb)(parm, array_index, copyno, index, buf,
                   cmp, mask) == MT_MISCOMPARE_STOP) {
                    parm->error_count++;
                    if (!parm->continue_on_error &&
                               (parm->error_count >= parm->error_max)) {
                        return SOC_E_FAIL;
                    }
        }
        }
    }
    }

    return SOC_E_NONE;
}

/*
 * memtest_test_by_entry_pattern
 *
 *   Test memories one entry at a time.
 */
STATIC int
memtest_test_by_entry_pattern(int unit, soc_mem_test_t *parm, uint32 seed0,
                          int pat, char *desc)
{
    int                 copyno;
    unsigned            array_index;
    uint32              buf[SOC_MAX_MEM_WORDS];
    uint32              mask[SOC_MAX_MEM_WORDS];
    uint32              tcammask[SOC_MAX_MEM_WORDS];
    uint32              eccmask[SOC_MAX_MEM_WORDS];
    uint32              forcemask[SOC_MAX_MEM_WORDS];
    uint32              forcedata[SOC_MAX_MEM_WORDS];
    uint32              cmp[SOC_MAX_MEM_WORDS];
    uint32              accum_tcammask, accum_forcemask;
    soc_mem_t           mem = parm->mem;
    int                 index, dw, i;
    int                 index_start = parm->index_start;
    int                 index_end = parm->index_end;
    int                 index_step = parm->index_step;
    int                 read_cnt;

    dw = soc_mem_entry_words(unit, mem);
    soc_mem_datamask_get(unit, mem, mask);
    soc_mem_tcammask_get(unit, mem, tcammask);
    soc_mem_eccmask_get(unit, mem, eccmask);
    soc_mem_forcedata_get(unit, mem, forcemask, forcedata);
    accum_tcammask = 0;
    for (i = 0; i < dw; i++) {
        accum_tcammask |= tcammask[i];
    }
    accum_forcemask = 0;
    for (i = 0; i < dw; i++) {
        accum_forcemask |= forcemask[i];
    }
    if (!parm->ecc_as_data) {
        for (i = 0; i < dw; i++) {
            mask[i] &= ~eccmask[i];
        }
    }
    soc_mem_datamask_memtest(unit, mem, mask);

    for (array_index = parm->array_index_start; array_index <= parm->array_index_end; ++array_index) {
        SOC_MEM_BLOCK_ITER(unit, mem, copyno) {
            if (parm->copyno != COPYNO_ALL && parm->copyno != copyno) {
                continue;
            }
            FOREACH_INDEX {
                if (soc_mem_test_skip(unit, parm->mem, index)) {
                    continue;
                }
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
                if (parm->array_index_start != 0 || parm->array_index_end != parm->array_index_start) {
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "  WRITE/READ %s[%u-%u].%s[%d]\n"),
                                 SOC_MEM_UFNAME(unit, mem),
                                 parm->array_index_start, parm->array_index_end,
                                 SOC_BLOCK_NAME(unit, copyno),
                                 index));
                } else 
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
                {
#ifdef BCM_POLAR_SUPPORT
                    if (SOC_IS_POLAR(unit)) {
                        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                    (BSL_META_U(unit,
                                                "  WRITE/READ %s[%d]\n"),
                                     SOC_ROBO_MEM_UFNAME(unit, mem),
                                     index));
                    } else
#endif /* BCM_POLAR_SUPPORT */
                    {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
                        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                    (BSL_META_U(unit,
                                                "  WRITE/READ %s.%s[%d]\n"),
                                     SOC_MEM_UFNAME(unit, mem),
                                     SOC_BLOCK_NAME(unit, copyno),
                                     index));
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
                    }
                }
                if (bsl_check(bslLayerSoc, bslSourceSocmem, bslSeverityNormal, unit)) {
                    LOG_CLI((BSL_META_U(unit,
                                        "   MASK")));
                    for (i = 0; i < dw; i++) {
                        LOG_CLI((BSL_META_U(unit,
                                            " 0x%08x"), mask[i]));
                    }
                    LOG_CLI((BSL_META_U(unit,
                                        "\n")));
                    if (accum_tcammask) {
                        LOG_CLI((BSL_META_U(unit,
                                            "   TCAM MASK")));
                        for (i = 0; i < dw; i++) {
                            LOG_CLI((BSL_META_U(unit,
                                                " 0x%08x"), tcammask[i]));
                        }
                    }
                    LOG_CLI((BSL_META_U(unit,
                                        "\n")));
                }
                /* First, write the data */
                fillpat(&seed0, mask, buf, pat, dw);
                if (accum_tcammask) {
                    /* data read back has dependency on mask */
                    if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
                       (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU)) {
                        for (i = 0; i < dw; i++) {
                            buf[i] &= ~tcammask[i];
                        }
                    } else if (soc_feature(unit, soc_feature_xy_tcam)) {
                        for (i = 0; i < dw; i++) {
                            buf[i] |= tcammask[i];
                        }
                    }
                }
                if (accum_forcemask) {
                    for (i = 0; i < dw; i++) {
                        buf[i] &= ~forcemask[i];
                        buf[i] |= forcedata[i];
                    }
                }
                for (i = 0; i < dw; i++) {
                    cmp[i] = buf[i];
                }
                if ((*parm->write_cb)(parm, array_index, copyno, index, buf) < 0) {
                    return -1;
                }
                /* Then, read the data */
                for (read_cnt = 0; read_cnt < parm->read_count; read_cnt++) {
                    if ((*parm->read_cb)(parm, array_index, copyno, index, buf) < 0) {
                        return -1;
                    }
                    for (i = 0; i < dw; i++) {
                        if ((buf[i] ^ cmp[i]) & mask[i]) {
                            break;
                        }
                    }
                    if (i < dw) {
                        parm->err_count++;
                        if ((*parm->miscompare_cb)(parm, array_index, copyno, index, buf,
                                                   cmp, mask) == MT_MISCOMPARE_STOP) {
                            parm->error_count++;
                            if (!parm->continue_on_error &&
                                       (parm->error_count >= parm->error_max)) {
                                return SOC_E_FAIL;
                            }
                        }
                    }
                }
            }
        }
    }

    return SOC_E_NONE;
}


/*
 * memtest_pattern
 *
 *   Calls memtest_fill and memtest_verify on copies of the table.
 */
STATIC int
memtest_pattern(int unit, soc_mem_test_t *parm, uint32 seed0,
        int pat, char *desc)
{
    int         copyno;
    uint32      seed;
    uint32      seed_save = 0;
    int         verify, rv, i;
    uint32      array_index;
    static char msg[80];

    if (parm->test_by_entry) {
        return memtest_test_by_entry_pattern(unit, parm, seed0, pat, desc);
    }

    /* Two passes: fill, then verify */

    for (verify = 0; verify < 2; verify++) {
        seed = seed0;

        for (array_index = parm->array_index_start; array_index <= parm->array_index_end; ++array_index) {

            SOC_MEM_BLOCK_ITER(unit, parm->mem, copyno) {
                if (parm->copyno != COPYNO_ALL && parm->copyno != copyno) {
                    continue;
                }
#ifdef BCM_ARAD_SUPPORT
                if (SOC_IS_ARAD(unit))
                {
                    if ((parm->mem == IHB_MEM_230000m) ||
                        (parm->mem == OCB_OCBM_EVENm) ||
                        (parm->mem == OCB_OCBM_ODDm)) {
                        continue;
                    }
                }
#endif            
                if (verify) {
                    seed_save = seed;
                    for (i = 0; i < parm->reverify_count + 1; i++) {
                        if (parm->reverify_delay > 0) {
                            sal_sleep(parm->reverify_delay);
                        }
                        if (parm->status_cb) {
                            sal_sprintf(msg, "Verifying %s", desc);
                            (*parm->status_cb)(parm, msg);
                        }
    
                        seed = seed_save;
                        if (memtest_verify(unit, parm, array_index, copyno, &seed, pat) != 0) {
                            if (!parm->continue_on_error &&
                                (parm->error_count >= parm->error_max)) {
                                return SOC_E_FAIL;
                            }
                        }
                    }
                } else {
                    if (parm->status_cb) {
                        sal_sprintf(msg, "Filling %s", desc);
                        (*parm->status_cb)(parm, msg);
                    }

                    if ((rv = memtest_fill(unit, parm, array_index, copyno,
                           &seed, pat)) < 0) {
                        return rv;
                    }
                }
            }
        }
    }

    return SOC_E_NONE;
}

int
soc_mem_test_skip(int unit, soc_mem_t mem, int index)
{
#ifdef BCM_ESW_SUPPORT
    if (mem == FP_GLOBAL_MASK_TCAMm || mem == FP_TCAMm ||
        mem == EFP_TCAMm || mem == VFP_TCAMm) {
        if (soc_feature(unit, soc_feature_field_ingress_two_slice_types) &&
            soc_feature(unit, soc_feature_field_stage_ingress_512_half_slice)){
            if (mem == FP_GLOBAL_MASK_TCAMm ||  mem == FP_TCAMm) {
                if (index < (soc_mem_index_count(unit, mem) / 2)) {
                    if ((index / 256) % 2) {
                        return TRUE;
                    }
                }
            }
        }

        if (soc_feature(unit, soc_feature_field_ingress_two_slice_types) &&
            soc_feature(unit, soc_feature_field_slices8)) {
            if (mem == FP_GLOBAL_MASK_TCAMm || mem == FP_TCAMm) {
                if (index >=
                            (soc_mem_index_count(unit, mem) * 3 / 4)) {
                    return TRUE;
                }
            }
        }

        if (soc_feature(unit, soc_feature_field_stage_egress_256_half_slice)) {
            if (mem == EFP_TCAMm) {
                if ((index / 128) % 2) {
                    return TRUE;
                }
            }
        }

        if (soc_feature(unit, soc_feature_field_stage_lookup_256_half_slice)) {
            if (mem == VFP_TCAMm) {
                if ((index / 64) % 2) {
                    return TRUE;
                }
            }
        }
        if (soc_feature(unit, soc_feature_field_slice_size128)) {
            if ((mem == FP_GLOBAL_MASK_TCAMm) || (mem == FP_TCAMm)) {
                if ((index/64) % 2) {
                    return TRUE;
                }
            }
        }
    }
#endif /* BCM_ESW_SUPPORT */

    return FALSE;
}


/*
 * Memory test main entry point
 */

int
soc_mem_test(soc_mem_test_t *parm)
{
    int            rv = SOC_E_NONE;
    int            unit = parm->unit;

    parm->err_count = 0;
    
    if (parm->patterns & MT_PAT_ZEROES) {
    /* Detect bits stuck at 1 */
    if ((rv = memtest_pattern(unit, parm, 0x00000000,
                  PAT_PLAIN, "0x00000000")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_ONES) {
    /* Detect bits stuck at 0 */
    if ((rv = memtest_pattern(unit, parm, 0xffffffff,
                  PAT_PLAIN, "0xffffffff")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_FIVES) {
    if ((rv = memtest_pattern(unit, parm, 0x55555555,
                  PAT_PLAIN, "0x55555555")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_AS) {
    if ((rv = memtest_pattern(unit, parm, 0xaaaaaaaa,
                  PAT_PLAIN, "0xaaaaaaaa")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_CHECKER) {
    /*
     * Checkerboard alternates 5's and a's to generate a pattern shown
     * below.  This is useful to detecting horizontally or vertically
     * shorted data lines (in cases where the RAM layout is plain).
     *
     *     1 0 1 0 1 0 1 0 1 0 1
     *     0 1 0 1 0 1 0 1 0 1 0
     *     1 0 1 0 1 0 1 0 1 0 1
     *     0 1 0 1 0 1 0 1 0 1 0
     *     1 0 1 0 1 0 1 0 1 0 1
     */
    if ((rv = memtest_pattern(unit, parm, 0x55555555,
                  PAT_XOR, "checker-board")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_ICHECKER) {
    /* Same as checker-board but inverted */
    if ((rv = memtest_pattern(unit, parm, 0xaaaaaaaa,
                  PAT_XOR, "inverted checker-board")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_ADDR) {
    /*
     * Write a unique value in every location to make sure all
     * locations are distinct (detect shorted address bits).
     * The unique value used is the address so dumping the failed
     * memory may indicate where the bad data came from.
     */
    if ((rv = memtest_pattern(unit, parm, 0,
                  PAT_INCR, "linear increment")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_RANDOM) {
    /*
     * Write a unique value in every location using pseudo-random data.
     */
    if ((rv = memtest_pattern(unit, parm, 0xd246fe4b,
                  PAT_RANDOM, "pseudo-random")) < 0) {
        goto done;
    }
    }

    if (parm->patterns & MT_PAT_HEX) {
        /*
         * Write a value specified by the user.
         */
        int i, val = 0;
        for (i = 0; i < 4; i ++) {
            val |= (parm->hex_byte & 0xff) << i*8;
        }
        if ((rv = memtest_pattern(unit, parm, val,
                                  PAT_PLAIN, "hex_byte")) < 0) {
          goto done;
        }
    }

    if (parm->err_count > 0) {
    rv = SOC_E_FAIL;
    }

 done:

    return rv;
}

#ifdef BCM_FIREBOLT_SUPPORT
STATIC int
_soc_fb_mem_parity_control(int unit, soc_mem_t mem, int copyno, int enable)
{
    uint32 imask, oimask, misc_cfg, omisc_cfg;
    soc_field_t parity_f = PARITY_CHECK_ENf;

    COMPILER_REFERENCE(copyno);

    switch (mem) {
        case L2Xm:
        case L2_ENTRY_ONLYm:
        if (soc_feature(unit, soc_feature_l2x_parity)) {
            SOC_IF_ERROR_RETURN(READ_L2_ENTRY_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, L2_ENTRY_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, L2_ENTRY_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_L2_ENTRY_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;

        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_ONLYm:
        case L3_ENTRY_VALID_ONLYm:
        if (soc_feature(unit, soc_feature_l3x_parity)) {
            SOC_IF_ERROR_RETURN(READ_L3_ENTRY_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, L3_ENTRY_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, L3_ENTRY_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_L3_ENTRY_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;

        case L3_DEFIPm:
        case L3_DEFIP_DATA_ONLYm:
        if (soc_feature(unit, soc_feature_l3defip_parity)) {
            SOC_IF_ERROR_RETURN(READ_L3_DEFIP_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, L3_DEFIP_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, L3_DEFIP_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;

#ifdef BCM_RAVEN_SUPPORT
        case DSCP_TABLEm:
        if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
            SOC_IF_ERROR_RETURN(READ_DSCP_TABLE_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, DSCP_TABLE_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, DSCP_TABLE_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_DSCP_TABLE_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;
     
        case EGR_L3_INTFm:
        if (!SOC_IS_HAWKEYE(unit)) {
            if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
                SOC_IF_ERROR_RETURN(READ_EGR_L3_INTF_PARITY_CONTROLr(unit, &imask));
                soc_reg_field_set(unit, EGR_L3_INTF_PARITY_CONTROLr, &imask,
                                  PARITY_ENf, (enable ? 1 : 0));
                soc_reg_field_set(unit, EGR_L3_INTF_PARITY_CONTROLr, &imask,
                                  PARITY_IRQ_ENf, (enable ? 1 : 0));
                SOC_IF_ERROR_RETURN(WRITE_EGR_L3_INTF_PARITY_CONTROLr(unit, imask));
            }
        }
        return SOC_E_NONE;

        case EGR_MASKm:
        if (!SOC_IS_HAWKEYE(unit)) {
            if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
                SOC_IF_ERROR_RETURN(READ_EGR_MASK_PARITY_CONTROLr(unit, &imask));
                soc_reg_field_set(unit, EGR_MASK_PARITY_CONTROLr, &imask,
                                  PARITY_ENf, (enable ? 1 : 0));
                soc_reg_field_set(unit, EGR_MASK_PARITY_CONTROLr, &imask,
                                  PARITY_IRQ_ENf, (enable ? 1 : 0));
                SOC_IF_ERROR_RETURN(WRITE_EGR_MASK_PARITY_CONTROLr(unit, imask));
            }
        }
        return SOC_E_NONE;

        case EGR_L3_NEXT_HOPm:
        if (!SOC_IS_HAWKEYE(unit)) {
            if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
                SOC_IF_ERROR_RETURN(READ_EGR_NEXT_HOP_PARITY_CONTROLr(unit, &imask));
                soc_reg_field_set(unit, EGR_NEXT_HOP_PARITY_CONTROLr, &imask,
                                  PARITY_ENf, (enable ? 1 : 0));
                soc_reg_field_set(unit, EGR_NEXT_HOP_PARITY_CONTROLr, &imask,
                                  PARITY_IRQ_ENf, (enable ? 1 : 0));
                SOC_IF_ERROR_RETURN(WRITE_EGR_NEXT_HOP_PARITY_CONTROLr(unit, imask));
            }
        }
        return SOC_E_NONE;

        case EGR_VLANm:
        if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
            SOC_IF_ERROR_RETURN(READ_EGR_VLAN_TABLE_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, EGR_VLAN_TABLE_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, EGR_VLAN_TABLE_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_EGR_VLAN_TABLE_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;

        case FP_POLICY_TABLEm:
        if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
            SOC_IF_ERROR_RETURN(READ_FP_POLICY_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, FP_POLICY_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, FP_POLICY_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;

        case ING_L3_NEXT_HOPm:
        if (!SOC_IS_HAWKEYE(unit)) {
            if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
                SOC_IF_ERROR_RETURN(READ_ING_L3_NEXT_HOP_PARITY_CONTROLr(unit, &imask));
                soc_reg_field_set(unit, ING_L3_NEXT_HOP_PARITY_CONTROLr, &imask,
                                  PARITY_ENf, (enable ? 1 : 0));
                soc_reg_field_set(unit, ING_L3_NEXT_HOP_PARITY_CONTROLr, &imask,
                                  PARITY_IRQ_ENf, (enable ? 1 : 0));
                SOC_IF_ERROR_RETURN(WRITE_ING_L3_NEXT_HOP_PARITY_CONTROLr(unit, imask));
            }
        }
        return SOC_E_NONE;

        case L2MCm:
        if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
            SOC_IF_ERROR_RETURN(READ_L2MC_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, L2MC_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, L2MC_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_L2MC_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;

        case L3_IPMCm:
        if (!SOC_IS_HAWKEYE(unit)) {
            if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
                SOC_IF_ERROR_RETURN(READ_L3_IPMC_PARITY_CONTROLr(unit, &imask));
                soc_reg_field_set(unit, L3_IPMC_PARITY_CONTROLr, &imask,
                                  PARITY_ENf, (enable ? 1 : 0));
                soc_reg_field_set(unit, L3_IPMC_PARITY_CONTROLr, &imask,
                                  PARITY_IRQ_ENf, (enable ? 1 : 0));
                SOC_IF_ERROR_RETURN(WRITE_L3_IPMC_PARITY_CONTROLr(unit, imask));
            }
        }
        return SOC_E_NONE;

        case VLAN_TABm:
        if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
            SOC_IF_ERROR_RETURN(READ_VLAN_PARITY_CONTROLr(unit, &imask));
            soc_reg_field_set(unit, VLAN_PARITY_CONTROLr, &imask,
                              PARITY_ENf, (enable ? 1 : 0));
            soc_reg_field_set(unit, VLAN_PARITY_CONTROLr, &imask,
                              PARITY_IRQ_ENf, (enable ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_VLAN_PARITY_CONTROLr(unit, imask));
        }
        return SOC_E_NONE;
#endif /* BCM_RAVEN_SUPPORT */

        default:
            break;    /* Look at MMU memories below */
    }

    SOC_IF_ERROR_RETURN(READ_MEMFAILINTMASKr(unit, &imask));
    oimask = imask;
    /* MISCCONFIGr has PARITY_CHECK_EN & CELLCRCCHECKEN control */
    SOC_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &misc_cfg));
    omisc_cfg = misc_cfg;

    switch (mem) {
        /* Covered by parity */
        case MMU_CFAPm:
            soc_reg_field_set(unit, MEMFAILINTMASKr, &imask,
                              CFAPPARITYERRORINTMASKf, (enable ? 1 : 0));
            break;

        case MMU_CCPm:
            soc_reg_field_set(unit, MEMFAILINTMASKr, &imask,
                              CCPPARITYERRORINTMASKf, (enable ? 1 : 0));
            break;

        case MMU_CBPPKTHEADER0m:
        case MMU_CBPPKTHEADER1m:
            soc_reg_field_set(unit, MEMFAILINTMASKr, &imask,
                              CBPPKTHDRPARITYERRORINTMASKf, (enable ? 1 : 0));
            break;

        case MMU_CBPCELLHEADERm:
            soc_reg_field_set(unit, MEMFAILINTMASKr, &imask,
                              CBPCELLHDRPARITYERRORINTMASKf, (enable ? 1 : 0));
            break;

        case MMU_XQ0m:
        case MMU_XQ1m:
        case MMU_XQ2m:
        case MMU_XQ3m:
        case MMU_XQ4m:
        case MMU_XQ5m:
        case MMU_XQ6m:
        case MMU_XQ7m:
        case MMU_XQ8m:
        case MMU_XQ9m:
        case MMU_XQ10m:
        case MMU_XQ11m:
        case MMU_XQ12m:
        case MMU_XQ13m:
        case MMU_XQ14m:
        case MMU_XQ15m:
        case MMU_XQ16m:
        case MMU_XQ17m:
        case MMU_XQ18m:
        case MMU_XQ19m:
        case MMU_XQ20m:
        case MMU_XQ21m:
        case MMU_XQ22m:
        case MMU_XQ23m:
        case MMU_XQ24m:
        case MMU_XQ25m:
        case MMU_XQ26m:
        case MMU_XQ27m:
        case MMU_XQ28m:
#if defined(BCM_RAPTOR_SUPPORT)
        case MMU_XQ29m:
        case MMU_XQ30m:
        case MMU_XQ31m:
        case MMU_XQ32m:
        case MMU_XQ33m:
        case MMU_XQ34m:
        case MMU_XQ35m:
        case MMU_XQ36m:
        case MMU_XQ37m:
        case MMU_XQ38m:
        case MMU_XQ39m:
        case MMU_XQ40m:
        case MMU_XQ41m:
        case MMU_XQ42m:
        case MMU_XQ43m:
        case MMU_XQ44m:
        case MMU_XQ45m:
        case MMU_XQ46m:
        case MMU_XQ47m:
        case MMU_XQ48m:
        case MMU_XQ49m:
        case MMU_XQ50m:
        case MMU_XQ51m:
        case MMU_XQ52m:
        case MMU_XQ53m:
#endif /* BCM_RAPTOR_SUPPORT */
            soc_reg_field_set(unit, MEMFAILINTMASKr, &imask,
                              XQPARITYERRORINTMASKf, (enable ? 1 : 0));
            break;

        /* Covered by CRC */
        case MMU_CBPDATA0m:
        case MMU_CBPDATA1m:
        case MMU_CBPDATA2m:
        case MMU_CBPDATA3m:
        case MMU_CBPDATA4m:
        case MMU_CBPDATA5m:
        case MMU_CBPDATA6m:
        case MMU_CBPDATA7m:
        case MMU_CBPDATA8m:
        case MMU_CBPDATA9m:
        case MMU_CBPDATA10m:
        case MMU_CBPDATA11m:
        case MMU_CBPDATA12m:
        case MMU_CBPDATA13m:
        case MMU_CBPDATA14m:
        case MMU_CBPDATA15m:
            soc_reg_field_set(unit, MEMFAILINTMASKr, &imask,
                              CRCERRORINTMASKf, (enable ? 1 : 0));
            parity_f = CELLCRCCHECKENf;
            break;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_HAWKEYE_SUPPORT)
        case MMU_MIN_BUCKET_GPORTm:
        case MMU_MAX_BUCKET_GPORTm:
            soc_reg_field_set(unit, MISCCONFIGr, &misc_cfg,
                              METERING_CLK_ENf, (enable ? 1 : 0));
            break;
#endif /* BCM_FIREBOLT2_SUPPORT */
        default:
            return SOC_E_NONE;    /* do nothing */
    }
    if (oimask != imask) {
        SOC_IF_ERROR_RETURN(WRITE_MEMFAILINTMASKr(unit, imask));
    }
    soc_reg_field_set(unit, MISCCONFIGr, &misc_cfg,
                      parity_f, (enable ? 1 : 0));
    if (omisc_cfg != misc_cfg) {
        SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, misc_cfg));
    }
    return SOC_E_NONE;
}
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
STATIC int
_soc_shadow_mem_parity_control(int unit, soc_mem_t mem, int copyno, int enable)
{

    COMPILER_REFERENCE(copyno);

    switch (mem) {
        case ING_VLAN_RANGEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IPARS_DBGCTRLr, REG_PORT_ANY, 
                                        VLR0_ENABLE_ECCf, (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IPARS_DBGCTRLr, REG_PORT_ANY, 
                                        VLR1_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case SOURCE_TRUNK_MAP_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IPARS_DBGCTRLr, REG_PORT_ANY, 
                                        STM_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case UDF_OFFSETm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IPARS_DBGCTRLr, REG_PORT_ANY, 
                                        UDF_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case VLAN_TABm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IVLAN_DBGCTRLr, REG_PORT_ANY, 
                                        VLAN_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case STG_TABm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IVLAN_DBGCTRLr, REG_PORT_ANY, 
                                        VLAN_STG_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case VLAN_SUBNETm:
        case VLAN_SUBNET_ONLYm:
        case VLAN_SUBNET_DATA_ONLYm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit,  VXLT_DEBUG_CONTROL_0r, 
                                        REG_PORT_ANY, VLAN_SUBNET_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit,  VXLT_DEBUG_CONTROL_0r, 
                                        REG_PORT_ANY, VLAN_SUBNET_DATA_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case VLAN_XLATEm:
        case VLAN_MACm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_2_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_3_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_4_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_5_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_6_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_XLATE_MEM_7_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case VLAN_PROTOCOL_DATAm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, VXLT_DEBUG_CONTROL_1r, 
                                        REG_PORT_ANY, VLAN_PROTOCOL_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case DSCP_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW1_ECC_DEBUGr, REG_PORT_ANY, 
                                        DSCP_TABLE_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case UFLOW_Bm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW1_ECC_DEBUGr, REG_PORT_ANY, 
                                        UFLOW_B_0_ENABLE_ECCf, (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW1_ECC_DEBUGr, REG_PORT_ANY, 
                                        UFLOW_B_1_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case UFLOW_Am:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW1_ECC_DEBUGr, REG_PORT_ANY, 
                                        UFLOW_A_0_ENABLE_ECCf, (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW1_ECC_DEBUGr, REG_PORT_ANY, 
                                        UFLOW_A_1_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case FP_TCAMm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_DEBUG_CONTROLr, REG_PORT_ANY, 
                                        TCAM_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case FP_METER_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_DEBUG_CONTROLr, REG_PORT_ANY, 
                                        REFRESH_ENABLEf, (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_EVEN_SLICE_0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_EVEN_SLICE_1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_EVEN_SLICE_2_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_EVEN_SLICE_3_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_ODD_SLICE_0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_ODD_SLICE_1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_ODD_SLICE_2_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_1r, REG_PORT_ANY, 
                                        METER_ODD_SLICE_3_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case FP_COUNTER_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        COUNTER_SLICE_0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        COUNTER_SLICE_1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        COUNTER_SLICE_2_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        COUNTER_SLICE_3_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case FP_STORM_CONTROL_METERSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        STORM_ENABLE_ECCf, (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_DEBUG_CONTROLr, REG_PORT_ANY, 
                                        REFRESH_ENABLEf, (enable ? 1 : 0))); 
            break;
        case FP_POLICY_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        POLICY_SLICE_0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        POLICY_SLICE_1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        POLICY_SLICE_2_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, FP_RAM_CONTROL_64r, REG_PORT_ANY, 
                                        POLICY_SLICE_3_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case CELL_BUFFER3m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        CELL_BUFFER3_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case CELL_BUFFER2m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        CELL_BUFFER2_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case CELL_BUFFER1m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        CELL_BUFFER1_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case CELL_BUFFER0m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        CELL_BUFFER0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case DEBUG_CAPTUREm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        DEBUG_CAPTURE_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case VLAN_PROFILE_2m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        VLAN_PROFILE_2_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case EGR_MASKm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        EGR_MASK_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case TRUNK_BITMAPm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG0r, REG_PORT_ANY, 
                                        TRUNK_BITMAP_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case E2E_HOL_STATUSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG1r, REG_PORT_ANY, 
                                        E2E_HOL_STATUS3_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG1r, REG_PORT_ANY, 
                                        E2E_HOL_STATUS2_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG1r, REG_PORT_ANY, 
                                        E2E_HOL_STATUS1_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ISW2_DEBUG1r, REG_PORT_ANY, 
                                        E2E_HOL_STATUS0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case EGR_VLANm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EVLAN_ECC_DEBUGr, REG_PORT_ANY, 
                                        EP_VLAN_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case EGR_VLAN_STGm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EVLAN_ECC_DEBUGr, 
                                        REG_PORT_ANY, EP_VLAN_STG_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case EGR_VLAN_XLATEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EVXLT_ECC_DEBUGr, REG_PORT_ANY, 
                                        EP_VLAN_XLATE_U_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EVXLT_ECC_DEBUGr, REG_PORT_ANY, 
                                        EP_VLAN_XLATE_L_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case EGR_PRI_CNG_MAPm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EVXLT_ECC_DEBUGr, REG_PORT_ANY, 
                                        EP_PRI_CNG_MAP_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case EGR_DSCP_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EVXLT_ECC_DEBUGr, REG_PORT_ANY, 
                                        EP_DSCP_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case EGR_SCI_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EHCPM_ECC_DEBUGr, REG_PORT_ANY, 
                                        EP_SCI_TABLE_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case EGR_PERQ_XMT_COUNTERSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_EDB_ECC_DEBUGr, REG_PORT_ANY, 
                                        EP_PERQ_XMT_COUNTER_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_CFAP_MEMm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        CFAP_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_PKTLINK0m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        PKTLINK_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_CELLLINKm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        CELLLINK_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_CBPDATA0m:
        case MMU_CBPDATA1m:
        case MMU_CBPDATA2m:
        case MMU_CBPDATA3m:
        case MMU_CBPDATA4m:
        case MMU_CBPDATA5m:
        case MMU_CBPDATA6m:
        case MMU_CBPDATA7m:
        case MMU_CBPDATA8m:
        case MMU_CBPDATA9m:
        case MMU_CBPDATA10m:
        case MMU_CBPDATA11m:
        case MMU_CBPDATA12m:
        case MMU_CBPDATA13m:
        case MMU_CBPDATA14m:
        case MMU_CBPDATA15m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        CELLDATA_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_CBPPKTLENGTHm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        PKTLENGTH_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_CBPPKTHEADER0_MEM0m:
        case MMU_CBPPKTHEADER0_MEM1m:
        case MMU_CBPPKTHEADER0_MEM2m:
        case MMU_CBPPKTHEADER0_MEM3m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        PKTHDR0_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_AGING_EXPm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, PKTAGINGTIMERr, REG_PORT_ANY, 
                                        DURATIONSELECTf, (enable ? 0x1000 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        AGING_EXP_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_AGING_CTRm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, PKTAGINGTIMERr, REG_PORT_ANY, 
                                        DURATIONSELECTf, (enable ? 0x1000 : 0))); 
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        AGING_CTR_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case ES_ARB_TDM_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG0r, REG_PORT_ANY, 
                                        ES_ARB_TDM_TABLE_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case CTR_MEMm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        CTR_MEM_ENABLE_ECCf, (enable ? 1 : 0))); 
            break;
        case MMU_WRED_PORT_THD_0_CELLm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        WRED_PORT_THD_0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_WRED_PORT_THD_1_CELLm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        WRED_PORT_THD_1_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_WRED_THD_0_CELLm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        WRED_THD_0_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_WRED_THD_1_CELLm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        WRED_THD_1_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_WRED_PORT_CFG_CELLm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        WRED_PORT_CFG_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_WRED_CFG_CELLm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        WRED_CFG_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
/*        case MMU_MTRO_CPU_PKT_SHAPING_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_CPU_PKT_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G0_BUCKET_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G0_BUCKET_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G0_CONFIG_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G0_CONFIG_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G1_BUCKET_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G1_BUCKET_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G1_CONFIG_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G1_CONFIG_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G2_BUCKET_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G2_BUCKET_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G2_CONFIG_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G2_CONFIG_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G3_BUCKET_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G3_BUCKET_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;
        case MMU_MTRO_SHAPE_G3_CONFIG_WRAPPERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY, 
                                        MTRO_SHAPE_G3_CONFIG_ENABLE_ECCf, 
                                        (enable ? 1 : 0))); 
            break;*/
        case ESEC_PKT_HEADER_CAPTURE_BUFFERm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        DEBUGFF_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        DEBUGFF_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case QL_TABLE0m:
        case QL_TABLE1m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, MISCCONFIGr, REG_PORT_ANY, 
                                        REFRESH_ENf, (enable ? 1 : 0)));
            break;
        case ESEC_SC_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        SCDB_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        SCDB_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case ESEC_SA_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        SADB_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        SADB_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case ESEC_SA_KEY_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        SAKEY_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        SAKEY_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case OUTUNCTRLUCASTPKTSm:
        case OUTUNCTRLMCASTPKTSm:
        case OUTUNCTRLBCASTPKTSm:
        case OUTCTRLMCASTPKTSm:    
        case OUTCTRLUCASTPKTSm:
        case OUTCTRLBCASTPKTSm:        
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        MIB1_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        MIB1_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case TXSAPRTCPKTSm:
        case TXSACRPTPKTSm:        
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        MIB2_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        MIB2_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case TXSCPRTCPKTSm:
        case TXSCCRPTPKTSm:        
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        MIB3_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        MIB3_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case TXSCPRTCBYTm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        MIB4_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        MIB4_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case TXSCCRPTBYTm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 1, 
                                        MIB5_ENABLE_ECCf, (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ESEC_ECC_DEBUGr, 5, 
                                        MIB5_ENABLE_ECCf, (enable ? 1 : 0)));
            break;
        case IL_STAT_MEM_0m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 9, 
                                        IL_RX_STAT0_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 13, 
                                        IL_RX_STAT0_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 9, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 13, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            break;
        case IL_STAT_MEM_1m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 9, 
                                        IL_RX_STAT1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 13, 
                                        IL_RX_STAT1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 9, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 13, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            break;
        case IL_STAT_MEM_2m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 9, 
                                        IL_RX_STAT2_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 13, 
                                        IL_RX_STAT2_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 9, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 13, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            break;
        case IL_STAT_MEM_3m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 9, 
                                        IL_TX_STAT0_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 13, 
                                        IL_TX_STAT0_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 9, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 13, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            break;
        case IL_STAT_MEM_4m:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 9, 
                                        IL_TX_STAT1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_ECC_DEBUG0r, 13, 
                                        IL_TX_STAT1_ENABLE_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 9, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IL_GLOBAL_CONFIGr, 13, 
                                        XGS_COUNTER_COMPAT_MODEf, 
                                        (enable ? 1 : 0)));
            break;
        case ISEC_SA_TABLEm:
        case ISEC_SA_KEY_TABLEm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, SA1_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, SA1_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, SA2_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, SA2_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSAUNUSEDSAPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSASTATSUNUSEDSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSASTATSUNUSEDSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSANOTUSINGSAPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSASTATSNOTUSINGSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSASTATSNOTUSINGSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSANOTVLDPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSASTATSNOTVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSASTATSNOTVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSAINVLDPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSASTATSINVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSASTATSINVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSAOKPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSASTATSOKPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSASTATSOKPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case INCTRLUCASTPKTSm:
        case INCTRLMCASTPKTSm:
        case INCTRLBCASTPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXPACKETTYPE_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXPACKETTYPE_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case INCTRLDISCPKTSm:
        case INCTRLERRPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXERRDSCRDSPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXERRDSCRDSPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break; 
        case RXUNTAGPKTSm:
        case RXNOTAGPKTSm:
        case RXBADTAGPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXTAGUNTAGNONEBAD_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXTAGUNTAGNONEBAD_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;  
        case RXUNKNOWNSCIPKTSm:
        case RXNOSCIPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCIUNKNOWNNONEPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCIUNKNOWNNONEPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break; 
        case RXSCUNUSEDSAPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1,
                                        RXSCSTATSUNUSEDSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSUNUSEDSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCNOTUSINGSAPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSNOTUSINGSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSNOTUSINGSAPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCLATEPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSLATEPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSLATEPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCNOTVLDPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSNOTVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSNOTVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCINVLDPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSINVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSINVALIDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCDLYPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSDELAYEDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSDELAYEDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCUNCHKPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSUNCHECKEDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSUNCHECKEDPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCOKPKTSm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSOKPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSOKPKTS_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCVLDTBYTm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSOCTETSVALIDATED_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSOCTETSVALIDATED_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case RXSCDCRPTBYTm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        RXSCSTATSOCTETSDECRYPTED_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        RXSCSTATSOCTETSDECRYPTED_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case FILTERMATCHCOUNTm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, 
                                        FILTERMATCHCOUNT_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, 
                                        FILTERMATCHCOUNT_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        case ISEC_DEBUG_RAMm:
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 1, DEBUGRAM_EN_ECCf, 
                                        (enable ? 1 : 0)));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, DEBUG20r, 5, DEBUGRAM_EN_ECCf, 
                                        (enable ? 1 : 0)));
            break;
        default:
            return SOC_E_NONE;    /* do nothing */
    }
    return SOC_E_NONE;
}
#endif

#ifdef BCM_BRADLEY_SUPPORT
STATIC int
_soc_hb_mem_parity_control(int unit, soc_mem_t mem, int copyno, int enable)
{
    uint32 rval, orval, imask, oimask,
           misc_cfg, sbs_ctrl;
    soc_reg_t mctlreg = INVALIDr, sbsreg = INVALIDr;
    soc_field_t imask_f[4] = { INVALIDf, INVALIDf, INVALIDf, INVALIDf };
    int en_val = enable ? 1 : 0;
    int i;

    COMPILER_REFERENCE(copyno);

    switch (mem) {
    case VLAN_TABm:
        if (!SOC_IS_SHADOW(unit)) {
            mctlreg = VLAN_PARITY_CONTROLr;
            imask_f[0] = TOQ0_VLAN_TBL_PAR_ERR_ENf;
            imask_f[1] = TOQ1_VLAN_TBL_PAR_ERR_ENf;
            imask_f[2] = MEM1_VLAN_TBL_PAR_ERR_ENf;
            sbsreg = SBS_CONTROLr;
        }
        break;
    case L2Xm:
    case L2_ENTRY_ONLYm:
#ifdef BCM_SCORPION_SUPPORT
    case L2_ENTRY_SCRATCHm:
#endif
        mctlreg = L2_ENTRY_PARITY_CONTROLr;
        break;
    case L2MCm:
        mctlreg = L2MC_PARITY_CONTROLr;
        sbsreg = SBS_CONTROLr;
        break;
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_VALID_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        mctlreg = L3_ENTRY_PARITY_CONTROLr;
        break;
    case EGR_L3_INTFm:
        mctlreg = EGR_L3_INTF_PARITY_CONTROLr;
        sbsreg = EGR_SBS_CONTROLr;
        break;
    case EGR_L3_NEXT_HOPm:
        mctlreg = EGR_L3_NEXT_HOP_PARITY_CONTROLr;
        sbsreg = EGR_SBS_CONTROLr;
        break;
    case EGR_VLANm:
        if (!SOC_IS_SHADOW(unit)) {
            mctlreg = EGR_VLAN_PARITY_CONTROLr;
            sbsreg = EGR_SBS_CONTROLr;
        }
        break;
#ifdef BCM_SCORPION_SUPPORT
    case SOURCE_TRUNK_MAP_TABLEm:
        if (SOC_IS_SC_CQ(unit) && !SOC_IS_SHADOW(unit)) {
            mctlreg = SRC_TRUNK_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case VLAN_MACm:
    case VLAN_XLATEm:
        if (SOC_IS_SC_CQ(unit) && !SOC_IS_SHADOW(unit)) {
            mctlreg = VLAN_MAC_OR_XLATE_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case VFP_POLICY_TABLEm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = VFP_POLICY_TABLE_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case L3_DEFIPm:
    case L3_DEFIP_DATA_ONLYm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg =  L3_DEFIP_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case INITIAL_ING_L3_NEXT_HOPm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = INITIAL_ING_L3_NEXT_HOP_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case FP_POLICY_TABLEm:
        if (SOC_IS_SC_CQ(unit) && !SOC_IS_SHADOW(unit)) {
            mctlreg = IFP_POLICY_TABLE_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case ING_L3_NEXT_HOPm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = ING_L3_NEXT_HOP_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case EGR_MASKm:
        if (SOC_IS_SC_CQ(unit) && !SOC_IS_SHADOW(unit)) {
            mctlreg = EGR_MASK_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case SRC_MODID_BLOCKm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = SRC_MODID_BLOCK_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case MODPORT_MAPm:
    case MODPORT_MAP_SWm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = MODPORT_MAP_SW_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case MODPORT_MAP_IMm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = MODPORT_MAP_IM_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case MODPORT_MAP_EMm:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = MODPORT_MAP_EM_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
    case EGR_VLAN_XLATEm:
        if (SOC_IS_SC_CQ(unit) && !SOC_IS_SHADOW(unit)) {
            mctlreg = EGR_VLAN_XLATE_PARITY_CONTROLr;
            sbsreg = EGR_SBS_CONTROLr;
        }
        break;
    case EGR_IP_TUNNELm:
    case EGR_IP_TUNNEL_IPV6m:
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = EGR_IP_TUNNEL_PARITY_CONTROLr;
            sbsreg = EGR_SBS_CONTROLr;
        } /* No parity in Bradley */
        break;
#endif
    case MMU_AGING_CTRm:
        imask_f[0] = AGING_CTR_PAR_ERR_ENf;
        break;
    case MMU_AGING_EXPm:
        imask_f[0] = AGING_EXP_PAR_ERR_ENf;
        break;
    case L3_IPMCm:
#ifdef BCM_SCORPION_SUPPORT
        if (SOC_IS_SC_CQ(unit)) {
            mctlreg = L3MC_PARITY_CONTROLr;
            sbsreg = SBS_CONTROLr;
        }
        /* Fall through */
#endif
    case MMU_IPMC_VLAN_TBLm:
    case MMU_IPMC_VLAN_TBL_MEM0m:
    case MMU_IPMC_VLAN_TBL_MEM1m:
    case MMU_IPMC_GROUP_TBL0m:
    case MMU_IPMC_GROUP_TBL1m:
    case MMU_IPMC_GROUP_TBL2m:
    case MMU_IPMC_GROUP_TBL3m:
#ifdef BCM_SCORPION_SUPPORT
    case MMU_IPMC_GROUP_TBL4m:
    case MMU_IPMC_GROUP_TBL5m:
    case MMU_IPMC_GROUP_TBL6m:
#endif
        imask_f[0] = TOQ0_IPMC_TBL_PAR_ERR_ENf;
        imask_f[1] = TOQ1_IPMC_TBL_PAR_ERR_ENf;
        imask_f[2] = MEM1_IPMC_TBL_PAR_ERR_ENf;
        imask_f[3] = ENQ_IPMC_TBL_PAR_ERR_ENf;
        break;
    case MMU_CFAP_MEMm :
        imask_f[0] = CFAP_PAR_ERR_ENf;
        break;
    case MMU_CCP_MEMm :
        imask_f[0] = CCP_PAR_ERR_ENf;
        break;
    case MMU_CELLLINKm:
        imask_f[0] = TOQ0_CELLLINK_PAR_ERR_ENf;
        imask_f[1] = TOQ1_CELLLINK_PAR_ERR_ENf;
        break;
    case MMU_PKTLINK0m:
    case MMU_PKTLINK1m:
    case MMU_PKTLINK2m:
    case MMU_PKTLINK3m:
    case MMU_PKTLINK4m:
    case MMU_PKTLINK5m:
    case MMU_PKTLINK6m:
    case MMU_PKTLINK7m:
    case MMU_PKTLINK8m:
    case MMU_PKTLINK9m:
    case MMU_PKTLINK10m:
    case MMU_PKTLINK11m:
    case MMU_PKTLINK12m:
    case MMU_PKTLINK13m:
    case MMU_PKTLINK14m:
    case MMU_PKTLINK15m:
    case MMU_PKTLINK16m:
    case MMU_PKTLINK17m:
    case MMU_PKTLINK18m:
    case MMU_PKTLINK19m:
    case MMU_PKTLINK20m:
#ifdef BCM_TRX_SUPPORT
    case MMU_PKTLINK21m:
    case MMU_PKTLINK22m:
    case MMU_PKTLINK23m:
    case MMU_PKTLINK24m:
    case MMU_PKTLINK25m:
    case MMU_PKTLINK26m:
    case MMU_PKTLINK27m:
    case MMU_PKTLINK28m:
#endif
        imask_f[0] = TOQ0_PKTLINK_PAR_ERR_ENf;
        imask_f[1] = TOQ1_PKTLINK_PAR_ERR_ENf;
        break;
    case MMU_CBPPKTHEADER0_MEM0m:
    case MMU_CBPPKTHEADER0_MEM1m:
    case MMU_CBPPKTHEADER0_MEM2m:
    case MMU_CBPPKTHEADER1_MEM0m:
    case MMU_CBPPKTHEADER1_MEM1m:
    case MMU_CBPPKTHEADERCPUm:
#ifdef BCM_SCORPION_SUPPORT
    case MMU_CBPPKTHEADER0_MEM3m:
#endif
        imask_f[0] = TOQ0_PKTHDR1_PAR_ERR_ENf;
        imask_f[1] = TOQ1_PKTHDR1_PAR_ERR_ENf;
        break;
    case MMU_CBPCELLHEADERm:
        imask_f[0] = TOQ0_CELLHDR_PAR_ERR_ENf;
        imask_f[1] = TOQ1_CELLHDR_PAR_ERR_ENf;
        break;
    case MMU_CBPDATA0m:
    case MMU_CBPDATA1m:
    case MMU_CBPDATA2m:
    case MMU_CBPDATA3m:
    case MMU_CBPDATA4m:
    case MMU_CBPDATA5m:
    case MMU_CBPDATA6m:
    case MMU_CBPDATA7m:
    case MMU_CBPDATA8m:
    case MMU_CBPDATA9m:
    case MMU_CBPDATA10m:
    case MMU_CBPDATA11m:
    case MMU_CBPDATA12m:
    case MMU_CBPDATA13m:
    case MMU_CBPDATA14m:
    case MMU_CBPDATA15m:
        /* Covered by CRC */
        imask_f[0] = DEQ0_CELLCRC_ERR_ENf;
        imask_f[1] = DEQ1_CELLCRC_ERR_ENf;
        break;
    case MMU_CBPPKTLENGTHm:
        /* Covered by CRC */
        imask_f[0] = DEQ0_LENGTH_PAR_ERR_ENf;
        imask_f[1] = DEQ1_LENGTH_PAR_ERR_ENf;
        break;

    case MMU_WRED_CFG_CELLm:
    case MMU_WRED_PORT_CFG_CELLm:
    case MMU_WRED_PORT_THD_0_CELLm:
    case MMU_WRED_PORT_THD_1_CELLm:
    case MMU_WRED_THD_0_CELLm:
    case MMU_WRED_THD_1_CELLm:
        imask_f[0] = WRED_PAR_ERR_ENf;
        break;
    default:
        /* Do nothing */
        return SOC_E_NONE;
    }

    /*
     * Turn off parity
     */
    if (mctlreg != INVALIDr) {
        /* Memory has a separate parity control */
        SOC_IF_ERROR_RETURN(soc_reg32_get(unit, mctlreg, REG_PORT_ANY, 0, &rval));
        orval = rval;
        soc_reg_field_set(unit, mctlreg, &rval, PARITY_ENf, en_val);
        soc_reg_field_set(unit, mctlreg, &rval, PARITY_IRQ_ENf, en_val);
        if (orval != rval) {
            SOC_IF_ERROR_RETURN(soc_reg32_set(unit, mctlreg, REG_PORT_ANY, 0, rval));
        }

        if ((sbsreg != INVALIDr) && (!SOC_IS_SHADOW(unit))) { /* Dual pipe controls needed */
            sbs_ctrl = 0;
            soc_reg_field_set(unit, sbsreg, &sbs_ctrl, PIPE_SELECTf,
                              SOC_PIPE_SELECT_Y);
            SOC_IF_ERROR_RETURN(soc_reg32_set(unit, sbsreg, REG_PORT_ANY, 0, sbs_ctrl));
            
            SOC_IF_ERROR_RETURN(soc_reg32_get(unit, mctlreg, REG_PORT_ANY, 0, &rval));
            orval = rval;
            soc_reg_field_set(unit, mctlreg, &rval, PARITY_ENf, en_val);
            soc_reg_field_set(unit, mctlreg, &rval, PARITY_IRQ_ENf, en_val);
            if (orval != rval) {
                SOC_IF_ERROR_RETURN(soc_reg32_set(unit, mctlreg, REG_PORT_ANY, 0, rval));
            }

            soc_reg_field_set(unit, sbsreg, &sbs_ctrl, PIPE_SELECTf,
                              SOC_PIPE_SELECT_X);
            SOC_IF_ERROR_RETURN(soc_reg32_set(unit, sbsreg, REG_PORT_ANY, 0, sbs_ctrl));
        }
    } else {
        /* Memory does not have a separate parity control */
        SOC_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &misc_cfg));
        soc_reg_field_set(unit, MISCCONFIGr, &misc_cfg, PARITY_STAT_CLEARf, 1);
        SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, misc_cfg));

        soc_reg_field_set(unit, MISCCONFIGr, &misc_cfg, PARITY_STAT_CLEARf, 0);
        if (soc_reg_field_valid(unit, MISCCONFIGr, PARITY_GEN_ENf)) {
            soc_reg_field_set(unit, MISCCONFIGr, &misc_cfg, PARITY_GEN_ENf, en_val);
            soc_reg_field_set(unit, MISCCONFIGr, &misc_cfg, PARITY_CHK_ENf, en_val);
        }
        soc_reg_field_set(unit, MISCCONFIGr, &misc_cfg, REFRESH_ENf, en_val);
        SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, misc_cfg));
    }

    /*
     * Disable parity and CRC interrupts
     */
    if ((imask_f[0] != INVALIDr) && SOC_REG_IS_VALID(unit, MEM_FAIL_INT_ENr)) {
        SOC_IF_ERROR_RETURN(READ_MEM_FAIL_INT_ENr(unit, &imask));
        oimask = imask;
        for (i = 0; i < COUNTOF(imask_f); i++) {
            if (imask_f[i] != INVALIDr) {
                soc_reg_field_set(unit, MEM_FAIL_INT_ENr, &imask, 
                                  imask_f[i], en_val);
            }
        }
        if (oimask != imask) {
            SOC_IF_ERROR_RETURN(WRITE_MEM_FAIL_INT_ENr(unit, imask));
        }
    }

    return SOC_E_NONE;
}
#endif /* BCM_BRADLEY_SUPPORT */

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
/*
 * Toggle parity checks during memory tests
 */
int
soc_mem_parity_control(int unit, soc_mem_t mem, int copyno, int enable)
{

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "soc_mem_parity_control: unit %d memory %s.%s %sable\n"),
                 unit,  SOC_MEM_UFNAME(unit, mem),
                 SOC_BLOCK_NAME(unit, copyno),
                 enable ? "en" : "dis"));
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        return _soc_shadow_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_SHADOW_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        return _soc_trident2_mem_ser_control(unit, mem, copyno, enable);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        return _soc_trident_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        return _soc_triumph3_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit)) {
        return _soc_triumph2_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit)) {
        return _soc_enduro_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_ENDURO_SUPPORT */
#ifdef BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANE(unit)) {
        return _soc_hurricane_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_HURRICANE_SUPPORT */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        return _soc_hurricane2_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_HURRICANE2_SUPPORT */
#ifdef BCM_GREYHOUND_SUPPORT
    if (SOC_IS_GREYHOUND(unit)) {
        return _soc_greyhound_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_GREYHOUND_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        return _soc_katana_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FB_FX_HX(unit)) {
        return _soc_fb_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_BRADLEY_SUPPORT
    if (SOC_IS_HBX(unit)) {
        return _soc_hb_mem_parity_control(unit, mem, copyno, enable);
    }
#endif /* BCM_BRADLEY_SUPPORT */

#ifdef    BCM_HERCULES15_SUPPORT
    if (SOC_IS_HERCULES15(unit)) {
        int     copy;
        int     port;
        uint32  interrupt = 0;
        uint32  regval;

        /* Shut off parity interrupts during memory test */
        switch (mem) {
            case MEM_XQm:
                soc_reg_field_set
                (unit, MMU_INTCTRLr, &interrupt, XQ_PE_ENf, 1);
                break;
            case MEM_PPm:
                soc_reg_field_set
                (unit, MMU_INTCTRLr, &interrupt, PP_DBE_ENf, 1);
                soc_reg_field_set
                (unit, MMU_INTCTRLr, &interrupt, PP_SBE_ENf, 1);
                break;
            case MEM_LLAm:
                soc_reg_field_set
                (unit, MMU_INTCTRLr, &interrupt, LLA_PE_ENf, 1);
                break;
            case MEM_INGBUFm:
            case MEM_MCm:
            case MEM_IPMCm:
            case MEM_UCm:
            case MEM_VIDm:
            case MEM_ING_SRCMODBLKm:
            case MEM_ING_MODMAPm:
            case MEM_EGR_MODMAPm:
                soc_reg_field_set
                (unit, MMU_INTCTRLr, &interrupt, ING_PE_ENf, 1);
                break;
                break;
            default:
                return SOC_E_NONE;    /* nothing to do */
        }
        SOC_MEM_BLOCK_ITER(unit, mem, copy) {
            if (copyno != COPYNO_ALL && copyno != copy) {
                continue;
            }

            /* find the ports matching this copy/blk */
            PBMP_ITER(SOC_BLOCK_BITMAP(unit, copy), port) {
                SOC_IF_ERROR_RETURN
                (READ_MMU_INTCTRLr(unit, port, &regval));
                if (enable) {
                    regval |= interrupt;
                } else {
                    regval &= ~interrupt;
                }
                if (enable) {
                    SOC_IF_ERROR_RETURN
                    (WRITE_MMU_INTCLRr(unit, port, interrupt));
                }
                SOC_IF_ERROR_RETURN
                (WRITE_MMU_INTCTRLr(unit, port, (port == 0) ? 0:regval));
                SOC_IF_ERROR_RETURN(READ_MMU_CFGr(unit, port, &regval));
                soc_reg_field_set(unit, MMU_CFGr, &regval,
                                PARITY_DIAGf, enable ? 0 : 1);
                SOC_IF_ERROR_RETURN(WRITE_MMU_CFGr(unit, port, regval));
            }
        }
    }
#endif    /* BCM_HERCULES15_SUPPORT */

    return SOC_E_NONE;
}

/*
 * Clean up parity bits after memtest to avoid later parity errors
 */
int
soc_mem_parity_clean(int unit, soc_mem_t mem, int copyno)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(mem);
    COMPILER_REFERENCE(copyno);
    /* To avoid later parity errors */
#if defined(BCM_FIREBOLT_SUPPORT)
    switch (mem) {
        case L2Xm:
        case L2_ENTRY_ONLYm:
        if (soc_feature(unit, soc_feature_l2x_parity)) {
            if (soc_mem_clear(unit, mem, copyno, TRUE) < 0) {
                return -1;
            }
        }
        break;

        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_ONLYm:
        case L3_ENTRY_VALID_ONLYm:
        if (soc_feature(unit, soc_feature_l3x_parity)) {
            if (soc_mem_clear(unit, mem, copyno, TRUE) < 0) {
                return -1;
            }
        }
        break;

        case L3_DEFIPm:
        case L3_DEFIP_DATA_ONLYm:
        if (soc_feature(unit, soc_feature_l3defip_parity)) {
            if (soc_mem_clear(unit, mem, copyno, TRUE) < 0) {
                return -1;
            }
        }
        break;

#ifdef BCM_RAPTOR_SUPPORT
        case DSCP_TABLEm:
        case EGR_L3_INTFm:
        case EGR_MASKm:
        case EGR_L3_NEXT_HOPm:
        case EGR_VLANm:
        case FP_POLICY_TABLEm:
        case ING_L3_NEXT_HOPm:
        case L2MCm:
        case L3_IPMCm:
        case VLAN_TABm:
        if (soc_feature(unit, soc_feature_ip_ep_mem_parity)) {
            if (soc_mem_clear(unit, mem, copyno, TRUE) < 0) {
                return -1;
            }
        }
        break;
#endif /* BCM_RAPTOR_SUPPORT */

#ifdef BCM_TRX_SUPPORT
        case MMU_WRED_CFG_CELLm:
        case MMU_WRED_THD_0_CELLm:
        case MMU_WRED_THD_1_CELLm:
        case MMU_WRED_PORT_CFG_CELLm:
        case MMU_WRED_PORT_THD_0_CELLm:
        case MMU_WRED_PORT_THD_1_CELLm:
#ifdef BCM_TRIUMPH_SUPPORT
        case MMU_WRED_CFG_PACKETm:
        case MMU_WRED_THD_0_PACKETm:
        case MMU_WRED_THD_1_PACKETm:
        case MMU_WRED_PORT_CFG_PACKETm:
        case MMU_WRED_PORT_THD_0_PACKETm:
        case MMU_WRED_PORT_THD_1_PACKETm:
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_ENDURO_SUPPORT
        case LMEPm:
#endif /* BCM_ENDURO_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
        case SVM_METER_TABLEm:
        case MMU_INTFO_QCN_CNM_TIMER_TBLm:
        case FP_METER_TABLEm:
        case FP_STORM_CONTROL_METERSm:
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_HURRICANE2_SUPPORT
        case MMU_IPMC_VLAN_TBLm:
        case MMU_IPMC_GROUP_TBL0m:
        case MMU_IPMC_GROUP_TBL1m:
#endif /* BCM_SURRICANE2_SUPPORT */
            if (soc_mem_clear(unit, mem, copyno, TRUE) < 0) {
                return -1;
            }
            break;
#endif /* BCM_TRX_SUPPORT */

        default:
            break; /* Do nothing */
    }
#endif
     return SOC_E_NONE;
}

/* Reenable parity after testing */
int
soc_mem_parity_restore(int unit, soc_mem_t mem, int copyno)
{
    /* 
     * Some devices need memories cleared before reenabling because
     * of background HW processes which scan the tables and throw errors
     */
    if (SOC_IS_TRX(unit)) {
        SOC_IF_ERROR_RETURN
            (soc_mem_parity_clean(unit, mem, copyno));
    }

    SOC_IF_ERROR_RETURN
        (soc_mem_parity_control(unit, mem, copyno, TRUE));

    /* 
     * Other devices need memories cleared after parity is reenabled
     * so the HW generation of parity bits is correct.
     */

    if (!SOC_IS_TRX(unit)) {
        SOC_IF_ERROR_RETURN
            (soc_mem_parity_clean(unit, mem, copyno));
    }

    return 0;
}

int
soc_mem_cpu_write_control(int unit, soc_mem_t mem, int copyno, int enable,
                          int *orig_enable)
{
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        return _soc_trident2_mem_cpu_write_control(unit, mem, copyno, enable,
                                                   orig_enable);
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    return SOC_E_NONE;
}


#if defined (SER_TR_TEST_SUPPORT)
static soc_ser_test_functions_t *ser_test_functions[SOC_MAX_NUM_DEVICES];

/*
 * Function:
 *      ser_test_mem_write
 * Purpose:
 *      A small helper function for ser_test_mem, combining common operations required to write
 * Parameters:
 *      unit        - (IN) Device Number
 *      test_data   - (IN) contains the current state of the test.
 * Returns:
 *      An error if one is generated by the soc_mem_write, otherwise SOC_E_NONE
 */
soc_error_t
ser_test_mem_write(int unit, ser_test_data_t *test_data)
{
    soc_error_t rv = SOC_E_NONE;
    soc_mem_field_set(unit, test_data->mem, test_data->entry_buf,
                      test_data->test_field, test_data->field_buf);
    if (test_data->acc_type != _SOC_ACC_TYPE_PIPE_ANY &&
        test_data->acc_type != _SOC_ACC_TYPE_PIPE_ALL) {
        rv = soc_mem_pipe_select_write(unit, SOC_MEM_NO_FLAGS,
                      test_data->mem, test_data->mem_block, test_data->acc_type,
                      test_data->index, test_data->entry_buf);
    } else {
        rv = soc_mem_write(unit, test_data->mem, test_data->mem_block,
                      test_data->index, test_data->entry_buf);
    }
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "unit %d %s entry %d mem write error\n"),
                   unit, test_data->mem_name, test_data->index));
    }
    return rv;
}


/*
 * Function:
 *      ser_test_mem_read
 * Purpose:
 *      A small helper function for ser_test_mem, combining common operations required to read
 * Parameters:
 *      unit        - (IN) Device Number
 *      test_data   - (IN & OUT) contains the current state of the test.
 *                    The members entry_buf and field_buf will be modified by this function
 * Returns:
 *      An error if one is generated by the soc_mem_read, otherwise SOC_E_NONE
 */
soc_error_t
ser_test_mem_read(int unit, ser_test_data_t *test_data)
{
    soc_error_t rv = SOC_E_NONE;
#ifdef INCLUDE_TCL
    /* If TCL is defined, the cache coherency flag will cause parity information
       to be DISCARDED on read. Turning this off here */
    int coherency_check = SOC_MEM_CACHE_COHERENCY_CHECK(unit);
    SOC_MEM_CACHE_COHERENCY_CHECK_SET(unit, FALSE);
#endif
    if (test_data->acc_type == _SOC_ACC_TYPE_PIPE_Y) {
        rv = soc_mem_pipe_select_read(unit, SOC_MEM_NO_FLAGS, test_data->mem, 
                                      test_data->mem_block,
                                      test_data->acc_type, test_data->index,
                                      test_data->entry_buf);
    } else {
        /*Enable NACK on read */
        rv = soc_mem_read_extended(unit, SOC_MEM_SCHAN_ERR_RETURN,
                                   test_data->mem, 0, test_data->mem_block,
                                   test_data->index, test_data->entry_buf);
    }
#ifdef INCLUDE_TCL
    SOC_MEM_CACHE_COHERENCY_CHECK_SET(unit, coherency_check);
#endif
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "unit %d NACK received for %s entry %d:\n\t"),
                   unit, test_data->mem_name, test_data->index));
    }
    soc_mem_field_get(unit, test_data->mem, test_data->entry_buf,
                          test_data->test_field, test_data->field_buf);
    return rv;
}


soc_error_t
soc_ser_test_inject_full(int unit, uint32 flags, ser_test_data_t *test_data)
{
    soc_field_t key_field, mask_field;
    if ((flags & SOC_INJECT_ERROR_TCAM_FLAG) && test_data->tcam_parity_bit >= 0) {
        if (SOC_MEM_FIELD_VALID(unit, test_data->mem, KEYf)){
            key_field = KEYf;
            mask_field = MASKf;
        } else if (SOC_MEM_FIELD_VALID(unit, test_data->mem, KEY0f)){
            key_field = KEY0f;
            mask_field = MASK0f;
        } else if (SOC_MEM_FIELD_VALID(unit, test_data->mem, KEY1f)){
            key_field = KEY1f;
            mask_field = MASK1f;
        } else {
            return SOC_E_FAIL;
        }
        test_data->test_field = key_field;
        SOC_IF_ERROR_RETURN(soc_ser_test_inject_error(unit, test_data));
        test_data->test_field = mask_field;
        SOC_IF_ERROR_RETURN(ser_test_mem_read(unit, test_data));
        if (test_data->field_buf[0] == 0) {
            return soc_ser_test_inject_error(unit, test_data);
        } else {
            return SOC_E_NONE;
        }

    } else {
        return soc_ser_test_inject_error(unit, test_data);
    }
}

soc_error_t
soc_ser_test_inject_error(int unit, ser_test_data_t *test_data)
{
    if((test_data->field_buf[0] & 1) == 1) {
        test_data->field_buf[0] &= 0xFFFFFFFE;
    } else {
        test_data->field_buf[0] |= 0x00000001;
    }
    test_data->badData = test_data->field_buf[0];
    /*Disable writing to cache for fudged data.*/
    SOC_MEM_TEST_SKIP_CACHE_SET(unit, TRUE);
    SOC_IF_ERROR_RETURN(ser_test_mem_write(unit, test_data));
    SOC_MEM_TEST_SKIP_CACHE_SET(unit, FALSE);
    return SOC_E_NONE;
}

/*
 * Function:
 *      ser_test_mem
 * Purpose:
 *      Serform a Software Error Recovery test on passed memory mem.
 * Parameters:
 *      unit                 - (IN) Device Number
 *      parity_enable_reg    - (IN) Register which turns on and off memory protection
 *      tcam_parity_bit      - (IN) Bit in the parity_enable_reg field for TCAM parity for this memory.
 *                             This must be -1 if this memory is not protected by the TCAM
 *      hw_parity_field      - (IN) If controlled by H/W, this is the field in the enable register that
 *                             coorisponds to this memory.
 *        mem                - (IN) The memory on which to test SER
 *      test_type            - (IN) Used to determine how many indexes to test for each memory table.
 *      mem_block            - (IN) The block for this memory.  MEM_BLOCK_ANY is fine for TCAM mems
 *      port                 - (IN) The port for this memory/control reg.  REG_PORT_ANY is fine for TCAMS
 *      error_count          - (OUT)Increments if a test on any of the indexes in this memory fail.
 * Returns:
 *      The number of failed tests (This can be more than one per memory if test_type is anything
 *      other than SER_SINGLE_INDEX.)
 */


soc_error_t
ser_test_mem_pipe(int unit, soc_reg_t parity_enable_reg, int tcam_parity_bit,
                            soc_field_t hw_parity_field, soc_mem_t mem, soc_field_t test_field,
                            _soc_ser_test_t test_type, soc_block_t mem_block,
                            soc_port_t port, soc_acc_type_t acc_type, int *error_count)
{
    uint32 tmp_entry[SOC_MAX_MEM_WORDS], field_data[SOC_MAX_REG_FIELD_WORDS];
    ser_test_data_t test_data;
    soc_ser_create_test_data(unit, tmp_entry, field_data, parity_enable_reg,
                             tcam_parity_bit, hw_parity_field, mem, test_field,
                             mem_block, port, acc_type, 0, &test_data);
    return ser_test_mem(unit, &test_data, test_type, error_count);
}

/*
 * Function:
 *      soc_ser_create_test_data
 * Purpose:
 *      Populates and performs data verification on a ser_test_data_t structure.  This was
 *      broken out into a separate function to avoid performing this work an excessive number
 *      of times.
 * Parameters:
 *      unit                 - (IN) Device Number
 *      tmp_entry            - (IN) A pointer to a uint32 array of size SOC_MEM_MAX_WORDS
 *      fieldData            - (IN) A pointer to a uint 32 array of size [SOC_MAX_REG_FIELD_WORDS]
 *      parity_enable_reg    - (IN) Register which turns on and off memory protection
 *      tcam_parity_bit      - (IN) Bit in the parity_enable_reg field for TCAM parity
                                    for this memory. This must be INVALID_TCAM_PARITY_BIT if
                                    this memory is not a TCAM protected memory.
 *      hw_parity_field      - (IN) If controlled by H/W, this is the field in the enable
                                    register that coorisponds to this memory.
 *      mem                  - (IN) The memory ID on which to test SER
 *      mem_block            - (IN) The block for this memory.
                                    MEM_BLOCK_ANY is fine for TCAM mems
 *      port                 - (IN) The port for this memory/control reg.
                                    REG_PORT_ANY is fine for TCAMS
 *      acc_type             - (IN) Access type for this memory.
 *      index                - (IN) The entry index on which to test SER
 *      test_data            - (OUT)A pointer to a data structure cotaining SER test info.
 * Returns:
 *      SOC_E_NONE if the test passes, and error if it does
 */
void
soc_ser_create_test_data(int unit, uint32 *tmp_entry, uint32 *field_data,
                         soc_reg_t parity_enable_reg, int tcam_parity_bit,
                         soc_field_t hw_parity_field, soc_mem_t mem,
                         soc_field_t test_field, soc_block_t mem_block,
                         soc_port_t port, soc_acc_type_t acc_type,
                         int index, ser_test_data_t *test_data)
{
    uint16 field_num = 0;

    test_data->mem = mem;
    test_data->parity_enable_reg = parity_enable_reg;
    test_data->parity_enable_field = hw_parity_field;
    test_data->tcam_parity_bit = tcam_parity_bit;
    test_data->mem_block = mem_block;
    test_data->port = port;
    test_data->mem_info = &SOC_MEM_INFO(unit, mem);
    /* An INVALIDf passed in for test_field implies we should select
     * any valid field. */
    if ((test_field == INVALIDf || test_field == 0) &&
        (test_data->mem_info != NULL)) {
        field_num = test_data->mem_info->nFields;
        test_data->test_field = test_data->mem_info->fields[field_num - 1].field;
    } else {
        test_data->test_field = test_field;
    }
    /* If an invalid test field was selected, attempt to replace it
     * with a valid one. */
    if (!SOC_MEM_FIELD_VALID(unit, test_data->mem, test_data->test_field)) {
        test_data->test_field = EVEN_PARITYf;
        if (!SOC_MEM_FIELD_VALID(unit, test_data->mem,
                                   test_data->test_field)) {
            test_data->test_field = PARITYf;
            if (!SOC_MEM_FIELD_VALID(unit, test_data->mem,
                                       test_data->test_field)) {
                test_data->test_field = EVEN_PARITY_0f;
                if (!SOC_MEM_FIELD_VALID(unit, test_data->mem,
                                           test_data->test_field) &&
                    (test_data->mem_info != NULL)) {
                    test_data->test_field = test_data->mem_info->fields[0].field;
                }
            }
        }
    }
    test_data->acc_type = acc_type;
    test_data->index = index;
    test_data->entry_buf = tmp_entry;
    test_data->field_buf = field_data;
#ifdef SOC_MEM_NAME
    /* coverity[secure_coding] */
    sal_strcpy(test_data->mem_name,
            SOC_MEM_NAME(unit, test_data->mem));
    /* coverity[secure_coding] */
    sal_strcpy(test_data->field_name,
            SOC_FIELD_NAME(unit, test_data->test_field));
#else
    sprintf(test_data->mem_name, "Mem ID: %d", test_data->mem);
    sprintf(test_data->field_name, "Field ID: %d", test_data->test_field);
#endif
    test_data->badData = 0;
}

int soc_ser_test_long_sleep = FALSE;
/*
 * Function:
 *      ser_test_mem
 * Purpose:
 *      Serform a Software Error Recovery test on passed memory mem.
 * Parameters:
 *      unit                 - (IN) Device Number
 *      test_data            - (IN) Structure which holds the test data and some
                                    intermediate results.
 *      test_type            - (IN) Used to determine how many indexes to test for
                                    each memory table.
 *      error_count          - (OUT)Increments if a test on any of the indexes
                                    in this memory fail.
 * Returns:
 *      SOC_E_NONE if the test passes, an error if it does not
 */
soc_error_t
ser_test_mem(int unit, ser_test_data_t *test_data,
                _soc_ser_test_t test_type, int * error_count)
{
    int numIndexesToTest, j, read_through, startErrorCount;
    int indexesToTest[3];
    indexesToTest[0] = soc_mem_index_min(unit,test_data->mem);
    startErrorCount = *error_count;
    /*Exit without testing if there is no  SER flag set for this memory*/
    if (!(SOC_MEM_INFO(unit,test_data->mem).flags & SOC_MEM_SER_FLAGS)) {
        /*Verbose output: Skipping memory: name*/
        return SOC_E_NONE;
    }
    /*Check that there is a meminfo structure for this memory.*/
    if (!SOC_MEM_IS_VALID(unit, test_data->mem)) {
        LOG_CLI((BSL_META_U(unit,
                            "%s is not a valid memory for this platform. Skipping.\n"),
                 test_data->mem_name));
        return SOC_E_MEMORY;
    }
    switch (test_type) {
    case SER_FIRST_MID_LAST_INDEX:
        indexesToTest[1] = soc_mem_index_max(unit,test_data->mem);
        indexesToTest[2] = (indexesToTest[1] - indexesToTest[0])/2;
        numIndexesToTest = 3;
        break;
    case SER_ALL_INDEXES:
        numIndexesToTest = soc_mem_index_max(unit,test_data->mem) + 1;
        break;
    case SER_SINGLE_INDEX:
        /*FALL THOUGH*/
    default:
        numIndexesToTest = 1;
    };
    for (j = 0; j < numIndexesToTest; j++) {
        if (test_type == SER_ALL_INDEXES){
            test_data->index = j;
        } else {
            test_data->index = indexesToTest[j];
        }
        if (soc_feature(unit, soc_feature_field_slice_size128)) {
            if ((test_data->mem == FP_TCAMm) || 
                (test_data->mem == FP_GLOBAL_MASK_TCAMm)) {
                /* Skip the entries that is not existed. */ 
                if ((test_data->index/64) % 2) {
                    continue;
                }
            }
        }
        /*Read through to the hardware.*/
        read_through = SOC_MEM_FORCE_READ_THROUGH(unit);
        SOC_MEM_FORCE_READ_THROUGH_SET(unit, TRUE);
        SOC_IF_ERROR_RETURN(ser_test_mem_read(unit, test_data));
        /*Disable Parity*/
        SOC_IF_ERROR_RETURN(_ser_test_parity_control(unit, test_data, 0));
        /*Inject error:*/
        SOC_IF_ERROR_RETURN(soc_ser_test_inject_error(unit, test_data));
        /*Read back write to ensure value is still fudged.*/
        SOC_IF_ERROR_RETURN(ser_test_mem_read(unit, test_data));
        if (test_data->badData != test_data->field_buf[0]) {
        LOG_CLI((BSL_META_U(unit,
                            "Injection Error for mem: %s field: %s\n"),
                 test_data->mem_name,
                 SOC_FIELD_NAME(unit,test_data->test_field)));
        (*error_count)++;
        continue;
        }
        /*Enable Parity (Even if it was not enabled before*/
        SOC_IF_ERROR_RETURN(_ser_test_parity_control(unit, test_data, 1));
        if (SAL_BOOT_QUICKTURN) {
            sal_usleep(10000); /* Add delay for QT to trigger parity error */
        }
        /*Read the memory entry with cache disabled, in order to induce the error*/
        (void)ser_test_mem_read(unit, test_data);
        sal_usleep(10000); /*wait for memory and register ops to complete.*/
        if (SAL_BOOT_QUICKTURN) {
            sal_usleep(10000000);    /* add more delay for Quickturn.*/
        } else if (soc_ser_test_long_sleep) {
            sal_usleep(100000);
        }
        /*Read the memory once more to compare it to the old value*/
        SOC_IF_ERROR_RETURN(ser_test_mem_read(unit, test_data));
        SOC_MEM_FORCE_READ_THROUGH_SET(unit, read_through);
        if ((SOC_MEM_INFO(unit, test_data->mem).flags & SOC_MEM_SER_FLAGS) ==
            SOC_MEM_FLAG_SER_ENTRY_CLEAR) {
            if (0 != test_data->field_buf[0]) {
                (*error_count)++;
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "SER failed to clear mem %s index %d\n"),
                           test_data->mem_name, test_data->index));
            }
        }else if (test_data->badData == test_data->field_buf[0]) {
            (*error_count)++;
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "SER failed to correct mem %s index %d field %s\n"),
                       test_data->mem_name, test_data->index,
                       SOC_FIELD_NAME(unit, test_data->test_field)));
        }
        else {
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "SER corrected mem %s index %d\n"),
                         test_data->mem_name, test_data->index));
        }
    }
    if (startErrorCount != *error_count) {
        LOG_CLI((BSL_META_U(unit,
                            "SER failed to correct memory %s acc_type: %d field: %s %d:%d\n"),
                 test_data->mem_name, (int)test_data->acc_type,
                 SOC_FIELD_NAME(unit, test_data->test_field),
                 startErrorCount, *error_count));
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *      _ser_test_parity_control_reg_set
 * Purpose:
 *      Enables/Disables parity for tcam memories using the 'newer' soc_reg_set
 *      call.  This is what is used for Trident_2 and possibly newer devices.
 * Parameters:
 *      unit       - (IN) Device Number
 *      test_data  - (IN) Contains parity registers to set.
 *      enable     - (IN) (Bool) enable parity if true, disable if false
 * Returns:
 *      SOC_E_NONE if no errors are encountered while setting parity.
 */
soc_error_t
_ser_test_parity_control_reg_set(int unit, ser_test_data_t *test_data, int enable)
{
    uint64 regData;
    if (enable) {
        COMPILER_64_SET(regData, 0, 0xFFFF);
    } else {
        COMPILER_64_SET(regData, 0, 0x0);
    }
    return soc_reg_set(unit, test_data->parity_enable_reg, test_data->port, 0, regData);
}


/*
 * Function:
 *      _ser_test_parity_control_pci_write
 * Purpose:
 *      Enables/Disables parity for tcam memories using the less-compatible
 *      soc_pci_write call.  Used for older devices such as Trident and older.
 * Parameters:
 *      unit       - (IN) Device Number
 *      test_data  - (IN) Contains parity registers to set.
 *      enable     - (IN) (Bool) enable parity if true, disable if false
 * Returns:
 *      SOC_E_NONE if no errors are encountered while setting parity.
 */
soc_error_t
_ser_test_parity_control_pci_write(int unit, ser_test_data_t *test_data, int enable)
{
    uint32 regDataOld;
    uint32 regAddr = soc_reg_addr(unit, test_data->parity_enable_reg, REG_PORT_ANY,
                                  test_data->tcam_parity_bit/16);
    if (enable) {
        regDataOld = 0xFFFF;
    } else {
        regDataOld = 0x0;
    }
    return soc_pci_write(unit,regAddr, regDataOld);
}


/*
 * Function:
 *      _ser_test_parity_control
 * Purpose:
 *      Disables/enables parity for the provided register.
 * Parameters:
 *      unit          - (IN) Device Number
 *      test_data     - (IN) Contains parity registers to set.
 *      enable        - (IN) (Bool) enable parity if true, disable if false
 *
 * Returns:
 *      SOC_E_NONE if no errors are encountered while setting parity.
 */
soc_error_t
_ser_test_parity_control(int unit, ser_test_data_t *test_data, int enable)
{
    soc_error_t rv = SOC_E_NONE;
    if (test_data->tcam_parity_bit >= 0) {
        /*This register controls a TCAM memory*/
        if (ser_test_functions[unit] != NULL &&
            ser_test_functions[unit]->parity_control != NULL) {
            rv = (*(ser_test_functions[unit]->parity_control))(unit, test_data,
                    enable);
        } else {
            rv = _ser_test_parity_control_reg_set(unit, test_data, enable);
        }
    } else {
        /*This register is monitored by hardware*/
        rv = soc_reg_field32_modify(unit, test_data->parity_enable_reg,
                                    test_data->port,
                                    test_data->parity_enable_field, enable);
    }
    return rv;
}


/*
 * Function:
 *      ser_test_cmd_generate
 * Purpose:
 *      Creates a test for a memory the user can then run from the commandline
 * Parameters:
 *      unit          - (IN) Device Number
 *      test_data     - (IN) Contains all test attributes required to print the
 *                           test commands
 *
 */
void
ser_test_cmd_generate(int unit, ser_test_data_t *test_data)
{
    char* pipe_str;
    int ypipe = FALSE;
    if (test_data != NULL && test_data->mem_name != NULL &&
        SOC_MEM_FIELD_VALID(unit, test_data->mem, test_data->test_field)) {
        if (test_data->acc_type == _SOC_ACC_TYPE_PIPE_Y) {
            pipe_str = "pipe_y ";
            ypipe = TRUE;
        } else {
            pipe_str = "";
        }
        LOG_CLI((BSL_META_U(unit,
                            "\nCommand line test for memory %s: access_type: %d\n"),
                 test_data->mem_name, test_data->acc_type));
        if (ypipe) {
            LOG_CLI((BSL_META_U(unit,
                                "s EGR_SBS_CONTROL PIPE_SELECT=1\n")));
            LOG_CLI((BSL_META_U(unit,
                                "s SBS_CONTROL PIPE_SELECT=1\n")));
        }
            LOG_CLI((BSL_META_U(unit,
                                "wr %s%s 1 1 %s=0\n"),
                     pipe_str, test_data->mem_name, test_data->field_name));
            LOG_CLI((BSL_META_U(unit,
                                "d %s%s 1 1\n"), pipe_str, test_data->mem_name));
            if (test_data->tcam_parity_bit != -1) {
                LOG_CLI((BSL_META_U(unit,
                                    "s %s %s=0\n"),
                         SOC_REG_NAME(unit,test_data->parity_enable_reg),
                         SOC_FIELD_NAME(unit,
                         test_data->parity_enable_field)));
            } else {
                LOG_CLI((BSL_META_U(unit,
                                    "s %s 0\n"),
                         SOC_REG_NAME(unit,test_data->parity_enable_reg)));
            }
            LOG_CLI((BSL_META_U(unit,
                                "wr nocache %s%s 1 1 %s=1\n"),
                     pipe_str, test_data->mem_name, test_data->field_name));
            if (test_data->tcam_parity_bit != -1) {
                LOG_CLI((BSL_META_U(unit,
                                    "s %s %s=1\n"),
                         SOC_REG_NAME(unit, test_data->parity_enable_reg),
                         SOC_FIELD_NAME(unit,
                         test_data->parity_enable_field)));
            } else {
                LOG_CLI((BSL_META_U(unit,
                                    "s %s 1\n"),
                         SOC_REG_NAME(unit,test_data->parity_enable_reg)));
            }
            LOG_CLI((BSL_META_U(unit,
                                "d %s%s 1 1\n"), pipe_str, test_data->mem_name));
        if (ypipe) {
            LOG_CLI((BSL_META_U(unit,
                                "s EGR_SBS_CONTROL PIPE_SELECT=0\n")));
            LOG_CLI((BSL_META_U(unit,
                                "s SBS_CONTROL PIPE_SELECT=0\n")));
        }

    } else {
        LOG_CLI((BSL_META_U(unit,
                            "ERROR: Attempted to print a cmd from missing data.\n")));
    }
}


/*
 * Function:
 *      soc_ser_test_overlays
 * Purpose:
 *      Enables SER test functions for a unit
 * Parameters:
 *      unit        - (IN) Device Number
 *      test_type   - (IN) Sets how many memories to test
 *      overlays    - (IN) List of all overlay memories to test
 *      pipe_select - (IN) Chip-specific function used to change pipe select
 */
int
soc_ser_test_overlays(int unit, _soc_ser_test_t test_type,
                      const soc_ser_overlay_test_t *overlays,
                      int(*pipe_select)(int,int,int) )
{
    int i, rv, mem_failed=0;
    soc_acc_type_t acc_type;
    uint32 tmp_entry[SOC_MAX_MEM_WORDS], fieldData[SOC_MAX_REG_FIELD_WORDS];
    ser_test_data_t test_data;
    if (overlays == NULL) {
        return -1;
    }
    for (i = 0; overlays[i].mem != INVALIDm; i++) {
        int last_failed = mem_failed;
        acc_type = overlays[i].acc_type;
        soc_ser_create_test_data(unit, tmp_entry, fieldData,
                                 overlays[i].parity_enable_reg,
                                 SOC_INVALID_TCAM_PARITY_BIT,
                                 overlays[i].parity_enable_field,
                                 overlays[i].mem, EVEN_PARITYf,
                                 MEM_BLOCK_ANY, REG_PORT_ANY,
                                 acc_type, 0, &test_data);
        pipe_select(unit, TRUE, ((acc_type == _SOC_ACC_TYPE_PIPE_Y)? 1:0));
        pipe_select(unit, FALSE, ((acc_type == _SOC_ACC_TYPE_PIPE_Y)? 1:0));
        rv = ser_test_mem(unit, &test_data, test_type, &mem_failed);
        if (rv < 0) {
            mem_failed++;
        }
        /*Clear memory so other views can be used.*/
        if (soc_mem_clear(unit,overlays[i].mem, MEM_BLOCK_ALL, TRUE) < 0) {
            return -1;
        }
        pipe_select(unit, TRUE,0);
        pipe_select(unit, FALSE,0);
        if (mem_failed != last_failed) {
            LOG_CLI((BSL_META_U(unit,
                                "%s failed overlay test.\n"),
                     SOC_MEM_NAME(unit,test_data.mem)));
        }
    }
    return mem_failed;
}


/*
 * Function:
 *      soc_ser_test_functions_register
 * Purpose:
 *      Enables SER test functions for a unit
 * Parameters:
 *      unit    - (IN) Device Number
 *      fun     - (IN) An instance of soc_ser_test_functions_t with pointers to
 *                     functions required for chip-specific SER test
                       implementations.
 */
void
soc_ser_test_functions_register(int unit, soc_ser_test_functions_t *fun)
{
    if (unit < SOC_MAX_NUM_DEVICES) {
        ser_test_functions[unit] = fun;
    }
    else {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Invalid unit parameter %d: passed to soc_ser_test_functions_t"),
                     unit));
    }
}


/*
 * Function:
 *      soc_ser_inject_error
 * Purpose:
 *      Redirect to appropriate chip-specific function.
 *      These are found in the chip's soc/esw/X.c file
 *      and registered during misc init.
 */
soc_error_t
soc_ser_inject_error(int unit, uint32 flags, soc_mem_t mem, int pipe,
                     int blk, int index)
{
    if( ser_test_functions[unit] != NULL &&
        ser_test_functions[unit]->inject_error_f != NULL) {
        return (*(ser_test_functions[unit]->inject_error_f))(unit, flags, mem,
                    pipe, blk, index);
    }
    return SOC_E_FAIL;
}

/*
 * Function:
 *      soc_ser_inject_support
 * Purpose:
 *      Routine to determine Memory is supported by error
 *      injection interface.
 */
soc_error_t
soc_ser_inject_support(int unit, soc_mem_t mem, int pipe)
{
    if( ser_test_functions[unit] != NULL &&
        ser_test_functions[unit]->injection_support != NULL) {
        return (*(ser_test_functions[unit]->injection_support))
                    (unit, mem, pipe);
    }
    return SOC_E_FAIL;
}

/*
 * Function:
 *      soc_ser_test_mem
 * Purpose:
 *      Redirect to appropriate chip-specific function.
 *      These are found in the chip's soc/esw/X.c file
 *      and registered during misc init.
 */
soc_error_t
soc_ser_test_mem(int unit, soc_mem_t mem, _soc_ser_test_t testType, int cmd)
{
    if( ser_test_functions[unit] != NULL &&
        ser_test_functions[unit]->test_mem != NULL) {
        return (*(ser_test_functions[unit]->test_mem))(unit, mem, testType, cmd);
    }
    return SOC_E_FAIL;
}


/*
 * Function:
 *      soc_ser_test
 * Purpose:
 *      Redirect to appropriate chip-specific function.
 *      These are found in the chip's soc/esw/X.c file and registered during misc init.
 */
soc_error_t
soc_ser_test(int unit, _soc_ser_test_t testType)
{
    if( ser_test_functions[unit] != NULL &&
        ser_test_functions[unit]->test != NULL) {
        return (*(ser_test_functions[unit]->test))(unit, testType);
    }
    return SOC_E_FAIL;
}
#endif /* defined (SER_TR_TEST_SUPPORT)*/
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT || BCM_POLAR_SUPPORT */
