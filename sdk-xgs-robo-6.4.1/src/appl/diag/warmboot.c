/*
 * $Id: warmboot.c,v 1.13 Broadcom SDK $
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
 * File:     warmboot.c
 * Purpose:     Sample Warm Boot cache in the Linux filesystem.
 */


#include <sal/core/boot.h>
#include <sal/core/libc.h>
#include <sal/appl/io.h>
#include <shared/bsl.h>
#include <soc/drv.h>
#include <soc/debug.h>

/* Required for WB to DRAM (and not a file) */
#ifdef BCM_ARAD_SUPPORT
    #include <soc/dpp/error.h>
    #include <soc/dpp/drv.h>
    #include <soc/dpp/ARAD/arad_dram.h>
    #include <bcm_int/dpp/error.h>
#endif

#ifdef BCM_WARM_BOOT_SUPPORT

#if (defined(LINUX) && !defined(__KERNEL__)) || defined(UNIX)

#include <stdio.h>

static FILE *scache_fp[SOC_MAX_NUM_DEVICES];
                                    /* Non-NULL indicates file opened */
static sal_mutex_t file_lock[SOC_MAX_NUM_DEVICES];
#ifndef NO_FILEIO
static char *scache_nm[SOC_MAX_NUM_DEVICES];
                                    /* Non-NULL indicates file selected */
#endif

STATIC int
appl_scache_file_read_func(int unit, uint8 *buf, int offset, int nbytes)
{
    size_t result;

    if (!scache_fp[unit]) {
        return SOC_E_UNIT;
    }
    if (sal_mutex_take(file_lock[unit],sal_mutex_FOREVER))
    {
        cli_out("Unit %d: Mutex take failed\n", unit);
        return SOC_E_FAIL;
    }
    if (0 != fseek(scache_fp[unit], offset, SEEK_SET)) {
        return SOC_E_FAIL;
    }

    result = fread(buf, 1, nbytes, scache_fp[unit]);
    if (result != nbytes) {
        sal_mutex_give(file_lock[unit]);
        return SOC_E_MEMORY;
    }
    if ( sal_mutex_give(file_lock[unit]))
    {
        cli_out("Unit %d: Mutex give failed\n", unit);
        return SOC_E_FAIL;
    }
    return SOC_E_NONE;
}

STATIC int
appl_scache_file_write_func(int unit, uint8 *buf, int offset, int nbytes)
{
    size_t result;

    if (!scache_fp[unit]) {
        return SOC_E_UNIT;
    }
    if (sal_mutex_take(file_lock[unit],sal_mutex_FOREVER))
    {
        cli_out("Unit %d: Mutex take failed\n", unit);
        return SOC_E_FAIL;
    }
    if (0 != fseek(scache_fp[unit], offset, SEEK_SET)) {
        return SOC_E_FAIL;
    }

    result = fwrite(buf, 1, nbytes, scache_fp[unit]);
    if (result != nbytes) {
        if ( sal_mutex_give(file_lock[unit]))
        {
            cli_out("Unit %d: Mutex give failed\n", unit);
            return SOC_E_FAIL;
        }
        return SOC_E_MEMORY;
    }
    fflush(scache_fp[unit]);
    if ( sal_mutex_give(file_lock[unit]))
    {
        cli_out("Unit %d: Mutex give failed\n", unit);
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}

#if BCM_CALADAN3_SUPPORT 
void*
appl_scache_mem_alloc(uint32 sz)
{
    void *ptr = sal_alloc(sz, "wb hdl");    
    printk("SCACHE ALLOC: 0x%08x of size 0x%08x\n", (int)ptr, sz);
    return ptr;
}

void
appl_scache_mem_free(void* ptr)
{    
    printk("SCACHE FREE: 0x%08x\n", (int)ptr);
    sal_free(ptr);
}
#endif /* BCM_CALADAN3_SUPPORT */

int
appl_scache_file_close(int unit)
{
#ifndef NO_FILEIO
    if (!scache_nm[unit]) {
    cli_out("Unit %d: Scache file is not set\n", unit);
    return -1;
    }
    if (sal_mutex_take(file_lock[unit],sal_mutex_FOREVER))
    {
        cli_out("Unit %d: Mutex take failed\n", unit);
        return SOC_E_FAIL;
    }
    if (scache_fp[unit]) {
    sal_fclose(scache_fp[unit]);
    scache_fp[unit] = 0;
    }
    sal_free(scache_nm[unit]);
    scache_nm[unit] = NULL;
    sal_mutex_destroy(file_lock[unit]);
    file_lock[unit]=0;
#endif
    return 0;
}

int
appl_scache_file_open(int unit, int warm_boot, char *filename)
{
#ifndef NO_FILEIO
    if (scache_nm[unit]) {
    appl_scache_file_close(unit);
        scache_nm[unit] = NULL;
    }
    if (NULL == filename) {
        return 0; /* No scache file */
    }
    file_lock[unit] = sal_mutex_create("schan-file");
    if (sal_mutex_take(file_lock[unit],sal_mutex_FOREVER))
    {
        cli_out("Unit %d: Mutex take failed\n", unit);
        return SOC_E_FAIL;
    }
    if ((scache_fp[unit] =
         sal_fopen(filename, warm_boot ? "r+" : "w+")) == 0) {
    cli_out("Unit %d: Error opening scache file\n", unit);
    return -1;
    }
    scache_nm[unit] =
        sal_strncpy((char *)sal_alloc(sal_strlen(filename) + 1, __FILE__),
                   filename, sal_strlen(filename));

    if (soc_switch_stable_register(unit,
                                   &appl_scache_file_read_func,
                                   &appl_scache_file_write_func,
#if BCM_CALADAN3_SUPPORT 
                                   &appl_scache_mem_alloc, &appl_scache_mem_free) < 0) {
#else /* BCM_CALADAN3_SUPPORT */
                                   NULL, NULL) < 0) {
#endif /* BCM_CALADAN3_SUPPORT */
        cli_out("Unit %d: soc_switch_stable_register failure\n", unit);
        return -1;
    }
     if ( sal_mutex_give(file_lock[unit]))
    {
        cli_out("Unit %d: Mutex give failed\n", unit);
        return SOC_E_FAIL;
    }
#endif /* NO_FILEIO */
    return 0;
}

