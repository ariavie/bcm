/*
 * $Id: bregex.c 1.64 Broadcom SDK $
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
 * COS Queue Management
 * Purpose: Regex API for flow tracking
 */
#include <sal/core/libc.h>

#include <soc/defs.h>
#if defined(BCM_TRIUMPH3_SUPPORT)

#if defined(INCLUDE_REGEX)
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/debug.h>

#include <soc/triumph3.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm/error.h>
#include <bcm/debug.h>
#include <bcm/bregex.h>
#include <bcm_int/regex_api.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/policer.h>

#include <bcm_int/esw_dispatch.h>

#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw/bregex.h>

#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm_int/esw/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */

#define BCM_TR3_MAX_ENGINE                  32
#define BCM_TR3_MAX_MATCH_MEM               4
#define BCM_TR3_MAX_NUM_MATCH               256
#define BCM_TR3_MAX_SLOTS                   512
#define BCM_TR3_MAX_TILES                   8

#define BCM_TR3_LINE_BYTE_SIZE              32
#define BCM_TR3_NUM_RV_PER_LINE             8

/* Maximum number of flow */
#define BCM_TR3_MAX_FLOWS   8192

/* maximum payload depth */
#define BCM_TR3_MAX_PAYLOAD_DEPTH   8192

/* maximum packets to SME */
#define BCM_TR3_MAX_PKT_TO_SME   16

#define BCM_REGEX_ENG2MATCH_MEMIDX(u,e)    ((e)/8)

#define BCM_REGEX_NUM_SLOT_PER_CPS 16

#define BCM_TR3_SPECIAL_SLOT 0x2012

typedef struct _bcm_regex_cps_obj_s {
    uint32    flags;
    int             foff;
    int             alloc_map[BCM_REGEX_NUM_SLOT_PER_CPS];
    int             refcnt[BCM_REGEX_NUM_SLOT_PER_CPS];
    int             from_line;
    int             req_lsz;
} _bcm_regex_cps_obj_t;

#define BCM_REGEX_CPS_F_ALLOC     1

#define BCM_REGEX_CPS_ALLOCED(pc) ((pc)->flags & BCM_REGEX_CPS_F_ALLOC)

#define BCM_REGEX_CPS_SET_ALLOCED(pc) (pc)->flags |= BCM_REGEX_CPS_F_ALLOC

#define BCM_REGEX_CPS_FREE_ALLOCED(pc) (pc)->flags &= ~BCM_REGEX_CPS_F_ALLOC

#define BCM_TR3_MAX_SIG_ID  1024

#define BCM_TR3_SIG_ID_MAP_BSZ   ((1024+31)/32)

#define BCM_TR3_POLICER_NUM     256

STATIC int _bcm_tr3_free_cps(int unit, _bcm_regex_cps_obj_t *pcps);

typedef struct _bcm_regex_engine_obj_s {
    int                     hw_idx;
    uint32            flags;
    int                     total_lsz;  
    /* HW mapping */
    int                     tile;
    int                     from_line;
    int                     req_lsz;
    _bcm_regex_cps_obj_t    cps;
} _bcm_regex_engine_obj_t;

#define BCM_TR3_REGEX_ENGINE_ALLOC          0x01
#define BCM_REGEX_ENGINE_ALLOCATED(eng) \
                        ((eng)->flags & BCM_TR3_REGEX_ENGINE_ALLOC)

#define BCM_REGEX_ENGINE_SET_ALLOCATED(eng) \
                         (eng)->flags |= BCM_TR3_REGEX_ENGINE_ALLOC

#define BCM_REGEX_ENGINE_SET_UNALLOCATED(eng) \
                         (eng)->flags &= ~BCM_TR3_REGEX_ENGINE_ALLOC

typedef struct _bcm_regex_lengine_s {
    bcm_regex_engine_t      id;
    uint32                  flags;
    bcm_regex_engine_config_t config;
    _bcm_regex_engine_obj_t *physical_engine;
} _bcm_regex_lengine_t;

typedef int (*reset_engine)(int unit, int engine_idx);
typedef int (*enable_engine)(int unit, _bcm_regex_engine_obj_t *pengine);
typedef int (*get_engine_info)(int unit, int engine_idx, 
                                            _bcm_regex_engine_obj_t *e);
typedef int (*disable_engine)(int unit, int engine_idx);
typedef uint32 (*l2pf)(int line, int off);

typedef struct bcm_regex_desc_s {
    int                 num_tiles;
    int                 num_engine_per_tile;
    int                 num_match_mem;
    int                 line_sz;
    reset_engine        reset_engine;
    get_engine_info     get_engine_info;
    enable_engine       enable_engine;
    disable_engine      disable_engine;
} bcm_regex_desc_t;

#define BCM_REGEX_RESET_ENGINE(d, u, e) (d)->info->reset_engine((u), (e))

#define BCM_REGEX_GET_ENGINE_INFO(d, u, pe, po) \
                                    (d)->info->get_engine_info((u), (pe), (po))

#define BCM_REGEX_ENABLE_ENGINE(d, u, pe) (d)->info->enable_engine((u), (pe))
#define BCM_REGEX_DISABLE_ENGINE(d, u, e) (d)->info->disable_engine((u), (e))

#define BCM_TR3_MATCH_BMAP_BSZ  ((BCM_TR3_MAX_NUM_MATCH+31)/32)

#define BCM_REGEX_DB_BUCKET_MAX 32

STATIC int _bcm_tr3_alloc_regex_engine(int unit, int clsz, int extra_lsz,
                            _bcm_regex_engine_obj_t **ppeng, int intile);

struct bcm_regexdb_entry_s;

typedef struct _bcm_tr3_regex_device_info_s {
    int max_engines;

    bcm_regex_desc_t *info;

    _bcm_regex_engine_obj_t physical_engines[BCM_TR3_MAX_ENGINE];

    /* logical engines table */
    _bcm_regex_lengine_t engines[BCM_TR3_MAX_ENGINE];

    uint32      match_alloc_bmap[BCM_TR3_MAX_MATCH_MEM][BCM_TR3_MATCH_BMAP_BSZ];

    struct bcm_regexdb_entry_s *l[BCM_REGEX_DB_BUCKET_MAX];

    sal_mutex_t regex_mutex;

    uint32      sigid[BCM_TR3_SIG_ID_MAP_BSZ];    

    bcm_stat_flex_pool_attribute_t flex_pools[2];

    uint32      policer_bmp[BCM_TR3_POLICER_NUM/sizeof(uint32)];
    bcm_policer_config_t policer_cfg[BCM_TR3_POLICER_NUM];
} _bcm_tr3_regex_device_info_t;

#define _BCM_TR3_FLEX_CTR_POOL_VALID(p) (((p) == -1) ? 0 : 1)

STATIC _bcm_tr3_regex_device_info_t *_bcm_tr3_regex_info[BCM_MAX_NUM_UNITS];

STATIC soc_profile_mem_t *_bcm_tr3_ft_policy_profile[BCM_MAX_NUM_UNITS];

#define REGEX_INFO(u) _bcm_tr3_regex_info[(u)]

#define BCM_TR3_NUM_ENGINE(d)   ((d)->max_engines)

#define BCM_TR3_LOGICAL_ENGINE(d,leid)  \
  ((leid) >= BCM_TR3_NUM_ENGINE((d))) ? NULL : &(d)->engines[(leid)]

#define BCM_TR3_PHYSICAL_ENGINE(d,peid) \
  ((peid) >= (BCM_TR3_NUM_ENGINE((d)))) ? NULL : &(d)->physical_engines[(peid)]

#define BCM_TR3_ENGINE_PER_TILE(d)  ((d)->info->num_engine_per_tile)

#define BCM_TR3_CPS_INFO(d,peid)    &((d)->physical_engines[(peid)].cps)

#define BCM_TR3_MATCH_BMAP_PTR(u,d,peid)    \
            (d)->match_alloc_bmap[BCM_REGEX_ENG2MATCH_MEMIDX((u),(peid))]

#define _BCM_TR3_POLICER_HW_INDEX(i) ((i) + 7936)

#define _BCM_TR3_ENCODE_POLICER_ID(u,idx)   \
             (((((u) & 255) << 16) | ((idx) & 0xffff)) | \
              (BCM_POLICER_TYPE_REGEX << BCM_POLICER_TYPE_SHIFT))

#define _BCM_TR3_POLICER_ID_TO_INDEX(pid) ((pid) & 0xffff)

typedef struct bcm_regexdb_entry_s {
    int         sigid;
    int         engid;
    int         match_id;
    uint16      match_index;
    int         provides;
    int         requires;
    short int   msetid;
    short int   mchkid;
    struct bcm_regexdb_entry_s *next;
} _bcm_regexdb_entry_t;

typedef struct _bcm_tr3_regex_data_s {
    int                     unit;
    uint32                  flags;
    int                     compile_size;
    /* HW engine information */
    _bcm_regex_engine_obj_t *hw_eng;
    bcm_regex_match_t       *matches;
    int                     num_match;
    int                     *msetids;
    int                     *mchkids;
    int                     *sig_ids;
    int                     *hw_match_idx;

    /* temp information to compile to HW */
    int                     last_state;
    soc_mem_t               mem;
    axp_sm_state_table_mem0_entry_t line;
   
    uint16                  *per_state_data;
    uint16                  wr_off;
    uint16                  line_off;
    int                     line_dirty;
} _bcm_tr3_regex_data_t;

typedef struct _bcm_regex_resource_s {
    _bcm_regex_engine_obj_t *pres;
    int     eng_idx;
    int     lsize;
} _bcm_regex_resource_t;

STATIC int
_bcm_regex_sort_resources_by_size(int unit,
                                  _bcm_regex_resource_t *res, int num_res);

STATIC int
_bcm_regex_sort_resources_by_offset(int unit, 
                                  _bcm_regex_resource_t *res, int num_res);

typedef struct _bcm_regex_tile_s {
    int valid;
    int freep;
    int avail;
    int oidx[BCM_TR3_MAX_ENGINE]; 
    int num_o;
    axp_sm_match_table_mem0_entry_t *match_mem;
} _bcm_regex_tile_t;

STATIC int
_bcm_tr3_arrange_resources_in_tiles(int unit, _bcm_regex_resource_t *res, 
                                    int num_res, int to_tiles, int to_tilee,
                                    _bcm_regex_tile_t *_tr3tile, int num_tile);

STATIC int
_bcm_tr3_rearrange_engines_in_hw(int unit, _bcm_regex_resource_t *res, 
                           int num_res, int to_tiles, int to_tilee, 
                           _bcm_regex_tile_t *_tr3tile);

STATIC int
_bcm_tr3_regex_alloc_setids(int unit, int *msetids, bcm_regex_match_t *matches,
                            int count, int pengine_hwidx);

STATIC int
_bcm_regexdb_handle_setid_avail(int unit, int avail, int engid,
                                int provides, int setid);

STATIC int _bcm_tr3_alloc_sigid(int unit, bcm_regex_match_t *match)
{
    _bcm_tr3_regex_device_info_t *device;
    int     i, j;

    device = REGEX_INFO(unit);

    for (i=0; i<BCM_TR3_SIG_ID_MAP_BSZ; i++) {
        if (device->sigid[i] == 0) {
            continue;
        }
        for (j = 0; j < 32; j++) {
            if (device->sigid[i] & (1 << j)) {
                device->sigid[i] &= ~(1 << j);
                return ((i*32) + j);
            }
        }
    }
    return -1;
}

STATIC int
_bcm_tr3_free_sigid(int unit, int sigid)
{
    _bcm_tr3_regex_device_info_t *device;

    if ((sigid < 0) || (sigid >= BCM_TR3_MAX_SIG_ID)) {
        return BCM_E_PARAM;
    }

    device = REGEX_INFO(unit);
    device->sigid[sigid/32] |= 1 << (sigid % 32);
    return BCM_E_NONE;
}

typedef struct _bcm_regexdb_cb_ua_s {
    int avail;
    int setid;
} _bcm_regexdb_cb_ua_t;

/* locking */
STATIC int _bcm_tr3_regex_lock (int unit)
{
    _bcm_tr3_regex_device_info_t *device;

    if ((device = REGEX_INFO(unit)) == NULL) {
        return BCM_E_INIT;
    }
    
    sal_mutex_take(device->regex_mutex, sal_mutex_FOREVER);
    return BCM_E_NONE; 
}

STATIC void _bcm_tr3_regex_unlock(int unit)
{
    _bcm_tr3_regex_device_info_t *device;

    device = REGEX_INFO(unit);
    sal_mutex_give(device->regex_mutex);
}

#define BCM_REGEX_LOCK(u) _bcm_tr3_regex_lock((u))

#define BCM_REGEX_UNLOCK(u) _bcm_tr3_regex_unlock((u))


/********** db *******/

#define BCM_REGEX_DB_MATCH_BUCKET(matchidx) \
                            ((matchidx) % BCM_REGEX_DB_BUCKET_MAX)

#define BCM_REGEXDB_MATCH_SIG_ID            0x1
#define BCM_REGEXDB_MATCH_ENGINE_ID         0x2
#define BCM_REGEXDB_MATCH_MATCH_ID          0x4
#define BCM_REGEXDB_MATCH_PROVIDES          0x10
#define BCM_REGEXDB_MATCH_REQUIRES          0x20

STATIC _bcm_regexdb_entry_t**
_bcm_regexdb_find(int unit, uint32 match_flags, _bcm_regexdb_entry_t *a)
{
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regexdb_entry_t **p, **res =  NULL;
    int i, from, to, found = 0;
    uint32 tmpf = match_flags;

    if ((match_flags & BCM_REGEXDB_MATCH_SIG_ID)) {
        from = to = BCM_REGEX_DB_MATCH_BUCKET(a->sigid);
    } else {
        from = 0;
        to = BCM_REGEX_DB_BUCKET_MAX - 1;
    }

    device = REGEX_INFO(unit);

    for (i = from; !found && (i <= to); i++) {
        p = &device->l[i];
        while (*p) {
            tmpf = match_flags;
            if ((match_flags & BCM_REGEXDB_MATCH_SIG_ID) &&
                (a->sigid == (*p)->sigid)) {
                tmpf &= ~BCM_REGEXDB_MATCH_SIG_ID;
            }
            if ((match_flags & BCM_REGEXDB_MATCH_ENGINE_ID) &&
                (a->engid == (*p)->engid)) {
                tmpf &= ~BCM_REGEXDB_MATCH_ENGINE_ID;
            }
        #if 0
            if ((match_flags & BCM_REGEXDB_MATCH_MATCH_INDEX) &&
                (a->match_index == (*p)->match_index)) {
                tmpf &= ~BCM_REGEXDB_MATCH_MATCH_INDEX;
            }
        #endif
            if ((match_flags & BCM_REGEXDB_MATCH_MATCH_ID) &&
                (a->match_id == (*p)->match_id)) {
                tmpf &= ~BCM_REGEXDB_MATCH_MATCH_ID;
            }
            if ((match_flags & BCM_REGEXDB_MATCH_PROVIDES) &&
                (a->provides == (*p)->provides)) {
                tmpf &= ~BCM_REGEXDB_MATCH_PROVIDES;
            }
            if ((match_flags & BCM_REGEXDB_MATCH_REQUIRES) &&
                (a->requires == (*p)->requires)) {
                tmpf &= ~BCM_REGEXDB_MATCH_REQUIRES;
            }

            /* match */
            if (tmpf == 0) {
                found = 1;
                res = p;
                break;
            }
            p = &((*p)->next);
        }
    }

    return found ? res : NULL;
}

typedef int (*regexdb_traverse)(int unit, _bcm_regexdb_entry_t *e, void *ua);

STATIC int
_bcm_regexdb_traverse(int unit, uint32 match_flags, _bcm_regexdb_entry_t *a,
                      regexdb_traverse cbf, void *ua)
{
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regexdb_entry_t **p;
    int i, from, to, skip;

    if ((match_flags & BCM_REGEXDB_MATCH_SIG_ID)) {
        from = to = BCM_REGEX_DB_MATCH_BUCKET(a->sigid);
    } else {
        from = 0;
        to = BCM_REGEX_DB_BUCKET_MAX - 1;
    }
    
    device = REGEX_INFO(unit);

    for (i = from; i <= to; i++) {
        p = &device->l[i];
        while (*p) {
            skip = 0;
            if (!skip && (match_flags & BCM_REGEXDB_MATCH_SIG_ID) &&
                (a->sigid != (*p)->sigid)) {
                skip = 1;
            }
            if (!skip && (match_flags & BCM_REGEXDB_MATCH_ENGINE_ID) &&
                (a->engid != (*p)->engid)) {
                skip = 1;
            }
            #if 0
            if (!skip && (match_flags & BCM_REGEXDB_MATCH_MATCH_INDEX) &&
                (a->match_index != (*p)->match_index)) {
                skip = 1;
            }
            #endif
            if (!skip && (match_flags & BCM_REGEXDB_MATCH_MATCH_ID) &&
                (a->match_id != (*p)->match_id)) {
                skip = 1;
            }
            if (!skip && (match_flags & BCM_REGEXDB_MATCH_PROVIDES) &&
                (a->provides != (*p)->provides)) {
                skip = 1;
            }
            if (!skip && (match_flags & BCM_REGEXDB_MATCH_REQUIRES) &&
                (a->requires != (*p)->requires)) {
                skip = 1;
            }

            /* match */
            if (!skip) {
                cbf(unit, *p, ua);
            }
            p = &((*p)->next);
        }    
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_regexdb_delete_cmn(int unit, uint32 match_flags, _bcm_regexdb_entry_t *a)
{
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regexdb_entry_t **p, *pn;
    int i, from, to, del, num_del = 0;

    if ((match_flags & BCM_REGEXDB_MATCH_SIG_ID)) {
        from = to = BCM_REGEX_DB_MATCH_BUCKET(a->match_index);
    } else {
        from = 0;
        to = BCM_REGEX_DB_BUCKET_MAX - 1;
    }
    
    device = REGEX_INFO(unit);

    for (i = from; i <= to; i++) {
        p = &device->l[i];
        while (*p) {
            del = 1;
            if (del && (match_flags & BCM_REGEXDB_MATCH_SIG_ID) &&
                (a->sigid != (*p)->sigid)) {
                del = 0;
            }
            if (del && (match_flags & BCM_REGEXDB_MATCH_ENGINE_ID) &&
                (a->engid != (*p)->engid)) {
                del = 0;
            }
            #if 0
            if ((match_flags & BCM_REGEXDB_MATCH_MATCH_INDEX) &&
                (a->match_index != (*p)->match_index)) {
                del = 0;
            }
            #endif
            if (del && (match_flags & BCM_REGEXDB_MATCH_MATCH_ID) &&
                (a->match_id != (*p)->match_id)) {
                del = 0;
            }
            if (del & (match_flags & BCM_REGEXDB_MATCH_PROVIDES) &&
                (a->provides != (*p)->provides)) {
                del = 0;
            }
            if (del && (match_flags & BCM_REGEXDB_MATCH_REQUIRES) &&
                (a->requires != (*p)->requires)) {
                del = 0;
            }

            if (del) {
                pn = *p;
                *p = pn->next;
                sal_free(pn);
                num_del++;
            } else {
                p = &((*p)->next);
            }
        }    
    }
    return !num_del;
}

/*
 * Mark all the match entries that wre in pending state because on
 * unavilalibility of setid, to enable state.
 */
STATIC int
_bcm_regexdb_setid_avail_cb(int unit, _bcm_regexdb_entry_t *en, void *vua) 
{
    soc_mem_t mem;
    axp_sm_match_table_mem0_entry_t match_entry;
    _bcm_regexdb_cb_ua_t *ua = (_bcm_regexdb_cb_ua_t*) vua;

    mem = AXP_SM_MATCH_TABLE_MEM0m + BCM_REGEX_ENG2MATCH_MEMIDX(unit,en->engid);

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, 
                          MEM_BLOCK_ALL, en->match_index, &match_entry));
    if (soc_mem_field32_get(unit, mem, &match_entry, MATCH_ENTRY_VALIDf) == 0) {
        return BCM_E_NONE;
    }
    if (ua->avail) {
        soc_mem_field32_set(unit, mem, 
                        &match_entry, CHECK_CROSS_SIG_FLAG_IS_SETf, 1);
        soc_mem_field32_set(unit, mem, 
                        &match_entry, CHECK_CROSS_SIG_FLAG_INDEXf, ua->setid);
    } else {
        soc_mem_field32_set(unit, mem, 
                        &match_entry, CHECK_CROSS_SIG_FLAG_IS_SETf, 0);
        en->mchkid = BCM_TR3_SPECIAL_SLOT;
    }
    SOC_IF_ERROR_RETURN(soc_mem_write(unit, mem, 
                              MEM_BLOCK_ALL, en->match_index, &match_entry));
    return BCM_E_NONE;
}

STATIC int
_bcm_regexdb_handle_setid_avail(int unit, int avail, int engid,
                                int provides, int setid)
{
    _bcm_regexdb_entry_t en;
    _bcm_regexdb_cb_ua_t ua;

    en.requires = provides;
    ua.avail = avail;
    ua.setid = setid;

    return _bcm_regexdb_traverse(unit, BCM_REGEXDB_MATCH_REQUIRES, 
                                 &en, _bcm_regexdb_setid_avail_cb, (void*) &ua);
}

STATIC int
_bcm_regexdb_engine_unavail_cb(int unit, _bcm_regexdb_entry_t *en, void *vua) 
{ 
    _bcm_tr3_regex_device_info_t *device;
    axp_sm_match_table_mem0_entry_t match_entry;
    soc_mem_t mem;
    uint32 *alloc_bmap;

    device = REGEX_INFO(unit);

    if (en->msetid >= 0) {
        _bcm_regexdb_handle_setid_avail(unit, 0, en->engid, 
                                        en->provides, en->msetid);
    }

    /* delete the match index */
    mem = AXP_SM_MATCH_TABLE_MEM0m + BCM_REGEX_ENG2MATCH_MEMIDX(unit,en->engid);

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, 
                          MEM_BLOCK_ALL, en->match_index, &match_entry));
    soc_mem_field32_set(unit, mem, &match_entry, MATCH_ENTRY_VALIDf, 0);
    SOC_IF_ERROR_RETURN(soc_mem_write(unit, mem, 
                              MEM_BLOCK_ALL, en->match_index, &match_entry));

    /* clear the alloc bitmap */
    alloc_bmap = BCM_TR3_MATCH_BMAP_PTR(unit,device, en->engid);

    alloc_bmap[en->match_index/32] &= ~(1 << (en->match_index % 32));

    _bcm_tr3_free_sigid(unit, en->sigid);

    return BCM_E_NONE;
}

