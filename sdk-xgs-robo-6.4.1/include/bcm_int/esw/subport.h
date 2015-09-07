/*
 * $Id: subport.h,v 1.22 Broadcom SDK $
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
 * This file contains subport definitions internal to the BCM library.
 */

#include <bcm/subport.h>

#ifndef _BCM_INT_ESW_SUBPORT_H_
#define _BCM_INT_ESW_SUBPORT_H_

#ifdef BCM_KATANA2_SUPPORT

/* Subport Common defines */
#define _BCM_KT2_PORT_TYPE_ETHERNET         0
#define _BCM_KT2_PORT_TYPE_CASCADED         4
#define _BCM_KT2_PORT_TYPE_CASCADED_LINKPHY 0
#define _BCM_KT2_PORT_TYPE_CASCADED_SUBTAG  1

#define _BCM_KT2_SUBPORT_GROUP_MAX        (128)
#define _BCM_KT2_SUBPORT_PORT_MAX         (128)

#define _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN    (42) 
#define _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX    (\
    _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN + _BCM_KT2_SUBPORT_PORT_MAX - 1) 

#define _BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(_unit, _gport) \
    (BCM_GPORT_IS_SUBPORT_PORT(_gport) && \
        ((soc_feature(_unit, soc_feature_linkphy_coe) && \
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT (_unit, _gport)) || \
        (soc_feature(_unit, soc_feature_subtag_coe) && \
         _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT (_unit, _gport))))

/* LinkPHY subport specific defines */
#define _BCM_KT2_LINKPHY_STREAMS_MAX                1024
#define _BCM_KT2_LINKPHY_PER_PORT_STREAMS_MAX       128

#define _BCM_KT2_LINKPHY_TX_DATA_BUF_START_ADDR_MAX 3399
#define _BCM_KT2_LINKPHY_TX_DATA_BUF_END_ADDR_MIN   23
#define _BCM_KT2_LINKPHY_TX_DATA_BUF_END_ADDR_MAX   3423

#define _BCM_KT2_LINKPHY_STREAM_ID_FLUSH_TIMEOUT    20000

/*
* Katana2 LinkPHY/SubTag subport feature supports
* 1.creation of multiple logical subport groups per port.
*        Port/trunk_id and group id are stored in subport group gport.
* 2.attachment of multiple subport ports to a subport group
*       A subport port is represented by
*       a. (port, external stream id array)] for LinkPHY subport
*       b. (port, VLAN) for SubTag subport
*       Group id and subport_port id are stored in subport port gport.
*/

#define _BCM_KT2_SUBPORT_TYPE_ETHERNET        0
#define _BCM_KT2_SUBPORT_TYPE_CASCADED        1
#define _BCM_KT2_SUBPORT_TYPE_LINKPHY         2
#define _BCM_KT2_SUBPORT_TYPE_SUBTAG          3

/*
* LinkPHY/SubTag Subport group gport format 
* bit 31-26 -- gport type (subport group)
* bit 25-24  -- subport group type (cascaded)
* bit 18-17 -- subport subtype (linkphy / subtag )
* bit 17    -- subport group is trunk indicator
* bit 16-9  -- subport group port number/trunk group id
* bit 8 -0  -- subport group index
*/
#define _BCM_KT2_SUBPORT_GROUP_TYPE_MASK      0x3
#define _BCM_KT2_SUBPORT_GROUP_TYPE_SHIFT     24
#define _BCM_KT2_SUBPORT_GROUP_SUBTYPE_MASK   0x3
#define _BCM_KT2_SUBPORT_GROUP_SUBTYPE_SHIFT  18
#define _BCM_KT2_SUBPORT_GROUP_PORT_MASK      0xFF
#define _BCM_KT2_SUBPORT_GROUP_PORT_SHIFT     9
#define _BCM_KT2_SUBPORT_GROUP_SPGID_MASK     0x1FF
#define _BCM_KT2_SUBPORT_GROUP_SPGID_SHIFT    0

/*
* LinkPHY/SubTag Subport port gport format 
* bit 31-26  -- gport type (subport port)
* bit 25-24  -- subport port type (LinkPHY / SubTag)
* bit 23-08  -- zero
* bit 07-00  -- subport port index  (range 42 to 169)
*/

#define _BCM_KT2_SUBPORT_PORT_TYPE_MASK           0x3
#define _BCM_KT2_SUBPORT_PORT_TYPE_SHIFT          24
#define _BCM_KT2_SUBPORT_PORT_ZERO_BITS_MASK      0xFFFF
#define _BCM_KT2_SUBPORT_PORT_ZERO_BITS_SHIFT     8
#define _BCM_KT2_SUBPORT_PORT_PP_PORT_MASK        0xFF
#define _BCM_KT2_SUBPORT_PORT_PP_PORT_SHIFT       0

