/*
 * $Id: bfcmap_util.h 1.3 Broadcom SDK $
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

#ifndef BFCMAP_UTIL_H
#define BFCMAP_UTIL_H

#include <bbase_util.h>

#define BFCMAP_PRINTF              BBASE_PRINTF

#define BFCMAP_DBG_PRINTF          BBASE_DBG_PRINTF


#ifndef BFCMAP_SAL_ASSERT
#define BFCMAP_SAL_ASSERT(c)       BMF_SAL_ASSERT(c) 
#endif

#ifndef BFCMAP_SAL_PRINTF
#define BFCMAP_SAL_PRINTF          BMF_SAL_PRINTF
#endif

#ifndef BFCMAP_SAL_DBG_PRINTF
#define BFCMAP_SAL_DBG_PRINTF      BMF_SAL_DBG_PRINTF
#endif

#ifndef BFCMAP_SAL_MEMCPY
#define BFCMAP_SAL_MEMCPY          BMF_SAL_MEMCPY
#endif

#ifndef BFCMAP_SAL_MEMCMP
#define BFCMAP_SAL_MEMCMP          BMF_SAL_MEMCMP
#endif

#ifndef BFCMAP_SAL_MEMSET
#define BFCMAP_SAL_MEMSET          BMF_SAL_MEMSET
#endif


#ifndef BFCMAP_SAL_STRLEN
#define BFCMAP_SAL_STRLEN          BMF_SAL_STRLEN
#endif

#ifndef BFCMAP_SAL_STRCPY
#define BFCMAP_SAL_STRCPY          BMF_SAL_STRCPY
#endif

#ifndef BFCMAP_SAL_STRNCPY
#define BFCMAP_SAL_STRNCPY         BMF_SAL_STRNCPY
#endif

#ifndef BFCMAP_SAL_STRCMP
#define BFCMAP_SAL_STRCMP          BMF_SAL_STRCMP
#endif

#ifndef BFCMAP_SAL_STRCMPI
#define BFCMAP_SAL_STRCMPI         BMF_SAL_STRCMPI
#endif

#ifndef BFCMAP_SAL_STRNCMP
#define BFCMAP_SAL_STRNCMP         BMF_SAL_STRNCMP
#endif

#ifndef BFCMAP_SAL_STRNCASECMP
#define BFCMAP_SAL_STRNCASECMP     BMF_SAL_STRNCASECMP
#endif

#ifndef BFCMAP_SAL_STRCHR
#define BFCMAP_SAL_STRCHR          BMF_SAL_STRCHR
#endif

#ifndef BFCMAP_SAL_STRSTR
#define BFCMAP_SAL_STRSTR          BMF_SAL_STRSTR
#endif

#ifndef BFCMAP_SAL_STRCAT
#define BFCMAP_SAL_STRCAT          BMF_SAL_STRCAT
#endif

#ifndef BFCMAP_SAL_STRTOK
#define BFCMAP_SAL_STRTOK          BMF_SAL_STRTOK
#endif

#ifndef BFCMAP_SAL_ATOI
#define BFCMAP_SAL_ATOI            BMF_SAL_ATOI
#endif

#ifndef BFCMAP_SAL_STRDUP 
#define BFCMAP_SAL_STRDUP          BMF_SAL_STRDUP
#endif

#ifndef BFCMAP_SAL_XSPRINTF
#define BFCMAP_SAL_XSPRINTF        BMF_SAL_XSPRINTF
#endif /* BFCMAP_SAL_XSPRINTF */

#endif /* BFCMAP_UTIL_H */

