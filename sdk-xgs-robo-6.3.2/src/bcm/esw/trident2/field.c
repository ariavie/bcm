/* $Id: field.c 1.58 Broadcom SDK $
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
 * Purpose:     BCM56850 Field Processor installation functions.
 */

#include <soc/defs.h>
#if defined(BCM_TRIDENT2_SUPPORT) && defined(BCM_FIELD_SUPPORT)

#include <soc/mem.h>
#include <soc/drv.h>
#include <soc/trident2.h>
#include <soc/triumph3.h>

#include <bcm/error.h>
#include <bcm/l3.h>
#include <bcm/field.h>
#include <bcm/tunnel.h>

#include <bcm_int/common/multicast.h>

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/field.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/policer.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/nat.h>

#define ALIGN32(x)      (((x) + 31) & ~31)

/*
 * Function:
 *     _field_td2_ingress_qualifiers_init
 * Purpose:
 *     Initialize device stage ingress qualifiers
 *     select codes & offsets
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     stage_fc   - (IN) Field Processor stage control structure.
 *
 * Returns:
 *     BCM_E_NONE
 */
STATIC int
_field_td2_ingress_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    const unsigned
        f2_offset = 10 - 2,
        fixed_offset = 138 - 2,
        fixed_pairing_overlay_offset = fixed_offset,
        fp_global_mask_tcam_ofs = ALIGN32(soc_mem_field_length(unit, FP_TCAMm, KEYf)),
        f3_offset = 2 - 2,
        f1_offset = 51 - 2,
        ipbm_pairing_f0_offset = 100 - 2;

    _FP_QUAL_DECL;

    /* Handled outside normal qualifier set/get scheme */

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyStage,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyStageIngress,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp4,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPorts,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyNormalizeIpAddrs,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyNormalizeMacAddrs,
                 _bcmFieldSliceSelDisable, 0,
                 0, 0
                 );

/* FPF1 single wide */
/* Present in Global Mask Field */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 0,
                               fp_global_mask_tcam_ofs + f1_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceDstClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 0,
                               fp_global_mask_tcam_ofs + f1_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceSrcClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVrf,
                               fp_global_mask_tcam_ofs + f1_offset + 20, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVpn,
                               fp_global_mask_tcam_ofs + f1_offset + 20, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVlan,
                               fp_global_mask_tcam_ofs + f1_offset + 20, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIngressEntitySelect, /* TBD */
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset + 33, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIngressEntitySelect, /* TBD */
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f1_offset + 33, 16);


    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunkMemberGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f1_offset + 26, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcGport,
                               _bcmFieldSliceSelFpf1, 1,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityCommonGport,
                               fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf1, 1,
                 fp_global_mask_tcam_ofs + f1_offset + 37, 3);



    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf1, 2,
                 fp_global_mask_tcam_ofs + f1_offset + 21, 3);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 0,
                               fp_global_mask_tcam_ofs + f1_offset + 24, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset + 24, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset + 24, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceDstClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset + 24, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 0,
                               fp_global_mask_tcam_ofs + f1_offset + 34, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset + 34, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset + 34, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 2,
                               _bcmFieldSliceSrcClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset + 34, 10);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset + 28, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset + 29, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterTpid,
                 _bcmFieldSliceSelFpf1, 3,
                 fp_global_mask_tcam_ofs + f1_offset + 32, 2);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 4,
                 fp_global_mask_tcam_ofs + f1_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 4,
                 fp_global_mask_tcam_ofs + f1_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 4,
                 fp_global_mask_tcam_ofs + f1_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 4,
                 fp_global_mask_tcam_ofs + f1_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf1, 4,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanOuter,
                 _bcmFieldSliceSelFpf1, 4,
                 fp_global_mask_tcam_ofs + f1_offset + 32, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanInner,
                 _bcmFieldSliceSelFpf1, 4,
                 fp_global_mask_tcam_ofs + f1_offset + 35, 3);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanTranslationHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 0, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanValid,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 3, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIngressStpState,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 4, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 6, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcStatic,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 7, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2DestHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 8, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2CacheHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 9, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3SrcHostHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpmcStarGroupHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestHostHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 11, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestRouteHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2StationMove,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 13, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDosAttack,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 14, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpTunnelHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 15, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel1Hit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillEgressRbridgeHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreSrcIpHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimSrcGportHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel2Hit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillIngressRbridgeHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreVfiHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimVfiHit,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsTerminated,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 18, 1);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyPacketRes,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 20, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 26, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 26, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 38, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 39, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerTpid,
                 _bcmFieldSliceSelFpf1, 5,
                 fp_global_mask_tcam_ofs + f1_offset + 42, 2);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 16, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 18, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTranslatedVlanFormat,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 20, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 22, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyPacketRes,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 25, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpInfo,
                 _bcmFieldSliceSelFpf1, 6,
                 fp_global_mask_tcam_ofs + f1_offset + 31, 2);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 7,
                 fp_global_mask_tcam_ofs + f1_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 7,
                 fp_global_mask_tcam_ofs + f1_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 7,
                 fp_global_mask_tcam_ofs + f1_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 7,
                 fp_global_mask_tcam_ofs + f1_offset + 13, 3);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 0,
                                fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceSrcClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVrf,
                               fp_global_mask_tcam_ofs + f1_offset + 26,
                               13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVpn,
                               fp_global_mask_tcam_ofs + f1_offset + 26,
                               13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf1, 7,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVlan,
                               fp_global_mask_tcam_ofs + f1_offset + 26,
                               13);
#if 0
/* TBD.. NO MPLS HERE. But RAL,GAL */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsOuterLabelPop,
                 _bcmFieldSliceSelFpf1, 7,
                 fp_global_mask_tcam_ofs + f1_offset + 39, 2);
    _FP_QUAL_ADD(unit, stage_fc,
                 bcmFieldQualifyMplsStationHitTunnelUnterminated,
                 _bcmFieldSliceSelFpf1, 7,
                 fp_global_mask_tcam_ofs + f1_offset + 39, 2);
#endif


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 8, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 0,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceSrcClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVrf,
                               fp_global_mask_tcam_ofs + f1_offset + 26,
                               13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVpn,
                               fp_global_mask_tcam_ofs + f1_offset + 26,
                               13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf1, 8,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVlan,
                               fp_global_mask_tcam_ofs + f1_offset + 26,
                               13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpAddrsNormalized,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 39, 1
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMacAddrsNormalized,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 40, 1
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyNatSrcRealmId,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 41, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyNatDstRealmId,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 43, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyNatNeeded,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 45, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIcmpError,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 46, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIpLocal,
                 _bcmFieldSliceSelFpf1, 8,
                 fp_global_mask_tcam_ofs + f1_offset + 47, 1);


    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData2,
                 _bcmFieldSliceSelFpf1, 9,
                 fp_global_mask_tcam_ofs + f1_offset, 32);



    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf1, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath,
                               fp_global_mask_tcam_ofs + f1_offset, 21);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                 _bcmFieldSliceSelFpf1, 10,
                 fp_global_mask_tcam_ofs + f1_offset + 21, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                 _bcmFieldSliceSelFpf1, 10,
                 fp_global_mask_tcam_ofs + f1_offset + 21, 16);



    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                 _bcmFieldSliceSelFpf1, 11,
                 fp_global_mask_tcam_ofs + f1_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                 _bcmFieldSliceSelFpf1, 11,
                 fp_global_mask_tcam_ofs + f1_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                 _bcmFieldSliceSelFpf1, 11,
                 fp_global_mask_tcam_ofs + f1_offset, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcModuleGport,
                               _bcmFieldSliceSelFpf1, 11,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf1, 11,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf1, 11,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset + 16,
                               16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                       _bcmFieldSliceSelFpf1, 11,
                       fp_global_mask_tcam_ofs + f1_offset + 32, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                       _bcmFieldSliceSelFpf1, 11,
                       fp_global_mask_tcam_ofs + f1_offset + 36, 2);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 12,
                 fp_global_mask_tcam_ofs + f1_offset, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 0,
                               fp_global_mask_tcam_ofs + f1_offset + 12,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset + 12,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset + 12,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceSrcClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset + 12,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset + 22,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f1_offset + 22,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 0,
                               fp_global_mask_tcam_ofs + f1_offset + 38,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 1,
                               fp_global_mask_tcam_ofs + f1_offset + 38,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 2,
                               fp_global_mask_tcam_ofs + f1_offset + 38,
                               10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf1, 12,
                               _bcmFieldSliceDstClassSelect, 3,
                               fp_global_mask_tcam_ofs + f1_offset + 38,
                               10);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf1, 13,
                 fp_global_mask_tcam_ofs + f1_offset + 37, 4);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf1, 13,
                 fp_global_mask_tcam_ofs + f1_offset + 41, 2);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVnTag,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagVn,
                               fp_global_mask_tcam_ofs + f1_offset, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyCnTag,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagCn,
                               fp_global_mask_tcam_ofs + f1_offset, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyFabricQueueTag,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagFabricQueue,
                               fp_global_mask_tcam_ofs + f1_offset,
                               33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVxlanNetworkId,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagVxlanNetworkId,
                               fp_global_mask_tcam_ofs + f1_offset, 24);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVxlanFlags,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagVxlanNetworkId,
                               fp_global_mask_tcam_ofs + f1_offset + 24, 8);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashUpper,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagRtag7A,
                               fp_global_mask_tcam_ofs + f1_offset + 16, 17);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashUpper,
                               _bcmFieldSliceSelFpf1, 13,
                               _bcmFieldSliceAuxTag1Select,
                               _bcmFieldAuxTagRtag7B,
                               fp_global_mask_tcam_ofs + f1_offset + 16, 17);
                                          
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashLower,
                     _bcmFieldDevSelDisable, 0, 
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select, _bcmFieldAuxTagRtag7A, 
                     0,
                     fp_global_mask_tcam_ofs + f1_offset, 16,  
                     fp_global_mask_tcam_ofs + f1_offset + 32, 1,
                     0, 0, 
                     0); 

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashLower,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 13,
                     _bcmFieldSliceAuxTag1Select, _bcmFieldAuxTagRtag7B,
                     0,
                     fp_global_mask_tcam_ofs + f1_offset, 16,
                     fp_global_mask_tcam_ofs + f1_offset + 32, 1,
                     0, 0,
                     0); 
 
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMacAddrsNormalized,
                 _bcmFieldSliceSelFpf1, 14,
                 fp_global_mask_tcam_ofs + f1_offset + 48, 1
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf1, 14, 
                 fp_global_mask_tcam_ofs + f1_offset, 48
                 );

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf1, 15,
                 fp_global_mask_tcam_ofs + f1_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf1, 15,
                 fp_global_mask_tcam_ofs + f1_offset + 8, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf1, 15,
                 fp_global_mask_tcam_ofs + f1_offset + 24, 16);
    /* L4_NORMALIZED */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf1, 15,
                 fp_global_mask_tcam_ofs + f1_offset + 41, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                 _bcmFieldSliceSelFpf1, 15,
                 fp_global_mask_tcam_ofs + f1_offset + 47, 2);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFcoeVersionIsZero,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFcoeSOF,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 1, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanRCtl,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 15, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVersion,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 23, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTHopCount,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 25, 5);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVsanId,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 30, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVsanPri,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 42, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanSrcFpmaCheck,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 45, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanSrcBindCheck,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 46, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanZoneCheck,
                 _bcmFieldSliceSelFpf1, 16,
                 fp_global_mask_tcam_ofs + f1_offset + 47, 2);
    /* FPF2  single wide */

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 0, f2_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 8, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpFlags,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 14, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 24, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIcmpTypeCode,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 56, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 64, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 96, 32);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 1, f2_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 8, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 14, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 24, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIcmpTypeCode,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 40, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 56, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 64, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                 _bcmFieldSliceSelFpf2, 1, f2_offset + 96, 32);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                 _bcmFieldSliceSelFpf2, 2, f2_offset, 128);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6,
                 _bcmFieldSliceSelFpf2, 3, f2_offset, 128);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 4, f2_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 4, f2_offset + 8, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6FlowLabel,
                 _bcmFieldSliceSelFpf2, 4, f2_offset + 14, 20);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf2, 4, f2_offset + 34, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 4, f2_offset + 42, 8);
/* L3_IIF */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6High,
                 _bcmFieldSliceSelFpf2, 4, f2_offset + 64, 64);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf2, 5, f2_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf2, 5, f2_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf2, 5, f2_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf2, 5, f2_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 5, f2_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 5, f2_offset + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 5, f2_offset + 80, 48);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf2, 6, f2_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf2, 6, f2_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf2, 6, f2_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf2, 6, f2_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 6, f2_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 6, f2_offset + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                 _bcmFieldSliceSelFpf2, 6, f2_offset + 80, 32);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf2, 7, f2_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf2, 7, f2_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 32, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 40, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 48, 32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 80, 48);

    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData0,
                 _bcmFieldSliceSelFpf2, 8, f2_offset, 128);

    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData1,
                 _bcmFieldSliceSelFpf2, 9, f2_offset, 128);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6High,
                 _bcmFieldSliceSelFpf2, 10, f2_offset, 64);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6High,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 64, 64);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 11, f2_offset + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 11, f2_offset + 80, 48);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanDFCtl,
                 _bcmFieldSliceSelFpf2, 12, f2_offset + 32, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanCSCtl,
                 _bcmFieldSliceSelFpf2, 12, f2_offset + 40, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanFCtl,
                 _bcmFieldSliceSelFpf2, 12, f2_offset + 48, 24);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanType,
                 _bcmFieldSliceSelFpf2, 12, f2_offset + 72, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanDstId,
                 _bcmFieldSliceSelFpf2, 12, f2_offset + 80, 24);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanSrcId,
                 _bcmFieldSliceSelFpf2, 12, f2_offset + 104, 24);
/* FPF 3 */
/* Present in Global Mask Field */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 0,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 1,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 2,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceDstClassSelect, 3,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 0,
                               fp_global_mask_tcam_ofs + f3_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 1,
                               fp_global_mask_tcam_ofs + f3_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 2,
                               fp_global_mask_tcam_ofs + f3_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceSrcClassSelect, 3,
                               fp_global_mask_tcam_ofs + f3_offset + 10, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVrf,
                               fp_global_mask_tcam_ofs + f3_offset + 20, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVpn,
                               fp_global_mask_tcam_ofs + f3_offset + 20, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVlan,
                               fp_global_mask_tcam_ofs + f3_offset + 20, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f3_offset + 33, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf3, 0,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f3_offset + 33, 16);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 0,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 1,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 2,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceSrcClassSelect, 3,
                               fp_global_mask_tcam_ofs + f3_offset, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVrf,
                               fp_global_mask_tcam_ofs + f3_offset + 10, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVpn,
                               fp_global_mask_tcam_ofs + f3_offset + 10, 13);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyForwardingVlanId,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceFwdFieldSelect,
                               _bcmFieldFwdFieldVlan,
                               fp_global_mask_tcam_ofs + f3_offset + 10, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 1,
                 fp_global_mask_tcam_ofs + f3_offset + 23, 12);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 0,
                               fp_global_mask_tcam_ofs + f3_offset + 35, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 1,
                               fp_global_mask_tcam_ofs + f3_offset + 35, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 2,
                               fp_global_mask_tcam_ofs + f3_offset + 35, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceDstClassSelect, 3,
                               fp_global_mask_tcam_ofs + f3_offset + 35, 10);
