/*
 * $Id: loopback2.c,v 1.30 Broadcom SDK $
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
 * Loopback Tests, version 2
 *
 *	MAC	Sends packets from CPU to MAC in loopback mode, and back
 *	PHY	Sends packets from CPU to PHY in loopback mode, and back
 *	EXT	Sends packets from CPU out one port, in another, and back
 *	SNAKE	Sends packets from CPU through ports via MAC or PHY loopback
 */

#include <shared/bsl.h>

#include <sal/core/boot.h>
#include <sal/appl/sal.h>
#include <shared/bsl.h>
#include <appl/diag/system.h>
#include <appl/diag/system.h>
#include <appl/diag/test.h>

#include <soc/debug.h>
#include <soc/hash.h>

#include <soc/higig.h>

#ifdef BCM_ESW_SUPPORT
#include <bcm_int/esw/mbcm.h>
#endif

#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/link.h>
#include <bcm/stg.h>
#include <bcm/stat.h>
#include <bcm/mcast.h>
#include <bcm/stack.h>

#include <appl/test/loopback2.h>

#include "testlist.h"

/*
 * Loopback work structure
 */

STATIC loopback2_test_t lb2_work[SOC_MAX_NUM_DEVICES];

int
lb2_mac_init(int unit, args_t *a, void **pa)
/*
 * Function: 	lb2_mac_init
 * Purpose:	Initialize MAC loopback test.
 * Parameters:	unit - unit number.
 * Returns:	0 for success, -1 for failed.
 */
{
    loopback2_test_t	*lw = &lb2_work[unit];
    loopback2_testdata_t	*lp = &lw->params[LB2_TT_MAC];
    parse_table_t	pt;

    lbu_setup(unit, lw);

    parse_table_init(unit, &pt);
    lbu_pkt_param_add(unit, &pt, lp);
    lbu_port_param_add(unit, &pt, lp);
    lbu_other_param_add(unit, &pt, lp);

    if (parse_arg_eq(a, &pt) < 0 || ARG_CNT(a) != 0) {
	test_error(unit,
		   "%s: Invalid option: %s\n",
		   ARG_CMD(a),
		   ARG_CUR(a) ? ARG_CUR(a) : "*");
	parse_arg_eq_done(&pt);
	return(-1);
    }
    parse_arg_eq_done(&pt);

    if (lbu_check_parms(lw, lp)) {
	return(-1);
    }

    /* Perform common loopback initialization */

    if (lbu_init(lw, lp)) {
	return(-1);
    }

    *pa = lw;

    return(0);
}

int
lb2_phy_init(int unit, args_t *a, void **pa)
/*
 * Function: 	lb2_phy_init
 * Purpose:	Initialize PHY loopback test.
 * Parameters:	unit - unit number.
 * Returns:	0 for success, -1 for failed.
 */
{
    loopback2_test_t	*lw = &lb2_work[unit];
    loopback2_testdata_t	*lp = &lw->params[LB2_TT_PHY];
    parse_table_t	pt;
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
    pbmp_t pbmp;
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */

    lbu_setup(unit, lw);

    parse_table_init(unit, &pt);
    lbu_pkt_param_add(unit, &pt, lp);
    lbu_port_param_add(unit, &pt, lp);
    lbu_other_param_add(unit, &pt, lp);

    if (parse_arg_eq(a, &pt) < 0 || ARG_CNT(a) != 0) {
	test_error(unit,
		   "%s: Invalid option: %s\n",
		   ARG_CMD(a),
		   ARG_CUR(a) ? ARG_CUR(a) : "*");
	parse_arg_eq_done(&pt);
	return(-1);
    }
    parse_arg_eq_done(&pt);

#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
    if (SOC_IS_NORTHSTAR(unit)||SOC_IS_NORTHSTARPLUS(unit)) {
        pbmp = soc_property_get_pbmp(unit, spn_PBMP_WAN_PORT, 0);
        SOC_PBMP_AND(pbmp, PBMP_ALL(unit));
        SOC_PBMP_REMOVE(lp->pbm, pbmp);
        cli_out("Removed WAN port(0x%x) from test ports\n",  \
                SOC_PBMP_WORD_GET(pbmp, 0));
    }
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPOR */

    if (lbu_check_parms(lw, lp)) {
	return(-1);
    }

    /* Perform common loopback initialization */

    if (lbu_init(lw, lp)) {
	return(-1);
    }

    *pa = lw;

    return(0);
}

