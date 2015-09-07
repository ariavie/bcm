/*
 * $Id: pbsmh.c 1.26 Broadcom SDK $
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
 * Utility routines for managing pbsmh headers
 * These are mainly used for debug purposes
 */

#include <assert.h>
#include <sal/core/libc.h>

#include <soc/types.h>
#include <soc/drv.h>
#include <soc/pbsmh.h>
#include <soc/cm.h>
#include <soc/dcb.h>
#include <soc/debug.h>

#ifdef BCM_XGS3_SWITCH_SUPPORT
static char *soc_pbsmh_field_names[] = {
    /* NOTE: strings must match soc_pbsmh_field_t */
    "start",
    "src_mod",
    "dst_port",
    "cos",
    "pri",
    "l3pbm_sel",
    "l2pbm_sel",
    "unicast",
    "tx_ts",
    "spid_override",
    "spid",
    "spap",
    "queue_num",
    "osts",
    "its_sign",
    "hdr_offset",
    "regen_udp_checksum",
    "int_pri",
    "nlf_port",
    "lm_ctr_index",
    "oam_replacement_type",
    "oam_replacement_offset",
    "ep_cpu_reasons",
    "header_type",
    "cell_error",
	"ipcf_ptr",
    NULL
};

soc_pbsmh_field_t
soc_pbsmh_name_to_field(int unit, char *name)
{
    soc_pbsmh_field_t           f;

    assert(COUNTOF(soc_pbsmh_field_names) - 1 == PBSMH_COUNT);

    COMPILER_REFERENCE(unit);

    for (f = 0; soc_pbsmh_field_names[f] != NULL; f++) {
        if (sal_strcmp(name, soc_pbsmh_field_names[f]) == 0) {
            return f;
        }
    }

    return PBSMH_invalid;
}

char *
soc_pbsmh_field_to_name(int unit, soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    assert(COUNTOF(soc_pbsmh_field_names) == PBSMH_COUNT);

    if (field < 0 || field >= PBSMH_COUNT) {
        return "??";
    } else {
        return soc_pbsmh_field_names[field];
    }
}

static void
soc_pbsmh_v1_field_set(int unit, soc_pbsmh_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:   mh->start = val;
                        mh->_rsvd0 = 0;
                        mh->_rsvd1 = 0;
                        mh->_rsvd2 = 0;
                        break;
    case PBSMH_src_mod: mh->src_mod = val; break;
    case PBSMH_dst_port:mh->dst_port = val;break;
    case PBSMH_cos:     mh->cos = val;break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%s val=0x%x\n",
                     unit, soc_pbsmh_field_to_name(unit, field), val);
        break;
    }
}

static uint32
soc_pbsmh_v1_field_get(int unit, soc_pbsmh_hdr_t *mh, soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:       return mh->start;
    case PBSMH_src_mod:     return mh->src_mod; 
    case PBSMH_dst_port:    return mh->dst_port;
    case PBSMH_cos:         return mh->cos;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}

static void
soc_pbsmh_v2_field_set(int unit, soc_pbsmh_v2_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->start = val;
                                mh->_rsvd0 = 0;
                                mh->_rsvd1 = 0;
                                mh->_rsvd2 = 0;
                                break;
    case PBSMH_src_mod:         mh->src_mod = val; break;
    case PBSMH_dst_port:        mh->dst_port = val; break;
    case PBSMH_cos:             mh->cos = val; break;
    case PBSMH_pri:             mh->pri = val; break;
    case PBSMH_l3pbm_sel:       mh->l3pbm_sel = val; break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%d val=0x%x\n",
                     unit, field, val);
        break;
    }
}

static uint32
soc_pbsmh_v2_field_get(int unit,
                       soc_pbsmh_v2_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:       return mh->start;
    case PBSMH_src_mod:     return mh->src_mod; 
    case PBSMH_dst_port:    return mh->dst_port;
    case PBSMH_cos:         return mh->cos;
    case PBSMH_pri:         return mh->pri;
    case PBSMH_l3pbm_sel:   return mh->l3pbm_sel;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}

