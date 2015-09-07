/*
 * $Id: socdiag-nsa.c 1.6 Broadcom SDK $
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
#ifdef NO_SAL_APPL

/******************************************************************************
 *
 * About NO_SAL_APPL mode
 *
 * This configuration is built with NO_SAL_APPL defined. 
 *
 * In this build configuration, the Application SAL is not required to be compiled
 * or ported to the target operating system/environment. 
 *
 * There are 4 functions you must provide to hook the diag shell up to your
 * environment. These 4 functions are documented below. 
 * They are also implemented for the simulation target. 
 *
 * You should provide your own implementation of these functions
 * to import the diag shell into your environment. 
 *
 *****************************************************************************/

/*
 * Function:
 *      sal_portable_vprintf
 * Purpose:
 *      Printf function used by the diag shell. 
 * Parameters:
 *      fmt -- format string
 *      vargs -- va_list
 *
 * Notes:
 *      Required for output from the shell
 *
 *      A version compatible with unix is provided here for
 *      the simulator build. Provide your own in your environment. 
 */
#include <stdarg.h>     /* for va_list */
#include <stdio.h>      /* for vprintf */
#include <sal/appl/io.h> /* for prototype for these functions */

int 
sal_portable_vprintf(const char* fmt, va_list vargs)
{
    return vprintf(fmt, vargs); 
}



/*
 * Function:
 *      sal_portable_readline
 * Purpose:
 *      Retrieve an input line for the diag shell. 
 * Parameters:
 *      prompt -- prompt to be printed to the user at the input line
 *      buf    -- receive buffer for the line
 *      bufsize -- maximum size of the receive buffer
 * Returns:
 *      buf
 *
 * Notes:
 *      Required for input to the diag shell. 
 *      If you are only calling sh_process_command() and not 
 *      the diag shell input loop, this function will never get called
 *      and you can just return NULL.
 */

extern char* 
sal_portable_readline(char *prompt, char *buf, int bufsize)
{
    if(prompt) {
        printf("%s", prompt); 
    }
    if(buf && bufsize > 0) {
        buf[0] = 0; 
        fgets(buf, bufsize, stdin); 
        if(buf[strlen(buf)-1] == '\n') {
            buf[strlen(buf)-1] = 0;
        }
    }
    return buf; 
}


/*
 * Function:
 *      sal_rand
 * Purpose:
 *      Returns a random number between 0 and SAL_RAND_MAX. 
 * Parameters:
 *      None
 * Returns:
 *      random number
 * Notes:
 *      Used by some of the tests in the diag shell. 
 */

#include <sal/appl/sal.h> /* Needed for declaration of sal_rand and sal_srand */
#include <stdlib.h>

int 
sal_rand(void)
{
    return (int)(random() & SAL_RAND_MAX);
}

/*
 * Function:
 *      sal_srand
 * Purpose:
 *      Seed the random number generator
 * Parameters:
 *      seed -- seed
 */
void 
sal_srand(unsigned int seed)
{
    srandom((unsigned int)seed);
}

/******************************************************************************
 *
 * END OF PORTABILITY FUNCTIONS FOR BCM_PORTABLE_DIAG_MODE
 *
 *****************************************************************************/




/******************************************************************************
 *
 * The rest of this file is concerned with providing an actual system main
 * application using the diag shell in portability mode. 
 * 
 * These are specific to the simulator platform and are not part of the 
 * portability functions required and described above. 
 * 
 */

#include <unistd.h>
#include <appl/diag/system.h> /* diag_shell */
#include <sal/appl/pci.h>     /* pci_device_iter */
#include <bde/pli/plibde.h>   /* bde_create */

ibde_t *bde;

int
bde_create(void)
{	
    return plibde_create(&bde);
}

/*
 * Main loop.
 */
int main(int argc, char *argv[])
{
    if (sal_core_init()) {
	printk("SAL Core Initialization failed\n");
	exit(1);
    }

    printk("Broadcom Command Monitor: (NO_SAL_APPL build)\n"
	   "Copyright (c) 1998-2010 Broadcom Corp.\n");
    printk("Version %s built %s\n", _build_release, _build_date);

    diag_shell();

    return 0;
}


/*
 * This function needed by the plibde implementation only
 */

#ifndef PCI_MAX_DEV
#define PCI_MAX_DEV 32
#endif

#ifndef PCI_MAX_BUS
#define PCI_MAX_BUS 1
#endif

int
pci_device_iter(int (*rtn)(pci_dev_t *dev,
			   uint16 pciVenID,
			   uint16 pciDevID,
			   uint8 pciRevID))
{
    uint32		venID, devID, revID;
    int			rv = 0;
    pci_dev_t		dev;
    
    dev.funcNo = 0;

    for (dev.busNo = 0;
	 dev.busNo < PCI_MAX_BUS && rv == 0;
	 dev.busNo++) {

	for (dev.devNo = 0;
	     dev.devNo < PCI_MAX_DEV && rv == 0;
	     dev.devNo++) {

	    if (dev.devNo == 12) {
		continue;
	    }

            /* NOTE -- pci_config_getw exported by plibde (not appl sal)  in this case */
	    venID = pci_config_getw(&dev, PCI_CONF_VENDOR_ID) & 0xffff;

	    if (venID == 0xffff) {
		continue;
	    }

	    devID = pci_config_getw(&dev, PCI_CONF_VENDOR_ID) >> 16;
	    revID = pci_config_getw(&dev, PCI_CONF_REVISION_ID) & 0xff;

	    rv = (*rtn)(&dev, venID, devID, revID);
	}
    }

    return rv;
}


#endif /* NO_SAL_APPL */