/*ARGSUSED*/
int
lb2_port_test(int unit, args_t *a, void *pa)
/*
 * Function: 	lb2_port_test
 * Purpose:	Perform MAC loopback tests.
 * Parameters:	unit - unit #
 *		a - arguments (Not used, assumed parse by lb_mac_init).
 * Returns:
 */
{
    loopback2_test_t	*lw = (loopback2_test_t *)pa;
    loopback2_testdata_t	*lp = lw->cur_params;
    int		port, rv;
    pbmp_t	pbm, tpbm;

    COMPILER_REFERENCE(a);

    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META_U(unit,
                         "lb_mac_test[%d]: Starting ....\n"), unit));

    lbu_stats_init(lw);

    BCM_PBMP_ASSIGN(pbm, lp->pbm);
    BCM_PBMP_ASSIGN(tpbm, lp->pbm);
    BCM_PBMP_AND(tpbm, PBMP_FE_ALL(unit));

    ENET_SET_MACADDR(lw->base_mac_src, lp->mac_src);
    ENET_SET_MACADDR(lw->base_mac_dst, lp->mac_dst);

    /*
     * Used to determine if ARL entry needs to be setup. Typically
     * required only for _E_ Ports.
     */

    BCM_PBMP_ASSIGN(tpbm, pbm);
    BCM_PBMP_AND(tpbm, PBMP_E_ALL(unit));
    if (BCM_PBMP_NOT_NULL(tpbm)) { 
        /* Don't setup arl if only hg ports */
        /*
         * Get our current MAC address for this port and be sure the
         * SOC will route packets to the CPU.
         */
        LOG_INFO(BSL_LS_APPL_TESTS,
                 (BSL_META_U(unit,
                             "Setting up ARL\n")));
        if (lbu_setup_arl_cmic(lw)) {
            return(-1);
        }
    }

    PBMP_ITER(pbm, port) {
        lw->tx_len = 0;                 /* Start off clean */

        lw->tx_port = port;
        lw->rx_port = port;

        rv = lbu_serial_txrx(lw);

        if (lp->inject) { /* Diagnostic */
            return 0;
        }

        if (rv < 0) {
            return -1;
        }
    }
    
    if (BCM_PBMP_NOT_NULL(tpbm)) { 
        lbu_cleanup_arl(lw);
    }

    lbu_stats_done(lw);

    return 0;
}

int
lb2_done(int unit, void *pa)
/*
 * Function: 	lb2_done
 * Purpose:	Clean up after new loopback tests.
 * Parameters:	unit - unit number.
 * Returns:	0 for success, -1 for failed.
 */
{
    soc_port_t	p;
    int		rv;

    if (pa) {
	loopback2_testdata_t *lp = ((loopback2_test_t *)pa)->cur_params;

        if (lp->inject) { /* Diagnostic */
            return 0;
        }

	/* Clear MAC loopback - Only if pa is set */

	PBMP_ITER(lp->pbm, p) {
	    rv = bcm_port_loopback_set(unit, p, BCM_PORT_LOOPBACK_NONE);
	    if (rv != BCM_E_NONE) {
		test_error(unit,
			   "Port %s: Failed to reset MAC loopback: %s\n",
			   SOC_PORT_NAME(unit, p), bcm_errmsg(rv));
                return rv;
	    }
	}

        if (lp->test_type == LB2_TT_SNAKE) {
            if ((rv = lbu_snake_done(&lb2_work[unit])) < 0) {
                return rv;
            }
        }
    }

    if ((rv = lbu_done(&lb2_work[unit])) < 0) {
        return rv;
    }

    return 0;
}

int
lb2_snake_init(int unit, args_t *a, void **pa)
/*
 * Function: 	lb2_snake_init
 * Purpose:	Initialize snake loopback test.
 * Parameters:	unit - unit number.
 * Returns:	0 for success, -1 for failed.
 */
{
    loopback2_test_t	*lw = &lb2_work[unit];
    loopback2_testdata_t	*lp = &lw->params[LB2_TT_SNAKE];
    parse_table_t	pt;
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
    soc_pbmp_t pbmp;
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */

    lbu_setup(unit, lw);

    parse_table_init(unit, &pt);
    lbu_pkt_param_add(unit, &pt, lp);
    lbu_port_param_add(unit, &pt, lp);
    lbu_other_param_add(unit, &pt, lp);

    if (parse_arg_eq(a, &pt) < 0 || ARG_CNT(a) != 0) {
	test_error(unit,
		   "%s: Invalid option: %s\n",
		   ARG_CMD(a),
		   ARG_CUR(a) ? ARG_CUR(a) : "*");
	return(-1);
    }

    if (lbu_check_parms(lw, lp)) {
	return(-1);
    }

    /* Snake test supported only on Ethernet ports on FB & ER */
    if (SOC_IS_FB_FX_HX(unit)) {
        SOC_PBMP_AND(lp->pbm, PBMP_E_ALL(unit));
    }

#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
    if (SOC_IS_NORTHSTAR(unit)||SOC_IS_NORTHSTARPLUS(unit)) {
        pbmp = soc_property_get_pbmp(unit, spn_PBMP_WAN_PORT, 0);
        SOC_PBMP_AND(pbmp, PBMP_ALL(unit));
        SOC_PBMP_REMOVE(lp->pbm, pbmp);
        cli_out("Removed WAN port(0x%x) from test ports\n",  \
                SOC_PBMP_WORD_GET(pbmp, 0));
    }
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPOR */

    /* Perform common loopback initialization */
    if (lbu_init(lw, lp)) {
	return(-1);
    }

    /* Perform snake specific initialization */
    if (lbu_snake_init(lw, lp)) {
	return(-1);
    }

    *pa = lw;

    return(0);
}

