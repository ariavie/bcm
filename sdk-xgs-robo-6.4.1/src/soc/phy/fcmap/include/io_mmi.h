/*
 * $Id: io_mmi.h,v 1.1 Broadcom SDK $
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

#ifndef BLMI_MMI_H
#define BLMI_MMI_H


#define BLMI_MMI_REG_CFG                (0x00)
#define BLMI_MMI_REG_STATUS             (0x01)

#define BLMI_MMI_ID1                    (0x02)
#define BLMI_MMI_ID2                    (0x03)
#define BLMI_MMI_REG_PORT_CMD           (0x04) 
    #define BLMI_MMI_PORT_CMD_DOIT           (0x1)
    #define BLMI_MMI_PORT_CMD_WRITE_EN       (0x2)
    #define BLMI_MMI_PORT_CMD_MEM_EN         (0x4)
    #define BLMI_MMI_PORT_CMD_RESET          (0x8000)

#define BLMI_MMI_REG_PORT_EXT_CMD       (0x05) 
    #define BLMI_MMI_PORT_EXT_BURST_EN       (0x01)
    #define BLMI_MMI_PORT_EXT_BURST_LEN(l)   (((l) & 0x1ff) << 1)

#define BLMI_MMI_REG_PORT_STATUS        (0x06) 
    #define BLMI_MMI_PORT_STATUS_READY        (0x1)
    #define BLMI_MMI_PORT_STATUS_ERROR        (0x1c)

#define BLMI_MMI_REG_PORT_ADDR_LSB      (0x07) 
#define BLMI_MMI_REG_PORT_ADDR_MSB      (0x08) 
#define BLMI_MMI_REG_PORT_DATA_LSB      (0x09) 
#define BLMI_MMI_REG_PORT_DATA_MSB      (0x0A)
#define BLMI_MMI_REG_PORT_FIFO_WR_PTR   (0x0B) 
#define BLMI_MMI_REG_PORT_FIFO_RD_PTR   (0x0C) 
#define BLMI_MMI_REG_PORT_TIMER         (0x0D) 

#endif /* BLMI_MMI_H */

