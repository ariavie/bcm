/*
 * $Id: fcoe.c,v 1.2 Broadcom SDK $
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
 * All Rights Reserved.$
 *
 * extender structure initializers
 */
#include <sal/core/libc.h>
 
#include <bcm/stat.h>
#include <bcm/fcoe.h>

/* 
 * Function:
 *      bcm_fcoe_route_t_init
 * Purpose:
 *      Initialize the fcoe route struct
 * Parameters: 
 *      route - Pointer to the struct to be init'ed
 */
void
bcm_fcoe_route_t_init(bcm_fcoe_route_t *route)
{   
    if (route != NULL) {
        sal_memset(route, 0, sizeof(*route));
    }
    return;
}

/* 
 * Function:
 *      bcm_fcoe_vsan_t_init
 * Purpose:
 *      Initialize the fcoe vsan struct
 * Parameters: 
 *      vsan - Pointer to the struct to be init'ed
 */
void
bcm_fcoe_vsan_t_init(bcm_fcoe_vsan_t *vsan)
{   
    if (vsan != NULL) {
        sal_memset(vsan, 0, sizeof(*vsan));
    }
    return;
}

/* 
 * Function:
 *      bcm_fcoe_zone_entry_t_init
 * Purpose:
 *      Initialize the fcoe zone struct
 * Parameters: 
 *      zone - Pointer to the struct to be init'ed
 */
void
bcm_fcoe_zone_entry_t_init(bcm_fcoe_zone_entry_t *zone)
{   
    if (zone != NULL) {
        sal_memset(zone, 0, sizeof(*zone));
    }
    return;
}

/* 
 * Function:
 *      bcm_fcoe_intf_config_t_init
 * Purpose:
 *      Initialize a bcm_fcoe_intf_config_t_init struct
 * Parameters: 
 *      intf - Pointer to the struct to be init'ed
 */
void
bcm_fcoe_intf_config_t_init(bcm_fcoe_intf_config_t *intf)
{   
    if (intf != NULL) {
        sal_memset(intf, 0, sizeof *intf);
    }
    return;
}

/* 
 * Function:
 *      bcm_fcoe_vsan_vft_t_init
 * Purpose:
 *      Initialize a bcm_fcoe_vsan_vft_t struct
 * Parameters: 
 *      vft - Pointer to the struct to be init'ed
 */
void
bcm_fcoe_vsan_vft_t_init(bcm_fcoe_vsan_vft_t *vft)
{   
    if (vft != NULL) {
        sal_memset(vft, 0, sizeof *vft);
    }
    return;
}

/* 
 * Function:
 *      bcm_fcoe_vsan_translate_key_config_t_init
 * Purpose:
 *      Initialize a bcm_fcoe_vsan_translate_key_config_t struct
 * Parameters: 
 *      key_config - Pointer to the struct to be init'ed
 */
void
bcm_fcoe_vsan_translate_key_config_t_init(bcm_fcoe_vsan_translate_key_config_t 
                                          *key_config)
{   
    if (key_config != NULL) {
        sal_memset(key_config, 0, sizeof *key_config);
    }
    return;
}

/* 
 * Function:
 *      bcm_fcoe_vsan_action_set_t_init
 * Purpose:
 *      Initialize a fcoe_vsan_action_set_t struct
 * Parameters: 
 *      action_set - Pointer to the struct to be init'ed
 */
void
bcm_fcoe_vsan_action_set_t_init(bcm_fcoe_vsan_action_set_t *action_set)
{   
    if (action_set != NULL) {
        sal_memset(action_set, 0, sizeof *action_set);
    }
    return;
}
