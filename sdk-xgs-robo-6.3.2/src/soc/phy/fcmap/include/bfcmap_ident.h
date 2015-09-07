/*
 * $Id: bfcmap_ident.h 1.3 Broadcom SDK $
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
 */

#ifndef BFCMAP_IDENT_H
#define BFCMAP_IDENT_H

struct bfcmap_chip_info_s;
struct bfcmap_port_ctrl_s;
struct bfcmap_dev_ctrl_s;
struct bfcmap_api_table_s;
struct bfcmap_counter_info_s;
struct bfcmap_counter_cb_s;

typedef struct bfcmap_drv_tbl_entry_s {
        char                        *name;
        bfcmap_core_t              id;
        int                         num_port;
        struct bfcmap_api_table_s   *apis;
} bfcmap_drv_tbl_entry_t;
 
/* Returns the bfcmap driver entry corresponding to device core dev_core */
bfcmap_drv_tbl_entry_t* _bfcmap_find_driver(bfcmap_core_t dev_core);

extern int _bfcmap_get_device(bfcmap_dev_addr_t dev_addr);
/*
 * Return mdev for device corresponding to its address.
 */
#define BFCMAP_GET_DEVICE_FROM_DEVADDR(a)  _bfcmap_get_device((a))

/*
 * Adds mapping between bfcmap_port_t and bfcmap_port_ctrl_t */
extern int _bfcmap_add_port(bfcmap_port_t p, struct bfcmap_port_ctrl_s *mpc);

#define BFCMAP_ADD_PORT(p, mpc)      _bfcmap_add_port((p), (mpc))

extern int _bfcmap_remove_port(bfcmap_port_t p);

#define BFCMAP_REMOVE_PORT(p)      _bfcmap_remove_port((p))

/*
 * Return bfcmap_port_ctrl_t corresponding to port.
 */
extern struct bfcmap_port_ctrl_s * _bfcmap_get_port_ctrl(bfcmap_port_t p);

#define BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)  \
                                    _bfcmap_get_port_ctrl((p))

/*
 * Return bfcmap_port_t corresponding to bfcmap_port_ctrl_t.
 */
extern bfcmap_port_t _bfcmap_get_port_id(struct bfcmap_port_ctrl_s *mpc);

#define BFCMAP_GET_PORT_ID(mpc)  (mpc)->portId

#endif /* BFCMAP_IDENT_H */

