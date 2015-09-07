/*
 * $Id: socdiag.c,v 1.9 Broadcom SDK $
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
 * socdiag: low-level diagnostics shell for Orion (SOC) driver.
 */

#include <shared/bsl.h>

#include <appl/diag/system.h>
#include <unistd.h>

#include <sal/core/boot.h>
#include <sal/appl/sal.h>
#include <sal/appl/config.h>
#include <soc/debug.h>

#include <bde/pli/plibde.h>

ibde_t *bde;

int
bde_create(void)
{	
    return plibde_create(&bde);
}

int devlist(const char* arg); 

/*
 * Main loop.
 */
int main(int argc, char *argv[])
{
    char *socrc = SOC_INIT_RC;
    char *config_file = NULL, *config_temp = NULL;
    int len = 0;

    if ((config_file = getenv("BCM_CONFIG_FILE")) != NULL) {
        len = sal_strlen(config_file);
        if ((config_temp = sal_alloc(len+5, NULL)) != NULL) {
            sal_strcpy(config_temp, config_file);
            sal_strcpy(&config_temp[len], ".tmp");
            sal_config_file_set(config_file, config_temp);
            sal_free(config_temp);
        } else {
            LOG_CLI((BSL_META("sal_alloc failed. \n")));
            exit(1);
        }
    }

    if (sal_core_init() < 0 || sal_appl_init() < 0) {
	LOG_CLI((BSL_META("SAL Initialization failed\n")));
	exit(1);
    }

#ifdef DEBUG_STARTUP
    debugk_select(DEBUG_STARTUP);
#endif

    LOG_CLI((BSL_META("Broadcom Command Monitor: "
                      "Copyright (c) 1998-2010 Broadcom Corp.\n")));
    LOG_CLI((BSL_META("Version %s built %s\n"), _build_release, _build_date));

    switch (argc) {
    case 3:
	socrc = argv[2];
    case 2:
	if (!strcmp(argv[1], "-testbios") || !strcmp(argv[1], "-tb")) {
	    diag_init();
	    if (socrc) {
		sh_rcload_file(0, NULL, socrc, 0);
	    }
	} else if (!strcmp(argv[1], "-reload") || !strcmp(argv[1], "-r")) {
	    sal_boot_flags_set(sal_boot_flags_get() | BOOT_F_RELOAD);
	    diag_shell();
        } else if (!strcmp(argv[1], "-devlist")) {
            devlist(argv[2]); 
            return 0; 
	} else {
	    LOG_CLI((BSL_META("Unknown option: %s\n"), argv[1]));
	    exit(1);
	}
	break;

    default:
	diag_shell();
	break;
    }

    return 0;
}

#include <soc/cm.h>
#include <appl/diag/sysconf.h>

int
devlist(const char* arg)
{
    sysconf_init(); 
    soc_cm_display_known_devices();
    return 0; 
}       
    
