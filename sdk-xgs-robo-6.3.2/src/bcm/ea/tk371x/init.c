/*
 * $Id: init.c 1.12 Broadcom SDK $
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
 * File:     init.c
 * Purpose:
 *
 */

#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/core/boot.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>

#include <bcm_int/control.h>
#include <bcm_int/common/lock.h>
#include <bcm_int/common/family.h>
#include <bcm_int/ea/init.h>
#include <bcm_int/ea/tk371x.h>

#include <bcm_int/control.h>
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/tk371x_dispatch.h>

#include <bcm_int/ea/tk371x/stat.h>
#include <bcm_int/ea/tk371x/port.h>

#include <bcm/types.h>
#include <bcm/init.h>
#include <bcm/error.h>
#include <bcm/debug.h>
#include <bcm/vlan.h>
#include <bcm/port.h>
#include <bcm/stat.h>
#include <bcm/l2.h>
#include <bcm/cosq.h>
#include <bcm/field.h>

static int bcm_init_flag = 0;

#define CLEAR_CALL(_rtn, _name) {                                       \
        int rv;                                                         \
        rv = (_rtn)(unit);                                              \
        if (rv < 0 && rv != BCM_E_UNAVAIL) {                            \
            soc_cm_debug(DK_ERR, "bcm_clear %d: %s failed %d. %s\n",    \
                         unit, _name, rv, bcm_errmsg(rv));              \
            return rv;                                                  \
        }                                                               \
}

int bcm_tk371x_clear(int unit)
{
	if (0 == BCM_TK371X_UNIT_VALID(unit)){
		return BCM_E_UNIT;
	}
    CLEAR_CALL(bcm_port_init, "port");
    CLEAR_CALL(bcm_l2_init, "L2");
    CLEAR_CALL(bcm_stat_init, "STAT");
    CLEAR_CALL(bcm_cosq_init, "COSQ");
    CLEAR_CALL(bcm_field_init, "field");
	return BCM_E_NONE;
}

/*
 * Function:
 *  bcm_tk371x_info_get
 * Purpose:
 *  Provide unit information to caller
 * Parameters:
 *  unit - ethernet access device
 *  info - (OUT) bcm unit info structure
 * Returns:
 *  BCM_E_XXX
 */
int bcm_tk371x_info_get(
	    int unit,
	    bcm_info_t *info)
{
	int rv = BCM_E_NONE;
	if (0 == BCM_TK371X_UNIT_VALID(unit)){
		return BCM_E_UNIT;
	}

	if (info == NULL){
		soc_cm_print("BCM_E_PARAM = %d\n", BCM_E_PARAM);
		return BCM_E_PARAM;
	}

	rv = _bcm_ea_info_get(unit, info);
	return rv;
}

/*
 * Function:
 *  bcm_tk371x_init
 * Purpose:
 *  Initialize the BCM API layer only, without reseting
 *  ethernet access chip.
 * Parameters:
 *  unit - Ethernet Access unit #.
 * Return:
 *  BCM_E_XXX
 */
int
bcm_tk371x_init(int unit)
{
	int rv = BCM_E_NONE;
    
	if (0 == BCM_TK371X_UNIT_VALID(unit)){
		return BCM_E_UNIT;
	}
	BCM_IF_ERROR_RETURN(bcm_chip_family_set(unit, BCM_FAMILY_TK371X));
	BCM_IF_ERROR_RETURN(bcm_tk371x_port_init(unit));
	BCM_IF_ERROR_RETURN(bcm_tk371x_l2_init(unit));
	BCM_IF_ERROR_RETURN(bcm_tk371x_cosq_init(unit));
	BCM_IF_ERROR_RETURN(bcm_tk371x_stat_init(unit));
	BCM_IF_ERROR_RETURN(bcm_tk371x_field_init(unit));
	bcm_init_flag = 1;
	return rv;
}

/*
 * Function:
 *  bcm_tk371x_init_check
 * Purpose:
 *  Return TRUE if bcm_init_bcm has already been called and succeeded
 * Parameters:
 *  unit - Ethernet Access unit #.
 * Return:
 *  BCM_E_XXX
 */
int
bcm_tk371x_init_check(int unit)
{
	if ((0 != BCM_TK371X_UNIT_VALID(unit)) && (1 == bcm_init_flag)){
		return BCM_E_NONE;
	}else{
		return BCM_E_FAIL;
	}
}

