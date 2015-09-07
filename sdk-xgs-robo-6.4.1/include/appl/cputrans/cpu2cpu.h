/*
 * $Id: cpu2cpu.h,v 1.9 Broadcom SDK $
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
 * File:        cpu2cpu.h
 * Purpose:     
 */

#ifndef   _CPUTRANS_CPU2CPU_H_
#define   _CPUTRANS_CPU2CPU_H_

#include <sdk_config.h>
#include <shared/idents.h>

#include <bcm/types.h>
#include <bcm/pkt.h>
#include <bcm/tx.h>
#include <bcm/rx.h>

#include <appl/cpudb/cpudb.h>



/* Default values for various parameters */
#ifndef C2C_VLAN_DEFAULT
#define C2C_VLAN_DEFAULT               1
#endif
#ifndef C2C_SNAP_MAC_DEFAULT
#define C2C_SNAP_MAC_DEFAULT           SHARED_BRCM_NTSW_SNAP_MAC
#endif
#ifndef C2C_SNAP_TYPE_DEFAULT
#define C2C_SNAP_TYPE_DEFAULT          SHARED_BRCM_NTSW_SNAP_TYPE
#endif
#define C2C_TRANS_TYPE_DEFAULT         SHARED_PKT_TYPE_C2C
#ifndef C2C_TRANS_PTR_DEFAULT
#define C2C_TRANS_PTR_DEFAULT          (&bcm_trans_ptr)
#endif
#ifndef C2C_SRC_MODID_DEFAULT
#define C2C_SRC_MODID_DEFAULT          -1
#endif

/* Configuration flags:
 *
 *     DEST_MAC_CHECK_ENABLE
 *          Normally, when a packet is checked, the destination
 *          MAC address is ignored.  It can be checked against
 *          the c2c mac address by setting this flag in c2c_config_set.
 */
#ifndef C2C_F_DEST_MAC_CHECK_ENABLE
#define C2C_F_DEST_MAC_CHECK_ENABLE 0x1
#endif

typedef void (*c2c_cb_f)(uint8 *pkt, void *cookie);

/****************************************************************
 *
 * C2C driver function type declarations
 *
 *     c2c_tx_f      Transmit a bcm_pkt_t pkt; same prototype as bcm_tx
 *     c2c_alloc_f   Allocate a packet buffer
 *     c2c_free_f    Free a packet buffer
 *
 ****************************************************************/

typedef int (*c2c_tx_f)(int unit, bcm_pkt_t *pkt, void *cookie);
typedef uint8 *(*c2c_alloc_f)(int len);
typedef void (*c2c_free_f)(uint8 *);

/****************************************************************
 *
 * Programming requirement:
 *     Must call c2c_config_set before calling any transmit
 *     function.  Pass in TX function (like bcm_tx), allocate
 *     and free functions for packet data.
 *
 ****************************************************************/

extern int c2c_snap_set(bcm_mac_t new_mac, uint16 new_type,
                        uint16 new_trans_type);
extern int c2c_snap_get(bcm_mac_t snap_mac, uint16 *new_type,
                        uint16 *trans_type);

/* Two alternatives for sending:
 *     c2c_tx:  One shot set and tear down
 *
 *     c2c_pkt_create, [c2c_pkt_update], c2c_pkt_send, c2c_pkt_destroy:
 *          Allows the same packet to be sent multiple times before
 *          tear down.
 */

extern int            c2c_tx         (cpudb_key_t dest_key,
                                      uint8 *pkt_buf,
                                      int len,
                                      int cos,
                                      int vlan,
                                      int seg_len,
                                      uint16 mplx_num,
                                      uint32 ct_flags,
                                      c2c_cb_f callback,
                                      void *cookie);

extern bcm_pkt_t *   c2c_pkt_create  (cpudb_key_t dest_key,
                                      uint8 *pkt_buf,
                                      int len,
                                      int cos,
                                      int vlan,
                                      int seg_len,
                                      uint16 mplx_num,
                                      uint32 ct_flags,
                                      int *tot_segs,
                                      int *rvp);

extern int           c2c_pkt_send    (bcm_pkt_t *pkt,
                                      c2c_cb_f callback,
                                      void *cookie);

extern int           c2c_pkt_update  (bcm_pkt_t *pkt,
                                      cpudb_key_t dest_key);

extern void          c2c_pkt_destroy (bcm_pkt_t *pkt);

extern int           c2c_pkt_recognize(uint8 *pkt_data,
                                       cpudb_key_t *src_key,
                                       uint16 *mplx_num);

extern int c2c_config_get(bcm_trans_ptr_t **trans_ptr, 
			  bcm_mac_t local_mac, 
			  uint32 *flags); 

extern int c2c_config_set(bcm_trans_ptr_t *trans_ptr,
			  bcm_mac_t local_mac,
			  uint32 flags); 

extern int c2c_cpu_add(const cpudb_entry_t *db_ent);
extern int c2c_cpu_remove(const cpudb_key_t key);
extern int c2c_cpu_clear(void);
extern int c2c_cpu_update(cpudb_ref_t db_ref);
extern int c2c_src_mod_set(int src_mod);
extern int c2c_src_mod_get(int *src_mod);

#endif /* _CPUTRANS_CPU2CPU_H_ */
