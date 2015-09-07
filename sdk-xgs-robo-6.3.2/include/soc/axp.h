/*
 * $Id: axp.h 1.3 Broadcom SDK $
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
 * AXP management routines
 */

#ifndef _SOC_AXP_H
#define _SOC_AXP_H

#include <soc/types.h>
#include <soc/mem.h>

typedef enum soc_axp_nlf_type_e {
    SOC_AXP_NLF_PASSTHRU = 0,
    SOC_AXP_NLF_WLAN_DECAP,
    SOC_AXP_NLF_WLAN_ENCAP,
    SOC_AXP_NLF_SM,         /* Signature match */
    SOC_AXP_NLF_NUM         /* Always last */
} soc_axp_nlf_type_t;

/* Loopback packet type, TR3 internal HW encoding */
typedef enum soc_axp_lpt_e {
    SOC_AXP_LPT_NOP = 0,
    SOC_AXP_LPT_MIM,
    SOC_AXP_LPT_TRILL_NETWORK_PORT,
    SOC_AXP_LPT_TRILL_ACCESS_PORT,
    SOC_AXP_LPT_WLAN_ENCAP,
    SOC_AXP_LPT_WLAN_DECAP,
    SOC_AXP_LPT_DPI_SM,
    SOC_AXP_LPT_MPLS_P2MP,
    SOC_AXP_LPT_IFP_GENERIC,
    SOC_AXP_LPT_MPLS_EXTENDED_LOOKUP,
    SOC_AXP_LPT_L2GRE,
    SOC_AXP_LPT_QCN,
    SOC_AXP_LPT_EP_REDIRECT,
    SOC_AXP_LPT_NUM
} soc_axp_lpt_t;

/* Loopback packet type, TR3 internal */
typedef enum soc_axp_egr_port_prop_e {
    SOC_AXP_EPP_LPORT_0 = 0,
    SOC_AXP_EPP_LPORT_1,
    SOC_AXP_EPP_LPORT_2,
    SOC_AXP_EPP_LPORT_3,
    SOC_AXP_EPP_LPORT_4,
    SOC_AXP_EPP_LPORT_5,
    SOC_AXP_EPP_USE_SGLP,
    SOC_AXP_EPP_USE_SVP,
    SOC_AXP_EPP_NUM
} soc_axp_egr_port_prop_t;

#endif  /* _SOC_AXP_H */

