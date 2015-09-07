/*
/* $Copyright: Copyright 2012 Broadcom Corporation.
/* This program is the proprietary software of Broadcom Corporation
/* and/or its licensors, and may only be used, duplicated, modified
/* or distributed pursuant to the terms and conditions of a separate,
/* written license agreement executed between you and Broadcom
/* (an "Authorized License").  Except as set forth in an Authorized
/* License, Broadcom grants no license (express or implied), right
/* to use, or waiver of any kind with respect to the Software, and
/* Broadcom expressly reserves all rights in and to the Software
/* and all intellectual property rights therein.  IF YOU HAVE
/* NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
/* IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
/* ALL USE OF THE SOFTWARE.  
/*  
/* Except as expressly set forth in the Authorized License,
/*  
/* 1.     This program, including its structure, sequence and organization,
/* constitutes the valuable trade secrets of Broadcom, and you shall use
/* all reasonable efforts to protect the confidentiality thereof,
/* and to use this information only in connection with your use of
/* Broadcom integrated circuit products.
/*  
/* 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
/* PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
/* REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
/* OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
/* DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
/* NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
/* ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
/* CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
/* OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
/* 
/* 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
/* BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
/* INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
/* ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
/* TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
/* POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
/* THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
/* WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
/* ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
$Id: b3a1acb3da6469234381acf382b87c2bc6a42f65 $
*/
,
                         p /*port*/,
                         BCM_PORT_PHY_CONTROL_CL72,
                         en /*0 -> disable, 1 -> enable */);
\endcode

\subsubsection fec-cntl FEC control (CL74)

The platform is assumed to know the speeds for which CL74 should be enabled.
Since enabling CL74 for non-CL74 speeds can have unpredictable results, hardware
has additional controls to simply ignore CL74 controls for non-CL74 speeds.
As an example, for SDK, CL74 can be controlled dynamically by BCM APIs.

\code
bcm_port_phy_control_set(0/*unit*/,
                         p/*port*/,
                         BCM_PORT_PHY_CONTROL_FORWARD_ERROR_CORRECTION,
                         en /*0 -> disable, 1 -> enable */);
\endcode

\subsection anccl72774 Auto-Negotiation Mode Control of CL72 and CL74

CL37 auto negotiation cannot use CL72 or CL74. They are only advertisable in
CL73 and CL37-BAM (proprietary) auto negotiation. In auto negotiation, the need to enable
these features are only known after auto negotiationotiation and speed resolution. Hence
these controls are handled by hardware/ucode rather than TMod.

\subsubsection anlnktrn Link Training (CL72)

Link training is enabled by default for CL73. It is not dynamically controlled.
It is defined as a static 'config' property, similar to the port auto
negotiation property.

\code
/* following two configs disable CL37 and enable CL73.  */
phy_an_c73_xe1=1 /* CL73 an port */
phy_an_c37_xe1=0 /* Not a CL37 an port */
/* following config enables CL72 */
phy_an_c72_xe=1
/* following config disables CL74 */
phy_an_c72_xe=0
\endcode

\subsubsection anfecctl FEC Control (CL74)
If the speed supports FEC, than FEC can be advertised. If both partners advertse
FEC, the link is established with FEC enabled. If one of them does <b>not</b>
advertise FEC, the link will <b>still</b> come up but with FEC disabled. So FEC
is controlled by simply not advertising FEC.

\code
/* following two configs disable CL37 and enable CL73.  */
phy_an_c73_xe1=1 /* CL73 an port */
phy_an_c37_xe1=0 /* Not a CL37 an port */
/* following config enables FEC */
phy_an_fec_xe=1
/* following config disables FEC */
phy_an_fec_xe=0
\endcode

\section pcs-func-ovr Overriding native with Customer PCS Functions.

This capability is no longer supported. Please contact the TEMod team if you
need this. (ravick@broadcom.com)

\section eyescan EyeScan

Eyescan utility measures link robustness. The SERDES slicer can be perturbed
horizontally and vertically and subsequently measure the BER at different
perturbations. Traffic is sent (mosly PRBS, using an internal generator) and the
error rates are recorded for a suite of perturbations and practical BER and
margins can be extrapolated using linear fit in the Q-function domain. Keep in
mind the eyescan  modes are intrusive. It cannot be done on a live link. For
more details of eyescan utility please see the Eagle Programmers Guide.

