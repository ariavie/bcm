/*
 * $Id: mcast.h,v 1.3 Broadcom SDK $
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
 * File:        mcast.h
 * Purpose:     
 */
#ifndef   _SOC_ROBO_MCAST_H_
#define   _SOC_ROBO_MCAST_H_

#include <soc/types.h>

typedef struct {
    int		l2mc_size;
    int	*l2mc_used;
} _soc_robo_mcast_t;

/* Soc layer device-independent L2 multicast address. */
typedef struct soc_mcast_addr_s {
    uint8 mac[6];                      /* 802.3 MAC address. */
    vlan_id_t vid;                     /* VLAN identifier. */
    soc_cos_t cos_dst;                  /* COS based on destination address. */
    soc_pbmp_t pbmp;                    /* Port bitmap. */
    soc_pbmp_t ubmp;                    /* Untagged port bitmap. */
    uint32 l2mc_index;                  /* L2MC index. */
    uint32 flags;                       /* See BCM_MCAST_XXX flag definitions. */
    int distribution_class;             /* Fabric Distribution Class. */
} soc_mcast_addr_t;

extern _soc_robo_mcast_t    robo_l2mc_info[SOC_MAX_NUM_DEVICES];

#define L2MC_INFO(unit)		(&robo_l2mc_info[unit])
#define	L2MC_SIZE(unit)		L2MC_INFO(unit)->l2mc_size
#define	L2MC_USED(unit)		L2MC_INFO(unit)->l2mc_used
#define L2MC_USED_SET(unit, n)	L2MC_USED(unit)[n] += 1
#define L2MC_USED_CLR(unit, n)	L2MC_USED(unit)[n] -= 1
#define L2MC_USED_ISSET(unit, n) (L2MC_USED(unit)[n] > 0)

#define L2MC_INIT(unit) \
	if (L2MC_USED(unit) == NULL) { return BCM_E_INIT; }
#define L2MC_ID(unit, id) \
	if (id < 0 || id >= L2MC_SIZE(unit)) { return BCM_E_PARAM; }


#endif	/* !_SOC_ROBO_MCAST_H_ */

