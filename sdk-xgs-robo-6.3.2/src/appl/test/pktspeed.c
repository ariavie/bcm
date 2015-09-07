/*
 * $Id: pktspeed.c 1.26 Broadcom SDK $
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
 * File:        pktspeed.c
 * Purpose:     Run a TX or RX reload test to check DMA speed
 *
 * Test Parameters:
 *       PORT       - TX port
 *       RXPORT     - RX port
 *       PKTsize    - What size packets to use
 *       ALLOCsize  - What allocation size to use for packets
 *       ChainLen   - How many DMA'd packets to set up in loop
 *       Seconds    - How many seconds to run
 *
 *   Test reports speed once per second.
 *
 * Notes:       Code originally from appl/diag/txrx.c
 *              Moved here to address PR SDK/422
 *
 */

#include <sal/core/time.h>
#include <sal/types.h>
#include <sal/appl/io.h>

#include <soc/types.h>
#include <soc/cm.h>
#include <soc/dma.h>
#include <soc/dcb.h>
#include <soc/drv.h>

#include <bcm/tx.h>
#include <bcm/pkt.h>
#include <bcm/rx.h>
#include <bcm/port.h>
#include <bcm/error.h>

#include "testlist.h"

#if defined (BCM_ESW_SUPPORT) || defined (BCM_PETRA_SUPPORT)

#define TEST_FAIL -1
#define TEST_OK    0

typedef struct pktspeed_param_s {
        int unit,        /* Which unit to test on */
            tx_port,     /* packet TX port (TX/RX test) */
            rx_port,     /* packet RX port (RX test) */
            pkt_size,    /* Size of actual pkt */
            alloc_size,  /* Allocation size of packet */
            chain_len,   /* Number of pkts to set up per cycle */
            seconds,     /* Number of seconds to run */
            poll;        /* T=poll F=interrupt */
    volatile COMPILER_DOUBLE count; /* Packet count */
} pktspeed_param_t;

static volatile int desc_intr_count = 0;
static volatile int chain_intr_count = 0;

static pktspeed_param_t     *pktspeed_param[SOC_MAX_NUM_DEVICES];

/*
 * Function:
 *      pktspeed_print_param
 *
 * Purpose:
 *      Print parameter structure
 *
 * Parameters:
 *      psp             - parameter structure
 *
 * Returns:
 *      Allocated structure or NULL if alloc failed.
 *
 */

STATIC void
pktspeed_print_param(pktspeed_param_t *psp)
{
    if (psp != NULL) {
        printk("port=%d ",  psp->tx_port);
        printk("rx=%d ",    psp->rx_port);
        printk("pkt=%d ",   psp->pkt_size);
        printk("alloc=%d ", psp->alloc_size);
        printk("cl=%d ",    psp->chain_len);
        printk("sec=%d ",   psp->seconds);
        printk("\n");
    } else {
        printk("psp(null)\n");
    }
}

/*
 * Function:
 *      pktspeed_print_report
 *
 * Purpose:
 *      Print speed report 
 *
 * Parameters:
 *      psp             - parameter structure
 *      t_diff          - time difference (usec)
 *
 * Returns:
 *      void
 *
 */

STATIC void
pktspeed_print_report(pktspeed_param_t *psp, COMPILER_DOUBLE count, int t_diff)
{
    COMPILER_DOUBLE bytes_per_second, pkts_per_second;
    COMPILER_DOUBLE t_diff_dbl, pkt_size_dbl;

    if (psp != NULL) {
        COMPILER_32_TO_DOUBLE(t_diff_dbl, t_diff);
        COMPILER_32_TO_DOUBLE(pkt_size_dbl, psp->pkt_size);
        pkts_per_second = count/t_diff_dbl;
        bytes_per_second = (count * pkt_size_dbl)/t_diff_dbl;
        printk("    %15" COMPILER_DOUBLE_FORMAT
               " %15" COMPILER_DOUBLE_FORMAT
               " %15" COMPILER_DOUBLE_FORMAT "\n",
               count, pkts_per_second, bytes_per_second);
    } else {
        printk("psp(null)\n");
    }
}