#if 0
/* TOS_FN_LOW */
#endif


    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunkMemberGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f3_offset + 28, 10);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcGport,
                               _bcmFieldSliceSelFpf3, 2,
                                _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityCommonGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 17);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcVirtualPortValid,
                 _bcmFieldSliceSelFpf3, 2,
                 fp_global_mask_tcam_ofs + f3_offset + 37, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf3, 2,
                 fp_global_mask_tcam_ofs + f3_offset + 38, 3);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 18, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTranslatedVlanFormat,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 20, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMHOpcode,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 22, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyPacketRes,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 25, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpInfo,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 31, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf3, 3,
                 fp_global_mask_tcam_ofs + f3_offset + 33, 16);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 28, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 29, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 32, 4);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 36, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIpLocal,
                 _bcmFieldSliceSelFpf3, 4,
                 fp_global_mask_tcam_ofs + f3_offset + 38, 1);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, 5,
                 fp_global_mask_tcam_ofs + f3_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 5,
                 fp_global_mask_tcam_ofs + f3_offset, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, 5,
                 fp_global_mask_tcam_ofs + f3_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, 5,
                 fp_global_mask_tcam_ofs + f3_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf3, 5,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanOuter,
                 _bcmFieldSliceSelFpf3, 5,
                 fp_global_mask_tcam_ofs + f3_offset + 32, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanInner,
                 _bcmFieldSliceSelFpf3, 5,
                 fp_global_mask_tcam_ofs + f3_offset + 35, 3);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanTranslationHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 0, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanValid,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 3, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIngressStpState,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 4, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 6, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2SrcStatic,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 7, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2DestHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 8, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2CacheHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 9, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3SrcHostHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpmcStarGroupHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 10, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestHostHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 11, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3DestRouteHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2StationMove,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 13, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDosAttack,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 14, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpTunnelHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 15, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel1Hit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillEgressRbridgeHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreSrcIpHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimSrcGportHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsLabel2Hit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTrillIngressRbridgeHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2GreVfiHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMimVfiHit,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 17, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMplsTerminated,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 18, 1);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyPacketRes,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 20, 6);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 26, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 26, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 38, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 6,
                 fp_global_mask_tcam_ofs + f3_offset + 39, 3);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyRangeCheck,
                 _bcmFieldSliceSelFpf3, 7,
                 fp_global_mask_tcam_ofs + f3_offset, 32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, 7,
                               _bcmFieldSliceIntfClassSelect, 0,
                               fp_global_mask_tcam_ofs + f3_offset + 24, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassVPort,
                               _bcmFieldSliceSelFpf3, 7,
                               _bcmFieldSliceIntfClassSelect, 1,
                               fp_global_mask_tcam_ofs + f3_offset + 24, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, 7,
                               _bcmFieldSliceIntfClassSelect, 2,
                               fp_global_mask_tcam_ofs + f3_offset + 24, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, 7,
                               _bcmFieldSliceIntfClassSelect, 4,
                               fp_global_mask_tcam_ofs + f3_offset + 24, 12);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6FlowLabel,
                 _bcmFieldSliceSelFpf3, 8,
                 fp_global_mask_tcam_ofs + f3_offset, 20);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, 8,
                 fp_global_mask_tcam_ofs + f3_offset + 20, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf3, 8,
                 fp_global_mask_tcam_ofs + f3_offset + 22, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTranslatedVlanFormat,
                 _bcmFieldSliceSelFpf3, 8,
                 fp_global_mask_tcam_ofs + f3_offset + 24, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerTpid,
                 _bcmFieldSliceSelFpf3, 8,
                 fp_global_mask_tcam_ofs + f3_offset + 26, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterTpid,
                 _bcmFieldSliceSelFpf3, 8,
                 fp_global_mask_tcam_ofs + f3_offset + 28, 2);

    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData3,
                 _bcmFieldSliceSelFpf3, 9,
                 fp_global_mask_tcam_ofs + f3_offset, 32);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf3, 10,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                 _bcmFieldSliceSelFpf3, 10,
                 fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                 _bcmFieldSliceSelFpf3, 10,
                 fp_global_mask_tcam_ofs + f3_offset + 21, 16);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                 _bcmFieldSliceSelFpf3, 11,
                 fp_global_mask_tcam_ofs + f3_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                 _bcmFieldSliceSelFpf3, 11,
                 fp_global_mask_tcam_ofs + f3_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                 _bcmFieldSliceSelFpf3, 11,
                 fp_global_mask_tcam_ofs + f3_offset, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                 _bcmFieldSliceSelFpf3, 11,
                 fp_global_mask_tcam_ofs + f3_offset + 7, 9);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf3, 11,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f3_offset + 16, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf3, 11,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f3_offset + 16, 16);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyColor,
                                 _bcmFieldSliceSelFpf3, 12,
                                 fp_global_mask_tcam_ofs + f3_offset + 41, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIntPriority,
                                 _bcmFieldSliceSelFpf3, 12,
                                 fp_global_mask_tcam_ofs + f3_offset + 37, 4);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyVnTag,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagVn,
                               fp_global_mask_tcam_ofs + f3_offset, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyCnTag,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagCn,
                               fp_global_mask_tcam_ofs + f3_offset, 33);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyFabricQueueTag,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagFabricQueue,
                               fp_global_mask_tcam_ofs + f3_offset, 33);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashUpper,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagRtag7A,
                               fp_global_mask_tcam_ofs + f3_offset + 16, 17);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashUpper,
                               _bcmFieldSliceSelFpf3, 12,
                               _bcmFieldSliceAuxTag2Select,
                               _bcmFieldAuxTagRtag7B,
                               fp_global_mask_tcam_ofs + f3_offset + 16, 17);

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7AHashLower,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select, _bcmFieldAuxTagRtag7A,
                     0,
                     fp_global_mask_tcam_ofs + f3_offset, 16,
                     fp_global_mask_tcam_ofs + f3_offset + 32, 1,
                     0, 0,
                     0);

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyRtag7BHashLower,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf3, 12,
                     _bcmFieldSliceAuxTag2Select, _bcmFieldAuxTagRtag7B,
                     0,
                     fp_global_mask_tcam_ofs + f3_offset, 16,
                     fp_global_mask_tcam_ofs + f3_offset + 32, 1,
                     0, 0,
                     0);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                 _bcmFieldSliceSelFpf3, 13,
                 fp_global_mask_tcam_ofs + f3_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeader2Type,
                 _bcmFieldSliceSelFpf3, 13,
                 fp_global_mask_tcam_ofs + f3_offset + 8, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderSubCode,
                 _bcmFieldSliceSelFpf3, 13,
                 fp_global_mask_tcam_ofs + f3_offset + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                 _bcmFieldSliceSelFpf3, 13,
                 fp_global_mask_tcam_ofs + f3_offset + 24, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstIpLocal,
                 _bcmFieldSliceSelFpf3, 13,
                 fp_global_mask_tcam_ofs + f3_offset + 32, 1
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpAddrsNormalized,
                 _bcmFieldSliceSelFpf3, 13,
                 fp_global_mask_tcam_ofs + f3_offset + 33, 1
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMacAddrsNormalized,
                 _bcmFieldSliceSelFpf3, 13,
                 fp_global_mask_tcam_ofs + f3_offset + 34, 1
                 );
#if 0
/* F3.13
L3_IIF
*/
#endif


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMacAddrsNormalized,
                 _bcmFieldSliceSelFpf3, 14,
                 fp_global_mask_tcam_ofs + f3_offset + 48, 1
                 );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf3, 14, 
                 fp_global_mask_tcam_ofs + f3_offset, 48
                 );


    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstPort,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstTrunk,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3Egress,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstL3EgressNextHops,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityL3Egress,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMulticastGroup,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMulticastGroup,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstMultipath,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceDstFwdEntitySelect,
                               _bcmFieldFwdEntityMultipath,
                               fp_global_mask_tcam_ofs + f3_offset, 21);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityGlp,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMplsGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityMimGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                               _bcmFieldSliceSelFpf3, 15,
                               _bcmFieldSliceIngressEntitySelect,
                               _bcmFieldFwdEntityModPortGport,
                               fp_global_mask_tcam_ofs + f3_offset + 21, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, 15,
                 fp_global_mask_tcam_ofs + f3_offset + 37, 12);



    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFcoeVersionIsZero,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFcoeSOF,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 1, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanRCtl,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 15, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVersion,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 23, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTHopCount,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 25, 5);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVsanId,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 30, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVsanPri,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 42, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanSrcFpmaCheck,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 45, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanSrcBindCheck,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 46, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanZoneCheck,
                 _bcmFieldSliceSelFpf3, 16,
                 fp_global_mask_tcam_ofs + f3_offset + 47, 2);
 


    /* FIXED single wide */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMyStationHit,
                 _bcmFieldSliceSelDisable, 0, fixed_offset, 1);
        _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 1, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 2, 3);
    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifySvpValid,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 5, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcVirtualPortValid,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 5, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 6, 5);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 11, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 12, 1);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyLoopback,
                               _bcmFieldSliceSelDisable, 0,
                               _bcmFieldSliceLoopbackTypeSelect, 0,
                               fixed_offset + 17, 1);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyLoopbackType,
                               _bcmFieldSliceSelDisable, 0,
                               _bcmFieldSliceLoopbackTypeSelect, 0,
                               fixed_offset + 13, 5);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTunnelType,
                               _bcmFieldSliceSelDisable, 0,
                               _bcmFieldSliceLoopbackTypeSelect, 1,
                               fixed_offset + 13, 5);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 18, 1);
#if 0
/* Fixed : IPV4_CHECKSUM_OK, REP_COPY */
#endif
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelDisable, 0, fixed_offset + 21, 1);

    /* Overlay of FIXED key in pairing mode */
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     0,
                     0, 0,
                     fixed_pairing_overlay_offset, 2,
                     0, 0,
                     1);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     0,
                     0, 0,
                     fixed_pairing_overlay_offset + 2, 8,
                     0, 0,
                     1);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     0,
                     0, 0,
                     fixed_pairing_overlay_offset + 10, 8,
                     0, 0,
                     1);

    /* Overlay of IPBM in pairing mode */
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     0,
                     0, 0, 
                     fp_global_mask_tcam_ofs + ipbm_pairing_f0_offset, 6, 
                     0, 0,
                     1);  
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     0,
                     0, 0, 
                     fp_global_mask_tcam_ofs + ipbm_pairing_f0_offset + 6, 16,
                     0, 0, 
                     1);   
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     _bcmFieldSliceSelDisable, 0,
                     0,
                     0, 0,
                     fp_global_mask_tcam_ofs + ipbm_pairing_f0_offset + 22, 16,
                     0, 0,
                     1); 

    return (BCM_E_NONE);
}


/*
 * Function:
 *     _field_td2_lookup_qualifiers_init
 * Purpose:
 *     Initialize device stage lookup qaualifiers
 *     select codes & offsets
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     stage_fc   - (IN) Field Processor stage control structure.

 * Returns:
 *     BCM_E_NONE
 */
