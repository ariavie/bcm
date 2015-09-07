/*
 * $Id: extender.c 1.3 Broadcom SDK $
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
 * Purpose: Implements ESW Port Extension APIs
 */

#if defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>

#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/extender.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/trident2.h>

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
/* Flag to check initialized status */
STATIC int extender_initialized[BCM_MAX_NUM_UNITS];

#define EXTENDER_INIT(unit)                               \
    do {                                                  \
        if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {  \
            return BCM_E_UNIT;                            \
        }                                                 \
        if (!extender_initialized[unit]) {                \
            return BCM_E_INIT;                            \
        }                                                 \
    } while (0)

/* 
 * Extender module lock
 */
STATIC sal_mutex_t extender_mutex[BCM_MAX_NUM_UNITS] = {NULL};

#define EXTENDER_LOCK(unit) \
        sal_mutex_take(extender_mutex[unit], sal_mutex_FOREVER);

#define EXTENDER_UNLOCK(unit) \
        sal_mutex_give(extender_mutex[unit]); 

/*
 * Function:
 *      _bcm_esw_extender_free_resources
 * Purpose:
 *      Free all allocated tables and memory
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */
STATIC void
_bcm_esw_extender_free_resources(int unit)
{
    if (extender_mutex[unit]) {
        sal_mutex_destroy(extender_mutex[unit]);
        extender_mutex[unit] = NULL;
    } 
}
#endif /* BCM_TRIUMPH3_SUPPORT || INCLUDE_L3 */

/*
 * Function:
 *      bcm_esw_extender_init
 * Purpose:
 *      Init extender module
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_extender_init(int unit)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        if (extender_initialized[unit]) {
            BCM_IF_ERROR_RETURN(bcm_esw_extender_cleanup(unit));
        }

        BCM_IF_ERROR_RETURN(bcm_tr3_extender_init(unit));

        if (extender_mutex[unit] == NULL) {
            extender_mutex[unit] = sal_mutex_create("extender mutex");
            if (extender_mutex[unit] == NULL) {
                _bcm_esw_extender_free_resources(unit);
                return BCM_E_MEMORY;
            }
        }

        extender_initialized[unit] = TRUE;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}


/* Function:
 *      bcm_extender_cleanup
 * Purpose:
 *      Detach extender module, clear all HW states
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      BCM_E_XXXX
 */
int
bcm_esw_extender_cleanup(int unit)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        BCM_IF_ERROR_RETURN(bcm_tr3_extender_cleanup(unit));

        _bcm_esw_extender_free_resources(unit);

        extender_initialized[unit] = FALSE;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_extender_port_add
 * Purpose:
 *      Create a Port Extender port.
 * Parameters:
 *      unit - (IN) SOC unit Number
 *      extender_port - (IN/OUT) Extender port information
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_extender_port_add(int unit, bcm_extender_port_t *extender_port)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_tr3_extender_port_add(unit, extender_port);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_extender_port_delete
 * Purpose:
 *      Delete a Port Extender port
 * Parameters:
 *      unit - (IN) SOC unit number
 *      extender_port_id - (IN) Extender port ID
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_extender_port_delete(int unit, bcm_gport_t extender_port_id)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {
        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_tr3_extender_port_delete(unit, extender_port_id);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_extender_port_delete_all
 * Purpose:
 *      Delete all Port Extender ports
 * Parameters:
 *      unit - (IN) SOC unit number
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_extender_port_delete_all(int unit)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {
        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_tr3_extender_port_delete_all(unit);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_extender_port_get
 * Purpose:
 *      Get info about a Port Extender port
 * Parameters:
 *      unit - (IN) SOC unit number
 *      extender_port - (IN/OUT) Extender port information
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_extender_port_get(int unit, bcm_extender_port_t *extender_port)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {
        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_tr3_extender_port_get(unit, extender_port);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_extender_port_traverse
 * Purpose:
 *      Traverse all valid Port Extender ports and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per Extender Port.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_extender_port_traverse(int unit,
                          bcm_extender_port_traverse_cb cb,
                          void *user_data)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {
        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_tr3_extender_port_traverse(unit, cb, user_data);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_extender_forward_add
 * Purpose:
 *      Add a downstream forwarding entry
 * Parameters:
 *      unit - (IN) Device Number
 *      extender_forward_entry - (IN) Forwarding entry
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_extender_forward_add(int unit,
        bcm_extender_forward_t *extender_forward_entry)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        if (soc_feature(unit, soc_feature_ism_memory)) {
            rv = bcm_tr3_extender_forward_add(unit, extender_forward_entry);
        } else {
#ifdef BCM_TRIDENT2_SUPPORT
            rv = bcm_td2_extender_forward_add(unit, extender_forward_entry);
#endif /* BCM_TRIDENT2_SUPPORT */
        }

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return rv;
}

/* Function:
 *      bcm_esw_extender_forward_delete
 * Purpose:
 *	Delete a downstream forwarding entry
 * Parameters:
 *      unit - (IN) Device Number
 *      extender_forward_entry - (IN) Forwarding entry
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_extender_forward_delete(int unit,
        bcm_extender_forward_t *extender_forward_entry)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        if (soc_feature(unit, soc_feature_ism_memory)) {
            rv = bcm_tr3_extender_forward_delete(unit, extender_forward_entry);
        } else {
#ifdef BCM_TRIDENT2_SUPPORT
            rv = bcm_td2_extender_forward_delete(unit, extender_forward_entry);
#endif /* BCM_TRIDENT2_SUPPORT */
        }

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return rv;
}

/* Function:
 *      bcm_esw_extender_forward_delete_all
 * Purpose:
 *      Delete all downstream forwarding entries
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_extender_forward_delete_all(int unit)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        if (soc_feature(unit, soc_feature_ism_memory)) {
            rv = bcm_tr3_extender_forward_delete_all(unit);
        } else {
#ifdef BCM_TRIDENT2_SUPPORT
            rv = bcm_td2_extender_forward_delete_all(unit);
#endif /* BCM_TRIDENT2_SUPPORT */
        }

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return rv;
}

