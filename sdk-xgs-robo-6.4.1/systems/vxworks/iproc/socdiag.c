/*
 * $Id: socdiag.c,v 1.9 Broadcom SDK $
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


#include <sal/appl/sal.h>
#include <soc/debug.h>
#include <sal/appl/io.h>
#include <sal/core/boot.h>
#include <sal/core/thread.h>



/* 
 * Create the iProc/KT2 ARM bde
 */

#include "bde/vxworks/vxbde.h"
#define IPROC_CMICD_BASE        (0x48000000)
ibde_t *bde;

int
bde_create(void)
{	
    vxbde_bus_t bus; 

    /* Refer to _iproc_device_create() in linux-kernel-bde.c */
    bus.base_addr_start = IPROC_CMICD_BASE;
    bus.int_line = 194;

    /* ARM Little Endian, Refer to $sdk\make\Makefile.linux-iproc-2_6 */
    bus.be_pio = 0;
    bus.be_packet = 0;
    bus.be_other = 0;

    return vxbde_create(&bus, &bde);
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
    sal_thread_create("bcmCLI", 128*1024,100, diag_shell, 0);
}
