/*
 * $Id: wcmod_phyreg.c 1.16.56.1 Broadcom SDK $
 *
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
 */
#include <sal/types.h>
#include "wcmod_main.h"
#include "wcmod_phyreg.h"


#if defined (_SDK_WCMOD_)
#include <soc/util.h>
#include <soc/cmic.h>
#include <soc/drv.h>
#endif


/*!
\brief WCMod clause 45 read driver.

\param pc

\param addr

\param data

\returns SOC_E_ERROR (not implemented)

\details  Not yet impelemented.

*/

int wcmod_cl45_read(wcmod_st *pc,uint32 addr,uint16 *data)
{
  int rv = 0;
#if defined  (_SDK_WCMOD_)
  rv = soc_esw_miimc45_read(pc->unit, pc->phy_ad, addr, data);
#endif

  if (pc->verbosity > 2)
    printf("%-22s: unit:%d port:%d phy_ad:%d addr:0X%08x data:0X%08x\n",
                                  __func__,pc->unit,pc->port,pc->phy_ad,addr,*data);
  return rv;
}

/*!
\brief WCMod clause 22 read driver.

\param pc

\param addr

\param data

\returns SOC_E_NONE if no errors. SOC_E_ERROR else.

\details 
This is the lowest level driver. It resolves platform level dependencies and
connects WCMod to the appropriate system-platform register access drivers.
*/

int wcmod_cl22_read(wcmod_st *pc,uint32 addr,uint16 *data)
{
  int rv = 0;
  /*int phy_ad = 0;*/
#if defined  (_SDK_WCMOD_)
  rv=   (pc->read)(pc->unit, pc->phy_ad, addr, data);
#endif
  if (pc->verbosity > 2)
    printf("%-22s: unit:%d port:%d phy_ad:%d addr:0x%08x data:0x%04x\n",
                                  __func__,pc->unit,pc->port,pc->phy_ad,addr,*data);
  return rv;
}

/*!
\brief WCMod clause 45 write driver.

\param pc

\param addr

\param data

\returns SOC_E_ERROR.  Not yet implemented.

\details Not yet implemented.
*/
int wcmod_cl45_write(wcmod_st *pc,uint32 addr,uint16 data)
{
  int rv;
  if (pc->verbosity > 2)
    printf("%-22s: unit:%d port:%d phy_ad:%d addr:0X%08x data:0X%08x\n",
			      __func__,pc->unit,pc->port,pc->phy_ad,addr,data);
#if defined (_SDK_WCMOD_)
  soc_esw_miimc45_write(pc->unit, pc->phy_ad, addr, data);
  rv =  SOC_E_NONE;
#endif
  return rv;
}

/*!
\brief WCMod clause 22 write driver.

\param pc

\param addr

\param data

\returns SOC_E_NONE if no errors, SOC_E_ERROR else.

\details 
This is the lowest level driver. It resolves platform level dependencies and
connects WCMod to the appropriate system-platform register access drivers.
*/
int wcmod_cl22_write(wcmod_st *pc,uint32 addr,uint16 data)
{
  int rv;
  if (pc->verbosity > 2)
    printf("%-22s: unit:%d port:%d phy_ad:%d addr:0x%08x data:0x%04x\n",
			       __func__,pc->unit,pc->port,pc->phy_ad,addr,data);
#if defined (_SDK_WCMOD_)
  rv =  (pc->write)(pc->unit, pc->phy_ad, addr, data);
#endif
  return rv;
}

int wcmod_cl45_modify(wcmod_st *pc,uint32 addr,uint16 data, uint16 mask)
{
  int rv;
  uint16  tmpData, otmp;

  rv = wcmod_cl45_read(pc, addr, &tmpData);
  otmp     = tmpData;
  tmpData &= ~mask;
  tmpData |= data;

  if (otmp != tmpData) {
    rv = wcmod_cl45_write(pc, addr, tmpData);
  }
  return rv;
}

