/*
 * $Id: port.h 1.118.2.1 Broadcom SDK $
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
 * This file defines common network port parameters.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define port parameters.
 */

#ifndef _SHR_PORT_H
#define _SHR_PORT_H

/*
 * Typedef:
 *    _shr_port_t
 * Purpose:
 *    Port number type for shared definitions
 * Note:
 *    Currently it is NOT used to define bcm_port_t and soc_port_t, but thus
 *    should be changed
 */
#include <sal/types.h>

typedef int _shr_port_t;

/*
 * Defines:
 *	_SHR_PORT_DUPLEX_*
 * Purpose:
 *	Defines duplexity of a port
 */

typedef enum _shr_port_duplex_e {
    _SHR_PORT_DUPLEX_HALF,
    _SHR_PORT_DUPLEX_FULL,
    _SHR_PORT_DUPLEX_COUNT	/* last, please */
} _shr_port_duplex_t;

/*
 * Defines:
 *  _SHR_PORT_IF_*
 * Purpose:
 *  Defines interface type between MAC and PHY.
 */

typedef enum _shr_port_if_e {
    _SHR_PORT_IF_NOCXN, /* No physical connection */
    _SHR_PORT_IF_NULL,  /* Pass-through connection without PHY */
    _SHR_PORT_IF_MII,
    _SHR_PORT_IF_GMII,
    _SHR_PORT_IF_SGMII,
    _SHR_PORT_IF_TBI,
    _SHR_PORT_IF_XGMII,
    _SHR_PORT_IF_RGMII,
    _SHR_PORT_IF_RvMII,
    _SHR_PORT_IF_SFI,
    _SHR_PORT_IF_XFI,
    _SHR_PORT_IF_KR,
    _SHR_PORT_IF_KR4,
    _SHR_PORT_IF_CR,
    _SHR_PORT_IF_CR4,
    _SHR_PORT_IF_XLAUI,
    _SHR_PORT_IF_SR,
    _SHR_PORT_IF_RXAUI,
    _SHR_PORT_IF_XAUI,
    _SHR_PORT_IF_SPAUI,
    _SHR_PORT_IF_QSGMII,
    _SHR_PORT_IF_ILKN,
    _SHR_PORT_IF_RCY,
    _SHR_PORT_IF_FAT_PIPE,
    _SHR_PORT_IF_CGMII,
    _SHR_PORT_IF_CAUI,
    _SHR_PORT_IF_LR,
    _SHR_PORT_IF_LR4,
    _SHR_PORT_IF_SR4,
    _SHR_PORT_IF_KX,
    _SHR_PORT_IF_ZR,
    _SHR_PORT_IF_SR10,
    _SHR_PORT_IF_OTL,
    _SHR_PORT_IF_CPU,
    _SHR_PORT_IF_OLP,
    _SHR_PORT_IF_OAMP,
    _SHR_PORT_IF_ERP,
    _SHR_PORT_IF_COUNT /* last, please */
} _shr_port_if_t;

/*
 * Defines:
 *  _SHR_PORT_MS_*
 * Purpose:
 *  Defines master/slave mode of a port (PHY).
 */

typedef enum _shr_port_ms_e {
    _SHR_PORT_MS_SLAVE,
    _SHR_PORT_MS_MASTER,
    _SHR_PORT_MS_AUTO,
    _SHR_PORT_MS_NONE,
    _SHR_PORT_MS_COUNT  /* last, please */
} _shr_port_ms_t;

/*
 * Defines:
 *  _SHR_PORT_ENCAP_*
 * Purpose:
 *  Defines various header encapsulation modes.
 * Notes:
 *  WARNING: values must match MAC_TXCTRL register HDRMODE field.
 */

typedef enum _shr_port_encap_e {
    _SHR_PORT_ENCAP_IEEE = 0,   /* IEEE 802.3 Ethernet-II  */
    _SHR_PORT_ENCAP_HIGIG = 1,  /* HIGIG Header mode       */
    _SHR_PORT_ENCAP_B5632 = 2,  /* BCM5632 Header mode     */
    _SHR_PORT_ENCAP_HIGIG2 = 3, /* HIGIG2 Header mode      */
    _SHR_PORT_ENCAP_HIGIG2_LITE = 4, /* HIGIG2 Header mode (Raptor style) */
    _SHR_PORT_ENCAP_HIGIG2_L2 = 5,  /* HIGIG2 Transport mode   */
    _SHR_PORT_ENCAP_HIGIG2_IP_GRE = 6,  /* HIGIG2 Tunnel mode   */
    _SHR_PORT_ENCAP_SBX = 7,        /* SBX Header mode         */
    _SHR_PORT_ENCAP_PREAMBLE_SOP_ONLY = 8,   /* 1B preamble mode*/
    _SHR_PORT_ENCAP_COUNT   /* last, please */
} _shr_port_encap_t;

#define _SHR_PORT_ENCAP_NAMES_INITIALIZER { \
    "IEEE", \
    "HiGig", \
    "B5632", \
    "HiGig2", \
    "HiGig2_L2", \
    "HiGig2_IP_GRE", \
    "Sbx",   \
    }

/*
 * Defines:
 *  _SHR_PORT_STP_*
 * Purpose:
 *  Defines the spanning tree states of a port.
 */

typedef enum _shr_port_stp_e {
    _SHR_PORT_STP_DISABLE,
    _SHR_PORT_STP_BLOCK,
    _SHR_PORT_STP_LISTEN,
    _SHR_PORT_STP_LEARN,
    _SHR_PORT_STP_FORWARD,
    _SHR_PORT_STP_COUNT    /* last, please */
} _shr_port_stp_t;

/*
 * Defines:
 *      _SHR_PORT_MDIX_*
 * Purpose:
 *      Defines the MDI crossover (MDIX) modes for the port
 */
typedef enum _shr_port_mdix_e {
    _SHR_PORT_MDIX_AUTO,
    _SHR_PORT_MDIX_FORCE_AUTO,
    _SHR_PORT_MDIX_NORMAL,
    _SHR_PORT_MDIX_XOVER,
    _SHR_PORT_MDIX_COUNT    /* last, please */
} _shr_port_mdix_t;

/*
 * Defines:
 *      _SHR_PORT_MDIX_STATUS_*
 * Purpose:
 *      Defines the MDI crossover state
 */
typedef enum _shr_port_mdix_status_e {
    _SHR_PORT_MDIX_STATUS_NORMAL,
    _SHR_PORT_MDIX_STATUS_XOVER,
    _SHR_PORT_MDIX_STATUS_COUNT       /* last, please */
} _shr_port_mdix_status_t;

/*
 * Defines:
 *      _SHR_PORT_MEDIUM_*
 * Purpose:
 *      Supported physical mediums
 */
