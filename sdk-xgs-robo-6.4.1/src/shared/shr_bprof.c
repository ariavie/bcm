/*
 * $Id: 03fb9d85be54afd53e09255874ddb981468a81c9 $
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
 * File: 	shr_bprof.c
 * 
 */
#include <sal/types.h>
#include <sal/core/time.h>
#include <shared/shr_bprof.h>
#include <shared/error.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>

static sal_usecs_t _bprof_stime_main_start;
static sal_usecs_t _bprof_stime_main_end;
static shr_bprof_stats_entry_t _bprof_stats[SHR_BPROF_STATS_MAX];

/* Max name should be of length < SHR_BPROF_STATS_MAX_NAME 
  This should match with shr_bprof_type_e in shr_bprof.h */
static char _bprof_stats_names[SHR_BPROF_STATS_MAX][SHR_BPROF_STATS_MAX_NAME] = {
    "soc probe",
    "soc attach",
    "init soc",
    "init misc",
    "init mmu",
    "init bcm",
    "commom",
    "port",
    "l2",
    "stg",
    "vlan",
    "trunk",
    "cosq",
    "mCast",
    "linkscan",
    "stat",
    "stk",
    "rate",
    "knet",
    "field",
    "mirror",
    "tx",
    "rx",
    "l3",
    "ipmc",
    "mpls",
    "mim",
    "wlan",
    "proxy",
    "subport",
    "qos",
    "trill",
    "niv",
    "l2gre",
    "vxlan",
    "extender",
    "multicast",
    "auth",
    "regex",
    "time",
    "oam",
    "failover",
    "ces",
    "ptp",
    "bfd",
    "global_meter",
    "fcoe"
};

void shr_bprof_stats_time_init(void)
{
    _bprof_stime_main_start = sal_time_usecs();

    return;
}

void shr_bprof_stats_time_end(void)
{
    _bprof_stime_main_end = sal_time_usecs();

    return;
}

sal_usecs_t shr_bprof_stats_time_taken(void)
{
    return (_bprof_stime_main_end - _bprof_stime_main_start);
}

char * shr_bprof_stats_name(int dispname)
{
    return _bprof_stats_names[dispname];
}

void shr_bprof_stats_timer_start(int op)
{
    sal_usecs_t now;                                                  

    now = sal_time_usecs();
    _bprof_stats[op].start_stime = now;

    return;
}

void shr_bprof_stats_timer_stop(int op)
{
    sal_usecs_t now;
    sal_usecs_t delta;

    now = sal_time_usecs();
    delta = now - _bprof_stats[op].start_stime;

    if (!_bprof_stats[op].runs) {
            _bprof_stats[op].total_stime += delta;
            _bprof_stats[op].runs++;
            _bprof_stats[op].min_stime = delta;
            _bprof_stats[op].max_stime = delta;
    } else {
        _bprof_stats[op].total_stime += delta;
        _bprof_stats[op].runs++;

        if (delta < _bprof_stats[op].min_stime) {
            _bprof_stats[op].min_stime = delta;
        } else if (delta > _bprof_stats[op].max_stime) {
            _bprof_stats[op].max_stime = delta;
        } 
    }

    return;
}

int shr_bprof_stats_get(int idx, shr_bprof_stats_entry_t *bprof_stats)
{
    if ((idx < 0) || (idx >= SHR_BPROF_STATS_MAX)) {
        return _SHR_E_UNAVAIL;
    }

    bprof_stats->start_stime = _bprof_stats[idx].start_stime;
    bprof_stats->total_stime = _bprof_stats[idx].total_stime;
    bprof_stats->max_stime = _bprof_stats[idx].max_stime;
    bprof_stats->min_stime = _bprof_stats[idx].min_stime;
    bprof_stats->runs = _bprof_stats[idx].runs;

    return _SHR_E_NONE;
}
