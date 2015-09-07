/*
 * $Id: niv.c 1.16.4.2 Broadcom SDK $
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
 * Purpose: Implements ESW NIV APIs
 */

#if defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>

#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/niv.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/trident2.h>

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
/* Flag to check initialized status */
STATIC int niv_initialized[BCM_MAX_NUM_UNITS];

#define NIV_INIT(unit)                                    \
    do {                                                  \
        if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {  \
            return BCM_E_UNIT;                            \
        }                                                 \
        if (!niv_initialized[unit]) {                     \
            return BCM_E_INIT;                            \
        }                                                 \
    } while (0)

/* 
 * NIV module lock
 */
STATIC sal_mutex_t niv_mutex[BCM_MAX_NUM_UNITS] = {NULL};

#define NIV_LOCK(unit) \
        sal_mutex_take(niv_mutex[unit], sal_mutex_FOREVER);

#define NIV_UNLOCK(unit) \
        sal_mutex_give(niv_mutex[unit]); 

/*
 * Function:
 *      _bcm_niv_check_init
 * Purpose:
 *      Check if NIV is initialized
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_niv_check_init(int unit)
{
    if (!niv_initialized[unit]) {
        return BCM_E_INIT;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_niv_free_resources
 * Purpose:
 *      Free NIV resources
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */
STATIC void
_bcm_esw_niv_free_resources(int unit)
{
    if (niv_mutex[unit]) {
        sal_mutex_destroy(niv_mutex[unit]);
        niv_mutex[unit] = NULL;
    } 
}
#endif /* BCM_TRIDENT_SUPPORT || INCLUDE_L3 */

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0     SOC_SCACHE_VERSION(1,0)
#define BCM_WB_VERSION_1_1     SOC_SCACHE_VERSION(1,1)
#define BCM_WB_DEFAULT_VERSION BCM_WB_VERSION_1_1

/*
 * Function:
 *      _bcm_esw_niv_sync
 * Purpose:
 *      Record NIV module persistent info for Level 2 Warm Boot
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_esw_niv_sync(int unit)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        soc_scache_handle_t scache_handle;
        uint8               *scache_ptr;

        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_NIV, 0);
        BCM_IF_ERROR_RETURN
            (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                     0, &scache_ptr,
                                     BCM_WB_DEFAULT_VERSION, NULL));

        BCM_IF_ERROR_RETURN(bcm_trident_niv_sync(unit, &scache_ptr));
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_NONE;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      bcm_esw_niv_init
 * Purpose:
 *      Initialize NIV module
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_init(int unit)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {

#ifdef BCM_WARM_BOOT_SUPPORT
        {
            uint32              required_scache_size;
            soc_scache_handle_t scache_handle;
            uint8               *scache_ptr;
            int                 rv = BCM_E_NONE;

            /* Get the required scache size */
            BCM_IF_ERROR_RETURN(bcm_trident_niv_required_scache_size_get(unit,
                        &required_scache_size));

            /* Allocate required scache */
            SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_NIV, 0);
            if (required_scache_size > 0) {
                rv = _bcm_esw_scache_ptr_get(unit, scache_handle, 
                        (0 == SOC_WARM_BOOT(unit)), required_scache_size,
                        &scache_ptr, BCM_WB_DEFAULT_VERSION, NULL);
                if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                    return rv;
                }
            }
        }
#endif /* BCM_WARM_BOOT_SUPPORT */

        if (niv_initialized[unit]) {
            BCM_IF_ERROR_RETURN(bcm_esw_niv_cleanup(unit));
        }

        BCM_IF_ERROR_RETURN(bcm_trident_niv_init(unit));

        if (niv_mutex[unit] == NULL) {
            niv_mutex[unit] = sal_mutex_create("niv mutex");
            if (niv_mutex[unit] == NULL) {
                bcm_esw_niv_cleanup(unit);
                return BCM_E_MEMORY;
            }
        }

#ifdef BCM_WARM_BOOT_SUPPORT
        if (SOC_WARM_BOOT(unit)) {
            BCM_IF_ERROR_RETURN(bcm_trident_niv_reinit(unit));
        }
