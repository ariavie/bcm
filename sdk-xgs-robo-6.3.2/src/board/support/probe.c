/*
 * $Id: probe.c 1.3 Broadcom SDK $
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
 * File:        probe.c
 * Purpose:     Board Probe functions
 *
 */

#include <sal/core/alloc.h>
#include <sal/core/libc.h>
#include <bcm/init.h>
#include <bcm/error.h>
#include <board/board.h>
#include <board_int/support.h>

/*
 * Function:
 *     board_probe_get
 * Purpose:
 *     Returns an array of bcm_info_t corresponding to the devices on
 *     the board. If max_num is zero, then actual will contain the
 *     number of element required to be allocated for info. If max_num
 *     is greater than zero, then populate info up to max_num elements,
 *     returning the number populated in actual.
 * Parameters:
 *     max_num  - (IN)  maximum length of info, or 0 to request length
 *     info     - (OUT) array of bcm_info_t
 *     actual   - (OUT) length of array
 * Returns:
 *     BCM_E_NONE - success
 *     BCM_E_XXX - failed
 */
int
board_probe_get(int max_num, bcm_info_t *info, int *actual)
{
    int unit, limit, rv, probed;

    limit = board_num_devices();

    /* handle query */
    if (max_num == 0) {
        if (actual) {
            *actual = limit;
        }
        return actual ? BCM_E_NONE : BCM_E_PARAM;
    }

    probed = 0;
    limit = limit < BCM_LOCAL_UNITS_MAX ? limit : BCM_LOCAL_UNITS_MAX;
    limit = max_num < limit ? max_num : limit;
    for (unit=0;  unit < limit; unit++ ) {
        rv = board_device_info(unit, &info[unit]);
        if (rv == BCM_E_UNAVAIL) {
            continue;
        } else if (BCM_FAILURE(rv)) {
            return rv;
        } else {
            probed++;
        }
    }

    if (actual) {
        *actual = unit;
    }

    return probed ? BCM_E_NONE : BCM_E_NOT_FOUND;
}
