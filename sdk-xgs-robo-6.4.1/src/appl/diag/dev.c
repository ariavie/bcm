/*
 * $Id: dev.c,v 1.12 Broadcom SDK $
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
 * device CLI commands
 */

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/dport.h>
#include <appl/diag/sysconf.h>
#include <shared/bsl.h>
#include <soc/debug.h>
#include <soc/hash.h>
#include <soc/cmext.h>
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif

#include <bcm/error.h>
#include <bcm/init.h>
#include <bcm/tunnel.h>
#include <bcm/ipmc.h>
#include <bcm/debug.h>
#include <ibde.h>


/*
 * Function:
 *      _device_cmd_detach
 * Description:
 * 	    Service routine used to remove bcm device.
 * Parameters:
 *      unit      - (IN) Device number.
 *      a         - (IN) Command arguments. 
 * Returns:
 *      CMD_XXX
 */
STATIC cmd_result_t
_device_cmd_detach(int unit, args_t *a) 
{
    parse_table_t   pt;
    int             rv;
    cmd_result_t    retCode;
    int             dunit = unit;

    parse_table_init(unit, &pt);

    parse_table_add(&pt, "Unit",    PQ_DFL | PQ_INT,  0, (void *)&dunit, 0);
    if (!parseEndOk(a, &pt, &retCode)) {
        return retCode;
    }

    rv  = bcm_detach(dunit);
    if (BCM_FAILURE(rv)) {
        cli_out("%s: bcm detach error : %s\n", 
                ARG_CMD(a), bcm_errmsg(rv));
        return (CMD_FAIL);
    }
    
    /* soc_cm_device_destroy also calls the appropriate soc_*_detach */
    rv = soc_cm_device_destroy(dunit);
    if (BCM_FAILURE(rv)) {
        cli_out("%s: soc_cm_device_destroy error : %s\n", 
                ARG_CMD(a), bcm_errmsg(rv));
        return (CMD_FAIL);
    }

    return (CMD_OK);

}

/*
 * Function:
 *      _device_cmd_attach
 * Description:
 * 	    Service routine used to add and initialize bcm device.
 * Parameters:
 *      unit      - (IN) Device number.
 *      a         - (IN) Command arguments. 
 * Returns:
 *      CMD_XXX
 */
