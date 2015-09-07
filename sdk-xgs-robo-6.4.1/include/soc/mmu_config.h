/*
 * $Id: mmu_config.h,v 1.5 Broadcom SDK $
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
 */


#ifndef _SOC_MMU_CONFIG_H_
#define _SOC_MMU_CONFIG_H_

/*
 * general syntax: buf.object[id].attribute[.port]="string"
 * Some examples of "cell value" string type:
 *   1000b (b for byte)
 *   1000k (k for kilobyte)
 *   1000m (m for megabyte)
 *   1000c (c for cell)
 *
 * objects     attributes        string type and trident register for reference
 * ========================================================================
 * device
 *            headroom           cell value
 *
 * pool#                         # is pool id
 *            size               cell value or %
 *                               This size will affect both ingress and egress
 *                               pool size, it is recommended to use percentage
 *                               since the total pool size calculate for
 *                               ingress and egress are different.
 *            yellow_size        cell value or %
 *                               If the specified value is percentage, it is
 *                               the percentage of the value specified by the
 *                               size attribute
 *                               Use the same value for size, yellow_size, and
 *                               red_size or use 100% for yellow_size and
 *                               red_size to disable color aware.
 *            red_size           cell value or %
 *                               (see notes for yellow_size)
 *
 * port
 *            guarantee          cell value
 *            pool_limit         cell value
 *            pool_resume        cell value
 *            pkt_size           cell value
 *
 * prigroup#                     # is priority group
 *            guarantee          cell value
 *                               specify 0 to disable
 *            headroom           cell value
 *            device_headroom_enable boolean
 *            port_guarantee_enable boolean
 *            port_max_enable    boolean
 *            pool_scale         floating point number value between 1/64 and 8
 *                               Dynamic method will be enabled if this
 *                               attribute is specified.
 *            pool_limit         cell value
 *            pool_resume        cell value
 *            pool_floor         cell value
 *            flow_control_enable boolean
 *
 * queue#                        # is queue id
 * equeue#                       # is extended queue id
 * mqueue#                       # is multicast queue id
 *            guarantee          cell value
 *            pool_scale         floating point number value between 1/64 and 8
 *                               Dynamic method will be enabled if this
 *                               attribute is specified.
 *            pool_limit         cell value
 *                               Specify 0 to indicate disabling queue limit
 *                               and allow to use shared space.
 *            pool_resume        cell value
 *            yellow_limit       cell value for static or % for dynamic
 *                               Use the same value for pool_limit,
 *                               yellow_limit and red_limit to disable color
 *                               aware.
 *                               Use percentage (such as 12.5%, ... 100%) to
 *                               indicate dynamic color limit
 *            red_limit          cell value for static or % for dynamic
 *                               (see notes for yellow_limit)
 *            yellow_resume      cell value
 *            red_resume         cell value
 *
 * ===== mapping =====
 * general syntax: buf.map.from_object.to_object[.port]="string"
 * string is a command separated value list, use 0 if the value is missing
 * for example:
 *     "a,b,c" has same effect as "a,b,c,0,0,0"
 *     "a,,c" has same effect as "a,0,c"
 *
 * buf.map.pri.prigroup[.port]="a,b,c,..."
 *   a is priority group id for priority 0
 *   b is priority group id for priority 1
 *   ...
 *
 * buf.map.prigroup.pool[.port]="a,b,c,..."
 *   a is service pool id for priority group 0
 *   b is service pool id for priority group 1
 *   ...
 *
 * buf.map.queue.pool[.port]="a,b,c,..."
 * buf.map.mqueue.pool[.port]="a,b,c,..."
 *   a is service pool id for queue 0
 *   b is service pool id for queue 1
 *   ...
 */

#define _MMU_CFG_BUF_DYNAMIC_FLAG        (0x80000000)
#define _MMU_CFG_BUF_PERCENT_FLAG        (0x40000000)

#define SOC_MMU_CFG_SERVICE_POOL_MAX    4
#define SOC_MMU_CFG_PORT_MAX            128
#define SOC_MMU_CFG_PRI_GROUP_MAX       8
#define SOC_MMU_CFG_INT_PRI_MAX         16
#define SOC_MMU_CFG_QGROUP_MAX          256
#define SOC_MMU_CFG_EQUEUE_MAX          512
#define SOC_MMU_CFG_RQE_QUEUE_MAX       11

typedef struct _soc_mmu_cfg_buf_prigroup_s {
    int pool_idx; /* from mapping variable */
    int guarantee;
    int headroom;
    int user_delay;
    int switch_delay;
    int pkt_size;
    int device_headroom_enable;
    int port_guarantee_enable;
    int port_max_enable;
    int pool_scale;
    int pool_limit;
    int pool_resume;
    int pool_floor;
    int flow_control_enable;
} _soc_mmu_cfg_buf_prigroup_t;

typedef struct _soc_mmu_cfg_buf_qgroup_s
{
    int guarantee;
    int discard_enable;
    int pool_scale;
    int pool_limit;
    int pool_resume;
    int yellow_limit;
    int red_limit;
    int yellow_resume;
    int red_resume;
} _soc_mmu_cfg_buf_qgroup_t;

typedef struct _soc_mmu_cfg_buf_queue_s
{
    int numq;
    int pool_idx; /* from mapping variable */
    int guarantee;
    int discard_enable;
    int pool_scale;
    int pool_limit;
    int pool_resume;
    int color_discard_enable;
    int yellow_limit;
    int red_limit;
    int yellow_resume;
    int red_resume;
    int qgroup_id;
    int qgroup_min_enable;
    int mcq_entry_guarantee;
    int mcqe_limit;
} _soc_mmu_cfg_buf_queue_t;

