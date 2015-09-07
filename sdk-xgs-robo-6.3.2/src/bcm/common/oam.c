/*
 * $Id: oam.c 1.14 Broadcom SDK $
 *
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
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software and
 * all intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED
 * LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY,
 * AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE
 * OF THE SOFTWARE. 
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and
 * to use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 * OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 * INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 * RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF
 * BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE
 * ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL
 * APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED
 * REMEDY.
 */

#include <sal/core/libc.h>
#include <bcm/oam.h>

/*
 * Function:
 *      bcm_oam_group_info_t_init
 * Purpose:
 *      Initialize an OAM group info structure
 * Parameters:
 *      group_info - (OUT) Pointer to OAM group info structure to be initialized
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
void 
bcm_oam_group_info_t_init(
    bcm_oam_group_info_t *group_info)
{
    sal_memset(group_info, 0, sizeof(bcm_oam_group_info_t));
}

/*
 * Function:
 *      bcm_oam_endpoint_info_t_init
 * Purpose:
 *      Initialize an OAM endpoint info structure
 * Parameters:
 *      endpoint_info - (OUT) Pointer to OAM endpoint info structure to be initialized
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
void 
bcm_oam_endpoint_info_t_init(
    bcm_oam_endpoint_info_t *endpoint_info)
{
    sal_memset(endpoint_info, 0, sizeof(bcm_oam_endpoint_info_t));
    endpoint_info->gport = BCM_GPORT_INVALID;
    endpoint_info->tx_gport = BCM_GPORT_INVALID;
}

/*
 * Function:
 *      bcm_oam_loss_t_init
 * Purpose:
 *      Initialize an OAM loss structure
 * Parameters:
 *      loss_ptr - (OUT) Pointer to OAM loss structure to be initialized
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
void
bcm_oam_loss_t_init(
    bcm_oam_loss_t *loss_ptr)
{
    sal_memset(loss_ptr, 0, sizeof(bcm_oam_loss_t));
    loss_ptr->pkt_pri_bitmap = ~0;
}

/*
 * Function:
 *      bcm_oam_delay_t_init
 * Purpose:
 *      Initialize an OAM delay structure
 * Parameters:
 *      delay_ptr - (OUT) Pointer to OAM delay structure to be initialized
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
void
bcm_oam_delay_t_init(
    bcm_oam_delay_t *delay_ptr)
{
    sal_memset(delay_ptr, 0, sizeof(bcm_oam_delay_t));
}

/*
 * Function:
 *      bcm_oam_psc_t_init
 * Purpose:
 *      Initialize an OAM PSC structure
 * Parameters:
 *      psc_ptr - (OUT) Pointer to OAM PSC structure to be initialized
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
void
bcm_oam_psc_t_init(
    bcm_oam_psc_t *psc_ptr)
{
    sal_memset(psc_ptr, 0, sizeof(bcm_oam_psc_t));
}

/*
 * Function:
 *      bcm_oam_loopback_t_init
 * Purpose:
 *      Initialize an OAM loopback structure
 * Parameters:
 *      loopback_ptr - (OUT) Pointer to OAM loopback structure to be initialized
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
void
bcm_oam_loopback_t_init(
    bcm_oam_loopback_t *loopback_ptr)
{
    sal_memset(loopback_ptr, 0, sizeof(bcm_oam_loopback_t));
}

/*
 * Function:
 *      bcm_oam_pw_status_t_init
 * Purpose:
 *      Initialize an OAM PW Status structure
 * Parameters:
 *      pw_status_ptr - (OUT) Pointer to OAM PW Status structure to be initialized
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
void
bcm_oam_pw_status_t_init(
    bcm_oam_pw_status_t *pw_status_ptr)
{
    sal_memset(pw_status_ptr, 0, sizeof(bcm_oam_pw_status_t));
}

