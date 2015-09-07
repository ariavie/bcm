/*
 * $Id: mcast.c,v 1.1 Broadcom SDK $
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
#include <shared/bsl.h>

#include <soc/robo.h>
#include <soc/drv.h>
#include <soc/types.h>
#include <soc/debug.h>
#include <soc/mcast.h>

/*
 *  Function : drv_harrier_mcast_bmp_get
 *
 *  Purpose :
 *      Get the multicast member ports from multicast entry
 *
 *  Parameters :
 *      unit    :   unit id
 *      entry   :   entry data pointer 
 *      bmp     :   group port member
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *
 */
int  
drv_harrier_mcast_bmp_get(int unit, uint32 *entry, soc_pbmp_t *bmp)
{
    uint32  entry_index, fld_v32;
    marl_pbmp_entry_t  mcast_entry;
    uint64 fld_v64;

    assert(entry);
    assert(bmp);

    /* get the multicast id */
    SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_GET
        (unit, DRV_MEM_MARL, DRV_MEM_FIELD_DEST_BITMAP, entry, &entry_index));

    /* get multicast group entry */
    SOC_IF_ERROR_RETURN(DRV_MEM_READ
        (unit, DRV_MEM_MCAST, entry_index, 1, (uint32 *)&mcast_entry));

    if (SOC_INFO(unit).port_num > 32) {
        SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_GET
            (unit, DRV_MEM_MCAST, DRV_MEM_FIELD_DEST_BITMAP, 
            (uint32 *)&mcast_entry, (uint32 *)&fld_v64));

        soc_robo_64_val_to_pbmp(unit, bmp, fld_v64);
    } else {
        SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_GET
            (unit, DRV_MEM_MCAST, DRV_MEM_FIELD_DEST_BITMAP, 
            (uint32 *)&mcast_entry, (uint32 *)&fld_v32));

        SOC_PBMP_WORD_SET(*bmp, 0, fld_v32);
    }
    
    LOG_INFO(BSL_LS_SOC_L2TABLE,
             (BSL_META_U(unit,
                         "drv_mcast_bmp_get: unit %d, bmp = 0x%x 0x%x\n"),
              unit, SOC_PBMP_WORD_GET(*bmp, 0), SOC_PBMP_WORD_GET(*bmp, 1)));

    return SOC_E_NONE;
}

 /*
 *  Function : drv_harrier_mcast_bmp_set
 *
 *  Purpose :
 *      Set the multicast member ports from multicast entry
 *
 *  Parameters :
 *      unit    :   unit id
 *      entry   :   entry data pointer 
 *      bmp     :   group port member
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *
 */
int  
drv_harrier_mcast_bmp_set(int unit, uint32 *entry, soc_pbmp_t bmp, uint32 flag)
{
    int loop, mcindex = -1;
    uint32  index, entry_index = 0;
    marl_pbmp_entry_t  mcast_entry;
    uint32  fld_value = 0;
    uint64  fld_v64;
    
    assert(entry);
    LOG_INFO(BSL_LS_SOC_L2TABLE,
             (BSL_META_U(unit,
                         "drv_mcast_bmp_set: unit %d, bmp = 0x%x 0x%x flag %x\n"),
              unit, SOC_PBMP_WORD_GET(bmp, 0), SOC_PBMP_WORD_GET(bmp, 1), flag));

	SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_GET
        (unit, DRV_MEM_MARL, DRV_MEM_FIELD_DEST_BITMAP, entry, &index));

    if( flag & DRV_MCAST_INDEX_ADD) {
        if (index){
            mcindex = index;
        } else {
            /* get a free index for creating */
            for (loop = 0; loop < L2MC_SIZE(unit); loop++) {
                if (!L2MC_USED_ISSET(unit, loop)) {
                    mcindex = loop;
                    break;
                }
            }
    
            if (mcindex == -1){
                return SOC_E_FULL;
            }
        }

        /* set mc-index field */
        fld_value = (uint32)mcindex;
        /* calculate entry index */
        entry_index = (uint32)mcindex;
        SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_SET
            (unit, DRV_MEM_MARL, DRV_MEM_FIELD_DEST_BITMAP, entry, &fld_value));

        /* Insert this address into arl table. */
        SOC_IF_ERROR_RETURN(DRV_MEM_INSERT
            (unit, DRV_MEM_ARL, entry, 
            (DRV_MEM_OP_BY_HASH_BY_MAC | DRV_MEM_OP_BY_HASH_BY_VLANID | 
            DRV_MEM_OP_REPLACE)));

        L2MC_USED_SET(unit, mcindex);
        index = mcindex;
    }
    entry_index = index;
    /* get multicast group entry */
    SOC_IF_ERROR_RETURN(DRV_MEM_READ
        (unit, DRV_MEM_MCAST, entry_index, 1, (uint32 *)&mcast_entry));

    /* set valid bit depend on bmp */
    if (SOC_INFO(unit).port_num > 32) {
        soc_robo_64_pbmp_to_val(unit, &bmp, &fld_v64);
        SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_SET
            (unit, DRV_MEM_MCAST, DRV_MEM_FIELD_DEST_BITMAP, 
            (uint32 *)&mcast_entry, (uint32 *)&fld_v64));
    } else {
        fld_value = SOC_PBMP_WORD_GET(bmp, 0);
        SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_SET
            (unit, DRV_MEM_MCAST, DRV_MEM_FIELD_DEST_BITMAP, 
            (uint32 *)&mcast_entry, (uint32 *)&fld_value));
    }

	SOC_IF_ERROR_RETURN(DRV_MEM_WRITE
        (unit, DRV_MEM_MCAST, entry_index, 1, (uint32 *)&mcast_entry));

    if( flag & DRV_MCAST_INDEX_REMOVE) {
        /* clear MC used info-base */
        L2MC_USED_CLR(unit, index);
    }

    return SOC_E_NONE;
}
