/*
 * $Id: sdk_source_check.h,v 1.1 Broadcom SDK $
 * 
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
 * Check for basic source code architecture violations.
 *
 * Note that if this file is included multiple times, it must
 * be propcessed for errors every time.
 */

/*
 * Default source check configuration
 */

#ifndef SCHK_SAL_APPL_HEADERS_IN_SOC
#define SCHK_SAL_APPL_HEADERS_IN_SOC    1
#endif
#ifndef SCHK_BCM_HEADERS_IN_SOC
#define SCHK_BCM_HEADERS_IN_SOC         1
#endif
#ifndef SCHK_PRINTK_IN_SOC
#define SCHK_PRINTK_IN_SOC              1
#endif
#ifndef SCHK_SAL_APPL_HEADERS_IN_BCM
#define SCHK_SAL_APPL_HEADERS_IN_BCM    1
#endif
#ifndef SCHK_PRINTK_IN_BCM
#define SCHK_PRINTK_IN_BCM              1
#endif
#ifndef SCHK_SOC_CM_PRINT_IN_APPL
#define SCHK_SOC_CM_PRINT_IN_APPL       0
#endif
#ifndef SCHK_SOC_CM_DEBUG_IN_APPL
#define SCHK_SOC_CM_DEBUG_IN_APPL       0
#endif
#ifndef SCHK_SSCANF_IN_ANY
#define SCHK_SSCANF_IN_ANY              1
#endif
#ifndef SCHK_FSCANF_IN_ANY
#define SCHK_FSCANF_IN_ANY              1
#endif

/* Help macros */
#if defined(_SAL_CONFIG_H) || defined(_SAL_PCI_H) || \
    defined(_SAL_IO_H) || defined(_SAL_H)
#ifndef _SAL_APPL_HDRS
#define _SAL_APPL_HDRS
#endif
#endif

#if defined(SOURCE_CHECK_LIBSOC)

#if SCHK_SAL_APPL_HEADERS_IN_SOC
#if defined(_SAL_APPL_HDRS)
#error SAL_APPL_headers_included_from_SOC_sources
#endif
#endif

#if SCHK_BCM_HEADERS_IN_SOC
#if defined(__BCM_TYPES_H__)
#error BCM_headers_included_from_SOC_sources
#endif
#endif

#if SCHK_PRINTK_IN_SOC
#ifndef printk
#define printk          DO_NOT_USE_printk_in_SOC
#endif
#endif

#endif /* defined(SOURCE_CHECK_LIBSOC) */


#if defined(SOURCE_CHECK_LIBBCM)

#if SCHK_SAL_APPL_HEADERS_IN_BCM
#if defined(_SAL_APPL_HDRS)
#error SAL_APPL_headers_included_from_BCM_sources
#endif
#endif

#if SCHK_PRINTK_IN_BCM
#ifndef printk
#define printk          DO_NOT_USE_printk_in_BCM
#endif
#endif

#endif /* defined(SOURCE_CHECK_LIBBCM) */


#if defined(SOURCE_CHECK_LIBAPPL) && 0

#if SCHK_SOC_CM_PRINT_IN_APPL
#ifndef soc_cm_print
#define soc_cm_print    DO_NOT_USE_soc_cm_print_in_APPL
#endif
#endif

#if SCHK_SOC_CM_DEBUG_IN_APPL
#ifndef soc_cm_debug
#define soc_cm_debug    DO_NOT_USE_soc_cm_debug_in_APPL
#endif
#endif

#endif /* defined(SOURCE_CHECK_LIBAPPL) */


/*
 * Checks that apply to all source files
 */

#if defined(SOURCE_CHECK_LIBSOC) || \
    defined(SOURCE_CHECK_LIBBCM) || \
    defined(SOURCE_CHECK_LIBAPPL)

#if SCHK_SSCANF_IN_ANY
#ifndef sscanf
#define sscanf          DO_NOT_USE_sscanf
#endif
#endif

#if SCHK_FSCANF_IN_ANY
#ifndef fscanf
#define fscanf          DO_NOT_USE_fscanf
#endif
#endif

#endif
