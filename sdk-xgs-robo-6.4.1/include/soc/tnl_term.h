/*
 * $Id: tnl_term.h,v 1.9 Broadcom SDK $
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
 */

#ifndef _TUNNEL_TERMINATION_H_
#define _TUNNEL_TERMINATION_H_
#include <shared/l3.h>

#define SOC_TNL_TERM_IPV4_ENTRY_WIDTH  (1) /* 1 table entry required     */
                                           /* for IPv4 tunnel terminator.*/
#define SOC_TNL_TERM_IPV6_ENTRY_WIDTH  (4) /* 4 table entries required   */
                                           /* for IPv6 tunnel terminator.*/

#define SOC_TNL_TERM_IDX_TO_BLOCK_START(_idx_, _block_start_, _block_size_)    \
       (_block_start_) = (_idx_) - ((_idx_) % _block_size_); 

/*
 * Tunnel termination entry structure. 
 * contains up to SOC_TNL_TERM_ENTRY_WIDTH_MAX L3_TUNNEL table entries
 */
typedef struct soc_tunnel_term_s {
    tunnel_entry_t entry_arr[SOC_TNL_TERM_IPV6_ENTRY_WIDTH];
} soc_tunnel_term_t;


/* Tunnel terminator hash key. */
typedef union soc_tnl_term_hash_key_u {
    /* IPv4/IPv6 Key fields */
    struct {
        uint8 dip[_SHR_L3_IP6_ADDRLEN];     /* Destination ip address. */
        uint8 sip[_SHR_L3_IP6_ADDRLEN];     /* Source ip address.      */
        uint8 sip_plen;                     /* Source ip prefix length.*/
        uint16 l4_src_port;                 /* L4 source port.         */
        uint16 l4_dst_port;                 /* L4 destination port.    */
        uint16 ip_protocol;                 /* Ip protocol.            */
    } ip_hash_key;

    /* MIM Key Fields */
    struct {
        uint16 sglp;                        /* MIM: SGLP               */
        uint16 bvid;                        /* MIM: BVlan              */
        sal_mac_addr_t bmacsa;              /* MIM: Bmacsa             */
    } mim_hash_key;

    /* MPLS Key Fields */
    struct {
        uint32 mpls_label;                  /* MPLS: Mpls_label        */
        uint16 module_id;                   /* MPLS: Module_id         */
        uint16 port;                        /* MPLS: Port              */
        uint16 trunk_id;                    /* MPLS: Trunk id          */
    } mpls_hash_key;
} soc_tnl_term_hash_key_t;


/*
 * Tunnel termination hash table. 
 */
typedef struct soc_tnl_term_hash_s {
    int         unit;
    int         entry_count;    /* Number entries in hash table */
    int         index_count;    /* Hash index max value + 1     */
    uint16      *table;         /* Hash table with 16 bit index */
    uint16      *link_table;    /* To handle collisions         */
} soc_tnl_term_hash_t;

/* Key bit width for CRC calculation. (words * 32) */
#define SOC_TNL_TERM_HASH_BIT_WIDTH    ((sizeof(soc_tnl_term_hash_key_t)) << 3)

/* 
 * Tunnel termination type tcam indexes. 
 */
typedef struct soc_tnl_term_state_s {
    int start;  /* start index for this entry priority. */
    int end;    /* End index for this entry priority. */
    int prev;   /* Previous (Lo to Hi) priority with non zero entry count. */
    int next;   /* Next (Hi to Lo) priority with non zero entry count. */
    int vent;   /* Number of valid entries. */
    int fent;   /* Number of free entries. */
} soc_tnl_term_state_t, *soc_tnl_term_state_p;

extern int soc_tunnel_term_init(int unit);
extern int soc_tunnel_term_deinit(int unit);
extern int soc_tunnel_term_insert(int unit, soc_tunnel_term_t *entry, uint32 *index);
extern int soc_tunnel_term_delete(int unit, soc_tunnel_term_t *key);
extern int soc_tunnel_term_delete_all(int unit);
extern int soc_tunnel_term_match(int unit, soc_tunnel_term_t *key,
                                   soc_tunnel_term_t *result);
extern void soc_tunnel_term_sw_dump(int unit);
extern int  soc_tunnel_term_used_get(int unit);

#if defined(BCM_WARM_BOOT_SUPPORT)
extern int soc_tunnel_term_reinit(int unit);
#endif /*  BCM_WARM_BOOT_SUPPORT */

#endif	/* !_TUNNEL_TERMINATION_H_ */
