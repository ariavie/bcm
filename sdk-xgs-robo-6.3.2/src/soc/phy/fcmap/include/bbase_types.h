/*
 * $Id: bbase_types.h 1.3 Broadcom SDK $
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
#ifndef __BBASE_TYPES_H
#define __BBASE_TYPES_H

#include <buser_config.h>

#if BMF_CONFIG_DEFINE_UINT8_T == 1
typedef BMF_CONFIG_TYPE_UINT8_T buint8_t; 
#endif

#if BMF_CONFIG_DEFINE_INT8_T == 1
typedef BMF_CONFIG_TYPE_INT8_T bint8_t; 
#endif

#if BMF_CONFIG_DEFINE_UINT16_T == 1
typedef BMF_CONFIG_TYPE_UINT16_T buint16_t; 
#endif

#if BMF_CONFIG_DEFINE_INT16_T == 1
typedef BMF_CONFIG_TYPE_INT16_T bint16_t; 
#endif

#if BMF_CONFIG_DEFINE_UINT32_T == 1
typedef BMF_CONFIG_TYPE_UINT32_T buint32_t; 
#endif

#if BMF_CONFIG_DEFINE_INT32_T == 1
typedef BMF_CONFIG_TYPE_INT32_T bint32_t; 
#endif

#if BMF_CONFIG_DEFINE_UINT64_T == 1
typedef BMF_CONFIG_TYPE_UINT64_T buint64_t; 
#endif

#if BMF_CONFIG_DEFINE_INT64_T == 1
typedef BMF_CONFIG_TYPE_INT64_T bint64_t; 
#endif


typedef buint8_t bmac_addr_t[6]; 

#define BMAC_TO_32_HI(mac)\
   ((((buint8_t *)(mac))[0] << 8  )|\
    (((buint8_t *)(mac))[1] << 0  ))
#define BMAC_TO_32_LO(mac)\
   ((((buint8_t *)(mac))[2] << 24 )|\
    (((buint8_t *)(mac))[3] << 16 )|\
    (((buint8_t *)(mac))[4] << 8  )|\
    (((buint8_t *)(mac))[5] << 0  ))
#define BMAC_BUILD_FROM_32(mac, hi, lo)\
   ((buint8_t *)(mac))[0] = ((hi) >> 8)  & 0xff ;\
   ((buint8_t *)(mac))[1] = ((hi) >> 0)  & 0xff ;\
   ((buint8_t *)(mac))[2] = ((lo) >> 24) & 0xff ;\
   ((buint8_t *)(mac))[3] = ((lo) >> 16) & 0xff ;\
   ((buint8_t *)(mac))[4] = ((lo) >> 8 ) & 0xff ;\
   ((buint8_t *)(mac))[5] = ((lo) >> 0 ) & 0xff ;

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef STATIC
#define STATIC static
#endif

typedef int   (*bprint_fn)(const char *format, ...);
#define BPRINT_FN_DEBUG      0x1
#define BPRINT_FN_PRINTF     0x2


#endif /* __BBASE_TYPES_H */

