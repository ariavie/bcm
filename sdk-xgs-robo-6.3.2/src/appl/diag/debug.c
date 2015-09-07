/*
 * $Id: debug.c 1.19 Broadcom SDK $
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
 * socdiag debug commands
 */

#include <appl/diag/system.h>
#include <soc/mem.h>

#include <soc/devids.h>
#include <soc/debug.h>

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#ifdef BROADCOM_DEBUG

/*
 * INTERNAL: Slot header format, for chip debug only
 */
typedef struct slot_header_s {
#if defined(LE_HOST)
    uint32 timestamp:14,        /* Timestamp for this Packet */
           ipx:1,               /* IPX Packet */
           ip:1,                /* IP Packet */
           BCxMC:1,             /* This is a Broadcast or Multicast Packet */
           O:2,                 /* Router help bits */
           cell_len:6,          /* How many bytes are in this cell */
           crc:2,               /* Tells Egress how to treat CRC */
           lc:1,                /* Last Cell of Packet */
           fc:1,                /* First Cell of Packet */
           purge:1,             /* Purge this packet */
           ncl:2;               /* Next Cell Length */
    uint32 cpu_opcode:4,        /* Opcode to help out the CPU */
           _pad0:1,             /* Reserved */
           slot_size:6,         /* Slot Size */
           copy_count:5,        /* How many copies of this slot are valid */
           nsp:16;              /* Next Slot Pointer */
    uint32 ls:1,                /* Last Slot in Chain */
           src_port:5,          /* The source port for this packet */
           _pad1:7,             /* Reserved */
           slot_csum_lo:19;     /* Slot Checksum (lower bits) */
    uint32 slot_csum_hi;        /* Slot Checksum (upper bits) */
#else /* BE host */
    uint32 ncl:2,
           purge:1,
           fc:1,
           lc:1,
           crc:2,
           cell_len:6,
           O:2,
           BCxMC:1,
           ip:1,
           ipx:1,
           timestamp:14;
    uint32 nsp:16,
           copy_count:5,
           slot_size:6,
           _pad0:1,
           cpu_opcode:4;
    uint32 slot_csum_lo:19,
           _pad1:7,
           src_port:5,
           ls:1;
    uint32 slot_csum_hi;
#endif
} slot_header_t;

#endif /* BROADCOM_DEBUG */
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */

char bkpmon_usage[] =
"Parameters: none\n\t"
"Monitors for backpressure discard messages.\n\t"
"Displays the gcccount registers when changes occur.\n";

cmd_result_t
dbg_bkpmon(int unit, args_t *a)
{
    uint32 curBkpReg, prevBkpReg;

    if (! sh_check_attached(ARG_CMD(a), unit))
        return CMD_FAIL;

    prevBkpReg = 0;

    printk("Monitoring Backpressure discard messages\n");

    for (;;) {
        while ((curBkpReg = soc_pci_read(unit,
                                            CMIC_IGBP_DISCARD)) == prevBkpReg)
            ;
        printk("CMIC Bkp Register = 0x%x\n", curBkpReg);
        sh_process_command(unit, "getreg gcccount");
        prevBkpReg = curBkpReg;
    }

    /* NOTREACHED */
}
