/*
 * $Id: port_ability.h,v 1.23 Broadcom SDK $
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
 * This file defines common network port modes.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define port modes.
 */

#ifndef _SOC_PORTABILITY_H
#define _SOC_PORTABILITY_H

#include <shared/port_ability.h>

typedef _shr_port_ability_t soc_port_ability_t;

#define SOC_PA_ABILITY_ALL     _SHR_PA_ABILITY_ALL
#define SOC_PA_ABILITY_NONE    _SHR_PA_ABILITY_NONE


typedef _shr_pa_encap_t soc_pa_encap_t;

#define SOC_PA_ENCAP_IEEE             _SHR_PA_ENCAP_IEEE 
#define SOC_PA_ENCAP_HIGIG            _SHR_PA_ENCAP_HIGIG 
#define SOC_PA_ENCAP_B5632            _SHR_PA_ENCAP_B5632 
#define SOC_PA_ENCAP_HIGIG2           _SHR_PA_ENCAP_HIGIG2 
#define SOC_PA_ENCAP_HIGIG2_LITE      _SHR_PA_ENCAP_HIGIG2_LITE 
#define SOC_PA_ENCAP_HIGIG2_L2        _SHR_PA_ENCAP_HIGIG2_L2 
#define SOC_PA_ENCAP_HIGIG2_IP_GRE    _SHR_PA_ENCAP_HIGIG2_IP_GRE 
#define SOC_PA_ENCAP_SBX              _SHR_PA_ENCAP_SBX 

/*
 * Defines:
 *      SOC_PA_SPEED_*
 * Purpose:
 *      Defines for port speeds.
 */
#define SOC_PA_SPEED_10MB         _SHR_PA_SPEED_10MB
#define SOC_PA_SPEED_20MB         _SHR_PA_SPEED_20MB
#define SOC_PA_SPEED_25MB         _SHR_PA_SPEED_25MB
#define SOC_PA_SPEED_33MB         _SHR_PA_SPEED_33MB
#define SOC_PA_SPEED_50MB         _SHR_PA_SPEED_50MB
#define SOC_PA_SPEED_100MB        _SHR_PA_SPEED_100MB
#define SOC_PA_SPEED_1000MB       _SHR_PA_SPEED_1000MB
#define SOC_PA_SPEED_2500MB       _SHR_PA_SPEED_2500MB
#define SOC_PA_SPEED_3000MB       _SHR_PA_SPEED_3000MB
#define SOC_PA_SPEED_5000MB       _SHR_PA_SPEED_5000MB
#define SOC_PA_SPEED_6000MB       _SHR_PA_SPEED_6000MB
#define SOC_PA_SPEED_10GB         _SHR_PA_SPEED_10GB
#define SOC_PA_SPEED_11GB         _SHR_PA_SPEED_11GB
#define SOC_PA_SPEED_12GB         _SHR_PA_SPEED_12GB
#define SOC_PA_SPEED_12P5GB       _SHR_PA_SPEED_12P5GB
#define SOC_PA_SPEED_13GB         _SHR_PA_SPEED_13GB
#define SOC_PA_SPEED_15GB         _SHR_PA_SPEED_15GB
#define SOC_PA_SPEED_16GB         _SHR_PA_SPEED_16GB
#define SOC_PA_SPEED_20GB         _SHR_PA_SPEED_20GB
#define SOC_PA_SPEED_21GB         _SHR_PA_SPEED_21GB
#define SOC_PA_SPEED_23GB         _SHR_PA_SPEED_23GB
#define SOC_PA_SPEED_24GB         _SHR_PA_SPEED_24GB
#define SOC_PA_SPEED_25GB         _SHR_PA_SPEED_25GB
#define SOC_PA_SPEED_30GB         _SHR_PA_SPEED_30GB
#define SOC_PA_SPEED_32GB         _SHR_PA_SPEED_32GB
#define SOC_PA_SPEED_40GB         _SHR_PA_SPEED_40GB
#define SOC_PA_SPEED_42GB         _SHR_PA_SPEED_42GB
#define SOC_PA_SPEED_48GB         _SHR_PA_SPEED_48GB
#define SOC_PA_SPEED_100GB        _SHR_PA_SPEED_100GB
#define SOC_PA_SPEED_106GB        _SHR_PA_SPEED_106GB
#define SOC_PA_SPEED_120GB        _SHR_PA_SPEED_120GB
#define SOC_PA_SPEED_127GB        _SHR_PA_SPEED_127GB

#define SOC_PA_SPEED_STRING       _SHR_PA_SPEED_STRING

