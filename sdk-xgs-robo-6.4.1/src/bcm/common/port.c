/*
 * $Id: port.c,v 1.19 Broadcom SDK $
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
 */

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/ll.h>
#include <soc/ptable.h>
#ifdef BCM_ESW_SUPPORT
#include <soc/tucana.h>
#endif
#include <bcm/port.h>
#include <bcm/vlan.h>
#include <bcm/error.h>
#include <bcm/link.h>
#include <bcm/stack.h>
#include <bcm/stg.h>
#include <bcm/rate.h>


/*
* Function:
*      bcm_port_tpid_class_t_init
* Purpose:
*      Initialize the bcm_port_tpid_class_t structure
* Parameters:
*      info - Pointer to the structure which should be initialized
* Returns:
*      NONE
*/
void 
bcm_port_tpid_class_t_init(bcm_port_tpid_class_t *tpid_class)
{
    if (NULL != tpid_class) {
        sal_memset(tpid_class, 0, sizeof (*tpid_class));
    }
    return;
}

/*
* Function:
*      bcm_port_info_t_init
* Purpose:
*      Initialize the bcm_port_info_t structure
* Parameters:
*      info - Pointer to structure which should be initialized
* Returns:
*      NONE
*/
void 
bcm_port_info_t_init(bcm_port_info_t *info)
{
    if (NULL != info) {
        sal_memset(info, 0, sizeof (*info));
    }
    return;
}

/*
 * Function:
 *      bcm_port_config_t_init
 * Purpose:
 *      Initialize a Port config object struct.
 * Parameters:
 *      pconfig - Pointer to the Port Config object struct.
 * Returns:
 *      NONE
 */
void
bcm_port_config_t_init(bcm_port_config_t *pconfig)
{
    if (pconfig != NULL) {
        sal_memset(pconfig, 0, sizeof (*pconfig));
    }
    return;
}


/*
 * Function:
 *      bcm_port_interface_config_t_init
 * Purpose:
 *      Initialize a Port interface config object struct.
 * Parameters:
 *      pconfig - Pointer to the Port Interface Config object struct.
 * Returns:
 *      NONE
 */
void
bcm_port_interface_config_t_init(bcm_port_interface_config_t *pconfig)
{
    if (pconfig != NULL) {
        sal_memset(pconfig, 0, sizeof (*pconfig));
    }
    return;
}


/*
 * Function:
 *      bcm_port_ability_t_init
 * Purpose:
 *      Initialize a Port ability object struct.
 * Parameters:
 *      pconfig - Pointer to the Port ability object struct.
 * Returns:
 *      NONE
 */
void
bcm_port_ability_t_init(bcm_port_ability_t *pability)
{
    if (pability != NULL) {
        sal_memset(pability, 0, sizeof (*pability));
    }
    return;
}

/*
 * Function:
 *      bcm_phy_config_t_init
 * Purpose:
 *      Initialize a PHY config object struct.
 * Parameters:
 *      pconfig - Pointer to the Port ability object struct.
 * Returns:
 *      NONE
 */
void
bcm_phy_config_t_init(bcm_phy_config_t *pconfig)
{
    if (pconfig != NULL) {
        sal_memset(pconfig, 0, sizeof (*pconfig));
    }
    return;
}

/*
 * Function:
 *      bcm_port_encap_config_t_init
 * Purpose:
 *      Initialize a Port encapsulation config object struct.
 * Parameters:
 *      encap_config - Pointer to the Port encapsultation config object struct.
 * Returns:
 *      NONE
 */
void
bcm_port_encap_config_t_init(bcm_port_encap_config_t *encap_config)
{
    if (encap_config != NULL) {
        sal_memset(encap_config, 0, sizeof (*encap_config));
    }
    return;
}

/*
 * Function:
 *      bcm_port_congestion_config_t_init
 * Purpose:
 *      Initialize a Port congestion config object struct.
 * Parameters:
 *      config - Pointer to the Port congestion config object struct.
 * Returns:
 *      NONE
 */
void
bcm_port_congestion_config_t_init(bcm_port_congestion_config_t *config)
{
    if (config != NULL) {
        sal_memset(config, 0, sizeof (*config));
    }
    return;
}

/* 
 * Function:
 *      bcm_port_match_info_t_init
 * Purpose:
 *      Initialize the port_match_info struct
 * Parameters: 
 *      port_match_info - Pointer to the struct to be init'ed
 */
void
bcm_port_match_info_t_init(bcm_port_match_info_t *port_match_info)
{   
    if (port_match_info != NULL) {
        sal_memset(port_match_info, 0, sizeof(*port_match_info));
    }
    return;
}

/*
 * Function:
 *      bcm_port_timesync_config_t_init
 * Purpose:
 *      Initialize a Port config object struct.
 * Parameters:
 *      pconfig - Pointer to the Port Config object struct.
 * Returns:
 *      NONE
 */
void
bcm_port_timesync_config_t_init(bcm_port_timesync_config_t *pconfig)
{
    if (pconfig != NULL) {
        sal_memset(pconfig, 0, sizeof (*pconfig));
    }
    return;
}

/*
 * Function:
 *      bcm_port_phy_timesync_config_t_init
 * Purpose:
 *      Initialize a Port config object struct.
 * Parameters:
 *      pconfig - Pointer to the Port Config object struct.
 * Returns:
 *      NONE
 */
void
bcm_port_phy_timesync_config_t_init(bcm_port_phy_timesync_config_t *pconfig)
{
    if (pconfig != NULL) {
        sal_memset(pconfig, 0, sizeof (*pconfig));
    }
    return;
}

/*
 * Function:
 *      bcm_priority_mapping_t_init
 * Purpose:
 *      Initialize a port vlan priority mapping struct.
 * Parameters:
 *      pri_map - Pointer to the pri_map struct.
 * Returns:
 *      NONE
 */
void
bcm_priority_mapping_t_init(bcm_priority_mapping_t *pri_map)
{
    if (pri_map != NULL) {
        sal_memset(pri_map, 0, sizeof (*pri_map));
    }
    return;
}
