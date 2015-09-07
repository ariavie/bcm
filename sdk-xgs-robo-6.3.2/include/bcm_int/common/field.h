/*
 * $Id: field.h 1.5 Broadcom SDK $
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
 * Purpose:     Common Internal Field Processor data structure definitions for the
 *              BCM library.
 *
 */

#ifndef _BCM_COMMON_FIELD_H
#define _BCM_COMMON_FIELD_H

#include <soc/defs.h>	
#include <bcm/debug.h>

#ifdef BCM_FIELD_SUPPORT

/*
 * Debugging output macros.
 */
#define FP_DEBUG(flags, stuff) BCM_DEBUG((flags) | BCM_DBG_FP, stuff)
#define FP_OUT(stuff)          BCM_DEBUG(BCM_DBG_FP, stuff)
#define FP_WARN(stuff)         FP_DEBUG(BCM_DBG_WARN, stuff)
#define FP_ERR(stuff)          FP_DEBUG(BCM_DBG_ERR, stuff)
#define FP_VERB(stuff)         FP_DEBUG(BCM_DBG_VERBOSE, stuff)
#define FP_VVERB(stuff)        FP_DEBUG(BCM_DBG_VVERBOSE, stuff)
#define FP_SHOW(stuff)         ((*soc_cm_print) stuff)

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/*
 * Typedef:
 *     _stage_id_t
 * Purpose:
 *     Holds format info for pipline stage id. 
 */
typedef int _field_stage_id_t;

/* Pipeline stages for packet processing. */
#define _BCM_FIELD_STAGE_INGRESS               (0)  
#define _BCM_FIELD_STAGE_EXTERNAL              (3)
#define _BCM_FIELD_STAGE_LOOKUP                (1)  
#define _BCM_FIELD_STAGE_EGRESS                (2)  

#define _BCM_FIELD_STAGE_STRINGS \
    {"Ingress",            \
     "Lookup",             \
     "Egress",             \
     "External"}

#endif /* BCM_FIELD_SUPPORT */
#endif /* _BCM_COMMON_FIELD_H */
