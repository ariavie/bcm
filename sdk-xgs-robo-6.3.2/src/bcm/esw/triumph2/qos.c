/*
 * $Id: qos.c 1.84.4.1 Broadcom SDK $
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
 * All Rights Reserved.$
 *
 * QoS module
 */
#include <soc/defs.h>

#include <assert.h>

#include <sal/core/libc.h>
#if defined(BCM_TRIUMPH2_SUPPORT)

#include <shared/util.h>
#include <soc/mem.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <bcm/qos.h>
#include <bcm/error.h>
#include <bcm_int/esw/triumph2.h>
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw_dispatch.h> 
#include <bcm_int/esw/switch.h>
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
#include <bcm_int/esw/mpls.h>
#endif /* INCLUDE_L3 && BCM_MPLS_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#endif

#define _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP    1
#define _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS      2
#define _BCM_QOS_MAP_TYPE_DSCP_TABLE         3
#define _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE     4
#define _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE   5
#define _BCM_QOS_MAP_TYPE_MASK           0x3ff
#define _BCM_QOS_MAP_SHIFT                  10

#define _BCM_QOS_MAP_CHUNK_PRI_CNG  16
#define _BCM_QOS_MAP_CHUNK_EGR_MPLS  64
#define _BCM_QOS_MAP_CHUNK_DSCP  64
#define _BCM_QOS_MAP_CHUNK_EGR_DSCP  64
#define _BCM_QOS_MAP_CHUNK_ING_MPLS_EXP  8
#define _BCM_QOS_MAP_CHUNK_OFFSET_MAP  16

#define _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP    \
            (soc_mem_index_count(unit, ING_PRI_CNG_MAPm)/ \
             _BCM_QOS_MAP_CHUNK_PRI_CNG)
#define _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS      \
            (soc_mem_index_count(unit, EGR_MPLS_PRI_MAPPINGm)/ \
             _BCM_QOS_MAP_CHUNK_EGR_MPLS)
#define _BCM_QOS_MAP_LEN_DSCP_TABLE         \
            (soc_mem_index_count(unit, DSCP_TABLEm)/ \
             _BCM_QOS_MAP_CHUNK_DSCP)
#define _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE     \
            (soc_mem_index_count(unit, EGR_DSCP_TABLEm)/ \
             _BCM_QOS_MAP_CHUNK_EGR_DSCP)
#define _BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP   \
            (soc_mem_is_valid(unit, ING_MPLS_EXP_MAPPINGm) ? \
             (soc_mem_index_count(unit, ING_MPLS_EXP_MAPPINGm)/ \
              _BCM_QOS_MAP_CHUNK_ING_MPLS_EXP) : 0)
#ifdef BCM_KATANA_SUPPORT
#define _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE         \
            (soc_mem_is_valid(unit, ING_QUEUE_OFFSET_MAPPING_TABLEm) ? \
            (soc_mem_index_count(unit, ING_QUEUE_OFFSET_MAPPING_TABLEm)/ \
             _BCM_QOS_MAP_CHUNK_OFFSET_MAP) : 0)
#endif
#define _BCM_QOS_GPORT_IS_VFI_VP_PORT(port) \
    (BCM_GPORT_IS_MIM_PORT(port) ||   \
     BCM_GPORT_IS_MPLS_PORT(port) || \
     BCM_GPORT_IS_VXLAN_PORT(port) || \
     BCM_GPORT_IS_L2GRE_PORT(port))

typedef struct _bcm_tr2_qos_bookkeeping_s {
    SHR_BITDCL *ing_pri_cng_bitmap; /* ING_PRI_CNG_MAP chunks used */
    uint32 *pri_cng_hw_idx; /* Actual profile number used */
    SHR_BITDCL *egr_mpls_bitmap; /* EGR_MPLS_EXP_MAPPING_1 / 
                                  EGR_MPLS_PRI_MAPPING chunks used */
    uint32 *egr_mpls_hw_idx; /* Actual profile number used */
    SHR_BITDCL *dscp_table_bitmap; /* DSCP_TABLE chunks used */
    uint32 *dscp_hw_idx; /* Actual profile number used */
    SHR_BITDCL *egr_dscp_table_bitmap; /* EGR_DSCP_TABLE chunks used */
    uint32 *egr_dscp_hw_idx; /* Actual profile number used */
    SHR_BITDCL *egr_mpls_bitmap_flags; /* indicates if egr_mpls_bitmap
                                          entry is L2 or MPLS */
    SHR_BITDCL *ing_mpls_exp_bitmap; /* indicates which chunks of 
                                ING_MPLS_EXP_MAPPINGm are used by QOS module */
#ifdef BCM_KATANA_SUPPORT
    SHR_BITDCL *offset_map_table_bitmap; /* ING_QUEUE_OFFSET_MAPPING_TABLE
                                            chunks used */
    uint32     *offset_map_hw_idx;   /* Actual profile number used */
#endif
} _bcm_tr2_qos_bookkeeping_t;

STATIC _bcm_tr2_qos_bookkeeping_t  _bcm_tr2_qos_bk_info[BCM_MAX_NUM_UNITS];
STATIC sal_mutex_t _tr2_qos_mutex[BCM_MAX_NUM_UNITS] = {NULL};
STATIC int tr2_qos_initialized[BCM_MAX_NUM_UNITS]; /* Flag to check init status */

/*
 * QoS resource lock
 */
#define QOS_LOCK(unit) \
        sal_mutex_take(_tr2_qos_mutex[unit], sal_mutex_FOREVER);

#define QOS_UNLOCK(unit) \
        sal_mutex_give(_tr2_qos_mutex[unit]);

#define QOS_INFO(_unit_) (&_bcm_tr2_qos_bk_info[_unit_])
#define DSCP_CODE_POINT_CNT 64
#define DSCP_CODE_POINT_MAX (DSCP_CODE_POINT_CNT - 1)

/*
 * ING_PRI_CNG_MAP usage bitmap operations
 */
#define _BCM_QOS_ING_PRI_CNG_USED_GET(_u_, _identifier_) \
        SHR_BITGET(QOS_INFO(_u_)->ing_pri_cng_bitmap, (_identifier_))
#define _BCM_QOS_ING_PRI_CNG_USED_SET(_u_, _identifier_) \
        SHR_BITSET(QOS_INFO((_u_))->ing_pri_cng_bitmap, (_identifier_))
#define _BCM_QOS_ING_PRI_CNG_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(QOS_INFO((_u_))->ing_pri_cng_bitmap, (_identifier_))

/*
 * EGR_MPLS_EXP_MAPPING_1 / EGR_MPLS_PRI_MAPPING usage bitmap operations
 */
#define _BCM_QOS_EGR_MPLS_USED_GET(_u_, _identifier_) \
        SHR_BITGET(QOS_INFO(_u_)->egr_mpls_bitmap, (_identifier_))
#define _BCM_QOS_EGR_MPLS_USED_SET(_u_, _identifier_) \
        SHR_BITSET(QOS_INFO((_u_))->egr_mpls_bitmap, (_identifier_))
#define _BCM_QOS_EGR_MPLS_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(QOS_INFO((_u_))->egr_mpls_bitmap, (_identifier_))

/*
 * DSCP_TABLE usage bitmap operations
 */
#define _BCM_QOS_DSCP_TABLE_USED_GET(_u_, _identifier_) \
        SHR_BITGET(QOS_INFO(_u_)->dscp_table_bitmap, (_identifier_))
#define _BCM_QOS_DSCP_TABLE_USED_SET(_u_, _identifier_) \
        SHR_BITSET(QOS_INFO((_u_))->dscp_table_bitmap, (_identifier_))
#define _BCM_QOS_DSCP_TABLE_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(QOS_INFO((_u_))->dscp_table_bitmap, (_identifier_))

/*
 * EGR_DSCP_TABLE usage bitmap operations
 */
#define _BCM_QOS_EGR_DSCP_TABLE_USED_GET(_u_, _identifier_) \
        SHR_BITGET(QOS_INFO(_u_)->egr_dscp_table_bitmap, (_identifier_))
#define _BCM_QOS_EGR_DSCP_TABLE_USED_SET(_u_, _identifier_) \
        SHR_BITSET(QOS_INFO((_u_))->egr_dscp_table_bitmap, (_identifier_))
#define _BCM_QOS_EGR_DSCP_TABLE_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(QOS_INFO((_u_))->egr_dscp_table_bitmap, (_identifier_))

/*
 * EGR_MPLS flags bitmap operations
 */
#define _BCM_QOS_EGR_MPLS_FLAGS_USED_GET(_u_, _identifier_) \
        SHR_BITGET(QOS_INFO(_u_)->egr_mpls_bitmap_flags, (_identifier_))
#define _BCM_QOS_EGR_MPLS_FLAGS_USED_SET(_u_, _identifier_) \
        SHR_BITSET(QOS_INFO((_u_))->egr_mpls_bitmap_flags, (_identifier_))
#define _BCM_QOS_EGR_MPLS_FLAGS_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(QOS_INFO((_u_))->egr_mpls_bitmap_flags, (_identifier_))

/*
 * ING_MPLS_EXP_MAPPING bitmap operations
 */
#define _BCM_QOS_ING_MPLS_EXP_USED_GET(_u_, _identifier_) \
        SHR_BITGET(QOS_INFO(_u_)->ing_mpls_exp_bitmap, (_identifier_))
#define _BCM_QOS_ING_MPLS_EXP_USED_SET(_u_, _identifier_) \
        SHR_BITSET(QOS_INFO((_u_))->ing_mpls_exp_bitmap, (_identifier_))
#define _BCM_QOS_ING_MPLS_EXP_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(QOS_INFO((_u_))->ing_mpls_exp_bitmap, (_identifier_))

#define QOS_INIT(unit)                                    \
    do {                                                  \
        if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {  \
            return BCM_E_UNIT;                            \
        }                                                 \
        if (!tr2_qos_initialized[unit]) {                           \
            return BCM_E_INIT;                            \
        }                                                 \
    } while (0)

/*
 * ING_QUEUE_OFFSET_MAPPING_TABLE usage bitmap operations
 */
#ifdef BCM_KATANA_SUPPORT
#define _BCM_QOS_OFFSET_MAP_TABLE_USED_GET(_u_, _identifier_) \
        SHR_BITGET(QOS_INFO(_u_)->offset_map_table_bitmap, (_identifier_))
#define _BCM_QOS_OFFSET_MAP_TABLE_USED_SET(_u_, _identifier_) \
        SHR_BITSET(QOS_INFO((_u_))->offset_map_table_bitmap, (_identifier_))
#define _BCM_QOS_OFFSET_MAP_TABLE_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(QOS_INFO((_u_))->offset_map_table_bitmap, (_identifier_))

#endif

STATIC int
_bcm_tr2_qos_id_alloc(int unit, SHR_BITDCL *bitmap, uint8 map_type)
{
    int i, map_size = -1;

    switch (map_type) {
        case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
            map_size = _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP;
            break;
        case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
            map_size = _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS;
            break;
        case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
            map_size = _BCM_QOS_MAP_LEN_DSCP_TABLE;
            break;
        case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
            map_size = _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE;
            break;
#ifdef BCM_KATANA_SUPPORT
        case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
            map_size = _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE;
            break;
#endif
        default:
            return BCM_E_PARAM;
    }

    for (i = 0; i < map_size; i++) {
        if (!SHR_BITGET(bitmap, i)) {
            return i;
        }
    }
    return -1;
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/* 
 * Function:
 *      _bcm_qos_sw_dump
 * Purpose:
 *      Displays QOS software state info
 * Parameters:
 *      unit        : (IN) Device Unit Number
 * Returns:
 *      NONE
 */
void
_bcm_tr2_qos_sw_dump(int unit)
{
    int     i;

    if (!tr2_qos_initialized[unit]) {
        soc_cm_print("ERROR: QOS module not initialized on Unit:%d \n", unit);
        return;
    }

    soc_cm_print("QOS: ING_PRI_CNG_MAP info \n");
    for (i=0; i<_BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP; i++) {
        if (_BCM_QOS_ING_PRI_CNG_USED_GET(unit, i)) {
            soc_cm_print("    map id:%4d    HW index:%4d\n", i, 
                         QOS_INFO(unit)->pri_cng_hw_idx[i]);
        }
    }

    soc_cm_print("QOS: EGR_MPLS_PRI_MAPPING info \n");
    for (i=0; i<_BCM_QOS_MAP_LEN_EGR_MPLS_MAPS; i++) {
        if (_BCM_QOS_EGR_MPLS_USED_GET(unit, i)) {
            soc_cm_print("    map id:%4d    HW index:%4d (%s)\n", i, 
                QOS_INFO(unit)->egr_mpls_hw_idx[i],
                (_BCM_QOS_EGR_MPLS_FLAGS_USED_GET(unit, i)? "MPLS" : "L2"));
        }
    }

    soc_cm_print("QOS: DSCP_TABLE info \n");
    for (i=0; i<_BCM_QOS_MAP_LEN_DSCP_TABLE; i++) {
        if (_BCM_QOS_DSCP_TABLE_USED_GET(unit, i)) {
            soc_cm_print("    map id:%4d    HW index:%4d\n", i, 
                         QOS_INFO(unit)->dscp_hw_idx[i]);
        }
    }

    soc_cm_print("QOS: EGR_DSCP_TABLE info \n");
    for (i=0; i<_BCM_QOS_MAP_LEN_EGR_DSCP_TABLE; i++) {
        if (_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, i)) {
            soc_cm_print("    map id:%4d    HW index:%4d\n", i, 
                         QOS_INFO(unit)->egr_dscp_hw_idx[i]);
        }
    }
#ifdef BCM_KATANA_SUPPORT
    soc_cm_print("QOS: OFFSET_MAP_TABLE info \n");
    for (i=0; i < _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE; i++) {
        if (_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, i)) {
            soc_cm_print("    map id:%4d    HW index:%4d\n", i,
                         QOS_INFO(unit)->offset_map_hw_idx[i]);
        }
    }
#endif
    soc_cm_print("QOS: ING_MPLS_EXP_MAPPING info \n");
    for (i=0; i<_BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP; i++) {
        if (_BCM_QOS_ING_MPLS_EXP_USED_GET(unit, i)) {
            soc_cm_print("    map id:%4d\n", i);
        }
    }
}
#endif

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0
#define _BCM_TR2_REINIT_INVALID_HW_IDX  0xff    /* used as an invalid hw idx */

/* 
 * Function:
 *      _bcm_tr2_qos_reinit_scache_len_get
 * Purpose:
 *      Helper utility to determine scache details.
 * Parameters:
 *      unit        : (IN) Device Unit Number
 *      scache_len  : (OUT) Total required scache length
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_qos_reinit_scache_len_get(int unit, uint32* scache_len)
{
    if (scache_len == NULL) {
        return BCM_E_PARAM;
    }

    *scache_len = _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP;
    *scache_len += _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS;
    *scache_len += _BCM_QOS_MAP_LEN_DSCP_TABLE;
    *scache_len += _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE;
    *scache_len += SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_EGR_MPLS_MAPS);
    *scache_len += SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP);
#ifdef BCM_KATANA_SUPPORT
    *scache_len += _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE;
#endif

    return BCM_E_NONE;
}

int
_bcm_tr2_qos_reinit_from_hw_state(int unit, soc_mem_t mem, soc_field_t field,
                                 uint8 map_type, SHR_BITDCL *hw_idx_bmp, 
                                 int hw_idx_bmp_len)
{
    int         rv = BCM_E_NONE;
    int         idx, min_idx, max_idx, map_id, hw_prof_idx, entry_type = 0;
    uint32      buf[SOC_MAX_MEM_FIELD_WORDS];
    uint32      *hw_idx_table;
    SHR_BITDCL  *map_bmp;

    switch (map_type) {
    case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
        map_bmp = QOS_INFO(unit)->ing_pri_cng_bitmap;
        hw_idx_table = QOS_INFO(unit)->pri_cng_hw_idx;
        break;
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
        map_bmp = QOS_INFO(unit)->egr_mpls_bitmap;
        hw_idx_table = QOS_INFO(unit)->egr_mpls_hw_idx;
        break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
        map_bmp = QOS_INFO(unit)->dscp_table_bitmap;
        hw_idx_table = QOS_INFO(unit)->dscp_hw_idx;
        break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
        map_bmp = QOS_INFO(unit)->egr_dscp_table_bitmap;
        hw_idx_table = QOS_INFO(unit)->egr_dscp_hw_idx;
        break;
#ifdef BCM_KATANA_SUPPORT
    case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
        map_bmp = QOS_INFO(unit)->offset_map_table_bitmap;
        hw_idx_table = QOS_INFO(unit)->offset_map_hw_idx;
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    min_idx = soc_mem_index_min(unit, mem);
    max_idx = soc_mem_index_max(unit, mem);
    for (idx = min_idx; idx < max_idx; idx++) {
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, idx, &buf);
        if (SOC_FAILURE(rv)) {
            SOC_ERROR_PRINT((DK_ERR, "Unit:%d, Error(%s) reading mem(%d) at "
                             "index:%d \n", unit, soc_errmsg(rv), mem, idx));
            return rv;
        }
        if (mem == EGR_L3_NEXT_HOPm) {
            entry_type = soc_mem_field32_get(unit, mem, &buf, ENTRY_TYPEf);
            if ((entry_type != 2/* SD-Tag */) && (entry_type != 3/* MIM */)) {
                continue; /* Neither SD-tag nor MiM view */
            }
        }
        if (mem == EGR_VLAN_XLATEm) {
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
                if (SOC_IS_TRIUMPH3(unit)) {
                    entry_type = soc_mem_field32_get(unit, mem, &buf, KEY_TYPEf);
                } else if (SOC_IS_TD2_TT2(unit)) {
                    entry_type = soc_mem_field32_get(unit, mem, &buf, ENTRY_TYPEf);
                }
                if ((entry_type != 2) && (entry_type != 3)) {
                    continue; /* Neither MIM_ISID nor MIM_ISID_DVP */
                }
            } else
