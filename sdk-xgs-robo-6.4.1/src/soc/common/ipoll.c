/*
 * $Id: ipoll.c,v 1.6 Broadcom SDK $
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
 * Functions for polling IRQs instead of using hardware interrupts.
 * Primarily intended for board bringup and debugging.
 *
 */

#include <sal/core/libc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>

#include <soc/drv.h>
#include <soc/ipoll.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif

#ifndef IPOLL_THREAD_DELAY
#define IPOLL_THREAD_DELAY      0
#endif

#ifndef IPOLL_THREAD_PRIO
#define IPOLL_THREAD_PRIO       100
#endif

typedef struct ipoll_ctrl_s {
    void *data;
    ipoll_handler_t handler;
    int paused;
} ipoll_ctrl_t;

static ipoll_ctrl_t _ictrl[SOC_MAX_NUM_DEVICES];
static int _ihandlers;

#define CMCx_IRQ0_STAT(_dev,_cmc) \
    soc_pci_read(_dev, CMIC_CMCx_IRQ_STAT0_OFFSET(_cmc))
#define CMCx_IRQ1_STAT(_dev,_cmc) \
    soc_pci_read(_dev, CMIC_CMCx_IRQ_STAT1_OFFSET(_cmc))
#define CMCx_IRQ2_STAT(_dev,_cmc) \
    soc_pci_read(_dev, CMIC_CMCx_IRQ_STAT2_OFFSET(_cmc))
#define CMCx_IRQ3_STAT(_dev,_cmc) \
    soc_pci_read(_dev, CMIC_CMCx_IRQ_STAT3_OFFSET(_cmc))
#define CMCx_IRQ4_STAT(_dev,_cmc) \
    soc_pci_read(_dev, CMIC_CMCx_IRQ_STAT4_OFFSET(_cmc))

/*
 * Function:
 *      soc_cmic_ipoll_handler
 * Description:
 *      Main CMIC interrupt handler in polled IRQ mode
 * Parameters:
 *      dev - device number
 * Returns:
 *      Nothing
 */
STATIC void
soc_cmic_ipoll_handler(int dev)
{
    if (soc_pci_read(dev, CMIC_IRQ_STAT) & SOC_IRQ_MASK(dev)) {
        _ictrl[dev].handler(_ictrl[dev].data);
    } else {
        if ((soc_feature(dev, soc_feature_extended_cmic_error)) ||
            (soc_feature(dev, soc_feature_short_cmic_error))) {
            if(soc_pci_read(dev, CMIC_IRQ_STAT_1) & SOC_IRQ1_MASK(dev) ||
               soc_pci_read(dev, CMIC_IRQ_STAT_2) & SOC_IRQ2_MASK(dev)) {
                _ictrl[dev].handler(_ictrl[dev].data);
            } 
        }
    }
}

/*
 * Function:
 *      soc_cmicm_ipoll_handler
 * Description:
 *      Main CMICm interrupt handler in polled IRQ mode
 * Parameters:
 *      dev - device number
 * Returns:
 *      Nothing
 */
STATIC void
soc_cmicm_ipoll_handler(int dev)
{
#ifdef BCM_CMICM_SUPPORT
    int cmc;

    cmc = SOC_PCI_CMC(dev);
#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || \
    defined(BCM_ARAD_SUPPORT) 
    if ((CMCx_IRQ0_STAT(dev, cmc) & SOC_CMCx_IRQ0_MASK(dev, cmc)) || 
        (CMCx_IRQ1_STAT(dev, cmc) & SOC_CMCx_IRQ1_MASK(dev, cmc)) ||
        (CMCx_IRQ2_STAT(dev, cmc) & SOC_CMCx_IRQ2_MASK(dev, cmc))) {
        _ictrl[dev].handler(_ictrl[dev].data);
    } else {
        if (soc_feature(dev, soc_feature_cmicm_multi_dma_cmc)) {
            if ((CMCx_IRQ0_STAT(dev, SOC_ARM_CMC(dev, 0)) & 
                 SOC_CMCx_IRQ0_MASK(dev, SOC_ARM_CMC(dev, 0))) || 
                (CMCx_IRQ0_STAT(dev, SOC_ARM_CMC(dev, 1)) &
                 SOC_CMCx_IRQ0_MASK(dev, SOC_ARM_CMC(dev, 1)))) {
                _ictrl[dev].handler(_ictrl[dev].data);
            }
        }
        if (soc_feature(dev, soc_feature_extended_cmic_error) ||
            soc_feature(dev, soc_feature_short_cmic_error)) {
            if ((CMCx_IRQ3_STAT(dev, cmc) & SOC_CMCx_IRQ3_MASK(dev,cmc)) ||
                (CMCx_IRQ4_STAT(dev, cmc) & SOC_CMCx_IRQ4_MASK(dev,cmc))) {
                _ictrl[dev].handler(_ictrl[dev].data);
            }
        }
    }
#endif                
#endif
}

