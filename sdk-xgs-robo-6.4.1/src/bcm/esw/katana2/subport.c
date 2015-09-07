/*
 * $Id: subport.c,v 1.53 Broadcom SDK $
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
 * File:    subport.c
 * Purpose: Katana2 SUBPORT functions
 */

#include <shared/bsl.h>

#include <soc/defs.h>
#include <sal/core/libc.h>
#if defined(BCM_KATANA2_SUPPORT)

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/katana2.h>
#include <bcm/error.h>
#include <bcm/subport.h>
#include <bcm_int/esw/katana2.h>
#include <bcm_int/esw/subport.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>

/*****************************************
* Subport common section - start
******************************************/

/*
* Bitmap of subport group id
* Each Subport group created has unique group id
* across all ports in the device irrespective of
* group type LinkPHY/SubTag
*/
STATIC SHR_BITDCL *_bcm_subport_group_bitmap[BCM_MAX_NUM_UNITS] = { 0 };

/* Count of all subport groups created */
STATIC int _bcm_subport_group_count[BCM_MAX_NUM_UNITS] = { 0 };

/* Count subport port per subport group*/
STATIC int *(_bcm_subport_group_subport_port_count[BCM_MAX_NUM_UNITS]) = { 0 };

/* Bitmap to manage PP_PORT indices for subport port addition*/
STATIC SHR_BITDCL *_bcm_subport_pp_port_bitmap[BCM_MAX_NUM_UNITS] = { 0 };
STATIC int _bcm_subport_pp_port_count[BCM_MAX_NUM_UNITS] = { 0 };

/* Get port number/trunk id associated with subport_group gport*/
#define _BCM_KT2_SUBPORT_GROUP_PORT_GET(_subport_group) \
        ((_subport_group >> _BCM_KT2_SUBPORT_GROUP_PORT_SHIFT) & \
            _BCM_KT2_SUBPORT_GROUP_PORT_MASK)

/* Get group id associated with subport_group gport*/
#define _BCM_KT2_SUBPORT_GROUP_ID_GET(_subport_group) \
        (((_subport_group) >> _BCM_KT2_SUBPORT_GROUP_SPGID_SHIFT) & \
            _BCM_KT2_SUBPORT_GROUP_SPGID_MASK)

#define _BCM_KT2_SUBPORT_GROUP_TYPE_CASCADED_SET(_subport_group) \
    ((_subport_group) |= ((_BCM_KT2_SUBPORT_TYPE_CASCADED & \
                          _BCM_KT2_SUBPORT_GROUP_TYPE_MASK) \
                              << _BCM_KT2_SUBPORT_GROUP_TYPE_SHIFT))
                         
#define _BCM_KT2_SUBPORT_LOCK(unit) \
    sal_mutex_take(_bcm_subport_mutex[unit], sal_mutex_FOREVER);

#define _BCM_KT2_SUBPORT_UNLOCK(unit) \
    sal_mutex_give(_bcm_subport_mutex[unit]);

STATIC sal_mutex_t _bcm_subport_mutex[BCM_MAX_NUM_UNITS] = {NULL};

#define _BCM_KT2_SUBPORT_CHECK_INIT(_unit_) \
        if (!_bcm_subport_group_bitmap[_unit_]) \
            return BCM_E_INIT

/*
 * Subport group usage bitmap operations
 */
#define _BCM_KT2_SUBPORT_GROUP_USED_GET(_u_, _group_) \
                SHR_BITGET(_bcm_subport_group_bitmap[_u_], (_group_))

#define _BCM_KT2_SUBPORT_GROUP_USED_SET(_u_, _group_) \
                SHR_BITSET(_bcm_subport_group_bitmap[_u_], (_group_))

#define _BCM_KT2_SUBPORT_GROUP_USED_CLR(_u_, _group_) \
                SHR_BITCLR(_bcm_subport_group_bitmap[_u_], (_group_))

#define _BCM_KT2_SUBPORT_PP_PORT_USED_GET(_u_, _pp_port_) \
                SHR_BITGET(_bcm_subport_pp_port_bitmap[_u_], (_pp_port_))

#define _BCM_KT2_SUBPORT_PP_PORT_USED_SET(_u_, _pp_port_) \
                SHR_BITSET(_bcm_subport_pp_port_bitmap[_u_], (_pp_port_))

#define _BCM_KT2_SUBPORT_PP_PORT_USED_CLR(_u_, _pp_port_) \
                SHR_BITCLR(_bcm_subport_pp_port_bitmap[_u_], (_pp_port_))


/*********************************
* Subport common  section - end
**********************************/

/*********************************
* LinkPHY subport section -start
**********************************/

/* Bitmap of LinkPHY subport group id */
STATIC SHR_BITDCL *_bcm_linkphy_group_bitmap[BCM_MAX_NUM_UNITS] = { 0 };

/* Count of all LinkPHY subport groups created */
STATIC int _bcm_linkphy_subport_group_count[BCM_MAX_NUM_UNITS] = { 0 };

/*
* Bitmap of device internal stream id usage.usage for Katana2
* Katana has 2 blocks and each block has 128 streams.
* External stream id is mapped to 256 device internal stream ids.
* Block 0 internal stream id base = 0
* Block 1 internal stream id base = 128
* device internal stream id = internal stream id + internal stream id base
*/
STATIC SHR_BITDCL *_bcm_dev_int_stream_id_bitmap[BCM_MAX_NUM_UNITS] = { 0 };

/* Set the LinkPHY subport group gport */
#define _BCM_KT2_LINKPHY_SUBPORT_GROUP_SET(_subport_group, \
                                           _port, _gid) \
    (BCM_GPORT_SUBPORT_GROUP_SET((_subport_group), \
                        (((_BCM_KT2_SUBPORT_TYPE_LINKPHY & \
                          _BCM_KT2_SUBPORT_GROUP_SUBTYPE_MASK) \
                              << _BCM_KT2_SUBPORT_GROUP_SUBTYPE_SHIFT) | \
                         ((_port & _BCM_KT2_SUBPORT_GROUP_PORT_MASK) \
                              << _BCM_KT2_SUBPORT_GROUP_PORT_SHIFT) | \
                         ((_gid & _BCM_KT2_SUBPORT_GROUP_SPGID_MASK) \
                              << _BCM_KT2_SUBPORT_GROUP_SPGID_SHIFT))))

#define _BCM_KT2_PORT_SUPPORTS_LINKPHY(_unit_, _port_) \
    (SOC_REG_PORT_VALID(_unit_, RXLP_PORT_ENABLEr, _port_))

/*
 * LinkPHY subport group usage bitmap operations
 */
#define _BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_GET(_u_, _group_) \
        SHR_BITGET(_bcm_linkphy_group_bitmap[_u_], (_group_))

#define _BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_SET(_u_, _group_) \
        SHR_BITSET(_bcm_linkphy_group_bitmap[_u_], (_group_))

#define _BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_CLR(_u_, _group_) \
        SHR_BITCLR(_bcm_linkphy_group_bitmap[_u_], (_group_))


/* Katana2 LinkPHY device internal stream id usage bitmap */
#define _BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_GET(_u_, _stream_id_) \
        SHR_BITGET(_bcm_dev_int_stream_id_bitmap[_u_], (_stream_id_))

#define _BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_SET(_u_, _stream_id_) \
        SHR_BITSET(_bcm_dev_int_stream_id_bitmap[_u_], (_stream_id_))

#define _BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_CLR(_u_, _stream_id_) \
        SHR_BITCLR(_bcm_dev_int_stream_id_bitmap[_u_], (_stream_id_))

/* LinkPHY subport port info structure */
typedef struct {
    bcm_gport_t group;       /* Subport group gort*/
    int         valid;
    int         num_streams; /* Number of sreams */
    uint16      ext_stream_idx[BCM_SUBPORT_CONFIG_MAX_STREAMS];
} _bcm_linkphy_subport_port_info_t;

/* LinkPHY subport port array */
_bcm_linkphy_subport_port_info_t
           *(_bcm_linkphy_subport_port_info[BCM_MAX_NUM_UNITS]) = { 0 };

#define _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port) \
    (_bcm_linkphy_subport_port_info[unit][pp_port].valid)

#define _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port) \
    (_bcm_linkphy_subport_port_info[unit][pp_port].group)

#define _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_NUM_STREAMS_GET(unit, pp_port) \
    (_bcm_linkphy_subport_port_info[unit][pp_port].num_streams)

#define _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_EXT_STREAM_ID_GET(unit, pp_port, \
    sp_stream_array_id) \
    (_bcm_linkphy_subport_port_info[unit] \
    [pp_port].ext_stream_idx[sp_stream_array_id])

#define _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_SET(unit, \
    pp_port, sp_group, sp_num_streams, is_valid) \
{ \
    _bcm_linkphy_subport_port_info[unit][pp_port].group = sp_group; \
     _bcm_linkphy_subport_port_info[unit][pp_port].num_streams \
                                   = sp_num_streams; \
    _bcm_linkphy_subport_port_info[unit][pp_port].valid = is_valid; \
}

/* LinkPHY counter related defines */

#define _BCM_KT2_LINKPHY_DIRECTION_MAX 2
#define _BCM_KT2_LINKPHY_DIRECTION_RX  0
#define _BCM_KT2_LINKPHY_DIRECTION_TX  1
#define _BCM_KT2_LINKPHY_BLOCK_MAX     2
#define _BCM_KT2_LINKPHY_POOL_MAX      4
#define _BCM_KT2_LINKPHY_POOL_FRAG_BYTE  0
#define _BCM_KT2_LINKPHY_POOL_FRAG       1
#define _BCM_KT2_LINKPHY_POOL_FRAME_BYTE 2
#define _BCM_KT2_LINKPHY_POOL_FRAME      3
#define _BCM_KT2_LINKPHY_COUNTER_MIN   0
#define _BCM_KT2_LINKPHY_COUNTER_MAX   128


static sal_mutex_t linkphy_ctr_mutex[BCM_UNITS_MAX] = {NULL};


static rxlp_debug_counter7_entry_t *linkphy_temp_ctr
               [BCM_UNITS_MAX] = {NULL};

static uint64 *linkphy_frag_byte_ctr
               [BCM_UNITS_MAX][_BCM_KT2_LINKPHY_DIRECTION_MAX]
               [_BCM_KT2_LINKPHY_BLOCK_MAX] = {{{NULL}}};
static uint64 *linkphy_frag_ctr
               [BCM_UNITS_MAX][_BCM_KT2_LINKPHY_DIRECTION_MAX]
               [_BCM_KT2_LINKPHY_BLOCK_MAX] = {{{NULL}}};
static uint64 *linkphy_frame_byte_ctr
               [BCM_UNITS_MAX][_BCM_KT2_LINKPHY_DIRECTION_MAX]
               [_BCM_KT2_LINKPHY_BLOCK_MAX] = {{{NULL}}};
static uint64 *linkphy_frame_ctr
               [BCM_UNITS_MAX][_BCM_KT2_LINKPHY_DIRECTION_MAX]
               [_BCM_KT2_LINKPHY_BLOCK_MAX] = {{{NULL}}};


#define BCM_LINKPHY_COUNTER_LOCK(unit) \
        sal_mutex_take(linkphy_ctr_mutex[unit], sal_mutex_FOREVER);
#define BCM_LINKPHY_COUNTER_UNLOCK(unit) \
        sal_mutex_give(linkphy_ctr_mutex[unit]);

static soc_mem_t linkphy_counter_table
         [_BCM_KT2_LINKPHY_DIRECTION_MAX][_BCM_KT2_LINKPHY_POOL_MAX] = {
             {RXLP_DEBUG_COUNTER8m,
              RXLP_DEBUG_COUNTER9m,
              RXLP_DEBUG_COUNTER10m,
              RXLP_DEBUG_COUNTER11m},
             {TXLP_DEBUG_COUNTER8m,
              TXLP_DEBUG_COUNTER9m,
              TXLP_DEBUG_COUNTER10m,
              TXLP_DEBUG_COUNTER11m}
             };

/*********************************
* LinkPHY subport section -end
**********************************/


/*********************************
* SubTag subport section - start
**********************************/

/* Bitmap of SubTag subport group id */
STATIC SHR_BITDCL *_bcm_subtag_group_bitmap[BCM_MAX_NUM_UNITS] = { 0 };

/* Count of all SubTag subport groups created */
STATIC int _bcm_subtag_subport_group_count[BCM_MAX_NUM_UNITS] = { 0 };

STATIC SHR_BITDCL *_bcm_subtag_vlan_id_bitmap
                   [BCM_MAX_NUM_UNITS][SOC_MAX_NUM_PORTS] = {{ 0 }};

/* Set the SubTag subport group gport */
#define _BCM_KT2_SUBTAG_SUBPORT_GROUP_SET(_subport_group, \
                                          _port, _gid) \
    (BCM_GPORT_SUBPORT_GROUP_SET((_subport_group), \
                        (((_BCM_KT2_SUBPORT_TYPE_SUBTAG & \
                          _BCM_KT2_SUBPORT_GROUP_SUBTYPE_MASK) \
                              << _BCM_KT2_SUBPORT_GROUP_SUBTYPE_SHIFT) | \
                         ((_port & _BCM_KT2_SUBPORT_GROUP_PORT_MASK) \
                              << _BCM_KT2_SUBPORT_GROUP_PORT_SHIFT) | \
                         ((_gid & _BCM_KT2_SUBPORT_GROUP_SPGID_MASK) \
                              << _BCM_KT2_SUBPORT_GROUP_SPGID_SHIFT))))

/*
 * SubTag subport group usage bitmap operations
 */
#define _BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_GET(_u_, _group_) \
        SHR_BITGET(_bcm_subtag_group_bitmap[_u_], (_group_))

#define _BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_SET(_u_, _group_) \
        SHR_BITSET(_bcm_subtag_group_bitmap[_u_], (_group_))

#define _BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_CLR(_u_, _group_) \
        SHR_BITCLR(_bcm_subtag_group_bitmap[_u_], (_group_))

/* Katana2 SubTag VLAN id usage bitmap for given port*/
#define _BCM_KT2_SUBTAG_VLAN_ID_USED_GET(_u_, _p, _vlan_id_) \
        SHR_BITGET(_bcm_subtag_vlan_id_bitmap[_u_][_p], (_vlan_id_))

#define _BCM_KT2_SUBTAG_VLAN_ID_USED_SET(_u_, _p, _vlan_id_) \
        SHR_BITSET(_bcm_subtag_vlan_id_bitmap[_u_][_p], (_vlan_id_))

#define _BCM_KT2_SUBTAG_VLAN_ID_USED_CLR(_u_, _p, _vlan_id_) \
        SHR_BITCLR(_bcm_subtag_vlan_id_bitmap[_u_][_p], (_vlan_id_))

/* SubTag subport port info structure */
typedef struct {
    bcm_gport_t group;  /* Subport group gort attached to */
    bcm_vlan_t  vlan;   /* VLAN ID */
    int         valid;
    int         subtag_tcam_hw_idx;
} _bcm_subtag_subport_port_info_t;

/* SubTag subport port array */
_bcm_subtag_subport_port_info_t
           *(_bcm_subtag_subport_port_info[BCM_MAX_NUM_UNITS]) = { 0 };


#define _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port) \
    (_bcm_subtag_subport_port_info[unit][pp_port].valid)

#define _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port) \
    (_bcm_subtag_subport_port_info[unit][pp_port].group)

#define _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VLAN_GET(unit, pp_port) \
    (_bcm_subtag_subport_port_info[unit][pp_port].vlan)

#define _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_TCAM_HW_ID_GET(unit, pp_port) \
    (_bcm_subtag_subport_port_info[unit][pp_port].subtag_tcam_hw_idx)

#define _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_SET(unit, pp_port,\
        sp_group, vlan_id, tcam_hw_id, is_valid) \
{ \
    _bcm_subtag_subport_port_info[unit][pp_port].group = sp_group; \
    _bcm_subtag_subport_port_info[unit][pp_port].vlan = vlan_id; \
    _bcm_subtag_subport_port_info[unit] \
                                 [pp_port].subtag_tcam_hw_idx = tcam_hw_id; \
    _bcm_subtag_subport_port_info[unit][pp_port].valid = is_valid; \
}


/*********************************
* SubTag subport section - end
**********************************/

/* prototypes - start */
STATIC int
_bcm_linkphy_subport_port_get(int unit,
                             bcm_gport_t subport_port_gport,
                             bcm_subport_config_t *config);

STATIC int
_bcm_linkphy_subport_port_delete(int unit,
                                bcm_gport_t subport_port_gport);

STATIC int
_bcm_subtag_subport_port_get(int unit,
                             bcm_gport_t subport_port_gport,
                             bcm_subport_config_t *config);

STATIC int
_bcm_subtag_subport_port_delete(int unit,
                                bcm_gport_t subport_port_gport);

#ifdef BCM_WARM_BOOT_SUPPORT
STATIC int
_bcm_kt2_subport_reinit(int unit);
#endif

/* prototypes - end */


/***********************************
* LinkPHY helper functions - start *
***********************************/

STATIC int
_bcm_linkphy_reg_mac_addr_set(int unit, bcm_port_t port,
                        soc_reg_t reg, soc_field_t field,
                        bcm_mac_t mac) 
{
    uint64 val, tmp, regval;
    int i;

    COMPILER_64_ZERO(val);
    COMPILER_64_ZERO(tmp);
    COMPILER_64_ZERO(regval);

    for (i = 0; i < 6; i++) {
        COMPILER_64_SET(tmp, 0, mac[i]);
        COMPILER_64_SHL(val, 8);
        COMPILER_64_OR(val, tmp);
    }
    BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &regval));
    soc_reg64_field_set(unit, reg, &regval, field, val);
    return soc_reg_set(unit, reg, port, 0, regval);
}

STATIC int
_bcm_linkphy_reg_mac_addr_get(int unit, bcm_port_t port,
                        soc_reg_t reg, soc_field_t field,
                        bcm_mac_t mac)
{
    uint64 regval, data;
    uint32 msw;
    uint32 lsw;

    COMPILER_64_ZERO(data);
    COMPILER_64_ZERO(regval);

    BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &regval));
    data = soc_reg64_field_get(unit, reg, regval, field);
    COMPILER_64_TO_32_HI(msw, data);
    COMPILER_64_TO_32_LO(lsw, data);

    mac[0] = (uint8) ( ( msw & 0x0000ff00 ) >> 8 );
    mac[1] = (uint8) ( msw & 0x000000ff );

    mac[2] = (uint8)  ( ( lsw & 0xff000000) >> 24 );
    mac[3] = (uint8)  ( ( lsw & 0x00ff0000) >> 16 );
    mac[4] = (uint8)  ( ( lsw & 0x0000ff00) >> 8 );
    mac[5] = (uint8)  ( lsw & 0x000000ff );

    return BCM_E_NONE;
}

int
_bcm_linkphy_port_ext_stream_id_base_get(int unit,
                                         bcm_port_t port,
                                         uint16 *ext_stream_id_base)
{
    uint32 regval = 0;
    soc_reg_t reg = RXLP_PORT_STREAM_ID_BASEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        SOC_IF_ERROR_RETURN(
            READ_RXLP_PORT_STREAM_ID_BASEr(unit, port, &regval));
        *ext_stream_id_base = soc_reg_field_get(unit, reg, regval, BASEf);
    }
    return BCM_E_NONE;
}

int
_bcm_linkphy_port_int_stream_id_base_get(int unit,
                                         bcm_port_t port,
                                         uint16 *int_stream_id_base)
{
    uint32 regval = 0;
    soc_reg_t reg = RXLP_INT_STREAM_ID_BASEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        SOC_IF_ERROR_RETURN(READ_RXLP_INT_STREAM_ID_BASEr(unit, port, &regval));
        *int_stream_id_base = soc_reg_field_get(unit, reg, regval, BASEf);
    }
    return BCM_E_NONE;
}

/*
* Set or clear the valid streams on a sub-port of a port-interface
* in a bitmap format.
* Each bit set indicates that SID being part of that port.
* Note that same stream cannot be enabled on more than one sub-port.
* When the port is in LP-mode, then Stream-ID for that sub-port is 
* in Range [0-127] only.
* Sub-ports cannot be dyncamically changed to LP->NonLP and vice-versa
* without resetting the entire block.
* However in LP-mode, streams could get added dynamically to a port.
*/

STATIC int
_bcm_linkphy_port_tx_active_stream_set(int unit,
                                       bcm_port_t port,
                                       uint16 int_stream_id,
                                       int enable)
{
    soc_mem_t mem;
    int rv = BCM_E_NONE, lp_index = 0, lp_block = 0, copyno = 0;
    int field_word = 0, bit_pos = 0;
    uint32 fldbuf[4] = { 0 };
    txlp_port_stream_bitmap_table_entry_t txlp_port_stream_bitmap;

    /* Set Active Stream ID
    *  Use TXLP_PORT_STREAM_BITMAP_TABLE
    *  This is a per-LP block, per-LP port memory.
    *  KT2 has 2 LP block and 4 LP ports per LP block
    */

    mem = TXLP_PORT_STREAM_BITMAP_TABLEm;
    if (SOC_MEM_FIELD_VALID(unit, mem, STREAM_BITMAPf)) {
        /* get the block and index for the LinkPHY memory */
        rv = soc_kt2_linkphy_port_blk_idx_get(unit, port, &lp_block, &lp_index);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  
                                   "ERROR: LinkPHY block, index get failed for port %d\n"), port));
            return rv;
        }

        copyno = SOC_MEM_BLOCK_ANY(unit, mem) + lp_block;
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, copyno, lp_index,
            &txlp_port_stream_bitmap));
        soc_mem_field_get(unit, mem, (uint32 *)&txlp_port_stream_bitmap,
                 STREAM_BITMAPf, fldbuf);

        field_word = int_stream_id / 32;
        bit_pos  = int_stream_id % 32;
        if (enable) {
             fldbuf[field_word] |= (1 << bit_pos);
        } else {
             fldbuf[field_word] &= ~(1 << bit_pos);
        }
        soc_mem_field_set(unit, mem, (uint32 *)&txlp_port_stream_bitmap,
                        STREAM_BITMAPf, fldbuf);
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, mem, copyno,
                    lp_index, &txlp_port_stream_bitmap));
    }

    return rv;
}

/* Map the Internal-SID(7b) to External-SID(10b) */
STATIC int
_bcm_linkphy_port_tx_int_to_ext_stream_map_set(int unit,
                                               int port,
                                               uint16 ext_stream_id,
                                               uint16 int_stream_id,
                                               int enable)
{
    int rv = BCM_E_NONE;
    soc_mem_t mem;
    int copyno = 0, lp_block = 0, lp_index = 0;
    txlp_int2ext_stream_map_table_entry_t txlp_int2ext_stream_map;

    /* Map Internal Stream ID to External Stream ID 
    *  Use TXLP_INT2EXT_STREAM_MAP_TABLE  table
    *  This is a per-LP block memory
    */
    mem = TXLP_INT2EXT_STREAM_MAP_TABLEm;
    if (SOC_MEM_FIELD_VALID(unit, mem, EXTERNAL_SIDf)) {
        /* get the block and index for the LinkPHY memory */
        rv = soc_kt2_linkphy_port_blk_idx_get(unit,port,
                                         &lp_block, &lp_index);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  
                                   "ERROR: LinkPHY block, index get failed for port %d\n"), port));
            return rv;
        }
        sal_memset(&txlp_int2ext_stream_map, 0, 
            sizeof(txlp_int2ext_stream_map_table_entry_t));

        copyno = SOC_MEM_BLOCK_ANY(unit, mem) + lp_block;
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, copyno, int_stream_id,
            &txlp_int2ext_stream_map));
        soc_mem_field32_set(unit, mem, (uint32 *)&txlp_int2ext_stream_map,
                 VALIDf, (enable) ? 1 : 0);
        soc_mem_field32_set(unit, mem, (uint32 *)&txlp_int2ext_stream_map,
                    EXTERNAL_SIDf, (enable) ? (uint32)ext_stream_id : 0);
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, mem, copyno, int_stream_id,
            &txlp_int2ext_stream_map));
    }

    return rv;
}

/* 
* Function for buffer allocation for the streams of LP-ENABLED ports.
*
* End address Range = [11-3423].
* Starting address Range = [0-3420].
* Note that [END-START+1] must be an integral multiple of "4".
*
* 856-cell worth of storage is available in the TXL-DATA-BUFFER. 
* 856 cells have to be distributed across:
* 1. All the streams of LP-ENABLED ports,
* 2. Non-LP ports.
*
* A cell=384B(4-lines of 96B). Hence Depth = 856 * 4 = 3424 lines.
* Following are the rules of how to configure space for each STREAM::
* 1.  Each active STREAM on an LP-PORT can be allocated utmost "N" cells, 
* 2.  "N" is an integer and [2* <= N <= 64] 
* 3.  Each STREAM is provided with a [start,end] pointer pairs. 
* 4.  Space for each STREAM = [end-start+1] lines of buffer.
* 5.  As each cell-occupies "4" lines in buffer, 
*     [end-start+1] must be a integral multiple of "4".
* 6.  The addresses must be configured linearly from "0" till "3423". 
* 7.  Wrap-around not supported.
* 8.  END must be always greater than START.
* 9.  If a stream is INACTIVE start=end=0.
* 10. This configuration has to be done before "enabling" the STREAM.
* NOTE: There has to be a minimum of "2" cell allocation 
* for a stream to function. If we allocate only "1" cell,
* there is potential that stream will be stuck.
*
* Following are the guidelines to configure number of cells/STREAM::
* The Line-rate is gaurenteed when below guidelines are adhered to.
*
* In LP-Mode:
* ===========
* Stream-Speed(Gbps) |  Min Num.Cells
*    < 1G            |     3
*      1G            |     6
*      10G           |     50 
*  
* In Non-LP-Mode:
* ===============
* DONOT PROGRAM.
*/

