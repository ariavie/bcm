/*
 * $Id: oam.c,v 1.42 Broadcom SDK $
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
 */

#include <soc/defs.h>
#include <soc/drv.h>

#include <bcm/oam.h>
#include <bcm/error.h>
#include <bcm_int/esw/oam.h>

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif

#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif

#if defined(BCM_ENDURO_SUPPORT)
#include <bcm_int/esw/enduro.h>
#endif

#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif

#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#endif
static sal_mutex_t mutex[BCM_MAX_NUM_UNITS];

int bcm_esw_oam_lock(int unit)
{
    if (mutex[unit] == NULL)
    {
        return BCM_E_INIT;
    }

    sal_mutex_take(mutex[unit], sal_mutex_FOREVER);

    return BCM_E_NONE;
}

int bcm_esw_oam_unlock(int unit)
{
    if (mutex[unit] == NULL)
    {
        return BCM_E_INIT;
    }

    if (sal_mutex_give(mutex[unit]) != 0)
    {
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_esw_oam_init
 * Purpose:
 *      Initialize the OAM subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_init(int unit)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) ||\
    defined(BCM_GREYHOUND_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
            SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_init(unit);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_init(unit);
        } else
#endif
        {
            if (mutex[unit] == NULL)
            {
                mutex[unit] = sal_mutex_create("oam.mutex");

                if (mutex[unit] == NULL)
                {
                    result = BCM_E_MEMORY;
                }
            }
#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_init(unit);
            } else
#endif
            {
                result = bcm_tr2x_oam_init(unit);
            }

            if (BCM_FAILURE(result))
            {
                sal_mutex_destroy(mutex[unit]);

                mutex[unit] = NULL;
            }
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_detach
 * Purpose:
 *      Shut down the OAM subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_detach(int unit)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) ||\
	defined(BCM_GREYHOUND_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_detach(unit);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_detach(unit);
        } else
#endif
        {
            if (mutex[unit] == NULL)
            {
                return BCM_E_NONE;
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_detach(unit);
            } else
#endif
            {
                result = bcm_tr2x_oam_detach(unit);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));

            sal_mutex_destroy(mutex[unit]);

            mutex[unit] = NULL;
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_group_create
 * Purpose:
 *      Create or replace an OAM group object
 * Parameters:
 *      unit - (IN) Unit number.
 *      group_info - (IN/OUT) Pointer to an OAM group structure
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_group_create(
    int unit, 
    bcm_oam_group_info_t *group_info)
{
    int result = BCM_E_UNAVAIL;

    if (NULL == group_info) {
        /* Input parameter check. */
        return (BCM_E_PARAM);
    }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
        /* Check if alarm priority value is in valid range. */
        if ((group_info->lowest_alarm_priority
                < bcmOAMGroupFaultAlarmPriorityDefectsAll)
            || (group_info->lowest_alarm_priority
                > bcmOAMGroupFaultAlarmPriorityDefectsNone)) 
        {
            return BCM_E_PARAM;
        }

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_group_create(unit, group_info);
        } else
#endif
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_group_create(unit, group_info);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
        if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
        {
            result = bcm_en_oam_group_create(unit, group_info);
        } else
#endif
            {
                result = bcm_tr2x_oam_group_create(unit, group_info);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_group_get
 * Purpose:
 *      Get an OAM group object
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The ID of the group object to get
 *      group_info - (OUT) Pointer to an OAM group structure to receive the data
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_group_get(
    int unit, 
    bcm_oam_group_t group, 
    bcm_oam_group_info_t *group_info)
{
    int result = BCM_E_UNAVAIL;

    if (NULL == group_info) {
        /* Input parameter check. */
        return (BCM_E_PARAM);
    }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) || 
            SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_group_get(unit, group, group_info);
        } else
#endif
/* KT2 OAM  */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_group_get(unit, group, group_info);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));
#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_group_get(unit, group, group_info);
            } else
#endif
            {
                result = bcm_tr2x_oam_group_get(unit, group, group_info);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_group_destroy
 * Purpose:
 *      Destroy an OAM group object.  All OAM endpoints associated
 *      with the group will also be destroyed.
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The ID of the OAM group object to destroy
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_group_destroy(
    int unit, 
    bcm_oam_group_t group)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
            SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_group_destroy(unit, group);
        } else
#endif
/* KT2 OAM  */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_group_destroy(unit, group);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));
#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_group_destroy(unit, group);
            } else
