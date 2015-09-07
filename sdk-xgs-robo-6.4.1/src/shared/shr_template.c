/*
 * $Id: shr_template.c,v 1.21 Broadcom SDK $
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
 * Global template management
 */

#include <shared/bsl.h>

#include <sal/core/sync.h>
#include <shared/shr_template.h>
#include <shared/error.h>

#if defined(BCM_PETRA_SUPPORT)

#include <soc/cm.h>
#include <soc/dpp/SAND/Utils/sand_multi_set.h>
#include <bcm_int/dpp/error.h>

/*****************************************************************************/
/*
 *  Internal implementation
 */

#define SHR_TEMPLATE_MAX_BUFFER_SIZE_FOR_PRINT  2048
/*
 *  This describes how a single resource maps to an underlying pool.
 *
 *  templatePoolId is the ID of the tepmlate pool on this unit from which this
 *  template is drawn.
 *
 *  templateElemSize is the number of elements in the specified template pool that
 *  must be taken to make a single element of this resource.  Basically, any
 *  alloc/free of this resource will multiply the number of elements by this
 *  value to determine how many to alloc/free of the underlying pool.
 *
 *  name is a string that names this resource.  It is used only for diagnostic
 *  purposes.  Internally, the provided name will be copied to the same cell as
 *  the structure, so the name array is really variable length.
 */
typedef struct _shr_template_type_desc_s {
    int templatePoolId;              /* unit specific template pool ID */
    int templateElemSize;            /* how many elems of this pool per this template type */
    int refCount;               /* number of elements allocated currently */
    char name[1];               /* descriptive name (for diagnostics) */
} _shr_template_type_desc_t;

/* A handle for a non-unit-based template manager instance */
typedef struct _shr_template_unit_desc_s *shr_mtemplate_handle_t;

/*
 *  This describes a single resource pool on a unit.
 *
 *  templateManagerType is the ID of the reousrce manager that will be used to
 *  manage this resource pool on this unit.
 *
 *  low is the minimum valid element of this resource.
 *
 *  count is the number of valid elements of this resource.
 *
 *  extras points to a struct (which will be appended to this during setup of
 *  this pool) that provides additional arguments to how the pool needs to be
 *  managed.  This is provided because some of the supported allocation
 *  managers require more information than just the range of valid IDs.
 *
 *  name is a string that names this resource.  It is used only for diagnostic
 *  purposes.  Internally, the provided name will be copied to the same cell as
 *  the structure, so the name array is really variable length.
 *
 *  Note that the extras struct will be appended to the same memory cell as
 *  this struct, after the name, and so the pointer will not need to be freed
 *  when this is destroyed.
 */
typedef struct _shr_template_pool_desc_s {
    shr_template_manage_t templateManagerType; /* which resoource manager to use */
    int template_low_id;                /* start template id */
    int template_count;                 /* Number of templates*/
    int max_entities;                 /* Number of Entities */
    size_t data_size;                   /* Data size */
    int refCount;                       /* number of types using this pool */
    void *templateHandle;               /* handle for this resource */
    void *extras;                       /* additional config per resmgr type */
    char *name;                       /* descriptive name for diagnostics */
} _shr_template_pool_desc_t;
                         
/*
 *  This structure describes attributes about the unit, and includes lists
 *  that are applicable to the unit.  Note that while resource IDs and resource
 *  types are unit-specific, the resource managers themselves are globally
 *  available to all units.
 *
 *  resTypeCount indicates how many different resources on this unit are being
 *  managed through this system.  It is possible for more than one resource to
 *  use the same resource pool, but it is not possible for one resource to use
 *  more than one resource pool.
 *
 *  resPoolCount indicates how many different resource pools are being managed
 *  on this unit.
 *
 *  res points to an array of resource descriptors,  Each descriptor will map a
 *  single resource on the unit to its underlying resource pool.
 *
 *  pool points to an array of resource pool descriptors.  Each of these will
 *  describe a single resource pool on the unit and map to the manager that
 *  will be used for that particular pool.
 */
typedef struct _shr_template_unit_desc_s {
    uint16 templateTypeCount;          /* maximum presented resource ID */
    uint16 templatePoolCount;          /* maximum resource pool ID */
    _shr_template_type_desc_t **template;   /* array of type -> pool map pointers */
    _shr_template_pool_desc_t **pool;  /* array of pool description pointers */
} _shr_template_unit_desc_t;


/*
 *  This structure is global, and points to the information about all units.
 *
 *  For each unit, it's just a pointer here, since this reduces the overall
 *  memory footprint for the case of units that do not use this mechanism, plus
 *  it allows each unit to use only as much memory as needed to describe its
 *  resources and pools and how they map.
 */
static _shr_template_unit_desc_t *_g_unitTemplateDesc[BCM_LOCAL_UNITS_MAX] = { NULL };

/*
 *  Various function prototypes per method for the alloc managers.
 */
typedef int (*_shr_template_manage_create)(_shr_template_pool_desc_t **desc,
                                     int template_low_id,
                                     int template_count,
                                     int max_entities,
                                     uint32 global_max,
                                     size_t data_size,
                                     const void *extras,
                                     const char *name);
typedef int (*_shr_template_manage_destroy)(_shr_template_pool_desc_t *desc);
typedef int (*_shr_template_manage_alloc)(_shr_template_pool_desc_t *desc,
                                    uint32 flags,
                                    const void* data,
                                    int *is_allocated,
                                    int *template);
typedef int (*_shr_template_manage_alloc_group)(_shr_template_pool_desc_t *desc,
                                    uint32 flags,
                                    const void* data,
                                    int nof_additions,
                                    int *is_allocated,
                                    int *template);
typedef int (*_shr_template_manage_exchange)(_shr_template_pool_desc_t *desc,
                                    uint32 flags,
                                    const void *data,
                                    int old_template,
                                    int *is_last,
                                    int *template,
                                    int *is_allocated); 
typedef int (*_shr_template_manage_exchange_test)(_shr_template_pool_desc_t *desc,
                                    uint32 flags,
                                    const void *data,
                                    int old_template,
                                    int *is_last,
                                    int *template,
                                    int *is_allocated);                       
typedef int (*_shr_template_manage_free)(_shr_template_pool_desc_t *desc,                                  
                                    int template,
                                    int *is_free);
typedef int (*_shr_template_manage_free_group)(_shr_template_pool_desc_t *desc,                                  
                                    int template,
                                    int nof_deductions,
                                    int *is_free);
typedef int (*_shr_template_manage_data_get)(_shr_template_pool_desc_t *desc,
                                    int template,
                                    void *data);
typedef int (*_shr_template_manage_index_get)(_shr_template_pool_desc_t *desc,
                                    const void *data, 
                                    int *template);
typedef int (*_shr_template_manage_ref_count_get)(_shr_template_pool_desc_t *desc,
                                    int template,
                                    uint32 *ref_count);
typedef int (*_shr_template_manage_clear)(_shr_template_pool_desc_t *desc);

/*
 *  This structure describes a single allocator mechanism, specifically by
 *  providing a set pointers to functions that are used to manipulate it.
 */
typedef struct _shr_template_managements_s {
    _shr_template_manage_create create;
    _shr_template_manage_destroy destroy;
    _shr_template_manage_alloc alloc;
    _shr_template_manage_alloc_group alloc_group;
    _shr_template_manage_exchange exchange;
    _shr_template_manage_exchange_test exchange_test;
    _shr_template_manage_free free;
    _shr_template_manage_free_group free_group;
    _shr_template_manage_data_get data_get;
    _shr_template_manage_ref_count_get ref_count_get;
    _shr_template_manage_clear clear;
    _shr_template_manage_index_get index_get;
    char *name;
} _shr_template_managements_t;

/*
 *  These prototypes are for the global const structure below that points to
 *  all of the various implementations.
 */
static int _shr_template_hash_create(_shr_template_pool_desc_t **desc,
                                     int template_low_id,
                                     int template_count,
                                     int max_entities,
                                     uint32 global_max,
                                     size_t data_size,
                                     const void *extras,
                                     const char *name);
static int _shr_template_hash_destroy(_shr_template_pool_desc_t *desc);
static int _shr_template_hash_alloc(_shr_template_pool_desc_t *desc,
                                 uint32 flags,
                                 const void* data,
                                 int *is_allocated,
                                 int *template);
static int _shr_template_hash_alloc_group(_shr_template_pool_desc_t *desc,
                                 uint32 flags,
                                 const void* data,
                                 int nof_additions,
                                 int *is_allocated,
                                 int *template);
static int _shr_template_hash_free(_shr_template_pool_desc_t *desc,
                                int template,
                                int *is_last);
static int _shr_template_hash_free_group(_shr_template_pool_desc_t *desc,
                                int template,
                                int nof_deductions,
                                int *is_last);
static int _shr_template_hash_exchange(_shr_template_pool_desc_t *desc,
                                uint32 flags,
                                const void *data,
                                int old_template,
                                int *is_last,
                                int *template,
                                int *is_allocated);
static int _shr_template_hash_exchange_test(_shr_template_pool_desc_t *desc,
                                uint32 flags,
                                const void *data,
                                int old_template,
                                int *is_last,
                                int *template,
                                int *is_allocated);
static int _shr_template_hash_data_get(_shr_template_pool_desc_t *desc,
                                int template,
                                void *data);
static int _shr_template_hash_ref_count_get(_shr_template_pool_desc_t *desc,
                                int template,
                                uint32 *ref_count);
static int _shr_template_hash_clear(_shr_template_pool_desc_t *desc);
static int _shr_template_hash_index_get(_shr_template_pool_desc_t *desc,
                                const void *data,
                                int* template);
/*
 *  Global const structure describing the various allocator mechanisms.
 */
