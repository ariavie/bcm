/*
 * $Id: phyreg.h,v 1.6 Broadcom SDK $
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
 * This file defines common PHY register definition.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define PHY register definition.
 */

#ifndef _PHYREG_H
#define _PHYREG_H

#include <shared/phyreg.h>

#define SOC_PHY_INTERNAL      _SHR_PORT_PHY_INTERNAL
#define SOC_PHY_NOMAP         _SHR_PORT_PHY_NOMAP
#define SOC_PHY_CLAUSE45      _SHR_PORT_PHY_CLAUSE45
#define SOC_PHY_I2C_DATA8     _SHR_PORT_PHY_I2C_DATA8       
#define SOC_PHY_I2C_DATA16    _SHR_PORT_PHY_I2C_DATA16  

#define SOC_PHY_CLAUSE45_ADDR(_devad, _regad) \
            _SHR_PORT_PHY_CLAUSE45_ADDR(_devad, _regad)
#define SOC_PHY_CLAUSE45_DEVAD(_reg_addr) \
            _SHR_PORT_PHY_CLAUSE45_DEVAD(_reg_addr)
#define SOC_PHY_CLAUSE45_REGAD(_reg_addr) \
            _SHR_PORT_PHY_CLAUSE45_REGAD(_reg_addr)
#define SOC_PHY_I2C_ADDR(_devad, _regad) \
	_SHR_PORT_PHY_I2C_ADDR(_devad, _regad)
#define SOC_PHY_I2C_DEVAD(_reg_addr) \
	_SHR_PORT_PHY_I2C_DEVAD(_reg_addr) 
#define SOC_PHY_I2C_REGAD(_reg_addr) \
	_SHR_PORT_PHY_I2C_REGAD(_reg_addr)

/* Indirect PHY register address flags */
#define SOC_PHY_REG_RESERVE_ACCESS    _SHR_PORT_PHY_REG_RESERVE_ACCESS
#define SOC_PHY_REG_1000X             _SHR_PORT_PHY_REG_1000X
#define SOC_PHY_REG_INDIRECT          _SHR_PORT_PHY_REG_INDIRECT

#define SOC_PHY_REG_INDIRECT_ADDR(_flags, _reg_type, _reg_selector) \
            _SHR_PORT_PHY_REG_INDIRECT_ADDR(_flags, _reg_type, _reg_selector)
#define SOC_PHY_REG_BANK(_reg_addr)  _SHR_PORT_PHY_REG_BANK(_reg_addr)
#define SOC_PHY_REG_ADDR(_reg_addr)  _SHR_PORT_PHY_REG_ADDR(_reg_addr)
#define SOC_PHY_REG_FLAGS(_reg_addr) _SHR_PORT_PHY_REG_FLAGS(_reg_addr)

#endif  /* !_PHYREG_H */