STATIC cmd_result_t
_device_cmd_attach(int unit, args_t *a) 
{
    parse_table_t   pt;
    int             rv;
    cmd_result_t    retCode;
    int             dunit = unit;
    int             devID;
    int             revID;
    const ibde_dev_t *dev;

    parse_table_init(unit, &pt);

    parse_table_add(&pt, "Unit",    PQ_DFL | PQ_INT,  0, (void *)&dunit, 0);
    if (!parseEndOk(a, &pt, &retCode)) {
        return retCode;
    }

    dev = bde->get_dev(dunit);
    devID = dev->device;
    revID = dev->rev;

#if defined(BCM_ENDURO_SUPPORT)
    /*
     * For Enduro, read revision ID from CMIC instead of PCIe config space.
     */
    if ((devID == BCM56132_DEVICE_ID) || (devID == BCM56134_DEVICE_ID) ||
        (devID == BCM56320_DEVICE_ID) || (devID == BCM56321_DEVICE_ID) || 
        (devID == BCM56331_DEVICE_ID) || (devID == BCM56333_DEVICE_ID) ||
        (devID == BCM56334_DEVICE_ID) || (devID == BCM56338_DEVICE_ID) ||
        (devID == BCM56230_DEVICE_ID) || (devID == BCM56231_DEVICE_ID)) {
        uint32          config = 0;
        int             big_pio, big_packet, big_other;

        bde->pci_bus_features(dunit, &big_pio, &big_packet, &big_other);

        if (big_pio) {
            config |= ES_BIG_ENDIAN_PIO;
        }
        if (big_packet) {
            config |= ES_BIG_ENDIAN_DMA_PACKET;
        }
        if (big_other) {
            config |= ES_BIG_ENDIAN_DMA_OTHER;
        }

        bde->write(dunit, CMIC_ENDIAN_SELECT, config);            
        revID = (bde->read(dunit, CMIC_REVID_DEVID) >> 16) & 0xff;
    }
#endif /* BCM_ENDURO_SUPPORT */

    if (soc_cm_device_supported(devID, revID) < 0) {
        /* Not a switch chip; continue probing other devices */
        return 0;
    }

    rv = soc_cm_device_create_id(devID, revID, NULL, dunit);
    if (BCM_FAILURE(rv)) {
        cli_out("%s: soc_cm_device_create_id error : %s\n", 
                ARG_CMD(a), bcm_errmsg(rv));
        return (CMD_FAIL);
    }

    rv = sysconf_attach(dunit);
    if (BCM_FAILURE(rv)) {
        cli_out("%s: sysconf_attach error : %s\n", 
                ARG_CMD(a), bcm_errmsg(rv));
        return (CMD_FAIL);
    }

#if defined(BCM_SBX_SUPPORT)
    if (SOC_IS_SBX(dunit)) {
        rv = soc_sbx_init(dunit);
        if (BCM_FAILURE(rv)) {
            cli_out("%s: soc_sbx_init error : %s\n", 
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }

        /* type = NULL for bcm_attach to find correct dipatch table */
        rv = bcm_attach(dunit, NULL, NULL, 0);
        if (BCM_FAILURE(rv)) {
            cli_out("%s: bcm_attach error : %s\n", 
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }

        rv = bcm_init(dunit);
        if (BCM_FAILURE(rv)) {
            cli_out("%s: bcm_init error : %s\n", 
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }

    } else
#endif /* BCM_SBX_SUPPORT */
#if defined(BCM_ESW_SUPPORT)
    if (SOC_IS_ESW(dunit)) {    

        rv = soc_init(dunit);
        if (BCM_FAILURE(rv)) {
            cli_out("%s: soc_init error : %s\n", 
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }
        
        rv = soc_misc_init(dunit);
        if (BCM_FAILURE(rv)) {
            cli_out("%s: soc_misc_init error : %s\n", 
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }
        
        rv = soc_mmu_init(dunit);
        if (BCM_FAILURE(rv)) {
            cli_out("%s: soc_mmu_init error : %s\n", 
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }
        rv = bcm_attach(dunit, "esw", NULL, 0);
    } else 
#endif
#if defined(BCM_TK371X_SUPPORT)
    if (SOC_IS_EA(dunit)){
        if (BCM_FAILURE(rv)) {
            cli_out("%s: soc_init error : %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }
        rv = bcm_attach(dunit, "ea", "tk3715", 0);
    }else
#endif
    {
#if defined(BCM_ROBO_SUPPORT)
        rv = soc_robo_init(dunit);
        if (BCM_FAILURE(rv)) {
            cli_out("%s: soc_init error : %s\n", 
                    ARG_CMD(a), bcm_errmsg(rv));
            return (CMD_FAIL);
        }
        rv = bcm_attach(dunit, "robo", NULL, 0);
#endif        
    }

    if (BCM_FAILURE(rv)) {
        cli_out("%s: bcm_attach error : %s\n", 
                ARG_CMD(a), bcm_errmsg(rv));
        return (CMD_FAIL);
    }
    return (CMD_OK);
}

char cmd_device_usage[] =
    "Usages:\n\t"
#ifdef COMPILER_STRING_CONST_LIMIT
    "  device <option> [args...]\n"
#else
    "  device attach [<unit>=<id>]\n\t"
    "  device detach [<unit>=<id>]\n"
#endif
    ;
cmd_result_t
cmd_device(int unit, args_t *a)
{
    char           *table;

    if ((table = ARG_GET(a)) == NULL) {
        return CMD_USAGE;
    }

    /* Device create*/
    if (sal_strcasecmp(table, "attach") == 0) {
         return _device_cmd_attach(unit, a); 
    }
    if (sal_strcasecmp(table, "detach") == 0) {
         return _device_cmd_detach(unit, a); 
    }
    return CMD_USAGE;
}