/*
 * Function:
 *  bcm_tk371x_init_selective
 * Purpose:
 *  Initialize specific bcm modules as desired.
 * Parameters:
 *  unit - Ethernet access chipsets unit #.
 *  flags - Combination of bit selectors (see init.h)
 * Returns:
 *  BCM_E_XXX
 */
int
bcm_tk371x_init_selective(
	    int unit,
	    uint32 flags)
{
	if (0 == BCM_TK371X_UNIT_VALID(unit)){
		return BCM_E_UNIT;
	}
    switch (flags) {
         case BCM_MODULE_PORT     :
        	 BCM_IF_ERROR_RETURN(bcm_tk371x_port_init(unit));
             break;
         case BCM_MODULE_L2       :
             BCM_IF_ERROR_RETURN(bcm_tk371x_l2_init(unit));
             break;
         case BCM_MODULE_COSQ     :
             BCM_IF_ERROR_RETURN(bcm_tk371x_cosq_init(unit));
             break;
         case BCM_MODULE_STAT     :
			 BCM_IF_ERROR_RETURN(bcm_tk371x_stat_init(unit));
             break;
         case BCM_MODULE_FIELD	  :
        	 BCM_IF_ERROR_RETURN(bcm_tk371x_field_init(unit));
        	 break;
         default:
             return BCM_E_UNAVAIL;
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *  bcm_tk371x_attach
 * Purpose:
 *  Attach and initialize bcm device
 * Parameters:
 *  unit - unit being detached
 * Returns:
 *  BCM_E_XXX
 */
int
_bcm_tk371x_attach(int unit, char *subtype)
{
	int rv = BCM_E_NONE;	
	 
	COMPILER_REFERENCE(subtype);

	BCM_CONTROL(unit)->capability |= BCM_CAPA_LOCAL;
    
	if (0 == BCM_TK371X_UNIT_VALID(unit)){
		return BCM_E_UNIT;
	}
    
	rv  = bcm_tk371x_init(unit);
	if (rv == BCM_E_NONE){
		bcm_info_t info;
		rv = _bcm_ea_info_get(unit, &info);
		if (rv != BCM_E_NONE) {
			return rv;
		}
		/* Initialize bcm layer */
    	BCM_CONTROL(unit)->chip_vendor = info.vendor;
    	BCM_CONTROL(unit)->chip_device = info.device;
    	BCM_CONTROL(unit)->chip_revision = info.revision;	
		BCM_CONTROL(unit)->capability |= BCM_CAPA_SWITCH;
		return BCM_E_NONE;
	}
	return rv;
}

/*
 * Function:
 *	   _bcm_tk371x_modules_deinit
 * Purpose:
 *	   De-initialize bcm modules
 * Parameters:
 *     unit - (IN) BCM device number.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tk371x_modules_deinit(int unit)
{
	int rv;

	rv = bcm_tk371x_field_detach(unit);
	BCM_IF_ERROR_NOT_UNAVAIL_RETURN(rv);

	rv = _bcm_tk371x_stat_detach(unit);
	BCM_IF_ERROR_NOT_UNAVAIL_RETURN(rv);

    rv = bcm_tk371x_cosq_detach(unit);
    BCM_IF_ERROR_NOT_UNAVAIL_RETURN(rv);

    rv = bcm_tk371x_l2_detach(unit);
    BCM_IF_ERROR_NOT_UNAVAIL_RETURN(rv);

    rv = _bcm_tk371x_port_detach(unit);
    BCM_IF_ERROR_NOT_UNAVAIL_RETURN(rv);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tk371x_detach
 * Purpose:
 *      Clean up bcm layer when unit is detached
 * Parameters:
 *      unit - unit being detached
 * Returns:
 *  BCM_E_XXX
 */
int _bcm_tk371x_detach(int unit)
{
	int rv;

	bcm_init_flag = 0;
    /*
     *  Don't move up, holding lock or disabling hw operations
     *  might prevent theads clean exit.
     */
    rv = _bcm_tk371x_modules_deinit(unit);
    BCM_IF_ERROR_NOT_UNAVAIL_RETURN(rv);
    return BCM_E_NONE;
}

/*  _bcm_tk371x_match
 * Purpose:
 *      match BCM control subtype strings for ea types
 * Parameters:
 *      unit - unit being detached
 * Returns:
 *    0 match
 *    !0 no match
 */
int
_bcm_tk371x_match(int unit, char *subtype_a, char *subtype_b)
{
    COMPILER_REFERENCE(unit);
    return sal_strcmp(subtype_a, subtype_b);
}
