/*
 * $Id: 960c0c5aca0d4c84920abb182c649e74de27ca5a $
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
 * Broadcom System Log Enables
 */

#include <shared/bslnames.h>
#include <appl/diag/bslenable.h>

/* Table for tracking which layers/sources/severities to enable */
static bsl_severity_t bslenable_severity[bslLayerCount][bslSourceCount];

/* Which sources are valid in which layers (fixed) */
BSL_SOURCES_VALID_DEF(bsl_source_t);
static bsl_source_t *sources_valid[] = BSL_SOURCES_VALID_INIT;

int
bslenable_source_valid(bsl_layer_t layer, bsl_source_t source)
{
    bsl_source_t *src;
    int idx;

    if (layer < 0 || layer >= bslLayerCount) {
        return 0;
    }
    if (source < 0 || source >= bslSourceCount) {
        return 0;
    }
    src = sources_valid[layer];
    for (idx = 0; src[idx] != bslSourceCount && idx < bslSourceCount; idx++) {
        if (source == src[idx]) {
            return 1;
        }
    }
    return 0;
}

void
bslenable_set(bsl_layer_t layer, bsl_source_t source, bsl_severity_t severity)
{
    if (layer < 0 || layer >= bslLayerCount) {
        return;
    }
    bslenable_severity[layer][source] = severity;
}

bsl_severity_t
bslenable_get(bsl_layer_t layer, bsl_source_t source)
{
    if (layer < bslLayerCount && source < bslSourceCount) {
        return bslenable_severity[layer][source];
    }
        
    return 0;
}

void
bslenable_reset(bsl_layer_t layer, bsl_source_t source)
{
    bsl_severity_t severity = bslSeverityWarn;

    if ((source == bslSourceShell) ||
        ((layer == bslLayerTks) && (source == bslSourceCommon))) {
        severity = bslSeverityInfo;
    }
        
    bslenable_set(layer, source, severity);
}

void
bslenable_reset_all(void)
{
    int layer, source;

    for (layer = 0; layer < bslLayerCount; layer++) {
        for (source = 0; source < bslSourceCount; source++) {
            bslenable_reset(layer, source);
        }
    }
}

int
bslenable_init(void)
{
    bslenable_reset_all();

    return 0;
}