STATIC int
_bcm_linkphy_port_tx_stream_addr_map_set(int unit,
                                         int port,
                                         uint16 int_stream_id,
                                         uint32 start_addr,
                                         uint32 end_addr,
                                         int enable)
{
    int rv = BCM_E_NONE;
    int copyno = 0, lp_block = 0, lp_index = 0;
    soc_mem_t mem;
    txlp_stream_addr_map_table_entry_t txlp_stream_addr_map;

    mem = TXLP_STREAM_ADDR_MAP_TABLEm;
    if (SOC_MEM_FIELD_VALID(unit, mem, START_ADDRf)) {
        if (enable) {
            if ((start_addr > _BCM_KT2_LINKPHY_TX_DATA_BUF_START_ADDR_MAX) ||
                (end_addr < _BCM_KT2_LINKPHY_TX_DATA_BUF_END_ADDR_MIN) ||
                (end_addr > _BCM_KT2_LINKPHY_TX_DATA_BUF_END_ADDR_MAX)) {
                return BCM_E_PARAM;
            }

            if ((end_addr - start_addr + 1) % 4) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR:for TX data buffer allocation [END-START+1] "
                                      "should be multiple of 4. \n(start =%d, end = %d)"
                                      "for port = %d, int_stream_id = %d\n"),
                           start_addr, end_addr,
                           port, int_stream_id));
                return BCM_E_PARAM;
            }
        } else {
            start_addr = 0;
            end_addr = 0;
        }

        /* get the block and index for the LinkPHY memory */
        rv = soc_kt2_linkphy_port_blk_idx_get(unit,port,
                                         &lp_block, &lp_index);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  
                                   "ERROR: LinkPHY block, index get failed for port %d\n"), port));
            return rv;
        }
        sal_memset(&txlp_stream_addr_map, 0, 
            sizeof(txlp_stream_addr_map_table_entry_t));

        copyno = SOC_MEM_BLOCK_ANY(unit, mem) + lp_block;
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, copyno, int_stream_id,
            &txlp_stream_addr_map));

        soc_mem_field32_set(unit, mem, (uint32 *)&txlp_stream_addr_map,
                 START_ADDRf, enable ? start_addr : 0);

        soc_mem_field32_set(unit, mem, (uint32 *)&txlp_stream_addr_map,
                 END_ADDRf, enable ? end_addr : 0);

        SOC_IF_ERROR_RETURN(soc_mem_write(unit, mem, copyno, int_stream_id,
            &txlp_stream_addr_map));
    }

    return rv;
}

STATIC int
_bcm_linkphy_port_rx_flow_control_set(int unit,
                                      int port,
                                      uint16 int_stream_id,
                                      int enable)
{
    soc_reg_t reg;
    int rv = BCM_E_NONE, reg_hi = 0, stream_bit = 0;
    uint64 rval64, data, bit_pos;

    COMPILER_64_ZERO(rval64);
    COMPILER_64_ZERO(data);
    COMPILER_64_ZERO(bit_pos);

    reg_hi = int_stream_id / 64;
    stream_bit = int_stream_id % 64;

    reg = (reg_hi) ? RXLP_DFC_STATUS_CPU_UPDATE_ENABLE_HIr :
                     RXLP_DFC_STATUS_CPU_UPDATE_ENABLE_LOr;

    if (SOC_REG_IS_VALID(unit, reg)) {
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));

        data = soc_reg64_field_get(unit, reg, rval64, DFC_STATUS_CPU_WRITE_ENf);

        COMPILER_64_SET(bit_pos, 0, 1);
        COMPILER_64_SHL(bit_pos, stream_bit);
        COMPILER_64_OR(data, bit_pos);

        soc_reg64_field_set(unit, reg, &rval64, DFC_STATUS_CPU_WRITE_ENf, data);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    }

    reg = (reg_hi) ? RXLP_DFC_STATUS_HIr : RXLP_DFC_STATUS_LOr;

    if (SOC_REG_IS_VALID(unit, reg)) {
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));

        data = soc_reg64_field_get(unit, reg, rval64, DFC_STATUSf);

        if (enable) {
            if (!COMPILER_64_BITTEST(data, stream_bit)) {
                COMPILER_64_SET(bit_pos, 0, 1);
                COMPILER_64_SHL(bit_pos, stream_bit);
                COMPILER_64_OR(data, bit_pos);

                soc_reg64_field_set(unit, reg, &rval64, DFC_STATUSf, data);
                SOC_IF_ERROR_RETURN(
                    soc_reg_set(unit, reg, port, 0, rval64));
            }
        } else {
            if (COMPILER_64_BITTEST(data, stream_bit)) {
                COMPILER_64_SET(bit_pos, 0, 1);
                COMPILER_64_SHL(bit_pos, stream_bit);
                COMPILER_64_NOT(bit_pos);
                COMPILER_64_AND(data,bit_pos);

                soc_reg64_field_set(unit, reg, &rval64, DFC_STATUSf, data);
                SOC_IF_ERROR_RETURN(
                    soc_reg_set(unit, reg, port, 0, rval64));
            }
        }
    }

    return rv;
}

/* Set and clear Bitmap(LSBs/MSBs) of active internal stream IDs 
* associated with a port.
* The same internal stream ID should not be active on more than one ports
*/
STATIC int
_bcm_linkphy_port_rx_active_stream_bitmap_set(int unit,
                                              int port,
                                              uint16 int_stream_id,
                                              int enable)
{
    soc_reg_t reg;
    int rv = BCM_E_NONE, reg_hi = 0, stream_bit = 0;
    uint64 rval64, data, bit_pos;

    COMPILER_64_ZERO(rval64);
    COMPILER_64_ZERO(data);
    COMPILER_64_ZERO(bit_pos);

    reg_hi = int_stream_id / 64;
    stream_bit = int_stream_id % 64;

    reg = (reg_hi) ? RXLP_PORT_ACTIVE_STREAM_BITMAP_HIr :
                     RXLP_PORT_ACTIVE_STREAM_BITMAP_LOr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {

        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));

        data = soc_reg64_field_get(unit, reg, rval64,ACTIVE_STREAM_BITMAPf);

        if (enable) {
            if (!COMPILER_64_BITTEST(data, stream_bit)) {
                COMPILER_64_SET(bit_pos, 0, 1);
                COMPILER_64_SHL(bit_pos, stream_bit);
                COMPILER_64_OR(data, bit_pos);

                soc_reg64_field_set(unit, reg, &rval64,
                    ACTIVE_STREAM_BITMAPf, data);
                SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
            }
        } else {
            if (COMPILER_64_BITTEST(data, stream_bit)) {
                COMPILER_64_SET(bit_pos, 0, 1);
                COMPILER_64_SHL(bit_pos, stream_bit);
                COMPILER_64_NOT(bit_pos);
                COMPILER_64_AND(data, bit_pos);

                soc_reg64_field_set(unit, reg, &rval64,
                    ACTIVE_STREAM_BITMAPf, data);
                SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
            }
        }
    }

    return rv;
}

/* Map (External Stream ID - RXLP_PORT_STREAM_ID_BASE) 
* to internal stream ID in the range 0-127.
* There is one copy of this table for each port.
* The internal stream IDs configured in this table 
* should be unique across the four ports associated with the RXLP slice.
*/
STATIC int
_bcm_linkphy_port_rx_int_stream_map_port_set(int unit,
                                             int port,
                                             uint16 int_stream_id,
                                             int enable)
{
    soc_mem_t mem;
    int rv = BCM_E_NONE, copyno = 0, lp_index = 0, lp_block = 0;
    rxlp_internal_stream_map_port_0_entry_t rxlp_int_stream_map_port;

    mem = RXLP_INTERNAL_STREAM_MAP_PORT_0m;
    if (SOC_MEM_FIELD_VALID(unit, mem, INTERNAL_STREAM_IDf)) {
        /* get the block and index for the LinkPHY memory */
        rv = soc_kt2_linkphy_port_blk_idx_get(unit,port,
                                         &lp_block, &lp_index);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  
                                   "ERROR: LinkPHY block, index get failed for port %d\n"), port));
            return rv;
        }
        sal_memset(&rxlp_int_stream_map_port, 0, 
            sizeof(rxlp_internal_stream_map_port_0_entry_t));

        /*
        * Internal stream id = 
        * RXLP_INTERNAL_STREAM_MAP_PORTn.rxlp0/1 [Internal Stream ID];
        */
        switch (lp_index) {
            case 0:
                mem = RXLP_INTERNAL_STREAM_MAP_PORT_0m;
                break;
            case 1:
                mem = RXLP_INTERNAL_STREAM_MAP_PORT_1m;
                break;
            case 2:
                mem = RXLP_INTERNAL_STREAM_MAP_PORT_2m;
                break;
            case 3:
                mem = RXLP_INTERNAL_STREAM_MAP_PORT_3m;
                break;
            default:
                return SOC_E_PORT;
        };
        copyno = SOC_MEM_BLOCK_ANY(unit, mem) + lp_block;
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, copyno, int_stream_id,
                                          &rxlp_int_stream_map_port));
        soc_mem_field32_set(unit, mem,
                    (uint32 *)&rxlp_int_stream_map_port, VALIDf,
                    (enable) ? 1 : 0);
        soc_mem_field32_set(unit, mem,
                    (uint32 *)&rxlp_int_stream_map_port, INTERNAL_STREAM_IDf,
                    (enable) ? int_stream_id : 0);
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, mem, copyno, int_stream_id,
                             &rxlp_int_stream_map_port));
    }

    return rv;
}

STATIC void
_bcm_linkphy_subport_stat_callback(int unit)
{
    int pool_id = 0, direction = 0, index = 0, block_id = 0, copyno = 0;
    int width = 0;
    soc_mem_t mem;
    uint32 ctr_new = 0, ctr_prev = 0, ctr_diff = 0;
    volatile uint64 *vptr;

    BCM_LINKPHY_COUNTER_LOCK(unit);
    if (linkphy_temp_ctr[unit] == NULL){
        LOG_WARN(BSL_LS_BCM_SUBPORT,
                 (BSL_META_U(unit,
                             "Unit %d Not Initilized or attached\n"),
                  unit));
    }

    for (direction = 0;
        direction < _BCM_KT2_LINKPHY_DIRECTION_MAX; direction++) {
        for (block_id = 0; block_id < _BCM_KT2_LINKPHY_BLOCK_MAX; block_id++) {
            for (pool_id = 0; pool_id < _BCM_KT2_LINKPHY_POOL_MAX; pool_id++) {

                mem = linkphy_counter_table[direction][pool_id];
                copyno = SOC_MEM_BLOCK_ANY(unit, mem) + block_id;
                width = soc_mem_field_length(unit, mem, COUNTf);
                if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
                     continue;
                }

                if (soc_mem_read_range(unit, mem, copyno, 0,
                        (_BCM_KT2_LINKPHY_COUNTER_MAX - 1),
                        linkphy_temp_ctr[unit]) != BCM_E_NONE) {
                    BCM_LINKPHY_COUNTER_UNLOCK(unit);
                    return;
                }
                for (index = 0; index <= (_BCM_KT2_LINKPHY_COUNTER_MAX - 1);
                    index++) {
                    switch (pool_id) {
                        case 0:
                            vptr = &linkphy_frag_byte_ctr[unit][direction]
                                                         [block_id][index];
                            break;
                        case 1:
                            vptr = &linkphy_frag_ctr[unit][direction]
                                                    [block_id][index];
                            break;
                        case 2:
                            vptr = &linkphy_frame_byte_ctr[unit][direction]
                                                          [block_id][index];
                            break;
                        case 3:
                        default:
                            vptr = &linkphy_frame_ctr[unit][direction]
                                                     [block_id][index];
                            break;
                    }
                    LOG_VERBOSE(BSL_LS_BCM_SUBPORT,
                                (BSL_META_U(unit,
                                            "%s_DEBUG_COUNTER%d.lp%d index:%d "
                                            "%s : old count:0x%08x%08x \n"),
                                 direction ? "TXLP" : "RXLP", 
                                 ((pool_id == 3) ? 11 :
                                  ((pool_id == 2) ? 10 :
                                   ((pool_id == 1) ? 9 : 8))),
                                 block_id, index,
                                 ((pool_id == 3) ? "Frame    " : 
                                  ((pool_id == 2) ? "FrameByte" :
                                   ((pool_id == 1) ? "Frag     " : "FragByte "))),
                                 COMPILER_64_HI(*vptr),
                                 COMPILER_64_LO(*vptr)));

                    ctr_prev = COMPILER_64_LO(*vptr);
                    ctr_prev &= ((1UL << width) - 1);

                    soc_mem_field_get(unit, mem,
                        (uint32 *)&linkphy_temp_ctr[unit][index],
                        COUNTf, &ctr_new);

                    if (ctr_new == ctr_diff) {
                        continue;
                    }

                    ctr_diff = ctr_new;

                    if (ctr_diff < ctr_prev) {
                        /* Counter has wrapped around. */
                        if (width < 32) {
                            ctr_diff += (1UL << width);
                        }
                    }

                    ctr_diff -= ctr_prev;

                    COMPILER_64_ADD_32(*vptr, ctr_diff);

                    LOG_VERBOSE(BSL_LS_BCM_SUBPORT,
                                (BSL_META_U(unit,
                                            "%s_DEBUG_COUNTER%d.lp%d index:%d "
                                            "%s : new_count:0x%08x%08x\n"),
                                 direction ? "TXLP" : "RXLP",
                                 ((pool_id == 3) ? 11 :
                                  ((pool_id == 2) ? 10 :
                                   ((pool_id == 1) ? 9 : 8))),
                                 block_id, index, 
                                 ((pool_id == 3) ? "Frame    " :
                                  ((pool_id == 2) ? "FrameByte" :
                                   ((pool_id == 1) ? "Frag     " : "FragByte "))),
                         COMPILER_64_HI(*vptr),
                         COMPILER_64_LO(*vptr)));
                } /*end index */
            }/* end pool */
        } /* end block */
    } /* end direction */
    BCM_LINKPHY_COUNTER_UNLOCK(unit);
    return;
}

