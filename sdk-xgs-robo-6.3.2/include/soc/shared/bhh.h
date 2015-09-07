/*
 * $Id: bhh.h 1.11 Broadcom SDK $
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
 * File:    bhh.h
 * Purpose: BHH definitions common to SDK and uKernel
 *
 * Notes:   Definition changes should be avoided in order to
 *          maintain compatibility between SDK and uKernel since
 *          both images are built and loaded separately.
 *
 */

#ifndef _SOC_SHARED_BHH_H
#define _SOC_SHARED_BHH_H

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif



/*****************************************
 * BHH uController Error codes
 */
typedef enum shr_bhh_uc_error_e {
    SHR_BHH_UC_E_NONE = 0,
    SHR_BHH_UC_E_INTERNAL,
    SHR_BHH_UC_E_MEMORY,
    SHR_BHH_UC_E_PARAM,
    SHR_BHH_UC_E_RESOURCE,
    SHR_BHH_UC_E_EXISTS,
    SHR_BHH_UC_E_NOT_FOUND,
    SHR_BHH_UC_E_UNAVAIL,
    SHR_BHH_UC_E_VERSION,
    SHR_BHH_UC_E_INIT
} shr_bhh_uc_error_t;

#define SHR_BHH_UC_SUCCESS(rv)              ((rv) == SHR_BHH_UC_E_NONE)
#define SHR_BHH_UC_FAILURE(rv)              ((rv) != SHR_BHH_UC_E_NONE)

/*
 * Macro:
 *      SHR_BHH_IF_ERROR_RETURN
 * Purpose:
 *      Evaluate _op as an expression, and if an error, return.
 * Notes:
 *      This macro uses a do-while construct to maintain expected
 *      "C" blocking, and evaluates "op" ONLY ONCE so it may be
 *      a function call that has side affects.
 */

#define SHR_BHH_IF_ERROR_RETURN(op)                                     \
    do {                                                                \
        int __rv__;                                                     \
        if ((__rv__ = (op)) != SHR_BHH_UC_E_NONE) {                     \
            return(__rv__);                                             \
        }                                                               \
    } while(0)

/*
 * BHH Session Set flags
 */
#define SHR_BHH_SESS_SET_F_CREATE               0x00000001
#define SHR_BHH_SESS_SET_F_LOCAL_DISC           0x00000002
#define SHR_BHH_SESS_SET_F_PERIOD               0x00000004
#define SHR_BHH_SESS_SET_F_LOCAL_MIN_RX         0x00000008
#define SHR_BHH_SESS_SET_F_LOCAL_MIN_ECHO_RX    0x00000010
#define SHR_BHH_SESS_SET_F_LOCAL_DIAG           0x00000020
#define SHR_BHH_SESS_SET_F_LOCAL_DEMAND         0x00000040
#define SHR_BHH_SESS_SET_F_LOCAL_DETECT_MULT    0x00000080
#define SHR_BHH_SESS_SET_F_SESSION_DESTROY      0x00000100
#define SHR_BHH_SESS_SET_F_SHA1_XMT_SEQ_INCR    0x00000200
#define SHR_BHH_SESS_SET_F_ENCAP                0x00000400
#define SHR_BHH_SESS_SET_F_CCM                  0x00001000
#define SHR_BHH_SESS_SET_F_LB                   0x00002000
#define SHR_BHH_SESS_SET_F_LM                   0x00004000
#define SHR_BHH_SESS_SET_F_DM                   0x00008000
#define SHR_BHH_SESS_SET_F_MIP                  0x00010000
#define SHR_BHH_SESS_SET_F_PASSIVE              0x00020000

/*
 * BHH Encapsulation types
 */
#define SHR_BHH_ENCAP_TYPE_RAW      0

/*
 * Op Codes
 */
#define SHR_BHH_OPCODE_CCM 1
#define SHR_BHH_OPCODE_LBM 3
#define SHR_BHH_OPCODE_LBR 2
#define SHR_BHH_OPCODE_AIS 33
#define SHR_BHH_OPCODE_LCK 35
#define SHR_BHH_OPCODE_LMM 43
#define SHR_BHH_OPCODE_LMR 42
#define SHR_BHH_OPCODE_1DM 45
#define SHR_BHH_OPCODE_DMM 47
#define SHR_BHH_OPCODE_DMR 46
#define SHR_BHH_OPCODE_CFS XX

/*
 * Flags
 */
#define BCM_BHH_INC_REQUESTING_MEP_TLV         0x00000001
#define BCM_BHH_LBM_INGRESS_DISCOVERY_MEP_TLV  0x00000010
#define BCM_BHH_LBM_EGRESS_DISCOVERY_MEP_TLV   0x00000020
#define BCM_BHH_LBM_ICC_MEP_TLV                0x00000040
#define BCM_BHH_LBM_ICC_MIP_TLV                0x00000080
#define BCM_BHH_LBR_ICC_MEP_TLV                0x00000100
#define BCM_BHH_LBR_ICC_MIP_TLV                0x00000200
#define BCM_BHH_LM_SINGLE_ENDED                0x00000400
#define BCM_BHH_DM_ONE_WAY                     0x00000800
#define BCM_BHH_LM_TX_ENABLE                   0x00001000
#define BCM_BHH_DM_TX_ENABLE                   0x00002000

 /*
  *
  */
#define BCM_BHH_DM_TYPE_NTP                0
#define BCM_BHH_DM_TYPE_PTP                1

/*
 * Macro:
 *      SHR_BHH_IF_ERROR_RETURN
 * Purpose:
 *      Evaluate _op as an expression, and if an error, return.
 * Notes:
 *      This macro uses a do-while construct to maintain expected
 *      "C" blocking, and evaluates "op" ONLY ONCE so it may be
 *      a function call that has side affects.
 */

#define SHR_BHH_IF_ERROR_RETURN(op)                                     \
    do {                                                                \
        int __rv__;                                                     \
        if ((__rv__ = (op)) != SHR_BHH_UC_E_NONE) {                     \
            return(__rv__);                                             \
        }                                                               \
    } while(0)

#endif /* _SOC_SHARED_BHH_H */
