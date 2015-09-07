/*
 * $Id: field.c 1.26 Broadcom SDK $
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
 * Purpose:     BCM56450 Field Processor installation functions.
 */
   
#include <soc/defs.h>
#if defined(BCM_KATANA2_SUPPORT) && defined(BCM_FIELD_SUPPORT)

#include <soc/mem.h>
#include <soc/drv.h>

#include <bcm/error.h>
#include <bcm/l3.h>
#include <bcm/field.h>
#include <bcm/tunnel.h>

#include <bcm_int/esw_dispatch.h>

#include <bcm_int/esw/field.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/katana.h>
#include <bcm_int/esw/katana2.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>

#define ALIGN32(x)      (((x) + 31) & ~31)

/* local/static function prototypes */
STATIC void _field_kt2_functions_init(_field_funct_t *functions) ;

/*
 * Function:
 *     _field_kt2_ingress_qualifiers_init
 * Purpose:
 *     Initialize device stage ingress qaualifiers 
 *     select codes & offsets
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     stage_fc   - (IN) Field Processor stage control structure. 
 *
 * Returns:
 *     BCM_E_NONE
 */
STATIC int
_field_kt2_ingress_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    const unsigned
        f1_ofs                  = 194 - 2,
        f2_ofs                  = 66 - 2,
        f3_ofs                  = 26 - 2,
        dw_f1_ofs               = 171 - 3 + 1,
        dw_f2_ofs               = 43 - 3 + 1,
        dw_f3_ofs               = 11 - 3 + 1,
        dw_f4_ofs               = 3 - 3 + 1,
        fp_global_mask_tcam_ofs =
                    ALIGN32(soc_mem_field_length(unit, FP_TCAMm, DATA_KEYf)),
        fixed_ofs               = 172 - 2;

    _FP_QUAL_DECL;

    /* Input parameters check. */
    if (NULL == stage_fc) {
        return (BCM_E_PARAM);
    }

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyStage,
                 _bcmFieldSliceSelDisable, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyStageIngress,
                 _bcmFieldSliceSelDisable, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp4,
                 _bcmFieldSliceSelDisable, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6,
                 _bcmFieldSliceSelDisable, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelDisable, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPorts,
                 _bcmFieldSliceSelDisable, 0, 0, 0);

    /* IFP single wide Fixed */
    /* N.B.  Fixed field is in FP_GLOBAL_MASK_TCAM */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 1, 3);
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifySvpValid,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 4, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 5, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 9, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 10, 1);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTunnelType,
                               _bcmFieldSliceSelDisable, 0,
                               _bcmFieldSliceLoopbackTypeSelect, 1,
                               fp_global_mask_tcam_ofs + fixed_ofs + 11, 4);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyLoopbackType,
                               _bcmFieldSliceSelDisable, 0,
                               _bcmFieldSliceLoopbackTypeSelect, 0,
                               fp_global_mask_tcam_ofs + fixed_ofs + 11, 4);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyLoopback,
                               _bcmFieldSliceSelDisable, 0,
                               _bcmFieldSliceLoopbackTypeSelect, 0,
                               fp_global_mask_tcam_ofs + fixed_ofs + 14, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 15, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyRecoverableDrop,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMyStationHit,
                 _bcmFieldSliceSelDisable, 0,
                 fp_global_mask_tcam_ofs + fixed_ofs + 18, 1);

    /* FPF3 primary slice single wide.
     * IFP single wide F3_0  (f3_ofs = 26 - 2)*/
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 0, f3_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 1, f3_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 2, f3_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 2, f3_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 0, f3_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 1, f3_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 3, f3_ofs+ 6 , 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 3, f3_ofs , 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVrf, f3_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVpn, f3_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, 
                               bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVlan, f3_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f3_ofs + 24, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMimGport, f3_ofs + 24, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL3Ingress,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f3_ofs + 24, 13);
    /* IFP single wide F3_1  (f3_ofs = 26 - 2) */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 0, f3_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 1, f3_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 2, f3_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 3, f3_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 0, f3_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 1, f3_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 2, f3_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 3, f3_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVrf, f3_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVpn, f3_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVlan, f3_ofs + 12, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 1, f3_ofs + 24, 12);

    /* IFP single wide F3_2  (f3_ofs = 26 - 2) */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityGlp, f3_ofs + 19, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityGlp, f3_ofs + 19, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf3, 2,
                                _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f3_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityMimGport, f3_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcModPortGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityModPortGport, f3_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcTrunkMemberGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityModPortGport, f3_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityModPortGport, f3_ofs + 26, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityCommonGport, f3_ofs + 19, 17);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcVirtualPortValid,
                 _bcmFieldSliceSelFpf3, 2, f3_ofs + 35, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf3, 2, f3_ofs + 36, 3);

    /* IFP single wide F3_3  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 16, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 18, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTranslatedVlanFormat,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 20, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 22, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyPacketRes,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 25, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpInfo,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 31, 3);

    /* IFP single wide F3_4  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 16, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 28, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 29, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 32, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf3, 4, f3_ofs + 34, 4);

    /* IFP single wide F3_5  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, 5, f3_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 5, f3_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, 5, f3_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, 5, f3_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf3, 5, f3_ofs + 16, 16);

    /* IFP single wide F3_6  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsTerminated,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanTranslationHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 1, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanValid,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 3, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIngressStpState,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 4, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 6, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcStatic,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 7, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2DestHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 8, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2CacheHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 9, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3SrcHostHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpmcStarGroupHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestHostHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 11, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestRouteHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2StationMove,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 13, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDosAttack,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 14, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpTunnelHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 15, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel1Hit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillEgressRbridgeHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreSrcIpHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimSrcGportHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel2Hit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillIngressRbridgeHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreVfiHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimVfiHit,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 18, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 18, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 18 + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 6, f3_ofs + 18 + 13, 3);

    /* IFP single wide F3_7  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyRangeCheck,
                 _bcmFieldSliceSelFpf3, 7, f3_ofs, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, 7,
                               _bcmFieldSliceIntfClassSelect, 0,
                               f3_ofs + 24, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, 7,
                               _bcmFieldSliceIntfClassSelect, 1,
                               f3_ofs + 24, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, 7,
                               _bcmFieldSliceIntfClassSelect, 2,
                               f3_ofs + 24, 8);

    /* IFP single wide F3_8  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6FlowLabel,
                 _bcmFieldSliceSelFpf3, 8, f3_ofs, 20);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, 8, f3_ofs + 20, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf3, 8, f3_ofs + 22, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTranslatedVlanFormat,
                 _bcmFieldSliceSelFpf3, 8, f3_ofs + 24, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerTpid,
                 _bcmFieldSliceSelFpf3, 8, f3_ofs + 26, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterTpid,
                 _bcmFieldSliceSelFpf3, 8, f3_ofs + 28, 2);

    /* IFP single wide F3_9  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData3,
                 _bcmFieldSliceSelFpf3, 9, f3_ofs, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsOuterLabelPop,
                 _bcmFieldSliceSelFpf3, 9, f3_ofs + 32, 1);
    _FP_QUAL_ADD(unit, stage_fc,
                 bcmFieldQualifyMplsStationHitTunnelUnterminated,
                 _bcmFieldSliceSelFpf3, 9, f3_ofs + 32, 1);

    /* IFP single wide F3_10  (f3_ofs = 26 - 2) */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup, f3_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath, f3_ofs, 19);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                 _bcmFieldSliceSelFpf3, 10, f3_ofs + 19, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                 _bcmFieldSliceSelFpf3, 10, f3_ofs + 19, 16);

    /* IFP single wide F3_11  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                 _bcmFieldSliceSelFpf3, 11, f3_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                 _bcmFieldSliceSelFpf3, 11, f3_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                 _bcmFieldSliceSelFpf3, 11, f3_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                 _bcmFieldSliceSelFpf3, 11, f3_ofs + 7, 9);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                 _bcmFieldSliceSelFpf3, 11, f3_ofs + 16, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                 _bcmFieldSliceSelFpf3, 11, f3_ofs + 16, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcVirtualPortValid,
                 _bcmFieldSliceSelFpf3, 11, f3_ofs + 29, 1);

    /* IFP single wide F3_12  (f3_ofs = 26 - 2) */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVnTag,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagVn, f3_ofs, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyCnTag,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagCn, f3_ofs, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyMplsForwardingLabel,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagMplsFwdingLabel, f3_ofs, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyMplsControlWord,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagMplsCntlWord, f3_ofs, 33);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelTtl,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select,
                     _bcmFieldAuxTagMplsFwdingLabel, 0,
                     f3_ofs, 8, f3_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelBos,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select,
                     _bcmFieldAuxTagMplsFwdingLabel, 0,
                     f3_ofs + 8, 1, f3_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelExp,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select,
                     _bcmFieldAuxTagMplsFwdingLabel, 0,
                     f3_ofs + 9, 3, f3_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyMplsForwardingLabelId,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagMplsFwdingLabel, f3_ofs + 12, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashUpper,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagRtag7A, f3_ofs + 16, 17);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashLower,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select,
                     _bcmFieldAuxTagRtag7A, 0,
                     f3_ofs, 16, f3_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashUpper,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagRtag7B, f3_ofs + 16, 17);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashLower,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select,
                     _bcmFieldAuxTagRtag7B, 0,
                     f3_ofs, 16, f3_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyLlidValue,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select,
                     _bcmFieldAuxTagLogicalLinkId, 0,
                     f3_ofs, 16, f3_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsControlWordValid,
                 _bcmFieldSliceSelFpf3, 12, f3_ofs + 33, 1);

    /* IFP single wide F3_13  (f3_ofs = 26 - 2) */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf3, 13, f3_ofs, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeader2Type,
                 _bcmFieldSliceSelFpf3, 13, f3_ofs + 8, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderSubCode,
                 _bcmFieldSliceSelFpf3, 13, f3_ofs + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                 _bcmFieldSliceSelFpf3, 13, f3_ofs + 24, 8);

    /* IFP single wide F2_0  (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 8, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpFlags,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 14, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 24, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIcmpTypeCode,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 56, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 64, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 96, 32);

    /* IFP single wide F2_1 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 8, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 14, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 24, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIcmpTypeCode,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 56, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 64, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                 _bcmFieldSliceSelFpf2, 1, f2_ofs + 96, 32);

    /* IFP single wide F2_2 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                 _bcmFieldSliceSelFpf2, 2, f2_ofs, 128);

    /* IFP single wide F2_3 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6,
                 _bcmFieldSliceSelFpf2, 3, f2_ofs, 128);

    /* IFP single wide F2_4 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 4, f2_ofs, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 4, f2_ofs + 8, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6FlowLabel,
                 _bcmFieldSliceSelFpf2, 4, f2_ofs + 14, 20);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf2, 4, f2_ofs + 34, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 4, f2_ofs + 42, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6High,
                 _bcmFieldSliceSelFpf2, 4, f2_ofs + 64, 64);

    /* IFP single wide F2_5 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs + 80, 48);

    /* IFP single wide F2_6 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf2, 6, f2_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf2, 6, f2_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf2, 6, f2_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf2, 6, f2_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 6, f2_ofs + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 6, f2_ofs + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                 _bcmFieldSliceSelFpf2, 6, f2_ofs + 80, 32);

    /* IFP single wide F2_7 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 32, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 40, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 48, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 80, 48);

    /* IFP single wide F2_8 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData0,
                 _bcmFieldSliceSelFpf2, 8, f2_ofs, 128);

    /* IFP single wide F2_9 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData1,
                     _bcmFieldSliceSelFpf2, 9, f2_ofs, 128);

    /* IFP single wide F2_10 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6High,
                     _bcmFieldSliceSelFpf2, 10, f2_ofs, 64);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6High,
                     _bcmFieldSliceSelFpf2, 10, f2_ofs + 64, 64);

    /* IFP single wide F2_11 (f2_ofs = 66 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 11, f2_ofs + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 11, f2_ofs + 80, 48);
    /* DIP_127_96 omitted -- no qualifier?  Omitted in Trident, too.*/

    /* IFP single wide F1_0 (f1_ofs = 194 - 2)*/
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 0, f1_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 1, f1_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 2, f1_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 3, f1_ofs, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 0, f1_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 1, f1_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 2, f1_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 3, f1_ofs + 6, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVrf, f1_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVpn, f1_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVlan, f1_ofs + 12, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f1_ofs + 24, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMimGport, f1_ofs + 24, 13);

    /* IFP single wide F1_1  (f1_ofs = 194 - 2)*/
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs + 19, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs + 19, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f1_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityMimGport, f1_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcModPortGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityModPortGport, f1_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcTrunkMemberGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityModPortGport, f1_ofs + 19, 17);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityModPortGport, f1_ofs + 26, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceSrcEntitySelect,
                               _bcmFieldFwdEntityCommonGport, f1_ofs + 19, 17);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcVirtualPortValid,
                 _bcmFieldSliceSelFpf1, 1, f1_ofs + 35, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf1, 1, f1_ofs + 36, 3);

    /* IFP single wide F1_2  (f1_ofs = 194 - 2)*/
    
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath, f1_ofs, 19);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf1, 2, f1_ofs + 19, 3);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 0, f1_ofs + 22, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 1, f1_ofs + 22, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 2, f1_ofs + 22, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 3, f1_ofs + 22, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 0, f1_ofs + 28, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 1, f1_ofs + 28, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 2, f1_ofs + 28, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 3, f1_ofs + 28, 6);

    /* IFP single wide F1_3  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs + 16, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs + 28, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs + 29, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterTpid,
                 _bcmFieldSliceSelFpf1, 3, f1_ofs + 32, 2);

    /* IFP single wide F1_4  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 4, f1_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 4, f1_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 4, f1_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 4, f1_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf1, 4, f1_ofs + 16, 16);

    /* IFP single wide F1_5  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsTerminated,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanTranslationHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 1, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanValid,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 3, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIngressStpState,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 4, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 6, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcStatic,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 7, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2DestHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 8, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2CacheHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 9, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3SrcHostHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpmcStarGroupHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestHostHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 11, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestRouteHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2StationMove,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 13, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDosAttack,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 14, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpTunnelHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 15, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel1Hit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillEgressRbridgeHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreSrcIpHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimSrcGportHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel2Hit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillIngressRbridgeHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreVfiHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimVfiHit,
                 _bcmFieldSliceSelFpf3, 5, f1_ofs + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf1, 5, f1_ofs + 18, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf1, 5, f1_ofs + 18, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf1, 5, f1_ofs + 30, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf1, 5, f1_ofs + 31, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerTpid,
                 _bcmFieldSliceSelFpf1, 5, f1_ofs + 34, 2);

    /* IFP single wide F1_6  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 16, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 18, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTranslatedVlanFormat,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 20, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 22, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyPacketRes,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 25, 5);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpInfo,
                 _bcmFieldSliceSelFpf1, 6, f1_ofs + 31, 3);

    /* IFP single wide F1_7  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs + 13, 3);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 0, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 1, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 2, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 3, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVrf, f1_ofs + 22, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVpn, f1_ofs + 22, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, 
                               bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVlan, f1_ofs + 22, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsOuterLabelPop,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs + 34 , 1);
    _FP_QUAL_ADD(unit, stage_fc, 
                 bcmFieldQualifyMplsStationHitTunnelUnterminated,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs + 34, 1);
    _FP_QUAL_ADD(unit, stage_fc,
                 bcmFieldQualifyGenericAssociatedChannelLabelValid,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs + 36, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyRouterAlertLabelValid,
                 _bcmFieldSliceSelFpf1, 7, f1_ofs + 36, 1);

    /* IFP single wide F1_8  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf1, 8, f1_ofs, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf1, 8, f1_ofs + 8, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 0, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 1, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 2, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 3, f1_ofs + 16, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVrf, f1_ofs + 22, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVpn, f1_ofs + 22, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, 
                               bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceFwdFieldSelect, 
                               _bcmFieldFwdFieldVlan, f1_ofs + 22, 12);

    /* IFP single wide F1_9  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData2,
                 _bcmFieldSliceSelFpf1, 9, f1_ofs, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsOuterLabelPop,
                 _bcmFieldSliceSelFpf1, 9, f1_ofs + 32 , 1);
    _FP_QUAL_ADD(unit, stage_fc, 
                 bcmFieldQualifyMplsStationHitTunnelUnterminated,
                 _bcmFieldSliceSelFpf1, 9, f1_ofs + 32, 1);
    _FP_QUAL_ADD(unit, stage_fc,
                 bcmFieldQualifyGenericAssociatedChannelLabelValid,
                 _bcmFieldSliceSelFpf1, 9, f1_ofs + 36, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyRouterAlertLabelValid,
                 _bcmFieldSliceSelFpf1, 9, f1_ofs + 36, 1);

    /* IFP single wide F1_10  (f1_ofs = 194 - 2)*/
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup, f1_ofs, 19);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath, f1_ofs, 19);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                 _bcmFieldSliceSelFpf1, 10, f1_ofs + 19, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                 _bcmFieldSliceSelFpf1, 10, f1_ofs + 19, 16);

    /* IFP single wide F1_11  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs + 7, 9);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs + 16, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs + 16, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcVirtualPortValid,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs + 29, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs + 30, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf1, 11, f1_ofs + 32, 4);

    /* IFP single wide F1_12  (f1_ofs = 194 - 2)*/
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 12, f1_ofs, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 0, f1_ofs + 12, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 1, f1_ofs + 12, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 2, f1_ofs + 12, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 3, f1_ofs + 12, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 0, f1_ofs + 18, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 1, f1_ofs + 18, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 2, f1_ofs + 18, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 3, f1_ofs + 18, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f1_ofs + 24, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport, f1_ofs + 24, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelAction,
                 _bcmFieldSliceSelFpf1, 12, f1_ofs + 37, 3);

    /* IFP single wide F1_13  (f1_ofs = 194 - 2)*/
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVnTag,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select, _bcmFieldAuxTagVn,
                               f1_ofs, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyCnTag,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select, _bcmFieldAuxTagCn,
                               f1_ofs, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyMplsForwardingLabel,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagMplsFwdingLabel, f1_ofs, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyMplsControlWord,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagMplsCntlWord, f1_ofs, 33);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelTtl,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select,
                     _bcmFieldAuxTagMplsFwdingLabel, 0,
                     f1_ofs, 8, f1_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelBos,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select,
                     _bcmFieldAuxTagMplsFwdingLabel, 0,
                     f1_ofs + 8, 1, f1_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelExp,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select,
                     _bcmFieldAuxTagMplsFwdingLabel, 0,
                     f1_ofs + 9, 3, f1_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyMplsForwardingLabelId,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagMplsFwdingLabel, f1_ofs + 12, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashUpper,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagRtag7A, f1_ofs + 16, 17);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashLower,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select,
                     _bcmFieldAuxTagRtag7A, 0,
                     f1_ofs, 16, f1_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashUpper,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagRtag7B, f1_ofs + 16, 17);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashLower,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select,
                     _bcmFieldAuxTagRtag7B, 0,
                     f1_ofs, 16, f1_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyLlidValue,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select,
                     _bcmFieldAuxTagLogicalLinkId, 0,
                     f1_ofs, 16, f1_ofs + 32, 1, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsControlWordValid,
                 _bcmFieldSliceSelFpf1, 13, f1_ofs + 33, 1);
   _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsForwardingLabelAction,
                 _bcmFieldSliceSelFpf1, 13, f1_ofs + 34, 3);

    /* IFP DWF4_1 (dw_f4_ofs = 3 - 3 + 1)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                            _bcmFieldSliceSelFpf4, 1,
                            _bcmFieldSliceSelDisable, 0, dw_f4_ofs, 6);

    /* IFP DWF3 (dw_f3_ofs = 11 - 3 + 1,)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTos,
                            _bcmFieldSliceSelDisable, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f3_ofs, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                            _bcmFieldSliceSelDisable, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f3_ofs + 8, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc,
                            bcmFieldQualifyExtensionHeaderSubCode,
                            _bcmFieldSliceSelDisable, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f3_ofs + 16, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                            _bcmFieldSliceSelDisable, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f3_ofs + 24, 8);

    /* IFP DWF2_0 (dw_f2_ofs = = 43 - 3 + 1)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyMplsTerminated,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyVlanTranslationHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 + 1, 2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanValid,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 + 3, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIngressStpState,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 + 4, 2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL2SrcHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 + 6, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL2SrcStatic,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 + 7, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL2DestHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 + 8, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL2CacheHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 + 9, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL3SrcHostHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +10, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpmcStarGroupHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +10, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL3DestHostHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +10, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL3DestRouteHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +12, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL2StationMove,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +13, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDosAttack,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +14, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpTunnelHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +15, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel1Hit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +16, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc,
                            bcmFieldQualifyTrillEgressRbridgeHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +16, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL2GreSrcIpHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +16, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyMimSrcGportHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +16, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel2Hit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +17, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc,
                            bcmFieldQualifyTrillIngressRbridgeHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +17, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL2GreVfiHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +17, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyMimVfiHit,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 14 +17, 1);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyRangeCheck,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs + 32, 32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIntfClassSelect, 0,
                            dw_f2_ofs + 56, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIntfClassSelect, 1,
                            dw_f2_ofs + 56, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIntfClassSelect, 2,
                            dw_f2_ofs + 56, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0,
                            dw_f2_ofs + 64, 32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceSelDisable, 0,
                            dw_f2_ofs + 96, 32);

    /* IFP DWF2_1 (dw_f2_ofs = = 43 - 3 + 1)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                            _bcmFieldSliceSelFpf2, 1,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs, 128);

    /* IFP DWF2_2 (dw_f2_ofs = = 43 - 3 + 1)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstIp6,
                            _bcmFieldSliceSelFpf2, 2,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs, 128);

    /* IFP DWF2_3 (dw_f2_ofs = = 43 - 3 + 1)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, _bcmFieldQualifyData1,
                            _bcmFieldSliceSelFpf2, 3,
                            _bcmFieldSliceSelDisable, 0, dw_f2_ofs, 128);
    /* IFP DWF1_0  (dw_f1_ofs = 171 - 3 + 1)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs + 8, 2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs + 10, 16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs + 26, 16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIcmpTypeCode,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs + 26, 16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityGlp, dw_f1_ofs + 42, 19);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityGlp, dw_f1_ofs + 42, 19);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityMplsGport, dw_f1_ofs + 42, 19);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityMimGport, dw_f1_ofs + 42, 19);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityL3Egress, dw_f1_ofs + 42, 19);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstL3EgressNextHops,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityL3Egress, dw_f1_ofs + 42, 19);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityMulticastGroup,
                            dw_f1_ofs + 42, 19);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceDstFwdEntitySelect,
                            _bcmFieldFwdEntityMultipath, dw_f1_ofs + 42, 19);

    /* IFP DWF1_1  (dw_f1_ofs = 171 - 3 + 1)*/
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc,
                            bcmFieldQualifyMplsForwardingLabelId,
                            _bcmFieldSliceSelFpf1, 1,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs, 20);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc,
                            bcmFieldQualifyMplsForwardingLabelAction,
                            _bcmFieldSliceSelFpf1, 1,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs + 20, 3);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyMplsControlWord,
                            _bcmFieldSliceSelFpf1, 1,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs + 23, 32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyMplsControlWordValid,
                            _bcmFieldSliceSelFpf1, 1,
                            _bcmFieldSliceSelDisable, 0, dw_f1_ofs + 55, 1);


    /* IPBM OVERLAY qualifiers for Katana2 */
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     1,
                     0, 16,
                     0, 0,
                     0, 0,
                     1);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     1,
                     0, 16,
                     0, 0,
                     0, 0,
                     1);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityGlp,
                     1,
                     16, 16,
                     0, 0,
                     0, 0,
                     1);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityGlp,
                     1,
                     16, 16,
                     0, 0,
                     0, 0,
                     1);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityModPortGport,
                     1,
                     16, 16,
                     0, 0,
                     0, 0,
                     1);
     _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityModPortGport,
                     1,
                     23, 8,
                     0, 0,
                     0, 0,
                     1);


    /* DONE DWF */
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_kt2_lookup_qualifiers_init
 * Purpose:
 *     Initialize device stage lookup qaualifiers 
 *     select codes & offsets
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     stage_fc   - (IN) Field Processor stage control structure. 
 *
 * Returns:
 *     BCM_E_NONE
 */