STATIC int
_bcm_linkphy_subport_counter_cleanup(int unit)
{
    int direction = 0, block_id = 0;

    if (linkphy_ctr_mutex[unit] != NULL) {
        BCM_LINKPHY_COUNTER_LOCK(unit);
        soc_counter_extra_unregister(unit, _bcm_linkphy_subport_stat_callback);
        BCM_LINKPHY_COUNTER_UNLOCK(unit);
        sal_mutex_destroy(linkphy_ctr_mutex[unit]);
        linkphy_ctr_mutex[unit] = NULL;
    }

    if (linkphy_temp_ctr[unit] != NULL){
        soc_cm_sfree(unit,linkphy_temp_ctr[unit]);
        linkphy_temp_ctr[unit] = NULL;
    }

    for (direction = 0;
        direction < _BCM_KT2_LINKPHY_DIRECTION_MAX; direction++) {
        for (block_id = 0; block_id < _BCM_KT2_LINKPHY_BLOCK_MAX; block_id++) {
            if (linkphy_frag_byte_ctr[unit][direction][block_id] != NULL) {
                 soc_cm_sfree(unit,
                    linkphy_frag_byte_ctr[unit][direction][block_id]);
                linkphy_frag_byte_ctr[unit][direction][block_id] = NULL;
            }
            if (linkphy_frag_ctr[unit][direction][block_id] != NULL) {
                soc_cm_sfree(unit,
                    linkphy_frag_ctr[unit][direction][block_id]);
                linkphy_frag_ctr[unit][direction][block_id] = NULL;
            }
            if (linkphy_frame_byte_ctr[unit][direction][block_id] != NULL) {
                soc_cm_sfree(unit,
                    linkphy_frame_byte_ctr[unit][direction][block_id]);
                linkphy_frame_byte_ctr[unit][direction][block_id] = NULL;
            }
            if (linkphy_frame_ctr[unit][direction][block_id] != NULL) {
                soc_cm_sfree(unit,
                    linkphy_frame_ctr[unit][direction][block_id]);
                linkphy_frame_ctr[unit][direction][block_id] = NULL;
            }
        }
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_linkphy_subport_counter_init(int unit)
{
    int direction = 0, block_id = 0, pool_id = 0, copyno = 0, index = 0;
    uint32 temp_alloc_size = 0, block_alloc_size = 0;
    soc_mem_t mem;

    _bcm_linkphy_subport_counter_cleanup(unit);

    if (linkphy_ctr_mutex[unit] == NULL) {
        linkphy_ctr_mutex[unit] = sal_mutex_create("subport counter mutex");
        if (linkphy_ctr_mutex[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }

    if (linkphy_temp_ctr[unit] != NULL){
        LOG_WARN(BSL_LS_BCM_SUBPORT,
                 (BSL_META_U(unit,
                             "WARN:Freeing linkphy_temp_ctr\n")));
        soc_cm_sfree(unit,linkphy_temp_ctr[unit]);
        linkphy_temp_ctr[unit] = NULL;
    }

    temp_alloc_size =
        sizeof(rxlp_debug_counter7_entry_t) * _BCM_KT2_LINKPHY_COUNTER_MAX;
    linkphy_temp_ctr[unit] = soc_cm_salloc(unit,
                      temp_alloc_size, "linkphy temp counter");

    if (linkphy_temp_ctr[unit] == NULL) {
        LOG_WARN(BSL_LS_BCM_SUBPORT,
                 (BSL_META_U(unit,
                             "linkphy temp allocation for unit:%d failed\n"),
                  unit));
        _bcm_linkphy_subport_counter_cleanup(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(linkphy_temp_ctr[unit], 0, temp_alloc_size);
    LOG_DEBUG(BSL_LS_BCM_SUBPORT,
              (BSL_META_U(unit,
                          "linkphy temp counter size:%d \n"),
               temp_alloc_size));


    for (direction = 0;
        direction < _BCM_KT2_LINKPHY_DIRECTION_MAX; direction++) {
        for (block_id = 0; block_id < _BCM_KT2_LINKPHY_BLOCK_MAX; block_id++) {
            for (pool_id = 0; pool_id < _BCM_KT2_LINKPHY_POOL_MAX; pool_id++) {

                mem = linkphy_counter_table[direction][pool_id];
                copyno = SOC_MEM_BLOCK_ANY(unit, mem) + block_id;
                if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
                     continue;
                }
                if (soc_mem_write_range(unit, mem, copyno,
                    0,
                    (_BCM_KT2_LINKPHY_COUNTER_MAX - 1),
                    linkphy_temp_ctr[unit])) {
                    LOG_ERROR(BSL_LS_BCM_SUBPORT,
                              (BSL_META_U(unit,
                                          "ERROR: zeroing LinkPHY counter failed\n")));
                    soc_cm_sfree(unit,linkphy_temp_ctr[unit]);
                    linkphy_temp_ctr[unit] = NULL;
                }
            }
        }
    }

    block_alloc_size = sizeof(uint64) * _BCM_KT2_LINKPHY_COUNTER_MAX;

    for (direction=0; direction < _BCM_KT2_LINKPHY_DIRECTION_MAX; direction++) {
        for (block_id = 0; block_id < _BCM_KT2_LINKPHY_BLOCK_MAX; block_id++) {
            /* fragment byte */
            if (linkphy_frag_byte_ctr[unit][direction][block_id] != NULL) {
                LOG_WARN(BSL_LS_BCM_SUBPORT,
                         (BSL_META_U(unit,
                                     "WARN:Freeing linkphy_frag_byte_ctr\n")));
                soc_cm_sfree(unit,
                    linkphy_frag_byte_ctr[unit][direction][block_id]);
                linkphy_frag_byte_ctr[unit][direction][block_id] = NULL;
            }

            linkphy_frag_byte_ctr[unit][direction][block_id] = 
                 soc_cm_salloc(unit, block_alloc_size,
                               "linkphy_fragment_byte counter");
            if (linkphy_frag_byte_ctr[unit][direction][block_id] == NULL) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "linkphy_frag_byte_ctr failed for "
                                       " unit:%d,dir:%d, block:%d \n"), unit, direction, block_id));
                _bcm_linkphy_subport_counter_cleanup(unit);
                return BCM_E_MEMORY;
            }
            sal_memset(linkphy_frag_byte_ctr[unit][direction][block_id],
                0, block_alloc_size);

            /* fragment */
            if (linkphy_frag_ctr[unit][direction][block_id] != NULL) {
                LOG_WARN(BSL_LS_BCM_SUBPORT,
                         (BSL_META_U(unit,
                                     "WARN:Freeing linkphy_frag_ctr\n")));
                soc_cm_sfree(unit,
                    linkphy_frag_ctr[unit][direction][block_id]);
                linkphy_frag_ctr[unit][direction][block_id] = NULL;
            }

            linkphy_frag_ctr[unit][direction][block_id] = 
                 soc_cm_salloc(unit, block_alloc_size,
                     "linkphy_fragment counter");
            if (linkphy_frag_ctr[unit][direction][block_id] == NULL) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "linkphy_frag_ctr failed for "
                                       " unit:%d,dir:%d, block:%d \n"), unit, direction, block_id));
                soc_cm_sfree(unit,
                    linkphy_frag_byte_ctr[unit][direction][block_id]);
                linkphy_frag_byte_ctr[unit][direction][block_id] = NULL;
                _bcm_linkphy_subport_counter_cleanup(unit);
                return BCM_E_MEMORY;
            }
            sal_memset(
              linkphy_frag_ctr[unit][direction][block_id], 0, block_alloc_size);

            /* frame byte */
            if (linkphy_frame_byte_ctr[unit][direction][block_id] != NULL) {
                LOG_WARN(BSL_LS_BCM_SUBPORT,
                         (BSL_META_U(unit,
                                     "WARN:Freeing linkphy_frame_byte_ctr\n")));
                soc_cm_sfree(unit,
                    linkphy_frame_byte_ctr[unit][direction][block_id]);
            }
            linkphy_frame_byte_ctr[unit][direction][block_id] = 
                 soc_cm_salloc(unit, block_alloc_size,
                     "linkphy_frame_byte counter");
            if (linkphy_frame_byte_ctr[unit][direction][block_id] == NULL) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "linkphy_frame_byte_ctr failed for "
                                       "unit:%d,dir:%d, block:%d \n"), unit, direction, block_id));
                soc_cm_sfree(unit,
                    linkphy_frag_byte_ctr[unit][direction][block_id]);
                soc_cm_sfree(unit,
                    linkphy_frag_ctr[unit][direction][block_id]);
                linkphy_frag_byte_ctr[unit][direction][block_id] = NULL;
                linkphy_frag_ctr[unit][direction][block_id] = NULL;
                linkphy_frame_byte_ctr[unit][direction][block_id] = NULL;
                _bcm_linkphy_subport_counter_cleanup(unit);
                return BCM_E_MEMORY;
            }
            sal_memset(linkphy_frame_byte_ctr[unit][direction][block_id],
                0, block_alloc_size);

            /* frame */
            if (linkphy_frame_ctr[unit][direction][block_id] != NULL) {
                LOG_WARN(BSL_LS_BCM_SUBPORT,
                         (BSL_META_U(unit,
                                     "WARN:Freeing linkphy_frame_ctr\n")));
                soc_cm_sfree(unit,
                    linkphy_frame_ctr[unit][direction][block_id]);
            }
            linkphy_frame_ctr[unit][direction][block_id] = 
                 soc_cm_salloc(unit, block_alloc_size, "linkphy_frame counter");
            if (linkphy_frame_ctr[unit][direction][block_id] == NULL) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "linkphy_frame_ctr failed for "
                                       "unit:%d,dir:%d block:%d\n"), unit, direction, block_id));
                soc_cm_sfree(unit,
                    linkphy_frag_byte_ctr[unit][direction][block_id]);
                soc_cm_sfree(unit,
                    linkphy_frag_ctr[unit][direction][block_id]);
                soc_cm_sfree(unit,
                    linkphy_frame_byte_ctr[unit][direction][block_id]);
                linkphy_frag_byte_ctr[unit][direction][block_id] = NULL;
                linkphy_frag_ctr[unit][direction][block_id] = NULL;
                linkphy_frame_byte_ctr[unit][direction][block_id] = NULL;
                _bcm_linkphy_subport_counter_cleanup(unit);
                return BCM_E_MEMORY;
            }
            sal_memset(linkphy_frame_ctr[unit][direction][block_id],
                0, block_alloc_size);

            for (index = 0; index < _BCM_KT2_LINKPHY_COUNTER_MAX; index++) {
                COMPILER_64_ZERO (
                    linkphy_frag_ctr[unit][direction][block_id][index]);
                COMPILER_64_ZERO (
                    linkphy_frag_byte_ctr[unit][direction][block_id][index]);
                COMPILER_64_ZERO (
                    linkphy_frame_ctr[unit][direction][block_id][index]);
                COMPILER_64_ZERO (
                    linkphy_frame_byte_ctr[unit][direction][block_id][index]);
            }
        }
    }

    soc_counter_extra_register(unit, _bcm_linkphy_subport_stat_callback);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_port_stat_set
 * Purpose:
 *      Set the statistics value for LinkPHY subport port and statistics type
 * Parameters:
 *      unit      - (IN) Device Number
 *      subport   - (IN) subport port
 *      stream_id - (IN) stream ID array index
 *      stat_type - (IN) statistics type
 *      val       - (IN) value
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_port_stat_set(
    int unit,
    bcm_gport_t subport,
    int stream_id,
    bcm_subport_stat_t stat_type,
    uint64 val)
{
    int direction = 0, block = 0, pool = 0, copyno = 0;
    int rv = BCM_E_NONE, counter_id = 0, i = 0;
    uint16 ext_stream_id_base = 0;
    uint32 val32 = 0, count_mask = 0;
    bcm_port_t port;
    bcm_subport_config_t config;
    soc_mem_t mem;
    rxlp_debug_counter7_entry_t temp_counter;

    /* get port from subport */
    bcm_subport_config_t_init(&config);
    rv = _bcm_linkphy_subport_port_get(unit, subport, &config);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: _bcm_linkphy_subport_port_get failed \n"
                               "for subport 0x%x\n"), subport));
        return rv;
    }

    port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(config.group);
    rv = _bcm_linkphy_port_ext_stream_id_base_get(unit, port,
            &ext_stream_id_base);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy port %d external "
                               "stream id base get failed\n"), port));
        return rv;
    }

    if ((stream_id != -1) && (stream_id >= config.num_streams)) {
        return BCM_E_PARAM;
    }

    switch (stat_type) {
        case bcmSubportLinkPhyStatRxFragmentBytes:
            direction = _BCM_KT2_LINKPHY_DIRECTION_RX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAG_BYTE;
            break;
        case bcmSubportLinkPhyStatRxFragments:
            direction = _BCM_KT2_LINKPHY_DIRECTION_RX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAG;
            break;
        case bcmSubportLinkPhyStatRxFrameBytes:
            direction = _BCM_KT2_LINKPHY_DIRECTION_RX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAME_BYTE;
            break;
        case bcmSubportLinkPhyStatRxFrames:
            direction = _BCM_KT2_LINKPHY_DIRECTION_RX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAME;
            break;
        case bcmSubportLinkPhyStatTxFragmentBytes:
            direction = _BCM_KT2_LINKPHY_DIRECTION_TX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAG_BYTE;
            break;
        case bcmSubportLinkPhyStatTxFragments:
            direction = _BCM_KT2_LINKPHY_DIRECTION_TX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAG;
            break;
        case bcmSubportLinkPhyStatTxFrameBytes:
            direction = _BCM_KT2_LINKPHY_DIRECTION_TX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAME_BYTE;
            break;
        case bcmSubportLinkPhyStatTxFrames:
            direction = _BCM_KT2_LINKPHY_DIRECTION_TX;
            pool = _BCM_KT2_LINKPHY_POOL_FRAME;
            break;
        default:
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "LinkPHY Statistic not supported: %d\n"), stat_type));
            return BCM_E_PARAM;
            break;
    }

    rv = soc_kt2_linkphy_port_blk_idx_get(unit, port, &block, NULL);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              
                               "ERROR: LinkPHY block, index get failed for port %d\n"), port));
        return rv;
    }

    BCM_LINKPHY_COUNTER_LOCK(unit);
    mem = linkphy_counter_table[direction][pool];
    copyno = SOC_MEM_BLOCK_ANY(unit, mem) + block;

    for (i =0; i < config.num_streams; i++) {
        if (stream_id != i) {
            /* if stream_id == -1 then set the stats for all
            * streams associated with the LinkPHY subport */
            if (stream_id != -1) {
                continue;
            }
        }

        counter_id = config.stream_id_array[i] - ext_stream_id_base;

        /* Read Hard copy */
        if (soc_mem_read(unit, mem, copyno, counter_id,
                         &temp_counter) != BCM_E_NONE) {
            BCM_LINKPHY_COUNTER_UNLOCK(unit);
            return BCM_E_INTERNAL;
        }

        /* Update soft copy */
        if ((stat_type == bcmSubportLinkPhyStatRxFragmentBytes) ||
            (stat_type ==  bcmSubportLinkPhyStatTxFragmentBytes)) {
            COMPILER_64_SET (linkphy_frag_byte_ctr[unit][direction]
                               [block][counter_id],
                               COMPILER_64_HI(val),
                               COMPILER_64_LO(val));
        } else if ((stat_type == bcmSubportLinkPhyStatRxFragments) ||
                   (stat_type == bcmSubportLinkPhyStatTxFragments)) {
            COMPILER_64_SET (linkphy_frag_ctr[unit][direction]
                               [block][counter_id],
                               COMPILER_64_HI(val),
                               COMPILER_64_LO(val));
        } else if ((stat_type == bcmSubportLinkPhyStatRxFrameBytes) ||
                   (stat_type == bcmSubportLinkPhyStatTxFrameBytes)) {
            COMPILER_64_SET (linkphy_frame_byte_ctr[unit][direction]
                               [block][counter_id],
                               COMPILER_64_HI(val),
                               COMPILER_64_LO(val));
        } else {
            /* if ((stat_type == bcmSubportLinkPhyStatRxFrames) ||
                  (stat_type == bcmSubportLinkPhyStatTxFrames)) { */
            COMPILER_64_SET (linkphy_frame_ctr[unit][direction]
                               [block][counter_id],
                               COMPILER_64_HI(val),
                               COMPILER_64_LO(val));
        }

        /* Modify Hard copy */
        count_mask = (1 << soc_mem_field_length(unit, mem, COUNTf)) - 1;
        val32 = COMPILER_64_LO(val) & count_mask;
       
        soc_mem_field_set(unit, mem, (uint32 *)&temp_counter, COUNTf, &val32);
        /* Update Hard Copy */
        if (soc_mem_write(unit, mem, copyno, counter_id,
                          &temp_counter) != BCM_E_NONE){
            BCM_LINKPHY_COUNTER_UNLOCK(unit);
            return BCM_E_INTERNAL;
        }
    }

    BCM_LINKPHY_COUNTER_UNLOCK(unit);
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_linkphy_subport_port_stat_get
 * Purpose:
 *      Get the statistics value for LinkPHY subport port and statistics type
 * Parameters:
 *      unit      - (IN) Device Number
 *      subport   - (IN) subport port
 *      stream_id - (IN) stream ID array index
 *      stat_type - (IN) statistics type
 *      val       - (OUT) value
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_port_stat_get(
    int unit,
    bcm_gport_t subport,
    int stream_id,
    bcm_subport_stat_t stat_type,
    uint64 *val)
{
    int direction = 0, block = 0;
    int rv = BCM_E_NONE, counter_id = 0, i = 0;
    uint16 ext_stream_id_base = 0;
    bcm_port_t port;
    bcm_subport_config_t config;

    /* get port from subport */
    bcm_subport_config_t_init(&config);
    rv = _bcm_linkphy_subport_port_get(unit, subport, &config);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: _bcm_linkphy_subport_port_get failed \n"
                               "for subport 0x%x\n"), subport));
        return rv;
    }

    port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(config.group);

    rv = _bcm_linkphy_port_ext_stream_id_base_get(unit, port,
            &ext_stream_id_base);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy port %d external "
                               "stream id base get failed\n"), port));
        return rv;
    }

    if ((stream_id != -1) && (stream_id >= config.num_streams)) {
        return BCM_E_PARAM;
    }

    rv = soc_kt2_linkphy_port_blk_idx_get(unit, port, &block, NULL);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              
                               "ERROR: LinkPHY block, index get failed for port %d\n"), port));
        return rv;
    }

    switch (stat_type) {
        case bcmSubportLinkPhyStatRxFragmentBytes:
        case bcmSubportLinkPhyStatRxFragments:
        case bcmSubportLinkPhyStatRxFrameBytes:
        case bcmSubportLinkPhyStatRxFrames:
            direction = _BCM_KT2_LINKPHY_DIRECTION_RX;
            break;
        case bcmSubportLinkPhyStatTxFragmentBytes:
        case bcmSubportLinkPhyStatTxFragments:
        case bcmSubportLinkPhyStatTxFrameBytes:
        case bcmSubportLinkPhyStatTxFrames:
            direction = _BCM_KT2_LINKPHY_DIRECTION_TX;
            break;
        default:
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "LinkPHY Statistic not supported: %d\n"), stat_type));
            return BCM_E_PARAM;
            break;
    }

    BCM_LINKPHY_COUNTER_LOCK(unit);

    COMPILER_64_ZERO(*val);

    for (i =0; i < config.num_streams; i++) {
        if (stream_id != i) {
            /* if stream_id == -1 then get the stats for all
            * streams associated with the LinkPHY subport and add them*/
            if (stream_id != -1) {
                continue;
            }
        }

        counter_id = config.stream_id_array[i] - ext_stream_id_base;

        if ((stat_type == bcmSubportLinkPhyStatRxFragmentBytes) ||
            (stat_type ==  bcmSubportLinkPhyStatTxFragmentBytes)) {
            COMPILER_64_ADD_64(*val, linkphy_frag_byte_ctr[unit][direction]
                                                         [block][counter_id]);
        } else if ((stat_type == bcmSubportLinkPhyStatRxFragments) ||
                   (stat_type == bcmSubportLinkPhyStatTxFragments)) {
            COMPILER_64_ADD_64(*val, linkphy_frag_ctr[unit][direction]
                                                    [block][counter_id]);
        } else if ((stat_type == bcmSubportLinkPhyStatRxFrameBytes) ||
                  (stat_type == bcmSubportLinkPhyStatTxFrameBytes)) {
            COMPILER_64_ADD_64(*val, linkphy_frame_byte_ctr[unit][direction]
                                                          [block][counter_id]);
        } else {
            /* if ((stat_type == bcmSubportLinkPhyStatRxFrames) ||
                  (stat_type == bcmSubportLinkPhyStatTxFrames)) { */
            COMPILER_64_ADD_64(*val, linkphy_frame_ctr[unit][direction]
                                                     [block][counter_id]);
        }
    }
    BCM_LINKPHY_COUNTER_UNLOCK(unit);
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_linkphy_subport_init
 * Purpose:
 *      Initialize LinkPHY subport module.
 * Parameters:
 *      unit      - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_init(int unit)
{
    int lp_block = 0, lp_index = 0;
    int rv = BCM_E_NONE, dev_int_stream_max = 0, index = 0;
    bcm_port_t port;
    uint32 regval = 0;
    soc_reg_t reg;
    soc_mem_t mem;
    soc_info_t *si = &SOC_INFO(unit);
    uint32 lp_enable, port_enable;
    uint32 cascaded_port_type, port_type;

    /* Clear TXLP_PORT_STREAM_BITMAP_TABLEm table */
    BCM_IF_ERROR_RETURN
        (soc_mem_clear(unit, TXLP_PORT_STREAM_BITMAP_TABLEm, COPYNO_ALL, TRUE));

    /* Check if pbmp_linkphy members are valid for supporting LinkPHY.
     * For Katana2(BCM56450) port 27 to 34 only support LinkPHY */
    SOC_PBMP_ITER(si->linkphy_pbm, port) {
        if (!_BCM_KT2_PORT_SUPPORTS_LINKPHY(unit, port)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: config variable pbmp_linkphy "
                                   "member port %d is invalid for LinkPHY support\n"), port));
            return BCM_E_PORT;
        }
    }

    SOC_PBMP_ITER(si->linkphy_pbm, port) {
        reg = RXLP_PORT_ENABLEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            BCM_IF_ERROR_RETURN(
                READ_RXLP_PORT_ENABLEr(unit, port, &regval));
            lp_enable = soc_reg_field_get(unit, reg, regval, LP_ENABLEf);

            if (soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                SOC_BLK_RXLP, NULL, &index) == SOC_E_NONE) {
                lp_enable |= ((1 << index) & 0xF);
            }

            regval = 0;
            soc_reg_field_set(unit, reg, &regval, LP_ENABLEf, lp_enable);
            BCM_IF_ERROR_RETURN( WRITE_RXLP_PORT_ENABLEr(unit,port,regval));
        }

        reg = TXLP_PORT_ENABLEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            BCM_IF_ERROR_RETURN(
                READ_TXLP_PORT_ENABLEr(unit, port, &regval));
            lp_enable = soc_reg_field_get(unit, reg, regval, LP_ENABLEf);
            port_enable = soc_reg_field_get(unit, reg, regval,PORT_ENABLEf);

            if (soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                SOC_BLK_TXLP, NULL, &index) == SOC_E_NONE) {
                lp_enable |= ((1 << index) & 0xF);
                port_enable |= ((1 << index) & 0xF);
            }

            regval = 0;
            soc_reg_field_set(unit, reg, &regval, LP_ENABLEf, lp_enable);
            soc_reg_field_set(unit, reg, &regval, PORT_ENABLEf,port_enable);
            BCM_IF_ERROR_RETURN(WRITE_TXLP_PORT_ENABLEr(unit, port,regval));
        }

        port_type = _BCM_KT2_PORT_TYPE_CASCADED;
        cascaded_port_type = _BCM_KT2_PORT_TYPE_CASCADED_LINKPHY;

        mem = ING_PHYSICAL_PORT_TABLEm;
        if (SOC_MEM_FIELD_VALID(unit, mem, PORT_TYPEf)) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify
                (unit, mem, port, PORT_TYPEf, port_type));
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                unit, mem, port, CASCADED_PORT_TYPEf, cascaded_port_type));
        }

        mem = EGR_PHYSICAL_PORTm;
        if (SOC_MEM_FIELD_VALID(unit, mem, PORT_TYPEf)) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                unit, mem, port, PORT_TYPEf, port_type));
        }
        if (SOC_MEM_FIELD_VALID(unit, mem, CASCADED_PORT_TYPEf)) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                unit, mem, port, CASCADED_PORT_TYPEf, cascaded_port_type));
        }

        /*
        * Set the RXLP/TXLP internal stream id base to 128 for block number 1
        * This is needed to support 256 streams
        */

        rv = soc_kt2_linkphy_port_blk_idx_get(unit,port,
                                             &lp_block, &lp_index);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  
                                   "ERROR: LinkPHY block, index get failed for port %d\n"), port));
            return rv;
        }

        reg = RXLP_INT_STREAM_ID_BASEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            soc_reg_field_set(unit, reg, &regval, BASEf, lp_block ? 128 : 0);
            BCM_IF_ERROR_RETURN(
                WRITE_RXLP_INT_STREAM_ID_BASEr(unit, port, regval));
        }
        reg = TXLP_INT_STREAM_ID_BASEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            soc_reg_field_set(unit, reg, &regval, BASEf, lp_block ? 128 : 0);
            BCM_IF_ERROR_RETURN(
                WRITE_TXLP_INT_STREAM_ID_BASEr(unit, port, regval));
        }
    }

    /* LinkPHY group bitmap to keep track of used LinkPHY groups */
    if (_bcm_linkphy_group_bitmap[unit] == NULL) {

        _bcm_linkphy_group_bitmap[unit] = 
            sal_alloc(SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_GROUP_MAX),
            "linkphy_subport_group_bitmap");

        if (_bcm_linkphy_group_bitmap[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:subport_init: group bitmap alloc failed\n")));

            return BCM_E_MEMORY;
        }
    }

    sal_memset(_bcm_linkphy_group_bitmap[unit], 0,
        SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_GROUP_MAX));

   /* LinkPHY device internal stream ID bitmap to keep track of the device
   * internal stream ID used. A device internal stream id can be active on
   * one port at a time.
   */
    dev_int_stream_max = 2 * _BCM_KT2_SUBPORT_PORT_MAX;
    if (_bcm_dev_int_stream_id_bitmap[unit] == NULL) {

        _bcm_dev_int_stream_id_bitmap[unit] =
            sal_alloc(SHR_BITALLOCSIZE(dev_int_stream_max),
                "dev_int_stream_id_bitmap");

        if (_bcm_dev_int_stream_id_bitmap[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:subport_init: dev_int_stream_id_bitmap "
                                   " alloc failed\n")));

            return BCM_E_MEMORY;
        }
    }
    sal_memset(_bcm_dev_int_stream_id_bitmap[unit], 0,
        SHR_BITALLOCSIZE(dev_int_stream_max));

    /* LinkPHY subport port info per pp_port */
    if (_bcm_linkphy_subport_port_info[unit] == NULL) {
        _bcm_linkphy_subport_port_info[unit] =
            sal_alloc((SOC_MAX_NUM_PP_PORTS *
                       sizeof(_bcm_linkphy_subport_port_info_t)),
                       "linkphy_subport_port_info");
        if (_bcm_linkphy_subport_port_info[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: "
                                   " linkphy_subport_port_info alloc failed\n")));

                return BCM_E_MEMORY;
        }
    }

    sal_memset(_bcm_linkphy_subport_port_info[unit], 0,
        (SOC_MAX_NUM_PP_PORTS * sizeof(_bcm_linkphy_subport_port_info_t)));

    for (index = 0; index < SOC_MAX_NUM_PP_PORTS; index++) {
        _bcm_linkphy_subport_port_info[unit][index].group = -1;
        _bcm_linkphy_subport_port_info[unit][index].num_streams = -1;
    }

    /* Set the STP state of subports */
    BCM_PBMP_ITER(SOC_INFO(unit).linkphy_pp_port_pbm, port) {
        bcm_esw_port_learn_set(unit, port,
                BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD);
        bcm_esw_port_stp_set(unit, port, BCM_STG_STP_FORWARD);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_group_create
 * Purpose:
 *      Create a LinkPHY subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport group config information
 *      group  - (OUT) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_group_create(int unit,
                                  int is_trunk, bcm_port_t port,
                                  int group_id, bcm_gport_t *group)
{
    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    _BCM_KT2_LINKPHY_SUBPORT_GROUP_SET(*group, port, group_id);
    _BCM_KT2_SUBPORT_GROUP_TYPE_CASCADED_SET(*group);

    _BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_SET(unit, group_id);
    _bcm_linkphy_subport_group_count[unit]++;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_group_destroy
 * Purpose:
 *      Destroy the LinkPHY subport group and all subport ports attached to it.
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier for subport group
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_linkphy_subport_group_destroy(int unit,
                                   bcm_gport_t group)
{
    int group_id = -1;
    int pp_port = 0, rv = BCM_E_NONE;
    int linkphy_subport_count = 0;
    bcm_gport_t subport_port_gport;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (_BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) {

        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(group);

        if (!_BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY subport group 0x%x not found.\n"),
                       group));
            return BCM_E_NOT_FOUND;
        }

        linkphy_subport_count =
            *(_bcm_subport_group_subport_port_count[unit] + group_id);

        /* delete all subport ports attached to the subport group */
        BCM_PBMP_ITER(SOC_INFO(unit).linkphy_pp_port_pbm, pp_port) {
            if (linkphy_subport_count == 0) {
                break;
            }

            if (!_BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port)) {
                continue;
            }

            if (group !=
                _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port)) {
                continue;
            }

            _BCM_KT2_SUBPORT_PORT_ID_SET(subport_port_gport, pp_port);

            _BCM_KT2_SUBPORT_PORT_TYPE_SET(subport_port_gport,
                                           _BCM_KT2_SUBPORT_TYPE_LINKPHY);

            rv = _bcm_linkphy_subport_port_delete(unit, subport_port_gport);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: LinkPHY subport group 0x%x destroy failed \n"
                                      " to delete subport port (subport group id %d, "
                                      " pp_port id %d)\n"),
                           group, group_id, pp_port));
                return rv;
            }
            linkphy_subport_count--;
        }

        *(_bcm_subport_group_subport_port_count[unit] + group_id) = 0;
        /* delete the subport group */
        _bcm_subport_group_count[unit]--;
        _BCM_KT2_SUBPORT_GROUP_USED_CLR(unit, group_id);
        _bcm_linkphy_subport_group_count[unit]--;
        _BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_CLR(unit, group_id);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_group_get
 * Purpose:
 *      Get a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier
 *      config - (OUT) Subport group config information
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_group_get(int unit,
                               bcm_gport_t group,
                               bcm_subport_group_config_t *config)
{
    int group_id = -1;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    if (_BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) {
        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(group);
        if (!_BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY subport group 0x%x not found.\n"),
                       group));
            return BCM_E_NOT_FOUND;;
        }
        config->flags |= BCM_SUBPORT_GROUP_TYPE_LINKPHY;
        config->port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(group);
    } else {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_group_traverse
 * Purpose:
 *      Traverse all valid subports for given LinkPHY subport group
 *      and call the user supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      group     - Subport group GPORT identifier
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 */

STATIC int
_bcm_linkphy_subport_group_traverse(int unit,
                                    bcm_gport_t group,
                                    bcm_subport_port_traverse_cb cb,
                                    void *user_data)
{
    int rv = BCM_E_NONE, group_id = 0;
    int pp_port = 0, subport_port;
    int linkphy_subport_count = 0;
    bcm_subport_config_t config;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    bcm_subport_config_t_init(&config);

    if (_BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) {
        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(group);

        if (!_BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY subport group 0x%x not found.\n"),
                       group));
            return BCM_E_NOT_FOUND;
        }

        linkphy_subport_count =
            *(_bcm_subport_group_subport_port_count[unit] + group_id);
        
        BCM_PBMP_ITER(SOC_INFO(unit).linkphy_pp_port_pbm, pp_port) {

            if (linkphy_subport_count == 0) {
                break;
            }

            if (!_BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port)) {
                continue;
            }

            if (group !=
                _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port)) {
                continue;
            }

            _BCM_KT2_SUBPORT_PORT_ID_SET(subport_port, pp_port);

            _BCM_KT2_SUBPORT_PORT_TYPE_SET(subport_port,
                                           _BCM_KT2_SUBPORT_TYPE_LINKPHY);

            rv = _bcm_linkphy_subport_port_get(unit, subport_port, &config);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: LinkPHY subport group 0x%x traverse failed \n"
                                      "to get subport port (group id %d, pp_port id %d)\n"),
                           group, group_id, pp_port));
                return rv;
            }

            rv = cb(unit, subport_port, &config, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: subport group 0x%x traverse failed \n"
                                       "to callback for subport port (group id %d, "
                                       "subport id %d)\n"), group, group_id, pp_port));
                return rv;
            }
