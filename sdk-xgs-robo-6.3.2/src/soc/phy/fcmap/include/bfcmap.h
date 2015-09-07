/*
 * $Id: bfcmap.h 1.11 Broadcom SDK $
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

#ifndef BFCMAP_H
#define BFCMAP_H

#include <shared/fcmap.h>

/*
 * Initilaize the BFCMAP module.
 */
extern int bfcmap_init(void);

/*
 * Attach console/debug functions to BFCMAP module.
 */
typedef _shr_bfcmap_print_fn bfcmap_print_fn;

#define BFCMAP_PRINT_DEBUG      BPRINT_FN_DEBUG
#define BFCMAP_PRINT_PRINTF     BPRINT_FN_PRINTF

/*
 * The following function sets/gets the print function pointers for
 * BFCMAP module. If the flag to this function is Zero, no printf are
 * done. If the flag is BFCMAP_PRINT_DEBUG, it sets the print function
 * pointer for the BFCMAP driver and API module.
 * If the flag is BFCMAP_PRINT_PRINTF, it sets the function pointer
 * for all other non-debug printf.
 */
extern int 
bfcmap_config_print_set(buint32_t flag, bfcmap_print_fn print_cb_fn);

extern int 
bfcmap_config_print_get(buint32_t *flag, bfcmap_print_fn *print_fn_cb);


/*
 * Enum for traffic direction. 
 * Ingress traffic is defined as the one coming from the Line side.
 * Egress traffic is defined as coming from the switch/ASIC side.
 */
typedef enum {
        BFCMAP_DIR_EGRESS  = _SHR_BFCMAP_DIR_EGRESS ,
        BFCMAP_DIR_INGRESS = _SHR_BFCMAP_DIR_INGRESS
} bfcmap_dir_t;

/*
 * Identifiers for BFCMAP core type.
 */
typedef enum {
    BFCMAP_CORE_UNKNOWN = _SHR_BFCMAP_CORE_UNKNOWN,
    BFCMAP_CORE_OCTAL_GIG = _SHR_BFCMAP_CORE_OCTAL_GIG,
    BFCMAP_CORE_BCM5458X = _SHR_BFCMAP_CORE_BCM5458X,
    BFCMAP_CORE_BCM5458X_B0 = _SHR_BFCMAP_CORE_BCM5458X_B0,
    BFCMAP_CORE_BCM8729 = _SHR_BFCMAP_CORE_BCM8729,
    BFCMAP_CORE_BCM8483X = _SHR_BFCMAP_CORE_BCM8483X,
    BFCMAP_CORE_BCM5438X = _SHR_BFCMAP_CORE_BCM5438X,
    BFCMAP_CORE_BCM84756 = _SHR_BFCMAP_CORE_BCM84756,
    BFCMAP_CORE_BCM84756_C0 = _SHR_BFCMAP_CORE_BCM84756_C0
} bfcmap_core_t;




/* BFCMAP port is Enabled. Default is disabled. */
#define BFCMAP_PORT_ENABLE                               0x0000001

/* Single secure channel i.e. point-to-point configuration or LAN mode */
#define BFCMAP_PORT_SINGLE_CHANNEL                       0x0000002

/* Ingress Store-forward mode. Default is cut-through mode. 
 * In cut-through mode bfcmap errors are indicated to switch by corrupted CRC,
 * whereas in store-forward mode, error indication can be provided in special 
 * VLAN header. */
#define BFCMAP_PORT_INGRESS_STORE_FORWARD                0x0000004

/* Egress store-forward mode */
#define BFCMAP_PORT_EGRESS_STORE_FORWARD                 0x0000008



/* Port Mode */
typedef enum bfcmap_port_mode_s {
    BFCMAP_FCOE_TO_FC_MODE=_SHR_BFCMAP_FCOE_TO_FC_MODE,
    BFCMAP_FCOE_TO_FCOE_MODE=_SHR_BFCMAP_FCOE_TO_FCOE_MODE
} bfcmap_port_mode_t;

