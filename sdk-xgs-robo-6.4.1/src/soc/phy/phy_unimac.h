/*
 * $Id: phy_unimac.h,v 1.1 Broadcom SDK $
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

#ifndef   _UNIMAC_H_
#define   _UNIMAC_H_ 

#include <bbase_types.h>
#include "phy_mac_ctrl.h"

extern int phy_unimac_mac_init(phy_mac_ctrl_t *mmc, int dev_port);

extern int phy_unimac_mac_auto_cfg(phy_mac_ctrl_t *mmc, 
                                 int dev_port, int fval);

extern int phy_unimac_mac_reset(phy_mac_ctrl_t *mmc, 
                              int dev_port, int fval);

extern int phy_unimac_mac_enable_set(phy_mac_ctrl_t *mmc, 
                                   int dev_port, int en);

extern int phy_unimac_mac_enable_get(phy_mac_ctrl_t *mmc, 
                                   int dev_port, int *en) ;

extern int phy_unimac_mac_speed_set(phy_mac_ctrl_t *mmc,
                                  int dev_port, int speed);

extern int phy_unimac_mac_speed_get(phy_mac_ctrl_t *mmc, 
                                  int dev_port, int *speed);

extern int phy_unimac_mac_duplex_set(phy_mac_ctrl_t *mmc, 
                                   int dev_port, int duplex);

extern int phy_unimac_mac_duplex_get(phy_mac_ctrl_t *mmc, 
                                   int dev_port, int *fd);

extern int phy_unimac_mac_pause_set(phy_mac_ctrl_t *mmc, 
                                      int dev_port, int pause);

extern int phy_unimac_mac_update_ipg(phy_mac_ctrl_t *mmc,  
                               int dev_port, int ipg);

extern int phy_unimac_mac_max_frame_set(phy_mac_ctrl_t *mmc, 
                                      int dev_port, buint32_t max_frame);

extern int phy_unimac_mac_max_frame_get(phy_mac_ctrl_t *mmc, 
                                      int dev_port, buint32_t *flen);

extern int phy_unimac_mac_pause_frame_fwd(phy_mac_ctrl_t *mmc,
                                        int dev_port, int pause_fwd);


/*
 * Command Register
 */

#define UNIMAC_REG_CMD_CFG_TXEN_MASK             1
#define UNIMAC_REG_CMD_CFG_RXEN_MASK             (1 << 1)
#define UNIMAC_REG_CMD_CFG_SPEED_MASK            ((1 << 2) | (1 << 3))
#define UNIMAC_REG_CMD_CFG_CRC_FWD_MASK          (1 << 6)
#define UNIMAC_REG_CMD_CFG_PAUSE_FWD_MASK        (1 << 7)
#define UNIMAC_REG_CMD_CFG_RX_PAUSE_IGNORE_MASK  (1 << 8)
#define UNIMAC_REG_CMD_CFG_DUPLEX_MASK           (1 << 10)
#define UNIMAC_REG_CMD_CFG_SW_RESET_MASK         (1 << 13)
#define UNIMAC_REG_CMD_CFG_ENA_EXA_CONFIG_MASK   (1 << 22)
#define UNIMAC_REG_CMD_CFG_CTL_FRM_ENA_MASK      (1 << 23)
#define UNIMAC_REG_CMD_CFG_TX_PAUSE_IGNORE_MASK  (1 << 28)

#define UNIMAC_REG_CMD_CFG_TXEN_SHIFT             0
#define UNIMAC_REG_CMD_CFG_RXEN_SHIFT             1
#define UNIMAC_REG_CMD_CFG_SPEED_SHIFT            2
#define UNIMAC_REG_CMD_CFG_CRC_FWD_SHIFT          6
#define UNIMAC_REG_CMD_CFG_PAUSE_FWD_SHIFT        7
#define UNIMAC_REG_CMD_CFG_RX_PAUSE_IGNORE_SHIFT  8
#define UNIMAC_REG_CMD_CFG_DUPLEX_SHIFT           10
#define UNIMAC_REG_CMD_CFG_SW_RESET_SHIFT         13
#define UNIMAC_REG_CMD_CFG_ENA_EXA_CONFIG_SHIFT   22
#define UNIMAC_REG_CMD_CTL_FRM_ENA_SHIFT          23
#define UNIMAC_REG_CMD_CFG_TX_PAUSE_IGNORE_SHIFT  28


#define BMACSEC_PHY54580_LINE_MAC_PORT(p)    (p)
#define BMACSEC_PHY54580_SWITCH_MAC_PORT(p)  ((p) + 8)