static const _shr_template_managements_t _shr_template_managements_mgrs[SHR_TEMPLATE_MANAGE_COUNT] =
    {
        {
            _shr_template_hash_create,
            _shr_template_hash_destroy,
            _shr_template_hash_alloc,
            _shr_template_hash_alloc_group,
            _shr_template_hash_exchange,
            _shr_template_hash_exchange_test,
            _shr_template_hash_free,
            _shr_template_hash_free_group,
            _shr_template_hash_data_get,
            _shr_template_hash_ref_count_get,
            _shr_template_hash_clear,
            _shr_template_hash_index_get,
            "SHR_TEMPLATE_MANAGE_HASH"
        } /* hash */
    };

/*
 *  Basic checks performed for many functions
 */
#define TEMPLATE_UNIT_CHECK(_unit, _unitInfo) \
    if ((0 > (_unit)) || (BCM_LOCAL_UNITS_MAX <= (_unit))) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
        (BSL_META_U(_unit, \
                    "invalid unit number %d\n"), \
                   _unit)); \
        return BCM_E_PARAM; \
    } \
    if (!(_g_unitTemplateDesc[_unit])) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
        (BSL_META_U(_unit, \
                    "unit %d is not initialised\n"), \
                   _unit)); \
        return BCM_E_INIT; \
    } \
    (_unitInfo) = _g_unitTemplateDesc[_unit]
#define TEMPLATE_HANDLE_VALID_CHECK(_handle) \
    if (!(_handle)) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
                  (BSL_META("NULL handle is not valid\n"))); \
        return BCM_E_PARAM; \
    }
#define TEMPLATE_POOL_VALID_CHECK(_handle, _pool) \
    if ((0 > (_pool)) || ((_handle)->templatePoolCount <= (_pool))) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
                  (BSL_META("%p pool %d does not exist\n"), \
                   ((void*)(_handle)), _pool)); \
        return BCM_E_PARAM; \
    }
#define TEMPLATE_POOL_EXIST_CHECK(_handle, _pool) \
    if (!((_handle)->pool[_pool])) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
                  (BSL_META("%p pool %d is not configured\n"), \
                   ((void*)(_handle)), _pool)); \
        return BCM_E_CONFIG; \
    }
#define TEMPLATE_TYPE_VALID_CHECK(_handle, _type) \
    if ((0 > (_type)) || ((_handle)->templateTypeCount <= (_type))) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
                  (BSL_META("%p template %d does not exist\n"), \
                   ((void*)(_handle)), _type)); \
        return BCM_E_PARAM; \
    }
#define TEMPLATE_TYPE_EXIST_CHECK(_handle, _type) \
    if (!((_handle)->template[_type])) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
                  (BSL_META("%p template %d is not configured\n"), \
                   ((void*)(_handle)), _type)); \
        return BCM_E_CONFIG; \
    }
#define TEMPLATE_PARAM_NULL_CHECK(_handle, _param) \
    if ((_param) == NULL) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
                  (BSL_META("%p template with obligatory argument is NULL\n"), \
                   _handle)); \
        return BCM_E_PARAM;  \
    }
#define TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(_thisPool,_template) \
      if (!(_template >= thisPool->template_low_id && _template <= (thisPool->template_low_id + thisPool->template_count-1))) { \
        LOG_ERROR(BSL_LS_SOC_COMMON, \
                  (BSL_META("_template id %d is not in correct range. Should be between thisPool->template_low_id %d template_count %d\n") \
                   , _template, thisPool->template_low_id, (thisPool->template_low_id + thisPool->template_count-1))); \
        return BCM_E_PARAM;  \
    }

    
/* Mtemplate_* prototypes */
static int
shr_mtemplate_alloc(shr_mtemplate_handle_t handle,
               int template_type,
               uint32 flags,
               const void *data,
               int *is_allocated,
               int *template);
static int
shr_mtemplate_alloc_group(shr_mtemplate_handle_t handle,
               int template_type,
               uint32 flags,
               const void *data,
               int nof_additions,
               int *is_allocated,
               int *template);
static int
shr_mtemplate_free(shr_mtemplate_handle_t handle,
               int template_type,
               int template,
               int *is_last);
static int
shr_mtemplate_free_group(shr_mtemplate_handle_t handle,
               int template_type,
               int template,
               int nof_deductions,
               int *is_last);
static int
shr_mtemplate_exchange(shr_mtemplate_handle_t handle,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int old_template,
                      int *is_last,
                      int *template,
                      int *is_allocated);
static int
shr_mtemplate_exchange_test(shr_mtemplate_handle_t handle,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int old_template,
                      int *is_last,
                      int *template,
                      int *is_allocated);
static int
shr_mtemplate_data_get(shr_mtemplate_handle_t handle,
                      int template_type,
                      int template,
                      void *data);
static int
shr_mtemplate_index_get(shr_mtemplate_handle_t handle,
                      int template_type,
                      const void *data, 
                      int *template);
static int
shr_mtemplate_ref_count_get(shr_mtemplate_handle_t handle,
                      int template_type,
                      int template,
                      uint32 *ref_count);
static int
shr_mtemplate_create(shr_mtemplate_handle_t *handle,
                int num_template_types,
                int num_template_pools);
static int
shr_mtemplate_get(shr_mtemplate_handle_t handle,
                 int *num_template_types,
                 int *num_template_pools);
static int
shr_mtemplate_pool_set(shr_mtemplate_handle_t handle,
                 int pool_id,
                 shr_template_manage_t manager,
                 int template_low_id,
                 int template_count,
                 int max_entities,
                 uint32 global_max,
                 size_t data_size,
                 const void *extras,
                 const char *name);
static int
shr_mtemplate_pool_get(shr_mtemplate_handle_t handle,
                      int pool_id,
                      shr_template_manage_t *manager,
                      int *template_low_id,
                      int *template_count,
                      int *max_entities,
                      size_t *data_size,
                      const void **extras,
                      const char **name);
static int
shr_mtemplate_type_set(shr_mtemplate_handle_t handle,
                      int template_type,
                      int pool_id,
                      const char *name);
static int
shr_mtemplate_type_get(shr_mtemplate_handle_t handle,
                  int template_type,
                  int *pool_id,
                  const char **name);
static int
_shr_mtemplate_destroy_data(_shr_template_unit_desc_t *unitData);
static int 
shr_mtemplate_dump(shr_mtemplate_handle_t handle, int template_type);
static int 
shr_mtemplate_clear(shr_mtemplate_handle_t handle,
                    int template_type);

/*****************************************************************************/
/*
 *  Exposed API implementation (handle based)
 */

/*
 *      Initialize the tamplate manager for the unit
 */
int
shr_template_init(int unit,
                  int num_template_types,
                  int num_template_pools)
{
    _shr_template_unit_desc_t *tempUnit;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "(%d, %d, %d) enter\n"),
               unit,
               num_template_types,
               num_template_pools));

    /* a little parameter checking */
    if ((0 > unit) || (BCM_LOCAL_UNITS_MAX <= unit)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "invalid unit number %d\n"),
                   unit));
        return BCM_E_PARAM;
    }

    /* get this unit's current information, mark it as destroyed */
    
    tempUnit = _g_unitTemplateDesc[unit];
    _g_unitTemplateDesc[unit] = NULL;
    if (tempUnit) {
        /* this unit has already been initialised; tear it down */
        
        result = _shr_mtemplate_destroy_data(tempUnit);
        if (BCM_E_NONE != result) {
            /* something went wrong with the teardown, put what's left back */
            _g_unitTemplateDesc[unit] = tempUnit;
        }
        tempUnit = NULL;
    }

    if (BCM_E_NONE == result) {
        result = shr_mtemplate_create(&tempUnit,
                                 num_template_types,
                                 num_template_pools);
        if (BCM_E_NONE == result) {
            _g_unitTemplateDesc[unit] = tempUnit;
        }
    } /* if (BCM_E_NONE == result) */

    /* return the result */
    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "(%d, %d, %d) return %d (%s)\n"),
               unit,
               num_template_types,
               num_template_pools,
               result,
               _SHR_ERRMSG(result)));
    return result;
}

/*
 *      Get number of template pools and types configured for a unit
 */
int
shr_template_get(int unit,
                 int *num_template_types,
                 int *num_template_pools)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the fetch */
    return shr_mtemplate_get(thisUnit, num_template_types, num_template_pools);
}

/*
 *      Configure a template pool for the unit
 */
int
shr_template_pool_set(int unit,
                 int pool_id,
                 shr_template_manage_t manager,
                 int template_low_id,
                 int template_count,
                 int max_entities,
                 uint32 global_max,
                 size_t data_size,
                 const void *extras,
                 const char *name)
{
    _shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_pool_set(thisUnit,
                             pool_id,
                             manager,
                             template_low_id,
                             template_count,
                             max_entities,
                             global_max,
                             data_size,
                             extras,
                             name);
}

/*
 *      Get configuration for a resource pool on a particular unit
 */
int
shr_template_pool_get(int unit,
                      int pool_id,
                      shr_template_manage_t *manager,
                      int *template_low_id,
                      int *template_count,
                      int *max_entities,
                      size_t *data_size,
                      const void **extras,
                      const char **name)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_pool_get(thisUnit,
                             pool_id,
                             manager,
                             template_low_id,
                             template_count,
                             max_entities,
                             data_size,
                             extras,
                             name);
}

/*
 *      Configure a template type
 */
int
shr_template_type_set(int unit,
                      int template_type,
                      int pool_id,
                      const char *name)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_type_set(thisUnit, template_type, pool_id, name);
}

/*
 *      Get information about a template type
 */
extern int
shr_template_type_get(int unit,
                      int template_type,
                      int *pool_id,
                      const char **name)
{
	 _shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_type_get(thisUnit, template_type, pool_id, name);
}

/*
 *      Remove all template management for a unit
 */
