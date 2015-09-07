/*
 * $Id: aefc328890396334c9fae1817cb41dc27549e031 $
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
 * Broadcom System Log Console Sink
 */

#include <appl/diag/bsldnx.h>

#include <sal/core/libc.h> 
#include <sal/appl/io.h> 
#include <appl/diag/bslsink.h>
#include <shared/bsl.h>


STATIC int
bsldnx_cons_vfprintf(void *file, const char *format, va_list args)
{
    return vprintk(format, args);
}

STATIC int
bsldnx_cons_init(int* sink_id)
{
    static bslsink_sink_t console_sink;
    bslsink_sink_t *sink = bslsink_sink_find("dnx console");

    if (sink == NULL) { /*no dnx console - create a new one*/
        /* Create console sink */
        sink = &console_sink;
        bslsink_sink_t_init(sink);
        sal_strncpy(sink->name, "dnx console", sizeof(sink->name));
        sink->vfprintf = bsldnx_cons_vfprintf;
        sink->enable_range.min = bslSeverityOff+1;
        sink->enable_range.max = bslSeverityCount-1;
        sal_strncpy(sink->prefix_format, "%f[%l]%F unit %u:",
                    sizeof(sink->prefix_format));
        sink->prefix_range.min = bslSeverityOff+1;
        sink->prefix_range.max = bslSeverityWarn;

        /*clear all unit, they will be set in bsldnx_mgmt_init*/
        SHR_BITCLR_RANGE(sink->units, 0, BSLSINK_MAX_NUM_UNITS);
        *sink_id = bslsink_sink_add(sink);
    } else{
        *sink_id = sink->sink_id;
    }
    
    return 0;
}

STATIC int
bsldnx_unit_move(int unit, int old_sink_id, int new_sink_id)
{
    bslsink_sink_t *old_sink, *new_sink;

    old_sink = bslsink_sink_find_by_id(old_sink_id);
    if (old_sink == NULL){
        return -1;
    }

    new_sink = bslsink_sink_find_by_id(new_sink_id);
    if (new_sink == NULL){
        return -1;
    }
    SHR_BITCLR(old_sink->units, unit);
    SHR_BITSET(new_sink->units, unit);
    return 0;
}

int
bsldnx_mgmt_init(int unit)
{
    int dnx_console_sink_id;
    bslsink_sink_t *console_sink_find = bslsink_sink_find("console");
    if (console_sink_find == NULL){
        return -1;
    }
    if (bsldnx_cons_init(&dnx_console_sink_id) != 0) {
        return -1;
    }
    return bsldnx_unit_move(unit, console_sink_find->sink_id,  dnx_console_sink_id);
}

