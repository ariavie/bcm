/*
 * $Id: bfcmap_io.c,v 1.1 Broadcom SDK $
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



/*********************************************************************
 * LMI wrapper functions
 *********************************************************************/

int 
bfcmap_io_mmi(bfcmap_dev_addr_t dev_addr, int dev_port, 
                   bfcmap_io_op_t op, buint32_t io_addr, 
                   int word_sz, int num_entry, buint32_t *data)
{
    return blmi_io_mmi(dev_addr, dev_port, op, io_addr, 
                   word_sz, num_entry, data);
}

/* MMI I/O function for MMI version 1 */
int 
bfcmap_io_mmi1(bfcmap_dev_addr_t dev_addr, int dev_port, 
                   bfcmap_io_op_t op, buint32_t io_addr, 
                   int word_sz, int num_entry, buint32_t *data)
{
    return blmi_io_mmi1(dev_addr, dev_port, op, io_addr, 
                   word_sz, num_entry, data);
}

/* MMI I/O function for MMI version 1 using clause45 */
int 
bfcmap_io_mmi1_cl45(bfcmap_dev_addr_t dev_addr, int dev_port, 
                     bfcmap_io_op_t op, buint32_t io_addr, 
                     int word_sz, int num_entry, buint32_t *data)
{
    return blmi_io_mmi1_cl45(dev_addr, dev_port, op, io_addr, 
                     word_sz, num_entry, data);
}

/* MDIO access routine. */
int 
bfcmap_io_mdio(bfcmap_dev_addr_t phy_addr, bfcmap_mdio_io_op_t op, 
                    buint32_t io_addr, buint16_t *data)
{
    return blmi_io_mdio(phy_addr, op, io_addr, data);
}


int
bfcmap_dev_mmi_mdio_register(bfcmap_dev_mmi_mdio_rd_f mmi_rd_f,
                              bfcmap_dev_mmi_mdio_wr_f mmi_wr_f)
{
    return blmi_dev_mmi_mdio_register(mmi_rd_f, mmi_wr_f);
}

