/* 
 * $Id: oam.c,v 1.9 Broadcom SDK $
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
 * File:        bcmx/oam.c
 * Purpose:     BCMX OAM APIs
 *
 */

#include <bcm/types.h>

#include <bcmx/lport.h>
#include <bcmx/oam.h>

#include "bcmx_int.h"

#define BCMX_OAM_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_OAM_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_OAM_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_OAM_DELETE_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_DELETE_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_OAM_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)


#define BCMX_OAM_GROUP_INFO_T_PTR_TO_BCM(_info)    \
    ((bcm_oam_group_info_t *)(_info))

#define BCMX_OAM_ENDPOINT_INFO_T_PTR_TO_BCM(_info)    \
    ((bcm_oam_endpoint_info_t *)(_info))


/*
 * Function:
 *     bcmx_oam_group_info_t_init
 */
void
bcmx_oam_group_info_t_init(bcmx_oam_group_info_t *group_info)
{
    if (group_info != NULL) {
        bcm_oam_group_info_t_init(BCMX_OAM_GROUP_INFO_T_PTR_TO_BCM
                                  (group_info));
    }
}

/*
 * Function:
 *     bcmx_oam_endpoint_info_t_init
 */
void
bcmx_oam_endpoint_info_t_init(bcmx_oam_endpoint_info_t *endpoint_info)
{
    if (endpoint_info != NULL) {
        bcm_oam_endpoint_info_t_init(BCMX_OAM_ENDPOINT_INFO_T_PTR_TO_BCM
                                     (endpoint_info));
    }
}


/*
 * Function:
 *     bcmx_oam_init
 * Purpose:
 *     Initialize the OAM subsystem.
 * Parameters:
 *     None
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_init(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_OAM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_init(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_OAM_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_oam_detach
 * Purpose:
 *     Shut down the OAM subsystem.
 * Parameters:
 *     None
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_detach(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_OAM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_detach(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_OAM_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_oam_group_create
 * Purpose:
 *     Create or replace an OAM group object.
 * Parameters:
 *     group_info - (IN/OUT) Pointer to an OAM group structure
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_group_create(bcmx_oam_group_info_t *group_info)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;
    uint32  flags_orig;

    BCMX_OAM_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(group_info);

    /* Store original 'xxx_WITH_ID' flag bit */
    flags_orig = group_info->flags & BCM_OAM_GROUP_WITH_ID;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_group_create(bcm_unit,
                                      BCMX_OAM_GROUP_INFO_T_PTR_TO_BCM
                                      (group_info));
        if (BCM_FAILURE(BCMX_OAM_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv))) {
            break;
        }
        
        /*
         * Use the ID from first successful 'create' if group ID
         * is not specified.
         */
        if (!(group_info->flags & BCM_OAM_GROUP_WITH_ID)) {
            if (BCM_SUCCESS(tmp_rv)) {
                group_info->flags |= BCM_OAM_GROUP_WITH_ID;
            }
        }
    }

    /* Restore 'xxx_WITH_ID' flag bit */
    group_info->flags &= ~BCM_OAM_GROUP_WITH_ID;
    group_info->flags |= flags_orig;

    return rv;
}

/*
 * Function:
 *     bcmx_oam_group_get
 * Purpose:
 *     Get an OAM group object.
 * Parameters:
 *     group      - The ID of the group object to get
 *     group_info - (OUT) Pointer to an OAM group structure
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_group_get(bcm_oam_group_t group, bcmx_oam_group_info_t *group_info)
{
    int                    rv = BCM_E_UNAVAIL, tmp_rv;
    int                    i;
    int                    bcm_unit;
    bcmx_oam_group_info_t  group_info_orig, group_info_tmp;
    int                    first = TRUE;

    BCMX_OAM_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(group_info);

    group_info_orig = *group_info;

    /* Gather 'faults' flags from all units */
    BCMX_UNIT_ITER(bcm_unit, i) {
        group_info_tmp = group_info_orig;
        tmp_rv = bcm_oam_group_get(bcm_unit, group,
                                   BCMX_OAM_GROUP_INFO_T_PTR_TO_BCM
                                   (&group_info_tmp));
        if (BCMX_OAM_GET_IS_VALID(bcm_unit, tmp_rv)) {
            rv = tmp_rv;
            if (BCM_FAILURE(tmp_rv)) {
                break;
            }

            if (first) {
                *group_info = group_info_tmp;
                first = FALSE;
            } else {
                group_info->faults |=
                    group_info_tmp.faults;
                group_info->persistent_faults |=
                    group_info_tmp.persistent_faults;
            }
        }
    }

    return rv;
}

