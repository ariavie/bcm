/*
 * $Id: bfcmap.c 1.6 Broadcom SDK $
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

#include <bfcmap.h>
#include <bfcmap_int.h>


char *bfcmap_errmsg[] = BFCMAP_ERRMSG_INIT;

bfcmap_dev_ctrl_t   *bfcmap_unit_cb[BFCMAP_MAX_UNITS];
        
STATIC int  _bfcmap_init_done = 0;

STATIC bfcmap_sal_lock_t    bfcmap_lock;

bprint_fn _bfcprint_fn_func_cb;
buint32_t _bfcprint_fn_flag;


/*********************************************************************
 * Local functions
 *********************************************************************/

STATIC int _bfcmap_device_create(bfcmap_core_t dev_core,
                                  bfcmap_dev_addr_t dev_addr,
                                  bfcmap_dev_io_f devio_f,
                                  int *devNum);

STATIC int _bfcmap_device_destroy(bfcmap_dev_ctrl_t  *dev_ctrl) ;

/*
 * Initializes the BFCMAP module.
 */
int bfcmap_init(void)
{
    if (!_bfcmap_init_done) {
        /*
         * create bfcmap module lock.
         */
        bfcmap_lock = BFCMAP_SAL_LOCK_CREATE("bfcmap_lock");

        /*
         * Initialize vlan map manager.
         */
        bfcmap_int_vlanmap_init();

        _bfcmap_init_done = 1;
    }
    return BFCMAP_E_NONE;
}

/*
 * Set the print and debug print function pointers.
 */
int 
bfcmap_config_print_set(buint32_t flag, bfcmap_print_fn print_fn_cb)
{
    /* Update the print flag. */
    _bfcprint_fn_flag = flag;

    /* Update the print function pointer */
    _bfcprint_fn_func_cb = print_fn_cb;

    return BFCMAP_E_NONE;
}

int 
bfcmap_config_print_get(buint32_t *flag, bfcmap_print_fn *print_fn_cb)
{
    /* Update the print flag. */
    if (flag) {
        *flag = _bfcprint_fn_flag;
    }

    /* Update the print function pointer */
    if (print_fn_cb) {
        *print_fn_cb = _bfcprint_fn_func_cb;
    }

    return BFCMAP_E_NONE;
}


/**********************************************************
 * Public driver functions
 *********************************************************/

/*
 * Function:
 *      bfcmap_port_create
 * Purpose:
 *      Create a BFCMAP Port
 * Parameters:
 *      p       - bfcmap port id
 *      dev_core- Device type
 *      dev_addr- Device address
 *      uc_dev_addr- Device address for microcontroller access for port
 *      dev_port- Port index with the device
 *      devio_f - Device I/O callback 
 */
int
bfcmap_port_create(bfcmap_port_t p, bfcmap_core_t dev_core,
                    bfcmap_dev_addr_t dev_addr,
                    bfcmap_dev_addr_t uc_dev_addr, int dev_port,
                    bfcmap_dev_io_f devio_f)
{
    bfcmap_dev_ctrl_t  *dev_ctrl;
    bfcmap_port_ctrl_t *port_ctrl;
    int                 mdev, new_device = 0, rv = BFCMAP_E_NONE;

    if (dev_port < 0) {
        BFCMAP_SAL_DBG_PRINTF("Error: Invalid port number (%d)\n", dev_port);
        return BFCMAP_E_PARAM;
    }
    
    if (!devio_f) {
        BFCMAP_SAL_DBG_PRINTF("Error: No BFCMAP IO callback specifed\n");
        return BFCMAP_E_PARAM;
    }
    
    if (BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) {
        BFCMAP_SAL_DBG_PRINTF("Error: Port (PortId=%d) already exists\n", p);
        return BFCMAP_E_PORT;
    }

    BFCMAP_SAL_DBG_PRINTF("Port create (dev_addr=%d,dev_port=%d)\n",
                           dev_addr, dev_port);

    /* Find the unit to which this port belongs */
    if ((mdev = BFCMAP_GET_DEVICE_FROM_DEVADDR(dev_addr)) < 0) {
        /* Find an unused mdev */
        if ((rv = _bfcmap_device_create(dev_core, dev_addr, 
                                        devio_f, &mdev)) < 0) {
            BFCMAP_SAL_DBG_PRINTF("Error: Port creation failed");
            return rv;
        }
        new_device = 1;
    }

    dev_ctrl = BFCMAP_DEVICE_CTRL(mdev);

    port_ctrl = BFCMAP_UNIT_PORT_CONTROL(dev_ctrl, dev_port);
    if (port_ctrl->f & BFCMAP_CTRL_FLAG_ATTACHED) {
        BFCMAP_SAL_DBG_PRINTF("Error: BFCMAP Port already attached");
        return BFCMAP_E_NONE;
    }

    BFCMAP_SAL_LOCK(bfcmap_lock);

    port_ctrl->f         |= BFCMAP_CTRL_FLAG_ATTACHED;
    port_ctrl->uc_dev_addr  = uc_dev_addr;

    /* Add mapping bfcmap_port_t and bfcmap_port_ctrl_t */
    BFCMAP_ADD_PORT(p, port_ctrl);

    /* 
     * Reset and intiialize the mdev.
     */
    if (new_device) {
        rv = BFCMAP_DEVICE_INIT(dev_ctrl);
    }

    if (rv == BFCMAP_E_NONE) {
        rv = BFCMAP_PORT_INIT(port_ctrl, NULL);
    }

    BFCMAP_SAL_UNLOCK(bfcmap_lock);

    return rv;
}

