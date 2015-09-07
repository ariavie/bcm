/*
 * $Id: trunk.h,v 1.14 Broadcom SDK $
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
 * This file contains TRUNK definitions internal to the BCM library.
 */

#ifndef _BCM_INT_TRUNK_H
#define _BCM_INT_TRUNK_H

typedef struct trunk_private_s {
    int     tid;            /* trunk group ID */
    int     in_use;
    int     psc;            /* port spec criterion (aka rtag) */
    int	    mc_index_spec;  /* requested by user */
    int	    mc_index_used;  /* actually used */
    int	    mc_port_used;   /* actually used or -1 */
} trunk_private_t;

#define TRUNK_CTRL_ADDR         soc_reg_addr(unit, TRUNK_TRUNK_CTRLr, 0, 0)
#define TRUNK_GROUP_ADDR(grp)   soc_reg_addr(unit, TRUNK_TRNK_GROUPr, 0, grp)

#define _BCM_TRUNK_PSC_VALID_VAL  0xf
#define _BCM_TRUNK_PSC_ORABLE_VALID_VAL  \
    (BCM_TRUNK_PSC_IPMACSA | BCM_TRUNK_PSC_IPMACDA | \
     BCM_TRUNK_PSC_IPSA | BCM_TRUNK_PSC_IPDA | \
     BCM_TRUNK_PSC_MACSA | BCM_TRUNK_PSC_MACDA | \
     BCM_TRUNK_PSC_VID)

void bcm5324_trunk_patch_linkscan(int unit, soc_port_t port, bcm_port_info_t *info);
int _bcm_robo_trunk_id_validate(int unit, bcm_trunk_t tid);
int _bcm_robo_trunk_gport_resolve
    (int unit, int member_count, bcm_trunk_member_t *member_array);

#endif  /* !_BCM_INT_TRUNK_H */
