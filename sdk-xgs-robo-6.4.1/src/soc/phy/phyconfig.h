/*
 * $Id: phyconfig.h,v 1.1 Broadcom SDK $
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
 * File:        phyconfig.h
 *
 * Purpose:     Set default configuration of the PHYs.
 */

#ifndef _PHY_CONFIG_H
#define _PHY_CONFIG_H

/************************/
/* Shared standard PHYs */
/************************/
#if defined(INCLUDE_SERDES)
#endif /* INCLUDE_SERDES */

#if defined(INCLUDE_PHY_XGXS)
#endif /* INCLUDE_PHY_XGXS */

#if defined(INCLUDE_PHY_FEGE)
#endif /* INCLUDE_PHY_FEGE */

#if defined(INCLUDE_PHY_XEHG) 
#endif /* INCLUDE_PHY_XEHG */


/*******************/
/* Internal SerDes */
/*******************/
#if defined(INCLUDE_PHY_5690)
#endif /* INCLUDE_PHY_5690 */

#if defined(INCLUDE_SERDES_ASSUMED)
#endif /* INCLUDE_SERDES_ASSUMED */

#if defined(INCLUDE_PHY_56XXX)
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_100FX)
#endif /* INCLUDE_SERDES_100FX */

#if defined(INCLUDE_SERDES_65LP)
#endif /* INCLUDE_SERDES_65LP */

#if defined(INCLUDE_SERDES_COMBO)
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_SERDES_COMBO65)
#endif /* INCLUDE_SERDES_COMBO65 */

#if defined(INCLUDE_SERDES_ROBO)
#endif /* INCLUDE_SERDES_ROBO */


/*********************/
/* Internal 10G PHYs */
/*********************/
#if defined(INCLUDE_PHY_XGXS1)
/* Workaround for Pass 0 Silicon */
#undef XGXS1_QUICK_LINK_FLAPPING_FIX
#endif /* INCLUDE_PHY_XGXS1 */

#if defined(INCLUDE_PHY_XGXS5)
#endif /* INCLUDE_PHY_XGXS5 */

#if defined(INCLUDE_PHY_XGXS6)
#endif /* INCLUDE_PHY_XGXS6 */

#if defined(INCLUDE_PHY_HL65)
#endif /* INCLUDE_PHY_HL65 */

/***********/
/* FE PHYs */
/***********/
#if defined(INCLUDE_PHY_522X)
#endif /* INCLUDE_PHY_522X */

/***********/
/* GE PHYs */
/***********/
#if defined(INCLUDE_PHY_5421S)
#endif /* INCLUDE_PHY_5421S */

#if defined(INCLUDE_PHY_54XX)
#endif /* INCLUDE_PHY_54XX */

#if defined(INCLUDE_PHY_5464_ESW)
#endif /* INCLUDE_PHY_5464_ESW */

#if defined(INCLUDE_PHY_5464_ROBO)
#endif /* INCLUDE_PHY_5464_ROBO */

#if defined(INCLUDE_PHY_5482)
#endif /* INCLUDE_PHY_5482 */


/*************/
/* 10GE PHYs */
/*************/
#if defined(INCLUDE_PHY_8703)
#endif /* INCLUDE_PHY_8703 */

#if defined(INCLUDE_PHY_8705)
#endif /* INCLUDE_PHY_8705 */

#if defined(INCLUDE_PHY_8706)
#endif /* INCLUDE_PHY_8706 */

#endif /* _PHY_CONFIG_H */
