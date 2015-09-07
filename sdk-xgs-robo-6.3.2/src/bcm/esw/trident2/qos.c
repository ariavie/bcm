/*
 * $Id: qos.c 1.3 Broadcom SDK $
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
 * File:        qos.c
 * Purpose:     Trident2 QoS functions
 */

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/hash.h>
#include <soc/l2x.h>
#include <soc/trident2.h>
#include <bcm/l2.h>
#include <bcm/error.h>
#include <bcm/fcoe.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/l2.h>
#include <bcm_int/esw/fcoe.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/triumph2.h>

#ifdef BCM_TRIDENT2_SUPPORT

#define _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP          8
#define _BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP         64

#define _BCM_QOS_MAX_VFT_PRIORITIES             8
#define _BCM_QOS_MAX_VSAN_PRIORITIES            16
#define _BCM_QOS_MAX_PKT_PRIORITIES             8

#define _BCM_QOS_MAX_VFT_PRI_MAPS                   \
            (soc_mem_index_count(unit, ING_VFT_PRI_MAPm)/ \
             _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP)

#define _BCM_QOS_MAX_VSAN_PRI_MAPS                   \
            (soc_mem_index_count(unit, EGR_VSAN_INTPRI_MAPm)/ \
             _BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP)

#define _BCM_QOS_MAX_VFT_PRI_MAP_IDS         ((_BCM_QOS_MAX_VFT_PRI_MAPS) * 8)
#define _BCM_QOS_MAX_VSAN_PRI_MAP_IDS        ((_BCM_QOS_MAX_VSAN_PRI_MAPS) * 8)

#define _BCM_QOS_MAP_TYPE_MASK           0x3ff
#define _BCM_QOS_MAP_SHIFT                  10

#define _BCM_QOS_NO_MAP                     -1

typedef struct _bcm_td2_qos_bookkeeping_s {
    SHR_BITDCL *ing_vft_pri_map;
    uint32     *ing_vft_pri_map_hwidx;
    SHR_BITDCL *egr_vft_pri_map;
    uint32     *egr_vft_pri_map_hwidx;
    SHR_BITDCL *egr_vsan_intpri_map;
    uint32     *egr_vsan_intpri_map_hwidx;
    sal_mutex_t qos_mutex;
} _bcm_td2_qos_bookkeeping_t;

static _bcm_td2_qos_bookkeeping_t _bcm_td2_qos_bk_info[BCM_MAX_NUM_UNITS];
static int _bcm_td2_qos_initialized[BCM_MAX_NUM_UNITS] = { 0 };

#define VFT_QOS_INFO(_unit_) (&_bcm_td2_qos_bk_info[_unit_])

#define _BCM_QOS_ING_VFT_PRI_MAP_TABLE_USED_GET(_u_, _identifier_) \
        SHR_BITGET(VFT_QOS_INFO(_u_)->ing_vft_pri_map, (_identifier_))
#define _BCM_QOS_ING_VFT_PRI_MAP_TABLE_USED_SET(_u_, _identifier_) \
        SHR_BITSET(VFT_QOS_INFO((_u_))->ing_vft_pri_map, (_identifier_))
#define _BCM_QOS_ING_VFT_PRI_MAP_TABLE_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(VFT_QOS_INFO((_u_))->ing_vft_pri_map, (_identifier_))

#define _BCM_QOS_EGR_VFT_PRI_MAP_TABLE_USED_GET(_u_, _identifier_) \
        SHR_BITGET(VFT_QOS_INFO(_u_)->egr_vft_pri_map, (_identifier_))
#define _BCM_QOS_EGR_VFT_PRI_MAP_TABLE_USED_SET(_u_, _identifier_) \
        SHR_BITSET(VFT_QOS_INFO((_u_))->egr_vft_pri_map, (_identifier_))
#define _BCM_QOS_EGR_VFT_PRI_MAP_TABLE_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(VFT_QOS_INFO((_u_))->egr_vft_pri_map, (_identifier_))

#define _BCM_QOS_EGR_VSAN_PRI_MAP_TABLE_USED_GET(_u_, _identifier_) \
        SHR_BITGET(VFT_QOS_INFO(_u_)->egr_vsan_intpri_map, (_identifier_))
#define _BCM_QOS_EGR_VSAN_PRI_MAP_TABLE_USED_SET(_u_, _identifier_) \
        SHR_BITSET(VFT_QOS_INFO((_u_))->egr_vsan_intpri_map, (_identifier_))
#define _BCM_QOS_EGR_VSAN_PRI_MAP_TABLE_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(VFT_QOS_INFO((_u_))->egr_vsan_intpri_map, (_identifier_))

#define QOS_LOCK(unit) \
        sal_mutex_take(VFT_QOS_INFO(unit)->qos_mutex, sal_mutex_FOREVER);
#define QOS_UNLOCK(unit) \
        sal_mutex_give(VFT_QOS_INFO(unit)->qos_mutex);

#define QOS_INIT(unit)                                    \
    do {                                                  \
        if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {  \
            return BCM_E_UNIT;                            \
        }                                                 \
        if (!_bcm_td2_qos_initialized[unit]) {            \
            return BCM_E_INIT;                            \
        }                                                 \
    } while (0)


#define TD2_HW_COLOR_GREEN   0
#define TD2_HW_COLOR_YELLOW  3
#define TD2_HW_COLOR_RED     1
#define TD2_HW_COLOR_ERROR  -1

/*
 * Function: 
 *     _bcm_td2_qos_free_resources
 * Purpose:
 *     Free TD2 QOS bookkeeping resources associated with a given unit.  This
 *     happens either during an error initializing or during a detach operation.
 * Parameters:
 *     unit - (IN) Unit to free resources of
 * Returns:
 *     BCM_E_XXX
 */
