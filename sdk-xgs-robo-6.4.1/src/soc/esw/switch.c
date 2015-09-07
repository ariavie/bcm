/*
 * $Id: a04dd98ad0459fba54e03596a681b8a658f98617 $
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
 * SOC switch Utilities
 */

#include <shared/bsl.h>

#include <soc/drv.h>
#include <soc/error.h>
#include <soc/scache.h>
#include <shared/alloc.h>


#ifdef BCM_WARM_BOOT_SUPPORT

#define SOC_CONTROL_WB_PART_0_VER_1_0       SOC_SCACHE_VERSION(1,0)
#define SOC_CONTROL_WB_PART_0_DEFAULT_VER   SOC_CONTROL_WB_PART_0_VER_1_0

/*
 * List of software-only switch controls that need to be recovered in Level 2.
 * There is no way to recover these controls in level 1 warmboot.
 *
 * The following enum is used to identify the boolean controls.
 */
enum soc_wb_boolean_controls_0_e {
    socControlUseGport = 0,
    socControlL2PortBlocking = 1,
    socControlAbortOnError = 2,
    socControlCount = 3
};

#define SOC_CONTROL_WB_NUM_ELE_0 8
static int soc_wb_boolean_controls_0[SOC_CONTROL_WB_NUM_ELE_0] =
{
    socControlUseGport,
    socControlL2PortBlocking,
    socControlAbortOnError,
    -1,
    -1,
    -1,
    -1,
    -1
};

/*
 * Function:
 *      soc_switch_control_scache_size_get
 * Description:
 *      Get the size of scache size required for level 2 WarmBoot.
 * Parameters:
 *      unit -    (IN) SOC device number.
 *      part -    (IN) Warmboot partition number.
 *      version - (IN) WarmBoot Level 2 scache version for switch module.
 * Returns:
 *      Size of scache space required in bytes.
 */
STATIC int
soc_switch_control_scache_size_get(int unit, int part, int version)
{
    int alloc_size = 0;

    if (part == SOC_SCACHE_SWITCH_CONTROL_PART0) {
        if (version >= SOC_CONTROL_WB_PART_0_VER_1_0) {
            alloc_size += 1;
        }

        /*
         * if (version >= SOC_CONTROL_WB_PART_0_VER_1_1) {
         *     // alloc_size += n;
         * }
         *
         */

    }

    return alloc_size;
}

/*
 * Function:
 *      soc_switch_control_scache_init
 * Description:
 *      WarmBoot recovery procedure for recovering global switch controls.
 * Parameters:
 *      unit -    (IN) BCM device number.
 * Returns:
 *      BCM_E_XXX
 */
