/*
 * $Id: bfd.h,v 1.37 Broadcom SDK $
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
 * File:    bfd.h
 * Purpose: BFD definitions common to SDK and uKernel
 *
 * Notes:   Definition changes should be avoided in order to
 *          maintain compatibility between SDK and uKernel since
 *          both images are built and loaded separately.
 *
 */

#ifndef _SOC_SHARED_BFD_H
#define _SOC_SHARED_BFD_H

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif

#include <shared/bfd.h>


/*****************************************
 * BFD uController Error codes
 */
typedef enum shr_bfd_uc_error_e {
    SHR_BFD_UC_E_NONE = 0,
    SHR_BFD_UC_E_INTERNAL,
    SHR_BFD_UC_E_MEMORY,
    SHR_BFD_UC_E_UNIT,
    SHR_BFD_UC_E_PARAM,
    SHR_BFD_UC_E_EMPTY,
    SHR_BFD_UC_E_FULL,
    SHR_BFD_UC_E_NOT_FOUND,
    SHR_BFD_UC_E_EXISTS,
    SHR_BFD_UC_E_TIMEOUT,
    SHR_BFD_UC_E_BUSY,
    SHR_BFD_UC_E_FAIL,
    SHR_BFD_UC_E_DISABLED,
    SHR_BFD_UC_E_BADID,
    SHR_BFD_UC_E_RESOURCE,
    SHR_BFD_UC_E_CONFIG,
    SHR_BFD_UC_E_UNAVAIL,
    SHR_BFD_UC_E_INIT,
    SHR_BFD_UC_E_PORT
} shr_bfd_uc_error_t;

#define SHR_BFD_UC_SUCCESS(rv)              ((rv) == SHR_BFD_UC_E_NONE)
#define SHR_BFD_UC_FAILURE(rv)              ((rv) != SHR_BFD_UC_E_NONE)

/*
 * Macro:
 *      SHR_BFD_IF_ERROR_RETURN
 * Purpose:
 *      Evaluate _op as an expression, and if an error, return.
 * Notes:
 *      This macro uses a do-while construct to maintain expected
 *      "C" blocking, and evaluates "op" ONLY ONCE so it may be
 *      a function call that has side affects.
 */

#define SHR_BFD_IF_ERROR_RETURN(op)                                     \
    do {                                                                \
        int __rv__;                                                     \
        if ((__rv__ = (op)) != SHR_BFD_UC_E_NONE) {                     \
            return(__rv__);                                             \
        }                                                               \
    } while(0)


/*
 * BFD for Unknown Remote Discriminator
 * 'your_discriminator' zero
 */
#define SHR_BFD_DISCR_UNKNOWN_SESSION_ID        0x7FF    /* Session ID */
#define SHR_BFD_DISCR_UNKNOWN_YOUR_DISCR        0        /* Discriminator */

/*
 * BFD Session Set flags
 */
#define SHR_BFD_SESS_SET_F_CREATE               0x00000001
#define SHR_BFD_SESS_SET_F_LOCAL_DISC           0x00000002
#define SHR_BFD_SESS_SET_F_LOCAL_MIN_TX         0x00000004
#define SHR_BFD_SESS_SET_F_LOCAL_MIN_RX         0x00000008
#define SHR_BFD_SESS_SET_F_LOCAL_MIN_ECHO_RX    0x00000010
#define SHR_BFD_SESS_SET_F_LOCAL_DIAG           0x00000020
#define SHR_BFD_SESS_SET_F_LOCAL_DEMAND         0x00000040
#define SHR_BFD_SESS_SET_F_LOCAL_DETECT_MULT    0x00000080
#define SHR_BFD_SESS_SET_F_SESSION_DESTROY      0x00000100
#define SHR_BFD_SESS_SET_F_SHA1_XMT_SEQ_INCR    0x00000200
#define SHR_BFD_SESS_SET_F_ENCAP                0x00000400
#define SHR_BFD_SESS_SET_F_REMOTE_DISC          0x00000800


/*
 * BFD Encapsulation types
 */
#define SHR_BFD_ENCAP_TYPE_RAW             0
#define SHR_BFD_ENCAP_TYPE_V4UDP           1
#define SHR_BFD_ENCAP_TYPE_V6UDP           2
#define SHR_BFD_ENCAP_TYPE_MPLS_TP_CC      3 /* MPLS-TP CC only mode */   
#define SHR_BFD_ENCAP_TYPE_MPLS_TP_CC_CV   4 /* MPLS-TP CC CV */   

#endif /* _SOC_SHARED_BFD_H */