/*
 * Defines:
 *      SOC_PA_PAUSE_*
 * Purpose:
 *      Defines for flow control abilities.
 */
#define SOC_PA_PAUSE_TX        _SHR_PA_PAUSE_TX
#define SOC_PA_PAUSE_RX        _SHR_PA_PAUSE_RX
#define SOC_PA_PAUSE_ASYMM     _SHR_PA_PAUSE_ASYMM

#define SOC_PA_PAUSE_STRING    _SHR_PA_PAUSE_STRING

/*
 * Defines:
 *      SOC_PA_INTF_*
 * Purpose:
 *      Defines for port interfaces supported.
 */
#define SOC_PA_INTF_TBI        _SHR_PA_INTF_TBI  
#define SOC_PA_INTF_MII        _SHR_PA_INTF_MII  
#define SOC_PA_INTF_GMII       _SHR_PA_INTF_GMII 
#define SOC_PA_INTF_RGMII      _SHR_PA_INTF_RGMII
#define SOC_PA_INTF_SGMII      _SHR_PA_INTF_SGMII
#define SOC_PA_INTF_XGMII      _SHR_PA_INTF_XGMII
#define SOC_PA_INTF_QSGMII     _SHR_PA_INTF_QSGMII
#define SOC_PA_INTF_CGMII      _SHR_PA_INTF_CGMII

#define SOC_PA_INTF_STRING     _SHR_PA_INTF_STRING

/*
 * Defines:
 *      SOC_PA_MEDIUM_*
 * Purpose:
 *      Defines for port medium modes.
 */
#define SOC_PA_MEDIUM_COPPER   _SHR_PA_MEDIUM_COPPER
#define SOC_PA_MEDIUM_FIBER    _SHR_PA_MEDIUM_FIBER

#define SOC_PA_MEDIUM_STRING   _SHR_PA_MEDIUM_STRING 

/*
 * Defines:
 *      SOC_PA_LOOPBACK_*
 * Purpose:
 *      Defines for port loopback modes.
 */
#define SOC_PA_LB_NONE         _SHR_PA_LB_NONE
#define SOC_PA_LB_MAC          _SHR_PA_LB_MAC
#define SOC_PA_LB_PHY          _SHR_PA_LB_PHY
#define SOC_PA_LB_LINE         _SHR_PA_LB_LINE

#define SOC_PA_LB_STRING       _SHR_PA_LB_STRING

/*
 * Defines:
 *      SOC_PA_FLAGS_*
 * Purpose:
 *      Defines for the reest of port ability flags.
 */
#define SOC_PA_AUTONEG         _SHR_PA_AUTONEG 
#define SOC_PA_COMBO           _SHR_PA_COMBO 


#define SOC_PA_FLAGS_STRING    _SHR_PA_FLAGS_STRING

/* Ability filters  */

#define SOC_PA_PAUSE           _SHR_PA_PAUSE

#define SOC_PA_SPEED_ALL    _SHR_PA_SPEED_ALL

#define SOC_PA_SPEED_MAX(m) _SHR_PA_SPEED_MAX(m)

#define SOC_PA_SPEED(s)     _SHR_PA_SPEED(s) 


#define SOC_PORT_ABILITY_SFI      _SHR_PORT_ABILITY_SFI
#define SOC_PORT_ABILITY_DUAL_SFI _SHR_PORT_ABILITY_DUAL_SFI
#define SOC_PORT_ABILITY_SCI      _SHR_PORT_ABILITY_SCI
#define SOC_PORT_ABILITY_SFI_SCI  _SHR_PORT_ABILITY_SFI_SCI


/* EEE Abilities */
#define SOC_PA_EEE_100MB_BASETX         _SHR_PA_EEE_100MB_BASETX
#define SOC_PA_EEE_1GB_BASET            _SHR_PA_EEE_1GB_BASET
#define SOC_PA_EEE_10GB_BASET           _SHR_PA_EEE_10GB_BASET
#define SOC_PA_EEE_10GB_KX              _SHR_PA_EEE_10GB_KX
#define SOC_PA_EEE_10GB_KX4             _SHR_PA_EEE_10GB_KX4
#define SOC_PA_EEE_10GB_KR              _SHR_PA_EEE_10GB_KR


/* Port ability functions */
extern int
soc_port_mode_to_ability(soc_port_mode_t mode, soc_port_ability_t *ability);

extern int
soc_port_ability_to_mode(soc_port_ability_t *ability, soc_port_mode_t *mode);

#endif  /* !_SOC_PORTABILITY_H */
