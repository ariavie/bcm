/*
 * $Id: firebolt.h,v 1.21 Broadcom SDK $
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
 * File:        firebolt.h
 */

#ifndef _SOC_FIREBOLT_H_
#define _SOC_FIREBOLT_H_

#include <soc/drv.h>

extern int soc_firebolt_misc_init(int);
extern int soc_firebolt_internal_mmu_init(int);
extern int soc_firebolt_mmu_init(int);
extern int soc_firebolt_age_timer_get(int, int *, int *);
extern int soc_firebolt_age_timer_max_get(int, int *);
extern int soc_firebolt_age_timer_set(int, int, int);
extern int soc_firebolt_stat_get(int, soc_port_t, int, uint64*);

extern void soc_fb_mmu_parity_error(void *unit, void *d1, void *d2,
                                    void *d3, void *d4 );
extern int soc_fb_xq_mem(int unit, soc_port_t port, soc_mem_t *xq);

extern soc_functions_t soc_firebolt_drv_funs;

extern void soc_helix_mem_config(int unit);
extern void soc_bcm53300_mem_config(int unit);

#ifdef BCM_RAPTOR_SUPPORT
extern void soc_bcm53724_mem_config(int unit);
extern void soc_bcm56225_mem_config(int unit);
#endif

/* Modport port to hg bitmap mapping function */
extern int soc_xgs3_port_to_higig_bitmap(int unit, soc_port_t port, 
                                         uint32 *hg_reg);
extern int soc_xgs3_higig_bitmap_to_port(int unit, uint32 hg_reg, 
                                         soc_port_t *port);
extern int soc_xgs3_higig_bitmap_to_port_all(int unit, uint32 hg_pbm,
                                             int port_max,
                                             soc_port_t *port_array,
                                             int *port_count);
extern int soc_xgs3_higig_bitmap_to_bitmap(int unit, pbmp_t hg_pbmp, pbmp_t *pbmp);
extern int soc_xgs3_bitmap_to_higig_bitmap(int unit, pbmp_t pbmp, pbmp_t *hg_pbm);
extern int soc_xgs3_higig_bitmap_to_higig_port_num(uint32 hg_pbm, uint32 *hg_port_num);
extern int soc_xgs3_port_num_to_higig_port_num(int unit, int port,
                                               int *hg_port);


#ifdef BCM_FIREBOLT2_SUPPORT
extern void soc_firebolt2_mem_config(int unit);
#endif /* BCM_FIREBOLT2_SUPPORT */

#define SOC_FB2_PROD_CFG_HX_SHIFT       8
#define SOC_FB2_PROD_CFG_HX_MASK        0xf

#endif	/* !_SOC_FIREBOLT_H_ */