/*
 * Function:
 *      pktspeed_done_desc
 *
 * Purpose:
 *      dv_done_desc callout increments packet count in interrupt context
 *
 * Parameters:
 *      u               - unit
 *      dv              - dv struct
 *      dcb             - dcp struct
 *
 * Returns:
 *      void
 *
 */

STATIC void
pktspeed_done_desc(int u, struct dv_s *dv, dcb_t *dcb)
{
    pktspeed_param_t *psp = (pktspeed_param_t *)dv->dv_public1.ptr;

    psp->count += psp->chain_len;

    desc_intr_count += 1;
}

STATIC void
pktspeed_done_chain(int unit, dv_t *dv_chain)
{
    chain_intr_count += 1;
}

/*
 * Function:
 *      pktspeed_run_test
 *
 * Purpose:
 *      Run actual pktspeed test
 *
 * Parameters:
 *      tx_test         - TRUE for TX test, FALSE for RX test
 *      psp             - test parameters
 *
 * Returns:
 *      TEST_OK
 *      TEST_FAIL
 *
 * Notes:
 *      Could possibly use a pause scheme at the end of the while loop.
 */

STATIC int
pktspeed_run_test(int tx_test, pktspeed_param_t *psp)
{
    pbmp_t tx_pbm;
    static pbmp_t empty_pbm;    /* Initialized to 0 automatically */
    char *pkt_data = NULL;
    dv_t *dv = NULL;
    sal_usecs_t start_time, cur_time;
    COMPILER_DOUBLE count;
    int t_diff, last_rpt;
    int tot_dcb;
    int i;
    int rv = TEST_OK;
    dcb_t	*d;
    int channel;
    bcm_pkt_t *pkt = NULL;
    static sal_mac_addr_t mac_dest = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    static sal_mac_addr_t mac_src  = {0x02, 0x11, 0x22, 0x33, 0x44, 0x66};

    if (psp == NULL) {
        printk("pktspeed test: NULL param struct\n");
        return(TEST_FAIL);
    }

    COMPILER_32_TO_DOUBLE(count, 0);

    if (tx_test) {
        bcm_port_loopback_set(psp->unit, psp->tx_port, BCM_PORT_LOOPBACK_MAC);
        bcm_port_enable_set(psp->unit, psp->tx_port, TRUE);
        SOC_PBMP_PORT_SET(tx_pbm, psp->tx_port);
        channel = 0;
        if (soc_dma_chan_config(psp->unit, channel, DV_TX, 0) < 0) {
            printk("pktspeed test: Could not configure u %d, chan %d\n",
                   psp->unit,
                   channel);
            rv = TEST_FAIL;
            goto done;
        }
    } else {  /* Set ports to loopback and send a broadcast packet */
        bcm_port_loopback_set(psp->unit, psp->tx_port, BCM_PORT_LOOPBACK_MAC);
        bcm_port_loopback_set(psp->unit, psp->rx_port, BCM_PORT_LOOPBACK_MAC);
        bcm_port_enable_set(psp->unit, psp->tx_port, TRUE);
        bcm_port_enable_set(psp->unit, psp->rx_port, TRUE);
        if ((bcm_pkt_alloc(psp->unit,
                           psp->pkt_size,
                           BCM_TX_CRC_REGEN, &pkt)) !=
                BCM_E_NONE) {
            printk("Could not allocate packet to tx\n");
            return TEST_FAIL;
        }
        /* Set up broadcast packet */
        BCM_PKT_HDR_DMAC_SET(pkt, mac_dest);
        BCM_PKT_HDR_SMAC_SET(pkt, mac_src);
        BCM_PKT_HDR_TPID_SET(pkt, ENET_DEFAULT_TPID);
        BCM_PKT_HDR_VTAG_CONTROL_SET(pkt, VLAN_CTRL(0, 0, 1));
        SOC_PBMP_PORT_SET(pkt->tx_pbmp, psp->tx_port);
        pkt->opcode = BCM_HG_OPCODE_BC;
        channel = 1;
        if (soc_dma_chan_config(psp->unit, channel, DV_RX, 0) < 0) {
            printk("pktspeed test: Could not configure u %d, chan %d\n",
                   psp->unit,
                   channel);
            rv = TEST_FAIL;
            goto done;
        }
        bcm_rx_queue_channel_set(psp->unit, -1, 1);

        {
            int i;
            for (i=0; i<100; i++) {
                /* Send broadcast packet */
                if (bcm_tx(psp->unit, pkt, NULL) != BCM_E_NONE) {
                    printk("Failed to transmit packet\n");
                    return TEST_FAIL;
                }
            }
        }
    }

    tot_dcb = psp->chain_len + 1;    /* Need one for reload */

    /* Round packet size up to 8 byte boundary */
    psp->alloc_size = (psp->alloc_size + 7) & 0xffffff8;

    if ((pkt_data = soc_cm_salloc(psp->unit, psp->alloc_size * psp->chain_len,
                                  "reload_data")) == NULL) {
        printk("Could not allocate packet data\n");
        rv = TEST_FAIL;
        goto done;
    }

    if ((dv = soc_dma_dv_alloc(psp->unit, tx_test ? DV_TX : DV_RX,
                               tot_dcb)) == NULL) {
        printk("Could not allocate DV\n");
        rv = TEST_FAIL;
        goto done;
    }

    /* DCBs MUST start off 0 */
    sal_memset(dv->dv_dcb, 0, SOC_DCB_SIZE(psp->unit) * tot_dcb);

    for (i = 0; i < psp->chain_len; i++) {
        if (soc_dma_desc_add(dv, (sal_vaddr_t)&pkt_data[i * psp->alloc_size],
                             psp->alloc_size, tx_pbm, empty_pbm,
                             empty_pbm, 0, NULL) < 0) {
            printk("Could not setup packet %d\n", i);
            rv = TEST_FAIL;
            goto done;
        }
        soc_dma_desc_end_packet(dv);
    }

    /* Set reload bit of last descriptor to point to first */
    if (soc_dma_rld_desc_add(dv, (sal_vaddr_t)dv->dv_dcb) < 0) {
        printk("Could not setup reload dcb\n");
        rv = TEST_FAIL;
        goto done;
    }

    d = SOC_DCB_IDX2PTR(psp->unit, dv->dv_dcb, dv->dv_vcnt - 1);
    SOC_DCB_CHAIN_SET(psp->unit, d, 1);

    /* Disable DMA interrupts - prevent soc_start_dma() from choking */
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(psp->unit, soc_feature_cmicm)) {
        (void)soc_cmicm_intr0_disable(psp->unit,
                        IRQ_CMCx_DESC_DONE(channel) | IRQ_CMCx_CHAIN_DONE(channel));
    } else
