/*
 * $Id: debug.h 1.4 Broadcom SDK $
 *
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

/******************************************************************************/
/*                                                                            */
/* Broadcom BCM5700 Linux Network Driver, Copyright (c) 2000 Broadcom         */
/* Corporation.                                                               */
/* All rights reserved.                                                       */
/*                                                                            */
/* History:                                                                   */
/*    02/25/00 Hav Khauv        Initial version.                              */
/******************************************************************************/

#ifndef DEBUG_H 
#define DEBUG_H 

#ifdef VXWORKS
#include <vxWorks.h>
#endif

/******************************************************************************/
/* Debug macros                                                               */
/******************************************************************************/

/* Code path for controlling output debug messages. */
/* Define your code path here. */
#define CP_INIT                     0x010000
#define CP_SEND                     0x020000
#define CP_RCV                      0x040000
#define CP_INT                      0x080000
#define CP_UINIT                    0x100000
#define CP_RESET                    0x200000

#define CP_ALL                      (CP_INIT | CP_SEND | CP_RCV | CP_INT | \
                                    CP_RESET | CP_UINIT)

#define CP_MASK                     0xffff0000


/* Debug message levels. */
#define LV_VERBOSE                  0x03
#define LV_INFORM                   0x02
#define LV_WARN                     0x01
#define LV_FATAL                    0x00

#define LV_MASK                     0xffff


/* Code path and messsage level combined.  These are the first argument of */
/* the DbgMessage macro. */
#define INIT_V                      (CP_INIT | LV_VERBOSE)
#define INIT_I                      (CP_INIT | LV_INFORM)
#define INIT_W                      (CP_INIT | LV_WARN)
#define SEND_V                      (CP_SEND | LV_VERBOSE)
#define SEND_I                      (CP_SEND | LV_INFORM)
#define SEND_W                      (CP_SEND | LV_WARN)
#define RCV_V                       (CP_RCV | LV_VERBOSE)
#define RCV_I                       (CP_RCV | LV_INFORM)
#define RCV_W                       (CP_RCV | LV_WARN)
#define INT_V                       (CP_INT | LV_VERBOSE)
#define INT_I                       (CP_INT | LV_INFORM)
#define INT_W                       (CP_INT | LV_WARN)
#define UINIT_V                     (CP_UINIT | LV_VERBOSE)
#define UINIT_I                     (CP_UINIT | LV_INFORM)
#define UINIT_W                     (CP_UINIT | LV_WARN)
#define RESET_V                     (CP_RESET | LV_VERBOSE)
#define RESET_I                     (CP_RESET | LV_INFORM)
#define RESET_W                     (CP_RESET | LV_WARN)
#define CPALL_V                     (CP_ALL | LV_VERBOSE)
#define CPALL_I                     (CP_ALL | LV_INFORM)
#define CPALL_W                     (CP_ALL | LV_WARN)


/* All code path message levels. */
#define FATAL                       (CP_ALL | LV_FATAL)
#define WARN                        (CP_ALL | LV_WARN)
#define INFORM                      (CP_ALL | LV_INFORM)
#define VERBOSE                     (CP_ALL | LV_VERBOSE)


/* These constants control the message output. */
/* Set your debug message output level and code path here. */
#ifndef DBG_MSG_CP
#define DBG_MSG_CP                  CP_ALL      /* Where to output messages. */
#endif

#ifndef DBG_MSG_LV
#define DBG_MSG_LV                  LV_VERBOSE  /* Level of message output. */
#endif

/* DbgMessage macro. */
#if DBG
#define DbgMessage(CNTRL, MESSAGE)  \
    if((CNTRL & DBG_MSG_CP) && ((CNTRL & LV_MASK) <= DBG_MSG_LV)) \
        printf MESSAGE
#define DbgBreak()                 DbgBreakPoint() 
#undef STATIC
#define STATIC
#else
#define DbgMessage(CNTRL, MESSAGE)
#define DbgBreak()
#undef STATIC
#define STATIC static
#endif /* DBG */



#endif /* DEBUG_H */