The slicer perturbation sweep can be both horizontal and vertical. The smallest
variation of the horizontal and vertical (there are 64 in each 'direction) is
called the 'step'. The perturbations are done across a range, defined by the
minimum and maximum values of horizontal and vertical perturbations.

When sweeps in both directions are performed, the eyescan is termed 2D (two
dimensional). In many cases the horizontal is fixed and only a vertical is
performed. This is termed 1D (one dimensional).

The general high level sequence to execute Eyescan is

- Bring links to required rates.
- Enable traffic (PRBS). This will also bring down the link.
- Run Eyescan code <-- there are different types as explained next.
- Stop PRBS and switch back links to mission mode (original configuration)

TEMod, built on top of eagle PMD software infrastructure provides two types of
eyescan information.
- The HighBER, or fast eyescan
- The LowBER, or custom eyescan
The next subsections will explain both methods in more detail.

\subsection highber Fast Eyescan
  This method is controlled by the ucode.  All ranges, sample times and steps
are fixed. The ucode generates the 2D info and stores it in ucode RAM and is
extracted by software and provided as a 2D integer array to customer. It can
also be printed on the screen as ASCII graphics. The ucode can be instructed to
generate eyes on one or more lanes and the data acquisition is parallel.

\code
The commands are

* Init all  OR rc. whatever method to bring the ports up.
* ps
* Linkscan off.
* Phy diag xe prbs set p=5
* Phy diag xe prbs get
* Phy diag xe prbs get
* Phy diag xe eyescan
* Phy diag pbm prbs clear

\endcode

\subsection lowber Custom Eyescan
This method allows flexibility to select the steps, range, sample time etc.
It is generally slower but potentially more accurate and controllable. The ucode
is setup per user's needs and it records the errors which software will extract.
into a 2D integer array.  It can be printed on the screen as well

\code
The commands are

1) Init all  OR rc. whatever method to bring the ports up.
2) ps
3) Linkscan off.
4) Phy diag xe prbs set p=5
5) Phy diag xe prbs get
6) Phy diag xe prbs get
7) Phy diag xe eyescan  type=c
8) Phy diag pbm prbs clear
\endcode

\subsection eyes_cntl Eyescan controls

At the User Interface (using SDK as an example), these parameters control the
generation of the eye.
- The type of Eyescan
 -# type=f  request fast eyescan (this is default)
 -# type=c  request custom eyescan
- flag
  -# '1' implies 1D
  -# if not provided, implies 2D
  -# no other values are valid
- The limits of the sweep (defines the range)
  -# veye_m
  -# veye_x
  -# heye_l (specify this alone if requesting 1D)
  -# heye_r
- the resolution (i.e. steps)
  -# sample_resolution (fixed for both Vertical and horizontal simultaneously)
  -# sample_resolution_v (to provide seperate vertical step)
- counter
  -# defunct.
- sample_time, time to run traffic to collect BER specified in ms

\section trg_phy_cfg_dis Triage Phy Configuration display 

TEMod supports category based diagnostics printouts in the diagnostics prompt.
Currently the following categories exist. 

\li TOPOLOGY: loopbacks/swaps/polarity/port modes
\li LINK STATE: sigdet/pll/pmd_lock/pcs_block/pcs_linkup
\li AUTONEG: type, abilities, resolved speed, link
\li SPEED:  speed set, hard table overrides, soft tables
\li TFC: prbs/pkt_gen/prtp/traffic
\li AN TIMERS: Various AN timers.

With the command below, we can get a report of the link state
\code
BCM.0> phy diag xe0 pcs link

+------------------------------------------------------------------------------+
| TRG ADR : 000165 LANE: 01    LINK STATE                   |   LH    |   LL   |
+-----------------------+-------------------+---------------+---------+--------+
| PMD PLL LOCK   : Y    | PCS SYNC   : Y    | PCS SYNC STAT : 0000    : 0000   |
| PMD SIG DETECT : NNYY | PCS LINK   : Y    | PCS LINK STAT : 0000    : 0000   |
| PMD LOCKED     : NNYY | PCS HI BER : 0000 | PCS HIGH BER  : 0000    : 0000   |
| PMD LATCH HI   : 0000 | PCS DESKEW : 0000 | PCS DESKEW    : 0000    : 0000   |
| PMD LATCH LO   : 0000 | PCS AMLOCK : 0000 | PCS AM LOCK   : 0000    : 0000   |
| RXLOCK LATCH HI: 0000 |                   |                                  |
| RXLOCK LATCH LO: 0000 |                   |                                  |
+-----------------------+-------------------+----------------------------------+
\endcode

With the command below, we can get a report of various speed parameters