#endif
            linkphy_subport_count--;
        }
    } else {
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_port_add
 * Purpose:
 *      Add a LinkPHY subport to a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport config information
 *      port   - (OUT) Subport_port GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_port_add(int unit,
                              bcm_subport_config_t *config,
                              bcm_gport_t *gport)
{
    int rv = BCM_E_NONE, port = 0, enable = 0;
    int sp_group_idx = -1, i = 0;
    int start_addr = 0, end_addr = 0;
    uint16 int_stream_id_base = 0, ext_stream_id_base = 0;
    uint16 dev_int_stream_id = 0, int_stream_id = 0;
    uint32 pp_port_index = 0;
    soc_mem_t mem;
    soc_info_t *si = &SOC_INFO(unit);
    device_stream_id_to_pp_port_map_entry_t dev_sid_to_pp_entry;
    pp_port_to_physical_port_map_entry_t    pp_to_port_entry;

    /* A. PARAMETER VALIDATION
    *******************************/
    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    /* Make sure the group exists */
    if (!_BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(config->group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: subport group 0x%x is not LinkPHY group\n"),
                   config->group));
        return BCM_E_PARAM;
    }

    sp_group_idx =
        _BCM_KT2_SUBPORT_GROUP_ID_GET(config->group);
    if (!_BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_GET(unit, sp_group_idx)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: subport group 0x%x does not exist. group id =%d\n"),
                   config->group, sp_group_idx));
        return BCM_E_NOT_FOUND;
    }

    port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(config->group);

    /* Check if port is member of pbmp_linkphy */
    if (!SOC_PBMP_MEMBER(si->linkphy_pbm, port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Port %d is not member of pbmp_linkphy\n"),
                   port));
        return BCM_E_PORT;
    }

    if (si->port_num_subport[port] <= 0) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: %d number of subports available for port %d\n"),
                   si->port_num_subport[port], port));
        return BCM_E_CONFIG;
    }

    if (config->num_streams > BCM_SUBPORT_CONFIG_MAX_STREAMS) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: num_streams %d cannot be greater than %d\n"),
                   config->num_streams, BCM_SUBPORT_CONFIG_MAX_STREAMS));
        return BCM_E_PARAM;
    }

    if (config->prop_flags & BCM_SUBPORT_PROPERTY_PHB) {
        if ((config->int_pri < 0 || config->int_pri >= (1 << 4)) ||
            ((config->color != bcmColorGreen) &&
             (config->color != bcmColorYellow) &&
             (config->color != bcmColorRed))) {
             LOG_ERROR(BSL_LS_BCM_SUBPORT,
                       (BSL_META_U(unit,
                                   "ERROR: int_pri %d or color %d is invalid\n"),
                        config->int_pri, config->color));
            return BCM_E_PARAM;
        }
    }

    rv = _bcm_linkphy_port_int_stream_id_base_get(unit, port,
            &int_stream_id_base);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy port %d internal "
                               "stream id base get failed\n"), port));
        return rv;
    }

    rv = _bcm_linkphy_port_ext_stream_id_base_get(unit, port,
            &ext_stream_id_base);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy port %d external "
                               "stream id base get failed\n"), port));
        return rv;
    }

    for (i = 0; i < config->num_streams; i++) {
        /* Check stream_id range for the port */
        if (config->stream_id_array[i] >=
            (ext_stream_id_base + _BCM_KT2_LINKPHY_PER_PORT_STREAMS_MAX)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: stream_id %d is out of range for port %d\n"),
                       config->stream_id_array[i], port));
            return BCM_E_PARAM;
        }

        /*
        * Check if the device internal stream_id is already active 
        * Device Internal stream id can be active only on one port at a time.
        */
        int_stream_id = config->stream_id_array[i] - ext_stream_id_base;
        dev_int_stream_id = int_stream_id + int_stream_id_base;

        if (dev_int_stream_id > soc_mem_index_max(unit,
                                    DEVICE_STREAM_ID_TO_PP_PORT_MAPm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: device internal stream_id %d is out of range for \n"
                                  "port %d. internal stream id base %d, internal stream id %d\n"),
                       dev_int_stream_id, port, int_stream_id_base, int_stream_id));

            return BCM_E_PARAM;
        }

        if (_BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_GET(unit,
                dev_int_stream_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Device internal stream_id %d already used.\n"),
                       dev_int_stream_id));
            return BCM_E_EXISTS;
        }
    }

    /* B. PP_PORT INDEX RESERVATION
    *************************************/
    for (i = si->port_subport_base[port];
        i <  si->port_subport_base[port] + si->port_num_subport[port]; i++) {
        if (!_BCM_KT2_SUBPORT_PP_PORT_USED_GET(unit, i)) {
            pp_port_index = i;
            break;
        }
    }

    if (i == (si->port_subport_base[port] + si->port_num_subport[port])) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: All %d subport pp_port already used.\n"),
                   (i - si->port_subport_base[port])));
        return BCM_E_FULL;
    }

    /* C. RXLP/TXLP PROGRAMMING
    *********************************/
    for (i = 0; i < config->num_streams; i++) {

        int_stream_id = config->stream_id_array[i] - ext_stream_id_base;

        /* 
        * Map Device Internal Stream ID to pp_port_index
        * Use the DEVICE_STREAM_ID_TO_PP_PORT_MAP table
        */

        dev_int_stream_id = int_stream_id + int_stream_id_base;
        mem = DEVICE_STREAM_ID_TO_PP_PORT_MAPm;

        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, dev_int_stream_id,
                &dev_sid_to_pp_entry);

        soc_mem_field32_set(unit, mem, (uint32 *)&dev_sid_to_pp_entry,
            PP_PORTf, pp_port_index);

        /* Set PHB for the subport */
        if (config->prop_flags & BCM_SUBPORT_PROPERTY_PHB) {
            soc_mem_field32_set(unit, mem, (uint32 *)&dev_sid_to_pp_entry,
                PHB_VALIDf, 1);
            soc_mem_field32_set(unit, mem, (uint32 *)&dev_sid_to_pp_entry,
                INT_PRIf, config->int_pri);
            soc_mem_field32_set(unit, mem, (uint32 *)&dev_sid_to_pp_entry,
                CNGf, config->color);
        }

        rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, dev_int_stream_id,
                &dev_sid_to_pp_entry);

        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: device internal stream id  %d to pp_port %d "
                                   "map failed.\n"), dev_int_stream_id, pp_port_index));

            return rv;
        }

        /*
        * Steps to Enable New-Stream 
        *  while traffic is flowing on other streams of a Port.:
        * ========================================================
        *    TXLP:
        * 1.First program the active-stream-bitmap-table with this stream.
        * 2.Program the INTERNAL-2-EXTERNAL mapping table for the new-stream.
        * 3.Program the Address-Map-table for that stream.
        * 4.After Step-3, TXLP will send requests to MMU for this stream
        *     and traffic will commence.
        *
        *     RXLP:
        * 1.Reconfigure RXLP_PORT_ACTIVE_STREAM_BITMAP_LO/HI based on
        *      the internal stream ID of the new stream.
        * 2.Configure mapping of external stream ID to internal space in the
        *      table RXLP_INTERNAL_STREAM_MAP_PORT_<0/1/2/3> depending on
        *      the port where the new stream is added.
        * 3.Configure table RXLP_MAX_FRAME_SIZE for the newly added stream ID.
        */

        /* TXLP */

        enable = 1;

        rv = _bcm_linkphy_port_tx_active_stream_set(unit,
                port, int_stream_id, enable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: TX active stream bitmap enable failed.\n"
                                   "port %d, internal stream_id = %d.\n"), port, int_stream_id));

            return rv;
        }

        rv = _bcm_linkphy_port_tx_int_to_ext_stream_map_set(unit, 
                port, config->stream_id_array[i], int_stream_id, enable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: TX internal to external stream mapping failed.\n"
                                  "port %d, internal stream_id = %d, external stream id = %d.\n"),
                       port, int_stream_id, config->stream_id_array[i]));

            return rv;
        }

        /*
        * Add start and end address management for TXL Data Buffer.
        * 856 cells have to be distributed across 128 internal streams.
        * Assuming that by default each stream speed is < 1G we will 
        * reserve 6 cells for each stream by default (6 * 128 = 768).
        * Remainig cells can be used when stream speed is
        * set to 10G as per following scheme.
        * Stream-Speed(Gbps) |  Min Num.Cells
        *    < 1G            |     3
        *      1G            |     6
        *      10G           |     50 
        */

        /*
        * each cell occupies 4 lines of buffer.
        * Assume that by default each stream speed is  1G.
        * so 24 lines of buffer are needed.
        */
        start_addr = (_BCM_KT2_LINKPHY_TX_DATA_BUF_END_ADDR_MIN + 1) *
                     (pp_port_index - _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN);
        end_addr = start_addr + _BCM_KT2_LINKPHY_TX_DATA_BUF_END_ADDR_MIN;

        rv = _bcm_linkphy_port_tx_stream_addr_map_set(unit,
                port, int_stream_id, start_addr, end_addr, enable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: TX data buffer address set failed.\n"
                                   "port %d, internal stream_id = %d, start_addr = %d "
                                   "end_addr = %d.\n"), port, int_stream_id, start_addr, end_addr));

            return rv;
        }

        /* RXLP */

        rv = _bcm_linkphy_port_rx_active_stream_bitmap_set(unit,
                port, int_stream_id, enable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: RX active stream enable failed\n"
                                   " port %d, internal stream_id = %d.\n"), port, int_stream_id));

            return rv;
        }

        rv = _bcm_linkphy_port_rx_int_stream_map_port_set(unit,
                port, int_stream_id, enable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: RX stream map for port enable failed.\n"
                                   "port %d, internal stream_id = %d.\n"), port, int_stream_id));

            return rv;
        }

        _BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_SET(unit, dev_int_stream_id);

        _bcm_linkphy_subport_port_info[unit]
            [pp_port_index].ext_stream_idx[i] = config->stream_id_array[i];
    }

    /*
    * Map PP_PORT to Physical port mapping (IP/ EP):
    * Use the PP_PORT_TO_PHYSICAL_PORT_MAP
    */
    mem = PP_PORT_TO_PHYSICAL_PORT_MAPm;
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, pp_port_index,
            &pp_to_port_entry);
    
    soc_mem_field32_set(unit, mem, (uint32 *)&pp_to_port_entry,
        DESTINATIONf, port);
    soc_mem_field32_set(unit, mem, (uint32 *)&pp_to_port_entry,
        ISTRUNKf, 0);

    rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, pp_port_index,
            &pp_to_port_entry);

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: pp_port id %d to physical port %d "
                               "map failed.\n"), pp_port_index, port));
        for (i = 0; i < config->num_streams; i++) {
                int_stream_id = config->stream_id_array[i] - ext_stream_id_base;
            dev_int_stream_id = int_stream_id + int_stream_id_base;
            _BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_CLR(unit,
                                                dev_int_stream_id);
        }
        return rv;
    }

    _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_SET(unit, pp_port_index,
        config->group, config->num_streams, TRUE);

    _BCM_KT2_SUBPORT_PORT_ID_SET(*gport, pp_port_index);

    _BCM_KT2_SUBPORT_PORT_TYPE_SET(*gport, _BCM_KT2_SUBPORT_TYPE_LINKPHY);

    *(_bcm_subport_group_subport_port_count[unit]+ sp_group_idx) += 1;

    _BCM_KT2_SUBPORT_PP_PORT_USED_SET(unit, pp_port_index);
    _bcm_subport_pp_port_count[unit]++;

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return rv;
}

/*
 * Function:
 *      _bcm_linkphy_subport_port_delete
 * Purpose:
 *      Delete a LinkPHY subport_port
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) Subport_port GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_port_delete(int unit,
                                 bcm_gport_t subport_port_gport)
{
    int rv = BCM_E_NONE, group_id = 0;
    int port = 0, subport_group = -1, disable = 0, poll_active = 0;
    int dev_int_stream_id = 0, num_streams = 0, i = 0;
    uint16 int_stream_id_base = 0, ext_stream_id_base = 0;
    uint16 int_stream_id = 0, pp_port_index = 0;
    uint32 timeoutval = 0, regval = 0;
    soc_timeout_t timeout;
    soc_reg_t reg;
    soc_mem_t mem;

    if (!_BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, subport_port_gport)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: gport 0x%x is not LinkPHY subport port type.\n"),
                   subport_port_gport));
        return BCM_E_PARAM;
    }

    pp_port_index = _BCM_KT2_SUBPORT_PORT_ID_GET(subport_port_gport);

    if (!_BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port_index)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: LinkPHY subport port gport 0x%x is invalid\n"),
                   subport_port_gport));
        return BCM_E_NOT_FOUND;
    }

    num_streams = _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_NUM_STREAMS_GET(unit,
                      pp_port_index);
    if (num_streams == -1) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Subport port 0x%x does not have any streams.\n"),
                   subport_port_gport));
        return BCM_E_NOT_FOUND;
    }

    subport_group =
        _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port_index);

    port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(subport_group);

    rv = _bcm_linkphy_port_int_stream_id_base_get(unit, port,
            &int_stream_id_base);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy subport group internal stream id "
                               "base get failed for port %d.\n"), port));
        return rv;
    }

    rv = _bcm_linkphy_port_ext_stream_id_base_get(unit, port,
            &ext_stream_id_base);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy subport group external stream id "
                               "base get failed for port %d.\n"), port));
        return rv;
    }

    /*Disable a Stream while traffic is flowing on other streams of a Port:
    *      RXLP:
    * 1.Set the bit corresponding to the internal stream ID in the register
    *   RXLP_DFC_STATUS_LO/HI to XON. 
    *   This is to enable flushing of the data egressing to that stream 
    *   from the TXLP.
    * 2.Ensure that the stream being disabled in not sending in data anymore
    * 3.Flush the left over data (partial cell, if any) for the stream
    *   in the RXLP by writing to the register RXLP_SW_FLUSH_CONTROL. 
    * 4.Poll the FLUSH_DONE bit in the register RXLP_SW_FLUSH_CONTROL.
    *   Once this bit is read as 1, proceed to the next step.
    * 5.Write 0 to the bit corresponding to the internal stream ID 
    *   in the register RXLP_PORT_ACTIVE_STREAM_BITMAP_LO/HI.
    * 6.Clear the configuration corresponding to the stream in
    *   the table RXLP_INTERNAL_STREAM_MAP_PORT_<0/1/2/3> depending
    *   on the port where the stream being disabled resides.
    *
    *       TXLP:
    * 1. Disabling a stream MUST be done after all the packets 
    *    for that stream have been drained from {MMU-TXLP} sub-system.
    *    This is like Stream/Queue-flush initiated by MMU.
    * 2.Once it has been ascertained that {MMU, TXLP} have been drained,
    *   only then can s/w disable the stream by removing the stream
    *   from ACTIVE-STREAM-BITMAP for that port.
    * 3.ADDR-MAP for this stream should be programmed to ZEROS.
    *   This will also RESET the outstanding MMU-Credit for that SID.
    *   (The same must also be done in MMU).
    */

    for (i = 0; i < num_streams; i++) {

        int_stream_id = _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_EXT_STREAM_ID_GET(
                        unit, pp_port_index, i) - ext_stream_id_base;

        dev_int_stream_id = int_stream_id + int_stream_id_base;
        mem = DEVICE_STREAM_ID_TO_PP_PORT_MAPm;
        if (dev_int_stream_id > soc_mem_index_max(unit,mem)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: device internal stream_id %d is out of range \n"
                                  "port %d, internal stream id base %d, internal stream id %d "
                                  " pp_port %d\n"),
                       dev_int_stream_id, port,
                       int_stream_id_base, int_stream_id, pp_port_index));
            return BCM_E_PARAM;
        }

        if (!_BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_GET(unit,
                dev_int_stream_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY subport port gport 0x%x is not found.\n"
                                  "Device internal stream_id = %d is not set for port %d\n"),
                       subport_port_gport, dev_int_stream_id, port));
            return BCM_E_NOT_FOUND;
        }

        /* RXLP */
        disable = 0;
        rv = _bcm_linkphy_port_rx_flow_control_set(unit, port,
                                                       int_stream_id, disable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: RX flow control disable failed\n"
                                   " for, internal stream_id = %d.\n"), int_stream_id));
            return rv;
        }

        reg = RXLP_SW_FLUSH_CONTROLr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {

            timeoutval = _BCM_KT2_LINKPHY_STREAM_ID_FLUSH_TIMEOUT;
            soc_timeout_init(&timeout, timeoutval, 0);

            if (!(SAL_BOOT_SIMULATION)) {
                BCM_IF_ERROR_RETURN(soc_reg_field32_modify(unit, reg,
                            port, FLUSH_STREAM_IDf, int_stream_id));
                BCM_IF_ERROR_RETURN(soc_reg_field32_modify(unit, reg,
                            port, FLUSH_ACTIVEf, 1));

                poll_active = 1;

                while (poll_active) {

                    if (soc_timeout_check(&timeout)) {
                        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                                  (BSL_META_U(unit,
                                              "ERROR: RX stream id %d flush timeout :"
                                               "subport_port gport 0x%x, port = %d, pp_port %d\n"),
                                   int_stream_id, subport_port_gport,
                                   port, pp_port_index));
                        return BCM_E_TIMEOUT;
                    }

                    BCM_IF_ERROR_RETURN(
                        READ_RXLP_SW_FLUSH_CONTROLr(unit, port, &regval));

                    poll_active = (soc_reg_field_get(unit, reg, regval,
                                       FLUSH_DONEf)) ? 0 :1;

                }
            }
        }

        rv = _bcm_linkphy_port_rx_active_stream_bitmap_set(unit,
                port, int_stream_id, disable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: RX active stream disable failed\n"
                                   " port %d, internal stream_id = %d.\n"), port, int_stream_id));
            return rv;
        }

        rv = _bcm_linkphy_port_rx_int_stream_map_port_set(unit,
                port, int_stream_id, disable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: RX stream map for port clear failed.\n"
                                   "port %d, internal stream_id = %d.\n"), port, int_stream_id));
            return rv;
        }

        /* TXLP */

        
        rv = _bcm_linkphy_port_tx_active_stream_set(unit,
                port, int_stream_id, disable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: TX active stream bitmap disable failed.\n"
                                   "port %d, internal stream_id = %d.\n"), port, int_stream_id));
            return rv;
        }

        rv = _bcm_linkphy_port_tx_int_to_ext_stream_map_set(unit, 
                port, 0, int_stream_id, disable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: TX internal to external stream mapp clear failed.\n"
                                  "port %d, internal stream_id = %d, external stream id = %d.\n"),
                       port, int_stream_id, 0));
            return rv;
        }

        rv = _bcm_linkphy_port_tx_stream_addr_map_set(unit,
                        port, int_stream_id, 0, 0, disable);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: TX data buffer address clear failed.\n"
                                   "port %d, internal stream_id = %d, start_addr = %d "
                                   "end_addr = %d.\n"), port, int_stream_id, 0, 0));
            return rv;
        }

        /* 
         * Clean Device Internal Stream ID to pp_port mapping
         */
        mem = DEVICE_STREAM_ID_TO_PP_PORT_MAPm;
        rv = soc_mem_field32_modify(unit, mem, dev_int_stream_id, PP_PORTf, 0);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: cleaning device internal stream id  %d to pp_port "
                                  "map failed.\n"),
                       dev_int_stream_id));
            return rv;
        }
        /* Clean PHB for the subport */
        BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
            unit, mem, dev_int_stream_id, PHB_VALIDf, 0));
        BCM_IF_ERROR_RETURN(
            soc_mem_field32_modify(unit, mem, dev_int_stream_id, INT_PRIf, 0));
        BCM_IF_ERROR_RETURN(
            soc_mem_field32_modify(unit, mem, dev_int_stream_id, CNGf, 0));

        _BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_CLR(unit, dev_int_stream_id);

        _bcm_linkphy_subport_port_info[unit]
            [pp_port_index].ext_stream_idx[i] = 0;
    }

    /*
     * clean PP_PORT to Physical port mapping
     */
    mem = PP_PORT_TO_PHYSICAL_PORT_MAPm;
    rv = soc_mem_field32_modify(unit,mem ,pp_port_index, DESTINATIONf, 0);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Cleaning pp_port id %d to physical port "
                               "map failed.\n"), pp_port_index));
        return rv;
    }

    _BCM_KT2_SUBPORT_PP_PORT_USED_CLR(unit, pp_port_index);
    _bcm_subport_pp_port_count[unit]--;

    *(_bcm_subport_group_subport_port_count[unit] + group_id) -= 1;

    _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_SET(unit, pp_port_index,
                                           BCM_GPORT_INVALID, -1, FALSE); 

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return rv;
}

/*
 * Function:
 *      _bcm_linkphy_subport_port_get
 * Purpose:
 *      Get the LinkPHY subport_port config (subport group gport and stream_id)
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) Subport_port GPORT (generic port) identifier
 *      config - (OUT) Subport port config
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_port_get(int unit,
                              bcm_gport_t subport_port_gport,
                              bcm_subport_config_t *config)
{
    int i = 0, port = 0, pp_port = 0;
    int rv = BCM_E_NONE;
    bcm_gport_t subport_group;
    uint16 int_stream_id = 0, int_stream_id_base = 0, dev_int_stream_id = 0;
    uint16 ext_stream_id_base = 0;
    device_stream_id_to_pp_port_map_entry_t dev_sid_to_pp_entry;
    soc_mem_t mem;

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    pp_port = _BCM_KT2_SUBPORT_PORT_ID_GET(subport_port_gport);

    subport_group =
        _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port);

    if (!BCM_GPORT_IS_SUBPORT_GROUP(subport_group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR:Invalid LinkPHY subport port gport 0x%x\n"),
                   subport_port_gport));
        return BCM_E_PARAM;
    }

    if (!_BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: LinkPHY subport port gport 0x%x is invalid\n"),
                   subport_port_gport));
        return BCM_E_NOT_FOUND;
    }

    config->group = subport_group;

    config->num_streams =
        _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_NUM_STREAMS_GET(unit, pp_port);

    port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(config->group);

    rv = _bcm_linkphy_port_int_stream_id_base_get(unit, port,
            &int_stream_id_base);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy subport group internal stream id "
                               "base get failed for port %d.\n"), port));
        return rv;
    }
    
    rv = _bcm_linkphy_port_ext_stream_id_base_get(unit, port,
               &ext_stream_id_base);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Linkphy subport group external stream id "
                               "base get failed for port %d.\n"), port));
        return rv;
    }

    for (i = 0; i < config->num_streams; i++) {
        int_stream_id =
            _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_EXT_STREAM_ID_GET(
                    unit, pp_port, i) - ext_stream_id_base;

        dev_int_stream_id = int_stream_id + int_stream_id_base;
        if (dev_int_stream_id > soc_mem_index_max(unit,
                                    DEVICE_STREAM_ID_TO_PP_PORT_MAPm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: device internal stream_id %d is out of range \n"
                                  "port %d, internal stream id base %d\n"),
                       dev_int_stream_id, port, int_stream_id_base));
            return BCM_E_PARAM;
        }

        if (!_BCM_KT2_LINKPHY_DEV_INT_STREAM_ID_USED_GET(unit,
                dev_int_stream_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY subport port gport 0x%x is not found.\n"
                                  "Device internal stream_id = %d is not set for port %d\n"),
                       subport_port_gport, dev_int_stream_id, port));
            return BCM_E_NOT_FOUND;
        }

        config->stream_id_array[i] =
            _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_EXT_STREAM_ID_GET(
                unit, pp_port, i);
    }

    mem = DEVICE_STREAM_ID_TO_PP_PORT_MAPm;
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, dev_int_stream_id,
            &dev_sid_to_pp_entry);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: DEVICE_STREAM_ID_TO_PP_PORT_MAPm read "
                               "failed.for index %d\n"), dev_int_stream_id));
        return rv;
    }

    if (soc_mem_field32_get(unit, mem,
            (uint32 *)&dev_sid_to_pp_entry, PHB_VALIDf)) {
        config->prop_flags |= BCM_SUBPORT_PROPERTY_PHB;
        config->int_pri = soc_mem_field32_get(unit, mem,
            (uint32 *)&dev_sid_to_pp_entry, INT_PRIf);
        config->color = soc_mem_field32_get(unit, mem,
            (uint32 *)&dev_sid_to_pp_entry, CNGf);
    } else {
        config->prop_flags &= ~BCM_SUBPORT_PROPERTY_PHB;
        config->int_pri = 0;
        config->color = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_port_traverse
 * Purpose:
 *      Add all valid LinkPHY subports under all valid LinkPHY subport groups
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_port_traverse(int unit,
                                   bcm_subport_port_traverse_cb cb,
                                   void *user_data)
{
    int rv = BCM_E_NONE;
    int pp_port_id = 0;
    bcm_subport_config_t config;
    bcm_gport_t linkphy_subport_port;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    BCM_PBMP_ITER(SOC_INFO(unit).linkphy_pp_port_pbm, pp_port_id) {

        if (!_BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port_id)) {
            continue;
        }

        bcm_subport_config_t_init(&config);

        _BCM_KT2_SUBPORT_PORT_ID_SET(linkphy_subport_port, pp_port_id);

        _BCM_KT2_SUBPORT_PORT_TYPE_SET(linkphy_subport_port,
                                   _BCM_KT2_SUBPORT_TYPE_LINKPHY);

        rv = _bcm_linkphy_subport_port_get(unit, linkphy_subport_port, &config);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport port traverse failed \n"
                                   "to get LinkPHY subport id %d\n"), pp_port_id));
            return rv;
        }

        rv = cb(unit, linkphy_subport_port, &config, user_data);
        COMPILER_REFERENCE(rv);
#ifdef BCM_CB_ABORT_ON_ERR
        if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport port traverse failed \n"
                                   "to callback for subport port (pp_port id %d)\n"), pp_port_id));
            return rv;
        }
#endif
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_linkphy_subport_cleanup
 * Purpose:
 *      Delate all valid LinkPHY subport ports and
 *      all valid LinkPHY subport groups.
 * Parameters:
 *      unit      - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_linkphy_subport_cleanup(int unit)
{
    int group_id = 0, rv = BCM_E_NONE, pp_port_id = 0;
    bcm_gport_t subport_group;
    bcm_gport_t subport_port;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    /* delete all LinkPHY subport ports */
    BCM_PBMP_ITER(SOC_INFO(unit).linkphy_pp_port_pbm, pp_port_id) {
        
        if (!_BCM_KT2_SUBPORT_PP_PORT_USED_GET(unit, pp_port_id)) {
            continue;
        }

        _BCM_KT2_SUBPORT_PORT_ID_SET(subport_port, pp_port_id);

        _BCM_KT2_SUBPORT_PORT_TYPE_SET(subport_port,
                                   _BCM_KT2_SUBPORT_TYPE_LINKPHY);

        rv = _bcm_linkphy_subport_port_delete(unit, subport_port);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY subport port delete failed (subport id %d)\n"),
                       pp_port_id));
            return rv;
        }

        subport_group =
            _BCM_KT2_LINKPHY_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port_id);

        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(subport_group);

        if (*(_bcm_subport_group_subport_port_count[unit] + group_id) == 0) {
             /* delete the subport group */
            _bcm_subport_group_count[unit]--;
            _BCM_KT2_SUBPORT_GROUP_USED_CLR(unit, group_id);
            _bcm_linkphy_subport_group_count[unit]--;
            _BCM_KT2_LINKPHY_SUBPORT_GROUP_USED_CLR(unit, group_id);
        }
    }

    return BCM_E_NONE;
}


/*********************************
 * LinkPHY helper functions -end *
 ********************************/


/**********************************
 * SubTag helper functions -start *
 *********************************/

/*
 * Function:
 *      _bcm_subtag_subport_free_entry_find
 * Purpose:
 *      Search for unused entry in the SUBPORT_TAG_TO_PP_PORT_MAP table
 * Parameters:
 *      unit - Device unit number
 *      index - (OUT) index where found
 * Returns:
 *      BOC_E_NONE      - free entry found
 *      BOC_E_FULL      - no free entries found
 */
STATIC int 
_bcm_subtag_subport_free_entry_find(int unit, int *free_index)
{
    subport_tag_to_pp_port_map_entry_t entry;
    int index, rv, start, end;
    soc_mem_t mem = SUBPORT_TAG_TO_PP_PORT_MAPm;

    start = soc_mem_index_min(unit, mem);
    end = soc_mem_index_max(unit, mem) + 1;

    for (index = start; index != end; index++) {
        rv = READ_SUBPORT_TAG_TO_PP_PORT_MAPm(unit,
                 MEM_BLOCK_ANY, index, &entry);
        if (SOC_SUCCESS(rv)) {
            if (!soc_SUBPORT_TAG_TO_PP_PORT_MAPm_field32_get(unit,
                                                 (uint32 *)&entry, VALIDf)) {
                *free_index = index;
                return BCM_E_NONE;
            }
        }
    }

    return BCM_E_FULL;
}


