/*
 * $Id: init.c,v 1.6 Broadcom SDK $
 *
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
 * File:     hal_vlan.h
 * Purpose:
 *
 */
#include <shared/bsl.h>

#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/core/boot.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <bcm/init.h>
#include <bcm/error.h>

#include <soc/ea/tk371x/ea_drv.h>
#include <soc/ea/tk371x/onu.h>
#include <bcm_int/ea/init.h>


int
_bcm_ea_clear(int unit)
{
    if (!((unit) >= 0 && ((unit) < (BCM_MAX_NUM_UNITS)))) {
        return BCM_E_UNIT;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 * 	bcm_ea_info_get
 * Purpose:
 *  Provide unit information to caller
 * Parameters:
 *  unit - ethernet access device
 *  info - (OUT) bcm unit info structure
 */
int
_bcm_ea_info_get(
	    int unit,
	    bcm_info_t *info)
{
#ifdef BCM_TK371X_SUPPORT
	uint16 device;
	int rc = BCM_E_NONE;
	uint16 vendor;
	if (ERROR == _soc_ea_jedec_id_get((uint8)unit, 0, &vendor)){
		LOG_CLI((BSL_META_U(unit,
                                    "_soc_ea_jedec_id_get BCM_E_INTERNAL = %d\n"), BCM_E_INTERNAL));
		rc = BCM_E_INTERNAL;
	}
	info->vendor = (int)vendor;

	if (ERROR == _soc_ea_chip_id_get((uint8)unit, 0, &device)){
		LOG_CLI((BSL_META_U(unit,
                                    "_soc_ea_chip_id_get BCM_E_INTERNAL = %d\n"), BCM_E_INTERNAL));
		rc = BCM_E_INTERNAL;
	}
	info->device = (uint32)device;

	if (ERROR == _soc_ea_revision_id_get((uint8)unit, &info->revision)){
		LOG_CLI((BSL_META_U(unit,
                                    "_soc_ea_revision_id_get BCM_E_INTERNAL = %d\n"), BCM_E_INTERNAL));
		rc = BCM_E_INTERNAL;
	}
	info->capability = 0;
    info->capability |= BCM_INFO_SWITCH;
	info->capability |= BCM_INFO_IPMC;
	return rc;
#endif
	return BCM_E_NONE;	
}

void
_bcm_ea_info_t_init(bcm_info_t *info)
{
	if (info != NULL){
		sal_memset(info, 0, sizeof(bcm_info_t));
	}
}