\code
BCM.0> phy diag xe0 pcs speed
+------------------------------------------------------------------------------+
| TRG ADR : 000165 LANE: 01    SPEED                                           |
+---------------------+-----------------------------+-----------+--------------+
| ST [0]              | SPD_ILLEGAL                 | NUM LANES : 0            |
| ST [1]              | SPD_ILLEGAL                 | NUM LANES : 0            |
| ST [2]              | SPD_ILLEGAL                 | NUM LANES : 0            |
| ST [3]              | SPD_ILLEGAL                 | NUM LANES : 0            |
+---------------------+-----------------------------+-----------+--------------+
|                          OEN SET OVR VALUE                                   |
+----------------+----------------+--------------------------------------------+
| NUM LANES: 0x0 | FEC ENA  : 0x0 | PMA_OS_MODE_1                              |
| 64B66DECR: 0x0 | CHKEND   : 0x1 | SCR_MODE_BYPASS                            |
| FECENABL : 0x0 | REORDER  : 0x0 | ENCODE_MODE_NONE                           |
| CL36ENA  : 0x0 | SGMIISPD : 0x0 | R_DESCR1_MODE_BYPASS                       |
| CLKCNT0  : 0x0 | CLKCNT1  : 0x0 | DECODER_MODE_NONE                          |
| LP CNT0  : 0x0 | LP CNT1  : 0x0 | R_DESKEW_MODE_BYPASS                       |
| MACCRDGEN: 0x0 | REPLCNT  : 0x0 | DESC2_MODE_NONE                            |
| PCSCRDENA: 0x0 | CLK CNT  : 0x0 | R_DESC2_BYTE_DELETION_100M                 |
| PCSCRDGEN: 0x0 |                | BLOCKSYNC_MODE_NONE                        |
+----------------+--+-------------+------+------------------+------------------+
|        SPEED      |        STATS0      |      STATS1      |   CREDIT STATS   |
+-------------------+--------------------+------------------+------------------+
| SPD CHG VLD  0    |     OS MODE 1      | DESCR MODE BYPASS| SGMII SPD : 0000 |
| SPD CHG DONE 0    |    SCR MODE 64B    |DECODE MODE CL49  |  CLK CNT0 : 0033 |
| SPD RESOLVED 0028 |    ENC MODE CL49   |DESKEW MODE BYPASS|  CLK CNT1 : 0000 |
| #LN RESOLVED 0000 |BLKSYNC MODE CL49   |DESCR2 MODE CL49  |   LP CNT0 : 0001 |
| PLL DIV      0010 |   CL72  ENA : 01   |  BYTE  DEL NONE  |   LP CNT1 : 0000 |
| REF CLOCK    0000 | CHKEND  ENA : 00   |64b66DEC EN 0     |  MAC  CGC : 0004 |
|                   |    FEC  ENA : 00   |                  |  REP  CNT : 0000 |
|                   |REORDER  ENA : 00   |                  |PCS CRD EN : 0000 |
|                   |   CL36  ENA : 00   |                  |PCS CK CNT : 0000 |
|                   |                    |                  |CRDGEN CNT : 0000 |
+-------------------+--------------------+------------------+------------------+

\endcode

With the command below, we can get a report of various auto-negotiation parameters