/* Port Speed */
typedef enum bfcmap_port_speed_s  {
    BFCMAP_PORT_SPEED_AN = _SHR_BFCMAP_PORT_SPEED_AN,
    BFCMAP_PORT_SPEED_2GBPS =_SHR_BFCMAP_PORT_SPEED_2GBPS,
    BFCMAP_PORT_SPEED_4GBPS =_SHR_BFCMAP_PORT_SPEED_4GBPS,
    BFCMAP_PORT_SPEED_8GBPS =_SHR_BFCMAP_PORT_SPEED_8GBPS,
    BFCMAP_PORT_SPEED_16GBPS = _SHR_BFCMAP_PORT_SPEED_16GBPS,
	BFCMAP_PORT_SPEED_AN_2GBPS =_SHR_BFCMAP_PORT_SPEED_AN_2GBPS,
	BFCMAP_PORT_SPEED_AN_4GBPS =_SHR_BFCMAP_PORT_SPEED_AN_4GBPS,
	BFCMAP_PORT_SPEED_AN_8GBPS =_SHR_BFCMAP_PORT_SPEED_AN_8GBPS,
	BFCMAP_PORT_SPEED_AN_16GBPS = _SHR_BFCMAP_PORT_SPEED_AN_16GBPS,
    BFCMAP_PORT_SPEED_MAX_COUNT=_SHR_BFCMAP_PORT_SPEED_MAX_COUNT
} bfcmap_port_speed_t;

/* Port State */
typedef enum bfcmap_port_state_s  {
    BFCMAP_PORT_STATE_INIT  = _SHR_BFCMAP_PORT_STATE_INIT,
    BFCMAP_PORT_STATE_RESET = _SHR_BFCMAP_PORT_STATE_RESET,
    BFCMAP_PORT_STATE_ACTIVE = _SHR_BFCMAP_PORT_STATE_ACTIVE,
    BFCMAP_PORT_STATE_LINKDOWN = _SHR_BFCMAP_PORT_STATE_LINKDOWN,
    BFCMAP_PORT_STATE_DISABLE = _SHR_BFCMAP_PORT_STATE_DISABLE,
    BFCMAP_PORT_STATE_MAX_COUNT = _SHR_BFCMAP_PORT_STATE_MAX_COUNT
} bfcmap_port_state_t;

typedef enum {
    BFCMAP_ENCAP_FCOE_FPMA             = 0,/* FPMA, use prefix and Node
                                             ID to construct FCoE MAC Address */
    BFCMAP_ENCAP_FCOE_ETH_ADDRESS_NULL = 1,/* Null address for FCOE MAC */
    BFCMAP_ENCAP_FCOE_ETH_ADDRESS_USER = 2/* User Provided Address */
} bfcmap_encap_mac_address_t;


typedef enum bfcmap_8g_fw_on_active_s {
    BFCMAP_8G_FW_ON_ACTIVE_ARBFF = _SHR_BFCMAP_8G_FW_ON_ACTIVE_ARBFF,
    BFCMAP_8G_FW_ON_ACTIVE_IDLE =  _SHR_BFCMAP_8G_FW_ON_ACTIVE_IDLE  
} bfcmap_8g_fw_on_active_t;


/*
 * BFCMAP port configuration structure.
 */
typedef _shr_bfcmap_port_config_t  bfcmap_port_config_t;

/*
 * Set BFCMAP Port configuration.
 */
extern int
bfcmap_port_config_set(bfcmap_port_t p, bfcmap_port_config_t *cfg);

/*
 * Get current BFCMAP Port configuration.
 */
extern int
bfcmap_port_config_get(bfcmap_port_t p, bfcmap_port_config_t *cfg);

/*
 * Prototype for Port interation callback function.
 */
typedef int (*bfcmap_port_traverse_cb)(bfcmap_port_t p, 
                                        bfcmap_core_t dev_core, 
                                        bfcmap_dev_addr_t dev_addr, 
                                        int dev_port, 
                                        bfcmap_dev_io_f devio_f, 
                                        void *user_data);

