/*
 * $Id: stat.c,v 1.7 Broadcom SDK $
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
 * File:     stat.c
 * Purpose:
 *
 */
#include <shared/bsl.h>

#include <soc/debug.h>
#include <bcm/types.h>
#include <bcm/stat.h>
#include <bcm_int/tk371x_dispatch.h>
#include <bcm_int/ea/init.h>
#include <bcm_int/ea/stat.h>
#include <bcm_int/ea/port.h>
#include <bcm_int/ea/tk371x/port.h>
#include <bcm/error.h>
#include <soc/types.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/ea/tk371x/onu.h>
#include <soc/ea/tk371x/ea_drv.h>
/*
 * Function:
 *	bcm_tk371x_stat_clear
 * Description:
 *	Clear the port based statistics from the EA port.
 * Parameters:
 *	unit - EA PCI device unit number (driver internal).
 *	port - zero-based port number
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_INTERNAL - Chip access failure.
 */
int
bcm_tk371x_stat_clear(
	    int unit,
	    bcm_port_t port)
{
	int rv = BCM_E_NONE;

	LOG_INFO(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "BCM API : bcm_tk371x_stat_clear..\n")));
	if (0 == BCM_TK371X_UNIT_VALID((unit))){
		LOG_INFO(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "UNIT : %d\n"), (unit)));
		return BCM_E_UNIT;
	}
	if (0 == TK371X_PORT_VALID(port)){
		LOG_INFO(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "PORT : %d\n"), (port)));
		return BCM_E_PORT;
	}

    rv = _bcm_ea_stat_clear(unit, port);
	return rv;
}

/*
 * Function:
 *	bcm_tk371x_stat_get
 * Description:
 *	Get the specified statistic
 * Parameters:
 *	unit - EA PCI device unit number (driver internal).
 *	port - zero-based port number
 *	type - SNMP statistics type (see stat.h)
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_PARAM - Illegal parameter.
 *	BCM_E_INTERNAL - Chip access failure.
 */
int
bcm_tk371x_stat_get(
	    int unit,
	    bcm_port_t port,
	    bcm_stat_val_t type,
	    uint64 *value)
{
	int rv = BCM_E_NONE;

	LOG_INFO(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "BCM API : bcm_tk371x_stat_get..\n")));
	if (0 == BCM_TK371X_UNIT_VALID((unit))){
		LOG_INFO(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "UNIT : %d\n"), (unit)));
		return BCM_E_UNIT;
	}
	if (0 == TK371X_PORT_VALID(port)){
		LOG_INFO(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "PORT : %d\n"), (port)));
		return BCM_E_PORT;
	}

	rv = _bcm_ea_stat_get(unit, port, type, value);

	return rv;
}

/*
 * Function:
 *	bcm_tk371x_stat_get32
 * Description:
 *	Get the specified statistic
 * Parameters:
 *	unit - EA PCI device unit number (driver internal).
 *	port - zero-based port number
 *	type - SNMP statistics type (see stat.h)
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_PARAM - Illegal parameter.
 *	BCM_E_INTERNAL - Chip access failure.
 */
int
bcm_tk371x_stat_get32(
	    int unit,
	    bcm_port_t port,
	    bcm_stat_val_t type,
	    uint32 *value)
{
	int rv = BCM_E_NONE;

	LOG_INFO(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "BCM API : bcm_tk371x_stat_get32..\n")));
	if (0 == BCM_TK371X_UNIT_VALID((unit))){
		LOG_INFO(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "UNIT : %d\n"), (unit)));
		return BCM_E_UNIT;
	}
	if (0 == TK371X_PORT_VALID(port)){
		LOG_INFO(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "PORT : %d\n"), (port)));
		return BCM_E_PORT;
	}

	rv = _bcm_ea_stat_get32(unit, port, type, value);

	return rv;
}

int
bcm_tk371x_stat_init(int unit)
{
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "BCM API : bcm_tk371x_stat_init()..\n")));
	if (0 == BCM_TK371X_UNIT_VALID(unit)){
		return BCM_E_UNIT;
	}
	_bcm_ea_stat_init();
	return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_tk371x_stat_multi_get
 * Purpose:
 *      Get the specified statistics from the device.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) <UNDEF>
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Array of SNMP statistics types defined in bcm_stat_val_t
 *      value_arr - (OUT) Collected statistics values
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_tk371x_stat_multi_get(
	    int unit,
	    bcm_port_t port,
	    int nstat,
	    bcm_stat_val_t *stat_arr,
	    uint64 *value_arr)
{
    int stix;

    if (nstat <= 0) {
        return BCM_E_PARAM;
    }

    if ((NULL == stat_arr) || (NULL == value_arr)) {
        return BCM_E_PARAM;
    }

    for (stix = 0; stix < nstat; stix++) {
        BCM_IF_ERROR_RETURN
            (bcm_stat_get(unit, port, stat_arr[stix],
                              &(value_arr[stix])));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tk371x_stat_multi_get32
 * Purpose:
 *      Get the specified statistics from the device.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) <UNDEF>
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Array of SNMP statistics types defined in bcm_stat_val_t
 *      value_arr - (OUT) Collected statistics values
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_tk371x_stat_multi_get32(
		int unit,
		bcm_port_t port,
		int nstat,
		bcm_stat_val_t *stat_arr,
		uint32 *value_arr)
{
    int stix;
    if (nstat <= 0) {
        return BCM_E_PARAM;
    }

    if ((NULL == stat_arr) || (NULL == value_arr)) {
        return BCM_E_PARAM;
    }

    for (stix = 0; stix < nstat; stix++) {
        BCM_IF_ERROR_RETURN
            (bcm_stat_get32(unit, port, stat_arr[stix],
                                &(value_arr[stix])));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_tk371x_stat_sync
 * Description:
 *	Synchronize software counters with hardware
 * Parameters:
 *	unit - EA device unit number (driver internal).
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_INTERNAL - Chip access failure.
 * Notes:
 *	Makes sure all counter hardware activity prior to the call to
 *	bcm_stat_sync is reflected in all bcm_stat_get calls that come
 *	after the call to bcm_stat_sync.
 */
int
bcm_tk371x_stat_sync(int unit)
{
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "BCM API : bcm_tk371x_stat_sync()..\n")));
	if (0 == BCM_TK371X_UNIT_VALID(unit)){
		return BCM_E_UNIT;
	}
	return _bcm_ea_stat_sync(unit);
}



int _bcm_tk371x_stat_detach(int unit)
{
	_bcm_ea_stat_init();
	return BCM_E_NONE;
}
