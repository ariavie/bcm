/*
 * $Id: debug.h 1.25 Broadcom SDK $
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

#ifndef _SOC_DEBUG_H
#define _SOC_DEBUG_H

#include <soc/cmdebug.h>
#include <soc/mdebug.h>

/****************************************************************
 *
 * SOC specific debugging flags
 *
 ****************************************************************/

#if defined(BROADCOM_DEBUG)

/*
 * Unified message format for all modules 
 * In order to use this format _ERR_MSG_MODULE_NAME must be defined 
 * For example in Fabric module the following will be in top of the c file: 
 *  #ifdef _ERR_MSG_MODULE_NAME
 *    #error "_ERR_MSG_MODULE_NAME redefined"
 *  #endif
 *  #define _ERR_MSG_MODULE_NAME FABRIC
 * And in the bottom:
 *  #undef _ERR_MSG_MODULE_NAME
*/
#define _SOC_MSG(string) "%s[%d]%s unit %d: " string "\n", __FILE__, __LINE__, FUNCTION_NAME(), unit
#define _SOC_GEN_DBG(module, lvl, msg)       SOC_DEBUG(lvl | module, msg)
#define _SOC_WARN(stuff)  _SOC_GEN_DBG(_ERR_MSG_MODULE_NAME, SOC_DBG_WARN, stuff)
#define _SOC_ERR(stuff)   _SOC_GEN_DBG(_ERR_MSG_MODULE_NAME, SOC_DBG_ERR, stuff)
#define _SOC_NORM(stuff)  _SOC_GEN_DBG(_ERR_MSG_MODULE_NAME, SOC_DBG_NORMAL, stuff)
#define _SOC_VERB(stuff)  _SOC_GEN_DBG(_ERR_MSG_MODULE_NAME, SOC_DBG_VERBOSE, stuff)
#define _SOC_VVERB(stuff) _SOC_GEN_DBG(_ERR_MSG_MODULE_NAME, SOC_DBG_VVERBOSE, stuff)

#else

#define _SOC_MSG(string) 
#define _SOC_GEN_DBG(module, lvl, msg)   
#define _SOC_WARN(stuff) 
#define _SOC_ERR(stuff)  
#define _SOC_NORM(stuff)  
#define _SOC_VERB(stuff)  
#define _SOC_VVERB(stuff)

#endif   /* defined(BROADCOM_DEBUG) */

#endif  /* !_SOC_DEBUG_H */
