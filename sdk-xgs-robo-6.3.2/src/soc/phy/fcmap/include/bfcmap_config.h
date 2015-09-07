/*
 * $Id: bfcmap_config.h 1.3 Broadcom SDK $
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

#ifndef __BFCMAP_CONFIG_H__
#define __BFCMAP_CONFIG_H__

#include <buser_config.h>

#ifndef BFCMAP_MAX_UNITS
#define BFCMAP_MAX_UNITS                    BMF_MAX_UNITS
#endif

/* Maximum number of ports per chip supported */
#ifndef BFCMAP_MAX_PORTS
#define BFCMAP_MAX_PORTS                    BMF_MAX_PORTS
#endif

#define BFCMAP_NUM_PORT                     BMF_NUM_PORT

#ifndef BFCMAP_CONFIG_DEFINE_UINT8_T
#define BFCMAP_CONFIG_DEFINE_UINT8_T        BMF_CONFIG_DEFINE_UINT8_T
#endif

/* Default type definition for uint8 */
#ifndef BFCMAP_CONFIG_TYPE_UINT8_T
#define BFCMAP_CONFIG_TYPE_UINT8_T          BMF_CONFIG_TYPE_UINT8_T
#endif

#ifndef BFCMAP_CONFIG_DEFINE_INT8_T
#define BFCMAP_CONFIG_DEFINE_INT8_T         BMF_CONFIG_DEFINE_INT8_T
#endif

#ifndef BFCMAP_CONFIG_TYPE_INT8_T
#define BFCMAP_CONFIG_TYPE_INT8_T           BMF_CONFIG_TYPE_INT8_T
#endif

/* Type buint16_t is not provided by the system */
#ifndef BFCMAP_CONFIG_DEFINE_UINT16_T
#define BFCMAP_CONFIG_DEFINE_UINT16_T       BMF_CONFIG_DEFINE_UINT16_T
#endif

/* Default type definition for uint16 */
#ifndef BFCMAP_CONFIG_TYPE_UINT16_T
#define BFCMAP_CONFIG_TYPE_UINT16_T         BMF_CONFIG_TYPE_UINT16_T
#endif

#ifndef BFCMAP_CONFIG_DEFINE_INT16_T
#define BFCMAP_CONFIG_DEFINE_INT16_T        BMF_CONFIG_DEFINE_INT16_T
#endif

#ifndef BFCMAP_CONFIG_TYPE_INT16_T
#define BFCMAP_CONFIG_TYPE_INT16_T          BMF_CONFIG_TYPE_INT16_T
#endif

/* Type buint32_t is not provided by the system */
#ifndef BFCMAP_CONFIG_DEFINE_UINT32_T
#define BFCMAP_CONFIG_DEFINE_UINT32_T       BMF_CONFIG_DEFINE_UINT32_T
#endif

/* Default type definition for uint32 */
#ifndef BFCMAP_CONFIG_TYPE_UINT32_T
#define BFCMAP_CONFIG_TYPE_UINT32_T         BMF_CONFIG_TYPE_UINT32_T
#endif

#ifndef BFCMAP_CONFIG_DEFINE_INT32_T
#define BFCMAP_CONFIG_DEFINE_INT32_T        BMF_CONFIG_DEFINE_INT32_T
#endif

#ifndef BFCMAP_CONFIG_TYPE_INT32_T
#define BFCMAP_CONFIG_TYPE_INT32_T          BMF_CONFIG_TYPE_INT32_T
#endif

#ifndef BFCMAP_CONFIG_DEFINE_UINT64_T
#define BFCMAP_CONFIG_DEFINE_UINT64_T       BMF_CONFIG_DEFINE_UINT64_T
#endif

/* Default type definition for uint64 */
#ifndef BFCMAP_CONFIG_TYPE_UINT64_T
#define BFCMAP_CONFIG_TYPE_UINT64_T         BMF_CONFIG_TYPE_UINT64_T
#endif

#ifndef BFCMAP_CONFIG_DEFINE_INT64_T
#define BFCMAP_CONFIG_DEFINE_INT64_T        BMF_CONFIG_DEFINE_INT64_T
#endif

#ifndef BFCMAP_CONFIG_TYPE_INT64_T
#define BFCMAP_CONFIG_TYPE_INT64_T          BMF_CONFIG_TYPE_INT64_T
#endif


#endif /* __BFCMAP_CONFIG_H__ */


