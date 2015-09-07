/*
 * $Id: allenum.h 1.8 Broadcom SDK $
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
 * File:        allenum.h
 * Purpose:     Enumerated types for fields, memories, registers
 */

#ifndef _SOC_EA_ALLENUM_H
#define _SOC_EA_ALLENUM_H

#ifndef EXTERN
# ifdef __cplusplus
#  define EXTERN extern "C"
# else
#  define EXTERN extern
# endif
#endif
#ifndef NUM_SOC_MEM 
#define NUM_SOC_MEM 4463    
#endif
typedef int soc_ea_mem_t;
#ifndef soc_mem_t
#define soc_mem_t soc_ea_mem_t
#endif

typedef int soc_ea_field_t;
#ifndef soc_field_t
#define soc_field_t soc_ea_field_t
#endif
#ifndef NUM_SOC_REG
#define NUM_SOC_REG 36652
#endif
typedef int soc_ea_reg_t;

#ifndef soc_reg_t
#define soc_reg_t soc_ea_reg_t
#endif

/* NOTE: 'FIELDf' is the zero value */
#define INVALID_Rf -1
#ifndef INVALIDf
#define INVALIDf INVALID_Rf
#endif

#ifndef  INVALIDr
#define INVALIDr 	-1
#endif

#ifdef BCM_TK371X_SUPPORT
#include <soc/ea/tk371x/allenum.h>
#ifndef SOC_EA_MAX_NUM_PORTS
#define SOC_EA_MAX_NUM_PORTS     11
#endif
#ifndef  SOC_EA_MAX_NUM_BLKS
#define SOC_EA_MAX_NUM_BLKS 	8
#endif
#ifndef SOC_MAX_NUM_PORTS 
#define SOC_MAX_NUM_PORTS 	(SOC_EA_MAX_NUM_PORTS)
#endif
#ifndef SOC_MAX_NUM_BLKS 
#define SOC_MAX_NUM_BLKS 	(SOC_EA_MAX_NUM_BLKS) 
#endif
#undef SOC_MAX_MEM_WORDS
#define SOC_MAX_MEM_WORDS   64
#endif

#endif /* _SOC_EA_ALLENUM_H */


