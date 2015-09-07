/*
 * $Id: armcore.c 1.3 Broadcom SDK $
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
 * tr145 - ARM core test
 * Test the ARM core(s) embedded in CMICM and IPROC switches
 */

#include <sal/types.h>
#include <sal/core/boot.h>
#include <sal/appl/sal.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include <soc/types.h>
#include <soc/debug.h>
#include <soc/shared/mos_msg_common.h>
#if defined(BCM_CMICM_SUPPORT) || defined(BCM_IPROC_SUPPORT)
#include <soc/uc_msg.h>
#endif

#include <bcm/error.h>

#include "testlist.h"
#include <appl/diag/system.h>
#include <appl/diag/progress.h>

#if defined(BCM_CMICM_SUPPORT) || defined(BCM_IPROC_SUPPORT)

/* Return codes for TEST_RUN calls */
#define MOS_TEST_RC_OK                  0
#define MOS_TEST_RC_FAILURE             0xffffffff
#define MOS_TEST_RC_INVALID             0xfffffffe

typedef struct arm_core_work_s {
    int         unit;                   /* unit number */
    int         uc_num;                 /* core # to test */
    uint32      tests;                  /* bitmap of tests to perform */
} arm_core_work_t;

static uint32 _arm_core_uc_msg_timeout_usecs = 10 * 1000 * 1000; /* 10sec */

static arm_core_work_t *ac_work[SOC_MAX_NUM_DEVICES];

static
int
arm_core_test_msg(arm_core_work_t *ac, int subclass, uint32 data_in, uint32 *data_out)
{
    int                 rv;
    mos_msg_data_t      send, reply;

    /* Build the message */
    sal_memset(&send, 0, sizeof(send));
    sal_memset(&reply, 0, sizeof(reply));
    send.s.mclass = MOS_MSG_CLASS_TEST;
    send.s.subclass = subclass;
    send.s.data = soc_htonl(data_in);

    /* Send and receive */
    rv = soc_cmic_uc_msg_send_receive(ac->unit, ac->uc_num, &send, &reply,
                                      _arm_core_uc_msg_timeout_usecs);

    /* Check reply class, subclass */
    if (rv != SOC_E_NONE) {
        test_error(ac->unit, "soc_cmic_uc_msg_send_receive failed (%d).\n", rv);
        return rv;
    }
    if ((reply.s.mclass != send.s.mclass) ||
        (reply.s.subclass != send.s.subclass)) {
        test_error(ac->unit, "unexpected message reply.\n");
        return SOC_E_INTERNAL;
    }

    /* return data */
    *data_out = soc_ntohl(reply.s.data);
    return SOC_E_NONE;
}

int
arm_core_test(int u, args_t *args, void *pa)
{
    arm_core_work_t     *ac = (arm_core_work_t *)pa;
    uint32              count;
    int                 r;
    int                 rv;
    uint32              rc;

    /* Start the test application */
    rv = soc_cmic_uc_appl_init(u, ac->uc_num, MOS_MSG_CLASS_TEST,
                               _arm_core_uc_msg_timeout_usecs,
                               0, 0);
    if (rv != SOC_E_NONE) {
        test_error(u, "Error starting test appl (%d).\n", rv);
        return -1;
    }

    rv = arm_core_test_msg(ac, MOS_MSG_SUBCLASS_TEST_COUNT, 0, &count);
    if (rv != SOC_E_NONE) {
        test_error(u, "Error communicating with test appl (%d).\n", rv);
        return -1;
    }

    /* If no tests, error */
    if (!count) {
        test_error(u, "uKernel image on core %d does not support test.\n",
                   ac->uc_num);
        return -1;
    }

    progress_init(count, 1, 0);
    for (r = 0; r < count; ) {
        rv = arm_core_test_msg(ac, MOS_MSG_SUBCLASS_TEST_RUN, r, &rc);
        if (rv != SOC_E_NONE) {
            test_error(u, "Error communicating with test appl (%d).\n", rv);
            return -1;
        }
        switch (rc) {
        case MOS_TEST_RC_OK:
            soc_cm_debug(DK_TESTS, "test %d PASS\n", r);
            break;
        case MOS_TEST_RC_FAILURE:
            soc_cm_debug(DK_TESTS, "test %d FAIL\n", r);
            break;
        case MOS_TEST_RC_INVALID:
            soc_cm_debug(DK_TESTS, "test %d SKIP\n", r);
            break;
        }
        r += 1;
        progress_report(1);
    }
    progress_done();
    return 0;
}

int
arm_core_test_init(int u, args_t *args, void **pa)
{
    arm_core_work_t     *ac;
    parse_table_t       pt;

    if (!soc_feature(u, soc_feature_mcs)
        && !soc_feature(u, soc_feature_iproc)) {
        test_error(u, "ERROR: test only valid on mcs or iproc enabled devices\n");
        return -1;
    } 

    if (ac_work[u] == NULL) {
        ac_work[u] = sal_alloc(sizeof(arm_core_work_t), "arm_core_test");
        if (ac_work[u] == NULL) {
            test_error(u, "ERROR: cannot allocate memory\n");
            return -1;
        }
        sal_memset(ac_work[u], 0, sizeof(arm_core_work_t));
    }
    ac = ac_work[u];
    ac->unit = u;

    parse_table_init(u, &pt);
    parse_table_add(&pt, "uc", PQ_INT, (void *)0, &ac->uc_num,
                    NULL);
    parse_table_add(&pt, "tests", PQ_INT, (void *)0xffffffff, &ac->tests,
                    NULL);

    if (parse_arg_eq(args, &pt) < 0 || ARG_CNT(args) != 0) {
        test_error(u, "%s: Invalid Option: %s\n", ARG_CMD(args),
                   ARG_CUR(args) ? ARG_CUR(args) : "*");
    }
    parse_arg_eq_done(&pt);

    *pa = (void *)ac;

    return 0;
}

int
arm_core_test_done(int u, void *pa)
{
    arm_core_work_t     *ac = (arm_core_work_t *)pa;

    if (NULL == ac) {
        return(0);
    }

    sal_free(ac);
    ac_work[u] = NULL;
    return(0);
}

#endif
