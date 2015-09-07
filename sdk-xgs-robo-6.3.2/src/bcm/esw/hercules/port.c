/*
 * $Id: port.c 1.9 Broadcom SDK $
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
 * File:        port.c
 * Purpose:     Hercules port function implementations
 */

#include <soc/defs.h>

#if defined(BCM_HERCULES_SUPPORT) || defined(BCM_HUMV_SUPPORT)|| \
        defined(BCM_CONQUEROR_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)

#include <soc/ptable.h>

#include <bcm/port.h>
#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/hercules.h>

int
bcm_hercules_port_cfg_get(int unit, bcm_port_t port, bcm_port_cfg_t *cfg)
{
    sal_memset(cfg, 0, sizeof(*cfg));
    cfg->pc_cml = PVP_CML_SWITCH;
    cfg->pc_stp_state = PVP_STP_FORWARDING;
    cfg->pc_vlan = 1;
    SOC_PBMP_ASSIGN(cfg->pc_pbm, PBMP_PORT_ALL(unit));

    /*
     * Interpret spanning tree state;  From Daniel Tai:
     *
     * TX_EN in MAC_TXCTRL
     * RX_EN in MAC_RXCTRL
     *
     * Ports may be enabled/disabled by programming the BigMAC.
     *
     * DISABLED  = both TX/RX disabled on MAC
     * BLOCKING  = RX enabled, TX disabled, all tables programmed to
     *             drop execept for CPU packets
     * LISTENING = RX enabled, TX enabled, all tables programmed to
     *             drop execept for CPU packets
     * LEARNING  = N/A
     * FORWARDING = both TX/RX enabled, all tables programmed
     *              normally
     */

    return BCM_E_NONE;
}

int
bcm_hercules_port_cfg_set(int unit, bcm_port_t port, bcm_port_cfg_t *cfg)
{
#if defined(HERCULES_DEBUG_CHECKING)
    /*
     * Check the following are 0:
     *    pc_mirror_ing, pc_vlan, pc_pbm, pc_ut_pbm, pc_l3_enable,
     *    pc_new_pri, pc_remap_pri_en, pc_dse_mode, pc_dscp,
     *    pc_frame_type, pc_ether_type,
     *
     * Issues:
     *    pc_cml, pc_cpu, pc_disc, pc_bpdu_disable, pc_trunk, pc_tgid,
     */
#endif
    /*
     * Settable parameters include:
     *     pc_stp_state:  Get rx/tx enable state from mac registers
     */

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_hercules_port_cfg_init
 * Purpose:
 *	Initialize the port tables according to Initial System
 *	Configuration (see init.c)
 * Parameters:
 *	unit - StrataSwitch unit number.
 *	port - Port number
 *	vd - Initial VLAN data information
 */

int
bcm_hercules_port_cfg_init(int unit, bcm_port_t port, bcm_vlan_data_t *vd)
{
    return BCM_E_NONE;
}

/* Placeholder routines for unsupported functionality */

int
bcm_hercules_port_rate_egress_set(int unit, int port,
				  uint32 kbits_sec,
				  uint32 kbits_burst, uint32 mode)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hercules_port_rate_egress_get(int unit, int port,
				  uint32 *kbits_sec,
				  uint32 *kbits_burst, uint32 *mode)
{
    return BCM_E_UNAVAIL;
}


#endif	/* BCM_HERCULES_SUPPORT */
