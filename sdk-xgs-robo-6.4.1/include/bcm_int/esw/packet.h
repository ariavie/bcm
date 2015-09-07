/*
 * $Id: packet.h,v 1.3 Broadcom SDK $
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
 * File:        packet.h
 */

#ifndef   _BCM_INT_PACKET_H_
#define   _BCM_INT_PACKET_H_

#include <sal/types.h>
#include <sal/core/sync.h>

#include <soc/dma.h>

typedef struct _bcm_pkt_dv_s {
    uint32 flags;
#define BCM_PKT_DV_ADD_STK_TAG    (1 << 0)
#define BCM_PKT_DV_ADD_VLAN_TAG   (1 << 1)
#define BCM_PKT_DV_DEL_VLAN_TAG   (1 << 2)
#define BCM_PKT_DV_ADD_HG_HDR     (1 << 3)

    /* We may be in a stacked system, but not add a stack tag */
#define BCM_PKT_DV_STACKED_SYSTEM (1 << 4)

    /* These are from the user */
    int      pkt_bytes;    /* Bytes in packet from user */
    int      payload_len;  /* Base payload length */
    int      payload_off;  /* Offset into packet where payload lies */
    int      l2_hdr_off;   /* Offset of L2 header in packet */
    int      pad_size;     /* Num bytes to pad if needed */
    uint16   vlan_tag;     /* VLAN tag_control (PRI + CFI + VLAN ID) */

    /* Bitmaps needed gotten from port encapsulation mode */
    pbmp_t   ether_pbm;    /* Port bitmap for nominal ports */
    pbmp_t   higig_pbm;    /* HiGig ports derived from pbmp */
    pbmp_t   allyr_pbm;    /* BCM 5632 ports */

    /* Current bit maps to be used for descriptor adds */
    pbmp_t   cur_pbm;
    pbmp_t   cur_upbm;
    pbmp_t   cur_l3pbm;

    /* Used to describe the packet and dv */
    int      ndv;          /* Number of DV pieces needed */
    dv_t     *dv;          /* Pointer to DV chain */
    uint32   dcb_flags;    /* DCB control word */
} _bcm_pkt_dv_t;


#endif	/* !_BCM_INT_PACKET_H_ */
