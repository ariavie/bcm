/*
 * $Id: tucana.h 1.6 Broadcom SDK $
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
 * File:        tucana.h
 */

#ifndef _SOC_TUCANA_H_
#define _SOC_TUCANA_H_

#include <soc/drv.h>


extern int soc_tucana_misc_init(int);
extern int soc_tucana_mmu_init(int);
extern int soc_tucana_age_timer_get(int, int *, int *);
extern int soc_tucana_age_timer_max_get(int, int *);
extern int soc_tucana_age_timer_set(int, int, int);
extern int soc_tucana_stat_get(int, soc_port_t, int, uint64*);

extern int soc_tucana_arl_parity_error(int unit, int blk, int mem);
extern int soc_tucana_mmu_parity_error(int unit, int blk, uint32 info);
extern int soc_tucana_mmu_crc_error(int unit, uint32 addr);
extern int soc_tucana_pdll_lock_loss(int unit, uint32 info);
extern int soc_tucana_num_cells(int unit, int *num_cells);

#define SOC_TUCANA_CRC_ADDR_MASK       0xffff
#define SOC_TUCANA_CRC_MEM_BITS_SHIFT  16
#define SOC_TUCANA_CRC_MEM_BITS_MASK   0x3

#define SOC_TUCANA_PARITY_XQ           0x001
#define SOC_TUCANA_PARITY_EBUF_HG      0x002
#define SOC_TUCANA_PARITY_EBUF         0x004
#define SOC_TUCANA_PARITY_CELLASMB     0x008
#define SOC_TUCANA_PARITY_RELMGR       0x010
#define SOC_TUCANA_PARITY_CCC          0x020
#define SOC_TUCANA_PARITY_CCPTR        0x040
#define SOC_TUCANA_PARITY_CG1M1_FIFO   0x080
#define SOC_TUCANA_PARITY_CG1M0_FIFO   0x100
#define SOC_TUCANA_PARITY_CG0M1_FIFO   0x200
#define SOC_TUCANA_PARITY_CG0M0_FIFO   0x400

#define SOC_TUCANA_MEM_FAIL_DLL 0x2
#define SOC_TUCANA_MEM_FAIL_PLL 0x1

/* Needed for MMU cell calculations.  This is a fixed HW value. */
#define SOC_TUCANA_MMU_PTRS_PER_PTR_BLOCK       125

extern soc_functions_t soc_tucana_drv_funs;

/*
 * IPORT Mode Definitions.
 *     These indicate the polarity of the IPORT_MODE bit in the
 *     rule and mask tables.
 * SOC_IPORT_MODE_SPECIFIC  -  The port field may be compared like
 *     any other field.  It indicates a speicific port (or all ones
 *     to indicate all ports in the block).  Note that if the
 *     IPORT_MASK in the mask is 0, then the IPORT field must be
 *     zero for proper operation; the IPORT field is ignored in the
 *     match in this case.
 * SOC_IPORT_MODE_ARBITRARY_BMAP -   Arbitrary bitmaps supported;
 *     special comparison and sorting is required.
 */
#define SOC_IPORT_MODE_SPECIFIC           0
#define SOC_IPORT_MODE_ARBITRARY_BMAP     1

/* Values for the port type field in the port table in tucana */

#define SOC_TUCANA_PORT_TYPE_DEFAULT 0
#define SOC_TUCANA_PORT_TYPE_TRUNK 1
#define SOC_TUCANA_PORT_TYPE_STACKING 2
#define SOC_TUCANA_PORT_TYPE_RESERVED 3

#endif	/* !_SOC_TUCANA_H_ */