/*!
\brief spin on a register bit.

\param pc

\param reg

\param bit_num

\param bitset

\param timeout

\returns SOC_E_NONE if no errors, SOC_E_ERROR else.

\details 
Waits for a reg. bit to be set. Spins until bit is set or time out. The time out
is programmable.
*/
int wcmod_regbit_set_wait_check(wcmod_st* pc,
                  int reg,              /* register to check */
                  int bit_num,          /* bit number to check */
                  int bitset,           /* check bit set or bit clear */
                  int timeout)          /* max wait time to check */
{
  uint16         data16 = 0;
  soc_timeout_t  to;
  int            rv;
  int            cond;

  /* override min_polls with iteration count for DV. */
#ifdef _SDK_WCMOD_
  soc_timeout_init(&to, timeout, 0);
#else
  wcmod_timeout_init(&to, timeout, (int)100);
#endif
  do {
     rv = WC40_REG_READ(pc->unit, pc, 0,reg,&data16);
     pc->accData = data16; 
     if (pc->verbosity > 2)
       printf ("%-22s: reg:0x%08x bit_num:0x%08x bitset:%d\n",
                                             __func__, reg, bit_num, bitset);
     cond = (bitset  && (data16 & bit_num)) || 
            (!bitset && !(data16 & bit_num));
     if (cond || (rv < 0)) {
       break;
    }
#ifdef _SDK_WCMOD_
  } while (!soc_timeout_check(&to));
#else
  } while (!wcmod_timeout_check(&to));
#endif
    /* Do one final check potentially after timeout as required by soc_timeout */
    if (rv >= 0) {
     rv = WC40_REG_READ(pc->unit, pc, 0,reg,&data16);
     pc->accData = data16; 
     if (pc->verbosity > 2)
       printf ("%-22s: reg:0x%08x bit_num:0x%08x bitset:%d\n",
                                             __func__, reg, bit_num, bitset);
     cond = (bitset  && (data16 & bit_num)) || 
            (!bitset && !(data16 & bit_num));
    }
  
  return (cond? SOC_E_NONE: SOC_E_TIMEOUT);
}

/*!
\brief Internal function to implement counter based timeout while polling for a
bit in a register.

\param to soc_timeout_t structure pointer

\param usec Microseconds to wait before timeout

\param min_polls Number of iterations before calling timeout.

\returns Nothing

\details 
*/
void wcmod_timeout_init(soc_timeout_t *to, uint32 usec, int min_polls)
{
#if defined (_SDK_WCMOD_)
  /* Not yet implemented. This is used to keep runaway waits in check */
  printf ("%-22s not implemented\n", __func__);
#endif
}

