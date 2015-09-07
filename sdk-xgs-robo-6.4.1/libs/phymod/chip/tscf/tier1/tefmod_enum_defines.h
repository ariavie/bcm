/*----------------------------------------------------------------------
 * $Id: ecd0dc1f19ec4f55061259da4621dedacf241088 $
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
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 ############### THIS FILE IS AUTOMATICALLY GENERATED.  ###############
 ############### DO !! NOT !! MANUALLY EDIT THIS FILE.  ###############
 *---------------------------------------------------------------------
 * Description: This file contains enums, elems and doxyten comments
 * needed for SerDes Configuration programs.
 *---------------------------------------------------------------------
 * CVS INFORMATION:
 * Please see inc/enum_desc.txt for CVS information.
 *----------------------------------------------------------------------
 */

/* This file is automatically generated. Do not modify it. Modify the
 * inc/enum_desc.txt to change enums, elems, or comments. For issues about
 * the process that creates this file contact the tefmod development team.
 */

#ifndef _TSCFMOD_ENUM_DEFINES_H
#define _TSCFMOD_ENUM_DEFINES_H

/*! \enum tefmod_lane_select_t 

tefmod_lane_select_t selects for programming, any combination of 4 lanes of PHY.
A '1' in the latter part of the enum name selects the lane.  Other parameters in
the #tefmod_st will decide what to do with the selected  lane(s). In most cases
we enable/disable a feature on the selected lanes.

If lane_select is set to TEFMOD_LANE_BCST for writes we broadcast to all lanes.

Note that you cannot read in broadcast mode.

*/

typedef enum {
  TEFMOD_LANE_0_0_0_0        = 0   ,  /*!< No lane              selected   */
  TEFMOD_LANE_0_0_0_1              ,  /*!< lane number  0       selected   */
  TEFMOD_LANE_0_0_1_0              ,  /*!< lane number  1       selected   */
  TEFMOD_LANE_0_0_1_1              ,  /*!< lane numbers 0,1     selected   */
  TEFMOD_LANE_0_1_0_0              ,  /*!< lane number  2       selected   */
  TEFMOD_LANE_0_1_0_1              ,  /*!< lane numbers 2,0     selected   */
  TEFMOD_LANE_0_1_1_0              ,  /*!< lane numbers 2,1     selected   */
  TEFMOD_LANE_0_1_1_1              ,  /*!< lane numbers 2,1,0   selected   */
  TEFMOD_LANE_1_0_0_0              ,  /*!< lane number  3       selected   */
  TEFMOD_LANE_1_0_0_1              ,  /*!< lane numbers 3,0     selected   */
  TEFMOD_LANE_1_0_1_0              ,  /*!< lane numbers 3,1     selected   */
  TEFMOD_LANE_1_0_1_1              ,  /*!< lane numbers 3,1,0   selected   */
  TEFMOD_LANE_1_1_0_0              ,  /*!< lane numbers 3,2     selected   */
  TEFMOD_LANE_1_1_0_1              ,  /*!< lane numbers 3,2,0   selected   */
  TEFMOD_LANE_1_1_1_0              ,  /*!< lane numbers 3,2,1   selected   */
  TEFMOD_LANE_1_1_1_1              ,  /*!< lane numbers 3,2,1,0 selected   */
  TEFMOD_LANE_BCST                 ,  /*!< lane numbers 3,2,1,0 BCST       */
  TEFMOD_LANE_ILLEGAL                /*!< Illegal (programmatic boundary) */
} tefmod_lane_select_t;

/*! \def CNT_tefmod_lane_select_t Types of enum tefmod_lane_select_t */
#define CNT_tefmod_lane_select_t 18

/*!
\brief
This array returns the string version of the enum #tefmod_lane_select_t when
indexed by the enum var.

*/
extern char* e2s_tefmod_lane_select_t [CNT_tefmod_lane_select_t];
/*!
\brief
This array associates the enum #tefmod_lane_select_t enum with a bit mask.
The index is the #tefmod_lane_select_t enum value.  The value for each entry is
interpreted as follows.  If bit [n] is 1, then lane [n] is enabled; if bit [n]
is 0, then lane [n] is disabled.  By enabled, we mean that such-and-such
function is to be called for a lane; by disabled, we mean that such-and-such
function is not to be called for a lane.  So a value of 0xF indicates that all
lanes or enabled, while 0x5 indicates that only lanes 0 and 2 are enabled.

*/
extern int e2n_tefmod_lane_select_t [CNT_tefmod_lane_select_t];
/*! \enum tefmod_spd_intfc_type_t 

All legal speed-interface combination are encapsulated in this enum

There are several speed and interface combinations allowed for a logical PHY
port. Names and speeds are self explanatory.

Speed and interface selection is combined because we don't want the speeds
to be incompatible with interface.

*/

