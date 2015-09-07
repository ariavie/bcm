/*
 * $Id: bradley.h,v 1.6 Broadcom SDK $
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
 * File:        bradley.h
 */

#ifndef _SOC_BRADLEY_H_
#define _SOC_BRADLEY_H_

#include <soc/drv.h>

extern int soc_bradley_misc_init(int);
extern int soc_bradley_mmu_init(int);
extern int soc_bradley_age_timer_get(int, int *, int *);
extern int soc_bradley_age_timer_max_get(int, int *);
extern int soc_bradley_age_timer_set(int, int, int);

extern void soc_bradley_mmu_parity_error(void *unit, void *d1, void *d2,
                                    void *d3, void *d4 );
#ifdef BCM_SCORPION_SUPPORT
extern void soc_scorpion_mem_config(int unit);
extern void soc_scorpion_parity_error(void *unit, void *d1, void *d2,
                                      void *d3, void *d4 );
#endif /* BCM_SCORPION_SUPPORT */

extern soc_functions_t soc_bradley_drv_funs;

#define SOC_PIPE_SELECT_X      0
#define SOC_PIPE_SELECT_Y      1

#define SOC_HBX_HIGIG2_MULTICAST_RANGE_MAX 0xffff
#define SOC_HBX_MULTICAST_RANGE_DEFAULT 4096

extern int soc_hbx_higig2_mcast_sizes_set(int unit, int bcast_size,
                                          int mcast_size, int ipmc_size);
extern int soc_hbx_higig2_mcast_sizes_get(int unit, int *bcast_size,
                                          int *mcast_size, int *ipmc_size);

extern int soc_hbx_mcast_size_set(int unit, int mc_size);
extern int soc_hbx_mcast_size_get(int unit, int *mc_base, int *mc_size);
extern int soc_hbx_ipmc_size_set(int unit, int ipmc_size);
extern int soc_hbx_ipmc_size_get(int unit, int *ipmc_base, int *ipmc_size);

#endif	/* !_SOC_BRADLEY_H_ */
