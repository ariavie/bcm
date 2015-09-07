/*
 * $Id: mem.h,v 1.110 Broadcom SDK $
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
 * socregs.c is missing some information about tables, including the
 * minimum and maximum indices, a user-friendly names for the tables,
 * and table descriptions.  socmem_info_t is a parallel structure
 * indexed by soc_mem_t that contains this information.
 *
 * CBP memory consists of the tables MMU_MEMORIES1_xx, which are of
 * varying lengths and widths, and the transaction queue memories
 * MMU_MEMORIES2_xx.
 */

#ifndef _SOC_MEM_H
#define _SOC_MEM_H

#include <sal/core/time.h>
#include <soc/defs.h>
#include <soc/memory.h>
#include <soc/error.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(PORTMOD_SUPPORT)
#include <soc/mcm/memacc.h>
#endif

/* Backdoor for mem tuner to update system configuration */
extern int (*soc_mem_config_set)(char *name, char *value);

#define    MEM_BLOCK_ANY        SOC_BLOCK_ANY    /* for reading */
#define    MEM_BLOCK_ALL        SOC_BLOCK_ALL    /* for writing */
#define    COPYNO_ALL           SOC_BLOCK_ALL    /* historical */


/* Memory read/write flags.  These are used for the following functions:
 * soc_mem_array_read, soc_mem_array_write_range, soc_mem_write_extended
 * soc_mem_read_extended
 */
#define SOC_MEM_NO_FLAGS           0x0
#define SOC_MEM_DONT_USE_CACHE     0x1
#define SOC_MEM_SCHAN_ERR_RETURN   0x2
#define SOC_MEM_DONT_CONVERT_XY2DM 0x4

/* soc_mem_array_write_range, flags definition: */
#define SOC_MEM_WRITE_SET_ONLY     0x1
#define SOC_MEM_WRITE_COMMIT_ONLY  0x2
#define SOC_MEM_WRITE_STATUS_ONLY  0x4

#define soc_mem_flags(unit, mem)          SOC_MEM_INFO(unit, mem).flags
#define soc_mem_cmp_fn(unit, mem)         SOC_MEM_INFO(unit, mem).cmp_fn
#define soc_mem_base(unit, mem)           SOC_MEM_INFO(unit, mem).base
#define soc_mem_index_min(unit, mem)      SOC_MEM_INFO(unit, mem).index_min
#define soc_mem_index_max(unit, mem)      SOP_MEM_STATE(unit, mem).index_max
#define soc_robo_mem_index_min(unit, mem) SOC_MEM_INFO(unit, mem).index_min
#define soc_robo_mem_index_max(unit, mem) SOC_MEM_INFO(unit, mem).index_max
#define soc_mem_index_count(unit, mem)    \
                (soc_mem_index_max(unit, mem) - \
                 soc_mem_index_min(unit, mem) + 1)
/* returns the maximum possible index for any dynamically extendable mem */
extern int soc_mem_index_limit(int unit, soc_mem_t mem);
#define    soc_mem_index_valid(unit, mem, index)    \
                ((index >= soc_mem_index_min(unit, mem)) && \
                 (index <= soc_mem_index_max(unit, mem)))
#define    soc_robo_mem_index_count(unit, mem)    \
                (soc_robo_mem_index_max(unit, mem) - \
                 soc_mem_index_min(unit, mem) + 1)
#define    soc_robo_mem_index_valid(unit, mem, index)    \
                ((index >= soc_mem_index_min(unit, mem)) && \
                 (index <= soc_robo_mem_index_max(unit, mem)))

/* soc_mem_index_last returns the last 'used' entry in sorted tables */
extern int soc_mem_index_last(int unit, soc_mem_t mem, int copyno);

#define soc_mem_entry_null(unit, mem) \
    SOC_MEM_INFO(unit, mem).null_entry
#define soc_mem_entry_bytes(unit, mem) \
    SOC_MEM_INFO(unit, mem).bytes
#define soc_mem_entry_words(unit, mem) \
    BYTES2WORDS(soc_mem_entry_bytes(unit, mem))
#if defined(BCM_ROBO_SUPPORT)    
#define soc_robo_mem_entry_zeroes(unit, mem) \
    _soc_robo_mem_entry_null_zeroes    
#endif /* BCM_ROBO_SUPPORT */
/* Macro for indexing tables DMA'd out from H/W */
#define soc_mem_table_idx_to_pointer(unit, mem, cast, table, index) \
    ((cast)(&(((uint32 *)(table))[soc_mem_entry_words(unit, mem) * (index)])))
#define soc_mem_entry_zeroes(unit, mem) \
    _soc_mem_entry_null_zeroes
extern int soc_mem_entry_bits(int unit, soc_mem_t mem);

#define __smck(unit, mem, flag) \
    ((soc_mem_flags(unit, mem) & (flag)) != 0)


#define         ENET_CMP_MACADDR(a, b)  \
    sal_memcmp((a), (b), sizeof(sal_mac_addr_t))

