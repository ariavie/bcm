/*
 * $Id: 66a6d902d600b6d6d3cf0403611aeff3d42e621c $
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

#ifndef _SHR_PORTMODE_H
#define _SHR_PORTMODE_H

typedef unsigned int _shr_port_mode_t;

/*
 * Defines:
 *      _SHR_PM_*
 * Purpose:
 *      Defines for port modes.
 */

#define _SHR_PM_10MB_HD         (1 << 0)
#define _SHR_PM_10MB_FD         (1 << 1)
#define _SHR_PM_100MB_HD        (1 << 2)
#define _SHR_PM_100MB_FD        (1 << 3)
#define _SHR_PM_1000MB_HD       (1 << 4)
#define _SHR_PM_1000MB_FD       (1 << 5)
#define _SHR_PM_2500MB_HD       (1 << 6)
#define _SHR_PM_2500MB_FD       (1 << 7)
#define _SHR_PM_10GB_HD         (1 << 8)
#define _SHR_PM_10GB_FD         (1 << 9)
#define _SHR_PM_PAUSE_TX        (1 << 10)       /* TX pause capable */
#define _SHR_PM_PAUSE_RX        (1 << 11)       /* RX pause capable */
#define _SHR_PM_PAUSE_ASYMM     (1 << 12)       /* Asymm pause capable (R/O) */
#define _SHR_PM_TBI             (1 << 13)       /* TBI mode supported */
#define _SHR_PM_MII             (1 << 14)       /* MII mode supported */
#define _SHR_PM_GMII            (1 << 15)       /* GMII mode supported */
#define _SHR_PM_SGMII           (1 << 16)       /* SGMII mode supported */
#define _SHR_PM_XGMII           (1 << 17)       /* XGMII mode supported */
#define _SHR_PM_LB_MAC          (1 << 18)       /* MAC loopback supported */
#define _SHR_PM_LB_NONE         (1 << 19)       /* Useful for automated test */
#define _SHR_PM_LB_PHY          (1 << 20)       /* PHY loopback supported */
#define _SHR_PM_AN              (1 << 21)       /* Auto-negotiation */
#define _SHR_PM_3000MB_HD       (1 << 29)
#define _SHR_PM_3000MB_FD       (1 << 30)
#define _SHR_PM_12GB_HD         (1 << 22)
#define _SHR_PM_12GB_FD         (1 << 23)
#define _SHR_PM_13GB_HD         (1 << 24)
#define _SHR_PM_13GB_FD         (1 << 25) 
#define _SHR_PM_16GB_HD         (1 << 26)
#define _SHR_PM_16GB_FD         (1 << 27)
#define _SHR_PM_COMBO           (1 << 31)      /* COMBO ports support both
                                                * copper and fiber interfaces */

#define _SHR_PM_PAUSE           (_SHR_PM_PAUSE_TX  | _SHR_PM_PAUSE_RX)
#define _SHR_PM_10MB            (_SHR_PM_10MB_HD   | _SHR_PM_10MB_FD)
#define _SHR_PM_100MB           (_SHR_PM_100MB_HD  | _SHR_PM_100MB_FD)
#define _SHR_PM_1000MB          (_SHR_PM_1000MB_HD | _SHR_PM_1000MB_FD)
#define _SHR_PM_2500MB          (_SHR_PM_2500MB_HD | _SHR_PM_2500MB_FD)
#define _SHR_PM_3000MB          (_SHR_PM_3000MB_HD | _SHR_PM_3000MB_FD)
#define _SHR_PM_10GB            (_SHR_PM_10GB_HD   | _SHR_PM_10GB_FD)
#define _SHR_PM_12GB            (_SHR_PM_12GB_HD   | _SHR_PM_12GB_FD)
#define _SHR_PM_13GB            (_SHR_PM_13GB_HD   | _SHR_PM_13GB_FD)
#define _SHR_PM_16GB            (_SHR_PM_16GB_HD   | _SHR_PM_16GB_FD)

#define _SHR_PM_SPEED_ALL       (_SHR_PM_16GB |         \
                                 _SHR_PM_13GB |         \
                                 _SHR_PM_12GB |         \
                                 _SHR_PM_10GB |         \
                                 _SHR_PM_3000MB |       \
                                 _SHR_PM_2500MB |       \
                                 _SHR_PM_1000MB |       \
                                 _SHR_PM_100MB |        \
                                 _SHR_PM_10MB)

#define _SHR_PM_SPEED_MAX(m)    (((m) & _SHR_PM_16GB)   ? 16000 : \
                                 ((m) & _SHR_PM_13GB)   ? 13000 : \
                                 ((m) & _SHR_PM_12GB)   ? 12000 : \
                                 ((m) & _SHR_PM_10GB)   ? 10000 : \
                                 ((m) & _SHR_PM_3000MB) ? 3000 : \
                                 ((m) & _SHR_PM_2500MB) ? 2500 : \
                                 ((m) & _SHR_PM_1000MB) ? 1000 : \
                                 ((m) & _SHR_PM_100MB)  ? 100 : \
                                 ((m) & _SHR_PM_10MB)   ? 10 : 0)

#define _SHR_PM_SPEED(s)        ((16000 == (s)) ? _SHR_PM_16GB : \
                                 (13000 == (s)) ? _SHR_PM_13GB : \
                                 (12000 == (s)) ? _SHR_PM_12GB : \
                                 (10000 == (s)) ? _SHR_PM_10GB : \
                                 (3000  == (s)) ? _SHR_PM_3000MB : \
                                 (2500  == (s)) ? _SHR_PM_2500MB : \
                                 (1000  == (s)) ? _SHR_PM_1000MB : \
                                 (100   == (s)) ? _SHR_PM_100MB : \
                                 (10    == (s)) ? _SHR_PM_10MB : 0)

#define _SHR_PM_FD              (_SHR_PM_16GB_FD |    \
                                 _SHR_PM_13GB_FD |    \
                                 _SHR_PM_12GB_FD |    \
                                 _SHR_PM_10GB_FD |    \
                                 _SHR_PM_3000MB_FD |    \
                                 _SHR_PM_2500MB_FD |    \
                                 _SHR_PM_1000MB_FD |    \
                                 _SHR_PM_100MB_FD |     \
                                 _SHR_PM_10MB_FD)

#define _SHR_PM_HD              (_SHR_PM_16GB_HD |    \
                                 _SHR_PM_13GB_HD |    \
                                 _SHR_PM_12GB_HD |    \
                                 _SHR_PM_10GB_HD |    \
                                 _SHR_PM_3000MB_HD |    \
                                 _SHR_PM_2500MB_HD |    \
                                 _SHR_PM_1000MB_HD |    \
                                 _SHR_PM_100MB_HD |     \
                                 _SHR_PM_10MB_HD)

#define _SHR_PORT_CONTROL_FABRIC_HEADER_83200                            (1 << 0)
#define _SHR_PORT_CONTROL_FABRIC_HEADER_88230                            (1 << 1)
#define _SHR_PORT_CONTROL_FABRIC_HEADER_88020_83200_88230_INTEROP        (1 << 2)
#define _SHR_PORT_CONTROL_FABRIC_HEADER_CUSTOM                           (1 << 3)
#define _SHR_PORT_CONTROL_FABRIC_HEADER_88640_TM                         (1 << 4)
#define _SHR_PORT_CONTROL_FABRIC_HEADER_88640_RAW                        (1 << 5)


#endif  /* !_SHR_PORTMODE_H */
