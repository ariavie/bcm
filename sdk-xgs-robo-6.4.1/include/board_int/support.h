/* 
 * $Id: support.h,v 1.2 Broadcom SDK $
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
 * File:        board_int/support.h
 * Purpose:     board support internal library interfaces
 */

#ifndef   _BOARD_INT_SUPPORT_H_
#define   _BOARD_INT_SUPPORT_H_

#include <sal/core/alloc.h>
#include <bcm/types.h>
#include <bcm/init.h>
#include <bcm/trunk.h>

#include <board/board.h>
#include <board/support.h>

#include <bcm_int/common/trunk.h>

/* Board tree */

typedef struct board_tree_driver_s {
    int num_connection;
    board_connection_t *connection;
    void *user_data;
    int (*info)(int unit, int *modid, bcm_trunk_chip_info_t *ti,
                void *user_data);
    int (*edge)(int unit, bcm_trunk_chip_info_t *ti,
                 bcm_trunk_add_info_t *ta, void *user_data);
    int (*vertex)(int unit, int modid, bcm_gport_t port, void *user_data);
} board_tree_driver_t;

typedef int (*board_connection_sp_cb_t)(board_connection_t *conn,
                                        void *user_data);

extern int board_tree_connect(board_tree_driver_t *tree_driver);

extern int board_connection_stack_ports(int max_num,
                                        board_connection_t *connection,
                                        int *actual);
extern int board_connection_add_internal_stack_ports(int count,
                                           board_connection_t *connection);

extern int board_connection_connected_stack_port_traverse(int count,
                                               board_connection_t *connection,
                                               board_connection_sp_cb_t func,
                                               void *user_data);

/* LED */
extern int board_led_program_write(int unit, int len, uint8 *pgm);
extern int board_led_program_read(int unit, int maxlen,
                                  uint8 *pgm, int *actual);
extern int board_led_data_write(int unit, int len, uint8 *data);
extern int board_led_data_read(int unit, int maxlen, uint8 *data, int *actual);
extern int board_led_enable_set(int unit, int value);
extern int board_led_enable_get(int unit, int *value);
extern int board_led_status_get(int unit, int ctl, int *value);

extern int board_led_init(int unit, int offset, int len, uint8 *pgm);
extern int board_led_deinit(int unit);


extern int board_device_reset(int unit);
extern int board_device_info(int unit, bcm_info_t *info);
extern int board_num_devices(void);
extern int board_device_modid_max(int unit);

extern int board_connections_alloc(board_driver_t *driver,
                                   board_connection_t **connection,
                                   int *actual);

extern int board_connection_gport_stackable(bcm_gport_t gport);

extern int board_probe_get(int max_num, bcm_info_t *info, int *actual);


/* Conveniences */
#define ALLOC(size) sal_alloc(size, (char *)FUNCTION_NAME())
#define FREE(ptr)   sal_free(ptr)


#endif /* _BOARD_INT_SUPPORT_H_ */
