/*
 * $Id: mirror.c,v 1.1 Broadcom SDK $
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
 */
#include <shared/bsl.h>

#include <soc/drv.h>
#include <soc/debug.h>

/*
 *  Function : drv_harrier_mirror_set
 *
 *  Purpose :
 *      Set ingress and egress ports of mirroring.
 *
 *  Parameters :
 *      unit        :  unit id.
 *      enable      :  enable/disable.
 *      mport_bmp   :  monitor port bitmap.
 *      ingress_bmp :  ingress port bitmap.
 *      egress_bmp  :  egress port bitmap.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_harrier_mirror_set(int unit, uint32 enable, soc_pbmp_t mport_bmp, 
    soc_pbmp_t ingress_bmp, soc_pbmp_t egress_bmp)
{
    uint64  reg_value;
    uint32  temp32;

    LOG_INFO(BSL_LS_SOC_PORT, \
             (BSL_META_U(unit, \
                         "drv_harrier_mirror_set: unit %d, \
                         %sable, mport_bmp = 0x%x, ingress_bmp = 0x%x, egress_bmp = 0x%x\n"),
              unit, (enable) ? "en" : "dis", \
              SOC_PBMP_WORD_GET(mport_bmp, 0), \
              SOC_PBMP_WORD_GET(ingress_bmp, 0), \
              SOC_PBMP_WORD_GET(egress_bmp, 0)));

    /* Check ingress mirror */
    SOC_IF_ERROR_RETURN(REG_READ_IGMIRCTLr
       (unit, (uint32 *)&reg_value));
    /* Write ingress mirror mask */
    temp32 = SOC_PBMP_WORD_GET(ingress_bmp, 0);
    SOC_IF_ERROR_RETURN(soc_IGMIRCTLr_field_set
        (unit, (uint32 *)&reg_value, IN_MIR_MSKf, &temp32));
    SOC_IF_ERROR_RETURN(REG_WRITE_IGMIRCTLr
        (unit, (uint32 *)&reg_value));

    /* Check egress mirror */
    SOC_IF_ERROR_RETURN(REG_READ_EGMIRCTLr
        (unit, (uint32 *)&reg_value));
    /* Write egress mirror mask */
    temp32 = SOC_PBMP_WORD_GET(egress_bmp, 0);
    SOC_IF_ERROR_RETURN(soc_EGMIRCTLr_field_set
        (unit, (uint32 *)&reg_value, OUT_MIR_MSKf, &temp32));
    SOC_IF_ERROR_RETURN(REG_WRITE_EGMIRCTLr
        (unit, (uint32 *)&reg_value));

    /* Check mirror control */
    SOC_IF_ERROR_RETURN(REG_READ_MIRCAPCTLr
        (unit, (uint32 *)&reg_value));

    /* Enable/Disable mirror */
    if (enable) {
        temp32 = 1;
    } else {
        temp32 = 0;

        /* Clear mirror-to ports mask when disable mirror */
        SOC_PBMP_CLEAR(mport_bmp);
    }
    SOC_IF_ERROR_RETURN(soc_MIRCAPCTLr_field_set
        (unit, (uint32 *)&reg_value, MIR_ENf, &temp32));

    /* Write mirror-to ports mask */
    temp32 = SOC_PBMP_WORD_GET(mport_bmp, 0);
    SOC_IF_ERROR_RETURN(soc_MIRCAPCTLr_field_set
        (unit, (uint32 *)&reg_value, SMIR_CAP_PORTf, &temp32));

    SOC_IF_ERROR_RETURN(REG_WRITE_MIRCAPCTLr
        (unit, (uint32 *)&reg_value));

    return SOC_E_NONE;
}

/*
 *  Function : drv_harrier_mirror_get
 *
 *  Purpose :
 *      Get ingress and egress ports of mirroring.
 *
 *  Parameters :
 *      unit        :  unit id.
 *      enable      :  enable/disable.
 *      mport_bmp   :  monitor port bitmap.
 *      ingress_bmp :  ingress port bitmap.
 *      egress_bmp  :  egress port bitmap.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_harrier_mirror_get(int unit, uint32 *enable, soc_pbmp_t *mport_bmp, 
    soc_pbmp_t *ingress_bmp, soc_pbmp_t *egress_bmp)
{
    uint64  reg_value;
    uint32  temp32 = 0;

    /* Ingress mask */
    SOC_IF_ERROR_RETURN(REG_READ_IGMIRCTLr
        (unit, (uint32 *)&reg_value));
    SOC_IF_ERROR_RETURN(soc_IGMIRCTLr_field_get
        (unit, (uint32 *)&reg_value, IN_MIR_MSKf, &temp32));
    SOC_PBMP_WORD_SET(*ingress_bmp, 0, temp32);

    /* Egress mask */
    SOC_IF_ERROR_RETURN(REG_READ_EGMIRCTLr
        (unit, (uint32 *)&reg_value));
    SOC_IF_ERROR_RETURN(soc_EGMIRCTLr_field_get
        (unit, (uint32 *)&reg_value, OUT_MIR_MSKf, &temp32));
    SOC_PBMP_WORD_SET(*egress_bmp, 0, temp32);

    /* Enable value */
    SOC_IF_ERROR_RETURN(REG_READ_MIRCAPCTLr
        (unit, (uint32 *)&reg_value));
    temp32 = 0;
    SOC_IF_ERROR_RETURN(soc_MIRCAPCTLr_field_get
        (unit, (uint32 *)&reg_value, MIR_ENf, &temp32));
    /* Enable/Disable mirror */
    if (temp32) {
        *enable = TRUE;
    } else {
        *enable = FALSE;
    }

    /* Monitor port */
    SOC_PBMP_CLEAR(*mport_bmp);
    SOC_IF_ERROR_RETURN(soc_MIRCAPCTLr_field_get
        (unit, (uint32 *)&reg_value, SMIR_CAP_PORTf, &temp32));
    SOC_PBMP_WORD_SET(*mport_bmp, 0, temp32);

    LOG_INFO(BSL_LS_SOC_PORT, \
             (BSL_META_U(unit, \
                         "drv_harrier_mirror_get : unit %d, \
                         %sable, mport_bmp = 0x%x, ingress_bmp = 0x%x, egress_bmp = 0x%x\n"),
              unit, (*enable) ? "en" : "dis", \
              SOC_PBMP_WORD_GET(*mport_bmp, 0), \
              SOC_PBMP_WORD_GET(*ingress_bmp, 0), \
              SOC_PBMP_WORD_GET(*egress_bmp, 0)));

    return SOC_E_NONE;
}