typedef enum _shr_port_medium_e {
    _SHR_PORT_MEDIUM_NONE              = 0,
    _SHR_PORT_MEDIUM_COPPER            = 1,
    _SHR_PORT_MEDIUM_FIBER             = 2,
    _SHR_PORT_MEDIUM_COUNT             /* last, please */
} _shr_port_medium_t;

/*
 * Defines:
 *      _SHR_PORT_MCAST_FLOOD_*
 * Purpose:
 *      Multicast packet flooding mode
 */
typedef enum _shr_port_mcast_flood_e {
    _SHR_PORT_MCAST_FLOOD_ALL     = 0,
    _SHR_PORT_MCAST_FLOOD_UNKNOWN = 1,
    _SHR_PORT_MCAST_FLOOD_NONE    = 2,
    _SHR_PORT_MCAST_FLOOD_COUNT         /* last, please */
} _shr_port_mcast_flood_t;

/*
 * Defines:
 *  _SHR_PORT_CABLE_STATE_*
 * Purpose:
 *  Cable diag state (per pair and overall)
 */
typedef enum {
    _SHR_PORT_CABLE_STATE_OK,
    _SHR_PORT_CABLE_STATE_OPEN,
    _SHR_PORT_CABLE_STATE_SHORT,
    _SHR_PORT_CABLE_STATE_OPENSHORT,
    _SHR_PORT_CABLE_STATE_CROSSTALK,
    _SHR_PORT_CABLE_STATE_UNKNOWN,
    _SHR_PORT_CABLE_STATE_COUNT /* last, as always */
} _shr_port_cable_state_t;

#define _SHR_PORT_CABLE_STATE_NAMES_INITIALIZER { \
    "Ok", \
    "Open", \
    "Short", \
    "Open/Short", \
    "Crosstalk", \
    "Unknown", \
    }

/*
 * Defines:
 *  _shr_port_cable_diag_t
 * Purpose:
 *  Shared definition of port (phy) cable diag status structure.
 *  Used both in soc/phy layer and in bcm layer.
 */
typedef struct _shr_port_cable_diag_s {
    _shr_port_cable_state_t state;  /* state of all pairs */
    int             npairs;    /* pair_* elements valid */
    _shr_port_cable_state_t pair_state[4];  /* pair state */
    int             pair_len[4];    /* pair length in metres */
    int             fuzz_len;   /* len values +/- this */
} _shr_port_cable_diag_t;

/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_*
 * Purpose:
 *     PHY specific control settings
 */
