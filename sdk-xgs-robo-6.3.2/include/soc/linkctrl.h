/* 
 * $Id: linkctrl.h 1.4 Broadcom SDK $
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
 * File:        linkctrl.h
 * Purpose:     Hardware Linkscan Control module
 */

#ifndef   _SOC_LINKCTRL_H_
#define   _SOC_LINKCTRL_H_

#include <soc/types.h>


/*
 * Provides port number defined in PC MIIM registers for
 * write/read of a port PHY link status.
 * Port values in MIIM are fixed based on the port physical
 * block type/instance/index.
 */
typedef struct soc_linkctrl_port_info_s {
    int     block_type;
    int     block_number;
    int     block_index;
    int     port;
} soc_linkctrl_port_info_t;


/*
 * Driver specific routines
 *
 * (See description for routines below)
 */
typedef struct soc_linkctrl_driver_s {
    soc_linkctrl_port_info_t  *port_info;  /* null indicates no mapping */
    int  (*ld_linkscan_hw_init)(int);
    int  (*ld_linkscan_config)(int, pbmp_t, pbmp_t);
    int  (*ld_linkscan_pause)(int);
    int  (*ld_linkscan_continue)(int);
    int  (*ld_update)(int);
    int  (*ld_hw_link_get)(int, pbmp_t *);
} soc_linkctrl_driver_t;

/*
 * Driver specific routines description
 *
 * Function:
 *     ld_linkscan_hw_init
 * Purpose:
 *     Initialize device specific HW linkscan.
 * Parameters:
 *     unit - Device unit number
 *
 *
 * Function:
 *     ld_linkscan_config
 * Purpose:
 *     Set ports to hardware linkscan.
 * Parameters:
 *     unit       - Device unit number
 *     mii_pbm    - Port bitmap of ports to scan with MIIM registers
 *     direct_pbm - Port bitmap of ports to scan using NON MII
 *
 *
 * Function:
 *     ld_linkscan_pause
 * Purpose:
 *     Pause hardware link scanning, without disabling it.
 *     This call is used to pause scanning temporarily.
 * Parameters:
 *     unit - Device unit number
 *
 *
 * Function:
 *     ld_linkscan_continue
 * Purpose:
 *     Continue hardware link scanning after it has been paused.
 * Parameters:
 *     unit - Device unit number
 *
 *
 * Function:    
 *     ld_update
 * Purpose:
 *     Update the forwarding state in device.
 * Parameters:  
 *     unit - Device unit number
 */


extern int soc_linkctrl_init(int unit, soc_linkctrl_driver_t *driver);
extern int soc_linkctrl_deinit(int unit);

/* These are called within src/bcm/ source files */
extern int soc_linkctrl_link_fwd_set(int unit, pbmp_t fwd);
extern int soc_linkctrl_link_fwd_get(int unit, pbmp_t *fwd);
extern int soc_linkctrl_link_mask_set(int unit, pbmp_t mask);
extern int soc_linkctrl_link_mask_get(int unit, pbmp_t *mask);

/* These are called within src/soc/ and src/bcm/ source files */
extern int soc_linkctrl_linkscan_register(int unit, void (*f)(int));
extern int soc_linkctrl_linkscan_hw_init(int unit);
extern int soc_linkctrl_linkscan_config(int unit, pbmp_t hw_mii_pbm,
                                        pbmp_t hw_direct_pbm);
extern int soc_linkctrl_linkscan_pause(int unit);
extern int soc_linkctrl_linkscan_continue(int unit);

extern int soc_linkctrl_miim_port_get(int unit, soc_port_t port,
                                      int *miim_port);
extern int soc_linkctrl_hw_link_get(int unit, soc_pbmp_t *hw_link);
#endif /* _SOC_LINKCTRL_H_ */