int
shr_template_detach(int unit)
{
    _shr_template_unit_desc_t *tempUnit;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "(%d) enter\n"),
               unit));

    /* get this unit's current information, mark it as destroyed */
    
    tempUnit = _g_unitTemplateDesc[unit];
    _g_unitTemplateDesc[unit] = NULL;
    if (tempUnit) {
        /* this unit has already been initialised; tear it down */
        result = _shr_mtemplate_destroy_data(tempUnit);
        if (BCM_E_NONE != result) {
            /* something went wrong with the teardown, put what's left back */
            _g_unitTemplateDesc[unit] = tempUnit;
        } else {
            sal_free(tempUnit);
        }
        tempUnit = NULL;
    } /* if (tempUnit) */
    /* else would be not inited, again, easy to detach in that case - NOP */

    /* return the result */
    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "(%d) return %d (%s)\n"),
               unit,
               result,
               _SHR_ERRMSG(result)));
    return result;
}

/*
 *      Find a template to assign to the requested data.
 */
int
shr_template_allocate(int unit,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int *is_allocated,
                      int *template)
{
    _shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_alloc(thisUnit, template_type, flags, data, is_allocated, template);
}

/*
 *      Find a template to assign to the requested data for a group of elements.
 */
int
shr_template_allocate_group(int unit,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int nof_additions,
                      int *is_allocated,
                      int *template)
{
    _shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_alloc_group(thisUnit, template_type, flags, data, nof_additions, is_allocated, template);
}
										 
/*
 *      Release a template (referrer no longer points to it)
 */
extern int
shr_template_free(int unit,
                  int template_type,
                  int template,
                  int *is_last)
{
    _shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_free(thisUnit, template_type, template, is_last);
}

/*
 *      Release several template referers (referrer no longer points to it)
 */
extern int
shr_template_free_group(int unit,
                  int template_type,
                  int template,
                  int nof_deductions,
                  int *is_last)
{
    _shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_free_group(thisUnit, template_type, template, nof_deductions, is_last);
}

/*
 *      Free a current template and allocate a new one for new data
 */
int
shr_template_exchange(int unit,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int old_template,
                      int *is_last,
                      int *template,
                      int *is_allocated)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_exchange(thisUnit, template_type, flags, data, old_template, is_last, template, is_allocated);
}

/*
 *      Tests if Free a current template and allocate a new one for new data is possible
 */
int
shr_template_exchange_test(int unit,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int old_template,
                      int *is_last,
                      int *template,
                      int *is_allocated)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_exchange_test(thisUnit, template_type, flags, data, old_template, is_last, template, is_allocated);
}
										 
/*
 *   Function
 *      shr_template_data_get
 *   Purpose
 *      Get the data from a specific template
 *   Parameters
 *      (IN) unit          : unit number of the device
 *      (IN) template_type : which template type to use
 *      (IN) template      : which template to get
 *      (OUT) data         : where to put the template's data
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 */
int
shr_template_data_get(int unit,
                      int template_type,
                      int template,
                      void *data)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_data_get(thisUnit, template_type, template, data);
}

/*
 *   Function
 *      shr_template_template_get
 *   Purpose
 *      Get the template from a specific data
 *   Parameters
 *      (IN) unit          : unit number of the device
 *      (IN) template_type : which template type to use
 *      (IN) data         : the template's data
 *      (OUT) template      : which template
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 */
int
shr_template_template_get(int unit,
                      int template_type,
                      const void *data,
                      int *template)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_index_get(thisUnit, template_type, data, template);
}


/*
 *   Function
 *      shr_template_ref_count_get
 *   Purpose
 *      Get the reference counter from a specific template id
 *   Parameters
 *      (IN) unit          : unit number of the device
 *      (IN) template_type : which template type to use
 *      (IN) template      : which template to get the reference count
 *      (OUT) ref_count    : where to put the template's reference count
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 */
int
shr_template_ref_count_get(int unit,
                      int template_type,
                      int template,
                      uint32 *ref_count)
{
	_shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_ref_count_get(thisUnit, template_type, template, ref_count);
}

/*
 *   Function
 *      shr_template_dump
 *   Purpose
 *      Diagnostic dump of a unit's resource management information
 *   Parameters
 *      (IN) unit      : unit number of the device
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      This displays the information using bsl_printf.
 */
int
shr_template_dump(int unit, int template_type)
{
    _shr_template_unit_desc_t *thisUnit;    

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_dump(thisUnit, template_type);
}

/*
 *   Function
 *      shr_template_clear
 *   Purpose
 *      Free all templates located in this template_type
 *      (without free memory of the template resource)
 *   Parameters
 *      (IN) unit     	   : unit number of the device
 *      (IN) template_type : which template type to use
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 */
int
shr_template_clear(int unit,
                   int template_type)
{
    _shr_template_unit_desc_t *thisUnit;

    /* a little parameter checking */
    TEMPLATE_UNIT_CHECK(unit, thisUnit);

    /* perform the action */
    return shr_mtemplate_clear(thisUnit,template_type);
}

/*
 *  Allocate template of a resource type
 */
static int
shr_mtemplate_alloc(shr_mtemplate_handle_t handle,
               int template_type,
               uint32 flags,
               const void *data,
               int *is_allocated,
               int *template)               
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %08X) enter\n"),
               (void*)handle,
               template_type,
               flags));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    
    TEMPLATE_PARAM_NULL_CHECK(handle,data);
    TEMPLATE_PARAM_NULL_CHECK(handle,is_allocated);
    TEMPLATE_PARAM_NULL_CHECK(handle,template);

    if (flags & (~SHR_TEMPLATE_MANAGE_SINGLE_FLAGS)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("invalid flags %08X\n"),
                   flags & (~SHR_TEMPLATE_MANAGE_SINGLE_FLAGS)));
        return BCM_E_PARAM;
    }
    
    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];
    if (flags & SHR_TEMPLATE_MANAGE_SINGLE_FLAGS) {
        /* verify template id from pool information */
        TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,*template);
    }
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].alloc(thisPool,
                                                                 flags,
                                                                 data,
                                                                 is_allocated,
                                                                 template);

     if (BCM_E_NONE == result) {
        /* account for successful allocation */
        handle->template[template_type]->refCount++;
    }

     LOG_DEBUG(BSL_LS_SOC_COMMON,
               (BSL_META("(%p, %d, %08X, %p, %d, %d) return %d (%s)\n"),
                (void*)handle,
                template_type,
                flags,
                data,
                *is_allocated,
                *template,
                result,
                _SHR_ERRMSG(result)));

    return result;
}

/*
 *  Allocate template of a resource type for a bunch of elements
 */
static int
shr_mtemplate_alloc_group(shr_mtemplate_handle_t handle,
               int template_type,
               uint32 flags,
               const void *data,
               int nof_additions,
               int *is_allocated,
               int *template)               
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %08X) enter\n"),
               (void*)handle,
               template_type,
               flags));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    
    TEMPLATE_PARAM_NULL_CHECK(handle,data);
    TEMPLATE_PARAM_NULL_CHECK(handle,is_allocated);
    TEMPLATE_PARAM_NULL_CHECK(handle,template);

    if (flags & (~SHR_TEMPLATE_MANAGE_SINGLE_FLAGS)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("invalid flags %08X\n"),
                   flags & (~SHR_TEMPLATE_MANAGE_SINGLE_FLAGS)));
        return BCM_E_PARAM;
    }

    
    if ((flags & (SHR_TEMPLATE_MANAGE_SET_WITH_ID)) == 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("WITH_ID must be specify %08X\n"),
                   flags));
        return BCM_E_PARAM;
    }
    
    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];
    if (flags & SHR_TEMPLATE_MANAGE_SINGLE_FLAGS) {
        /* verify template id from pool information */
        TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,*template);
    }
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].alloc_group(thisPool,
                                                                 flags,
                                                                 data,
                                                                 nof_additions,
                                                                 is_allocated,
                                                                 template);

     if (BCM_E_NONE == result) {
        /* account for successful allocation */
        handle->template[template_type]->refCount+=nof_additions;
    }

     LOG_DEBUG(BSL_LS_SOC_COMMON,
               (BSL_META("(%p, %d, %08X, %p, %d, %d, %d) return %d (%s)\n"),
                (void*)handle,
                template_type,
                flags,
                data,
                nof_additions,
                *is_allocated,
                *template,
                result,
                _SHR_ERRMSG(result)));

    return result;
}

/* Free template (if needed) and decrement reference of the given resource */
static int
shr_mtemplate_free(shr_mtemplate_handle_t handle,
               int template_type,
               int template,
               int *is_last)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) enter\n"),
               (void*)handle,
               template_type,
               template));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);

    TEMPLATE_PARAM_NULL_CHECK(handle,is_last);
    
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];
    /* verify template id from pool information */
    TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,template);
    
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].free(thisPool,
                                                                template,
                                                                is_last);

    if (BCM_E_NONE == result) {
        /* account for successful deallocation */
        handle->template[template_type]->refCount--;
    }

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               template,
               *is_last,
               result,
               _SHR_ERRMSG(result)));
    return result;
}


/* Free template (if needed) and decrement reference of the given resource */
static int
shr_mtemplate_free_group(shr_mtemplate_handle_t handle,
               int template_type,
               int template,
               int nof_deductions,
               int *is_last)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) enter\n"),
               (void*)handle,
               template_type,
               template));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);

    TEMPLATE_PARAM_NULL_CHECK(handle,is_last);
    
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];
    /* verify template id from pool information */
    TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,template);
    
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].free_group(thisPool,
                                                                template,
                                                                nof_deductions,
                                                                is_last);

    if (BCM_E_NONE == result) {
        /* account for successful deallocation */
        handle->template[template_type]->refCount -= nof_deductions;
    }

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               template,
               *is_last,
               result,
               _SHR_ERRMSG(result)));
    return result;
}

