/*
 * $Id: bregex.c 1.11 Broadcom SDK $
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
 * COS Queue Management
 * Purpose: API to program Regex engine for flow tracking
 */

#include <sal/core/libc.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>

#include <bcm/error.h>
#include <bcm/cosq.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/scorpion.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/hurricane.h>
#include <bcm_int/esw/fb4regex.h>

#include <bcm_int/esw_dispatch.h>

#if defined(INCLUDE_REGEX)

int bcm_esw_regex_config_set(int unit, bcm_regex_config_t *config)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_config_set(unit, config);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_config_get(int unit, bcm_regex_config_t *config)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_config_get(unit, config);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_exclude_add(int unit, uint8 protocol, 
                            uint16 l4_start, uint16 l4_end)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_exclude_add(unit, protocol, l4_start, l4_end);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_exclude_delete(int unit, uint8 protocol, 
                            uint16 l4_start, uint16 l4_end)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_exclude_delete(unit, protocol, l4_start, l4_end);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_exclude_delete_all(int unit)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_exclude_delete_all(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_exclude_get(int unit, int array_size, uint8 *protocol, 
                                uint16 *l4low, uint16 *l4high, int *array_count)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

    if (!(array_size && protocol && l4low && l4high && array_count)) {
        return BCM_E_PARAM;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_exclude_get(unit, array_size, protocol,
                                         l4low, l4high, array_count);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_engine_create(int unit, bcm_regex_engine_config_t *config, 
                            bcm_regex_engine_t *engid)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_engine_create(unit, config, engid);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_engine_destroy(int unit, bcm_regex_engine_t engid)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_engine_destroy(unit, engid);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_engine_traverse(int unit, bcm_regex_engine_traverse_cb cb, 
                                void *user_data)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_engine_traverse(unit, cb, user_data);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_engine_get(int unit, bcm_regex_engine_t engid, 
                            bcm_regex_engine_config_t *config)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_engine_get(unit, engid, config);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_match_check(int unit, bcm_regex_match_t* match, 
                                int count, int* metric)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_match_check(unit, match, count, metric);

    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_match_set(int unit, bcm_regex_engine_t engid, 
                              bcm_regex_match_t* match, int count)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_match_set(unit, engid, match, count);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}


int bcm_esw_regex_report_register(int unit, uint32 reports, 
                                  bcm_regex_report_cb callback, void *user_data)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_esw_regex_report_register(unit, reports, 
                                             callback, user_data);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_report_unregister(int unit, uint32 reports, 
                                bcm_regex_report_cb callback, void *user_data)
{
    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_esw_regex_report_unregister(unit, reports, 
                                               callback, user_data);
    }
#endif /* BCM_TRIUMPH3_SUPPORT */    

    return BCM_E_UNAVAIL;
}

int bcm_esw_regex_init(int unit)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _bcm_tr3_regex_init(unit);
    }
#endif
    return BCM_E_UNAVAIL;
}

int _bcm_esw_regex_sync(int unit)
{
#ifdef BCM_WARM_BOOT_SUPPORT
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) && soc_feature(unit, soc_feature_regex)) {
        return _bcm_esw_tr3_regex_sync(unit);
    }
#endif
#endif
    return BCM_E_UNAVAIL;
}

#endif /* INCLUDE_REGEX */

