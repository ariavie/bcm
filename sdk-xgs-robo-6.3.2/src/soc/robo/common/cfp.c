/*
 * $Id: cfp.c 1.3 Broadcom SDK $
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
 * CFP driver service
 */
#include <assert.h>
#include <soc/types.h>
#include <soc/error.h>
#include <soc/cfp.h>


_fp_id_info_t _robo_fp_id_info[SOC_MAX_NUM_DEVICES];

/*
 * Function: drv_cfp_qset_get
 *
 * Purpose:
 *     Get the qualify bit value from CFP entry. 
 *
 * Parameters:
 *     unit - BCM device number
 *     qual - qualify ID
 *     entry -CFP entry
 *     val(OUT) -TRUE/FALSE
 *
 * Returns:
 *     SOC_E_NONE
 *     SOC_E_XXX
 *
 * Note:
 *     
 */
int
drv_cfp_qset_get(int unit, uint32 qual, drv_cfp_entry_t *entry, uint32 *val)
{
    int rv = SOC_E_NONE;
    uint32  wp, bp;

    assert(entry);
    
    wp = qual / 32;
    bp = qual & (32 - 1);
    if (entry->w[wp] & (1 << bp)) {
        *val = TRUE;
    } else {
        *val = FALSE;
    }

    return rv;
}

/*
 * Function: drv_cfp_qset_set
 *
 * Purpose:
 *     Set/Reset the qualify bit value to CFP entry. 
 *
 * Parameters:
 *     unit - BCM device number
 *     qual - qualify ID
 *     entry -CFP entry
 *     val -TRUE/FALSE
 *
 * Returns:
 *     SOC_E_NONE
 *     SOC_E_XXX
 *
 * Note:
 *     
 */
int
drv_cfp_qset_set(int unit, uint32 qual, drv_cfp_entry_t *entry, uint32 val)
{
    int rv = SOC_E_NONE;
    uint32  wp, bp, temp = 0;

    assert(entry);
    
    wp = qual / 32;
    bp = qual & (32 - 1);
    if (val) {
        temp = 1;
    }

    if (temp) {
        entry->w[wp] |= (1 << bp);
    } else {
        entry->w[wp] &= ~(1 << bp);
    }
    
    return rv;
}


