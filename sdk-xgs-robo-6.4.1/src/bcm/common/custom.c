/*
 * $Id: custom.c,v 1.4 Broadcom SDK $
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
 * Custom callouts
 * Mostly useful for running customized composite apis over remote units.
 */

#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/custom.h>

#include <bcm_int/control.h>

static bcm_custom_cb_t	_custom_unit_func[BCM_UNITS_MAX];
static void 	       *_custom_unit_user_data[BCM_UNITS_MAX];

int
bcm_common_custom_register(int unit,
                           bcm_custom_cb_t func,
                           void *user_data)
{
    if (unit >= BCM_CONTROL_MAX) {
        return BCM_E_UNIT;
    }
    _custom_unit_func[unit] = func;
    _custom_unit_user_data[unit] = user_data;
    return BCM_E_NONE;
}

int
bcm_common_custom_unregister(int unit)
{
    return bcm_custom_register(unit, NULL, NULL);
}

int
bcm_common_custom_port_set(int unit,
                           bcm_port_t port,
                           int type,
                           int len,
                           uint32 *args)
{
    bcm_custom_cb_t     func;
    void *user_data;

    func = NULL;
    user_data = NULL;
    if (unit >= BCM_CONTROL_MAX) {
        return BCM_E_UNIT;
    } else {
        func = _custom_unit_func[unit];
        user_data = _custom_unit_user_data[unit];
   }
    if (func == NULL) {
        return BCM_E_UNAVAIL;
    }
    return (*func)(unit, port, BCM_CUSTOM_SET, type, len, args, NULL, user_data);
}

int
bcm_common_custom_port_get(int unit,
                           bcm_port_t port,
                           int type,
                           int max_len,
                           uint32 *args,
                           int *actual_len)
{
    bcm_custom_cb_t     func;
    void *user_data;

    func = NULL;
    user_data = NULL;
    if (unit >= BCM_CONTROL_MAX) {
        return BCM_E_UNIT;
    } else {
        func = _custom_unit_func[unit];
        user_data = _custom_unit_user_data[unit];
    }
    if (func == NULL) {
        return BCM_E_UNAVAIL;
    }
    
    if (actual_len) {
        /* default actual length is max_len passed in; application
           callback can modify to the appropriate value */
        *actual_len = max_len;
    }
    return (*func)(unit, port, BCM_CUSTOM_GET, type,
                   max_len, args, actual_len, user_data);
}