#endif /* BCM_TRIUMPH3_SUPPORT */
            {
                entry_type = soc_mem_field32_get(unit, mem, &buf, ENTRY_TYPEf);
                if ((entry_type != 3) && (entry_type != 4)) {
                    continue; /* Neither ISID_XLATE nor ISID_DVP_XLATE */
                }
            }
        }
        /* Some tables have a bit that specifies whether the profile is valid */
        if (soc_mem_field_valid(unit, mem, TRUST_OUTER_DOT1Pf)) {
            if (soc_mem_field32_get(unit, mem, &buf, TRUST_OUTER_DOT1Pf) == 0) {
                continue;
            }
        }
        if (soc_mem_field_valid(unit, mem, TRUST_DOT1Pf)) {
            if (soc_mem_field32_get(unit, mem, &buf, TRUST_DOT1Pf) == 0) {
                continue;
            }
        }
        if (soc_mem_field_valid(unit, mem, SD_TAG__SD_TAG_DOT1P_PRI_SELECTf)) {
            if (soc_mem_field32_get(unit, mem, &buf, 
                                    SD_TAG__SD_TAG_DOT1P_PRI_SELECTf) == 0) {
                continue;
            }
        }
        if (soc_mem_field_valid(unit, mem, REMARK_DOT1Pf)) {
            if (soc_mem_field32_get(unit, mem, &buf, REMARK_DOT1Pf) == 0) {
                continue;
            }
        }
        if (soc_mem_field_valid(unit, mem, DOT1P_PRI_SELECTf)) {
            if (soc_mem_field32_get(unit, mem, &buf, DOT1P_PRI_SELECTf) == 0) {
                continue;
            }
        }
        if (soc_mem_field_valid(unit, mem, SD_TAG_DOT1P_PRI_SELECTf)) {
            if (soc_mem_field32_get(unit, mem, &buf, 
                                    SD_TAG_DOT1P_PRI_SELECTf) == 0) {
                continue;
            }
        }
        if (soc_mem_field_valid(unit, mem, 
                                MIM_ISID__SD_TAG_DOT1P_PRI_SELECTf)) {
            if (soc_mem_field32_get(unit, mem, &buf, 
                                    MIM_ISID__SD_TAG_DOT1P_PRI_SELECTf) == 0) {
                continue;
            }
        }
        if ((mem == EGR_IP_TUNNELm) || (mem == EGR_L3_INTFm)) {
            if (soc_mem_field32_get(unit, mem, &buf, DSCP_SELf) != 2) {
                continue;
            }
        }

        if (mem == L3_IIFm && field == TRUST_DSCP_PTRf) {
            int field_len = 0, iif_profile_idx = 0;
            iif_profile_entry_t l3_iif_profile;
            if (soc_feature(unit, soc_feature_l3_iif_profile)) {
                field_len = soc_mem_field_length(unit, L3_IIF_PROFILEm, field);
                iif_profile_idx = soc_mem_field32_get(unit, mem, &buf,
                                                      L3_IIF_PROFILE_INDEXf);
                BCM_IF_ERROR_RETURN(soc_mem_read(unit, L3_IIF_PROFILEm,
                                     SOC_BLOCK_ANY, iif_profile_idx,
                                     &l3_iif_profile));
                hw_prof_idx = soc_mem_field32_get(unit, L3_IIF_PROFILEm,
                                                  &l3_iif_profile,
                                                  TRUST_DSCP_PTRf);
            } else {
                field_len = soc_mem_field_length(unit, mem, field);
                hw_prof_idx = soc_mem_field32_get(unit, mem, &buf, field);
            }
            if (field_len == _BCM_TR_L3_TRUST_DSCP_PTR_BIT_SIZE) { 
                if (hw_prof_idx == 0x3f) {
                    continue;
                }
            } else if (field_len == _BCM_TD_L3_TRUST_DSCP_PTR_BIT_SIZE) {
                if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) { 
                    if (hw_prof_idx == 0x7f) {
                        continue;
                    } 
                } else {
                    if (hw_prof_idx == 0x0) {
                        continue;
                    }
                }
            }
        } else {
            hw_prof_idx = soc_mem_field32_get(unit, mem, &buf, field);
        }

        if (hw_prof_idx > (hw_idx_bmp_len - 1)) {
            SOC_ERROR_PRINT((DK_ERR, "Unit:%d, Invalid profile(%d) in mem(%d) "
                             "at index:%d\n", unit, hw_prof_idx, mem, idx));
            return BCM_E_INTERNAL;
        }
        if (hw_prof_idx && (!SHR_BITGET(hw_idx_bmp, hw_prof_idx))) {
            /* non-zero profile id and not stored previously */
            map_id = _bcm_tr2_qos_id_alloc(unit, map_bmp, map_type);
            if (map_id < 0) {
                SOC_ERROR_PRINT((DK_ERR, "Unit:%d, Invalid profile(%d) in mem"
                           "(%d) at index:%d\n", unit, hw_prof_idx, mem, idx));
                return BCM_E_RESOURCE;
            }
            hw_idx_table[map_id] = hw_prof_idx;
            SHR_BITSET(hw_idx_bmp, hw_prof_idx);
            SHR_BITSET(map_bmp, map_id);
        }
    }
    
    return rv;
}

/* 
 * Function:
 *      _bcm_tr2_qos_unsynchronized_reinit
 * Purpose:
 *      This routine handles Level 1 Warmboot init. In this style of reinit, the 
 *      map-ids allocated previously are not guaranteed to be the same.
 *      Cold-boot: Does nothing
 *      Warm-boot: The data-structures are populated by reading the HW tables. 
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_qos_unsynchronized_reinit(int unit)
{
    int                 rv = BCM_E_NONE;
    int                 bmp_len;
    SHR_BITDCL          *temp_bmp;

    /* read hw tables and populate the state */
    bmp_len = _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP;
    temp_bmp = sal_alloc(SHR_BITALLOCSIZE(bmp_len), "temp_bmp");
    sal_memset(temp_bmp, 0x0, SHR_BITALLOCSIZE(bmp_len));
    
    if (soc_mem_is_valid(unit, SOURCE_VPm)) {
        rv = _bcm_tr2_qos_reinit_from_hw_state(unit, SOURCE_VPm, 
                           TRUST_DOT1P_PTRf, _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP,
                           temp_bmp, bmp_len);
    }
    if (BCM_SUCCESS(rv) && soc_mem_is_valid(unit, LPORT_TABm)) {
        rv = _bcm_tr2_qos_reinit_from_hw_state(unit, LPORT_TABm, 
                           TRUST_DOT1P_PTRf, _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP,
                           temp_bmp, bmp_len);
    }
    if (BCM_SUCCESS(rv) && soc_mem_is_valid(unit, PORT_TABm)) {
        rv = _bcm_tr2_qos_reinit_from_hw_state(unit, PORT_TABm, 
                           TRUST_DOT1P_PTRf, _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP,
                           temp_bmp, bmp_len);
    }
    if (BCM_SUCCESS(rv) && soc_mem_field_valid(unit, VLAN_PROFILE_TABm, 
                                               TRUST_DOT1P_PTRf)) {
        rv = _bcm_tr2_qos_reinit_from_hw_state(unit, VLAN_PROFILE_TABm, 
                           TRUST_DOT1P_PTRf, _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP,
                           temp_bmp, bmp_len);
    }
    if (BCM_SUCCESS(rv) && soc_mem_field_valid(unit, VFIm, TRUST_DOT1P_PTRf)) {
        rv = _bcm_tr2_qos_reinit_from_hw_state(unit, VFIm, 
                           TRUST_DOT1P_PTRf, _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP,
                           temp_bmp, bmp_len);
    }
    sal_free(temp_bmp);

    /* now extract EGR_MPLS_PRI_MAPPING state */
    if (BCM_SUCCESS(rv)) {
        bmp_len = _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS;
        temp_bmp = sal_alloc(SHR_BITALLOCSIZE(bmp_len), "temp_bmp");
        sal_memset(temp_bmp, 0x0, SHR_BITALLOCSIZE(bmp_len));
        if (soc_mem_is_valid(unit, EGR_L3_NEXT_HOPm)){
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, EGR_L3_NEXT_HOPm, 
                           SD_TAG__SD_TAG_DOT1P_MAPPING_PTRf, 
                           _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS, temp_bmp, bmp_len);
        }
        if (BCM_SUCCESS(rv) && soc_mem_is_valid(unit, EGR_IP_TUNNELm)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, EGR_IP_TUNNELm, 
                           DOT1P_MAPPING_PTRf, _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS,
                           temp_bmp, bmp_len);
        }
        if (BCM_SUCCESS(rv) && 
            soc_mem_field_valid(unit, EGR_MPLS_VC_AND_SWAP_LABEL_TABLEm, 
                                SD_TAG_DOT1P_MAPPING_PTRf)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, 
                     EGR_MPLS_VC_AND_SWAP_LABEL_TABLEm, 
                     SD_TAG_DOT1P_MAPPING_PTRf, _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS,
                     temp_bmp, bmp_len);
        }
        if (BCM_SUCCESS(rv) && 
            soc_mem_field_valid(unit, EGR_VLANm, DOT1P_MAPPING_PTRf)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, 
                     EGR_VLANm, DOT1P_MAPPING_PTRf, 
                     _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS,
                     temp_bmp, bmp_len);
        }
        if (BCM_SUCCESS(rv) && 
            soc_mem_field_valid(unit, EGR_VLAN_XLATEm, 
                                MIM_ISID__DOT1P_MAPPING_PTRf)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, 
                     EGR_VLAN_XLATEm, MIM_ISID__DOT1P_MAPPING_PTRf, 
                     _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS,
                     temp_bmp, bmp_len);
        }
        sal_free(temp_bmp);
    }

    /* now extract DSCP_TABLE state */
    if (BCM_SUCCESS(rv)) {
        bmp_len = _BCM_QOS_MAP_LEN_DSCP_TABLE;
        temp_bmp = sal_alloc(SHR_BITALLOCSIZE(bmp_len), "temp_bmp");
        sal_memset(temp_bmp, 0x0, SHR_BITALLOCSIZE(bmp_len));
        if (soc_mem_field_valid(unit, L3_IIFm, TRUST_DSCP_PTRf)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, L3_IIFm, 
                                                   TRUST_DSCP_PTRf,
                              _BCM_QOS_MAP_TYPE_DSCP_TABLE, temp_bmp, bmp_len);
        }
        if (BCM_SUCCESS(rv) && 
            soc_mem_field_valid(unit, SOURCE_VPm, TRUST_DSCP_PTRf)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, SOURCE_VPm, 
                                                   TRUST_DSCP_PTRf,
                              _BCM_QOS_MAP_TYPE_DSCP_TABLE, temp_bmp, bmp_len);
        }
        sal_free(temp_bmp);
    }

    /* now extract EGR_DSCP_TABLE state */
    if (BCM_SUCCESS(rv)) {
        bmp_len = _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE;
        temp_bmp = sal_alloc(SHR_BITALLOCSIZE(bmp_len), "temp_bmp");
        sal_memset(temp_bmp, 0x0, SHR_BITALLOCSIZE(bmp_len));
        if (soc_mem_field_valid(unit, EGR_IP_TUNNELm, DSCP_MAPPING_PTRf)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, EGR_IP_TUNNELm, 
                            DSCP_MAPPING_PTRf,_BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE,
                            temp_bmp, bmp_len);
        }
        if (soc_mem_field_valid(unit, EGR_L3_INTFm, DSCP_MAPPING_PTRf)) {
            rv = _bcm_tr2_qos_reinit_from_hw_state(unit, EGR_L3_INTFm, 
                            DSCP_MAPPING_PTRf,_BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE,
                            temp_bmp, bmp_len);
        }
        sal_free(temp_bmp);
    }

    return rv;
}

/* 
 * Function:
 *      _bcm_tr2_qos_extended_reinit
 * Purpose:
 *      This routine handles Level 2 Warmboot init. 
 *      Cold-boot: scache location is allocated.
 *      Warm-boot: The data-structs are populated from recovered scache state
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_qos_extended_reinit(int unit)
{
    soc_scache_handle_t handle;
    uint8               *scache_ptr;
    uint32              hw_idx, scache_len;
    int                 idx;
    int                 rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(_bcm_tr2_qos_reinit_scache_len_get(unit, &scache_len));
    SOC_SCACHE_HANDLE_SET(handle, unit, BCM_MODULE_QOS, 0);

    if (SOC_WARM_BOOT(unit)) {
        /* during warm-boot recover state */
        BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, handle, FALSE,
                                            scache_len, &scache_ptr, 
                                            BCM_WB_DEFAULT_VERSION, NULL));

        /* recover from scache into book-keeping structs */
        for (idx = 0; idx < _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP; idx++) {
            hw_idx = (uint32) (*scache_ptr++);
            if (hw_idx != _BCM_TR2_REINIT_INVALID_HW_IDX) {
                _BCM_QOS_ING_PRI_CNG_USED_SET(unit, idx);
                QOS_INFO(unit)->pri_cng_hw_idx[idx] = hw_idx;
            }
        }
        for (idx = 0; idx < _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS; idx++) {
            hw_idx = (uint32) (*scache_ptr++);
            if (hw_idx != _BCM_TR2_REINIT_INVALID_HW_IDX) {
                _BCM_QOS_EGR_MPLS_USED_SET(unit, idx);
                QOS_INFO(unit)->egr_mpls_hw_idx[idx] = hw_idx;
            }
        }
        for (idx = 0; idx < _BCM_QOS_MAP_LEN_DSCP_TABLE; idx++) {
            hw_idx = (uint32) (*scache_ptr++);
            if (hw_idx != _BCM_TR2_REINIT_INVALID_HW_IDX) {
                _BCM_QOS_DSCP_TABLE_USED_SET(unit, idx);
                QOS_INFO(unit)->dscp_hw_idx[idx] = hw_idx;
            }
        }
        for (idx = 0; idx < _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE; idx++) {
            hw_idx = (uint32) (*scache_ptr++);
            if (hw_idx != _BCM_TR2_REINIT_INVALID_HW_IDX) {
                _BCM_QOS_EGR_DSCP_TABLE_USED_SET(unit, idx);
                QOS_INFO(unit)->egr_dscp_hw_idx[idx] = hw_idx;
            }
        }
        sal_memcpy(QOS_INFO(unit)->egr_mpls_bitmap_flags, scache_ptr, 
                   SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_EGR_MPLS_MAPS));
        scache_ptr += SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_EGR_MPLS_MAPS);
        sal_memcpy(QOS_INFO(unit)->ing_mpls_exp_bitmap, scache_ptr, 
                   SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP));
#ifdef BCM_KATANA_SUPPORT
        scache_ptr += SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP);
        for (idx = 0; idx < _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE; idx++) {
            hw_idx = (uint32) (*scache_ptr++);
            if (hw_idx != _BCM_TR2_REINIT_INVALID_HW_IDX) {
                _BCM_QOS_OFFSET_MAP_TABLE_USED_SET(unit, idx);
                QOS_INFO(unit)->dscp_hw_idx[idx] = hw_idx;
            }
        }
#endif
    } else {
        /* During cold-boot. Allocate a stable cache if not already done */
        rv = _bcm_esw_scache_ptr_get(unit, handle, TRUE, scache_len, 
                                     &scache_ptr, BCM_WB_DEFAULT_VERSION, NULL);
    }

    return rv;
}