/*
 * Iterates over all the BFCMAP ports and calls user provided callback
 * function for each port.
 */
extern int bfcmap_port_traverse(bfcmap_port_traverse_cb callbk, 
                                void *user_data);

/*
 * BFCMAP VLAN - VSAN Mapping
 */
typedef _shr_bfcmap_vlan_vsan_map_t  bfcmap_vlan_vsan_map_t;

/*
 * Add entry into the VLAN to VSAN table for BFCMAP port
 */
extern int
bfcmap_vlan_map_add(bfcmap_port_t p, bfcmap_vlan_vsan_map_t *vlan);

/*
 * Get entry from the VLAN to VSAN table for BFCMAP port
 */
extern int
bfcmap_vlan_map_get(bfcmap_port_t p, bfcmap_vlan_vsan_map_t *vlan);

/*
 * Delete entry from the VLAN to VSAN table for BFCMAP port
 */
extern int
bfcmap_vlan_map_delete(bfcmap_port_t p, bfcmap_vlan_vsan_map_t *vlan);


/*
 * BFCMAP Events that are triggered by the BFCMAP driver.
 */
typedef enum {
    BFCMAP_EVENT_FC_LINK_INIT = _SHR_BFCMAP_EVENT_FC_LINK_INIT,
    BFCMAP_EVENT_FC_LINK_RESET = _SHR_BFCMAP_EVENT_FC_LINK_RESET,
    BFCMAP_EVENT_FC_LINK_DOWN = _SHR_BFCMAP_EVENT_FC_LINK_DOWN,
    BFCMAP_EVENT_FC_R_T_TIMEOUT = _SHR_BFCMAP_EVENT_FC_R_T_TIMEOUT,
    BFCMAP_EVENT_FC_E_D_TIMEOUT = _SHR_BFCMAP_EVENT_FC_E_D_TIMEOUT,

    BFCMAP_EVENT__COUNT = _SHR_BFCMAP_EVENT__COUNT
} bfcmap_event_t;

/*
 * BFCMAP event handler entry function. This function is/should be called
 * by the user either periodically or when when an interrupt is triggered.
 * This function will collect the pending events reported by the HW and call
 * the user provided callbacks for vaious events.
 */
int bfcmap_event_handler(bfcmap_dev_addr_t dev_addr);

/*
 * Prototype for the event callback function.
 */
typedef int (*bfcmap_event_cb_fn)(bfcmap_port_t p,      /* Port ID          */
                                  bfcmap_event_t event, /* Event            */
                                  void *user_data);         /* secure assoc ID  */

/*
 * Register user event handler callback function.
 */
extern int 
bfcmap_event_register(bfcmap_event_cb_fn cb, void *user_data);
                               
/*
 * Unregister user event handler callabck function.
 */
extern int 
bfcmap_event_unregister(bfcmap_event_cb_fn cb);

/*
 * Enables/Diable handling of the BFCMAP event.
 */
extern int bfcmap_event_enable_set(bfcmap_event_t event, int enable);

/*
 * Return the current event hanler status for specified event.
 */
extern int bfcmap_event_enable_get(bfcmap_event_t event, int *enable);

/*
 * The following enums define the stats/couter types.
 */
