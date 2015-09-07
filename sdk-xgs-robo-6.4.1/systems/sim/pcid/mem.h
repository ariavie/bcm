/* 
 * $Id: mem.h,v 1.26 Broadcom SDK $
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
 * File:        mem.h
 * Purpose:     Include file for memory access functions: ARL, L2X, L3X.
 */

#ifndef _PCID_MEM_H
#define _PCID_MEM_H

#include "pcid.h"
#include <sys/types.h>
#include <soc/mcm/memregs.h>


#ifdef	BCM_XGS_SWITCH_SUPPORT
extern int soc_internal_l2x_read(pcid_info_t *pcid_info, uint32 addr,
                                 l2x_entry_t *entry);

extern int soc_internal_l2x_write(pcid_info_t *pcid_info, uint32 addr, 
                                  l2x_entry_t *entry);
extern int soc_internal_l2x_lkup(pcid_info_t *pcid_info, 
                                 l2x_entry_t *entry_lookup, uint32 *result);
extern int soc_internal_l2x_init(pcid_info_t * pcid_info) ;
extern int soc_internal_l2x_del(pcid_info_t *pcid_info, l2x_entry_t *entry);
extern int soc_internal_l2x_ins(pcid_info_t *pcid_info, l2x_entry_t *entry);
#endif	/* BCM_XGS_SWITCH_SUPPORT */
#ifdef	BCM_FIREBOLT_SUPPORT
extern int soc_internal_l2x2_entry_ins(pcid_info_t *pcid_info, uint8 banks,
                                       l2x_entry_t *entry,
                                       uint32 *result);
extern int soc_internal_l2x2_entry_del(pcid_info_t *pcid_info, uint8 banks,
                                       l2x_entry_t *entry,
                                       uint32 *result);
extern int soc_internal_l2x2_entry_lkup(pcid_info_t * pcid_info, uint8 banks,
                                        l2x_entry_t *entry_lookup,
                                        uint32 *result);
extern int soc_internal_l3x2_read(pcid_info_t * pcid_info, soc_mem_t mem,
                                  uint32 addr, uint32 *entry);
extern int soc_internal_l3x2_write(pcid_info_t * pcid_info, soc_mem_t mem,
                                   uint32 addr, uint32 *entry);
extern int soc_internal_l3x2_entry_ins(pcid_info_t *pcid_info,
                                       uint32 inv_bank_map, void *entry,
                                       uint32 *result);
extern int soc_internal_l3x2_entry_del(pcid_info_t *pcid_info,
                                       uint32 inv_bank_map, void *entry,
                                       uint32 *result);
extern int soc_internal_l3x2_entry_lkup(pcid_info_t * pcid_info,
                                       uint32 inv_bank_map, void *entry,
                                       uint32 *result);
#endif	/* BCM_FIREBOLT_SUPPORT */
#ifdef	BCM_EASYRIDER_SUPPORT
extern int soc_internal_l2_er_entry_ins(pcid_info_t *pcid_info,
                                        uint32 *entry);
extern int soc_internal_l2_er_entry_del(pcid_info_t *pcid_info,
                                        uint32 *entry);
extern int soc_internal_l2_er_entry_lkup(pcid_info_t * pcid_info,
                                         uint32 *entry);
extern int soc_internal_l3_er_entry_ins(pcid_info_t *pcid_info,
                                        uint32 *entry, int l3v6);
extern int soc_internal_l3_er_entry_del(pcid_info_t *pcid_info,
                                        uint32 *entry, int l3v6);
extern int soc_internal_l3_er_entry_lkup(pcid_info_t * pcid_info,
                                         uint32 *entry, int l3v6);
extern int soc_internal_mcmd_write(pcid_info_t * pcid_info,
                                   soc_mem_t mem, uint32 *data, int cmd);
#endif	/* BCM_EASYRIDER_SUPPORT */
#ifdef BCM_TRX_SUPPORT
extern int soc_internal_l2_tr_entry_ins(pcid_info_t *pcid_info,
                                        uint32 inv_bank_map, void *entry,
                                        uint32 *result);
extern int soc_internal_l2_tr_entry_del(pcid_info_t *pcid_info,
                                        uint32 inv_bank_map, void *entry,
                                        uint32 *result);
extern int soc_internal_l2_tr_entry_lkup(pcid_info_t *pcid_info,
                                         uint32 inv_bank_map, void *entry,
                                         uint32 *result);
extern int soc_internal_vlan_xlate_hash(pcid_info_t * pcid_info,
                                        vlan_xlate_entry_t *entry, int dual);

extern int soc_internal_vlan_xlate_entry_read(pcid_info_t * pcid_info, 
                                              uint32 addr,
                                              vlan_xlate_entry_t *entry);
extern int soc_internal_vlan_xlate_extended_entry_read(pcid_info_t * pcid_info, 
                                                       uint32 block, uint32 addr,
                                                       vlan_xlate_entry_t *entry);
extern int soc_internal_vlan_xlate_entry_write(pcid_info_t * pcid_info, 
                                              uint32 addr,
                                              vlan_xlate_entry_t *entry);