static void
soc_pbsmh_v3_field_set(int unit, soc_pbsmh_v3_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->start = val;
                                mh->_rsvd0 = 0;
                                mh->_rsvd1 = 0;
                                mh->_rsvd2 = 0;
                                break;
    case PBSMH_src_mod:         mh->src_mod_hi = (val >> 4); 
                                mh->src_mod_lo = (val & 0xf); 
                                break;
    case PBSMH_dst_port:        mh->dst_port = val; break;
    case PBSMH_cos:             mh->cos = val; break;
    case PBSMH_pri:             mh->pri = val; break;
    case PBSMH_l3pbm_sel:       mh->l3pbm_sel = val; break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%d val=0x%x\n",
                     unit, field, val);
        break;
    }
}

static uint32
soc_pbsmh_v3_field_get(int unit,
                       soc_pbsmh_v3_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:       return mh->start;
    case PBSMH_src_mod:     return ((mh->src_mod_hi << 4) | (mh->src_mod_lo)); 
    case PBSMH_dst_port:    return mh->dst_port;
    case PBSMH_cos:         return mh->cos;
    case PBSMH_pri:         return mh->pri;
    case PBSMH_l3pbm_sel:   return mh->l3pbm_sel;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}

static void
soc_pbsmh_v4_field_set(int unit, soc_pbsmh_v4_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->start = val;
                                mh->_rsvd0 = 0;
                                mh->_rsvd1 = 0;
                                mh->_rsvd2 = 0;
                                break;
    case PBSMH_src_mod:         mh->src_mod_hi = (val >> 7); 
                                mh->src_mod_lo = (val & 0x7f); 
                                break;
    case PBSMH_dst_port:        mh->dst_port = val; break;
    case PBSMH_cos:             mh->cos = val; break;
    case PBSMH_pri:             mh->pri_hi = (val >> 3); 
                                mh->pri_lo = (val & 0x7); 
                                break;
    case PBSMH_l3pbm_sel:       mh->l3pbm_sel = val; break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%d val=0x%x\n",
                     unit, field, val);
        break;
    }
}

static uint32
soc_pbsmh_v4_field_get(int unit,
                       soc_pbsmh_v4_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:       return mh->start;
    case PBSMH_src_mod:     return ((mh->src_mod_hi << 7) | (mh->src_mod_lo)); 
    case PBSMH_dst_port:    return mh->dst_port;
    case PBSMH_cos:         return mh->cos;
    case PBSMH_pri:         return ((mh->pri_hi << 3) | (mh->pri_lo));
    case PBSMH_l3pbm_sel:   return mh->l3pbm_sel;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}

static void
soc_pbsmh_v5_field_set(int unit, soc_pbsmh_v5_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->start = val;
                                mh->_rsvd0 = 0;
                                mh->_rsvd1 = 0;
                                mh->_rsvd2 = 0;
                                mh->_rsvd3 = 0;
                                mh->_rsvd4 = 0;
                                mh->_rsvd5 = 0;
                                mh->_rsvd6 = 0;
                                mh->_rsvd7 = 0;
                                break;
    case PBSMH_src_mod:         mh->src_mod = val; break;
    case PBSMH_dst_port:        mh->dst_port = val; break;
    case PBSMH_cos:             mh->cos = val; break;
    case PBSMH_pri:             mh->input_pri = val; break;
    case PBSMH_l3pbm_sel:       mh->set_l3bm = val; break;
    case PBSMH_l2pbm_sel:       mh->set_l2bm = val; break;
    case PBSMH_unicast:         mh->unicast = val; break;
    case PBSMH_tx_ts:           mh->tx_ts = val; break;
    case PBSMH_spid_override:   mh->spid_override = val; break;
    case PBSMH_spid:            mh->spid = val; break;
    case PBSMH_spap:            mh->spap = val; break;
    case PBSMH_queue_num:       mh->queue_num = val; break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%d val=0x%x\n",
                     unit, field, val);
        break;
    }
}

static uint32
soc_pbsmh_v5_field_get(int unit,
                       soc_pbsmh_v5_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:         return mh->start;
    case PBSMH_src_mod:       return mh->src_mod;
    case PBSMH_dst_port:      return mh->dst_port;
    case PBSMH_cos:           return mh->cos;
    case PBSMH_pri:           return mh->input_pri;
    case PBSMH_l3pbm_sel:     return mh->set_l3bm;
    case PBSMH_l2pbm_sel:     return mh->set_l2bm;
    case PBSMH_unicast:       return mh->unicast;
    case PBSMH_tx_ts:         return mh->tx_ts;
    case PBSMH_spid_override: return mh->spid_override;
    case PBSMH_spid:          return mh->spid;
    case PBSMH_spap:          return mh->spap;
    case PBSMH_queue_num:     return mh->queue_num;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}


