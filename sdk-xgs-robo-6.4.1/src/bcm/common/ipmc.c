/*
 * $Id: ipmc.c,v 1.8 Broadcom SDK $
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

#ifdef INCLUDE_L3

#include <soc/drv.h>
#include <soc/mem.h>

#include <bcm/error.h>
#include <bcm/ipmc.h>
#include <bcm/stat.h>
#include <bcm/vlan.h>
#include <bcm/debug.h>
/*
 * Function:
 *	bcm_ipmc_addr_t_init
 * Description:
 *	Initialize a bcm_ipmc_addr_t to a specified source IP address,
 *	destination IP address and VLAN, while zeroing all other fields.
 * Parameters:
 *	data - Pointer to bcm_ipmc_addr_t.
 * Returns:
 *	Nothing.
 */

void
bcm_ipmc_addr_t_init(bcm_ipmc_addr_t *data)
{
    if (NULL != data) {
        sal_memset(data, 0, sizeof(bcm_ipmc_addr_t));
        data->rp_id = BCM_IPMC_RP_ID_INVALID;
    }
}

/*
 * Function:
 *      bcm_ipmc_counters_t_init
 * Description:
 *      Initialize a ipmc counters object struct.
 * Parameters:
 *      ipmc_counter - Pointer to IPMC counters object struct.
 * Returns:
 *      Nothing.
 */
void
bcm_ipmc_counters_t_init(bcm_ipmc_counters_t *ipmc_counter)
{
    if (ipmc_counter != NULL) {
        sal_memset(ipmc_counter, 0, sizeof(*ipmc_counter));
    }
    return;
}

/*
 * Function:
 *	bcm_ipmc_range_t_init
 * Description:
 *	Initialize a bcm_ipmc_range_t to all zeroes.
 * Parameters:
 *	data - Pointer to bcm_ipmc_range_t.
 * Returns:
 *	Nothing.
 */
void
bcm_ipmc_range_t_init(bcm_ipmc_range_t *range)
{
    if (NULL != range) {
        sal_memset(range, 0, sizeof(bcm_ipmc_range_t));
    }
}

#else 
int _bcm_ipmc_not_empty;
#endif  /* INCLUDE_L3 */

