/*
 * $Id: bist.c,v 1.20 Broadcom SDK $
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
 * Built-in Self Test for ARL, L3, and CBP memories
 */


#include <shared/bsl.h>

#include <sal/types.h>
#include <soc/mem.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include "testlist.h"

#ifdef BCM_DFE_SUPPORT
#include <soc/dfe/cmn/dfe_drv.h>
#endif
#ifdef BCM_88650_A0
    #include <soc/dpp/ARAD/arad_init.h>
    #include <bcm/init.h>
    #include <soc/dpp/drv.h>
#endif

#if defined (BCM_ESW_SUPPORT) ||  defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT) 
typedef struct bist_testdata_s {
    soc_mem_t    mems[NUM_SOC_MEM];
    int     num_mems;
} bist_testdata_t;

int
bist_test_init(int u, args_t *a, void **p)
{

    bist_testdata_t    *bd = 0;

    if (SOC_IS_DFE(u)){
        return 0;
    }
#ifdef BCM_88650_A0
    else if (SOC_IS_ARAD(u)){
        char *arg0 = ARG_CUR(a);
        if (arg0 != NULL && sal_strcasecmp(arg0, "all") == 0) {
            ARG_NEXT(a); /* ignore the default argument for this TR */
        }
        return init_deinit_test_init(u, a, p);
    }
#endif

    if ((bd = sal_alloc(sizeof (*bd), "bist-testdata")) == 0) {
    goto fail;
    }

#if defined (BCM_ESW_SUPPORT)
    if (bist_args_to_mems(u, a, bd->mems, &bd->num_mems) < 0) {
    goto fail;
    }
#endif
    if ((soc_reset_init(u)) < 0) {
        goto fail;
    }

    if (soc_mem_debug_set(u, 0) < 0) {
    goto fail;
    }

    *p = (void *) bd;

    return 0;

 fail:
    if (bd) {
    sal_free(bd);
    }

    return -1;
}

int
bist_test(int u, args_t *a, void *p)
{
#if defined (BCM_ESW_SUPPORT)
    bist_testdata_t    *bd = p;
#endif  
    int        rv = 0;
    COMPILER_REFERENCE(a);  

#if defined (BCM_DFE_SUPPORT)
    if (SOC_IS_DFE(u)) {
        rv = soc_dfe_drv_mbist(u, 0);
    } else
#endif     
#ifdef BCM_88650_A0
    if (SOC_IS_ARAD(u)) {
        SOC_DPP_CONFIG(u)->tm.various_bm |= DPP_VARIOUS_BM_FORCE_MBIST_TEST; /* mark MBIST to be performed by the init */
        if ((rv = init_deinit_test(u, a, p))) { /* perform deinit and init */
            test_error(u, "BIST or deinint/init failed: %s\n", soc_errmsg(rv));
        }
        SOC_DPP_CONFIG(u)->tm.various_bm &= ~(uint8)DPP_VARIOUS_BM_FORCE_MBIST_TEST;
        return rv;
    } else
#endif     
#if defined (BCM_ESW_SUPPORT)
    {
        rv = soc_bist(u,
               bd->mems, bd->num_mems,
               SOC_CONTROL(u)->bistTimeout);
    }
#else
    {
        cli_out("device not supported\n");
    }
#endif
    if (rv < 0) {
        test_error(u, "BIST failed: %s\n", soc_errmsg(rv));
        return -1;
    }
    return 0;
}

int
bist_test_done(int u, void *p)
{
    int        rv;
#ifdef BCM_88650_A0
    if (SOC_IS_ARAD(u))
    {
        return init_deinit_test_done(u, p);
    } else
#endif
    if(SOC_IS_DFE(u)){
        return 0;
    } else {
        rv = soc_reset_init(u) ;
    }
    sal_free(p);

    if (rv < 0) {
    test_error(u, "Post-BIST reset failed: %s\n", soc_errmsg(rv));
    return -1;
    }

    return 0;
}
#endif /* BCM_ESW_SUPPORT */
