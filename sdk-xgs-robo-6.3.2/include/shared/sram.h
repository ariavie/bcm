/*
 * $Id: sram.h 1.3 Broadcom SDK $
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
 * File:        sram.h
 */
#ifndef _SHR_SRAM_H
#define _SHR_SRAM_H

/* External DDR2_SRAM interface */
#define _SHR_EXT_SRAM_CSE_MODE     0
#define _SHR_EXT_SRAM_HSE_MODE     1

typedef struct _shr_ext_sram_entry_s {
    int    addr0;       /* Address register for data0 (always valid) */
    int    addr1;       /* Address register for data1 (-1 for invalid) */
    uint32 data0[8];    /* Data register 0 */
    uint32 data1[8];    /* Data register 1 */
    int wdoebr;         /* wdoeb_rise */
    int wdoebf;         /* wdoeb_fall */
    int wdmr;           /* write data mask rise */
    int wdmf;           /* write data mask fall */
    int rdmr;           /* read data mask rise */
    int rdmf;           /* read data mask fall */
    int test_mode;      /* 00:WW, 01:RR, 10:WR 11:WW-RR */
    int adr_mode;       /* 00:ALT_A0A1, 01:INC1, 10:INC2, 11:ONLY_A0 */
    int latency;        /* 0:read data in 8 mclks, 1:7 mclks */
    int em_if_type;     /* 1: ext mem if type qdr2, 0: ddr2 type */
    int em_fall_rise;   /* 1: use fall_rise for compare, 0: use rise_fall for compare */
    int w2r_nops;       /* num nops between write to read - for ddr2 */
    int r2w_nops;       /* num nops between read to write - for ddr2 */
    int err_cnt;        /* Error count */
    int err_bmp;        /* Error bit map */
    uint32 err_addr;    /* Error address */
    uint32 err_data[8]; /* Error data */
} _shr_ext_sram_entry_t;

#endif /* _SHR_SRAM_H */