STATIC void
_bcm_td2_qos_free_resources(int unit)
{
    _bcm_td2_qos_bookkeeping_t *qos_info = VFT_QOS_INFO(unit);

    if (!qos_info) {
        return;
    }

    if (qos_info->ing_vft_pri_map) {
        sal_free(qos_info->ing_vft_pri_map);
        qos_info->ing_vft_pri_map = NULL;
    }

    if (qos_info->ing_vft_pri_map_hwidx) {
        sal_free(qos_info->ing_vft_pri_map_hwidx);
        qos_info->ing_vft_pri_map_hwidx = NULL;
    }

    if (qos_info->egr_vft_pri_map) {
        sal_free(qos_info->egr_vft_pri_map);
        qos_info->egr_vft_pri_map = NULL;
    }

    if (qos_info->egr_vft_pri_map_hwidx) {
        sal_free(qos_info->egr_vft_pri_map_hwidx);
        qos_info->egr_vft_pri_map_hwidx = NULL;
    }

    if (qos_info->egr_vsan_intpri_map) {
        sal_free(qos_info->egr_vsan_intpri_map);
        qos_info->egr_vsan_intpri_map = NULL;
    }

    if (qos_info->egr_vft_pri_map_hwidx) {
        sal_free(qos_info->egr_vft_pri_map_hwidx);
        qos_info->egr_vft_pri_map_hwidx = NULL;
    }

    if (qos_info->qos_mutex) {
        sal_mutex_destroy(qos_info->qos_mutex);
        qos_info->qos_mutex = NULL;
    }

}

/*
 * Function: 
 *     bcm_td2_qos_detach
 * Purpose:
 *     Detach the QOS module.  Called when the module should de-initialize.
 * Parameters:
 *     unit - (IN) Unit being detached.
 * Returns:
 *     BCM_E_XXX
 */
int 
bcm_td2_qos_detach(int unit)
{
    if (_bcm_td2_qos_initialized[unit]) {
        _bcm_td2_qos_initialized[unit] = 0;
        _bcm_td2_qos_free_resources(unit);
    }

    return BCM_E_NONE;
}