STATIC int
bcm_regexdb_find_chkid_for_requires(int unit, int requires, int *slot)
{
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_cps_obj_t *pcps;
    int i, j;

    device = REGEX_INFO(unit);
    for (i = 0; i < BCM_TR3_NUM_ENGINE(device); i++) {
        pcps = BCM_TR3_CPS_INFO(device, i);
        if (!BCM_REGEX_CPS_ALLOCED(pcps)) {
            continue;
        }

        for (j=0; j<BCM_REGEX_NUM_SLOT_PER_CPS; j++) {
            if (pcps->alloc_map[j] == requires) {
                *slot = pcps->foff + j;
                return BCM_E_NONE;
            }
        }
    }
    return BCM_E_NOT_FOUND;
}

STATIC _bcm_regexdb_entry_t*
bcm_regexdb_find_match_entry_by_sig_id(int unit, int sigid)
{
    _bcm_regexdb_entry_t e, **ppe;
    e.sigid = sigid;
    ppe = _bcm_regexdb_find(unit, BCM_REGEXDB_MATCH_SIG_ID, &e);
    return ppe ? *ppe : NULL;
}

STATIC _bcm_regexdb_entry_t*
bcm_regexdb_find_match_entry_by_match_id(int unit, int match_id)
{
    _bcm_regexdb_entry_t e, **ppe;
    e.match_id = match_id;
    ppe = _bcm_regexdb_find(unit, BCM_REGEXDB_MATCH_MATCH_ID, &e);
    return ppe ? *ppe : NULL;
}

#if 0
STATIC int
bcm_regexdb_delete_by_match_id(int unit, int match_id)
{
    _bcm_regexdb_entry_t en;
    en.match_id = match_id;

    return _bcm_regexdb_delete_cmn(unit, BCM_REGEXDB_MATCH_MATCH_ID, &en);
}
#endif

STATIC int
bcm_regexdb_delete_by_engine_id(int unit, int engid)
{
    _bcm_regexdb_entry_t en;
    en.engid = engid;
    return _bcm_regexdb_delete_cmn(unit, BCM_REGEXDB_MATCH_ENGINE_ID, &en);
}

STATIC int
bcm_regexdb_add_entry(int unit, int match_idx, int eng_idx, 
                      int match_id, int sigid, int provides, int requires,
                      int msetid, int mchkid)
{
    _bcm_regexdb_entry_t *en, **ppen;
    _bcm_tr3_regex_device_info_t *device;

    en = bcm_regexdb_find_match_entry_by_match_id(unit, match_id);
    if (en) {
        return BCM_E_EXISTS;
    }

    en = sal_alloc(sizeof(_bcm_regexdb_entry_t), "regx_db-entry");
    en->sigid = sigid;
    en->match_index = match_idx;
    en->match_id = match_id;
    en->provides = provides;
    en->requires = requires;
    en->msetid = msetid;
    en->mchkid = mchkid;
    en->engid = eng_idx;
    en->next = NULL;

    device = REGEX_INFO(unit);

    ppen = &device->l[BCM_REGEX_DB_MATCH_BUCKET(sigid)];
    while(*ppen) {
        ppen = &(*ppen)->next;
    }

    BCM_DEBUG(BCM_DBG_REGEX, 
                ("signature %d added (match_index=%d engid=%d provides=%d \
                  requires=%d msetid=%d mchkid=%d)\n",
                  sigid, match_idx, eng_idx, provides, requires, msetid, mchkid));

    *ppen = en;
    return 0;
}

/*
 * Compare the objects.
 */
STATIC int _bcm_regex_object_cmp_size(void*a, void*b)
{
    _bcm_regex_resource_t *oa, *ob;

    oa = (_bcm_regex_resource_t*)a;
    ob = (_bcm_regex_resource_t*)b;

    return ob->pres->total_lsz - oa->pres->total_lsz;
}

STATIC int _bcm_regex_object_cmp_offset(void*a, void*b)
{
    _bcm_regex_resource_t *oa, *ob;
    _bcm_regex_engine_obj_t *eng_a, *eng_b;

    oa = (_bcm_regex_resource_t*)a;
    ob = (_bcm_regex_resource_t*)b;

    eng_a = (_bcm_regex_engine_obj_t*)oa->pres;
    eng_b = (_bcm_regex_engine_obj_t*)ob->pres;

    if (eng_a->tile != eng_b->tile) {
        /* if not in same tile, return a big bumber */
        return (eng_a->tile - eng_b->tile)*0x072011;
    }

    return eng_a->from_line - eng_b->from_line;
}

STATIC int
_bcm_regex_sort_resources_by_size(int unit,
                                  _bcm_regex_resource_t *res, int num_res)
{
    _shr_sort(res, num_res, sizeof(_bcm_regex_resource_t), 
                        _bcm_regex_object_cmp_size);
    return BCM_E_NONE;
}

STATIC int
_bcm_regex_sort_resources_by_offset(int unit, 
                                  _bcm_regex_resource_t *res, int num_res)
{
    _shr_sort(res, num_res, sizeof(_bcm_regex_resource_t), 
                        _bcm_regex_object_cmp_offset);
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_make_resource_objs(int unit, _bcm_regex_resource_t **ppres,
                                    int *pnum_res, int tiles, int tilee)
{
    _bcm_tr3_regex_device_info_t *device;
    int i, j, nept, count;
    _bcm_regex_resource_t *pres;
    _bcm_regex_engine_obj_t *pengine = NULL;
    
    *ppres = NULL;
    *pnum_res = 0;

    device = REGEX_INFO(unit);

    nept = BCM_TR3_ENGINE_PER_TILE(device);

    for (count=0, i=tiles; i<=tilee; i++) {
        for (j=0; j<nept; j++) {
            pengine = BCM_TR3_PHYSICAL_ENGINE(device, (i*nept) + j);
            if (!BCM_REGEX_ENGINE_ALLOCATED(pengine)) {
                continue;
            }
            count++;
        }
    }

    if (count == 0) {
        return BCM_E_NONE;
    }
    
    pres = sal_alloc(sizeof(_bcm_regex_resource_t)*count, "res_objs");
    
    for (count=0, i=tiles; i<=tilee; i++) {
        for (j=0; j<nept; j++) {
            pengine = BCM_TR3_PHYSICAL_ENGINE(device, (i*nept) + j);
            if (!BCM_REGEX_ENGINE_ALLOCATED(pengine)) {
                continue;
            }
            pres[count].pres = pengine;
            pres[count].eng_idx = pengine->hw_idx;
            pres[count].lsize = pengine->total_lsz;
            count++;
        }
    }

    *pnum_res = count;
    *ppres = pres;

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_free_resource_objs(int unit, _bcm_regex_resource_t **ppres, 
                            int *num_res)
{
    if (*ppres) {
        sal_free(*ppres);
        *ppres = NULL;
        *num_res = 0;
    }
    return BCM_E_NONE;
}

static int _encode_max_flow(int max_flow)
{
    int enc;

    if (max_flow <= 1024) {
        enc = 0;
    } else if (max_flow <= 2048) {
        enc = 1;
    } else if (max_flow <= 4096) {
        enc = 2;
    } else if (max_flow <= 8192) {
        enc = 3;
    } else {
        enc = -1;
    }
    return enc;
}

static int _decode_max_flow(int hw_enc)
{
    int max_flow;

    if (hw_enc == 0) {
        max_flow = 1024;
    } else if (hw_enc == 1) {
        max_flow = 2048;
    } else if (hw_enc == 2) {
        max_flow = 4096;
    } else if (hw_enc == 3) {
        max_flow = 8192;
    } else {
        max_flow = -1;
    }
    return max_flow;
}

typedef enum _bcm_tr3_ctr_mode_e {
    _CTR_MODE_NONE  = 0,
    _CTR_MODE_PER_LEG,
    _CTR_MODE_BOTH_LEG
} _bcm_tr3_ctr_mode_t;

STATIC int
_bcm_tr3_ctr_mode_set(int unit, _bcm_tr3_ctr_mode_t mode)
{
    uint32 regval, fval;

    SOC_IF_ERROR_RETURN(READ_FT_CONFIGr(unit, &regval));
    switch (mode) {
        case _CTR_MODE_NONE:
            fval = 0;
        break;
        case _CTR_MODE_BOTH_LEG:
            fval = 1;
        break;
        case _CTR_MODE_PER_LEG:
            fval = 2;
        break;
        default:
            return BCM_E_PARAM;
    }

    soc_reg_field_set(unit, FT_CONFIGr, &regval, CNT_MODEf, fval);
    SOC_IF_ERROR_RETURN(WRITE_FT_CONFIGr(unit, regval));
    
    return BCM_E_NONE;
}

STATIC 
_bcm_tr3_ctr_mode_t _bcm_tr3_ctr_mode_get(int unit)
{
    uint32 regval, fval;

    SOC_IF_ERROR_RETURN(READ_FT_CONFIGr(unit, &regval));
    fval = soc_reg_field_get(unit, FT_CONFIGr, regval, CNT_MODEf);
    
    switch (fval) {
        case 1:
            return _CTR_MODE_BOTH_LEG;
        case 2:
            return _CTR_MODE_PER_LEG;
        default:
        case 0:
            return _CTR_MODE_NONE;
    }
}

STATIC int _bcm_tr3_flex_ctr_pool_map(int unit, _bcm_tr3_ctr_mode_t mode)
{
    _bcm_tr3_regex_device_info_t *device;
    int dir, rgn, poff;
    uint64 regval, fval;
    soc_field_t poolf[] = { FWD_00f, FWD_01f, FWD_10f, FWD_11f, 
                            REV_00f,  REV_01f, REV_10f, REV_11f };

    if ((device = REGEX_INFO(unit)) == NULL) {
        return BCM_E_INIT;
    }

    if (mode == _CTR_MODE_NONE) {
        return BCM_E_NONE;
    }

    SOC_IF_ERROR_RETURN(READ_FT_CTR_POOLr(unit, &regval));

    /* Alloc pools */
    for (dir = 0; dir < ((mode == _CTR_MODE_PER_LEG) ? 2 : 1); dir++) {
        device->flex_pools[dir].flags = BCM_FLEX_ALLOCATE_POOL;
        device->flex_pools[dir].pool_id = dir;
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_pool_operation(unit, 
                                          &device->flex_pools[dir]));
    }

    for (rgn = 0; rgn < 8; rgn++) {
        poff = (mode == _CTR_MODE_PER_LEG) ? rgn/4 : 0;
        COMPILER_64_SET(fval, 0, device->flex_pools[poff].pool_id);
        soc_reg64_field_set(unit, FT_CTR_POOLr, &regval, poolf[rgn], fval);
    }
    
    SOC_IF_ERROR_RETURN(WRITE_FT_CTR_POOLr(unit, regval));

    return BCM_E_NONE;
}

STATIC int _bcm_tr3_flex_ctr_pool_unmap(int unit)
{
    _bcm_tr3_regex_device_info_t *device;
    int i;

    if ((device = REGEX_INFO(unit)) == NULL) {
        return BCM_E_INIT;
    }

    for (i = 0; i < 2; i++) {
        if (!_BCM_TR3_FLEX_CTR_POOL_VALID(device->flex_pools[i].pool_id)) {
            continue;
        }

        device->flex_pools[i].flags = BCM_FLEX_ALLOCATE_POOL;
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_pool_operation(unit, 
                                          &device->flex_pools[i]));
        device->flex_pools[i].pool_id = -1;
    }
    
    return BCM_E_NONE;
}

int 
_bcm_tr3_regex_config_set(int unit, bcm_regex_config_t *config)
{
    int max_flows, payload_depth, inspect_num_pkt;
    _bcm_tr3_regex_device_info_t *device;
    _bcm_tr3_ctr_mode_t ctr_mode;
    uint32 field_val;
    uint32 regval;

    if ((device = REGEX_INFO(unit)) == NULL) {
        return BCM_E_INIT;
    }

    if (!config) {
        return BCM_E_PARAM;
    }

    /*
     * Adjust encodings.
     */
    max_flows = ((config->max_flows < 0) || 
                 (_encode_max_flow(config->max_flows) == -1)) ? 
                BCM_TR3_MAX_FLOWS : config->max_flows;

    payload_depth = (config->payload_depth <= 0) ?
                BCM_TR3_MAX_PAYLOAD_DEPTH : config->payload_depth;

    inspect_num_pkt = (config->inspect_num_pkt <= 0) ?
                BCM_TR3_MAX_PKT_TO_SME : config->inspect_num_pkt;

    /* validate params */
    if ((config->max_flows > BCM_TR3_MAX_FLOWS) ||
        (config->payload_depth > BCM_TR3_MAX_PAYLOAD_DEPTH) ||
        (config->inspect_num_pkt > BCM_TR3_MAX_PKT_TO_SME)) {
        return BCM_E_PARAM;
    }

    /* set MACSA/DA */
    if (config->flags & BCM_REGEX_CONFIG_ENABLE) {
        field_val = (config->dst_mac[0] << 8) |    /* MAC-DA[47:40] */
                    (config->dst_mac[1]);          /* MAC-DA[39:32] */
        SOC_IF_ERROR_RETURN(READ_FT_RESULT_DA_MSr(unit, &regval));
        soc_reg_field_set(unit, FT_RESULT_DA_MSr, &regval, DAf, field_val);
        SOC_IF_ERROR_RETURN(WRITE_FT_RESULT_DA_MSr(unit, regval));

        field_val = (config->dst_mac[2] << 24) |    /* MAC-DA[31:24] */
                    (config->dst_mac[3] << 16) |    /* MAC-DA[23:16] */
                    (config->dst_mac[4] << 8) |     /* MAC-DA[15:8] */
                    (config->dst_mac[5]);           /* MAC-DA[7:0] */

        regval = 0;
        soc_reg_field_set(unit, FT_RESULT_DA_LSr, &regval, DAf, field_val);
        SOC_IF_ERROR_RETURN(WRITE_FT_RESULT_DA_LSr(unit, regval));

        regval = 0;
        field_val = (config->dst_mac[0] << 24) |    /* MAC-DA[47:40] */
                    (config->dst_mac[1] << 16) |    /* MAC-DA[39:32] */
                    (config->dst_mac[2] << 8) |     /* MAC-DA[31:24] */
                    (config->dst_mac[3]);           /* MAC-DA[23:16] */
        soc_reg_field_set(unit, AXP_SM_REPORT_PACKET_CONTROL0r, &regval, 
                            UPPER_MACDAf, field_val);
        SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REPORT_PACKET_CONTROL0r(unit, regval));

        regval = 0;
        field_val = (config->dst_mac[4] << 8) | (config->dst_mac[5]);
        soc_reg_field_set(unit, AXP_SM_REPORT_PACKET_CONTROL1r, &regval, 
                            LOWER_MACDAf, field_val);
        field_val = (config->src_mac[0] << 8) | (config->src_mac[1]);
        soc_reg_field_set(unit, AXP_SM_REPORT_PACKET_CONTROL1r, &regval, 
                            UPPER_MACSAf, field_val);
        SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REPORT_PACKET_CONTROL1r(unit, regval));

        regval = 0;
        field_val = ((config->src_mac[2] <<24) | 
                     (config->src_mac[3] << 16) |
                     (config->src_mac[4] << 8) |
                     (config->src_mac[5]));
        soc_reg_field_set(unit, AXP_SM_REPORT_PACKET_CONTROL2r, &regval, 
                            LOWER_MACSAf, field_val);
        SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REPORT_PACKET_CONTROL2r(unit, regval));

        regval = 0;
        soc_reg_field_set(unit, FT_RESULT_LENGTH_TYPEr, &regval, ENABLEf, 1);
        soc_reg_field_set(unit, FT_RESULT_LENGTH_TYPEr, &regval, 
                          LENGTH_TYPEf, config->ethertype);
        SOC_IF_ERROR_RETURN(WRITE_FT_RESULT_LENGTH_TYPEr(unit, regval));

        regval = 0;
        soc_reg_field_set(unit, AXP_SM_REPORT_PACKET_CONTROL3r, &regval, 
                          VLANf, 1);
        soc_reg_field_set(unit, AXP_SM_REPORT_PACKET_CONTROL3r, &regval, 
                          ETHER_TYPEf, config->ethertype);
        SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REPORT_PACKET_CONTROL3r(unit, regval));
    }

    /* Set Reports */
    SOC_IF_ERROR_RETURN(READ_FT_EVENT_CONFIGr(unit, &regval));
    if (config->flags & BCM_REGEX_CONFIG_ENABLE) {
        soc_reg_field_set(unit, FT_EVENT_CONFIGr, &regval, NEW_FLOWf, 
                ((config->report_flags & BCM_REGEX_REPORT_NEW) ? 1 : 0));
        soc_reg_field_set(unit, FT_EVENT_CONFIGr, &regval, RESf,
            ((config->report_flags & BCM_REGEX_REPORT_MATCHED) ? 1 : 0));
        soc_reg_field_set(unit, FT_EVENT_CONFIGr, &regval, RETIREDf,
            ((config->report_flags & BCM_REGEX_REPORT_END) ? 1 : 0));
    } else {
        soc_reg_field_set(unit, FT_EVENT_CONFIGr, &regval, NEW_FLOWf, 0);
        soc_reg_field_set(unit, FT_EVENT_CONFIGr, &regval, RESf, 0);
        soc_reg_field_set(unit, FT_EVENT_CONFIGr, &regval, RETIREDf, 0);
    }
    SOC_IF_ERROR_RETURN(WRITE_FT_EVENT_CONFIGr(unit, regval));
   
    SOC_IF_ERROR_RETURN(READ_FT_AGE_CONTROLr(unit,&regval));
    if (config->flags & BCM_REGEX_CONFIG_AGING) {
        soc_reg_field_set(unit, FT_AGE_CONTROLr, &regval, AGE_ENABLEf, 
                    ((config->inactivity_timeout_usec >= 0) ? 1 : 0));
    }
    soc_reg_field_set(unit, FT_AGE_CONTROLr, &regval, INDEXf, 0);
    SOC_IF_ERROR_RETURN(WRITE_FT_AGE_CONTROLr(unit, regval));

    SOC_IF_ERROR_RETURN(READ_FT_CONFIGr(unit,&regval));
    soc_reg_field_set(unit, FT_CONFIGr, &regval, MAX_BYTESf, 
                                                     payload_depth - 1);
    soc_reg_field_set(unit, FT_CONFIGr, &regval, MAX_PKTSf, 
                                                    inspect_num_pkt - 1);
    soc_reg_field_set(unit, FT_CONFIGr, &regval, TABLE_SIZEf, 
                                        _encode_max_flow(max_flows));

    soc_reg_field_set(unit, FT_CONFIGr, &regval, INT_SME_ENABLEf, 1);
    soc_reg_field_set(unit, FT_CONFIGr, &regval, EXT_SME_ENABLEf, 0);

    soc_reg_field_set(unit, FT_CONFIGr, &regval, FT_DISABLEf, 
                (config->flags & BCM_REGEX_CONFIG_ENABLE) ? 0 : 1);

    soc_reg_field_set(unit, FT_CONFIGr, &regval, TCP_END_DETECTf, 
          (config->flags & BCM_REGEX_CONFIG_TCP_SESSION_DETECT) ? 1 : 0);

    soc_reg_field_set(unit, FT_CONFIGr, &regval, NORMALIZEf, 1);

    soc_reg_field_set(unit, FT_CONFIGr, &regval, IPV4_CREATE_ENf,
                    (config->flags & BCM_REGEX_CONFIG_IP4) ? 1 : 0);

    soc_reg_field_set(unit, FT_CONFIGr, &regval, IPV6_CREATE_ENf,
                        (config->flags & BCM_REGEX_CONFIG_IP6) ? 1 : 0);

    SOC_IF_ERROR_RETURN(WRITE_FT_CONFIGr(unit,regval));

    field_val = (config->flags & BCM_REGEX_CONFIG_ENABLE) ? 1 : 0;
    SOC_IF_ERROR_RETURN(
                READ_AXP_SM_SIGNATURE_MATCH_CONTROLr(unit, &regval));
    soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                                             &regval, ENABLEf, field_val);
    soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                                                &regval, RESETf, 0);
    if (field_val) {
        soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                          &regval, STATE_TABLE_ACCESS_LIMIT_ENABLEf, 1);
        soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                         &regval, DROP_ON_L4_CHECKSUM_ERROR_ENABLEf, 1);
        soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                       &regval, DROP_ON_PACKET_LENGTH_ERROR_ENABLEf, 1);
        soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                      &regval, DROP_ON_FLOW_TIMESTAMP_ERROR_ENABLEf, 1);
        soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                                &regval, MAX_BYTE_INSPECTED_ENABLEf, 1);
        soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                        &regval, DROP_ON_FLOW_PKT_NUM_ERROR_ENABLEf, 1);
        soc_reg_field_set(unit, AXP_SM_SIGNATURE_MATCH_CONTROLr, 
                      &regval, DROP_ON_FLOW_TABLE_ECC_ERROR_ENABLEf, 0);
    }
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_SIGNATURE_MATCH_CONTROLr(unit, 
                                                            regval));

    SOC_IF_ERROR_RETURN(READ_AUX_ARB_CONTROL_2r(unit, &regval));
    soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &regval, 
                            FT_REFRESH_ENABLEf, field_val);
    SOC_IF_ERROR_RETURN(WRITE_AUX_ARB_CONTROL_2r(unit, regval));


    ctr_mode = _bcm_tr3_ctr_mode_get(unit);
    if (config->flags & (BCM_REGEX_COUNTER_ENABLE | 
                         BCM_REGEX_PER_DIRECTION_COUNTER_ENABLE)) {
        if ((ctr_mode != _CTR_MODE_NONE) &&
            (((config->flags & BCM_REGEX_PER_DIRECTION_COUNTER_ENABLE) &&
              (ctr_mode != _CTR_MODE_PER_LEG)) || 
             (ctr_mode != _CTR_MODE_BOTH_LEG))) {
            _bcm_tr3_ctr_mode_set(unit, _CTR_MODE_NONE);
            _bcm_tr3_flex_ctr_pool_unmap(unit);
        }
        ctr_mode = (config->flags & BCM_REGEX_PER_DIRECTION_COUNTER_ENABLE) ? 
                                    _CTR_MODE_PER_LEG : _CTR_MODE_BOTH_LEG;
        _bcm_tr3_flex_ctr_pool_map(unit, ctr_mode);
        _bcm_tr3_ctr_mode_set(unit, ctr_mode);
    } else {
        /* free up the pools */
        _bcm_tr3_ctr_mode_set(unit, _CTR_MODE_NONE);
        _bcm_tr3_flex_ctr_pool_unmap(unit);
    }

    return BCM_E_NONE;
}