#else /* (defined(LINUX) && !defined(__KERNEL__)) || defined(UNIX) */

int
appl_scache_file_close(int unit)
{
    return 0;
}

int
appl_scache_file_open(int unit, int warm_boot)
{
    return 0;
}

#endif /* (defined(LINUX) && !defined(__KERNEL__)) || defined(UNIX) */

/*
 * Function:
 *     appl_warm_boot_event_handler_cb
 *
 * Purpose:
 *     Warm boot event handler call back routine for test purpose.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     event    - (IN) Switch event type
 *     argX     - (IN) Event Arguments
 *     userdata - (IN) user supplied data
 * Returns:
 *     BCM_E_XXX
 */
void
appl_warm_boot_event_handler_cb(int unit, soc_switch_event_t event,
                               uint32 arg1, uint32 arg2, uint32 arg3,
                               void *userdata)
{
    switch (event) {
        case SOC_SWITCH_EVENT_STABLE_FULL:
        case SOC_SWITCH_EVENT_STABLE_ERROR:
        case SOC_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN:
        case SOC_SWITCH_EVENT_WARM_BOOT_DOWNGRADE:
            assert(0);
            break;
        default:
            break;
    }
    return;
}

#ifdef BCM_ARAD_SUPPORT
int
_arad_user_buffer_dram_read_cb(
    int unit, 
    uint8 *buf, 
    int offset, 
    int nbytes) {

    int rv = BCM_E_NONE;

    rv = handle_sand_result(soc_arad_user_buffer_dram_read(
         unit, 
         0x0, 
         buf, 
         offset, 
         nbytes));

    return rv;
}

int
_arad_user_buffer_dram_write_cb(
    int unit, 
    uint8 *buf, 
    int offset, 
    int nbytes) {
    SOC_SAND_IF_ERR_RETURN(
       soc_arad_user_buffer_dram_write(
         unit, 
         0x0, 
         buf, 
         offset, 
         nbytes));

    return BCM_E_NONE;
}

/* 
 * Write function signature to register for the application provided
 * stable for Level 2 Warm Boot in user buffer (dram)
 */

int
appl_scache_user_buffer(int unit)
{
    if (soc_switch_stable_register(unit,
                                   &_arad_user_buffer_dram_read_cb,
                                   &_arad_user_buffer_dram_write_cb,
                                   NULL, NULL) < 0) {
        cli_out("Unit %d: soc_switch_stable_register failure\n", unit);
        return -1;
    }

    return 0;
}
#endif /* BCM_ARAD_SUPPORT */

#else /* BCM_WARM_BOOT_SUPPORT */
int _warmboot_not_empty;
#endif /* BCM_WARM_BOOT_SUPPORT */


