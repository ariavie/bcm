/*
 * $Id: bfcmap_ident.c,v 1.3 Broadcom SDK $
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

/**********************************************************
 * Table of BFCMAP drivers
 **********************************************************/

#if defined(BCM84756_A0) || defined(BCM84756_C0) 
extern bfcmap_drv_t bfcmap84756_a0_drv;
#endif

/***********************************************************
 *  Static/local function declaration
 **********************************************************/
STATIC bfcmap_drv_tbl_entry_t  bfcmap_drv_table[] = {
#if defined(BCM84756_A0)
    { "BFCMAP84756_A0", 
       BFCMAP_CORE_BCM84756,
       4,
       &bfcmap84756_a0_drv,
    },
#endif /* BCM84756_A0 */
#if defined(BCM84756_C0)
    { "BFCMAP84756_C0", 
       BFCMAP_CORE_BCM84756_C0,
       4,
       &bfcmap84756_a0_drv,
    },
#endif /* BCM84756_A0 */
   {NULL, BFCMAP_CORE_UNKNOWN, 0, NULL}  /* Last entry */
};

/*
 * Return bfcmap driver entry corresponding to device core dev_core.
 */
bfcmap_drv_tbl_entry_t* _bfcmap_find_driver(bfcmap_core_t dev_core)
{
    bfcmap_drv_tbl_entry_t *entry = &bfcmap_drv_table[0];

    while((entry->id != BFCMAP_CORE_UNKNOWN) && (entry->id != dev_core)) {
        entry++;
    }
    if (entry->id == BFCMAP_CORE_UNKNOWN) {
        return NULL;
    }
    return entry;
}

/*
 * bfcmap_port_t <--> bfcmap_ctrl mapping entry.
 */
typedef struct bfcmap_portid_map_s {
    bfcmap_port_t   p;
    bfcmap_port_ctrl_t   *mpctrl;
} bfcmap_portid_map_t;

/*
 * Mapping table between bfcmap_port_t and internal bfcmap control
 * structure bfcmap_port_ctrl_t.
 */
STATIC bfcmap_portid_map_t _portid_map_tbl[BFCMAP_NUM_PORT];

int _bfcmap_get_device(bfcmap_dev_addr_t dev_addr) 
{
    int unit;
    bfcmap_dev_ctrl_t *bfcmap_unit_ctrl = NULL;

    for (unit = 0; unit < BFCMAP_MAX_UNITS; unit++) {
        bfcmap_unit_ctrl = BFCMAP_DEVICE_CTRL(unit);
        if (bfcmap_unit_ctrl &&
            (bfcmap_unit_ctrl->flags & BFCMAP_UNIT_ATTACHED) &&
            (BFCMAP_SAL_DEV_ADDR_CMP(bfcmap_unit_ctrl->dev_addr, 
                                     dev_addr))) {
            break;
        }
    }

    return (unit < BFCMAP_MAX_UNITS) ? unit : -1;
}

STATIC int bfcmap_num_port = 0;

STATIC int _bfcmap_get_portmap_index(bfcmap_port_t p)
{
    int low, hi, mid;

    low = 0;
    hi = bfcmap_num_port - 1;

    while (low <= hi) {
        mid = (low + hi)/2;
        BFCMAP_SAL_ASSERT(_portid_map_tbl[mid].mpctrl != NULL);
        if (p < _portid_map_tbl[mid].p) {
            hi = mid - 1;
        } else if (p > _portid_map_tbl[mid].p) {
            low = mid + 1;
        } else {
            return mid;
        }
    }
    return -1;
}

/*
 * Add port mapping for bfcmap port mapping table.
 */
int _bfcmap_add_port(bfcmap_port_t p, bfcmap_port_ctrl_t *mpc)
{
    int ii, jj;
    
    if (bfcmap_num_port == BFCMAP_NUM_PORT) {
        return -1;
    }

    /*
     * Check if port already exists.
     */
    if (_bfcmap_get_portmap_index(p) >= 0) {
        return -1;
    }

    ii = 0;
    while ((ii < bfcmap_num_port) && (_portid_map_tbl[ii].p < p)) {
        ii++;
    }

    if (ii < bfcmap_num_port) {
        for (jj = bfcmap_num_port; jj > ii; jj--) {
            _portid_map_tbl[jj].mpctrl = _portid_map_tbl[jj - 1].mpctrl;
            _portid_map_tbl[jj].p = _portid_map_tbl[jj - 1].p;
        }
    }
    _portid_map_tbl[ii].mpctrl = mpc;
    _portid_map_tbl[ii].p = p;
    mpc->portId = p;
    bfcmap_num_port++;

    return (ii);
}

/*
 * Remove port mapping for bfcmap mapping table.
 */
int _bfcmap_remove_port(bfcmap_port_t p)
{
    int jj, index;
    
    if ((index = _bfcmap_get_portmap_index(p)) < 0) {
        /* Port doesnt exist. */
        return -1;
    }

    for (jj = index; jj < bfcmap_num_port - 1; jj++) {
        _portid_map_tbl[jj].mpctrl = _portid_map_tbl[jj + 1].mpctrl;
        _portid_map_tbl[jj].p = _portid_map_tbl[jj + 1].p;
    }
    _portid_map_tbl[jj].mpctrl = NULL;
    bfcmap_num_port--;
        
    return 0;
}

/*
 * Return bfcmap_port_ctrl_t corresponding to bfcmap_port_t.
 */
bfcmap_port_ctrl_t * _bfcmap_get_port_ctrl(bfcmap_port_t p)
{
    int index;

    if ((index = _bfcmap_get_portmap_index(p)) >= 0) {
        return _portid_map_tbl[index].mpctrl;
    }

    return NULL;
}