int 
_bcm_tr3_regex_config_get(int unit, bcm_regex_config_t *config)
{
    _bcm_tr3_ctr_mode_t ctr_mode;
    uint32 field_val;
    uint32 regval;

    if (!config) {
        return BCM_E_PARAM;
    }

    /* Set Reports */
    SOC_IF_ERROR_RETURN(READ_FT_EVENT_CONFIGr(unit, &regval));
    if (soc_reg_field_get(unit, FT_EVENT_CONFIGr, regval, NEW_FLOWf)) {
        config->report_flags |= BCM_REGEX_REPORT_NEW;
    }
    if (soc_reg_field_get(unit, FT_EVENT_CONFIGr, regval, RESf)) {
        config->report_flags |= BCM_REGEX_REPORT_MATCHED;
    }
    if (soc_reg_field_get(unit, FT_EVENT_CONFIGr, regval, RETIREDf)) {
        config->report_flags |= BCM_REGEX_REPORT_END;
    }
    

    SOC_IF_ERROR_RETURN(READ_FT_CONFIGr(unit,&regval));
    config->payload_depth = 
            soc_reg_field_get(unit, FT_CONFIGr, regval, MAX_BYTESf) + 1;
    config->inspect_num_pkt = 
            soc_reg_field_get(unit, FT_CONFIGr, regval, MAX_PKTSf) + 1;
    config->max_flows = _decode_max_flow(soc_reg_field_get(unit, 
                                        FT_CONFIGr, regval, TABLE_SIZEf));

    config->flags = 0;
    if (soc_reg_field_get(unit, FT_CONFIGr, regval, FT_DISABLEf) == 0) {
        config->flags |= BCM_REGEX_CONFIG_ENABLE;
    }

    if (soc_reg_field_get(unit, FT_CONFIGr, regval, TCP_END_DETECTf)) {
        config->flags |= BCM_REGEX_CONFIG_TCP_SESSION_DETECT;
    }

    if (soc_reg_field_get(unit, FT_CONFIGr, regval, IPV4_CREATE_ENf)) {
        config->flags |= BCM_REGEX_CONFIG_IP4;
    }

    if (soc_reg_field_get(unit, FT_CONFIGr, regval, IPV6_CREATE_ENf)) {
        config->flags |= BCM_REGEX_CONFIG_IP6;
    }

    /* get MACSA/DA */
    SOC_IF_ERROR_RETURN(READ_AXP_SM_REPORT_PACKET_CONTROL0r(unit, &regval));
    field_val = soc_reg_field_get(unit, AXP_SM_REPORT_PACKET_CONTROL0r, 
                                    regval, UPPER_MACDAf);
    config->dst_mac[0] = ((field_val >> 24) & 0xff);
    config->dst_mac[1] = ((field_val >> 16) & 0xff);
    config->dst_mac[2] = ((field_val >> 8) & 0xff);
    config->dst_mac[3] = (field_val & 0xff);

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REPORT_PACKET_CONTROL2r(unit, &regval));
    field_val = soc_reg_field_get(unit, AXP_SM_REPORT_PACKET_CONTROL2r, 
                                  regval, LOWER_MACSAf);
    config->src_mac[2] = ((field_val >> 24) & 0xff);
    config->src_mac[3] = ((field_val >> 16) & 0xff);
    config->src_mac[4] = ((field_val >> 8) & 0xff);
    config->src_mac[5] = (field_val & 0xff);

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REPORT_PACKET_CONTROL1r(unit, &regval));
    field_val = soc_reg_field_get(unit, AXP_SM_REPORT_PACKET_CONTROL1r, regval, 
                                  UPPER_MACSAf);
    config->src_mac[0] = ((field_val >> 8) & 0xff);
    config->src_mac[1] = (field_val & 0xff);

    field_val = soc_reg_field_get(unit, AXP_SM_REPORT_PACKET_CONTROL1r, 
                                 regval, LOWER_MACDAf);
    config->dst_mac[4] = ((field_val >> 8) & 0xff);
    config->dst_mac[5] = (field_val & 0xff);

    SOC_IF_ERROR_RETURN(READ_FT_RESULT_LENGTH_TYPEr(unit, &regval));
    config->ethertype = soc_reg_field_get(unit, 
                     FT_RESULT_LENGTH_TYPEr, regval, LENGTH_TYPEf);

    ctr_mode = _bcm_tr3_ctr_mode_get(unit);
    if (ctr_mode == _CTR_MODE_PER_LEG) {
        config->flags |= BCM_REGEX_PER_DIRECTION_COUNTER_ENABLE;
    } else if (ctr_mode == _CTR_MODE_BOTH_LEG) {
        config->flags |= BCM_REGEX_COUNTER_ENABLE;
    }
    return BCM_E_NONE;
}

int _bcm_tr3_regex_exclude_delete_all(int unit)
{
    int rv;

    BCM_REGEX_LOCK(unit);
    rv = soc_mem_clear(unit, FT_L4PORTm, COPYNO_ALL, TRUE);
    BCM_REGEX_UNLOCK(unit);
    return rv;
}

#define BCM_TR3_PROTOCOL_TCP        6
#define BCM_TR3_PROTOCOL_UDP        17

STATIC
int _bcm_tr3_valid_protocol(int unit, uint16 protocol)
{
    if ((protocol == BCM_TR3_PROTOCOL_TCP) || 
        (protocol == BCM_TR3_PROTOCOL_UDP)) {
        return BCM_E_NONE;
    }
    return BCM_E_UNAVAIL;
}

#define BCM_TR3_EXCLUDE_OP_INSERT   0
#define BCM_TR3_EXCLUDE_OP_DELETE   1

#define BCM_TR3_NUM_PROCOL          2

#define BCM_TR3_MAX_EXCLUDE_RANGE   8

typedef struct _bcm_tr3_l4_range_s {
    uint16 start;
    uint16 end;
    int    tcp;
    int    any;
} _bcm_tr3_l4_range_t;

static int
_bcm_tr3_regex_sort_ranges(void *a, void *b)
{
    _bcm_tr3_l4_range_t *ia, *ib;
    ia = (_bcm_tr3_l4_range_t*) a;
    ib = (_bcm_tr3_l4_range_t*) b;
    return ia->start - ib->start; 
}

#define _TR3_REGEX_E_RANGE(s,t,a,b)     \
            ((s))->tcp = (t);             \
            ((s))->any = 0;               \
            ((s))->start = (a);           \
            ((s))->end = (b);

#define _TR3_REGEX_E_RANGE_START(r)     (r)->start
#define _TR3_REGEX_E_RANGE_END(r)     (r)->end
#define _TR3_REGEX_E_RANGE_PROTOCOL(r)     (r)->tcp
                

STATIC int
_bcm_tr3_merge_ranges(_bcm_tr3_l4_range_t *range, int *count)
{
    int in_cnt = *count, i, j, o_cnt = in_cnt, pr1, pr2;

    _shr_sort(range, in_cnt, sizeof(_bcm_tr3_l4_range_t), 
                             _bcm_tr3_regex_sort_ranges);

    for (i = 0; i < o_cnt - 1;) {
        pr1 = _TR3_REGEX_E_RANGE_PROTOCOL(&range[i]);
        pr2 = _TR3_REGEX_E_RANGE_PROTOCOL(&range[i + 1]);

        if ((_TR3_REGEX_E_RANGE_START(&range[i]) == 
                            _TR3_REGEX_E_RANGE_START(&range[i+1])) &&
            (_TR3_REGEX_E_RANGE_END(&range[i]) == 
                            _TR3_REGEX_E_RANGE_END(&range[i+1]))) {
            if (pr1 != pr2) {
                range[i].any = 1;
            }

            o_cnt -= 1;
            for (j = i+1; j < o_cnt; j++) {
                range[j] = range[j+1];
            }
        } else if (pr1 != pr2) {
            continue;
        } else if (_TR3_REGEX_E_RANGE_END(&range[i]) >= 
                            _TR3_REGEX_E_RANGE_START(&range[i+1])) {
            if (_TR3_REGEX_E_RANGE_END(&range[i+1]) > 
                            _TR3_REGEX_E_RANGE_END(&range[i])) {
                _TR3_REGEX_E_RANGE(&range[i],
                                   _TR3_REGEX_E_RANGE_PROTOCOL(&range[i]),
                                   _TR3_REGEX_E_RANGE_START(&range[i]),
                                   _TR3_REGEX_E_RANGE_END(&range[i+1]));
            } else {
                _TR3_REGEX_E_RANGE(&range[i],
                                   _TR3_REGEX_E_RANGE_PROTOCOL(&range[i]),
                                   _TR3_REGEX_E_RANGE_START(&range[i]),
                                   _TR3_REGEX_E_RANGE_END(&range[i]));
            }

            o_cnt -= 1;
            for (j = i+1; j < o_cnt; j++) {
                range[j] = range[j+1];
            }
        } else {
            i++;
        }
    }
    *count = o_cnt;
    return 0;
}

STATIC
int _bcm_tr3_exclude_op(int unit, int op, int tcp,
                        uint16 l4start, uint16 l4end)
{
    _bcm_tr3_l4_range_t *tcomp, *ucomp, *fcomp, *prange;
    uint16 this_start, this_end, num_te;
    int     rv = BCM_E_NONE, *pnum;
    int     i, j, tcpi=0, udpi = 0, num_entry=0;
    ft_l4port_entry_t   ftl4m;

    num_entry = soc_mem_index_count(unit, FT_L4PORTm);
    num_te = (num_entry * 2) + 1;
    tcomp = sal_alloc(sizeof(_bcm_tr3_l4_range_t) * num_te, "sortee");
    ucomp = sal_alloc(sizeof(_bcm_tr3_l4_range_t) * num_te, "sorteeu");
    fcomp = sal_alloc(sizeof(_bcm_tr3_l4_range_t) * 2 * num_te, "sorteef");

    /* read all the entries so that we can rebalnace the entries ie
     * merge/split based on ranges. */
    for (i=0; i < num_entry; i++) {
        if (READ_FT_L4PORTm(unit, MEM_BLOCK_ANY, i, &ftl4m)) {
            goto done;
        }

        if (soc_mem_field32_get(unit, FT_L4PORTm, &ftl4m, VALIDf) == 0) {
            break;
        }
        
        this_start = soc_mem_field32_get(unit, FT_L4PORTm, 
                                         &ftl4m, L4_PORT_LOWERf);
        this_end = soc_mem_field32_get(unit, FT_L4PORTm, 
                                         &ftl4m, L4_PORT_UPPERf);

        if (soc_mem_field32_get(unit, FT_L4PORTm, &ftl4m, TCPf)) {
            _TR3_REGEX_E_RANGE(&tcomp[tcpi], 1, this_start, this_end);
            tcpi++;
        }
        if (soc_mem_field32_get(unit, FT_L4PORTm, &ftl4m, UDPf)) {
            _TR3_REGEX_E_RANGE(&ucomp[udpi], 0, this_start, this_end);
            udpi++;
        }
    }

    if (tcp) {
        prange = tcomp;
        pnum = &tcpi;
    } else {
        prange = ucomp;
        pnum = &udpi;
    }

    if (op == BCM_TR3_EXCLUDE_OP_INSERT) {
        /* insert the new range at the end */
        _TR3_REGEX_E_RANGE(&prange[*pnum], tcp, l4start, l4end);
        *pnum += 1;
    }

    _bcm_tr3_merge_ranges(tcomp, &tcpi);
    _bcm_tr3_merge_ranges(ucomp, &udpi);


    if (op == BCM_TR3_EXCLUDE_OP_DELETE) {
        for (i = 0; i < *pnum; i++) {
            if ((l4start <= _TR3_REGEX_E_RANGE_START(&prange[i])) &&
                (l4end >= _TR3_REGEX_E_RANGE_START(&prange[i]))) {
                for (j = i+1; j < *pnum; j++) {
                    prange[j - 1] = prange[j];
                }
                *pnum -= 1;
                i -= 1;
            } else if ((l4start > _TR3_REGEX_E_RANGE_START(&prange[i])) &&
                       (l4end >= _TR3_REGEX_E_RANGE_END(&prange[i]))) {
                _TR3_REGEX_E_RANGE(&prange[i], tcp, 
                    _TR3_REGEX_E_RANGE_START(&prange[i]), l4start - 1);
            } else if ((l4end >= _TR3_REGEX_E_RANGE_START(&prange[i])) &&
                       (l4end < _TR3_REGEX_E_RANGE_END(&prange[i]))) {
                _TR3_REGEX_E_RANGE(&prange[i], tcp, l4end + 1,
                                    _TR3_REGEX_E_RANGE_END(&prange[i]));
            } else if ((l4start > _TR3_REGEX_E_RANGE_START(&prange[i])) &&
                       (l4end < _TR3_REGEX_E_RANGE_END(&prange[i]))) {
                /* make space */
                for (j = *pnum - 1; j > i; j--) {
                    prange[j+1] = prange[j];
                }
                _TR3_REGEX_E_RANGE(&prange[i+1], tcp, l4end + 1, 
                                _TR3_REGEX_E_RANGE_END(&prange[i]));
                _TR3_REGEX_E_RANGE(&prange[i], tcp, 
                        _TR3_REGEX_E_RANGE_START(&prange[i]), l4start - 1);

                *pnum += 1;
                i += 1;
            }
        }
    }

    /* merge tcp and  udp entries */

    if ((tcpi + udpi) == 0) {
        rv = _bcm_tr3_regex_exclude_delete_all(unit);
        goto done;
    }

    j = 0;
    for (i = 0; i < tcpi; i++) {
        fcomp[j++] = tcomp[i];
    }

    for (i = 0; i < udpi; i++) {
        fcomp[j++] = ucomp[i];
    }
    
    _bcm_tr3_merge_ranges(fcomp, &j);
    if (j > num_entry) {
        return BCM_E_RESOURCE;
    }

    for (i = 0; i < j; i++) {
        sal_memset(&ftl4m, 0, sizeof(ft_l4port_entry_t));
        soc_mem_field32_set(unit, FT_L4PORTm, &ftl4m, VALIDf, 1);
        soc_mem_field32_set(unit, FT_L4PORTm, &ftl4m, 
               L4_PORT_LOWERf, _TR3_REGEX_E_RANGE_START(&fcomp[i]));
        soc_mem_field32_set(unit, FT_L4PORTm, &ftl4m, 
               L4_PORT_UPPERf, _TR3_REGEX_E_RANGE_END(&fcomp[i]));

        if (fcomp[i].any == 1) {
            soc_mem_field32_set(unit, FT_L4PORTm, &ftl4m, TCPf, 1);
            soc_mem_field32_set(unit, FT_L4PORTm, &ftl4m, UDPf, 1);
        } else if (_TR3_REGEX_E_RANGE_PROTOCOL(&fcomp[i])) {
            soc_mem_field32_set(unit, FT_L4PORTm, &ftl4m, TCPf, 1);
        } else {
            soc_mem_field32_set(unit, FT_L4PORTm, &ftl4m, UDPf, 1);
        }
        rv = WRITE_FT_L4PORTm(unit, MEM_BLOCK_ANY, i, &ftl4m);
        if (BCM_FAILURE(rv)) {
            goto done;
        }
    }

    for (; i < num_entry; i++) {
        sal_memset(&ftl4m, 0, sizeof(ft_l4port_entry_t));
        rv = WRITE_FT_L4PORTm(unit, MEM_BLOCK_ANY, i, &ftl4m);
        if (BCM_FAILURE(rv)) {
            goto done;
        }
    }

done:
    sal_free(tcomp);
    sal_free(ucomp);
    sal_free(fcomp);

    return rv;
}

int _bcm_tr3_regex_exclude_add(int unit, uint8 protocol, 
                              uint16 l4_start, uint16 l4_end)
{
    int rv;
    if (_bcm_tr3_valid_protocol(unit, protocol)) {
        return BCM_E_PARAM;
    }
    
    BCM_REGEX_LOCK(unit);
    rv = _bcm_tr3_exclude_op(unit, BCM_TR3_EXCLUDE_OP_INSERT, 
                        (protocol == BCM_TR3_PROTOCOL_TCP) ? 1 : 0,
                        l4_start, l4_end);
    BCM_REGEX_UNLOCK(unit);
    return rv;
}

int _bcm_tr3_regex_exclude_delete(int unit, uint8 protocol, 
                              uint16 l4_start, uint16 l4_end)
{
    int rv;

    if (_bcm_tr3_valid_protocol(unit, protocol)) {
        return BCM_E_PARAM;
    }
    
    BCM_REGEX_LOCK(unit);
    rv = _bcm_tr3_exclude_op(unit, BCM_TR3_EXCLUDE_OP_DELETE, 
                        (protocol == BCM_TR3_PROTOCOL_TCP) ? 1 : 0,
                        l4_start, l4_end);
    BCM_REGEX_UNLOCK(unit);
    return rv;
}

int _bcm_tr3_regex_exclude_get(int unit, int array_size, uint8 *protocol, 
                        uint16 *l4low, uint16 *l4high, int *array_count)
{
    int i, valid, nume = 0;
    uint16  this_start, this_end;
    ft_l4port_entry_t   ftl4m;

    *array_count = 0;

    BCM_REGEX_LOCK(unit);
    for (i=0; (i < soc_mem_index_count(unit, FT_L4PORTm)) && 
                                  (nume < array_size); i++) {
        SOC_IF_ERROR_RETURN(READ_FT_L4PORTm(unit, MEM_BLOCK_ANY, i, &ftl4m));
        valid = soc_mem_field32_get(unit, FT_L4PORTm, &ftl4m, VALIDf);
        if (!valid) {
            continue;
        }
        
        this_start = soc_mem_field32_get(unit, FT_L4PORTm, 
                                         &ftl4m, L4_PORT_LOWERf);
        this_end = soc_mem_field32_get(unit, FT_L4PORTm, 
                                         &ftl4m, L4_PORT_UPPERf);

        if (soc_mem_field32_get(unit, FT_L4PORTm, &ftl4m, TCPf)) {
            l4low[nume] = this_start;
            l4high[nume] = this_end;
            protocol[nume] = BCM_TR3_PROTOCOL_TCP;
            nume++;
            if (nume == array_size) {
                break;
            }
        }
        if (soc_mem_field32_get(unit, FT_L4PORTm, &ftl4m, UDPf)) {
            l4low[nume] = this_start;
            l4high[nume] = this_end;
            protocol[nume] = BCM_TR3_PROTOCOL_UDP;
            nume++;
            if (nume == array_size) {
                break;
            }
        }
    }

    BCM_REGEX_UNLOCK(unit);

    *array_count = nume;
    return BCM_E_NONE;
}

/* remap tables */
STATIC uint8 *cmap_lower_tbl = NULL;

/* tr3 engine */
STATIC int _bcm_tr3_engine_reset(int unit, int eng_idx)
{
    uint32  regval;

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, &regval));

    /* reset the regex engines and clear out the state memories. */
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, 
                      &regval, REGEX_RESETf, 1);
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, 
                      &regval, REGEX_ENABLEf, 0);
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, regval));
    return BCM_E_NONE;
}

/* CHECK : this just returns start of dfa , change name */
STATIC int _bcm_tr3_get_engine_info(int unit, int eng_idx, 
                                    _bcm_regex_engine_obj_t *pinfo)
{
    uint32  regval;

    pinfo->flags = 0;
    pinfo->req_lsz = 0;
    pinfo->from_line = -1;

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, &regval));
    if (soc_reg_field_get(unit, AXP_SM_REGEX_CONTROL0r, regval, REGEX_ENABLEf)) {
        pinfo->flags = BCM_TR3_REGEX_ENGINE_ALLOC;
        /* read the regex code pointers */
        SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL1r(unit, eng_idx, &regval));
        pinfo->from_line = soc_reg_field_get(unit, AXP_SM_REGEX_CONTROL1r, 
                                             regval, REGEX_START_PTR0f);
    }
    return BCM_E_NONE;
}

#define BCM_REGEX_SM_LINE2_OFFSET(l) ((l) * BCM_TR3_NUM_RV_PER_LINE)

STATIC int 
_bcm_tr3_engine_enable(int unit, _bcm_regex_engine_obj_t *pengine)
{
    uint32  regval, regval1, start_addr;
    int     eng_idx;

    eng_idx = pengine->hw_idx;

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, &regval));
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, &regval, REGEX_RESETf, 0);
    if (soc_reg_field_get(unit, AXP_SM_REGEX_CONTROL0r, regval, REGEX_ENABLEf)) {
    
        soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, &regval, REGEX_ENABLEf, 0);
        SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, regval));
    }

    /* program the regex program/state offsets. */
    start_addr = BCM_REGEX_SM_LINE2_OFFSET(pengine->from_line);
    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL1r(unit, eng_idx, &regval1));
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL1r, &regval1, REGEX_START_PTR0f,
                      start_addr);
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL1r, &regval1, REGEX_START_PTR1f,
                      start_addr);
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL1r(unit, eng_idx, regval1));

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL2r(unit, eng_idx, &regval1));
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL2r, &regval1, REGEX_START_PTR3f,
                      start_addr);
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL2r, &regval1, REGEX_START_PTR2f,
                      start_addr);
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL2r(unit, eng_idx, regval1));

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL3r(unit, eng_idx, &regval1));
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL3r, &regval1, REGEX_START_PTR4f,
                      start_addr);
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL3r, &regval1, REGEX_START_PTR5f,
                      start_addr);
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL3r(unit, eng_idx, regval1));

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL4r(unit, eng_idx, &regval1));
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL4r, &regval1, REGEX_START_PTR7f,
                      start_addr);
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL4r, &regval1, REGEX_START_PTR6f,
                      start_addr);
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL4r(unit, eng_idx, regval1));

    /* enable the engine */
    if (BCM_REGEX_CPS_ALLOCED(&pengine->cps)) {
        start_addr = BCM_REGEX_SM_LINE2_OFFSET(pengine->cps.from_line);
        soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, &regval, 
                            CONTEXT_BASE_PTRf, start_addr);
    } else {
        soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, &regval, 
                            CONTEXT_BASE_PTRf, 0);
    }
    
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, &regval, REGEX_ENABLEf, 1);
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, regval));
    return BCM_E_NONE;
}