/*
 * Function:
 *      _bcm_subtag_subport_init
 * Purpose:
 *      Initialize SubTag subport module.
 * Parameters:
 *      unit      - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_init(int unit)
{
    bcm_port_t port;
    int num_vlan = 0, index = 0;
    soc_info_t *si = &SOC_INFO(unit);

    /* Clear SUBPORT_TAG_TO_PP_PORT_MAP table */
    BCM_IF_ERROR_RETURN
        (soc_mem_clear(unit, SUBPORT_TAG_TO_PP_PORT_MAPm, COPYNO_ALL, TRUE));

    num_vlan = soc_mem_index_count(unit, VLAN_TABm);

    BCM_PBMP_ITER(si->subtag_allowed_pbm, port) {

        /* check to fix coverity static overrun error */
        if (port < SOC_MAX_NUM_PORTS) {

            /* SubTag VLAN ID bitmap to keep track of the VLAN ID used per 
             * SubTag enabled port.
             */
            if (_bcm_subtag_vlan_id_bitmap[unit][port] == NULL) {

                _bcm_subtag_vlan_id_bitmap[unit][port] =
                    sal_alloc(SHR_BITALLOCSIZE(num_vlan),
                            "_bcm_subtag_vlan_id_bitmap");

                if (_bcm_subtag_vlan_id_bitmap[unit][port] == NULL) {
                    LOG_ERROR(BSL_LS_BCM_SUBPORT,
                              (BSL_META_U(unit,
                                          "ERROR:_bcm_subtag_vlan_id_bitmap "
                                           " alloc failed\n")));

                    return BCM_E_MEMORY;
                }
            }
            sal_memset(_bcm_subtag_vlan_id_bitmap[unit][port], 0,
                    SHR_BITALLOCSIZE(num_vlan));
        }
    }


    /* SubTag subport group bitmap to keep track of used SubTag groups */
    if (_bcm_subtag_group_bitmap[unit] == NULL) {

        _bcm_subtag_group_bitmap[unit] = 
             sal_alloc(SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_GROUP_MAX),
             "subtag_subport_group_bitmap");

        if (_bcm_subtag_group_bitmap[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:subtag group bitmap alloc failed\n")));

             return BCM_E_MEMORY;
        }
    }

    sal_memset(_bcm_subtag_group_bitmap[unit], 0,
        SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_GROUP_MAX));


    /* SubTag subport port info per subport group per subport port */
    if (_bcm_subtag_subport_port_info[unit] == NULL) {
        _bcm_subtag_subport_port_info[unit] =
            sal_alloc((SOC_MAX_NUM_PP_PORTS *
                       sizeof(_bcm_subtag_subport_port_info_t)),
                       "subtag_subport_port_info");
        if (_bcm_subtag_subport_port_info[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: "
                                   " subtag_subport_port_info alloc failed\n")));

            return BCM_E_MEMORY;
        }
    }

    sal_memset(_bcm_subtag_subport_port_info[unit], 0,
        (SOC_MAX_NUM_PP_PORTS * sizeof(_bcm_subtag_subport_port_info_t)));

    for (index = 0; index < SOC_MAX_NUM_PP_PORTS; index++) {
        _bcm_subtag_subport_port_info[unit][index].group = BCM_GPORT_INVALID;
        _bcm_subtag_subport_port_info[unit][index].vlan = BCM_VLAN_INVALID;
        _bcm_subtag_subport_port_info[unit][index].subtag_tcam_hw_idx = -1;
    }

    /* Set the STP state of subports */
    BCM_PBMP_ITER(SOC_INFO(unit).subtag_pp_port_pbm, port) {
        bcm_esw_port_learn_set(unit, port,
                BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD);
        bcm_esw_port_stp_set(unit, port, BCM_STG_STP_FORWARD);
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_subtag_subport_group_create
 * Purpose:
 *      Create a SubTag subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport group config information
 *      group  - (OUT) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_group_create(int unit,
                                 int is_trunk, bcm_port_t port,
                                 int group_id, bcm_gport_t *group)
{
    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    _BCM_KT2_SUBTAG_SUBPORT_GROUP_SET(*group, port, group_id);
    _BCM_KT2_SUBPORT_GROUP_TYPE_CASCADED_SET(*group);

    _BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_SET(unit, group_id);
    _bcm_subtag_subport_group_count[unit]++;

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_subtag_subport_group_destroy
 * Purpose:
 *      Destroy the SubTag subport group and all subport ports attached to it.
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier for subport group
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_subtag_subport_group_destroy(int unit,
                                  bcm_gport_t group)
{
    int group_id = -1;
    int pp_port = 0, rv = BCM_E_NONE;
    int subtag_subport_count = 0;
    bcm_gport_t subport_port_gport;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group)) {

        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(group);

        if (!_BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag subport group 0x%x not found.\n"),
                       group));
            return BCM_E_NOT_FOUND;
        }

        subtag_subport_count =
            *(_bcm_subport_group_subport_port_count[unit] + group_id);

        /* delete all subport ports attached to the subport group */
        BCM_PBMP_ITER(SOC_INFO(unit).subtag_pp_port_pbm, pp_port) {
            if (subtag_subport_count == 0) {
                break;
            }

            if (!_BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port)) {
                continue;
            }

            if (group !=
                _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port)) {
                continue;
            }

            _BCM_KT2_SUBPORT_PORT_ID_SET(subport_port_gport, pp_port);

            _BCM_KT2_SUBPORT_PORT_TYPE_SET(subport_port_gport,
                                   _BCM_KT2_SUBPORT_TYPE_SUBTAG);

            rv = _bcm_subtag_subport_port_delete(unit,subport_port_gport);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: SubTag subport group 0x%x destroy failed \n"
                                      " to delete subport port (subport group id %d, "
                                      " subport id %d)\n"),
                           group, group_id, pp_port));
                return rv;
            }
            subtag_subport_count--;
        }

        *(_bcm_subport_group_subport_port_count[unit] + group_id) = 0;
        /* delete the subport group */
        _bcm_subport_group_count[unit]--;
        _BCM_KT2_SUBPORT_GROUP_USED_CLR(unit, group_id);
        _bcm_subtag_subport_group_count[unit]--;
        _BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_CLR(unit, group_id);
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_subtag_subport_group_get
 * Purpose:
 *      Get a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier
 *      config - (OUT) Subport group config information
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_group_get(int unit,
                              bcm_gport_t group,
                              bcm_subport_group_config_t *config)
{
    int group_id = -1;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group)) {
        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(group);
        if (!_BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag subport group 0x%x not found.\n"),
                       group));
            return BCM_E_NOT_FOUND;;
        }
        config->flags |= BCM_SUBPORT_GROUP_TYPE_SUBPORT_TAG;
        config->port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(group);
    } else {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_subtag_subport_group_traverse
 * Purpose:
 *      Traverse all valid subports for given SubTag subport group
 *      and call the user supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      group     - Subport group GPORT identifier
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 */

STATIC int
_bcm_subtag_subport_group_traverse(int unit,
                                   bcm_gport_t group,
                                   bcm_subport_port_traverse_cb cb,
                                   void *user_data)
{
    int rv = BCM_E_NONE, group_id = 0;
    int pp_port = 0, subport_port;
    int subtag_subport_count = 0;
    bcm_subport_config_t config;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    bcm_subport_config_t_init(&config);

    if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group)) {
        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(group);

        if (!_BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag subport group 0x%x not found.\n"),
                       group));
            return BCM_E_NOT_FOUND;
        }

        subtag_subport_count =
            *(_bcm_subport_group_subport_port_count[unit] + group_id);

        
        BCM_PBMP_ITER(SOC_INFO(unit).subtag_pp_port_pbm, pp_port) {

            if (subtag_subport_count == 0) {
               break;
            }

            if (!_BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port)) {
               continue;
            }

            if (group !=
               _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port)) {
               continue;
            }

            _BCM_KT2_SUBPORT_PORT_ID_SET(subport_port, pp_port);

            _BCM_KT2_SUBPORT_PORT_TYPE_SET(subport_port,
                                   _BCM_KT2_SUBPORT_TYPE_SUBTAG);

            rv = _bcm_subtag_subport_port_get(unit, subport_port, &config);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: SubTag subport group 0x%x traverse failed \n"
                                      "to get subport port (group id %d, subport id %d)\n"),
                           group, group_id, pp_port));
                return rv;
            }

            rv = cb(unit, subport_port, &config, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: subport group 0x%x traverse failed \n"
                                       "to callback for subport port (group id %d, "
                                       "subport id %d)\n"), group, group_id, pp_port));
                return rv;
            }
#endif
            subtag_subport_count--;
        }
    } else {
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_subtag_subport_port_add
 * Purpose:
 *      Add a SubTag subport to a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport config information
 *      port   - (OUT) Subport_port GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_port_add(int unit,
                             bcm_subport_config_t *config,
                             bcm_gport_t *gport)
{
    int rv = BCM_E_NONE, port = -1;
    int subtag_index = -1;
    int sp_group_idx = -1, i = 0;
    uint16 vlan_id = 0;
    uint32 pp_port_index = 0;
    soc_mem_t mem;
    soc_info_t *si = &SOC_INFO(unit);
    subport_tag_to_pp_port_map_entry_t    subtag_to_pp_entry;
    pp_port_to_physical_port_map_entry_t  pp_to_port_entry;

    /* A. PARAMETER VALIDATION
    *******************************/
    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_GROUP(config->group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: group 0x%x is not subport group gport\n"),
                   config->group));
        return BCM_E_PARAM;
    }

    /* Make sure the group exists */
    if (!_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(config->group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: subport group 0x%x is not SubTag group\n"),
                   config->group));
        return BCM_E_PARAM;
    }

    sp_group_idx = _BCM_KT2_SUBPORT_GROUP_ID_GET(config->group);

    if (!_BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_GET(unit, sp_group_idx)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: group 0x%x does not exist. group id =%d\n"),
                   config->group, sp_group_idx));
        return BCM_E_NOT_FOUND;
    }

    port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(config->group);

    /* Check if port is member of pbmp_subtag  */
    if (!SOC_PBMP_MEMBER(si->subtag_pbm, port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Port %d is not member of pbmp_subport\n"),
                   port));
        return BCM_E_PORT;
    }

    if (si->port_num_subport[port] <= 0) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: %d number of subports available for port %d\n"),
                   si->port_num_subport[port], port));
        return BCM_E_CONFIG;
    }

    if ((config->pkt_vlan & 0xFFF) <= BCM_VLAN_NONE) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: (config->pkt_vlan & 0xFFF) %d is invalid\n"),
                   (config->pkt_vlan & 0xFFF)));
        return BCM_E_PARAM;
    }

    if (config->prop_flags & BCM_SUBPORT_PROPERTY_PHB) {
        if (config->int_pri < 0 || config->int_pri > 15) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: int_pri %d is invalid\n"),
                       config->int_pri));
            return BCM_E_PARAM;
        }
        if ((config->color != bcmColorGreen) &&
             (config->color != bcmColorYellow) &&
             (config->color != bcmColorRed)) {
             LOG_ERROR(BSL_LS_BCM_SUBPORT,
                       (BSL_META_U(unit,
                                   "ERROR: color %d is invalid\n"),
                        config->color));
            return BCM_E_PARAM;
        }
    }

    /* Check if VLAN id is alredy used for the port */
    vlan_id = config->pkt_vlan & 0xFFF; 

    if (_BCM_KT2_SUBTAG_VLAN_ID_USED_GET(unit, port, vlan_id)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: VLAN id %d already used for port %d\n"),
                   vlan_id, port));
        return BCM_E_EXISTS;
    }

    /* B. find free entry in subtag to pp_port Mapping table
    **********************************************************/

    mem = SUBPORT_TAG_TO_PP_PORT_MAPm;

    rv = _bcm_subtag_subport_free_entry_find(unit, &subtag_index);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Free entry not found in "
                               " subtag to pp_port map table\n")));
        return rv;
    }

    /* C. PP_PORT INDEX RESERVATION
    *************************************/
    /* Reserve free PP_PORT index */

    for (i = si->port_subport_base[port];
         i < si->port_subport_base[port] + si->port_num_subport[port]; i++) {
        if (!_BCM_KT2_SUBPORT_PP_PORT_USED_GET(unit, i)) {
            pp_port_index = i;
            break;
        }
    }

    if (i == (si->port_subport_base[port] + si->port_num_subport[port])) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: All %d subport pp_port already used.\n"),
                   (i - si->port_subport_base[port])));
        return BCM_E_FULL;
    }


    /* D. Map VLAN ID, port to pp_port_index
    ***********************************************/
    /* Use the SUBPORT_TAG_TO_PP_PORT_MAPm table */

    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, subtag_index,
            &subtag_to_pp_entry);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "SUBPORT_TAG_TO_PP_PORT_MAPm read failed at index %d\n"),
                   subtag_index));

        return rv;
    }

    /* Set the SUBPORT_TAG, PHYSICAL_PORT and VALID fields */
    soc_SUBPORT_TAG_TO_PP_PORT_MAPm_field32_set(unit,
        (uint32 *)&subtag_to_pp_entry, SUBPORT_TAGf, config->pkt_vlan);
    soc_SUBPORT_TAG_TO_PP_PORT_MAPm_field32_set(unit,
        (uint32 *)&subtag_to_pp_entry, PHYSICAL_PORTf, port);
    soc_SUBPORT_TAG_TO_PP_PORT_MAPm_field32_set(unit,
        (uint32 *)&subtag_to_pp_entry, VALIDf, 1);

    soc_SUBPORT_TAG_TO_PP_PORT_MAPm_field32_set(unit,
        (uint32 *)&subtag_to_pp_entry, SUBPORT_TAG_MASKf, 0xFFF);
    soc_SUBPORT_TAG_TO_PP_PORT_MAPm_field32_set(unit,
        (uint32 *)&subtag_to_pp_entry, PHYSICAL_PORT_MASKf,
        ((1 << soc_mem_field_length(unit, mem, PHYSICAL_PORT_MASKf)) - 1));

    /* Set the PP_PORT field */
    soc_SUBPORT_TAG_TO_PP_PORT_MAPm_field32_set(unit,
        (uint32 *)&subtag_to_pp_entry, PP_PORTf, pp_port_index);

    /* Set PHB for the subport */
    if (config->prop_flags & BCM_SUBPORT_PROPERTY_PHB) {
        soc_mem_field32_set(unit, mem, (uint32 *)&subtag_to_pp_entry,
            PHB_VALIDf, 1);
        soc_mem_field32_set(unit, mem, (uint32 *)&subtag_to_pp_entry,
            INT_PRIf, config->int_pri);
        soc_mem_field32_set(unit, mem, (uint32 *)&subtag_to_pp_entry,
            CNGf, config->color);
    }

    rv = WRITE_SUBPORT_TAG_TO_PP_PORT_MAPm(unit, MEM_BLOCK_ANY, subtag_index,
            &subtag_to_pp_entry);

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: (VLAN id %d, port %d) to pp_port %d "
                               "map failed.\n"), vlan_id, port, pp_port_index));

        return rv;
    }

    /* E. Map PP_PORT to Physical port mapping (IP/ EP):
    *****************************************************/
    /* Use the PP_PORT_TO_PHYSICAL_PORT_MAP */
    mem = PP_PORT_TO_PHYSICAL_PORT_MAPm;
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, pp_port_index,
            &pp_to_port_entry);

    soc_mem_field32_set(unit, mem, (uint32 *)&pp_to_port_entry,
        DESTINATIONf, port);
    soc_mem_field32_set(unit, mem, (uint32 *)&pp_to_port_entry,
        ISTRUNKf, 0);

    rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, pp_port_index,
            &pp_to_port_entry);

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: pp_port id %d to physical port %d "
                               "map failed.\n"), pp_port_index, port));

        /* free/invalidate the SUBTAG_TO_PP_PORT_MAP table entry */
       BCM_IF_ERROR_RETURN(soc_mem_field32_modify(unit,
                   SUBPORT_TAG_TO_PP_PORT_MAPm, subtag_index, VALIDf, 0));
        return rv;
    }

    _BCM_KT2_SUBTAG_VLAN_ID_USED_SET(unit, port, vlan_id);

    _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_SET(unit, pp_port_index,
        config->group, config->pkt_vlan, subtag_index, TRUE);

    _BCM_KT2_SUBPORT_PORT_ID_SET(*gport, pp_port_index);
    
    _BCM_KT2_SUBPORT_PORT_TYPE_SET(*gport, _BCM_KT2_SUBPORT_TYPE_SUBTAG);

    *(_bcm_subport_group_subport_port_count[unit] + sp_group_idx) += 1;

    _BCM_KT2_SUBPORT_PP_PORT_USED_SET(unit, pp_port_index);
    _bcm_subport_pp_port_count[unit]++;

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    return rv;
}


/*
 * Function:
 *      _bcm_subtag_subport_port_delete
 * Purpose:
 *      Delete a SubTag subport_port
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) Subport_port GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_port_delete(int unit,
                                bcm_gport_t subport_port_gport)
{
    int rv = BCM_E_NONE, group_id = 0;
    int port = 0, subport_group = -1;
    int subtag_to_pp_port_tab_idx = -1;
    uint16 pp_port_index = 0;
    bcm_vlan_t vlan_id;
    soc_mem_t mem;
    subport_tag_to_pp_port_map_entry_t subport_tag_to_pp_port_map_entry;

    if (!_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, subport_port_gport)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: gport 0x%x is not SubTag subport port type.\n"),
                   subport_port_gport));
        return BCM_E_PARAM;
    }

    pp_port_index =
        _BCM_KT2_SUBPORT_PORT_ID_GET(subport_port_gport);

    if (!_BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port_index)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: SubTag subport port gport 0x%x is invalid\n"),
                   subport_port_gport));
        return BCM_E_NOT_FOUND;
    }

    subport_group =
        _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port_index);

    port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(subport_group);

    subtag_to_pp_port_tab_idx =
        _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_TCAM_HW_ID_GET(unit, pp_port_index);

    mem = SUBPORT_TAG_TO_PP_PORT_MAPm;
    if (subtag_to_pp_port_tab_idx < 0 ||
        subtag_to_pp_port_tab_idx > soc_mem_index_max(unit,mem)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: SubTag subport port gport 0x%x is not found.\n"
                               "subtag_to_pp_port  tcam idx = %d\n"), subport_port_gport,
                   subtag_to_pp_port_tab_idx));
        return BCM_E_NOT_FOUND;
    }

    vlan_id = _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VLAN_GET(unit, pp_port_index) &
              0xFFF;

    if (!_BCM_KT2_SUBTAG_VLAN_ID_USED_GET(unit, port, vlan_id)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: SubTag subport port gport 0x%x is not found.\n"
                              "VLAN id = %d is not set for port %d\n"),
                   subport_port_gport, vlan_id, port));
        return BCM_E_NOT_FOUND;
    }

    

    /* 
    * Clean port, VLAN ID to pp_port mapping
    */
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, subtag_to_pp_port_tab_idx,
             &subport_tag_to_pp_port_map_entry);
    soc_mem_field32_set(unit, mem, (uint32 *)&subport_tag_to_pp_port_map_entry,
        SUBPORT_TAGf, 0);
    soc_mem_field32_set(unit, mem, (uint32 *)&subport_tag_to_pp_port_map_entry,
        PP_PORTf, 0);
    soc_mem_field32_set(unit, mem, (uint32 *)&subport_tag_to_pp_port_map_entry,
        VALIDf, 0);
    /* Clean PHB for the subport */
    soc_mem_field32_set(unit, mem, (uint32 *)&subport_tag_to_pp_port_map_entry,
        PHB_VALIDf, 0);
    soc_mem_field32_set(unit, mem, (uint32 *)&subport_tag_to_pp_port_map_entry,
        INT_PRIf, 0);
    soc_mem_field32_set(unit, mem, (uint32 *)&subport_tag_to_pp_port_map_entry,
        CNGf, 0);
    rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, subtag_to_pp_port_tab_idx,
            &subport_tag_to_pp_port_map_entry);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: subport_tag %d to pp_port %d "
                               "map failed.\n"), vlan_id, pp_port_index));
        return rv;
    }

    /*
     * clean PP_PORT to Physical port mapping
     */
    mem = PP_PORT_TO_PHYSICAL_PORT_MAPm;
    rv = soc_mem_field32_modify(unit,mem ,pp_port_index, DESTINATIONf, 0);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Cleaning pp_port id %d to physical port "
                               "map failed.\n"), pp_port_index));
        return rv;
    }

    _BCM_KT2_SUBTAG_VLAN_ID_USED_CLR(unit, port, vlan_id);

    _BCM_KT2_SUBPORT_PP_PORT_USED_CLR(unit, pp_port_index);
    _bcm_subport_pp_port_count[unit]--;

    *(_bcm_subport_group_subport_port_count[unit] + group_id) -= 1;

    _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_SET(unit, pp_port_index,
                              BCM_GPORT_INVALID, BCM_VLAN_INVALID, -1, FALSE);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return rv;
}


/*
 * Function:
 *      _bcm_subtag_subport_port_get
 * Purpose:
 *      Get the SubTag subport_port config (subport group gport and stream_id)
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) Subport_port GPORT (generic port) identifier
 *      config - (OUT) Subport port config
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_port_get(int unit,
                             bcm_gport_t subport_port_gport,
                             bcm_subport_config_t *config)
{
    int pp_port = 0;
    int rv = BCM_E_NONE;
    bcm_gport_t subport_group;
    int subtag_to_pp_hw_id = -1;
    subport_tag_to_pp_port_map_entry_t subtag_pp_entry;
    soc_mem_t mem;

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, subport_port_gport)) {

        pp_port = _BCM_KT2_SUBPORT_PORT_ID_GET(subport_port_gport);

        subport_group =
            _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port);

        if (!BCM_GPORT_IS_SUBPORT_GROUP(subport_group)) {
            LOG_VERBOSE(BSL_LS_BCM_SUBPORT,
                        (BSL_META_U(unit,
                                    "ERROR:Invalid SubTag subport port gport 0x%x\n"),
                         subport_port_gport));
            return BCM_E_PARAM;
        }

        if (!_BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port)) {
            LOG_VERBOSE(BSL_LS_BCM_SUBPORT,
                        (BSL_META_U(unit,
                                    "ERROR: SubTag subport port gport 0x%x not found\n"),
                         subport_port_gport));
            return BCM_E_NOT_FOUND;
        }

        config->group = subport_group;

        config->pkt_vlan =
            _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VLAN_GET(unit, pp_port);

        subtag_to_pp_hw_id =
            _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_TCAM_HW_ID_GET(unit, pp_port);

        mem = SUBPORT_TAG_TO_PP_PORT_MAPm;
        if ((subtag_to_pp_hw_id < 0) ||
            (subtag_to_pp_hw_id > soc_mem_index_max(unit, mem))) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag subport port gport 0x%x is not found.\n"
                                   "subtag_to_pp_port_tab idx = %d\n"), subport_port_gport,
                       subtag_to_pp_hw_id));
            return BCM_E_NOT_FOUND;
        }

        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, subtag_to_pp_hw_id,
                &subtag_pp_entry);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: DEVICE_STREAM_ID_TO_PP_PORT_MAPm read "
                                   "failed.for index %d\n"), subtag_to_pp_hw_id));
            return rv;
        }

        if (soc_mem_field32_get(unit, mem,
                (uint32 *)&subtag_pp_entry, PHB_VALIDf)) {
            config->prop_flags |= BCM_SUBPORT_PROPERTY_PHB;
            config->int_pri = soc_mem_field32_get(unit, mem,
                (uint32 *)&subtag_pp_entry, INT_PRIf);
            config->color = soc_mem_field32_get(unit, mem,
                (uint32 *)&subtag_pp_entry, CNGf);
        } else {
            config->prop_flags &= ~BCM_SUBPORT_PROPERTY_PHB;
            config->int_pri = 0;
            config->color = 0;
        }
    } else {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_subtag_subport_port_traverse
 * Purpose:
 *      Add all valid SubTag subports under all valid SubTag subport groups
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_port_traverse(int unit,
                                  bcm_subport_port_traverse_cb cb,
                                  void *user_data)
{
    int rv = BCM_E_NONE;
    int pp_port_id = 0;
    bcm_subport_config_t config;
    bcm_gport_t subtag_subport_port;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    BCM_PBMP_ITER(SOC_INFO(unit).subtag_pp_port_pbm, pp_port_id) {

        if (!_BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_VALID_GET(unit, pp_port_id)) {
            continue;
        }

        bcm_subport_config_t_init(&config);

        _BCM_KT2_SUBPORT_PORT_ID_SET(subtag_subport_port, pp_port_id);
        
        _BCM_KT2_SUBPORT_PORT_TYPE_SET(subtag_subport_port,
                                          _BCM_KT2_SUBPORT_TYPE_SUBTAG);

        rv = _bcm_subtag_subport_port_get(unit, subtag_subport_port, &config);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport port traverse failed \n"
                                   "to get SubTag subport id %d\n"), pp_port_id));
            return rv;
        }

        rv = cb(unit, subtag_subport_port, &config, user_data);
        COMPILER_REFERENCE(rv);
#ifdef BCM_CB_ABORT_ON_ERR
        if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport port traverse failed \n"
                                   "to callback for subport port (subport_port id %d)\n"),
                       pp_port_id));
            return rv;
        }
#endif
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_subtag_subport_cleanup
 * Purpose:
 *      Delate all valid SubTag subport ports and
 *      all valid SubTag subport groups.
 * Parameters:
 *      unit      - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_subtag_subport_cleanup(int unit)
{
    int group_id = 0, rv = BCM_E_NONE, pp_port_id = 0;
    bcm_port_t port;
    bcm_gport_t subport_group;
    bcm_gport_t subport_port;
    soc_mem_t mem;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

   /* delete all SubTag subport ports */
    BCM_PBMP_ITER(SOC_INFO(unit).subtag_pp_port_pbm, pp_port_id) {
        
        if (!_BCM_KT2_SUBPORT_PP_PORT_USED_GET(unit, pp_port_id)) {
            continue;
        }

        _BCM_KT2_SUBPORT_PORT_ID_SET(subport_port, pp_port_id);

        _BCM_KT2_SUBPORT_PORT_TYPE_SET(subport_port,
                                          _BCM_KT2_SUBPORT_TYPE_SUBTAG);

        rv = _bcm_subtag_subport_port_delete(unit, subport_port);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag subport port delete failed (subport id %d)\n"),
                       pp_port_id));
            return rv;
        }

        subport_group =
            _BCM_KT2_SUBTAG_SUBPORT_PORT_INFO_GROUP_GET(unit, pp_port_id);

        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(subport_group);

        if (*(_bcm_subport_group_subport_port_count[unit] + group_id) == 0) {
             /* delete the subport group */
            _bcm_subport_group_count[unit]--;
            _BCM_KT2_SUBPORT_GROUP_USED_CLR(unit, group_id);
            _bcm_linkphy_subport_group_count[unit]--;
            _BCM_KT2_SUBTAG_SUBPORT_GROUP_USED_CLR(unit, group_id);
        }
    }

    /* Cleanup SubTag VLAN id bit map */
    BCM_PBMP_ITER(SOC_INFO(unit).subtag_pbm, port) {
        mem = ING_PHYSICAL_PORT_TABLEm;
        if (SOC_MEM_FIELD_VALID(unit, mem, CASCADED_PORT_TYPEf)) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                unit, mem, port, CASCADED_PORT_TYPEf, 0));
        }
        if (SOC_MEM_FIELD_VALID(unit, mem, PORT_TYPEf)) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify
                (unit, mem, port, PORT_TYPEf, _BCM_KT2_PORT_TYPE_ETHERNET));
        }
        mem = EGR_PHYSICAL_PORTm;
        if (SOC_MEM_FIELD_VALID(unit, mem, CASCADED_PORT_TYPEf)) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                unit, mem, port, CASCADED_PORT_TYPEf, 0));
        }
        if (SOC_MEM_FIELD_VALID(unit, mem, PORT_TYPEf)) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                unit, mem, port, PORT_TYPEf, _BCM_KT2_PORT_TYPE_ETHERNET));
        }
    }

    BCM_PBMP_ITER(SOC_INFO(unit).subtag_allowed_pbm, port) {
        /* check to fix coverity static overrun error */
        if (port < SOC_MAX_NUM_PORTS) {
            sal_free(_bcm_subtag_vlan_id_bitmap[unit][port]);
            _bcm_subtag_vlan_id_bitmap[unit][port] = NULL;
        }
    }

    return BCM_E_NONE;

}