/*
 * Function:
 *      soc_ipoll_thread
 * Description:
 *      Thread context for interrupt handlers in polled IRQ mode
 * Parameters:
 *      data - poll delay in usecs (passed as pointer)
 * Returns:
 *      Nothing
 */
STATIC void
soc_ipoll_thread(void *data)
{
    int dev, spl, udelay;

    udelay = PTR_TO_INT(data);

    while (_ihandlers) {

        spl = sal_splhi();

        for (dev = 0; dev < SOC_MAX_NUM_DEVICES; dev++) {
            if (_ictrl[dev].handler != NULL && !_ictrl[dev].paused) {
                if (soc_feature(dev, soc_feature_cmicm)) {
                    soc_cmicm_ipoll_handler(dev);
                } else {
                    soc_cmic_ipoll_handler(dev);
                }
            }
        }

        sal_spl(spl);

        if (udelay) {
            sal_usleep(udelay);
        } else {
            sal_thread_yield();
        }
    }
    sal_thread_exit(0);
}

/*
 * Function:
 *      soc_ipoll_connect
 * Description:
 *      Install interrupt handler in polled IRQ mode
 * Parameters:
 *      dev - device number
 *      handler - interrupt handler
 *      data - interrupt handler data
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_XXX - an error occurred
 * Notes:
 *      The first handler installed be cause the polling
 *      thread to be created.
 */
int
soc_ipoll_connect(int dev, ipoll_handler_t handler, void *data)
{
    int spl;
    int udelay, prio;
    int start_thread = 0;

    if (dev >= SOC_MAX_NUM_DEVICES) {
        return SOC_E_PARAM;
    }

    spl = sal_splhi();

    if (_ictrl[dev].handler == NULL) {
        if (_ihandlers++ == 0) {
            /* Create thread if first handler */
            start_thread = 1;
        }
    }
    _ictrl[dev].handler = handler;
    _ictrl[dev].data = data;
    _ictrl[dev].paused = 0;

    sal_spl(spl);

    if (start_thread) {
        udelay = soc_property_get(dev, spn_POLLED_IRQ_DELAY, IPOLL_THREAD_DELAY);
        prio = soc_property_get(dev, spn_POLLED_IRQ_PRIORITY, IPOLL_THREAD_PRIO);
        sal_thread_create("bcmPOLL",
                          SAL_THREAD_STKSZ, prio,
                          soc_ipoll_thread, INT_TO_PTR(udelay));
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_ipoll_disconnect
 * Description:
 *      Uninstall interrupt handler from polled IRQ mode
 * Parameters:
 *      dev - device number
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_XXX - an error occurred
 * Notes:
 *      When the last handler is uninstalled the polling
 *      thread will terminate.
 */
int
soc_ipoll_disconnect(int dev)
{
    int spl;

    if (dev >= SOC_MAX_NUM_DEVICES) {
        return SOC_E_PARAM;
    }

    spl = sal_splhi();

    if (_ictrl[dev].handler != NULL) {
        _ictrl[dev].handler = NULL;
        _ictrl[dev].data = NULL;
        _ihandlers--;
    }

    sal_spl(spl);

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_ipoll_pause
 * Description:
 *      Temporarily suspend IRQ polling
 * Parameters:
 *      dev - device number
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_XXX - an error occurred
 */
int
soc_ipoll_pause(int dev)
{
    int spl;

    if (dev >= SOC_MAX_NUM_DEVICES) {
        return SOC_E_PARAM;
    }

    spl = sal_splhi();

    _ictrl[dev].paused++;

    sal_spl(spl);

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_ipoll_continue
 * Description:
 *      Resume suspended IRQ polling
 * Parameters:
 *      dev - device number
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_XXX - an error occurred
 */
int
soc_ipoll_continue(int dev)
{
    int spl;

    if (dev >= SOC_MAX_NUM_DEVICES) {
        return SOC_E_PARAM;
    }

    spl = sal_splhi();

    _ictrl[dev].paused--;

    sal_spl(spl);

    return SOC_E_NONE;
}
