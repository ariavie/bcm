/*
 * $Id: cmext.h,v 1.3 Broadcom SDK $
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

#ifndef _SOC_CMEXT_H
#define _SOC_CMEXT_H

#include <soc/devids.h>
#include <soc/cmtypes.h>

extern int soc_cm_init(void);
extern int soc_cm_deinit(void);
/*
 * DRIVER/DEVICE INITIALIZATION
 * ----------------------------------------
 * 
 * Step One: Device/Revision Ids
 * -------------------------------
 * Each Broadcom device can be identified by 2 values -- its
 * Device ID and its Revision ID. The combination of these two values
 * specify a particular Broadcom device. 
 *
 * Before the BCM driver is assigned the task of driving 
 * a Broadcom device, you must ascertain the Device ID and the 
 * Revision ID for the device to be driven (for a PCI device, these
 * are in the PCI Configuration Space). 
 *
 *
 * After the Device ID and Revision ID are discovered (typically at
 * system initialization time), you should query the SOC driver to 
 * discover whether it supports the discovered device using the following 
 * function:
 */

/*
 * Syntax:     int soc_cm_device_supported(uint16 device_id,
 *					   uint8 revision_id)
 *
 * Purpose:    Determine whether this driver supports a particular device. 
 * 
 * Parameters:
 *             device_id    -- The 16-bit device id for the device. 
 *             revision_id  -- The 8-bit revision id for the device. 
 * 
 * Returns:
 *             0 if the device is supported. 
 *             < 0 if the device is not supported. 
 */

extern int soc_cm_device_supported(uint16 dev_id, uint8 rev_id);

/*
 * Step Two: Device Creation
 * -----------------------------------------
 * When you have a device that the SOC driver supports, 
 * You must create a driver device for it. 
 * You create a driver device by calling cm_device_create() with
 * the device and revision ids. 
 *
 * cm_device_create() will return a handle that should be used
 * when referring to the device in the future. 

 * cm_device_create_id() might be used if application want to 
 * force a speicifc handle when referring to the device in the future.
 * Passing -1 as a "dev" parameter will force api to allocate a new handle.
 */

extern int soc_cm_device_create(uint16 dev_id, uint16 rev_id, void *cookie);
extern int soc_cm_device_create_id(uint16 dev_id, uint16 rev_id, 
                                   void *cookie, int dev);


/*
 * Step Three: Device Initialization
 * -----------------------------------------
 * After you have initialized a device, 
 * you must provide several accessor functions that will be
 * used by the SOC driver to communicate with the device at 
 * the lowest level -- the SOC driver will access the device
 * through these routines, and these routines only. 
 *
 * You provide these access vectors to the driver
 * by filling in the 'cm_device_t' structure. See <cmtypes.h>
 */

extern int soc_cm_device_init(int dev, soc_cm_device_vectors_t *vectors);

extern int soc_cm_device_destroy(int dev);

#endif	/* !_SOC_CMEXT_H */