/* 
 * Function:
 *      _bcm_tr2_qos_reinit_hw_profiles_update
 * Purpose:
 *      Updates the shared memory profile tables reference counts
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_qos_reinit_hw_profiles_update (int unit)
{
    int     i;

    for (i=0; i<_BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP; i++) {
        if (_BCM_QOS_ING_PRI_CNG_USED_GET(unit, i)) {
            BCM_IF_ERROR_RETURN(_bcm_ing_pri_cng_map_entry_reference(unit, 
                              ((QOS_INFO(unit)->pri_cng_hw_idx[i]) * 
                               _BCM_QOS_MAP_CHUNK_PRI_CNG), 
                               _BCM_QOS_MAP_CHUNK_PRI_CNG));
        }
    }
    for (i=0; i<_BCM_QOS_MAP_LEN_EGR_MPLS_MAPS; i++) {
        if (_BCM_QOS_EGR_MPLS_USED_GET(unit, i)) {
            BCM_IF_ERROR_RETURN(_bcm_egr_mpls_combo_map_entry_reference(unit, 
                             ((QOS_INFO(unit)->egr_mpls_hw_idx[i]) * 
                              _BCM_QOS_MAP_CHUNK_EGR_MPLS), 
                             _BCM_QOS_MAP_CHUNK_EGR_MPLS));
        }
    }
    for (i=0; i<_BCM_QOS_MAP_LEN_DSCP_TABLE; i++) {
        if (_BCM_QOS_DSCP_TABLE_USED_GET(unit, i)) {
            BCM_IF_ERROR_RETURN(_bcm_dscp_table_entry_reference(unit, 
                                 ((QOS_INFO(unit)->dscp_hw_idx[i]) * 
                                  _BCM_QOS_MAP_CHUNK_DSCP), 
                                 _BCM_QOS_MAP_CHUNK_DSCP));
        }
    }
    for (i=0; i<_BCM_QOS_MAP_LEN_EGR_DSCP_TABLE; i++) {
        if (_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, i)) {
            BCM_IF_ERROR_RETURN(_bcm_egr_dscp_table_entry_reference(unit, 
                             ((QOS_INFO(unit)->egr_dscp_hw_idx[i]) * 
                              _BCM_QOS_MAP_CHUNK_EGR_DSCP), 
                             _BCM_QOS_MAP_CHUNK_EGR_DSCP));
        }
    }
#ifdef BCM_KATANA_SUPPORT
    for (i=0; i< _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE; i++) {
        if (_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, i)) {
            BCM_IF_ERROR_RETURN(_bcm_offset_map_table_entry_reference(unit,
                                 ((QOS_INFO(unit)->offset_map_hw_idx[i]) *
                                  _BCM_QOS_MAP_CHUNK_OFFSET_MAP),
                                 _BCM_QOS_MAP_CHUNK_OFFSET_MAP));
        }
    }

#endif

    return BCM_E_NONE;
}

/* 
 * Function:
 *      _bcm_tr2_qos_reinit
 * Purpose:
 *      Top level QOS init routine for Warm-Boot
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_qos_reinit(int unit)
{
    int rv = BCM_E_NONE;
    int stable_size = 0;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    if (stable_size == 0) {
        if (!SOC_WARM_BOOT(unit)) {
            return rv; /* do nothing in cold-boot */
        }
        rv = _bcm_tr2_qos_unsynchronized_reinit(unit);
    } else {
        /* Limited is same as extended */
        rv = _bcm_tr2_qos_extended_reinit(unit);
    }

    if (BCM_SUCCESS(rv)) {
        rv = _bcm_tr2_qos_reinit_hw_profiles_update(unit);
    }

    return rv;
}

/* 
 * Function:
 *      bcm_tr2_qos_sync
 * Purpose:
 *      This routine extracts the state that needs to be stored from the 
 *      book-keeping structs and stores it in the allocated scache location 
 * Compression format:  0   - A     pri_cng_hw_idx table
 *                      A+1 - B     egr_mpls_hw_idx table
 *                      B+1 - C     dscp_hw_idx table
 *                      C+1 - D     egr_dscp_hw_idx table
 *                      D+1 - E     egr_mpls flags
 *                      E+1 - F     ing_mpls_exp table usage by QOS module
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_qos_sync(int unit)
{
    soc_scache_handle_t handle;
    uint8               *scache_ptr;
    uint32              hw_idx, scache_len;
    int                 idx;
    
    BCM_IF_ERROR_RETURN(_bcm_tr2_qos_reinit_scache_len_get(unit, &scache_len));
    SOC_SCACHE_HANDLE_SET(handle, unit, BCM_MODULE_QOS, 0);
    BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, handle, FALSE,
                                                scache_len, &scache_ptr, 
                                                BCM_WB_DEFAULT_VERSION, NULL));

    /* now store the state into the compressed format */
    for (idx = 0; idx < _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP; idx++) {
        if (_BCM_QOS_ING_PRI_CNG_USED_GET(unit, idx)) {
            hw_idx = QOS_INFO(unit)->pri_cng_hw_idx[idx];
        } else {
            hw_idx = _BCM_TR2_REINIT_INVALID_HW_IDX;
        }
        *scache_ptr++ = hw_idx;
    }
    for (idx = 0; idx < _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS; idx++) {
        if (_BCM_QOS_EGR_MPLS_USED_GET(unit, idx)) {
            hw_idx = QOS_INFO(unit)->egr_mpls_hw_idx[idx];
        } else {
            hw_idx = _BCM_TR2_REINIT_INVALID_HW_IDX;
        }
        *scache_ptr++ = hw_idx;
    }
    for (idx = 0; idx < _BCM_QOS_MAP_LEN_DSCP_TABLE; idx++) {
        if (_BCM_QOS_DSCP_TABLE_USED_GET(unit, idx)) {
            hw_idx = QOS_INFO(unit)->dscp_hw_idx[idx];
        } else {
            hw_idx = _BCM_TR2_REINIT_INVALID_HW_IDX;
        }
        *scache_ptr++ = hw_idx;
    }
    for (idx = 0; idx < _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE; idx++) {
        if (_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, idx)) {
            hw_idx = QOS_INFO(unit)->egr_dscp_hw_idx[idx];
        } else {
            hw_idx = _BCM_TR2_REINIT_INVALID_HW_IDX;
        }
        *scache_ptr++ = hw_idx;
    }
    sal_memcpy(scache_ptr, QOS_INFO(unit)->egr_mpls_bitmap_flags, 
               SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_EGR_MPLS_MAPS));
    scache_ptr += SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_EGR_MPLS_MAPS);
    sal_memcpy(scache_ptr, QOS_INFO(unit)->ing_mpls_exp_bitmap, 
               SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP));
#ifdef BCM_KATANA_SUPPORT
    scache_ptr += SHR_BITALLOCSIZE(_BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP);
    for (idx = 0; idx < _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE; idx++) {
        if (_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, idx)) {
            hw_idx = QOS_INFO(unit)->offset_map_hw_idx[idx];
        } else {
            hw_idx = _BCM_TR2_REINIT_INVALID_HW_IDX;
        }
        *scache_ptr++ = hw_idx;
    }
#endif

    return BCM_E_NONE;
}
#endif

STATIC void
_bcm_tr2_qos_free_resources(int unit)
{
    _bcm_tr2_qos_bookkeeping_t *qos_info = QOS_INFO(unit);

    if (!qos_info) {
        return;
    }

    /* Destroy ingress profile bitmap */
    if (qos_info->ing_pri_cng_bitmap) {
        sal_free(qos_info->ing_pri_cng_bitmap);
        qos_info->ing_pri_cng_bitmap = NULL;
    }
    if (qos_info->pri_cng_hw_idx) {
        sal_free(qos_info->pri_cng_hw_idx);
        qos_info->pri_cng_hw_idx = NULL;
    }

    if (qos_info->egr_mpls_hw_idx) {
        int id;
        /* Delete all combo mpls hw index profiles */
        for (id = 0; id < _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS; id++)
        {
            if (qos_info->egr_mpls_hw_idx[id] != 0) {
                _bcm_egr_mpls_combo_map_entry_delete (unit, 
                                           (qos_info->egr_mpls_hw_idx[id] *
                                            _BCM_QOS_MAP_CHUNK_EGR_MPLS));
                qos_info->egr_mpls_hw_idx[id] = 0;
                _BCM_QOS_EGR_MPLS_USED_CLR(unit, id);
                _BCM_QOS_EGR_MPLS_FLAGS_USED_CLR(unit, id);
            }
        }
        sal_free(qos_info->egr_mpls_hw_idx);
        qos_info->egr_mpls_hw_idx = NULL;
    }

    /* Destroy egress profile usage bitmap */
    if (qos_info->egr_mpls_bitmap) {
        sal_free(qos_info->egr_mpls_bitmap);
        qos_info->egr_mpls_bitmap = NULL;
    }

    /* Destroy DSCP table profile usage bitmap */
    if (qos_info->dscp_table_bitmap) {
        sal_free(qos_info->dscp_table_bitmap);
        qos_info->dscp_table_bitmap = NULL;
    }
    if (qos_info->dscp_hw_idx) {
        sal_free(qos_info->dscp_hw_idx);
        qos_info->dscp_hw_idx = NULL;
    }

    /* Destroy egress DSCP table profile usage bitmap */
    if (qos_info->egr_dscp_table_bitmap) {
        sal_free(qos_info->egr_dscp_table_bitmap);
        qos_info->egr_dscp_table_bitmap = NULL;
    }
    if (qos_info->egr_dscp_hw_idx) {
        sal_free(qos_info->egr_dscp_hw_idx);
        qos_info->egr_dscp_hw_idx = NULL;
    }

    /* Destroy egress profile flags bitmap */
    if (qos_info->egr_mpls_bitmap_flags) {
        sal_free(qos_info->egr_mpls_bitmap_flags);
        qos_info->egr_mpls_bitmap_flags = NULL;
    }

    /* Destroy ing_mpls_exp bitmap */
    if (qos_info->ing_mpls_exp_bitmap) {
        sal_free(qos_info->ing_mpls_exp_bitmap);
        qos_info->ing_mpls_exp_bitmap = NULL;
    }
#ifdef BCM_KATANA_SUPPORT
/* Destroy queue map table profile usage bitmap */
    if (qos_info->offset_map_table_bitmap) {
        sal_free(qos_info->offset_map_table_bitmap);
        qos_info->offset_map_table_bitmap = NULL;
    }
    if (qos_info->offset_map_hw_idx) {
        sal_free(qos_info->offset_map_hw_idx);
        qos_info->offset_map_hw_idx = NULL;
    }
#endif

    /* Destroy the mutex */
    if (_tr2_qos_mutex[unit]) {
        sal_mutex_destroy(_tr2_qos_mutex[unit]);
        _tr2_qos_mutex[unit] = NULL;
    }
}