\code
BCM.0> phy diag xe0 pcs aneg 
+------------------------------------------------------------------------------+
| TRG ADR : 000165 LANE: 01     AUTONEG                                        |
+-------------+-------------+------------------------------+-------------------+
| AN37: N     | AN73 : N    | AN HCD SPD : 1000M           |  AN LINK : DN     |
+-------------------+-------+------+-----------------------+-------------------+
| ANX4 CTRL: 0x0000 | ENS : 0x0000 | CL37 BAM:0x0000 BASE :0x0000              |
| ANX4 OVR0: 0x0000 | OVR1: 0x0000 | CL73 BAM:0x0000 BASE1:0x0000 BASE0:0x02A0 |
+-------------------+--------------+----+--------------------------------------+
|      CLAUSE 37    |      CLAUSE 73    |                                      |
+-------------------+-------------------+--------------------------------------+
| BAM ENA       : 0 | BAM  ENA     : 0  | NUM ADV LANES : 1                    |
| AN  ENA       : 0 | AN   ENA     : 0  | FAIL COUNT LIM: 0                    |
| SGMII ENA     : 0 | HPAM ENA     : 0  |                                      |
| BAM2SGMII ENA : 0 | BAM3HPAM ENA : 0  |                                      |
| SGMII2CL37 ENA: 0 | HPAM2CL73 ENA: 0  |                                      |
| AN RESTART    : 0 | AN RESTART   : 0  |                                      |
+-------------------+-------------------+--------------------------------------+
|            CL37 ABILITIES             |         CL73 ABILITIES               |
+---------------+-----------------------+-----------------+--------------------+
| SWRST DIS : 0 | ANRST DIS    :0       | NONCE OVR : 0   | NONCE VAL: 0       |
| PD 2 CL37 : 0 | NEXT PAGE    :0       | TX NONCE  : 0x15| BASE SEL : Rsvd    |
| HALF DUPLX: 0 | FULL DUPLEX  :0       | NEXT PAGE  : 0  | FEC      : 0       |
| PAUSE     : 0 | SGMII MASTER :0       | REMOTE FLT : 0  | PAUSE    : 0       |
| SGMII FDUP: 0 | SGMII SPD    :10Mb/s  |-----------------+--------------------|
| OVR1G ABIL: 0 | OVR1G PAGECNT:0       | 1000BASE KX : 0 | 10GBASE KX4 :0     |
| BAM CODE      : 0x0000                | 10GBASE KR  : 0 | 10GBASE KR4 :0     |
|---------------+-----------------------| 40GBASE CR4 : 0 | 100GBASE CR1:0     |
|                                       | HPAM_20GKR2 : 0 | BAM CODE    :0x0000|
|                                       | 20GBASE CR2 : 0 | 20GBASE KR2 : 0    |
+---------------------------------------+--------------------------------------+
|                               OVER1G ABILITIES                               |
+-------------------+--------------------+-----------------+-------------------+
| HG2         : 0   | FEC          : 0   | CL72         : 0|                   |
| 40GBASE X4  : 0   | 32P7GBASE X4 : 0   | 31P5GBASE X4 : 0| 25P455GBASE X4: 0 |
| 21GBASE X4  : 0   | 20GBASEX2 CX4: 0   | 20GBASE X2   : 0| 20GBASE X4    : 0 |
| 16GBASE X4  : 0   | 15P75GBASE X2: 0   | 15GBASE X4   : 0| 13GBASE X4    : 0 |
+-------------------+--------------------+-----------------+-------------------+
\endcode

With the command below, we can get a report of various Traffic sub-configurations

\code
BCM.0> phy diag xe0 pcs tfc 
+------------------------------------------------------------------------------+
| TRG ADR : 000165 LANE: 01    INTERNAL TFC                                    |
+------------------------------------------+-----------------------------------+
| Traffic_type: MAC                      |                                     |
+------------------------------------------+-----------------------------------+
\endcode

With the command below, we can get a report of various auto-negotiation timers

\code
BCM.0> phy diag xe0 pcs antimers
+------------------------------------------------------------------------------+
| TRG ADR : 000165 LANE: 01    AN TIMERS                                       |
+--------------------------------------+---------------------------------------+
| CL37 RESTART          : 0x0000029A   | CL37 ACK               : 0x0000029A   |
| CL37 ERR              : 0x00000000   | CL37 LINK BREAK        : 0x000010ED   |
| CL73 ERR              : 0x00000000   | CL73 DME LOCK          : 0x000014D4   |
| LINK_UP               : 0x0000029A   | PS SD                  : 0x00000a6a   |
| SYNC STATUS           : 0x0000029A   | PD TO CL37             : 0x00000A6A   |
| IGNORE LINK           : 0x0000029A   | SGMII                  : 0x0000006B   |
| DME PAGE MIN          : 0x0000005F   | DME PAGE MAX           : 0x00000076   |
| FAIL INHIBIT W/O CL72 : 0x000014D5   | FAIL INHIBIT WITH CL72 : 0x00008382   |
+--------------------------------------+---------------------------------------+
\endcode

With the command below, we can get a report of PCS state machines

\code
BCM.0> phy diag xe0 pcs state   
+------------------------------------------------------------------------------+
|                                 DEBUG STATE                                  |
+--------------------------------------+---------------------------------------+
| SC_DEBUG_STATE    : 0x0000ef74       |  FSM_STATUS : 0x0000EF74              |
| TLA_SEQUENCER STS : 0x00000001       |                                       |
+--------------------------------------+---------------------------------------+
\endcode

With the catchall command below, we can get a comprehensive report of all PCS.
This is ssentially all the previous commands.