extern int soc_internal_vlan_xlate_extended_entry_write(pcid_info_t * pcid_info, 
                                                        uint32 block, uint32 addr,
                                                        vlan_xlate_entry_t *entry);
extern int soc_internal_vlan_xlate_entry_ins(pcid_info_t *pcid_info, 
                                             uint8 banks,
                                             vlan_xlate_entry_t *entry, 
                                             uint32 *result);
extern int soc_internal_vlan_xlate_entry_del(pcid_info_t * pcid_info, 
                                             uint8 banks,
                                             vlan_xlate_entry_t *entry, 
                                             uint32 *result);
extern int soc_internal_vlan_xlate_entry_lkup(pcid_info_t * pcid_info, 
                                              uint8 banks,
                                              vlan_xlate_entry_t *entry_lookup, 
                                              uint32 *result);

extern int soc_internal_egr_vlan_xlate_entry_ins(pcid_info_t *pcid_info,
                                                 uint8 banks,
                                                 egr_vlan_xlate_entry_t *entry,
                                                 uint32 *result);
extern int soc_internal_egr_vlan_xlate_entry_del(pcid_info_t * pcid_info,
                                                 uint8 banks,
                                                 egr_vlan_xlate_entry_t *entry,
                                                 uint32 *result);
extern int soc_internal_egr_vlan_xlate_entry_lkup(pcid_info_t * pcid_info,
                                                  uint8 banks,
                                                  egr_vlan_xlate_entry_t *entry_lookup,
                                                  uint32 *result);
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
extern int soc_internal_ing_vp_vlan_member_ins(pcid_info_t *pcid_info,
                                               uint8 banks, void *entry,
                                               uint32 *result);
extern int soc_internal_ing_vp_vlan_member_del(pcid_info_t *pcid_info,
                                               uint8 banks, void *entry,
                                               uint32 *result);
extern int soc_internal_ing_vp_vlan_member_lkup(pcid_info_t *pcid_info,
                                                uint8 banks, void *entry,
                                                uint32 *result);

extern int soc_internal_egr_vp_vlan_member_ins(pcid_info_t *pcid_info,
                                               uint8 banks, void *entry,
                                               uint32 *result);
extern int soc_internal_egr_vp_vlan_member_del(pcid_info_t *pcid_info,
                                               uint8 banks, void *entry,
                                               uint32 *result);
extern int soc_internal_egr_vp_vlan_member_lkup(pcid_info_t *pcid_info,
                                                uint8 banks, void *entry,
                                                uint32 *result);

extern int soc_internal_ing_dnat_address_type_ins(pcid_info_t *pcid_info,
                                               uint8 banks, void *entry,
                                               uint32 *result);
extern int soc_internal_ing_dnat_address_type_del(pcid_info_t *pcid_info,
                                               uint8 banks, void *entry,
                                               uint32 *result);
extern int soc_internal_ing_dnat_address_type_lkup(pcid_info_t *pcid_info,
                                                uint8 banks, void *entry,
                                                uint32 *result);

extern int soc_internal_l2_endpoint_id_ins(pcid_info_t *pcid_info, uint8 banks,
                                           void *entry, uint32 *result);
extern int soc_internal_l2_endpoint_id_del(pcid_info_t *pcid_info, uint8 banks,
                                           void *entry, uint32 *result);
extern int soc_internal_l2_endpoint_id_lkup(pcid_info_t *pcid_info,
                                            uint8 banks, void *entry,
                                            uint32 *result);

extern int soc_internal_endpoint_queue_map_ins(pcid_info_t *pcid_info,
                                               uint8 banks,
                                               void *entry, uint32 *result);
extern int soc_internal_endpoint_queue_map_del(pcid_info_t *pcid_info,
                                               uint8 banks,
                                               void *entry, uint32 *result);
extern int soc_internal_endpoint_queue_map_lkup(pcid_info_t *pcid_info,
                                                uint8 banks, void *entry,
                                                uint32 *result);
#endif /* BCM_TRIDNET2_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
extern int soc_internal_mpls_hash(pcid_info_t * pcid_info,
                                  mpls_entry_entry_t *entry, int dual);

extern int soc_internal_mpls_entry_read(pcid_info_t * pcid_info,
                                        uint32 addr,
                                        mpls_entry_entry_t *entry);
extern int soc_internal_mpls_entry_write(pcid_info_t * pcid_info,
                                         uint32 addr,
                                         mpls_entry_entry_t *entry);
extern int soc_internal_mpls_entry_ins(pcid_info_t *pcid_info,
                                       uint8 banks,
                                       mpls_entry_entry_t *entry,
                                       uint32 *result);
extern int soc_internal_mpls_entry_del(pcid_info_t * pcid_info,
                                       uint8 banks,
                                       mpls_entry_entry_t *entry,
                                       uint32 *result);
extern int soc_internal_mpls_entry_lkup(pcid_info_t * pcid_info,
                                        uint8 banks,
                                        mpls_entry_entry_t *entry_lookup,
                                        uint32 *result);

#endif /* BCM_TRIUMPH_SUPPORT */
#endif	/* _PCID_MEM_H */
