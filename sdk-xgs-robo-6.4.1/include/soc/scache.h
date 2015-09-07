/*
 * $Id: scache.h,v 1.16 Broadcom SDK $
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
 * This file contains structure and routine declarations for the
 * storage cache management internal api.
 *
 * The scache is used to manage handles to persistent storage
 *
 */

#ifndef _SOC_SCACHE_H_
#define _SOC_SCACHE_H_
#ifdef BCM_WARM_BOOT_SUPPORT

#include <shared/warmboot.h>
#include <soc/types.h>
#include <soc/defs.h>

#define SOC_DEFAULT_LVL2_STABLE_SIZE (1000 * 3072)

#define SOC_SCACHE_MEMCACHE_HANDLE   0xFF
#define SOC_SCACHE_SOC_DEFIP_HANDLE 0xFE
#define SOC_SCACHE_FLEXIO_HANDLE     0xFD
#define SOC_SCACHE_SERDES_HANDLE     0xFC

#define SOC_SCACHE_SWITCH_CONTROL 0xFC
#define SOC_SCACHE_SWITCH_CONTROL_PART0 0x0


#ifndef  SCACHE_PHASE1_UPGRADE_COMPLETE
/* upgrade: temporary  - shd be removed when scache upgrade is complete */

#define SOC_STABLE_BASIC      0x00000001
#define SOC_STABLE_FIELD      0x00000002
#define SOC_STABLE_MPLS       0x00000004
#define SOC_STABLE_MULTICAST  0x00000008
#define SOC_STABLE_DIFFSERV   0x00000010

#define SOC_STABLE_CORRUPT    1
#define SOC_STABLE_STALE      2

#define SOC_STABLE_FLAGS(unit)    (soc_stable_tmp_flags_get(unit))
#define SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) \
    (SOC_STABLE_FLAGS(unit) & SOC_STABLE_BASIC)
#endif

/* 
 * Read function signature to register for the application provided
 * stable for Level 2 Warm Boot
 */
typedef int (*soc_read_func_t)(
    int unit, 
    uint8 *buf, 
    int offset, 
    int nbytes);

/* 
 * Write function signature to register for the application provided
 * stable for Level 2 Warm Boot
 */
typedef int (*soc_write_func_t)(
    int unit, 
    uint8 *buf, 
    int offset, 
    int nbytes);

#define SOC_SCACHE_MAX_MODULES 256

extern int scache_max_partitions[SOC_MAX_NUM_DEVICES][SOC_SCACHE_MAX_MODULES];

/* 
 * Set the maximum number of partitions used in scache for a module 
 *
 */

#define SOC_SCACHE_MODULE_MAX_PARTITIONS_SET(unit, module, partitions) \
       (scache_max_partitions[unit][module] > partitions) ? \
      0: (scache_max_partitions[unit][module] = partitions) 

#define SOC_SCACHE_MODULE_MAX_PARTITIONS_GET(unit, module, partitions) \
       (*partitions = scache_max_partitions[unit][module])


/* 
 * [De]Allocator function signatures to register for the application provided
 * stable for Level 2 Warm Boot.  The allocator function is called for each
 * new cache handle allocated by the SDK.  This buffer is used for persistent
 * storage synchronization.  Access to it is protected by an internal lock.
 * When a commit occurs, this buffer is passed to the read/write functions
 * registered with the stable.
 */
typedef void * (*soc_alloc_func_t) (uint32 sz);
typedef void   (*soc_free_func_t) (void *p);

/* Register read/write and alloc/free interfaces with the scache.
 * if alloc_f is null, sal_alloc used;
 * if free_f is null, sall_free is used.
 */
extern int soc_switch_stable_register(int unit, soc_read_func_t rf, 
                                      soc_write_func_t wf,
                                      soc_alloc_func_t alloc_f,
                                      soc_free_func_t free_f);