/*!
\brief Tier1 API for register read.  Resolves cl22/cl45 + dxgxs considerations

\param unit Passed in. Need not be used.

\param pc Pointer to wcmod_st

\param a 32 bit address

\param phy_data 16 bit data pointer.

\returns  0 on success. Non-zero on fail.

\details 
*/
int wcmod_reg_aer_read(uint32 unit, wcmod_st *pc, uint32 a, uint16 *phy_data)
{
  uint16 data16 = 0;
  int    cl45Devid, cl45_ln_no, cl45_addr;

  uint32 phy_reg_aer = 0, phy_reg_blk = 0,phy_reg_addr = 0, addr = a;

  if (pc->lane_num_ignore) {  /* if asked not to ignore */
    addr = addr & 0xf800ffff; /* kill the extension. i.e. always do lane 0. */
  } else {
    if (pc->lane_select == WCMOD_LANE_BCST) {
       if (pc->verbosity > 1) printf ("%s: WARN: BCST ignored for read\n", __func__);
      addr = addr & 0xf800ffff; /* kill the extension. i.e. always do lane 0. */
    } else {
      addr |= ((pc->this_lane & 0x7) << 16);
    }
  }

  if (pc->mdio_type == WCMOD_MDIO_CL45) {
    phy_reg_addr = PHY_AER_REG_ADDR_CL45_REGAD(addr);   /* 0000FFFF  */
    phy_reg_aer  = PHY_AER_REG_ADDR_AER(addr);          /*(FFFF0000 >> 16)*/
    cl45Devid    = PHY_AER_REG_ADDR_CL45_DEVID(addr);   /* F8000000 >> 27 */
    cl45_ln_no   = PHY_AER_REG_ADDR_LANE_NUM(addr);     /* 07FF0000 >> 16 */
    cl45_addr    = SOC_PHY_CLAUSE45_ADDR(cl45Devid,0xffde);

    if (cl45_ln_no) /* write the lane number to the AER only if nonzero */
      wcmod_cl45_write(pc,cl45_addr,cl45_ln_no);

    wcmod_cl45_read(pc,cl45_addr,&data16);

    if (cl45_ln_no) /* restore back the lane number to 0 */
      wcmod_cl45_write(pc,cl45_addr,0);

  } else if (pc->mdio_type == WCMOD_MDIO_CL22) {
    phy_reg_aer  = PHY_AER_REG_ADDR_AER(addr);    /* upper 16 bits */
    phy_reg_blk  = PHY_AER_REG_ADDR_BLK(addr);    /* 12 bits mask=0xfff0 */
    phy_reg_addr = PHY_AER_REG_ADDR_REGAD(addr);  /* 5 bits {15,3,2,1,0} */

    CHK_RET_VAL_FUNC(wcmod_cl22_write(pc, 0x1f, 0xffd0));
    CHK_RET_VAL_FUNC(wcmod_cl22_write(pc, 0x1e, phy_reg_aer));
    CHK_RET_VAL_FUNC(wcmod_cl22_write(pc, 0x1f, phy_reg_blk)); /* Map block */
    CHK_RET_VAL_FUNC(wcmod_cl22_read(pc, phy_reg_addr, &data16)); /* Read reg */
  }

  if (pc->verbosity > 1)
    printf("%-22s: unit:%d port:%d addr:0x%x data:0x%x\n",
                                  __func__,pc->unit,pc->port,phy_reg_addr,data16);
  *phy_data = data16;
  return SOC_E_NONE;
}

/*!
\brief Tier1 API for register write.  Resolves cl22/cl45 + dxgxs considerations

\param unit Passed in. Need not be used.

\param pc Pointer to wcmod_st

\param a 32 bit address

\param d 16 bit data to be written.

\returns  0 on success. Non-zero on fail.

\details  
*/
int wcmod_reg_aer_write(uint32 unit, wcmod_st *pc, uint32 a, uint16 d)
{
  uint32 phy_reg_aer, phy_reg_blk,phy_reg_addr, addr = a;
  int    cl45_devid, cl45_ln_no, cl45_addr;
  int    rv = 0;
  int data = d;

#if defined  (_SDK_WCMOD_)
  if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit)) {
      return rv;
  }