typedef enum {
  TEFMOD_SPD_ZERO            = 0   ,  /*!< Illegal value (enum boundary)   */
  TEFMOD_SPD_10_SGMII              ,  /*!< 10Mb SGMII (serial)             */
  TEFMOD_SPD_100_SGMII             ,  /*!< 100Mb SGMII (serial)            */
  TEFMOD_SPD_1000_SGMII            ,  /*!< 1Gb SGMII (serial)              */
  TEFMOD_SPD_2500                  ,  /*!< 2.5Gb  based on 1000BASE-X      */
  TEFMOD_SPD_10000_XFI             ,  /*!< 10Gb serial XFI                 */
  TEFMOD_SPD_10600_XFI_HG          ,  /*!< 10.5Gb serial XFI (HgSOLO)      */
  TEFMOD_SPD_20000_XFI             ,  /*!< 20Gb serial XFI                 */
  TEFMOD_SPD_21200_XFI_HG          ,  /*!< 21.2Gb serial XFI (HgSOLO)      */
  TEFMOD_SPD_25000_XFI             ,  /*!< 25Gb serial XFI                 */
  TEFMOD_SPD_26500_XFI_HG          ,  /*!< 26.5Gb serial XFI (HgSOLO)      */
  TEFMOD_SPD_20G_MLD_X2            ,  /*!< 20Gb serial XFI                 */
  TEFMOD_SPD_21G_MLD_HG_X2         ,  /*!< 21.5Gb serial XFI (HgSOLO)      */
  TEFMOD_SPD_40G_MLD_X2            ,  /*!< 40Gb serial XFI                 */
  TEFMOD_SPD_42G_MLD_HG_X2         ,  /*!< 42.4Gb serial XFI (HgSOLO)      */
  TEFMOD_SPD_40G_MLD_X4            ,  /*!< 40Gb serial XFI                 */
  TEFMOD_SPD_42G_MLD_HG_X4         ,  /*!< 42.4Gb serial XFI (HgSOLO)      */
  TEFMOD_SPD_50G_MLD_X2            ,  /*!< 50Gb serial XFI                 */
  TEFMOD_SPD_53G_MLD_HG_X2         ,  /*!< 53.0Gb serial XFI (HgSOLO)      */
  TEFMOD_SPD_100G_MLD_X4           ,  /*!< 100Gb serial XFI                */
  TEFMOD_SPD_106G_MLD_HG_X4        ,  /*!< 106.0Gb serial XFI (HgSOLO)     */
  TEFMOD_SPD_10000_HI              ,  /*!< 10Gb XAUI HiG                   */
  TEFMOD_SPD_10000                 ,  /*!< 10Gb XAUI                       */
  TEFMOD_SPD_12000_HI              ,  /*!< 12Gb XAUI HiG                   */
  TEFMOD_SPD_13000                 ,  /*!< 13Gb XAUI                       */
  TEFMOD_SPD_15000                 ,  /*!< 15Gb XAUI                       */
  TEFMOD_SPD_16000                 ,  /*!< 16Gb XAUI                       */
  TEFMOD_SPD_20000                 ,  /*!< 20Gb XFI                        */
  TEFMOD_SPD_20000_SCR             ,  /*!< 20Gb XAUI scrambled             */
  TEFMOD_SPD_21000                 ,  /*!< 21Gb XAUI                       */
  TEFMOD_SPD_25455                 ,  /*!< 25Gb XFI   64/66 codec          */
  TEFMOD_SPD_31500                 ,  /*!< 31.5Gb quad lane XAUI           */
  TEFMOD_SPD_40G_X4                ,  /*!< 40Gb quad lane XAUI             */
  TEFMOD_SPD_42G_X4                ,  /*!< 40Gb quad lane XAUI  HiG        */
  TEFMOD_SPD_40G_XLAUI             ,  /*!< 40Gb quad lane  MLD             */
  TEFMOD_SPD_42G_XLAUI             ,  /*!< 42Gb quad lane  MLD             */
  TEFMOD_SPD_10000_X2              ,  /*!< 10Gb dual lane                  */
  TEFMOD_SPD_10000_HI_DXGXS        ,  /*!< 10Gb dual lane XGXS HiG         */
  TEFMOD_SPD_10000_DXGXS           ,  /*!< 10Gb dual lane XGXS             */
  TEFMOD_SPD_10000_HI_DXGXS_SCR       ,  /*!< 10Gb dual lane,scrambled,HiG    */
  TEFMOD_SPD_10000_DXGXS_SCR       ,  /*!< 10Gb dual lane scrambled        */
  TEFMOD_SPD_10500_HI_DXGXS        ,  /*!< 10.5Gb dual lane XGXS HiG       */
  TEFMOD_SPD_12773_HI_DXGXS        ,  /*!< 12.73Gb dual lane XGXS HiG      */
  TEFMOD_SPD_12773_DXGXS           ,  /*!< 12.73Gb dual lane XGXS          */
  TEFMOD_SPD_15750_HI_DXGXS        ,  /*!< 15.75Gb scrambled dual lane HiG */
  TEFMOD_SPD_20G_MLD_DXGXS         ,  /*!< 20Gb dual lane MLD              */
  TEFMOD_SPD_21G_HI_MLD_DXGXS       ,  /*!< 20Gb dual lane HiG MLD          */
  TEFMOD_SPD_20G_DXGXS             ,  /*!< 20Gb dual lane BRCM             */
  TEFMOD_SPD_21G_HI_DXGXS          ,  /*!< 21.2Gb dual HiG(20+plldiv=70)   */
  TEFMOD_SPD_100G_CR10             ,  /*!< 100G                            */
  TEFMOD_SPD_120G_CR12             ,  /*!< 120G                            */
  TEFMOD_SPD_10000_XFI_HG2         ,  /*!< 10G HG2                         */
  TEFMOD_SPD_50G_MLD_X4            ,  /*!< 50G Serial XFI                  */
  TEFMOD_SPD_53G_MLD_HG_X4         ,  /*!< 53G Serial XFI (HgSOLO)         */
  TEFMOD_SPD_10000_XFI_CR1         ,  /*!< 10Gb serial XFI                 */
  TEFMOD_SPD_10600_XFI_HG_CR1       ,  /*!< 10Gb serial XFI                 */
  TEFMOD_SPD_CL73_20G              ,  /*!< 1G CL73 Auto-neg                */
  TEFMOD_SPD_CL73_25G              ,  /*!< 1G CL73 Auto-neg                */
  TEFMOD_SPD_1G_20G                ,  /*!< 1G CL36 1G                      */
  TEFMOD_SPD_1G_25G                ,  /*!< 1G CL36 1G                      */
  TEFMOD_SPD_ILLEGAL                 /*!< Illegal value (enum boundary)   */
} tefmod_spd_intfc_type_t;

/*! \def CNT_tefmod_spd_intfc_type_t Types of enum tefmod_spd_intfc_type_t */
#define CNT_tefmod_spd_intfc_type_t 61

/*!
\brief
This array returns the string version of the enum #tefmod_lane_select_t when
indexed by the enum var.

*/
extern char* e2s_tefmod_spd_intfc_type_t [CNT_tefmod_spd_intfc_type_t];
/*!
\brief
This array associates the enum #tefmod_lane_select_t enum with a bit mask.
The index is the #tefmod_lane_select_t enum value.  The value for each entry is
interpreted as follows.  If bit [n] is 1, then lane [n] is enabled; if bit [n]
is 0, then lane [n] is disabled.  By enabled, we mean that such-and-such
function is to be called for a lane; by disabled, we mean that such-and-such
function is not to be called for a lane.  So a value of 0xF indicates that all
lanes or enabled, while 0x5 indicates that only lanes 0 and 2 are enabled.

*/
extern int e2n_tefmod_spd_intfc_type_t [CNT_tefmod_spd_intfc_type_t];
/*! \enum tefmod_regacc_type_t 

Types of MDIO to access PHY registers. IEEE clause 22 and clause 45 are
supported. Selecting clause 22 selects enhanced clause 22 mode with extensions
for 64K registers.

*/

typedef enum {
  TEFMOD_REGACC_CL22         = 0   ,  /*!< IEEE clause 22 based MDIO (for PMD only) */
  TEFMOD_REGACC_CL45               ,  /*!< IEEE clause 45 based MDIO (for PMD only) */
  TEFMOD_REGACC_TOTSC              ,  /*!< Mission mode */
  TEFMOD_REGACC_SBUS_FD            ,  /*!< Probably used in PM testbenches */
  TEFMOD_REGACC_SBUS_BD            ,  /*!< Probably used in PM testbenches */
  TEFMOD_REGACC_ILLEGAL              /*!< Illegal value (enum boundary) */
} tefmod_regacc_type_t;

/*! \def CNT_tefmod_regacc_type_t Types of enum tefmod_regacc_type_t */
#define CNT_tefmod_regacc_type_t 6

/*!
\brief
This array returns the string version of the enum #tefmod_mdio_type when indexed
by the enum var.

*/
extern char* e2s_tefmod_regacc_type_t [CNT_tefmod_regacc_type_t];
/*! \enum tefmod_port_type_t 

This is the port mode type enumeration.

WC can be configured in combo mode (i.e. entire WC is a single port) or
independent mode (i.e. WC has more than one port (2, 3, or 4) that are
controlled individually.

*/

typedef enum {
  TEFMOD_MULTI_PORT          = 0   ,  /*!< Each channel is one logical port */
  TEFMOD_TRI1_PORT                 ,  /*!< 3 ports, one of them paird as follows (0,1,2-3) */
  TEFMOD_TRI2_PORT                 ,  /*!< 3 ports, one of them paird as follows (0-1,2,3) */
  TEFMOD_DXGXS                     ,  /*!< Each paired channel(0-1, 2-3) is one logical port */
  TEFMOD_SINGLE_PORT               ,  /*!< single port mode: 4 channels as one logical port */
  TEFMOD_PORT_MODE_ILLEGAL           /*!< Illegal value (enum boundary) */
} tefmod_port_type_t;

/*! \def CNT_tefmod_port_type_t Types of enum tefmod_port_type_t */
#define CNT_tefmod_port_type_t 6

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_port_type_t [CNT_tefmod_port_type_t];
/*! \enum tefmod_sc_mode_type_t 

This is the sc mode type enumeration.



*/

typedef enum {
  TEFMOD_SC_MODE_HT_WITH_BASIC_OVERRIDE = 0   ,  /*!< TEFMOD_SC_MODE_HT_WITH_BASIC_OVERRIDE */
  TEFMOD_SC_MODE_HT_OVERRIDE       ,  /*!< TEFMOD_SC_MODE_ST_OVERRIDE */
  TEFMOD_SC_MODE_ST                ,  /*!< TEFMOD_SC_MODE_ST */
  TEFMOD_SC_MODE_ST_OVERRIDE       ,  /*!< TEFMOD_SC_MODE_ST */
  TEFMOD_SC_MODE_AN_CL37           ,  /*!< TEFMOD_SC_MODE_AN_CL73 */
  TEFMOD_SC_MODE_AN_CL73           ,  /*!< TEFMOD_SC_MODE_AN_CL73 */
  TEFMOD_SC_MODE_BYPASS            ,  /*!< TEFMOD_SC_MODE_BYPASS */
  TEFMOD_SC_MODE_ILLEGAL             /*!< Illegal value (enum boundary) */
} tefmod_sc_mode_type_t;