STATIC int
_field_kt2_lookup_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    const unsigned f1_ofs = 160,
                   f2_ofs = 32,
                   f3_ofs = 0;
    _FP_QUAL_DECL;

    /* Input parameters check. */
    if (NULL == stage_fc) {
        return (BCM_E_PARAM);
    }

    /* Enable the overlay of Sender Ethernet Address onto MACSA
     * on ARP/RARP packets.
     */
    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, ING_CONFIG_64r, REG_PORT_ANY,
                                ARP_VALIDATION_ENf, 1));

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyStageLookup,
                 _bcmFieldSliceSelDisable, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp4,
                 _bcmFieldSliceSelDisable, 0, 0, 0);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6,
                 _bcmFieldSliceSelDisable, 0, 0, 0);

    /* VFP_F3_0 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 0, f3_ofs, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 0, f3_ofs, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 0, f3_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 0, f3_ofs + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf3, 0, f3_ofs + 16, 16);

    /* VFP_F3_1 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 0, f3_ofs, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4DstPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 1, f3_ofs, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 0,f3_ofs + 16, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4SrcPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 1,f3_ofs + 16, 16);

    /* VFP_F3_2 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 1, f3_ofs, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 0, f3_ofs, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 1, f3_ofs + 8, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 0, f3_ofs + 8, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderSubCode,
                 _bcmFieldSliceSelFpf3, 2, f3_ofs + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                 _bcmFieldSliceSelFpf3, 2, f3_ofs + 24, 8);

    /* VFP_F3_3 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 8, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 8, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 20, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 3, f3_ofs + 21, 3);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcModuleGport,
                               _bcmFieldSliceSelFpf3, 3,
                               _bcmFieldSliceSrcTypeSelect, 0, f3_ofs + 24, 8);

    /* VFP_F2_0 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerTtl,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 0, f2_ofs + 8, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpFrag,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 14, 2);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 14, 2);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 16, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 16, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4DstPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 24, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 24, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4SrcPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 40, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 40, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 56, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 56, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerDstIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 64, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 64, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerSrcIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 96, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 96, 32);
    /* VFP_F2_1 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerSrcIp6,
                               _bcmFieldSliceSelFpf2, 1,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 128);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                               _bcmFieldSliceSelFpf2, 1,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 128);
    /* VFP_F2_2 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerDstIp6,
                               _bcmFieldSliceSelFpf2, 2,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 128);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6,
                               _bcmFieldSliceSelFpf2, 2,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 128);
    /* VFP_F2_3 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 3, f2_ofs + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 3, f2_ofs + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 3, f2_ofs + 80, 48);
    /* VFP_F2_4 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 8, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 8, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerDstIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 16, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 16, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerSrcIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 48, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 48, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 4, 112, 48);

    /* VFP_F2_5 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 8, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 8, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerDstIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 16, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 16, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerSrcIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 48, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 48, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 5, f2_ofs + 80, 48);

    /* VFP_F2_6 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerSrcIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 64);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 64);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerDstIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 1,f2_ofs + 64, 64);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 0,f2_ofs + 64, 64);

    /* VFP_F2_7 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVnTag,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs, 33);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySnap,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 64, 40);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyLlc,
                 _bcmFieldSliceSelFpf2, 7, f2_ofs + 104, 24);

    /* VFP_F2_8 */
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData0,
                 _bcmFieldSliceSelFpf2, 8, f2_ofs, 128);

    /* VFP_F2_9 */
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData1,
                 _bcmFieldSliceSelFpf2, 9, f2_ofs, 128);

    /* VFP_F1_0 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpProtocolCommon,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f1_ofs, 3);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocolCommon,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f1_ofs, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTunnelTerminated,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 3, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerTpid,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 5, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterTpid,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 7, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 9, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGigProxy,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 11, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 13, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 13, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 25, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 26, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 29, 2);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityGlp,
                 0,
                 f1_ofs + 31, 7, /* Port value in SGLP */
                 f1_ofs + 38, 8, /* Module value in SGLP */
                 f1_ofs + 46, 1, /* Trunk bit in SGLP (should be 0) */
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityGlp,
                 0,
                 f1_ofs + 31, 15, /* trunk id field of SGLP */
                 f1_ofs + 46, 1,  /* trunk bit of SGLP */
                 0, 0,
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityMplsGport,
                 0,
                 f1_ofs + 31, 16, /* S_FIELD */
                 f1_ofs + 56, 1,  /* SVP_VALID */
                 0, 0,
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityMimGport,
                 0,
                 f1_ofs + 31, 16, /* S_FIELD */
                 f1_ofs + 56, 1,  /* SVP_VALID */
                 0, 0,
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcWlanGport,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityWlanGport,
                 0,
                 f1_ofs + 31, 16, /* S_FIELD */
                 f1_ofs + 56, 1,  /* SVP_VALID */
                 0, 0,
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityModPortGport,
                 0,
                 f1_ofs + 31, 15, /* mod + port field of unresolved SGLP */
                 0, 0,
                 0, 0,
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityModPortGport,
                 0,
                 f1_ofs + 31 + 7, 8, /* mod field of unresolved SGLP */
                 0, 0,
                 0, 0,
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityPortGroupNum,
                 0,
                 f1_ofs + 31, 7, /* ingress port field */
                 0, 0,
                 0, 0,
                 0
                 );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldDevSelDisable, 0,
                 _bcmFieldSliceSelFpf1, 0,
                 _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityPortGroupNum,
                 0,
                 f1_ofs + 31 + 7, 8, /* ingress port group field */
                 0, 0,
                 0, 0,
                 0
                 );

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpType,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f1_ofs + 48, 4);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f1_ofs + 48, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports, 
                 _bcmFieldSliceSelFpf1, 0, f1_ofs + 52, 1);


    /* VFP_DOUBLE_WIDE_F3 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_ofs, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTos,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_ofs, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTtl,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_ofs + 8, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_ofs + 8, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_ofs + 16, 6);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpFrag,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_ofs + 22, 2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_ofs + 22, 2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_ofs + 24, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_ofs + 24, 8);

    /* VFP_DOUBLE_WIDE_F2_0 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTtl,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 8, 6);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpFrag,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 14, 2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 14, 2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 16, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTos,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 16, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerL4DstPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 24, 16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 24, 16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerL4SrcPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 40, 16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 40, 16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 56, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 56, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerDstIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 64, 32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 64, 32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs + 96, 32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs + 96, 32);

    /* VFP_DOUBLE_WIDE_F2_1 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp6,
                            _bcmFieldSliceSelFpf2, 1,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_ofs, 128);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                            _bcmFieldSliceSelFpf2, 1,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 128);

    /* VFP_DOUBLE_WIDE_F2_2 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, _bcmFieldQualifyData2,
                            _bcmFieldSliceSelFpf2, 2,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_ofs, 128);

    /* VFP_DOUBLE_WIDE_F1 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc,
                            bcmFieldQualifyExtensionHeaderSubCode,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_ofs, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_ofs + 8, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_ofs + 16, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_ofs + 24, 16);
    return (BCM_E_NONE);
}


/*
 * Function:
 *     _field_kt2_egress_qualifiers_init
 * Purpose:
 *     Initialize device stage egress qaualifiers 
 *     select codes & offsets
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     stage_fc   - (IN) Field Processor stage control structure. 
 *
 * Returns:
 *     BCM_E_NONE
 */