STATIC int _bcm_tr3_engine_disable(int unit, int eng_idx)
{
    uint32  regval;

    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, &regval));
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, &regval, REGEX_ENABLEf, 0);
    SOC_IF_ERROR_RETURN(WRITE_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, regval));
    return BCM_E_NONE;
}

STATIC bcm_regex_desc_t tr3_regex_info = { 8, 
                                           4, 
                                           4,
                                           32,
                                           _bcm_tr3_engine_reset, 
                                           _bcm_tr3_get_engine_info,
                                           _bcm_tr3_engine_enable,
                                           _bcm_tr3_engine_disable,
                                         };

#define BCM_REGEX_ENGINE_ID(u,i)  (((u)<<16) | (i))
#define BCM_REGEX_ENGINE_INDEX_FROM_ID(u,id)  ((id) & 0xffff)

STATIC int
_bcm_tr3_find_engine(int unit, bcm_regex_engine_t engid,
                        _bcm_regex_lengine_t** plengine)
{
    int idx;
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_lengine_t *lengine = NULL;
    
    device = REGEX_INFO(unit);

    if (!device) {
        return BCM_E_UNAVAIL;
    }

    idx = BCM_REGEX_ENGINE_INDEX_FROM_ID(unit, engid);

    if (idx >= BCM_TR3_NUM_ENGINE(device)) {
        return BCM_E_PARAM;
    }

    lengine = BCM_TR3_LOGICAL_ENGINE(device, idx);
    if (!BCM_REGEX_ENGINE_ALLOCATED(lengine)) {
        return BCM_E_NOT_FOUND;
    }
    *plengine = lengine;
    return BCM_E_NONE;
}

int 
_bcm_tr3_regex_engine_create(int unit, bcm_regex_engine_config_t *config, 
                            bcm_regex_engine_t *engid)
{
    int i;
    _bcm_regex_lengine_t *lengine = NULL, *tmp;
    _bcm_tr3_regex_device_info_t *device;
   
    device = REGEX_INFO(unit);
    if (!device) {
        return BCM_E_INIT;
    }
    
    BCM_REGEX_LOCK(unit);
    for (i = 0; i < BCM_TR3_NUM_ENGINE(device); i++) {
        tmp = BCM_TR3_LOGICAL_ENGINE(device, i);
        if (BCM_REGEX_ENGINE_ALLOCATED(tmp)) {
            continue;
        }
        BCM_REGEX_ENGINE_SET_ALLOCATED(tmp);
        sal_memcpy(&tmp->config, config, sizeof(bcm_regex_engine_config_t));
        lengine = tmp;
        break;
    }
    BCM_REGEX_UNLOCK(unit);
    if (lengine) {
        *engid = lengine->id;
    }
    return (lengine) ? BCM_E_NONE : BCM_E_RESOURCE;
}

STATIC int
_bcm_tr3_regex_hw_engine_detach(int unit, _bcm_regex_engine_obj_t *pengine)
{
    _bcm_regex_cps_obj_t *pcps;
    _bcm_tr3_regex_device_info_t *device;
#if 0
    _bcm_regexdb_cb_ua_t ua;
#endif
    _bcm_regexdb_entry_t en;

    BCM_DEBUG(BCM_DBG_REGEX,("resetting engine.."));

    device = REGEX_INFO(unit);
    if (!pengine || !BCM_REGEX_ENGINE_ALLOCATED(pengine)) {
        return BCM_E_NONE;
    }

    SOC_IF_ERROR_RETURN(BCM_REGEX_DISABLE_ENGINE(device, unit, pengine->hw_idx));

    /*
     * unavail any check slots which depend on signatures in this engine.
     */
#if 0
    ua.avail = 0;
    ua.setid = BCM_TR3_SPECIAL_SLOT;
#endif
    en.engid = pengine->hw_idx;
    _bcm_regexdb_traverse(unit, BCM_REGEXDB_MATCH_ENGINE_ID, &en, 
                           _bcm_regexdb_engine_unavail_cb, NULL);

    bcm_regexdb_delete_by_engine_id(unit, pengine->hw_idx);

    pcps = BCM_TR3_CPS_INFO(device, pengine->hw_idx);
    _bcm_tr3_free_cps(unit, pcps);

    BCM_REGEX_ENGINE_SET_UNALLOCATED(pengine);
    pengine->from_line = -1;
    pengine->flags = 0;
    pengine->req_lsz = 0;

    return BCM_E_NONE;
}

int 
_bcm_tr3_regex_engine_destroy(int unit, bcm_regex_engine_t engid)
{
    int     idx, rv = BCM_E_NONE, num_alloc = 0, i;
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_lengine_t *lengine = NULL;
    _bcm_regex_engine_obj_t *pengine = NULL;

    device = REGEX_INFO(unit);

    if (!device) {
        return BCM_E_UNAVAIL;
    }

    BCM_REGEX_LOCK(unit);

    idx = BCM_REGEX_ENGINE_INDEX_FROM_ID(unit, engid);
    lengine = BCM_TR3_LOGICAL_ENGINE(device, idx);
    if (!lengine) {
        return BCM_E_PARAM;
    }

    if (!BCM_REGEX_ENGINE_ALLOCATED(lengine)) {
        BCM_REGEX_UNLOCK(unit);
        return BCM_E_PARAM;
    }
    pengine = lengine->physical_engine;

    /*
     * Disable the HW dfa engine and detach from logical engine 
     **/
    rv = _bcm_tr3_regex_hw_engine_detach(unit, pengine);
    if (SOC_FAILURE(rv)) {
        BCM_REGEX_UNLOCK(unit);
        return rv;
    }

    BCM_REGEX_ENGINE_SET_UNALLOCATED(lengine);
    lengine->physical_engine = NULL;

    num_alloc = 0;
    for (i = 0; i < BCM_TR3_NUM_ENGINE(device); i++) {
        lengine = BCM_TR3_LOGICAL_ENGINE(device, i);
        if (BCM_REGEX_ENGINE_ALLOCATED(lengine)) {
            num_alloc++;
        }
    }
   
    return rv;
}

int 
_bcm_tr3_regex_engine_traverse(int unit, bcm_regex_engine_traverse_cb cb, 
                                void *user_data)
{
    _bcm_tr3_regex_device_info_t *device;
    bcm_regex_engine_config_t     config;
    _bcm_regex_lengine_t *lengine = NULL;
    int i, rv = BCM_E_NONE, eng_idx;
    uint32  regval, fval;

    if ((device = REGEX_INFO(unit)) == NULL) {
        return BCM_E_INIT;
    }

    BCM_REGEX_LOCK(unit);

    for (i = 0; i < BCM_TR3_NUM_ENGINE(device); i++) {
        lengine = BCM_TR3_LOGICAL_ENGINE(device, i);
        if (!BCM_REGEX_ENGINE_ALLOCATED(lengine)) {
            continue;
        }

        /* continue if no physical engine attached. */
        if (lengine->physical_engine == NULL) {
            continue;
        }
        
        sal_memset(&config, 0, sizeof(config));

        /* read the config from HW */
        eng_idx = lengine->physical_engine->hw_idx;
        rv = READ_AXP_SM_REGEX_CONTROL0r(unit, eng_idx, &regval);
        if (SOC_FAILURE(rv)) {
            goto fail;
        }
        fval = soc_reg_field_get(unit, AXP_SM_REGEX_CONTROL0r,
                                    regval, CONTEXT_STORAGE_MODEf);
        if (fval == 1) {
            config.flags |= BCM_REGEX_ENGINE_CONFIG_MULTI_PACKET;
        }
        rv = cb(unit, lengine->id, &config, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
        if (rv) {
            break;
        }
#endif
    }

fail:
    BCM_REGEX_UNLOCK(unit);
    return rv;
}

int 
_bcm_tr3_regex_engine_get(int unit, bcm_regex_engine_t engid, 
                            bcm_regex_engine_config_t *config)
{
    _bcm_regex_lengine_t *lengine = NULL;
    uint32  regval, fval;

    SOC_IF_ERROR_RETURN(_bcm_tr3_find_engine(unit, engid, &lengine));

    sal_memcpy(config, &lengine->config, sizeof(bcm_regex_engine_config_t));

    if (lengine->physical_engine) {
        /* read the config from HW */
        SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL0r(unit, 
                                lengine->physical_engine->hw_idx, &regval));
        fval = soc_reg_field_get(unit, AXP_SM_REGEX_CONTROL0r,
                                 regval, CONTEXT_STORAGE_MODEf);
        if (fval == 1) {
            config->flags |= BCM_REGEX_ENGINE_CONFIG_MULTI_PACKET;
        }
    }
    return BCM_E_NONE;
}

STATIC soc_mem_t
_bcm_tr3_get_axp_sm_memory(int unit, int tile)
{
    soc_mem_t mem = -1;

    switch (tile) {
        case 0: mem = AXP_SM_STATE_TABLE_MEM0m; break;
        case 1: mem = AXP_SM_STATE_TABLE_MEM1m; break;
        case 2: mem = AXP_SM_STATE_TABLE_MEM2m; break;
        case 3: mem = AXP_SM_STATE_TABLE_MEM3m; break;
        case 4: mem = AXP_SM_STATE_TABLE_MEM4m; break;
        case 5: mem = AXP_SM_STATE_TABLE_MEM5m; break;
        case 6: mem = AXP_SM_STATE_TABLE_MEM6m; break;
        case 7: mem = AXP_SM_STATE_TABLE_MEM7m; break;
        default: break;
    }
    return mem;
}

/* CHECK */
#define BCM_TR3_ADD_TRANSITION_TO_STATE(t,s,c)    \
            (t)[(s)] = (((t)[(s)] & 0xc000) | (((t)[(s)] & 0x3fff) + (c)))

#define BCM_TR3_SET_STATE_TRANSITION_OFFSET(t,s,o)    \
            (t)[(s)] = (((t)[(s)] & 0xc000) | ((o) & 0x3fff))

#define BCM_TR3_NUM_STATE_TRANSITION(t,s)   ((t)[(s)] & 0x3fff)

#define BCM_TR3_NUM_STATE_OFFSET(t,s)   ((t)[(s)] & 0x3fff)

#define BCM_TR3_STATE_OFFSET(t,s)   ((t)[(s)] & 0x3fff)

#define BCM_TR3_DFA_STATE_F_FINAL   0x1
#define BCM_TR3_SET_STATE_FLAGS(t,s,f)    \
            (t)[(s)] = ((t)[(s)] & 0x3fff) | (((f) & 3) << 14)

#define BCM_TR3_GET_STATE_FLAGS(t,s,f)  (((t)[(s)] >> 14) & 3)

#define _BCM_TR3_SET_DEFAULT_POLICY 0x01

STATIC int
_bcm_tr3_install_action_policy(int unit, bcm_regex_match_t *match, 
                            uint32 flags, int *policy_index)
{
    int rv, pindex;
    uint32 hw_index;
    ft_policy_entry_t policy;
    void *entries[1];

    sal_memset(&policy, 0, sizeof(ft_policy_entry_t));

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_DROP) {
        soc_mem_field32_set(unit, FT_POLICYm, &policy, R_DROPf, 1);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, Y_DROPf, 1);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, G_DROPf, 1);
    }

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_COPY_TO_CPU) {
        soc_mem_field32_set(unit, FT_POLICYm, &policy, R_COPY_TO_CPUf, 1);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, Y_COPY_TO_CPUf, 1);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, G_COPY_TO_CPUf, 1);
    }

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_INT_PRI) {
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                            R_COS_INT_PRIf, match->new_int_pri);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                            Y_COS_INT_PRIf, match->new_int_pri);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                            G_COS_INT_PRIf, match->new_int_pri);
    }

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_DSCP) {
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                                    R_NEW_DSCPf, match->new_dscp);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                                    Y_NEW_DSCPf, match->new_dscp);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                                    G_NEW_DSCPf, match->new_dscp);
    }

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_PKT_PRI) {
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                                    G_NEW_PKT_PRIf, match->new_pkt_pri);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, G_CHANGE_PKT_PRIf, 1);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                                    Y_NEW_PKT_PRIf, match->new_pkt_pri);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, Y_CHANGE_PKT_PRIf, 1);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, 
                                    R_NEW_PKT_PRIf, match->new_pkt_pri);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, R_CHANGE_PKT_PRIf, 1);
    }

    if (match->policer_id >= 0) {
        pindex = _BCM_TR3_POLICER_ID_TO_INDEX(match->policer_id);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, METER_SETf, pindex);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, METER_PAIR_MODEf, 7);
        soc_mem_field32_set(unit, FT_POLICYm, &policy, R_DROPf, 1);
    }

    if (flags & _BCM_TR3_SET_DEFAULT_POLICY) {
        rv = soc_mem_write(unit, FT_POLICYm, MEM_BLOCK_ALL, 0, &policy);
    } else {
        entries[0] = &policy;
        rv = soc_profile_mem_add(unit, _bcm_tr3_ft_policy_profile[unit], entries,
                                 1, &hw_index);
        if (!rv) {
            *policy_index = hw_index;
        }
    }
    return rv;
}

STATIC int
_bcm_tr3_set_match_action(int unit, int eng_idx, bcm_regex_match_t *match, 
                          int *match_idx, int msetid, int mchkid, int sigid)
{
    uint32 *alloc_bmap, fval;
    _bcm_tr3_regex_device_info_t *device;
    int     i, j, idx, policy_index = -1, rv = BCM_E_NONE;
    axp_sm_match_table_mem0_entry_t match_entry;
    int     defer_enable = 0;
    ft_age_profile_entry_t ftpm;
    int     age_profile = -1, ticks, max_index, anc_end = 0, ptlen;
    soc_mem_t mem;

    device = REGEX_INFO(unit);

    ptlen = sal_strlen(match->pattern);
    if (ptlen && (match->pattern[ptlen - 1] == '$') &&
        ((ptlen > 1) ? (match->pattern[ptlen - 2] != '\\') : 1)) {
        anc_end = 1;
    }

    /*
     * defer enabling the entry if check slot is not determined yet,
     * either the corresponding setid signature is not programmed.
     */
     if (mchkid == BCM_TR3_SPECIAL_SLOT) {
        defer_enable = 1;
     }

    /*
     * Alloc action index entry.
     */
    if ((match->action_flags & BCM_REGEX_MATCH_ACTION_IGNORE) == 0) {
        SOC_IF_ERROR_RETURN(_bcm_tr3_install_action_policy(unit, match, 0,
                                                        &policy_index));
        /* if the flow is just ignore type, program the entry and enable
         * it now even if check slot is pernding */
         defer_enable = 0;
    }

    alloc_bmap = BCM_TR3_MATCH_BMAP_PTR(unit, device, eng_idx);
    mem = AXP_SM_MATCH_TABLE_MEM0m + BCM_REGEX_ENG2MATCH_MEMIDX(unit,eng_idx);
    idx = -1;
    for (i = 0; i < BCM_TR3_MATCH_BMAP_BSZ; i++) {
        if (alloc_bmap[i] == 0xffffffff) {
            continue;
        }
        for (j = 0; j < 32; j++) {
            if (alloc_bmap[i] & (1 << j)) {
                continue;
            }
            idx = (i*32) + j;
            break;
        }
        break;
    }

    if (idx < 0) {
        rv = BCM_E_NOT_FOUND;
        goto fail;
    }

    /* mark the entry as alloced */
    alloc_bmap[idx/32] |= (1 << (idx % 32));

    sal_memset(&match_entry, 0, sizeof(match_entry));

    soc_mem_field32_set(unit, mem, &match_entry, CHECK_LAST_BYTE_ANCHORf, anc_end);

    soc_mem_field32_set(unit, mem, &match_entry, REPORT_SIGNATURE_IDf, sigid);

    soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PROTOCOLf, 3);

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_IGNORE) {
        soc_mem_field32_set(unit, mem, &match_entry, REPORT_KEEP_FLOWf, 0);
        soc_mem_field32_set(unit, mem, &match_entry, MATCH_REPORT_LEVELf, 2);
    } else {
        soc_mem_field32_set(unit, mem, &match_entry, MATCH_REPORT_LEVELf, 3);
        soc_mem_field32_set(unit, mem, &match_entry, REPORT_KEEP_FLOWf, 1);
        soc_mem_field32_set(unit, mem, &match_entry, REPORT_APPLY_ACTIONf, 1);
        soc_mem_field32_set(unit, mem, &match_entry, REPORT_ACTION_INDEXf, 
                                                                policy_index);
    }

    if (msetid >= 0) {
        soc_mem_field32_set(unit, mem, 
                          &match_entry, MATCH_CROSS_SIG_FLAG_SETf, 1);
        soc_mem_field32_set(unit, mem, 
                        &match_entry, MATCH_CROSS_SIG_FLAG_INDEXf, msetid);
        soc_mem_field32_set(unit, mem, &match_entry, MATCH_REPORT_LEVELf, 0);
    }

    if ((mchkid >= 0) && (mchkid != BCM_TR3_SPECIAL_SLOT)) {
        soc_mem_field32_set(unit, mem, 
                            &match_entry, CHECK_CROSS_SIG_FLAG_IS_SETf, 1);

        soc_mem_field32_set(unit, mem, 
                            &match_entry, CHECK_CROSS_SIG_FLAG_INDEXf, mchkid);
    }

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_NOTIFY_HEADER_ONLY) {
        soc_mem_field32_set(unit, mem, &match_entry, REPORT_PACKET_HEADER_ONLYf, 1);
    }

    if (match->action_flags & BCM_REGEX_MATCH_ACTION_NOTIFY_COPY_TO_CPU) {
        soc_mem_field32_set(unit, mem, &match_entry, REPORT_COPY_TO_CPUf, 1);
    }

    if (match->payload_length_min > 0) {
        soc_mem_field32_set(unit, mem, &match_entry, 
                            CHECK_L4_PAYLOAD_LENGTH_MINf, 
                            (match->payload_length_min & 0x3fff));
    } else {
        soc_mem_field32_set(unit, mem, &match_entry, 
                            CHECK_L4_PAYLOAD_LENGTH_MINf, 0);
    }

    if (match->payload_length_max >= 0) {
        soc_mem_field32_set(unit, mem, &match_entry, 
                            CHECK_L4_PAYLOAD_LENGTH_MAXf, 
                            (match->payload_length_max & 0x3fff));
    } else {
        soc_mem_field32_set(unit, mem, &match_entry, 
                            CHECK_L4_PAYLOAD_LENGTH_MAXf, 0x3fff);
    }

    if (match->flags & BCM_REGEX_MATCH_PKT_SEQUENCE) {
        soc_mem_field32_set(unit, mem, &match_entry, CHECK_CONTIGUOUS_FLOWf, 1);
    }

    fval = 3;
    if (match->flags & (BCM_REGEX_MATCH_TO_SERVER | BCM_REGEX_MATCH_TO_CLIENT)) {
        fval = (match->flags & BCM_REGEX_MATCH_TO_SERVER) ? 1 : 2;
    }
    soc_mem_field32_set(unit, mem, &match_entry, CHECK_PKT_DIRECTIONf, fval);

    soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_MAXf, 0xffff);
    if (match->flags & BCM_REGEX_MATCH_L4_SRC_PORT) {
        soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_MINf, 
                                                        match->l4_src_port_low);
        soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_MAXf, 
                                                        match->l4_src_port_high);
    } 
    if (match->flags & BCM_REGEX_MATCH_L4_DST_PORT) {
        soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_MINf, 
                                                        match->l4_dst_port_low);
        soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_MAXf, 
                                                        match->l4_dst_port_high);
    }
    soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_VALUEf, 0xffff);
    soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_MASKf, 0);
    if ((match->flags & BCM_REGEX_MATCH_L4_DST_PORT) && 
        (match->flags & BCM_REGEX_MATCH_L4_SRC_PORT)) {
        fval = 3;
    } else if (match->flags & BCM_REGEX_MATCH_L4_DST_PORT) {
        fval = 2;
    } else if (match->flags & BCM_REGEX_MATCH_L4_SRC_PORT) {
        fval = 1;
    } else { 
        fval = 3;
    }
    soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PORT_SELECTf, fval);

    fval = (match->flags & BCM_REGEX_MATCH_MULTI_FLOW) ? 1 : 0;
    soc_mem_field32_set(unit, mem, &match_entry, REPORT_HTTP_PERSISTENTf, fval);

    soc_mem_field32_set(unit, mem, &match_entry, REPORT_INACTIVITY_INDEXf, 0);

    soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PAYLOAD_LENGTH_MAXf, 
                                                                        0x3fff);

    soc_mem_field32_set(unit, mem, &match_entry, CHECK_L4_PAYLOAD_LENGTH_MINf, 0);

    soc_mem_field32_set(unit, mem, &match_entry, CHECK_CONTIGUOUS_FLOWf, 0);
   
    soc_mem_field32_set(unit, mem, &match_entry, MATCH_PRIORITYf, match->priority);
    soc_mem_field32_set(unit, mem, &match_entry, CHECK_FIRST_PACKETf, 
                                                (match->sequence ? 0 : 1));

    soc_mem_field32_set(unit, mem, &match_entry, MATCH_ENTRY_VALIDf, 
                                                            !defer_enable);
   
    if (match->inactivity_timeout_usec >= 0)  {
        ticks = match->inactivity_timeout_usec/10000;
        max_index = soc_mem_index_max(unit, FT_AGE_PROFILEm);
        for (i = max_index; i >= 0; i--) {
            SOC_IF_ERROR_RETURN(READ_FT_AGE_PROFILEm(unit, MEM_BLOCK_ALL,
                                                        i, &ftpm));
            if (ticks >= soc_mem_field32_get(unit, 
                                    FT_AGE_PROFILEm, &ftpm, TIMEOUTf)) {
                age_profile = (i == max_index) ? i : i + 1;
                break;
            }
        }
    }
    if (age_profile == -1) {
        age_profile = soc_mem_index_max(unit, FT_AGE_PROFILEm);
    }
   
    soc_mem_field32_set(unit, mem, &match_entry, 
                                    REPORT_INACTIVITY_INDEXf, age_profile);

    *match_idx = idx;

    SOC_IF_ERROR_RETURN(
            soc_mem_write(unit, mem, MEM_BLOCK_ALL, idx, &match_entry));

    /* add to db */
    BCM_DEBUG(BCM_DBG_REGEX,("Add entry to DB, sig=%d match=%d p=%d r=%d ms=%d mc=%d\n",
                                    sigid, match->match_id, match->provides, 
                                    match->requires, msetid, mchkid));
    BCM_IF_ERROR_RETURN(bcm_regexdb_add_entry(unit, idx, eng_idx, match->match_id, 
                    sigid, match->provides, match->requires, msetid, mchkid));

    return BCM_E_NONE;

