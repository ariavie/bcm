/*
 * $Id: c040684817eb9e4118d057b82eb61264cae30f73 $
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
 * File:    external_stack.c
 *
 * Purpose: 
 *
 * Functions:
 *      _bcm_ptp_external_stack_create
 *      _bcm_ptp_ext_stack_reset
 *      _bcm_ptp_ext_stack_start
 *      _bcm_ptp_write_pcishared_uint8_aligned_array
 *
 *      esw_set_ext_stack_config_uint32
 *      esw_set_ext_stack_config_array
 */

#if defined(INCLUDE_PTP)

#include <soc/defs.h>
#include <soc/drv.h>

#include <sal/appl/io.h>
#include <sal/core/dpc.h>

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm/error.h>

#if defined(BCM_PTP_EXTERNAL_STACK_SUPPORT)

/* Constants */
#define FAULT_CHECK_US (100000)  /* every .1 seconds */

/* Event support (ToP OUT-OF-MEMORY).*/
#define TOP_OOM_MSGDATA_SIZE_OCTETS                 (8)
#define TOP_OOM_MINIMUM_FREE_MEMORY_THRESHOLD_BYTES (1024)
#define TOP_OOM_ORDBLK_FREE_MEMORY_THRESHOLD_BYTES  (1024)

extern int _bcm_ptp_write_ks_uint32(int idx, uint32 addr, uint32 val);
extern int _bcm_ptp_read_ks_uint32(int idx, uint32 addr, uint32 *value);

extern void _bcm_ptp_read_pcishared_uint8_aligned_array(
    int pci_idx,
    uint32 addr,
    uint8 * array,
    int array_len);

extern void _bcm_ptp_write_pcishared_uint8_aligned_array(
    int pci_idx,
    uint32 addr,
    uint8 * array,
    int array_len);

extern int _bcm_ptp_ext_stack_reset(int pci_idx);

/*
 * Function:
 *      _bcm_ptp_external_stack_create
 * Purpose:
 *      Create a PTP stack instance
 * Parameters:
 *      unit - (IN) Unit number.
 *      ptp_info - (IN/OUT) Pointer to an PTP Stack Info structure
 * Returns:
 *      BCM_E_...
 * Notes:
 */
int
_bcm_ptp_external_stack_create(
    int unit,
    bcm_ptp_stack_info_t *info,
    bcm_ptp_stack_id_t ptp_id)
{
    _bcm_ptp_info_t *ptp_info_p;
    _bcm_ptp_stack_info_t *stack_p;
    uint32 tpidvlan;

    SET_PTP_INFO;
    if (!SOC_HAS_PTP_EXTERNAL_STACK_SUPPORT(unit)) {
        return BCM_E_UNAVAIL;
    }

    stack_p = &ptp_info_p->stack_info[ptp_id];

    /* Set up dispatch for external transport */
    stack_p->transport_init = _bcm_ptp_external_transport_init;
    stack_p->tx = _bcm_ptp_external_tx;
    stack_p->tx_completion = _bcm_ptp_external_tx_completion;
    stack_p->rx_free = _bcm_ptp_external_rx_response_free;
    
    /* stack_p->transport_terminate = _bcm_ptp_external_transport_terminate; */

    /* Assuming that the unit has been locked by the caller */
    sal_memcpy(&stack_p->ext_info, info->ext_stack_info, sizeof(bcm_ptp_external_stack_info_t));

    /* Set the PCI read and write functions */
    stack_p->ext_info.cookie = INT_TO_PTR(ptp_id);
    stack_p->ext_info.read_fn = &_bcm_ptp_read_pcishared_uint32;
    stack_p->ext_info.write_fn = &_bcm_ptp_write_pcishared_uint32;

    /* Config for Host <-> BCM53903 comms */
    esw_set_ext_stack_config_array(stack_p, CONFIG_HOST_OFFSET, stack_p->ext_info.host_mac, 6);

    esw_set_ext_stack_config_array(stack_p, CONFIG_HOST_OFFSET + 8, stack_p->ext_info.top_mac, 6);

    esw_set_ext_stack_config_uint32(stack_p, CONFIG_HOST_OFFSET + 16, stack_p->ext_info.host_ip_addr);

    esw_set_ext_stack_config_uint32(stack_p, CONFIG_HOST_OFFSET + 20, stack_p->ext_info.top_ip_addr);

    tpidvlan = 0x81000000 + ((int)(stack_p->ext_info.vlan_pri) << 13) + stack_p->ext_info.vlan;

    esw_set_ext_stack_config_uint32(stack_p, CONFIG_HOST_OFFSET + 24, tpidvlan);

    /* Config for BCM53903 that is currently hardwired on host side */
    /* outer / inner TPIDs for VLAN */
    esw_set_ext_stack_config_uint32(stack_p, CONFIG_VLAN_OFFSET, 0x91008100);
    /* MPLS label ethertype */
    esw_set_ext_stack_config_uint32(stack_p, CONFIG_MPLS_OFFSET, 0x88470000);

#if 0
    
    SOC_PBMP_PORT_ADD(pbmp, 0x03008000);
#endif

    /* Set config for loaded firmware */
    _bcm_ptp_write_pcishared_uint8_aligned_array(ptp_id, CONFIG_BASE, stack_p->persistent_config, CONFIG_TOTAL_SIZE);

    _bcm_ptp_ext_stack_start(ptp_id);

    return BCM_E_NONE;
}