/* Initialize the QoS module. */
int 
bcm_tr2_qos_init(int unit)
{
    _bcm_tr2_qos_bookkeeping_t *qos_info = QOS_INFO(unit);
    int ing_profiles, egr_profiles, rv = BCM_E_NONE;
    int dscp_profiles, egr_dscp_profiles, ing_mpls_profiles;
#ifdef BCM_KATANA_SUPPORT
    int offset_map_profiles = 0;
#endif
    ing_profiles = _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP;
    egr_profiles = _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS;
    dscp_profiles = _BCM_QOS_MAP_LEN_DSCP_TABLE;
    egr_dscp_profiles = _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE;
    ing_mpls_profiles = _BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP;
#ifdef BCM_KATANA_SUPPORT
    offset_map_profiles = _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE;
#endif

    if (tr2_qos_initialized[unit]) {
        BCM_IF_ERROR_RETURN(bcm_tr2_qos_detach(unit));
    }

    /* Create mutex */
    if (NULL == _tr2_qos_mutex[unit]) {
        _tr2_qos_mutex[unit] = sal_mutex_create("qos mutex");
        if (_tr2_qos_mutex[unit] == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }

    /* Allocate ingress profile usage bitmap */
    if (NULL == qos_info->ing_pri_cng_bitmap) {
        qos_info->ing_pri_cng_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(ing_profiles), "ing_pri_cng_bitmap");
        if (qos_info->ing_pri_cng_bitmap == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->ing_pri_cng_bitmap, 0, SHR_BITALLOCSIZE(ing_profiles));
    if (NULL == qos_info->pri_cng_hw_idx) {
        qos_info->pri_cng_hw_idx = 
            sal_alloc(sizeof(uint32) * ing_profiles, "pri_cng_hw_idx");
        if (qos_info->pri_cng_hw_idx == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }    
    }
    sal_memset(qos_info->pri_cng_hw_idx, 0, (sizeof(uint32) * ing_profiles));

    /* Allocate egress profile usage bitmap */
    if (NULL == qos_info->egr_mpls_bitmap) {
        qos_info->egr_mpls_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(egr_profiles), "egr_mpls_bitmap");
        if (qos_info->egr_mpls_bitmap == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->egr_mpls_bitmap, 0, SHR_BITALLOCSIZE(egr_profiles));
    if (NULL == qos_info->egr_mpls_hw_idx) {
        qos_info->egr_mpls_hw_idx = 
            sal_alloc(sizeof(uint32) * egr_profiles, "egr_mpls_hw_idx");
        if (qos_info->egr_mpls_hw_idx == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }    
    }
    sal_memset(qos_info->egr_mpls_hw_idx, 0, (sizeof(uint32) * egr_profiles));

    /* Allocate DSCP_TABLE profile usage bitmap */
    if (NULL == qos_info->dscp_table_bitmap) {
        qos_info->dscp_table_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(dscp_profiles), "dscp_table_bitmap");
        if (qos_info->dscp_table_bitmap == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->dscp_table_bitmap, 0, SHR_BITALLOCSIZE(dscp_profiles));
    if (NULL == qos_info->dscp_hw_idx) {
        qos_info->dscp_hw_idx = 
            sal_alloc(sizeof(uint32) * dscp_profiles, "dscp_hw_idx");
        if (qos_info->dscp_hw_idx == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }    
    }
    sal_memset(qos_info->dscp_hw_idx, 0, (sizeof(uint32) * dscp_profiles));

    /* Allocate EGR_DSCP_TABLE profile usage bitmap */
    if (NULL == qos_info->egr_dscp_table_bitmap) {
        qos_info->egr_dscp_table_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(egr_dscp_profiles), "egr_dscp_table_bitmap");
        if (qos_info->egr_dscp_table_bitmap == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->egr_dscp_table_bitmap, 0, 
               SHR_BITALLOCSIZE(egr_dscp_profiles));
    if (NULL == qos_info->egr_dscp_hw_idx) {
        qos_info->egr_dscp_hw_idx = 
            sal_alloc(sizeof(uint32) * egr_dscp_profiles, "egr_dscp_hw_idx");
        if (qos_info->egr_dscp_hw_idx == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }    
    }
    sal_memset(qos_info->egr_dscp_hw_idx, 0, (sizeof(uint32) * egr_dscp_profiles));

    /* Allocate egress profile usage bitmap */
    if (NULL == qos_info->egr_mpls_bitmap_flags) {
        qos_info->egr_mpls_bitmap_flags =
            sal_alloc(SHR_BITALLOCSIZE(egr_profiles), "egr_mpls_bitmap_flags");
        if (qos_info->egr_mpls_bitmap_flags == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->egr_mpls_bitmap_flags, 0, SHR_BITALLOCSIZE(egr_profiles));

    /* Allocate ingress mpls profile usage bitmap */
    if (NULL == qos_info->ing_mpls_exp_bitmap) {
        qos_info->ing_mpls_exp_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(ing_mpls_profiles), "ing_mpls_exp_bitmap");
        if (qos_info->ing_mpls_exp_bitmap == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->ing_mpls_exp_bitmap, 0, SHR_BITALLOCSIZE(ing_mpls_profiles));
#ifdef BCM_KATANA_SUPPORT
     /* Allocate queue offset map usage bitmap */
    if (NULL == qos_info->offset_map_table_bitmap) {
        qos_info->offset_map_table_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(offset_map_profiles),
                                          "offset_map_table_bitmap");
        if (qos_info->offset_map_table_bitmap == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->offset_map_table_bitmap, 0, 
                                    SHR_BITALLOCSIZE(offset_map_profiles));
    if (NULL == qos_info->offset_map_hw_idx) {
        qos_info->offset_map_hw_idx =
           sal_alloc(sizeof(uint32) * offset_map_profiles, "offset_map_hw_idx");
        if (qos_info->offset_map_hw_idx == NULL) {
            _bcm_tr2_qos_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(qos_info->offset_map_hw_idx, 0, 
                              (sizeof(uint32) * offset_map_profiles));
#endif /* BCM_KATANA_SUPPORT */


#ifdef BCM_WARM_BOOT_SUPPORT
    rv = _bcm_tr2_qos_reinit(unit); 
    if (SOC_FAILURE(rv)) {
        _bcm_tr2_qos_free_resources(unit);
        return rv;
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    tr2_qos_initialized[unit] = TRUE;
    return rv;
}

/* Detach the QoS module. */
int 
bcm_tr2_qos_detach(int unit)
{
    int rv = BCM_E_NONE;
    _bcm_tr2_qos_free_resources(unit);
    tr2_qos_initialized[unit] = FALSE;
    return rv;
}

STATIC int
_bcm_tr2_qos_l2_map_create(int unit, uint32 flags, int *map_id) 
{
    ing_pri_cng_map_entry_t ing_pri_map[_BCM_QOS_MAP_CHUNK_PRI_CNG];
    ing_untagged_phb_entry_t ing_untagged_phb;
    egr_mpls_pri_mapping_entry_t egr_mpls_pri_map[_BCM_QOS_MAP_CHUNK_EGR_MPLS];
    egr_mpls_exp_mapping_1_entry_t egr_mpls_exp_map[_BCM_QOS_MAP_CHUNK_EGR_MPLS];
    egr_mpls_exp_mapping_2_entry_t egr_mpls_exp_map2[_BCM_QOS_MAP_CHUNK_EGR_MPLS];
    int id, index = -1, rv = BCM_E_NONE;
    void *entries[3];
    if (flags & BCM_QOS_MAP_INGRESS) {
        /* Check for pre-specified ID */
        if (flags & BCM_QOS_MAP_WITH_ID) {
            id = *map_id & _BCM_QOS_MAP_TYPE_MASK;
            if ((*map_id >> _BCM_QOS_MAP_SHIFT) != 
                _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP) {
                return BCM_E_BADID;
            }
            if (_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
                if (!(flags & BCM_QOS_MAP_REPLACE)) {
                    return BCM_E_EXISTS;
                }
            } else {
                _BCM_QOS_ING_PRI_CNG_USED_SET(unit, id);
            }
        } else {
            id = _bcm_tr2_qos_id_alloc(unit, QOS_INFO(unit)->ing_pri_cng_bitmap, 
                                   _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP);
            if (id == -1) {
                return BCM_E_RESOURCE;
            }
            _BCM_QOS_ING_PRI_CNG_USED_SET(unit, id);
            *map_id = id | (_BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP 
                      << _BCM_QOS_MAP_SHIFT);
        }
        /* Reserve a chunk in the ING_PRI_CNG_MAP table */
        sal_memset(ing_pri_map, 0, sizeof(ing_pri_map));
        entries[0] = &ing_pri_map;
        entries[1] = &ing_untagged_phb;
        BCM_IF_ERROR_RETURN(_bcm_ing_pri_cng_map_entry_add(unit, entries, 
                                            _BCM_QOS_MAP_CHUNK_PRI_CNG,
                                                           (uint32 *)&index));
        QOS_INFO(unit)->pri_cng_hw_idx[id] = (index / 
                                              _BCM_QOS_MAP_CHUNK_PRI_CNG);
    } else if (flags & BCM_QOS_MAP_EGRESS) {
        /* Check for pre-specified ID */
        if (flags & BCM_QOS_MAP_WITH_ID) {
            id = *map_id & _BCM_QOS_MAP_TYPE_MASK;
            if ((*map_id >> _BCM_QOS_MAP_SHIFT) != 
                _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS) {
                return BCM_E_BADID;
            }
            if (_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
                if (!(flags & BCM_QOS_MAP_REPLACE)) {
                    return BCM_E_EXISTS;
                }
            } else {
                _BCM_QOS_EGR_MPLS_USED_SET(unit, id);
            }
        } else {
            id = _bcm_tr2_qos_id_alloc(unit, QOS_INFO(unit)->egr_mpls_bitmap, 
                                   _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS);
            if (id == -1) {
                return BCM_E_RESOURCE;
            }
            _BCM_QOS_EGR_MPLS_USED_SET(unit, id);
            *map_id = id | (_BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS 
                      << _BCM_QOS_MAP_SHIFT);
        }
        /* Reserve a chunk in the EGR_MPLS_EXP_MAPPING_1/EGR_MPLS_PRI_MAPPING 
           tables - these two tables always done together */
        sal_memset(egr_mpls_pri_map, 0, sizeof(egr_mpls_pri_map));
        entries[0] = egr_mpls_pri_map;
        sal_memset(egr_mpls_exp_map, 0, sizeof(egr_mpls_exp_map));
        entries[1] = egr_mpls_exp_map;
        sal_memset(egr_mpls_exp_map2, 0, sizeof(egr_mpls_exp_map2));
        entries[2] = egr_mpls_exp_map2;
        BCM_IF_ERROR_RETURN(_bcm_egr_mpls_combo_map_entry_add(unit, entries, 
                                                   _BCM_QOS_MAP_CHUNK_EGR_MPLS,
                                                              (uint32 *)&index));
        QOS_INFO(unit)->egr_mpls_hw_idx[id] = (index / 
                                               _BCM_QOS_MAP_CHUNK_EGR_MPLS);
    }
    return rv;
} 

STATIC int
_bcm_tr2_qos_l3_map_create(int unit, uint32 flags, int *map_id) 
{
    dscp_table_entry_t dscp_table[_BCM_QOS_MAP_CHUNK_DSCP];
    egr_dscp_table_entry_t egr_dscp_table[_BCM_QOS_MAP_CHUNK_EGR_DSCP];
    int id, index = -1, rv = BCM_E_NONE;
    void *entries[1];
    if (flags & BCM_QOS_MAP_INGRESS) {
        /* Check for pre-specified ID */
        if (flags & BCM_QOS_MAP_WITH_ID) {
            id = *map_id & _BCM_QOS_MAP_TYPE_MASK;
            if ((*map_id >> _BCM_QOS_MAP_SHIFT) != 
                _BCM_QOS_MAP_TYPE_DSCP_TABLE) {
                return BCM_E_BADID;
            }
            if (_BCM_QOS_DSCP_TABLE_USED_GET(unit, id)) {
                if (!(flags & BCM_QOS_MAP_REPLACE)) {
                    return BCM_E_EXISTS;
                }
            } else {
                _BCM_QOS_DSCP_TABLE_USED_SET(unit, id);
            }
        } else {
            id = _bcm_tr2_qos_id_alloc(unit, QOS_INFO(unit)->dscp_table_bitmap, 
                                   _BCM_QOS_MAP_TYPE_DSCP_TABLE);
            if (id == -1) {
                return BCM_E_RESOURCE;
            }
            _BCM_QOS_DSCP_TABLE_USED_SET(unit, id);
            *map_id = id | (_BCM_QOS_MAP_TYPE_DSCP_TABLE 
                      << _BCM_QOS_MAP_SHIFT);
        }
        /* Reserve a chunk in the DSCP_TABLE */
        sal_memset(dscp_table, 0, sizeof(dscp_table));
        entries[0] = &dscp_table;
        BCM_IF_ERROR_RETURN(_bcm_dscp_table_entry_add(unit, entries, 
                                                      _BCM_QOS_MAP_CHUNK_DSCP,
                                                      (uint32 *)&index));
        QOS_INFO(unit)->dscp_hw_idx[id] = index / _BCM_QOS_MAP_CHUNK_DSCP;
    } else if (flags & BCM_QOS_MAP_EGRESS) {
        /* Check for pre-specified ID */
        if (flags & BCM_QOS_MAP_WITH_ID) {
            id = *map_id & _BCM_QOS_MAP_TYPE_MASK;
            if ((*map_id >> _BCM_QOS_MAP_SHIFT) != 
                _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE) {
                return BCM_E_BADID;
            }
            if (_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, id)) {
                if (!(flags & BCM_QOS_MAP_REPLACE)) {
                    return BCM_E_EXISTS;
                }
            } else {
                _BCM_QOS_EGR_DSCP_TABLE_USED_SET(unit, id);
            }
        } else {
            id = _bcm_tr2_qos_id_alloc(unit, QOS_INFO(unit)->egr_dscp_table_bitmap, 
                                   _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE);
            if (id == -1) {
                return BCM_E_RESOURCE;
            }
            _BCM_QOS_EGR_DSCP_TABLE_USED_SET(unit, id);
            *map_id = id | (_BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE 
                      << _BCM_QOS_MAP_SHIFT);
        }
        /* Reserve a chunk in the EGR_DSCP_TABLE */
        sal_memset(egr_dscp_table, 0, sizeof(egr_dscp_table));
        entries[0] = &egr_dscp_table;
        BCM_IF_ERROR_RETURN(_bcm_egr_dscp_table_entry_add(unit, entries, 
                                                   _BCM_QOS_MAP_CHUNK_EGR_DSCP,
                                                          (uint32 *)&index));
        QOS_INFO(unit)->egr_dscp_hw_idx[id] = (index / 
                                               _BCM_QOS_MAP_CHUNK_EGR_DSCP);
    }
    return rv;
}

STATIC int
_bcm_tr2_qos_mpls_map_create(int unit, uint32 flags, int *map_id) 
{
    if (flags & BCM_QOS_MAP_INGRESS) {
        /* The ING_MPLS_EXP_MAPPING table is used */
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
        uint32 mpls_exp_map_flags = BCM_MPLS_EXP_MAP_INGRESS;

        if (flags & BCM_QOS_MAP_WITH_ID) {
            mpls_exp_map_flags |= BCM_MPLS_EXP_MAP_WITH_ID;
        }

        BCM_IF_ERROR_RETURN(bcm_tr_mpls_exp_map_create(unit,
                                            mpls_exp_map_flags, map_id));
        _BCM_QOS_ING_MPLS_EXP_USED_SET(unit, 
                        ((*map_id) & _BCM_TR_MPLS_EXP_MAP_TABLE_NUM_MASK));
#endif
    } else if (flags & BCM_QOS_MAP_EGRESS) {
        /* The EGR_MPLS_EXP_MAPPING_1 table is used */
        /* Shared with L2 egress map */
        BCM_IF_ERROR_RETURN(_bcm_tr2_qos_l2_map_create(unit, 
                                                  BCM_QOS_MAP_EGRESS, map_id));
        _BCM_QOS_EGR_MPLS_FLAGS_USED_SET(unit, 
                                         ((*map_id) & _BCM_QOS_MAP_TYPE_MASK));
    } else {
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}
#ifdef BCM_KATANA_SUPPORT
/* Create a ING_QUEUE_OFFSET_MAP profile */
STATIC int 
_bcm_kt_ing_queue_offset_map_create(int unit, uint32 flags, int *map_id)
{
    ing_queue_offset_mapping_table_entry_t 
                             offset_map_table[_BCM_QOS_MAP_CHUNK_OFFSET_MAP];
    void *entries[1];
    int id = 0, index = -1;
    if (flags & BCM_QOS_MAP_WITH_ID) {
      /* Check for pre-specified ID */
        id = *map_id & _BCM_QOS_MAP_TYPE_MASK;
        if ((*map_id >> _BCM_QOS_MAP_SHIFT) !=
            _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE) {
            return BCM_E_BADID;
        }
        if (_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, id)) {
            if (!(flags & BCM_QOS_MAP_REPLACE)) {
                return BCM_E_EXISTS;
            }
        } else {
                _BCM_QOS_OFFSET_MAP_TABLE_USED_SET(unit, id);
        }

    } else {
        id = _bcm_tr2_qos_id_alloc(unit, QOS_INFO(unit)->offset_map_table_bitmap,
                                  _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE);
            if (id == -1) {
                return BCM_E_RESOURCE;
            }
            _BCM_QOS_OFFSET_MAP_TABLE_USED_SET(unit, id);
            *map_id = id | (_BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE
                                                     << _BCM_QOS_MAP_SHIFT);
    }
    /* Reserve a chunk in the ING_QUEUE_OFFSET_MAPPING_TABLE */
    sal_memset(offset_map_table, 0, sizeof(offset_map_table));
    entries[0] = &offset_map_table;
    BCM_IF_ERROR_RETURN(_bcm_offset_map_table_entry_add(unit, entries,
                     _BCM_QOS_MAP_CHUNK_OFFSET_MAP, (uint32 *)&index));
    QOS_INFO(unit)->offset_map_hw_idx[id] = 
                                   (index / _BCM_QOS_MAP_CHUNK_OFFSET_MAP);
    return BCM_E_NONE;
}
#endif /* BCM_KATANA_SUPPORT */

/* Create a QoS map profile */
/* This will allocate an ID and a profile index with all-zero mapping */
int 
bcm_tr2_qos_map_create(int unit, uint32 flags, int *map_id)
{
    int rv = BCM_E_UNAVAIL;
    QOS_INIT(unit);
    if (flags == 0) {
        return BCM_E_PARAM;
    }
    QOS_LOCK(unit);
    if (flags & BCM_QOS_MAP_L2) {
        rv = _bcm_tr2_qos_l2_map_create(unit, flags, map_id);
    } else if (flags & BCM_QOS_MAP_L3) {
        rv = _bcm_tr2_qos_l3_map_create(unit, flags, map_id);
    } else if (flags & BCM_QOS_MAP_MPLS) {
        if (!soc_feature(unit, soc_feature_mpls)) {
            QOS_UNLOCK(unit);
            return rv;
        }
        rv = _bcm_tr2_qos_mpls_map_create(unit, flags, map_id);
    }
#ifdef BCM_KATANA_SUPPORT
    else if (flags & BCM_QOS_MAP_QUEUE) {
        rv = _bcm_kt_ing_queue_offset_map_create(unit, flags, map_id);
    }
#endif
#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    QOS_UNLOCK(unit);
    return rv;
}

/* Destroy a QoS map profile */
/* This will free the profile index and de-allocate the ID */
int 
bcm_tr2_qos_map_destroy(int unit, int map_id)
{
    int id, rv = BCM_E_UNAVAIL;
    QOS_INIT(unit);
    id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    QOS_LOCK(unit);
    switch (map_id >> _BCM_QOS_MAP_SHIFT) {
    case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
        if (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        } else {
            rv = _bcm_ing_pri_cng_map_entry_delete
                     (unit, QOS_INFO(unit)->pri_cng_hw_idx[id] * 
                      _BCM_QOS_MAP_CHUNK_PRI_CNG);
            QOS_INFO(unit)->pri_cng_hw_idx[id] = 0;
            _BCM_QOS_ING_PRI_CNG_USED_CLR(unit, id);
        }
        break;
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
        if (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        } else {
            rv = _bcm_egr_mpls_combo_map_entry_delete (unit, 
                                       (QOS_INFO(unit)->egr_mpls_hw_idx[id] *
                                        _BCM_QOS_MAP_CHUNK_EGR_MPLS));
            QOS_INFO(unit)->egr_mpls_hw_idx[id] = 0;
            _BCM_QOS_EGR_MPLS_USED_CLR(unit, id);
            _BCM_QOS_EGR_MPLS_FLAGS_USED_CLR(unit, id);
        }
        break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
        if (!_BCM_QOS_DSCP_TABLE_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        } else {
            rv = _bcm_dscp_table_entry_delete
                     (unit, (QOS_INFO(unit)->dscp_hw_idx[id] * 
                      _BCM_QOS_MAP_CHUNK_DSCP));
            QOS_INFO(unit)->dscp_hw_idx[id] = 0;
            _BCM_QOS_DSCP_TABLE_USED_CLR(unit, id);
        }
        break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
        if (!_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        } else {
            rv = _bcm_egr_dscp_table_entry_delete
                    (unit, (QOS_INFO(unit)->egr_dscp_hw_idx[id] * 
                     _BCM_QOS_MAP_CHUNK_EGR_DSCP));
            QOS_INFO(unit)->egr_dscp_hw_idx[id] = 0;
            _BCM_QOS_EGR_DSCP_TABLE_USED_CLR(unit, id);
        }
        break;
#ifdef BCM_KATANA_SUPPORT
    case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
        if (!_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        } else {
            rv = _bcm_offset_map_table_entry_delete
                     (unit, (QOS_INFO(unit)->offset_map_hw_idx[id] *
                      _BCM_QOS_MAP_CHUNK_OFFSET_MAP));
            QOS_INFO(unit)->offset_map_hw_idx[id] = 0;
            _BCM_QOS_OFFSET_MAP_TABLE_USED_CLR(unit, id);
        }
        break;
#endif

    default:
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
        if (map_id & _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_INGRESS) {
            if (!soc_feature(unit, soc_feature_mpls)) {
                QOS_UNLOCK(unit);
                return rv;
            }
            rv = bcm_tr_mpls_exp_map_destroy(unit, map_id);
        } else
#endif /* INCLUDE_L3 && BCM_MPLS_SUPPORT */
        {
            rv = BCM_E_PARAM;
        }
        break;
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    QOS_UNLOCK(unit);
    return rv;
}

/* Add an entry to a QoS map */
/* Read the existing profile chunk, modify what's needed and add the 
 * new profile. This can result in the HW profile index changing for a 
 * given ID */
int 
bcm_tr2_qos_map_add(int unit, uint32 flags, bcm_qos_map_t *map, int map_id)
{
    int alloc_size, offset, cng, id, index, rv = BCM_E_NONE;
    ing_pri_cng_map_entry_t ing_pri_map[_BCM_QOS_MAP_CHUNK_PRI_CNG];
    ing_untagged_phb_entry_t ing_untagged_phb;
    egr_mpls_pri_mapping_entry_t *egr_mpls_pri_map;
    egr_mpls_exp_mapping_1_entry_t *egr_mpls_exp_map;
    egr_mpls_exp_mapping_2_entry_t *egr_mpls_exp_map2;
    dscp_table_entry_t *dscp_table;
    egr_dscp_table_entry_t *egr_dscp_table;
    void *entries[3], *entry;
#ifdef BCM_KATANA_SUPPORT
    ing_queue_offset_mapping_table_entry_t *offset_map_table;
#endif

    QOS_INIT(unit);

    id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    QOS_LOCK(unit);
    switch (map_id >> _BCM_QOS_MAP_SHIFT) {
    case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
        if (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_PARAM;
        } else {
            if (!(flags & BCM_QOS_MAP_L2) || !(flags & BCM_QOS_MAP_INGRESS)) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            if ((map->int_pri > 15) || (map->int_pri < 0) || 
                (map->pkt_pri > 7) || (map->pkt_cfi > 1) || 
                ((map->color != bcmColorGreen) && 
                (map->color != bcmColorYellow) && 
                (map->color != bcmColorRed))) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }

            entries[0] = ing_pri_map;
            entries[1] = &ing_untagged_phb;

            /* Base index of table in hardware */
            index = QOS_INFO(unit)->pri_cng_hw_idx[id] * 
                    _BCM_QOS_MAP_CHUNK_PRI_CNG;

            rv = _bcm_ing_pri_cng_map_entry_get(unit, index, 
                                                _BCM_QOS_MAP_CHUNK_PRI_CNG, 
                                                entries);
            if (BCM_FAILURE(rv)) {
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Offset within table */
            offset = (map->pkt_pri << 1) | map->pkt_cfi;

            /* Modify what's needed */
            entry = &ing_pri_map[offset];
            soc_mem_field32_set(unit, ING_PRI_CNG_MAPm, entry,
                                PRIf, map->int_pri);
            soc_mem_field32_set(unit, ING_PRI_CNG_MAPm, entry,
                                 CNGf, _BCM_COLOR_ENCODING(unit, map->color));

            /* Delete the old profile chunk */
            rv = _bcm_ing_pri_cng_map_entry_delete(unit, index);
            if (BCM_FAILURE(rv)) {
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Add new chunk and store new HW index */
            rv = _bcm_ing_pri_cng_map_entry_add(unit, entries,
                                                _BCM_QOS_MAP_CHUNK_PRI_CNG, 
                                                (uint32 *)&index);
            QOS_INFO(unit)->pri_cng_hw_idx[id] = (index / 
                                                 _BCM_QOS_MAP_CHUNK_PRI_CNG);
        }
        break;
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
        if (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_PARAM;
        } else {
            if (!((flags & BCM_QOS_MAP_L2) || (flags & BCM_QOS_MAP_MPLS))
                 || !(flags & BCM_QOS_MAP_EGRESS)) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            if ((map->int_pri > 15) || (map->int_pri < 0) || 
                (map->pkt_pri > 7) || (map->pkt_cfi > 1) || 
                ((map->color != bcmColorGreen) && 
                (map->color != bcmColorYellow) && 
                (map->color != bcmColorRed))) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            if (flags & BCM_QOS_MAP_MPLS) {
                if ((map->exp > 7) || (map->exp < 0)) {
                    QOS_UNLOCK(unit);
                    return BCM_E_PARAM;
                }
            }

            /* Allocate memory for DMA */
            alloc_size = (_BCM_QOS_MAP_CHUNK_EGR_MPLS * 
                         sizeof(egr_mpls_pri_mapping_entry_t));
            egr_mpls_pri_map = soc_cm_salloc(unit, alloc_size,
                                             "TR2 egr mpls pri map");
            if (NULL == egr_mpls_pri_map) {
                QOS_UNLOCK(unit);
                return (BCM_E_MEMORY);
            }
            sal_memset(egr_mpls_pri_map, 0, alloc_size);

            alloc_size = (_BCM_QOS_MAP_CHUNK_EGR_MPLS * 
                          sizeof(egr_mpls_exp_mapping_1_entry_t));
            egr_mpls_exp_map = soc_cm_salloc(unit, alloc_size,
                                             "TR2 egr mpls exp map");
            if (NULL == egr_mpls_exp_map) {
                sal_free(egr_mpls_pri_map);
                QOS_UNLOCK(unit);
                return (BCM_E_MEMORY);
            }
            sal_memset(egr_mpls_exp_map, 0, alloc_size);

            alloc_size = (_BCM_QOS_MAP_CHUNK_EGR_MPLS * 
                          sizeof(egr_mpls_exp_mapping_2_entry_t));
            egr_mpls_exp_map2= soc_cm_salloc(unit, alloc_size,
                                             "TR2 egr mpls exp map2");
            if (NULL == egr_mpls_exp_map2) {
                sal_free(egr_mpls_pri_map);
                sal_free(egr_mpls_exp_map);
                QOS_UNLOCK(unit);
                return (BCM_E_MEMORY);
            }
            sal_memset(egr_mpls_exp_map2, 0, alloc_size);

            /* Base index of table in hardware */
            index = (QOS_INFO(unit)->egr_mpls_hw_idx[id] * 
                     _BCM_QOS_MAP_CHUNK_EGR_MPLS);

            /* Offset within table */
            cng = _BCM_COLOR_ENCODING(unit, map->color);
            offset = (map->int_pri << 2) | cng;

            /* Read the old profile chunk */
            rv = soc_mem_read_range(unit, EGR_MPLS_PRI_MAPPINGm, MEM_BLOCK_ANY, 
                                    index, 
                                    (index + (_BCM_QOS_MAP_CHUNK_EGR_MPLS -1)),
                                    egr_mpls_pri_map);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, egr_mpls_pri_map);
                soc_cm_sfree(unit, egr_mpls_exp_map);
                QOS_UNLOCK(unit);
                return (rv);
            }
            if (SOC_MEM_IS_VALID(unit, EGR_MPLS_EXP_MAPPING_1m)) {
                rv = soc_mem_read_range(unit, EGR_MPLS_EXP_MAPPING_1m, MEM_BLOCK_ANY, 
                                        index, 
                                        (index + (_BCM_QOS_MAP_CHUNK_EGR_MPLS -1)),
                                        egr_mpls_exp_map);
                if (BCM_FAILURE(rv)) {
                    soc_cm_sfree(unit, egr_mpls_pri_map);
                    soc_cm_sfree(unit, egr_mpls_exp_map);
                    QOS_UNLOCK(unit);
                    return (rv);
                }
            }

            if (SOC_MEM_IS_VALID(unit, EGR_MPLS_EXP_MAPPING_2m)) {
                rv = soc_mem_read_range(unit, EGR_MPLS_EXP_MAPPING_2m, MEM_BLOCK_ANY, 
                                        index, 
                                        (index + (_BCM_QOS_MAP_CHUNK_EGR_MPLS -1)),
                                        egr_mpls_exp_map2);
                if (BCM_FAILURE(rv)) {
                    soc_cm_sfree(unit, egr_mpls_pri_map);
                    soc_cm_sfree(unit, egr_mpls_exp_map);
                    soc_cm_sfree(unit, egr_mpls_exp_map2);
                    QOS_UNLOCK(unit);
                    return (rv);
                }
            }

            /* Modify what's needed */
            entry = &egr_mpls_pri_map[offset];
            soc_mem_field32_set(unit, EGR_MPLS_PRI_MAPPINGm, entry, 
                                NEW_PRIf, map->pkt_pri);
            soc_mem_field32_set(unit, EGR_MPLS_PRI_MAPPINGm, entry,
                                NEW_CFIf, map->pkt_cfi);
            if (SOC_MEM_IS_VALID(unit, EGR_MPLS_EXP_MAPPING_1m)) {
                entry = &egr_mpls_exp_map[offset];
                soc_mem_field32_set(unit, EGR_MPLS_EXP_MAPPING_1m, entry, 
                                    PRIf, map->pkt_pri);
                soc_mem_field32_set(unit, EGR_MPLS_EXP_MAPPING_1m, entry, 
                                    CFIf, map->pkt_cfi);
                if (flags & BCM_QOS_MAP_MPLS) {
                    soc_mem_field32_set(unit, EGR_MPLS_EXP_MAPPING_1m, entry, 
                                        MPLS_EXPf, map->exp);
                }
            }
            if (SOC_MEM_IS_VALID(unit, EGR_MPLS_EXP_MAPPING_2m)) {
                entry = &egr_mpls_exp_map2[offset];
                soc_mem_field32_set(unit, EGR_MPLS_EXP_MAPPING_2m, entry, 
                                    PRIf, map->pkt_pri);
                soc_mem_field32_set(unit, EGR_MPLS_EXP_MAPPING_2m, entry, 
                                    CFIf, map->pkt_cfi);
                if (flags & BCM_QOS_MAP_MPLS) {
                    soc_mem_field32_set(unit, EGR_MPLS_EXP_MAPPING_2m, entry, 
                                        MPLS_EXPf, map->exp);
                }
            }

            /* Delete the old profile chunk */
            rv = _bcm_egr_mpls_combo_map_entry_delete(unit, index);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, egr_mpls_pri_map);
                soc_cm_sfree(unit, egr_mpls_exp_map);
                soc_cm_sfree(unit, egr_mpls_exp_map2);
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Add new chunk and store new HW index */
            entries[0] = egr_mpls_pri_map;
            entries[1] = egr_mpls_exp_map;
            entries[2] = egr_mpls_exp_map2;
            rv = _bcm_egr_mpls_combo_map_entry_add(unit, entries,
                                                   _BCM_QOS_MAP_CHUNK_EGR_MPLS, 
                                                   (uint32 *)&index);
            QOS_INFO(unit)->egr_mpls_hw_idx[id] = (index / 
                                                  _BCM_QOS_MAP_CHUNK_EGR_MPLS);

            /* Free the DMA memory */
            soc_cm_sfree(unit, egr_mpls_pri_map);
            soc_cm_sfree(unit, egr_mpls_exp_map);
            soc_cm_sfree(unit, egr_mpls_exp_map2);
        }
        break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
        if (!_BCM_QOS_DSCP_TABLE_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_PARAM;
        } else {
            if (!(flags & BCM_QOS_MAP_L3) || !(flags & BCM_QOS_MAP_INGRESS)) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            if ((map->int_pri > 15) || (map->int_pri < 0) ||
                (map->dscp > DSCP_CODE_POINT_MAX) || (map->dscp < 0) ||
                ((map->color != bcmColorGreen) && 
                 (map->color != bcmColorYellow) && 
                 (map->color != bcmColorRed))) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }

            /* Allocate memory for DMA */
            alloc_size = _BCM_QOS_MAP_CHUNK_DSCP * sizeof(dscp_table_entry_t);
            dscp_table = soc_cm_salloc(unit, alloc_size,
                                       "TR2 dscp table");
            if (NULL == dscp_table) {
                QOS_UNLOCK(unit);
                return (BCM_E_MEMORY);
            }
            sal_memset(dscp_table, 0, alloc_size);

            /* Base index of table in hardware */
            index = QOS_INFO(unit)->dscp_hw_idx[id] * _BCM_QOS_MAP_CHUNK_DSCP;

            /* Offset within table */
            offset = map->dscp;

            /* Read the old profile chunk */
            rv = soc_mem_read_range(unit, DSCP_TABLEm, MEM_BLOCK_ANY, 
                                    index, index + (_BCM_QOS_MAP_CHUNK_DSCP-1),
                                    dscp_table);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, dscp_table);
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Modify what's needed */
            entry = &dscp_table[offset];
            cng = _BCM_COLOR_ENCODING(unit, map->color);
            soc_DSCP_TABLEm_field32_set(unit, entry, DSCPf, map->dscp);
            soc_DSCP_TABLEm_field32_set(unit, entry, PRIf, map->int_pri);
            soc_DSCP_TABLEm_field32_set(unit, entry, CNGf, cng);

            /* Delete the old profile chunk */
            rv = _bcm_dscp_table_entry_delete(unit, index);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, dscp_table);
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Add new chunk and store new HW index */
            entries[0] = dscp_table;
            rv = _bcm_dscp_table_entry_add(unit, entries, _BCM_QOS_MAP_CHUNK_DSCP, 
                                           (uint32 *)&index);
            QOS_INFO(unit)->dscp_hw_idx[id] = index / _BCM_QOS_MAP_CHUNK_DSCP;

            /* Free the DMA memory */
            soc_cm_sfree(unit, dscp_table);
        }
        break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
        if (!_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_PARAM;
        } else {
            if (!(flags & BCM_QOS_MAP_L3) || !(flags & BCM_QOS_MAP_EGRESS)) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            if ((map->int_pri > 15) || (map->int_pri < 0) ||
                (map->dscp > DSCP_CODE_POINT_MAX) || (map->dscp < 0) ||
                ((map->color != bcmColorGreen) && 
                 (map->color != bcmColorYellow) && 
                 (map->color != bcmColorRed))) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            /* Allocate memory for DMA */
            alloc_size = (_BCM_QOS_MAP_CHUNK_EGR_DSCP * 
                          sizeof(egr_dscp_table_entry_t));
            egr_dscp_table = soc_cm_salloc(unit, alloc_size,
                                           "TR2 egr_dscp table");
            if (NULL == egr_dscp_table) {
                QOS_UNLOCK(unit);
                return (BCM_E_MEMORY);
            }
            sal_memset(egr_dscp_table, 0, alloc_size);

            /* Base index of table in hardware */
            index = (QOS_INFO(unit)->egr_dscp_hw_idx[id] * 
                     _BCM_QOS_MAP_CHUNK_EGR_DSCP);

            /* Offset within table */
            cng = _BCM_COLOR_ENCODING(unit, map->color);
            offset = (map->int_pri << 2) | cng;

            /* Read the old profile chunk */
            rv = soc_mem_read_range(unit, EGR_DSCP_TABLEm, MEM_BLOCK_ANY, 
                                    index, 
                                    index + (_BCM_QOS_MAP_CHUNK_EGR_DSCP -1), 
                                    egr_dscp_table);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, egr_dscp_table);
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Modify what's needed */
            entry = &egr_dscp_table[offset];
            soc_EGR_DSCP_TABLEm_field32_set(unit, entry, DSCPf, map->dscp);

            /* Delete the old profile chunk */
            rv = _bcm_egr_dscp_table_entry_delete(unit, index);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, egr_dscp_table);
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Add new chunk and store new HW index */
            entries[0] = egr_dscp_table;
            rv = _bcm_egr_dscp_table_entry_add(unit, entries, 
                                               _BCM_QOS_MAP_CHUNK_EGR_DSCP, 
                                               (uint32 *)&index);
            QOS_INFO(unit)->egr_dscp_hw_idx[id] = (index / 
                                                  _BCM_QOS_MAP_CHUNK_EGR_DSCP);

            /* Free the DMA memory */
            soc_cm_sfree(unit, egr_dscp_table);
        }
        break;
