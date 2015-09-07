/*
 * $Id: warmboot.h 1.9 Broadcom SDK $
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
 * File:        warmboot.h
 * Purpose:     
 */

#ifndef   _SHR_WARMBOOT_H_
#define   _SHR_WARMBOOT_H_

#ifdef BCM_WARM_BOOT_SUPPORT

#include <sal/types.h>

/*
 * For tracking the information required by Warm Boot Level 2,
 * we need a handle to both store and recover the scache information.
 * This handle is formatted as follows:
 *
 *  31       24 23        16 15          0
 * |  Unit #   |  Module #  |  Sequence # |
 *
 * Where the unit # is the BCM unit number, the module # is the
 * BCM API module index, and the sequence number is selected by
 * the requesting module, in case multiple scache segments are required.
 */

#define _SHR_SCACHE_HANDLE_UNIT_MASK            0xff
#define _SHR_SCACHE_HANDLE_MODULE_MASK          0xff
#define _SHR_SCACHE_HANDLE_SEQUENCE_MASK        0xff

#define _SHR_SCACHE_HANDLE_UNIT_SHIFT           24
#define _SHR_SCACHE_HANDLE_MODULE_SHIFT         16

#define _SHR_SCACHE_HANDLE_SET(_handle_, _unit_, _module_, _sequence_) \
        ((_handle_) = \
            (((_unit_) & _SHR_SCACHE_HANDLE_UNIT_MASK) << \
                            _SHR_SCACHE_HANDLE_UNIT_SHIFT) | \
            (((_module_) & _SHR_SCACHE_HANDLE_MODULE_MASK) << \
                            _SHR_SCACHE_HANDLE_MODULE_SHIFT) | \
            ((_sequence_) & _SHR_SCACHE_HANDLE_SEQUENCE_MASK))

#define _SHR_SCACHE_HANDLE_UNIT_GET(_handle_) \
        (((_handle_) >> _SHR_SCACHE_HANDLE_UNIT_SHIFT) & \
                _SHR_SCACHE_HANDLE_UNIT_MASK)

#define _SHR_SCACHE_HANDLE_MODULE_GET(_handle_) \
        (((_handle_) >> _SHR_SCACHE_HANDLE_MODULE_SHIFT) & \
                _SHR_SCACHE_HANDLE_MODULE_MASK)

#define _SHR_SCACHE_HANDLE_SEQUENCE_GET(_handle_) \
        ((_handle_) & _SHR_SCACHE_HANDLE_SEQUENCE_MASK)


/* scache warm boot versioning */
#define _SHR_SCACHE_V_MAJOR_SHIFT    8
#define _SHR_SCACHE_V_MAJOR_MASK     0xFF

#define _SHR_SCACHE_V_MINOR_SHIFT    0
#define _SHR_SCACHE_V_MINOR_MASK     0xFF

#define _SHR_SCACHE_VERSION(major_, minor_)  \
        (((major_) & _SHR_SCACHE_V_MAJOR_MASK) << _SHR_SCACHE_V_MAJOR_SHIFT | \
        (((minor_) & _SHR_SCACHE_V_MINOR_MASK) << _SHR_SCACHE_V_MINOR_SHIFT))

#define _SHR_SCACHE_VERSION_MAJOR(v_)    \
        (((v_) >> _SHR_SCACHE_V_MAJOR_SHIFT) & _SHR_SCACHE_V_MAJOR_MASK)
#define _SHR_SCACHE_VERSION_MINOR(v_)     \
        (((v_) >> _SHR_SCACHE_V_MINOR_SHIFT) & _SHR_SCACHE_V_MINOR_MASK)


/* A common scache layout for warm boot */
typedef struct _shr_wb_cache_s {
    uint16  version;        /* for upgrade scenarios */
    uint16  reserved;
    uint8   cache[1];        /* variable length, MUST BE LAST */
} _shr_wb_cache_t;

/* Size of scache used by control data */
#define _SHR_WB_SCACHE_CONTROL_SIZE   \
   ((size_t) &((_shr_wb_cache_t*)0)->cache)

/* get the size of the caller usable scache,
 * size of _shr_wb_cache_t->scache 
 */
#define _SHR_WB_SCACHE_SIZE(raw_size_) \
  ((raw_size_) - _SHR_WB_SCACHE_CONTROL_SIZE)


#define __WB_CHECK_OVERRUN(z,str)				    	       \
    if (ptr+z>end_ptr){							       \
      SOC_ERROR_PRINT((DK_ERR, "%s: Incoherent state of scache (%s is not stored correctly).\n", \
                       FUNCTION_NAME(), str));                          \
      return SOC_E_INTERNAL;						       \
    }

/* Following macro used for warm and cold boot */
#define __WB_DECOMPRESS_SCALAR(type, var) do {\
    if (SOC_WARM_BOOT(unit)){				\
      __WB_CHECK_OVERRUN(sizeof(type), #var);		\
      (var) = *(type*)ptr; ptr+=sizeof(type);		\
    }else{						\
      scache_len += sizeof(type);			\
    }							\
  } while(0)

#define __WB_COMPRESS_SCALAR(type, var) do {\
    __WB_CHECK_OVERRUN(sizeof(type), #var);		\
    *(type*)ptr = (var); ptr+=sizeof(type); } while(0)

#define _WB_OP_DECOMPRESS (0)
#define _WB_OP_COMPRESS   (1)
#define _WB_OP_SIZE       (2)
#define _WB_OP_DUMP       (3)


#endif /* BCM_WARM_BOOT_SUPPORT */

#endif /* _SHR_WARMBOOT_H_ */