fail:
    return rv;
}

STATIC regex_cb_error_t 
_bcm_regex_dfa_cb_calc_size(uint32 flags, int match_idx, int in_state, 
                           int from_c, int to_c, int to_state, 
                           int num_dfa_state, void *user_data)
{
    _bcm_tr3_regex_data_t *cd = (_bcm_tr3_regex_data_t*)user_data;

    if (flags & DFA_TRAVERSE_START) {
        cd->compile_size = 0;
        cd->last_state = 0;
        return REGEX_CB_OK;
    }

    if (flags & DFA_TRAVERSE_END) {
        return REGEX_CB_OK;
    }

    if (cd->last_state != in_state) {
        #if 0
        soc_cm_print("State %s %d:\n", (match_idx>=0)? "[FINAL]" : "", in_state);
        #endif
        cd->compile_size++;
    }

    #if 0
    soc_cm_print("\t %d -> %d [%d-%d];\n", in_state, to_state, from_c, to_c);
    #endif
    /* if last state is not same as this state, insert goto IDLE state */
    cd->compile_size++;
    cd->last_state = in_state;
    return REGEX_CB_OK;
}

STATIC int
_bcm_tr3_regex_rv_fields_get(int unit, int eoff, soc_field_t *minf,
                             soc_field_t *maxf, soc_field_t *matchf,
                             soc_field_t *ptrf)
{
    *minf = RV_MIN0f + eoff;
    *maxf = RV_MAX0f + eoff;
    *matchf = RV_MATCHID0f + eoff;
    *ptrf = RV_PTR0f + eoff;
    return 0;
}

STATIC int
_bcm_tr3_regex_rv_set(int unit, soc_mem_t mem,
                      axp_sm_state_table_mem0_entry_t *line,
                      uint8 fromc, uint8 toc, int eoff,
                      uint32 jump_off, int match_index)
{
    soc_field_t minf, maxf, matchf, ptrf;

    _bcm_tr3_regex_rv_fields_get(unit, eoff, &minf, &maxf, &matchf, &ptrf);
    
    if (match_index >= 0) {
        soc_mem_field32_set(unit, mem, (void*)line, minf, 255);
        soc_mem_field32_set(unit, mem, (void*)line, maxf, 1);
        soc_mem_field32_set(unit, mem, (void*)line, matchf, match_index);
    } else {
        soc_mem_field32_set(unit, mem, (void*)line, minf, fromc);
        soc_mem_field32_set(unit, mem, (void*)line, maxf, toc);
        soc_mem_field32_set(unit, mem, (void*)line, ptrf, jump_off);
    }
    return 0;
}

STATIC int
_bcm_tr3_regex_rv_get(int unit, soc_mem_t mem,
                      axp_sm_state_table_mem0_entry_t *line,
                      uint8 *fromc, uint8 *toc, int eoff,
                      uint32 *jump_off, int *match_index)
{
    soc_field_t minf, maxf, matchf, ptrf;

    _bcm_tr3_regex_rv_fields_get(unit, eoff, &minf, &maxf, &matchf, &ptrf);
    
    *match_index = -1;
    *jump_off = 0xffff;
    *fromc = soc_mem_field32_get(unit, mem, (void*)line, minf);
    *toc = soc_mem_field32_get(unit, mem, (void*)line, maxf);
    if ((*fromc == 255) && ((*toc == 1) || (*toc == 2))) {
        *match_index = soc_mem_field32_get(unit, mem, (void*)line, matchf);
    } else {
        *jump_off = soc_mem_field32_get(unit, mem, (void*)line, ptrf);
    }
    return 0;
}

STATIC regex_cb_error_t 
_bcm_tr3_regex_compiler_preprocess_cb(uint32 flags, int match_idx, 
                            int in_state, int from_c, int to_c, int to_state, 
                           int num_dfa_state, void *user_data)
{
    _bcm_tr3_regex_data_t *cd = (_bcm_tr3_regex_data_t*)user_data;
    int insert_fail = 0, i, ofst, size, num_state;

    if (flags & DFA_TRAVERSE_START) {
        cd->last_state = 0;
        return REGEX_CB_OK;
    } else if (flags & DFA_TRAVERSE_END) {
        /* adjust the per state table data to make generate the
         * offset table. */
        ofst = BCM_TR3_NUM_STATE_TRANSITION(cd->per_state_data, 0);
        BCM_TR3_SET_STATE_TRANSITION_OFFSET(cd->per_state_data,0,0);
        for (i=1; i<num_dfa_state; i++) {
            num_state = BCM_TR3_NUM_STATE_TRANSITION(cd->per_state_data, i);
            BCM_TR3_SET_STATE_TRANSITION_OFFSET(cd->per_state_data,i,ofst);
            ofst += num_state;
        }
        return REGEX_CB_OK;
    }

    /* if last state is not same as this state, insert goto IDLE state */
    if (cd->last_state != in_state) {
        insert_fail = 1;
    }

    if (cd->per_state_data == NULL) {
        size = sizeof(uint16)*num_dfa_state;
        cd->per_state_data = sal_alloc(size, "dfa_state_tbl");
        sal_memset(cd->per_state_data, 0, size);
    }
    if (insert_fail) {
        BCM_TR3_ADD_TRANSITION_TO_STATE(cd->per_state_data, cd->last_state,1);
    }
    BCM_TR3_ADD_TRANSITION_TO_STATE(cd->per_state_data, in_state, 1);
    if (flags & DFA_STATE_FINAL) {
        BCM_TR3_SET_STATE_FLAGS(cd->per_state_data, 
                                in_state, BCM_TR3_DFA_STATE_F_FINAL);
    }
    cd->last_state = in_state;
    return REGEX_CB_OK;
}

STATIC regex_cb_error_t 
_bcm_tr3_regex_compiler_install_cb(uint32 flags, int match_idx, int in_state, 
                           int from_c, int to_c, int to_state, 
                           int num_dfa_state, void *user_data)
{
    _bcm_tr3_regex_data_t *cd = (_bcm_tr3_regex_data_t*)user_data;
    int insert_fail = 0, i, j, unit, jf, hw_match_idx;
    int msetid, mchkid, sigid;
    uint16      ptr;

    unit = cd->unit;
    
    if (flags & DFA_TRAVERSE_START) {
        cd->last_state = 0;
        cd->line_dirty = 0;
        cd->wr_off = 0;
        cd->line_off = cd->hw_eng->from_line;
        cd->hw_match_idx = sal_alloc(sizeof(int)*cd->num_match, "hw_match_idx");
        for (i=0; i<cd->num_match; i++) {
            cd->hw_match_idx[i] = -1;
        }
        cd->mem = _bcm_tr3_get_axp_sm_memory(unit, cd->hw_eng->tile);
        if (cd->mem < 0) {
            return REGEX_CB_ABORT;
        }
        return REGEX_CB_OK;
    } else if (flags & DFA_TRAVERSE_END) {
        if (cd->line_dirty) {
            /* fill rest of the line with fail statements */
            for (i=cd->wr_off; i<BCM_TR3_NUM_RV_PER_LINE; i++) {
                _bcm_tr3_regex_rv_set(unit, cd->mem, &cd->line, 0, 
                                        255, i, 0xffff, -1);
            }
	    soc_mem_write(unit, cd->mem, MEM_BLOCK_ALL, cd->line_off, &cd->line);
        }
        if (cd->per_state_data) {
            sal_free(cd->per_state_data);
            cd->per_state_data = NULL;
        }
        if (cd->hw_match_idx) {
            sal_free(cd->hw_match_idx);
            cd->hw_match_idx = NULL;
        }
        return REGEX_CB_OK;
    }

    /* if last state is not same as this state, insert goto IDLE state */
    if (cd->last_state != in_state) {
        insert_fail = 1;
    }

    /* INSTALL */
    jf = 2 | (insert_fail ? 1 : 0);
    for (j=0; j<2; j++) {
        if ((jf & (1 << j)) == 0) {
            continue;
        }

        if ((cd->wr_off % BCM_TR3_NUM_RV_PER_LINE) == 0) {
            if (cd->line_dirty) {
                soc_mem_write(unit, cd->mem, 
                                MEM_BLOCK_ALL, cd->line_off, &cd->line);
                cd->line_off++;
                cd->line_dirty = 0;
            }
            sal_memset(&cd->line, 0, sizeof(axp_sm_state_table_mem0_entry_t));
            soc_mem_field32_set(unit, cd->mem, &cd->line, STATE_DATA_TYPEf, 1);
            cd->wr_off = 0;
        }

        if (j == 1) {
            /* encode an rv entry */
            if (match_idx >= 0) {
                msetid = cd->msetids ? cd->msetids[match_idx] : -1;
                mchkid = cd->mchkids ? cd->mchkids[match_idx] : -1;
                sigid = cd->sig_ids[match_idx];
                hw_match_idx = cd->hw_match_idx[match_idx];
                if (hw_match_idx == -1) {
                    if (_bcm_tr3_set_match_action(unit, cd->hw_eng->hw_idx, 
                                              &cd->matches[match_idx], 
                                              &hw_match_idx, 
                                              msetid, mchkid, sigid)) {
                        BCM_DEBUG(BCM_DBG_REGEX,("error\n"));
                        return REGEX_CB_ABORT;
                    }
                    cd->hw_match_idx[match_idx] = hw_match_idx;
                }

                _bcm_tr3_regex_rv_set(unit, cd->mem, &cd->line, 255, 2, 
                                      cd->wr_off, 0, hw_match_idx);

            } else {
                ptr = (cd->hw_eng->from_line * BCM_TR3_NUM_RV_PER_LINE) +
                        BCM_TR3_STATE_OFFSET(cd->per_state_data, to_state);
                _bcm_tr3_regex_rv_set(unit, cd->mem, &cd->line, from_c, 
                                        to_c, cd->wr_off, ptr, -1);
            }
        } else {
            _bcm_tr3_regex_rv_set(unit, cd->mem, &cd->line, 0, 
                                        255, cd->wr_off, 0xffff, -1);
        }
        cd->line_dirty = 1;
        cd->wr_off++;
    }
    cd->last_state = in_state;
    return REGEX_CB_OK;
}

/* Note: the caller should allocate enough state line to read the dfa */
STATIC int
_bcm_tr3_regex_read_hw_dfa(int unit, _bcm_regex_engine_obj_t *pengine,
                           axp_sm_state_table_mem0_entry_t *pmem)
{
    soc_mem_t mem;

    mem = _bcm_tr3_get_axp_sm_memory(unit, pengine->tile);
    soc_mem_read_range(unit, mem, MEM_BLOCK_ALL, pengine->from_line, 
                       pengine->from_line + pengine->req_lsz - 1, pmem);
    return BCM_E_NONE;
}

/*
 * Rearrange the engines and optimize the use of tile resources,
 * so that all the states in the tiles are utilized to fullest.
 */
STATIC int
_bcm_tr3_rearrage_engines(int unit)
{
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_resource_t *res;
    int rv = BCM_E_NONE;
    int tiles, tilee, num_res;
    _bcm_regex_tile_t  tmp_tiles[BCM_TR3_MAX_TILES];

    device = REGEX_INFO(unit);

    tiles = 0;
    tilee = device->info->num_tiles - 1;

    _bcm_tr3_make_resource_objs(unit, &res, &num_res, tiles, tilee);

    if (num_res == 0) {
        goto done;
    }

    _bcm_regex_sort_resources_by_size(unit, res, num_res);

    _bcm_tr3_arrange_resources_in_tiles(unit, res, num_res, tiles, tilee,
                                            tmp_tiles, BCM_TR3_MAX_TILES);

    rv = _bcm_tr3_rearrange_engines_in_hw(unit, res, num_res, 
                                               tiles, tilee, tmp_tiles);
done:
    return rv;
}

STATIC int _bcm_tr3_get_tile_line_size(int unit)
{
    return soc_mem_index_count(unit, AXP_SM_STATE_TABLE_MEM0m);
}

STATIC int _bcm_tr3_get_cps_line_size(int unit)
{
    uint32  regval, fval, cslsize;

    SOC_IF_ERROR_RETURN(READ_FT_CONFIGr(unit,&regval));
    fval = soc_reg_field_get(unit, FT_CONFIGr, regval, TABLE_SIZEf);
    cslsize = (_decode_max_flow(fval) * 2)/BCM_TR3_LINE_BYTE_SIZE;
    return cslsize;
}

STATIC int
_bcm_tr3_calc_dfa_memsize(int unit, char **re, uint32 *re_flags, 
                          int num_pattern, uint32 flags, 
                         int *hw_states, void **ppdfa)
{
    _bcm_tr3_regex_data_t cdata;
    void    *pdfa;

    if(bcm_regex_compile(re, re_flags, num_pattern, 0, &pdfa) < 0) {
        return BCM_E_FAIL;
    }

    sal_memset(&cdata, 0, sizeof(_bcm_tr3_regex_data_t));
    cdata.unit = unit;

    if (bcm_regex_dfa_traverse(pdfa, 
                                _bcm_regex_dfa_cb_calc_size, (void *)&cdata)) {
        return BCM_E_INTERNAL;
    }

    soc_cm_debug(DK_VERBOSE, "%s,%d,DFA (num_state=%d)\n", 
            FUNCTION_NAME(),__LINE__, cdata.compile_size);

    *hw_states = cdata.compile_size;
    *ppdfa = pdfa;
    return BCM_E_NONE;
}
        
STATIC int
_bcm_tr3_set_remap_table(int unit, int hw_idx, uint8 *cmap_tbl)
{
    int i;
    soc_mem_t remap_mem = AXP_SM_CHAR_REMAP0m + hw_idx;
    axp_sm_char_remap0_entry_t remap_en;
    uint32  regval;
   
    if (cmap_tbl) {
        for (i=0; i<256; i+=4) {
            soc_mem_field32_set(unit, remap_mem, &remap_en, 
                              REMAP_VAL0f, *(cmap_tbl+i));
            soc_mem_field32_set(unit, remap_mem, &remap_en, 
                              REMAP_VAL1f, *(cmap_tbl + i+1));
            soc_mem_field32_set(unit, remap_mem, &remap_en, 
                              REMAP_VAL2f, *(cmap_tbl + i+2));
            soc_mem_field32_set(unit, remap_mem, &remap_en, 
                              REMAP_VAL3f, *(cmap_tbl + i+3));
            soc_mem_write(unit, remap_mem, MEM_BLOCK_ALL, i/4, &remap_en);
        }
    }
    SOC_IF_ERROR_RETURN(READ_AXP_SM_REGEX_CONTROL0r(unit, hw_idx, &regval));
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, 
                                  &regval, CHAR_REMAP_ENABLEf, cmap_tbl?1:0);
    SOC_IF_ERROR_RETURN(
                    WRITE_AXP_SM_REGEX_CONTROL0r(unit, hw_idx, regval));
    return 0;
}

