/*
 * $Id: switch.c 1.15 Broadcom SDK $
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
 * File:       switch.c
 * Purpose:    Switch common APIs
 */

#ifdef _ERR_MSG_MODULE_NAME
  #error "_ERR_MSG_MODULE_NAME redefined"
#endif

#define _ERR_MSG_MODULE_NAME BCM_DBG_SWITCH

#include <soc/drv.h>
#include <bcm/switch.h>
#include <bcm/init.h>
#include <bcm/error.h>
#include <bcm_int/common/debug.h>

#ifdef WB_TEST_EPNI_ENABLE 
  #include <soc/dpp/ARAD/arad_reg_access.h>
  #include <soc/dpp/drv.h>
#endif

#ifdef INCLUDE_INTR 
#include <appl/dcmn/interrupts/interrupt_handler.h>
#if defined(BCM_ARAD_SUPPORT)
#include <appl/dcmn/rx_los/rx_los.h>
#endif
#endif 


#if (defined(BCM_DPP_SUPPORT) && defined(BCM_WARM_BOOT_SUPPORT))
#include <bcm_int/dpp/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */


/*
 * Function:
 *     bcm_switch_pkt_info_t_init
 * Description:
 *     Initialize an switch packet parameter strucutre
 * Parameters:
 *     pkt_info - pointer to switch packet parameter strucutre
 * Return: none
 */
void
bcm_switch_pkt_info_t_init(bcm_switch_pkt_info_t *pkt_info)
{
    sal_memset(pkt_info, 0, sizeof(bcm_switch_pkt_info_t));
}

/*
 * Function:
 *     bcm_switch_network_group_config_t_init
 * Description:
 *     Initialize a bcm_switch_network_group_config_t structure.
 * Parameters:
 *     config - pointer to switch network group config parameter strucutre
 * Return: none
 */
extern void bcm_switch_network_group_config_t_init(
    bcm_switch_network_group_config_t *config)
{
    sal_memset(config, 0, sizeof(bcm_switch_network_group_config_t));
}
/*WARMBOOT_SUPPORT && DPP*/


#if (defined(BCM_DPP_SUPPORT) && defined(BCM_WARM_BOOT_SUPPORT))


int
_bcm_switch_warmboot_reset_test(int unit)
{
    int rv = BCM_E_NONE;
	int warmboot_test_mode_enable;
    int no_wb_test;
    uint8 is_main;
    int test_started = 0;
#ifdef WB_TEST_EPNI_ENABLE
    uint32 fld_val;
    int device_id;
#endif

    BCM_INIT_FUNC_DEFS;

    is_main = (sal_thread_self() == sal_thread_main_get());

	/* Only main thread will trigger the test. Other threads (linkscan/counter) should not */
    if (is_main) {
        _bcm_dpp_switch_warmboot_test_mode_get(unit,&warmboot_test_mode_enable);
        _bcm_dpp_switch_warmboot_no_wb_test_get(unit,&no_wb_test);

        if ((warmboot_test_mode_enable == 1) && (!no_wb_test)) {
            BCM_DEBUG((BCM_DBG_VERBOSE), ("Unit:%d Starting warm boot test\n", unit));
    		/*waiting for warmboot test to finish to avoide recursive calling to _bcm_switch_warmboot_reset_test function*/
    		_bcm_dpp_switch_warmboot_test_mode_set(unit,0);
    		rv = bcm_switch_control_set(unit, bcmSwitchControlSync, 1);
            test_started = 1;
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during bcm_switch_control_set\n", unit));

#ifdef INCLUDE_INTR
#if defined(BCM_ARAD_SUPPORT)
    		rv = rx_los_unit_detach(unit);
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during appl deint\n", unit));   

    		rv = interrupt_handler_appl_deinit(unit);
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during appl deint\n", unit));   
#endif  
#endif  
            rv = bcm_detach(unit);
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during bcm detach\n", unit));

            rv = soc_deinit(unit);
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during soc deinit\n", unit));

            sal_sleep(1);

            /*warmboot on*/
            SOC_WARM_BOOT_START(unit);
		 
            rv = soc_reset_init(unit);
            
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during reset init\n", unit));

            rv = bcm_init(unit);
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during bcm init\n", unit));

            		 
            

#ifdef INCLUDE_INTR
#if defined(BCM_ARAD_SUPPORT)
            rv = interrupt_handler_appl_init(unit);
            BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during appl init\n", unit));

            /*warmboot off*/
            SOC_WARM_BOOT_DONE(unit);   
             
            {
                bcm_pbmp_t pbmp_default;
                BCM_PBMP_CLEAR(pbmp_default);

                rv = rx_los_unit_attach(unit, pbmp_default, 0);
                BCM_IF_ERR_EXIT_MSG(rv, ("Unit:%d warm boot test failed during appl init\n", unit));
            }
#endif  
            /*warmboot off*/
            SOC_WARM_BOOT_DONE(unit);

#ifdef WB_TEST_EPNI_ENABLE
            /* Set trigger of EPNI credits to other interfaces */
            fld_val = 0x7;
            device_id = soc_dpp_unit_to_sand_dev_id(unit);
            soc_reg32_set(device_id, EPNI_INIT_TXI_CONFIGr,REG_PORT_ANY, 0, fld_val);
#endif  
#endif
	   }
	}
exit:
    if ( (is_main) && (test_started)) {
        /*_bcm_dpp_switch_warmboot_test_mode_get(unit,&warmboot_test_mode_enable);
        if (warmboot_test_mode_enable == 0) {*/
        if (rv != BCM_E_NONE) {
            BCM_DEBUG((BCM_DBG_VERBOSE), ("Unit:%d Warm boot test failed\n", unit));
        }else {
            BCM_DEBUG((BCM_DBG_VERBOSE), ("Unit:%d Warm boot test finish successfully\n", unit));
        }
        /*enable warmboot test*/
        _bcm_dpp_switch_warmboot_test_mode_set(unit,1);
        /*}*/
    }
    BCM_FUNC_RETURN;
}

#endif /*(BCM_DPP_SUPPORT) && (BCM_WARM_BOOT_SUPPORT)*/
#undef _ERR_MSG_MODULE_NAME