typedef enum _shr_port_phy_control_e {
    _SHR_PORT_PHY_CONTROL_WAN,
    _SHR_PORT_PHY_CONTROL_PREEMPHASIS,
    _SHR_PORT_PHY_CONTROL_DRIVER_CURRENT,
    _SHR_PORT_PHY_CONTROL_PRE_DRIVER_CURRENT,
    _SHR_PORT_PHY_CONTROL_EQUALIZER_BOOST,
    _SHR_PORT_PHY_CONTROL_INTERFACE,
    _SHR_PORT_PHY_CONTROL_INTERFACE_MAX,
    _SHR_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED,
    _SHR_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED_SPEED,
    _SHR_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED_DUPLEX,
    _SHR_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED_PAUSE,
    _SHR_PORT_PHY_CONTROL_MACSEC_PAUSE_RX_FORWARD,
    _SHR_PORT_PHY_CONTROL_MACSEC_PAUSE_TX_FORWARD,
    _SHR_PORT_PHY_CONTROL_MACSEC_LINE_IPG,
    _SHR_PORT_PHY_CONTROL_MACSEC_SWITCH_IPG,
    _SHR_PORT_PHY_CONTROL_LONGREACH_SPEED,
    _SHR_PORT_PHY_CONTROL_LONGREACH_PAIRS,
    _SHR_PORT_PHY_CONTROL_LONGREACH_GAIN,
    _SHR_PORT_PHY_CONTROL_LONGREACH_AUTONEG,
    _SHR_PORT_PHY_CONTROL_LONGREACH_LOCAL_ABILITY,
    _SHR_PORT_PHY_CONTROL_LONGREACH_REMOTE_ABILITY,
    _SHR_PORT_PHY_CONTROL_LONGREACH_CURRENT_ABILITY,
    _SHR_PORT_PHY_CONTROL_LONGREACH_MASTER,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ACTIVE,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ENABLE,
    _SHR_PORT_PHY_CONTROL_LOOPBACK_EXTERNAL,
    _SHR_PORT_PHY_CONTROL_CLOCK_ENABLE,
    _SHR_PORT_PHY_CONTROL_CLOCK_SECONDARY_ENABLE,
    _SHR_PORT_PHY_CONTROL_CLOCK_FREQUENCY,
    _SHR_PORT_PHY_CONTROL_PORT_PRIMARY,
    _SHR_PORT_PHY_CONTROL_PORT_OFFSET,
    _SHR_PORT_PHY_CONTROL_PRE_PREEMPHASIS,
    _SHR_PORT_PHY_CONTROL_ENCODING,
    _SHR_PORT_PHY_CONTROL_SCRAMBLER,
    _SHR_PORT_PHY_CONTROL_PRBS_POLYNOMIAL,
    _SHR_PORT_PHY_CONTROL_PRBS_TX_INVERT_DATA,
    _SHR_PORT_PHY_CONTROL_PRBS_TX_ENABLE,
    _SHR_PORT_PHY_CONTROL_PRBS_RX_ENABLE,
    _SHR_PORT_PHY_CONTROL_PRBS_RX_STATUS,
    _SHR_PORT_PHY_CONTROL_SERDES_DRIVER_TUNE,
    _SHR_PORT_PHY_CONTROL_SERDES_DRIVER_EQUALIZATION_TUNE_STATUS_FAR_END,
    _SHR_PORT_PHY_CONTROL_8B10B,
    _SHR_PORT_PHY_CONTROL_64B66B,
    _SHR_PORT_PHY_CONTROL_POWER,
    _SHR_PORT_PHY_CONTROL_POWER_AUTO_SLEEP_TIME,
    _SHR_PORT_PHY_CONTROL_POWER_AUTO_WAKE_TIME,
    _SHR_PORT_PHY_CONTROL_LINKDOWN_TRANSMIT,
    _SHR_PORT_PHY_CONTROL_EDC_CONFIG,
    _SHR_PORT_PHY_CONTROL_EDC_MODE,
    _SHR_PORT_PHY_CONTROL_EEE,
    _SHR_PORT_PHY_CONTROL_EEE_AUTO,
    _SHR_PORT_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_EEE_AUTO_BUFFER_LIMIT,
    _SHR_PORT_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY,
    _SHR_PORT_PHY_CONTROL_EEE_TRANSMIT_WAKE_TIME,
    _SHR_PORT_PHY_CONTROL_EEE_RECEIVE_WAKE_TIME,
    _SHR_PORT_PHY_CONTROL_EEE_TRANSMIT_SLEEP_TIME,
    _SHR_PORT_PHY_CONTROL_EEE_RECEIVE_SLEEP_TIME,
    _SHR_PORT_PHY_CONTROL_EEE_TRANSMIT_QUIET_TIME,
    _SHR_PORT_PHY_CONTROL_EEE_RECEIVE_QUIET_TIME,
    _SHR_PORT_PHY_CONTROL_EEE_TRANSMIT_REFRESH_TIME,
    _SHR_PORT_PHY_CONTROL_EEE_TRANSMIT_EVENTS,
    _SHR_PORT_PHY_CONTROL_EEE_TRANSMIT_DURATION,
    _SHR_PORT_PHY_CONTROL_EEE_RECEIVE_EVENTS,
    _SHR_PORT_PHY_CONTROL_EEE_RECEIVE_DURATION,
    _SHR_PORT_PHY_CONTROL_EEE_STATISTICS_CLEAR,
    _SHR_PORT_PHY_CONTROL_SERDES_TUNE_MARGIN_MAX,
    _SHR_PORT_PHY_CONTROL_SERDES_TUNE_MARGIN_MODE,
    _SHR_PORT_PHY_CONTROL_SERDES_TUNE_MARGIN_VALUE,
    _SHR_PORT_PHY_CONTROL_PARALLEL_DETECTION,
    _SHR_PORT_PHY_CONTROL_PARALLEL_DETECTION_10G,
    _SHR_PORT_PHY_CONTROL_LOOPBACK_INTERNAL,
    _SHR_PORT_PHY_CONTROL_LOOPBACK_REMOTE,
    _SHR_PORT_PHY_CONTROL_LOOPBACK_REMOTE_PCS_BYPASS,
    _SHR_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION,
    _SHR_PORT_PHY_CONTROL_CUSTOM1,
    _SHR_PORT_PHY_CONTROL_BERT_PATTERN,
    _SHR_PORT_PHY_CONTROL_BERT_RUN,
    _SHR_PORT_PHY_CONTROL_BERT_PACKET_SIZE,
    _SHR_PORT_PHY_CONTROL_BERT_IPG,
    _SHR_PORT_PHY_CONTROL_BERT_TX_PACKETS,
    _SHR_PORT_PHY_CONTROL_BERT_RX_PACKETS,
    _SHR_PORT_PHY_CONTROL_BERT_RX_ERROR_BITS,
    _SHR_PORT_PHY_CONTROL_BERT_RX_ERROR_BYTES,
    _SHR_PORT_PHY_CONTROL_BERT_RX_ERROR_PACKETS,
    _SHR_PORT_PHY_CONTROL_PREEMPHASIS_LANE0,
    _SHR_PORT_PHY_CONTROL_PREEMPHASIS_LANE1,
    _SHR_PORT_PHY_CONTROL_PREEMPHASIS_LANE2,
    _SHR_PORT_PHY_CONTROL_PREEMPHASIS_LANE3,
    _SHR_PORT_PHY_CONTROL_DRIVER_CURRENT_LANE0,
    _SHR_PORT_PHY_CONTROL_DRIVER_CURRENT_LANE1,
    _SHR_PORT_PHY_CONTROL_DRIVER_CURRENT_LANE2,
    _SHR_PORT_PHY_CONTROL_DRIVER_CURRENT_LANE3,
    _SHR_PORT_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE0,
    _SHR_PORT_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE1,
    _SHR_PORT_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE2,
    _SHR_PORT_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE3,
    _SHR_PORT_PHY_CONTROL_TX_LANE_SQUELCH,
    _SHR_PORT_PHY_CONTROL_DRIVER_POST2_CURRENT,
    _SHR_PORT_PHY_CONTROL_RX_PPM,
    _SHR_PORT_PHY_CONTROL_RX_PEAK_FILTER,
    _SHR_PORT_PHY_CONTROL_RX_LOW_FREQ_PEAK_FILTER,
    _SHR_PORT_PHY_CONTROL_RX_VGA,
    _SHR_PORT_PHY_CONTROL_RX_TAP1,
    _SHR_PORT_PHY_CONTROL_RX_TAP2,
    _SHR_PORT_PHY_CONTROL_RX_TAP3,
    _SHR_PORT_PHY_CONTROL_RX_TAP4,
    _SHR_PORT_PHY_CONTROL_RX_TAP5,
    _SHR_PORT_PHY_CONTROL_RX_VGA_RELEASE,
    _SHR_PORT_PHY_CONTROL_RX_TAP1_RELEASE,
    _SHR_PORT_PHY_CONTROL_RX_TAP2_RELEASE,
    _SHR_PORT_PHY_CONTROL_RX_TAP3_RELEASE,
    _SHR_PORT_PHY_CONTROL_RX_TAP4_RELEASE,
    _SHR_PORT_PHY_CONTROL_RX_TAP5_RELEASE,
    _SHR_PORT_PHY_CONTROL_RX_PLUS1_SLICER,
    _SHR_PORT_PHY_CONTROL_RX_MINUS1_SLICER,
    _SHR_PORT_PHY_CONTROL_RX_D_SLICER,
    _SHR_PORT_PHY_CONTROL_RX_SIGNAL_DETECT,
    _SHR_PORT_PHY_CONTROL_RX_SEQ_DONE,
    _SHR_PORT_PHY_CONTROL_RX_POLARITY,
    _SHR_PORT_PHY_CONTROL_TX_POLARITY,
    _SHR_PORT_PHY_CONTROL_RX_RESET,
    _SHR_PORT_PHY_CONTROL_TX_RESET,
    _SHR_PORT_PHY_CONTROL_CL72,
    _SHR_PORT_PHY_CONTROL_CL72_STATUS,
    _SHR_PORT_PHY_CONTROL_LANE_SWAP,
    _SHR_PORT_PHY_CONTROL_DUMP,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_20BIT,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_256BIT,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA0,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA1,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA2,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA3,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA4,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA5,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA6,
    _SHR_PORT_PHY_CONTROL_TX_PATTERN_DATA7,
    _SHR_PORT_PHY_CONTROL_PHASE_INTERP,
    _SHR_PORT_PHY_CONTROL_PON_LASER_TX_POWER_TIME,
    _SHR_PORT_PHY_CONTROL_PON_LASER_TX_POWER_MODE,
    _SHR_PORT_PHY_CONTROL_PON_LASER_TX_STATUS,
    _SHR_PORT_PHY_CONTROL_PON_LASER_RX_STATE,
    _SHR_PORT_PHY_CONTROL_PON_LASER_TRANCEIVER_TEMP,
    _SHR_PORT_PHY_CONTROL_PON_LASER_SUPPLY_VOLTAGE,
    _SHR_PORT_PHY_CONTROL_PON_LASER_TX_BIAS,
    _SHR_PORT_PHY_CONTROL_PON_LASER_TX_POWER,
    _SHR_PORT_PHY_CONTROL_PON_LASER_RX_POWER,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_HIGH_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_HIGH_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_HIGH_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_LOW_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_LOW_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_LOW_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_HIGH_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_HIGH_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_HIGH_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_LOW_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_LOW_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_LOW_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_HIGH_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_HIGH_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_HIGH_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_LOW_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_LOW_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_LOW_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_HIGH_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_VCC_HIGH_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_HIGH_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_LOW_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_VCC_LOW_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_LOW_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_HIGH_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_HIGH_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_HIGH_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_LOW_ALARM_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_LOW_ALARM_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_LOW_ALARM_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_HIGH_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_HIGH_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_HIGH_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_LOW_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_LOW_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_RX_POWER_LOW_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_HIGH_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_HIGH_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_HIGH_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_LOW_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_LOW_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_POWER_LOW_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_HIGH_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_HIGH_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_HIGH_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_LOW_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_LOW_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TX_BIAS_LOW_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_HIGH_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_VCC_HIGH_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_HIGH_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_LOW_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_VCC_LOW_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_VCC_LOW_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_HIGH_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_HIGH_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_HIGH_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_LOW_WARNING_STATE,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_LOW_WARNING_REPORT_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_PON_TEMP_LOW_WARNING_CLEAR_THRESHOLD,
    _SHR_PORT_PHY_CONTROL_DLL_POWER_AUTO,
    _SHR_PORT_PHY_CONTROL_TX_PPM_ADJUST,
    _SHR_PORT_PHY_CONTROL_FIRMWARE_MODE,
    _SHR_PORT_PHY_CONTROL_VCO_FREQ,
    _SHR_PORT_PHY_CONTROL_PLL_DIVIDER,
    _SHR_PORT_PHY_CONTROL_OVERSAMPLE_MODE,
    _SHR_PORT_PHY_CONTROL_REF_CLK,
    _SHR_PORT_PHY_CONTROL_SOFTWARE_RX_LOS,
    _SHR_PORT_PHY_CONTROL_PRBS_DECOUPLED_TX_ENABLE,
    _SHR_PORT_PHY_CONTROL_PRBS_DECOUPLED_TX_INVERT_DATA,
    _SHR_PORT_PHY_CONTROL_PRBS_DECOUPLED_TX_POLYNOMIAL,
    _SHR_PORT_PHY_CONTROL_PRBS_DECOUPLED_RX_ENABLE,
    _SHR_PORT_PHY_CONTROL_PRBS_DECOUPLED_RX_INVERT_DATA,
    _SHR_PORT_PHY_CONTROL_PRBS_DECOUPLED_RX_POLYNOMIAL,
    _SHR_PORT_PHY_CONTROL_RX_PEAK_FILTER_TEMP_COMP,
    _SHR_PORT_PHY_CONTROL_CL73_FSM_AUTO_RECOVER,
    _SHR_PORT_PHY_CONTROL_RX_SEQ_TOGGLE,
    _SHR_PORT_PHY_CONTROL_DRIVER_SUPPLY,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE,
    _SHR_PORT_PHY_CONTROL_HG2_BCM_CODEC_ENABLE,
    _SHR_PORT_PHY_CONTROL_NUM      /* Must be last */
} _shr_port_phy_control_t;

