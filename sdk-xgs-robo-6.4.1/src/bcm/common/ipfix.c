/*
 * $Id: ipfix.c,v 1.3 Broadcom SDK $
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
 * Purpose:    IPFIX common APIs
 */

#include <soc/drv.h>
#include <bcm/ipfix.h>

/*
 * Function:
 *     bcm_ipfix_config_t_init
 * Description:
 *     Initialize an IPFIX config structure
 * Parameters:
 *     config    pointer to IPFIX config structure
 * Return: none
 */
void
bcm_ipfix_config_t_init(bcm_ipfix_config_t *config)
{
    if (NULL != config) {
        sal_memset(config, 0, sizeof(bcm_ipfix_config_t));
    }
    return;
}

/*
 * Function:
 *     bcm_ipfix_rate_t_init
 * Description:
 *     Initialize an IPFIX rate structure
 * Parameters:
 *     rate      pointer to IPFIX rate structure
 * Return: none
 */
void
bcm_ipfix_rate_t_init(bcm_ipfix_rate_t *rate)
{
    if (NULL != rate) {
        sal_memset(rate, 0, sizeof(bcm_ipfix_rate_t));
    }
    return;
}

/*
 * Function:
 *     bcm_ipfix_mirror_config_t_init
 * Description:
 *     Initialize an IPFIX mirror config structure
 * Parameters:
 *     config    pointer to IPFIX mirror config structure
 * Return: none
 */
void
bcm_ipfix_mirror_config_t_init(bcm_ipfix_mirror_config_t *config)
{
    if (NULL != config) {
        sal_memset(config, 0, sizeof(bcm_ipfix_mirror_config_t));
    }
    return;
}
