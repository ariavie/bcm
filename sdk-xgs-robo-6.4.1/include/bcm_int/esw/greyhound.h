/*
 * $Id: greyhound.h,v 1.1.8.3 Broadcom SDK $
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
 * File:        greyhound.h
 * Purpose:     Function declarations for Greyhound bcm functions
 */

#ifndef _BCM_INT_GREYHOUND_H_
#define _BCM_INT_GREYHOUND_H_
#ifdef BCM_FIELD_SUPPORT 
#include <bcm_int/esw/field.h>
#endif /* BCM_FIELD_SUPPORT */
#if defined(INCLUDE_L3)
#include <bcm/extender.h>
#include <bcm/niv.h>
#endif

#if defined(BCM_GREYHOUND_SUPPORT)
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/oam.h>
#include <bcm_int/esw/cosq.h>
#include <soc/greyhound.h>

#ifdef BCM_FIELD_SUPPORT 
extern int _bcm_field_gh_init(int unit, _field_control_t *fc);
#endif
extern int bcm_gh_port_niv_type_set(int unit, bcm_port_t port, int value);
extern int bcm_gh_port_niv_type_get(int unit, bcm_port_t port, int *value);
extern int bcm_gh_port_extender_type_set(int unit, bcm_port_t port, int value);
extern int bcm_gh_port_extender_type_get(int unit, bcm_port_t port, int *value);

#if defined(INCLUDE_L3)

/* NIV */
#define GH_L2_HASH_KEY_TYPE_VIF 5 /* Key type of L2X table for NIV */

extern int bcm_gh_niv_forward_add(int unit, bcm_niv_forward_t *iv_fwd_entry);
extern int bcm_gh_niv_forward_delete(int unit, bcm_niv_forward_t *iv_fwd_entry);
extern int bcm_gh_niv_forward_delete_all(int unit);
extern int bcm_gh_niv_forward_get(int unit, bcm_niv_forward_t *iv_fwd_entry);
extern int bcm_gh_niv_forward_traverse(int unit,
                                       bcm_niv_forward_traverse_cb cb,
                                       void *user_data);

extern int bcm_gh_niv_ethertype_set(int unit, int ethertype);
extern int bcm_gh_niv_ethertype_get(int unit, int *ethertype);

/* Port Extender */
#define GH_L2_HASH_KEY_TYPE_PE_VID 6 /* Key type of L2X table for Port Extender */

extern int bcm_gh_extender_forward_add(int unit, bcm_extender_forward_t *forward_entry);
extern int bcm_gh_extender_forward_delete(int unit, bcm_extender_forward_t *forward_entry);
extern int bcm_gh_extender_forward_delete_all(int unit);
extern int bcm_gh_extender_forward_get(int unit, bcm_extender_forward_t *forward_entry);
extern int bcm_gh_extender_forward_traverse(int unit,
                                            bcm_extender_forward_traverse_cb cb,
                                            void *user_data);

extern int bcm_gh_port_etag_pcp_de_source_set(int unit, bcm_port_t port, int value);
extern int bcm_gh_port_etag_pcp_de_source_get(int unit, bcm_port_t port, int *value);


extern int bcm_gh_etag_ethertype_set(int unit, int ethertype);
extern int bcm_gh_etag_ethertype_get(int unit, int *ethertype);
#endif /* INCLUDE_L3 */


/* WRED */
extern int
bcm_gh_cosq_discard_set(int unit, uint32 flags);

extern int
bcm_gh_cosq_discard_get(int unit, uint32 *flags);

extern int
bcm_gh_cosq_discard_port_set(int unit, bcm_port_t port,
                             bcm_cos_queue_t cosq,
                             uint32 color,
                             int drop_start,
                             int drop_slope,
                             int average_time);
extern int
bcm_gh_cosq_discard_port_get(int unit, bcm_port_t port,
                             bcm_cos_queue_t cosq,
                             uint32 color,
                             int *drop_start,
                             int *drop_slope,
                             int *average_time);

extern int
bcm_gh_cosq_gport_discard_set(int unit, bcm_gport_t port, 
                                bcm_cos_queue_t cosq,
                                bcm_cosq_gport_discard_t *discard);

extern int
bcm_gh_cosq_gport_discard_get(int unit, bcm_gport_t port, 
                                bcm_cos_queue_t cosq,
                                bcm_cosq_gport_discard_t *discard);

extern int
bcm_gh_cosq_port_pfc_op(int unit, bcm_port_t port,
                       bcm_switch_control_t sctype, _bcm_cosq_op_t op,
                       bcm_cos_queue_t cosq);
extern int
bcm_gh_cosq_port_pfc_get(int unit, bcm_port_t port,
                        bcm_switch_control_t sctype, 
                        bcm_cos_queue_t *cosq);
extern int 
bcm_gh_port_init(int unit);
#endif  /* BCM_GREYHOUND_SUPPORT */
#endif  /* !_BCM_INT_GREYHOUND_H_ */