#endif /* BCM_WARM_BOOT_SUPPORT */

        niv_initialized[unit] = TRUE;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_cleanup
 * Purpose:
 *      Detach NIV module, clear all HW states
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_cleanup(int unit)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {

        BCM_IF_ERROR_RETURN(bcm_trident_niv_cleanup(unit));

        _bcm_esw_niv_free_resources(unit);

        niv_initialized[unit] = FALSE;

        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_port_add
 * Purpose:
 *      Create a NIV port
 * Parameters:
 *      unit - (IN) SOC unit Number
 *      niv_port - (IN/OUT) NIV port information (OUT : niv_port_id)
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_niv_port_add(int unit, bcm_niv_port_t *niv_port)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {

        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_port_add(unit, niv_port);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}
  
/* Function:
 *      bcm_esw_niv_port_delete
 * Purpose:
 *      Delete a NIV port
 * Parameters:
 *      unit - (IN) SOC unit number
 *      niv_port_id - (IN) NIV port ID
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_niv_port_delete(int unit, bcm_gport_t niv_port_id)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_port_delete(unit, niv_port_id);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_port_delete_all
 * Purpose:
 *      Delete all NIV ports
 * Parameters:
 *      unit - (IN) SOC unit number
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_niv_port_delete_all(int unit)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_port_delete_all(unit);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_port_get
 * Purpose:
 *      Get info about a NIV port
 * Parameters:
 *      unit - (IN) SOC unit number
 *      niv_port - (IN/OUT) NIV port information (IN : niv_port_id)
 * Returns:
 *      BCM_E_XXX
 */
int bcm_esw_niv_port_get(int unit, bcm_niv_port_t *niv_port)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_port_get(unit, niv_port);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_port_traverse
 * Purpose:
 *      Traverse all valid NIV port entries and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per NIV Port entry.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_port_traverse(int unit,
                          bcm_niv_port_traverse_cb cb,
                          void *user_data)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_port_traverse(unit, cb, user_data);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_forward_add
 * Purpose:
 *      Create a NIV forwarding table entry
 * Parameters:
 *      unit - (IN) Device Number
 *      iv_fwd_entry - (IN) NIV forwarding table entry
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_forward_add(int unit, bcm_niv_forward_t *iv_fwd_entry)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            rv = bcm_tr3_niv_forward_add(unit, iv_fwd_entry);
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            rv = bcm_trident_niv_forward_add(unit, iv_fwd_entry);
        }

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}
  