/*********************************
 * SubTag helper functions -end *
 ********************************/



STATIC void
bcm_kt2_subport_free_resource(int unit)
{
    if(_bcm_subport_group_bitmap[unit] != NULL) {
        sal_free(_bcm_subport_group_bitmap[unit]);
        _bcm_subport_group_bitmap[unit] = NULL;
    }

    if(_bcm_subport_pp_port_bitmap[unit] != NULL) {
        sal_free(_bcm_subport_pp_port_bitmap[unit]);
        _bcm_subport_pp_port_bitmap[unit] = NULL;
    }

    if (soc_feature(unit, soc_feature_linkphy_coe)) {

        if(_bcm_linkphy_group_bitmap[unit] != NULL) {
            sal_free(_bcm_linkphy_group_bitmap[unit]);
            _bcm_linkphy_group_bitmap[unit] = NULL;
        }

        if (_bcm_linkphy_subport_port_info[unit] != NULL) {
            sal_free(_bcm_linkphy_subport_port_info[unit]);
            _bcm_linkphy_subport_port_info[unit] = NULL;
        }

    } /* end of  soc_feature_linkphy_coe */

    if (soc_feature(unit, soc_feature_subtag_coe)) {

        if(_bcm_subtag_group_bitmap[unit] != NULL) {
            sal_free(_bcm_subtag_group_bitmap[unit]);
            _bcm_subtag_group_bitmap[unit] = NULL;
        }

        if (_bcm_subtag_subport_port_info[unit] != NULL) {
            sal_free(_bcm_subtag_subport_port_info[unit]);
            _bcm_subtag_subport_port_info[unit] = NULL;
        }

    } /* end of  soc_feature_subtag_coe */

    if (_bcm_subport_group_subport_port_count[unit] != NULL) {
        sal_free(_bcm_subport_group_subport_port_count[unit]);
        _bcm_subport_group_subport_port_count[unit] = NULL;
    }

    if (_bcm_subport_mutex[unit] != NULL) {
        sal_mutex_destroy(_bcm_subport_mutex[unit]);
        _bcm_subport_mutex[unit] = NULL;
    }

}

/*
 * Function:
 *      bcm_kt2_nhi_group_tc_profile_init
 * Purpose:
 *      Initialize kt2 NHI_GROUP_TC_PROFILE table. KT2 provides a TC PROFILE table with 256
 *      items which are used to store the NHI_OFFSET.  The index of TC PROFILE derives from 
 *      concatenating the TC PROFILE INDEX and internal priority of the packet.
 * Parameters:
 *      unit      - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
bcm_kt2_nhi_group_tc_profile_init(int unit)
{
    int rv = BCM_E_NONE;
    int nhi_group, int_pri;
    nhi_group_tc_profile_entry_t nhi_entry;
    
    if (SOC_MEM_IS_VALID(unit, NHI_GROUP_TC_PROFILEm)) {
        for (nhi_group = 0; nhi_group <= 0xf; nhi_group++) {
            for (int_pri = 0; int_pri <= 0xf; int_pri++) {
                sal_memset(&nhi_entry, 0, sizeof(nhi_group_tc_profile_entry_t));  
                soc_mem_field32_set(unit, NHI_GROUP_TC_PROFILEm, &nhi_entry, 
                                NHI_OFFSETf, int_pri);
                rv = soc_mem_write(unit, NHI_GROUP_TC_PROFILEm,
                        MEM_BLOCK_ALL, (nhi_group<<4)|int_pri, &nhi_entry);
                if (SOC_FAILURE(rv)) {
                    LOG_ERROR(BSL_LS_BCM_SUBPORT,
                              (BSL_META_U(unit,
                                          "ERROR: NHI_GROUP_TC_PROFIE init failed\n")));
                    return rv;
                }
            }
        }
    }

    return rv;  
}


/*
 * Function:
 *      bcm_kt2_subport_init
 * Purpose:
 *      Initialize the SUBPORT software module
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_init(int unit)
{
    int rv = BCM_E_NONE;
    soc_info_t *si = &SOC_INFO(unit);

    /* Subport group bitmap to keep track of used subport groups */
    if (_bcm_subport_group_bitmap[unit] == NULL) {

        _bcm_subport_group_bitmap[unit] = 
            sal_alloc(SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_GROUP_MAX),
            "subport_group_bitmap");

        if (_bcm_subport_group_bitmap[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:subport_init: group bitmap alloc failed\n")));

            bcm_kt2_subport_free_resource(unit);

            return BCM_E_MEMORY;
        }
    }

    sal_memset(_bcm_subport_group_bitmap[unit], 0,
        SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_GROUP_MAX));

    _bcm_subport_group_count[unit] = 0;

    /* Keep Count of subport port created per subport group */
    if (_bcm_subport_group_subport_port_count[unit] == NULL) {
        _bcm_subport_group_subport_port_count[unit] =
            sal_alloc((_BCM_KT2_SUBPORT_GROUP_MAX * sizeof(int)),
                        "subport_group_subport_port_count");
        if (_bcm_subport_group_subport_port_count[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: "
                                   "subport_group_subport_port_count alloc failed\n")));

            return BCM_E_MEMORY;
        }
    }

    sal_memset(_bcm_subport_group_subport_port_count[unit], 0,
        (_BCM_KT2_SUBPORT_GROUP_MAX * sizeof(int)));

    /* LinkPHY subport initialization */
    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        BCM_PBMP_NOT_NULL(si->linkphy_pbm)) {
        rv = _bcm_linkphy_subport_init(unit);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY init failed.\n")));

            bcm_kt2_subport_free_resource(unit);
            return rv;
        }
    }

    /* Subtag CoE subport initialization */
    if (soc_feature(unit, soc_feature_subtag_coe) &&
        BCM_PBMP_NOT_NULL(si->subtag_allowed_pbm)) {
        rv = _bcm_subtag_subport_init(unit);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag init failed.\n")));

            bcm_kt2_subport_free_resource(unit);
            return rv;
        }
    }

    if (soc_feature(unit, soc_feature_lltag)) {
        rv = bcm_kt2_nhi_group_tc_profile_init(unit);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: tc profile init failed.\n")));

            bcm_kt2_subport_free_resource(unit);
            return rv;
        }
    }

    if (_bcm_subport_pp_port_bitmap[unit] == NULL) {

        _bcm_subport_pp_port_bitmap[unit] =
        sal_alloc(SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX + 1),
            "subport_group_bitmap");

        if (_bcm_subport_pp_port_bitmap[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:subport_init: pp_port bitmap alloc failed\n")));

            bcm_kt2_subport_free_resource(unit);

            return BCM_E_MEMORY;
        }
    }

    sal_memset(_bcm_subport_pp_port_bitmap[unit], 0,
        SHR_BITALLOCSIZE(_BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX + 1));

    _bcm_subport_pp_port_count[unit] = 0;

    /* Create subport mutex */
    if (NULL == _bcm_subport_mutex[unit]) {
        _bcm_subport_mutex[unit] =
            sal_mutex_create("linkphy_subtag subport mutex");

        if (_bcm_subport_mutex[unit] == NULL) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:linkphy_subtag subport mutex create failed\n")));

            return BCM_E_MEMORY;
        }
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    if(SOC_WARM_BOOT(unit)) {
        rv = _bcm_kt2_subport_reinit(unit);
        if (rv != BCM_E_NONE) {
        }
    }
#endif /* BCM_WARM_BOOT_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_kt2_subport_group_create
 * Purpose:
 *      Create a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport group config information
 *      group  - (OUT) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_group_create(int unit, 
                             bcm_subport_group_config_t *config,
                             bcm_gport_t *group)
{
    int rv = BCM_E_NONE, group_id = -1, i = 0, is_trunk = 0;
    bcm_port_t port;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if ((config == NULL) || (group == NULL)) {
        return BCM_E_PARAM;
    }

    if ((config->flags & BCM_SUBPORT_GROUP_TYPE_LINKPHY) &&
        (config->flags & BCM_SUBPORT_GROUP_TYPE_SUBPORT_TAG)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR:Either LinkPHY or SubTag group can "
                               "be created at a time\n")));
        return BCM_E_PARAM;
    }

    rv = _bcm_esw_port_gport_validate(unit, config->port, &port);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Invalid gport 0x%x for subport group create.\n"),
                   config->port));
        return BCM_E_PORT;
    }

    if ((config->flags & BCM_SUBPORT_GROUP_TYPE_LINKPHY) &&
        !BCM_PBMP_MEMBER(si->linkphy_pbm, port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: port %d is not member of pbmp_linkphy\n"),
                   port));
        return BCM_E_PORT;
    }

    if ((config->flags & BCM_SUBPORT_GROUP_TYPE_SUBPORT_TAG) &&
        !BCM_PBMP_MEMBER(si->subtag_pbm, port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: SubportTag/CoE should be enabled"
                               " for port %d\n"), port));
        return BCM_E_PORT;
    }

    if (_bcm_subport_group_count[unit] == _BCM_KT2_SUBPORT_GROUP_MAX) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: All %d subport groups already used.\n"),
                   _BCM_KT2_SUBPORT_GROUP_MAX));
        return BCM_E_FULL;
    }

    if (config->flags & BCM_SUBPORT_GROUP_WITH_ID) {
        group_id = BCM_GPORT_SUBPORT_GROUP_GET(*group);
        /* Group id should be within 0 to 128.
        * BCM_GPORT_SUBPORT_GROUP_MASK is 0xffffff (for all chips)
        * but subtag/linkphy group id is only first 9 bits (8-0). 
        * Check that only subtag/linkphy group id bits (8-0) are configured*/
        if ((group_id >> _BCM_KT2_SUBPORT_GROUP_PORT_SHIFT)
                & _SHR_GPORT_SUBPORT_GROUP_MASK) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport group id = %d should be within 0 to"
                                   " %d\n"), group_id, _BCM_KT2_SUBPORT_GROUP_MAX-1));
            return BCM_E_PARAM;
        }
        if (_BCM_KT2_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport group id = %d is already used\n"),
                       group_id));
            return BCM_E_EXISTS;
        }
    } else {
        for (i =0; i < _BCM_KT2_SUBPORT_GROUP_MAX; i++) {
            if (!_BCM_KT2_SUBPORT_GROUP_USED_GET(unit, i)) {
                group_id = i;
                break;
            }
        }

        if (i == _BCM_KT2_SUBPORT_GROUP_MAX) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: All %d subport groups already used.\n"),
                       i));
            return BCM_E_FULL;
        }
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe) && 
        (config->flags & BCM_SUBPORT_GROUP_TYPE_LINKPHY)) {

        rv  =_bcm_linkphy_subport_group_create(unit,
                is_trunk, port, group_id, group);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY subport group create failed"
                                   " for port %d. rv = %d(%s)\n"),
                       config->port, rv, soc_errmsg(rv)));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
    } else if (soc_feature(unit, soc_feature_subtag_coe) && 
        (config->flags & BCM_SUBPORT_GROUP_TYPE_SUBPORT_TAG)) {
        rv  =_bcm_subtag_subport_group_create(unit,
                is_trunk, port, group_id, group);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag subport group create failed"
                                   " for port %d. rv = %d(%s)\n"),
                       config->port, rv, soc_errmsg(rv)));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
    }

    _BCM_KT2_SUBPORT_GROUP_USED_SET(unit, group_id);
    _bcm_subport_group_count[unit]++;

    _BCM_KT2_SUBPORT_UNLOCK(unit);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return BCM_E_NONE;

}


/*
 * Function:
 *      bcm_kt2_subport_group_destroy
 * Purpose:
 *      Destroy a subport group and all subport ports attached to it.
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier for subport group
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_kt2_subport_group_destroy(int unit,
                              bcm_gport_t group)
{
    int rv = BCM_E_NONE;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_GROUP(group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: group 0x%x is not subport group gport\n"),
                   group));
        return BCM_E_PARAM;
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) {
        if (BCM_PBMP_IS_NULL(si->linkphy_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv = _bcm_linkphy_subport_group_destroy(unit, group);
    } else if (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group)) {
        if (BCM_PBMP_IS_NULL(si->subtag_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv = _bcm_subtag_subport_group_destroy(unit, group);
    }

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: subport group 0x%x destroy failed. rv= %d(%s)\n"),
                   group, rv, soc_errmsg(rv)));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_group_get
 * Purpose:
 *      Get a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier
 *      config - (OUT) Subport group config information
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_group_get(int unit,
                          bcm_gport_t group,
                          bcm_subport_group_config_t *config)
{
    int rv = BCM_E_NONE;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    if (!BCM_GPORT_IS_SUBPORT_GROUP(group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: group 0x%x is not subport group gport\n"),
                   group));
        return BCM_E_PARAM;
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) {
        if (BCM_PBMP_IS_NULL(si->linkphy_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv = _bcm_linkphy_subport_group_get(unit, group, config);
    } else if (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group)) {
        if (BCM_PBMP_IS_NULL(si->subtag_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv = _bcm_subtag_subport_group_get(unit, group, config);
    }

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: subport group get failed for 0x%x, rv = %d(%s)\n"),
                   group, rv, soc_errmsg(rv)));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_group_traverse
 * Purpose:
 *      Traverse all valid subports for given subport group and call the user
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      group     - Subport group GPORT identifier
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 */

int
bcm_kt2_subport_group_traverse(int unit,
                               bcm_gport_t group,
                               bcm_subport_port_traverse_cb cb,
                               void *user_data)
{
    int rv = BCM_E_NONE;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_GROUP(group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: group 0x%x is not subport group gport\n"),
                   group));
        return BCM_E_PARAM;
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) {
        if (BCM_PBMP_IS_NULL(si->linkphy_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv =_bcm_linkphy_subport_group_traverse(unit, group, cb, user_data);
    } else if (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group)) {
        if (BCM_PBMP_IS_NULL(si->subtag_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv =_bcm_subtag_subport_group_traverse(unit, group, cb, user_data);
    }

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: subport group traverse failed for 0x%x, rv = %d(%s)\n"),
                   group, rv, soc_errmsg(rv)));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_group_resolve
 * Purpose:
 *      resolve physical port associated with the LinkPHy subport group
 */
int
bcm_kt2_subport_group_resolve(int unit,
                              bcm_gport_t subport_group_gport,
                              bcm_module_t *modid, bcm_port_t *port,
                              bcm_trunk_t *trunk_id, int *id)

{
    bcm_module_t my_modid;
    uint16 group_id;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    *modid = -1;
    *port = -1;
    *id = -1;
    *trunk_id = BCM_TRUNK_INVALID;

    if (!BCM_GPORT_IS_SUBPORT_GROUP(subport_group_gport)) {
        LOG_WARN(BSL_LS_BCM_SUBPORT,
                 (BSL_META_U(unit,
                             "gport 0x%x is not subport gport\n"),
                  subport_group_gport));
        return BCM_E_NONE;
    }

    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(subport_group_gport)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(subport_group_gport))) {

        group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(subport_group_gport);
        if(!_BCM_KT2_SUBPORT_GROUP_USED_GET(unit, group_id)) {
            LOG_WARN(BSL_LS_BCM_SUBPORT,
                     (BSL_META_U(unit,
                                 "subport gport 0x%x is not used\n"),
                      subport_group_gport));
            return BCM_E_NONE;
        }

        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        *modid = my_modid;
        *port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(subport_group_gport);
    } else if (soc_feature(unit, soc_feature_subport_enhanced)) {
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
        return _bcm_tr2_subport_group_resolve(unit, subport_group_gport,
                                            modid, port, trunk_id,id);
#endif
    } else if (soc_feature(unit, soc_feature_subport)) {
#if defined (BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
        return _bcm_tr_subport_group_resolve(unit, subport_group_gport,
                                             modid, port, trunk_id, id);
#endif
    }

    return BCM_E_NONE;

}

/*
 * Function:
 *      bcm_kt2_subport_port_add
 * Purpose:
 *      Add a subport to a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport config information
 *      port   - (OUT) Subport_port GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_port_add(int unit,
                         bcm_subport_config_t *config,
                         bcm_gport_t *gport)
{
    int rv = BCM_E_NONE;
    uint16 group_id;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_GROUP(config->group)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: config->group 0x%x is not subport group gport\n"),
                   config->group));
        return BCM_E_PARAM;
    }

    group_id = _BCM_KT2_SUBPORT_GROUP_ID_GET(config->group);
    if(!_BCM_KT2_SUBPORT_GROUP_USED_GET(unit, group_id)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR:config->group 0x%x is not found\n"),
                   config->group));
        return BCM_E_NOT_FOUND;
    }

    if (_bcm_subport_pp_port_count[unit] == _BCM_KT2_SUBPORT_PORT_MAX) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR:All %d subport pp_port used\n"),
                   _BCM_KT2_SUBPORT_PORT_MAX));
        return BCM_E_FULL;
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(config->group)) {
        if (BCM_PBMP_IS_NULL(si->linkphy_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv = _bcm_linkphy_subport_port_add(unit, config, gport);
    } else if (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(config->group)) {
        if (BCM_PBMP_IS_NULL(si->subtag_pbm)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag ports not configured\n")));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        rv = _bcm_subtag_subport_port_add(unit, config, gport);
    }

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Subport port add failed. rv = %d(%s)\n"),
                   rv, soc_errmsg(rv)));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return rv;
}

/*
 * Function:
 *      bcm_kt2_subport_port_delete
 * Purpose:
 *      Delete a subport_port
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) Subport_port GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_port_delete(int unit,
                            bcm_gport_t subport_port_gport)
{
    int rv = BCM_E_NONE;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_PORT(subport_port_gport)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: gport 0x%x is not subport port gport\n"),
                   subport_port_gport));
        return BCM_E_PARAM;
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, subport_port_gport)) {
        rv = _bcm_linkphy_subport_port_delete(unit, subport_port_gport);
    } else if (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, subport_port_gport)) {
        rv = _bcm_subtag_subport_port_delete(unit, subport_port_gport);
    }

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Subport port 0x%x delete failed. rv = %d(%s)\n"),
                   subport_port_gport, rv, soc_errmsg(rv)));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return rv;
}

/*
 * Function:
 *      bcm_kt2_subport_port_get
 * Purpose:
 *      Get the subport_port config
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) Subport_port GPORT (generic port) identifier
 *      config - (OUT) Subport port config
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_kt2_subport_port_get(int unit,
                         bcm_gport_t subport_port_gport,
                         bcm_subport_config_t *config)
{
    int rv = BCM_E_NONE;

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_PORT(subport_port_gport)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: gport 0x%x is not subport port gport\n"),
                   subport_port_gport));
        return BCM_E_PARAM;
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, subport_port_gport)) {
        rv = _bcm_linkphy_subport_port_get(unit, subport_port_gport, config);
    } else if (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, subport_port_gport)) {
        rv = _bcm_subtag_subport_port_get(unit, subport_port_gport, config);
    }

    if (SOC_FAILURE(rv)) {
        LOG_VERBOSE(BSL_LS_BCM_SUBPORT,
                    (BSL_META_U(unit,
                                "ERROR: Subport port 0x%x get failed. rv=%d(%s)\n"),
                     subport_port_gport, rv, soc_errmsg(rv)));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_port_traverse
 * Purpose:
 *      Add all valid subports under all valid subport groups
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_kt2_subport_port_traverse(int unit,
                              bcm_subport_port_traverse_cb cb,
                              void *user_data)
{
    int rv = BCM_E_NONE;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    _BCM_KT2_SUBPORT_LOCK(unit);

    if(soc_feature(unit, soc_feature_linkphy_coe) &&
        BCM_PBMP_NOT_NULL(si->linkphy_pbm)) {
        rv = _bcm_linkphy_subport_port_traverse(unit, cb, user_data);
    }
    if (soc_feature(unit, soc_feature_subtag_coe) &&
        BCM_PBMP_NOT_NULL(si->subtag_pbm)) {
        rv = _bcm_subtag_subport_port_traverse(unit, cb, user_data);
    }

    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR:Subport port traverse failed. rv = %d(%s)\n"),
                   rv,soc_errmsg(rv)));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_port_resolve
 * Purpose:
 *      resolve pp_port associated with the LinkPHy subport port 
 */
int
bcm_kt2_subport_port_resolve(int unit,
                              bcm_gport_t subport_port_gport,
                              bcm_module_t *modid, bcm_port_t *port,
                              bcm_trunk_t *trunk_id, int *id)
{
    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    *modid = -1;
    *port = -1;
    *id = -1;
    *trunk_id = BCM_TRUNK_INVALID;

    if (!BCM_GPORT_IS_SUBPORT_PORT(subport_port_gport)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: gport 0x%x is not subport gport\n"),
                   subport_port_gport));
        return BCM_E_PARAM;
    }

    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, subport_port_gport)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
        _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, subport_port_gport))) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_pp_port_to_modport_get(
                                unit, subport_port_gport, modid, port));
    } else if (soc_feature(unit, soc_feature_subport_enhanced)) {
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
        return _bcm_tr2_subport_port_resolve(unit,subport_port_gport,
                                             modid, port, trunk_id, id);
#endif /* defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3) */
    } else if (soc_feature(unit, soc_feature_subport)) {
#if defined (BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
        return _bcm_tr_subport_port_resolve(unit, subport_port_gport,
                                            modid, port, trunk_id, id);
#endif
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_pp_port_subport_info_get
 * Purpose:
 *      Get physical port number and port_type (LinkPHY/SubTag), SubTag/Device internal
 *      stream IDs for given pp_port.
 *      If subport_port and group are not configured for the pp_port the port number
 *      and type are returned as -1.
 * Returns
 *      BCM_E_XXX
 */

int bcm_kt2_subport_pp_port_subport_info_get(int unit,
                              bcm_port_t pp_port, 
                              _bcm_kt2_subport_info_t *subport_info)
{
    bcm_gport_t subport_port;
    int rv = BCM_E_NONE, i = 0;
    uint16 int_stream_id_base = 0, ext_stream_id_base = 0, int_stream_id = 0;
    bcm_subport_config_t subport_port_config;

    if (subport_info == NULL) {
        return BCM_E_MEMORY;
    }

    sal_memset(subport_info, 0, sizeof(_bcm_kt2_subport_info_t));

    if ((pp_port < _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) ||
        (pp_port > _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
        if (SOC_PORT_VALID(unit, pp_port)) {
            subport_info->port = pp_port;
            return BCM_E_NONE;
        } else {
            return BCM_E_PORT;
        }
    }

    if (BCM_PBMP_MEMBER(SOC_INFO(unit).linkphy_pp_port_pbm, pp_port)) {
        subport_info->port_type = _BCM_KT2_SUBPORT_TYPE_LINKPHY;
    } else if (BCM_PBMP_MEMBER(SOC_INFO(unit).subtag_pp_port_pbm, pp_port)) {
        subport_info->port_type = _BCM_KT2_SUBPORT_TYPE_SUBTAG;
    } else {
        return BCM_E_PORT;
    }

    _BCM_KT2_SUBPORT_PORT_ID_SET(subport_port, pp_port);
    _BCM_KT2_SUBPORT_PORT_TYPE_SET(subport_port, subport_info->port_type);

    rv = bcm_kt2_subport_port_get(unit, subport_port, &subport_port_config);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    subport_info->port =
        _BCM_KT2_SUBPORT_GROUP_PORT_GET(subport_port_config.group);

    if (subport_info->port_type == _BCM_KT2_SUBPORT_TYPE_SUBTAG) {
        subport_info->subtag = subport_port_config.pkt_vlan & 0xFFF;
    } else {
        subport_info->num_streams = subport_port_config.num_streams;

        rv = _bcm_linkphy_port_int_stream_id_base_get(unit, subport_info->port,
                &int_stream_id_base);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Linkphy port %d internal "
                                   "stream id base get failed\n"), subport_info->port));
            return rv;
        }

        rv = _bcm_linkphy_port_ext_stream_id_base_get(unit, subport_info->port,
                       &ext_stream_id_base);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Linkphy subport group external stream id "
                                   "base get failed for port %d.\n"), subport_info->port));
            return rv;
        }

        for (i = 0; i < subport_port_config.num_streams; i++) {
            int_stream_id =
                subport_port_config.stream_id_array[i] - ext_stream_id_base;
            subport_info->dev_int_stream_id[i] =
                int_stream_id + int_stream_id_base;
        }
    }

    return BCM_E_NONE;

}