#define soc_mem_is_readonly(unit, mem)  __smck(unit, mem, SOC_MEM_FLAG_READONLY)
#define soc_mem_is_writeonly(unit, mem) __smck(unit, mem, SOC_MEM_FLAG_WRITEONLY)
#define soc_mem_is_signal(unit, mem)    __smck(unit, mem, SOC_MEM_FLAG_SIGNAL)
#define soc_mem_is_valid(unit, mem)    \
        (SOC_MEM_IS_VALID(unit, mem) && __smck(unit, mem, SOC_MEM_FLAG_VALID))
#define soc_mem_is_debug(unit, mem)     __smck(unit, mem, SOC_MEM_FLAG_DEBUG)
#define soc_mem_is_sorted(unit, mem)    __smck(unit, mem, SOC_MEM_FLAG_SORTED)
#define soc_mem_is_cbp(unit, mem)       __smck(unit, mem, SOC_MEM_FLAG_CBP)
#define soc_mem_is_cachable(unit, mem)  __smck(unit, mem, SOC_MEM_FLAG_CACHABLE)
#define soc_mem_is_bistepic(unit, mem)  __smck(unit, mem, SOC_MEM_FLAG_BISTEPIC)
#define soc_mem_is_bistcbp(unit, mem)   __smck(unit, mem, SOC_MEM_FLAG_BISTCBP)
#define soc_mem_is_bistffp(unit, mem)   __smck(unit, mem, SOC_MEM_FLAG_BISTFFP)
#define soc_mem_is_unified(unit, mem)   __smck(unit, mem, SOC_MEM_FLAG_UNIFIED)
#define soc_mem_is_hashed(unit, mem)    __smck(unit, mem, SOC_MEM_FLAG_HASHED)
#define soc_mem_is_cam(unit, mem)       __smck(unit, mem, SOC_MEM_FLAG_CAM)
#define soc_mem_is_aggr(unit, mem)      __smck(unit, mem, SOC_MEM_FLAG_AGGR)
#define soc_mem_is_cmd(unit, mem)       __smck(unit, mem, SOC_MEM_FLAG_CMD)
#define soc_mem_is_mview(unit, mem)     __smck(unit, mem, SOC_MEM_FLAG_MULTIVIEW)

#define soc_mem_bist_bit(unit, mem)  ((soc_mem_flags(unit, mem) & \
                                       SOC_MEM_FLAG_BISTBIT) >> \
                                      SOC_MEM_FLAG_BISTBSHFT)

extern void soc_mem_entry_dump(int unit, soc_mem_t mem, void *buf);
extern void soc_mem_entry_dump_vertical(int unit, soc_mem_t mem, void *buf);
extern void soc_mem_entry_dump_if_changed(int unit, soc_mem_t mem,
                                          void *buf, char *prefix);

#define soc_mem_compare_key(unit, mem, a, b) \
    ((*(SOC_MEM_INFO(unit, mem).cmp_fn))((unit), (a), (b)))

#define soc_mem_compare_entry(unit, mem, a, b) \
    (sal_memcmp((a), (b), (soc_mem_entry_words((unit), (mem)) * 4)))

/*
 * Memory Test Module
 *
 * Streamlined module designed for inclusion in the SOC driver for
 * performing power-on memory tests.
 *
 * This module is also used by the main SOC diagnostics memory tests,
 * fronted by user interface code.
 */

/* Values miscompare_cb may return */

#define MT_MISCOMPARE_STOP    0    /* Stop and fail memory test */
#define MT_MISCOMPARE_CONT    1    /* Continue test */

/* Values for patterns input */

#define MT_PAT_ZEROES   (1 << 0)   /* All zeroes pattern */
#define MT_PAT_ONES     (1 << 1)   /* All ones pattern */
#define MT_PAT_FIVES    (1 << 2)   /* All 5s pattern */
#define MT_PAT_AS       (1 << 3)   /* All As pattern */
#define MT_PAT_CHECKER  (1 << 4)   /* Checkerboard pattern */
#define MT_PAT_ICHECKER (1 << 5)   /* Inverted checkerboard */
#define MT_PAT_ADDR     (1 << 6)   /* Data=Address pattern */
#define MT_PAT_RANDOM   (1 << 7)   /* Pseudo-random pattern */
#define MT_PAT_HEX      (1 << 8)   /* Write specific hex byte */

/*
 * Memory test input parameters
 * Must be completely filled in when calling soc_mem_test().
 */