#endif
            {
                result = bcm_tr2x_oam_group_destroy(unit, group);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_group_destroy_all
 * Purpose:
 *      Destroy all OAM group objects.  All OAM endpoints will also be
 *      destroyed.
 * Parameters:
 *      unit - (IN) Unit number.
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_group_destroy_all(
    int unit)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
            SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_group_destroy_all(unit);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_group_destroy_all(unit);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
        if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
        {
            result = bcm_en_oam_group_destroy_all(unit);
        } else
#endif
            {
                result = bcm_tr2x_oam_group_destroy_all(unit);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_group_traverse
 * Purpose:
 *      Traverse the entire set of OAM groups, calling a specified
 *      callback for each one
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) A pointer to the callback function to call for each OAM group
 *      user_data - (IN) Pointer to user data to supply in the callback
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_group_traverse(
    int unit, 
    bcm_oam_group_traverse_cb cb, 
    void *user_data)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit)||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_group_traverse(unit, cb, user_data);
        } else
#endif
/* KT2 OAM  */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_group_traverse(unit, cb, user_data);
        } else 
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));
#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_group_traverse(unit, cb, user_data);
            } else
#endif
            {
                result = bcm_tr2x_oam_group_traverse(unit, cb, user_data);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_endpoint_create
 * Purpose:
 *      Create or replace an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint_info - (IN/OUT) Pointer to an OAM endpoint structure
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_endpoint_create(
    int unit, 
    bcm_oam_endpoint_info_t *endpoint_info)
{
    int result = BCM_E_UNAVAIL;

    if (NULL == endpoint_info) {
        /* Input parameter check. */
        return (BCM_E_PARAM);
    }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit)
			|| SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_endpoint_create(unit, endpoint_info);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_endpoint_create(unit, endpoint_info);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));
#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_endpoint_create(unit, endpoint_info);
            } else
#endif
            {
                result = bcm_tr2x_oam_endpoint_create(unit, endpoint_info);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_endpoint_get
 * Purpose:
 *      Get an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint - (IN) The ID of the endpoint object to get
 *      endpoint_info - (OUT) Pointer to an OAM endpoint structure to receive the data
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_endpoint_get(
    int unit, 
    bcm_oam_endpoint_t endpoint, 
    bcm_oam_endpoint_info_t *endpoint_info)
{
    int result = BCM_E_UNAVAIL;

    if (NULL == endpoint_info) {
        /* Input parameter check. */
        return (BCM_E_PARAM);
    }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit)||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_endpoint_get(unit, endpoint, endpoint_info);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_endpoint_get(unit, endpoint, endpoint_info);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));
#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_endpoint_get(unit, endpoint, endpoint_info);
            } else
#endif
            {
                result = bcm_tr2x_oam_endpoint_get(unit, endpoint, endpoint_info);
            }
            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_endpoint_destroy
 * Purpose:
 *      Destroy an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint - (IN) The ID of the OAM endpoint object to destroy
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_endpoint_destroy(
    int unit, 
    bcm_oam_endpoint_t endpoint)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_endpoint_destroy(unit, endpoint);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_endpoint_destroy(unit, endpoint);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));
#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_endpoint_destroy(unit, endpoint);
            } else
#endif
            {
                result = bcm_tr2x_oam_endpoint_destroy(unit, endpoint);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_endpoint_destroy_all
 * Purpose:
 *      Destroy all OAM endpoint objects associated with a given OAM
 *      group
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The OAM group whose endpoints should be destroyed
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_endpoint_destroy_all(
    int unit, 
    bcm_oam_group_t group)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit)||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_endpoint_destroy_all(unit, group);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_endpoint_destroy_all(unit, group);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_endpoint_destroy_all(unit, group);
            } else
#endif
            {
                result = bcm_tr2x_oam_endpoint_destroy_all(unit, group);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_endpoint_traverse
 * Purpose:
 *      Traverse the set of OAM endpoints associated with the
 *      specified group, calling a specified callback for each one
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The OAM group whose endpoints should be traversed
 *      cb - (IN) A pointer to the callback function to call for each OAM endpoint in the specified group
 *      user_data - (IN) Pointer to user data to supply in the callback
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_endpoint_traverse(
    int unit, 
    bcm_oam_group_t group, 
    bcm_oam_endpoint_traverse_cb cb, 
    void *user_data)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_endpoint_traverse(unit, group, cb, user_data);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_endpoint_traverse(unit, group, cb, user_data);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_endpoint_traverse(unit, group, cb, user_data);
            } else
