/*
 * $Id: phyacd.c 1.6 Broadcom SDK $
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
 * File:        phyacd.c
 * Purpose:     PHY ACD routines
 */

#include <sal/core/thread.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/phy/phyctrl.h>
#include "phyreg.h"

#define READ_PHY_EXP_REG(_unit, _phy_ctrl, reg, _val) \
            phy_reg_ge_read((_unit), (_phy_ctrl), 0x00, 0x0f00 | ((reg) & 0xff), 0x15, (_val))
#define WRITE_PHY_EXP_REG(_unit, _phy_ctrl, reg, _val) \
            phy_reg_ge_write((_unit), (_phy_ctrl), 0x00, 0x0f00 | ((reg) & 0xff), \
                               0x15, (_val))
#define MODIFY_PHY_EXP_REG(_unit, _phy_ctrl, reg, _val, _mask) \
            phy_reg_ge_modify((_unit), (_phy_ctrl), 0x00, 0x0f00 | ((reg) & 0xff), \
                                0x15, (_val), (_mask))

int
phy_acd_cable_diag_init(int unit, soc_port_t port)
{
    phy_ctrl_t  *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

#if 0
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0x00, 0x9140)); /* Reset chip */
#endif

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0x90, 0x0000)); /* disable BroadR-Reach */
                                                                      
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xC7, 0xA01A)); /*frct_i_2 = 240, frct_i_1 = 4 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xC8, 0x0000)); /*th_silent = 1 for 54382 ADC */

    /*modified silent detection to last at least 200us */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xC9, 0x00EF)); /*silent_c_th = 13, block_width_i = 9 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xCC, 0x200)); /*temp using 1 probe */

    /*changed for 54780B0 - these values are already defaulted */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xCE, 0x4000));  /*lp_drop_wait = 1, lp_safe_time = 5 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xCF, 0x3000));    /*disable both types of random starts */

    /*leave it to run time script - don't do it here - disable/enable ACD clocks */

    /*------- page 01 --------------*/

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE0, 0x0010));     /*set th[1] and th[0]=24 value */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE1, 0xD0D));          /*set th[3]  th[2] value */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE2, 0x0));            /*restore exp. E2 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE3, 0x1000)); /*increase ppw[1] to 3, keep ppw[0] To 3 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE4, 0x0)); /*default ppw[3] to 6 and default ppw[2] to 4*/

    /*changed for 54780B0 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE7, 0xA0)); /*lower th_spike to 5 in order to detect link
                                                     pulses */
    /*---------------------------------------------------------------------------------*/
    /*--- Now load values --- */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xEF, 0x409F));         /*write to shadow page 01 word E7, E4, E3, E2, E1, E0 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xCD, 0x1000));         /*write strobe 1 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xCD, 0x0));            /*write strobe 0 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE0, 0x0));              /*restore exp. E0 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE1, 0x0));              /*restore exp. E1 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE2, 0x0));              /*restore exp. E2 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE3, 0x0));              /*restore exp. E3 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE4, 0x0));              /*restore exp. E4 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE7, 0x0));              /*restore exp. E7 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xEF, 0x0));              /*restore exp. EF */

    /*------- page 10 --------*/

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE0, 0x3600)); /*increase f_count_offset[1] to 32 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE1, 0xC)); /*default f_count_offset[3] to 29  increase f_count_offset[2] to 24 */

    /*  Modify f_count_offset[3] after 1st round DVT */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE1, 0x343A)); /*increase f_count_offset[3] to 41  increase f_count_offset[2] to 34 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE2, 0x0)); /*default f_count_offset[5] to 16  default f_count_offset[4] to 48 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE3, 0x0000));               /*set i_pga[0] to 0 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE4, 0x8000));               /*set pga_0 = 0x4f for ppw=3 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE5, 0x000C));               /* set pga_0 = 0x4f for ppw=3 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE7, 0x0000));               /*keep ramp_step to 1 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE9, 0x0400));               /*set pga_ramp_prd to 2 */


    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xED, 0x0000));               /*keep default hppga_bypass 0 */

    /*--- Now load values ---- */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xEF, 0xA1BF));           /*write to shadow page 10 word ED, E8, E7, E5, E4, E3, E2, E1, E0 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xCD, 0x1000));           /*write strobe 1 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xCD, 0x0));              /*write strobe 0 */

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE0, 0x0));              /*restore exp. E0 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE1, 0x0));              /*restore exp. E1 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE2, 0x0));              /*restore exp. E2 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE3, 0x0));              /*restore exp. E3 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE4, 0x0));              /*restore exp. E4 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE5, 0x0));              /*restore exp. E5 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE7, 0x0));              /*restore exp. E7 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE8, 0x0));              /*restore exp. E8 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xE9, 0x0));              /*restore exp. E9 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xED, 0x0));              /*restore exp. ED */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc, 0xEF, 0x0));              /*restore exp. EF */

    return SOC_E_NONE;
}