/* Function:
 *      bcm_esw_extender_forward_get
 * Purpose:
 *      Get downstream forwarding entry
 * Parameters:
 *      unit - (IN) Device Number
 *      extender_forward_entry - (IN/OUT) Forwarding entry 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_extender_forward_get(int unit,
        bcm_extender_forward_t *extender_forward_entry)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        if (soc_feature(unit, soc_feature_ism_memory)) {
            rv = bcm_tr3_extender_forward_get(unit, extender_forward_entry);
        } else {
#ifdef BCM_TRIDENT2_SUPPORT
            rv = bcm_td2_extender_forward_get(unit, extender_forward_entry);
#endif /* BCM_TRIDENT2_SUPPORT */
        }

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return rv;
}

/* Function:
 *      bcm_esw_extender_forward_traverse
 * Purpose:
 *      Traverse all valid downstream forwarding entries and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per forwarding entry.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_extender_forward_traverse(int unit,
                             bcm_extender_forward_traverse_cb cb,
                             void *user_data)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        if (soc_feature(unit, soc_feature_ism_memory)) {
            rv = bcm_tr3_extender_forward_traverse(unit, cb, user_data);
        } else {
#ifdef BCM_TRIDENT2_SUPPORT
            rv = bcm_td2_extender_forward_traverse(unit, cb, user_data);
#endif /* BCM_TRIDENT2_SUPPORT */
        }

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return rv;
}

/*
 * Function:
 *      _bcm_esw_extender_port_source_vp_lag_set
 * Purpose:
 *      Set source VP LAG for an Extender virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) Extender virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_extender_port_source_vp_lag_set(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension) &&
        soc_feature(unit, soc_feature_vp_lag)) {
        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_td2_extender_port_source_vp_lag_set(unit, gport, vp_lag_vp);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_extender_port_source_vp_lag_clear
 * Purpose:
 *      Clear source VP LAG for an Extender virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) Extender virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_extender_port_source_vp_lag_clear(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension) &&
        soc_feature(unit, soc_feature_vp_lag)) {
        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_td2_extender_port_source_vp_lag_clear(unit, gport, vp_lag_vp);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_extender_port_source_vp_lag_get
 * Purpose:
 *      Get source VP LAG for an Extender virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) Extender virtual port GPORT ID. 
 *      vp_lag_vp - (OUT) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_extender_port_source_vp_lag_get(int unit, bcm_gport_t gport,
        int *vp_lag_vp)
{
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension) &&
        soc_feature(unit, soc_feature_vp_lag)) {
        int rv;

        EXTENDER_INIT(unit);

        EXTENDER_LOCK(unit);

        rv = bcm_td2_extender_port_source_vp_lag_get(unit, gport, vp_lag_vp);

        EXTENDER_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_extender_sw_dump
 * Purpose:
 *     Displays extender module information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_extender_sw_dump(int unit)
{
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_port_extension)) {
        bcm_tr3_extender_sw_dump(unit);
    }
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else   /* INCLUDE_L3 */
int bcm_esw_extender_not_empty;
#endif  /* INCLUDE_L3 */