#endif
            {
                result = bcm_tr2x_oam_endpoint_traverse(unit, group, cb, user_data);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_event_register
 * Purpose:
 *      Register a callback for handling OAM events
 * Parameters:
 *      unit - (IN) Unit number.
 *      event_types - (IN) The set of OAM events for which the specified callback should be called
 *      cb - (IN) A pointer to the callback function to call for the specified OAM events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_event_register(
    int unit, 
    bcm_oam_event_types_t event_types, 
    bcm_oam_event_cb cb, 
    void *user_data)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit)||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_event_register(unit, event_types, cb, user_data);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_event_register(unit, event_types, cb, user_data);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_event_register(unit, event_types, cb, user_data);
            } else
#endif
            {
                result = bcm_tr2x_oam_event_register(unit, event_types, cb, user_data);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

/*
 * Function:
 *      bcm_esw_oam_event_unregister
 * Purpose:
 *      Unregister a callback for handling OAM events
 * Parameters:
 *      unit - (IN) Unit number.
 *      event_types - (IN) The set of OAM events for which the specified callback should not be called
 *      cb - (IN) A pointer to the callback function to unregister from the specified OAM events
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_oam_event_unregister(
    int unit, 
    bcm_oam_event_types_t event_types, 
    bcm_oam_event_cb cb)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit) ||
			SOC_IS_GREYHOUND(unit)) {
            result = bcm_tr3_oam_event_unregister(unit, event_types, cb);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = bcm_kt2_oam_event_unregister(unit, event_types, cb);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = bcm_en_oam_event_unregister(unit, event_types, cb);
            } else
#endif
            {
                result = bcm_tr2x_oam_event_unregister(unit, event_types, cb);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_esw_oam_sync
 * Purpose:
 *      Unregister a callback for handling OAM events
 * Parameters:
 *      unit - (IN) Unit number.
 *      event_types - (IN) The set of OAM events for which the specified callback should not be called
 *      cb - (IN) A pointer to the callback function to unregister from the specified OAM events
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_esw_oam_sync(int unit)
{
    int result = BCM_E_UNAVAIL;

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit)
			|| SOC_IS_GREYHOUND(unit)) {
            result = _bcm_tr3_oam_sync(unit);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            result = _bcm_kt2_oam_sync(unit);
        } else
#endif
        {
            BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

#if defined(BCM_ENDURO_SUPPORT)
            if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
            {
                result = _bcm_en_oam_sync(unit);
            }
            else
#endif
            {
                result = _bcm_tr2x_oam_sync(unit);
            }

            BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
        }
    }
#endif

    return result;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_oam_sw_dump
 * Purpose:
 *     Displays oam information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_oam_sw_dump(int unit)
{
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (soc_feature(unit, soc_feature_oam))
    {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_TRIUMPH3(unit)) || SOC_IS_HURRICANE2(unit)||
            SOC_IS_GREYHOUND(unit)) {
            _bcm_tr3_oam_sw_dump(unit);
        } else
#endif
/* KT2 OAM */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            _bcm_kt2_oam_sw_dump(unit);
        } else
#endif
#if defined(BCM_ENDURO_SUPPORT)
        if (SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
        {
            _bcm_en_oam_sw_dump(unit);
        }
        else
#endif
        {
            _bcm_tr2x_oam_sw_dump(unit);
        }
    }
#endif
    return;
}
#endif


int
bcm_esw_oam_loopback_add(int unit, bcm_oam_loopback_t *loopback_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh)))
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_loopback_add(unit, loopback_ptr);
        }
#endif

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_loopback_add(unit, loopback_ptr);
        }
#endif

#if defined(BCM_KATANA_SUPPORT)
        BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

        if (SOC_IS_KATANAX(unit))
        {
            result = bcm_en_oam_loopback_add(unit, loopback_ptr);
        }

        BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
#endif 

    }
#endif

    return result;
}

int
bcm_esw_oam_loopback_get(int unit, bcm_oam_loopback_t *loopback_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh)))
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_loopback_get(unit, loopback_ptr);
        }
#endif 

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_loopback_get(unit, loopback_ptr);
        }
#endif 

#if defined(BCM_KATANA_SUPPORT)
        BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

        if (SOC_IS_KATANAX(unit))
        {
            result = bcm_en_oam_loopback_get(unit, loopback_ptr);
        }

        BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
