/* 
 * $Id: niv.c,v 1.2 Broadcom SDK $
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
 * File:        bcmx/niv.c
 * Purpose:     BCMX NIV APIs
 *
 */

#ifdef INCLUDE_L3

#include <bcm/types.h>

#include <bcmx/lport.h>
#include <bcmx/niv.h>
#include "bcmx_int.h"

#define BCMX_NIV_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_NIV_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_NIV_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_NIV_DELETE_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_DELETE_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_NIV_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)


#define BCMX_NIV_PORT_T_PTR_TO_BCM(_port)    \
    ((bcm_niv_port_t *)(_port))

#define BCMX_NIV_FORWARD_T_PTR_TO_BCM(_info)    \
    ((bcm_niv_forward_t *)(_info))


/* Initialize the NIV port structure. */
void
bcmx_niv_port_t_init(bcmx_niv_port_t *niv_port)
{
    if (niv_port != NULL) {
        bcm_niv_port_t_init(BCMX_NIV_PORT_T_PTR_TO_BCM(niv_port));
    }
}

/* Initialize the NIV Forwarding Entry structure. */
void
bcmx_niv_forward_t_init(bcmx_niv_forward_t *iv_fwd_entry)
{
    if (iv_fwd_entry != NULL) {
        bcm_niv_forward_t_init(BCMX_NIV_FORWARD_T_PTR_TO_BCM(iv_fwd_entry));
    }
}

/*
 * Function:
 *     bcmx_niv_init
 * Purpose:
 *     Initialize NIV module.
 * Parameters:
 *     None
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_niv_init(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_init(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_NIV_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_niv_cleanup
 * Purpose:
 *     Detach NIV module, clear all HW states.
 * Parameters:
 *     None
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
int
bcmx_niv_cleanup(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_cleanup(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_NIV_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_niv_port_add
 * Purpose:
 *     Create NIV Port.
 * Parameters:
 *     niv_port - (IN/OUT) NIV port information
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     NIV port is a global entity, apply to all units.
 */
int
bcmx_niv_port_add(bcmx_niv_port_t *niv_port)
{
    int     rv = BCM_E_UNAVAIL, tmp_rv;
    int     i, bcm_unit;
    uint32  flags_orig;

    BCMX_NIV_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(niv_port);

    /* Store original 'xxx_WITH_ID' flag bit */
    flags_orig = niv_port->flags & BCM_NIV_PORT_WITH_ID;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_port_add(bcm_unit,
                                  BCMX_NIV_PORT_T_PTR_TO_BCM(niv_port));
        if (BCM_FAILURE(BCMX_NIV_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv))) {
            break;
        }

        /*
         * Use the ID from first successful 'create' if NIV port ID
         * is not specified.
         */
        if (!(niv_port->flags & BCM_NIV_PORT_WITH_ID)) {
            if (BCM_SUCCESS(tmp_rv)) {
                niv_port->flags |= BCM_NIV_PORT_WITH_ID;
            }
        }
    }

    /* Restore 'xxx_WITH_ID' flag bit */
    niv_port->flags &= ~BCM_NIV_PORT_WITH_ID;
    niv_port->flags |= flags_orig;

    return rv;
}

/*
 * Function:
 *     bcmx_niv_port_delete
 * Purpose:
 *     Delete a NIV port.
 * Parameters:
 *     niv_port_id - NIV port id to delete
 * Returns:
 *     BCM_E_XXX
 */
int
bcmx_niv_port_delete(bcm_gport_t niv_port_id)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_port_delete(bcm_unit, niv_port_id);
        BCM_IF_ERROR_RETURN
            (BCMX_NIV_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_niv_port_delete_all
 * Purpose:
 *     Delete all NIV ports.
 * Parameters:
 *     None
 * Returns:
 *     BCM_E_XXX
 */
int
bcmx_niv_port_delete_all(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_port_delete_all(bcm_unit);
        BCM_IF_ERROR_RETURN
            (BCMX_NIV_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_niv_port_get
 * Purpose:
 *     Get NIV port information for given ID.
 * Parameters:
 *     niv_port  - (IN/OUT) NIV port info
 * Returns:
 *     BCM_E_XXX
 */
int
bcmx_niv_port_get(bcmx_niv_port_t *niv_port)
{
    int  rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(niv_port);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_niv_port_get(bcm_unit, BCMX_NIV_PORT_T_PTR_TO_BCM(niv_port));
        if (BCMX_NIV_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *     bcmx_niv_forward_add
 * Purpose:
 *     Add NIV forwarding table entry.
 * Parameters:
 *     iv_fwd_entry - (IN) NIV Forward entry information
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     NIV Forward entry is a global entity, apply to all units.
 */
int
bcmx_niv_forward_add(bcmx_niv_forward_t *iv_fwd_entry)
{
    int     rv = BCM_E_UNAVAIL, tmp_rv;
    int     i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(iv_fwd_entry);


    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_forward_add(bcm_unit,
                                     BCMX_NIV_FORWARD_T_PTR_TO_BCM(iv_fwd_entry));
        BCM_IF_ERROR_RETURN
            (BCMX_NIV_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_niv_forward_delete
 * Purpose:
 *     Delete a NIV Forward entry.
 * Parameters:
 *     iv_fwd_entry - (IN) NIV forward entry id to delete
 * Returns:
 *     BCM_E_XXX
 */
int
bcmx_niv_forward_delete(bcmx_niv_forward_t *iv_fwd_entry)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_forward_delete(bcm_unit, iv_fwd_entry);
        BCM_IF_ERROR_RETURN
            (BCMX_NIV_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_niv_forward_delete_all
 * Purpose:
 *     Delete all NIV Forwarding table entries.
 * Parameters:
 *     None
 * Returns:
 *     BCM_E_XXX
 */
int
bcmx_niv_forward_delete_all(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_niv_forward_delete_all(bcm_unit);
        BCM_IF_ERROR_RETURN
            (BCMX_NIV_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_niv_forward_get
 * Purpose:
 *     Get NIV Forwarding entry information for given ID.
 * Parameters:
 *     iv_fwd_entry - (IN/OUT) NIV forwarding entry info
 * Returns:
 *     BCM_E_XXX
 */
int
bcmx_niv_forward_get(bcmx_niv_forward_t *iv_fwd_entry)
{
    int  rv;
    int  i, bcm_unit;

    BCMX_NIV_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(iv_fwd_entry);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_niv_forward_get(bcm_unit,
                                 BCMX_NIV_FORWARD_T_PTR_TO_BCM(iv_fwd_entry));
        if (BCMX_NIV_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}

#endif /* INCLUDE_L3 */