#ifdef BCM_KATANA_SUPPORT
    case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
        if (!_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, id)) {
            QOS_UNLOCK(unit);
            return BCM_E_PARAM;
        } else {
            if ((map->int_pri > 15) || (map->int_pri < 0) ||
                (map->queue_offset < 0) || (map->queue_offset > 7)) { /* max no of queues =8 */
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            /* Allocate memory for DMA */
            alloc_size = _BCM_QOS_MAP_CHUNK_OFFSET_MAP * 
                               sizeof(ing_queue_offset_mapping_table_entry_t);
            offset_map_table = soc_cm_salloc(unit, alloc_size,
                                       "KT Offset map table");
            if (NULL == offset_map_table) {
                QOS_UNLOCK(unit);
                return (BCM_E_MEMORY);
            }
            sal_memset(offset_map_table, 0, alloc_size);

            /* Base index of table in hardware */
            index = QOS_INFO(unit)->offset_map_hw_idx[id] * 
                                   _BCM_QOS_MAP_CHUNK_OFFSET_MAP;

            /* Offset within table */
            offset = map->queue_offset;

            /* Read the old profile chunk */
            rv = soc_mem_read_range(unit, ING_QUEUE_OFFSET_MAPPING_TABLEm, 
                                    MEM_BLOCK_ANY, index, 
                                    index + (_BCM_QOS_MAP_CHUNK_OFFSET_MAP - 1),
                                    offset_map_table);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, offset_map_table);
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Modify what's needed */
            entry = &offset_map_table[map->int_pri];
            soc_ING_QUEUE_OFFSET_MAPPING_TABLEm_field32_set(unit, entry, 
                                           QUEUE_OFFSETf, map->queue_offset);

            /* Delete the old profile chunk */
            rv = _bcm_offset_map_table_entry_delete(unit, index);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, offset_map_table);
                QOS_UNLOCK(unit);
                return (rv);
            }

            /* Add new chunk and store new HW index */
            entries[0] = offset_map_table;
            rv = _bcm_offset_map_table_entry_add(unit, entries, 
                                      _BCM_QOS_MAP_CHUNK_OFFSET_MAP,
                                                    (uint32 *)&index);
            QOS_INFO(unit)->offset_map_hw_idx[id] = 
                                 index / _BCM_QOS_MAP_CHUNK_OFFSET_MAP;

            /* Free the DMA memory */
            soc_cm_sfree(unit, offset_map_table);
        }
        break;
#endif
    default:
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
        if (map_id & _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_INGRESS) {
            bcm_mpls_exp_map_t ing_exp_map;
            bcm_mpls_exp_map_t_init(&ing_exp_map);
            if (!(flags & BCM_QOS_MAP_MPLS) || !(flags & BCM_QOS_MAP_INGRESS)) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            if ((map->int_pri > 15) || (map->int_pri < 0) || 
                (map->exp > 7) || (map->exp < 0) || 
                ((map->color != bcmColorGreen) && 
                (map->color != bcmColorYellow) && 
                (map->color != bcmColorRed))) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            /* Call the triumph/mpls.c implementation */
            ing_exp_map.exp = map->exp;
            ing_exp_map.priority = map->int_pri;
            ing_exp_map.color = map->color;
            rv = bcm_tr_mpls_exp_map_set(unit, map_id, &ing_exp_map);
        } else
#endif /* defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT) */
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit) && (flags == BCM_QOS_MAP_SUBPORT)) {
            rv = bcm_kt2_subport_egr_subtag_dot1p_map_add(unit, map);
        } else
#endif /* defined(BCM_KATANA2_SUPPORT) */
        {
            QOS_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        break;
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    QOS_UNLOCK(unit);
    return rv;
}

