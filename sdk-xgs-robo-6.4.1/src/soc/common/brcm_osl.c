/*
 * $Id: brcm_osl.c,v 1.7 Broadcom SDK $
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
 * Linux OS Independent Layer
 */

#define	BRCM_OSL

#include <shared/bsl.h>

#include <shared/et/typedefs.h>
#include <shared/et/osl.h>
#include <shared/et/bcmenetrxh.h>
#include <soc/ethdma.h>
#if defined(KEYSTONE) || defined(ROBO_4704) || defined(IPROC_CMICD)

void*
et_pktget(void *dev, int chan, uint len, bool send)
{
    int unit = (int)dev;
    eth_dv_t *dv;

    if (send) {
        return ((void *)NULL);
    }

    dv = et_soc_rx_chain_get(unit, chan, 0);

    return ((void*) dv);
}

void
et_pktfree(void* dev, void *p)
{
    int unit = (int)dev;
    eth_dv_t *dv = (eth_dv_t *)p;

    switch (dv->dv_op) {
        case DV_TX:
            if (dv->dv_done_chain) {
                dv->dv_done_chain(dv->dv_unit, dv);
            }
            break;

        case DV_RX:
            if (!dv->dv_length) {
                dv->dv_flags |= RXF_RXER;
            }

            /* free allocated packet buffer */
            if (dv->dv_done_packet) {
                dv->dv_flags |= RXF_RXER;
                dv->dv_done_packet(dv->dv_unit, dv, dv->dv_dcb);
            } else {
                soc_cm_sfree(unit, (void *)dv->dv_dcb->dcb_vaddr);
                soc_eth_dma_dv_free(unit, (eth_dv_t *)dv);
            }
            break;

        default:
            LOG_CLI((BSL_META_U(unit,
                                "ERROR: unit %d unknown dma op %d\n"), unit, dv->dv_op));
            assert(0);
            return;
    }
}

#endif /* defined(KEYSTONE) || defined(ROBO_4704) || defined(IPROC_CMICD) */
