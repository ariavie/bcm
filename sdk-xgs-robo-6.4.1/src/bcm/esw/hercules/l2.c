/*
 * $Id: l2.c,v 1.15 Broadcom SDK $
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
 * File:        l2.c
 * Purpose:     BCM layer function driver
 * Notes:	Mostly empty for Hercules (does not have L2 table)
 */

#include <bcm/l2.h>
#include <bcm/port.h>
#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/hercules.h>

int
bcm_hercules_l2_addr_add(int unit, bcm_l2_addr_t *l2addr)
{
    return (BCM_E_UNAVAIL);
}

int
bcm_hercules_l2_addr_delete(int unit, bcm_mac_t mac, bcm_vlan_t vid)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_l2_addr_get(int unit, sal_mac_addr_t mac,
			 bcm_vlan_t vid, bcm_l2_addr_t *l2addr)
{
    return (BCM_E_UNAVAIL);
}

/*
 * Function:
 *	bcm_hercules_l2_init
 * Description:
 *	Initialize chip-dependent parts of L2 module
 * Parameters:
 *	unit        - StrataSwitch unit number.
 */

int
bcm_hercules_l2_init(int unit)
{
    COMPILER_REFERENCE(unit);

    return SOC_E_NONE;
}

/*
 * Function:
 *	bcm_hercules_l2_term
 * Description:
 *	Finalize chip-dependent parts of L2 module
 * Parameters:
 *	unit        - StrataSwitch unit number.
 */

int
bcm_hercules_l2_term(int unit)
{
    COMPILER_REFERENCE(unit);

    return SOC_E_NONE;
}

int
bcm_hercules_l2_conflict_get(int unit, bcm_l2_addr_t *addr,
                          bcm_l2_addr_t *cf_array, int cf_max,
                          int *cf_count)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(addr);
    COMPILER_REFERENCE(cf_array);
    COMPILER_REFERENCE(cf_max);
    COMPILER_REFERENCE(cf_count);

    return BCM_E_UNAVAIL;
}

