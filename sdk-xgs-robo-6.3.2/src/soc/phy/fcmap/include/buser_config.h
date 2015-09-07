/*
 * $Id: buser_config.h 1.3 Broadcom SDK $
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

#ifndef __BMF_CONFIG_H__
#define __BMF_CONFIG_H__

#ifndef __PEDANTIC__
#ifndef COMPILER_OVERRIDE_NO_LONGLONG
#ifndef COMPILER_HAS_LONGLONG
#define COMPILER_HAS_LONGLONG
#endif /* !COMPILER_HAS_LONGLONG */
#endif /* !COMPILER_OVERRIDE_NO_LONGLONG */
#endif /* !__PEDANTIC__ */

#include <bcompiler.h>

#ifndef BMF_MAX_UNITS
#define BMF_MAX_UNITS                    16
#endif

/* Maximum number of ports per chip supported */
#ifndef BMF_MAX_PORTS
#define BMF_MAX_PORTS                    8        
#endif

#define BMF_NUM_PORT     (BMF_MAX_UNITS * BMF_MAX_PORTS)

#ifndef BMF_CONFIG_DEFINE_UINT8_T
#define BMF_CONFIG_DEFINE_UINT8_T               1         
#endif

/* Default type definition for uint8 */
#ifndef BMF_CONFIG_TYPE_UINT8_T
#define BMF_CONFIG_TYPE_UINT8_T                 unsigned char
#endif

#ifndef BMF_CONFIG_DEFINE_INT8_T
#define BMF_CONFIG_DEFINE_INT8_T               1         
#endif

#ifndef BMF_CONFIG_TYPE_INT8_T
#define BMF_CONFIG_TYPE_INT8_T                 char
#endif

/* Type buint16_t is not provided by the system */
#ifndef BMF_CONFIG_DEFINE_UINT16_T
#define BMF_CONFIG_DEFINE_UINT16_T              1         
#endif

/* Default type definition for uint16 */
#ifndef BMF_CONFIG_TYPE_UINT16_T
#define BMF_CONFIG_TYPE_UINT16_T                unsigned short
#endif

#ifndef BMF_CONFIG_DEFINE_INT16_T
#define BMF_CONFIG_DEFINE_INT16_T              1         
#endif

#ifndef BMF_CONFIG_TYPE_INT16_T
#define BMF_CONFIG_TYPE_INT16_T                short int
#endif

/* Type buint32_t is not provided by the system */
#ifndef BMF_CONFIG_DEFINE_UINT32_T
#define BMF_CONFIG_DEFINE_UINT32_T              1         
#endif

/* Default type definition for uint32 */
#ifndef BMF_CONFIG_TYPE_UINT32_T
#define BMF_CONFIG_TYPE_UINT32_T                unsigned int
#endif

#ifndef BMF_CONFIG_DEFINE_INT32_T
#define BMF_CONFIG_DEFINE_INT32_T              1         
#endif

#ifndef BMF_CONFIG_TYPE_INT32_T
#define BMF_CONFIG_TYPE_INT32_T                int
#endif

#ifndef BMF_CONFIG_DEFINE_UINT64_T
#define BMF_CONFIG_DEFINE_UINT64_T              1         
#endif

/* Default type definition for uint64 */
#ifndef BMF_CONFIG_TYPE_UINT64_T
#define BMF_CONFIG_TYPE_UINT64_T                BCOMPILER_COMPILER_UINT64
#endif

#ifndef BMF_CONFIG_DEFINE_INT64_T
#define BMF_CONFIG_DEFINE_INT64_T              1         
#endif

#ifndef BMF_CONFIG_TYPE_INT64_T
#define BMF_CONFIG_TYPE_INT64_T                BCOMPILER_COMPILER_INT64
#endif


#endif /* __BMF_CONFIG_H__ */


