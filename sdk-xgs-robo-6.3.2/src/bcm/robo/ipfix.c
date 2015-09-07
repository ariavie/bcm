/*
 * $Id: ipfix.c 1.10 Broadcom SDK $
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
 * File:       ipfix.c
 * Purpose:    IPFIX API
 */

#include <soc/drv.h>
#include <bcm/ipfix.h>
#include <bcm/error.h>

/*
 * Function:
 *     bcm_robo_ipfix_register
 * Description:
 *     To register the callback function for flow info export
 * Parameters:
 *     unit          device number
 *     callback      user callback function
 *     userdata      user data used as argument during callcack
 * Return:
 *     BCM_E_XXX
 */
int
bcm_robo_ipfix_register(int unit,
                       bcm_ipfix_callback_t callback,
                       void *userdata)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *     bcm_robo_ipfix_unregister
 * Description:
 *     To unregister the callback function for flow info export
 * Parameters:
 *     unit          device number
 *     callback      user callback function
 *     userdata      user data used as argument during callcack
 * Return:
 *     BCM_E_XXX
 */
int
bcm_robo_ipfix_unregister(int unit,
                         bcm_ipfix_callback_t callback,
                         void *userdata)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *     bcm_robo_ipfix_config_set function
 * Description:
 *     To set per port IPFIX configuration
 * Parameters:
 *     unit            device number
 *     port            port number
 *     stage           BCM_IPFIX_STAGE_INGRESS or BCM_IPFIX_STAGE_EGRESS
 *     config          pointer to ipfix configuration
 * Return:
 *     BCM_E_XXX
 */
int
bcm_robo_ipfix_config_set(int unit,
                          bcm_port_t port,
                          bcm_ipfix_stage_t stage,
                          bcm_ipfix_config_t *config)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *     bcm_robo_ipfix_config_get function
 * Description:
 *     To get per port IPFIX configuration
 * Parameters:
 *     unit            device number
 *     port            port number
 *     stage           BCM_IPFIX_STAGE_INGRESS or BCM_IPFIX_STAGE_EGRESS
 *     config          pointer to ipfix configuration
 * Return:
 *     BCM_E_XXX
 */
int
bcm_robo_ipfix_config_get(int unit,
                          bcm_port_t port,
                          bcm_ipfix_stage_t stage,
                          bcm_ipfix_config_t *config)
{
    return BCM_E_UNAVAIL;
}

/* Add an IPFIX flow rate meter entry */
int bcm_robo_ipfix_rate_create(int unit, 
                        bcm_ipfix_rate_t *rate_info)
{
    return BCM_E_UNAVAIL;
}

/* Delete an IPFIX flow rate meter entry */
int bcm_robo_ipfix_rate_destroy(int unit, 
                        bcm_ipfix_rate_id_t rate_id)
{
    return BCM_E_UNAVAIL;
}

/* Get IPFIX flow rate meter entry for the specified id */
int bcm_robo_ipfix_rate_get(int unit, 
                        bcm_ipfix_rate_t *rate_info)
{
    return BCM_E_UNAVAIL;
}


/* Traverse through IPFIX flow rate meter entries */
int bcm_robo_ipfix_rate_traverse(int unit, 
                        bcm_ipfix_rate_traverse_cb cb, 
                        void *userdata)
{
    return BCM_E_UNAVAIL;
}

/* Delete all IPFIX flow rate meter entries */
int bcm_robo_ipfix_rate_destroy_all(int unit)
{
    return BCM_E_UNAVAIL;
}

/* Add a mirror destination to the IPFIX flow rate meter entry */
int bcm_robo_ipfix_rate_mirror_add(int unit, 
                        bcm_ipfix_rate_id_t rate_id, 
                        bcm_gport_t mirror_dest_id)
{
    return BCM_E_UNAVAIL;
}

/* Delete a mirror destination from the IPFIX flow rate meter entry */
int bcm_robo_ipfix_rate_mirror_delete(int unit, 
                        bcm_ipfix_rate_id_t rate_id, 
                        bcm_gport_t mirror_dest_id)
{
    return BCM_E_UNAVAIL;
}

/* 
 * Delete all mirror destination associated to the IPFIX flow rate meter
 * entry
 */
int bcm_robo_ipfix_rate_mirror_delete_all(int unit, 
                        bcm_ipfix_rate_id_t rate_id)
{
    return BCM_E_UNAVAIL;
}

/* Get all mirror destination from the IPFIX flow rate meter entry */
int bcm_robo_ipfix_rate_mirror_get(int unit, 
                        bcm_ipfix_rate_id_t rate_id, 
                        int mirror_dest_size, 
                        bcm_gport_t *mirror_dest_id, 
                        int *mirror_dest_count)
{
    return BCM_E_UNAVAIL;
}

/* Set IPFIX mirror control configuration of the specified port */
int bcm_robo_ipfix_mirror_config_set(int unit,
                                     bcm_ipfix_stage_t stage,
                                     bcm_gport_t port,
                                     bcm_ipfix_mirror_config_t *config)
{
    return BCM_E_UNAVAIL;
}

/* Get IPFIX mirror control configuration of the specified port */
int bcm_robo_ipfix_mirror_config_get(int unit,
                                     bcm_ipfix_stage_t stage,
                                     bcm_gport_t port,
                                     bcm_ipfix_mirror_config_t *config)
{
    return BCM_E_UNAVAIL;
}

/* Add an IPFIX mirror destination to the specified port */
int bcm_robo_ipfix_mirror_port_dest_add(int unit,
                                        bcm_ipfix_stage_t stage,
                                        bcm_gport_t port,
                                        bcm_gport_t mirror_dest_id)
{
    return BCM_E_UNAVAIL;
}

/* Delete an IPFIX mirror destination from the specified port */
int bcm_robo_ipfix_mirror_port_dest_delete(int unit,
                                           bcm_ipfix_stage_t stage,
                                           bcm_gport_t port,
                                           bcm_gport_t mirror_dest_id)
{
    return BCM_E_UNAVAIL;
}

/* Delete all IPFIX mirror destination from the specified port */
int bcm_robo_ipfix_mirror_port_dest_delete_all(int unit,
                                               bcm_ipfix_stage_t stage,
                                               bcm_gport_t port)
{
    return BCM_E_UNAVAIL;
}

/* Get all IPFIX mirror destination of the specified port */
int bcm_robo_ipfix_mirror_port_dest_get(int unit,
                                        bcm_ipfix_stage_t stage,
                                        bcm_gport_t port,
                                        int mirror_dest_size,
                                        bcm_gport_t *mirror_dest_id,
                                        int *mirror_dest_count)
{
    return BCM_E_UNAVAIL;
}
