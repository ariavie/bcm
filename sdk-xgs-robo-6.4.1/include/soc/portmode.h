/*
 * $Id: portmode.h,v 1.37 Broadcom SDK $
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

#ifndef _SOC_PORTMODE_H
#define _SOC_PORTMODE_H

#include <shared/portmode.h>
#include <shared/phyconfig.h>

#include <soc/defs.h>

/*
 * Defines:
 *      SOC_PM_*
 * Purpose:
 *      Defines for port modes.
 */
/* Port Speeds */
#define SOC_PM_10MB_HD          _SHR_PM_10MB_HD
#define SOC_PM_10MB_FD          _SHR_PM_10MB_FD
#define SOC_PM_100MB_HD         _SHR_PM_100MB_HD
#define SOC_PM_100MB_FD         _SHR_PM_100MB_FD
#define SOC_PM_1000MB_HD        _SHR_PM_1000MB_HD
#define SOC_PM_1000MB_FD        _SHR_PM_1000MB_FD
#define SOC_PM_2500MB_HD        _SHR_PM_2500MB_HD
#define SOC_PM_2500MB_FD        _SHR_PM_2500MB_FD
#define SOC_PM_3000MB_HD        _SHR_PM_3000MB_HD
#define SOC_PM_3000MB_FD        _SHR_PM_3000MB_FD
#define SOC_PM_10GB_HD          _SHR_PM_10GB_HD
#define SOC_PM_10GB_FD          _SHR_PM_10GB_FD
#define SOC_PM_12GB_HD          _SHR_PM_12GB_HD
#define SOC_PM_12GB_FD          _SHR_PM_12GB_FD
#define SOC_PM_13GB_HD          _SHR_PM_13GB_HD
#define SOC_PM_13GB_FD          _SHR_PM_13GB_FD
#define SOC_PM_16GB_HD          _SHR_PM_16GB_HD
#define SOC_PM_16GB_FD          _SHR_PM_16GB_FD
#define SOC_PM_SPEED_ALL        _SHR_PM_SPEED_ALL

/* Pause Modes */
#define SOC_PM_PAUSE_TX         _SHR_PM_PAUSE_TX
#define SOC_PM_PAUSE_RX         _SHR_PM_PAUSE_RX
#define SOC_PM_PAUSE_ASYMM      _SHR_PM_PAUSE_ASYMM

/* Interface Types */
#define SOC_PM_TBI              _SHR_PM_TBI
#define SOC_PM_MII              _SHR_PM_MII
#define SOC_PM_GMII             _SHR_PM_GMII
#define SOC_PM_SGMII            _SHR_PM_SGMII
#define SOC_PM_XGMII            _SHR_PM_XGMII

/* Loopback Mode */
#define SOC_PM_LB_MAC           _SHR_PM_LB_MAC
#define SOC_PM_LB_PHY           _SHR_PM_LB_PHY
#define SOC_PM_LB_NONE          _SHR_PM_LB_NONE

/* Autonegotiation */
#define SOC_PM_AN               _SHR_PM_AN

/* Combo port */
#define SOC_PM_COMBO            _SHR_PM_COMBO

#define SOC_PM_PAUSE            _SHR_PM_PAUSE

#define SOC_PM_10MB             _SHR_PM_10MB
#define SOC_PM_100MB            _SHR_PM_100MB
#define SOC_PM_1000MB           _SHR_PM_1000MB
#define SOC_PM_2500MB           _SHR_PM_2500MB
#define SOC_PM_3000MB           _SHR_PM_3000MB
#define SOC_PM_10GB             _SHR_PM_10GB
#define SOC_PM_12GB             _SHR_PM_12GB
#define SOC_PM_13GB             _SHR_PM_13GB
#define SOC_PM_16GB             _SHR_PM_16GB

#define SOC_PM_SPEED_MAX(m)     _SHR_PM_SPEED_MAX(m)
#define SOC_PM_SPEED(s)         _SHR_PM_SPEED(s)

#define SOC_PM_FD               _SHR_PM_FD
#define SOC_PM_HD               _SHR_PM_HD

/*
 * Defines:
 *      SOC_PM_ABILITY_[MAC/PHY]
 * Purpose:
 *      Defines a mask of modes that are exclusive to the MAC or
 *      PHY drivers. To determine the ability of a port, the 
 *      ability masks for the MAC and PHY are ANDed together, and then
 *      ORed with the MAC/PHY ability bits selected by these masks.
 */

#define SOC_PM_ABILITY_PHY      (SOC_PM_LB_PHY|SOC_PM_AN|SOC_PM_COMBO)
#define SOC_PM_ABILITY_MAC      (SOC_PM_LB_MAC)

