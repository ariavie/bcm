/*
 * $Id: bfcmap_stat.h,v 1.1 Broadcom SDK $
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

#ifndef BFCMAP_STATS_H
#define BFCMAP_STATS_H

struct bfcmap_port_ctrl_s;
struct bfcmap_dev_ctrl_s;
struct bfcmap_api_table_s;
struct bfcmap_counter_info_s;
struct bfcmap_counter_cb_s;

typedef struct bfcmap_counter_info_s {
    bfcmap_stat_t    stat;
    buint32_t           address;
    buint16_t           delta;

#define BFCMAP_COUNTER_F_PORT           0x01
#define BFCMAP_COUNTER_F_SECURE_CHAN    0x02
#define BFCMAP_COUNTER_F_SECURE_ASSOC   0x04
#define BFCMAP_COUNTER_F_FLOW           0x08

#define BFCMAP_COUNTER_F_MEM            0x10
#define BFCMAP_COUNTER_F_REG            0x20

#define BFCMAP_COUNTER_F_SIZE32         0x100
#define BFCMAP_COUNTER_F_SIZE64         0x200

#define BFCMAP_COUNTER_F_SW             0x1000
    buint16_t  flags;
} bfcmap_counter_info_t;

#define BFCMAP_COUNTER_F_NOT_IMPLEMENTED    0x8000

/*
 * The following structure contains helper functions which are provided
 * by the chip implementation to common counter framework to comute the
 * correct counter address.
 */
typedef int (*counter_get_assoc_idx)(struct bfcmap_port_ctrl_s*, int assocId);
typedef int (*counter_get_chan_idx)(struct bfcmap_port_ctrl_s*, int chanId);

typedef struct bfcmap_counter_cb_s {
    int                     num_chan;
    counter_get_chan_idx    get_chan_idx;
    int                     num_assoc;
    counter_get_assoc_idx   get_assoc_idx;
    int      (*hw_clear_port_counter)(struct bfcmap_port_ctrl_s*);
    int      (*sw_clear_port_counter)(struct bfcmap_port_ctrl_s*);
    int      (*sw_get_counter)(struct bfcmap_port_ctrl_s*, bfcmap_counter_info_t*,
                               int, int, buint32_t*, buint64_t*);
    int      (*sw_set_counter)(struct bfcmap_port_ctrl_s*, bfcmap_counter_info_t*,
                               int, int, buint32_t*, buint64_t*);
} bfcmap_counter_cb_t;


extern int 
bfcmap_int_stat_clear(struct bfcmap_port_ctrl_s *mpc);

extern int 
bfcmap_int_stat_get(struct bfcmap_port_ctrl_s *mpc, bfcmap_stat_t stat, 
                        int chanId, int assocId, buint64_t *val);

extern int 
bfcmap_int_stat_get32(struct bfcmap_port_ctrl_s *mpc, bfcmap_stat_t stat, 
                          int chanId, int assocId, buint32_t *val);

extern int 
bfcmap_int_stat_set(struct bfcmap_port_ctrl_s *mpc, bfcmap_stat_t stat, 
                            int chanId, int assocId,  buint64_t val);

extern int 
bfcmap_int_stat_set32(struct bfcmap_port_ctrl_s *mpc,bfcmap_stat_t stat, 
                             int chanId, int assocId, buint32_t val);

extern bfcmap_counter_info_t *
bfcmap_int_stat_tbl_create(struct bfcmap_dev_ctrl_s*, 
                                 bfcmap_counter_info_t *stats_tbl);

#endif /* BFCMAP_STATS_H */

