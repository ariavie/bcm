/*
 * $Id: linkscan.c,v 1.4 Broadcom SDK $
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
 */

#include "vxWorks.h"
#include "intLib.h"
#if (CPU_FAMILY == MIPS)
#include "mbz.h"
#endif
#include "config.h"
#include "stdio.h"
#include "assert.h"
#include "time.h"
#include "taskLib.h"
#include "sysLib.h"
#include "string.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciIntLib.h"
#include "systemInit.h"
#include "dmaOps.h"

static pbmp_t myLinkMask[MAX_DEVICES];

#define soc_link_mask_get(u,m) (*(m)=myLinkMask[u])
#define soc_link_mask_set(u,m) (myLinkMask[u]=(m),0)

int bcm_port_enable_set(int unit, bcm_port_t port, int enable);
int phy_fe_ge_link_get(int unit, soc_port_t port, int *link);

static void ledproc_linkscan_cb(int unit, soc_port_t port, int linkstatus);
int PCI_DEBUG_ON = 0;

static int
_bcm_linkscan_update_port(int unit, int port)
{
    pbmp_t lc_pbm_link;
    int    cur_link, new_link;

    soc_link_mask_get(unit, &lc_pbm_link);
    cur_link = (lc_pbm_link >> port) & 1;

    SOC_IF_ERROR_RETURN(phy_fe_ge_link_get(unit, port, &new_link));

    if (cur_link == new_link) { /* No change */
        return BCM_E_NONE;
    }

    PRINTF_DEBUG2(("MII_STAT : 0x%08x 0x%08x\n", new_link, cur_link));

    /*
     * If disabling, stop ingresses from sending any more traffic to
     * this port.
     */
    if (!new_link) {
        SOC_PBMP_PORT_REMOVE(lc_pbm_link, port);
    } else {
        SOC_PBMP_PORT_ADD(lc_pbm_link, port);
    }

    PRINTF_DEBUG(("SETTING 0x%08x\n", lc_pbm_link));
    PCI_DEBUG_ON = 0;
    soc_link_mask_set(unit, lc_pbm_link);

    ledproc_linkscan_cb(unit, port, new_link);

    /* Program MACs (only if port is not forced) */
    SOC_IF_ERROR_RETURN(bcm_port_enable_set(unit, port, new_link));

    PCI_DEBUG_ON = 0;

    return BCM_E_NONE;
}


static int
_bcm_linkscan_update(int unit)
{
    pbmp_t     save_link_change, new_link;
    bcm_port_t port;

    soc_link_mask_get(unit, &save_link_change);

    HE_PBMP_GE_ITER(unit, port) {
        SOC_IF_ERROR_RETURN(_bcm_linkscan_update_port(unit, port));
    }

    soc_link_mask_get(unit, &new_link);

    if (save_link_change != new_link) {
        WRITE_EPC_LINK_BMAPr(unit, new_link | 0x00000001);
        {
            pbmp_t foo;
            READ_EPC_LINK_BMAPr(unit,&foo);
            PRINTF_DEBUG(("EPC LINK :: %d 0x%08x\n", unit, foo));
        }
    }
    return BCM_E_NONE;
}


#define CMIC_LED_CTRL               0x00001000
#define CMIC_LED_CTRL               0x00001000
#define CMIC_LED_STATUS             0x00001004
#define CMIC_LED_PROGRAM_RAM_BASE   0x00001800
#define CMIC_LED_DATA_RAM_BASE      0x00001c00
#define CMIC_LED_PROGRAM_RAM(_a)    (CMIC_LED_PROGRAM_RAM_BASE + 4 * (_a))
#define CMIC_LED_PROGRAM_RAM_SIZE   0x100
#define CMIC_LED_DATA_RAM(_a)       (CMIC_LED_DATA_RAM_BASE + 4 * (_a))
#define CMIC_LED_DATA_RAM_SIZE      0x100

#define LS_LED_DATA_OFFSET          0x80

static int
ledproc_load(int unit, uint8 *program, int bytes)
{
    int offset;

    SOC_IF_ERROR_RETURN(soc_pci_write(unit, CMIC_LED_CTRL, 0x0));

    for (offset = 0; offset < CMIC_LED_PROGRAM_RAM_SIZE; offset++) {
        SOC_IF_ERROR_RETURN(soc_pci_write(unit,
            CMIC_LED_PROGRAM_RAM(offset),
            (offset < bytes) ? (uint32) program[offset] : 0));
    }

    for (offset = 0x80; offset < CMIC_LED_DATA_RAM_SIZE; offset++) {
        SOC_IF_ERROR_RETURN(soc_pci_write(unit,CMIC_LED_DATA_RAM(offset),0));
    }

    SOC_IF_ERROR_RETURN(soc_pci_write(unit, CMIC_LED_CTRL, 0x1));

    return SOC_E_NONE;
}


static void
ledproc_linkscan_cb(int unit, soc_port_t port, int linkstatus)
{
}


void
linkScanThread(void *opt)
{
#if 0
    int unit = (int)opt;

    while (1) {
        if (_bcm_linkscan_update(unit) != 0) {
            PRINTF_ERROR(("LINKSCAN TASK FAIL........ terminating"));
            break;
        }
        sal_usleep(100 * MILLISECOND_USEC);
    }
#endif
    return;
}


int
myLinkScan(int unit)
{
    if (sal_thread_create("LinkScan", SAL_THREAD_STKSZ,
                           8, linkScanThread, (void*)unit) == NULL) {
        PRINTF_ERROR(("FAIL TO INITIALIZE LINKSCANTHREAD\n"));
        return -1;
    }

    return 0;
}