static void
soc_pbsmh_v6_field_set(int unit, soc_pbsmh_v6_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->start = val;
                                mh->_rsvd0 = 0;
                                mh->_rsvd1 = 0;
                                mh->_rsvd2 = 0;
                                mh->_rsvd3 = 0;
                                mh->_rsvd4 = 0;
                                mh->_rsvd5 = 0;
                                break;
    case PBSMH_src_mod:         mh->src_mod = val; break;
    case PBSMH_dst_port:        mh->dst_port = val; break;
    case PBSMH_cos:             mh->cos = val; break;
    case PBSMH_pri:             mh->input_pri = val; break;
    case PBSMH_l3pbm_sel:       mh->set_l3bm = val; break;
    case PBSMH_l2pbm_sel:       mh->set_l2bm = val; break;
    case PBSMH_unicast:         mh->unicast = val; break;
    case PBSMH_tx_ts:           mh->tx_ts = val; break;
    case PBSMH_spid_override:   mh->spid_override = val; break;
    case PBSMH_spid:            mh->spid = val; break;
    case PBSMH_spap:            mh->spap = val; break;
    case PBSMH_queue_num:
        mh->queue_num_1 = val & 0x3;
        mh->queue_num_2 = (val >> 2) & 0xff;
        mh->queue_num_3 = (val >> 10) & 0x3;
        break;
    case PBSMH_osts:            mh->osts = val; break;
    case PBSMH_its_sign:        mh->its_sign = val; break;
    case PBSMH_hdr_offset:
        mh->hdr_offset_1 = val & 0x1f;
        mh->hdr_offset_2 = (val >> 5) & 0x7;
        break;
    case PBSMH_regen_udp_checksum: mh->regen_upd_chksum = val; break;
    case PBSMH_ipcf_ptr:        mh->ipcf_ptr = val; break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%d val=0x%x\n",
                     unit, field, val);
        break;
    }
}

static uint32
soc_pbsmh_v6_field_get(int unit,
                       soc_pbsmh_v6_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:         return mh->start;
    case PBSMH_src_mod:       return mh->src_mod;
    case PBSMH_dst_port:      return mh->dst_port;
    case PBSMH_cos:           return mh->cos;
    case PBSMH_pri:           return mh->input_pri;
    case PBSMH_l3pbm_sel:     return mh->set_l3bm;
    case PBSMH_l2pbm_sel:     return mh->set_l2bm;
    case PBSMH_unicast:       return mh->unicast;
    case PBSMH_tx_ts:         return mh->tx_ts;
    case PBSMH_spid_override: return mh->spid_override;
    case PBSMH_spid:          return mh->spid;
    case PBSMH_spap:          return mh->spap;
    case PBSMH_queue_num:
        return (mh->queue_num_1 | (mh->queue_num_2 << 2)  | (mh->queue_num_3 << 10));
    case PBSMH_osts:          return mh->osts;
    case PBSMH_its_sign:      return mh->its_sign;
    case PBSMH_hdr_offset:
        return (mh->hdr_offset_1 | (mh->hdr_offset_2 << 5));
    case PBSMH_regen_udp_checksum: return mh->regen_upd_chksum;
    case PBSMH_ipcf_ptr:      return mh->ipcf_ptr;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}

