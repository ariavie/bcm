/*
 * $Id: packet.c 1.14 Broadcom SDK $
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
 * File:        packet.c
 * Purpose:
 * Requires:    
 */

#include "cmicsim.h"
#include <sal/core/alloc.h>

#if !defined(min)
#define min(a,b)  ((a) < (b) ? (a) : (b))
#endif


int
pcid_add_pkt(pcid_info_t *pcid_info, uint8 *pkt_data, int pkt_len, uint32 *dcbd)
{
    packet_t *newpkt, *pp;
    int      i; 
    int      word_count=8; /* FB/ER 3 + 5 words of HG + PBE */
    
    if (pcid_info->pkt_count < CPU_MAX_PACKET_QUEUE) {
        newpkt = sal_alloc(sizeof(packet_t), "pcid_add_pkt");
        if (newpkt == NULL) {
            return 1;
        }
        newpkt->length = min(pkt_len, PKT_SIZE_MAX);
        newpkt->consum = 0;
        newpkt->next = 0;

#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(pcid_info->unit)) {
            word_count = 13; /* 4 + 7 words of HG + PBE */
        }
#endif /* BCM_TRX_SUPPORT */

        for (i = 0; i < word_count; i++) {
            newpkt->dcbd[i] = dcbd[i];
        }
        memset(newpkt->data, 0, PKT_SIZE_MAX);
        memcpy(newpkt->data, pkt_data, min(pkt_len, PKT_SIZE_MAX));

        sal_mutex_take(pcid_info->pkt_mutex, sal_mutex_FOREVER);
        ++(pcid_info->pkt_count);
        /* Queue current packet on end of list */
        if (!pcid_info->pkt_list) {
            pcid_info->pkt_list = newpkt;
        } else {
            for (pp = pcid_info->pkt_list; pp->next; pp = pp->next)
                ;
            pp->next = newpkt;
        }
        sal_mutex_give(pcid_info->pkt_mutex);

        return 0;
    }

    return 1;
}

