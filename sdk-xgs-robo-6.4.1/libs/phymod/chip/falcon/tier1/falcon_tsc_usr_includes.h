/**************************************************************************************/
/*  File Name     :  falcon_tsc_usr_includes.h                                        */
/*  Created On    :  05/07/2014                                                       */
/*  Created By    :  Kiran Divakar                                                    */
/*  Description   :  Header file which includes all required std libraries and macros */
/*  Revision      :  $Id: falcon_tsc_usr_includes.h 503 2014-05-15 21:01:42Z kirand $ */
/*                                                                                    */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                     */
/*  All Rights Reserved                                                               */
/*  No portions of this material may be reproduced in any form without                */
/*  the written permission of:                                                        */
/*      Broadcom Corporation                                                          */
/*      5300 California Avenue                                                        */
/*      Irvine, CA  92617                                                             */
/*                                                                                    */
/*  All information contained in this document is Broadcom Corporation                */
/*  company private proprietary, and trade secret.                                    */
/*                                                                                    */
/**************************************************************************************/
/**************************************************************************************/
/*
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

/** @file falcon_tsc_usr_includes.h
 * Header file which includes all required std libraries and macros 
 */
#ifndef FALCON_TSC_API_USR_INCLUDES_H
#define FALCON_TSC_API_USR_INCLUDES_H

/* Standard libraries that can be replaced by your custom libraries */
#ifdef _MSC_VER
/* Enclose all standard headers in a pragma to remove warings for MS compiler */
#pragma warning( push, 0 )
#endif
/* #include <stdint.h> */
/* #include <string.h> */
#ifdef _MSC_VER
#pragma warning( pop )
#endif

/* Redefine macros according your compiler requirements */
/* #define USR_PRINTF(stuff)             usr_logger_write stuff */
#define USR_PRINTF(stuff)             PHYMOD_DEBUG_ERROR(stuff)
#define USR_MEMSET(mem, val, num)     memset(mem, val, num)
#define USR_STRLEN(string)            strlen(string)
#define USR_STRCAT(str1, str2)        strcat(str1, str2)
#define USR_STRCPY(str1, str2)        strcpy(str1, str2)
#define USR_STRCMP(str1, str2)        strcmp(str1, str2)
#define USR_STRNCMP(str1, str2, num)  strncmp(str1, str2, num)
#define USR_DOUBLE                    int 

/* Implementation specific macros below */
/* #define usr_logger_write(...) logger_write(-1,__VA_ARGS__) */

#endif