STATIC int
_bcm_tr3_install_dfa(int unit, void *dfa, bcm_regex_match_t *matches,
                     int *msetids, int *mchkids, int *sig_ids,
                     int count, _bcm_regex_engine_obj_t* pengine)
{
    _bcm_tr3_regex_data_t user_data;

    /* Install is done in 2 stages, first stage, preprocess stage
     * calculate the enteries required per state including any fail states.
     * this will create a map :
     *  stateA-> num_hw_state, offset in state_table
     *  stateB-> num_hw_state, offset in state_table
     *  using this its easy to generate the jump pointers to next state.
     */

    BCM_DEBUG(BCM_DBG_REGEX,("installing DFA\n"));
    /*
     * Traverse al the DFA with flags preprocess */
    sal_memset(&user_data, 0, sizeof(user_data));
    user_data.matches = matches;
    user_data.num_match = count;
    user_data.msetids = msetids;
    user_data.mchkids = mchkids;
    user_data.sig_ids = sig_ids;
    user_data.unit = unit;
    user_data.hw_eng = pengine;

    BCM_DEBUG(BCM_DBG_REGEX,("preprocessing .."));
    if (bcm_regex_dfa_traverse(dfa, _bcm_tr3_regex_compiler_preprocess_cb,
                                    (void*) &user_data)) {
        BCM_DEBUG(BCM_DBG_REGEX,("Error\n"));
        return BCM_E_INTERNAL;
    }

    /* traverse again and install it */
    BCM_DEBUG(BCM_DBG_REGEX,("HW Install .."));
    if (bcm_regex_dfa_traverse(dfa, _bcm_tr3_regex_compiler_install_cb,
                                    (void*) &user_data)) {
        BCM_DEBUG(BCM_DBG_REGEX,("Error\n"));
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_disable_tile(int unit, int tile)
{
    _bcm_tr3_regex_device_info_t *device;
    int e, eng_idx;
    
    device = REGEX_INFO(unit);
    for (e = 0; e < BCM_TR3_ENGINE_PER_TILE(device); e++) {
        eng_idx = (tile*BCM_TR3_ENGINE_PER_TILE(device)) + e;
        BCM_REGEX_DISABLE_ENGINE(device, unit, eng_idx);
    }

    return 0;
}

STATIC int
_bcm_tr3_enable_tile(int unit, int tile)
{
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_engine_obj_t      *pengine;
    int e, eng_idx;
    
    device = REGEX_INFO(unit);
    for (e = 0; e < BCM_TR3_ENGINE_PER_TILE(device); e++) {
        eng_idx = (tile*BCM_TR3_ENGINE_PER_TILE(device)) + e;
        pengine = BCM_TR3_PHYSICAL_ENGINE(device, eng_idx); 
        if (!BCM_REGEX_ENGINE_ALLOCATED(pengine)) {
            continue;
        }
        BCM_REGEX_ENABLE_ENGINE(device, unit, pengine);
    }

    return 0;
}

STATIC int 
_bcm_tr3_init_cps_block(int unit, _bcm_regex_cps_obj_t *pcps)
{
    int     i;

    for (i = 0; i < BCM_REGEX_NUM_SLOT_PER_CPS; i++) {
        pcps->alloc_map[i] = -1;
        pcps->refcnt[i] = 0;
    }
    return BCM_E_NONE;
}

STATIC int _bcm_tr3_free_cps(int unit, _bcm_regex_cps_obj_t *pcps)
{
    int i;

    for (i = 0; i < BCM_REGEX_NUM_SLOT_PER_CPS; i++) {
        pcps->alloc_map[i] = -1;
        pcps->refcnt[i] = 0;
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_regex_alloc_one_cps_slot(int unit, int linkid, 
                              _bcm_regex_cps_obj_t *pcps, int *slot)
{
    int j;

    if (linkid <= 0) {
        return BCM_E_PARAM;
    }

    for (j=0; j<BCM_REGEX_NUM_SLOT_PER_CPS; j++) {
        if (pcps->alloc_map[j] == linkid) {
            *slot = pcps->foff + j;
            pcps->alloc_map[j] = linkid;
            pcps->refcnt[j]++;
            return BCM_E_NONE;
        }
    }

    for (j=0; j<BCM_REGEX_NUM_SLOT_PER_CPS; j++) {
        if (pcps->alloc_map[j] == -1) {
            *slot = pcps->foff + j;
            pcps->alloc_map[j] = linkid;
            pcps->refcnt[j] = 1;
            return BCM_E_NONE;
        }
    }

    return BCM_E_RESOURCE;
}

STATIC int
_bcm_regex_free_one_cps_slot(int unit, _bcm_regex_cps_obj_t *pcps, int slot)
{
    int b, idx;
    
    b = (slot/BCM_REGEX_NUM_SLOT_PER_CPS)*BCM_REGEX_NUM_SLOT_PER_CPS;
    if (pcps->foff != b) {
        return BCM_E_PARAM;
    }
   
    idx = slot % BCM_REGEX_NUM_SLOT_PER_CPS;
    if (pcps->refcnt[idx]) {
        pcps->refcnt[idx]--;
        if (pcps->refcnt[idx] == 0) {
            pcps->alloc_map[idx] = -1;
        }
    }
    
    return BCM_E_RESOURCE;
}

STATIC int
_bcm_tr3_regex_alloc_setids(int unit, int *msetids, bcm_regex_match_t *matches,
                            int count, int pengine_hwidx)
{
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_cps_obj_t *pcps;
    int si, rv = BCM_E_NONE;

    device = REGEX_INFO(unit);
    pcps = BCM_TR3_CPS_INFO(device, pengine_hwidx);

    for (si = 0; si < count; si++) {
        if (matches[si].provides <= 0) { /* setids */
            msetids[si] = -1;
            continue;
        }

        /* Multiple signature might set the same flag, in which case 
         * reuse the flag, ie 
         * S{a} -> sets f1
         * S{b} -> sets f1
         * S{c} -> checks for f1, something similar to 
         * match = (S{a} || S{bb}) && S{c}
         */
        rv = _bcm_regex_alloc_one_cps_slot(unit, matches[si].provides,
                                                pcps, &msetids[si]);
        if (rv) {
            break;
        }
    }

    if (rv) {
        for (si = 0; si < count; si++) {
            if (msetids[si] == -1) {
                continue;
            }
            _bcm_regex_free_one_cps_slot(unit, pcps, msetids[si]);
        }
        _bcm_tr3_free_cps(unit, pcps);
    }
    return rv;
}

STATIC int
_bcm_tr3_regex_alloc_chkids(int unit, int *mchkids, bcm_regex_match_t *matches,
                            int count, int pengine_hwidx, int defer)
{
    int rv = BCM_E_NONE, i;

    for (i = 0; i < count; i++) {
        if (matches[i].requires <= 0) { /* chkids */
            mchkids[i] = -1;
            continue;
        }
        rv = bcm_regexdb_find_chkid_for_requires(unit, 
                                        matches[i].requires, &mchkids[i]);
        if (rv) {
            if (defer) {
                mchkids[i] = BCM_TR3_SPECIAL_SLOT;
            } else {
                return BCM_E_RESOURCE;
            }
        }
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_attach_hw_engine(int unit, _bcm_regex_lengine_t *lengine,
                          int req_lsz, int *msetids, int *mchkids,
                          bcm_regex_match_t *matches, int *sigids, 
                          int count)
{
    int rv = BCM_E_NONE, try, engidx, i;
    bcm_regex_engine_config_t config;
    _bcm_regex_engine_obj_t *pengine = NULL;
    int  extra_lsz = 0;

    engidx = BCM_REGEX_ENGINE_INDEX_FROM_ID(unit, lengine->id);

    /*
     * Cannot do both multipacket and cross packet mode 
     */
    SOC_IF_ERROR_RETURN(_bcm_tr3_regex_engine_get(unit, engidx, &config));
    if ((config.flags & BCM_REGEX_ENGINE_CONFIG_MULTI_PACKET) && 
        (msetids || mchkids)) {
        return BCM_E_CONFIG;
    }

    if ((config.flags & BCM_REGEX_ENGINE_CONFIG_MULTI_PACKET) || msetids || 
         mchkids) {
        extra_lsz = _bcm_tr3_get_cps_line_size(unit);
    }
    
    for(pengine=NULL, try=0; (try<2) && !pengine; try++) {
        rv = _bcm_tr3_alloc_regex_engine(unit, req_lsz, extra_lsz, &pengine, -1);
        if (rv && (try == 0)) {
            /* first try and alloc failed, try to rearrang engines
             * across tile and optimize resources to see if we can still
             * fit things.
             */
            BCM_DEBUG(BCM_DBG_REGEX,("Alloc failed.. rearrange engines\n"));
            rv = _bcm_tr3_rearrage_engines(unit);
            if (rv) {
                BCM_DEBUG(BCM_DBG_REGEX,("rearrange engines failed.\n"));
                goto fail;
            }
        }
    }

    /* allocate signature IDs, this is internal ID for a pattern and maps
     * 1 to 1 to match_id provided by the application. The reason for the
     * remapping is , internal signature ID space is limited whereas 
     * application can pick arbitary values for the match-id.
     */
    for (i = 0; i < count; i++) {
        sigids[i] = _bcm_tr3_alloc_sigid(unit, &matches[i]);
    }

    /*
     * Allocate set slots if required 
     */
    if (msetids) {
        rv = _bcm_tr3_regex_alloc_setids(unit, msetids, 
                                           matches, count, pengine->hw_idx);
        if (rv) {
            goto fail;
        }
    }

    /*
     * allocate check slots if required 
     */
    if (mchkids) {
        rv = _bcm_tr3_regex_alloc_chkids(unit, mchkids, 
                                            matches, count, pengine->hw_idx, 1);
        if (rv) {
            goto fail;
        }
    }

    lengine->physical_engine = pengine;

    return BCM_E_NONE;
    
fail:

    if (pengine) {
        _bcm_tr3_regex_hw_engine_detach(unit, pengine);
    }

    return rv;
}

int
_bcm_tr3_regex_match_check(int unit, bcm_regex_match_t *matches,
                           int count, int *metric)
{
    char **res;
    uint32   *res_flags;
    int     i, hw_states, ncs = 0, nncs = 0, rv = BCM_E_NONE;

    if ((count <= 0) || (!matches) || (!metric))  {
        return BCM_E_PARAM;
    }

    res = sal_alloc(count * sizeof(char*), "tmp_regex_patterns");
    res_flags = sal_alloc(count * sizeof(uint32), "tmp_regex_ptr");

    for (i=0; i<count; i++) {
        res[i] = matches[i].pattern;
        res_flags[i] = 0;
        if (matches[i].flags & BCM_REGEX_MATCH_CASE_INSENSITIVE) {
            nncs++;
        } else {
            ncs++;
        }
    }

#if 1
    if (ncs && nncs) {
        for (i = 0; i < count; i++) {
            if (matches[i].flags & BCM_REGEX_MATCH_CASE_INSENSITIVE) {
                res_flags[i] = BCM_TR3_REGEX_CFLAG_EXPAND_LCUC;
            }
        }
    }
#else
    if (nncs) {
        for (i = 0; i < count; i++) {
            if (matches[i].flags & BCM_REGEX_MATCH_CASE_INSENSITIVE) {
                res_flags[i] = BCM_TR3_REGEX_CFLAG_EXPAND_LCUC;
            }
        }
    }
#endif

    if (!ncs && nncs) {
        for (i = 0; i < count; i++) {
            if (matches[i].flags & BCM_REGEX_MATCH_CASE_INSENSITIVE) {
                res_flags[i] = BCM_TR3_REGEX_CFLAG_EXPAND_LC;
            }
        }
    }
    
    BCM_REGEX_LOCK(unit);

    rv = _bcm_tr3_calc_dfa_memsize(unit, res, res_flags, count, 
                                    0, &hw_states, NULL);
    if (!rv && (hw_states >= 0)) {
        *metric = hw_states;
    }

    BCM_REGEX_UNLOCK(unit);

    sal_free(res);
    sal_free(res_flags);
    return rv;
}

int
_bcm_tr3_regex_match_set(int unit, bcm_regex_engine_t engid, 
                         bcm_regex_match_t *matches, int count)
{
    char                         **res        = NULL;
    uint32                        *res_flags  = NULL;
    uint32                         cflags     = 0, regval, fval;
    void                          *dfa;
    int                            i, ncs     = 0, nncs = 0, hw_states, *cps_msetids = NULL, *cps_mchkids = NULL;
    int                            rv         = SOC_E_NONE, req_lsz, need_setids = 0, need_chkids = 0;
    _bcm_tr3_regex_device_info_t  *device;
    _bcm_regex_lengine_t          *lengine    = NULL;
    _bcm_regex_engine_obj_t       *pengine    = NULL;
    int                           *sig_ids    = NULL;
    int                            tot_length = 0;

    if (!(device = REGEX_INFO(unit))) {
        return BCM_E_INIT;
    }

    if ((engid == -1) && (count > 0)) {
        return _bcm_tr3_install_action_policy(unit, &matches[0], 
                                     _BCM_TR3_SET_DEFAULT_POLICY, NULL);
    }

    /* Ensure that at least one regex has non-zero length. Otherwise, no DFA can be
     * generated. Making the check here so that the engine is not disturbed if invalid
     * parameters are supplied. Note that it is valid to supply a count of zero which
     * means to delete the engine. */
    if (count > 0) {
        for (i = 0; i < count; i++) {
            if (NULL != matches[i].pattern) {
                tot_length += strlen(matches[i].pattern);
            }
        }
        if (tot_length <= 0) {
            return BCM_E_PARAM;
        }
    }

    rv = _bcm_tr3_find_engine(unit, engid, &lengine);
    if (rv) {
        return BCM_E_PARAM;
    }
   
    BCM_REGEX_LOCK(unit);

    /* if engine already has patterns, delete them */
    pengine = lengine->physical_engine;
    rv = _bcm_tr3_regex_hw_engine_detach(unit, pengine);
    if (rv) {
        goto fail;
    }

    if (count <= 0) {
        BCM_REGEX_UNLOCK(unit);
        return BCM_E_NONE;
    }

    res = sal_alloc(count * sizeof(char*), "tmp_regex_ptr");
    res_flags = sal_alloc(count * sizeof(uint32), "tmp_regex_ptr");
    sig_ids = sal_alloc(count * sizeof(int), "tmp_sigids");

    /* iterate over all the patterns and see if we can enable
     * case insesnsitive feature in HW vs in software by doing [xX] */
    for (i = 0; i < count; i++) {
        res[i] = matches[i].pattern;
        sig_ids[i] = -1;
        res_flags[i] = 0;
        if (matches[i].flags & BCM_REGEX_MATCH_CASE_INSENSITIVE) {
            nncs++;
        } else {
            ncs++;
        }
        if (matches[i].provides > 0) {
            need_setids += 1;
        }
        if (matches[i].requires > 0) {
            need_chkids += 1;
        }
    }

    /* if the matches are mixed that is, it has both case sensitive and 
     * non-case sensitive matches mixes, we need to expand the case 
     * case insensitive patterns using [xX] */
    if (ncs && nncs) {
        for (i = 0; i < count; i++) {
            if (matches[i].flags & BCM_REGEX_MATCH_CASE_INSENSITIVE) {
                res_flags[i] = BCM_TR3_REGEX_CFLAG_EXPAND_LCUC;
            }
        }
    }

    if (!ncs && nncs) {
        for (i = 0; i < count; i++) {
            if (matches[i].flags & BCM_REGEX_MATCH_CASE_INSENSITIVE) {
                res_flags[i] = BCM_TR3_REGEX_CFLAG_EXPAND_LC;
            }
        }
    }
    
    dfa = NULL;
    rv = _bcm_tr3_calc_dfa_memsize(unit, res, res_flags, count, 
                                    cflags, &hw_states, &dfa);
    if (rv) {
        goto fail;
    }

    req_lsz = (hw_states + BCM_TR3_NUM_RV_PER_LINE-1)/BCM_TR3_NUM_RV_PER_LINE;
    if (req_lsz > _bcm_tr3_get_tile_line_size(unit)) {
        rv = BCM_E_RESOURCE;
        goto fail;
    }

    BCM_DEBUG(BCM_DBG_REGEX,("%d RV entry, %d lines needed\n", hw_states, req_lsz));

    /*
     * Find the new cps slots needs, eliminate the ones for which one leg
     * already exist.
     */
    if (need_setids) {
        cps_msetids = sal_alloc(sizeof(int)*count, "cps_setids");
        if (!cps_msetids) {
            rv = BCM_E_MEMORY;
            goto fail;
        }
        for (i = 0; i < count; i++) {
            cps_msetids[i] = -1;
        }
    }

    if (need_chkids) {
        cps_mchkids = sal_alloc(sizeof(int)*count, "cps_chkids");
        if (!cps_mchkids) {
            rv = BCM_E_MEMORY;
            goto fail;
        }
        for (i = 0; i < count; i++) {
            cps_mchkids[i] = -1;
        }
    }

    /* 
     * find the best tile/physical regex engine where this should be installed
     */
    rv = _bcm_tr3_attach_hw_engine(unit, lengine, req_lsz, 
                     cps_msetids, cps_mchkids, matches, sig_ids, count);
    if (rv) {
        goto fail;
    }

    rv = _bcm_tr3_install_dfa(unit, dfa, matches, cps_msetids, cps_mchkids, 
                              sig_ids, count, lengine->physical_engine);
    if (rv) {
        goto fail;
    }

    /* program the mapping table. */
    if (ncs && nncs) {
        _bcm_tr3_set_remap_table(unit, lengine->physical_engine->hw_idx, NULL);
    } else {
        _bcm_tr3_set_remap_table(unit, lengine->physical_engine->hw_idx,
                                 cmap_lower_tbl);
    }
    
    rv = READ_AXP_SM_REGEX_CONTROL0r(unit, 
                            lengine->physical_engine->hw_idx, &regval);
    if (SOC_FAILURE(rv)) {
        goto fail;
    }
    fval = 0;
    if (cps_msetids || need_chkids) {
        fval = 2;
    } else if (lengine->config.flags & BCM_REGEX_ENGINE_CONFIG_MULTI_PACKET) {
        fval = 1;
    }
    soc_reg_field_set(unit, AXP_SM_REGEX_CONTROL0r, &regval,
                             CONTEXT_STORAGE_MODEf, fval);
    WRITE_AXP_SM_REGEX_CONTROL0r(unit, lengine->physical_engine->hw_idx, regval);

    /*
     * resolve any pending matches.
     */
     if (cps_msetids) {
        for (i = 0; i < count; i++) {
            if (cps_msetids[i] == -1) {
                continue;
            }
            _bcm_regexdb_handle_setid_avail(unit, 1, 
                                            lengine->physical_engine->hw_idx,
                                            matches[i].provides, cps_msetids[i]);
        }
     }

    BCM_REGEX_ENABLE_ENGINE(device, unit, lengine->physical_engine);

fail:
    if (res) {
        sal_free(res);
    }

    if (res_flags) {
        sal_free(res_flags);
    }

    if (dfa) {
        bcm_regex_dfa_free(dfa);
    }

    if (cps_msetids) {
        sal_free(cps_msetids);
    }

    if (cps_mchkids) {
        sal_free(cps_mchkids);
    }

    if (sig_ids) {
        sal_free(sig_ids);
    }

    if (rv) {
        if (pengine) {
            _bcm_tr3_regex_hw_engine_detach(unit, pengine);
        }
        if (sig_ids) {
            for (i = 0; i < count; i++) {
                _bcm_tr3_free_sigid(unit, sig_ids[i]);
            }
        }
    }

    BCM_REGEX_UNLOCK(unit);

    return rv;
}

STATIC int
_bcm_regex_init_cmn(int unit, bcm_regex_desc_t *d, int cold)
{
    _bcm_tr3_regex_device_info_t *device;
    int     i, j, count;
    soc_mem_t mem;
    int entry_words[1], min_index_array[1];
    _bcm_regex_cps_obj_t *pcps;
    ft_age_profile_entry_t ftpm;
    uint32          regval, fval;
    bcm_regex_match_t def_mp;

    if (!cmap_lower_tbl) {
        cmap_lower_tbl = sal_alloc(sizeof(uint8)*256, "remap_table");
        for (i=0; i<256; i++) {
            cmap_lower_tbl[i] = ((i >= 65) && (i <= 90)) ? i + 32 : i;
        }
    }

    /* Init logical engines.  */
    if ((device = REGEX_INFO(unit)) == NULL) {
        device = REGEX_INFO(unit) = sal_alloc(sizeof(_bcm_tr3_regex_device_info_t), 
                              "_bcm_tr3_regex_device_info");
        if (!device) {
            return BCM_E_MEMORY;
        }
        sal_memset(device, 0, sizeof(_bcm_tr3_regex_device_info_t));
        device->info = d;
        BCM_TR3_NUM_ENGINE(device) = d->num_tiles * d->num_engine_per_tile;

        for (i = 0; i < 2; i++) {
            device->flex_pools[i].pool_id = -1;
        }
    }

    sal_memset(device->sigid, 0xff, BCM_TR3_SIG_ID_MAP_BSZ*sizeof(uint32));
    sal_memset(device->policer_bmp, 0, BCM_TR3_POLICER_NUM/sizeof(uint32));
    device->regex_mutex = sal_mutex_create("regex_mutex");

    for (i = 0; i < BCM_TR3_NUM_ENGINE(device); i++) {
        sal_memset(&device->engines[i], 0, sizeof(_bcm_regex_lengine_t));
        sal_memset(&device->physical_engines[i], 0, sizeof(_bcm_regex_engine_obj_t));
        device->physical_engines[i].hw_idx = i;
        device->physical_engines[i].tile = (int)i/d->num_engine_per_tile;
        device->physical_engines[i].from_line = -1;
        device->physical_engines[i].req_lsz = 0;
        device->physical_engines[i].flags = 0;
        device->engines[i].id = BCM_REGEX_ENGINE_ID(unit, i);
        if (cold) {
            SOC_IF_ERROR_RETURN(BCM_REGEX_RESET_ENGINE(device, unit, i));
            bcm_regexdb_delete_by_engine_id(unit, i);
        }
        /* reset cps entries */
        pcps = BCM_TR3_CPS_INFO(device, i);
        pcps->foff = i*BCM_REGEX_NUM_SLOT_PER_CPS;
        pcps->flags = 0;
    }

    for (i=0; i<d->num_match_mem; i++) {
        sal_memset(device->match_alloc_bmap[i], 0, 
                                BCM_TR3_MATCH_BMAP_BSZ*sizeof(uint32));
        if (cold) {
            /* clear match memory */
            SOC_IF_ERROR_RETURN(soc_mem_clear(unit, 
                    AXP_SM_MATCH_TABLE_MEM0m + i, COPYNO_ALL, TRUE));
        }
    }

    if (_bcm_tr3_ft_policy_profile[unit] == NULL) {
        _bcm_tr3_ft_policy_profile[unit] =
            sal_alloc(sizeof(soc_profile_mem_t), "FT_Policy Profile Mem");
        if (_bcm_tr3_ft_policy_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_ft_policy_profile[unit]);
    }
    mem = FT_POLICYm;
    entry_words[0] = sizeof(ft_policy_entry_t) / sizeof(uint32);
    min_index_array[0] = 1; /* reserve entry Zero */
    SOC_IF_ERROR_RETURN(soc_profile_mem_index_create(unit, &mem, entry_words, 
             &min_index_array[0], NULL, NULL, 1, _bcm_tr3_ft_policy_profile[unit]));

    if (cold) {
        count = soc_mem_index_count(unit, FT_AGE_PROFILEm);
        sal_memset(&ftpm, 0, sizeof(ftpm));
        for (i = 0; i < count; i++) {
            if (i == (count - 1)) {
                fval = 0x3fff;
            } else {
                fval = (i <= 8) ? (16 << i) : 0x1000 + (0x400 * (i - 8)) - 1;
            }
            soc_mem_field32_set(unit, FT_AGE_PROFILEm, &ftpm, TIMEOUTf, fval);
            SOC_IF_ERROR_RETURN(WRITE_FT_AGE_PROFILEm(unit, MEM_BLOCK_ALL, 
                                                                    i, &ftpm));
        }

        SOC_IF_ERROR_RETURN(READ_FT_REFRESH_CFGr(unit, &regval));
        soc_reg_field_set(unit, FT_REFRESH_CFGr, &regval, ENABLEf, 1);
        SOC_IF_ERROR_RETURN(WRITE_FT_REFRESH_CFGr(unit, regval));

        SOC_IF_ERROR_RETURN(READ_FT_HASH_CONTROLr(unit, &regval));
        soc_reg_field_set(unit, FT_HASH_CONTROLr, &regval, HASH_SELECT_Af, 1);
        soc_reg_field_set(unit, FT_HASH_CONTROLr, &regval, HASH_SELECT_Bf, 1);
        SOC_IF_ERROR_RETURN(WRITE_FT_HASH_CONTROLr(unit, regval));

        regval = 0;
        SOC_IF_ERROR_RETURN(WRITE_AXP_SM_MEM_ECC_GEN_CONTROLr(unit, regval));

        for (j = 0; j < 8; j++) {
            mem = _bcm_tr3_get_axp_sm_memory(unit, j);
            SOC_IF_ERROR_RETURN(soc_mem_clear(unit, mem, MEM_BLOCK_ALL, 1));
        }
        BCM_IF_ERROR_RETURN(_bcm_tr3_regex_exclude_delete_all(unit));

        /* create a default policy profile */
        sal_memset(&def_mp, 0, sizeof(bcm_regex_match_t));
        BCM_IF_ERROR_RETURN(_bcm_tr3_install_action_policy(unit, &def_mp, 
                                        _BCM_TR3_SET_DEFAULT_POLICY, NULL));
    
	BCM_IF_ERROR_RETURN(_bcm_esw_regex_report_reset(unit));
    }
    
    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0

typedef struct _bcm_regex_dal_s {
    int (*dal_op_u32)(uint8 **buf, uint32 *data, int in);
    int (*dal_op_u16)(uint8 **buf, uint16 *data, int in);
    int (*dal_op_u8)(uint8 **buf, uint8 *data, int in);
} _bcm_regex_dal_t;

STATIC int _bcm_regex_dal_op_u32(uint8 **buf, uint32 *data, int in)
{
    uint8 *pbuf = *buf;
    int i, bshf = (sizeof(uint32) - 1) * 8;
    uint32 v;

    if (in) {
        for (i = 0; i < sizeof(uint32); i++, bshf -= 8) {
            *pbuf++ = (uint8)((*data) >> bshf) & 0xff;
        }
    } else {
        v = 0;
        for (i = 0; i < sizeof(uint32); i++) {
            v |= (uint32)((*pbuf) << bshf);
            pbuf++;
        }
        *data = v;
    }
    *buf = pbuf;
    return 0;
}

STATIC int _bcm_regex_dal_op_u16(uint8 **buf, uint16 *data, int in)
{
    uint8 *pbuf = *buf;
    int i, bshf = (sizeof(uint16) - 1) * 8;
    uint16 v;

    if (in) {
        for (i = 0; i < sizeof(uint16); i++, bshf -= 8) {
            *pbuf++ = (uint8)((*data) >> bshf) & 0xff;
        }
    } else {
        v = 0;
        for (i = 0; i < sizeof(uint16); i++) {
            v |= (uint16)((*pbuf) << bshf);
            pbuf++;
        }
        *data = v;
    }
    *buf = pbuf;
    return 0;
}

STATIC int _bcm_regex_dal_op_u8(uint8 **buf, uint8 *data, int in)
{
    uint8 *pbuf = *buf;

    if (in) {
        *pbuf++ = *data;
    } else {
        *data = *pbuf++;
    }
    *buf = pbuf;
    return 0;
}

STATIC _bcm_regex_dal_t bcm_regex_dal = {
                        _bcm_regex_dal_op_u32,
                        _bcm_regex_dal_op_u16,
                        _bcm_regex_dal_op_u8,
                                    };

#define BCM_REGEX_DAL_RD_U32(b,d) bcm_regex_dal.dal_op_u32((b),(d),0)
#define BCM_REGEX_DAL_RD_U16(b,d) bcm_regex_dal.dal_op_u16((b),(d),0)
#define BCM_REGEX_DAL_RD_U8(b,d) bcm_regex_dal.dal_op_u8((b),(d),0)

#define BCM_REGEX_DAL_WR_U32(b,d) bcm_regex_dal.dal_op_u32((b),(d),1)
#define BCM_REGEX_DAL_WR_U16(b,d) bcm_regex_dal.dal_op_u16((b),(d),1)
#define BCM_REGEX_DAL_WR_U8(b,d) bcm_regex_dal.dal_op_u8((b),(d),1)

#define BCM_REGEX_WARMBOOT_ENGINE_SIG   0x07062005
#define BCM_REGEX_WARMBOOT_MATCH_SIG    0x07062012

STATIC int _bcm_regex_engine_reinit(int unit, uint8 **regex_data)
{
    uint32 v, i, leid, peid, req_lsz, cps_lsz;
    uint16 v16;
    uint8  v8, tv8;
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_engine_obj_t      engine_info, *pengine;
    _bcm_regex_lengine_t         *lengine;

    device = REGEX_INFO(unit);

    /* read marker */
    BCM_REGEX_DAL_RD_U32(regex_data, &v);
    if (v != BCM_REGEX_WARMBOOT_ENGINE_SIG) {
        return BCM_E_MEMORY;
    }

    BCM_REGEX_DAL_RD_U8(regex_data, &v8);

    for (i=0; i<v8; i++) {
        /* logicval engine id */
        BCM_REGEX_DAL_WR_U8(regex_data, &tv8);
        leid = (uint32) tv8;
        /* physical engine id */
        BCM_REGEX_DAL_WR_U8(regex_data, &tv8);
        peid = (uint32) tv8;
        /* engine dfa length */
        BCM_REGEX_DAL_WR_U16(regex_data, &v16);
        req_lsz = (uint16) v16;
        /* engine cps length */
        BCM_REGEX_DAL_WR_U16(regex_data, &v16);
        cps_lsz = (uint16) v16;

        BCM_REGEX_GET_ENGINE_INFO(device, unit, peid, &engine_info);
        if (!BCM_REGEX_ENGINE_ALLOCATED(&engine_info)) {
            continue;
        }
        pengine = BCM_TR3_PHYSICAL_ENGINE(device, peid); 
        lengine = BCM_TR3_LOGICAL_ENGINE(device, leid);
        BCM_REGEX_ENGINE_SET_ALLOCATED(lengine);
        BCM_REGEX_ENGINE_SET_ALLOCATED(pengine);
        pengine->from_line = engine_info.from_line;
        pengine->req_lsz = req_lsz;
        pengine->cps.req_lsz = cps_lsz;
        pengine->total_lsz = cps_lsz + req_lsz;
        /* set physical engine mapping */
        lengine->physical_engine = pengine;
    }
    
    BCM_REGEX_DAL_RD_U32(regex_data, &v);
    if (v != ~BCM_REGEX_WARMBOOT_ENGINE_SIG) {
        return BCM_E_MEMORY;
    }
    return BCM_E_NONE;
}

STATIC int _bcm_regex_engine_sync(int unit, uint8 **regex_data)
{
    uint32 i, v32;
    uint8  v8;
    uint16 v16;
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regex_lengine_t         *lengine;

    device = REGEX_INFO(unit);

    /* read marker */
    v32 = BCM_REGEX_WARMBOOT_ENGINE_SIG;
    BCM_REGEX_DAL_WR_U32(regex_data, &v32);

    for (i=0,v8=0; i<BCM_TR3_NUM_ENGINE(device); i++) {
        lengine = BCM_TR3_LOGICAL_ENGINE(device, i);
        if (BCM_REGEX_ENGINE_ALLOCATED(lengine)) {
            v8++;
        }
    }
    
    BCM_REGEX_DAL_WR_U8(regex_data, &v8);

    /*
     * write engine data.
     */
    for (i=0; i<BCM_TR3_NUM_ENGINE(device); i++) {
        lengine = BCM_TR3_LOGICAL_ENGINE(device, i);
        if (!BCM_REGEX_ENGINE_ALLOCATED(lengine)) {
            continue;
        }

        /* engine data:
         *  engine logical index (u8), engine physical index(u8),
         *  dfa length (u16)
         */
        /* engine logical index */
        v8 = (uint8)i;
        BCM_REGEX_DAL_WR_U8(regex_data, &v8);
        /* engine physical index */
        v8 = (uint8) lengine->physical_engine->hw_idx;
        BCM_REGEX_DAL_WR_U8(regex_data, &v8);
        /* engine dfa size */
        v16 = lengine->physical_engine->req_lsz;
        BCM_REGEX_DAL_WR_U16(regex_data, &v16);
        /* engine cps size */
        v16 = lengine->physical_engine->cps.req_lsz;
        BCM_REGEX_DAL_WR_U16(regex_data, &v16);
    }

    v32 = ~BCM_REGEX_WARMBOOT_ENGINE_SIG;
    BCM_REGEX_DAL_WR_U32(regex_data, &v32);
    return 0;
}

STATIC int _bcm_regex_match_reinit(int unit, uint8 **regex_data)
{
    uint32 i, v32;
    uint16 f, num_match = 0;
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regexdb_entry_t pe;
    axp_sm_match_table_mem0_entry_t match_entry;
    soc_mem_t mem;
    int msetid = -1, mchkid = -1, hw_match_idx;
    uint32 *alloc_bmap;
    _bcm_regex_engine_obj_t *pengine = NULL;

    device = REGEX_INFO(unit);

    BCM_REGEX_DAL_RD_U32(regex_data, &v32);
    if (v32 != BCM_REGEX_WARMBOOT_MATCH_SIG) {
        return BCM_E_MEMORY;
    }

    /* read the number of match entries */
    BCM_REGEX_DAL_RD_U16(regex_data, &num_match);

    for (i=0; i<num_match; i++) {
        sal_memset(&pe, 0, sizeof(_bcm_regexdb_entry_t));

        /* read 2 byte flag */
        BCM_REGEX_DAL_RD_U16(regex_data, &f);

        /* read match id */
        BCM_REGEX_DAL_RD_U32(regex_data, (uint32*)&pe.engid);

        pengine = BCM_TR3_PHYSICAL_ENGINE(device, pe.engid);

        /* read signature id */
        BCM_REGEX_DAL_RD_U32(regex_data, (uint32*)&pe.sigid);

        device->sigid[pe.sigid/32] |= 1 << (pe.sigid % 32);

        /* read match id */
        BCM_REGEX_DAL_RD_U32(regex_data, (uint32*)&pe.match_id);

        /* read match index */
        BCM_REGEX_DAL_RD_U32(regex_data, (uint32*)&pe.match_index);

        /* read provides and requires ID if valid */
        if (f & BCM_REGEXDB_MATCH_PROVIDES) {
            BCM_REGEX_DAL_RD_U32(regex_data, (uint32*)&pe.provides);
        }

        if (f & BCM_REGEXDB_MATCH_REQUIRES) {
            BCM_REGEX_DAL_RD_U32(regex_data, (uint32*)&pe.requires);
        }

        /* read match entry and find the msetid and mchkid */
        mem = AXP_SM_MATCH_TABLE_MEM0m + 
                        BCM_REGEX_ENG2MATCH_MEMIDX(unit,pe.engid);

        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, 
                    MEM_BLOCK_ALL, pe.match_index, &match_entry));

        if (soc_mem_field32_get(unit, mem, &match_entry, 
                                            MATCH_CROSS_SIG_FLAG_SETf)) {
            /* set the new setid */
            msetid = soc_mem_field32_get(unit, mem, &match_entry, 
                                            MATCH_CROSS_SIG_FLAG_INDEXf);
            pengine->cps.alloc_map[msetid % BCM_REGEX_NUM_SLOT_PER_CPS] = msetid;
            pengine->cps.alloc_map[msetid % BCM_REGEX_NUM_SLOT_PER_CPS]++;
        }

        if (soc_mem_field32_get(unit, mem, &match_entry,
                                CHECK_CROSS_SIG_FLAG_IS_SETf)) {
            /* set the new chkid */
            mchkid = soc_mem_field32_get(unit, mem, &match_entry,
                                             MATCH_CROSS_SIG_FLAG_INDEXf);
        }

        alloc_bmap = BCM_TR3_MATCH_BMAP_PTR(unit, device, pe.engid);
        alloc_bmap[pe.match_index/32] |= 1 << (pe.match_index % 32);

        /* 
         * also ref count for match profiles.
         * also cps allocation and reference count 
         */
        hw_match_idx = (pe.match_index % 1024);

        alloc_bmap = BCM_TR3_MATCH_BMAP_PTR(unit, device, pe.engid);
        alloc_bmap[hw_match_idx/32] &= ~(1 << (hw_match_idx % 32));
        BCM_IF_ERROR_RETURN(bcm_regexdb_add_entry(unit, pe.match_index, pe.engid, 
               pe.match_id, pe.sigid, pe.provides, pe.requires, msetid, mchkid));
    }

    /* end marker */
    BCM_REGEX_DAL_WR_U32(regex_data, &v32);
    if (v32 != ~BCM_REGEX_WARMBOOT_MATCH_SIG) {
        return BCM_E_MEMORY;
    }

    return BCM_E_NONE;
}

STATIC int _bcm_regex_match_sync(int unit, uint8 **regex_data)
{
    uint32 i, v32;
    uint8  *num_match_loc;
    uint16 f, num_match = 0;
    _bcm_tr3_regex_device_info_t *device;
    _bcm_regexdb_entry_t *pe;

    device = REGEX_INFO(unit);

    v32 = BCM_REGEX_WARMBOOT_MATCH_SIG;
    BCM_REGEX_DAL_WR_U32(regex_data, &v32);

    /* save the location to write the number of match entries */
    num_match_loc = *regex_data;
    BCM_REGEX_DAL_WR_U16(regex_data, &num_match);

    for (i=0; i<BCM_REGEX_DB_BUCKET_MAX; i++) {
        pe = device->l[i];
        while (pe) {
            f = 0;
            if (pe->provides > 0) {
                f |= BCM_REGEXDB_MATCH_PROVIDES;
            }
            if (pe->requires > 0) {
                f |= BCM_REGEXDB_MATCH_REQUIRES;
            }
            /* write 2 byte flag */
            BCM_REGEX_DAL_WR_U16(regex_data, &f);

            /* write physical engine id */
            v32 = (uint32) pe->engid;
            BCM_REGEX_DAL_WR_U32(regex_data, &v32);

            /* write signature id */
            v32 = (uint32) pe->sigid;
            BCM_REGEX_DAL_WR_U32(regex_data, &v32);

            /* write match id */
            v32 = (uint32) pe->match_id;
            BCM_REGEX_DAL_WR_U32(regex_data, &v32);

            /* write match index */
            v32 = (uint32) pe->match_index;
            BCM_REGEX_DAL_WR_U32(regex_data, &v32);

            /* write provides and requires ID if valid */
            if (f & BCM_REGEXDB_MATCH_PROVIDES) {
                v32 = (uint32) pe->provides;
                BCM_REGEX_DAL_WR_U32(regex_data, &v32);
            }

            if (f & BCM_REGEXDB_MATCH_REQUIRES) {
                v32 = (uint32) pe->requires;
                BCM_REGEX_DAL_WR_U32(regex_data, &v32);
            }
            num_match++;
            pe = pe->next;
        }
    }

    BCM_REGEX_DAL_WR_U16(&num_match_loc, &num_match);

    /* end marker */
    v32 = ~BCM_REGEX_WARMBOOT_MATCH_SIG;
    BCM_REGEX_DAL_WR_U32(regex_data, &v32);

    return BCM_E_NONE;
}

#define REGEX_WB_CACHE_SZ       4096

STATIC int 
_bcm_tr3_regex_reinit(int unit)
{
    soc_scache_handle_t scache_handle;
    int     rv = BCM_E_NONE;
    int     stable_size;
    uint8   *regex_data;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_REGEX, 0);

    if (!SOC_WARM_BOOT(unit)) {
        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
                                     REGEX_WB_CACHE_SZ, &regex_data, 
                                     BCM_WB_DEFAULT_VERSION, NULL);
        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
            return rv;
        }
        return BCM_E_NONE;
    } else {

        SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                     stable_size, &regex_data,
                                     BCM_WB_DEFAULT_VERSION, NULL);

        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
            return rv;
        }

        if (NULL == regex_data) {
            return BCM_E_MEMORY;
        }

        SOC_IF_ERROR_RETURN(_bcm_regex_engine_reinit(unit, &regex_data));

        SOC_IF_ERROR_RETURN(_bcm_regex_match_reinit(unit, &regex_data));
    }

    return rv;
}

