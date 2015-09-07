/*
 * $Id: 5c59718722d1997b94183f58ce2168b25485a389 $
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
 * File:    internal_stack.c
 *
 * Purpose: 
 *
 * Functions:
 *      esw_init_pci
 *      set_ext_stack_pciconfig
 *      ks_pci_info_setup
 *
 *      _bcm_ptp_write_ks_uint32
 *      _bcm_ptp_read_ks_uint32
 *      _bcm_ptp_write_pcishared_uint32
 *      _bcm_ptp_read_pcishared_uint32
 *      _bcm_ptp_write_pcishared_uint8
 *      _bcm_ptp_read_pcishared_uint8
 */

#if defined(INCLUDE_PTP)
#include <sal/types.h>
#include <sal/core/spl.h>
#include <sal/core/libc.h>
#include <soc/types.h>
#include <soc/cm.h>
#include <soc/cmext.h>
#include <soc/defs.h>
#include <soc/drv.h>
#include <assert.h>

#if defined(PTP_KEYSTONE_STACK)
#ifndef __KERNEL__
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <bcm/ptp.h>
#include <bcm/error.h>
#include <bcm_int/common/ptp.h>

/* For each Keystone TOP, we keep the information needed to do PCIe shared
 *   memory reads & writes:
 */
typedef struct _bcm_esw_ptp_pci_info_s {
    sal_vaddr_t host_membase;   /* window base address in host's address space */
    uint32 device_membase;      /* window base address in TOP device's address space */
    uint32 pci_dev_idx;         /* PCIe device index */
} _bcm_esw_ptp_pci_info_t;

/* While Keystone TOP's are directly connected to the host via PCIe, they also have an associated Unit, and
 * a stack number within that unit.  So the maximum that we can support is BCM_MAX_NUM_UNITS * PTP_MAX_STACKS_PER_UNIT
 *
 * After enumerating a given Keystone/BCM53903, the information is stored here:
 */
_bcm_esw_ptp_pci_info_t _bcm_esw_ptp_pci_info[BCM_MAX_NUM_UNITS * PTP_MAX_STACKS_PER_UNIT] = {{0}};

/* Function pointer for PCI configuration function: */
bcm_ptp_pci_setconfig_t esw_pciconfig_func = (bcm_ptp_pci_setconfig_t)0;

/* Wrapper that uses esw_pciconfig_func if set, else soc_cm_pci_conf_write */
int set_ext_stack_pciconfig(int idx, uint32 pciconfig_register, uint32 value);

int ks_pci_info_setup(int ks_num);

/*
 * Function:
 *      esw_set_pci_config_func
 * Purpose:
 *      Can be used to override set_ext_stack_pciconfig() as the
 *      function called to set PCI configuration memory
 * Parameters:
 *      pci_setconfig - (IN) The function to be used to set PCI configuration memory
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */

