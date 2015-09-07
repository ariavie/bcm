/* $Id: field.c 1.470.2.5 Broadcom SDK $
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
 * File:        field.c
 * Purpose:     BCM56624 Field Processor installation functions.
 */
#include <soc/defs.h>
#if defined(BCM_TRX_SUPPORT) && defined(BCM_FIELD_SUPPORT)

#include <soc/mem.h>
#include <soc/drv.h>
#include <bcm/error.h>
#include <bcm/l3.h>
#include <bcm/field.h>
#include <bcm/mirror.h>
#include <bcm/tunnel.h>
#include <bcm_int/esw/field.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/katana.h>
#include <bcm_int/esw/scorpion.h>
#include <bcm_int/esw/mirror.h>
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/flex_ctr.h>
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/policer.h>
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/nat.h>
#endif
#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#endif

typedef struct _field_flex_stat_info_s {
    int   flex_mode;
    int   hw_flags;
    uint8 valid;
    uint8 flex_stat_map;
} _field_flex_stat_info_t;


soc_tcam_partition_type_t _bcm_field_fp_tcam_partitions[] =  { 
    TCAM_PARTITION_ACL_L2C,
    TCAM_PARTITION_ACL_L2,
    TCAM_PARTITION_ACL_IP4C,
    TCAM_PARTITION_ACL_IP4,
    TCAM_PARTITION_ACL_L2IP4,
    TCAM_PARTITION_ACL_IP6C,
    TCAM_PARTITION_ACL_IP6S,
    TCAM_PARTITION_ACL_IP6F,
    TCAM_PARTITION_ACL_L2IP6};

soc_mem_t _bcm_field_ext_data_mask_mems[] = {
    EXT_ACL144_TCAM_L2m, EXT_ACL288_TCAM_L2m,
    EXT_ACL144_TCAM_IPV4m, EXT_ACL288_TCAM_IPV4m, 0,
    EXT_ACL144_TCAM_IPV6m, 0, 0, 0};
soc_mem_t _bcm_field_ext_data_mems[] = {
    0, 0,
    0, 0, EXT_ACL432_TCAM_DATA_L2_IPV4m,
    0, EXT_ACL360_TCAM_DATA_IPV6_SHORTm, EXT_ACL432_TCAM_DATA_IPV6_LONGm,  
    EXT_ACL432_TCAM_DATA_L2_IPV6m};
soc_mem_t _bcm_field_ext_mask_mems[] = {
    0, 0,
    0, 0, EXT_ACL432_TCAM_MASKm,
    0, EXT_ACL360_TCAM_MASKm, EXT_ACL432_TCAM_MASKm, EXT_ACL432_TCAM_MASKm};

soc_mem_t _bcm_field_ext_policy_mems[] = {
    EXT_FP_POLICY_ACL144_L2m, EXT_FP_POLICY_ACL288_L2m, 
    EXT_FP_POLICY_ACL144_IPV4m, EXT_FP_POLICY_ACL288_IPV4m, 
    EXT_FP_POLICY_ACL432_L2_IPV4m, 
    EXT_FP_POLICY_ACL144_IPV6m, EXT_FP_POLICY_ACL360_IPV6_SHORTm, 
    EXT_FP_POLICY_ACL432_IPV6_LONGm, 
    EXT_FP_POLICY_ACL432_L2_IPV6m};

soc_mem_t _bcm_field_ext_counter_mems[] = {
    EXT_FP_CNTR_ACL144_L2m, 
    EXT_FP_CNTR_ACL288_L2m, 
    EXT_FP_CNTR_ACL144_IPV4m, 
    EXT_FP_CNTR_ACL288_IPV4m, 
    EXT_FP_CNTR_ACL432_L2_IPV4m, 
    EXT_FP_CNTR_ACL144_IPV6m, 
    EXT_FP_CNTR_ACL360_IPV6_SHORTm, 
    EXT_FP_CNTR_ACL432_IPV6_LONGm,
    EXT_FP_CNTR_ACL432_L2_IPV6m};
const soc_field_t _bcm_field_trx_slice_pairing_field[] = {
    SLICE1_0_PAIRINGf,
    SLICE3_2_PAIRINGf,
    SLICE5_4_PAIRINGf,
    SLICE7_6_PAIRINGf,
    SLICE9_8_PAIRINGf,
    SLICE11_10_PAIRINGf,
    SLICE13_12_PAIRINGf,
    SLICE15_14_PAIRINGf
};
const soc_field_t _bcm_field_trx_slice_wide_mode_field[16] = {
    SLICE0_DOUBLE_WIDE_MODEf,
    SLICE1_DOUBLE_WIDE_MODEf,
    SLICE2_DOUBLE_WIDE_MODEf,
    SLICE3_DOUBLE_WIDE_MODEf,
    SLICE4_DOUBLE_WIDE_MODEf,
    SLICE5_DOUBLE_WIDE_MODEf,
    SLICE6_DOUBLE_WIDE_MODEf,
    SLICE7_DOUBLE_WIDE_MODEf,
    SLICE8_DOUBLE_WIDE_MODEf,
    SLICE9_DOUBLE_WIDE_MODEf,
    SLICE10_DOUBLE_WIDE_MODEf,
    SLICE11_DOUBLE_WIDE_MODEf,
    SLICE12_DOUBLE_WIDE_MODEf,
    SLICE13_DOUBLE_WIDE_MODEf,
    SLICE14_DOUBLE_WIDE_MODEf,
    SLICE15_DOUBLE_WIDE_MODEf};
static soc_field_t _trx_efp_slice_mode[4][2] =  {
    {SLICE_0_MODEf, SLICE_0_IPV6_KEY_MODEf},
    {SLICE_1_MODEf, SLICE_1_IPV6_KEY_MODEf},
    {SLICE_2_MODEf, SLICE_2_IPV6_KEY_MODEf},
    {SLICE_3_MODEf, SLICE_3_IPV6_KEY_MODEf}};
const soc_field_t _bcm_field_trx_field_sel[16][3] = {
    {SLICE0_F1f, SLICE0_F2f, SLICE0_F3f},
    {SLICE1_F1f, SLICE1_F2f, SLICE1_F3f},
    {SLICE2_F1f, SLICE2_F2f, SLICE2_F3f},
    {SLICE3_F1f, SLICE3_F2f, SLICE3_F3f},
    {SLICE4_F1f, SLICE4_F2f, SLICE4_F3f},
    {SLICE5_F1f, SLICE5_F2f, SLICE5_F3f},
    {SLICE6_F1f, SLICE6_F2f, SLICE6_F3f},
    {SLICE7_F1f, SLICE7_F2f, SLICE7_F3f},
    {SLICE8_F1f, SLICE8_F2f, SLICE8_F3f},
    {SLICE9_F1f, SLICE9_F2f, SLICE9_F3f},
    {SLICE10_F1f, SLICE10_F2f, SLICE10_F3f},
    {SLICE11_F1f, SLICE11_F2f, SLICE11_F3f},
    {SLICE12_F1f, SLICE12_F2f, SLICE12_F3f},
    {SLICE13_F1f, SLICE13_F2f, SLICE13_F3f},
    {SLICE14_F1f, SLICE14_F2f, SLICE14_F3f},
    {SLICE15_F1f, SLICE15_F2f, SLICE15_F3f}};
const soc_field_t _bcm_field_trx_d_type_sel[] =  {
    SLICE0_D_TYPE_SELf, SLICE1_D_TYPE_SELf,
    SLICE2_D_TYPE_SELf, SLICE3_D_TYPE_SELf,
    SLICE4_D_TYPE_SELf, SLICE5_D_TYPE_SELf,
    SLICE6_D_TYPE_SELf, SLICE7_D_TYPE_SELf,
    SLICE8_D_TYPE_SELf, SLICE9_D_TYPE_SELf,
    SLICE10_D_TYPE_SELf, SLICE11_D_TYPE_SELf,
    SLICE12_D_TYPE_SELf, SLICE13_D_TYPE_SELf,
    SLICE14_D_TYPE_SELf, SLICE15_D_TYPE_SELf};
const soc_field_t _bcm_field_trx_s_type_sel[] =  {
    SLICE0_S_TYPE_SELf, SLICE1_S_TYPE_SELf,
    SLICE2_S_TYPE_SELf, SLICE3_S_TYPE_SELf,
    SLICE4_S_TYPE_SELf, SLICE5_S_TYPE_SELf,
    SLICE6_S_TYPE_SELf, SLICE7_S_TYPE_SELf,
    SLICE8_S_TYPE_SELf, SLICE9_S_TYPE_SELf,
    SLICE10_S_TYPE_SELf, SLICE11_S_TYPE_SELf,
    SLICE12_S_TYPE_SELf, SLICE13_S_TYPE_SELf,
    SLICE14_S_TYPE_SELf, SLICE15_S_TYPE_SELf};
const soc_field_t _bcm_field_trx_dw_f4_sel[] =  {
    SLICE_0_F4f, SLICE_1_F4f,
    SLICE_2_F4f, SLICE_3_F4f,
    SLICE_4_F4f, SLICE_5_F4f,
    SLICE_6_F4f, SLICE_7_F4f,
    SLICE_8_F4f, SLICE_9_F4f,
    SLICE_10_F4f, SLICE_11_F4f,
    SLICE_12_F4f, SLICE_13_F4f,
    SLICE_14_F4f, SLICE_15_F4f};
static soc_field_t _trx_ifp_double_wide_key[] = {
    SLICE0_DOUBLE_WIDE_KEY_SELECTf,
    SLICE1_DOUBLE_WIDE_KEY_SELECTf,
    SLICE2_DOUBLE_WIDE_KEY_SELECTf,
    SLICE3_DOUBLE_WIDE_KEY_SELECTf,
    SLICE4_DOUBLE_WIDE_KEY_SELECTf,
    SLICE5_DOUBLE_WIDE_KEY_SELECTf,
    SLICE6_DOUBLE_WIDE_KEY_SELECTf,
    SLICE7_DOUBLE_WIDE_KEY_SELECTf,
    SLICE8_DOUBLE_WIDE_KEY_SELECTf,
    SLICE9_DOUBLE_WIDE_KEY_SELECTf,
    SLICE10_DOUBLE_WIDE_KEY_SELECTf,
    SLICE11_DOUBLE_WIDE_KEY_SELECTf,
    SLICE12_DOUBLE_WIDE_KEY_SELECTf,
    SLICE13_DOUBLE_WIDE_KEY_SELECTf,
    SLICE14_DOUBLE_WIDE_KEY_SELECTf,
    SLICE15_DOUBLE_WIDE_KEY_SELECTf};
soc_field_t _trx_src_class_id_sel[] = {
    SLICE_0_SRC_CLASS_ID_SELf,
    SLICE_1_SRC_CLASS_ID_SELf,
    SLICE_2_SRC_CLASS_ID_SELf,
    SLICE_3_SRC_CLASS_ID_SELf,
    SLICE_4_SRC_CLASS_ID_SELf,
    SLICE_5_SRC_CLASS_ID_SELf,
    SLICE_6_SRC_CLASS_ID_SELf,
    SLICE_7_SRC_CLASS_ID_SELf,
    SLICE_8_SRC_CLASS_ID_SELf,
    SLICE_9_SRC_CLASS_ID_SELf,
    SLICE_10_SRC_CLASS_ID_SELf,
    SLICE_11_SRC_CLASS_ID_SELf,
    SLICE_12_SRC_CLASS_ID_SELf,
    SLICE_13_SRC_CLASS_ID_SELf,
    SLICE_14_SRC_CLASS_ID_SELf,
    SLICE_15_SRC_CLASS_ID_SELf};
soc_field_t _trx_dst_class_id_sel[] = {
    SLICE_0_DST_CLASS_ID_SELf,
    SLICE_1_DST_CLASS_ID_SELf,
    SLICE_2_DST_CLASS_ID_SELf,
    SLICE_3_DST_CLASS_ID_SELf,
    SLICE_4_DST_CLASS_ID_SELf,
    SLICE_5_DST_CLASS_ID_SELf,
    SLICE_6_DST_CLASS_ID_SELf,
    SLICE_7_DST_CLASS_ID_SELf,
    SLICE_8_DST_CLASS_ID_SELf,
    SLICE_9_DST_CLASS_ID_SELf,
    SLICE_10_DST_CLASS_ID_SELf,
    SLICE_11_DST_CLASS_ID_SELf,
    SLICE_12_DST_CLASS_ID_SELf,
    SLICE_13_DST_CLASS_ID_SELf,
    SLICE_14_DST_CLASS_ID_SELf,
    SLICE_15_DST_CLASS_ID_SELf};
soc_field_t _trx_interface_class_id_sel[] = {
    SLICE_0_INTERFACE_CLASS_ID_SELf,
    SLICE_1_INTERFACE_CLASS_ID_SELf,
    SLICE_2_INTERFACE_CLASS_ID_SELf,
    SLICE_3_INTERFACE_CLASS_ID_SELf,
    SLICE_4_INTERFACE_CLASS_ID_SELf,
    SLICE_5_INTERFACE_CLASS_ID_SELf,
    SLICE_6_INTERFACE_CLASS_ID_SELf,
    SLICE_7_INTERFACE_CLASS_ID_SELf,
    SLICE_8_INTERFACE_CLASS_ID_SELf,
    SLICE_9_INTERFACE_CLASS_ID_SELf,
    SLICE_10_INTERFACE_CLASS_ID_SELf,
    SLICE_11_INTERFACE_CLASS_ID_SELf,
    SLICE_12_INTERFACE_CLASS_ID_SELf,
    SLICE_13_INTERFACE_CLASS_ID_SELf,
    SLICE_14_INTERFACE_CLASS_ID_SELf,
    SLICE_15_INTERFACE_CLASS_ID_SELf};

const soc_field_t _bcm_field_trx_vfp_double_wide_sel[]= {
    SLICE_0_DOUBLE_WIDE_KEY_SELECTf,
    SLICE_1_DOUBLE_WIDE_KEY_SELECTf,
    SLICE_2_DOUBLE_WIDE_KEY_SELECTf,
    SLICE_3_DOUBLE_WIDE_KEY_SELECTf
};
const soc_field_t _bcm_field_trx_vfp_field_sel[][2] = {
    { SLICE_0_F2f, SLICE_0_F3f },
    { SLICE_1_F2f, SLICE_1_F3f },
    { SLICE_2_F2f, SLICE_2_F3f },
    { SLICE_3_F2f, SLICE_3_F3f }
};
const soc_field_t _bcm_field_trx_vfp_ip_header_sel[]= {
    SLICE_0_IP_FIELD_SELECTf,
    SLICE_1_IP_FIELD_SELECTf,
    SLICE_2_IP_FIELD_SELECTf,
    SLICE_3_IP_FIELD_SELECTf
};
const soc_field_t _trx_vfp_src_type_sel[]= {
    SLICE_0_SOURCE_TYPE_SELf, SLICE_1_SOURCE_TYPE_SELf,
    SLICE_2_SOURCE_TYPE_SELf, SLICE_3_SOURCE_TYPE_SELf};

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
/* Static variables. */
static soc_field_t _trx2_offset_field[2][8] = {
    {
        UDF1_OFFSET0f, UDF1_OFFSET1f,
        UDF1_OFFSET2f, UDF1_OFFSET3f,
        UDF1_OFFSET4f, UDF1_OFFSET5f,
        UDF1_OFFSET6f, UDF1_OFFSET7f
    },
    {
        UDF2_OFFSET0f, UDF2_OFFSET1f,
        UDF2_OFFSET2f, UDF2_OFFSET3f,
        UDF2_OFFSET4f, UDF2_OFFSET5f,
        UDF2_OFFSET6f, UDF2_OFFSET7f
    }
};
static soc_field_t _trx2_base_field[2][8] = {
    {
        UDF1_BASE_OFFSET_0f, UDF1_BASE_OFFSET_1f,
        UDF1_BASE_OFFSET_2f, UDF1_BASE_OFFSET_3f,
        UDF1_BASE_OFFSET_4f, UDF1_BASE_OFFSET_5f,
        UDF1_BASE_OFFSET_6f, UDF1_BASE_OFFSET_7f
    },
    {
        UDF2_BASE_OFFSET_0f, UDF2_BASE_OFFSET_1f,
        UDF2_BASE_OFFSET_2f, UDF2_BASE_OFFSET_3f,
        UDF2_BASE_OFFSET_4f, UDF2_BASE_OFFSET_5f,
        UDF2_BASE_OFFSET_6f, UDF2_BASE_OFFSET_7f
    }
};
#endif /* BCM_TRIUMPH2_SUPPORT || BCM_TRIDENT_SUPPORT */

const soc_field_t _bcm_trx2_aux_tag_1_field[] = {
    SLICE0_AUX_TAG_1_SELf,
    SLICE1_AUX_TAG_1_SELf,
    SLICE2_AUX_TAG_1_SELf,
    SLICE3_AUX_TAG_1_SELf,
    SLICE4_AUX_TAG_1_SELf,
    SLICE5_AUX_TAG_1_SELf,
    SLICE6_AUX_TAG_1_SELf,
    SLICE7_AUX_TAG_1_SELf,
    SLICE8_AUX_TAG_1_SELf,
    SLICE9_AUX_TAG_1_SELf,
    SLICE10_AUX_TAG_1_SELf,
    SLICE11_AUX_TAG_1_SELf,
    SLICE12_AUX_TAG_1_SELf,
    SLICE13_AUX_TAG_1_SELf,
    SLICE14_AUX_TAG_1_SELf,
    SLICE15_AUX_TAG_1_SELf
};

const soc_field_t _bcm_trx2_aux_tag_2_field[] = {
    SLICE0_AUX_TAG_2_SELf,
    SLICE1_AUX_TAG_2_SELf,
    SLICE2_AUX_TAG_2_SELf,
    SLICE3_AUX_TAG_2_SELf,
    SLICE4_AUX_TAG_2_SELf,
    SLICE5_AUX_TAG_2_SELf,
    SLICE6_AUX_TAG_2_SELf,
    SLICE7_AUX_TAG_2_SELf,
    SLICE8_AUX_TAG_2_SELf,
    SLICE9_AUX_TAG_2_SELf,
    SLICE10_AUX_TAG_2_SELf,
    SLICE11_AUX_TAG_2_SELf,
    SLICE12_AUX_TAG_2_SELf,
    SLICE13_AUX_TAG_2_SELf,
    SLICE14_AUX_TAG_2_SELf,
    SLICE15_AUX_TAG_2_SELf
};

const soc_field_t _bcm_trx2_vrf_force_forwarding_enable_field[] = {
    SLICE_0_ENABLEf,
    SLICE_1_ENABLEf,
    SLICE_2_ENABLEf,
    SLICE_3_ENABLEf,
    SLICE_4_ENABLEf,
    SLICE_5_ENABLEf,
    SLICE_6_ENABLEf,
    SLICE_7_ENABLEf,
    SLICE_8_ENABLEf,
    SLICE_9_ENABLEf,
    SLICE_10_ENABLEf,
    SLICE_11_ENABLEf,
    SLICE_12_ENABLEf,
    SLICE_13_ENABLEf,
    SLICE_14_ENABLEf,
    SLICE_15_ENABLEf
};

#ifdef BCM_KATANA_SUPPORT
#define _BCM_QOS_MAP_MASK                            0x1fff
#define _BCM_QOS_MAP_OFFSET_MASK                     0x3ff
#define _BCM_QOS_MAP_TYPE_SHIFT                        10
#define _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE   5
#define _BCM_QOS_MAP_ING_QUEUE_OFFSET_MAX              7
#endif
/*
 * Function:
 *     _bcm_field_trx_egress_selcode_get
 * Purpose:
 *     Finds a select encodings that will satisfy the
 *     requested qualifier set (Qset).
 * Parameters:
 *     unit      - (IN) BCM unit number.
 *     stage_fc  - (IN) Stage Field control structure.
 *     qset_req  - (IN) Client qualifier set.
 *     fg        - (IN/OUT)Select code information filled into the group.
 *
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_egress_selcode_get(int unit, _field_stage_t *stage_fc,
                               bcm_field_qset_t *qset_req,
                               _field_group_t *fg)
{
    int             rv;          /* Operation return status. */

    /* Input parameters check. */
    if ((NULL == fg) || (NULL == qset_req) || (NULL == stage_fc)) {
        return (BCM_E_PARAM);
    }

    if  (fg->flags & _FP_GROUP_SPAN_SINGLE_SLICE) {
        /* Attempt _BCM_FIELD_EFP_KEY4  (L2 key). */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY4, 0, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }

        /* Attempt _BCM_FIELD_EFP_KEY5  (L3 any key). */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY5, 0, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }

        /* Attempt _BCM_FIELD_EFP_KEY1  (IPv4 key). */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY1, 0, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }

        /* Attempt _BCM_FIELD_EFP_KEY2  (IPv6 key). */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY2, 0, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }
    }  else  {
        /* L2 + L3 double wide predefined key. */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY5,
                                           _BCM_FIELD_EFP_KEY4, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }

        /* L2 + L3 v4 double wide predefined key. */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY1,
                                           _BCM_FIELD_EFP_KEY4, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }

        /* L2 + L3 v6  double wide predefined key. */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY2,
                                           _BCM_FIELD_EFP_KEY4, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }

        /* IPv6 double wide predefined key. */
        rv = _bcm_field_egress_key_attempt(unit, stage_fc, qset_req,
                                           _BCM_FIELD_EFP_KEY3,
                                           _BCM_FIELD_EFP_KEY2, fg);
        if (BCM_SUCCESS(rv) || (BCM_FAILURE(rv) && (rv != BCM_E_RESOURCE))) {
            return rv;
        }
    }
    return (BCM_E_RESOURCE);
}

/*
 * Function:
 *     _bcm_field_trx_egress_mode_set
 *
 * Purpose:
 *     Helper function to _bcm_field_fb_mode_install that sets the mode of a
 *     slice in a register value that is to be used for FP_SLICE_CONFIGr.
 *
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     slice_numb - (IN) Slice number to set mode for.
 *     fg         - (IN) Installed group structure.
 *     flags      - (IN) New group/slice mode.
 *
 * Returns:
 *     BCM_E_XXX
 */

int
_bcm_field_trx_egress_mode_set(int unit, uint8 slice_numb,
                               _field_group_t *fg, uint8 flags)
{
    uint32 mode_val[2];
    /* Input parameters check. */
    if ((NULL == fg) || (slice_numb >= COUNTOF(_trx_efp_slice_mode))) {
        return (BCM_E_PARAM);
    }

    mode_val[1]  = _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_SIP6;

    if (flags & _FP_GROUP_SPAN_DOUBLE_SLICE) {
        /* IP + L2 Double wide key. */
        if ((_BCM_FIELD_EFP_KEY5 == fg->sel_codes[0].fpf3) && \
            (_BCM_FIELD_EFP_KEY4 == fg->sel_codes[1].fpf3)) {
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY;
        }

        /* DstIP6 + SrcIp6 or IPv4 + L2 Double wide key. */
        if (((_BCM_FIELD_EFP_KEY3 == fg->sel_codes[0].fpf3) && \
            (_BCM_FIELD_EFP_KEY2 == fg->sel_codes[1].fpf3)) ||
            BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifyIp4)) {
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE;
        }

        /* IPv6 + L2 Double wide key. */
        else if ((_BCM_FIELD_EFP_KEY2 == fg->sel_codes[0].fpf3) && \
                 (_BCM_FIELD_EFP_KEY4 == fg->sel_codes[1].fpf3)) {
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6;
            if (_FP_SELCODE_DONT_CARE != fg->sel_codes[0].ip6_addr_sel) {
                mode_val[1] =fg->sel_codes[0].ip6_addr_sel;
            }
        } else {
            /* IPv4/Don't care + L2 Double wide key. */
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY;
        }

        BCM_IF_ERROR_RETURN
            (soc_reg_fields32_modify(unit, EFP_SLICE_CONTROLr, REG_PORT_ANY, 2,
                                     _trx_efp_slice_mode[slice_numb], mode_val));
    } else {
        if (_BCM_FIELD_EFP_KEY4 == fg->sel_codes[0].fpf3) {
            /* L2 - Slice mode. */
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L2;

        } else  if (_BCM_FIELD_EFP_KEY5 == fg->sel_codes[0].fpf3) {
            /* L3 - Any single wide key. */
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3_ANY;

        } else  if ((_BCM_FIELD_EFP_KEY1 == fg->sel_codes[0].fpf3)  &&
                    BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifyIp4)) {
            /* L3 - IPv4 single wide key. */
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3;

        } else if ((_BCM_FIELD_EFP_KEY2 == fg->sel_codes[0].fpf3)) {
            /* L3 - IPv6 single wide key. */
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3;
            if (_FP_SELCODE_DONT_CARE != fg->sel_codes[0].ip6_addr_sel) {
                mode_val[1] =fg->sel_codes[0].ip6_addr_sel;
            }
        } else {
            /* L3 common key. */
            mode_val[0] = _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3_ANY;
        }

        BCM_IF_ERROR_RETURN
            (soc_reg_fields32_modify(unit, EFP_SLICE_CONTROLr, REG_PORT_ANY, 2,
                                     _trx_efp_slice_mode[slice_numb], mode_val));

    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_fb_egress_slice_clear
 *
 * Purpose:
 *     Reset slice configuraton on group deletion event.
 *
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     slice_numb - (IN) Slice number to set mode for.
 *
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_egress_slice_clear(int unit, uint8 slice_numb)
{
    uint32 mode_val[2] =  {0, 0};
    int rv;

    /* Input parameters check. */
    if (slice_numb >= COUNTOF(_trx_efp_slice_mode)) {
        return (BCM_E_PARAM);
    }

    rv = soc_reg_fields32_modify(unit, EFP_SLICE_CONTROLr, REG_PORT_ANY,
                             2, _trx_efp_slice_mode[slice_numb], mode_val);

    return (rv);
}

/*
 * Function:
 *     _field_trx_mode_set
 *
 * Purpose:
 *    Auxiliary routine used to set group pairing mode.
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     slice_numb - (IN) Slice number to set mode for.
 *     fg         - (IN) Installed group structure.
 *     flags      - (IN) New group/slice mode.
 *
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_mode_set(int unit, uint8 slice_numb, _field_group_t *fg, uint8 flags)
{
    int rv;     /* Operation return status. */

    /* Input parameter check. */
    if (NULL == fg) {
        return (BCM_E_PARAM);
    }

    switch (fg->stage_id) {
      case _BCM_FIELD_STAGE_INGRESS:
          rv = BCM_E_NONE; /* Mode and select codes programmed together. */
          break;
      case _BCM_FIELD_STAGE_LOOKUP:
          rv  = _bcm_field_fb_lookup_mode_set(unit, slice_numb, fg, flags);
          break;
      case _BCM_FIELD_STAGE_EGRESS:
          rv = _bcm_field_trx_egress_mode_set(unit, slice_numb, fg, flags);
          break;
#if defined (BCM_TRIUMPH_SUPPORT)
      case _BCM_FIELD_STAGE_EXTERNAL:
          rv = _bcm_field_tr_external_mode_set(unit, slice_numb, fg, flags);
          break;
#endif /* BCM_TRIUMPH_SUPPORT */
      default:
          rv = BCM_E_PARAM;
    }
    return (rv);
}


/*
 * Function:
 *      _bcm_trx_range_checker_selcodes_update
 *
 * Purpose:
 *     Update group select codes based on range checker id
 *     used in field group entry.
 * Parameters:
 *     unit          - (IN) BCM device number.
 *     f_ent         - (IN) Field entry.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_trx_range_checker_selcodes_update(int unit, _field_entry_t *f_ent)

{
    uint8            slice_number;          /* Slices iterator.         */
    uint8            entry_part;            /* Wide entry part number.  */
    uint32           buf[SOC_MAX_MEM_FIELD_WORDS];/* HW entry buffer.   */
    _field_sel_t     *sel;                  /* Group field selectors.   */
    _field_group_t   *fg;                   /* Field group structure.   */
    _field_slice_t   *fs;                   /* Field slice structure.   */
    int              rv;                    /* Operation return status. */

    /* Input parameters check. */
    if (NULL == f_ent)  {
        return (BCM_E_PARAM);
    }

    fg = f_ent->group;

    /* Get tcam part. */
    rv = _bcm_field_entry_flags_to_tcam_part (f_ent->flags, fg->flags,
                                              &entry_part);
    BCM_IF_ERROR_RETURN(rv);

    /* Per slice selectors installed in primary portion only. */
    if ((entry_part > 0) && fg->sel_codes[entry_part].intraslice) {
        entry_part--;
    }

    /* Get slice number for the entry part. */
    rv = _bcm_field_tcam_part_to_slice_number(entry_part, fg->flags,
                                              &slice_number);
    BCM_IF_ERROR_RETURN(rv);

    /* Update selector value in all group slices. */
    fs = fg->slices + slice_number;

    sel = fg->sel_codes + entry_part;
    if (3 == sel->intf_class_sel) {
        return (BCM_E_NONE);
    } else if ((_FP_SELCODE_DONT_CARE != fs->intf_class_sel)
                && (3 != fs->intf_class_sel)) {
        return (BCM_E_RESOURCE);
    } else {
        sel->intf_class_sel = 3;  /* Range checker selector. */
    }

    /* Update FP_SLICE_KEY_CONTROL memory entry. */
    sal_memset(buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));
    rv = soc_mem_read(unit, FP_SLICE_KEY_CONTROLm, MEM_BLOCK_ANY, 0, buf);
    BCM_IF_ERROR_RETURN(rv);

    while (fs != NULL) {
        /* Set interface class select field. */
        soc_mem_field32_set(unit, FP_SLICE_KEY_CONTROLm, buf,
                            _trx_interface_class_id_sel[fs->slice_number],
                            sel->intf_class_sel);

        fs->intf_class_sel = sel->intf_class_sel;
        fs = fs->next;
    }
    rv = soc_mem_write(unit, FP_SLICE_KEY_CONTROLm, MEM_BLOCK_ALL, 0, buf);
    return (rv);
}

/*
 * Function:
 *     _bcm_field_trx_ingress_pfs_bmap_get
 *
 * Purpose:
 *     Fill in set of (PFS) indexes applicable for the specific group.
 *
 * Parameters:
 *     unit          - (IN) BCM device number.
 *     fg            - (IN) Field group.
 *     pbmp          - (IN) Group  active port bit map.
 *     selcode_index - (IN) Index into select codes array.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_ingress_pfs_bmap_get(int unit, _field_group_t *fg,
                                    bcm_pbmp_t *pbmp, int selcode_index,
                                    SHR_BITDCL *pfs_bmp)
{
    bcm_port_t         idx;               /* Device pfs indexes iterator.*/
    int                rv;                /* Operation return status.    */
    _field_sel_t       *sel;              /* Group field selectors.      */
    bcm_port_config_t  port_config;       /* Device port configuration.  */

    /* Input parameters check. */
    if ((NULL == fg) || (NULL == pbmp) || (NULL == pfs_bmp)) {
        return (BCM_E_PARAM);
    }

    /* Get group select codes. */
    sel = &fg->sel_codes[selcode_index];

    /* Read device port bitmaps. */
    rv = bcm_esw_port_config_get(unit, &port_config);
    BCM_IF_ERROR_RETURN(rv);
    if (fg->flags & _FP_GROUP_WLAN) {
        SHR_BITSET(pfs_bmp, soc_mem_index_max(unit, FP_PORT_FIELD_SELm));
    } else if ((fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) &&
        ((selcode_index == 1) || (selcode_index == 3)) &&
        (sel->fpf2 != _FP_SELCODE_DONT_CARE)) {
        sal_memcpy(pfs_bmp, &port_config.all, sizeof(bcm_pbmp_t));
    } else {
        sal_memcpy(pfs_bmp, pbmp, sizeof(bcm_pbmp_t));
    }

#if defined(BCM_ENDURO_SUPPORT)
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
        /* Have to set up CPU port for Enduro & Hurricane. */
        SHR_BITSET(pfs_bmp, 34);
    }
#endif /* BCM_ENDURO_SUPPORT */

    /* Following devices have static FP selector mapping when XE port
     * operating on HG mode. 
     * Rest devices have FPF_sel_index in port table.
     */
#if defined(BCM_TRX_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit) ||
        SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)
        || SOC_IS_SC_CQ(unit)) {
        for (idx = 0; idx < BCM_PBMP_PORT_MAX; idx++) {
            if (SOC_PORT_VALID(unit, idx) && SHR_BITGET(pfs_bmp, idx)) {
#if defined(BCM_SCORPION_SUPPORT)
                if (SOC_IS_SC_CQ(unit)) {
                    SHR_BITSET(pfs_bmp, idx + 29);
                    continue;
                }
#endif /* BCM_SCORPION_SUPPORT */
                if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
                    switch (idx) {
                        case 26:
                            SHR_BITSET(pfs_bmp, 30);
                            break;
                        case 27:
                            SHR_BITSET(pfs_bmp, 31);
                            break;
                        case 28:
                            SHR_BITSET(pfs_bmp, 32);
                            break;
                        case 29:
                            SHR_BITSET(pfs_bmp, 33);
                            break;
                        default:
                            ;
                    }
                    continue;
                }
                if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
                    switch (idx) {
                        case 2:
                            SHR_BITSET(pfs_bmp, 54);
                            break;
                        case 14:
                            SHR_BITSET(pfs_bmp, 55);
                            break;
                        case 26:
                            SHR_BITSET(pfs_bmp, 56);
                            break;
                        case 27:
                            SHR_BITSET(pfs_bmp, 57);
                            break;
                        case 28:
                            SHR_BITSET(pfs_bmp, 58);
                            break;
                        case 29:
                            SHR_BITSET(pfs_bmp, 59);
                            break;
                        case 30:
                            SHR_BITSET(pfs_bmp, 60);
                            break;
                        case 31:
                            SHR_BITSET(pfs_bmp, 61);
                            break;
                        case 0:
                            SHR_BITSET(pfs_bmp, 62);
                            break;
                        default:
                            ;
                    }
                }
            }
        }
    }
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    /* FPF programming for loopback port.*/
    if (soc_feature(unit, soc_feature_lport_tab_profile)) {
        SHR_BITSET(pfs_bmp,
                   soc_mem_index_max(unit, FP_PORT_FIELD_SELm) - 1);
    }
#endif /* BCM_TRIUMPH_SUPPORT */
    return (rv);
}
/*
 * Function:
 *     _field_trx_ingress_selcodes_install
 *
 * Purpose:
 *     Writes the field select codes (ie. FPFx).
 *
 * Parameters:
 *     unit          - (IN) BCM device number.
 *     fg            - (IN) Field group.
 *     slice_numb    - (IN) Slice number group installed in.
 *     pbmp          - (IN) Group  active port bit map.
 *     selcode_index - (IN) Index into select codes array.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_ingress_selcodes_install(int unit, _field_group_t *fg,
                                    uint8 slice_num, bcm_pbmp_t *pbmp,
                                    int selcode_index)
{
    _field_stage_t   *stage_fc;
    bcm_port_t       idx;                   /* Device pfs indexes iterator.*/
    uint32           buf[SOC_MAX_MEM_FIELD_WORDS];/* HW entry buffer.      */
    SHR_BITDCL       *pfs_bmp;              /* PFS bitmap.                 */
    int              pfs_idx_count;         /* PFS index count.            */
    _field_sel_t     *sel;                  /* Group field selectors.      */
    soc_field_t      fld;                   /* Port field select field.    */
    uint32           value;                 /* Per slice selector.         */
    int              rv;                    /* Operation return status.    */
    int              ingress_entity = _FP_SELCODE_DONT_CARE;

    /* Input parameters check. */

    if (NULL == fg) {
        return (BCM_E_PARAM);
    }

    rv = _field_stage_control_get(unit, fg->stage_id, &stage_fc);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    if (slice_num >= stage_fc->tcam_slices) {
        return (BCM_E_PARAM);
    }

    sel = &fg->sel_codes[selcode_index];

    /* Get port field select table size and allocated bitmap of indexes
     * applicable to the group.
     */
    pfs_bmp = NULL;
    pfs_idx_count = soc_mem_index_count(unit, FP_PORT_FIELD_SELm);
    _FP_XGS3_ALLOC(pfs_bmp,
                   MAX(SHR_BITALLOCSIZE(pfs_idx_count), sizeof(bcm_pbmp_t)),
                   "PFS bmp");
    if (NULL == pfs_bmp) {
        return (BCM_E_MEMORY);
    }

    /* Populate pfs indexes applicable for the group. */
    rv = _bcm_field_trx_ingress_pfs_bmap_get(unit, fg, pbmp,
                                             selcode_index, pfs_bmp);
    if (BCM_FAILURE(rv)) {
        sal_free(pfs_bmp);
        return (rv);
    }

    if ((fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) &&
        ((selcode_index == 1) || (selcode_index == 3))) {

        /* Write appropriate values in FP_DOUBLE_WIDE_F4_SELECTr and */
        if ((sel->fpf4 != _FP_SELCODE_DONT_CARE) &&
            SOC_REG_FIELD_VALID(unit, FP_DOUBLE_WIDE_F4_SELECTr,
                                _bcm_field_trx_dw_f4_sel[slice_num])) {
            rv = soc_reg_field32_modify(unit, FP_DOUBLE_WIDE_F4_SELECTr,
                                        REG_PORT_ANY,
                                        _bcm_field_trx_dw_f4_sel[slice_num],
                                        sel->fpf4);
            if (BCM_FAILURE(rv)) {
                sal_free(pfs_bmp);
                return (rv);
            }
        }

        if ((sel->fpf2 != _FP_SELCODE_DONT_CARE) &&
            SOC_MEM_FIELD_VALID(unit, FP_PORT_FIELD_SELm,
                                _trx_ifp_double_wide_key[slice_num])) {
            /* Do the same thing for each entry in FP_PORT_FIELD_SEL table */
            for (idx = 0; idx < pfs_idx_count; idx++) {
                if (0 == SHR_BITGET(pfs_bmp, idx)) {
                    continue;
                }
                sal_memset(buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));
                rv = soc_mem_read(unit, FP_PORT_FIELD_SELm, MEM_BLOCK_ANY,
                                  idx, buf);
                if (BCM_FAILURE(rv)) {
                    sal_free(pfs_bmp);
                    return (rv);
                }
                soc_mem_field32_set(unit, FP_PORT_FIELD_SELm, buf,
                                    _trx_ifp_double_wide_key[slice_num],
                                    sel->fpf2);




                rv = soc_mem_write(unit, FP_PORT_FIELD_SELm, MEM_BLOCK_ALL,
                                   idx, buf);
                if (BCM_FAILURE(rv)) {
                    sal_free(pfs_bmp);
                    return (rv);
                }
            }
        }
    } else {
        /* Iterate over all ports */
        for (idx = 0; idx < pfs_idx_count; idx++) {
            if (0 == SHR_BITGET(pfs_bmp, idx)) {
                continue;
            }
            sal_memset(buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));
            /* Read Port's current entry in FP_PORT_FIELD_SEL table */
            rv = soc_mem_read(unit, FP_PORT_FIELD_SELm, MEM_BLOCK_ANY, idx, buf);
            if (BCM_FAILURE(rv)) {
                sal_free(pfs_bmp);
                return (rv);
            }
            /* Set slice mode. */
            fld = _bcm_field_trx_slice_wide_mode_field[slice_num];
            if (SOC_MEM_FIELD_VALID(unit, FP_PORT_FIELD_SELm, fld)) {
                value = (fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) ? 1 : 0;
                soc_FP_PORT_FIELD_SELm_field32_set(unit, buf, fld, value);
            }

            /* Set inter slice pair mode.  */
            value = (fg->flags & _FP_GROUP_SPAN_DOUBLE_SLICE) ? 1 : 0;
            fld = _bcm_field_trx_slice_pairing_field[slice_num / 2];
            soc_FP_PORT_FIELD_SELm_field32_set(unit, buf, fld, value);

            /* modify 0-3 fields depending on state of SELCODE_INVALID */
            if (sel->fpf1 != _FP_SELCODE_DONT_CARE) {
                soc_mem_field32_set(unit,
                                    FP_PORT_FIELD_SELm, buf,
                                    _bcm_field_trx_field_sel[slice_num][0],
                                    sel->fpf1);
            }

            if (sel->fpf2 != _FP_SELCODE_DONT_CARE) {
                soc_mem_field32_set(unit,
                                    FP_PORT_FIELD_SELm, buf,
                                    _bcm_field_trx_field_sel[slice_num][1],
                                    sel->fpf2);
            }
            if (sel->fpf3 != _FP_SELCODE_DONT_CARE) {
                soc_mem_field32_set(unit,
                                    FP_PORT_FIELD_SELm, buf,
                                    _bcm_field_trx_field_sel[slice_num][2],
                                    sel->fpf3);
            }

            /* Set destination forwarding type selection. */
            if (sel->dst_fwd_entity_sel != _FP_SELCODE_DONT_CARE) {
                switch (sel->dst_fwd_entity_sel) {
                case _bcmFieldFwdEntityMultipath:
                    value = 7;
                    break;
                case _bcmFieldFwdEntityMplsGport:
                case _bcmFieldFwdEntityMimGport:
                case _bcmFieldFwdEntityWlanGport:
                    value = 3;
                    break;
                case _bcmFieldFwdEntityMulticastGroup:
                    value = 2;
                    break;
                case _bcmFieldFwdEntityL3Egress:
                    value = 1;
                    break;
                default:
                    value = 0;
                    break;
                }
                if (SOC_MEM_FIELD_VALID(unit,
                        FP_PORT_FIELD_SELm,
                        _bcm_field_trx_d_type_sel[slice_num]
                        )
                    ) {
                    soc_mem_field32_set(unit,
                        FP_PORT_FIELD_SELm,
                        buf,
                        _bcm_field_trx_d_type_sel[slice_num],
                        value
                        );
                }
            }

            /* Set source entity type selection. */
            if (sel->src_entity_sel != _FP_SELCODE_DONT_CARE) {
                ingress_entity = sel->src_entity_sel;
            } else if (sel->ingress_entity_sel != _FP_SELCODE_DONT_CARE) {
                ingress_entity = sel->ingress_entity_sel;
            }

            if (ingress_entity != _FP_SELCODE_DONT_CARE) {
                switch (ingress_entity) {
                  case _bcmFieldFwdEntityMplsGport:
                  case _bcmFieldFwdEntityMimGport:
                  case _bcmFieldFwdEntityWlanGport:
                      value = 3;
                      break;
                  case _bcmFieldFwdEntityModPortGport:
                      value = 2;
                      break;
                  case _bcmFieldFwdEntityGlp:
                      value = 1;
                      break;
                  case _bcmFieldFwdEntityCommonGport:
                      /* Value 0 supports SVP (or) SGLP.
                       * CommonGport can be VP or GLP.
                       */
                  default:
                      value = 0;
                      break;
                }
                if (SOC_MEM_FIELD_VALID(unit,
                         FP_PORT_FIELD_SELm,
                        _bcm_field_trx_s_type_sel[slice_num]
                        )
                    ) {
                    soc_mem_field32_set
                        (unit,
                         FP_PORT_FIELD_SELm,
                         buf,
                         _bcm_field_trx_s_type_sel[slice_num],
                         value
                         );
                }
            }

            /* Write each port's new entry */
            rv = soc_mem_write(unit, FP_PORT_FIELD_SELm,
                               MEM_BLOCK_ALL, idx, buf);
            if (BCM_FAILURE(rv)) {
                sal_free(pfs_bmp);
                return (rv);
            }
        }
    }
    sal_free(pfs_bmp);

    /* Set VRF Force Forwarding Enable Field */
    if (sel->fwd_field_sel != _FP_SELCODE_DONT_CARE) {
        if (_bcmFieldFwdFieldVrf == sel->fwd_field_sel) {
            rv = soc_reg_field32_modify(unit, FP_FORCE_FORWARDING_FIELDr,
                    REG_PORT_ANY,
                    _bcm_trx2_vrf_force_forwarding_enable_field[slice_num], 1);
            BCM_IF_ERROR_RETURN(rv);
        }
    }

    /* Update FP_SLICE_KEY_CONTROL memory entry. */
    sal_memset(buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));
    rv = soc_mem_read(unit, FP_SLICE_KEY_CONTROLm, MEM_BLOCK_ANY, 0, buf);
    BCM_IF_ERROR_RETURN(rv);


    /* Set source class select field. */
    if (sel->src_class_sel != _FP_SELCODE_DONT_CARE) {
        soc_mem_field32_set(unit, FP_SLICE_KEY_CONTROLm, buf,
                            _trx_src_class_id_sel[slice_num],
                            sel->src_class_sel);
    }
    /* Set destination class select field. */
    if (sel->dst_class_sel != _FP_SELCODE_DONT_CARE) {
        soc_mem_field32_set(unit, FP_SLICE_KEY_CONTROLm, buf,
                            _trx_dst_class_id_sel[slice_num],
                            sel->dst_class_sel);
    }

    /* Set interface class select field. */
    if (sel->intf_class_sel != _FP_SELCODE_DONT_CARE) {
        soc_mem_field32_set(unit, FP_SLICE_KEY_CONTROLm, buf,
                            _trx_interface_class_id_sel[slice_num],
                            sel->intf_class_sel);
    }

    if (sel->aux_tag_1_sel != _FP_SELCODE_DONT_CARE) {
        switch (sel->aux_tag_1_sel) {
        case _bcmFieldAuxTagAny:
            value = 0;
            break;
        case _bcmFieldAuxTagVn:
            value = 1;
            break;
        case _bcmFieldAuxTagCn:
            value = 2;
            break;
        case _bcmFieldAuxTagFabricQueue:
            value = 3;
            break;
        case _bcmFieldAuxTagMplsFwdingLabel:
            value = 4;
            break;
        case _bcmFieldAuxTagMplsCntlWord:
            value = 5;
            break;
        case _bcmFieldAuxTagRtag7A:
            value = 6;
            break;
        case _bcmFieldAuxTagRtag7B:
            value = 7;
            break;
        case _bcmFieldAuxTagVxlanNetworkId:
        case _bcmFieldAuxTagVxlanFlags:
        case _bcmFieldAuxTagLogicalLinkId:
            value = 8;
            break;
        default:
            return (BCM_E_INTERNAL);
        }

        soc_mem_field32_set(unit, FP_SLICE_KEY_CONTROLm, buf,
                            _bcm_trx2_aux_tag_1_field[slice_num],
                            value
                            );
    }

    if (sel->aux_tag_2_sel != _FP_SELCODE_DONT_CARE) {
        switch (sel->aux_tag_2_sel) {
        case _bcmFieldAuxTagAny:
            value = 0;
            break;
        case _bcmFieldAuxTagVn:
            value = 1;
            break;
        case _bcmFieldAuxTagCn:
            value = 2;
            break;
        case _bcmFieldAuxTagFabricQueue:
            value = 3;
            break;
        case _bcmFieldAuxTagMplsFwdingLabel:
            value = 4;
            break;
        case _bcmFieldAuxTagMplsCntlWord:
            value = 5;
            break;
        case _bcmFieldAuxTagRtag7A:
            value = 6;
            break;
        case _bcmFieldAuxTagRtag7B:
            value = 7;
            break;
        case _bcmFieldAuxTagVxlanNetworkId:
        case _bcmFieldAuxTagVxlanFlags:
        case _bcmFieldAuxTagLogicalLinkId:
            value = 8;
            break;
        default:
            return (BCM_E_INTERNAL);
        }

        soc_mem_field32_set(unit, FP_SLICE_KEY_CONTROLm, buf,
                            _bcm_trx2_aux_tag_2_field[slice_num],
                            value
                            );
    }

    rv = soc_mem_write(unit, FP_SLICE_KEY_CONTROLm, MEM_BLOCK_ALL, 0, buf);
    return (rv);
}

/*
 * Function:
 *     _field_trx_ingress_slice_clear
 *
 * Purpose:
 *     Resets the IFP field slice configuration.
 *
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     slice_numb - (IN) Field slice number.
 *
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_ingress_slice_clear(int unit, uint8 slice_numb)
{
    int                         index;
    int                         index_max;
    fp_port_field_sel_entry_t   pfs_entry;
    soc_field_t                 dw_fld;
    int                         rv;

    /* Iterate over all indexes */
    index_max = soc_mem_index_max(unit, FP_PORT_FIELD_SELm);
    for (index = 0; index < index_max; index++) {
        /* Read Port's current entry in FP_PORT_FIELD_SEL table */
        rv = READ_FP_PORT_FIELD_SELm(unit, MEM_BLOCK_ALL,
                                     index, &pfs_entry);
        BCM_IF_ERROR_RETURN(rv);
        soc_FP_PORT_FIELD_SELm_field32_set
            (unit,
             &pfs_entry,
             _bcm_field_trx_field_sel[slice_numb][0],
             0
             );
        BCM_IF_ERROR_RETURN(rv);

        soc_FP_PORT_FIELD_SELm_field32_set
            (unit,
             &pfs_entry,
             _bcm_field_trx_field_sel[slice_numb][1],
             0
             );
        BCM_IF_ERROR_RETURN(rv);
        soc_FP_PORT_FIELD_SELm_field32_set
            (unit,
             &pfs_entry,
             _bcm_field_trx_field_sel[slice_numb][2],
             0
             );
        BCM_IF_ERROR_RETURN(rv);

        dw_fld = _trx_ifp_double_wide_key[slice_numb];
        if (SOC_MEM_FIELD_VALID(unit, FP_PORT_FIELD_SELm, dw_fld)) {
            soc_FP_PORT_FIELD_SELm_field32_set(unit, &pfs_entry, dw_fld, 0);
        }

        dw_fld = _bcm_field_trx_slice_wide_mode_field[slice_numb];
        if (SOC_MEM_FIELD_VALID(unit, FP_PORT_FIELD_SELm, dw_fld)) {
            soc_FP_PORT_FIELD_SELm_field32_set(unit, &pfs_entry, dw_fld, 0);
        }

        dw_fld = _bcm_field_trx_slice_pairing_field[slice_numb / 2];
        soc_FP_PORT_FIELD_SELm_field32_set(unit, &pfs_entry, dw_fld, 0);

        rv = WRITE_FP_PORT_FIELD_SELm(unit, MEM_BLOCK_ALL, index, &pfs_entry);
        BCM_IF_ERROR_RETURN(rv);
    }

    dw_fld = _bcm_field_trx_dw_f4_sel[slice_numb];
    if (SOC_REG_FIELD_VALID(unit, FP_DOUBLE_WIDE_F4_SELECTr, dw_fld)) {
        rv = soc_reg_field32_modify(unit, FP_DOUBLE_WIDE_F4_SELECTr,
                                    REG_PORT_ANY, dw_fld,  0);
        BCM_IF_ERROR_RETURN(rv);
    }

    /* Clear VRF Force Forwarding Enable Field */
    if (SOC_REG_FIELD_VALID(unit, FP_FORCE_FORWARDING_FIELDr,
                _bcm_trx2_vrf_force_forwarding_enable_field[slice_numb])) {
        rv =  soc_reg_field32_modify(unit, FP_FORCE_FORWARDING_FIELDr,
                    REG_PORT_ANY,
                    _bcm_trx2_vrf_force_forwarding_enable_field[slice_numb], 0);
            BCM_IF_ERROR_RETURN(rv);
    }
    return (BCM_E_NONE);
}
/*
 * Function:
 *     _field_trx_lookup_selcodes_install
 *
 * Purpose:
 *     Writes the field select codes (ie. FPFx).
 *     for VFP (_BCM_FIELD_STAGE_LOOKUP) lookup stage.
 *
 * Parameters:
 *     unit  - BCM device number
 *     fs    - slice that needs its select codes written
 *
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_lookup_selcodes_install(int unit, _field_group_t *fg,
                                   uint8 slice_numb, int selcode_index)
{
    uint32        reg_val;
    _field_sel_t  *sel;
    int           rv;
    sel = &fg->sel_codes[selcode_index];

    rv = READ_VFP_KEY_CONTROLr(unit, &reg_val);
    BCM_IF_ERROR_RETURN(rv);

    if ((fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) &&
        (selcode_index % 2)) {
        if (sel->fpf2 != _FP_SELCODE_DONT_CARE) {
#ifndef BCM_HURRICANE2_SUPPORT
            soc_reg_field_set(unit,
                              VFP_KEY_CONTROLr,
                              &reg_val,
                              _bcm_field_trx_vfp_double_wide_sel[slice_numb],
                              sel->fpf2
                              );
#endif
        }
    } else {
        if (sel->fpf2 != _FP_SELCODE_DONT_CARE) {
            soc_reg_field_set(unit,
                              VFP_KEY_CONTROLr,
                              &reg_val,
                              _bcm_field_trx_vfp_field_sel[slice_numb][0],
                              sel->fpf2
                              );
        }
        if (sel->fpf3 != _FP_SELCODE_DONT_CARE) {
            soc_reg_field_set(unit,
                              VFP_KEY_CONTROLr,
                              &reg_val,
                              _bcm_field_trx_vfp_field_sel[slice_numb][1],
                              sel->fpf3
                              );
        }
    }
    rv = WRITE_VFP_KEY_CONTROLr(unit, reg_val);
    BCM_IF_ERROR_RETURN(rv);

#ifndef BCM_HURRICANE2_SUPPORT
    /* Set inner/outer ip header selection. */
    if (sel->ip_header_sel != _FP_SELCODE_DONT_CARE) {
        rv = soc_reg_field32_modify(unit,
                                    VFP_KEY_CONTROL_2r,
                                    REG_PORT_ANY,
                                    _bcm_field_trx_vfp_ip_header_sel[slice_numb],
                                    sel->ip_header_sel
                                    );
    }
#endif

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA(unit)) {
       if (sel->src_type_sel != _FP_SELCODE_DONT_CARE) {
          rv = soc_reg_field32_modify(unit, VFP_KEY_CONTROL_2r, REG_PORT_ANY,
                                    _trx_vfp_src_type_sel[slice_numb],
                         sel->src_type_sel == _bcmFieldFwdEntityGlp ? 1 : 0);
       }
    }

    if (SOC_IS_KATANA2(unit)) {
       if (sel->src_type_sel != _FP_SELCODE_DONT_CARE) {
          rv = soc_reg_field32_modify(unit, VFP_KEY_CONTROL_1r, REG_PORT_ANY,
                                    _trx_vfp_src_type_sel[slice_numb],
                            sel->src_type_sel == _bcmFieldFwdEntityGlp ? 1 :0);
       }
    }
#endif
    return (rv);
}

/*
 * Function:
 *     _field_trx_lookup_slice_clear
 *
 * Purpose:
 *     Reset slice configuraton on group deletion event.
 *
 * Parameters:
 *     unit  - BCM device number
 *     fs    - slice that needs its select codes written
 *
 * Returns:
 *     BCM_E_NONE     - Success
 *
 * Note:
 *     Unit lock should be held by calling function.
 */
STATIC int
_field_trx_lookup_slice_clear(int unit, uint8 slice_numb)
{
    uint32 reg_val;
    int    rv;

    SOC_IF_ERROR_RETURN(READ_VFP_KEY_CONTROLr(unit, &reg_val));
#ifndef BCM_HURRICANE2_SUPPORT
    soc_reg_field_set(unit,
                      VFP_KEY_CONTROLr,
                      &reg_val,
                      _bcm_field_trx_vfp_double_wide_sel[slice_numb],
                      0
                      );
#endif
    soc_reg_field_set(unit,
                      VFP_KEY_CONTROLr,
                      &reg_val,
                      _bcm_field_trx_vfp_field_sel[slice_numb][0],
                      0
                      );
    soc_reg_field_set(unit,
                      VFP_KEY_CONTROLr,
                      &reg_val,
                      _bcm_field_trx_vfp_field_sel[slice_numb][1],
                      0
                      );
    soc_reg_field_set(unit,
                      VFP_KEY_CONTROLr,
                      &reg_val,
                      _bcm_field_trx_slice_pairing_field[slice_numb / 2],
                      0
                      );
    SOC_IF_ERROR_RETURN(WRITE_VFP_KEY_CONTROLr(unit, reg_val));

    rv = soc_reg_field32_modify(unit,
                                VFP_KEY_CONTROL_2r,
                                REG_PORT_ANY,
                                _bcm_field_trx_vfp_ip_header_sel[slice_numb],
                                0
                                );
    if (SOC_IS_KATANA(unit)) {
       rv = soc_reg_field32_modify(unit, VFP_KEY_CONTROL_2r, REG_PORT_ANY,
                                    _trx_vfp_src_type_sel[slice_numb], 0);
    }

    if (SOC_IS_KATANA2(unit)) {
      rv = soc_reg_field32_modify(unit, VFP_KEY_CONTROL_1r, REG_PORT_ANY,
                                    _trx_vfp_src_type_sel[slice_numb], 0);
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_field_trx_slice_clear
 *
 * Purpose:
 *     Clear slice configuration on group removal
 *
 * Parameters:
 *     unit  - BCM device number
 *     fg    - Field group slice belongs to
 *     fs    - Field slice structure.
 *
 * Returns:
 *     BCM_E_XXX
 *
 */
int
_bcm_field_trx_slice_clear(int unit, _field_group_t *fg, _field_slice_t *fs)
{
    int rv;

    switch (fs->stage_id) {
      case _BCM_FIELD_STAGE_INGRESS:
          rv = _bcm_field_trx_ingress_slice_clear(unit, fs->slice_number);
          break;
      case _BCM_FIELD_STAGE_LOOKUP:
          rv = _field_trx_lookup_slice_clear(unit, fs->slice_number);
          break;
      case _BCM_FIELD_STAGE_EGRESS:
          rv = _bcm_field_trx_egress_slice_clear(unit, fs->slice_number);
          break;
#if defined(BCM_TRIUMPH_SUPPORT)
      case _BCM_FIELD_STAGE_EXTERNAL:
          rv = _bcm_field_tr_external_slice_clear(unit, fg);
          break;
#endif /* BCM_TRIUMPH_SUPPORT */
      default:
          rv = BCM_E_INTERNAL;
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_field_trx_selcodes_install
 *
 * Purpose:
 *     Writes the field select codes (ie. FPFx).
 *
 * Parameters:
 *     unit  - BCM device number
 *     fs    - slice that needs its select codes written
 *
 * Returns:
 *     BCM_E_INTERNAL - On read/write errors
 *     BCM_E_NONE     - Success
 *
 * Note:
 *     Unit lock should be held by calling function.
 */
int
_bcm_field_trx_selcodes_install(int unit, _field_group_t *fg,
                                uint8 slice_numb, bcm_pbmp_t pbmp,
                                int selcode_index)
{
    int rv;    /* Operation return status. */

    /* Input parameters check. */
    if (NULL == fg) {
        return (BCM_E_PARAM);
    }

    /* Set slice mode. Single/Double/Triple, Intraslice */
    rv = _bcm_field_trx_mode_set(unit, slice_numb, fg, fg->flags);
    BCM_IF_ERROR_RETURN(rv);

    switch (fg->stage_id) {
      case _BCM_FIELD_STAGE_INGRESS:
          rv = _bcm_field_trx_ingress_selcodes_install(unit, fg, slice_numb,
                                                       &pbmp, selcode_index);
          break;
      case _BCM_FIELD_STAGE_LOOKUP:
          rv = _field_trx_lookup_selcodes_install(unit, fg, slice_numb,
                                                  selcode_index);
          break;
      case _BCM_FIELD_STAGE_EGRESS:
      case _BCM_FIELD_STAGE_EXTERNAL:
          rv = (BCM_E_NONE);
          break;
      default:
          rv = (BCM_E_PARAM);
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_field_trx_qual_lists_get
 * Purpose:
 *     Build a group's qualifiers array by assembling
 *     qualifiers from each select code.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     stage_fc  - (IN) Stage field control structure.
 *     fg        - (IN) Group control structure.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_qual_lists_get(int unit, _field_stage_t *stage_fc,
                             _field_group_t *fg)
{
    return (BCM_E_NONE);
}


/*
 * Function:
 *     _bcm_field_trx_tcam_get
 * Purpose:
 *    Get the rules to be written into tcam.
 * Parameters:
 *     unit      -  (IN) BCM device number.
 *     tcam_mem  -  (IN) TCAM memory
 *     f_ent     -  (IN)  Field entry structure to get tcam info from.
 *     buf       -  (OUT) TCAM entry
 * Returns:
 *     BCM_E_NONE  - Success
 * Note:
 *     Unit lock should be held by calling function.
 */
int
_bcm_field_trx_tcam_get(int unit, soc_mem_t mem,
                        _field_entry_t *f_ent, uint32 *buf)
{
    soc_field_t         key_field, mask_field;
    _field_tcam_t       *tcam;
    _field_group_t      *fg;
    uint32 valid_value = 0;

    fg = f_ent->group;

    if (_BCM_FIELD_STAGE_INGRESS == fg->stage_id) {
#ifdef BCM_TRIDENT_SUPPORT
        if (mem == FP_GM_FIELDSm) {
            valid_value = 1;
            tcam = &f_ent->extra_tcam;
            key_field = KEYf;
            mask_field = MASKf;
        } else
#endif /* BCM_TRIDENT_SUPPORT */
        if ((SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || SOC_IS_KATANAX(unit))
             && FP_GLOBAL_MASK_TCAMm == mem) {
	    if (SOC_IS_KATANAX(unit)) {
                valid_value = SOC_IS_KATANA2(unit) ? 3: 1;
            }
            tcam = &f_ent->extra_tcam;
            key_field = IPBMf;
            mask_field = IPBM_MASKf;
        } else {
            valid_value = (fg->flags & _FP_GROUP_LOOKUP_ENABLED) ? 3 : 2;
            tcam = &f_ent->tcam;
        /* Intra-slice double wide key */
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD_TT(unit)) {
                key_field = KEYf;
                mask_field = MASKf;
            } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_HURRICANE2_SUPPORT
            if (SOC_IS_HURRICANE2(unit)) {
               key_field = DATA_KEYf;
               mask_field = DATA_MASKf;
            } else
#endif
            {
                key_field = DATAf;
                mask_field = DATA_MASKf;
            }
        }

        if (soc_mem_field_valid(unit, mem, VALIDf)) {
            soc_mem_field32_set(unit, mem, buf, VALIDf, valid_value);
        }
        soc_mem_field_set(unit, mem, buf, key_field, tcam->key);
        soc_mem_field_set(unit, mem, buf, mask_field, tcam->mask);
    } else {
        tcam = &f_ent->tcam;

        if (_BCM_FIELD_STAGE_LOOKUP == fg->stage_id) {
            mask_field = MASKf;
        } else if (_BCM_FIELD_STAGE_EGRESS == fg->stage_id) {
            mask_field = KEY_MASKf;
        } else {
            return (BCM_E_PARAM);
        }

        if (SOC_IS_TRIDENT2(unit) && (_BCM_FIELD_STAGE_EGRESS == fg->stage_id)) {
            soc_mem_field32_set(unit, mem, buf, KEY_SPAREf, 0);
            soc_mem_field32_set(unit, mem, buf, KEY_MASK_SPAREf, 0);
        } 

        soc_mem_field_set(unit, mem, buf, KEYf, tcam->key);
        soc_mem_field_set(unit, mem, buf, mask_field, tcam->mask);
        soc_mem_field32_set(unit, mem, buf, VALIDf,
                            (fg->flags & _FP_GROUP_LOOKUP_ENABLED) ? 3 : 2);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_tcp_ttl_tos_init
 * Purpose:
 *     Initialize the TCP_FN, TTL_FN and TOS_FN tables
 * Parameters:
 *     unit       - (IN) BCM device number.
 * Returns:
 *     BCM_E_NONE
 * Notes:
 */
int
_bcm_field_trx_tcp_ttl_tos_init(int unit)
{
    int i;
    tcp_fn_entry_t tcp_entry;
    ttl_fn_entry_t ttl_entry;
    tos_fn_entry_t tos_entry;

    if (SOC_WARM_BOOT(unit)) {
        return (BCM_E_NONE);
    }

    sal_memset(&tcp_entry, 0, sizeof(tcp_fn_entry_t));
    sal_memset(&ttl_entry, 0, sizeof(ttl_fn_entry_t));
    sal_memset(&tos_entry, 0, sizeof(tos_fn_entry_t));

    /* TCP_FN table */
    for (i = soc_mem_index_min(unit, TCP_FNm);
         i <= soc_mem_index_max(unit, TCP_FNm); i++) {

        soc_mem_field32_set(unit, TCP_FNm, &tcp_entry, FN0f, i);
        soc_mem_field32_set(unit, TCP_FNm, &tcp_entry, FN1f, i);

        soc_mem_write(unit, TCP_FNm, MEM_BLOCK_ALL, i, &tcp_entry);
    }

    /* TTL_FN table */
    for (i = soc_mem_index_min(unit, TTL_FNm);
         i <= soc_mem_index_max(unit, TTL_FNm); i++) {

        soc_mem_field32_set(unit, TTL_FNm, &ttl_entry, FN0f, i);
        soc_mem_field32_set(unit, TTL_FNm, &ttl_entry, FN1f, i);

        soc_mem_write(unit, TTL_FNm, MEM_BLOCK_ALL, i, &ttl_entry);
    }

    /* TOS_FN table */
    for (i = soc_mem_index_min(unit, TOS_FNm);
         i <= soc_mem_index_max(unit, TOS_FNm); i++) {

        soc_mem_field32_set(unit, TOS_FNm, &tos_entry, FN0f, i);
        soc_mem_field32_set(unit, TOS_FNm, &tos_entry, FN1f, i);

        soc_mem_write(unit, TOS_FNm, MEM_BLOCK_ALL, i, &tos_entry);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_field_trx_write_slice_map_egress
 *
 * Purpose:
 *     Write the EFP_SLICE_MAP (EGRESS)
 *
 * Parameters:
 *     unit
 *     stage_fc - pointer to stage control block
 *
 * Returns:
 *     BCM_E_XXX
 *
 * Notes:
 */
int
_bcm_field_trx_write_slice_map_egress(int unit, _field_stage_t *stage_fc)
{
    soc_field_t field;               /* HW entry fields.         */
    uint32 map_entry;                /* HW entry buffer.         */
    int vmap_size;                   /* Virtual map index count. */
    uint32 value;                    /* Field entry value.       */
    int idx;                         /* Map fields iterator.     */
    int rv;                          /* Operation return status. */

    uint32 virtual_to_physical_map[] = {
        VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBERf,
        VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBERf,
        VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBERf,
        VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBERf};
    uint32 virtual_to_group_map[] = {
        VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUPf,
        VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUPf,
        VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUPf,
        VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUPf};

    /* Calculate virtual map size. */
    rv = _bcm_field_virtual_map_size_get(unit, stage_fc, &vmap_size);
    BCM_IF_ERROR_RETURN(rv);

    rv = READ_EFP_SLICE_MAPr(unit, &map_entry);
    BCM_IF_ERROR_RETURN(rv);
    for (idx = 0; idx < vmap_size; idx++) {
        value = (stage_fc->vmap[_FP_VMAP_DEFAULT][idx]).vmap_key;
        field = virtual_to_physical_map[idx];
        soc_reg_field_set(unit, EFP_SLICE_MAPr, &map_entry, field, value);

        value = (stage_fc->vmap[_FP_VMAP_DEFAULT][idx]).virtual_group;
        field = virtual_to_group_map[idx];
        soc_reg_field_set(unit, EFP_SLICE_MAPr, &map_entry, field, value);
    }

    rv = WRITE_EFP_SLICE_MAPr(unit, map_entry);
    BCM_IF_ERROR_RETURN(rv);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_write_slice_map_vfp
 *
 * Purpose:
 *     Write the VFP_SLICE_MAP (LOOKUP)
 *
 * Parameters:
 *     unit
 *     stage_fc - pointer to stage control block
 *
 * Returns:
 *     BCM_E_XXX
 *
 */
int
_bcm_field_trx_write_slice_map_vfp(int unit, _field_stage_t *stage_fc)
{
    soc_field_t field;               /* HW entry fields.         */
    uint32 map_entry;                /* HW entry buffer.         */
    int vmap_size;                   /* Virtual map index count. */
    uint32 value;                    /* Field entry value.       */
    int idx;                         /* Map fields iterator.     */
    int rv;                          /* Operation return status. */

    uint32 virtual_to_physical_map[] = {
        VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBERf,
        VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBERf,
        VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBERf,
        VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBERf};
    uint32 virtual_to_group_map[] = {
        VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUPf,
        VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUPf,
        VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUPf,
        VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUPf};

    /* Calculate virtual map size. */
    rv = _bcm_field_virtual_map_size_get(unit, stage_fc, &vmap_size);
    BCM_IF_ERROR_RETURN(rv);

    rv = READ_VFP_SLICE_MAPr(unit, &map_entry);
    BCM_IF_ERROR_RETURN(rv);

    for (idx = 0; idx < vmap_size; idx++) {
        value = (stage_fc->vmap[_FP_VMAP_DEFAULT][idx]).vmap_key;
        field = virtual_to_physical_map[idx];
        soc_reg_field_set(unit, VFP_SLICE_MAPr, &map_entry, field, value);

        value = (stage_fc->vmap[_FP_VMAP_DEFAULT][idx]).virtual_group;
        field = virtual_to_group_map[idx];
        soc_reg_field_set(unit, VFP_SLICE_MAPr, &map_entry, field, value);
    }

    rv = WRITE_VFP_SLICE_MAPr(unit, map_entry);
    BCM_IF_ERROR_RETURN(rv);

    return (BCM_E_NONE);
}


#ifdef INCLUDE_L3
/*
 * Function:
 *     _field_trx_policy_set_l3_info
 * Purpose:
 *     Install l3 forwarding policy entry.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     mem       - (IN) Policy table memory.
 *     value     - (IN) Egress object id or combined next hop information.
 *     buf       - (IN/OUT) Hw entry buffer to write.
 * Returns:
 *     BCM_E_XXX
 */

STATIC int
_field_trx_policy_set_l3_info(int unit, soc_mem_t mem, int value, uint32 *buf)
{
    uint32 flags;         /* L3 forwarding flags           */
    int nh_ecmp_id;       /* Next hop/Ecmp group id.       */
    int retval;           /* Operation return value.       */

    /* Resove next hop /ecmp group id. */
    retval = _bcm_field_policy_set_l3_nh_resolve(unit,  value,
                                                 &flags, &nh_ecmp_id);
    BCM_IF_ERROR_RETURN(retval);

    if (flags & BCM_L3_MULTIPATH) {
        FP_VVERB(("FP(unit %d) vverb: Install mpath L3 policy (Ecmp_group: %d)))",
                  unit, nh_ecmp_id));
        PolicySet(unit, mem, buf, ECMPf, 1);
        PolicySet(unit, mem, buf, ECMP_PTRf, nh_ecmp_id);
    } else {
        FP_VVERB(("FP(unit %d) vverb: Install unipath L3 policy(Next hop id: %d)))",
                  unit, nh_ecmp_id));
        if (SOC_MEM_FIELD_VALID(unit, mem, ECMPf)) {
            PolicySet(unit, mem, buf, ECMPf, 0);
        }
        PolicySet(unit, mem, buf, NEXT_HOP_INDEXf, nh_ecmp_id);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx_policy_egr_nexthop_info_set
 * Purpose:
 *     Set next hop redirection value in the policy table entry.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     mem       - (IN) Policy table memory.
 *     value     - (IN) Egress object id or combined next hop information.
 *     buf       - (IN/OUT) Hw entry buffer to write.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_policy_egr_nexthop_info_set(int unit, soc_mem_t mem,
    int value, uint32 *buf)
{
    uint32 redirect_to_nhi = 0; /* Hw config + (ECMP or NH Info)    */
    uint32 flags;               /* L3 forwarding flags.             */
    int nh_ecmp_id;             /* Next hop/Ecmp group id.          */

    /* Resove next hop /ecmp group id. */
    BCM_IF_ERROR_RETURN(_bcm_field_policy_set_l3_nh_resolve(unit,  value,
            &flags, &nh_ecmp_id));

    /* Check if NextHop is ECMP or regular Next Hop */
    if (flags & BCM_L3_MULTIPATH) {
        /* ECMP next hop */
        if (soc_feature(unit, soc_feature_field_action_redirect_ecmp)) {
            if (SOC_IS_KATANAX(unit)) {
                redirect_to_nhi = (0x2 << 16) | nh_ecmp_id;
            } else {
                redirect_to_nhi = (0x3 << 16) | nh_ecmp_id;
            }
        } else {
            /* Non-Trident devices do not support redirect to ECMP action */
            return (BCM_E_UNAVAIL);
        }
        FP_VVERB(("FP(unit %d) vverb: Set ECMP (Group id: %d\n)))",
            unit, nh_ecmp_id));
    } else {
        /* Regular next hop */
        if (SOC_IS_TRIDENT2(unit)) {
            redirect_to_nhi = (0x2 << 18) | nh_ecmp_id;
        } else if (SOC_IS_TD_TT(unit)) {
            redirect_to_nhi = (0x2 << 16) | nh_ecmp_id;
        } else if (SOC_IS_KATANAX(unit)) {
            redirect_to_nhi = (0x1 << 16) | nh_ecmp_id;
        } else {
            redirect_to_nhi = (0x1 << 14) | nh_ecmp_id;
        }
        FP_VVERB(("FP(unit %d) vverb: Set unipath (Nexthop index: %d\n)))",
            unit, nh_ecmp_id));
    }

    /* Set policy table redirection fields */
    PolicySet(unit, mem, buf, REDIRECTIONf, redirect_to_nhi);
    PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 1);
    return (BCM_E_NONE);
}
#endif /* INCLUDE_L3 */

/*
 * Function:
 *     _field_trx_action_fabricQueue_actions_set
 * Purpose:
 *     Install add fabric tag action
 * Parameters:
 *     unit     - (IN) BCM device number
 *     mem      - (IN) Policy table memory
 *     cosq_idx  - (IN) cosq index or (ucast_queue_group/ucast_subscriber) cosq gport
 *     profile_idx - (IN) QoS map profile index
 *     buf      - (IN/OUT) Hw entry buffer to write.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_action_fabricQueue_actions_set(int unit, soc_mem_t mem,
                                    int cosq_idx, int profile_idx, uint32 *buf) 
{
    int traffic_manager;  /* Indicates the intended application for the
                            SBX_QUEUE_TAG.0x0-Fabric Interface Chip.
                                          0x1 Traffic Manager.*/
    int tag_type;        /*  SBX tag type.                    */
    uint32 param0 = 0;
    uint32 in_flags = 0;
#ifdef BCM_KATANA_SUPPORT
    bcm_port_t port;
    int queue_index = 0, mask = 0, param1 = 0, modid = 0;
    ing_queue_map_entry_t ing_queue_entry;
#endif

    if (NULL == buf) {
        return (BCM_E_PARAM);
    }

    param0 = cosq_idx;

#if defined(BCM_KATANA_SUPPORT)
    param1 = profile_idx;
    /* If param1 BCM_FABRIC_QUEUE_xxx flags are set then 
    *  param0 is ucast_queu_group or ucast_subscriber_queue_group cosq gport
    */
    if (SOC_IS_KATANAX(unit)) {
        if ((param1 & BCM_FABRIC_QUEUE_DEST_OFFSET) ||
            (param1 & BCM_FABRIC_QUEUE_CUSTOMER)) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(param0) ||
                BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(param0)) {
#if defined(BCM_KATANA2_SUPPORT)
                if (SOC_IS_KATANA2(unit)) {
                    BCM_IF_ERROR_RETURN(_bcm_kt2_cosq_index_resolve(unit,
                        param0, 0, _BCM_KT_COSQ_INDEX_STYLE_BUCKET,
                        &port, &cosq_idx, NULL));
                } else
#endif
                {
                    if (SOC_IS_KATANA(unit)) {
                        BCM_IF_ERROR_RETURN(_bcm_kt_cosq_index_resolve(unit,
                            param0, 0, _BCM_KT_COSQ_INDEX_STYLE_BUCKET,
                            &port, &cosq_idx, NULL));
                    }
                }
                in_flags = param1;
            } else {
                return BCM_E_PARAM;
            }
        } else {
            /* backward compatibility : API passing hw_index directly */
            BCM_IF_ERROR_RETURN(
                bcm_kt_cosq_port_get(unit, cosq_idx & 0xffff, &port));
            in_flags = param0;
        }
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &modid));
        port = ((modid & 0xff) << 7) | (port & 0x7f);
    } else
#endif
    {
        in_flags = param0;
    }

    /* Tag type resolution. */
    if (in_flags & BCM_FABRIC_QUEUE_QOS_BASE) {
        tag_type = 0x3;  /* Offset to base queue number from the
                          QUEUE_MAP Table. Index into QUEUE_MAP
                          Table is {DST_MODID, DST_PID} */
    } else if (in_flags & BCM_FABRIC_QUEUE_DEST_OFFSET) {
        tag_type = 0x2;  /* Index into QUEUE_MAP Table used for lookup. */
    } else {
        tag_type = 0x1;  /* Explicit queue number. */
    }

    PolicySet(unit, mem, buf, EH_TAG_TYPEf, tag_type);

    /* Traffic manager vs Fabric chip queue selection. */
    if (soc_mem_field_valid (unit, mem, EH_TMf)) {
        traffic_manager = (cosq_idx & BCM_FABRIC_QUEUE_CUSTOMER) ? 0x1 : 0x0;
        PolicySet(unit, mem, buf, EH_TMf, traffic_manager);
    }

    /* Set queue number. */
    PolicySet(unit, mem, buf, EH_QUEUE_TAGf, cosq_idx & 0xffff);

#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x1);
        PolicySet(unit, mem, buf, REDIRECTION_DGLPf, port);

        if (tag_type == 0x2)
        {
            queue_index = cosq_idx & 0xffff;
            sal_memset(&ing_queue_entry, 0, sizeof(ing_queue_map_entry_t));
            BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_QUEUE_MAPm,
                              MEM_BLOCK_ANY,  queue_index, &ing_queue_entry));
            if (soc_ING_QUEUE_MAPm_field32_get(unit, &ing_queue_entry, 
                                               QUEUE_SET_BASEf) != 0) {
               return BCM_E_BUSY;
            }
            soc_mem_field32_set(unit, ING_QUEUE_MAPm,
                                &ing_queue_entry, QUEUE_SET_BASEf, queue_index);
            mask = (1 << soc_mem_field_length(
                       unit, ING_QUEUE_MAPm, QUEUE_OFFSET_PROFILE_INDEXf)) - 1;
            soc_mem_field32_set(unit, ING_QUEUE_MAPm, &ing_queue_entry,
                            QUEUE_OFFSET_PROFILE_INDEXf, profile_idx & mask);
            BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_WRITE(unit, ING_QUEUE_MAPm,
                            queue_index, &ing_queue_entry));
        }
    }
#endif

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx_action_copy_to_cpu
 * Purpose:
 *     Install copy to cpu action in policy table.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     mem      - (IN) Policy table memory
 *     f_ent    - (IN) Field entry structure to get policy info from
 *     fa       - (IN  Field action
 *     buf      - (OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
int
_field_trx_action_copy_to_cpu(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                              _field_action_t *fa, uint32 *buf)
{
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    switch (fa->action) {
      case bcmFieldActionTimeStampToCpu:
          PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x5);
          PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x5);
          PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x5);
          PolicySet(unit, mem, buf, R_DROPf, 0x1);
          PolicySet(unit, mem, buf, Y_DROPf, 0x1);
          PolicySet(unit, mem, buf, G_DROPf, 0x1);
          break;
      case bcmFieldActionRpTimeStampToCpu:
          PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x5);
          PolicySet(unit, mem, buf, R_DROPf, 0x1);
          break;
      case bcmFieldActionYpTimeStampToCpu:
          PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x5);
          PolicySet(unit, mem, buf, Y_DROPf, 0x1);
          break;
      case bcmFieldActionGpTimeStampToCpu:
          PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x5);
          PolicySet(unit, mem, buf, G_DROPf, 0x1);
          break;
      case bcmFieldActionCopyToCpu:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicySet(unit, mem, buf, COPY_TO_CPUf, 0x1);
          } else {
              PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x1);
              PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x1);
              PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x1);
          }
          break;
      case bcmFieldActionRpCopyToCpu:
          PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x1);
          break;
      case bcmFieldActionYpCopyToCpu:
          PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x1);
          break;
      case bcmFieldActionGpCopyToCpu:
          PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x1);
          break;
      default:
          return (BCM_E_INTERNAL);
    }

    if (fa->param[0] != 0) {
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, VFP_MATCHED_RULEf, fa->param[1]);
        } else {
            PolicySet(unit, mem, buf, MATCHED_RULEf, fa->param[1]);
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx_action_copy_to_cpu_cancel
 * Purpose:
 *     Override copy to cpu action in policy table.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     mem      - (IN) Policy table memory
 *     f_ent    - (IN) Field entry structure to get policy info from
 *     fa       - (IN  Field action
 *     buf      - (OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_action_copy_to_cpu_cancel(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                                     _field_action_t *fa, uint32 *buf)
{
    uint32 value = 0;

    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    switch (fa->action) {
      case bcmFieldActionTimeStampToCpuCancel:
          PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x2);
          PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x2);
          PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x2);
          PolicySet(unit, mem, buf, R_DROPf, 0x2);
          PolicySet(unit, mem, buf, Y_DROPf, 0x2);
          PolicySet(unit, mem, buf, G_DROPf, 0x2);
          break;
      case bcmFieldActionRpTimeStampToCpuCancel:
          PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x2);
          PolicySet(unit, mem, buf, R_DROPf, 0x2);
          break;
      case bcmFieldActionYpTimeStampToCpuCancel:
          PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x2);
          PolicySet(unit, mem, buf, Y_DROPf, 0x2);
          break;
      case bcmFieldActionGpTimeStampToCpuCancel:
          PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x2);
          PolicySet(unit, mem, buf, G_DROPf, 0x2);
          break;
      case bcmFieldActionCopyToCpuCancel:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicySet(unit, mem, buf, COPY_TO_CPUf, 0x2);
          } else {
              value = COPY_TO_CPU_CANCEL;
#if defined(BCM_TRIUMPH3_SUPPORT)
              if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
                  SOC_IS_KATANA2(unit)) { 
                  if ((SWITCH_TO_CPU_CANCEL ==
                       PolicyGet(unit, mem, buf, R_COPY_TO_CPUf)) && 
                      (SWITCH_TO_CPU_CANCEL == 
                       PolicyGet(unit, mem, buf, Y_COPY_TO_CPUf)) &&
                      (SWITCH_TO_CPU_CANCEL ==
                       PolicyGet(unit, mem, buf, G_COPY_TO_CPUf))) {

                      value = COPY_AND_SWITCH_TO_CPU_CANCEL; 
          }
              }
#endif
              PolicySet(unit, mem, buf, R_COPY_TO_CPUf, value);
              PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, value);
              PolicySet(unit, mem, buf, G_COPY_TO_CPUf, value);
          }

          break;
      case bcmFieldActionRpCopyToCpuCancel:
          value = COPY_TO_CPU_CANCEL; 
#if defined(BCM_TRIUMPH3_SUPPORT)
          if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
              SOC_IS_KATANA2(unit)) {
              if (SWITCH_TO_CPU_CANCEL == 
                  PolicyGet(unit, mem, buf, R_COPY_TO_CPUf)) {

                  value = COPY_AND_SWITCH_TO_CPU_CANCEL;
              }
          }
#endif
          PolicySet(unit, mem, buf, R_COPY_TO_CPUf, value);
          break;
      case bcmFieldActionYpCopyToCpuCancel:
          value = COPY_TO_CPU_CANCEL;
#if defined(BCM_TRIUMPH3_SUPPORT)
          if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
              SOC_IS_KATANA2(unit)) {
              if (SWITCH_TO_CPU_CANCEL ==
                  PolicyGet(unit, mem, buf, Y_COPY_TO_CPUf)) {

                  value = COPY_AND_SWITCH_TO_CPU_CANCEL;
              }
          }
#endif
          PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, value);
          break;
      case bcmFieldActionGpCopyToCpuCancel:
          value = COPY_TO_CPU_CANCEL;
#if defined(BCM_TRIUMPH3_SUPPORT)
          if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
              SOC_IS_KATANA2(unit)) {
              if (SWITCH_TO_CPU_CANCEL ==
                  PolicyGet(unit, mem, buf, G_COPY_TO_CPUf)) {

                  value = COPY_AND_SWITCH_TO_CPU_CANCEL;
              }
          }
#endif
          PolicySet(unit, mem, buf, G_COPY_TO_CPUf, value);

          break;
      default:
          return (BCM_E_INTERNAL);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx_action_ecn_update
 * Purpose:
 *     Install ECN bits int IP header TOS field update action.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     mem      - (IN) Policy table memory.
 *     f_ent    - (IN) Entry structure.
 *     fa       - (IN) Field action.
 *     buf      - (OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_action_ecn_update (int unit, soc_mem_t mem, _field_entry_t *f_ent,
                                  _field_action_t *fa, uint32 *buf)
{
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    
    /* ECN value sanity check. */
    switch (fa->param[0]) {
      case 0x1:
      case 0x2:
      case 0x3:
          break;
      case 0:
          return (BCM_E_UNAVAIL);
      default:
          return (BCM_E_PARAM);
    }

    if (SOC_IS_TD2_TT2(unit)) {
        switch (fa->action) {
            case bcmFieldActionEcnNew:
                PolicySet(unit, mem, buf, R_CHANGE_ECNf, 1);
                PolicySet(unit, mem, buf, R_NEW_ECNf, fa->param[0]);
                PolicySet(unit, mem, buf, Y_CHANGE_ECNf, 1);
                PolicySet(unit, mem, buf, Y_NEW_ECNf, fa->param[0]);
                PolicySet(unit, mem, buf, G_CHANGE_ECNf, 1);
                PolicySet(unit, mem, buf, G_NEW_ECNf, fa->param[0]);
                break;
            case bcmFieldActionRpEcnNew:
                PolicySet(unit, mem, buf, R_CHANGE_ECNf, 1);
                PolicySet(unit, mem, buf, R_NEW_ECNf, fa->param[0]);
                break;
            case bcmFieldActionYpEcnNew:
                PolicySet(unit, mem, buf, Y_CHANGE_ECNf, 1);
                PolicySet(unit, mem, buf, Y_NEW_ECNf, fa->param[0]);
                break;
            case bcmFieldActionGpEcnNew:
                PolicySet(unit, mem, buf, G_CHANGE_ECNf, 1);
                PolicySet(unit, mem, buf, G_NEW_ECNf, fa->param[0]);
                break;
            default:
                return (BCM_E_PARAM);
        }
    }
    else {
        switch (fa->action) {
            case bcmFieldActionEcnNew:
                PolicySet(unit, mem, buf, R_CHANGE_ECNf, fa->param[0]);
                PolicySet(unit, mem, buf, Y_CHANGE_ECNf, fa->param[0]);
                PolicySet(unit, mem, buf, G_CHANGE_ECNf, fa->param[0]);
                break;
            case bcmFieldActionRpEcnNew:
                PolicySet(unit, mem, buf, R_CHANGE_ECNf, fa->param[0]);
                break;
            case bcmFieldActionYpEcnNew:
                PolicySet(unit, mem, buf, Y_CHANGE_ECNf, fa->param[0]);
                break;
            case bcmFieldActionGpEcnNew:
                PolicySet(unit, mem, buf, G_CHANGE_ECNf, fa->param[0]);
                break;
            default:
                return (BCM_E_PARAM);
        }
    }

    return (BCM_E_NONE);
}

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT)
/*
 * Function:
 *     _field_trx_flex_stat_action_set
 * Purpose:
 *     Install flex counter update action into policy table.
 * Parameters:
 *     unit         - (IN) BCM device number
 *     f_ent        - (IN) entry structure to get policy info from
 *     mem          - (IN) Policy table memory
 *     tcam_idx     - (IN) Common index of various tables
 *     buf          - (OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_flex_stat_action_set(int unit,
                                _field_entry_t *f_ent,
                                soc_mem_t mem,
                                int tcam_idx,
                                uint32 *buf)
{
    _field_stat_t  *f_st;  /* Field statistics descriptor. */
    int             idx;   /* Stat array index.            */


    /* Increment statistics hw reference count. */
    if ((f_ent->statistic.flags & _FP_ENTRY_STAT_VALID)
        && !(f_ent->statistic.flags & _FP_ENTRY_STAT_INSTALLED)) {
        /* Get statistics entity description structure. */
        BCM_IF_ERROR_RETURN
            (_bcm_field_stat_get(unit, f_ent->statistic.sid, &f_st));
        if (_FP_INVALID_INDEX != f_st->hw_index) {
            /*
             * For STATs that are shared by entries, hardware counters
             * are not allocated again. But reference count is incremented
             * for these counters.
             */
            f_st->hw_ref_count++;

            BCM_IF_ERROR_RETURN
                (_bcm_esw_stat_flex_attach_ingress_table_counters1
                    (unit, mem, tcam_idx, f_st->hw_mode, f_st->hw_index,
                    f_st->pool_index, buf));

            /* Mark entry as installed. */
            f_ent->statistic.flags |=  _FP_ENTRY_STAT_INSTALLED;

            /* 
             * Write individual statistics previous value, first time
             * entry is installed in hardware.
             */
            if ((1 == f_st->hw_ref_count)
                && !(f_ent->flags & _FP_ENTRY_INSTALLED)) {
                for (idx = 0; idx < f_st->nstat; idx++) {
                    BCM_IF_ERROR_RETURN
                        (_field_stat_value_set(unit, f_st, f_st->stat_arr[idx],
                                               f_st->stat_values[idx]));
                }
            }
        }
    } else {
        /* Disable counting if counter was not attached to the entry. */
        PolicySet(unit, mem, buf, FLEX_CTR_POOL_NUMBERf, 0);
        PolicySet(unit, mem, buf, FLEX_CTR_OFFSET_MODEf, 0);
        PolicySet(unit, mem, buf, FLEX_CTR_BASE_COUNTER_IDXf, 0);
    }

    return (BCM_E_NONE);
}
#endif

/*
 * Function:
 *     _bcm_field_trx_stat_action_set
 * Purpose:
 *     Install counter update action into policy table.
 * Parameters:
 *     unit         - (IN) BCM device number
 *     f_ent        - (IN) entry structure to get policy info from
 *     mem          - (IN) Policy table memory
 *     tcam_idx     - (IN) Common index of various tables
 *     buf          - (OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_stat_action_set(int unit, _field_entry_t *f_ent,
                               soc_mem_t mem, int tcam_idx,
                               uint32 *buf)
{
    _field_stat_t  *f_st;  /* Field statistics descriptor. */
    int mode;              /* Counter hw mode.             */
    int idx;               /* Counter index.               */
    int rv;                /* Opear return status.         */

    if (NULL == f_ent || NULL == buf) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    f_st = NULL;

    /* VFP doesn't have counters. */
    if ((_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id)
        && (0 == soc_feature(unit, soc_feature_field_vfp_flex_counter)
        && (!soc_feature(unit, soc_feature_advanced_flex_counter)))) {
        return (BCM_E_NONE);
    }

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT)
    if (soc_feature(unit, soc_feature_advanced_flex_counter)
        && (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id)) {
        return (_field_trx_flex_stat_action_set
                    (unit, f_ent, mem, tcam_idx, buf));
    }
#endif

    if ((0 == (f_ent->statistic.flags & _FP_ENTRY_STAT_INSTALLED)) ||
        ((f_ent->flags & _FP_ENTRY_PRIMARY) &&
         (f_ent->flags & _FP_ENTRY_STAT_IN_SECONDARY_SLICE))) {
        /* Disable counting if counter was not attached to the entry. */
        idx = 0;
        mode = 0;
    } else {
        /* Get statistics entity description structure. */
        rv = _bcm_field_stat_get(unit, f_ent->statistic.sid, &f_st);
        BCM_IF_ERROR_RETURN(rv);
        idx = f_st->hw_index;
        mode = f_st->hw_mode;

        /* Adjust counter hw mode for COUNTER_MODE_YES_NO/NO_YES */
        if (f_ent->statistic.flags & _FP_ENTRY_STAT_USE_ODD) {
            mode++;
        }

    }

    /* Write policy table counter config. */
    if (_BCM_FIELD_STAGE_EXTERNAL == f_ent->group->stage_id) {
        if (NULL != f_st) {
            f_st->pool_index = f_ent->fs->slice_number;
            f_st->hw_index = f_ent->slice_idx;
        }
        PolicySet(unit, mem, buf, EXT_COUNTER_MODEf, mode);
    } else if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
        PolicySet(unit, mem, buf, PID_COUNTER_MODEf, mode);
        PolicySet(unit, mem, buf, PID_COUNTER_INDEXf, idx);
    } else if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
        PolicySet(unit, mem, buf, USE_VINTF_CTR_IDXf, mode);
        PolicySet(unit, mem, buf, VINTF_CTR_IDXf, idx);
    } else {
        PolicySet(unit, mem, buf, COUNTER_MODEf, mode);
        PolicySet(unit, mem, buf, COUNTER_INDEXf, idx);
    }
    return (BCM_E_NONE);
}


/*
 * Function:
 *     _field_trx_redirect_profile_get
 * Purpose:
 *     Get the redirect profile for the unit
 * Parameters:
 *     unit             - BCM device number
 *     redirect_profile - (OUT) redirect profile
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     For triumph, External TCAM and IFP refer to same IFP_REDIRECTION_PROFILEm
 */
STATIC int
_field_trx_redirect_profile_get(int unit, soc_profile_mem_t **redirect_profile)
{
    _field_stage_t *stage_fc;

    /* Get stage control structure. */
    BCM_IF_ERROR_RETURN
        (_field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc));

    *redirect_profile = &stage_fc->redirect_profile;

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_redirect_profile_ref_count_get
 * Purpose:
 *     Get redirect profile entry use count.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     index     - (IN) Profile entry index.
 *     ref_count - (OUT) redirect profile use count.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_redirect_profile_ref_count_get(int unit, int index, int *ref_count)
{
    soc_profile_mem_t *redirect_profile;

    if (NULL == ref_count) {
        return (BCM_E_PARAM);
    }

    /* Get the redirect profile */
    BCM_IF_ERROR_RETURN
        (_field_trx_redirect_profile_get(unit, &redirect_profile));

    BCM_IF_ERROR_RETURN(soc_profile_mem_ref_count_get(unit, redirect_profile,
                                                      index, ref_count));
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_redirect_profile_delete
 * Purpose:
 *     Delete redirect profile entry.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     index     - (IN) Profile entry index.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_redirect_profile_delete(int unit, int index)
{
    soc_profile_mem_t *redirect_profile;

    /* Get the redirect profile */
    BCM_IF_ERROR_RETURN
        (_field_trx_redirect_profile_get(unit, &redirect_profile));

    BCM_IF_ERROR_RETURN(soc_profile_mem_delete(unit, redirect_profile, index));
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_redirect_profile_alloc
 * Purpose:
 *     Allocate redirect profile index
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     f_ent    - (IN) Field entry structure to get policy info from.
 *     fa       - (IN) Field action.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_redirect_profile_alloc(int unit, _field_entry_t *f_ent,
                                      _field_action_t *fa)
{
    ifp_redirection_profile_entry_t entry_arr[2];
    uint32            *entry_ptr[2];
    soc_profile_mem_t *redirect_profile;
    int               rv;
#ifdef INCLUDE_L3
    bcm_pbmp_t       ipmc_l2_pbmp, ipmc_l3_pbmp;
    int              ipmc_index;
    int              entry_count;
#endif /* INCLUDE_L3 */
    bcm_mcast_addr_t mcaddr;
    bcm_pbmp_t       pbmp;
    void             *entries[1];
    soc_mem_t       profile_mem = IFP_REDIRECTION_PROFILEm;

    entry_ptr[0] = (uint32 *)entry_arr;
    entry_ptr[1] =  entry_ptr[0] + soc_mem_entry_words(unit, profile_mem);
    entries[0] = (void *)&entry_arr;

    if ((NULL == f_ent) || (NULL == fa)) {
        return (BCM_E_PARAM);
    }

    /* Reset redirection profile entry. */
    sal_memcpy(entry_ptr[0], soc_mem_entry_null(unit, profile_mem),
               soc_mem_entry_words(unit, profile_mem) * sizeof(uint32));
    sal_memcpy(entry_ptr[1], soc_mem_entry_null(unit, profile_mem),
               soc_mem_entry_words(unit, profile_mem) * sizeof(uint32));

    /* Get the redirect profile */
    rv = _field_trx_redirect_profile_get(unit, &redirect_profile);
    BCM_IF_ERROR_RETURN(rv);

    switch (fa->action) {
      case bcmFieldActionRedirectPbmp:
      case bcmFieldActionRedirectBcastPbmp:
      case bcmFieldActionEgressMask:
      case bcmFieldActionEgressPortsAdd:
          SOC_PBMP_CLEAR(pbmp);
          SOC_PBMP_WORD_SET(pbmp, 0, fa->param[0]);
          SOC_PBMP_WORD_SET(pbmp, 1, fa->param[1]);
          /* Trident family could support more than 64-ports on a device*/
          if (SOC_IS_TD_TT(unit)) {
              SOC_PBMP_WORD_SET(pbmp, 2, fa->param[2]);
          }
          /* Trident2 device can support more than 96-ports */
          if (SOC_IS_TD2_TT2(unit)) {
              SOC_PBMP_WORD_SET(pbmp, 3, fa->param[3]);
          }
          /* Katana2 device can support 170 pp_ports */
          if (SOC_IS_KATANA2(unit)) {
              SOC_PBMP_WORD_SET(pbmp, 2, fa->param[2]);
              SOC_PBMP_WORD_SET(pbmp, 3, fa->param[3]);
              SOC_PBMP_WORD_SET(pbmp, 4, fa->param[4]);
              SOC_PBMP_WORD_SET(pbmp, 5, fa->param[5]);
          }

          soc_mem_pbmp_field_set(unit, profile_mem, entry_ptr[0], BITMAPf, &pbmp);
          rv = soc_profile_mem_add(unit, redirect_profile, entries,
                                   1, (uint32*) &fa->hw_index);
          BCM_IF_ERROR_RETURN(rv);
          break;
#ifdef INCLUDE_L3
      case bcmFieldActionRedirectIpmc:
          if (_BCM_MULTICAST_IS_SET(fa->param[0])) {
              if (0 == _BCM_MULTICAST_IS_L3(fa->param[0])) {
                  return (BCM_E_PARAM);
              }
              ipmc_index = _BCM_MULTICAST_ID_GET(fa->param[0]);
          } else {
              ipmc_index = fa->param[0];
          }
          rv = _bcm_esw_multicast_ipmc_read(unit, ipmc_index,
                                            &ipmc_l2_pbmp, &ipmc_l3_pbmp);
          BCM_IF_ERROR_RETURN(rv);
          if (SOC_IS_TR_VL(unit)) {
              entry_count = 2;
              soc_mem_pbmp_field_set(unit, profile_mem, entry_ptr[0], BITMAPf,
                                     &ipmc_l3_pbmp);
              soc_mem_pbmp_field_set(unit, profile_mem, entry_ptr[1], BITMAPf,
                                     &ipmc_l2_pbmp);
          } else {
              entry_count = 1;
              soc_mem_pbmp_field_set(unit, profile_mem, entry_ptr[0], L3_BITMAPf,
                                     &ipmc_l3_pbmp);
              soc_mem_pbmp_field_set(unit, profile_mem, entry_ptr[0], BITMAPf,
                                     &ipmc_l2_pbmp);
          }
          soc_mem_field32_set(unit, profile_mem, entry_ptr[0], MC_INDEXf, ipmc_index);
          /* MTU profile index overlayed on MC_INDEXf. */
          rv = soc_profile_mem_add(unit, redirect_profile,
                                   entries, entry_count, (uint32*)&fa->hw_index);
          BCM_IF_ERROR_RETURN(rv);
          break;
#endif /* INCLUDE_L3 */
      case bcmFieldActionRedirectMcast:
          rv = _bcm_xgs3_mcast_index_port_get(unit, fa->param[0], &mcaddr);
          BCM_IF_ERROR_RETURN(rv);
          soc_mem_pbmp_field_set(unit, profile_mem, entry_ptr[0], BITMAPf,
                                 &mcaddr.pbmp);
          soc_mem_field32_set(unit, profile_mem, entry_ptr[0], MC_INDEXf, fa->param[0]);
          rv = soc_profile_mem_add(unit, redirect_profile, entries,
                                   1, (uint32*)&fa->hw_index);
          BCM_IF_ERROR_RETURN(rv);
          break;
      default:
          return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_action_redirect
 * Purpose:
 *     Install redirect action in policy table.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     mem      - (IN) Policy table memory.
 *     f_ent    - (IN) Field entry structure to get policy info from.
 *     fa       - (IN) Field action.
 *     buf      - (OUT) Field Policy table entry.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_action_redirect(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                           _field_action_t *fa, uint32 *buf)
{
    uint32          redir_field = 0;
    int             shift_val;
    soc_field_t     redir_field_type = INVALIDf;

    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    switch (fa->action) {
    case bcmFieldActionOffloadRedirect:
        redir_field = ((fa->param[0] & 0x7f) << 6);
        redir_field |= (fa->param[1] & 0x3f);
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x1);
        PolicySet(unit, mem, buf, REDIRECTIONf, redir_field);

        PolicySet(unit, mem, buf, HI_PRI_ACTION_CONTROLf, 0x1);
        PolicySet(unit, mem, buf, HI_PRI_RESOLVEf, 0x1);
        PolicySet(unit, mem, buf, SUPPRESS_COLOR_SENSITIVE_ACTIONSf, 0x1);
        PolicySet(unit, mem, buf, DEFER_QOS_MARKINGSf, 0x1);
        PolicySet(unit, mem, buf, SUPPRESS_SW_ACTIONSf, 0x1);
        PolicySet(unit, mem, buf, SUPPRESS_VXLTf, 0x1);
        break;
    case bcmFieldActionRedirect: /* param0 = modid, param1 = port*/
        redir_field_type = REDIRECTIONf;
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            redir_field_type = REDIRECT_DVPf;
            if (BCM_GPORT_IS_MPLS_PORT(fa->param[1])) {
                redir_field = BCM_GPORT_MPLS_PORT_ID_GET((int)fa->param[1]);
                PolicySet(unit, mem, buf, UNICAST_REDIRECT_CONTROLf, 0x6);
            }else if (BCM_GPORT_IS_MIM_PORT(fa->param[1])) {
                redir_field = BCM_GPORT_MIM_PORT_ID_GET((int)fa->param[1]);
                PolicySet(unit, mem, buf, UNICAST_REDIRECT_CONTROLf, 0x6);
            }else if (BCM_GPORT_IS_NIV_PORT(fa->param[1])) {
                redir_field = BCM_GPORT_NIV_PORT_ID_GET((int)fa->param[1]);
                PolicySet(unit, mem, buf, UNICAST_REDIRECT_CONTROLf, 0x6);
            }else if (BCM_GPORT_IS_TRILL_PORT(fa->param[1])) {
                redir_field = BCM_GPORT_TRILL_PORT_ID_GET((int)fa->param[1]);
                PolicySet(unit, mem, buf, UNICAST_REDIRECT_CONTROLf, 0x6);
            }else if (BCM_GPORT_IS_L2GRE_PORT(fa->param[1])) {
                redir_field = BCM_GPORT_L2GRE_PORT_ID_GET((int)fa->param[1]);
                PolicySet(unit, mem, buf, UNICAST_REDIRECT_CONTROLf, 0x6);
            } else {
                redir_field_type = REDIRECTIONf;
                redir_field = ((fa->param[0] & 0xff) << 7);
                redir_field |= (fa->param[1] & 0x7f);
            }
        } else
#endif
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                if (BCM_GPORT_IS_MPLS_PORT(fa->param[1])) {
                    redir_field = BCM_GPORT_MPLS_PORT_ID_GET((int)fa->param[1]);
                    redir_field |= (3 << 17);
                } else if (BCM_GPORT_IS_MIM_PORT(fa->param[1])) {
                    redir_field = BCM_GPORT_MIM_PORT_ID_GET((int)fa->param[1]);
                    redir_field |= (3 << 17);
                } else {
                    redir_field = ((fa->param[0] & 0xff) << 7);
                    redir_field |= (fa->param[1] & 0x7f);
                }
            } else
#endif /* BCM_TRIDENT_SUPPORT */
                {
                    redir_field = ((fa->param[0] & 0x7f) << 6);
                    redir_field |= (fa->param[1] & 0x3f);
                }
        if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            if (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit)) {
                PolicySet(unit, mem, buf, REDIRECTION_DESTINATIONf, redir_field);
                PolicySet(unit, mem, buf, DROP_ORIGINAL_PACKETf, 0x1);
            } else {
                PolicySet(unit, mem, buf, REDIRECTIONf, redir_field);
            }
            PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x5);
        } else {
            PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x1);
            PolicySet(unit, mem, buf, redir_field_type, redir_field);
        }
        break;
    case bcmFieldActionRedirectTrunk:    /* param0 = trunk ID */
        if (SOC_IS_TRIUMPH3(unit)) {
            shift_val = 9;
        } else {
            shift_val = SOC_IS_TR_VL(unit) ? 
                    ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? 9 : 7) : 8;
        }
        redir_field |= (0x40 << shift_val);  /* Trunk indicator. */
        redir_field |= fa->param[0];

        if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            if (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit)) {
                PolicySet(unit, mem, buf, REDIRECTION_DESTINATIONf, redir_field);
                PolicySet(unit, mem, buf, DROP_ORIGINAL_PACKETf, 0x1);
            } else {
                PolicySet(unit, mem, buf, REDIRECTIONf, redir_field);
            }
            PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x6);
        } else {
            PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x1);
            PolicySet(unit, mem, buf, REDIRECTIONf, redir_field);
        }
        break;
    case bcmFieldActionRedirectCancel:
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 2);
        break;
    case bcmFieldActionRedirectPbmp:
        PolicySet(unit, mem, buf, REDIRECTIONf, fa->hw_index);
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x3);
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT; 
        break;
    case bcmFieldActionEgressMask:
        PolicySet(unit, mem, buf, REDIRECTIONf, fa->hw_index);
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x4);
        break;
    case bcmFieldActionEgressPortsAdd:
        PolicySet(unit, mem, buf, REDIRECTIONf, fa->hw_index);
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x5);
        break;
#ifdef INCLUDE_L3
    case bcmFieldActionRedirectIpmc:
        if (soc_feature(unit, soc_feature_field_action_redirect_ipmc)) {
            if (_BCM_MULTICAST_IS_SET(fa->param[0])) {
                if (0 == _BCM_MULTICAST_IS_L3(fa->param[0])) {
                    return (BCM_E_PARAM);
                }
                redir_field = _BCM_MULTICAST_ID_GET(fa->param[0]);
            } else {
                redir_field = fa->param[0];
            }
        } else {
            redir_field = fa->hw_index;
        }
        /* Assign IPMC action to redirect profile index. */
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
            shift_val = 16;
        } else {
            shift_val = SOC_IS_TR_VL(unit) ? ((SOC_IS_TD_TT(unit)) ? 13 : 12) : 13;
        }

        PolicySet(unit, mem, buf, REDIRECTIONf,
                  (redir_field | (3 << shift_val)));
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x3);
        break;
#endif /* INCLUDE_L3 */
    case bcmFieldActionRedirectMcast:
        if (soc_feature(unit, soc_feature_field_action_redirect_ipmc)) {
            if (_BCM_MULTICAST_IS_SET(fa->param[0])) {
                if (0 == _BCM_MULTICAST_IS_L2(fa->param[0])) {
                    return (BCM_E_PARAM);
                }
                redir_field = _BCM_MULTICAST_ID_GET(fa->param[0]);
            } else {
                redir_field = fa->param[0];
            }
        } else {
            redir_field = fa->hw_index;
        }

        if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf,
                      (SOC_IS_TRIUMPH3(unit) ? REDIRECTION_DESTINATIONf : 
                       REDIRECTIONf), redir_field);
            PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x7);
        } else {
            /* Assign MCAST action to redirect profile index. */
            if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
                shift_val = 16;
            } else {
                shift_val = SOC_IS_TR_VL(unit) ? ((SOC_IS_TD_TT(unit)) ? 13 : 12) : 13;
            }
            PolicySet(unit, mem, buf, REDIRECTIONf,
                      (redir_field | (2 << shift_val)));
            PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x3);
        }
        break;
    case bcmFieldActionRedirectVlan:
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
            shift_val = 16;
        } else {
            shift_val = SOC_IS_TD_TT(unit) ? 13 : 12;
        }
        PolicySet(unit, mem, buf, REDIRECTIONf, (1 << shift_val));
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x3);
        break;
    case bcmFieldActionRedirectBcastPbmp:
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
            shift_val = 16;
        } else {
            shift_val = SOC_IS_TD_TT(unit) ? 13 : 12;
        }
        redir_field = fa->hw_index;
        PolicySet(unit, mem, buf, REDIRECTIONf,
                  (redir_field | (1 << 11) | (1 << shift_val)));
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x3);
        break;
    default:
        return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_action_params_check
 * Purpose:
 *     Check field action parameters.
 * Parameters:
 *     unit     - BCM device number
 *     f_ent    - Field entry structure.
 *     fa       - field action
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_action_params_check(int unit,_field_entry_t *f_ent,
                                   _field_action_t *fa)
{
    uint32   redir_field = 0;
    int      shift_val;
    soc_field_t fld;                    /* Policy table field info */
#if defined(INCLUDE_L3)
    uint32 flags;            /* L3 forwarding flags.    */
    int nh_ecmp_id;          /* Next hop/Ecmp group id. */
    int svp;
#endif /* INCLUDE_L3 */
    soc_mem_t mem;           /* Policy table memory id. */
    soc_mem_t tcam_mem;      /* Tcam memory id.         */
    int rv;                  /* Operation return value. */
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
    int cosq_new = 0;
#endif /* !BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
    int temp_param;
#endif

    if (NULL == f_ent || NULL == fa) {
        return (BCM_E_PARAM);
    }

    /* Resolve policy memory id. */
#if defined(BCM_TRIUMPH_SUPPORT)
    if (f_ent->group->stage_id == _BCM_FIELD_STAGE_EXTERNAL) {
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            rv = _bcm_field_tr3_external_policy_mem_get(unit, fa, &mem);
        } else
#endif
            {
                rv = _bcm_field_tr_external_policy_mem_get(unit, fa, &mem);
            }
        BCM_IF_ERROR_RETURN(rv);
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
        {
            rv = _field_fb_tcam_policy_mem_get(unit, f_ent->group->stage_id,
                                               &tcam_mem, &mem);
            BCM_IF_ERROR_RETURN(rv);
        }
    
    switch (fa->action) {
      case bcmFieldActionMultipathHash:
          PolicyCheck(unit, mem, ECMP_HASH_SELf, fa->param[0]);
          break;
      case bcmFieldActionCopyToCpu:
      case bcmFieldActionRpCopyToCpu:
      case bcmFieldActionYpCopyToCpu:
      case bcmFieldActionGpCopyToCpu:
      case bcmFieldActionTimeStampToCpu:
      case bcmFieldActionRpTimeStampToCpu:
      case bcmFieldActionYpTimeStampToCpu:
      case bcmFieldActionGpTimeStampToCpu:
          if (fa->param[0] != 0) {
              if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
                  PolicyCheck(unit, mem, VFP_MATCHED_RULEf, fa->param[1]);
              } else {
                  PolicyCheck(unit, mem, MATCHED_RULEf, fa->param[1]);
              }
          }
          break;
      case bcmFieldActionClassDestSet:
          PolicyCheck(unit, mem, VFP_CLASS_ID_Lf, fa->param[0]);
          break;

      case bcmFieldActionClassSourceSet:
          PolicyCheck(unit, mem, VFP_CLASS_ID_Hf, fa->param[0]);
          break;

      case bcmFieldActionVrfSet:
          if (soc_feature(unit, soc_feature_mpls)) {
              PolicyCheck(unit, mem, VFP_VRF_IDf, fa->param[0]);
          }  else {
              PolicyCheck(unit, mem, VFP_VRF_IDf, fa->param[0]);

          }
          break;

      case bcmFieldActionDropPrecedence:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, NEW_CNGf, fa->param[0]);
          } else {
              PolicyCheck(unit, mem, R_DROP_PRECEDENCEf, fa->param[0]);
              PolicyCheck(unit, mem, Y_DROP_PRECEDENCEf, fa->param[0]);
              PolicyCheck(unit, mem, G_DROP_PRECEDENCEf, fa->param[0]);
          }
          break;
      case bcmFieldActionPrioPktNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              fld = (soc_mem_field_valid(unit, mem, NEW_OUTER_DOT1Pf)
                     ? NEW_OUTER_DOT1Pf : NEW_PKT_PRIORITYf);
                
              PolicyCheck(unit, mem, fld, fa->param[0]);
          } else {
              PolicyCheck(unit, mem, R_NEW_PKT_PRIf, fa->param[0]);
              PolicyCheck(unit, mem, Y_NEW_PKT_PRIf, fa->param[0]);
              PolicyCheck(unit, mem, G_NEW_PKT_PRIf, fa->param[0]);
          }
          break;
      case bcmFieldActionEcnNew:
          if (SOC_IS_TRIDENT2(unit)) {
              PolicyCheck(unit, mem, R_NEW_ECNf, fa->param[0]);
              PolicyCheck(unit, mem, Y_NEW_ECNf, fa->param[0]);
              PolicyCheck(unit, mem, G_NEW_ECNf, fa->param[0]);
          } else {
              PolicyCheck(unit, mem, R_CHANGE_ECNf, fa->param[0]);
              PolicyCheck(unit, mem, Y_CHANGE_ECNf, fa->param[0]);
              PolicyCheck(unit, mem, G_CHANGE_ECNf, fa->param[0]);
          }
          break;

      case bcmFieldActionDscpNew:
          PolicyCheck(unit, mem, R_NEW_DSCPf, fa->param[0]);
          PolicyCheck(unit, mem, Y_NEW_DSCPf, fa->param[0]);
          if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, G_NEW_DSCPf, fa->param[0]);
          } else {
              PolicyCheck(unit, mem, G_NEW_DSCP_TOSf, fa->param[0]);
          }
          break;

      case bcmFieldActionCosQNew:
      case bcmFieldActionUcastCosQNew:
#if defined(BCM_TRIDENT2_SUPPORT)
          if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) &&
              SOC_IS_TD_TT(unit)) {
              rv = _bcm_td2_cosq_index_resolve(unit, fa->param[0], 0,
                       _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                       &cosq_new, NULL);
              BCM_IF_ERROR_RETURN(rv);

              PolicyCheck(unit, mem, R_COS_INT_PRIf, cosq_new);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, cosq_new);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, cosq_new);
          } else
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
          if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) &&
              SOC_IS_TD_TT(unit)) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                       _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                       &cosq_new, NULL);
              BCM_IF_ERROR_RETURN(rv);

              PolicyCheck(unit, mem, R_COS_INT_PRIf, cosq_new);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, cosq_new);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, cosq_new);
          } else
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
          if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) ||
              BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(fa->param[0]))
              && SOC_IS_KATANAX(unit)) {
              rv = _bcm_kt_cosq_index_resolve(unit, fa->param[0], 0,
                  _BCM_KT_COSQ_INDEX_STYLE_BUCKET, NULL,
                  &cosq_new, NULL);
              BCM_IF_ERROR_RETURN(rv);

              PolicyCheck(unit, mem, R_COS_INT_PRIf, cosq_new);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, cosq_new);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, cosq_new);
          } else
#endif /* BCM_KATANA_SUPPORT */
          {
              PolicyCheck(unit, mem, R_COS_INT_PRIf, fa->param[0]);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, fa->param[0]);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, fa->param[0]);
          }
          break;

#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
      case bcmFieldActionMcastCosQNew:
          if (SOC_IS_TD_TT(unit) &&
              BCM_GPORT_IS_MCAST_QUEUE_GROUP(fa->param[0])) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                       _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                       &cosq_new, NULL);
              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, R_COS_INT_PRIf, (cosq_new << 4));
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, (cosq_new << 4));
              PolicyCheck(unit, mem, G_COS_INT_PRIf, (cosq_new << 4));
          } else {
              PolicyCheck(unit, mem, R_COS_INT_PRIf, (fa->param[0] << 4));
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, (fa->param[0] << 4));
              PolicyCheck(unit, mem, G_COS_INT_PRIf, (fa->param[0] << 4));
          }
          break;
#endif /* !BCM_TRIDENT_SUPPORT */

      case bcmFieldActionVlanCosQNew:
          /* Add 8 to the value since VLAN shaping queues are 8..23 */
          PolicyCheck(unit, mem, R_COS_INT_PRIf, fa->param[0] + 8);
          PolicyCheck(unit, mem, Y_COS_INT_PRIf, fa->param[0] + 8);
          PolicyCheck(unit, mem, G_COS_INT_PRIf, fa->param[0] + 8);
          break;

      case bcmFieldActionPrioIntNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, NEW_INT_PRIORITYf,  fa->param[0]);
          } else {
              PolicyCheck(unit, mem, R_COS_INT_PRIf, (0xf & fa->param[0]));
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, (0xf & fa->param[0]));
              PolicyCheck(unit, mem, G_COS_INT_PRIf, (0xf & fa->param[0]));
          }
          break;

      case bcmFieldActionPrioPktAndIntNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
#ifdef BCM_TRIDENT_SUPPORT
              if (SOC_IS_TD_TT(unit)
                  || SOC_IS_KATANAX(unit)
                  || SOC_IS_TRIUMPH3(unit)) {
                  PolicyCheck(unit, mem, NEW_OUTER_DOT1Pf, fa->param[0]);
              } else
#endif /* !BCM_TRIDENT_SUPPORT */
              {
                  PolicyCheck(unit, mem, NEW_PKT_PRIORITYf, fa->param[0]);
              }
              PolicyCheck(unit, mem, NEW_INT_PRIORITYf,  fa->param[0]);
          } else {
              PolicyCheck(unit, mem, R_NEW_PKT_PRIf, fa->param[0]);
              PolicyCheck(unit, mem, R_COS_INT_PRIf, (0xf & fa->param[0]));
              PolicyCheck(unit, mem, Y_NEW_PKT_PRIf, fa->param[0]);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, (0xf & fa->param[0]));
              PolicyCheck(unit, mem, G_NEW_PKT_PRIf, fa->param[0]);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, (0xf & fa->param[0]));
          }
          break;

      case bcmFieldActionCosQCpuNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, CPU_COSf, fa->param[0]);
          } else if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, NEW_CPU_COSf, fa->param[0]);
          } else {
              PolicyCheck(unit, mem, CPU_COSf, fa->param[0]);
          }
          break;

      case bcmFieldActionOuterVlanPrioNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
#ifdef BCM_TRIDENT_SUPPORT
              if (SOC_IS_TD_TT(unit)
                  || SOC_IS_KATANAX(unit)
                  || SOC_IS_TRIUMPH3(unit)) {
                  PolicyCheck(unit, mem, NEW_OUTER_DOT1Pf, fa->param[0]);
              } 
#endif /* !BCM_TRIDENT_SUPPORT */
          } else {
          PolicyCheck(unit, mem, R_NEW_DOT1Pf, fa->param[0]);
          PolicyCheck(unit, mem, Y_NEW_DOT1Pf, fa->param[0]);
          PolicyCheck(unit, mem, G_NEW_DOT1Pf, fa->param[0]);
          }
          break;

      case bcmFieldActionInnerVlanPrioNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
#ifdef BCM_TRIDENT_SUPPORT
              if (SOC_IS_TD_TT(unit)
                  || SOC_IS_KATANAX(unit)
                  || SOC_IS_TRIUMPH3(unit)) {
                  PolicyCheck(unit, mem, NEW_INNER_DOT1Pf, fa->param[0]);
              }
#endif /* !BCM_TRIDENT_SUPPORT */
          } else {
          PolicyCheck(unit, mem, R_NEW_INNER_PRIf, fa->param[0]);
          PolicyCheck(unit, mem, Y_NEW_INNER_PRIf, fa->param[0]);
          PolicyCheck(unit, mem, G_NEW_INNER_PRIf, fa->param[0]);
          } 
          break;

      case bcmFieldActionOuterVlanCfiNew:
#ifdef BCM_TRIDENT_SUPPORT
        if ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) || 
             SOC_IS_TRIUMPH3(unit)) 
              && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, NEW_OUTER_CFIf, fa->param[0]);
          } else 
#endif
          {
              PolicyCheck(unit, mem, R_NEW_OUTER_CFIf, fa->param[0]);
              PolicyCheck(unit, mem, Y_NEW_OUTER_CFIf, fa->param[0]);
              PolicyCheck(unit, mem, G_NEW_OUTER_CFIf, fa->param[0]);
          }
          break;

      case bcmFieldActionInnerVlanCfiNew:
#ifdef BCM_TRIDENT_SUPPORT
        if ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) || 
             SOC_IS_TRIUMPH3(unit))   
              && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, NEW_INNER_CFIf, fa->param[0]);
          } else 
#endif
          {
              PolicyCheck(unit, mem, R_NEW_INNER_CFIf, fa->param[0]);
              PolicyCheck(unit, mem, Y_NEW_INNER_CFIf, fa->param[0]);
              PolicyCheck(unit, mem, G_NEW_INNER_CFIf, fa->param[0]);
          }
          break;

      case bcmFieldActionInnerVlanAdd:
          PolicyCheck(unit, mem, NEW_INNER_VLANf, fa->param[0]);
          break;

      case bcmFieldActionOuterVlanAdd:
          PolicyCheck(unit, mem, NEW_OUTER_VLANf, fa->param[0]);
          break;

      case bcmFieldActionOuterVlanLookup:
          PolicyCheck(unit, mem, NEW_OUTER_VLANf, fa->param[0]);
          break;

      case bcmFieldActionInnerVlanNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, NEW_INNER_VLANf, fa->param[0]);
          } else if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, PID_NEW_INNER_VIDf, fa->param[0]);
          }
          break;

      case bcmFieldActionOuterVlanNew:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, NEW_OUTER_VLANf, fa->param[0]);
          } else if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, PID_NEW_OUTER_VIDf, fa->param[0]);
          } else {
              if (SOC_IS_TRIUMPH2(unit) || SOC_IS_TD_TT(unit)
                  || SOC_IS_KATANAX(unit) || SOC_IS_APOLLO(unit)
                  || SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
                  PolicyCheck(unit, EGR_L3_INTFm, VIDf, fa->param[0]);
              } else {
                  PolicyCheck(unit, EGR_L3_NEXT_HOPm, INTF_NUMf,
                      fa->param[0]);
              }
          }
          break;

      case bcmFieldActionMirrorIngress:
      case bcmFieldActionMirrorEgress:
          rv = _bcm_field_action_dest_check(unit, fa);
          BCM_IF_ERROR_RETURN(rv);
          break;
      case bcmFieldActionOffloadRedirect:
      case bcmFieldActionRedirect:
          rv = _bcm_field_action_dest_check(unit, fa);
          BCM_IF_ERROR_RETURN(rv);
#ifdef BCM_TRIDENT_SUPPORT
          if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
              if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
                  if (BCM_GPORT_IS_MPLS_PORT(fa->param[1]) ||
                      BCM_GPORT_IS_MIM_PORT(fa->param[1]) ||
                      BCM_GPORT_IS_NIV_PORT(fa->param[1]) ||
                      BCM_GPORT_IS_TRILL_PORT(fa->param[1]) ||
                      BCM_GPORT_IS_L2GRE_PORT(fa->param[1])) {
                      return (BCM_E_PARAM);
                  }
              } else {
                  if (SOC_IS_TRIUMPH3(unit)) {
                      if (BCM_GPORT_IS_MPLS_PORT(fa->param[1])) {
                          redir_field = BCM_GPORT_MPLS_PORT_ID_GET((int)fa->param[1]);
                          redir_field |= (6 << 18);
                      } else if(BCM_GPORT_IS_MIM_PORT(fa->param[1])) {
                          redir_field = BCM_GPORT_MIM_PORT_ID_GET((int)fa->param[1]);
                          redir_field |= (6 << 18);
                      } else if (BCM_GPORT_IS_NIV_PORT(fa->param[1])) {
                          redir_field = BCM_GPORT_NIV_PORT_ID_GET((int)fa->param[1]);
                          redir_field |= (6 << 18);
                      } else if (BCM_GPORT_IS_TRILL_PORT(fa->param[1])) {
                          redir_field = BCM_GPORT_TRILL_PORT_ID_GET((int)fa->param[1]);
                          redir_field |= (6 << 18);
                      } else if (BCM_GPORT_IS_L2GRE_PORT(fa->param[1])) {
                          redir_field = BCM_GPORT_L2GRE_PORT_ID_GET((int)fa->param[1]);
                          redir_field |= (6 << 18);
                      } else {
                          redir_field = ((fa->param[0] & 0xff) << 7);
                          redir_field |= (fa->param[1] & 0x7f);  
                      }                                          
                  } else if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                      if (BCM_GPORT_IS_MPLS_PORT(fa->param[1])) {
                          redir_field = BCM_GPORT_MPLS_PORT_ID_GET((int)fa->param[1]);
                          redir_field |= (3 << 17);              
                      } else if (BCM_GPORT_IS_MIM_PORT(fa->param[1])) {
                          redir_field = BCM_GPORT_MIM_PORT_ID_GET((int)fa->param[1]);
                          redir_field |= (3 << 17);              
                      } else {                                   
                          redir_field = ((fa->param[0] & 0xff) << 7);
                          redir_field |= (fa->param[1] & 0x7f);  
                      }                                          
                  }                                              
              }
          } else
#endif /* BCM_TRIDENT_SUPPORT */
          {
              redir_field = ((fa->param[0] & 0x7f) << 6);
              redir_field |= (fa->param[1] & 0x3f);
          }
          if (SOC_IS_TRIUMPH3(unit) &&
              (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id)) {
              PolicyCheck(unit, mem, REDIRECTION_DESTINATIONf, redir_field);
          }
          else {
              PolicyCheck(unit, mem, REDIRECTIONf, redir_field);
          }
          break;
      case bcmFieldActionRedirectTrunk:    /* param0 = trunk ID */
          shift_val = SOC_IS_TR_VL(unit) ? 
                  ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? 9 : 7) : 8;
          redir_field |= (0x40 << shift_val);  /* Trunk indicator. */
          redir_field |= fa->param[0];

          if (SOC_IS_TRIUMPH3(unit) &&
              (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id)) {
              PolicyCheck(unit, mem, REDIRECTION_DESTINATIONf, redir_field);
          }
          else {
              PolicyCheck(unit, mem, REDIRECTIONf, redir_field);
          }
          break;
#ifdef INCLUDE_L3
      case bcmFieldActionRedirectIpmc:
          if (soc_feature(unit, soc_feature_field_action_redirect_ipmc)) {
              if (_BCM_MULTICAST_IS_SET(fa->param[0])) {
                  if (0 == _BCM_MULTICAST_IS_L3(fa->param[0])) {
                      return (BCM_E_PARAM);
                  }
                  redir_field = _BCM_MULTICAST_ID_GET(fa->param[0]);
              } else {
                  redir_field = fa->param[0];
              }
          } else {
              redir_field = 0;
          }
          /* Assign IPMC action to redirect profile index. */
          shift_val = SOC_IS_TR_VL(unit) ? 
              ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? 13 : 12) : 13;
          PolicyCheck(unit, mem, REDIRECTIONf,
                         (redir_field | (3 << shift_val)));
          break;
#endif /* INCLUDE_L3 */
      case bcmFieldActionRedirectMcast:
          if (soc_feature(unit, soc_feature_field_action_redirect_ipmc)) {
              if (_BCM_MULTICAST_IS_SET(fa->param[0])) {
                  if (0 == _BCM_MULTICAST_IS_L2(fa->param[0])) {
                      return (BCM_E_PARAM);
                  }
                  redir_field = _BCM_MULTICAST_ID_GET(fa->param[0]);
              } else {
                  redir_field = fa->param[0];
              }
          } else {
              redir_field = 0;
          }

          if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
              PolicyCheck(unit, mem,
                          (SOC_IS_TRIUMPH3(unit) ? REDIRECTION_DESTINATIONf : 
                           REDIRECTIONf),
                          redir_field);
          } else {
              /* Assign MCAST action to redirect profile index. */
              shift_val = SOC_IS_TR_VL(unit) ? 
                  ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? 13 : 12) : 13;
              PolicyCheck(unit, mem, REDIRECTIONf,
                              (redir_field | (2 << shift_val)));
          }
          break;
      case bcmFieldActionOffloadClassSet:
          PolicyCheck(unit, mem, PPD3_CLASS_TAGf, fa->param[0]);
          break;
      case bcmFieldActionRpDropPrecedence:
          PolicyCheck(unit, mem, R_DROP_PRECEDENCEf, fa->param[0]);
          break;
      case bcmFieldActionRpPrioPktNew:
          PolicyCheck(unit, mem, R_NEW_PKT_PRIf, fa->param[0]);
          break;
      case bcmFieldActionRpEcnNew:
          if ((fa->param[0] <= 0) || (fa->param[0] > 3)) {
              return (BCM_E_PARAM);
          }
          break;
      case bcmFieldActionRpDscpNew:
          PolicyCheck(unit, mem, R_NEW_DSCPf, fa->param[0]);
          break;
      case bcmFieldActionRpCosQNew:
#if defined(BCM_TRIDENT_SUPPORT)
      case bcmFieldActionRpUcastCosQNew:
          if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) &&
              SOC_IS_TD_TT(unit)) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                   _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                   &cosq_new, NULL);

              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, R_COS_INT_PRIf, cosq_new);
          } else
#endif /*!BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
          if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) ||
              BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(fa->param[0]))
              && SOC_IS_KATANAX(unit)) {
              rv = _bcm_kt_cosq_index_resolve(unit, fa->param[0], 0,
                   _BCM_KT_COSQ_INDEX_STYLE_BUCKET, NULL,
                   &cosq_new, NULL);

              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, R_COS_INT_PRIf, cosq_new);
          } else
#endif /* !BCM_KATANA_SUPPORT*/
          {
              PolicyCheck(unit, mem, R_COS_INT_PRIf, fa->param[0]);
          }
          break;
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
      case bcmFieldActionRpMcastCosQNew:
          if (SOC_IS_TD_TT(unit) &&
              BCM_GPORT_IS_MCAST_QUEUE_GROUP(fa->param[0])) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                       _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                       &cosq_new, NULL);
              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, R_COS_INT_PRIf, (cosq_new << 4));
          } else {
              PolicyCheck(unit, mem, R_COS_INT_PRIf, (fa->param[0] << 4));
          }
          break;
#endif /* !BCM_TRIDENT_SUPPORT */
      case bcmFieldActionRpVlanCosQNew:
          /* Add 8 to the value since VLAN shaping queues are 8..23 */
          PolicyCheck(unit, mem, R_COS_INT_PRIf, fa->param[0] + 8);
          break;
      case bcmFieldActionRpPrioPktAndIntNew:
          PolicyCheck(unit, mem, R_NEW_PKT_PRIf, fa->param[0]);
          PolicyCheck(unit, mem, R_COS_INT_PRIf, (0xf & fa->param[0]));
          break;
      case bcmFieldActionRpOuterVlanPrioNew:
          PolicyCheck(unit, mem, R_NEW_DOT1Pf, fa->param[0]);
          break;
      case bcmFieldActionRpInnerVlanPrioNew:
          PolicyCheck(unit, mem, R_NEW_INNER_PRIf, fa->param[0]);
          break;
      case bcmFieldActionRpOuterVlanCfiNew:
          PolicyCheck(unit, mem, R_NEW_OUTER_CFIf, fa->param[0]);
          break;
      case bcmFieldActionRpInnerVlanCfiNew:
          PolicyCheck(unit, mem, R_NEW_INNER_CFIf, fa->param[0]);
          break;
      case bcmFieldActionRpPrioIntNew:
          PolicyCheck(unit, mem, R_COS_INT_PRIf, (0xf & fa->param[0]));
          break;
      case bcmFieldActionYpDropPrecedence:
          PolicyCheck(unit, mem, Y_DROP_PRECEDENCEf, fa->param[0]);
          break;
      case bcmFieldActionYpPrioPktNew:
          PolicyCheck(unit, mem, Y_NEW_PKT_PRIf, fa->param[0]);
          break;
      case bcmFieldActionYpEcnNew:
          if ((fa->param[0] <= 0) || (fa->param[0] > 3)) {
              return (BCM_E_PARAM);
          }
          break;
      case bcmFieldActionYpDscpNew:
          PolicyCheck(unit, mem, Y_NEW_DSCPf, fa->param[0]);
          break;
      case bcmFieldActionYpCosQNew:
#if defined(BCM_TRIDENT_SUPPORT)
      case bcmFieldActionYpUcastCosQNew:
          if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) &&
              SOC_IS_TD_TT(unit)) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                   _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                   &cosq_new, NULL);

              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, cosq_new);
          } else
#endif  /*!BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
          if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) ||
              BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(fa->param[0]))
              && SOC_IS_KATANAX(unit)) {
              rv = _bcm_kt_cosq_index_resolve(unit, fa->param[0], 0,
                   _BCM_KT_COSQ_INDEX_STYLE_BUCKET, NULL,
                   &cosq_new, NULL);

              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, cosq_new);
          } else
#endif /* !BCM_KATANA_SUPPORT*/
          {
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, fa->param[0]);
          }
          break;
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
      case bcmFieldActionYpMcastCosQNew:
          if (SOC_IS_TD_TT(unit) &&
              BCM_GPORT_IS_MCAST_QUEUE_GROUP(fa->param[0])) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                       _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                       &cosq_new, NULL);
              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, (cosq_new << 4));
          } else
          {
              PolicyCheck(unit, mem, Y_COS_INT_PRIf, (fa->param[0] << 4));
          }
          break;
#endif /* !BCM_TRIDENT_SUPPORT */
      case bcmFieldActionYpVlanCosQNew:
          /* Add 8 to the value since VLAN shaping queues are 8..23 */
          PolicyCheck(unit, mem, Y_COS_INT_PRIf, fa->param[0] + 8);
          break;
      case bcmFieldActionYpPrioPktAndIntNew:
          PolicyCheck(unit, mem, Y_NEW_PKT_PRIf, fa->param[0]);
          PolicyCheck(unit, mem, Y_COS_INT_PRIf, (0xf & fa->param[0]));
          break;
      case bcmFieldActionYpOuterVlanPrioNew:
          PolicyCheck(unit, mem, Y_NEW_DOT1Pf, fa->param[0]);
          break;
      case bcmFieldActionYpInnerVlanPrioNew:
          PolicyCheck(unit, mem, Y_NEW_INNER_PRIf, fa->param[0]);
          break;
      case bcmFieldActionYpOuterVlanCfiNew:
          PolicyCheck(unit, mem, Y_NEW_OUTER_CFIf, fa->param[0]);
          break;
      case bcmFieldActionYpInnerVlanCfiNew:
          PolicyCheck(unit, mem, Y_NEW_INNER_CFIf, fa->param[0]);
          break;
      case bcmFieldActionYpPrioIntNew:
          PolicyCheck(unit, mem, Y_COS_INT_PRIf, (0xf & fa->param[0]));
          break;
      case bcmFieldActionGpDropPrecedence:
          PolicyCheck(unit, mem, G_DROP_PRECEDENCEf, fa->param[0]);
          break;
      case bcmFieldActionGpPrioPktNew:
          PolicyCheck(unit, mem, G_NEW_PKT_PRIf, fa->param[0]);
          break;
      case bcmFieldActionGpEcnNew:
          if ((fa->param[0] <= 0) || (fa->param[0] > 3)) {
              return (BCM_E_PARAM);
          }
          break;
      case bcmFieldActionGpDscpNew:
          if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, G_NEW_DSCPf, fa->param[0]);
          } else {
              PolicyCheck(unit, mem, G_NEW_DSCP_TOSf, fa->param[0]);
          }
          break;
      case bcmFieldActionGpTosPrecedenceNew:
          PolicyCheck(unit, mem, G_NEW_DSCP_TOSf, fa->param[0]);
          break;
      case bcmFieldActionGpCosQNew:
#if defined(BCM_TRIDENT_SUPPORT)
      case bcmFieldActionGpUcastCosQNew:
          if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) &&
              SOC_IS_TD_TT(unit)) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                   _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                   &cosq_new, NULL);

              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, cosq_new);
          } else
#endif /*!BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
          if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) ||
              BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(fa->param[0]))
              && SOC_IS_KATANAX(unit)) {
              rv = _bcm_kt_cosq_index_resolve(unit, fa->param[0], 0,
                   _BCM_KT_COSQ_INDEX_STYLE_BUCKET, NULL,
                   &cosq_new, NULL);
 
              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, cosq_new);
          } else
#endif /* !BCM_KATANA_SUPPORT*/
          {
              PolicyCheck(unit, mem, G_COS_INT_PRIf, fa->param[0]);
          }
          break;

#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
      case bcmFieldActionGpMcastCosQNew:
          if (SOC_IS_TD_TT(unit) &&
              BCM_GPORT_IS_MCAST_QUEUE_GROUP(fa->param[0])) {
              rv = _bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                       _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                       &cosq_new, NULL);
              BCM_IF_ERROR_RETURN(rv);
              PolicyCheck(unit, mem, G_COS_INT_PRIf, (cosq_new << 4));
          } else
          {
              PolicyCheck(unit, mem, G_COS_INT_PRIf, (fa->param[0] << 4));
          }
          break;
#endif /* !BCM_TRIDENT_SUPPORT */
      case bcmFieldActionGpVlanCosQNew:
          /* Add 8 to the value since VLAN shaping queues are 8..23 */
          PolicyCheck(unit, mem, G_COS_INT_PRIf, fa->param[0] + 8);
          break;
      case bcmFieldActionGpPrioPktAndIntNew:
          PolicyCheck(unit, mem, G_NEW_PKT_PRIf, fa->param[0]);
          PolicyCheck(unit, mem, G_COS_INT_PRIf, (0xf & fa->param[0]));
          break;
      case bcmFieldActionGpOuterVlanPrioNew:
          PolicyCheck(unit, mem, G_NEW_DOT1Pf, fa->param[0]);
          break;
      case bcmFieldActionGpInnerVlanPrioNew:
          PolicyCheck(unit, mem, G_NEW_INNER_PRIf, fa->param[0]);
          break;
      case bcmFieldActionGpOuterVlanCfiNew:
          PolicyCheck(unit, mem, G_NEW_OUTER_CFIf, fa->param[0]);
          break;
      case bcmFieldActionGpInnerVlanCfiNew:
          PolicyCheck(unit, mem, G_NEW_INNER_CFIf, fa->param[0]);
          break;
      case bcmFieldActionGpPrioIntNew:
          PolicyCheck(unit, mem, G_COS_INT_PRIf, (0xf & fa->param[0]));
          break;
      case bcmFieldActionAddClassTag:
          /* 
           * The NEXT_HOP_INDEXf is 14-bits wide in FP_POLICY_TABLEm view.
           * But only 13-bits are valid for this action 
           */
#ifdef BCM_TRIUMPH_SUPPORT         
          if (SOC_IS_TR_VL(unit)) {
              if (!(fa->param[0] <= ((1 << 13) - 1))) {
                  return BCM_E_PARAM;
              }
          } else
#endif /* (BCM_TRIUMPH_SUPPORT) */
          {
              PolicyCheck(unit, mem, NEXT_HOP_INDEXf, fa->param[0]);
          }
          break;
      case bcmFieldActionFabricQueue:
#if defined(BCM_KATANA_SUPPORT)
          /* For Katana if param1 flags are set then param0 should be
          * ucast/ucast_subscriber group cosq gport */
          if (SOC_IS_KATANAX(unit)) {
              if ((fa->param[1] & BCM_FABRIC_QUEUE_DEST_OFFSET) ||
                  (fa->param[1] & BCM_FABRIC_QUEUE_CUSTOMER)) {
                  if (BCM_GPORT_IS_SET(fa->param[0])) {
                      if(BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(fa->param[0])
                          || BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0])){
                          if (fa->param[1] & BCM_FABRIC_QUEUE_DEST_OFFSET) {
                              temp_param = fa->param[1] & _BCM_QOS_MAP_MASK;
                              if (((temp_param >> _BCM_QOS_MAP_TYPE_SHIFT) !=
                                  _BCM_QOS_MAP_TYPE_ING_QUEUE_OFFSET_MAP_TABLE)
                                  || ((temp_param & _BCM_QOS_MAP_OFFSET_MASK) >
                                       _BCM_QOS_MAP_ING_QUEUE_OFFSET_MAX)) {
                                  return BCM_E_PARAM;
                              }
                          }
                      } else {
                          return BCM_E_PARAM;
                      }
                  } else {
                      return BCM_E_PARAM;
                  }
              } else {
                  /* maintail backward compatibility */
                  if ((fa->param[0] & BCM_FABRIC_QUEUE_QOS_BASE)  &&
                      (fa->param[0] & BCM_FABRIC_QUEUE_DEST_OFFSET)) {
                      return (BCM_E_PARAM);
                  }
        
                  if (fa->param[0] &
                       ~(BCM_FABRIC_QUEUE_CUSTOMER |
                         BCM_FABRIC_QUEUE_QOS_BASE |
                         BCM_FABRIC_QUEUE_DEST_OFFSET |
                         0xffff)) {
                      return (BCM_E_PARAM);
                  }
              }
          } else
#endif
          {
              if ((fa->param[0] & BCM_FABRIC_QUEUE_QOS_BASE)  &&
                  (fa->param[0] & BCM_FABRIC_QUEUE_DEST_OFFSET)) {
                  return (BCM_E_PARAM);
              }
    
              if (fa->param[0] &
                   ~(BCM_FABRIC_QUEUE_CUSTOMER |
                     BCM_FABRIC_QUEUE_QOS_BASE |
                     BCM_FABRIC_QUEUE_DEST_OFFSET |
                     0xffff)) {
                  return (BCM_E_PARAM);
              }
          }

          break;

#if defined(BCM_TRIDENT_SUPPORT)
        case bcmFieldActionCompressSrcIp6:
        case bcmFieldActionCompressDstIp6:
            if (SOC_IS_TD_TT(unit) &&
                (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id)) {
                PolicyCheck(unit, mem, IPV6_TO_IPV4_MAP_OFFSET_SETf,
                            fa->param[0]);
            }
            break;
#endif /* !BCM_TRIDENT_SUPPORT */

#ifdef INCLUDE_L3
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
      case bcmFieldActionIncomingMplsPortSet:
          if (!BCM_GPORT_IS_MPLS_PORT(fa->param[0])) {
              return BCM_E_PARAM;
          }
#endif
          break;
      case bcmFieldActionIngressGportSet:

          if (BCM_GPORT_IS_MPLS_PORT(fa->param[0]) &&
              soc_feature(unit, soc_feature_mpls)) {
              svp = BCM_GPORT_MPLS_PORT_ID_GET((bcm_gport_t)fa->param[0]);
          } else if (BCM_GPORT_IS_MIM_PORT(fa->param[0]) &&
                     soc_feature(unit, soc_feature_mim)) {
              svp = BCM_GPORT_MIM_PORT_ID_GET((bcm_gport_t)fa->param[0]);
          } else if (BCM_GPORT_IS_WLAN_PORT(fa->param[0]) &&
                     soc_feature(unit, soc_feature_wlan)) {
              svp = BCM_GPORT_WLAN_PORT_ID_GET((bcm_gport_t)fa->param[0]);
          } else if (BCM_GPORT_IS_TRILL_PORT(fa->param[0]) &&
                     soc_feature(unit, soc_feature_trill)) {
              svp = BCM_GPORT_TRILL_PORT_ID_GET((bcm_gport_t)fa->param[0]);
          } else if (BCM_GPORT_IS_NIV_PORT(fa->param[0]) &&
                     soc_feature(unit, soc_feature_niv)) {
              svp = BCM_GPORT_NIV_PORT_ID_GET((bcm_gport_t)fa->param[0]);
          } else {
              return BCM_E_PARAM;
          }
          PolicyCheck(unit, mem, SVPf, svp);

          break;
      case bcmFieldActionL3IngressSet:               

          if (0 == SHR_BITGET(BCM_XGS3_L3_ING_IF_INUSE(unit), fa->param[0])) {
              return BCM_E_PARAM;
          }
          PolicyCheck(unit, mem, L3_IIFf, fa->param[0]);
          break;
      case bcmFieldActionL3ChangeVlan:
      case bcmFieldActionL3ChangeMacDa:
      case bcmFieldActionL3Switch:
          rv = _bcm_field_policy_set_l3_nh_resolve(unit,  fa->param[0],
                                                   &flags, &nh_ecmp_id);
          BCM_IF_ERROR_RETURN(rv);
          if (flags & BCM_L3_MULTIPATH) {
              PolicyCheck(unit, mem, ECMP_PTRf, nh_ecmp_id);
          } else {
              PolicyCheck(unit, mem, NEXT_HOP_INDEXf, nh_ecmp_id);
          }
          break;
#endif /* INCLUDE_L3 */
      case bcmFieldActionOamUpMep:
          PolicyCheck(unit, mem, OAM_UP_MEPf, fa->param[0]);
          break;
      case bcmFieldActionOamTx:
          PolicyCheck(unit, mem, OAM_TXf, fa->param[0]);
          break;
      case bcmFieldActionOamLmepMdl:
          PolicyCheck(unit, mem, OAM_LMEP_MDLf, fa->param[0]);
          break;
      case bcmFieldActionOamServicePriMappingPtr:
          PolicyCheck(unit, mem, OAM_SERVICE_PRI_MAPPING_PTRf, fa->param[0]);
          break;
      case bcmFieldActionOamLmBasePtr:
          PolicyCheck(unit, mem, OAM_LM_BASE_PTRf, fa->param[0]);
          break;
      case bcmFieldActionOamDmEnable:
          PolicyCheck(unit, mem, OAM_DM_ENf, fa->param[0]);
          break;
      case bcmFieldActionOamLmEnable:
          PolicyCheck(unit, mem, OAM_LM_ENf, fa->param[0]);
          break;
      case bcmFieldActionOamLmepEnable:
          PolicyCheck(unit, mem, OAM_LMEP_ENf, fa->param[0]);
          break;
      case bcmFieldActionOamPbbteLookupEnable:
          if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
              PolicyCheck(unit, mem, OAM_PBBTE_LOOKUP_ENABLEf, fa->param[0]);
          }
          break;
      case bcmFieldActionServicePoolIdNew:
          if (0 != (fa->param[0] >> 2)) {
             return BCM_E_PARAM;
          }
          break;
#ifdef INCLUDE_L3
      case bcmFieldActionRedirectEgrNextHop:
            /* Get next hop info from Egress Object ID param */
          BCM_IF_ERROR_RETURN(_bcm_field_policy_set_l3_nh_resolve(unit,
              fa->param[0], &flags, &nh_ecmp_id));

          if (flags & BCM_L3_MULTIPATH) {
              /* Param0 - ECMP next hop */
              if (0 == soc_feature(unit, soc_feature_field_action_redirect_ecmp)) {
                  return (BCM_E_PARAM);
              }
              PolicyCheck(unit, mem, REDIRECTIONf, (nh_ecmp_id | (0x3 << 16)));
          } else {
              /* Param0 - Regular next hop */
              if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                  PolicyCheck(unit, mem, REDIRECTIONf, (nh_ecmp_id | (0x2 << 16)));
              } else {
                  PolicyCheck(unit, mem, REDIRECTIONf, (nh_ecmp_id | (0x1 << 14)));
              }
          }
          break;
#endif
      default:
          return BCM_E_NONE;
    }
    return (BCM_E_NONE);
}

int
_bcm_field_trx_vlan_format_qualify_is_double_tagged(unsigned data, unsigned mask)
{
    if ((mask & (BCM_FIELD_VLAN_FORMAT_OUTER_TAGGED
                 | BCM_FIELD_VLAN_FORMAT_OUTER_TAGGED_VID_ZERO
                 )
         ) == 0
        || (mask & (BCM_FIELD_VLAN_FORMAT_INNER_TAGGED
                    | BCM_FIELD_VLAN_FORMAT_INNER_TAGGED_VID_ZERO
                    )
            ) == 0
        || (data & (BCM_FIELD_VLAN_FORMAT_OUTER_TAGGED
                    | BCM_FIELD_VLAN_FORMAT_OUTER_TAGGED_VID_ZERO
                    )
            ) == 0
        || (data & (BCM_FIELD_VLAN_FORMAT_INNER_TAGGED
                    | BCM_FIELD_VLAN_FORMAT_INNER_TAGGED_VID_ZERO
                    )
            ) == 0
        ) {
        return (FALSE);
    }

    return (TRUE);
}

/*
 * Function:
 *     _bcm_field_trx_action_depends_check
 * Purpose:
 *     Check field action parameters.
 * Parameters:
 *     unit     - BCM device number
 *     f_ent    - Field entry structure.
 *     fa       - field action
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_action_depends_check(int             unit,
                                    _field_entry_t  *f_ent,
                                    _field_action_t *fa
                                    )
{
    switch (fa->action) {
    case bcmFieldActionOuterVlanCopyInner:
    case bcmFieldActionOuterVlanPrioCopyInner:
    case bcmFieldActionOuterVlanCfiCopyInner:
    case bcmFieldActionInnerVlanCopyOuter:
    case bcmFieldActionInnerVlanPrioCopyOuter:
    case bcmFieldActionInnerVlanCfiCopyOuter:
        /* These actions are valid only if:
           - the group's qset includes bcmFieldQualifyVlanFormat, and
           - the entry qualifies bcmFieldQualifyVlanFormat as "double-tagged"
        */

        {
            int   rv;
            uint8 data, mask;

            if (!BCM_FIELD_QSET_TEST(f_ent->group->qset,
                                     bcmFieldQualifyVlanFormat
                                     )
                ) {
                return (BCM_E_CONFIG);
            }

            rv = _field_qualify_VlanFormat_get(unit,
                                               f_ent->eid,
                                               bcmFieldQualifyVlanFormat,
                                               &data,
                                               &mask
                                               );
            if (BCM_FAILURE(rv)) {
                return (rv);
            }

            if (!_bcm_field_trx_vlan_format_qualify_is_double_tagged(data, mask)) {
                return (BCM_E_CONFIG);
            }
        }

        break;

    default:
        break;
    }

    return (BCM_E_NONE);
}

#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
/*
 * Function:
 *     _bcm_field_trx_ucast_mcast_action_update
 * Purpose:
 *     Modify Unicast and Multicast queue values
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     mem      - (IN) Policy table memory.
 *     f_ent    - (IN) Field entry structure to get policy info from.
 *     tcam_idx - (IN) Field policy table entry index.
 *     fa       - (IN) Field action.
 *     buf      - (OUT) Field Policy table entry.
 * Returns:
 *     BCM_E_XXX
 */
STATIC
int _bcm_field_trx_ucast_mcast_action_update(int unit, soc_mem_t mem,
                                            _field_entry_t *f_ent,
                                            int tcam_idx, _field_action_t *fa,
                                            uint32 *buf)
{
    int ucast_cosq_new = BCM_COS_INVALID;   /* Unicast new Queue value */
    int mcast_cosq_new = BCM_COS_INVALID;   /* Muliticast new Queue value */
    int ucosq = BCM_COS_INVALID;            /* Current unicast queue value */
    int mcosq = BCM_COS_INVALID;            /* Current mcast queue value */
    _field_action_t *f_ent_act = NULL;      /* Field entry action */
    uint8 mcast_mode_set = 0;               /* Multicast action is set */
    uint8 ucast_mode_set = 0;               /* Unicast action is set */

    /* Check and return error for invalid cases */
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    if (!(SOC_IS_TD_TT(unit))) {
        return BCM_E_UNAVAIL;
    }

    FP_VVERB(("FP(unit %d) vverb: _bcm_field_trx_ucast_mcast_action_update ", unit));
    FP_VVERB(("eid=%d, tcam_idx=0x%x\n ", f_ent->eid, tcam_idx));

    /* Get Queue value for Unicast actions */
    if ((bcmFieldActionCosQNew == fa->action) ||
        (bcmFieldActionRpCosQNew == fa->action)||
        (bcmFieldActionYpCosQNew == fa->action)||
        (bcmFieldActionGpCosQNew == fa->action)||
        (bcmFieldActionUcastCosQNew == fa->action)||
        (bcmFieldActionRpUcastCosQNew == fa->action) ||
        (bcmFieldActionYpUcastCosQNew == fa->action) ||
        (bcmFieldActionGpUcastCosQNew == fa->action)) {
        /*
         * Check if queue parameter value is of GPORT type.
         */
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0])) {
            BCM_IF_ERROR_RETURN
                (_bcm_td_cosq_index_resolve(unit, fa->param[0], 0,
                    _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                    &ucast_cosq_new, NULL));
        } else {
            ucast_cosq_new = fa->param[0];
        }
    }

    /* Get Queue value for Multicast actions */
    if ((bcmFieldActionMcastCosQNew == fa->action) ||
        (bcmFieldActionRpMcastCosQNew == fa->action) ||
        (bcmFieldActionYpMcastCosQNew == fa->action) ||
        (bcmFieldActionGpMcastCosQNew == fa->action)) {
        /*
         * Check if queue parameter value is of GPORT type.
         */
        if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(fa->param[0])) {
            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                fa->param[0], 0, _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                &mcast_cosq_new, NULL));
        } else {
            mcast_cosq_new = fa->param[0];
        }
    }

    switch (fa->action) {
        case bcmFieldActionCosQNew:
            /* Set mode info for all colors */
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
            /* Set queue info for all colors */
            PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                (_FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new,
                    ucast_cosq_new)));
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                (_FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new,
                    ucast_cosq_new)));
            PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                (_FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new,
                    ucast_cosq_new)));
            break;
        case bcmFieldActionRpCosQNew:
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                (_FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new,
                    ucast_cosq_new)));
            break;
        case bcmFieldActionYpCosQNew:
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                (_FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new,
                    ucast_cosq_new)));
            break;
        case bcmFieldActionGpCosQNew:
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                (_FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new,
                    ucast_cosq_new)));
            break;
        case bcmFieldActionUcastCosQNew:
            /* Check if corresponding multicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionMcastCosQNew == f_ent_act->action) {
                    /* Get current Mcast queue value */
                    mcast_mode_set = 1;
                    if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(f_ent_act->param[0])) {
                        if (SOC_IS_TD_TT(unit)) {
                            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                                f_ent_act->param[0], 0,
                                _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                                &mcosq, NULL));
                        }
                    } else {
                        mcosq = f_ent_act->param[0];
                    }
                }
            }
            if (0 == mcast_mode_set) {
                /* Set Unicast packet queue mode */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_NEW_MODE);

                /* Set Unicast packet queue value */
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_SET(ucast_cosq_new));
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_SET(ucast_cosq_new));
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_SET(ucast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new, mcosq));
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new, mcosq));
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new, mcosq));
            }
            break;
        case bcmFieldActionMcastCosQNew:
            /* Check if corresponding unicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionUcastCosQNew == f_ent_act->action) {
                    /* Get current Mcast queue value */
                    ucast_mode_set = 1;
                    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(f_ent_act->param[0])) {
                        if (SOC_IS_TD_TT(unit)) {
                            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                            f_ent_act->param[0], 0,
                                _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                                &ucosq, NULL));
                        }
                    } else {
                        ucosq = f_ent_act->param[0];
                    }
                }
            }
            if (0 == ucast_mode_set) {
                /* Change Unicast packet queue mode */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_NEW_MODE);

                /* Change Unicast packet queue value */
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_SET(mcast_cosq_new));
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_SET(mcast_cosq_new));
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_SET(mcast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);

                /* Set Ucast and Mcast queue values */
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucosq, mcast_cosq_new));
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucosq, mcast_cosq_new));
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucosq, mcast_cosq_new));
            }
            break;
        case bcmFieldActionRpUcastCosQNew:
            /* Check if corresponding multicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionRpMcastCosQNew == f_ent_act->action) {
                    /* Get current Mcast queue value */
                    mcast_mode_set = 1;
                    if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(f_ent_act->param[0])) {
                        if (SOC_IS_TD_TT(unit)) {
                            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                                f_ent_act->param[0], 0,
                                _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                                &mcosq, NULL));
                        }
                    } else {
                        mcosq = f_ent_act->param[0];
                    }
                }
            }

            if (0 == mcast_mode_set) {
                /* Set Unicast packet queue mode */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_NEW_MODE);
                /* Set Unicast packet queue value */
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_SET(ucast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);

                /* Set Ucast and Mcast queue values */
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new, mcosq));
            }
            break;
        case bcmFieldActionRpMcastCosQNew:
            /* Check if corresponding unicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionRpUcastCosQNew == f_ent_act->action) {
                    /* Get current Mcast queue value */
                    ucast_mode_set = 1;
                    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(f_ent_act->param[0])) {
                        if (SOC_IS_TD_TT(unit)) {
                            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                                f_ent_act->param[0], 0,
                                _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                                &ucosq, NULL));
                        }
                    } else {
                        ucosq = f_ent_act->param[0];
                    }
                }
            }
            if (0 == ucast_mode_set) {
                /* Change Unicast packet queue mode */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_NEW_MODE);

                /* Change Unicast packet queue value */
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_SET(mcast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);

                /* Set Ucast and Mcast queue values */
                PolicySet(unit, mem, buf, R_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucosq, mcast_cosq_new));
            }
            break;
        case bcmFieldActionYpUcastCosQNew:
            /* Check if corresponding multicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionYpMcastCosQNew == f_ent_act->action) {
                    /* Get current Mcast queue value */
                    mcast_mode_set = 1;
                    mcosq = f_ent_act->param[0];
                }
            }
            if (0 == mcast_mode_set) {
                /* Set Unicast packet queue mode */
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_NEW_MODE);
                /* Set Unicast packet queue value */
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_SET(ucast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);

                /* Set Ucast and Mcast queue values */
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new, mcosq));
            }
            break;
        case bcmFieldActionYpMcastCosQNew:
            /* Check if corresponding unicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionYpUcastCosQNew == f_ent_act->param[0]) {
                    /* Get current unicast queue value */
                    ucast_mode_set = 1;
                    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(f_ent_act->param[0])) {
                        if (SOC_IS_TD_TT(unit)) {
                            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                                f_ent_act->param[0], 0,
                                _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                                &ucosq, NULL));
                        }
                    } else {
                        ucosq = f_ent_act->param[0];
                    }
                }
            }
            if (0 == ucast_mode_set) {
                /* Change Unicast packet queue mode */
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_NEW_MODE);

                /* Change Unicast packet queue value */
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_SET(mcast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);

                /* Set Ucast and Mcast queue values */
                PolicySet(unit, mem, buf, Y_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucosq, mcast_cosq_new));

            }
            break;
        case bcmFieldActionGpUcastCosQNew:
            /* Check if corresponding multicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionGpMcastCosQNew == f_ent_act->action) {
                    /* Get current Mcast queue value */
                    mcast_mode_set = 1;
                    if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(f_ent_act->param[0])) {
                        if (SOC_IS_TD_TT(unit)) {
                            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                                f_ent_act->param[0], 0,
                                _BCM_TD_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL,
                                &mcosq, NULL));
                        }
                    } else {
                        mcosq = f_ent_act->param[0];
                    }
                }
            }
            if (0 == mcast_mode_set) {
                /* Set Unicast packet queue mode */
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_NEW_MODE);
                /* Set Unicast packet queue value */
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_UCAST_QUEUE_SET(ucast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);

                /* Set Ucast and Mcast queue values */
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucast_cosq_new, mcosq));
            }
            break;
        case bcmFieldActionGpMcastCosQNew:
            /* Check if corresponding unicast action is set for this entry */
            for (f_ent_act = f_ent->actions; fa != NULL; fa = fa->next) {
                if (bcmFieldActionGpUcastCosQNew == f_ent_act->param[0]) {
                    /* Get current unicast queue value */
                    ucast_mode_set = 1;
                    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(f_ent_act->param[0])) {
                        if (SOC_IS_TD_TT(unit)) {
                            BCM_IF_ERROR_RETURN(_bcm_td_cosq_index_resolve(unit,
                                f_ent_act->param[0], 0,
                                _BCM_TD_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL,
                                &ucosq, NULL));
                        }
                    } else {
                        ucosq = f_ent_act->param[0];
                    }
                }
            }
            if (0 == ucast_mode_set) {
                /* Change Unicast packet queue mode */
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_NEW_MODE);

                /* Change Unicast packet queue value */
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_MCAST_QUEUE_SET(mcast_cosq_new));
            } else {
                /* Set Ucast and Mcast queue modes */
                PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE);

                /* Set Ucast and Mcast queue values */
                PolicySet(unit, mem, buf, G_COS_INT_PRIf,
                    _FP_ACTION_UCAST_MCAST_QUEUE_SET(ucosq, mcast_cosq_new));
            }
            break;
        default:
            return (BCM_E_UNAVAIL);
    }
    return (BCM_E_NONE);
}
#endif /* !BCM_TRIDENT_SUPPORT */

#if defined(BCM_KATANA_SUPPORT)
/*
 * Function:
 *     _field_kt_action_update
 * Purpose:
 *     Modify action values
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     mem      - (IN) Policy table memory.
 *     f_ent    - (IN) Field entry structure to get policy info from.
 *     tcam_idx - (IN) Field policy table entry index.
 *     fa       - (IN) Field action.
 *     buf      - (OUT) Field Policy table entry.
 * Returns:
 *     BCM_E_XXX
 */
STATIC
int _field_kt_action_update(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                            int tcam_idx, _field_action_t *fa, uint32 *buf)
{
    int cosq_new = BCM_COS_INVALID;   /* Unicast new Queue value */

    /* Check and return error for invalid cases */
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    if (!(SOC_IS_KATANAX(unit))) {
        return BCM_E_UNAVAIL;
    }

    FP_VVERB(("FP(unit %d) vverb: _field_kt_action_update ", unit));
    FP_VVERB(("eid=%d, tcam_idx=0x%x\n ", f_ent->eid, tcam_idx));

    /* Get Queue value for Unicast actions */
    if ((bcmFieldActionCosQNew == fa->action) ||
        (bcmFieldActionRpCosQNew == fa->action)||
        (bcmFieldActionYpCosQNew == fa->action)||
        (bcmFieldActionGpCosQNew == fa->action)||
        (bcmFieldActionUcastCosQNew == fa->action)||
        (bcmFieldActionRpUcastCosQNew == fa->action) ||
        (bcmFieldActionYpUcastCosQNew == fa->action) ||
        (bcmFieldActionGpUcastCosQNew == fa->action)) {
        /*
         * Check if queue parameter value is of GPORT type.
         */
        if (BCM_GPORT_IS_SET(fa->param[0]) &&
            (BCM_GPORT_IS_UCAST_QUEUE_GROUP(fa->param[0]) ||
             BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(fa->param[0]))) {
                /* The field x_CHANGE_COS_OR_INT_PRI takes 
                 * hardware index number corresponding to the cos queue.
                 * Currently _bcm_kt_cosq_index_resolve() returns hw index
                 * when flag is _BCM_KT_COSQ_INDEX_STYLE_BUCKET.
                 */
                BCM_IF_ERROR_RETURN
                    (_bcm_kt_cosq_index_resolve(unit, fa->param[0], 0,
                        _BCM_KT_COSQ_INDEX_STYLE_BUCKET, NULL,
                        &cosq_new, NULL));
        } else {
            cosq_new = fa->param[0];
        }
    }
    switch (fa->action) {
        case bcmFieldActionCosQNew:
        case bcmFieldActionUcastCosQNew:
            /* Set mode info for all colors */
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_QUEUE_NEW_MODE);
            /* Set queue info for all colors */
            PolicySet(unit, mem, buf, R_COS_INT_PRIf, cosq_new);
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf, cosq_new);
            PolicySet(unit, mem, buf, G_COS_INT_PRIf, cosq_new);
            break;
        case bcmFieldActionRpCosQNew:
        case bcmFieldActionRpUcastCosQNew:
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, R_COS_INT_PRIf, cosq_new);
            break;
        case bcmFieldActionYpCosQNew:
        case bcmFieldActionYpUcastCosQNew:
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf, cosq_new);
            break;
        case bcmFieldActionGpCosQNew:
        case bcmFieldActionGpUcastCosQNew:
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf,
                _FP_ACTION_UCAST_QUEUE_NEW_MODE);
            PolicySet(unit, mem, buf, G_COS_INT_PRIf, cosq_new);
            break;
        default:
            return (BCM_E_UNAVAIL);
    }
    return (BCM_E_NONE);
}
#endif  /* !BCM_KATANA_SUPPORT */

#define BCM_FIELD_OAM_SESSION_ID_MAX 0xffff
/*
 * Function:
 *     _bcm_field_trx_action_get
 * Purpose:
 *     Get the actions to be written
 * Parameters:
 *     unit     - BCM device number
 *     mem      - Policy table memory
 *     f_ent    - entry structure to get policy info from
 *     tcam_idx - index into TCAM
 *     fa       - field action
 *     buf      - (OUT) Field Policy table entry
 * Returns:
 *     BCM_E_NONE
 *     BCM_E_PARAM - Action parameter out-of-range or unrecognized action.
 * Notes:
 *     This is a simple read/modify/write pattern.
 *     FP unit lock should be held by calling function.
 */
int
_bcm_field_trx_action_get(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                          int tcam_idx, _field_action_t *fa, uint32 *buf)
{
    uint32    mode;
    int       rv;
    uint32 value = 0;
    soc_field_t l3sw_or_change_l2 = INVALIDf;
#if defined(INCLUDE_L3)
    int       svp = 0;
#endif
    
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    FP_VVERB(("FP(unit %d) vverb: BEGIN _bcm_field_trx_action_get(eid=%d, tcam_idx=0x%x)\n",
        unit, f_ent->eid, tcam_idx));

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        rv = _bcm_field_tr3_action_get(unit, mem, f_ent, tcam_idx, fa,
                buf);
        if (BCM_SUCCESS(rv)) {
            fa->flags &= ~_FP_ACTION_DIRTY; /* Mark action as installed. */
            return rv;
        }
    }
#endif

#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        rv = _bcm_field_td2_action_get(unit, mem, f_ent, tcam_idx, fa, buf);
        if (BCM_SUCCESS(rv)) {
            fa->flags &= ~_FP_ACTION_DIRTY; /* Mark action as installed. */
            return rv;
        }
    }
#endif

    if (SOC_MEM_FIELD_VALID(unit, mem, G_L3SW_CHANGE_MACDA_OR_VLANf)) {
        l3sw_or_change_l2 = G_L3SW_CHANGE_MACDA_OR_VLANf;
    } else {
        l3sw_or_change_l2 = G_L3SW_CHANGE_L2_FIELDSf;
    }

    switch (fa->action) {
    case bcmFieldActionMultipathHash:
        PolicySet(unit, mem, buf, ECMP_HASH_SELf, fa->param[0]);
        break;
    case bcmFieldActionCopyToCpu:
    case bcmFieldActionRpCopyToCpu:
    case bcmFieldActionYpCopyToCpu:
    case bcmFieldActionGpCopyToCpu:
    case bcmFieldActionTimeStampToCpu:
    case bcmFieldActionRpTimeStampToCpu:
    case bcmFieldActionYpTimeStampToCpu:
    case bcmFieldActionGpTimeStampToCpu:
#ifdef BCM_TRIUMPH2_SUPPORT
        if (!soc_feature(unit, soc_feature_internal_loopback) &&
            (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id)) {
            /* Loopback port is not available */
            return BCM_E_UNAVAIL;
        }
#endif
        rv = _field_trx_action_copy_to_cpu(unit, mem, f_ent, fa, buf);
        BCM_IF_ERROR_RETURN(rv);
        break;

    case bcmFieldActionCopyToCpuCancel:
    case bcmFieldActionRpCopyToCpuCancel:
    case bcmFieldActionYpCopyToCpuCancel:
    case bcmFieldActionGpCopyToCpuCancel:
    case bcmFieldActionTimeStampToCpuCancel:
    case bcmFieldActionRpTimeStampToCpuCancel:
    case bcmFieldActionYpTimeStampToCpuCancel:
    case bcmFieldActionGpTimeStampToCpuCancel:
        rv = _field_trx_action_copy_to_cpu_cancel(unit, mem, f_ent, fa, buf);
        BCM_IF_ERROR_RETURN(rv);
        break;

    case bcmFieldActionDrop:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, DROPf, 0x1);
        } else {
            PolicySet(unit, mem, buf, R_DROPf, 0x1);
            PolicySet(unit, mem, buf, Y_DROPf, 0x1);
            PolicySet(unit, mem, buf, G_DROPf, 0x1);
        }
        break;

    case bcmFieldActionDropCancel:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, DROPf, 0x2);
        } else {
            PolicySet(unit, mem, buf, R_DROPf, 0x2);
            PolicySet(unit, mem, buf, Y_DROPf, 0x2);
            PolicySet(unit, mem, buf, G_DROPf, 0x2);
        }
        break;

    case bcmFieldActionClassDestSet:
        PolicySet(unit, mem, buf, USE_VFP_CLASS_ID_Lf, 0x1);
        PolicySet(unit, mem, buf, VFP_CLASS_ID_Lf, fa->param[0]);
        break;

    case bcmFieldActionClassSourceSet:
        PolicySet(unit, mem, buf, USE_VFP_CLASS_ID_Hf, 0x1);
        PolicySet(unit, mem, buf, VFP_CLASS_ID_Hf, fa->param[0]);
        break;

    case bcmFieldActionVrfSet:
        if (soc_feature(unit, soc_feature_mpls)) {
            if (SOC_MEM_FIELD_VALID(unit, mem, FIELDS_ACTIONf)) {
                PolicySet(unit, mem, buf, FIELDS_ACTIONf, 0x3);
            } else {
                PolicySet(unit, mem, buf, MPLS_ACTIONf, 0x3);
            }
            PolicySet(unit, mem, buf, VFP_VRF_IDf, fa->param[0]);
        }  else {
            if (SOC_MEM_FIELD_VALID(unit, mem, FIELDS_ACTIONf)) {
                PolicySet(unit, mem, buf, FIELDS_ACTIONf, 0x3);
            } else {
                PolicySet(unit, mem, buf, USE_VFP_VRF_IDf , 0x1);
            }
            PolicySet(unit, mem, buf, VFP_VRF_IDf, fa->param[0]);

        }
        break;

    case bcmFieldActionDropPrecedence:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            uint8 hardware_color;

            /* Convert to hardware color code because the tr2x
               VFP hardware doesn't do it for us */

            switch (fa->param[0])
                {
                case BCM_FIELD_COLOR_GREEN:
                    hardware_color = 0;
                    break;

                case BCM_FIELD_COLOR_RED:
                    hardware_color = 1;
                    break;

                case BCM_FIELD_COLOR_YELLOW:
                    hardware_color = 3;
                    break;

                default:
                    return BCM_E_PARAM;
                }

            PolicySet(unit, mem, buf, CHANGE_CNGf, 0x1);
            PolicySet(unit, mem, buf, NEW_CNGf, hardware_color);
        } else {
            PolicySet(unit, mem, buf, R_DROP_PRECEDENCEf, fa->param[0]);
            PolicySet(unit, mem, buf, Y_DROP_PRECEDENCEf, fa->param[0]);
            PolicySet(unit, mem, buf, G_DROP_PRECEDENCEf, fa->param[0]);
        }
        break;

    case bcmFieldActionPrioPktCopy:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x4);
        break;

    case bcmFieldActionPrioPktNew:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
#ifdef BCM_TRIDENT_SUPPORT
            if (soc_mem_field_valid(unit, mem, NEW_OUTER_DOT1Pf)) {
                PolicySet(unit, mem, buf, CHANGE_OUTER_DOT1Pf, 0x1);
                PolicySet(unit, mem, buf, NEW_OUTER_DOT1Pf, fa->param[0]);
            } else
#endif /* !BCM_TRIDENT_SUPPORT */
            {
                PolicySet(unit, mem, buf, CHANGE_PKT_PRIORITYf, 0x1);
                PolicySet(unit, mem, buf, NEW_PKT_PRIORITYf, fa->param[0]);
            }
        } else {
            PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x5);
            PolicySet(unit, mem, buf, R_NEW_PKT_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x5);
            PolicySet(unit, mem, buf, Y_NEW_PKT_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x5);
            PolicySet(unit, mem, buf, G_NEW_PKT_PRIf, fa->param[0]);
        }
        break;

    case bcmFieldActionPrioPktTos:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x6);
        break;

    case bcmFieldActionPrioPktCancel:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x7);
        break;

    case bcmFieldActionEcnNew:
        BCM_IF_ERROR_RETURN
            (_field_trx_action_ecn_update (unit, mem, f_ent, fa, buf));
        break;

    case bcmFieldActionDscpNew:
        PolicySet(unit, mem, buf, R_CHANGE_DSCPf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_DSCPf, fa->param[0]);
        PolicySet(unit, mem, buf, Y_CHANGE_DSCPf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_DSCPf, fa->param[0]);
        if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, G_CHANGE_DSCPf, 0x1);
            PolicySet(unit, mem, buf, G_NEW_DSCPf, fa->param[0]);
        } else {
            PolicySet(unit, mem, buf, G_CHANGE_DSCP_TOSf, 0x3);
            PolicySet(unit, mem, buf, G_NEW_DSCP_TOSf, fa->param[0]);
        }
        break;

    case bcmFieldActionDscpCancel:
        PolicySet(unit, mem, buf, R_CHANGE_DSCPf, 0x2);
        PolicySet(unit, mem, buf, Y_CHANGE_DSCPf, 0x2);
        PolicySet(unit, mem, buf, G_CHANGE_DSCP_TOSf, 0x4);
        break;

    case bcmFieldActionCosQNew:
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
        if (SOC_IS_TD_TT(unit)) {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        } else
#endif
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit) && BCM_GPORT_IS_SET(fa->param[0])) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x1);
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x1);
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x1);
            PolicySet(unit, mem, buf, R_COS_INT_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, G_COS_INT_PRIf, fa->param[0]);
        }
        break;

    case bcmFieldActionVlanCosQNew:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x2);
        /* Add 8 to the value since VLAN shaping queues are 8..23 */
        PolicySet(unit, mem, buf, R_COS_INT_PRIf, fa->param[0] + 8);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x2);
        PolicySet(unit, mem, buf, Y_COS_INT_PRIf, fa->param[0] + 8);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x2);
        PolicySet(unit, mem, buf, G_COS_INT_PRIf, fa->param[0] + 8);
        break;

#if defined(BCM_TRIDENT_SUPPORT)
    case bcmFieldActionUcastCosQNew:
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit)) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        }
        break;
    case bcmFieldActionMcastCosQNew:
        BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit, mem,
                f_ent, tcam_idx, fa, buf));
        break;
#endif /* !BCM_TRIDENT_SUPPORT */

    case bcmFieldActionPrioIntCopy:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x4);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x4);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;

    case bcmFieldActionPrioIntNew:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_INT_PRIORITYf, 0x1);
            PolicySet(unit, mem, buf, NEW_INT_PRIORITYf,  fa->param[0]);
        } else {
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x5);
            PolicySet(unit, mem, buf, R_COS_INT_PRIf, (0xf & fa->param[0]));
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x5);
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf, (0xf & fa->param[0]));
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x5);
            PolicySet(unit, mem, buf, G_COS_INT_PRIf, (0xf & fa->param[0]));
        }
        break;

    case bcmFieldActionPrioIntTos:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x6);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x6);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;

    case bcmFieldActionPrioIntCancel:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x7);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x7);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;

    case bcmFieldActionPrioPktAndIntNew:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD_TT(unit)
                || SOC_IS_KATANAX(unit)
                || SOC_IS_TRIUMPH3(unit)) {
                PolicySet(unit, mem, buf, CHANGE_OUTER_DOT1Pf, 0x1);
                PolicySet(unit, mem, buf, NEW_OUTER_DOT1Pf, fa->param[0]);
            } else
#endif
            {
                PolicySet(unit, mem, buf, CHANGE_PKT_PRIORITYf, 0x1);
                PolicySet(unit, mem, buf, NEW_PKT_PRIORITYf, fa->param[0]);
            }
            PolicySet(unit, mem, buf, CHANGE_INT_PRIORITYf, 0x1);
            PolicySet(unit, mem, buf, NEW_INT_PRIORITYf,  fa->param[0]);
        } else {
            PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x5);
            PolicySet(unit, mem, buf, R_NEW_PKT_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x5);
            PolicySet(unit, mem, buf, R_COS_INT_PRIf, (0xf & fa->param[0]));
            PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x5);
            PolicySet(unit, mem, buf, Y_NEW_PKT_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x5);
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf, (0xf & fa->param[0]));
            PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x5);
            PolicySet(unit, mem, buf, G_NEW_PKT_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x5);
            PolicySet(unit, mem, buf, G_COS_INT_PRIf, (0xf & fa->param[0]));
        }
        break;

    case bcmFieldActionPrioPktAndIntCopy:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x4);
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x4);
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;

    case bcmFieldActionPrioPktAndIntTos:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x6);
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x6);
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;

    case bcmFieldActionPrioPktAndIntCancel:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x7);
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x7);
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;

    case bcmFieldActionCosQCpuNew:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CPU_COSf, fa->param[0]);
        } else if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_CPU_COSf, 0x1);
            PolicySet(unit, mem, buf, NEW_CPU_COSf, fa->param[0]);
        } else {
            PolicySet(unit, mem, buf, CHANGE_CPU_COSf, 0x1);
            PolicySet(unit, mem, buf, CPU_COSf, fa->param[0]);
        }
        break;

    case bcmFieldActionOuterVlanPrioNew:
#ifdef BCM_TRIDENT_SUPPORT
        if ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) ||
            SOC_IS_TRIUMPH3(unit))
            && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_OUTER_DOT1Pf, 0x2);
            PolicySet(unit, mem, buf, NEW_OUTER_DOT1Pf, fa->param[0]);
        } else
#endif
        {
        PolicySet(unit, mem, buf, R_CHANGE_DOT1Pf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_DOT1Pf, fa->param[0]);
        PolicySet(unit, mem, buf, Y_CHANGE_DOT1Pf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_DOT1Pf, fa->param[0]);
        PolicySet(unit, mem, buf, G_CHANGE_DOT1Pf, 0x1);
        PolicySet(unit, mem, buf, G_NEW_DOT1Pf, fa->param[0]);
        }
        break;

    case bcmFieldActionOuterVlanPrioCopyInner:
        PolicySet(unit, mem, buf, CHANGE_OUTER_DOT1Pf, 3);
        break;

    case bcmFieldActionInnerVlanPrioNew:
#ifdef BCM_TRIDENT_SUPPORT
        if ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) ||
            SOC_IS_TRIUMPH3(unit))
            && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_INNER_DOT1Pf, 0x2);
            PolicySet(unit, mem, buf, NEW_INNER_DOT1Pf, fa->param[0]);
        } else
#endif
        {  
        PolicySet(unit, mem, buf, R_REPLACE_INNER_PRIf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_INNER_PRIf, fa->param[0]);
        PolicySet(unit, mem, buf, Y_REPLACE_INNER_PRIf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_INNER_PRIf, fa->param[0]);
        PolicySet(unit, mem, buf, G_REPLACE_INNER_PRIf, 0x1);
        PolicySet(unit, mem, buf, G_NEW_INNER_PRIf, fa->param[0]);
        }  
        break;

    case bcmFieldActionInnerVlanPrioCopyOuter:
        PolicySet(unit, mem, buf, CHANGE_INNER_DOT1Pf, 3);
        break;

    case bcmFieldActionOuterVlanCfiNew:
#ifdef BCM_TRIDENT_SUPPORT
        if ((SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit))
            && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_OUTER_CFIf, 0x2);
            PolicySet(unit, mem, buf, NEW_OUTER_CFIf, fa->param[0]);
        } else
#endif
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANAX(unit)
            && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_OUTER_CFIf, 0x1);
            PolicySet(unit, mem, buf, NEW_OUTER_CFIf, fa->param[0]);
        } else
#endif
        {
            PolicySet(unit, mem, buf, R_CHANGE_OUTER_CFIf, 0x1);
            PolicySet(unit, mem, buf, R_NEW_OUTER_CFIf, fa->param[0]);
            PolicySet(unit, mem, buf, Y_CHANGE_OUTER_CFIf, 0x1);
            PolicySet(unit, mem, buf, Y_NEW_OUTER_CFIf, fa->param[0]);
            PolicySet(unit, mem, buf, G_CHANGE_OUTER_CFIf, 0x1);
            PolicySet(unit, mem, buf, G_NEW_OUTER_CFIf, fa->param[0]);
        }
        break;

    case bcmFieldActionOuterVlanCfiCopyInner:
        PolicySet(unit, mem, buf, CHANGE_OUTER_CFIf, 3);
        break;

    case bcmFieldActionInnerVlanCfiNew:
#ifdef BCM_TRIDENT_SUPPORT
        if ((SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) 
            && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_INNER_CFIf, 0x2);
            PolicySet(unit, mem, buf, NEW_INNER_CFIf, fa->param[0]);
        } else
#endif
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANAX(unit) 
            && _BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, CHANGE_INNER_CFIf, 0x1);
            PolicySet(unit, mem, buf, NEW_INNER_CFIf, fa->param[0]);
        } else
#endif
        {
            PolicySet(unit, mem, buf, R_CHANGE_INNER_CFIf, 0x1);
            PolicySet(unit, mem, buf, R_NEW_INNER_CFIf, fa->param[0]);
            PolicySet(unit, mem, buf, Y_CHANGE_INNER_CFIf, 0x1);
            PolicySet(unit, mem, buf, Y_NEW_INNER_CFIf, fa->param[0]);
            PolicySet(unit, mem, buf, G_CHANGE_INNER_CFIf, 0x1);
            PolicySet(unit, mem, buf, G_NEW_INNER_CFIf, fa->param[0]);
        }
        break;

    case bcmFieldActionInnerVlanCfiCopyOuter:
        PolicySet(unit, mem, buf, CHANGE_INNER_CFIf, 3);
        break;

    case bcmFieldActionSwitchToCpuCancel:
        value = SWITCH_TO_CPU_CANCEL;
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
            SOC_IS_KATANA2(unit)) {
            if ((COPY_TO_CPU_CANCEL ==
                 PolicyGet(unit, mem, buf, R_COPY_TO_CPUf)) &&
                (COPY_TO_CPU_CANCEL ==
                 PolicyGet(unit, mem, buf, Y_COPY_TO_CPUf)) &&
                (COPY_TO_CPU_CANCEL ==
                 PolicyGet(unit, mem, buf, G_COPY_TO_CPUf))) {

                 value = COPY_AND_SWITCH_TO_CPU_CANCEL;                 
            }
        }
#endif
        PolicySet(unit, mem, buf, R_COPY_TO_CPUf, value);
        PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, value);
        PolicySet(unit, mem, buf, G_COPY_TO_CPUf, value);
        break;

    case bcmFieldActionSwitchToCpuReinstate:
        PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x4);
        PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x4);
        PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x4);
        break;

    case bcmFieldActionInnerVlanAdd:
        PolicySet(unit, mem, buf, INNER_VLAN_ACTIONSf, 0x1);
        PolicySet(unit, mem, buf, NEW_INNER_VLANf, fa->param[0]);
        break;

    case bcmFieldActionOuterVlanAdd:
        PolicySet(unit, mem, buf, OUTER_VLAN_ACTIONSf, 0x1);
        PolicySet(unit, mem, buf, NEW_OUTER_VLANf, fa->param[0]);
        break;

    case bcmFieldActionInnerVlanDelete:
        PolicySet(unit, mem, buf, INNER_VLAN_ACTIONSf, 0x3);
        break;

    case bcmFieldActionOuterVlanLookup:
        PolicySet(unit, mem, buf, OUTER_VLAN_ACTIONSf, 0x3);
        PolicySet(unit, mem, buf, NEW_OUTER_VLANf, fa->param[0]);
        break;

    case bcmFieldActionIpFix:
        if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, PID_IPFIX_ACTIONSf, 0x1);
        } else {
            PolicySet(unit, mem, buf, IPFIX_CONTROLf, 0x1);
        }
        break;

    case bcmFieldActionIpFixCancel:
        if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, PID_IPFIX_ACTIONSf, 0x2);
        } else {
            PolicySet(unit, mem, buf, IPFIX_CONTROLf, 0x2);
        }
        break;

    case bcmFieldActionDoNotLearn:
        PolicySet(unit, mem, buf, DO_NOT_LEARNf, 0x1);
        break;

    case bcmFieldActionDoNotCheckVlan:
        PolicySet(unit, mem, buf, DISABLE_VLAN_CHECKSf, 0x1);
        break;

    case bcmFieldActionInnerVlanNew:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, INNER_VLAN_ACTIONSf, 0x2);
            PolicySet(unit, mem, buf, NEW_INNER_VLANf, fa->param[0]);
        } else if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, PID_REPLACE_INNER_VIDf, 0x1);
            PolicySet(unit, mem, buf, PID_NEW_INNER_VIDf, fa->param[0]);
        }
        break;

    case bcmFieldActionInnerVlanCopyOuter:
        PolicySet(unit, mem, buf, INNER_VLAN_ACTIONSf, 4);
        break;

    case bcmFieldActionOuterVlanNew:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, OUTER_VLAN_ACTIONSf, 0x2);
            PolicySet(unit, mem, buf, NEW_OUTER_VLANf, fa->param[0]);
        } else if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, PID_REPLACE_OUTER_VIDf, 0x1);
            PolicySet(unit, mem, buf, PID_NEW_OUTER_VIDf, fa->param[0]);
        } else {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x1);
            if (SOC_MEM_FIELD_VALID(unit, mem, ECMPf)) {
                PolicySet(unit, mem, buf, ECMPf, 0);
            }
            PolicySet(unit, mem, buf, NEXT_HOP_INDEXf, fa->hw_index);
            f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        }
        break;

    case bcmFieldActionOuterVlanCopyInner:
        PolicySet(unit, mem, buf, OUTER_VLAN_ACTIONSf, 4);
        break;

    case bcmFieldActionSrcMacNew:
    case bcmFieldActionDstMacNew:
        if (SOC_IS_TD_TT(unit)
            || SOC_IS_TRIUMPH2(unit)
            || SOC_IS_APOLLO(unit)
            || SOC_IS_KATANAX(unit)
            || SOC_IS_ENDURO(unit)
            || SOC_IS_HURRICANEX(unit)
            || SOC_IS_TRIUMPH3(unit)
            ) {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x1);
        } else {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x4);
        }
        if (SOC_MEM_FIELD_VALID(unit, mem, ECMPf)) {
            PolicySet(unit, mem, buf, ECMPf, 0);
        }
        PolicySet(unit, mem, buf, NEXT_HOP_INDEXf, fa->hw_index);
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionVnTagNew:
    case bcmFieldActionVnTagDelete:
        PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x1);
        if (SOC_MEM_FIELD_VALID(unit, mem, ECMPf)) {
            PolicySet(unit, mem, buf, ECMPf, 0);
        }
        PolicySet(unit, mem, buf, NEXT_HOP_INDEXf, fa->hw_index);
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionOuterTpidNew:
        BCM_IF_ERROR_RETURN(_bcm_field_tpid_hw_encode(unit, fa->param[0], &mode));
        PolicySet(unit, mem, buf, PID_REPLACE_OUTER_TPIDf, 1);
        PolicySet(unit, mem, buf, PID_OUTER_TPID_INDEXf, mode);
        break;

    case bcmFieldActionMirrorOverride:
        PolicySet(unit, mem, buf, MIRROR_OVERRIDEf, 1);
        break;

    case bcmFieldActionDoNotChangeTtl:
        PolicySet(unit, mem, buf, DO_NOT_CHANGE_TTLf, 1);
        break;

    case bcmFieldActionDoNotCheckUrpf:
        PolicySet(unit, mem, buf, DO_NOT_URPFf, 1);
        break;

    case bcmFieldActionCnmCancel:
        PolicySet(unit, mem, buf, DO_NOT_GENERATE_CNMf, 1);
        break;
#if defined(BCM_TRIDENT_SUPPORT) 
    case bcmFieldActionDynamicHgTrunkCancel:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
            PolicySet(unit, mem, buf, HGT_DLB_DISABLEf, 1);
        } 
#endif
        if (SOC_IS_TRIDENT(unit)) {
        PolicySet(unit, mem, buf, DISABLE_DYNAMIC_LOAD_BALANCINGf, 1);
        }
        break;
#endif
    case bcmFieldActionMirrorIngress:    /* param0=modid, param1=port/tgid */
        BCM_IF_ERROR_RETURN
            (_bcm_field_trx_mirror_ingress_add(unit, mem, f_ent, fa, buf));
        break;
    case bcmFieldActionMirrorEgress:     /* param0=modid, param1=port/tgid */
        BCM_IF_ERROR_RETURN
            (_bcm_field_trx_mirror_egress_add(unit, mem, f_ent, fa, buf));
        break;
    case bcmFieldActionOffloadRedirect:
    case bcmFieldActionRedirect:
    case bcmFieldActionRedirectTrunk:
    case bcmFieldActionRedirectCancel:
    case bcmFieldActionRedirectPbmp:
#ifdef INCLUDE_L3
    case bcmFieldActionRedirectIpmc:
#endif /* INCLUDE_L3 */
    case bcmFieldActionRedirectMcast:
    case bcmFieldActionRedirectVlan:
    case bcmFieldActionRedirectBcastPbmp:
    case bcmFieldActionEgressMask:
    case bcmFieldActionEgressPortsAdd:
        BCM_IF_ERROR_RETURN
            (_field_trx_action_redirect(unit, mem, f_ent, fa, buf));
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionOffloadDropIndication:
        PolicySet(unit, mem, buf, SPLIT_DROP_RESOLVEf, 0x1);
        break;
    case bcmFieldActionOffloadClassSet:
        PolicySet(unit, mem, buf, PPD3_CLASS_TAGf, fa->param[0]);
        break;
    case bcmFieldActionRpDrop:
        PolicySet(unit, mem, buf, R_DROPf, 0x1);
        break;
    case bcmFieldActionRpDropCancel:
        PolicySet(unit, mem, buf, R_DROPf, 0x2);
        break;
    case bcmFieldActionRpDropPrecedence:
        PolicySet(unit, mem, buf, R_DROP_PRECEDENCEf, fa->param[0]);
        break;
    case bcmFieldActionRpSwitchToCpuCancel:
        value = SWITCH_TO_CPU_CANCEL;
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
            SOC_IS_KATANA2(unit)) {
            if (COPY_TO_CPU_CANCEL == 
                PolicyGet(unit, mem, buf, R_COPY_TO_CPUf)) {

                value = COPY_AND_SWITCH_TO_CPU_CANCEL;
            }
        }
#endif 
        PolicySet(unit, mem, buf, R_COPY_TO_CPUf, value);
        break;
    case bcmFieldActionRpSwitchToCpuReinstate:
        PolicySet(unit, mem, buf, R_COPY_TO_CPUf, 0x4);
        break;
    case bcmFieldActionRpPrioPktCopy:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x4);
        break;
    case bcmFieldActionRpPrioPktNew:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x5);
        PolicySet(unit, mem, buf, R_NEW_PKT_PRIf, fa->param[0]);
        break;
    case bcmFieldActionRpPrioPktTos:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x6);
        break;
    case bcmFieldActionRpPrioPktCancel:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x7);
        break;
    case bcmFieldActionRpEcnNew:
        BCM_IF_ERROR_RETURN
            (_field_trx_action_ecn_update (unit, mem, f_ent, fa, buf));
        break;
    case bcmFieldActionRpDscpNew:
        PolicySet(unit, mem, buf, R_CHANGE_DSCPf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_DSCPf, fa->param[0]);
        break;
    case bcmFieldActionRpDscpCancel:
        PolicySet(unit, mem, buf, R_CHANGE_DSCPf, 0x2);
        break;
    case bcmFieldActionRpCosQNew:
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
        if (SOC_IS_TD_TT(unit)) {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        } else
#endif
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit) && BCM_GPORT_IS_SET(fa->param[0])) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x1);
            PolicySet(unit, mem, buf, R_COS_INT_PRIf, fa->param[0]);
        }
        break;
    case bcmFieldActionRpVlanCosQNew:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x2);
        /* Add 8 to the value since VLAN shaping queues are 8..23 */
        PolicySet(unit, mem, buf, R_COS_INT_PRIf, fa->param[0] + 8);
        break;
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
    case bcmFieldActionRpUcastCosQNew:
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit)) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        } 
        break;
    case bcmFieldActionRpMcastCosQNew:
        BCM_IF_ERROR_RETURN(_bcm_field_trx_ucast_mcast_action_update(unit, mem,
            f_ent, tcam_idx, fa, buf));
        break;
#endif /* !BCM_TRIDENT_SUPPORT */
    case bcmFieldActionRpPrioPktAndIntCopy:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;
    case bcmFieldActionRpPrioPktAndIntNew:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x5);
        PolicySet(unit, mem, buf, R_NEW_PKT_PRIf, fa->param[0]);
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x5);
        PolicySet(unit, mem, buf, R_COS_INT_PRIf, (0xf & fa->param[0]));
        break;
    case bcmFieldActionRpPrioPktAndIntTos:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;
    case bcmFieldActionRpPrioPktAndIntCancel:
        PolicySet(unit, mem, buf, R_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;
    case bcmFieldActionRpOuterVlanPrioNew:
        PolicySet(unit, mem, buf, R_CHANGE_DOT1Pf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_DOT1Pf, fa->param[0]);
        break;
    case bcmFieldActionRpInnerVlanPrioNew:
        PolicySet(unit, mem, buf, R_REPLACE_INNER_PRIf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_INNER_PRIf, fa->param[0]);
        break;
    case bcmFieldActionRpOuterVlanCfiNew:
        PolicySet(unit, mem, buf, R_CHANGE_OUTER_CFIf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_OUTER_CFIf, fa->param[0]);
        break;
    case bcmFieldActionRpInnerVlanCfiNew:
        PolicySet(unit, mem, buf, R_CHANGE_INNER_CFIf, 0x1);
        PolicySet(unit, mem, buf, R_NEW_INNER_CFIf, fa->param[0]);
        break;
    case bcmFieldActionRpPrioIntCopy:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;
    case bcmFieldActionRpPrioIntNew:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x5);
        PolicySet(unit, mem, buf, R_COS_INT_PRIf, (0xf & fa->param[0]));
        break;
    case bcmFieldActionRpPrioIntTos:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;
    case bcmFieldActionRpPrioIntCancel:
        PolicySet(unit, mem, buf, R_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;
    case bcmFieldActionYpDrop:
        PolicySet(unit, mem, buf, Y_DROPf, 0x1);
        break;
    case bcmFieldActionYpDropCancel:
        PolicySet(unit, mem, buf, Y_DROPf, 0x2);
        break;
    case bcmFieldActionYpDropPrecedence:
        PolicySet(unit, mem, buf, Y_DROP_PRECEDENCEf, fa->param[0]);
        break;
    case bcmFieldActionYpSwitchToCpuCancel:
        value = SWITCH_TO_CPU_CANCEL;
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
            SOC_IS_KATANA2(unit)) {
            if (COPY_TO_CPU_CANCEL ==
                PolicyGet(unit, mem, buf, Y_COPY_TO_CPUf)) {

                value = COPY_AND_SWITCH_TO_CPU_CANCEL;
            }
        }
#endif
        PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, value);
        break;
    case bcmFieldActionYpSwitchToCpuReinstate:
        PolicySet(unit, mem, buf, Y_COPY_TO_CPUf, 0x4);
        break;
    case bcmFieldActionYpPrioPktCopy:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x4);
        break;
    case bcmFieldActionYpPrioPktNew:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x5);
        PolicySet(unit, mem, buf, Y_NEW_PKT_PRIf, fa->param[0]);
        break;
    case bcmFieldActionYpPrioPktTos:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x6);
        break;
    case bcmFieldActionYpPrioPktCancel:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x7);
        break;
    case bcmFieldActionYpEcnNew:
        BCM_IF_ERROR_RETURN
            (_field_trx_action_ecn_update (unit, mem, f_ent, fa, buf));
        break;
    case bcmFieldActionYpDscpNew:
        PolicySet(unit, mem, buf, Y_CHANGE_DSCPf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_DSCPf, fa->param[0]);
        break;
    case bcmFieldActionYpDscpCancel:
        PolicySet(unit, mem, buf, Y_CHANGE_DSCPf, 0x2);
        break;
    case bcmFieldActionYpCosQNew:
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
        if (SOC_IS_TD_TT(unit)) {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        } else
#endif
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit) && BCM_GPORT_IS_SET(fa->param[0])) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x1);
            PolicySet(unit, mem, buf, Y_COS_INT_PRIf, fa->param[0]);
        }
        break;
    case bcmFieldActionYpVlanCosQNew:
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x2);
        /* Add 8 to the value since VLAN shaping queues are 8..23 */
        PolicySet(unit, mem, buf, Y_COS_INT_PRIf, fa->param[0] + 8);
        break;

#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
    case bcmFieldActionYpUcastCosQNew:
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit)) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        }
        break;
    case bcmFieldActionYpMcastCosQNew:
        BCM_IF_ERROR_RETURN(_bcm_field_trx_ucast_mcast_action_update(unit, mem,
            f_ent, tcam_idx, fa, buf));
        break;
#endif /* !BCM_TRIDENT_SUPPORT */
    case bcmFieldActionYpPrioPktAndIntCopy:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;
    case bcmFieldActionYpPrioPktAndIntNew:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x5);
        PolicySet(unit, mem, buf, Y_NEW_PKT_PRIf, fa->param[0]);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x5);
        PolicySet(unit, mem, buf, Y_COS_INT_PRIf, (0xf & fa->param[0]));
        break;
    case bcmFieldActionYpPrioPktAndIntTos:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;
    case bcmFieldActionYpPrioPktAndIntCancel:
        PolicySet(unit, mem, buf, Y_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;
    case bcmFieldActionYpOuterVlanPrioNew:
        PolicySet(unit, mem, buf, Y_CHANGE_DOT1Pf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_DOT1Pf, fa->param[0]);
        break;
    case bcmFieldActionYpInnerVlanPrioNew:
        PolicySet(unit, mem, buf, Y_REPLACE_INNER_PRIf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_INNER_PRIf, fa->param[0]);
        break;
    case bcmFieldActionYpOuterVlanCfiNew:
        PolicySet(unit, mem, buf, Y_CHANGE_OUTER_CFIf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_OUTER_CFIf, fa->param[0]);
        break;
    case bcmFieldActionYpInnerVlanCfiNew:
        PolicySet(unit, mem, buf, Y_CHANGE_INNER_CFIf, 0x1);
        PolicySet(unit, mem, buf, Y_NEW_INNER_CFIf, fa->param[0]);
        break;
    case bcmFieldActionYpPrioIntCopy:
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;
    case bcmFieldActionYpPrioIntNew:
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x5);
        PolicySet(unit, mem, buf, Y_COS_INT_PRIf, (0xf & fa->param[0]));
        break;
    case bcmFieldActionYpPrioIntTos:
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;
    case bcmFieldActionYpPrioIntCancel:
        PolicySet(unit, mem, buf, Y_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;
    case bcmFieldActionGpDrop:
        PolicySet(unit, mem, buf, G_DROPf, 0x1);
        break;
    case bcmFieldActionGpDropCancel:
        PolicySet(unit, mem, buf, G_DROPf, 0x2);
        break;
    case bcmFieldActionGpDropPrecedence:
        PolicySet(unit, mem, buf, G_DROP_PRECEDENCEf, fa->param[0]);
        break;
    case bcmFieldActionGpSwitchToCpuCancel:
        value = SWITCH_TO_CPU_CANCEL;
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) ||
            SOC_IS_KATANA2(unit)) {
            if (COPY_TO_CPU_CANCEL ==
                PolicyGet(unit, mem, buf, G_COPY_TO_CPUf)) {

                value = COPY_AND_SWITCH_TO_CPU_CANCEL;
            }
        }
#endif
        PolicySet(unit, mem, buf, G_COPY_TO_CPUf, value);
        break;
    case bcmFieldActionGpSwitchToCpuReinstate:
        PolicySet(unit, mem, buf, G_COPY_TO_CPUf, 0x4);
        break;
    case bcmFieldActionGpPrioPktCopy:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x4);
        break;
    case bcmFieldActionGpPrioPktNew:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x5);
        PolicySet(unit, mem, buf, G_NEW_PKT_PRIf, fa->param[0]);
        break;
    case bcmFieldActionGpPrioPktTos:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x6);
        break;
    case bcmFieldActionGpPrioPktCancel:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x7);
        break;
    case bcmFieldActionGpEcnNew:
        BCM_IF_ERROR_RETURN
            (_field_trx_action_ecn_update (unit, mem, f_ent, fa, buf));
        break;
    case bcmFieldActionGpTosPrecedenceNew:
        PolicySet(unit, mem, buf, G_CHANGE_DSCP_TOSf, 0x1);
        PolicySet(unit, mem, buf, G_NEW_DSCP_TOSf, fa->param[0]);
        break;
    case bcmFieldActionGpTosPrecedenceCopy:
        PolicySet(unit, mem, buf, G_CHANGE_DSCP_TOSf, 0x2);
        break;
    case bcmFieldActionGpDscpNew:
        if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, G_CHANGE_DSCPf, 1);
            PolicySet(unit, mem, buf, G_NEW_DSCPf, fa->param[0]);
        } else {
            PolicySet(unit, mem, buf, G_CHANGE_DSCP_TOSf, 0x3);
            PolicySet(unit, mem, buf, G_NEW_DSCP_TOSf, fa->param[0]);
        }
        break;
    case bcmFieldActionGpDscpCancel:
        PolicySet(unit, mem, buf, G_CHANGE_DSCP_TOSf, 0x4);
        break;
    case bcmFieldActionGpCosQNew:
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
        if (SOC_IS_TD_TT(unit)) {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        } else
#endif
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit) && BCM_GPORT_IS_SET(fa->param[0])) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x1);
            PolicySet(unit, mem, buf, G_COS_INT_PRIf, fa->param[0]);
        }
        break;
    case bcmFieldActionGpVlanCosQNew:
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x2);
        /* Add 8 to the value since VLAN shaping queues are 8..23 */
        PolicySet(unit, mem, buf, G_COS_INT_PRIf, fa->param[0] + 8);
        break;

#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
    case bcmFieldActionGpUcastCosQNew:
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit)) {
            BCM_IF_ERROR_RETURN (_field_kt_action_update(unit, mem,
                    f_ent, tcam_idx, fa, buf));
        } else
#endif
        {
            BCM_IF_ERROR_RETURN (_bcm_field_trx_ucast_mcast_action_update(unit,
                mem, f_ent, tcam_idx, fa, buf));
        }
        break;
    case bcmFieldActionGpMcastCosQNew:
        BCM_IF_ERROR_RETURN(_bcm_field_trx_ucast_mcast_action_update(unit, mem,
            f_ent, tcam_idx, fa, buf));
        break;
#endif /* !BCM_TRIDENT_SUPPORT */
    case bcmFieldActionGpPrioPktAndIntCopy:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x4);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;
    case bcmFieldActionGpPrioPktAndIntNew:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x5);
        PolicySet(unit, mem, buf, G_NEW_PKT_PRIf, fa->param[0]);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x5);
        PolicySet(unit, mem, buf, G_COS_INT_PRIf, (0xf & fa->param[0]));
        break;
    case bcmFieldActionGpPrioPktAndIntTos:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x6);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;
    case bcmFieldActionGpPrioPktAndIntCancel:
        PolicySet(unit, mem, buf, G_CHANGE_PKT_PRIf, 0x7);
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;
    case bcmFieldActionGpOuterVlanPrioNew:
        PolicySet(unit, mem, buf, G_CHANGE_DOT1Pf, 0x1);
        PolicySet(unit, mem, buf, G_NEW_DOT1Pf, fa->param[0]);
        break;
    case bcmFieldActionGpInnerVlanPrioNew:
        PolicySet(unit, mem, buf, G_REPLACE_INNER_PRIf, 0x1);
        PolicySet(unit, mem, buf, G_NEW_INNER_PRIf, fa->param[0]);
        break;
    case bcmFieldActionGpOuterVlanCfiNew:
        PolicySet(unit, mem, buf, G_CHANGE_OUTER_CFIf, 0x1);
        PolicySet(unit, mem, buf, G_NEW_OUTER_CFIf, fa->param[0]);
        break;
    case bcmFieldActionGpInnerVlanCfiNew:
        PolicySet(unit, mem, buf, G_CHANGE_INNER_CFIf, 0x1);
        PolicySet(unit, mem, buf, G_NEW_INNER_CFIf, fa->param[0]);
        break;
    case bcmFieldActionGpPrioIntCopy:
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x4);
        break;
    case bcmFieldActionGpPrioIntNew:
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x5);
        PolicySet(unit, mem, buf, G_COS_INT_PRIf, (0xf & fa->param[0]));
        break;
    case bcmFieldActionGpPrioIntTos:
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x6);
        break;
    case bcmFieldActionGpPrioIntCancel:
        PolicySet(unit, mem, buf, G_CHANGE_COS_OR_INT_PRIf, 0x7);
        break;
    case bcmFieldActionAddClassTag:
        if (SOC_MEM_FIELD_VALID(unit, mem, G_L3SW_CHANGE_L2_FIELDSf)) {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x4);
        } else {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x3);
        }
        PolicySet(unit, mem, buf, NEXT_HOP_INDEXf, fa->param[0]);
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionFabricQueue:
        PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x3);
        BCM_IF_ERROR_RETURN
            (_field_trx_action_fabricQueue_actions_set(unit, mem, fa->param[0], 
                                                       fa->param[1], buf));
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
#if defined(BCM_TRIDENT_SUPPORT)
    case bcmFieldActionCompressSrcIp6:
            if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
                PolicySet(unit, mem, buf, IPV6_TO_IPV4_MAP_SIP_VALIDf, 0x1);
                PolicySet(unit, mem, buf, IPV6_TO_IPV4_MAP_OFFSET_SETf, fa->param[0]);
        }
        break;
    case bcmFieldActionCompressDstIp6:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
                PolicySet(unit, mem, buf, IPV6_TO_IPV4_MAP_DIP_VALIDf, 0x1);
                PolicySet(unit, mem, buf, IPV6_TO_IPV4_MAP_OFFSET_SETf, fa->param[0]);
        }
        break;
#endif /* !BCM_TRIDENT_SUPPORT */

#ifdef INCLUDE_L3
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
    case bcmFieldActionIncomingMplsPortSet:
        if (!BCM_GPORT_IS_MPLS_PORT(fa->param[0])) {
            return BCM_E_PARAM;
        }
        if (SOC_MEM_FIELD_VALID(unit, mem, FIELDS_ACTIONf)) {
            PolicySet(unit, mem, buf, FIELDS_ACTIONf, 0x1);
        } else {
            PolicySet(unit, mem, buf, MPLS_ACTIONf, 0x1);
        }
        svp = BCM_GPORT_MPLS_PORT_ID_GET((bcm_gport_t)fa->param[0]);
        PolicySet(unit, mem, buf, SVPf, svp);
        PolicySet(unit, mem, buf, DISABLE_VLAN_CHECKSf, 1);

        if (SOC_IS_TRIDENT2(unit)) {
            PolicySet(unit, mem, buf, SVP_VALIDf, 1);
        }
        break;
#endif

    case bcmFieldActionIngressGportSet:
        if (SOC_MEM_FIELD_VALID(unit, mem, FIELDS_ACTIONf)) {
            PolicySet(unit, mem, buf, FIELDS_ACTIONf, 0x1);
        } else {
            PolicySet(unit, mem, buf, MPLS_ACTIONf, 0x1);
        }

        if (BCM_GPORT_IS_MPLS_PORT(fa->param[0])) {
            svp = BCM_GPORT_MPLS_PORT_ID_GET((bcm_gport_t)fa->param[0]);
        } else if (BCM_GPORT_IS_MIM_PORT(fa->param[0])) {
            svp = BCM_GPORT_MIM_PORT_ID_GET((bcm_gport_t)fa->param[0]);
        } else if (BCM_GPORT_IS_WLAN_PORT(fa->param[0])) {
            svp = BCM_GPORT_WLAN_PORT_ID_GET((bcm_gport_t)fa->param[0]);
        } else if (BCM_GPORT_IS_TRILL_PORT(fa->param[0])) {
            svp = BCM_GPORT_TRILL_PORT_ID_GET((bcm_gport_t)fa->param[0]);
        } else if (BCM_GPORT_IS_NIV_PORT(fa->param[0])) {
            svp = BCM_GPORT_NIV_PORT_ID_GET((bcm_gport_t)fa->param[0]);
        } else {
            return BCM_E_PARAM;
        }
        PolicySet(unit, mem, buf, SVPf, svp);
        PolicySet(unit, mem, buf, DISABLE_VLAN_CHECKSf, 1);

        if (SOC_IS_TRIDENT2(unit)) {
            PolicySet(unit, mem, buf, SVP_VALIDf, 1);
        }
        break;                 
    case bcmFieldActionL3IngressSet:
        if (SOC_MEM_FIELD_VALID(unit, mem, FIELDS_ACTIONf)) {
            PolicySet(unit, mem, buf, FIELDS_ACTIONf, 0x2);
        } else {
            PolicySet(unit, mem, buf, MPLS_ACTIONf, 0x2);
        }
        PolicySet(unit, mem, buf, L3_IIFf, fa->param[0]);
        PolicySet(unit, mem, buf, DISABLE_VLAN_CHECKSf, 1);
        break; 
    case bcmFieldActionL3ChangeVlan:
        PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x1);
        BCM_IF_ERROR_RETURN
            (_field_trx_policy_set_l3_info(unit, mem, fa->param[0], buf));
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionL3ChangeVlanCancel:
        PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x2);
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionL3ChangeMacDa:
        if (SOC_MEM_FIELD_VALID(unit, mem, G_L3SW_CHANGE_L2_FIELDSf)) {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x1);
        } else {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x4);
        }
        BCM_IF_ERROR_RETURN
            (_field_trx_policy_set_l3_info(unit, mem,  fa->param[0], buf));
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionL3ChangeMacDaCancel:
        if (SOC_MEM_FIELD_VALID(unit, mem, G_L3SW_CHANGE_L2_FIELDSf)) {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x2);
        } else {
            PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x5);
        }
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionL3Switch:
        PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x6);
        BCM_IF_ERROR_RETURN
            (_field_trx_policy_set_l3_info(unit, mem, fa->param[0], buf));
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
    case bcmFieldActionL3SwitchCancel:
        PolicySet(unit, mem, buf, l3sw_or_change_l2, 0x7);
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
#endif /* INCLUDE_L3 */
    case bcmFieldActionOamUpMep:
        PolicySet(unit, mem, buf, OAM_UP_MEPf, fa->param[0]);
        break;
    case bcmFieldActionOamTx:
        PolicySet(unit, mem, buf, OAM_TXf, fa->param[0]);
        break;
    case bcmFieldActionOamLmepMdl:
        PolicySet(unit, mem, buf, OAM_LMEP_MDLf, fa->param[0]);
        break;
    case bcmFieldActionOamServicePriMappingPtr:
        PolicySet(unit, mem, buf, OAM_SERVICE_PRI_MAPPING_PTRf, fa->param[0]);
        break;
    case bcmFieldActionOamLmBasePtr:
        PolicySet(unit, mem, buf, OAM_LM_BASE_PTRf, fa->param[0]);
        break;
    case bcmFieldActionOamDmEnable:
        PolicySet(unit, mem, buf, OAM_DM_ENf, fa->param[0]);
        break;
    case bcmFieldActionOamLmEnable:
        PolicySet(unit, mem, buf, OAM_LM_ENf, fa->param[0]);
        break;
    case bcmFieldActionOamLmepEnable:
        PolicySet(unit, mem, buf, OAM_LMEP_ENf, fa->param[0]);
        break;
    case bcmFieldActionOamPbbteLookupEnable:
        if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
            PolicySet(unit, mem, buf, OAM_PBBTE_LOOKUP_ENABLEf, fa->param[0]);
        }
        break;
    case bcmFieldActionServicePoolIdNew:
        PolicySet(unit, mem, buf, CHANGE_CPU_COSf, 0x2);
        PolicySet(unit, mem, buf, CPU_COSf, ((fa->param[0] << 2) | 0x20));
        break;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    case bcmFieldActionUseGlobalMeterColor:
        if (_BCM_FIELD_STAGE_INGRESS == f_ent->group->stage_id) {
            if (soc_feature(unit, soc_feature_global_meter)) {
                PolicySet(unit, mem, buf, USE_SVC_METER_COLORf,1);
            }
        }
        
        if (SOC_IS_TRIUMPH3(unit)) {
            if (_BCM_FIELD_STAGE_EXTERNAL == f_ent->group->stage_id) {
                if (soc_feature(unit, soc_feature_global_meter)) {
                    PolicySet(unit, mem, buf, USE_SVC_METER_COLORf,1);
                }
            }
        }
        break;
#endif
#ifdef INCLUDE_L3
    case bcmFieldActionRedirectEgrNextHop:
        BCM_IF_ERROR_RETURN
            (_field_trx_policy_egr_nexthop_info_set(unit, mem,
                fa->param[0], buf));
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
        break;
#endif
#if defined(BCM_KATANA2_SUPPORT)
    case bcmFieldActionOamDomain:  
        if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
            if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
                if ((fa->param[0] >= bcmFieldOamDomainCount) || 
                    (fa->param[1] >= bcmFieldOamDomainCount)) {  
                    return (BCM_E_PARAM);
                }  
                PolicySet(unit, mem, buf, OAM_KEY1f, fa->param[0]);
                PolicySet(unit, mem, buf, OAM_KEY2f, fa->param[1]);
            }
        }
        break;

    case bcmFieldActionOamOlpHeaderAdd: 
        if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
            if (_BCM_FIELD_STAGE_INGRESS == f_ent->group->stage_id) {
                if (fa->param[0] >= bcmFieldOlpHeaderTypeCount) {  
                    return (BCM_E_PARAM);
                }  
                PolicySet(unit, mem, buf, OLP_HDR_ADDf, 1);
                PolicySet(unit, mem, buf, OLP_HDR_TYPE_COMPRESSEDf, fa->param[0]);
            }
        }
        break;

    case bcmFieldActionOamSessionId: 
        if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
            if (_BCM_FIELD_STAGE_INGRESS == f_ent->group->stage_id) {
                if (fa->param[0] > BCM_FIELD_OAM_SESSION_ID_MAX) {  
                    return (BCM_E_PARAM);
                }  
                PolicySet(unit, mem, buf, OAM_SESSION_IDf, fa->param[0]);
            }
        }
        break;
#endif
#if defined(BCM_KATANA2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    case bcmFieldActionDoNotOverride:
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit)) {
            PolicySet(unit, mem, buf, ACTION_PRI_MODIFIERf, 1);
            PolicySet(unit, mem, buf, SVC_METER_INDEX_PRIORITYf, 0);
        }
        break; 
#endif
    default:
        return (BCM_E_UNAVAIL);
    }

    fa->flags &= ~_FP_ACTION_DIRTY; /* Mark action as installed. */

    FP_VVERB(("FP(unit %d) vverb: END _bcm_field_trx_action_get()\n", unit));
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_mirror_add
 * Purpose:
 *     Set mirroring destination & enable mirroring for rule matching
 *     packets.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     mem      - (IN) Policy table memory
 *     f_ent    - (IN) Entry structure to get policy info from
 *     fa       - (IN) field action
 *     buf      - (IN/OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
int
_field_trx2_mirror_add(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                       _field_action_t *fa, uint32 *buf)
{
    int mtp_index;
    uint32 enable;
    int index;

    soc_field_t mtp_field[] = {
        MTP_INDEX0f, MTP_INDEX1f,
        MTP_INDEX2f, MTP_INDEX3f};

    /* Input parameters check. */
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }
    mtp_index = fa->hw_index;
    index = fa->hw_index;
    if (index >= COUNTOF(mtp_field)) {
        return (BCM_E_INTERNAL);
    }

    if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
        PolicySet(unit, mem, buf,
            (SOC_IS_TRIUMPH3(unit) ? REDIRECTION_DESTINATIONf : REDIRECTIONf),
            0x1 << mtp_index);
        PolicySet(unit, mem, buf, G_PACKET_REDIRECTIONf, 0x4);
        f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
    } else {
        enable = PolicyGet(unit, mem, buf, MIRRORf);
        PolicySet(unit, mem, buf, mtp_field[index], mtp_index);
        PolicySet(unit, mem, buf, MIRRORf, (enable | (0x1 << index)));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_mirror_ingress_add
 * Purpose:
 *     Set ingress mirroring destination & enable mirroring for rule matching
 *     packets.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     mem      - (IN) Policy table memory
 *     f_ent    - (IN) Entry structure to get policy info from
 *     fa       - (IN) field action
 *     buf      - (IN/OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_mirror_ingress_add(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                                  _field_action_t *fa, uint32 *buf)
{
    int mtp_index;
    uint32 enable;

    /* Input parameters check. */
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

#ifdef BCM_TRIUMPH2_SUPPORT
   if (soc_feature(unit, soc_feature_mirror_flexible)) {
        return _field_trx2_mirror_add(unit, mem, f_ent, fa, buf);
    }
#endif

    /* Allocate mirror to index. */
    mtp_index = fa->hw_index;
    enable = PolicyGet(unit, mem, buf, INGRESS_MIRRORf);
    /* If destination 0 not in use install new mtp to destination 0. */
     if(0 == (enable & 0x1)) {
        PolicySet(unit, mem, buf, IM0_MTP_INDEXf, mtp_index);
        PolicySet(unit, mem, buf, INGRESS_MIRRORf, (enable | 0x1));

        return (BCM_E_NONE);
    }

    /* If destination 1 not in use install new mtp to destination 1. */
    if(0 == (enable & 0x2) && SOC_MEM_FIELD_VALID(unit, mem, IM1_MTP_INDEXf)) {
        PolicySet(unit, mem, buf, IM1_MTP_INDEXf, mtp_index);
        PolicySet(unit, mem, buf, INGRESS_MIRRORf, (enable | 0x2));
        return (BCM_E_NONE);
    }
    return (BCM_E_RESOURCE);
}

/*
 * Function:
 *     _bcm_field_trx_mirror_egress_add
 * Purpose:
 *     Set ingress mirroring destination & enable mirroring for rule matching
 *     packets.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     mem      - (IN) Policy table memory
 *     f_ent    - (IN) Entry structure to get policy info from
 *     fa       - (IN) field action
 *     buf      - (IN/OUT) Field Policy table entry
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_mirror_egress_add(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                                 _field_action_t *fa, uint32 *buf)
{
    int mtp_index;
    int enable;

    /* Input parameters check. */
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        return _field_trx2_mirror_add(unit, mem, f_ent, fa, buf);
    }
#endif

    /* Allocate mirror to index. */
    mtp_index = fa->hw_index;
    enable = PolicyGet(unit, mem, buf, EGRESS_MIRRORf);
    /* If destination 0 not in use install new mtp to destination 0. */
     if(0 == (enable & 0x1)) {
        PolicySet(unit, mem, buf, EM0_MTP_INDEXf, mtp_index);
        PolicySet(unit, mem, buf, EGRESS_MIRRORf, (enable | 0x1));

        return (BCM_E_NONE);
    }

    /* If destination 1 not in use install new mtp to destination 1. */
    if(0 == (enable & 0x2) && SOC_MEM_FIELD_VALID(unit, mem, EM1_MTP_INDEXf)) {
        PolicySet(unit, mem, buf, EM1_MTP_INDEXf, mtp_index);
        PolicySet(unit, mem, buf, EGRESS_MIRRORf, (enable | 0x2));
        return (BCM_E_NONE);
    }
    return (BCM_E_RESOURCE);
}

/*
 * Function:
 *     _bcm_field_trx_action_support_check
 *
 * Purpose:
 *     Check if action is supported by device.
 *
 * Parameters:
 *     unit   -(IN) BCM device number
 *     f_ent  -(IN) Field entry structure.
 *     action -(IN) Action to check(bcmFieldActionXXX)
 *     result -(OUT)
 *               TRUE   - action is supported by device
 *               FALSE  - action is NOT supported by device
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_stage_action_support_check(int                unit,
                                          unsigned           stage,
                                          bcm_field_action_t action,
                                          int                *result
                                          )
{
#if defined(BCM_TRIDENT_SUPPORT)
    int vfp_compress_en = 0;
#endif /* !BCM_TRIDENT_SUPPORT */

    if (stage == _BCM_FIELD_STAGE_EGRESS) {
        switch (action) {
          case bcmFieldActionDrop:
          case bcmFieldActionDropCancel:
          case bcmFieldActionInnerVlanNew:
          case bcmFieldActionOuterVlanNew:
          case bcmFieldActionOuterTpidNew:
          case bcmFieldActionOuterVlanPrioNew:
          case bcmFieldActionInnerVlanPrioNew:
          case bcmFieldActionOuterVlanCfiNew:
          case bcmFieldActionInnerVlanCfiNew:
              *result = TRUE;
              break;
          case bcmFieldActionMirrorEgress:
              *result = soc_feature(unit, soc_feature_egr_mirror_true) \
                            ? TRUE : FALSE;
              break;
          case bcmFieldActionIpFix:
          case bcmFieldActionIpFixCancel:
              *result = soc_feature(unit, soc_feature_ipfix) ? TRUE : FALSE;
              break;
          case bcmFieldActionDscpNew:
          case bcmFieldActionRpDrop:
          case bcmFieldActionRpDropCancel:
          case bcmFieldActionRpDscpNew:
          case bcmFieldActionRpOuterVlanPrioNew:
          case bcmFieldActionRpInnerVlanPrioNew:
          case bcmFieldActionRpOuterVlanCfiNew:
          case bcmFieldActionRpInnerVlanCfiNew:

          case bcmFieldActionYpDrop:
          case bcmFieldActionYpDropCancel:
          case bcmFieldActionYpDscpNew:
          case bcmFieldActionYpOuterVlanPrioNew:
          case bcmFieldActionYpInnerVlanPrioNew:
          case bcmFieldActionYpOuterVlanCfiNew:
          case bcmFieldActionYpInnerVlanCfiNew:

          case bcmFieldActionGpDrop:
          case bcmFieldActionGpDropCancel:
          case bcmFieldActionGpDscpNew:
          case bcmFieldActionGpOuterVlanPrioNew:
          case bcmFieldActionGpInnerVlanPrioNew:
          case bcmFieldActionGpOuterVlanCfiNew:
          case bcmFieldActionGpInnerVlanCfiNew:
              *result = TRUE;
              break;
          case bcmFieldActionCopyToCpu:
          case bcmFieldActionCopyToCpuCancel:
          case bcmFieldActionRpCopyToCpu:
          case bcmFieldActionRpCopyToCpuCancel:
          case bcmFieldActionYpCopyToCpu:
          case bcmFieldActionYpCopyToCpuCancel:
          case bcmFieldActionGpCopyToCpu:
          case bcmFieldActionGpCopyToCpuCancel:
              *result = SOC_MEM_FIELD_VALID(unit, EFP_POLICY_TABLEm,
                                            G_COPY_TO_CPUf) ? TRUE : FALSE;
              break;
                  case bcmFieldActionCosQCpuNew:
              *result = SOC_MEM_FIELD_VALID(unit, EFP_POLICY_TABLEm,
                                                NEW_CPU_COSf) ? TRUE : FALSE;
              break;
          case bcmFieldActionRedirect:
          case bcmFieldActionRedirectTrunk:
          case bcmFieldActionRedirectMcast:
              if (SOC_IS_TRIUMPH3(unit) &&
                   soc_feature(unit, soc_feature_axp)) {
                  *result = TRUE;
              }
              else {
                  *result = FALSE;
              }
              break;
          default:
              *result = FALSE;
        }
        return (BCM_E_NONE);
    }
    if (stage == _BCM_FIELD_STAGE_LOOKUP) {
        switch (action) {
        case bcmFieldActionDrop:
        case bcmFieldActionDropCancel:
        case bcmFieldActionCopyToCpu:
        case bcmFieldActionCopyToCpuCancel:
        case bcmFieldActionPrioPktNew:
        case bcmFieldActionInnerVlanAdd:
        case bcmFieldActionInnerVlanNew:
        case bcmFieldActionInnerVlanDelete:
        case bcmFieldActionOuterVlanAdd:
        case bcmFieldActionOuterVlanNew:
        case bcmFieldActionOuterVlanLookup:
        case bcmFieldActionCosQCpuNew:
        case bcmFieldActionPrioPktAndIntNew:
        case bcmFieldActionPrioIntNew:
        case bcmFieldActionDropPrecedence:
        case bcmFieldActionVrfSet:
        case bcmFieldActionDoNotLearn:
        case bcmFieldActionDoNotCheckVlan:
        case bcmFieldActionClassDestSet:
        case bcmFieldActionClassSourceSet:
            *result = TRUE;
            break;
        case bcmFieldActionIncomingMplsPortSet:
            *result = soc_feature(unit, soc_feature_mpls) ? TRUE : FALSE;
            break;
        case bcmFieldActionIngressGportSet:
            *result = ( soc_feature(unit, soc_feature_mpls)   ||     
                        soc_feature(unit, soc_feature_mim)    ||     
                        soc_feature(unit, soc_feature_trill)  ||     
                        soc_feature(unit, soc_feature_niv)    ||     
                        soc_feature(unit, soc_feature_wlan) ) ? TRUE : FALSE;
            break;
        case bcmFieldActionL3IngressSet:             
            *result = soc_feature(unit, soc_feature_l3_ingress_interface) ? TRUE : FALSE;
             break;
        case bcmFieldActionOamPbbteLookupEnable:
            *result = soc_feature(unit,
                        soc_feature_field_oam_actions) ? TRUE : FALSE;
            break;
        case bcmFieldActionOuterVlanCopyInner:
        case bcmFieldActionOuterVlanPrioCopyInner:
        case bcmFieldActionOuterVlanCfiCopyInner:
        case bcmFieldActionInnerVlanCopyOuter:
        case bcmFieldActionInnerVlanPrioCopyOuter:
        case bcmFieldActionInnerVlanCfiCopyOuter:
        case bcmFieldActionInnerVlanCfiNew:
        case bcmFieldActionOuterVlanCfiNew:
        case bcmFieldActionOuterVlanPrioNew:
        case bcmFieldActionInnerVlanPrioNew: 
#ifdef BCM_TRIDENT_SUPPORT
            *result = (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) || 
                       SOC_IS_TRIUMPH3(unit));
            break;
    case bcmFieldActionCompressSrcIp6:
    case bcmFieldActionCompressDstIp6:
        if (SOC_IS_TD_TT(unit) &&
            SOC_MEM_FIELD_VALID(unit, VFP_POLICY_TABLEm,
                IPV6_TO_IPV4_MAP_OFFSET_SETf)) {
            BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchIp6CompressEnable, &vfp_compress_en));
            *result = vfp_compress_en ? TRUE : FALSE;
        } else {
            *result = FALSE;
        }
        break;
#endif /* !BCM_TRIDENT_SUPPORT */

        case bcmFieldActionRegexActionCancel:
            *result = soc_feature(unit, soc_feature_regex) ? TRUE : FALSE;
        break;

#if defined(BCM_KATANA2_SUPPORT)       
        case bcmFieldActionOamDomain:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                *result = TRUE;
            } else {
                *result = FALSE;
            } 
            break;
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        case bcmFieldActionDoNotOverride:
            if (SOC_IS_KATANA2(unit) || SOC_IS_TRIUMPH3(unit)) {
                *result = TRUE;
            } else {
                *result = FALSE;
            }
            break;
#endif

        default:
            *result = FALSE;
        }
        return (BCM_E_NONE);
    }

    if ((stage == _BCM_FIELD_STAGE_INGRESS) ||
        (stage == _BCM_FIELD_STAGE_EXTERNAL)) {
        switch (action) {
          case bcmFieldActionRedirectEgrNextHop:
#if defined(INCLUDE_L3)
              if (soc_feature(unit, soc_feature_field_action_redirect_nexthop)
                  || soc_feature(unit, soc_feature_field_action_redirect_ecmp)) {
                  *result = TRUE;
              } else 
#endif
              {
                  *result = FALSE;
              }
              break;
          case bcmFieldActionAddClassTag:
          case bcmFieldActionCopyToCpu:
          case bcmFieldActionCopyToCpuCancel:
          case bcmFieldActionDrop:
          case bcmFieldActionDropCancel:
          case bcmFieldActionDropPrecedence:
          case bcmFieldActionColorIndependent:
          case bcmFieldActionL3ChangeVlan:
          case bcmFieldActionL3ChangeVlanCancel:
          case bcmFieldActionL3ChangeMacDa:
          case bcmFieldActionL3ChangeMacDaCancel:
          case bcmFieldActionL3Switch:
          case bcmFieldActionL3SwitchCancel:
          case bcmFieldActionPrioPktCopy:
          case bcmFieldActionPrioPktNew:
          case bcmFieldActionPrioPktTos:
          case bcmFieldActionPrioPktCancel:
          case bcmFieldActionEcnNew:
          case bcmFieldActionDscpNew:
          case bcmFieldActionDscpCancel:
          case bcmFieldActionCosQNew:
          case bcmFieldActionCosQCpuNew:
          case bcmFieldActionVlanCosQNew:
          case bcmFieldActionMirrorOverride:
          case bcmFieldActionSwitchToCpuCancel:
          case bcmFieldActionSwitchToCpuReinstate:
          case bcmFieldActionRedirect:
          case bcmFieldActionRedirectTrunk:
          case bcmFieldActionRedirectCancel:
          case bcmFieldActionRedirectPbmp:
          case bcmFieldActionRedirectIpmc:
          case bcmFieldActionRedirectMcast:
              *result = TRUE;
              break;
          case bcmFieldActionRedirectBcastPbmp:
              if (SOC_IS_TD_TT(unit) || SOC_IS_ENDURO(unit) ||
                  SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH2(unit)
                  || SOC_IS_TRIUMPH3(unit) || SOC_IS_APOLLO(unit)) {
                  *result = TRUE;
              } else {
                  *result = FALSE;
              }
              break;
          case bcmFieldActionRedirectVlan:
              if (SOC_IS_TR_VL(unit)) {
                  *result= TRUE;
              } else {
                  *result = FALSE;
              }
              break;
          case bcmFieldActionEgressMask:
          case bcmFieldActionEgressPortsAdd:
          case bcmFieldActionDoNotChangeTtl:
              *result = TRUE;
              break;
          case bcmFieldActionDoNotCheckUrpf:
              *result = SOC_MEM_FIELD_VALID(unit, FP_POLICY_TABLEm, DO_NOT_URPFf);
              break;
          case bcmFieldActionPrioPktAndIntCopy:
          case bcmFieldActionPrioPktAndIntNew:
          case bcmFieldActionPrioPktAndIntTos:
          case bcmFieldActionPrioPktAndIntCancel:
          case bcmFieldActionPrioIntCopy:
          case bcmFieldActionPrioIntNew:
          case bcmFieldActionPrioIntTos:
          case bcmFieldActionPrioIntCancel:
          case bcmFieldActionMirrorIngress:
          case bcmFieldActionMirrorEgress:

          case bcmFieldActionRpDrop:
          case bcmFieldActionRpDropCancel:
          case bcmFieldActionRpDropPrecedence:
          case bcmFieldActionRpCopyToCpu:
          case bcmFieldActionRpCopyToCpuCancel:
          case bcmFieldActionRpPrioPktCopy:
          case bcmFieldActionRpPrioPktNew:
          case bcmFieldActionRpPrioPktTos:
          case bcmFieldActionRpPrioPktCancel:
          case bcmFieldActionRpDscpNew:
          case bcmFieldActionRpDscpCancel:
          case bcmFieldActionRpEcnNew:
          case bcmFieldActionRpCosQNew:
          case bcmFieldActionRpVlanCosQNew:
          case bcmFieldActionRpPrioPktAndIntCopy:
          case bcmFieldActionRpPrioPktAndIntNew:
          case bcmFieldActionRpPrioPktAndIntTos:
          case bcmFieldActionRpPrioPktAndIntCancel:
          case bcmFieldActionRpSwitchToCpuCancel:
          case bcmFieldActionRpSwitchToCpuReinstate:
          case bcmFieldActionRpPrioIntCopy:
          case bcmFieldActionRpPrioIntNew:
          case bcmFieldActionRpPrioIntTos:
          case bcmFieldActionRpPrioIntCancel:

          case bcmFieldActionYpDrop:
          case bcmFieldActionYpDropCancel:
          case bcmFieldActionYpDropPrecedence:
          case bcmFieldActionYpCopyToCpu:
          case bcmFieldActionYpCopyToCpuCancel:
          case bcmFieldActionYpPrioPktCopy:
          case bcmFieldActionYpPrioPktNew:
          case bcmFieldActionYpPrioPktTos:
          case bcmFieldActionYpPrioPktCancel:
          case bcmFieldActionYpDscpNew:
          case bcmFieldActionYpDscpCancel:
          case bcmFieldActionYpEcnNew:
          case bcmFieldActionYpCosQNew:
          case bcmFieldActionYpVlanCosQNew:
          case bcmFieldActionYpPrioPktAndIntCopy:
          case bcmFieldActionYpPrioPktAndIntNew:
          case bcmFieldActionYpPrioPktAndIntTos:
          case bcmFieldActionYpPrioPktAndIntCancel:
          case bcmFieldActionYpSwitchToCpuCancel:
          case bcmFieldActionYpSwitchToCpuReinstate:
          case bcmFieldActionYpPrioIntCopy:
          case bcmFieldActionYpPrioIntNew:
          case bcmFieldActionYpPrioIntTos:
          case bcmFieldActionYpPrioIntCancel:

          case bcmFieldActionGpDrop:
          case bcmFieldActionGpDropCancel:
          case bcmFieldActionGpDropPrecedence:
          case bcmFieldActionGpCopyToCpu:
          case bcmFieldActionGpCopyToCpuCancel:
          case bcmFieldActionGpPrioPktCopy:
          case bcmFieldActionGpPrioPktNew:
          case bcmFieldActionGpPrioPktTos:
          case bcmFieldActionGpPrioPktCancel:
          case bcmFieldActionGpDscpNew:
          case bcmFieldActionGpTosPrecedenceNew:
          case bcmFieldActionGpTosPrecedenceCopy:
          case bcmFieldActionGpDscpCancel:
          case bcmFieldActionGpEcnNew:
          case bcmFieldActionGpCosQNew:
          case bcmFieldActionGpVlanCosQNew:
          case bcmFieldActionGpPrioPktAndIntCopy:
          case bcmFieldActionGpPrioPktAndIntNew:
          case bcmFieldActionGpPrioPktAndIntTos:
          case bcmFieldActionGpPrioPktAndIntCancel:
          case bcmFieldActionGpSwitchToCpuCancel:
          case bcmFieldActionGpSwitchToCpuReinstate:
          case bcmFieldActionGpPrioIntCopy:
          case bcmFieldActionGpPrioIntNew:
          case bcmFieldActionGpPrioIntTos:
          case bcmFieldActionGpPrioIntCancel:
              *result = TRUE;
              break;
          case bcmFieldActionMultipathHash:
              *result = SOC_MEM_FIELD_VALID(unit, FP_POLICY_TABLEm, ECMP_HASH_SELf);
              break;
          case bcmFieldActionIpFix:
          case bcmFieldActionIpFixCancel:
              *result = soc_feature(unit, soc_feature_ipfix) ? TRUE : FALSE;
              break;
          case bcmFieldActionOamUpMep:
          case bcmFieldActionOamTx:
          case bcmFieldActionOamLmepMdl:
          case bcmFieldActionOamServicePriMappingPtr:
          case bcmFieldActionOamLmBasePtr:
          case bcmFieldActionOamDmEnable:
          case bcmFieldActionOamLmEnable:
          case bcmFieldActionOamLmepEnable:
              *result = soc_feature(unit,
                            soc_feature_field_oam_actions) ? TRUE : FALSE;
              break;
          case bcmFieldActionTimeStampToCpu:
          case bcmFieldActionRpTimeStampToCpu:
          case bcmFieldActionYpTimeStampToCpu:
          case bcmFieldActionGpTimeStampToCpu:
          case bcmFieldActionTimeStampToCpuCancel:
          case bcmFieldActionRpTimeStampToCpuCancel:
          case bcmFieldActionYpTimeStampToCpuCancel:
          case bcmFieldActionGpTimeStampToCpuCancel:
              *result = soc_feature(unit, soc_feature_field_action_timestamp) ? \
                        TRUE : FALSE;
              break;
          case bcmFieldActionOffloadRedirect:
          case bcmFieldActionOffloadClassSet:
          case bcmFieldActionOffloadDropIndication:
              *result = (SOC_MEM_FIELD_VALID(unit,
                            FP_POLICY_TABLEm, HI_PRI_ACTION_CONTROLf)
                            && (stage == _BCM_FIELD_STAGE_INGRESS));
              break;
          case bcmFieldActionFabricQueue:
              *result = (soc_feature(unit,
                            soc_feature_field_action_fabric_queue)
                            && (stage == _BCM_FIELD_STAGE_INGRESS))
                            ? TRUE : FALSE;
              break;
          case bcmFieldActionSrcMacNew:
#if defined(INCLUDE_L3)
              if ((stage == _BCM_FIELD_STAGE_INGRESS)
                  && (SOC_IS_TD_TT(unit)
                      || SOC_IS_TRIUMPH2(unit)
                      || SOC_IS_APOLLO(unit)
                      || SOC_IS_KATANAX(unit)
                      || SOC_IS_ENDURO(unit)
                      || SOC_IS_HURRICANEX(unit)
                      || SOC_IS_TRIUMPH3(unit)
                      )
                  ) {
                  *result = (soc_feature(unit,
                                soc_feature_field_action_l2_change) &&
                             soc_feature(unit,
                                soc_feature_l3)) ? TRUE : FALSE;
              } else {
                *result = FALSE;
              }
#else /* INCLUDE_L3 */
              *result = FALSE;
#endif /* !INCLUDE_L3 */
              break;
          case bcmFieldActionDstMacNew:
          case bcmFieldActionOuterVlanNew:
#if defined(INCLUDE_L3)
              if (stage == _BCM_FIELD_STAGE_INGRESS) {
                  *result = soc_feature(unit,
                                soc_feature_field_action_l2_change) ? \
                                TRUE : FALSE;
              } else {
                  *result = FALSE;
              }
#else /* INCLUDE_L3 */
              *result = FALSE;
#endif /* !INCLUDE_L3 */
              break;
          case bcmFieldActionVnTagNew:
          case bcmFieldActionVnTagDelete:
              *result = soc_feature(unit, soc_feature_niv) ? TRUE : FALSE;
              break;
          case bcmFieldActionCnmCancel:
              *result = SOC_MEM_FIELD_VALID(unit, FP_POLICY_TABLEm,
                                            DO_NOT_GENERATE_CNMf);
              break;
#if defined(BCM_TRIDENT_SUPPORT)
          case bcmFieldActionDynamicHgTrunkCancel:
              if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT(unit) ||
                  SOC_IS_TRIDENT2(unit)) {
                  *result = TRUE;
              } else {
                  *result = FALSE;
              }
              break;
#endif
          case bcmFieldActionUcastCosQNew:
          case bcmFieldActionGpUcastCosQNew:
          case bcmFieldActionYpUcastCosQNew:
          case bcmFieldActionRpUcastCosQNew:
              *result = (soc_feature(unit, soc_feature_ets) || 
                             SOC_IS_KATANAX(unit)) ? TRUE : FALSE;
              break;
          case bcmFieldActionMcastCosQNew:
          case bcmFieldActionGpMcastCosQNew:
          case bcmFieldActionYpMcastCosQNew:
          case bcmFieldActionRpMcastCosQNew:
              *result = soc_feature(unit, soc_feature_ets) ? TRUE : FALSE;
              break;
          case bcmFieldActionServicePoolIdNew:
              if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
                  SOC_IS_TD_TT(unit) || SOC_IS_VALKYRIE2(unit)) {
                  *result = TRUE;
              } else {
                  *result = FALSE;
              }
              break;
          case bcmFieldActionNatCancel:
          case bcmFieldActionNat:
          case bcmFieldActionNatEgressOverride:
              if (0 == soc_feature(unit, soc_feature_nat)) {
                  *result = FALSE;
              }
              else {
                  *result = TRUE;
              }
              break;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
          case bcmFieldActionUseGlobalMeterColor:
              *result = soc_feature(unit, soc_feature_global_meter) ? TRUE : FALSE;
              break;
#endif /* BCM_KATANA_SUPPORT or BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)       
        case bcmFieldActionOamOlpHeaderAdd:
              if (SOC_IS_KATANA2(unit) && 
                  (soc_feature(unit, soc_feature_oam))) {
                *result = TRUE;
            } else {
                *result = FALSE;
            } 
            break;
         case bcmFieldActionOamSessionId:
              if (SOC_IS_KATANA2(unit) && 
                  (soc_feature(unit, soc_feature_oam))) {
                *result = TRUE;
            } else {
                *result = FALSE;
            } 
            break;
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
          case bcmFieldActionTrunkResilientHashCancel:
          case bcmFieldActionHgTrunkResilientHashCancel:
          case bcmFieldActionEcmpResilientHashCancel:
              if (SOC_IS_TD2_TT2(unit)) {
                  *result = TRUE;
              } else {
                  *result = FALSE;
              }
              break;  
#endif
          default:
              *result = FALSE;
        }
    }
    return (BCM_E_NONE);
}


int
_bcm_field_trx_action_support_check(int unit, _field_entry_t *f_ent,
                                    bcm_field_action_t action, int *result)
{
    /* Input parameters check */
    if ((NULL == f_ent) || (NULL == result)) {
        return (BCM_E_PARAM);
    }
    if (NULL == f_ent->group) {
        return (BCM_E_PARAM);
    }

    return (_bcm_field_trx_stage_action_support_check(unit, f_ent->group->stage_id,
                                                      action, result));
}


/*
 * Function:
 *     _bcm_field_trx_action_conflict_check
 *
 * Purpose:
 *     Check if two action conflict (occupy the same field in FP policy table)
 *
 * Parameters:
 *     unit    -(IN)BCM device number
 *     f_ent   -(IN)Field entry structure.
 *     action -(IN) Action to check(bcmFieldActionXXX)
 *     action1 -(IN) Action to check(bcmFieldActionXXX)
 *
 * Returns:
 *     BCM_E_CONFIG - if actions do conflict
 *     BCM_E_NONE   - if there is no conflict
 */
int
_bcm_field_trx_stage_action_conflict_check(int                unit,
                                           unsigned           stage,
                                           bcm_field_action_t action1,
                                           bcm_field_action_t action
                                           )
{
    /* Two identical actions are forbidden. */
    if (action1 == action) {
        if ((action != bcmFieldActionMirrorIngress) &&
            (action != bcmFieldActionMirrorEgress)) {
            return (BCM_E_CONFIG);
        }
    }

    if (stage == _BCM_FIELD_STAGE_EGRESS) {
        switch (action1) {
          case bcmFieldActionDrop:
          case bcmFieldActionDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDropCancel);
              break;
          case bcmFieldActionRpDrop:
          case bcmFieldActionRpDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDropCancel);
              break;
          case bcmFieldActionYpDrop:
          case bcmFieldActionYpDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDropCancel);
              break;
          case bcmFieldActionGpDrop:
          case bcmFieldActionGpDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDropCancel);
              break;

          case bcmFieldActionCopyToCpu:
          case bcmFieldActionCopyToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpuCancel);
              break;
          case bcmFieldActionRpCopyToCpu:
          case bcmFieldActionRpCopyToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpuCancel);
              break;
          case bcmFieldActionYpCopyToCpu:
          case bcmFieldActionYpCopyToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpuCancel);
              break;
          case bcmFieldActionGpCopyToCpu:
          case bcmFieldActionGpCopyToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpuCancel);
              break;
          case bcmFieldActionOuterVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpOuterVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpOuterVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpOuterVlanPrioNew);
              break;
          case bcmFieldActionRpOuterVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpOuterVlanPrioNew);
              break;
          case bcmFieldActionYpOuterVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpOuterVlanPrioNew);
              break;
          case bcmFieldActionGpOuterVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpOuterVlanPrioNew);
              break;
          case bcmFieldActionInnerVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpInnerVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpInnerVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpInnerVlanPrioNew);
              break;
          case bcmFieldActionRpInnerVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpInnerVlanPrioNew);
              break;
          case bcmFieldActionYpInnerVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpInnerVlanPrioNew);
              break;
          case bcmFieldActionGpInnerVlanPrioNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanPrioNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpInnerVlanPrioNew);
              break;
          case bcmFieldActionIpFix:
          case bcmFieldActionIpFixCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionIpFix);
              _FP_ACTIONS_CONFLICT(bcmFieldActionIpFixCancel);
              break;
          case bcmFieldActionOuterVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpOuterVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpOuterVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpOuterVlanCfiNew);
              break;
          case bcmFieldActionRpOuterVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpOuterVlanCfiNew);
              break;
          case bcmFieldActionYpOuterVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpOuterVlanCfiNew);
              break;
          case bcmFieldActionGpOuterVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpOuterVlanCfiNew);
              break;
          case bcmFieldActionInnerVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpInnerVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpInnerVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpInnerVlanCfiNew);
              break;
          case bcmFieldActionRpInnerVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpInnerVlanCfiNew);
              break;
          case bcmFieldActionYpInnerVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpInnerVlanCfiNew);
              break;
          case bcmFieldActionGpInnerVlanCfiNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanCfiNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpInnerVlanCfiNew);
              break;
          case bcmFieldActionInnerVlanNew:
          case bcmFieldActionOuterVlanNew:
          case bcmFieldActionOuterTpidNew:
          case bcmFieldActionRpDscpNew:
          case bcmFieldActionYpDscpNew:
          case bcmFieldActionGpDscpNew:
              break;
          default:
              break;
        }
        return (BCM_E_NONE);
    }
    if (stage == _BCM_FIELD_STAGE_LOOKUP) {
        switch (action1) {
        case bcmFieldActionDrop:
        case bcmFieldActionDropCancel:
            _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
            _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
            break;
        case bcmFieldActionCopyToCpu:
        case bcmFieldActionCopyToCpuCancel:
            _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
            _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
            break;
        case bcmFieldActionInnerVlanAdd:
        case bcmFieldActionInnerVlanNew:
        case bcmFieldActionInnerVlanDelete:
            _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanAdd);
            _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanDelete);
            break;
        case bcmFieldActionOuterVlanAdd:
        case bcmFieldActionOuterVlanNew:
        case bcmFieldActionOuterVlanLookup:
            _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanAdd);
            _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanLookup);
            break;
        case bcmFieldActionPrioPktNew:
            _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
            break;
        case bcmFieldActionPrioIntNew:
            _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
            break;
        case bcmFieldActionPrioPktAndIntNew:
            _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
            break;
        case bcmFieldActionCosQCpuNew:
            _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
            _FP_ACTIONS_CONFLICT(bcmFieldActionServicePoolIdNew);
            break;
        case bcmFieldActionDropPrecedence:
        case bcmFieldActionVrfSet:
        case bcmFieldActionDoNotLearn:
        case bcmFieldActionClassDestSet:
        case bcmFieldActionClassSourceSet:
        case bcmFieldActionOamPbbteLookupEnable:
            break;
        case bcmFieldActionDoNotCheckVlan:
        case bcmFieldActionIncomingMplsPortSet:
        case bcmFieldActionIngressGportSet:
        case bcmFieldActionL3IngressSet:             
            _FP_ACTIONS_CONFLICT(bcmFieldActionDoNotCheckVlan);
            _FP_ACTIONS_CONFLICT(bcmFieldActionIncomingMplsPortSet);
            _FP_ACTIONS_CONFLICT(bcmFieldActionIngressGportSet);
            _FP_ACTIONS_CONFLICT(bcmFieldActionL3IngressSet);
            break;
        case bcmFieldActionOuterVlanCopyInner:
            _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanCopyOuter);
            break;
        case bcmFieldActionOuterVlanPrioCopyInner:
            _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanPrioCopyOuter);
            break;
        case bcmFieldActionOuterVlanCfiCopyInner:
            _FP_ACTIONS_CONFLICT(bcmFieldActionInnerVlanCfiCopyOuter);
            break;
        case bcmFieldActionInnerVlanCopyOuter:
            _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanCopyInner);
            break;
        case bcmFieldActionInnerVlanPrioCopyOuter:
            _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanPrioCopyInner);
            break;
        case bcmFieldActionInnerVlanCfiCopyOuter:
            _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanCfiCopyInner);
            break;
        default:
            break;
        }
        return (BCM_E_NONE);
    }

    if ((stage == _BCM_FIELD_STAGE_INGRESS) ||
        (stage == _BCM_FIELD_STAGE_EXTERNAL)) {
        switch (action1) {

          case bcmFieldActionCopyToCpuCancel:
          case bcmFieldActionSwitchToCpuCancel:
#if defined(BCM_TRIUMPH3_SUPPORT)
              if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) || 
                  SOC_IS_KATANA2(unit)) {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuReinstate);
              } else 
#endif
              {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuReinstate);
              }      
              break;
          case bcmFieldActionCopyToCpu:
          case bcmFieldActionSwitchToCpuReinstate:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuReinstate);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuReinstate);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuReinstate);
              break;
          case bcmFieldActionRpCopyToCpuCancel:
          case bcmFieldActionRpSwitchToCpuCancel:
#if defined(BCM_TRIUMPH3_SUPPORT)
              if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) || 
                  SOC_IS_KATANA2(unit)) {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuReinstate);
              } else
#endif
              {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuReinstate);  
              }
              break; 
          case bcmFieldActionRpCopyToCpu:
          case bcmFieldActionRpSwitchToCpuReinstate:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpSwitchToCpuReinstate);
              break;
          case bcmFieldActionYpCopyToCpuCancel:
          case bcmFieldActionYpSwitchToCpuCancel:
#if defined(BCM_TRIUMPH3_SUPPORT)
              if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) || 
                  SOC_IS_KATANA2(unit)) {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuReinstate);
              } else 
#endif
              {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuReinstate);  
              } 
              break;
          case bcmFieldActionYpCopyToCpu:
          case bcmFieldActionYpSwitchToCpuReinstate:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpSwitchToCpuReinstate);
              break;
          case bcmFieldActionGpCopyToCpuCancel:
          case bcmFieldActionGpSwitchToCpuCancel:
#if defined(BCM_TRIUMPH3_SUPPORT)
              if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit) || 
                  SOC_IS_KATANA2(unit)) {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuReinstate);
              } else
#endif
              {
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuCancel);
                  _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuReinstate);
              }
              break;
          case bcmFieldActionGpCopyToCpu:
          case bcmFieldActionGpSwitchToCpuReinstate:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSwitchToCpuReinstate);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCopyToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpSwitchToCpuReinstate);
              break;
          case bcmFieldActionDrop:
          case bcmFieldActionDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDropCancel);
              break;
          case bcmFieldActionRpDrop:
          case bcmFieldActionRpDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDropCancel);
              break;
          case bcmFieldActionYpDrop:
          case bcmFieldActionYpDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDropCancel);
              break;
          case bcmFieldActionGpDrop:
          case bcmFieldActionGpDropCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDrop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDropCancel);
              break;
          case bcmFieldActionL3ChangeVlan:
          case bcmFieldActionL3ChangeVlanCancel:
          case bcmFieldActionL3ChangeMacDa:
          case bcmFieldActionL3ChangeMacDaCancel:
          case bcmFieldActionL3Switch:
          case bcmFieldActionL3SwitchCancel:
          case bcmFieldActionAddClassTag:
          case bcmFieldActionFabricQueue:
              _FP_ACTIONS_CONFLICT(bcmFieldActionAddClassTag);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlanCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDa);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDaCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3Switch);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3SwitchCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSrcMacNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDstMacNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionFabricQueue);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVnTagNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVnTagDelete);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectEgrNextHop);
              break;
          case bcmFieldActionOuterVlanNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionFabricQueue);
              _FP_ACTIONS_CONFLICT(bcmFieldActionAddClassTag);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlanCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDa);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDaCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3Switch);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3SwitchCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectEgrNextHop);
              break;
          case bcmFieldActionVnTagDelete:
          case bcmFieldActionVnTagNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionVnTagDelete);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVnTagNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionFabricQueue);
              _FP_ACTIONS_CONFLICT(bcmFieldActionAddClassTag);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlanCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDa);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDaCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3Switch);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3SwitchCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectEgrNextHop);
              break;
          case bcmFieldActionSrcMacNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionSrcMacNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionFabricQueue);
              _FP_ACTIONS_CONFLICT(bcmFieldActionAddClassTag);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlanCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDa);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDaCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3Switch);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3SwitchCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectEgrNextHop);
              break;
          case bcmFieldActionDstMacNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDstMacNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionFabricQueue);
              _FP_ACTIONS_CONFLICT(bcmFieldActionAddClassTag);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlanCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDa);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDaCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3Switch);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3SwitchCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectEgrNextHop);
              break;
          case bcmFieldActionRedirect:
          case bcmFieldActionRedirectTrunk:
          case bcmFieldActionRedirectCancel:
          case bcmFieldActionRedirectPbmp:
          case bcmFieldActionRedirectIpmc:
          case bcmFieldActionRedirectMcast:
          case bcmFieldActionRedirectVlan:
          case bcmFieldActionRedirectBcastPbmp:
          case bcmFieldActionOffloadRedirect:
          case bcmFieldActionEgressMask:
          case bcmFieldActionEgressPortsAdd:
          case bcmFieldActionRedirectEgrNextHop:
              _FP_ACTIONS_CONFLICT(bcmFieldActionOffloadRedirect);
              _FP_ACTIONS_CONFLICT(bcmFieldActionEgressMask);
              _FP_ACTIONS_CONFLICT(bcmFieldActionEgressPortsAdd);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirect);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectTrunk);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectPbmp);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectIpmc);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectMcast);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectBcastPbmp);
              break;
          case bcmFieldActionPrioPktCopy:
          case bcmFieldActionPrioPktNew:
          case bcmFieldActionPrioPktTos:
          case bcmFieldActionPrioPktCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCancel);
              break;
          case bcmFieldActionPrioPktAndIntCopy:
          case bcmFieldActionPrioPktAndIntNew:
          case bcmFieldActionPrioPktAndIntTos:
          case bcmFieldActionPrioPktAndIntCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionPrioIntNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntNew);
          /*
           * coverity[fallthrough]
           * This is intentional as the rest of the statements should be
           * executed for this case also
           */
          case bcmFieldActionPrioIntCopy:
          case bcmFieldActionPrioIntTos:
          case bcmFieldActionPrioIntCancel:
          case bcmFieldActionVlanCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionUcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionMcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionRpPrioPktCopy:
          case bcmFieldActionRpPrioPktNew:
          case bcmFieldActionRpPrioPktTos:
          case bcmFieldActionRpPrioPktCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCancel);
              break;
          case bcmFieldActionRpPrioPktAndIntCopy:
          case bcmFieldActionRpPrioPktAndIntNew:
          case bcmFieldActionRpPrioPktAndIntTos:
          case bcmFieldActionRpPrioPktAndIntCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              break;
          case bcmFieldActionRpPrioIntCopy:
          case bcmFieldActionRpPrioIntNew:
          case bcmFieldActionRpPrioIntTos:
          case bcmFieldActionRpPrioIntCancel:
          case bcmFieldActionRpCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              break;
          case bcmFieldActionRpUcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              break;
          case bcmFieldActionRpMcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPortPrioIntCosQNew);
              break;
          case bcmFieldActionYpPrioPktCopy:
          case bcmFieldActionYpPrioPktNew:
          case bcmFieldActionYpPrioPktTos:
          case bcmFieldActionYpPrioPktCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCancel);
              break;
          case bcmFieldActionYpPrioPktAndIntCopy:
          case bcmFieldActionYpPrioPktAndIntNew:
          case bcmFieldActionYpPrioPktAndIntTos:
          case bcmFieldActionYpPrioPktAndIntCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              break;
          case bcmFieldActionYpPrioIntCopy:
          case bcmFieldActionYpPrioIntNew:
          case bcmFieldActionYpPrioIntTos:
          case bcmFieldActionYpPrioIntCancel:
          case bcmFieldActionYpCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              break;
          case bcmFieldActionYpUcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              break;
          case bcmFieldActionYpMcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              break;
          case bcmFieldActionGpPrioPktCopy:
          case bcmFieldActionGpPrioPktNew:
          case bcmFieldActionGpPrioPktTos:
          case bcmFieldActionGpPrioPktCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCancel);
              break;
          case bcmFieldActionGpPrioPktAndIntCopy:
          case bcmFieldActionGpPrioPktAndIntNew:
          case bcmFieldActionGpPrioPktAndIntTos:
          case bcmFieldActionGpPrioPktAndIntCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionGpPrioIntCopy:
          case bcmFieldActionGpPrioIntNew:
          case bcmFieldActionGpPrioIntTos:
          case bcmFieldActionGpPrioIntCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionGpCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionGpUcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionGpMcastCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQCpuNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPortPrioIntCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPortPrioIntCosQNew);
              break;
          case bcmFieldActionDropPrecedence:
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDropPrecedence);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDropPrecedence);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDropPrecedence);
              break;
          case bcmFieldActionRpDropPrecedence:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropPrecedence);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDropPrecedence);
              break;
          case bcmFieldActionYpDropPrecedence:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropPrecedence);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDropPrecedence);
              break;
          case bcmFieldActionGpDropPrecedence:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDropPrecedence);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDropPrecedence);
              break;
          case bcmFieldActionTimeStampToCpu:
          case bcmFieldActionTimeStampToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpTimeStampToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpTimeStampToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTimeStampToCpuCancel);
              break;
          case bcmFieldActionRpTimeStampToCpu:
          case bcmFieldActionRpTimeStampToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpTimeStampToCpuCancel);
              break;
          case bcmFieldActionYpTimeStampToCpu:
          case bcmFieldActionYpTimeStampToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpTimeStampToCpuCancel);
              break;
          case bcmFieldActionGpTimeStampToCpu:
          case bcmFieldActionGpTimeStampToCpuCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionTimeStampToCpuCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTimeStampToCpu);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTimeStampToCpuCancel);
              break;
          case bcmFieldActionIpFix:
          case bcmFieldActionIpFixCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionIpFix);
              _FP_ACTIONS_CONFLICT(bcmFieldActionIpFixCancel);
              break;
          case bcmFieldActionDscpNew:
          case bcmFieldActionDscpCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDscpCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDscpCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTosPrecedenceNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTosPrecedenceCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDscpCancel);
              break;
          case bcmFieldActionRpDscpNew:
          case bcmFieldActionRpDscpCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpDscpCancel);
              break;
          case bcmFieldActionYpDscpNew:
          case bcmFieldActionYpDscpCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpDscpCancel);
              break;
          case bcmFieldActionGpDscpNew:
          case bcmFieldActionGpDscpCancel:
          case bcmFieldActionGpTosPrecedenceNew:
          case bcmFieldActionGpTosPrecedenceCopy:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDscpCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDscpNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTosPrecedenceNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpTosPrecedenceCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpDscpCancel);
              break;
          case bcmFieldActionEcnNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpEcnNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpEcnNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpEcnNew);
              break;
          case bcmFieldActionRpEcnNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionEcnNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpEcnNew);
              break;
          case bcmFieldActionYpEcnNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionEcnNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpEcnNew);
              break;
          case bcmFieldActionGpEcnNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionEcnNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpEcnNew);
              break;
          case bcmFieldActionMirrorIngress:
              _FP_ACTIONS_CONFLICT(bcmFieldActionMirrorOverride);
              break;
          case bcmFieldActionMirrorEgress:
              _FP_ACTIONS_CONFLICT(bcmFieldActionMirrorOverride);
              break;
          case bcmFieldActionMirrorOverride:
              _FP_ACTIONS_CONFLICT(bcmFieldActionMirrorIngress);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMirrorEgress);
              break;
          case bcmFieldActionPortPrioIntCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              break;
          case bcmFieldActionRpPortPrioIntCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRpMcastCosQNew);
              break;
          case bcmFieldActionYpPortPrioIntCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionYpMcastCosQNew);
              break;
          case bcmFieldActionGpPortPrioIntCosQNew:
              _FP_ACTIONS_CONFLICT(bcmFieldActionCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpVlanCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCopy);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntTos);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpPrioPktAndIntCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpUcastCosQNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionGpMcastCosQNew);
              break;
          case bcmFieldActionColorIndependent:
              _FP_ACTIONS_CONFLICT(bcmFieldActionColorIndependent);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeVlanCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDa);
              _FP_ACTIONS_CONFLICT(bcmFieldActionSrcMacNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionDstMacNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3ChangeMacDaCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3Switch);
              _FP_ACTIONS_CONFLICT(bcmFieldActionL3SwitchCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionAddClassTag);
              _FP_ACTIONS_CONFLICT(bcmFieldActionOuterVlanNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectVlan);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectMcast);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectIpmc);
              _FP_ACTIONS_CONFLICT(bcmFieldActionEgressPortsAdd);
              _FP_ACTIONS_CONFLICT(bcmFieldActionEgressMask);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirect);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectTrunk);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectEgrNextHop);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVnTagNew);
              _FP_ACTIONS_CONFLICT(bcmFieldActionVnTagDelete);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectPbmp);
              _FP_ACTIONS_CONFLICT(bcmFieldActionOffloadRedirect);
              _FP_ACTIONS_CONFLICT(bcmFieldActionRedirectBcastPbmp);
              _FP_ACTIONS_CONFLICT(bcmFieldActionFabricQueue);
              _FP_ACTIONS_CONFLICT(bcmFieldActionMirrorEgress);
             break;
          case bcmFieldActionDynamicHgTrunkCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionTrunkLoadBalanceCancel);
             break;
          case bcmFieldActionTrunkLoadBalanceCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionDynamicHgTrunkCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionTrunkResilientHashCancel);
              _FP_ACTIONS_CONFLICT(bcmFieldActionHgTrunkResilientHashCancel);
              break;
          case bcmFieldActionHgTrunkResilientHashCancel:
          case bcmFieldActionTrunkResilientHashCancel:
              _FP_ACTIONS_CONFLICT(bcmFieldActionTrunkLoadBalanceCancel);
             break;
          case bcmFieldActionDoNotChangeTtl:
          case bcmFieldActionDoNotCheckUrpf:
          case bcmFieldActionMultipathHash:
          case bcmFieldActionOamUpMep:
          case bcmFieldActionOamTx:
          case bcmFieldActionOamLmepMdl:
          case bcmFieldActionOamServicePriMappingPtr:
          case bcmFieldActionOamLmBasePtr:
          case bcmFieldActionOamDmEnable:
          case bcmFieldActionOamLmEnable:
          case bcmFieldActionOamLmepEnable:
          case bcmFieldActionOffloadClassSet:
          case bcmFieldActionOffloadDropIndication:
          case bcmFieldActionCnmCancel:
              break;
          default:
              break;
        }
    }
    return (BCM_E_NONE);
}


int
_bcm_field_trx_action_conflict_check(int                unit,
                                     _field_entry_t     *f_ent,
                                     bcm_field_action_t action1,
                                     bcm_field_action_t action
                                     )
{
    /* Input parameters check */
    if (NULL == f_ent) {
        return (BCM_E_PARAM);
    }
    if (NULL == f_ent->group) {
        return (BCM_E_PARAM);
    }

    return (_bcm_field_trx_stage_action_conflict_check(unit, f_ent->group->stage_id, action1, action));
}

/*
 * Function:
 *     _bcm_field_trx_egress_key_match_type_set
 *
 * Purpose:
 *     Set key match type based on entry group.
 *     NOTE: For double wide entries key type must be the same for
 *           both parts of the entry.
 * Parameters:
 *     unit   - (IN) BCM device number
 *     f_ent  - (IN) Slice number to enable
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_egress_key_match_type_set (int unit, _field_entry_t *f_ent)
{
    _field_group_t           *fg;   /* Field group entry belongs to. */
    uint32                   data;  /* Key match type.               */
    uint32                   mask;  /* Key match type mask.          */
    /* Key match type offset.        */
    _bcm_field_qual_offset_t q_offset = {KEYf, 207, 3, 0, 0, 0, 0};

    /* Input parameters check. */
    if (NULL == f_ent) {
        return (BCM_E_PARAM);
    }
    if (NULL == (fg = f_ent->group)) {
        return (BCM_E_PARAM);
    }

    if (fg->flags & _FP_GROUP_SPAN_SINGLE_SLICE) {
        switch (fg->sel_codes[0].fpf3) {
          case _BCM_FIELD_EFP_KEY1:
              data = _BCM_FIELD_EFP_KEY1_MATCH_TYPE;
              break;
          case _BCM_FIELD_EFP_KEY2:
              data = _BCM_FIELD_EFP_KEY2_MATCH_TYPE;
              break;
          case _BCM_FIELD_EFP_KEY4:
              data = _BCM_FIELD_EFP_KEY4_MATCH_TYPE;
              break;
          case _BCM_FIELD_EFP_KEY5:
              data = _BCM_FIELD_EFP_KEY1_MATCH_TYPE;
              break;
          default:
              return (BCM_E_INTERNAL);
        }
    } else {
        switch (fg->sel_codes[1].fpf3) {
          case _BCM_FIELD_EFP_KEY2:
              data = _BCM_FIELD_EFP_KEY2_KEY3_MATCH_TYPE;
              break;
          case _BCM_FIELD_EFP_KEY4:
              if ((_BCM_FIELD_EFP_KEY1 == fg->sel_codes[0].fpf3) ||
                  (_BCM_FIELD_EFP_KEY5 == fg->sel_codes[0].fpf3)) {
                  data = _BCM_FIELD_EFP_KEY1_KEY4_MATCH_TYPE;
              } else {
                  data = _BCM_FIELD_EFP_KEY2_KEY4_MATCH_TYPE;
              }
              break;
          default:
              return (BCM_E_INTERNAL);
        }
    }

    if (SOC_IS_TD2_TT2(unit)) {
        q_offset.offset += 23;
        q_offset.width += 1;
    }
    else if (SOC_IS_TR_VL(unit)) {
        q_offset.offset += 4;
    }
    
    if (SOC_IS_TD2_TT2(unit)) {
        mask = 0xf;
    }
    else {
        mask = 0x7;
    }
   return _bcm_field_qual_value_set(unit, &q_offset, f_ent, &data, &mask);
}

/*
 * Function:
 *     _bcm_field_trx_qualify_ip_type
 * Purpose:
 *     Install ip type qualifier into TCAM
 * Parameters:
 *     unit  - (IN) BCM device number
 *     f_ent - (IN) Field entry qualifier
 *     type  - (IN) Ip Type.
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
#define BCM_FIELD_IPTYPE_BAD 0xff
int
_bcm_field_trx_qualify_ip_type(int unit, _field_entry_t *f_ent,
                               bcm_field_IpType_t type)
{

    _field_group_t      *fg;
    uint32              data = BCM_FIELD_IPTYPE_BAD,
                        mask = BCM_FIELD_IPTYPE_BAD;

    fg = f_ent->group;
    if (NULL == fg) {
        return (BCM_E_INTERNAL);
    }

    switch (type) {
      case bcmFieldIpTypeAny:
          data = 0x0;
          mask = 0x0;
          break;
      case bcmFieldIpTypeNonIp:
          data = 0xf;
          mask = 0xf;
          break;
      case bcmFieldIpTypeIp:
          data = 0x0;
          mask = 0x8;
          break;
          break;
      case bcmFieldIpTypeIpv4NoOpts:
          data = 0x0;
          mask = 0xf;
          break;
      case bcmFieldIpTypeIpv4WithOpts:
          data = 0x1;
          mask = 0xf;
          break;
      case bcmFieldIpTypeIpv4Any:
          data = 0x0;
          mask = 0xe;
          break;
      case bcmFieldIpTypeIpv6NoExtHdr:
          data = 0x4;
          mask = 0xf;
          break;
      case bcmFieldIpTypeIpv6OneExtHdr:
          data = 0x5;
          mask = 0xf;
          break;
      case bcmFieldIpTypeIpv6TwoExtHdr:
          data = 0x6;
          mask = 0xf;
          break;
      case bcmFieldIpTypeIpv6:
          data = 0x4;
          mask = 0xc;
          break;
      case bcmFieldIpTypeArp:
          data = 0x8;
          mask = 0xe;
          break;
      case bcmFieldIpTypeArpRequest:
          data = 0x8;
          mask = 0xf;
          break;
      case bcmFieldIpTypeArpReply:
          data = 0x9;
          mask = 0xf;
          break;
      case bcmFieldIpTypeTrill:
          data = 0xa;
          mask = 0xf;
          break;
      case bcmFieldIpTypeFCoE:
#if defined(BCM_TRIDENT2_SUPPORT)
      if (SOC_IS_TD2_TT2(unit)) {
              data = 0xb;
              mask = 0xf;
      }
#endif
      break;
      case bcmFieldIpTypeMplsUnicast:
          data = 0xc;
          mask = 0xf;
          break;
      case bcmFieldIpTypeMplsMulticast:
          data = 0xd;
          mask = 0xf;
          break;
      case bcmFieldIpTypeMim:
          data = 0xe;
          mask = 0xf;
          break;
      default:
          break;
    }
    FP_VVERB(("FP(unit %d) vverb: entry=%d qualifying on Iptype, data=%#x, mask=%#x\n))",
             unit, f_ent->eid, data, mask));

    if ((data == BCM_FIELD_IPTYPE_BAD) ||
        (mask == BCM_FIELD_IPTYPE_BAD)) {
        return (BCM_E_UNAVAIL);
    }

    return _field_qualify32(unit, f_ent->eid, bcmFieldQualifyIpType,
                            data, mask);
}
#undef BCM_FIELD_IPTYPE_BAD
/*
 * Function:
 *     _bcm_field_trx_qualify_ip_type_get
 * Purpose:
 *     Read ip type qualifier match criteria from the HW.
 * Parameters:
 *     unit  - (IN) BCM device number
 *     f_ent - (IN) Field entry qualifier
 *     type  - (OUT) Ip Type.
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
int
_bcm_field_trx_qualify_ip_type_get(int unit, _field_entry_t *f_ent,
                              bcm_field_IpType_t *type)
{
    _field_group_t *fg;      /* Field group structure.      */
    uint32 hw_data;          /* HW encoded qualifier data.  */
    uint32 hw_mask;          /* HW encoding qualifier mask. */
    int rv;                  /* Operation return status.    */

    /* Input parameters checks. */
    if ((NULL == f_ent) || (NULL == type)) {
        return (BCM_E_PARAM);
    }

    fg = f_ent->group;
    if (NULL == fg) {
        return (BCM_E_INTERNAL);
    }

    /* Read qualifier match value and mask. */
    rv = _bcm_field_entry_qualifier_uint32_get(unit, f_ent->eid,
                                               bcmFieldQualifyIpType,
                                               &hw_data, &hw_mask);
    BCM_IF_ERROR_RETURN(rv);

    if ((0 == hw_data) && (0 == hw_mask)) {
        *type = bcmFieldIpTypeAny;
    } else if ((0xf == hw_data) && (0xf == hw_mask)) {
        *type = bcmFieldIpTypeNonIp;
    } else if ((0x0 == hw_data) && (0x8 == hw_mask)) {
        *type = bcmFieldIpTypeIp;
    } else if ((0x0 == hw_data) && (0xf == hw_mask)) {
        *type = bcmFieldIpTypeIpv4NoOpts;
    } else if ((0x1 == hw_data) && (0xf == hw_mask)) {
        *type = bcmFieldIpTypeIpv4WithOpts;
    } else if ((0x0 == hw_data) && (0xe == hw_mask)) {
        *type = bcmFieldIpTypeIpv4Any;
    } else if ((0x4 == hw_data) && (0xc == hw_mask)) {
        *type = bcmFieldIpTypeIpv6;
    } else if ((0x8 == hw_data) && (0xe == hw_mask)) {
        *type = bcmFieldIpTypeArp;
    } else if ((0x8 == hw_data) && (0xf == hw_mask)) {
        *type = bcmFieldIpTypeArpRequest;
    } else if ((0x9 == hw_data) && (0xf == hw_mask)) {
        *type = bcmFieldIpTypeArpReply;
    } else 
#if defined(BCM_TRIDENT2_SUPPORT)
    if ((0xb == hw_data) && (0xf == hw_mask) && SOC_IS_TD2_TT2(unit)) {
            *type = bcmFieldIpTypeFCoE;
    }  else
#endif
    if ((0xc == hw_data) && (0xf == hw_mask)) {
        *type = bcmFieldIpTypeMplsUnicast;
    } else if ((0xd == hw_data) && (0xf == hw_mask)) {
        *type = bcmFieldIpTypeMplsMulticast;
    } else 
    {
        return (BCM_E_INTERNAL);
    }
    return (BCM_E_NONE);
}


/*
 * Function:
 *     _bcm_field_trx_range_check_set
 * Purpose:
 *     Write the group's range checking parameters into the
 *     FP_RANGE_CHECK  memory.
 * Parameters:
 *     unit   - (IN) BCM device number.
 *     range  - (IN) Range HW index
 *     flags  - (IN) One of more of the BCM_FIELD_RANGE_* flags
 *     enable - (IN) TRUE or FALSE
 *     min    - (IN) Lower bounds of port range to be checked
 *     max    - (IN) Upper bounds of port range to be checked
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_range_check_set(int unit, int range, uint32 flags, int enable,
                               bcm_l4_port_t min, bcm_l4_port_t max)
{
                            /* Range table entry buffer. */
    uint32                  tbl_entry[SOC_MAX_MEM_FIELD_WORDS];

    soc_mem_t               fp_range_check_mem;     /* Range table memory. */
    uint32                  range_type_encoded = 0; /* Range type encoded. */
    uint32                  range_type = 0;         /* Range type.         */

#if defined(BCM_TRIUMPH_SUPPORT)
    if (flags & BCM_FIELD_RANGE_EXTERNAL) {
        fp_range_check_mem = ESM_RANGE_CHECKm;
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
    {
        fp_range_check_mem = FP_RANGE_CHECKm;
    }

    /* Index sanity check. */
    if (!soc_mem_index_valid(unit, fp_range_check_mem, range)) {
        return (BCM_E_PARAM);
    }

    if (enable) {
        range_type = flags & (BCM_FIELD_RANGE_SRCPORT |    \
                              BCM_FIELD_RANGE_DSTPORT |    \
                              BCM_FIELD_RANGE_OUTER_VLAN | \
                              BCM_FIELD_RANGE_PACKET_LENGTH);
        switch (range_type) {
          case BCM_FIELD_RANGE_SRCPORT:
              range_type_encoded = 0x0;
              break;
          case BCM_FIELD_RANGE_DSTPORT:
              range_type_encoded = 0x1;
              break;
          case BCM_FIELD_RANGE_OUTER_VLAN:
              range_type_encoded = 0x2;
              break;
          case BCM_FIELD_RANGE_PACKET_LENGTH:
              range_type_encoded = 0x3;
              break;
          default:
              FP_ERR(("FP(unit %d) Error: unsupported flags %#x\n", unit, flags));
              return (BCM_E_PARAM);
        }
    }

    /* read/modify/write range check memory */
    sal_memset(tbl_entry, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

    /* Set source/destination port selection. */
    soc_mem_field_set(unit, fp_range_check_mem, tbl_entry,
                      FIELD_SELECTf, &range_type_encoded);

    if (SOC_MEM_FIELD_VALID(unit, fp_range_check_mem, ENABLEf)) {
        soc_mem_field_set(unit, fp_range_check_mem, tbl_entry,
                          ENABLEf, (uint32 *) &enable);
    }

    /* Set range min value. */
    soc_mem_field_set(unit, fp_range_check_mem, tbl_entry,
                      LOWER_BOUNDSf, (uint32 *) &min);

    /* Set range max value. */
    soc_mem_field_set(unit, fp_range_check_mem, tbl_entry,
                      UPPER_BOUNDSf, (uint32 *) &max);

    /* Write entry back to hw. */
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, fp_range_check_mem,
                                      MEM_BLOCK_ALL, range, tbl_entry));

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_udf_index_to_flags
 * Purpose:
 *     Translate the FP_UDF_OFFSETm  table index to API flags.
 * Parameters:
 *     tbl_idx - (IN) Index into FP_UDF_OFFSET table
 *     flags   - (OUT) Client specified flags.
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
int
_bcm_field_trx_udf_index_to_flags(int unit, uint32 tbl_idx, int *flags)
{
    int                 index;

    /* Input parameters check. */
    if (NULL == flags) {
        return (BCM_E_PARAM);
    }

    if (SOC_IS_TR_VL(unit)) {
        index = tbl_idx & 0x1f;
    } else if  (SOC_IS_SC_CQ(unit)) {
        index = tbl_idx & 0xf;
    } else {
        index = tbl_idx & 0x1f; /* TRX */
    }

    switch (index) {
      case 0x0:
          *flags |= BCM_FIELD_USER_IP4_HDR_ONLY;
          break;
      case 0x1:
          *flags |= BCM_FIELD_USER_IP6_HDR_ONLY;
          break;
      case 0x2:
          *flags |= BCM_FIELD_USER_IP4_OVER_IP4;
          break;
      case 0x3:
          *flags |= BCM_FIELD_USER_IP6_OVER_IP4;
          break;
      case 0x4:
          *flags |= BCM_FIELD_USER_IP4_OVER_IP6;
          break;
      case 0x5:
          *flags |= BCM_FIELD_USER_IP6_OVER_IP6;
          break;
      case 0x6:
          *flags |= BCM_FIELD_USER_GRE_IP4_OVER_IP4;
          break;
      case 0x7:
          *flags |= BCM_FIELD_USER_GRE_IP6_OVER_IP4;
          break;
      case 0x8:
          *flags |= BCM_FIELD_USER_GRE_IP4_OVER_IP6;
          break;
      case 0x9:
          *flags |= BCM_FIELD_USER_GRE_IP6_OVER_IP6;
          break;
      case 0xa:
          if (SOC_IS_SC_CQ(unit)) {
              *flags |= BCM_FIELD_USER_IP_NOTUSED;
          } else {
              *flags |= BCM_FIELD_USER_ONE_MPLS_LABEL;
          }
          break;
      case 0xb:
          if (SOC_IS_SC_CQ(unit)) {
                /* _BCM_FIELD_DATA_FORMAT_IP4_PROTOCOL0 */
          } else {
              *flags |= BCM_FIELD_USER_TWO_MPLS_LABELS;
          }
          break;
      case 0xc:
          if (SOC_IS_SC_CQ(unit)) {
                /* _BCM_FIELD_DATA_FORMAT_IP4_PROTOCOL1 */
          } else {
              *flags |= BCM_FIELD_USER_IP_NOTUSED;
          }
          break;
      case 0xd:
          if (SOC_IS_SC_CQ(unit)) {
                /* _BCM_FIELD_DATA_FORMAT_IP6_PROTOCOL0 */
          } else {
                /* _BCM_FIELD_DATA_FORMAT_IP4_PROTOCOL0 */
          }
      case 0xe:
          if (SOC_IS_SC_CQ(unit)) {
                /* _BCM_FIELD_DATA_FORMAT_IP6_PROTOCOL1 */
          } else {
                /* _BCM_FIELD_DATA_FORMAT_IP4_PROTOCOL1 */
          }
      case 0xf:
          if (SOC_IS_TR_VL(unit)) {
                /* _BCM_FIELD_DATA_FORMAT_IP6_PROTOCOL0 */
          }
      case 0x10:
          if (SOC_IS_TR_VL(unit)) {
                /* _BCM_FIELD_DATA_FORMAT_IP6_PROTOCOL0 */
          }
      default:
          return (BCM_E_INTERNAL);
    }

    index = (tbl_idx >> (SOC_IS_SC_CQ(unit) ?  4 : 5)) & 0x3;
    switch (index) {
      case 0x0:
          *flags |= BCM_FIELD_USER_L2_ETHERNET2;
          break;
      case 0x1:
          *flags |= BCM_FIELD_USER_L2_SNAP;
          break;
      case 0x2:
          *flags |= BCM_FIELD_USER_L2_LLC;
          break;
      default:
          return (BCM_E_INTERNAL);
    }

    index =  (tbl_idx >> (SOC_IS_SC_CQ(unit) ?  6 : 7));
    switch (index) {
      case 0x0:
          *flags |= BCM_FIELD_USER_VLAN_NOTAG;
          break;
      case 0x1:
          *flags |= BCM_FIELD_USER_VLAN_ONETAG;
          break;
      case 0x2:
          *flags |= BCM_FIELD_USER_VLAN_TWOTAG;
          break;
      default:
          return (BCM_E_INTERNAL);
    }
    return (BCM_E_NONE);
}

int
_bcm_field_trx_control_arp_set(int unit,  bcm_field_control_t control, uint32 state)
{
    uint64 regval;
    uint32 enable;

    BCM_IF_ERROR_RETURN(READ_ING_CONFIG_64r(unit, &regval));

    enable = soc_reg64_field32_get(unit, ING_CONFIG_64r,
                                   regval, ARP_RARP_TO_FPf);

    switch (control) {
      case bcmFieldControlArpAsIp:
          if (state) {
              enable |= (0x1);
          } else {
              enable &= ~(0x1);
          }
          break;
      case bcmFieldControlRarpAsIp:
          if (state) {
              enable |= (0x2);
          } else {
              enable &= ~(0x2);
          }
          break;
      default:
          return (BCM_E_UNAVAIL);
    }

    soc_reg64_field32_set(unit, ING_CONFIG_64r, &regval, ARP_RARP_TO_FPf, enable);
    BCM_IF_ERROR_RETURN(WRITE_ING_CONFIG_64r(unit, regval));
    return (BCM_E_NONE);
}

/*
 * Function:
 *   _bcm_field_trx_stat_hw_mode_to_bmap
 *
 * Description:
 *      HW counter mode to statistics bitmap.
 * Parameters:
 *      unit           - (IN) BCM device number.
 *      mode           - (IN) HW counter mode.
 *      stage_id       - (IN) Stage id.
 *      hw_bmap        - (OUT) Statistics bitmap.
 *      hw_entry_count - (OUT) Number of counters required.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_field_trx_stat_hw_mode_to_bmap(int unit, uint8 mode,
                                    _field_stage_id_t stage_id,
                                    uint32 *hw_bmap, uint8 *hw_entry_count)
{
    uint32 value = 0;
    /* Input parameters check. */
    if ((NULL == hw_bmap) || (NULL == hw_entry_count)) {
        return (BCM_E_PARAM);
    }
    switch (mode) {
      case 1:
          value |= (1 << (int)bcmFieldStatPackets);
          value |= (1 << (int)bcmFieldStatBytes);
          *hw_entry_count = 1;
          break;
      case 2:
          if (_BCM_FIELD_STAGE_EXTERNAL == stage_id) {
              value |= (1 << (int)bcmFieldStatGreenBytes);
              value |= (1 << (int)bcmFieldStatGreenPackets);
          } else {
              value |= (1 << (int)bcmFieldStatPackets);
              value |= (1 << (int)bcmFieldStatBytes);
          }
          *hw_entry_count = 1;
          break;
      case 3:
          if (_BCM_FIELD_STAGE_EXTERNAL == stage_id) {
              value |= (1 << (int)bcmFieldStatYellowBytes);
              value |= (1 << (int)bcmFieldStatYellowPackets);
              *hw_entry_count = 1;
          } else {
              value |= (1 << (int)bcmFieldStatRedBytes);
              value |= (1 << (int)bcmFieldStatRedPackets);
              value |= (1 << (int)bcmFieldStatNotRedBytes);
              value |= (1 << (int)bcmFieldStatNotRedPackets);
              *hw_entry_count = 2;
          }
          break;
      case 4:
          if (_BCM_FIELD_STAGE_EXTERNAL == stage_id) {
              value |= (1 << (int)bcmFieldStatRedBytes);
              value |= (1 << (int)bcmFieldStatRedPackets);
              *hw_entry_count = 1;
          } else {
              value |= (1 << (int)bcmFieldStatGreenBytes);
              value |= (1 << (int)bcmFieldStatGreenPackets);
              value |= (1 << (int)bcmFieldStatNotGreenBytes);
              value |= (1 << (int)bcmFieldStatNotGreenPackets);
              *hw_entry_count = 2;
          }
          break;
      case 5:
          if (_BCM_FIELD_STAGE_EXTERNAL == stage_id) {
              value |= (1 << (int)bcmFieldStatNotGreenBytes);
              value |= (1 << (int)bcmFieldStatNotGreenPackets);
              *hw_entry_count = 1;
          } else {
              value |= (1 << (int)bcmFieldStatGreenBytes);
              value |= (1 << (int)bcmFieldStatGreenPackets);
              value |= (1 << (int)bcmFieldStatRedBytes);
              value |= (1 << (int)bcmFieldStatRedPackets);
              value |= (1 << (int)bcmFieldStatNotYellowBytes);
              value |= (1 << (int)bcmFieldStatNotYellowPackets);
              *hw_entry_count = 2;
          }
          break;
      case 6:
          if (_BCM_FIELD_STAGE_EXTERNAL == stage_id) {
              value |= (1 << (int)bcmFieldStatNotYellowBytes);
              value |= (1 << (int)bcmFieldStatNotYellowPackets);
              *hw_entry_count = 1;
          } else {
              value |= (1 << (int)bcmFieldStatGreenBytes);
              value |= (1 << (int)bcmFieldStatGreenPackets);
              value |= (1 << (int)bcmFieldStatYellowBytes);
              value |= (1 << (int)bcmFieldStatYellowPackets);
              value |= (1 << (int)bcmFieldStatNotRedBytes);
              value |= (1 << (int)bcmFieldStatNotRedPackets);
              *hw_entry_count = 2;
          }
          break;
      case 7:
          if (_BCM_FIELD_STAGE_EXTERNAL == stage_id) {
              value |= (1 << (int)bcmFieldStatNotRedBytes);
              value |= (1 << (int)bcmFieldStatNotRedPackets);
              *hw_entry_count = 1;
          } else {
              value |= (1 << (int)bcmFieldStatRedBytes);
              value |= (1 << (int)bcmFieldStatRedPackets);
              value |= (1 << (int)bcmFieldStatYellowBytes);
              value |= (1 << (int)bcmFieldStatYellowPackets);
              value |= (1 << (int)bcmFieldStatNotGreenBytes);
              value |= (1 << (int)bcmFieldStatNotGreenPackets);
              *hw_entry_count = 2;
          }
          break;
      default:
              *hw_entry_count = 0;
    }
    *hw_bmap = value;
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx_meter_table_get
 * Purpose:
 *     Get policer table name.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     stage_id  - (IN) Field entry pipeline stage.
 *     mem_x     - (OUT) Policer table name X pipeline.
 *     mem_y     - (OUT) Policer table name Y pipeline.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_meter_table_get(int unit, _field_stage_id_t stage_id,
                           soc_mem_t *mem_x, soc_mem_t *mem_y)
{
    if ((NULL == mem_x) || (NULL == mem_y)) {
        return (BCM_E_PARAM);
    }

    *mem_x = *mem_y = INVALIDm;

    /* Resolve meter table name. */
    switch (stage_id) {
      case _BCM_FIELD_STAGE_INGRESS:
      case _BCM_FIELD_STAGE_EXTERNAL:
          *mem_x = FP_METER_TABLEm;
          break;
      case _BCM_FIELD_STAGE_EGRESS:
          if (SOC_IS_SC_CQ(unit) ||
                SOC_IS_TD_TT(unit)) {
              *mem_x = EFP_METER_TABLE_Xm;
              *mem_y = EFP_METER_TABLE_Ym;
          } else {
              *mem_x = EFP_METER_TABLEm;
          }
          break;
      default:
          return (BCM_E_INTERNAL);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx_policer_hw_update
 * Purpose:
 *     Update (Install) the policer parameters
 * Parameters:
 *     unit          - (IN) BCM device number.
 *     f_ent         - (IN) Field entry policer attached to.
 *     index_mtr     - (IN) Peak/Committed
 *     bucket_size   - (IN) Encoded bucket size.
 *     refresh_rate  - (IN) Tokens refresh rate.
 *     granularity   - (IN) Tokens granularity.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_policer_hw_update(int unit, _field_entry_t *f_ent,
                             _field_policer_t *f_pl, uint8 index_mtr,
                             uint32 bucket_size, uint32 refresh_rate,
                             uint32 granularity, soc_mem_t meter_table)
{
    uint32          meter_entry[SOC_MAX_MEM_FIELD_WORDS]; /* HW entry buffer.*/
    int             bucket_cnt_bitsize;      /* Bucket count field bit size. */
    int             bucket_max_bitsize;      /* Bucket size field bit size.  */
    int             meter_hw_idx;            /* Meter table index.           */
    uint32          bucketcount;             /* Bucket count initial value.  */
    int             meter_offset;            /* Peak/Comm meter selection.   */
    _field_stage_t  *stage_fc;               /* Field stage control.         */
    int             rv;                      /* Operation return status.     */

    if ((NULL == f_pl) || (NULL == f_ent) || (INVALIDm == meter_table)) {
        return (BCM_E_PARAM);
    }

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, f_ent->fs->stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);


    /* Calculate initial backet count.
     * bucket size << (bit count diff - 1(sign) -1 (overflow))  - 1
     */
    if (bucket_size) {
        bucket_cnt_bitsize = soc_mem_field_length(unit, meter_table,
                                                  BUCKETCOUNTf);
        bucket_max_bitsize = soc_mem_field_length(unit, meter_table,
                                                  BUCKETSIZEf);
        bucketcount = (bucket_size
                        << (bucket_cnt_bitsize - bucket_max_bitsize - 2))  - 1;
        bucketcount &= ((1 << bucket_cnt_bitsize) - 1);
    } else {
        bucketcount = 0;
    }

    meter_offset = (BCM_FIELD_METER_PEAK == index_mtr) ? 0 : 1;

    if (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS) {
        /* 
         * Hw index is:
         * ((Pool number * pool_size) + (2 * pair number) + meter_offset)
         */
        meter_hw_idx = (f_pl->pool_index
                        * stage_fc->meter_pool[f_pl->pool_index]->pool_size)
                        + (2 * f_pl->hw_index) + meter_offset;
        FP_VVERB(("FP(unit %d) vverb: pool_idx:%d pool_sz:%d pair_num:%d hw_idx:%d\n",
                 unit, f_pl->pool_index,
                 stage_fc->meter_pool[f_pl->pool_index]->pool_size,
                 f_pl->hw_index,
                 meter_hw_idx));
    } else {
        /* 
         * Hw index is:
         * ((slice index) + (2 * pair number) + meter_offset)
         */
        meter_hw_idx = stage_fc->slices[f_pl->pool_index].start_tcam_idx + \
                       (2 * f_pl->hw_index) + meter_offset;

        FP_VVERB(("FP(unit %d) vverb: slice_num:%d tcam_idx:%d pair_num:%d hw_idx%d\n",
                 unit, f_pl->pool_index,
                 stage_fc->slices[f_pl->pool_index].start_tcam_idx,
                 f_pl->hw_index,
                 meter_hw_idx));
    }

    if (meter_hw_idx < soc_mem_index_min(unit, meter_table) ||
        meter_hw_idx > soc_mem_index_max(unit, meter_table)) {
        return (BCM_E_INTERNAL);
    }

    /* Update meter config in hw. */
    rv = soc_mem_read(unit, meter_table, MEM_BLOCK_ANY,
                      meter_hw_idx, &meter_entry);
    BCM_IF_ERROR_RETURN(rv);

    soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESHCOUNTf,
                        refresh_rate);
    soc_mem_field32_set(unit, meter_table, &meter_entry, BUCKETSIZEf,
                        bucket_size);
    soc_mem_field32_set(unit, meter_table, &meter_entry, METER_GRANf,
                        granularity);
    soc_mem_field32_set(unit, meter_table, &meter_entry, BUCKETCOUNTf,
                        bucketcount);

    /* Refresh mode is only set to 1 for Single Rate. Other modes get 0 */
    if (f_pl->cfg.mode  == bcmPolicerModeSrTcm) {
        soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESH_MODEf, 1);
    } else if (f_pl->cfg.mode  == bcmPolicerModeCoupledTrTcmDs) {
        soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESH_MODEf, 2);
    } else {
        soc_mem_field32_set(unit, meter_table, &meter_entry, REFRESH_MODEf, 0);
    }

    if (soc_feature(unit, soc_feature_field_packet_based_metering)
        && (soc_mem_field_valid(unit, meter_table, PKTS_BYTESf))) {
        if (f_pl->cfg.flags & BCM_POLICER_MODE_PACKETS) {
            soc_mem_field32_set(unit, meter_table, &meter_entry, PKTS_BYTESf, 1);
        } else {
            soc_mem_field32_set(unit, meter_table, &meter_entry, PKTS_BYTESf, 0);
        }
    }

    rv = soc_mem_write(unit, meter_table, MEM_BLOCK_ALL,
                       meter_hw_idx, meter_entry);

    return (rv);
}


/*
 * Function:
 *     _field_trx_default_policer_set
 *
 * Purpose:
 *     Get metering portion of Policy Table.
 *
 * Parameters:
 *     unit      - (IN)BCM device number.
 *     stage_fc  - (IN)Stage control structure.
 *     level     - (IN)Policer level.
 *     mem       - (IN)Policy table memory.
 *     buf       - (IN/OUT)Hardware policy entry
 *
 * Returns:
 *     BCM_E_NONE  - Success
 *
 */
STATIC int
_field_trx_default_policer_set(int unit, _field_stage_t *stage_fc,
                               int level, soc_mem_t mem, uint32 *buf)
{
    soc_field_t             pair_mode_field;/* Hw field for pair mode.   */
    soc_field_t             modifier_field; /* Hw field for meter update.*/

    /* Input parameter check. */
    if ((NULL == stage_fc) || (NULL == buf))  {
        return (BCM_E_PARAM);
    }

    if (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS) {
        if (level) {
            pair_mode_field = METER_SHARING_MODEf;
            modifier_field = METER_SHARING_MODE_MODIFIERf;
        } else {
            pair_mode_field = METER_PAIR_MODEf;
            modifier_field = METER_PAIR_MODE_MODIFIERf;
        }

        /* Set the meter mode to default. */
        soc_mem_field32_set(unit, mem, buf, pair_mode_field, 0);
        /* Preserve packet color. */
        soc_mem_field32_set(unit, mem, buf, modifier_field, 1);
    } else if (stage_fc->stage_id == _BCM_FIELD_STAGE_EGRESS) {
        if (0 == level) {
            /* Set the meter mode to default. */
            soc_mem_field32_set(unit, mem, buf, METER_PAIR_MODEf, 0);
            /* Preserve packet color. */
            soc_mem_field32_set(unit, mem, buf, METER_TEST_EVENf, 1);
        }
    } else {
        return BCM_E_PARAM;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx_policer_action_set
 *
 * Purpose:
 *     Get metering portion of Policy Table.
 *
 * Parameters:
 *     unit      - (IN)BCM device number.
 *     f_ent     - (IN)Software entry structure to get tcam info from.
 *     mem       - (IN)Policy table memory.
 *     buf       - (IN/OUT)Hardware policy entry
 *
 * Returns:
 *     BCM_E_NONE  - Success
 *
 */
int
_bcm_field_trx_policer_action_set(int unit, _field_entry_t *f_ent,
                                  soc_mem_t mem, uint32 *buf)
{
    _field_entry_policer_t  *f_ent_pl; /* Field entry policer descriptor.*/
    _field_stage_t          *stage_fc; /* Stage field control structure. */
    bcm_field_stage_t       stage_id;  /* Stage that contains meter.     */
    _field_policer_t        *f_pl;     /* Field policer descriptor.      */
    int                     idx;       /* Policers levels iterator.      */
    uint32                  meter_pair_mode = BCM_FIELD_METER_MODE_DEFAULT;
                                       /* Hw meter usage bits.      */
    soc_field_t             pair_mode_field;/* Hw field for pair mode.   */
    soc_field_t             index_field;    /* Hw field for meter index. */
    soc_field_t             modifier_field; /* Hw field for meter update.*/
    int                     meter_pair_idx; /* Meter pair index.         */
    uint32                  meter_merge = 0;/* Meter merge mode.         */
    int                     rv;             /* Operation return status.  */


    /* Input parameter check. */
    if ((NULL == f_ent) || (NULL == buf))  {
        return (BCM_E_PARAM);
    }
    if (NULL == f_ent->group) {
        return (BCM_E_PARAM);
    }

    stage_id = (f_ent->group->stage_id == _BCM_FIELD_STAGE_EXTERNAL)
                ? _BCM_FIELD_STAGE_INGRESS : f_ent->group->stage_id;

    /* No policers at lookup stage. */
    if (_BCM_FIELD_STAGE_LOOKUP == stage_id) {
        return (BCM_E_NONE);
    }

    /* Get stage control structure. */
    rv = _field_stage_control_get(unit, stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    for (idx = 0; idx < _FP_POLICER_LEVEL_COUNT; idx++) {
        f_ent_pl = f_ent->policer + idx;

#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD_TT(unit) && (idx > 0)) {
            /* Trident device does not support Level1 policers */
            break;
        }
#endif
        if (0 == (f_ent_pl->flags & _FP_POLICER_INSTALLED)) {
            /* Install preserve the color policer. */
            rv = _field_trx_default_policer_set(unit, stage_fc, idx,
                                                mem, buf);
            BCM_IF_ERROR_RETURN(rv);
            continue;
        }

        /* Get policer config. */
        rv = _bcm_field_policer_get(unit, f_ent_pl->pid, &f_pl);
        BCM_IF_ERROR_RETURN(rv);

        if (0 == f_pl->level) {
            /* Get hw encoding for meter mode. */
            rv = _bcm_field_meter_pair_mode_get(unit, f_pl, &meter_pair_mode);
            BCM_IF_ERROR_RETURN(rv);
            /* If level 0 policer is Modified trTcm ->
             * meter sharing mode is dual.
             */
            if ((f_pl->cfg.mode == bcmPolicerModeTrTcmDs) ||
                (f_pl->cfg.mode == bcmPolicerModeCoupledTrTcmDs)) {
                meter_merge = 3;
            }
        }

       if (stage_fc->stage_id == _BCM_FIELD_STAGE_INGRESS) {
           /* Pair index is (Pool number * Pairs in Pool + Pair number) */
            meter_pair_idx
                = (f_pl->pool_index
                    * (stage_fc->meter_pool[f_pl->pool_index]->num_meter_pairs))
                    + (f_pl->hw_index);
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD_TT(unit)) {
                soc_mem_field32_set(unit, mem, buf, METER_PAIR_INDEX_ODDf,
                                    meter_pair_idx);
                soc_mem_field32_set(unit, mem, buf, METER_PAIR_INDEX_EVENf,
                                    meter_pair_idx);
                soc_mem_field32_set(unit, mem, buf, METER_PAIR_MODEf,
                                    meter_pair_mode);

                /*
                 * Flow mode is the only one that cares about the test and
                 * update bits.
                 */
                if (_FP_POLICER_EXCESS_HW_METER(f_pl)) {
                    /* Excess meter - even index. */
                    soc_mem_field32_set(unit, mem, buf, METER_TEST_ODDf, 0);
                    soc_mem_field32_set(unit, mem, buf, METER_TEST_EVENf, 1);
                    soc_mem_field32_set(unit, mem, buf, METER_UPDATE_ODDf, 0);
                    soc_mem_field32_set(unit, mem, buf, METER_UPDATE_EVENf, 1);
                } else if (_FP_POLICER_COMMITTED_HW_METER(f_pl)) {
                    /* Committed meter - odd index. */
                    soc_mem_field32_set(unit, mem, buf, METER_TEST_ODDf, 1);
                    soc_mem_field32_set(unit, mem, buf, METER_TEST_EVENf, 0);
                    soc_mem_field32_set(unit, mem, buf, METER_UPDATE_ODDf, 1);
                    soc_mem_field32_set(unit, mem, buf, METER_UPDATE_EVENf, 0);
                }

                if ((f_pl->cfg.mode == bcmPolicerModePassThrough) ||
                    (f_pl->cfg.mode == bcmPolicerModeSrTcmModified)) {
                    soc_mem_field32_set(unit, mem, buf,
                                        METER_PAIR_MODE_MODIFIERf, 1);
                }
                return (BCM_E_NONE);
            } else
#endif
            {
                /*
                 * Get the meter index in the global meter pool
                 * IMP: This is the meter pair index, not the individual meter's
                 * which will be 2x (+1) (see _field_triumph_meter_install)
                 */
                if (f_pl->level) {
                    pair_mode_field = METER_SHARING_MODEf;
                    index_field = SHARED_METER_PAIR_INDEXf;
                    modifier_field = METER_SHARING_MODE_MODIFIERf;
                    meter_pair_mode = (meter_merge) ? meter_merge
                                        : ((f_pl->cfg.flags
                                            & BCM_POLICER_COLOR_MERGE_OR)
                                            ? 1 : 2);
                } else {
                    pair_mode_field = METER_PAIR_MODEf;
                    index_field = METER_PAIR_INDEXf;
                    modifier_field = METER_PAIR_MODE_MODIFIERf;
                }
                
                soc_mem_field32_set(unit, mem, buf, index_field,
                                    meter_pair_idx);
                
                /* Set the meter mode */
                soc_mem_field32_set(unit, mem, buf, pair_mode_field,
                                    meter_pair_mode);
                
                /*
                 * Flow mode cares about the MODIFIER field
                 *     Excess (Even): 0
                 *     COMMITTED/PASS THROUGH (Odd): 1
                 */
                if ((f_pl->cfg.mode == bcmPolicerModePassThrough)
                    || (f_pl->cfg.mode == bcmPolicerModeSrTcmModified)
                    || (_FP_POLICER_COMMITTED_HW_METER(f_pl))) {
                    soc_mem_field32_set(unit, mem, buf, modifier_field, 1);
                } else if (_FP_POLICER_EXCESS_HW_METER(f_pl)) {
                    soc_mem_field32_set(unit, mem, buf, modifier_field, 0);
                }
            }
        } else if (stage_fc->stage_id == _BCM_FIELD_STAGE_EGRESS) {
            /* Get the even and odd indexes from the entry. The even and odd
             * meter indices are the same.
             */
            soc_mem_field32_set(unit, mem, buf, METER_INDEX_EVENf,
                                f_pl->hw_index);
            soc_mem_field32_set(unit, mem, buf, METER_INDEX_ODDf,
                                f_pl->hw_index);

            /* Get the meter mode */
            
            soc_mem_field32_set(unit, mem, buf, METER_PAIR_MODEf,
                                meter_pair_mode);

            /*
             * Flow mode is the only one that cares about the test and
             * update bits.
             */
            if (_FP_POLICER_EXCESS_HW_METER(f_pl)) {
                /* Excess meter - even index. */
                soc_mem_field32_set(unit, mem, buf, METER_TEST_ODDf, 0);
                soc_mem_field32_set(unit, mem, buf, METER_TEST_EVENf, 1);
                soc_mem_field32_set(unit, mem, buf, METER_UPDATE_ODDf, 0);
                soc_mem_field32_set(unit, mem, buf, METER_UPDATE_EVENf, 1);
            } else if (_FP_POLICER_COMMITTED_HW_METER(f_pl)) {
                /* Committed meter - odd index. */
                soc_mem_field32_set(unit, mem, buf, METER_TEST_ODDf, 1);
                soc_mem_field32_set(unit, mem, buf, METER_TEST_EVENf, 0);
                soc_mem_field32_set(unit, mem, buf, METER_UPDATE_ODDf, 1);
                soc_mem_field32_set(unit, mem, buf, METER_UPDATE_EVENf, 0);
            } else if (f_pl->cfg.mode == bcmPolicerModePassThrough) {
                soc_mem_field32_set(unit, mem, buf, METER_TEST_EVENf, 1);
            }
        } else {
            return BCM_E_PARAM;
        }
    }
    return (BCM_E_NONE);
}


/*
 * Function:
 *     _bcm_field_trx_policer_install
 * Purpose:
 *     Install a meter pair into the hardware tables.
 * Parameters:
 *     unit   - (IN) BCM device number.
 *     f_ent  - (IN) Field entry.
 *     f_pl   - (IN) Field policer descriptor.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx_policer_install(int unit, _field_entry_t *f_ent,
                               _field_policer_t *f_pl)
{
    uint32    bucketsize_peak = 0;      /* Bucket size.             */
    uint32    refresh_rate_peak = 0;    /* Policer refresh rate.    */
    uint32    granularity_peak = 0;     /* Policer granularity.     */
    uint32    bucketsize_commit = 0;    /* Bucket size.             */
    uint32    refresh_rate_commit = 0;  /* Policer refresh rate.    */
    uint32    granularity_commit = 0;   /* Policer granularity.     */
    int       refresh_bitsize;          /* Number of bits for the
                                           refresh rate field.      */
    int       bucket_max_bitsize;       /* Number of bits for the
                                           bucket max field.        */
    int       rv;                       /* Operation return status. */
    soc_mem_t meter_table_x;            /* Meter table name.        */
    soc_mem_t meter_table_y;            /* Meter table name.        */
    uint32    flags;                    /* Policer flags.           */

    /* Input parameters check. */
    if ((NULL == f_ent) || (NULL == f_pl)) {
        return (BCM_E_PARAM);
    }

    if (NULL == f_ent->group || NULL == f_ent->fs) {
        return (BCM_E_INTERNAL);
    }

    if (0 == (f_pl->hw_flags & _FP_POLICER_DIRTY)) {
        return (BCM_E_NONE);
    }

    /* Resolve meter table name. */
    rv = _field_trx_meter_table_get(unit, f_ent->fs->stage_id,
                                    &meter_table_x, &meter_table_y);
    BCM_IF_ERROR_RETURN(rv);

    refresh_bitsize = soc_mem_field_length(unit, meter_table_x, REFRESHCOUNTf);
    bucket_max_bitsize = soc_mem_field_length(unit, meter_table_x, BUCKETSIZEf);

    /* lookup bucket size from tables */
    flags = _BCM_XGS_METER_FLAG_GRANULARITY | _BCM_XGS_METER_FLAG_FP_POLICER;

    /* Set packet mode flags setting */
    if (f_pl->cfg.flags & BCM_POLICER_MODE_PACKETS) {
        flags |= _BCM_XGS_METER_FLAG_PACKET_MODE;
    } else {
        flags &= ~_BCM_XGS_METER_FLAG_PACKET_MODE;
    }
    
    if (f_pl->cfg.mode  != bcmPolicerModeSrTcm) {
        if (f_pl->hw_flags & _FP_POLICER_COMMITTED_DIRTY) {
            /* Calculate policer bucket size/refresh_rate/granularity. */
            rv = _bcm_xgs_kbits_to_bucket_encoding(f_pl->cfg.ckbits_sec,
                                                   f_pl->cfg.ckbits_burst,
                                                   flags, refresh_bitsize,
                                                   bucket_max_bitsize,
                                                   &refresh_rate_commit,
                                                   &bucketsize_commit,
                                                   &granularity_commit);
            /* Programm policer parameters into hw. */
            rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                               BCM_FIELD_METER_COMMITTED,
                                               bucketsize_commit,
                                               refresh_rate_commit,
                                               granularity_commit, meter_table_x);

            if (BCM_SUCCESS(rv) && (INVALIDm != meter_table_y)) {
                rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                                   BCM_FIELD_METER_COMMITTED,
                                                   bucketsize_commit,
                                                   refresh_rate_commit,
                                                   granularity_commit,
                                                   meter_table_y);
            }
            f_pl->hw_flags &= ~_FP_POLICER_COMMITTED_DIRTY;
        }

        if (f_pl->hw_flags & _FP_POLICER_PEAK_DIRTY) {

            if (_FP_POLICER_EXCESS_HW_METER(f_pl)) {
                /*
                 * Rates are always set in committed variables,
                 * for flow meters.
                 */
                f_pl->cfg.pkbits_sec = f_pl->cfg.ckbits_sec;
                f_pl->cfg.pkbits_burst = f_pl->cfg.ckbits_burst;
            }

            /* Calculate policer bucket size/refresh_rate/granularity. */
            rv = _bcm_xgs_kbits_to_bucket_encoding(f_pl->cfg.pkbits_sec,
                                                   f_pl->cfg.pkbits_burst,
                                                   flags,
                                                   refresh_bitsize,
                                                   bucket_max_bitsize,
                                                   &refresh_rate_peak,
                                                   &bucketsize_peak,
                                                   &granularity_peak);
            /* Programm policer parameters into hw. */
            rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                               BCM_FIELD_METER_PEAK,
                                               bucketsize_peak, refresh_rate_peak,
                                               granularity_peak, meter_table_x);

            if (BCM_SUCCESS(rv) && (INVALIDm != meter_table_y)) {
                rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                                   BCM_FIELD_METER_PEAK,
                                                   bucketsize_peak,
                                                   refresh_rate_peak,
                                                   granularity_peak,
                                                   meter_table_y);
            }

            f_pl->hw_flags &= ~_FP_POLICER_PEAK_DIRTY;

            if (_FP_POLICER_EXCESS_HW_METER(f_pl)) {
                /* Reset peak meter rates. */
                f_pl->cfg.pkbits_sec = 0;
                f_pl->cfg.pkbits_burst = 0;
            }
        }
    } else {
        if (f_pl->hw_flags & _FP_POLICER_COMMITTED_DIRTY) {
            /* Calculate policer bucket size/refresh_rate/granularity. */
            rv = _bcm_xgs_kbits_to_bucket_encoding(f_pl->cfg.ckbits_sec,
                                                   f_pl->cfg.ckbits_burst,
                                                   flags, refresh_bitsize,
                                                   bucket_max_bitsize,
                                                   &refresh_rate_commit,
                                                   &bucketsize_commit,
                                                   &granularity_commit);
            BCM_IF_ERROR_RETURN(rv);
        }

        if (f_pl->hw_flags & _FP_POLICER_PEAK_DIRTY) {
            /* Calculate policer bucket size/refresh_rate/granularity. */
            rv = _bcm_xgs_kbits_to_bucket_encoding(f_pl->cfg.pkbits_sec,
                                                   f_pl->cfg.pkbits_burst,
                                                   flags,
                                                   refresh_bitsize,
                                                   bucket_max_bitsize,
                                                   &refresh_rate_peak,
                                                   &bucketsize_peak,
                                                   &granularity_peak);
            BCM_IF_ERROR_RETURN(rv);
        }

        if (granularity_commit != granularity_peak) {
            if (granularity_commit < granularity_peak) {
                rv = _bcm_xgs_kbits_to_dual_bucket_encoding(f_pl->cfg.ckbits_sec,
                                                            f_pl->cfg.ckbits_burst,
                                                            flags,
                                                            refresh_bitsize,
                                                            bucket_max_bitsize,
                                                            granularity_peak,
                                                            &refresh_rate_commit,
                                                            &bucketsize_commit,
                                                            &granularity_commit);
            } else if (granularity_commit > granularity_peak) {
                rv = _bcm_xgs_kbits_to_dual_bucket_encoding(f_pl->cfg.pkbits_sec,
                                                            f_pl->cfg.pkbits_burst,
                                                            flags,
                                                            refresh_bitsize,
                                                            bucket_max_bitsize,
                                                            granularity_commit,
                                                            &refresh_rate_peak,
                                                            &bucketsize_peak,
                                                            &granularity_peak);
            }
            BCM_IF_ERROR_RETURN(rv);
        }
        /* Programm policer parameters into hw. */
        rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                           BCM_FIELD_METER_COMMITTED,
                                           bucketsize_commit,
                                           refresh_rate_commit,
                                           granularity_commit, meter_table_x);

        if (BCM_SUCCESS(rv) && (INVALIDm != meter_table_y)) {
            rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                               BCM_FIELD_METER_COMMITTED,
                                               bucketsize_commit,
                                               refresh_rate_commit,
                                               granularity_commit,
                                               meter_table_y);
        }

        f_pl->hw_flags &= ~_FP_POLICER_COMMITTED_DIRTY;

        /* Programm policer parameters into hw. */
        rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                           BCM_FIELD_METER_PEAK,
                                           bucketsize_peak, refresh_rate_peak,
                                           granularity_peak, meter_table_x);

        if (BCM_SUCCESS(rv) && (INVALIDm != meter_table_y)) {
            rv =  _field_trx_policer_hw_update(unit, f_ent, f_pl,
                                               BCM_FIELD_METER_PEAK,
                                               bucketsize_peak,
                                               refresh_rate_peak,
                                               granularity_peak,
                                               meter_table_y);
        }

        f_pl->hw_flags &= ~_FP_POLICER_PEAK_DIRTY;
    }
    return rv;
}

/*
 * Function:
 *     _bcm_field_trx_stat_index_get
 * Purpose:
 *      Get hw indexes and flags needed to compose requested statistic.
 *
 * Parameters:
 *   unit          - (IN)  BCM device number.
 *   f_st          - (IN)  Field statistics entity.
 *   stat          - (IN)  Counter type.
 *   idx1          - (OUT)  Primary counter index.
 *   idx2          - (OUT)  Secondary counter index.
 *   out_flags     - (OUT)  Counter flags.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_field_trx_stat_index_get(int unit, _field_stat_t *f_st,
                             bcm_field_stat_t stat,
                             int *idx1, int *idx2, uint32 *out_flags)
{
    _field_stage_t *stage_fc;        /* Stage field control structure. */
    int            rv;               /* Operation return status.       */
    uint32         flags = 0;        /* _FP_STAT_XXX flags.            */
    uint32         counter_tbl_idx;  /* Counter table index.           */
    int            index1  = _FP_INVALID_INDEX;
    int            index2  = _FP_INVALID_INDEX;
#if defined(BCM_TRIDENT_SUPPORT)
    int lower_cntr_mode = 0;
    int upper_cntr_mode = 0;
#endif

    /* Input parameters check. */
    if ((NULL == idx1) || (NULL == idx2) ||
        (NULL == out_flags) || (NULL == f_st)) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    *idx1 = *idx2 = _FP_INVALID_INDEX;

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, f_st->stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

#if defined (BCM_TRIUMPH_SUPPORT)
    /* External stage counter index translation. */
    if (_BCM_FIELD_STAGE_EXTERNAL == f_st->stage_id) {
        rv = _bcm_field_tr_external_counter_idx_get(unit, f_st->pool_index,
                                                    f_st->hw_index, idx1);
        switch (stat) {
          case bcmFieldStatBytes:
          case bcmFieldStatGreenBytes:
          case bcmFieldStatYellowBytes:
          case bcmFieldStatRedBytes:
          case bcmFieldStatNotGreenBytes:
          case bcmFieldStatNotYellowBytes:
          case bcmFieldStatNotRedBytes:
              flags |= _FP_STAT_BYTES;
              break;
          default:
              break;
        }
        *out_flags = flags;
        return (rv);
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    /* Stat entity indexes are adjusted for policy table counter pairs. */
    counter_tbl_idx = f_st->hw_index << 1;

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_KATANA_SUPPORT)
    if ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) &&
        (f_st->stage_id == _BCM_FIELD_STAGE_INGRESS)) {

        lower_cntr_mode = (f_st->hw_mode & 0x7);
        upper_cntr_mode = ((f_st->hw_mode & (0x7 << 3)) >> 3);
        switch ((int)stat) {
            case bcmFieldStatGreenBytes:
                flags |= _FP_STAT_BYTES;
            case bcmFieldStatGreenPackets:
                if (upper_cntr_mode & 0x1) {
                    index1 = counter_tbl_idx + 1;
                } else if (lower_cntr_mode & 0x1) {
                    index1 = counter_tbl_idx;
                } else {
                    return BCM_E_INTERNAL;
                }
                break;

            case bcmFieldStatYellowBytes:
                flags |= _FP_STAT_BYTES;
            case bcmFieldStatYellowPackets:
                if (upper_cntr_mode & 0x2) {
                    index1 = counter_tbl_idx + 1;
                } else if (lower_cntr_mode & 0x2) {
                    index1 = counter_tbl_idx;
                } else {
                    return BCM_E_INTERNAL;
                }
                break;

            case bcmFieldStatRedBytes:
                flags |= _FP_STAT_BYTES;
            case bcmFieldStatRedPackets:
                if (upper_cntr_mode & 0x4) {
                    index1 = counter_tbl_idx + 1;
                } else if (lower_cntr_mode & 0x4) {
                    index1 = counter_tbl_idx;
                } else {
                    return BCM_E_INTERNAL;
                }
                break;

          case bcmFieldStatNotGreenBytes:
                flags |= _FP_STAT_BYTES;
          case bcmFieldStatNotGreenPackets:
                if (upper_cntr_mode & 0x6) {
                    index1 = counter_tbl_idx + 1;
                } else if (((upper_cntr_mode & 0x6) == 0) ||
                            (upper_cntr_mode & 0x1)) {
                    switch (lower_cntr_mode) {
                        case 0x2:
                        case 0x4:
                        case 0x6:
                            index1 = counter_tbl_idx;
                            break;
                        default:
                            return BCM_E_INTERNAL;
                    }
                } else if (((upper_cntr_mode & 0x2) && (lower_cntr_mode & 0x4)) ||
                            ((upper_cntr_mode & 0x4) && (lower_cntr_mode & 0x2))) {
                    index1 = counter_tbl_idx;
                    index2 = counter_tbl_idx + 1;
                    flags = _FP_STAT_ADD;
                } else if ((lower_cntr_mode & 0x6) == 0) {
                    switch (upper_cntr_mode) {
                        case 0x2:
                        case 0x4:
                        case 0x6:
                            index1 = counter_tbl_idx + 1;
                            break;
                        default:
                            return BCM_E_INTERNAL;
                    }
                } else {
                    return BCM_E_INTERNAL;
                }
                break;

          case bcmFieldStatNotYellowBytes:
                flags |= _FP_STAT_BYTES;
          case bcmFieldStatNotYellowPackets:
                if (upper_cntr_mode & 0x5) {
                    index1 = counter_tbl_idx + 1;
                } else if (((upper_cntr_mode & 0x5) == 0) ||
                        ((upper_cntr_mode & 0x2))) {
                    switch (lower_cntr_mode) {
                        case 0x1:
                        case 0x4:
                        case 0x5:
                            index1 = counter_tbl_idx;
                            break;
                        default:
                            return BCM_E_INTERNAL;
                    }
                } else if (((upper_cntr_mode & 0x1) && (lower_cntr_mode & 0x4)) ||
                            ((upper_cntr_mode & 0x4) && (lower_cntr_mode & 0x1))) {
                    index1 = counter_tbl_idx;
                    index2 = counter_tbl_idx + 1;
                    flags = _FP_STAT_ADD;
                }  else if ((lower_cntr_mode & 0x5) == 0) {
                    switch (upper_cntr_mode) {
                        case 0x1:
                        case 0x4:
                        case 0x5:
                            index1 = counter_tbl_idx + 1;
                            break;
                        default:
                            return BCM_E_INTERNAL;
                    }
                } else {
                    return BCM_E_INTERNAL;
                }
                break;

          case bcmFieldStatNotRedBytes:
                flags |= _FP_STAT_BYTES;
          case bcmFieldStatNotRedPackets:
                if (upper_cntr_mode & 0x3) {
                    index1 = counter_tbl_idx + 1;
                } else if (((upper_cntr_mode & 0x3) == 0) ||
                        ((upper_cntr_mode & 0x4))) {
                    switch (lower_cntr_mode) {
                        case 0x1:
                        case 0x2:
                        case 0x3:
                            index1 = counter_tbl_idx;
                            break;
                        default:
                            return BCM_E_INTERNAL;
                    }
                } else if (((upper_cntr_mode & 0x1) && (lower_cntr_mode & 0x2)) ||
                            ((upper_cntr_mode & 0x2) && (lower_cntr_mode & 0x1))) {
                    index1 = counter_tbl_idx;
                    index2 = counter_tbl_idx + 1;
                    flags = _FP_STAT_ADD;
                } else if ((lower_cntr_mode & 0x3) == 0) {
                    switch (upper_cntr_mode) {
                        case 0x1:
                        case 0x2:
                        case 0x3:
                            index1 = counter_tbl_idx + 1;
                            break;
                        default:
                            return BCM_E_INTERNAL;
                    }
                } else {
                    return BCM_E_INTERNAL;
                }
                break;

          case bcmFieldStatBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatPackets:
                          if ((lower_cntr_mode ^ upper_cntr_mode) &&
                                          ((lower_cntr_mode != 0) &&
                                           (upper_cntr_mode != 0))) {
                                  index1 = counter_tbl_idx;
                                  index2 = counter_tbl_idx + 1;
                                  flags = _FP_STAT_ADD;
                          } else if (upper_cntr_mode &&
                                          (lower_cntr_mode & 0x7) == 0) {
                                  index1 = counter_tbl_idx + 1;
                          } else if (upper_cntr_mode == 0 && lower_cntr_mode) {
                                  index1 = counter_tbl_idx;
                          } else if ((lower_cntr_mode == 0x7) &&
                                          (upper_cntr_mode == 0x7)) {
                                  index1 = counter_tbl_idx + 1;
                          } else {
                                  return BCM_E_INTERNAL;
                          }
                          break;

          default:
              return (BCM_E_INTERNAL);
        }
        } else
#endif /* BCM_TRIDENT_SUPPORT || BCM_KATANA_SUPPORT*/
    {
        switch ((int)stat) {
          case bcmFieldStatBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatPackets:
              switch (f_st->hw_mode) {
                case (0x2):
                    index1 = counter_tbl_idx + 1;
                    break;
                case (0x1):
                    index1 = counter_tbl_idx;
                    break;
                default:
                    return (BCM_E_INTERNAL);
              }
              break;
          case bcmFieldStatGreenBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatGreenPackets:
              index1 = counter_tbl_idx + 1;
              break;

          case bcmFieldStatYellowBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatYellowPackets:
              index1 = counter_tbl_idx;
              break;

          case bcmFieldStatRedBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatRedPackets:
              switch (f_st->hw_mode) {
                case 0x3:
                case 0x7:
                    index1 = counter_tbl_idx + 1;
                    break;
                case 0x5:
                    index1 = counter_tbl_idx;
                    break;
                default:
                    return (BCM_E_INTERNAL);
              }
              break;

          case bcmFieldStatNotGreenBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatNotGreenPackets:
              switch (f_st->hw_mode) {
                case 0x4:
                    index1 = counter_tbl_idx;
                    break;
                case 0x7:
                    index1 = counter_tbl_idx;
                    index2 = counter_tbl_idx + 1;
                    flags |= _FP_STAT_ADD;
                    break;
                default:
                    return (BCM_E_INTERNAL);
              }
              break;

          case bcmFieldStatNotYellowBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatNotYellowPackets:
              switch (f_st->hw_mode) {
                case 0x5:
                    index1 = counter_tbl_idx;
                    index2 = counter_tbl_idx + 1;
                    flags |= _FP_STAT_ADD;
                    break;
                default:
                    return (BCM_E_INTERNAL);
              }
              break;

          case bcmFieldStatNotRedBytes:
              flags |= _FP_STAT_BYTES;
          case bcmFieldStatNotRedPackets:
              switch (f_st->hw_mode) {
                case (0x3):
                    index1 = counter_tbl_idx;
                    break;
                case (0x6):
                    index1 = counter_tbl_idx;
                    index2 = counter_tbl_idx + 1;
                    flags |= _FP_STAT_ADD;
                    break;
                default:
                    return (BCM_E_INTERNAL);
              }
              break;
          default:
              return (BCM_E_INTERNAL);
        }
    }


    /* Calculate  counter table entry index. */
    if (_FP_INVALID_INDEX != index1) {
        if (stage_fc->flags & _FP_STAGE_GLOBAL_COUNTERS) {
            *idx1 = index1;
        } else {
            *idx1 = stage_fc->slices[f_st->pool_index].start_tcam_idx + index1;
        }
    } else {
        *idx1 = _FP_INVALID_INDEX;
    }

    if (_FP_INVALID_INDEX != index2) {
        if (stage_fc->flags & _FP_STAGE_GLOBAL_COUNTERS) {
            *idx2 = index2;
        } else {
            *idx2 = stage_fc->slices[f_st->pool_index].start_tcam_idx + index2;
        }
    } else {
        *idx2 = _FP_INVALID_INDEX;
    }
    *out_flags = flags;
    return (BCM_E_NONE);
}

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT)
/*
 * Function:
 *     _field_trx2_udf_tcam_entry_move
 * Purpose:
 *     Move a single entry in  udf tcam and offset tables.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     data_ctrl - (IN) Data control structure.
 *     dest      - (IN) Insertion target index.
 *     free_slot - (IN) Free slot.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_entry_move(int unit,
                               _field_data_control_t *data_ctrl,
                               int src, int dst)
{
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.   */
    int rv;                                 /* Operation return status.   */

    /* Input parameters check. */
    if (NULL == data_ctrl) {
        return (BCM_E_PARAM);
    }

    rv = soc_mem_read(unit, FP_UDF_OFFSETm, MEM_BLOCK_ANY, src, hw_buf);
    BCM_IF_ERROR_RETURN(rv);
    rv = soc_mem_write(unit, FP_UDF_OFFSETm, MEM_BLOCK_ALL, dst, hw_buf);
    BCM_IF_ERROR_RETURN(rv);
    rv = soc_mem_read(unit, FP_UDF_TCAMm, MEM_BLOCK_ANY, src, hw_buf);
    BCM_IF_ERROR_RETURN(rv);
    rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, dst, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Clear entries at old index. */
    rv = soc_mem_write(unit, FP_UDF_OFFSETm, MEM_BLOCK_ALL, src,
                       soc_mem_entry_null(unit, FP_UDF_OFFSETm));
    BCM_IF_ERROR_RETURN(rv);

    rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, src,
                       soc_mem_entry_null(unit, FP_UDF_TCAMm));
    BCM_IF_ERROR_RETURN(rv);

    /* Update sw structure tracking entry use. */
    sal_memcpy(data_ctrl->tcam_entry_arr + dst,
               data_ctrl->tcam_entry_arr + src,
               sizeof(_field_data_tcam_entry_t));
    sal_memset(data_ctrl->tcam_entry_arr + src, 0,
               sizeof(_field_data_tcam_entry_t));
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_move_down
 * Purpose:
 *     Moved udf tcam entries down
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     data_ctrl - (IN) Data control structure.
 *     dest      - (IN) Insertion target index.
 *     free_slot - (IN) Free slot.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_move_down(int unit,
                              _field_data_control_t *data_ctrl,
                              int dest, int free_slot)
{
    int idx;          /* Entries iterator.        */
    int rv;           /* Operation return status. */

    /* Input parameters check. */
    if (NULL == data_ctrl) {
        return (BCM_E_PARAM);
    }

    for (idx = free_slot; idx < dest; idx ++) {
        rv = _field_trx2_udf_tcam_entry_move(unit, data_ctrl, idx + 1, idx);
        BCM_IF_ERROR_RETURN(rv);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_move_up
 * Purpose:
 *     Moved udf tcam entries up.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     data_ctrl - (IN) Data control structure.
 *     dest      - (IN) Insertion target index.
 *     free_slot - (IN) Free slot.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_move_up(int unit,
                            _field_data_control_t *data_ctrl,
                            int dest, int free_slot)
{
    int idx;          /* Entries iterator.        */
    int rv;           /* Operation return status. */

    /* Input parameters check. */
    if (NULL == data_ctrl) {
        return (BCM_E_PARAM);
    }

    for (idx = free_slot; idx > dest; idx --) {
        rv = _field_trx2_udf_tcam_entry_move(unit, data_ctrl, idx - 1, idx);
        BCM_IF_ERROR_RETURN(rv);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entries_compare
 * Purpose:
 *     To find out if a duplicate entry already exists.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     hw_entry  - (IN) Hardware entry.
 *     sw_entry  - (IN) Software entry to be matched.
 *     flags_hw  - (IN) Flag bits denote hadware entry details.
 *     flags_sw  - (IN) Flag bits denote software entry details.
 * Returns:
 *     TRUE or FALSE
 */

STATIC int
_field_trx2_udf_tcam_entries_compare(int unit, uint32 *hw_entry,
                                     uint32 *sw_entry, uint32 flags_hw,
                                     uint32 flags_sw)
{
    uint16 hw_etype_mask = 0;
    uint16 hw_etype = 0;
    uint16 sw_etype_mask = 0;
    uint16 sw_etype = 0;

    uint32 hw_ipproto_mask = 0;
    uint32 hw_ipproto= 0;
    uint32 sw_ipproto_mask = 0;
    uint32 sw_ipproto= 0;

    if (0 == (flags_hw  ^ flags_sw)) {
        /* Ip Protocol is the higher 8 bits of the L3_FIELDSf(bits 16:23). 
         * Check if the Hw and Sw has different IP protocol value, then
         * return False, to create new Hardware Index for the incoming values.
         */
        if ((hw_ipproto_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_entry,
                                                   L3_FIELDS_MASKf))) {
            hw_ipproto = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_entry,
                                             L3_FIELDSf);
        }
        if ((sw_ipproto_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, sw_entry,
                                                   L3_FIELDS_MASKf))) {
            sw_ipproto = soc_mem_field32_get(unit, FP_UDF_TCAMm, sw_entry,
                                             L3_FIELDSf);
        }

        hw_ipproto_mask = (hw_ipproto_mask >> 16) & 0xff;
        hw_ipproto = (hw_ipproto >> 16) & 0xff;
        sw_ipproto_mask = (sw_ipproto_mask >> 16) & 0xff;
        sw_ipproto = (sw_ipproto >> 16) & 0xff;

        if (hw_ipproto_mask && sw_ipproto_mask) {
            if ((hw_ipproto_mask & hw_ipproto) != (sw_ipproto_mask & sw_ipproto)) {
                return (FALSE);
            }
        }

        if ((hw_etype_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_entry,
                                              L2_ETHER_TYPE_MASKf))) {
            hw_etype = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_entry,
                                           L2_ETHER_TYPEf);
        }

        if ((sw_etype_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, sw_entry,
                                              L2_ETHER_TYPE_MASKf))) {
            sw_etype = soc_mem_field32_get(unit, FP_UDF_TCAMm, sw_entry,
                                           L2_ETHER_TYPEf);
        }

        /* 
         * Perform slow protocol PDU Ethertype match.
         * Examples:
         *  Case 1:
         *    if (0x800 or 0x86DD or 0x8847 or 0x8906 or 0x8914) != (0x8809) 
         *          return FALSE;
         *  Case 2:
         *    if (0x8809 == 0x8809) then check VLAN Tag match.
         */
        if ((hw_etype_mask && sw_etype_mask)
             && (hw_etype == 0x8809 || sw_etype == 0x8809)) {

            /*
             * For Ethertype == 0x8809.
             * Compare number of VLAN tags in hardware entry vs new entry.
             */
            if ((hw_etype == sw_etype)
                && ((flags_hw & BCM_FIELD_USER_VLAN_MASK) 
                    == (flags_sw & BCM_FIELD_USER_VLAN_MASK))) {

                /* Matching entry already exists. */
                return (TRUE);

            } else {
                /* 
                 * Ethertype vales are not same or Number of VLAN Tags
                 * are different. 
                 */
                return (FALSE);
            }
        }
        if ((hw_etype_mask & hw_etype) != (sw_etype_mask & sw_etype)) {
            /* 
             * Ethertype vales are not same. 
             */
            return (FALSE);
        }
        /* Matching entry already exists. */
        return (TRUE);
    } else {
        /* Matching entry does not exist. */
        return (FALSE);
    }
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_match
 * Purpose:
 *     Match tcam entry against currently instllaed ones
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     stage_fc - (IN) Stage field control structure.
 *     hw_buf   - (IN) Hw buffer.
 *     tcam_idx - (IN) Allocated tcam index.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_entry_match(int unit, _field_stage_t *stage_fc,
                                uint32 *hw_buf, int *tcam_idx)
{
    uint32 *buffer;      /* Hw buffer to dma udf tcam. */
    uint32 *entry_ptr = 0;   /* Tcam entry pointer.        */
    int alloc_size;      /* Memory allocation size.    */
    int entry_size;      /* Single tcam entry size.    */
    soc_mem_t mem;       /* Udf tcam memory id.        */
    int idx_count;       /* Tcam entry count.          */
    int idx;             /* Tcam entries iterator.     */
    int rv;              /* Operation return status.   */
        uint32 flags_hw = 0; /* UDF entry in Hw */
        uint32 flags_sw = 0; /* UDF entry in Sw */

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == stage_fc) || (NULL == tcam_idx)) {
        return (BCM_E_PARAM);
    }

    mem = FP_UDF_TCAMm;
    idx_count = soc_mem_index_count(unit, mem);
    entry_size = sizeof(fp_udf_tcam_entry_t);
    alloc_size = entry_size * idx_count;

    /* Allocate memory buffer. */
    buffer = soc_cm_salloc(unit, alloc_size, "Udf tcam");
    if (buffer == NULL) {
        return (BCM_E_MEMORY);
    }

    sal_memset(buffer, 0, alloc_size);

    /* Read table to the buffer. */
    rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY,
                            soc_mem_index_min(unit, mem),
                            soc_mem_index_max(unit, mem), buffer);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, buffer);
        return (BCM_E_INTERNAL);
    }

    for (idx = 0; idx < idx_count; idx++) {
        if (stage_fc->data_ctrl->tcam_entry_arr[idx].ref_count) {
            entry_ptr = soc_mem_table_idx_to_pointer(unit, mem, uint32 *,
                                                     buffer, idx);
            /* Parse entry key to udf flags. */
            rv = _bcm_field_trx2_udf_tcam_entry_parse(unit, entry_ptr, &flags_hw);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, buffer);
                return rv;
            }

            /* Parse entry key to udf flags. */
            rv = _bcm_field_trx2_udf_tcam_entry_parse(unit, hw_buf, &flags_sw);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, buffer);
                return rv;
            }

            /* 
             * Compare hardware entry with new entry to be inserted. 
             * If an entry already exists for this packet match format,
             * then return the hardware index of existing entry to increment
             * its reference count.
             */
            if (_field_trx2_udf_tcam_entries_compare(unit, entry_ptr, hw_buf,
                                                     flags_hw, flags_sw)) {
                *tcam_idx = idx;
                break;
            }
        }
    }
    soc_cm_sfree(unit, buffer);
    return (idx < idx_count) ? BCM_E_NONE : BCM_E_NOT_FOUND;
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_insert
 * Purpose:
 *     Insert udf tcam entry into the tcam.
 *     No actual write happens in this routine
 *     only tcam reorganization and index allocation.
 *     SW must write FP_UDF_OFFSET table before inserting
 *     valid tcam entry.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     priority - (IN) Inserted rule priority.
 *     tcam_idx - (IN) Allocated tcam index.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_entry_insert(int unit, uint32 *hw_buf,
                                 uint8 priority, int *tcam_idx)
{
    _field_data_tcam_entry_t *tcam_entry_arr; /* Tcam entries array.  */
    _field_stage_t *stage_fc;      /* Stage field contral structure.  */
    int unused_entry_min;          /* Unused entry before the range.  */
    int unused_entry_max;          /* Unused entry after the range.   */
    int range_min;                 /* Index min for entry insertion.  */
    int range_max;                 /* Index max for entry insertion.  */
    int idx_max;                   /* FP UDF tcam table index max.    */
    int idx;                       /* FP UDF tcam table interator.    */
    int rv;                        /* Operation return status.        */

    /* Input parameters check. */
    if (NULL == tcam_idx) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    idx_max = soc_mem_index_max(unit, FP_UDF_TCAMm);
    range_min = soc_mem_index_min(unit, FP_UDF_TCAMm);
    range_max = idx_max;
    unused_entry_min = unused_entry_max = -1;


    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);
    tcam_entry_arr = stage_fc->data_ctrl->tcam_entry_arr;


    /* Check if identical entry already exists. */
    rv = _field_trx2_udf_tcam_entry_match(unit, stage_fc, hw_buf, tcam_idx);
    if (BCM_SUCCESS(rv)) {
        if (tcam_entry_arr[*tcam_idx].priority != priority) {
            return (BCM_E_RESOURCE);
        }
        tcam_entry_arr[*tcam_idx].ref_count++;
        return (BCM_E_NONE);
    } else if (rv != BCM_E_NOT_FOUND) {
        return (rv);
    }

    /* No identical entry found try to allocate an unused entry and
     * reshuffle tcam if necessary to organize entries by priority.
     */
    for (idx = 0; idx <= idx_max; idx++) {
        if (0 == stage_fc->data_ctrl->tcam_entry_arr[idx].ref_count) {
            if (idx <= range_max) {
                /* Any free index below range max can be used for insertion. */
                unused_entry_min = idx;
            } else {
                /* There is no point to continue after first
                 * free index above range max.
                 */
                unused_entry_max = idx;
                break;
            }
            continue;
        }
        /* Identify insertion range. */
        if (tcam_entry_arr[idx].priority > priority) {
            range_min = idx;
        } else if (tcam_entry_arr[idx].priority < priority) {
            if (idx < range_max) {
                range_max = idx;
            }
        }
    }

    /* Check if tcam is completely full. */
    if ((unused_entry_min == -1) && (unused_entry_max == -1)) {
        return (BCM_E_FULL);
    }

    /*  Tcam entries shuffling. */
    if (unused_entry_min > range_min) {
        *tcam_idx = unused_entry_min;
    } else if (unused_entry_min == -1) {
        rv = _field_trx2_udf_tcam_move_up(unit, stage_fc->data_ctrl,
                                         range_max, unused_entry_max);

        BCM_IF_ERROR_RETURN(rv);
        *tcam_idx = range_max;
    } else if (unused_entry_max == -1) {
        rv = _field_trx2_udf_tcam_move_down(unit, stage_fc->data_ctrl,
                                           range_min, unused_entry_min);
        BCM_IF_ERROR_RETURN(rv);
        *tcam_idx = range_min;
    } else if ((range_min - unused_entry_min) >
               (unused_entry_max - range_max)) {
        rv = _field_trx2_udf_tcam_move_up(unit, stage_fc->data_ctrl,
                                         range_max, unused_entry_max);
        BCM_IF_ERROR_RETURN(rv);
        *tcam_idx = range_max;
    } else {
        rv = _field_trx2_udf_tcam_move_down(unit, stage_fc->data_ctrl,
                                           range_min, unused_entry_min);
        BCM_IF_ERROR_RETURN(rv);
        *tcam_idx = range_min;
    }

    /* Index was successfully allocated. */
    tcam_entry_arr[*tcam_idx].ref_count = 1;
    tcam_entry_arr[*tcam_idx].priority = priority;
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_offset_entry_write
 * Purpose:
 *     Write the udf offsets to FP_UDF_OFFSETm
 * Parameters:
 *     unit         - (IN) BCM device number.
 *     entry_idx    - (IN) Entry index in FP_UDF_OFFSETm.
 *     udf_num      - (IN) 0 for UDF1_xxx, 1 for UDF2_xxx.
 *     user_num     - (IN) 0 for UDFx_OFFSET0, 1 for UDFx_OFFSET1, ...
 *     offset_base  - (IN) Offset number base point.
 *     offset_value - (IN) Offset word number.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_offset_entry_write(int unit, int entry_idx,
                                   int udf_num, int user_num,
                                   bcm_field_data_offset_base_t offset_base,
                                   uint32 offset_value)
{
    fp_udf_offset_entry_t offset_buf;    /* Udf offset entry.             */
    int offset_base_encoding;            /* Hw udf offset base encodings. */
    int rv;                              /* Operation return status.      */

    offset_value &= ~_BCM_FIELD_USER_OFFSET_FLAGS;

    if (offset_value >=
        ((uint32)1 << soc_mem_field_length(unit, FP_UDF_OFFSETm,
                                   _trx2_offset_field[udf_num][user_num]))) {
        return BCM_E_PARAM;
    }

    soc_mem_field32_set(unit, FP_UDF_OFFSETm, (uint32 *)&offset_buf,
                        _trx2_offset_field[udf_num][user_num], offset_value);

    rv = soc_mem_read(unit, FP_UDF_OFFSETm, MEM_BLOCK_ANY,
                      entry_idx, (uint32 *)&offset_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set udf offset base. */
    switch (offset_base) {
      case bcmFieldDataOffsetBaseHigigHeader:
      case bcmFieldDataOffsetBaseHigig2Header:
          offset_base_encoding = 0;
          break;
      case bcmFieldDataOffsetBasePacketStart:
          offset_base_encoding = 1;
          /* Offset value 126 (2bytes). Offset value 0 pkt offset 2 */
          break;
      case bcmFieldDataOffsetBaseTrillHeader:
      case bcmFieldDataOffsetBaseOuterL3Header:
      case bcmFieldDataOffsetBaseFcoeHeader:
          offset_base_encoding = 2;
          break;
      case bcmFieldDataOffsetBaseInnerL3Header:
      case bcmFieldDataOffsetBaseOuterL4Header:
          offset_base_encoding = 3;
          break;
      case bcmFieldDataOffsetBaseTrillPayload:
      case bcmFieldDataOffsetBaseInnerL4Header:
          offset_base_encoding = 4;
          break;
      default:
          return (BCM_E_PARAM);
    }

    /* Set udf offset. */
    soc_mem_field32_set(unit, FP_UDF_OFFSETm, (uint32 *)&offset_buf,
                        _trx2_offset_field[udf_num][user_num], offset_value);
    soc_mem_field32_set(unit, FP_UDF_OFFSETm, (uint32 *)&offset_buf,
                        _trx2_base_field[udf_num][user_num],
                        offset_base_encoding);
    /* Write udf offset entry back to HW. */
    rv = soc_mem_write(unit, FP_UDF_OFFSETm, MEM_BLOCK_ALL,
                       entry_idx, (uint32 *)&offset_buf);
    return (rv);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_l3_parse
 * Purpose:
 *     Parse udf tcam entry l3 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     pkt_fmt  - (OUT) Packet format structure.
 * Returns:
 *     BCM_E_XXX
 */
int
_field_trx2_udf_tcam_entry_l3_parse(int unit, uint32 *hw_buf,
                                   bcm_field_data_packet_format_t *pkt_fmt)
{
    uint32 ethertype = -1;
    uint32 inner_iptype = -1;
    uint32 ethertype_mask = -1;
    uint32 outer_iptype = -1;
    uint32 outer_iptype_ip4_val = -1;
    uint32 outer_iptype_ip6_val = -1;
    uint32 outer_iptype_mask = -1;
    uint32 inner_iptype_mask = -1;
    uint32 l3fields = -1;
    soc_mem_t mem = FP_UDF_TCAMm;
    uint32 iptype_none_val = 0;
    uint8  fc_header_encode = 0;
    uint8  fc_hdr_encode_mask = 0;

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == pkt_fmt)) {
        return (BCM_E_PARAM);
    }
    if (SOC_IS_TD_TT(unit)
        || SOC_IS_KATANAX(unit)
        || SOC_IS_TRIUMPH3(unit)) {
        iptype_none_val = 2;
    }

    if ((ethertype_mask =
            soc_mem_field32_get(unit, mem, hw_buf, L2_ETHER_TYPE_MASKf))) {
        ethertype = soc_mem_field32_get(unit, mem, hw_buf, L2_ETHER_TYPEf);
    }

    if ((inner_iptype_mask =
            soc_mem_field32_get(unit, mem, hw_buf, INNER_IP_TYPE_MASKf))) {
        inner_iptype = soc_mem_field32_get(unit, mem, hw_buf, INNER_IP_TYPEf);
    }

    outer_iptype = soc_mem_field32_get(unit, mem, hw_buf, OUTER_IP_TYPEf);
    outer_iptype_mask =  soc_mem_field32_get(unit, mem,
                                          hw_buf, OUTER_IP_TYPE_MASKf);

    if (soc_mem_field32_get(unit, mem, hw_buf, L3_FIELDS_MASKf)) {
        l3fields = soc_mem_field32_get(unit, mem, hw_buf, L3_FIELDSf);
    } else if (0x8847 == ethertype) {
        /* MPLS_ANY has L3_FIELDSf == 0 */
        l3fields = soc_mem_field32_get(unit, mem, hw_buf, L3_FIELDSf);
    }

    outer_iptype_ip4_val = 2;
    outer_iptype_ip6_val = 4;
#if defined(BCM_TRIDENT_SUPPORT)
   if (SOC_IS_TRIUMPH3(unit)
       || SOC_IS_TD_TT(unit)) {
       outer_iptype_ip4_val = 1;
       outer_iptype_ip6_val = 5;     
   }
#endif      

    /* 
     * Logic below implicitly checks Ethertype value for IP, MPLS and FCoE
     * frames.
     * 0x0800 - IPv4, 0x86DD - IPv6, 0x8847 - MPLS , 0x8906 + 0x8914 - FCoE
     */

    if ((ethertype == 0x800) && (outer_iptype == outer_iptype_ip4_val)) {
 	pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_NONE;
 	pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP4_WITH_OPTIONS;
 	pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP_NONE;
    } else if ((ethertype == 0x86dd) && 
               (outer_iptype == outer_iptype_ip6_val)) {
 	pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_NONE;
 	pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP6_WITH_OPTIONS;
 	pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP_NONE;
    } else if ((ethertype == 0x800) && (inner_iptype ==  iptype_none_val)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP_NONE;
    } else if ((ethertype == 0x86dd) && (inner_iptype ==  iptype_none_val)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP_NONE;
    } else if ((ethertype == 0x800) && (l3fields ==  0x40000)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP4;
    } else if ((ethertype == 0x800) && (l3fields ==  0x290000)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP6;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x40000)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP4;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x290000)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP6;
    } else if ((ethertype == 0x800) && (l3fields ==  0x2f0800)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP4;
    } else if ((ethertype == 0x800) && (l3fields ==  0x2f86dd)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP6;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x2f0800)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP4;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x2f86dd)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP6;
    } else if ((ethertype == 0x8847) && (l3fields ==  0x1)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_MPLS;
        pkt_fmt->mpls = BCM_FIELD_DATA_FORMAT_MPLS_ONE_LABEL;
    } else if ((ethertype == 0x8847) && (l3fields ==  0x2)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_MPLS;
        pkt_fmt->mpls = BCM_FIELD_DATA_FORMAT_MPLS_TWO_LABELS;
    } else if ((ethertype == 0x8847) && (l3fields ==  0x0)) {
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_MPLS;
        pkt_fmt->mpls = BCM_FIELD_DATA_FORMAT_MPLS_ANY;
    } else if (ethertype == 0x8906 || ethertype == 0x8914) {
        pkt_fmt->tunnel = (ethertype == 0x8906)
            ? BCM_FIELD_DATA_FORMAT_TUNNEL_FCOE
            : BCM_FIELD_DATA_FORMAT_TUNNEL_FCOE_INIT;


        fc_header_encode = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_1f);
        fc_hdr_encode_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_1_MASKf);

        if (0 != fc_header_encode && 0 != fc_hdr_encode_mask) {

            switch (fc_header_encode & fc_hdr_encode_mask) {
                case 1:
                    pkt_fmt->fibre_chan_outer 
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN;
                    break;
                case 2:
                    pkt_fmt->fibre_chan_outer 
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_VIRTUAL;
                    break;
                case 3:
                    pkt_fmt->fibre_chan_outer 
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ENCAP;
                    break;
                case 4:
                    pkt_fmt->fibre_chan_outer 
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ROUTED;
                    break;
                default:
                    return (BCM_E_INTERNAL);
            }
        }

        fc_header_encode = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_2f);
        fc_hdr_encode_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_2_MASKf);

        if (0 != fc_header_encode && 0 != fc_hdr_encode_mask) {

            switch (fc_header_encode & fc_hdr_encode_mask) {
                case 1:
                    pkt_fmt->fibre_chan_inner
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN;
                    break;
                case 2:
                    pkt_fmt->fibre_chan_inner
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_VIRTUAL;
                    break;
                case 3:
                    pkt_fmt->fibre_chan_inner
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ENCAP;
                    break;
                case 4:
                    pkt_fmt->fibre_chan_inner
                        = BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ROUTED;
                    break;
                default:
                    return (BCM_E_INTERNAL);
            }
        }
    } else if (outer_iptype_mask == 0x7) {
        /* 
         * For Non-IP traffic: OUTER_IP_TYPE_MASK = 0x7 and
         * OUTER_IP_TYPE = 0 (TR2) or 2 (Trident, Katana).
         */
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP_NONE;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP_NONE;
    } else if (0x0 == ethertype_mask && 0x0 == inner_iptype_mask
                && 0x0 == outer_iptype_mask) {
        /*
         * Do not care about IP headers and Tunnel Type.
         */
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_ANY;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP_ANY;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP_ANY;
    } else if (outer_iptype_mask == 0x0) {
        /*
         * Do not care about Outer IP. Inner IP is not valid
         * as its not a Tunnel.
         */
        pkt_fmt->tunnel = BCM_FIELD_DATA_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_FIELD_DATA_FORMAT_IP_ANY;
        pkt_fmt->inner_ip = BCM_FIELD_DATA_FORMAT_IP_NONE;
    } else {
        return (BCM_E_INTERNAL);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_l3_init
 * Purpose:
 *     Initialize udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     pkt_fmt  - (IN) Packet format structure.
 *     hw_buf   - (IN/OUT) Hw buffer.
 *     priority - (IN/OUT) udf tcam entry priority.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_entry_l3_init(int unit,
                                  bcm_field_data_packet_format_t *pkt_fmt,
                                  uint32 *hw_buf, uint8 *priority)
{
    int key;
    uint32 iptype_none_val = 0;
    uint32 outer_iptype_ip4_val = 0;
    uint32 outer_iptype_ip6_val = 0;

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == priority)) {
        return (BCM_E_PARAM);
    }

    if (SOC_IS_TD_TT(unit)
        || SOC_IS_KATANAX(unit)
        || SOC_IS_TRIUMPH3(unit)) {
        iptype_none_val = 2;
    }

    outer_iptype_ip4_val = 2;
    outer_iptype_ip6_val = 4;
#if defined(BCM_TRIDENT_SUPPORT)
   if (SOC_IS_TRIUMPH3(unit)
       || SOC_IS_TD_TT(unit)) {
       outer_iptype_ip4_val = 1;
       outer_iptype_ip6_val = 5;
   }
#endif

    if (pkt_fmt->tunnel == BCM_FIELD_DATA_FORMAT_TUNNEL_NONE) {

        *priority += _FP_DATA_QUALIFIER_PRIO_L3_FORMAT;
        /* inner ip type (none). */
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            INNER_IP_TYPEf, iptype_none_val);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            INNER_IP_TYPE_MASKf, 0x7);

        if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP4_WITH_OPTIONS) {
 	    /* L2 ether type 0x800 */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        L2_ETHER_TYPEf, 0x800);
 	    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        L2_ETHER_TYPE_MASKf, 0xffff);
 	    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        OUTER_IP_TYPEf, outer_iptype_ip4_val);
 	    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        OUTER_IP_TYPE_MASKf, 0x7);
 	} else if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP6_WITH_OPTIONS) {
 	    /* L2 ether type 0x86dd */
 	    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        L2_ETHER_TYPEf, 0x86dd);
 	    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        L2_ETHER_TYPE_MASKf, 0xffff);
 	    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        OUTER_IP_TYPEf, outer_iptype_ip6_val);
 	    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
 	                        OUTER_IP_TYPE_MASKf, 0x7);
 	} else if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP4) {
            /* L2 ether type 0x800 */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPEf, 0x800);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPE_MASKf, 0xffff);
        } else if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP6) {
            /* L2 ether type 0x86dd */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPEf, 0x86dd);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPE_MASKf, 0xffff);
        } else if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP_NONE) {
            /* Non IP traffic only */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                OUTER_IP_TYPEf, iptype_none_val);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                OUTER_IP_TYPE_MASKf, 0x7);
        } else {
            /* Outer ip type don't care */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                OUTER_IP_TYPE_MASKf, 0x0);
        }
    } else if (pkt_fmt->tunnel == BCM_FIELD_DATA_FORMAT_TUNNEL_IP_IN_IP) {
        *priority = _FP_DATA_QUALIFIER_PRIO_L3_FORMAT;
        if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP4) {
            /* L2 ether type 0x800 */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPEf, 0x800);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPE_MASKf, 0xffff);

            if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP4) {
                /* Inner ip protocol v4. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x40000);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xff0000);
            } else if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP6) {
                /* Inner ip protocol v6. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x290000);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xff0000);
            } else {
                return (BCM_E_UNAVAIL);
            }
        } else if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP6) {
            /* L2 ether type 0x86dd */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPEf, 0x86dd);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPE_MASKf, 0xffff);

            if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP4) {
                /* Inner ip protocol v4. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x40000);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xff0000);
            } else if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP6) {
                /* Inner ip protocol v6. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x290000);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xff0000);
            } else {
                return (BCM_E_UNAVAIL);
            }
        } else {
            return (BCM_E_UNAVAIL);
        }
    } else if (pkt_fmt->tunnel == BCM_FIELD_DATA_FORMAT_TUNNEL_GRE) {

        *priority = _FP_DATA_QUALIFIER_PRIO_L3_FORMAT;
        if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP4) {
            /* L2 ether type 0x800 */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPEf, 0x800);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPE_MASKf, 0xffff);

            if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP4) {
                /* Inner ip protocol gre, gre ethertype 0x800. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x02f0800);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xffffff);
            } else if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP6) {
                /* Inner ip protocol gre, gre ethertype 0x86dd. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x02f86dd);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xffffff);
            } else {
                return (BCM_E_UNAVAIL);
            }
        } else if (pkt_fmt->outer_ip == BCM_FIELD_DATA_FORMAT_IP6) {
            /* L2 ether type 0x86dd */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPEf, 0x86dd);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_ETHER_TYPE_MASKf, 0xffff);

            if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP4) {
                /* Inner ip protocol gre, gre ethertype 0x800. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x02f0800);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xffffff);
            } else if (pkt_fmt->inner_ip == BCM_FIELD_DATA_FORMAT_IP6) {
                /* Inner ip protocol gre, gre ethertype 0x86dd. */
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDSf, 0x02f86dd);
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                    L3_FIELDS_MASKf, 0xffffff);
            } else {
                return (BCM_E_UNAVAIL);
            }
        } else {
            return (BCM_E_UNAVAIL);
        }
    } else if (pkt_fmt->tunnel == BCM_FIELD_DATA_FORMAT_TUNNEL_MPLS) {

                *priority += _FP_DATA_QUALIFIER_PRIO_MPLS_FORMAT;

        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            L2_ETHER_TYPEf, 0x8847);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            L2_ETHER_TYPE_MASKf, 0xffff);
        if (pkt_fmt->mpls == BCM_FIELD_DATA_FORMAT_MPLS_ONE_LABEL) {

            *priority += _FP_DATA_QUALIFIER_PRIO_MPLS_ONE_LABEL;

            /* L2 ether type 0x8847, outer label1 bos == 1 */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L3_FIELDSf, 0x1);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L3_FIELDS_MASKf, 0xffffff);
        } else if (pkt_fmt->mpls == BCM_FIELD_DATA_FORMAT_MPLS_TWO_LABELS) {

            *priority += _FP_DATA_QUALIFIER_PRIO_MPLS_TWO_LABEL;

            /* L2 ether type 0x8847, outer label1 bos == 0 */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L3_FIELDSf, 0x2);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L3_FIELDS_MASKf, 0xffffff);
        } else if (pkt_fmt->mpls == BCM_FIELD_DATA_FORMAT_MPLS_ANY) {
            /* L2 ether type 0x8847 */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L3_FIELDSf, 0x0);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L3_FIELDS_MASKf, 0x0);
        } else {
            return (BCM_E_UNAVAIL);
        }
    } else if (pkt_fmt->tunnel == BCM_FIELD_DATA_FORMAT_TUNNEL_FCOE ||
               pkt_fmt->tunnel == BCM_FIELD_DATA_FORMAT_TUNNEL_FCOE_INIT) {
        if (!SOC_MEM_FIELD_VALID(unit, FP_UDF_TCAMm, FC_HDR_ENCODE_1f)) {
            return BCM_E_UNAVAIL;
        }

        *priority += _FP_DATA_QUALIFIER_PRIO_FCOE_FORMAT;

        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, L2_ETHER_TYPEf,
                            pkt_fmt->tunnel ==
                            BCM_FIELD_DATA_FORMAT_TUNNEL_FCOE ?
                            0x8906 : 0x8914);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, L2_ETHER_TYPE_MASKf,
                            0xffff);
        switch (pkt_fmt->fibre_chan_outer) {
            case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN:
                key = 1;
                break;
            case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ENCAP:
                key = 3;
                break;
            case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_VIRTUAL:
                key = 2;
                break;
            case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ROUTED:
                key = 4;
                break;
            default:
                return BCM_E_UNAVAIL;
        }
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, FC_HDR_ENCODE_1f,
            key);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, FC_HDR_ENCODE_1_MASKf,
            0x7);

        if (0 != pkt_fmt->fibre_chan_inner) {

            /* Entry matching on two extended headers has higher priority */
            *priority += _FP_DATA_QUALIFIER_PRIO_FCOE_FORMAT;

            switch (pkt_fmt->fibre_chan_inner) {
                case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN:
                    key = 1;
                    break;
                case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ENCAP:
                    key = 3;
                    break;
                case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_VIRTUAL:
                    key = 2;
                    break;
                case BCM_FIELD_DATA_FORMAT_FIBRE_CHAN_ROUTED:
                    key = 4;
                    break;
                default:
                    return BCM_E_UNAVAIL;
            }
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                FC_HDR_ENCODE_2f, key);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                FC_HDR_ENCODE_2_MASKf, 0x7);
        }
    } else if (BCM_FIELD_DATA_FORMAT_TUNNEL_ANY == pkt_fmt->tunnel
                && BCM_FIELD_DATA_FORMAT_IP_ANY == pkt_fmt->outer_ip
                && BCM_FIELD_DATA_FORMAT_IP_ANY == pkt_fmt->inner_ip) {
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, L2_ETHER_TYPEf, 0x0);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, L2_ETHER_TYPE_MASKf,
            0x0);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, L3_FIELDSf, 0x0);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, L3_FIELDS_MASKf, 0x0);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, OUTER_IP_TYPEf, 0x0);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, OUTER_IP_TYPE_MASKf,
            0x0);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, INNER_IP_TYPEf, 0x0);
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, INNER_IP_TYPE_MASKf,
            0x0);
        if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_1f)) {
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, FC_HDR_ENCODE_1f, 0);
        }
        if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_1_MASKf)) {
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, FC_HDR_ENCODE_1_MASKf, 0x0);
        }
        if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_2f)) {
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, FC_HDR_ENCODE_2f, 0);
        }
        if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_2_MASKf)) {
                soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, FC_HDR_ENCODE_2_MASKf, 0x0);
        }
    } else {
        return (BCM_E_UNAVAIL);
    }

    *priority += _FP_DATA_QUALIFIER_PRIO_L3_FORMAT;
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_vlanformat_parse
 * Purpose:
 *     Parse udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     vlanformat - (OUT) BCM_FIELD_DATA_FORMAT_L2_XXX
 * Returns:
 *     BCM_E_XXX
 */
int
_field_trx2_udf_tcam_entry_vlanformat_parse(int unit, uint32 *hw_buf,
                                           uint16 *vlanformat)
{
    uint32 tag_status;

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == vlanformat)) {
        return (BCM_E_PARAM);
    }

    if (soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, L2_TAG_STATUS_MASKf)) {
        tag_status = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                         L2_TAG_STATUSf);
        switch (tag_status) {
          case 0:
              *vlanformat = BCM_FIELD_DATA_FORMAT_VLAN_NO_TAG;
              break;
          case 1:
              *vlanformat = BCM_FIELD_DATA_FORMAT_VLAN_SINGLE_TAGGED;
              break;
          case 2:
              *vlanformat = BCM_FIELD_DATA_FORMAT_VLAN_DOUBLE_TAGGED;
              break;
          default:
              break;
        }
    } else {
        *vlanformat = BCM_FIELD_DATA_FORMAT_VLAN_TAG_ANY;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_vlanformat_init
 * Purpose:
 *     Initialize udf tcam entry vlan tag format match key.
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     vlanformat - (IN) BCM_FIELD_DATA_FORMAT_L2_XXX
 *     hw_buf     - (IN/OUT) Hw buffer.
 *     priority   - (OUT) tcam entry priority.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_entry_vlanformat_init(int unit, uint16 vlanformat,
                                          uint32 *hw_buf, uint8 *priority)
{
    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == priority)) {
        return (BCM_E_PARAM);
    }

    /* Translate L2 flag bits to index */
    switch (vlanformat) {
      case BCM_FIELD_DATA_FORMAT_VLAN_NO_TAG:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUSf, 0);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUS_MASKf, 0x3);
          *priority += _FP_DATA_QUALIFIER_PRIO_VLAN_FORMAT;
          break;
      case BCM_FIELD_DATA_FORMAT_VLAN_SINGLE_TAGGED:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUSf, 1);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUS_MASKf, 0x3);
          *priority += _FP_DATA_QUALIFIER_PRIO_VLAN_FORMAT;
          break;
      case BCM_FIELD_DATA_FORMAT_VLAN_DOUBLE_TAGGED:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUSf, 2);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUS_MASKf, 0x3);
          *priority += _FP_DATA_QUALIFIER_PRIO_VLAN_FORMAT;
          break;
      case BCM_FIELD_DATA_FORMAT_VLAN_TAG_ANY:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUSf, 0);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TAG_STATUS_MASKf, 0);
          break;
      default:
          return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_l2format_parse
 * Purpose:
 *     Parse udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     l2format - (OUT) BCM_FIELD_DATA_FORMAT_L2_XXX
 * Returns:
 *     BCM_E_XXX
 */
int
_field_trx2_udf_tcam_entry_l2format_parse(int unit, uint32 *hw_buf,
                                         uint16 *l2format)
{
    uint32 l2type;

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == l2format)) {
        return (BCM_E_PARAM);
    }

    if (soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, L2_TYPE_MASKf)) {
        l2type = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, L2_TYPEf);
        switch (l2type) {
          case 0:
              *l2format = BCM_FIELD_DATA_FORMAT_L2_ETH_II;
              break;
          case 1:
              *l2format = BCM_FIELD_DATA_FORMAT_L2_SNAP;
              break;
          case 2:
              *l2format = BCM_FIELD_DATA_FORMAT_L2_LLC;
              break;
          default:
              break;
        }
    } else {
        *l2format = BCM_FIELD_DATA_FORMAT_L2_ANY;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_udf_tcam_entry_l2format_init
 * Purpose:
 *     Initialize udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     l2format - (IN) BCM_FIELD_DATA_FORMAT_L2_XXX
 *     hw_buf   - (OUT) Hw buffer.
 *     priority - (OUT) Tcam entry priority
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_udf_tcam_entry_l2format_init(int unit, uint16 l2format,
                                        uint32 *hw_buf, uint8 *priority)
{
    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == priority)) {
        return (BCM_E_PARAM);
    }

    /* Translate L2 flag bits to index */
    switch (l2format) {
      case BCM_FIELD_DATA_FORMAT_L2_ETH_II:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 0);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0x3);
          *priority += _FP_DATA_QUALIFIER_PRIO_L2_FORMAT;
          break;
      case BCM_FIELD_DATA_FORMAT_L2_SNAP:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 1);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0x3);
          *priority += _FP_DATA_QUALIFIER_PRIO_L2_FORMAT;
          break;
      case BCM_FIELD_DATA_FORMAT_L2_LLC:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 2);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0x3);
          *priority += _FP_DATA_QUALIFIER_PRIO_L2_FORMAT;
          break;
      case BCM_FIELD_DATA_FORMAT_L2_ANY:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 0);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0);
          break;
      default:
          return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_trx2_udf_tcam_entry_parse
 * Purpose:
 *     Parse udf tcam entry key
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     flags    - (OUT) Udf spec index (encoded flags).
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_trx2_udf_tcam_entry_parse(int unit, uint32 *hw_buf, uint32 *flags)
{
    bcm_field_data_packet_format_t pkt_fmt;
    int rv;

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == flags)) {
        return (BCM_E_PARAM);
    }
    bcm_field_data_packet_format_t_init(&pkt_fmt);

    /* Get valid bit. */
    if (soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, VALIDf)) {
        *flags = _BCM_FIELD_USER_OFFSET_VALID;
    } else {
        *flags = 0;
        return (BCM_E_NONE);
    }

    /* Parse vlan_tag format.*/
    rv = _field_trx2_udf_tcam_entry_vlanformat_parse(unit, hw_buf,
                                                    &pkt_fmt.vlan_tag);
    BCM_IF_ERROR_RETURN(rv);

    /* Parse l2 format.*/
    rv = _field_trx2_udf_tcam_entry_l2format_parse(unit, hw_buf, &pkt_fmt.l2);
    BCM_IF_ERROR_RETURN(rv);

    /* Parse l3 fields.*/
    rv = _field_trx2_udf_tcam_entry_l3_parse(unit, hw_buf, &pkt_fmt);
    BCM_IF_ERROR_RETURN(rv);

    /* 
     * Translate Data qualifier representation to UDF flag bits value.
     */

    /* IPv4, IPv6, MPLS and FCoE packet formats are taken care below. */
    if (BCM_FIELD_DATA_FORMAT_TUNNEL_NONE == pkt_fmt.tunnel) {
        if (BCM_FIELD_DATA_FORMAT_IP4_WITH_OPTIONS == pkt_fmt.outer_ip) {
            *flags |=  (BCM_FIELD_USER_IP4_HDR_ONLY |
                        BCM_FIELD_USER_OPTION_ADJUST);
        } else if (BCM_FIELD_DATA_FORMAT_IP6_WITH_OPTIONS == pkt_fmt.outer_ip) {
            *flags |=  (BCM_FIELD_USER_IP6_HDR_ONLY |
                        BCM_FIELD_USER_OPTION_ADJUST);
        } else if (BCM_FIELD_DATA_FORMAT_IP4 ==  pkt_fmt.outer_ip) {
            *flags |= BCM_FIELD_USER_IP4_HDR_ONLY;
        } else  if (BCM_FIELD_DATA_FORMAT_IP6 ==  pkt_fmt.outer_ip) {
            *flags |= BCM_FIELD_USER_IP6_HDR_ONLY;
        } else {
            *flags |= BCM_FIELD_USER_IP_NOTUSED;
        }
    } else if (BCM_FIELD_DATA_FORMAT_TUNNEL_IP_IN_IP == pkt_fmt.tunnel) {
        if (BCM_FIELD_DATA_FORMAT_IP4 ==  pkt_fmt.outer_ip) {
            if (BCM_FIELD_DATA_FORMAT_IP4 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_IP4_OVER_IP4;
            } else if (BCM_FIELD_DATA_FORMAT_IP6 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_IP6_OVER_IP4;
            }
        } else {
            if (BCM_FIELD_DATA_FORMAT_IP4 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_IP4_OVER_IP6;
            } else if (BCM_FIELD_DATA_FORMAT_IP6 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_IP6_OVER_IP6;
            }
        }
    } else if (BCM_FIELD_DATA_FORMAT_TUNNEL_GRE == pkt_fmt.tunnel) {
        if (BCM_FIELD_DATA_FORMAT_IP4 ==  pkt_fmt.outer_ip) {
            if (BCM_FIELD_DATA_FORMAT_IP4 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_GRE_IP4_OVER_IP4;
            } else if (BCM_FIELD_DATA_FORMAT_IP6 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_GRE_IP6_OVER_IP4;
            }
        } else {
            if (BCM_FIELD_DATA_FORMAT_IP4 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_GRE_IP4_OVER_IP6;
            } else if (BCM_FIELD_DATA_FORMAT_IP6 ==  pkt_fmt.inner_ip) {
                *flags |= BCM_FIELD_USER_GRE_IP6_OVER_IP6;
            }
        }
    } else if (BCM_FIELD_DATA_FORMAT_TUNNEL_MPLS == pkt_fmt.tunnel) {
        if (BCM_FIELD_DATA_FORMAT_MPLS_ONE_LABEL ==  pkt_fmt.mpls) {
            *flags |= BCM_FIELD_USER_ONE_MPLS_LABEL;
        } else if (BCM_FIELD_DATA_FORMAT_MPLS_TWO_LABELS == pkt_fmt.mpls) {
            *flags |= BCM_FIELD_USER_TWO_MPLS_LABELS;
        } else if (BCM_FIELD_DATA_FORMAT_MPLS_ANY == pkt_fmt.mpls) {
            *flags |= BCM_FIELD_USER_ANY_MPLS_LABELS;
        }
    } else if ((BCM_FIELD_DATA_FORMAT_TUNNEL_FCOE == pkt_fmt.tunnel)
                || (BCM_FIELD_DATA_FORMAT_TUNNEL_FCOE_INIT == pkt_fmt.tunnel)) {
        if (0 != pkt_fmt.fibre_chan_inner && 0 !=  pkt_fmt.fibre_chan_outer) {
            *flags |= BCM_FIELD_USER_TWO_FCOE_HEADER;
        } else if (0 !=  pkt_fmt.fibre_chan_outer
                    && 0 == pkt_fmt.fibre_chan_inner) {
            *flags |= BCM_FIELD_USER_ONE_FCOE_HEADER;
        }
    }


    /* Get VLAN tag format flags. */
    switch (pkt_fmt.vlan_tag) {
      case BCM_FIELD_DATA_FORMAT_VLAN_NO_TAG:
          *flags |= BCM_FIELD_USER_VLAN_NOTAG;
          break;
      case BCM_FIELD_DATA_FORMAT_VLAN_SINGLE_TAGGED:
          *flags |= BCM_FIELD_USER_VLAN_ONETAG;
          break;
      case BCM_FIELD_DATA_FORMAT_VLAN_DOUBLE_TAGGED:
          *flags |= BCM_FIELD_USER_VLAN_TWOTAG;
          break;
      default:
          /* For VLAN Tag Any. */
          *flags |= BCM_FIELD_USER_VLAN_NOTUSED;
          break;
    }

    /* Get L2 format flags. */
    switch (pkt_fmt.l2) {
      case BCM_FIELD_DATA_FORMAT_L2_ETH_II:
          *flags |= BCM_FIELD_USER_L2_ETHERNET2;
          break;
      case BCM_FIELD_DATA_FORMAT_L2_SNAP:
          *flags |= BCM_FIELD_USER_L2_SNAP;
          break;
      case BCM_FIELD_DATA_FORMAT_L2_LLC:
          *flags |= BCM_FIELD_USER_L2_LLC;
          break;
      default:
          /* For L2 Tag Any. */
          *flags |= BCM_FIELD_USER_L2_OTHER;
          break;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx2_data_offset_install
 * Purpose:
 *     Write the info in udf_tcam/udf_offset to the hardware
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     tbl_idx  - (IN) FP_UDF_OFFSET table index.
 *     f_dq     - (IN) Data qualifier structure.
 *     offset   - (IN) Word offset value FP_UDF_OFFSETm.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_data_tcam_offset_install(int unit,
                                    _field_data_qualifier_t *f_dq,
                                    int tbl_idx, int offset)
{
    _field_stage_t *stage_fc;
    uint32  word_offset; /* Offset iterator.              */
    int     udf_idx, user_idx;
    int     rv;          /* Operation return status.      */

    /* Input parameters check. */
    if (NULL == f_dq) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN
        (_field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc));

    word_offset = (offset < 0) ? 0 : (uint32)offset;

    /* Find a proper word to insert the data. */
    for (udf_idx = 0; udf_idx < 2; udf_idx++) {
        for (user_idx = 0; user_idx < stage_fc->data_ctrl->num_elems;
             user_idx++) {
            if (!(f_dq->hw_bmap &
                  (1 << (udf_idx *
                         stage_fc->data_ctrl->num_elems + user_idx)))) {
                continue;
            }

            rv = _field_trx2_udf_offset_entry_write(unit, tbl_idx, udf_idx,
                                                    user_idx,
                                                    f_dq->offset_base,
                                                    word_offset);
            BCM_IF_ERROR_RETURN(rv);

            if (offset >= 0) {
                word_offset++;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _field_trx2_data_elem_offset_calc
 * Purpose:
 *     Calculate element offset based on data qualifier
 *     relative offset, base offset and common offset.
 * Parameters:
 *     unit             - (IN) BCM device number.
 *     f_dq             - (IN) Data qualifier structure.
 *     reltive_offset   - (IN) Word offset value FP_UDF_OFFSETm.
 *     elem_offset       - (OUT) Calculated index value.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx2_data_elem_offset_calc(int unit, _field_data_qualifier_t *f_dq,
                                  int relative_offset, int *elem_offset)
{
    _field_stage_t *stage_fc;
    int offset;

    /* Input parameters check. */
    if ((NULL == f_dq) || (NULL == elem_offset)) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN
        (_field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc));

    offset = f_dq->offset + relative_offset;

    if (stage_fc->data_ctrl->elem_size <= 2) { /* no wrap around needed */
        offset = (offset % 128) / stage_fc->data_ctrl->elem_size;
    } else {
        switch (f_dq->offset_base) {
        case bcmFieldDataOffsetBaseHigigHeader:
        case bcmFieldDataOffsetBaseHigig2Header:
            offset = ((offset + 2) % 128) / 4;
            break;
        case bcmFieldDataOffsetBasePacketStart:
            offset = ((offset + 2) % 128) / 4;
            break;
        default:
            offset = (offset % 128) / 4;
            break;
        }
    }

    *elem_offset = offset;
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _field_trx2_data_qualifier_etype_tcam_key_init
 * Purpose:
 *      Initialize ethertype based udf tcam entry.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      etype      - (IN) Ethertype based offset specification.
 *      hw_buf     - (OUT) Hardware buffer.
 *      priority   - (OUT) Tcam entry priority
 * Returns:
 *      BCM_E_XXX
 */
int
_field_trx2_data_qualifier_etype_tcam_key_init(int unit,
                                              bcm_field_data_ethertype_t *etype,
                                              uint32 *hw_buf, uint8 *priority)
{
    int rv;               /* Operation return status. */

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == priority)) {
        return (BCM_E_PARAM);
    }

    *priority = 0;

    /* Set valid bit. */
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, VALIDf, 1);

    /* Set l2 format. */
    rv = _field_trx2_udf_tcam_entry_l2format_init(unit, etype->l2,
                                                 hw_buf, priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Set vlan tag format. */
    rv = _field_trx2_udf_tcam_entry_vlanformat_init(unit, etype->vlan_tag,
                                                   hw_buf, priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Set ethertype value. */
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L2_ETHER_TYPEf, etype->ethertype);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L2_ETHER_TYPE_MASKf, 0xffff);
    *priority += _FP_DATA_QUALIFIER_PRIO_MISC;

    return (rv);
}

/*
 * Function:
 *      _field_trx2_data_qualifier_ip_proto_tcam_key_init
 * Purpose:
 *      Initialize ethertype based udf tcam entry.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      ip_proto   - (IN) Ip Protocol based offset specification.
 *      hw_buf     - (OUT) Hardware buffer.
 *      priority   - (OUT) Tcam entry priority
 * Returns:
 *      BCM_E_XXX
 */
int
_field_trx2_data_qualifier_ip_proto_tcam_key_init(int unit,
                                 bcm_field_data_ip_protocol_t *ip_proto,
                                 uint32 *hw_buf, uint8 *priority)
{
    int rv;               /* Operation return status. */

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == priority)) {
        return (BCM_E_PARAM);
    }

    *priority = 0;

    /* Set valid bit. */
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, VALIDf, 1);

    /* Set l2 format. */
    rv = _field_trx2_udf_tcam_entry_l2format_init(unit, ip_proto->l2,
                                                 hw_buf, priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Set vlan tag format. */
    rv = _field_trx2_udf_tcam_entry_vlanformat_init(unit, ip_proto->vlan_tag,
                                                   hw_buf, priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Set ethertype value. */
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L3_FIELDSf, (ip_proto->ip << 16));
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L3_FIELDS_MASKf, 0xff0000);
    if (ip_proto->ip) {
        *priority += _FP_DATA_QUALIFIER_PRIO_IPPROTO;
    }
    *priority += _FP_DATA_QUALIFIER_PRIO_MISC;

    return (rv);
}

/*
 * Function:
 *      _field_trx2_data_qualifier_pkt_format_tcam_key_init
 * Purpose:
 *      Initialize ethertype based udf tcam entry.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      pkt_format - (IN) Packet format based offset specification.
 *      hw_buf     - (OUT) Hardware buffer.
 *      priority   - (OUT) Tcam entry priority
 * Returns:
 *      BCM_E_XXX
 */
int
_field_trx2_data_qualifier_pkt_format_tcam_key_init(int unit,
                                 bcm_field_data_packet_format_t *pkt_format,
                                 uint32 *hw_buf, uint8 *priority)
{
    int rv;               /* Operation return status. */

    /* Input parameters check. */
    if ((NULL == hw_buf) || (NULL == priority)) {
        return (BCM_E_PARAM);
    }

    *priority = 0;

    /* Set valid bit. */
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, VALIDf, 1);

    /* Set l2 format. */
    rv = _field_trx2_udf_tcam_entry_l2format_init(unit, pkt_format->l2,
                                                 hw_buf, priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Set vlan tag format. */
    rv = _field_trx2_udf_tcam_entry_vlanformat_init(unit, pkt_format->vlan_tag,
                                                   hw_buf, priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Set l3 packet format. */
    rv = _field_trx2_udf_tcam_entry_l3_init(unit, pkt_format, hw_buf, priority);

    return (rv);
}

/*
 * Function:
 *      _bcm_field_trx2_data_qualifier_ethertype_add
 * Purpose:
 *      Add ethertype based offset to data qualifier object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      qual_id    - (IN) Data qualifier id.
 *      etype      - (IN) Ethertype based offset specification.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_field_trx2_data_qualifier_ethertype_add(int unit,  int qual_id,
                                 bcm_field_data_ethertype_t *etype)
{
    fp_udf_tcam_entry_t     tcam_buf;     /* Udf tcam entry.            */
    _field_stage_t          *stage_fc;    /* Stage field control.       */
    _field_data_qualifier_t *f_dq;        /* Data qualifier descriptor. */
    uint8                   priority;     /* Udf tcam entry priority.   */
    int                     tcam_idx;     /* Tcam insertion index.      */
    int                     offset;       /* Qualifier tcam offset.     */
    int                     rv;           /* Operation return status.   */

    /* Input parameters check. */
    if (NULL == etype) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    sal_memset(&tcam_buf, 0, sizeof(fp_udf_tcam_entry_t));

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    /* Get data qualifier info. */
    rv = _bcm_field_data_qualifier_get(unit, stage_fc, qual_id,  &f_dq);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize ethertype data qualifier key. */
    rv = _field_trx2_data_qualifier_etype_tcam_key_init(unit, etype,
                                                       (uint32 *)&tcam_buf,
                                                       &priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Reorganize the tcam and reserve an index for the entry. */
    rv = _field_trx2_udf_tcam_entry_insert(unit, (uint32*)&tcam_buf,
                                          priority, &tcam_idx);
    BCM_IF_ERROR_RETURN(rv);

    /* Calculate element offset. */
    rv = _field_trx2_data_elem_offset_calc(unit, f_dq,
                                           etype->relative_offset, &offset);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize udf offset entry. */
    rv = _field_trx2_data_tcam_offset_install(unit, f_dq, tcam_idx, offset);
    BCM_IF_ERROR_RETURN(rv);

    /* Insert udf tcam entry. */
    rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, tcam_idx, &tcam_buf);
    BCM_IF_ERROR_RETURN(rv);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_field_trx2_data_qualifier_ethertype_delete
 * Purpose:
 *      Remove ethertype based offset from data qualifier object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      qual_id    - (IN) Data qualifier id.
 *      etype      - (IN) Ethertype based offset specification.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_field_trx2_data_qualifier_ethertype_delete(int unit, int qual_id,
                                 bcm_field_data_ethertype_t *etype)
{
    _field_data_tcam_entry_t *tcam_entry_arr; /* Tcam entries array.     */
    fp_udf_tcam_entry_t      tcam_buf;     /* Udf tcam entry.            */
    _field_stage_t           *stage_fc;    /* Stage field control.       */
    _field_data_qualifier_t  *f_dq;        /* Data qualifier descriptor. */
    uint8                    priority;     /* Udf tcam entry priority.   */
    int                      tcam_idx;     /* Tcam insertion index.      */
    int                      rv;           /* Operation return status.   */

    /* Input parameters check. */
    if (NULL == etype) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    sal_memset(&tcam_buf, 0, sizeof(fp_udf_tcam_entry_t));

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);


    /* Get data qualifier info. */
    rv = _bcm_field_data_qualifier_get(unit, stage_fc, qual_id,  &f_dq);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize ethertype data qualifier key. */
    rv = _field_trx2_data_qualifier_etype_tcam_key_init(unit, etype,
                                                       (uint32 *)&tcam_buf,
                                                       &priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Match the tcam entry. */
    rv = _field_trx2_udf_tcam_entry_match(unit, stage_fc,
                               (uint32*)&tcam_buf, &tcam_idx);
    BCM_IF_ERROR_RETURN(rv);

    tcam_entry_arr = stage_fc->data_ctrl->tcam_entry_arr;
    if (tcam_entry_arr[tcam_idx].ref_count > 0) {
        tcam_entry_arr[tcam_idx].ref_count--;
    }
    if (0 == tcam_entry_arr[tcam_idx].ref_count) {
        /* Initialize udf offset entry. */
        rv = _field_trx2_data_tcam_offset_install(unit, f_dq, tcam_idx, -1);
        BCM_IF_ERROR_RETURN(rv);
        /* Reset udf tcam entry. */
        rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, tcam_idx,
                           &soc_mem_entry_null(unit, FP_UDF_TCAMm));
        BCM_IF_ERROR_RETURN(rv);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_field_trx2_data_qualifier_ip_protocol_add
 * Purpose:
 *      Add ipprotocol based offset to data qualifier object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      qual_id    - (IN) Data qualifier id.
 *      ip_proto   - (IN) Ip protocol based offset specification.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_field_trx2_data_qualifier_ip_protocol_add(int unit,  int qual_id,
                                 bcm_field_data_ip_protocol_t *ip_proto)
{
    fp_udf_tcam_entry_t     tcam_buf;     /* Udf tcam entry.            */
    _field_stage_t          *stage_fc;    /* Stage field control.       */
    _field_data_qualifier_t *f_dq;        /* Data qualifier descriptor. */
    uint8                   priority;     /* Udf tcam entry priority.   */
    int                     tcam_idx;     /* Tcam insertion index.      */
    int                     offset;       /* Qualifier tcam offset.     */
    int                     rv;           /* Operation return status.   */

    /* Input parameters check. */
    if (NULL == ip_proto) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    sal_memset(&tcam_buf, 0, sizeof(fp_udf_tcam_entry_t));

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    /* Get data qualifier info. */
    rv = _bcm_field_data_qualifier_get(unit, stage_fc, qual_id,  &f_dq);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize ipprotocol data qualifier key. */
    rv = _field_trx2_data_qualifier_ip_proto_tcam_key_init(unit, ip_proto,
                                                          (uint32 *)&tcam_buf,
                                                          &priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Reorganize the tcam and reserve an index for the entry. */
    rv = _field_trx2_udf_tcam_entry_insert(unit, (uint32*)&tcam_buf,
                                          priority, &tcam_idx);
    BCM_IF_ERROR_RETURN(rv);

    /* Calculate elem offset. */
    rv = _field_trx2_data_elem_offset_calc(unit, f_dq,
                                           ip_proto->relative_offset, &offset);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize udf offset entry. */
    rv = _field_trx2_data_tcam_offset_install(unit, f_dq, tcam_idx, offset);
    BCM_IF_ERROR_RETURN(rv);

    /* Insert udf tcam entry. */
    rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, tcam_idx, &tcam_buf);
    BCM_IF_ERROR_RETURN(rv);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_field_trx2_data_qualifier_ip_protocol_delete
 * Purpose:
 *      Remove ipprotocol based offset from data qualifier object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      qual_id    - (IN) Data qualifier id.
 *      ip_proto   - (IN) Ip protocol based offset specification.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_field_trx2_data_qualifier_ip_protocol_delete(int unit, int qual_id,
                                 bcm_field_data_ip_protocol_t *ip_proto)
{
    _field_data_tcam_entry_t *tcam_entry_arr; /* Tcam entries array.     */
    fp_udf_tcam_entry_t      tcam_buf;     /* Udf tcam entry.            */
    _field_stage_t           *stage_fc;    /* Stage field control.       */
    _field_data_qualifier_t  *f_dq;        /* Data qualifier descriptor. */
    uint8                    priority;     /* Udf tcam entry priority.   */
    int                      tcam_idx;     /* Tcam insertion index.      */
    int                      rv;           /* Operation return status.   */

    /* Input parameters check. */
    if (NULL == ip_proto) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    sal_memset(&tcam_buf, 0, sizeof(fp_udf_tcam_entry_t));

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    /* Get data qualifier info. */
    rv = _bcm_field_data_qualifier_get(unit, stage_fc, qual_id,  &f_dq);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize ipprotocol data qualifier key. */
    rv = _field_trx2_data_qualifier_ip_proto_tcam_key_init(unit, ip_proto,
                                                          (uint32 *)&tcam_buf,
                                                          &priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Match the tcam entry. */
    rv = _field_trx2_udf_tcam_entry_match(unit, stage_fc,
                                         (uint32*)&tcam_buf, &tcam_idx);
    BCM_IF_ERROR_RETURN(rv);

    tcam_entry_arr = stage_fc->data_ctrl->tcam_entry_arr;
    if (tcam_entry_arr[tcam_idx].ref_count > 0) {
        tcam_entry_arr[tcam_idx].ref_count--;
    }
    if (0 == tcam_entry_arr[tcam_idx].ref_count) {
        /* Initialize udf offset entry. */
        rv = _field_trx2_data_tcam_offset_install(unit, f_dq, tcam_idx, -1);
        BCM_IF_ERROR_RETURN(rv);
        /* Reset udf tcam entry. */
        rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, tcam_idx,
                           &soc_mem_entry_null(unit, FP_UDF_TCAMm));
        BCM_IF_ERROR_RETURN(rv);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_field_trx2_data_qualifier_packet_format_add
 * Purpose:
 *      Add ipprotocol based offset to data qualifier object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      qual_id    - (IN) Data qualifier id.
 *      pkt_format - (IN) Packet format based offset specification.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_field_trx2_data_qualifier_packet_format_add(int unit,  int qual_id,
                                 bcm_field_data_packet_format_t *pkt_format)
{
    fp_udf_tcam_entry_t     tcam_buf;     /* Udf tcam entry.            */
    _field_stage_t          *stage_fc;    /* Stage field control.       */
    _field_data_qualifier_t *f_dq;        /* Data qualifier descriptor. */
    uint8                   priority;     /* Udf tcam entry priority.   */
    int                     tcam_idx;     /* Tcam insertion index.      */
    int                     offset;       /* Qualifier tcam offset.     */
    int                     rv;           /* Operation return status.   */

    /* Input parameters check. */
    if (NULL == pkt_format) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    sal_memset(&tcam_buf, 0, sizeof(fp_udf_tcam_entry_t));

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    /* Get data qualifier info. */
    rv = _bcm_field_data_qualifier_get(unit, stage_fc, qual_id,  &f_dq);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize packet format data qualifier key. */
    rv = _field_trx2_data_qualifier_pkt_format_tcam_key_init(unit, pkt_format,
                                                          (uint32 *)&tcam_buf,
                                                          &priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Reorganize the tcam and reserve an index for the entry. */
    rv = _field_trx2_udf_tcam_entry_insert(unit, (uint32*)&tcam_buf,
                                          priority, &tcam_idx);
    BCM_IF_ERROR_RETURN(rv);

    /* Calculate elem offset. */
    rv = _field_trx2_data_elem_offset_calc(unit, f_dq,
                                           pkt_format->relative_offset,
                                           &offset);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize udf offset entry. */
    rv = _field_trx2_data_tcam_offset_install(unit, f_dq, tcam_idx, offset);
    BCM_IF_ERROR_RETURN(rv);

    /* Insert udf tcam entry. */
    rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, tcam_idx, &tcam_buf);
    BCM_IF_ERROR_RETURN(rv);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_field_trx2_data_qualifier_packet_format_delete
 * Purpose:
 *      Remove ipprotocol based offset from data qualifier object.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      qual_id    - (IN) Data qualifier id.
 *      pkt_format - (IN) Packet format based udf offset specification.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_field_trx2_data_qualifier_packet_format_delete(int unit, int qual_id,
                                 bcm_field_data_packet_format_t *pkt_format)
{
    _field_data_tcam_entry_t *tcam_entry_arr; /* Tcam entries array.     */
    fp_udf_tcam_entry_t      tcam_buf;     /* Udf tcam entry.            */
    _field_stage_t           *stage_fc;    /* Stage field control.       */
    _field_data_qualifier_t  *f_dq;        /* Data qualifier descriptor. */
    uint8                    priority;     /* Udf tcam entry priority.   */
    int                      tcam_idx;     /* Tcam insertion index.      */
    int                      rv;           /* Operation return status.   */

    /* Input parameters check. */
    if (NULL == pkt_format) {
        return (BCM_E_PARAM);
    }

    /* Initialization. */
    sal_memset(&tcam_buf, 0, sizeof(fp_udf_tcam_entry_t));

    /* Get stage field control structure. */
    rv = _field_stage_control_get(unit, _BCM_FIELD_STAGE_INGRESS, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    /* Get data qualifier info. */
    rv = _bcm_field_data_qualifier_get(unit, stage_fc, qual_id,  &f_dq);
    BCM_IF_ERROR_RETURN(rv);

    /* Initialize packet_format data qualifier key. */
    rv = _field_trx2_data_qualifier_pkt_format_tcam_key_init(unit, pkt_format,
                                                          (uint32 *)&tcam_buf,
                                                          &priority);
    BCM_IF_ERROR_RETURN(rv);

    /* Match the tcam entry. */
    rv = _field_trx2_udf_tcam_entry_match(unit, stage_fc,
                                         (uint32*)&tcam_buf, &tcam_idx);
    BCM_IF_ERROR_RETURN(rv);

    tcam_entry_arr = stage_fc->data_ctrl->tcam_entry_arr;
    if (tcam_entry_arr[tcam_idx].ref_count > 0) {
        tcam_entry_arr[tcam_idx].ref_count--;
    }
    if (0 == tcam_entry_arr[tcam_idx].ref_count) {
        /* Initialize udf offset entry. */
        rv = _field_trx2_data_tcam_offset_install(unit, f_dq, tcam_idx, -1);
        BCM_IF_ERROR_RETURN(rv);
        /* Reset udf tcam entry. */
        rv = soc_mem_write(unit, FP_UDF_TCAMm, MEM_BLOCK_ALL, tcam_idx,
                           &soc_mem_entry_null(unit, FP_UDF_TCAMm));
        BCM_IF_ERROR_RETURN(rv);
    }

    return (BCM_E_NONE);
}
#endif /* BCM_TRIUMPH2_SUPPORT || BCM_TRIDENT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT

/* Version Change..Storing Katana:stat flex field info with VFP stage */
#define BCM_WB_VERSION_1_3                SOC_SCACHE_VERSION(1,3)
#define BCM_WB_VERSION_1_2                SOC_SCACHE_VERSION(1,2)
#define BCM_WB_VERSION_1_1                SOC_SCACHE_VERSION(1,1)
#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_3

STATIC soc_field_t _vfp_slice_wide_mode_field[4] = {
    SLICE_0_DOUBLE_WIDE_MODEf,
    SLICE_1_DOUBLE_WIDE_MODEf,
    SLICE_2_DOUBLE_WIDE_MODEf,
    SLICE_3_DOUBLE_WIDE_MODEf};

#if defined (BCM_KATANA_SUPPORT) || defined (BCM_TRIUMPH3_SUPPORT) \
    || defined (BCM_TRIDENT2_SUPPORT)
static _field_flex_stat_info_t flex_info={0};
#endif

int
_bcm_field_trx_meter_rate_burst_recover(uint32 unit,
                                        uint32 meter_table,
                                        uint32 mem_idx,
                                        uint32 *prate,
                                        uint32 *pburst
                                        )
{
    uint32 ent[SOC_MAX_MEM_FIELD_WORDS];
    uint32 refresh_rate, granularity, bucket_max;

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, meter_table, MEM_BLOCK_ANY,
                                     mem_idx, ent));

    refresh_rate = soc_mem_field32_get(unit, meter_table, ent, REFRESHCOUNTf);
    granularity = soc_mem_field32_get(unit, meter_table, ent, METER_GRANf);
    bucket_max = soc_mem_field32_get(unit, meter_table, ent, BUCKETSIZEf);

    BCM_IF_ERROR_RETURN(_bcm_xgs_bucket_encoding_to_kbits
        (refresh_rate, bucket_max, granularity,
         (_BCM_XGS_METER_FLAG_GRANULARITY | _BCM_XGS_METER_FLAG_FP_POLICER),
         prate, pburst));

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_trx_meter_pool_resolve
 * Purpose:
 *     Given meter pair index, determine meter pool number and meter index
 *     in meter pool.
 * Parameters:
 *     unit             - (IN) BCM device number.
 *     stage_fc         - (IN) FP stage control info.
 *     meter_pair_idx   - (IN) Global meter pair index value. 
 *     pool_num         - (OUT) Meter pool number.
 *     meter_idx        - (OUT) Meter pair index in pool number context.
 * Returns:
 *     BCM_E_XXX
 */
STATIC
int _field_trx_meter_pool_resolve(int unit, _field_stage_t *stage_fc,
                                  int meter_pair_idx, int *pool_num,
                                  int *meter_idx)
{
    int pool_idx = _FP_INVALID_INDEX;       /* Meter pool number. */
    int meter_index  = _FP_INVALID_INDEX;   /* Meter index.       */

    /* Input parameter check. */
    if (NULL == stage_fc || NULL == pool_num || NULL == meter_idx) {
        return (BCM_E_INTERNAL);
    }

    /* Calculate pool_index and hw_index */
    for (meter_index = meter_pair_idx, pool_idx = 0;
         meter_index >= (stage_fc->meter_pool[pool_idx]->num_meter_pairs);
         meter_index -= (stage_fc->meter_pool[pool_idx]->num_meter_pairs),
                         ++pool_idx);
    *pool_num = pool_idx;
    *meter_idx = meter_index;
    return (BCM_E_NONE);
}

/*
 * Function:
 *    _field_trx_meter_index_in_use
 * Purpose:
 *    Returns success if meter index/meter pair is in use.
 * Parameters:
 *     unit             - (IN) BCM device number.
 *     stage_fc         - (IN) FP stage control info.
 *     fs               - (IN) Slice where meter resides.
 *     f_mp             - (IN) Meter pool info.
 *     meter_pair_mode  - (IN) Meter mode.
 *     meter_offset     - (IN) Odd/Even meter offset value.
 *     idx              - (IN) Meter pair index.
 * Returns:
 *     BCM_E_XXX
 */
STATIC
int _field_trx_meter_index_in_use(int unit, _field_stage_t *stage_fc,
                                _field_slice_t *fs,
                                _field_meter_pool_t *f_mp,
                                uint32 meter_pair_mode,
                                uint32 meter_offset,
                                int idx)
{
    /* Input parameter check. */
    if (NULL == stage_fc || NULL == f_mp || NULL == fs) {
        return (BCM_E_INTERNAL);
    }
    
    if (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS) {
        /* Device stage supports global meter pools. */
        if (meter_pair_mode == BCM_FIELD_METER_MODE_FLOW
            && _FP_METER_BMP_TEST(f_mp->meter_bmp, ((idx * 2) + meter_offset))) {
            /* Flow mode meter index in use. */
            return (BCM_E_NONE);
        } else if (_FP_METER_BMP_TEST(f_mp->meter_bmp, (idx * 2))
                    && _FP_METER_BMP_TEST(f_mp->meter_bmp, ((idx * 2) + 1))) {
            /* Non-Flow mode meter pair index in use. */
            return (BCM_E_NONE);
        }
    } else {
        /* Meters are per-slice resource. */
        if (meter_pair_mode == BCM_FIELD_METER_MODE_FLOW
            && _FP_METER_BMP_TEST(fs->meter_bmp, ((idx * 2) + meter_offset))) {
            /* Flow mode meter index in use. */
            return (BCM_E_NONE);
        } else if (_FP_METER_BMP_TEST(fs->meter_bmp, (idx * 2))
                    && _FP_METER_BMP_TEST(fs->meter_bmp, ((idx * 2) + 1))) {
            /* Non-Flow mode meter pair index in use. */
            return (BCM_E_NONE);
        }
    }

    return (BCM_E_NOT_FOUND);
}

/*
 * Function:
 *    _field_trx_meter_recover
 * Purpose:
 *    Recover field entry polier configuration.
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     f_ent      - (IN) Field entry structure.
 *     part       - (IN) Field entry part number.
 *     pid        - (IN) Policer identifier.
 *     level      - (IN) Policer level (0/1).
 *     polic_mem  - (IN) Policy table memory name.
 *     policy_buf - (IN) Policy table entry pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_trx_meter_recover(int             unit,
                         _field_entry_t  *f_ent,
                         int             part,
                         bcm_policer_t   pid,
                         uint32          level,
                         soc_mem_t       policy_mem,
                         uint32          *policy_buf
                         )
{
    _field_group_t *fg;                 /* Field group information.         */
    _field_stage_t *stage_fc;           /* Field stage information.         */
    _field_control_t *fc;               /* Unit field control information.  */
    _field_slice_t *fs;                 /* Field slice information.         */
    _field_policer_t *f_pl = NULL;      /* Policer descriptor pointer type. */
    _field_entry_policer_t *f_ent_pl;   /* Policer attached to entry.       */
    int rv, policer_id, meter_hw_idx;   /* Meter pair/flow meter hw index.  */
    int pair_index = _FP_INVALID_INDEX; /* Global meter pair index value.   */
    int pool_index = _FP_INVALID_INDEX; /* Meter pool number.               */
    int hw_index = _FP_INVALID_INDEX;   /* Pool local meter pair index.     */
    soc_field_t mpair_idx_f;            /* Meter pair index field.          */
    soc_field_t modifier_f;             /* Meter modifier field.            */
    uint32 idx, found = 0;              /* Policer hash idx; Policer found. */
    uint32 flags = 0;                   /* Policer flags.                   */
    uint32 meter_pair_mode = 0;         /* Flow/SrTcm/TrTcm meter modes.    */
    uint32 modifier_mode = 0;           /* Committed - 1/Excess meter - 0.  */
    uint32 meter_offset = 0;            /* Even or Odd meter index.         */
    _field_meter_pool_t *f_mp = NULL;   /* Meter pool information.          */
    _meter_config_t meter_conf;         /* Meter configuration in hardware. */
    soc_mem_t meter_mem = 0;            /* Meter memory name.               */

    fg = f_ent->group;
    fs = f_ent[part].fs;

    sal_memset(&meter_conf, 0, sizeof(_meter_config_t));

    /* Get field control */
    BCM_IF_ERROR_RETURN(_field_control_get(unit, &fc));

    /* Get stage control */
    rv = _field_stage_control_get(unit, fg->stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    /* Retrieve meter config parameters from policy table and meter table. */
    switch (fg->stage_id) {
        case  _BCM_FIELD_STAGE_INGRESS:
            meter_mem = FP_METER_TABLEm;
            /* Get - Flow/SrTcm/TrTcm mode. */
            if (level == 1) {
                if ((meter_pair_mode = soc_mem_field32_get
                                        (unit, policy_mem,
                                         policy_buf, METER_SHARING_MODEf))
                    == 0) {
                    return (BCM_E_NONE);
                }
                mpair_idx_f = SHARED_METER_PAIR_INDEXf;
                modifier_f = METER_SHARING_MODE_MODIFIERf;
            } else {
                if ((meter_pair_mode = soc_mem_field32_get
                                        (unit, policy_mem,
                                         policy_buf, METER_PAIR_MODEf))
                    == 0) {
                    return (BCM_E_NONE);
                }
                mpair_idx_f = SOC_IS_TD_TT(unit) ? METER_PAIR_INDEX_EVENf
                                : METER_PAIR_INDEXf;
                modifier_f = METER_PAIR_MODE_MODIFIERf;
            }

            /* Get meter pair index value from policy table. */
            pair_index = soc_mem_field32_get(unit, policy_mem, policy_buf,
                                             mpair_idx_f);

            /* modifier_mode: 0 = Excess Meter, 1 = Committed Meter. */
            modifier_mode = soc_mem_field32_get(unit, policy_mem, policy_buf,
                                                modifier_f);

            /* Determine meter offset value for flow meters. */
            if (BCM_FIELD_METER_MODE_FLOW == meter_pair_mode) {
                /* Trident device supports only Level0 policer mode. */
                if (SOC_IS_TD_TT(unit) && (0 == level)) {
                    meter_offset = (1 == soc_mem_field32_get(unit,
                                            policy_mem, policy_buf,
                                            METER_TEST_EVENf)) ? 0 : 1;
                } else {
                    meter_offset = (0 == modifier_mode) ? 0 : 1;
                }
            }

            /* Get meter pool number and meter pair index in pool context. */
            BCM_IF_ERROR_RETURN
                (_field_trx_meter_pool_resolve(unit, stage_fc, pair_index,
                                               &pool_index, &hw_index));

            /* Get meter pool info pointer. */
            f_mp = stage_fc->meter_pool[pool_index];

            /* Setup meter pool to entry slice mapping. */
            if (_FP_INVALID_INDEX == f_mp->slice_id) {
                f_mp->slice_id = fg->slices->slice_number;
                f_mp->level = level;
            }
            break;

        case _BCM_FIELD_STAGE_EGRESS:
            meter_mem = EFP_METER_TABLEm;
            
            meter_conf.meter_mode = soc_mem_field32_get(unit, policy_mem,
                                                        policy_buf,
                                                        METER_PAIR_MODEf);
            meter_conf.meter_idx = soc_mem_field32_get(unit, policy_mem,
                                                       policy_buf,
                                                       METER_INDEX_EVENf);
            meter_conf.meter_update_odd = soc_mem_field32_get(unit,
                                            policy_mem, policy_buf,
                                            METER_UPDATE_ODDf);
            meter_conf.meter_test_odd = soc_mem_field32_get(unit, policy_mem,
                                                            policy_buf,
                                                            METER_TEST_ODDf);
            meter_conf.meter_update_even = soc_mem_field32_get(unit,
                                            policy_mem, policy_buf,
                                            METER_UPDATE_EVENf);
            meter_conf.meter_test_even = soc_mem_field32_get(unit,
                                                             policy_mem,
                                                             policy_buf,
                                                             METER_TEST_EVENf);

            /* Get - Flow/SrTcm/TrTcm mode. */
            meter_pair_mode = meter_conf.meter_mode;
            
            /* modifier_mode: 0 = Excess Meter, 1 = Committed Meter. */
            if (BCM_FIELD_METER_MODE_FLOW == meter_pair_mode) {
                modifier_mode = (meter_conf.meter_update_even
                                 && meter_conf.meter_test_even) ? 0 : 1;
                meter_offset = (0 == modifier_mode) ? 0 : 1;
            }

            /* Slice in which meter config resides. */
            pool_index = fs->slice_number;

            /* Slice meter pair index. */
            hw_index = meter_conf.meter_idx;

            if ((BCM_FIELD_METER_MODE_DEFAULT == meter_pair_mode)
                && (0 == hw_index)) {
                /* bcmPolicerModePassThrough */
                return BCM_E_NONE;
            }
            break;

        case _BCM_FIELD_STAGE_EXTERNAL:
            meter_mem = FP_METER_TABLEm;

            /* Get - Flow/SrTcm/TrTcm mode. */
            if (level == 1) {
                if ((meter_pair_mode = soc_mem_field32_get
                                        (unit, policy_mem, policy_buf,
                                         METER_SHARING_MODEf))
                    == 0) {
                    return (BCM_E_NONE);
                }
                mpair_idx_f = METER_PAIR_INDEXf;
                modifier_f = METER_SHARING_MODE_MODIFIERf;
            } else {
                if ((meter_pair_mode = soc_mem_field32_get(unit, policy_mem,
                                                           policy_buf,
                                                           METER_PAIR_MODEf))
                    == 0) {
                    return (BCM_E_NONE);
                }
                
                mpair_idx_f = METER_PAIR_INDEXf;
                modifier_f = METER_PAIR_MODE_MODIFIERf;
            }
            
            pair_index = soc_mem_field32_get(unit, policy_mem, policy_buf,
                                             mpair_idx_f);

            /* modifier_mode: 0 = Excess Meter, 1 = Committed Meter. */
            modifier_mode = soc_mem_field32_get(unit, policy_mem, policy_buf,
                                                modifier_f);

            /* Determine meter offset value for flow meters. */
            if (BCM_FIELD_METER_MODE_FLOW == meter_pair_mode) {
                meter_offset = (0 == modifier_mode) ? 0 : 1;
            }

            /* Get meter pool number and meter index. */
            BCM_IF_ERROR_RETURN
                (_field_trx_meter_pool_resolve(unit, stage_fc, pair_index,
                                               &pool_index, &hw_index));
            f_mp = stage_fc->meter_pool[pool_index];
            if (_FP_INVALID_INDEX == f_mp->slice_id) {
                f_mp->slice_id = stage_fc->tcam_slices;
                f_mp->level = level;
            }
            break;

        default:
            /* Must be a valid stage. */
            return (BCM_E_INTERNAL);
    }

    /* Check if meter index is already in use. */
    if (BCM_SUCCESS(_field_trx_meter_index_in_use(unit, stage_fc, fs, f_mp,
                                                  meter_pair_mode,
                                                  meter_offset,
                                                  hw_index))) {
        found = 0;
        /* 
         * Check and increment reference count for poliers that are
         * attached to more than one entry.
         */ 
        for (idx = 0; idx < _FP_HASH_SZ(fc); idx++) {
            f_pl = fc->policer_hash[idx];
            while (f_pl != NULL) {
                if ((f_pl->hw_index == hw_index) &&
                    (f_pl->pool_index == pool_index) &&
                    (f_pl->stage_id == fg->stage_id)) {
                    found = 1;
                    break;
                }
                f_pl = f_pl->next;
            }
            if (found) {
                break;
            }
        }
        if (!found) {
            return BCM_E_INTERNAL;
        }
        f_pl->hw_ref_count++;
        f_pl->sw_ref_count++;
    } else {
        /* Policer does not exist => Allocate new policer object */
        if (fc->l2warm) {
            policer_id = pid;
        } else {
            BCM_IF_ERROR_RETURN(_field_policer_id_alloc(unit, &policer_id));
        }

        _FP_XGS3_ALLOC(f_pl, sizeof (_field_policer_t), "Field policer entity");
        if (f_pl == NULL) {
            return (BCM_E_MEMORY);
        }

        flags |= _FP_POLICER_INSTALLED;
        if ((f_pl->level = level) == 1 && meter_pair_mode == 1) {
            f_pl->cfg.flags |= BCM_POLICER_COLOR_MERGE_OR;
        }
        f_pl->sw_ref_count = 2;
        f_pl->hw_ref_count = 1;
        f_pl->pid          = policer_id;
        f_pl->stage_id     = fg->stage_id;
        f_pl->pool_index   = pool_index;
        f_pl->hw_index     = hw_index;

        /* Calculate hardware meter index. */
        if (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS) {
            /*
             * Hw index is:
             * ((Pool number * pool_size) + (2 * pair number)
             */
            meter_hw_idx = (pool_index
                            * stage_fc->meter_pool[pool_index]->size)
                            + (2 * hw_index);
        } else {
            /*
             * Hw index is:
             * (slice index) + (2 * pair number)
             */
            meter_hw_idx = stage_fc->slices[pool_index].start_tcam_idx
                            + (2 * hw_index);
        }

        switch (meter_pair_mode) {
            case BCM_FIELD_METER_MODE_DEFAULT: /* 0 */
                f_pl->cfg.mode = bcmPolicerModeGreen;
                break;
            
            case BCM_FIELD_METER_MODE_FLOW: /* 1 */
                switch (fg->stage_id) {
                    case _BCM_FIELD_STAGE_INGRESS:
                    case _BCM_FIELD_STAGE_EXTERNAL:
                        if (SOC_IS_TD_TT(unit)) {
                            f_pl->cfg.mode = bcmPolicerModeCommitted;
                            if (meter_offset) {
                                /* Flow mode using committed hardware meter. */
                                _FP_POLICER_EXCESS_HW_METER_CLEAR(f_pl);
                            } else {
                                /* Flow mode using excess hardware meter. */
                                _FP_POLICER_EXCESS_HW_METER_SET(f_pl);
                            }
                        } else {
                            f_pl->cfg.mode = bcmPolicerModeCommitted;
                            if (1 == modifier_mode) {
                                /* Flow mode using committed hardware meter. */
                                _FP_POLICER_EXCESS_HW_METER_CLEAR(f_pl);
                            } else if (0 == modifier_mode) {
                                /* Flow mode using excess hardware meter. */
                                _FP_POLICER_EXCESS_HW_METER_SET(f_pl);
                            } else {
                                sal_free(f_pl);
                                return BCM_E_INTERNAL;
                            }
                        }

                        /* Update with meter_offset for flow mode meters. */
                        meter_hw_idx += meter_offset;

                        /* Get policer rates. */
                        _bcm_field_trx_meter_rate_burst_recover(unit,
                            FP_METER_TABLEm, meter_hw_idx,
                            &f_pl->cfg.ckbits_sec,
                            &f_pl->cfg.ckbits_burst);
                        break;

                    case _BCM_FIELD_STAGE_EGRESS:
                        f_pl->cfg.mode = bcmPolicerModeCommitted;
                        if (1 == modifier_mode) {
                            /* Flow mode using committed hardware meter. */
                            _FP_POLICER_EXCESS_HW_METER_CLEAR(f_pl);
                        } else if (0 == modifier_mode) {
                            /* Flow mode using excess hardware meter. */
                            _FP_POLICER_EXCESS_HW_METER_SET(f_pl);
                        } else {
                            sal_free(f_pl);
                            return BCM_E_INTERNAL;
                        }

                        /* Update with meter_offset for flow mode meters. */
                        meter_hw_idx += meter_offset;

                        _bcm_field_trx_meter_rate_burst_recover(unit,
                            EFP_METER_TABLEm, meter_hw_idx,
                            &f_pl->cfg.ckbits_sec,
                            &f_pl->cfg.ckbits_burst);
                        break;
                    default:
                        /* Must be a valid pipeline stage. */
                        return (BCM_E_INTERNAL);
                }
                break;

            case BCM_FIELD_METER_MODE_trTCM_COLOR_BLIND: /* 2 */
                f_pl->cfg.flags |= BCM_POLICER_COLOR_BLIND;
                /* Fall through */
            case BCM_FIELD_METER_MODE_trTCM_COLOR_AWARE: /* 3 */
                f_pl->cfg.mode = bcmPolicerModeTrTcm;

                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    meter_hw_idx, &f_pl->cfg.pkbits_sec,
                    &f_pl->cfg.pkbits_burst);

                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    meter_hw_idx + 1, &f_pl->cfg.ckbits_sec,
                    &f_pl->cfg.ckbits_burst);
                break;
        
            case BCM_FIELD_METER_MODE_srTCM_COLOR_BLIND: /* 6 */
                f_pl->cfg.flags |= BCM_POLICER_COLOR_BLIND;
                /* Fall through */
            case BCM_FIELD_METER_MODE_srTCM_COLOR_AWARE: /* 7 */
                if (fg->stage_id == _BCM_FIELD_STAGE_EGRESS) {
                    f_pl->cfg.mode = bcmPolicerModeSrTcm;
                } else {
                    f_pl->cfg.mode = modifier_mode
                                        ? bcmPolicerModeSrTcmModified
                                        : bcmPolicerModeSrTcm;
                }

                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    meter_hw_idx, &f_pl->cfg.pkbits_sec,
                    &f_pl->cfg.pkbits_burst);
                
                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    meter_hw_idx + 1, &f_pl->cfg.ckbits_sec,
                    &f_pl->cfg.ckbits_burst);
                break;
            case 4:
                if (fg->stage_id == _BCM_FIELD_STAGE_EGRESS) {
                    f_pl->cfg.mode = bcmPolicerModePassThrough;
                    break;
                } else {
                    f_pl->cfg.flags |= BCM_POLICER_COLOR_BLIND;
                    /* Fall through */
                }
            case 5:
                if (fg->stage_id != _BCM_FIELD_STAGE_EGRESS) {
                    f_pl->cfg.mode = modifier_mode
                                        ? bcmPolicerModeCoupledTrTcmDs
                                        : bcmPolicerModeTrTcmDs;
                }
                break;
            default:
                ;
        }

        /* Assume policer was created using the policer_create API */
        if (fg->flags & _FP_GROUP_SPAN_DOUBLE_SLICE) {
            if (fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
                if ((part == 2) || (part == 3)) {
                    f_ent->flags |= _FP_ENTRY_POLICER_IN_SECONDARY_SLICE;
                }
            } else {
                if (part == 1) {
                    f_ent->flags |= _FP_ENTRY_POLICER_IN_SECONDARY_SLICE;
                }
            }
        }

        _FP_HASH_INSERT(fc->policer_hash, f_pl,
            (policer_id & _FP_HASH_INDEX_MASK(fc)));
        fc->policer_count++;

        /* Mark meter index as used in slice/pool */
        if (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS
            && (_BCM_FIELD_STAGE_EGRESS != fg->stage_id)) {
            if (!_FP_POLICER_IS_FLOW_MODE(f_pl)) {
                _FP_METER_BMP_ADD(f_mp->meter_bmp, (hw_index * 2));
                _FP_METER_BMP_ADD(f_mp->meter_bmp, ((hw_index * 2) + 1));
                f_mp->free_meters -= 2;
            } else {
                _FP_METER_BMP_ADD(f_mp->meter_bmp,
                                  ((hw_index * 2) + meter_offset));
                f_mp->free_meters--;
            }
        } else {
            if (!_FP_POLICER_IS_FLOW_MODE(f_pl)) {
                _FP_METER_BMP_ADD(fs->meter_bmp, (hw_index * 2));
                _FP_METER_BMP_ADD(fs->meter_bmp, ((hw_index * 2) + 1));
            } else {
                _FP_METER_BMP_ADD(fs->meter_bmp,
                                  ((hw_index * 2) + meter_offset));
            }
        }
        fg->group_status.meter_count++;
    }

    /* Associate the policer object with the entry */
    f_ent_pl = &f_ent->policer[0];
    f_ent_pl->flags |= (_FP_POLICER_VALID | flags);
    f_ent_pl->pid = f_pl->pid;

    return BCM_E_NONE;
}

/*
 * Function:
 *    _field_sc_cq_meter_recover
 * Purpose:
 *    Recover field entry polier configuration.
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     f_ent      - (IN) Field entry structure.
 *     part       - (IN) Field entry part number.
 *     pid        - (IN) Policer identifier.
 *     policy_buf - (IN) Policy table entry pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_sc_cq_meter_recover(int            unit,
                           _field_entry_t *f_ent,
                           int            part,
                           bcm_policer_t  pid,
                           uint32         *policy_buf
                           )
{
    int rv, policer_id;                    /* Policer identifier.           */
    int pair_index = _FP_INVALID_INDEX;    /* Global meter pair index.      */
    int meter_hw_idx = _FP_INVALID_INDEX;  /* Meter index value.            */
    int pool_index = _FP_INVALID_INDEX;    /* Meter pool index.             */
    int hw_index = _FP_INVALID_INDEX;      /* Pool local meter pair index.  */
    uint32 ent_pl_flags = 0;               /* Entry policer flags.          */
    uint32 stage_id;                       /* Field processor Stage ID.     */
    uint32 meter_pair_mode = 0;            /* Flow/SrTcm/TrTcm meter modes. */
    uint32 modifier_mode = 0;              /* Committed(1)/Excess(0) meter. */
    uint32 meter_offset = 0;               /* Even or Odd meter index.      */
    _field_group_t *fg;                    /* Field group information.      */
    _field_stage_t *stage_fc;              /* Field stage information.      */
    _field_control_t *fc;                  /* Field control information.    */
    _field_slice_t *fs;                    /* Field slice information.      */
    _field_policer_t *f_pl = NULL;         /* Policer descriptor info.      */
    _field_entry_policer_t *f_ent_pl;      /* Policer attached to entry.    */
    _field_meter_pool_t *f_mp = NULL;      /* Meter pool information.       */
    _meter_config_t meter_conf;            /* Meter config in hardware.     */
    soc_mem_t meter_mem;                   /* Meter memory name.            */
    
    fg = f_ent->group;
    fs = f_ent[part].fs;

    /* Get field control handle. */
    BCM_IF_ERROR_RETURN(_field_control_get(unit, &fc));
    stage_id = f_ent->group->stage_id;

    /* Get unit stage control handle. */
    rv = _field_stage_control_get(unit, stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    switch (stage_id) {
        /* Ingress stage */
        case _BCM_FIELD_STAGE_INGRESS:
            meter_mem = FP_METER_TABLEm;
            meter_conf.meter_mode
                = soc_mem_field32_get(unit, FP_POLICY_TABLEm, policy_buf,
                                      METER_PAIR_MODEf);
            meter_conf.meter_mode_modifier
                = soc_mem_field32_get(unit, FP_POLICY_TABLEm, policy_buf,
                                      METER_PAIR_MODE_MODIFIERf);
            meter_conf.meter_idx
                = soc_mem_field32_get(unit, FP_POLICY_TABLEm, policy_buf,
                                      METER_INDEX_EVENf);
            meter_conf.meter_update_odd
                = soc_mem_field32_get(unit, FP_POLICY_TABLEm, policy_buf,
                                      METER_UPDATE_ODDf);
            meter_conf.meter_test_odd
                = soc_mem_field32_get(unit, FP_POLICY_TABLEm, policy_buf,
                                      METER_TEST_ODDf);
            meter_conf.meter_update_even
                = soc_mem_field32_get(unit, FP_POLICY_TABLEm, policy_buf,
                                      METER_UPDATE_EVENf);
            meter_conf.meter_test_even
                = soc_mem_field32_get(unit, FP_POLICY_TABLEm, policy_buf,
                                      METER_TEST_EVENf);

            meter_pair_mode = meter_conf.meter_mode;
            pair_index = meter_conf.meter_idx;
            if ((meter_pair_mode == 0) && (pair_index == 0)) {
                return BCM_E_NONE;
            }

            if (BCM_FIELD_METER_MODE_FLOW == meter_pair_mode) {
                /* modifier_mode: 0 = Excess Meter, 1 = Committed Meter. */
                modifier_mode = meter_conf.meter_mode_modifier;
                /* Determine meter offset value for flow meters. */
                meter_offset = (0 == modifier_mode) ? 0 : 1;
            }

            /* Get meter pool number and meter index. */
            BCM_IF_ERROR_RETURN
                (_field_trx_meter_pool_resolve(unit, stage_fc, pair_index,
                                               &pool_index, &hw_index));

            f_mp = stage_fc->meter_pool[pool_index];
            if (_FP_INVALID_INDEX == f_mp->slice_id) {
                f_mp->slice_id = fg->slices->slice_number;
                f_mp->level = 0;
            }
            break;
        case _BCM_FIELD_STAGE_EGRESS:
            /* Egress stage */
            meter_mem = EFP_METER_TABLEm;
            meter_conf.meter_mode = soc_mem_field32_get(unit, EFP_POLICY_TABLEm,
                                                        policy_buf,
                                                        METER_PAIR_MODEf);
            meter_conf.meter_idx = soc_mem_field32_get(unit, EFP_POLICY_TABLEm,
                                                       policy_buf,
                                                       METER_INDEX_EVENf);
            meter_conf.meter_update_odd = soc_mem_field32_get(unit,
                                            EFP_POLICY_TABLEm, policy_buf,
                                            METER_UPDATE_ODDf);
            meter_conf.meter_test_odd = soc_mem_field32_get(unit,
                                            EFP_POLICY_TABLEm, policy_buf,
                                            METER_TEST_ODDf);
            meter_conf.meter_update_even = soc_mem_field32_get(unit,
                                            EFP_POLICY_TABLEm, policy_buf,
                                            METER_UPDATE_EVENf);
            meter_conf.meter_test_even = soc_mem_field32_get(unit,
                                            EFP_POLICY_TABLEm, policy_buf,
                                            METER_TEST_EVENf);
            meter_pair_mode = meter_conf.meter_mode;
            pool_index = fs->slice_number;
            hw_index = meter_conf.meter_idx;
            if ((meter_pair_mode == 0) && (hw_index == 0)) {
                return BCM_E_NONE;
            }
        
            if (BCM_FIELD_METER_MODE_FLOW == meter_pair_mode) {
                /* modifier_mode: 0 = Excess Meter, 1 = Committed Meter. */
                modifier_mode = (meter_conf.meter_update_even
                                    && meter_conf.meter_test_even) ? 0 : 1;
                /* Determine meter offset value for flow meters. */
                meter_offset = (0 == modifier_mode) ? 0 : 1;
            }
            break;
        default:
            /* Must be a valid stage. */
            return (BCM_E_INTERNAL);
    }

    /* Check if meter index is already in use. */
    if (BCM_SUCCESS(_field_trx_meter_index_in_use(unit, stage_fc, fs, f_mp,
                                                  meter_pair_mode,
                                                  meter_offset,
                                                  hw_index))) {
        uint32 idx, found = 0;

        for (idx = 0; idx < _FP_HASH_SZ(fc); idx++) {
            f_pl = fc->policer_hash[idx];
            while (f_pl != NULL) {
                if ((f_pl->hw_index == hw_index) &&
                    (f_pl->pool_index == pool_index) &&
                    (f_pl->stage_id == stage_id)) {
                    found = 1;
                    break;
                }
                f_pl = f_pl->next;
            }
            if (found) {
                break;
            }
        }
        if (!found) {
            return BCM_E_INTERNAL;
        }
        f_pl->hw_ref_count++;
        f_pl->sw_ref_count++;
    } else {
        /* Policer does not exist => Allocate new policer object */
        if (fc->l2warm) {
            policer_id = pid;
        } else {
            BCM_IF_ERROR_RETURN(_field_policer_id_alloc(unit, &policer_id));
        }

        _FP_XGS3_ALLOC(f_pl, sizeof (_field_policer_t), "Field policer entity");
        if (f_pl == NULL) {
            return (BCM_E_MEMORY);
        }

        ent_pl_flags       |= _FP_POLICER_INSTALLED;
        f_pl->sw_ref_count = 2;
        f_pl->hw_ref_count = 1;
        f_pl->pid          = policer_id;
        f_pl->stage_id     = fg->stage_id;
        f_pl->pool_index   = pool_index;
        f_pl->hw_index     = hw_index;

        /* Calculate hardware meter index. */
        if (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS) {
            /*
             * Hw index is:
             * ((Pool number * pool_size) + (2 * pair number)
             */
            meter_hw_idx = (pool_index
                            * stage_fc->meter_pool[pool_index]->size)
                            + (2 * hw_index);
        } else {
            /*
             * Hw index is:
             * (slice index) + (2 * pair number)
             */
            meter_hw_idx = stage_fc->slices[pool_index].start_tcam_idx
                            + (2 * hw_index);
        }

        switch (meter_pair_mode) {
            case BCM_FIELD_METER_MODE_DEFAULT:
                f_pl->cfg.mode = bcmPolicerModeGreen;
                break;

            case BCM_FIELD_METER_MODE_FLOW:
                f_pl->cfg.mode = bcmPolicerModeCommitted;
                if (meter_offset) {
                    /* Flow mode using committed hardware meter. */
                    _FP_POLICER_EXCESS_HW_METER_CLEAR(f_pl);
                } else {
                    /* Flow mode using excess hardware meter. */
                    _FP_POLICER_EXCESS_HW_METER_SET(f_pl);
                }
                meter_hw_idx += meter_offset;
                _bcm_field_trx_meter_rate_burst_recover(unit,
                    meter_mem, meter_hw_idx, &f_pl->cfg.ckbits_sec,
                    &f_pl->cfg.ckbits_burst);
                break;

            case BCM_FIELD_METER_MODE_trTCM_COLOR_BLIND:
                f_pl->cfg.flags |= BCM_POLICER_COLOR_BLIND;
                /* Fall through */
            case BCM_FIELD_METER_MODE_trTCM_COLOR_AWARE:
                f_pl->cfg.mode = bcmPolicerModeTrTcm;

                /* Get excess meter config from hardware. */
                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    meter_hw_idx, &f_pl->cfg.pkbits_sec,
                    &f_pl->cfg.pkbits_burst);

                /* Get committed meter config from hardware. */
                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    (meter_hw_idx + 1), &f_pl->cfg.ckbits_sec,
                    &f_pl->cfg.ckbits_burst);
                break;
            case 4:
                if (fg->stage_id == _BCM_FIELD_STAGE_EGRESS) {
                    f_pl->cfg.mode = bcmPolicerModePassThrough;
                }
                break;
            case 5:
                break;
            case BCM_FIELD_METER_MODE_srTCM_COLOR_BLIND:
                f_pl->cfg.flags |= BCM_POLICER_COLOR_BLIND;
                /* Fall through */
            case BCM_FIELD_METER_MODE_srTCM_COLOR_AWARE:
                f_pl->cfg.mode = bcmPolicerModeSrTcm;

                /* Get excess meter config from hardware. */
                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    meter_hw_idx, &f_pl->cfg.pkbits_sec,
                    &f_pl->cfg.pkbits_burst);

                /* Get committed meter config from hardware. */
                _bcm_field_trx_meter_rate_burst_recover(unit, meter_mem,
                    (meter_hw_idx + 1), &f_pl->cfg.ckbits_sec,
                    &f_pl->cfg.ckbits_burst);
                break;
            default:
                break;
        }

        /* Assume policer was created using the policer_create API */
        if (fg->flags & _FP_GROUP_SPAN_DOUBLE_SLICE) {
            if (fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
                if ((part == 2) || (part == 3)) {
                    f_ent->flags |= _FP_ENTRY_POLICER_IN_SECONDARY_SLICE;
                }
            } else {
                if (part == 1) {
                    f_ent->flags |= _FP_ENTRY_POLICER_IN_SECONDARY_SLICE;
                }
            }
        }
        _FP_HASH_INSERT(fc->policer_hash, f_pl,
                        (policer_id & _FP_HASH_INDEX_MASK(fc)));
        fc->policer_count++;

        /* Mark meter index as used in slice/pool */
        if (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS
            && (_BCM_FIELD_STAGE_EGRESS != stage_id)) {
            if (!_FP_POLICER_IS_FLOW_MODE(f_pl)) {
                _FP_METER_BMP_ADD(f_mp->meter_bmp, (hw_index * 2));
                _FP_METER_BMP_ADD(f_mp->meter_bmp, ((hw_index * 2) + 1));
                f_mp->free_meters -= 2;
            } else {
                _FP_METER_BMP_ADD(f_mp->meter_bmp,
                                  ((hw_index * 2) + meter_offset));
                f_mp->free_meters--;
            }
        } else {
            if (!_FP_POLICER_IS_FLOW_MODE(f_pl)) {
                _FP_METER_BMP_ADD(fs->meter_bmp, (hw_index * 2));
                _FP_METER_BMP_ADD(fs->meter_bmp, ((hw_index * 2) + 1));
            } else {
                _FP_METER_BMP_ADD(fs->meter_bmp,
                                  ((hw_index * 2) + meter_offset));
            }
        }
        fg->group_status.meter_count++;
    }

    /* Associate the policer object with the entry */
    f_ent_pl = &f_ent->policer[0];
    f_ent_pl->flags |= (_FP_POLICER_VALID | ent_pl_flags);
    f_ent_pl->pid = f_pl->pid;

    return BCM_E_NONE;
}

STATIC int
_field_tr2_counter_recover(int              unit,
                           _field_entry_t   *f_ent,
                           uint32           ctr_mode,
                           uint32           ctr_idx,
                           int              part,
                           bcm_field_stat_t sid
                           )
{
    _field_group_t *fg;
    _field_stage_t *stage_fc;
    _field_stage_id_t stage_id;
    _field_control_t *fc;
    _field_slice_t *fs;
    _field_stat_t *f_st = NULL;
    _field_entry_stat_t *f_ent_st = NULL;
    int rv, idx, stat_id, found;
    uint32 sub_mode = 0, ent_st_flags = 0;
    bcm_field_stat_t stat_arr[4];
    uint8 nstat = 2;
    uint8 hw_entry_count = 1;
    int index = 0, rshift = 0;

    fg = f_ent->group;
    fs = f_ent[part].fs;

    /* Get field control and stage control */
    BCM_IF_ERROR_RETURN(_field_control_get(unit, &fc));
    stage_id = f_ent->group->stage_id;
    rv = _field_stage_control_get(unit, stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    sal_memset(stat_arr, 0, sizeof(stat_arr));

    /* Search if counter has already been detected */
    /* Go over the counter in the slice */
    if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
        hw_entry_count =
            ((ctr_mode & (0x7)) ? 1 : 0) + ((ctr_mode & (0x7 << 3)) ? 1 : 0);
    } else {
        sub_mode = ctr_mode & ~BCM_FIELD_COUNTER_MODE_BYTES;
        if ((sub_mode == BCM_FIELD_COUNTER_MODE_RED) ||
            (sub_mode == BCM_FIELD_COUNTER_MODE_YELLOW) ||
            (sub_mode == BCM_FIELD_COUNTER_MODE_GREEN) ||
            (sub_mode == BCM_FIELD_COUNTER_MODE_YES_NO) ||
            (sub_mode == BCM_FIELD_COUNTER_MODE_DEFAULT)) {
            hw_entry_count = 1;
        } else {
            hw_entry_count = 2;
        }
    }

    found = 0;
    if (stage_id != _BCM_FIELD_STAGE_EXTERNAL) {
        if (((1 == _FP_COUNTER_BMP_TEST(fs->counter_bmp, 2 * ctr_idx)) &&
             (1 == _FP_COUNTER_BMP_TEST(fs->counter_bmp, 2 * ctr_idx + 1)) &&
             (hw_entry_count == 2)) ||
            ((1 == _FP_COUNTER_BMP_TEST(fs->counter_bmp, 2 * ctr_idx)) &&
             (hw_entry_count == 1) && (ctr_mode % 2 == 0)) ||
            ((1 == _FP_COUNTER_BMP_TEST(fs->counter_bmp, 2 * ctr_idx + 1)) &&
             (hw_entry_count == 1) && (ctr_mode % 2))) {
            /* Counter has been detected - increment reference count */
            /* Happens when counter is shared by different entries */
            /* Search the hash to match against the HW index */

            for (idx = 0; idx < _FP_HASH_SZ(fc); idx++) {
                f_st = fc->stat_hash[idx];
                while (f_st != NULL) {
                    if ((f_st->hw_index == ctr_idx) &&
                        (f_st->pool_index == fs->slice_number) &&
                        (f_st->hw_mode == ctr_mode) &&
                        (f_st->stage_id == stage_id)) {
                        found = 1;
                        break;
                    }
                    f_st = f_st->next;
                }
                if (found) {
                    break;
                }
            }
            if (!found) {
                return BCM_E_INTERNAL;
            }
            f_st->hw_ref_count++;
            f_st->sw_ref_count++;
            f_ent_st = &f_ent->statistic;
        }
    }

    if (!found) {
        /* Allocate new stat object */
        if (fc->l2warm) {
            stat_id = sid;
        } else {
            BCM_IF_ERROR_RETURN(_field_stat_id_alloc(unit, &stat_id));
        }
        _FP_XGS3_ALLOC(f_st, sizeof (_field_stat_t), "Field stat entity");
        if (NULL == f_st) {
            return (BCM_E_MEMORY);
        }
        ent_st_flags |= _FP_ENTRY_STAT_INSTALLED;
        f_st->sw_ref_count = 2;
        f_st->hw_ref_count = 1;
        f_st->pool_index = fs->slice_number;
        f_st->hw_index = ctr_idx;
        f_st->sid = stat_id;
        f_st->stage_id = fg->stage_id;
        f_st->gid = fg->gid;
        f_st->hw_mode = ctr_mode;
        f_ent_st = &f_ent->statistic;

        if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
            while (hw_entry_count >= 1) {

                /* Lower counter is not used */
                if ((1 == hw_entry_count) &&
                    (2 == nstat) &&
                    (0 == ((ctr_mode) & 0x7))) {
                    rshift += 3;
                }

                switch ((ctr_mode >> rshift) & 0x7) {
                    case 1:
                        /* TR Mode = 4 */
                        /* 001 */
                        stat_arr[index++] = bcmFieldStatGreenBytes;
                        stat_arr[index++] = bcmFieldStatGreenPackets;
                        break;
                    case 2:
                        /* 010 */
                        stat_arr[index++] = bcmFieldStatYellowBytes;
                        stat_arr[index++] = bcmFieldStatYellowPackets;
                        break;
                    case 3:
                        /* TR Mode = 3 */
                        /* TR Mode = 6 */
                        /* 011 */
                        stat_arr[index++] = bcmFieldStatNotRedBytes;
                        stat_arr[index++] = bcmFieldStatNotRedPackets;
                        break;
                    case 4:
                        /* TR Mode = 3 */
                        /* 100 */
                        stat_arr[index++] = bcmFieldStatRedBytes;
                        stat_arr[index++] = bcmFieldStatRedPackets;
                        break;
                    case 5:
                        /* TR Mode = 5 */
                        /* 101 */
                        stat_arr[index++] = bcmFieldStatNotYellowBytes;
                        stat_arr[index++] = bcmFieldStatNotYellowPackets;
                        break;
                    case 6:
                        /* TR Mode = 4 */
                        /* TR Mode = 7 */
                        /* 110 */
                        stat_arr[index++] = bcmFieldStatNotGreenBytes;
                        stat_arr[index++] = bcmFieldStatNotGreenPackets;
                        break;
                    case 7:
                        /* TR Mode = 1 */
                        /* 111 */
                        stat_arr[index++] = bcmFieldStatBytes;
                        stat_arr[index++] = bcmFieldStatPackets;
                        break;
                    default:
                        break;
                }
                hw_entry_count--;
                /* Get Upper counter mode */
                rshift += 3;
                /* Upper and Lower both counters used */
                if (1 == hw_entry_count) {
                    nstat += 2;
                }
            }
        } else {
            switch (sub_mode) {
                case BCM_FIELD_COUNTER_MODE_DEFAULT:
                    stat_arr[0] = bcmFieldStatBytes;
                    stat_arr[1] = bcmFieldStatPackets;
                    nstat = 2;
                    break;
                case BCM_FIELD_COUNTER_MODE_YES_NO:
                    stat_arr[0] = bcmFieldStatBytes;
                    stat_arr[1] = bcmFieldStatPackets;
                    nstat = 2;
                    break;
                case BCM_FIELD_COUNTER_MODE_RED_NOTRED:
                    stat_arr[0] = bcmFieldStatRedBytes;
                    stat_arr[1] = bcmFieldStatRedPackets;
                    stat_arr[2] = bcmFieldStatNotRedBytes;
                    stat_arr[3] = bcmFieldStatNotRedPackets;
                    nstat = 4;
                    break;
                case BCM_FIELD_COUNTER_MODE_GREEN_NOTGREEN:
                    stat_arr[0] = bcmFieldStatGreenBytes;
                    stat_arr[1] = bcmFieldStatGreenPackets;
                    stat_arr[2] = bcmFieldStatNotGreenBytes;
                    stat_arr[3] = bcmFieldStatNotGreenPackets;
                    nstat = 4;
                    break;
                case BCM_FIELD_COUNTER_MODE_GREEN_RED:
                    stat_arr[0] = bcmFieldStatGreenBytes;
                    stat_arr[1] = bcmFieldStatGreenPackets;
                    stat_arr[2] = bcmFieldStatRedBytes;
                    stat_arr[3] = bcmFieldStatRedPackets;
                    nstat = 4;
                    break;
                case BCM_FIELD_COUNTER_MODE_GREEN_YELLOW:
                    stat_arr[0] = bcmFieldStatGreenBytes;
                    stat_arr[1] = bcmFieldStatGreenPackets;
                    stat_arr[2] = bcmFieldStatYellowBytes;
                    stat_arr[3] = bcmFieldStatYellowPackets;
                    nstat = 4;
                    break;
                case BCM_FIELD_COUNTER_MODE_RED_YELLOW:
                    stat_arr[0] = bcmFieldStatRedBytes;
                    stat_arr[1] = bcmFieldStatRedPackets;
                    stat_arr[2] = bcmFieldStatYellowBytes;
                    stat_arr[3] = bcmFieldStatYellowPackets;
                    nstat = 4;
                    break;
                default:
                    break;
            }
        }
        rv = _field_stat_array_init(unit, f_st, nstat, stat_arr);
        if (BCM_FAILURE(rv)) {
            sal_free(f_st);
            return rv;
        }

        if (fg->flags & _FP_GROUP_SPAN_DOUBLE_SLICE) {
            if (fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
                if ((part == 2) || (part == 3)) {
                    f_ent->flags |= _FP_ENTRY_STAT_IN_SECONDARY_SLICE;
                }
            } else {
                if (part == 1) {
                    f_ent->flags |= _FP_ENTRY_STAT_IN_SECONDARY_SLICE;
                }
            }
        }
        _FP_HASH_INSERT(fc->stat_hash, f_st,
                        (stat_id & _FP_HASH_INDEX_MASK(fc)));
        fc->stat_count++;

        if (stage_id != _BCM_FIELD_STAGE_EXTERNAL) {
            /* Mark as used in the slice */
            if (2 == f_st->hw_entry_count) {
                _FP_COUNTER_BMP_ADD(fs->counter_bmp, 2 * ctr_idx);
                _FP_COUNTER_BMP_ADD(fs->counter_bmp, 2 * ctr_idx + 1);
            } else {
                if (ctr_mode % 2) {
                    _FP_COUNTER_BMP_ADD(fs->counter_bmp, 2 * ctr_idx + 1);
                } else {
                    _FP_COUNTER_BMP_ADD(fs->counter_bmp, 2 * ctr_idx);
                }
            }
        }

        fg->group_status.counter_count++;
    }

    /* Associate the stat object with the entry */
    f_ent_st->flags |= (_FP_ENTRY_STAT_VALID | ent_st_flags);
    f_ent_st->sid = f_st->sid;

    return (BCM_E_NONE);
}


int
_field_trx_actions_recover_action_add(int                unit,
                                      _field_entry_t     *f_ent,
                                      bcm_field_action_t action,
                                      uint32             param0,
                                      uint32             param1,
                                      uint32             param2,
                                      uint32             param3,
                                      uint32             param4,
                                      uint32             param5,
                                      uint32             hw_index
                                      )
{
    int             rv;
    _field_action_t *fa = NULL;

    rv = _field_action_alloc(unit, action,
             param0, param1, param2, param3, param4, param5, &fa);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    fa->hw_index = hw_index;

    fa->flags &= ~_FP_ACTION_DIRTY; /* Mark action as installed. */

    /* Add action to front of entry's linked-list. */
    fa->next = f_ent->actions;
    f_ent->actions  = fa;

    return (BCM_E_NONE);
}
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
/*
 * Function:
 *     _field_flex_counter_recover
 * Purpose:
 *     Recovers flex counter stat entry in warm-boot stage
 *     
 * Parameters:
 *     unit      - (IN) BCM unit number.
 *     f_ent     - (IN) Field entry.
 *     part      - (IN) Field entry part
 *     sid       - (IN) stat id
 *
 * Returns:
 *     BCM_E_XXX
 */
STATIC
int _field_flex_counter_recover(int            unit,
                                _field_entry_t *f_ent,
                                int            part,
                                int            sid)
{
    _field_group_t      *fg=NULL;
    _field_stage_t      *stage_fc=NULL;
    _field_stage_id_t   stage_id;
    _field_control_t    *fc=NULL;
    _field_stat_t       *f_st = NULL;
    _field_entry_stat_t *f_ent_st = NULL;
    int                 rv=0;
    int                 stat_id=0;
    bcm_field_stat_t    stat_arr[2]={bcmFieldStatBytes, bcmFieldStatPackets};
    uint8               nstat = 2;

    fg = f_ent->group;

    /* Get field control and stage control */
    BCM_IF_ERROR_RETURN(_field_control_get(unit, &fc));
    stage_id = f_ent->group->stage_id;
    rv = _field_stage_control_get(unit, stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    rv  = _bcm_field_stat_get(unit, sid, &f_st);
    if (!((rv == BCM_E_NOT_FOUND) || (rv == BCM_E_NONE))) {
        return rv;
    }
    if (rv == BCM_E_NOT_FOUND) {
        BCM_IF_ERROR_RETURN(_field_stat_id_alloc(unit, &stat_id));
        _FP_XGS3_ALLOC(f_st, sizeof (_field_stat_t), "Field stat entity");
        if (NULL == f_st) {
            return (BCM_E_MEMORY);
        }
        f_st->hw_ref_count = 0;
        f_st->sw_ref_count = 1;
        f_st->pool_index = _FP_INVALID_INDEX;
        f_st->hw_index = _FP_INVALID_INDEX;
        f_st->sid = sid;
        f_st->hw_flags = 0; /* INTERNAL */
        f_st->stage_id = fg->stage_id;
        f_st->gid      = fg->gid;
        /* Allocate counters array. */
        if (flex_info.valid == 1) {
            switch(flex_info.flex_stat_map) {
            case 1:
                nstat = 1;
                stat_arr[0] = bcmFieldStatBytes;
                FP_VERB(("_field_flex_counter_recover: StatBytes \n"));
                break;
            case 2:
                nstat = 1;
                stat_arr[0] = bcmFieldStatPackets;
                FP_VERB(("_field_flex_counter_recover: StatPackets \n"));
                break;
            case 3:
                FP_VERB(("_field_flex_counter_recover:StatBytes & Packets\n"));
                break;
            default:
                FP_WARN(("_field_flex_counter_recover:Default Bytes&Pkts.\n"));
                break;
            }
        } else {
            FP_WARN(("_field_flex_counter_recover:   flex info not valid!.\n"));
        }
        rv = _field_stat_array_init(unit, f_st, nstat, stat_arr);
        if (BCM_FAILURE(rv)) {
            sal_free(f_st);
            return (rv);
        }
        f_st->hw_entry_count = 1;
        _FP_HASH_INSERT(fc->stat_hash, f_st, 
                        (sid & _FP_HASH_INDEX_MASK(fc)));
        fc->stat_count++;
    }
    f_ent_st = &f_ent->statistic;
    f_st->hw_ref_count++;
    f_st->sw_ref_count++;
    /* Associate the stat object with the entry */
    f_ent_st->flags |= (_FP_ENTRY_STAT_VALID | _FP_ENTRY_STAT_INSTALLED);
    f_ent_st->sid = f_st->sid;
    return BCM_E_NONE;
}
#endif

int
_field_tr2_actions_recover(int              unit,
                           soc_mem_t        policy_mem,
                           uint32           *policy_entry,
                           _field_entry_t   *f_ent,
                           int              part,
                           bcm_field_stat_t sid,
                           bcm_policer_t    pid
                           )
{
    soc_field_t fld;                    /* Policy table field info */
    uint32 fldval, hw_index, append;
    uint32 param0, param1, param2 = 0, param3 = 0, param4 = 0, param5 = 0;
    uint32 ctr_mode, ctr_idx;           /* Counter mode and index info */
    bcm_field_action_t action;          /* Action type */
    int rv, i, num_fields;              /* Number of fields in policy table info */
    soc_mem_t profile_mem = IFP_REDIRECTION_PROFILEm; /* Redirection profile
                                                       *  table */
    ifp_redirection_profile_entry_t entry_arr[2]; /* Redirection profile entry */
    uint32 *entry_ptr[2];                /* Redirection profile pointer */
    soc_profile_mem_t *redirect_profile; /* Redirect Profile info */
    bcm_pbmp_t pbmp;                     /* Port bitmap info */
    uint8 pkt_int_new = 0;               /* Packet and Internal Priority info */
    bcm_module_t modid;                  /* Modid Info */
    bcm_gport_t  gport;                  /* GPORT info */
    uint32 mirror_pbm;                   /* Mirror bitmap info */
    uint16 bit_pos;                      /* Bit position info*/
    int mtp_index;                       /* Mirror to port index */
    egr_l3_next_hop_entry_t egr_l3_next_hop_entry; /* Egress Next Hop table
                                                    * entry info */
    sal_mac_addr_t mac_addr;             /* MAC address info */
    uint32 mac_addr_words[2];            /* MAC address info for Param[0], Param[1] */
    uint16 vlan_id;                      /* VLAN ID info */
    int8   uc_cosq = -1;                 /* Unicast COS Queue value */
    int8   mc_cosq = -1;                 /* Multicast COS Queue value */
    soc_field_t tmp_fld;                   
#if defined(BCM_TRIDENT2_SUPPORT)
    /* lookup hash selection profile table */
    soc_mem_t hash_select_profile_mem[2] = { VFP_HASH_FIELD_BMAP_TABLE_Am,
                                             VFP_HASH_FIELD_BMAP_TABLE_Bm};
    vfp_hash_field_bmap_table_a_entry_t hash_select_entry_arr[2]; /* hash select
                                                                   * profile
                                                                   * entry*/
    uint32            *hash_select_entry_ptr[2];
    soc_profile_mem_t *hash_select_profile[2];

#endif /* BCM_TRIDENT2_SUPPORT */


    entry_ptr[0] = (uint32 *)entry_arr;
    entry_ptr[1] =  entry_ptr[0] + soc_mem_entry_words(unit, profile_mem);

    /* Reset redirection profile entry. */
    sal_memcpy(entry_ptr[0], soc_mem_entry_null(unit, profile_mem),
               soc_mem_entry_words(unit, profile_mem) * sizeof(uint32));
    sal_memcpy(entry_ptr[1], soc_mem_entry_null(unit, profile_mem),
               soc_mem_entry_words(unit, profile_mem) * sizeof(uint32));

    /* Get the redirect profile */
    rv = _field_trx_redirect_profile_get(unit, &redirect_profile);
    BCM_IF_ERROR_RETURN(rv);

    SOC_PBMP_CLEAR(pbmp);
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIDENT2(unit)) { 
        hash_select_entry_ptr[0] = (uint32 *)hash_select_entry_arr;
        hash_select_entry_ptr[1] =  hash_select_entry_ptr[0] + 
                                    soc_mem_entry_words(unit,
                                    hash_select_profile_mem[0]);

        /* Reset hash select profile entry. */
        sal_memcpy(hash_select_entry_ptr[0], 
                   soc_mem_entry_null(unit, hash_select_profile_mem[0]),
                   soc_mem_entry_words(unit, hash_select_profile_mem[0]) * 
                                       sizeof(uint32));
        sal_memcpy(hash_select_entry_ptr[1], 
                   soc_mem_entry_null(unit, hash_select_profile_mem[0]),
                   soc_mem_entry_words(unit, hash_select_profile_mem[0]) * 
                                       sizeof(uint32));

        /* Get the hash select profile */
        rv = _bcm_field_td2_hash_select_profile_get(unit, 
                                                    hash_select_profile_mem[0],
                                                    &hash_select_profile[0]);
        BCM_IF_ERROR_RETURN(rv);

        rv = _bcm_field_td2_hash_select_profile_get(unit, 
                                                    hash_select_profile_mem[1],
                                                    &hash_select_profile[1]);
        BCM_IF_ERROR_RETURN(rv);
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    num_fields = SOC_MEM_INFO(unit, policy_mem).nFields;

    for (i = 0; i < num_fields; i++) {
        fld = SOC_MEM_INFO(unit, policy_mem).fields[i].field;

#ifdef BCM_TRIUMPH3_SUPPORT
        /* Skip FP_POLICY_TABLE_A/B, PROFILE_SET fields. */
        if (SOC_IS_TRIUMPH3(unit)
            && (soc_mem_field_length(unit, policy_mem, fld) > 32)) {
            continue;
        }
#endif
        fldval = PolicyGet(unit, policy_mem, policy_entry, fld);
        action = param0 = param1 = hw_index = append = 0;
        uc_cosq = mc_cosq = -1;

        switch (fld) {
        case ECMP_HASH_SELf:
            if (fldval != 0) {
                action = bcmFieldActionMultipathHash;
                param0 = fldval;
                append = 1;
            }

            break;

        case GREEN_TO_PIDf:
            if (fldval) {
                f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
            }
            break;
        case ECN_CNGf:
            if (fldval) {
                action = bcmFieldActionEcnNew;
                append = 1;
            }
            break;
        case RP_CHANGE_DSCPf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionRpDscpNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, RP_DSCPf);
                    append = 1;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;
        case YP_CHANGE_DSCPf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionYpDscpNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, YP_DSCPf);
                    append = 1;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;
        case COPY_TO_CPUf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionCopyToCpu;
                    if ((fldval = PolicyGet(unit,
                                            policy_mem,
                                            policy_entry,
                                            VFP_MATCHED_RULEf
                                            )
                         )
                        != 0
                        ) {
                        param0 = 1;
                        param1 = fldval;
                    }

                    append = 1;

                    break;

                case 2:
                    action = bcmFieldActionCopyToCpuCancel;
                    append = 1;

                    break;

                default:
                    break; /* Do nothing */
                }
            break;
        case G_COPY_TO_CPUf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionGpCopyToCpu;
                    param1 = PolicyGet(unit, policy_mem, policy_entry, MATCHED_RULEf);
                    if (param1 != 0) {
                        param0 = 1;
                    }
                    append = 1;
                    break;
                case 2:
                    action = PolicyGet(unit, policy_mem, policy_entry, G_DROPf) == 2
                        ? bcmFieldActionGpTimeStampToCpuCancel
                        : bcmFieldActionGpCopyToCpuCancel;
                    append = 1;
                    break;
                case 3:
                    action = bcmFieldActionGpSwitchToCpuCancel;
                    append = 1;
                    break;
                case 4:
                    action = bcmFieldActionGpSwitchToCpuReinstate;
                    append = 1;
                    break;
                case 5:
                    action = bcmFieldActionGpTimeStampToCpu;
                    param0 = 1;
                    param1 = fldval;
                    append = 1;
                    break;
                case 6:
                    action = bcmFieldActionGpCopyToCpuCancel;
                    rv = _field_trx_actions_recover_action_add(unit,
                                                               f_ent,
                                                               action,
                                                               param0,
                                                               param1,
                                                               param2,
                                                               param3,
                                                               param4,
                                                               param5,
                                                               hw_index);
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }

                    action = bcmFieldActionGpSwitchToCpuCancel;
                    rv = _field_trx_actions_recover_action_add(unit,
                                                               f_ent,
                                                               action,
                                                               param0,
                                                               param1,
                                                               param2,
                                                               param3,
                                                               param4,
                                                               param5,
                                                               hw_index);
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }
                    append = 0;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;
        case Y_COPY_TO_CPUf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionYpCopyToCpu;
                    param1 = PolicyGet(unit, policy_mem, policy_entry, MATCHED_RULEf);
                    if (param1 != 0) {
                        param0 = 1;
                    }
                    append = 1;
                    break;
                case 2:
                    action = PolicyGet(unit, policy_mem, policy_entry, Y_DROPf) == 2
                        ? bcmFieldActionYpTimeStampToCpuCancel
                        : bcmFieldActionYpCopyToCpuCancel;
                    append = 1;
                    break;
                case 3:
                    action = bcmFieldActionYpSwitchToCpuCancel;
                    append = 1;
                    break;
                case 4:
                    action = bcmFieldActionYpSwitchToCpuReinstate;
                    append = 1;
                    break;
                case 5:
                    action = bcmFieldActionYpTimeStampToCpu;
                    param0 = 1;
                    param1 = fldval;
                    append = 1;
                    break;
                case 6:
                    action = bcmFieldActionYpCopyToCpuCancel;
                    rv = _field_trx_actions_recover_action_add(unit,
                                                               f_ent,
                                                               action,
                                                               param0,
                                                               param1,
                                                               param2,
                                                               param3,
                                                               param4,
                                                               param5,
                                                               hw_index);
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }

                    action = bcmFieldActionYpSwitchToCpuCancel;
                    rv = _field_trx_actions_recover_action_add(unit,
                                                               f_ent,
                                                               action,
                                                               param0,
                                                               param1,
                                                               param2,
                                                               param3,
                                                               param4,
                                                               param5,
                                                               hw_index);
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }
                    append = 0;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;
        case R_COPY_TO_CPUf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionRpCopyToCpu;
                    param1 = PolicyGet(unit, policy_mem, policy_entry, MATCHED_RULEf);
                    if (param1 != 0) {
                        param0 = 1;
                    }
                    append = 1;
                    break;
                case 2:
                    action = PolicyGet(unit, policy_mem, policy_entry, R_DROPf) == 2
                        ? bcmFieldActionRpTimeStampToCpuCancel
                        : bcmFieldActionRpCopyToCpuCancel;
                    append = 1;
                    break;
                case 3:
                    action = bcmFieldActionRpSwitchToCpuCancel;
                    append = 1;
                    break;
                case 4:
                    action = bcmFieldActionRpSwitchToCpuReinstate;
                    append = 1;
                    break;
                case 5:
                    action = bcmFieldActionRpTimeStampToCpu;
                    param0 = 1;
                    param1 = fldval;
                    append = 1;
                    break;
                case 6:
                    action = bcmFieldActionRpCopyToCpuCancel;
                    rv = _field_trx_actions_recover_action_add(unit,
                                                               f_ent,
                                                               action,
                                                               param0,
                                                               param1,
                                                               param2,
                                                               param3,
                                                               param4,
                                                               param5,
                                                               hw_index);
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }

                    action = bcmFieldActionRpSwitchToCpuCancel;
                    rv = _field_trx_actions_recover_action_add(unit,
                                                               f_ent,
                                                               action,
                                                               param0,
                                                               param1,
                                                               param2,
                                                               param3,
                                                               param4,
                                                               param5,
                                                               hw_index);
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }
                    append = 0;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;
        case PACKET_REDIRECTIONf:
            switch (fldval) {
                case 1:
                    if (PolicyGet(unit, policy_mem, policy_entry,
                                  REDIRECTIONf) & 0x20) {
                        action = bcmFieldActionRedirectTrunk;
                        param0 = PolicyGet(unit, policy_mem, policy_entry,
                                           REDIRECTIONf) & 0x1f;
                    } else {
                        action = bcmFieldActionRedirect;
                        param0 = PolicyGet(unit, policy_mem, policy_entry,
                                           REDIRECTIONf) >> 6;
                        param1 = PolicyGet(unit, policy_mem, policy_entry,
                                           REDIRECTIONf) & 0x3f;
                    }
                    append = 1;
                    break;
                case 2:
                    action = bcmFieldActionRedirectCancel;
                    append = 1;
                    break;
                case 3:
                    action = bcmFieldActionRedirectPbmp;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, REDIRECTIONf);
                    append = 1;
                    break;
                case 4:
                    action = bcmFieldActionEgressMask;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, REDIRECTIONf);
                    append = 1;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;
        case DROPf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionDrop;
                    append = 1;
                    break;
                case 2:
                    action = bcmFieldActionDropCancel;
                    append = 1;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;

        case R_DROPf:
            switch (fldval) {
            case 1:
                if (f_ent->group->stage_id == _BCM_FIELD_STAGE_INGRESS
                    && PolicyGet(unit,
                                 policy_mem,
                                 policy_entry,
                                 R_COPY_TO_CPUf
                                 )
                    == 5
                    ) {
                    break;
                }

                action = bcmFieldActionRpDrop;
                append = 1;

                break;

            case 2:
                if (f_ent->group->stage_id == _BCM_FIELD_STAGE_INGRESS
                    && PolicyGet(unit,
                                 policy_mem,
                                 policy_entry,
                                 R_COPY_TO_CPUf
                                 )
                    == 2
                    ) {
                    break;
                }

                action = bcmFieldActionRpDropCancel;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case Y_DROPf:
            switch (fldval) {
            case 1:
                if (f_ent->group->stage_id == _BCM_FIELD_STAGE_INGRESS
                    && PolicyGet(unit,
                                 policy_mem,
                                 policy_entry,
                                 Y_COPY_TO_CPUf
                                 )
                    == 5
                    ) {
                    break;
                }

                action = bcmFieldActionYpDrop;
                append = 1;

                break;

            case 2:
                if (f_ent->group->stage_id == _BCM_FIELD_STAGE_INGRESS
                    && PolicyGet(unit,
                                 policy_mem,
                                 policy_entry,
                                 Y_COPY_TO_CPUf
                                 )
                    == 2
                    ) {
                    break;
                }

                action = bcmFieldActionYpDropCancel;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case G_DROPf:
            switch (fldval) {
            case 1:
                if (f_ent->group->stage_id == _BCM_FIELD_STAGE_INGRESS
                    && PolicyGet(unit,
                                 policy_mem,
                                 policy_entry,
                                 G_COPY_TO_CPUf
                                 )
                    == 5
                    ) {
                    break;
                }

                action = bcmFieldActionGpDrop;
                append = 1;

                break;

            case 2:
                if (f_ent->group->stage_id == _BCM_FIELD_STAGE_INGRESS
                    && PolicyGet(unit,
                                 policy_mem,
                                 policy_entry,
                                 G_COPY_TO_CPUf
                                 )
                    == 2
                    ) {
                    break;
                }

                action = bcmFieldActionGpDropCancel;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case MIRROR_OVERRIDEf:
            if (fldval) {
                action = bcmFieldActionMirrorOverride;
                append = 1;
            }

            break;
        case G_L3SW_CHANGE_MACDA_OR_VLANf:
            /* Some chips (e.g. TR2, Apollo) have 2 names for the same field */
            if (soc_mem_field_valid(unit, policy_mem,
                    G_L3SW_CHANGE_MACDA_OR_VLANf)
                && soc_mem_field_valid(unit, policy_mem,
                    G_L3SW_CHANGE_L2_FIELDSf)
                && fld == G_L3SW_CHANGE_MACDA_OR_VLANf) {
                /* Both names valid
                 *   => Skip the second (older) name of this field
                 */
                continue;
            }
            switch (fldval) {
                int ecmp_paths, nh_ecmp;
                case 1:
                    action = bcmFieldActionL3ChangeVlan;
                    if (PolicyGet(unit, policy_mem, policy_entry, ECMPf)) {
                        ecmp_paths = PolicyGet(unit, policy_mem,
                                               policy_entry, ECMP_COUNTf) + 1;
                        nh_ecmp = PolicyGet(unit, policy_mem,
                                            policy_entry, ECMP_PTRf);
                        param0 = _FP_L3_ACTION_PACK_ECMP(nh_ecmp, ecmp_paths);
                    } else {
                        nh_ecmp = PolicyGet(unit, policy_mem,
                                            policy_entry, NEXT_HOP_INDEXf);
                        param0 = _FP_L3_ACTION_PACK_NEXT_HOP(nh_ecmp);
                    }
                    append = 1;

                    if (PolicyGet(unit, policy_mem, policy_entry, ECMPf) == 0) {

                        /* Get the Next Hop Index value */
                        hw_index = PolicyGet(unit, policy_mem, policy_entry,
                                        NEXT_HOP_INDEXf);

                        /* Read Next Hop information into entry */
                        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                                MEM_BLOCK_ANY, hw_index,
                                egr_l3_next_hop_entry.entry_data);
                        if (BCM_FAILURE(rv)) {
                            return (rv);
                        }

                        /* Get the VLAN ID infomation */
                        if (SOC_IS_SC_CQ(unit)) {
                            vlan_id = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm,
                                        egr_l3_next_hop_entry.entry_data,
                                        INTF_NUMf);
                        } else {
                            vlan_id = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm,
                                        egr_l3_next_hop_entry.entry_data,
                                        OVIDf);
                        }

                        /* Re-Create bcmFieldActionOuterVlanNew action for the
                         * FP entry */
                        rv = _field_trx_actions_recover_action_add(unit,
                                f_ent, bcmFieldActionOuterVlanNew, vlan_id, 0,
                                0, 0, 0, 0, hw_index);
                        if (BCM_FAILURE(rv)) {
                            return (rv);
                        }
                    }
                    break;
                case 2:
                    action = bcmFieldActionL3ChangeVlanCancel;
                    append = 1;
                    break;
                case 3:
                    action = bcmFieldActionAddClassTag;
                    nh_ecmp = PolicyGet(unit, policy_mem, policy_entry,
                                NEXT_HOP_INDEXf);
                    param0 = _FP_L3_ACTION_PACK_NEXT_HOP(nh_ecmp);
                    append = 1;
                    break;
                case 4:
                    action = bcmFieldActionL3ChangeMacDa;
                    if (PolicyGet(unit, policy_mem, policy_entry, ECMPf)) {
                        ecmp_paths = PolicyGet(unit, policy_mem,
                                               policy_entry, ECMP_COUNTf) + 1;
                        nh_ecmp = PolicyGet(unit, policy_mem,
                                            policy_entry, ECMP_PTRf);
                        param0 = _FP_L3_ACTION_PACK_ECMP(nh_ecmp, ecmp_paths);
                    } else {
                        nh_ecmp = PolicyGet(unit, policy_mem,
                                            policy_entry, NEXT_HOP_INDEXf);
                        param0 = _FP_L3_ACTION_PACK_NEXT_HOP(nh_ecmp);
                    }
                    append = 1;

                    if (PolicyGet(unit, policy_mem, policy_entry, ECMPf) == 0) {

                        /* Get the Next Hop Index value */
                        hw_index = PolicyGet(unit, policy_mem, policy_entry,
                                        NEXT_HOP_INDEXf);

                        /* Read Next Hop information into entry */
                        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                                MEM_BLOCK_ANY, hw_index,
                                egr_l3_next_hop_entry.entry_data);
                        if (BCM_FAILURE(rv)) {
                            return (rv);
                        }

                        /* Get the DA/DMAC infomation */
                        soc_mem_mac_addr_get(unit, EGR_L3_NEXT_HOPm,
                            egr_l3_next_hop_entry.entry_data,
                            MAC_ADDRESSf, mac_addr);
                        SAL_MAC_ADDR_TO_UINT32(mac_addr, mac_addr_words);

                        /* Re-Create bcmFieldActionDstMacNew action for the
                         * FP entry */
                        rv = _field_trx_actions_recover_action_add(unit,
                                f_ent, bcmFieldActionDstMacNew,
                                mac_addr_words[0], mac_addr_words[1], 0, 0,
                                0, 0, hw_index);
                        if (BCM_FAILURE(rv)) {
                            return (rv);
                        }
                    }
                    break;
                case 5:
                    action = bcmFieldActionL3ChangeMacDaCancel;
                    append = 1;
                    break;
                case 6:
                    action = bcmFieldActionL3Switch;
                    if (PolicyGet(unit, policy_mem, policy_entry, ECMPf)) {
                        ecmp_paths = PolicyGet(unit, policy_mem,
                                               policy_entry, ECMP_COUNTf) + 1;
                        nh_ecmp = PolicyGet(unit, policy_mem,
                                            policy_entry, ECMP_PTRf);
                        param0 = _FP_L3_ACTION_PACK_ECMP(nh_ecmp, ecmp_paths);
                    } else {
                        nh_ecmp = PolicyGet(unit, policy_mem,
                                            policy_entry, NEXT_HOP_INDEXf);
                        param0 = _FP_L3_ACTION_PACK_NEXT_HOP(nh_ecmp);
                    }
                    append = 1;
                    break;
                case 7:
                    action = bcmFieldActionL3SwitchCancel;
                    append = 1;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;

        case G_L3SW_CHANGE_L2_FIELDSf:
            switch (fldval) {
            int class_tag;
            case 1:
                if (!SOC_MEM_FIELD_VALID(unit, policy_mem, ECMPf)
                    || PolicyGet(unit, policy_mem, policy_entry, ECMPf) == 0
                    ) {
                    egr_l3_next_hop_entry_t egr_l3_next_hop_entry;
                    unsigned                if_idx;
                    egr_l3_intf_entry_t     egr_l3_intf_entry;
                    sal_mac_addr_t          mac_addr;
                    uint32                  mac_addr_words[2];

                    /* Fetch L3 next-hop and interface records */

                    hw_index = PolicyGet(unit,
                                         policy_mem,
                                         policy_entry,
                                         NEXT_HOP_INDEXf
                                         );

                    rv = soc_mem_read(unit,
                                      EGR_L3_NEXT_HOPm,
                                      MEM_BLOCK_ANY,
                                      hw_index,
                                      egr_l3_next_hop_entry.entry_data
                                      );

                    if (BCM_FAILURE(rv)) {
                        return (rv);
                    }

                    if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                        if (soc_mem_field32_get(unit,
                                                EGR_L3_NEXT_HOPm,
                                                egr_l3_next_hop_entry.entry_data,
                                                ENTRY_TYPEf
                                                )
                            != 6
                            ) {
                            return (BCM_E_INTERNAL) ;
                        }

                        if_idx = soc_mem_field32_get(
                                     unit,
                                     EGR_L3_NEXT_HOPm,
                                     egr_l3_next_hop_entry.entry_data,
                                     IFP_ACTIONS__INTF_NUMf
                                                     );

                        rv = soc_mem_read(unit,
                                          EGR_L3_INTFm,
                                          MEM_BLOCK_ANY,
                                          if_idx,
                                          egr_l3_intf_entry.entry_data
                                          );

                        if (BCM_FAILURE(rv)) {
                            return (rv);
                        }

                        /* Inform L3 module these interface and next-hop
                           records are in use.
                        */

                        /* L3 module is not initialized yet;
                           L3 to supply function calls for reference count accounting,
                           callable at this point.

                        BCM_L3_INTF_USED_SET(unit, if_idx);
                        _bcm_xgs3_nh_ref_cnt_incr(unit, hw_index);
                        */

                        if (soc_mem_field32_get(
                                unit,
                                EGR_L3_NEXT_HOPm,
                                egr_l3_next_hop_entry.entry_data,
                                IFP_ACTIONS__L3_UC_DA_DISABLEf
                                                )
                            == 0
                            ) {
                            /* Replace MAC DA not disabled
                               => Recover value for new MAC DA
                            */

                            soc_mem_mac_addr_get(unit,
                                                 EGR_L3_NEXT_HOPm,
                                                 egr_l3_next_hop_entry.entry_data,
                                                 IFP_ACTIONS__MAC_ADDRESSf,
                                                 mac_addr
                                                 );

                            SAL_MAC_ADDR_TO_UINT32(mac_addr, mac_addr_words);

                            rv = _field_trx_actions_recover_action_add(
                                     unit,
                                     f_ent,
                                     bcmFieldActionDstMacNew,
                                     mac_addr_words[0],
                                     mac_addr_words[1],
                                     0,
                                     0,
                                     0,
                                     0,
                                     hw_index
                                                                       );
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                        }

                        if (soc_mem_field32_get(
                                unit,
                                EGR_L3_NEXT_HOPm,
                                egr_l3_next_hop_entry.entry_data,
                                IFP_ACTIONS__L3_UC_SA_DISABLEf
                                                )
                            == 0
                            ) {
                            /* Replace MAC SA not disabled
                               => Recover value for new MAC SA
                            */

                            soc_mem_mac_addr_get(unit,
                                                 EGR_L3_INTFm,
                                                 egr_l3_intf_entry.entry_data,
                                                 MAC_ADDRESSf,
                                                 mac_addr
                                                 );

                            SAL_MAC_ADDR_TO_UINT32(mac_addr, mac_addr_words);

                            rv = _field_trx_actions_recover_action_add(
                                     unit,
                                     f_ent,
                                     bcmFieldActionSrcMacNew,
                                     mac_addr_words[0],
                                     mac_addr_words[1],
                                     0,
                                     0,
                                     0,
                                     0,
                                     hw_index
                                                                       );
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                        }

                        if (soc_mem_field32_get(
                                unit,
                                EGR_L3_NEXT_HOPm,
                                egr_l3_next_hop_entry.entry_data,
                                IFP_ACTIONS__L3_UC_VLAN_DISABLEf
                                                )
                            == 0
                            ) {
                            /* Replace outer VID not disabled
                               => Recover value for new outer VID
                            */

                            rv = _field_trx_actions_recover_action_add(
                                     unit,
                                     f_ent,
                                     bcmFieldActionOuterVlanNew,
                                     soc_mem_field32_get(
                                         unit,
                                         EGR_L3_INTFm,
                                         egr_l3_intf_entry.entry_data,
                                         VIDf
                                                         ),
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     hw_index
                                                                       );
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                        }

                        if (SOC_MEM_FIELD_VALID(unit, EGR_L3_NEXT_HOPm, 
                                                IFP_ACTIONS__VNTAG_ACTIONf)) {
                            switch (soc_mem_field32_get(
                                    unit,
                                    EGR_L3_NEXT_HOPm,
                                    egr_l3_next_hop_entry.entry_data,
                                    IFP_ACTIONS__VNTAG_ACTIONf
                                                    )
                                ) {
                            case 1:
                                /* Replace VNTAG
                                   => Recover value for new VNTAG
                                */

                                rv = _field_trx_actions_recover_action_add(
                                         unit,
                                         f_ent,
                                         bcmFieldActionVnTagNew,
                                         soc_mem_field32_get(
                                             unit,
                                             EGR_L3_NEXT_HOPm,
                                             egr_l3_next_hop_entry.entry_data,
                                             IFP_ACTIONS__VNTAGf
                                                         ),
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         hw_index
                                                                       );
                                if (BCM_FAILURE(rv)) {
                                    return (rv);
                                }

                                break;

                            case 3:
                                /* VNTAG delete */

                                rv = _field_trx_actions_recover_action_add(
                                         unit,
                                         f_ent,
                                         bcmFieldActionVnTagDelete,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         hw_index
                                                                       );
                                if (BCM_FAILURE(rv)) {
                                    return (rv);
                                }

                                break;

                            default:
                                ;

                            }
                        }
                    } else {
                        /* Non-Trident */

                        if_idx = soc_mem_field32_get(
                                     unit,
                                     EGR_L3_NEXT_HOPm,
                                     egr_l3_next_hop_entry.entry_data,
                                     L3__INTF_NUMf
                                                     );

                        rv = soc_mem_read(unit,
                                          EGR_L3_INTFm,
                                          MEM_BLOCK_ANY,
                                          if_idx,
                                          egr_l3_intf_entry.entry_data
                                          );

                        if (BCM_FAILURE(rv)) {
                            return (rv);
                        }

                        /* Inform L3 module these interface and next-hop
                           records are in use.
                        */

                        /* L3 module is not initialized yet;
                           L3 to supply function calls for reference count accounting,
                           callable at this point.

                        BCM_L3_INTF_USED_SET(unit, if_idx);
                        _bcm_xgs3_nh_ref_cnt_incr(unit, hw_index);
                        */

                        if (soc_mem_field32_get(
                                unit,
                                EGR_L3_NEXT_HOPm,
                                egr_l3_next_hop_entry.entry_data,
                                L3__L3_UC_DA_DISABLEf
                                                )
                            == 0
                            ) {
                            /* Replace MAC DA not disabled
                               => Recover value for new MAC DA
                            */

                            soc_mem_mac_addr_get(
                                unit,
                                EGR_L3_NEXT_HOPm,
                                egr_l3_next_hop_entry.entry_data,
                                L3__MAC_ADDRESSf,
                                mac_addr
                                                 );

                            SAL_MAC_ADDR_TO_UINT32(mac_addr,
                                                   mac_addr_words
                                                   );

                            rv = _field_trx_actions_recover_action_add(
                                     unit,
                                     f_ent,
                                     bcmFieldActionDstMacNew,
                                     mac_addr_words[0],
                                     mac_addr_words[1],
                                     0,
                                     0,
                                     0,
                                     0,
                                     hw_index
                                                                       );
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                        }

                        if (soc_mem_field32_get(
                                unit,
                                EGR_L3_NEXT_HOPm,
                                egr_l3_next_hop_entry.entry_data,
                                L3__L3_UC_SA_DISABLEf
                                                )
                            == 0
                            ) {
                            /* Replace MAC SA not disabled
                               => Recover value for new MAC SA
                            */

                            soc_mem_mac_addr_get(unit,
                                                 EGR_L3_INTFm,
                                                 egr_l3_intf_entry.entry_data,
                                                 MAC_ADDRESSf,
                                                 mac_addr
                                                 );

                            SAL_MAC_ADDR_TO_UINT32(mac_addr,
                                                   mac_addr_words
                                                   );

                            rv = _field_trx_actions_recover_action_add(
                                     unit,
                                     f_ent,
                                     bcmFieldActionSrcMacNew,
                                     mac_addr_words[0],
                                     mac_addr_words[1],
                                     0,
                                     0,
                                     0,
                                     0,
                                     hw_index
                                                                       );
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                        }

                        if (soc_mem_field32_get(
                                unit,
                                EGR_L3_NEXT_HOPm,
                                egr_l3_next_hop_entry.entry_data,
                                L3__L3_UC_VLAN_DISABLEf
                                                )
                            == 0
                            ) {
                            /* Replace outer VID not disabled
                               => Recover value for new outer VID
                            */

                            rv = _field_trx_actions_recover_action_add(
                                     unit,
                                     f_ent,
                                     bcmFieldActionOuterVlanNew,
                                     soc_mem_field32_get(
                                         unit,
                                         EGR_L3_INTFm,
                                         egr_l3_intf_entry.entry_data,
                                         VIDf
                                                         ),
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     hw_index
                                                                       );
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                        }
                    }
                }

                break;

            case 4:
                action = bcmFieldActionAddClassTag;
                class_tag = PolicyGet(unit, policy_mem, policy_entry,
                                      NEXT_HOP_INDEXf);
                param0 = _FP_L3_ACTION_PACK_NEXT_HOP(class_tag);
                append = 1;
                break;

            default:
                ;
            }

            break;

        case DROP_PRECEDENCEf:
            if (fldval) {
                action = bcmFieldActionDropPrecedence;
                param0 = fldval;
                append = 1;
            }
            break;
        case RP_DROP_PRECEDENCEf:
            if (fldval) {
                action = bcmFieldActionRpDropPrecedence;
                param0 = fldval;
                append = 1;
            }
            break;
        case YP_DROP_PRECEDENCEf:
            if (fldval) {
                action = bcmFieldActionYpDropPrecedence;
                param0 = fldval;
                append = 1;
            }
            break;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
        case FLEX_CTR_BASE_COUNTER_IDXf:
             if (fldval) {
                 BCM_IF_ERROR_RETURN(_field_flex_counter_recover(
                                     unit, f_ent, part, sid));
             }
             break;
#endif
        case COUNTER_MODEf:
            if (fldval) {
                ctr_mode = fldval;
                ctr_idx = PolicyGet(unit, policy_mem, policy_entry,
                                    COUNTER_INDEXf);
                BCM_IF_ERROR_RETURN
                   (_field_tr2_counter_recover(unit,
                                               f_ent,
                                               ctr_mode,
                                               ctr_idx,
                                               part,
                                               sid
                                               )
                    );
            }
            break;

        case EXT_COUNTER_MODEf:
            if (fldval != 0) {
                BCM_IF_ERROR_RETURN
                   (_field_tr2_counter_recover(unit,
                                               f_ent,
                                               fldval,
                                               f_ent->slice_idx,
                                               part,
                                               sid
                                               )
                    );
            }

            break;

#ifdef BCM_TRIUMPH3_SUPPORT
        case COUNTER_SETf:
            if (fldval) {
                BCM_IF_ERROR_RETURN
                    (_bcm_field_tr3_counter_recover(unit,
                                                f_ent,
                                                policy_entry,
                                                part,
                                                sid));
            }
            break;
#endif

        case METER_PAIR_MODEf:
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_field_tr3_meter_recover(unit,
                                              f_ent,
                                              part,
                                              pid,
                                              0,
                                              policy_mem,
                                              policy_entry
                                              )
                    );
            } else
#endif
            {
                BCM_IF_ERROR_RETURN(
                    SOC_IS_SC_CQ(unit)
                    ? _field_sc_cq_meter_recover(unit,
                                                 f_ent,
                                                 part,
                                                 pid,
                                                 policy_entry
                                                 )
                    : _field_trx_meter_recover(unit,
                                               f_ent,
                                               part,
                                               pid,
                                               0,
                                               policy_mem,
                                               policy_entry
                                               )
                    );
            }
            break;

        case METER_SHARING_MODEf:
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_field_tr3_meter_recover(unit,
                                              f_ent,
                                              part,
                                              pid,
                                              1,
                                              policy_mem,
                                              policy_entry
                                              )
                    );
            } else
#endif
            {

                BCM_IF_ERROR_RETURN
                    (_field_trx_meter_recover(unit,
                                              f_ent,
                                              part,
                                              pid,
                                              1,
                                              policy_mem,
                                              policy_entry
                                              )
                    );
            }
            break;

        case CHANGE_CPU_COSf:
            switch (fldval) {
            case 1:         /* Change CPU CoS */
                action = bcmFieldActionCosQCpuNew;
                param0 = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   soc_mem_field_valid(unit, policy_mem, NEW_CPU_COSf)
                                   ? NEW_CPU_COSf : CPU_COSf
                                   );
                append = 1;

                break;

                case 2:
                    action = bcmFieldActionServicePoolIdNew;
                    param0 = ((PolicyGet(unit, policy_mem, policy_entry, CPU_COSf) >> 2) & 0x3);
                    append = 1;
                    break;

            default:
                ;
            }

            break;

        case PID_IPFIX_ACTIONSf:
            switch (fldval) {
                case 1:
                    action = bcmFieldActionIpFix;
                    append = 1;
                    break;
                case 2:
                    action = bcmFieldActionIpFixCancel;
                    append = 1;
                    break;
                default:
                    break; /* Do nothing */
            }
            break;
        case PID_REPLACE_INNER_VIDf:
            if (fldval) {
                action = bcmFieldActionInnerVlanNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, PID_NEW_INNER_VIDf);
                append = 1;
            }
            break;
        case PID_REPLACE_INNER_PRIf:
            if (fldval) {
                action = bcmFieldActionInnerVlanPrioNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, PID_NEW_INNER_PRIf);
                append = 1;
            }
            break;
        case PID_REPLACE_OUTER_VIDf:
            if (fldval) {
                action = bcmFieldActionOuterVlanNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, PID_NEW_OUTER_VIDf);
                append = 1;
            }
            break;
        case PID_REPLACE_OUTER_TPIDf:
            if (fldval) {
                uint32 mode;
                uint16 tpid;
                action = bcmFieldActionOuterTpidNew;
                mode = PolicyGet(unit, policy_mem, policy_entry, PID_OUTER_TPID_INDEXf);
                BCM_IF_ERROR_RETURN
                    (_field_tpid_hw_decode(unit, mode, &tpid));
                param0 = tpid;
                append = 1;
            }
            break;

        case R_CHANGE_DSCPf:
            switch (fldval) {
            case 1:
                action = bcmFieldActionRpDscpNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, R_NEW_DSCPf);

                append = 1;

                break;

            case 2:
                action = bcmFieldActionRpDscpCancel;

                append = 1;

                break;

            default:
                ;
            }

            break;

        case Y_CHANGE_DSCPf:
            switch (fldval) {
            case 1:
                action = bcmFieldActionYpDscpNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, Y_NEW_DSCPf);

                append = 1;

                break;

            case 2:
                action = bcmFieldActionYpDscpCancel;

                append = 1;

                break;

            default:
                ;
            }

            break;

        case G_CHANGE_DSCPf:
            switch (fldval) {
            case 1:
                action = bcmFieldActionGpDscpNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_DSCPf);

                append = 1;

                break;

            case 2:
                action = bcmFieldActionGpDscpCancel;

                append = 1;

                break;

            default:
                ;
            }

            break;

        case G_CHANGE_DSCP_TOSf:
            switch (fldval) {
            case 1:
                action = bcmFieldActionGpTosPrecedenceNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_DSCP_TOSf);

                append = 1;

                break;

            case 2:
                action = bcmFieldActionGpTosPrecedenceCopy;

                append = 1;

                break;

            case 3:
                action = bcmFieldActionGpDscpNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_DSCP_TOSf);

                append = 1;

                break;

            case 4:
                action = bcmFieldActionGpDscpCancel;

                append = 1;

                break;

            default:
                ;
            }

            break;

        case R_REPLACE_INNER_PRIf:
            if (fldval) {
                action = bcmFieldActionRpInnerVlanPrioNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, R_NEW_INNER_PRIf);
                append = 1;
            }
            break;
        case Y_REPLACE_INNER_PRIf:
            if (fldval) {
                action = bcmFieldActionYpInnerVlanPrioNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, Y_NEW_INNER_PRIf);
                append = 1;
            }
            break;
        case G_REPLACE_INNER_PRIf:
            if (fldval) {
                action = bcmFieldActionGpInnerVlanPrioNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_INNER_PRIf);
                append = 1;
            }
            break;
        case R_CHANGE_DOT1Pf:
            if (fldval) {
                action = bcmFieldActionRpOuterVlanPrioNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, R_NEW_DOT1Pf);
                append = 1;
            }
            break;
        case Y_CHANGE_DOT1Pf:
            if (fldval) {
                action = bcmFieldActionYpOuterVlanPrioNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, Y_NEW_DOT1Pf);
                append = 1;
            }
            break;
        case G_CHANGE_DOT1Pf:
            if (fldval) {
                action = bcmFieldActionGpOuterVlanPrioNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_DOT1Pf);
                append = 1;
            }
            break;
        case R_CHANGE_INNER_CFIf:
            if (fldval) {
                action = bcmFieldActionRpInnerVlanCfiNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, R_NEW_INNER_CFIf);
                append = 1;
            }
            break;
        case Y_CHANGE_INNER_CFIf:
            if (fldval) {
                action = bcmFieldActionYpInnerVlanCfiNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, Y_NEW_INNER_CFIf);
                append = 1;
            }
            break;
        case G_CHANGE_INNER_CFIf:
            if (fldval) {
                action = bcmFieldActionGpInnerVlanCfiNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_INNER_CFIf);
                append = 1;
            }
            break;
        case R_CHANGE_OUTER_CFIf:
            if (fldval) {
                action = bcmFieldActionRpOuterVlanCfiNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, R_NEW_OUTER_CFIf);
                append = 1;
            }
            break;
        case Y_CHANGE_OUTER_CFIf:
            if (fldval) {
                action = bcmFieldActionYpOuterVlanCfiNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, Y_NEW_OUTER_CFIf);
                append = 1;
            }
            break;
        case G_CHANGE_OUTER_CFIf:
            if (fldval) {
                action = bcmFieldActionGpOuterVlanCfiNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_OUTER_CFIf);
                append = 1;
            }
            break;
        case PID_COUNTER_MODEf:
            if (fldval) {
                ctr_mode = fldval;
                ctr_idx = PolicyGet(unit, policy_mem, policy_entry,
                                    PID_COUNTER_INDEXf);
                BCM_IF_ERROR_RETURN
                   (_field_tr2_counter_recover(unit,
                                               f_ent,
                                               ctr_mode,
                                               ctr_idx,
                                               part,
                                               sid
                                               )
                    );
            }
            break;
        case OAM_PBBTE_LOOKUP_ENABLEf:
            if (fldval) {
                action = bcmFieldActionOamPbbteLookupEnable;
                param0 = fldval;
                append = 1;
            }
            break;
        case USE_VINTF_CTR_IDXf:
            
            break;
        case CPU_COSf:
            if (fldval) {
                action = bcmFieldActionCosQCpuNew;
                param0 = fldval;
                append = 1;
            }
            break;
        case DO_NOT_LEARNf:
            if (fldval) {
                action = bcmFieldActionDoNotLearn;
                append = 1;
            }
            break;
        case CHANGE_INT_PRIORITYf:
            if (fldval) {
                if (SOC_IS_TD_TT(unit)
                    || SOC_IS_KATANAX(unit)
                    || SOC_IS_TRIUMPH3(unit)) {
                    pkt_int_new = PolicyGet(unit, policy_mem, policy_entry,
                                            CHANGE_OUTER_DOT1Pf);
                } else {
                    pkt_int_new = PolicyGet(unit, policy_mem, policy_entry,
                                            CHANGE_PKT_PRIORITYf);
                }

                if (1 == pkt_int_new) {
                    action = bcmFieldActionPrioPktAndIntNew;
                } else {
                    action = bcmFieldActionPrioIntNew;
                }
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_INT_PRIORITYf);
                append = 1;
            }
            break;
        case CHANGE_PKT_PRIORITYf:
            if (fldval) {
                if (PolicyGet(unit, policy_mem, policy_entry,
                              CHANGE_INT_PRIORITYf) == 1) {
                    action = bcmFieldActionPrioPktAndIntNew;
                } else {
                    action = bcmFieldActionPrioPktNew;
                }
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_PKT_PRIORITYf);
                append = 1;
            }
            break;
        case OUTER_VLAN_ACTIONSf:
            switch (fldval) {
            case 1:
                action = bcmFieldActionOuterVlanAdd;
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_OUTER_VLANf);
                append = 1;
                break;
            case 2:
                action = bcmFieldActionOuterVlanNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_OUTER_VLANf);
                append = 1;
                break;
            case 3:
                action = bcmFieldActionOuterVlanLookup;
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_OUTER_VLANf);
                append = 1;
                break;
            case 4:
                action = bcmFieldActionOuterVlanCopyInner;
                append = 1;
            default:
                break; /* Do nothing */
            }
            break;
        case INNER_VLAN_ACTIONSf:
            switch (fldval) {
            case 1:
                action = bcmFieldActionInnerVlanAdd;
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_INNER_VLANf);
                append = 1;
                break;
            case 2:
                action = bcmFieldActionInnerVlanNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_INNER_VLANf);
                append = 1;
                break;
            case 3:
                action = bcmFieldActionInnerVlanDelete;
                append = 1;
                break;
            case 4:
                action = bcmFieldActionInnerVlanCopyOuter;
                append = 1;
                break;
            default:
                break; /* Do nothing */
            }
            break;
        case CHANGE_CNGf:
            if (fldval) {
                param0 = PolicyGet(unit, policy_mem, policy_entry, NEW_CNGf);
                action = bcmFieldActionDropPrecedence;
                switch (param0) {
                    case 0:
                        param0 = BCM_FIELD_COLOR_GREEN;
                        append = 1;
                        break;
                    case 1:
                        param0 = BCM_FIELD_COLOR_RED;
                        append = 1;
                        break;
                    case 3:
                        param0 = BCM_FIELD_COLOR_YELLOW;
                        append = 1;
                        break;
                    default:
                        break; /* Do nothing */
                }
            }
            break;

        case R_CHANGE_PKT_PRIf:
            switch (fldval) {
            case 4:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   R_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 4
                    ? bcmFieldActionRpPrioPktAndIntCopy
                    : bcmFieldActionRpPrioPktCopy;
                append = 1;

                break;

            case 5:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   R_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 5
                    ? bcmFieldActionRpPrioPktAndIntNew
                    : bcmFieldActionRpPrioPktNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, R_NEW_PKT_PRIf);
                append = 1;

                break;

            case 6:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   R_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 6
                    ? bcmFieldActionRpPrioPktAndIntTos
                    : bcmFieldActionRpPrioPktTos;
                append = 1;

                break;

            case 7:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   R_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 7
                    ? bcmFieldActionRpPrioPktAndIntCancel
                    : bcmFieldActionRpPrioPktCancel;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case R_CHANGE_COS_OR_INT_PRIf:
            switch (fldval) {
            case 1:
                if (SOC_IS_TD_TT(unit)) {
                    /* Recovered as part of G_CHANGE_COS_OR_INT_PRIf */
                    if ((1 == PolicyGet(unit, policy_mem, policy_entry,
                            G_CHANGE_COS_OR_INT_PRIf))
                        && (1 == PolicyGet(unit, policy_mem, policy_entry,
                            Y_CHANGE_COS_OR_INT_PRIf))) {
                        continue;
                    }
                    action = bcmFieldActionRpCosQNew;
                    param0 = PolicyGet(unit,
                                       policy_mem,
                                       policy_entry,
                                       R_COS_INT_PRIf
                                       )
                             & 0xf;
                    append = 1;
                } else {
                    action = bcmFieldActionRpCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, R_COS_INT_PRIf);

                    append = 1;
                }

                break;

            case 2:
                if (SOC_IS_TD_TT(unit)) {
                    action = bcmFieldActionRpCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, R_COS_INT_PRIf) & 0x7f;

                    append = 1;
                } else {
                    action = bcmFieldActionRpVlanCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, R_COS_INT_PRIf) - 8;

                    append = 1;
                }

                break;

            case 4:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              R_CHANGE_PKT_PRIf
                              )
                    != 4
                    ) {
                    action = bcmFieldActionRpPrioIntCopy;

                    append = 1;
                }

                break;

            case 5:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              R_CHANGE_PKT_PRIf
                              )
                    != 5
                    ) {
                    action = bcmFieldActionRpPrioIntNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, R_COS_INT_PRIf);

                    append = 1;
                }

                break;

            case 6:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              R_CHANGE_PKT_PRIf
                              )
                    != 6
                    ) {
                    action = bcmFieldActionRpPrioIntTos;

                    append = 1;
                }

                break;

            case 7:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              R_CHANGE_PKT_PRIf
                              )
                    != 7
                    ) {
                    action = bcmFieldActionRpPrioIntCancel;

                    append = 1;
                }

                break;
            case 8:
                if (SOC_IS_TD_TT(unit)) {
                    if ((8 == PolicyGet(unit, policy_mem, policy_entry,
                            G_CHANGE_COS_OR_INT_PRIf))
                        && (8 == PolicyGet(unit, policy_mem, policy_entry,
                            Y_CHANGE_COS_OR_INT_PRIf))) {
                        /* recovered as part of G_CHANGE_COS_OR_INT_PRIf */
                        continue;
                    } else {
                        action = bcmFieldActionRpUcastCosQNew;
                        param0 = PolicyGet(unit,
                                        policy_mem,
                                        policy_entry,
                                        R_COS_INT_PRIf)
                                 & 0xf;
                        append = 1;
                    }
                } else if (SOC_IS_TRIUMPH3(unit)) {
                    action = bcmFieldActionRpCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, R_COS_INT_PRIf);
                    append = 1;
                }
                break;
            case 9:
                if (SOC_IS_TD_TT(unit)) {
                    if ((9 == PolicyGet(unit, policy_mem, policy_entry,
                            G_CHANGE_COS_OR_INT_PRIf))
                        && (9 == PolicyGet(unit, policy_mem, policy_entry,
                            Y_CHANGE_COS_OR_INT_PRIf))) {
                        /* recovered as part of G_CHANGE_COS_OR_INT_PRIf */
                        continue;
                    } else {
                        action = bcmFieldActionRpMcastCosQNew;
                        param0 = (PolicyGet(unit,
                                        policy_mem,
                                        policy_entry,
                                        R_COS_INT_PRIf) >> 0x4)
                                  & 0xf;
                        append = 1;
                    }
                }
                break;

            default:
                ;
            }

            break;

        case Y_CHANGE_PKT_PRIf:
            switch (fldval) {
            case 4:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   Y_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 4
                    ? bcmFieldActionYpPrioPktAndIntCopy
                    : bcmFieldActionYpPrioPktCopy;
                append = 1;

                break;

            case 5:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   Y_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 5
                    ? bcmFieldActionYpPrioPktAndIntNew
                    : bcmFieldActionYpPrioPktNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, Y_NEW_PKT_PRIf);
                append = 1;

                break;

            case 6:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   Y_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 6
                    ? bcmFieldActionYpPrioPktAndIntTos
                    : bcmFieldActionYpPrioPktTos;
                append = 1;

                break;

            case 7:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   Y_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 7
                    ? bcmFieldActionYpPrioPktAndIntCancel
                    : bcmFieldActionYpPrioPktCancel;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case Y_CHANGE_COS_OR_INT_PRIf:
            switch (fldval) {
            case 1:
                if (SOC_IS_TD_TT(unit)) {
                    /* Recovered as part of G_CHANGE_COS_OR_INT_PRIf */
                    if ((1 == PolicyGet(unit, policy_mem, policy_entry,
                            G_CHANGE_COS_OR_INT_PRIf))
                        && (1 == PolicyGet(unit, policy_mem, policy_entry,
                            R_CHANGE_COS_OR_INT_PRIf))) {
                        continue;
                    }
                    action = bcmFieldActionYpCosQNew;
                    param0 = PolicyGet(unit,
                                       policy_mem,
                                       policy_entry,
                                       Y_COS_INT_PRIf
                                       )
                             & 0xf;

                    append = 1;
                } else {
                    action = bcmFieldActionYpCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, Y_COS_INT_PRIf);

                    append = 1;
                }

                break;

            case 2:
                action = bcmFieldActionYpVlanCosQNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, Y_COS_INT_PRIf) - 8;

                append = 1;

                break;

            case 4:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              Y_CHANGE_PKT_PRIf
                              )
                    != 4
                    ) {
                    action = bcmFieldActionYpPrioIntCopy;

                    append = 1;
                }

                break;

            case 5:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              Y_CHANGE_PKT_PRIf
                              )
                    != 5
                    ) {
                    action = bcmFieldActionYpPrioIntNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, Y_COS_INT_PRIf);

                    append = 1;
                }

                break;

            case 6:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              Y_CHANGE_PKT_PRIf
                              )
                    != 6
                    ) {
                    action = bcmFieldActionYpPrioIntTos;

                    append = 1;
                }

                break;

            case 7:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              Y_CHANGE_PKT_PRIf
                              )
                    != 7
                    ) {
                    action = bcmFieldActionYpPrioIntCancel;

                    append = 1;
                }
        
                break;
            case 8:
                if (SOC_IS_TD_TT(unit)) {
                    if ((8 == PolicyGet(unit, policy_mem, policy_entry,
                            G_CHANGE_COS_OR_INT_PRIf))
                        && (8 == PolicyGet(unit, policy_mem, policy_entry,
                            R_CHANGE_COS_OR_INT_PRIf))) {
                        /* recovered as part of G_CHANGE_COS_OR_INT_PRIf */
                        continue;
                    } else {
                        action = bcmFieldActionYpUcastCosQNew;
                        param0 = PolicyGet(unit,
                                        policy_mem,
                                        policy_entry,
                                        Y_COS_INT_PRIf)
                                 & 0xf;
                        append = 1;
                    }
                } else if (SOC_IS_TRIUMPH3(unit)) {
                    action = bcmFieldActionYpCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, Y_COS_INT_PRIf);
                    append = 1;
                }
                break;
            case 9:
                if (SOC_IS_TD_TT(unit)) {
                    if ((9 == PolicyGet(unit, policy_mem, policy_entry,
                            G_CHANGE_COS_OR_INT_PRIf))
                        && (9 == PolicyGet(unit, policy_mem, policy_entry,
                            R_CHANGE_COS_OR_INT_PRIf))) {
                        /* recovered as part of G_CHANGE_COS_OR_INT_PRIf */
                        continue;
                    } else {
                        action = bcmFieldActionYpMcastCosQNew;
                        param0 = (PolicyGet(unit,
                                        policy_mem,
                                        policy_entry,
                                        Y_COS_INT_PRIf) >> 0x4)
                                  & 0xf;
                        append = 1;
                    }
                }
                break;

            default:
                ;
            }

            break;

        case G_CHANGE_PKT_PRIf:
            switch (fldval) {
            case 4:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   G_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 4
                    ? bcmFieldActionGpPrioPktAndIntCopy
                    : bcmFieldActionGpPrioPktCopy;
                append = 1;

                break;

            case 5:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   G_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 5
                    ? bcmFieldActionGpPrioPktAndIntNew
                    : bcmFieldActionGpPrioPktNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_NEW_PKT_PRIf);
                append = 1;

                break;

            case 6:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   G_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 6
                    ? bcmFieldActionGpPrioPktAndIntTos
                    : bcmFieldActionGpPrioPktTos;
                append = 1;

                break;

            case 7:
                action = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   G_CHANGE_COS_OR_INT_PRIf
                                   )
                    == 7
                    ? bcmFieldActionGpPrioPktAndIntCancel
                    : bcmFieldActionGpPrioPktCancel;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case G_CHANGE_COS_OR_INT_PRIf:
            switch (fldval) {
            case 1:
                if (SOC_IS_TD_TT(unit)) {
                    if ((1 == PolicyGet(unit, policy_mem, policy_entry,
                            Y_CHANGE_COS_OR_INT_PRIf))
                        && (1 == PolicyGet(unit, policy_mem, policy_entry,
                            R_CHANGE_COS_OR_INT_PRIf))) {
                        uc_cosq = PolicyGet(unit,
                                            policy_mem,
                                            policy_entry,
                                            G_COS_INT_PRIf)
                                  & 0xf;
                        mc_cosq = ((PolicyGet(unit,
                                              policy_mem,
                                              policy_entry,
                                              G_COS_INT_PRIf) >> 0x4)
                                    & 0xf);
                        if (uc_cosq == mc_cosq) {
                            /* 
                             * Since queue values are same, cold boot action
                             * was set using bcmFieldActionCosQNew
                             */
                            action = bcmFieldActionCosQNew;
                            param0 = uc_cosq;
                            append = 1;
                        } else {
                            /* 
                             * UC and MC COS queues are different. So, cold
                             * boot config was done using two actions
                             * bcmFieldActionUcastCosQNew and
                             * bcmFieldActionMcastCosQNew
                             */
                            rv = _field_trx_actions_recover_action_add(unit,
                                    f_ent, bcmFieldActionUcastCosQNew, uc_cosq,
                                    0, 0, 0, 0, 0, hw_index);
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                            rv = _field_trx_actions_recover_action_add(unit,
                                    f_ent, bcmFieldActionMcastCosQNew, mc_cosq,
                                    0, 0, 0, 0, 0, hw_index);
                            if (BCM_FAILURE(rv)) {
                                return (rv);
                            }
                        }
                    } else {
                        action = bcmFieldActionGpCosQNew;
                        param0 = PolicyGet(unit,
                                       policy_mem,
                                       policy_entry,
                                       G_COS_INT_PRIf)
                                 & 0xf;
                        append = 1;
                    }
                } else {
                    action = bcmFieldActionGpCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, G_COS_INT_PRIf);
                    append = 1;
                }

                break;

            case 2:
                action = bcmFieldActionGpVlanCosQNew;
                param0 = PolicyGet(unit, policy_mem, policy_entry, G_COS_INT_PRIf) - 8;

                append = 1;

                break;

            case 4:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              G_CHANGE_PKT_PRIf
                              )
                    != 4
                    ) {
                    action = bcmFieldActionGpPrioIntCopy;

                    append = 1;
                }

                break;

            case 5:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              G_CHANGE_PKT_PRIf
                              )
                    != 5
                    ) {
                    action = bcmFieldActionGpPrioIntNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, G_COS_INT_PRIf);

                    append = 1;
                }

                break;

            case 6:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              G_CHANGE_PKT_PRIf
                              )
                    != 6
                    ) {
                    action = bcmFieldActionGpPrioIntTos;

                    append = 1;
                }

                break;

            case 7:
                if (PolicyGet(unit,
                              policy_mem,
                              policy_entry,
                              G_CHANGE_PKT_PRIf
                              )
                    != 7
                    ) {
                    action = bcmFieldActionGpPrioIntCancel;

                    append = 1;
                }

                break;

            case 8:
                if (SOC_IS_TD_TT(unit)) {
                    uc_cosq = PolicyGet(unit,
                                        policy_mem,
                                        policy_entry,
                                        G_COS_INT_PRIf)
                              & 0xf;
                    if ((8 == PolicyGet(unit, policy_mem, policy_entry,
                            Y_CHANGE_COS_OR_INT_PRIf))
                        && (8 == PolicyGet(unit, policy_mem, policy_entry,
                            R_CHANGE_COS_OR_INT_PRIf))) {
                        action = bcmFieldActionUcastCosQNew;
                        param0 = uc_cosq;
                        append = 1;
                    } else {
                        action = bcmFieldActionGpUcastCosQNew;
                        param0 = uc_cosq;
                        append = 1;
                    }
                } else if (SOC_IS_TRIUMPH3(unit)) {
                    action = bcmFieldActionGpCosQNew;
                    param0 = PolicyGet(unit, policy_mem, policy_entry, G_COS_INT_PRIf);
                    append = 1;
                }
                break;

            case 9:
                if (SOC_IS_TD_TT(unit)) {
                    mc_cosq = (PolicyGet(unit,
                                        policy_mem,
                                        policy_entry,
                                        G_COS_INT_PRIf) >> 0x4)
                              & 0xf;
                    if ((9 == PolicyGet(unit, policy_mem, policy_entry,
                            Y_CHANGE_COS_OR_INT_PRIf))
                        && (9 == PolicyGet(unit, policy_mem, policy_entry,
                            R_CHANGE_COS_OR_INT_PRIf))) {
                        action = bcmFieldActionMcastCosQNew;
                        param0 = mc_cosq;
                        append = 1;
                    } else {
                        action = bcmFieldActionGpMcastCosQNew;
                        param0 = mc_cosq;
                        append = 1;
                    }
                }
                break;

            default:
                ;
            }

            break;

        case R_DROP_PRECEDENCEf:
            if (fldval) {
                action = bcmFieldActionRpDropPrecedence;
                param0 = fldval;
                append = 1;
            }

            break;

        case Y_DROP_PRECEDENCEf:
            if (fldval) {
                action = bcmFieldActionYpDropPrecedence;
                param0 = fldval;
                append = 1;
            }

            break;

        case G_DROP_PRECEDENCEf:
            if (fldval) {
                action = bcmFieldActionGpDropPrecedence;
                param0 = fldval;
                append = 1;
            }

            break;

        case USE_VFP_CLASS_ID_Lf:
            if (fldval) {
                action = bcmFieldActionClassDestSet;
                param0 = PolicyGet(unit, policy_mem, policy_entry, VFP_CLASS_ID_Lf);
                append = 1;
            }
            break;
        case USE_VFP_CLASS_ID_Hf:
            if (fldval) {
                action = bcmFieldActionClassSourceSet;
                param0 = PolicyGet(unit, policy_mem, policy_entry, VFP_CLASS_ID_Hf);
                append = 1;
            }
            break;
        case FIELDS_ACTIONf:
        case MPLS_ACTIONf:
            {
                uint32 data = 0;
                if ((fld == MPLS_ACTIONf)
                     && (soc_mem_field_valid(unit, policy_mem, FIELDS_ACTIONf))) {
                    /* Recovery to be done as a part of FIELDS_ACTIONf, if present.
                       If FIELDS_ACTIONf is not present, then recover from 
                       MPLS_ACTIONSf.
                     */
                    continue;
                }
                switch (fldval) {
                    case 1:
                        /* Virtual Port Actions will be recovered as Gport,
                         * even if they were to be configured using respective
                         * actions (like bcmFieldActionIncomingMplsPortSet).
                         */
                        action = bcmFieldActionIngressGportSet;

                        data = PolicyGet(unit, policy_mem, policy_entry, SVPf);
                        /* Virtual port need to be converted back to Gport. */
                        param0 = data;
                        append = 1;
                        break;
                    case 2:
                        action = bcmFieldActionL3IngressSet;
                        data = PolicyGet(unit, policy_mem, policy_entry, L3_IIFf);
                        /* L3_IIF needs to be converted back to Ingress L3 interface. */
                        param0 = data;               
                        append = 1;                  
                        break;
                    case 3:
                        action = bcmFieldActionVrfSet;
                        param0 = PolicyGet(unit, policy_mem, policy_entry, VFP_VRF_IDf);
                        append = 1;
                        break;
                    default:
                        break; /* Do nothing */
                }
            }
            break;
        case DISABLE_VLAN_CHECKSf:
            tmp_fld = (soc_mem_field_valid(unit, policy_mem, FIELDS_ACTIONf)
                     ? FIELDS_ACTIONf : MPLS_ACTIONf);
            if (!soc_mem_field_valid(unit, policy_mem, tmp_fld)
                || PolicyGet(unit, policy_mem, policy_entry, tmp_fld) != 1
                ) {
                if (fldval) {
                    action = bcmFieldActionDoNotCheckVlan;
                    append = 1;
                }
            }
            break;

        case R_CHANGE_ECNf:
            if (fldval != 0) {
                action = bcmFieldActionRpEcnNew;
                param0 = fldval;

                append = 1;
            }

            break;

        case Y_CHANGE_ECNf:
            if (fldval != 0) {
                action = bcmFieldActionYpEcnNew;
                param0 = fldval;

                append = 1;
            }

            break;

        case G_CHANGE_ECNf:
            if (fldval != 0) {
                action = bcmFieldActionGpEcnNew;
                param0 = fldval;

                append = 1;
            }

            break;

        case DO_NOT_CHANGE_TTLf:
            if (fldval) {
                action = bcmFieldActionDoNotChangeTtl;

                append = 1;
            }

            break;

        case DO_NOT_URPFf:
            if (fldval) {
                action = bcmFieldActionDoNotCheckUrpf;

                append = 1;
            }

            break;

           /***** Mirroring recovery, for Triumph / Valkyrie / Enduro *****/

        case INGRESS_MIRRORf:
            if (fldval & (1U << 0)) {
                bcm_module_t modid;
                bcm_gport_t  port;

                hw_index = PolicyGet(unit,
                                     policy_mem,
                                     policy_entry,
                                     soc_mem_field_valid(unit, policy_mem, MTP_INDEX0f)
                                     ? MTP_INDEX0f
                                     : IM0_MTP_INDEXf
                                     );

                /* Translate hw index into (module_num, port_num) */

                _bcm_esw_mirror_mtp_to_modport(unit,
                                               hw_index,
                                               FALSE,
                                               BCM_MIRROR_PORT_INGRESS,
                                               &modid,
                                               &port
                                               );

                rv = _field_trx_actions_recover_action_add(
                         unit,
                         f_ent,
                         bcmFieldActionMirrorIngress,
                         modid,
                         port,
                         0,
                         0,
                         0,
                         0,
                         hw_index
                                                           );

                if (BCM_FAILURE(rv)) {
                    return rv;
                }
            }

            if (fldval & (1U << 1)) {
                bcm_module_t modid;
                bcm_gport_t  port;

                hw_index = PolicyGet(unit,
                                     policy_mem,
                                     policy_entry,
                                     soc_mem_field_valid(unit, policy_mem, MTP_INDEX1f)
                                     ? MTP_INDEX1f
                                     : IM1_MTP_INDEXf
                                     );

                /* Translate hw index into (module_num, port_num) */

                _bcm_esw_mirror_mtp_to_modport(unit,
                                               hw_index,
                                               FALSE,
                                               BCM_MIRROR_PORT_INGRESS,
                                               &modid,
                                               &port
                                               );

                rv = _field_trx_actions_recover_action_add(
                         unit,
                         f_ent,
                         bcmFieldActionMirrorIngress,
                         modid,
                         port,
                         0,
                         0,
                         0,
                         0,
                         hw_index
                                                           );
                if (BCM_FAILURE(rv)) {
                    return rv;
                }
            }

            break;

        case EGRESS_MIRRORf:
            if (fldval & (1U << 0)) {
                bcm_module_t modid;
                bcm_gport_t  port;

                hw_index = PolicyGet(unit,
                                     policy_mem,
                                     policy_entry,
                                     soc_mem_field_valid(unit, policy_mem, MTP_INDEX2f)
                                     ? MTP_INDEX2f
                                     : EM0_MTP_INDEXf
                                     );

                /* Translate hw index into (module_num, port_num) */

                _bcm_esw_mirror_mtp_to_modport(unit,
                                               hw_index,
                                               FALSE,
                                               BCM_MIRROR_PORT_EGRESS,
                                               &modid,
                                               &port
                                               );

                rv = _field_trx_actions_recover_action_add(
                         unit,
                         f_ent,
                         bcmFieldActionMirrorEgress,
                         modid,
                         port,
                         0,
                         0,
                         0,
                         0,
                         hw_index
                                                           );
                if (BCM_FAILURE(rv)) {
                    return rv;
                }
            }

            if (fldval & (1U << 1)) {
                bcm_module_t modid;
                bcm_gport_t  port;

                hw_index = PolicyGet(unit,
                                     policy_mem,
                                     policy_entry,
                                     soc_mem_field_valid(unit, policy_mem, MTP_INDEX3f)
                                     ? MTP_INDEX3f
                                     : EM1_MTP_INDEXf
                                     );

                /* Translate hw index into (module_num, port_num) */

                _bcm_esw_mirror_mtp_to_modport(unit,
                                               hw_index,
                                               FALSE,
                                               BCM_MIRROR_PORT_EGRESS,
                                               &modid,
                                               &port
                                               );

                rv = _field_trx_actions_recover_action_add(
                         unit,
                         f_ent,
                         bcmFieldActionMirrorEgress,
                         modid,
                         port,
                         0,
                         0,
                         0,
                         0,
                         hw_index
                                                           );
                if (BCM_FAILURE(rv)) {
                    return rv;
                }
            }

            break;

            /***** Mirroring recovery, for Triumph2/Apollo *****/

        case MIRRORf:
            {
                static const unsigned mtp_index_field[] = {
                    MTP_INDEX0f, MTP_INDEX1f, MTP_INDEX2f, MTP_INDEX3f
                };

                uint32   ms_reg, mtp_type;
                unsigned b, i;
                bcm_module_t modid;
                bcm_gport_t  port;

                if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
                    /* Enduro/Katana has this field, but as an overlay -- 
                       mirroring for Enduro/Katana is handled above.
                    */

                    break;
                }

                /* Get the mirroring direction control setting */

                BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &ms_reg));
                mtp_type = soc_reg_field_get(unit, MIRROR_SELECTr, ms_reg, MTP_TYPEf);

                /* For each mirroring slot in the policy entry, ... */

                for (b = 1, i = 0; i < COUNTOF(mtp_index_field); ++i, b <<= 1) {
                    if (!(fldval & b)) {
                        /* Slot not in use => Skip */

                        continue;
                    }

                    /* Slot in use => Recover mirror action */

                    hw_index = PolicyGet(unit,
                                         policy_mem,
                                         policy_entry,
                                         mtp_index_field[i]
                                         );

                    /* Translate hw index into (module_num, port_num) */

                    _bcm_esw_mirror_mtp_to_modport(unit,
                                                   hw_index,
                                                   FALSE,
                                                   mtp_type & b
                                                       ? BCM_MIRROR_PORT_EGRESS
                                                       : BCM_MIRROR_PORT_INGRESS,
                                                   &modid,
                                                   &port
                                                   );

                    rv = _field_trx_actions_recover_action_add(
                             unit,
                             f_ent,
                             mtp_type & b
                                 ? bcmFieldActionMirrorEgress
                                 : bcmFieldActionMirrorIngress,
                             modid,
                             port,
                             0,
                             0,
                             0,
                             0,
                             hw_index
                                                               );
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }
                }
            }

            break;

        case G_PACKET_REDIRECTIONf:
            {
                soc_field_t redir_fld = REDIRECTIONf;
                uint32 redir = 0;
#if (defined BCM_TRIDENT_SUPPORT)
                uint32 redir_offset = 0;
#endif
                unsigned sh = SOC_IS_TR_VL(unit)
                    ? ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? 9 : 7)
                    : 8,
                    m = (1 << sh) - 1;

                if (SOC_IS_TRIUMPH3(unit) && 
                    (policy_mem == EFP_POLICY_TABLEm) &&
                    (soc_mem_field_valid(unit, policy_mem, REDIRECTION_DESTINATIONf))) {
                    redir_fld = REDIRECTION_DESTINATIONf;
                }
                redir = PolicyGet(unit,
                                  policy_mem,
                                  policy_entry,
                                  redir_fld
                                  );

#if (defined BCM_TRIDENT_SUPPORT)
                if (SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit)) {
                    redir_offset = 3;
                }
#endif

                switch (fldval) {
                case 1:
                    if (SOC_IS_KATANAX(unit)) {
                        /* 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 */
                        /* RED[17:16] == 0 REDIRECT TO DGLP       */
                           /* DST_T : RED[15]: Trunk Group Bit    */
                           /* DST_MODID  : RED[14:7]              */
                           /* DST_PORTID : RED[ 6:0]              */
                        /* RED[17:16] == 1 REDIRECT TO NHI        */
                           /* NHI : RED[13:0]                     */
                        /* RED[17:16] == 2 REDIRECT TO ECMP GROUP */
                           /* ECMP Group Point : RED[ 9:0]        */
                        /* RED[17:16] == 3 INVALID                */
                        /* Should get constants from regfile!     */
                        switch((redir >> 16) & 0x3) {
                        case 0: /* REDIRECT TO DGLP */ 
                                if ((redir  & 0x8000) == 0x8000) {
                                     action = bcmFieldActionRedirectTrunk;
                                     param0 = redir & 0x7ff;
                                } else {
                                     action = bcmFieldActionRedirect;
                                     param0 = (redir >> 7) & 0xff;
                                     param1 = (redir & 0x7f);
                                }
                                break;
                        case 1: /* REDIRECT TO NHI */
#ifdef INCLUDE_L3
                                action = bcmFieldActionRedirectEgrNextHop;
                                param0 = (redir & 0x3fff) + 
                                         (BCM_XGS3_EGRESS_IDX_MIN);
#endif
                                break;
                        case 2: /* REDIRECT TO ECMP GROUP */
#ifdef INCLUDE_L3
                                action = bcmFieldActionRedirectEgrNextHop;
                                param0 = (redir & 0x3ff) +
                                         (BCM_XGS3_MPATH_EGRESS_IDX_MIN);
#endif
                                break; 
                        case 3:
                        default: /* Wrong */
                                return BCM_E_CONFIG;
                        }
                    } else if (SOC_IS_TD_TT(unit)) {
#ifdef INCLUDE_L3
                        if ((redir >> 17) == 0x3) {
                            action = bcmFieldActionRedirect;
                            param1 = redir & ((1 << 13) - 1);
                            param1 |=  ((SOC_WARM_BOOT_IS_MIM(unit)
                                     || _bcm_vp_used_get(unit, param1, _bcmVpTypeMim)
                                     )
                                    ? _SHR_GPORT_TYPE_MIM_PORT
                                    : _SHR_GPORT_TYPE_MPLS_PORT
                                    ) << _SHR_GPORT_TYPE_SHIFT;
                        } else if ((redir >> 16) == 0x2) {
                            /* Regular Next Hop */
                            action = bcmFieldActionRedirectEgrNextHop;
                            param0 = (redir & ((1 << 16) - 1))
                                            + (BCM_XGS3_EGRESS_IDX_MIN);
                        } else if ((redir >> 16) == 0x3) {
                            /* ECMP Next Hop Group */
                            action = bcmFieldActionRedirectEgrNextHop;
                            param0 = (redir & ((1 << 16) - 1))
                                            + (BCM_XGS3_MPATH_EGRESS_IDX_MIN);
                        }
#endif  /* INCLUDE_L3 */
                    } else if (((redir >> 14) == 0x1)
                               && soc_feature(unit, soc_feature_field_action_redirect_nexthop)) {
#ifdef INCLUDE_L3
                        /* Regular Next Hop */
                        action = bcmFieldActionRedirectEgrNextHop;
                        param0 = (redir & ((1 << 14) - 1))
                                        + (BCM_XGS3_EGRESS_IDX_MIN);
#endif  /* INCLUDE_L3 */
                    } else if (redir & (0x40 << sh)) {
                        action = bcmFieldActionRedirectTrunk;
                        param0 = redir & m;
                    } else {
                        if (SOC_MEM_FIELD_VALID(unit, policy_mem,
                                                HI_PRI_ACTION_CONTROLf)) {
                            action = PolicyGet(unit,
                                               policy_mem,
                                               policy_entry,
                                               HI_PRI_ACTION_CONTROLf
                                               )
                                == 1
                                && PolicyGet(unit,
                                             policy_mem,
                                             policy_entry,
                                             HI_PRI_RESOLVEf
                                             )
                                == 1
                                && PolicyGet(unit,
                                             policy_mem,
                                             policy_entry,
                                             SUPPRESS_COLOR_SENSITIVE_ACTIONSf
                                             )
                                == 1
                                && PolicyGet(unit,
                                             policy_mem,
                                             policy_entry,
                                             DEFER_QOS_MARKINGSf
                                             )
                                == 1
                                && PolicyGet(unit,
                                             policy_mem,
                                             policy_entry,
                                             SUPPRESS_SW_ACTIONSf
                                             )
                                == 1
                                && PolicyGet(unit,
                                             policy_mem,
                                             policy_entry,
                                             SUPPRESS_VXLTf
                                             )
                                == 1
                                ? bcmFieldActionOffloadRedirect :
                                bcmFieldActionRedirect;
                            param0 = redir >> 6;
                            param1 = redir & 0x3f;
                        } else {
                            action = bcmFieldActionRedirect;
                            param0 = redir >> 6;
                            param1 = redir & 0x3f;
                        }
                    }

                    append = 1;

                    break;

                case 2:
                    action = bcmFieldActionRedirectCancel;

                    break;

                case 3:
#if defined(BCM_TRIDENT_SUPPORT) /* BCM_TRIDENT_SUPPORT */
                    if (SOC_IS_KATANAX(unit)) {
                        /* 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0       */
                        /* RED[13:12] == 1 Broadcast Replace PortBitMap      */
                        /*            PortBitMapIndex = RED[11:0]            */
                        /* RED[13:12] == 2 L2MC Replace Port BitMap          */
                        /*            L2MC_Index=PortBitMapIndex = RED[11:0] */
                        /* RED[13:12] == 3 L3MC Replace Port BitMap          */
                        /*            L3MC_Index=PortBitMapIndex = RED[11:0] */
                        /* Should get constants from regfile!     */
                        switch((redir >> 12) & 0x3) {
                        case 1 :  /* Broadcast */
                                  if ((redir >> 11) & 0x01) {
                                      /* RedirectBcastPbmp */
                                      hw_index = (redir & 0x3ff);
                                      action = bcmFieldActionRedirectBcastPbmp;
                                      rv = soc_mem_read(unit, profile_mem, 
                                               MEM_BLOCK_ANY, hw_index, 
                                               entry_ptr[0]);
                                      BCM_IF_ERROR_RETURN(rv);
                                      soc_mem_pbmp_field_get(unit, profile_mem,
                                          entry_ptr[0], BITMAPf, &pbmp);
                                      param0 = SOC_PBMP_WORD_GET(pbmp, 0);
                                      param1 = SOC_PBMP_WORD_GET(pbmp, 1);
                                      param2 = SOC_PBMP_WORD_GET(pbmp, 2);
                                      if (SOC_IS_KATANA2(unit)) {
                                            param3 = SOC_PBMP_WORD_GET(pbmp, 3);
                                            param4 = SOC_PBMP_WORD_GET(pbmp, 4);
                                            param5 = SOC_PBMP_WORD_GET(pbmp, 5);
                                      }
                                      SOC_PROFILE_MEM_REFERENCE(unit, 
                                          redirect_profile, hw_index, 1);
                                      SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, 
                                          redirect_profile, hw_index, 1);
                                 } else {
                                     /* RedirectVlan */
                                     action = bcmFieldActionRedirectVlan;
                                 }
                                 break;
                        case 2 :  /* L2MC */
                                  /* RedirectMcast */
                                  action = bcmFieldActionRedirectMcast;
                                  if (soc_feature(unit,
                                      soc_feature_field_action_redirect_ipmc)) {
                                      param0 = (redir & 0xfff);
                                  } else {
                                      hw_index = (redir & 0xfff);
                                  }
                                  break;
                        case 3 :  /* L3MC */
                                  /* RedirectIpmc */
                                  action = bcmFieldActionRedirectIpmc;
                                  if (soc_feature(unit,
                                      soc_feature_field_action_redirect_ipmc)) {
                                      param0 = (redir & 0xfff);
                                  } else {
                                      hw_index = (redir & 0xfff);
                                  }
                                  break;
                        default  :  
                            /* RedirectPbmp */
                            hw_index = redir;
                            action = bcmFieldActionRedirectPbmp;
                            rv = soc_mem_read(unit, profile_mem, MEM_BLOCK_ANY,
                                              hw_index, entry_ptr[0]);
                            BCM_IF_ERROR_RETURN(rv);
                            soc_mem_pbmp_field_get(unit, profile_mem, entry_ptr[0],
                                                   BITMAPf, &pbmp);
                            param0 = SOC_PBMP_WORD_GET(pbmp, 0);
                            param1 = SOC_PBMP_WORD_GET(pbmp, 1);
                            param2 = SOC_PBMP_WORD_GET(pbmp, 2);
                            if (SOC_IS_KATANA2(unit)) {
                                param3 = SOC_PBMP_WORD_GET(pbmp, 3);
                                param4 = SOC_PBMP_WORD_GET(pbmp, 4);
                                param5 = SOC_PBMP_WORD_GET(pbmp, 5);
                            }
                            SOC_PROFILE_MEM_REFERENCE(unit, redirect_profile,
                                                  hw_index, 1);
                            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, redirect_profile,
                                                        hw_index, 1);
                        }
                    } else if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
                switch ((redir >> (13 + redir_offset)) & 0x03) {
                    case 0x01:
                        if ((redir >> 11) & 0x01) {
                            /* RedirectBcastPbmp */
                            hw_index = (redir & 0x3ff);
                            action = bcmFieldActionRedirectBcastPbmp;
                            rv = soc_mem_read(unit, profile_mem, MEM_BLOCK_ANY,
                                    hw_index, entry_ptr[0]);
                            BCM_IF_ERROR_RETURN(rv);
                            soc_mem_pbmp_field_get(unit, profile_mem, entry_ptr[0],
                                BITMAPf, &pbmp);
                            param0 = SOC_PBMP_WORD_GET(pbmp, 0);
                            param1 = SOC_PBMP_WORD_GET(pbmp, 1);
                            param2 = SOC_PBMP_WORD_GET(pbmp, 2);
                            if (SOC_IS_KATANA2(unit)) {
                                param3 = SOC_PBMP_WORD_GET(pbmp, 3);
                                param4 = SOC_PBMP_WORD_GET(pbmp, 4);
                                param5 = SOC_PBMP_WORD_GET(pbmp, 5);
                            }

                            if (SOC_IS_TD2_TT2(unit)) {
                                param3 = SOC_PBMP_WORD_GET(pbmp, 3);
                            }
                            SOC_PROFILE_MEM_REFERENCE(unit, redirect_profile,
                                                      hw_index, 1);
                            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, redirect_profile,
                                                            hw_index, 1);
                            break;
                        } else {
                            /* RedirectVlan */
                            action = bcmFieldActionRedirectVlan;
                            break;
                        }
                        break;
                    case 0x02:
                        /* RedirectMcast */
                        action = bcmFieldActionRedirectMcast;
                        if (soc_feature(unit,
                                        soc_feature_field_action_redirect_ipmc)) {
                            param0 = (redir & 0x1fff);
                        } else {
                            hw_index = (redir & 0x1fff);
                        }
                        break;
                    case 0x03:
                        /* RedirectIpmc */
                        action = bcmFieldActionRedirectIpmc;
                        if (soc_feature(unit,
                                        soc_feature_field_action_redirect_ipmc)) {
                            param0 = (redir & 0xfff);
                        } else {
                            hw_index = (redir & 0xfff);
                        }
                        break;
                    default:
                        /* RedirectPbmp */
                        hw_index = redir;
                        action = bcmFieldActionRedirectPbmp;
                        rv = soc_mem_read(unit, profile_mem, MEM_BLOCK_ANY,
                                hw_index, entry_ptr[0]);
                        BCM_IF_ERROR_RETURN(rv);
                        soc_mem_pbmp_field_get(unit, profile_mem, entry_ptr[0],
                            BITMAPf, &pbmp);
                        param0 = SOC_PBMP_WORD_GET(pbmp, 0);
                        param1 = SOC_PBMP_WORD_GET(pbmp, 1);
                        param2 = SOC_PBMP_WORD_GET(pbmp, 2);
                        if (SOC_IS_KATANA2(unit)) {
                            param3 = SOC_PBMP_WORD_GET(pbmp, 3);
                            param4 = SOC_PBMP_WORD_GET(pbmp, 4);
                            param5 = SOC_PBMP_WORD_GET(pbmp, 5);
                        }

                        if (SOC_IS_TD2_TT2(unit)) {
                            param3 = SOC_PBMP_WORD_GET(pbmp, 3);
                        }
                        SOC_PROFILE_MEM_REFERENCE(unit, redirect_profile,
                                                  hw_index, 1);
                        SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, redirect_profile,
                                                        hw_index, 1);
                }
            } else
#endif /* !BCM_TRIDENT_SUPPORT */
            {
                        switch ((redir >> 11) & 0x07) {
                    case 0x02:
                        action = bcmFieldActionRedirectVlan;
                        break;
                    case 0x03:
                        hw_index = (redir & 0x3ff);
                        action = bcmFieldActionRedirectBcastPbmp;
                        rv = soc_mem_read(unit, profile_mem, MEM_BLOCK_ANY,
                                hw_index, entry_ptr[0]);
                        BCM_IF_ERROR_RETURN(rv);
                        soc_mem_pbmp_field_get(unit, profile_mem, entry_ptr[0],
                                BITMAPf, &pbmp);
                        param0 = SOC_PBMP_WORD_GET(pbmp, 0);
                        param1 = SOC_PBMP_WORD_GET(pbmp, 1);
                        SOC_PROFILE_MEM_REFERENCE(unit, redirect_profile,
                                                  hw_index, 1);
                        SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, redirect_profile,
                                                        hw_index, 1);
                        break;
                    case 0x04:
                        action = bcmFieldActionRedirectMcast;

                        if (soc_feature(unit,
                                        soc_feature_field_action_redirect_ipmc)) {
                            param0 = redir;
                        } else {
                            hw_index = redir;
                        }
                        break;
                    case 0x06:
                        action = bcmFieldActionRedirectIpmc;
                        if (soc_feature(unit,
                                        soc_feature_field_action_redirect_ipmc)) {
                            param0 = redir;
                        } else {
                            hw_index = redir;
                        }
                        break;
                    default:
                        hw_index = redir;
                        action = bcmFieldActionRedirectPbmp;
                        rv = soc_mem_read(unit, profile_mem, MEM_BLOCK_ANY,
                                hw_index, entry_ptr[0]);
                        BCM_IF_ERROR_RETURN(rv);
                        soc_mem_pbmp_field_get(unit, profile_mem, entry_ptr[0],
                            BITMAPf, &pbmp);
                        param0 = SOC_PBMP_WORD_GET(pbmp, 0);
                        param1 = SOC_PBMP_WORD_GET(pbmp, 1);
                        SOC_PROFILE_MEM_REFERENCE(unit, redirect_profile,
                                                  hw_index, 1);
                        SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, redirect_profile,
                                                        hw_index, 1);
                }
            }
            append = 1;
                    break;
                case 4:
                    if (_BCM_FIELD_STAGE_EGRESS == f_ent->group->stage_id) {
                        mirror_pbm = PolicyGet(unit, policy_mem,
                                           policy_entry, redir_fld);
                        for (bit_pos = 1, mtp_index = 0;
                                mtp_index < BCM_MIRROR_MTP_COUNT;
                                ++mtp_index, bit_pos <<= 1) {
                            if (!(mirror_pbm & bit_pos)) {
                                /* Slot not in use => Skip */
                                continue;
                            }
                            /* Translate hw index into (module_num, port_num) */
                            _bcm_esw_mirror_mtp_to_modport(unit, mtp_index,
                                TRUE, BCM_MIRROR_PORT_EGRESS_TRUE, &modid,
                                &gport);
                            rv = _field_trx_actions_recover_action_add(unit,
                                    f_ent, bcmFieldActionMirrorEgress, -1,
                                    gport, 0, 0, 0, 0, mtp_index);
                            if (BCM_FAILURE(rv)) {
                                return rv;
                            }
                        }
                    } else {
                        action   = bcmFieldActionEgressMask;
                        hw_index = redir;
                        rv = soc_mem_read(unit, profile_mem, MEM_BLOCK_ANY,
                                          hw_index, entry_ptr[0]);
                        BCM_IF_ERROR_RETURN(rv);
                        soc_mem_pbmp_field_get(unit, profile_mem, entry_ptr[0],
                                               BITMAPf, &pbmp);
                        param0 = SOC_PBMP_WORD_GET(pbmp, 0);
                        param1 = SOC_PBMP_WORD_GET(pbmp, 1);
                        if (SOC_IS_KATANA2(unit)) {
                            param2 = SOC_PBMP_WORD_GET(pbmp, 2);
                            param3 = SOC_PBMP_WORD_GET(pbmp, 3);
                            param4 = SOC_PBMP_WORD_GET(pbmp, 4);
                            param5 = SOC_PBMP_WORD_GET(pbmp, 5);
                        }

                        SOC_PROFILE_MEM_REFERENCE(unit, redirect_profile,
                            hw_index, 1);
                        SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, redirect_profile,
                            hw_index, 1);
                        append = 1;
                    }
                    break;

                case 5:
                    action   = bcmFieldActionEgressPortsAdd;
                    hw_index = redir;

                    append = 1;

                    break;

                default:
                    ;
                }
            }

            break;

        case PPD3_CLASS_TAGf:
            if (fldval != 0) {
                action = bcmFieldActionOffloadClassSet;
                param0 = fldval;
                append = 1;
            }

            break;

            /* Additions for Enduro */

        case OAM_LMEP_ENf:
            if (fldval) {
                action = bcmFieldActionOamLmepEnable;
                param0 = fldval;
                append = 1;
            }

            break;

        case OAM_LM_ENf:
            if (fldval) {
                action = bcmFieldActionOamLmEnable;
                param0 = fldval;
                append = 1;
            }

            break;

        case OAM_DM_ENf:
            if (fldval) {
                action = bcmFieldActionOamDmEnable;
                param0 = fldval;
                append = 1;
            }

            break;

        case OAM_LM_BASE_PTRf:
            if (fldval != 0) {
                action = bcmFieldActionOamLmBasePtr;
                param0 = fldval;
                append = 1;
            }

            break;

        case OAM_SERVICE_PRI_MAPPING_PTRf:
            if (fldval != 0) {
                action = bcmFieldActionOamServicePriMappingPtr;
                param0 = fldval;
                append = 1;
            }

            break;

        case OAM_LMEP_MDLf:
            if (fldval != 0) {
                action = bcmFieldActionOamLmepMdl;
                param0 = fldval;
                append = 1;
            }

            break;

        case OAM_TXf:
            if (fldval) {
                action = bcmFieldActionOamTx;
                param0 = fldval;
                append = 1;
            }

            break;

        case OAM_UP_MEPf:
            if (fldval) {
                action = bcmFieldActionOamUpMep;
                param0 = fldval;
                append = 1;
            }

            break;

        case DO_NOT_GENERATE_CNMf:
            if (fldval) {
                action = bcmFieldActionCnmCancel;
                append = 1;
            }

            break;

        case DISABLE_DYNAMIC_LOAD_BALANCINGf:
        case HGT_DLB_DISABLEf:
            if (fldval) {
                action = bcmFieldActionDynamicHgTrunkCancel;
                append = 1;
#if (defined(BCM_TRIDENT_SUPPORT) && defined(BCM_TRIUMPH3_SUPPORT)) || \
    (defined(BCM_TRIDENT_SUPPORT) && defined(BCM_TRIDENT2_SUPPORT))
                if (SOC_IS_TRIDENT2(unit)) {
                    if (PolicyGet(unit, policy_mem, policy_entry, 
                        LAG_RH_DISABLEf) && PolicyGet(unit, policy_mem, 
                        policy_entry, HGT_RH_DISABLEf)) {
                        action = bcmFieldActionTrunkLoadBalanceCancel;
                    }
                } else if (SOC_IS_TRIUMPH3(unit)) {
                    if (PolicyGet(unit, policy_mem, policy_entry,
                                                LAG_DLB_DISABLEf)) {
                        action = bcmFieldActionTrunkLoadBalanceCancel;
                    }
                } 
#endif
            }

            break;

        case CHANGE_OUTER_DOT1Pf:
            switch (fldval) {
            case 1:
                action
                    = PolicyGet(unit, policy_mem, policy_entry,
                        CHANGE_INT_PRIORITYf)
                        ? bcmFieldActionPrioPktAndIntNew
                        : bcmFieldActionPrioPktNew;
                if (bcmFieldActionPrioPktAndIntNew == action) {
                    /* 
                     * PrioPktAndIntNew action is recovered by
                     * CHANGE_INT_PRIORITYf
                     */
                    continue;
                }
                param0 = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   NEW_OUTER_DOT1Pf
                                   );

                append = 1;

                break;

            case 3:
                action = bcmFieldActionOuterVlanPrioCopyInner;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case CHANGE_OUTER_CFIf:
            switch (fldval) {
            case 2:
                action = bcmFieldActionOuterVlanCfiNew;
                param0 = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   NEW_OUTER_CFIf
                                   );

                append = 1;

                break;
            case 3:
                action = bcmFieldActionOuterVlanCfiCopyInner;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case CHANGE_INNER_DOT1Pf:
            switch (fldval) {
            case 2:
                /* No corresponding cold config (?) */

                break;

            case 3:
                action = bcmFieldActionInnerVlanPrioCopyOuter;
                append = 1;

                break;

            default:
                ;
            }

            break;

        case CHANGE_INNER_CFIf:
            switch (fldval) {
            case 2:
                action = bcmFieldActionInnerVlanCfiNew;
                param0 = PolicyGet(unit,
                                   policy_mem,
                                   policy_entry,
                                   NEW_INNER_CFIf
                                   );
                append = 1;
                break;

            case 3:
                action = bcmFieldActionInnerVlanCfiCopyOuter;
                append = 1;

                break;

            default:
                ;
            }

            break;

    case IPV6_TO_IPV4_MAP_SIP_VALIDf:
        if (fldval != 0) {
            action = bcmFieldActionCompressSrcIp6;
            param0 = PolicyGet(unit,
                               policy_mem,
                               policy_entry,
                               IPV6_TO_IPV4_MAP_OFFSET_SETf);
            append = 1;
        }
        break;

    case IPV6_TO_IPV4_MAP_DIP_VALIDf:
        if (fldval != 0) {
            action = bcmFieldActionCompressDstIp6;
            param0 = PolicyGet(unit,
                               policy_mem,
                               policy_entry,
                               IPV6_TO_IPV4_MAP_OFFSET_SETf); 
            append = 1;
        }
        break;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    case USE_SVC_METER_COLORf:
        if (fldval != 0) {
            action = bcmFieldActionUseGlobalMeterColor;
            append = 1;
        }
        break;
#endif 
        case USE_VFP_VRF_IDf:   
            if (fldval) {
                action = bcmFieldActionVrfSet;
                param0 = PolicyGet(unit, policy_mem, policy_entry, VFP_VRF_IDf);
                append = 1;
            }
            break;

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    case OAM_KEY1f:
        if (fldval != 0) {
            action = bcmFieldActionOamDomain;
            param0 = PolicyGet(unit, policy_mem, policy_entry, OAM_KEY1f);
            param1 = PolicyGet(unit, policy_mem, policy_entry, OAM_KEY2f);
            append = 1;
        }
        break;

    case OLP_HDR_ADDf:
        if (fldval != 0) {
            action = bcmFieldActionOamOlpHeaderAdd;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               OLP_HDR_TYPE_COMPRESSEDf);
            append = 1;
        }
        break;

    case OAM_SESSION_IDf:
        if (fldval != 0) {
            action = bcmFieldActionOamSessionId;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               OAM_SESSION_IDf);
            append = 1;
        }
        break;
#endif 
#if defined(BCM_TRIDENT2_SUPPORT)
    case FCOE_SRC_BIND_CHECK_ENABLEf:
        if (fldval != 0) {
            action = bcmFieldActionFibreChanSrcBindEnable;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               FCOE_SRC_BIND_CHECK_ENABLEf);
            append = 1;
        }
        break;
    case FCOE_SRC_FPMA_PREFIX_CHECK_ENABLEf:
        if (fldval != 0) {
            action = bcmFieldActionFibreChanFpmaPrefixCheckEnable;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               FCOE_SRC_FPMA_PREFIX_CHECK_ENABLEf);
            append = 1;
        }
        break;
    case FCOE_ZONE_CHECK_ENABLEf:
        if (fldval != 0) {
            action = bcmFieldActionFibreChanZoneCheckEnable;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               FCOE_ZONE_CHECK_ENABLEf);
            append = 1;
        }
        break;
    case FCOE_VSAN_IDf:
        if (fldval != 0) {
            action = bcmFieldActionFibreChanVsanId;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               FCOE_VSAN_IDf);
            append = 1;
        }
        break;
    case FCOE_ZONE_CHECK_ACTIONf:
        if (fldval != 0) {
            action = bcmFieldActionFibreChanZoneCheckActionCancel;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               FCOE_ZONE_CHECK_ACTIONf);
            append = 1;
        }
        break;
    case FCOE_VSAN_PRI_VALIDf:
        if (fldval != 0) {
            action = bcmFieldActionFibreChanIntVsanPri;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               FCOE_VSAN_PRIf);
            append = 1;
        }
        break;
   case DO_NOT_NATf:
        if (fldval != 0) {
            action = bcmFieldActionNatCancel;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               DO_NOT_NATf);
            append = 1;
        }
        break;
   case NAT_ENABLEf:
        if (fldval != 0) {
            action = bcmFieldActionNat;
            param0 = PolicyGet(unit, policy_mem, policy_entry, 
                               NAT_ENABLEf);
            append = 1;
        }
        break;
#ifdef INCLUDE_L3 
   case NAT_PACKET_EDIT_IDXf:
        if (fldval != 0) {
            action = bcmFieldActionNatEgressOverride;
            param0 = BCM_L3_NAT_EGRESS_SW_IDX_GET(PolicyGet(unit, policy_mem,
                                         policy_entry, NAT_PACKET_EDIT_IDXf),
                                    PolicyGet(unit, policy_mem, policy_entry,
                                                NAT_PACKET_EDIT_ENTRY_SELf));
            append = 1;
        }
        break;
   case NAT_PACKET_EDIT_ENTRY_SELf:
        /* The param value is updated in NAT_PACKET_EDIT_IDXf */
        break; 
#endif /* INCLUDE_L3 */
   case SFLOW_ING_SAMPLEf:
        if (fldval != 0) {
            action = bcmFieldActionIngSampleEnable;
            append = 1;
        }
        break;
   case SFLOW_EGR_SAMPLEf:
        if (fldval != 0) {
            action = bcmFieldActionEgrSampleEnable;
            append = 1;
        }
        break;
    case HASH_FIELD_BITMAP_PTR_Af:
        if (SOC_IS_TRIDENT2(unit)) {
            hw_index = PolicyGet(unit, policy_mem, policy_entry, 
                                 HASH_FIELD_BITMAP_PTR_Af);
            action = bcmFieldActionHashSelect0;
            rv = soc_mem_read(unit, hash_select_profile_mem[0], 
                              MEM_BLOCK_ANY, hw_index, 
                              entry_ptr[0]);
            BCM_IF_ERROR_RETURN(rv);
            param0 = PolicyGet(unit, hash_select_profile_mem[0], 
                               entry_ptr[0], BITMAPf);
            if (param0) {
                append = 1;
            }
        }
        break;
    case HASH_FIELD_BITMAP_PTR_Bf:
        if (SOC_IS_TRIDENT2(unit)) {
            hw_index = PolicyGet(unit, policy_mem, policy_entry, 
                                 HASH_FIELD_BITMAP_PTR_Bf);
            action = bcmFieldActionHashSelect1;
            rv = soc_mem_read(unit, hash_select_profile_mem[1], 
                              MEM_BLOCK_ANY, hw_index, 
                              entry_ptr[0]);
            BCM_IF_ERROR_RETURN(rv);
            param0 = PolicyGet(unit, hash_select_profile_mem[1], 
                               entry_ptr[0], BITMAPf);
            if (param0) {
                append = 1;
            }
        }
        break;
    case LAG_RH_DISABLEf:
        if (fldval != 0) {
            action = bcmFieldActionTrunkResilientHashCancel; 
            if (SOC_IS_TRIDENT2(unit)) {
                if (PolicyGet(unit, policy_mem, policy_entry,
                       HGT_DLB_DISABLEf) && PolicyGet(unit, policy_mem,
                        policy_entry, HGT_RH_DISABLEf)) {
                    action = bcmFieldActionTrunkLoadBalanceCancel;
                }
            }
            append = 1;
        }
        break;
    case HGT_RH_DISABLEf:
        if (fldval != 0) {
            action = bcmFieldActionHgTrunkResilientHashCancel;
            if (SOC_IS_TRIDENT2(unit)) {
                if (PolicyGet(unit, policy_mem, policy_entry,
                       HGT_DLB_DISABLEf) && PolicyGet(unit, policy_mem,
                        policy_entry, LAG_RH_DISABLEf)) {
                    action = bcmFieldActionTrunkLoadBalanceCancel;
                }
            }
            append = 1;
        }
        break;
    case ECMP_RH_DISABLEf:
        if (fldval != 0) {
            action = bcmFieldActionEcmpResilientHashCancel;
            append = 1;
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */

        default:
            break; /* Do nothing */
        }


        if (append) {
            rv = _field_trx_actions_recover_action_add(unit,
                                                       f_ent,
                                                       action,
                                                       param0,
                                                       param1,
                                                       param2,
                                                       param3,
                                                       param4,
                                                       param5,
                                                       hw_index
                                                       );
            if (BCM_FAILURE(rv)) {
                return rv;
            }
        }
    }

    return BCM_E_NONE;
}

STATIC int
_field_src_dst_entity_compare(_bcm_field_fwd_entity_sel_t entity1,
                              _bcm_field_fwd_entity_sel_t entity2)
{
    switch (entity1) {
    case _bcmFieldFwdEntityMplsGport:
    case _bcmFieldFwdEntityMimGport:
    case _bcmFieldFwdEntityWlanGport:
        if ((entity2 == _bcmFieldFwdEntityMplsGport) ||
            (entity2 == _bcmFieldFwdEntityMimGport) ||
            (entity2 == _bcmFieldFwdEntityWlanGport)) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;
    default:
        if (entity1 != entity2) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}


int
_field_tr2_slice_key_control_entry_recover(int      unit,
                                           unsigned slice_num,
                                           _field_sel_t *sel
                                           )
{
    int                          rv;
    fp_slice_key_control_entry_t buf;

    sel->src_class_sel
        = sel->dst_class_sel
        = sel->intf_class_sel
        = sel->aux_tag_1_sel
        = sel->aux_tag_2_sel
        = _FP_SELCODE_DONT_CARE;

    if (!SOC_MEM_IS_VALID(unit, FP_SLICE_KEY_CONTROLm)) {
        return (BCM_E_NONE);
    }

    rv = soc_mem_read(unit,
                      FP_SLICE_KEY_CONTROLm,
                      MEM_BLOCK_ALL,
                      0,
                      buf.entry_data
                      );
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    /* Get source class select field. */
    sel->src_class_sel = soc_mem_field32_get(unit,
                                             FP_SLICE_KEY_CONTROLm,
                                             buf.entry_data,
                                             _trx_src_class_id_sel[slice_num]
                                             );

    /* Get destination class select field. */
    sel->dst_class_sel = soc_mem_field32_get(unit,
                                             FP_SLICE_KEY_CONTROLm,
                                             buf.entry_data,
                                             _trx_dst_class_id_sel[slice_num]
                                             );

    /* Get interface class select field. */
    sel->intf_class_sel = soc_mem_field32_get(
                              unit,
                              FP_SLICE_KEY_CONTROLm,
                              buf.entry_data,
                              _trx_interface_class_id_sel[slice_num]
                                              );

    sel->aux_tag_1_sel = soc_mem_field32_get_def(
                             unit,
                             FP_SLICE_KEY_CONTROLm,
                             buf.entry_data,
                             _bcm_trx2_aux_tag_1_field[slice_num],
                             _FP_SELCODE_DONT_CARE
                                                 );

    sel->aux_tag_2_sel = soc_mem_field32_get_def(
                             unit,
                             FP_SLICE_KEY_CONTROLm,
                             buf.entry_data,
                             _bcm_trx2_aux_tag_2_field[slice_num],
                             _FP_SELCODE_DONT_CARE
                                                 );

    return (BCM_E_NONE);
}

/* Allocate and initialize a group structure */

int
_field_tr2_group_construct_alloc(int unit, _field_group_t **pfg)
{
    _field_group_t *fg = 0;
    unsigned       i;

    _FP_XGS3_ALLOC(fg, sizeof(*fg), "field group");
    if (fg == 0) {
        return BCM_E_MEMORY;
    }

    for (i = 0; i < COUNTOF(fg->sel_codes); ++i) {
        _FIELD_SELCODE_CLEAR(fg->sel_codes[i]);
        fg->sel_codes[i].intraslice = _FP_SELCODE_DONT_USE;
    }
    _bcm_field_group_status_init(unit, &fg->group_status);

    *pfg = fg;

    return (BCM_E_NONE);
}

/* Add the given qualifier to the given group */

STATIC int
_field_tr2_group_qual_add(_bcm_field_group_qual_t *grp_qual,
                          unsigned                 qid,
                          _bcm_field_qual_offset_t *qual_offset
                          )
{
    uint16                   *new_qid_arr;
    _bcm_field_qual_offset_t *new_qoffset_arr;
    unsigned                 new_size;

    new_size = grp_qual->size + 1;

    new_qid_arr = (uint16 *) sal_alloc(new_size * sizeof(grp_qual->qid_arr[0]),
                                       "Group qual id"
                                       );
    if (new_qid_arr == 0) {
        return (BCM_E_MEMORY);
    }

    new_qoffset_arr = (_bcm_field_qual_offset_t *)
        sal_alloc(new_size * sizeof(grp_qual->offset_arr[0]),
                  "Group qual offset");
    if (new_qoffset_arr == 0) {
        sal_free(new_qid_arr);

        return (BCM_E_MEMORY);
    }

    if (grp_qual->size != 0) {
        sal_memcpy(new_qid_arr,
                   grp_qual->qid_arr,
                   grp_qual->size * sizeof(grp_qual->qid_arr[0])
                   );
        sal_memcpy(new_qoffset_arr,
                   grp_qual->offset_arr,
                   grp_qual->size * sizeof(grp_qual->offset_arr[0])
                   );

        sal_free(grp_qual->qid_arr);
        sal_free(grp_qual->offset_arr);
    }

    new_qid_arr[new_size - 1]     = qid;
    new_qoffset_arr[new_size - 1] = *qual_offset;

    grp_qual->qid_arr    = new_qid_arr;
    grp_qual->offset_arr = new_qoffset_arr;
    grp_qual->size       = new_size;

    return (BCM_E_NONE);
}


/* Recover all qualifiers for the given group, based on its selector codes */

STATIC int
_field_tr2_group_construct_quals_add(int              unit,
                                     _field_control_t *fc,
                                     _field_stage_t   *stage_fc,
                                     _field_group_t   *fg
                                     )
{
    int rv;
    int parts_cnt, part_idx;

    rv = _bcm_field_entry_tcam_parts_count(unit, fg->flags, &parts_cnt);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    for (part_idx = 0; part_idx < parts_cnt; ++part_idx) {
        _bcm_field_group_qual_t *grp_qual = &fg->qual_arr[part_idx];
        unsigned qid;

        for (qid = 0; qid < _bcmFieldQualifyCount; ++qid) {
            _bcm_field_qual_info_t *f_qual_arr = stage_fc->f_qual_arr[qid];
            unsigned               j;
            uint8                  diff_cnt;

            if (f_qual_arr == NULL) {
                continue; /* Qualifier does not exist in this stage */
            }

            if (fc->l2warm && !BCM_FIELD_QSET_TEST(fg->qset, qid)) {
                continue; /* Qualifier not present in the group */
            }

            /* Add all of the stage's qualifiers that match the recovered
               selector codes.  Qualifiers that appear more than once
               (because more than one configuration of a qualifier matches
               the recovered selector codes) will be cleaned up later.
            */

            for (j = 0; j < f_qual_arr->conf_sz; j++) {
                if (_field_selector_diff(unit,
                                         fg->sel_codes,
                                         part_idx,
                                         &f_qual_arr->conf_arr[j].selector,
                                         &diff_cnt
                                         )
                    != BCM_E_NONE
                    || diff_cnt != 0
                    ) {
                    continue;
                }

                if (!fc->l2warm) {
                    BCM_FIELD_QSET_ADD(fg->qset, qid);
                }

                _field_tr2_group_qual_add(grp_qual,
                                          qid,
                                          &f_qual_arr->conf_arr[j].offset
                                          );

                _field_qset_udf_bmap_reinit(unit,
                                            stage_fc,
                                            &fg->qset,
                                            qid
                                            );
            }
        }
    }

    return (BCM_E_NONE);
}


int
_field_tr2_group_construct(int                   unit,
                           _field_hw_qual_info_t *hw_sel,
                           int                   intraslice,
                           int                   paired,
                           _field_control_t      *fc,
                           bcm_port_t            port,
                           _field_stage_id_t     stage_id,
                           int                   slice_idx
                           )
{
    int rv = BCM_E_NONE;
    _field_group_t *fg_ptr = fc->groups;
    bcm_field_group_t gid;
    int               priority;
    _field_stage_t *stage_fc;
    bcm_field_qset_t qset;
    bcm_pbmp_t all_pbmp;

    bcm_field_qset_t_init(&qset);

    /* Get the stage control structure */
    BCM_IF_ERROR_RETURN(_field_stage_control_get(unit, stage_id, &stage_fc));

    /* Iterate over groups and compare all selectors */
    fg_ptr = fc->groups;
    while (fg_ptr != NULL) {
        if (intraslice && !(fg_ptr->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE)) {
            fg_ptr = fg_ptr->next;
            continue;
        }
        if (!intraslice && (fg_ptr->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE)) {
            fg_ptr = fg_ptr->next;
            continue;
        }
        if (paired && (fg_ptr->flags & _FP_GROUP_SPAN_SINGLE_SLICE)) {
            fg_ptr = fg_ptr->next;
            continue;
        }
        if (!paired && (fg_ptr->flags & _FP_GROUP_SPAN_DOUBLE_SLICE)) {
            fg_ptr = fg_ptr->next;
            continue;
        }
        /* Check for primary slice numbers - there may be groups with the same
           QSET but residing in different slices */
        if (slice_idx != fg_ptr->slices[0].slice_number) {
            fg_ptr = fg_ptr->next;
            continue;
        }
        /* Compare primary slice selectors */
        if ((fg_ptr->sel_codes[0].fpf1 != hw_sel->pri_slice[0].fpf1) ||
            (fg_ptr->sel_codes[0].fpf2 != hw_sel->pri_slice[0].fpf2) ||
            (fg_ptr->sel_codes[0].fpf3 != hw_sel->pri_slice[0].fpf3) ||
            (fg_ptr->sel_codes[0].fpf4 != hw_sel->pri_slice[0].fpf4) ||
            (!_field_src_dst_entity_compare(fg_ptr->sel_codes[0].src_entity_sel,
                                        hw_sel->pri_slice[0].src_entity_sel)) ||
            (!_field_src_dst_entity_compare
                                   (fg_ptr->sel_codes[0].ingress_entity_sel,
                                    hw_sel->pri_slice[0].ingress_entity_sel)) ||
            (!_field_src_dst_entity_compare
                                    (fg_ptr->sel_codes[0].dst_fwd_entity_sel,
                                    hw_sel->pri_slice[0].dst_fwd_entity_sel
                                     )
             ) ||
            (fg_ptr->sel_codes[0].aux_tag_1_sel != hw_sel->pri_slice[0].aux_tag_1_sel) ||
            (fg_ptr->sel_codes[0].aux_tag_2_sel != hw_sel->pri_slice[0].aux_tag_2_sel)
            || ((SOC_IS_TRIUMPH2(unit)
                || SOC_IS_APOLLO(unit)
                || SOC_IS_VALKYRIE2(unit)
                || SOC_IS_TRIUMPH3(unit)
                || SOC_IS_ENDURO(unit))
                && (fg_ptr->sel_codes[0].loopback_type_sel
                != hw_sel->pri_slice[0].loopback_type_sel))) {
            fg_ptr = fg_ptr->next;
            continue;
        }
        if (intraslice
            && (fg_ptr->sel_codes[1].fpf2 != hw_sel->pri_slice[1].fpf2
                || fg_ptr->sel_codes[1].fpf4 != hw_sel->pri_slice[1].fpf4
                || !_field_src_dst_entity_compare
                       (fg_ptr->sel_codes[1].dst_fwd_entity_sel,
                        hw_sel->pri_slice[1].dst_fwd_entity_sel)
                )
            ) {
            fg_ptr = fg_ptr->next;
            continue;
        }
        /* Compare second and third slice config if mode is paired and intraslice**/
        if (paired && intraslice) {
            if ((fg_ptr->sel_codes[2].fpf1 != hw_sel->sec_slice[0].fpf1) ||
                (fg_ptr->sel_codes[2].fpf2 != hw_sel->sec_slice[0].fpf2) ||
                (fg_ptr->sel_codes[2].fpf3 != hw_sel->sec_slice[0].fpf3) ||
                (fg_ptr->sel_codes[2].fpf4 != hw_sel->sec_slice[0].fpf4) ||
                (!_field_src_dst_entity_compare
                                   (fg_ptr->sel_codes[2].src_entity_sel,
                                    hw_sel->sec_slice[0].src_entity_sel)) ||
                (!_field_src_dst_entity_compare
                                   (fg_ptr->sel_codes[2].ingress_entity_sel,
                                    hw_sel->sec_slice[0].ingress_entity_sel)) ||
                (!_field_src_dst_entity_compare
                                   (fg_ptr->sel_codes[2].dst_fwd_entity_sel,
                                hw_sel->sec_slice[0].dst_fwd_entity_sel
                                    )
                 ) ||
                (fg_ptr->sel_codes[2].aux_tag_1_sel != hw_sel->sec_slice[0].aux_tag_1_sel) ||
                (fg_ptr->sel_codes[2].aux_tag_2_sel != hw_sel->sec_slice[0].aux_tag_2_sel)
                ) {
                fg_ptr = fg_ptr->next;
                continue;
            }
            if (fg_ptr->sel_codes[3].fpf2 != hw_sel->sec_slice[1].fpf2
                    || fg_ptr->sel_codes[3].fpf4 != hw_sel->sec_slice[1].fpf4
                    || !_field_src_dst_entity_compare
                                     (fg_ptr->sel_codes[3].dst_fwd_entity_sel,
                                      hw_sel->sec_slice[1].dst_fwd_entity_sel)
                ) {
                fg_ptr = fg_ptr->next;
                continue;
            }
        }

        /* Compare secondary slice if paired */
        if (paired && !intraslice) {
            if ((fg_ptr->sel_codes[1].fpf1 != hw_sel->sec_slice[0].fpf1) ||
                (fg_ptr->sel_codes[1].fpf2 != hw_sel->sec_slice[0].fpf2) ||
                (fg_ptr->sel_codes[1].fpf3 != hw_sel->sec_slice[0].fpf3) ||
                (fg_ptr->sel_codes[1].fpf4 != hw_sel->sec_slice[0].fpf4) ||
                (!_field_src_dst_entity_compare
                                    (fg_ptr->sel_codes[1].src_entity_sel,
                                    hw_sel->sec_slice[0].src_entity_sel)) ||
                (!_field_src_dst_entity_compare
                                    (fg_ptr->sel_codes[1].ingress_entity_sel,
                                    hw_sel->sec_slice[0].ingress_entity_sel)) ||
                (!_field_src_dst_entity_compare
                                   (fg_ptr->sel_codes[1].dst_fwd_entity_sel,
                                hw_sel->sec_slice[0].dst_fwd_entity_sel)) ||
                (fg_ptr->sel_codes[1].aux_tag_1_sel != hw_sel->sec_slice[0].aux_tag_1_sel) ||
                (fg_ptr->sel_codes[1].aux_tag_2_sel != hw_sel->sec_slice[0].aux_tag_2_sel)
                ) {
                fg_ptr = fg_ptr->next;
                continue;
            }
        }

        /* Found a match - add port to group pbmp */
        BCM_PBMP_PORT_ADD(fg_ptr->pbmp, port);
        BCM_PBMP_OR(fg_ptr->slices[0].pbmp, fg_ptr->pbmp);
        if (1 == paired) {
            BCM_PBMP_OR(fg_ptr->slices[1].pbmp, fg_ptr->pbmp);
        }

        if (fc->l2warm) {
            /* Get stored group ID and QSET for Level 2 */
            BCM_IF_ERROR_RETURN
                (_field_group_info_retrieve(unit,
                                            port,
                                            &gid,
                                            &priority,
                                            &qset,
                                            fc
                                            )
                 );
            if (gid != -1) {
                sal_memcpy(&fg_ptr->qset, &qset, sizeof(bcm_field_qset_t));
                fg_ptr->gid      = gid;
                fg_ptr->priority = priority;
            }
        }
        break;
    }
    if (fg_ptr == NULL) {
        /* No match - create a new group and populate the group structure */

        rv = _field_tr2_group_construct_alloc(unit, &fg_ptr);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }

        if (fc->l2warm) {
            /* Get stored group ID and QSET for Level 2 */
            rv = _field_group_info_retrieve(unit,
                                            port,
                                            &gid,
                                            &priority,
                                            &qset,
                                            fc
                                            );
            if (gid != -1) {
                sal_memcpy(&fg_ptr->qset, &qset, sizeof(bcm_field_qset_t));
            }
        } else {
            /* Generate group ID (a ++ operation) for Level 1 */

            unsigned vmap, vslice;

            rv = _field_group_id_generate(unit, &gid);

            if (BCM_SUCCESS(rv)) {
                for (priority = -1, vmap = 0; priority == -1 && vmap < _FP_VMAP_CNT; ++vmap) {
                    for (vslice = 0; vslice < COUNTOF(stage_fc->vmap[0]); ++vslice) {
                        if (stage_fc->vmap[vmap][vslice].vmap_key == slice_idx) {
                            priority = stage_fc->vmap[vmap][vslice].priority;
                            
                            break;
                        }
                    }
                }
                
                if (priority == -1) {
                    rv = BCM_E_INTERNAL;
                }
            }
        }
        if (BCM_FAILURE(rv)) {
            sal_free(fg_ptr);
            return rv;
        }

        fg_ptr->gid      = gid;
        fg_ptr->priority = priority;
        fg_ptr->stage_id = stage_id;

        /* Set group's selector codes to those recovered from hardware */
        fg_ptr->sel_codes[0] = hw_sel->pri_slice[0];
        if (paired && intraslice) {
            /* 4 parts */
            fg_ptr->sel_codes[1] = hw_sel->pri_slice[1];
            fg_ptr->sel_codes[2] = hw_sel->sec_slice[0];
            fg_ptr->sel_codes[3] = hw_sel->sec_slice[1];
        } else if (paired && !intraslice) {
            /* 2 parts */
            fg_ptr->sel_codes[1] = hw_sel->sec_slice[0];
        } else if (!paired && intraslice) {
            /* 2 parts */
            fg_ptr->sel_codes[1] = hw_sel->pri_slice[1];
        }

        /* Refine selector codes, if possible */
        if (fc->l2warm) {
            /* Refine VP selectors only if it's used in hardware. */
            if (hw_sel->pri_slice[0].dst_fwd_entity_sel
                == _bcmFieldFwdEntityMimGport) {
                if (BCM_FIELD_QSET_TEST
                        (fg_ptr->qset, bcmFieldQualifyDstMplsGport)) {
                    fg_ptr->sel_codes[0].dst_fwd_entity_sel
                        = _bcmFieldFwdEntityMplsGport;
                } else if (BCM_FIELD_QSET_TEST
                            (fg_ptr->qset, bcmFieldQualifyDstMimGport)) {
                    fg_ptr->sel_codes[0].dst_fwd_entity_sel
                        = _bcmFieldFwdEntityMimGport;
                } else if (BCM_FIELD_QSET_TEST
                            (fg_ptr->qset, bcmFieldQualifyDstWlanGport)) {
                    fg_ptr->sel_codes[0].dst_fwd_entity_sel
                        = _bcmFieldFwdEntityWlanGport;
                }
            }

            if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                    bcmFieldQualifySrcMplsGport)) {
                if (_FP_SELCODE_DONT_CARE
                    != fg_ptr->sel_codes[0].ingress_entity_sel) {
                    fg_ptr->sel_codes[0].ingress_entity_sel =
                                           _bcmFieldFwdEntityMplsGport;
                } else if (_bcmFieldFwdEntityAny
                    != fg_ptr->sel_codes[0].src_entity_sel) {
                    fg_ptr->sel_codes[0].src_entity_sel =
                                           _bcmFieldFwdEntityMplsGport;
                }
            } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                           bcmFieldQualifySrcMimGport)) {
                if (_FP_SELCODE_DONT_CARE
                    != fg_ptr->sel_codes[0].ingress_entity_sel) {
                    fg_ptr->sel_codes[0].ingress_entity_sel =
                                           _bcmFieldFwdEntityMimGport;
                } else if (_bcmFieldFwdEntityAny
                    != fg_ptr->sel_codes[0].src_entity_sel) {
                    fg_ptr->sel_codes[0].src_entity_sel =
                                           _bcmFieldFwdEntityMimGport;
                }
            } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                           bcmFieldQualifySrcWlanGport)) {
                if (_bcmFieldFwdEntityAny
                    != fg_ptr->sel_codes[0].src_entity_sel) {
                    fg_ptr->sel_codes[0].src_entity_sel =
                                           _bcmFieldFwdEntityWlanGport;
                }
            } else if ((BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                            bcmFieldQualifyInPort) ||
                        BCM_FIELD_QSET_TEST(fg_ptr->qset, 
                                            bcmFieldQualifyInterfaceClassPort))
                       && (_BCM_FIELD_STAGE_LOOKUP == stage_id)) {
                /* Only for VFP. IFP uses IPBM for InPort */ 
                if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[0].src_entity_sel) {
                    fg_ptr->sel_codes[0].src_entity_sel =
                                           _bcmFieldFwdEntityPortGroupNum;
                }
            }
        }

        if (port != -1) {
            BCM_PBMP_PORT_ADD(fg_ptr->pbmp, port);
        } else {
            SOC_PBMP_CLEAR(all_pbmp);
            SOC_PBMP_ASSIGN(all_pbmp, PBMP_PORT_ALL(unit));
            SOC_PBMP_OR(all_pbmp, PBMP_CMIC(unit));
#if defined(BCM_KATANA2_SUPPORT)
            if (soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) {
                _bcm_kt2_subport_pbmp_update(unit, &all_pbmp);
            }
#endif
            SOC_PBMP_ASSIGN(fg_ptr->pbmp, all_pbmp);
        }

        /* double-wide but not paired */
        if (intraslice) {
            fg_ptr->flags |= _FP_GROUP_INTRASLICE_DOUBLEWIDE;

            if (fc->l2warm
                && (hw_sel->pri_slice[1].dst_fwd_entity_sel
                    == _bcmFieldFwdEntityMimGport)) {
                /* Refine VP selectors only if it's used in hardware. */
                if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                        bcmFieldQualifyDstMplsGport)) {
                    fg_ptr->sel_codes[1].dst_fwd_entity_sel =
                                           _bcmFieldFwdEntityMplsGport;
                } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                               bcmFieldQualifyDstMimGport)) {
                    fg_ptr->sel_codes[1].dst_fwd_entity_sel =
                                            _bcmFieldFwdEntityMimGport;
                } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                               bcmFieldQualifyDstWlanGport)) {
                    fg_ptr->sel_codes[1].dst_fwd_entity_sel =
                                           _bcmFieldFwdEntityWlanGport;
                }
            }
        }

        if ((1 == paired) && (1 == intraslice)) {
            fg_ptr->flags |= _FP_GROUP_SPAN_DOUBLE_SLICE;
            fg_ptr->flags |= _FP_GROUP_INTRASLICE_DOUBLEWIDE;

            if (fc->l2warm) {
                /* Refine VP selectors only if it's used in hardware. */
                if (hw_sel->sec_slice[0].dst_fwd_entity_sel
                    == _bcmFieldFwdEntityMimGport) {
                    if (BCM_FIELD_QSET_TEST
                            (fg_ptr->qset, bcmFieldQualifyDstMplsGport)) {
                        fg_ptr->sel_codes[2].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityMplsGport;
                    } else if (BCM_FIELD_QSET_TEST
                                (fg_ptr->qset, bcmFieldQualifyDstMimGport)) {
                        fg_ptr->sel_codes[2].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityMimGport;
                    } else if (BCM_FIELD_QSET_TEST
                                (fg_ptr->qset, bcmFieldQualifyDstWlanGport)) {
                        fg_ptr->sel_codes[2].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityWlanGport;
                    }
                }

                if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                        bcmFieldQualifySrcMplsGport)) {
                    if (_FP_SELCODE_DONT_CARE
                        != fg_ptr->sel_codes[2].ingress_entity_sel) {
                        fg_ptr->sel_codes[2].ingress_entity_sel =
                                               _bcmFieldFwdEntityMplsGport;
                    } else if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[2].src_entity_sel) {
                        fg_ptr->sel_codes[2].src_entity_sel =
                                               _bcmFieldFwdEntityMplsGport;
                    }
                } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                               bcmFieldQualifySrcMimGport)) {
                    if (_FP_SELCODE_DONT_CARE
                        != fg_ptr->sel_codes[2].ingress_entity_sel) {
                        fg_ptr->sel_codes[2].ingress_entity_sel =
                                               _bcmFieldFwdEntityMimGport;
                    } else if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[2].src_entity_sel) {
                        fg_ptr->sel_codes[2].src_entity_sel =
                                               _bcmFieldFwdEntityMimGport;
                    }
                } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                               bcmFieldQualifySrcWlanGport)) {
                    if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[2].src_entity_sel) {
                        fg_ptr->sel_codes[2].src_entity_sel =
                                               _bcmFieldFwdEntityWlanGport;
                    }
                } else if ((BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                                bcmFieldQualifyInPort) ||
                            BCM_FIELD_QSET_TEST(fg_ptr->qset, 
                                                bcmFieldQualifyInterfaceClassPort))
                           && (_BCM_FIELD_STAGE_LOOKUP == stage_id)) {
                    /* Only for VFP. IFP uses IPBM for InPort */ 
                    if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[2].src_entity_sel) {
                        fg_ptr->sel_codes[2].src_entity_sel =
                                               _bcmFieldFwdEntityPortGroupNum;
                    }
                }

                /* part-4 */
                if (hw_sel->sec_slice[1].dst_fwd_entity_sel
                    == _bcmFieldFwdEntityMimGport) {
                    if (BCM_FIELD_QSET_TEST
                            (fg_ptr->qset, bcmFieldQualifyDstMplsGport)) {
                        fg_ptr->sel_codes[3].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityMplsGport;
                    } else if (BCM_FIELD_QSET_TEST
                                (fg_ptr->qset, bcmFieldQualifyDstMimGport)) {
                        fg_ptr->sel_codes[3].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityMimGport;
                    } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                                bcmFieldQualifyDstWlanGport)) {
                        fg_ptr->sel_codes[3].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityWlanGport;
                    }
                }
            }
        } else if ((1 == paired) && (0 == intraslice)) {
            /* paired but not intraslice */
            fg_ptr->flags |= _FP_GROUP_SPAN_DOUBLE_SLICE;

            if (fc->l2warm) {
                /* part-2 */
                /* Refine VP selectors only if it's used in hardware. */
                if (hw_sel->sec_slice[0].dst_fwd_entity_sel
                    == _bcmFieldFwdEntityMimGport) {
                    if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                            bcmFieldQualifyDstMplsGport)) {
                        fg_ptr->sel_codes[1].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityMplsGport;
                    } else if (BCM_FIELD_QSET_TEST
                                (fg_ptr->qset, bcmFieldQualifyDstMimGport)) {
                        fg_ptr->sel_codes[1].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityMimGport;
                    } else if (BCM_FIELD_QSET_TEST
                                (fg_ptr->qset, bcmFieldQualifyDstWlanGport)) {
                        fg_ptr->sel_codes[1].dst_fwd_entity_sel
                            = _bcmFieldFwdEntityWlanGport;
                    }
                }

                if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                        bcmFieldQualifySrcMplsGport)) {
                    if (_FP_SELCODE_DONT_CARE
                        != fg_ptr->sel_codes[1].ingress_entity_sel) {
                        fg_ptr->sel_codes[1].ingress_entity_sel =
                                               _bcmFieldFwdEntityMplsGport;
                    } else if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[1].src_entity_sel) {
                        fg_ptr->sel_codes[1].src_entity_sel =
                                               _bcmFieldFwdEntityMplsGport;
                    }
                } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                               bcmFieldQualifySrcMimGport)) {
                    if (_FP_SELCODE_DONT_CARE
                        != fg_ptr->sel_codes[1].ingress_entity_sel) {
                        fg_ptr->sel_codes[1].ingress_entity_sel =
                                               _bcmFieldFwdEntityMimGport;
                    } else if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[1].src_entity_sel) {
                        fg_ptr->sel_codes[1].src_entity_sel =
                                               _bcmFieldFwdEntityMimGport;
                    }
                } else if (BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                               bcmFieldQualifySrcWlanGport)) {
                    if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[1].src_entity_sel) {
                        fg_ptr->sel_codes[1].src_entity_sel =
                                               _bcmFieldFwdEntityWlanGport;
                    }
                } else if ((BCM_FIELD_QSET_TEST(fg_ptr->qset,
                                                bcmFieldQualifyInPort) ||
                            BCM_FIELD_QSET_TEST(fg_ptr->qset, 
                                                bcmFieldQualifyInterfaceClassPort))
                           && (_BCM_FIELD_STAGE_LOOKUP == stage_id)) {
                    /* Only for VFP. IFP uses IPBM for InPort */ 
                    if (_bcmFieldFwdEntityAny
                        != fg_ptr->sel_codes[1].src_entity_sel) {
                        fg_ptr->sel_codes[1].src_entity_sel =
                                               _bcmFieldFwdEntityPortGroupNum;
                    }
                }
            }
        } else {
            fg_ptr->flags |= _FP_GROUP_SPAN_SINGLE_SLICE;
        }

        _field_tr2_group_construct_quals_add(unit,
                                             fc,
                                             stage_fc,
                                             fg_ptr
                                             );

        /* Associate slice(s) to group */
        if (fg_ptr->slices == NULL) {
            fg_ptr->slices = stage_fc->slices + slice_idx;
            if (1 == intraslice) {
                fg_ptr->slices->group_flags |= _FP_GROUP_INTRASLICE_DOUBLEWIDE;
                if (1 == paired) {
                    fg_ptr->slices[1].group_flags |=
                                               _FP_GROUP_INTRASLICE_DOUBLEWIDE;
                }
            }
        }

        BCM_PBMP_OR(fg_ptr->slices[0].pbmp, fg_ptr->pbmp);
        if (1 == paired) {
            BCM_PBMP_OR(fg_ptr->slices[1].pbmp, fg_ptr->pbmp);
        }

        /* Initialize group default ASET list. */
        rv = _field_group_default_aset_set(unit, fg_ptr);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }

        fg_ptr->flags |= _FP_GROUP_LOOKUP_ENABLED;
        fg_ptr->next = fc->groups;
        fc->groups = fg_ptr;
    }

    return rv;
}


/* Delete the qualifier at the given index from the given group's qualifier array */

STATIC void
_field_tr2_group_qual_del(_bcm_field_group_qual_t *grp_qual, unsigned idx)
{
    uint16                   *qidp    = &grp_qual->qid_arr[idx];
    _bcm_field_qual_offset_t *offsetp = &grp_qual->offset_arr[idx];
    unsigned                 n;

    /* Shuffle all qualifier entries down by one slot, overwriting the one to be deleted */

    for (n = grp_qual->size - 1 - idx; n; --n, ++qidp, ++offsetp) {
        qidp[0]    = qidp[1];
        offsetp[0] = offsetp[1];
    }

    /* Reduce qualifier count */

    --grp_qual->size;
}

/* Eliminate duplicate (i.e. unused) qualifiers from a group's qualifier array */

STATIC void
_field_tr2_group_part_cleanup(_bcm_field_group_qual_t *grp_qual, uint8 *cnt_arr,
                            uint32 *tcam_mask, int part_idx)
{
    unsigned i;

    for (i = 0; i < grp_qual->size; ) {
        _bcm_field_qual_offset_t *q = &grp_qual->offset_arr[i];
        unsigned                 b = 0, b1 = 0, b2 = 0;
        uint8                    *cntp = &cnt_arr[grp_qual->qid_arr[i]];

        if ((*cntp == 1)
            || ((0 == part_idx)
            && (bcmFieldQualifyInPort == grp_qual->qid_arr[i]
            || bcmFieldQualifyInPorts == grp_qual->qid_arr[i]
            || bcmFieldQualifyStage == grp_qual->qid_arr[i]
            || bcmFieldQualifyStageIngress == grp_qual->qid_arr[i]
            || bcmFieldQualifyStageLookup == grp_qual->qid_arr[i]
            || bcmFieldQualifyStageExternal == grp_qual->qid_arr[i]
            || bcmFieldQualifyStageEgress == grp_qual->qid_arr[i]))) {
            /* Qualifier appears only once OR dup qualifier should not
             * be removed from primary part_idx.
             *  => Don't delete it from the group.
             */
            ++i;

            continue;
        }

        /* Check the qualifier against the given TCAM mask */

        SHR_BITTEST_RANGE(tcam_mask, q->offset, q->width, b);
        if (q->width1 != 0) {
            SHR_BITTEST_RANGE(tcam_mask, q->offset1, q->width1, b1);
        }
        if (q->width2 != 0) {
            SHR_BITTEST_RANGE(tcam_mask, q->offset2, q->width2, b2);
        }

        if (b | b1 | b2) {
            /* Qualifier is in use (i.e. has a non-empty mask)
               => Don't delete it from the group.
            */

            ++i;

            continue;
        }

        /* Qualifier appears more than once, and this instance does not
           appear in the given TCAM mask
           => Delete qualifier from group.
        */

        _field_tr2_group_qual_del(grp_qual, i);
        --*cntp;
    }
}

int
_field_tr2_pbmp_status_get_from_fp_gm(int unit, int phys_tcam_idx,
                                    uint8 *clearInPorts,
                                    _field_table_pointers_t *fp_table_pointers)
{
    fp_global_mask_tcam_entry_t *global_mask_tcam_entry;
    fp_tcam_entry_t             *tcam_entry;
    bcm_pbmp_t                  entry_pbmp, entry_mask_pbmp;
#if (defined (BCM_TRIDENT_SUPPORT) || defined(BCM_SCORPION_SUPPORT) || defined(BCM_CONQUEROR_SUPPORT))
    bcm_pbmp_t                  temp_pbmp, temp_pbm_mask;
#endif
    uint32                      *fp_tcam_buf = NULL;
    char                        *fp_gm_tcam_buf = NULL; /* Buffer to read the 
                                                         * FP_GLOBAL_MASK_TCAM
                                                         * table */

#if defined(BCM_TRIDENT_SUPPORT)
    char *fp_gm_tcam_x_buf = NULL;                     /* Buffer to read
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * table. */
    fp_global_mask_tcam_x_entry_t *fp_gm_tcam_x_entry; /* Pointer to
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * entry */
    char *fp_gm_tcam_y_buf = NULL;                     /* Buffer to read
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * table. */
    fp_global_mask_tcam_y_entry_t *fp_gm_tcam_y_entry; /* Pointer to
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * entry */
#endif
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_CONQUEROR_SUPPORT)
    uint32  tcam_dual_pipe_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Y-Pipe FP_TCAM
                                                            * entry. */
#endif


    if ((NULL == clearInPorts) || (NULL == fp_table_pointers)) {
          return (BCM_E_INTERNAL);
    }

    BCM_PBMP_CLEAR(entry_pbmp);
    BCM_PBMP_CLEAR(entry_mask_pbmp);
#if (defined (BCM_TRIDENT_SUPPORT) || defined(BCM_SCORPION_SUPPORT) || defined(BCM_CONQUEROR_SUPPORT))
    BCM_PBMP_CLEAR(temp_pbmp);
    BCM_PBMP_CLEAR(temp_pbm_mask);
#endif

    if (soc_mem_is_valid(unit, FP_GLOBAL_MASK_TCAMm)) {
#if defined (BCM_TRIDENT_SUPPORT)
        if (SOC_IS_TD_TT(unit)) {

            fp_gm_tcam_x_buf = fp_table_pointers->fp_gm_tcam_x_buf;
            fp_gm_tcam_y_buf = fp_table_pointers->fp_gm_tcam_y_buf; 

            if ((NULL == fp_gm_tcam_x_buf) || (NULL == fp_gm_tcam_y_buf )) {
                 return (BCM_E_INTERNAL);
            }

            /* Get X-Pipe IPBM KEY and MASK value. */
            fp_gm_tcam_x_entry = soc_mem_table_idx_to_pointer(unit,
                                            FP_GLOBAL_MASK_TCAM_Xm,
                                            fp_global_mask_tcam_x_entry_t *,
                                            fp_gm_tcam_x_buf,
                                            phys_tcam_idx);
            soc_mem_pbmp_field_get(unit,
                                   FP_GLOBAL_MASK_TCAM_Xm,
                                   fp_gm_tcam_x_entry,
                                   IPBMf,
                                   &entry_pbmp);
            soc_mem_pbmp_field_get(unit,
                                   FP_GLOBAL_MASK_TCAM_Xm,
                                   fp_gm_tcam_x_entry,
                                   IPBM_MASKf,
                                   &entry_mask_pbmp);

            /* Get Y-Pipe IPBM KEY and MASK value. */
            fp_gm_tcam_y_entry = soc_mem_table_idx_to_pointer(unit,
                                            FP_GLOBAL_MASK_TCAM_Ym,
                                            fp_global_mask_tcam_y_entry_t *,
                                            fp_gm_tcam_y_buf,
                                            phys_tcam_idx);
            soc_mem_pbmp_field_get(unit,
                                   FP_GLOBAL_MASK_TCAM_Ym,
                                   fp_gm_tcam_y_entry,
                                   IPBMf,
                                   &temp_pbmp);
            soc_mem_pbmp_field_get(unit,
                                   FP_GLOBAL_MASK_TCAM_Ym,
                                   fp_gm_tcam_y_entry,
                                   IPBM_MASKf,
                                   &temp_pbm_mask);
            SOC_PBMP_OR(entry_pbmp, temp_pbmp);
            SOC_PBMP_OR(entry_mask_pbmp, temp_pbm_mask);
        } else
#endif
        {
            fp_gm_tcam_buf = fp_table_pointers->fp_global_mask_tcam_buf;

            if (NULL == fp_gm_tcam_buf) {
                return BCM_E_INTERNAL;  
            }
            global_mask_tcam_entry = soc_mem_table_idx_to_pointer(unit,
                                                FP_GLOBAL_MASK_TCAMm,
                                                fp_global_mask_tcam_entry_t *,
                                                fp_gm_tcam_buf,
                                                phys_tcam_idx);
            soc_mem_pbmp_field_get(unit,
                                   FP_GLOBAL_MASK_TCAMm,
                                   global_mask_tcam_entry,
                                   IPBMf,
                                   &entry_pbmp);
            soc_mem_pbmp_field_get(unit,
                                   FP_GLOBAL_MASK_TCAMm,
                                   global_mask_tcam_entry,
                                   IPBM_MASKf,
                                   &entry_mask_pbmp);
        }

    } else {

        fp_tcam_buf = fp_table_pointers->fp_tcam_buf;
        if (NULL == fp_tcam_buf) {
            return BCM_E_INTERNAL; 
        }
        tcam_entry = soc_mem_table_idx_to_pointer(unit, FP_TCAMm,
                                                  fp_tcam_entry_t *,
                                                  fp_tcam_buf,
                                                  phys_tcam_idx);

        if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) == 0) {
            return BCM_E_INTERNAL;
        }

    
        soc_mem_pbmp_field_get(unit,
                               FP_TCAMm,
                               tcam_entry,
                               IPBMf,
                               &entry_pbmp);
        soc_mem_pbmp_field_get(unit,
                               FP_TCAMm,
                               tcam_entry,
                               IPBM_MASKf,
                               &entry_mask_pbmp);
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_CONQUEROR_SUPPORT)
        /*
         * Scorpion and Conqurer are dual pipe line devices.
         * Retrieve IPBMf and IPBM_MASKf field values from Y Pipleline
         * FP_TCAM table.
         */
        if (SOC_IS_SC_CQ(unit)) {
            sal_memset(&tcam_dual_pipe_entry, 0,sizeof(tcam_dual_pipe_entry));

            BCM_IF_ERROR_RETURN(soc_mem_read(unit, FP_TCAM_Ym, MEM_BLOCK_ANY,
                                phys_tcam_idx, tcam_dual_pipe_entry));

            soc_mem_pbmp_field_get(unit,
                                   FP_TCAM_Ym,
                                   tcam_dual_pipe_entry,
                                   IPBMf,
                                   &temp_pbmp);
            soc_mem_pbmp_field_get(unit,
                                   FP_TCAM_Ym,
                                   tcam_dual_pipe_entry,
                                   IPBM_MASKf,
                                   &temp_pbm_mask);
            SOC_PBMP_OR(entry_pbmp, temp_pbmp);
            SOC_PBMP_OR(entry_mask_pbmp, temp_pbm_mask);
        }
#endif
      
    }

    if(BCM_PBMP_IS_NULL(entry_pbmp) && BCM_PBMP_IS_NULL(entry_mask_pbmp)) {
        *clearInPorts = 1;
    } else {
        *clearInPorts = 0;
    }

    return BCM_E_NONE;
}

/* Eliminate duplicate qualifiers from all groups in the given stage */

int
_field_tr2_stage_reinit_all_groups_cleanup(int unit, 
                                     _field_control_t *fc,
                                     unsigned stage_id,
                                     _field_table_pointers_t *fp_table_pointers)
{
    int            idx, rv = BCM_E_NONE;
    uint8          clear_inports = 0, installed_entry_found = 0;
    unsigned       cnt_arr_sz;
    uint8          *cnt_arr;
    _field_group_t *fg;
    _field_entry_t      *f_ent;

    cnt_arr_sz = _bcmFieldQualifyCount * sizeof(cnt_arr[0]);
    if ((cnt_arr = (uint8 *) sal_alloc(cnt_arr_sz, "qual cnt array")) == 0) {
        return (BCM_E_MEMORY);
    }

    /* For all groups, ... */

    for (fg = fc->groups; fg; fg = fg->next) {
        int      parts_cnt, part_idx;
        unsigned duplf;

        /* In the given stage, ... */

        if (fg->stage_id != stage_id) {
            continue;
        }

        rv = _bcm_field_entry_tcam_parts_count(unit, fg->flags, &parts_cnt);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }

        /* Construct an array of counts, 1 per qualifier, and set each count
           to the number times the qualifier occurs in the given group
        */

        sal_memset(cnt_arr, 0, cnt_arr_sz);
        duplf = FALSE;
        for (part_idx = 0; part_idx < parts_cnt; ++part_idx) {
            _bcm_field_group_qual_t *grp_qual = &fg->qual_arr[part_idx];
            unsigned                i;

            /* Construct an array of counts, 1 per qualifier, and set each count
               to the number times the qualifier occurs in the given group
            */

            for (i = 0; i < grp_qual->size; ++i) {
                if (++cnt_arr[grp_qual->qid_arr[i]] > 1) {
                    /* Count for a qualifier just exceeded 1 */

                    duplf = TRUE;       /* Indicate there are duplicate qualifiers */
                }
            }
        }

        if (!duplf) {
            /* No duplicate qualifiers => Skip to next group */

            continue;
        }

        /* There are some duplicate qualifiers
           => Compute the union of all TCAM masks, over all entries in
           group, and use that to determine which qualifiers are in use.
        */

        for (part_idx = 0; part_idx < parts_cnt; ++part_idx) {
            _bcm_field_group_qual_t *grp_qual = &fg->qual_arr[part_idx];
            unsigned                i;
            uint32                  tcam_mask[SOC_MAX_MEM_FIELD_WORDS];

            sal_memset(tcam_mask, 0, sizeof(tcam_mask));

            for (i = 0; i < fg->group_status.entry_count; ++i) {
                _field_entry_t *f_ent               = &fg->entry_arr[i][part_idx];
                unsigned       free_entry_tcam_buff = (f_ent->tcam.key == 0);
                unsigned       j;

                rv = _bcm_field_qual_tcam_key_mask_get(unit, f_ent);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }

                for (j = 0; j < (f_ent->tcam.key_size / sizeof(uint32)); ++j) {
                    tcam_mask[j] |= f_ent->tcam.mask[j];
                }

                if (free_entry_tcam_buff) {
                    if (f_ent->tcam.key != 0) {
                        sal_free(f_ent->tcam.key);
                        sal_free(f_ent->tcam.mask);
                    }

                    if (f_ent->flags & _FP_ENTRY_USES_IPBM_OVERLAY) {
                        if (f_ent->extra_tcam.key != 0) {
                            sal_free(f_ent->extra_tcam.key);
                            sal_free(f_ent->extra_tcam.mask);
                        }
                    }

                    f_ent->tcam.key
                        = f_ent->tcam.mask
                        = f_ent->extra_tcam.key
                        = f_ent->extra_tcam.mask
                        = 0;
                }
            }

            /* Use the union of all TCAM masks for all entries in the group to
               eliminate duplicate qualifiers from group
            */

            _field_tr2_group_part_cleanup(grp_qual, cnt_arr, tcam_mask, part_idx);
        }

        if (_BCM_FIELD_STAGE_INGRESS == stage_id) {

            if (BCM_FIELD_QSET_TEST(fg->qset,bcmFieldQualifyLoopback) || 
                BCM_FIELD_QSET_TEST(fg->qset,bcmFieldQualifyLoopbackType)) {

                clear_inports        = 0;
                installed_entry_found = 0;

                for(idx = 0; idx < fg->group_status.entry_count; idx++) {
                    f_ent = fg->entry_arr[idx];
                    if(_FP_ENTRY_INSTALLED & f_ent->flags) {
                        installed_entry_found =1;
                        break;
                    }
                }

                if(installed_entry_found) {

                    rv = _field_tr2_pbmp_status_get_from_fp_gm(unit, 
                                                       f_ent->slice_idx,
                                                       &clear_inports,
                                                       fp_table_pointers);
                    if (BCM_FAILURE(rv)) {
                        goto cleanup;
                    }
        
                    if (clear_inports) {
                        BCM_FIELD_QSET_REMOVE(fg->qset,bcmFieldQualifyInPort);
                        BCM_FIELD_QSET_REMOVE(fg->qset,bcmFieldQualifyInPorts);
                    }
                }
            }
        }
    }

 cleanup:
    sal_free(cnt_arr);

    return (rv);
}


int
_field_tr2_ifp_slice_expanded_status_get(int            unit,
                                         _field_stage_t *stage_fc,
                                         int            *expanded
                                         )
{
    static const soc_field_t ifp_virtual_to_physical_map_tr[][16] = {
        { VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_4_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_5_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_6_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_7_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_8_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_9_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_10_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_11_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_12_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_13_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_14_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
          VIRTUAL_SLICE_15_PHYSICAL_SLICE_NUMBER_ENTRY_0f

        },
        { VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_4_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_5_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_6_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_7_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_8_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_9_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_10_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_11_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_12_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_13_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_14_PHYSICAL_SLICE_NUMBER_ENTRY_1f,
          VIRTUAL_SLICE_15_PHYSICAL_SLICE_NUMBER_ENTRY_1f

        },
        { VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_4_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_5_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_6_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_7_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_8_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_9_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_10_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_11_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_12_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_13_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_14_PHYSICAL_SLICE_NUMBER_ENTRY_2f,
          VIRTUAL_SLICE_15_PHYSICAL_SLICE_NUMBER_ENTRY_2f
        }
    };
    static const soc_field_t ifp_virtual_to_group_map_tr[][16] = {
        { VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_4_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_5_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_6_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_7_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_8_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_9_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_10_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_11_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_12_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_13_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_14_VIRTUAL_SLICE_GROUP_ENTRY_0f,
          VIRTUAL_SLICE_15_VIRTUAL_SLICE_GROUP_ENTRY_0f

        },
        { VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_4_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_5_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_6_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_7_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_8_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_9_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_10_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_11_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_12_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_13_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_14_VIRTUAL_SLICE_GROUP_ENTRY_1f,
          VIRTUAL_SLICE_15_VIRTUAL_SLICE_GROUP_ENTRY_1f
        },
        { VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_4_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_5_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_6_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_7_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_8_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_9_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_10_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_11_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_12_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_13_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_14_VIRTUAL_SLICE_GROUP_ENTRY_2f,
          VIRTUAL_SLICE_15_VIRTUAL_SLICE_GROUP_ENTRY_2f
        }
    };

    int                  i, vmap, vmap_cnt, vslice, slice_idx, vgroup = -1, max = -1;
    int                  nslices;
    fp_slice_map_entry_t map_entry;
    soc_field_t          fld;

    if (!soc_mem_field_valid(unit,
                             FP_SLICE_MAPm,
                             ifp_virtual_to_physical_map_tr[0][0]
                             )
        ) {
        /* Fall back to Firebolt */

        return (_field_slice_expanded_status_get(unit, stage_fc, expanded));
    }

    BCM_IF_ERROR_RETURN(READ_FP_SLICE_MAPm(unit, MEM_BLOCK_ANY, 0, &map_entry));
    nslices = stage_fc->tcam_slices;
    /* Iterate over virtual slices to get physical slices and virtual groups */



    vmap_cnt =
        /* Trident uses the field nomenclature from TR2, but has only 1 vmap */
        (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? 1 : _FP_VMAP_CNT;

    for (vmap = 0; vmap < vmap_cnt; ++vmap) {
        for (vslice = 0; vslice < nslices; vslice++) {
            fld = ifp_virtual_to_physical_map_tr[vmap][vslice];
            if (!soc_mem_field_valid(unit,FP_SLICE_MAPm,fld)) {
                continue;
            }
            slice_idx = soc_FP_SLICE_MAPm_field32_get(unit, &map_entry, fld);
            /* Now get virtual group for this virtual slice */
            fld = ifp_virtual_to_group_map_tr[vmap][vslice];
            if (!soc_mem_field_valid(unit,FP_SLICE_MAPm,fld)) {
                continue;
            }
            vgroup = soc_FP_SLICE_MAPm_field32_get(unit, &map_entry, fld);
            /* Phys slice_idx <=> virtual vslice <=> vgroup */
            stage_fc->vmap[vmap][vslice].vmap_key = slice_idx;
            stage_fc->vmap[vmap][vslice].virtual_group = vgroup;
            stage_fc->vmap[vmap][vslice].priority = vgroup;
            
           }

        /* See if virtual slice is the highest number in the virtual group */
        for (vslice = 0; (vslice < nslices) && (vmap == 0); vslice++) {
            max = -1;
            for (i = 0; i < nslices; i++) {
                if ((stage_fc->vmap[vmap][vslice].virtual_group ==
                    stage_fc->vmap[vmap][i].virtual_group)) {
                    if (i > max) {
                        max = i;
                    }
                }
            }
            if ((max != vslice) && (max != -1)) {
                expanded[stage_fc->vmap[vmap][vslice].vmap_key] = 1;
            }
        }
    }

    return BCM_E_NONE;
}

int
_field_tr2_group_entry_write(int unit, int slice_idx, _field_slice_t *fs,
                             _field_control_t *fc, _field_stage_t *stage_fc)
{
    int qset_count, ratio, idx, prev_prio, multigroup, master_slice;
    uint8 stat_present, pol_present, prio_ctrl;
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1];
    _field_group_t *fg;
    _field_entry_t *f_ent;
    bcm_port_t port;
    bcm_field_qualify_t q;
    bcm_pbmp_t port_cmic_pbmp;
    _field_slice_t *temp_fs = fs;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
    _field_stat_t  *f_st;
    int flex_mode=0;
    int hw_flags=0;
    uint8 flex_stat_map=0;
#endif


    SOC_PBMP_ASSIGN(port_cmic_pbmp, PBMP_PORT_ALL(unit));
    SOC_PBMP_OR(port_cmic_pbmp, PBMP_CMIC(unit));
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &port_cmic_pbmp);
    }
#endif

    /* Check for multiple groups in a slice */
    if (SOC_PBMP_EQ(fs->pbmp, port_cmic_pbmp)) {
        multigroup = 0;
    } else {
        FP_VVERB(("_field_scache_sync:   Multigroup.\n"));
        multigroup = 1;
    }
    FP_VVERB(("_field_scache_sync:   Writing slice %d @ byte %d...\n",
              slice_idx, fc->scache_pos));

    buf[fc->scache_pos] = (slice_idx << 1) | multigroup;
    fc->scache_pos++;

    /* Get the master slice if expanded */
    master_slice = slice_idx;
    while (temp_fs->prev != NULL) {
        temp_fs = temp_fs->prev;
        master_slice = temp_fs->slice_number;
    }

    /* Traverse all groups and dump group data for this slice */
    fg = fc->groups;
    while (fg != NULL) {
        if (fg->stage_id != stage_fc->stage_id) {
            fg = fg->next;
            continue; /* Not in this stage */
        }
        if (fg->slices[0].slice_number != master_slice) {
            FP_VVERB(("_field_scache_sync:   No match on group %d.\n",
                      fg->gid));
            fg = fg->next;
            continue; /* Not in this slice */
        }
        FP_VVERB(("_field_scache_sync:   Match on group %d...\n",
                  fg->gid));

        FP_VVERB(("_field_scache_sync:   Writing group %d @ byte %d...\n",
                  fg->gid, fc->scache_pos));

        SOC_PBMP_ITER(fg->pbmp, port) {
            break; /* Pick first port found as representative */
        }

        if (fc->flags & _FP_STABLE_SAVE_LONG_IDS) {
            /* Save long GID */

            buf[fc->scache_pos] = fg->gid;
            fc->scache_pos++;
            buf[fc->scache_pos] = fg->gid >> 8;
            fc->scache_pos++;
            buf[fc->scache_pos] = fg->gid >> 16;
            fc->scache_pos++;
            buf[fc->scache_pos] = fg->gid >> 24;
            fc->scache_pos++;
        } else {
            /* Save compact GID */

            buf[fc->scache_pos] = fg->gid; /* GID_lo */
            fc->scache_pos++;
            buf[fc->scache_pos] = fg->gid >> 8; /* GID_hi */
            fc->scache_pos++;
        }
        buf[fc->scache_pos] = port;        /* Rep_port */
        fc->scache_pos++;

        /* Write group priority */
        buf[fc->scache_pos] = fg->priority;
        fc->scache_pos++;
        buf[fc->scache_pos] = fg->priority >> 8;
        fc->scache_pos++;
        buf[fc->scache_pos] = fg->priority >> 16;
        fc->scache_pos++;
        buf[fc->scache_pos] = fg->priority >> 24;
        fc->scache_pos++;

        /* Obtain list of qualifiers in fg->qset */
        qset_count = 0;
        _FIELD_QSET_INCL_INTERNAL_ITER(fg->qset, q) {
            qset_count++;
        }

        FP_VVERB(("_field_scache_sync:   Writing qset count (%d) @ byte %d...\n",
                  qset_count, fc->scache_pos));

        buf[fc->scache_pos] = qset_count; /* qset_count */
        fc->scache_pos++;
        _FIELD_QSET_INCL_INTERNAL_ITER(fg->qset, q) {
            FP_VVERB(("_field_scache_sync:     Writing qid %d @ byte %d...\n",
                      q, fc->scache_pos));

            buf[fc->scache_pos] = q; /* QID */
            fc->scache_pos++;
            if(NULL != buf1) {
                buf1[fc->scache_pos1] = q >> 8; 
                fc->scache_pos1++;
            }
        }
        fg = fg->next;
        if ((fg != NULL)
            && multigroup
            && (fg->slices[0].slice_number == slice_idx)
            ) {
            buf[fc->scache_pos] = 1; /* Next group valid */
        } else {
            buf[fc->scache_pos] = 0; /* Next group not valid */
        }
        fc->scache_pos++;
    }
    /* Traverse all entries and dump entry data for this slice */
    if (fs->group_flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
        ratio = 2;
    } else {
        ratio = 1;
    }
    prev_prio = -1;
    for (idx = 0; idx < fs->entry_count / ratio; idx++) {
        /* Find EID that matches this HW index */
        f_ent = fs->entries[idx];
        if (f_ent == NULL) {
            continue;
        }
        if (f_ent->flags & _FP_ENTRY_INSTALLED) {
            prio_ctrl = (f_ent->prio != prev_prio) ? 1 : 0;
            stat_present = (f_ent->statistic.flags &
                            _FP_ENTRY_STAT_INSTALLED) ? 1 : 0;
            pol_present = (f_ent->policer[0].flags &
                           _FP_POLICER_INSTALLED) ? 1 : 0;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
            if ((stat_present) && 
                (soc_feature(unit, soc_feature_advanced_flex_counter)) &&
                (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id)) {
                BCM_IF_ERROR_RETURN(_bcm_field_stat_get(
                                    unit, f_ent->statistic.sid, &f_st));
                flex_mode  = f_st->flex_mode;
                hw_flags  = f_st->hw_flags;
                switch(f_st->nstat) {
                case 1:if(f_st->stat_arr[0] == bcmFieldStatBytes) {
                          flex_stat_map = 1;
                       } else {
                          flex_stat_map = 2;
                       }
                       break;
                case 2:
                default: /* Although 0-and-default cases are invalid */
                       flex_stat_map=3;
                       break;
                }
            }
#endif
            if (fc->flags & _FP_STABLE_SAVE_LONG_IDS) {
                /* Save long EID */

                buf[fc->scache_pos] = f_ent->eid;
                fc->scache_pos++;
                buf[fc->scache_pos] = f_ent->eid >> 8;
                fc->scache_pos++;
                buf[fc->scache_pos] = f_ent->eid >> 16;
                fc->scache_pos++;
                buf[fc->scache_pos] = f_ent->eid >> 24;
                fc->scache_pos++;

                buf[fc->scache_pos] = prio_ctrl
                    | (stat_present << 1)
                    | (pol_present << 2);
                fc->scache_pos++;
            } else {
                /* Save compact EID */

                buf[fc->scache_pos] = f_ent->eid & 0xFF; /* EID_lo */
                fc->scache_pos++;
                buf[fc->scache_pos] = (f_ent->eid >> 8) | (prio_ctrl << 5) |
                    (stat_present << 6) | (pol_present << 7);
                /* EID_hi + control bits */
                fc->scache_pos++;
            }

            if (multigroup) {
                /* Multiple groups in slice => Save id of group entry belongs to */

                if (fc->flags & _FP_STABLE_SAVE_LONG_IDS) {
                    /* Save complete GID */

                    buf[fc->scache_pos] = f_ent->group->gid;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->group->gid >> 8;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->group->gid >> 16;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->group->gid >> 24;
                    fc->scache_pos++;
                } else {
                    /* Save compact GID */

                    buf[fc->scache_pos] = f_ent->group->gid; /* GID_lo */
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->group->gid >> 8; /* GID_hi */
                    fc->scache_pos++;
                }
            }
            if (prio_ctrl) {
                /* 32-bit priority is written */
                buf[fc->scache_pos] = f_ent->prio & 0xFF;
                fc->scache_pos++;
                buf[fc->scache_pos] = (f_ent->prio >> 8) & 0xFF;
                fc->scache_pos++;
                buf[fc->scache_pos] = (f_ent->prio >> 16) & 0xFF;
                fc->scache_pos++;
                buf[fc->scache_pos] = (f_ent->prio >> 24) & 0xFF;
                fc->scache_pos++;
                prev_prio = f_ent->prio;
            }

            /* Write policer and stat object IDs if present */

            if (fc->flags & _FP_STABLE_SAVE_LONG_IDS) {
                /* Save complete policer and stat ids */

                if (pol_present) {
                    buf[fc->scache_pos] = f_ent->policer[0].pid;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->policer[0].pid >> 8;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->policer[0].pid >> 16;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->policer[0].pid >> 24;
                    fc->scache_pos++;
                }

                if (stat_present) {
                    buf[fc->scache_pos] = f_ent->statistic.sid;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->statistic.sid >> 8;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->statistic.sid >> 16;
                    fc->scache_pos++;
                    buf[fc->scache_pos] = f_ent->statistic.sid >> 24;
                    fc->scache_pos++;
                }
            } else {
                /* Save compact policer and stat ids */

                if (pol_present) {
                    buf[fc->scache_pos] = (f_ent->policer[0].pid) & 0xFF;
                    fc->scache_pos++;
                }
                if (stat_present) {
                    if (pol_present) {
                        buf[fc->scache_pos] =
                            (((f_ent->policer[0].pid >> 8) & 0xF) << 4) |
                            (f_ent->statistic.sid & 0xF);
                        fc->scache_pos++;
                    } else {
                        buf[fc->scache_pos] = f_ent->statistic.sid & 0xF;
                        fc->scache_pos++;
                    }
                    buf[fc->scache_pos] = (f_ent->statistic.sid >> 4) & 0xFF;
                    fc->scache_pos++;
                } else if (pol_present) {
                    buf[fc->scache_pos] =
                        ((f_ent->policer[0].pid >> 8) & 0xF) << 4;
                    fc->scache_pos++;
                }
            }
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
            if ((stat_present)  &&
                (soc_feature(unit, soc_feature_advanced_flex_counter)) &&
                (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id)) {
                buf[fc->scache_pos] = flex_mode;
                fc->scache_pos++;
                buf[fc->scache_pos] = flex_mode >> 8;
                fc->scache_pos++;
                buf[fc->scache_pos] = flex_mode >> 16;
                fc->scache_pos++;
                buf[fc->scache_pos] = flex_mode >> 24;
                fc->scache_pos++;
                buf[fc->scache_pos] = hw_flags;
                fc->scache_pos++;
                buf[fc->scache_pos] = hw_flags >> 8;
                fc->scache_pos++;
                buf[fc->scache_pos] = hw_flags >> 16;
                fc->scache_pos++;
                buf[fc->scache_pos] = hw_flags >> 24;
                fc->scache_pos++;
                buf[fc->scache_pos] = flex_stat_map;
                fc->scache_pos++;
           }
#endif
        }
    }
    /* Mark next slice valid */
    return BCM_E_NONE;
}

/*
 * This function manages the Level 2 warm boot cache. The data will be stored
 * in the following format:
 * Start indicator (32 bits): Value = IFP/EFP/VFP
 *
 * Range check IDs (IFP)
 * 8-bit ID count (1 uint8)
 * ID0 (4 x uint8)
 * .
 * IDN (4 x uint8)
 *
 * Data qualifier data, as follows:
 * If the device has a FP_UDF_TCAM, the following is present, for each TCAM entry:
 * 8-bit TCAM entry reference count (1 uint 8)
 * Followed by:
 * 8-bit data qualifier count (1 uint 8)
 * 32-bit data qualifier id (1 int)
 * 2-bit "length remainder" | 3-bit flags | 3-bit offset base (1 uint 8)
 * 16-bit data qualifier match-data offset (1 uint 16)
 * 16-bit hw_bmap field of data qualifier
 *   (Note that this field is linked to the hardware; namely, the width of this
 *   bitmap is the number of UDF n-byte chunks supported, i.e. the number of
 *   "columns" in the UDF_OFFSET table.)
 *
 * Group/entry data
 * Slice num (5 bits) | Multigroup slice indicator (1 bit): (1 uint8)
 * GID_lo (8 bits): (1 uint 8)
 * GID_hi (8 bits): (1 uint 8)
 * Representative_port (8 bits): (1 uint 8)
 * Group priority (32 bits): (4 uint 8)
 * Qualifier count (8 bits): (1 uint 8)
 * QID 1 (8 bits): (1 uint 8)
 * .
 * QID N (8 bits): (1 uint 8)
 * Next_group_valid (1 bit) | Reserved (7 bits): (1 uint 8)
 *
 * Groups are immediately followed by entries, which can have one of the
 * following two formats depending on the multigroup slice indicator:
 * Long format:
 * 8 bit EID_lo: (1 uint 8)
 * 5 bit EID_hi | priority_ctrl (1 bit) | Policer (1 bit) | Stat (1 bit):
  (1 uint 8)
 * 8 bit GID_lo: (1 uint 8)
 * 8 bit GID_hi: (1 uint 8)
 * priority_0: (1 uint 8)
 * priority_1: (1 uint 8) <---- The 32-bit priority is stored only if it is
 * priority_2: (1 uint 8)       different from the entry before it.
 * priority_3: (1 uint 8)
 * PID_lo (8 bits): (1 uint8)
 * PID_hi (4 bits) | SID_lo (4 bits): (1 uint8)
 * SID_hi (4 bits): (1 uint 8)
 * Short format:
 * 8 bit EID_lo: (1 uint 8)
 * 5 bit EID_hi | priority_ctrl (1 bit) | Policer (1 bit) | Stat (1 bit):
  (1 uint 8)
 * priority_0: (1 uint 8)
 * priority_1: (1 uint 8) <---- The 32-bit priority is stored only if it is
 * priority_2: (1 uint 8)       different from the entry before it.
 * priority_3: (1 uint 8)
 * PID_lo (8 bits): (1 uint8)
 * PID_hi (4 bits) | SID_lo (4 bits): (1 uint8)
 * SID_hi (8 bits): (1 uint 8)
 * End indicator (32 bits): Value = IFP/EFP/VFP
 */

STATIC int
_field_tr2_ext_scache_sync_chk(int              unit,
                               _field_control_t *fc,
                               _field_stage_t   *stage_fc
                               );
STATIC int
_field_tr2_ext_scache_sync(int              unit,
                           _field_control_t *fc,
                           _field_stage_t   *stage_fc
                           );

int
_field_tr2_scache_sync(int              unit,
                       _field_control_t *fc,
                       _field_stage_t   *stage_fc
                       )
{
    int slice_idx, range_count = 0;
    int rv = BCM_E_NONE;
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1]; 
    _field_slice_t *fs;
    _field_group_t *fg;
    uint32 start_char, end_char;
    soc_field_t fld;
    int efp_slice_mode, paired = 0;
    uint32 val;
    fp_port_field_sel_entry_t pfs;
    _field_data_control_t *data_ctrl;
    _field_range_t *fr;
    int ratio = 0;
    int idx;
    _field_entry_t *f_ent;


    soc_field_t _fb2_slice_pairing_field[8] = {
        SLICE1_0_PAIRINGf,   SLICE3_2_PAIRINGf,
        SLICE5_4_PAIRINGf,   SLICE7_6_PAIRINGf,
        SLICE9_8_PAIRINGf,   SLICE11_10_PAIRINGf,
        SLICE13_12_PAIRINGf, SLICE15_14_PAIRINGf};
    soc_field_t _efp_slice_mode[] = {SLICE_0_MODEf, SLICE_1_MODEf,
                                     SLICE_2_MODEf, SLICE_3_MODEf};

    switch (stage_fc->stage_id) {
    case _BCM_FIELD_STAGE_INGRESS:
        start_char = _FIELD_IFP_DATA_START;
        end_char = _FIELD_IFP_DATA_END;
        break;
    case _BCM_FIELD_STAGE_EGRESS:
        start_char = _FIELD_EFP_DATA_START;
        end_char = _FIELD_EFP_DATA_END;
        break;
    case _BCM_FIELD_STAGE_LOOKUP:
        start_char = _FIELD_VFP_DATA_START;
        end_char = _FIELD_VFP_DATA_END;
        break;
    case _BCM_FIELD_STAGE_EXTERNAL:
        if (_field_tr2_ext_scache_sync_chk(unit, fc, stage_fc)) {
            return (_field_tr2_ext_scache_sync(unit, fc, stage_fc));
        }
        start_char = _FIELD_EXTFP_DATA_START;
        end_char   = _FIELD_EXTFP_DATA_END;
        break;
    default:
        return BCM_E_PARAM;
    }

    FP_VVERB(("_field_scache_sync: Synching scache for FP stage %d...\n", stage_fc->stage_id));

    _field_scache_stage_hdr_save(fc, start_char);

    /* Save the range check IDs */
    if (stage_fc->stage_id == _BCM_FIELD_STAGE_INGRESS) {
        fr = stage_fc->ranges;
        while (fr) {
            fr = fr->next;
            range_count++;
        }
        buf[fc->scache_pos] = (uint8)range_count;
        fc->scache_pos++;
        if (range_count) {
            fr = stage_fc->ranges;
            while (fr) {
                buf[fc->scache_pos] = fr->rid & 0xFF;
                fc->scache_pos++;
                buf[fc->scache_pos] = (fr->rid >> 8) & 0xFF;
                fc->scache_pos++;
                buf[fc->scache_pos] = (fr->rid >> 16) & 0xFF;
                fc->scache_pos++;
                buf[fc->scache_pos] = (fr->rid >> 24) & 0xFF;
                fc->scache_pos++;
                fr = fr->next;
            }
        }
    }

    /* Save data qualifiers */

    if ((data_ctrl = stage_fc->data_ctrl) != 0) {
        if (soc_mem_is_valid(unit, FP_UDF_TCAMm)) {
            /* Device has UDF TCAM =>
               Save internal information regarding TCAM entry usage
            */

            _field_data_tcam_entry_t *p;
            unsigned                 n;

            for (p = data_ctrl->tcam_entry_arr,
                     n = soc_mem_index_count(unit, FP_UDF_TCAMm);
                 n;
                 --n, ++p
                 ) {
                buf[fc->scache_pos] = p->ref_count;
                fc->scache_pos++;
            }
        }

        _field_scache_sync_data_quals_write(fc, data_ctrl);
    }

    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        FP_VVERB(("_field_scache_sync: Checking slice %d...\n", slice_idx));
        /* Skip slices without groups */
        fs = stage_fc->slices + slice_idx;
        fg = fc->groups;
        while (fg != NULL) {
            if (fg->stage_id != stage_fc->stage_id) {
                fg = fg->next;
                continue; /* Not in this stage */
            }
            if (fg->slices[0].slice_number == slice_idx) {
                break;
            }
            fg = fg->next;
        }
        if (fg == NULL) {
            continue; /* No group found */
        }
        /* Also skip expanded slices */
        if (stage_fc->slices[slice_idx].prev != NULL) {
            continue;
        }
        /* Ignore secondary slice in paired mode */
        switch (stage_fc->stage_id) {
        case _BCM_FIELD_STAGE_INGRESS:
            BCM_IF_ERROR_RETURN(soc_mem_read(unit, FP_PORT_FIELD_SELm,
                                             MEM_BLOCK_ANY, 0, &pfs));
            fld = _fb2_slice_pairing_field[slice_idx / 2];
            paired = soc_FP_PORT_FIELD_SELm_field32_get(unit,
                                                        &pfs, fld);
            break;
        case _BCM_FIELD_STAGE_EGRESS:
            BCM_IF_ERROR_RETURN(READ_EFP_SLICE_CONTROLr(unit, &val));
            efp_slice_mode = soc_reg_field_get(unit, EFP_SLICE_CONTROLr,
                                               val,_efp_slice_mode[slice_idx]);
            if ((efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6)) {
                paired = 1;
            }
            break;
        case _BCM_FIELD_STAGE_LOOKUP:
            BCM_IF_ERROR_RETURN(READ_VFP_KEY_CONTROLr(unit, &val));
            fld = _bcm_field_trx_slice_pairing_field[slice_idx / 2];
            paired = soc_reg_field_get(unit, VFP_KEY_CONTROLr, val, fld);
            break;
        case _BCM_FIELD_STAGE_EXTERNAL:
            paired = 0;
            break;
        default:
            return BCM_E_PARAM;
            break;
        }
        if (paired && (slice_idx % 2)) {
            continue;
        }
        BCM_IF_ERROR_RETURN
            (_field_tr2_group_entry_write(unit, slice_idx, fs, fc, stage_fc));
    }

    /* Now sync the expanded slices */
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        fs = stage_fc->slices + slice_idx;
        /* Skip empty slices */
        if (fs->entry_count == fs->free_count) {
            FP_VVERB(("_field_scache_sync:   Slice is empty.\n"));
            continue;
        }
        /* Skip master slices */
        if (stage_fc->slices[slice_idx].prev == NULL) {
            continue;
        }

        /* 
         * Skip expanded slices with no entries installed in Hw
         * to match recovery logic.
         */
        if (fs->group_flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
            ratio = 2;
        } else {
            ratio = 1;
        }

        for (idx = 0; idx < fs->entry_count / ratio; idx++) {
            /* Find EID that matches this HW index */
            f_ent = fs->entries[idx];
            if (f_ent == NULL) {
                continue;
            }
            if (!(f_ent->flags & _FP_ENTRY_INSTALLED)) {
                continue;
            }
            break;
        }

        if (idx == (fs->entry_count / ratio)) {
            continue;
        }

        /* Ignore secondary slice in paired mode */
        switch (stage_fc->stage_id) {
        case _BCM_FIELD_STAGE_INGRESS:
            BCM_IF_ERROR_RETURN(soc_mem_read(unit, FP_PORT_FIELD_SELm,
                                             MEM_BLOCK_ANY, 0, &pfs));
            fld = _fb2_slice_pairing_field[slice_idx / 2];
            paired = soc_FP_PORT_FIELD_SELm_field32_get(unit,
                                                        &pfs, fld);
            break;
        case _BCM_FIELD_STAGE_EGRESS:
            BCM_IF_ERROR_RETURN(READ_EFP_SLICE_CONTROLr(unit, &val));
            efp_slice_mode = soc_reg_field_get(unit, EFP_SLICE_CONTROLr,
                                               val,_efp_slice_mode[slice_idx]);
            if ((efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6)) {
                paired = 1;
            }
            break;
        case _BCM_FIELD_STAGE_LOOKUP:
            BCM_IF_ERROR_RETURN(READ_VFP_KEY_CONTROLr(unit, &val));
            fld = _bcm_field_trx_slice_pairing_field[slice_idx / 2];
            paired = soc_reg_field_get(unit, VFP_KEY_CONTROLr, val, fld);
            break;
        case _BCM_FIELD_STAGE_EXTERNAL:
            paired = 0;
            break;
        default:
            return BCM_E_PARAM;
            break;
        }
        if (paired && (slice_idx % 2)) {
            continue;
        }
        BCM_IF_ERROR_RETURN
            (_field_tr2_group_entry_write(unit, slice_idx, fs, fc, stage_fc));
    }

    FP_VVERB(("_field_scache_sync: Writing end of section @ byte %d.\n",
              fc->scache_pos));

    /* Mark the end of the IFP section */
    buf[fc->scache_pos] = end_char & 0xFF;
    fc->scache_pos++;
    buf[fc->scache_pos] = (end_char >> 8) & 0xFF;
    fc->scache_pos++;
    buf[fc->scache_pos] = (end_char >> 16) & 0xFF;
    fc->scache_pos++;
    buf[fc->scache_pos] = (end_char >> 24) & 0xFF;
    fc->scache_pos++;
    fc->scache_usage = fc->scache_pos; /* Usage in bytes */
    
    if(NULL != buf1) {
        /* Mark the end of the IFP section */
        buf1[fc->scache_pos1] = end_char & 0xFF;
        fc->scache_pos1++;
        buf1[fc->scache_pos1] = (end_char >> 8) & 0xFF;
        fc->scache_pos1++;
        buf1[fc->scache_pos1] = (end_char >> 16) & 0xFF;
        fc->scache_pos1++;
        buf1[fc->scache_pos1] = (end_char >> 24) & 0xFF;
        fc->scache_pos1++;
    }
    return rv;
}


/* Retrieve all GIDs from a slice during Level 2 warm boot */
int
_field_trx_scache_slice_group_recover(int              unit,
                                      _field_control_t *fc,
                                      int              slice_num,
                                      int              *multigroup,
                                      _field_stage_t   *stage_fc
                                      )
{
    int i, slice_idx;
    int next_group_valid;
    int rv = BCM_E_INTERNAL;
    int qset_count;
    int qset;
    bcm_field_group_t gid;
    int               priority;
    bcm_port_t port;
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1];
    _field_slice_group_info_t *new_grp;
    int vmap_size; /* Virtual map index count. */

    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
        return (_field_scache_slice_group_recover(unit,
                                                  fc, slice_num, multigroup
                                                  )
                );
    }

    /* Parse the scache buffer to recover GIDs and QSETs */
    /* To be used in stage reinit */
    slice_idx = buf[fc->scache_pos] >> 1;
    FP_VVERB(("Read slice index %d @ byte %d\n", slice_idx, fc->scache_pos));
    if (multigroup) {
        *multigroup = buf[fc->scache_pos] & 0x1;
    }

    fc->scache_pos++;
    if (slice_idx != slice_num) {
        /* No groups stored for this slice */
        fc->scache_pos--;
        return BCM_E_NOT_FOUND;
    }
    fc->group_arr = NULL;

    /* Parse all the group data for this slice */
    do {
        if (fc->flags & _FP_STABLE_SAVE_LONG_IDS) {
            /* Read long group id */

            gid = buf[fc->scache_pos];
            fc->scache_pos++;
            gid |= buf[fc->scache_pos] << 8;
            fc->scache_pos++;
            gid |= buf[fc->scache_pos] << 16;
            fc->scache_pos++;
            gid |= buf[fc->scache_pos] << 24;
            fc->scache_pos++;
        } else {
            /* Read compact group id  */

            gid = buf[fc->scache_pos]; /* GID_lo */
            fc->scache_pos++;
            gid |= (buf[fc->scache_pos] << 8); /* GID_hi */
            fc->scache_pos++;
        }

        port = buf[fc->scache_pos]; /* Rep_port */
        fc->scache_pos++;

        FP_VVERB(("Read group id %d @ %d.\n", gid, fc->scache_pos - 3));

        /* Read group priority */
        priority = 0;
        priority |= buf[fc->scache_pos];
        fc->scache_pos++;
        priority |= buf[fc->scache_pos] << 8;
        fc->scache_pos++;
        priority |= buf[fc->scache_pos] << 16;
        fc->scache_pos++;
        priority |= buf[fc->scache_pos] << 24;
        fc->scache_pos++;

        /* Add group info to temp linked list */
        new_grp = NULL;
        _FP_XGS3_ALLOC(new_grp, sizeof(_field_slice_group_info_t),
                       "Temp group info");
        if (NULL == new_grp) {
            return (BCM_E_MEMORY);
        }
        new_grp->gid = gid;
        new_grp->priority = priority;
        BCM_PBMP_PORT_ADD(new_grp->pbmp, port);
        new_grp->next = fc->group_arr;
        fc->group_arr = new_grp;
        qset_count = buf[fc->scache_pos]; /* QSET_count */
        fc->scache_pos++;

        FP_VVERB(("Read qset count %d @ %d.\n", qset_count, fc->scache_pos - 1));

        for (i = 0; i < qset_count; i++) {
            qset = 0;
            qset |=  buf[fc->scache_pos];
            fc->scache_pos++;
            if(NULL != buf1) {
                qset |= buf1[fc->scache_pos1] << 8;
                fc->scache_pos1++;
            } 
            BCM_FIELD_QSET_ADD(new_grp->qset, qset);

            FP_VVERB(("Read qualifier %d @ %d.\n", qset, fc->scache_pos - 1));
        }
        next_group_valid =  buf[fc->scache_pos];
        fc->scache_pos++;
    } while (next_group_valid);
    /* The pointer has now advanced to the "entries" section */

    /* Calculate virtual map size. */
    rv = _bcm_field_virtual_map_size_get(unit, stage_fc, &vmap_size);
    BCM_IF_ERROR_RETURN(rv);

    for (i = 0; i < vmap_size; i++) {
        if (stage_fc->vmap[0][i].vmap_key == slice_idx) {
            stage_fc->vmap[0][i].valid = TRUE;
            stage_fc->vmap[0][i].priority = new_grp->priority;
        }
    }

    FP_VVERB(("Done reading slice @ %d.\n", fc->scache_pos));
    return rv;
}

int
_field_trx_entry_info_retrieve(int               unit,
                               bcm_field_entry_t *eid,
                               int               *prio,
                               _field_control_t  *fc,
                               int               multigroup,
                               int               *prev_prio,
                               bcm_field_stat_t  *sid,
                               bcm_policer_t     *pid,
                               _field_stage_t    *stage_fc
                               )
{
    uint8 stat_present, pol_present, prio_ctrl;
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];

    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
        return (_field_entry_info_retrieve(unit,
                                           eid,
                                           prio,
                                           fc,
                                           multigroup,
                                           prev_prio,
                                           sid,
                                           pid
                                           )
                );
    }

    *eid = *sid = *pid = 0;

    if (fc->flags & _FP_STABLE_SAVE_LONG_IDS) {
        /* Read long EID and flags */

        *eid |=  buf[fc->scache_pos];
        fc->scache_pos++;
        *eid |=  buf[fc->scache_pos] << 8;
        fc->scache_pos++;
        *eid |=  buf[fc->scache_pos] << 16;
        fc->scache_pos++;
        *eid |=  buf[fc->scache_pos] << 24;
        fc->scache_pos++;

        prio_ctrl = buf[fc->scache_pos] & 1;
        stat_present = (buf[fc->scache_pos] >> 1) & 1;
        pol_present = (buf[fc->scache_pos] >> 2) & 1;
        fc->scache_pos++;
    } else {
        /* Read compact EID and flags */

        *eid |=  buf[fc->scache_pos]; /* EID_lo */
        fc->scache_pos++;
        *eid |= (buf[fc->scache_pos] & 0x1F) << 8; /* EID_hi */
        prio_ctrl = (buf[fc->scache_pos] >> 5) & 0x1;
        stat_present = (buf[fc->scache_pos] >> 6) & 0x1;
        pol_present = (buf[fc->scache_pos] >> 7) & 0x1;
        fc->scache_pos++;
    }

    FP_VVERB(("Read entry id %d @ byte %d.\n", *eid, fc->scache_pos - 2));

    if (multigroup) {
        fc->scache_pos += (fc->flags & _FP_STABLE_SAVE_LONG_IDS) ? 4 : 2; /* Skip over GID */
    }

    if (prio_ctrl) {
        *prio = 0;
        *prio |= buf[fc->scache_pos];
        fc->scache_pos++;
        *prio |= buf[fc->scache_pos] << 8;
        fc->scache_pos++;
        *prio |= buf[fc->scache_pos] << 16;
        fc->scache_pos++;
        *prio |= buf[fc->scache_pos] << 24;
        fc->scache_pos++;
        *prev_prio = *prio;
    } else {
        *prio = *prev_prio;
    }

    if (fc->flags & _FP_STABLE_SAVE_LONG_IDS) {
        /* Read long policer and stat ids, if present */

        if (pol_present) {
            *pid |= buf[fc->scache_pos];
            fc->scache_pos++;
            *pid |= buf[fc->scache_pos] << 8;
            fc->scache_pos++;
            *pid |= buf[fc->scache_pos] << 16;
            fc->scache_pos++;
            *pid |= buf[fc->scache_pos] << 24;
            fc->scache_pos++;
        }

        if (stat_present) {
            *sid |= buf[fc->scache_pos];
            fc->scache_pos++;
            *sid |= buf[fc->scache_pos] << 8;
            fc->scache_pos++;
            *sid |= buf[fc->scache_pos] << 16;
            fc->scache_pos++;
            *sid |= buf[fc->scache_pos] << 24;
            fc->scache_pos++;
        }
    } else {
        /* Read compact policer and stat ids, if present */

        if (pol_present) {
            *pid |= buf[fc->scache_pos];
            fc->scache_pos++;
        }
        if (stat_present) {
            if (pol_present) {
                *pid |= (buf[fc->scache_pos] & 0xf0) << 4;
            }
            *sid |= buf[fc->scache_pos] & 0xf;
            fc->scache_pos++;
            *sid |= buf[fc->scache_pos] << 4;
            fc->scache_pos++;
        } else if (pol_present) {
            *pid |= (buf[fc->scache_pos] & 0xf0) << 4;
            fc->scache_pos++;
        }
    }
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
    if ((stat_present) &&
        (soc_feature(unit, soc_feature_advanced_flex_counter)) &&
        (stage_fc->stage_id == _BCM_FIELD_STAGE_LOOKUP) &&
        (flex_info.valid == 0)) {
         flex_info.flex_mode = buf[fc->scache_pos];
         fc->scache_pos++;
         flex_info.flex_mode |= buf[fc->scache_pos] << 8;
         fc->scache_pos++;
         flex_info.flex_mode |= buf[fc->scache_pos] << 16;
         fc->scache_pos++;
         flex_info.flex_mode |= buf[fc->scache_pos] << 24;
         fc->scache_pos++;
         flex_info.hw_flags = buf[fc->scache_pos];
         fc->scache_pos++;
         flex_info.hw_flags |= buf[fc->scache_pos] << 8;
         fc->scache_pos++;
         flex_info.hw_flags |= buf[fc->scache_pos] << 16;
         fc->scache_pos++;
         flex_info.hw_flags |= buf[fc->scache_pos] << 24;
         fc->scache_pos++;
         flex_info.flex_stat_map |= buf[fc->scache_pos];
         fc->scache_pos++;

         flex_info.valid = 1;
    }
#endif

    FP_VVERB(("Done reading entry @ %d.\n", fc->scache_pos));

    return BCM_E_NONE;
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
int _field_adv_flex_stat_info_retrieve(int unit, int stat_id)
{
    _field_stat_t *f_st = NULL;
    bcm_stat_group_mode_t   stat_mode;    /* Stat type bcmStatGroupModeXXX. */
    bcm_stat_object_t       stat_obj;     /* Stat object type.              */
    uint32                  pool_num;     /* Flex Stat Hw Pool No.          */
    uint32                  base_index;   /* Flex Stat counter base index.  */
    int                     offset_mode = 0;

    if (!soc_feature(unit, soc_feature_advanced_flex_counter)) {
        return BCM_E_INTERNAL;
    }

    if ((flex_info.valid == 1) && (flex_info.flex_mode != 0)) {
        BCM_IF_ERROR_RETURN(_bcm_field_stat_get(unit, stat_id, &f_st));
                                                
        f_st->flex_mode = flex_info.flex_mode;
        f_st->hw_flags = flex_info.hw_flags;
        _bcm_esw_stat_get_counter_id_info(f_st->flex_mode,
                                          &stat_mode,
                                          &stat_obj,
                                          (uint32 *)&offset_mode,
                                          &pool_num,
                                          &base_index
                                          );
        f_st->hw_index = base_index;
        f_st->pool_index = pool_num;
        f_st->hw_mode = stat_mode; /* Shouldn't be offset_mode*/
        f_st->hw_entry_count = 1; /* PlsNote:For SingleMode=1 */
        /* Currently OnlySingleMode is supportes so above OK */
        /* else decision will be based on stat_mode  
        Probably one helper function will be required */
    }
    flex_info.valid=0;
    return BCM_E_NONE;
}
#endif

#if 0
STATIC int
mem_read(int unit, int mem, char *str, void **buf)
{
    if ((*buf = soc_cm_salloc(unit,
                              soc_mem_index_count(unit, mem)
                              * soc_mem_entry_bytes(unit, mem),
                              str
                              )
         )
        == NULL
        ) {
        return (BCM_E_MEMORY);
    }

    return (soc_mem_read_range(unit,
                               mem,
                               MEM_BLOCK_ANY,
                               soc_mem_index_min(unit, mem),
                               soc_mem_index_max(unit, mem),
                               *buf
                               )
            );
}
#endif

/*
 * Function:
 *     _field_tr2_loopback_type_sel_recover
 *
 * Purpose:
 *     Retrieve slice TunnelType/LoopbackType settings from installed
 *     field entry.
 *
 * Parameters:
 *     unit              - (IN) BCM device number
 *     slice_idx         - (IN) Slice number to enable
 *     fp_tcam_buf       - (IN) TCAM entry
 *     stage_fc          - (IN) FP stage control info.
 *     intraslice        - (IN) Slice in Intra-Slice Double Wide mode.
 *     loopback_type_sel - (OUT) Tunnel/Loopback Type selector value
 *
 * Returns:
 *     Nothing
 */
STATIC int
_field_tr2_loopback_type_sel_recover(int unit,
                                     int slice_idx,
                                     uint32 *fp_tcam_buf,
                                     _field_stage_t *stage_fc,
                                     int intraslice,
                                     int8 *loopback_type_sel)
{
    fp_tcam_entry_t *tcam_entry;                /* Installed TCAM entry.     */
    uint32 tunnel_loopback_type = 0;            /* Hw Tunnel/Loopback value. */
    fp_slice_map_entry_t fp_slice_map_buf;      /* Slice map table entry.    */
    _field_slice_t       *fs;                   /* Pointer to Slice info.    */
    _field_control_t     *fc;                   /* Pointer to field control  */
    int slice_ent_cnt;                          /* No. of entries in slice.  */
    int idx;                                    /* Slice index.              */
    int vmap_size, index;                       /* Virtual Map entry size.   */
    int virtual_group = -1;                     /* Virtual Group ID.         */
    int phy_slice;                              /* Physical slice value.     */
    static const soc_field_t physical_slice[] = {
        VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_4_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_5_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_6_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_7_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_8_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_9_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_10_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_11_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_12_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_13_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_14_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_15_PHYSICAL_SLICE_NUMBER_ENTRY_0f,
        VIRTUAL_SLICE_16_PHYSICAL_SLICE_NUMBER_ENTRY_0f
    }, slice_group[] = {
        VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_4_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_5_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_6_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_7_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_8_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_9_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_10_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_11_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_12_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_13_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_14_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_15_VIRTUAL_SLICE_GROUP_ENTRY_0f,
        VIRTUAL_SLICE_16_VIRTUAL_SLICE_GROUP_ENTRY_0f
    };

    BCM_IF_ERROR_RETURN(_field_control_get(unit, &fc));

    vmap_size = stage_fc->tcam_slices;
    if (fc->flags & _FP_EXTERNAL_PRESENT) {
        ++vmap_size;
    }

    BCM_IF_ERROR_RETURN(soc_mem_read(unit,
                                     FP_SLICE_MAPm,
                                     MEM_BLOCK_ANY,
                                     0,
                                     fp_slice_map_buf.entry_data
                                     )
                        );

    /* Get the virtual group id for given slice_idx. */
    for (index = 0; index < vmap_size; index++) {
        phy_slice = soc_mem_field32_get(unit,
                            FP_SLICE_MAPm,
                            fp_slice_map_buf.entry_data,
                            physical_slice[index]);
        if (slice_idx != phy_slice) {
            continue;
        }
        virtual_group = soc_mem_field32_get(unit,
                            FP_SLICE_MAPm,
                            fp_slice_map_buf.entry_data,
                            slice_group[index]);
        break;
    }

    /* Must be a valid virtual_group to proceed further. */
    if (-1 == virtual_group) {
        return (BCM_E_INTERNAL);
    }

    /*
     * For expanded group slices, check if any entry in
     * this slice has loopback_type_sel selector set.
     */
    for (index = 0; index < vmap_size; index++) {
        if (virtual_group
            != soc_mem_field32_get(unit,
                                   FP_SLICE_MAPm,
                                   fp_slice_map_buf.entry_data,
                                   slice_group[index])) {
            continue;
        }

        phy_slice = soc_mem_field32_get(unit,
                            FP_SLICE_MAPm,
                            fp_slice_map_buf.entry_data,
                            physical_slice[index]);
        fs = stage_fc->slices + phy_slice;
        slice_ent_cnt = fs->entry_count;
    
        if (intraslice) {
            slice_ent_cnt >>= 1;
        }

        for (idx = 0; idx < slice_ent_cnt; idx++) {
            tcam_entry = soc_mem_table_idx_to_pointer
                            (unit,
                             FP_TCAMm,
                             fp_tcam_entry_t *,
                             fp_tcam_buf,
                             idx + slice_ent_cnt * phy_slice
                             );
            if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) != 0) {
                /* Extract Tunnel Info from tcam_entry  */
                _field_extract((uint32 *)tcam_entry,
                               21,
                               4,
                               &tunnel_loopback_type
                               );
                switch (tunnel_loopback_type) {
                    case 0x1: /* IP_TUNNEL       */
                    case 0x2: /* MPLS_TUNNEL     */
                    case 0x3: /* MIM_TUNNEL      */
                    case 0x4: /* WLAN_WTP TUNNEL */
                    case 0x5: /* WLAN_AC TUNNEL  */
                    case 0x6: /* AMT_TUNNEL      */
                    case 0x7: /* TRILL_TUNNEL    */
                        *loopback_type_sel = 1;
                        break;
                    case 0x8: /* NONE                                    */
                    case 0x9: /* EP REDIRECTION LOOPBACK                 */
                    case 0xb: /* MiM LOOPBACK                            */
                    case 0xc: /* WLAN LOOPBACK || Trill Network Loopback */
                    case 0xd: /* Trill Access Loopback                   */
                    case 0xe: /* EGRESS MIRROR LOOPBACK */
                        *loopback_type_sel = 0;
                        break;
                    default:
                        ;
                }
                
                if (*loopback_type_sel != _FP_SELCODE_DONT_CARE) {
                    goto done;
                }
            }
        }
    }

done:
    return (BCM_E_NONE);
}

void
_field_tr2_ingress_entity_get(int unit, int slice_idx, uint32 *fp_tcam_buf,
                              int slice_ent_cnt, _field_stage_t *stage_fc,
                              int8 *ingress_entity_sel)
{
    fp_tcam_entry_t *tcam_entry;
    uint32 svp_valid = 0;
    int idx;

    for (idx = 0; idx < slice_ent_cnt; idx++) {
        tcam_entry = soc_mem_table_idx_to_pointer(unit, FP_TCAMm,
                                                  fp_tcam_entry_t *,
                                                  fp_tcam_buf, idx +
                                                  slice_ent_cnt *
                                                  slice_idx);
        if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) != 0) {
            _field_extract((uint32 *)tcam_entry, 14, 1, &svp_valid);
            if (svp_valid) {
                break;
            }
        }
    }
    if (svp_valid) {
        *ingress_entity_sel = _bcmFieldFwdEntityMimGport;
    }
    return;
}

int
_field_tr2_stage_ingress_reinit(int              unit,
                                _field_control_t *fc,
                                _field_stage_t   *stage_fc
                                )
{
    static const soc_field_t ifp_en_flds[] = {
        FP_SLICE_ENABLE_SLICE_0f,
        FP_SLICE_ENABLE_SLICE_1f,
        FP_SLICE_ENABLE_SLICE_2f,
        FP_SLICE_ENABLE_SLICE_3f,
        FP_SLICE_ENABLE_SLICE_4f,
        FP_SLICE_ENABLE_SLICE_5f,
        FP_SLICE_ENABLE_SLICE_6f,
        FP_SLICE_ENABLE_SLICE_7f,
        FP_SLICE_ENABLE_SLICE_8f,
        FP_SLICE_ENABLE_SLICE_9f,
        FP_SLICE_ENABLE_SLICE_10f,
        FP_SLICE_ENABLE_SLICE_11f,
        FP_SLICE_ENABLE_SLICE_12f,
        FP_SLICE_ENABLE_SLICE_13f,
        FP_SLICE_ENABLE_SLICE_14f,
        FP_SLICE_ENABLE_SLICE_15f
    };
    static const soc_field_t ifp_lk_en_flds[] = {
        FP_LOOKUP_ENABLE_SLICE_0f,
        FP_LOOKUP_ENABLE_SLICE_1f,
        FP_LOOKUP_ENABLE_SLICE_2f,
        FP_LOOKUP_ENABLE_SLICE_3f,
        FP_LOOKUP_ENABLE_SLICE_4f,
        FP_LOOKUP_ENABLE_SLICE_5f,
        FP_LOOKUP_ENABLE_SLICE_6f,
        FP_LOOKUP_ENABLE_SLICE_7f,
        FP_LOOKUP_ENABLE_SLICE_8f,
        FP_LOOKUP_ENABLE_SLICE_9f,
        FP_LOOKUP_ENABLE_SLICE_10f,
        FP_LOOKUP_ENABLE_SLICE_11f,
        FP_LOOKUP_ENABLE_SLICE_12f,
        FP_LOOKUP_ENABLE_SLICE_13f,
        FP_LOOKUP_ENABLE_SLICE_14f,
        FP_LOOKUP_ENABLE_SLICE_15f
    };
    static const soc_field_t ifp_en_field_tbl[][3] = {
        {  SLICE0_F1f,  SLICE0_F2f,  SLICE0_F3f },
        {  SLICE1_F1f,  SLICE1_F2f,  SLICE1_F3f },
        {  SLICE2_F1f,  SLICE2_F2f,  SLICE2_F3f },
        {  SLICE3_F1f,  SLICE3_F2f,  SLICE3_F3f },
        {  SLICE4_F1f,  SLICE4_F2f,  SLICE4_F3f },
        {  SLICE5_F1f,  SLICE5_F2f,  SLICE5_F3f },
        {  SLICE6_F1f,  SLICE6_F2f,  SLICE6_F3f },
        {  SLICE7_F1f,  SLICE7_F2f,  SLICE7_F3f },
        {  SLICE8_F1f,  SLICE8_F2f,  SLICE8_F3f },
        {  SLICE9_F1f,  SLICE9_F2f,  SLICE9_F3f },
        { SLICE10_F1f, SLICE10_F2f, SLICE10_F3f },
        { SLICE11_F1f, SLICE11_F2f, SLICE11_F3f },
        { SLICE12_F1f, SLICE12_F2f, SLICE12_F3f },
        { SLICE13_F1f, SLICE13_F2f, SLICE13_F3f },
        { SLICE14_F1f, SLICE14_F2f, SLICE14_F3f },
        { SLICE15_F1f, SLICE15_F2f, SLICE15_F3f }
    };
    static const soc_field_t ifp_en_sd_type_field_tbl[][2] = {
        {  SLICE0_S_TYPE_SELf,  SLICE0_D_TYPE_SELf},
        {  SLICE1_S_TYPE_SELf,  SLICE1_D_TYPE_SELf},
        {  SLICE2_S_TYPE_SELf,  SLICE2_D_TYPE_SELf},
        {  SLICE3_S_TYPE_SELf,  SLICE3_D_TYPE_SELf},
        {  SLICE4_S_TYPE_SELf,  SLICE4_D_TYPE_SELf},
        {  SLICE5_S_TYPE_SELf,  SLICE5_D_TYPE_SELf},
        {  SLICE6_S_TYPE_SELf,  SLICE6_D_TYPE_SELf},
        {  SLICE7_S_TYPE_SELf,  SLICE7_D_TYPE_SELf},
        {  SLICE8_S_TYPE_SELf,  SLICE8_D_TYPE_SELf},
        {  SLICE9_S_TYPE_SELf,  SLICE9_D_TYPE_SELf},
        { SLICE10_S_TYPE_SELf, SLICE10_D_TYPE_SELf},
        { SLICE11_S_TYPE_SELf, SLICE11_D_TYPE_SELf},
        { SLICE12_S_TYPE_SELf, SLICE12_D_TYPE_SELf},
        { SLICE13_S_TYPE_SELf, SLICE13_D_TYPE_SELf},
        { SLICE14_S_TYPE_SELf, SLICE14_D_TYPE_SELf},
        { SLICE15_S_TYPE_SELf, SLICE15_D_TYPE_SELf}
    };
    static const soc_field_t ifp_en_slice_wide_mode_field[] = {
        SLICE0_DOUBLE_WIDE_MODEf,
        SLICE1_DOUBLE_WIDE_MODEf,
        SLICE2_DOUBLE_WIDE_MODEf,
        SLICE3_DOUBLE_WIDE_MODEf,
        SLICE4_DOUBLE_WIDE_MODEf,
        SLICE5_DOUBLE_WIDE_MODEf,
        SLICE6_DOUBLE_WIDE_MODEf,
        SLICE7_DOUBLE_WIDE_MODEf,
        SLICE8_DOUBLE_WIDE_MODEf,
        SLICE9_DOUBLE_WIDE_MODEf,
        SLICE10_DOUBLE_WIDE_MODEf,
        SLICE11_DOUBLE_WIDE_MODEf,
        SLICE12_DOUBLE_WIDE_MODEf,
        SLICE13_DOUBLE_WIDE_MODEf,
        SLICE14_DOUBLE_WIDE_MODEf,
        SLICE15_DOUBLE_WIDE_MODEf
    };
    static const soc_field_t ifp_en_double_wide_key[] = {
        SLICE0_DOUBLE_WIDE_KEY_SELECTf,
        SLICE1_DOUBLE_WIDE_KEY_SELECTf,
        SLICE2_DOUBLE_WIDE_KEY_SELECTf,
        SLICE3_DOUBLE_WIDE_KEY_SELECTf,
        SLICE4_DOUBLE_WIDE_KEY_SELECTf,
        SLICE5_DOUBLE_WIDE_KEY_SELECTf,
        SLICE6_DOUBLE_WIDE_KEY_SELECTf,
        SLICE7_DOUBLE_WIDE_KEY_SELECTf,
        SLICE8_DOUBLE_WIDE_KEY_SELECTf,
        SLICE9_DOUBLE_WIDE_KEY_SELECTf,
        SLICE10_DOUBLE_WIDE_KEY_SELECTf,
        SLICE11_DOUBLE_WIDE_KEY_SELECTf,
        SLICE12_DOUBLE_WIDE_KEY_SELECTf,
        SLICE13_DOUBLE_WIDE_KEY_SELECTf,
        SLICE14_DOUBLE_WIDE_KEY_SELECTf,
        SLICE15_DOUBLE_WIDE_KEY_SELECTf
    };
    static const soc_field_t ifp_en_slice_pairing_field[] = {
        SLICE1_0_PAIRINGf,
        SLICE3_2_PAIRINGf,
        SLICE5_4_PAIRINGf,
        SLICE7_6_PAIRINGf,
        SLICE9_8_PAIRINGf,
        SLICE11_10_PAIRINGf,
        SLICE13_12_PAIRINGf,
        SLICE15_14_PAIRINGf
    };
    static const soc_field_t ifp_en_ing_f4_reg[] = {
        SLICE_0_F4f,
        SLICE_1_F4f,
        SLICE_2_F4f,
        SLICE_3_F4f,
        SLICE_4_F4f,
        SLICE_5_F4f,
        SLICE_6_F4f,
        SLICE_7_F4f,
        SLICE_8_F4f,
        SLICE_9_F4f,
        SLICE_10_F4f,
        SLICE_11_F4f,
        SLICE_12_F4f,
        SLICE_13_F4f,
        SLICE_14_F4f,
        SLICE_15_F4f
    };

    int idx, slice_idx, vslice_idx,index_min, index_max, rv = BCM_E_NONE;
    int group_found, mem_sz, parts_count, slice_ent_cnt;
    int i, pri_tcam_idx, part_index, slice_number, expanded[16];
    int prio, prev_prio, multigroup, max, master_slice;
    bcm_field_entry_t eid;
    bcm_field_stat_t sid;
    bcm_policer_t pid;
    uint16 version;
    uint32 *fp_tcam_buf = NULL; /* Buffer to read the FP_TCAM table */
    char *fp_pfs_buf = NULL; /* Buffer to read the FP_PORT_FIELD_SEL table */
    char *fp_policy_buf = NULL; /* Buffer to read the FP_POLICY table */
    char *fp_global_mask_tcam_buf = NULL; /* Buffer to read the FP_GLOBAL_MASK_TCAM table */
    uint32 rval, paired, intraslice;
    uint32 temp;
    soc_field_t fld;
    bcm_port_t port;
    fp_port_field_sel_entry_t *pfs_entry;
    fp_tcam_entry_t *tcam_entry;
    fp_policy_table_entry_t *policy_entry;
    fp_global_mask_tcam_entry_t *global_mask_tcam_entry;
    _field_hw_qual_info_t hw_sels;
    _field_slice_t *fs;
    _field_group_t *fg;
    _field_entry_t *f_ent = NULL;
    bcm_pbmp_t entry_pbmp, entry_mask_pbmp, temp_pbmp;
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1];
    bcm_pbmp_t port_cmic_pbmp;
    int8 s_field, d_field;
    int phys_tcam_idx;
    uint8 old_physical_slice, slice_num;
    uint32 entry_flags;
    _field_table_pointers_t *fp_table_pointers = NULL;

#if defined(BCM_TRIDENT_SUPPORT) \
    || defined(BCM_SCORPION_SUPPORT) \
    || defined(BCM_CONQUEROR_SUPPORT)
    bcm_pbmp_t temp_pbm_mask; /* Y-Pipe IPBM MASK value. */
#endif
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_CONQUEROR_SUPPORT)
    uint32  tcam_dual_pipe_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Y-Pipe FP_TCAM
                                                            * entry. */
#endif
#if defined(BCM_TRIDENT_SUPPORT)
    char *fp_gm_tcam_x_buf = NULL;                     /* Buffer to read
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * table. */
    fp_global_mask_tcam_x_entry_t *fp_gm_tcam_x_entry; /* Pointer to
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * entry */
    char *fp_gm_tcam_y_buf = NULL;                     /* Buffer to read
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * table. */
    fp_global_mask_tcam_y_entry_t *fp_gm_tcam_y_entry; /* Pointer to
                                                        * FP_GLOBAL_MASK_TCAM_Y
                                                        * entry */
#endif

    BCM_PBMP_ASSIGN(port_cmic_pbmp, PBMP_PORT_ALL(unit));
    BCM_PBMP_OR(port_cmic_pbmp, PBMP_CMIC(unit));
    BCM_PBMP_CLEAR(entry_pbmp);
    BCM_PBMP_CLEAR(entry_mask_pbmp);

    fc->scache_pos = 0;
    fc->scache_pos1 = 0;

    if (fc->l2warm) {
        sal_memcpy(&version, fc->scache_ptr[_FIELD_SCACHE_PART_0], sizeof(uint16));
        if (version > BCM_WB_DEFAULT_VERSION) {
            /* Notify the application with an event */
            /* The application will then need to reconcile the
               version differences using the documented behavioral
               differences on per module (handle) basis */
            SOC_IF_ERROR_RETURN
                (soc_event_generate(unit, SOC_SWITCH_EVENT_WARM_BOOT_DOWNGRADE,
                                    BCM_MODULE_FIELD, version,
                                    BCM_WB_DEFAULT_VERSION));
        }

        fc->scache_pos += SOC_WB_SCACHE_CONTROL_SIZE;

        if(NULL != fc->scache_ptr[_FIELD_SCACHE_PART_1]) {
            sal_memcpy(&version, fc->scache_ptr[_FIELD_SCACHE_PART_1], sizeof(uint16));
            if (version > BCM_WB_DEFAULT_VERSION) {
                /* Notify the application with an event */
                /* The application will then need to reconcile the
                   version differences using the documented behavioral
                   differences on per module (handle) basis */
                SOC_IF_ERROR_RETURN
                    (soc_event_generate(unit,
                                        SOC_SWITCH_EVENT_WARM_BOOT_DOWNGRADE,
                                        BCM_MODULE_FIELD, version,
                                        BCM_WB_DEFAULT_VERSION));
            }

            fc->scache_pos1 += SOC_WB_SCACHE_CONTROL_SIZE;
        }
    }

    SOC_PBMP_CLEAR(entry_pbmp);
    sal_memset(expanded, 0, 16 * sizeof(int));

    if (fc->l2warm) {
        rv = _field_scache_stage_hdr_chk(fc, _FIELD_IFP_DATA_START);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }
    }

    fp_table_pointers = soc_cm_salloc(unit, sizeof(_field_table_pointers_t),
                                                "FP Table buffer Pointers");

    if (NULL == fp_table_pointers) {
        return BCM_E_MEMORY;
    }
    rv = _field_table_pointers_init(unit, fp_table_pointers);
    BCM_IF_ERROR_RETURN(rv);
 
    /* DMA various tables */

    fp_tcam_buf = soc_cm_salloc(unit, sizeof(fp_tcam_entry_t) *
                                soc_mem_index_count(unit, FP_TCAMm),
                                "FP TCAM buffer");
    if (NULL == fp_tcam_buf) {
        return BCM_E_MEMORY;
    }
    sal_memset(fp_tcam_buf, 0, sizeof(fp_tcam_entry_t) *
               soc_mem_index_count(unit, FP_TCAMm));
    index_min = soc_mem_index_min(unit, FP_TCAMm);
    index_max = soc_mem_index_max(unit, FP_TCAMm);
    fs = stage_fc->slices;
    if (stage_fc->flags & _FP_STAGE_HALF_SLICE) {
        slice_ent_cnt = fs->entry_count * 2;
        /* DMA in chunks */
        for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
            fs = stage_fc->slices + slice_idx;
            if ((rv = soc_mem_read_range(unit, FP_TCAMm, MEM_BLOCK_ALL,
                                         slice_idx * slice_ent_cnt,
                                         slice_idx * slice_ent_cnt +
                                             fs->entry_count / 2 - 1,
                                         fp_tcam_buf + slice_idx *
                                             slice_ent_cnt *
                                   soc_mem_entry_words(unit, FP_TCAMm))) < 0 ) {
                goto cleanup;
            }
            if ((rv = soc_mem_read_range(unit, FP_TCAMm, MEM_BLOCK_ALL,
                                         slice_idx * slice_ent_cnt +
                                         fs->entry_count,
                                         slice_idx * slice_ent_cnt +
                                         fs->entry_count +
                                             fs->entry_count / 2 - 1,
                                         fp_tcam_buf + (slice_idx *
                                         slice_ent_cnt + fs->entry_count) *
                                    soc_mem_entry_words(unit, FP_TCAMm))) < 0 ) {
                goto cleanup;
            }
        }
    } else {
        slice_ent_cnt = fs->entry_count;
        if ((rv = soc_mem_read_range(unit, FP_TCAMm, MEM_BLOCK_ALL,
                                     index_min, index_max, fp_tcam_buf)) < 0 ) {
            goto cleanup;
        }
    }
    
    fp_pfs_buf = soc_cm_salloc(unit, SOC_MEM_TABLE_BYTES(unit,
                               FP_PORT_FIELD_SELm),
                               "FP PORT_FIELD_SEL buffer");
    if (NULL == fp_pfs_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    index_min = soc_mem_index_min(unit, FP_PORT_FIELD_SELm);
    index_max = soc_mem_index_max(unit, FP_PORT_FIELD_SELm);
    if ((rv = soc_mem_read_range(unit, FP_PORT_FIELD_SELm, MEM_BLOCK_ALL,
                                 index_min, index_max, fp_pfs_buf)) < 0 ) {
        goto cleanup;
    }

    fp_policy_buf = soc_cm_salloc(unit, SOC_MEM_TABLE_BYTES
                                  (unit, FP_POLICY_TABLEm),
                                  "FP POLICY TABLE buffer");
    if (NULL == fp_policy_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    index_min = soc_mem_index_min(unit, FP_POLICY_TABLEm);
    index_max = soc_mem_index_max(unit, FP_POLICY_TABLEm);
    if ((rv = soc_mem_read_range(unit, FP_POLICY_TABLEm, MEM_BLOCK_ALL,
                               index_min, index_max, fp_policy_buf)) < 0 ) {
        goto cleanup;
    }

    if (soc_mem_is_valid(unit, FP_GLOBAL_MASK_TCAMm)) {
#if defined(BCM_TRIDENT_SUPPORT)
        /* Allocate buffer to DMA X/Y-Pipe GM TCAM table entries. */
        if (SOC_IS_TD_TT(unit)) {
            fp_gm_tcam_x_buf = soc_cm_salloc(unit,
                                             SOC_MEM_TABLE_BYTES
                                             (unit,
                                             FP_GLOBAL_MASK_TCAM_Xm),
                                             "FP_GLOBAL_MASK_TCAM X-buffer"
                                             );
            if (fp_gm_tcam_x_buf == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }

            index_min = soc_mem_index_min(unit, FP_GLOBAL_MASK_TCAM_Xm);
            index_max = soc_mem_index_max(unit, FP_GLOBAL_MASK_TCAM_Xm);
            if ((rv = soc_mem_read_range(unit,
                                         FP_GLOBAL_MASK_TCAM_Xm,
                                         MEM_BLOCK_ALL,
                                         index_min,
                                         index_max,
                                         fp_gm_tcam_x_buf
                                         )) < 0) {
                goto cleanup;
            }

            fp_gm_tcam_y_buf = soc_cm_salloc(unit,
                                             SOC_MEM_TABLE_BYTES
                                             (unit,
                                             FP_GLOBAL_MASK_TCAM_Ym),
                                             "FP_GLOBAL_MASK_TCAM Y-buffer"
                                             );
            if (fp_gm_tcam_y_buf == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }

            index_min = soc_mem_index_min(unit, FP_GLOBAL_MASK_TCAM_Ym);
            index_max = soc_mem_index_max(unit, FP_GLOBAL_MASK_TCAM_Ym);
            if ((rv = soc_mem_read_range(unit,
                                         FP_GLOBAL_MASK_TCAM_Ym,
                                         MEM_BLOCK_ALL,
                                         index_min,
                                         index_max,
                                         fp_gm_tcam_y_buf
                                         )) < 0) {
                goto cleanup;
            }
        } else
#endif
        {
            fp_global_mask_tcam_buf = soc_cm_salloc(unit,
                                        SOC_MEM_TABLE_BYTES(
                                        unit,
                                        FP_GLOBAL_MASK_TCAMm),
                                        "FP_GLOBAL_MASK_TCAM buffer");
            if (fp_global_mask_tcam_buf == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            index_min = soc_mem_index_min(unit, FP_GLOBAL_MASK_TCAMm);
            index_max = soc_mem_index_max(unit, FP_GLOBAL_MASK_TCAMm);
            if ((rv = soc_mem_read_range(unit,
                        FP_GLOBAL_MASK_TCAMm,
                        MEM_BLOCK_ALL,
                        index_min,
                        index_max,
                        fp_global_mask_tcam_buf)) < 0) {
                goto cleanup;
            }
        }
    } else {
        /* No global mask TCAM
           => Entries apply to all ports
        */

        BCM_PBMP_ASSIGN(entry_pbmp, PBMP_ALL(unit));
        BCM_PBMP_ASSIGN(entry_mask_pbmp, PBMP_ALL(unit));
#if defined(BCM_KATANA2_SUPPORT)
        if (soc_feature(unit, soc_feature_linkphy_coe) ||
            soc_feature(unit, soc_feature_subtag_coe)) {
            _bcm_kt2_subport_pbmp_update(unit, &entry_pbmp);
            _bcm_kt2_subport_pbmp_update(unit, &entry_mask_pbmp);
        }
#endif

    }

#if defined(BCM_TRIDENT_SUPPORT)
    fp_table_pointers->fp_gm_tcam_x_buf        = fp_gm_tcam_x_buf;
    fp_table_pointers->fp_gm_tcam_y_buf        = fp_gm_tcam_y_buf;
#endif
    fp_table_pointers->fp_global_mask_tcam_buf = fp_global_mask_tcam_buf;

    /* Recover range checkers */
    rv = _field_range_check_reinit(unit, stage_fc, fc);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    /* Recover data qualifiers */
    rv = _field_data_qual_recover(unit, fc, stage_fc);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    /* Get slice expansion status and virtual map */
    if ((rv = _field_tr2_ifp_slice_expanded_status_get(unit,
                                                       stage_fc,
                                                       expanded
                                                       )
         ) < 0
        ) {
        goto cleanup;
    }

    /* Iterate over the slices */
    if ((rv = READ_FP_SLICE_ENABLEr(unit, &rval)) < 0) {
        goto cleanup;
    }
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        /* Ignore disabled slice */
        if ((soc_reg_field_get(unit,
                               FP_SLICE_ENABLEr,
                               rval,
                               ifp_en_flds[slice_idx]
                               )
             == 0
             ) || soc_reg_field_get(unit,
                                    FP_SLICE_ENABLEr,
                                    rval,
                                    ifp_lk_en_flds[slice_idx]
                                    )
            == 0
            ) {
            continue;
        }
        /* Ignore secondary slice in paired mode */
        pfs_entry = soc_mem_table_idx_to_pointer(unit, FP_PORT_FIELD_SELm,
                                                fp_port_field_sel_entry_t *,
                                                fp_pfs_buf, 0);
        fld = ifp_en_slice_pairing_field[slice_idx / 2];
        paired = soc_FP_PORT_FIELD_SELm_field32_get(unit,
                                                    pfs_entry, fld);

        intraslice =
            soc_mem_field32_get_def(unit,
                                    FP_PORT_FIELD_SELm,
                                    pfs_entry,
                                    ifp_en_slice_wide_mode_field[slice_idx],
                                    FALSE
                                    );

        if (paired && (slice_idx % 2)) {
            continue;
        }

        /* Don't need to read selectors for expanded slice */
        if (expanded[slice_idx]) {
            continue;
        }

        /* Skip if slice has no valid groups and entries */
        fs = stage_fc->slices + slice_idx;
        slice_ent_cnt = fs->entry_count;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            if (_bcm_field_slice_offset_to_tcam_idx(unit,
                                                    stage_fc,
                                                    slice_idx,
                                                    idx,
                                                    &phys_tcam_idx
                                                    )
                != BCM_E_NONE
                ) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            tcam_entry = soc_mem_table_idx_to_pointer(unit, FP_TCAMm,
                                                      fp_tcam_entry_t *,
                                                      fp_tcam_buf,
                                                      phys_tcam_idx
                                                      );
            if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) != 0) {
                break;
            }
        }
        if ((idx == slice_ent_cnt) && !fc->l2warm) {
            continue;
        }

        /* If Level 2, retrieve the GIDs in this slice */
        if (fc->l2warm) {
            rv = _field_trx_scache_slice_group_recover(unit,
                                                       fc,
                                                       slice_idx,
                                                       &multigroup,
                                                       stage_fc
                                                       );
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                fc->l2warm = 0;
                goto cleanup;
            }
            if (rv == BCM_E_NOT_FOUND) {
                rv = BCM_E_NONE;
                continue;
            }
        }

        /* Iterate over FP_PORT_FIELD_SEL for all ports and this slice
         * to identify bins for selectors (also groups for level 1) */
        PBMP_ALL_ITER(unit, port) {
            if (IS_LB_PORT(unit, port)) {
                continue;
            }

            _FIELD_SELCODE_CLEAR(hw_sels.pri_slice[0]);
            hw_sels.pri_slice[0].intraslice
                = hw_sels.pri_slice[0].secondary = _FP_SELCODE_DONT_USE;
            _FIELD_SELCODE_CLEAR(hw_sels.pri_slice[1]);
            hw_sels.pri_slice[1].intraslice
                = hw_sels.pri_slice[1].secondary = _FP_SELCODE_DONT_USE;
            _FIELD_SELCODE_CLEAR(hw_sels.sec_slice[0]);
            hw_sels.sec_slice[0].intraslice
                = hw_sels.sec_slice[0].secondary = _FP_SELCODE_DONT_USE;
            _FIELD_SELCODE_CLEAR(hw_sels.sec_slice[1]);
            hw_sels.sec_slice[1].intraslice
                = hw_sels.sec_slice[1].secondary = _FP_SELCODE_DONT_USE;

            pfs_entry = soc_mem_table_idx_to_pointer
                            (unit, FP_PORT_FIELD_SELm,
                             fp_port_field_sel_entry_t *, fp_pfs_buf, port);
            /* Get primary slice's selectors */
            hw_sels.pri_slice[0].fpf1 = soc_FP_PORT_FIELD_SELm_field32_get(
                                            unit,
                                            pfs_entry,
                                            ifp_en_field_tbl[slice_idx][0]
                                                                           );
            hw_sels.pri_slice[0].fpf2 = soc_FP_PORT_FIELD_SELm_field32_get(
                                            unit,
                                            pfs_entry,
                                            ifp_en_field_tbl[slice_idx][1]
                                                                           );
            hw_sels.pri_slice[0].fpf3 = soc_FP_PORT_FIELD_SELm_field32_get(
                                            unit,
                                            pfs_entry,
                                            ifp_en_field_tbl[slice_idx][2]
                                                                           );
            /* Retrieve Tunnel Type information */
            if (SOC_IS_TRIUMPH2(unit)
                || SOC_IS_APOLLO(unit)
                || SOC_IS_VALKYRIE2(unit)
                || SOC_IS_ENDURO(unit)
                ) {
                rv = _field_tr2_loopback_type_sel_recover
                        (unit,
                         slice_idx,
                         fp_tcam_buf,
                         stage_fc,
                         intraslice,
                         &(hw_sels.pri_slice[0].loopback_type_sel)
                         );
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
            }

            if (soc_mem_field_valid(unit, FP_PORT_FIELD_SELm, SLICE0_S_TYPE_SELf)) {
                s_field = soc_FP_PORT_FIELD_SELm_field32_get(
                              unit,
                              pfs_entry,
                              ifp_en_sd_type_field_tbl[slice_idx][0]
                                                             );
                d_field = soc_FP_PORT_FIELD_SELm_field32_get(
                              unit,
                              pfs_entry,
                              ifp_en_sd_type_field_tbl[slice_idx][1]
                                                             );
                if ((hw_sels.pri_slice[0].fpf1 == 0) ||
                    (hw_sels.pri_slice[0].fpf3 == 0)) {
                    /* Check for L3_IIF or SVP */
                    _field_tr2_ingress_entity_get(unit, slice_idx, fp_tcam_buf,
                                                  slice_ent_cnt, stage_fc,
                        &(hw_sels.pri_slice[0].ingress_entity_sel));
                }
            } else {
                /* Device (Triumph, for one) does not support source and destination qualification
                   => Use a default value
                */

                s_field = d_field = -1;
            }

            switch (s_field) {
            case 1:
                hw_sels.pri_slice[0].src_entity_sel = _bcmFieldFwdEntityGlp;
                break;
            case 2:
                hw_sels.pri_slice[0].src_entity_sel =
                    _bcmFieldFwdEntityModPortGport;
                break;
            case 3:
                /* Need to adjust later based on actual VP */
                hw_sels.pri_slice[0].src_entity_sel =
                    _bcmFieldFwdEntityMimGport;
                break;
            default:
                hw_sels.pri_slice[0].src_entity_sel = _bcmFieldFwdEntityAny;
                break;
            }

            switch (d_field) {
            case 0:
                hw_sels.pri_slice[0].dst_fwd_entity_sel =
                        _bcmFieldFwdEntityGlp;
                break;
            case 1:
                hw_sels.pri_slice[0].dst_fwd_entity_sel =
                    _bcmFieldFwdEntityL3Egress;
                break;
            case 2:
                hw_sels.pri_slice[0].dst_fwd_entity_sel =
                                _bcmFieldFwdEntityMulticastGroup;
                break;
            case 3:
                /* Need to adjust later based on actual VP */
                hw_sels.pri_slice[0].dst_fwd_entity_sel =
                    _bcmFieldFwdEntityMimGport;
                break;
            case 7:
                hw_sels.pri_slice[0].dst_fwd_entity_sel = 
                                     _bcmFieldFwdEntityMultipath;
                break;
            default:
                hw_sels.pri_slice[0].dst_fwd_entity_sel = _bcmFieldFwdEntityAny;
                break;
            }

            _field_tr2_slice_key_control_entry_recover(unit,
                                                       slice_idx,
                                                       &hw_sels.pri_slice[0]
                                                       );

            /* If intraslice, get double-wide key */
            if (intraslice) {
                uint32 dwf4sel;

                hw_sels.pri_slice[1].intraslice = TRUE;
                hw_sels.pri_slice[1].fpf2
                    = soc_mem_field32_get(unit,
                                          FP_PORT_FIELD_SELm,
                                          pfs_entry,
                                          ifp_en_double_wide_key[slice_idx]
                                          );

                READ_FP_DOUBLE_WIDE_F4_SELECTr(unit, &dwf4sel);
                hw_sels.pri_slice[1].fpf4
                    = soc_reg_field_get(unit,
                                        FP_DOUBLE_WIDE_F4_SELECTr,
                                        dwf4sel,
                                        ifp_en_ing_f4_reg[slice_idx]
                                        );

                if (soc_mem_field_valid(unit, FP_PORT_FIELD_SELm,
                                        SLICE0_S_TYPE_SELf)) {
                    d_field = soc_mem_field32_get(unit,
                                          FP_PORT_FIELD_SELm,
                                          pfs_entry,
                                          ifp_en_sd_type_field_tbl[slice_idx][1]
                                          );
                    switch (d_field) {
                        case 0:
                            hw_sels.pri_slice[1].dst_fwd_entity_sel =
                                _bcmFieldFwdEntityGlp;
                            break;
                        case 1:
                            hw_sels.pri_slice[1].dst_fwd_entity_sel =
                                _bcmFieldFwdEntityL3Egress;
                            break;
                        case 2:
                            hw_sels.pri_slice[1].dst_fwd_entity_sel =
                                _bcmFieldFwdEntityMulticastGroup;
                            break;
                        case 3:
                            /* Need to adjust later based on actual VP */
                            hw_sels.pri_slice[1].dst_fwd_entity_sel =
                                _bcmFieldFwdEntityMimGport;
                            break;
                        case 7:
                            hw_sels.pri_slice[1].dst_fwd_entity_sel = 
                                     _bcmFieldFwdEntityMultipath;
                            break;
                        default:
                            hw_sels.pri_slice[1].dst_fwd_entity_sel =
                                _bcmFieldFwdEntityGlp;
                        break;
                    }
                }
            }
            /* If paired, get secondary slice's selectors */
            if (paired) {
                hw_sels.sec_slice[0].secondary = _FP_SELCODE_DONT_CARE;

                hw_sels.sec_slice[0].fpf1 = soc_FP_PORT_FIELD_SELm_field32_get
                                         (unit, pfs_entry,
                                          ifp_en_field_tbl[slice_idx + 1][0]);
                hw_sels.sec_slice[0].fpf2 = soc_FP_PORT_FIELD_SELm_field32_get
                                         (unit, pfs_entry,
                                          ifp_en_field_tbl[slice_idx + 1][1]);
                hw_sels.sec_slice[0].fpf3 = soc_FP_PORT_FIELD_SELm_field32_get
                                         (unit, pfs_entry,
                                          ifp_en_field_tbl[slice_idx + 1][2]);

                if (soc_mem_field_valid(unit, FP_PORT_FIELD_SELm,
                                        SLICE0_S_TYPE_SELf)) {
                    s_field = soc_FP_PORT_FIELD_SELm_field32_get(
                                  unit,
                                  pfs_entry,
                                  ifp_en_sd_type_field_tbl[slice_idx + 1][0]
                                                             );
                    d_field = soc_FP_PORT_FIELD_SELm_field32_get(
                                  unit,
                                  pfs_entry,
                                  ifp_en_sd_type_field_tbl[slice_idx + 1][1]
                                                             );
                    if ((hw_sels.sec_slice[0].fpf1 == 0) ||
                        (hw_sels.sec_slice[0].fpf3 == 0)) {
                        /* Check for L3_IIF or SVP */
                        _field_tr2_ingress_entity_get(unit, slice_idx,
                                                      fp_tcam_buf,
                                                      slice_ent_cnt, stage_fc,
                            &(hw_sels.pri_slice[0].ingress_entity_sel));
                    }
                }
                switch (s_field) {
                case 1:
                    hw_sels.sec_slice[0].src_entity_sel = _bcmFieldFwdEntityGlp;
                    break;
                case 2:
                    hw_sels.sec_slice[0].src_entity_sel =
                                                 _bcmFieldFwdEntityModPortGport;
                    break;
                case 3:
                    /* Need to adjust later based on actual VP */
                    hw_sels.sec_slice[0].src_entity_sel =
                                                     _bcmFieldFwdEntityMimGport;
                    break;
                 default:
                    hw_sels.sec_slice[0].src_entity_sel = _bcmFieldFwdEntityAny;
                    break;
                }

                switch (d_field) {
                case 0:
                    hw_sels.sec_slice[0].dst_fwd_entity_sel =
                                                 _bcmFieldFwdEntityGlp;
                    break;
                case 1:
                    hw_sels.sec_slice[0].dst_fwd_entity_sel =
                                                 _bcmFieldFwdEntityL3Egress;
                    break;
                case 2:
                    hw_sels.pri_slice[0].dst_fwd_entity_sel =
                                            _bcmFieldFwdEntityMulticastGroup;
                    break;
                case 3:
                    /* Need to adjust later based on actual VP */
                    hw_sels.sec_slice[0].dst_fwd_entity_sel =
                                                     _bcmFieldFwdEntityMimGport;
                    break;
                case 7:
                    hw_sels.pri_slice[0].dst_fwd_entity_sel = 
                                     _bcmFieldFwdEntityMultipath;
                            break;
                 default:
                    hw_sels.sec_slice[0].dst_fwd_entity_sel =
                                                          _bcmFieldFwdEntityAny;
                    break;
                }

                _field_tr2_slice_key_control_entry_recover(unit,
                                                           slice_idx,
                                                           &hw_sels.sec_slice[0]
                                                           );

                if (intraslice) {
                    uint32 dwf4sel;

                    hw_sels.sec_slice[1].intraslice = TRUE;
                    hw_sels.sec_slice[1].fpf2
                        = soc_mem_field32_get(unit,
                                           FP_PORT_FIELD_SELm,
                                           pfs_entry,
                                           ifp_en_double_wide_key[slice_idx + 1]
                                           );

                    READ_FP_DOUBLE_WIDE_F4_SELECTr(unit, &dwf4sel);
                    hw_sels.sec_slice[1].fpf4
                        = soc_reg_field_get(unit,
                                            FP_DOUBLE_WIDE_F4_SELECTr,
                                            dwf4sel,
                                            ifp_en_ing_f4_reg[slice_idx + 1]
                                            );

                    if (soc_mem_field_valid(unit, FP_PORT_FIELD_SELm,
                                            SLICE0_S_TYPE_SELf)) {
                        d_field = soc_mem_field32_get(unit,
                                          FP_PORT_FIELD_SELm,
                                          pfs_entry,
                                          ifp_en_sd_type_field_tbl[slice_idx][1]
                                          );
                        switch (d_field) {
                        case 0:
                            hw_sels.sec_slice[1].dst_fwd_entity_sel =
                                                     _bcmFieldFwdEntityGlp;
                            break;
                        case 1:
                            hw_sels.sec_slice[1].dst_fwd_entity_sel =
                                                     _bcmFieldFwdEntityL3Egress;
                            break;
                        case 2:
                            hw_sels.sec_slice[1].dst_fwd_entity_sel =
                                               _bcmFieldFwdEntityMulticastGroup;
                            break;
                        case 3:
                            /* Need to adjust later based on actual VP */
                            hw_sels.sec_slice[1].dst_fwd_entity_sel =
                                                     _bcmFieldFwdEntityMimGport;
                            break;
                        case 7:
                            hw_sels.pri_slice[1].dst_fwd_entity_sel = 
                                     _bcmFieldFwdEntityMultipath;
                            break;
                        default:
                            hw_sels.sec_slice[1].dst_fwd_entity_sel =
                                                          _bcmFieldFwdEntityGlp;
                            break;
                        }
                    }
                }
            }
            /* Create a group based on HW qualifiers (or find existing) */
            rv = _field_tr2_group_construct(unit,
                                            &hw_sels,
                                            intraslice,
                                            paired,
                                            fc,
                                            port,
                                            _BCM_FIELD_STAGE_INGRESS,
                                            slice_idx
                                            );
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }

        /* Now go over the entries in this slice */
        fs = stage_fc->slices + slice_idx;
        slice_ent_cnt = fs->entry_count;
        if (fs->group_flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
            slice_ent_cnt >>= 1;
            fs->free_count >>= 1;
        }
        prev_prio = -1;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            group_found = 0;

            if (_bcm_field_slice_offset_to_tcam_idx(unit,
                                                    stage_fc,
                                                    slice_idx,
                                                    idx,
                                                    &phys_tcam_idx
                                                    )
                != BCM_E_NONE
                ) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            tcam_entry = soc_mem_table_idx_to_pointer(unit, FP_TCAMm,
                                                      fp_tcam_entry_t *,
                                                      fp_tcam_buf,
                                                      phys_tcam_idx
                                                      );
            if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) == 0) {
                continue;
            }

            /* Check which ports are applicable to this entry */

            if (soc_mem_is_valid(unit, FP_GLOBAL_MASK_TCAMm)) {
#if defined (BCM_TRIDENT_SUPPORT)
                if (SOC_IS_TD_TT(unit)) {
                    /* Get X-Pipe IPBM KEY and MASK value. */
                    fp_gm_tcam_x_entry = soc_mem_table_idx_to_pointer(
                                            unit,
                                            FP_GLOBAL_MASK_TCAM_Xm,
                                            fp_global_mask_tcam_x_entry_t *,
                                            fp_gm_tcam_x_buf,
                                            phys_tcam_idx);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Xm,
                                           fp_gm_tcam_x_entry,
                                           IPBMf,
                                           &entry_pbmp);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Xm,
                                           fp_gm_tcam_x_entry,
                                           IPBM_MASKf,
                                           &entry_mask_pbmp);

                    /* Get Y-Pipe IPBM KEY and MASK value. */
                    fp_gm_tcam_y_entry = soc_mem_table_idx_to_pointer(
                                            unit,
                                            FP_GLOBAL_MASK_TCAM_Ym,
                                            fp_global_mask_tcam_y_entry_t *,
                                            fp_gm_tcam_y_buf,
                                            phys_tcam_idx);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Ym,
                                           fp_gm_tcam_y_entry,
                                           IPBMf,
                                           &temp_pbmp);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Ym,
                                           fp_gm_tcam_y_entry,
                                           IPBM_MASKf,
                                           &temp_pbm_mask);
                    SOC_PBMP_OR(entry_pbmp, temp_pbmp);
                    SOC_PBMP_OR(entry_mask_pbmp, temp_pbm_mask);
                } else
#endif
                {
                    global_mask_tcam_entry = soc_mem_table_idx_to_pointer(
                                                unit,
                                                FP_GLOBAL_MASK_TCAMm,
                                                fp_global_mask_tcam_entry_t *,
                                                fp_global_mask_tcam_buf,
                                                phys_tcam_idx);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAMm,
                                           global_mask_tcam_entry,
                                           IPBMf,
                                           &entry_pbmp);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAMm,
                                           global_mask_tcam_entry,
                                           IPBM_MASKf,
                                           &entry_mask_pbmp);
                }
            } else {
                soc_mem_pbmp_field_get(unit,
                                       FP_TCAMm,
                                       tcam_entry,
                                       IPBMf,
                                       &entry_pbmp
                                       );
                soc_mem_pbmp_field_get(unit,
                                       FP_TCAMm,
                                       tcam_entry,
                                       IPBM_MASKf,
                                       &entry_mask_pbmp
                                       );
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_CONQUEROR_SUPPORT)
                /*
                 * Scorpion and Conqurer are dual pipe line devices.
                 * Retrieve IPBMf and IPBM_MASKf field values from Y Pipleline
                 * FP_TCAM table.
                 */
                if (SOC_IS_SC_CQ(unit)) {
                    sal_memset(&tcam_dual_pipe_entry, 0,
                        sizeof(tcam_dual_pipe_entry));
                    rv = soc_mem_read(unit, FP_TCAM_Ym, MEM_BLOCK_ANY,
                            phys_tcam_idx, tcam_dual_pipe_entry);
                    if (BCM_FAILURE(rv)) {
                        goto cleanup;
                    }
                    soc_mem_pbmp_field_get(unit,
                                           FP_TCAM_Ym,
                                           tcam_dual_pipe_entry,
                                           IPBMf,
                                           &temp_pbmp
                                           );
                    soc_mem_pbmp_field_get(unit,
                                           FP_TCAM_Ym,
                                           tcam_dual_pipe_entry,
                                           IPBM_MASKf,
                                           &temp_pbm_mask
                                           );
                    SOC_PBMP_OR(entry_pbmp, temp_pbmp);
                    SOC_PBMP_OR(entry_mask_pbmp, temp_pbm_mask);
                }
#endif
            }

            /* Search groups to find match */
            fg = fc->groups;
            while (fg != NULL) {
                /* Check if group is in this slice */
                fs = &fg->slices[0];
                if (fs->slice_number != slice_idx) {
                    fg = fg->next;
                    continue;
                }

                /* Check if entry_pbmp is a subset of group pbmp */

                SOC_PBMP_CLEAR(temp_pbmp);
                SOC_PBMP_ASSIGN(temp_pbmp, fg->pbmp);
                SOC_PBMP_AND(temp_pbmp, entry_pbmp);
                if (SOC_PBMP_EQ(temp_pbmp, entry_pbmp)) {
                    group_found = 1;
                    break;
                }

                fg = fg->next;
            }
            if (!group_found) {
                return BCM_E_INTERNAL; /* Should never happen */
            }

            /* Allocate memory for the entry */
            rv = _bcm_field_entry_tcam_parts_count(unit, fg->flags,
                                                   &parts_count);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            mem_sz = parts_count * sizeof (_field_entry_t);
            _FP_XGS3_ALLOC(f_ent, mem_sz, "field entry");
            if (f_ent == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            sid = pid = -1;
            if (fc->l2warm) {
                rv = _field_trx_entry_info_retrieve(unit,
                                                    &eid,
                                                    &prio,
                                                    fc,
                                                    multigroup,
                                                    &prev_prio,
                                                    &sid,
                                                    &pid,
                                                    stage_fc
                                                    );
                
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
            } else {
                _bcm_field_last_alloc_eid_incr();
            }
            pri_tcam_idx = phys_tcam_idx;

            vslice_idx = _field_physical_to_virtual(unit, slice_idx, stage_fc);
            if (vslice_idx == -1) {
                rv = BCM_E_INTERNAL;
                sal_free(f_ent);
                goto cleanup;
            }

            for (i = 0; i < parts_count; i++) {
                if (fc->l2warm) {
                    /* Use retrieved EID and priority */
                    f_ent[i].eid = eid;
                    /* 
                     * In warm boot level 2, during sync, entry priority is 
                     * written only for the first part of the entry and during 
                     * recovery, the priority value from the first part is 
                     * assigned to all parts of entry 
                     */ 
                    f_ent[i].prio = prio;
                } else {
                    f_ent[i].eid = _bcm_field_last_alloc_eid_get();
                    /* 
                     * For warm boot level 1, priority derived from vslice_idx 
                     * is assigned to all parts of the entry 
                     */
                    f_ent[i].prio = (vslice_idx << 10) | (slice_ent_cnt - idx);
                }
                f_ent[i].group = fg;
                if (i == 0) {
                    BCM_PBMP_ASSIGN(f_ent[i].pbmp.data, entry_pbmp);
                    BCM_PBMP_ASSIGN(f_ent[i].pbmp.mask, entry_mask_pbmp);
                }

                if (fc->flags & _FP_COLOR_INDEPENDENT) {
                    f_ent[i].flags |= _FP_ENTRY_COLOR_INDEPENDENT;
                }
                rv = _bcm_field_tcam_part_to_entry_flags(i, fg->flags,
                                                         &f_ent[i].flags);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_entry_part_tcam_idx_get(unit, f_ent,
                                                        pri_tcam_idx,
                                                        i, &part_index);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_tcam_idx_to_slice_offset(unit, stage_fc,
                                                         part_index,
                                                         &slice_number,
                                                (int *)&f_ent[i].slice_idx);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                f_ent[i].fs = stage_fc->slices + slice_number;
                BCM_PBMP_OR(f_ent[i].fs->pbmp, fg->pbmp);
                if (0 == (f_ent[i].flags & _FP_ENTRY_SECOND_HALF)) {
                    /* Decrement slice free entry count for primary
                       entries. */
                    f_ent[i].fs->free_count--;
                }
                /* Assign entry to a slice */
                f_ent[i].fs->entries[f_ent[i].slice_idx] = f_ent + i;
                f_ent[i].flags |= _FP_ENTRY_INSTALLED;

                if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) == 3) {
                    f_ent[i].flags |= _FP_ENTRY_ENABLED;
                }

                if (SOC_IS_TD_TT(unit)) {
                    if (paired && i == 1) {
                        f_ent[i].flags |= _FP_ENTRY_USES_IPBM_OVERLAY;
                    }
                }

                /* Get the actions associated with this part of the entry */

                policy_entry = soc_mem_table_idx_to_pointer(
                                   unit,
                                   FP_POLICY_TABLEm,
                                   fp_policy_table_entry_t *,
                                   fp_policy_buf,
                                   part_index
                                                            );
                rv = _field_tr2_actions_recover(unit,
                                                FP_POLICY_TABLEm,
                                                (uint32 *) policy_entry,
                                                f_ent,
                                                i,
                                                sid,
                                                pid
                                                );
            }

            /* Add to the group */
            rv = _field_group_entry_add(unit, fg, f_ent);
            if (BCM_FAILURE(rv)) {
                sal_free(f_ent);
                goto cleanup;
            }
            f_ent = NULL;
        }

        /* Free up the temporary slice group info */
        if (fc->l2warm) {
            _field_scache_slice_group_free(unit,
                                           fc,
                                           slice_idx
                                           );
        }
    }

    /* Now go over the expanded slices */
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        if (!expanded[slice_idx]) {
            continue;
        }
        /* Ignore secondary slice in paired mode */
        pfs_entry = soc_mem_table_idx_to_pointer(unit, FP_PORT_FIELD_SELm,
                                                fp_port_field_sel_entry_t *,
                                                fp_pfs_buf, 0);
        fld = ifp_en_slice_pairing_field[slice_idx / 2];
        paired = soc_FP_PORT_FIELD_SELm_field32_get(unit,
                                                    pfs_entry, fld);
        if (paired && (slice_idx % 2)) {
            continue;
        }
        /* Skip if slice has no valid entries */
        fs = stage_fc->slices + slice_idx;
        slice_ent_cnt = fs->entry_count;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            if (_bcm_field_slice_offset_to_tcam_idx(unit,
                                                    stage_fc,
                                                    slice_idx,
                                                    idx,
                                                    &phys_tcam_idx
                                                    )
                != BCM_E_NONE
                ) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            tcam_entry = soc_mem_table_idx_to_pointer(unit, FP_TCAMm,
                                                      fp_tcam_entry_t *,
                                                      fp_tcam_buf,
                                                      phys_tcam_idx
                                                      );
            if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) != 0) {
                break;
            }
        }
        if (idx == slice_ent_cnt) {
            continue;
        }
        /* If Level 2, retrieve the GIDs in this slice */
        if (fc->l2warm) {
            rv = _field_scache_slice_group_recover(unit, fc, slice_idx,
                                                       &multigroup);
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                fc->l2warm = 0;
                goto cleanup;
            }
            if (rv == BCM_E_NOT_FOUND) {
                rv = BCM_E_NONE;
                continue;
            }
        }
        /* Now find the master slice for this virtual group */
        vslice_idx = _field_physical_to_virtual(unit, slice_idx, stage_fc);
        if (vslice_idx < 0) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        max = -1;
        for (i = 0; i < stage_fc->tcam_slices; i++) {
            if ((stage_fc->vmap[0][vslice_idx].virtual_group ==
                stage_fc->vmap[0][i].virtual_group) && (i != vslice_idx)) {
                if (i > max) {
                    max = i;
                }
            }
        }
        if (max < 0) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        master_slice = stage_fc->vmap[0][max].vmap_key;
        /* See which group is in this slice - can be only one */
        fg = fc->groups;
        while (fg != NULL) {
            /* Check if group is in this slice */
            fs = &fg->slices[0];
            if (fs->slice_number == master_slice) {
                break;
            }
            fg = fg->next;
        }
        if (fg == NULL) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        /* Get number of entry parts for the group. */
        rv = _bcm_field_entry_tcam_parts_count (unit, fg->flags, &parts_count);
        BCM_IF_ERROR_RETURN(rv);

        old_physical_slice = fs->slice_number;

        /* Set up the new physical slice parameters in Software */
        for(part_index = parts_count - 1; part_index >= 0; part_index--) {
            /* Get entry flags. */
            rv = _bcm_field_tcam_part_to_entry_flags(part_index, fg->flags, &entry_flags);
            BCM_IF_ERROR_RETURN(rv);
    
            /* Get slice id for entry part */
            rv = _bcm_field_tcam_part_to_slice_number(part_index, fg->flags, &slice_num);
            BCM_IF_ERROR_RETURN(rv);
            
            /* Get slice pointer. */
            fs = stage_fc->slices + slice_idx + slice_num;

            if (0 == (entry_flags & _FP_ENTRY_SECOND_HALF)) {
                /* Set per slice configuration &  number of free entries in the slice.*/
                fs->free_count = fs->entry_count;
                if (fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
                    fs->free_count >>= 1;
                }
                /* Set group flags in in slice.*/ 
                fs->group_flags = fg->flags & _FP_GROUP_STATUS_MASK;
    
                /* Add slice to slices linked list . */
                stage_fc->slices[old_physical_slice + slice_num].next = fs;
                fs->prev = &stage_fc->slices[old_physical_slice + slice_num];
            }
        }

        prev_prio = -1;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            if (_bcm_field_slice_offset_to_tcam_idx(unit,
                                                    stage_fc,
                                                    slice_idx,
                                                    idx,
                                                    &phys_tcam_idx
                                                    )
                != BCM_E_NONE
                ) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            tcam_entry = soc_mem_table_idx_to_pointer(unit, FP_TCAMm,
                                                      fp_tcam_entry_t *,
                                                      fp_tcam_buf,
                                                      phys_tcam_idx
                                                      );
            if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) == 0) {
                continue;
            }
            /* Check which ports are applicable to this entry */

            if (soc_mem_is_valid(unit, FP_GLOBAL_MASK_TCAMm)) {
#if defined (BCM_TRIDENT_SUPPORT)
                if (SOC_IS_TD_TT(unit)) {
                    /* Get X-Pipe IPBM KEY and MASK value. */
                    fp_gm_tcam_x_entry = soc_mem_table_idx_to_pointer(
                                            unit,
                                            FP_GLOBAL_MASK_TCAM_Xm,
                                            fp_global_mask_tcam_x_entry_t *,
                                            fp_gm_tcam_x_buf,
                                            phys_tcam_idx);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Xm,
                                           fp_gm_tcam_x_entry,
                                           IPBMf,
                                           &entry_pbmp);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Xm,
                                           fp_gm_tcam_x_entry,
                                           IPBM_MASKf,
                                           &entry_mask_pbmp);

                    /* Get Y-Pipe IPBM KEY and MASK value. */
                    fp_gm_tcam_y_entry = soc_mem_table_idx_to_pointer(
                                            unit,
                                            FP_GLOBAL_MASK_TCAM_Ym,
                                            fp_global_mask_tcam_y_entry_t *,
                                            fp_gm_tcam_y_buf,
                                            phys_tcam_idx);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Ym,
                                           fp_gm_tcam_y_entry,
                                           IPBMf,
                                           &temp_pbmp);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAM_Ym,
                                           fp_gm_tcam_y_entry,
                                           IPBM_MASKf,
                                           &temp_pbm_mask);
                    SOC_PBMP_OR(entry_pbmp, temp_pbmp);
                    SOC_PBMP_OR(entry_mask_pbmp, temp_pbm_mask);
                } else
#endif
                {
                    global_mask_tcam_entry = soc_mem_table_idx_to_pointer(
                                                unit,
                                                FP_GLOBAL_MASK_TCAMm,
                                                fp_global_mask_tcam_entry_t *,
                                                fp_global_mask_tcam_buf,
                                                phys_tcam_idx);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAMm,
                                           global_mask_tcam_entry,
                                           IPBMf,
                                           &entry_pbmp);
                    soc_mem_pbmp_field_get(unit,
                                           FP_GLOBAL_MASK_TCAMm,
                                           global_mask_tcam_entry,
                                           IPBM_MASKf,
                                           &entry_mask_pbmp);
                }
            } else {
                soc_mem_pbmp_field_get(unit,
                                       FP_TCAMm,
                                       tcam_entry,
                                       IPBMf,
                                       &entry_pbmp
                                       );
                soc_mem_pbmp_field_get(unit,
                                       FP_TCAMm,
                                       tcam_entry,
                                       IPBM_MASKf,
                                       &entry_mask_pbmp
                                       );
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_CONQUEROR_SUPPORT)
                /*
                 * Scorpion and Conqurer are dual pipe line devices.
                 * Retrieve IPBMf and IPBM_MASKf field values from Y Pipleline
                 * FP_TCAM table.
                 */
                if (SOC_IS_SC_CQ(unit)) {
                    sal_memset(&tcam_dual_pipe_entry, 0,
                        sizeof(tcam_dual_pipe_entry));
                    rv = soc_mem_read(unit, FP_TCAM_Ym, MEM_BLOCK_ANY,
                            phys_tcam_idx, tcam_dual_pipe_entry);
                    if (BCM_FAILURE(rv)) {
                        goto cleanup;
                    }
                    soc_mem_pbmp_field_get(unit,
                                           FP_TCAM_Ym,
                                           tcam_dual_pipe_entry,
                                           IPBMf,
                                           &temp_pbmp
                                           );
                    soc_mem_pbmp_field_get(unit,
                                           FP_TCAM_Ym,
                                           tcam_dual_pipe_entry,
                                           IPBM_MASKf,
                                           &temp_pbm_mask
                                           );
                    SOC_PBMP_OR(entry_pbmp, temp_pbmp);
                    SOC_PBMP_OR(entry_mask_pbmp, temp_pbm_mask);
                }
#endif
            }

            /* Allocate memory for the entry */
            rv = _bcm_field_entry_tcam_parts_count(unit, fg->flags,
                                                   &parts_count);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            mem_sz = parts_count * sizeof (_field_entry_t);
            _FP_XGS3_ALLOC(f_ent, mem_sz, "field entry");
            if (f_ent == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            sid = pid = -1;
            if (fc->l2warm) {
                rv = _field_trx_entry_info_retrieve(unit,
                                                    &eid,
                                                    &prio,
                                                    fc,
                                                    multigroup,
                                                    &prev_prio,
                                                    &sid,
                                                    &pid,
                                                    stage_fc
                                                    );
                
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
            } else {
                _bcm_field_last_alloc_eid_incr();
            }
            pri_tcam_idx = phys_tcam_idx;
            for (i = 0; i < parts_count; i++) {
                if (fc->l2warm) {
                    /* Use retrieved EID and priority */
                    f_ent[i].eid = eid;
                    /* 
                     * In warm boot level 2, during sync, entry priority is 
                     * written only for the first part of the entry and during
                     * recovery, the priority value from the first part is 
                     * assigned to all parts of entry 
                     */ 
                    f_ent[i].prio = prio;
                } else {
                    f_ent[i].eid = _bcm_field_last_alloc_eid_get();
                    /* 
                     * For warm boot level 1, priority derived from vslice_idx 
                     * is assigned to all parts of the entry 
                     */
                    f_ent[i].prio = (vslice_idx << 10) | (slice_ent_cnt - idx);
                }
                f_ent[i].group = fg;
                if (i == 0) {
                    BCM_PBMP_ASSIGN(f_ent[i].pbmp.data, entry_pbmp);
                    BCM_PBMP_ASSIGN(f_ent[i].pbmp.mask, entry_mask_pbmp);
                }

                if (fc->flags & _FP_COLOR_INDEPENDENT) {
                    f_ent[i].flags |= _FP_ENTRY_COLOR_INDEPENDENT;
                }
                rv = _bcm_field_tcam_part_to_entry_flags(i, fg->flags,
                                                         &f_ent[i].flags);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_entry_part_tcam_idx_get(unit, f_ent,
                                                        pri_tcam_idx,
                                                        i, &part_index);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_tcam_idx_to_slice_offset(unit, stage_fc,
                                                         part_index,
                                                         &slice_number,
                                                (int *)&f_ent[i].slice_idx);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                f_ent[i].fs = stage_fc->slices + slice_number;
                BCM_PBMP_OR(f_ent[i].fs->pbmp, fg->pbmp);
                if (0 == (f_ent[i].flags & _FP_ENTRY_SECOND_HALF)) {
                    /* Decrement slice free entry count for primary
                       entries. */
                    f_ent[i].fs->free_count--;
                }
                /* Assign entry to a slice */
                f_ent[i].fs->entries[f_ent[i].slice_idx] = f_ent + i;
                f_ent[i].flags |= _FP_ENTRY_INSTALLED;

                if (soc_FP_TCAMm_field32_get(unit, tcam_entry, VALIDf) == 3) {
                    f_ent[i].flags |= _FP_ENTRY_ENABLED;
                }

                /* Get the actions associated with this part of the entry */

                policy_entry = soc_mem_table_idx_to_pointer(
                                   unit,
                                   FP_POLICY_TABLEm,
                                   fp_policy_table_entry_t *,
                                   fp_policy_buf,
                                   part_index
                                                            );
                rv = _field_tr2_actions_recover(unit,
                                                FP_POLICY_TABLEm,
                                                (uint32 *) policy_entry,
                                                f_ent,
                                                i,
                                                sid,
                                                pid
                                                );
            }
            /* Add to the group */
            rv = _field_group_entry_add(unit, fg, f_ent);
            if (BCM_FAILURE(rv)) {
                sal_free(f_ent);
                goto cleanup;
            }
            f_ent = NULL;
        }
        /* Free up the temporary slice group info */
        if (fc->l2warm) {
            _field_scache_slice_group_free(unit,
                                           fc,
                                           slice_idx
                                           );
        }
    }
    if (fc->l2warm) {
        temp = 0;
        temp |= buf[fc->scache_pos];
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 8;
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 16;
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 24;
        fc->scache_pos++;
        if (temp != _FIELD_IFP_DATA_END) {
            fc->l2warm = 0;
            rv = BCM_E_INTERNAL;
        }

        if (NULL != buf1) {
            temp = 0;
            temp |= buf1[fc->scache_pos1];
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 8;
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 16;
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 24;
            fc->scache_pos1++;
            if (temp != _FIELD_IFP_DATA_END) {
                fc->l2warm = 0;
                rv = BCM_E_INTERNAL;
            }
        }

    }

      _field_tr2_stage_reinit_all_groups_cleanup(unit, fc,
                                                 _BCM_FIELD_STAGE_INGRESS,
                                                 fp_table_pointers);
cleanup:
    
#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_TD_TT(unit)) {
        if (fp_gm_tcam_x_buf) {
            soc_cm_sfree(unit, fp_gm_tcam_x_buf);
        }
        if (fp_gm_tcam_y_buf) {
            soc_cm_sfree(unit, fp_gm_tcam_y_buf);
        }
    } else
#endif
    {
        if (fp_global_mask_tcam_buf) {
            soc_cm_sfree(unit, fp_global_mask_tcam_buf);
        }
    }

    if (fp_tcam_buf) {
        soc_cm_sfree(unit, fp_tcam_buf);
    }
    if (fp_pfs_buf) {
        soc_cm_sfree(unit, fp_pfs_buf);
    }
    if (fp_policy_buf) {
        soc_cm_sfree(unit, fp_policy_buf);
    }
    if (fp_table_pointers) {
        soc_cm_sfree(unit, fp_table_pointers);
    }

    return rv;
}


static uint16 list_key1[27] = {bcmFieldQualifyL4Ports, bcmFieldQualifyIpFrag,
                      bcmFieldQualifyTcpControl, bcmFieldQualifyL4DstPort,
                      bcmFieldQualifyL4SrcPort, bcmFieldQualifyTtl,
                      bcmFieldQualifyIpProtocol, bcmFieldQualifyDstIp,
                      bcmFieldQualifySrcIp, bcmFieldQualifyTos,
                      bcmFieldQualifyHiGigProxy, bcmFieldQualifyHiGig,
                      bcmFieldQualifyInnerVlanId, bcmFieldQualifyInPort,
                      bcmFieldQualifyL3Routable, bcmFieldQualifyMirrorCopy,
                      bcmFieldQualifyOuterVlan, bcmFieldQualifyOuterVlanId,
                      bcmFieldQualifyOuterVlanCfi, bcmFieldQualifyOuterVlanPri,
                      bcmFieldQualifyVlanFormat, bcmFieldQualifyDstHiGig,
                      bcmFieldQualifyInterfaceClassPort, bcmFieldQualifyOutPort,
                      bcmFieldQualifyIpType, bcmFieldQualifyDrop,
                      bcmFieldQualifyIp4};

static uint16 list_key2[24] = {bcmFieldQualifyL4Ports, bcmFieldQualifyIpProtocol,
                      bcmFieldQualifySrcIp6, bcmFieldQualifyDstIp6,
                      bcmFieldQualifySrcIp6High, bcmFieldQualifyDstIp6High,
                      bcmFieldQualifyTos, bcmFieldQualifyHiGigProxy,
                      bcmFieldQualifyHiGig, bcmFieldQualifyInnerVlanId,
                      bcmFieldQualifyInPort, bcmFieldQualifyL3Routable,
                      bcmFieldQualifyMirrorCopy, bcmFieldQualifyOuterVlan,
                      bcmFieldQualifyOuterVlanId, bcmFieldQualifyOuterVlanCfi,
                      bcmFieldQualifyOuterVlanPri, bcmFieldQualifyVlanFormat,
                      bcmFieldQualifyDstHiGig, bcmFieldQualifyInterfaceClassPort,
                      bcmFieldQualifyOutPort, bcmFieldQualifyIpType,
                      bcmFieldQualifyDrop, bcmFieldQualifyIp6};

static uint16 list_key3[12] = {bcmFieldQualifyL4Ports, bcmFieldQualifyIpFrag,
                      bcmFieldQualifyTcpControl, bcmFieldQualifyL4DstPort,
                      bcmFieldQualifyL4SrcPort, bcmFieldQualifyTtl,
                      bcmFieldQualifyDstIp6,
                      bcmFieldQualifyExtensionHeaderSubCode,
                      bcmFieldQualifyExtensionHeaderType, bcmFieldQualifyIpType,
                      bcmFieldQualifyDrop, bcmFieldQualifyIp6};

static uint16 list_key4[24] = {bcmFieldQualifyL4Ports, bcmFieldQualifyL2Format,
                      bcmFieldQualifyEtherType, bcmFieldQualifySrcMac,
                      bcmFieldQualifyDstMac, bcmFieldQualifyVlanTranslationHit,
                      bcmFieldQualifyHiGigProxy, bcmFieldQualifyHiGig,
                      bcmFieldQualifyInnerVlanId, bcmFieldQualifyInnerVlanCfi,
                      bcmFieldQualifyInnerVlanPri, bcmFieldQualifyInPort,
                      bcmFieldQualifyL3Routable, bcmFieldQualifyMirrorCopy,
                      bcmFieldQualifyOuterVlan, bcmFieldQualifyOuterVlanId,
                      bcmFieldQualifyOuterVlanCfi, bcmFieldQualifyOuterVlanPri,
                      bcmFieldQualifyVlanFormat, bcmFieldQualifyDstHiGig,
                      bcmFieldQualifyInterfaceClassPort, bcmFieldQualifyOutPort,
                      bcmFieldQualifyIpType, bcmFieldQualifyDrop};

static uint16 list_key5[23] = {bcmFieldQualifyL4Ports, bcmFieldQualifyTcpControl,
                      bcmFieldQualifyL4DstPort, bcmFieldQualifyL4SrcPort,
                      bcmFieldQualifyTtl, bcmFieldQualifyIpProtocol,
                      bcmFieldQualifyTos, bcmFieldQualifyHiGigProxy,
                      bcmFieldQualifyHiGig, bcmFieldQualifyInnerVlanId,
                      bcmFieldQualifyInPort, bcmFieldQualifyL3Routable,
                      bcmFieldQualifyMirrorCopy, bcmFieldQualifyOuterVlan,
                      bcmFieldQualifyOuterVlanId, bcmFieldQualifyOuterVlanCfi,
                      bcmFieldQualifyOuterVlanPri, bcmFieldQualifyVlanFormat,
                      bcmFieldQualifyDstHiGig, bcmFieldQualifyInterfaceClassPort,
                      bcmFieldQualifyOutPort, bcmFieldQualifyIpType,
                      bcmFieldQualifyDrop};

int
_field_tr2_stage_egress_reinit(int unit, _field_control_t *fc,
                               _field_stage_t *stage_fc)
{
    int idx, slice_idx, vslice_idx, index_min, index_max, rv = BCM_E_NONE;
    int mem_sz, slice_ent_cnt, parts_count = 1;
    int i, pri_tcam_idx, part_index, slice_number, phys_tcam_idx;
    int prio, prev_prio, expanded[4];
    uint16 *list_arr[2];
    uint32 *efp_tcam_buf = NULL; /* Buffer to read the EFP_TCAM table */
    char *efp_policy_buf = NULL; /* Buffer to read the EFP_POLICY table */
    uint32 rval, efp_slice_mode, efp_key_mode, key_match_type, temp;
    bcm_field_group_t gid;
    int priority, multigroup, max, master_slice;
    bcm_field_entry_t eid;
    bcm_field_stat_t sid;
    bcm_policer_t pid;
    efp_tcam_entry_t *efp_tcam_entry;
    efp_policy_table_entry_t *efp_policy_entry;
    _field_slice_t *fs;
    _field_group_t *fg;
    _field_entry_t *f_ent = NULL;
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1];
    bcm_field_qset_t qset;
    uint8 old_physical_slice, slice_num;
    uint32 entry_flags;
    bcm_pbmp_t all_pbmp;

    soc_field_t efp_en_flds[4] = {SLICE_ENABLE_SLICE_0f, SLICE_ENABLE_SLICE_1f,
                                  SLICE_ENABLE_SLICE_2f, SLICE_ENABLE_SLICE_3f};

    soc_field_t efp_lk_en_flds[4] =
                     {LOOKUP_ENABLE_SLICE_0f, LOOKUP_ENABLE_SLICE_1f,
                      LOOKUP_ENABLE_SLICE_2f, LOOKUP_ENABLE_SLICE_3f};

    sal_memset(expanded, 0, 4 * sizeof(int));

    if (fc->l2warm) {
        rv = _field_scache_stage_hdr_chk(fc, _FIELD_EFP_DATA_START);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }
    }

    /* DMA various tables */
    efp_tcam_buf = soc_cm_salloc(unit, sizeof(efp_tcam_entry_t) *
                                 soc_mem_index_count(unit, EFP_TCAMm),
                                 "EFP TCAM buffer");
    if (NULL == efp_tcam_buf) {
        return BCM_E_MEMORY;
    }
    sal_memset(efp_tcam_buf, 0, sizeof(efp_tcam_entry_t) *
               soc_mem_index_count(unit, EFP_TCAMm));
    index_min = soc_mem_index_min(unit, EFP_TCAMm);
    index_max = soc_mem_index_max(unit, EFP_TCAMm);
    fs = stage_fc->slices;
    if (stage_fc->flags & _FP_STAGE_HALF_SLICE) {
        slice_ent_cnt = fs->entry_count * 2;
        /* DMA in chunks */
        for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
            fs = stage_fc->slices + slice_idx;
            if ((rv = soc_mem_read_range(unit, EFP_TCAMm, MEM_BLOCK_ALL,
                                         slice_idx * slice_ent_cnt,
                                         slice_idx * slice_ent_cnt +
                                             fs->entry_count - 1,
                                         efp_tcam_buf + slice_idx *
                                             slice_ent_cnt *
                                  soc_mem_entry_words(unit, EFP_TCAMm))) < 0 ) {
                goto cleanup;
            }
        }
    } else {
        slice_ent_cnt = fs->entry_count;
        if ((rv = soc_mem_read_range(unit, EFP_TCAMm, MEM_BLOCK_ALL,
                                     index_min, index_max,
                                     efp_tcam_buf)) < 0 ) {
            goto cleanup;
        }
    }
    efp_policy_buf = soc_cm_salloc(unit, SOC_MEM_TABLE_BYTES
                                  (unit, EFP_POLICY_TABLEm),
                                  "EFP POLICY TABLE buffer");
    if (NULL == efp_policy_buf) {
        return BCM_E_MEMORY;
    }
    index_min = soc_mem_index_min(unit, EFP_POLICY_TABLEm);
    index_max = soc_mem_index_max(unit, EFP_POLICY_TABLEm);
    if ((rv = soc_mem_read_range(unit, EFP_POLICY_TABLEm, MEM_BLOCK_ALL,
                                 index_min, index_max,
                                 efp_policy_buf)) < 0 ) {
        goto cleanup;
    }

    /* Get slice expansion status and virtual map */
    if ((rv = _field_slice_expanded_status_get(unit, stage_fc, expanded)) < 0) {
        goto cleanup;
    }

    /* Iterate over the slices */
    if ((rv = READ_EFP_SLICE_CONTROLr(unit, &rval)) < 0) {
        goto cleanup;
    }
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        /* Ignore disabled slice */
        if ((soc_reg_field_get(unit, EFP_SLICE_CONTROLr, rval,
                               efp_en_flds[slice_idx]) == 0) ||
            (soc_reg_field_get(unit, EFP_SLICE_CONTROLr, rval,
                               efp_lk_en_flds[slice_idx]) == 0)) {
            continue;
        }

        efp_slice_mode = soc_reg_field_get(unit, EFP_SLICE_CONTROLr, rval,
                                           _trx_efp_slice_mode[slice_idx][0]);
        efp_key_mode = soc_reg_field_get(unit, EFP_SLICE_CONTROLr, rval,
                                         _trx_efp_slice_mode[slice_idx][1]);
        /* Skip if slice has no valid groups and entries */
        fs = stage_fc->slices + slice_idx;
        key_match_type = ~0;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            efp_tcam_entry = soc_mem_table_idx_to_pointer
                                 (unit, EFP_TCAMm, efp_tcam_entry_t *,
                                  efp_tcam_buf, idx +
                                  slice_ent_cnt * slice_idx);
            if (soc_EFP_TCAMm_field32_get(unit, efp_tcam_entry,
                                          VALIDf) != 0) {
                /* Get KEY_MATCH_TYPE from first valid entry */
#if defined(BCM_TRIDENT2_SUPPORT)
                /* In TD2, EFP_TCAM entry KEY field width is (230 + 2b VALID) */
                if (SOC_IS_TD2_TT2(unit)) {
                    _field_extract((uint32 *)efp_tcam_entry, 230 + 2, 4,
                                &key_match_type);
                } else
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
                /* In TR3, EFP_TCAM entry KEY field width is (231 + 2b VALID) */ 
                if (SOC_IS_TRIUMPH3(unit)) {
                    _field_extract((uint32 *)efp_tcam_entry, 231 + 2, 3,
                                &key_match_type);
                } else
#endif
                {
                _field_extract((uint32 *)efp_tcam_entry, 211 + 2, 3,
                                &key_match_type);
                }
                break;
            }
        }
        if (idx == slice_ent_cnt && !fc->l2warm) {
            continue;
        }
        /* Skip second part of slice pair */
        if (((efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6)) &&
            (slice_idx % 2)) {
            continue;
        }
        /* Don't need to read selectors for expanded slice */
        if (expanded[slice_idx]) {
            continue;
        }
        fg = NULL;
        if (efp_slice_mode <= 5) { /* Valid values */
            /* If Level 2, retrieve the GIDs in this slice */
            if (fc->l2warm) {
                rv = _field_trx_scache_slice_group_recover(unit,
                                                           fc,
                                                           slice_idx,
                                                           NULL,
                                                           stage_fc
                                                           );
                if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                    fc->l2warm = 0;
                    goto cleanup;
                }
                if (rv == BCM_E_NOT_FOUND) {
                    rv = BCM_E_NONE;
                    continue;
                }
            }

            rv = _field_tr2_group_construct_alloc(unit, &fg);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }

            if (fc->l2warm) {
                /* Get stored group ID and QSET for Level 2 */
                rv = _field_group_info_retrieve(unit,
                                                -1,
                                                &gid,
                                                &priority,
                                                &qset,
                                                fc
                                                );
                sal_memcpy(&fg->qset, &qset, sizeof(bcm_field_qset_t));
            } else {
                if ((rv = _field_group_id_generate(unit, &gid)) == BCM_E_NONE) {
                    unsigned vmap, vslice;

                    for (priority = -1, vmap = 0; priority == -1 && vmap < _FP_VMAP_CNT; ++vmap) {
                        for (vslice = 0; vslice < COUNTOF(stage_fc->vmap[0]); ++vslice) {
                            if (stage_fc->vmap[vmap][vslice].vmap_key == slice_idx) {
                                priority = stage_fc->vmap[vmap][vslice].priority;

                                break;
                            }
                        }
                    }

                    if (priority == -1) {
                        rv = BCM_E_INTERNAL;
                    }
                }
            }
            if (BCM_FAILURE(rv)) {
                sal_free(fg);
                goto cleanup;
            }

            fg->gid      = gid;
            fg->priority = slice_idx;
            fg->stage_id = stage_fc->stage_id;

            list_arr[0] = list_arr[1] = NULL;
            switch (efp_slice_mode) {
                case _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L2:
                    fg->flags |= _FP_GROUP_SPAN_SINGLE_SLICE;
                    fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY4;
                    parts_count = 1;
                    list_arr[0] = list_key4;
                    break;
                case _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3:
                    fg->flags |= _FP_GROUP_SPAN_SINGLE_SLICE;
                    if (key_match_type == ~0 /* No entries */
                        || key_match_type == _BCM_FIELD_EFP_KEY1_MATCH_TYPE
                        ) {
                        /* IPv4 single wide key */
                        fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY1;
                        parts_count = 1;
                        list_arr[0] = list_key1;
                        fg->sel_codes[0].ip6_addr_sel = _FP_SELCODE_DONT_CARE;
                    } else {
                        /* key_match_type == _BCM_FIELD_EFP_KEY2_MATCH_TYPE */
                        /* IPv6 single wide key */
                        fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY2;
                        parts_count = 1;
                        list_arr[0] = list_key2;
                        fg->sel_codes[0].ip6_addr_sel = efp_key_mode;
                    }
                    break;
                case _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3_ANY:
                    fg->flags |= _FP_GROUP_SPAN_SINGLE_SLICE;
                    if ((fc->l2warm) && 
                       (BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifySrcIp) ||
                        BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifyDstIp) ||
                        BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifyIpFrag))) {
                        if (key_match_type == _BCM_FIELD_EFP_KEY1_MATCH_TYPE) {
                            /* IPv4 single wide key */
                            fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY1;
                            parts_count = 1;
                            list_arr[0] = list_key1;
                            fg->sel_codes[0].ip6_addr_sel = _FP_SELCODE_DONT_CARE;
                        }
                    } else {
                    fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY5;
                    parts_count = 1;
                    list_arr[0] = list_key5;
                    }
                    break;
                case _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE:
                    fg->flags |= _FP_GROUP_SPAN_DOUBLE_SLICE;
                    if (key_match_type == ~0 /* No entries */
                        || key_match_type == _BCM_FIELD_EFP_KEY2_KEY3_MATCH_TYPE
                        ) {
                        fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY3;
                        fg->sel_codes[1].fpf3 = _BCM_FIELD_EFP_KEY2;
                        list_arr[0] = list_key3;
                        list_arr[1] = list_key2;
                    } else { /* _BCM_FIELD_EFP_KEY1_KEY4_MATCH_TYPE */
                        fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY1;
                        fg->sel_codes[1].fpf3 = _BCM_FIELD_EFP_KEY4;
                        list_arr[0] = list_key1;
                        list_arr[1] = list_key4;
                    }
                    parts_count = 2;
                    break;
                case _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY:
                    fg->flags |= _FP_GROUP_SPAN_DOUBLE_SLICE;
                    if ((fc->l2warm) && 
                       (BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifySrcIp) ||
                        BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifyDstIp) ||
                        BCM_FIELD_QSET_TEST(fg->qset, bcmFieldQualifyIpFrag))) {
                        if (key_match_type == _BCM_FIELD_EFP_KEY1_KEY4_MATCH_TYPE) {
                            /* IPv4 double wide key */
                            fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY1;
                            fg->sel_codes[1].fpf3 = _BCM_FIELD_EFP_KEY4;
                            list_arr[0] = list_key1;
                            list_arr[1] = list_key4;
                        }
                    } else {
                    fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY5;
                    fg->sel_codes[1].fpf3 = _BCM_FIELD_EFP_KEY4;
                    list_arr[0] = list_key5;
                    list_arr[1] = list_key4;
                    }
                    parts_count = 2;
                    break;
                case _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6:
                    fg->flags |= _FP_GROUP_SPAN_DOUBLE_SLICE;
                    fg->sel_codes[0].fpf3 = _BCM_FIELD_EFP_KEY2;
                    fg->sel_codes[1].fpf3 = _BCM_FIELD_EFP_KEY4;
                    list_arr[0] = list_key2;
                    list_arr[1] = list_key4;
                    fg->sel_codes[0].ip6_addr_sel = efp_key_mode;
                    parts_count = 2;
                    break;
                default:
                    break;
            }

            _field_tr2_group_construct_quals_add(unit, fc, stage_fc, fg);

            /* Associate slice(s) to group */
            fg->slices = stage_fc->slices + slice_idx;
            SOC_PBMP_CLEAR(all_pbmp);
            SOC_PBMP_ASSIGN(all_pbmp, PBMP_PORT_ALL(unit));
            SOC_PBMP_OR(all_pbmp, PBMP_CMIC(unit));
#if defined(BCM_KATANA2_SUPPORT)
            if (soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) {
                _bcm_kt2_subport_pbmp_update(unit, &all_pbmp);
            }
#endif
            SOC_PBMP_ASSIGN(fg->pbmp, all_pbmp);
            BCM_PBMP_OR(fs->pbmp, fg->pbmp);

            /* Initialize group default ASET list. */
            rv = _field_group_default_aset_set(unit, fg);
            if (BCM_FAILURE(rv)) {
                sal_free(fg);
                goto cleanup;
            }

            fg->flags |= _FP_GROUP_LOOKUP_ENABLED;
            fg->next = fc->groups;
            fc->groups = fg;

            /* Now go over the entries in this slice */
            prev_prio = -1;
            for (idx = 0; idx < slice_ent_cnt; idx++) {
                efp_tcam_entry = soc_mem_table_idx_to_pointer
                                     (unit, EFP_TCAMm, efp_tcam_entry_t *,
                                      efp_tcam_buf, idx +
                                      slice_ent_cnt * slice_idx);
                if (soc_EFP_TCAMm_field32_get(unit, efp_tcam_entry,
                                              VALIDf) == 0) {
                    continue;
                }
                /* Allocate memory for this entry */
                mem_sz = parts_count * sizeof (_field_entry_t);
                _FP_XGS3_ALLOC(f_ent, mem_sz, "field entry");
                if (f_ent == NULL) {
                    rv = BCM_E_MEMORY;
                    goto cleanup;
                }
                sid = pid = -1;
                if (fc->l2warm) {
                    rv = _field_trx_entry_info_retrieve(unit,
                                                        &eid,
                                                        &prio,
                                                        fc,
                                                        0,
                                                        &prev_prio,
                                                        &sid,
                                                        &pid,
                                                        stage_fc
                                                        );
                    
                    if (BCM_FAILURE(rv)) {
                        sal_free(f_ent);
                        goto cleanup;
                    }
                } else {
                    _bcm_field_last_alloc_eid_incr();
                }
                pri_tcam_idx = idx + slice_ent_cnt * slice_idx;
                for (i = 0; i < parts_count; i++) {
                    if (fc->l2warm) {
                        f_ent[i].eid = eid;
                    } else {
                        f_ent[i].eid = _bcm_field_last_alloc_eid_get();
                    }
                    f_ent[i].group = fg;
                    if (fc->flags & _FP_COLOR_INDEPENDENT) {
                        f_ent[i].flags |= _FP_ENTRY_COLOR_INDEPENDENT;
                    }
                    rv = _bcm_field_tcam_part_to_entry_flags(i, fg->flags,
                                                             &f_ent[i].flags);
                    if (BCM_FAILURE(rv)) {
                        sal_free(f_ent);
                        goto cleanup;
                    }
                    rv = _bcm_field_entry_part_tcam_idx_get(unit, f_ent,
                                                            pri_tcam_idx,
                                                            i, &part_index);
                    if (BCM_FAILURE(rv)) {
                        sal_free(f_ent);
                        goto cleanup;
                    }
                    rv = _bcm_field_tcam_idx_to_slice_offset(unit, stage_fc,
                                                             part_index,
                                                             &slice_number,
                                                (int *)&f_ent[i].slice_idx);
                    if (BCM_FAILURE(rv)) {
                        sal_free(f_ent);
                        goto cleanup;
                    }
                    f_ent[i].fs = stage_fc->slices + slice_number;
                    if (0 == (f_ent[i].flags & _FP_ENTRY_SECOND_HALF)) {
                        /* Decrement slice free entry count for primary
                           entries. */
                        f_ent[i].fs->free_count--;
                    }
                    /* Assign entry to a slice */
                    f_ent[i].fs->entries[f_ent[i].slice_idx] = f_ent + i;
                    f_ent[i].flags |= _FP_ENTRY_INSTALLED;

                    if (soc_EFP_TCAMm_field32_get(unit, efp_tcam_entry, VALIDf) == 3) {
                        f_ent[i].flags |= _FP_ENTRY_ENABLED;
                    }

                    /* Get the actions associated with this entry part */
                    efp_policy_entry = soc_mem_table_idx_to_pointer
                                               (unit, EFP_POLICY_TABLEm,
                                                efp_policy_table_entry_t *,
                                                efp_policy_buf, part_index);
                    rv = _field_tr2_actions_recover(unit,
                                                    EFP_POLICY_TABLEm,
                                                    (uint32 *) efp_policy_entry,
                                                    f_ent,
                                                    i,
                                                    sid,
                                                    pid
                                                    );
                }
                /* Add to the group */
                rv = _field_group_entry_add(unit, fg, f_ent);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                f_ent = NULL;
            }
        }
        /* Free up the temporary slice group info */
        if (fc->l2warm) {
            _field_scache_slice_group_free(unit,
                                           fc,
                                           slice_idx
                                           );
        }
    }

    /* Now go over the expanded slices */
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        if (!expanded[slice_idx]) {
            continue;
        }
        /* Ignore secondary slice in paired mode */
        efp_slice_mode = soc_reg_field_get(unit, EFP_SLICE_CONTROLr, rval,
                                           _trx_efp_slice_mode[slice_idx][0]);
        if (((efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY) ||
             (efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6)) &&
            (slice_idx % 2)) {
            continue;
        }
        /* Skip if slice has no valid entries */
        fs = stage_fc->slices + slice_idx;
        slice_ent_cnt = fs->entry_count;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            if (_bcm_field_slice_offset_to_tcam_idx(unit,
                stage_fc, slice_idx, idx, &phys_tcam_idx) != BCM_E_NONE) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            efp_tcam_entry = soc_mem_table_idx_to_pointer(unit, 
                             EFP_TCAMm, efp_tcam_entry_t *,
                             efp_tcam_buf, phys_tcam_idx);
            if (soc_EFP_TCAMm_field32_get(unit, efp_tcam_entry, VALIDf) != 0) {
                break;
            }
        }
        if (idx == slice_ent_cnt) {
            continue;
        }
        /* If Level 2, retrieve the GIDs in this slice */
        if (fc->l2warm) {
            rv = _field_scache_slice_group_recover(unit,
                     fc, slice_idx, &multigroup);
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                fc->l2warm = 0;
                goto cleanup;
            }
            if (rv == BCM_E_NOT_FOUND) {
                rv = BCM_E_NONE;
                continue;
            }
        }
        /* Now find the master slice for this virtual group */
        vslice_idx = _field_physical_to_virtual(unit, slice_idx, stage_fc);
        if (vslice_idx < 0) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        max = -1;
        for (i = 0; i < stage_fc->tcam_slices; i++) {
            if ((stage_fc->vmap[0][vslice_idx].virtual_group ==
                stage_fc->vmap[0][i].virtual_group) && (i != vslice_idx)) {
                if (i > max) {
                    max = i;
                }
            }
        }
        if (max < 0) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        master_slice = stage_fc->vmap[0][max].vmap_key;
        /* See which group is in this slice - can be only one */
        fg = fc->groups;
        while (fg != NULL) {
            /* Check if group is in this slice */
            fs = &fg->slices[0];
            if (fs->slice_number == master_slice) {
                break;
            }
            fg = fg->next;
        }
        if (fg == NULL) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        old_physical_slice = fs->slice_number;

        /* Set up the new physical slice parameters in Software */
        for(part_index = parts_count - 1; part_index >= 0; part_index--) {
            /* Get entry flags. */
            rv = _bcm_field_tcam_part_to_entry_flags(part_index, fg->flags, &entry_flags);
            BCM_IF_ERROR_RETURN(rv);
    
            /* Get slice id for entry part */
            rv = _bcm_field_tcam_part_to_slice_number(part_index, fg->flags, &slice_num);
            BCM_IF_ERROR_RETURN(rv);
            
            /* Get slice pointer. */
            fs = stage_fc->slices + slice_idx + slice_num;

            if (0 == (entry_flags & _FP_ENTRY_SECOND_HALF)) {
                /* Set per slice configuration &  number of free entries in the slice.*/
                fs->free_count = fs->entry_count;
                if (fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
                    fs->free_count >>= 1;
                }
                /* Set group flags in in slice.*/ 
                fs->group_flags = fg->flags & _FP_GROUP_STATUS_MASK;
    
                /* Add slice to slices linked list . */
                stage_fc->slices[old_physical_slice + slice_num].next = fs;
                fs->prev = &stage_fc->slices[old_physical_slice + slice_num];
            }
        }

        prev_prio = -1;
        SOC_PBMP_CLEAR(all_pbmp);
        SOC_PBMP_ASSIGN(all_pbmp, PBMP_PORT_ALL(unit));
        SOC_PBMP_OR(all_pbmp, PBMP_CMIC(unit));
#if defined(BCM_KATANA2_SUPPORT)
        if (soc_feature(unit, soc_feature_linkphy_coe) ||
            soc_feature(unit, soc_feature_subtag_coe)) {
            _bcm_kt2_subport_pbmp_update(unit, &all_pbmp);
        }
#endif
        SOC_PBMP_ASSIGN(fg->pbmp, all_pbmp);
        BCM_PBMP_OR(fs->pbmp, fg->pbmp);
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            if (_bcm_field_slice_offset_to_tcam_idx(unit,
                stage_fc, slice_idx, idx, &phys_tcam_idx) != BCM_E_NONE) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            efp_tcam_entry = soc_mem_table_idx_to_pointer(unit,
                             EFP_TCAMm, efp_tcam_entry_t *,
                             efp_tcam_buf, phys_tcam_idx);
            if (soc_EFP_TCAMm_field32_get(unit, efp_tcam_entry, VALIDf) == 0) {
                continue;
            }

            /* Allocate memory for the entry */
            rv = _bcm_field_entry_tcam_parts_count(unit, fg->flags,
                                                   &parts_count);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            mem_sz = parts_count * sizeof (_field_entry_t);
            _FP_XGS3_ALLOC(f_ent, mem_sz, "field entry");
            if (f_ent == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            sid = pid = -1;
            if (fc->l2warm) {
                rv = _field_trx_entry_info_retrieve(unit,
                         &eid, &prio, fc, multigroup, &prev_prio, &sid, &pid,
                         stage_fc);
                
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
            } else {
                _bcm_field_last_alloc_eid_incr();
            }
            pri_tcam_idx = phys_tcam_idx;
            for (i = 0; i < parts_count; i++) {
                if (fc->l2warm) {
                    /* Use retrieved EID */
                    f_ent[i].eid = eid;
                } else {
                    f_ent[i].eid = _bcm_field_last_alloc_eid_get();
                }
                f_ent[i].group = fg;

                if (fc->flags & _FP_COLOR_INDEPENDENT) {
                    f_ent[i].flags |= _FP_ENTRY_COLOR_INDEPENDENT;
                }
                rv = _bcm_field_tcam_part_to_entry_flags(i, fg->flags,
                                                         &f_ent[i].flags);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_entry_part_tcam_idx_get(unit,
                         f_ent, pri_tcam_idx, i, &part_index);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_tcam_idx_to_slice_offset(unit,
                         stage_fc, part_index, &slice_number,
                         (int *)&f_ent[i].slice_idx);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                f_ent[i].fs = stage_fc->slices + slice_number;
                if (0 == (f_ent[i].flags & _FP_ENTRY_SECOND_HALF)) {
                    /* Decrement slice free entry count for primary
                       entries. */
                    f_ent[i].fs->free_count--;
                }
                /* Assign entry to a slice */
                f_ent[i].fs->entries[f_ent[i].slice_idx] = f_ent + i;
                f_ent[i].flags |= _FP_ENTRY_INSTALLED;

                if (soc_EFP_TCAMm_field32_get(unit, efp_tcam_entry, VALIDf) == 3) {
                    f_ent[i].flags |= _FP_ENTRY_ENABLED;
                }

                /* Get the actions associated with this part of the entry */
                efp_policy_entry = soc_mem_table_idx_to_pointer(
                    unit, EFP_POLICY_TABLEm, efp_policy_table_entry_t *,
                    efp_policy_buf, part_index);
                rv = _field_tr2_actions_recover(unit,
                    EFP_POLICY_TABLEm, (uint32 *) efp_policy_entry,
                    f_ent, i, sid, pid);
            }
            /* Add to the group */
            if (fc->l2warm) {
                f_ent->prio = prio;
            } else {
                f_ent->prio = (vslice_idx << 10) | (slice_ent_cnt - idx);
            }
            rv = _field_group_entry_add(unit, fg, f_ent);
            if (BCM_FAILURE(rv)) {
                sal_free(f_ent);
                goto cleanup;
            }
            f_ent = NULL;
        }
        /* Free up the temporary slice group info */
        if (fc->l2warm) {
            _field_scache_slice_group_free(unit, fc, slice_idx);
        }
    }
    if (fc->l2warm) {
        temp = 0;
        temp |= buf[fc->scache_pos];
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 8;
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 16;
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 24;
        fc->scache_pos++;
        if (temp != _FIELD_EFP_DATA_END) {
            fc->l2warm = 0;
            rv = BCM_E_INTERNAL;
        }

        if (NULL != buf1) {
            temp = 0;
            temp |= buf1[fc->scache_pos1];
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 8;
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 16;
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 24;
            fc->scache_pos1++;
            if (temp != _FIELD_EFP_DATA_END) {
                fc->l2warm = 0;
                rv = BCM_E_INTERNAL;
            }
        }

    }

    _field_tr2_stage_reinit_all_groups_cleanup(unit, fc, 
                                               _BCM_FIELD_STAGE_EGRESS,
                                               NULL);

cleanup:
    if (efp_tcam_buf) {
        soc_cm_sfree(unit, efp_tcam_buf);
    }
    if (efp_policy_buf) {
        soc_cm_sfree(unit, efp_policy_buf);
    }
    return rv;
}

int
_field_tr2_stage_lookup_reinit(int unit, _field_control_t *fc,
                               _field_stage_t *stage_fc)
{
    int vslice_idx, max, master_slice;
    int idx, slice_idx, index_min, index_max, ratio, rv = BCM_E_NONE;
    int group_found, mem_sz, parts_count, slice_ent_cnt, expanded[4];
    int i, pri_tcam_idx, part_index, slice_number, prio, prev_prio;
    uint32 *vfp_tcam_buf = NULL; /* Buffer to read the VFP_TCAM table */
    char *vfp_policy_buf = NULL; /* Buffer to read the VFP_POLICY table */
    uint32 rval, paired, intraslice, dbl_wide_key, dbl_wide_key_sec;
    uint32 vfp_key, vfp_key2, temp;
#if defined(BCM_KATANA2_SUPPORT)
    uint64 vfp_key_1 = 0;
#endif
    soc_field_t fld;
    vfp_tcam_entry_t *vfp_tcam_entry;
    vfp_policy_table_entry_t *vfp_policy_entry;
    _field_hw_qual_info_t hw_sels;
    _field_slice_t *fs;
    _field_group_t *fg;
    _field_entry_t *f_ent = NULL;
    bcm_pbmp_t entry_pbmp, temp_pbmp;
    bcm_field_entry_t eid;
    bcm_field_stat_t sid;
    bcm_policer_t pid;
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1];
#if defined(BCM_KATANA_SUPPORT) 
    _field_stat_t *f_st = NULL;
    bcm_stat_group_mode_t   stat_mode;    /* Stat type bcmStatGroupModeXXX. */
    bcm_stat_object_t       stat_obj;     /* Stat object type.              */
    uint32                  pool_num;     /* Flex Stat Hw Pool No.          */
    uint32                  base_index;   /* Flex Stat counter base index.  */
#endif

#if defined(BCM_KATANA_SUPPORT)
    int offset_mode = 0, policer_index = 0;
#endif
    soc_field_t vfp_en_flds[4] = {SLICE_ENABLE_SLICE_0f, SLICE_ENABLE_SLICE_1f,
                                  SLICE_ENABLE_SLICE_2f, SLICE_ENABLE_SLICE_3f};

    soc_field_t vfp_lk_en_flds[4] =
                     {LOOKUP_ENABLE_SLICE_0f, LOOKUP_ENABLE_SLICE_1f,
                      LOOKUP_ENABLE_SLICE_2f, LOOKUP_ENABLE_SLICE_3f};

    SOC_PBMP_CLEAR(entry_pbmp);
    sal_memset(expanded, 0, 4 * sizeof(int));

    if (fc->l2warm) {
        rv = _field_scache_stage_hdr_chk(fc, _FIELD_VFP_DATA_START);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }
    }

    /* DMA various tables */
    vfp_tcam_buf = soc_cm_salloc(unit, sizeof(vfp_tcam_entry_t) *
                                soc_mem_index_count(unit, VFP_TCAMm),
                                "VFP TCAM buffer");
    if (NULL == vfp_tcam_buf) {
        return BCM_E_MEMORY;
    }
    sal_memset(vfp_tcam_buf, 0, sizeof(vfp_tcam_entry_t) *
               soc_mem_index_count(unit, VFP_TCAMm));
    index_min = soc_mem_index_min(unit, VFP_TCAMm);
    index_max = soc_mem_index_max(unit, VFP_TCAMm);
    fs = stage_fc->slices;
    if (stage_fc->flags & _FP_STAGE_HALF_SLICE) {
        slice_ent_cnt = fs->entry_count * 2;
        /* DMA in chunks */
        for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
            fs = stage_fc->slices + slice_idx;
            if ((rv = soc_mem_read_range(unit, VFP_TCAMm, MEM_BLOCK_ALL,
                                         slice_idx * slice_ent_cnt,
                                         slice_idx * slice_ent_cnt +
                                             fs->entry_count / 2 - 1,
                                         vfp_tcam_buf + slice_idx *
                                             slice_ent_cnt *
                                  soc_mem_entry_words(unit, VFP_TCAMm))) < 0 ) {
                goto cleanup;
            }
            if ((rv = soc_mem_read_range(unit, VFP_TCAMm, MEM_BLOCK_ALL,
                                         slice_idx * slice_ent_cnt +
                                         fs->entry_count,
                                         slice_idx * slice_ent_cnt +
                                         fs->entry_count +
                                             fs->entry_count / 2 - 1,
                                         vfp_tcam_buf + (slice_idx *
                                         slice_ent_cnt + fs->entry_count) *
                                   soc_mem_entry_words(unit, VFP_TCAMm))) < 0 ) {
                goto cleanup;
            }
        }
    } else {
        slice_ent_cnt = fs->entry_count;
        if ((rv = soc_mem_read_range(unit, VFP_TCAMm, MEM_BLOCK_ALL,
                                     index_min, index_max,
                                     vfp_tcam_buf)) < 0 ) {
            goto cleanup;
        }
    }
    vfp_policy_buf = soc_cm_salloc(unit, SOC_MEM_TABLE_BYTES
                                  (unit, VFP_POLICY_TABLEm),
                                  "VFP POLICY TABLE buffer");
    if (NULL == vfp_policy_buf) {
        return BCM_E_MEMORY;
    }
    index_min = soc_mem_index_min(unit, VFP_POLICY_TABLEm);
    index_max = soc_mem_index_max(unit, VFP_POLICY_TABLEm);
    if ((rv = soc_mem_read_range(unit, VFP_POLICY_TABLEm, MEM_BLOCK_ALL,
                                 index_min, index_max,
                                 vfp_policy_buf)) < 0 ) {
        goto cleanup;
    }

    /* Get slice expansion status and virtual map */
    if ((rv = _field_slice_expanded_status_get(unit, stage_fc, expanded)) < 0) {
        goto cleanup;
    }

    /* Iterate over the slices */
    if ((rv = READ_VFP_SLICE_CONTROLr(unit, &rval)) < 0) {
        goto cleanup;
    }
    if ((rv = READ_VFP_KEY_CONTROLr(unit, &vfp_key)) < 0) {
        goto cleanup;
    }
    if ((rv = READ_VFP_KEY_CONTROL_2r(unit, &vfp_key2)) < 0) {
        goto cleanup;
    }
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        /* Ignore disabled slice */
        if ((soc_reg_field_get(unit, VFP_SLICE_CONTROLr, rval,
                               vfp_en_flds[slice_idx]) == 0) ||
            (soc_reg_field_get(unit, VFP_SLICE_CONTROLr, rval,
                               vfp_lk_en_flds[slice_idx]) == 0)) {
            continue;
        }
        /* Ignore secondary slice in paired mode */
        fld = _bcm_field_trx_slice_pairing_field[slice_idx / 2];
        paired = soc_reg_field_get(unit, VFP_KEY_CONTROLr, vfp_key, fld);
        fld = _vfp_slice_wide_mode_field[slice_idx];
        intraslice = soc_reg_field_get(unit, VFP_KEY_CONTROLr, vfp_key, fld);
        if (paired && (slice_idx % 2)) {
            continue;
        }
        /* Don't need to read selectors for expanded slice */
        if (expanded[slice_idx]) {
            continue;
        }
        /* Skip if slice has no valid groups and entries */
        fs = stage_fc->slices + slice_idx;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            vfp_tcam_entry = soc_mem_table_idx_to_pointer
                                 (unit, VFP_TCAMm, vfp_tcam_entry_t *,
                                  vfp_tcam_buf, idx +
                                  slice_ent_cnt * slice_idx);
            if (soc_VFP_TCAMm_field32_get(unit, vfp_tcam_entry,
                                          VALIDf) != 0) {
                break;
            }
        }
        if (idx == slice_ent_cnt && !fc->l2warm) {
            continue;
        }
        /* If Level 2, retrieve the GIDs in this slice */
        if (fc->l2warm) {
            rv = _field_trx_scache_slice_group_recover(unit,
                                                       fc,
                                                       slice_idx,
                                                       NULL,
                                                       stage_fc
                                                       );
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                fc->l2warm = 0;
                goto cleanup;
            }
            if (rv == BCM_E_NOT_FOUND) {
                rv = BCM_E_NONE;
                continue;
            }
        }

        /* Construct the group based on HW selector values */
        _FIELD_SELCODE_CLEAR(hw_sels.pri_slice[0]);
        hw_sels.pri_slice[0].intraslice = _FP_SELCODE_DONT_USE;
        _FIELD_SELCODE_CLEAR(hw_sels.pri_slice[1]);
        hw_sels.pri_slice[1].intraslice = _FP_SELCODE_DONT_USE;
        _FIELD_SELCODE_CLEAR(hw_sels.sec_slice[0]);
        hw_sels.sec_slice[0].intraslice = _FP_SELCODE_DONT_USE;
        _FIELD_SELCODE_CLEAR(hw_sels.sec_slice[1]);
        hw_sels.sec_slice[1].intraslice = _FP_SELCODE_DONT_USE;

        /* Get primary slice's selectors */
        hw_sels.pri_slice[0].fpf1 = 0;
        hw_sels.pri_slice[0].fpf2
            = soc_reg_field_get(unit,
                                VFP_KEY_CONTROLr,
                                vfp_key,
                                _bcm_field_trx_vfp_field_sel[slice_idx][0]
                                );
        hw_sels.pri_slice[0].fpf3
            = soc_reg_field_get(unit,
                                VFP_KEY_CONTROLr,
                                vfp_key,
                                _bcm_field_trx_vfp_field_sel[slice_idx][1]
                                );
        hw_sels.pri_slice[0].ip_header_sel
            = soc_reg_field_get(unit,
                                VFP_KEY_CONTROL_2r,
                                vfp_key2,
                                _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                                );
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANA(unit)) {
           hw_sels.pri_slice[0].src_entity_sel
               = soc_reg_field_get(unit,
                                   VFP_KEY_CONTROL_2r,
                                   vfp_key2,
                                   _trx_vfp_src_type_sel[slice_idx]
                                   );
         }
#endif
#if defined (BCM_KATANA2_SUPPORT)
         if (SOC_IS_KATANA2(unit)) {
            hw_sels.pri_slice[0].src_entity_sel
                = soc_reg_field_get(unit,
                                    VFP_KEY_CONTROL_1r,
                                    vfp_key_1,
                                    _trx_vfp_src_type_sel[slice_idx]
                                    );           
         }
#endif
        /* If intraslice, get double-wide key - only 2 options */
        if (intraslice) {
            dbl_wide_key = soc_reg_field_get(
                               unit,
                               VFP_KEY_CONTROLr,
                               vfp_key,
                               _bcm_field_trx_vfp_double_wide_sel[slice_idx]
                                             );
            hw_sels.pri_slice[1].intraslice = TRUE;
            hw_sels.pri_slice[1].fpf2 = dbl_wide_key;
            hw_sels.pri_slice[1].ip_header_sel
                = soc_reg_field_get(unit,
                                    VFP_KEY_CONTROL_2r, vfp_key2,
                                    _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                                    );
#if defined(BCM_KATANA_SUPPORT)
            if (SOC_IS_KATANA(unit)) {
               hw_sels.pri_slice[1].src_entity_sel
                   = soc_reg_field_get(unit,
                                       VFP_KEY_CONTROL_2r,
                                       vfp_key2,
                                       _trx_vfp_src_type_sel[slice_idx]
                                       );
            }
#endif
#if defined(BCM_KATANA2_SUPPORT)
            if (SOC_IS_KATANA2(unit)) {
               hw_sels.pri_slice[1].src_entity_sel
                   = soc_reg_field_get(unit,
                                       VFP_KEY_CONTROL_1r,
                                       vfp_key_1,
                                       _trx_vfp_src_type_sel[slice_idx]
                                       );
            }
#endif
        }
        /* If paired, get secondary slice's selectors */
        if (paired) {
            hw_sels.sec_slice[0].fpf1 = 0;
            hw_sels.sec_slice[0].fpf2
                = soc_reg_field_get(
                      unit,
                      VFP_KEY_CONTROLr,
                      vfp_key,
                      _bcm_field_trx_vfp_field_sel[slice_idx + 1][0]
                                    );
            hw_sels.sec_slice[0].fpf3
                = soc_reg_field_get(
                      unit,
                      VFP_KEY_CONTROLr,
                      vfp_key,
                      _bcm_field_trx_vfp_field_sel[slice_idx + 1][1]
                                    );
            hw_sels.sec_slice[0].ip_header_sel
                = soc_reg_field_get(unit,
                                    VFP_KEY_CONTROL_2r, vfp_key2,
                                    _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                                    );
#if defined(BCM_KATANA_SUPPORT)
            if (SOC_IS_KATANA(unit)) {
               hw_sels.sec_slice[0].src_entity_sel
                   = soc_reg_field_get(unit,
                                       VFP_KEY_CONTROL_2r,
                                       vfp_key2,
                                       _trx_vfp_src_type_sel[slice_idx]
                                       );
            }
#endif
#if defined(BCM_KATANA2_SUPPORT)
            if (SOC_IS_KATANA2(unit)) {
               hw_sels.sec_slice[0].src_entity_sel
                   = soc_reg_field_get(unit,
                                       VFP_KEY_CONTROL_1r,
                                       vfp_key_1,
                                       _trx_vfp_src_type_sel[slice_idx]
                                       );
            }
#endif
            if (intraslice) {
                dbl_wide_key_sec
                    = soc_reg_field_get(
                          unit,
                          VFP_KEY_CONTROLr,
                          vfp_key,
                          _bcm_field_trx_vfp_double_wide_sel[slice_idx]
                                        );
                hw_sels.sec_slice[1].intraslice = TRUE;
                hw_sels.sec_slice[1].fpf2 = dbl_wide_key_sec;
                hw_sels.sec_slice[1].ip_header_sel
                    = soc_reg_field_get(
                          unit,
                          VFP_KEY_CONTROL_2r,
                          vfp_key2,
                          _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                                        );
#if defined(BM_KATANA_SUPPORT)
                if (SOC_IS_KATANA(unit)) {
                   hw_sels.sec_slice[1].src_entity_sel
                       = soc_reg_field_get(unit,
                                           VFP_KEY_CONTROL_2r,
                                           vfp_key2,
                                           _trx_vfp_src_type_sel[slice_idx]
                                           );
                }
#endif
#if defined (BCM_KATANA2_SUPPORT)
                if (SOC_IS_KATANA2(unit)) {
                  hw_sels.sec_slice[1].src_entity_sel
                      = soc_reg_field_get(unit,
                                          VFP_KEY_CONTROL_1r,
                                          vfp_key_1,
                                          _trx_vfp_src_type_sel[slice_idx]
                                          );
                }
#endif
            }
        }
        /* Create a group based on HW qualifiers (or find existing) */
        rv = _field_tr2_group_construct(unit, &hw_sels, intraslice,
                                        paired, fc, -1,
                                        _BCM_FIELD_STAGE_LOOKUP,
                                        slice_idx
                                        );
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        /* Now go over the entries */
        fs = stage_fc->slices + slice_idx;
        if (fs->group_flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
            fs->free_count >>= 1;
            ratio = 2;
        } else {
            ratio = 1;
        }
        prev_prio = -1;
        for (idx = 0; idx < slice_ent_cnt / ratio; idx++) {
            group_found = 0;
            vfp_tcam_entry = soc_mem_table_idx_to_pointer
                                 (unit, VFP_TCAMm, vfp_tcam_entry_t *,
                                  vfp_tcam_buf, idx +
                                  slice_ent_cnt * slice_idx);
            if (soc_VFP_TCAMm_field32_get(unit, vfp_tcam_entry,
                                          VALIDf) == 0) {
                continue;
            }
            /* All ports are applicable to this entry */
            SOC_PBMP_ASSIGN(entry_pbmp, PBMP_PORT_ALL(unit));
            SOC_PBMP_OR(entry_pbmp, PBMP_CMIC(unit));
#if defined(BCM_KATANA2_SUPPORT)
            if (soc_feature(unit, soc_feature_linkphy_coe) ||
                soc_feature(unit, soc_feature_subtag_coe)) {
                _bcm_kt2_subport_pbmp_update(unit, &entry_pbmp);
            }
#endif
            /* Search groups to find match */
            fg = fc->groups;
            while (fg != NULL) {
                /* Check if group is in this slice */
                fs = &fg->slices[0];
                if (fs->slice_number != slice_idx) {
                    fg = fg->next;
                    continue;
                }
                /* Check if entry_pbmp is a subset of group pbmp */
                SOC_PBMP_ASSIGN(temp_pbmp, fg->pbmp);
                SOC_PBMP_AND(temp_pbmp, entry_pbmp);
                if (SOC_PBMP_EQ(temp_pbmp, entry_pbmp)) {
                    group_found = 1;
                    break;
                }
                fg = fg->next;
            }
            if (!group_found) {
                return BCM_E_INTERNAL; /* Should never happen */
            }
            /* Allocate memory for the entry */
            rv = _bcm_field_entry_tcam_parts_count(unit, fg->flags,
                                                   &parts_count);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            mem_sz = parts_count * sizeof (_field_entry_t);
            _FP_XGS3_ALLOC(f_ent, mem_sz, "field entry");
            if (f_ent == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            sid = pid = -1;
            if (fc->l2warm) {
                rv = _field_trx_entry_info_retrieve(unit,
                                                    &eid,
                                                    &prio,
                                                    fc,
                                                    0,
                                                    &prev_prio,
                                                    &sid,
                                                    &pid,
                                                    stage_fc
                                                    );
                
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
            } else {
                _bcm_field_last_alloc_eid_incr();
            }
            pri_tcam_idx = idx + slice_ent_cnt * slice_idx;
            for (i = 0; i < parts_count; i++) {
                if (fc->l2warm) {
                    /* Use retrieved EID */
                    f_ent[i].eid = eid;
                } else {
                    f_ent[i].eid = _bcm_field_last_alloc_eid_get();
                }
                f_ent[i].group = fg;
                if (fc->flags & _FP_COLOR_INDEPENDENT) {
                    f_ent[i].flags |= _FP_ENTRY_COLOR_INDEPENDENT;
                }
                rv = _bcm_field_tcam_part_to_entry_flags(i, fg->flags,
                                                         &f_ent[i].flags);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_entry_part_tcam_idx_get(unit, f_ent,
                                                        pri_tcam_idx,
                                                        i, &part_index);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_tcam_idx_to_slice_offset(unit, stage_fc,
                                                         part_index,
                                                         &slice_number,
                                                (int *)&f_ent[i].slice_idx);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                f_ent[i].fs = stage_fc->slices + slice_number;
                if (0 == (f_ent[i].flags & _FP_ENTRY_SECOND_HALF)) {
                    /* Decrement slice free entry count for primary
                       entries. */
                    f_ent[i].fs->free_count--;
                }
                /* Assign entry to a slice */
                f_ent[i].fs->entries[f_ent[i].slice_idx] = f_ent + i;
                BCM_PBMP_OR(f_ent[i].fs->pbmp, fg->pbmp);
                f_ent[i].flags |= _FP_ENTRY_INSTALLED;

                if (soc_VFP_TCAMm_field32_get(unit, vfp_tcam_entry, VALIDf) == 3) {
                    f_ent[i].flags |= _FP_ENTRY_ENABLED;
                }

                /* Get the actions associated with this part of the entry */
                vfp_policy_entry = soc_mem_table_idx_to_pointer
                                                (unit, VFP_POLICY_TABLEm,
                                                 vfp_policy_table_entry_t *,
                                                 vfp_policy_buf, part_index);
                rv = _field_tr2_actions_recover(unit,
                                                VFP_POLICY_TABLEm,
                                                (uint32 *) vfp_policy_entry,
                                                f_ent,
                                                i,
                                                sid,
                                                pid
                                                );
#if defined(BCM_KATANA_SUPPORT) 
                if (SOC_IS_KATANAX(unit)) {
                    if ((flex_info.valid == 1) && (flex_info.flex_mode != 0)) {
                        BCM_IF_ERROR_RETURN(_bcm_field_stat_get(
                                            unit, f_ent->statistic.sid, &f_st));
                        f_st->flex_mode = flex_info.flex_mode;
                        f_st->hw_flags = flex_info.hw_flags;
                        _bcm_esw_stat_get_counter_id_info(f_st->flex_mode,
                                          &stat_mode,
                                          &stat_obj,
                                          (uint32 *)&offset_mode,
                                          &pool_num,
                                          &base_index
                                          );
                        f_st->hw_index = base_index;
                        f_st->pool_index = pool_num;
                        f_st->hw_mode = stat_mode; /* Shouldn't be offset_mode*/
                        f_st->hw_entry_count = 1; /* PlsNote:For SingleMode=1 */
                        /* Currently OnlySingleMode is supportes so above OK */
                        /* else decision will be based on stat_mode  
                           Probably one helper function will be required */
                    }
                }
                flex_info.valid=0;
#endif

#if defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_KATANAX(unit)) {
                    policer_index = PolicyGet(unit, 
                                          VFP_POLICY_TABLEm,
                                          vfp_policy_entry,
                                          SVC_METER_INDEXf); 
                    offset_mode  = PolicyGet(unit, 
                                   VFP_POLICY_TABLEm,
                                   vfp_policy_entry,
                                   SVC_METER_OFFSET_MODEf); 
                    rv = _bcm_esw_get_policer_id_from_index_offset(unit, 
                                            policer_index, offset_mode, 
                                            &(f_ent->global_meter_policer.pid));
                    if (BCM_FAILURE(rv)) {
                       sal_free(f_ent);
                       goto cleanup;
                    }
                    f_ent->global_meter_policer.flags = PolicyGet(unit, 
                                                    VFP_POLICY_TABLEm,
                                                    vfp_policy_entry,
                                                    SVC_METER_INDEX_PRIORITYf); 
                }
#endif  /* BCM_KATANA_SUPPORT */
            }
            /* Add to the group */
            if (fc->l2warm) {
                f_ent->prio = prio;
            } else {
                f_ent->prio = (slice_idx << 10) | (slice_ent_cnt - idx);
            }
            rv = _field_group_entry_add(unit, fg, f_ent);
            if (BCM_FAILURE(rv)) {
                sal_free(f_ent);
                goto cleanup;
            }
            f_ent = NULL;
        }
        /* Free up the temporary slice group info */
        if (fc->l2warm) {
            _field_scache_slice_group_free(unit,
                                           fc,
                                           slice_idx
                                           );
        }
    }

    /* Now go over the expanded slices */
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        if (!expanded[slice_idx]) {
            continue;
        }
        /* Ignore secondary slice in paired mode */
        fld = _bcm_field_trx_slice_pairing_field[slice_idx / 2];
        paired = soc_reg_field_get(unit, VFP_KEY_CONTROLr, vfp_key, fld);
        if (paired && (slice_idx % 2)) {
            continue;
        }
        /* Skip if slice has no valid groups and entries */
        fs = stage_fc->slices + slice_idx;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            vfp_tcam_entry = soc_mem_table_idx_to_pointer
                                 (unit, VFP_TCAMm, vfp_tcam_entry_t *,
                                  vfp_tcam_buf, idx +
                                  slice_ent_cnt * slice_idx);
            if (soc_VFP_TCAMm_field32_get(unit, vfp_tcam_entry,
                                          VALIDf) != 0) {
                break;
            }
        }

        if (idx == slice_ent_cnt && !fc->l2warm) {
            continue;
        }

        /* If Level 2, retrieve the GIDs in this slice */
        if (fc->l2warm) {
            rv = _field_trx_scache_slice_group_recover(unit,
                                                       fc,
                                                       slice_idx,
                                                       NULL,
                                                       stage_fc
                                                       );
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                fc->l2warm = 0;
                goto cleanup;
            }
            if (rv == BCM_E_NOT_FOUND) {
                rv = BCM_E_NONE;
                continue;
            }
        }

        /* Now find the master slice for this virtual group */
        vslice_idx = _field_physical_to_virtual(unit, slice_idx, stage_fc);
        if (vslice_idx < 0) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }

        max = -1;
        for (i = 0; i < stage_fc->tcam_slices; i++) {
            if ((stage_fc->vmap[0][vslice_idx].virtual_group ==
                stage_fc->vmap[0][i].virtual_group) && (i != vslice_idx)) {
                if (i > max) {
                    max = i;
                }
            }
        }
        if (max < 0) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        master_slice = stage_fc->vmap[0][max].vmap_key;
        /* See which group is in this slice - can be only one */
        fg = fc->groups;
        while (fg != NULL) {
            /* Check if group is in this slice */
            fs = &fg->slices[0];
            if (fs->slice_number == master_slice) {
                break;
            }
            fg = fg->next;
        }
        if (fg == NULL) {
            rv = BCM_E_INTERNAL;

            goto cleanup;
        }

        fs = stage_fc->slices + slice_idx;
        fs->group_flags = fg->flags & _FP_GROUP_STATUS_MASK;
        /* Append slice to group's doubly-linked list of slices */

        {
            _field_slice_t *p, *q;

            for (p = 0, q = fg->slices; q; q = q->next) {
                p = q;
            }

            *(p ? &p->next : &fg->slices) = fs;

            fs->prev = p;
            fs->next = 0;
        }

        /* Now go over the entries */
        fs = stage_fc->slices + slice_idx;
        if (fs->group_flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {
            ratio = 2;
            fs->free_count >>= 1;
        } else {
            ratio = 1;
        }

        prev_prio = -1;
        for (idx = 0; idx < slice_ent_cnt / ratio; idx++) {
            vfp_tcam_entry = soc_mem_table_idx_to_pointer
                                 (unit, VFP_TCAMm, vfp_tcam_entry_t *,
                                  vfp_tcam_buf, idx +
                                  slice_ent_cnt * slice_idx);
            if (soc_VFP_TCAMm_field32_get(unit, vfp_tcam_entry,
                                          VALIDf) == 0) {
                continue;
            }

            /* Allocate memory for the entry */
            rv = _bcm_field_entry_tcam_parts_count(unit, fg->flags,
                                                   &parts_count);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            mem_sz = parts_count * sizeof (_field_entry_t);
            _FP_XGS3_ALLOC(f_ent, mem_sz, "field entry");
            if (f_ent == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            sid = pid = -1;
            if (fc->l2warm) {
                rv = _field_trx_entry_info_retrieve(unit,
                                                    &eid,
                                                    &prio,
                                                    fc,
                                                    0,
                                                    &prev_prio,
                                                    &sid,
                                                    &pid,
                                                    stage_fc
                                                    );
                
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
            } else {
                _bcm_field_last_alloc_eid_incr();
            }
            pri_tcam_idx = idx + slice_ent_cnt * slice_idx;
            for (i = 0; i < parts_count; i++) {
                if (fc->l2warm) {
                    /* Use retrieved EID */
                    f_ent[i].eid = eid;
                } else {
                    f_ent[i].eid = _bcm_field_last_alloc_eid_get();
                }
                f_ent[i].group = fg;
                if (fc->flags & _FP_COLOR_INDEPENDENT) {
                    f_ent[i].flags |= _FP_ENTRY_COLOR_INDEPENDENT;
                }
                rv = _bcm_field_tcam_part_to_entry_flags(i, fg->flags,
                                                         &f_ent[i].flags);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_entry_part_tcam_idx_get(unit, f_ent,
                                                        pri_tcam_idx,
                                                        i, &part_index);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_tcam_idx_to_slice_offset(unit, stage_fc,
                                                         part_index,
                                                         &slice_number,
                                                (int *)&f_ent[i].slice_idx);
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                f_ent[i].fs = stage_fc->slices + slice_number;
                if (0 == (f_ent[i].flags & _FP_ENTRY_SECOND_HALF)) {
                    /* Decrement slice free entry count for primary
                       entries. */
                    f_ent[i].fs->free_count--;
                }
                /* Assign entry to a slice */
                f_ent[i].fs->entries[f_ent[i].slice_idx] = f_ent + i;
                BCM_PBMP_OR(f_ent[i].fs->pbmp, fg->pbmp);
                f_ent[i].flags |= _FP_ENTRY_INSTALLED;

                if (soc_VFP_TCAMm_field32_get(unit, vfp_tcam_entry, VALIDf) == 3) {
                    f_ent[i].flags |= _FP_ENTRY_ENABLED;
                }

                /* Get the actions associated with this part of the entry */
                vfp_policy_entry = soc_mem_table_idx_to_pointer
                                                (unit, VFP_POLICY_TABLEm,
                                                 vfp_policy_table_entry_t *,
                                                 vfp_policy_buf, part_index);
                rv = _field_tr2_actions_recover(unit,
                                                VFP_POLICY_TABLEm,
                                                (uint32 *) vfp_policy_entry,
                                                f_ent,
                                                i,
                                                sid,
                                                pid
                                                );
#if defined(BCM_KATANA_SUPPORT) /* Retrieving meter flags. */
                if (SOC_IS_KATANAX(unit)) {
                    policer_index = PolicyGet(unit,
                                          VFP_POLICY_TABLEm,
                                          vfp_policy_entry,
                                          SVC_METER_INDEXf);
                    offset_mode  = PolicyGet(unit,
                                   VFP_POLICY_TABLEm,
                                   vfp_policy_entry,
                                   SVC_METER_OFFSET_MODEf);
                    rv = _bcm_esw_get_policer_id_from_index_offset(unit, 
                                            policer_index, offset_mode, 
                                            &(f_ent->global_meter_policer.pid));
                    if (BCM_FAILURE(rv)) {
                       sal_free(f_ent);
                       goto cleanup;
                    }
                    f_ent->global_meter_policer.flags = PolicyGet(unit,
                                                    VFP_POLICY_TABLEm,
                                                    vfp_policy_entry,
                                                    SVC_METER_INDEX_PRIORITYf);
                }
#endif  /* BCM_KATANA_SUPPORT */ 
            }
            /* Add to the group */
            if (fc->l2warm) {
                f_ent->prio = prio;
            } else {
                f_ent->prio = (slice_idx << 10) | (slice_ent_cnt - idx);
            }
            rv = _field_group_entry_add(unit, fg, f_ent);
            if (BCM_FAILURE(rv)) {
                sal_free(f_ent);
                goto cleanup;
            }
            f_ent = NULL;
        }
        /* Free up the temporary slice group info */
        if (fc->l2warm) {
            _field_scache_slice_group_free(unit,
                                           fc,
                                           slice_idx
                                           );
        }
    }

    if (fc->l2warm) {
        temp = 0;
        temp |= buf[fc->scache_pos];
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 8;
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 16;
        fc->scache_pos++;
        temp |= buf[fc->scache_pos] << 24;
        fc->scache_pos++;
        if (temp != _FIELD_VFP_DATA_END) {
            fc->l2warm = 0;
            rv = BCM_E_INTERNAL;
        }
        if (NULL != buf1) {
            temp = 0;
            temp |= buf1[fc->scache_pos1];
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 8;
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 16;
            fc->scache_pos1++;
            temp |= buf1[fc->scache_pos1] << 24;
            fc->scache_pos1++;
            if (temp != _FIELD_VFP_DATA_END) {
                fc->l2warm = 0;
                rv = BCM_E_INTERNAL;
            }
        }
 
    }

    _field_tr2_stage_reinit_all_groups_cleanup(unit, fc,
                                               _BCM_FIELD_STAGE_LOOKUP,
                                               NULL);

cleanup:

    if (vfp_tcam_buf) {
        soc_cm_sfree(unit, vfp_tcam_buf);
    }
    if (vfp_policy_buf) {
        soc_cm_sfree(unit, vfp_policy_buf);
    }
    return rv;
}

#if 0
/* Returns:  1 if any bit of buf is set between offset->offset and
 *             offset->offset + offset->width
 *           0 otherwise
 */
STATIC int
_ext_mask_is_set(int unit,
                 _bcm_field_qual_offset_t *offset,
                 soc_mem_t mem,
                 uint32 *buf
                 )

{
    soc_mem_info_t *meminfo = &SOC_MEM_INFO(unit, mem);
    int len = offset->width, bp = offset->offset;
    uint32 temp;

    switch (mem) {
    case EXT_ACL144_TCAM_L2m:
    case EXT_ACL144_TCAM_IPV4m:
    case EXT_ACL144_TCAM_IPV6m:
        bp += 144;
        break;

    case EXT_ACL288_TCAM_L2m:
    case EXT_ACL288_TCAM_IPV4m:
        bp += 288;
        break;

    default:
        ;
    }

#define FIX_MEM_ORDER_E(v,m) (((m)->flags & SOC_MEM_FLAG_BE) ? \
                                BYTES2WORDS((m)->bytes)-1-(v) : \
                                (v))
    while (len > 0) {
        temp = 0;
        do {
            /* coverity [forward_null] */
            temp =
                (temp << 1) |
                ((buf[FIX_MEM_ORDER_E(bp / 32, meminfo)] >>
                  (bp & (32 - 1))) & 1);
            if (temp > 0) {
                return TRUE;
            }
            len--;
            bp++;
        } while (len & (32 - 1));
    }
    return FALSE;
}
#endif

/***************************************************************************
 *
 * Support for use of external memory as scache
 *
 ***************************************************************************/

#define FIELD_SIZE(t, f)  (sizeof(((t *) 0)->f))

STATIC void
_field_tr2_ext_part_mems(int unit, unsigned part_idx, soc_mem_t *mems)
{
    /* mems[0] = policy mem
       mems[1] = data mem or data-and-mask mem
       mems[2] = mask mem or INVALIDm
    */

    mems[0] = _bcm_field_ext_policy_mems[part_idx];
    if ((mems[1] = _bcm_field_ext_data_mask_mems[part_idx]) == 0) {
        mems[1] = _bcm_field_ext_data_mems[part_idx];
        mems[2] = _bcm_field_ext_mask_mems[part_idx];
    } else {
        mems[2] = INVALIDm;
    }
}


STATIC unsigned
_field_tr2_ext_scache_usable_bytes_per_word(int       unit,
                                            soc_mem_t *mems,
                                            unsigned  mem_idx
                                            )
{
    switch (mem_idx) {
    case 0:                     /* policy mem */
        return (SOC_MEM_BYTES(unit, mems[0]) - 3);
    case 1:                     /* data+mask or data mem */
        return (soc_mem_field_length(unit, mems[1], DATAf) >> 3);
    default:
        ;
    }

    return (0);
}


STATIC uint32
_field_tr2_ext_scache_size_prop_get(int unit, unsigned part_idx)
{
    static const char * const scache_size_props[] = {
        spn_EXT_L2C_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_L2_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_IP4C_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_IP4_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_L2IP4_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_IP6C_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_IP6S_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_IP6F_ACL_TABLE_SCACHE_SIZE,
        spn_EXT_L2IP6_ACL_TABLE_SCACHE_SIZE
    };

    return (soc_property_get(unit, scache_size_props[part_idx], 0xffffffff));
}


unsigned
_field_trx_ext_scache_size(int       unit,
                           unsigned  part_idx,
                           soc_mem_t *mems
                           )
{
    uint32 sz;

    if ((sz = _field_tr2_ext_scache_size_prop_get(unit, part_idx)) == 0xffffffff) {
        /* Scache size not specified => Don't reserve any entries */

        return (0);
    }

    if (sz == 0) {
        /* Scache size specified as 0
           => Calculate space needed to store all entries */
        
        /* Calculate usable bytes (i.e. width) of an entry in TCAM and policy memory
         * - Consider both data and mask mems, as appicable
         * - Last byte in a data or mask entry might be a partial byte (i.e. less
         * than 8 bits), so don't include it
         */
        
        unsigned  i, w, f, n, T;
        
        for (w = 0, i = 0; i < 2; ++i) {
            w += _field_tr2_ext_scache_usable_bytes_per_word(unit, mems, i);
        }
        
        /* Let
           T = total number of entries in slice
           S = number of entries reserved for scache
           E = number of usable entries
           
           Thus, E + S = T
           
           Let
           f = Fixed scache storage required, in bytes
           n = Number of bytes of scache storage per entry
           w = Width of an scache word, in bytes
           
           So, S = ceiling((f + nE) / w)
           
           Therefore, (f + nE) <= wS <= (f + nE + w - 1)
           [aobve as inequalify, all values are whole numbers]
           
           But, E = T - S [from above]
           
           So, (f + nT) <= (n + w)S <= (f + nT + w - 1)
           
           Therefore, S = floor((f + nT + w - 1) / (n + w))
        */
        
        f =
            /* header */
            4                                  
            /* group-present flag */
            + 1                                
            /* group id */
            + FIELD_SIZE(_field_group_t, gid)
            /* group priority */
            + FIELD_SIZE(_field_group_t, priority)
            /* group qset (a guess at a reasonable upper limit) */
            + 20
            /* trailer */
            + 4;                    
        n = 
            /* entry id */
            FIELD_SIZE(_field_entry_t, eid)
            /* priority/stat/policer flags */
            + 1
            /* entry priority */
            + FIELD_SIZE(_field_entry_t, prio)
            /* stat id */
            + FIELD_SIZE(_field_entry_t, statistic.sid)
            /* policer id */
            + FIELD_SIZE(_field_entry_t, policer[0].pid);
        T = soc_mem_index_count(unit, mems[0]);
        
        return((f + n * T + w - 1) / (n + w));
    }

    /* Otherwise, use specified value */
            
    return (sz);
}


int
_bcm_esw_field_tr2_ext_scache_size(int       unit,
                                   unsigned  part_idx
                                   )
{
    soc_mem_t mems[3];
    
    _field_tr2_ext_part_mems(unit, part_idx, mems);
    return (_field_trx_ext_scache_size(unit, part_idx, mems));
}


struct _field_tr2_ext_scache_rw {
    int              unit;
    _field_control_t *fc;
    soc_mem_t mems[3];
    unsigned  scache_nwords, scache_idx_min, scache_idx_max;
    struct _field_tr2_scache_mem_info {
        uint32 bytes_per_word;
        uint32 *buf;
    } mem_info[3];
    struct _field_tr2_scache_chunk {
        uint8  usable_bytes_per_word;
        uint32 *pos, *limit, w;
    } chunk[2];
    unsigned  cur_chunk_idx;
};

STATIC int
_field_tr2_ext_scache_init(struct _field_tr2_ext_scache_rw *h,
                           int                             unit,
                           unsigned                        part_idx
                           )
{
    struct _field_tr2_scache_chunk *ch;
    unsigned n, i;

    sal_memset(h, 0, sizeof(*h));

    h->unit = unit;
    BCM_IF_ERROR_RETURN(_field_control_get(h->unit, &h->fc));
    _field_tr2_ext_part_mems(unit, part_idx, h->mems);
    for (i = 0; i < COUNTOF(h->mems); ++i) {
        if (h->mems[i] == INVALIDm) {
            break;
        }
        h->mem_info[i].bytes_per_word
            = WORDS2BYTES(SOC_MEM_WORDS(h->unit, h->mems[i]));
    }
    h->scache_nwords = _field_trx_ext_scache_size(h->unit,
                                                  part_idx,
                                                  h->mems
                                                  );
    if (h->scache_nwords != 0) {
        /* Set up for ext mem as scache */
        
        h->scache_idx_max = soc_mem_index_max(h->unit, h->mems[0]);
        h->scache_idx_min = h->scache_idx_max + 1 - h->scache_nwords;
        
        for (i = 0, ch = h->chunk, n = COUNTOF(h->chunk); n; --n, ++ch, ++i) {
            ch->usable_bytes_per_word
                = _field_tr2_ext_scache_usable_bytes_per_word(h->unit, h->mems, i);
        }
    }

    return (BCM_E_NONE);
}

STATIC void
_field_tr2_ext_scache_release(struct _field_tr2_ext_scache_rw *h)
{
    unsigned i;

    for (i = 0; i < COUNTOF(h->mem_info); ++i) {
        if (h->mem_info[i].buf) {
            soc_cm_sfree(h->unit, h->mem_info[i].buf);
            
            h->mem_info[i].buf = 0;
        }
    }
}

STATIC int
_field_tr2_ext_scache_wr_init(struct _field_tr2_ext_scache_rw *h,
                              int                             unit,
                              unsigned                        part_idx
                              )
{
    struct _field_tr2_scache_chunk *ch;    
    unsigned n, i;

    _field_tr2_ext_scache_init(h, unit, part_idx);

    if (h->scache_nwords != 0) {
        for (ch = h->chunk, i = 0, n = COUNTOF(h->chunk); n; --n, ++i, ++ch) {
            unsigned s;
            uint32   *buf;
            
            s = h->scache_nwords * h->mem_info[i].bytes_per_word;
            if ((buf = soc_cm_salloc(unit, s, "")) == 0) {
                _field_tr2_ext_scache_release(h);
                return (BCM_E_MEMORY);
            }
            h->mem_info[i].buf = buf;
            
            ch->pos   = buf;
            ch->limit = (uint32 *)((uint8 *) buf + s);
        }
    }

    return (BCM_E_NONE);
}

STATIC int
_field_tr2_ext_scache_wr(struct _field_tr2_ext_scache_rw *h, uint8 val)
{
    if (h->scache_nwords != 0) {
        struct _field_tr2_scache_chunk *ch = &h->chunk[h->cur_chunk_idx];    
        unsigned sh;
        uint32   *p;
        
        if (ch->pos >= ch->limit) {
            if (++h->cur_chunk_idx >= COUNTOF(h->chunk)) {
                _field_tr2_ext_scache_release(h);
                return (BCM_E_FAIL);
            }
            ch = &h->chunk[h->cur_chunk_idx];
        }
        
        sh = (ch->w & 3) << 3;
        p  = &ch->pos[ch->w >> 2];
        *p = (*p & ~(0xff << sh)) | val << sh;
        
        if (++ch->w >= ch->usable_bytes_per_word) {
            ch->pos = (uint32 *)((uint8 *) ch->pos
                                 + h->mem_info[h->cur_chunk_idx].bytes_per_word
                                 );
            ch->w = 0;
        }
    } else {
        _field_control_t *fc = h->fc;
        uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
        buf[fc->scache_pos] = val;
        ++fc->scache_pos;
    }

    return (BCM_E_NONE);
}

STATIC int
_field_tr2_ext_scache_wr_uint(struct _field_tr2_ext_scache_rw *h,
                              uint32                          val,
                              unsigned                        nbytes
                              )
{
    return (_field_tr2_ext_scache_wr(h, val) != BCM_E_NONE
            || (nbytes > 1 && _field_tr2_ext_scache_wr(h, val >> 8) != BCM_E_NONE)
            || (nbytes > 2 && _field_tr2_ext_scache_wr(h, val >> 16) != BCM_E_NONE)
            || (nbytes > 2 && _field_tr2_ext_scache_wr(h, val >> 24) != BCM_E_NONE)
            ? BCM_E_FAIL : BCM_E_NONE
            );
}

#define _FIELD_TR2_EXT_SCACHE_WR_UINT(h, x) \
    (_field_tr2_ext_scache_wr_uint((h), (x), sizeof(x)))

STATIC int
_field_tr2_ext_scache_wr_commit(struct _field_tr2_ext_scache_rw *h)
{
    int result = BCM_E_NONE;

    if (h->scache_nwords != 0) {
        uint32 m[(432 + 31) >> 5];
        
        result = soc_mem_write_range(h->unit,
                                     h->mems[0],
                                     MEM_BLOCK_ALL,
                                     h->scache_idx_min,
                                     h->scache_idx_max,
                                     h->mem_info[0].buf
                                     );
        if (result != BCM_E_NONE) {
            goto cleanup;
        }
        
        sal_memset(m, 0xff, sizeof(m));
        
        if (h->mems[2] == INVALIDm) {
            unsigned n;
            uint32   *p;
            
            for (p = h->mem_info[1].buf, n = h->scache_nwords;
                 n;
                 --n, p = (uint32 *)((uint8 *) p + h->mem_info[1].bytes_per_word)
                 ) {
                soc_mem_mask_field_set(h->unit, h->mems[1], p, MASKf, m);
            }
        } else {
            uint32 mm[COUNTOF(m)];
            
            soc_mem_mask_field_set(h->unit, h->mems[2], mm, MASKf, m);
            soc_mem_write(h->unit, h->mems[2], MEM_BLOCK_ALL, 0, mm);
        }
        
        result = soc_mem_write_range(h->unit,
                                     h->mems[1],
                                     MEM_BLOCK_ALL,
                                     h->scache_idx_min,
                                     h->scache_idx_max,
                                     h->mem_info[1].buf
                                     );
        
    cleanup:
        _field_tr2_ext_scache_release(h);
    }

    return (result);
}

STATIC int
_field_tr2_ext_scache_rd_init(struct _field_tr2_ext_scache_rw *h,
                              int                             unit,
                              unsigned                        part_idx
                              )
{
    int      result = BCM_E_NONE;
    unsigned i;

    _field_tr2_ext_scache_init(h, unit, part_idx);

    for (i = 0; i < COUNTOF(h->mem_info); ++i) {
        uint32 *buf;
        
        if (h->mems[i] == INVALIDm) {
            break;
        }
        
        if ((buf = soc_cm_salloc(unit,
                                 soc_mem_index_count(unit, h->mems[0])
                                 * h->mem_info[i].bytes_per_word,
                                 ""
                                 )
             )
            == 0
            ) {
            _field_tr2_ext_scache_release(h);
            return (BCM_E_MEMORY);
        }
        
        h->mem_info[i].buf = buf;
    }
    
    result = soc_mem_read_range(h->unit,
                                h->mems[0],
                                MEM_BLOCK_ANY,
                                soc_mem_index_min(h->unit, h->mems[0]),
                                soc_mem_index_max(h->unit, h->mems[0]),
                                h->mem_info[0].buf
                                );
    
    if (result != BCM_E_NONE) {
        goto done;
    }
    
    if (h->mems[2] == INVALIDm) {
        result = soc_mem_read_range(h->unit,
                                    h->mems[1],
                                    MEM_BLOCK_ANY,
                                    soc_mem_index_min(h->unit, h->mems[1]),
                                    soc_mem_index_max(h->unit, h->mems[1]),
                                    h->mem_info[1].buf
                                    );
    } else {
        unsigned n;
        uint32   *p1, *p2;
        
        for (p1 = h->mem_info[1].buf, p2 = h->mem_info[2].buf,
                 i = soc_mem_index_min(h->unit, h->mems[1]),
                 n = soc_mem_index_count(h->unit, h->mems[1]);
             n;
             --n, ++i,
                 p1 = (uint32 *)((uint8 *) p1 + h->mem_info[1].bytes_per_word),
                 p2 = (uint32 *)((uint8 *) p2 + h->mem_info[2].bytes_per_word)
             ) {
            if ((result = soc_mem_read(h->unit,
                                       h->mems[1],
                                       MEM_BLOCK_ANY,
                                       i,
                                       p1
                                       )
                 )
                != BCM_E_NONE
                || (result = soc_mem_read(h->unit,
                                          h->mems[2],
                                          MEM_BLOCK_ANY,
                                          0,
                                          p2
                                          )
                    )
                != BCM_E_NONE
                ) {
                goto done;
            }
        }
    }

    if (h->scache_nwords != 0) {
        for (i = 0; i < COUNTOF(h->chunk); ++i) {
            h->chunk[i].pos   = (uint32 *)((uint8 *) h->mem_info[i].buf
                                           + h->scache_idx_min
                                           * h->mem_info[i].bytes_per_word
                                           );
            h->chunk[i].limit = (uint32 *)((uint8 *) h->chunk[i].pos
                                           + h->scache_nwords
                                           * h->mem_info[i].bytes_per_word
                                           );
        }
    }
    
 done:
    if (result != BCM_E_NONE) {
        _field_tr2_ext_scache_release(h);   
    }

    return (result);
}

STATIC int
_field_tr2_ext_scache_rd(struct _field_tr2_ext_scache_rw *h, uint8 *val)
{
    if (h->scache_nwords != 0) {
        struct _field_tr2_scache_chunk *ch = &h->chunk[h->cur_chunk_idx];    
        
        if (ch->pos >= ch->limit) {
            if (++h->cur_chunk_idx >= COUNTOF(h->chunk)) {
                _field_tr2_ext_scache_release(h);
                return (BCM_E_FAIL);
            }
            ch = &h->chunk[h->cur_chunk_idx];
        }
        
        *val = ch->pos[ch->w >> 2] >> ((ch->w & 3) << 3);
        
        if (++ch->w >= ch->usable_bytes_per_word) {
            ch->pos = (uint32 *)((uint8 *) ch->pos
                                 + h->mem_info[h->cur_chunk_idx].bytes_per_word
                                 );
            ch->w = 0;
        }
    } else {
        _field_control_t *fc = h->fc;
        uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
        *val = buf[fc->scache_pos];
        ++fc->scache_pos;
    }

    return (BCM_E_NONE);
}

STATIC int
_field_tr2_ext_scache_rd_uint(struct _field_tr2_ext_scache_rw *h,
                              uint32                          *val,
                              unsigned                        nbytes
                              )
{
    uint8  b;
    uint32 result;
    
    BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd(h, &b));
    result = b;
    if (nbytes > 1) {
        BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd(h, &b));
        result |= b << 8;
    }
    if (nbytes > 2) {
        BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd(h, &b));
        result |= b << 16;
        BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd(h, &b));
        result |= b << 24;
    }

    *val = result;

    return (BCM_E_NONE);
}

#define _FIELD_TR2_EXT_SCACHE_RD_UINT(h, x) \
    (_field_tr2_ext_scache_rd_uint((h), &(x), sizeof(x)))

STATIC uint32 *
_field_tr2_ext_scache_rd_rec_ptr(struct _field_tr2_ext_scache_rw *h,
                                 unsigned                        mem_idx,
                                 unsigned                        rec_idx
                                 )
{
    return ((uint32 *)((uint8 *) h->mem_info[mem_idx].buf
                       + rec_idx * h->mem_info[mem_idx].bytes_per_word
                       )
            );
}


STATIC int
_field_tr2_ext_scache_sync_chk(int              unit,
                               _field_control_t *fc,
                               _field_stage_t   *stage_fc
                               )
{
    unsigned slice_idx;

    for (slice_idx = 0; slice_idx < _FP_EXT_NUM_PARTITIONS; ++slice_idx) {
        if (_bcm_esw_field_tr2_ext_scache_size(unit, slice_idx) != 0) {
            return (TRUE);
        }
    }

    return (FALSE);
}


STATIC int
_field_tr2_ext_scache_sync(int              unit,
                           _field_control_t *fc,
                           _field_stage_t   *stage_fc
                           )
{
    unsigned slice_idx;

    for (slice_idx = 0; slice_idx < _FP_EXT_NUM_PARTITIONS; ++slice_idx) {
        _field_slice_t                 *fs;
        _field_group_t                 *fg;
        struct _field_tr2_ext_scache_rw h[1];

        fs = &stage_fc->slices[slice_idx];

        BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr_init(h, unit, slice_idx));
        
        /* Write start sequence */
        BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr_uint(h,
                                                         _FIELD_EXTFP_DATA_START,
                                                         4
                                                         )
                            );
        
        /* Find group for partition (slice) */
        for (fg = fc->groups; fg; fg = fg->next) {
            if (fg->stage_id == _BCM_FIELD_STAGE_EXTERNAL
                && fg->slices->slice_number == slice_idx
                ) {
                break;
            }
        }
        if (fg == 0) {
            /* No group found */
            
            /* Write group-present flag as FALSE */
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr(h, FALSE));
        } else {
            /* Group found */

            unsigned       n, q;
            _field_entry_t **p, *f_ent;
            int            prev_pri = 0;

            /* Write group-present flag as TRUE */
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr(h, TRUE));

            /* Write group id */
            BCM_IF_ERROR_RETURN(_FIELD_TR2_EXT_SCACHE_WR_UINT(h, fg->gid));
            
            /* Write group priority */
            BCM_IF_ERROR_RETURN(_FIELD_TR2_EXT_SCACHE_WR_UINT(h, fg->priority));

            /* Write group qset */
            n = 0;
            _FIELD_QSET_INCL_INTERNAL_ITER(fg->qset, q) {
                ++n;
            }
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr(h, n));
            _FIELD_QSET_INCL_INTERNAL_ITER(fg->qset, q) {
                BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr(h, q));
            }

            /* Write entries */
            for (p = fs->entries, n = fs->entry_count; n; --n, ++p) {
                unsigned ent_prif, ent_statf, ent_polf;

                if ((f_ent = *p) == 0) {
                    continue;
                }

                /* Write entry id */
                BCM_IF_ERROR_RETURN(_FIELD_TR2_EXT_SCACHE_WR_UINT(h,
                                                                 f_ent->eid
                                                                 )
                                    );

                ent_prif = (n == fs->entry_count || f_ent->prio != prev_pri);
                ent_statf = (f_ent->statistic.flags & _FP_ENTRY_STAT_INSTALLED)
                    ? 1 : 0;
                ent_polf = (f_ent->policer[0].flags & _FP_POLICER_INSTALLED)
                    ? 1 : 0;

                BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr(
                                        h,
                                        (ent_prif << 2)
                                            | (ent_statf << 1)
                                            | ent_polf
                                                             )
                                    );
                
                if (ent_prif) {
                    /* First entry, or priority differs from previous entry */

                    /* Write entry priority */
                    BCM_IF_ERROR_RETURN(_FIELD_TR2_EXT_SCACHE_WR_UINT(
                                            h,
                                            f_ent->prio
                                                                      )
                                        );
                }

                if (ent_statf) {
                    /* Stat defined for entry */

                    /* Write stat id */
                    BCM_IF_ERROR_RETURN(_FIELD_TR2_EXT_SCACHE_WR_UINT(
                                            h,
                                            f_ent->statistic.sid
                                                                      )
                                        );
                }
                
                if (ent_polf) {
                    /* Policer defined for entry */

                    /* Write policer id */
                    BCM_IF_ERROR_RETURN(_FIELD_TR2_EXT_SCACHE_WR_UINT(
                                            h,
                                            f_ent->policer[0].pid
                                                                      )
                                        );
                }

                prev_pri = f_ent->prio;
            }
        }

        BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_wr_uint(h,
                                                         _FIELD_EXTFP_DATA_END,
                                                         4
                                                         )
                            );

        _field_tr2_ext_scache_wr_commit(h);
    }

    return (BCM_E_NONE);
}


STATIC int
_field_tr2_ext_slice_to_pkt_type(_field_slice_t *fs, unsigned *pkt_type)
{
    switch (fs->slice_number) {
    case _FP_EXT_ACL_144_L2:
    case _FP_EXT_ACL_L2:
        *pkt_type = _FP_EXT_L2;
        break;
    case _FP_EXT_ACL_144_IPV4:
    case _FP_EXT_ACL_IPV4:
    case _FP_EXT_ACL_L2_IPV4:
        *pkt_type = _FP_EXT_IP4;
        break;
    case _FP_EXT_ACL_144_IPV6:
    case _FP_EXT_ACL_IPV6_SHORT:
    case _FP_EXT_ACL_IPV6_FULL:
    case _FP_EXT_ACL_L2_IPV6:
        *pkt_type = _FP_EXT_IP6;
        break;
    default:
        return (BCM_E_INTERNAL);
    }

    return (BCM_E_NONE);
}


STATIC int
_field_tr2_stage_ext_group_construct(
    int               unit,
    _field_control_t  *fc,
    _field_stage_t    *stage_fc,
    _field_slice_t    *fs,
    unsigned          pkt_type,
    bcm_field_group_t gid,
    int               pri,
    bcm_field_qset_t  *qset,
    _field_group_t    **pfg
                                          )
{
    int                    result = BCM_E_NONE;
    _field_group_t         *fg = 0;
    unsigned               i, j;
    bcm_port_t             port;
    unsigned               qual_idx;
    _bcm_field_qual_info_t *f_qual_arr;

    _FP_XGS3_ALLOC(fg, sizeof(*fg), "field group");
    if (fg == NULL) {
        result = BCM_E_MEMORY;
        goto done;
    } 
    
    fg->gid      = gid;
    fg->priority = pri;
    fg->qset     = *qset;
                
    /* Recover group's qual_arr */
                
    for (fg->qual_arr[0].size = qual_idx = 0; qual_idx < _bcmFieldQualifyCount; ++qual_idx) {
        if (BCM_FIELD_QSET_TEST(fg->qset, qual_idx)) {
            ++fg->qual_arr[0].size;
        }
    }

    _FP_XGS3_ALLOC(fg->qual_arr[0].qid_arr, fg->qual_arr[0].size * sizeof(fg->qual_arr[0].qid_arr[0]),
                   "Group qual id"
                   );
    _FP_XGS3_ALLOC(fg->qual_arr[0].offset_arr, fg->qual_arr[0].size * sizeof(fg->qual_arr[0].offset_arr[0]),
                   "Group qual offset"
                   );
                
    for (i = qual_idx = 0; qual_idx < _bcmFieldQualifyCount; ++qual_idx) {
        if (!BCM_FIELD_QSET_TEST(fg->qset, qual_idx)) {
            continue;
        }

        fg->qual_arr[0].qid_arr[i] = qual_idx;

        f_qual_arr = stage_fc->f_qual_arr[qual_idx];
                    
        for (j = 0; j < f_qual_arr->conf_sz; ++j) {
            /* assert(f_qual_arr->conf_arr[i].selector.pri_sel) == _bcmFieldSliceSelExternal */
                        
            if (f_qual_arr->conf_arr[j].selector.pri_sel_val == fs->slice_number) {
                fg->qual_arr[0].offset_arr[i] = f_qual_arr->conf_arr[j].offset;                           
                            
                break;
            }
        }

        ++i;
    }

    fg->stage_id          =  stage_fc->stage_id;
    for (i = 0; i < COUNTOF(fg->sel_codes); ++i) {
        _FIELD_SELCODE_CLEAR(fg->sel_codes[i]);
    }
    fg->sel_codes[0].intraslice = fg->sel_codes[0].secondary = _FP_SELCODE_DONT_USE;
    fg->sel_codes[0].extn =  fs->slice_number;
    fg->flags             = _FP_GROUP_SPAN_SINGLE_SLICE | _FP_GROUP_LOOKUP_ENABLED;
    fg->slices            =  fs;
    fs->group_flags       = _FP_GROUP_SPAN_SINGLE_SLICE;

    /* Recover ports for group */
                
    BCM_PBMP_CLEAR(fg->pbmp);                
    PBMP_ALL_ITER(unit, port) {
        uint32 esm_port_mode;
                    
        result = soc_reg32_read(unit, soc_reg_addr(unit, ESM_MODE_PER_PORTr, port, 0), &esm_port_mode);
        if (result != BCM_E_NONE) {
            goto done;
        }

        switch (pkt_type) {
        case _FP_EXT_L2:
            if (soc_reg_field_get(unit, ESM_MODE_PER_PORTr, esm_port_mode, L2_ACL_ENf) != 0) {
                BCM_PBMP_PORT_ADD(fg->pbmp, port);
            }
            break;
        case _FP_EXT_IP4:
            if (soc_reg_field_get(unit, ESM_MODE_PER_PORTr, esm_port_mode, IPV4_ACL_MODEf) != 0) {
                BCM_PBMP_PORT_ADD(fg->pbmp, port);
            }
            break;
        case _FP_EXT_IP6:
            if (soc_reg_field_get(unit, ESM_MODE_PER_PORTr, esm_port_mode, IPV6_ACL_MODEf) != 0) {
                BCM_PBMP_PORT_ADD(fg->pbmp, port);
            }
            break;
        default:
            ;
        }
    }

    fs->pbmp = fg->pbmp;

    _bcm_field_group_status_init(unit, &fg->group_status);

    /* Hook new group into stage */

    fg->next = fc->groups;
    fc->groups = fg;

 done:
    if (result == BCM_E_NONE) {
        *pfg = fg;
    } else if (fg) {
        sal_free(fg);
    }

    return (result);
}


STATIC int
_field_tr2_stage_external_reinit_old(int              unit,
                                     _field_control_t *fc,
                                     _field_stage_t   *stage_fc
                                     )
{
    int      rv = BCM_E_NONE, i, j;
    unsigned slice_idx, cum_slice_entry_cnt[TCAM_PARTITION_ACL_L2IP6 + 1 - TCAM_PARTITION_ACL_L2];
    _field_group_t *fg    = 0;
    _field_entry_t *f_ent = 0;

    for (j = 0, i = stage_fc->tcam_slices - 1; i >= 0; --i) {
        cum_slice_entry_cnt[i] = j + stage_fc->slices[i].entry_count;
        j = cum_slice_entry_cnt[i];
    }

    /* For each slice, ... */

    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; ++slice_idx) {
        _field_slice_t *fs = &stage_fc->slices[slice_idx];
        int            port;
        unsigned       pkt_type=0, entry_idx, entry_cnt;
        soc_mem_t      tcam_data_mask_mem = _bcm_field_ext_data_mask_mems[slice_idx];
        soc_mem_t      tcam_data_mem      = _bcm_field_ext_data_mems[slice_idx];
        soc_mem_t      tcam_mask_mem      = _bcm_field_ext_mask_mems[slice_idx];
        soc_mem_t      policy_mem         = _bcm_field_ext_policy_mems[slice_idx];

        switch (slice_idx) {
        case _FP_EXT_ACL_144_L2:
        case _FP_EXT_ACL_L2:
            pkt_type = _FP_EXT_L2;
            break;
        case _FP_EXT_ACL_144_IPV4:
        case _FP_EXT_ACL_IPV4:
        case _FP_EXT_ACL_L2_IPV4:
            pkt_type = _FP_EXT_IP4;
            break;
        case _FP_EXT_ACL_144_IPV6:
        case _FP_EXT_ACL_IPV6_SHORT:
        case _FP_EXT_ACL_IPV6_FULL:
        case _FP_EXT_ACL_L2_IPV6:
            pkt_type = _FP_EXT_IP6;
            break;
        default:
            ;
        }

        /* Find all valid entries in slice's TCAM */

        for (entry_cnt = entry_idx = 0;
             entry_idx < fs->entry_count;
             ++entry_idx
             ) {
            ext_fp_policy_entry_t          policy_buf;
            ext_ifp_action_profile_entry_t ext_ifp_action_buf;
            bcm_field_entry_t eid;
            int               prio;
            int               multigroup=0;
            int               prev_prio = -1;
            bcm_field_stat_t  sid = -1;
            bcm_policer_t     pid = -1;
            unsigned          qual_idx;
            _bcm_field_qual_info_t *f_qual_arr;

            rv = soc_mem_read(unit, policy_mem, MEM_BLOCK_ANY, entry_idx, policy_buf.entry_data);
            if (rv != BCM_E_NONE) {
                goto cleanup;
            }
            if (soc_mem_field32_get(unit, policy_mem, policy_buf.entry_data, VALIDf) == 0) {
                /* TCAM entry not in use => Skip */

                continue;
            }

            /* TCAM entry in use */

            if (entry_cnt == 0) {
                /* First in-use entry found => Create group */

                int multigroup = FALSE; /* Will always be FALSE for external */
                bcm_field_group_t gid;
                int               priority;
                bcm_field_qset_t  qset;

                /* If Level 2, retrieve the GIDs in this slice */
                /* For external, will be exactly 1 group */
                if (fc->l2warm) {
                    rv = _field_trx_scache_slice_group_recover(unit,
                                                               fc,
                                                               slice_idx,
                                                               &multigroup,
                                                               stage_fc
                                                               );
                    if (BCM_FAILURE(rv)) {
                        goto cleanup;
                    }
                }

                _FP_XGS3_ALLOC(fg, sizeof(*fg), "field group");
                if (fg == NULL) {
                    return BCM_E_MEMORY;
                }

                if (fc->l2warm) {
                    /* Get stored group ID and QSET for Level 2 */
                    rv = _field_group_info_retrieve(unit,
                                                    0 /* port */,
                                                    &gid,
                                                    &priority,
                                                    &qset,
                                                    fc
                                                    );
                    if (gid != -1) {
                        sal_memcpy(&fg->qset, &qset, sizeof(fg->qset));
                    }
                } else {
                    /* Generate group ID (a ++ operation) for Level 1 */

                    if ((rv = _field_group_id_generate(unit, &gid)) == BCM_E_NONE) {
                        unsigned vmap, vslice;

                        for (priority = -1, vmap = 0; priority == -1 && vmap < _FP_VMAP_CNT; ++vmap) {
                            for (vslice = 0; vslice < COUNTOF(stage_fc->vmap[0]); ++vslice) {
                                if (stage_fc->vmap[vmap][vslice].vmap_key == slice_idx) {
                                    priority = stage_fc->vmap[vmap][vslice].priority;

                                    break;
                                }
                            }
                        }

                        if (priority == -1) {
                            rv = BCM_E_INTERNAL;
                        }
                    }

                    /* Recover group's qset */

                    BCM_FIELD_QSET_INIT(fg->qset);
                    BCM_FIELD_QSET_ADD(fg->qset, bcmFieldQualifyStage);
                    BCM_FIELD_QSET_ADD(fg->qset, bcmFieldQualifyStageExternal);

                    for (qual_idx = 0; qual_idx < _bcmFieldQualifyCount; ++qual_idx) {
                        f_qual_arr = stage_fc->f_qual_arr[qual_idx];

                        if (f_qual_arr == NULL) {
                            continue; /* Qualifier does not exist in this stage */
                        }

                        for (i = 0; i < f_qual_arr->conf_sz; ++i) {
                            /* assert(f_qual_arr->conf_arr[i].selector.pri_sel) == _bcmFieldSliceSelExternal */

                            if (f_qual_arr->conf_arr[i].selector.pri_sel_val == slice_idx) {
                                BCM_FIELD_QSET_ADD(fg->qset, qual_idx);

                                break;
                            }
                        }
                    }
                }

                /* Recover group's qual_arr */

                for (fg->qual_arr[0].size = qual_idx = 0; qual_idx < _bcmFieldQualifyCount; ++qual_idx) {
                    if (BCM_FIELD_QSET_TEST(fg->qset, qual_idx)) {
                        ++fg->qual_arr[0].size;
                    }
                }

                _FP_XGS3_ALLOC(fg->qual_arr[0].qid_arr, fg->qual_arr[0].size * sizeof(fg->qual_arr[0].qid_arr[0]),
                               "Group qual id"
                               );
                _FP_XGS3_ALLOC(fg->qual_arr[0].offset_arr, fg->qual_arr[0].size * sizeof(fg->qual_arr[0].offset_arr[0]),
                               "Group qual offset"
                               );

                for (j = qual_idx = 0; qual_idx < _bcmFieldQualifyCount; ++qual_idx) {
                    if (!BCM_FIELD_QSET_TEST(fg->qset, qual_idx)) {
                        continue;
                    }

                    fg->qual_arr[0].qid_arr[j] = qual_idx;

                    f_qual_arr = stage_fc->f_qual_arr[qual_idx];

                    for (i = 0; i < f_qual_arr->conf_sz; ++i) {
                        /* assert(f_qual_arr->conf_arr[i].selector.pri_sel) == _bcmFieldSliceSelExternal */

                        if (f_qual_arr->conf_arr[i].selector.pri_sel_val == slice_idx) {
                            fg->qual_arr[0].offset_arr[j] = f_qual_arr->conf_arr[i].offset;

                            break;
                        }
                    }

                    ++j;
                }

                fg->gid               =  gid;
                fg->priority          =  priority;
                fg->stage_id          =  stage_fc->stage_id;
                fg->sel_codes[0].extn =  slice_idx;
                fg->slices            =  fs;
                fg->flags             |= _FP_GROUP_SPAN_SINGLE_SLICE | _FP_GROUP_LOOKUP_ENABLED;

                /* Recover ports for group */

                BCM_PBMP_CLEAR(fg->pbmp);
                PBMP_ALL_ITER(unit, port) {
                    uint32 esm_port_mode;

                    rv = soc_reg32_read(unit, soc_reg_addr(unit, ESM_MODE_PER_PORTr, port, 0), &esm_port_mode);
                    if (rv != BCM_E_NONE) {
                        goto cleanup;
                    }

                    switch (pkt_type) {
                    case _FP_EXT_L2:
                        if (soc_reg_field_get(unit, ESM_MODE_PER_PORTr, esm_port_mode, L2_ACL_ENf) != 0) {
                            BCM_PBMP_PORT_ADD(fg->pbmp, port);
                        }
                        break;
                    case _FP_EXT_IP4:
                        if (soc_reg_field_get(unit, ESM_MODE_PER_PORTr, esm_port_mode, IPV4_ACL_MODEf) != 0) {
                            BCM_PBMP_PORT_ADD(fg->pbmp, port);
                        }
                        break;
                    case _FP_EXT_IP6:
                        if (soc_reg_field_get(unit, ESM_MODE_PER_PORTr, esm_port_mode, IPV6_ACL_MODEf) != 0) {
                            BCM_PBMP_PORT_ADD(fg->pbmp, port);
                        }
                        break;
                    default:
                        ;
                    }
                }

                fs->pbmp = fg->pbmp;

                /* Hook new group into stage */

                fg->next = fc->groups;
                fc->groups = fg;
            }

            /* Add entry to group */

            _FP_XGS3_ALLOC(f_ent, sizeof(*f_ent), "field entry");
            if (f_ent == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }

            if (fc->l2warm) {
                rv = _field_trx_entry_info_retrieve(unit,
                                                    &eid,
                                                    &prio,
                                                    fc,
                                                    multigroup,
                                                    &prev_prio,
                                                    &sid,
                                                    &pid,
                                                    stage_fc
                                                    );
                
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
            } else {
                _bcm_field_last_alloc_eid_incr();
            }

            if (fc->l2warm) {
                /* Use retrieved EID */
                f_ent->eid = eid;
            } else {
                f_ent->eid = _bcm_field_last_alloc_eid_get();
            }
            f_ent->group = fg;

            /* ???
            BCM_PBMP_ASSIGN(f_ent[i].pbmp.data, entry_pbmp);
            BCM_PBMP_ASSIGN(f_ent[i].pbmp.mask, PBMP_ALL(unit));
            */

            if (fc->flags & _FP_COLOR_INDEPENDENT) {
                f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
            }

            f_ent->flags     = _FP_ENTRY_PRIMARY;
            f_ent->slice_idx = entry_idx;
            f_ent->fs        = fs;

            f_ent->tcam.key_size = ((soc_mem_field_length(unit, tcam_data_mask_mem ? tcam_data_mask_mem : tcam_data_mem, DATAf) + 31) & ~31) >> (5 - 2);

            f_ent->tcam.key_hw  = sal_alloc(f_ent->tcam.key_size, "TCAM data buf");
            f_ent->tcam.mask_hw = sal_alloc(f_ent->tcam.key_size, "TCAM mask buf");

            if (tcam_data_mask_mem != 0) {
                ext_acl288_tcam_entry_t tcam_buf;

                rv = soc_mem_read(unit, tcam_data_mask_mem, MEM_BLOCK_ANY, entry_idx, tcam_buf.entry_data);
                if (rv != BCM_E_NONE) {
                    goto cleanup;
                }
                soc_mem_field_get(unit, tcam_data_mask_mem, tcam_buf.entry_data, DATAf, f_ent->tcam.key_hw);
                soc_mem_mask_field_get(unit, tcam_data_mask_mem, tcam_buf.entry_data, MASKf, f_ent->tcam.mask_hw);
            } else {
                ext_acl432_tcam_data_entry_t tcam_buf;

                rv = soc_mem_read(unit, tcam_data_mem, MEM_BLOCK_ANY, entry_idx, tcam_buf.entry_data);
                if (rv != BCM_E_NONE) {
                    goto cleanup;
                }
                soc_mem_field_get(unit, tcam_data_mem, tcam_buf.entry_data, DATAf, f_ent->tcam.key_hw);

                /* Depth of EXT_ACL432_TCAM_MASKm table is one, i.e. has only one valid index */
                rv = soc_mem_read(unit, tcam_mask_mem, MEM_BLOCK_ANY, 0, tcam_buf.entry_data);
                if (rv != BCM_E_NONE) {
                    goto cleanup;
                }
                soc_mem_mask_field_get(unit, tcam_mask_mem, tcam_buf.entry_data, MASKf, f_ent->tcam.mask_hw);
            }

            /* Assign entry to a slice */
            fs->entries[entry_idx] = f_ent;
            f_ent->flags |= _FP_ENTRY_INSTALLED;

            /* Recover actions */

            _field_tr2_actions_recover(unit,
                                       policy_mem,
                                       policy_buf.entry_data,
                                       f_ent,
                                       0,
                                       sid,
                                       pid
                                       );

            /* Recover actions in EXT_IFP_ACTION_PROFILEm */

            rv = soc_mem_read(unit,
                         EXT_IFP_ACTION_PROFILEm,
                         MEM_BLOCK_ANY,
                         soc_mem_field32_get(unit, policy_mem, policy_buf.entry_data, PROFILE_PTRf),
                         ext_ifp_action_buf.entry_data
                         );
            if (rv != BCM_E_NONE) {
                goto cleanup;
            }
            _field_tr2_actions_recover(unit,
                                       EXT_IFP_ACTION_PROFILEm,
                                       ext_ifp_action_buf.entry_data,
                                       f_ent,
                                       0,
                                       sid,
                                       pid
                                       );

            /* Add to the group */
            if (fc->l2warm) {
                f_ent->prio = prio;
            } else {
                f_ent->prio = cum_slice_entry_cnt[slice_idx] - entry_idx;
            }
            rv = _field_group_entry_add(unit, fg, f_ent);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }

            f_ent = 0;

            ++entry_cnt;
        }

        /* Free up the temporary slice group info */
        if (fc->l2warm) {
            _field_scache_slice_group_free(unit,
                                           fc,
                                           slice_idx
                                           );
        }

        fg = 0;
    }

 cleanup:
    if (f_ent) {
        sal_free(f_ent);
    }
    if (fg) {
        sal_free(fg);
    }

    return (rv);
}


int
_field_tr2_stage_external_reinit(int              unit,
                                 _field_control_t *fc, 
                                 _field_stage_t   *stage_fc
                                 )
{
    int      result = BCM_E_NONE;
    int      i, j;
    unsigned slice_idx;
    unsigned cum_slice_entry_cnt[
                 TCAM_PARTITION_ACL_L2IP6 + 1 - TCAM_PARTITION_ACL_L2
                                 ];
    _field_group_t *fg    = 0;
    _field_entry_t *f_ent = 0;

    if (!_field_tr2_ext_scache_sync_chk(unit, fc, stage_fc)) {
        return (_field_tr2_stage_external_reinit_old(unit, fc, stage_fc));
    }

    /* Compute cumulative entry counts (number of entries in each and all
       preceding slices) */

    for (j = 0, i = stage_fc->tcam_slices - 1; i >= 0; --i) {
        cum_slice_entry_cnt[i] = j + stage_fc->slices[i].entry_count;
        j = cum_slice_entry_cnt[i];
    }

    /* For each slice, ... */

    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; ++slice_idx) {
        _field_slice_t *fs = &stage_fc->slices[slice_idx];
        unsigned       entry_idx, entry_cnt;
        struct _field_tr2_ext_scache_rw h[1];
        bcm_field_group_t gid;
        bcm_field_qset_t  qset;
        int               pri, sid = -1, pid = -1;
        unsigned          n, pkt_type = 0;
        uint32 x;

        _field_tr2_ext_slice_to_pkt_type(fs, &pkt_type);

        _field_tr2_ext_scache_rd_init(h, unit, slice_idx);

        if (fc->l2warm) {
            /* Check for start sequence */
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd_uint(h, &x, 4));
            if (x != _FIELD_EXTFP_DATA_START) {
                /* Start sequence not found */

                result = BCM_E_INTERNAL;
                goto error;
            }

            /* Read group-present flag */
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd_uint(h, &x, 1));
            if (!x) {
                /* Group not present */

                /* Skip to next slice */
                goto slice_done;
            }

            /* Group is present */

            /* Read group's id, priority and qset */
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd_uint(
                                    h,
                                    &x,
                                    sizeof(fg->gid)
                                                              )
                                );
            gid = (bcm_field_group_t) x;
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd_uint(
                                    h,
                                    &x,
                                    sizeof(fg->priority)
                                                              )
                                );
            pri = (bcm_field_group_t) x;

            bcm_field_qset_t_init(&qset);
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd_uint(h, &x, 1));
            for (n = x; n; --n) {
                BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd_uint(h, &x, 1));
                BCM_FIELD_QSET_ADD(qset, x);
            }

            /* Create group */
            _field_tr2_stage_ext_group_construct(unit,
                                                 fc,
                                                 stage_fc,
                                                 fs,
                                                 pkt_type,
                                                 gid,
                                                 pri,
                                                 &qset,
                                                 &fg
                                                 ); 
        }

        /* Find all valid entries in slice's TCAM */
        
        for (entry_cnt = entry_idx = 0;
             entry_idx < fs->entry_count;
             ++entry_idx
             ) {
            uint32                         *policy;
            ext_ifp_action_profile_entry_t ext_ifp_action_buf;
            int               prev_pri = 0;
            unsigned          profile_ptr;

            policy = _field_tr2_ext_scache_rd_rec_ptr(h, 0, entry_idx);
            if (soc_mem_field32_get(unit, h->mems[0], policy, VALIDf) == 0) {
                /* TCAM entry not in use => Skip */

                continue;
            }

            /* TCAM entry in use */
            
            if (entry_cnt == 0 && !fc->l2warm) {
                /* First in-use entry found, and level-1 warm start
                   => Create group
                */

                unsigned               qual_idx;
                _bcm_field_qual_info_t *f_qual_arr;
                _field_stage_t         *stage_fc_ifp;
                unsigned               vslice;
                
                /* Generate group ID (a ++ operation) for Level 1 */
                
                result = _field_group_id_generate(unit, &gid);
                if (result != BCM_E_NONE) {
                    goto error;
                }
                
                /* Recover group's qset */
                
                BCM_FIELD_QSET_INIT(qset);
                BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStage);
                BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageExternal);
                
                for (qual_idx = 0;
                     qual_idx < _bcmFieldQualifyCount;
                     ++qual_idx
                     ) {
                    f_qual_arr = stage_fc->f_qual_arr[qual_idx];
                    
                    if (f_qual_arr == NULL) {
                        continue; /* Qualifier does not exist in this stage */
                    }
                    
                    for (i = 0; i < f_qual_arr->conf_sz; ++i) {
                        /* assert(f_qual_arr->conf_arr[i].selector.pri_sel)
                           == _bcmFieldSliceSelExternal */
                        
                        if (f_qual_arr->conf_arr[i].selector.pri_sel_val
                            == slice_idx
                            ) {
                            BCM_FIELD_QSET_ADD(qset, qual_idx);
                            
                            break;
                        }
                    }
                }
                
                result = _field_stage_control_get(unit,
                                                  _BCM_FIELD_STAGE_INGRESS,
                                                  &stage_fc_ifp
                                                  );
                if (result != BCM_E_NONE) {
                    goto error;
                }

                for (pri = -1, vslice = 0;
                     vslice < COUNTOF(stage_fc_ifp->vmap[0]);
                     ++vslice
                     ) {
                    if (stage_fc_ifp->vmap[pkt_type][vslice].vmap_key
                        == slice_idx
                        ) {
                        pri = stage_fc->vmap[pkt_type][vslice].priority;
                        
                        break;
                    }
                }
                
                if (pri == -1) {
                    result = BCM_E_INTERNAL;
                    goto error;
                }

                _field_tr2_stage_ext_group_construct(unit,
                                                     fc,
                                                     stage_fc,
                                                     fs,
                                                     pkt_type,
                                                     gid,
                                                     pri,
                                                     &qset,
                                                     &fg
                                                     );
            }
            
            _FP_XGS3_ALLOC(f_ent, sizeof(*f_ent), "field entry");
            if (f_ent == NULL) {
                result = BCM_E_MEMORY;
                goto error;
            }

            if (fc->l2warm) {
                /* Level-2 warm start */

                uint8 f;

                /* Read entry id */
                _field_tr2_ext_scache_rd_uint(h, &x, sizeof(f_ent->eid));
                f_ent->eid = (bcm_field_entry_t) x;

                /* Read priority/stat/policer flags */
                result = _field_tr2_ext_scache_rd(h, &f);
                if (result != BCM_E_NONE) {
                    goto error;
                }

                if (f & (1 << 2)) {
                    /* Read entry priority */
                    _field_tr2_ext_scache_rd_uint(h, &x, sizeof(f_ent->prio));
                    prev_pri = f_ent->prio = (int) x;
                } else {
                    /* Assign entry same priority as previous entry */
                    f_ent->prio = prev_pri;
                }
                if (f & (1 << 1)) {
                    /* Read stat id */
                    _field_tr2_ext_scache_rd_uint(h,
                                                  &x, 
                                                  sizeof(f_ent->statistic.sid)
                                                  );
                    sid = (int) x;
                }
                if (f & (1 << 0)) {
                    /* Read policer id */
                    _field_tr2_ext_scache_rd_uint(h,
                                                  &x,
                                                  sizeof(f_ent->policer[0].pid)
                                                  );
                    pid = (bcm_policer_t) x;
                }
            } else {
                /* Level-1 warm start */

                /* Assign arbitary (last + 1) entry id */
                _bcm_field_last_alloc_eid_incr();
                f_ent->eid = _bcm_field_last_alloc_eid_get();

                /* Assign entry priority based on position in TCAM */
                f_ent->prio = cum_slice_entry_cnt[slice_idx] - entry_idx;
            }

            /* ???
            BCM_PBMP_ASSIGN(f_ent[i].pbmp.data, entry_pbmp);
            BCM_PBMP_ASSIGN(f_ent[i].pbmp.mask, PBMP_ALL(unit));
            */

            if (fc->flags & _FP_COLOR_INDEPENDENT) {
                f_ent->flags |= _FP_ENTRY_COLOR_INDEPENDENT;
            }

            f_ent->flags     = _FP_ENTRY_PRIMARY;
            f_ent->slice_idx = entry_idx;
            f_ent->fs        = fs;

            f_ent->tcam.key_size = ((soc_mem_field_length(unit, 
                                                          h->mems[1],
                                                          DATAf
                                                          )
                                     + 31
                                     )
                                    & ~31
                                    )
                >> (5 - 2);

            f_ent->tcam.key_hw  = sal_alloc(f_ent->tcam.key_size, "TCAM data buf");
            f_ent->tcam.mask_hw = sal_alloc(f_ent->tcam.key_size, "TCAM mask buf");

            if (h->mems[2] == INVALIDm) {
                uint32 *p;

                p = _field_tr2_ext_scache_rd_rec_ptr(h, 1, entry_idx);
                soc_mem_field_get(unit,
                                  h->mems[1],
                                  p,
                                  DATAf,
                                  f_ent->tcam.key_hw
                                  );
                soc_mem_mask_field_get(unit,
                                       h->mems[1],
                                       p,
                                       MASKf,
                                       f_ent->tcam.mask_hw
                                       );
            } else {
                soc_mem_field_get(unit,
                                  h->mems[1],
                                  _field_tr2_ext_scache_rd_rec_ptr(
                                      h,
                                      1,
                                      entry_idx
                                                                   ),
                                  DATAf,
                                  f_ent->tcam.key_hw
                                  );

                soc_mem_mask_field_get(unit,
                                       h->mems[2],
                                       _field_tr2_ext_scache_rd_rec_ptr(
                                           h,
                                           2,
                                           entry_idx
                                                                        ),
                                       MASKf,
                                       f_ent->tcam.mask_hw
                                       );
            }

            /* Assign entry to a slice */
            fs->entries[entry_idx] = f_ent;
            f_ent->flags |= _FP_ENTRY_INSTALLED;
            --fs->free_count;

            /* Assign entry to group */
            f_ent->group = fg;

            /* Recover actions */

            _field_tr2_actions_recover(unit,
                                       h->mems[0],
                                       policy,
                                       f_ent,
                                       0, 
                                       sid,
                                       pid
                                       );

            /* Recover actions in EXT_IFP_ACTION_PROFILEm */

            profile_ptr = soc_mem_field32_get(unit,
                                              h->mems[0],
                                              policy,
                                              PROFILE_PTRf
                                              );

            soc_profile_mem_reference(unit,
                                      &stage_fc->ext_act_profile,
                                      profile_ptr,
                                      1
                                      );

            result = soc_mem_read(unit,
                                  EXT_IFP_ACTION_PROFILEm,
                                  MEM_BLOCK_ANY,
                                  profile_ptr,
                                  ext_ifp_action_buf.entry_data
                                  );
            if (result != BCM_E_NONE) {
                goto error;
            }
            _field_tr2_actions_recover(unit,
                                       EXT_IFP_ACTION_PROFILEm,
                                       ext_ifp_action_buf.entry_data,
                                       f_ent,
                                       0,
                                       sid,
                                       pid
                                       );

            /* Add entry to the group */
            result = _field_group_entry_add(unit, fg, f_ent);
            if (BCM_FAILURE(result)) {
                sal_free(f_ent);
                goto error;
            }

            f_ent = 0;

            ++entry_cnt;
        }
        
    slice_done:
        result = _bcm_field_prio_mgmt_slice_reinit(unit, stage_fc, fs);
        if (BCM_FAILURE(result)) {
            goto error;
        }

        if (fc->l2warm) {
            uint32 x;

            /* Check for end sequence */
            BCM_IF_ERROR_RETURN(_field_tr2_ext_scache_rd_uint(h, &x, 4));
            if (x != _FIELD_EXTFP_DATA_END) {
                /* End sequence not found */

                /* Indicate error */
                result = BCM_E_INTERNAL;
            }
        }

    error:
        _field_tr2_ext_scache_release(h);

        if (result != BCM_E_NONE) {
            break;
        }

        fg = 0;
    }

    if (f_ent) {
        sal_free(f_ent);
    }
    if (fg) {
        sal_free(fg);
    }

    return (result);
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#else /* BCM_TRX_SUPPORT && BCM_FIELD_SUPPORT */
int _trx_field_not_empty;
#endif  /* BCM_TRX_SUPPORT && BCM_FIELD_SUPPORT */