/*
 * Function:
 *      bcm_kt2_subport_cleanup
 * Purpose:
 *      Delate all valid subport ports and all valid subport groups.
        Also clean up software resources.
 * Parameters:
 *      unit      - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_cleanup(int unit)
{
    int rv = BCM_E_NONE;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe)) {
        if (BCM_PBMP_NOT_NULL(si->linkphy_pbm)) {
            rv = _bcm_linkphy_subport_cleanup(unit);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: LinkPHY cleanup failed\n")));
                return rv;
            }
        }
    }
    if (soc_feature(unit, soc_feature_subtag_coe)) {
        if (BCM_PBMP_NOT_NULL(si->subtag_allowed_pbm)) {
            rv = _bcm_subtag_subport_cleanup(unit);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: SubTag cleanup failed\n")));
                return rv;
            }
        }
    }

    bcm_kt2_subport_free_resource(unit);

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_kt2_subport_group_linkphy_config_get
 * Purpose:
 *      Get ALL or selective LinkPHY configuration attributes 
 *      on a given port GPORT.
 * Parameters:
 * Parameters:
 *      uint (IN) BCM device number
 *      port (IN) port GPORT
 *      linkphy_cfg (OUT) LinkPHY configuration 
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *     When retrieving, if an error is returned then the entire contents of the 
 *        linkphy_config structure are undefined.
 */

int 
bcm_kt2_subport_group_linkphy_config_get(int unit, 
                             bcm_gport_t port, 
                             bcm_subport_group_linkphy_config_t *linkphy_config)
{
    int rv = BCM_E_NONE;
    uint32 regval = 0;
    uint32 header_mode = 0;
    uint32 mac_addr[2] = {0, 0};
    uint64 rval64, data;
    uint16 base_stream_id = 0;
    soc_reg_t reg;
    int base_stream_id_max = 0;
    int ne_mac_check = 0, fe_mac_check = 0, dfc_destmac_check = 0;
    int dfc_opcode_check = 0, dfc_time_check = 0;
    soc_info_t *si = &SOC_INFO(unit);

    if (!soc_feature(unit, soc_feature_linkphy_coe)) {
        return BCM_E_UNAVAIL;
    }

    if (linkphy_config == NULL) {
        return BCM_E_PARAM;
    }

    if (!BCM_PBMP_MEMBER(si->linkphy_pbm, port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Invalid port %d for LinkPHY config get\n"),
                   port));
        return BCM_E_PORT;
    }

    sal_memset(linkphy_config, 0, sizeof(bcm_subport_group_linkphy_config_t));

    reg = RXLP_PORT_STREAM_ID_BASEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        base_stream_id_max = _BCM_KT2_LINKPHY_STREAMS_MAX -
                             _BCM_KT2_LINKPHY_PER_PORT_STREAMS_MAX;
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_STREAM_ID_BASEr(unit, port, &regval));
        base_stream_id = soc_reg_field_get(unit, reg, regval, BASEf);
        if ((base_stream_id % 8) || (base_stream_id > base_stream_id_max)) {
            linkphy_config->action_mask &= ~BCM_SUBPORT_LINKPHY_BASE_STREAM_ID;
        } else {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_BASE_STREAM_ID;
            linkphy_config->base_stream_id = base_stream_id;
        }
    }

    reg = RXLP_PORT_LP_MODE_CONTROLr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_LP_MODE_CONTROLr(unit, port, &regval));
        header_mode = soc_reg_field_get(unit, reg, regval, HEADER_MODEf);
        if ((header_mode == bcmSubportLinkphyHeaderModeEthAdapt) ||
            (header_mode == bcmSubportLinkphyHeaderModeTciLength)) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_HEADER_MODE;
            linkphy_config->header_mode = header_mode;
        }
    }

    COMPILER_64_ZERO(rval64);
    COMPILER_64_ZERO(data);
    reg = RXLP_PORT_NEAR_END_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_NEAR_END_MAC_ADDRr(unit, port, &rval64));
        data = soc_reg64_field_get(unit, reg, rval64, CHECK_ENABLEf);
        COMPILER_64_TO_32_LO(regval, data);
        if ((ne_mac_check = (regval & 0x1))) {
            linkphy_config->rx_check_flags |=
                BCM_SUBPORT_LINKPHY_RX_NEAREND_MAC_CHECK;
        }
    }

    COMPILER_64_ZERO(rval64);
    COMPILER_64_ZERO(data);
    reg = RXLP_PORT_FAR_END_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_FAR_END_MAC_ADDRr(unit, port, &rval64));
        data = soc_reg64_field_get(unit, reg, rval64, CHECK_ENABLEf);
        COMPILER_64_TO_32_LO(regval, data);
        if ((fe_mac_check = (regval & 0x1))) {
            linkphy_config->rx_check_flags |=
                BCM_SUBPORT_LINKPHY_RX_FAREND_MAC_CHECK;
        }
    }

    COMPILER_64_ZERO(rval64);
    COMPILER_64_ZERO(data);
    reg = RXLP_PORT_DFC_DESTINATION_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_DFC_DESTINATION_MAC_ADDRr(unit, port, &rval64));
        data = soc_reg64_field_get(unit, reg, rval64, CHECK_ENABLEf);
        COMPILER_64_TO_32_LO(regval, data);
        if ((dfc_destmac_check = (regval & 0x1))) {
            linkphy_config->rx_check_flags |=
                BCM_SUBPORT_LINKPHY_RX_DFC_DEST_MAC_CHECK;
        }
    }

    reg = RXLP_PORT_DFC_OPCODEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_OPCODEr(unit, port, &regval));
        if ((dfc_opcode_check =
            soc_reg_field_get(unit, reg, regval, CHECK_ENABLEf))) {
            linkphy_config->rx_check_flags |=
                BCM_SUBPORT_LINKPHY_RX_DFC_OPCODE_CHECK;
        }
    }

    reg = RXLP_PORT_DFC_TIMEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_TIMEr(unit, port, &regval));
        if ((dfc_time_check =
            soc_reg_field_get(unit, reg, regval, CHECK_ENABLEf))) {
            linkphy_config->rx_check_flags |=
                BCM_SUBPORT_LINKPHY_RX_DFC_TIME_CHECK;
        }
    }

    if (ne_mac_check || fe_mac_check || dfc_destmac_check ||
        dfc_opcode_check || dfc_time_check) {
        linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_CHECK_FLAGS;
    }

    reg = RXLP_PORT_VLAN_TPIDr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_VLAN_TPIDr(unit, port, &regval));
        if ((linkphy_config->rx_vlan_tpid = 
                soc_reg_field_get(unit, reg, regval, VLAN_TPIDf))) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_VLAN_TPID;
        }
    }

    reg = RXLP_PORT_NEAR_END_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(_bcm_linkphy_reg_mac_addr_get(unit, port,
            RXLP_PORT_NEAR_END_MAC_ADDRr, MAC_ADDRf,
            linkphy_config->rx_nearend_mac));
        SAL_MAC_ADDR_TO_UINT32(linkphy_config->rx_nearend_mac, mac_addr);
        if ((mac_addr[0] != 0) || (mac_addr[1] != 0)) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_NEAREND_MAC;
        }
    }

    reg = RXLP_PORT_FAR_END_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(_bcm_linkphy_reg_mac_addr_get(unit, port,
            RXLP_PORT_FAR_END_MAC_ADDRr, MAC_ADDRf,
            linkphy_config->rx_farend_mac));
        SAL_MAC_ADDR_TO_UINT32(linkphy_config->rx_farend_mac, mac_addr);
        if ((mac_addr[0] != 0) || (mac_addr[1] != 0)) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_FAREND_MAC;
        }
    }

    reg = RXLP_PORT_DFC_DESTINATION_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(_bcm_linkphy_reg_mac_addr_get(unit, port,
            RXLP_PORT_DFC_DESTINATION_MAC_ADDRr, MAC_ADDRf,
            linkphy_config->rx_dfc_dest_mac));
        SAL_MAC_ADDR_TO_UINT32(linkphy_config->rx_dfc_dest_mac, mac_addr);
        if ((mac_addr[0] != 0) || (mac_addr[1] != 0)) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_DFC_DEST_MAC;
        }
    }

    reg = RXLP_PORT_DFC_OPCODEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_OPCODEr(unit, port, &regval));
        if ((linkphy_config->rx_dfc_opcode = 
                soc_reg_field_get(unit, reg, regval, OPCODEf))) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_DFC_OPCODE;
        }
    }

    reg = RXLP_PORT_DFC_TIMEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_TIMEr(unit, port, &regval));
        if ((linkphy_config->rx_dfc_time = 
                soc_reg_field_get(unit, reg, regval, DFC_TIMEf))) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_DFC_TIME;
        }
    }

    reg = RXLP_PORT_DFC_MAC_TYPEr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_MAC_TYPEr(unit, port, &regval));
        if ((linkphy_config->rx_dfc_mac_type =
                soc_reg_field_get(unit, reg, regval, MAC_TYPEf))) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_RX_DFC_MAC_TYPE;
        }
    }

    reg = TXLP_PORT_VLAN_TPIDr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(READ_TXLP_PORT_VLAN_TPIDr(unit, port, &regval));
        if ((linkphy_config->tx_vlan_tpid = 
                soc_reg_field_get(unit, reg, regval, VLAN_TPIDf))) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_TX_VLAN_TPID;
        }
    }

    reg = TXLP_PORT_NEAR_END_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(_bcm_linkphy_reg_mac_addr_get(unit, port,
            TXLP_PORT_NEAR_END_MAC_ADDRr, MAC_ADDRf,
            linkphy_config->tx_nearend_mac));
        SAL_MAC_ADDR_TO_UINT32(linkphy_config->tx_nearend_mac, mac_addr);
        if ((mac_addr[0] != 0) || (mac_addr[1] != 0)) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_TX_NEAREND_MAC;
        }
    }

    reg = TXLP_PORT_FAR_END_MAC_ADDRr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(_bcm_linkphy_reg_mac_addr_get(unit, port,
            TXLP_PORT_FAR_END_MAC_ADDRr, MAC_ADDRf,
            linkphy_config->tx_farend_mac));
        SAL_MAC_ADDR_TO_UINT32(linkphy_config->tx_farend_mac, mac_addr);
        if ((mac_addr[0] != 0) || (mac_addr[1] != 0)) {
            linkphy_config->action_mask |= BCM_SUBPORT_LINKPHY_TX_FAREND_MAC;
        }
    }


    reg = TXLP_PORT_LP_MODE_CONTROLr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        BCM_IF_ERROR_RETURN(
            READ_TXLP_PORT_LP_MODE_CONTROLr(unit, port, &regval));
        if (soc_reg_field_get(unit, reg, regval, PENULTIMATE_MODEf)) {
            linkphy_config->action_mask |= 
                BCM_SUBPORT_LINKPHY_TX_PENULTIMATE_MODE;
        }
    }

    return rv;
}

/**
 * Function:
 *      bcm_kt2_subport_group_linkphy_config_set
 * Purpose:
 *      Set ALL or selective LinkPHY configuration attributes 
 *      on a given port GPORT.
 * Parameters:
 *      uint (IN) BCM device number
 *      port (IN) port GPORT
 *      linkphy_cfg (IN) LinkPHY configuration 
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      The 'linkphy_config.action_mask' field is used to indicate if ALL or 
 *              selective attributes will be set or get.
 *      While setting or restoring LinkPHY port configuration, if the call
 *              returns an error then the port state is undefined.
 *      Each field in linkphy_config structure has a corresponding mask bit
 *              to set that value during the execution of this functions.
 *              The OR of all BCM_SUBPORT_LINKPHY_xxx masks
 *              should be stored in the linkphy_config.action_mask field.
 *      The 'linkphy_config.base_stream_id' member should be a multiple
 *              of 8 and <=896 for bcm5645x device.
 *              LinkPHY fragment headers with stream ID range 
 *              {base_stream_id, base_stream_id+127} are valid for the gport.
 *              Fragments with stream id outside this range will be dropped.
 *              It is recommended to configure the 'base_stream_id' first
 *              on a given port before adding LinkPHY subports to the port
 *              using bcm_subport_port_add() API.
 *      The 'linkphy_config.rx_check_flags' member is used to enable LinkPHY 
 *              fragment header parameter checks on received fragments.
 */

int
bcm_kt2_subport_group_linkphy_config_set(int unit,
                             bcm_gport_t port,
                             bcm_subport_group_linkphy_config_t *linkphy_config)
{
    int rv = BCM_E_NONE;
    uint32 mask = 0, regval = 0, rx_check_flags = 0;
    uint64 rval64, data;
    uint16 base_stream_id = 0;
    soc_reg_t reg;
    int base_stream_id_max = 0;
    soc_info_t *si = &SOC_INFO(unit);

    COMPILER_64_ZERO(rval64);
    COMPILER_64_ZERO(data);

    if (!soc_feature(unit, soc_feature_linkphy_coe)) {
        return BCM_E_UNAVAIL;
    }

    if (!BCM_PBMP_MEMBER(si->linkphy_pbm, port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Invalid port %d for LinkPHY config set\n"),
                   port));
        return BCM_E_PORT;
    }

    mask = linkphy_config->action_mask;

    if (mask & BCM_SUBPORT_LINKPHY_BASE_STREAM_ID) {
        base_stream_id = linkphy_config->base_stream_id;
        base_stream_id_max = _BCM_KT2_LINKPHY_STREAMS_MAX -
                             _BCM_KT2_LINKPHY_PER_PORT_STREAMS_MAX;
        if ((base_stream_id % 8) || (base_stream_id > base_stream_id_max)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:base stream id should be "
                                   "multiple of 8 and <= %d\n"), base_stream_id_max));
            return BCM_E_PARAM;
        }

        reg = RXLP_PORT_STREAM_ID_BASEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            regval = 0;
            BCM_IF_ERROR_RETURN(
                READ_RXLP_PORT_STREAM_ID_BASEr(unit, port, &regval));
            soc_reg_field_set(unit, reg, &regval, BASEf, base_stream_id);
            BCM_IF_ERROR_RETURN(
                WRITE_RXLP_PORT_STREAM_ID_BASEr(unit, port, regval));
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_HEADER_MODE) {
        if ((linkphy_config->header_mode !=
                bcmSubportLinkphyHeaderModeEthAdapt) &&
            (linkphy_config->header_mode !=
                bcmSubportLinkphyHeaderModeTciLength)){
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: invalid LinkPHY header mode %d\n"),
                       linkphy_config->header_mode));
            return BCM_E_PARAM;
        }
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_IS_KATANA2(unit) &&
            (linkphy_config->header_mode ==
                bcmSubportLinkphyHeaderModeTciLength)){
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY TCI+Length header mode is not supported.\n")));
            return BCM_E_PARAM;
        }
#endif

        reg = RXLP_PORT_LP_MODE_CONTROLr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            regval = 0;
            BCM_IF_ERROR_RETURN(
                READ_RXLP_PORT_LP_MODE_CONTROLr(unit, port, &regval));
            soc_reg_field_set(unit, reg, &regval,
                HEADER_MODEf, linkphy_config->header_mode);
            BCM_IF_ERROR_RETURN(
                WRITE_RXLP_PORT_LP_MODE_CONTROLr(unit, port, regval));
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_CHECK_FLAGS) {
        rx_check_flags = linkphy_config->rx_check_flags;

        reg = RXLP_PORT_NEAR_END_MAC_ADDRr;
        COMPILER_64_ZERO(rval64);
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_NEAR_END_MAC_ADDRr(unit, port, &rval64));
        COMPILER_64_ZERO(data);
        COMPILER_64_SET(data, 0,
          ((rx_check_flags & BCM_SUBPORT_LINKPHY_RX_NEAREND_MAC_CHECK) ? 1: 0));
        soc_reg64_field_set(unit, reg, &rval64, CHECK_ENABLEf, data);
        BCM_IF_ERROR_RETURN(
            WRITE_RXLP_PORT_NEAR_END_MAC_ADDRr(unit, port, rval64));

        reg = RXLP_PORT_FAR_END_MAC_ADDRr;
        COMPILER_64_ZERO(rval64);
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_FAR_END_MAC_ADDRr(unit, port, &rval64));
        COMPILER_64_ZERO(data);
        COMPILER_64_SET(data, 0,
          ((rx_check_flags & BCM_SUBPORT_LINKPHY_RX_FAREND_MAC_CHECK) ? 1: 0));
        soc_reg64_field_set(unit, reg, &rval64, CHECK_ENABLEf, data);
        BCM_IF_ERROR_RETURN(
            WRITE_RXLP_PORT_FAR_END_MAC_ADDRr(unit, port, rval64));

        reg = RXLP_PORT_DFC_DESTINATION_MAC_ADDRr;
        COMPILER_64_ZERO(rval64);
        BCM_IF_ERROR_RETURN(
            READ_RXLP_PORT_DFC_DESTINATION_MAC_ADDRr(unit, port, &rval64));
        COMPILER_64_ZERO(data);
        COMPILER_64_SET(data, 0, ((rx_check_flags &
                        BCM_SUBPORT_LINKPHY_RX_DFC_DEST_MAC_CHECK) ? 1: 0));
        soc_reg64_field_set(unit, reg, &rval64, CHECK_ENABLEf, data);
        BCM_IF_ERROR_RETURN(
            WRITE_RXLP_PORT_DFC_DESTINATION_MAC_ADDRr(unit, port, rval64));

        reg = RXLP_PORT_DFC_OPCODEr;
        regval = 0;
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_OPCODEr(unit, port, &regval));
        soc_reg_field_set(unit, reg, &regval, CHECK_ENABLEf,
            (rx_check_flags & BCM_SUBPORT_LINKPHY_RX_DFC_OPCODE_CHECK) ? 1 : 0);
        BCM_IF_ERROR_RETURN(WRITE_RXLP_PORT_DFC_OPCODEr(unit, port, regval));

        reg = RXLP_PORT_DFC_TIMEr;
        regval = 0;
        BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_TIMEr(unit, port, &regval));
        soc_reg_field_set(unit, reg, &regval, CHECK_ENABLEf,
            (rx_check_flags & BCM_SUBPORT_LINKPHY_RX_DFC_TIME_CHECK) ? 1 : 0);
        BCM_IF_ERROR_RETURN(WRITE_RXLP_PORT_DFC_TIMEr(unit, port, regval));
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_VLAN_TPID) {
        reg = RXLP_PORT_VLAN_TPIDr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            regval = 0;
            BCM_IF_ERROR_RETURN(READ_RXLP_PORT_VLAN_TPIDr(unit, port, &regval));
            soc_reg_field_set(unit, reg, &regval, VLAN_TPIDf,
                              linkphy_config->rx_vlan_tpid);
            BCM_IF_ERROR_RETURN(WRITE_RXLP_PORT_VLAN_TPIDr(unit, port, regval));
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_NEAREND_MAC) {
        rv = _bcm_linkphy_reg_mac_addr_set(unit, port,
                RXLP_PORT_NEAR_END_MAC_ADDRr, MAC_ADDRf,
                linkphy_config->rx_nearend_mac);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY RX nearend MAC set failed\n")));
            return rv;
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_FAREND_MAC) {
        rv = _bcm_linkphy_reg_mac_addr_set(unit, port,
                RXLP_PORT_FAR_END_MAC_ADDRr, MAC_ADDRf,
                linkphy_config->rx_farend_mac);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY RX farend MAC set failed\n")));
            return rv;
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_DFC_DEST_MAC) {
        rv = _bcm_linkphy_reg_mac_addr_set(unit, port,
                RXLP_PORT_DFC_DESTINATION_MAC_ADDRr, MAC_ADDRf,
                linkphy_config->rx_dfc_dest_mac);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY RX DFC dest MAC set failed\n")));
            return rv;
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_DFC_OPCODE) {
        reg = RXLP_PORT_DFC_OPCODEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            regval = 0;
            BCM_IF_ERROR_RETURN(
                READ_RXLP_PORT_DFC_OPCODEr(unit, port, &regval));
            soc_reg_field_set(unit, reg, &regval, OPCODEf,
                              linkphy_config->rx_dfc_opcode);
            BCM_IF_ERROR_RETURN(
                WRITE_RXLP_PORT_DFC_OPCODEr(unit, port, regval));
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_DFC_TIME) {
        reg = RXLP_PORT_DFC_TIMEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            regval = 0;
            BCM_IF_ERROR_RETURN(READ_RXLP_PORT_DFC_TIMEr(unit, port, &regval));
            soc_reg_field_set(unit, reg, &regval, DFC_TIMEf,
                              linkphy_config->rx_dfc_time);
            BCM_IF_ERROR_RETURN(WRITE_RXLP_PORT_DFC_TIMEr(unit, port, regval));
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_RX_DFC_MAC_TYPE) {
        reg = RXLP_PORT_DFC_MAC_TYPEr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            regval = 0;
            BCM_IF_ERROR_RETURN(
                READ_RXLP_PORT_DFC_MAC_TYPEr(unit, port, &regval));
            soc_reg_field_set(unit, reg, &regval, MAC_TYPEf,
                              linkphy_config->rx_dfc_mac_type);
            BCM_IF_ERROR_RETURN(
                WRITE_RXLP_PORT_DFC_MAC_TYPEr(unit, port, regval));
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_TX_VLAN_TPID) {
        reg = TXLP_PORT_VLAN_TPIDr;
        if (SOC_REG_PORT_VALID(unit, reg, port)) {
            regval = 0;
            BCM_IF_ERROR_RETURN(
                READ_TXLP_PORT_VLAN_TPIDr(unit, port, &regval));
            soc_reg_field_set(unit, reg, &regval, VLAN_TPIDf,
                              linkphy_config->tx_vlan_tpid);
            BCM_IF_ERROR_RETURN(
                WRITE_TXLP_PORT_VLAN_TPIDr(unit, port, regval));
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_TX_NEAREND_MAC) {
        rv = _bcm_linkphy_reg_mac_addr_set(unit, port,
                TXLP_PORT_NEAR_END_MAC_ADDRr, MAC_ADDRf,
                linkphy_config->tx_nearend_mac);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY TX nearend MAC set failed\n")));
            return rv;
        }
    }

    if (mask & BCM_SUBPORT_LINKPHY_TX_FAREND_MAC) {
        rv = _bcm_linkphy_reg_mac_addr_set(unit, port,
                TXLP_PORT_FAR_END_MAC_ADDRr, MAC_ADDRf,
                linkphy_config->tx_farend_mac);
        if (BCM_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: LinkPHY TX farend MAC set failed\n")));
            return rv;
        }
    }

    reg = TXLP_PORT_LP_MODE_CONTROLr;
    if (SOC_REG_PORT_VALID(unit, reg, port)) {
        regval = 0;
        BCM_IF_ERROR_RETURN(
            READ_TXLP_PORT_LP_MODE_CONTROLr(unit, port, &regval));
        soc_reg_field_set(unit, reg, &regval, PENULTIMATE_MODEf,
            (mask & BCM_SUBPORT_LINKPHY_TX_PENULTIMATE_MODE) ? 1 : 0);
        BCM_IF_ERROR_RETURN(
            WRITE_TXLP_PORT_LP_MODE_CONTROLr(unit, port, regval));
    }

    return rv;
}

