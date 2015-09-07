/*
 * $Id: mmu_config.c,v 1.12 Broadcom SDK $
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
 * CMAC driver
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/boot.h>
#include <shared/bsl.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/ll.h>
#include <soc/debug.h>
#include <soc/mmu_config.h>

#if defined(BCM_TRIUMPH3_SUPPORT)

STATIC void
_soc_mmu_cfg_property_get(int unit, soc_port_t port, const char *obj,
                          int index, const char *attr, int *setting)
{
    char suffix;

    if (port < 0) {
        *setting = soc_property_obj_attr_get
            (unit, spn_BUF, obj, index, attr, 0, &suffix, *setting);
    } else {
        *setting = soc_property_port_obj_attr_get
            (unit, port, spn_BUF, obj, index, attr, 0, &suffix, *setting);
    }
}

STATIC void
_soc_mmu_cfg_property_get_cells(int unit, soc_port_t port, const char *obj,
                                int index, const char *attr, int allow_dynamic,
                                int *setting, int byte_per_cell)
{
    int val;
    char suffix;

    /* scale up by 10 during calculation */
    if (*setting & _MMU_CFG_BUF_DYNAMIC_FLAG) {
        suffix =  '%';
        val = *setting & ~_MMU_CFG_BUF_DYNAMIC_FLAG;
        val = (val + 1) * 125; /* 0:125 (12.5%), ... 7:1000 (100%) */

    } else {
        suffix = '\0';
        val = *setting * 10;
    }

    if (port < 0) {
        val = soc_property_obj_attr_get
            (unit, spn_BUF, obj, index, attr, 1, &suffix, val);
    } else {
        val = soc_property_port_obj_attr_get
            (unit, port, spn_BUF, obj, index, attr, 1, &suffix, val);
    }

    if (val < 0) { /* treat negative number as zero */
        val = 0;
    }

    /* scale down by 10 after calculation */
    if (allow_dynamic && suffix == '%') {
        val = val > 1000 ? 7 : (val - 1) / 125; /* each unit represent 12.5% */
        val |= _MMU_CFG_BUF_DYNAMIC_FLAG;
    } else {
        val /= 10;
        switch (suffix) {
        case 'B': /* byte */
        case 'b':
            val = _MMU_CFG_MMU_BYTES_TO_CELLS(val, byte_per_cell);
            break;
        case 'K': /* kilobyte */
        case 'k':
            val = _MMU_CFG_MMU_BYTES_TO_CELLS(val * 1024, byte_per_cell);
            break;
        case 'M': /* megabyte */
        case 'm':
            val = _MMU_CFG_MMU_BYTES_TO_CELLS(val * 1048576, byte_per_cell);
            break;
        default:
            break;
        }
    }

    *setting = val;
}

STATIC void
_soc_mmu_cfg_property_get_percentage_x100(int unit, soc_port_t port,
                                          const char *obj, int index,
                                          const char *attr, int *setting)
{
    int val;
    char suffix;

    if (*setting & _MMU_CFG_BUF_PERCENT_FLAG) {
        suffix =  '%';
        val = *setting & ~_MMU_CFG_BUF_PERCENT_FLAG;
    } else {
        suffix =  '\0';
        val = *setting;
    }

    if (port < 0) {
        val = soc_property_obj_attr_get
            (unit, spn_BUF, obj, index, attr, 2, &suffix, val);
    } else {
        val = soc_property_port_obj_attr_get
            (unit, port, spn_BUF, obj, index, attr, 2, &suffix, val);
    }

    if (suffix == '%') {
        if (val < 0) { /* treat negative number as zero */
            val = 0;
        } else if (val > 10000) { /* treat number larger than 100% as 100% */
            val = 10000;
        }
        *setting = val | _MMU_CFG_BUF_PERCENT_FLAG;
    } else {
        *setting = val;
    }
}

