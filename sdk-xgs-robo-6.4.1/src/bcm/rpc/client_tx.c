/*
 * $Id: client_tx.c,v 1.1 Broadcom SDK $
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
 * File:        client_tx.c
 * Purpose:     Client implementation of bcm_tx* functions.
 * Requires:
 *
 */

#include <shared/bsl.h>

#include <bcm/tx.h>
#include <bcm/error.h>

#if defined(BROADCOM_DEBUG)
#include <soc/debug.h>
#include <soc/cm.h>
#define TX_DEBUG(stuff)         LOG_ERROR(BSL_LS_SOC_COMMON, stuff)
#else
#define TX_DEBUG(stuff)
#endif  /* BROADCOM_DEBUG */


/*
 * Function:
 *      bcm_client_tx_init
 * Purpose:
 *      Initialize BCM TX API on client
 * Parameters:
 *      unit - transmission unit
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */

int
bcm_client_tx_init(int unit)
{
    /* Set up TX tunnel receiver and transmitter */
    /* Currently this is done elsewhere */
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_client_tx
 * Purpose:
 *     Transmit a single packet on client
 * Parameters:
 *      unit - transmission unit
 *      pkt - The tx packet structure
 *      cookie - Callback cookie when tx done
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      XGS3 devices cannot dispatch packets to multiple ports. To emulate this
 *      functionality, bcm_tx iterates over the requested bitmaps. The callback,
 *      if applicable, shall be called only once.
 */

int
bcm_client_tx(int unit, bcm_pkt_t *pkt, void *cookie)
{
    /* Tunnel the packet to the remote CPU */
    bcm_cpu_tunnel_mode_t mode = BCM_CPU_TUNNEL_PACKET; /* default */

    if (pkt->call_back != NULL && cookie != NULL) {
        TX_DEBUG(("bcm_tx ERROR:  "
                     "Cookie non-NULL on async tunnel call\n"));
        return BCM_E_PARAM;
    }

    if (pkt->flags & BCM_TX_BEST_EFFORT) {
        mode = BCM_CPU_TUNNEL_PACKET_BEST_EFFORT;
    } else if (pkt->flags & BCM_TX_RELIABLE) {
        mode = BCM_CPU_TUNNEL_PACKET_RELIABLE;
    }

    return bcm_tx_cpu_tunnel(pkt, unit, 0, BCM_CPU_TUNNEL_F_PBMP, mode);
}

/*
 * Function:
 *      bcm_client_tx_array
 * Purpose:
 *      Transmit an array of packets on client
 * Parameters:
 *      unit - transmission unit
 *      pkt - array of pointers to packets to transmit
 *      count - Number of packets in list
 *      all_done_cb - Callback function (if non-NULL) when all pkts trx'd
 *      cookie - Callback cookie.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Can't tunnel an array to the client, so return error.
 */

int
bcm_client_tx_array(int unit, bcm_pkt_t **pkt, int count,
                    bcm_pkt_cb_f all_done_cb, void *cookie)
{
    TX_DEBUG(("bcm_tx_array ERROR:  Cannot tunnel\n"));
    return BCM_E_PARAM;
}

/*
 * Function:
 *      bcm_client_tx_list
 * Purpose:
 *      Transmit a linked list of packets
 * Parameters:
 *      unit - transmission unit
 *      pkt - Pointer to linked list of packets
 *      all_done_cb - Callback function (if non-NULL) when all pkts trx'd
 *      cookie - Callback cookie.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Can't tunnel a list to the client, so return error.
 */

int
bcm_client_tx_list(int unit, bcm_pkt_t *pkt, bcm_pkt_cb_f all_done_cb,
                   void *cookie)
{
    TX_DEBUG(("bcm_tx_list ERROR:  Cannot tunnel\n"));
    return BCM_E_PARAM;
}

/*
 * Function:
 *      bcm_client_tx_pkt_setup
 * Purpose:
 *      Default packet setup routine for transmit on client
 * Parameters:
 *      unit         - Unit on which being transmitted
 *      tx_pkt       - Packet to update
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */

int
bcm_client_tx_pkt_setup(int unit, bcm_pkt_t *tx_pkt)
{
    return BCM_E_NONE;
}
