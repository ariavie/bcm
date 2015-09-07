/*
 * $Id: attrib.c 1.3 Broadcom SDK $
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
 * File:        attrib.c
 * Purpose:     board attribute management
 *
 */

#include <sal/core/alloc.h>
#include <sal/core/libc.h>
#include <bcm/error.h>
#include <board/board.h>
#include <board/manager.h>
#include "mgr.h"


/*
 * Function:
 *     _attribute_match
 * Purpose:
 *     Match a requested attribute name with the registered one.
 *     Right now, just do a simple string match. It's possible
 *     to do something more complicated along the lines of the
 *     soc property suffixes, to pass additional data to the
 *     attribute function via the name field.
 *
 * Parameters:
 *      reqname - (IN) requested name
 *      regname - (IN) registered name
 * Returns:
 *      0 - match
 *     !0 - no match
 */
STATIC int
_attribute_match(char *reqname, char *regname)
{
    return !sal_strcmp(reqname, regname);
}

/*
 * Function:
 *     board_attribute_get
 * Purpose:
 *     Return a value for an attribute of the current board.
 * Parameters:
 *     attrib - (IN) board attribute id string
 *     value -  (OUT) pointer to attribute value
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_UNAVAIL - attribute not supported by board driver
 *     BCM_E_NOT_FOUND - attribute unknown to board manager
 *     BCM_E_XXX - failed
 */
int
board_attribute_get(char *name, int *value)
{
    int rv = BCM_E_FAIL;
    board_driver_t *driver;
    int i;

    BOARD_INIT;
    BOARD_LOCK;
    if ((driver = board_driver())) {
        for (i=0; i<driver->num_attribute; i++) {
            if (_attribute_match(name, driver->attribute[i].name)) {
                rv = BCM_E_UNAVAIL;
                if (driver->attribute[i].get) {
                    rv = driver->attribute[i].get(driver, name, value);
                }
                break;
            }
        }
    }
    BOARD_UNLOCK;

    return rv;
}

/*
 * Function:
 *     board_attribute_set
 * Purpose:
 *     Set attribute of the current board to the given value.
 * Parameters:
 *     attrib - (IN) board attribute id string
 *     value  - (IN) attribute value
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_UNAVAIL - attribute not supported by board driver
 *     BCM_E_NOT_FOUND - attribute unknown to board driver
 *     BCM_E_XXX - failed
 */
int
board_attribute_set(char *name, int value)
{
    int rv = BCM_E_FAIL;
    board_driver_t *driver;
    int i;

    BOARD_INIT;
    BOARD_LOCK;
    if ((driver = board_driver()) != NULL) {
        for (i=0; i<driver->num_attribute; i++) {
            if (_attribute_match(name, driver->attribute[i].name)) {
                rv = BCM_E_UNAVAIL;
                if (driver->attribute[i].set) {
                    rv = driver->attribute[i].set(driver, name, value);
                }
                break;
            }
        }
    }
    BOARD_UNLOCK;

    return rv;
}

/*
 * Function:
 *     board_attributes_get
 * Purpose:
 *     Return an array of all attributes known by the board driver up
 *     to 'max_num' number of attributes. The actual length is returned
 *     in 'actual'. If 'max_num' is zero, then 'actual' will return the
 *     length of the attribute array, and not write to 'attrib'.
 * Parameters:
 *     max_num - (IN)  length of attribute array
 *     attrib  - (OUT) array of board_attribute_t
 *     actual  - (OUT) actual attribute array length
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX  - failed
 */
int
board_attributes_get(int max_num, board_attribute_t *attribute, int *actual)
{
    int rv = BCM_E_UNAVAIL;
    board_driver_t *driver;

    BOARD_INIT;
    BOARD_LOCK;
    if ((driver = board_driver()) != NULL) {
        if (!max_num) {
            if (actual) {
                *actual = driver->num_attribute;
                rv = BCM_E_NONE;
            } else {
                rv = BCM_E_PARAM;
            }
        } else {
            if (attribute) {
                if (max_num < driver->num_attribute) {
                    max_num = driver->num_attribute;
                }
                sal_memcpy(attribute,
                           driver->attribute,
                           max_num*sizeof(board_attribute_t));
                if (actual) {
                    *actual = max_num;
                }
                rv = BCM_E_NONE;
            } else {
                rv = BCM_E_PARAM;
            }
        }
    }
    BOARD_UNLOCK;

    return rv;
}