/* Check if gport is LinkPHY subport_group */
#define _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(_subport_group)    \
        ((((_subport_group >> _BCM_KT2_SUBPORT_GROUP_SUBTYPE_SHIFT) & \
            _BCM_KT2_SUBPORT_GROUP_SUBTYPE_MASK) ==    \
            _BCM_KT2_SUBPORT_TYPE_LINKPHY) && \
         (((_subport_group >> _BCM_KT2_SUBPORT_GROUP_TYPE_SHIFT) & \
            _BCM_KT2_SUBPORT_GROUP_TYPE_MASK) ==    \
            _BCM_KT2_SUBPORT_TYPE_CASCADED))

/* Check if gport is SubTag subport_group */
#define _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(_subport_group)    \
        ((((_subport_group >> _BCM_KT2_SUBPORT_GROUP_SUBTYPE_SHIFT) & \
            _BCM_KT2_SUBPORT_GROUP_SUBTYPE_MASK) ==    \
            _BCM_KT2_SUBPORT_TYPE_SUBTAG) && \
         (((_subport_group >> _BCM_KT2_SUBPORT_GROUP_TYPE_SHIFT) & \
            _BCM_KT2_SUBPORT_GROUP_TYPE_MASK) ==    \
            _BCM_KT2_SUBPORT_TYPE_CASCADED))

/* Set the subport port gport  pp_port id*/
#define _BCM_KT2_SUBPORT_PORT_ID_SET(_subport_port, _pp_port)  \
    (BCM_GPORT_SUBPORT_PORT_SET((_subport_port), \
        ((_pp_port & _BCM_KT2_SUBPORT_PORT_PP_PORT_MASK) \
             << _BCM_KT2_SUBPORT_PORT_PP_PORT_SHIFT)))

/* Set the subport port gport  type*/
#define _BCM_KT2_SUBPORT_PORT_TYPE_SET(_subport_port, _type)  \
    (_subport_port |= ((_type & _BCM_KT2_SUBPORT_PORT_TYPE_MASK) \
          << _BCM_KT2_SUBPORT_PORT_TYPE_SHIFT))

/* Get the subport port gport pp_port id */
#define _BCM_KT2_SUBPORT_PORT_ID_GET(_subport_port)   \
    (((_subport_port) >> _BCM_KT2_SUBPORT_PORT_PP_PORT_SHIFT) & \
                         _BCM_KT2_SUBPORT_PORT_PP_PORT_MASK)


/* Check if gport is LinkPHY subport_port */
#define _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(_unit, _subport_port)    \
        (((((_subport_port) >> _BCM_KT2_SUBPORT_PORT_TYPE_SHIFT) & \
                        _BCM_KT2_SUBPORT_PORT_TYPE_MASK) == \
                        _BCM_KT2_SUBPORT_TYPE_LINKPHY) && \
         ((((_subport_port) >> _BCM_KT2_SUBPORT_PORT_ZERO_BITS_SHIFT) & \
                        _BCM_KT2_SUBPORT_PORT_ZERO_BITS_MASK) == 0) && \
        (BCM_PBMP_MEMBER(SOC_INFO(_unit).linkphy_pp_port_pbm, \
            (((_subport_port) >> _BCM_KT2_SUBPORT_PORT_PP_PORT_SHIFT) & \
                        _BCM_KT2_SUBPORT_PORT_PP_PORT_MASK))))

/* Check if gport is SubTag subport_port */
#define _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(_unit, _subport_port)    \
        (((((_subport_port) >> _BCM_KT2_SUBPORT_PORT_TYPE_SHIFT) & \
                        _BCM_KT2_SUBPORT_PORT_TYPE_MASK) == \
                        _BCM_KT2_SUBPORT_TYPE_SUBTAG) && \
         ((((_subport_port) >> _BCM_KT2_SUBPORT_PORT_ZERO_BITS_SHIFT) & \
                        _BCM_KT2_SUBPORT_PORT_ZERO_BITS_MASK) == 0) && \
        (BCM_PBMP_MEMBER(SOC_INFO(_unit).subtag_pp_port_pbm, \
            (((_subport_port) >> _BCM_KT2_SUBPORT_PORT_PP_PORT_SHIFT) & \
                        _BCM_KT2_SUBPORT_PORT_PP_PORT_MASK))))

typedef struct {
    bcm_port_t port;
    int        port_type;
    bcm_vlan_t subtag;
    int        num_streams;
    uint16     dev_int_stream_id[BCM_SUBPORT_CONFIG_MAX_STREAMS];
} _bcm_kt2_subport_info_t;

#endif /* BCM_KATANA2_SUPPORT */


#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_tr_subport_sw_dump(int unit);
extern void _bcm_tr2_subport_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* !_BCM_INT_ESW_SUBPORT_H_ */
