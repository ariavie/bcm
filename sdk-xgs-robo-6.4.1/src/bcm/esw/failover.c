/*
 * $Id: failover.c,v 1.26 Broadcom SDK $
 *
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
 * ESW failover API
 */

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>
#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw_dispatch.h>
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/field.h>
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#include <bcm/failover.h>
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */


#ifdef INCLUDE_L3

/*
 * Function:
 *		bcm_esw_failover_init
 * Purpose:
 *		Init  failover module
 * Parameters:
 *		IN :  unit
 * Returns:
 *		BCM_E_XXX
 */

int
bcm_esw_failover_init(int unit)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
        return bcm_tr2_failover_init(unit);
    }
#endif
    return BCM_E_UNAVAIL;
}


 /* Function:
 *      bcm_failover_cleanup
 * Purpose:
 *      Detach the failover module, clear all HW states
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXXX
 */

int
bcm_esw_failover_cleanup(int unit)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
        return bcm_tr2_failover_cleanup(unit);
    }
#endif
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *		bcm_esw_failover_create
 * Purpose:
 *		Create a failover object
 * Parameters:
 *		IN :  unit
 *           IN :  flags
 *           OUT :  failover_id
 * Returns:
 *		BCM_E_XXX
 */

int 
bcm_esw_failover_create(int unit, uint32 flags, bcm_failover_t *failover_id)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
              rv = bcm_tr2_failover_create(unit, flags, failover_id);
              bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		bcm_esw_failover_destroy
 * Purpose:
 *		Destroy a failover object
 * Parameters:
 *		IN :  unit
 *           IN :  failover_id
 * Returns:
 *		BCM_E_XXX
 */

int 
bcm_esw_failover_destroy(int unit, bcm_failover_t failover_id)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_lock (unit);
         if (  rv == BCM_E_NONE ) {
              rv = bcm_tr2_failover_destroy(unit, failover_id);
              bcm_tr2_failover_unlock (unit);
         }
         return rv;
   }
#endif
    return rv;
}

/*
 * Function:
 *		bcm_esw_failover_set
 * Purpose:
 *		Set a failover object to enable or disable (note that failover object
 *            0 is reserved
 * Parameters:
 *		IN :  unit
 *           IN :  failover_id
 *           IN :  value
 * Returns:
 *		BCM_E_XXX
 */


int 
bcm_esw_failover_set(int unit, bcm_failover_t failover_id, int value)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_KATANA2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    bcm_failover_element_t failover;
#endif

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_KATANA2(unit)) {
        bcm_failover_element_t_init (&failover);
        failover.failover_id = failover_id;
        rv = bcm_tr2_failover_lock (unit);
        if ( rv == BCM_E_NONE ) {
            rv = bcm_kt2_failover_status_set(unit, &failover, value);
            bcm_tr2_failover_unlock (unit);
        }
        return rv;
    }
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_TRIUMPH3(unit)) {
         bcm_failover_element_t_init (&failover);
         failover.failover_id = failover_id;
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
               rv = bcm_tr3_failover_status_set(unit, &failover, value);
               bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
               rv = bcm_tr2_failover_set(unit, failover_id, value);
               bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		bcm_esw_failover_get
 * Purpose:
 *		Get the enable status of a failover object
 * Parameters:
 *		IN :  unit
 *           IN :  failover_id
 *           OUT :  value
 * Returns:
 *		BCM_E_XXX
 */


int 
bcm_esw_failover_get(int unit, bcm_failover_t failover_id, int *value)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_KATANA2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        bcm_failover_element_t failover;
#endif
    
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_KATANA2(unit)) {
        bcm_failover_element_t_init (&failover);
        failover.failover_id = failover_id;
        rv = bcm_tr2_failover_lock (unit);
        if ( rv == BCM_E_NONE ) {
            rv = bcm_kt2_failover_status_get(unit, &failover, value);
            bcm_tr2_failover_unlock (unit);
        }
        return rv;
    }
#endif
    