STATIC int
_field_kt2_egress_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    _FP_QUAL_DECL;
    _key_fld_ = KEYf;

    /* Input parameters check. */
    if (NULL == stage_fc) {
        return (BCM_E_PARAM);
    }

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyStageEgress,
                 _bcmFieldSliceSelDisable, 0, 0, 0);

    /*EFP_KEY1 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 0, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 1, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 5, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 7, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 9, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 15, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 31, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 47, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 55, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 63, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 95, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 127, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGigProxy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 135, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 136, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 137, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 149, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 157, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 158, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 159, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 159, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 171, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 172, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 175, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 177, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 178, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 186, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 194, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 198, 1);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 1, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 2, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassTrill,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 3, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassWlan,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 3, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassL2Gre,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 3, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 212, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 212, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 212, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 226, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp4,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 0, 0);

    /*EFP_KEY2 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 0, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 1, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceIp6AddrSelect,
                               _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_SIP6,
                               9, 128);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceIp6AddrSelect,
                               _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_DIP6,
                               9, 128);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6High,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceIp6AddrSelect,
                               _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_SIP_DIP_64,
                               9, 64);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6High,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceIp6AddrSelect,
                               _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_SIP_DIP_64,
                               73, 64);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 137, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGigProxy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 145, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 146, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 147, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 159, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 167, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 168, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 169, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 169, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 181, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 182, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 185, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 187, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 188, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 1, 196, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 2, 196, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 3, 196, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0, 196, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 209, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 213, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 214, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 214, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 214, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 228, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 0, 0);

    /*EFP_KEY3 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 0, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 1, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 5, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 7, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 9, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 15, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 31, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 47, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 55, 128);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderSubCode,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 183, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 191, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 199, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 203, 1);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 0, 204, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 1, 204, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassTrill,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 2, 204, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassWlan,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 2, 204, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassL2Gre,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 2, 204, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 0, 0);

    /*EFP_KEY4 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 0, 1);
    
    /* EFP_KEY4: DVP_VALID 1-bit @ bit 1 */
    /* EFP_KEY4: DVP 12-bit @ bit 2 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 18, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 22, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 24, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 26, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 42, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 90, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanTranslationHit,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 138, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGigProxy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 139, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 140, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 141, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 142, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 145, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 157, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 165, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 166, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 167, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 167, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 179, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 180, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 183, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 185, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 186, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 1, 194, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0, 194, 13);
    
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 208, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 212, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 213, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 213, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 213, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 227, 2);

    /* KEY1 without v4 specific fields. */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 0, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 1, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 5, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 9, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 15, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 31, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 47, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 55, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 127, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGigProxy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 135, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 136, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 137, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 149, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 157, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 158, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 159, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 159, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 171, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 172, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 175, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 177, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 178, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 186, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 194, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 198, 1);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 1, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 2, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassTrill,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 3, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassWlan,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 3, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassL2Gre,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 3, 199, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0, 199, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 212, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 212, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 212, 14);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 226, 2);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_kt2_qualifiers_init
 * Purpose:
 *     Initialize device qaualifiers select codes & offsets
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     stage_fc   - (IN) Field Processor stage control structure. 
 *
 * Returns:
 *     BCM_E_NONE
 * Notes:
 */