typedef enum {
        fc_rxdebug0=_shr_fc_rxdebug0,
        fc_rxdebug1=_shr_fc_rxdebug1,
        fc_rxunicastpkts=_shr_fc_rxunicastpkts,
        fc_rxgoodframes=_shr_fc_rxgoodframes,
        fc_rxbcastpkts=_shr_fc_rxbcastpkts,
        fc_rxbbcredit0=_shr_fc_rxbbcredit0,
        fc_rxinvalidcrc=_shr_fc_rxinvalidcrc,
        fc_rxframetoolong=_shr_fc_rxframetoolong,
        fc_rxtruncframes=_shr_fc_rxtruncframes,
        fc_rxdelimitererr=_shr_fc_rxdelimitererr,
        fc_rxothererr=_shr_fc_rxothererr,
        fc_rxruntframes=_shr_fc_rxruntframes,
        fc_rxlipcount=_shr_fc_rxlipcount,
        fc_rxnoscount=_shr_fc_rxnoscount,
        fc_rxerrframes=_shr_fc_rxerrframes,
        fc_rxdropframes=_shr_fc_rxdropframes,
        fc_rxlinkfail=_shr_fc_rxlinkfail,
        fc_rxlosssync=_shr_fc_rxlosssync,
        fc_rxlosssig=_shr_fc_rxlosssig,
        fc_rxprimseqerr=_shr_fc_rxprimseqerr,
        fc_rxinvalidword=_shr_fc_rxinvalidword,
        fc_rxinvalidset=_shr_fc_rxinvalidset,
        fc_rxencodedisparity=_shr_fc_rxencodedisparity,
        fc_rxbyt=_shr_fc_rxbyt,
        fc_txdebug0=_shr_fc_txdebug0,
        fc_txdebug1=_shr_fc_txdebug1,
        fc_txunicastpkts=_shr_fc_txunicastpkts,
        fc_txbcastpkts=_shr_fc_txbcastpkts,
        fc_txbbcredit0=_shr_fc_txbbcredit0,
        fc_txgoodframes=_shr_fc_txgoodframes,
        fc_txfifounderrun=_shr_fc_txfifounderrun,
        fc_txdropframes=_shr_fc_txdropframes,
        fc_txbyt=_shr_fc_txbyt,

	bfcmap_stat__count = _shr_bfcmap_stat__count
} bfcmap_stat_t;

/*
 * Crear all the stats for the specified port.
 */
extern int bfcmap_stat_clear(bfcmap_port_t p);

/*
 * Return the current statistics for the specified counter.
 */
extern int bfcmap_stat_get(bfcmap_port_t p, bfcmap_stat_t stat, buint64_t *val);

/*
 * Return the current statistics for the specified counter.
 */
extern int bfcmap_stat_get32(bfcmap_port_t p, bfcmap_stat_t stat,
                             buint32_t *val);

/*
 * BFCMAP Port create.
 * This API creates a BFCMAP port and binds the bfcmap_port_t
 * to particular bfcmap device/port.
 */
extern int 
bfcmap_port_create(bfcmap_port_t p,             /* port identifier    */
                    bfcmap_core_t dev_core,      /* device core type   */
                    bfcmap_dev_addr_t dev_addr,  /* device address     */
                    bfcmap_dev_addr_t uc_dev_addr,  /* uc device address */
                    int dev_port,                 /* device port index  */
                    bfcmap_dev_io_f devio_f);    /* device I/O callback*/

/*
 * BFCMAP Port destroy.
 */
extern int bfcmap_port_destroy(bfcmap_port_t p);

/*
 * BFCMAP Port reset.
 * The FC link reset protocol perform LINK reset when protocol errors
 * are detected. The FC-2 protocol errors,
 * e.g. Receive frame when no B2B credits exist,
 *  timeout conditions need FC Link reset. 
 */
extern int bfcmap_port_reset(bfcmap_port_t p);

/*
 * BFCMAP Port shutdown.
 * Initiates FC link Online to Offline protocol on the FC port
 */
extern int bfcmap_port_shutdown(bfcmap_port_t p);

/*
 * BFCMAP Port Enable.
 * Enables FC link to Online from Offline
 */
extern int bfcmap_port_link_enable(bfcmap_port_t p);

/*
 * BFCMAP Port Bounce link.
 * Bounce FC link 
 */
extern int bfcmap_port_bounce(bfcmap_port_t p);

/*
 * BFCMAP Port Speed.
 * Sets FC link speed
 */
extern int bfcmap_port_speed_set(bfcmap_port_t p, bfcmap_port_speed_t speed);



#endif /* BFCMAP_H */