static int
_phy54680_read_exp_c_array(int unit, phy_ctrl_t  *pc, uint16 exp_c[])
{
    SOC_IF_ERROR_RETURN
        (READ_PHY_EXP_REG(unit, pc, 0xc0, &exp_c[0]));

    SOC_IF_ERROR_RETURN
        (READ_PHY_EXP_REG(unit, pc, 0xc1, &exp_c[1]));

    SOC_IF_ERROR_RETURN
        (READ_PHY_EXP_REG(unit, pc, 0xc2, &exp_c[2]));

    SOC_IF_ERROR_RETURN
        (READ_PHY_EXP_REG(unit, pc, 0xc3, &exp_c[3]));

    SOC_IF_ERROR_RETURN
        (READ_PHY_EXP_REG(unit, pc, 0xc4, &exp_c[4]));

    SOC_IF_ERROR_RETURN
        (READ_PHY_EXP_REG(unit, pc, 0xc5, &exp_c[5]));

    SOC_IF_ERROR_RETURN
        (READ_PHY_EXP_REG(unit, pc, 0xc6, &exp_c[6]));

    SOC_DEBUG_PRINT((DK_PHY, "u=%d p=%d EXP_C C0=%04x C1=%04x C2=%04x C3=%04x C4=%04x C5=%04x C6=%04x\n", 
       unit, pc->port, exp_c[0], exp_c[1], exp_c[2], exp_c[3], exp_c[4], exp_c[5], exp_c[6]));

    return SOC_E_NONE;
}

#define N_BUSY_CYCLE_WAIT   300

#define ACD_INVALID                   0x0000
#define ACD_NO_FAULT                  0x0001
#define ACD_OPEN                      0x0002
#define ACD_SHORT                     0x0003
#define ACD_PIN_SHORT_OR_XT           0x0004
#define ACD_FORCED                    0x0009

#define ERROR_ACD_BUSY_ON_DEMAND      0x0001
#define ERROR_ACD_BUSY_DURING_FLUSH   0x0002
#define ERROR_ACD_INVALID             0x0004
#define ERROR_ACD_NO_NEW_RESULT       0x0008

#define ERROR_ACD_LINK_NO_NEW_RESULT  0x0010
#define ERROR_ACD_UNSTABLE_LINK       0x0020
#define ERROR_ACD_NO_LINK             0x0040