/*
 * Function:
 *      bfcmap_port_destroy
 * Purpose:
 *      Destroy a BFCMAP Port
 * Parameters:
 *      p       - bfcmap port id
 */
int
bfcmap_port_destroy(bfcmap_port_t p)
{
    bfcmap_dev_ctrl_t  *dev_ctrl;
    bfcmap_port_ctrl_t       *port_ctrl;
    int                 ii, f;

    BFCMAP_SAL_LOCK(bfcmap_lock);

    port_ctrl = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p);
    if (!port_ctrl) {
        BFCMAP_SAL_UNLOCK(bfcmap_lock);
        BFCMAP_SAL_DBG_PRINTF("Error: BFCMAP Port not found\n");
        return BFCMAP_E_PORT;
    }
   

    /*
     * Free up any resources occupied by the port.
     */
    BFCMAP_PORT_UNINIT(port_ctrl);

    /*
     * Remove port mapping from internal tables.
     */
    BFCMAP_REMOVE_PORT(p);

    /* Un-attach the driver */
    port_ctrl->f      &= ~BFCMAP_CTRL_FLAG_ATTACHED;

    /* 
     * Free up mdev resources if none of the mports is attached.
     */
    dev_ctrl = port_ctrl->parent;
    f = 0;
    port_ctrl = BFCMAP_UNIT_PORT_CONTROL(dev_ctrl, 0);
    for (ii=0; ii<dev_ctrl->num_ports; ii++) {
        if (port_ctrl->f & BFCMAP_CTRL_FLAG_ATTACHED) {
            f = 1;
            break;
        }
        port_ctrl++;
    }

    if (!f) {
        BFCMAP_DEVICE_UNINIT(dev_ctrl);
        _bfcmap_device_destroy(dev_ctrl);
    }

    BFCMAP_SAL_UNLOCK(bfcmap_lock);

    return BFCMAP_E_NONE;
}

/*
 * Function:
 *      bfcmap_port_traverse
 * Purpose:
 *      Destroy a BFCMAP Port
 * Parameters:
 *      callback        - port traverse callback function
 *      user_data       - user callback data
 */
int 
bfcmap_port_traverse(bfcmap_port_traverse_cb callback, void *user_data)
{
    bfcmap_port_t      p;
    bfcmap_port_ctrl_t       *mpc;
    bfcmap_dev_ctrl_t  *mdc;
    int                 mdev, port;
    int                 rv = BFCMAP_E_NONE ;
    
    for (mdev = 0; mdev < BFCMAP_MAX_UNITS; mdev++) {
        mdc = BFCMAP_DEVICE_CTRL(mdev);
        if (mdc == NULL) {
            continue;
        }
        for (port = 0; port < BFCMAP_DEVICE_NUM_PORTS(mdc); port++) {
            mpc = BFCMAP_UNIT_PORT_CONTROL(mdc, port);
            BFCMAP_SAL_ASSERT(mpc);
            if ((mpc->f & BFCMAP_CTRL_FLAG_ATTACHED) == 0) {
                continue;
            }
            p = BFCMAP_GET_PORT_ID(mpc);
            rv = callback(p, mdc->core_type, mpc->dev_addr, port, 
                                        mpc->msec_io_f, user_data);
            if (rv < 0) {
                return rv;
            }
        }
    }
    return rv;
}