int _bcm_esw_tr3_regex_sync(int unit)
{
    soc_scache_handle_t scache_handle;
    int       stable_size, rv = BCM_E_NONE;
    uint8     *regex_data, *data_start;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_REGEX, 0);
    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    
    rv = _bcm_esw_scache_ptr_get(unit,
                                 scache_handle, FALSE,
                                 REGEX_WB_CACHE_SZ, &data_start,
                                 BCM_WB_DEFAULT_VERSION, NULL);

    if (BCM_FAILURE(rv) && (rv != BCM_E_INTERNAL)) {
        return rv;
    }

    if (NULL == data_start) {
        return BCM_E_MEMORY;
    }
    regex_data = data_start;
    rv = _bcm_regex_engine_sync(unit, &regex_data);
    if (SOC_FAILURE(rv)) {
        goto fail;
    }

    rv = _bcm_regex_match_sync(unit, &regex_data);

fail:
    return rv;
}
#endif

int _bcm_tr3_regex_init(int unit)
{
    bcm_regex_desc_t *d = NULL;
    int cold = 1, rv = BCM_E_NONE;

    if (SOC_IS_TRIUMPH3(unit)) {
        d = &tr3_regex_info;
    }

    if (!d) {
        return BCM_E_UNAVAIL;
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        cold = 0;
    }
#endif

    BCM_REGEX_LOCK(unit);

    rv = _bcm_regex_init_cmn(unit, d, cold);
    if (SOC_FAILURE(rv)) {
        goto fail;
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    rv = _bcm_tr3_regex_reinit(unit);
#endif

fail:
    return rv;
}

STATIC int
_bcm_tr3_regex_hw_dfa_relocate(int unit, 
                                soc_mem_t state_mem,
                                int num_lines, int prev_offset, int new_offset,
                                axp_sm_state_table_mem0_entry_t *state_membuf,
                                soc_mem_t to_match_mem, 
                                soc_mem_t from_match_mem,
                                axp_sm_match_table_mem0_entry_t *match_membuf, 
                                int *to_match_alloc_base,
                                int *setid_remap, int setid_tblsz,
                                int update_db)
{
    int i, rvo, adj, sigid, ptrv;
    soc_field_t minf, maxf, matchf, ptrf;
    uint32  minv, maxv, matchv;
    axp_sm_state_table_mem0_entry_t *pstmem;
    axp_sm_match_table_mem0_entry_t *pmtmem;
    int old_chkid, old_setid;
    int next_index = *to_match_alloc_base;
    _bcm_regexdb_entry_t* dbe = NULL;

    adj = (new_offset - prev_offset) * BCM_TR3_NUM_RV_PER_LINE;
    for (i = 0; i < num_lines; i++) {
        pstmem = state_membuf + i;
        for (rvo = 0; rvo < BCM_TR3_NUM_RV_PER_LINE; rvo++) {
            _bcm_tr3_regex_rv_fields_get(unit, rvo, &minf, &maxf, &matchf, &ptrf);
            
            minv = soc_mem_field32_get(unit, state_mem, (void*)pstmem, minf);
            maxv = soc_mem_field32_get(unit, state_mem, (void*)pstmem, maxf);
            if ((minv == 255) && ((maxv == 2) || (maxv == 1))) {
                /* match state */
                matchv = soc_mem_field32_get(unit, state_mem, 
                                             (void*)pstmem, matchf);
                /* get the match entry */
                pmtmem = match_membuf + matchv;
                
                sigid = soc_mem_field32_get(unit, from_match_mem, pmtmem, 
                                                           REPORT_SIGNATURE_IDf);

                if (update_db) {
                    dbe = bcm_regexdb_find_match_entry_by_sig_id(unit, sigid);
                    if (!dbe) {
                        return BCM_E_INTERNAL;
                    }
                }

                /* check if setid or checkid needs relocation. */
                if (soc_mem_field32_get(unit, from_match_mem, pmtmem, 
                                                    MATCH_ENTRY_VALIDf)) {
                    if (soc_mem_field32_get(unit, from_match_mem, 
                                        pmtmem, MATCH_CROSS_SIG_FLAG_SETf)) {
                        /* set the new setid */
                        old_chkid = soc_mem_field32_get(unit, from_match_mem, 
                                          pmtmem, MATCH_CROSS_SIG_FLAG_INDEXf);
                        
                        soc_mem_field32_set(unit, from_match_mem, 
                                        pmtmem, MATCH_CROSS_SIG_FLAG_INDEXf, 
                                        setid_remap[old_chkid]);
                        dbe->msetid = setid_remap[old_chkid];
                    }
                    
                    if (soc_mem_field32_get(unit, from_match_mem, 
                                        pmtmem, CHECK_CROSS_SIG_FLAG_IS_SETf)) {
                        /* set the new chkid */
                        old_setid  = soc_mem_field32_get(unit, from_match_mem, 
                                          pmtmem, CHECK_CROSS_SIG_FLAG_INDEXf);
                        
                        soc_mem_field32_set(unit, from_match_mem, 
                                        pmtmem, CHECK_CROSS_SIG_FLAG_INDEXf, 
                                        setid_remap[old_setid]);
                        dbe->mchkid = setid_remap[old_setid];
                    }
                }
                
                /* write the entry to new match memory */
                SOC_IF_ERROR_RETURN(soc_mem_write(unit, to_match_mem, 
                                    MEM_BLOCK_ALL, next_index, pmtmem));
                next_index++;

            } else {
                ptrv = soc_mem_field32_get(unit, state_mem, (void*)pstmem, ptrf);
                if (ptrv != 0xffff) {
                    ptrv += adj;
                }
                soc_mem_field32_set(unit, state_mem, (void*)pstmem, ptrf, ptrv);
            }
        }
	soc_mem_write(unit, state_mem, MEM_BLOCK_ALL, new_offset+i, pstmem);
    }

    *to_match_alloc_base = next_index;
    return 0;
}

STATIC int
_bcm_tr3_arrange_resources_in_tiles(int unit, _bcm_regex_resource_t *res, 
                                    int num_res, int to_tiles, int to_tilee,
                                    _bcm_regex_tile_t *_tr3tile, int num_tile)
{
    _bcm_tr3_regex_device_info_t *device;
    int i, t, tile_lsz, placed;
    int max_per_tile;

    if (to_tilee >= num_tile) {
        return BCM_E_PARAM;
    }

    device = REGEX_INFO(unit);

    max_per_tile = BCM_TR3_ENGINE_PER_TILE(device);
    tile_lsz = _bcm_tr3_get_tile_line_size(unit);
    sal_memset(_tr3tile, 0, sizeof(_bcm_regex_tile_t)*num_tile);
    for (i = to_tiles; i <= to_tilee; i++) {
        _tr3tile[i].valid = 1;
        _tr3tile[i].avail = tile_lsz;
        _tr3tile[i].num_o = 0;
    }

    for (i = 0; i < num_res; i++) {
        placed = 0;
        for (t = to_tiles; t <= to_tilee; t++) {
            if (!_tr3tile[t].valid) {
                continue;
            }

            if ((_tr3tile[t].avail >= res[i].lsize) && 
                (_tr3tile[t].num_o < max_per_tile)) {
                _tr3tile[t].oidx[_tr3tile[t].num_o++] = i;
                _tr3tile[t].avail -= res[i].lsize;
                placed = 1;
                break;
            }
        }
        if (!placed) {
            return BCM_E_RESOURCE;
        }
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_rearrange_engines_in_hw(int unit, _bcm_regex_resource_t *res, 
                           int num_res, int to_tiles, int to_tilee, 
                           _bcm_regex_tile_t *_tr3tile)
{
    _bcm_tr3_regex_device_info_t *device;
    int i, t, j, k, last_match_offset, off;
    int max_per_tile, maxm, old_tile;
    axp_sm_state_table_mem0_entry_t *pmem;
    _bcm_regex_resource_t *pres;
    _bcm_regex_engine_obj_t physical_engines[BCM_TR3_MAX_ENGINE];
    int *old2new, *new2old, *oldleid2peid, *old_setid2new;
    int new_peid, old_peid, new_setidb, old_setidb;
    soc_mem_t mem, from_match_mem, to_match_mem;
    _bcm_regex_engine_obj_t *pengine = NULL, *old_pe, *new_pe;
    int rv = BCM_E_NONE, match_off;
    _bcm_regex_lengine_t *lengine;
    uint32 *alloc_bmap;
    axp_sm_state_table_mem0_entry_t *dfa_cp[BCM_TR3_MAX_ENGINE], *pdfa; 

    if (!num_res) {
        return BCM_E_NONE;
    }

    device = REGEX_INFO(unit);

    max_per_tile = BCM_TR3_ENGINE_PER_TILE(device);
    for (t=to_tiles; t <= to_tilee; t++) {
        _tr3tile[t].match_mem =  NULL;
    }
   
    old2new = sal_alloc(sizeof(int)*BCM_TR3_MAX_ENGINE, "old2new_emap");
    new2old = sal_alloc(sizeof(int)*BCM_TR3_MAX_ENGINE, "new2old_emap");
    oldleid2peid = sal_alloc(sizeof(int)*BCM_TR3_MAX_ENGINE, "old2new_peid");
    old_setid2new = sal_alloc(sizeof(int)*BCM_TR3_MAX_SLOTS, "setid_map");
    sal_memset(dfa_cp, 0, 
               sizeof(axp_sm_state_table_mem0_entry_t*)*BCM_TR3_MAX_ENGINE);

    if (!(old2new && new2old && oldleid2peid && old_setid2new)) {
        rv = BCM_E_MEMORY;
        goto fail;
    }

    for (i = 0; i < BCM_TR3_MAX_ENGINE; i++) {
        old2new[i] = -1;
        new2old[i] = -1;
        oldleid2peid[i] = -1;
    }

    for (i = 0; i < BCM_TR3_MAX_SLOTS; i++) {
        old_setid2new[i] = -1;
    }

    /*
     * Read the dfa states from HW.
     */
    for (t=to_tiles; t<=to_tilee; t++) {
        if (_tr3tile[t].num_o == 0) {
            continue;
        }

        /* disable the tile */
        _bcm_tr3_disable_tile(unit, t);

        for (i = 0; i < _tr3tile[t].num_o; i++) {
            new_peid = (t * max_per_tile) + i;
            pres = res + _tr3tile[t].oidx[i];
            pengine = pres->pres;
            old_peid = pengine->hw_idx;
            /* add to old2new map table */
            old2new[old_peid] = new_peid;
            new2old[new_peid] = old_peid;
            pmem = sal_alloc(sizeof(axp_sm_state_table_mem0_entry_t)*pengine->req_lsz, 
                                "dfa_cp");
            dfa_cp[old_peid] = pmem;
            _bcm_tr3_regex_read_hw_dfa(unit, pengine, pmem);
        }
        if (_tr3tile[t].match_mem == NULL) {
            mem = AXP_SM_MATCH_TABLE_MEM0m + 
                BCM_REGEX_ENG2MATCH_MEMIDX(unit,pengine->hw_idx);
            maxm = soc_mem_index_max(unit, mem);
            _tr3tile[t].match_mem = sal_alloc(
                sizeof(axp_sm_match_table_mem0_entry_t)*(maxm+1), "match_mem_cp");
    
            /* read the match memory */
            soc_mem_read_range(unit, mem, MEM_BLOCK_ALL, 0, 
                                maxm, _tr3tile[t].match_mem);

            /* clear out the match memory */
            soc_mem_clear(unit, mem, MEM_BLOCK_ALL, 1);
        }
    }

    /*
     * Adjust the logical to physical engine mapping.
     */
    sal_memcpy(physical_engines, device->physical_engines, 
                sizeof(physical_engines));
   
    /* remap logical to physcial mapping */
    for (i=0; i<BCM_TR3_NUM_ENGINE(device); i++) {
        lengine = &device->engines[i];
        if ((pengine = lengine->physical_engine) == NULL) {
            continue;
        }
        old_peid = pengine->hw_idx;
        oldleid2peid[i] = old_peid;
        lengine->physical_engine = NULL;

        pengine->total_lsz = 0;
        pengine->flags = 0;
        pengine->from_line = -1;
        pengine->req_lsz = 0;
        pengine->cps.flags = 0;
        pengine->cps.from_line = -1;
        pengine->cps.req_lsz = 0;
        pengine->total_lsz = 0;
    }

    /* remap logical engines to new physical engines */
    for (i=0; i<BCM_TR3_NUM_ENGINE(device); i++) {
        if (oldleid2peid[i] == -1) {
            continue;
        }
        old_peid = oldleid2peid[i];
        old_pe = &physical_engines[old_peid];
        new_peid = old2new[old_peid];
        new_pe = &device->physical_engines[new_peid];
        device->engines[i].physical_engine = new_pe;

        /* assign engine info/params to new engine from old engine */

        /* keep the from offset since we might have to adjust the 
         * jump offsets in the DFA code. */
        new_pe->from_line   = old_pe->from_line;
        new_pe->req_lsz     = old_pe->req_lsz;
        new_pe->total_lsz   = old_pe->total_lsz;
        new_pe->flags       = old_pe->flags;
        sal_memcpy(&new_pe->cps, &old_pe->cps, sizeof(_bcm_regex_cps_obj_t));
        new_setidb = new_peid * BCM_REGEX_NUM_SLOT_PER_CPS;
        new_pe->cps.foff = new_setidb;
        old_setidb = old_peid * BCM_REGEX_NUM_SLOT_PER_CPS;
        for (k = 0; k < BCM_REGEX_NUM_SLOT_PER_CPS; k++) {
            old_setid2new[old_setidb + k] = new_setidb + k;
        }
    }

    /* flush the DFA to new location/tiles */
    for (t = to_tiles; t <= to_tilee; t++) {
        sal_memset(device->match_alloc_bmap[t], 0, 
                                        BCM_TR3_MATCH_BMAP_BSZ*sizeof(uint32));
        off = 0;
        match_off = 0;
        pengine = &device->physical_engines[t*max_per_tile];
        mem = _bcm_tr3_get_axp_sm_memory(unit, t);
        for (i = 0; i < BCM_TR3_ENGINE_PER_TILE(device); i++, pengine++) {
            if (!BCM_REGEX_ENGINE_ALLOCATED(pengine)) {
                continue;
            }
            pdfa = dfa_cp[new2old[pengine->hw_idx]];   
            if (pdfa == NULL) {
                continue;
            }

            to_match_mem = AXP_SM_MATCH_TABLE_MEM0m + 
                    BCM_REGEX_ENG2MATCH_MEMIDX(unit,pengine->hw_idx);

            from_match_mem = AXP_SM_MATCH_TABLE_MEM0m + 
                    BCM_REGEX_ENG2MATCH_MEMIDX(unit,new2old[pengine->hw_idx]);

            old_tile = new2old[pengine->hw_idx]/max_per_tile;

            last_match_offset = match_off;

            /* adjust the jump offsets */
            _bcm_tr3_regex_hw_dfa_relocate(unit, mem, pengine->req_lsz, 
                                           pengine->from_line, off,
                                           pdfa,
                                           to_match_mem, 
                                           from_match_mem, 
                                           _tr3tile[old_tile].match_mem,
                                           &match_off,
                                           old_setid2new, 512, 1);

            /* set the base for DFA in this engine */
            pengine->from_line = off;
            off += pengine->total_lsz;

            if (pengine->cps.from_line >= 0) {
                
            }

            /* update the alloc map */
            alloc_bmap = &device->match_alloc_bmap[t][0];
            for (j = last_match_offset; j < match_off; j++) {
                alloc_bmap[j/32] |= (1 << (j % 32));
            }

            sal_free(pdfa);
            dfa_cp[new2old[pengine->hw_idx]] = NULL;
        }
    }

    /* free up the alloced resources */
    for (t = to_tiles; t <= to_tilee; t++) {
        if (_tr3tile[t].num_o) {
            _bcm_tr3_enable_tile(unit, t);
        }
    }

fail:
    if (old2new) {
        sal_free(old2new);
    }

    if (new2old) {
        sal_free(new2old);
    }

    if (oldleid2peid) {
        sal_free(oldleid2peid);
    }

    if (old_setid2new) {
        sal_free(old_setid2new);
    }

    for (t = to_tiles; t <= to_tilee; t++) {
        if (_tr3tile[t].match_mem) {
            sal_free(_tr3tile[t].match_mem);
            _tr3tile[t].match_mem = NULL;
        }
    }
    return rv;
}

STATIC int 
_bcm_tr3_alloc_lines_in_tile(int unit, int tile, int require_lsz,
                             int *start_line, int compress)
{
    _bcm_tr3_regex_device_info_t *device;
    int maxlines, i, num_res, as, ae, top = 0;
    _bcm_regex_resource_t *pres, *a;
    _bcm_regex_engine_obj_t *peng = NULL;
    int rv = BCM_E_NONE, lines_avail, hole;
    _bcm_regex_tile_t  tmp_tiles[BCM_TR3_MAX_TILES];

    *start_line = -1;

    device = REGEX_INFO(unit);

    maxlines = lines_avail = _bcm_tr3_get_tile_line_size(unit);
    peng = BCM_TR3_PHYSICAL_ENGINE(device, 
                                    tile*BCM_TR3_ENGINE_PER_TILE(device));
    for (i=0; i<BCM_TR3_ENGINE_PER_TILE(device); i++, peng++) {
        if (!BCM_REGEX_ENGINE_ALLOCATED(peng)) {
            continue;
        }
        lines_avail -= peng->total_lsz;
    }

    if (lines_avail < require_lsz) {
        return BCM_E_RESOURCE;
    }
    
    _bcm_tr3_make_resource_objs(unit, &pres, &num_res, tile, tile);

    if (compress) {
        /* Arrange the objects by size. */
        _bcm_regex_sort_resources_by_size(unit, pres, num_res);

        _bcm_tr3_arrange_resources_in_tiles(unit, pres, num_res, tile, tile,
                                            tmp_tiles, BCM_TR3_MAX_TILES);
        /* compress */
        if (_bcm_tr3_rearrange_engines_in_hw(unit, pres, num_res, 
                                             tile, tile, tmp_tiles)) {
            return BCM_E_INTERNAL;
        }
    } else {
        _bcm_regex_sort_resources_by_offset(unit, pres, num_res);
    }

    top = 0; 
    *start_line = -1;
    for (i=0; i<num_res; i++) {
        a = pres + i;
        as = a->pres->from_line;
        hole = as - top;
        if (hole >= require_lsz) {
            *start_line = top;
            break;
        }
        ae = as + a->pres->total_lsz - 1;
        top = ae+1;
    }

    if ((*start_line < 0) && (maxlines - top) >= require_lsz) {
        *start_line = top;
    }

    if (pres) {
        _bcm_tr3_free_resource_objs(unit, &pres, &num_res);
    }
    return rv;
}

STATIC int _bcm_tr3_alloc_regex_engine(int unit, int clsz, int extra_lsz,
                            _bcm_regex_engine_obj_t **ppeng, int tile)
{
    _bcm_tr3_regex_device_info_t *device;
    int i, j, try, alloc_lsz;
    int tiles, tilee, start_line, rv;
    _bcm_regex_engine_obj_t *peng = NULL;

    device = REGEX_INFO(unit);

    if (tile >= 0) {
        tiles = tilee = tile;
    } else {
        tiles = 0;
        tilee = device->info->num_tiles - 1;
    }

    *ppeng = NULL;
    alloc_lsz = clsz + extra_lsz;

    /*
     * 2 stage allocation, first try find memory where contingious
     * memory available, if not found, in second try, compress the
     * state memory to allocate CPS memory.
     */
    for (try=0; (try<2); try++) {
        /* iterate all the tiles and alocate cps resource */
        for (i = tiles; i <= tilee; i++) {
            peng = BCM_TR3_PHYSICAL_ENGINE(device, 
                                i*BCM_TR3_ENGINE_PER_TILE(device));
            for (j=0; j<BCM_TR3_ENGINE_PER_TILE(device); j++, peng++) {
                if (BCM_REGEX_ENGINE_ALLOCATED(peng)) {
                    continue;
                }
                /* allocate state mem. */
                rv = _bcm_tr3_alloc_lines_in_tile(unit, peng->tile,
                                        alloc_lsz, &start_line, try);
                /* unable to alloc lines in this tile, try next tile. */
                if (rv) {
                    break;
                }
                peng->from_line = start_line;
                peng->req_lsz = clsz;
                *ppeng = peng;
                BCM_REGEX_ENGINE_SET_ALLOCATED(peng);
                peng->cps.from_line = peng->from_line + peng->req_lsz;
                peng->cps.req_lsz = extra_lsz;
                peng->total_lsz = clsz + extra_lsz;
                if (extra_lsz) {
                    _bcm_tr3_init_cps_block(unit, &peng->cps);
                    BCM_REGEX_CPS_SET_ALLOCED(&peng->cps);
                }
                goto phy_engine_alloced;
            }
        }
    }

phy_engine_alloced:
    BCM_DEBUG(BCM_DBG_REGEX,("engine engine %d allocated\n", peng->hw_idx));
    return (*ppeng) ? BCM_E_NONE : BCM_E_RESOURCE;
}

int 
_bcm_tr3_get_match_id(int unit, int signature_id, int *match_id)
{
    _bcm_regexdb_entry_t *pe;
    
    pe = bcm_regexdb_find_match_entry_by_sig_id(unit, signature_id);
    if (!pe) {
        return BCM_E_NOT_FOUND;
    }

    *match_id = pe->match_id;

    return BCM_E_NONE;
}

int _bcm_tr3_policer_create(int unit, bcm_policer_config_t *pol_cfg,
                            bcm_policer_t *policer_id)
{
    uint32          meter_entry[SOC_MAX_MEM_FIELD_WORDS]; /* HW entry buffer.*/
    _bcm_tr3_regex_device_info_t *device;
    int ii, index, from_ii, to_ii, hw_index, rv;
    soc_mem_t meter_table = FP_METER_TABLEm;
    int       refresh_bitsize;
    int       bucket_max_bitsize, bucket_cnt_bitsize; 
    uint32    flags;                    /* Policer flags.           */
    uint32    bucketsize = 0;      /* Bucket size.             */
    uint32    refresh_rate = 0;    /* Policer refresh rate.    */
    uint32    granularity = 0;     /* Policer granularity.     */
    uint32    bucketcount;

    device = REGEX_INFO(unit);

    if (pol_cfg->flags & BCM_POLICER_WITH_ID) {
        if (*policer_id >= BCM_TR3_POLICER_NUM) {
            return BCM_E_PARAM;
        }
        from_ii = *policer_id;
        to_ii   = *policer_id + 1;
    } else {
        from_ii = 0;
        to_ii = BCM_TR3_POLICER_NUM;
    }
    
    for (index = -1, ii = from_ii; ii < to_ii; ii++) {
        if ((device->policer_bmp[ii/32] & (1 << ii)) == 0) {
            index = ii;
            break;
        }
    }

    if (index == -1) {
        return BCM_E_RESOURCE;
    }

    hw_index = _BCM_TR3_POLICER_HW_INDEX(index);

    refresh_bitsize = soc_mem_field_length(unit, meter_table, REFRESHCOUNTf);
    bucket_max_bitsize = soc_mem_field_length(unit, meter_table, BUCKETSIZEf);
    flags = _BCM_XGS_METER_FLAG_GRANULARITY | _BCM_XGS_METER_FLAG_FP_POLICER;

    /* Set packet mode flags setting */
    if (pol_cfg->flags & BCM_POLICER_MODE_PACKETS) {
        flags |= _BCM_XGS_METER_FLAG_PACKET_MODE;
    } else {
        flags &= ~_BCM_XGS_METER_FLAG_PACKET_MODE;
    }
    
    if((rv = _bcm_xgs_kbits_to_bucket_encoding(pol_cfg->ckbits_sec,
                                           pol_cfg->ckbits_burst,
                                           flags, refresh_bitsize,
                                           bucket_max_bitsize,
                                           &refresh_rate,
                                           &bucketsize,
                                           &granularity))) {
        goto error;
    }

    if ((rv = soc_mem_read(unit, meter_table, MEM_BLOCK_ANY,
                        hw_index, &meter_entry))) {
        goto error;
    }

    if (bucketsize) {
        bucket_cnt_bitsize = soc_mem_field_length(unit, meter_table,
                                                  BUCKETCOUNTf);
        bucket_max_bitsize = soc_mem_field_length(unit, meter_table,
                                                  BUCKETSIZEf);
        bucketcount =
            (bucketsize << (bucket_cnt_bitsize - bucket_max_bitsize - 2))  - 1;
        bucketcount &= ((1 << bucket_cnt_bitsize) - 1);
    } else {
        bucketcount = 0;
    }

    soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESHCOUNTf,
                        refresh_rate);
    soc_mem_field32_set(unit, meter_table, &meter_entry, BUCKETSIZEf,
                        bucketsize);
    soc_mem_field32_set(unit, meter_table, &meter_entry, METER_GRANf,
                        granularity);
    soc_mem_field32_set(unit, meter_table, &meter_entry, BUCKETCOUNTf,
                        bucketcount);

    /* Refresh mode is only set to 1 for Single Rate. Other modes get 0 */
    if (pol_cfg->mode  == bcmPolicerModeSrTcm) {
        soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESH_MODEf, 1);
    } else if (pol_cfg->mode  == bcmPolicerModeCoupledTrTcmDs) {
        soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESH_MODEf, 2);
    } else {
        soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESH_MODEf, 0);
    }

    
    if (soc_feature(unit, soc_feature_field_packet_based_metering)
        && (soc_mem_field_valid(unit, meter_table, PKTS_BYTESf))) {
        if (pol_cfg->flags & BCM_POLICER_MODE_PACKETS) {
            soc_mem_field32_set(unit, meter_table, &meter_entry, PKTS_BYTESf, 1);
        } else {
            soc_mem_field32_set(unit, meter_table, &meter_entry, PKTS_BYTESf, 0);
        }
    }

    if ((rv = soc_mem_write(unit, meter_table, MEM_BLOCK_ALL, 
                                    hw_index, meter_entry))) {
        goto error;
    }

    sal_memcpy(&device->policer_cfg[index], pol_cfg, sizeof(bcm_policer_config_t));
    *policer_id = _BCM_TR3_ENCODE_POLICER_ID(unit, index);

    return BCM_E_NONE;

