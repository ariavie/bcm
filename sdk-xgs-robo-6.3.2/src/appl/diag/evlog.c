/*
 * $Id: evlog.c 1.5 Broadcom SDK $
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
 * File:        evlog.c
 * Purpose:
 * Requires:    
 */

#include <shared/evlog.h>
#include <appl/diag/evlog.h>
#include <sal/core/libc.h>
#include <shared/alloc.h>

#include <bcm/error.h>

#if defined(INCLUDE_SHARED_EVLOG)

#define EV_ALLOC(ev, targ, size) do {                                   \
        targ = sal_alloc(size, "evlog");                                \
        if ((targ) == NULL) {                                           \
            diag_evlog_destroy(ev);                                     \
            return NULL;                                                \
        }                                                               \
        sal_memset(targ, 0, size);                                      \
    } while (0)

shared_evlog_t *
diag_evlog_create(int max_args, int max_entries)
{
    shared_evlog_t *ev = NULL;

    EV_ALLOC(ev, ev, sizeof(*ev));
    EV_ALLOC(ev, ev->ev_strs, (sizeof(char *) * max_entries));
    EV_ALLOC(ev, ev->ev_args, (sizeof(int) * max_args * max_entries));
    EV_ALLOC(ev, ev->ev_ts, (sizeof(sal_usecs_t) * max_entries));
    EV_ALLOC(ev, ev->ev_tid, (sizeof(sal_thread_t) * max_entries));
    EV_ALLOC(ev, ev->ev_flags, (sizeof(uint32) * max_entries));
    ev->max_entries = max_entries;
    ev->max_args = max_args;

    return ev;
}
#undef EV_ALLOC

#define EV_FREE(targ) if ((targ) != NULL) sal_free(targ)

void
diag_evlog_destroy(shared_evlog_t *ev)
{
    if (ev != NULL) {
        EV_FREE(ev->ev_strs);
        EV_FREE(ev->ev_args);
        EV_FREE(ev->ev_ts);
        EV_FREE(ev->ev_tid);
        EV_FREE(ev->ev_flags);

        sal_free(ev);
    }
}
#undef EV_FREE


/* To do:  Implement a reasonable "dump" function */
int
diag_evlog_dump(shared_evlog_t *ev, int count, int oldest)
{
    /*
     * If count == 0, dump all entries in log
     * If count > 0, dump that many entries moving forward in time.
     * If count < 0, dump that many entries moving backward in time.
     *
     * If oldest is true, dump earliest entries.
     * Otherwise dump most recent entries.
     */

    return BCM_E_UNAVAIL;
}

#endif  /* INCLUDE_SHARED_EVLOG */
