/*
 * $Id: sbusdma.h,v 1.17 Broadcom SDK $
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
 * File:        sbusdma.h
 * Purpose:     Maps out structures used for SBUSDMA operations and
 *              exports routines.
 */

#ifndef _SOC_SBUSDMA_H
#define _SOC_SBUSDMA_H

#include <soc/defs.h>
#include <soc/types.h>
#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/core/sync.h>
#include <sal/core/thread.h>

#define SOC_SBUSDMA_TYPE_TDMA      0
#define SOC_SBUSDMA_TYPE_SLAM      1
#define SOC_SBUSDMA_TYPE_DESC      2

#define SOC_SBUSDMA_CH_PER_CMC     3

/***** Register mode stuff *****/
extern int
_soc_mem_array_sbusdma_read(int unit, soc_mem_t mem, unsigned array_index,
                            int copyno, int index_min, int index_max,
                            uint32 ser_flags, void *buffer);
extern int
_soc_mem_sbusdma_read(int unit, soc_mem_t mem, int copyno,
                      int index_min, int index_max,
                      uint32 ser_flags, void *buffer);
extern int
_soc_mem_array_sbusdma_write(int unit, soc_mem_t mem, unsigned array_index_start, 
                             unsigned array_index_end, int copyno,
                             int index_begin, int index_end, 
                             void *buffer, uint8 mem_clear, 
                             int clear_buf_ent);
extern int
_soc_mem_sbusdma_write(int unit, soc_mem_t mem, int copyno,
                       int index_begin, int index_end, 
                       void *buffer, uint8 mem_clear, int clear_buf_ent);
extern int
_soc_mem_sbusdma_clear(int unit, soc_mem_t mem, int copyno, void *null_entry);

extern int
_soc_mem_sbusdma_clear_specific(int unit, soc_mem_t mem,
                                unsigned array_index_min, unsigned array_index_max,
                                int copyno,
                                int index_min, int index_max,
                                void *null_entry);

/***** Descriptor mode stuff *****/
/* Public structures and routines */
typedef struct { /* h/w desc structure */
#define SOC_SBUSDMA_CTRL_LAST      (1<<31)
#define SOC_SBUSDMA_CTRL_NOT_LAST ~(1<<31)
#define SOC_SBUSDMA_CTRL_SKIP      (1<<30)
#define SOC_SBUSDMA_CTRL_NOT_SKIP ~(1<<30)
#define SOC_SBUSDMA_CTRL_JUMP      (1<<29)
#define SOC_SBUSDMA_CTRL_NOT_JUMP ~(1<<29)
#define SOC_SBUSDMA_CTRL_APND      (1<<28)
#define SOC_SBUSDMA_CTRL_NOT_APND ~(1<<28)
    uint32 cntrl;    /* DMA control info */
    uint32 req;      /* DMA request info (refer h/w spec for details) */
    uint32 count;    /* DMA count */
    uint32 opcode;   /* Schan opcode (refer h/w spec for details) */
    uint32 addr;     /* Schan address */
    uint32 hostaddr; /* h/w mapped host address */
} soc_sbusdma_desc_t;

typedef int sbusdma_desc_handle_t;
/* status: Non-zero value indicates an error */
typedef void (*soc_sbd_dm_cb_f)(int unit, int status, 
                                sbusdma_desc_handle_t handle, void *data);

typedef struct {
/* Used to indicate that the supplied h/w descriptor info should be used 
   and not generated internally by the create routine */
#define SOC_SBUSDMA_CFG_USE_SUPPLIED_DESC 0x001 
/* Used to indicate that this is a h/w managed descriptor */
#define SOC_SBUSDMA_CFG_DESC_HW_MANAGED   0x002
/* Indicates that the counter is implemented as a memory in the h/w */
#define SOC_SBUSDMA_CFG_COUNTER_IS_MEM    0x100
/* Indicates dma to floor */
#define SOC_SBUSDMA_CFG_NO_BUFF           0x200
    uint32 flags;
    char name[16];               /* Descriptor name */
    uint32 cfg_count;            /* Number of items in descriptor chain (1: single descriptor mode) */
    soc_sbusdma_desc_t *hw_desc; /* Custom hw descriptor info (Optional). 
                                    Valid only when SOC_SBUSDMA_CFG_USE_SUPPLIED_DESC is set */
    void *buff;                  /* Common/contiguous host memory buffer for the complete chain */
    soc_sbd_dm_cb_f cb;          /* Callback function for completion */
    void *data;                  /* Caller's data transparently returned in callback */
} soc_sbusdma_desc_ctrl_t;
    