/*
 * Function:
 *      bfcmap_port_config_set
 * Purpose:
 *      Set the BFCMAP Port configuration.
 * Parameters:
 *      p       - BFCMAP port identifier
 *      cfg     - pointer to bfcmap configuration structure
 */
int
bfcmap_port_config_set(bfcmap_port_t p, bfcmap_port_config_t *cfg)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_CONFIG_SET(mpc, cfg);
}

/*
 * Function:
 *      bfcmap_port_config_get
 * Purpose:
 *      Get the current BFCMAP Port configuration.
 * Parameters:
 *      p       - BFCMAP port identifier
 *      cfg     - pointer to bfcmap configuration structure
 */
int
bfcmap_port_config_get(bfcmap_port_t p, bfcmap_port_config_t *cfg)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_CONFIG_GET(mpc, cfg);
}

#if 0
/*
 * Function:
 *      bfcmap_port_capability_get
 * Purpose:
 *      Get the current BFCMAP Port capability
 * Parameters:
 *      p       - BFCMAP port identifier
 *      cap     - pointer to bfcmap capability structure
 */
int
bfcmap_port_capability_get(bfcmap_port_t p, bfcmap_port_capability_t *cap)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_CAPABILITY_GET(mpc, cap);
}

#endif

/*
 * Function:
 *      bfcmap_port_reset
 * Purpose:
 *      Reset the BFCMAP Port 
 * Parameters:
 *      p       - BFCMAP port identifier
 */
int
bfcmap_port_reset(bfcmap_port_t p)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_RESET(mpc);
}

/*
 * Function:
 *      bfcmap_port_shutdown
 * Purpose:
 *      Shutdown the BFCMAP Port 
 * Parameters:
 *      p       - BFCMAP port identifier
 */
int
bfcmap_port_shutdown(bfcmap_port_t p)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_SHUTDOWN(mpc);
}

/*
 * Function:
 *      bfcmap_port_link_enable
 * Purpose:
 *      Enable the BFCMAP Port 
 * Parameters:
 *      p       - BFCMAP port identifier
 */
int
bfcmap_port_link_enable(bfcmap_port_t p)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_LINK_ENABLE(mpc);
}

/*
 * Function:
 *      bfcmap_port_speed
 * Purpose:
 *      Sets speed on the BFCMAP Port 
 * Parameters:
 *      p       - BFCMAP port identifier
 */
int
bfcmap_port_speed_set(bfcmap_port_t p, bfcmap_port_speed_t speed)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_SPEED_SET(mpc, speed);
}

/*
 * Function:
 *      bfcmap_port_bounce
 * Purpose:
 *      Bounce link on the BFCMAP Port 
 * Parameters:
 *      p       - BFCMAP port identifier
 */
int
bfcmap_port_bounce(bfcmap_port_t p)
{
    bfcmap_port_ctrl_t   *mpc;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_BOUNCE(mpc);
}

/*
 * Function:
 *      bfcmap_vlan_map_add
 * Purpose:
 *      Add entry into the VLAN to VSAN table for BFCMAP port
 * Parameters:
 *      p       - BFCMAP port identifier
 *      vlan    - pointer to entry in bfcmap VLAN - VSAN Map 
 */
int
bfcmap_vlan_map_add(bfcmap_port_t p, bfcmap_vlan_vsan_map_t *vlan)
{
    int rv = BFCMAP_E_NONE;
    bfcmap_port_ctrl_t   *mpc;
    bfcmap_int_vlanmap_entry_t *vlanmap_entry;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }

    if (mpc->mp == NULL) {
        return BFCMAP_E_UNAVAIL;
    }

    if (vlan == NULL || 
        !BFCMAP_MP_VLAN_VSAN_VALID(vlan->vlan_vid, vlan->vsan_vfid)) {
        return BFCMAP_E_PARAM;
    }

    BFCMAP_LOCK_PORT(mpc);

    vlanmap_entry = bfcmap_int_vlanmap_add_or_update(mpc, vlan);
    if (vlanmap_entry == NULL) {
        rv = BFCMAP_E_RESOURCE;
        goto done;
    }

    rv = bfcmap_int_vlanmap_add_to_hw(mpc, mpc->mp, vlanmap_entry);
    if (rv < 0) {
        bfcmap_int_vlanmap_delete(mpc, vlan->vlan_vid, vlan->vsan_vfid);
    }
