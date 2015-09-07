/*
 * $Id: debug.h 1.14.2.1 Broadcom SDK $
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
 */

#ifndef __BCM_DEBUG_EX_H__
#define __BCM_DEBUG_EX_H__

#include <bcm/debug.h>


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
#define _BCM_MSG(string) "%s[%d]%s unit %d: " string "\n", __FILE__, __LINE__, FUNCTION_NAME(), unit
#define _BCM_MSG_NO_UNIT(string) "%s[%d]%s: " string "\n", __FILE__, __LINE__, FUNCTION_NAME()
#define _BCM_GEN_DBG(module, lvl, msg)       BCM_DEBUG(lvl | module, msg)
#define _BCM_WARN(stuff)  _BCM_GEN_DBG(_ERR_MSG_MODULE_NAME, BCM_DBG_WARN, stuff)
#define _BCM_ERR(stuff)   _BCM_GEN_DBG(_ERR_MSG_MODULE_NAME, BCM_DBG_ERR, stuff)
#define _BCM_VERB(stuff)  _BCM_GEN_DBG(_ERR_MSG_MODULE_NAME, BCM_DBG_VERBOSE, stuff)
#define _BCM_VVERB(stuff) _BCM_GEN_DBG(_ERR_MSG_MODULE_NAME, BCM_DBG_VVERBOSE, stuff)

#else
#define _BCM_MSG(string) 
#define _BCM_MSG_NO_UNIT(string)
#define _BCM_GEN_DBG(module, lvl, msg)   
#define _BCM_WARN(stuff) 
#define _BCM_ERR(stuff) 
#define _BCM_VERB(stuff) 
#define _BCM_VVERB(stuff)          
#endif

/*Single point of exit*/
#define BCM_EXIT goto exit

/* Must appear at each function right after parameters definition */
#define BCM_INIT_FUNC_DEFS \
    int _rv = BCM_E_NONE, _lock_taken = 0; \
    (void)_lock_taken; \
    _BCM_VVERB((_BCM_MSG("enter"))); 

#define BCM_INIT_FUNC_DEFS_NO_UNIT \
    int _rv = BCM_E_NONE, _lock_taken = 0; \
    (void)_lock_taken; \
    _BCM_VVERB((_BCM_MSG_NO_UNIT("enter"))); 

#define BCM_FUNC_ERROR \
    BCM_FAILURE(_rv)

#define BCM_FUNC_RETURN \
    _BCM_VVERB((_BCM_MSG("exit"))); \
    return _rv;

#define BCM_RETURN_VAL_EXIT(rv_override) \
    _rv = rv_override; \
    BCM_EXIT;

#define BCM_FUNC_RETURN_VOID \
    _BCM_VVERB((_BCM_MSG("exit")));\
        COMPILER_REFERENCE(_rv);

#define BCM_FUNC_RETURN_NO_UNIT \
    _BCM_VVERB((_BCM_MSG_NO_UNIT("exit"))); \
    return _rv;

#define BCM_FUNC_RETURN_VOID_NO_UNIT \
    _BCM_VVERB((_BCM_MSG_NO_UNIT("exit")));\
    COMPILER_REFERENCE(_rv);

#define BCM_IF_ERR_EXIT(_rc) \
      do { \
        int __err__rc = _rc; \
        if(__err__rc != BCM_E_NONE) { \
            _BCM_ERR((_BCM_MSG("%s"),bcm_errmsg(__err__rc))); \
            _rv = __err__rc; \
            BCM_EXIT; \
        } \
      } while(0)

#define BCM_IF_ERR_NOT_UNAVAIL_EXIT(_rc) \
      do { \
        int __err__rc = _rc; \
        if(__err__rc != BCM_E_NONE && __err__rc != BCM_E_UNAVAIL) { \
            _BCM_ERR((_BCM_MSG("%s"),bcm_errmsg(__err__rc))); \
            _rv = __err__rc; \
            BCM_EXIT; \
        } \
      } while(0)

#define BCM_IF_ERR_EXIT_NO_UNIT(_rc) \
      do { \
        int __err__rc = _rc; \
        if(__err__rc != BCM_E_NONE) { \
            _BCM_ERR((_BCM_MSG_NO_UNIT("%s"),bcm_errmsg(__err__rc))); \
            _rv = __err__rc; \
            BCM_EXIT; \
        } \
      } while(0)

#define BCM_IF_ERR_EXIT_MSG(_rc, stuff) \
      do { \
        int __err__rc = _rc; \
        if(__err__rc != BCM_E_NONE) { \
            _BCM_ERR(stuff); \
            _rv = __err__rc; \
            BCM_EXIT; \
        } \
      } while(0)

#define BCM_ERR_EXIT_MSG(_rc, stuff) \
      do { \
            _BCM_ERR(stuff); \
            _rv = _rc; \
            BCM_EXIT; \
      } while(0)
      
#define BCM_ERR_EXIT_NO_MSG(_rc) \
      do { \
            _rv = _rc; \
            BCM_EXIT; \
      } while(0)

#define BCM_NULL_CHECK(arg) \
    do {   \
        if ((arg) == NULL) \
        { BCM_ERR_EXIT_MSG(BCM_E_PARAM,(_BCM_MSG("null parameter" ))); } \
    } while (0)

#define BCM_NULL_CHECK_NO_UNIT(arg) \
    do {   \
        if ((arg) == NULL) \
        { BCM_ERR_EXIT_MSG(BCM_E_PARAM,(_BCM_MSG_NO_UNIT("null parameter" ))); } \
    } while (0)

#define BCM_ALLOC(pointer, mem_size, alloc_comment)   \
  do {   \
      if((pointer) == NULL) { \
          (pointer) = sal_alloc((mem_size), (alloc_comment)); \
      } else {\
          BCM_ERR_EXIT_MSG(BCM_E_PARAM,(_BCM_MSG_NO_UNIT("attempted to allocate to a non NULL pointer: %s"), (alloc_comment))); \
      } \
  } while (0)

#define BCM_FREE(pointer)   \
  do {   \
      if((pointer) != NULL) { \
          sal_free((void*)(pointer)); \
          (pointer) = NULL; \
      } \
  } while (0)

# define BCM_VERIFY(expr) ((void)0)

#endif /* __BCM_DEBUG_EX_H__ */
