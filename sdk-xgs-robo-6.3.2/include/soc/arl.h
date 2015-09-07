/*
 * $Id: arl.h 1.15 Broadcom SDK $
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
 * File: 	arl.h
 * Purpose: 	Defines structures and routines for ARL operations
 *              defined in:
 *              drv/arl.c      HW table management
 *              drv/arlmsg.c   ARL message handling
 */

#ifndef _SOC_ARL_H
#define _SOC_ARL_H

#include <shared/avl.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/mcm/memregs.h>
#endif
#ifdef BCM_ROBO_SUPPORT        
#include <soc/robo/mcm/memregs.h>
#endif
extern int soc_arl_attach(int unit);
extern int soc_arl_detach(int unit);
extern int soc_arl_init(int unit);



#define DRV_ARL_DEBUG(flags, stuff) SOC_DEBUG((flags) | SOC_DBG_ARL, stuff)
#define DRV_ARL_OUT(stuff)          SOC_DEBUG(SOC_DBG_ARL, stuff)
#define DRV_ARL_WARN(stuff)         DRV_ARL_DEBUG(SOC_DBG_WARN, stuff)
#define DRV_ARL_ERR(stuff)          DRV_ARL_DEBUG(SOC_DBG_ERR, stuff)
#define DRV_ARL_VERB(stuff)         DRV_ARL_DEBUG(SOC_DBG_VERBOSE, stuff)
#define DRV_ARL_VVERB(stuff)        DRV_ARL_DEBUG(SOC_DBG_VVERBOSE, stuff)
#define DRV_ARL_SHOW(stuff)         ((*soc_cm_print) stuff)


#define _ROBO_SEARCH_LOCK (1 << 0)
#define _ROBO_SCAN_LOCK (1 << 1)


#define TBX_ARL_SCAN_LOCK(unit, soc)\
    do{\
        MEM_RWCTRL_REG_LOCK(soc);\
        soc->arl_exit &= ~(_ROBO_SCAN_LOCK);\
        DRV_ARL_VVERB(("%s %d MEM_RWCTRL_REG_LOCK\n",__FUNCTION__,__LINE__));\
    }while(0)


#define TBX_ARL_SCAN_UNLOCK(unit, soc)\
    do{\
        DRV_ARL_VVERB(("%s %d MEM_RWCTRL_REG_UNLOCK\n",__FUNCTION__,__LINE__));\
        soc->arl_exit |= _ROBO_SCAN_LOCK;\
        MEM_RWCTRL_REG_UNLOCK(soc);\
    }while(0)


#define VO_ARL_SEARCH_LOCK(unit, soc) \
    do{\
        if(SOC_IS_VO(unit)) {\
            ARL_MEM_SEARCH_LOCK(soc);\
            soc->arl_exit &= ~(_ROBO_SEARCH_LOCK);\
            DRV_ARL_VVERB(("%s %d ARL_MEM_SEARCH_LOCK--\n",__FUNCTION__,__LINE__));\
        }\
    }while(0)


#define VO_ARL_SEARCH_UNLOCK(unit, soc) \
do{\
    if(SOC_IS_VO(unit)) {\
        DRV_ARL_VVERB(("%s %d ARL_MEM_SEARCH_UNLOCK\n",__FUNCTION__,__LINE__));\
        soc->arl_exit |= _ROBO_SEARCH_LOCK;\
        ARL_MEM_SEARCH_UNLOCK(soc);\
    }\
}while(0)


typedef void (*soc_robo_arl_cb_fn)(int unit,
                              l2_arl_sw_entry_t *entry_del,
                              l2_arl_sw_entry_t *entry_add,
                              void *fn_data);
extern int soc_robo_arl_register(int unit, soc_robo_arl_cb_fn fn, 
                                    void *fn_data);
extern int soc_robo_arl_unregister(int unit, soc_robo_arl_cb_fn fn, 
                                    void *fn_data);

#define soc_robo_arl_unregister_all(unit) \
	soc_robo_arl_unregister((unit), NULL, NULL)

extern void soc_robo_arl_callback(int unit,
                             l2_arl_sw_entry_t *entry_del,
                             l2_arl_sw_entry_t *entry_add);
/*
 * ARL miscellaneous functions
 */

#define ARL_MODE_NONE		0
#define ARL_MODE_ROBO_POLL	1
#define ARL_MODE_SEARCH_VALID   ARL_MODE_ROBO_POLL
#define ARL_MODE_SCAN_VALID	2

#define ARL_ROBO_POLL_INTERVAL	3000000

extern int soc_arl_mode_set(int unit, int mode);
extern int soc_arl_mode_get(int unit, int *mode);

extern int soc_robo_arl_mode_set(int unit, int mode);
extern int soc_robo_arl_mode_get(int unit, int *mode);

extern int soc_arl_freeze(int unit);
extern int soc_arl_thaw(int unit);
extern int soc_robo_arl_freeze(int unit);
extern int soc_robo_arl_is_frozen(int unit, int *frozen);
extern int soc_robo_arl_thaw(int unit);
extern int soc_arl_frozen_cml_set(int unit, soc_port_t port, int cml,
				  int *repl_cml);
extern int soc_arl_frozen_cml_get(int unit, soc_port_t port, int *cml);
extern void _drv_arl_hash(uint8 *hash_value, uint8 length, uint16 *hash_result);
/*
 * For ARL software shadow database access
 */
extern int soc_arl_database_dump(int unit, uint32 index, 
                                     l2_arl_sw_entry_t *entry);
extern int soc_arl_database_delete(int unit, uint32 index);
extern int soc_arl_database_add(int unit, uint32 index, int pending);

/* new SOC driver to serve the ROBO's SW/HW ARL frozen sync detection */
extern void soc_arl_frozen_sync_init(int unit);
extern void soc_arl_frozen_sync_status(int unit, int *is_sync);

#define ARL_TABLE_WRITE 0  /* For ARL Write operateion */
#define ARL_TABLE_READ 1   /* For ARL Read operateion */
#define ARL_ENTRY_NULL(e1)\
    ((e1)->entry_data[0] == 0 && \
     (e1)->entry_data[1] == 0 && \
     (e1)->entry_data[2] == 0)
 

#define _ARL_SEARCH_VALID_OP_START      0x1
#define _ARL_SEARCH_VALID_OP_GET          0x2
#define _ARL_SEARCH_VALID_OP_DONE       0x3
#define _ARL_SEARCH_VALID_OP_NEXT       0x4
extern int soc_arl_search_valid(int unit, int op, void *index, void *entry, void *entry1);

#endif	/* !_SOC_ARL_H */
