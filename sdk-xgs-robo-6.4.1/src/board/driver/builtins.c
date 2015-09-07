/*
 * $Id: builtins.c,v 1.1 Broadcom SDK $
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
 * File:        builtins.c
 * Purpose:     board driver loader
 *
 */

#include <sal/core/libc.h>
#include <bcm/error.h>
#include <board/board.h>
#include <board/driver.h>

/* Built in driver list */
STATIC board_driver_t *_board_builtin_driver[] = {
#if defined(INCLUDE_BOARD_GENERIC)
    &generic_board,
#endif /* INCLUDE_BOARD_GENERIC */
#if defined(INCLUDE_BOARD_BCM956504R48)
    &bcm956504r48_board,
#endif /* INCLUDE_BOARD_BCM956504R48 */
#if defined(INCLUDE_BOARD_BCM956504R24)
    &bcm956504r24_board,
#endif
#if defined(INCLUDE_BOARD_BCM988230)
    &bcm988230_board,
#endif /* INCLUDE_BOARD_BCM988230 */
#if defined(INCLUDE_BOARD_ROBO)
    &robo_board,
#endif /* INCLUDE_BOARD_ROBO */
    /* keep as last */
    &default_board
};


/*
 * Function:
 *     board_driver_builtins_add
 * Purpose:
 *     Registers all known SDK provided board drivers.
 * Parameters:
 *     none
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX  - failed
 */
int
board_driver_builtins_add(void)
{
    int i, rv = BCM_E_NOT_FOUND;

    for (i=0; i<COUNTOF(_board_builtin_driver); i++) {
        rv = board_driver_add(_board_builtin_driver[i]);
        if (BCM_FAILURE(rv) && rv != BCM_E_EXISTS) {
            break;
        }
    }

    return rv;
}

/*
 * Function:
 *     board_driver_builtins_delete
 * Purpose:
 *     Unregisters all known SDK provided board drivers.
 * Parameters:
 *     none
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX  - failed
 */
int
board_driver_builtins_delete(void)
{
    int i, rv = BCM_E_NOT_FOUND;

    for (i=0; i<COUNTOF(_board_builtin_driver); i++) {
        rv = board_driver_delete(_board_builtin_driver[i]);
        if (BCM_FAILURE(rv) && rv != BCM_E_NOT_FOUND) {
            break;
        }
    }

    return rv;
}

/*
 * Function:
 *     board_driver_builtins_get
 * Purpose:
 *     Returns an array of all built in board drivers.
 * Parameters:
 *     max_num - (IN)  length of board driver array
 *     driver  - (OUT) array of pointers to board_driver_t
 *     actual  - (OUT) actual board driver array length
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX  - failed
 */
int
board_driver_builtins_get(int max_num,
                          board_driver_t **driver,
                          int *actual)
{
    if (!max_num) {
        if (actual) {
            *actual = COUNTOF(_board_builtin_driver);
        }
        return actual ? BCM_E_NONE : BCM_E_PARAM;
    } else {
        if (max_num < COUNTOF(_board_builtin_driver)) {
            max_num = COUNTOF(_board_builtin_driver);
        }
        sal_memcpy(driver, _board_builtin_driver,
                   max_num * sizeof(_board_builtin_driver[0]));
    }

    if (actual) {
        *actual = max_num;
    }

    return BCM_E_NONE;
}