STATIC int
_field_td2_lookup_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    const unsigned f1_offset = 164, f2_offset = 36, f3_offset = 0;
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

    /* FPF1 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyInnerIpProtocolCommon,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f1_offset, 3);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocolCommon,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f1_offset, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTunnelTerminated,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 3, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerTpid,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 5, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterTpid,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 7, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL2Format,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 9, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGigProxy,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 11, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 13, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 13, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 25, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 26, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 29, 2);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerIpType,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f1_offset + 31,
                               4);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                               _bcmFieldSliceSelFpf1, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f1_offset + 31,
                               4);
    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcPort,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityGlp,
                     0,
                     f1_offset + 36, 7, /* Port value in SGLP */
                     f1_offset + 43, 8, /* Module value in SGLP */
                     f1_offset + 51, 1, /* Trunk bit in SGLP (should be 0) */
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcTrunk,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityGlp,
                     0,
                     f1_offset + 36, 15, /* trunk id field of SGLP */
                     f1_offset + 51, 1,  /* trunk bit of SGLP */
                     0, 0,
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcMplsGport,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityMplsGport,
                     0,
                     f1_offset + 36, 16, /* S_FIELD */
                     f1_offset + 55, 1,  /* SVP_VALID */
                     0, 0,
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcMimGport,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityMimGport,
                     0,
                     f1_offset + 36, 16, /* S_FIELD */
                     f1_offset + 55, 1,  /* SVP_VALID */
                     0, 0,
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcWlanGport,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityWlanGport,
                     0,
                     f1_offset + 36, 16, /* S_FIELD */
                     f1_offset + 55, 1,  /* SVP_VALID */
                     0, 0,
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcModPortGport,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityModPortGport,
                     0,
                     f1_offset + 36, 15, /* mod + port field of unresolved SGLP */
                     0, 0,
                     0, 0,
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityModPortGport,
                     0,
                     f1_offset + 36 + 7, 8, /* mod field of unresolved SGLP */
                     0, 0,
                     0, 0,
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityPortGroupNum,
                     0,
                     f1_offset + 36, 7, /* ingress port field */
                     0, 0,
                     0, 0,
                     0
                     );

    _FP_QUAL_EXT_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                     _bcmFieldDevSelDisable, 0,
                     _bcmFieldSliceSelFpf1, 0,
                     _bcmFieldSliceSrcEntitySelect, _bcmFieldFwdEntityPortGroupNum,
                     0,
                     f1_offset + 36 + 7, 8, /* ingress port group field */
                     0, 0,
                     0, 0,
                     0
                     );

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelFpf1, 0, f1_offset + 56, 1);


    /* FPF2 */
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerTtl,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                 _bcmFieldSliceSelFpf2, 0, f2_offset + 8, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerIpFrag,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 14,
                               2);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 14,
                               2);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 16,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 16,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4DstPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 24,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 24,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4SrcPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 40,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 40,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 56,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 56,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerDstIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 64,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 64,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 96,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                               _bcmFieldSliceSelFpf2, 0,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 96,
                               32);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp6,
                               _bcmFieldSliceSelFpf2, 1,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset, 128);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                               _bcmFieldSliceSelFpf2, 1,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset, 128);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerDstIp6,
                               _bcmFieldSliceSelFpf2, 2,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset, 128);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6,
                               _bcmFieldSliceSelFpf2, 2,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset, 128);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf2, 3, f2_offset + 16, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 3, f2_offset + 32, 48);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 3, f2_offset + 80, 48);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 8,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 8,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerDstIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 16,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 16,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 48,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                               _bcmFieldSliceSelFpf2, 4,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 48,
                               32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcMac,
                 _bcmFieldSliceSelFpf2, 4, f2_offset + 80, 48);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 8,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 8,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerDstIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 16,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 16,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 48,
                               32);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                               _bcmFieldSliceSelFpf2, 5,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 48,
                               32);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMac,
                 _bcmFieldSliceSelFpf2, 5, f2_offset + 80, 48);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset, 64);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset, 64);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerDstIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 64,
                               64);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstIp6High,
                               _bcmFieldSliceSelFpf2, 6,
                               _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 64,
                               64);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVnTag,
                 _bcmFieldSliceSelFpf2, 7, f2_offset, 33);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySnap,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 64, 40);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyLlc,
                 _bcmFieldSliceSelFpf2, 7, f2_offset + 104, 24);

    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData0,
                 _bcmFieldSliceSelFpf2, 8, f2_offset, 128);

    _FP_QUAL_ADD(unit, stage_fc, _bcmFieldQualifyData1,
                 _bcmFieldSliceSelFpf2, 9, f2_offset, 128);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFcoeSOF,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 6, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanDFCtl,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 14, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanCSCtl,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 22, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanFCtl,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 30, 24);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanType,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 54, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanDstId,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 62, 24);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanSrcId,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 86, 24);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanRCtl,
                 _bcmFieldSliceSelFpf2, 10, f2_offset + 110, 8);

    /* FPF3 */
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 0, f3_offset + 0, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 0, f3_offset + 0, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 0, f3_offset + 12, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 0, f3_offset + 13, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                 _bcmFieldSliceSelFpf3, 0, f3_offset + 16, 16);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4DstPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 0,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 0,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerL4SrcPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 16,
                               16);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                               _bcmFieldSliceSelFpf3, 1,
                               _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 16,
                               16);

    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 0,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyTos,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 0,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 8,
                               8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                               _bcmFieldSliceSelFpf3, 2,
                               _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 8,
                               8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderSubCode,
                 _bcmFieldSliceSelFpf3, 2, f3_offset + 16, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                 _bcmFieldSliceSelFpf3, 2, f3_offset + 24, 8);


    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                 _bcmFieldSliceSelFpf3, 3, f3_offset + 0, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, 3, f3_offset + 8, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, 3, f3_offset + 8, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, 3, f3_offset + 20, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, 3, f3_offset + 21, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifySrcModuleGport,
                 _bcmFieldSliceSelFpf3, 3, f3_offset + 24, 8);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTHopCount,
                 _bcmFieldSliceSelFpf3, 5, f3_offset, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVsanId,
                 _bcmFieldSliceSelFpf3, 5, f3_offset + 8, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVsanPri,
                 _bcmFieldSliceSelFpf3, 5, f3_offset + 20, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyFibreChanVFTVersion,
                 _bcmFieldSliceSelFpf3, 5, f3_offset + 23, 2);

    /* DWF3 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 0, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTos,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 0, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTtl,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 8, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 8, 8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 16,
                            6);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpFrag,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 22,
                            2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 22,
                            2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f3_offset + 24,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                            _bcmFieldSliceSelFpf3, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f3_offset + 24,
                            8);

    /* DWF2 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTtl,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 0,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTtl,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 0,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTcpControl,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 8,
                            6);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpFrag,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 14,
                            2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpFrag,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 14,
                            2);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerTos,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 16,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyTos,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 16,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerL4DstPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 24,
                            16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL4DstPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 24,
                            16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerL4SrcPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 40,
                            16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyL4SrcPort,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 40,
                            16);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerIpProtocol,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 56,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyIpProtocol,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 56,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerDstIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 64,
                            32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyDstIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 64,
                            32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset + 96,
                            32);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifySrcIp,
                            _bcmFieldSliceSelFpf2, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset + 96,
                            32);

    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInnerSrcIp6,
                            _bcmFieldSliceSelFpf2, 1,
                            _bcmFieldSliceIpHeaderSelect, 1, f2_offset,
                            128);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifySrcIp6,
                            _bcmFieldSliceSelFpf2, 1,
                            _bcmFieldSliceIpHeaderSelect, 0, f2_offset,
                            128);

    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, _bcmFieldQualifyData1,
                            _bcmFieldSliceSelFpf2, 2,
                            _bcmFieldSliceSelDisable, 0,
                            f2_offset, 128);

    /* DWF1 */
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc,
                            bcmFieldQualifyExtensionHeaderSubCode,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_offset + 0,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyExtensionHeaderType,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_offset + 8,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_offset + 16,
                            8);
    _FP_QUAL_INTRASLICE_ADD(unit, stage_fc, bcmFieldQualifyEtherType,
                            _bcmFieldSliceSelFpf1, 0,
                            _bcmFieldSliceIpHeaderSelect, 0, f1_offset + 24,
                            16);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_td2_egress_qualifiers_init
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
_field_td2_egress_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    _FP_QUAL_DECL;
    _key_fld_ = KEYf;

    /* Input parameters check. */
    if (NULL == stage_fc) {
        return (BCM_E_PARAM);
    }

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyStageEgress,
                 _bcmFieldSliceSelDisable, 0, 0, 0);

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
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIcmpTypeCode,
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
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 149, 7);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 156, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 157, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 158, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 158, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 170, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 171, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 174, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 176, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 177, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 185, 7);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 192, 4);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 12
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 12
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassTrill,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 12
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassL2Gre,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 12
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               197, 13
                               );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 210, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 210, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 210, 13);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 223, 2);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 225, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp4,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY1, 0, 0);

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
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 159, 7);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 166, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 167, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 168, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 168, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 180, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 181, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 184, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 186, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 187, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 12
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 12
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 3,
                               195, 7
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2,
                               _bcmFieldSliceSelEgrClassF2, 0,
                               195, 13
                               );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 208, 5);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 213, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 213, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 213, 13);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 226, 2);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 228, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY2, 0, 0);

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
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIcmpTypeCode,
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
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 0,
                               204, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 1,
                               204, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassTrill,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 2,
                               204, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassL2Gre,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3,
                               _bcmFieldSliceSelEgrClassF3, 2,
                               204, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 216, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIp6,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY3, 0, 0);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL4Ports,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 0, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMplsGport,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 1, 15);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstMimGport,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 1, 15);
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
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 141, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 141, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 142, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 145, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 157, 7);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 164, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 165, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 166, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 166, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 178, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 179, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 182, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 184, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 185, 8);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 1,
                               193, 7);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyCpuQueue,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 2,
                               193, 6);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc,
                               bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4,
                               _bcmFieldSliceSelEgrClassF4, 0,
                               193, 13
                               );
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyIpType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 206, 5);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 211, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVrf,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 211, 13);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVpn,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 211, 13);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 224, 2);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY4, 226, 1);

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGigProxy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 126, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 127, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInnerVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 128, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 140, 7);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyL3Routable,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 147, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyMirrorCopy,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 148, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlan,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 149, 16);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanId,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 149, 12);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanCfi,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 161, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOuterVlanPri,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 162, 3);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyVlanFormat,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 165, 2);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDstHiGig,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 167, 1);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 168, 8);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyOutPort,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 176, 7);
    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyDrop,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 183, 1);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifySrcClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyDstClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL2,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyInterfaceClassL3,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClass,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 1,
                               200, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassL3Interface,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 2,
                               200, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassTrill,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 3,
                               200, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyEgressClassL2Gre,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 3,
                               200, 12);
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressClassField,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );
    _FP_QUAL_TWO_SLICE_SEL_ADD(unit, stage_fc, bcmFieldQualifyIngressInterfaceClassPort,
                               _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5,
                               _bcmFieldSliceSelEgrClassF1, 0,
                               200, 13
                               );

    _FP_QUAL_ADD(unit, stage_fc, bcmFieldQualifyForwardingType,
                 _bcmFieldSliceSelFpf3, _BCM_FIELD_EFP_KEY5, 198, 2);
 
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_td2_lookup_selcodes_install
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
_field_td2_lookup_selcodes_install(int            unit,
                                   _field_group_t *fg,
                                   uint8          slice_num,
                                   int            selcode_idx
                                   )
{
    static const soc_field_t s_type_fld_tbl[] = {
        SLICE_0_S_TYPE_SELf,
        SLICE_1_S_TYPE_SELf,
        SLICE_2_S_TYPE_SELf,
        SLICE_3_S_TYPE_SELf
    };

    _field_sel_t * const sel = &fg->sel_codes[selcode_idx];
    int           errcode = BCM_E_NONE;
    uint64        regval;
    uint64        val;

    BCM_IF_ERROR_RETURN(soc_reg64_get(unit,
                                      VFP_KEY_CONTROL_1r,
                                      REG_PORT_ANY,
                                      0,
                                      &regval
                                      )
                        );


    if ((fg->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE)
        && (selcode_idx & 1)
        ) {
        if (sel->fpf2 != _FP_SELCODE_DONT_CARE) {
            COMPILER_64_SET(val, 0, sel->fpf2);
            soc_reg64_field_set(unit,
                                VFP_KEY_CONTROL_1r,
                                &regval,
                                _bcm_field_trx_vfp_double_wide_sel[slice_num],
                                val
                                );
        }
    } else {
        if (sel->fpf2 != _FP_SELCODE_DONT_CARE) {
            COMPILER_64_SET(val, 0, sel->fpf2);
            soc_reg64_field_set(unit,
                                VFP_KEY_CONTROL_1r,
                                &regval,
                                _bcm_field_trx_vfp_field_sel[slice_num][0],
                                val
                                );
        }
        if (sel->fpf3 != _FP_SELCODE_DONT_CARE) {
            COMPILER_64_SET(val, 0, sel->fpf3);
            soc_reg64_field_set(unit,
                                VFP_KEY_CONTROL_1r,
                                &regval,
                                _bcm_field_trx_vfp_field_sel[slice_num][1],
                                val
                                );
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
        soc_reg64_field_set(unit,
                            VFP_KEY_CONTROL_1r,
                            &regval,
                            s_type_fld_tbl[slice_num],
                            val
                            );
    }

    BCM_IF_ERROR_RETURN(soc_reg64_set(unit,
                                      VFP_KEY_CONTROL_1r,
                                      REG_PORT_ANY,
                                      0,
                                      regval
                                      )
                        );

    /* Set inner/outer ip header selection. */
    if (sel->ip_header_sel != _FP_SELCODE_DONT_CARE) {
        errcode = soc_reg_field32_modify(
                      unit,
                      VFP_KEY_CONTROL_2r,
                      REG_PORT_ANY,
                      _bcm_field_trx_vfp_ip_header_sel[slice_num],
                      sel->ip_header_sel
                      );
    }

    return (errcode);
}

STATIC int
_field_td2_egress_selcodes_install(int            unit,
                                   _field_group_t *fg,
                                   uint8          slice_num,
                                   bcm_pbmp_t     *pbmp,
                                   int            selcode_idx
                                   )
{
    static const soc_field_t fldtbl[][4] = {
        { SLICE_0_F1f, SLICE_1_F1f, SLICE_2_F1f, SLICE_3_F1f },
        { SLICE_0_F2f, SLICE_1_F2f, SLICE_2_F2f, SLICE_3_F2f },
        { SLICE_0_F4f, SLICE_1_F4f, SLICE_2_F4f, SLICE_3_F4f }
    };

    _field_sel_t * const sel = &fg->sel_codes[selcode_idx];

    if (sel->egr_class_f1_sel != _FP_SELCODE_DONT_CARE) {
        BCM_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                                                   EFP_CLASSID_SELECTORr,
                                                   REG_PORT_ANY,
                                                   fldtbl[0][slice_num],
                                                   sel->egr_class_f1_sel
                                                   )
                            );
    }
    if (sel->egr_class_f2_sel != _FP_SELCODE_DONT_CARE) {
        BCM_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                                                   EFP_CLASSID_SELECTORr,
                                                   REG_PORT_ANY,
                                                   fldtbl[1][slice_num],
                                                   sel->egr_class_f2_sel
                                                   )
                            );
    }
    if (sel->egr_class_f4_sel != _FP_SELCODE_DONT_CARE) {
        BCM_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                                                   EFP_CLASSID_SELECTORr,
                                                   REG_PORT_ANY,
                                                   fldtbl[2][slice_num],
                                                   sel->egr_class_f4_sel
                                                   )
                            );
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_td2_lookup_mode_set
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
_field_td2_lookup_mode_set(int            unit,
                           uint8          slice_num,
                           _field_group_t *fg,
                           uint8          flags
                           )
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

    BCM_IF_ERROR_RETURN(soc_reg64_get(unit,
                                      VFP_KEY_CONTROL_1r,
                                      REG_PORT_ANY,
                                      0,
                                      &vfp_key_control_1_buf
                                      )
                        );

    soc_reg64_field32_set(unit,
                          VFP_KEY_CONTROL_1r,
                          &vfp_key_control_1_buf,
                          pairing_flds[slice_num >> 1],
                          flags & _FP_GROUP_SPAN_DOUBLE_SLICE ? 1 : 0
                          );
    soc_reg64_field32_set(unit,
                          VFP_KEY_CONTROL_1r,
                          &vfp_key_control_1_buf,
                          wide_mode_flds[slice_num],
                          flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE ? 1 : 0
                          );


    return (soc_reg64_set(unit,
                          VFP_KEY_CONTROL_1r,
                          REG_PORT_ANY,
                          0,
                          vfp_key_control_1_buf
                          )
            );
}

/*
 * Function:
 *     _field_td2_mode_set
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
_field_td2_mode_set(int unit, uint8 slice_num, _field_group_t *fg, uint8 flags)
{
    int (*func)(int unit, uint8 slice_num, _field_group_t *fg, uint8 flags);

    switch (fg->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            func = _field_td2_lookup_mode_set;
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
_field_td2_selcodes_install(int            unit,
                            _field_group_t *fg,
                            uint8          slice_num,
                            bcm_pbmp_t     pbmp,
                            int            selcode_idx
                            )
{
    int rv;

    /* Input parameters check. */
    if (NULL == fg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_field_td2_mode_set(unit, slice_num, fg, fg->flags));

    switch (fg->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            rv = _field_td2_lookup_selcodes_install(unit, fg, slice_num,
                                                    selcode_idx);
            break;
        case _BCM_FIELD_STAGE_INGRESS:
            rv =  _bcm_field_trx_ingress_selcodes_install(unit, fg, slice_num,
                                                          &pbmp, selcode_idx);
            break;
        case _BCM_FIELD_STAGE_EGRESS:
            rv = _field_td2_egress_selcodes_install(unit, fg, slice_num,
                                                    &pbmp, selcode_idx);;
            break;
        default:
            return (BCM_E_INTERNAL);
    }
    return (rv);
}