/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_FIRMWARE_MODE_*
 * Purpose:
 *     FIRMWARE MODE
 */
typedef enum _shr_port_phy_control_firmware_mode_e {
    _SHR_PORT_PHY_FIRMWARE_DEFAULT = 0,   /* default */
    _SHR_PORT_PHY_FIRMWARE_SFP_OPT_SR4,   /* optical short range */
    _SHR_PORT_PHY_FIRMWARE_SFP_DAC,       /* direct attach copper */
    _SHR_PORT_PHY_FIRMWARE_XLAUI,         /* 40G XLAUI mode */
    _SHR_PORT_PHY_FIRMWARE_FORCE_OSDFE,   /* force over sample digital feedback equalization */
    _SHR_PORT_PHY_FIRMWARE_FORCE_BRDFE,   /* force baud rate digital feedback equalization */
    _SHR_PORT_PHY_FIRMWARE_SW_CL72,       /* software cl72 with AN on */
    _SHR_PORT_PHY_FIRMWARE_CL72_WITHOUT_AN   /* cl72 without AN */
} _shr_port_phy_control_firmware_mode_t;

/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_*
 * Purpose:
 *    Set autoneg mode
 */
typedef enum _shr_port_phy_control_autoneg_mode_e {
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL37 = 1,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL37_BAM,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL37_CL73,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL37_CL73_BAM,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL37_BAM_CL73,
    _SHR_PORT_PHY_CONTROL_AUTONEG_MODE_CL37_BAM_CL73_BAM
} _shr_port_phy_control_autoneg_mode_t;


/*
 * Defines:
 *     _SHR_PORT_PRBS_POLYNOMIAL_*
 * Purpose:
 *     PRBS polynomial type
 */
typedef enum _shr_port_prbs_polynomial_e {
    _SHR_PORT_PRBS_POLYNOMIAL_X7_X6_1,
    _SHR_PORT_PRBS_POLYNOMIAL_X15_X14_1
} _shr_port_prbs_polynomial_t;

/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_*
 * Purpose:
 *     PHY specific control settings
 */
typedef enum _shr_port_phy_control_longreach_ability_e {
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_NONE,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_10M_1PAIR=1,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_10M_2PAIR=2,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_20M_1PAIR=4,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_20M_2PAIR=8,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_25M_1PAIR=16,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_25M_2PAIR=32,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_33M_1PAIR=64,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_33M_2PAIR=128,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_50M_1PAIR=256,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_50M_2PAIR=512,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_100M_1PAIR=1024,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_100M_2PAIR=2048,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_100M_4PAIR=4096,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_PAUSE_RX=8192,
    _SHR_PORT_PHY_CONTROL_LONGREACH_ABILITY_PAUSE_TX=16384
} _shr_port_phy_control_longreach_ability_t;

/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_POWER_*
 * Purpose:
 *     PHY specific values for _SHR_PORT_PHY_CONTROL_POWER type
 */
