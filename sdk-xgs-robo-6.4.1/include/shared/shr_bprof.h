/* 
 * $Id: f10dbb99706e65589776888381e9577f44515339 $
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
 * File:        shr_bprof.h
 * Purpose:     Defines profiling tool data structures 
 * Usage:   define enum for the profiling section of code and add entry for 
 * corresponding text in shr_bprof.c and use SHR_BPROF_STATS_TIME()
 */
#ifndef _SHR_BPROF_H
#define _SHR_BPROF_H

#include <sal/core/time.h>

#define SHR_BPROF_STATS_MAX_NAME     80

typedef struct shr_bprof_stats_entry_s {
    sal_usecs_t start_stime;
    sal_usecs_t total_stime;
    sal_usecs_t max_stime;
    sal_usecs_t min_stime;
    uint32 runs;
} shr_bprof_stats_entry_t;

/* This should match with _bprof_stats_names in shr_bprof.c */
typedef enum shr_bprof_type_e {
    SHR_BPROF_SOC_PROBE = 0,
    SHR_BPROF_SOC_ATTACH,
    SHR_BPROF_SOC_INIT, 
    SHR_BPROF_SOC_MISC,
    SHR_BPROF_SOC_MMU,
    SHR_BPROF_BCM_INIT,
    SHR_BPROF_BCM_COMMON,
    SHR_BPROF_BCM_PORT,
    SHR_BPROF_BCM_L2,
    SHR_BPROF_BCM_STG,
    SHR_BPROF_BCM_VLAN,
    SHR_BPROF_BCM_TRUNK,
    SHR_BPROF_BCM_COSQ,
    SHR_BPROF_BCM_MCAST,
    SHR_BPROF_BCM_LINKSCAN,
    SHR_BPROF_BCM_STAT,
    SHR_BPROF_BCM_STK,
    SHR_BPROF_BCM_RATE,
    SHR_BPROF_BCM_KNET,
    SHR_BPROF_BCM_FIELD,
    SHR_BPROF_BCM_MIRROR,
    SHR_BPROF_BCM_TX,
    SHR_BPROF_BCM_RX,
    SHR_BPROF_BCM_L3,
    SHR_BPROF_BCM_IPMC,
    SHR_BPROF_BCM_MPLS,
    SHR_BPROF_BCM_MIM,
    SHR_BPROF_BCM_WLAN,
    SHR_BPROF_BCM_PROXY,
    SHR_BPROF_BCM_SUBPORT,
    SHR_BPROF_BCM_QOS,
    SHR_BPROF_BCM_TRILL,
    SHR_BPROF_BCM_NIV,
    SHR_BPROF_BCM_L2GRE,
    SHR_BPROF_BCM_VXLAN,
    SHR_BPROF_BCM_EXTENDER,
    SHR_BPROF_BCM_MULTICAST,
    SHR_BPROF_BCM_AUTH,
    SHR_BPROF_BCM_REGEX,
    SHR_BPROF_BCM_TIME,
    SHR_BPROF_BCM_OAM,
    SHR_BPROF_BCM_FAILOVER,
    SHR_BPROF_BCM_CES,
    SHR_BPROF_BCM_PTP,
    SHR_BPROF_BCM_BFD,
    SHR_BPROF_BCM_GLB_METER,
    SHR_BPROF_BCM_FCOE,
    SHR_BPROF_BCM_UDF,
    SHR_BPROF_STATS_MAX
} shr_bprof_type_t;

char * shr_bprof_stats_name(int dispname);
void shr_bprof_stats_time_init(void);
void shr_bprof_stats_time_end(void);
sal_usecs_t shr_bprof_stats_time_taken(void);
void shr_bprof_stats_timer_start(int op);
void shr_bprof_stats_timer_stop(int op);
int shr_bprof_stats_get(int idx, shr_bprof_stats_entry_t *bprof_stats);

#ifdef BCM_BPROF_STATS
#define SHR_BPROF_STATS_DECL  volatile int _once

#define SHR_BPROF_STATS_TIME(stat)            \
    shr_bprof_stats_timer_start(stat);        \
    for (_once = 0; _once == 0; shr_bprof_stats_timer_stop(stat), _once = 1)
#else
#define SHR_BPROF_STATS_DECL
#define SHR_BPROF_STATS_TIME(stat)
#endif /* BCM_BPROF_STATS */
#endif /*_SHR_BPROF_H */
