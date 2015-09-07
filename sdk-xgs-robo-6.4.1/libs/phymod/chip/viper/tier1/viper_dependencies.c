/*
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
 *  $Id: 07c386907bef500e8c8892225c388b6dda82b3f4 $
*/  

#include <phymod/phymod.h>
#include "viper_common.h"
#include "viper_err_code.h"
#include <phymod/acc/phymod_tsc_iblk.h>

err_code_t viper_delay_us(uint32_t delay_us){
	PHYMOD_USLEEP(delay_us);
	return ( 0 );
}

err_code_t viper_delay_ns(uint16_t delay_ns) {
	uint32_t delay;
	delay = delay_ns / 1000; 
	if (!delay ) {
		delay = 1;
	}
	PHYMOD_USLEEP(delay);
	return ( 0 );
}

uint8_t viper_get_lane(const phymod_access_t *pa) {
    if (pa->lane == 0x1) {
	    return ( 0 );
    } else if (pa->lane == 0x2) {
	    return ( 1 );
    } else if (pa->lane == 0x4) {
	    return ( 2 );
    } else if (pa->lane == 0x8) {
	    return ( 3 );
    } else {
	    return ( 0 );
    }
}

void viper_pmd_mwr_reg (PHYMOD_ST *pa, uint16_t address, 
                        uint16_t mask, uint8_t lsb, uint16_t val) {

    PHYMOD_ST pa_copy;	
    uint32_t  mymask  = (uint32_t)mask;
    uint32_t  i, data = ((mymask << 16) & 0xffff0000) | val << lsb;

    PHYMOD_MEMCPY((void*)&pa_copy, (void*)pa, sizeof(phymod_access_t));
    for(i=1; i <= 0x8; i = i << 1) {
        if ( i & pa->lane ) {
            pa_copy.lane = i;
            phymod_tsc_iblk_write(&pa_copy, 
                   (PHYMOD_REG_ACC_TSC_IBLK | 0x10000 | (uint32_t) address), data);
        }
    }
}


uint16_t viper_pmd_rd_reg(PHYMOD_ST *pa, uint16_t address){

	uint32_t data;
	phymod_tsc_iblk_read(pa, (PHYMOD_REG_ACC_TSC_IBLK | 0x10000 
                              | (uint32_t) address), &data);
	data = data & 0xffff; 
	return ( (uint16_t)data);
}


err_code_t viper_pmd_rdt_reg(PHYMOD_ST *pa, uint16_t address, uint16_t *val) {
	uint32_t data;
	phymod_tsc_iblk_read(pa, (PHYMOD_REG_ACC_TSC_IBLK | 0x10000 | 
                                        (uint32_t) address), &data);
	data = data & 0xffff; 
	*val = (uint16_t)data;
	return ( 0 );
}

void viper_pmd_wr_reg(const phymod_access_t *pa, uint16_t address, uint16_t val){
	uint32_t data = 0xffff & val;
	phymod_tsc_iblk_write(pa, (PHYMOD_REG_ACC_TSC_IBLK | 0x10000 
                                     | (uint32_t) address), data);
}
