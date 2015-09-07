/*
 * $Id: triumph2.h,v 1.27 Broadcom SDK $
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
 * File:        triumph2.h
 */

#ifndef _SOC_TRIUMPH2_H_
#define _SOC_TRIUMPH2_H_

#include <soc/drv.h>
#include <soc/mem.h>
#include <shared/sram.h>

typedef int (*soc_triumph2_oam_handler_t)(int unit, soc_field_t fault_field);

extern void soc_triumph2_stat_nack(int unit, int *fixed);
extern int soc_triumph2_misc_init(int);
extern int soc_triumph2_mmu_init(int);
extern int soc_triumph2_age_timer_get(int, int *, int *);
extern int soc_triumph2_age_timer_max_get(int, int *);
extern int soc_triumph2_age_timer_set(int, int, int);
extern void soc_triumph2_parity_error(void *unit_vp, void *d1, void *d2,
                                      void *d3, void *d4);
extern void soc_triumph2_mem_nack(void *unit_vp, void *addr_vp, void *d2,
                                     void *d3, void *d4);
extern void soc_triumph2_ser_fail(int unit);
extern int soc_triumph2_ser_mem_clear(int unit, soc_mem_t mem);
extern int _soc_triumph2_mem_parity_control(int unit, soc_mem_t mem,
                                            int copyno, int enable);
extern int _soc_triumph2_esm_process_intr_status(int unit);
extern void soc_triumph2_esm_intr_status(void *unit_vp, void *d1, void *d2,
                                         void *d3, void *d4);
extern int soc_triumph2_pipe_mem_clear(int unit);
extern soc_functions_t soc_triumph2_drv_funs;
extern void soc_triumph2_oam_handler_register(int unit,
											  soc_triumph2_oam_handler_t
											  handler);
extern int soc_tr2_xqport_mode_change(int unit, soc_port_t port, 
                                      soc_mac_mode_t mode);
extern void soc_triumph2_mem_config(int unit);

#if defined(SER_TR_TEST_SUPPORT)
extern int soc_tr2_ser_inject_error(int unit, uint32 flags, soc_mem_t mem,
									int pipeTarget, int block, int index);
extern int soc_tr2_ser_mem_test(int unit, soc_mem_t mem,
							    _soc_ser_test_t test_type, int cmd);
extern int soc_tr2_ser_test(int unit, _soc_ser_test_t test_type);
extern int _soc_triumph2_mem_nack_error_test(int unit,
											 _soc_ser_test_t test_type,
											 int *testErrors);
#endif

#endif    /* !_SOC_TRIUMPH2_H_ */
