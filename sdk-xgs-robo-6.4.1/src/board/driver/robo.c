/* 
 * $Id: robo.c,v 1.2 Broadcom SDK $
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
 * File:        robo.c
 * Purpose:     generic ROBO board driver
 *
 */

#include <shared/bsl.h>

#include <soc/drv.h> /* SOC_IS_ROBO */
#include <sal/core/alloc.h>
#include <sal/core/libc.h>
#include <bcm/error.h>
#include <bcm/stack.h>

#include <board/board.h>
#include <board/manager.h>
#include <board/support.h>
#include <board/driver.h>

#if defined (INCLUDE_BOARD_ROBO)

#define MAX_DESC 80
#define MAX_UNIT 1
#define UNIT 0

#define PRIVATE(field) (((robo_private_data_t *)(driver->user_data))->field)

typedef struct robo_private_data_s {
    char description[MAX_DESC];
    bcm_info_t info;
} robo_private_data_t;

STATIC robo_private_data_t _robo_private_data;

/*
 * Function:
 *     _robo_num_modid_get
 * Purpose:
 *     Get the number of modids required by this board.
 *     ROBO devices have no modids.
 * Parameters:
 *     driver  - (IN)  current driver (ignored)
 *     name    - (IN)  attribute name (ignored)
 *     modid   - (OUT) modid
 * Returns:
 *     BCM_E_NONE - success
 */
STATIC int
_robo_num_modid_get(board_driver_t *driver, char *name, int *num_modid)
{
    *num_modid = 0;
    return BCM_E_NONE;
}

/*
 * Function:
 *     _robo_description
 * Purpose:
 *     Return the board description string
 * Parameters:
 *     driver - (IN)  current driver
 * Returns:
 *     description string
 */
STATIC char *
_robo_description(board_driver_t *driver)
{
    return PRIVATE(description);
}

/*
 * Function:
 *     _robo_probe
 * Purpose:
 *     Test if board devices are appropriate for this board.
 *     All ROBO devices are appropriate.
 * Parameters:
 *      driver - (IN) board driver
 *      num    - (IN) number of devices; length of info array
 *      info   - (IN) array of bcm_info_t
 * Returns:
 *     BCM_E_NONE - devices match what is expected for this board
 *     BCM_E_XXX - failed
 */
STATIC int
_robo_probe(board_driver_t *driver, int num, bcm_info_t *info)
{
    int rv = BCM_E_FAIL;

    
    if (num == MAX_UNIT && SOC_IS_ROBO(0)) {
        LOG_VERBOSE(BSL_LS_BOARD_COMMON,
                    (BSL_META(__FILE__": accept\n")));
        rv = BCM_E_NONE;
    } else {
        LOG_VERBOSE(BSL_LS_BOARD_COMMON,
                    (BSL_META(__FILE__": reject - num_units(%d) != %d\n"),
                     num,
                     MAX_UNIT));
    }

    return rv;
}

/*
 * Function:
 *     _robo_start
 * Purpose:
 *     Start board according to flags
 * Parameters:
 *      driver - (IN) current driver
 *      flags  - (IN) start flags
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */
STATIC int
_robo_start(board_driver_t *driver, uint32 flags)
{

    /* init the unit */
    if (flags & BOARD_START_F_WARM_BOOT) {
        return BCM_E_UNAVAIL;
    } else {
        BCM_IF_ERROR_RETURN(board_device_init(UNIT));
        BCM_IF_ERROR_RETURN(board_port_init(UNIT));
    }
    
    if (!(flags & (BOARD_START_F_CLEAR|BOARD_START_F_WARM_BOOT))) {

        /* Cold start */

        /* Reset user data */
        sal_memset(driver->user_data, 0, sizeof(robo_private_data_t));

        BCM_IF_ERROR_RETURN(bcm_info_get(UNIT, &PRIVATE(info)));

        sal_sprintf(PRIVATE(description),
                    "Robo %s board driver",
                    board_device_name(UNIT));

    } else if (flags & BOARD_START_F_CLEAR) {
        /* nothing to clear */
    }

    LOG_VERBOSE(BSL_LS_BOARD_COMMON,
                (BSL_META(__FILE__": started %s - %s\n"),
                 robo_board.name,
                 PRIVATE(description)));
    
    return BCM_E_NONE;
}

/*
 * Function:
 *     _robo_stop
 * Purpose:
 *     Stop the board
 * Parameters:
 *      driver - (IN) current driver
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */
STATIC int
_robo_stop(board_driver_t *driver)
{
    BCM_IF_ERROR_RETURN(board_port_deinit(UNIT));
    BCM_IF_ERROR_RETURN(board_device_deinit(UNIT));

    /* Reset user data */
    sal_memset(driver->user_data, 0, sizeof(robo_private_data_t));

    return BCM_E_NONE;
}

/* Supported attributes */
STATIC board_attribute_t _robo_attrib[] = {
    {
        BOARD_ATTRIBUTE_NUM_MODID,
        _robo_num_modid_get,
        NULL
    }
};

/* Board driver */
board_driver_t robo_board = {
    "robo",
    BOARD_GENERIC_PRIORITY+1,
    (void *)&_robo_private_data,
    0,
    NULL,
    COUNTOF(_robo_attrib),
    _robo_attrib,
    _robo_description,
    _robo_probe,
    _robo_start,
    _robo_stop
};

#endif /* INCLUDE_BOARD_ROBO */