typedef enum _shr_port_phy_control_power_e {
    _SHR_PORT_PHY_CONTROL_POWER_FULL,
    _SHR_PORT_PHY_CONTROL_POWER_LOW,
    _SHR_PORT_PHY_CONTROL_POWER_AUTO,
    /*
       Power mode added(SDK-30895). Requirement is to disable Auto Power Down
       without changing Tx bias current
    */
    _SHR_PORT_PHY_CONTROL_POWER_AUTO_DISABLE,
    _SHR_PORT_PHY_CONTROL_POWER_AUTO_FULL,
    _SHR_PORT_PHY_CONTROL_POWER_AUTO_LOW
} _shr_port_phy_control_power_t;

/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_FEC_*
 * Purpose:
 *     PHY specific values for _SHR_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION
 */
typedef enum _shr_port_phy_control_fec_e {
    _SHR_PORT_PHY_CONTROL_FEC_OFF,
    _SHR_PORT_PHY_CONTROL_FEC_ON,
    _SHR_PORT_PHY_CONTROL_FEC_AUTO
} _shr_port_phy_control_fec_t;


/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_EDC_CONFIG_*
 * Purpose:
 *     Valid control values for _SHR_PORT_PHY_CONTROL_EDC_CONFIG type
 */
typedef enum _shr_port_phy_control_edc_config_e {
    _SHR_PORT_PHY_CONTROL_EDC_CONFIG_NONE,
    _SHR_PORT_PHY_CONTROL_EDC_CONFIG_HARDWARE,
    _SHR_PORT_PHY_CONTROL_EDC_CONFIG_SOFTWARE
} _shr_port_phy_control_edc_config_t;

/*
 * Defines:
 *     _SHR_PORT_PHY_CONTROL_BERT_*
 * Purpose:
 *     Valid control values for _SHR_PORT_PHY_CONTROL_BERT_PATTERN type
 */
typedef enum _shr_port_phy_control_bert_pattern_e {
    _SHR_PORT_PHY_CONTROL_BERT_CRPAT = 1,
    _SHR_PORT_PHY_CONTROL_BERT_CJPAT = 2
} _shr_port_phy_control_bert_pattern_t;

typedef enum _shr_port_phy_control_driver_supply_e {
	_SHR_PORT_PHY_CONTROL_DRIVER_SUPPLY_1_5V_1_0V,
	_SHR_PORT_PHY_CONTROL_DRIVER_SUPPLY_1_0V_700MV,
	_SHR_PORT_PHY_CONTROL_DRIVER_SUPPLY_1_5V_700MV
} _shr_port_phy_control_driver_supply_t;

/*
 * Congestion support (HCFC)
 */
#define _SHR_PORT_CHANNEL_ID_PORT_BASE_SHIFT                                          8
#define _SHR_PORT_CHANNEL_ID_PORT_BASE_MASK                                           0xFF
#define _SHR_PORT_CHANNEL_ID_PORT_CH_SHIFT                                            0
#define _SHR_PORT_CHANNEL_ID_PORT_CH_MASK                                             0xFF

#define _SHR_PORT_CONGESTION_MAPPING_CLEAR                                            0x01
#define _SHR_PORT_CONGESTION_MAPPING_RX                                               0x02
#define _SHR_PORT_CONGESTION_MAPPING_TX                                               0x04

#define _SHR_PORT_CHANNEL_ID_SET(_channel_id, _port_base, _port_ch)                     \
              (_channel_id = (((_port_base & _SHR_PORT_CHANNEL_ID_PORT_BASE_MASK) <<    \
                                        _SHR_PORT_CHANNEL_ID_PORT_BASE_SHIFT) |         \
               ((_port_ch & _SHR_PORT_CHANNEL_ID_PORT_CH_MASK) <<                       \
                                        _SHR_PORT_CHANNEL_ID_PORT_CH_SHIFT)))

#define _SHR_PORT_CHANNEL_ID_PORT_BASE_GET(_channel_id)                                 \
              ((_channel_id >> _SHR_PORT_CHANNEL_ID_PORT_BASE_SHIFT) & _SHR_PORT_CHANNEL_ID_PORT_BASE_MASK)

#define _SHR_PORT_CHANNEL_ID_PORT_CH_GET(_channel_id)                  \
              ((_channel_id >> _SHR_PORT_CHANNEL_ID_PORT_CH_SHIFT) & _SHR_PORT_CHANNEL_ID_PORT_CH_MASK)

#define _SHR_PORT_PHY_TIMESYNC_CAP_MPLS                             (1U << 0)
#define _SHR_PORT_PHY_TIMESYNC_CAP_ENHANCED_TSFIFO                  (1U << 1)
#define _SHR_PORT_PHY_TIMESYNC_CAP_INBAND_TS                        (1U << 2)
#define _SHR_PORT_PHY_TIMESYNC_CAP_FOLLOW_UP_ASSIST                 (1U << 3)
#define _SHR_PORT_PHY_TIMESYNC_CAP_DELAY_RESP_ASSIST                (1U << 4)
#define _SHR_PORT_PHY_TIMESYNC_CAP_CAPTURE_TIMESTAMP_MSG            (1U << 5)

#define _SHR_PORT_PHY_TIMESYNC_VALID_FLAGS                         (1U <<  0)
#define _SHR_PORT_PHY_TIMESYNC_VALID_ITPID                         (1U <<  1)
#define _SHR_PORT_PHY_TIMESYNC_VALID_OTPID                         (1U <<  2)
#define _SHR_PORT_PHY_TIMESYNC_VALID_OTPID2                        (1U <<  3)
#define _SHR_PORT_PHY_TIMESYNC_VALID_GMODE                         (1U <<  4)
#define _SHR_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE                (1U <<  5)
#define _SHR_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE                  (1U <<  6)
#define _SHR_PORT_PHY_TIMESYNC_VALID_TS_DIVIDER                    (1U <<  7)
#define _SHR_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE             (1U <<  8)
#define _SHR_PORT_PHY_TIMESYNC_VALID_TX_TIMESTAMP_OFFSET           (1U <<  9)
#define _SHR_PORT_PHY_TIMESYNC_VALID_RX_TIMESTAMP_OFFSET           (1U << 10)
#define _SHR_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE                  (1U << 11)
#define _SHR_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE         (1U << 12)
#define _SHR_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE        (1U << 13)
#define _SHR_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE       (1U << 14)
#define _SHR_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE                  (1U << 15)
#define _SHR_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE         (1U << 16)
#define _SHR_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE        (1U << 17)
#define _SHR_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE       (1U << 18)
#define _SHR_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL                  (1U << 19)
#define _SHR_PORT_PHY_TIMESYNC_VALID_RX_LINK_DELAY                 (1U << 20)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_SYNC_FREQ            (1U << 21)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1              (1U << 22)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2              (1U << 23)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3              (1U << 24)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_LOOP_FILTER     (1U << 25)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE       (1U << 26)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA (1U << 27)
#define _SHR_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL    (1U << 28)