int
soc_switch_control_scache_init(int unit)
{
    int rv = SOC_E_NONE;
    int i, arg, part;
    int stable_size, stable_used;
    uint32 alloc_get, alloc_sz, realloc_sz;
    uint8 *switch_scache;
    uint16 default_ver, recovered_ver;
    soc_scache_handle_t  scache_handle;

    /* Limited scache mode unsupported */
    if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
        return (SOC_E_NONE);
    }

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if (!stable_size) {
        /* stable not allocated */
        return SOC_E_NONE;
    }

    realloc_sz = 0;
    part = SOC_SCACHE_SWITCH_CONTROL_PART0;
    default_ver = SOC_CONTROL_WB_PART_0_DEFAULT_VER;

    alloc_sz = soc_switch_control_scache_size_get(unit, part, default_ver);

    SOC_SCACHE_ALIGN_SIZE(alloc_sz);
    alloc_sz += SOC_WB_SCACHE_CONTROL_SIZE;

    SOC_IF_ERROR_RETURN(soc_stable_used_get(unit, &stable_used));

    if (stable_size < (stable_used + alloc_sz)) {
        /* stable used up */
        return SOC_E_RESOURCE;
    }

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, SOC_SCACHE_SWITCH_CONTROL, part);

    /* Get the pointer for the Level 2 cache */
    rv = soc_scache_ptr_get(unit, scache_handle,
                            &switch_scache, &alloc_get);

    if (SOC_E_NOT_FOUND == rv) {
        /* Not yet allocated - Cold Boot or Warm Upgrade */
        SOC_IF_ERROR_RETURN
            (soc_scache_alloc(unit, scache_handle, alloc_sz));

        rv = soc_scache_ptr_get(unit, scache_handle,
                                &switch_scache, &alloc_get);
        if (SOC_FAILURE(rv)) {
            return rv;
        } else if (alloc_get != alloc_sz) {
            /* Expected size doesn't match retrieved size */
            return SOC_E_INTERNAL;
        } else if (NULL == switch_scache) {
            return SOC_E_MEMORY;
        }
    } else if (SOC_FAILURE(rv)) {
        return rv;
    }

    /* recover version info */
    sal_memcpy(&recovered_ver, switch_scache, sizeof(uint16));

    /* Advance over scache control info */
    switch_scache += SOC_WB_SCACHE_CONTROL_SIZE;

    /* If cold-booting or re-init in cold, return from here */
    if (!SOC_WARM_BOOT(unit)) {
        return SOC_E_NONE;
    }

    /* Scache Version 1.0 */
    if (recovered_ver >= SOC_CONTROL_WB_PART_0_VER_1_0) {
        for (i = 0; (i < SOC_CONTROL_WB_NUM_ELE_0); i++) {
            if (-1 != soc_wb_boolean_controls_0[i]) {
                arg = !!(*switch_scache & (1 << i));
                switch (i) {
                    case socControlUseGport:
                        SOC_USE_GPORT_SET(unit, arg);
                        break;
                    case socControlL2PortBlocking:
                        SOC_L2X_GROUP_ENABLE_SET(unit, arg);
                        break;
                    case socControlAbortOnError:
#ifdef BCM_CB_ABORT_ON_ERR
                        SOC_CB_ABORT_ON_ERR(unit) = arg;
#endif /* BCM_CB_ABORT_ON_ERR */
                        break;
                    default:
                        break;
                }
            }
        }
        switch_scache++;
    }

    /*
     * if (recovered_ver >= SOC_CONTROL_WB_PART_0_VER_1_1) {
     *    // Recover state corresponding to WB version 1.1
     * }
     *
     */

    realloc_sz += alloc_sz - alloc_get;

    if (realloc_sz > 0) {
        rv = soc_scache_realloc(unit, scache_handle, realloc_sz);
    }

    return rv;
}

/*
 * Function:
 *      soc_switch_control_scache_sync
 * Description:
 *      Save golbal switch state to scache.
 * Parameters:
 *      unit -    (IN) BCM device number.
 * Returns:
 *      SOC_E_XXX
 */
int
soc_switch_control_scache_sync(int unit)
{
    int                  i, arg;
    int                  rv = SOC_E_NONE;
    uint8                sc_map = 0;
    uint16               default_ver;
    uint32               alloc_get;
    uint8                *switch_scache;
    soc_scache_handle_t  scache_handle;

    /* Limited scache mode unsupported */
    if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
        return SOC_E_NONE;
    }

    default_ver = SOC_CONTROL_WB_PART_0_DEFAULT_VER;
    SOC_SCACHE_HANDLE_SET(scache_handle, unit,
                          SOC_SCACHE_SWITCH_CONTROL,
                          SOC_SCACHE_SWITCH_CONTROL_PART0);

    /* Get the pointer for the Level 2 cache */
    rv = soc_scache_ptr_get(unit, scache_handle,
                            &switch_scache, &alloc_get);

    SOC_IF_ERROR_RETURN(rv);

    /* save default version */
    sal_memcpy(switch_scache, &default_ver, sizeof(uint16));
    switch_scache += SOC_WB_SCACHE_CONTROL_SIZE;

    /* Enable bits if the corresponding switch control is set */
    for (i = 0; (i < SOC_CONTROL_WB_NUM_ELE_0); i++) {
        if (-1 != soc_wb_boolean_controls_0[i]) {
            arg = 0;
            switch (i) {
                case socControlUseGport:
                    arg = SOC_USE_GPORT(unit);
                    break;
                case socControlL2PortBlocking:
                    arg = SOC_L2X_GROUP_ENABLE_GET(unit);
                    break;
                case socControlAbortOnError:
#ifdef BCM_CB_ABORT_ON_ERR
                    arg = SOC_CB_ABORT_ON_ERR(unit);
#endif /* BCM_CB_ABORT_ON_ERR */
                    break;
                default:
                    break;
            }
            if (arg) {
                sc_map |= (1 << i);
            }
        }
    }
    *switch_scache = sc_map;

    switch_scache++;

    return rv;
}

#else
int _soc_esw_switch_not_empty;
#endif /* BCM_WARM_BOOT_SUPPORT */

