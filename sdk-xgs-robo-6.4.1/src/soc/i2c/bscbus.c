/*
 * $Id: bscbus.c,v 1.1.2.7 Broadcom SDK $
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

#ifdef INCLUDE_I2C

#include <sal/types.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/drv.h>
#include <soc/cm.h>
#include <soc/i2c.h>
#include <soc/bsc.h>

#define TEST_DATA	0xfe

/*
 * Function: soc_bsc_attach
 *
 * Purpose: I2C Bus attach routine, main entry point for I2C startup.
 * Initialize the I2C controller configuration for the specified
 * device.
 *
 * Parameters:
 *    unit - device number
 *
 * Returns:
 *	SOC_E_* if negative
 *	count of found devices if positive or zero
 *
 * Notes: Default is Interrupt mode, if both are selected Interrupt is
 *       chosen.
 */
int
soc_bsc_attach(int unit)
{
	soc_bsc_bus_t *bscbus;

	bscbus = BSCBUS(unit);
	if (bscbus == NULL) {
		bscbus = sal_alloc(sizeof(*bscbus), "bsc_bus");
		if (bscbus == NULL) {
			return SOC_E_MEMORY;
		}
		SOC_CONTROL(unit)->bsc_bus = bscbus;
		sal_memset(bscbus, 0, sizeof(*bscbus));
	}

	/* If not yet done, create synchronization semaphores/mutex */
	if (bscbus->bsc_mutex == NULL) {
		bscbus->bsc_mutex = sal_mutex_create("BSC Mutex");
		if (bscbus->bsc_mutex == NULL) {
			return SOC_E_MEMORY;
		}
	}

	/* Set semaphore timeout values */
	bscbus->flags = SOC_BSC_ATTACHED;

	/*
	 * Probe for BSC devices, update device list for detected devices ...
	 */
	return soc_bsc_probe(unit);
}

/*
 * Function: soc_bsc_detach
 *
 * Purpose: BSC detach routine: free resources used by BSC bus driver.
 *
 * Paremeters:
 *    unit - device number
 * Returns:
 *    SOC_E_NONE - no error
 * Notes:
 *    none
 */
int
soc_bsc_detach(int unit)
{
	soc_bsc_bus_t *bscbus = BSCBUS(unit);

	if (bscbus != NULL) {
		sal_free(bscbus);
		SOC_CONTROL(unit)->bsc_bus = NULL;
	}

	return SOC_E_NONE;
}
#endif /* INCLUDE_I2C */