done:
    BFCMAP_UNLOCK_PORT(mpc);

    return rv;
}

/*
 * Function:
 *      bfcmap_vlan_map_get
 * Purpose:
 *      Get entry from the VLAN to VSAN table for BFCMAP port
 * Parameters:
 *      p       - BFCMAP port identifier
 *      vlan    - pointer to entry in bfcmap VLAN - VSAN Map 
 */
int
bfcmap_vlan_map_get(bfcmap_port_t p, bfcmap_vlan_vsan_map_t *vlan)
{
    int rv = BFCMAP_E_NONE;
    bfcmap_port_ctrl_t   *mpc;
    bfcmap_int_vlanmap_entry_t *vlanmap_entry;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }

    if (vlan == NULL) {
        return BFCMAP_E_PARAM;
    }

    if (mpc->mp == NULL) {
        return BFCMAP_E_UNAVAIL;
    }

    BFCMAP_LOCK_PORT(mpc);

    if ((vlanmap_entry = bfcmap_int_vlanmap_get_next_or_first(mpc, mpc->mp,
            vlan->vlan_vid, vlan->vsan_vfid)) == NULL) {
        rv = BFCMAP_E_NOT_FOUND;
        goto done;
    }


    BFCMAP_SAL_ASSERT(vlanmap_entry->mapper); 
    BFCMAP_SAL_ASSERT(vlanmap_entry->mapper == mpc->mp); 

    BFCMAP_SAL_MEMCPY(vlan, &vlanmap_entry->vlanmap,
                      sizeof(bfcmap_vlan_vsan_map_t));

done:
    BFCMAP_UNLOCK_PORT(mpc);
    return rv;
}

/*
 * Function:
 *      bfcmap_vlan_map_delete
 * Purpose:
 *      Delete entry from the VLAN to VSAN table for BFCMAP port
 * Parameters:
 *      p       - BFCMAP port identifier
 *      vlan    - pointer to entry in bfcmap VLAN - VSAN Map 
 */
int
bfcmap_vlan_map_delete(bfcmap_port_t p, bfcmap_vlan_vsan_map_t *vlan)
{
    int rv = BFCMAP_E_NONE;
    bfcmap_port_ctrl_t   *mpc;
    bfcmap_int_vlanmap_entry_t *vlanmap_entry;
    bfcmap_int_hw_mapper_t      *mp;

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }

    if (vlan == NULL) {
        return BFCMAP_E_PARAM;
    }

    if ((vlanmap_entry = bfcmap_int_vlanmap_find_by_id(mpc, 
            vlan->vlan_vid, vlan->vsan_vfid)) == NULL) {
        return BFCMAP_E_NOT_FOUND;
    }

    mp = vlanmap_entry->mapper;

    BFCMAP_SAL_ASSERT(mp); 

    BFCMAP_LOCK_PORT(mpc);

    if (mp && (bfcmap_int_vlanmap_delete_from_hw(mpc, mp, vlanmap_entry) < 0)){
        rv = BFCMAP_E_INTERNAL;
        goto done;
    }

    bfcmap_int_vlanmap_delete(mpc, vlan->vlan_vid, vlan->vsan_vfid);

done:
    BFCMAP_UNLOCK_PORT(mpc);

    return rv;
}

/*
 * Function:
 *      bfcmap_stat_clear 
 * Purpose:
 *      Clear all Stats/counters for the specified port
 * Parameters:
 *      p       - BFCMAP port identifier
 */
int 
bfcmap_stat_clear(bfcmap_port_t p)
{
    bfcmap_port_ctrl_t   *mpc;
    
    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return bfcmap_int_stat_clear(mpc);
}

/*
 * Function:
 *      bfcmap_stat_get
 * Purpose:
 *      Return specified Stats/counters.
 * Parameters:
 *      p       - BFCMAP port identifier
 *      stat    - Counter idenfier
 *      val     - Pointer to buint64_t where counter is returned
 */
