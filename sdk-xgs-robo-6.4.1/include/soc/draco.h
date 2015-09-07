/*
 * $Id: draco.h,v 1.4 Broadcom SDK $
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
 * File:        draco.h
 */

#ifndef _SOC_DRACO_H_
#define _SOC_DRACO_H_

#include <soc/drv.h>

extern int soc_draco_misc_init(int);
extern int soc_draco_mmu_init(int);
extern int soc_draco_age_timer_get(int, int *, int *);
extern int soc_draco_age_timer_max_get(int, int *);
extern int soc_draco_age_timer_set(int, int, int);
extern int soc_draco_stat_get(int, soc_port_t, int, uint64*);

extern soc_functions_t soc_draco_drv_funs;
extern soc_functions_t soc_draco15_drv_funs;

extern int soc_draco15_mmu_init(int);
extern int soc_draco15_mac_hash(int unit, uint8 *key);
extern int soc_draco15_mac_software_hash(int unit, int hash_sel, 
					    uint8 *key);

extern int soc_mem_draco15_vlan_mac_search(int unit, int *index_ptr, 
					   void *entry_data);
extern int soc_mem_draco15_vlan_mac_insert(int unit, void *entry_data);
extern int soc_mem_draco15_vlan_mac_delete(int unit, void *key_data); 

#define SOC_DRACO_PORT_TYPE_DEFAULT 0
#define SOC_DRACO_PORT_TYPE_TRUNK 1
#define SOC_DRACO_PORT_TYPE_STACKING 2
#define SOC_DRACO_PORT_TYPE_RESERVED 3

#endif	/* !_SOC_DRACO_H_ */