int esw_init_pci (bcm_ptp_pci_setconfig_t pci_setconfig)
{
    esw_pciconfig_func = pci_setconfig;
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_ptp_write_ks_uint32
 * Purpose:
 *      Write a word to PCI shared memory for a given indexed TOP
 * Parameters:
 *      idx     - (IN) index of Keystone TOP
 *      addr    - (IN) address to write to
 *      val     - (IN) value to write
 * Returns:
 *      BCM_E_NONE
 * Notes:
 *      used for _bcm_ptp_stack_info_t->writed_fn
 */
int _bcm_ptp_write_ks_uint32(int idx, uint32 addr, uint32 val)
{
    uint32 new_window_base = (addr & 0xfffff000);
    uint32 offset = (addr - new_window_base) / sizeof(uint32);
    volatile uint32 *mapped_base;
    int spl;
    int rv;

    rv = ks_pci_info_setup(idx);
    if (rv != BCM_E_NONE) {
        return rv;
    }

    mapped_base = (volatile uint32 *)UINTPTR_TO_PTR(_bcm_esw_ptp_pci_info[idx].host_membase);

    spl = sal_splhi();

    if (_bcm_esw_ptp_pci_info[idx].device_membase != new_window_base) {
        _bcm_esw_ptp_pci_info[idx].device_membase = new_window_base;
        rv = set_ext_stack_pciconfig(idx, 0x80, new_window_base);
    }
    if (rv == BCM_E_NONE) {
        mapped_base[offset] = val;
    }

    sal_spl(spl);
    return 0;
}

/*
 * Function:
 *      _bcm_ptp_read_ks_uint32
 * Purpose:
 *      Read a word from PCI shared memory for a given indexed TOP
 * Parameters:
 *      idx     - (IN) index of Keystone TOP
 *      addr    - (IN) address to read from
 *      value   - (OUT) value read
 * Returns:
 *      BCM_E_NONE
 * Notes:
 *      used for _bcm_ptp_stack_info_t->read_fn
 */
int _bcm_ptp_read_ks_uint32(int idx, uint32 addr, uint32 *value)
{
    uint32 new_window_base = (addr & 0xfffff000);
    uint32 offset = (addr - new_window_base) / sizeof(uint32);
    volatile uint32 *mapped_base;
    int rv;

    rv = ks_pci_info_setup(idx);
    if (rv != BCM_E_NONE) {
        return rv;
    }

    mapped_base = (volatile uint32 *)UINTPTR_TO_PTR(_bcm_esw_ptp_pci_info[idx].host_membase);

    int spl = sal_splhi();

    if (_bcm_esw_ptp_pci_info[idx].device_membase != new_window_base) {
        _bcm_esw_ptp_pci_info[idx].device_membase = new_window_base;
        rv = set_ext_stack_pciconfig(idx, 0x80, new_window_base);
    }
    if (rv == BCM_E_NONE) {
        *value = mapped_base[offset];
    }

    sal_spl(spl);
    return rv;
}


int set_ext_stack_pciconfig(int idx, uint32 pciconfig_register, uint32 value) {
    if (esw_pciconfig_func) {
        esw_pciconfig_func(_bcm_esw_ptp_pci_info[idx].pci_dev_idx, pciconfig_register, value);
    } else {
        soc_cm_pci_conf_write(_bcm_esw_ptp_pci_info[idx].pci_dev_idx, pciconfig_register, value);
    }
    return 0;
}

/* Enumerate the given Keystone on the PCI bus */
int ks_pci_info_setup(int ks_num)
{
    int dev;
    uint16 dev_id;
    uint8 rev_id;
    int dev_count;
    int ks_count = 0;
    int rv = BCM_E_NONE;

    /* If we've already found this TOP via enumeration, just return */
    if (_bcm_esw_ptp_pci_info[ks_num].host_membase != 0) {
        return BCM_E_NONE;
    }

    if (soc_cm_device_supported(BCM53000PCIE_DEVICE_ID,
            BCM53000_A0_REV_ID)) {
        LOG_ERROR(BSL_LS_BCM_PTP,
                  (BSL_META("Keystone device not supported\n")));
        return BCM_E_UNAVAIL;
    }

    dev_count = soc_cm_get_num_devices();
    for (dev = 0; dev < dev_count; dev++) {
        dev_id = 0;
        rev_id = 0;
        soc_cm_get_id(dev, &dev_id, &rev_id);
        if (dev_id == BCM53000PCIE_DEVICE_ID) {
            if (ks_count == ks_num) {
                /* This is the Keystone we want */

                /* cache the PCI device index */
                _bcm_esw_ptp_pci_info[ks_num].pci_dev_idx = dev;

                /* cache the base address for future operations */
                _bcm_esw_ptp_pci_info[ks_num].host_membase = soc_cm_get_base_address(dev);

                /* Set the window base to zero */
                _bcm_esw_ptp_pci_info[ks_num].device_membase = 0;
                rv = set_ext_stack_pciconfig(ks_num, 0x80, 0);

                /*Update  Command/Status (0x4) <- 0x00100146 */
                return rv;
            }
            ks_count++;
        }
    }
    return BCM_E_FAIL;
}


/*
 * Function:
 *      _bcm_ptp_write_pcishared_uint32
 * Purpose:
 *      Write a word to PCI shared memory
 * Parameters:
 *      cookie  - (IN) indicates the KS that we will write to
 *      addr    - (IN) address to write to
 *      val     - (IN) value to write
 * Returns:
 *      BCM_E_NONE
 * Notes:
 *      used for _bcm_ptp_stack_info_t->writed_fn
 */
int _bcm_ptp_write_pcishared_uint32(void *cookie, uint32 addr, uint32 val)
{
    return _bcm_ptp_write_ks_uint32(PTR_TO_INT(cookie), addr, val);
}

/*
 * Function:
 *      _bcm_ptp_read_pcishared_uint32
 * Purpose:
 *      Read a word from PCI shared memory
 * Parameters:
 *      cookie  - (IN) PCI memory base
 *      addr    - (IN) address to read from
 *      value   - (OUT) value read
 * Returns:
 *      BCM_E_NONE
 * Notes:
 *      used for _bcm_ptp_stack_info_t->read_fn
 */
int _bcm_ptp_read_pcishared_uint32(void *cookie, uint32 addr, uint32 *value)
{
    return _bcm_ptp_read_ks_uint32(PTR_TO_INT(cookie), addr, value);
}


/* /\* Note, this implementation can be optimized if it is known that 8-bit accesses *\/ */
/* /\*   via PCI shared memory work correctly.  This is meant to be a                *\/ */
/* /\*   lowest-common-denominator portable version.                                 *\/ */
void _bcm_ptp_write_pcishared_uint8(bcm_ptp_external_stack_info_t *stack_p, uint32 addr, uint8 val)
{
    uint32 aligned_addr = (addr & 0xfffffffc);
    uint32 orig_word;
    unsigned shift;
    uint32 masked_word;
    uint32 new_word;

    stack_p->read_fn(stack_p->cookie, aligned_addr, &orig_word);

    /* Find shift needed to put the low byte of the word into the desired position */
    /* BCM53903 is big-endian, so first byte (offset 0) has shift 24,              */
    /*    last byte (offset 3) has shift 0                                         */
    shift = 8 * (3 - (addr - aligned_addr));

    masked_word = orig_word & ~(0xff << shift);
    new_word = masked_word | (((uint32)val) << shift);

    stack_p->write_fn(stack_p->cookie, aligned_addr, new_word);
}


/* Note, this implementation can be optimized if it is known that 8-bit accesses */
/*   via PCI shared memory work correctly.  This is meant to be a                */
/*   lowest-common-denominator portable version.                                 */
uint8 _bcm_ptp_read_pcishared_uint8(bcm_ptp_external_stack_info_t *stack_p, uint32 addr)
{
    uint32 aligned_addr = (addr & 0xfffffffc);
    uint32 value32;
    unsigned shift;
    uint8 ret;

    stack_p->read_fn(stack_p->cookie, aligned_addr, &value32);

    /* Find shift needed to put the low byte of the word into the desired position */
    /* BCM53903 is big-endian, so first byte (offset 0) has shift 24,              */
    /*    last byte (offset 3) has shift 0                                         */
    shift = 8 * (3 - (addr - aligned_addr));
    ret = value32 >> shift;

    return ret;
}


/* void _bcm_ptp_read_pcishared_uint8_aligned_array(bcm_ptp_external_stack_info_t *stack_p, uint32 addr, uint8 * array, int array_len) */
/* { */
/*     while (array_len > 0) { */
/*         uint32 value; */
/*         _bcm_ptp_read_pcishared_uint32(stack_p->cookie, addr, &value); */
/*         array[0] = (value >> 24); */
/*         array[1] = (value >> 16); */
/*         array[2] = (value >> 8); */
/*         array[3] = value; */
/*         array += 4; */
/*         array_len -= 4; */
/*         addr += 4; */
/*     } */
/* } */



#endif /* defined(PTP_KEYSTONE_STACK) */
#endif /* defined(INCLUDE_PTP) */