\code
BCM.0> phy diag xe2 pcs
+------------------------------------------------------------------------------+
| TRG ADR : 000165 LANE: 01     TOPOLOGY                                       |
+-------------------+--------------------------+---------------+---------------+
| PCSLCL LPBK: NNNN | PCS LANE SWAP L2P : 3210 | TX POLARITY : 0 | PORT NUM : 0|
| PCSRMT LPBK: NNNN | PMD LANE ADDR IDX : 3210 |                 | SNGLMODE : 0|
| PMDDIG LPBK: NNNN | PMD TO AFE        : 3210 | RX POLARITY : 0 | PORT MODE: 0|
| PMDREM LPBK: NNNN |                          |                               |
+-------------------+--------------------------+-------------------------------+
| TRG ADR : 000165 LANE: 01    LINK STATE                   |   LH    |   LL   |
+-----------------------+-------------------+---------------+---------+--------+
| PMD PLL LOCK   : Y    | PCS SYNC   : Y    | PCS SYNC STAT : 0000    : 0000   |
| PMD SIG DETECT : NNYY | PCS LINK   : Y    | PCS LINK STAT : 0000    : 0000   |
| PMD LOCKED     : NNYY | PCS HI BER : 0000 | PCS HIGH BER  : 0000    : 0000   |
| PMD LATCH HI   : 0000 | PCS DESKEW : 0000 | PCS DESKEW    : 0000    : 0000   |
| PMD LATCH LO   : 0000 | PCS AMLOCK : 0000 | PCS AM LOCK   : 0000    : 0000   |
| RXLOCK LATCH HI: 0000 |                   |                                  |
| RXLOCK LATCH LO: 0000 |                   |                                  |
+-----------------------+-------------------+----------------------------------+
| TRG ADR : 000165 LANE: 01     AUTONEG                                        |
+-------------+-------------+------------------------------+-------------------+
| AN37: N     | AN73 : N    | AN HCD SPD : 1000M           |  AN LINK : DN     |
+-------------------+-------+------+-----------------------+-------------------+
| ANX4 CTRL: 0x0000 | ENS : 0x0000 | CL37 BAM:0x0000 BASE :0x0000              |
| ANX4 OVR0: 0x0000 | OVR1: 0x0000 | CL73 BAM:0x0000 BASE1:0x0000 BASE0:0x02A0 |
+-------------------+--------------+----+--------------------------------------+
|      CLAUSE 37    |      CLAUSE 73    |                                      |
+-------------------+-------------------+--------------------------------------+
| BAM ENA       : 0 | BAM  ENA     : 0  | NUM ADV LANES : 1                    |
| AN  ENA       : 0 | AN   ENA     : 0  | FAIL COUNT LIM: 0                    |
| SGMII ENA     : 0 | HPAM ENA     : 0  |                                      |
| BAM2SGMII ENA : 0 | BAM3HPAM ENA : 0  |                                      |
| SGMII2CL37 ENA: 0 | HPAM2CL73 ENA: 0  |                                      |
| AN RESTART    : 0 | AN RESTART   : 0  |                                      |
+-------------------+-------------------+--------------------------------------+
|            CL37 ABILITIES             |         CL73 ABILITIES               |
+---------------+-----------------------+-----------------+--------------------+
| SWRST DIS : 0 | ANRST DIS    :0       | NONCE OVR : 0   | NONCE VAL: 0       |
| PD 2 CL37 : 0 | NEXT PAGE    :0       | TX NONCE  : 0x15| BASE SEL : Rsvd    |
| HALF DUPLX: 0 | FULL DUPLEX  :0       | NEXT PAGE  : 0  | FEC      : 0       |
| PAUSE     : 0 | SGMII MASTER :0       | REMOTE FLT : 0  | PAUSE    : 0       |
| SGMII FDUP: 0 | SGMII SPD    :10Mb/s  |-----------------+--------------------|
| OVR1G ABIL: 0 | OVR1G PAGECNT:0       | 1000BASE KX : 0 | 10GBASE KX4 :0     |
| BAM CODE      : 0x0000                | 10GBASE KR  : 0 | 10GBASE KR4 :0     |
|---------------+-----------------------| 40GBASE CR4 : 0 | 100GBASE CR1:0     |
|                                       | HPAM_20GKR2 : 0 | BAM CODE    :0x0000|
|                                       | 20GBASE CR2 : 0 | 20GBASE KR2 : 0    |
+---------------------------------------+--------------------------------------+
|                               OVER1G ABILITIES                               |
+-------------------+--------------------+-----------------+-------------------+
| HG2         : 0   | FEC          : 0   | CL72         : 0|                   |
| 40GBASE X4  : 0   | 32P7GBASE X4 : 0   | 31P5GBASE X4 : 0| 25P455GBASE X4: 0 |
| 21GBASE X4  : 0   | 20GBASEX2 CX4: 0   | 20GBASE X2   : 0| 20GBASE X4    : 0 |
| 16GBASE X4  : 0   | 15P75GBASE X2: 0   | 15GBASE X4   : 0| 13GBASE X4    : 0 |
+-------------------+--------------------+-----------------+-------------------+
| TRG ADR : 000165 LANE: 01    SPEED                                           |
+---------------------+-----------------------------+-----------+--------------+
| ST [0]              | SPD_ILLEGAL                 | NUM LANES : 0            |
| ST [1]              | SPD_ILLEGAL                 | NUM LANES : 0            |
| ST [2]              | SPD_ILLEGAL                 | NUM LANES : 0            |
| ST [3]              | SPD_ILLEGAL                 | NUM LANES : 0            |
+---------------------+-----------------------------+-----------+--------------+
|                          OEN SET OVR VALUE                                   |
+----------------+----------------+--------------------------------------------+
| NUM LANES: 0x0 | FEC ENA  : 0x0 | PMA_OS_MODE_1                              |
| 64B66DECR: 0x0 | CHKEND   : 0x1 | SCR_MODE_BYPASS                            |
| FECENABL : 0x0 | REORDER  : 0x0 | ENCODE_MODE_NONE                           |
| CL36ENA  : 0x0 | SGMIISPD : 0x0 | R_DESCR1_MODE_BYPASS                       |
| CLKCNT0  : 0x0 | CLKCNT1  : 0x0 | DECODER_MODE_NONE                          |
| LP CNT0  : 0x0 | LP CNT1  : 0x0 | R_DESKEW_MODE_BYPASS                       |
| MACCRDGEN: 0x0 | REPLCNT  : 0x0 | DESC2_MODE_NONE                            |
| PCSCRDENA: 0x0 | CLK CNT  : 0x0 | R_DESC2_BYTE_DELETION_100M                 |
| PCSCRDGEN: 0x0 |                | BLOCKSYNC_MODE_NONE                        |
+----------------+--+-------------+------+------------------+------------------+
|        SPEED      |        STATS0      |      STATS1      |   CREDIT STATS   |
+-------------------+--------------------+------------------+------------------+
| SPD CHG VLD  0    |     OS MODE 1      | DESCR MODE BYPASS| SGMII SPD : 0000 |
| SPD CHG DONE 0    |    SCR MODE 64B    |DECODE MODE CL49  |  CLK CNT0 : 0033 |
| SPD RESOLVED 0028 |    ENC MODE CL49   |DESKEW MODE BYPASS|  CLK CNT1 : 0000 |
| #LN RESOLVED 0000 |BLKSYNC MODE CL49   |DESCR2 MODE CL49  |   LP CNT0 : 0001 |
| PLL DIV      0010 |   CL72  ENA : 01   |  BYTE  DEL NONE  |   LP CNT1 : 0000 |
| REF CLOCK    0000 | CHKEND  ENA : 00   |64b66DEC EN 0     |  MAC  CGC : 0004 |
|                   |    FEC  ENA : 00   |                  |  REP  CNT : 0000 |
|                   |REORDER  ENA : 00   |                  |PCS CRD EN : 0000 |
|                   |   CL36  ENA : 00   |                  |PCS CK CNT : 0000 |
|                   |                    |                  |CRDGEN CNT : 0000 |
+-------------------+--------------------+------------------+------------------+