/*! \def CNT_tefmod_sc_mode_type_t Types of enum tefmod_sc_mode_type_t */
#define CNT_tefmod_sc_mode_type_t 8

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_sc_mode_type_t [CNT_tefmod_sc_mode_type_t];
/*! \enum tefmod_diag_type_t 

tefmod_diag_type_t enumerates categories of diagnostic data.
has many intermediate stages between down and up. This enum is work in progress


<table cellspacing=0>
<tr><td colspan=3><B>'per_lane_control' bit-mappings</B></td></tr>

<tr><td><B>Type</B></td><td><B>Description</B></td><td><B>Scope</B></td></tr>

<tr><td>General</td>
<td> Combo/independent, Device and Revision Id, VCO settings, Firmware state
and version, active/passive lanes, MDIO type. PLL info, Oversampling Info</td>
<td>Device</td></tr>

<tr><td>Link</td>
<td> Speeds, oversampling, interface, forced/Autoneg, link status, sync
status, RX sequencer on/off </td>
<td>Lane</td></tr>

<tr><td>Autoneg</td>
<td> Local and remote advertisement, link status, cl73/37/BAM info </td>
<td>Lane</td></tr>

<tr><td>Internal Traffic</td>
<td> PRBS type, CJPat Type, Prog_data value, Any associated recorded errors,
and misc. info (IPG etc.)</td>
<td>Lane </td></tr>

<tr>
<td>DFE</td>
<td>Equalization info, Tap settings. (pre/post/overrides), peaking filter values </td>
<td>Lane</td></tr>

<tr><td>IEEE info</td>
<td>Clause 72, FEC</td>
<td>Lane</td></tr>

<tr><td>Topology</td>
<td>Looping info (Gloop/rloop), lane swapping, polarity swap info</td>
<td>Device</td></tr>

<tr><td>EEE</td>
<td>EEE full, passthru modes, some window values?</td>
<td>Lane</td></tr>

<tr><td>Eye Margin</td>
<td>Eye margin measurement (readout only)</td>
<td>Lane</td></tr>

<tr><td>All</td>
<td>All of the above. Except eye margin.</td>
<td>Device</td></tr>

</table>


*/

typedef enum {
  TEFMOD_DIAG_GENERAL        = 0x00000001 ,  /*!< General device wide information.         */
  TEFMOD_DIAG_TOPOLOGY       = 0x00000002 ,  /*!< Loopbacks etc.                           */
  TEFMOD_DIAG_LINK           = 0x00000004 ,  /*!< Link specific info.                      */
  TEFMOD_DIAG_SPEED          = 0x00000008 ,  /*!< sub-category of TEFMOD_DIAG_LINK(for SDK) */
  TEFMOD_DIAG_ANEG           = 0x00000010 ,  /*!< Autoneg specific info.                   */
  TEFMOD_DIAG_TFC            = 0x00000020 ,  /*!< State of tx/rx internal tfc              */
  TEFMOD_DIAG_AN_TIMERS      = 0x00000040 ,  /*!< AN timers */
  TEFMOD_DIAG_STATE          = 0x00000080 ,  /*!< Debug state registers */
  TEFMOD_DIAG_DEBUG          = 0x00000100 ,  /*!< Debug */
  TEFMOD_DIAG_IEEE           = 0x00000200 ,  /*!< IEEE related info                        */
  TEFMOD_DIAG_EEE            = 0x00000400 ,  /*!< EEE                                      */
  TEFMOD_SERDES_DIAG         = 0x00000800 ,  /*!< PMD Triage */
  TEFMOD_DIAG_ALL            = 0x00000fff ,  /*!< Everything but eye margin                */
  TEFMOD_DIAG_ILLEGAL        = 0x00000000   /*!< Illegal value. programmatic boundary.    */
} tefmod_diag_type_t;

/*! \def CNT_tefmod_diag_type_t Types of enum tefmod_diag_type_t */
#define CNT_tefmod_diag_type_t 14

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_diag_type_t [CNT_tefmod_diag_type_t];
/*! \enum tefmod_model_type_t 

tefmod_model_type_t enumerates different generations and  revisions of PHY.
Note that we pretty much only look at different versions of PHY.

We rely on all the information being in the SERDES_ID register.


*/

typedef enum {
  TEFMOD_WC                  = 0   ,  /*!< Generic PHY (any model) */
  TEFMOD_WC_A0                     ,  /*!< PHY Version A0 */
  TEFMOD_WC_A1                     ,  /*!< PHY Version A1 */
  TEFMOD_WC_A2                     ,  /*!< PHY Version A2 */
  TEFMOD_WC_B0                     ,  /*!< PHY Version B0 */
  TEFMOD_WC_B1                     ,  /*!< PHY Version B1 */
  TEFMOD_WC_B2                     ,  /*!< PHY Version B2 */
  TEFMOD_WC_C0                     ,  /*!< PHY Version C0, model no. differs from other WCs */
  TEFMOD_WC_C1                     ,  /*!< PHY Version C1, model no. differs from other WCs */
  TEFMOD_WC_C2                     ,  /*!< PHY Version C2, model no. differs from other WCs */
  TEFMOD_WC_D0                     ,  /*!< PHY Version D0 */
  TEFMOD_WC_D1                     ,  /*!< PHY Version D1 */
  TEFMOD_WC_D2                     ,  /*!< PHY Version D2 */
  TEFMOD_XN                        ,  /*!< Generic Xenia Core (any model) */
  TEFMOD_WL                        ,  /*!< Generic PHY Lite (any model) */
  TEFMOD_WL_A0                     ,  /*!< WarpLite Core */
  TEFMOD_QS                        ,  /*!< Generic QSGMII core (any model) */
  TEFMOD_QS_A0                     ,  /*!< QSGMII core Version A0 */
  TEFMOD_QS_B0                     ,  /*!< QSGMII core Version B0 */
  TEFMOD_MODEL_TYPE_ILLEGAL          /*!< Illegal value. programmatic boundary. */
} tefmod_model_type_t;

/*! \def CNT_tefmod_model_type_t Types of enum tefmod_model_type_t */
#define CNT_tefmod_model_type_t 20

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_model_type_t [CNT_tefmod_model_type_t];
/*! \enum tefmod_an_type_t 

tefmod_an_type_t enumerates different types of autoneg modes in the PHY.

*/

typedef enum {
  TEFMOD_CL73                = 0   ,  /*!< PHY autoneg CL73 enable */
  TEFMOD_CL37                      ,  /*!< PHY autoneg CL37 enable */
  TEFMOD_CL73_BAM                  ,  /*!< PHY autoneg CL73 enable with Brcm Aneg Mode */
  TEFMOD_CL37_BAM                  ,  /*!< PHY autoneg CL37 enable with  Brcm Aneg Mode */
  TEFMOD_CL37_SGMII                ,  /*!< PHY autoneg for SGMII */
  TEFMOD_HPAM                      ,  /*!< PHY autoneg for HP aneg */
  TEFMOD_AN_TYPE_ILLEGAL             /*!< Illegal value. programmatic boundary. */
} tefmod_an_type_t;

/*! \def CNT_tefmod_an_type_t Types of enum tefmod_an_type_t */
#define CNT_tefmod_an_type_t 7

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_an_type_t [CNT_tefmod_an_type_t];
/*! \enum tefmod_eye_direction_t 

The direction of slicer changing always moves from the middle of the eye.
Currently all speeds have the following bit positions in tefmod_st

\li TEFMOD_EYE_VU: Vertical,   Upward direction
\li TEFMOD_EYE_VD: Vertical,   Downward direction
\li TEFMOD_EYE_HL: Horizontal, Left  direction
\li TEFMOD_EYE_HR: Horizontal, Right direction


*/

