/*
 * $Id: bmacsec_port_ability.h 1.3 Broadcom SDK $
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

/* Common definitions for port_ability (BMACSEC) */

#ifndef BMACSEC_PORT_ABILITY_H
#define BMACSEC_PORT_ABILITY_H

typedef enum bmacsec_phy_speed_s {
    BMACSEC_SPEED_10 = 0,
    BMACSEC_SPEED_100,
    BMACSEC_SPEED_1000,
    BMACSEC_SPEED_2500
}bmacsec_phy_speed_t;

typedef enum bmacsec_phy_duplex_s {
    BMACSEC_HALF_DUPLEX = 0,
    BMACSEC_FULL_DUPLEX
}bmacsec_phy_duplex_t;

typedef enum bmacsec_phy_port_mode_s {
    BMACSEC_PHY_FIBER = 0,
    BMACSEC_PHY_COPPER
}bmacsec_phy_port_mode_t;

typedef enum bmacsec_phy_port_medium_s {
    BMACSEC_PHY_MEDIUM_COPPER = 0,
    BMACSEC_PHY_MEDIUM_FIBER,
    BMACSEC_PHY_MEDIUM_BOTH
}bmacsec_phy_port_medium_t;


/*
 * Switch MAC speed and duplex policies
 */
typedef enum bmacsec_phy_mac_policy_s {
    BMACSEC_SWITCH_FIXED = 0,
    BMACSEC_SWITCH_FOLLOWS_LINE
}bmacsec_phy_mac_policy_t;

 
typedef enum bmacsec_port_mdix_s {
    BMACSEC_PORT_MDIX_AUTO,
    BMACSEC_PORT_MDIX_FORCE_AUTO,
    BMACSEC_PORT_MDIX_NORMAL,
    BMACSEC_PORT_MDIX_XOVER
}bmacsec_port_mdix_t;

typedef enum bmacsec_port_mdix_status_s {
    BMACSEC_PORT_MDIX_STATUS_NORMAL,
    BMACSEC_PORT_MDIX_STATUS_XOVER
}bmacsec_port_mdix_status_t;


typedef enum bmacsec_phy_master_s {
    BMACSEC_PORT_MS_SLAVE =0,
    BMACSEC_PORT_MS_MASTER,
    BMACSEC_PORT_MS_AUTO,
    BMACSEC_PORT_MS_NONE
}bmacsec_phy_master_t;

typedef unsigned int bmacsec_port_mode_t;
typedef unsigned int bmacsec_pa_encap_t;

typedef struct bmacsec_port_ability_s {
    bmacsec_port_mode_t speed_half_duplex;
    bmacsec_port_mode_t speed_full_duplex;
    bmacsec_port_mode_t pause;
    bmacsec_port_mode_t interface;
    bmacsec_port_mode_t medium;
    bmacsec_port_mode_t loopback;
    bmacsec_port_mode_t flags;
    bmacsec_port_mode_t eee;
    bmacsec_pa_encap_t  encap;
} bmacsec_port_ability_t;


/*
 * Defines:
 *      BMACSEC_PA_SPEED*
 * Purpose:
 *      Defines for port ability speed.
 * 
 * Note: The defines are matched with SDK defines
 */

#define BMACSEC_PA_SPEED_10MB         (1 << 0)
#define BMACSEC_PA_SPEED_100MB        (1 << 5)
#define BMACSEC_PA_SPEED_1000MB       (1 << 6)

/*
 * Defines:
 *      _BMACSEC_PA_PAUSE_*
 * Purpose:
 *      Defines for flow control abilities.
 *
 * Note: The defines are matched with SDK defines
 */
#define BMACSEC_PA_PAUSE_TX        (1 << 0)       /* TX pause capable */
#define BMACSEC_PA_PAUSE_RX        (1 << 1)       /* RX pause capable */
#define BMACSEC_PA_PAUSE_ASYMM     (1 << 2)       /* Asymm pause capable (R/O) */
#define BMACSEC_PA_PAUSE           (BMACSEC_PA_PAUSE_TX  | BMACSEC_PA_PAUSE_RX)



/*
 * Defines:
 *      BMACSEC_PA_INTF_*
 * Purpose:
 *      Defines for port interfaces supported.
    */
#define BMACSEC_PA_INTF_MII        (1 << 1)       /* MII mode supported */
#define BMACSEC_PA_INTF_GMII       (1 << 2)       /* GMII mode supported */
#define BMACSEC_PA_INTF_SGMII      (1 << 4)       /* SGMII mode supported */
#define BMACSEC_PA_INTF_XGMII      (1 << 5)       /* XGMII mode supported */
#define BMACSEC_PA_INTF_QSGMII     (1 << 6)       /* QSGMII mode supported */


/*
 * Defines:
 *      BMACSEC_PA_LOOPBACK_*
 * Purpose:
 *      Defines for port loopback modes.
 */
#define BMACSEC_PA_LB_NONE         (1 << 0)       /* Useful for automated test */
#define BMACSEC_PA_LB_MAC          (1 << 1)       /* MAC loopback supported */
#define BMACSEC_PA_LB_PHY          (1 << 2)       /* PHY loopback supported */
#define BMACSEC_PA_LB_LINE         (1 << 3)       /* PHY lineside loopback */

/*
 * Defines:
 *      BMACSEC_PA_FLAGS_*
 * Purpose:
 *      Defines for the reest of port ability flags.
 */
#define BMACSEC_PA_AUTONEG         (1 << 0)       /* Auto-negotiation */
#define BMACSEC_PA_COMBO           (1 << 1)       /* COMBO ports support both
                                                * copper and fiber interfaces */




#define BMACSEC_PA_SPEED_ALL     (BMACSEC_PA_1000MB |      \
                                 BMACSEC_PA_100MB |        \
                                 BMACSEC_PA_10MB)

#define BMACSEC_PA_SPEED_MAX(m)  (((m) & BMACSEC_PA_1000MB) ? 1000 : \
                                 ((m) & BMACSEC_PA_100MB)  ? 100 :   \
                                 ((m) & BMACSEC_PA_10MB)   ? 10 : 0)

#define BMACSEC_PA_SPEED(s)      ((1000  == (s)) ? BMACSEC_PA_1000MB : \
                                 (100    == (s)) ? BMACSEC_PA_100MB :  \
                                 (10     == (s)) ? BMACSEC_PA_10MB : 0)

#define BMACSEC_PA_FD            (BMACSEC_PA_1000MB |    \
                                 BMACSEC_PA_100MB   |    \
                                 BMACSEC_PA_10MB)

#define BMACSEC_PA_HD            (BMACSEC_PA_1000MB |    \
                                 BMACSEC_PA_100MB |      \
                                 BMACSEC_PA_10MB)


/* EEE abilities */
#define BMACSEC_PA_EEE_100MB_BASETX       (1 << 0)    /* EEE for 100M-BaseTX */
#define BMACSEC_PA_EEE_1GB_BASET          (1 << 1)    /* EEE for 1G-BaseT */
#define BMACSEC_PA_EEE_10GB_BASET         (1 << 2)    /* EEE for 10G-BaseT */
#define BMACSEC_PA_EEE_10GB_KX            (1 << 3)    /* EEE for 10G-KX */
#define BMACSEC_PA_EEE_10GB_KX4           (1 << 4)    /* EEE for 10G-KX4 */
#define BMACSEC_PA_EEE_10GB_KR            (1 << 5)    /* EEE for 10G-KR */

#endif
