/*
 * $Id: link.h 1.5 Broadcom SDK $
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
 * Common internal definitions for BCM Linkscan module
 */

#ifndef _BCM_INT_LINK_H_
#define _BCM_INT_LINK_H_

#include <sal/types.h>
#include <bcm/types.h>
#include <bcm/debug.h>

/*
 * Debug Output Macros
 */
#define LINK_DEBUG(flags, stuff)  BCM_DEBUG(flags | BCM_DBG_LINK, stuff)
#define LINK_OUT(stuff)           LINK_DEBUG(BCM_DBG_LINK, stuff)
#define LINK_WARN(stuff)          LINK_DEBUG(BCM_DBG_WARN, stuff)
#define LINK_ERR(stuff)           LINK_DEBUG(BCM_DBG_ERR, stuff)
#define LINK_VERB(stuff)          LINK_DEBUG(BCM_DBG_VERBOSE, stuff)

/*
 * Driver specific routines
 *
 * (See description for routines below)
 */
typedef struct _bcm_ls_driver_s {
    void (*ld_hw_interrupt)(int, bcm_pbmp_t*);
    int  (*ld_port_link_get)(int, bcm_port_t, int, int*);
    int  (*ld_internal_select)(int, bcm_port_t);
    int  (*ld_update_asf)(int, bcm_port_t, int, int, int);
    int  (*ld_trunk_sw_failover_trigger)(int, bcm_pbmp_t, bcm_pbmp_t);
} _bcm_ls_driver_t;

/*
 * Driver specific routines description
 *
 * Function:
 *     ld_hw_interrupt
 * Purpose:
 *     Routine handler for hardware linkscan interrupt.
 * Parameters:
 *     unit - Device unit number
 *     pbmp - (OUT) Returns bitmap of ports that require hardware re-scan
 *
 *
 * Function:
 *     ld_port_link_get
 * Purpose:
 *     Return current PHY up/down status.
 * Parameters:
 *     unit - Device unit number
 *     port - Device port number
 *     hw   - If TRUE, assume hardware linkscan is active and use it
 *              to reduce PHY reads.
 *            If FALSE, do not use information from hardware linkscan.
 *     up   - (OUT) TRUE for link up, FALSE for link down.
 *
 *
 * Function:
 *     ld_internal_select
 * Purpose:
 *     Select the source of the CMIC link status interrupt
 *     to be the Internal Serdes on given port.
 * Parameters:
 *     unit - Device unit number
 *     port - Device port number
 *
 *
 * Function:
 *     ld_update_asf
 * Purpose:
 *     Update Alternate Store and Forward parameters for a port.
 * Parameters:
 *     unit   - Device unit number
 *     port   - Device port number
 *     linkup - port link state (0=down, 1=up)
 *     speed  - port speed
 *     duplex - port duplex (0=half, 1=full)
 *
 *
 * Function:
 *     ld_trunk_sw_failover_trigger
 * Purpose:
 *     Remove specified ports with link down from trunks.
 * Parameters:
 *     unit        - Device unit number
 *     pbmp_active - Bitmap of ports
 *     pbmp_status - Bitmap of port status
 */

extern int _bcm_linkscan_init(int unit, _bcm_ls_driver_t *driver);

extern int _bcm_link_get(int unit, bcm_port_t port, int *link);
extern int _bcm_link_force(int unit, bcm_port_t port, int force, int link);

#ifdef BCM_WARM_BOOT_SUPPORT
int bcm_linkscan_sync(int unit, int sync);
#endif
#endif /* _BCM_INT_LINK_H_ */