static void
soc_pbsmh_v7_field_set(int unit, soc_pbsmh_v7_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->overlay1.start = val;
              mh->overlay1.header_type = PBS_MH_V7_HDR_TYPE_FROM_CPU;
                                mh->overlay1.cell_error = 0;
                                mh->overlay1.oam_replacement_offset = 0;
                                mh->overlay1.oam_replacement_type = 0;
                                mh->overlay2._rsvd3 = 0;
                                break;
    case PBSMH_src_mod:         mh->overlay1.src_mod = val; break;
    case PBSMH_dst_port:        mh->overlay1.dst_port = val; break;
    case PBSMH_cos:             mh->overlay2.cos = val; break;
    case PBSMH_pri:             mh->overlay1.input_pri = val; break;
    case PBSMH_l3pbm_sel:       mh->overlay1.set_l3bm = val; break;
    case PBSMH_l2pbm_sel:       mh->overlay1.set_l2bm = val; break;
    case PBSMH_unicast:         mh->overlay2.unicast = val; break;
    case PBSMH_tx_ts:           mh->overlay1.tx_ts = val; break;
    case PBSMH_spid_override:   mh->overlay2.spid_override = val; break;
    case PBSMH_spid:            mh->overlay2.spid = val; break;
    case PBSMH_spap:            mh->overlay2.spap = val; break;
    case PBSMH_queue_num:
        mh->overlay1.queue_num_1 = val & 0xff;
        mh->overlay1.queue_num_2 = (val >> 8) & 0x3;
        if (SOC_DCB_TYPE(unit) == 26) {
            mh->overlay1.queue_num_3 = (val >> 10) & 0x3;
        }
        break;
    case PBSMH_osts:            mh->overlay1.osts = val; break;
    case PBSMH_its_sign:        mh->overlay1.its_sign = val; break;
    case PBSMH_hdr_offset:      mh->overlay1.hdr_offset = val; break;
    case PBSMH_regen_udp_checksum: mh->overlay1.regen_udp_checksum = val; break;
    case PBSMH_int_pri:         mh->overlay1.int_pri = val; break;
    case PBSMH_nlf_port:        mh->overlay1.nlf_port = val; break;
    case PBSMH_lm_ctr_index:
        mh->overlay2.lm_counter_index_1 = val & 0xff;
        mh->overlay2.lm_counter_index_2 = (val >> 8) & 0xff;
        mh->overlay2._rsvd3 = 0;
        break;
    case PBSMH_oam_replacement_type:
        mh->overlay1.oam_replacement_type = val;
        break;
    case PBSMH_oam_replacement_offset:
        mh->overlay1.oam_replacement_offset = val;
        break;
    case PBSMH_ep_cpu_reasons: 
        mh->overlay1._rsvd5 = 0;
        mh->overlay1.ep_cpu_reason_code_1 = val & 0xff;
        mh->overlay1.ep_cpu_reason_code_2 = (val >> 8) & 0xff;
        mh->overlay1.ep_cpu_reason_code_3 = (val >> 16) & 0xf;
        break;
    case PBSMH_header_type:     mh->overlay1.header_type = val; break;
    case PBSMH_cell_error:      mh->overlay1.cell_error = val; break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%d val=0x%x\n",
                     unit, field, val);
        break;
    }
}

static uint32
soc_pbsmh_v7_field_get(int unit,
                       soc_pbsmh_v7_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:         return mh->overlay1.start;
    case PBSMH_src_mod:       return mh->overlay1.src_mod;
    case PBSMH_dst_port:      return mh->overlay1.dst_port;
    case PBSMH_cos:           return mh->overlay2.cos;
    case PBSMH_pri:           return mh->overlay1.input_pri;
    case PBSMH_l3pbm_sel:     return mh->overlay1.set_l3bm;
    case PBSMH_l2pbm_sel:     return mh->overlay1.set_l2bm;
    case PBSMH_unicast:       return mh->overlay2.unicast;
    case PBSMH_tx_ts:         return mh->overlay1.tx_ts;
    case PBSMH_spid_override: return mh->overlay2.spid_override;
    case PBSMH_spid:          return mh->overlay2.spid;
    case PBSMH_spap:          return mh->overlay2.spap;
    case PBSMH_queue_num:
        if (SOC_DCB_TYPE(unit) == 26) {
            return (mh->overlay1.queue_num_1 |
                    (mh->overlay1.queue_num_2 << 8) |
                    (mh->overlay1.queue_num_3 << 10));
        } else {
            return (mh->overlay1.queue_num_1 |
                    (mh->overlay1.queue_num_2 << 8));
        }
    case PBSMH_osts:          return mh->overlay1.osts;
    case PBSMH_its_sign:      return mh->overlay1.its_sign;
    case PBSMH_hdr_offset:    return mh->overlay1.hdr_offset;
    case PBSMH_regen_udp_checksum: return mh->overlay1.regen_udp_checksum;
    case PBSMH_int_pri:       return mh->overlay1.int_pri;
    case PBSMH_nlf_port:      return mh->overlay1.nlf_port;
    case PBSMH_lm_ctr_index:
        return (mh->overlay2.lm_counter_index_1 |
                (mh->overlay2.lm_counter_index_2 << 8));
    case PBSMH_oam_replacement_type:
        return mh->overlay1.oam_replacement_type;
    case PBSMH_oam_replacement_offset:
        return mh->overlay1.oam_replacement_offset;
    case PBSMH_ep_cpu_reasons: 
        return (mh->overlay1.ep_cpu_reason_code_1 |
                (mh->overlay1.ep_cpu_reason_code_2 << 8) |
                (mh->overlay1.ep_cpu_reason_code_3 << 16));
    case PBSMH_header_type:   return mh->overlay1.header_type;
    case PBSMH_cell_error:    return mh->overlay1.cell_error;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}