typedef enum {
  TEFMOD_EYE_VU              = 0   ,  /*!< Vertical,   Upward direction */
  TEFMOD_EYE_VD                    ,  /*!< Vertical,   Downward direction */
  TEFMOD_EYE_HL                    ,  /*!< Horizontal, Left  direction */
  TEFMOD_EYE_HR                    ,  /*!< Horizontal, Right direction */
  TEFMOD_EYE_ILLEGAL                 /*!< Programmatic illegal boundary. */
} tefmod_eye_direction_t;

/*! \def CNT_tefmod_eye_direction_t Types of enum tefmod_eye_direction_t */
#define CNT_tefmod_eye_direction_t 5

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_eye_direction_t [CNT_tefmod_eye_direction_t];


typedef enum {
  TEFMOD_ASPD_10M            = 0   ,  /*!< No documentation   */
  TEFMOD_ASPD_100M                 ,  /*!< No documentation   */
  TEFMOD_ASPD_1000M                ,  /*!< No documentation   */
  TEFMOD_ASPD_2p5G_X1              ,  /*!< No documentation   */
  TEFMOD_ASPD_5G_X4                ,  /*!< No documentation   */
  TEFMOD_ASPD_6G_X4                ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_CX4              ,  /*!< No documentation   */
  TEFMOD_ASPD_12G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_12p5G_X4             ,  /*!< No documentation   */
  TEFMOD_ASPD_13G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_15G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_16G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_1G_KX1               ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_KX4              ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_KR1              ,  /*!< No documentation   */
  TEFMOD_ASPD_5G_X1                ,  /*!< No documentation   */
  TEFMOD_ASPD_6p36G_X1             ,  /*!< No documentation   */
  TEFMOD_ASPD_20G_CX4              ,  /*!< No documentation   */
  TEFMOD_ASPD_21G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_25p45G_X4            ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_X2_NOSCRAMBLE       ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_CX2_NOSCRAMBLE       ,  /*!< No documentation   */
  TEFMOD_ASPD_10p5G_X2             ,  /*!< No documentation   */
  TEFMOD_ASPD_10p5G_CX2_NOSCRAMBLE       ,  /*!< No documentation   */
  TEFMOD_ASPD_12p7G_X2             ,  /*!< No documentation   */
  TEFMOD_ASPD_12p7G_CX2            ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_X1               ,  /*!< No documentation   */
  TEFMOD_ASPD_40G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_20G_X2               ,  /*!< No documentation   */
  TEFMOD_ASPD_20G_CX2              ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_SFI              ,  /*!< No documentation   */
  TEFMOD_ASPD_31p5G_X4             ,  /*!< No documentation   */
  TEFMOD_ASPD_32p7G_X4             ,  /*!< No documentation   */
  TEFMOD_ASPD_20G_X4               ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_X2               ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_CX2              ,  /*!< No documentation   */
  TEFMOD_ASPD_12G_SCO_R2           ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_SCO_X2           ,  /*!< No documentation   */
  TEFMOD_ASPD_40G_KR4              ,  /*!< No documentation   */
  TEFMOD_ASPD_40G_CR4              ,  /*!< No documentation   */
  TEFMOD_ASPD_100G_CR10            ,  /*!< No documentation   */
  TEFMOD_ASPD_5G_X2                ,  /*!< No documentation   */
  TEFMOD_ASPD_15p75G_X2            ,  /*!< No documentation   */
  TEFMOD_ASPD_2G_FC                ,  /*!< No documentation   */
  TEFMOD_ASPD_4G_FC                ,  /*!< No documentation   */
  TEFMOD_ASPD_8G_FC                ,  /*!< No documentation   */
  TEFMOD_ASPD_10G_CX1              ,  /*!< No documentation   */
  TEFMOD_ASPD_1G_CX1               ,  /*!< No documentation   */
  TEFMOD_ASPD_20G_KR2              ,  /*!< No documentation   */
  TEFMOD_ASPD_20G_CR2              ,  /*!< No documentation   */
  TEFMOD_ASPD_TYPE_ILLEGAL           /*!< Programmatic illegal boundary. */
} tefmod_aspd_type_t;

/*! \def CNT_tefmod_aspd_type_t Types of enum tefmod_aspd_type_t */
#define CNT_tefmod_aspd_type_t 52

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_aspd_type_t [CNT_tefmod_aspd_type_t];
/*!
\brief
This array associates the enum #tefmod_lane_select_t enum with a bit mask.
The index is the #tefmod_lane_select_t enum value.  The value for each entry is
interpreted as follows.  If bit [n] is 1, then lane [n] is enabled; if bit [n]
is 0, then lane [n] is disabled.  By enabled, we mean that such-and-such
function is to be called for a lane; by disabled, we mean that such-and-such
function is not to be called for a lane.  So a value of 0xF indicates that all
lanes or enabled, while 0x5 indicates that only lanes 0 and 2 are enabled.

*/
extern int e2n_tefmod_aspd_type_t [CNT_tefmod_aspd_type_t];
/*! \enum tefmod_tech_ability_t 

tefmod_tech_ability_t enumerates different types of speed advertisements in the
basic autoneg page. Currently for CL73 only.

Currently all speeds have the following bit positions in tefmod_st

\li TEFMOD_ABILITY_1G               Bit Pos: 0
\li TEFMOD_ABILITY_10G_KR           Bit Pos: 1
\li TEFMOD_ABILITY_40G_KR4          Bit Pos: 2
\li TEFMOD_ABILITY_40G_CR4          Bit Pos: 3
\li TEFMOD_ABILITY_100G_CR10        Bit Pos: 4
\li TEFMOD_ABILITY_10G_HIGIG        Bit Pos: 5
\li TEFMOD_ABILITY_10G_CX4          Bit Pos: 6
\li TEFMOD_ABILITY_20G_X4           Bit Pos: 7
\li TEFMOD_ABILITY_40G              Bit Pos: 8
\li TEFMOD_ABILITY_25P455G          Bit Pos: 9
\li TEFMOD_ABILITY_21G_X4           Bit Pos: 10
\li TEFMOD_ABILITY_20G_X4S          Bit Pos: 11
\li TEFMOD_ABILITY_10G_DXGXS_HIGIG  Bit Pos: 12
\li TEFMOD_ABILITY_10G_DXGXS        Bit Pos: 13
\li TEFMOD_ABILITY_10P5G_DXGXS      Bit Pos: 14
\li TEFMOD_ABILITY_12P5_DXGXS       Bit Pos: 15
\li TEFMOD_ABILITY_20G_KR2_HIGIG    Bit Pos: 16
\li TEFMOD_ABILITY_20G_KR2          Bit Pos: 17
\li TEFMOD_ABILITY_20G_CR2          Bit Pos: 18
\li TEFMOD_ABILITY_15P75G_R2        Bit Pos: 19
\li TEFMOD_ABILITY_100G_KR4         Bit Pos: 20 
\li TEFMOD_ABILITY_100G_CR4         Bit Pos: 21 
\li TEFMOD_ABILITY_40G_KR2          Bit Pos: 22 
\li TEFMOD_ABILITY_40G_CR2          Bit Pos: 23 
\li TEFMOD_ABILITY_50G_KR2          Bit Pos: 24 
\li TEFMOD_ABILITY_50G_CR2          Bit Pos: 25 
\li TEFMOD_ABILITY_50G_KR4          Bit Pos: 26 
\li TEFMOD_ABILITY_50G_CR4          Bit Pos: 27 
\li TEFMOD_ABILITY_20G_KR1          Bit Pos: 28 
\li TEFMOD_ABILITY_20G_CR1          Bit Pos: 29 
\li TEFMOD_ABILITY_25G_KR1          Bit Pos: 30 
\li TEFMOD_ABILITY_25G_CR1          Bit Pos: 31 


*/