int _bcm_ptp_ext_stack_reset(int pci_idx)
{
    /*** Reset Core & ChipCommon in DMP ***/
    /* CPU Master DMP : soft reset : resetctrl (0x18103800) <- 0x1 (bit[0] <- 1 enter reset) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18103800, 1));
    /* ChipCommon Master DMP : soft reset : resetctrl (0x18100800) <- 0x1 (bit[0] <- 1 enter reset) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18100800, 1));

    /* delay 1ms */
    sal_usleep(1000);

    /*** Enable Core & ChipCommon clocks and bring out of DMP reset ***/
    /* CPU Master DMP : enable clock : ioctrl    (0x18103408) <- 0x1 (bit[0] <- 1 enable clock) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18103408, 1));
    /* CPU Master DMP : soft reset : resetctrl (0x18103800) <- 0x0 (bit[0] <- 0 exit reset) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18103800, 0));
    /* ChipCommon Master DMP : enable clock : ioctrl    (0x18100408) <- 0x1 (bit[0] <- 1 enable clock) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18100408, 1));
    /* ChipCommon Master DMP : soft reset : resetctrl (0x18100800) <- 0x0 (bit[0] <- 1 exit reset) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18100800, 0));

    /*** Enable SOCRAM clocks and bring out of DMP reset ***/
    /* SOCRAM0 Slave DMP : enable clock : ioctrl (0x18107408) <- 0x1 (bit[0] <- 1 to enable clock) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18107408, 1));
    /* SOCRAM0 Slave DMP : enable clock : resetctrl (0x18107800) <- 0x0 (bit[0] <- 0 exit reset) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18107800, 0));

    /*** set MIPS resetvec to start of SOCRAM ***/
    /* Core Resetvec : set reset vector : resetvec (0x18003004) <- 0xb9000000 (start of SOCRAM) */
    BCM_IF_ERROR_RETURN(_bcm_ptp_write_ks_uint32(pci_idx, 0x18003004, 0xb9000000));

    /* MIPS Corecontrol : core soft reset : (0x18003000) <- 0x7 (bit[0] <- 1 force reset) */
    /*                                                          (bit[1] <- 1 alternate resetvec) */
    /*                                                          (bit[2] <- 1 HT clock) */
    _bcm_ptp_write_ks_uint32(pci_idx, 0x18003000, 7);

    return BCM_E_NONE;
}


#define MAGIC_READ_VALUE    0xdeadc0de

int _bcm_ptp_ext_stack_start(int pci_idx)
{
    int rv = BCM_E_NONE;
    int boot_iter;
    uint32 value = MAGIC_READ_VALUE;

    /* MIPS Corecontrol : core soft reset : (0x18003000) <- 0x6 (bit[0] <- 0 leave reset) */
    /*                                                          (bit[1] <- 1 alternate resetvec) */
    /*                                                          (bit[2] <- 1 HT clock) */
    _bcm_ptp_write_ks_uint32(pci_idx, 0x18003000, 6);

    for (boot_iter = 0; boot_iter < MAX_BOOT_ITER; ++boot_iter) {
        _bcm_ptp_read_ks_uint32(pci_idx, BOOT_STATUS_ADDR, &value);
        if (value != MAGIC_READ_VALUE) {
            break;
        }

        /* delay 1 ms */
        sal_usleep(1000);
    }

    if (boot_iter == MAX_BOOT_ITER) {
        rv = BCM_E_FAIL;
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META("external stack start failed")));
    }

    return rv;
}


/* Set a value both on BCM53903 and in the persistent config used to reset BCM53903 after a load. */
void
esw_set_ext_stack_config_uint32(_bcm_ptp_stack_info_t *stack_p, uint32 offset, uint32 value)
{
    stack_p->ext_info.write_fn(stack_p->ext_info.cookie, CONFIG_BASE + offset, value);
    _bcm_ptp_uint32_write(&stack_p->persistent_config[offset], value);
}


/* Set an array of values as above */
void
esw_set_ext_stack_config_array(_bcm_ptp_stack_info_t* stack_p, uint32 offset, const uint8 * array, int len)
{
    while (len--) {
        _bcm_ptp_write_pcishared_uint8(&stack_p->ext_info, CONFIG_BASE + offset, *array);
        stack_p->persistent_config[offset] = *array;
        ++array;
        ++offset;
    }
}


void _bcm_ptp_write_pcishared_uint8_aligned_array(int pci_idx, uint32 addr, uint8 * array, int array_len)
{
    while (array_len > 0) {
        uint32 value = ( (((uint32)array[0]) << 24) | (((uint32)array[1]) << 16) |
                         (((uint32)array[2]) << 8)  | (uint32)array[3] );
        _bcm_ptp_write_ks_uint32(pci_idx, addr, value);
        array += 4;
        array_len -= 4;
        addr += 4;
    }
}



#endif /* defined(BCM_PTP_EXTERNAL_STACK_SUPPORT) */
#endif /* defined(INCLUDE_PTP)*/