\endcode

The PMD data dump has similarly controls. The DSC states are shown with the
command below.

\code
BCM.0> phy diag xe0 dsc

***********************************
**** SERDES CORE DISPLAY STATE ****
***********************************

Average Die TMON_reg13bit = 5999
Temperature Force Val     = 255
Temperature Index         = 10  [40C to 48C]
Core Event Log Level      = 1

Core DP Reset State       = 0

Common Ucode Version       = 0xe10e
Common Ucode Minor Version = 0x0
AFE Hardware Version       = 0x0

LN (CDRxN  ,UC_CFG) SD LCK RXPPM CLK90 CLKP1 PF(M,L) VGA DCO P1mV M1mV
DFE(1,2,3,4,5,dcd1,dcd2)   SLICER(ze,zo,pe,po,me,mo) TXPPM TXEQ(n1,m,p1,p2)
EYE(L,R,U,D) LINK_TIME
0 (OSx8.25,0x40)   1   1    0   42    21   7, 0    45   0    0   0   0,  0,  0,
0,  0,  0,  0 -54,-54,-54,-38,-14,-54      0    12,102, 0, 0    0, 0, 0, 0
3.6
\endcode

The command below is yet to be implemented.

\code
BCM.0> phy diag xe0 dsc ber
\endcode
The command below shows a variety of PMD core AND lane configurations.
\code
BCM.0> phy diag xe0 dsc config

