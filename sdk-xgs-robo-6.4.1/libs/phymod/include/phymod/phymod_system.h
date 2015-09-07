/*
 * $Id: phymod_system.h,v 1.1.2.4 Broadcom SDK $
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

#ifndef __PHYMOD_SYSTEM_H__
#define __PHYMOD_SYSTEM_H__

#include <phymod/phymod_types.h>

/*
 * This file defines the system interface for a given platform.
 */

#if PHYMOD_CONFIG_INCLUDE_ERROR_PRINT
#ifndef PHYMOD_DEBUG_ERROR
#error PHYMOD_DEBUG_ERROR must be defined when PHYMOD_CONFIG_INCLUDE_ERROR_PRINT is enabled
#endif
#else
#ifdef PHYMOD_DEBUG_ERROR
#undef PHYMOD_DEBUG_ERROR
#endif
#define PHYMOD_DEBUG_ERROR()
#endif

#if PHYMOD_CONFIG_INCLUDE_DEBUG_PRINT
#ifndef PHYMOD_DEBUG_VERBOSE
#error PHYMOD_DEBUG_VERBOSE must be defined when PHYMOD_CONFIG_INCLUDE_DEBUG_PRINT is enabled
#endif
#else
#ifdef PHYMOD_DEBUG_VERBOSE
#undef PHYMOD_DEBUG_VERBOSE
#endif
#define PHYMOD_DEBUG_VERBOSE()
#endif

#ifndef PHYMOD_USLEEP
#error PHYMOD_USLEEP must be defined for the target system
#else
extern void PHYMOD_USLEEP(uint32_t usecs);
#endif

#ifndef PHYMOD_SLEEP
#error PHYMOD_SLEEP must be defined for the target system
#else
extern void PHYMOD_SLEEP(int secs);
#endif

#ifndef PHYMOD_MALLOC
#error PHYMOD_MALLOC must be defined for the target system
#else
extern void *PHYMOD_MALLOC(size_t size, char *descr);
#endif

#ifndef PHYMOD_FREE
#error PHYMOD_FREE must be defined for the target system
#else
extern void PHYMOD_FREE(void *ptr);
#endif

#ifndef PHYMOD_MEMSET
#error PHYMOD_MEMSET must be defined for the target system
#else
extern void *PHYMOD_MEMSET(void *dst_void, int val, size_t len);
#endif

#ifndef PHYMOD_MEMCPY
#error PHYMOD_MEMCPY must be defined for the target system
#else
extern void *PHYMOD_MEMCPY(void *dst, const void *src, size_t n);
#endif

#ifndef PHYMOD_STRCMP
#error PHYMOD_STRCMP must be defined for the target system
#else
extern int PHYMOD_STRCMP(const char *str1, const char *str2);
#endif

#ifndef PHYMOD_ASSERT
#define PHYMOD_ASSERT(expr_)
#endif

#endif /* __PHYMOD_SYSTEM_H__ */
