/*
 * $Id: phyreg.h 1.24 Broadcom SDK $
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
 * File:        phyreg.h
 *
 * Data structures and function prototypes for PHY register access.
 */

#ifndef   _PHYREG_LOCAL_H_
#define   _PHYREG_LOCAL_H_

#define PHY_AER_REG_ADDR_AER(_addr)    (((_addr) >> 16) & 0x0000FFFF)  
#define PHY_AER_REG_ADDR_BLK(_addr)    (((_addr) & 0x000FFF0))
#define PHY_AER_REG_ADDR_REGAD(_addr)  ((((_addr) & 0x00008000) >> 11) | \
                                        ((_addr) & 0x0000000F))
#define PHY_AER_REG        0xFFDE
#define PHY_AER_REG_ADDR_CL45_DEVID(_addr) (((_addr) >> 27) & 0x0000001F)
#define PHY_AER_REG_ADDR_LANE_NUM(_addr)   (((_addr) >> 16) & 0x07FF)
#define PHY_AER_REG_ADDR_CL45_REGAD(_addr) (((_addr) & 0x000FFFF))

#define READ_PHY_REG(_unit, _pc,  _addr, _value) \
            ((_pc->read)((_unit), (_pc->phy_id), (_addr), (_value)))
#define WRITE_PHY_REG(_unit, _pc, _addr, _value) \
            ((_pc->write)((_unit), (_pc->phy_id), (_addr), (_value)))
#define MODIFY_PHY_REG(_unit, _pc, _addr, _data, _mask) \
            (phy_reg_modify((_unit), (_pc), (_addr), (_data), (_mask)))

extern int phy_reg_modify(int unit, phy_ctrl_t *pc, uint32 phy_addr,
                          uint16 data, uint16 mask);

extern int phy_reg_fe_read(int unit, phy_ctrl_t *pc, uint16 reg_bank, 
                           uint8 reg_addr, uint16 *data);
extern int phy_reg_fe_write(int unit, phy_ctrl_t *pc, uint16 reg_bank, 
                         uint8 reg_addr, uint16 data);
extern int phy_reg_fe_modify(int unit, phy_ctrl_t *pc, uint16 reg_bank,
                             uint8 reg_addr, uint16 data, uint16 mask);

extern int phy_reg_ge_read(int unit, phy_ctrl_t *pc, uint32 flags, 
                           uint16 reg_bank, uint8 reg_addr, uint16 *data);
extern int phy_reg_ge_write(int unit, phy_ctrl_t *pc, uint32 flags, 
                            uint16 reg_bank, uint8 reg_addr, uint16 data);
extern int phy_reg_ge_modify(int unit, phy_ctrl_t *pc, uint32 flags, 
                             uint16 reg_bank, uint8 reg_addr, 
                             uint16 data, uint16 mask);

extern int phy_reg_xge_read(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
                             uint8 dev_addr, uint16 reg_addr, uint16 *data);
extern int phy_reg_xge_write(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
                             uint8 dev_addr, uint16 reg_addr, uint16 data);
extern int phy_reg_xge_modify(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
                             uint8 dev_addr, uint16 reg_addr, uint16 data, uint16 mask);


extern int phy_reg_serdes_read(int unit, phy_ctrl_t *pc, uint16 reg_bank, 
                           uint8 reg_addr, uint16 *data);
extern int phy_reg_serdes_write(int unit, phy_ctrl_t *pc, uint16 reg_bank, 
                            uint8 reg_addr, uint16 data);
extern int phy_reg_serdes_modify(int unit, phy_ctrl_t *pc, uint16 reg_bank,
                             uint8 reg_addr, uint16 data, uint16 mask);

extern int phy_reg_xgxs_read(int unit, phy_ctrl_t *pc,
                           uint16 reg_bank, uint8 reg_addr, uint16 *data);
extern int phy_reg_xgxs_write(int unit, phy_ctrl_t *pc,
                           uint16 reg_bank, uint8 reg_addr, uint16 data);
extern int phy_reg_xgxs_modify(int unit, phy_ctrl_t *pc,
                           uint16 reg_bank, uint8 reg_addr, 
                           uint16 data, uint16 mask);