typedef enum {
  TEFMOD_ABILITY_1G          = 0   ,  /*!< please write comments        */
  TEFMOD_ABILITY_10G_KR            ,  /*!< please write comments        */
  TEFMOD_ABILITY_40G_KR4           ,  /*!< please write comments        */
  TEFMOD_ABILITY_40G_CR4           ,  /*!< please write comments        */
  TEFMOD_ABILITY_100G_CR10         ,  /*!< please write comments        */
  TEFMOD_ABILITY_10G_HIGIG         ,  /*!< please write comments        */
  TEFMOD_ABILITY_10G_CX4           ,  /*!< please write comments        */
  TEFMOD_ABILITY_20G_X4            ,  /*!< please write comments        */
  TEFMOD_ABILITY_40G               ,  /*!< please write comments        */
  TEFMOD_ABILITY_25P455G           ,  /*!< please write comments        */
  TEFMOD_ABILITY_21G_X4            ,  /*!< please write comments        */
  TEFMOD_ABILITY_20G_X4S           ,  /*!< XAUI 8b10b scrambled data    */
  TEFMOD_ABILITY_10G_DXGXS_HIGIG       ,  /*!< please write comments        */
  TEFMOD_ABILITY_10G_DXGXS         ,  /*!< please write comments        */
  TEFMOD_ABILITY_10P5G_DXGXS       ,  /*!< please write comments        */
  TEFMOD_ABILITY_12P5_DXGXS        ,  /*!< please write comments        */
  TEFMOD_ABILITY_20G_KR2_HIGIG       ,  /*!< please write comments        */
  TEFMOD_ABILITY_20G_KR2           ,  /*!< please write comments        */
  TEFMOD_ABILITY_20G_CR2           ,  /*!< please write comments        */
  TEFMOD_ABILITY_15P75G_R2         ,  /*!< please write comments        */
  TEFMOD_ABILITY_100G_KR4          ,  /*!< please write comments        */
  TEFMOD_ABILITY_100G_CR4          ,  /*!< please write comments        */
  TEFMOD_ABILITY_40G_KR2           ,  /*!< please write comments        */
  TEFMOD_ABILITY_40G_CR2           ,  /*!< please write comments        */
  TEFMOD_ABILITY_50G_KR2           ,  /*!< please write comments        */
  TEFMOD_ABILITY_50G_CR2           ,  /*!< please write comments        */
  TEFMOD_ABILITY_50G_KR4           ,  /*!< please write comments        */
  TEFMOD_ABILITY_50G_CR4           ,  /*!< please write comments        */
  TEFMOD_ABILITY_20G_KR1           ,  /*!< please write comments        */
  TEFMOD_ABILITY_20G_CR1           ,  /*!< please write comments        */
  TEFMOD_ABILITY_25G_KR1           ,  /*!< please write comments        */
  TEFMOD_ABILITY_25G_CR1           ,  /*!< please write comments        */
  TEFMOD_ABILITY_ILLEGAL             /*!< Illegal. Programmatic boundary */
} tefmod_tech_ability_t;

/*! \def CNT_tefmod_tech_ability_t Types of enum tefmod_tech_ability_t */
#define CNT_tefmod_tech_ability_t 33

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_tech_ability_t [CNT_tefmod_tech_ability_t];
/*!
\brief
This array associates the enum #tefmod_lane_select_t enum with a bit mask.
The index is the #tefmod_lane_select_t enum value.  The value for each entry is
interpreted as follows.  If bit [n] is 1, then lane [n] is enabled; if bit [n]
is 0, then lane [n] is disabled.  By enabled, we mean that such-and-such
function is to be called for a lane; by disabled, we mean that such-and-such
function is not to be called for a lane.  So a value of 0xF indicates that all
lanes or enabled, while 0x5 indicates that only lanes 0 and 2 are enabled.

*/
extern int e2n_tefmod_tech_ability_t [CNT_tefmod_tech_ability_t];
/*! \enum tefmod_cl37bam_ability_t 

This array associates the enum #tefmod_lane_select_t enum with a bit mask.
The index is the #tefmod_lane_select_t enum value.  The value for each entry is
interpreted as follows.  If bit [n] is 1, then lane [n] is enabled; if bit [n]
is 0, then lane [n] is disabled.  Enabled implies the function under
consideration be enebled for the lane. Disabled means the opposite.  So a value
of 0xF indicates that all lanes or enabled, while 0x5 indicates that only lanes
0 and 2 are enabled.

tefmod_cl37bam_ability_t enumerates different types of speed advertisements in the
basic autoneg page for CL37BAM only.

Currently all speeds have the following bit positions in tefmod_st

\li TEFMOD_BAM37ABL_2P5G             Bit Pos: 0
\li TEFMOD_BAM37ABL_5G_X4            Bit Pos: 1
\li TEFMOD_BAM37ABL_6G_X4            Bit Pos: 2
\li TEFMOD_BAM37ABL_10G_HIGIG        Bit Pos: 3
\li TEFMOD_BAM37ABL_10G_CX4          Bit Pos: 4
\li TEFMOD_BAM37ABL_12G_X4           Bit Pos: 5
\li TEFMOD_BAM37ABL_12P5_X4          Bit Pos: 6
\li TEFMOD_BAM37ABL_13G_X4           Bit Pos: 7
\li TEFMOD_BAM37ABL_15G_X4           Bit Pos: 8
\li TEFMOD_BAM37ABL_16G_X4           Bit Pos: 9
\li TEFMOD_BAM37ABL_20G_X4_CX4       Bit Pos: 10
\li TEFMOD_BAM37ABL_20G_X4           Bit Pos: 11
\li TEFMOD_BAM37ABL_21G_X4           Bit Pos: 12
\li TEFMOD_BAM37ABL_25P455G          Bit Pos: 13
\li TEFMOD_BAM37ABL_31P5G            Bit Pos: 14
\li TEFMOD_BAM37ABL_32P7G            Bit Pos: 15
\li TEFMOD_BAM37ABL_40G              Bit Pos: 16
\li TEFMOD_BAM37ABL_10G_X2_CX4       Bit Pos: 17
\li TEFMOD_BAM37ABL_10G_DXGXS        Bit Pos: 18
\li TEFMOD_BAM37ABL_10P5G_DXGXS      Bit Pos: 19
\li TEFMOD_BAM37ABL_12P7_DXGXS       Bit Pos: 20
\li TEFMOD_BAM37ABL_15P75G_R2        Bit Pos: 21
\li TEFMOD_BAM37ABL_20G_X2_CX4       Bit Pos: 22
\li TEFMOD_BAM37ABL_20G_X2           Bit Pos: 23


*/

typedef enum {
  TEFMOD_BAM37ABL_2P5G       = 0   ,  /*!< X1 BRCM */
  TEFMOD_BAM37ABL_5G_X4            ,  /*!< BRCM */
  TEFMOD_BAM37ABL_6G_X4            ,  /*!< BRCM */
  TEFMOD_BAM37ABL_10G_HIGIG        ,  /*!< HG (10G_X4) */
  TEFMOD_BAM37ABL_10G_CX4          ,  /*!< (10G_X4_CX4) */
  TEFMOD_BAM37ABL_12G_X4           ,  /*!< HG */
  TEFMOD_BAM37ABL_12P5_X4          ,  /*!< HG */
  TEFMOD_BAM37ABL_13G_X4           ,  /*!< HG */
  TEFMOD_BAM37ABL_15G_X4           ,  /*!< HG */
  TEFMOD_BAM37ABL_16G_X4           ,  /*!< HG */
  TEFMOD_BAM37ABL_20G_X4_CX4       ,  /*!< XAUI 8b10b scram(20G_X4S) */
  TEFMOD_BAM37ABL_20G_X4           ,  /*!< HG */
  TEFMOD_BAM37ABL_21G_X4           ,  /*!< HG */
  TEFMOD_BAM37ABL_25P455G          ,  /*!< X4 HG */
  TEFMOD_BAM37ABL_31P5G            ,  /*!< X4 HG */
  TEFMOD_BAM37ABL_32P7G            ,  /*!< X4 HG */
  TEFMOD_BAM37ABL_40G              ,  /*!< X4 BRCM HG (40G_X4) */
  TEFMOD_BAM37ABL_10G_X2_CX4       ,  /*!< 10G_X2_CX4 */
  TEFMOD_BAM37ABL_10G_DXGXS        ,  /*!< 10G_X2 */
  TEFMOD_BAM37ABL_10P5G_DXGXS       ,  /*!< 10P5_X2 */
  TEFMOD_BAM37ABL_12P7_DXGXS       ,  /*!< 12P7_X2 */
  TEFMOD_BAM37ABL_15P75G_R2        ,  /*!< 15P75_X2 */
  TEFMOD_BAM37ABL_20G_X2_CX4       ,  /*!< 20G_X2_CX4 */
  TEFMOD_BAM37ABL_20G_X2           ,  /*!< 20G_X2 */
  TEFMOD_BAM37ABL_ILLEGAL            /*!< Illegal. Programmatic boundary */
} tefmod_cl37bam_ability_t;