#define _SHR_PORT_PHY_TIMESYNC_ENABLE                               (1U<<0)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE                    (1U<<1) /* per port */
#define _SHR_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE                  (1U<<2)
#define _SHR_PORT_PHY_TIMESYNC_RX_CRC_ENABLE                        (1U<<3)
#define _SHR_PORT_PHY_TIMESYNC_8021AS_ENABLE                        (1U<<4)
#define _SHR_PORT_PHY_TIMESYNC_L2_ENABLE                            (1U<<5)
#define _SHR_PORT_PHY_TIMESYNC_IP4_ENABLE                           (1U<<6)
#define _SHR_PORT_PHY_TIMESYNC_IP6_ENABLE                           (1U<<7)
#define _SHR_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT                        (1U<<8) /* 0 - NCO FREQ CTRL REG, 1 - DPLL */
#define _SHR_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT_MODE                   (1U<<9) /* DPLL 0 - phase, 1 - freq lock */
#define _SHR_PORT_PHY_TIMESYNC_1588_ENCRYPTED_MODE                  (1U<<10)
#define _SHR_PORT_PHY_TIMESYNC_FOLLOW_UP_ASSIST_ENABLE              (1U<<11)
#define _SHR_PORT_PHY_TIMESYNC_DELAY_RESP_ASSIST_ENABLE             (1U<<12)
#define _SHR_PORT_PHY_TIMESYNC_64BIT_TIMESTAMP_ENABLE               (1U<<13)
#define _SHR_PORT_PHY_TIMESYNC_1588_OVER_HSR_ENABLE                 (1U<<14)

#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_TX_SYNC            (1U<<15)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_TX_DELAY_REQ       (1U<<16)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_TX_PDELAY_REQ      (1U<<17)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_TX_PDELAY_RESP     (1U<<18)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_RX_SYNC            (1U<<19)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_RX_DELAY_REQ       (1U<<20)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_RX_PDELAY_REQ      (1U<<21)
#define _SHR_PORT_PHY_TIMESYNC_CAPTURE_TIMESTAMP_RX_PDELAY_RESP     (1U<<22)

#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK             (1U << 0)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE                (1U << 1)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE            (1U << 2)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE           (1U << 3)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE         (1U << 4)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_UPDATE            (1U << 5)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_CAP_SRC_PORT_CLK_ID        (1U << 6)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_VLAN_ID              (1U << 7)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_SRC_PORT_NUM         (1U << 8)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_MAC_ADDR             (1U << 9)
#define _SHR_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_IP_ADDR              (1U << 10)

/* Base time structure. */
typedef struct _shr_time_spec_s {
    uint8 isnegative;   /* Sign identifier. */
    uint64 seconds;     /* Seconds absolute value. */
    uint32 nanoseconds; /* Nanoseconds absolute value. */
} _shr_time_spec_t;

/* Actions on Egress event messages */
typedef enum _shr_port_phy_timesync_event_message_egress_mode_e {
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_NONE,
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_UPDATE_CORRECTIONFIELD,
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_REPLACE_CORRECTIONFIELD_ORIGIN,
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_CAPTURE_TIMESTAMP
} _shr_port_phy_timesync_event_message_egress_mode_t;

/* Actions on ingress event messages */
typedef enum _shr_port_phy_timesync_event_message_ingress_mode_e {
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_NONE,
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_UPDATE_CORRECTIONFIELD,
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_INSERT_TIMESTAMP,
    _SHR_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_INSERT_DELAYTIME
} _shr_port_phy_timesync_event_message_ingress_mode_t;

/* Global mode actions */
typedef enum _shr_port_phy_timesync_global_mode_e {
    _SHR_PORT_PHY_TIMESYNC_MODE_FREE,
    _SHR_PORT_PHY_TIMESYNC_MODE_SYNCIN,
    _SHR_PORT_PHY_TIMESYNC_MODE_CPU
} _shr_port_phy_timesync_global_mode_t;


typedef enum _shr_port_phy_timesync_framesync_mode_s
{
  _SHR_PORT_PHY_TIMESYNC_FRAMESYNC_NONE,
  _SHR_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCIN0,
  _SHR_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCIN1,
  _SHR_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCOUT,
  _SHR_PORT_PHY_TIMESYNC_FRAMESYNC_CPU
} _shr_port_phy_timesync_framesync_mode_t;


typedef struct _shr_port_phy_framesync_s {

    _shr_port_phy_timesync_framesync_mode_t mode;
    uint32 length_threshold;
    uint32 event_offset;

} _shr_port_phy_timesync_framesync_t;


typedef enum _shr_port_phy_timesync_syncout_mode_s
{
  _SHR_PORT_PHY_TIMESYNC_SYNCOUT_DISABLE,
  _SHR_PORT_PHY_TIMESYNC_SYNCOUT_ONE_TIME,
  _SHR_PORT_PHY_TIMESYNC_SYNCOUT_PULSE_TRAIN,
  _SHR_PORT_PHY_TIMESYNC_SYNCOUT_PULSE_TRAIN_WITH_SYNC
} _shr_port_phy_timesync_syncout_mode_t;

typedef struct _shr_port_phy_timesync_syncout_s {

  _shr_port_phy_timesync_syncout_mode_t mode;
  uint16 pulse_1_length; /* in nanoseconds */
  uint16 pulse_2_length; /* in nanoseconds */
  uint32 interval; /* in nanoseconds */
  uint64 syncout_ts;

} _shr_port_phy_timesync_syncout_t;


    /* mpls_label_flags */
#define _SHR_PORT_PHY_TIMESYNC_MPLS_LABEL_IN                         (1U<<0)
#define _SHR_PORT_PHY_TIMESYNC_MPLS_LABEL_OUT                        (1U<<1)

typedef struct _shr_port_phy_timesync_mpls_label_s {

    uint32 value; /* bits [19:0] */
    uint32 mask; /* bits [19:0] */
    uint32 flags; /* label flags */

} _shr_port_phy_timesync_mpls_label_t;


    /* mpls_flags */
#define _SHR_PORT_PHY_TIMESYNC_MPLS_ENABLE                           (1U<<0)
#define _SHR_PORT_PHY_TIMESYNC_MPLS_ENTROPY_ENABLE                   (1U<<1)
#define _SHR_PORT_PHY_TIMESYNC_MPLS_SPECIAL_LABEL_ENABLE             (1U<<2)
#define _SHR_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE              (1U<<3)

