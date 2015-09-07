/*
 *         
 * $Id: phymod.c,v 1.2.2.13 Broadcom SDK $
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
 */

#include <phymod/phymod.h>
#include <phymod/phymod_dispatch.h>
#include <phymod/phymod_acc.h>

int phymod_core_probe(const phymod_access_t* access, phymod_dispatch_type_t* type)
{
    uint32_t is_identified;
    phymod_dispatch_type_t idx;
    phymod_core_access_t core;
    uint32_t core_id;
    uint32_t id2;
    uint32_t id3;
    int rv;
    int ioerr = 0;
        
    /*parameters validation*/
    if(PHYMOD_E_OK != phymod_access_t_validate(access)) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("access validation failed")));
    }

    if(type == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("type NULL parameter")));
    }
    ioerr += PHYMOD_BUS_READ(access, 2, &id2);
    ioerr += PHYMOD_BUS_READ(access, 3, &id3);
    if(ioerr != 0){
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_IO, (_PHYMOD_MSG("error reading core identification registers")));
    }
    core_id = ((id2 & 0xffff) << 16) + (id3 & 0xffff);

    /*Identify phy type & driver*/
    PHYMOD_MEMCPY(&core.access, access, sizeof(core.access));
    core.type = phymodDispatchTypeCount;
    is_identified = 0;
    for(idx = 0 ; idx < phymodDispatchTypeCount ; idx++) {
        if(NULL != __phymod__dispatch__[idx]->f_phymod_core_identify) {
            rv = __phymod__dispatch__[idx]->f_phymod_core_identify(&core, core_id, &is_identified);
            if (rv == PHYMOD_E_NONE && is_identified) {
                (*type) = idx;
                break;
            }
        }
    }

    if(idx == phymodDispatchTypeCount) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INIT, (_PHYMOD_MSG("identification failed")));
    }
         
    return PHYMOD_E_NONE;
}



