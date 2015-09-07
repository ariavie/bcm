/*
 * $Id: api_xlate_port.h 1.9 Broadcom SDK $
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
 * BCM internal port translation functions
 */

#ifndef _BCM_XLATE_PORT_H
#define _BCM_XLATE_PORT_H

#include <sal/core/libc.h> /* sal_memcpy */
#include <bcm/types.h>

extern int _bcm_api_xlate_port_init(int unit);

extern int _bcm_api_xlate_port_cleanup(int unit);

extern int _bcm_api_xlate_port_map(int unit, bcm_port_t aport, bcm_port_t pport);

extern int _bcm_api_xlate_port_a2p(int unit, bcm_port_t *port);

extern int _bcm_api_xlate_port_p2a(int unit, bcm_port_t *port);

extern int _bcm_api_xlate_port_pbmp_a2p(int unit, bcm_pbmp_t *pbmp);

extern int _bcm_api_xlate_port_pbmp_p2a(int unit, bcm_pbmp_t *pbmp);

extern int _bcm_xlate_sysport_s2p(int unit, bcm_port_t *port);

extern int _bcm_xlate_sysport_p2s(int unit, bcm_port_t *port);

#ifdef INCLUDE_BCM_API_XLATE_PORT

#define BCM_API_XLATE_PORT_A2P(_u,_p)           _bcm_api_xlate_port_a2p(_u,(bcm_port_t*)(_p))
#define BCM_API_XLATE_PORT_P2A(_u,_p)           _bcm_api_xlate_port_p2a(_u,(bcm_port_t*)(_p))
#define BCM_API_XLATE_PORT_DECL(_var)           bcm_port_t _var = 0
#define BCM_API_XLATE_PORT_ASSIGN(_dst,_src)    _dst = _src
#define BCM_API_XLATE_PORT_SAVE(_dst,_src)      BCM_API_XLATE_PORT_ASSIGN(_dst,_src)
#define BCM_API_XLATE_PORT_RESTORE(_dst,_src)   BCM_API_XLATE_PORT_ASSIGN(_dst,_src)

#define BCM_API_XLATE_PORT_PBMP_A2P(_u,_p)      _bcm_api_xlate_port_pbmp_a2p(_u,(_p))
#define BCM_API_XLATE_PORT_PBMP_P2A(_u,_p)      _bcm_api_xlate_port_pbmp_p2a(_u,(_p))
#define BCM_API_XLATE_PORT_PBMP_DECL(_t)        bcm_pbmp_t _t
/* We avoid BCM_PBMP_ASSIGN since this may cause (invalid) compiler warnings */
#define BCM_API_XLATE_PORT_PBMP_ASSIGN(_t,_p)   sal_memcpy(&(_t),&(_p),sizeof(_t))
#define BCM_API_XLATE_PORT_PBMP_SAVE(_t,_p)     BCM_API_XLATE_PORT_PBMP_ASSIGN(_t,_p)
#define BCM_API_XLATE_PORT_PBMP_RESTORE(_p,_t)  BCM_API_XLATE_PORT_PBMP_ASSIGN(_p,_t)

#define BCM_API_XLATE_PORT_COND(_c)             if(_c)
#define BCM_API_XLATE_PORT_ITER_DECL(_i)        int _i
#define BCM_API_XLATE_PORT_ITER(_c,_i)          for(_i=0;_i<(_c);_i++)

#else

#define BCM_API_XLATE_PORT_A2P(_u,_p)
#define BCM_API_XLATE_PORT_P2A(_u,_p)
#define BCM_API_XLATE_PORT_DECL(_var)
#define BCM_API_XLATE_PORT_ASSIGN(_dst,_src)
#define BCM_API_XLATE_PORT_SAVE(_dst,_src)
#define BCM_API_XLATE_PORT_RESTORE(_dst,_src)

#define BCM_API_XLATE_PORT_PBMP_A2P(_u,_p)
#define BCM_API_XLATE_PORT_PBMP_P2A(_u,_p)
#define BCM_API_XLATE_PORT_PBMP_DECL(_t)
#define BCM_API_XLATE_PORT_PBMP_ASSIGN(_t,_p)
#define BCM_API_XLATE_PORT_PBMP_SAVE(_t,_p)
#define BCM_API_XLATE_PORT_PBMP_RESTORE(_p,_t)

#define BCM_API_XLATE_PORT_COND(_c)
#define BCM_API_XLATE_PORT_ITER_DECL(_t)
#define BCM_API_XLATE_PORT_ITER(_c,_i)

#endif /* BCM_INCLUDE_API_XLATE_PORT */

#define BCM_XLATE_SYSPORT_S2P(_u,_p)            _bcm_xlate_sysport_s2p(_u,(bcm_port_t*)(_p))
#define BCM_XLATE_SYSPORT_P2S(_u,_p)            _bcm_xlate_sysport_p2s(_u,(bcm_port_t*)(_p))

#endif /* _BCM_XLATE_PORT_H */