error:
    if (index >= 0) {
        device->policer_bmp[index/32] &= ~(1 << (index % 32));
    }
    return rv;
}

int _bcm_tr3_policer_destroy(int unit, bcm_policer_t policer_id)
{
    uint32          meter_entry[SOC_MAX_MEM_FIELD_WORDS]; /* HW entry buffer.*/
    _bcm_tr3_regex_device_info_t *device;
    int hw_index, index;

    index = _BCM_TR3_POLICER_ID_TO_INDEX(policer_id);
    hw_index = _BCM_TR3_POLICER_HW_INDEX(index);

    if (hw_index < 0) {
        return BCM_E_PARAM;
    }

    device = REGEX_INFO(unit);

    sal_memset(meter_entry, 0, sizeof(uint32) * SOC_MAX_MEM_FIELD_WORDS);
    
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, FP_METER_TABLEm, MEM_BLOCK_ALL, 
                                        hw_index, meter_entry));

    device->policer_bmp[index/32] &= ~(1 << (index % 32));
    sal_memset(&device->policer_cfg[index], 0, sizeof(bcm_policer_config_t));
    return BCM_E_NONE;
}

int _bcm_tr3_policer_destroy_all(int unit)
{
    _bcm_tr3_regex_device_info_t *device;
    int ii, pid;

    device = REGEX_INFO(unit);

    for (ii = 0; ii < BCM_TR3_POLICER_NUM; ii++)  {
        if (device->policer_bmp[ii/32] & (1 << (ii % 32))) {
            pid = _BCM_TR3_ENCODE_POLICER_ID(unit, ii);
            BCM_IF_ERROR_RETURN(_bcm_tr3_policer_destroy(unit, pid));
        }
    }
    return BCM_E_NONE;
}

int _bcm_tr3_policer_traverse(int unit, bcm_policer_traverse_cb cb, 
                              void *user_data)
{
    _bcm_tr3_regex_device_info_t *device;
    int ii, pid, rv;

    device = REGEX_INFO(unit);

    for (ii = 0; ii < BCM_TR3_POLICER_NUM; ii++)  {
        if (device->policer_bmp[ii/32] & (1 << (ii % 32))) {
            pid = _BCM_TR3_ENCODE_POLICER_ID(unit, ii);
            rv = cb(unit, pid, &device->policer_cfg[ii], user_data);
            if (rv) {
#ifdef BCM_CB_ABORT_ON_ERR
                break;
#endif
            }
        }
    }
    return BCM_E_NONE;
}

#define TR3_REGEX_DUMP_DFA_MEM_OFFSET 1

void _bcm_regex_dump_dfa(int unit, bcm_regex_engine_t engid)
{
    _bcm_regex_lengine_t *lengine = NULL;
    _bcm_regex_engine_obj_t *pengine = NULL;
    axp_sm_state_table_mem0_entry_t *memch;
    soc_mem_t   mem;
    int     i, j, num_states;
    uint8 fc, tc;
    int midx;
    uint32 joff;
    int *jtbl, this_state, base, p;

    if (_bcm_tr3_find_engine(unit, engid, &lengine) || 
        ((pengine = lengine->physical_engine) == NULL)) {
        return;
    }

    if (pengine->from_line < 0) {
        soc_cm_print("Invalid DFA state memory offset of allocated engine.\n");
        return;
    }

    base = pengine->from_line*BCM_TR3_NUM_RV_PER_LINE;
    mem = _bcm_tr3_get_axp_sm_memory(unit, pengine->tile);
    memch = sal_alloc(sizeof(axp_sm_state_table_mem0_entry_t)*pengine->req_lsz,
                        "dfa_cpd");

    _bcm_tr3_regex_read_hw_dfa(unit, pengine, memch);
 
    jtbl = sal_alloc(sizeof(int)*pengine->req_lsz*BCM_TR3_NUM_RV_PER_LINE, 
                        "re_jtbl");
    for (i=0; i<pengine->req_lsz*BCM_TR3_NUM_RV_PER_LINE; i++) {
        jtbl[i] = -1;
    }
    num_states = 1;
    for (i = 0; i < pengine->req_lsz; i++) {
        for (j = 0; j < BCM_TR3_NUM_RV_PER_LINE; j++) {
            _bcm_tr3_regex_rv_get(unit, mem, &memch[i], &fc, &tc, 
                                  j, &joff, &midx);
            if ((midx < 0) && (joff != 0xffff)) {
                if (jtbl[joff - base] == -1) {
                    jtbl[joff - base] = num_states++;
                }
            }
        }
    }
    jtbl[0] = 0;

    soc_cm_print("\n------------------DFA -(Tile:%d)---------------\n", pengine->tile);
    this_state = 0;
    p = 0;
    for (i = 0; i < pengine->req_lsz; i++) {
        for (j=0; j<BCM_TR3_NUM_RV_PER_LINE; j++) {
            if (jtbl[((i*BCM_TR3_NUM_RV_PER_LINE)+j)] != -1) {
                this_state = jtbl[((i*BCM_TR3_NUM_RV_PER_LINE)+j)];
                p = 1;
            }
            
            _bcm_tr3_regex_rv_get(unit, mem, &memch[i], &fc, &tc, 
                                  j, &joff, &midx);

            if (p) {
#if TR3_REGEX_DUMP_DFA_MEM_OFFSET==0
                soc_cm_print("State %s %d\n", (midx >= 0) ? "[FINAL]" : "", this_state);
#else
                soc_cm_print("State %s %d @line=%d, offset=%d\n", 
                                    (midx >= 0) ? "[FINAL]" : "", 
                                    this_state, pengine->from_line+i, j);
#endif
                p = 0;
            }
            if (joff != 0xffff) {
                soc_cm_print("\t %d -> %d : [\\%d-\\%d];\n", this_state,
                             jtbl[joff - base], fc, tc);
            }
        }
    }
    sal_free(memch);
    sal_free(jtbl);
    return; 
}

void _bcm_regex_sw_dump_engine(int unit, bcm_regex_engine_t engid,
                                uint32 flags)
{
    _bcm_regex_lengine_t *lengine = NULL;
    _bcm_regex_engine_obj_t *pengine = NULL;

    if (_bcm_tr3_find_engine(unit, engid, &lengine) || 
        ((pengine = lengine->physical_engine) == NULL)) {
        return;
    }

    /* dump engine info */
    soc_cm_print("Regex Engine %d : Start offset:%04d Size (lines):%04d\n",
                 pengine->hw_idx, pengine->from_line, pengine->req_lsz);
    soc_cm_print("          CPS   :%s\n", 
                 BCM_REGEX_CPS_ALLOCED(&pengine->cps) ? "Yes" : "No");
    if (BCM_REGEX_CPS_ALLOCED(&pengine->cps)) {
        soc_cm_print(" From line  : %04d Size (lines):%04d\n",
                     pengine->cps.from_line, pengine->cps.req_lsz);
    }
    if (flags) {
        _bcm_regex_dump_dfa(unit, engid);
    }
}

void _bcm_regex_sw_dump(int unit)
{
    _bcm_tr3_regex_device_info_t *device;
    int i, j, nept, count;
    _bcm_regex_engine_obj_t *pengine = NULL;
    _bcm_regex_lengine_t *tmp;
    
    device = REGEX_INFO(unit);

    nept = BCM_TR3_ENGINE_PER_TILE(device);

    for (count = 0, i=0; i<device->info->num_tiles; i++, count = 0) {
        for (j=0; j<nept; j++) {
            pengine = BCM_TR3_PHYSICAL_ENGINE(device, (i*nept) + j);
            if (!BCM_REGEX_ENGINE_ALLOCATED(pengine)) {
                continue;
            }
            count++;
        }
        if (count == 0) {
            continue;
        }
        soc_cm_print("Tile %02d: %0d Engines in use.\n", i, count);
    }

    for (i = 0; i < BCM_TR3_NUM_ENGINE(device); i++) {
        tmp = BCM_TR3_LOGICAL_ENGINE(device, i);
        if (!BCM_REGEX_ENGINE_ALLOCATED(tmp)) {
            continue;
        }
        _bcm_regex_sw_dump_engine(unit, tmp->id, 0);
    }
}

#endif /* INCLUDE_REGEX */

#endif  /* BCM_TRIUMPH3_SUPPORT */