typedef struct soc_mem_test_s {
    int unit;                   /* Unit number under test */

    void *userdata;             /* Ignored by soc_mem_test */

    uint32 patterns;            /* Logical OR of MT_PAT_xx (above) */
    uint8 hex_byte;             /* Specified byte pattern */

    soc_mem_t mem;              /* Memory under test */

    int copyno;                 /* Memory to test or COPYNO_ALL */

    int test_by_entry;          /* Test each memory line, not fill */
    int reverify_count;         /* Fill mode: Repeat reads N times */
    int reverify_delay;         /* Was n sec between read sets */
    int continue_on_error;      /* Do not abort test due to errors */
    int error_count;            /* Running total of errors */
    int error_max;              /* Abort test after N errors */

    int index_start;            /* First index to test */
    int index_end;              /* Last index to test */
    unsigned array_index_start; /* First array index to test (if this is a memory array) */
    unsigned array_index_end;   /* Last array index to test */
    int index_step;             /* Index increment (could be neg.) */

    int read_count;             /* # reads to issue */
    int err_count;              /* Count of miscompares so far */
    int ecc_as_data;            /* treat ecc field as regular field */

    void (*status_cb)(struct soc_mem_test_s *parm, char *status_str);
                                        /* Callback to report test status */

    int (*write_cb)(struct soc_mem_test_s *parm,
                                        /* Callback to write a table entry */
                                        /* Returns -1 on error, 0 on success */
                    unsigned array_index,
                    int copyno,
                    int index,
                    uint32 *entry_data);

    int (*read_cb)(struct soc_mem_test_s *parm,
                                        /* Callback to read a table entry */
                                        /* Returns -1 on error, 0 on success */
                   unsigned array_index,
                   int copyno,
                   int index,
                   uint32 *entry_data);

    int (*miscompare_cb)(struct soc_mem_test_s *parm,
                                        /* Callback when a miscompare occurs */
                                        /* Returns MT_MISCOMPARE_xx (above) */
                         unsigned array_index,
                         int copyno,
                         int index,
                         uint32 *read_data,
                         uint32 *wrote_data,
                         uint32 *mask_data);
} soc_mem_test_t;

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) \
         || defined(BCM_RAPTOR_SUPPORT)
typedef struct dual_hash_info_s {
    int hash_sel0;
    int hash_sel1;
    int bucket_size;
    soc_mem_t base_mem;
} dual_hash_info_t;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT || BCM_RAPTOR_SUPPORT */

extern int soc_mem_test_skip(int unit, soc_mem_t mem, int index);
extern int soc_mem_test(soc_mem_test_t *m); /* Returns 0 on pass, -1 on fail */
extern int soc_mem_parity_control(int unit, soc_mem_t mem,
                                  int copyno, int enable);
extern int soc_mem_cpu_write_control(int unit, soc_mem_t mem, int copyno,
                                     int enable, int *orig_enable);
extern int soc_mem_parity_clean(int unit, soc_mem_t mem, int copyno);
extern int soc_mem_parity_restore(int unit, soc_mem_t mem, int copyno);
extern void soc_mem_datamask_memtest(int unit, soc_mem_t mem, uint32 *buf);

/* _SOC_ACC_TYPE_PIPE_ANY maps to the first pipe where applicable */
typedef enum {
    _SOC_ACC_TYPE_PIPE_ALL   = -2,
    _SOC_ACC_TYPE_PIPE_ANY   = -1,
    _SOC_ACC_TYPE_PIPE_GROUP = 0,
    _SOC_ACC_TYPE_PIPE_X     = 1,
    _SOC_ACC_TYPE_PIPE_Y     = 2,
    _SOC_ACC_TYPE_PIPE_SBS   = 6
} soc_acc_type_t;

typedef enum {
    SOC_PIPE_SELECT_ANY = -1,
    SOC_PIPE_SELECT_ALL = -1,
    SOC_PIPE_SELECT_X_COMMON   = 0,
    SOC_PIPE_SELECT_0   = 0,
    SOC_PIPE_SELECT_Y_COMMON   = 1,
    SOC_PIPE_SELECT_1   = 1
} soc_pipe_select_t;

/* SBUS acc type bits for pre-extended SBUS address format */
#define _SOC_MEM_ADDR_ACC_TYPE_MASK     0x7
#define _SOC_MEM_ADDR_ACC_TYPE_SHIFT     17
#define _SOC_MEM_ADDR_ACC_TYPE_PIPE_X     1
#define _SOC_MEM_ADDR_ACC_TYPE_PIPE_Y     2
#define _SOC_MEM_ADDR_ACC_TYPE_PIPE_SBS   6

extern void _soc_mem_tcam_dm_to_xy(int unit, soc_mem_t mem, int count,
                                   uint32 *dm_entry, uint32 *xy_entry,
                                   uint32 *cache_entry);
/*
 * Comparison functions for memory tables.
 */

#define SOC_MEM_INIT_FLD(table, mem, buf, fld, fldbuf) \
        (soc_meminfo_field_set(mem, (table)[mem], (uint32 *)(buf), \
        fld, (uint32 *)(fldbuf)))

#define SOC_MEM_INIT_FLD_FORCE(table, mem, buf, fld, value) \
        (soc_meminfo_field32_force(mem, (table)[mem], (uint32 *)(buf), \
        fld, value))

#define SOC_MEM_INIT_FLD_MACADDR(table, mem, buf, fld, mac) \
        (soc_meminfo_mac_addr_set(mem, (table)[mem], (uint32 *)(buf), \
        fld, mac))

/* Disabled */
#define SOC_MEM_VALIDATE(_table, _mem_name)