STATIC void
_soc_mmu_cfg_property_get_scale(int unit, soc_port_t port,
                                const char *obj, int index,
                                const char *attr, int *setting)
{
    int val, alpha;
    char suffix;

    alpha = 15625;  /* 1/64 = 0.015625, scale up by 1000000 (2**6) */
    val = *setting < 0 ? -1 : alpha << *setting;

    if (port < 0) {
        val = soc_property_obj_attr_get
            (unit, spn_BUF, obj, index, attr, 6, &suffix, val);
    } else {
        val = soc_property_port_obj_attr_get
            (unit, port, spn_BUF, obj, index, attr, 6, &suffix, val);
    }

    if (val < 0) { /* ignore negative number */
        *setting = -1;
    } else {
        for (*setting = 0; *setting < 9; (*setting)++) {
            if (val <= (alpha << *setting)) {
                break;
            }
        }
    }
}

void
_soc_mmu_cfg_buf_read(int unit, _soc_mmu_cfg_buf_t *buf,
                                 _soc_mmu_device_info_t *devcfg)
{
    soc_info_t *si;
    _soc_mmu_cfg_buf_pool_t *buf_pool;
    _soc_mmu_cfg_buf_port_t *buf_port;
    _soc_mmu_cfg_buf_port_pool_t *buf_port_pool;
    _soc_mmu_cfg_buf_prigroup_t *buf_prigroup;
    _soc_mmu_cfg_buf_queue_t *buf_queue;
    int port, idx, count;
    _soc_mmu_cfg_buf_qgroup_t *queue_group;
    char name[80];
    int values[64];

    si = &SOC_INFO(unit);

    _soc_mmu_cfg_property_get_cells
        (unit, -1, spn_DEVICE, -1, spn_HEADROOM, FALSE, &buf->headroom,
         devcfg->mmu_cell_size);

    for (idx = 0; idx < _SOC_MMU_CFG_DEV_POOL_NUM(devcfg); idx++) {
        buf_pool = &buf->pools[idx];

        _soc_mmu_cfg_property_get_percentage_x100
            (unit, -1, spn_POOL, idx, spn_SIZE, &buf_pool->size);

        _soc_mmu_cfg_property_get_percentage_x100
            (unit, -1, spn_POOL, idx, spn_YELLOW_SIZE, &buf_pool->yellow_size);

        _soc_mmu_cfg_property_get_percentage_x100
            (unit, -1, spn_POOL, idx, spn_RED_SIZE, &buf_pool->red_size);
    }

    for (idx = 0; idx < SOC_MMU_CFG_QGROUP_MAX; idx++) {
        queue_group = &buf->qgroups[idx];

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_QGROUP, idx, spn_GUARANTEE, FALSE,
             &queue_group->guarantee, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get
            (unit, -1, spn_QGROUP, idx, spn_DISCARD_ENABLE,
             &queue_group->discard_enable);

        _soc_mmu_cfg_property_get_scale
            (unit, -1, spn_QGROUP, idx, spn_POOL_SCALE,
             &queue_group->pool_scale);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_QGROUP, idx, spn_POOL_LIMIT, FALSE,
             &queue_group->pool_limit, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_QGROUP, idx, spn_POOL_RESUME, FALSE,
             &queue_group->pool_resume, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_QGROUP, idx, spn_YELLOW_LIMIT, TRUE,
             &queue_group->yellow_limit, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_QGROUP, idx, spn_RED_LIMIT, TRUE,
             &queue_group->red_limit, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_QGROUP, idx, spn_YELLOW_RESUME, FALSE,
             &queue_group->yellow_resume, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_QGROUP, idx, spn_RED_RESUME, FALSE,
             &queue_group->red_resume, devcfg->mmu_cell_size);

    }

    PBMP_ALL_ITER(unit, port) {
        buf_port = &buf->ports[port];

        for (idx = 0; idx < _SOC_MMU_CFG_DEV_POOL_NUM(devcfg); idx++) {
            buf_port_pool = &buf_port->pools[idx];
            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_INGPORTPOOL, idx, spn_GUARANTEE, FALSE,
                 &buf_port_pool->guarantee, devcfg->mmu_cell_size);
            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_INGPORTPOOL, idx, spn_POOL_LIMIT, FALSE,
                 &buf_port_pool->pool_limit, devcfg->mmu_cell_size);
            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_INGPORTPOOL, idx, spn_POOL_RESUME, FALSE,
                 &buf_port_pool->pool_resume, devcfg->mmu_cell_size);
        }

        /* priority group */
        for (idx = 0; idx < _SOC_MMU_CFG_DEV_PG_NUM(devcfg); idx++) {
            buf_prigroup = &buf_port->prigroups[idx];

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_PRIGROUP, idx, spn_GUARANTEE, FALSE,
                 &buf_prigroup->guarantee, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_PRIGROUP, idx, spn_HEADROOM, FALSE,
                 &buf_prigroup->headroom, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_PRIGROUP, idx, spn_USER_DELAY,
                 &buf_prigroup->user_delay);

            _soc_mmu_cfg_property_get
                (unit, port, spn_PRIGROUP, idx, spn_SWITCH_DELAY,
                 &buf_prigroup->switch_delay);

            _soc_mmu_cfg_property_get
                (unit, port, spn_PRIGROUP, idx, spn_PKT_SIZE,
                 &buf_prigroup->pkt_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_PRIGROUP, idx, spn_DEVICE_HEADROOM_ENABLE,
                 &buf_prigroup->device_headroom_enable);

            _soc_mmu_cfg_property_get_scale
                (unit, port, spn_PRIGROUP, idx, spn_POOL_SCALE,
                 &buf_prigroup->pool_scale);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_PRIGROUP, idx, spn_POOL_LIMIT, FALSE,
                 &buf_prigroup->pool_limit, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_PRIGROUP, idx, spn_POOL_RESUME, FALSE,
                 &buf_prigroup->pool_resume, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_PRIGROUP, idx, spn_POOL_FLOOR, FALSE,
                 &buf_prigroup->pool_floor, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_PRIGROUP, idx, spn_FLOW_CONTROL_ENABLE,
                 &buf_prigroup->flow_control_enable);
        }

        /* multicast queue */
        for (idx = 0; idx < si->port_num_cosq[port]; idx++) {
            buf_queue = &buf_port->queues[idx];

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_GUARANTEE, FALSE,
                 &buf_queue->guarantee, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_MQUEUE, idx, spn_DISCARD_ENABLE,
                 &buf_queue->discard_enable);

            _soc_mmu_cfg_property_get_scale
                (unit, port, spn_MQUEUE, idx, spn_POOL_SCALE,
                 &buf_queue->pool_scale);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_POOL_LIMIT, FALSE,
                 &buf_queue->pool_limit, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_POOL_RESUME, FALSE,
                 &buf_queue->pool_resume, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_MQUEUE, idx, spn_COLOR_DISCARD_ENABLE,
                 &buf_queue->color_discard_enable);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_RED_LIMIT, TRUE,
                 &buf_queue->red_limit, devcfg->mmu_cell_size);
          
             _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_YELLOW_LIMIT, TRUE,
                 &buf_queue->yellow_limit, devcfg->mmu_cell_size);


            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_RED_RESUME, TRUE,
                 &buf_queue->red_resume, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_YELLOW_RESUME, TRUE,
                 &buf_queue->yellow_resume, devcfg->mmu_cell_size);


            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_MQUEUE, idx, spn_POOL, FALSE,
                 &buf_queue->pool_idx, devcfg->mmu_cell_size);
        }

        /* unicast queue */
        for (idx = 0; idx < si->port_num_uc_cosq[port]; idx++) {
            buf_queue = &buf_port->queues[si->port_num_cosq[port] + idx];

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_GUARANTEE, FALSE,
                 &buf_queue->guarantee, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_QUEUE, idx, spn_DISCARD_ENABLE,
                 &buf_queue->discard_enable);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_POOL, FALSE,
                 &buf_queue->pool_idx, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_scale
                (unit, port, spn_QUEUE, idx, spn_POOL_SCALE,
                 &buf_queue->pool_scale);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_POOL_LIMIT, FALSE,
                 &buf_queue->pool_limit, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_POOL_RESUME, FALSE,
                 &buf_queue->pool_resume, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_QUEUE, idx, spn_COLOR_DISCARD_ENABLE,
                 &buf_queue->color_discard_enable);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_YELLOW_LIMIT, TRUE,
                 &buf_queue->yellow_limit, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_RED_LIMIT, TRUE,
                 &buf_queue->red_limit, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_YELLOW_RESUME, FALSE,
                 &buf_queue->yellow_resume, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_RED_RESUME, FALSE,
                 &buf_queue->red_resume, devcfg->mmu_cell_size);

            _soc_mmu_cfg_property_get
                (unit, port, spn_QUEUE, idx, spn_QGROUP_ID,
                 &buf_queue->qgroup_id);

            _soc_mmu_cfg_property_get_cells
                (unit, port, spn_QUEUE, idx, spn_QGROUP_GUARANTEE_ENABLE, FALSE,
                 &buf_queue->qgroup_min_enable, devcfg->mmu_cell_size);
        }

        /* internal priority to priority group mapping */
        sal_sprintf(name, "%s.%s.%s.%s",
                    spn_BUF, spn_MAP, spn_PRI, spn_PRIGROUP);
        (void)soc_property_port_get_csv(unit, port, name, 16,
                                        buf_port->pri_to_prigroup);

        /* priority group to pool mapping */
        for (idx = 0; idx < _SOC_MMU_CFG_DEV_PG_NUM(devcfg); idx++) {
            values[idx] = buf_port->prigroups[idx].pool_idx;
        }

        sal_sprintf(name, "%s.%s.%s.%s",
                    spn_BUF, spn_MAP, spn_PRIGROUP, spn_POOL);
        count = soc_property_port_get_csv(unit, port, name, 
                                _SOC_MMU_CFG_DEV_PG_NUM(devcfg), values);
        for (idx = 0; idx < count; idx++) {
            buf_port->prigroups[idx].pool_idx = values[idx];
        }
    }

    /* virtual queue profiles */
    for (idx = 0; idx < SOC_MMU_CFG_EQUEUE_MAX; idx++) {
        buf_queue = &buf->equeues[idx];

       _soc_mmu_cfg_property_get 
            (unit, -1, spn_EQUEUE, idx, spn_NUMQ, &buf_queue->numq);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_GUARANTEE, FALSE,
             &buf_queue->guarantee, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get
            (unit, -1, spn_EQUEUE, idx, spn_DISCARD_ENABLE,
             &buf_queue->discard_enable);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_POOL, FALSE,
             &buf_queue->pool_idx, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_scale
            (unit, -1, spn_EQUEUE, idx, spn_POOL_SCALE,
             &buf_queue->pool_scale);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_POOL_LIMIT, FALSE,
             &buf_queue->pool_limit, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_POOL_RESUME, FALSE,
             &buf_queue->pool_resume, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get
            (unit, -1, spn_EQUEUE, idx, spn_COLOR_DISCARD_ENABLE,
             &buf_queue->color_discard_enable);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_YELLOW_LIMIT, TRUE,
             &buf_queue->yellow_limit, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_RED_LIMIT, TRUE,
             &buf_queue->red_limit, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_YELLOW_RESUME, FALSE,
             &buf_queue->yellow_resume, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_RED_RESUME, FALSE,
             &buf_queue->red_resume, devcfg->mmu_cell_size);

        _soc_mmu_cfg_property_get
            (unit, -1, spn_EQUEUE, idx, spn_QGROUP_ID,
             &buf_queue->qgroup_id);

        _soc_mmu_cfg_property_get_cells
            (unit, -1, spn_EQUEUE, idx, spn_QGROUP_GUARANTEE_ENABLE, FALSE,
             &buf_queue->qgroup_min_enable, devcfg->mmu_cell_size);
    }
}