/*! \def CNT_tefmod_cl37bam_ability_t Types of enum tefmod_cl37bam_ability_t */
#define CNT_tefmod_cl37bam_ability_t 25

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_cl37bam_ability_t [CNT_tefmod_cl37bam_ability_t];
/*! \enum tefmod_diag_an_type_t 

This array associates the enum #tefmod_lane_select_t enum with a bit mask.
The index is the #tefmod_lane_select_t enum value.  The value for each entry is
interpreted as follows.  If bit [n] is 1, then lane [n] is enabled; if bit [n]
is 0, then lane [n] is disabled.  By enabled, we mean that such-and-such
function is to be called for a lane; by disabled, we mean that such-and-such
function is not to be called for a lane.  So a value of 0xF indicates that all
lanes or enabled, while 0x5 indicates that only lanes 0 and 2 are enabled.

*/

typedef enum {
  TEFMOD_DIAG_AN_DONE        = 0   ,  /*!< AN completion check */
  TEFMOD_DIAG_AN_HCD               ,  /*!< AN HCD speed check */
  TEFMOD_DIAG_AN_TYPE_ILLEGAL         /*!< Illegal value. programmatic boundary */
} tefmod_diag_an_type_t;

/*! \def CNT_tefmod_diag_an_type_t Types of enum tefmod_diag_an_type_t */
#define CNT_tefmod_diag_an_type_t 3

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_diag_an_type_t [CNT_tefmod_diag_an_type_t];
/*! \enum tefmod_tier1_function_type_t 

This array associates the enum #tefmod_lane_select_t enum with a bit mask.
The index is the #tefmod_lane_select_t enum value.  The value for each entry is
interpreted as follows.  If bit [n] is 1, then lane [n] is enabled; if bit [n]
is 0, then lane [n] is disabled.  By enabled, we mean that such-and-such
function is to be called for a lane; by disabled, we mean that such-and-such
function is not to be called for a lane.  So a value of 0xF indicates that all
lanes or enabled, while 0x5 indicates that only lanes 0 and 2 are enabled.

*/

typedef enum {
  PCS_BYPASS_CTL             = 0   ,  /*!< tefmod_pcs_bypass_ctl */
  CREDIT_SET                       ,  /*!< tefmod_credit_set */
  ENCODE_SET                       ,  /*!< tefmod_encode_set */
  DECODE_SET                       ,  /*!< tefmod_decode_set */
  CREDIT_CONTROL                   ,  /*!< tefmod_credit_control */
  AFE_SPEED_UP_DSC_VGA             ,  /*!< tefmod_afe_speed_up_dsc_vga */
  TX_LANE_CONTROL                  ,  /*!< tefmod_tx_lane_control */
  RX_LANE_CONTROL                  ,  /*!< tefmod_rx_lane_control */
  TX_LANE_DISABLE                  ,  /*!< tefmod_tx_lane_disable */
  POWER_CONTROL                    ,  /*!< tefmod_power_control */
  AUTONEG_SET                      ,  /*!< tefmod_autoneg_set */
  AUTONEG_GET                      ,  /*!< tefmod_autoneg_get */
  AUTONEG_CONTROL                  ,  /*!< tefmod_autoneg_control */
  AUTONEG_PAGE_SET                 ,  /*!< tefmod_autoneg_page_set */
  REG_READ                         ,  /*!< tefmod_reg_read */
  REG_WRITE                        ,  /*!< tefmod_reg_write */
  PRBS_CHECK                       ,  /*!< tefmod_prbs_check */
  CJPAT_CRPAT_CONTROL              ,  /*!< tefmod_cjpat_crpat_control */
  CJPAT_CRPAT_CHECK                ,  /*!< tefmod_cjpat_crpat_check */
  TEFMOD_DIAG                      ,  /*!< tefmod_diag */
  LANE_SWAP                        ,  /*!< tefmod_lane_swap */
  PARALLEL_DETECT_CONTROL          ,  /*!< tefmod_parallel_detect_control */
  CLAUSE_72_CONTROL                ,  /*!< tefmod_clause72_control */
  PLL_SEQUENCER_CONTROL            ,  /*!< tefmod_pll_sequencer_control */
  PLL_LOCK_WAIT                    ,  /*!< tefmod_pll_lock_wait */
  DUPLEX_CONTROL                   ,  /*!< tefmod_duplex_control */
  REVID_READ                       ,  /*!< tefmod_revid_read */
  BYPASS_SC                        ,  /*!< tefmod_bypass_sc */
  SET_POLARITY                     ,  /*!< tefmod_tx_rx_polarity */
  SET_PORT_MODE                    ,  /*!< tefmod_set_port_mode */
  SET_AN_PORT_MODE                 ,  /*!< tefmod_set_an_port_mode */
  PRBS_CONTROL                     ,  /*!< tefmod_prbs_control */
  PRBS_MODE                        ,  /*!< tefmod_prbs_mode */
  SOFT_RESET                       ,  /*!< tefmod_soft_reset */
  SET_SPD_INTF                     ,  /*!< tefmod_set_spd_intf */
  TX_BERT_CONTROL                  ,  /*!< tefmod_tx_bert_control */
  RX_LOOPBACK_CONTROL              ,  /*!< tefmod_rx_loopback_control */
  RX_PMD_LOOPBACK_CONTROL          ,  /*!< tefmod_rx_pmd_loopback_control */
  TX_LOOPBACK_CONTROL              ,  /*!< tefmod_tx_loopback_control */
  TX_PMD_LOOPBACK_CONTROL          ,  /*!< tefmod_tx_pmd_loopback_control */
  CORE_RESET                       ,  /*!< tefmod_core_reset */
  REFCLK_SET                       ,  /*!< tefmod_refclk_set */
  WAIT_PMD_LOCK                    ,  /*!< tefmod_wait_pmd_lock */
  FIRMWARE_SET                     ,  /*!< tefmod_firmware_set */
  INIT_PCS_FALCON                  ,  /*!< tefmod_init_pcs_falcon */
  DISABLE_PCS_FALCON               ,  /*!< tefmod_disable_pcs_falcon */
  INIT_PMD_FALCON                  ,  /*!< tefmod_init_pmd_falcon */
  PMD_LANE_SWAP_TX                 ,  /*!< tefmod_pmd_lane_swap_tx */
  PMD_LANE_SWAP                    ,  /*!< tefmod_pmd_lane_swap */
  PCS_LANE_SWAP                    ,  /*!< tefmod_pcs_lane_swap */
  SET_SC_SPEED                     ,  /*!< tefmod_set_sc_speed */
  CHECK_SC_STATS                   ,  /*!< tefmod_check_status */
  SET_OVERRIDE_0                   ,  /*!< tefmod_set_override_0 */
  SET_OVERRIDE_1                   ,  /*!< tefmod_set_override_1 */
  PMD_RESET_REMOVE                 ,  /*!< tefmod_pmd_reset_remove */
  PMD_RESET_BYPASS                 ,  /*!< tefmod_pmd_reset_bypass */
  INIT_PCS_ILKN                    ,  /*!< tefmod_init_pcs_ilkn */
  TOGGLE_SW_SPEED_CHANGE           ,  /*!< tefmod_toggle_sw_speed_change */
  TIER1_FUNCTION_ILLEGAL             /*!< illegal */
} tefmod_tier1_function_type_t;