***********************************
**** SERDES CORE CONFIGURATION ****
***********************************

uC Config VCO Rate   = 19 (10.250GHz)
Core Config from PCS = 0

Lane Addr 0          = 0
Lane Addr 1          = 1
Lane Addr 2          = 2
Lane Addr 3          = 3
TX Lane Map 0        = 0
TX Lane Map 1        = 1
TX Lane Map 2        = 2
TX Lane Map 3        = 3

*************************************
**** SERDES LANE 0 CONFIGURATION ****
*************************************
Auto-Neg Enabled      = 0
DFE on                = 0
Brdfe_on              = 0
Media Type            = 2
Unreliable LOS        = 1
Scrambling Disable    = 0
CL72 Emulation Enable = 0
Lane Config from PCS  = 0

CL72 Training Enable  = 0
EEE Mode Enable       = 0
OSR Mode Force        = 1
OSR Mode Force Val    = 8
TX Polarity Invert    = 0
RX Polarity Invert    = 0

TXFIR Post2           = 0
TXFIR Post3           = 0
TXFIR Override Enable = 0
TXFIR Main Override   = 102
TXFIR Pre Override    = 12
TXFIR Post Override   = 0
\endcode
Get CL72 specific information with this command
\code
BCM.0> phy diag xe0 dsc cl72  

***********************************
**** SERDES CORE DISPLAY STATE ****
***********************************

Average Die TMON_reg13bit = 6025
Temperature Force Val     = 255
Temperature Index         = 10  [40C to 48C]
Core Event Log Level      = 1

Core DP Reset State       = 0

Common Ucode Version       = 0xe10e
Common Ucode Minor Version = 0x0
AFE Hardware Version       = 0x0

LN (CDRxN  ,UC_CFG) SD LCK RXPPM CLK90 CLKP1 PF(M,L) VGA DCO P1mV M1mV
DFE(1,2,3,4,5,dcd1,dcd2)   SLICER(ze,zo,pe,po,me,mo) TXPPM TXEQ(n1,m,p1,p2)
EYE(L,R,U,D) LINK_TIME
0 (OSx8.25,0x40)   1   1    0   42    21   7, 0    45   0    0   0   0,  0,
0,  0,  0,  0,  0 -54,-54,-54,-38,-14,-54      0    12,102, 0, 0    0, 0, 0, 0
3.6
\endcode
Get DSC specific information with this command
\code
BCM.0> phy diag xe0 dsc debug

************************************
**** SERDES LANE 0 DEBUG STATUS ****
************************************

Restart Count       = 1
Reset Count         = 1
PMD Lock Count      = 2

Disable Startup PF Adaptation           = 0
Disable Startup DC Adaptation           = 0
Disable Startup Slicer Offset Tuning    = 0
Disable Startup Clk90 offset Adaptation = 0
Disable Startup P1 level Tuning         = 0
Disable Startup Eye Adaptaion           = 0
Disable Startup All Adaptaion           = 0

Disable Startup DFE Tap1 Adaptation = 0
Disable Startup DFE Tap2 Adaptation = 0
Disable Startup DFE Tap3 Adaptation = 0
Disable Startup DFE Tap4 Adaptation = 0
Disable Startup DFE Tap5 Adaptation = 0
Disable Startup DFE Tap1 DCD        = 0
Disable Startup DFE Tap2 DCD        = 0

Disable Steady State PF Adaptation           = 0
Disable Steady State DC Adaptation           = 0
Disable Steady State Slicer Offset Tuning    = 0
Disable Steady State Clk90 offset Adaptation = 0
Disable Steady State P1 level Tuning         = 0
Disable Steady State Eye Adaptaion           = 0
Disable Steady State All Adaptaion           = 0

Disable Steady State DFE Tap1 Adaptation = 0
Disable Steady State DFE Tap2 Adaptation = 0
Disable Steady State DFE Tap3 Adaptation = 0
Disable Steady State DFE Tap4 Adaptation = 0
Disable Steady State DFE Tap5 Adaptation = 0
Disable Steady State DFE Tap1 DCD        = 0
Disable Steady State DFE Tap2 DCD        = 0

Retune after Reset    = 1
Clk90 offset Adjust   = 135
Clk90 offset Override = 0
Lane Event Log Level  = 2
\endcode

\section TSCE12-intro TSCE12

