/*
 * HND SiliconBackplane MIPS core software interface.
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
 * $Id: hndmips.h,v 1.2 Broadcom SDK $
 */

#ifndef _hndmips_h_
#define _hndmips_h_

extern void si_mips_init(si_t *sih, uint shirq_map_base);
extern bool si_mips_setclock(si_t *sih, uint32 mipsclock, uint32 sbclock, uint32 pciclock);
extern void enable_pfc(uint32 mode);
extern uint32 si_memc_get_ncdl(si_t *sih);

#if defined(BCMINTERNAL) || defined (BCMPERFSTATS)
/* enable counting - exclusive version. Only one set of counters allowed at a time */
extern void hndmips_perf_cyclecount_enable(void);
extern void hndmips_perf_instrcount_enable(void);
extern void hndmips_perf_icachecount_enable(void);
extern void hndmips_perf_dcachecount_enable(void);
/* start and stop counting */
#define hndmips_perf_start01() \
	MTC0(C0_PERFORMANCE, 4, MFC0(C0_PERFORMANCE, 4) | 0x80008000)
#define hndmips_perf_stop01() \
	MTC0(C0_PERFORMANCE, 4, MFC0(C0_PERFORMANCE, 4) & ~0x80008000)
/* retrieve coutners - counters *decrement* */
#define hndmips_perf_read0() -(long)(MFC0(C0_PERFORMANCE, 0))
#define hndmips_perf_read1() -(long)(MFC0(C0_PERFORMANCE, 1))
#define hndmips_perf_read2() -(long)(MFC0(C0_PERFORMANCE, 2))
/* enable counting - modular version. Each counters can be enabled separately. */
extern void hndmips_perf_icache_hit_enable(void);
extern void hndmips_perf_icache_miss_enable(void);
extern uint32 hndmips_perf_read_instrcount(void);
extern uint32 hndmips_perf_read_cache_miss(void);
extern uint32 hndmips_perf_read_cache_hit(void);
#endif /*  defined(BCMINTERNAL) || defined (BCMPERFSTATS) */

#endif /* _hndmips_h_ */