static void
soc_pbsmh_v8_field_set(int unit, soc_pbsmh_v8_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->overlay1.start = val;
                                mh->overlay1.header_type = PBS_MH_V7_HDR_TYPE_FROM_CPU;
                                mh->overlay1.cell_error = 0;
                                mh->overlay1.oam_replacement_offset = 0;
                                mh->overlay1.oam_replacement_type = 0;
                                break;
    case PBSMH_src_mod:         mh->overlay1.src_mod = val; break;
    case PBSMH_dst_port:        mh->overlay1.dst_pp_port = val; break;
    case PBSMH_cos:             mh->overlay1.cos = val; break;
    case PBSMH_pri:             mh->overlay1.input_pri = val; break;
    case PBSMH_l3pbm_sel:       mh->overlay1.set_l3bm = val; break;
    case PBSMH_l2pbm_sel:       mh->overlay1.set_l2bm = val; break;
    case PBSMH_unicast:         mh->overlay1.unicast = val; break;
    case PBSMH_tx_ts:           mh->overlay1.tx_ts = val; break;
    case PBSMH_spid_override:   mh->overlay1.spid_override = val; break;
    case PBSMH_spid:            mh->overlay1.spid = val; break;
    case PBSMH_spap:            mh->overlay1.spap = val; break;
    case PBSMH_queue_num:
        mh->overlay1.queue_num_1 = val & 0xff;
        mh->overlay1.queue_num_2 = (val >> 8) & 0xf;
        break;
    case PBSMH_osts:            mh->overlay1.osts = val; break;
    case PBSMH_its_sign:        mh->overlay1.its_sign = val; break;
    case PBSMH_hdr_offset:      mh->overlay1.hdr_offset = val; break;
    case PBSMH_regen_udp_checksum: mh->overlay1.regen_udp_checksum = val; break;
    case PBSMH_int_pri:         mh->overlay4.int_pri = val; break;
    case PBSMH_lm_ctr_index:
        mh->overlay1.lm_counter_index_1 = val & 0x3f;
        mh->overlay1.lm_counter_index_2 = (val >> 6) & 0x3f;
        break;
    case PBSMH_oam_replacement_type:
        mh->overlay1.oam_replacement_type = val;
        break;
    case PBSMH_oam_replacement_offset:
        mh->overlay1.oam_replacement_offset = val;
        break;
    case PBSMH_ep_cpu_reasons: 
        mh->overlay2.ep_cpu_reason_code_1 = val & 0x7f;
        mh->overlay2.ep_cpu_reason_code_2 = (val >> 7) & 0xff;
        mh->overlay2.ep_cpu_reason_code_3 = (val >> 15) & 0x1f;
        break;
    case PBSMH_header_type:     mh->overlay1.header_type = val; break;
    case PBSMH_cell_error:      mh->overlay1.cell_error = val; break;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_set: unit %d: Unknown pbsmh field=%d val=0x%x\n",
                     unit, field, val);
        break;
    }
}

