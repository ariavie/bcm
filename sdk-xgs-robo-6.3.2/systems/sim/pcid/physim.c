/* 
 * $Id: physim.c 1.7 Broadcom SDK $
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
 * The part of PCID that simulates a PHY
 *
 * Requires:
 *     
 * Provides:
 *     soc_internal_miim_op
 */

#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/time.h>

#include <soc/cmic.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <sal/appl/io.h>
#include <bde/pli/verinet.h>

#include "pcid.h"
#include "cmicsim.h"

#define PHY_ID_COUNT		256
#define PHY_ADDR_COUNT		32

static uint16 phy_resetval_5228[PHY_ADDR_COUNT] = {
    0x3000, 0x7809, 0x0040, 0x61d4, 0x01e1, 0x0000, 0x0004, 0x2001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x1000, 0x0001, 0x0000, 0x0000, 0x0200, 0x0600, 0x0100, 0x0000,
    0x003c, 0x0002, 0x1f00, 0x009a, 0x002c, 0x0000, 0x0000, 0x000b,
};

static uint16 phy_resetval_5402[PHY_ADDR_COUNT] = {
    0x1100, 0x7949, 0x0020, 0x6061, 0x0061, 0x0000, 0x0004, 0x2001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3000,
    0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0420, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
};

static uint16 phy_regs[PHY_ID_COUNT][PHY_ADDR_COUNT];

static void
phy_reset(uint8 phy_id)
{
    /* This only handles (kind of) the 24+2 switches */

    sal_memcpy(&phy_regs[phy_id],
	       (phy_id < 25) ? phy_resetval_5228 : phy_resetval_5402,
	       sizeof (phy_regs[phy_id]));
}

/*
 * soc_internal_miim_op
 *
 * Look at miim parameters and make up a response.
 */

void
soc_internal_miim_op(pcid_info_t *pcid_info, int read)
{
    uint8           phy_id;
    uint8           phy_addr;
    uint16          phy_wr_data;

    phy_addr = PCIM(pcid_info, CMIC_MIIM_PARAM) >> 24 & 0x1f;
    phy_id = PCIM(pcid_info, CMIC_MIIM_PARAM) >> 16 & 0xff;

    /* Simulate power-on reset */

    if (phy_regs[phy_id][2] == 0 && phy_regs[phy_id][3] == 0) {
	phy_reset(phy_id);
    }

    if (read) {
	debugk(DK_VERBOSE,
	       "Responding to MIIM read: id=0x%02x addr=0x%02x data=0x%04x\n",
	       phy_id, phy_addr, phy_regs[phy_id][phy_addr]);
	PCIM(pcid_info, CMIC_MIIM_READ_DATA) = phy_regs[phy_id][phy_addr];
    } else {
	phy_wr_data = PCIM(pcid_info, CMIC_MIIM_PARAM) & 0xffff;
	debugk(DK_VERBOSE,
	       "Responding to MIIM write: "
	       "id=0x%02x addr=0x%02x data=0x%04x\n",
	       phy_id, phy_addr, phy_wr_data);
	if (phy_addr == 0 && (phy_wr_data & 0x8000) != 0) {
	    phy_reset(phy_id);
	} else {
	    phy_regs[phy_id][phy_addr] = phy_wr_data;
	}
    }

    PCIM(pcid_info, CMIC_SCHAN_CTRL) |= SC_MIIM_OP_DONE_TST;
}