/* Clear an entry from a QoS map */
/* Read the existing profile chunk, modify what's needed and add the 
 * new profile. This can result in the HW profile index changing for a 
 * given ID */
int 
bcm_tr2_qos_map_delete(int unit, uint32 flags, bcm_qos_map_t *map, int map_id)
{
    int rv = BCM_E_NONE;
    bcm_qos_map_t clr_map;

    QOS_INIT(unit);

    switch (map_id >> _BCM_QOS_MAP_SHIFT) {
    case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
        sal_memcpy(&clr_map, map, sizeof(clr_map));
        clr_map.int_pri = 0;
        clr_map.color = bcmColorGreen;
        rv = bcm_tr2_qos_map_add(unit, flags, &clr_map, map_id);
        break;
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
        sal_memcpy(&clr_map, map, sizeof(clr_map));
        clr_map.pkt_pri = 0;
        clr_map.pkt_cfi = 0;
        rv = bcm_tr2_qos_map_add(unit, flags, &clr_map, map_id);
        break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
        sal_memcpy(&clr_map, map, sizeof(clr_map));
        clr_map.int_pri = 0;
        clr_map.color = bcmColorGreen;
        rv = bcm_tr2_qos_map_add(unit, flags, &clr_map, map_id);
        break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
        sal_memcpy(&clr_map, map, sizeof(clr_map));
        clr_map.dscp = 0;
        rv = bcm_tr2_qos_map_add(unit, flags, &clr_map, map_id);
        break;
#ifdef BCM_KATANA_SUPPORT
    case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
        sal_memcpy(&clr_map, map, sizeof(clr_map));
        clr_map.queue_offset = 0;
        rv = bcm_tr2_qos_map_add(unit, flags, &clr_map, map_id);
        break;
#endif

    default:
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit) && (flags == BCM_QOS_MAP_SUBPORT)) {
            rv = bcm_kt2_subport_egr_subtag_dot1p_map_delete(unit, map);
        } else
#endif /* defined(BCM_KATANA2_SUPPORT) */
        {
            sal_memcpy(&clr_map, map, sizeof(clr_map));
            clr_map.int_pri = 0;
            clr_map.color = bcmColorGreen;
            rv = bcm_tr2_qos_map_add(unit, flags, &clr_map, map_id);
        }
        break;
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    return rv;
}

/* Attach a QoS map to an object (Gport) */
int 
bcm_tr2_qos_port_map_set(int unit, bcm_gport_t port, int ing_map, int egr_map)
{
    int index, id, alloc_size, rv = BCM_E_NONE, idx;
    uint8 pri, cfi;
    char *buf, *buf2;
    void *entries[1];
#ifdef INCLUDE_L3
    source_vp_entry_t svp;
    egr_wlan_dvp_entry_t wlan_dvp;
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS];
    int tunnel, vp, encap_id, nhi, l3_iif = 0;
    int tunnel_id_max, tunnel_count;
    egr_l3_next_hop_entry_t egr_nh;
    soc_field_t dot1p_pri_select_field=0;
    soc_field_t dot1p_mapping_ptr_field=0;
#endif

    QOS_INIT(unit);

    QOS_LOCK(unit);
#ifdef INCLUDE_L3
    if (BCM_GPORT_IS_SET(port) && !BCM_GPORT_IS_MODPORT(port) && 
        !BCM_GPORT_IS_LOCAL(port)) {
        /* Deal with different types of gports */
        if (_BCM_QOS_GPORT_IS_VFI_VP_PORT(port)) {
            /* Deal with MiM and MPLS ports */
            if (BCM_GPORT_IS_MIM_PORT(port)) {
                vp = BCM_GPORT_MIM_PORT_ID_GET(port);
                if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeMim)) {
                    QOS_UNLOCK(unit);
                    return BCM_E_BADID;
                } 
            } else if BCM_GPORT_IS_VXLAN_PORT(port) {
                vp = BCM_GPORT_VXLAN_PORT_ID_GET(port);
                if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
                    QOS_UNLOCK(unit);
                    return BCM_E_BADID;
                }
            } else if BCM_GPORT_IS_L2GRE_PORT(port) {
                vp = BCM_GPORT_L2GRE_PORT_ID_GET(port);
                if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
                    QOS_UNLOCK(unit);
                    return BCM_E_BADID;
                }
            } else {
                vp = BCM_GPORT_MPLS_PORT_ID_GET(port);
                if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeMpls)) {
                    QOS_UNLOCK(unit);
                    return BCM_E_BADID;
                } 
            }
            if (ing_map != -1) { /* -1 means no change */
                /* TRUST_DOT1P_PTR from SOURCE_VP table */
                rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
                if (ing_map == 0) {
                    /* Clear the existing map */
                    soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DOT1P_PTRf, 0);
                    if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_V4f)) {
                        soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_V4f, 0);
                    }
                    if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_V6f)) {
                        soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_V6f, 0);
                    }
                    if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_PTRf)) {
                        soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_PTRf, 0);
                    }
                } else {
                    /* Use the provided map */
                    id = ing_map & _BCM_QOS_MAP_TYPE_MASK; 
                    switch (ing_map >> _BCM_QOS_MAP_SHIFT) {
                      case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
                        if (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
                            QOS_UNLOCK(unit);
                            return BCM_E_PARAM;
                        }
                        soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DOT1P_PTRf,
                        QOS_INFO(unit)->pri_cng_hw_idx[id]);
                        break;

                      case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
                        if (!_BCM_QOS_DSCP_TABLE_USED_GET(unit, id)) {
                            QOS_UNLOCK(unit);
                            return BCM_E_PARAM;
                        }
                        if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_PTRf)) {
                            soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_PTRf,
                            QOS_INFO(unit)->dscp_hw_idx[id]);
                        }
                        if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_V4f)) {
                            soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_V4f, 1);
                        }

                        if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_V6f)) {
                            soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_V6f, 1);
                        }
                        break;
                    }
                }
                rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
            }
            if (egr_map != -1) { /* -1 means no change */
                /* SD_TAG::DOT1P_MAPPING_PTR from EGR_L3_NEXT_HOP */
                if (BCM_GPORT_IS_MIM_PORT(port)) {
                    rv = bcm_tr2_multicast_mim_encap_get(unit, 0, 0, port, 
                                                         &encap_id);
                } else {
                    rv = bcm_tr2_multicast_vpls_encap_get(unit, 0, 0, port, 
                                                          &encap_id);
                }
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
#if defined(BCM_ENDURO_SUPPORT)
                if (SOC_IS_ENDURO(unit)) {
                    nhi = encap_id;
                } else 
#endif /* BCM_ENDURO_SUPPORT */
                {
                    nhi = encap_id - BCM_XGS3_DVP_EGRESS_IDX_MIN;
                }
                rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                                  nhi, &egr_nh);
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
                /*  For Katana chip, bit position for DOT1P_PRI_SELECT and
                    DOT1P_MAPPING_PTR fields are different in MIM and SD_TAG
                    views hence below adding below expandable logic for
                    selecting appropriate fields.
                 */
                if (BCM_GPORT_IS_MIM_PORT(port)) {
                    if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm,
                                            MIM__DOT1P_PRI_SELECTf) &&
                        soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm,
                                            MIM__DOT1P_MAPPING_PTRf)) {
                        dot1p_pri_select_field  = MIM__DOT1P_PRI_SELECTf;
                        dot1p_mapping_ptr_field = MIM__DOT1P_MAPPING_PTRf;
                    } else {
                        QOS_UNLOCK(unit);
                        return BCM_E_INTERNAL;
                    }
                } else {
                    dot1p_pri_select_field  = SD_TAG__SD_TAG_DOT1P_PRI_SELECTf;
                    dot1p_mapping_ptr_field = SD_TAG__SD_TAG_DOT1P_MAPPING_PTRf;
                }

                if (egr_map == 0) {
                    /* Clear the existing map */
                    index = 0;
                    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                        dot1p_pri_select_field,0);
                } else {
                    /* Use the provided map */
                    if ((egr_map >> _BCM_QOS_MAP_SHIFT) != 
                         _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    id = egr_map & _BCM_QOS_MAP_TYPE_MASK;
                    if (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    index = QOS_INFO(unit)->egr_mpls_hw_idx[id];
                    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                        dot1p_pri_select_field,1);
                }
                soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,  
                                        dot1p_mapping_ptr_field, index);
                rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ALL, 
                                   nhi, &egr_nh);
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
            }
        } else if (BCM_GPORT_IS_WLAN_PORT(port)) {
            /* Deal with WLAN ports */
            vp = BCM_GPORT_WLAN_PORT_ID_GET(port);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeWlan)) {
                QOS_UNLOCK(unit);
                return BCM_E_BADID;
            }
            if (ing_map != -1) { /* -1 means no change */
                if (ing_map == 0) {
                    /* Clear the existing map */
                    index = 0;
                } else {
                    /* Use the provided map */
                    if ((ing_map >> _BCM_QOS_MAP_SHIFT) != 
                         _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    id = ing_map & _BCM_QOS_MAP_TYPE_MASK;
                    if (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    index = QOS_INFO(unit)->pri_cng_hw_idx[id];
                }
#if defined(BCM_TRIUMPH3_SUPPORT)
                if (SOC_IS_TRIUMPH3(unit)) {
                    rv = bcm_tr3_wlan_lport_field_set(unit, port,
                                                      TRUST_DOT1P_PTRf, index);
                } else
#endif /* BCM_TRIUMPH3_SUPPORT */
                { 
                    rv = bcm_tr2_wlan_lport_field_set(unit, port,
                                                      TRUST_DOT1P_PTRf, index);
                }
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
            }

            if (egr_map != -1) { /* -1 means no change */
#if defined(BCM_TRIUMPH3_SUPPORT)
                if (SOC_IS_TRIUMPH3(unit)) {
                    if (egr_map == 0) {
                        /* Clear the existing map */
                        index = 0;
                    } else {
                        if ((egr_map >> _BCM_QOS_MAP_SHIFT) != 
                             _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS) {
                            QOS_UNLOCK(unit);
                            return BCM_E_PARAM;
                        }
                        id = egr_map & _BCM_QOS_MAP_TYPE_MASK;
                        if (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
                            QOS_UNLOCK(unit);
                            return BCM_E_PARAM;
                        }
                        index = QOS_INFO(unit)->egr_mpls_hw_idx[id];
                    }

                    rv = _bcm_tr3_qos_wlan_port_map_set(
                                            unit, port, egr_map, index);
                    if (rv < 0) {
                        QOS_UNLOCK(unit);
                        return rv;
                    }
                } else
#endif /* BCM_TRIUMPH3_SUPPORT */
                {
                    /* EGR_WLAN_DVP.TUNNEL_INDEX points to CAPWAP initiator.
                     * EGR_IP_TUNNEL.DOT1P_MAPPING_PTR contains the map */
                    rv = READ_EGR_WLAN_DVPm(unit, MEM_BLOCK_ANY, vp, &wlan_dvp);
                    if (rv < 0) {
                        QOS_UNLOCK(unit);
                        return rv;
                    }
                    tunnel = soc_EGR_WLAN_DVPm_field32_get(unit, &wlan_dvp, 
                                                           TUNNEL_INDEXf);
                    rv = soc_mem_read(unit, EGR_IP_TUNNELm, MEM_BLOCK_ANY, 
                                      tunnel, tnl_entry);
                    if (rv < 0) {
                        QOS_UNLOCK(unit);
                        return rv;
                    }
                    if (egr_map == 0) {
                        /* Clear the existing map */
                        index = 0;
                        soc_mem_field32_set(unit, EGR_IP_TUNNELm, tnl_entry, 
                                            DOT1P_PRI_SELECTf, 0);
                    } else {
                        /* Use the provided map */
                        if ((egr_map >> _BCM_QOS_MAP_SHIFT) != 
                             _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS) {
                            QOS_UNLOCK(unit);
                            return BCM_E_PARAM;
                        }
                        id = egr_map & _BCM_QOS_MAP_TYPE_MASK;
                        if (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
                            QOS_UNLOCK(unit);
                            return BCM_E_PARAM;
                        }
                        index = QOS_INFO(unit)->egr_mpls_hw_idx[id];
                        soc_mem_field32_set(unit, EGR_IP_TUNNELm, tnl_entry, 
                                            DOT1P_PRI_SELECTf, 1);
                    }
                    soc_mem_field32_set(unit, EGR_IP_TUNNELm, tnl_entry,  
                                        DOT1P_MAPPING_PTRf, index);
                    rv = soc_mem_write(unit, EGR_IP_TUNNELm, MEM_BLOCK_ALL, 
                                       tunnel, tnl_entry);
                    if (rv < 0) {
                        QOS_UNLOCK(unit);
                        return rv;
                    }
                }
            }
        } else if (BCM_GPORT_IS_TUNNEL(port)) {
            /* Deal with tunnel initiators and terminators */
            tunnel = BCM_GPORT_TUNNEL_ID_GET(port);
            if (ing_map != -1) { /* -1 means no change */
                /* Get L3_IIF from L3_TUNNEL.
                 * Get TRUST_DSCP_PTR from L3_IIF */
                tunnel_id_max = soc_mem_index_max(unit, L3_TUNNELm);
                tunnel_count = soc_mem_index_count(unit, L3_TUNNELm);
                alloc_size = tunnel_count * sizeof(tnl_entry);
                buf = soc_cm_salloc(unit, alloc_size, "TR2 L3 TUNNEL");
                if (NULL == buf) {
                    QOS_UNLOCK(unit);
                    return (BCM_E_MEMORY);
                }
                rv = soc_mem_read_range(unit, L3_TUNNELm, MEM_BLOCK_ANY,  
                                        0, tunnel_id_max, (void *)buf);
                if (rv < 0) {
                    soc_cm_sfree(unit, buf);
                    QOS_UNLOCK(unit);
                    return rv;
                }

                if (SOC_MEM_FIELD_VALID(unit, L3_TUNNELm, TUNNEL_IDf)) {
                    for (index = 0; index < tunnel_id_max; index++) 
                    {
                        entries[0] = soc_mem_table_idx_to_pointer(unit, L3_TUNNELm, 
                                                                  void *, buf, 
                                                                  index);
                        if (tunnel == soc_mem_field32_get(unit, L3_TUNNELm, 
                                                          entries[0], TUNNEL_IDf)) {
                            /* Tunnel found - get L3_IIF */
                            l3_iif = soc_mem_field32_get(unit, L3_TUNNELm, 
                                                         entries[0], L3_IIFf);
                            break; 
                        }
                    }
                } else {

                    index = tunnel;
                }

                soc_cm_sfree(unit, buf);
                if (index == tunnel_id_max) {
                    QOS_UNLOCK(unit);
                    return BCM_E_NOT_FOUND; /* Tunnel not found */
                }
                if (ing_map == 0) {
                    /* Clear the existing map */
                    index = 0;
                } else {
                    /* Use the provided map */
                    if ((ing_map >> _BCM_QOS_MAP_SHIFT) != 
                         _BCM_QOS_MAP_TYPE_DSCP_TABLE) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    id = ing_map & _BCM_QOS_MAP_TYPE_MASK;
                    if (!_BCM_QOS_DSCP_TABLE_USED_GET(unit, id)) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    index = QOS_INFO(unit)->dscp_hw_idx[id];
                }
                if (soc_mem_field_valid(unit, L3_IIFm, TRUST_DSCP_PTRf)) {
                    rv = soc_mem_field32_modify(unit, L3_IIFm, l3_iif, 
                                            TRUST_DSCP_PTRf, index);
                }
#ifdef BCM_TRIDENT2_SUPPORT
                if (SOC_IS_TD2_TT2(unit)) { /* Get l3 iif profile entry */
                    uint32 l3_index;
                    void *entries[1];
                    iif_profile_entry_t l3_iif_profile;
                    iif_entry_t entry;
                    entries[0] = &l3_iif_profile;

                    /* Read interface config from hw. */
                    rv = soc_mem_read(unit, L3_IIFm, MEM_BLOCK_ANY, l3_iif,
                                      (uint32 *)&entry);
                    if (BCM_SUCCESS(rv)) {
                        l3_index = soc_mem_field32_get(unit, L3_IIFm, 
                                                       (uint32 *)&entry,
                                                      L3_IIF_PROFILE_INDEXf); 
                        rv = _bcm_l3_iif_profile_entry_get(unit, 
                                              l3_index, 1, entries);
                        if (BCM_SUCCESS(rv)) {
                            (void)soc_mem_field32_set(unit, L3_IIF_PROFILEm, 
                                                      &l3_iif_profile, 
                                                      TRUST_DSCP_PTRf, index);
                        }
                        _bcm_l3_iif_profile_entry_update(unit, entries, l3_index);
                    }
                }
#endif /* BCM_TRIDENT2_SUPPORT */
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
            }
            if (egr_map != -1) { /* -1 means no change */
                /* DSCP_MAPPING_PTR from EGR_IP_TUNNEL */
                rv = soc_mem_read(unit, EGR_IP_TUNNELm, MEM_BLOCK_ANY, 
                                  tunnel, tnl_entry);
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
                if (egr_map == 0) {
                    /* Clear existing map */
                    index = 0;
                    soc_mem_field32_set(unit, EGR_IP_TUNNELm, tnl_entry, 
                                        DSCP_SELf, 0);
                } else {
                    /* Use the provided map */
                    if ((egr_map >> _BCM_QOS_MAP_SHIFT) != 
                         _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    id = egr_map & _BCM_QOS_MAP_TYPE_MASK;
                    if (!_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, id)) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    index = QOS_INFO(unit)->egr_dscp_hw_idx[id];
                    soc_mem_field32_set(unit, EGR_IP_TUNNELm, tnl_entry, 
                                        DSCP_SELf, 2);
                }
                soc_mem_field32_set(unit, EGR_IP_TUNNELm, tnl_entry,  
                                    DSCP_MAPPING_PTRf, index);
                rv = soc_mem_write(unit, EGR_IP_TUNNELm, MEM_BLOCK_ALL, 
                                   tunnel, tnl_entry);
                if (rv < 0) {
                    QOS_UNLOCK(unit);
                    return rv;
                }
            }
        }
    } else