#endif
    {
        soc_intr_disable(psp->unit,
                         IRQ_DESC_DONE(channel) | IRQ_CHAIN_DONE(channel));
    }
    if (!psp->poll) {
        /* Setup callout if interrupt */
        dv->dv_done_desc   = pktspeed_done_desc;
        dv->dv_done_chain   = pktspeed_done_chain;
        dv->dv_public1.ptr = (void *)psp;
        dv->dv_flags      |= DV_F_NOTIFY_DSC | DV_F_NOTIFY_CHN;
    }

    cur_time = start_time = sal_time_usecs();
    t_diff = 0;
    last_rpt = 0;

    /* Start DMA */
    if (soc_dma_start(psp->unit, channel, dv) < 0) {
        printk("pktspeed test: Could not start dv, u %d, chan %d\n",
	       psp->unit, channel);
        rv = TEST_FAIL;
        goto done;
    }

    printk("Starting %s reload test:\n",
           tx_test ? "TX" : "RX");
    pktspeed_print_param(psp);
    printk("    %15s %15s %15s\n",
           "Total Pkts", "Pkts/Second", "Bytes/Second");
    while (t_diff <= psp->seconds) {
        if (psp->poll) {
            /* Monitor done bit in last real dcb */
            soc_cm_sinval(psp->unit, d, SOC_DCB_SIZE(psp->unit));
            if (SOC_DCB_DONE_GET(psp->unit, d)) {
                psp->count += psp->chain_len;
                count = psp->count;
                SOC_DCB_DONE_SET(psp->unit, d, 0);
                soc_cm_sflush(psp->unit, d, SOC_DCB_SIZE(psp->unit));
            }
        } else {
            sal_sleep(1);
            count = psp->count;
        }
        /* Report every second */
        if (t_diff >= last_rpt + 1) {
            last_rpt = t_diff;
            pktspeed_print_report(psp, count, t_diff);
        }  
        cur_time = sal_time_usecs();
        t_diff = SAL_USECS_SUB(cur_time, start_time)/1000000;
    }