/*
 * Function:
 *     bcmx_oam_group_destroy
 * Purpose:
 *     Destroy an OAM group object.  All OAM endpoints associated
 *     with the group will also be destroyed.
 * Parameters:
 *     group - The ID of the OAM group object to destroy
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_group_destroy(bcm_oam_group_t group)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_OAM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_group_destroy(bcm_unit, group);
        BCM_IF_ERROR_RETURN
            (BCMX_OAM_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_oam_group_destroy_all
 * Purpose:
 *     Destroy all OAM group objects.  All OAM endpoints will also be
 *     destroyed.
 * Parameters:
 *     None
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_group_destroy_all(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_OAM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_group_destroy_all(bcm_unit);
        BCM_IF_ERROR_RETURN
            (BCMX_OAM_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_oam_endpoint_create
 * Purpose:
 *     Create or replace an OAM endpoint object.
 * Parameters:
 *     endpoint_info - (IN/OUT) Pointer to an OAM endpoint structure
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_endpoint_create(bcmx_oam_endpoint_info_t *endpoint_info)
{
    int          rv = BCM_E_UNAVAIL, tmp_rv;
    int          i, bcm_unit;
    bcm_port_t   bcm_port;
    bcm_gport_t  gport;
    uint32       flags_orig;

    BCMX_OAM_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(endpoint_info);

    /*
     * If gport is a physical port, apply to unit where port resides.
     * Else, apply to all units.
     */
    gport = endpoint_info->gport;
    if (BCM_SUCCESS(_bcmx_dest_to_unit_port(gport, &bcm_unit, &bcm_port,
                                            BCMX_DEST_CONVERT_DEFAULT))) {
        return bcm_oam_endpoint_create(bcm_unit,
                                       BCMX_OAM_ENDPOINT_INFO_T_PTR_TO_BCM
                                       (endpoint_info));

    }

    /* Store original 'xxx_WITH_ID' flag bit */
    flags_orig = endpoint_info->flags & BCM_OAM_ENDPOINT_WITH_ID;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_endpoint_create(bcm_unit,
                                         BCMX_OAM_ENDPOINT_INFO_T_PTR_TO_BCM
                                         (endpoint_info));
        if (BCM_FAILURE(BCMX_OAM_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv))) {
            break;
        }
        
        /*
         * Use the ID from first successful 'create' if group ID
         * is not specified.
         */
        if (!(endpoint_info->flags & BCM_OAM_ENDPOINT_WITH_ID)) {
            if (BCM_SUCCESS(tmp_rv)) {
                endpoint_info->flags |= BCM_OAM_ENDPOINT_WITH_ID;
            }
        }
    }

    /* Restore 'xxx_WITH_ID' flag bit */
    endpoint_info->flags &= ~BCM_OAM_ENDPOINT_WITH_ID;
    endpoint_info->flags |= flags_orig;

    return rv;
}

/*
 * Function:
 *     bcmx_oam_endpoint_get
 * Purpose:
 *     Get an OAM endpoint object.
 * Parameters:
 *     endpoint      - The ID of the endpoint object to get
 *     endpoint_info - (OUT) Pointer to an OAM endpoint structure
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_endpoint_get(bcm_oam_endpoint_t endpoint,
                      bcmx_oam_endpoint_info_t *endpoint_info)
{
    int                       rv = BCM_E_UNAVAIL, tmp_rv;
    int                       i;
    int                       bcm_unit;
    bcmx_oam_endpoint_info_t  endpoint_info_orig, endpoint_info_tmp;
    int                       first = TRUE;

    BCMX_OAM_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(endpoint_info);

    endpoint_info_orig = *endpoint_info;

    /* Gather 'faults' flags from all units */
    BCMX_UNIT_ITER(bcm_unit, i) {
        endpoint_info_tmp = endpoint_info_orig;
        tmp_rv = bcm_oam_endpoint_get(bcm_unit, endpoint,
                                      BCMX_OAM_ENDPOINT_INFO_T_PTR_TO_BCM
                                      (&endpoint_info_tmp));
        if (BCMX_OAM_GET_IS_VALID(bcm_unit, tmp_rv)) {
            rv = tmp_rv;
            if (BCM_FAILURE(tmp_rv)) {
                break;
            }

            if (first) {
                *endpoint_info = endpoint_info_tmp;
                first = FALSE;
            } else {
                endpoint_info->faults |=
                    endpoint_info_tmp.faults;
                endpoint_info->persistent_faults |=
                    endpoint_info_tmp.persistent_faults;
            }
        }
    }

    return rv;
}

/*
 * Function:
 *     bcmx_oam_endpoint_destroy
 * Purpose:
 *     Destroy an OAM endpoint object.
 * Parameters:
 *     endpoint - The ID of the OAM endpoint object to destroy
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_endpoint_destroy(bcm_oam_endpoint_t endpoint)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_OAM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_endpoint_destroy(bcm_unit, endpoint);
        BCM_IF_ERROR_RETURN
            (BCMX_OAM_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_oam_endpoint_destroy_all
 * Purpose:
 *     Destroy all OAM endpoint objects associated with a given OAM
 *     group.
 * Parameters:
 *     group - The OAM group whose endpoints should be destroyed
 * Results:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_oam_endpoint_destroy_all(bcm_oam_group_t group)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_OAM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_oam_endpoint_destroy_all(bcm_unit, group);
        BCM_IF_ERROR_RETURN
            (BCMX_OAM_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