int
lb2_snake_test(int unit, args_t *a, void *pa)
/*
 * Function: 	lb2_snake_test
 * Purpose:	Run snake loopback test.
 * Parameters:	unit - unit number.
 * Returns:	0 for success, -1 for failed.
 */
{
    loopback2_test_t	   *lw = (loopback2_test_t *) pa;
    loopback2_testdata_t   *lp = lw->cur_params;
    int		    c_count, ix;
    int		    rv, ret_val = 0;
    lb2_port_stat_t  *stats;
#ifdef BCM_ROBO_SUPPORT
    soc_port_t	p;
#endif

    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(a);
    COMPILER_REFERENCE(lp);

    stats = sal_alloc(SOC_MAX_NUM_PORTS *
				  sizeof(lb2_port_stat_t),
				  "Stats");

    lw->tx_cos = lp->cos_start;
    lw->tx_len = lp->len_start;
    lw->tx_ppt = lp->ppt_start;

#ifdef BCM_ROBO_SUPPORT
    /* 
     * The BCM53101 (Lotus) has limited packet buffer inside chip.
     * It need to enable the flow control to avoid the packets dropped.
     */
    if (SOC_IS_LOTUS(unit)) {
        PBMP_ITER(lp->pbm, p) {
            rv = bcm_port_pause_set(unit, p, TRUE, TRUE);
            if (rv != BCM_E_NONE) {
                test_error(unit,
			   "Port %s: Failed to set TX/RX pause %s\n",
			   SOC_PORT_NAME(unit, p), bcm_errmsg(rv));
                sal_free(stats);
                return rv;
            }
        }
    }

#endif

    for (c_count = 0; c_count < lp->iterations; c_count++) {

	cli_out("\nLB: loop %d of %d: "
                "circular test on ports for %d seconds\n",
                c_count + 1, lp->iterations, lp->duration);

        /* bcm call here someday.  For now, bcm_stat_clear is
         * just a wrapper for this function, and only does one port */
        if (!lp->inject) {
            if (SOC_IS_ROBO(unit)){
#ifdef BCM_ROBO_SUPPORT
                if ((rv = soc_robo_counter_set32_by_port(unit,  
                                           PBMP_PORT_ALL(unit), 0)) < 0) {
                    test_error(unit,
                           "Could not clear counters: %s\n",
                           soc_errmsg(rv));
                    ret_val = -1;
                    break;
                }
#endif
            } else {
#ifdef BCM_ESW_SUPPORT
                if ((rv = soc_counter_set32_by_port(unit,  
                                      PBMP_PORT_ALL(unit), 0)) < 0) {
                    test_error(unit,
                               "Could not clear counters: %s\n",
                               soc_errmsg(rv));
                    ret_val = -1;
                    break;
                }
#endif
            }
        }

        if (lp->inject) {
            lbu_snake_tx(lw);
            ret_val = 0;
            break;
        }

        for (ix = 0; ix < SOC_MAX_NUM_PORTS; ix++) {
            stats[ix].initialized = FALSE;
        }

        if ((rv = lbu_snake_txrx(lw, stats)) < 0) {
            test_error(unit, "Snake test failed\n");
            ret_val = -1;
            break;
        }

        /* Increment parameters for next cycle */
        lw->tx_cos += 1;
        if (lw->tx_cos > lp->cos_end) {
            lw->tx_cos = lp->cos_start;
        }
        lw->tx_len += lp->len_inc;
        if (lw->tx_len > lp->len_end) {
            lw->tx_len = lp->len_start;
        }
        lw->tx_ppt += lp->ppt_inc;
        if (lw->tx_ppt > lp->ppt_end) {
            lw->tx_ppt = lp->ppt_start;
        }
    }
#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_LOTUS(unit)) {
        PBMP_ITER(lp->pbm, p) {
            rv = bcm_port_pause_set(unit, p, FALSE, FALSE);
            if (rv != BCM_E_NONE) {
                test_error(unit,
			   "Port %s: Failed to set TX/RX pause: %s\n",
			   SOC_PORT_NAME(unit, p), bcm_errmsg(rv));
                sal_free(stats);
                return rv;
            }
        }
    }

#endif    

    sal_free(stats);
    return ret_val;
}