int
_soc_mmu_cfg_buf_calculate(int unit, _soc_mmu_cfg_buf_t *buf,
                           _soc_mmu_device_info_t *devcfg)
{
    soc_info_t *si;
    _soc_mmu_cfg_buf_pool_t *buf_pool;
    _soc_mmu_cfg_buf_queue_t *buf_queue;
    _soc_mmu_cfg_buf_port_t *buf_port;
    _soc_mmu_cfg_buf_qgroup_t *queue_group;
    _soc_mmu_cfg_buf_prigroup_t *buf_prigroup;
    _soc_mmu_cfg_buf_port_pool_t *buf_port_pool;
    _soc_mmu_cfg_buf_mcengine_queue_t *buf_rqe_queue;
    int port, idx, headroom[SOC_MMU_CFG_SERVICE_POOL_MAX];
    int min_guarentees[SOC_MMU_CFG_SERVICE_POOL_MAX];
    int queue_min[SOC_MMU_CFG_SERVICE_POOL_MAX];
    int pool, qgidx, total_usable_buffer;
    uint8 *qgpid;
    int mcq_entry_reserved[SOC_MMU_CFG_SERVICE_POOL_MAX];

    for (idx = 0; idx < SOC_MMU_CFG_SERVICE_POOL_MAX; idx++) {
        queue_min[idx] = 0;
        min_guarentees[idx] = 0;
        headroom[idx] = 0;
        mcq_entry_reserved[idx] = 0;
    }

    si = &SOC_INFO(unit);

    total_usable_buffer = _SOC_MMU_CFG_MMU_TOTAL_CELLS(devcfg);

    for (idx = 0; idx < _SOC_MMU_CFG_DEV_POOL_NUM(devcfg); idx++) {
        buf_pool = &buf->pools[idx];
        if ((buf_pool->size & ~_MMU_CFG_BUF_PERCENT_FLAG) == 0) {
            continue;
        }

        if (buf_pool->size & _MMU_CFG_BUF_PERCENT_FLAG) {
            buf_pool->total = ((buf_pool->size & ~_MMU_CFG_BUF_PERCENT_FLAG) *
                                total_usable_buffer) / 10000;
            if (devcfg->flags & SOC_MMU_CFG_F_EGR_MCQ_ENTRY) {
                buf_pool->total_mcq_entry = 
                    ((buf_pool->size & ~_MMU_CFG_BUF_PERCENT_FLAG) *
                                             devcfg->total_mcq_entry) / 10000;
            }
        }
    }

    PBMP_ALL_ITER(unit, port) {
        buf_port = &buf->ports[port];
        for (idx = 0; idx < _SOC_MMU_CFG_DEV_PG_NUM(devcfg); idx++) {
            buf_prigroup = &buf_port->prigroups[idx];
            if (buf_prigroup->user_delay != -1 &&
                buf_prigroup->switch_delay != -1) {
                /*
                 * number of max leftever cells =
                 *   port speed (megabits per sec) * 10**6 (megabits to bit) *
                 *   delay (nsec) / 10**9 (nsecs to second) /
                 *   8 (bits per byte) /
                 *   (IPG (12 bytes) + preamble (8 bytes) +
                 *    worse case packet size (145 bytes)) *
                 *   worse case packet size (2 cells)
                 * heandroom =
                 *   mtu (cells) + number of leftover cells
                 */
                buf_prigroup->headroom = buf_prigroup->pkt_size +
                    si->port_speed_max[port] *
                    (buf_prigroup->user_delay + buf_prigroup->switch_delay) *
                    2 / (8 * (12 + 8 + 145) * 1000);
            }
        }
    }

    PBMP_ALL_ITER(unit, port) {
        buf_port = &buf->ports[port];
        for (idx = 0; idx < _SOC_MMU_CFG_DEV_PG_NUM(devcfg); idx++) {
            buf_prigroup = &buf_port->prigroups[idx];
            headroom[buf_prigroup->pool_idx] += buf_prigroup->headroom;
            min_guarentees[buf_prigroup->pool_idx] += buf_prigroup->guarantee;
        }

        /* port mins */
        for (idx = 0; idx < _SOC_MMU_CFG_DEV_POOL_NUM(devcfg); idx++) {
            buf_port_pool = &buf_port->pools[idx];
            min_guarentees[idx] += buf_port_pool->guarantee;
        }

        for (idx = 0; idx < si->port_num_cosq[port]; idx++) {
            buf_queue = &buf_port->queues[idx];
            if ((buf_queue->qgroup_id == -1) ||
                (buf_queue->qgroup_min_enable == 0)) {
                queue_min[buf_queue->pool_idx] += buf_queue->guarantee;
            }

            mcq_entry_reserved[buf_queue->pool_idx] += 
                                        buf_queue->mcq_entry_guarantee;
        }

        for (idx = 0; idx < si->port_num_uc_cosq[port]; idx++) {
            buf_queue = &buf_port->queues[si->port_num_cosq[port] + idx];
            if ((buf_queue->qgroup_id == -1) ||
                (buf_queue->qgroup_min_enable == 0)) {
                queue_min[buf_queue->pool_idx] += buf_queue->guarantee;
            }
        }
    }

    if (devcfg->flags & SOC_MMU_CFG_F_RQE) {
        for (idx = 0; idx < 11; idx++) {
            buf_rqe_queue = &buf->rqe_queues[idx];
            queue_min[buf_rqe_queue->pool_idx] += buf_rqe_queue->guarantee;
        }
    }

    qgpid = sal_alloc(sizeof(uint8)*SOC_MMU_CFG_QGROUP_MAX, "queue2grp");
    if (!qgpid) {
        return SOC_E_MEMORY;
    }

    for (idx = 0; idx < SOC_MMU_CFG_QGROUP_MAX; idx++) {
        qgpid[idx] = 0;
    }

    /* Check all the queues belonging to queue group use same service pool. */
    pool = -1;

    PBMP_ALL_ITER(unit, port) {
        buf_port = &buf->ports[port];
        for (idx = 0; idx < si->port_num_cosq[port]; idx++) {
            buf_queue = &buf_port->queues[idx];

            if (buf_queue->qgroup_id == -1) {
                continue;
            }

            qgidx = buf_queue->qgroup_id;
            /* deduct qgroup guarantee if not done so far */
            if (qgpid[qgidx] == 0) {
                queue_group = &buf->qgroups[qgidx];
                queue_min[buf_queue->pool_idx] += queue_group->guarantee;
            }
            qgpid[qgidx] |= 1 << buf_queue->pool_idx;
        }

        for (idx = 0; idx < si->port_num_uc_cosq[port]; idx++) {
            buf_queue = &buf_port->queues[si->port_num_cosq[port] + idx];

            if (buf_queue->qgroup_id == -1) {
                continue;
            }
            qgidx = buf_queue->qgroup_id;
            if (qgpid[qgidx] == 0) {
                queue_group = &buf->qgroups[qgidx];
                queue_min[buf_queue->pool_idx] += queue_group->guarantee;
            }
            qgpid[qgidx] |= 1 << buf_queue->pool_idx;
        }
    }
    
    for (idx = 0; idx < SOC_MMU_CFG_EQUEUE_MAX; idx++) {
        buf_queue = &buf->equeues[idx];
        if (buf_queue->numq <= 0) {
            continue;
        }
        
        pool = buf_queue->pool_idx;
        if (buf_queue->qgroup_id == -1) {
            queue_min[pool] += buf_queue->guarantee * buf_queue->numq;
        } else {
            qgidx = buf_queue->qgroup_id;
            if (qgpid[qgidx] == 0) {
                queue_group = &buf->qgroups[qgidx];
                queue_min[buf_queue->pool_idx] += queue_group->guarantee;
            }
            qgpid[qgidx] |= 1 << buf_queue->pool_idx;
        }
    }

    for (idx = 0; idx < SOC_MMU_CFG_QGROUP_MAX; idx++) {
        if (qgpid[idx] & (qgpid[idx] - 1)) {
            LOG_CLI((BSL_META_U(unit,
                                "Queue belonging to same group use different Pools !!")));
            sal_free(qgpid);
            return SOC_E_PARAM;
        }
    }
    
    sal_free(qgpid);

    for (idx = 0; idx < _SOC_MMU_CFG_DEV_POOL_NUM(devcfg); idx++) {
        buf_pool = &buf->pools[idx];
        
        buf_pool->queue_guarantee = queue_min[idx];
        buf_pool->prigroup_headroom = headroom[idx];
        buf_pool->prigroup_guarantee = min_guarentees[idx];
        buf_pool->mcq_entry_reserved = mcq_entry_reserved[idx];
#if 0
        buf_pool->total -= (buf_pool->queue_guarantee + 
                            buf_pool->prigroup_headroom +
                            buf_pool->prigroup_guarantee);
#endif
    }

    if (bsl_check(bslLayerSoc, bslSourceCommon, bslSeverityVerbose, unit)) {
        LOG_CLI((BSL_META_U(unit,
                            "MMU buffer usage:\n")));
        LOG_CLI((BSL_META_U(unit,
                            "  Global headroom: %d\n"), buf->headroom));
        for (idx = 0; idx < _SOC_MMU_CFG_DEV_POOL_NUM(devcfg); idx++) {
            buf_pool = &buf->pools[idx];
            if ((buf_pool->size & ~_MMU_CFG_BUF_PERCENT_FLAG) == 0) {
                continue;
            }
            LOG_CLI((BSL_META_U(unit,
                                "  Pool %d total prigroup guarantee: %d\n"),
                     idx, buf_pool->prigroup_guarantee));
            LOG_CLI((BSL_META_U(unit,
                                "  Pool %d total prigroup headroom: %d\n"),
                     idx, buf_pool->prigroup_headroom));
            LOG_CLI((BSL_META_U(unit,
                                "  Pool %d total queue guarantee: %d\n"),
                     idx, buf_pool->queue_guarantee));
            LOG_CLI((BSL_META_U(unit,
                                "  Pool %d total mcq entry reserved: %d\n"),
                     idx, buf_pool->mcq_entry_reserved));
        }
    }

    return SOC_E_NONE;
}