extern int phy_reg_xgxs6_read(int unit, phy_ctrl_t *pc, uint32 flags, 
                           uint16 reg_bank, uint8 reg_addr, uint16 *data);
extern int phy_reg_xgxs6_write(int unit, phy_ctrl_t *pc, uint32 flags, 
                           uint16 reg_bank, uint8 reg_addr, uint16 data);
extern int phy_reg_xgxs6_modify(int unit, phy_ctrl_t *pc, uint32 flags, 
                           uint16 reg_bank, uint8 reg_addr, 
                           uint16 data, uint16 mask);

extern int phy_reg_aer_read(int unit, phy_ctrl_t *pc, uint32 reg_addr,
                            uint16 *data);
extern int phy_reg_aer_write(int unit, phy_ctrl_t *pc, uint32 reg_addr,
                            uint16 data);
extern int phy_reg_aer_modify(int unit, phy_ctrl_t *pc, uint32 reg_addr,
                            uint16 data, uint16 mask);

extern int phy_reg_qsgmii_aer_read(int unit, phy_ctrl_t *pc, uint32 reg_addr,
                            uint16 *data);
extern int phy_reg_qsgmii_aer_write(int unit, phy_ctrl_t *pc, uint32 reg_addr,
                            uint16 data);
extern int phy_reg_qsgmii_aer_modify(int unit, phy_ctrl_t *pc, uint32 reg_addr,
                            uint16 data, uint16 mask);

/* Register access mappings */

/* hl65 register access */
#define HL65_REG_READ(_unit, _pc, _flags, _reg_addr, _val) \
        phy_reg_aer_read((_unit), (_pc), \
        (((_pc)->flags & PHYCTRL_MDIO_ADDR_SHARE)? \
        ((_reg_addr)|(((_pc)->lane_num) <<16)):(_reg_addr)),(_val))
                                                                                
#define HL65_REG_WRITE(_unit, _pc, _flags, _reg_addr, _val) \
        phy_reg_aer_write((_unit), (_pc), \
        (((_pc)->flags & PHYCTRL_MDIO_ADDR_SHARE)? \
        ((_reg_addr)| (((_pc)->lane_num)<<16)):(_reg_addr)),(_val))
                                                                                
#define HL65_REG_MODIFY(_unit, _pc, _flags, _reg_addr, _val, _mask) \
        phy_reg_aer_modify((_unit), (_pc), \
        (((_pc)->flags & PHYCTRL_MDIO_ADDR_SHARE)? \
        ((_reg_addr) | (((_pc)->lane_num)<<16)):(_reg_addr)),(_val), (_mask))


/* XGXS_16G register access */

#define XGXS16G_REG_READ(_unit, _pc, _flags, _reg_addr, _val) \
        phy_reg_aer_read((_unit), (_pc), \
        (((_pc)->flags & PHYCTRL_MDIO_ADDR_SHARE)? \
        ((_reg_addr) | ((((_pc)->phy_id & 0x1f) \
        + (_pc)->lane_num)<<16)):(_reg_addr)),(_val))
                                                                                
#define XGXS16G_REG_WRITE(_unit, _pc, _flags, _reg_addr, _val) \
        phy_reg_aer_write((_unit), (_pc), \
        (((_pc)->flags & PHYCTRL_MDIO_ADDR_SHARE)? \
        ((_reg_addr) | ((((_pc)->phy_id & 0x1f) \
        + (_pc)->lane_num)<<16)):(_reg_addr)),(_val))
                                                                                
#define XGXS16G_REG_MODIFY(_unit, _pc, _flags, _reg_addr, _val, _mask) \
        phy_reg_aer_modify((_unit), (_pc), \
        (((_pc)->flags & PHYCTRL_MDIO_ADDR_SHARE)? \
        ((_reg_addr) | ((((_pc)->phy_id & 0x1f) \
        + (_pc)->lane_num)<<16)):(_reg_addr)),(_val), (_mask))

#endif /* _PHYREG_LOCAL_H_ */