int 
bfcmap_stat_get(bfcmap_port_t p, bfcmap_stat_t stat, buint64_t *val)
{
    bfcmap_port_ctrl_t   *mpc;

    if (!val) {
        return BFCMAP_E_PARAM;
    }

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return bfcmap_int_stat_get(mpc, stat, -1, -1, val);
}

/*
 * Function:
 *      bfcmap_stat_get32
 * Purpose:
 *      Return specified Stats/counters.
 * Parameters:
 *      p       - BFCMAP port identifier
 *      stat    - Counter idenfier
 *      val     - Pointer to buint32_t where counter is returned
 */
int 
bfcmap_stat_get32(bfcmap_port_t p, bfcmap_stat_t stat, buint32_t *val)
{
    bfcmap_port_ctrl_t   *mpc;
    
    if (!val) {
        return BFCMAP_E_PARAM;
    }

    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return bfcmap_int_stat_get32(mpc, stat, -1, -1, val);
}



/*
 * Function:
 *      bfcmap_event_handler
 * Purpose:
 *      BFCMAP event (interrupt handler) function. This function
 *      services any pending events reported by the HW and calls 
 *      user registered event notification function.
 * Parameters:
 *      dev_addr - BFCMAP device address
 */
int 
bfcmap_event_handler(bfcmap_dev_addr_t dev_addr)
{
    int mdev;
    bfcmap_dev_ctrl_t *mdc;
    
    mdev = BFCMAP_GET_DEVICE_FROM_DEVADDR(dev_addr);

    if (mdev >= 0) {
        mdc = BFCMAP_DEVICE_CTRL(mdev);

        return BFCMAP_DEVICE_EVENT_HANDLER(mdc);
    } else {
        return BFCMAP_E_PARAM;
    }
}

typedef struct bfcmap_event_cb_info_s {
    bfcmap_event_cb_fn                 cb_f;
    void                                *user_data;
    struct bfcmap_event_cb_info_s      *next;
} bfcmap_event_cb_info_t;

STATIC bfcmap_event_cb_info_t *bfcmap_event_cb = NULL;

STATIC buint32_t _event_mask;

/*
 * Function:
 *      bfcmap_event_register
 * Purpose:
 *      Register application callback function for event handling.
 * Parameters:
 *      cb      - Callback function
 *      user_data - User data
 */
int 
bfcmap_event_register(bfcmap_event_cb_fn cb, void *user_data)
{
    bfcmap_event_cb_info_t *new_cb;

    new_cb = BFCMAP_SAL_MALLOC(sizeof(bfcmap_event_cb_info_t), 
                               "BFCMAP_event_info");

    if (new_cb == NULL) {
        return BFCMAP_E_MEMORY;
    }

    new_cb->cb_f = cb;
    new_cb->user_data = user_data;

    BFCMAP_SAL_LOCK(bfcmap_lock);

    new_cb->next = bfcmap_event_cb;
    bfcmap_event_cb = new_cb;

    BFCMAP_SAL_UNLOCK(bfcmap_lock);

    return BFCMAP_E_NONE;
}

/*
 * Function:
 *      bfcmap_event_unregister
 * Purpose:
 *      Unregister application event handling.
 * Parameters:
 *      cb      - Callback function
 */
int 
bfcmap_event_unregister(bfcmap_event_cb_fn cb)
{
    bfcmap_event_cb_info_t **cbInfo = &bfcmap_event_cb;
    bfcmap_event_cb_info_t *cb_tmp;

    BFCMAP_SAL_LOCK(bfcmap_lock);
    while (*cbInfo && ((*cbInfo)->cb_f != cb)) {
        cbInfo = &(*cbInfo)->next;
    }
    BFCMAP_SAL_UNLOCK(bfcmap_lock);

    if (*cbInfo == NULL) {
        /* Callback not registered, return error. */
        return BFCMAP_E_NOT_FOUND;
    }

    BFCMAP_SAL_LOCK(bfcmap_lock);
    cb_tmp = *cbInfo;
    *cbInfo = cb_tmp->next;
    BFCMAP_SAL_UNLOCK(bfcmap_lock);
   
    BFCMAP_SAL_FREE(cb_tmp);
    return BFCMAP_E_NONE;
}

/*
 * Function:
 *      bfcmap_event_enable_set
 * Purpose:
 *      Enable/Disable event handler for specified event
 * Parameters:
 *      event   - Event ID
 *      enable  - 1 to Enable or Zero to disable
 */