static int 
_exec_ACD(int unit, phy_ctrl_t  *pc, int *error_flag, uint16 *fault, int *index, int *peak_amplitude)
{
    uint16 EXP_C[7];
    int i, new_result, local_error_flag;

    local_error_flag = *error_flag;

    /* Silence Coverity */
    EXP_C[0] = EXP_C[1] = EXP_C[2] = EXP_C[3] = EXP_C[4] = EXP_C[5] = EXP_C[6] = 0;

    SOC_IF_ERROR_RETURN
        (_phy54680_read_exp_c_array(unit, pc, EXP_C)); /* read all 7 registers */

    if ((EXP_C[0] & 0x0800)) {
        SOC_DEBUG_PRINT((DK_PHY,"ACD Engine still busy u=%d p=%d\n", unit, pc->port));
    }

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc,0xC0, 0x2000));    /*configure acd engine with tdr_gain = 1 */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc,0xC0, 0xA000));    /*start acd engine with tdr_gain = 1 */

    new_result = 0;

    for (i = 0; i < N_BUSY_CYCLE_WAIT; i++) {
        SOC_IF_ERROR_RETURN
            (_phy54680_read_exp_c_array(unit, pc, EXP_C)); /* read all 7 registers */
        if (EXP_C[0] & 0x4) {   /* there is new ACD result */
            new_result = 1;
        }
        if (!(EXP_C[0] & 0x0800)) {
            break;
        }
    }

    if (i >= N_BUSY_CYCLE_WAIT) {
        local_error_flag |= ERROR_ACD_BUSY_ON_DEMAND;
        SOC_DEBUG_PRINT((DK_PHY,"u=%d p=%d ERROR_ACD_BUSY_ON_DEMAND\n", unit, pc->port));
    }
    if (!new_result) {
        local_error_flag |= ERROR_ACD_NO_NEW_RESULT;
        SOC_DEBUG_PRINT((DK_PHY,"u=%d p=%d ERROR_ACD_NO_NEW_RESULT\n", unit, pc->port));
    }
    if (EXP_C[0] & 0x8) {
        local_error_flag |= ERROR_ACD_INVALID;
        SOC_DEBUG_PRINT((DK_PHY,"u=%d p=%d ERROR_ACD_INVALID\n", unit, pc->port));
    }

    *fault = EXP_C[1];
    *index = (EXP_C[6] + EXP_C[4]) / 2;
    *peak_amplitude = (EXP_C[5] + EXP_C[3]) / 2;
    return local_error_flag;
}

int
phy_acd_cable_diag(int unit, soc_port_t port,
            soc_port_cable_diag_t *status)
{
    phy_ctrl_t  *pc;
    uint16 fault;
    int error_flag, index, peak_amplitude;

    pc = EXT_PHY_SW_STATE(unit, port);
    error_flag = 0;

    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc,0xA4, 0x0008)); /*phase i_phase */
    SOC_IF_ERROR_RETURN
        (WRITE_PHY_EXP_REG(unit, pc,0xA4, 0x4008)); /*phase i_phase */

    error_flag = _exec_ACD(unit, pc, &error_flag, &fault, &index, &peak_amplitude);

    if (error_flag) {
        SOC_DEBUG_PRINT((DK_WARN,"u=%d p=%d cable diag test failed error_flag = 0x%04x\n", unit, pc->port, error_flag));
        return SOC_E_FAIL;
    }
    status->npairs = 1;
    status->fuzz_len = 0;

    SOC_DEBUG_PRINT((DK_PHY,"u=%d p=%d fault = %x\n", unit, pc->port, fault));

    if (fault == 0x2222) {
        status->state = SOC_PORT_CABLE_STATE_OPEN;
        status->pair_state[0] = SOC_PORT_CABLE_STATE_OPEN;
        status->pair_len[0] = (index * 1000) / 1325;
    } else if (fault == 0x3333) {
        status->state = SOC_PORT_CABLE_STATE_SHORT;
        status->pair_state[0] = SOC_PORT_CABLE_STATE_SHORT;
        status->pair_len[0] = (index * 1000) / 1325;
    } else {
        status->state = SOC_PORT_CABLE_STATE_OK;
        status->pair_state[0] = SOC_PORT_CABLE_STATE_OK;
        status->pair_len[0] = 0;
    }

    return SOC_E_NONE;
}
