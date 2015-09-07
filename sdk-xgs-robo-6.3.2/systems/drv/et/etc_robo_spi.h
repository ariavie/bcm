/*
 * $Id: etc_robo_spi.h 1.10 Broadcom SDK $
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
 * BCM53xx RoboSwitch utility functions
 */

#ifndef _robo_spi_h_
#define _robo_spi_h_

#if defined(ROBO_OLD)
#define ROBO_CPU_PORT_PAGE  0x10
/* should set to 0x18, but 0x18 can't read the phy id value. */
#endif

#define SPI_FIFO_MAX_SIZE  32
#define SPI_INTFLAG_TIMEOUT  5000

#define SPI_STATES_DISABLE  0x0
#define SPI_STATES_ENABLE  0x1
#define SPI_STATES_WRITE  0x2
#define SPI_STATES_READ  0x4

/* SPI mode control definition */
#define SPI_MODE_CTRL_MODE  0x0        /* SPI Mode (CPOL, CPHA) */
#define SPI_MODE_CTRL_ACKEN  0x1        /* SPI RACK enable */
#define SPI_MODE_CTRL_ENDIAN  0x2        /* SPI Big Endian enable */
#define SPI_MODE_CTRL_CLOCK  0x4        /* SPI Clock divider parameter */
#define SPI_MODE_CTRL_LSBEN  0x10        /* SPI LSB first enable */

#define SPI_CCD_MAX  0xf        /* max N value for spi clock divider parameter(CCD) */

/* reutrn value for SPI driver */
#define SPI_ERR_NONE  0
#define SPI_ERR_TIMEOUT  -1
#define SPI_ERR_INTERNAL  -2
#define SPI_ERR_PARAM  -3
#define SPI_ERR_UNAVAIL  -4
#define SPI_ERR_UNKNOW  -5

#define _DD_MAKEMASK1(n) (1 << (n))
#define _DD_MAKEMASK(v,n) ((((1)<<(v))-1) << (n))
#define _DD_MAKEVALUE(v,n) ((v) << (n))
#define _DD_GETVALUE(v,n,m) (((v) & (m)) >> (n))

/* SPICONFIG: SPI Configuration Register (0x284, R/W) */
#define S_SPICFG_SS        0        /* SPI SS (device(n)) enable */
#define V_SPICFG_SS(x)    _DD_MAKEVALUE(x,S_SPICFG_SS)

#define S_SPICFG_RDC      3        /* SPI Read byte count */
#define V_SPICFG_RDC(x)   _DD_MAKEVALUE(x,S_SPICFG_RDC)

#define S_SPICFG_WDC      13        /* SPI Write data byte count */
#define V_SPICFG_WDC(x)   _DD_MAKEVALUE(x,S_SPICFG_WDC)

#define S_SPICFG_WCC      23        /* SPI Write command byte count */
#define V_SPICFG_WCC(x)   _DD_MAKEVALUE(x,S_SPICFG_WCC)

#define V_SPICFG_START    _DD_MAKEVALUE(1,31)        /* Start SPI transfer */

#define SPI_FREQ_DEFAULT 2000000  /* 2MHz */
#define SPI_FREQ_20MHZ   20000000 /* 20MHz */
#define SPI_FREQ_8051    20000    /* 20KHz */

extern void * robo_attach(void *sih, uint8 ss);

extern void robo_detach(void *robo);
extern void robo_switch_bus(void *robo,uint8 bustype);
extern void robo_rreg(void *robo, uint8 cid, uint8 page, uint8 addr, uint8 *buf, uint len);
extern void robo_wreg(void *robo, uint8 cid, uint8 page, uint8 addr, uint8 *buf, uint len);
extern void robo_rvmii(void *robo, uint8 cid);
extern void robo_i2c_rreg(void *robo, uint8 chipid, uint8 addr, uint8 *buf, uint len);
extern void robo_i2c_wreg(void *robo, uint8 chipid, uint8 addr, uint8 *buf, uint len);
extern void robo_i2c_rreg_intr(void *robo, uint8 chipid, uint8 *buf);
extern void robo_i2c_read_ARA(void *robo, uint8 *chipid);
extern int chipc_spi_set_freq(void* robo, cc_spi_id_t id, uint32 speed_hz);
extern void robo_select_device(void *robo,uint16 phyidh,uint16 phyidl);
extern void robo_mdio_reset(void) ;
extern void setSPI(int fEnable);

#endif /* _robo_spi_h_ */