typedef struct _shr_port_phy_timesync_mpls_control_s {

    uint32 flags;
    uint32 special_label; /* bits [19:0] */
    _shr_port_phy_timesync_mpls_label_t labels[10]; /* Timesync MPLS labels */
    int size; /* Number of elements in label array */

} _shr_port_phy_timesync_mpls_control_t;

#define _SHR_PORT_PHY_TIMESYNC_TN_LOAD                               (1U<<23)
#define _SHR_PORT_PHY_TIMESYNC_TN_ALWAYS_LOAD                        (1U<<22)
#define _SHR_PORT_PHY_TIMESYNC_TIMECODE_LOAD                         (1U<<21)
#define _SHR_PORT_PHY_TIMESYNC_TIMECODE_ALWAYS_LOAD                  (1U<<20)
#define _SHR_PORT_PHY_TIMESYNC_SYNCOUT_LOAD                          (1U<<19)
#define _SHR_PORT_PHY_TIMESYNC_SYNCOUT_ALWAYS_LOAD                   (1U<<18)
#define _SHR_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD                      (1U<<17)
#define _SHR_PORT_PHY_TIMESYNC_NCO_DIVIDER_ALWAYS_LOAD               (1U<<16)
#define _SHR_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD                       (1U<<15)
#define _SHR_PORT_PHY_TIMESYNC_LOCAL_TIME_ALWAYS_LOAD                (1U<<14)
#define _SHR_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD                       (1U<<13)
#define _SHR_PORT_PHY_TIMESYNC_NCO_ADDEND_ALWAYS_LOAD                (1U<<12)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD                 (1U<<11)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_ALWAYS_LOAD          (1U<<10)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD                   (1U<<9)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_ALWAYS_LOAD            (1U<<8)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD             (1U<<7)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_ALWAYS_LOAD      (1U<<6)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_K3_LOAD                          (1U<<5)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_K3_ALWAYS_LOAD                   (1U<<4)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_K2_LOAD                          (1U<<3)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_K2_ALWAYS_LOAD                   (1U<<2)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_K1_LOAD                          (1U<<1)
#define _SHR_PORT_PHY_TIMESYNC_DPLL_K1_ALWAYS_LOAD                   (1U<<0)

#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT                   (1U<<1)

#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_0                 COMPILER_64_INIT((1U<<0),0)
#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_1                 COMPILER_64_INIT((1U<<1),0)
#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_2                 COMPILER_64_INIT((1U<<2),0)
#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_3                 COMPILER_64_INIT((1U<<3),0)
#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_4                 COMPILER_64_INIT((1U<<4),0)
#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_5                 COMPILER_64_INIT((1U<<5),0)
#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_6                 COMPILER_64_INIT((1U<<6),0)
#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_7                 COMPILER_64_INIT((1U<<7),0)

#define _SHR_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT                   (1U<<0)

#define _SHR_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_MASK              (1U<<1)
#define _SHR_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT_MASK              (1U<<0)

/* Fast call actions */
typedef enum _shr_port_control_timesync_e {
    _SHR_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_FRAMESYNC,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_LOCAL_TIME,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_LOAD_CONTROL,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT_MASK,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET,
    _SHR_PORT_CONTROL_PHY_TIMESYNC_NSE_TIME_OFFSET
} _shr_port_control_phy_timesync_t;

typedef enum _shr_port_phy_timesync_msg_e {
  _SHR_PORT_PHY_TIMESYNC_MSG_SYNC,
  _SHR_PORT_PHY_TIMESYNC_MSG_DELAY_REQ,
  _SHR_PORT_PHY_TIMESYNC_MSG_PDELAY_REQ,
  _SHR_PORT_PHY_TIMESYNC_MSG_PDELAY_RESP
} _shr_port_phy_timesync_msg_t;

typedef enum _shr_port_phy_timesync_prot_e {
  _SHR_PORT_PHY_TIMESYNC_PROT_LAYER2,
  _SHR_PORT_PHY_TIMESYNC_PROT_IPV4_UDP,
  _SHR_PORT_PHY_TIMESYNC_PROT_IPV6_UDP
} _shr_port_phy_timesync_prot_t;

typedef enum _shr_port_phy_timesync_dir_e {
  _SHR_PORT_PHY_TIMESYNC_DIR_EGRESS,
  _SHR_PORT_PHY_TIMESYNC_DIR_INGRESS
} _shr_port_phy_timesync_dir_t;

typedef struct _shr_port_phy_timesync_inband_ts_control_s {
    uint32 flags;
    int    resv0_id;
} _shr_port_phy_timesync_inband_ts_control_t;

/* Base timesync configuration type. */
typedef struct _shr_port_phy_timesync_enhanced_capture_s {
    _shr_port_phy_timesync_msg_t msg_type;
    _shr_port_phy_timesync_prot_t protocol;
    _shr_port_phy_timesync_dir_t direction;
    int seq_id;
    uint32 domain;
    uint64 timestamp;
    uint64 source_port_identity;
    uint16 source_port;
    uint16 vlan_id;
} _shr_port_phy_timesync_enhanced_capture_t;

/* Base timesync configuration type. */
typedef struct _shr_port_phy_timesync_config_s {
    uint32 capabilities;                /* Flags BCM_PORT_PHY_TIMESYNC_CAP_* */
    uint32 validity_mask;               /* Flags BCM_PORT_PHY_TIMESYNC_VALID_* */
    uint32 flags;                       /* Flags BCM_PORT_PHY_TIMESYNC_* */
    uint16 itpid;                       /* 1588 inner tag */
    uint16 otpid;                       /* 1588 outer tag */
    uint16 otpid2;                      /* 1588 outer tag 2 */

    _shr_port_phy_timesync_inband_ts_control_t inband_ts_control; /* Inband TS control */

    _shr_port_phy_timesync_global_mode_t gmode; /* Global mode */

    _shr_port_phy_timesync_framesync_t framesync;
    _shr_port_phy_timesync_syncout_t syncout;
    uint16 ts_divider;

    _shr_time_spec_t original_timecode;  /* Original timecode to be inserted */
    uint32 tx_timestamp_offset;         /* TX AFE delay in ns - per port */
    uint32 rx_timestamp_offset;         /* RX AFE delay in ns - per port */
    uint32 rx_link_delay;

    _shr_port_phy_timesync_event_message_egress_mode_t tx_sync_mode; /* sync */
    _shr_port_phy_timesync_event_message_egress_mode_t tx_delay_request_mode; /* delay request */
    _shr_port_phy_timesync_event_message_egress_mode_t tx_pdelay_request_mode; /* pdelay request */
    _shr_port_phy_timesync_event_message_egress_mode_t tx_pdelay_response_mode; /* pdelay response */

    _shr_port_phy_timesync_event_message_ingress_mode_t rx_sync_mode; /* sync */
    _shr_port_phy_timesync_event_message_ingress_mode_t rx_delay_request_mode; /* delay request */
    _shr_port_phy_timesync_event_message_ingress_mode_t rx_pdelay_request_mode; /* pdelay request */
    _shr_port_phy_timesync_event_message_ingress_mode_t rx_pdelay_response_mode; /* pdelay response */

    _shr_port_phy_timesync_mpls_control_t mpls_control;

    uint32 sync_freq;   /* sync frequency */

    uint16 phy_1588_dpll_k1;
    uint16 phy_1588_dpll_k2;
    uint16 phy_1588_dpll_k3;
    uint64 phy_1588_dpll_loop_filter; /* loop filter */
    uint64 phy_1588_dpll_ref_phase;   /* ref phase */
    uint32 phy_1588_dpll_ref_phase_delta;   /* ref phase delta */

} _shr_port_phy_timesync_config_t;