STATIC int
_field_td2_group_install(int unit, _field_group_t *fg)
{
    _field_slice_t *fs;        /* Slice pointer.           */
    uint8  slice_number;       /* Slices iterator.         */
    int    parts_count;        /* Number of entry parts.   */
    int    idx;                /* Iteration index.         */

    if (NULL == fg) {
        return (BCM_E_PARAM);
    }

    /* Get number of entry parts. */
    BCM_IF_ERROR_RETURN(_bcm_field_entry_tcam_parts_count(unit,
                                                          fg->flags,
                                                          &parts_count
                                                          )
                        );

    for (idx = 0; idx < parts_count; ++idx) {
        BCM_IF_ERROR_RETURN(_bcm_field_tcam_part_to_slice_number(idx,
                                                                 fg->flags,
                                                                 &slice_number
                                                                 )
                            );
        fs = fg->slices + slice_number;

        BCM_IF_ERROR_RETURN(_field_td2_selcodes_install(unit,
                                                        fg,
                                                        fs->slice_number,
                                                        fg->pbmp,
                                                        idx
                                                        )
                            );
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _field_td2_lookup_slice_clear
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
_field_td2_lookup_slice_clear(int unit, unsigned slice_num)
{
    uint64 reg_val;

    BCM_IF_ERROR_RETURN(soc_reg64_get(unit,
                                      VFP_KEY_CONTROL_1r,
                                      REG_PORT_ANY,
                                      0,
                                      &reg_val
                                      )
                        );
    soc_reg64_field32_set(unit,
                          VFP_KEY_CONTROL_1r,
                          &reg_val,
                          _bcm_field_trx_vfp_double_wide_sel[slice_num],
                          0
                          );
    soc_reg64_field32_set(unit,
                          VFP_KEY_CONTROL_1r,
                          &reg_val,
                          _bcm_field_trx_vfp_field_sel[slice_num][0],
                          0
                          );
    soc_reg64_field32_set(unit,
                          VFP_KEY_CONTROL_1r,
                          &reg_val,
                          _bcm_field_trx_vfp_field_sel[slice_num][1],
                          0
                          );
    soc_reg64_field32_set(unit,
                          VFP_KEY_CONTROL_1r,
                          &reg_val,
                          _bcm_field_trx_slice_pairing_field[slice_num >> 1],
                          0
                          );
    BCM_IF_ERROR_RETURN(soc_reg64_set(unit,
                                      VFP_KEY_CONTROL_1r,
                                      REG_PORT_ANY,
                                      0,
                                      reg_val
                                      )
                        );


    return (soc_reg_field32_modify(unit,
                                   VFP_KEY_CONTROL_2r,
                                   REG_PORT_ANY,
                                   _bcm_field_trx_vfp_ip_header_sel[slice_num],
                                   0
                                   )
            );
}

/*
 * Function:
 *     _bcm_field_td2_slice_clear
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
_field_td2_slice_clear(int unit, _field_group_t *fg, _field_slice_t *fs)
{
    int rv;

    switch (fs->stage_id) {
      case _BCM_FIELD_STAGE_INGRESS:
          rv = _bcm_field_trx_ingress_slice_clear(unit, fs->slice_number);
          break;
      case _BCM_FIELD_STAGE_LOOKUP:
          rv = _field_td2_lookup_slice_clear(unit, fs->slice_number);
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
 *     _field_td2_entry_move
 * Purpose:
 *     Copy an entry from one TCAM index to another. It copies the values in
 *     hardware from the old index to the new index. 
 *     IT IS ASSUMED THAT THE NEW INDEX IS EMPTY (VALIDf=00) IN HARDWARE.
 *     The old Hardware index is cleared at the end.
 * Parameters:
 *     unit           - (IN) BCM device number. 
 *     f_ent          - (IN) Entry to move
 *     parts_count    - (IN) Field entry parts count.
 *     tcam_idx_old   - (IN) Source entry tcam index.
 *     tcam_idx_new   - (IN) Destination entry tcam index.
 *                          
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_td2_entry_move(int unit, _field_entry_t *f_ent, int parts_count,
                      int *tcam_idx_old, int *tcam_idx_new)
{
    uint32  e[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to zero TCAM & Policy entry.*/
    uint32  p[_FP_MAX_ENTRY_WIDTH][SOC_MAX_MEM_FIELD_WORDS]; /* For policies  */
    int new_slice_numb = 0;             /* Entry new slice number.            */
    int new_slice_idx = 0;              /* Entry new offset in the slice      */
    soc_mem_t tcam_mem;                 /* TCAM memory id.                    */
    soc_mem_t policy_mem;               /* Policy table memory id .           */
    int tcam_idx_max;                   /* TCAM memory max index.             */
    int tcam_idx_min;                   /* TCAM memory min index.             */
    _field_stage_t *stage_fc;           /* Stage field control structure.     */
    _field_stage_id_t stage_id;         /* Field pipeline stage id.           */
    int idx;                            /* Iteration index.                   */
    _field_policer_t *f_pl = NULL;      /* Field policer descriptor.          */
    _field_stat_t    *f_st = NULL;      /* Field statistics descriptor.       */
    _field_group_t   *fg;               /* Field group structure.             */
    int              rv;                /* Operation return status.           */
    fp_global_mask_tcam_x_entry_t fp_global_mask_x[_FP_MAX_ENTRY_WIDTH];
    fp_global_mask_tcam_y_entry_t fp_global_mask_y[_FP_MAX_ENTRY_WIDTH];
    fp_global_mask_tcam_entry_t fp_global_mask[_FP_MAX_ENTRY_WIDTH];
    fp_gm_fields_entry_t fp_gm_fields[_FP_MAX_ENTRY_WIDTH];
    bcm_pbmp_t pbmp_x, pbmp_y;


    /* Input parameters check. */
    if ((NULL == f_ent) || (NULL == tcam_idx_old) || (NULL == tcam_idx_new)) {
        return (BCM_E_PARAM);
    }

    fg = f_ent->group;

    /* Get field stage control . */
    stage_id = f_ent->group->stage_id;
    rv = _field_stage_control_get(unit, stage_id, &stage_fc);
    BCM_IF_ERROR_RETURN(rv);

    /* Get entry tcam and actions. */
    rv = _field_fb_tcam_policy_mem_get(unit, stage_id, &tcam_mem, &policy_mem);
    BCM_IF_ERROR_RETURN(rv);

    tcam_idx_max = soc_mem_index_max(unit, tcam_mem);
    tcam_idx_min = soc_mem_index_min(unit, tcam_mem);

    for (idx = 0; idx < parts_count; idx++) {
        /* Index sanity check. */
        if ((tcam_idx_old[idx] < tcam_idx_min) || (tcam_idx_old[idx] > tcam_idx_max) ||
            (tcam_idx_new[idx] < tcam_idx_min) || (tcam_idx_new[idx] > tcam_idx_max)) {
            FP_VVERB(("FP(unit %d) vverb: Invalid index range for _field_td2_entry_move \n \
                      from %d to %d", unit, tcam_idx_old[idx], tcam_idx_new[idx]));
            return (BCM_E_PARAM);
        }

        /* Read policy entry from current tcam index. */
        rv = soc_mem_read(unit, policy_mem, MEM_BLOCK_ANY, tcam_idx_old[idx], p[idx]);
        BCM_IF_ERROR_RETURN(rv);

        /*
         * On Trident2, VALIDf of FP_GLOBAL_MASK_TCAMm must be set
         * for all parts of an wide/paired mode entry.
         * This bit would have been set during entry install.
         * Read this value from all parts of an entry while moving.
         * Otherwise, actions related to this entry would not work.
         */
        if (SOC_IS_TRIDENT2(unit) && (stage_id == _BCM_FIELD_STAGE_INGRESS)) {
            if ((fg->flags & _FP_GROUP_SPAN_DOUBLE_SLICE)
                && (0x1 & idx)) {
                rv = READ_FP_GM_FIELDSm(unit, MEM_BLOCK_ANY,
                        tcam_idx_old[idx], &fp_gm_fields[idx]);
                BCM_IF_ERROR_RETURN(rv);
            } else {
                rv = READ_FP_GLOBAL_MASK_TCAMm(unit, MEM_BLOCK_ANY,
                        tcam_idx_old[idx], &fp_global_mask[idx]);
                BCM_IF_ERROR_RETURN(rv);
            }
        }
    }

    /* Calculate primary entry new slice & offset in the slice. */
    rv = _bcm_field_tcam_idx_to_slice_offset(unit, stage_fc, tcam_idx_new[0],
                                             &new_slice_numb, &new_slice_idx);
    BCM_IF_ERROR_RETURN(rv);

    /* Update policy entry if moving across the slices. */
    if (f_ent->fs->slice_number != new_slice_numb) {
        /* Get policer associated with the entry. */
        if ((0 == (stage_fc->flags & _FP_STAGE_GLOBAL_METER_POOLS)) &&
            (f_ent->policer[0].flags & _FP_POLICER_INSTALLED)) {
            BCM_IF_ERROR_RETURN (_bcm_field_policer_get(unit,
                                                        f_ent->policer[0].pid,
                                                        &f_pl));
        }
        /* Get statistics entity associated with the entry. */
        if ((0 == (stage_fc->flags & _FP_STAGE_GLOBAL_COUNTERS)) &&
            (f_ent->statistic.flags & _FP_ENTRY_STAT_INSTALLED)) {
            BCM_IF_ERROR_RETURN (_bcm_field_stat_get(unit,
                                                     f_ent->statistic.sid,
                                                     &f_st));
        }
        if (fg->flags & (_FP_GROUP_SPAN_SINGLE_SLICE |
                         _FP_GROUP_INTRASLICE_DOUBLEWIDE)) {
            /*
             * For _FP_GROUP_INTRASLICE_DOUBLEWIDE, *even* if it is
             *     _FP_GROUP_SPAN_DOUBLE_SLICE, we do this.
             *     This is because in intraslice double-wide, the PRI
             *     slice has tcam_slice_sz/2 entries, and same number
             *     of counter/meter pairs.
             *         Thus, counter/meter will always be allocated in the
             *         PRI slice.
             */
            if (NULL != f_st) {
                /*
                 * Set the index of the counter for entry in new slice
                 * The new index has already been calculated in
                 * _field_entry_move
                 */
                soc_mem_field32_set(unit, policy_mem, (uint32 *) p[0],
                                    COUNTER_INDEXf, f_st->hw_index);
                soc_mem_field32_set(unit, policy_mem, (uint32 *) p[0],
                                    COUNTER_MODEf, f_st->hw_mode);
            }
            if (NULL != f_pl) {
                /*
                 * Set the index of the meter for entry in new slice
                 * The new index has already been calculated in
                 * _field_entry_move
                 */
                soc_mem_field32_set(unit, policy_mem, (uint32 *) p[0],
                                    METER_INDEX_EVENf, f_pl->hw_index);
                soc_mem_field32_set(unit, policy_mem, (uint32 *) p[0],
                                    METER_INDEX_ODDf, f_pl->hw_index);
            }
        } else {
            if (NULL != f_st) {
                _bcm_field_fb_counter_adjust_wide_mode(unit, policy_mem,
                                                       f_st, f_ent,
                                                       f_ent + 1,
                                                       new_slice_numb,
                                                       p[0], p[1]);
            }
            if (NULL != f_pl) {
                _bcm_field_fb_meter_adjust_wide_mode(unit, policy_mem,
                                                     f_pl, f_ent, f_ent + 1,
                                                     p[0], p[1]);
            }
        }
    }

    /*
     * Write entry to the destination
     * ORDER is important
     */
    for (idx = parts_count - 1; idx >= 0; idx--) {

        /* Write duplicate  policy entry to new tcam index. */
        rv = soc_mem_write(unit, policy_mem, MEM_BLOCK_ALL, tcam_idx_new[idx], p[idx]);
        BCM_IF_ERROR_RETURN(rv);
        if ((SOC_IS_TRIDENT2(unit)) && (stage_id == _BCM_FIELD_STAGE_INGRESS)) {
            if ((fg->flags & _FP_GROUP_SPAN_DOUBLE_SLICE)
                && (0x1 & idx)) {
                rv = WRITE_FP_GM_FIELDSm(unit, MEM_BLOCK_ALL,
                        tcam_idx_new[idx], &fp_gm_fields[idx]);
                BCM_IF_ERROR_RETURN(rv);
            } else {
                rv = READ_FP_GLOBAL_MASK_TCAM_Xm(unit, MEM_BLOCK_ANY,
                        tcam_idx_old[idx], &fp_global_mask_x[idx]);
                BCM_IF_ERROR_RETURN(rv);
                rv = READ_FP_GLOBAL_MASK_TCAM_Ym(unit, MEM_BLOCK_ANY,
                        tcam_idx_old[idx], &fp_global_mask_y[idx]);
                BCM_IF_ERROR_RETURN(rv);

                soc_mem_pbmp_field_get(unit, FP_GLOBAL_MASK_TCAM_Xm,
                                       &fp_global_mask_x[idx],
                                       IPBMf, &pbmp_x);
                soc_mem_pbmp_field_get(unit, FP_GLOBAL_MASK_TCAM_Ym,
                                       &fp_global_mask_y[idx],
                                       IPBMf, &pbmp_y); 
                BCM_PBMP_OR(pbmp_x, pbmp_y); 

                soc_mem_pbmp_field_set(unit,
                                       FP_GLOBAL_MASK_TCAMm,
                                       &fp_global_mask[idx],
                                       IPBMf,
                                       &pbmp_x
                                       );

                rv = WRITE_FP_GLOBAL_MASK_TCAMm(unit, MEM_BLOCK_ALL,
                        tcam_idx_new[idx], &fp_global_mask[idx]);
                BCM_IF_ERROR_RETURN(rv);
            }
        }

        /* Read tcam entry from current tcam index. */
        rv = soc_mem_read(unit, tcam_mem, MEM_BLOCK_ANY, tcam_idx_old[idx], e);
        BCM_IF_ERROR_RETURN(rv);

        /* Write duplicate  tcam entry to new tcam index. */
        rv = soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, tcam_idx_new[idx], e);
        BCM_IF_ERROR_RETURN(rv);
    }

    /*
     * Clear old location
     * ORDER is important
     */
    for (idx = 0; idx < parts_count; idx++) {
        rv = _field_fb_tcam_policy_clear(unit, stage_id, tcam_idx_old[idx]);
        BCM_IF_ERROR_RETURN(rv);
    }
    return (BCM_E_NONE);
}

int
_bcm_field_td2_qualify_LoopbackType(bcm_field_LoopbackType_t loopback_type,
                                    uint32                   *tcam_data,
                                    uint32                   *tcam_mask
                                    )
{
    switch (loopback_type) {
        case bcmFieldLoopbackTypeMim:
            *tcam_data = 0x10;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldLoopbackTypeTrillNetwork:
            *tcam_data = 0x11;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldLoopbackTypeTrillAccess:
            *tcam_data = 0x12;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldLoopbackTypeQcn:
            *tcam_data = 0x17;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldLoopbackTypeVxlan:
            *tcam_data = 0x1b;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldLoopbackTypeL2Gre:
            *tcam_data = 0x1e;
            *tcam_mask = 0x1f;
            break;
        default:
            return (BCM_E_PARAM);
    }

    return (BCM_E_NONE);
}

int
_bcm_field_td2_qualify_LoopbackType_get(uint8                    tcam_data,
                                        uint8                    tcam_mask,
                                        bcm_field_LoopbackType_t *loopback_type
                                        )
{
    switch (tcam_data & tcam_mask) {
        case 0x10:
            *loopback_type = bcmFieldLoopbackTypeMim;
            break;
        case 0x11:
            *loopback_type = bcmFieldLoopbackTypeTrillNetwork;
            break;
        case 0x12:
            *loopback_type = bcmFieldLoopbackTypeTrillAccess;
            break;
        case 0x17:
            *loopback_type = bcmFieldLoopbackTypeQcn;
            break;
        case 0x1b:
            *loopback_type = bcmFieldLoopbackTypeVxlan;
            break;
        case 0x1e:
            *loopback_type = bcmFieldLoopbackTypeL2Gre;
            break;
        default:
            return (BCM_E_PARAM);
    }

    return (BCM_E_NONE);
}