#endif
    {
        /* Deal with physical ports */
        if (BCM_GPORT_IS_SET(port)) {
            if (bcm_esw_port_local_get(unit, port, &port) < 0) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
        }
        if (!SOC_PORT_VALID(unit, port)) {
            QOS_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        if (ing_map != -1) { /* -1 means no change */
            /* Make the port's TRUST_DOT1_PTR point to the profile index */
            if (ing_map == 0) {
                /* Clear the existing map */
                soc_mem_lock(unit, PORT_TABm);
                rv = _bcm_tr2_port_tab_set(unit, port, TRUST_DOT1P_PTRf, 0);
                soc_mem_unlock(unit, PORT_TABm);      
            } else {
                id = ing_map & _BCM_QOS_MAP_TYPE_MASK; 
                switch (ing_map >> _BCM_QOS_MAP_SHIFT) {
                  case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
                    if (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
                        QOS_UNLOCK(unit);
                        return BCM_E_PARAM;
                    }
                    soc_mem_lock(unit, PORT_TABm);
                    rv = _bcm_tr2_port_tab_set(unit, port, TRUST_DOT1P_PTRf,
                                           QOS_INFO(unit)->pri_cng_hw_idx[id]);
                    soc_mem_unlock(unit, PORT_TABm);
                    break;
                  case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
                    if (SOC_IS_KATANA(unit)) {
                        /* TRUST_DSCP_PTR is already reserved for katana. So don't overwrite
                         * Return error if user is tyring to set some other map other than
                         * the default one.
                         */
                        soc_mem_lock(unit, PORT_TABm);
                        rv = _bcm_tr2_port_tab_get(unit, port,TRUST_DSCP_PTRf, &idx);
                        if (ing_map != idx) {
                            rv = BCM_E_CONFIG;
                        }
                        soc_mem_unlock(unit, PORT_TABm);
                    }
                    break;
                  default:
                    rv = BCM_E_CONFIG;
                }
            }
        }
        if (egr_map != -1) { /* -1 means no change */
            /* Copy the corresponding chunk from EGR_MPLS profiles to the 
             * EGR_PRI_CNG_MAP table, which is directly indexed by port */
            alloc_size = 64 * sizeof(egr_pri_cng_map_entry_t);
            buf = soc_cm_salloc(unit, alloc_size, "TR2 egr pri cng map");
            if (NULL == buf) {
                QOS_UNLOCK(unit);
                return (BCM_E_MEMORY);
            }
            sal_memset(buf, 0, alloc_size);
            index = port << 6;
            if (egr_map == 0) {
                /* Clear the existing map */
                rv = soc_mem_write_range(unit, EGR_PRI_CNG_MAPm, MEM_BLOCK_ALL, 
                                         index, index + 63, (void *)buf);
                if (BCM_FAILURE(rv)) {
                    soc_cm_sfree(unit, buf);
                    QOS_UNLOCK(unit);
                    return rv;
                }
            } else {
                if ((egr_map >> _BCM_QOS_MAP_SHIFT) != 
                     _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS) {
                    QOS_UNLOCK(unit);
                    return BCM_E_PARAM;
                }
                id = egr_map & _BCM_QOS_MAP_TYPE_MASK;
                if (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
                    QOS_UNLOCK(unit);
                    return BCM_E_PARAM;
                }
                buf2 = soc_cm_salloc(unit, alloc_size, "TR2 egr mpls exp map");
                if (NULL == buf2) {
                    soc_cm_sfree(unit, buf);
                    QOS_UNLOCK(unit);
                    return (BCM_E_MEMORY);
                }
                sal_memset(buf2, 0, alloc_size);
                index = (QOS_INFO(unit)->egr_mpls_hw_idx[id] * 
                         _BCM_QOS_MAP_CHUNK_EGR_MPLS);
                rv = soc_mem_read_range(unit, EGR_MPLS_EXP_MAPPING_1m,  
                                        MEM_BLOCK_ANY, index, index + 63, 
                                        (void *)buf2);
                if (BCM_FAILURE(rv)) {
                    soc_cm_sfree(unit, buf);
                    soc_cm_sfree(unit, buf2);
                    QOS_UNLOCK(unit);
                    return rv;
                }
                for (index = 0; index < _BCM_QOS_MAP_CHUNK_EGR_MPLS; index++) {
                    entries[0] = soc_mem_table_idx_to_pointer(unit, 
                                                              EGR_MPLS_EXP_MAPPING_1m, 
                                                              void *, buf2, index);
                    pri = soc_mem_field32_get(unit, EGR_MPLS_EXP_MAPPING_1m, 
                                              entries[0], PRIf);
                    cfi = soc_mem_field32_get(unit, EGR_MPLS_EXP_MAPPING_1m, 
                                              entries[0], CFIf);
                    entries[0] = soc_mem_table_idx_to_pointer(unit, 
                                              EGR_PRI_CNG_MAPm, 
                                              void *, buf, index);
                    soc_mem_field32_set(unit, EGR_PRI_CNG_MAPm, 
                                        entries[0], PRIf, pri);
                    soc_mem_field32_set(unit, EGR_PRI_CNG_MAPm, 
                                        entries[0], CFIf, cfi);
                } 
                index = port << 6;
                rv = soc_mem_write_range(unit, EGR_PRI_CNG_MAPm, MEM_BLOCK_ALL, 
                                         index, index + 63, (void *)buf);
                if (BCM_FAILURE(rv)) {
                    soc_cm_sfree(unit, buf);
                    soc_cm_sfree(unit, buf2);
                    QOS_UNLOCK(unit);
                    return rv;
                }
                soc_cm_sfree(unit, buf2);
            }
            soc_cm_sfree(unit, buf);
        }
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    QOS_UNLOCK(unit);
    return rv;
}

/* Retrieve a QOS map attached to an Object (Gport) */
int
bcm_tr2_qos_port_map_get(int unit, bcm_gport_t port, 
                         int *ing_map, int *egr_map)
{
    return BCM_E_UNAVAIL;
}

/* Attach a QoS map to vlan,port */
int 
bcm_tr2_qos_port_vlan_map_set(int unit, bcm_gport_t port, bcm_vlan_t vid, int ing_map, int egr_map)
{
    uint32 vlan_buf[SOC_MAX_MEM_FIELD_WORDS];
    vlan_profile_tab_entry_t entry;
    pbmp_t pbmp, ubmp;
    int rv, vlan_profile_index;
    int id, cur_id, map_id;

    QOS_INIT(unit);

    /* Validate Args */
    /* Deal with physical ports */
    if (BCM_GPORT_IS_SET(port)) {
        if (bcm_esw_port_local_get(unit, port, &port) < 0) {
            return BCM_E_PARAM;
        }
    }

    if (vid == BCM_VLAN_NONE) {
        return BCM_E_PARAM;
    }

    if (!SOC_PORT_VALID(unit, port)) {
        return BCM_E_PARAM;
    }

    /* check vlan membership for the port */
    BCM_IF_ERROR_RETURN(
        bcm_esw_vlan_port_get(unit, vid, &pbmp, &ubmp));
    if (!SOC_PBMP_MEMBER(pbmp, port)) {
        return BCM_E_PARAM;
    }

    if (egr_map != -1) { 
        /* Egress vlan qos map support not available */
        return BCM_E_UNAVAIL;
    }

    QOS_LOCK(unit);

    map_id = 0;
    if (ing_map != -1) { /* -1 means no change */
        /* Make the port's vlan TRUST_DOT1_PTR point to the profile index */
        if (ing_map != 0) {
            if ((ing_map >> _BCM_QOS_MAP_SHIFT) != 
                 _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            id = ing_map & _BCM_QOS_MAP_TYPE_MASK;
            if (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
                QOS_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            map_id = QOS_INFO(unit)->pri_cng_hw_idx[id];
        }
    }

    soc_mem_lock(unit, VLAN_TABm);

    rv = soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, vid, vlan_buf);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }
    
    /* Check for VLAN validity */
    if (0 == soc_mem_field32_get(unit, VLAN_TABm, vlan_buf, VALIDf)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return BCM_E_NOT_FOUND;
    }

    vlan_profile_index = soc_mem_field32_get(unit, VLAN_TABm, vlan_buf, VLAN_PROFILE_PTRf);

    rv = soc_mem_read(unit, VLAN_PROFILE_TABm, MEM_BLOCK_ANY, vlan_profile_index, &entry);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    cur_id = soc_VLAN_PROFILE_TABm_field32_get(unit, &entry, TRUST_DOT1P_PTRf);
    if (map_id != cur_id) {
        soc_VLAN_PROFILE_TABm_field32_set(unit, &entry, TRUST_DOT1P_PTRf,
                                                        map_id);
        if (SOC_MEM_FIELD_VALID(unit, VLAN_PROFILE_TABm, TRUST_DOT1Pf)) {
            soc_VLAN_PROFILE_TABm_field32_set(unit, &entry, TRUST_DOT1Pf, 1);
        }
        rv = soc_mem_write(unit, VLAN_PROFILE_TABm, MEM_BLOCK_ALL, vlan_profile_index, &entry);
    }

    soc_mem_unlock(unit, VLAN_TABm);
    QOS_UNLOCK(unit);
    return rv;
}


/* Retrieve a QOS map attached to an vlan and port */
int
bcm_tr2_qos_port_vlan_map_get(int unit, bcm_gport_t port, bcm_vlan_t vid,
                         int *ing_map, int *egr_map)
{

    uint32 vlan_buf[SOC_MAX_MEM_FIELD_WORDS];
    vlan_profile_tab_entry_t entry;
    pbmp_t pbmp, ubmp;
    int rv, idx, vlan_profile_index;
    int map_id;

    QOS_INIT(unit);

    if (ing_map == NULL) {
        return BCM_E_PARAM;
    }

    /* Deal with physical ports */
    if (BCM_GPORT_IS_SET(port)) {
        if (bcm_esw_port_local_get(unit, port, &port) < 0) {
            return BCM_E_PARAM;
        }
    }

    if (vid == BCM_VLAN_NONE) {
        return BCM_E_PARAM;
    }

    if (!SOC_PORT_VALID(unit, port)) {
        return BCM_E_PARAM;
    }


    /* check vlan membership for the port */
    BCM_IF_ERROR_RETURN(
        bcm_esw_vlan_port_get(unit, vid, &pbmp, &ubmp));
    if (!SOC_PBMP_MEMBER(pbmp, port)) {
        return BCM_E_PARAM;
    }

    QOS_LOCK(unit);

    soc_mem_lock(unit, VLAN_TABm);

    rv = soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, vid, vlan_buf);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }
    
    /* Check for VLAN validity */
    if (0 == soc_mem_field32_get(unit, VLAN_TABm, vlan_buf, VALIDf)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return BCM_E_NOT_FOUND;
    }

    vlan_profile_index = soc_mem_field32_get(unit, VLAN_TABm, vlan_buf, VLAN_PROFILE_PTRf);

    rv = soc_mem_read(unit, VLAN_PROFILE_TABm, MEM_BLOCK_ANY, vlan_profile_index, &entry);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    map_id = soc_VLAN_PROFILE_TABm_field32_get(unit, &entry, TRUST_DOT1P_PTRf);

    soc_mem_unlock(unit, VLAN_TABm);

    *ing_map = -1;
    for (idx = 0; idx < _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP; idx++) {
        if (_BCM_QOS_ING_PRI_CNG_USED_GET(unit, idx) &&
            (QOS_INFO(unit)->pri_cng_hw_idx[idx] == map_id)) {
            *ing_map = idx | 
                    (_BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP << _BCM_QOS_MAP_SHIFT);
            break;
        }
    }

    if (egr_map != NULL) {
        *egr_map = -1;
    }

    QOS_UNLOCK(unit);
    return rv;
}

/* bcm_tr2_qos_map_multi_get */
int
bcm_tr2_qos_map_multi_get(int unit, uint32 flags,
                          int map_id, int array_size, 
                          bcm_qos_map_t *array, int *array_count)
{
    int             rv = BCM_E_NONE;
    int             num_entries, idx, id, hw_id, alloc_size, entry_size, count;
    uint8           *dma_buf;
    void            *entry;
    soc_mem_t       mem;
    bcm_qos_map_t   *map;

    /* ignore with_id & replace flags */
    flags &= (~(BCM_QOS_MAP_WITH_ID | BCM_QOS_MAP_REPLACE));
    id = map_id & _BCM_QOS_MAP_TYPE_MASK;
    mem = INVALIDm;
    hw_id = 0;
    num_entries = 0;
    entry_size = 0;
    QOS_LOCK(unit);
    switch (map_id >> _BCM_QOS_MAP_SHIFT) {
    case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
        if ((flags != (BCM_QOS_MAP_L2 | BCM_QOS_MAP_INGRESS)) ||
            (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id))) {
            rv = BCM_E_PARAM;
        }
        num_entries = _BCM_QOS_MAP_CHUNK_PRI_CNG;
        entry_size = sizeof(ing_pri_cng_map_entry_t);
        hw_id = QOS_INFO(unit)->pri_cng_hw_idx[id]* _BCM_QOS_MAP_CHUNK_PRI_CNG;
        mem = ING_PRI_CNG_MAPm;
        break;
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
        if (((flags != (BCM_QOS_MAP_L2 | BCM_QOS_MAP_EGRESS)) &&
             (flags != (BCM_QOS_MAP_MPLS | BCM_QOS_MAP_EGRESS))) ||
            (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id))) {
            rv = BCM_E_PARAM;
        }
        num_entries = _BCM_QOS_MAP_CHUNK_EGR_MPLS;
        entry_size = sizeof(egr_mpls_pri_mapping_entry_t);
        hw_id = (QOS_INFO(unit)->egr_mpls_hw_idx[id] * 
                _BCM_QOS_MAP_CHUNK_EGR_MPLS);
        mem = EGR_MPLS_PRI_MAPPINGm;
        break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
        if ((flags != (BCM_QOS_MAP_L3 | BCM_QOS_MAP_INGRESS)) ||
            (!_BCM_QOS_DSCP_TABLE_USED_GET(unit, id))) {
            rv = BCM_E_PARAM;
        }
        num_entries = _BCM_QOS_MAP_CHUNK_DSCP;
        entry_size = sizeof(dscp_table_entry_t);
        hw_id = QOS_INFO(unit)->dscp_hw_idx[id] * _BCM_QOS_MAP_CHUNK_DSCP;
        mem = DSCP_TABLEm;
        break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
        if ((flags != (BCM_QOS_MAP_L3 | BCM_QOS_MAP_EGRESS)) ||
            (!_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, id))) {
            rv = BCM_E_PARAM;
        }
        num_entries = _BCM_QOS_MAP_CHUNK_EGR_DSCP;
        entry_size = sizeof(egr_dscp_table_entry_t);
        hw_id = (QOS_INFO(unit)->egr_dscp_hw_idx[id] * 
                _BCM_QOS_MAP_CHUNK_EGR_DSCP);
        mem = EGR_DSCP_TABLEm;
        break;
#ifdef BCM_KATANA_SUPPORT
    case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
        if ((flags != (BCM_QOS_MAP_QUEUE)) ||
            (!_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, id))) {
            rv = BCM_E_PARAM;
        }
        num_entries = _BCM_QOS_MAP_CHUNK_OFFSET_MAP;
        entry_size = sizeof(ing_queue_offset_mapping_table_entry_t);
        hw_id = QOS_INFO(unit)->offset_map_hw_idx[id] *
                                       _BCM_QOS_MAP_CHUNK_OFFSET_MAP;
        mem = ING_QUEUE_OFFSET_MAPPING_TABLEm;
        break;
#endif
    default:
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
        if ((map_id & _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_MASK) == 
            _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_INGRESS) {
            /* Map created using TR mpls module */
            if (flags != (BCM_QOS_MAP_MPLS | BCM_QOS_MAP_INGRESS)) {
                rv = BCM_E_PARAM;
            }
            id = map_id & _BCM_TR_MPLS_EXP_MAP_TABLE_NUM_MASK;
            if (!_BCM_QOS_ING_MPLS_EXP_USED_GET(unit, id)) {
                rv = BCM_E_PARAM;
            }
            num_entries = _BCM_QOS_MAP_CHUNK_ING_MPLS_EXP;
            entry_size = sizeof(ing_mpls_exp_mapping_entry_t);
            hw_id = id * num_entries; /* soc profile mem is not used. So direct hw index */
            mem = ING_MPLS_EXP_MAPPINGm;
        } else 
