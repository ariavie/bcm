/*
 * $Id: snmp.c,v 1.20 Broadcom SDK $
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
 * SNMP Statistics Retrieval test.
 * 
 * For every port, and every speed possible on that port, can
 * we retrieve SNMP statistics without an error? This test will
 * confirm that this is the case.
 *
 */

#if defined(VXWORKS)
#include <vxWorks.h>  /* to work around stdarg conditionals problem */
#endif
#include <stdarg.h>
#include <sal/types.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <soc/debug.h>
#include <sal/appl/pci.h>
#include <bcm/error.h>
#include <bcm/link.h>
#include <bcm/stat.h>
#include <appl/diag/test.h>
#include <shared/bsl.h>
#include "testlist.h"

#if defined (BCM_ESW_SUPPORT) || defined (BCM_PETRA_SUPPORT)

#undef min
#define min(x, y) ((x) <= (y) ? (x) : (y))

typedef struct snmp_test_s {

    int unit;
    int speed;
    int failures;
    int not_impl;
    bcm_port_info_t snmp_save_port[SOC_MAX_NUM_PORTS];

} snmp_test_t;

int
snmp_test_done(int u, void *pa)
{
    int                 port, rv;
    snmp_test_t         *b = (snmp_test_t *) pa;

    PBMP_PORT_ITER(b->unit, port) {
        if ((rv = bcm_port_info_set(b->unit, port,
                                    &b->snmp_save_port[port])) < 0) {
            test_error(b->unit,
                       "Port %d: Could not set port info: %s\n",
                       port + 1, bcm_errmsg(rv));
            return -1;
        }
    }

    sal_free(b);

    return 0;
}

int
snmp_test_init(int u, args_t *a, void **pa)
{
    snmp_test_t         *b = 0;
    int                 port, rv = -1;

    if ((b = sal_alloc(sizeof (snmp_test_t), "snmp")) == 0) {
        goto done;
    }

    memset(b, 0, sizeof (*b));
    b->unit = u;

    *pa = (void *) b;

    /* Save port states */
    PBMP_PORT_ITER(b->unit, port) {
        if ((rv = bcm_port_info_get(b->unit, port,
                                    &b->snmp_save_port[port])) < 0) {
            test_error(b->unit,
                       "Port %d: Could not get port info: %s\n",
                       port + 1, bcm_errmsg(rv));
            return -1;
        }
    }
    
    rv = 0;

 done:
    if (rv < 0) {
        snmp_test_done(u, *pa);
    }

    return rv;
}

static int
snmp_test_begin(snmp_test_t *b,
                int speed)
{
    uint64              v64;
    int                 port, rv, speedToUse = 0;
    bcm_stat_val_t      object;
    bcm_port_ability_t  ability;

    b->failures = 0;
    b->not_impl = 0;

    /* Set up all port speeds */
    PBMP_PORT_ITER(b->unit, port) {
        rv = bcm_port_ability_local_get(b->unit, port, &ability);

        switch(speed) {
           case 10: 
               if ((ability.speed_full_duplex & SOC_PA_SPEED_10MB) || 
                   (ability.speed_half_duplex & SOC_PA_SPEED_10MB)){
                   speedToUse = 10;
                   break;
               }
               /* fall through */
           case 100:
               if ((ability.speed_full_duplex & SOC_PA_SPEED_100MB) || 
                   (ability.speed_half_duplex & SOC_PA_SPEED_100MB)){
                   speedToUse = 100;
                   break;
               }
               /* fall through */
           case 1000: 
               if ((ability.speed_full_duplex & SOC_PA_SPEED_1000MB) || 
                   (ability.speed_half_duplex & SOC_PA_SPEED_1000MB)){
                   speedToUse = 1000;
                   break;
               }
               /* fall through */
           case 2500: 
               if ((ability.speed_full_duplex & SOC_PA_SPEED_2500MB) || 
                   (ability.speed_half_duplex & SOC_PA_SPEED_2500MB)){
                   speedToUse = 2500;
                   break;
               }
               /* fall through */
           case 3000: 
               if ((ability.speed_full_duplex & SOC_PA_SPEED_3000MB) || 
                   (ability.speed_half_duplex & SOC_PA_SPEED_3000MB)){
                   speedToUse = 3000;
                   break;
               }
               /* fall through */
           case 10000:
               if ((ability.speed_full_duplex & SOC_PA_SPEED_10GB) || 
                   (ability.speed_half_duplex & SOC_PA_SPEED_10GB)){
                   speedToUse = 10000;
                   break;
               }
               /* fall through */
           default:
               rv = bcm_port_speed_max(b->unit, port, &speedToUse);
       } 
 
        if (BCM_FAILURE(rv) || (!speedToUse)) {
            test_error(b->unit,
                       "Port %d: Could not configure speed : %d\n",
                        port, speed);
        }
                         
        if ((rv = bcm_port_speed_set(b->unit, port, speedToUse)) < 0) {
            test_error(b->unit,
                       "Port %d: Could not set speed to %d: %s\n",
                       port, speedToUse, bcm_errmsg(rv));
        }
    }

    b->speed = speed;

    /* Now, call bcm_stat_get() for every stat on every port */

    PBMP_PORT_ITER(b->unit, port) {
        /*
         * Now, for every object, find out if it generates an error on
         * that port.
         */
        for (object = 0; object < snmpValCount; object++) {
            rv = bcm_stat_get(b->unit, port, object, &v64);

            if (rv == BCM_E_UNAVAIL) {
                b->not_impl++;
            } else if (rv < 0) {
                cli_out("FAIL: u=%d:speed=%d:port=%d:object=%d:rv=%d: %s\n",
                        b->unit, b->speed,port, object, rv, bcm_errmsg(rv));
                b->failures++;
            } else {
                /* No output */
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(b->unit,
                                        "PASS: u=%d:speed=%d:port=%d:object=%d:rv=%d: %s\n"),
                             b->unit, b->speed, port, object, rv, bcm_errmsg(rv)));
            }
        }
    }

    return 1;
}

static void
snmp_test_end(snmp_test_t *b)
{
    cli_out("All Ports: Max speed=%d: %d failures; %d vars not implemented\n",
            b->speed, b->failures, b->not_impl);
}

int
snmp_test_test(int unit, args_t *a, void *pa)
{
    snmp_test_t         *b = (snmp_test_t *) pa;
    int                 i, speeds[] = {10, 100, 1000, 10000, -1};
    int                 rc = 0;

    COMPILER_REFERENCE(a);

    for (i = 0; speeds[i] > 0; i++) {
        snmp_test_begin(b, speeds[i]);
        snmp_test_end(b);

        if (b->failures > 0) {
            rc = -1;
        }
    }

    return rc;
}

#endif /* BCM_ESW_SUPPORT */