/* _shr_port_phy_timesync_config_set */
extern int _shr_port_phy_timesync_config_set(
    int unit,
    _shr_port_t port,
    _shr_port_phy_timesync_config_t *conf);

/* _shr_port_phy_timesync_config_get */
extern int _shr_port_phy_timesync_config_get(
    int unit,
    _shr_port_t port,
    _shr_port_phy_timesync_config_t *conf);

/* _shr_port_control_phy_timesync_set */
extern int _shr_port_control_phy_timesync_set(
    int unit,
    _shr_port_t port,
    _shr_port_control_phy_timesync_t type,
    uint64 value);

/* _shr_port_control_phy_timesync_get */
extern int _shr_port_control_phy_timesync_get(
    int unit,
    _shr_port_t port,
    _shr_port_control_phy_timesync_t type,
    uint64 *value);

/* _shr_port_control_phy_timesync_enhanced_capture_get */
extern int _shr_port_control_phy_timesync_enhanced_capture_get(
    int unit,
    _shr_port_t port,
    _shr_port_phy_timesync_enhanced_capture_t *value);

/***************************
 * OAM related definitions
 ***************************
 */

#define _SHR_PORT_PHY_OAM_MAC_CHECK_ENABLE    (1U << 0) /* Enable MAC check for
                                                          Delay Measurement */
#define _SHR_PORT_PHY_OAM_CONTROL_WORD_ENABLE (1U << 1) /* Enable Control Word
                                                          for Delay Measurement */
#define _SHR_PORT_PHY_OAM_DM_ENTROPY_ENABLE   (1U << 2) /* Enable entropy for
                                                          Delay Measurement */
#define _SHR_PORT_PHY_OAM_DM_TS_FORMAT        (1U << 3) /* Select timestamp
                                                          format PTP(0)/NTP(1)
                                                          for Delay Measurement */

/* Port controls for phy OAM */
typedef enum _shr_port_control_phy_oam_e {
    _SHR_PORT_CONTROL_PHY_OAM_DM_TX_ETHERTYPE = 1, /* OAM Delay measurement Tx Ethertype */
    _SHR_PORT_CONTROL_PHY_OAM_DM_RX_ETHERTYPE, /* OAM Delay measurement Rx Ethertype */
    _SHR_PORT_CONTROL_PHY_OAM_DM_TX_PORT_MAC_ADDRESS_INDEX, /* Index to select the MAC address. Use
                                           index of 1 to select MacAddress1,
                                           index of 2 for MacAddress2 and so
                                           forth. */
    _SHR_PORT_CONTROL_PHY_OAM_DM_RX_PORT_MAC_ADDRESS_INDEX, /* Index to select the MAC address. Use
                                           index of 1 to select MacAddress1,
                                           index of 2 for MacAddress2 and so
                                           forth. */
    _SHR_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_1, /* OAM Delay measurement MAC address 1.
                                           There can be multiple MAC address to
                                           choose from in a PHY depending upon
                                           the design */
    _SHR_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_2, /* OAM Delay measurement MAC address 2.
                                           There can be multiple MAC address to
                                           choose from in a PHY depending upon
                                           the design */
    _SHR_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_3 /* OAM Delay measurement MAC address 3.
                                           There can be multiple MAC address to
                                           choose from in a PHY depending upon
                                           the design */
} _shr_port_control_phy_oam_t;

/* Phy OAM Delay Measurement Modes */
typedef enum _shr_port_config_phy_oam_dm_mode_e {
    _SHR_PORT_CONFIG_PHY_OAM_DM_Y1731 = 1, /* OAM Delay measurement Y.7131 Mode */
    _SHR_PORT_CONFIG_PHY_OAM_DM_BHH, /* OAM Delay measurement BHH Mode */
    _SHR_PORT_CONFIG_PHY_OAM_DM_IETF /* OAM Delay measurement IETF Mode */
} _shr_port_config_phy_oam_dm_mode_t;

/* Phy OAM Delay measurement config type. */
typedef struct _shr_port_config_phy_oam_dm_s {
    uint32 flags;                    /* OAM Delay Measurement config flags */
    _shr_port_config_phy_oam_dm_mode_t mode; /* OAM Delay Measurement mode Y.1731,
                                           BHH or IETF */
} _shr_port_config_phy_oam_dm_t;

/* Phy OAM config type. */
typedef struct _shr_port_config_phy_oam_s {
    _shr_port_config_phy_oam_dm_t tx_dm_config; /* OAM delay measurement config for Tx */
    _shr_port_config_phy_oam_dm_t rx_dm_config; /* OAM delay measurement config for Rx */
} _shr_port_config_phy_oam_t;

#ifndef BCM_HIDE_DISPATCHABLE

/* _shr_port_config_phy_oam_set */
extern int _shr_port_config_phy_oam_set(
    int unit,
    _shr_port_t port,
    _shr_port_config_phy_oam_t *conf);

/* _shr_port_config_phy_oam_get */
extern int _shr_port_config_phy_oam_get(
    int unit,
    _shr_port_t port,
    _shr_port_config_phy_oam_t *conf);

/* _shr_port_control_phy_oam_set */
extern int _shr_port_control_phy_oam_set(
    int unit,
    _shr_port_t port,
    _shr_port_control_phy_oam_t type,
    uint64 value);

/* _shr_port_control_phy_oam_get */
extern int _shr_port_control_phy_oam_get(
    int unit,
    _shr_port_t port,
    _shr_port_control_phy_oam_t type,
    uint64 *value);

#endif /* BCM_HIDE_DISPATCHABLE */

#endif	/* !_SHR_PORT_H */
