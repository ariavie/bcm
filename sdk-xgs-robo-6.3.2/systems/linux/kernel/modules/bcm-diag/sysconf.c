/*
 * $Id: sysconf.c 1.8 Broadcom SDK $
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

#include <sal/appl/sal.h>
#include <sal/appl/config.h>
#include <sal/appl/io.h>

#include <soc/debug.h>
#include <soc/cmext.h>
#include <soc/drv.h>

#include <appl/diag/sysconf.h>

#include <ibde.h>

#include <bcm-core.h>

static int
_sysconf_debug_out(uint32 flags, const char *format, va_list args)
    __attribute__ ((format (printf, 2, 0)));


int
sysconf_probe(void)
{
    return 0;
}

int
sysconf_attach(int unit)
{
#if defined(BCM_EA_SUPPORT) && defined(BCM_TK371X_SUPPORT)
    if(SOC_IS_EA(unit)){
        soc_ea_pre_attach(unit);
        return 0;
    }
#endif /* defined(BCM_EA_SUPPORT) && defined(BCM_TK371X_SUPPORT) */
    
    if ( (soc_attached(unit)) ||
        (CMDEV(unit).dev.info->dev_type & SOC_ETHER_DEV_TYPE) ) {
        printk("OK - already attached\n");
        return 0;
    }
    return -1;
}

int
sysconf_detach(int unit)
{
    return 0;
}


static int
_sysconf_debug_out(uint32 flags, const char *format, va_list args)
{
    return (flags) ? vdebugk(flags, format, args) : vprintk(format, args);
}

static int
_sysconf_dump(soc_cm_dev_t *dev)
{
    /* add to bde? */
    const ibde_dev_t *d = bde->get_dev(dev->dev);
    printk("PCI: Base=%p\n", (void *)d->base_address);
    return 0;
}

int
sysconf_debug_vectors_get(soc_cm_init_t* dst)
{
    dst->debug_out = _sysconf_debug_out;
    dst->debug_check = debugk_check;
    dst->debug_dump = _sysconf_dump;
    return 0;
}
    
int
sysconf_init(void)
{
    soc_cm_init_t init_data;

    sysconf_debug_vectors_get(&init_data);
    return bcore_debug_register(&init_data);
}
