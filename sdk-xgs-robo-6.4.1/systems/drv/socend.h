/*
 * $Id: socend.h,v 1.5 Broadcom SDK $
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
 * File:	socend.h
 * Purpose:	Defines SOC private device control structure for
 *		VxWorks SENS driver.
 */
#ifndef __SOCEND_H
#define __SOCEND_H

#include <bcm/rx.h>
#include <bcm/tx.h>
#include <bcm/pkt.h>

#define	SOC_END_STRING	"Broadcom StrataSwitch (BCM56XX) SENS Driver"

/*
 * Typedef:	socend_t
 * Purpose:	Describes the control structure for 1 END device on the
 *		SOC.
 */
typedef struct socend_s {
    struct end_object	se_eo;		/* SENS: END_OBJ */
    sal_mac_addr_t	se_mac;		/* SENS: Interface Mac address */
    bcm_vlan_t		se_vlan;	/* SENS: Interface VLAN ID */
    int			se_unit;	/* SOC Unit # */
    struct socend_s	*se_next;	/* Linked list of devices off Unit */
} socend_t;

/*
 * TX and RX packet snooping/filter typedefs; see socend.c.
 */

typedef void (*socend_snoop_ip_tx_t)(int unit, void *packet, int len);
typedef bcm_rx_t (*socend_snoop_ip_rx_t)(int unit, bcm_pkt_t *pkt, int *enqueue);

/*
 * Defines: SOC_END_XXX
 * Purpose: SOC End Driver memory buffer allocation parameters. 
 */
#define	SOC_END_PK_SZ	1600		/* Packet Size */
#define	SOC_END_CLBUFS	(128)		/* Cluster Buffers */
#define	SOC_END_CLBLKS	256		/* Total Cluster Headers */
#define	SOC_END_MBLKS	64		/* Total Packet Headers */
#define	SOC_END_SPEED	10000000	/* 10 Mb/s */

#define SOCEND_COS_DEFAULT 3
#define SOCEND_COS_MAP_DEFAULT {0, 0, 0, 0, 0, 0, 3, 3}

/*
 * Defines: SOC_END_FLG_XXX
 * Purpose: Various driver mode and configuration flags.
 */
#define SOC_END_FLG_MGMT   (1 << 1)
#define SOC_END_FLG_BCAST  (1 << 2)

/* BSP support routine: return switch address */
IMPORT STATUS sysSwitchAddrGet(char *mac);

/* StrataSwitch (TM) SOC Driver API Initialization */
IMPORT int socdrv(void);

#endif /* __SOCEND_H */