/* Function:
 *      bcm_esw_niv_forward_delete
 * Purpose:
 *	Delete a NIV forwarding table entry
 * Parameters:
 *      unit - (IN) Device Number
 *      iv_fwd_entry - (IN) NIV forwarding table entry
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_forward_delete(int unit, bcm_niv_forward_t *iv_fwd_entry)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            rv = bcm_tr3_niv_forward_delete(unit, iv_fwd_entry);
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            rv = bcm_trident_niv_forward_delete(unit, iv_fwd_entry);
        }

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_forward_delete_all
 * Purpose:
 *      Delete all NIV Forwarding table entries
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_forward_delete_all(int unit)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            rv = bcm_tr3_niv_forward_delete_all(unit);
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            rv = bcm_trident_niv_forward_delete_all(unit);
        }

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_forward_get
 * Purpose:
 *      Get NIV forwarding table entry
 * Parameters:
 *      unit - (IN) Device Number
 *      iv_fwd_entry - (IN/OUT) NIV forwarding table info
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_forward_get(int unit, bcm_niv_forward_t *iv_fwd_entry)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            rv = bcm_tr3_niv_forward_get(unit, iv_fwd_entry);
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            rv = bcm_trident_niv_forward_get(unit, iv_fwd_entry);
        }

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_forward_traverse
 * Purpose:
 *      Traverse all valid NIV forward entries and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per NIV forward entry.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_forward_traverse(int unit,
                             bcm_niv_forward_traverse_cb cb,
                             void *user_data)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            rv = bcm_tr3_niv_forward_traverse(unit, cb, user_data);
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            rv = bcm_trident_niv_forward_traverse(unit, cb, user_data);
        }

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_niv_port_source_vp_lag_set
 * Purpose:
 *      Set source VP LAG for a NIV virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) NIV virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_niv_port_source_vp_lag_set(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv) &&
        soc_feature(unit, soc_feature_vp_lag)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_td2_niv_port_source_vp_lag_set(unit, gport, vp_lag_vp);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_niv_port_source_vp_lag_clear
 * Purpose:
 *      Clear source VP LAG for a NIV virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) NIV virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_niv_port_source_vp_lag_clear(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv) &&
        soc_feature(unit, soc_feature_vp_lag)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_td2_niv_port_source_vp_lag_clear(unit, gport, vp_lag_vp);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_esw_niv_port_source_vp_lag_get
 * Purpose:
 *      Get source VP LAG for a NIV virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) NIV virtual port GPORT ID. 
 *      vp_lag_vp - (OUT) VP representing the VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_niv_port_source_vp_lag_get(int unit, bcm_gport_t gport,
        int *vp_lag_vp)
{
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv) &&
        soc_feature(unit, soc_feature_vp_lag)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_td2_niv_port_source_vp_lag_get(unit, gport, vp_lag_vp);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_egress_add
 * Purpose:
 *      Add a NIV egress object to a NIV port
 * Parameters:
 *      unit - (IN) SOC unit Number
 *      niv_port - (IN) NIV port
 *      niv_egress - (IN/OUT) NIV egress object, the egress_if field is output.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_egress_add(
    int unit, 
    bcm_gport_t niv_port, 
    bcm_niv_egress_t *niv_egress)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_egress_add(unit, niv_port, niv_egress);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_egress_delete
 * Purpose:
 *      Delete a NIV egress object from a NIV port
 * Parameters:
 *      unit - (IN) SOC unit Number
 *      niv_port - (IN) NIV port
 *      niv_egress - (IN) NIV egress object
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_egress_delete(
    int unit, 
    bcm_gport_t niv_port, 
    bcm_niv_egress_t *niv_egress)
{

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_egress_delete(unit, niv_port, niv_egress);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_egress_set
 * Purpose:
 *      Set an array of NIV egress objects for a NIV port
 * Parameters:
 *      unit - (IN) SOC unit Number
 *      niv_port - (IN) NIV port
 *      array_size - (IN) Number of NIV egress objects
 *      niv_egress_array - (IN/OUT) Array of NIV egress objects. Each NIV
 *                                  egress structure's egress_if field is
 *                                  an output.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_niv_egress_set(
    int unit, 
    bcm_gport_t niv_port, 
    int array_size, 
    bcm_niv_egress_t *niv_egress_array)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_egress_set(unit, niv_port, array_size,
                niv_egress_array);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_egress_get
 * Purpose:
 *      Get an array of NIV egress objects for a NIV port
 * Parameters:
 *      unit - (IN) SOC unit Number
 *      niv_port - (IN) NIV port
 *      array_size - (IN) Size of NIV egress objects array.
 *      niv_egress_array - (OUT) Array of NIV egress objects.
 *      count - (OUT) Number of NIV egress objects returned.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_niv_egress_get(
    int unit, 
    bcm_gport_t niv_port, 
    int array_size, 
    bcm_niv_egress_t *niv_egress_array, 
    int *count)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int rv;

        NIV_INIT(unit);

        NIV_LOCK(unit);

        rv = bcm_trident_niv_egress_get(unit, niv_port, array_size,
                niv_egress_array, count);

        NIV_UNLOCK(unit);

        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

   return BCM_E_UNAVAIL;
}

/* Function:
 *      bcm_esw_niv_egress_delete_all
 * Purpose:
 *      Delete all NIV egress objects from a NIV port
 * Parameters:
 *      unit - (IN) SOC unit Number
 *      niv_port - (IN) NIV port
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_niv_egress_delete_all(
    int unit, 
    bcm_gport_t niv_port)
{
    return bcm_esw_niv_egress_set(unit, niv_port, 0, NULL);
}


#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_niv_sw_dump
 * Purpose:
 *     Displays NIV information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_niv_sw_dump(int unit)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        bcm_trident_niv_sw_dump(unit);
    }
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else   /* INCLUDE_L3 */
int bcm_esw_niv_not_empty;
#endif  /* INCLUDE_L3 */