#endif

  if (pc->verbosity > 1)
    printf("%-22s: unit:%d port:%d addr:0x%x data:0x%x\n",
    						__func__,pc->unit,pc->port,a,d);

  if (pc->lane_num_ignore) {  /* if asked not to ignore */
    addr = addr & 0xf800ffff; /* kill the extension. i.e. always write to 0. */
  } else {
    if (pc->lane_select == WCMOD_LANE_BCST || pc->dxgxs == 3) {
      addr |= (pc->aer_bcst_ofs_strap) << 16;
    } else if (pc->dxgxs == 1 || pc->dxgxs == 2) {
      addr |= ((pc->aer_bcst_ofs_strap+pc->dxgxs) << 16);
    } else {
      addr |= ((pc->this_lane & 0x7) << 16);
    }
  }   
  
  if (pc->mdio_type == WCMOD_MDIO_CL45) {
    phy_reg_addr = PHY_AER_REG_ADDR_CL45_REGAD(addr);   /* 0000FFFF  */
    phy_reg_aer  = PHY_AER_REG_ADDR_AER(addr);          /*(FFFF0000 >> 16)*/
    cl45_devid   = PHY_AER_REG_ADDR_CL45_DEVID(addr);   /* F8000000 >> 27 */
    cl45_ln_no   = PHY_AER_REG_ADDR_LANE_NUM(addr);     /* 07FF0000 >> 16 */
    cl45_addr    = SOC_PHY_CLAUSE45_ADDR(cl45_devid,0xffde);

    if (cl45_ln_no) /* write the lane number to the AER only if nonzero */
      wcmod_cl45_write(pc,cl45_addr,cl45_ln_no);

    wcmod_cl45_write(pc, cl45_addr,data);

    if (cl45_ln_no) /* restore back the lane number to 0 */
      wcmod_cl45_write(pc,cl45_addr,0);
  } else if (pc->mdio_type == WCMOD_MDIO_CL22) {
    phy_reg_aer  = PHY_AER_REG_ADDR_AER(addr);
    phy_reg_blk  = PHY_AER_REG_ADDR_BLK(addr);
    phy_reg_addr = PHY_AER_REG_ADDR_REGAD(addr);

  if (pc->verbosity > 1)
    printf("%-22s: unit:%d port:%d phy_ad:%d addr:0x%08x data:0x%04x\n",
                               __func__,pc->unit,pc->port,pc->phy_ad,addr,data);
    rv = wcmod_cl22_write(pc, 0x1f, 0xffd0);
    rv = wcmod_cl22_write(pc, 0x1e, phy_reg_aer);
    rv = wcmod_cl22_write(pc, 0x1f, phy_reg_blk); /* map block */
    rv = wcmod_cl22_write(pc, phy_reg_addr, data); /* write reg */
  }
  else {
    printf ("%-22s ERROR: Bad mdio_type:%d\n",__func__, pc->mdio_type);
    return SOC_E_ERROR;
  }
  return rv;
}

/*!
\brief Tier1 API for register read-modify-write.  Resolves cl22/cl45 + dxgxs
considerations

\param unit Passed in. Need not be used.

\param pc Pointer to wcmod_st

\param a 32 bit address

\param d 16 bit data to be written.

\param m 16 bit data mask to control the read-modify-write sequence.

\returns  0 on success. Non-zero on fail.

\details 
*/
int wcmod_reg_aer_modify(uint32 unit, wcmod_st *pc,uint32 a,uint16 d,uint16 m)
{
  uint16  otmp;
  int rv=0;
  uint16 tmp_data=0;
  int    cl45_devid, cl45_ln_no, cl45_addr;
  int data = (d & 0xffff), mask = m;
  uint32 addr = a, raddr=a, waddr=a;
  int dxgxs   = pc->dxgxs;
  uint32 phy_reg_aer, phy_reg_blk,phy_reg_addr;

#if defined  (_SDK_WCMOD_)
  if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit)) {
      return rv;
  }
