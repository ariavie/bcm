/*
 * $Id: rpc.h,v 1.7 Broadcom SDK $
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
 * Remote Procedure Call BCM Dispatch Utilities
 */

#ifndef _BCM_INT_RPC_H
#define _BCM_INT_RPC_H

#ifdef  BCM_RPC_SUPPORT

#include <appl/cpudb/cpudb.h>
#include <bcm_int/rpc/entry.h>

typedef struct bcm_rpc_sreq_s {
    struct bcm_rpc_sreq_s  *next;
    cpudb_key_t         cpu;
    uint8               *buf;
    void                *cookie;
    uint32              rpckey[BCM_RPC_LOOKUP_KEYLEN];
} bcm_rpc_sreq_t;

extern void bcm_rpc_run(bcm_rpc_sreq_t *sreq);
extern uint8 *bcm_rpc_setup(uint8 dir, uint32 *key,
                            uint32 len, uint32 seq, uint32 arg);
extern void bcm_rpc_free(uint8 *buf, void *cookie);
extern int  bcm_rpc_request(int unit, uint8 *buf, int len, uint8 **rbuf,
                            void **cookie);
extern int  bcm_rpc_reply(cpudb_key_t cpu, uint8 *buf, int len);
extern int  bcm_rpc_start(void);
extern int  bcm_rpc_stop(void);
extern int  bcm_rpc_detach(int unit);
extern int  bcm_rpc_cleanup(cpudb_key_t cpu);

#endif  /* BCM_RPC_SUPPORT */
#endif  /* _BCM_INT_RPC_H */
