/*
 * $Id: socdiag.c 1.10 Broadcom SDK $
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
 */

#include <vxWorks.h>
#include <version.h>
#include <kernelLib.h>
#include <sysLib.h>
#include <drv/pci/pciConfigLib.h>

#include <sal/appl/sal.h>
#include <soc/debug.h>
#include <sal/appl/io.h>
#include <sal/core/boot.h>
#include <sal/core/thread.h>

#if defined(USE_SIBYTE_INTR)

int total_soc_device = 0;
volatile int IN_SOC_INTR_INVOKE = 0;

void SOC_INTR_INVOKE(void)
{
    volatile int    s;
    int u;

    if (IN_SOC_INTR_INVOKE) {
        return;
    }

    s = sal_splhi();
    IN_SOC_INTR_INVOKE = 1;
    sal_spl(s);
    for (u = 0; u < total_soc_device; u++) {
        soc_intr(u);
    }
    s = sal_splhi();
    IN_SOC_INTR_INVOKE = 0;
    sal_spl(s);
}
#else
#define SOC_INTR_INVOKE
#endif

/*ARGSUSED*/
static void
led_string(void *p)
{
    uint32 period = (1000000 / 3);
    int blue_led = 0;

    for (;;) {
	sal_led_string("BCM\\");
	sal_usleep(period / 4);
	sal_led_string("BCM|");
	sal_usleep(period / 4);
	sal_led_string("BCM/");
	sal_usleep(period / 4);
	sal_led_string("BCM-");
	sal_usleep(period / 4);
        sal_led(blue_led);
        blue_led = !blue_led;
    }
}

/* 
 * Create the BCM SiByte1125/1250 MIPS bde
 */

#include "bde/vxworks/vxbde.h"

ibde_t *bde;

/*
 * Temp - display raw PCI header
 */
void
read_pci_conf(int bus, int dev, int func, int begin, int end)
{
    int i;
    unsigned int pdata;

    printf("bus %d dev %d func %d\n", bus, dev, func);
    for(i = begin; i <= end; i += 4) {
        pciConfigInLong(bus, dev, func, i, &pdata);
        printf("REG%02x = 0x%08x\n", i, pdata);
    }
    return;
}

int
bde_create(void)
{	
    vxbde_bus_t bus; 
    int rv;

#if 0
    bus.base_addr_start = 0x41000000;
#else
    /* Set base_addr_start to 0 so we don't re-write the SOC
     * PCI BARs in vxbde_create().
     */
    bus.base_addr_start = 0x0;
#endif
    bus.int_line = 0x38;

#ifdef MIPSEL
    bus.be_pio = 1;
    bus.be_packet = 0;
    bus.be_other = 0;
#else
    bus.be_pio = 1;
    bus.be_packet = 0;
    bus.be_other = 1;
#endif

    rv = vxbde_create(&bus, &bde);

#if defined(USE_SIBYTE_INTR)
    total_soc_device = bde->num_devices();
#endif

    return rv;
}

/*
 * Main: start the diagnostics and CLI shell under vxWorks.
 */
void vxSpawn(void)
{
    extern void diag_shell(void *);
    sal_core_init();
    sal_appl_init();
#ifdef DEBUG_STARTUP
    debugk_select(DEBUG_STARTUP);
#endif
    printk ("SOC BIOS (VxWorks) %s v%s.\n", sysModel(), vxWorksVersion);
    printk ("Kernel: %s.\n", kernelVersion());
    printk ("Made on %s.\n", creationDate);
    sal_thread_create("bcmCLI", 128*1024, 100, diag_shell, 0);
    sal_thread_create("bcmLED", 8192, 99, led_string, 0);
}
