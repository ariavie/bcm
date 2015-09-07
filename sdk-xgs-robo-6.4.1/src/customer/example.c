/*
 * $Id: example.c,v 1.2 Broadcom SDK $
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
 * File:	example.c
 * Purpose:     To provide an example on how to add customer-specific APIs 
 *              by placing addtional code in the files in src/customer 
 *              directory
 */

/*
 * Here are the typical include files that might be needed
 */

/* 
 * Asserts really help making the code more robust and easy to debug
 */
#include <assert.h>

/*
 * SAL makes it portable across many platforms. For the driver "add-ons" only
 * the Core SAL is needed.
 */
#include <sal/core/libc.h>

/*
 * If you plan to add some code to BCM shell or another high-level application
 * you might need to use the Applications SAL
 */
#include <sal/appl/io.h>
#include <sal/appl/pci.h>

/*
 * If you plan to access chips at the low-level, you might need to include 
 * the SOC API declarations. This, however might make your code incompatible
 * with the future versions of BCM API
 */
#include <soc/register.h>
#include <soc/mem.h>
#include <soc/drv.h>

/*
 * The best way to go is to add code written on top of the BCM API -- the 
 * best API you can find, anyways!
 */
#include <bcm/port.h>
#include <bcm/vlan.h>
#include <bcm/error.h>
#include <bcm/link.h>

/*
 * And don't forget that you can place your own header files in the 
 * include/customer directory
 */
#include <customer/example.h>

/***************************************************************************/

/*
 * Function:   example_bcm_ok
 * Purpose:    A placeholder
 * Parameters: None
 * Returns:    BCM_E_NONE (always)
 *
 * Notes:      Please, use your company name as a prefix.
 */
int
example_bcm(void)
{
    return BCM_E_NONE;
}
