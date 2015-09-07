/*
 * $Id: cmdebug.h 1.2 Broadcom SDK $
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

#ifndef _SOC_CM_DEBUG_H
#define _SOC_CM_DEBUG_H

#include <sal/types.h>
#include <sal/core/libc.h>

/*
 * These are the possible debug types/flags used by the driver output
 * routine soc_cm_debug().
 */

#define DK_PCI         (1 << 0)    /* PCI reads/writes */
#define DK_SCHAN       (1 << 1)    /* S-Channel operations */
#define DK_SOCMEM      (1 << 2)    /* Memory table operations */
#define DK_SYMTAB      (1 << 3)    /* Symbol parsing routines */
#define DK_VERINET     (1 << 4)    /* Verilog PLI transactions */
#define DK_L3          (1 << 5)    /* L3, DefIP, IPMC and MPLS */
#define DK_INTR        (1 << 6)    /* Interrupt processing */
#define DK_ARL         (1 << 7)    /* ARL DMA and poll routines */
#define DK_ARLMON      (1 << 8)    /* Monitor ARL ins/del */
#define DK_L2TABLE     (1 << 9)    /* Debug software L2 table */
#define DK_DMA         (1 << 10)   /* DMA operations */
#define DK_PORT        (1 << 11)   /* Port operations */
#define DK_PACKET      (1 << 12)   /* Loopback packet data */
#define DK_TX          (1 << 13)   /* Packet transmit */
#define DK_RCLOAD      (1 << 14)   /* Echo cmds before exec */
#define DK_TESTS       (1 << 15)   /* Verbose during tests */
#define DK_VERBOSE     (1 << 16)   /* General verbose output */
#define DK_MIIM        (1 << 17)   /* MIIM register operations */
#define DK_PHY         (1 << 18)   /* PHY operations */
#define DK_END         (1 << 19)   /* Show END driver output */
#define DK_LINK        (1 << 20)   /* Show Link status changes */
#define DK_ERR         (1 << 21)   /* Print errors */
#define DK_COUNTER     (1 << 22)   /* Counter DMA, etc */
#define DK_IP          (1 << 23)   /* IP/Ethernet Stack */
#define DK_STP         (1 << 24)   /* 802.1D Spanning Tree */
#define DK_VLAN        (1 << 25)   /* VLAN gateway */
#define DK_RX          (1 << 26)   /* Packet Receive */
#define DK_WARN        (1 << 27)   /* Print warnings */
#define DK_I2C         (1 << 28)   /* I2C driver */
#define DK_REG         (1 << 29)   /* I2C driver */
#define DK_MEM         (1 << 30)   /* Memory Driver */
#define DK_STK         (1 << 31)   /* Stacking */

/*
 * SOC_DEBUG_PRINT - Debug messages, compiled conditionally under DEBUG_PRINT
 * SOC_ERROR_PRINT - Error messages, compiled conditionally under DEBUG_ERROR
 * SOC_DEBUG_PRINT_IF_ERROR - conditionally compiled in under DEBUG_PRINT, 
 *        calls debug print routine if first parameter if a BCM/SOC 
 *        error.
 */

#define    _SOC_DEBUG_IF(_x, _s)    if ((_x) < 0) soc_cm_debug _s

#if defined(BROADCOM_DEBUG) || defined(DEBUG_PRINT)
#   define    SOC_DEBUG_PRINT(_x)        soc_cm_debug _x
#   define    SOC_IF_ERROR_DEBUG_PRINT(_x, _s) _SOC_DEBUG_IF(_x, _s)
#else
#   define    SOC_DEBUG_PRINT(_x)    ((void)0)
#   define    SOC_IF_ERROR_DEBUG_PRINT(_x, _s) ((void)(_x))
#endif

#if defined(BROADCOM_DEBUG) || defined(DEBUG_ERROR)
#   define    SOC_ERROR_PRINT(_x)    soc_cm_debug _x
#else
#   define    SOC_ERROR_PRINT(_x)    ((void)0)
#endif

/* Names of debug flags by bit position (see soc/debug.h) */

extern char     *soc_cm_debug_names[32];

/* Debug Output */

extern int      soc_cm_print(const char *format, ...)
                             COMPILER_ATTRIBUTE ((format (printf, 1, 2)));
extern int      soc_cm_vprint(const char *fmt, va_list varg)
                             COMPILER_ATTRIBUTE ((format (printf, 1, 0)));
extern int      soc_cm_debug(uint32 flags, const char *format, ...)
                             COMPILER_ATTRIBUTE ((format (printf, 2, 3)));
extern int      soc_cm_debug_check(uint32 flags);
extern int      soc_cm_dump(int dev);

#endif  /* !_SOC_CM_DEBUG_H */