static uint32
soc_pbsmh_v8_field_get(int unit,
                       soc_pbsmh_v8_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:         return mh->overlay1.start;
    case PBSMH_src_mod:       return mh->overlay1.src_mod;
    case PBSMH_dst_port:      return mh->overlay1.dst_pp_port;
    case PBSMH_cos:           return mh->overlay2.cos;
    case PBSMH_pri:           return mh->overlay1.input_pri;
    case PBSMH_l3pbm_sel:     return mh->overlay1.set_l3bm;
    case PBSMH_l2pbm_sel:     return mh->overlay1.set_l2bm;
    case PBSMH_unicast:       return mh->overlay1.unicast;
    case PBSMH_tx_ts:         return mh->overlay1.tx_ts;
    case PBSMH_spid_override: return mh->overlay1.spid_override;
    case PBSMH_spid:          return mh->overlay1.spid;
    case PBSMH_spap:          return mh->overlay1.spap;
    case PBSMH_queue_num:
            return (mh->overlay1.queue_num_1 |
                    (mh->overlay1.queue_num_2 << 8));
    case PBSMH_osts:          return mh->overlay1.osts;
    case PBSMH_its_sign:      return mh->overlay1.its_sign;
    case PBSMH_hdr_offset:    return mh->overlay1.hdr_offset;
    case PBSMH_regen_udp_checksum: return mh->overlay1.regen_udp_checksum;
    case PBSMH_int_pri:       return mh->overlay4.int_pri;
    case PBSMH_lm_ctr_index:
        return (mh->overlay1.lm_counter_index_1 |
                (mh->overlay1.lm_counter_index_2 << 6));
    case PBSMH_oam_replacement_type:
        return mh->overlay1.oam_replacement_type;
    case PBSMH_oam_replacement_offset:
        return mh->overlay1.oam_replacement_offset;
    case PBSMH_ep_cpu_reasons: 
        return (mh->overlay2.ep_cpu_reason_code_1 |
                (mh->overlay2.ep_cpu_reason_code_2 << 7) |
                (mh->overlay2.ep_cpu_reason_code_3 << 15));
    case PBSMH_header_type:   return mh->overlay1.header_type;
    case PBSMH_cell_error:    return mh->overlay1.cell_error;
    default:
        soc_cm_debug(DK_WARN,
                     "pbsmh_get: unit %d: Unknown pbsmh field=%d\n",
                     unit, field);
        return 0;
    }
}

void
soc_pbsmh_field_set(int unit, soc_pbsmh_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    switch(SOC_DCB_TYPE(unit)) {
    case 9:
    case 10:
    case 13:
        soc_pbsmh_v1_field_set(unit, mh, field, val);
        break;
    case 11:
    case 12:
    case 15:
    case 17:
    case 18:
        {
            soc_pbsmh_v2_hdr_t *mhv2 = (soc_pbsmh_v2_hdr_t *)mh;
            soc_pbsmh_v2_field_set(unit, mhv2, field, val);
            break;
        }
    case 14:
    case 19:
    case 20:
        {
            soc_pbsmh_v3_hdr_t *mhv3 = (soc_pbsmh_v3_hdr_t *)mh;
            soc_pbsmh_v3_field_set(unit, mhv3, field, val);
            break;
        }
    case 16:
    case 22:
        {
            soc_pbsmh_v4_hdr_t *mhv4 = (soc_pbsmh_v4_hdr_t *)mh;
            soc_pbsmh_v4_field_set(unit, mhv4, field, val);
            break;
        }
    case 21:
        {
            soc_pbsmh_v5_hdr_t *mhv5 = (soc_pbsmh_v5_hdr_t *)mh;
            soc_pbsmh_v5_field_set(unit, mhv5, field, val);
            break;
        }
    case 24:
        {
            soc_pbsmh_v6_hdr_t *mhv6 = (soc_pbsmh_v6_hdr_t *)mh;
            soc_pbsmh_v6_field_set(unit, mhv6, field, val);
            break;
        }
    case 23:
    case 26:
    case 30:
        {
            soc_pbsmh_v7_hdr_t *mhv7 = (soc_pbsmh_v7_hdr_t *)mh;
            soc_pbsmh_v7_field_set(unit, mhv7, field, val);
            break;
        }
    case 29:
        {
            soc_pbsmh_v8_hdr_t *mhv8 = (soc_pbsmh_v8_hdr_t *)mh;
            soc_pbsmh_v8_field_set(unit, mhv8, field, val);
            break;
        }
    default:
        break;
    }
}