typedef _shr_port_mode_t soc_port_mode_t;

typedef _shr_phy_config_t soc_phy_config_t;

/*
 * Typedef:
 *      soc_port_if_t, soc_port_ms_t, soc_port_duplex_t
 * Purpose:
 *      Defines interface type between MAC and PHY.
 *      Defines master/slave mode of a port (PHY).
 *      Defines duplexity of a port
 * Notes:
 *      Must be kept in sync with _SHR_PORT_IF_xxx and BCM_PORT_IF_xxx.
 */

typedef _shr_port_if_t soc_port_if_t;
#define SOC_PORT_IF_NOCXN   _SHR_PORT_IF_NOCXN
#define SOC_PORT_IF_NULL    _SHR_PORT_IF_NULL
#define SOC_PORT_IF_MII     _SHR_PORT_IF_MII
#define SOC_PORT_IF_GMII    _SHR_PORT_IF_GMII
#define SOC_PORT_IF_SGMII   _SHR_PORT_IF_SGMII
#define SOC_PORT_IF_TBI     _SHR_PORT_IF_TBI
#define SOC_PORT_IF_XGMII   _SHR_PORT_IF_XGMII
#define SOC_PORT_IF_XAUI    _SHR_PORT_IF_XGMII
/*Special XAUI IF for DNX since the legacy directed to XGMII*/
#define SOC_PORT_IF_DNX_XAUI _SHR_PORT_IF_XAUI
#define SOC_PORT_IF_RGMII   _SHR_PORT_IF_RGMII
#define SOC_PORT_IF_RvMII   _SHR_PORT_IF_RvMII
#define SOC_PORT_IF_SFI     _SHR_PORT_IF_SFI
#define SOC_PORT_IF_XFI     _SHR_PORT_IF_XFI
#define SOC_PORT_IF_KR      _SHR_PORT_IF_KR
#define SOC_PORT_IF_KR4     _SHR_PORT_IF_KR4
#define SOC_PORT_IF_CR      _SHR_PORT_IF_CR
#define SOC_PORT_IF_CR4     _SHR_PORT_IF_CR4
#define SOC_PORT_IF_XLAUI   _SHR_PORT_IF_XLAUI
#define SOC_PORT_IF_SR      _SHR_PORT_IF_SR
#define SOC_PORT_IF_RXAUI   _SHR_PORT_IF_RXAUI
#define SOC_PORT_IF_SPAUI   _SHR_PORT_IF_SPAUI
#define SOC_PORT_IF_QSGMII  _SHR_PORT_IF_QSGMII
#define SOC_PORT_IF_ILKN    _SHR_PORT_IF_ILKN
#define SOC_PORT_IF_RCY     _SHR_PORT_IF_RCY
#define SOC_PORT_IF_FAT_PIPE _SHR_PORT_IF_FAT_PIPE
#define SOC_PORT_IF_CGMII   _SHR_PORT_IF_CGMII
#define SOC_PORT_IF_CAUI    _SHR_PORT_IF_CAUI
#define SOC_PORT_IF_LR      _SHR_PORT_IF_LR
#define SOC_PORT_IF_LR4     _SHR_PORT_IF_LR4
#define SOC_PORT_IF_SR4     _SHR_PORT_IF_SR4
#define SOC_PORT_IF_KX      _SHR_PORT_IF_KX
#define SOC_PORT_IF_ZR      _SHR_PORT_IF_ZR
#define SOC_PORT_IF_SR10    _SHR_PORT_IF_SR10
#define SOC_PORT_IF_OTL     _SHR_PORT_IF_OTL
#define SOC_PORT_IF_CPU     _SHR_PORT_IF_CPU
#define SOC_PORT_IF_OLP     _SHR_PORT_IF_OLP
#define SOC_PORT_IF_OAMP    _SHR_PORT_IF_OAMP
#define SOC_PORT_IF_ERP     _SHR_PORT_IF_ERP
#define SOC_PORT_IF_TM_INTERNAL_PKT _SHR_PORT_IF_TM_INTERNAL_PKT
#define SOC_PORT_IF_XLAUI2  _SHR_PORT_IF_XLAUI2
#define SOC_PORT_IF_COUNT   _SHR_PORT_IF_COUNT

