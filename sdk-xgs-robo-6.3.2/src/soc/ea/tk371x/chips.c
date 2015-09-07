/*
 * $Id: chips.c 1.2 Broadcom SDK $
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
#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/ea/tk371x/TkDefs.h>

int
soc_features_tk371x_a0(int unit, soc_feature_t feature)
{
   switch (feature) {
    case soc_feature_field:
        return TRUE;
    default:
        return FALSE;
    }
}

static soc_block_info_t soc_blocks_tk371x_a0[] = {
	{ SOC_EA_BLK_PON,	0,	0,	3	},	/* 0 Pon */
	{ SOC_EA_BLK_GE,	0,	0,	0	},	/* 1 Ge */
	{ SOC_EA_BLK_FE,	1,	0,	1	},	/* 2 Fe */
	{ SOC_EA_BLK_LLID,	2,	0,	2	},	/* 3 LLID */
	{ -1,		        -1, -1,	-1	}	/* end */
};

static soc_port_info_t soc_ports_tk371x_a0[] = {
	{ 0,      0   },	/* 0 Pon.0 */
	{ 1,      1   },	/* 1 Ge.0 */
	{ 2,      2   },	/* 2 FE.0 */
	{ 3,      3   },	/* 3 Llid.0 */
	{ 3,      3   },	/* 4 Llid.1 */
	{ 3,      3   },	/* 5 Llid.2 */
	{ 3,      3   },	/* 6 Llid.3 */
	{ 3,      3   },	/* 7 Llid.4 */
	{ 3,      3   },	/* 8 Llid.5 */
	{ 3,      3   },	/* 9 Llid.6 */
	{ 3,      3   },	/* 10 Llid.7 */
	{ -1,	 -1	  }	/* end */
};

soc_driver_t soc_driver_tk371x_a0 = {
	/* type           */    SOC_CHIP_TK371X_A0,
	/* chip_string    */    "TK371X",
	/* origin         */	" Not located in Server Need Verified",
	/* pci_vendor     */	BROADCOM_VENDOR_ID,
	/* pci_device     */	TK371X_DEVICE_ID,
	/* pci_revision   */	TK371X_A0_REV_ID,
	/* num_cos        */	0,
	/* reg_info       */	NULL,
        /* reg_above_64_info */ NULL,
	/* reg_array_info */    NULL,
        /* mem_info       */	NULL,
	/* soc_mem_t      */	NULL,
	/* mem_array_info */    NULL,
	/* block_info     */   	soc_blocks_tk371x_a0,
	/* port_info      */	soc_ports_tk371x_a0,
	/* counter_maps   */	NULL,
        /* features       */	       soc_features_tk371x_a0,
	/* init           */	       NULL,
	/* services       */	       NULL
};  /* soc_driver */
