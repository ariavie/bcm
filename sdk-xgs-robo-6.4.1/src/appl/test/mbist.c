/*
 * $Id: bist.c,v 1.21 Broadcom SDK $
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
 * Memory Built-in Self Test
 */


#include <sal/types.h>
#include <soc/mem.h>
#include <shared/bsl.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <bcm/link.h>
#include "testlist.h"

#if defined(BCM_TRIDENT2_SUPPORT)

typedef enum {
    MEMORY_BIST,
    LOGIC_BIST,
    ALL_BIST
} bist_type_t;

static bist_type_t bist_test_parameters[SOC_MAX_NUM_DEVICES];

int
memory_logic_bist_test_init(int u, args_t *a, void **p)
{
    char * test_type_name = NULL;
    bist_type_t * bist_type = NULL;
    parse_table_t pt;
    
    bist_type = &bist_test_parameters[u];
    parse_table_init(u, &pt);
    parse_table_add(&pt,  "type",    PQ_STRING, "memory",
            &(test_type_name), NULL);
    parse_default_fill(&pt);
    if (parse_arg_eq(a, &pt) < 0) {
        cli_out("%s: Invalid option: %s\n",
                ARG_CMD(a), ARG_CUR(a));
        return -1;
    }
    if (!sal_strcasecmp(test_type_name, "memory")) {
        *bist_type = MEMORY_BIST;
    } else if (!sal_strcasecmp(test_type_name, "logic")) {
        *bist_type = LOGIC_BIST;
    } else if (!sal_strcasecmp(test_type_name, "all")) {
        *bist_type = ALL_BIST;
    } else {
        cli_out("Invalid test type selected.\n");
        return -1;
    }
    if (SOC_IS_TRIDENT2(u)){
    	  BCM_IF_ERROR_RETURN(bcm_linkscan_enable_set(u, 0));
        if ((soc_reset_init(u)) < 0) {
            return -1;
        }
    }
    *p = bist_type;
    return 0;
}

int
memory_logic_bist_test(int u, args_t *a, void *p)
{
    bist_type_t * bist_type = p;    
    if (SOC_IS_TRIDENT2(u) && ((*bist_type == MEMORY_BIST) || 
                               (*bist_type == ALL_BIST))) {
        cli_out("MEMORY BIST...\n");
        (void)trident2_mem_bist(u);
        cli_out("REGFILE BIST...\n");
        (void)trident2_regfile_bist(u);
    }
    return 0;
}

int
memory_logic_bist_test_done(int u, void *p)
{
    if (SOC_IS_TRIDENT2(u)){
        if ((soc_reset_init(u)) < 0) {
            return -1;
        }
    }
    return 0;
}

#endif