STATIC int
_field_kt2_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    int rv;           /* Operation return status. */

    /* Input parameters check. */
    if (NULL == stage_fc) {
        return (BCM_E_PARAM);
    }

    /* Allocated stage qualifiers configuration array. */
    _FP_XGS3_ALLOC(stage_fc->f_qual_arr, 
                   (_bcmFieldQualifyCount * sizeof(_bcm_field_qual_info_t *)),
                   "Field qualifiers");
    if (NULL == stage_fc->f_qual_arr) {
        return (BCM_E_MEMORY);
    }
    
    switch (stage_fc->stage_id) {
      case _BCM_FIELD_STAGE_INGRESS:
          rv = _field_kt2_ingress_qualifiers_init(unit, stage_fc);
          break;
      case _BCM_FIELD_STAGE_LOOKUP:
          rv = _field_kt2_lookup_qualifiers_init(unit, stage_fc);
          break;
      case _BCM_FIELD_STAGE_EGRESS:
          rv = _field_kt2_egress_qualifiers_init(unit, stage_fc);
          break;
      case _BCM_FIELD_STAGE_EXTERNAL:  
      default: 
          sal_free(stage_fc->f_qual_arr);
          return (BCM_E_PARAM);
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_field_kt2_init

 * Purpose:
 *     Perform initializations that are specific to BCM56230. This
 *     includes initializing the FP field select bit offset tables for FPF[1-3]
 *     for every stage. 
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     fc         - (IN) Field Processor control structure. 
 *
 * Returns:
 *     BCM_E_NONE
 * Notes:
 */
int
_bcm_field_kt2_init(int unit, _field_control_t *fc)
{
    _field_stage_t *stage_fc; /* Stages iteration pointer */

    /* Input parameters check. */
    if (NULL == fc) {
        return (BCM_E_PARAM);
    }

    stage_fc = fc->stages;
    while (stage_fc) {
        /* Clear hardware table */
        BCM_IF_ERROR_RETURN(_bcm_field_tr_hw_clear(unit, stage_fc));

        /* Initialize qualifiers info. */
        BCM_IF_ERROR_RETURN(_field_kt2_qualifiers_init(unit, stage_fc));

        if (_BCM_FIELD_STAGE_EXTERNAL == stage_fc->stage_id) {
            _bcm_field_tr_external_init(unit, stage_fc);
            stage_fc = stage_fc->next;
            continue;
        }

        /* Goto next stage */
        stage_fc = stage_fc->next;
    }

    /* Initialize the TOS_FN, TTL_FN, TCP_FN tables */
    BCM_IF_ERROR_RETURN(_bcm_field_trx_tcp_ttl_tos_init(unit));
    
    if (0 == SOC_WARM_BOOT(unit)) {
        /* Enable filter processor */
        BCM_IF_ERROR_RETURN(_field_port_filter_enable_set(unit, fc, TRUE));

        /* Enable meter refresh */
        BCM_IF_ERROR_RETURN(_field_meter_refresh_enable_set(unit, fc, TRUE));
    }

    /* Initialize the function pointers */
    _field_kt2_functions_init(&fc->functions);;

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_kt2_lookup_selcodes_install
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
_field_kt2_lookup_selcodes_install(int            unit,
                                   _field_group_t *fg,
                                   uint8          slice_num,
                                   int            selcode_idx)
{
    static const soc_field_t s_type_fld_tbl[] = {
        SLICE_0_SOURCE_TYPE_SELf,
        SLICE_1_SOURCE_TYPE_SELf,
        SLICE_2_SOURCE_TYPE_SELf,
        SLICE_3_SOURCE_TYPE_SELf
    };

    _field_sel_t * const sel = &fg->sel_codes[selcode_idx];
    int           errcode = BCM_E_NONE;
    uint64        regval;
    uint64        val;

    BCM_IF_ERROR_RETURN(soc_reg64_get(unit, VFP_KEY_CONTROL_1r,
                                      REG_PORT_ANY, 0, &regval));

    if ((fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE)
        && (selcode_idx & 1)) {
        if (sel->fpf2 != _FP_SELCODE_DONT_CARE) {
            COMPILER_64_SET(val, 0, sel->fpf2);
            soc_reg64_field_set(unit, VFP_KEY_CONTROL_1r, &regval,
                            _bcm_field_trx_vfp_double_wide_sel[slice_num], val);
        }
    } else {
        if (sel->fpf2 != _FP_SELCODE_DONT_CARE) {
            COMPILER_64_SET(val, 0, sel->fpf2);
            soc_reg64_field_set(unit, VFP_KEY_CONTROL_1r, &regval,
                            _bcm_field_trx_vfp_field_sel[slice_num][0], val);
        }
        if (sel->fpf3 != _FP_SELCODE_DONT_CARE) {
            COMPILER_64_SET(val, 0, sel->fpf3);
            soc_reg64_field_set(unit, VFP_KEY_CONTROL_1r, &regval,
                            _bcm_field_trx_vfp_field_sel[slice_num][1], val);
        }
    }

    if (sel->src_entity_sel != _FP_SELCODE_DONT_CARE) {
        uint32 value;

        switch (sel->src_entity_sel) {
            case _bcmFieldFwdEntityPortGroupNum:
                value = 4;
                break;
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
            default:
                return (BCM_E_INTERNAL);
        }
        COMPILER_64_SET(val, 0, value);
        soc_reg64_field_set(unit, VFP_KEY_CONTROL_1r, &regval,
                            s_type_fld_tbl[slice_num], val);
    }

    BCM_IF_ERROR_RETURN(
        soc_reg64_set(unit, VFP_KEY_CONTROL_1r, REG_PORT_ANY, 0, regval));

    /* Set inner/outer ip header selection. */
    if (sel->ip_header_sel != _FP_SELCODE_DONT_CARE) {
        errcode = soc_reg_field32_modify(
                      unit, VFP_KEY_CONTROL_2r, REG_PORT_ANY,
                      _bcm_field_trx_vfp_ip_header_sel[slice_num],
                      sel->ip_header_sel);
    }
    return (errcode);
}

STATIC int
_field_kt2_egress_selcodes_install(int            unit,
                                   _field_group_t *fg,
                                   uint8          slice_num,
                                   bcm_pbmp_t     *pbmp,
                                   int            selcode_idx)
{
    static const soc_field_t fldtbl[][4] = {
        { SLICE_0_F1f, SLICE_1_F1f, SLICE_2_F1f, SLICE_3_F1f },
        { SLICE_0_F2f, SLICE_1_F2f, SLICE_2_F2f, SLICE_3_F2f },
        { SLICE_0_F3f, SLICE_1_F3f, SLICE_2_F3f, SLICE_3_F3f },
        { SLICE_0_F4f, SLICE_1_F4f, SLICE_2_F4f, SLICE_3_F4f }
    };

    _field_sel_t * const sel = &fg->sel_codes[selcode_idx];

    if (sel->egr_class_f1_sel != _FP_SELCODE_DONT_CARE) {
        BCM_IF_ERROR_RETURN(
            soc_reg_field32_modify(unit, EFP_CLASSID_SELECTORr, REG_PORT_ANY,
                               fldtbl[0][slice_num], sel->egr_class_f1_sel));
    }
    if (sel->egr_class_f2_sel != _FP_SELCODE_DONT_CARE) {
        BCM_IF_ERROR_RETURN(
           soc_reg_field32_modify(unit, EFP_CLASSID_SELECTORr, REG_PORT_ANY,
                               fldtbl[1][slice_num], sel->egr_class_f2_sel));
    }
    if (sel->egr_class_f3_sel != _FP_SELCODE_DONT_CARE) {
        BCM_IF_ERROR_RETURN(
            soc_reg_field32_modify(unit, EFP_CLASSID_SELECTORr, REG_PORT_ANY,
                                fldtbl[2][slice_num], sel->egr_class_f3_sel));
    }
    if (sel->egr_class_f4_sel != _FP_SELCODE_DONT_CARE) {
        BCM_IF_ERROR_RETURN(
            soc_reg_field32_modify(unit, EFP_CLASSID_SELECTORr, REG_PORT_ANY,
                                fldtbl[3][slice_num], sel->egr_class_f4_sel));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_kt2_lookup_mode_set
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
STATIC int
_field_kt2_lookup_mode_set(int            unit,
                           uint8          slice_num,
                           _field_group_t *fg,
                           uint8          flags)
{
    static const soc_field_t wide_mode_flds[] = {
        SLICE_0_DOUBLE_WIDE_MODEf,
        SLICE_1_DOUBLE_WIDE_MODEf,
        SLICE_2_DOUBLE_WIDE_MODEf,
        SLICE_3_DOUBLE_WIDE_MODEf
    }, pairing_flds[] = {
        SLICE1_0_PAIRINGf,
        SLICE3_2_PAIRINGf
    };

    uint64 vfp_key_control_1_buf;

    BCM_IF_ERROR_RETURN(
        soc_reg64_get(unit, VFP_KEY_CONTROL_1r, REG_PORT_ANY, 0,
                      &vfp_key_control_1_buf));

    soc_reg64_field32_set(unit, VFP_KEY_CONTROL_1r,
                          &vfp_key_control_1_buf, pairing_flds[slice_num >> 1],
                          flags & _FP_GROUP_SPAN_DOUBLE_SLICE ? 1 : 0);
    soc_reg64_field32_set(unit, VFP_KEY_CONTROL_1r,
                          &vfp_key_control_1_buf, wide_mode_flds[slice_num],
                          flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE ? 1 : 0);

    return (soc_reg64_set(unit, VFP_KEY_CONTROL_1r, REG_PORT_ANY, 0,
                          vfp_key_control_1_buf));
}

/*
 * Function:
 *     _field_kt2_mode_set
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

STATIC int
_field_kt2_mode_set(int unit, uint8 slice_num,
                         _field_group_t *fg, uint8 flags)
{
    int (*func)(int unit, uint8 slice_num, _field_group_t *fg, uint8 flags);

    switch (fg->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            func = _field_kt2_lookup_mode_set;
            break;
        case _BCM_FIELD_STAGE_INGRESS:
            return (BCM_E_NONE);
        case _BCM_FIELD_STAGE_EGRESS:
            func = _bcm_field_trx_egress_mode_set;
            break;
        default:
            return (BCM_E_INTERNAL);
    }

    return ((*func)(unit, slice_num, fg, flags));
}

STATIC int
_field_kt2_egress_key_match_type_set (int unit, _field_entry_t *f_ent)
{
    static const _bcm_field_qual_offset_t q_offset = { KEYf, 231, 3, 0, 0, 0, 0 };

    const _field_group_t *fg = f_ent->group;

    uint32 data, mask;

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

    mask = 0x7;

    return (_bcm_field_qual_value_set(unit,
                                      (_bcm_field_qual_offset_t *) &q_offset,
                                      f_ent,
                                      &data,
                                      &mask
                                      )
            );
}

STATIC int
_field_kt2_selcodes_install(int            unit,
                            _field_group_t *fg,
                            uint8          slice_num,
                            bcm_pbmp_t     pbmp,
                            int            selcode_idx)
{
    int rv;

    /* Input parameters check. */
    if (NULL == fg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_field_kt2_mode_set(unit, slice_num, fg, fg->flags));

    switch (fg->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            rv = _field_kt2_lookup_selcodes_install(unit, fg, slice_num,
                                                    selcode_idx);
            break;
        case _BCM_FIELD_STAGE_INGRESS:
            rv =  _bcm_field_trx_ingress_selcodes_install(unit, fg, slice_num,
                                                          &pbmp, selcode_idx);
            break;
        case _BCM_FIELD_STAGE_EGRESS:
            rv = _field_kt2_egress_selcodes_install(unit, fg, slice_num,
                                                    &pbmp, selcode_idx);;
            break;
        default:
            return (BCM_E_INTERNAL);
    }
    return (rv);
}

STATIC int
_field_kt2_group_install(int unit, _field_group_t *fg)
{
    _field_slice_t *fs;        /* Slice pointer.           */
    uint8  slice_number;       /* Slices iterator.         */
    int    parts_count;        /* Number of entry parts.   */
    int    idx;                /* Iteration index.         */

    if (NULL == fg) {
        return (BCM_E_PARAM);
    }

    /* Get number of entry parts. */
    BCM_IF_ERROR_RETURN(
        _bcm_field_entry_tcam_parts_count(unit, fg->flags, &parts_count));

    for (idx = 0; idx < parts_count; ++idx) {
        BCM_IF_ERROR_RETURN(
           _bcm_field_tcam_part_to_slice_number(idx, fg->flags, &slice_number));
        fs = fg->slices + slice_number;

        BCM_IF_ERROR_RETURN(
            _field_kt2_selcodes_install(unit, fg, fs->slice_number,
                                        fg->pbmp, idx));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_kt2_lookup_slice_clear
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
_field_kt2_lookup_slice_clear(int unit, unsigned slice_num)
{
    uint64 reg_val;

    BCM_IF_ERROR_RETURN(
        soc_reg64_get(unit, VFP_KEY_CONTROL_1r, REG_PORT_ANY, 0, &reg_val));
    soc_reg64_field32_set(unit, VFP_KEY_CONTROL_1r, &reg_val,
                          _bcm_field_trx_vfp_double_wide_sel[slice_num], 0);
    soc_reg64_field32_set(unit, VFP_KEY_CONTROL_1r, &reg_val,
                          _bcm_field_trx_vfp_field_sel[slice_num][0], 0);
    soc_reg64_field32_set(unit, VFP_KEY_CONTROL_1r, &reg_val,
                          _bcm_field_trx_vfp_field_sel[slice_num][1], 0);
    soc_reg64_field32_set(unit, VFP_KEY_CONTROL_1r, &reg_val,
                         _bcm_field_trx_slice_pairing_field[slice_num >> 1], 0);
    BCM_IF_ERROR_RETURN(
        soc_reg64_set(unit, VFP_KEY_CONTROL_1r, REG_PORT_ANY, 0, reg_val));

    return (soc_reg_field32_modify(unit, VFP_KEY_CONTROL_2r, REG_PORT_ANY,
                           _bcm_field_trx_vfp_ip_header_sel[slice_num],0));
}

/*
 * Function:
 *     _field_kt2_slice_clear
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
STATIC int
_field_kt2_slice_clear(int unit, _field_group_t *fg, _field_slice_t *fs)
{
    int rv;

    switch (fs->stage_id) {
      case _BCM_FIELD_STAGE_INGRESS:
          rv = _bcm_field_trx_ingress_slice_clear(unit, fs->slice_number);
          break;
      case _BCM_FIELD_STAGE_LOOKUP:
          rv = _field_kt2_lookup_slice_clear(unit, fs->slice_number);
          break;
      case _BCM_FIELD_STAGE_EGRESS:
          rv = _bcm_field_trx_egress_slice_clear(unit, fs->slice_number);
          break;
      default:
          rv = BCM_E_INTERNAL;
    }
    return (rv);
}

/*
 * Function:
 *     _field_kt2_write_slice_map_ingress
 * Purpose:
 *     Write FP_SLICE_MAP
 * Parameters:
 *     unit       - (IN) BCM device number
 *     stage_fc   - (IN) stage control structure
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_kt2_write_slice_map_ingress(int unit, _field_stage_t *stage_fc)
{
    fp_slice_map_entry_t entry;
    soc_field_t field;
    uint32 value;
    int vmap_size;
    int index;
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
        VIRTUAL_SLICE_15_PHYSICAL_SLICE_NUMBER_ENTRY_0f
    };
    static const soc_field_t slice_group[] = {
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
        VIRTUAL_SLICE_15_VIRTUAL_SLICE_GROUP_ENTRY_0f
    };

    /* Calculate virtual map size. */
    BCM_IF_ERROR_RETURN
        (_bcm_field_virtual_map_size_get(unit, stage_fc, &vmap_size));

    SOC_IF_ERROR_RETURN(READ_FP_SLICE_MAPm(unit, MEM_BLOCK_ANY, 0, &entry));

    for (index = 0; index < vmap_size; index++) {
        value = stage_fc->vmap[_FP_VMAP_DEFAULT][index].vmap_key;
        field = physical_slice[index];
        soc_FP_SLICE_MAPm_field32_set(unit, &entry, field, value);

        value = stage_fc->vmap[_FP_VMAP_DEFAULT][index].virtual_group;
        field = slice_group[index];
        soc_FP_SLICE_MAPm_field32_set(unit, &entry, field, value);
    }

    return WRITE_FP_SLICE_MAPm(unit, MEM_BLOCK_ALL, 0, &entry);
}

/*
 * Function:
 *     _field_kt2_write_slice_map
 * Purpose:
 *     Write FP_SLICE_MAP, EFP_SLICE_MAP, VFP_SLICE_MAP
 * Parameters:
 *     unit       - (IN) BCM device number
 *     stage_fc   - (IN) stage control structure
 * Returns:
 *     BCM_E_XXX
 */
int
_field_kt2_write_slice_map(int unit, _field_stage_t *stage_fc)
{
    if (stage_fc->stage_id == _BCM_FIELD_STAGE_INGRESS) {
        return _field_kt2_write_slice_map_ingress(unit, stage_fc);
    } else if (stage_fc->stage_id == _BCM_FIELD_STAGE_EGRESS) {
        return _bcm_field_trx_write_slice_map_egress(unit, stage_fc);
    } else if (stage_fc->stage_id == _BCM_FIELD_STAGE_LOOKUP) {
        return _bcm_field_trx_write_slice_map_vfp(unit, stage_fc);
    }

    return BCM_E_INTERNAL;
}

/*
 * Function:
 *     _field_kt2_counter_write
 * Purpose:
 *     Write accumulated sw & hw counters
 * Parameters:
 *   unit          - (IN) BCM device number
 *   stage_fc      - (IN) Stage field control
 *   counter_x_mem - (IN) Memory name for X pipe FP counter
 *   mem_x_buf     - (IN) Memory table entry for X pipe FP counter
 *   counter_y_mem - (IN) Memory name for Y pipe FP counter
 *   mem_y_buf     - (IN) Memory table entry for Y pipe FP counter
 *   idx           - (IN) Counter index
 *   packet_count  - (IN) Packet counter value
 *   byte_count    - (IN) Byte counter value
 * Returns:
 *    BCM_E_XXX
 */
STATIC int
_field_kt2_counter_write(int unit, soc_mem_t mem, int idx, uint32 *buf,
                            _field_counter32_collect_t *cntrs32_buf,
                            _field_counter64_collect_t *cntrs64_buf,
                            uint64 *packet_count, uint64 *byte_count)
{
    uint32  hw_val[2];     /* Parsed field counter value.*/

    /* Set byte count in the memory */
    if (NULL != byte_count) {
        COMPILER_64_ZERO(cntrs64_buf->accumulated_counter);
        COMPILER_64_OR(cntrs64_buf->accumulated_counter, (*byte_count));
        COMPILER_64_SET(cntrs64_buf->last_hw_value,
                        COMPILER_64_HI(*byte_count) & 0xf,
                        COMPILER_64_LO(*byte_count));
        hw_val[0] = COMPILER_64_LO(cntrs64_buf->last_hw_value);
        hw_val[1] = COMPILER_64_HI(cntrs64_buf->last_hw_value);
        soc_mem_field_set(unit, mem, buf, BYTE_COUNTERf, hw_val);
    }
    /* Set packet count in the memory */
    if (NULL != packet_count) {
        COMPILER_64_ZERO(cntrs32_buf->accumulated_counter);
        COMPILER_64_OR(cntrs32_buf->accumulated_counter,
                       (*packet_count));
        cntrs32_buf->last_hw_value =
            (COMPILER_64_LO(*packet_count) & 0x1fffffff);
        hw_val[0] = cntrs32_buf->last_hw_value;
        hw_val[1] = 0;
        soc_mem_field_set(unit, mem, buf, PACKET_COUNTERf, hw_val);
    }

    return soc_mem_write(unit, mem, MEM_BLOCK_ALL, idx, buf);
}

/*
 * Function:
 *     _field_kt2_counter_set
 * Purpose:
 *     Write accumulated sw and hw counters
 * Parameters:
 *   unit          - (IN) BCM device number
 *   stage_fc      - (IN) Stage field control
 *   counter_x_mem - (IN) Memory name for X pipe FP counter
 *   mem_x_buf     - (IN) Memory table entry for X pipe FP counter
 *   counter_y_mem - (IN) Memory name for Y pipe FP counter
 *   mem_y_buf     - (IN) Memory table entry for Y pipe FP counter
 *   idx           - (IN) Counter index
 *   packet_count  - (IN) Packet counter value
 *   byte_count    - (IN) Byte counter value
 * Returns:
 *    BCM_E_XXX
 */
STATIC int
_field_kt2_counter_set(int unit, _field_stage_t *stage_fc,
                          soc_mem_t counter_x_mem, uint32 *mem_x_buf,
                          soc_mem_t counter_y_mem, uint32 *mem_y_buf,
                          int idx, uint64 *packet_count, uint64 *byte_count)
{
    _field_counter32_collect_t *cntrs32_buf;  /* Sw packet counter value    */
    _field_counter64_collect_t *cntrs64_buf;  /* Sw byte counter value      */

    /* Input parameters check. */
    if (NULL == stage_fc) {
        return BCM_E_PARAM;
    }

    /* Write counter value. */
    if (NULL != mem_x_buf) {
        cntrs64_buf = &stage_fc->_field_x64_counters[idx];
        cntrs32_buf = &stage_fc->_field_x32_counters[idx];
        BCM_IF_ERROR_RETURN(_field_kt2_counter_write(
                                unit, counter_x_mem, idx, mem_x_buf,
                                cntrs32_buf, cntrs64_buf,
                                packet_count, byte_count));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _field_kt2_counter_get
 * Purpose:
 *     Update accumulated sw counter and return current counter value.
 * Parameters:
 *   unit          - (IN)  BCM device number
 *   stage_fc      - (IN)  Stage field control
 *   counter_x_mem - (IN)  Memory name for X pipe FP counter
 *   mem_x_buf     - (IN)  Memory table entry for X pipe FP counter
 *   counter_y_mem - (IN)  Memory name for Y pipe FP counter
 *   mem_y_buf     - (IN)  Memory table entry for Y pipe FP counter
 *   idx           - (IN)  Counter index
 *   packet_count  - (OUT) Packet counter value
 *   byte_count    - (OUT) Byte counter value
 * Returns:
 *    BCM_E_XXX
 */
STATIC int
_field_kt2_counter_get(int unit, _field_stage_t *stage_fc,
                          soc_mem_t counter_x_mem, uint32 *mem_x_buf,
                          soc_mem_t counter_y_mem, uint32 *mem_y_buf,
                          int idx, uint64 *packet_count, uint64 *byte_count)
{
    _field_counter32_collect_t *cntrs32_buf = NULL;  /* Sw packet counter value    */
    _field_counter64_collect_t *cntrs64_buf = NULL;  /* Sw byte counter value      */
    soc_memacc_t *memacc_pkt = NULL, *memacc_byte = NULL; /*Memory access cache*/
    uint32  hw_val[2];     /* Parsed field counter value.*/

    /* Input parameters check. */
    if ((NULL == mem_x_buf) || (INVALIDm == counter_x_mem) || \
        (NULL == stage_fc)) {
        return (BCM_E_PARAM);
    }

    /* Byte counter. */
    hw_val[0] = hw_val[1] = 0;
    cntrs64_buf = &stage_fc->_field_x64_counters[idx];
    memacc_byte =
        &(stage_fc->_field_memacc_counters[_FIELD_COUNTER_MEMACC_BYTE]);
    soc_memacc_field_get(memacc_byte, mem_x_buf, hw_val);
    _bcm_field_35bit_counter_update(unit, hw_val, cntrs64_buf);
    /* Return counter value to caller if (out) pointer was provided. */
    if (NULL != byte_count) {
        COMPILER_64_ADD_64(*byte_count, cntrs64_buf->accumulated_counter);
    }

    /* Packet counter. */
    cntrs32_buf = &stage_fc->_field_x32_counters[idx];
    memacc_pkt =
        &(stage_fc->_field_memacc_counters[_FIELD_COUNTER_MEMACC_PACKET]);
    soc_memacc_field_get(memacc_pkt, mem_x_buf, hw_val);
    _bcm_field_29bit_counter_update(unit, hw_val, cntrs32_buf);
    /* Return counter value to caller if (out) pointer was provided. */
    if (NULL != packet_count) {
        COMPILER_64_ADD_64(*packet_count, cntrs32_buf->accumulated_counter);
    }
    return BCM_E_NONE;
}

STATIC int
_field_kt2_ingress_qual_tcam_key_mask_get(int unit,
                                          _field_entry_t *f_ent,
                                          _field_tcam_t *tcam)
{
    int errcode = BCM_E_INTERNAL;
    const unsigned fp_tcam_words
        = BITS2WORDS(soc_mem_field_length(unit,
                    FP_TCAMm,
                    DATAf
                    )
                );

    tcam->key_size
        = WORDS2BYTES(fp_tcam_words
                + BITS2WORDS(soc_mem_field_length(
                        unit,
                        FP_GLOBAL_MASK_TCAMm,
                        FULL_KEYf
                        )
                    )
                );
    tcam->key  = sal_alloc(tcam->key_size, " ");
    tcam->mask = sal_alloc(tcam->key_size, " ");
    if (tcam->key == 0 || tcam->mask == 0) {
        errcode = BCM_E_MEMORY;
        goto error;
    }
    sal_memset(tcam->key,  0, tcam->key_size);
    sal_memset(tcam->mask, 0, tcam->key_size);

    if (f_ent->flags & _FP_ENTRY_INSTALLED) {
        int    tcam_idx;
        uint32 tcam_entry[SOC_MAX_MEM_FIELD_WORDS];

        errcode = _bcm_field_entry_tcam_idx_get(unit, f_ent, &tcam_idx);
        if (BCM_FAILURE(errcode)) {
            goto error;
        }

        errcode = soc_mem_read(unit, FP_TCAMm, MEM_BLOCK_ANY,
                               tcam_idx, tcam_entry);
        if (BCM_FAILURE(errcode)) {
            goto error;
        }
        soc_mem_field_get(unit, FP_TCAMm, tcam_entry, DATAf, tcam->key);
        soc_mem_field_get(unit, FP_TCAMm, tcam_entry, DATA_MASKf, tcam->mask);

        errcode = soc_mem_read(unit,
                FP_GLOBAL_MASK_TCAMm,
                MEM_BLOCK_ANY,
                tcam_idx,
                tcam_entry
                );
        if (BCM_FAILURE(errcode)) {
            goto error;
        }
        soc_mem_field_get(unit,
                FP_GLOBAL_MASK_TCAMm,
                tcam_entry,
                FULL_KEYf,
                tcam->key + fp_tcam_words
                );
        soc_mem_field_get(unit,
                FP_GLOBAL_MASK_TCAMm,
                tcam_entry,
                FULL_MASKf,
                tcam->mask + fp_tcam_words
                );

    }

    return (BCM_E_NONE);

 error:
    if (tcam->key) {
        sal_free(tcam->key);
        tcam->key = 0;
    }
    if (tcam->mask) {
        sal_free(tcam->mask);
        tcam->mask = 0;
    }

    return (errcode);
}

int
_bcm_field_kt2_qual_tcam_key_mask_get(int unit,
                                      _field_entry_t *f_ent,
                                      _field_tcam_t *tcam
                                      )
{
    switch (f_ent->group->stage_id) {
    case _BCM_FIELD_STAGE_INGRESS:
        return (_field_kt2_ingress_qual_tcam_key_mask_get(unit, f_ent, tcam));
    default:
        ;
    }

    return (_field_qual_tcam_key_mask_get(unit, f_ent, tcam, 0));
}

STATIC int
_field_kt2_ingress_qual_tcam_key_mask_set(int unit,
                                          _field_entry_t *f_ent,
                                          unsigned       tcam_idx,
                                          unsigned       validf)
{
    _field_tcam_t  * const tcam = &f_ent->tcam;
    _field_tcam_t  * const extra_tcam = &f_ent->extra_tcam;
    _field_group_t * const fg = f_ent->group;
    uint32         tcam_entry[SOC_MAX_MEM_FIELD_WORDS];
    const unsigned fp_tcam_words
        = BITS2WORDS(soc_mem_field_length(unit,
                    FP_TCAMm,
                    DATAf
                    )
                );


    BCM_IF_ERROR_RETURN(soc_mem_read(unit, FP_TCAMm, MEM_BLOCK_ANY,
                                     tcam_idx, tcam_entry));
    soc_mem_field_set(unit, FP_TCAMm, tcam_entry, DATAf, tcam->key);
    soc_mem_field_set(unit, FP_TCAMm, tcam_entry, DATA_MASKf, tcam->mask);
    soc_mem_field32_set(unit, FP_TCAMm, tcam_entry, VALIDf, validf ? 
                       ((fg->flags & _FP_GROUP_LOOKUP_ENABLED) ? 3 : 2) : 0);
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, FP_TCAMm, MEM_BLOCK_ALL,
                                      tcam_idx, tcam_entry));

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, FP_GLOBAL_MASK_TCAMm, MEM_BLOCK_ANY,
                                     tcam_idx, tcam_entry));
   if (!(f_ent->flags & _FP_ENTRY_USES_IPBM_OVERLAY)) {
       /* In TCAM A fixed and IPBM are valid so setting accordingly */
       soc_mem_field_set(unit,
               FP_GLOBAL_MASK_TCAMm,
               tcam_entry,
               FULL_KEYf,
               tcam->key + fp_tcam_words
               );
       soc_mem_field_set(unit,
               FP_GLOBAL_MASK_TCAMm,
               tcam_entry,
               FULL_MASKf,
               tcam->mask + fp_tcam_words
               );
       soc_mem_field_set(unit, FP_GLOBAL_MASK_TCAMm, tcam_entry, IPBMf,
               f_ent->pbmp.data.pbits);
       soc_mem_field_set(unit, FP_GLOBAL_MASK_TCAMm, tcam_entry, IPBM_MASKf,
               f_ent->pbmp.mask.pbits);
   } else {
       /* In TCAM B IPBM overlay bits are valid so setting accordingly */
       soc_mem_field_set(unit,
               FP_GLOBAL_MASK_TCAMm,
               tcam_entry,
               IPBMf,
               extra_tcam->key 
               );
       soc_mem_field_set(unit,
               FP_GLOBAL_MASK_TCAMm,
               tcam_entry,
               IPBM_MASKf,
               extra_tcam->mask 
               );
    }

    soc_mem_field32_set(unit, FP_GLOBAL_MASK_TCAMm, tcam_entry, VALIDf,
                        validf ? 3 : 0);
    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, FP_GLOBAL_MASK_TCAMm, MEM_BLOCK_ALL,
                      tcam_idx, tcam_entry));

    return (BCM_E_NONE);
}


