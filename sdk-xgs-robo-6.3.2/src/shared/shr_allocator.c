/*
 * $Id: shr_allocator.c 1.5 Broadcom SDK $
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
 * Global resource allocator
 */

#include <sal/core/sync.h>
#include <sal/core/libc.h>
#include <shared/shr_allocator.h>

#include <bcm/error.h>
#include <bcm/debug.h>

#define ALCR_DEBUG(flags, stuff)  BCM_DEBUG(flags | BCM_DBG_API, stuff)
#define ALCR_OUT(stuff)           ALCR_DEBUG(BCM_DBG_API, stuff)
#define ALCR_WARN(stuff)          ALCR_DEBUG(BCM_DBG_WARN, stuff)
#define ALCR_ERR(stuff)           ALCR_DEBUG(BCM_DBG_ERR, stuff)
#define ALCR_VERB(stuff)          ALCR_DEBUG(BCM_DBG_VERBOSE, stuff)
#define ALCR_VVERB(stuff)         ALCR_DEBUG(BCM_DBG_VVERBOSE, stuff)

/*
 * one stucture per unit
 */
_shr_res_handle_t _g_shr_res_handles[BCM_LOCAL_UNITS_MAX][SHR_USR_RES_MAX];

_shr_hw_res_t    *_g_mapped_hw_res[BCM_LOCAL_UNITS_MAX];

_shr_hw_res_attrs_t *_g_shr_res_attrs[BCM_LOCAL_UNITS_MAX];

/*
 * LOCK related defines
 */
sal_mutex_t  _shr_resource_mlock[BCM_MAX_NUM_UNITS];

#define _SHR_RESOURCE_LOCK_CREATED_ALREADY(unit) \
    (_shr_resource_mlock[(unit)] != NULL)

#define _SHR_RESOURCE_LOCK(unit)                    \
    (_SHR_RESOURCE_LOCK_CREATED_ALREADY(unit) ?      \
     sal_mutex_take(_shr_resource_mlock[(unit)], sal_mutex_FOREVER)?BCM_E_UNIT:BCM_E_NONE: \
     BCM_E_UNIT)

#define _SHR_RESOURCE_UNLOCK(unit)          \
    sal_mutex_give(_shr_resource_mlock[(unit)])

/* A common debug message prefix */
#define _SHR_D(_u, string)   "[%d:%s]: " string, _u, FUNCTION_NAME()

/*
 *   Function
 *      _shr_get_resource_handle
 *   Purpose
 *      Get the internal resource handle based on unit and type.
 *   Parameters
 *      unit    - (IN)  unit number of the device
 *      type    - (IN)  resource type
 *                      (one of _shr_usr_res_types_t)
 *      handle  - (OUT) resource allocation handle
 *   Returns
 *     BCM_E_NONE   if got the handle
 *     BCM_E_*      as appropriate otherwise
 */

STATIC int
_shr_get_resource_handle(int                        unit,
                         _shr_usr_res_types_t       type,
                         _shr_res_handle_t          *handle)
{
    _shr_hw_res_t         hw_res;
    _shr_res_handle_t     *res_handle;

    if ((type >= SHR_USR_RES_MAX) || (handle == NULL)) {
        return BCM_E_PARAM;
    }
    hw_res = _g_mapped_hw_res[unit][type];

    res_handle = &_g_shr_res_handles[unit][hw_res];
    if (res_handle->alloc_style == SHR_ALLOC_STYLE_NONE) {
        return BCM_E_PARAM;
    }

    *handle = *res_handle;
    return BCM_E_NONE;
}

/*
 *   Function
 *      _shr_resource_alloc
 *   Purpose
 *      Allocate "count" number of resources.
 *   Parameters
 *      unit    - (IN)  unit number of the device
 *      type    - (IN)  resource type
 *                      (one of _shr_usr_res_types_t)
 *      count   - (IN)  Number of resources required
 *      elements- (OUT) Resource Index return by the underlying allocator
 *      flags   - (IN)  _SHR_RES_FLAGS_CONTIGOUS
 *                      if the resources need to be contigous
 *   Returns
 *      BCM_E_NONE if element allocated successfully
 *      BCM_E_RESOURCE if resource is in reserved range and compliant, not 
 *                     neccessarily an error.
 *      BCM_E_* as appropriate otherwise
 */