#endif

    }
#endif 

    return result;
}

int
bcm_esw_oam_loopback_delete(int unit, bcm_oam_loopback_t *loopback_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh)))
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_loopback_delete(unit, loopback_ptr);
        }
#endif

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_loopback_delete(unit, loopback_ptr);
        }
#endif

#if defined(BCM_KATANA_SUPPORT) 
        BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

        if (SOC_IS_KATANAX(unit))
        {
            result = bcm_en_oam_loopback_delete(unit, loopback_ptr);
        }

        BCM_IF_ERROR_RETURN(bcm_esw_oam_unlock(unit));
#endif

    }
#endif

    return result;
}

int 
bcm_esw_oam_loss_add(
    int unit, 
    bcm_oam_loss_t *loss_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh))) 
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_loss_add(unit, loss_ptr);
        }
#endif      /* KATANA2 */

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_loss_add(unit, loss_ptr);
        }
#endif      /* TRIUMPH3 */

    }
#endif      /* BHH */

    return result;
}

int 
bcm_esw_oam_loss_delete(
    int unit, 
    bcm_oam_loss_t *loss_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH) && defined(BCM_TRIUMPH3_SUPPORT)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh)))
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_loss_delete(unit, loss_ptr);
        } 
#endif      /* KATANA2 */

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_loss_delete(unit, loss_ptr);
        } 
#endif      /* TRIUMPH3 */


    }
#endif

    return result;
}

int 
bcm_esw_oam_loss_get(
    int unit, 
    bcm_oam_loss_t *loss_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh))) 
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_loss_get(unit, loss_ptr);
        }
#endif      /* KATANA2 */

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_loss_get(unit, loss_ptr);
        }
#endif      /* TRIUMPH3 */

    }
#endif      /* BHH */

    return result;
}

int 
bcm_esw_oam_delay_add(
    int unit, 
    bcm_oam_delay_t *delay_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh)))
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_delay_add(unit, delay_ptr);
        }
#endif      /* KATANA2 */

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_delay_add(unit, delay_ptr);
        }
#endif      /* TRIUMPH3 */

    }
#endif      /* BHH */

    return result;
}

int 
bcm_esw_oam_delay_delete(
    int unit, 
    bcm_oam_delay_t *delay_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)

    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh)))
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_delay_delete(unit, delay_ptr);
        } 
#endif /* KATANA2 */

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_delay_delete(unit, delay_ptr);
        } 
#endif /*TRIUMPH3 */

    }
#endif /* BHH */

    return result;
}

int 
bcm_esw_oam_delay_get(
    int unit, 
    bcm_oam_delay_t *delay_ptr)
{
    int result = BCM_E_UNAVAIL;

#if defined(INCLUDE_BHH)
    if ((soc_feature(unit, soc_feature_oam)) && (soc_feature(unit, soc_feature_bhh)))
    {

#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            return bcm_kt2_oam_delay_get(unit, delay_ptr);
        } 
#endif /* KATANA2 */

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            return bcm_tr3_oam_delay_get(unit, delay_ptr);
        } 
#endif /* TRIUMPH3 */

    }
#endif /* BHH */

    return result;
}

int 
bcm_esw_oam_endpoint_action_set(
    int unit,
    bcm_oam_endpoint_t id,
    bcm_oam_endpoint_action_t *action) 
{
    int result = BCM_E_UNAVAIL;
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_oam)) &&  (SOC_IS_KATANA2(unit))) { 
        result = bcm_kt2_oam_endpoint_action_set(unit, id, action); 
    } 
#endif /* BCM_KATANA2_SUPPORT */
    return result;
}

int 
bcm_esw_oam_control_get(
    int unit,
    bcm_oam_control_type_t type,
    uint64            *arg) 
{
    int result = BCM_E_UNAVAIL;
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_oam)) &&  (SOC_IS_KATANA2(unit))) { 
        result = bcm_kt2_oam_control_get(unit, type, arg); 
    } 
#endif /* BCM_KATANA2_SUPPORT */
    return result;
}

int 
bcm_esw_oam_control_set(
    int unit,
    bcm_oam_control_type_t type,
    uint64            arg) 
{
    int result = BCM_E_UNAVAIL;
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_oam)) &&  (SOC_IS_KATANA2(unit))) { 
        result = bcm_kt2_oam_control_set(unit, type, arg); 
    } 
#endif /* BCM_KATANA2_SUPPORT */
    return result;
}

