/*
 * $Id: et_dbg.h 1.7 Broadcom SDK $
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
 * Minimal debug/trace/assert driver definitions for
 * Broadcom Home Networking Division 10/100 Mbit/s Ethernet
 * Device Driver.
 */

#ifndef _et_dbg_
#define _et_dbg_

#ifdef	BCMDBG
/*
 * et_msg_level is a bitvector:
 *	0	errors
 *	1	function-level tracing
 *	2	one-line frame tx/rx summary
 *	3	complex frame tx/rx in hex
 */
#define	ET_ERROR(args)	if (!(et_msg_level & 1)) ; else soc_cm_print args
#define	ET_TRACE(args)	if (!(et_msg_level & 2)) ; else soc_cm_print args
#define	ET_PRHDR(msg, eh, len)	if (!(et_msg_level & 4)) ; else etc_soc_prhdr(msg, eh, len)
#define	ET_PRPKT(msg, buf, len)	if (!(et_msg_level & 8)) ; else prhex(msg, buf, len)
#define	EB_TRACE(args)	if (!(et_msg_level & 0x10)) ; else soc_cm_print args
extern void etc_soc_prhdr(char *msg, struct ether_header *eh, uint len);
#else	/* BCMDBG */
#define	ET_ERROR(args)
#define	ET_TRACE(args)
#define	ET_PRHDR(msg, eh, len)
#define	ET_PRPKT(msg, buf, len)
#define	EB_TRACE(args)
#endif	/* BCMDBG */

extern int et_msg_level;

#ifdef BCMINTERNAL
#define	ET_LOG(fmt, a1, a2)	if (!(et_msg_level & 0x10000)) ; else bcmlog(fmt, a1, a2)
#else
#define	ET_LOG(fmt, a1, a2)
#endif

/* include port-specific tunables */
#ifdef NDIS
#include <et_ndis.h>
#elif defined(vxworks)
#include <et_vx.h>
#elif defined(linux) && defined(__KERNEL__)
#include <soc/et_soc.h>
#elif PMON
#include <et_pmon.h>
#elif _CFE_
#include <et_cfe.h>
#else
#include <soc/et_soc.h>
#endif

#endif /* _et_dbg_ */