/* 
 * CRC Forward
 */
#define UNIMAC_REG_CMD_CFG_CRC_FWD_DISABLE 0
#define UNIMAC_REG_CMD_CFG_CRC_FWD_ENABLE  1

/*
 * Frame Length 
 */
#define UNIMAC_REG_DEFAULT_MAX_FRAME_SIZE        0x0003fff
#define UNIMAC_REG_FRM_LENr_FMR_LEN_MASK         0x0003fff

/* DEFAULT IPG */
#define UNIMAC_REG_DEFAULT_IPG        12


/* MACSEC Control register */
#define UNIMAC_REG_TX_CRC_ENABLE_MASK        (1 << 1)
#define UNIMAC_REG_TX_CRC_ENABLE_SHIFT       1

#define UNIMAC_REG_TX_CRC_ENABLE             1



#define UNIMAC_COMMAND_CONFIGr(p) \
		(0x00100202 + (((p) & 0xf) << 12))

#define UNIMAC_FRM_LENGTHr(p) \
		(0x00100205 + (((p) & 0xf) << 12))

#define UNIMAC_TX_IPG_LENGTHr(p) \
		(0x00100217 + (((p) & 0xf) << 12))

#define UNIMAC_MACSEC_CNTRLr(p)  \
                (0x001002c5 + (((p) & 0xf) << 12))

#define BMACSEC_NUM_WORDS 1

#define BMACSEC_MAC_READ_REG(mmc, _dev_port, _io_addr, _data)         \
               (mmc)->devio_f((mmc)->dev_addr, (_dev_port), BLMI_IO_REG_RD, \
                               (_io_addr), BMACSEC_NUM_WORDS, 1, (_data))

#define BMACSEC_MAC_WRITE_REG(mmc, _dev_port, _io_addr, _data)        \
               (mmc)->devio_f((mmc)->dev_addr, (_dev_port), BLMI_IO_REG_WR, \
                               (_io_addr), BMACSEC_NUM_WORDS, 1, &(_data))


/* Command Config Register */
#define UNIMAC_READ_CMD_CFG_REG(mmc, _dev_port, _data)         \
            BMACSEC_MAC_READ_REG((mmc), (_dev_port),           \
                                 UNIMAC_COMMAND_CONFIGr(_dev_port),  \
                                 (_data))

#define UNIMAC_WRITE_CMD_CFG_REG(mmc, _dev_port, _data)        \
            BMACSEC_MAC_WRITE_REG((mmc), (_dev_port),          \
                                  UNIMAC_COMMAND_CONFIGr(_dev_port), \
                                  (_data))

/* Frame Length Register */
#define UNIMAC_READ_FRM_LEN_REG(mmc, _dev_port, _data)         \
            BMACSEC_MAC_READ_REG((mmc), (_dev_port),           \
                                 UNIMAC_FRM_LENGTHr(_dev_port),      \
                                 (_data))

#define UNIMAC_WRITE_FRM_LEN_REG(mmc, _dev_port, _data)        \
            BMACSEC_MAC_WRITE_REG((mmc), (_dev_port),          \
                                  UNIMAC_FRM_LENGTHr(_dev_port),     \
                                  (_data))

/*  TX IPG Register */
#define UNIMAC_READ_TX_IPG_REG(mmc, _dev_port, _data)          \
            BMACSEC_MAC_READ_REG((mmc), (_dev_port),           \
                                 UNIMAC_TX_IPG_LENGTHr(_dev_port),   \
                                 (_data))

#define UNIMAC_WRITE_TX_IPG_REG(mmc, _dev_port, _data)         \
            BMACSEC_MAC_WRITE_REG((mmc), (_dev_port),          \
                                  UNIMAC_TX_IPG_LENGTHr(_dev_port),  \
                                  (_data))

/* MACSEC Control register */
#define UNIMAC_READ_MACSEC_CNTL_REG(mmc, _dev_port, _data)     \
            BMACSEC_MAC_READ_REG((mmc), (_dev_port),           \
                                 UNIMAC_MACSEC_CNTRLr(_dev_port),    \
                                 (_data))

#define UNIMAC_WRITE_MACSEC_CNTL_REG(mmc, _dev_port, _data)    \
            BMACSEC_MAC_WRITE_REG((mmc), (_dev_port),          \
                                  UNIMAC_MACSEC_CNTRLr(_dev_port),   \
                                  (_data))

#endif   /* _UNIMAC_H_ */

