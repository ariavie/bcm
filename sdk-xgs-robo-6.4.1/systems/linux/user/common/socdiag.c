/*
 * $Id: socdiag.c,v 1.25 Broadcom SDK $
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

#include <unistd.h>
#include <stdlib.h>

#include <sal/core/boot.h>
#include <sal/appl/sal.h>
#include <sal/appl/config.h>
#include <sal/appl/pci.h>
#include <soc/debug.h>
#include <shared/shr_bprof.h>

#include <appl/diag/system.h>

#include <linux-bde.h>

#if defined(MEMLOG_SUPPORT) && defined(__GNUC__)
#include <sal/core/memlog.h>
#include <sys/stat.h>
#include <fcntl.h>
int memlog_fd = 0;
char memlog_buf[MEM_LOG_BUF_SIZE];
#endif

ibde_t *bde;

/* The bus properties are (currently) the only system specific
 * settings required. 
 * These must be defined beforehand 
 */

#ifndef SYS_BE_PIO
#error "SYS_BE_PIO must be defined for the target platform"
#endif
#ifndef SYS_BE_PACKET
#error "SYS_BE_PACKET must be defined for the target platform"
#endif
#ifndef SYS_BE_OTHER
#error "SYS_BE_OTHER must be defined for the target platform"
#endif

#if !defined(SYS_BE_PIO) || !defined(SYS_BE_PACKET) || !defined(SYS_BE_OTHER)
#error "platform bus properties not defined."
#endif

#ifdef INCLUDE_KNET

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <soc/knet.h>
#include <uk-proxy-kcom.h>
#include <bcm-knet-kcom.h>

/* Function defined in linux-user-bde.c */
extern int
bde_irq_mask_set(int unit, uint32 addr, uint32 mask);

static soc_knet_vectors_t knet_vect_uk_proxy = {
    {
        uk_proxy_kcom_open,
        uk_proxy_kcom_close,
        uk_proxy_kcom_msg_send,
        uk_proxy_kcom_msg_recv
    },
    bde_irq_mask_set
};

static soc_knet_vectors_t knet_vect_bcm_knet = {
    {
        bcm_knet_kcom_open,
        bcm_knet_kcom_close,
        bcm_knet_kcom_msg_send,
        bcm_knet_kcom_msg_recv
    },
    bde_irq_mask_set
};

static void
knet_kcom_config(void)
{
    soc_knet_vectors_t *knet_vect;
    char *kcom_name;
    int procfd;
    char procbuf[128];

    /* Direct IOCTL by default */
    knet_vect = &knet_vect_bcm_knet;
    kcom_name = "bcm-knet";

    if ((procfd = open("/proc/linux-uk-proxy", O_RDONLY)) >= 0) {
        if ((read(procfd, procbuf, sizeof(procbuf))) > 0 &&
            strstr(procbuf, "KCOM_KNET : ACTIVE") != NULL) {
            /* Proxy loaded and active */
            knet_vect = &knet_vect_uk_proxy;
            kcom_name = "uk-proxy";
        }
        close(procfd);
    }

    soc_knet_config(knet_vect);
    var_set("kcom", kcom_name, 0, 0);
}

#endif /* INCLUDE_KNET */

int
bde_create(void)
{	
    linux_bde_bus_t bus;
    bus.be_pio = SYS_BE_PIO;
    bus.be_packet = SYS_BE_PACKET;
    bus.be_other = SYS_BE_OTHER;
    return linux_bde_create(&bus, &bde);
}

/*
 * Main loop.
 */
int main(int argc, char *argv[])
{
    int i, len;
    char *envstr;
    char *config_file, *config_temp;

#ifdef BCM_BPROF_STATS
    shr_bprof_stats_time_init();
#endif

    if ((envstr = getenv("BCM_CONFIG_FILE")) != NULL) {
        config_file = envstr;
        len = sal_strlen(config_file);
        if ((config_temp = sal_alloc(len+5, NULL)) != NULL) {
            sal_strcpy(config_temp, config_file);
            sal_strcpy(&config_temp[len], ".tmp");
            sal_config_file_set(config_file, config_temp);
            sal_free(config_temp);
        }
    }

    if (sal_core_init() < 0 || sal_appl_init() < 0) {
        /*
         * If SAL initialization fails then printf will most
         * likely assert or fail. Try direct console access
         * instead to get the error message out.
         */
	char *estr = "SAL Initialization failed\r\n";
        sal_console_write(estr, sal_strlen(estr) + 1);
	exit(1);
    }
    
    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--reload")) {
	    sal_boot_flags_set(sal_boot_flags_get() | BOOT_F_RELOAD);
	}
    }

#ifdef MEMLOG_SUPPORT
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-memlog") && argv[i+1] != NULL) {
            memlog_fd = creat(argv[i+1], S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        }
    }
#endif
#ifdef INCLUDE_KNET
    knet_kcom_config();
#endif

    diag_shell();

    linux_bde_destroy(bde);
    return 0;
}



/*
 * These stubs are here for legacy compatability reasons. 
 * They are used only by the diag/test code, not the driver, 
 * so they are really not that important. 
 */

void pci_print_all(void)
{
    int device;

    if (NULL == bde) {
        sal_printf("Devices not probed yet.\n");
        return;
    }

    sal_printf("Scanning function 0 of devices 0-%d\n", bde->num_devices(BDE_SWITCH_DEVICES) - 1);
    sal_printf("device fn venID devID class  rev MBAR0    MBAR1    IPIN ILINE\n");

    for (device = 0; device < bde->num_devices(BDE_SWITCH_DEVICES); device++) {
	uint32		vendorID, deviceID, class, revID;
	uint32		MBAR0, MBAR1, ipin, iline;
	
	vendorID = (bde->pci_conf_read(device, PCI_CONF_VENDOR_ID) & 0x0000ffff);
	
	if (vendorID == 0)
	    continue;
	

#define CONFIG(offset)	bde->pci_conf_read(device, (offset))

	deviceID = (CONFIG(PCI_CONF_VENDOR_ID) & 0xffff0000) >> 16;
	class    = (CONFIG(PCI_CONF_REVISION_ID) & 0xffffff00) >>  8;
	revID    = (CONFIG(PCI_CONF_REVISION_ID) & 0x000000ff) >>  0;
	MBAR0    = (CONFIG(PCI_CONF_BAR0) & 0xffffffff) >>  0;
	MBAR1    = (CONFIG(PCI_CONF_BAR1) & 0xffffffff) >>  0;
	iline    = (CONFIG(PCI_CONF_INTERRUPT_LINE) & 0x000000ff) >>  0;
	ipin     = (CONFIG(PCI_CONF_INTERRUPT_LINE) & 0x0000ff00) >>  8;
	
#undef CONFIG

	sal_printf("%02x  %02x %04x  %04x  "
		   "%06x %02x  %08x %08x %02x   %02x\n",
		   device, 0, vendorID, deviceID, class, revID,
		   MBAR0, MBAR1, ipin, iline);
    }
}