int
_bcm_field_kt2_qual_tcam_key_mask_set(int            unit,
                                      _field_entry_t *f_ent,
                                      unsigned       validf
                                      )
{
    int       tcam_idx;
    soc_mem_t tcam_mem;

    BCM_IF_ERROR_RETURN(
        _bcm_field_entry_tcam_idx_get(unit, f_ent, &tcam_idx));

    switch (f_ent->group->stage_id) {
    case _BCM_FIELD_STAGE_INGRESS:
        return (_field_kt2_ingress_qual_tcam_key_mask_set(unit,
                                                     f_ent, tcam_idx, validf));
    case _BCM_FIELD_STAGE_EGRESS:
        tcam_mem = EFP_TCAMm;
        break;
    case _BCM_FIELD_STAGE_LOOKUP:
        tcam_mem = VFP_TCAMm;
        break;
    default:
        return (BCM_E_INTERNAL);
    }
    {
        uint32 tcam_entry[SOC_MAX_MEM_FIELD_WORDS];

        BCM_IF_ERROR_RETURN(
            _bcm_field_trx_tcam_get(unit, tcam_mem, f_ent, tcam_entry));
        BCM_IF_ERROR_RETURN(
            soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, tcam_idx, tcam_entry));
    }

    return (BCM_E_NONE);
}