/*! \def CNT_tefmod_tier1_function_type_t Types of enum tefmod_tier1_function_type_t */
#define CNT_tefmod_tier1_function_type_t 59

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_tier1_function_type_t [CNT_tefmod_tier1_function_type_t];
/*! \enum tefmod_fec_en_parm_t 

For passing parameters to tefmod_FEC_control.

*/

typedef enum {
  TEFMOD_CL91_TX_EN_DIS      = 0x01 ,  /*!< Enable / Disable CL91 TX */
  TEFMOD_CL91_RX_EN_DIS      = 0x02 ,  /*!< Enable / Disable CL91 RX */
  TEFMOD_CL91_IND_ONLY_EN_DIS = 0x04 ,  /*!< Enable / Disable CL91 indication only */
  TEFMOD_CL91_COR_ONLY_EN_DIS = 0x08 ,  /*!< Enable / Disable CL91 correction only */
  TEFMOD_CL74_TX_EN_DIS      = 0x10 ,  /*!< Enable / Disable CL74 TX */
  TEFMOD_CL74_RX_EN_DIS      = 0x20 ,  /*!< Enable / Disable CL74 RX */
  TEFMOD_CL74_CL91_EN_DIS    = 0x40   /*!< Enable / Disable CL74CL91 for autoneg */
} tefmod_fec_en_parm_t;

/*! \def CNT_tefmod_fec_en_parm_t Types of enum tefmod_fec_en_parm_t */
#define CNT_tefmod_fec_en_parm_t 7

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_fec_en_parm_t [CNT_tefmod_fec_en_parm_t];
/*! \enum tefmod_tx_disable_enum_t 

For various tx disable condition use this enum.

*/

typedef enum {
  TEFMOD_TX_LANE_TRAFFIC     = 0x01 ,  /*!< Enable / Disable CL91 TX */
  TEFMOD_TX_LANE_RESET       = 0x02   /*!< Enable / Disable CL91 RX */
} tefmod_tx_disable_enum_t;

/*! \def CNT_tefmod_tx_disable_enum_t Types of enum tefmod_tx_disable_enum_t */
#define CNT_tefmod_tx_disable_enum_t 2

/*!
\brief
This array returns the string version of the enum #tefmod_port_type_t when indexed
by the enum var.

*/
extern char* e2s_tefmod_tx_disable_enum_t [CNT_tefmod_tx_disable_enum_t];
/*! \enum tefmod_os_mode_type 


The oversampling mode means that bits are sent out more than once to reduce
the effective frequency of data transfer. For example you can send 5G bits
over a 10G bit lane by sending every bit twice, or OS=2. When the OS is 3.3
it's a bit tricky, we send bits 3 or 4 times in a pattern like so
3,3,3,4,3,3,3,4,... Similarly for OS = 8, we send 8,8,8,9,8,8,8,9,...


*/

typedef enum {
  TEFMOD_PMA_OS_MODE_1       = 0   ,  /*!< Over sampling Mode 1         */
  TEFMOD_PMA_OS_MODE_2       = 1   ,  /*!< Over sampling Mode 2         */
  TEFMOD_PMA_OS_MODE_4       = 2   ,  /*!< Over sampling Mode 4         */
  TEFMOD_PMA_OS_MODE_16_25   = 8   ,  /*!< Over sampling Mode 8.25      */
  TEFMOD_PMA_OS_MODE_20_625  = 12  ,  /*!< Over sampling Mode 10        */
  TEFMOD_PMA_OS_MODE_ILLEGAL = 15    /*!< Over sampling Mode Illegal   */
} tefmod_os_mode_type;

/*! \def CNT_tefmod_os_mode_type Types of enum tefmod_os_mode_type */
#define CNT_tefmod_os_mode_type 6

/*!
\brief
This array returns the string of #tefmod_os_mode when indexed by the enum.

*/
extern char* e2s_tefmod_os_mode_type [CNT_tefmod_os_mode_type];
/*!
\brief
This array returns the over sampling value of #tefmod_os_mode when indexed by
the enum. For floating values 3.3 and 8.25 it returns 33 and 825.

*/
extern int e2n_tefmod_os_mode_type [CNT_tefmod_os_mode_type];
/*! \enum tefmod_scr_mode 

This mode indicates how many bits of a 'frame' will be scrambled. We have either
66 or 80 bits per frame (depending on 8b10b or 64b66b encoding). Normally
scrambling is

\li bypassed for baud rates under 6.25G (0).
\li we scramble all 66 bits (1)
\li all 80 bits(2)
\li only 64 of the 66b (3). i.e we don't scramble the 'sync' bits.

*/

typedef enum {
  TEFMOD_SCR_MODE_BYPASS     = 0   ,  /*!< Scrambling Mode bypassed      */
  TEFMOD_SCR_MODE_CL49       = 1   ,  /*!< Scrambling Mode 66B           */
  TEFMOD_SCR_MODE_40G_2_LANE = 2   ,  /*!< Scrambling Mode 80B           */
  TEFMOD_SCR_MODE_100G       = 3   ,  /*!< Scrambling Mode 64B           */
  TEFMOD_SCR_MODE_20G        = 4   ,  /*!< Scrambling Mode 64B           */
  TEFMOD_SCR_MODE_40G_4_LANE = 5   ,  /*!< Scrambling Mode 64B           */
  TEFMOD_SCR_MODE_ILLEGAL    = 15    /*!< SCR Mode Mode Illegal   */
} tefmod_scr_mode;

/*! \def CNT_tefmod_scr_mode Types of enum tefmod_scr_mode */
#define CNT_tefmod_scr_mode 7

/*!
\brief
This array returns the string of #tefmod_scr_mode when indexed by the enum.

*/
extern char* e2s_tefmod_scr_mode [CNT_tefmod_scr_mode];
/*!
\brief
This array returns the scrambling mode of #tefmod_scr_mode when indexed by
the enum.

*/
extern int e2n_tefmod_scr_mode [CNT_tefmod_scr_mode];
/*! \enum tefmod_encode_mode 

Serial bits are encoded when transmitted. The encoding depends on the baud rate
and the link type
\li 000 All encoding functions disabled for lane
\li 001 8b10b  (cl48 )
\li 010 8b10b  (cl48 rxaui)
\li 011 8b10b  (cl36 )
\li 100 64b66b (cl82 )
\li 101 64b66b (cl49 )
\li 110 64b66b (brcm )

*/

typedef enum {
  TEFMOD_ENCODE_MODE_NONE    = 0   ,  /*!< Encoding Mode NONE     */
  TEFMOD_ENCODE_MODE_CL49    = 1   ,  /*!< Encoding Mode CL49     */
  TEFMOD_ENCODE_MODE_CL82    = 2   ,  /*!< Encoding Mode CL82     */
  TEFMOD_ENCODE_MODE_ILLEGAL = 15    /*!< Encoding Mode Illegal  */
} tefmod_encode_mode;

/*! \def CNT_tefmod_encode_mode Types of enum tefmod_encode_mode */
#define CNT_tefmod_encode_mode 4

/*!
\brief
This array returns the string of #tefmod_encode_mode when indexed by the enum.

*/
extern char* e2s_tefmod_encode_mode [CNT_tefmod_encode_mode];
/*!
\brief
This array returns the encoding mode of #tefmod_encode_mode when indexed by
the enum.

*/
extern int e2n_tefmod_encode_mode [CNT_tefmod_encode_mode];
/*! \enum tefmod_descrambler_mode 

The descrambling must match the scrambling done at the transmitting port.

*/

typedef enum {
  TEFMOD_R_DESCR_MODE_BYPASS = 0   ,  /*!< No descrambling */
  TEFMOD_R_DESCR_MODE_CL49   = 1   ,  /*!< CL49 */
  TEFMOD_R_DESCR_MODE_CL82   = 2   ,  /*!< CL82 */
  TEFMOD_R_DESCR_MODE_ILLEGAL = 3     /*!< Illegal */
} tefmod_descrambler_mode;

