/*
 * $Id: robo.h,v 1.10 Broadcom SDK $
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
 * File:        robo.h
 */

#ifndef _SOC_ROBO_H_
#define _SOC_ROBO_H_

#include <soc/drv.h>

#define SOC_ROBO_PORT_INFO(unit, port)  (soc_robo_port_info[(unit)][(port)])
#define SOC_ROBO_PORT_MEDIUM_MODE_SET(unit, port, medium)   \
    (SOC_ROBO_PORT_INFO(unit, port).cur_medium = (medium))
#define SOC_ROBO_PORT_MEDIUM_MODE(unit, port)   \
    (SOC_ROBO_PORT_INFO(unit, port).cur_medium)
#define SOC_IS_RORO_PORT_MEDIUM_MODE_COPPER(unit, port) \
    SOC_ROBO_PORT_MEDIUM_MODE((unit),(port)) == (SOC_PORT_MEDIUM_COPPER) ?\
    TRUE : FALSE
#define SOC_ROBO_PORT_MAC_DRIVER(unit, port)    \
    (SOC_ROBO_PORT_INFO(unit, port).p_mac)
#define SOC_ROBO_PORT_INIT(unit) \
    if (soc_robo_port_info[unit] == NULL) { return SOC_E_INIT; }

typedef struct soc_robo_port_info_s 
{
    mac_driver_t    *p_mac;     /* Per port MAC driver */
    int            p_ut_prio;    /* Untagged priority */

    uint8   cur_medium;     /* current medium (copper/fiber) */

    uint32 ing_sample_rate; /* ingress sample rate */
    uint32 eg_sample_rate; /* egress sample rate */
    uint32 ing_sample_prio; /* priority of ingress sample packets */
    uint32 eg_sample_prio; /* priority of egress sample packets */
} soc_robo_port_info_t;

extern soc_robo_port_info_t *soc_robo_port_info[SOC_MAX_NUM_DEVICES];

extern int soc_robo_misc_init(int);
extern int soc_robo_mmu_init(int);
extern int soc_robo_age_timer_get(int, int *, int *);
extern int soc_robo_age_timer_max_get(int, int *);
extern int soc_robo_age_timer_set(int, int, int);
extern int soc_robo_64_val_to_pbmp(int, soc_pbmp_t *, uint64);
extern int soc_robo_64_pbmp_to_val(int , soc_pbmp_t *, uint64 *);


extern soc_functions_t soc_robo_drv_funs;
extern int bcm53222_attached;

extern int soc_robo_5324_mmu_default_set(int unit);
extern int soc_robo_5348_mmu_default_set(int unit);

extern void soc_robo_counter_thread_run_set(int run);

extern int soc_robo_dos_monitor_enable_set(int unit, sal_usecs_t interval);
extern int soc_robo_dos_monitor_enable_get(int unit, sal_usecs_t *interval);
extern int soc_robo_dos_monitor_last_event(int unit, uint32 *events_bmp);
extern int soc_robo_dos_monitor_init(int unit);
extern int soc_robo_dos_monitor_deinit(int unit);

#endif  /* !_SOC_ROBO_H_ */
