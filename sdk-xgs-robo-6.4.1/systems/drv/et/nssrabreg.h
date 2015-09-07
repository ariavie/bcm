/*
 * $Id: nssrabreg.h,v 1.2 Broadcom SDK $
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
 * BCM5301X SRAB register definition
 */


#ifndef _nssrab_core_h_
#define _nssrab_core_h_




#ifndef PAD
#define     _PADLINE(line)    pad ## line
#define     _XSTR(line)     _PADLINE(line)
#define     PAD     XSTR(__LINE__)
#endif


typedef volatile struct _nssrabregs {
    uint32 	PAD[11];
    uint32 	chipcommonb_srab_cmdstat;
    uint32 	chipcommonb_srab_wdh;
    uint32 	chipcommonb_srab_wdl;
    uint32 	chipcommonb_srab_rdh;
    uint32 	chipcommonb_srab_rdl;
    uint32 	chipcommonb_srab_sw_if;
    uint32 	chipcommonb_srab_sw_intr;
} nssrabregs_t;

/*  chipcommonb_srab_cmdstat offset0x702c  */
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_GORDYN_SHIFT	0
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_GORDYN_MASK	0x1       
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_WRITE_SHIFT	1
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_WRITE_MASK	0x2       
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_RST_SHIFT	2
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_RST_MASK	0x4       
#define 	CHIPCOMMONB_SRAB_CMDSTAT_RESERVED_SHIFT	3
#define 	CHIPCOMMONB_SRAB_CMDSTAT_RESERVED_MASK	0xfff8    
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_OFFSET_SHIFT	16
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_OFFSET_MASK	0xff0000  
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_PAGE_SHIFT	24
#define 	CHIPCOMMONB_SRAB_CMDSTAT_SRA_PAGE_MASK	0xff000000

/*  chipcommonb_srab_wdh offset0x7030  */
#define 	CHIPCOMMONB_SRAB_WDH_SR_WDATA_H_SHIFT	0
#define 	CHIPCOMMONB_SRAB_WDH_SR_WDATA_H_MASK	0xffffffff

/*  chipcommonb_srab_wdl offset0x7034  */
#define 	CHIPCOMMONB_SRAB_WDL_SR_WDATA_L_SHIFT	0
#define 	CHIPCOMMONB_SRAB_WDL_SR_WDATA_L_MASK	0xffffffff

/*  chipcommonb_srab_rdh offset0x7038  */
#define 	CHIPCOMMONB_SRAB_RDH_SR_RDATA_H_SHIFT	0
#define 	CHIPCOMMONB_SRAB_RDH_SR_RDATA_H_MASK	0xffffffff

/*  chipcommonb_srab_rdl offset0x703c  */
#define 	CHIPCOMMONB_SRAB_RDL_SR_RDATA_L_SHIFT	0
#define 	CHIPCOMMONB_SRAB_RDL_SR_RDATA_L_MASK	0xffffffff

/*  chipcommonb_srab_sw_if offset0x7040  */
#define 	CHIPCOMMONB_SRAB_SW_IF_SPIMUX_ARM_SEL_SHIFT	0
#define 	CHIPCOMMONB_SRAB_SW_IF_SPIMUX_ARM_SEL_MASK	0x1       
#define 	CHIPCOMMONB_SRAB_SW_IF_HOST_INTR_SHIFT	1
#define 	CHIPCOMMONB_SRAB_SW_IF_HOST_INTR_MASK	0x2       
#define 	CHIPCOMMONB_SRAB_SW_IF_SPI_BUSY_SHIFT	2
#define 	CHIPCOMMONB_SRAB_SW_IF_SPI_BUSY_MASK	0x4       
#define 	CHIPCOMMONB_SRAB_SW_IF_RCAREQ_SHIFT	3
#define 	CHIPCOMMONB_SRAB_SW_IF_RCAREQ_MASK	0x8       
#define 	CHIPCOMMONB_SRAB_SW_IF_RCAGNT_SHIFT	4
#define 	CHIPCOMMONB_SRAB_SW_IF_RCAGNT_MASK	0x10      
#define 	CHIPCOMMONB_SRAB_SW_IF_SOC_BOOT_DONE_SHIFT	5
#define 	CHIPCOMMONB_SRAB_SW_IF_SOC_BOOT_DONE_MASK	0x20      
#define 	CHIPCOMMONB_SRAB_SW_IF_SW_INIT_DONE_SHIFT	6
#define 	CHIPCOMMONB_SRAB_SW_IF_SW_INIT_DONE_MASK	0x40      
#define 	CHIPCOMMONB_SRAB_SW_IF_RESERVED_1_SHIFT	7
#define 	CHIPCOMMONB_SRAB_SW_IF_RESERVED_1_MASK	0x80      
#define 	CHIPCOMMONB_SRAB_SW_IF_OTP_CTRL_SHIFT	8
#define 	CHIPCOMMONB_SRAB_SW_IF_OTP_CTRL_MASK	0xff00    
#define 	CHIPCOMMONB_SRAB_SW_IF_RESERVED_SHIFT	16
#define 	CHIPCOMMONB_SRAB_SW_IF_RESERVED_MASK	0xffff0000

/*  chipcommonb_srab_sw_intr offset0x7044  */
#define 	CHIPCOMMONB_SRAB_SW_INTR_SWITCH_INTR_CLR_SHIFT	0
#define 	CHIPCOMMONB_SRAB_SW_INTR_SWITCH_INTR_CLR_MASK	0xffffffff



#endif /* _nssrab_core_h_ */
