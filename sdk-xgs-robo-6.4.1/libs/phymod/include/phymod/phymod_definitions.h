/*
 *         
 * $Id: phymod_definitions.h,v 1.2.2.12 Broadcom SDK $
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
 *     
 * Phymod external definitions
 *
 */



#ifndef _PHYMOD_DEFINITIONS_H_
#define _PHYMOD_DEFINITIONS_H_

#include <phymod/phymod_system.h>

#define _PHYMOD_MSG(string) "%s[%d]%s: " string "\n", __FILE__, __LINE__, FUNCTION_NAME()


#ifdef PHYMOD_DIAG 
typedef struct enum_mapping_s {
    char        *key;           /* String to match */
    uint32_t    enum_value;     /* Value associated with string */
} enum_mapping_t;
#endif



#define PHYMOD_E_OK PHYMOD_E_NONE

#define PHYMOD_LOCK_TAKE(t) do{\
        if((t->access.bus->mutex_give != NULL) && (t->access.bus->mutex_take != NULL)){\
            PHYMOD_IF_ERR_RETURN(t->access.bus->mutex_take(t->access.user_acc));\
        }\
    }while(0)
 
#define PHYMOD_LOCK_GIVE(t) do{\
        if((t->access.bus->mutex_give != NULL) && (t->access.bus->mutex_take != NULL)){\
            PHYMOD_IF_ERR_RETURN(t->access.bus->mutex_give(t->access.user_acc));\
        }\
    }while(0)

/* Dispatch */
#define PHYMOD_DRIVER_TYPE_GET(p,t) ((*t) = p->type)

/* Functions structure */
#define PHYMOD_IF_ERR_RETURN(A) \
    do {   \
        if ((A) != PHYMOD_E_NONE) \
        {  return A; } \
    } while (0)

#define PHYMOD_RETURN_WITH_ERR(A, B) \
    do {   \
        PHYMOD_DEBUG_ERROR(B);     \
        return A; \
    } while (0)

#define PHYMOD_NULL_CHECK(A) \
    do {   \
        if ((A) == NULL) \
        { PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("null parameter"))); } \
    } while (0)

#define PHYMOD_ERR_VALUE_CHECK(A, VAL) \
    do {   \
        if ((A) == VAL) \
        { PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("invalid value"))); } \
    } while (0)

#endif /*_PHYMOD_DEFINITIONS_H_*/