done:
    if (dv) {
        soc_dma_abort_dv(psp->unit, dv);
        soc_dma_dv_free(psp->unit, dv);
    }
    if (pkt_data) {
        soc_cm_sfree(psp->unit, pkt_data);
    }
    if (pkt) {
        bcm_pkt_free(psp->unit, pkt);
    }
    bcm_port_loopback_set(psp->unit, psp->tx_port, BCM_PORT_LOOPBACK_NONE);
    if (!tx_test) {
        bcm_port_loopback_set(psp->unit, psp->rx_port,
                              BCM_PORT_LOOPBACK_NONE);
    }

    return rv;
}

/*
 * Function:
 *      pktspeed_alloc
 *
 * Purpose:
 *      Allocate and initialize test parameter structure
 *
 * Parameters:
 *      unit            - Unit these parameters are for
 *
 * Returns:
 *      Allocated structure or NULL if alloc failed.
 *
 */

STATIC pktspeed_param_t *
pktspeed_alloc(int unit)
{
    pktspeed_param_t *psp =
      (pktspeed_param_t *)sal_alloc(sizeof(pktspeed_param_t),
                                    "Pktspeed test config");

    if (psp != NULL) {
        psp->unit       = unit;
        if(SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit)) {
            psp->tx_port = 30;
        } else if(SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
            psp->tx_port = 2;
        } else if (SOC_IS_KATANA(unit)) {
            psp->tx_port = (BCM_PBMP_MEMBER(PBMP_PORT_ALL(unit), 1)) ? 1 : 27;
        } else {
            psp->tx_port = 1;
        }
        psp->rx_port    = -1;   /* Select automatically */
        psp->pkt_size   = 68;
        psp->alloc_size = 68;
        psp->chain_len  = 1000;
        psp->seconds    = 10;
        psp->poll       = TRUE;
        COMPILER_32_TO_DOUBLE(psp->count, 0);
    }
    return psp;
}

/*
 * Function:
 *      pktspeed_free
 *
 * Purpose:
 *      Free test parameter structure
 *
 * Parameters:
 *      psp             - parameter structure pointer
 *
 * Returns:
 *      TEST_OK
 *      TEST_FAIL
 *
 */

STATIC int pktspeed_free(pktspeed_param_t *psp)
{
    int rv = TEST_FAIL;

    if (!psp) {
        return rv;
    }
    sal_free(psp);

    return TEST_OK;
}

/*
 * Function:
 *      pktspeed_test_init
 *
 * Purpose:
 *      Parse test arguments and save parameter structure locally
 *
 * Parameters:
 *      unit            - unit to test
 *      args            - test arguments
 *      pa              - test cookie (not used)
 *
 * Returns:
 *      TEST_OK
 *      TEST_FAIL
 *
 */