The TSCE12 is uses 3 TSCE cores to provide 12 physical 10G lanes. It can also be
configured as three, independent TSCs, each of which support 4 lanes. In the
first case it interfaces on the system side with a CMAC and in the second case
it interfaces with three, separate, XLMACs.

When in three-core mode, the operations of the core are no different than the
TSCE. So this section discusses the case when the three cores operating with a
CMAC. Another block of logic also provides for 100G or 120G MLD.

\subsection 100gcfgconst_tsc12 100G configuration constraints in TSCE12

To support 100G in three core mode, we have to select 10 out of 12 lanes. Two
lanes will be unused. So we have the notations of 4-4-2, 3-4-3, or 2-4-4 from
TSCE12.

\li The 4-4-2 means the first and second cores deploy all 4 lanes, and the
third core deploys logic lanes 0 and 1.

\li 3-4-3 means the first core provides lanes 0, 1, and 2, the second core
provides all 4 logic lanes, and the third code provides lane 0, 1, and 2.

\li 2-4-4 means the first core provides logic lane0 and lane1, and the second
and third core provide all 4 logic lanes.

The logic lane 0 of the individual core must always be active. In other words
the two unused lanes cannot be logic lane 0 in any 100G configuration.

The logic lane order is the same for data striping. Also each core has lane swap
functions within the core that could be used to accommodate board routing lane
swap applications.
<b>NOTE:</b> In TR3 and Arad 100G HW, there is an MLD reorder register that
can achieve a restricted logical lane swap cross 3 cores. But in TD2+, the
lane swap is within a single core. 

\subsection tsc12-port-trn Configuration transitions

Broadcom PHY ports could be easily reconfigured to meet wide applications, such
as lane swap, speed change, port size change (flex port). But due to the 100G HW
design, there is some limitation worth noting.

In the 'before' column, the given configuration has a connection to 0-9 lanes
of the cable connector which provides 100G traffic. This type of connector is
not IEEE standard. In the after row, the given configuration has a connection to
1-10 lanes of the cable connector which provides 100G traffic. This type of
connector is for IXIA/IEEE testing. Note that the transition
table is written for a given board design/routing and lane swap is not required.

The following table shows the possible configuration transitions for 100G forced
speed modes for different cabling.

<table cellspacing=5>
<tr><td colspan=3><B>'Transition table' bit-mappings</B></td></tr>
<tr><td><B>Before(0-9)\\After(1-10)</B></td> <td><B>4-4-2</B></td> <td><B>3-4-3</B></td> <td><B>2-4-4</B>     </td></tr>
<tr><td>4-4-2</td> <td>Impossible</td> <td>OK</td>         <td>Impossible</td></tr>
<tr><td>4-4-2</td> <td>Impossible</td> <td>Impossible</td> <td>OK        </td></tr>
<tr><td>4-4-2</td> <td>Impossible</td> <td>Impossible</td> <td>Impossible</td></tr>
</table>

\li The impossible mark applies to the configuration transitions that incur due
to 0-9 vs 1-10 lane selection between two types of cabling.

\subsection cl37-100-an CL73 100G AN considerations

For auto negotiationotiation, we need to first identify logic lane 0 which will carry out
page exchanges and speed negotiation. Further the CL73 auto negotiationotiation could
negotiate to 100G, 40G, 10G KR, 10G-XAUI, or even 1G. For 40G, the design would
require a 4-lane XLMAC bandwidth.  Thus for the 4-4-2 configuration, only the
lane 0 of the first or second core can be used for auto negotiationotiation if 40G is a
required advertisable speed.  For the 3-4-3 configuration , only the second core
can be used. Similarly for 2-4-4 configuration, only the second and third core's
lane 0 can be used.  Proper port configuration requires to setup the correct
XLMAC out of three XLMACs for speeds less than 100G. But for 100G ports, the logic
lane 0 is not always in the first core. So some of the configuration settings
are BRCM TD2+ specific and we have to ensure the future BRCM products are
backward compatible. 

<table cellspacing=5>
<tr><td><B>Configuration</B></td> <td><B>Core for Lane</B></td></tr>
<tr><td>4-4-2</td> <td>1 or 2</td></tr>
<tr><td>3-4-3</td> <td>2     </td></tr>
<tr><td>2-4-4</td> <td>2 or 3</td></tr>
</table>

If we want to support configurations mentioned above, the driver would need the
platform to provide information (for example, in SDK we sould call them SOC
properties) 
- Lane configuration identification 4-4-2, 3-4-3, or 2-4-4.
- The core supporting  lane 0 for auto-negotiation.
For incompatible combinations of the soc properties, the platform should default
to a known working combination or handle the error appropriately.

*/
