/*
 * $Id: stat.h,v 1.28 Broadcom SDK $
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
 * This file contains STAT definitions internal to the BCM library.
 */

#ifndef _BCM_INT_STAT_H
#define _BCM_INT_STAT_H

#include <bcm/stat.h>

/*
 * Utility routines for statistics accumulation
 */

/* The REG_* macros require the following declaration in any function which
 * uses them:
 */

#define REG_MATH_DECL \
        uint64 reg_val

#define REG_ADD(unit, port, sync_mode, reg, val)                           \
    if (SOC_REG_IS_VALID(unit, reg) && SOC_REG_IS_COUNTER(unit, reg)) {    \
        if (sync_mode) {                                                   \
            SOC_IF_ERROR_RETURN(soc_counter_sync_get(unit, port, reg,      \
                                                0, &reg_val));             \
        } else {                                                           \
            SOC_IF_ERROR_RETURN(soc_counter_get(unit, port, reg,           \
                                                0, &reg_val));             \
        }                                                                  \
        COMPILER_64_ADD_64(val, reg_val);                                  \
    }
#define REG_ADD_IDX(unit, port, sync_mode, reg, val, ar_idx)               \
    if (SOC_REG_IS_VALID(unit, reg) && SOC_REG_IS_COUNTER(unit, reg)) {    \
        if (sync_mode) {                                                   \
            SOC_IF_ERROR_RETURN(soc_counter_sync_get(unit, port, reg,      \
                                                ar_idx, &reg_val));        \
        } else {                                                           \
            SOC_IF_ERROR_RETURN(soc_counter_get(unit, port, reg,           \
                                                ar_idx, &reg_val));        \
        }                                                                  \
        COMPILER_64_ADD_64(val, reg_val);                                  \
    }
#define REG_SUB(unit, port, sync_mode, reg, val)                           \
    if (SOC_REG_IS_VALID(unit, reg) && SOC_REG_IS_COUNTER(unit, reg)) {    \
        if (sync_mode) {                                                   \
            SOC_IF_ERROR_RETURN(soc_counter_sync_get(unit, port, reg,      \
                                                0, &reg_val));             \
        } else {                                                           \
            SOC_IF_ERROR_RETURN(soc_counter_get(unit, port, reg,           \
                                                0, &reg_val));             \
        }                                                                  \
        if (COMPILER_64_GT(val, reg_val)) {                                \
            COMPILER_64_SUB_64(val, reg_val);                              \
        } else {                                                           \
            COMPILER_64_ZERO(val);                                         \
        }                                                                  \
    }
#define REG_SUB_IDX(unit, port, sync_mode, reg, val, ar_idx)               \
    if (SOC_REG_IS_VALID(unit, reg) && SOC_REG_IS_COUNTER(unit, reg)) {    \
        if (sync_mode) {                                                   \
            SOC_IF_ERROR_RETURN(soc_counter_sync_get(unit, port, reg,      \
                                                ar_idx, &reg_val));        \
        } else {                                                           \
            SOC_IF_ERROR_RETURN(soc_counter_get(unit, port, reg,           \
                                                ar_idx, &reg_val));        \
        }                                                                  \
        COMPILER_64_SUB_64(val, reg_val);                                  \
    }

/*
 * For collecting addition non-DMA counters, unlike the non-DMA counter
 * in the soc_counter_non_dma_t, the counters collected here are not shown
 * in "show counter" command.
 */
enum {
    _BCM_STAT_EXTRA_COUNTER_EGRDROPPKTCOUNT = 0,
    _BCM_STAT_EXTRA_COUNTER_COUNT
};
typedef struct _bcm_stat_extra_counter_s {
    soc_reg_t reg;
    uint32 *ctr_prev;
    uint64 *count64;
} _bcm_stat_extra_counter_t;

/* Oversize packet error control */
extern SHR_BITDCL *_bcm_stat_ovr_control;

#define COUNT_OVR_ERRORS(unit)                                    \
    (soc_feature(unit, soc_feature_stat_jumbo_adj) &&             \
     ((_bcm_stat_ovr_control != NULL) &&                          \
      (SHR_BITGET(&_bcm_stat_ovr_control[(unit)],(port)))))

/* Oversize packet size threshold accessor functions */
extern int _bcm_esw_stat_ovr_threshold_set(int unit, bcm_port_t port, int value);
extern int _bcm_esw_stat_ovr_threshold_get(int unit, bcm_port_t port, int *value);
extern int _bcm_esw_stat_ovr_error_control_set(int unit, bcm_port_t port, int value);
extern int _bcm_esw_stat_ovr_error_control_get(int unit, bcm_port_t port, int *value);


/* Library-private functions exported from stat_fe.c */

extern int _bcm_stat_fe_get(int unit, bcm_port_t port, int sync_mode, 
                            bcm_stat_val_t type, uint64 *val);

/* Library-private functions exported from stat_ge.c */

extern int _bcm_stat_ge_get(int unit, bcm_port_t port, int sync_mode, 
                            bcm_stat_val_t type, uint64 *val,
                            int incl_non_ge_stat);

/* Library-private functions exported from stat_xe.c */

extern int _bcm_stat_xe_get(int unit, bcm_port_t port, int sync_mode, 
                            bcm_stat_val_t type, uint64 *val);

/* Library-private functions exported from stat_hg.c */

extern int _bcm_stat_hg_get(int unit, bcm_port_t port, int sync_mode, 
                            bcm_stat_val_t type, uint64 *val);

/* Library-private functions exported from stat_generic.c */

extern int _bcm_stat_generic_get(int unit, bcm_port_t port, int sync_mode,
                                 bcm_stat_val_t type, uint64 *val);

extern int _bcm_stat_counter_extra_get(int unit, soc_reg_t reg,
                                       soc_port_t port, uint64 *val);
extern int _bcm_stat_counter_non_dma_extra_get(int unit,
                                  soc_counter_non_dma_id_t non_dma_id,
                                                    soc_port_t port,
                                                    uint64 *val);
extern int _bcm_esw_stat_detach(int unit);
extern int _bcm_esw_stat_sync(int unit);

/* Stat chunks for warm-boot functionality */
#ifdef BCM_WARM_BOOT_SUPPORT
#define _BCM_STAT_WARM_BOOT_CHUNK_PORTS    0
#define _BCM_STAT_WARM_BOOT_CHUNK_FLEX     1
extern int _bcm_esw_stat_sync_version_above_equal(int unit,uint16 version);
#endif

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_stat_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* !_BCM_INT_STAT_H */