#endif

  /* set AER to broadcast two lanes, dxgxs=1 1st dxgxs, dxgxs=2 2nd  */
  if (pc->lane_num_ignore) {  /* if asked not to ignore */
    addr = addr & 0xf800ffff; /* kill the extension. i.e. always write to 0. */
    raddr = addr; waddr = addr;
  } else {
    if (pc->lane_select == WCMOD_LANE_BCST || pc->dxgxs == 3) {
      raddr = addr & 0xf800ffff; /* kill extension. i.e. always read from 0. */
      waddr = ((pc->aer_bcst_ofs_strap & 0x7ff) << 16) | addr;
    } else if (pc->dxgxs == 1 || pc->dxgxs == 2) {
      waddr = (((pc->aer_bcst_ofs_strap+dxgxs)&0x7ff) << 16) | addr;
      raddr = ((pc->this_lane & 0x3) << 16) | addr;
      if (pc->verbosity > 1)
        printf ("%-22s w/r:%x/%x strap:%x dx:%x ad:%x\n", __func__, waddr,raddr,
                                           pc->aer_bcst_ofs_strap, dxgxs, addr);
    } else {
      addr |= ((pc->this_lane & 0x7) << 16);
      raddr = addr; waddr = addr;
    }
  }
  if (pc->verbosity > 1)
    printf("%-22s:Pre Mod:addr:0x%08x->(w/r:%08x/%08x) data:0x%04x mask:0x%x\n",
                                            __func__,a,waddr,raddr, data, mask);

       if (pc->mdio_type == WCMOD_MDIO_CL45) {
    phy_reg_aer  = PHY_AER_REG_ADDR_AER(addr);
    phy_reg_addr = PHY_AER_REG_ADDR_CL45_REGAD(addr);
    cl45_devid = PHY_AER_REG_ADDR_CL45_DEVID(addr);
    cl45_ln_no   = PHY_AER_REG_ADDR_LANE_NUM(addr);     /* 07FF0000 >> 16 */
    cl45_addr    = SOC_PHY_CLAUSE45_ADDR(cl45_devid,0xffde);

    if (cl45_ln_no) /* write the lane number to the AER only if nonzero */
      wcmod_cl45_write(pc,cl45_addr,cl45_ln_no);

    /* read */
    wcmod_cl45_read(pc,cl45_addr,&tmp_data);

    /* modify */
    otmp     = tmp_data;
    tmp_data &= ~mask;
    tmp_data |= data;

    /* modify */
    if (otmp != tmp_data)
      wcmod_cl45_write(pc, cl45_addr,tmp_data);
    else
      printf ("%-22s Write skipped. No need to write\n", __func__);

    if (cl45_ln_no) /* restore back the lane number to 0 */
      wcmod_cl45_write(pc,cl45_addr,0);

  } else if (pc->mdio_type == WCMOD_MDIO_CL22) {
    phy_reg_aer  = PHY_AER_REG_ADDR_AER(raddr);    /* upper 16 bits */
    phy_reg_blk  = PHY_AER_REG_ADDR_BLK(raddr);    /* 12 bits mask=0xfff0 */
    phy_reg_addr = PHY_AER_REG_ADDR_REGAD(raddr);  /* 5 bits {15,3,2,1,0} */

    /*  read out value. */
    rv = wcmod_cl22_write(pc, 0x1f, 0xffd0);
    rv = wcmod_cl22_write(pc, 0x1e, phy_reg_aer);
    rv = wcmod_cl22_write(pc, 0x1f, phy_reg_blk); /* map block */
    rv = wcmod_cl22_read(pc, phy_reg_addr, &tmp_data); /* read reg */

    /* modify */
    data &= mask;
    tmp_data &= ~(mask);
    tmp_data |= data;

    /* write back */
    phy_reg_aer  = PHY_AER_REG_ADDR_AER(waddr);    /* upper 16 bits */
    phy_reg_blk  = PHY_AER_REG_ADDR_BLK(waddr);    /* 12 bits mask=0xfff0 */
    phy_reg_addr = PHY_AER_REG_ADDR_REGAD(waddr);  /* 5 bits {15,3,2,1,0} */
    rv = wcmod_cl22_write(pc, 0x1f, 0xffd0);
    rv = wcmod_cl22_write(pc, 0x1e, phy_reg_aer);
    rv = wcmod_cl22_write(pc, 0x1f, phy_reg_blk);
    rv = wcmod_cl22_write(pc, phy_reg_addr, tmp_data);
  }
  else {
    printf ("%-22s FATAL: Bad mdio_type:%d\n",__func__, pc->mdio_type);
    return SOC_E_ERROR;
  }
  if (pc->verbosity > 1)
    printf("%-22s:Post Mod: phy_ad:%d addr:0x%08x data:0x%04x mask:0x%x\n",
                                           __func__,pc->phy_ad,addr,data, mask);
  return rv;
}

/*!
\brief Check for timeout. Maintains down counter

\param to

\returns : 1 only if timeout. Else 0.

\details 
*/
int wcmod_timeout_check(soc_timeout_t *to)
{
#if defined (_SDK_WCMOD_)
  /* to be implemented */
  return SOC_E_NONE;
#endif

}

/*!
\brief Internal function. Platform dependant microsecond delay

\param usec delay in microseconds.

\returns : Nothing.

\details 
*/
#ifndef _SDK_WCMOD_
int sal_udelay(uint32 usec)
{
  printf ("%-22s FATAL: unimplemented function\n",__func__);
  return SOC_E_ERROR;
}
#endif /*_SDK_WCMOD_ */
