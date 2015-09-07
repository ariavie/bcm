/*
 * $Id: mirror.h,v 1.9 Broadcom SDK $
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
 */
#ifndef _BCM_INT_MIRROR_H
#define _BCM_INT_MIRROR_H
#include <shared/pbmp.h>
#include <shared/types.h>
#include <soc/macipadr.h>


#define _BCM_MIRROR_INGRESS_FROM_FP   0x100
/*
 * One entry for each SOC device containing mirroring 
 * information for that device.
 */
typedef struct mirror_cntl_s {
    int  mirr_enable;     /* is mirroring globally enabled ? */
    int  mirr_to_port;
    pbmp_t mirr_to_ports; 
    int  blk_none_mirr_traffic; 
    /* All traffic to MIRROR_CAPTURE_PORT will be blocked 
       except mirror traffic.
     */
    uint16  ingress_mirr_ctrl;
    /* bit[15:14] - filter mode. 
       00:Mirror all ingress frames, 
       01:Mirror all received frames with DA=IN_MIRROR_MAC
       10:Mirror all received frames with SA=IN_MIRROR_MAC
       bit13 - Ingress Divider Enable
       bit[12:0] - Ingress Mirror Port Mask 
     */
    uint16  in_mirr_div; /* bit[15:10] - Reserved, bit[9:0] - in_mirr_div */
    uint16  egress_mirr_ctrl;  
    /* bit[15:14] - filter mode.
       00: Mirror all ingress frames
       01: Mirror all received frames with DA = IN_MIRROR_MAC
       10: Mirror all received frames with SA= IN)_MIRROR_MAC
       bit13 - egress Divider Enable
       bit[12:0] - egress Mirror Port Mask 
     */
    uint16  en_mirr_div; /* bit[15:10] - Reserved, bit[9:0] - en_mirr_div */
    sal_mac_addr_t ingress_mac;
    sal_mac_addr_t egress_mac;    
} mirror_cntl_t;


extern int bcm_robo_dirty_mirror_mode_set(int unit, mirror_cntl_t *mirror_ctrl);
extern int bcm_robo_dirty_mirror_mode_get(int unit, mirror_cntl_t *mirror_ctrl);
extern int mirrEn5380;

extern int bcm_robo_mirror_deinit(int unit);
extern int _bcm_robo_mirror_fp_dest_add(int unit, int flags, int port);
extern int _bcm_robo_mirror_fp_dest_delete(int unit, int flags, int port);
extern int _bcm_robo_mirror_to_port_check(int unit, bcm_pbmp_t mport_pbmp, 
    bcm_pbmp_t igr_pbmp, bcm_pbmp_t egr_pbmp);
#endif
