/*
 * $Id: phymod_custom_config.h,v 1.1.2.6 Broadcom SDK $
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
 * System interface definitions for Switch SDK
 */

#ifndef __PHYMOD_CUSTOM_CONFIG_H__
#define __PHYMOD_CUSTOM_CONFIG_H__

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <sal/core/alloc.h>

#define PHYMOD_EAGLE_SUPPORT
#define PHYMOD_FALCON_SUPPORT
#define PHYMOD_QSGMIIE_SUPPORT
#define PHYMOD_TSCE_SUPPORT
#define PHYMOD_TSCF_SUPPORT

#define PHYMOD_DEBUG_ERROR(stuff_) \
    LOG_ERROR(BSL_LS_SOC_PHYMOD, stuff_)

#define PHYMOD_DEBUG_VERBOSE(stuff_) \
    LOG_VERBOSE(BSL_LS_SOC_PHYMOD, stuff_)

/* Do not map directly to SAL function */
#define PHYMOD_USLEEP   soc_phymod_usleep
#define PHYMOD_SLEEP    soc_phymod_sleep
#define PHYMOD_STRCMP   soc_phymod_strcmp
#define PHYMOD_MEMSET   soc_phymod_memset
#define PHYMOD_MEMCPY   soc_phymod_memcpy
#define PHYMOD_MALLOC   soc_phymod_alloc
#define PHYMOD_FREE     soc_phymod_free

/* These functions map directly to SAL functions */
#define PHYMOD_STRLEN   sal_strlen
#define PHYMOD_STRSTR   sal_strstr
#define PHYMOD_STRCHR   sal_strchr
#define PHYMOD_STRNCMP  sal_strncmp
#define PHYMOD_SNPRINTF sal_snprintf
#define PHYMOD_SPRINTF  sal_sprintf
#define PHYMOD_STRCAT   sal_strcat
#define PHYMOD_STRCPY   sal_strcpy
#define PHYMOD_STRNCPY  sal_strncpy
#define PHYMOD_ATOI     sal_atoi

#ifdef __KERNEL__
#define PHYMOD_STRTOUL  simple_strtol
#else
#include <stdlib.h>
#define PHYMOD_STRTOUL  sal_strtoul
#endif

#include <sal/appl/io.h>
#define PHYMOD_PRINTF   sal_printf

/* Use SDK-versions of stdint types */
#define PHYMOD_CONFIG_DEFINE_UINT8_T    0
#define uint8_t uint8
#define PHYMOD_CONFIG_DEFINE_UINT16_T   0
#define uint16_t uint16
#define PHYMOD_CONFIG_DEFINE_UINT32_T   0
#define uint32_t uint32

/* No need to define size_t */
#define PHYMOD_CONFIG_DEFINE_SIZE_T     0

/* Include register reset values in PHY symbol tables */
#define PHYMOD_CONFIG_INCLUDE_RESET_VALUES      1

#endif /* __PHYMOD_CUSTOM_CONFIG_H__ */
