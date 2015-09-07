/*
 * $Id: port.h 1.12 Broadcom SDK $
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
 * This file contains port module definitions internal to
 * the BCM library.
 */
#ifndef _BCM_INT_TK371X_PORT_H
#define _BCM_INT_TK371X_PORT_H
#include <soc/ea/tk371x/CtcMiscApi.h>

#define _BCM_TK371X_PON_PORT_BASE		0
#define _BCM_TK371X_MAX_PON_PORT_NUM	1
#define _BCM_TK371X_UNI_PORT_BASE		1
#define _BCM_TK371X_MAX_UNI_PORT_NUM	2
#define _BCM_TK371X_LLID_PORT_BASE		3
#define _BCM_TK371X_MAX_LLID_PORT_NUM	8

#define TK371X_PON_PORT_VALID(port) ((port) == _BCM_TK371X_PON_PORT_BASE)
#define TK371X_UNI_PORT_VALID(port) ((port) >= _BCM_TK371X_UNI_PORT_BASE && \
								(port) < _BCM_TK371X_LLID_PORT_BASE)
#define TK371X_GE_PORT_VALID(port)  ((port) == _BCM_TK371X_UNI_PORT_BASE)
#define TK371X_FE_PORT_VALID(port)	 ((port) == (_BCM_TK371X_UNI_PORT_BASE + 1))
#define TK371X_LLID_PORT_VALID(port)	(((port) >= _BCM_TK371X_LLID_PORT_BASE) && \
								((port) < (_BCM_TK371X_LLID_PORT_BASE + _BCM_TK371X_MAX_LLID_PORT_NUM)))
#define TK371X_PORT_VALID(port)	(TK371X_PON_PORT_VALID(port) || \
									TK371X_UNI_PORT_VALID(port) || \
									TK371X_LLID_PORT_VALID(port))

typedef enum {
	bcmTk371xPortArlDisLearning = 0,
	bcmTk371xPortArlHwLearning = 1,
	bcmTk371xPortFwdModeDisable = 0,
	bcmTk371xPortFwdModeEnable = 1,
}_bcm_tk371x_port_learn_mode_t;

typedef struct _bcm_tk371x_port_laser_para_s{
	uint16 temp;
	uint16 vcc;
	uint16 tx_bias;
	uint16 tx_power;
	uint16 rx_power;
}_bcm_tk371x_port_laser_para_t;

#define PORT_PON_LASER_TX_CONFIG_TIME		0x01
#define PORT_PON_LASER_TX_CONFIG_MODE		0x02
#define PORT_PON_LASER_TX_CONFIG_TIME_MODE	0x03

typedef struct pon_larse_tx_config_s{
	uint8 flag;
	CtcExtONUTxPowerSupplyControl txpws_control;
}pon_larse_tx_config_t;

extern int _bcm_tk371x_port_detach(int unit);

#endif
