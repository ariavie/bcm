/*
 * $Id: phy_mac_ctrl.c,v 1.5 Broadcom SDK $
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
 */

int phy_mac_not_empty;
#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
#include <buser_sal.h>
#include "phy_mac_ctrl.h"

extern phy_mac_drv_t phy_unimac_drv;
#if 0
extern phy_mac_drv_t phy_bigmac_drv;
#endif
extern phy_mac_drv_t phy_xmac_drv;

/*
 * Allocate mac control block and attach the MAC driver.
 */
phy_mac_ctrl_t* 
phy_mac_driver_attach(blmi_dev_addr_t dev_addr,
                         phy_mactype_t  mac_type,
                         blmi_dev_io_f   mmi_cbf)
{
    phy_mac_ctrl_t *mmc;
    
    mmc = sal_alloc(sizeof(phy_mac_ctrl_t), 
                               "bmac_driver");
    if (mmc == NULL) {
        return NULL;
    }

    if (mac_type == PHY_MAC_CORE_UNIMAC) {
        sal_memcpy(&mmc->mac_drv, &phy_unimac_drv, sizeof(phy_mac_drv_t));
#if 0
    } else if (mac_type == PHY_MAC_CORE_BIGMAC) {
        sal_memcpy(&mmc->mac_drv, &phy_bigmac_drv, sizeof(phy_mac_drv_t));
#endif
    } else if (mac_type == PHY_MAC_CORE_XMAC) {
        sal_memcpy(&mmc->mac_drv, &phy_xmac_drv, sizeof(phy_mac_drv_t));
    } else {
        sal_free(mmc);
        mmc = NULL;
    }

    if (mmc) {
        mmc->devio_f = mmi_cbf;
        mmc->dev_addr = dev_addr;
        mmc->flag = 0;
    }

    return mmc;
}

int phy_mac_driver_detach(phy_mac_ctrl_t *mmc)
{
    sal_free(mmc);
    return 0;
}


/*
 *  Set config flag for the mac driver init
*/
void 
phy_mac_driver_config(phy_mac_ctrl_t *mmc,
                         phy_mactype_t  mac_type,
                         uint32 flag)
{
    if (mmc == NULL) {
        return;
    }

    mmc->flag = flag;
    return;
}

#endif /* defined(INCLUDE_FCMAP) || defined (INCLUDE_MACSEC) */
