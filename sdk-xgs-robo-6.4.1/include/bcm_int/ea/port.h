/*
 * $Id: port.h,v 1.10 Broadcom SDK $
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
 * File:     port.h
 * Purpose:
 *
 */
#ifndef _BCM_INT_EA_PORT_H
#define _BCM_INT_EA_PORT_H

#include <bcm/types.h>
#include <bcm/port.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/ea/tk371x/event.h>
#include <soc/ea/tk371x/CtcMiscApi.h>
#include <soc/ea/tk371x/TkOnuApi.h>
#include <soc/ea/tk371x/TkTmApi.h>

#define _BCM_EA_PORT_DEBUG		0
#define _BCM_EA_PORT_SUPPORTED	0

extern int _bcm_ea_port_detach(int unit);
extern int _bcm_tk371x_port_selective_get(
		int unit, bcm_port_t port, bcm_port_info_t *info);
extern int _bcm_ea_port_autoneg_get(
	    int unit, bcm_port_t port, int *autoneg);
extern int _bcm_ea_port_autoneg_set(
	    int unit, bcm_port_t port, int autoneg);
extern int _bcm_ea_port_config_get(
	    int unit,bcm_port_config_t *config);
extern int _bcm_ea_port_control_get(
	    int unit, bcm_port_t port, bcm_port_control_t type, int *value);
extern int _bcm_ea_port_control_set(
	    int unit, bcm_port_t port, bcm_port_control_t type, int value);
extern int _bcm_ea_port_duplex_get(
	    int unit, bcm_port_t port, int *duplex);
extern int  _bcm_ea_port_duplex_set(
	    int unit, bcm_port_t port, int duplex);
extern int _bcm_ea_port_enable_get(
	    int unit, bcm_port_t port, int *enable);
extern int _bcm_ea_port_enable_set(
	    int unit, bcm_port_t port, int enable);
extern int _bcm_ea_port_frame_max_get(
	    int unit, bcm_port_t port, int *size);
extern int _bcm_ea_port_frame_max_set(
	    int unit, bcm_port_t port, int size);
extern int _bcm_ea_port_info_get(
	    int unit, bcm_port_t port, bcm_port_info_t *info);
extern int _bcm_ea_port_info_restore(
	    int unit, bcm_port_t port, bcm_port_info_t *info);
extern int _bcm_ea_port_info_save(
	    int unit, bcm_port_t port, bcm_port_info_t *info);
/* Get or set multiple port characteristics. */
extern int _bcm_ea_port_info_set(
	    int unit, bcm_port_t port, bcm_port_info_t *info);
extern int _bcm_ea_port_init(int units);
extern int _bcm_ea_port_learn_get(
		int unit, bcm_port_t port, uint32 *flags);
extern int _bcm_ea_port_learn_modify(
	    int unit, bcm_port_t port, uint32 add, uint32 remove);
extern int _bcm_ea_port_learn_set(
	    int unit, bcm_port_t port, uint32 flags);
extern int _bcm_ea_port_link_status_get(
	    int unit, bcm_port_t port, int *status);
extern int _bcm_ea_port_phy_control_get(
	    int unit, bcm_port_t port, bcm_port_phy_control_t type, uint32 *value);
extern int _bcm_ea_port_phy_control_set(
	    int unit, bcm_port_t port, bcm_port_phy_control_t type, uint32 value);
extern int _bcm_ea_port_speed_get(
	    int unit, bcm_port_t port, int *speed);
extern int _bcm_ea_port_speed_set(
	    int unit, bcm_port_t port, int speed);
extern int _bcm_ea_port_link_status_get(
		int unit,
		bcm_port_t port, int *up);
extern int _bcm_ea_port_advert_get(
		int unit, bcm_port_t port, bcm_port_abil_t *ability_mask);
extern int _bcm_ea_port_advert_remote_get(
		int unit, bcm_port_t port, bcm_port_abil_t *ability_mask);

#endif /* _BCM_INT_EA_PORT_H */