int
_soc_mmu_cfg_buf_check(int unit, _soc_mmu_cfg_buf_t *buf, 
                        _soc_mmu_device_info_t *devcfg)
{
    _soc_mmu_cfg_buf_pool_t *buf_pool;
    _soc_mmu_cfg_buf_port_t *buf_port;
    _soc_mmu_cfg_buf_prigroup_t *buf_prigroup;
    int yellow_cells, red_cells;
    int port, dft_pool, idx;
    uint32 pool_map;

    SOC_IF_ERROR_RETURN(_soc_mmu_cfg_buf_calculate(unit, buf, devcfg));

    dft_pool =0;
    pool_map = 0;
    for (idx = 0; idx < _SOC_MMU_CFG_DEV_POOL_NUM(devcfg); idx++) {
        buf_pool = &buf->pools[idx];
        if ((buf_pool->size & ~_MMU_CFG_BUF_PERCENT_FLAG) == 0) {
            continue;
        }

        if (pool_map == 0) {
            dft_pool = idx;
        }
        pool_map |= 1 << idx;

        if (buf_pool->total <= 0) {
            LOG_CLI((BSL_META_U(unit,
                                "Pool %d has no shared space after "
                                "deducting guaranteed !!"), idx));
            return SOC_E_FAIL;
        }

        if (buf_pool->yellow_size & _MMU_CFG_BUF_PERCENT_FLAG) {
            yellow_cells = (buf_pool->yellow_size & ~_MMU_CFG_BUF_PERCENT_FLAG) *
                buf_pool->total / 10000;
        } else {
            yellow_cells = buf_pool->yellow_size;
        }
        if (buf_pool->red_size & _MMU_CFG_BUF_PERCENT_FLAG) {
            red_cells = (buf_pool->red_size & ~_MMU_CFG_BUF_PERCENT_FLAG) *
                buf_pool->total / 10000;
        } else {
            red_cells = buf_pool->red_size;
        }

        if (yellow_cells > red_cells) {
            LOG_CLI((BSL_META_U(unit,
                                "MMU config pool %d: Yellow cells offset is higher "
                                "than red cells\n"), idx));
        }
        if (red_cells > buf_pool->total) {
            LOG_CLI((BSL_META_U(unit,
                                "MMU config pool %d: Red cells offset is higher "
                                "than pool shared cells\n"), idx));
        }
    }

    PBMP_ALL_ITER(unit, port) {
        buf_port = &buf->ports[port];

        /* internal priority to priority group mapping */
        for (idx = 0; idx < 16; idx++) {
            if (buf_port->pri_to_prigroup[idx] < 0 ||
                buf_port->pri_to_prigroup[idx] >= _SOC_MMU_CFG_DEV_PG_NUM(devcfg)) {
                LOG_CLI((BSL_META_U(unit,
                                    "MMU config port %d: Invalid prigroup value (%d) "
                                    "for internal priority %d\n"),
                         port, buf_port->pri_to_prigroup[idx], idx));
                buf_port->pri_to_prigroup[idx] = 0; /* use prigroup 0 */
            }
        }

        /* priority group to pool mapping */
        for (idx = 0; idx < _SOC_MMU_CFG_DEV_PG_NUM(devcfg); idx++) {
            buf_prigroup = &buf_port->prigroups[idx];
            if (buf_prigroup->pool_idx < 0 ||
                buf_prigroup->pool_idx >= _SOC_MMU_CFG_DEV_POOL_NUM(devcfg)) {
                LOG_CLI((BSL_META_U(unit,
                                    "MMU config port %d prigroup %d: "
                                    "Invalid pool value (%d)\n"),
                         port, idx, buf_prigroup->pool_idx));
                /* use first non-empty pool */
                buf_prigroup->pool_idx = dft_pool;
            } else if (!(pool_map & (1 << buf_prigroup->pool_idx))) {
                LOG_CLI((BSL_META_U(unit,
                                    "MMU config port %d prigroup %d: "
                                    "Pool %d is empty\n"),
                         port, idx, buf_prigroup->pool_idx));
            }
        }

#if 0
        /* queue to pool mapping */
        count = (IS_LB_PORT(unit, port) ? 5 : si->port_num_cosq[port]) +
                                               si->port_num_uc_cosq[port] +
                                               si->port_num_ext_cosq[port];
        for (idx = 0; idx < count; idx++) {
            buf_queue = &buf->ports[port].queues[idx];

            queue_idx = idx;
            if (queue_idx < si->port_num_cosq[port]) {
                sal_sprintf(queue_name, "mqueue %d", queue_idx);
            } else {
                queue_idx -= si->port_num_cosq[port];
                if (queue_idx < si->port_num_uc_cosq[port]) {
                    sal_sprintf(queue_name, "queue %d", queue_idx);
                } else {
                    queue_idx -= si->port_num_uc_cosq[port];
                    sal_sprintf(queue_name, "equeue %d", queue_idx);
                }
            }

            if (buf_queue->pool_idx < 0 ||
                buf_queue->pool_idx >= _SOC_MMU_CFG_DEV_POOL_NUM(devcfg)) {
                LOG_CLI((BSL_META_U(unit,
                                    "MMU config port %d %s: "
                         "Invalid pool value (%d)\n"),
                         port, queue_name, buf_queue->pool_idx));
                buf_queue->pool_idx = dft_pool; /* use first non-empty pool */
            } else if (!(pool_map & (1 << buf_queue->pool_idx))) {
                LOG_CLI((BSL_META_U(unit,
                                    "MMU config port %d %s: Pool %d is empty\n"),
                         port, queue_name, buf_queue->pool_idx));
            }
        }
#endif
    }

    return SOC_E_NONE;
}