uint32
soc_pbsmh_field_get(int unit, soc_pbsmh_hdr_t *mh, soc_pbsmh_field_t field)
{
    switch(SOC_DCB_TYPE(unit)) {
    case 9:
    case 10:
    case 13:
        return soc_pbsmh_v1_field_get(unit, mh, field);
        break;
    case 11:
    case 12:
    case 15:
    case 17:
    case 18:
        {
            soc_pbsmh_v2_hdr_t *mhv2 = (soc_pbsmh_v2_hdr_t *)mh;
            return soc_pbsmh_v2_field_get(unit, mhv2, field);
            break;
        }
    case 14:
    case 19:
    case 20:
        {
            soc_pbsmh_v3_hdr_t *mhv3 = (soc_pbsmh_v3_hdr_t *)mh;
            return soc_pbsmh_v3_field_get(unit, mhv3, field);
            break;
        }
    case 16:
    case 22:
        {
            soc_pbsmh_v4_hdr_t *mhv4 = (soc_pbsmh_v4_hdr_t *)mh;
            return soc_pbsmh_v4_field_get(unit, mhv4, field);
            break;
        }
    case 21:
        {
            soc_pbsmh_v5_hdr_t *mhv5 = (soc_pbsmh_v5_hdr_t *)mh;
            return soc_pbsmh_v5_field_get(unit, mhv5, field);
            break;
        }
    case 24:
        {
            soc_pbsmh_v6_hdr_t *mhv6 = (soc_pbsmh_v6_hdr_t *)mh;
            return soc_pbsmh_v6_field_get(unit, mhv6, field);
            break;
        }
    case 23:
    case 26:
    case 30:
        {
            soc_pbsmh_v7_hdr_t *mhv7 = (soc_pbsmh_v7_hdr_t *)mh;
            return soc_pbsmh_v7_field_get(unit, mhv7, field);
            break;
        }
    case 29:
        {
            soc_pbsmh_v8_hdr_t *mhv8 = (soc_pbsmh_v8_hdr_t *)mh;
            return soc_pbsmh_v8_field_get(unit, mhv8, field);
            break;
        }
    default:
        break;
    }
    return 0;
}