static int
shr_mtemplate_exchange(shr_mtemplate_handle_t handle,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int old_template,
                      int *is_last,
                      int *template,
                      int *is_allocated)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;
    uint32 valid_flags = SHR_TEMPLATE_MANAGE_SINGLE_FLAGS | SHR_TEMPLATE_MANAGE_SET_WITH_ID | SHR_TEMPLATE_MANAGE_IGNORE_DATA;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %08X, %d) enter\n"),
               (void*)handle,
               template_type,
               flags,
               old_template));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    
    TEMPLATE_PARAM_NULL_CHECK(handle,data);
    TEMPLATE_PARAM_NULL_CHECK(handle,is_last);
    TEMPLATE_PARAM_NULL_CHECK(handle,is_allocated);
    TEMPLATE_PARAM_NULL_CHECK(handle,template);

    if (flags & ~valid_flags) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("invalid flags %08X\n"),
                   flags & ~valid_flags));
        return BCM_E_PARAM;
    }

    if ((flags & SHR_TEMPLATE_MANAGE_IGNORE_DATA) && !(flags & SHR_TEMPLATE_MANAGE_SET_WITH_ID)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("SHR_TEMPLATE_MANAGE_IGNORE_DATA is only valid in combination with SHR_TEMPLATE_MANAGE_SET_WITH_ID\n")));
        return BCM_E_PARAM;
    }
    
    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];

    /* verify template id from pool information */
    TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,old_template);
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].exchange(thisPool,
                                                                 flags,
                                                                 data,
                                                                 old_template,
                                                                 is_last,
                                                                 template,
                                                                 is_allocated);

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %08X, %d, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               flags,
               old_template,
               *is_last,
               result,
               _SHR_ERRMSG(result)));

    return result;
}

static int
shr_mtemplate_exchange_test(shr_mtemplate_handle_t handle,
                      int template_type,
                      uint32 flags,
                      const void *data,
                      int old_template,
                      int *is_last,
                      int *template,
                      int *is_allocated)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %08X, %d) enter\n"),
               (void*)handle,
               template_type,
               flags,
               old_template));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    
    TEMPLATE_PARAM_NULL_CHECK(handle,data);
    TEMPLATE_PARAM_NULL_CHECK(handle,is_last);
    TEMPLATE_PARAM_NULL_CHECK(handle,is_allocated);
    TEMPLATE_PARAM_NULL_CHECK(handle,template);

    if (flags & (~SHR_TEMPLATE_MANAGE_SINGLE_FLAGS)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("invalid flags %08X\n"),
                   flags & (~SHR_TEMPLATE_MANAGE_SINGLE_FLAGS)));
        return BCM_E_PARAM;
    }
    
    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];

    /* verify template id from pool information */
    TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,old_template);
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].exchange_test(thisPool,
                                                                 flags,
                                                                 data,
                                                                 old_template,
                                                                 is_last,
                                                                 template,
                                                                 is_allocated);

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %08X, %d, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               flags,
               old_template,
               *is_last,
               result,
               _SHR_ERRMSG(result)));

    return result;
}

static int
shr_mtemplate_data_get(shr_mtemplate_handle_t handle,
                      int template_type,
                      int template,
                      void *data)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) enter\n"),
               (void*)handle,
               template_type,
               template));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    
    TEMPLATE_PARAM_NULL_CHECK(handle,data);
    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];

    /* verify template id from pool information */
    TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,template);

    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].data_get(thisPool,
                                                                 template,
                                                                 data);

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               template,
               result,
               _SHR_ERRMSG(result)));

    return result;
}

static int
shr_mtemplate_index_get(shr_mtemplate_handle_t handle,
                      int template_type,
                      const void *data, 
                      int *template)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d) enter\n"),
               (void*)handle,
               template_type));


    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);

    TEMPLATE_PARAM_NULL_CHECK(handle,data);
    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];

    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].index_get(thisPool,
                                                                 data,
                                                                 template);

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               *template,
               result,
               _SHR_ERRMSG(result)));

    return result;
}


static int
shr_mtemplate_ref_count_get(shr_mtemplate_handle_t handle,
                            int template_type,
                            int template,
                            uint32 *ref_count)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) enter\n"),
               (void*)handle,
               template_type,
               template));
               

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    
    TEMPLATE_PARAM_NULL_CHECK(handle,ref_count);

    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];

    /* verify template id from pool information */
    TEMPLATE_POOL_PARAM_TEMPLATE_ID_CHECK(thisPool,template);
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].ref_count_get(thisPool,
                                                                 template,
                                                                 ref_count);

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               template,
               result,
               _SHR_ERRMSG(result)));

    return result;
}

static int
shr_mtemplate_create(shr_mtemplate_handle_t *handle,
                int num_template_types,
                int num_template_pools)
{
    _shr_template_unit_desc_t *tempHandle;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d) enter\n"),
               (void*)handle,
               num_template_types,
               num_template_pools));

    /* a little parameter checking */

    if (!handle) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("obligatory OUT argument must not be NULL\n")));
        result = BCM_E_PARAM;
    }
    if (1 > num_template_pools) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("resource pools %d; must be > 0\n"),
                   num_template_pools));
        result =  BCM_E_PARAM;
    }
    if (1 > num_template_types) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("resource types %d; must be > 0\n"),
                   num_template_types));
        result =  BCM_E_PARAM;
    }
    if (BCM_E_NONE != result) {
        /* displayed diagnostics above */
        return result;
    }
    /* set things up */
    tempHandle = sal_alloc(sizeof(_shr_template_unit_desc_t) +
                         (sizeof(_shr_template_pool_desc_t) * num_template_pools) +
                         (sizeof(_shr_template_type_desc_t) * num_template_types),
                         "resource descriptor");
    if (!tempHandle) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)(sizeof(_shr_template_unit_desc_t) +
                   (sizeof(_shr_template_pool_desc_t) * num_template_pools) +
                   (sizeof(_shr_template_type_desc_t) * num_template_types))));
        result = BCM_E_MEMORY;
    } else { /* if (!tempUnit) */
        /* got the unit information heap cell, set it up */
        sal_memset(tempHandle,
                   0x00,
                   sizeof(_shr_template_unit_desc_t) +
                   (sizeof(_shr_template_pool_desc_t*) * num_template_pools) +
                   (sizeof(_shr_template_type_desc_t*) * num_template_types));
        tempHandle->pool = (_shr_template_pool_desc_t**)(&(tempHandle[1]));
        tempHandle->template = (_shr_template_type_desc_t**)(&(tempHandle->pool[num_template_pools]));
        tempHandle->templateTypeCount = num_template_types;
        tempHandle->templatePoolCount = num_template_pools;
        *handle = tempHandle;
    } /* if (!tempUnit) */

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(&(%p), %d, %d) return %d (%s)\n"),
               (void*)(*handle),
               num_template_types,
               num_template_pools,
               result,
               _SHR_ERRMSG(result)));
    return result;
}


static int
shr_mtemplate_get(shr_mtemplate_handle_t handle,
                 int *num_template_types,
                 int *num_template_pools)
{
    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %p, %p) enter\n"),
               (void*)handle,
               num_template_types,
               num_template_pools));

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    /* return the requested information */
    if (num_template_pools) {
        *num_template_pools = handle->templatePoolCount;
    }
    if (num_template_types) {
        *num_template_types = handle->templateTypeCount;
    }

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, &(%d), &(%d)) return %d (%s)\n"),
               (void*)handle,
               num_template_types?*num_template_types:-1,
               num_template_pools?*num_template_pools:-1,
               BCM_E_NONE,
               _SHR_ERRMSG(BCM_E_NONE)));
    return BCM_E_NONE;
}

static int
shr_mtemplate_pool_set(shr_mtemplate_handle_t handle,
                 int pool_id,
                 shr_template_manage_t manager,
                 int template_low_id,
                 int template_count,
                 int max_entities,
                 uint32 global_max,
                 size_t data_size,
                 const void *extras,
                 const char *name)
{
    _shr_template_pool_desc_t *tempPool;
    _shr_template_pool_desc_t *oldPool;
    int result = BCM_E_NONE;
    int xresult;
    const char *noname = "???";

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %s, %d, %d, %d, %d, %p, \"%s\") enter\n"),
               (void*)handle,
               pool_id,
               ((0 <= manager) && (SHR_TEMPLATE_MANAGE_COUNT > manager))?_shr_template_managements_mgrs[manager].name:"INVALID",
               template_low_id,
               template_count,
               max_entities,
               (int)data_size,
               extras,
               name?name:noname));

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_POOL_VALID_CHECK(handle,pool_id);
    if ((0 > manager) || (SHR_TEMPLATE_MANAGE_COUNT <= manager)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("template manager type %d not supported\n"),
                   manager));
        return BCM_E_PARAM;
    }
    if (0 >= data_size) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("data size must be positive\n")));
        return BCM_E_PARAM;
    }
    if (0 >= max_entities) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("maximum entities must be positive\n")));
        return BCM_E_PARAM;
    }
    if (0 > template_low_id) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("template low id cant be negative \n")));
        return BCM_E_PARAM;
    }
    if (0 >= template_count) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("template count must be positive \n")));
        return BCM_E_PARAM;
    }

    TEMPLATE_PARAM_NULL_CHECK(handle,extras);
    if ((handle->pool[pool_id]) && (handle->pool[pool_id]->refCount)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("%p pool %d (%s) can not be changed because it"
                   " has %d types that use it\n"),
                   (void*)handle,
                   pool_id,
                   handle->pool[pool_id]->name,
                   handle->pool[pool_id]->refCount));
        return BCM_E_CONFIG;
    }
    
    
    oldPool = handle->pool[pool_id];
    handle->pool[pool_id] = NULL;
    /* create the new pool */
    result = _shr_template_managements_mgrs[manager].create(&tempPool,
                                                 template_low_id,
                                                 template_count,
                                                 max_entities,
                                                 global_max,
                                                 data_size,
                                                 extras,
                                                 name?name:noname);
    if (BCM_E_NONE == result) {
        /* new one created successfully */
        tempPool->templateManagerType = manager;
        tempPool->refCount = 0;
        if (oldPool) {
            /* old one exists; get rid of it */
            result = _shr_template_managements_mgrs[oldPool->templateManagerType].destroy(oldPool);
            if (BCM_E_NONE != result) {
                handle->pool[pool_id] = oldPool;
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META("unable to destroy %p old pool %d (%s):"
                           " %d (%s)\n"),
                           (void*)handle,
                           pool_id,
                           oldPool->name,
                           result,
                           _SHR_ERRMSG(result)));
                xresult = _shr_template_managements_mgrs[tempPool->templateManagerType].destroy(tempPool);
                if (BCM_E_NONE != xresult) {
                    LOG_ERROR(BSL_LS_SOC_COMMON,
                              (BSL_META("unable to destroy new pool for %p pool"
                               " %d after replace error: %d (%s)\n"),
                               (void*)handle,
                               pool_id,
                               xresult,
                               _SHR_ERRMSG(xresult)));
                }
                sal_free(tempPool);
                tempPool = NULL;
            } else{
                sal_free(oldPool);
                oldPool = NULL;
            }/* if (BCM_E_NONE != result) */
        } /* if (oldPool) */
    } /* if (BCM_E_NONE == result) */
    if (BCM_E_NONE == result) {
        handle->pool[pool_id] = tempPool;
    }

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %s, %d, %d, %d, %d, %p, \"%s\") return %d (%s)\n"),
               (void*)handle,
               pool_id,
               _shr_template_managements_mgrs[manager].name,
               template_low_id,
               template_count,
               max_entities,
               (int)data_size,
               extras,
               name?name:noname,
               result,
               _SHR_ERRMSG(result)));

    return result;
}