int
bfcmap_event_enable_set(bfcmap_event_t event, int enable)
{
    if (event >= BFCMAP_EVENT__COUNT) {
        return BFCMAP_E_PARAM;
    }

    if (enable) {
        _event_mask |= (1 << (int)event);
    } else {
        _event_mask &= ~(1 << (int)event);
    }
    return BFCMAP_E_NONE;
}

/*
 * Function:
 *      bfcmap_event_enable_get
 * Purpose:
 *      Return current enable/disable status for specified event.
 * Parameters:
 *      event   - Event ID
 *      enable  - 1 to Enable or Zero to disable
 */
int
bfcmap_event_enable_get(bfcmap_event_t event, int *enable)
{
    if (event >= BFCMAP_EVENT__COUNT) {
        return BFCMAP_E_PARAM;
    }

    *enable = (_event_mask & (1 << (int)event)) ? 1 : 0;
    return BFCMAP_E_NONE;
}

int
bfcmap_event(bfcmap_port_ctrl_t *mpc, bfcmap_event_t event, 
              int chanId, int assocId)
{
    int              rv = BFCMAP_E_NONE;
    bfcmap_port_t   p = BFCMAP_GET_PORT_ID(mpc);
    bfcmap_event_cb_info_t *cbInfo = bfcmap_event_cb;

    if (_event_mask & (1 << (int)event)) {
        while (cbInfo) {
            cbInfo->cb_f(p, event, cbInfo->user_data);
            cbInfo = cbInfo->next;
        }
    }
    return rv;
}

/******************* Diag related wrappers**********************/

#if 0

/*
 * Function:
 *      bfcmap_encrypt_op
 * Purpose:
 *      Performs encryption operation on the specified data
 * Parameters:
 *      p               - BFCMAP port identifier
 *      protected_data  - Pointer to data to be protected
 *      protected_dlen  - Length of protected_data
 *      confidential_data  - Pointer to data to be encrpted
 *      confidential_dlen  - Length of confidential_data
 *      out_data           - Pointer to data where out is returned
 *      out_len            - Length of the total output (includes ICV)
 */
int 
bfcmap_encrypt_op(bfcmap_port_t p, 
                   bfcmap_crypto_aes128_gcm_t *key,
                   buint8_t *protected_data, int protected_dlen,
                   buint8_t *confidential_data, int confidential_dlen,
                   buint8_t *out_data, int out_len)
{
    bfcmap_port_ctrl_t   *mpc;
    
    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_DIAG_CIPHER_OP_ENCRYPT(mpc, key, protected_data,
                        protected_dlen, confidential_data, confidential_dlen,
                        out_data, out_len);
}

/*
 * Function:
 *      bfcmap_decrypt_op
 * Purpose:
 *      Performs encryption operation on the specified data
 * Parameters:
 *      p               - BFCMAP port identifier
 *      protected_data  - Pointer to data to be protected
 *      protected_dlen  - Length of protected_data
 *      confidential_data  - Pointer to data to be encrpted
 *      confidential_dlen  - Length of confidential_data
 *      icv_data           - Pointer to ICV
 *      icv_len            - Length of ICV
 *      out_data           - Pointer to data where out is returned
 *      out_len            - Length of the total output (includes ICV)
 */
int 
bfcmap_decrypt_op(bfcmap_port_t p, 
                   bfcmap_crypto_aes128_gcm_t *key,
                   buint8_t *protected_data, int protected_dlen,
                   buint8_t *confidential_data, int confidential_dlen,
                   buint8_t *icvdata, int icvlen,
                   buint8_t *out_data, int out_len)
{
    bfcmap_port_ctrl_t   *mpc;
    
    if ((mpc = BFCMAP_GET_PORT_CONTROL_FROM_PORTID(p)) == NULL) {
        return BFCMAP_E_PORT;
    }
    return BFCMAP_PORT_DIAG_CIPHER_OP_DECRYPT(mpc, key, protected_data,
                        protected_dlen, confidential_data, confidential_dlen,
                        icvdata, icvlen, out_data, out_len);
}

#endif


/*******************************************************************
 * STATIC/local functions
 ******************************************************************/

