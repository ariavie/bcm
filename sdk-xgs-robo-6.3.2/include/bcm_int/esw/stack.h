/*
 * $Id: stack.h 1.16 Broadcom SDK $
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
 * File:	stack_int.h
 */

#ifndef _BCM_INT_STACK_H_
#define _BCM_INT_STACK_H_

/*
 * Stack tag structure
 */
typedef struct stk_tag_s {
#if defined(LE_HOST)
    uint32 rsvp:2;
    uint32 stk_modid:5;
    uint32 ed:1;
    uint32 em:1;
    uint32 md:1;
    uint32 mirr:1;
    uint32 pfm:2;
    uint32 dst_rtag:3;
    uint32 dst_tgid:3;
    uint32 dst_t:1;
    uint32 src_rtag:3;
    uint32 src_tgid:3;
    uint32 src_t:1;
    uint32 stk_cnt:5;
#else
    uint32 stk_cnt:5;
    uint32 src_t:1;
    uint32 src_tgid:3;
    uint32 src_rtag:3;
    uint32 dst_t:1;
    uint32 dst_tgid:3;
    uint32 dst_rtag:3;
    uint32 pfm:2;
    uint32 mirr:1;
    uint32 md:1;
    uint32 em:1;
    uint32 ed:1;
    uint32 stk_modid:5;
    uint32 rsvp:2;
#endif
} stk_tag_t;

#define STK_TAG_LEN 4

#define STACK_TAG(_scnt, _st, _stgid, _srtag, _dt, _dtgid, \
                 _drtag, _pfm, _m, _md, _em, _ed, _smod)  \
        ((_scnt & 0x1f) << 27 |\
        (_st & 0x1) << 26     |\
        (_stgid & 0x7) << 23  |\
        (_srtag & 0x7) << 20  |\
        (_dt & 0x1) << 19     |\
        (_dtgid & 0x7) << 16  |\
        (_drtag & 0x7) << 13  |\
        (_pfm & 0x3) << 11    |\
        (_m & 0x1) << 10      |\
        (_md & 0x1) << 9      |\
        (_em & 0x1) << 8      |\
        (_ed & 0x1) << 7      |\
        (_smod & 0x1f) << 2)

#define STK_MODID_GET(_tag) \
        ((_tag >> 2) & 0x1f)

#define STK_MODID_SET(_ptr_tag, _smod) \
        (*_ptr_tag = ((*_ptr_tag & ~(0x1f << 2)) | ((_smod & 0x1f) << 2)))

#define STK_CNT_GET(_tag) \
        ((_tag >> 27) & 0x1f)

#define STK_CNT_SET(_ptr_tag, _cnt) \
        (*_ptr_tag = ((*_ptr_tag & ~(0x1f << 27)) | ((_cnt & 0x1f) << 27)))

#define STK_PFM_GET(_tag) \
        ((_tag >> 11) & 0x3)

#define STK_PFM_SET(_ptr_tag, _pfm) \
        (*_ptr_tag = ((*_ptr_tag & ~(0x3 << 11)) | ((_pfm & 0x3) << 11)))

enum {
    _BCM_STK_PORT_MODPORT_OP_SET = 1,
    _BCM_STK_PORT_MODPORT_OP_ADD,
    _BCM_STK_PORT_MODPORT_OP_DELETE,
    _BCM_STK_PORT_MODPORT_OP_COUNT
};

enum {
    _BCM_STK_PORT_MODPORT_DMVOQ_GRP_OP_SET = 1,
    _BCM_STK_PORT_MODPORT_DMVOQ_GRP_OP_GET,
    _BCM_STK_PORT_MODPORT_DMVOQ_GRP_OP_COUNT
};

extern int _bcm_esw_stk_detach(int unit);

typedef void (*_bcm_stk_modid_chg_cb_t)(
    int unit, 
    int  modid,
    void *userdata);

extern int _bcm_esw_stk_modid_chg_cb_register(int unit, 
                                              _bcm_stk_modid_chg_cb_t cb, 
                                              void *userdata);
extern int _bcm_esw_stk_modid_chg_cb_unregister(int unit, 
                                              _bcm_stk_modid_chg_cb_t cb); 
extern int _bcm_esw_stk_modmap_map(int unit, int setget,
                   bcm_module_t mod_in, bcm_port_t port_in,
                   bcm_module_t *mod_out, bcm_port_t *port_out);

extern int _bcm_esw_tr_trunk_override_ucast_set(int unit, bcm_port_t port,
                                      bcm_trunk_t hgtid, int modid, int enable);

extern int _bcm_esw_tr_trunk_override_ucast_get(int unit, bcm_port_t port,
                                     bcm_trunk_t hgtid, int modid, int *enable); 
extern int _bcm_esw_src_modid_base_index_get(int unit, bcm_module_t modid,
                                     int *base_index);
extern int _bcm_esw_src_modid_port_get(int unit, int stm_inx, bcm_module_t *modid,
                                     int *port);
extern int _bcm_esw_src_mod_port_table_index_get(int unit, bcm_module_t modid,
                                     bcm_port_t port, int *index);

extern int
bcm_stk_modport_voq_cosq_profile_get(int unit, bcm_port_t ing_port, 
                               bcm_module_t dest_modid, int *profile_id);

extern int
bcm_stk_modport_voq_cosq_profile_set(int unit, bcm_port_t ing_port, 
                               bcm_module_t dest_modid, int profile_id);

#if defined(BCM_WARM_BOOT_SUPPORT) 
extern int bcm_esw_reload_stk_my_modid_get(int unit);
extern int _bcm_esw_stk_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_stk_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif	/* !_BCM_INT_STACK_H_ */
