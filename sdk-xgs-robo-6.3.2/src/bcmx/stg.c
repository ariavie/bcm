/*
 * $Id: stg.c 1.12 Broadcom SDK $
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
 * File:    bcmx/stg.c
 * Purpose: stg.c BCMX source file
 */

#include <bcm/types.h>

#include <bcmx/stg.h>
#include <bcmx/lport.h>
#include <bcmx/bcmx.h>

#include "bcmx_int.h"

#define BCMX_STG_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_STG_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_STG_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_STG_DELETE_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_DELETE_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_STG_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)

#define BCMX_STG_ERROR_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)


int
bcmx_stg_init(void)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_stg_init(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_STG_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *      bcmx_stg_default_get
 */

int
bcmx_stg_default_get(bcm_stg_t *stg_ptr)
{
    int rv, i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(stg_ptr);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_stg_default_get(bcm_unit, stg_ptr);
        if (BCMX_STG_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcmx_stg_default_set
 */

int
bcmx_stg_default_set(bcm_stg_t stg)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_stg_default_set(bcm_unit, stg);
        BCM_IF_ERROR_RETURN(BCMX_STG_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *      bcmx_stg_vlan_add
 */

int
bcmx_stg_vlan_add(bcm_stg_t stg,
                  bcm_vlan_t vid)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_stg_vlan_add(bcm_unit, stg, vid);
        BCM_IF_ERROR_RETURN(BCMX_STG_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *      bcmx_stg_vlan_remove
 */

int
bcmx_stg_vlan_remove(bcm_stg_t stg,
                     bcm_vlan_t vid)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_stg_vlan_remove(bcm_unit, stg, vid);
        BCM_IF_ERROR_RETURN(BCMX_STG_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *      bcmx_stg_vlan_remove_all
 */

int
bcmx_stg_vlan_remove_all(bcm_stg_t stg)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_stg_vlan_remove_all(bcm_unit, stg);
        BCM_IF_ERROR_RETURN(BCMX_STG_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *      bcmx_stg_vlan_list
 */

int
bcmx_stg_vlan_list(bcm_stg_t stg,
                   bcm_vlan_t **list,
                   int *count)
{
    int rv, i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(list);
    BCMX_PARAM_NULL_CHECK(count);

    BCMX_LOCAL_UNIT_ITER(bcm_unit, i) {
        rv = bcm_stg_vlan_list(bcm_unit, stg, list, count);
        if (BCMX_STG_ERROR_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcmx_stg_vlan_list_destroy
 */

int
bcmx_stg_vlan_list_destroy(bcm_vlan_t *list,
                           int count)
{
    int rv, i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_PARAM_ARRAY_NULL_CHECK(count, list);

    BCMX_LOCAL_UNIT_ITER(bcm_unit, i) {
        rv = bcm_stg_vlan_list_destroy(bcm_unit, list, count);
        if (BCMX_STG_ERROR_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcmx_stg_create
 * Notes:
 *      Attempt to create the same STGID across all chips; the
 *      first time we succeed, we call "create_id".  Will
 *      return BCM_E_NONE if the stg id exists in some chips.
 */

int
bcmx_stg_create(bcm_stg_t *stg_ptr)
{
    int rv = BCM_E_UNAVAIL;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_stg_create(bcm_unit, stg_ptr);
        if (BCMX_STG_ERROR_IS_VALID(bcm_unit, rv)) {
            if (BCM_SUCCESS(rv)) {  /* Found an STG */
                rv = bcmx_stg_create_id(*stg_ptr);
            }
            break;
        }
    }

    return rv;
}


/*
 * Function:
 *      bcmx_stg_create_id
 * Notes:
 *      Returns BCM_E_NONE if the STG already exists.
 */

int
bcmx_stg_create_id(bcm_stg_t stg)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_stg_create_id(bcm_unit, stg);
        BCM_IF_ERROR_RETURN(BCMX_STG_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *      bcmx_stg_destroy
 */

int
bcmx_stg_destroy(bcm_stg_t stg)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_stg_destroy(bcm_unit, stg);
        BCM_IF_ERROR_RETURN(BCMX_STG_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *      bcmx_stg_list
 */

int
bcmx_stg_list(bcm_stg_t **list,
              int *count)
{
    int rv, i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(list);
    BCMX_PARAM_NULL_CHECK(count);

    BCMX_LOCAL_UNIT_ITER(bcm_unit, i) {
        rv = bcm_stg_list(bcm_unit, list, count);
        if (BCMX_STG_ERROR_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcmx_stg_list_destroy
 */

int
bcmx_stg_list_destroy(bcm_stg_t *list,
                      int count)
{
    int rv, i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_PARAM_ARRAY_NULL_CHECK(count, list);

    BCMX_LOCAL_UNIT_ITER(bcm_unit, i) {
        rv = bcm_stg_list_destroy(bcm_unit, list, count);
        if (BCMX_STG_ERROR_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcmx_stg_stp_set
 */

int
bcmx_stg_stp_set(bcm_stg_t stg,
                 bcmx_lport_t port,
                 int stp_state)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_STG_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_stg_stp_set(bcm_unit,
                           stg,
                           bcm_port,
                           stp_state);
}


/*
 * Function:
 *      bcmx_stg_stp_get
 */

int
bcmx_stg_stp_get(bcm_stg_t stg,
                 bcmx_lport_t port,
                 int *stp_state)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_STG_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_stg_stp_get(bcm_unit,
                           stg,
                           bcm_port,
                           stp_state);
}


/*
 * Function:
 *      bcmx_stg_count_get
 */

int
bcmx_stg_count_get(int *max_stg)
{
    int rv, i, bcm_unit;

    BCMX_STG_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(max_stg);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_stg_count_get(bcm_unit, max_stg);
        if (BCMX_STG_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}
