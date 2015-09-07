/*
 * $Id: ptable.h 1.3 Broadcom SDK $
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

#ifndef _SOC_PTABLE_H
#define _SOC_PTABLE_H

/*
 * PTABLE bit definitions
 */

/* Values for SP_ST */

#define PVP_STP_DISABLED	0	/* Disabled */
#define PVP_STP_BLOCKING	1	/* Blocking/Listening */
#define PVP_STP_LEARNING	2	/* Learning */
#define PVP_STP_FORWARDING	3	/* Forwarding */

/*
 * Values for CML (control what happens on Source Lookup Failure packet).
 * The last two are for Draco only; on StrataSwitch CML is only 2 bits.
 */

#define	PVP_CML_SWITCH		0	/*   Learn ARL, !CPU,  Forward */
#define	PVP_CML_CPU		1	/*  !Learn ARL,  CPU, !Forward */
#define	PVP_CML_FORWARD		2	/*  !Learn ARL, !CPU,  Forward */
#define	PVP_CML_DROP		3	/*  !Learn ARL, !CPU, !Forward */
#define PVP_CML_CPU_SWITCH	4	/*   Learn ARL,  CPU,  Forward */
#define PVP_CML_CPU_FORWARD 	5	/*  !Learn ARL,  CPU,  Forward */

/* Egress port table redirection types */
#define SOC_EPT_REDIRECT_NOOP           0       /* NOOP - do not redirect */
#define SOC_EPT_REDIRECT_MIRROR         4       /* Mirror bitmap */
#define SOC_EPT_REDIRECT_SYS_PORT       5       /* System port */
#define SOC_EPT_REDIRECT_TRUNK          6       /* Trunk group */
#define SOC_EPT_REDIRECT_L2MC           7       /* L2MC group */

#endif	/* !_SOC_PTABLE_H */
