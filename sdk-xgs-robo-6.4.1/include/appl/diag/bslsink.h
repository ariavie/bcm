/*
 * $Id: 72150511024e0c5f372ba2cde4e86b8ffe645da5 $
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
 * Broadcom System Log Sinks
 */

#ifndef _DIAG_BSLSINK_H
#define _DIAG_BSLSINK_H

#include <stdarg.h>
#include <shared/bslext.h>
#include <bcm/types.h>

#define BSLSINK_SINK_NAME_MAX           32
#define BSLSINK_PREFIX_FORMAT_MAX       32

#ifndef BSLSINK_PREFIX_MAX
#define BSLSINK_PREFIX_MAX              128
#endif

#define BSLSINK_MAX_NUM_UNITS           (BCM_UNITS_MAX + 1)
#define BSLSINK_MAX_NUM_PORTS           BCM_PBMP_PORT_MAX

/* Pseudo-unit to allow separate control of BSL_UNIT_UNKNOWN messages */
#define BSLSINK_UNIT_UNKNOWN            (BSLSINK_MAX_NUM_UNITS - 1)

typedef struct bslsink_severity_range_s {
    bsl_severity_t min;
    bsl_severity_t max;
} bslsink_severity_range_t;

typedef int (*bslsink_vfprintf_f)(void *, const char *, va_list);

typedef struct bslsink_sink_s {

    /* Next sink */
    struct bslsink_sink_s *next;

    /* Sink name */
    char name[BSLSINK_SINK_NAME_MAX];

    /* Unique ID for retrieval and removal */
    int sink_id;

    /* Low-level output function */
    bslsink_vfprintf_f vfprintf;

    /* Opaque file handle */
    void *file;

    /* Messages within this severity range will be printed */
    bslsink_severity_range_t enable_range;

    /* Messages within this severity range will be prefixed */
    bslsink_severity_range_t prefix_range;

    /* Messages with these units will be printed (use -1 to skip check) */
    SHR_BITDCLNAME(units, BSLSINK_MAX_NUM_UNITS);

    /* Messages with these ports will be printed (use -1 to skip check) */
    SHR_BITDCLNAME(ports, BSLSINK_MAX_NUM_PORTS);

    /* Messages with this parameter will be printed (-1 to ignore) */
    int xtra;

    /* Skip this many characters of the BSL_FILE path */
    int path_offset;

    /* Prefix configuration */
    char prefix_format[BSLSINK_PREFIX_FORMAT_MAX+1];

} bslsink_sink_t;

extern int
bslsink_out(bslsink_sink_t *sink, bsl_meta_t *meta,
            const char *format, va_list args);

extern void
bslsink_sink_t_init(bslsink_sink_t *sink);

extern int
bslsink_sink_add(bslsink_sink_t *new_sink);

extern bslsink_sink_t *
bslsink_sink_find(const char *name);

extern bslsink_sink_t *
bslsink_sink_find_by_id(int sink_id);

extern int
bslsink_init(void);

#endif /* !_DIAG_BSLSINK_H */
