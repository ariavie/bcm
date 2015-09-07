/*
 * In strata Switch, this file is auto-generated from the
 * registers file. But not the same with Strata Switch,
 * BCM47XX has no such generator/tool to prepare this
 *
 * $Id: bcm4713_a0.c,v 1.7 Broadcom SDK $
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
 * File:	bcm4713_a0.i
 * Purpose:	bcm4713_a0 chip specific information (register)
 *
 * Note :
 */


#include <sal/core/libc.h>
#include <soc/defs.h>
#include <soc/mem.h>
#include <soc/robo/mcm/driver.h>
#include <soc/register.h>
#include <soc/robo/mcm/allfields.h>
#include <soc/counter.h>
#include <soc/memory.h>


/* Forward declaration of init function */
static void chip_init_bcm4713_a0(void);

static soc_block_info_t soc_blocks_bcm4713_a0[] = {
	{ -1,		-1,	-1,	-1	}	/* end */
};

soc_driver_t soc_driver_bcm4713_a0 = {
	/* type         */	SOC_CHIP_BCM4713_A0,
	/* chip_string  */	"BCM4713",
	/* origin       */	"HND",
	/* pci_vendor   */	BROADCOM_VENDOR_ID,
	/* pci_device   */	BCM4713_DEVICE_ID,
	/* pci_revision */	BCM4713_A0_REV_ID,
	/* num_cos      */	0,
	/* reg_info     */	NULL,
        /* reg_above_64_info */ NULL,
        /* reg_array_info */    NULL,
	/* mem_info     */	NULL,
        /* mem_aggr     */      NULL,
        /* mem_array_info */    NULL,
	/* block_info   */	soc_blocks_bcm4713_a0,
	/* port_info    */	NULL,
	/* counter_maps */	NULL,
	/* features     */	NULL,
	/* init         */	chip_init_bcm4713_a0,
       /* soc_driver */ NULL,
};

/* Chip specific bottom matter from memory file */

/****************************************************************
 *
 * Function:    chip_init_bcm5338_a0
 * Purpose:
 *     Initialize software internals for bcm5690_a0 device:
 *         Initialize null memories
 * Parameters:  void
 * Returns:     void
 *
 ****************************************************************/
static void
chip_init_bcm4713_a0(void)
{

}

/* End of chip specific bottom matter */
