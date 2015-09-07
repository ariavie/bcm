/*
 * $Id: f8f8e40f6f8d486a70937cf34353c831aa7fb1e6 $
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

/*
 * Test driver for call-back functions in Linux KNET driver.
 *
 * The module can be built from the standard Linux user mode target
 * directories using the following command (assuming bash), e.g.
 *
 *   cd $SDK/systems/linux/user/gto-2_6
 *   BUILD_KNET_CB=1 make -s mod
 *
 */

#include <gmodule.h> /* Must be included first */
#include <kcom.h>
#include <bcm-knet.h>

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Broadcom Linux KNET Call-Back Driver");
MODULE_LICENSE("Proprietary");

/* Module Information */
#define MODULE_MAJOR 121
#define MODULE_NAME "linux-knet-cb"

static void
show_mac(struct sk_buff *skb)
{
    printk("DMAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
           skb->data[0], skb->data[1], skb->data[2],
           skb->data[3], skb->data[4], skb->data[5]);
}

static struct sk_buff *
test_rx_cb(struct sk_buff *skb, int dev_no, void *meta)
{
    printk("rx_cb for dev %d\n", dev_no);
    if (skb->data[5] == 0x03) {
        dev_kfree_skb(skb);
        return NULL;
    }
    show_mac(skb);
    return skb;
}

static struct sk_buff *
test_tx_cb(struct sk_buff *skb, int dev_no, void *meta)
{
    printk("tx_cb for dev %d\n", dev_no);
    show_mac(skb);
    skb->data[5] += 1;
    return skb;
}

/*
 * Generic module functions
 */

static int
_pprint(void)
{   
    pprintf("Broadcom Linux KNET Call-Back Driver\n");

    return 0;
}

static int
_cleanup(void)
{
    bkn_rx_skb_cb_unregister(test_rx_cb);
    bkn_tx_skb_cb_unregister(test_tx_cb);

    return 0;
}   

static int
_init(void)
{
    bkn_rx_skb_cb_register(test_rx_cb);
    bkn_tx_skb_cb_register(test_tx_cb);

    return 0;
}

static gmodule_t _gmodule = {
    name: MODULE_NAME, 
    major: MODULE_MAJOR, 
    init: _init,
    cleanup: _cleanup, 
    pprint: _pprint, 
    ioctl: NULL,
    open: NULL, 
    close: NULL, 
}; 

gmodule_t*
gmodule_get(void)
{
    EXPORT_NO_SYMBOLS;
    return &_gmodule;
}