extern int soc_stable_attach(int unit); 
extern int soc_stable_set(int unit, int arg, uint32 flags); 
extern int soc_stable_get(int unit, int *arg, uint32 *flags);  
extern int soc_stable_size_set(int unit, int arg); 
extern int soc_stable_size_get(int unit, int *arg);
extern int soc_stable_used_get(int unit, int *arg);
extern int soc_scache_handle_used_set(int unit, soc_scache_handle_t handle, uint32 arg);
extern int soc_scache_handle_used_get(int unit, soc_scache_handle_t handle, uint32 *arg);

#ifndef  SCACHE_PHASE1_UPGRADE_COMPLETE
/* upgrade: temporary  - shd be removed when scache upgrade is complete */
typedef enum {
    sf_index_min, sf_index_max
} soc_stable_field_t;

extern int soc_stable_tmp_access(int unit, soc_stable_field_t field,
                                 uint32 *val, int get);
extern int soc_stable_tmp_flags_get(int unit);
#endif /*  SCACHE_PHASE1_UPGRADE_COMPLETE */


#define SOC_SCACHE_HANDLE_SET(_handle_, _unit_, _module_, _sequence_) \
        _SHR_SCACHE_HANDLE_SET(_handle_, _unit_, _module_, _sequence_)

#define SOC_SCACHE_HANDLE_UNIT_GET(_handle_) \
        _SHR_SCACHE_HANDLE_UNIT_GET(_handle_)

#define SOC_SCACHE_HANDLE_MODULE_GET(_handle_) \
        _SHR_SCACHE_HANDLE_MODULE_GET(_handle_)

#define SOC_SCACHE_HANDLE_SEQUENCE_GET(_handle_) \
        _SHR_SCACHE_HANDLE_SEQUENCE_GET(_handle_)

#define SOC_SCACHE_ALIGN_SIZE(size) \
        (size) = ((size) + 3) & (~3)

/* scache warm boot versioning */
#define SOC_SCACHE_VERSION(major_, minor_)  _SHR_SCACHE_VERSION(major_, minor_)
#define SOC_SCACHE_VERSION_MAJOR(v_)        _SHR_SCACHE_VERSION_MAJOR(v_)
#define SOC_SCACHE_VERSION_MINOR(v_)        _SHR_SCACHE_VERSION_MINOR(v_)

/* A common scache layout for warm boot */
typedef _shr_wb_cache_t soc_wb_cache_t;

/* Size of scache used by control data */
#define SOC_WB_SCACHE_CONTROL_SIZE          _SHR_WB_SCACHE_CONTROL_SIZE

/* get the size of the caller usable scache,
 * size of _shr_wb_cache_t->scache 
 */
#define SOC_WB_SCACHE_SIZE(raw_size_)       _SHR_WB_SCACHE_SIZE(raw_size_)

/* global scache operations */
extern int soc_scache_init(int unit, uint32 size, uint32 flags);
extern int soc_scache_detach(int unit);
extern int soc_scache_recover(int unit);
extern int soc_scache_commit(int unit);
extern int soc_scache_is_config(int unit);
extern int soc_scache_commit_specific_data(int unit, soc_scache_handle_t handle, uint32 data_size, uint8 *data, int offset);

extern int soc_scache_hsbuf_crc32_append(int unit, soc_scache_handle_t handle_id);

/* per-handle based operations */
extern int soc_scache_alloc(int unit, soc_scache_handle_t handle, uint32 size);
extern int soc_scache_realloc(int unit, soc_scache_handle_t handle, int32 size);
extern int soc_scache_ptr_get(int unit, soc_scache_handle_t handle, 
                              uint8 **ptr, uint32 *size);
extern void soc_scache_info_dump(int unit);
extern int soc_scache_handle_lock(int unit, soc_scache_handle_t handle); 
extern int soc_scache_handle_unlock(int unit, soc_scache_handle_t handle);

extern int
soc_versioned_scache_ptr_get(int unit, soc_scache_handle_t handle, int create,
                             uint32 *size, uint8 **scache_ptr, 
                             uint16 default_ver, uint16 *recovered_ver);

#endif /*  BCM_WARM_BOOT_SUPPORT */
#endif /* _SOC_SCACHE_H_ */