static int
shr_mtemplate_pool_get(shr_mtemplate_handle_t handle,
                      int pool_id,
                      shr_template_manage_t *manager,
                      int *template_low_id,
                      int *template_count,
                      int *max_entities,
                      size_t *data_size,
                      const void **extras,
                      const char **name)
{
    _shr_template_pool_desc_t *thisPool;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %p, %p, %p, %p, %p, %p, %p) enter\n"),
               (void*)handle,
               pool_id,
               manager,
               template_low_id,
               template_count,
               max_entities,
               data_size,
               extras,
               name));

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_POOL_VALID_CHECK(handle, pool_id);
    TEMPLATE_POOL_EXIST_CHECK(handle, pool_id);
    /* fill in the caller's request */
    thisPool = handle->pool[pool_id];
    if (manager) {
        *manager = thisPool->templateManagerType;
    }
    if (template_low_id) {
        *template_low_id = thisPool->template_low_id;
    }
    if (template_count) {
        *template_count = thisPool->template_count;
    }
    if (max_entities) {
        *max_entities = thisPool->max_entities;
    }
    if (data_size) {
        *data_size = thisPool->data_size;
    }
    if (extras) {
        *extras = thisPool->extras;
    }
    if (name) {
        *name = thisPool->name;
    }

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, &(%s), &(%d), &(%d), &(%d), &(%d), &(%p), &(\"%s\")) return %d (%s)\n"),
               (void*)handle,
               pool_id,
               manager?_shr_template_managements_mgrs[*manager].name:"NULL",
               template_low_id?*template_low_id:0,
               template_count?*template_count:0,
               max_entities?*max_entities:0,
               data_size?(int)*data_size:0,
               extras?*extras:NULL,
               name?*name:"NULL",
               BCM_E_NONE,
               _SHR_ERRMSG(BCM_E_NONE)));
    return BCM_E_NONE;
}

static int
shr_mtemplate_type_set(shr_mtemplate_handle_t handle,
                      int template_type,
                      int pool_id,
                      const char *name)
{
    _shr_template_type_desc_t *tempType;
    _shr_template_type_desc_t *oldType;
    int result = BCM_E_NONE;
    const char *noname = "???";
    int len_name = 0;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d, \"%s\") enter\n"),
               (void*)handle,
               template_type,
               pool_id,
               name?name:noname));

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_POOL_VALID_CHECK(handle, pool_id);
    TEMPLATE_POOL_EXIST_CHECK(handle, pool_id);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    if ((handle->template[template_type]) && (handle->template[template_type]->refCount)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("%p template type %d (%s) can not be changed"
                   " because it has %d elements in use\n"),
                   (void*)handle,
                   template_type,
                   handle->template[template_type]->name,
                   handle->template[template_type]->refCount));
        return BCM_E_CONFIG;
    }
    if (!name) {
        /* force a non-NULL name pointer */
        name = noname;
    }

    if ((handle->template[template_type]) && (handle->template[template_type]->refCount)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("%p template type %d (%s) can not be changed"
                   " because it has %d elements in use\n"),
                   (void*)handle,
                   template_type,
                   handle->template[template_type]->name,
                   handle->template[template_type]->refCount));
        return BCM_E_CONFIG;
    }
    
    oldType = handle->template[template_type];
    handle->template[template_type] = NULL;
    /* allocate new type descriptor */
    /* note base type includes one character, so don't need to add NUL here */
    len_name = sal_strlen(name);
    tempType = sal_alloc(sizeof(*tempType) + len_name,
                         "template type descriptor");
    if (tempType) {
        /* got the needed memory; set it up */
        sal_memset(tempType,
                   0x00,
                   sizeof(*tempType) + len_name);
        tempType->templatePoolId = pool_id;
        sal_strncpy(&(tempType->name[0]), name, len_name);
        if (len_name)
            *((char*)&(tempType->name[0])+len_name) = '\0';
        if (oldType) {
            /* there was an old one; get rid of it and adjust references */
            handle->pool[oldType->templatePoolId]->refCount--;
            sal_free(oldType);
        }
        /* adjust references and put this type in place */
        handle->pool[pool_id]->refCount++;
        handle->template[template_type] = tempType;
    } else {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for %p resource"
                   " type %d\n"),
                   (unsigned int)(sizeof(*tempType) + sal_strlen(name)),
                   (void*)handle,
                   template_type));
        result = BCM_E_MEMORY;
        /* restore the old type */
        handle->template[template_type] = oldType;
    }

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %d, \"%s\") return %d (%s)\n"),
               (void*)handle,
               template_type,
               pool_id,
               name?name:noname,
               result,
               _SHR_ERRMSG(result)));
    return result;
}
static int
shr_mtemplate_type_get(shr_mtemplate_handle_t handle,
                  int template_type,
                  int *pool_id,
                  const char **name)
{
    _shr_template_type_desc_t *thisType;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, %p, %p) enter\n"),
               (void*)handle,
               template_type,
               pool_id,
               name));

    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    /* fill in the caller's request */
    thisType = handle->template[template_type];
    if (pool_id) {
        *pool_id = thisType->templatePoolId;
    }
    if (name) {
        *name = thisType->name;
    }

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d, &(%d), &(\"%s\")) return %d (%s)\n"),
               (void*)handle,
               template_type,
               pool_id ? *pool_id : 0,
               name ? *name : "NULL",
               BCM_E_NONE,
               _SHR_ERRMSG(BCM_E_NONE)));
    return BCM_E_NONE;
}

/*
 *  Destroys all of the resources and then pools for a unit.
 */
static int
_shr_mtemplate_destroy_data(_shr_template_unit_desc_t *unitData)
{
    int i;
    int result = BCM_E_NONE;
    _shr_template_type_desc_t *type;
    _shr_template_pool_desc_t *pool;

    /* destroy resources */
    for (i = 0; i < unitData->templateTypeCount; i++) {
        if (unitData->template[i]) {
            
            type = unitData->template[i];
            unitData->template[i] = NULL;
            if (type->refCount) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META("%p type %d (%s): still in use (%d)\n"),
                          unitData,
                          i,
                          &(type->name[0]),
                          type->refCount));
            }
            unitData->pool[type->templatePoolId]->refCount--;
            sal_free(type);
        }
    } /* for (all resources this unit) */

    /* destroy pools */
    for (i = 0;
         (i < unitData->templatePoolCount) && (BCM_E_NONE == result);
         i++) {
        if (unitData->pool[i]) {
            
            pool = unitData->pool[i];
            unitData->pool[i] = NULL;
            if (pool->refCount) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META("%p pool %d (%s): unexpectedly still"
                          " in use (%d) - invalid condition???\n"),
                          unitData,
                          i,
                          &(pool->name[0]),
                          pool->refCount));
            }
            result = _shr_template_managements_mgrs[pool->templateManagerType].destroy(pool);
            if (BCM_E_NONE != result) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META("%p pool %d (%s): unable to destroy:"
                           " %d (%s)\n"),
                           unitData,
                           i,
                           &(pool->name[0]),
                           result,
                           _SHR_ERRMSG(result)));
                unitData->pool[i] = pool;
            } /* if (BCM_E_NONE != result) */
            sal_free(pool);
        } /* if (unitData->pool[i]) */
    } /* for (all pools as long as no errors) */
    return result;
}

