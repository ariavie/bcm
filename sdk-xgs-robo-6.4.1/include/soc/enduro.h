/*
 * $Id: enduro.h,v 1.13 Broadcom SDK $
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
 * File:        enduro.h
 */

#ifndef _SOC_ENDURO_H_
#define _SOC_ENDURO_H_

#include <soc/drv.h>
#include <shared/sram.h>

typedef int (*soc_enduro_oam_handler_t)(int unit, soc_field_t fault_field);

typedef _shr_ext_sram_entry_t en_ext_sram_entry_t;
 
#define SOC_EN_BROADCAST_RANGE_DEFAULT 4096
#define SOC_EN_L2_MULTICAST_RANGE_DEFAULT 1024
#define SOC_EN_IP_MULTICAST_RANGE_DEFAULT 2048

extern int soc_enduro_misc_init(int);
extern int soc_enduro_mmu_init(int);
extern int soc_enduro_age_timer_get(int, int *, int *);
extern int soc_enduro_age_timer_max_get(int, int *);
extern int soc_enduro_age_timer_set(int, int, int);
extern void soc_enduro_oam_handler_register(int unit, soc_enduro_oam_handler_t handler);
extern void soc_enduro_parity_error(void *unit_vp, void *d1, void *d2,
                                     void *d3, void *d4);
extern int _soc_enduro_mem_parity_control(int unit, soc_mem_t mem,
                                            int copyno, int enable);
extern void soc_enduro_mem_config(int unit);
extern soc_functions_t soc_enduro_drv_funs;
extern void soc_enduro_mem_nack(void *unit_vp, void *addr_vp, void *blk_vp, 
                     void *d3, void *d4);
extern int soc_enduro_ser_inject_error(int unit, uint32 flags, soc_mem_t mem,
                               int pipeTarget, int block, int index);
extern soc_error_t soc_enduro_ser_error_injection_support (int unit, soc_mem_t mem, int pipe);
extern int soc_enduro_l3_defip_index_map(int unit, soc_mem_t mem, int index);
#endif	/* !_SOC_ENDURO_H_ */
