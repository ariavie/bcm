/*
 * $Id: blmi_io.h 1.3 Broadcom SDK $
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
#ifndef BLMI_IO_H
#define BLMI_IO_H
#include <bbase_types.h>
#include <blmi_err.h>


#define BLMI_MAX_UNITS  BMF_MAX_UNITS

/*
 * typedef for address.
 */
typedef buint32_t      blmi_dev_addr_t;

/*
 * MACSEC I/O operation type identifier.
 */
typedef enum {
	BLMI_IO_REG_RD,   /* Register Read    */
	BLMI_IO_REG_WR,   /* Register Write   */
	BLMI_IO_TBL_RD,   /* Table Read       */
	BLMI_IO_TBL_WR    /* Table Write      */
} blmi_io_op_t;

/*
 * MACSEC I/O callback prototype.
 */
typedef int (*blmi_dev_io_f)(blmi_dev_addr_t devaddr,/* device addr*/
                               int dev_port,        /* dev port index  */
                               blmi_io_op_t op,  /* I/O operation   */  
                               buint32_t io_addr,   /* I/O address     */
                               int word_sz,         /* Word size       */
                               int num_entry,       /* Num entry       */
                               buint32_t *data);    /* Data buffer     */

/*
 * MMI read callback prototype.
 */
typedef int (*blmi_dev_mmi_mdio_rd_f)(blmi_dev_addr_t dev_addr, 
                                    buint32_t miim_reg, buint16_t *data);

/*
 * MMI write callback prototype.
 */
typedef int (*blmi_dev_mmi_mdio_wr_f)(blmi_dev_addr_t dev_addr, 
                                    buint32_t miim_reg, buint16_t data);

/*
 * MMI IO callback registeration.
 */
extern int
blmi_dev_mmi_mdio_register(blmi_dev_mmi_mdio_rd_f mmi_rd_f,
                              blmi_dev_mmi_mdio_wr_f mmi_wr_f);

/*
 * MDIO I/O operation type identifier.
 */
typedef enum {
	BLMI_MDIO_IO_REG_RD,   /* Register Read    */
	BLMI_MDIO_IO_REG_WR    /* Register Write   */
} blmi_mdio_io_op_t;

/* MMI IO routine */
/* MMI I/O function for MMI version 0, i.e. device bcm545XX. */
extern int 
blmi_io_mmi(blmi_dev_addr_t dev_addr, int dev_port, 
                   blmi_io_op_t op, buint32_t io_addr, 
                   int word_sz, int num_entry, buint32_t *data);

/* MMI I/O function for MMI version 1 */
extern int 
blmi_io_mmi1(blmi_dev_addr_t dev_addr, int dev_port, 
                   blmi_io_op_t op, buint32_t io_addr, 
                   int word_sz, int num_entry, buint32_t *data);

/* MMI I/O function for MMI version 1 using clause45 */
extern int 
blmi_io_mmi1_cl45(blmi_dev_addr_t dev_addr, int dev_port, 
                     blmi_io_op_t op, buint32_t io_addr, 
                     int word_sz, int num_entry, buint32_t *data);

/* MMI I/O function for MMI version 1 with quad select support */
extern int 
blmi_io_mmi1_quad(blmi_dev_addr_t dev_addr, int dev_port, 
                     blmi_io_op_t op, buint32_t io_addr, 
                     int word_sz, int num_entry, buint32_t *data);

/* MDIO access routine. */
extern int 
blmi_io_mdio(blmi_dev_addr_t phy_addr, blmi_mdio_io_op_t op, 
                    buint32_t io_addr, buint16_t *data);

#define BLMI_IO_CL45_ADDRESS(cl45_dev, reg)      \
                                (((cl45_dev) << 16) | (reg))

#endif /* BLMI_IO_H */

