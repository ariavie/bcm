/*
 * $Id: l3.c,v 1.2 Broadcom SDK $
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
 * File:        l3.c
 * Purpose:     Hurricane L3 function implementations
 */
#include <soc/defs.h>
#ifdef INCLUDE_L3

#include <assert.h>

#include <sal/core/libc.h>

#include <shared/util.h>
#include <soc/mem.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/l3x.h>
#include <soc/lpm.h>
#include <soc/tnl_term.h>

#include <bcm/l3.h>
#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/hurricane.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/xgs3.h>

#include <bcm_int/esw_dispatch.h>



/*
 * Function:
 *      _bcm_hu_l3_intf_mtu_set
 * Purpose:
 *      Set  L3 interface MTU value. 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      intf_info - (IN)Interface information.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_hu_l3_intf_mtu_set(int unit, _bcm_l3_intf_cfg_t *intf_info)
{
    mtu_values_entry_t mtu_value_buf;  /* Buffer to write mtu. */
    uint32  *mtu_value_buf_p;          /* Write buffer address.*/
    void *null_entry = soc_mem_entry_null(unit, L3_MTU_VALUESm);

    /* Input parameters check */
    if (NULL == intf_info) {
        return (BCM_E_PARAM);
    }

    if(!SOC_MEM_FIELD32_VALUE_FIT(unit, L3_MTU_VALUESm, 
                                  MTU_SIZEf, intf_info->l3i_mtu)) {
        return (BCM_E_PARAM);
    }

    if ((intf_info->l3i_index < soc_mem_index_min(unit, L3_MTU_VALUESm)) || 
        (intf_info->l3i_index > soc_mem_index_max(unit, L3_MTU_VALUESm))) {
        return (BCM_E_PARAM);
    }

    /* Reset the buffer to default value. */
    mtu_value_buf_p = (uint32 *)&mtu_value_buf;
    sal_memcpy(mtu_value_buf_p, null_entry, sizeof(mtu_values_entry_t));

    if (intf_info->l3i_mtu) {
        /* Set mtu. */
        soc_mem_field32_set(unit, L3_MTU_VALUESm, mtu_value_buf_p, 
                            MTU_SIZEf, intf_info->l3i_mtu);
    }

    return BCM_XGS3_MEM_WRITE(unit, L3_MTU_VALUESm, 
                              intf_info->l3i_index, mtu_value_buf_p);
}

/*
 * Function:
 *      _bcm_hu_l3_intf_mtu_get
 * Purpose:
 *      Get  L3 interface MTU value. 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      intf_info - (IN/OUT)Interface information with updated mtu.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_hu_l3_intf_mtu_get(int unit, _bcm_l3_intf_cfg_t *intf_info)
{
    mtu_values_entry_t mtu_value_buf;  /* Buffer to write mtu. */
    uint32  *mtu_value_buf_p;          /* Write buffer address.           */

    /* Input parameters check */
    if (NULL == intf_info) {
        return (BCM_E_PARAM);
    }

    if ((intf_info->l3i_index < soc_mem_index_min(unit, L3_MTU_VALUESm)) || 
        (intf_info->l3i_index > soc_mem_index_max(unit, L3_MTU_VALUESm))) {
        return (BCM_E_PARAM);
    }
    
    /* Zero the buffer. */
    mtu_value_buf_p = (uint32 *)&mtu_value_buf;
    sal_memset(mtu_value_buf_p, 0, sizeof(mtu_values_entry_t));

    /* Read mtu table entry by index. */
    BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_READ(unit, L3_MTU_VALUESm,
                                          intf_info->l3i_index, mtu_value_buf_p));
    intf_info->l3i_mtu = 
        soc_mem_field32_get(unit, L3_MTU_VALUESm, mtu_value_buf_p, MTU_SIZEf);

    return (BCM_E_NONE);
}


#else  /* INCLUDE_L3 */
int bcm_esw_hurricane_l3_not_empty;
#endif /* INCLUDE_L3 */