/*! \def CNT_tefmod_descrambler_mode Types of enum tefmod_descrambler_mode */
#define CNT_tefmod_descrambler_mode 4

/*!
\brief
This array returns the string of #tefmod_descrambler_mode when indexed by the enum.

*/
extern char* e2s_tefmod_descrambler_mode [CNT_tefmod_descrambler_mode];
/*! \enum tefmod_dec_tl_mode 

The descrambling must match the scrambling done at the transmitting port.

*/

typedef enum {
  TEFMOD_DEC_TL_MODE_NONE    = 0   ,  /*!< decoder mode NONE */
  TEFMOD_DEC_TL_MODE_CL49    = 1   ,  /*!< decoder mode CL49 */
  TEFMOD_DEC_TL_MODE_CL82    = 2   ,  /*!< decoder mode CL82 */
  TEFMOD_DEC_TL_MODE_ILLEGAL = 7     /*!< decoder mode ILLEGAL */
} tefmod_dec_tl_mode;

/*! \def CNT_tefmod_dec_tl_mode Types of enum tefmod_dec_tl_mode */
#define CNT_tefmod_dec_tl_mode 4

/*!
\brief
This array returns the string of #tefmod_dec_tl_mode when indexed by the enum.

*/
extern char* e2s_tefmod_dec_tl_mode [CNT_tefmod_dec_tl_mode];
/*! \enum tefmod_dec_fsm_mode 

The descrambling must match the scrambling done at the transmitting port.

*/

typedef enum {
  TEFMOD_DEC_FSM_MODE_NONE   = 0   ,  /*!< decoder mode NONE */
  TEFMOD_DEC_FSM_MODE_CL49   = 1   ,  /*!< decoder mode CL49 */
  TEFMOD_DEC_FSM_MODE_CL82   = 2   ,  /*!< decoder mode CL82 */
  TEFMOD_DEC_FSM_MODE_ILLEGAL = 7     /*!< decoder mode ILLEGAL */
} tefmod_dec_fsm_mode;

/*! \def CNT_tefmod_dec_fsm_mode Types of enum tefmod_dec_fsm_mode */
#define CNT_tefmod_dec_fsm_mode 4

/*!
\brief
This array returns the string of #tefmod_dec_fsm_mode when indexed by the enum.

*/
extern char* e2s_tefmod_dec_fsm_mode [CNT_tefmod_dec_fsm_mode];
/*! \enum tefmod_deskew_mode 

Deskew Mode is a function of speed.

*/

typedef enum {
  TEFMOD_R_DESKEW_MODE_BYPASSi = 0   ,  /*!< deskew mode BYPASS */
  TEFMOD_R_DESKEW_MODE_10G   = 1   ,  /*!< deskew mode 10G */
  TEFMOD_R_DESKEW_MODE_40G_4_LANE = 2   ,  /*!< deskew mode 40G_4_LANE */
  TEFMOD_R_DESKEW_MODE_40G_2_LANE = 3   ,  /*!< deskew mode 40G_2_LANE */
  TEFMOD_R_DESKEW_MODE_100G  = 4   ,  /*!< deskew mode 100G */
  TEFMOD_R_DESKEW_MODE_CL49  = 5   ,  /*!< deskew mode CL49 */
  TEFMOD_R_DESKEW_MODE_CL91  = 6   ,  /*!< deskew mode CL91 */
  TEFMOD_R_DESKEW_MODE_ILLEGAL = 7     /*!< deskew mode ILLEGAL */
} tefmod_deskew_mode;

/*! \def CNT_tefmod_deskew_mode Types of enum tefmod_deskew_mode */
#define CNT_tefmod_deskew_mode 8

/*!
\brief
This array returns the string of #tefmod_deskew_mode when indexed by the enum.

*/
extern char* e2s_tefmod_deskew_mode [CNT_tefmod_deskew_mode];
/*! \enum tefmod_t_fifo_mode 


*/

typedef enum {
  TEFMOD_T_FIFO_MODE_NONE    = 0   ,  /*!< t_fifo mode NONE */
  TEFMOD_T_FIFO_MODE_40G     = 1   ,  /*!< t_fifo mode 40G */
  TEFMOD_T_FIFO_MODE_100G    = 2   ,  /*!< t_fifo mode 100G */
  TEFMOD_T_FIFO_MODE_20G     = 3   ,  /*!< t_fifo mode 20G */
  TEFMOD_T_FIFO_MODE_ILLEGAL = 7     /*!< t_fifo mode ILLEGAL */
} tefmod_t_fifo_mode;

/*! \def CNT_tefmod_t_fifo_mode Types of enum tefmod_t_fifo_mode */
#define CNT_tefmod_t_fifo_mode 5

/*!
\brief
This array returns the string of #tefmod_t_fifo_mode when indexed by the enum.

*/
extern char* e2s_tefmod_t_fifo_mode [CNT_tefmod_t_fifo_mode];
/*! \enum tefmod_bs_btmx_mode 


*/

typedef enum {
  TEFMOD_BS_BTMX_MODE_NONE   = 0   ,  /*!< bs_btmx mode NONE */
  TEFMOD_BS_BTMX_MODE_1to1   = 1   ,  /*!< bs_btmx mode 1to1 */
  TEFMOD_BS_BTMX_MODE_2to1   = 2   ,  /*!< bs_btmx mode 2to1 */
  TEFMOD_BS_BTMX_MODE_5to1   = 2   ,  /*!< bs_btmx mode 5to1 */
  TEFMOD_BS_BTMX_MODE_ILLEGAL = 7     /*!< bs_btmx mode ILLEGAL */
} tefmod_bs_btmx_mode;

/*! \def CNT_tefmod_bs_btmx_mode Types of enum tefmod_bs_btmx_mode */
#define CNT_tefmod_bs_btmx_mode 5

/*!
\brief
This array returns the string of #tefmod_bs_btmx_mode when indexed by the enum.

*/
extern char* e2s_tefmod_bs_btmx_mode [CNT_tefmod_bs_btmx_mode];
/*! \enum tefmod_bs_dist_mode 


*/

typedef enum {
  TEFMOD_BS_DIST_MODE_5_LANE_TDM = 0   ,  /*!< bs_dist mode 5LN_TDM */
  TEFMOD_BS_DIST_MODE_2_LANE_TDM_2_VLANE = 1   ,  /*!< bs_dist mode 2LN_TDM_2VLN */
  TEFMOD_BS_DIST_MODE_2_LANE_TDM_1_VLANE = 2   ,  /*!< bs_dist mode 2LN_TDM_1VLN */
  TEFMOD_BS_DIST_MODE_NO_TDM = 3   ,  /*!< bs_dist mode NO_TDM */
  TEFMOD_BS_DIST_MODE_ILLEGAL = 7     /*!< bs_dist mode ILLEGAL */
} tefmod_bs_dist_mode;

/*! \def CNT_tefmod_bs_dist_mode Types of enum tefmod_bs_dist_mode */
#define CNT_tefmod_bs_dist_mode 5

/*!
\brief
This array returns the string of #tefmod_bs_dist_mode when indexed by the enum.

*/
extern char* e2s_tefmod_bs_dist_mode [CNT_tefmod_bs_dist_mode];

typedef enum {
  TEFMOD_AN_PROPERTY_ENABLE_NONE = 0x00000000 ,  /*!<  */
  TEFMOD_AN_PROPERTY_ENABLE_HPAM_TO_CL73_AUTO = 0x00000001 ,  /*!<  */
  TEFMOD_AN_PROPERTY_ENABLE_CL73_BAM_TO_HPAM_AUTO = 0x00000002 ,  /*!<  */
  TEFMOD_AN_PROPERTY_ENABLE_ILLEGAL = 0x00000004   /*!<  */
} an_property_enable;

/*! \def CNT_an_property_enable Types of enum an_property_enable */
#define CNT_an_property_enable 4

#endif /* _TSCFMOD_ENUM_DEFINES_H */
