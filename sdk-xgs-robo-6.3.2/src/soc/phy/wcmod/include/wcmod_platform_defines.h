/*
 * $Id: wcmod_platform_defines.h 1.2 Broadcom SDK $
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
 *
 */
#include <sal/types.h>
#ifndef _WCMOD_PLATFORM_DEFINES_H_
#define _WCMOD_PLATFORM_DEFINES_H_

#include "wcmod_defines.h"
#include "wcmod_enum_defines.h"

#ifdef _SDK_WCMOD_
#include <sal/core/sync.h>
#else
#include "sync.h"
#endif /* _SDK_WCMOD_ */

#if defined (_DV_ODYSSEY)

typedef enum {CHIP, TESTBENCH, DEVICE_ILLEGAL } device_type;

typedef struct {
	int              core_num; /* the serdes number on the chip */
	device_type      device;   /* indicates either chip or testbench */
	wcmod_model_type core;     /* indicates the type of core */
} platform_info_type;

#elif defined (_DV_TRIUMPH3)

#define MAX_CORES 18 /* maximum number of cores for a particular core_type */

typedef enum {CHIP,
              TESTBENCH,
              DEVICE_ILLEGAL /* This must be last */
} device_type;

typedef struct {
  int                 core_num; /* the serdes number on the chip */
  device_type         device;   /* indicates either chip or testbench */
  wcmod_model_type    core;     /* indicates the type of core */
} platform_info_type;

#elif defined (_DV_NATIVE)
typedef struct {
  int unit;
  int prt_ad;
  int phy_ad;
} platform_info_type;

#elif defined (_DV_REDSTONE)
typedef struct {
  int unit;
  int prt_ad;
  int phy_ad;
} platform_info_type;

#elif defined (STANDALONE)
/* dummy platform_info_type. */
typedef struct {
  int id; /* dummy  */
} platform_info_type;

#elif defined (_MDK_WCMOD_)

typedef struct {
  int id; /* unique id (core_num, device, core)-tuple that indicates what mutex to use */
} platform_info_type;

#else
  BAD_DEFINES_IN_WCMOD_PLATFORM_DEFINES_H
#endif

int init_platform_info(platform_info_type* pi);
int init_wcmod_st     (wcmod_st* ws);
int init_platform_info_from_wcmod_st(wcmod_st* from, platform_info_type* pi);
int init_platform_info_to_wcmod_st  (wcmod_st* to,   platform_info_type* pi);
int copy_platform_info(platform_info_type* from, platform_info_type* pi);

#endif /* _WCMOD_PLATFORM_DEFINES_H_ */