static int 
shr_mtemplate_dump(shr_mtemplate_handle_t handle, int template_type)
{
	_shr_template_pool_desc_t *thisPool;
    _shr_template_type_desc_t *thisTemplate;
    int index, template, start_index, last_index, buff_counter;
    int result = BCM_E_NONE;
    char buff[SHR_TEMPLATE_MAX_BUFFER_SIZE_FOR_PRINT];
    uint32 ref_count;
    
    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);

    if( template_type != -1)
    {
        if (template_type < 0 || template_type > handle->templateTypeCount)
        {
            LOG_CLI((BSL_META("  templateType not exist (%d..%d)\n"), 0, handle->templateTypeCount));
            return BCM_E_PARAM;
        }
        start_index = template_type;
        last_index = template_type + 1;
    }
    else
    {
        /* dump information about the handle */
        LOG_CLI((BSL_META("  template manager (%p)\n"), (void*)handle));
        LOG_CLI((BSL_META("  template type count = %d\n"), handle->templateTypeCount));
        LOG_CLI((BSL_META("  template pool count = %d\n"), handle->templatePoolCount));
        start_index = 0;
        last_index = handle->templateTypeCount;
    }
    
    for (index = start_index; index < last_index; index++) {
        thisTemplate = handle->template[index];        

        if (thisTemplate) {
            thisPool = handle->pool[thisTemplate->templatePoolId];            

            LOG_CLI((BSL_META("  template type %d (%s) template pool %d (%s)\n"), index, &(thisTemplate->name[0]), thisTemplate->templatePoolId, &(thisPool->name[0])));
            LOG_CLI((BSL_META("  first template index = %d nof templates     = %d\n"), thisPool->template_low_id, thisPool->template_count));
            LOG_CLI((BSL_META("  number of elements using this template type = %d\n"), thisTemplate->refCount));
            LOG_CLI((BSL_META("  template data size %d \n"), (int)(thisPool->data_size)));

            if(template_type != -1) {                
                for (template = thisPool->template_low_id; template < thisPool->template_low_id + thisPool->template_count; template++) {
                    result = _shr_template_managements_mgrs[thisPool->templateManagerType].ref_count_get(thisPool, template, &ref_count);
                    if (BCM_E_NONE != result) {
                        LOG_CLI((BSL_META("  error retriving ref counter \n")));
                        return BCM_E_FAIL;
                    }

                    if (ref_count > 0 && SHR_TEMPLATE_MAX_BUFFER_SIZE_FOR_PRINT >= thisPool->data_size) {
                        result = _shr_template_managements_mgrs[thisPool->templateManagerType].data_get(thisPool, template, buff);
                        if (BCM_E_NONE != result) {
                            LOG_CLI((BSL_META("  error geting data for template ID %d\n"), template));
                        } else{
                            LOG_CLI((BSL_META("  template id = %d ref counter = %d template data (hex): "), template, ref_count));
                            for (buff_counter = 0; buff_counter < thisPool->data_size; buff_counter++) {
                                LOG_CLI((BSL_META("%02x"), buff[buff_counter]));
                            }
                            LOG_CLI((BSL_META("\n\n")));
                        }
                    }                    
                }
            }
        } else {
            LOG_CLI((BSL_META("   template type %d is not in use\n"), index));
        }
        LOG_CLI((BSL_META("\n")));
    }
    return result;
}

static int 
shr_mtemplate_clear(shr_mtemplate_handle_t handle,
                    int template_type)
{
    _shr_template_pool_desc_t *thisPool;
    int result = BCM_E_NONE;

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d) enter\n"),
               (void*)handle,
               template_type));
               
    /* a little parameter checking */
    TEMPLATE_HANDLE_VALID_CHECK(handle);
    TEMPLATE_TYPE_VALID_CHECK(handle, template_type);
    TEMPLATE_TYPE_EXIST_CHECK(handle, template_type);
    
    /* get the pool information */
    thisPool = handle->pool[handle->template[template_type]->templatePoolId];
    /* make the call */
    result = _shr_template_managements_mgrs[thisPool->templateManagerType].clear(thisPool);

    LOG_DEBUG(BSL_LS_SOC_COMMON,
              (BSL_META("(%p, %d) return %d (%s)\n"),
               (void*)handle,
               template_type,
               result,
               _SHR_ERRMSG(result)));

    return result;
}
/* ******************************************************************/

/* Hash implementation */

typedef struct _shr_template_hash_data_s {
    SOC_SAND_MULTI_SET_INFO multi_set;
} _shr_template_hash_data_t;

/* Hash Utils functions { */
uint32
  _shr_template_hash_multiset_buffer_get_entry(
    SOC_SAND_IN  int                                unit,
    SOC_SAND_IN  uint32                             sec_hanlde,
    SOC_SAND_IN  uint8                              *buffer,
    SOC_SAND_IN  uint32                             offset,
    SOC_SAND_IN  uint32                             len,
    SOC_SAND_OUT uint8                              *data
  )
{
  uint32
    res = SOC_SAND_OK;

  res = SOC_SAND_OK; sal_memcpy(
    data,
    buffer + (offset * len),
    len
    );

  return res;
}


uint32
  _shr_template_hash_multiset_buffer_set_entry(
    SOC_SAND_IN  int                             unit,
    SOC_SAND_IN  uint32                             sec_hanlde,
    SOC_SAND_INOUT  uint8                           *buffer,
    SOC_SAND_IN  uint32                             offset,
    SOC_SAND_IN  uint32                             len,
    SOC_SAND_IN  uint8                              *data
  )
{
  uint32
    res = SOC_SAND_OK;

  res = SOC_SAND_OK; sal_memcpy(
    buffer + (offset * len),
    data,
    len
    );

  return res;
}

/* Hash Utils functions } */

static int _shr_template_hash_create(_shr_template_pool_desc_t **desc,
                                     int template_low_id,
                                     int template_count,
                                     int max_entities,
                                     uint32 global_max,
                                     size_t data_size,
                                     const void *extras,
                                     const char *name)
{
    /* need the base descriptor */
    int result = BCM_E_NONE;
    const shr_template_manage_hash_compare_extras_t *info = extras;
    _shr_template_hash_data_t *hash_data;
    SOC_SAND_MULTI_SET_INFO* multi_set_info;
    uint32 soc_sand_rv = 0;
    int len_name = sal_strlen(name);

    *desc = sal_alloc(sizeof(**desc),"Hash template main descriptor");
    if (!(*desc)) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for main descriptor\n"),
                   (unsigned int)(sizeof(**desc))));
        return BCM_E_MEMORY;
    }
    /* Allocate other stuctures within the descriptor */
    (*desc)->extras = sal_alloc(sizeof(*info),"Hash template extras");
    if (!((*desc)->extras)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for data\n"),
                   (unsigned int)(sizeof(*info))));
        sal_free(*desc);
        *desc = NULL;
        return BCM_E_MEMORY;
    }
    (*desc)->name = sal_alloc(len_name + 1,"Hash template name");
    if (!((*desc)->name)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for data\n"),
                   (unsigned int)(len_name)));
        sal_free((*desc)->extras);
        sal_free(*desc);
        (*desc)->extras = NULL;
        *desc = NULL;
        return BCM_E_MEMORY;
    }
    
    /* Clear structures */
    sal_memset((*desc)->extras, 0x00, sizeof(*info));
    sal_memset((*desc)->name, 0x00, len_name);
    
    (*desc)->template_low_id = template_low_id;
    (*desc)->template_count = template_count;
    (*desc)->max_entities = max_entities;
    (*desc)->data_size = data_size;
    ((shr_template_manage_hash_compare_extras_t*)((*desc)->extras))->orig_data_size = info->orig_data_size;
    ((shr_template_manage_hash_compare_extras_t*)((*desc)->extras))->to_stream = info->to_stream;
    ((shr_template_manage_hash_compare_extras_t*)((*desc)->extras))->from_stream = info->from_stream;

    sal_strncpy((*desc)->name, name, len_name);
    if (len_name)
        (*desc)->name[len_name] = '\0';
    
    /* now allocate the hash space */
    (*desc)->templateHandle = sal_alloc(sizeof(_shr_template_hash_data_t),
                                   "hash template data");
    if (!((*desc)->templateHandle)) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for data\n"),
                   (unsigned int)(sizeof(_shr_template_hash_data_t))));
        sal_free((*desc)->name);
        sal_free((*desc)->extras);
        (*desc)->name = NULL;
        (*desc)->extras = NULL;
        sal_free(*desc);
        *desc = NULL;
        return BCM_E_MEMORY;
    }
    sal_memset((*desc)->templateHandle,
               0x00,
               sizeof(_shr_template_hash_data_t));

    /* Init hash template */
    hash_data = ((*desc)->templateHandle);    
    multi_set_info = &(hash_data->multi_set);
    soc_sand_SAND_MULTI_SET_INFO_clear(multi_set_info);
    multi_set_info->init_info.get_entry_fun = _shr_template_hash_multiset_buffer_get_entry;
    multi_set_info->init_info.set_entry_fun = _shr_template_hash_multiset_buffer_set_entry;
    multi_set_info->init_info.max_duplications = (*desc)->max_entities;
    multi_set_info->init_info.member_size = ((*desc)->data_size);
    multi_set_info->init_info.nof_members = (*desc)->template_count;
    multi_set_info->init_info.sec_handle = 0;
    multi_set_info->init_info.prime_handle = 0;
    soc_sand_rv  = soc_sand_multi_set_create(
            multi_set_info
          );
    if (SOC_SAND_FAILURE(soc_sand_rv)) {
        
        /* Init failed, let's free all memory */
        sal_free((*desc)->name);
        sal_free((*desc)->extras);
        sal_free((*desc)->templateHandle);
        (*desc)->name = NULL;
        (*desc)->extras = NULL;
        (*desc)->templateHandle = NULL;
        sal_free(*desc);
        *desc = NULL;
        return BCM_E_MEMORY;
    }
    

    /* all's well if we got here */
    return result;
}

static int _shr_template_hash_destroy(_shr_template_pool_desc_t *desc)
{
    int res = BCM_E_NONE;
    uint32 soc_sand_rv;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;

    soc_sand_rv = soc_sand_multi_set_destroy(&(hash_data->multi_set));
    SOC_SAND_IF_ERR_RETURN(soc_sand_rv);

    sal_free(desc->extras);
    sal_free(desc->name);
    sal_free(desc->templateHandle);

    return res;
}

