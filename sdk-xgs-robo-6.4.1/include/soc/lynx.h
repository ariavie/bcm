/*
 * $Id: lynx.h,v 1.2 Broadcom SDK $
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
 * File:        lynx.h
 */

#ifndef _SOC_LYNX_H_
#define _SOC_LYNX_H_

#include <soc/drv.h>

extern int soc_lynx_misc_init(int);
extern int soc_lynx_mmu_init(int);
extern int soc_lynx_age_timer_get(int, int *, int *);
extern int soc_lynx_age_timer_max_get(int, int *);
extern int soc_lynx_age_timer_set(int, int, int);
extern int soc_lynx_stat_get(int, soc_port_t, int, uint64*);

extern int soc_lynx_xpic_parity_error(int unit, int blk);
extern int soc_lynx_arl_parity_error(int unit, int blk);
extern int soc_lynx_mmu_parity_error(int unit, int blk);

extern soc_functions_t soc_lynx_drv_funs;

#define SOC_LYNX_PARITY_XPIC_EGR_IPMCDATA   0x00000001
#define SOC_LYNX_PARITY_XPIC_EGR_SPVLAN     0x00000002
#define SOC_LYNX_PARITY_XPIC_EGR_IPMCLS     0x00000004
#define SOC_LYNX_PARITY_XPIC_EGR_IPMCMS     0x00000008
#define SOC_LYNX_PARITY_XPIC_ING_CELLBUF    0x00000010
#define SOC_LYNX_PARITY_XPIC_VPLS_TABLE     0x00000020
#define SOC_LYNX_PARITY_XPIC_FFP_METER1     0x00000040
#define SOC_LYNX_PARITY_XPIC_FFP_METER0     0x00000080
#define SOC_LYNX_PARITY_XPIC_FFP_IRULE1     0x00000100
#define SOC_LYNX_PARITY_XPIC_FFP_IRULE0     0x00000200
#define SOC_LYNX_PARITY_XPIC_VLANSTG        0x00000400 /* 74 */
#define SOC_LYNX_PARITY_XPIC_MASK_73        0x000003ff
#define SOC_LYNX_PARITY_XPIC_MASK_74        0x000007ff
#define SOC_LYNX_PARITY_XPIC_CLEAR          \
        (SOC_LYNX_PARITY_XPIC_EGR_SPVLAN | SOC_LYNX_PARITY_XPIC_VPLS_TABLE)

#define SOC_LYNX_PARITY_ARL_L2RAM_MASK      0x000000ff
#define SOC_LYNX_PARITY_ARL_L2_VALID        0x00000100
#define SOC_LYNX_PARITY_ARL_L2_STATIC       0x00000200
#define SOC_LYNX_PARITY_ARL_L3RAM_MASK      0x0003fc00
#define SOC_LYNX_PARITY_ARL_L3_VALID        0x00040000
#define SOC_LYNX_PARITY_ARL_QVLAN           0x00080000
#define SOC_LYNX_PARITY_ARL_STG             0x00100000
#define SOC_LYNX_PARITY_ARL_SPF             0x00200000
#define SOC_LYNX_PARITY_ARL_L2MC            0x00400000
#define SOC_LYNX_PARITY_ARL_DEFIP           0x00800000
#define SOC_LYNX_PARITY_ARL_L3IF            0x01000000
#define SOC_LYNX_PARITY_ARL_IPMC            0x02000000
#define SOC_LYNX_PARITY_ARL_MASK            0x03ffffff

#define SOC_LYNX_PARITY_ARL_L3RAM_SHIFT     10

#define SOC_LYNX_PARITY_MMU_XQ0             0x00000001
#define SOC_LYNX_PARITY_MMU_XQ1             0x00000002
#define SOC_LYNX_PARITY_MMU_XQ2             0x00000004
#define SOC_LYNX_PARITY_MMU_CFAP            0x00000008
#define SOC_LYNX_PARITY_MMU_CCP             0x00000010
#define SOC_LYNX_PARITY_MMU_MASK            0x0000001f

#endif	/* !_SOC_LYNX_H_ */
