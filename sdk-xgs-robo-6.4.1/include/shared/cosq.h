/*
 * $Id: cosq.h,v 1.4 Broadcom SDK $
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
 * This file defines COSQ constants 
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to share  COSQ constants.
 */

#ifndef _SHR_COSQ_H
#define _SHR_COSQ_H

#define _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_WHOLE_SHIFT    0
#define _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_WHOLE_MASK     0xFFFF
#define _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_FRACT_SHIFT    (16)
#define _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_FRACT_MASK     0xFFFF

#define _SHR_COSQ_GPORT_PRIORITY_PROFILE_SHIFT                  16
#define _SHR_COSQ_GPORT_PRIORITY_PROFILE_MASK			0x07

/* 
 *  COSQ macros for bcm_cosq_control_t types
 */
#define _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTILIZATION_SET(whole_utilization, fractional_utilization) \
              (((whole_utilization & _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_WHOLE_MASK) <<           \
                                        _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_WHOLE_SHIFT) |        \
               ((fractional_utilization & _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_FRACT_MASK) <<      \
                                        _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_FRACT_SHIFT))

#define _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTILIZATION_WHOLE_GET(utilization)                         \
                ((utilization >> _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_WHOLE_SHIFT) & _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_WHOLE_MASK)


#define _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTILIZATION_FRACTIONAL_GET(utilization)                    \
                ((utilization >> _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_FRACT_SHIFT) & _SHR_COSQ_CONTROL_FABRIC_CONNECT_MIN_UTIL_FRACT_MASK)


/*
 * priority profile template
 */
#define _SHR_COSQ_GPORT_PRIORITY_PROFILE_SET(_flags, _profile)             \
    _flags = _flags | ((_profile & _SHR_COSQ_GPORT_PRIORITY_PROFILE_MASK) << _SHR_COSQ_GPORT_PRIORITY_PROFILE_SHIFT)
#define _SHR_COSQ_GPORT_PRIORITY_PROFILE_GET(_flags)                       \
    ((_flags >> _SHR_COSQ_GPORT_PRIORITY_PROFILE_SHIFT) & _SHR_COSQ_GPORT_PRIORITY_PROFILE_MASK)

#endif	/* !_SHR_COSQ_H */
