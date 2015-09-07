/*
 * $Id: auth.h 1.17 Broadcom SDK $
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
 * This file contains Auth definitions internal to the BCM library.
 */
#ifndef _BCM_INT_AUTH_H
#define _BCM_INT_AUTH_H

#include <soc/drv.h>
#define BCM_AUTH_SEC_NONE           0x00010000
#define BCM_AUTH_SEC_STATIC_ACCEPT  0x00020000
#define BCM_AUTH_SEC_STATIC_REJECT  0x00040000
#define BCM_AUTH_SEC_SA_NUM         0x00080000
#define BCM_AUTH_SEC_SA_MATCH       0x00100000
#define BCM_AUTH_SEC_SA_MOVEMENT    0x00200000  /* The SA of a received packet 
                                                                                         matches an ARL entry with the 
                                                                                           associated port/LAG different 
                                                                                           from the corresponding ingress 
                                                                                           port/LAG.*/
#define BCM_AUTH_SEC_EXTEND_MODE    0x00400000
#define BCM_AUTH_SEC_SIMPLIFY_MODE  0x00800000

#ifdef BCM_TB_SUPPORT
#define BCM_AUTH_SEC_SA_OVERLIMIT_DROP    BCM_AUTH_SEC_SA_NUM | \
                                                                              BCM_AUTH_SEC_EXTEND_MODE
#define BCM_AUTH_SEC_SA_OVERLIMIT_CPUCOPY    BCM_AUTH_SEC_SA_NUM | \
                                                                                    BCM_AUTH_SEC_SIMPLIFY_MODE
#define BCM_AUTH_SEC_SA_UNKNOWN_DROP    BCM_AUTH_SEC_SA_MATCH | \
                                                                             BCM_AUTH_SEC_EXTEND_MODE
#define BCM_AUTH_SEC_SA_UNKNOWN_CPUCOPY    BCM_AUTH_SEC_SA_MATCH | \
                                                                                   BCM_AUTH_SEC_SIMPLIFY_MODE
#define BCM_AUTH_SEC_SA_MOVEMENT_DROP    BCM_AUTH_SEC_SA_MOVEMENT | \
                                                                              BCM_AUTH_SEC_EXTEND_MODE
#define BCM_AUTH_SEC_SA_MOVEMENT_CPUCOPY    BCM_AUTH_SEC_SA_MOVEMENT | \
                                                                                    BCM_AUTH_SEC_SIMPLIFY_MODE
#endif /* BCM_TB_SUPPORT */

/* 
  * To indicate EAP PDU CPUCopy is disable (drop EAP packet) 
  * 0 : Drop EAP packet.
  * 1 : EAP PDU CPUCopy is enabled.
  */
#define BCM_AUTH_SEC_RX_EAP_DROP  0x01000000

#define STATIC_MAC_WRITE 0  /* For Static Mac Security Write operateion */
#define STATIC_MAC_READ  1  /* For Static Mac Security Read operateion */

#define MAX_SEC_MAC	16 /* Sec Macs per port */


extern int bcm_robo_auth_sec_mac_add(int unit, 
                bcm_port_t port, bcm_mac_t mac);
extern int bcm_robo_auth_sec_mac_delete(int unit, 
                bcm_port_t port, bcm_mac_t mac);
extern int bcm_robo_auth_sec_mac_get(int unit, 
                bcm_port_t port,bcm_mac_t *mac, int *num);
extern int bcm_robo_auth_sec_mode_set(int unit, 
                bcm_port_t port, int mode,int mac_num);
extern int bcm_robo_auth_sec_mode_get(int unit, 
                bcm_port_t port, int *mode, int *mac_num);
extern int 
_bcm_robo_auth_sec_mode_set(int unit, bcm_port_t port, int mode,
                 int mac_num);


#endif
