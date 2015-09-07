/* 
 * $Id: rlink.h,v 1.5 Broadcom SDK $
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
 * File:        rlink.h
 * Purpose:     RLINK protocol interfaces
 */

#ifndef   _BCM_SRC_RLINK_H
#define   _BCM_SRC_RLINK_H

#include <shared/idents.h>

#define	RLINK_CLIENT_ID		SHARED_CLIENT_ID_RLINK

typedef enum {
    RLINK_MSG_ADD,		/* scan add (client to server) */
    RLINK_MSG_DELETE,		/* scan delete (client to server) */
    RLINK_MSG_NOTIFY,		/* run handlers (server to client) */
    RLINK_MSG_TRAVERSE          /* traverse processing */
} rlink_msg_t;

typedef enum {
    RLINK_TYPE_LINK = 1,        /* linkscan */
    RLINK_TYPE_AUTH,            /* auth unauth callback */
    RLINK_TYPE_L2,              /* l2 notify */
    RLINK_TYPE_RX,              /* RX tunnelling */
    RLINK_TYPE_ADD,		/* Msg add processing */
    RLINK_TYPE_DELETE,		/* Msg delete processing */
    RLINK_TYPE_OAM,             /* oam event */
    RLINK_TYPE_BFD,             /* bfd event */
    RLINK_TYPE_FABRIC,          /* fabric event */
    RLINK_TYPE_START,           /* traverse start c>s */
    RLINK_TYPE_NEXT,            /* traverse next c>s */
    RLINK_TYPE_QUIT,            /* traverse quit c>s */
    RLINK_TYPE_ERROR,           /* traverse error s>c */
    RLINK_TYPE_MORE,            /* traverse more s>c */
    RLINK_TYPE_DONE             /* traverse done s>c */
} rlink_type_t;


extern uint8 *bcm_rlink_decode(uint8 *buf,
                               rlink_msg_t *msgp,
                               rlink_type_t *typep,
                               int *unitp);

extern uint8 *bcm_rlink_encode(uint8 *buf,
                               rlink_msg_t msg,
                               rlink_type_t type,
                               int unit);


#endif /* _BCM_SRC_RLINK_H */