typedef struct _soc_mmu_cfg_buf_pool_s {
    int size;
    int yellow_size;
    int red_size;
    int total; /* calculated value (not from config variable) */
    int prigroup_guarantee; /* calculated value (not from config variable) */
    int prigroup_headroom; /* calculated value (not from config variable) */
    int queue_guarantee; /* calculated value (not from config variable) */
    int mcq_entry_reserved;
    int total_mcq_entry;
    int total_rqe_entry;
} _soc_mmu_cfg_buf_pool_t;

typedef struct _soc_mmu_cfg_buf_port_pool_s {
    int guarantee;
    int pool_limit;
    int pool_resume;
} _soc_mmu_cfg_buf_port_pool_t;

typedef struct _soc_mmu_cfg_buf_port_s
{
    int guarantee;
    int pool_limit;
    int pool_resume;
    int pkt_size;
    _soc_mmu_cfg_buf_prigroup_t prigroups[SOC_MMU_CFG_PRI_GROUP_MAX];
    _soc_mmu_cfg_buf_queue_t *queues;
    int pri_to_prigroup[SOC_MMU_CFG_INT_PRI_MAX]; /* from mapping variable */
    _soc_mmu_cfg_buf_port_pool_t pools[SOC_MMU_CFG_SERVICE_POOL_MAX];
} _soc_mmu_cfg_buf_port_t;

typedef struct _soc_mmu_cfg_buf_mcengine_queue_s {
    int pool_idx;
    int guarantee;
    int pool_scale;
    int pool_limit;
    int discard_enable;
    int yellow_limit;
    int red_limit;
} _soc_mmu_cfg_buf_mcengine_queue_t;

typedef struct _soc_mmu_cfg_buf_s
{
    int headroom;
    _soc_mmu_cfg_buf_pool_t pools[SOC_MMU_CFG_SERVICE_POOL_MAX];
    _soc_mmu_cfg_buf_port_t ports[SOC_MMU_CFG_PORT_MAX];
    _soc_mmu_cfg_buf_qgroup_t qgroups[SOC_MMU_CFG_QGROUP_MAX];
    _soc_mmu_cfg_buf_mcengine_queue_t rqe_queues[SOC_MMU_CFG_RQE_QUEUE_MAX];
    _soc_mmu_cfg_buf_queue_t  equeues[SOC_MMU_CFG_EQUEUE_MAX];
} _soc_mmu_cfg_buf_t;

#define SOC_MMU_CFG_INIT(cfg)   \
            sal_memset(cfg, 0, sizeof(_soc_mmu_cfg_buf_t))

/*
 * flags to specifiy various capablities of the MMU for the device.
 */

/* supports port min guarentee */
#define SOC_MMU_CFG_F_PORT_MIN              0x01

/* supports per port per pool min guarentee */
#define SOC_MMU_CFG_F_PORT_POOL_MIN         0x02
#define SOC_MMU_CFG_F_RQE                   0x04
#define SOC_MMU_CFG_F_EGR_MCQ_ENTRY         0x08

typedef struct _soc_mmu_device_info_s {
    uint32  flags;
    uint32  max_pkt_byte;
    uint32  mmu_hdr_byte;
    uint32  jumbo_pkt_size;
    uint32  default_mtu_size;
    uint32  mmu_total_cell;
    uint32  mmu_cell_size;
    uint32  num_pg;
    uint32  num_service_pool;
    uint32  total_mcq_entry;
    uint32  rqe_queue_num;
} _soc_mmu_device_info_t;

#define _MMU_CFG_MMU_BYTES_TO_CELLS(byte,cellhdr) (((byte) + (cellhdr) - 1) / (cellhdr))

#define _SOC_MMU_CFG_DEV_POOL_NUM(dcfg) ((dcfg)->num_service_pool)
#define _SOC_MMU_CFG_DEV_PG_NUM(dcfg) ((dcfg)->num_pg)
#define _SOC_MMU_CFG_MMU_TOTAL_CELLS(dcfg)  ((dcfg)->mmu_total_cell)

#if 0
#define _SOC_MMU_CFG_DEV_EXT_NUM_COSQ(dcfg, port) (dcfg)->port_ext_num_cosq[port]
#define _SOC_MMU_CFG_DEV_MC_NUM_COSQ(dcfg, port) (dcfg)->port_mc_num_cosq[port]
#define _SOC_MMU_CFG_DEV_UC_NUM_COSQ(dcfg, port) (dcfg)->port_uc_num_cosq[port]
#endif

void
_soc_mmu_cfg_buf_read(int unit, _soc_mmu_cfg_buf_t *buf,
                                 _soc_mmu_device_info_t *devcfg);

int
_soc_mmu_cfg_buf_calculate(int unit, _soc_mmu_cfg_buf_t *buf,
                           _soc_mmu_device_info_t *devcfg);

int
_soc_mmu_cfg_buf_check(int unit, _soc_mmu_cfg_buf_t *buf, 
                        _soc_mmu_device_info_t *devcfg);

_soc_mmu_cfg_buf_t* soc_mmu_cfg_alloc(int unit);

void soc_mmu_cfg_free(int unit, _soc_mmu_cfg_buf_t *cfg);

#endif /* _SOC_MMU_CONFIG_H_ */