/*
 * Function:
 *      bcm_kt2_subport_counter_init
 * Purpose:
 *      Initialize the subport counters
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_counter_init(int unit)
{
    int rv = BCM_E_NONE;
    if (soc_feature(unit, soc_feature_linkphy_coe)) {
        rv = _bcm_linkphy_subport_counter_init(unit);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport counter init failed\n")));
            return rv;
        }
    }
    if (soc_feature(unit, soc_feature_subtag_coe)) {
        
    }
    return rv;
}

/*
 * Function:
 *      bcm_kt2_subport_counter_cleanup
 * Purpose:
 *      cleanup the subport counters
 * Parameters:
 *      unit      - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_counter_cleanup(int unit)
{
    int rv = BCM_E_NONE;
    if (soc_feature(unit, soc_feature_linkphy_coe)) {
        rv = _bcm_linkphy_subport_counter_cleanup(unit);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: subport counter cleanup failed\n")));
            return rv;
        }
    }
    if (soc_feature(unit, soc_feature_subtag_coe)) {
        
    }
    return rv;
}

/*
 * Function:
 *      bcm_kt2_subport_port_stat_set
 * Purpose:
 *      Set the statistics value for a given subport port and statistics type
 * Parameters:
 *      unit      - (IN) Device Number
 *      port      - (IN) subport port
 *      stream_id - (IN) stream ID array index (if LinkPHY subport)
 *      stat_type - (IN) statistics type
 *      val       - (IN) value
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_port_stat_set(
    int unit,
    bcm_gport_t port,
    int stream_id,
    bcm_subport_stat_t stat_type,
    uint64 val)
{
    int rv = BCM_E_NONE;

    /* Parameter validation */
    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_PORT(port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: gport 0x%x is not subport port gport\n"),
                   port));
        return BCM_E_PARAM;
    }

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port)) {
        _BCM_KT2_SUBPORT_LOCK(unit);
        rv = _bcm_linkphy_subport_port_stat_set(unit,
                port, stream_id, stat_type, val);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Subport 0x%x stat set failed. rv = %d(%s)\n"),
                       port, rv, soc_errmsg(rv)));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
        _BCM_KT2_SUBPORT_UNLOCK(unit);
    } else {
        rv = BCM_E_UNAVAIL;
    }

    return rv;

}

/*
 * Function:
 *      bcm_kt2_subport_port_stat_get
 * Purpose:
 *      Get the statistics value for a given subport port and statistics type
 * Parameters:
 *      unit      - (IN) Device Number
 *      port      - (IN) subport port
 *      stream_id - (IN) stream ID array index (if LinkPHY subport)
 *      stat_type - (IN) statistics type
 *      val       - (OUT) value
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_port_stat_get(
    int unit,
    bcm_gport_t port,
    int stream_id,
    bcm_subport_stat_t stat_type,
    uint64 *val)
{
    int rv = BCM_E_NONE;

    /* Parameter validation */
    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!BCM_GPORT_IS_SUBPORT_PORT(port)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: gport 0x%x is not subport port gport\n"),
                   port));
        return BCM_E_PARAM;
    }

    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port)) {

        _BCM_KT2_SUBPORT_LOCK(unit);

        rv = _bcm_linkphy_subport_port_stat_get(unit,
                port, stream_id, stat_type, val);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Subport 0x%x stat get failed. rv = %d(%s)\n"),
                       port, rv, soc_errmsg(rv)));
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
        _BCM_KT2_SUBPORT_UNLOCK(unit);
    } else {
        rv = BCM_E_UNAVAIL;
    }

    return rv;

}

/*
 * Function:
 *      bcm_kt2_subport_egr_subtag_dot1p_map_add
 * Purpose:
 *      Map the egress SubTag CoE packet's PHB (internal priority and color) 
 *      to subport tag's Priority Code Point(PCP) and Drop Eligible(DE).
 * Parameters:
 *      unit      - (IN) Device Number
 *      map       - (IN) qos map
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_egr_subtag_dot1p_map_add(int unit,
                                         bcm_qos_map_t *map)
{
    soc_mem_t mem;
    int index;
    egr_subport_tag_dot1p_map_entry_t egr_subtag_dot1p_map;

    mem = EGR_SUBPORT_TAG_DOT1P_MAPm;

    if (!soc_feature(unit, soc_feature_subtag_coe)) {
        return BCM_E_UNAVAIL;
    }

    if (map->etag_pcp > 7 ||
        map->etag_de > 1 ||
        map->int_pri < 0 || map->int_pri > 15 ||
        ((map->color != bcmColorGreen) && 
        (map->color != bcmColorYellow) && 
        (map->color != bcmColorRed))) {
        return BCM_E_PARAM;
    }

    /* EGR_SUBPORT_TAG_DOT1P_MAPm table is indexed with
     * internal priority - 4 bits [5:2] and color - 2 bits[1:0]
     */
    index = ((map->int_pri << 2) | map->color) & soc_mem_index_max(unit, mem);

    sal_memset(&egr_subtag_dot1p_map, 0, sizeof(egr_subtag_dot1p_map));
    soc_mem_field32_set(unit, mem, &egr_subtag_dot1p_map, SUBPORT_TAG_PCPf,
                        map->etag_pcp);
    soc_mem_field32_set(unit, mem, &egr_subtag_dot1p_map, SUBPORT_TAG_DEf,
                        map->etag_de);
    SOC_IF_ERROR_RETURN(WRITE_EGR_SUBPORT_TAG_DOT1P_MAPm(unit,
                            MEM_BLOCK_ALL, index, &egr_subtag_dot1p_map));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_egr_subtag_dot1p_map_delete
 * Purpose:
 *      Delete the map the egress SubTag CoE packet's
 *      PHB (internal priority and color) 
 *      to subport tag's Priority Code Point(PCP) and Drop Eligible(DE).
 * Parameters:
 *      unit      - (IN) Device Number
 *      map       - (IN) qos map
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_kt2_subport_egr_subtag_dot1p_map_delete(int unit,
                                            bcm_qos_map_t *map)
{
    soc_mem_t mem;
    int index;
    egr_subport_tag_dot1p_map_entry_t egr_subtag_dot1p_map;

    mem = EGR_SUBPORT_TAG_DOT1P_MAPm;

    if (!soc_feature(unit, soc_feature_subtag_coe)) {
        return BCM_E_UNAVAIL;
    }

    if (map->etag_pcp > 7 ||
        map->etag_de > 1 ||
        map->int_pri < 0 || map->int_pri > 15 ||
        ((map->color != bcmColorGreen) && 
        (map->color != bcmColorYellow) && 
        (map->color != bcmColorRed))) {
        return BCM_E_PARAM;
    }

    /* EGR_SUBPORT_TAG_DOT1P_MAPm table is indexed with
     * internal priority - 4 bits [5:2] and color - 2 bits[1:0]
     */
    index = ((map->int_pri << 2) | map->color) & soc_mem_index_max(unit, mem);

    sal_memset(&egr_subtag_dot1p_map, 0, sizeof(egr_subtag_dot1p_map));
    soc_mem_field32_set(unit, mem, &egr_subtag_dot1p_map, SUBPORT_TAG_PCPf, 0);
    soc_mem_field32_set(unit, mem, &egr_subtag_dot1p_map, SUBPORT_TAG_DEf, 0);
    SOC_IF_ERROR_RETURN(WRITE_EGR_SUBPORT_TAG_DOT1P_MAPm(unit,
                            MEM_BLOCK_ALL, index, &egr_subtag_dot1p_map));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_subtag_port_tpid_set
 * Description:
 *      Set the default Tag Protocol ID for a SubTag port.
 * Parameters:
 *      unit  - (IN) Device number
 *      gport - (IN) SubTag group gport
 *      tpid  - (IN) Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */

int 
bcm_kt2_subport_subtag_port_tpid_set(int unit,
                                     bcm_gport_t gport,
                                     uint16 tpid)
{
    int rv = BCM_E_UNAVAIL;
    bcm_port_t port;
    soc_mem_t mem;
    ing_physical_port_table_entry_t ing_physical_port_entry;
    egr_physical_port_entry_t       egr_physical_port_entry;

    if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(gport)) {
        port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(gport);

        mem = ING_PHYSICAL_PORT_TABLEm;
        if (port > soc_mem_index_max(unit, mem)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Invalid port %d in subport group 0x%x\n"),
                       port, gport));
        }
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, port,
            &ing_physical_port_entry);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:ING_PHYSICAL_PORT_TABLEm read failed\n")));
            return rv;
        }
        if (soc_mem_field32_get(unit, mem, &ing_physical_port_entry,
            CASCADED_PORT_TYPEf) == _BCM_KT2_PORT_TYPE_CASCADED_SUBTAG) {
            rv = soc_mem_field32_modify(unit, mem , port, SUBPORT_TAG_TPIDf,
                tpid);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: SubTag TPID setting failed for port %d\n"),
                           port));
                return rv;
            }
        } else {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Port %d is not SubTag Cascaded\n"),
                       port));
            return BCM_E_PORT;
        }
        mem = EGR_PHYSICAL_PORTm;
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, port,
            &egr_physical_port_entry);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:EGR_PHYSICAL_PORTm read failed\n")));
            return rv;
        }
        rv = soc_mem_field32_modify(unit, mem , port, SUBPORT_TAG_TPIDf, tpid);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag TPID setting failed for egr_port %d\n"),
                       port));
            return rv;
        }
    } else {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Invalid gport 0x%x for SubTag TPID set\n"),
                   gport));
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_subport_subtag_port_tpid_get
 * Description:
 *      Retrieve the default Tag Protocol ID for a SubTag port.
 * Parameters:
 *      unit  - (IN) Device number
 *      gport - (IN)SubTag group gport
 *      tpid  - (OUT) Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */

int
bcm_kt2_subport_subtag_port_tpid_get(int unit,
                                     bcm_gport_t gport,
                                     uint16 *tpid)
{
    int rv = BCM_E_UNAVAIL;
    bcm_port_t port;
    soc_mem_t mem;
    ing_physical_port_table_entry_t ing_physical_port_entry;

    if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(gport)) {
        port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(gport);

        mem = ING_PHYSICAL_PORT_TABLEm;
        if (port > soc_mem_index_max(unit, mem)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Invalid port %d in subport group 0x%x\n"),
                       port, gport));
        }
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, port,
            &ing_physical_port_entry);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:ING_PHYSICAL_PORT_TABLEm read failed\n")));
            return rv;
        }
        if (soc_mem_field32_get(unit, mem, &ing_physical_port_entry,
            CASCADED_PORT_TYPEf) == _BCM_KT2_PORT_TYPE_CASCADED_SUBTAG) {
            *tpid = soc_mem_field32_get(unit, mem, &ing_physical_port_entry,
                        SUBPORT_TAG_TPIDf);
        } else {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Port %d is not SubTag Cascaded\n"),
                       port));
            return BCM_E_PORT;
        }
    } else {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Invalid gport 0x%x for SubTag TPID set\n"),
                   gport));
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_kt2_subport_subtag_port_tpid_delete
 * Description:
 *      Delete allowed TPID for a SubTag port.
 * Parameters:
 *      unit  - (IN) Device number
 *      gport - (IN) SubTag group gport
 *      tpid  - (IN) Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */

int
bcm_kt2_subport_subtag_port_tpid_delete(int unit,
                                        bcm_gport_t gport,
                                        uint16 tpid)
{
    int rv = BCM_E_UNAVAIL;
    bcm_port_t port;
    soc_mem_t mem;
    ing_physical_port_table_entry_t ing_physical_port_entry;
    egr_physical_port_entry_t       egr_physical_port_entry;

    if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(gport)) {
        port = _BCM_KT2_SUBPORT_GROUP_PORT_GET(gport);

        mem = ING_PHYSICAL_PORT_TABLEm;
        if (port > soc_mem_index_max(unit, mem)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Invalid port %d in subport group 0x%x\n"),
                       port, gport));
        }
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, port,
            &ing_physical_port_entry);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:ING_PHYSICAL_PORT_TABLEm read failed\n")));
            return rv;
        }
        if (soc_mem_field32_get(unit, mem, &ing_physical_port_entry,
            CASCADED_PORT_TYPEf) == _BCM_KT2_PORT_TYPE_CASCADED_SUBTAG) {
            rv = soc_mem_field32_modify(unit, mem , port, SUBPORT_TAG_TPIDf, 0);
            if (SOC_FAILURE(rv)) {
                LOG_ERROR(BSL_LS_BCM_SUBPORT,
                          (BSL_META_U(unit,
                                      "ERROR: SubTag TPID delete failed for port %d\n"),
                           port));
                return rv;
            }
        } else {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Port %d is not SubTag Cascaded\n"),
                       port));
            return BCM_E_PORT;
        }
        mem = EGR_PHYSICAL_PORTm;
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, port,
            &egr_physical_port_entry);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR:EGR_PHYSICAL_PORTm read failed\n")));
            return rv;
        }
        rv = soc_mem_field32_modify(unit, mem , port, SUBPORT_TAG_TPIDf, 0);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: SubTag TPID delete failed for egr_port %d\n"),
                       port));
            return rv;
        }
    } else {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR: Invalid gport 0x%x for SubTag TPID set\n"),
                   gport));
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}

STATIC int
    _bcm_linkphy_subport_cb(int unit,
    bcm_gport_t subport_gport, 
    bcm_subport_config_t *linkphy_config, 
    void *user_data)
{
    int stream_idx = 0;
    int rv = 0, stat = 0;
    uint64 val;

    char *stat_name[]=
        {"RxFragByte ","RxFrag     ","RxFrameByte",
         "RxFrame    ","TxFragByte ","TxFrag     ",
         "TxFrameByte","TxFrame    "};
    bcm_subport_stat_t stat_type[]={
        bcmSubportLinkPhyStatRxFragmentBytes,
        bcmSubportLinkPhyStatRxFragments, 
        bcmSubportLinkPhyStatRxFrameBytes,
        bcmSubportLinkPhyStatRxFrames,
        bcmSubportLinkPhyStatTxFragmentBytes,
        bcmSubportLinkPhyStatTxFragments,
        bcmSubportLinkPhyStatTxFrameBytes,
        bcmSubportLinkPhyStatTxFrames
    };

    if (linkphy_config->num_streams > 0) {
        for(stream_idx = 0;
            stream_idx < linkphy_config->num_streams; stream_idx++) {
            for(stat = 0; stat <= 7; stat++) {
                COMPILER_64_ZERO(val);
                rv = bcm_kt2_subport_port_stat_get(unit, subport_gport,
                         stream_idx, stat_type[stat], &val);
                if (rv < 0) {
                    LOG_CLI((BSL_META_U(unit,
                                        "Subport(0x%x) stat get failed\n"),
                             subport_gport));
                } else {
                    if (!COMPILER_64_IS_ZERO(val)) {
                        LOG_CLI((BSL_META_U(unit,
                                            "Subport(0x%x) Port(%02d) "
                                            "ExtStreamId(%04d) %s 0x%08x%08x\n"),
                                 subport_gport,
                                 BCM_GPORT_SUBPORT_PORT_GET(subport_gport),
                                 linkphy_config->stream_id_array[stream_idx],
                                 stat_name[stat],
                                 COMPILER_64_HI(val),
                                 COMPILER_64_LO(val)));
                    }
                }
            }
        }
    }
    return BCM_E_NONE;
}

STATIC void
_bcm_kt2_linkphy_counter_show(int unit,
    int block_id, int index)
{
    char *stat[] = {"RxFragByte ","RxFrag     ","RxFrameByte", "RxFrame    ",
                    "TxFragByte ","TxFrag     ","TxFrameByte", "TxFrame    "};
    int stat_type_id = 0;
    int direction = 0, pool_id = 0;
    volatile uint64 *vptr;

    BCM_LINKPHY_COUNTER_LOCK(unit);
        for (direction = 0; direction < _BCM_KT2_LINKPHY_DIRECTION_MAX;
            direction++) {
            for (pool_id = 0; pool_id < _BCM_KT2_LINKPHY_POOL_MAX; pool_id++) {
                switch (pool_id) {
                    case 0:
                        vptr = &linkphy_frag_byte_ctr[unit]
                                [direction][block_id][index];
                        break;
                    case 1:
                        vptr = &linkphy_frag_ctr[unit]
                                [direction][block_id][index];
                        break;
                    case 2:
                        vptr = &linkphy_frame_byte_ctr[unit]
                                [direction][block_id][index];
                        break;
                    case 3:
                    default:
                        vptr = &linkphy_frame_ctr[unit]
                                [direction][block_id][index];
                        break;
                }
                stat_type_id =
                    (direction*_BCM_KT2_LINKPHY_POOL_MAX) + pool_id;
                if (!(COMPILER_64_IS_ZERO(*vptr))) {
                    LOG_CLI((BSL_META_U(unit,
                                        "StreamID = %d  %s%d  %s  0x%08x%08x\n"),
                             index,
                             ((direction) ? "TXLP" : "RXLP"),
                             block_id,
                             stat[stat_type_id],
                             COMPILER_64_HI(*vptr),
                             COMPILER_64_LO(*vptr)));
                }
            }
        }
        BCM_LINKPHY_COUNTER_UNLOCK(unit);
    return;
}

int
bcm_kt2_subport_port_stat_show(int unit, uint32 flag, int sid)
{
    int block_id = 0, index = 0;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if(soc_feature(unit, soc_feature_linkphy_coe) &&
        SOC_INFO(unit).linkphy_enabled) {
        if (flag == 0) {
            _bcm_linkphy_subport_port_traverse(unit,
                _bcm_linkphy_subport_cb, NULL);
        } else { /* flag == 1*/
            if (sid != -1) {
                if (sid >= _BCM_KT2_LINKPHY_PER_PORT_STREAMS_MAX) {
                    LOG_CLI((BSL_META_U(unit,
                                        "invalid stream ID\n")));
                    return BCM_E_PARAM;
                } else {
                    for (block_id = 0;
                        block_id < _BCM_KT2_LINKPHY_BLOCK_MAX; block_id++) {
                        _bcm_kt2_linkphy_counter_show(unit, block_id, sid);
                    }
                }
            } else {
                for (index = 0;
                    index < _BCM_KT2_LINKPHY_COUNTER_MAX; index++) {
                    for (block_id = 0;
                        block_id < _BCM_KT2_LINKPHY_BLOCK_MAX; block_id++) {
                        _bcm_kt2_linkphy_counter_show(unit, block_id, index);
                    }
                }
            }
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_port_control_subtag_status_set
 * Description:
 *      Set the SubTag status for a given port.
 * Parameters:
 *      unit  - (IN) Device number
 *      port  - (IN) port
 *      value - (IN) Enable/Disable status
 * Return Value:
 *      BCM_E_XXX
 * Note: The application need to delete all subport groups/ports, if any,
 *       before disabling the SubTag CoE for given port.
 */

int bcm_kt2_port_control_subtag_status_set(int unit,
                                 bcm_port_t port, int value)
{
    int rv = BCM_E_NONE;
    int  start = 0, end = 0;
    uint32 port_type, cascaded_port_type;
    soc_mem_t mem;
    soc_info_t *si = &SOC_INFO(unit);

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!soc_feature(unit, soc_feature_subtag_coe)) {
        return BCM_E_UNAVAIL;
    }
    if (!BCM_PBMP_MEMBER(si->subtag_allowed_pbm, port)) {
        return BCM_E_PORT;
    }
    port_type = value ? _BCM_KT2_PORT_TYPE_CASCADED :
                        _BCM_KT2_PORT_TYPE_ETHERNET;
    cascaded_port_type = value ? _BCM_KT2_PORT_TYPE_CASCADED_SUBTAG : 0;

    _BCM_KT2_SUBPORT_LOCK(unit);
    mem = ING_PHYSICAL_PORT_TABLEm;
    if (SOC_MEM_FIELD_VALID(unit, mem, PORT_TYPEf)) {
        rv = soc_mem_field32_modify( unit, mem, port, PORT_TYPEf, port_type);
        if (SOC_FAILURE(rv)) {
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
    }
    if (SOC_MEM_FIELD_VALID(unit, mem, CASCADED_PORT_TYPEf)) {
        rv = soc_mem_field32_modify(unit, mem, port,
                 CASCADED_PORT_TYPEf, cascaded_port_type);
        if (SOC_FAILURE(rv)) {
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
    }

    mem = EGR_PHYSICAL_PORTm;
    if (SOC_MEM_FIELD_VALID(unit, mem, PORT_TYPEf)) {
        rv = soc_mem_field32_modify(unit, mem, port, PORT_TYPEf, port_type);
        if (SOC_FAILURE(rv)) {
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
    }
    if (SOC_MEM_FIELD_VALID(unit, mem, CASCADED_PORT_TYPEf)) {
        rv = soc_mem_field32_modify(unit, mem, port,
                 CASCADED_PORT_TYPEf, cascaded_port_type);
        if (SOC_FAILURE(rv)) {
            _BCM_KT2_SUBPORT_UNLOCK(unit);
            return rv;
        }
    }

    start = si->port_subport_base[port] - KT2_MIN_SUBPORT_INDEX;
    end = start + si->port_num_subport[port] - 1;
    if (value) {
        BCM_PBMP_PORT_ADD(si->subtag_pbm, port);
        BCM_PBMP_PORT_ADD(si->subtag.bitmap, port);
        SOC_IF_ERROR_RETURN
            (soc_kt2_cosq_port_coe_linkphy_status_set(unit, port, 1));
        SOC_IF_ERROR_RETURN
            (soc_kt2_cosq_s1_range_set(unit, port, start, end, 0));
        SOC_IF_ERROR_RETURN
            (soc_kt2_cosq_repl_map_set(unit, port, start, end, 1));
    } else {
        if (BCM_PBMP_MEMBER(si->subtag_pbm, port)) {
            BCM_PBMP_PORT_REMOVE(si->subtag_pbm, port);
            BCM_PBMP_PORT_REMOVE(si->subtag.bitmap, port);
            SOC_IF_ERROR_RETURN
                (soc_kt2_cosq_port_coe_linkphy_status_set(unit, port, 0));
            SOC_IF_ERROR_RETURN
                (soc_kt2_cosq_s1_range_set(unit, port, 0, 0, 0));
            SOC_IF_ERROR_RETURN
                (soc_kt2_cosq_repl_map_set(unit, port, start, end, 0));
        }
    }
    _BCM_KT2_SUBPORT_UNLOCK(unit);
    return rv;
}
/*
 * Function:
 *      bcm_kt2_port_control_subtag_status_get
 * Description:
 *      Get the SubTag status for a given port.
 * Parameters:
 *      unit  - (IN) Device number
 *      port  - (IN) port
 *      value - (OUT) Enable/Disable status
 * Return Value:
 *      BCM_E_XXX
 */

int bcm_kt2_port_control_subtag_status_get(int unit,
                                 bcm_port_t port, int *value)
{
    int rv = BCM_E_NONE;
    soc_mem_t mem;
    soc_info_t *si = &SOC_INFO(unit);
    ing_physical_port_table_entry_t ing_physical_port_entry;

    _BCM_KT2_SUBPORT_CHECK_INIT(unit);

    if (!soc_feature(unit, soc_feature_subtag_coe)) {
        return BCM_E_UNAVAIL;
    }
    if (!BCM_PBMP_MEMBER(si->subtag_allowed_pbm, port)) {
        return BCM_E_PORT;
    }

    _BCM_KT2_SUBPORT_LOCK(unit);

    *value = 0;
    mem = ING_PHYSICAL_PORT_TABLEm;
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, port,
            &ing_physical_port_entry);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_SUBPORT,
                  (BSL_META_U(unit,
                              "ERROR:ING_PHYSICAL_PORT_TABLEm read failed\n")));
        _BCM_KT2_SUBPORT_UNLOCK(unit);
        return rv;
    }
    if ((soc_mem_field32_get(unit, mem, &ing_physical_port_entry,
            CASCADED_PORT_TYPEf) == _BCM_KT2_PORT_TYPE_CASCADED_SUBTAG) &&
        (soc_mem_field32_get(unit, mem, &ing_physical_port_entry,
            PORT_TYPEf) == _BCM_KT2_PORT_TYPE_CASCADED)) {
        *value = 1;
    }

    _BCM_KT2_SUBPORT_UNLOCK(unit);

    return rv;
}

int
_bcm_kt2_modport_is_local_coe_subport(int unit, int mod, bcm_port_t port,
                                      int *is_local_subport)
{
    int pp_port = 0, mymodid = 0;

    *is_local_subport = 0;

    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mymodid));
        /* For LinkPHY/CoE pp_ports mymodid+1 is used */
        if (mod == (mymodid + 1)) {
            BCM_IF_ERROR_RETURN(_bcm_kt2_modport_to_pp_port_get(unit,
                                    mod, port, &pp_port));
            /* For KT2, pp_port 42 to 169 are used for LinkPHY/CoE subport.*/
             if ((pp_port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) &&
                 (pp_port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) {
                 *is_local_subport = 1;
             }
        }
    }
    return BCM_E_NONE;
}


#ifdef BCM_WARM_BOOT_SUPPORT
/* 
 * Function:
 *     _bcm_kt2_subport_reinit
 * Purpose:
 *     Reinit for warm boot.
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_kt2_subport_reinit(int unit)
{
    
    return BCM_E_NONE; 
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *      _bcm_kt2_subport_sw_dump
 * Purpose:
 *      Print Subport module s/w state
 * Returns: 
 *      void
 */
void
_bcm_kt2_subport_sw_dump(int unit)
{
    int idx;

    LOG_CLI((BSL_META_U(unit,
                        "Subport Groups:\n")));
    for(idx = 0; idx < _BCM_KT2_SUBPORT_GROUP_MAX; idx++) {
        if(_BCM_KT2_SUBPORT_GROUP_USED_GET(unit, idx)) {
            LOG_CLI((BSL_META_U(unit,
                                "%d "), idx));
        }
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n----\n")));

    LOG_CLI((BSL_META_U(unit,
                        "Subport stream ids and pp_port used:\n")));
    

}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else  /* BCM_KATANA2_SUPPORT */
int bcm_esw_kt2_subport_not_empty;
#endif /* BCM_KATANA2_SUPPORT */


