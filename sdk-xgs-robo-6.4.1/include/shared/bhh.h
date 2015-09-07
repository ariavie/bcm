/* 
 * $Id: bhh.h,v 1.2 Broadcom SDK $
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
 * File:        bhh.h
 * Purpose:     Defines common BHH parameters.
 */

#ifndef   _SHR_BHH_H_
#define   _SHR_BHH_H_


/* BHH Session States */
typedef enum _shr_bhh_session_state_e {
    _SHR_BHH_SESSION_STATE_ADMIN_DOWN,
    _SHR_BHH_SESSION_STATE_DOWN,
    _SHR_BHH_SESSION_STATE_INIT,
    _SHR_BHH_SESSION_STATE_UP,
    _SHR_BHH_SESSION_STATE_COUNT        /* MUST BE LAST  */
} _shr_bhh_session_state_t;


/* BHH Diagnostic Codes */
typedef enum _shr_bhh_diag_code_e {
    _SHR_BHH_DIAG_CODE_NONE,
    _SHR_BHH_DIAG_CODE_CTRL_DETECT_TIME_EXPIRED,
    _SHR_BHH_DIAG_CODE_ECHO_FAILED,
    _SHR_BHH_DIAG_CODE_NEIGHBOR_SIGNALED_SESSION_DOWN,
    _SHR_BHH_DIAG_CODE_FORWARDING_PLANE_RESET,
    _SHR_BHH_DIAG_CODE_PATH_DOWN,
    _SHR_BHH_DIAG_CODE_CONCATENATED_PATH_DOWN,
    _SHR_BHH_DIAG_CODE_ADMIN_DOWN,
    _SHR_BHH_DIAG_CODE_REVERSE_CONCATENATED_PATH_DOWN,
    _SHR_BHH_DIAG_CODE_COUNT        /* MUST BE LAST  */
} _shr_bhh_diag_code_t;


#endif /* _SHR_BHH_H_ */
