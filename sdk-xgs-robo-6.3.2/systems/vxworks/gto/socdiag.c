/*
 * $Id: socdiag.c 1.14 Broadcom SDK $
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
#include <gto.h>
#include <stdlib.h>

#include <sal/appl/sal.h>
#include <soc/debug.h>
#include <sal/appl/io.h>
#include <sal/core/boot.h>
#include <sal/core/thread.h>

/*
 * NOTE: The LED blink process in usrConfig.c blinks the LED at a
 * rate dependent on system load. This task simply shows BCM with
 * a swirl on the LCD display.
 */
extern void sysLedOn();
extern void sysLedOff();
/*ARGSUSED*/
static void
led_beat(void *p)
{
    uint32 period = (1000000 / 3);

    for (;;) {
        sysLedOn(); 
        sal_usleep(period);
        sysLedOff();
        sal_usleep(period);
    }
}

/*
 * Create the BMW/MPC8245 bde
 */

#include "bde/vxworks/vxbde.h"

ibde_t *bde;

int
bde_create(void)
{
    vxbde_bus_t bus;
    bus.base_addr_start = 0x00000000;
    bus.int_line = 2;
    bus.be_pio = 1;
    bus.be_packet = 0;
    bus.be_other = 1;

    return vxbde_create(&bus, &bde);
}

int 
bde_mem_test(void)
{
    unsigned int *mem;
    size_t mem_size = 0x20000000; 
    size_t element; 
    size_t count; 

#define MEM_SIZE_64MB  0x4000000

   
    mem = NULL;
    do {
        mem_size = mem_size - MEM_SIZE_64MB;

        mem = malloc(mem_size);
    } while ((mem == NULL) && (mem_size != 0));

    if (mem != NULL) {
        count = mem_size / sizeof(unsigned int);

        printf ("Tesing 0x%08x bytes of memory.\n", mem_size);
        printf ("Start at 0x%08x, end at 0x%08x\n", &mem[0], &mem[count]);
 
        for (element = 0; element < count; element++) {
            mem[element] = (unsigned int)&mem[element];
        }
    
        for (element = 0; element < count; element++) {
            if (mem[element] != (unsigned int)&mem[element]) {
                printf("mem comparison failed at 0x%08x = 0x%08x\n",
                       &mem[element], mem[element]);
            }
        }
        free(mem);
    }
    return 0;
}

/*
 * Main: start the diagnostics and CLI shell under vxWorks.
 */
void
vxSpawn(void)
{
    extern void diag_shell(void *);

#ifdef WIND_DEBUG
    IMPORT taskDelay(int ticks);
    printf("Waiting for CrossWind attach ...\n");
    taskDelay(sysClkRateGet()*60);
#endif
    sal_core_init();
    sal_appl_init();
#ifdef DEBUG_STARTUP
    debugk_select(DEBUG_STARTUP);
#endif
    printk ("SOC BIOS (VxWorks) %s v%s.\n", sysModel(), vxWorksVersion);
    printk ("Kernel: %s.\n", kernelVersion());
    printk ("Made on %s.\n", creationDate);
    sal_thread_create("bcmCLI", 128*1024,100, diag_shell, 0);
    sal_thread_create("bcmLED", 8192, 99, led_beat, 0);
}