void
soc_pbsmh_dump(int unit, char *pfx, soc_pbsmh_hdr_t *mh)
{
    switch(SOC_DCB_TYPE(unit)) {
    default:
        soc_cm_print("%s<START=0x%02x COS=%d PRI=%d L3PBM_SEL=%d "
                     "SRC_MODID=%d DST_PORT=%d>\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_start),
                     soc_pbsmh_field_get(unit, mh, PBSMH_cos),
                     soc_pbsmh_field_get(unit, mh, PBSMH_pri),
                     soc_pbsmh_field_get(unit, mh, PBSMH_l3pbm_sel),
                     soc_pbsmh_field_get(unit, mh, PBSMH_src_mod),
                     soc_pbsmh_field_get(unit, mh, PBSMH_dst_port));
        break;
    case 21:
        soc_cm_print("%s0x%02x%02x%02x%02x "
                     "<START=0x%02x>\n",
                     pfx,
                     ((uint8 *)mh)[0],
                     ((uint8 *)mh)[1],
                     ((uint8 *)mh)[2],
                     ((uint8 *)mh)[3],
                     soc_pbsmh_field_get(unit, mh, PBSMH_start));
        soc_cm_print("%s0x%02x%02x%02x%02x "
                     "<TX_TX=%d SPID_OVERRIDE=%d SPID=%d SPAP=%d\n",
                     pfx,
                     ((uint8 *)mh)[4],
                     ((uint8 *)mh)[5],
                     ((uint8 *)mh)[6],
                     ((uint8 *)mh)[7],
                     soc_pbsmh_field_get(unit, mh, PBSMH_tx_ts),
                     soc_pbsmh_field_get(unit, mh, PBSMH_spid_override),
                     soc_pbsmh_field_get(unit, mh, PBSMH_spid),
                     soc_pbsmh_field_get(unit, mh, PBSMH_spap));
        soc_cm_print("%s            "
                     "SET_L3BM=%d SET_L2BM=%d UNICAST=%d SRC_MODID=%d>\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_l3pbm_sel),
                     soc_pbsmh_field_get(unit, mh, PBSMH_l2pbm_sel),
                     soc_pbsmh_field_get(unit, mh, PBSMH_unicast),
                     soc_pbsmh_field_get(unit, mh, PBSMH_src_mod));
        soc_cm_print("%s0x%02x%02x%02x%02x "
                     "<INPUT_PRI=%d QUEUE_NUM=%d COS=%d LOCAL_DEST_PORT=%d>\n",
                     pfx,
                     ((uint8 *)mh)[8],
                     ((uint8 *)mh)[9],
                     ((uint8 *)mh)[10],
                     ((uint8 *)mh)[11],
                     soc_pbsmh_field_get(unit, mh, PBSMH_pri),
                     soc_pbsmh_field_get(unit, mh, PBSMH_queue_num),
                     soc_pbsmh_field_get(unit, mh, PBSMH_cos),
                     soc_pbsmh_field_get(unit, mh, PBSMH_dst_port));
        break;
    case 23:
        soc_cm_print("%s0x%02x%02x%02x%02x "
                     "<START=0x%02x HEADER_TYPE=0x%02x\n",
                     pfx,
                     ((uint8 *)mh)[0],
                     ((uint8 *)mh)[1],
                     ((uint8 *)mh)[2],
                     ((uint8 *)mh)[3],
                     soc_pbsmh_field_get(unit, mh, PBSMH_start),
                     soc_pbsmh_field_get(unit, mh, PBSMH_header_type));
        soc_cm_print("%s            "
                     "LM_COUNTER_INDEX=0x%02x EP_CPU_REASON_CODE=0x%03x>\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_lm_ctr_index),
                     soc_pbsmh_field_get(unit, mh, PBSMH_ep_cpu_reasons));
        soc_cm_print("%s0x%02x%02x%02x%02x "
                     "<OAM_REPLACEMENT_OFFSET=0x%02x "
                     "OAM_REPLACMENT_TYPE=%d\n",
                     pfx,
                     ((uint8 *)mh)[4],
                     ((uint8 *)mh)[5],
                     ((uint8 *)mh)[6],
                     ((uint8 *)mh)[7],
                     soc_pbsmh_field_get(unit, mh,
                                         PBSMH_oam_replacement_offset),
                     soc_pbsmh_field_get(unit, mh,
                                         PBSMH_oam_replacement_type));
        soc_cm_print("%s            "
                     "OSTS=%d REGEN_UDP_CHECKSUM=%d ITS_SIGN=%d TX_TS=%d\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_osts),
                     soc_pbsmh_field_get(unit, mh, PBSMH_regen_udp_checksum),
                     soc_pbsmh_field_get(unit, mh, PBSMH_its_sign),
                     soc_pbsmh_field_get(unit, mh, PBSMH_tx_ts));
        soc_cm_print("%s            "
                     "SET_L3BM=%d TS_HDR_OFFSET=0x%02x "
                     "SET_L2BM=%d\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_l3pbm_sel),
                     soc_pbsmh_field_get(unit, mh, PBSMH_hdr_offset),
                     soc_pbsmh_field_get(unit, mh, PBSMH_l2pbm_sel));
        soc_cm_print("%s            "
                     "LOCAL_DEST_PORT=%d CELL_ERROR=%d>\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_dst_port),
                     soc_pbsmh_field_get(unit, mh, PBSMH_cell_error));

        soc_cm_print("%s0x%02x%02x%02x%02x "
                     "<INPUT_PRI=%d COS=%d SPID_OVERRIDE=%d "
                     "SPID=%d SPAP=%d\n",
                     pfx,
                     ((uint8 *)mh)[8],
                     ((uint8 *)mh)[9],
                     ((uint8 *)mh)[10],
                     ((uint8 *)mh)[11],
                     soc_pbsmh_field_get(unit, mh, PBSMH_pri),
                     soc_pbsmh_field_get(unit, mh, PBSMH_cos),
                     soc_pbsmh_field_get(unit, mh, PBSMH_spid_override),
                     soc_pbsmh_field_get(unit, mh, PBSMH_spid),
                     soc_pbsmh_field_get(unit, mh, PBSMH_spap));
        soc_cm_print("%s            "
                     "UNICAST=%d QUEUE_NUM=%d SRC_MODID=%d\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_unicast),
                     soc_pbsmh_field_get(unit, mh, PBSMH_queue_num),
                     soc_pbsmh_field_get(unit, mh, PBSMH_src_mod));
        soc_cm_print("%s            "
                     "NLF_PORT_NUMBER=%d>\n",
                     pfx,
                     soc_pbsmh_field_get(unit, mh, PBSMH_nlf_port));
        break;
    }
}

#endif /* BCM_XGS3_SWITCH_SUPPORT */