_soc_mmu_cfg_buf_t* soc_mmu_cfg_alloc(int unit)
{
    soc_info_t *si;
    _soc_mmu_cfg_buf_t *buf;
    _soc_mmu_cfg_buf_queue_t *buf_queue;
    int num_cosq, alloc_size=0;
    soc_port_t port;

    si = &SOC_INFO(unit);

    alloc_size = sizeof(_soc_mmu_cfg_buf_t);
    PBMP_ALL_ITER(unit, port) {
        num_cosq = si->port_num_cosq[port] + si->port_num_uc_cosq[port];
        alloc_size += sizeof(_soc_mmu_cfg_buf_queue_t) * num_cosq;
    }
    buf = sal_alloc(alloc_size, "MMU config buffer");
    if (buf == NULL) {
        return NULL;
    }

    sal_memset(buf, 0, alloc_size);
    buf_queue = (_soc_mmu_cfg_buf_queue_t *)&buf[1];
    PBMP_ALL_ITER(unit, port) {
        num_cosq = si->port_num_cosq[port] + si->port_num_uc_cosq[port];
        buf->ports[port].queues = buf_queue;
        buf_queue += num_cosq;
    }
    return buf;
}

void
soc_mmu_cfg_free(int unit, _soc_mmu_cfg_buf_t *cfg)
{
    sal_free(cfg);
}

#endif /* defined(BCM_TRIUMPH3_SUPPORT) */
