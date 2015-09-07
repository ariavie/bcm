/*
 * $Id: bbase_util.h,v 1.2 Broadcom SDK $
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

#ifndef BBASE_UTIL_H
#define BBASE_UTIL_H

#include <sal/types.h>
#include <sal/core/spl.h>
#include <sal/core/libc.h>
#include <soc/cm.h>
#include <bbase_types.h>
#ifndef  BMF_INCLUDE_FOR_LKM
#include <buser_sal.h>
#endif

extern bprint_fn _bfcprint_fn_func_cb;
extern buint32_t _bfcprint_fn_flag;

#if 0
#define BBASE_PRINTF(...)                               \
      if ((_bfcprint_fn_flag & BPRINT_FN_PRINTF) &&       \
            (_bfcprint_fn_func_cb)) {                     \
            _bfcprint_fn_func_cb(__VA_ARGS__);            \
      }

#define BBASE_DBG_PRINTF(...)                           \
      if ((_bfcprint_fn_flag & BPRINT_FN_DEBUG) &&        \
            (_bfcprint_fn_func_cb)) {                     \
            _bfcprint_fn_func_cb(__VA_ARGS__);            \
      }
#else
#define BBASE_PRINTF            bsl_printf
#define BBASE_DBG_PRINTF \
      if (_bfcprint_fn_flag & BPRINT_FN_DEBUG) bsl_printf

#endif


#ifndef BMF_SAL_ASSERT
#define BMF_SAL_ASSERT(c)
#endif

#ifndef BMF_SAL_PRINTF
#define BMF_SAL_PRINTF          BBASE_PRINTF
#endif

#ifndef BMF_SAL_DBG_PRINTF
#define BMF_SAL_DBG_PRINTF      BBASE_DBG_PRINTF
#endif

#ifndef BMF_SAL_MEMCPY
#define BMF_SAL_MEMCPY           sal_memcpy
#define BMACSEC_INCLUDE_LIB_MEMCPY
#endif

#ifndef BMF_SAL_MEMCMP
#define BMF_SAL_MEMCMP           sal_memcmp
#define BMACSEC_INCLUDE_LIB_MEMCMP
#endif

#ifndef BMF_SAL_MEMSET
#define BMF_SAL_MEMSET           sal_memset
#define BMACSEC_INCLUDE_LIB_MEMSET
#endif


#ifndef BMF_SAL_STRLEN
#define BMF_SAL_STRLEN           sal_strlen
#define BMACSEC_INCLUDE_LIB_STRLEN
#endif

#ifndef BMF_SAL_STRCPY
#define BMF_SAL_STRCPY           sal_strcpy
#define BMACSEC_INCLUDE_LIB_STRCPY
#endif

#ifndef BMF_SAL_STRNCPY
#define BMF_SAL_STRNCPY           sal_strncpy
#define BMACSEC_INCLUDE_LIB_STRNCPY
#endif

#ifndef BMF_SAL_STRCMP
#define BMF_SAL_STRCMP           sal_strcmp
#define BMACSEC_INCLUDE_LIB_STRCMP
#endif

#ifndef BMF_SAL_STRCMPI
#define BMF_SAL_STRCMPI           sal_strcmpi
#define BMACSEC_INCLUDE_LIB_STRCMPI
#endif

#ifndef BMF_SAL_STRNCMP
#define BMF_SAL_STRNCMP           sal_strncmp
#define BMACSEC_INCLUDE_LIB_STRNCMP
#endif

#ifndef BMF_SAL_STRNCASECMP
#define BMF_SAL_STRNCASECMP           sal_strncasecmp
#define BMACSEC_INCLUDE_LIB_STRCASECMP
#endif

#ifndef BMF_SAL_STRCHR
#define BMF_SAL_STRCHR           sal_strchr
#define BMACSEC_INCLUDE_LIB_STRCHR
#endif

#ifndef BMF_SAL_STRSTR
#define BMF_SAL_STRSTR           sal_strstr
#define BMACSEC_INCLUDE_LIB_STRSTR
#endif

#ifndef BMF_SAL_STRCAT
#define BMF_SAL_STRCAT           sal_strcat
#define BMACSEC_INCLUDE_LIB_STRCAT
#endif

#ifndef BMF_SAL_STRTOK
#define BMF_SAL_STRTOK           sal_strtok
#define BMACSEC_INCLUDE_LIB_STRTOK
#endif

#ifndef BMF_SAL_ATOI
#define BMF_SAL_ATOI           sal_atoi
#define BMACSEC_INCLUDE_LIB_ATOI
#endif

#ifndef BMF_SAL_STRDUP 
#define BMF_SAL_STRDUP           sal_strdup
#define BMACSEC_INCLUDE_LIB_STRDUP
#endif

#ifndef BMF_SAL_XSPRINTF
#define BMF_SAL_XSPRINTF sal_xsprintf
#define BMACSEC_INCLUDE_LIB_XSPRINTF
#endif /* BMF_SAL_XSPRINTF */

#ifdef  BMF_INCLUDE_FOR_LKM

extern int bmacsec_cli_register(void *);
EXPORT_SYMBOL(bmacsec_cli_register);
extern int bmacsec_handle_cmd(int, char **);
EXPORT_SYMBOL(bmacsec_handle_cmd);

EXPORT_SYMBOL(_bfcprint_fn_flag);
EXPORT_SYMBOL(_bfcprint_fn_func_cb);
EXPORT_SYMBOL(bmacsec_memcpy);
EXPORT_SYMBOL(bmacsec_memset);
EXPORT_SYMBOL(bmacsec_xsprintf);
EXPORT_SYMBOL(bmacsec_atoi);
EXPORT_SYMBOL(bmacsec_strcat);
EXPORT_SYMBOL(bmacsec_strchr);
EXPORT_SYMBOL(bmacsec_strcmp);
EXPORT_SYMBOL(bmacsec_strcmpi);
EXPORT_SYMBOL(bmacsec_strncmp);
EXPORT_SYMBOL(bmacsec_strncasecmp);
EXPORT_SYMBOL(bmacsec_strcpy);
EXPORT_SYMBOL(bmacsec_strdup);
EXPORT_SYMBOL(bmacsec_strlen);
EXPORT_SYMBOL(bmacsec_strncpy);
EXPORT_SYMBOL(bmacsec_strstr);
EXPORT_SYMBOL(bmacsec_strtok);

#endif /* BMF_INCLUDE_FOR_LKM */

#endif /* BBASE_UTIL_H */