/*
 * Function:
 *     _field_kt2_functions_init
 *
 * Purpose:
 *     Set up functions pointers 
 *
 * Parameters:
 *     stage_fc - (IN/OUT) pointers to stage control block whe the device 
 *                         and stage specific functions will be registered.
 *
 * Returns:
 *     nothing
 * Notes:
 */
STATIC void
_field_kt2_functions_init(_field_funct_t *functions)
{
    functions->fp_detach               = _bcm_field_tr_detach;
    functions->fp_group_install        = _field_kt2_group_install;
    functions->fp_selcodes_install     = _field_kt2_selcodes_install;
    functions->fp_slice_clear          = _field_kt2_slice_clear;
    functions->fp_entry_remove         = _bcm_field_fb_entry_remove;
    functions->fp_entry_move           = _bcm_field_fb_entry_move;
    functions->fp_selcode_get          = _bcm_field_tr_selcode_get;
    functions->fp_selcode_to_qset      = _bcm_field_selcode_to_qset;
    functions->fp_qual_list_get        = _bcm_field_qual_lists_get;
    functions->fp_tcam_policy_clear    = NULL;
    functions->fp_tcam_policy_install  = _bcm_field_tr_entry_install;
    functions->fp_tcam_policy_reinstall = _bcm_field_tr_entry_reinstall;
    functions->fp_policer_install      = _bcm_field_trx_policer_install;
    functions->fp_write_slice_map      = _field_kt2_write_slice_map;
    functions->fp_qualify_ip_type      = _bcm_field_trx_qualify_ip_type;
    functions->fp_qualify_ip_type_get  = _bcm_field_trx_qualify_ip_type_get;
    functions->fp_action_support_check = _bcm_field_trx_action_support_check;
    functions->fp_action_conflict_check = _bcm_field_trx_action_conflict_check;
    functions->fp_counter_get          = _field_kt2_counter_get;
    functions->fp_counter_set          = _field_kt2_counter_set;
    functions->fp_stat_index_get       = _bcm_field_trx_stat_index_get;
    functions->fp_action_params_check  = _bcm_field_trx_action_params_check;
    functions->fp_action_depends_check = _bcm_field_trx_action_depends_check;
    functions->fp_external_entry_install  = NULL;
    functions->fp_external_entry_reinstall = NULL;
    functions->fp_external_entry_remove   = NULL;
    functions->fp_external_entry_prio_set = NULL;
    functions->fp_egress_key_match_type_set =
                      _field_kt2_egress_key_match_type_set;
    functions->fp_data_qualifier_ethertype_add =
                      _bcm_field_trx2_data_qualifier_ethertype_add;
    functions->fp_data_qualifier_ethertype_delete=
                      _bcm_field_trx2_data_qualifier_ethertype_delete;
    functions->fp_data_qualifier_ip_protocol_add =
                      _bcm_field_trx2_data_qualifier_ip_protocol_add;
    functions->fp_data_qualifier_ip_protocol_delete=
                      _bcm_field_trx2_data_qualifier_ip_protocol_delete;
    functions->fp_data_qualifier_packet_format_add=
                      _bcm_field_trx2_data_qualifier_packet_format_add;
    functions->fp_data_qualifier_packet_format_delete=
                      _bcm_field_trx2_data_qualifier_packet_format_delete;
}

