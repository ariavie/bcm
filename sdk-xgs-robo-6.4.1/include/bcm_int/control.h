/*
 * $Id: control.h,v 1.18 Broadcom SDK $
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
 * BCM Control
 */

#ifndef	_BCM_INT_CONTROL_H
#define	_BCM_INT_CONTROL_H

#include <bcm/types.h>
#include <bcm/init.h>
#include <bcm_int/dispatch.h>

#ifndef	BCM_CONTROL_MAX
#define	BCM_CONTROL_MAX	BCM_UNITS_MAX
#endif

/*
 * BCM Supported Dispatch Type Enumeration. 
 */
#define BCM_DLIST_ENTRY(_dtype) \
bcmDtype_##_dtype,

typedef enum bcm_dtype_e {
#include <bcm_int/bcm_dlist.h>
    bcmTypeCount,
    bcmTypeNone = -1
} bcm_dtype_t; 

typedef struct {
    int			unit;		/* driver unit */
    void		*drv_control;	/* driver control structure */
    bcm_dtype_t         dtype;          /* driver dispatch type */
    const char*         name;           /* driver dispatch name */
    char		*subtype;	/* attached subtype (or NULL) */

    uint32		chip_vendor;	/* switch chip ident (pci id) */
    uint32		chip_device;
    uint32		chip_revision;
    uint32		capability;	/* chip capabilities */
#define		BCM_CAPA_SWITCH		BCM_INFO_SWITCH	/* net switch chip */
#define		BCM_CAPA_FABRIC		BCM_INFO_FABRIC	/* fabric chip */
#define		BCM_CAPA_L3		BCM_INFO_L3	/* chip has layer 3 */
#define		BCM_CAPA_IPMC		BCM_INFO_IPMC	/* chip has IP mcast */
#define		BCM_CAPA_LOCAL		0x1000	/* local (direct) dispatch */
#define		BCM_CAPA_REMOTE		0x2000	/* remotely dispatch */
#define		BCM_CAPA_COMPOSITE	0x4000	/* composite dispatch */
#define		BCM_CAPA_CALLBACK	0x8000	/* attach callback complete */

#if defined(BCM_SBX_SUPPORT)
#define         BCM_CAPA_SWITCH_SBX     0x10000 /* sbx switch chip */
#define         BCM_CAPA_FABRIC_SBX     0x20000 /* sbx fabric chip */
#else
#define         BCM_CAPA_SWITCH_SBX     0
#define         BCM_CAPA_FABRIC_SBX     0
#endif /* BCM_SBX_SUPPORT */

    /* per module storage */
    void		*data_auth;
    void		*data_i2c;
    void		*data_cosq;
    void		*data_diffserv;
    void		*data_dmux;
    void		*data_filter;
    void		*data_init;
    void		*data_ipmc;
    void		*data_l2;
    void		*data_l3;
    void		*data_link;
    void		*data_mcast;
    void		*data_meter;
    void		*data_mirror;
    void		*data_pkt;
    void		*data_port;
    void		*data_rate;
    void		*data_rx;
    void		*data_stack;
    void		*data_stat;
    void		*data_stg;
    void		*data_switch;
    void		*data_trunk;
    void		*data_tx;
    void		*data_vlan;
    void		*data_misc1;
    void		*data_misc2;
    void		*data_misc3;
    void		*data_misc4;
} bcm_control_t;

extern bcm_control_t	*bcm_control[BCM_CONTROL_MAX];

#define		BCM_CONTROL(_unit)	bcm_control[_unit]
#define		BCM_DISPATCH(_unit)	BCM_CONTROL(_unit)->dispatch

/* BCM control unit in legal range? */
#define         BCM_CONTROL_UNIT_LEGAL(_unit) \
    (((_unit) >= 0) && ((_unit) < BCM_CONTROL_MAX))

/* use below dispatch layer or during init/deinit */
#define		BCM_CONTROL_UNIT_VALID(_unit) \
    ((BCM_CONTROL_UNIT_LEGAL(_unit)) && (BCM_CONTROL(_unit) != NULL))

/* use at dispatch layer and above */
#ifdef BCM_CONTROL_API_TRACKING

extern uint32   bcm_control_unit_valid[BCM_CONTROL_MAX];
extern int      bcm_unit_refcount(int unit, int refcount);


#define		BCM_UNIT_VALID(_unit)	(BCM_CONTROL_UNIT_LEGAL(_unit) && \
					 bcm_control_unit_valid[_unit] != 0)

/* BCM_UNIT_CHECK/BCM_UNIT_IDLE and
   BCM_UNIT_BUSY/BCM_UNIT_IDLE must always be used in pairs.
   BCM_UNIT_CHECK() returns a unit valid flag and is used as a predicate.
   BCM_UNIT_BUSY() does not return a flag and is used standalone.
   BCM_UNIT_IDLE() is always used standalone.
*/
#define         BCM_UNIT_CHECK(unit) bcm_unit_refcount((unit), 1)
#define         BCM_UNIT_BUSY(unit) ((void)BCM_UNIT_CHECK(unit))
#define         BCM_UNIT_IDLE(unit) ((void)bcm_unit_refcount((unit), -1))

#else /* !BCM_CONTROL_API_TRACKING */

#define		BCM_UNIT_VALID(_unit) BCM_CONTROL_UNIT_VALID(_unit)
#define         BCM_UNIT_CHECK(unit) BCM_UNIT_VALID(unit) 
#define         BCM_UNIT_BUSY(unit) 
#define         BCM_UNIT_IDLE(unit)

#endif /* BCM_CONTROL_API_TRACKING */

#define		BCM_CAPABILITY(_unit)	BCM_CONTROL(_unit)->capability
#define		BCM_IS_LOCAL(_unit)	(BCM_CAPABILITY(_unit) & \
					 BCM_CAPA_LOCAL)
#define		BCM_IS_REMOTE(_unit)	(BCM_CAPABILITY(_unit) & \
					 BCM_CAPA_REMOTE)
#define		BCM_IS_SWITCH(_unit)	(BCM_CAPABILITY(_unit) & \
					 BCM_CAPA_SWITCH)
#define		BCM_IS_FABRIC(_unit)	(BCM_CAPABILITY(_unit) & \
					 BCM_CAPA_FABRIC)
#define         BCM_IS_SWITCH_SBX(_unit) (BCM_CAPABILITY(_unit) & \
					  BCM_CAPA_SWITCH_SBX)
#define         BCM_IS_FABRIC_SBX(_unit) (BCM_CAPABILITY(_unit) & \
					  BCM_CAPA_FABRIC_SBX)
#define         BCM_DTYPE(_unit)        BCM_CONTROL(_unit)->dtype
#define         BCM_TYPE_NAME(_unit)     BCM_CONTROL(_unit)->name

#endif	/* _BCM_INT_CONTROL_H */