#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_TRIUMPH3(unit)) {
         bcm_failover_element_t_init (&failover);
         failover.failover_id = failover_id;
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
               rv = bcm_tr3_failover_status_get(unit, &failover, value);
               bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
              rv = bcm_tr2_failover_get(unit, failover_id, value);
              bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		bcm_esw_failover_status_set
 * Purpose:
 *		Set a failover object to enable or disable (note that failover object
 *            0 is reserved
 * Parameters:
 *		IN :  unit
 *           IN :  failover
 *           IN :  value
 * Returns:
 *		BCM_E_XXX
 */


int 
bcm_esw_failover_status_set(int unit,
                                      bcm_failover_element_t *failover,
                                      int enable)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_KATANA2_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_KATANA2(unit)) {
        rv = bcm_tr2_failover_lock (unit);
        if ( rv == BCM_E_NONE ) {
            rv = bcm_kt2_failover_status_set(unit, failover, enable);
            bcm_tr2_failover_unlock (unit);
        }
        return rv;
    }
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_TRIUMPH3(unit)) {
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
               rv = bcm_tr3_failover_status_set(unit, failover, enable);
               bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
               rv = bcm_tr2_failover_set(unit, failover->failover_id, enable);
               bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		bcm_esw_failover_status_get
 * Purpose:
 *		Get the enable status of a failover object
 * Parameters:
 *		IN :  unit
 *           IN :  failover
 *           OUT :  value
 * Returns:
 *		BCM_E_XXX
 */


int 
bcm_esw_failover_status_get(int unit,
                                      bcm_failover_element_t *failover,
                                      int *value)
{
    int rv = BCM_E_UNAVAIL;
#ifdef BCM_KATANA2_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_KATANA2(unit)) {
        rv = bcm_tr2_failover_lock (unit);
        if ( rv == BCM_E_NONE ) {
            rv = bcm_kt2_failover_status_get(unit, failover, value);
            bcm_tr2_failover_unlock (unit);
        }
        return rv;
    }
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_TRIUMPH3(unit)) {
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
              rv = bcm_tr3_failover_status_get(unit, failover, value);
              bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_lock (unit);
         if ( rv == BCM_E_NONE ) {
              rv = bcm_tr2_failover_get(unit, failover->failover_id, value);
              bcm_tr2_failover_unlock (unit);
         }
         return rv;
    }
#endif
    return rv;
}


/*
 * Function:
 *		_bcm_esw_failover_mpls_check
 * Purpose:
 *		Check Failover for MPLS Port
 * Parameters:
 *		IN :  unit
 *           IN :  mpls_port
 * Returns:
 *		BCM_E_XXX
 */


int
_bcm_esw_failover_mpls_check(int unit, bcm_mpls_port_t  *mpls_port)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit))) {
         rv = bcm_tr3_failover_mpls_check (unit, mpls_port);
         return rv;
    }
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_mpls_check (unit, mpls_port);
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		_bcm_esw_failover_egr_check
 * Purpose:
 *		Check Failover for Egress Object
 * Parameters:
 *		IN :  unit
 *           IN :  Egress Object
 * Returns:
 *		BCM_E_XXX
 */


int
_bcm_esw_failover_egr_check(int unit, bcm_l3_egress_t  *egr)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit))) {
         rv = bcm_tr3_failover_egr_check (unit, egr);
         return rv;
    }
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_egr_check (unit, egr);
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *      _bcm_esw_failover_extender_check
 * Purpose:
 *      Check Failover for EXTENDER Port
 * Parameters:
 *      IN :  unit
 *      IN :  extender_port
 * Returns:
 *      BCM_E_XXX
 */


int
_bcm_esw_failover_extender_check(int unit, 
                                 bcm_extender_port_t *extender_port)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit))) {
        rv = bcm_tr3_failover_extender_check(unit, extender_port);
        return rv;
    }
#endif

    return rv;
}

