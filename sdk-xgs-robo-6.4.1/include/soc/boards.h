/*
 * $Id: boards.h,v 1.2 Broadcom SDK $
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
 * Purpose:     Definitions for the board type.
 */

#ifndef   _SOC_BOARD_H_
#define   _SOC_BOARD_H_

#include <sal/types.h>

#include <soc/defs.h>
#include <soc/drv.h>

/*
 * SOC_BOARD_CXN_MAX:  This is the maximum connections allowed on
 * one board interconnecting units.  We count each connection twice
 * (to support simplex connections and make other things easier.)
 * This is based on Hercules w/ 8 ports.  Draco could conceivably
 * connect to 13 devices on one board......
 *
 * SOC_BOARD_DEV_MAX:  Max number of SOC units controlled by one
 * CPU.  Same as SOC_ROBO_NUM_DEVICES.
 */

#define SOC_BOARD_CXN_MAX      16
#define SOC_BOARD_DEV_MAX      SOC_ROBO_NUM_DEVICES

/* Enumerated description of known boards */
typedef enum soc_board_ids_e {
    SOC_BOARD_INVALID,
    SOC_BOARD_5338_P24,
    SOC_BOARD_5338_B0_P24,
    SOC_BOARD_5338_P32,
    SOC_BOARD_5324_P4,    
    SOC_BOARD_5324_A1_P4,    
    SOC_BOARD_5380_P24,    
    SOC_BOARD_5325_P24,
    SOC_BOARD_NUM_BOARDS     /* Always last, please */
} soc_board_ids_t;

/*
 * Typedef:  soc_board_cxn_t
 * Indicate an on board connection with the following data type:
 *    src_port:          Port on unit                       
 *    tx_unit, tx_port:  Where does TX of src_port go?      
 *    rx_unit, rx_port:  Where does RX of src_port come from? 
 *    reachable:         Which units on this board are
 *                       reachable thru tx.  This is a BITMAP
 *                       indexed by SOC unit number.
 */

typedef struct soc_board_cxn_s {
    int                  src_port; /* Port on unit */
    int                  tx_unit;  /* Where does TX of src_port go? */
    int                  tx_port;
    int                  rx_unit;  /* Where does RX of src_port come from? */
    int                  rx_port;
    uint32               reachable; /* Which units are reachable thru tx */
} soc_board_cxn_t;

/*
 * Typedef:  soc_unit_info_t
 * Describe a unit and its connections on a board.
 *    dev_id:           Id value from devids.h
 *    num_cxns:          How many stack connections to
 *                       other chips on this board.
 *    cxn:               List of connections.  See above.
 */
typedef struct soc_unit_static_s {
    int                  dev_id;
    soc_driver_t         *soc_driver;
    int                  max_port;   /* Max physical port number */
} soc_unit_static_t;

typedef struct soc_unit_info_s {
    soc_unit_static_t    *static_info;
    int                  num_cxns;
    soc_board_cxn_t      cxn[SOC_BOARD_CXN_MAX];
} soc_unit_info_t;

/*
 * Typedef:  soc_board_t
 *    Describes a board
 *    board_id:          Value from above enum
 *    num_units:         How many devices on this board
 *    unit_info:         Array of unit descripts from above.
 *    max_vlans:         How many VLANs does the board support
 *    max_stg:           How many Spanning Tree Groups does it spt
 *    max_trunks:        How many trunks does the board support
 *    max_ports_per_trunk:   How many ports can be trunked together
 */

typedef struct soc_board_s {
    soc_board_ids_t      board_id;
    int                  num_units;
    soc_unit_info_t      unit_info[SOC_BOARD_DEV_MAX];
    /* Some parameters related to capabilities */
    int                  max_vlans;
    int                  max_stg;
    int                  max_trunks;
    int                  max_ports_per_trunk;
    char                 *board_name;
} soc_board_t;

/* Variable:
 *     known_boards:   The list of descriptions of known boards
 */

extern soc_board_t soc_known_boards[SOC_BOARD_NUM_BOARDS];

#endif /* _SOC_BOARD_H_ */
