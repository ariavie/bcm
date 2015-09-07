/* 
 * $Id: failover.c,v 1.6 Broadcom SDK $
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
 * File:        bcmx/failover.c
 * Purpose:     BCMX Forwarding Failover Protection APIs
 *
 */
#include <bcm/types.h>

#include <bcmx/failover.h>

#ifdef INCLUDE_L3

#include "bcmx_int.h"

#define BCMX_FAILOVER_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_FAILOVER_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_FAILOVER_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_FAILOVER_DELETE_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_DELETE_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_FAILOVER_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)


/*
 * Function:
 *     bcmx_failover_init
 */
int
bcmx_failover_init(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_FAILOVER_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_failover_init(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_FAILOVER_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *     bcmx_failover_cleanup
 */
int
bcmx_failover_cleanup(void)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_FAILOVER_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_failover_cleanup(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_FAILOVER_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *     bcmx_failover_create
 */
int
bcmx_failover_create(uint32 flags, bcm_failover_t *failover_id)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_FAILOVER_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(failover_id);

    BCMX_UNIT_ITER(bcm_unit, i) {

        tmp_rv = bcm_failover_create(bcm_unit, flags, failover_id);
        BCM_IF_ERROR_RETURN
            (BCMX_FAILOVER_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));

        /*
         * Use the ID from first successful 'create' if group ID
         * is not specified.
         */
        if (!(flags & BCM_FAILOVER_WITH_ID)) {
            if (BCM_SUCCESS(tmp_rv)) {
                flags |= BCM_FAILOVER_WITH_ID;
            }
        }
    }

    return rv;
}


/*
 * Function:
 *     bcmx_failover_destroy
 */
int
bcmx_failover_destroy(bcm_failover_t failover_id)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_FAILOVER_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_failover_destroy(bcm_unit, failover_id);
        BCM_IF_ERROR_RETURN
            (BCMX_FAILOVER_DELETE_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *     bcmx_failover_set
 */
int
bcmx_failover_set(bcm_failover_t failover_id, int enable)
{
    int  rv = BCM_E_UNAVAIL, tmp_rv;
    int  i, bcm_unit;

    BCMX_FAILOVER_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_failover_set(bcm_unit, failover_id, enable);
        BCM_IF_ERROR_RETURN
            (BCMX_FAILOVER_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *     bcmx_failover_get
 */
int
bcmx_failover_get(bcm_failover_t failover_id, int *enable)
{
    int  rv;
    int  i;
    int  bcm_unit;

    BCMX_FAILOVER_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(enable);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_failover_get(bcm_unit, failover_id, enable);
        if (BCMX_FAILOVER_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}

#else /* INCLUDE_L3 */
/*
 * Function:
 *     bcmx_failover_init
 */
int
bcmx_failover_init(void)
{
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *     bcmx_failover_cleanup
 */
int
bcmx_failover_cleanup(void)
{
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *     bcmx_failover_create
 */
int
bcmx_failover_create(uint32 flags, bcm_failover_t *failover_id)
{
    COMPILER_REFERENCE(flags);
    COMPILER_REFERENCE(failover_id);
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *     bcmx_failover_destroy
 */
int
bcmx_failover_destroy(bcm_failover_t failover_id)
{
    COMPILER_REFERENCE(failover_id);
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *     bcmx_failover_set
 */
int
bcmx_failover_set(bcm_failover_t failover_id, int enable)
{
    COMPILER_REFERENCE(failover_id);
    COMPILER_REFERENCE(enable);
    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *     bcmx_failover_get
 */
int
bcmx_failover_get(bcm_failover_t failover_id, int *enable)
{
    COMPILER_REFERENCE(failover_id);
    COMPILER_REFERENCE(enable);
    return BCM_E_UNAVAIL;
}
#endif /* INCLUDE_L3 */