int
_bcm_field_kt2_qualify_class(int                 unit,
                             bcm_field_entry_t   entry,
                             bcm_field_qualify_t qual,
                             uint32              *data,
                             uint32              *mask
                             )
{
    _field_entry_t *f_ent;
    unsigned       cl_width, cl_max;

    /* Get field entry part that contains the qualifier. */
    BCM_IF_ERROR_RETURN(_bcm_field_entry_qual_get(unit, entry, qual, &f_ent));

    switch (f_ent->group->stage_id) {
    case _BCM_FIELD_STAGE_INGRESS:
        break;


    case _BCM_FIELD_STAGE_LOOKUP:
        break;

    case _BCM_FIELD_STAGE_EGRESS:
        {
            uint32         ifp_cl_type;

            cl_width = 9;
            cl_max   = 1 << cl_width;

            if (*data >= cl_max || (*mask != BCM_FIELD_EXACT_MATCH_MASK && *mask >= cl_max)) {
                return (BCM_E_PARAM);
            }

            /* Need to set IFP_CLASS_TYPE in TCAM (upper 4 bits) */

            switch (qual) {
            case bcmFieldQualifyIngressClassField:
                ifp_cl_type = 15;
                break;
            default:
                return (BCM_E_INTERNAL);
            }

            *data |= ifp_cl_type << cl_width;
            *mask |= 0xf << cl_width;
        }
        break;

        default:
        return (BCM_E_INTERNAL);
    }

    return (BCM_E_NONE);
}

#else /* BCM_KATANA2_SUPPORT && BCM_FIELD_SUPPORT */
int _kt2_field_not_empty;
#endif  /* BCM_KATANA2_SUPPORT && BCM_FIELD_SUPPORT */
