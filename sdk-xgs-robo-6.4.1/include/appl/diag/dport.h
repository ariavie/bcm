/*
 * $Id: dport.h,v 1.2 Broadcom SDK $
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
 * The diag shell port mapping functions are located in the SOC layer
 * for historical reasons, but the mappings are not used internally
 * in either the SOC layer or the BCM layer.
 *
 */

#ifndef __DIAG_DPORT_H__
#define __DIAG_DPORT_H__

#include <sal/types.h>

#include <soc/dport.h>

#include <bcm_int/api_xlate_port.h>

#ifdef INCLUDE_BCM_API_XLATE_PORT
#define XLATE_P2A(_u,_p) BCM_API_XLATE_PORT_P2A(_u,&_p) == 0
#else
#define XLATE_P2A(_u,_p) 1
#endif

/* Iterate over BCM port bitmap in user port order */
#define DPORT_BCM_PBMP_ITER(_u,_pbm,_dport,_p) \
        for ((_dport) = 0, (_p) = -1; (_dport) < SOC_DPORT_MAX; (_dport)++) \
                if (((_p) = soc_dport_to_port(_u,_dport)) >= 0 && \
                    XLATE_P2A(_u,_p) && SOC_PBMP_MEMBER((_pbm),(_p)))

/* Iterate over (SOC) port bitmap in user port order */
#define DPORT_SOC_PBMP_ITER(_unit,_pbm,_dport,_port) \
    SOC_DPORT_PBMP_ITER(_unit,_pbm,_dport,_port)

/* Tranalate user port number to internal port number */
#define DPORT_TO_PORT(_unit,_dport) \
    soc_dport_to_port(_unit,_dport)

/* Tranalate internal port number to user port number */
#define DPORT_FROM_PORT(_unit,_port) \
    soc_dport_from_port(_unit,_port)

/* Get user port number or port index based on currrent configuration */
#define DPORT_FROM_DPORT_INDEX(_unit,_dport,_i) \
    soc_dport_from_dport_idx(_unit,_dport,_i)

/* Map a user port number to an internal port number */
#define DPORT_MAP_PORT(_unit,_dport,_port) \
    soc_dport_map_port(_unit,_dport,_port)

/* Update dport mappings and port names */
#define DPORT_MAP_UPDATE(_unit) \
    soc_dport_map_update(_unit)

#endif /* __DIAG_DPORT_H__ */