STATIC int
_bfcmap_device_create(bfcmap_core_t dev_core,
                       bfcmap_dev_addr_t dev_addr,
                       bfcmap_dev_io_f devio_f, 
                       int *devNum)
{
    int                 mdev, ii;
    bfcmap_dev_ctrl_t  *dev_ctrl = NULL;
    bfcmap_port_ctrl_t       *port_ctrl, *bfcmap_port_ctrl_tbl;
    bfcmap_drv_tbl_entry_t     *drv_entry;

    if ((drv_entry = _bfcmap_find_driver(dev_core)) == NULL) {
        BFCMAP_SAL_DBG_PRINTF("Error: Unsupported BFCMAP device %d\n", 
                          dev_core);
        return BFCMAP_E_NOT_FOUND;
    }

    BFCMAP_SAL_LOCK(bfcmap_lock);
    for (mdev = 0; mdev < BFCMAP_MAX_UNITS; mdev++) {
        dev_ctrl = BFCMAP_DEVICE_CTRL(mdev);
        if (dev_ctrl == NULL) {
            if ((dev_ctrl = BFCMAP_SAL_MALLOC(
                  sizeof(bfcmap_dev_ctrl_t), "BFCMAP_DEVICE_CTRL")) == NULL) {

                BFCMAP_SAL_DBG_PRINTF("Memory allocation failed for \
                                  BFCMAP UNIT CB");

                BFCMAP_SAL_UNLOCK(bfcmap_lock);
                return BFCMAP_E_MEMORY;
            }
            break;
        }
    }

    if (mdev == BFCMAP_MAX_UNITS) {
        BFCMAP_SAL_UNLOCK(bfcmap_lock);
        return BFCMAP_E_MEMORY;
    }

    BFCMAP_SAL_MEMSET(dev_ctrl, 0, sizeof(bfcmap_dev_ctrl_t));
    dev_ctrl->mdev = mdev;
    dev_ctrl->dev_addr = dev_addr;
    dev_ctrl->num_ports = drv_entry->num_port;
    dev_ctrl->flags = BFCMAP_UNIT_ATTACHED;
    dev_ctrl->core_type = dev_core;

    /* Create device lock */
    dev_ctrl->ulock = BFCMAP_SAL_LOCK_CREATE("dev lock");

    if ((bfcmap_port_ctrl_tbl = BFCMAP_SAL_MALLOC(
                 (sizeof(bfcmap_port_ctrl_t)*drv_entry->num_port), 
                                "bfcmap_port_ctrl_tbl")) == NULL) {

        BFCMAP_SAL_FREE(dev_ctrl);
        BFCMAP_SAL_DBG_PRINTF("Failed to allocate memory for bfcmap_port_ctrl table");
        BFCMAP_SAL_UNLOCK(bfcmap_lock);
        return BFCMAP_E_MEMORY;
    }

    dev_ctrl->ports = bfcmap_port_ctrl_tbl;

    port_ctrl = &bfcmap_port_ctrl_tbl[0];
    for (ii = 0; ii < drv_entry->num_port; ii++) {
        BFCMAP_SAL_MEMSET(port_ctrl, 0, sizeof(bfcmap_port_ctrl_t));
        port_ctrl->parent = dev_ctrl;
        port_ctrl->api = drv_entry->apis;
        port_ctrl->mport      = ii;
        port_ctrl->mdev      = mdev;
        port_ctrl->dev_addr   = dev_addr;
        port_ctrl->msec_io_f  = devio_f;
        port_ctrl->plock = BFCMAP_SAL_LOCK_CREATE("bfcmap port lock");
        port_ctrl++;
    }
    BFCMAP_DEVICE_CTRL(mdev) = dev_ctrl;

    BFCMAP_SAL_UNLOCK(bfcmap_lock);

    *devNum = mdev;
    return BFCMAP_E_NONE;
}

STATIC int
_bfcmap_device_destroy(bfcmap_dev_ctrl_t  *dev_ctrl) 
{
    if (!dev_ctrl) {
        BFCMAP_SAL_UNLOCK(bfcmap_lock);
        return BFCMAP_E_NONE;
    }

    BFCMAP_SAL_LOCK(bfcmap_lock);

    BFCMAP_DEVICE_CTRL(dev_ctrl->mdev) = NULL;
    BFCMAP_SAL_FREE(dev_ctrl->ports);
    BFCMAP_SAL_FREE(dev_ctrl);

    BFCMAP_SAL_UNLOCK(bfcmap_lock);

    return BFCMAP_E_NONE;
}

