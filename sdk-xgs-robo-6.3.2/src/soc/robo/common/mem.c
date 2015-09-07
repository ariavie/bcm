/*
 * $Id: mem.c 1.4 Broadcom SDK $
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
#include <sal/core/libc.h>
#include <assert.h>
#include <soc/mem.h>
#include <soc/drv.h>
#include <soc/debug.h>

#include <soc/error.h>

/*
 *  Function : drv_mem_cache_get
 *
 *  Purpose :
 *    Get the status of caching is enabled for a specified memory.
 *
 *  Parameters :
 *      unit    :   unit id
 *      mem     :   memory type.
 *      enable  :   status of cacheing enable or not.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *      Only for VLAN and SEC_MAC memory now.
 *
 */

int
drv_mem_cache_get(int unit,
                  uint32 mem, uint32 *enable)
{
    int mem_id;
    assert(SOC_UNIT_VALID(unit));
    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }

    switch (mem) {
        case DRV_MEM_VLAN:
            mem_id = INDEX(VLAN_1Qm);
            break;
        default:
            *enable = FALSE;
            return SOC_E_NONE;
    }
    assert(SOC_MEM_IS_VALID(unit, mem_id));
    if (!SOC_MEM_IS_VALID(unit, mem_id))  {
        return SOC_E_PARAM;
    }
    /* 
    For robo-SDK, we defined one block for each memory types.
    */
    *enable = (SOC_MEM_STATE(unit, mem_id).cache[0] != NULL) ? 1 : 0; 
    return SOC_E_NONE;
}

/*
 *  Function : drv_mem_cache_set
 *
 *  Purpose :
 *    Set the status of caching is enabled for a specified memory.
 *
 *  Parameters :
 *      unit    :   unit id
 *      mem     :   memory type.
 *      enable  :   status of cacheing enable or not.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *      Only for VLAN and SEC_MAC memory now.
 *
 */

int
drv_mem_cache_set(int unit,
                  uint32 mem, uint32 enable)
{
    soc_memstate_t *memState;
    int index_cnt;
    int cache_size, vmap_size;
    uint8 *vmap;
    uint32 *cache;
    int mem_id;
    int     count, i;
    int entry_size;

    assert(SOC_UNIT_VALID(unit));
    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }
    switch (mem) {
        case DRV_MEM_VLAN:
            mem_id = INDEX(VLAN_1Qm);
            count = 1;
            break;
        default:
            return SOC_E_UNAVAIL;
    }
    for (i = 0; i < count; i++) {
        assert(SOC_MEM_IS_VALID(unit, mem_id));
        if (!SOC_MEM_IS_VALID(unit, mem_id))  {
            return SOC_E_PARAM;
        }
        entry_size = soc_mem_entry_bytes(unit, mem_id);
        memState = &SOC_MEM_STATE(unit, mem_id);
        index_cnt = soc_robo_mem_index_max(unit, mem_id) + 1;
        cache_size = index_cnt * entry_size;
        vmap_size = (index_cnt + 7) / 8;

        soc_cm_debug(DK_SOCMEM,
                "drv_mem_cache_set: unit %d memory %s %sable\n",
                unit, (SOC_IS_ROBO(unit)?SOC_ROBO_MEM_UFNAME(unit, mem_id):""),
                enable ? "en" : "dis");

        /* 
        For robo-SDK, we defined one block for each memory types.
        */
        /*
         * Turn off caching if currently enabled.
         */

        cache = memState->cache[0];
        vmap = memState->vmap[0];

        /* Zero before free to prevent potential race condition */

        if (cache != NULL) {
            memState->cache[0] = NULL;
            memState->vmap[0] = NULL;

            sal_free(cache);
            sal_free(vmap);
        }

        if (!enable) {
            return SOC_E_NONE;
        }

        MEM_LOCK(unit, mem_id);

        /* Allocate new cache */

        if ((cache = sal_alloc(cache_size, "table-cache")) == NULL) {
            MEM_UNLOCK(unit, mem_id);
            return SOC_E_MEMORY;
        }

        if ((vmap = sal_alloc(vmap_size, "table-vmap")) == NULL) {
            sal_free(cache);
            MEM_UNLOCK(unit, mem_id);
            return SOC_E_MEMORY;
        }

        sal_memset(vmap, 0, vmap_size);

        /* Set memState->cache last to avoid race condition */

        soc_cm_debug(DK_SOCMEM,
                     "drv_mem_cache_set: cache=%p size=%d vmap=%p\n",
                     (void *)cache, cache_size, (void *)vmap);

        memState->vmap[0] = vmap;
        memState->cache[0] = cache;

        MEM_UNLOCK(unit, mem_id);

        mem_id++;
    }

    return SOC_E_NONE;
}