#if 1
#define SOC_PORT_IF_NAMES_INITIALIZER { \
        "None", \
        "Null", \
        "MII", \
        "GMII", \
        "SGMII", \
        "TBI", \
        "XGMII", \
        "RGMII", \
        "RvMII", \
        "SFI", \
        "XFI", \
        "KR", \
        "KR4", \
        "CR", \
        "CR4", \
        "XLAUI", \
        "SR", \
        "RXAUI", \
        "XAUI", \
        "SPAUI", \
        "QSGMII", \
        "ILKN", \
        "RCY", \
        "FAT_PIPE", \
        "CGMII", \
        "CAUI", \
        "LR", \
        "LR4", \
        "SR4", \
        "KX", \
        "ZR", \
        "SR10", \
        "OTL", \
        "CPU", \
        "OLP", \
        "OAMP", \
        "ERP", \
        "SYNC_PACKET", \
}
#endif
typedef _shr_port_ms_t soc_port_ms_t; 
#define SOC_PORT_MS_SLAVE    _SHR_PORT_MS_SLAVE
#define SOC_PORT_MS_MASTER   _SHR_PORT_MS_MASTER
#define SOC_PORT_MS_AUTO     _SHR_PORT_MS_AUTO
#define SOC_PORT_MS_NONE     _SHR_PORT_MS_NONE
#define SOC_PORT_MS_COUNT    _SHR_PORT_MS_COUNT

typedef _shr_port_duplex_t soc_port_duplex_t;
#define SOC_PORT_DUPLEX_HALF   _SHR_PORT_DUPLEX_HALF
#define SOC_PORT_DUPLEX_FULL   _SHR_PORT_DUPLEX_FULL
#define SOC_PORT_DUPLEX_COUNT  _SHR_PORT_DUPLEX_COUNT

typedef _shr_port_mdix_t soc_port_mdix_t;
#define SOC_PORT_MDIX_AUTO          _SHR_PORT_MDIX_AUTO
#define SOC_PORT_MDIX_FORCE_AUTO    _SHR_PORT_MDIX_FORCE_AUTO
#define SOC_PORT_MDIX_NORMAL        _SHR_PORT_MDIX_NORMAL
#define SOC_PORT_MDIX_XOVER         _SHR_PORT_MDIX_XOVER
#define SOC_PORT_MDIX_COUNT         _SHR_PORT_MDIX_COUNT

typedef _shr_port_mdix_status_t soc_port_mdix_status_t;
#define SOC_PORT_MDIX_STATUS_NORMAL _SHR_PORT_MDIX_STATUS_NORMAL
#define SOC_PORT_MDIX_STATUS_XOVER  _SHR_PORT_MDIX_STATUS_XOVER
#define SOC_PORT_MDIX_STATUS_COUNT  _SHR_PORT_MDIX_STATUS_COUNT

typedef _shr_port_medium_t soc_port_medium_t;
#define SOC_PORT_MEDIUM_NONE        _SHR_PORT_MEDIUM_NONE
#define SOC_PORT_MEDIUM_COPPER      _SHR_PORT_MEDIUM_COPPER
#define SOC_PORT_MEDIUM_FIBER       _SHR_PORT_MEDIUM_FIBER
#define SOC_PORT_MEDIUM_COUNT       _SHR_PORT_MEDIUM_COUNT

typedef _shr_port_mcast_flood_t soc_port_mcast_flood_t;
#define SOC_PORT_MCAST_FLOOD_ALL     _SHR_PORT_MCAST_FLOOD_ALL
#define SOC_PORT_MCAST_FLOOD_UNKNOWN _SHR_PORT_MCAST_FLOOD_UNKNOWN
#define SOC_PORT_MCAST_FLOOD_NONE    _SHR_PORT_MCAST_FLOOD_NONE
#define SOC_PORT_MCAST_FLOOD_COUNT   _SHR_PORT_MCAST_FLOOD_COUNT

typedef _shr_port_cable_state_t soc_port_cable_state_t;
#define SOC_PORT_CABLE_STATE_OK        _SHR_PORT_CABLE_STATE_OK
#define SOC_PORT_CABLE_STATE_OPEN      _SHR_PORT_CABLE_STATE_OPEN
#define SOC_PORT_CABLE_STATE_SHORT     _SHR_PORT_CABLE_STATE_SHORT
#define SOC_PORT_CABLE_STATE_OPENSHORT _SHR_PORT_CABLE_STATE_OPENSHORT
#define SOC_PORT_CABLE_STATE_CROSSTALK _SHR_PORT_CABLE_STATE_CROSSTALK
#define SOC_PORT_CABLE_STATE_UNKNOWN   _SHR_PORT_CABLE_STATE_UNKNOWN
#define SOC_PORT_CABLE_STATE_COUNT     _SHR_PORT_CABLE_STATE_COUNT

typedef _shr_port_cable_diag_t soc_port_cable_diag_t;

extern int
soc_port_phy_pll_os_set(int unit, int port, uint32 vco_freq, uint32 oversample_mode, uint32 pll_divider);

#endif  /* !_SOC_PORTMODE_H */
