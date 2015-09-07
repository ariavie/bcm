/*
 * $Id: 8c37962a44d39cdc667f672dec75b0f7085e0f93 $
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
 * Broadcom System Log Management
 */

#include <sal/core/libc.h> 
#include <sal/appl/io.h> 
#include <shared/bsl.h>
#include <shared/bslnames.h>
#include <appl/diag/bslenable.h>
#include <appl/diag/bslcons.h>
#include <appl/diag/bslfile.h>
#include <appl/diag/bslsink.h>
#include <appl/diag/bslmgmt.h>

/*
 * Output hook for core BSL configuration
 */
STATIC int
bslmgmt_out_hook(bsl_meta_t *meta, const char *format, va_list args)
{
    bslsink_sink_t *sink = bslsink_sink_find_by_id(0);

    while (sink != NULL) {
        bslsink_out(sink, meta, format, args);
        sink = sink->next;
    }
    return 0;
}

/*
 * Check hook for core BSL configuration
 */
STATIC int
bslmgmt_check_hook(bsl_packed_meta_t meta_pack)
{
    int layer, source, severity;

    layer = BSL_LAYER_GET(meta_pack);
    source = BSL_SOURCE_GET(meta_pack);
    severity = BSL_SEVERITY_GET(meta_pack);

    return (severity <= bslenable_get(layer, source));
}

int
bslmgmt_init(void)
{
    bsl_config_t bsl_config;

    bslenable_reset_all();

    bsl_config_t_init(&bsl_config);
    bsl_config.out_hook = bslmgmt_out_hook;
    bsl_config.check_hook = bslmgmt_check_hook;
    bsl_init(&bsl_config);

    /* Initialize output hook */
    bslsink_init();

    /* Create console sink */
    bslcons_init();

#if 0
    /* Create file sink */
    bslfile_init();
#endif

    return 0;
}