static int _shr_template_hash_alloc(_shr_template_pool_desc_t *desc,
                                 uint32 flags,
                                 const void* data,
                                 int *is_allocated,
                                 int *template)
{
    int res = BCM_E_NONE;   
    SOC_SAND_MULTI_SET_KEY * val;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint8 add_success;
    shr_template_manage_hash_compare_extras_t *extra = desc->extras;
    uint32 soc_sand_rv = SOC_SAND_OK;
    uint8 first_appear;
    int template_alloc = 0;
    
    val = sal_alloc((desc->data_size),"Data buffer");

    if (!val) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)
                   ((desc->data_size))));
        res = BCM_E_MEMORY;
    } else {
        sal_memset(val,0x0,(desc->data_size));

        /* move to buffer */
        res = extra->to_stream(data,val,desc->data_size);
        if (res != BCM_E_NONE) {
            goto exit;
        }

        if (flags & SHR_TEMPLATE_MANAGE_SET_WITH_ID) {
            template_alloc = *template - desc->template_low_id;
            soc_sand_rv = soc_sand_multi_set_member_add_at_index(
                        &(hash_data->multi_set),
                        val,
                        template_alloc,
			 &first_appear,
                        &add_success);            
        } else {
           /* Add new data */
            soc_sand_rv = soc_sand_multi_set_member_add(
                        &(hash_data->multi_set),
                        val,
                        (uint32 *)&template_alloc,
			 &first_appear,                        
                        &add_success);
            *template = template_alloc + desc->template_low_id;
        }
   
		*is_allocated = first_appear;

        if (SOC_SAND_FAILURE(soc_sand_rv)) {
            res = BCM_E_INTERNAL;
            goto exit;
        }

        if (!add_success) {
            res = BCM_E_MEMORY;
            goto exit;
        }
        
    }

exit:
    if (val) {
        sal_free(val);
    }
    
    return res;
}

static int _shr_template_hash_alloc_group(_shr_template_pool_desc_t *desc,
                                 uint32 flags,
                                 const void* data,
                                 int nof_additions,
                                 int *is_allocated,
                                 int *template)
{
    int res = BCM_E_NONE;   
    SOC_SAND_MULTI_SET_KEY * val;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint8 add_success;
    shr_template_manage_hash_compare_extras_t *extra = desc->extras;
    uint32 soc_sand_rv = SOC_SAND_OK;
    uint8 first_appear;
    int template_alloc = 0;
    
    val = sal_alloc((desc->data_size),"Data buffer");

    if (!val) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)
                   ((desc->data_size))));
        res = BCM_E_MEMORY;
    } else {
        sal_memset(val,0x0,(desc->data_size));

        /* move to buffer */
        res = extra->to_stream(data,val,desc->data_size);
        if (res != BCM_E_NONE) {
            goto exit;
        }

        if (flags & SHR_TEMPLATE_MANAGE_SET_WITH_ID) {
            template_alloc = *template - desc->template_low_id;
            soc_sand_rv = soc_sand_multi_set_member_add_at_index_nof_additions(
                        &(hash_data->multi_set),
                        val,
                        template_alloc,
                        nof_additions,
                        &first_appear,
                        &add_success);            
        } else {
             
             res = BCM_E_INTERNAL;
             goto exit;
        }
   
        *is_allocated = first_appear;

        if (SOC_SAND_FAILURE(soc_sand_rv)) {
            res = BCM_E_INTERNAL;
            goto exit;
        }

        if (!add_success) {
            res = BCM_E_MEMORY;
            goto exit;
        }
        
    }

exit:
    if (val) {
        sal_free(val);
    }
    
    return res;
}

static int _shr_template_hash_free(_shr_template_pool_desc_t *desc,
                                int template,
                                int *is_last)
{
    int res = BCM_E_NONE;   
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint8 last_appear = 0;
    uint32 soc_sand_rv;
    int template_alloc;
    
    /* Remove old template */
    template_alloc = template - desc->template_low_id;
    soc_sand_rv = soc_sand_multi_set_member_remove_by_index(
              &(hash_data->multi_set),
              template_alloc,
              &last_appear);
    
    if (SOC_SAND_FAILURE(soc_sand_rv)) {
        res = BCM_E_INTERNAL;
    } else {
        if (last_appear) {
            *is_last = 1;
        } else {
            *is_last = 0;
        }
    }

    return res;
}

static int _shr_template_hash_free_group(_shr_template_pool_desc_t *desc,
                                int template,
                                int nof_deductions,
                                int *is_last)
{
    int res = BCM_E_NONE;   
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint8 last_appear = 0;
    uint32 soc_sand_rv;
    int template_alloc;
    
    /* Remove old template */
    template_alloc = template - desc->template_low_id;
    soc_sand_rv = soc_sand_multi_set_member_remove_by_index_multiple(
              &(hash_data->multi_set),
              template_alloc,
              nof_deductions,
              &last_appear);
    
    if (SOC_SAND_FAILURE(soc_sand_rv)) {
        res = BCM_E_INTERNAL;
    } else {
        if (last_appear) {
            *is_last = 1;
        } else {
            *is_last = 0;
        }
    }

    return res;
}

static int _shr_template_hash_exchange(_shr_template_pool_desc_t *desc,
                                uint32 flags,
                                const void *data,
                                int old_template,
                                int *is_last,
                                int *template,
                                int *is_allocated)
{
    int res = BCM_E_NONE;   
    SOC_SAND_MULTI_SET_KEY * val;
    SOC_SAND_MULTI_SET_KEY * old_val;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint8 add_success;
    uint8 first_appear = 0;
    uint8 last_appear = 0;
    int template_alloc = 0;
    int old_template_alloc;

    shr_template_manage_hash_compare_extras_t *extra = desc->extras;
    uint32 soc_sand_rv;
    uint32 ref_count_old;
    
    old_template_alloc = old_template - desc->template_low_id;
    
    val = sal_alloc((desc->data_size),"Data buffer");
    old_val = sal_alloc((desc->data_size),"Data buffer old");
    if (!val || !old_val) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)
                   ((desc->data_size))));
        res = BCM_E_MEMORY;
        goto exit;
    } else {
        sal_memset(val,0x0,(desc->data_size));

        /* move to buffer */
        res = extra->to_stream(data,val,(desc->data_size));
        if (res != BCM_E_NONE) {
            goto exit;
        }

        /* Save old data in case of failure */
        soc_sand_rv = soc_sand_multi_set_get_by_index(
                      &(hash_data->multi_set),
                      old_template_alloc,
                      old_val,
                      &ref_count_old);
        if (SOC_SAND_FAILURE(soc_sand_rv)) {
            /* Get old data failed */
            res = BCM_E_INTERNAL;
            goto exit;
        }

        if (ref_count_old == 0 && !(flags & SHR_TEMPLATE_MANAGE_IGNORE_NOT_EXIST_OLD_TEMPLATE)) {
            /* User gave old template that was empty */
            res = BCM_E_PARAM;
            goto exit;
        }
                      
        if (ref_count_old != 0) {
            /* Remove old template */
            soc_sand_rv = soc_sand_multi_set_member_remove_by_index(
                          &(hash_data->multi_set),
                          old_template_alloc,
						  &last_appear);
                          
			*is_last = last_appear;
            if (SOC_SAND_FAILURE(soc_sand_rv)) {
                /* Remove failed */
                res = BCM_E_INTERNAL;
                goto exit;
            }
        }
        
        /* WITH_ID means that the user would like to exchange the current template for a specific new template */
        /* (as opposed to changing the template to any new template containing the supplied data). */
        /* If IGNORE_DATA is not specified, then the user would also like to change the value of the new template. */
        if (flags & SHR_TEMPLATE_MANAGE_SET_WITH_ID) {
            uint32 new_template = (uint32) *template;
            uint32 old_data_ref_count;

            if (!(flags & SHR_TEMPLATE_MANAGE_IGNORE_DATA)) {
                uint32 new_data_ref_count;
                uint32 old_template_of_new_data;

                /* Make sure that the new data does not exist already for another index. */
                /* This check is actually made inside the insert by index as well. */
                /* However no distinguishable error is returned for it. */
                soc_sand_rv = soc_sand_multi_set_member_lookup(
                              &(hash_data->multi_set),
                              val,
                              &old_template_of_new_data,
                              &new_data_ref_count);
                if (SOC_SAND_FAILURE(soc_sand_rv)) {
                    /* Get data failed */
                    res = BCM_E_INTERNAL;
                    goto exit;
                }

                if ((old_template_of_new_data != new_template) && (new_data_ref_count > 0)) {
                    res = BCM_E_EXISTS;
                    goto exit;
                }

            }

            /* Get the current data at index (and the ref count to it) and put it into the buffer (val). */
            soc_sand_rv = soc_sand_multi_set_get_by_index(
                          &(hash_data->multi_set),
                          new_template,
                          val,
                          &old_data_ref_count);
            if (SOC_SAND_FAILURE(soc_sand_rv)) {
                /* Get data failed */
                res = BCM_E_INTERNAL;
                goto exit;
            }

            if ((flags & SHR_TEMPLATE_MANAGE_IGNORE_DATA) && (old_data_ref_count == 0)) {
                /* WITH_ID and IGNORE_DATA are invalid if the template does not exist - because we don't know where to get */
                /* the new value from. */
                res = BCM_E_NOT_FOUND;
                goto exit;
            }
            
            template_alloc = (old_data_ref_count == 0) ? 1 : 0;

            /* If we ignore the data or we have a new template, then we need not change the existing template value. */
            if ((flags & SHR_TEMPLATE_MANAGE_IGNORE_DATA) || (old_data_ref_count == 0)) {
                /* Add data by index. */
                soc_sand_rv = soc_sand_multi_set_member_add_at_index(
                        &(hash_data->multi_set),
                        val,
                        new_template,
                        &first_appear,
                        &add_success
                );  
            } else /* We need to change the current template value. */ {
                uint8 last_appear;

                /* Free all members using the old data. */
                soc_sand_rv = soc_sand_multi_set_member_remove_by_index_multiple(
                              &(hash_data->multi_set),
                              new_template,
                              old_data_ref_count,
                              &last_appear);
                if (SOC_SAND_FAILURE(soc_sand_rv)) {
                    /* Get data failed */
                    res = BCM_E_INTERNAL;
                    goto exit;
                }

                /* VERIFY(data_template == new_template); */
                /* VERIFY(last_appear); */

                /* Move the new data to the buffer. */
                res = extra->to_stream(data,val,(desc->data_size));
                if (res != BCM_E_NONE) {
                    goto exit;
                }

                /* Add ref_count + 1 members using the new data at the required index. */
                soc_sand_rv = soc_sand_multi_set_member_add_at_index_nof_additions(
                          &(hash_data->multi_set),
                          val,
                          new_template,
                          old_data_ref_count + 1,
                          &first_appear,
                          &add_success
                );  

            }
        } else /* if (!WITH_ID) */ {
            /* Add new data. */
            soc_sand_rv = soc_sand_multi_set_member_add(
                    &(hash_data->multi_set),
                    val,
                    (uint32 *)&template_alloc,
                    &first_appear,
                    &add_success
            );

            *template = template_alloc + desc->template_low_id;
        }

        *is_allocated = first_appear;
   
        if (SOC_SAND_FAILURE(soc_sand_rv) || !add_success) {
            /* Add new failed, return old data */
            if (!add_success) {
                res = BCM_E_MEMORY;
            } else {
                /* General error */
                res = BCM_E_INTERNAL;
            }
            
            if (ref_count_old != 0) {
            
                soc_sand_rv = soc_sand_multi_set_member_add_at_index(
                     &(hash_data->multi_set),
                     old_val,
                     old_template_alloc,                 
                     &first_appear,
                     &add_success);
                
                if (SOC_SAND_FAILURE(soc_sand_rv) || !add_success) {
                    /* Internal error */
                    res = BCM_E_INTERNAL;
                }
            }
            
            goto exit;
        }

    }

