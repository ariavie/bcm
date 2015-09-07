/*
 * $Id: acl_util.h,v 1.16 Broadcom SDK $
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
 * File:        acl_util.h
 *     
 * Purpose:     
 *     Access Control List (ACL) utility functions. These
 *     are intended primarily to provide additional support for the CLI code.
 *     and API prototypes.
 */

#ifndef   _ACL_UTIL_H_
#define   _ACL_UTIL_H_

#if defined(INCLUDE_ACL)

#include <sal/core/sync.h>
#include <appl/acl/acl.h>

#define ACL_MASK_ETHERTYPE      (0xffff)
#define ACL_MASK_IPPROTOCOL     (0xff)

/*
 * Typedef: _acl_rule_link_t
 *
 * Purpose:
 *     Linked-list wrapper for public bcma_acl_rule_t type.
 *
 * Fields:
 *     rule  - Public rule parameters
 *     *next - Link to next node rule
 *
 */
typedef struct _acl_rule_link_s {
    bcma_acl_rule_t          *rule;     /* Public rule parameters     */
    uint32                   entries;   /* Number of physical entries */
    struct _acl_rule_link_s  *next;     /* Link to next node rule     */
} _acl_rule_link_t;

/*
 * Typedef: _acl_link_t
 *
 * Purpose:
 *     Linked-list wrapper for public bcma_acl_t type.
 *
 * Fields:
 *     - <bcma_acl_t> list Public data for this list node
 *     - <_acl_rule_link_t> *rules Linked list of rules
 *     - <_acl_rule_link_t> *cur Current rule, used for iterating through list
 *     - <_acl_link_t> *next    Link to next list node
 *
 */
typedef struct _acl_link_s {
    bcma_acl_t               *list;     /* This list node public data */
    _acl_rule_link_t         *rules;    /* Linked list of rules       */
    _acl_rule_link_t         *cur;      /* Current rule               */
    struct _acl_link_s       *prev;     /* Link to previous list node */
    struct _acl_link_s       *next;     /* Link to next list node     */
} _acl_link_t;

/*
 * Typedef: _acl_control_t
 *
 * Purpose:
 *    System-wide ACL data.
 *
 * Fields:
 *    - <_acl_link_t> *lists     Linked-list of ACLs
 *    - <_acl_link_t> *cur       Current ACL; used for iterating over lists
 *    - sal_mutex_t acl_lock     System lock
 *
 */
typedef struct _acl_control_s {
    _acl_link_t         head;           /* Linked-list of ACLs              */
    _acl_link_t         *cur;           /* Current ACL                      */
    sal_mutex_t         acl_lock;       /* System lock                      */
}_acl_control_t;

/*
 * Typedef: acl_node_t
 *
 * Purpose:
 *     Single element in a list of nodes that make up a range. For instance, if
 *     someone qualifies on a range of VLAN IDs, this will get implemented as a
 *     list of data/mask pairs that cover the requested range.
 *
 * Fields:
 *     data -
 *     mask -
 *
 */
typedef struct acl_node_s {
    uint16              data;
    uint16              mask;
    struct acl_node_s   *next;
} acl_node_t;

#define _BCMA_ACL_MAC_ALL_ONES {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

/* Prototypes  */
extern _acl_link_t      *_acl_first(_acl_control_t *control);
extern _acl_link_t      *_acl_next(_acl_control_t *control);
extern bcma_acl_rule_t  *_acl_rule_first(_acl_link_t *list_link);
extern bcma_acl_rule_t  *_acl_rule_next(_acl_link_t *list_link);
extern _acl_rule_link_t *_acl_rule_link_find(bcma_acl_rule_id_t rid);

extern int acl_range_to_list(uint16 min, uint16 max, acl_node_t **list,
                             int *count);
extern int acl_range_destroy(acl_node_t *list, int count);

#endif /* INCLUDE_ACL */

#endif /* _ACL_UTIL_H_ */
