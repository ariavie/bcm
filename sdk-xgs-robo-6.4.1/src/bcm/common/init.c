/*
 * $Id: init.c,v 1.13 Broadcom SDK $
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
#include <sal/core/time.h>
#include <sal/core/boot.h>
#include <shared/bsl.h>
#include <bcm/module.h>
#include <bcm/init.h>

#include <soc/mem.h>
#include <soc/drv.h>

/* Protection mutexes for each unit. */
sal_mutex_t _bcm_lock[BCM_MAX_NUM_UNITS];

/*
 * Function:
 *    bcm_module_name
 * Purpose:
 *    Return the name of a module given its number
 * Parameters:
 *    unit - placeholder
 *    module_num - One of BCM_MODULE_xxx
 * Returns:
 *    Pointer to static char string
 * Notes:
 *    This function to be moved to its own module when it makes sense.
 */

STATIC char *_bcm_module_names[] = BCM_MODULE_NAMES_INITIALIZER;

char *
bcm_module_name(int unit, int module_num)
{
    if (sizeof(_bcm_module_names) / sizeof(_bcm_module_names[0])
                                                    != BCM_MODULE__COUNT) {
        int i;

        i = sizeof(_bcm_module_names) / sizeof(_bcm_module_names[0]) - 1;

        LOG_ERROR(BSL_LS_BCM_INIT,
                  (BSL_META_U(unit,
                              "bcm_module_name: BCM_MODULE_NAMES_INITIALIZER(%d) and BCM_MODULE__COUNT(%d) mis-match\n"), i, BCM_MODULE__COUNT));
        for(;i >= 0;i--) {
            LOG_ERROR(BSL_LS_BCM_INIT,
                      (BSL_META_U(unit,
                                  "%2d. %s\n"), i, _bcm_module_names[i]));
        }
    }
    if (module_num < 0 || module_num >= BCM_MODULE__COUNT) {
        return "unknown";
    }

    return _bcm_module_names[module_num];
}

/*
 * Function:
 *      bcm_info_t_init
 * Purpose:
 *      Initialize the bcm_info_t structure.
 * Parameters:
 *      info - Pointer to bcm_info_t structure.
 * Returns:
 *      NONE
 */
void
bcm_info_t_init(bcm_info_t *info)
{
    if (info != NULL) {
        sal_memset(info, 0, sizeof (*info));
    }
    return;
}

/*
 * Function:
 *      bcm_attach_info_t_init
 * Purpose:
 *      Initialize the bcm_attach_info_t structure.
 * Parameters:
 *      info - (IN/OUT) Pointer to structure to initialize.
 * Returns:
 *      NONE
 */
void
bcm_attach_info_t_init(bcm_attach_info_t *info)
{
    if (info != NULL) {
        sal_memset(info, 0, sizeof (*info));
    }
    return;
}

/*
 * Function:
 *      bcm_detach_retry_t_init
 * Purpose:
 *      Initialize the bcm_detach_retry_t structure.
 * Parameters:
 *      retry - Pointer to bcm_detach_retry_t structure.
 * Returns:
 *      NONE
 */
void
bcm_detach_retry_t_init(bcm_detach_retry_t *retry)
{
    if (retry != NULL) {
        sal_memset(retry, 0, sizeof (*retry));
    }
    return;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_shutdown
 * Purpose:
 *      Free up resources without touching hardware
 * Parameters:
 *      unit    - switch device
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_shutdown(int unit)
{
    int warm_boot;
    int rv;

    /* Since this API is used for warm-boot, we need to enable
       warm boot here even if it hasn't been already, and restore
       it afterward if necessary. */

    warm_boot = SOC_WARM_BOOT(unit);

    if (!warm_boot)
    {
        SOC_WARM_BOOT_START(unit);
    }

    rv = bcm_detach(unit);

    if (!warm_boot)
    {
        SOC_WARM_BOOT_DONE(unit);
    }

    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

