/*
 * $Id: bfcmap_int.h 1.4 Broadcom SDK $
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

#ifndef BFCMAP_INTERNAL_H
#define BFCMAP_INTERNAL_H

#include <bfcmap_sal.h>
#include <bfcmap_util.h>
#include <bfcmap_vlan.h>
#include <bfcmap_stat.h>
#include <bfcmap_ident.h>
#include <bfcmap_drv.h>
#include <bfcmap_ctrl.h>


/* bmacsec equiv of reg/mem is in mem.h */
#define BFCMAP_REG64_READ(c,a,pv) \
            (c)->msec_io_f((c)->dev_addr, BFCMAP_PORT((c)),  \
                            BFCMAP_IO_REG_RD, (a), 2, 1, (pv))

#define BFCMAP_REG32_READ(c,a,pv) \
            (c)->msec_io_f((c)->dev_addr, BFCMAP_PORT((c)),  \
                            BFCMAP_IO_REG_RD, (a), 1, 1, (pv))

#define BFCMAP_REG64_WRITE(c,a,pv) \
            (c)->msec_io_f((c)->dev_addr, BFCMAP_PORT((c)),  \
                            BFCMAP_IO_REG_WR, (a), 2, 1, (pv))

#define BFCMAP_REG32_WRITE(c,a,pv) \
            (c)->msec_io_f((c)->dev_addr, BFCMAP_PORT((c)),  \
                            BFCMAP_IO_REG_WR, (a), 1, 1, (pv))


#define BFCMAP_WWN_HI(mac)          BMAC_TO_32_HI(mac)
#define BFCMAP_WWN_LO(mac)          BMAC_TO_32_LO(mac)
#define BFCMAP_WWN_BUILD(mac,hi,lo) BMAC_BUILD_FROM_32(mac,hi,lo)

#define BFCMAP_PID_TO_32(pid)  \
   ((((buint8_t *)(pid))[0] << 16 )|\
    (((buint8_t *)(pid))[1] << 8  )|\
    (((buint8_t *)(pid))[2] << 0  ))

#define BFCMAP_PID_BUILD(pid, val32)\
   ((buint8_t *)(pid))[0] = ((val32) >> 16) & 0xff ;\
   ((buint8_t *)(pid))[1] = ((val32) >> 8 ) & 0xff ;\
   ((buint8_t *)(pid))[2] = ((val32) >> 0 ) & 0xff ;


typedef enum {
    BFCMAP_UC_FIRMWARE_INIT = 0xA,
    BFCMAP_UC_LINK_RESET,
    BFCMAP_UC_LINK_BOUNCE,
    BFCMAP_UC_LINK_ENABLE,
    BFCMAP_UC_LINK_SPEED
} bfcmap_lmi_uc_cmds_t ;

typedef enum {
    BFCMAP_UC_LINK_UP = 0x1A,
    BFCMAP_UC_LINK_FAILURE
} bfcmap_uc_lmi_cmds_t ;


#endif /* BFCMAP_INTERNAL_H */