extern int _soc_mem_cmp_gm(int, void *, void *);
extern int _soc_mem_cmp_arl(int, void *, void *);
extern int _soc_mem_cmp_marl(int, void *, void *);
extern int _soc_mem_cmp_rule(int, void *, void *);
extern int _soc_mem_cmp_rule_5665(int, void *, void *);
extern int _soc_mem_cmp_word0(int, void *, void *);
extern int _soc_mem_cmp_vtable(int, void *, void *);
extern int _soc_mem_cmp_ipmc(int, void *, void *);
extern int _soc_mem_cmp_undef(int, void *, void *);

#ifdef    BCM_XGS_SWITCH_SUPPORT
extern int _soc_mem_cmp_l2x(int, void *, void *);
extern int _soc_mem_cmp_l2x_sync(int, void *, void *);
extern int _soc_mem_cmp_l3x(int, void *, void *);
extern int _soc_mem_cmp_l3x_sync(int unit);
extern int _soc_mem_cmp_l3x_set(int unit, uint32 ipmc_config);
#endif    /* BCM_XGS_SWITCH_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
extern int _soc_mem_cmp_tr3_l2x(int, void *, void *);
extern int _soc_mem_cmp_tr3_l2x_sync(int, void *, void *, uint8);
extern int _soc_mem_cmp_tr3_ext_l2x(int, void *, void *);
extern int _soc_mem_cmp_tr3_ext_l2x_1_sync(int, void *, void *, uint8);
extern int _soc_mem_cmp_tr3_ext_l2x_2_sync(int, void *, void *, uint8);
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
extern int _soc_mem_cmp_l2x2(int, void *, void *);
extern int _soc_mem_cmp_l3x2(int, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_l3x2_ip4ucast(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_l3x2_ip4mcast(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_l3x2_ip6ucast(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_l3x2_ip6mcast(int u, void *e_a, void *e_b);
extern int _soc_mem_cmp_lpm(int u, void *e_a, void *e_b);
#endif    /* BCM_FIREBOLT_SUPPORT || BCM_BRADLEY_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
extern int _soc_mem_cmp_vlan_mac(int unit, void *ent_a, void *ent_b);
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_TRX_SUPPORT
extern int _soc_mem_cmp_vlan_mac_tr(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_vlan_xlate_tr(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_egr_vlan_xlate_tr(int unit, void *ent_a, void *ent_b);
#ifdef BCM_TRIUMPH_SUPPORT
extern int _soc_mem_cmp_mpls_entry_tr(int unit, void *ent_a, void *ent_b);
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
extern int _soc_mem_cmp_ing_vp_vlan_membership(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_egr_vp_vlan_membership(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_ing_dnat_address_type(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_l2_endpoint_id(int unit, void *ent_a, void *ent_b);
extern int _soc_mem_cmp_endpoint_queue_map(int unit, void *ent_a, void *ent_b);
#endif /* BCM_TRIDENT2_SUPPORT */
#endif /* BCM_TRX_SUPPORT */

#ifdef    BCM_EASYRIDER_SUPPORT
extern int _soc_mem_cmp_l2er(int, void *, void *);
extern int _soc_mem_cmp_l3v4er(int, void *, void *);
extern int _soc_mem_cmp_l3v6er(int, void *, void *);
extern int _soc_mem_cmp_l3_defip_alg(int, void *, void *);
#endif    /* BCM_EASYRIDER_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) \
         || defined(BCM_RAPTOR_SUPPORT)
extern int _soc_mem_dual_hash_insert(int unit, soc_mem_t mem, int copyno,
                                     void *entry_data, void *old_entry_data,
                                     int recurse_depth);
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT || BCM_RAPTOR_SUPPORT */

/* Initialize Slot Free Address Pool and Cell Free Address Pool */
extern int soc_mem_cfap_init(int unit);

/* Lock a memory to allow for atomic manipulations by multiple tasks */
#define    soc_mem_lock(_u, _m)    MEM_LOCK(_u, _m)
#define    soc_mem_unlock(_u, _m)    MEM_UNLOCK(_u, _m)

/* robo devices*/
#define ARL_SW_TABLE_LOCK(s)     sal_mutex_take(s->arl_table_Mutex, sal_mutex_FOREVER)
#define ARL_SW_TABLE_UNLOCK(s)   sal_mutex_give(s->arl_table_Mutex)
#define ARL_MEM_SEARCH_LOCK(s)   sal_mutex_take(s->arl_mem_search_Mutex, sal_mutex_FOREVER)
#define ARL_MEM_SEARCH_UNLOCK(s) sal_mutex_give(s->arl_mem_search_Mutex)
#define MEM_RWCTRL_REG_LOCK(s)   sal_mutex_take(s->mem_rwctrl_reg_Mutex, \
                                                sal_mutex_FOREVER)
#define MEM_RWCTRL_REG_UNLOCK(s) sal_mutex_give(s->mem_rwctrl_reg_Mutex)

#ifdef BCM_CMICM_SUPPORT
extern int
soc_host_ccm_copy(int unit, void *srcbuf, void *dstbuf,
                  int count, int endian);
#endif

/*
 * Write internal memories
 * NOTE: GBP/CBP memory is only accessible when in DEBUG mode.
 */
extern int soc_mem_write(int unit,
                         soc_mem_t mem,
                         int copyno, /* COPYNO_ALL allowed */
                         int index,
                         void *entry_data);

extern int soc_mem_write_extended(int unit, uint32 flags,
                         soc_mem_t mem,
                         int copyno, /* COPYNO_ALL allowed */
                         int index,
                         void *entry_data);
/*
 * Write internal memory arrays
 * NOTE: GBP/CBP memory is only accessible when in DEBUG mode.
 */
extern int soc_mem_array_write(int unit,
                               soc_mem_t mem,
                               unsigned array_index, /* index/element in the array */
                               int copyno,           /* COPYNO_ALL allowed */
                               int index,
                               void *entry_data);
extern int soc_mem_array_write_extended(int unit, uint32 flags,
                               soc_mem_t mem,
                               unsigned array_index, /* index/element in the array */
                               int copyno,           /* COPYNO_ALL allowed */
                               int index,
                               void *entry_data);

extern int soc_mem_write_range(int unit, /* DMA-accelerated on XGS3 chips */
                               soc_mem_t mem,
                               int copyno,
                               int index_min,
                               int index_max,
                               void *entry_array);

extern int soc_mem_array_write_range(int unit, /* DMA-accelerated on XGS3 chips */
                                     uint32 flags,
                                     soc_mem_t mem,
                                     unsigned array_index,
                                     int copyno,
                                     int index_min,
                                     int index_max,
                                     void *entry_array);

/* Validate if memory field is valid.*/
extern int soc_mem_field_valid(int unit, soc_mem_t mem, soc_field_t field);
#define SOC_MEM_FIELD_VALID(_u_,_mem_,_fld_) \
    soc_mem_field_valid(_u_, _mem_, _fld_)

/*
 * Read internal memories
 * NOTE: GBP/CBP memory is only accessible when in DEBUG mode.
 */
extern int soc_mem_read(int unit,
                        soc_mem_t mem,
                        int copyno,
                        int index,
                        void *entry_data);

extern int soc_mem_read_no_cache(int unit,
                                 soc_mem_t mem,
                                 int copyno,
                                 int index,
                                 void *entry_data);
/*
 * Read internal memories and pass required flags
 */
extern int soc_mem_read_extended(int unit, uint32 flags,
                                 soc_mem_t mem,
                                 unsigned array_index, /* index/element in the array */
                                 int copyno,
                                 int index,
                                 void *entry_data);

/*
 * Read internal memory arrays
 * NOTE: GBP/CBP memory is only accessible when in DEBUG mode.
 */
extern int soc_mem_array_read(int unit,
                              soc_mem_t mem,
                              unsigned array_index, /* index/element in the array */
                              int copyno,
                              int index,
                              void *entry_data);

extern int soc_mem_read_range(int unit, /* DMA-accelerated on XGS chips */
                              soc_mem_t mem,
                              int copyno,
                              int index_min,
                              int index_max,
                              void *entry_array);

extern int soc_mem_ser_read_range(int unit,
                                  soc_mem_t mem,
                                  int copyno,
                                  int index_min,
                                  int index_max,
                                  uint32 ser_flags,
                                  void *entry_array);

extern int soc_mem_array_read_range(int unit, /* DMA-accelerated on XGS chips */
                                    soc_mem_t mem,
                                    unsigned array_index,
                                    int copyno,
                                    int index_min,
                                    int index_max,
                                    void *entry_array);

extern int soc_mem_array_read_flags(int unit,
                                    soc_mem_t mem,
                                    unsigned array_index, /* index/element in the array */
                                    int copyno,
                                    int index,
                                    void *entry_data,
                                    int flags);

/*
 * Write internal memories while overriding pipe selection
 */
extern int soc_mem_pipe_select_write(int unit, uint32 flags,
                                     soc_mem_t mem,
                                     int copyno, /* COPYNO_ALL allowed */
                                     int acc_type,
                                     int index,
                                     void *entry_data);

/*
 * Read internal memories while overriding pipe selection
 */
extern int soc_mem_pipe_select_read(int unit,
                                    uint32 flags,
                                    soc_mem_t mem,
                                    int copyno, /* COPYNO_ALL allowed */
                                    int acc_type,
                                    int index,
                                    void *entry_data);

extern int soc_mem_dmaable(int unit, soc_mem_t mem, int copyno);
extern int soc_mem_slamable(int unit, soc_mem_t mem, int copyno);

/* Routines for tables that are maintained in sorted order */
extern int soc_mem_clear(int unit,
                         soc_mem_t mem,
                         int copyno, /* COPYNO_ALL allowed */
                         int force_all);

extern int soc_mem_field_clear_all(int unit,
                                   soc_mem_t mem,
                                   soc_field_t field,
                                   int copyno, /* COPYNO_ALL allowed */
                                   int force_all);

extern int soc_mem_search(int unit,
                          soc_mem_t mem,
                          int copyno,
                          int *index_ptr,
                          void *key_data,
                          void *entry_data,
                          int lowest_match);

extern int soc_mem_field32_modify(int unit, soc_mem_t mem, int index, 
                                  soc_field_t field, uint32 value);
extern int soc_mem_fields32_modify(int unit, soc_mem_t mem, int index, 
                                   int field_count, soc_field_t *fields,
                                   uint32 *values);
extern int soc_mem_field32_fit(int unit, soc_mem_t mem, 
                               soc_field_t field, uint32 value);

/* Return code that may be returned by soc_mem_search (filters only) */
#define SOC_MEM_PARTIAL_MATCH 1

extern int soc_mem_insert(int unit,
                          soc_mem_t mem,
                          int copyno, /* COPYNO_ALL allowed */
                          void *entry_data);

extern int soc_mem_bank_insert(int unit,
                               soc_mem_t mem,
                               int32 banks,
                               int copyno,
                               void *entry_data,
                               void *old_entry_data);

extern int soc_mem_insert_return_old(int unit, 
                                     soc_mem_t mem, 
                                     int copyno,
                                     void *entry_data, 
                                     void *old_entry_data);

extern int soc_mem_delete_index(int unit,
                                soc_mem_t mem,
                                int copyno, /* COPYNO_ALL allowed */
                                int index);

extern int soc_mem_delete(int unit,
                          soc_mem_t mem,
                          int copyno, /* COPYNO_ALL allowed */
                          void *key_data);

extern int soc_mem_delete_return_old(int unit, 
                                     soc_mem_t mem, 
                                     int copyno,
                                     void *key_data, 
                                     void *old_entry_data);

extern int soc_mem_pop(int unit,
                       soc_mem_t mem,
                       int copyno, /* COPYNO_ALL allowed */
                       void *entry_data);

extern int soc_mem_push(int unit,
                        soc_mem_t mem,
                        int copyno, /* COPYNO_ALL allowed */
                        void *entry_data);

#define SOC_MEM_FIFO_DMA_CHANNEL_0 0x0
#define SOC_MEM_FIFO_DMA_CHANNEL_1 0x1
#define SOC_MEM_FIFO_DMA_CHANNEL_2 0x2
#define SOC_MEM_FIFO_DMA_CHANNEL_3 0x3

extern int soc_mem_fifo_dma_start(int unit, int chan,
                                  soc_mem_t mem, int copyno,
                                  int host_entries, void *host_buf);

extern int soc_mem_fifo_dma_stop(int unit, int chan);

extern int soc_mem_fifo_dma_get_read_ptr(int unit, int chan, void **host_ptr,
                                         int *count);

extern int soc_mem_fifo_dma_advance_read_ptr(int unit, int chan, int count);

#ifdef BCM_CMICM_SUPPORT
extern int soc_mem_fifo_dma_get_num_entries(int unit, int chan, int *count);

extern int soc_mem_fifo_dma_set_entries_read(int unit, int chan, uint32 num);
#endif /* BCM_CMICM_SUPPORT */

extern int soc_mem_cache_set(int unit,
                             soc_mem_t mem,
                             int copyno, /* COPYNO_ALL allowed */
                             int enable);
extern int soc_mem_cache_invalidate(int unit,
                                    soc_mem_t mem,
                                    int copyno,
                                    int index);

extern int soc_mem_cache_get(int unit,
                             soc_mem_t mem,
                             int copyno);
extern int soc_mem_cache_scache_init(int unit);
extern int soc_mem_cache_scache_sync(int unit);
extern int soc_mem_cache_scache_get(int unit);
extern int soc_mem_cache_scache_load(int unit, soc_mem_t mem, int *offset);

extern int soc_control_defip_scache_init(int unit);
extern int soc_control_defip_scache_sync(int unit);
extern int soc_control_defip_scache_get(int unit);
extern int soc_control_defip_scache_load(int unit, int *num_ipv6_128b_entries);

extern int soc_mem_entries(int unit,
                           soc_mem_t mem,
                           int copyno);

#define SOC_MEM_HASH_BANK0_BIT           0x1
#define SOC_MEM_HASH_BANK1_BIT           0x2
#define SOC_MEM_HASH_BANK0_ONLY          0x2
#define SOC_MEM_HASH_BANK1_ONLY          0x1
#define SOC_MEM_HASH_BANK_BOTH           0
#define SOC_MEM_HASH_BANK_ALL            -1
#define SOC_MEM_DUAL_HASH_RECURSE_DEPTH  0
#define SOC_MEM_MULTI_HASH_RECURSE_DEPTH 0

extern int soc_mem_generic_lookup(int unit, 
                                  soc_mem_t mem, 
                                  int copyno,
                                  int32 banks, 
                                  void *key, 
                                  void *result, 
                                  int *index_ptr);

extern int soc_mem_generic_insert(int unit, 
                                  soc_mem_t mem, 
                                  int copyno,
                                  int32 banks, 
                                  void *entry, 
                                  void *old_entry, 
                                  int *index_ptr);

extern int soc_mem_generic_delete(int unit, 
                                  soc_mem_t mem, 
                                  int copyno,
                                  int32 banks, 
                                  void *entry, 
                                  void *old_entry, 
                                  int *index_ptr);

/* ALPM Memory functions */
extern int soc_mem_alpm_lookup(int unit, 
                               soc_mem_t mem, 
                               int bucket_index,
                               int copyno, 
                               uint32 banks_disable,
                               void *key,
                               void *result,
                               int *index_ptr);

extern int soc_mem_alpm_insert(int unit,
                               soc_mem_t mem,
                               int bucket_pointer,
                               int copyno,
                               int32 banks_disable,
                               void *entry,
                               void *old_entry, 
                               int *index_ptr);
extern int soc_mem_alpm_delete(int unit,
                               soc_mem_t mem, 
                               int bucket, 
                               int copyno,
                               int32 banks_disable,
                               void *entry, 
                               void *old_entry,
                               int *index_ptr);

extern int soc_mem_alpm_read(int unit,
                             soc_mem_t mem,
                             int copyno,
                             int bank,
                             int index,
                             int entry,
                             void *entry_data);

extern int soc_mem_alpm_write(int unit,
                              soc_mem_t mem,
                              int copyno,    /* Use COPYNO_ALL for all */
                              int bank,
                              int index,
                              int entry,
                              void *entry_data);



/*
 * Putting the MMU in debug mode quiesces packet activity in and out of
 * the MMU, so it is safe to read/write GBP/CBP memory.
 */
extern int soc_mem_debug_set(int unit, int enable);
extern int soc_mem_debug_get(int unit, int *enable);

extern int soc_robo_mem_debug_set(int unit, int enable);
extern int soc_robo_mem_debug_get(int unit, int *enable);

#ifdef SOC_MEM_L3_DEFIP_WAR
extern int soc_fb_l3_defip_index_map(int unit, int index);
#endif
/*
 * Map an index in L3_DEFIP to the real h/w index
 */

extern int soc_tr2_l3_defip_index_map(int unit, int index);

#ifdef INCLUDE_MEM_SCAN
/*
 * Memory error scan thread management
 */
extern int soc_mem_scan_running(int unit, int *rate, sal_usecs_t *interval);
extern int soc_mem_scan_start(int unit, int rate, sal_usecs_t interval);
extern int soc_mem_scan_stop(int unit);
extern void soc_mem_scan_ser_list_register(int unit, int generic,
                                           void *ser_info);
extern void soc_mem_scan_tcam_cache_update(int unit, soc_mem_t mem,
                                           int index_begin, int index_end,
                                           uint32 *xy_entries);
#endif /* INCLUDE_MEM_SCAN */

/* Function for memory iteration. */
typedef int (*soc_mem_iter_f)(int unit, soc_mem_t mem, void *data);
extern int soc_mem_iterate(int unit, soc_mem_iter_f do_it, void *data);
#define MEM_RW_CTRL_ADDR        drv_reg_addr(unit, GENMEM_CTLr, 0, 0)
#define MEM_RW_DATA_ADDR        drv_reg_addr(unit, GENMEM_DATAr, 0, 0)
#define MEM_RW_ADDR_ADDR        drv_reg_addr(unit, GENMEM_ADDRr, 0, 0)


#define SEC_MAC_READ_ADDR       soc_reg_addr(unit, I3E1Q_VLAN_TAB_READr, 0, 0)
#define VLAN_TABLE_READ_ADDR    soc_reg_addr(unit, I3E1Q_VLAN_TAB_READr, 0, 0)

#define VLAN_TABLE_WRITE_ADDR   soc_reg_addr(unit, I3E1Q_VLAN_TAB_WRITEr, 0, 0)

#define MEM_TABLE_READ  1       /* For Memory Read Operation */
#define MEM_TABLE_WRITE 0       /* For Memory Write Operation */
#define ADDR_TBL_MEM_ADDR_ADDR  \
    soc_reg_addr(unit, GMEM_ACES_ADDRTBL_MEMADDRr, 0, 0)
#define ADDR_TBL_MEM_DATA_ADDR  \
    soc_reg_addr(unit, GMEM_ACES_ADDRTBL_MEMDATAr, 0, 0)

#if defined(SER_TR_TEST_SUPPORT)
#define SOC_INVALID_TCAM_PARITY_BIT -1
#define SOC_MAX_NAME_LEN 400

#define SOC_MEM_PIPE_GROUP 0
#define SOC_MEM_PIPE_X     1
#define SOC_MEM_PIPE_Y     2
#define SOC_MEM_PIPE_ANY  -1
/*Flags for soc_ser_test_inject_full*/
#define SOC_INJECT_ERROR_NO_FLAGS  0
#define SOC_INJECT_ERROR_TCAM_FLAG 1
/*Flags for soc_ser_test_overlays*/
#define SOC_SER_OVERLAY_TEST_NO_FLAG 0
#define SOC_SER_OVERLAY_TEST_YPIPE   1
typedef enum {
    SER_SINGLE_INDEX,
    SER_FIRST_MID_LAST_INDEX,
    SER_ALL_INDEXES
} _soc_ser_test_t;
typedef struct soc_ser_overlay_test_s {
    soc_mem_t       mem;
    soc_reg_t       parity_enable_reg;
    soc_field_t     parity_enable_field;
    soc_mem_t       base_mem;
    soc_acc_type_t  acc_type;
} soc_ser_overlay_test_t;
typedef struct ser_test_data_s {
    soc_mem_t      mem;
    soc_reg_t      parity_enable_reg;
    soc_field_t    parity_enable_field;
    int            tcam_parity_bit;
    soc_block_t    mem_block;
    soc_port_t     port;
    soc_mem_info_t *mem_info;
    soc_field_t    test_field;
    soc_acc_type_t acc_type;
    int            index;
    uint32         *entry_buf;
    uint32         *field_buf;
    char           mem_name[SOC_MAX_NAME_LEN];
    char           field_name[SOC_MAX_NAME_LEN];
    uint32         badData;
} ser_test_data_t;
typedef struct soc_ser_test_functions_s {
    soc_error_t (*inject_error_f)(int, uint32, soc_mem_t, int, int, int);
    soc_error_t (*test_mem)(int, soc_mem_t, _soc_ser_test_t, int);
    soc_error_t (*test)(int, _soc_ser_test_t);
    soc_error_t (*parity_control)(int, ser_test_data_t *, int);
    soc_error_t (*injection_support)(int, soc_mem_t, int);
} soc_ser_test_functions_t;

extern int soc_ser_test_long_sleep;
extern void soc_ser_test_functions_register(int unit,
                            soc_ser_test_functions_t *fun);
extern soc_error_t soc_ser_inject_error(int unit, uint32 flags,soc_mem_t mem,
                                        int pipe, int blk, int index);
extern soc_error_t soc_ser_inject_support(int unit, soc_mem_t mem, int pipe);
extern soc_error_t soc_ser_test(int unit, _soc_ser_test_t testType);
extern soc_error_t soc_ser_test_mem(int unit, soc_mem_t mem,
                            _soc_ser_test_t testType, int cmd);
extern soc_error_t ser_test_mem_write(int unit, ser_test_data_t *test_data);
extern soc_error_t ser_test_mem_read(int unit, ser_test_data_t *test_data);
extern soc_error_t soc_ser_test_inject_full(int unit, uint32 flags,
                                            ser_test_data_t *test_data);
extern soc_error_t soc_ser_test_inject_error(int unit,
                            ser_test_data_t *test_data);
extern void ser_test_cmd_generate(int unit, ser_test_data_t *test_data);
extern void soc_ser_create_test_data(int unit, uint32 *tmp_entry,
                            uint32 *field_data, soc_reg_t parity_enable_reg,
                            int tcam_parity_bit, soc_field_t hw_parity_field,
                            soc_mem_t mem, soc_field_t test_field,
                            soc_block_t mem_block, soc_port_t port,
                            soc_acc_type_t acc_type, int index,
                            ser_test_data_t *test_data);
extern soc_error_t  ser_test_mem(int unit, ser_test_data_t *test_data,
                            _soc_ser_test_t test_type, int *error_count);
extern int soc_ser_test_overlays(int unit, _soc_ser_test_t test_type,
                                 const soc_ser_overlay_test_t *overlays,
                                 int(*pipe_select)(int,int,int) );
extern soc_error_t ser_test_mem_pipe(int unit, soc_reg_t parity_enable_reg,
                            int tcam_parity_bit, soc_field_t hw_parity_field,
                            soc_mem_t mem, soc_field_t test_field,
                            _soc_ser_test_t test_type, soc_block_t mem_block,
                            soc_port_t port, int pipe, int * error_count);
extern soc_error_t _ser_test_parity_control(int unit,ser_test_data_t *test_data,
                            int enable);
extern soc_error_t _ser_test_parity_control_reg_set(int unit,
                            ser_test_data_t *test_data, int enable);
extern soc_error_t _ser_test_parity_control_pci_write(int unit,
                            ser_test_data_t *test_data, int enable);
#endif    /*defined(SER_TR_TEST_SUPPORT)*/

extern int soc_l3_defip_index_map(int unit, int wide, int index);
extern int soc_l3_defip_urpf_index_map(int unit, int wide, int index);
extern int soc_l3_defip_index_remap(int unit, int wide, int index);
extern int soc_l3_defip_urpf_index_remap(int unit, int wide, int index);
extern int soc_l3_defip_index_mem_map(int unit, int index, soc_mem_t* mem);
extern int soc_l3_defip_indexes_init(int unit, int val);
extern int soc_defip_tables_resize(int unit, int num_ipv6_128b_entries);
extern void soc_mem_config_save(int unit, char *config_str, char *config_value);
#endif    /* !_SOC_MEM_H */
