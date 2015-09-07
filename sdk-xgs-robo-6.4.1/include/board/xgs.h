/* 
 * $Id: xgs.h,v 1.1 Broadcom SDK $
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
 * File:        xgs.h
 * Purpose:     Public XGS board driver delarations
 */

#ifndef   _XGS_H_
#define   _XGS_H_

/* High level XGS board support */

#include <board/board.h>

extern char *board_xgs_description(board_driver_t *driver);
extern int board_xgs_probe(board_driver_t *driver, int num,
                                  bcm_info_t *info);
extern int board_xgs_start(board_driver_t *driver, uint32 flags);
extern int board_xgs_stop(board_driver_t *driver);
extern int board_xgs_modid_get(board_driver_t *driver,
                                      char *name, int *modid);
extern int board_xgs_modid_set(board_driver_t *driver,
                                      char *name, int modid);
extern int board_xgs_num_modid_get(board_driver_t *driver,
                                          char *name, int *num_modid);

#define BOARD_DRIVER_ANY_DEVICE 0xffffffff

#define BOARD_DRIVER_PRIVATE_DATA(driver) \
  ((void *)& _ ## driver ## _private_data)

#define BOARD_DRIVER_STRUCTURE(driver) driver ## _board

/* Cached device data */
typedef struct board_xgs_device_data_s {
    bcm_info_t info;
    int modid;
    int num_modid;
    int modid_mask;
} board_xgs_device_data_t;

typedef struct board_xgs_data_s {
    /* declaration
       populated at compile time
       do not reorder */
    int num_device; /* num_unit? */
    int num_supported_device;
    uint32 *supported_device;
    int num_led_program;
    uint8 *led_program;
    int led_port_offset; /* this is LED program dependent */
    int num_internal_connection;
    board_connection_t *internal_connection;

    /* instantiation
       populated at run time
       no ordering requirements */
    char *description;
    board_xgs_device_data_t *device; /* size: num_devices */
    bcm_gport_t cpu;
} board_xgs_data_t;

#define ARRAY(a) COUNTOF(a), a

#define BOARD_XGS_DRIVER(driver) \
board_driver_t BOARD_DRIVER_STRUCTURE(driver) = { \
  #driver, \
  BOARD_DEFAULT_PRIORITY, \
  BOARD_DRIVER_PRIVATE_DATA(driver),\
  0,\
  NULL,\
  0,\
  NULL,\
  board_xgs_description,\
  board_xgs_probe,\
  board_xgs_start,\
  board_xgs_stop \
}


#endif /* _XGS_H_ */