int
_shr_resource_alloc(int                        unit,
                        _shr_usr_res_types_t   type,
                        uint32                     count,
                        uint32                    *elements,
                        uint32                     flags)
{
    _shr_res_handle_t    handle;
    _shr_hw_res_attrs_t *res_attrs;
    int                       status, res;
    uint32                    i, size;
    uint32                   *done;
    shr_aidxres_element_t     tmp_elem, first, last;    

    if ((count <= 0) || (elements == NULL)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_SHR_RESOURCE_LOCK(unit));

    status = _shr_get_resource_handle(unit,
                                          type,
                                          &handle);
    if (status != BCM_E_NONE) {
        _SHR_RESOURCE_UNLOCK(unit);
        return status;
    }

    res_attrs =  &_g_shr_res_attrs[unit][_g_mapped_hw_res[unit][type]];

    /*
     * if the reserve flag is set, make sure that the elements are
     * available
     */
    if (flags & _SHR_RES_FLAGS_RESERVE) {
        int checkVal = BCM_E_NOT_FOUND;
        int inRange = 0;
        
        if (res_attrs->reservedHigh && res_attrs->reservedLow) {

            for (i = 0; i < count; i++) {
                if (elements[i] >= res_attrs->reservedLow && 
                    elements[i] <= res_attrs->reservedHigh) {
                    inRange++;
                }
            }

            ALCR_VERB((_SHR_D(unit, "Found a reserved range on resource %d: "
                              "0x%08x-0x%08x count=%d inRange=%d\n"), 
                       type, res_attrs->reservedLow, res_attrs->reservedHigh,
                       count, inRange));
            
            if (inRange && inRange != count) {
                /* part some elements are in reserved range, but not all */
                _SHR_RESOURCE_UNLOCK(unit);
                return BCM_E_PARAM;
            }

            /* If these elements are in the reserved range, 
             * verify they exist 
             */
            if (inRange) {
                checkVal = BCM_E_EXISTS;
            }
        }

        switch (handle.alloc_style) {
            case SHR_ALLOC_STYLE_SINGLE:
            case SHR_ALLOC_STYLE_SCALED:
                for (i = 0; i < count; i++) {

                    /* check and apply translator */
                    tmp_elem = elements[i];
                    if (res_attrs->e2i) {
                        tmp_elem = res_attrs->e2i(unit, elements[i]);
                    }

                    res = shr_idxres_list_elem_state(handle.idx_handle, 
                                                     tmp_elem);
                    if (res != checkVal) {
                        status = BCM_E_BUSY;
                        break;
                    }
                    
                }
            break;

            case  SHR_ALLOC_STYLE_VERSATILE:
                for (i = 0; i < count; i++) {
                    if (flags & _SHR_RES_FLAGS_CONTIGOUS) {
                        tmp_elem = (shr_aidxres_element_t) (elements[0] + i);
                    } else {
                        tmp_elem = (shr_aidxres_element_t)elements[i];
                    }
                    /* check and apply translator */
                    if (res_attrs->e2i) {
                        tmp_elem = res_attrs->e2i(unit, elements[i]);
                    }

                    res = shr_aidxres_list_elem_state(handle.aidx_handle,
                                                      tmp_elem);
                    if (res != checkVal) {
                        status = BCM_E_BUSY;
                        break;
                    }
                }

            break;
        }

        /* may not be a real error; informing the caller that the resource
         * is in the reserved range, and checks out as OK.
         */ 
        if (inRange && BCM_SUCCESS(status)) {
            status = BCM_E_RESOURCE;
        }
    }

    if (status != BCM_E_NONE) {
        _SHR_RESOURCE_UNLOCK(unit);
        return status;
    }

    done = NULL;
    if ( (count > 1) &&
         (!(flags & ((_SHR_RES_FLAGS_CONTIGOUS) |
                     (_SHR_RES_FLAGS_RESERVE))))) {
        size = (sizeof(uint32) * count);
        done = (uint32 *)sal_alloc(size, "RES-LIB");
        if (done == NULL) {
            _SHR_RESOURCE_UNLOCK(unit);
            return BCM_E_MEMORY;
        }
        sal_memset(done, 0, size);
    }

    if ((flags & _SHR_RES_FLAGS_RESERVE) && (res_attrs->e2i)) {
        for (i = 0; i < count; i++) {
            elements[i] = res_attrs->e2i(unit, elements[i]);
        }
    }

    switch (handle.alloc_style) {
        case SHR_ALLOC_STYLE_SINGLE:
            if (count > 1) {
                status = BCM_E_PARAM;
            } else {
                if (flags & _SHR_RES_FLAGS_RESERVE) {
                    first  = (shr_aidxres_element_t)elements[0];
                    last   = (shr_aidxres_element_t)elements[0];
                    status = shr_idxres_list_reserve(handle.idx_handle,
                                                     first,
                                                     last);
                } else {
                    status = shr_idxres_list_alloc(handle.idx_handle,
                                                   (shr_aidxres_element_t *)elements);
                }
            }
        break;

        case  SHR_ALLOC_STYLE_SCALED:
            if (flags & _SHR_RES_FLAGS_CONTIGOUS) {
                status = BCM_E_PARAM;
            } else {
                if (flags & _SHR_RES_FLAGS_RESERVE) {
                    for (i = 0; i < count; i++) {
                        first  = (shr_aidxres_element_t)elements[i];
                        last   = (shr_aidxres_element_t)elements[i];
                        shr_idxres_list_reserve(handle.idx_handle,
                                                first,
                                                last);
                    }
                } else {
                    if (count == 1) {
                        status = shr_idxres_list_alloc(handle.idx_handle,
                                                       (shr_aidxres_element_t *)elements);
                    } else {
                        status = shr_idxres_list_alloc_set(handle.idx_handle,
                                                           (shr_idxres_element_t)   count,
                                                           (shr_idxres_element_t *) elements,
                                                           (shr_idxres_element_t *) done);
                    }
                }
            }
        break;

        case SHR_ALLOC_STYLE_VERSATILE:
            if (flags & _SHR_RES_FLAGS_RESERVE) {
                if ((count == 1) || ((flags & _SHR_RES_FLAGS_CONTIGOUS))) {
                    first  = (shr_aidxres_element_t)elements[0];
                    last   = (shr_aidxres_element_t)elements[0] + count - 1;
                    status = shr_aidxres_list_reserve(handle.aidx_handle,
                                                      first,
                                                      last);
                } else {
                    for (i = 0; i < count; i++) {
                        first  = (shr_aidxres_element_t)elements[i];
                        last   = (shr_aidxres_element_t)elements[i];
                        status = shr_aidxres_list_reserve(handle.aidx_handle,
                                                          first,
                                                          last);
                    }
                }
            } else {
                if (count == 1) {
                    status = shr_aidxres_list_alloc(handle.aidx_handle,
                                                    (shr_aidxres_element_t *) elements);
                } else {
                    if (!(flags & _SHR_RES_FLAGS_CONTIGOUS)) {
                        status = shr_aidxres_list_alloc_set(handle.aidx_handle,
                                                            (shr_aidxres_element_t)   count,
                                                            (shr_aidxres_element_t *) elements,
                                                            (shr_aidxres_element_t *) done);
                    } else {
                        shr_aidxres_list_alloc_block(handle.aidx_handle,
                                                     (shr_aidxres_element_t)  count,
                                                     (shr_aidxres_element_t *)elements);
                    }
                }
            }

        break;
    }

    if (done != NULL) {
        if (status != BCM_E_NONE) {
            for (i = 0; i < count; i++) {
                if (done[i]) {
                    if (handle.alloc_style == SHR_ALLOC_STYLE_VERSATILE) {
                        shr_aidxres_list_free(handle.aidx_handle,
                                              done[i]);
                    } else {
                         shr_idxres_list_free(handle.idx_handle,
                                              done[i]);
                    }
                }
            }
        }
        sal_free(done);
    }

    if (status != BCM_E_NONE) {
        _SHR_RESOURCE_UNLOCK(unit);
        return status;
    }

    /* check and apply translators */
    if (res_attrs->i2e) {
        for (i = 0; i < count; i++) {
            elements[i] = res_attrs->i2e(unit, elements[i]);
        }
    }

    _SHR_RESOURCE_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 *   Function
 *      _shr_resource_free
 *   Purpose
 *      Free the resources specified in the array
 *   Parameters
 *      unit    - (IN)  unit number of the device
 *      type    - (IN)  resource type
 *                      (one of _shr_usr_res_types_t)
 *      count   - (IN)  Number of resources to be freed
 *      elements- (IN)  Resource Ids to be freed
 *   Returns
 *      BCM_E_NONE if element freed successfully
 *      BCM_E_* as appropriate otherwise
 */

int
_shr_resource_free(int                        unit,
                       _shr_usr_res_types_t   type,
                       uint32                     count,
                       uint32                    *elements,
                       uint32                     flags)
{
    _shr_res_handle_t     handle;
    int                       status;
    uint32                    i;
    _shr_hw_res_attrs_t *res_attrs;

    if ((count <= 0) || (elements == NULL)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_SHR_RESOURCE_LOCK(unit));

    status = _shr_get_resource_handle(unit,
                                          type,
                                          &handle);

    if (status != BCM_E_NONE) {
        _SHR_RESOURCE_UNLOCK(unit);
        return status;
    }

    res_attrs =  &_g_shr_res_attrs[unit][_g_mapped_hw_res[unit][type]];
    
    if (res_attrs->reservedHigh && res_attrs->reservedLow) {
        int inRange = 0;

        for (i = 0; i < count; i++) {
            if (elements[i] >= res_attrs->reservedLow && 
                elements[i] <= res_attrs->reservedHigh) {
                inRange++;
            }
        }

        ALCR_VERB((_SHR_D(unit, "Found a reserved range on resource %d: "
                          "0x%08x-0x%08x count=%d inRange=%d\n"), 
                   type, res_attrs->reservedLow, res_attrs->reservedHigh,
                   count, inRange));

        if (inRange && (inRange != count)) {
            /* part some elements are in reserved range, but not all */
            _SHR_RESOURCE_UNLOCK(unit);
            return BCM_E_PARAM;
        }

        /* nothing to do */
        if (inRange) {
            _SHR_RESOURCE_UNLOCK(unit);
            return BCM_E_NONE;
        }
    }

    /* check and apply translators */
    if (res_attrs->e2i) {
        for (i = 0; i < count; i++) {
            elements[i] = res_attrs->e2i(unit, elements[i]);
        }
    }

    if ((handle.alloc_style == SHR_ALLOC_STYLE_VERSATILE) &&
        (flags & _SHR_RES_FLAGS_CONTIGOUS)) {
        status =  shr_aidxres_list_free(handle.aidx_handle,
                                        (shr_aidxres_element_t)elements[0]);
    } else {
        for (i = 0; i < count; i++) {
            if (handle.alloc_style == SHR_ALLOC_STYLE_VERSATILE) {
                status = shr_aidxres_list_free(handle.aidx_handle,
                                               (shr_aidxres_element_t)elements[i]);
            } else {
                status =  shr_idxres_list_free(handle.idx_handle,
                                               (shr_idxres_element_t)elements[i]);
            }
        }
    }

    _SHR_RESOURCE_UNLOCK(unit);
    return status;
}

/*
 *   Function
 *      _shr_resource_test
 *   Purpose
 *      Check to see if the specified resource is available
 *   Parameters
 *      unit    - (IN)  unit number of the device
 *      type    - (IN)  resource type
 *                      (one of _shr_usr_res_types_t)
 *      element - (IN)  Resource Id to be checked
 *   Returns
 *      BCM_E_NOT_FOUND if resource is available
 *      BCM_E_EXIST     if resource is in use
 *      BCM_E_*         as appropriate otherwise
 */

int
_shr_resource_test(int                        unit,
                       _shr_usr_res_types_t   type,
                       uint32                     element)
{
    _shr_res_handle_t     handle;
    int                       status;
    _shr_hw_res_attrs_t  *res_attrs;

    BCM_IF_ERROR_RETURN(_SHR_RESOURCE_LOCK(unit));

    status = _shr_get_resource_handle(unit,
                                          type,
                                          &handle);
    if (status != BCM_E_NONE) {
        _SHR_RESOURCE_UNLOCK(unit);
        return status;
    }

    res_attrs =  &_g_shr_res_attrs[unit][_g_mapped_hw_res[unit][type]];

    /* check and apply translators */
    if (res_attrs->e2i) {
        element = res_attrs->e2i(unit, element);
    }

    if (handle.alloc_style == SHR_ALLOC_STYLE_VERSATILE) {
        status = shr_aidxres_list_elem_state(handle.aidx_handle,
                                             (shr_aidxres_element_t)element);
    } else {
        status = shr_idxres_list_elem_state(handle.idx_handle,
                                            (shr_idxres_element_t)element);
    }

    _SHR_RESOURCE_UNLOCK(unit);
    return status;
}

/*
 *   Function
 *      _shr_shr_resource_uninit
 *   Purpose
 *      Destroy the all the resource managers for this unit
 *   Parameters
 *      (IN) unit   : unit number of the device
 *   Returns
 *       BCM_E_NONE all resources are successfully destroyed
 *       BCM_E_* as appropriate otherwise
 *   Notes
 *       Returns error is any of the resources cannot be destroyed
 */

int
_shr_resource_uninit(int unit)
{
    uint32                     i;
    int                        status, ret;
    _shr_res_handle_t     *res_handle;


    BCM_IF_ERROR_RETURN(_SHR_RESOURCE_LOCK(unit));
    ret = BCM_E_NONE;

    for (i = SHR_HW_RES_RES0;
         i < SHR_HW_RES_MAX; i++) {
         res_handle         = &_g_shr_res_handles[unit][i];
         if (res_handle->alloc_style == SHR_ALLOC_STYLE_VERSATILE) {
             status = shr_aidxres_list_destroy(res_handle->aidx_handle);
         } else {
             status = shr_idxres_list_destroy(res_handle->idx_handle);
         }
         if (status != BCM_E_NONE) {
             ret = status;
         }
    }
    

    _SHR_RESOURCE_UNLOCK(unit);
    if (_shr_resource_mlock[unit] != NULL) {
        sal_mutex_destroy(_shr_resource_mlock[unit]);
        _shr_resource_mlock[unit] = NULL;
    }

    /**
     * No use send error here.. we have destroyed the lock,
     * _init() needs to be done
     */
    if (ret != BCM_E_NONE) {
        ret = BCM_E_NONE;
    }
    return ret;
}

/*
 *   Function
 *      _shr_alloc_range_get
 *   Purpose
 *     Retrieve the range of valid IDs for the given resource
 *
 *   Parameters
 *      (IN)  unit   : unit number of the device
 *      (IN)  type   : resource to get
 *      (OUT) first  : first valid ID
 *      (OUT) last   : last valid ID
 *   Returns
 *       BCM_E_NONE - All required resources are allocated
 *       BCM_E_*    - failure
 *   Notes
 */
int _shr_alloc_range_get(int unit, _shr_usr_res_types_t type,
                         uint32 *first, uint32 *last)
{
    _shr_res_handle_t      *res_handle;
    _shr_hw_res_attrs_t    *res_attrs;
    _shr_hw_res_t           res;
    int rv = BCM_E_NONE;
    uint32 validLow, validHigh, freeCount, allocCount, junk;

    if (type >= SHR_USR_RES_MAX) {
        return BCM_E_PARAM;
    }

    res = _g_mapped_hw_res[unit][type];

    BCM_IF_ERROR_RETURN(_SHR_RESOURCE_LOCK(unit));

    res_handle = &_g_shr_res_handles[unit][res];
    res_attrs  =  &_g_shr_res_attrs[unit][res];

    if (res_attrs->alloc_style == SHR_ALLOC_STYLE_VERSATILE) {
        rv = shr_aidxres_list_state(res_handle->u.handle_aidx,
                                    first, last, 
                                    &validLow, &validHigh, 
                                    &freeCount, &allocCount, &junk, &junk);
        
    } else if (res_attrs->alloc_style == SHR_ALLOC_STYLE_SCALED) {
        rv = shr_idxres_list_state_scaled(res_handle->u.handle_idx,
                                          first, last,
                                          &validLow, &validHigh, 
                                          &freeCount, &allocCount, &junk);
    } else { 
        rv = shr_idxres_list_state(res_handle->u.handle_idx,
                                   first, last, 
                                   &validLow, &validHigh, 
                                   &freeCount, &allocCount);
        
    }

    _SHR_RESOURCE_UNLOCK(unit);
    return rv;
}


/*
 *   Function
 *      _shr_range_reserve
 *   Purpose
 *     Reserve a range of IDs of a given resource
 *
 *   Parameters
 *      (IN)  unit   : unit number of the device
 *      (IN) type    : resource to set
 *      (IN) highOrLow  : TRUE - set Upper bounds
 *                      : FALSE - set lower bounds
 *      (IN) val    : inclusive bound to set
 *   Returns
 *       BCM_E_NONE - All required resources are allocated
 *       BCM_E_*    - failure
 *   Notes
 */
int _shr_range_reserve(int unit, _shr_usr_res_types_t type,
                       int highOrLow, uint32 val)
{
    _shr_res_handle_t      *res_handle;
    _shr_hw_res_attrs_t    *res_attrs;
    _shr_hw_res_t           res;
    int rv = BCM_E_NONE;
    int clearIt = 0, reserveIt = 0;
    uint32 first, last;

    if (type >=  SHR_USR_RES_MAX) {
        return BCM_E_PARAM;
    }

    res = _g_mapped_hw_res[unit][type];
    
    BCM_IF_ERROR_RETURN(_SHR_RESOURCE_LOCK(unit));

    res_handle = &_g_shr_res_handles[unit][res];
    res_attrs  =  &_g_shr_res_attrs[unit][res];

    first = res_attrs->reservedLow;
    last = res_attrs->reservedHigh;

    /* Zero for any value, high or low, will clear the known range */
    if (val == 0) {
        clearIt = 1;
        res_attrs->reservedLow = 0;
        res_attrs->reservedHigh = 0;
    } else {
        if (highOrLow) {
            res_attrs->reservedHigh = val; 
        } else {
            res_attrs->reservedLow = val;
        }

        if (res_attrs->reservedHigh && res_attrs->reservedLow) {
            if (res_attrs->reservedHigh < res_attrs->reservedLow) {
                ALCR_ERR((_SHR_D(unit, "Upper bounds is set less than lower"
                             " bounds: 0x%x < 0x%x\n"),
                          res_attrs->reservedHigh, res_attrs->reservedLow));

                rv = BCM_E_PARAM;
            } else {
                reserveIt = 1;
            }
        }
    }

    if (reserveIt) {
        
        first = res_attrs->reservedLow;
        last = res_attrs->reservedHigh;

        if (res_attrs->alloc_style == SHR_ALLOC_STYLE_VERSATILE) {
            rv = shr_aidxres_list_reserve(res_handle->u.handle_aidx, 
                                          first, last);
        } else if (res_attrs->alloc_style == SHR_ALLOC_STYLE_SCALED) {
            rv = shr_idxres_list_reserve(res_handle->u.handle_idx, 
                                         first, last);
        } else { 
            rv = shr_idxres_list_reserve(res_handle->u.handle_idx,
                                         first, last);
        }
       /* 
        ALCR_VERB((_SHR_D(unit, "Reserved resource %s (%d/%d) : "
                                "0x%08x-0x%08x rv=%d %s\n"),
                   _shr_resource_to_str(type), type, res, 
                   first, last, rv, bcm_errmsg(rv)));
*/
    } else if (clearIt) {

        if (first && last) {
            int elt, ignoreRv;

            for (elt = first; elt <= last; elt++) {

                if (res_attrs->alloc_style == SHR_ALLOC_STYLE_VERSATILE) {
                    ignoreRv = 
                        shr_aidxres_list_free(res_handle->u.handle_aidx, elt);
                    
                } else if (res_attrs->alloc_style == SHR_ALLOC_STYLE_SCALED) {
                    ignoreRv =
                        shr_idxres_list_free(res_handle->u.handle_idx, elt);
                } else { 
                    ignoreRv = shr_idxres_list_free(res_handle->u.handle_idx, 
                                                    elt);
                }

                if (BCM_FAILURE(ignoreRv)) {
                    ALCR_VERB((_SHR_D(unit, "failed to free element "
                                      "0x%08x  rv=%d %s (ignored)\n"),
                               elt, ignoreRv, bcm_errmsg(ignoreRv)));
                }
            } 
/*
            ALCR_VERB((_SHR_D(unit, "Freed reserved resource %s (%d/%d) ids: "
                              "0x%08x-0x%08x\n"),
                       _shr_resource_to_str(type), type, res, 
                       first, last)); */
        }
    }
    
    _SHR_RESOURCE_UNLOCK(unit);
    return rv;
}
