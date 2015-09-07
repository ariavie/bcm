/*
 * $Id: mmuerr.h,v 1.2 Broadcom SDK $
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

#ifndef _SOC_MMUERR_H
#define _SOC_MMUERR_H

extern int soc_mmu_error_init(int unit);
extern int soc_mmu_error_done(int unit);
extern int soc_mmu_error_all(int unit);

/* MMU Error Status register decodings */
#define SOC_ERR_HAND_MMU_XQ_PARITY   0x00000001   /* Notify */
#define SOC_ERR_HAND_MMU_LLA_PARITY  0x00000002   /* Fatal */
#define SOC_ERR_HAND_MMU_PP_SBE      0x00000004   /* Notify */
#define SOC_ERR_HAND_MMU_PP_DBE      0x00000008   /* Notify */
#define SOC_ERR_HAND_MMU_UPK_PARITY  0x00000010   /* Fatal */
#define SOC_ERR_HAND_MMU_ING_PARITY  0x00000020   /* Fatal */
#define SOC_ERR_HAND_MMU_EGR_PARITY  0x00000040   /* Fatal */
#define SOC_ERR_HAND_MMU_ALL         0x0000003f   /* Mask */
#define SOC_ERR_HAND_MMU_ALL_H15     0x0000007f   /* Mask */

#define SOC_MMU_ERR_CLEAR_ALL        0x000001ff   /* Mask */

#define SOC_MMU_ERR_ING_SHIFT        21
#define SOC_MMU_ERR_ING_MASK         0x7

#define SOC_MMU_ERR_ING_ING_BUF      1
#define SOC_MMU_ERR_ING_ING_VLAN     2
#define SOC_MMU_ERR_ING_ING_MC       4

/* Statistics tracking for each port's MMU */
typedef struct soc_mmu_error_s {
    int pp_sbe_blocks_init;
    int pp_dbe_blocks_init;
    int pp_sbe_cells_init;
    int pp_dbe_cells_init;
    int pp_sbe_blocks;
    int pp_dbe_blocks;
    int pp_sbe_cells;
    int pp_dbe_cells;
    int xq_parity;
    int lla_parity;
    int upk_parity;
    int ing_parity;
    int egr_parity;
} soc_mmu_error_t;

#endif  /* !_SOC_MMUERR_H */


