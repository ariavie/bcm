/*
 * $Id: lport.h 1.4 Broadcom SDK $
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
 * File:        lport.h
 * Purpose:     BCMX Internal Logical and User Port definitions
 */

#ifndef   _BCMX_INT_LPORT_H_
#define   _BCMX_INT_LPORT_H_

#include <bcm/types.h>
#include <bcmx/types.h>

typedef struct _bcmx_port_s         _bcmx_port_t;
typedef struct _bcmx_uport_hash_s   _bcmx_uport_hash_t;

/* BCMX Logical Port */
struct _bcmx_port_s {
    int             bcm_unit;    /* Unit port belongs to */
    bcm_port_t      bcm_port;    /* Port number relative to bcm unit */
    bcmx_uport_t    uport;       /* Cookie for application lport */
    uint32          flags;       /* Flags BCMX_PORT_F_xxx */
    int             modid;       /* Module id for this lport */
    bcm_port_t      modport;     /* Module relative port (0..31) */
};

/* Hash is allocated on BCMX initialization */
struct _bcmx_uport_hash_s {
    bcmx_uport_t uport;
    bcmx_lport_t lport;
    _bcmx_uport_hash_t *next;
    _bcmx_uport_hash_t *prev;
};

/* 
 * BCMX application logical port definitions.
 * 
 * The mapping: "application port -> BCMX logical port" is currently a
 * hash + doubly linked list. It is assumed that adding/removing ports is
 * a relatively rare occurrance and that an allocation/free call is
 * acceptable during this process.
 * 
 * Alternatively:
 * 
 *   ((((PTR_TO_INT(uport)) >> 16) ^ ((PTR_TO_INT(uport)) & 0xffff)) % \
 *       BCMX_UPORT_HASH_COUNT)
 */
#define BCMX_UPORT_HASH_COUNT   73       /* Prime */
#define BCMX_UPORT_HASH(uport)  ((PTR_TO_INT(uport)) % BCMX_UPORT_HASH_COUNT) 
extern _bcmx_uport_hash_t *_bcmx_uport_hash[BCMX_UPORT_HASH_COUNT];


/*
 * Deprecated - No longer supported.
 */
extern int bcmx_lport_max;                  /* Set to 0 */
extern bcmx_lport_t _bcmx_lport_first;
extern bcmx_uport_t _bcmx_uport_invalid;
extern _bcmx_port_t *_bcmx_port;

#endif /* _BCMX_INT_LPORT_H_ */
