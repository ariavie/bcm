/*
 * $Id: mmi_cmn.h,v 1.1 Broadcom SDK $
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

#ifndef BLMI_MMI_CMN_H
#define BLMI_MMI_CMN_H

#include <blmi_io.h>
#undef  STATIC
#define STATIC static

/*******************************************************
 * Trace callback
 ******************************************************/
typedef void (*_mmi_trace_callback_t)(
    blmi_dev_addr_t dev_addr,
    blmi_io_op_t op,
    buint32_t io_addr,
    buint32_t *data, 
    int word_sz,
    int num_entry);


/*******************************************************
 * LMI control structure
 ******************************************************/
typedef struct _mmi_dev_info_s {
    blmi_dev_addr_t     addr;
    buser_sal_lock_t    lock;
    int                 used;
    int                 fifo_size;
    int                 is_cl45;
    int                 cl45_dev;
    buint32_t           cap_flags;
} _mmi_dev_info_t;

/*******************************************************
 * Pre-process LMI callback
 ******************************************************/
typedef int (*_mmi_pre_cb_t)(
    _mmi_dev_info_t *dev_info,
    int dev_port,
    blmi_io_op_t op,
    buint32_t *io_addr,
    buint32_t *data, 
    int word_sz,
    int num_entry);

/*******************************************************
 * Post-process LMI callback
 ******************************************************/
typedef int (*_mmi_post_cb_t)(
    _mmi_dev_info_t *dev_info,
    int dev_port,
    blmi_io_op_t op,
    buint32_t *io_addr,
    buint32_t *data, 
    int word_sz,
    int num_entry);


extern blmi_dev_mmi_mdio_rd_f _blmi_mmi_rd_f;
extern blmi_dev_mmi_mdio_wr_f _blmi_mmi_wr_f;

_mmi_dev_info_t*
_blmi_mmi_create_device(blmi_dev_addr_t dev_addr, int cl45);

_mmi_dev_info_t * 
_blmi_mmi_cmn_get_device_info(blmi_dev_addr_t dev_addr);


/*
 * The following flags define the capability of the MMI
 * core.
 */
/* Move register/table entries within the device */
#define BLMI_MMI_CAP_DMA_WITHIN_DEVICE       0x01
/* DMA to/from host to device table/registers */
#define BLMI_MMI_CAP_DMA_TO_FROM_HOST        0x02
/* Write the same entry to incremental addresses */
#define BLMI_MMI_CAP_WRITE_MULTIPLE          0x04


/* Trace functions */
extern _mmi_trace_callback_t  _blmi_trace_cb;
extern unsigned int           _blmi_trace_flags;
#define BLMI_TRACE_REG    0x01
#define BLMI_TRACE_TBL    0x02
int
_blmi_register_trace(unsigned int flags, _mmi_trace_callback_t cb);

int
_blmi_unregister_trace(unsigned int flags);


#define BLMI_TRACE_CALLBACK(dev_info,op,io_addr,data,word_sz,num_entry)\
if (_blmi_trace_flags) {                                               \
  switch (op) {                                                        \
    case BLMI_IO_REG_RD:                                               \
    case BLMI_IO_REG_WR:                                               \
        if (_blmi_trace_flags & BLMI_TRACE_REG) {                      \
            _blmi_trace_cb(                                            \
                (dev_info)->addr,                                      \
                op,                                                    \
                io_addr,                                               \
                data,                                                  \
                word_sz,                                               \
                num_entry);                                            \
        }                                                              \
        break;                                                         \
    case BLMI_IO_TBL_RD:                                               \
    case BLMI_IO_TBL_WR:                                               \
        if (_blmi_trace_flags & BLMI_TRACE_TBL) {                      \
            _blmi_trace_cb(                                            \
                (dev_info)->addr,                                      \
                op,                                                    \
                io_addr,                                               \
                data,                                                  \
                word_sz,                                               \
                num_entry);                                            \
        }                                                              \
        break;                                                         \
  }                                                                    \
}
    

#endif /* BLMI_MMI_CMN_H */

