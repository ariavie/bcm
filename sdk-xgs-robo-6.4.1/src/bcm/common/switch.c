/*
 * $Id: switch.c,v 1.16 Broadcom SDK $
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
 * File:       switch.c
 * Purpose:    Switch common APIs
 */

#ifdef _ERR_MSG_MODULE_NAME
  #error "_ERR_MSG_MODULE_NAME redefined"
#endif

#define _ERR_MSG_MODULE_NAME BCM_DBG_SWITCH

#include <soc/drv.h>
#include <bcm/switch.h>
#include <bcm/init.h>
#include <bcm/error.h>
#include <bcm_int/common/debug.h>
#include <appl/diag/shell.h>
#include <bcm_int/control.h>



/*
 * Function:
 *     bcm_switch_pkt_info_t_init
 * Description:
 *     Initialize an switch packet parameter strucutre
 * Parameters:
 *     pkt_info - pointer to switch packet parameter strucutre
 * Return: none
 */
void
bcm_switch_pkt_info_t_init(bcm_switch_pkt_info_t *pkt_info)
{
    sal_memset(pkt_info, 0, sizeof(bcm_switch_pkt_info_t));
}

/*
 * Function:
 *     bcm_switch_network_group_config_t_init
 * Description:
 *     Initialize a bcm_switch_network_group_config_t structure.
 * Parameters:
 *     config - pointer to switch network group config parameter strucutre
 * Return: none
 */
extern void bcm_switch_network_group_config_t_init(
    bcm_switch_network_group_config_t *config)
{
    sal_memset(config, 0, sizeof(bcm_switch_network_group_config_t));
}

/*
 * Function:
 *     bcm_switch_service_config_t_init
 * Description:
 *     Initialize a bcm_switch_service_config_t structure.
 * Parameters:
 *     config - pointer to service config parameter strucutre
 * Return: none
 */
extern void bcm_switch_service_config_t_init(
    bcm_switch_service_config_t *config)
{
    sal_memset(config, 0, sizeof(bcm_switch_service_config_t));
}

void bcm_switch_profile_mapping_t_init(bcm_switch_profile_mapping_t *profile_mapping)
{
    if (profile_mapping != NULL) {
        sal_memset(profile_mapping, 0, sizeof(bcm_switch_profile_mapping_t));
    }
}

#ifdef BCM_WARM_BOOT_SUPPORT

/* Externs for bcm_<arch>_switch_control_get functions */
#define BCM_DLIST_ENTRY(_dtype) \
extern int bcm_##_dtype##_switch_control_set( \
    int unit, bcm_switch_control_t type, int arg);
#include <bcm_int/bcm_dlist.h>

/* Dispatch table for this function, which uses the externs above */
#define BCM_DLIST_ENTRY(_dtype)\
bcm_##_dtype##_switch_control_set,
static int (*_switch_control_set_disp[])(
    int unit, 
    bcm_switch_control_t type, 
    int arg) =
{
#include <bcm_int/bcm_dlist.h>
0
};

/* Externs for bcm_<arch>_switch_control_get functions */
#define BCM_DLIST_ENTRY(_dtype) \
extern int bcm_##_dtype##_switch_control_get( \
    int unit, bcm_switch_control_t type, int *arg);
#include <bcm_int/bcm_dlist.h>

/* Dispatch table for this function, which uses the externs above */
#define BCM_DLIST_ENTRY(_dtype)\
bcm_##_dtype##_switch_control_get,
static int (*_switch_control_get_disp[])(
    int unit, 
    bcm_switch_control_t type, 
    int *arg) =
{
#include <bcm_int/bcm_dlist.h>
0
};


/**
 * Function:
 *      _bcm_switch_state_sync
 * Purpose:
 *      Sync warm boot cache if necessary
 * Parameters:
 *      unit - (IN) Unit number.
 *      dtype - (IN) Dispatch index
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */

int
_bcm_switch_state_sync(int unit, bcm_dtype_t dtype)
{
    int rv = BCM_E_UNIT;
    int arg;

    if (SOC_UNIT_VALID(unit)) {
        rv = _switch_control_get_disp[dtype](unit,
                                             bcmSwitchControlAutoSync, &arg);
        if (BCM_SUCCESS(rv) && (arg != 0)) {
            if (SOC_CONTROL(unit)->scache_dirty) {
                rv = _switch_control_set_disp[dtype](unit,
                                                     bcmSwitchControlSync, 1);
            }
        }
    }

    return rv;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#undef _ERR_MSG_MODULE_NAME
