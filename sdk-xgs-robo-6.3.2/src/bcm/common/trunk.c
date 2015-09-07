/*
 * $Id: trunk.c 1.10 Broadcom SDK $
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

#include <sal/types.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>

#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/mcast.h>
#include <bcm/l2.h>
#include <bcm/stack.h>
#include <bcm/port.h>
#include <bcm/trunk.h>
#include <bcm/switch.h>

#include <bcm_int/common/trunk.h>

/*
 * Function:
 *      bcm_trunk_info_t_init
 * Purpose:
 *      Initialize the bcm_trunk_info_t structure
 * Parameters:
 *      trunk_info - Pointer to structure which should be initialized
 * Returns:
 *      NONE
 */
void 
bcm_trunk_info_t_init(bcm_trunk_info_t *trunk_info)
{
    if (NULL != trunk_info) {
        sal_memset(trunk_info, 0, sizeof (*trunk_info));
    } 
    return;
}

/*
 * Function:
 *      bcm_trunk_add_info_t_init
 * Purpose:
 *      Initialize the bcm_trunk_add_info_t structure
 * Parameters:
 *      add_info - Pointer to structure which should be initialized
 * Returns:
 *      NONE
 */
void 
bcm_trunk_add_info_t_init(bcm_trunk_add_info_t *add_info)
{
    if (NULL != add_info) {
        sal_memset(add_info, 0, sizeof (*add_info));
    } 
    return;
}

/*
 * Function:
 *      bcm_trunk_chip_info_t_init
 * Purpose:
 *      Initialize the bcm_trunk_chip_info_t structure
 * Parameters:
 *      trunk_chip_info - Pointer to trunk chip info structure
 * Returns:
 *      NONE
 */
void
bcm_trunk_chip_info_t_init(bcm_trunk_chip_info_t *trunk_chip_info)
{
    if (trunk_chip_info != NULL) {
        sal_memset(trunk_chip_info, 0, sizeof (*trunk_chip_info));
    }
    return;
}

/*
 * Function:
 *      bcm_trunk_member_t_init
 * Purpose:
 *      Initialize the bcm_trunk_member_t structure
 * Parameters:
 *      trunk_member - Pointer to structure which should be initialized
 * Returns:
 *      NONE
 */
void 
bcm_trunk_member_t_init(bcm_trunk_member_t *trunk_member)
{
    if (NULL != trunk_member) {
        sal_memset(trunk_member, 0, sizeof (*trunk_member));
    } 
    return;
}

