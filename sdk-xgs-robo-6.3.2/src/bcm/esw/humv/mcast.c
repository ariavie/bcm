/*
 * $Id: mcast.c 1.9 Broadcom SDK $
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
 * File:        mcast.c
 * Purpose:     Tracks and manages L2 Multicast tables.
 */

#include <soc/defs.h>
#if defined(BCM_BRADLEY_SUPPORT)
#include <soc/drv.h>
#include <soc/mem.h>

#include <bcm/error.h>
#include <bcm/l2.h>

/*
 * Function:
 *      bcm_humv_mcast_init
 * Purpose:
 *      Initialize multicast api components
 * Returns:
 *      BCM_E_XXX on error
 *      number of mcast entries supported on success
 */

int
bcm_humv_mcast_init(int unit)
{
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        uint32 mc_ctrl;
        soc_control_t       *soc = SOC_CONTROL(unit);

        /* Recover the mcast size settings */
        SOC_IF_ERROR_RETURN
            (READ_MC_CONTROL_1r(unit, &mc_ctrl));
        soc->higig2_bcast_size = 
            soc_reg_field_get(unit, MC_CONTROL_1r, mc_ctrl,
                              HIGIG2_BC_SIZEf);

        SOC_IF_ERROR_RETURN
            (READ_MC_CONTROL_2r(unit, &mc_ctrl));
        soc->higig2_mcast_size = 
            soc_reg_field_get(unit, MC_CONTROL_2r, mc_ctrl,
                              HIGIG2_L2MC_SIZEf);

        SOC_IF_ERROR_RETURN
            (READ_MC_CONTROL_3r(unit, &mc_ctrl));
        soc->higig2_ipmc_size = 
            soc_reg_field_get(unit, MC_CONTROL_3r, mc_ctrl,
                              HIGIG2_IPMC_SIZEf);

        SOC_IF_ERROR_RETURN
            (READ_MC_CONTROL_5r(unit, &mc_ctrl));
        soc->mcast_size = 
            soc_reg_field_get(unit, MC_CONTROL_5r, mc_ctrl,
                              SHARED_TABLE_L2MC_SIZEf);
        soc->ipmc_size = 
            soc_reg_field_get(unit, MC_CONTROL_5r, mc_ctrl,
                              SHARED_TABLE_IPMC_SIZEf);


    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    /* Other initialization relocated to soc_misc_init */
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_humv_mcast_detach
 * Purpose:
 *      De-initialize multicast api components
 * Returns:
 *      BCM_E_XXX on error
 *      number of mcast entries supported on success
 */

int
_bcm_humv_mcast_detach(int unit)
{
    return BCM_E_NONE;
}
#else /* BCM_BRADLEY_SUPPORT */
int _humv_mcast_not_empty;
#endif  /* BCM_BRADLEY_SUPPORT */
