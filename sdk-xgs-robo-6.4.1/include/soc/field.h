/*
 * $Id: field.h,v 1.18 Broadcom SDK $
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
 * File:        field.h
 * Purpose:     Register/memory field descriptions
 */

#ifndef _SOC_FIELD_H
#define _SOC_FIELD_H

#include <soc/types.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(PORTMOD_SUPPORT)
#include <soc/mcm/allenum.h>
#endif
#include <soc/schanmsg.h>
#include <soc/types.h>
#ifdef BCM_ROBO_SUPPORT
#include <soc/robo/mcm/allenum.h>
#endif      

#ifdef BCM_EA_SUPPORT    
#include <soc/ea/allenum.h>
#endif

/* Values for flags */

#define SOCF_LE		    0x01	/* little endian */
#define SOCF_RO		    0x02	/* read only */
#define SOCF_WO		    0x04	/* write only */

#define SOCF_LSB	    0x05	/* Used for define the MAC Address format */
#define SOCF_MSB	    0x06	/* Used for define the MAC Address format */

#define SOCF_SC		    0x08       
#define SOCF_RES        0x10    /* reserved (do not test, etc.) */
#define SOCF_GLOBAL     0x20    /* Fields common to different views */
#define SOCF_W1TC	    0x40	/* write 1 to clear, TREX2 STYLE field only */
#define SOCF_COR	    0x80	/* clear on read, TREX2 STYLE field only */
#define SOCF_PUNCH	    0x100	/* punch bit, write 1 but always read 0. TREX2 STYLE field only */
#define SOCF_WVTC       0x200   /* write value to clear, TREX2 STYLE field only */
#define SOCF_RWBW       0x400   /* read/write, block writable, TREX2 STYLE field only */
#define SOCF_SIG        0x800   /* Field is a signal*/
#define SOCF_INTR       0x1000  /*field is an interrupt */
#define SOCF_IGNORE_DEFAULT_TEST       0x2000  /*field is an for ignore default test */



typedef struct soc_field_info_s {
	soc_field_t	field;
	uint16		len;	/* Bits in field */
	uint16		bp;	/* Least bit position of the field */
	uint16  	flags;	/* Logical OR of SOCF_* */
} soc_field_info_t;

/*
 * Find field _fsrch in the field list _flist (with _fnum entries)
 * Sets _infop to the field_info of _fsrch or NULL if not found
 */
#define	SOC_FIND_FIELD(_fsrch, _flist, _fnum, _infop) do { \
    soc_field_info_t *__s, *__m, *__e; \
    _infop = NULL; \
    __s = _flist; \
    if (__s->field == _fsrch) { \
        _infop = __s; \
    } else { \
        __e = &__s[_fnum-1]; \
        if (__e->field == _fsrch) { \
            _infop = __e; \
        } else { \
            __m = __s + _fnum/2; \
            while ((__s < __e) && (__m < __e) && (__s->field != _fsrch) && \
                   (__m->field != _fsrch)) { \
                if (_fsrch < __m->field) { \
                    __e = __m - 1; \
                } else if (_fsrch > __m->field) { \
                    __s = __m + 1; \
                } else { \
                    break; \
                } \
                __m = __s + ((__e-__s)+1)/2; \
            } \
            if (__m->field == _fsrch) { \
                _infop = __m; \
            } else if (__s->field == _fsrch) { \
                _infop = __s; \
            } \
        } \
    } \
} while (0)

EXTERN int soc_mem_field_length(int unit, soc_mem_t mem, soc_field_t field);

#define SOC_MEM_FIELD32_VALUE_MAX(_unit, _mem, _field)         \
    (((uint32)(0xFFFFFFFF)) >>                                 \
     (32 - soc_mem_field_length((_unit), (_mem), (_field))))      

#define SOC_MEM_FIELD32_VALUE_FIT(_unit, _mem, _field, _value) \
    (((uint32)(_value)) <= SOC_MEM_FIELD32_VALUE_MAX((_unit), (_mem), (_field)))

#if !defined(SOC_NO_NAMES)
EXTERN char *soc_fieldnames[];
EXTERN char *soc_robo_fieldnames[];
#define SOC_FIELD_NAME(unit, field)		soc_fieldnames[field]
#else
#define SOC_FIELD_NAME(unit, field)		""
#endif

#endif	/* !_SOC_FIELD_H */
