/*
 * $Id: error.h,v 1.23 Broadcom SDK $
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
 * This file defines common error codes to be shared between API layers.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define error codes.
 */

#ifndef _SHR_ERROR_H
#define _SHR_ERROR_H

typedef enum {
    _SHR_E_NONE                 = 0,
    _SHR_E_INTERNAL             = -1,
    _SHR_E_MEMORY               = -2,
    _SHR_E_UNIT                 = -3,
    _SHR_E_PARAM                = -4,
    _SHR_E_EMPTY                = -5,
    _SHR_E_FULL                 = -6,
    _SHR_E_NOT_FOUND            = -7,
    _SHR_E_EXISTS               = -8,
    _SHR_E_TIMEOUT              = -9,
    _SHR_E_BUSY                 = -10,
    _SHR_E_FAIL                 = -11,
    _SHR_E_DISABLED             = -12,
    _SHR_E_BADID                = -13,
    _SHR_E_RESOURCE             = -14,
    _SHR_E_CONFIG               = -15,
    _SHR_E_UNAVAIL              = -16,
    _SHR_E_INIT                 = -17,
    _SHR_E_PORT                 = -18,

    _SHR_E_LIMIT                = -19           /* Must come last */
} _shr_error_t;

#define _SHR_ERRMSG_INIT        { \
        "Ok",                           /* E_NONE */ \
        "Internal error",               /* E_INTERNAL */ \
        "Out of memory",                /* E_MEMORY */ \
        "Invalid unit",                 /* E_UNIT */ \
        "Invalid parameter",            /* E_PARAM */ \
        "Table empty",                  /* E_EMPTY */ \
        "Table full",                   /* E_FULL */ \
        "Entry not found",              /* E_NOT_FOUND */ \
        "Entry exists",                 /* E_EXISTS */ \
        "Operation timed out",          /* E_TIMEOUT */ \
        "Operation still running",      /* E_BUSY */ \
        "Operation failed",             /* E_FAIL */ \
        "Operation disabled",           /* E_DISABLED */ \
        "Invalid identifier",           /* E_BADID */ \
        "No resources for operation",   /* E_RESOURCE */ \
        "Invalid configuration",        /* E_CONFIG */ \
        "Feature unavailable",          /* E_UNAVAIL */ \
        "Feature not initialized",      /* E_INIT */ \
        "Invalid port",                 /* E_PORT */ \
        "Unknown error"                 /* E_LIMIT */ \
        }

extern char *_shr_errmsg[];

#define _SHR_ERRMSG(r)          \
        _shr_errmsg[(((int)r) <= 0 && ((int)r) > _SHR_E_LIMIT) ? -(r) : -_SHR_E_LIMIT]

#define _SHR_E_SUCCESS(rv)              ((rv) >= 0)
#define _SHR_E_FAILURE(rv)              ((rv) < 0)

/*
 * Macro:
 *      _SHR_E_IF_ERROR_RETURN
 * Purpose:
 *      Evaluate _op as an expression, and if an error, return.
 * Notes:
 *      This macro uses a do-while construct to maintain expected
 *      "C" blocking, and evaluates "op" ONLY ONCE so it may be
 *      a function call that has side affects.
 */

#ifdef DEBUG_ERR_TRACE
#include <soc/debug.h>
#include <soc/cm.h>
#define _SHR_ERROR_TRACE(__errcode__)  SOC_DEBUG_PRINT((DK_ERR, "ERROR(%s, %u, %d)\n", __FILE__, __LINE__, __errcode__))
#define _SHR_RETURN(__expr__)  do { int __errcode__ = (__expr__);  if (__errcode__ < 0) _SHR_ERROR_TRACE(__errcode__);  return (__errcode__); } while (0)
#else
#define _SHR_ERROR_TRACE(__errcode__)
#define _SHR_RETURN(__expr__)  return (__expr__)
#endif

#define _SHR_E_IF_ERROR_RETURN(op) \
    do { int __rv__; if ((__rv__ = (op)) < 0) { _SHR_ERROR_TRACE(__rv__);  return(__rv__); } } while(0)

#define _SHR_E_IF_ERROR_NOT_UNAVAIL_RETURN(op)                       \
    do {                                                                \
        int __rv__;                                                     \
        if (((__rv__ = (op)) < 0) && (__rv__ != _SHR_E_UNAVAIL)) {      \
            return(__rv__);                                             \
        }                                                               \
    } while(0)


typedef enum {
    _SHR_SWITCH_EVENT_IO_ERROR      = 1,
    _SHR_SWITCH_EVENT_PARITY_ERROR  = 2,
    _SHR_SWITCH_EVENT_THREAD_ERROR  = 3,
    _SHR_SWITCH_EVENT_ACCESS_ERROR  = 4,
    _SHR_SWITCH_EVENT_ASSERT_ERROR  = 5,
    _SHR_SWITCH_EVENT_MODID_CHANGE  = 6,
    _SHR_SWITCH_EVENT_DOS_ATTACK    = 7,
    _SHR_SWITCH_EVENT_STABLE_FULL   = 8,
    _SHR_SWITCH_EVENT_STABLE_ERROR   = 9,
    _SHR_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN = 10,
    _SHR_SWITCH_EVENT_WARM_BOOT_DOWNGRADE = 11,
    _SHR_SWITCH_EVENT_TUNE_ERROR = 12,
    _SHR_SWITCH_EVENT_DEVICE_INTERRUPT  = 13,
    _SHR_SWITCH_EVENT_ALARM = 14,
    _SHR_SWITCH_EVENT_MMU_BST_TRIGGER = 15,
    _SHR_SWITCH_EVENT_EPON_ALARM = 16,
    _SHR_SWITCH_EVENT_COUNT             /* last, as always */
} _shr_switch_event_t;

#endif  /* !_SHR_ERROR_H */