#endif /* INCLUDE_L3 && BCM_MPLS_SUPPORT */
        {
            rv = BCM_E_PARAM;
        }
    }
    QOS_UNLOCK(unit);
    BCM_IF_ERROR_RETURN(rv);

    if (array_size == 0) { /* querying the size of map-chunk */
        *array_count = num_entries;
        return BCM_E_NONE;
    }
    if (!array || !array_count) {
        return BCM_E_PARAM;
    }

    /* Allocate memory for DMA & regular */
    alloc_size = num_entries * entry_size;
    dma_buf = soc_cm_salloc(unit, alloc_size, "TR2 qos multi get DMA buf");
    if (!dma_buf) {
        return BCM_E_MEMORY;
    }
    sal_memset(dma_buf, 0, alloc_size);

    /* Read the profile chunk */
    rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, hw_id, 
                            (hw_id + num_entries - 1), dma_buf);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, dma_buf);
        return rv;
    }

    count = 0;
    QOS_LOCK(unit);
    switch (map_id >> _BCM_QOS_MAP_SHIFT) {
    case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
        for (idx=0; ((idx<num_entries) && (count < array_size)); idx++) {
            map = &array[idx];
            sal_memset(map, 0x0, sizeof(bcm_qos_map_t));
            map->pkt_pri = ((idx & 0x1e) >> 1);
            map->pkt_cfi = (idx & 0x1);
            entry = soc_mem_table_idx_to_pointer(unit, mem, void *, 
                                                 dma_buf, idx);
            map->int_pri = soc_mem_field32_get(unit, mem, entry, PRIf);
            map->color = _BCM_COLOR_DECODING(unit, soc_mem_field32_get(unit, 
                                 mem, entry, CNGf));
            count++;
        }
        break;
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
        for (idx=0; ((idx<num_entries) && (count < array_size)); idx++) {
            map = &array[idx];
            sal_memset(map, 0x0, sizeof(bcm_qos_map_t));
            map->int_pri = ((idx & 0x3c) >> 2);
            map->color = _BCM_COLOR_DECODING(unit, (idx & 0x3));
            entry = soc_mem_table_idx_to_pointer(unit, mem, void *, 
                                                 dma_buf, idx);
            map->pkt_pri = soc_mem_field32_get(unit, mem, entry, NEW_PRIf);
            map->pkt_cfi = soc_mem_field32_get(unit, mem, entry, NEW_CFIf);
            count++;
        }
        break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
        for (idx=0; ((idx<num_entries) && (count < array_size)); idx++) {
            map = &array[idx];
            sal_memset(map, 0x0, sizeof(bcm_qos_map_t));
            map->dscp = idx;
            entry = soc_mem_table_idx_to_pointer(unit, mem, void *, 
                                                 dma_buf, idx);
            map->int_pri = soc_mem_field32_get(unit, mem, entry, PRIf);
            map->color = _BCM_COLOR_DECODING(unit, soc_mem_field32_get(unit, 
                                 mem, entry, CNGf));
            count++;
        }
        break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
        for (idx=0; ((idx<num_entries) && (count < array_size)); idx++) {
            map = &array[idx];
            sal_memset(map, 0x0, sizeof(bcm_qos_map_t));
            map->int_pri = ((idx & 0x3c) >> 2);
            map->color = _BCM_COLOR_DECODING(unit, (idx & 0x3));
            entry = soc_mem_table_idx_to_pointer(unit, mem, void *, 
                                                 dma_buf, idx);
            map->dscp = soc_mem_field32_get(unit, mem, entry, DSCPf);
            count++;
        }
        break;
#ifdef BCM_KATANA_SUPPORT
    case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
        for (idx=0; ((idx<num_entries) && (count < array_size)); idx++) {
            map = &array[idx];
            sal_memset(map, 0x0, sizeof(bcm_qos_map_t));
            map->int_pri = idx;
            entry = soc_mem_table_idx_to_pointer(unit, mem, void *,
                                                 dma_buf, idx);
            map->queue_offset = soc_mem_field32_get(unit, mem, entry, 
                                                          QUEUE_OFFSETf);
            count++;
        }
        break;
#endif
    default:
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
        if ((map_id & _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_MASK) == 
            _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_INGRESS) {
            for (idx=0; ((idx<num_entries) && (count < array_size)); idx++) {
                map = &array[idx];
                sal_memset(map, 0x0, sizeof(bcm_qos_map_t));
                map->exp = idx;
                entry = soc_mem_table_idx_to_pointer(unit, mem, void *, 
                                                     dma_buf, idx);
                map->color = _BCM_COLOR_DECODING(unit,soc_mem_field32_get(unit,
                                 mem, entry, CNGf));
                map->int_pri = soc_mem_field32_get(unit, mem, entry, PRIf);
                count++;
            }
        } else 
#endif /* INCLUDE_L3 && BCM_MPLS_SUPPORT */
        {
            rv = BCM_E_INTERNAL; /* should not hit this */
        }
    }
    QOS_UNLOCK(unit);
    soc_cm_sfree(unit, dma_buf);

    BCM_IF_ERROR_RETURN(rv);

    *array_count = count;
    return BCM_E_NONE;
}

/* Get the list of all created Map-ids along with corresponding flags */
int
bcm_tr2_qos_multi_get(int unit, int array_size, int *map_ids_array, 
                      int *flags_array, int *array_count)
{
    int rv = BCM_E_NONE;
    int idx, count;

    QOS_INIT(unit);
    QOS_LOCK(unit);
    if (array_size == 0) {
        /* querying the number of map-ids for storage allocation */
        if (array_count == NULL) {
            rv = BCM_E_PARAM;
        }
        if (BCM_SUCCESS(rv)) {
            count = 0;
            *array_count = 0;
            SHR_BITCOUNT_RANGE(QOS_INFO(unit)->ing_pri_cng_bitmap, count, 
                               0, _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP);
            *array_count += count;
            count = 0;
            SHR_BITCOUNT_RANGE(QOS_INFO(unit)->egr_mpls_bitmap, count,
                               0, _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS);
            *array_count += count;
            count = 0;
            SHR_BITCOUNT_RANGE(QOS_INFO(unit)->dscp_table_bitmap, count,
                               0, _BCM_QOS_MAP_LEN_DSCP_TABLE);
            *array_count += count;
            count = 0;
            SHR_BITCOUNT_RANGE(QOS_INFO(unit)->egr_dscp_table_bitmap, count,
                               0, _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE);
            *array_count += count;
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
            count = 0;
            SHR_BITCOUNT_RANGE(QOS_INFO(unit)->ing_mpls_exp_bitmap, count,
                               0, _BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP);
            *array_count += count;
#endif
#ifdef BCM_KATANA_SUPPORT
            count = 0;
            SHR_BITCOUNT_RANGE(QOS_INFO(unit)->offset_map_table_bitmap, count,
                               0, _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE);
            *array_count += count;
#endif
        }
    } else {
        if ((map_ids_array == NULL) || (flags_array == NULL) || 
            (array_count == NULL)) {
            rv = BCM_E_PARAM;
        }
        if (BCM_SUCCESS(rv)) {
            count = 0;
            for (idx = 0; ((idx < _BCM_QOS_MAP_LEN_ING_PRI_CNG_MAP) && 
                         (count < array_size)); idx++) {
                if (_BCM_QOS_ING_PRI_CNG_USED_GET(unit, idx)) {
                    *(map_ids_array + count) = idx | 
                     (_BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP << _BCM_QOS_MAP_SHIFT);
                    *(flags_array + count) = (BCM_QOS_MAP_L2 | 
                                              BCM_QOS_MAP_INGRESS);
                    count++;
                }
            }
            for (idx = 0; ((idx < _BCM_QOS_MAP_LEN_EGR_MPLS_MAPS) && 
                         (count < array_size)); idx++) {
                if (_BCM_QOS_EGR_MPLS_USED_GET(unit, idx)) {
                    *(map_ids_array + count) = idx | 
                     (_BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS << _BCM_QOS_MAP_SHIFT);
                    *(flags_array + count) = (BCM_QOS_MAP_EGRESS |
                                 (_BCM_QOS_EGR_MPLS_FLAGS_USED_GET(unit, idx)? 
                                  BCM_QOS_MAP_MPLS : BCM_QOS_MAP_L2));
                    count++;
                }
            }
            for (idx = 0; ((idx < _BCM_QOS_MAP_LEN_DSCP_TABLE) && 
                         (count < array_size)); idx++) {
                if (_BCM_QOS_DSCP_TABLE_USED_GET(unit, idx)) {
                    *(map_ids_array + count) = idx | 
                     (_BCM_QOS_MAP_TYPE_DSCP_TABLE << _BCM_QOS_MAP_SHIFT);
                    *(flags_array + count) = (BCM_QOS_MAP_L3 | 
                                              BCM_QOS_MAP_INGRESS);
                    count++;
                }
            }
            for (idx = 0; ((idx < _BCM_QOS_MAP_LEN_EGR_DSCP_TABLE) && 
                         (count < array_size)); idx++) {
                if (_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, idx)) {
                    *(map_ids_array + count) = idx | 
                     (_BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE << _BCM_QOS_MAP_SHIFT);
                    *(flags_array + count) = (BCM_QOS_MAP_L3 | 
                                              BCM_QOS_MAP_EGRESS);
                    count++;
                }
            }
#if defined(INCLUDE_L3) && defined(BCM_MPLS_SUPPORT)
            for (idx = 0; ((idx < _BCM_QOS_MAP_LEN_ING_MPLS_EXP_MAP) && 
                         (count < array_size)); idx++) {
                if (_BCM_QOS_ING_MPLS_EXP_USED_GET(unit, idx)) {
                    *(map_ids_array + count) = (idx | 
                                      _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_INGRESS);
                    *(flags_array + count) = (BCM_QOS_MAP_MPLS | 
                                              BCM_QOS_MAP_INGRESS);
                    count++;
                }
            }
#endif /* INCLUDE_L3 && BCM_MPLS_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
            for (idx = 0; ((idx < _BCM_QOS_MAP_LEN_OFFSET_MAP_TABLE) &&
                         (count < array_size)); idx++) {
                if (_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, idx)) {
                    *(map_ids_array + count) = idx |
                     (_BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE << 
                                                     _BCM_QOS_MAP_SHIFT);
                    *(flags_array + count) = BCM_QOS_MAP_QUEUE;
                    count++;
                }
            }
#endif /* BCM_KATANA_SUPPORT */
            *array_count = count;
        }
    }
    QOS_UNLOCK(unit);
    return rv;
}

/* Function:
*	   _bcm_tr2_qos_id2idx
* Purpose:
*	   Translate map ID into hardware table index used by API
* Parameters:
* Returns:
*	   BCM_E_XXX
*/	   
int
_bcm_tr2_qos_id2idx(int unit, int map_id, int *hw_idx)
{
    int id;

    QOS_INIT(unit);

    id = map_id & _BCM_QOS_MAP_TYPE_MASK;

    QOS_LOCK(unit);
    switch (map_id >> _BCM_QOS_MAP_SHIFT) {
    case _BCM_QOS_MAP_TYPE_ING_PRI_CNG_MAP:
         if (!_BCM_QOS_ING_PRI_CNG_USED_GET(unit, id)) {
              QOS_UNLOCK(unit);
              return BCM_E_NOT_FOUND;
         } else {
              *hw_idx = QOS_INFO(unit)->pri_cng_hw_idx[id];
         }
         break;
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
         if (!_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
              QOS_UNLOCK(unit);
              return BCM_E_NOT_FOUND;
         } else {
              *hw_idx = QOS_INFO(unit)->egr_mpls_hw_idx[id];
         }
         break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
         if (!_BCM_QOS_DSCP_TABLE_USED_GET(unit, id)) {
              QOS_UNLOCK(unit);
              return BCM_E_NOT_FOUND;
         } else {
              *hw_idx = QOS_INFO(unit)->dscp_hw_idx[id];
         }
         break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
         if (!_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, id)) {
              QOS_UNLOCK(unit);
              return BCM_E_NOT_FOUND;
         } else {
              *hw_idx =  QOS_INFO(unit)->egr_dscp_hw_idx[id];
         }
         break;
#ifdef BCM_KATANA_SUPPORT
    case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
         if (!_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, id)) {
              QOS_UNLOCK(unit);
              return BCM_E_NOT_FOUND;
         } else {
              *hw_idx = QOS_INFO(unit)->offset_map_hw_idx[id];
         }
         break;
#endif
    default:
         QOS_UNLOCK(unit);
         return BCM_E_NOT_FOUND;
    }
    QOS_UNLOCK(unit);
    return BCM_E_NONE;
}

/* Function:
*	   _bcm_tr2_qos_id2idx
* Purpose:
*	   Translate hardware table index into map ID used by API 
* Parameters:
* Returns:
*	   BCM_E_XXX
*/	   
int
_bcm_tr2_qos_idx2id(int unit, int hw_idx, int type, int *map_id)
{
    int id, num_map;

    switch (type) {
    case _BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS:
         num_map = soc_mem_index_count(unit, EGR_MPLS_EXP_MAPPING_1m) / 64;
         for (id = 0; id < num_map; id++) {
              if (_BCM_QOS_EGR_MPLS_USED_GET(unit, id)) {
                   if (QOS_INFO(unit)->egr_mpls_hw_idx[id] == hw_idx) {
                        *map_id = id | (_BCM_QOS_MAP_TYPE_EGR_MPLS_MAPS 
                                                                << _BCM_QOS_MAP_SHIFT);
                        return BCM_E_NONE;
                   }
              }
         }
         break;
    case _BCM_QOS_MAP_TYPE_DSCP_TABLE:
         num_map = soc_mem_index_count(unit, DSCP_TABLEm) / 64;
         for (id = 0; id < num_map; id++) {
              if (_BCM_QOS_DSCP_TABLE_USED_GET(unit, id)) {
                   if (QOS_INFO(unit)->dscp_hw_idx[id] == hw_idx) {
                        *map_id = id | (_BCM_QOS_MAP_TYPE_DSCP_TABLE 
                                                                << _BCM_QOS_MAP_SHIFT);
                        return BCM_E_NONE;
                   }
              }
         }
         break;
    case _BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE:
         num_map = soc_mem_index_count(unit, EGR_DSCP_TABLEm) / 64;
         for (id = 0; id < num_map; id++) {
              if (_BCM_QOS_EGR_DSCP_TABLE_USED_GET(unit, id)) {
                   if (QOS_INFO(unit)->egr_dscp_hw_idx[id] == hw_idx) {
                        *map_id = id | (_BCM_QOS_MAP_TYPE_EGR_DSCP_TABLE 
                                                                << _BCM_QOS_MAP_SHIFT);
                        return BCM_E_NONE;
                   }
              }
         }
         break;
#ifdef BCM_KATANA_SUPPORT
     case _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE:
         num_map = soc_mem_index_count(unit, ING_QUEUE_OFFSET_MAPPING_TABLEm) /
                                                 _BCM_QOS_MAP_CHUNK_OFFSET_MAP;
         for (id = 0; id < num_map; id++) {
              if (_BCM_QOS_OFFSET_MAP_TABLE_USED_GET(unit, id)) {
                   if (QOS_INFO(unit)->offset_map_hw_idx[id] == hw_idx) {
                        *map_id = 
                           id | (_BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE << 
                                                           _BCM_QOS_MAP_SHIFT);
                        return BCM_E_NONE;
                   }
              }
         }
         break;
#endif
    default:
         return BCM_E_NOT_FOUND;
    }
    return BCM_E_NOT_FOUND;
}

#ifdef INCLUDE_L3
int
_bcm_tr2_vp_dscp_map_mode_set(int unit, bcm_gport_t port, int mode)
{
    source_vp_entry_t svp;
    int rv = BCM_E_NONE;
    int vp = -1;
    /* Deal with different types of gports */
    if (_BCM_QOS_GPORT_IS_VFI_VP_PORT(port)) {
        /* Deal with MiM and MPLS ports */
        if (BCM_GPORT_IS_MIM_PORT(port)) {
            vp = BCM_GPORT_MIM_PORT_ID_GET(port);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeMim)) {
                return BCM_E_BADID;
            }
        } else if BCM_GPORT_IS_VXLAN_PORT(port) {
            vp = BCM_GPORT_VXLAN_PORT_ID_GET(port);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
                QOS_UNLOCK(unit);
                return BCM_E_BADID;
            }
        } else if BCM_GPORT_IS_L2GRE_PORT(port) {
            vp = BCM_GPORT_L2GRE_PORT_ID_GET(port);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
                QOS_UNLOCK(unit);
                return BCM_E_BADID;
            }
        } else {
            vp = BCM_GPORT_MPLS_PORT_ID_GET(port);
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeMpls)) {
                return BCM_E_BADID;
            }
        }
    }
    rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
    mode = mode ? 1 : 0;
    if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_V4f)) {
        soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_V4f, mode);
    }
    if (soc_mem_field_valid(unit,SOURCE_VPm,TRUST_DSCP_V6f)) {
        soc_SOURCE_VPm_field32_set(unit, &svp, TRUST_DSCP_V6f, mode);
    }
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    return rv;
}
#endif

#endif
