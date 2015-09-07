/*
 * $Id: progress.c 1.9 Broadcom SDK $
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
 * Progress Report module
 *
 *   Usage:
 *
 *	Call progress_init and pass it a measure of the total count.
 *		To avoid messing up the display, try not to print
 *		anything from now until you've called progress_done.
 *
 *	In your loop, call progress_report on every iteration and pass it
 *		an increment to the current count (usually 1).
 *		Regardless of how often it is called, an adaptive
 *		algorithm calls time() on average once per second,
 *		and updates the display if the percentage changes.
 *
 *	Call progress_status at any time to add or change a status
 *		message displayed along with the percent done.
 *
 *	Call progress_done when finished.
 *
 *   If the entire procedure completes before start_seconds
 *		elapses, nothing is printed at all.
 */

#include <appl/diag/progress.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>
#include <sal/core/time.h>

struct {
    int start_seconds;
    int disable;
    uint32 total_count;
    uint32 partial_count;
    sal_time_t start_time;
    sal_time_t check_time;
    int printed_something;
    int last_frac_printed;
    int iter_per_check_time;
    int iter_count;
    char *status_str;
} progress;

#define VT100_CLEAR_TO_EOL	"\033\133\113"

void
progress_init(uint32 total_count, int start_seconds, int disable)
{
    progress.start_seconds = start_seconds;
    progress.disable = disable;
    progress.total_count = total_count;
    progress.partial_count = 0;
    progress.start_time = sal_time();
    progress.check_time = progress.start_time;
    progress.printed_something = 0;
    progress.last_frac_printed = -1;
    progress.iter_per_check_time = 1;
    progress.iter_count = 0;
    progress.status_str = 0;
}

void
progress_status(char *status_str)
{
    progress.status_str = status_str;
    progress.last_frac_printed = -1;
}

void
progress_report(uint32 count_incr)
{
    sal_time_t tm;
    if (progress.disable)
	return;
    if ((progress.partial_count += count_incr) > progress.total_count)
	progress.partial_count = progress.total_count;
    if (++progress.iter_count < progress.iter_per_check_time)
	return;
    tm = sal_time();
    if (progress.check_time == tm) {
	int old_iter = progress.iter_per_check_time;
	/* Calling time() more than once per second; slow down */
	progress.iter_per_check_time =
	    progress.iter_per_check_time * 5 / 4;
	if (progress.iter_per_check_time == old_iter)
	    progress.iter_per_check_time++;
    } else {
	if (tm > progress.check_time + 1) {
	    /* Calling time() less than once per second; speed up */
	    progress.iter_per_check_time =
		progress.iter_per_check_time * 4 / 5;
	}
	progress.check_time = tm;
	if (tm > progress.start_time + progress.start_seconds) {
	    int frac;
	    if (progress.total_count >= 0x1000000)	/* Don't overflow */
		frac = progress.partial_count / (progress.total_count / 100);
	    else
		frac = (100 * progress.partial_count) / progress.total_count;
	    if (frac != progress.last_frac_printed) {
		printk("\r[%d%% done] %s " VT100_CLEAR_TO_EOL,
			    frac,
			    progress.status_str ? progress.status_str : "");
		progress.printed_something = 1;
		progress.last_frac_printed = frac;
	    }
	}
    }
    progress.iter_count = 0;
}

void
progress_done(void)
{
    if (progress.printed_something)
	printk("\r" VT100_CLEAR_TO_EOL);
}