/*
 * Function: 
 *     bcm_td2_qos_init
 * Purpose:
 *     Initialize the TD2 QOS module.  Allocate bookkeeping information.
 * Parameters:
 *     unit - (IN) Unit being initialized
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_qos_init(int unit)
{
    _bcm_td2_qos_bookkeeping_t *qos_info = VFT_QOS_INFO(unit);
    int vft_map_size  = _BCM_QOS_MAX_VFT_PRI_MAP_IDS;
    int vsan_map_size = _BCM_QOS_MAX_VSAN_PRI_MAP_IDS;

    if (_bcm_td2_qos_initialized[unit]) {
        BCM_IF_ERROR_RETURN(bcm_td2_qos_detach(unit));
    }

    memset(qos_info, 0, sizeof *qos_info);

    /* ingress priority map bitmap */
    qos_info->ing_vft_pri_map = sal_alloc(SHR_BITALLOCSIZE(vft_map_size), 
                                          "ing_vft_pri_map");
    if (qos_info->ing_vft_pri_map == NULL) {
        _bcm_td2_qos_free_resources(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(qos_info->ing_vft_pri_map, 0, SHR_BITALLOCSIZE(vft_map_size));

    /* ingress priority map hardware indexes */
    qos_info->ing_vft_pri_map_hwidx = sal_alloc(sizeof(uint32) * vft_map_size, 
                                             "ing_vft_pri_map_hwidx");
    if (qos_info->ing_vft_pri_map_hwidx == NULL) {
        _bcm_td2_qos_free_resources(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(qos_info->ing_vft_pri_map_hwidx, 0, 
               sizeof(uint32) * vft_map_size);

    /* egress priority map bitmap */
    qos_info->egr_vft_pri_map = sal_alloc(SHR_BITALLOCSIZE(vft_map_size), 
                                          "egr_vft_pri_map");
    if (qos_info->egr_vft_pri_map == NULL) {
        _bcm_td2_qos_free_resources(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(qos_info->egr_vft_pri_map, 0, SHR_BITALLOCSIZE(vft_map_size));

    /* egress priority map hardware indexes */
    qos_info->egr_vft_pri_map_hwidx = sal_alloc(sizeof(uint32) * vft_map_size, 
                                             "egr_vft_pri_map_hwidx");
    if (qos_info->egr_vft_pri_map_hwidx == NULL) {
        _bcm_td2_qos_free_resources(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(qos_info->egr_vft_pri_map_hwidx, 0, 
               sizeof(uint32) * vft_map_size);

    /* egress vsan priority map bitmap */
    qos_info->egr_vsan_intpri_map = sal_alloc(SHR_BITALLOCSIZE(vsan_map_size), 
                                              "egr_vsan_intpri_map");
    if (qos_info->egr_vsan_intpri_map == NULL) {
        _bcm_td2_qos_free_resources(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(qos_info->egr_vsan_intpri_map, 0, 
               SHR_BITALLOCSIZE(vsan_map_size));

    /* egress vsan priority map hardware indexes */
    qos_info->egr_vsan_intpri_map_hwidx = 
        sal_alloc(sizeof(uint32) * vsan_map_size, "egr_vsan_intpri_map_hwidx");
    if (qos_info->egr_vsan_intpri_map_hwidx == NULL) {
        _bcm_td2_qos_free_resources(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(qos_info->egr_vsan_intpri_map_hwidx, 0, 
               sizeof(uint32) * vsan_map_size);

    /* accessor mutex */
    qos_info->qos_mutex = sal_mutex_create("vft qos_mutex");
    if (qos_info->qos_mutex == NULL) {
        _bcm_td2_qos_free_resources(unit);
        return BCM_E_MEMORY;
    }

    _bcm_td2_qos_initialized[unit] = 1;
    return BCM_E_NONE;
}

/*
 * Function: 
 *     _bcm_td2_qos_map_id_alloc
 * Purpose:
 *     Allocate a new QOS map ID.  The bitmap given contains bits set for 
 *     existing IDs, so find the first empty one, set it, and return it.
 * Parameters:
 *     unit   - (IN) Unit to be operated on
 *     bitmap - (IN/OUT)
 *     id     - (OUT)
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_qos_map_id_alloc(int unit, SHR_BITDCL *bitmap, int *id, 
                          int map_size)
{
    int i;

    for (i = 0; i < map_size; i++) {
        if (!SHR_BITGET(bitmap, i)) {
            SHR_BITSET(bitmap, i);
            *id = i;
            return BCM_E_NONE;
        }
    }

    return BCM_E_RESOURCE;
}

/*
 * Function: 
 *     _bcm_td2_qos_map_create_with_id
 * Purpose:
 *     Allocate a new QOS map ID where the app has specified the ID.  So just
 *     check that the requested ID is available, and mark it as used.
 * Parameters:
 *     unit       - (IN) Unit to be operated on
 *     flags      - (IN) Used only to test if BCM_QOS_MAP_REPLACE is set
 *     bitmap     - (IN/OUT) Bitmap containing the set of used IDs
 *     map_id     - (IN) The ID requested by the application
 *     is_ingress - (IN) 1=ingress map, 0=egress map
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_qos_map_create_with_id(int unit, int flags, SHR_BITDCL *bitmap, 
                                    int *map_id, int is_ingress, int max_ids)
{
    int id = 0, map_type = 0, rv = BCM_E_NONE;

    id = *map_id & _BCM_QOS_MAP_TYPE_MASK;
    map_type = *map_id >> _BCM_QOS_MAP_SHIFT;

    if (id < 0 || id >= max_ids) {
        return BCM_E_BADID;
    }

    if ((map_type == _BCM_QOS_MAP_TYPE_ING_VFT_PRI_MAP && is_ingress)
        || (map_type == _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP && !is_ingress)
        || (map_type == _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP && !is_ingress)) {

        if (SHR_BITGET(bitmap, id) == 1) {
            if (!(flags & BCM_QOS_MAP_REPLACE)) {
                rv = BCM_E_EXISTS;
            }
        }

        if (BCM_SUCCESS(rv)) {
            SHR_BITSET(bitmap, id);
        }
    } else {
        rv = BCM_E_PARAM;
    }

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_qos_count_used_maps
 * Purpose:
 *     Count the number maps currently used in hardware.  Since IDs in app-space
 *     can point to the same hardware index, count how many hardware indexes are
 *     used, ignoring duplicates.
 * Parameters:
 *     unit   - (IN) Unit to be operated on
 *     bitmap - (IN) Bitmap of used map IDs
 *     hwmap  - (IN) Array that maps userspace IDs to hardware IDs
 * Returns:
 *     integer containing the number of used hardware indexes
 */
STATIC int
_bcm_td2_qos_count_used_maps(int unit, SHR_BITDCL *bitmap, uint32 *hwmap, 
                             int map_size, int max_maps) 
{
    uint8 *hw_used = sal_alloc(map_size, "td2_qos_used_maps");
    int total_used = 0;
    int i = 0;
    
    if (hw_used == NULL) {
        return max_maps;
    }

    memset(hw_used, 0, map_size);

    for (i = 0; i < map_size; i++) {
        if (SHR_BITGET(bitmap, i) && hw_used[hwmap[i]] == 0) {
            hw_used[hwmap[i]] = 1;
            total_used++;
        }
    }

    sal_free(hw_used);
    return total_used;
}

/*
 * Function: 
 *     _bcm_td2_qos_flags_sanity_check
 * Purpose:
 *     Check that the combination of flags specified is valid.
 * Parameters:
 *     flags  - (IN) BCM_QOS_MAP_* flags (eg. ingress/egress/with_id/replace)
  * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_qos_flags_sanity_check(uint32 flags)
{
    int rv = BCM_E_NONE;

    /* must be ingress or egress, not both */
    if ((flags & BCM_QOS_MAP_INGRESS) && (flags & BCM_QOS_MAP_EGRESS)) {
        rv = BCM_E_PARAM;
    }

    /* must be ingress or egress, not neither */
    if (!(flags & BCM_QOS_MAP_INGRESS) && !(flags & BCM_QOS_MAP_EGRESS)) {
        rv = BCM_E_PARAM;
    }

    /* must be VFT or VSAN, not both */
    if ((flags & BCM_QOS_MAP_VFT) && (flags & BCM_QOS_MAP_VSAN)) {
        rv = BCM_E_PARAM;
    }

    /* must be VFT or VSAN, not neither */
    if (!(flags & BCM_QOS_MAP_VFT) && !(flags & BCM_QOS_MAP_VSAN)) {
        rv = BCM_E_PARAM;
    }

    /* VSAN maps can only be egress */
    if ((flags & BCM_QOS_MAP_INGRESS) && (flags & BCM_QOS_MAP_VSAN)) {
        rv = BCM_E_PARAM;
    }

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_qos_assign_creation_table_vars
 * Purpose:
 *     Assign the various pointers to tables and table sizes and bitmaps used
 *     for the bcm_td2_qos_map_create so that it can create the different types
 *     of maps with common logic.  
 * 
 *     Assumes "flags" has already been validated.
 * Parameters:
 *     unit       - (IN)  Unit to be operated on
 *     flags      - (IN)  BCM_QOS_MAP_* flags (eg. ingress/egress/with_id/etc)
 *     is_ingress - (OUT) 1 for ingress, 0 for egress map
 *     bitmap     - (OUT) pointer to priority map's bitmap
 *     hwmap      - (OUT) pointer to priority map's hwidx mapping
 *     map_type   - (OUT) _BCM_QOS_MAP_TYPE_*
 *     map_size   - (OUT) size of the selected map
 *     max_maps   - (OUT) maximum number of maps available for the selected type
 * Returns:
 *     None
 */
STATIC void
_bcm_td2_qos_assign_creation_table_vars(int unit, int flags, int *is_ingress, 
                                        SHR_BITDCL **bitmap,
                                        uint32 **hwmap, int *map_type, 
                                        int *map_size, int *max_maps)
{
    if (flags & BCM_QOS_MAP_INGRESS) {
        *is_ingress = 1;
        *bitmap     = VFT_QOS_INFO(unit)->ing_vft_pri_map;
        *hwmap      = VFT_QOS_INFO(unit)->ing_vft_pri_map_hwidx;
        *map_type   = _BCM_QOS_MAP_TYPE_ING_VFT_PRI_MAP;
        *map_size   = _BCM_QOS_MAX_VFT_PRI_MAP_IDS;
        *max_maps   = _BCM_QOS_MAX_VFT_PRI_MAPS;
    } else if (flags & BCM_QOS_MAP_EGRESS) {
        *is_ingress = 0;

        if (flags & BCM_QOS_MAP_VFT) {
            *bitmap     = VFT_QOS_INFO(unit)->egr_vft_pri_map;
            *hwmap      = VFT_QOS_INFO(unit)->egr_vft_pri_map_hwidx;
            *map_type   = _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP;
            *map_size   = _BCM_QOS_MAX_VFT_PRI_MAP_IDS; 
            *max_maps   = _BCM_QOS_MAX_VFT_PRI_MAPS;
        } else if (flags & BCM_QOS_MAP_VSAN) {
            *bitmap     = VFT_QOS_INFO(unit)->egr_vsan_intpri_map;
            *hwmap      = VFT_QOS_INFO(unit)->egr_vsan_intpri_map_hwidx;
            *map_type   = _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP;
            *map_size   = _BCM_QOS_MAX_VSAN_PRI_MAP_IDS; 
            *max_maps   = _BCM_QOS_MAX_VSAN_PRI_MAPS;
        }
    }

}

/*
 * Function: 
 *     bcm_td2_qos_map_create
 * Purpose:
 *     Create a new VFT QOS map, mapping VFT priority to internal (for ingress)
 *     or internal to VFT (for egress).
 * Parameters:
 *     unit   - (IN) Unit to be operated on
 *     flags  - (IN) BCM_QOS_MAP_* flags (eg. ingress/egress/with_id/replace)
 *     map_id - (IN/OUT) If BCM_QOS_MAP_WITH_ID is set, then this is the ID
 *                       being requested.  Otherwise, it's assigned as the ID
 *                       allocated.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_qos_map_create(int unit, uint32 flags, int *map_id)
{
    int        is_ingress = 0;
    int        map_type = 0;
    SHR_BITDCL *bitmap = NULL;
    uint32     index = 0;
    int        rv    = BCM_E_NONE;
    int        id    = 0;
    void       *entries = NULL;
    uint32     *hwmap = NULL;
    int        map_size = 0;
    int        max_maps = 0;

    QOS_INIT(unit);
    QOS_LOCK(unit);

    rv = _bcm_td2_qos_flags_sanity_check(flags);

    if (BCM_SUCCESS(rv)) {
        _bcm_td2_qos_assign_creation_table_vars(unit, flags, &is_ingress, 
                                                &bitmap, &hwmap, &map_type, 
                                                &map_size, &max_maps);
    }

    if (BCM_SUCCESS(rv)) {
        if (_bcm_td2_qos_count_used_maps(unit, bitmap, hwmap, map_size, 
                                         max_maps) >= max_maps) {
            rv = BCM_E_MEMORY;
        }
    }

    if (BCM_SUCCESS(rv)) {
        if (flags & BCM_QOS_MAP_WITH_ID) {
            rv = _bcm_td2_qos_map_create_with_id(unit, flags, bitmap, map_id, 
                                                 is_ingress, map_size);
            id = *map_id & _BCM_QOS_MAP_TYPE_MASK;
        } else {
            rv = _bcm_td2_qos_map_id_alloc(unit, bitmap, &id, map_size);
            if (BCM_SUCCESS(rv)) {
                *map_id = id | (map_type << _BCM_QOS_MAP_SHIFT);
            }
        }
    }

    if (BCM_SUCCESS(rv)) {
        if (flags & BCM_QOS_MAP_VSAN) {
            egr_vsan_intpri_map_entry_t table[_BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP];
            memset(&table, 0, sizeof table);
            entries = &table;

            rv = _bcm_egr_vsan_intpri_map_entry_add(unit, &entries, 
                                                _BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP, 
                                                &index);

            if (BCM_SUCCESS(rv)) {
                VFT_QOS_INFO(unit)->egr_vsan_intpri_map_hwidx[id] = index;
            }
        }
        else if (flags & BCM_QOS_MAP_INGRESS) {
            ing_vft_pri_map_entry_t table[_BCM_QOS_MAP_CHUNK_VFT_PRI_MAP];
            memset(&table, 0, sizeof table);
            entries = &table;

            rv = _bcm_ing_vft_pri_map_entry_add(unit, &entries, 
                                                _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP, 
                                                &index);

            if (BCM_SUCCESS(rv)) {
                VFT_QOS_INFO(unit)->ing_vft_pri_map_hwidx[id] = index;
            }
        } else {
            egr_vft_pri_map_entry_t table[_BCM_QOS_MAP_CHUNK_VFT_PRI_MAP];
            memset(&table, 0, sizeof table);
            entries = &table;

            rv = _bcm_egr_vft_pri_map_entry_add(unit, &entries, 
                                                _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP, 
                                                &index);

            if (BCM_SUCCESS(rv)) {
                VFT_QOS_INFO(unit)->egr_vft_pri_map_hwidx[id] = index;
            }
        }
    }

    QOS_UNLOCK(unit);

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_qos_map_destroy
 * Purpose:
 *     Destroy a VFT/VSAN map ID by removing it from hardware.
 * Parameters:
 *     unit  - (IN) The unit being operated on
 *     table - (IN) Indicates either ingress or egress
 *     id    - (IN) The map ID being destroyed
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_qos_map_destroy(int unit, int table, int id)
{
    int rv = BCM_E_NONE;
    _bcm_td2_qos_bookkeeping_t *qos_info = VFT_QOS_INFO(unit);

    if (table == _BCM_QOS_MAP_TYPE_ING_VFT_PRI_MAP) {
        rv = _bcm_ing_vft_pri_map_entry_del(unit, 
                                        qos_info->ing_vft_pri_map_hwidx[id]);
    } else if (table == _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP) {
        rv = _bcm_ing_vft_pri_map_entry_del(unit, 
                                        qos_info->egr_vft_pri_map_hwidx[id]);
    } else if (table == _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP) {
        rv = _bcm_egr_vsan_intpri_map_entry_del(unit, 
                                    qos_info->egr_vsan_intpri_map_hwidx[id]);
    }

    return rv;
}

/*
 * Function: 
 *     bcm_td2_qos_map_destroy
 * Purpose:
 *     Destroy a VFT QOS map ID by removing it from hardware and deleting the
 *     associated software state info.
 * Parameters:
 *     unit   - (IN) Unit being operated on
 *     map_id - (IN) QOS map ID to be destroyed
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_qos_map_destroy(int unit, int map_id)
{
    int id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    int rv = BCM_E_UNAVAIL;

    QOS_INIT(unit);
    QOS_LOCK(unit);

    switch (map_id >> _BCM_QOS_MAP_SHIFT) {
        case _BCM_QOS_MAP_TYPE_ING_VFT_PRI_MAP:
            if (!_BCM_QOS_ING_VFT_PRI_MAP_TABLE_USED_GET(unit, id)) {
                rv = BCM_E_NOT_FOUND;
            } else {
                rv = _bcm_td2_qos_map_destroy(unit, 
                        _BCM_QOS_MAP_TYPE_ING_VFT_PRI_MAP, id);

                if (BCM_SUCCESS(rv)) {
                    _BCM_QOS_ING_VFT_PRI_MAP_TABLE_USED_CLR(unit, id);
                }
            }
            break;

        case _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP:
            if (!_BCM_QOS_EGR_VFT_PRI_MAP_TABLE_USED_GET(unit, id)) {
                rv = BCM_E_NOT_FOUND;
            } else {
                rv = _bcm_td2_qos_map_destroy(unit, 
                        _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP, id);

                if (BCM_SUCCESS(rv)) {
                    _BCM_QOS_EGR_VFT_PRI_MAP_TABLE_USED_CLR(unit, id);
                }
            }
            break;

        case _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP:
            if (!_BCM_QOS_EGR_VSAN_PRI_MAP_TABLE_USED_GET(unit, id)) {
                rv = BCM_E_NOT_FOUND;
            } else {
                rv = _bcm_td2_qos_map_destroy(unit, 
                        _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP, id);

                if (BCM_SUCCESS(rv)) {
                    _BCM_QOS_EGR_VSAN_PRI_MAP_TABLE_USED_CLR(unit, id);
                }
            }
            break;

        default:
            rv = BCM_E_PARAM;
    }

    QOS_UNLOCK(unit);

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_qos_map_add_sanity_check
 * Purpose:
 *     Check that all the paramters given to the bcm_qos_map_add() function make
 *     sense and are internally consistent.
 * Parameters:
 *     unit   - (IN) The unit being operated on
 *     flags  - (IN) BCM_QOS_MAP_* flags sent in by the app
 *     map    - (IN) The map structure sent in by the app
 *     map_id - (IN) The map ID sent in by the app
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_qos_map_add_sanity_check(int unit, uint32 flags, bcm_qos_map_t *map, 
                                  int map_id)
{
    int map_type = map_id >> _BCM_QOS_MAP_SHIFT;
    int id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    int rv;

    rv = _bcm_td2_qos_flags_sanity_check(flags);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if ((flags & BCM_QOS_MAP_INGRESS) 
        && (map_type != _BCM_QOS_MAP_TYPE_ING_VFT_PRI_MAP)) {
        return BCM_E_PARAM;
    }

    if ((flags & BCM_QOS_MAP_EGRESS) 
        && ((map_type != _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP)
             && (map_type != _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP))
           ) {
        return BCM_E_PARAM;
    }

    if ((flags & BCM_QOS_MAP_INGRESS) 
        && (_BCM_QOS_ING_VFT_PRI_MAP_TABLE_USED_GET(unit, id) == 0)) {
        return BCM_E_BADID;
    }

    if ((flags & BCM_QOS_MAP_EGRESS) 
        && (map_type == _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP)
        && (_BCM_QOS_EGR_VFT_PRI_MAP_TABLE_USED_GET(unit, id) == 0)) {
        return BCM_E_BADID;
    }

    if ((flags & BCM_QOS_MAP_EGRESS) 
        && (map_type == _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP)
        && (_BCM_QOS_EGR_VSAN_PRI_MAP_TABLE_USED_GET(unit, id) == 0)) {
        return BCM_E_BADID;
    }

    if (map->int_pri < 0) {
        return BCM_E_PARAM;
    }

    if (flags & BCM_QOS_MAP_VFT) {
        if (map->int_pri >= _BCM_QOS_MAX_VFT_PRIORITIES) {
            return BCM_E_PARAM;
        }
    } else if (flags & BCM_QOS_MAP_VSAN) {
        if (map->int_pri >= _BCM_QOS_MAX_VSAN_PRIORITIES) {
            return BCM_E_PARAM;
        }
    }

    if (map->pkt_pri >= _BCM_QOS_MAX_PKT_PRIORITIES) {
        return BCM_E_PARAM;
    }

    if (flags & BCM_QOS_MAP_VSAN) {
        if (map->color != bcmColorGreen && map->color != bcmColorYellow
            && map->color != bcmColorRed) {
            return BCM_E_PARAM;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function: 
 *     _bcm_td2_ing_qos_map_update_index
 * Purpose:
 *     Update an existing profile (or a newly created one) in the hardware's
 *     ING_VFT_PRI_MAP table with a new mapping.  
 *   
 *     The structure is: (1) get existing profile for this map from hardware, 
 *     (2) update the profile in RAM, (3) delete the old profile from hardware, 
 *     and (4) push the updated profile in RAM to hardware.
 * Parameters:
 *     unit   - (IN) The unit being operated on
 *     map    - (IN) Map structure sent in by app to be applied to hardware
 *     map_id - (IN) Map ID sent in by app to be applied to hardware
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_ing_qos_map_update_index(int unit, bcm_qos_map_t *map, int map_id)
{
    void    *entries;
    int     rv;
    uint32  index;
    int     id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    ing_vft_pri_map_entry_t table[_BCM_QOS_MAP_CHUNK_VFT_PRI_MAP];

    index = VFT_QOS_INFO(unit)->ing_vft_pri_map_hwidx[id];

    entries = &table;
    rv = _bcm_ing_vft_pri_map_entry_get(unit, index, 
                                        _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP, 
                                        &entries);

    if (BCM_SUCCESS(rv)) {
        ing_vft_pri_map_entry_t *entry = &table[map->pkt_pri];

        soc_mem_field32_set(unit, ING_VFT_PRI_MAPm, entry, FCOE_VFT_PRIf, 
                            map->int_pri);

        rv = _bcm_ing_vft_pri_map_entry_del(unit, (int)index);
    }

    if (BCM_SUCCESS(rv)) {
        rv = _bcm_ing_vft_pri_map_entry_add(unit, &entries, 
                                            _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP, 
                                            &index);
        VFT_QOS_INFO(unit)->ing_vft_pri_map_hwidx[id] = index;
    }

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_egr_qos_map_update_index
 * Purpose:
 *     Update an existing profile (or a newly created one) in the hardware's
 *     EGR_VFT_PRI_MAP table with a new mapping.  
 *   
 *     The structure is: (1) get existing profile for this map from hardware, 
 *     (2) update the profile in RAM, (3) delete the old profile from hardware, 
 *     and (4) push the updated profile in RAM to hardware.
 * Parameters:
 *     unit   - (IN) The unit being operated on
 *     map    - (IN) Map structure sent in by app to be applied to hardware
 *     map_id - (IN) Map ID sent in by app to be applied to hardware
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_egr_qos_map_update_index(int unit, bcm_qos_map_t *map, int map_id)
{
    void    *entries;
    int     rv;
    uint32  index;
    int     id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    egr_vft_pri_map_entry_t table[_BCM_QOS_MAP_CHUNK_VFT_PRI_MAP];

    index = VFT_QOS_INFO(unit)->egr_vft_pri_map_hwidx[id];

    entries = &table;
    rv = _bcm_egr_vft_pri_map_entry_get(unit, index, 
                                        _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP, 
                                        &entries);

    if (BCM_SUCCESS(rv)) {
        egr_vft_pri_map_entry_t *entry = &table[map->int_pri];

        soc_mem_field32_set(unit, ING_VFT_PRI_MAPm, entry, FCOE_VFT_PRIf, 
                            map->pkt_pri);

        rv = _bcm_egr_vft_pri_map_entry_del(unit, index);
    }

    if (BCM_SUCCESS(rv)) {
        rv = _bcm_egr_vft_pri_map_entry_add(unit, &entries, 
                                            _BCM_QOS_MAP_CHUNK_VFT_PRI_MAP, 
                                            &index);
        VFT_QOS_INFO(unit)->egr_vft_pri_map_hwidx[id] = index;
    }

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_color_to_hw_color
 * Purpose:
 *      Convert from bcm_color_t enum to hardware color values for VSAN QOS
 *      mapping
 * Parameters:
 *      color    - (IN)  Value to convert from
 *      hw_color - (OUT) Converted hardware color value
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_color_to_hw_color(bcm_color_t color, int *hw_color)
{
    int rv = BCM_E_NONE;

    switch (color) {
        case bcmColorRed:    *hw_color = TD2_HW_COLOR_RED;    break;
        case bcmColorYellow: *hw_color = TD2_HW_COLOR_YELLOW; break;
        case bcmColorGreen:  *hw_color = TD2_HW_COLOR_GREEN;  break;
        default:
            rv = BCM_E_PARAM;
            break;
    }

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_egr_vsan_map_update_index
 * Purpose:
 *     Update an existing profile (or a newly created one) in the hardware's
 *     EGR_VSAN_INTPRI_MAP table with a new mapping.  
 *   
 *     The structure is: (1) get existing profile for this map from hardware, 
 *     (2) update the profile in RAM, (3) delete the old profile from hardware, 
 *     and (4) push the updated profile in RAM to hardware.
 * Parameters:
 *     unit   - (IN) The unit being operated on
 *     map    - (IN) Map structure sent in by app to be applied to hardware
 *     map_id - (IN) Map ID sent in by app to be applied to hardware
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_egr_vsan_map_update_index(int unit, bcm_qos_map_t *map, int map_id)
{
    void    *entries;
    int     rv;
    uint32  index;
    int     id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    egr_vsan_intpri_map_entry_t table[_BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP];

    index = VFT_QOS_INFO(unit)->egr_vsan_intpri_map_hwidx[id];

    entries = &table;
    rv = _bcm_egr_vsan_intpri_map_entry_get(unit, index, 
                                           _BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP, 
                                           &entries);

    if (BCM_SUCCESS(rv)) {
        int hw_color = TD2_HW_COLOR_ERROR;
        egr_vsan_intpri_map_entry_t *entry = NULL;

        rv = _bcm_td2_color_to_hw_color(map->color, &hw_color);
        if (BCM_SUCCESS(rv)) {
            entry = &table[(_BCM_QOS_MAX_VSAN_PRIORITIES * hw_color) 
                           + map->int_pri];

            soc_mem_field32_set(unit, EGR_VSAN_INTPRI_MAPm, entry, PRIf, 
                                map->pkt_pri);

            rv = _bcm_egr_vsan_intpri_map_entry_del(unit, index);
        }
    }

    if (BCM_SUCCESS(rv)) {
        rv = _bcm_egr_vsan_intpri_map_entry_add(unit, &entries, 
                                            _BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP, 
                                            &index);
        VFT_QOS_INFO(unit)->egr_vsan_intpri_map_hwidx[id] = index;
    }

    return rv;
}
/*
 * Function: 
 *     bcm_td2_qos_map_add
 * Purpose:
 *     Add a new QOS mapping to hardware.
 * Parameters:
 *     unit   - (IN) The unit being operated on
 *     flags  - (IN) BCM_QOS_MAP_* flags
 *     map    - (IN) Structure containing the priorities to be mapped
 *     map_id - (IN) The ID of the map to add the new information to
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_qos_map_add(int unit, uint32 flags, bcm_qos_map_t *map, int map_id)
{
    int rv = BCM_E_NONE;

    QOS_INIT(unit);
    QOS_LOCK(unit);

    rv = _bcm_td2_qos_map_add_sanity_check(unit, flags, map, map_id);
    if (BCM_SUCCESS(rv)) {
        if (flags & BCM_QOS_MAP_VSAN) {
            rv = _bcm_td2_egr_vsan_map_update_index(unit, map, map_id);
        } else if (flags & BCM_QOS_MAP_INGRESS) {
            rv = _bcm_td2_ing_qos_map_update_index(unit, map, map_id);
        } else {
            rv = _bcm_td2_egr_qos_map_update_index(unit, map, map_id);
        }
    }

    QOS_UNLOCK(unit);

    return rv;
}

/*
 * Function: 
 *     bcm_td2_qos_map_delete
 * Purpose:
 *     Delete a mapping (for example, the map from 7 to 3).  This is equivalent
 *     to changing it to a map from 7 to 0, so this function maps the call into
 *     bcm_td2_qos_map_add().
 * Parameters:
 *     unit   - (IN) The unit being operated on
 *     flags  - (IN) BCM_QOS_MAP_* flags
 *     map    - (IN) Structure containing the priorities to be mapped
 *     map_id - (IN) The ID of the map to add the new information to
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_qos_map_delete(int unit, uint32 flags, bcm_qos_map_t *map, int map_id)
{
    int rv = BCM_E_NONE;
    bcm_qos_map_t clr_map;
    
    QOS_INIT(unit);

    sal_memcpy(&clr_map, map, sizeof clr_map);

    rv = _bcm_td2_qos_flags_sanity_check(flags);
    if (BCM_SUCCESS(rv)) {
        if (flags & BCM_QOS_MAP_INGRESS) {
            clr_map.int_pri = 0;
        } else {
            clr_map.pkt_pri = 0;
        }
    }

    if (BCM_SUCCESS(rv)) {
        rv = bcm_td2_qos_map_add(unit, flags, &clr_map, map_id);
    }

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_qos_apply_ing_map_to_port
 * Purpose:
 *     Apply a fully created ingress map to a hardware port.  This is the final
 *     step in the process, where the mapping becomes operational.
 * Parameters:
 *     unit       - (IN) Unit being operated on
 *     id         - (IN) ID of the map to be applied
 *     local_port - (IN) Local port number to apply map to
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_qos_apply_ing_map_to_port(int unit, int id, bcm_port_t local_port)
{
    int rv;

    soc_mem_lock(unit, PORT_TABm);

    rv = _bcm_tr2_port_tab_set(unit, local_port, FCOE_VFT_PRI_MAP_PROFILEf,
                               VFT_QOS_INFO(unit)->ing_vft_pri_map_hwidx[id]);

    soc_mem_unlock(unit, PORT_TABm);

    return rv;
}

/*
 * Function: 
 *     _bcm_td2_qos_apply_egr_map_to_port
 * Purpose:
 *     Apply a fully created egress map to a hardware port.  This is the final
 *     step in the process, where the mapping becomes operational.
 * Parameters:
 *     unit       - (IN) Unit being operated on
 *     id         - (IN) ID of the map to be applied
 *     local_port - (IN) Local port number to apply map to
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td2_qos_apply_egr_map_to_port(int unit, int id, bcm_port_t local_port)
{
    int rv;

    soc_mem_lock(unit, PORT_TABm);

    rv = _bcm_td2_egr_port_set(unit, local_port, FCOE_VFT_PRI_MAP_PROFILEf,
                               VFT_QOS_INFO(unit)->egr_vft_pri_map_hwidx[id]);

    soc_mem_unlock(unit, PORT_TABm);

    return rv;
}

/*
 * Function: 
 *     bcm_td2_qos_get_egr_vsan_hw_idx
 * Purpose:
 *     Given a QOS map ID for a VSAN mapping profile, return the hardware index
 *     into the EGR_VSAN_INTPRI_MAP table.
 * Parameters:
 *     unit       - (IN) Unit being operated on
 *     egr_map_id - (IN) ID of the map to be retrieved
 *     hw_index   - (OUT) Hardware index that egr_map_id maps to
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_qos_get_egr_vsan_hw_idx(int unit, int egr_map_id, int *hw_index)
{
    int map_type = egr_map_id >> _BCM_QOS_MAP_SHIFT;
    int id       = egr_map_id & _BCM_QOS_MAP_TYPE_MASK;

    if ((map_type != _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP) 
        || (_BCM_QOS_EGR_VSAN_PRI_MAP_TABLE_USED_GET(unit, id) == 0)) {
        return BCM_E_BADID;
    }

    *hw_index = VFT_QOS_INFO(unit)->egr_vsan_intpri_map_hwidx[id] 
                / _BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP;

    return BCM_E_NONE;
}

/*
 * Function: 
 *     bcm_td2_vsan_profile_to_qos_id
 * Purpose:
 *     Given a hardware profile number index, find the QOS map ID which maps to it
 * Parameters:
 *     unit        - (IN) Unit being operated on
 *     profile_num - (IN) profile number of the map to be retrieved
 *     egr_map     - (OUT) Map ID that points to profile_num
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_vsan_profile_to_qos_id(int unit, int profile_num, int *egr_map)
{
    int i;

    for (i = 0; i < _BCM_QOS_MAX_VSAN_PRI_MAP_IDS; i++) {
        if (VFT_QOS_INFO(unit)->egr_vsan_intpri_map_hwidx[i] 
            == (profile_num * _BCM_QOS_MAP_CHUNK_VSAN_PRI_MAP)) {
            *egr_map = (_BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP << _BCM_QOS_MAP_SHIFT) 
                       | i;
            return BCM_E_NONE;
        }
    }

    return BCM_E_BADID;
}

/*
 * Function: 
 *     bcm_td2_qos_port_map_set
 * Purpose:
 *     Apply a fully created ingress and/or egress map to a given port.  If
 *     either map is given as -1 (_BCM_QOS_NO_MAP), then the parameter should be
 *     ignored.
 * Parameters:
 *     unit    - (IN) The unit to be operated on
 *     port    - (IN) The port number for the map to be applied to
 *     ing_map - (IN) The ID of an ingress map to be applied
 *     egr_map - (IN) The ID of an egress map to be applied
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_td2_qos_port_map_set(int unit, bcm_gport_t port, int ing_map, int egr_map)
{
    int rv = BCM_E_NONE;
    bcm_port_t local_port;
    
    QOS_INIT(unit);
    QOS_LOCK(unit);

    if (!BCM_GPORT_IS_SET(port)) {
        rv = BCM_E_PARAM;
    }

    if (BCM_SUCCESS(rv)) {
        rv = bcm_esw_port_local_get(unit, port, &local_port);
    }

    if (BCM_SUCCESS(rv) && !SOC_PORT_VALID(unit, local_port)) {
        rv = BCM_E_PARAM;
    }

    if (BCM_SUCCESS(rv) && ing_map != _BCM_QOS_NO_MAP) {
        int id = ing_map & _BCM_QOS_MAP_TYPE_MASK;

        if ((ing_map >> _BCM_QOS_MAP_SHIFT) != 
            _BCM_QOS_MAP_TYPE_ING_VFT_PRI_MAP) {
            rv = BCM_E_PARAM;
        }

        if (BCM_SUCCESS(rv) && 
            _BCM_QOS_ING_VFT_PRI_MAP_TABLE_USED_GET(unit, id) != 1) {
            rv = BCM_E_PARAM;
        }

        if (BCM_SUCCESS(rv)) {
            rv = _bcm_td2_qos_apply_ing_map_to_port(unit, ing_map, local_port);
        }
    }

    if (BCM_SUCCESS(rv) && egr_map != _BCM_QOS_NO_MAP) {
        int id = egr_map & _BCM_QOS_MAP_TYPE_MASK;

        if ((egr_map >> _BCM_QOS_MAP_SHIFT) == 
            _BCM_QOS_MAP_TYPE_EGR_VFT_PRI_MAP) {
            if (_BCM_QOS_EGR_VFT_PRI_MAP_TABLE_USED_GET(unit, id) == 1) {
                rv = _bcm_td2_qos_apply_egr_map_to_port(unit, egr_map, 
                                                        local_port);
            } else {
                rv = BCM_E_PARAM;
            }
        } else if ((egr_map >> _BCM_QOS_MAP_SHIFT) == 
                 _BCM_QOS_MAP_TYPE_EGR_VSAN_PRI_MAP) {
            rv = BCM_E_PARAM; /* should use bcm_td2_fcoe_intf_config_set */
        }
    }

    QOS_UNLOCK(unit);

    return rv;
}

#undef DSCP_CODE_POINT_CNT
#define DSCP_CODE_POINT_CNT 64
#undef DSCP_CODE_POINT_MAX
#define DSCP_CODE_POINT_MAX (DSCP_CODE_POINT_CNT - 1)
#define _BCM_QOS_MAP_CHUNK_DSCP  64

 /* Function:
 *      bcm_td2_port_dscp_map_set
 * Purpose:
 *      Internal implementation for bcm_td2_port_dscp_map_set
 * Parameters:
 *      unit - switch device
 *      port - switch port or -1 for global table
 *      srccp - src code point or -1
 *      mapcp - mapped code point or -1
 *      prio - priority value for mapped code point
 *              -1 to use port default untagged priority
 *              BCM_PRIO_RED    can be or'ed into the priority
 *              BCM_PRIO_YELLOW can be or'ed into the priority
 * Returns:
 *      BCM_E_XXX
 */


int
bcm_td2_port_dscp_map_set(int unit, bcm_port_t port, int srccp,
                            int mapcp, int prio, int cng)
{
    port_tab_entry_t         pent;
    void                     *entries[2];
    uint32                   old_profile_index = 0;
    int                      index = 0;
    int                      rv = BCM_E_NONE;
    dscp_table_entry_t dscp_table[64];
    int  offset = 0,i = 0;   
    void *entry;
    /* Lock the port table */
    soc_mem_lock(unit, PORT_TABm); 

    /* Get profile index from port table. */
    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY,
                         port, &pent);
    if (BCM_FAILURE(rv)) {
        goto cleanup_bcm_td2_port_dscp_map_set;
    }

    old_profile_index =
        soc_mem_field32_get(unit, PORT_TABm, &pent, TRUST_DSCP_PTRf) *  DSCP_CODE_POINT_CNT;

    sal_memset(dscp_table, 0, sizeof(dscp_table));

            /* Base index of table in hardware */

    entries[0] = &dscp_table;

    rv =  _bcm_dscp_table_entry_get(unit,
                             old_profile_index, 64, entries);
    offset = srccp;
            /* Modify what's needed */

    if (offset < 0) {
        for (i = 0; i <= DSCP_CODE_POINT_MAX; i++) {
             entry = &dscp_table[i];
             soc_DSCP_TABLEm_field32_set(unit, entry, DSCPf, mapcp);
             soc_DSCP_TABLEm_field32_set(unit, entry, PRIf, prio);
             soc_DSCP_TABLEm_field32_set(unit, entry, CNGf, cng);
        }
    } else {
        entry = &dscp_table[offset];
        soc_DSCP_TABLEm_field32_set(unit, entry, DSCPf, mapcp);
        soc_DSCP_TABLEm_field32_set(unit, entry, PRIf, prio);
        soc_DSCP_TABLEm_field32_set(unit, entry, CNGf, cng);
    }

            /* Add new chunk and store new HW index */

    rv = _bcm_dscp_table_entry_add(unit, entries, _BCM_QOS_MAP_CHUNK_DSCP, 
                                           (uint32 *)&index);
    if (BCM_FAILURE(rv)) {
        goto cleanup_bcm_td2_port_dscp_map_set;
    }

    if (index != old_profile_index) {
        soc_mem_field32_set(unit, PORT_TABm, &pent, TRUST_DSCP_PTRf,
                            index / _BCM_QOS_MAP_CHUNK_DSCP);
        rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port,
                            &pent);
        if (BCM_FAILURE(rv)) {
            goto cleanup_bcm_td2_port_dscp_map_set;
        }
    }

    rv = _bcm_dscp_table_entry_delete(unit, old_profile_index);
    cleanup_bcm_td2_port_dscp_map_set:
    /* Release port table lock */
        soc_mem_unlock(unit, PORT_TABm);
        return rv;
}
#endif /* BCM_TRIDENT2_SUPPORT */
