/* 
 * $Id: board.h,v 1.1 Broadcom SDK $
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
 * File:        board.h
 * Purpose:     Board driver interface
 */

#ifndef   _BOARD_H_
#define   _BOARD_H_

#if defined(INCLUDE_BOARD)

#include <bcm/types.h>
#include <bcm/init.h>

#define BOARD_LOWEST_PRIORITY   0
#define BOARD_GENERIC_PRIORITY 25
#define BOARD_DEFAULT_PRIORITY 50

#define BOARD_START_F_COLD_BOOT 0x00000000
#define BOARD_START_F_WARM_BOOT 0x00000001
#define BOARD_START_F_CLEAR     0x00000002

/*
 * Board driver interface
 */

typedef struct board_connection_s {
    bcm_gport_t from;
    bcm_gport_t to;
} board_connection_t;

struct board_driver_s; /* forward reference */

typedef int (*board_attribute_get_f)(struct board_driver_s *driver,
                                     char *name, int *value);
typedef int (*board_attribute_set_f)(struct board_driver_s *driver,
                                     char *name, int value);

typedef struct board_attribute_s {
    char                   *name;
    char                   *description;
    char                   *units;
    char                   *format;
    board_attribute_get_f  get;
    board_attribute_set_f  set;
} board_attribute_t;

typedef char *(*board_desc_f)(struct board_driver_s *driver);
typedef int (*board_probe_f)(struct board_driver_s *driver,
                             int num, bcm_info_t *info);
typedef int (*board_start_f)(struct board_driver_s *driver, uint32 flags);
typedef int (*board_stop_f)(struct board_driver_s *driver);

typedef struct board_driver_s {
    char                   *name;
    int                    priority;
    void                   *user_data;
    int                    num_connection;
    board_connection_t     *connection;
    int                    num_attribute;
    board_attribute_t      *attribute;
    board_desc_f           description;
    board_probe_f          probe;
    board_start_f          start;
    board_stop_f           stop;
} board_driver_t;

/* Board Manager API */
extern int board_init(void);
extern int board_deinit(void);

extern int board_driver_add(board_driver_t *driver);
extern int board_driver_delete(board_driver_t *driver);
extern int board_driver_builtins_add(void);
extern int board_driver_builtins_delete(void);
extern int board_driver_builtins_get(int max_num,
                                     board_driver_t **driver,
                                     int *actual);

extern int board_drivers_get(int max_num,
                             board_driver_t **driver,
                             int *actual);

extern char *board_probe(void);
extern int board_find(char *name);
extern int board_attach(char *name);
extern int board_detach(void);

/* Board System API */
extern char *board_name(void);
extern char *board_description(void);
extern int board_start(uint32 start_flags);
extern int board_stop(void);
extern int board_attribute_get(char *attrib, int *value);
extern int board_attribute_set(char *attrib, int value);
extern int board_attributes_get(int max_num, board_attribute_t *attrib,
                                int *actual);
extern int board_connections_get(int max_num,
                                 board_connection_t *connection,
                                 int *actual);

#endif /* INCLUDE_BOARD */

#endif /* _BOARD_H_ */
