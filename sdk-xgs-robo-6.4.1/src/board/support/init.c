/* 
 * $Id: init.c,v 1.2 Broadcom SDK $
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
 * File:        init.c
 * Purpose:     device initialization
 *
 */

#include <soc/drv.h>
#include <sal/core/time.h>
#include <sal/core/boot.h>
#include <bcm/init.h>
#include <bcm/port.h>
#include <bcm/link.h>
#include <bcm/stat.h>
#include <bcm/error.h>
#include <board_int/support.h>


#define BCM_IF_NOT_UNAVAIL_ERROR_RETURN(x) { \
  int _rv = x; \
  if (_rv != BCM_E_UNAVAIL) {  BCM_IF_ERROR_RETURN(_rv); } \
} while (0)


/*
 * Function:
 *     board_port_init
 * Purpose:
 *     Initialize ports on a unit to a known state, and initialize
 *     services on the ports
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */

int
board_port_init(int unit)
{
    sal_usecs_t usec;
    bcm_port_config_t  config;
    bcm_port_t port;
    int rv;

    usec = soc_property_get(unit, spn_BCM_LINKSCAN_INTERVAL, 250000);
    BCM_IF_ERROR_RETURN(bcm_linkscan_enable_set(unit, usec));

    BCM_IF_ERROR_RETURN(bcm_port_config_get(unit, &config));

    BCM_PBMP_ITER(config.port, port) {
        int abil;

        rv = bcm_port_stp_set(unit, port, (int)BCM_PORT_STP_FORWARD);
        BCM_IF_NOT_UNAVAIL_ERROR_RETURN(rv);
        
        rv = bcm_port_ability_get(unit, port, (bcm_port_abil_t *)&abil);
        if (BCM_SUCCESS(rv)) {
            rv = bcm_port_advert_set(unit, port, abil);
        }
        BCM_IF_NOT_UNAVAIL_ERROR_RETURN(rv);

        rv = bcm_port_autoneg_set(unit, port, TRUE);
        BCM_IF_NOT_UNAVAIL_ERROR_RETURN(rv);

        rv = bcm_linkscan_mode_set(unit, port, BCM_LINKSCAN_MODE_SW);
        BCM_IF_NOT_UNAVAIL_ERROR_RETURN(rv);

        rv = bcm_stat_clear(unit, port);
        BCM_IF_ERROR_RETURN(rv);

        rv = bcm_port_learn_set(unit, port,
                                BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD);
        BCM_IF_NOT_UNAVAIL_ERROR_RETURN(rv);
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *     board_port_deinit
 * Purpose:
 *     Undo port initialization
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */
int
board_port_deinit(int unit)
{
    return bcm_linkscan_detach(unit);
}

/*
 * Function:
 *     board_device_init
 * Purpose:
 *     Architecture independent device initialization
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */
int
board_device_init(int unit)
{
    BCM_IF_ERROR_RETURN(board_device_reset(unit));
    BCM_IF_ERROR_RETURN(bcm_init(unit));
    if ((SAL_BOOT_SIMULATION) && (!SAL_BOOT_BCMSIM)) {
        /* When in PCID simulation, initialize all the modules needed
           by any of the board drivers that bcm_init() skips. */
        BCM_IF_ERROR_RETURN(bcm_trunk_init(unit));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     board_device_deinit
 * Purpose:
 *     Deinitialize device
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */
int
board_device_deinit(int unit)
{
    /* nothing for now */
    return BCM_E_NONE;
    
}

/*
 * Function:
 *     board_device_cpu_port_get
 * Purpose:
 *     Get the first CPU port of a device
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */
int
board_device_cpu_port_get(int unit, bcm_gport_t *cpu)
{
    bcm_port_config_t port_info;
    int port = -1;
    int rv = BCM_E_NOT_FOUND;

    if (!cpu) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(bcm_port_config_get(unit, &port_info));
    
    BCM_PBMP_ITER(port_info.cpu, port) {
        break;
    }

    if (port >= 0) {
        *cpu = BCM_GPORT_DEVPORT(unit, port);
        rv = BCM_E_NONE;
    }

    return rv;
}