typedef struct {
    /* NOTE: If hw_desc is used from desc_ctrl then the following
             are ignored: blk, acc_type, addr, width, count, addr_shift */
    int acc_type;      /* access type */
    uint32 blk;        /* Schan block number */
    uint32 addr;       /* Schan addr */
    uint32 width;      /* Counter width */
    uint32 count;      /* Number of counter entries */
    uint32 addr_shift; /* Gap between individual counters (refer h/w spec for details).
                          NOTE: this value should be the same for all counters in a descriptor */
    void *buff;        /* Specific host memory buffer for each descriptor.
                          NOTE: 'buff' is used only if the supplied common/continuous 
                                buffer in desc_ctrl is NULL */
} soc_sbusdma_desc_cfg_t;

extern int
soc_sbusdma_desc_create(int unit, soc_sbusdma_desc_ctrl_t *ctrl, 
                        soc_sbusdma_desc_cfg_t *cfg, 
                        sbusdma_desc_handle_t *desc_handle);
extern int
soc_sbusdma_desc_get(int unit, sbusdma_desc_handle_t desc_handle,
                     soc_sbusdma_desc_ctrl_t *ctrl,
                     soc_sbusdma_desc_cfg_t *cfg);
extern int
soc_sbusdma_desc_get_state(int unit, sbusdma_desc_handle_t desc_handle, 
                           uint8 *state);
/* NOTE: cfg_index only applies to multiple descriptors */
extern int
soc_sbusdma_desc_skip(int unit, sbusdma_desc_handle_t desc_handle, 
                      uint16 cfg_index);
extern int
soc_sbusdma_desc_delete(int unit, sbusdma_desc_handle_t desc_handle);
/* NOTE: cfg_index only applies to multiple descriptors */
extern int
soc_sbusdma_desc_update(int unit, sbusdma_desc_handle_t desc_handle, 
                        uint16 cfg_index, 
                        soc_sbusdma_desc_ctrl_t *ctrl,
                        soc_sbusdma_desc_cfg_t *cfg);
extern int
soc_sbusdma_desc_extend(int unit, sbusdma_desc_handle_t desc_handle, 
                        soc_sbusdma_desc_ctrl_t *ctrl,
                        soc_sbusdma_desc_cfg_t *cfg);
extern int
soc_sbusdma_desc_run(int unit, sbusdma_desc_handle_t desc_handle);

/* Private structures and routines */
#define SOC_SBUSDMA_MAX_DESC 500
typedef struct _soc_sbusdma_state_s { /* s/w state maintenance structure */
    sbusdma_desc_handle_t handle;
    soc_sbusdma_desc_ctrl_t ctrl;
    soc_sbusdma_desc_cfg_t *cfg;
    soc_sbusdma_desc_t *desc;
    uint8 status;
} _soc_sbusdma_state_t;

typedef struct {
    sal_mutex_t lock;
    sal_usecs_t timeout;
    sal_sem_t intr;
    int intrEnb;
    char name[16];
    VOL sal_thread_t pid;
    uint32 count;
    uint8 init;
    uint8 active;
    _soc_sbusdma_state_t *working;
    uint32 _working;
    _soc_sbusdma_state_t *handles[SOC_SBUSDMA_MAX_DESC+1];
} soc_sbusdma_desc_info_t;

#define SOC_SBUSDMA_DM_INFO(unit) \
    SOC_SBUSDMA_INFO(unit)
#define SOC_SBUSDMA_DM_MUTEX(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->lock
#define SOC_SBUSDMA_DM_TO(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->timeout
#define SOC_SBUSDMA_DM_INTR(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->intr
#define SOC_SBUSDMA_DM_INTRENB(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->intrEnb
#define SOC_SBUSDMA_DM_NAME(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->name
#define SOC_SBUSDMA_DM_PID(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->pid
#define SOC_SBUSDMA_DM_START(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->start
#define SOC_SBUSDMA_DM_LAST(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->last
#define SOC_SBUSDMA_DM_COUNT(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->count
#define SOC_SBUSDMA_DM_INIT(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->init
#define SOC_SBUSDMA_DM_ACTIVE(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->active
#define SOC_SBUSDMA_DM_WORKING(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->working
#define SOC_SBUSDMA_DM_HANDLES(unit) \
    SOC_SBUSDMA_DM_INFO(unit)->handles

extern int soc_sbusdma_desc_init(int unit, int interval, uint8 intrEnb);
extern int soc_sbusdma_desc_detach(int unit);

extern int _soc_l2mod_start(int unit, uint32 flags, sal_usecs_t interval);
extern int _soc_l2mod_stop(int unit);

#endif /* !_SOC_SBUSDMA_H */