int
pktspeed_test_init(int unit, args_t *args, void **pa)
{
    parse_table_t	pt;
    pktspeed_param_t    *psp = pktspeed_alloc(unit);

    if (psp == NULL) {
        printk("%s: out of memory\n", ARG_CMD(args));
        return(TEST_FAIL);
    }

    parse_table_init(unit, &pt);

    parse_table_add(&pt, "PORT", PQ_DFL|PQ_PORT, 0,
                    &psp->tx_port, NULL);
    parse_table_add(&pt, "RXport", PQ_DFL|PQ_PORT, 0,
                    &psp->rx_port, NULL);
    parse_table_add(&pt, "PKTsize", PQ_DFL|PQ_INT, 0,
                    &psp->pkt_size, NULL);
    parse_table_add(&pt, "ALLOCsize", PQ_DFL|PQ_INT, 0,
                    &psp->alloc_size, NULL);
    parse_table_add(&pt, "ChainLen", PQ_DFL|PQ_INT, 0,
                    &psp->chain_len, NULL);
    parse_table_add(&pt, "SEConds", PQ_DFL|PQ_INT, 0,
                    &psp->seconds, NULL);

    /* Parse remaining arguments */
    if (0 > parse_arg_eq(args, &pt)) {
	test_error(unit,
		   "%s: Invalid option: %s\n",
		   ARG_CMD(args),
		   ARG_CUR(args) ? ARG_CUR(args) : "*");
	parse_arg_eq_done(&pt);
	return(TEST_FAIL);
    }
    parse_arg_eq_done(&pt);

    if (psp->pkt_size > psp->alloc_size) {
	test_error(unit,
                   "%s: Error: packet size > alloc size\n",
                   ARG_CMD(args));
	parse_arg_eq_done(&pt);
	return(TEST_FAIL);
    }
    parse_arg_eq_done(&pt);

    if (psp->rx_port < 0) {     /* Find an unused port */
        int p;

        /* Find first available port != tx_port */
        PBMP_ITER(PBMP_PORT_ALL(unit), p) {
            if (p != psp->tx_port) {
                psp->rx_port = p;
                break;
            }
        }

        /* Check to see if one was found */
        if (psp->rx_port < 0) {
            test_error(unit,
                       "%s: Could not find an available RX port.\n",
                       ARG_CMD(args));
            return(TEST_FAIL);
        }

    } else {                    /* RX port specified */

        /* Make sure selected RX port is valid */
        if (!BCM_PBMP_MEMBER(PBMP_PORT_ALL(unit), psp->rx_port)) {
            test_error(unit,
                       "%s: Invalid RX port %d.\n",
                       ARG_CMD(args), psp->rx_port);
            return(TEST_FAIL);
        }
    }

    /* Make sure TX port is valid, too */
    if (!BCM_PBMP_MEMBER(PBMP_PORT_ALL(unit), psp->tx_port)) {
        test_error(unit,
                   "%s: Invalid TX port %d.\n",
                   ARG_CMD(args), psp->tx_port);
        return(TEST_FAIL);
    }

    pktspeed_param[unit] = psp;

    return TEST_OK;
}

/*
 * Function:
 *      pktspeed_test_tx
 *
 * Purpose:
 *      Runs TX test
 *
 * Parameters:
 *      unit            - unit to test
 *      a               - test arguments (ignored)
 *      pa              - test cookie (ignored)
 *
 * Returns:
 *      TEST_OK
 *      TEST_FAIL
 *
 */

int
pktspeed_test_tx(int unit, args_t *a, void *pa)
{
    return pktspeed_run_test(TRUE, pktspeed_param[unit]);
}

/*
 * Function:
 *      pktspeed_test_rx
 *
 * Purpose:
 *      Runs RX test
 *
 * Parameters:
 *      unit            - unit to test
 *      a               - test arguments (ignored)
 *      pa              - test cookie (ignored)
 *
 * Returns:
 *      TEST_OK
 *      TEST_FAIL
 *
 */

int
pktspeed_test_rx(int unit, args_t *a, void *pa)
{
    return pktspeed_run_test(FALSE, pktspeed_param[unit]);
}

/*
 * Function:
 *      pktspeed_test_done
 *
 * Purpose:
 *      Cleans up after tx or rx test completes
 *
 * Parameters:
 *      unit            - unit to test
 *      pa              - test cookie (ignored)
 *
 * Returns:
 *      TEST_OK
 *      TEST_FAIL
 */

int
pktspeed_test_done(int unit, void *pa)
{
    return  pktspeed_free(pktspeed_param[unit]);
}

#endif /* BCM_ESW_SUPPORT */