int
_bcm_field_td2_qualify_TunnelType(bcm_field_TunnelType_t tunnel_type,
                                  uint32                 *tcam_data,
                                  uint32                 *tcam_mask
                                  )
{
    switch (tunnel_type) {
        case bcmFieldTunnelTypeAny:
            *tcam_data = 0x0;
            *tcam_mask = 0x0;
            break;
        case bcmFieldTunnelTypeIp:
            *tcam_data = 0x1;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldTunnelTypeMpls:
            *tcam_data = 0x2;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldTunnelTypeMim:
            *tcam_data = 0x3;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldTunnelTypeAutoMulticast:
            *tcam_data = 0x6;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldTunnelTypeTrill:
            *tcam_data = 0x7;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldTunnelTypeL2Gre:
            *tcam_data = 0x8;
            *tcam_mask = 0x1f;
            break;
        case bcmFieldTunnelTypeVxlan:
            *tcam_data = 0x9;
            *tcam_mask = 0x1f;
            break;
        default:
            return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}

int
_bcm_field_td2_qualify_TunnelType_get(uint8                  tcam_data,
                                      uint8                  tcam_mask,
                                      bcm_field_TunnelType_t *tunnel_type
                                      )
{
    switch (tcam_data & tcam_mask) {
        case 0x1:
            *tunnel_type = bcmFieldTunnelTypeIp;
            break;
        case 0x2:
            *tunnel_type = bcmFieldTunnelTypeMpls;
            break;
        case 0x3:
            *tunnel_type = bcmFieldTunnelTypeMim;
            break;
        case 0x6:
            *tunnel_type = bcmFieldTunnelTypeAutoMulticast;
            break;
        case 0x7:
            *tunnel_type = bcmFieldTunnelTypeTrill;
            break;
        case 0x8:
            *tunnel_type = bcmFieldTunnelTypeL2Gre;
            break;
        case 0x9:
            *tunnel_type = bcmFieldTunnelTypeVxlan;
            break;
        default:
            return (BCM_E_INTERNAL);
    }

    return (BCM_E_NONE);
}

static const struct {
    uint8 api, hw;
} pkt_res_xlate_tbl[] = {
    {  BCM_FIELD_PKT_RES_UNKNOWN, 0x00 },
    {  BCM_FIELD_PKT_RES_CONTROL, 0x01 },
    {  BCM_FIELD_PKT_RES_BPDU, 0x04 },
    {  BCM_FIELD_PKT_RES_L2BC, 0x0c },
    {  BCM_FIELD_PKT_RES_L2UC, 0x08 },
    {  BCM_FIELD_PKT_RES_L2UNKNOWN, 0x09 },
    {  BCM_FIELD_PKT_RES_L3MCUNKNOWN, 0x13 },
    {  BCM_FIELD_PKT_RES_L3MCKNOWN, 0x12 },
    {  BCM_FIELD_PKT_RES_L2MCKNOWN, 0x0a },
    {  BCM_FIELD_PKT_RES_L2MCUNKNOWN, 0x0b },
    {  BCM_FIELD_PKT_RES_L3UCKNOWN, 0x10 },
    {  BCM_FIELD_PKT_RES_L3UCUNKNOWN, 0x11 },
    {  BCM_FIELD_PKT_RES_MPLSKNOWN, 0x1c },
    {  BCM_FIELD_PKT_RES_MPLSL3KNOWN, 0x1a },
    {  BCM_FIELD_PKT_RES_MPLSL2KNOWN, 0x18 },
    {  BCM_FIELD_PKT_RES_MPLSUNKNOWN, 0x19 },
    {  BCM_FIELD_PKT_RES_MIMKNOWN, 0x20 },
    {  BCM_FIELD_PKT_RES_MIMUNKNOWN, 0x21 },
    {  BCM_FIELD_PKT_RES_TRILLKNOWN, 0x28 },
    {  BCM_FIELD_PKT_RES_TRILLUNKNOWN, 0x29 },
    {  BCM_FIELD_PKT_RES_NIVKNOWN, 0x30 },
    {  BCM_FIELD_PKT_RES_NIVUNKNOWN, 0x31 },
    {  BCM_FIELD_PKT_RES_OAM, 0x02 },
    {  BCM_FIELD_PKT_RES_BFD, 0x03 },
    {  BCM_FIELD_PKT_RES_ICNM, 0x05 },
    {  BCM_FIELD_PKT_RES_IEEE1588, 0x06 },
    {  BCM_FIELD_PKT_RES_L2GREKNOWN, 0x32 },
    {  BCM_FIELD_PKT_RES_VXLANKNOWN, 0x33 },
    {  BCM_FIELD_PKT_RES_FCOEKNOWN, 0x34 },
    {  BCM_FIELD_PKT_RES_FCOEUNKNOWN, 0x35 }
};


int
_bcm_field_td2_qualify_PacketRes(int               unit,
                                 bcm_field_entry_t entry,
                                 uint32            *data,
                                 uint32            *mask
                                 )
{
    unsigned i;


    /* Translate data #defines to hardware encodings */

    for (i = 0; i < COUNTOF(pkt_res_xlate_tbl); ++i) {
        if (*data == pkt_res_xlate_tbl[i].api) {
            *data = pkt_res_xlate_tbl[i].hw;
            return (BCM_E_NONE);
        }
    }

    return (BCM_E_INTERNAL);
}


int
_bcm_field_td2_qualify_PacketRes_get(int               unit,
                                     bcm_field_entry_t entry,
                                     uint32            *data,
                                     uint32            *mask
                                     )
{
    unsigned i;

    /* Translate data #defines from hardware encodings */

    for (i = 0; i < COUNTOF(pkt_res_xlate_tbl); ++i) {
        if (*data == pkt_res_xlate_tbl[i].hw) {
            *data = pkt_res_xlate_tbl[i].api;
            return (BCM_E_NONE);
        }
    }

    return (BCM_E_INTERNAL);
}

/*
 * Function:
 *     _bcm_field_td2_hash_select_profile_get
 * Purpose:
 *     Get the redirect profile for the unit
 * Parameters:
 *     unit                - (IN) BCM device number
 *     profile_mem         - (IN) HASH selection table mem
 *     hash_select_profile - (OUT) hash selection profile
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     
 */
int
_bcm_field_td2_hash_select_profile_get(int unit, 
                                       soc_mem_t profile_mem,
                                       soc_profile_mem_t **hash_select_profile)
{
    _field_stage_t *stage_fc;

    
    if ((profile_mem != VFP_HASH_FIELD_BMAP_TABLE_Am) &&
        (profile_mem != VFP_HASH_FIELD_BMAP_TABLE_Bm)) {
        return BCM_E_PARAM; 
    } 
         
    /* Get stage control structure. */
    BCM_IF_ERROR_RETURN
        (_field_stage_control_get(unit, _BCM_FIELD_STAGE_LOOKUP, &stage_fc));

    if (profile_mem == VFP_HASH_FIELD_BMAP_TABLE_Am) {
        *hash_select_profile = &stage_fc->hash_select[0];
    } else {
        *hash_select_profile = &stage_fc->hash_select[1];
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_td2_hash_select_profile_ref_count_get
 * Purpose:
 *     Get hash selection profile entry use count.
 * Parameters:
 *     unit        - (IN) BCM device number.
 *     profile_mem - (IN) HASH selection table mem
 *     index       - (IN) Profile entry index.
 *     ref_count   - (OUT) redirect profile use count.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_td2_hash_select_profile_ref_count_get(int unit,
                                                 soc_mem_t profile_mem, 
                                                 int index, 
                                                 int *ref_count)
{
    soc_profile_mem_t *hash_select_profile;

    if (NULL == ref_count) {
        return (BCM_E_PARAM);
    }

    /* Get the redirect profile */
    BCM_IF_ERROR_RETURN
        (_bcm_field_td2_hash_select_profile_get(unit,  profile_mem, 
                                                &hash_select_profile));

    BCM_IF_ERROR_RETURN(soc_profile_mem_ref_count_get(unit,
                                                      hash_select_profile,
                                                      index, ref_count));
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_td2_hash_select_profile_delete
 * Purpose:
 *     Delete hash select profile entry.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     index     - (IN) Profile entry index.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_td2_hash_select_profile_delete(int unit,
                                          soc_mem_t profile_mem,
                                          int index) 
{
    soc_profile_mem_t *hash_select_profile;

    /* Get the redirect profile */
    BCM_IF_ERROR_RETURN
        (_bcm_field_td2_hash_select_profile_get(unit,  profile_mem, 
                                                &hash_select_profile));

    BCM_IF_ERROR_RETURN
        (soc_profile_mem_delete(unit, hash_select_profile, index));

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_td2_hash_select_profile_alloc
 * Purpose:
 *     Allocate hash selection profile index
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     f_ent    - (IN) Field entry structure to get policy info from.
 *     fa       - (IN) Field action.
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_td2_hash_select_profile_alloc(int unit, _field_entry_t *f_ent,
                                             _field_action_t *fa)
   
{
    vfp_hash_field_bmap_table_a_entry_t entry_arr[2];
    uint32            *entry_ptr[2];
    soc_profile_mem_t *hash_select_profile;
    int               rv;
    void              *entries[1];
    soc_mem_t         profile_mem;

    profile_mem = (fa->action == bcmFieldActionHashSelect0)? 
                                    VFP_HASH_FIELD_BMAP_TABLE_Am:
                                    VFP_HASH_FIELD_BMAP_TABLE_Bm ;

    entry_ptr[0] = (uint32 *)entry_arr;
    entry_ptr[1] =  entry_ptr[0] + soc_mem_entry_words(unit, profile_mem);
    entries[0] = (void *)&entry_arr;

    if ((NULL == f_ent) || (NULL == fa)) {
        return (BCM_E_PARAM);
    }

    /* Reset hash select profile entry. */
    sal_memcpy(entry_ptr[0], soc_mem_entry_null(unit, profile_mem),
               soc_mem_entry_words(unit, profile_mem) * sizeof(uint32));
    sal_memcpy(entry_ptr[1], soc_mem_entry_null(unit, profile_mem),
               soc_mem_entry_words(unit, profile_mem) * sizeof(uint32));
 
    /* Get the hash_select profile */
    rv = _bcm_field_td2_hash_select_profile_get(unit,  profile_mem, 
                                                &hash_select_profile);
    BCM_IF_ERROR_RETURN(rv);

    switch (fa->action) {
      case bcmFieldActionHashSelect0:
      case bcmFieldActionHashSelect1:
          soc_mem_field_set(unit, profile_mem, entry_ptr[0], 
                            BITMAPf, &fa->param[0]);
          rv = soc_profile_mem_add(unit, hash_select_profile, entries,
                                   1, (uint32*) &fa->hw_index);
                           
          BCM_IF_ERROR_RETURN(rv);
          break;
      default:
          return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}
/*
 * Function:
 *     _bcm_field_td2_hash_select_profile_hw_free
 *
 * Purpose:
 *     Free hash selection profile indexes required for entry installation.
 *
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     f_ent     - (IN) Field entry descriptor.
 *     flags     - (IN) Free flags (old/new/both). 
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
int
_bcm_field_td2_hash_select_profile_hw_free(int unit, 
                                           _field_entry_t *f_ent, 
                                           uint32 flags)
{
    _field_action_t *fa;  /* Field action descriptor. */
    int rv = BCM_E_NONE;  /* Operation return status. */
    soc_mem_t mem; /* hash selection memory */

    /* Applicable to stage lookup on TRIDENT2 devices. */
    if ((0 == SOC_IS_TRIDENT2(unit)) ||
        (_BCM_FIELD_STAGE_LOOKUP != f_ent->group->stage_id)) {
        return (BCM_E_NONE);
    }

    /* Extract the policy info from the entry structure. */
    for (fa = f_ent->actions; fa != NULL; fa = fa->next) {
        switch (fa->action) {
          case bcmFieldActionHashSelect0:
          case bcmFieldActionHashSelect1:
              mem = ((fa->action == bcmFieldActionHashSelect0)?   
                     VFP_HASH_FIELD_BMAP_TABLE_Am: 
                     VFP_HASH_FIELD_BMAP_TABLE_Bm);

              if ((flags & _FP_ACTION_RESOURCE_FREE) && 
                  (_FP_INVALID_INDEX != fa->hw_index)) {
                  rv = _bcm_field_td2_hash_select_profile_delete(unit, 
                                                                 mem,
                                                                 fa->hw_index);
                  BCM_IF_ERROR_RETURN(rv);
                  fa->hw_index = _FP_INVALID_INDEX;
              } 

              if ((flags & _FP_ACTION_OLD_RESOURCE_FREE) && 
                  (_FP_INVALID_INDEX != fa->old_index)) {
                  rv = _bcm_field_td2_hash_select_profile_delete(unit,
                                                                 mem,
                                                                 fa->old_index);
                  BCM_IF_ERROR_RETURN(rv);
                  fa->old_index = _FP_INVALID_INDEX;
              }
              break;
          default:
              break;
        }
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_field_td2_hash_select_profile_hw_alloc
 *
 * Purpose:
 *     Allocate redirect profile index required for entry installation.
 *
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     f_ent     - (IN) Field entry descriptor.
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
int
_bcm_field_td2_hash_select_profile_hw_alloc (int unit, _field_entry_t *f_ent)
{
    int rv = BCM_E_NONE;  /* Operation return status.     */
    _field_action_t *fa;  /* Field action descriptor.     */
    int ref_count;        /* Profile use reference count. */
    soc_mem_t mem;        /* Profile mem. */

    /* Applicable to stage lookup on TRIDENT2 devices only. */
    if ((0 == SOC_IS_TRIDENT2(unit)) || 
        (_BCM_FIELD_STAGE_LOOKUP != f_ent->group->stage_id)) { 
        return (BCM_E_NONE);
    }

    /* Extract the policy info from the entry structure. */
    for (fa = f_ent->actions; 
        ((fa != NULL) && (_FP_ACTION_VALID & fa->flags)); 
        fa = fa->next) {
        switch (fa->action) {
          case bcmFieldActionHashSelect0:
          case bcmFieldActionHashSelect1:
              mem = ((fa->action == bcmFieldActionHashSelect0)?   
                     VFP_HASH_FIELD_BMAP_TABLE_Am: 
                     VFP_HASH_FIELD_BMAP_TABLE_Bm);

              /*
               * Store previous hardware index value in old_index.
               */ 
              fa->old_index = fa->hw_index;

              rv = _bcm_field_td2_hash_select_profile_alloc(unit, f_ent, fa);
              if ((BCM_E_RESOURCE == rv) && 
                  (_FP_INVALID_INDEX != fa->old_index)) {
                  /* Destroy old profile ONLY 
                   * if it is not used by other entries.
                   */
                  rv = _bcm_field_td2_hash_select_profile_ref_count_get(unit,
                                                                 mem,  
                                                                 fa->old_index,
                                                                 &ref_count);
                  BCM_IF_ERROR_RETURN(rv);
                  if (ref_count > 1) {
                      return (BCM_E_RESOURCE);
                  }
                  rv = _bcm_field_td2_hash_select_profile_delete(unit,
                                                                 mem, 
                                                                 fa->old_index);
                  BCM_IF_ERROR_RETURN(rv);

                  /* Destroy old profile is no longer required. */
                  fa->old_index = _FP_INVALID_INDEX;

                  /* Reallocate profile for new action. */
                  rv = _bcm_field_td2_hash_select_profile_alloc(unit, 
                                                                f_ent, 
                                                                fa);
              } 
              break;
          default:
              break;
        }
        if (BCM_FAILURE(rv)) {
            _bcm_field_td2_hash_select_profile_hw_free(unit, f_ent, _FP_ACTION_HW_FREE);
            break;
        }
    }

    return (rv);
}

/*
 * Function:
 *     _field_td2_action_hash_select
 * Purpose:
 *     Install hash selection action in policy table.
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
_field_td2_action_hash_select(int unit, soc_mem_t mem, _field_entry_t *f_ent,
                                  _field_action_t *fa, uint32 *buf)
{
    soc_field_t   hash_select_field = INVALIDf;

    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    switch (fa->action) {
    case bcmFieldActionHashSelect0:
        hash_select_field = HASH_FIELD_BITMAP_PTR_Af; 
        break; 
         
    case bcmFieldActionHashSelect1:
        hash_select_field = HASH_FIELD_BITMAP_PTR_Bf; 
        break;
    default:
        return (BCM_E_PARAM);
    }

    PolicySet(unit, mem, buf, hash_select_field, fa->hw_index);
    return (BCM_E_NONE);
}

STATIC int
_field_td2_stage_action_support_check(int                unit,
                                      unsigned           stage,
                                      bcm_field_action_t action,
                                      int                *result
                                      )
{
    switch (action) {
        case bcmFieldActionFibreChanSrcBindEnable:
        case bcmFieldActionFibreChanFpmaPrefixCheckEnable:
        case bcmFieldActionFibreChanZoneCheckEnable:
        case bcmFieldActionHashSelect0:
        case bcmFieldActionHashSelect1:
            *result = (stage == _BCM_FIELD_STAGE_LOOKUP);
            return (BCM_E_NONE);
            break;
        case bcmFieldActionFibreChanVsanId:
            *result = (stage == _BCM_FIELD_STAGE_INGRESS || _BCM_FIELD_STAGE_LOOKUP);
            return (BCM_E_NONE);
            break;
        case bcmFieldActionFibreChanZoneCheckActionCancel:
        case bcmFieldActionFibreChanIntVsanPri:
        case bcmFieldActionNewClassId:
        case bcmFieldActionEgressClassSelect:
        case bcmFieldActionHiGigClassSelect:
        case bcmFieldActionIngSampleEnable: 
        case bcmFieldActionEgrSampleEnable:
        case bcmFieldActionTrunkLoadBalanceCancel:
            *result = (stage == _BCM_FIELD_STAGE_INGRESS);
            return (BCM_E_NONE);
            break;
        case bcmFieldActionTrunkResilientHashCancel:
        case bcmFieldActionHgTrunkResilientHashCancel:
        case bcmFieldActionEcmpResilientHashCancel:
            *result = (stage == _BCM_FIELD_STAGE_INGRESS);
            return (BCM_E_NONE);
            break;
        default:
        ;
    }

    return (_bcm_field_trx_stage_action_support_check(unit, stage, action, result));
}


STATIC int
_field_td2_action_support_check(int                unit,
                                _field_entry_t     *f_ent,
                                bcm_field_action_t action,
                                int                *result
                                )
{
    return (_field_td2_stage_action_support_check(unit,
                                                  f_ent->group->stage_id,
                                                  action,
                                                  result
                                                  )
            );
}

STATIC int
_field_td2_action_params_check(int             unit,
                               _field_entry_t  *f_ent,
                               _field_action_t *fa
                               )
{
    soc_mem_t mem;              /* Policy table memory id. */
    soc_mem_t tcam_mem;         /* Tcam memory id.         */

    if (_BCM_FIELD_STAGE_EXTERNAL != f_ent->group->stage_id) {

        BCM_IF_ERROR_RETURN(_field_fb_tcam_policy_mem_get(unit,
            f_ent->group->stage_id, &tcam_mem, &mem));

        switch (fa->action) {
            case bcmFieldActionFibreChanSrcBindEnable:
                PolicyCheck(unit, mem, FCOE_SRC_BIND_CHECK_ENABLEf, fa->param[0]);
                return (BCM_E_NONE);
            case bcmFieldActionFibreChanFpmaPrefixCheckEnable:
                PolicyCheck(unit, mem, FCOE_SRC_FPMA_PREFIX_CHECK_ENABLEf, fa->param[0]);
                return (BCM_E_NONE);
            case bcmFieldActionFibreChanZoneCheckEnable:
                PolicyCheck(unit, mem, FCOE_ZONE_CHECK_ENABLEf, fa->param[0]);
                return (BCM_E_NONE);
            case bcmFieldActionFibreChanVsanId:
                PolicyCheck(unit, mem, FCOE_VSAN_IDf, fa->param[0]);
                return (BCM_E_NONE);
            case bcmFieldActionFibreChanZoneCheckActionCancel:
                PolicyCheck(unit, mem, FCOE_ZONE_CHECK_ACTIONf, fa->param[0]);
                return (BCM_E_NONE);
            case bcmFieldActionNewClassId:
                PolicyCheck(unit, mem, I2E_CLASSIDf, fa->param[0]);
                return (BCM_E_NONE);
            case bcmFieldActionEgressClassSelect:
                return (fa->param[0] <= BCM_FIELD_EGRESS_CLASS_SELECT_NEW
                    ? BCM_E_NONE : BCM_E_PARAM
                    );
            case bcmFieldActionHiGigClassSelect:
                return (fa->param[0] <= BCM_FIELD_HIGIG_CLASS_SELECT_PORT
                    ? BCM_E_NONE : BCM_E_PARAM
                    );
            case bcmFieldActionFibreChanIntVsanPri:
                PolicyCheck(unit, mem, FCOE_VSAN_PRIf, fa->param[0]);
                return (BCM_E_NONE);
            case bcmFieldActionHashSelect0:
            case bcmFieldActionHashSelect1:
                if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
                    mem = (fa->action == bcmFieldActionHashSelect0)? 
                                          VFP_HASH_FIELD_BMAP_TABLE_Am:
                                          VFP_HASH_FIELD_BMAP_TABLE_Bm;

                    PolicyCheck(unit, mem, BITMAPf, fa->param[0]);
                    return (BCM_E_NONE);
                }
                break;
            default:
            ;
        }
    }

    return (_bcm_field_trx_action_params_check(unit, f_ent, fa));
}
 
STATIC int
_field_td2_ingress_qual_tcam_key_mask_get(int unit,
                                          _field_entry_t *f_ent,
                                          _field_tcam_t *tcam
                                          )
{
    int errcode = BCM_E_INTERNAL;
    soc_mem_t mem;
    const unsigned fp_tcam_words
        = BITS2WORDS(soc_mem_field_length(unit,
                                          FP_TCAMm,
                                          KEYf
                                          )
                     );

    tcam->key_size
        = WORDS2BYTES(fp_tcam_words
                      + BITS2WORDS(soc_mem_field_length(
                                       unit,
                                       FP_GLOBAL_MASK_TCAMm,
                                       KEYf
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

        sal_memset(tcam_entry, 0, sizeof(tcam_entry));

        errcode = _bcm_field_entry_tcam_idx_get(unit, f_ent, &tcam_idx);
        if (BCM_FAILURE(errcode)) {
            goto error;
        }

        errcode = soc_mem_read(unit,
                               FP_TCAMm,
                               MEM_BLOCK_ANY,
                               tcam_idx,
                               tcam_entry
                               );
        if (BCM_FAILURE(errcode)) {
            goto error;
        }
        soc_mem_field_get(unit, FP_TCAMm, tcam_entry, KEYf, tcam->key);
        soc_mem_field_get(unit, FP_TCAMm, tcam_entry, MASKf, tcam->mask);
        
        if (f_ent->flags & _FP_ENTRY_SECONDARY) {
            mem = FP_GM_FIELDSm;
        } else {
            mem = FP_GLOBAL_MASK_TCAMm;
        }

        errcode = soc_mem_read(unit,
                               mem,
                               MEM_BLOCK_ANY,
                               tcam_idx,
                               tcam_entry
                               );
        if (BCM_FAILURE(errcode)) {
            goto error;
        }

        soc_mem_field_get(unit,
                          mem,
                          tcam_entry,
                          KEYf,
                          tcam->key + fp_tcam_words
                          );
        soc_mem_field_get(unit,
                          mem,
                          tcam_entry,
                          MASKf,
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
_bcm_field_td2_qual_tcam_key_mask_get(int unit,
                                      _field_entry_t *f_ent,
                                      _field_tcam_t *tcam
                                      )
{
    switch (f_ent->group->stage_id) {
    case _BCM_FIELD_STAGE_INGRESS:
        return (_field_td2_ingress_qual_tcam_key_mask_get(unit, f_ent, tcam));
    default:
        ;
    }

    return (_field_qual_tcam_key_mask_get(unit, f_ent, tcam, 0));
}

STATIC int
_field_td2_ingress_qual_tcam_key_mask_set(int            unit,
                                          _field_entry_t *f_ent,
                                          unsigned       tcam_idx,
                                          unsigned       validf
                                          )
{
    uint32  e[SOC_MAX_MEM_FIELD_WORDS] = {0}; /* Buffer to fill Policy & TCAM entry.*/
    _field_tcam_t  * const tcam = &f_ent->tcam;
    _field_group_t * const fg = f_ent->group;
    const unsigned fp_tcam_words
        = BITS2WORDS(soc_mem_field_length(unit,
                                          FP_TCAMm,
                                          KEYf
                                          )
                     );
    fp_global_mask_tcam_entry_t tcam_entry;
    fp_global_mask_tcam_x_entry_t tcam_entry_x;
    fp_global_mask_tcam_y_entry_t tcam_entry_y;
    bcm_pbmp_t pbmp_x;
    bcm_pbmp_t pbmp_y;

    sal_memset(&tcam_entry, 0, sizeof(fp_global_mask_tcam_x_entry_t));

    BCM_IF_ERROR_RETURN(soc_mem_read(unit,
                                     FP_TCAMm,
                                     MEM_BLOCK_ANY,
                                     tcam_idx,
                                     e
                                     )
                        );
    soc_mem_field_set(unit, FP_TCAMm, e, KEYf, tcam->key);
    soc_mem_field_set(unit, FP_TCAMm, e, MASKf, tcam->mask);
    soc_mem_field32_set(unit,
                        FP_TCAMm,
                        e,
                        VALIDf,
                        validf
                        ? ((fg->flags & _FP_GROUP_LOOKUP_ENABLED)
                           ? 3 : 2
                           )
                        : 0
                        );
    BCM_IF_ERROR_RETURN(soc_mem_write(unit,
                                      FP_TCAMm,
                                      MEM_BLOCK_ALL,
                                      tcam_idx,
                                      e
                                      )
                        );

    if (f_ent->flags & _FP_ENTRY_SECONDARY) {
        FP_VVERB(("Overlay in use\n"));

        sal_memset(e, 0, SOC_MAX_MEM_FIELD_WORDS);
        BCM_IF_ERROR_RETURN(soc_mem_read(unit,
                                         FP_GM_FIELDSm,
                                         MEM_BLOCK_ANY,
                                         tcam_idx,
                                         e
                                         )
                            );

        soc_mem_field32_set(unit, FP_GM_FIELDSm, e, VALIDf, validf ? 3 : 2);
        soc_mem_field_width_fit_set(unit, FP_GM_FIELDSm, e, KEYf, tcam->key + fp_tcam_words);
        soc_mem_field_width_fit_set(unit, FP_GM_FIELDSm, e, MASKf, tcam->mask + fp_tcam_words);

        BCM_IF_ERROR_RETURN(WRITE_FP_GM_FIELDSm(unit, MEM_BLOCK_ALL, tcam_idx,
            e));
    } else {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit,
                                         FP_GLOBAL_MASK_TCAMm,
                                         MEM_BLOCK_ANY,
                                         tcam_idx,
                                         &tcam_entry
                                         )
                            );

        soc_mem_field_set(unit,
                          FP_GLOBAL_MASK_TCAMm,
                          (uint32 *)&tcam_entry,
                          KEYf,
                          tcam->key + fp_tcam_words
                          );
        soc_mem_field_set(unit,
                          FP_GLOBAL_MASK_TCAMm,
                          (uint32 *)&tcam_entry,
                          MASKf,
                          tcam->mask + fp_tcam_words
                          );

        if (BCM_FIELD_QSET_TEST(f_ent->group->qset, bcmFieldQualifyInPort) ||
            BCM_FIELD_QSET_TEST(f_ent->group->qset, bcmFieldQualifyInPorts)) {
            soc_mem_field_set(unit, FP_GLOBAL_MASK_TCAMm, (uint32 *)&tcam_entry,
                              IPBMf, (uint32 *)&(f_ent->pbmp.data));
            soc_mem_field_width_fit_set(unit, FP_GLOBAL_MASK_TCAMm,
                                        (uint32 *)&tcam_entry, IPBM_MASKf,
                                        (uint32 *)&(f_ent->pbmp.mask));

        }
        else {
            BCM_PBMP_CLEAR(pbmp_x);
            BCM_PBMP_CLEAR(pbmp_y);
            sal_memset(&tcam_entry_x, 0, sizeof(fp_global_mask_tcam_x_entry_t));
            sal_memset(&tcam_entry_y, 0, sizeof(fp_global_mask_tcam_y_entry_t));

            BCM_IF_ERROR_RETURN(soc_mem_read(unit,
                                            FP_GLOBAL_MASK_TCAM_Xm,
                                            MEM_BLOCK_ANY,
                                            tcam_idx,
                                            &tcam_entry_x
                                            )
                                );
            BCM_IF_ERROR_RETURN(soc_mem_read(unit,
                                            FP_GLOBAL_MASK_TCAM_Ym,
                                            MEM_BLOCK_ANY,
                                            tcam_idx,
                                            &tcam_entry_y
                                            )
                                );

            soc_mem_pbmp_field_get(unit, FP_GLOBAL_MASK_TCAM_Xm, &tcam_entry_x,
                                   IPBMf, &pbmp_x);
            soc_mem_pbmp_field_get(unit, FP_GLOBAL_MASK_TCAM_Ym, &tcam_entry_y,
                                   IPBMf, &pbmp_y);

            BCM_PBMP_OR(pbmp_x, pbmp_y);

            soc_mem_pbmp_field_set(unit,
                                   FP_GLOBAL_MASK_TCAMm,
                                   &tcam_entry,
                                   IPBMf,
                                   &pbmp_x 
                                   );
        }

        if (!(f_ent->flags & _FP_ENTRY_SECOND_HALF)) {
            soc_mem_field_set(unit,
                              FP_GLOBAL_MASK_TCAMm,
                              (uint32 *)&tcam_entry,
                              IPBMf,
                              f_ent->pbmp.data.pbits
                              );
            soc_mem_field_set(unit,
                              FP_GLOBAL_MASK_TCAMm,
                              (uint32 *)&tcam_entry,
                              IPBM_MASKf,
                              f_ent->pbmp.mask.pbits
                              );
        }
        soc_mem_field32_set(unit,
                            FP_GLOBAL_MASK_TCAMm,
                            (uint32 *)&tcam_entry,
                            VALIDf,
                            validf ? 3 : 2
                            );
        BCM_IF_ERROR_RETURN(soc_mem_write(unit,
                                          FP_GLOBAL_MASK_TCAMm,
                                          MEM_BLOCK_ALL,
                                          tcam_idx,
                                          &tcam_entry
                                          )
                            );

    }
    return (BCM_E_NONE);
}

int
_bcm_field_td2_qual_tcam_key_mask_set(int            unit,
                                      _field_entry_t *f_ent,
                                      unsigned       validf
                                      )
{
    int       tcam_idx;
    soc_mem_t tcam_mem;

    BCM_IF_ERROR_RETURN(_bcm_field_entry_tcam_idx_get(unit,
                                                      f_ent,
                                                      &tcam_idx
                                                      )
                        );

    switch (f_ent->group->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            tcam_mem = VFP_TCAMm;
            break;
        case _BCM_FIELD_STAGE_INGRESS:
            return (_field_td2_ingress_qual_tcam_key_mask_set(unit,
                                                              f_ent,
                                                              tcam_idx,
                                                              validf));
        case _BCM_FIELD_STAGE_EGRESS:
            tcam_mem = EFP_TCAMm;
            break;
        default:
            return (BCM_E_INTERNAL);
    }
    {
        uint32 tcam_entry[SOC_MAX_MEM_FIELD_WORDS] = {0};

        BCM_IF_ERROR_RETURN(_bcm_field_trx_tcam_get(unit, tcam_mem,
                                                    f_ent, tcam_entry));
        BCM_IF_ERROR_RETURN(soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL,
                                          tcam_idx, tcam_entry));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_td2_action_get
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
_bcm_field_td2_action_get(int             unit,
                          soc_mem_t       mem,
                          _field_entry_t  *f_ent,
                          int             tcam_idx,
                          _field_action_t *fa,
                          uint32          *buf
                          )
{
#ifdef INCLUDE_L3
    int32     hw_idx=0, hw_half=0;
#endif /* INCLUDE_L3 */
 
    if (NULL == f_ent || NULL == fa || NULL == buf) {
        return (BCM_E_PARAM);
    }

    switch (fa->action) {
        case bcmFieldActionFibreChanSrcBindEnable:
            PolicySet(unit, mem, buf, FCOE_SRC_BIND_CHECK_ENABLEf, fa->param[0]);
            break;
        case bcmFieldActionFibreChanFpmaPrefixCheckEnable:
            PolicySet(unit, mem, buf, FCOE_SRC_FPMA_PREFIX_CHECK_ENABLEf, fa->param[0]);
            break;
        case bcmFieldActionFibreChanZoneCheckEnable:
            PolicySet(unit, mem, buf, FCOE_ZONE_CHECK_ENABLEf, fa->param[0]);
            break;
        case bcmFieldActionFibreChanVsanId:
            PolicySet(unit, mem, buf, FCOE_VSAN_IDf, fa->param[0]);
            break;
        case bcmFieldActionFibreChanZoneCheckActionCancel:
            PolicySet(unit, mem, buf, FCOE_ZONE_CHECK_ACTIONf, fa->param[0]);
            break;
        case bcmFieldActionFibreChanIntVsanPri:
            PolicySet(unit, mem, buf, FCOE_VSAN_PRIf, fa->param[0]);
            PolicySet(unit, mem, buf, FCOE_VSAN_PRI_VALIDf, 1);
            break;
        case bcmFieldActionNewClassId:
            PolicySet(unit, mem, buf, I2E_CLASSIDf, fa->param[0]);
            PolicySet(unit, mem, buf, G_L3SW_CHANGE_L2_FIELDSf, 0x8);
            break;

        case bcmFieldActionEgressClassSelect:
            {
                unsigned i2e_cl_sel;

                switch (fa->param[0]) {
                    case BCM_FIELD_EGRESS_CLASS_SELECT_PORT:
                        i2e_cl_sel = 0x1;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_SVP:
                        i2e_cl_sel = 0x2;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_L3_IIF:
                        i2e_cl_sel = 0x3;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_FIELD_SRC:
                        i2e_cl_sel = 0x4; /* VFP hi */
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_FIELD_DST:
                        i2e_cl_sel = 0x5; /* VFP lo */
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_L2_SRC:
                        i2e_cl_sel = 0x6;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_L2_DST:
                        i2e_cl_sel = 0x7;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_L3_SRC:
                        i2e_cl_sel = 0x8;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_L3_DST:
                        i2e_cl_sel = 0x9;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_VLAN:
                        i2e_cl_sel = 0xa;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_VRF:
                        i2e_cl_sel = 0xb;
                        break;
                    case BCM_FIELD_EGRESS_CLASS_SELECT_NEW:
                        i2e_cl_sel = 0xf;
                        break;
                    default:
                        /* Invalid parameter should have been caught earlier */
                        return (BCM_E_INTERNAL);
                }
                PolicySet(unit, mem, buf, I2E_CLASSID_SELf, i2e_cl_sel);
            }
            break;

        case bcmFieldActionHiGigClassSelect:
            {
                unsigned hg_cl_sel;

                switch (fa->param[0]) {
                    case BCM_FIELD_HIGIG_CLASS_SELECT_EGRESS:
                        hg_cl_sel = 1;
                        break;
                    case BCM_FIELD_HIGIG_CLASS_SELECT_PORT:
                        hg_cl_sel = 4;
                        break;
                    default:
                        /* Invalid parameter should have been caught earlier */
                        return (BCM_E_INTERNAL);
                }

                PolicySet(unit, mem, buf, HG_CLASSID_SELf, hg_cl_sel);
            }
            break;

        case bcmFieldActionNatCancel:
            PolicySet(unit, mem, buf, DO_NOT_NATf, 0x1);
            break;

        case bcmFieldActionNat:
            PolicySet(unit, mem, buf, NAT_ENABLEf, 0x1);
            break;

        case bcmFieldActionNatEgressOverride:
#ifdef INCLUDE_L3
            BCM_L3_NAT_EGRESS_HW_IDX_GET(fa->param[0], hw_idx, hw_half);
            if ((hw_idx < 0) || (hw_idx > soc_mem_index_max(unit,
                                          EGR_NAT_PACKET_EDIT_INFOm))) {
               return BCM_E_PARAM;
            }

            PolicySet(unit, mem, buf, NAT_PACKET_EDIT_IDXf, hw_idx);
            PolicySet(unit, mem, buf, NAT_PACKET_EDIT_ENTRY_SELf, hw_half);    
#else
            return (BCM_E_UNAVAIL);   
#endif /* INCLUDE_L3 */ 
            break;

        case bcmFieldActionIngSampleEnable: 
             PolicySet(unit, mem, buf, SFLOW_ING_SAMPLEf, 0x1);
             break;

        case bcmFieldActionEgrSampleEnable: 
             PolicySet(unit, mem, buf, SFLOW_EGR_SAMPLEf, 0x1);
             break;

        case bcmFieldActionHashSelect0:
        case bcmFieldActionHashSelect1:
            /* hash selection available only in lookup stage */
            if (_BCM_FIELD_STAGE_LOOKUP == f_ent->group->stage_id) {
                _field_td2_action_hash_select(unit, mem, f_ent, fa, buf);
            }
            break;

        case bcmFieldActionTrunkLoadBalanceCancel:
             PolicySet(unit, mem, buf, LAG_RH_DISABLEf, 1);
             PolicySet(unit, mem, buf, HGT_RH_DISABLEf, 1);
             PolicySet(unit, mem, buf, HGT_DLB_DISABLEf, 1);          
             break;

        case bcmFieldActionEcmpResilientHashCancel:
             PolicySet(unit, mem, buf, ECMP_RH_DISABLEf, 1);
             break;

        case bcmFieldActionHgTrunkResilientHashCancel:
             PolicySet(unit, mem, buf, HGT_RH_DISABLEf, 1);
             break;

        case bcmFieldActionTrunkResilientHashCancel:
             PolicySet(unit, mem, buf, LAG_RH_DISABLEf, 1);
             break;

        default:
            return (BCM_E_UNAVAIL);
    }

    return (BCM_E_NONE);
}


/*
 * Function:
 *     _field_td2_ingress_write_slice_map
 * Purpose:
 *     Write FP_SLICE_MAP
 * Parameters:
 *     unit       - (IN) BCM device number
 *     stage_fc   - (IN) stage control structure
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_field_td2_ingress_write_slice_map(int unit, _field_stage_t *stage_fc)
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
        VIRTUAL_SLICE_11_PHYSICAL_SLICE_NUMBER_ENTRY_0f
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
        VIRTUAL_SLICE_11_VIRTUAL_SLICE_GROUP_ENTRY_0f
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
 *     _bcm_field_td2_write_slice_map
 * Purpose:
 *     Write FP_SLICE_MAP, EFP_SLICE_MAP, VFP_SLICE_MAP
 * Parameters:
 *     unit       - (IN) BCM device number
 *     stage_fc   - (IN) stage control structure
 * Returns:
 *     BCM_E_XXX
 */

int
_bcm_field_td2_write_slice_map(int unit, _field_stage_t *stage_fc)
{
    switch (stage_fc->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            return (_bcm_field_trx_write_slice_map_vfp(unit, stage_fc));

        case _BCM_FIELD_STAGE_INGRESS:
            return (_field_td2_ingress_write_slice_map(unit, stage_fc));

        case _BCM_FIELD_STAGE_EGRESS:
            return (_bcm_field_trx_write_slice_map_egress(unit, stage_fc));

        default:
            ;
    }

    return (BCM_E_INTERNAL);
}

/*
 * Function:
 *     _field_td2_functions_init
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
_field_td2_functions_init(_field_funct_t *functions)
{
    functions->fp_detach               = _bcm_field_tr_detach;
    functions->fp_group_install        = _field_td2_group_install;
    functions->fp_selcodes_install     = _field_td2_selcodes_install;
    functions->fp_slice_clear          = _field_td2_slice_clear;
    functions->fp_entry_remove         = _bcm_field_fb_entry_remove;
    functions->fp_entry_move           = _field_td2_entry_move;
    functions->fp_selcode_get          = _bcm_field_tr_selcode_get;
    functions->fp_selcode_to_qset      = _bcm_field_selcode_to_qset;
    functions->fp_qual_list_get        = _bcm_field_qual_lists_get;
    functions->fp_tcam_policy_clear    = NULL;
    functions->fp_tcam_policy_install  = _bcm_field_tr_entry_install;
    functions->fp_tcam_policy_reinstall = _bcm_field_tr_entry_reinstall;
    functions->fp_policer_install      = _bcm_field_trx_policer_install;
    functions->fp_write_slice_map      = _bcm_field_td2_write_slice_map;
    functions->fp_qualify_ip_type      = _bcm_field_trx_qualify_ip_type;
    functions->fp_qualify_ip_type_get  = _bcm_field_trx_qualify_ip_type_get;
    functions->fp_action_support_check = _field_td2_action_support_check;
    functions->fp_action_conflict_check = _bcm_field_trx_action_conflict_check;
    functions->fp_counter_get          = _bcm_field_td_counter_get;
    functions->fp_counter_set          = _bcm_field_td_counter_set;
    functions->fp_stat_index_get       = _bcm_field_trx_stat_index_get;
    functions->fp_action_params_check  = _field_td2_action_params_check;
    functions->fp_action_depends_check = _bcm_field_trx_action_depends_check;
    functions->fp_egress_key_match_type_set
        = _bcm_field_trx_egress_key_match_type_set;
    functions->fp_external_entry_install  = NULL;
    functions->fp_external_entry_reinstall  = NULL;
    functions->fp_external_entry_remove   = NULL;
    functions->fp_external_entry_prio_set = NULL;
    functions->fp_data_qualifier_ethertype_add
        = _bcm_field_trx2_data_qualifier_ethertype_add;
    functions->fp_data_qualifier_ethertype_delete
        = _bcm_field_trx2_data_qualifier_ethertype_delete;
    functions->fp_data_qualifier_ip_protocol_add
        = _bcm_field_trx2_data_qualifier_ip_protocol_add;
    functions->fp_data_qualifier_ip_protocol_delete
        = _bcm_field_trx2_data_qualifier_ip_protocol_delete;
    functions->fp_data_qualifier_packet_format_add
        = _bcm_field_trx2_data_qualifier_packet_format_add;
    functions->fp_data_qualifier_packet_format_delete
        = _bcm_field_trx2_data_qualifier_packet_format_delete;
}

/*
 * Function:
 *     _field_td2_qualifiers_init
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
_field_td2_qualifiers_init(int unit, _field_stage_t *stage_fc)
{
    /* Allocated stage qualifiers configuration array. */
    _FP_XGS3_ALLOC(stage_fc->f_qual_arr,
                   (_bcmFieldQualifyCount * sizeof(_bcm_field_qual_info_t *)),
                   "Field qualifiers");
    if (stage_fc->f_qual_arr == 0) {
        return (BCM_E_MEMORY);
    }

    switch (stage_fc->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            return (_field_td2_lookup_qualifiers_init(unit, stage_fc));
        case _BCM_FIELD_STAGE_INGRESS:
            return (_field_td2_ingress_qualifiers_init(unit, stage_fc));
        case _BCM_FIELD_STAGE_EGRESS:
            return (_field_td2_egress_qualifiers_init(unit, stage_fc));
        default:
            ;
    }

    sal_free(stage_fc->f_qual_arr);
    return (BCM_E_INTERNAL);
}

int
_bcm_field_td2_stage_init(int unit, _field_stage_t *stage_fc)
{
    switch (stage_fc->stage_id) {
        case _BCM_FIELD_STAGE_LOOKUP:
            /* Flags */
            stage_fc->flags |= (_FP_STAGE_SLICE_ENABLE
                                | _FP_STAGE_AUTO_EXPANSION
                                | _FP_STAGE_GLOBAL_COUNTERS);
            /* Slice geometry */
            stage_fc->tcam_sz     = soc_mem_index_count(unit, VFP_TCAMm);
            stage_fc->tcam_slices = 4;
            break;
        case _BCM_FIELD_STAGE_INGRESS:
            /* Flags */
            stage_fc->flags
                |= _FP_STAGE_SLICE_ENABLE
                | _FP_STAGE_GLOBAL_METER_POOLS
                | _FP_STAGE_SEPARATE_PACKET_BYTE_COUNTERS
                | _FP_STAGE_AUTO_EXPANSION;
            /* Slice geometry */
            stage_fc->tcam_sz     = soc_mem_index_count(unit, FP_TCAMm);
            stage_fc->tcam_slices = 12;

            if (soc_feature(unit, soc_feature_field_stage_half_slice)) {
                stage_fc->flags |= _FP_STAGE_HALF_SLICE;
            }
            break;
        case _BCM_FIELD_STAGE_EGRESS:
            /* Flags */
            stage_fc->flags |= _FP_STAGE_SLICE_ENABLE
                | _FP_STAGE_GLOBAL_COUNTERS
                | _FP_STAGE_SEPARATE_PACKET_BYTE_COUNTERS
                | _FP_STAGE_AUTO_EXPANSION;
            /* Slice geometry */
            stage_fc->tcam_sz     = soc_mem_index_count(unit, EFP_TCAMm);
            stage_fc->tcam_slices = 4;
            break;
        default:
            return (BCM_E_INTERNAL);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_td2_init
 * Purpose:
 *     Perform initializations that are specific to BCM56850. This
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
_bcm_field_td2_init(int unit, _field_control_t *fc)
{
    _field_stage_t *stage_fc;

    /* Input parameters check. */
    if (NULL == fc) {
        return (BCM_E_PARAM);
    }

    stage_fc = fc->stages;
    while (stage_fc) {

        if (!SAL_BOOT_BCMSIM && !SAL_BOOT_QUICKTURN) {
            /* Clear hardware table */
            BCM_IF_ERROR_RETURN(_bcm_field_tr_hw_clear(unit, stage_fc));
        }

        /* Initialize qualifiers info. */
        BCM_IF_ERROR_RETURN(_field_td2_qualifiers_init(unit, stage_fc));

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
    _field_td2_functions_init(&fc->functions);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_td2_qualify_class
 * Purpose:
 *     Qualifies field entry classifier id data and mask value
 * Parameters:
 *     unit  - (IN) BCM device number
 *     entry - (IN) Field entry identifier
 *     data  - (IN) Classifier ID lookup key value
 *     mask  - (IN) Classifier ID lookup mask value
 * Returns:
 *     BCM_E_XXXX
 */
int
_bcm_field_td2_qualify_class(int unit,
                             bcm_field_entry_t entry,
                             bcm_field_qualify_t qual,
                             uint32 *data,
                             uint32 *mask
                             )
{
    _field_entry_t *f_ent;
    uint32 cl_width, cl_max;
    uint32 ifp_cl_type;

    /* Get field entry part that contains the qualifier. */
    BCM_IF_ERROR_RETURN(_bcm_field_entry_qual_get(unit, entry, qual, &f_ent));

    switch (f_ent->group->stage_id) {
        case _BCM_FIELD_STAGE_INGRESS:
            switch (qual) {
                case bcmFieldQualifySrcClassL2:
                case bcmFieldQualifySrcClassL3:
                case bcmFieldQualifySrcClassField:
                case bcmFieldQualifyDstClassL2:
                case bcmFieldQualifyDstClassL3:
                case bcmFieldQualifyDstClassField:
                    cl_width = 10;
                    break;
                case bcmFieldQualifyInterfaceClassL2:
                case bcmFieldQualifyInterfaceClassL3:
                case bcmFieldQualifyInterfaceClassPort:
                case bcmFieldQualifyInterfaceClassVPort:  
                    cl_width = 12;
                    break;
                default:
                    return (BCM_E_INTERNAL);
            }

            cl_max = 1 << cl_width;

            if (*data >= cl_max || (*mask != BCM_FIELD_EXACT_MATCH_MASK && *mask >= cl_max)) {
                return (BCM_E_PARAM);
            }
            break;

        case _BCM_FIELD_STAGE_LOOKUP:
            switch (qual) {
                case bcmFieldQualifyInterfaceClassPort:
                    cl_width = 8;
                    break;
                case bcmFieldQualifyInterfaceClassL3:
                    cl_width = 8;
                    break;
                default:
                    return (BCM_E_INTERNAL);
            }

            cl_max   = 1 << cl_width;
            if (*data >= cl_max || (*mask != BCM_FIELD_EXACT_MATCH_MASK && *mask >= cl_max)) {
                return (BCM_E_PARAM);
            }
            break;

        case _BCM_FIELD_STAGE_EGRESS:
            cl_width = 9;
            cl_max   = 1 << cl_width;

            if (*data >= cl_max || (*mask != BCM_FIELD_EXACT_MATCH_MASK && *mask >= cl_max)) {
                return (BCM_E_PARAM);
            }

            /* Need to set IFP_CLASS_TYPE in TCAM (upper 4 bits) */
            switch (qual) {
                case bcmFieldQualifySrcClassL2:
                    ifp_cl_type = 6;
                    break;
                case bcmFieldQualifySrcClassL3:
                    ifp_cl_type = 8;
                    break;
                case bcmFieldQualifySrcClassField:
                    ifp_cl_type = 4;
                    break;
                case bcmFieldQualifyDstClassL2:
                    ifp_cl_type = 7;
                    break;
                case bcmFieldQualifyDstClassL3:
                    ifp_cl_type = 9;
                    break;
                case bcmFieldQualifyDstClassField:
                    ifp_cl_type = 5;
                    break;
                case bcmFieldQualifyInterfaceClassL2:
                    ifp_cl_type = 10;
                    break;
                case bcmFieldQualifyInterfaceClassL3:
                    ifp_cl_type = 3;
                    break;
                case bcmFieldQualifyIngressClassField:
                    ifp_cl_type = 15;
                    break;
                case bcmFieldQualifyIngressInterfaceClassPort:
                    ifp_cl_type = 1;
                    break;
                default:
                    return (BCM_E_INTERNAL);
            }

            *data |= ifp_cl_type << cl_width;
            *mask |= 0xf << cl_width;
            break;

        default:
            return (BCM_E_INTERNAL);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_field_td2_qualify_class_get
 * Purpose:
 *     Retrieve field entry classifier id data and mask values
 * Parameters:
 *     unit  - (IN) BCM device number
 *     entry - (IN) Field entry identifier
 *     qual  - (IN) Field entry qualifier enumeration
 *     data  - (IN/OUT) Classifier ID lookup key value
 *     mask  - (IN/OUT) Classifier ID lookup mask value
 * Returns:
 *     BCM_E_XXXX
 */
int
_bcm_field_td2_qualify_class_get(int unit,
                                 bcm_field_entry_t entry,
                                 bcm_field_qualify_t qual,
                                 uint32 *data,
                                 uint32 *mask
                                 )
{
    _field_entry_t *f_ent;
    const uint32 m = (1 << 9) - 1;

    /* Get field entry part that contains the qualifier. */
    BCM_IF_ERROR_RETURN(_bcm_field_entry_qual_get(unit, entry, qual, &f_ent));

    if (f_ent->group->stage_id == _BCM_FIELD_STAGE_EGRESS) {
        /* Mask off IFP_CLASSID_TYPE */
        *data &= m;
        *mask &= m;
    }

    return (BCM_E_NONE);
}

#ifdef BCM_WARM_BOOT_SUPPORT
STATIC soc_field_t _td2_vfp_slice_wide_mode_field[4] = {
    SLICE_0_DOUBLE_WIDE_MODEf,
    SLICE_1_DOUBLE_WIDE_MODEf,
    SLICE_2_DOUBLE_WIDE_MODEf,
    SLICE_3_DOUBLE_WIDE_MODEf};

/*
 * Function:
 *     _bcm_field_td2_scache_sync
 *
 * Purpose:
 *     Save field module software state to external cache.
 *
 * Parameters:
 *     unit             - (IN) BCM device number
 *     fc               - (IN) Pointer to device field control structure
 *     stage_fc         - (IN) FP stage control info.
 *
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_td2_scache_sync(int              unit,
                           _field_control_t *fc,
                           _field_stage_t   *stage_fc
                           )
{
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1];
    uint32 start_char, end_char;
    uint32 val;
    uint64 rval;
    int slice_idx, range_count = 0;
    int rv = BCM_E_NONE;
    int efp_slice_mode, paired = 0;
    _field_slice_t *fs;
    _field_group_t *fg;
    _field_data_control_t *data_ctrl;
    _field_range_t *fr;
    fp_port_field_sel_entry_t pfs;
    soc_field_t fld;
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
            /*
            if (_field_tr2_ext_scache_sync_chk(unit, fc, stage_fc)) {
                return (_field_tr2_ext_scache_sync(unit, fc, stage_fc));
            } */
            start_char = _FIELD_EXTFP_DATA_START;
            end_char   = _FIELD_EXTFP_DATA_END;
            break;
        default:
            return BCM_E_PARAM;
    }

    FP_VVERB(("FP(unit %d) vverb: _bcm_field_td2_scache_sync() - Synching scache"
             " for FP stage %d...\n", unit, stage_fc->stage_id));

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
        FP_VVERB(("FP(unit %d) vverb: _bcm_field_td2_scache_sync() - "
                 "Checking slice %d...\n", unit, slice_idx));
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
                BCM_IF_ERROR_RETURN(soc_mem_read(unit,
                                                 FP_PORT_FIELD_SELm,
                                                 MEM_BLOCK_ANY,
                                                 0,
                                                 &pfs));
                fld = _fb2_slice_pairing_field[slice_idx / 2];
                paired = soc_FP_PORT_FIELD_SELm_field32_get(unit,
                                                            &pfs,
                                                            fld);
                break;
            case _BCM_FIELD_STAGE_EGRESS:
                BCM_IF_ERROR_RETURN(READ_EFP_SLICE_CONTROLr(unit, &val));
                efp_slice_mode = soc_reg_field_get(unit,
                                     EFP_SLICE_CONTROLr,
                                     val,
                                     _efp_slice_mode[slice_idx]);
                if ((efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE)
                    || (efp_slice_mode
                    == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY)
                    || (efp_slice_mode
                    == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6)) {
                    paired = 1;
                }
                break;
            case _BCM_FIELD_STAGE_LOOKUP:
                BCM_IF_ERROR_RETURN(READ_VFP_KEY_CONTROL_1r(unit, &rval));
                fld = _bcm_field_trx_slice_pairing_field[slice_idx / 2];
                paired = soc_reg64_field32_get(unit,
                            VFP_KEY_CONTROL_1r,
                            rval,
                            fld);
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
            (_field_tr2_group_entry_write(unit,
                slice_idx,
                fs,
                fc,
                stage_fc));
    }

    /* Now sync the expanded slices */
    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
        fs = stage_fc->slices + slice_idx;
        /* Skip empty slices */
        if (fs->entry_count == fs->free_count) {
            FP_VVERB(("FP(unit %d) vverb: _bcm_field_td2_scache_sync() -"
                     " Slice is empty.\n", unit));
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
                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit,
                        FP_PORT_FIELD_SELm,
                        MEM_BLOCK_ANY,
                        0,
                        &pfs));
                fld = _fb2_slice_pairing_field[slice_idx / 2];
                paired = soc_FP_PORT_FIELD_SELm_field32_get(unit,
                            &pfs,
                            fld);
                break;
            case _BCM_FIELD_STAGE_EGRESS:
                BCM_IF_ERROR_RETURN(READ_EFP_SLICE_CONTROLr(unit, &val));
                efp_slice_mode = soc_reg_field_get(unit,
                                    EFP_SLICE_CONTROLr,
                                    val,
                                    _efp_slice_mode[slice_idx]);
                if ((efp_slice_mode == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE)
                    || (efp_slice_mode
                    == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY)
                    || (efp_slice_mode
                    == _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6)) {
                    paired = 1;
                }
                break;
            case _BCM_FIELD_STAGE_LOOKUP:
                BCM_IF_ERROR_RETURN(READ_VFP_KEY_CONTROL_1r(unit, &rval));
                fld = _bcm_field_trx_slice_pairing_field[slice_idx / 2];
                paired = soc_reg64_field32_get(unit,
                            VFP_KEY_CONTROL_1r,
                            rval,
                            fld);
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

    FP_VVERB(("FP(unit %d) vverb: _bcm_field_td2_scache_sync() -"
             " Writing end of section @ byte %d.\n", unit, fc->scache_pos));

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

/*
 * Function:
 *     _bcm_field_td2_stage_lookup_reinit
 *
 * Purpose:
 *     Reconstruct's VFP/Lookup stage groups and entries.
 *
 * Parameters:
 *     unit             - (IN) BCM device number.
 *     fc               - (IN) Device field control structure pointer.
 *     stage_fc         - (IN) FP stage control info.
 *
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_field_td2_stage_lookup_reinit(int unit,
                               _field_control_t *fc,
                               _field_stage_t *stage_fc)
{
    int vslice_idx, max, master_slice;
    int idx, slice_idx, index_min, index_max, ratio, rv = BCM_E_NONE;
    int group_found, mem_sz, parts_count, slice_ent_cnt, expanded[4];
    int i, pri_tcam_idx, part_index, slice_number, prio, prev_prio;
    int paired, intraslice;
    char *vfp_policy_buf = NULL; /* Buffer to read the VFP_POLICY table */
    uint8 *buf = fc->scache_ptr[_FIELD_SCACHE_PART_0];
    uint8 *buf1 = fc->scache_ptr[_FIELD_SCACHE_PART_1];
    uint32 rval, dbl_wide_key, dbl_wide_key_sec;
    uint32 *vfp_tcam_buf = NULL; /* Buffer to read the VFP_TCAM table */
    uint32 vfp_key2, temp;
    uint64 vfp_key_1;
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
    soc_field_t vfp_en_flds[4] = {SLICE_ENABLE_SLICE_0f,
                                  SLICE_ENABLE_SLICE_1f,
                                  SLICE_ENABLE_SLICE_2f,
                                  SLICE_ENABLE_SLICE_3f};

    soc_field_t vfp_lk_en_flds[4] = {LOOKUP_ENABLE_SLICE_0f,
                                     LOOKUP_ENABLE_SLICE_1f,
                                     LOOKUP_ENABLE_SLICE_2f,
                                     LOOKUP_ENABLE_SLICE_3f};

    SOC_PBMP_CLEAR(entry_pbmp);
    sal_memset(expanded, 0, 4 * sizeof(int));

    if (fc->l2warm) {
        rv = _field_scache_stage_hdr_chk(fc, _FIELD_VFP_DATA_START);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }
    }

    /* DMA various tables */
    vfp_tcam_buf = soc_cm_salloc(unit,
                    sizeof(vfp_tcam_entry_t) *
                    soc_mem_index_count(unit, VFP_TCAMm),
                    "VFP TCAM buffer");
    if (NULL == vfp_tcam_buf) {
        return BCM_E_MEMORY;
    }

    sal_memset(vfp_tcam_buf,
               0,
               sizeof(vfp_tcam_entry_t) *
               soc_mem_index_count(unit, VFP_TCAMm)
               );
    index_min = soc_mem_index_min(unit, VFP_TCAMm);
    index_max = soc_mem_index_max(unit, VFP_TCAMm);

    fs = stage_fc->slices;

    if (stage_fc->flags & _FP_STAGE_HALF_SLICE) {
        slice_ent_cnt = fs->entry_count * 2;
        /* DMA in chunks */
        for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {
            fs = stage_fc->slices + slice_idx;
            if ((rv = soc_mem_read_range
                        (unit,
                         VFP_TCAMm, MEM_BLOCK_ALL,
                         slice_idx * slice_ent_cnt,
                         slice_idx * slice_ent_cnt + fs->entry_count / 2 - 1,
                         vfp_tcam_buf + slice_idx * slice_ent_cnt
                         * soc_mem_entry_words(unit, VFP_TCAMm))
                         ) < 0) {
                goto cleanup;
            }

            if ((rv = soc_mem_read_range(unit,
                        VFP_TCAMm, MEM_BLOCK_ALL,
                        slice_idx * slice_ent_cnt + fs->entry_count,
                        slice_idx * slice_ent_cnt
                        + fs->entry_count + fs->entry_count / 2 - 1,
                        vfp_tcam_buf + (slice_idx
                        * slice_ent_cnt + fs->entry_count)
                        * soc_mem_entry_words(unit, VFP_TCAMm))
                        ) < 0 ) {
                goto cleanup;
            }
        }
    } else {
        slice_ent_cnt = fs->entry_count;
        if ((rv = soc_mem_read_range(unit,
                    VFP_TCAMm,
                    MEM_BLOCK_ALL,
                    index_min,
                    index_max,
                    vfp_tcam_buf)
                    ) < 0 ) {
            goto cleanup;
        }
    }

    vfp_policy_buf = soc_cm_salloc(unit,
                        SOC_MEM_TABLE_BYTES(unit, VFP_POLICY_TABLEm),
                        "VFP POLICY TABLE buffer"
                        );
    if (NULL == vfp_policy_buf) {
        return BCM_E_MEMORY;
    }
    index_min = soc_mem_index_min(unit, VFP_POLICY_TABLEm);
    index_max = soc_mem_index_max(unit, VFP_POLICY_TABLEm);
    if ((rv = soc_mem_read_range(unit,
                VFP_POLICY_TABLEm,
                MEM_BLOCK_ALL,
                index_min,
                index_max,
                vfp_policy_buf)
                ) < 0 ) {
        goto cleanup;
    }

    /* Get slice expansion status and virtual map */
    if ((rv = _field_slice_expanded_status_get
                (unit,
                 stage_fc,
                 expanded)
                ) < 0) {
        goto cleanup;
    }

    /* Iterate over the slices */
    if ((rv = READ_VFP_SLICE_CONTROLr(unit, &rval)) < 0) {
        goto cleanup;
    }

    if ((rv = READ_VFP_KEY_CONTROL_1r(unit, &vfp_key_1)) < 0) {
        goto cleanup;
    }

    if ((rv = READ_VFP_KEY_CONTROL_2r(unit, &vfp_key2)) < 0) {
        goto cleanup;
    }

    for (slice_idx = 0; slice_idx < stage_fc->tcam_slices; slice_idx++) {

        /* Ignore disabled slice */
        if ((soc_reg_field_get
                (unit,
                 VFP_SLICE_CONTROLr,
                 rval,
                 vfp_en_flds[slice_idx]
                 ) == 0)
            || (soc_reg_field_get
                    (unit,
                     VFP_SLICE_CONTROLr,
                     rval, vfp_lk_en_flds[slice_idx]
                     ) == 0)) {
            continue;
        }

        /* Ignore secondary slice in paired mode */
        fld = _bcm_field_trx_slice_pairing_field[slice_idx / 2];
        paired = soc_reg64_field32_get
                    (unit,
                     VFP_KEY_CONTROL_1r,
                     vfp_key_1,
                     fld
                     );

        fld = _td2_vfp_slice_wide_mode_field[slice_idx];
        intraslice = soc_reg64_field32_get
                        (unit,
                         VFP_KEY_CONTROL_1r,
                         vfp_key_1,
                         fld
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
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            vfp_tcam_entry = soc_mem_table_idx_to_pointer
                                (unit,
                                 VFP_TCAMm,
                                 vfp_tcam_entry_t *,
                                 vfp_tcam_buf, idx + slice_ent_cnt
                                 * slice_idx
                                 );
            if (soc_VFP_TCAMm_field32_get(unit,
                                          vfp_tcam_entry,
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
            = soc_reg64_field32_get
                (unit,
                 VFP_KEY_CONTROL_1r,
                 vfp_key_1,
                 _bcm_field_trx_vfp_field_sel[slice_idx][0]
                 );
        hw_sels.pri_slice[0].fpf3
            = soc_reg64_field32_get
                (unit,
                 VFP_KEY_CONTROL_1r,
                 vfp_key_1,
                 _bcm_field_trx_vfp_field_sel[slice_idx][1]
                 );

        hw_sels.pri_slice[0].ip_header_sel
            = soc_reg_field_get
                (unit,
                 VFP_KEY_CONTROL_2r,
                 vfp_key2,
                 _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                 );

        /* If intraslice, get double-wide key - only 2 options */
        if (intraslice) {
            dbl_wide_key = soc_reg64_field32_get
                            (unit,
                             VFP_KEY_CONTROL_1r,
                             vfp_key_1,
                             _bcm_field_trx_vfp_double_wide_sel[slice_idx]
                             );
            hw_sels.pri_slice[1].intraslice = TRUE;
            hw_sels.pri_slice[1].fpf2 = dbl_wide_key;
            hw_sels.pri_slice[1].ip_header_sel
                = soc_reg_field_get
                    (unit,
                     VFP_KEY_CONTROL_2r,
                     vfp_key2,
                    _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                    );
        }

        /* If paired, get secondary slice's selectors */
        if (paired) {
            hw_sels.sec_slice[0].fpf1 = 0;
            hw_sels.sec_slice[0].fpf2
                = soc_reg64_field32_get
                    (unit,
                     VFP_KEY_CONTROL_1r,
                     vfp_key_1,
                     _bcm_field_trx_vfp_field_sel[slice_idx + 1][0]
                     );
            hw_sels.sec_slice[0].fpf3
                = soc_reg64_field32_get
                    (unit,
                     VFP_KEY_CONTROL_1r,
                     vfp_key_1,
                     _bcm_field_trx_vfp_field_sel[slice_idx + 1][1]
                     );
            hw_sels.sec_slice[0].ip_header_sel
                = soc_reg_field_get
                    (unit,
                     VFP_KEY_CONTROL_2r, vfp_key2,
                     _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                     );
            /* If in intraslie double wide mode, get DW keysel value. */
            if (intraslice) {
                dbl_wide_key_sec
                    = soc_reg64_field32_get
                        (unit,
                         VFP_KEY_CONTROL_1r,
                         vfp_key_1,
                         _bcm_field_trx_vfp_double_wide_sel[slice_idx]
                         );
                hw_sels.sec_slice[1].intraslice = TRUE;
                hw_sels.sec_slice[1].fpf2 = dbl_wide_key_sec;
                hw_sels.sec_slice[1].ip_header_sel
                    = soc_reg_field_get
                        (unit,
                         VFP_KEY_CONTROL_2r,
                         vfp_key2,
                         _bcm_field_trx_vfp_ip_header_sel[slice_idx]
                         );
            }
        }

        /* Create a group based on HW qualifiers (or find existing) */
        rv = _field_tr2_group_construct
                (unit, &hw_sels, intraslice,
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
                                 (unit,
                                  VFP_TCAMm,
                                  vfp_tcam_entry_t *, vfp_tcam_buf,
                                  idx + slice_ent_cnt * slice_idx
                                  );
            if (soc_VFP_TCAMm_field32_get
                    (unit,
                     vfp_tcam_entry,
                     VALIDf
                     ) == 0) {
                continue;
            }

            /* All ports are applicable to this entry */
            SOC_PBMP_ASSIGN(entry_pbmp, PBMP_PORT_ALL(unit));
            SOC_PBMP_OR(entry_pbmp, PBMP_CMIC(unit));

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
            rv = _bcm_field_entry_tcam_parts_count
                    (unit,
                     fg->flags,
                     &parts_count
                     );
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
                rv = _bcm_field_tcam_part_to_entry_flags
                        (i,
                         fg->flags,
                         &f_ent[i].flags
                         );
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_entry_part_tcam_idx_get
                        (unit,
                         f_ent,
                         pri_tcam_idx,
                         i,
                         &part_index
                         );
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_tcam_idx_to_slice_offset
                        (unit,
                         stage_fc,
                         part_index,
                         &slice_number,
                         (int *)&f_ent[i].slice_idx
                         );
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
                                    (unit,
                                     VFP_POLICY_TABLEm,
                                     vfp_policy_table_entry_t *,
                                     vfp_policy_buf,
                                     part_index
                                     );
                rv = _field_tr2_actions_recover
                        (unit,
                         VFP_POLICY_TABLEm,
                         (uint32 *) vfp_policy_entry,
                         f_ent,
                         i,
                         sid,
                         pid
                         );
                if (soc_feature(unit, soc_feature_advanced_flex_counter)) {
                    _field_adv_flex_stat_info_retrieve(unit, f_ent->statistic.sid);
                }
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
        paired = soc_reg64_field32_get
                    (unit,
                     VFP_KEY_CONTROL_1r,
                     vfp_key_1,
                     fld
                     );
        if (paired && (slice_idx % 2)) {
            continue;
        }
        /* Skip if slice has no valid groups and entries */
        fs = stage_fc->slices + slice_idx;
        for (idx = 0; idx < slice_ent_cnt; idx++) {
            vfp_tcam_entry = soc_mem_table_idx_to_pointer
                                 (unit,
                                  VFP_TCAMm,
                                  vfp_tcam_entry_t *,
                                  vfp_tcam_buf,
                                  idx + slice_ent_cnt * slice_idx
                                  );
            if (soc_VFP_TCAMm_field32_get
                    (unit,
                     vfp_tcam_entry,
                     VALIDf) != 0) {
                break;
            }
        }

        if (idx == slice_ent_cnt && !fc->l2warm) {
            continue;
        }

        /* If Level 2, retrieve the GIDs in this slice */
        if (fc->l2warm) {
            rv = _field_trx_scache_slice_group_recover
                    (unit,
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
                                 (unit,
                                  VFP_TCAMm,
                                  vfp_tcam_entry_t *,
                                  vfp_tcam_buf,
                                  idx + slice_ent_cnt * slice_idx
                                  );
            if (soc_VFP_TCAMm_field32_get
                    (unit,
                     vfp_tcam_entry,
                     VALIDf
                     ) == 0) {
                continue;
            }

            /* Allocate memory for the entry */
            rv = _bcm_field_entry_tcam_parts_count
                    (unit,
                     fg->flags,
                     &parts_count
                     );
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
                rv = _field_trx_entry_info_retrieve
                        (unit,
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
                rv = _bcm_field_tcam_part_to_entry_flags
                        (i,
                         fg->flags,
                         &f_ent[i].flags
                         );
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_entry_part_tcam_idx_get
                        (unit,
                         f_ent,
                         pri_tcam_idx,
                         i,
                         &part_index
                         );
                if (BCM_FAILURE(rv)) {
                    sal_free(f_ent);
                    goto cleanup;
                }
                rv = _bcm_field_tcam_idx_to_slice_offset
                        (unit,
                         stage_fc,
                         part_index,
                        &slice_number,
                        (int *)&f_ent[i].slice_idx
                        );
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
                                    (unit,
                                     VFP_POLICY_TABLEm,
                                     vfp_policy_table_entry_t *,
                                     vfp_policy_buf,
                                     part_index
                                     );
                rv = _field_tr2_actions_recover
                        (unit,
                         VFP_POLICY_TABLEm,
                         (uint32 *) vfp_policy_entry,
                         f_ent,
                         i,
                         sid,
                         pid
                         );
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

    _field_tr2_stage_reinit_all_groups_cleanup
        (unit,
         fc,
         _BCM_FIELD_STAGE_LOOKUP,
         NULL
         );

cleanup:
    if (vfp_tcam_buf) {
        soc_cm_sfree(unit, vfp_tcam_buf);
    }
    if (vfp_policy_buf) {
        soc_cm_sfree(unit, vfp_policy_buf);
    }
    return rv;
}
#endif
#else /* BCM_TRIDENT2_SUPPORT && BCM_FIELD_SUPPORT */
int _td2_field_not_empty;
#endif  /* BCM_TRIDENT2_SUPPORT && BCM_FIELD_SUPPORT */