exit:
    if (val) {
        sal_free(val);
    }
    if (old_val) {
        sal_free(old_val);
    }
    

    return res;
}

static int _shr_template_hash_exchange_test(_shr_template_pool_desc_t *desc,
                                uint32 flags,
                                const void *data,
                                int old_template,
                                int *is_last,
                                int *template,
                                int *is_allocated)
{
    int res = BCM_E_NONE;   
    SOC_SAND_MULTI_SET_KEY * val;
    SOC_SAND_MULTI_SET_KEY * old_val;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint8 add_success;
    uint8 first_appear = 0;
    uint8 last_appear = 0;
    int old_template_alloc = 0;
    int new_template_alloc = 0;

    shr_template_manage_hash_compare_extras_t *extra = desc->extras;
    uint32 soc_sand_rv;
    uint32 ref_count_old;

    old_template_alloc = old_template - desc->template_low_id;
    
    val = sal_alloc((desc->data_size),"Data buffer");
    old_val = sal_alloc((desc->data_size),"Data buffer old");
    if (!val || !old_val) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)
                   ((desc->data_size))));
        res = BCM_E_MEMORY;
        goto exit;
    } else {
        sal_memset(val,0x0,(desc->data_size));
             
        /* move to buffer */
        res = extra->to_stream(data,val,(desc->data_size));
        if (res != BCM_E_NONE) {
            goto exit;
        }

        /* Save old data in case of failure */
        soc_sand_rv = soc_sand_multi_set_get_by_index(
                      &(hash_data->multi_set),
                      old_template_alloc,
                      old_val,
                      &ref_count_old);
        if (SOC_SAND_FAILURE(soc_sand_rv)) {
            /* Get old data failed */
            res = BCM_E_INTERNAL;
            goto exit;
        }

        if (ref_count_old == 0 && !(flags & SHR_TEMPLATE_MANAGE_IGNORE_NOT_EXIST_OLD_TEMPLATE)) {
            /* User gave old template that was empty */
            res = BCM_E_PARAM;
            goto exit;
        }
                      
        if (ref_count_old != 0) {    
            soc_sand_rv = soc_sand_multi_set_member_remove_by_index(
                          &(hash_data->multi_set),
                          old_template_alloc,
                          &last_appear);

            *is_last = last_appear;
            
            if (SOC_SAND_FAILURE(soc_sand_rv)) {
                res = BCM_E_INTERNAL;
                goto exit;
            }
        }

        /* Add new data */
        soc_sand_rv = soc_sand_multi_set_member_add(
                &(hash_data->multi_set),
                val,
                (uint32 *)&new_template_alloc,
                &first_appear,
                &add_success
        );
        *template = new_template_alloc + desc->template_low_id;

        *is_allocated = first_appear;
   
        if (SOC_SAND_FAILURE(soc_sand_rv) || !add_success) {
            if (!add_success) {
                res = BCM_E_MEMORY;
            } else {
                /* General error */
                res = BCM_E_INTERNAL;
            }
        }
        else {
            /* Remove new data */
               
            soc_sand_rv = soc_sand_multi_set_member_remove_by_index(
                          &(hash_data->multi_set),
                          new_template_alloc,
                          &last_appear);
        }

        if (ref_count_old != 0) {
            soc_sand_rv = soc_sand_multi_set_member_add_at_index(
                 &(hash_data->multi_set),
                 old_val,
                 old_template_alloc,
                 &first_appear,
                 &add_success);
             
            if (SOC_SAND_FAILURE(soc_sand_rv)) {
                res = BCM_E_INTERNAL;
                goto exit;                   
            }        
        }
    }

exit:
    if (val) {
        sal_free(val);
    }
    if (old_val) {
        sal_free(old_val);
    }
    

    return res;
}

static int _shr_template_hash_data_get(_shr_template_pool_desc_t *desc,
                                int template,
                                void *data)
{
    int res = BCM_E_NONE;   
    SOC_SAND_MULTI_SET_KEY * val;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint32 ref_count;
    shr_template_manage_hash_compare_extras_t *extra = desc->extras;
    uint32 soc_sand_rv;
    int template_alloc;
        
    template_alloc = template - desc->template_low_id;

    val = sal_alloc((desc->data_size),"Data buffer");

    if (!val) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)
                   ((desc->data_size))));
        res = BCM_E_MEMORY;
    } else {
        sal_memset(val,0x0,(desc->data_size));

        soc_sand_rv = soc_sand_multi_set_get_by_index(
                    &(hash_data->multi_set),
                    template_alloc,
                    val,
                    &ref_count);

   
        if (!SOC_SAND_FAILURE(soc_sand_rv) && ref_count != 0) {
            res = (extra->from_stream(data,val,(desc->data_size)));
            if (res != BCM_E_NONE) {
                /* Problem with from stream */
                res =  BCM_E_INTERNAL;
            }
        } else {
            /* No data */
            res = BCM_E_NOT_FOUND;
        }
    }

    if (val) {
        sal_free(val);
    }
    return res;
}

static int _shr_template_hash_index_get(_shr_template_pool_desc_t *desc,
                                const void *data,
                                int* template)
{
    int res = BCM_E_NONE;
    SOC_SAND_MULTI_SET_KEY * val;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint32 ref_count;
    shr_template_manage_hash_compare_extras_t *extra = desc->extras;
    uint32 soc_sand_rv;
    uint32 template_alloc;

    val = sal_alloc((desc->data_size),"Data buffer");

    if (!val) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)
                   ((desc->data_size))));
        res = BCM_E_MEMORY;
    } else {
        sal_memset(val,0x0,(desc->data_size));

        /* move to buffer */
        res = extra->to_stream(data,val,(desc->data_size));
        if (res != BCM_E_NONE) {
            goto exit;
        }

        /* Make sure that the new data does not exist already for another index. */
        /* This check is actually made inside the insert by index as well. */
        /* However no distinguishable error is returned for it. */
        soc_sand_rv = soc_sand_multi_set_member_lookup(
                      &(hash_data->multi_set),
                      val,
                      &template_alloc,
                      &ref_count);
        if (SOC_SAND_FAILURE(soc_sand_rv)) {
            /* Get data failed */
            res = BCM_E_INTERNAL;
            goto exit;
        }

        if (ref_count == 0) {
            res = BCM_E_NOT_FOUND;
        } else {
            *template = template_alloc + desc->template_low_id;
        }
    }

exit:
    if (val) {
        sal_free(val);
    }
    return res;
}


static int _shr_template_hash_ref_count_get(_shr_template_pool_desc_t *desc,
                                int template,
                                uint32 *ref_count)
{
    int res = BCM_E_NONE;   
    SOC_SAND_MULTI_SET_KEY * val;
    _shr_template_hash_data_t *hash_data = desc->templateHandle;        
    uint32 soc_sand_rv;
    uint32 tmp_ref_count = 0;
    int template_alloc;

    template_alloc = template - desc->template_low_id;
        
    val = sal_alloc((desc->data_size),"Data buffer");

    if (!val) {
        /* alloc failed */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("unable to allocate %u bytes for info\n"),
                   (unsigned int)
                   ((desc->data_size))));
        res = BCM_E_MEMORY;
    } else {
        sal_memset(val,0x0,(desc->data_size));

        soc_sand_rv = soc_sand_multi_set_get_by_index(
                    &(hash_data->multi_set),
                    template_alloc,
                    val,
                    &tmp_ref_count);

   
        if (!SOC_SAND_FAILURE(soc_sand_rv)) {
            /* Update reference count */
            *ref_count = tmp_ref_count;
        } else {
            /* Got error */
            res = BCM_E_INTERNAL;
        }
    }

    if (val) {
        sal_free(val);
    }
    return res;
}

static int _shr_template_hash_clear(_shr_template_pool_desc_t *desc)
{
    int res = BCM_E_NONE;   
    _shr_template_hash_data_t *hash_data = desc->templateHandle;
    uint32 soc_sand_rv;
    
    soc_sand_rv = soc_sand_multi_set_clear(&(hash_data->multi_set));
    SOC_SAND_IF_ERR_RETURN(soc_sand_rv);    
   
    return res;
}
#endif /* BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