/*
 * Function:
 *		_bcm_esw_failover_prot_nhi_create
 * Purpose:
 *		Create Protection next_hop
 * Parameters:
 *		IN :  unit
  *          IN :  Next_Hop_Index
 *           IN :  Egress Object
 * Returns:
 *		BCM_E_XXX
 */


int
_bcm_esw_failover_prot_nhi_create(int unit, uint32 flags,
                                              int nh_index, int prot_nh_index,
                                              bcm_multicast_t mc_group,
                                              bcm_failover_t failover_id)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit))) {
        BCM_IF_ERROR_RETURN(bcm_tr2_failover_prot_nhi_create( unit, nh_index ));
        rv = bcm_tr3_failover_prot_nhi_set(unit, flags, nh_index, 
                         prot_nh_index, mc_group, failover_id );
          return rv;
    }
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
        BCM_IF_ERROR_RETURN(bcm_tr2_failover_prot_nhi_create( unit, nh_index ));
        rv = bcm_tr2_failover_prot_nhi_set(unit, nh_index,
                         prot_nh_index, failover_id );
        return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		_bcm_esw_failover_prot_nhi_cleanup
 * Purpose:
 *		Cleanup protection Next_hop
 * Parameters:
 *		IN :  unit
  *          IN :  Next_Hop_Index
 * Returns:
 *		BCM_E_XXX
 */


int
_bcm_esw_failover_prot_nhi_cleanup(int unit, int nh_index)
{
    int rv = BCM_E_UNAVAIL;

#ifdef BCM_KATANA2_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_KATANA2(unit)) {
        rv = bcm_kt2_failover_prot_nhi_cleanup  (unit, nh_index);
        return rv;
    }
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_failover) && SOC_IS_TRIUMPH3(unit)) {
         rv = bcm_tr3_failover_prot_nhi_cleanup  (unit, nh_index);
         return rv;
    }
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_prot_nhi_cleanup  (unit, nh_index);
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		_bcm_esw_failover_prot_nhi_get
 * Purpose:
 *		Cleanup protection Next_hop
 * Parameters:
 *		IN :  unit
  *          IN :  Next_Hop_Index
 * Returns:
 *		BCM_E_XXX
 */

int
_bcm_esw_failover_prot_nhi_get (int unit, int nh_index,
         bcm_failover_t *failover_id, int *prot_nh_index, int *multicast_group)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit))) {
         rv = bcm_tr3_failover_prot_nhi_get (unit, nh_index, 
                        failover_id, prot_nh_index, multicast_group);
         return rv;
    }
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
         rv = bcm_tr2_failover_prot_nhi_get  (unit, nh_index, 
                        failover_id, prot_nh_index);
         return rv;
    }
#endif
    return rv;
}

/*
 * Function:
 *		_bcm_esw_failover_prot_nhi_update
 * Purpose:
 *		Update protection Next_hop
 * Parameters:
 *		IN :  unit
  *          IN :  Next_Hop_Index
 * Returns:
 *		BCM_E_XXX
 */

int
_bcm_esw_failover_prot_nhi_update (int unit,
                                                int old_nh_index,
                                                int new_nh_index)

{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit))) {
        rv = bcm_tr3_failover_prot_nhi_update (unit,
                old_nh_index, new_nh_index);
        return rv;
    }
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_failover)) {
        rv = bcm_tr2_failover_prot_nhi_update (unit,
                old_nh_index, new_nh_index);
        return rv;
    }
#endif
    return rv;
}

#else   /* INCLUDE_L3 */
int bcm_esw_failover_not_empty;
#endif  /* INCLUDE_L3 */

int
bcm_esw_failover_ring_config_get(int unit,
                                             bcm_failover_ring_t *failover_ring)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit))) {
         return bcm_tr3_failover_ring_config_get(unit, failover_ring);
    }
#endif

    return rv;
}

int
bcm_esw_failover_ring_config_set(int unit,
                                            bcm_failover_ring_t *failover_ring)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)

    if (soc_feature(unit, soc_feature_failover) &&
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit))) {
         return bcm_tr3_failover_ring_config_set(unit, failover_ring);
    }
#endif

    return rv;
}

