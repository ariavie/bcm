/*
 * $Id: cosq.c,v 1.190 Broadcom SDK $
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
 * COS Queue Management
 * Purpose: API to set different cosq, priorities, and scheduler registers.
 */

#include <shared/bsl.h>

#include <soc/defs.h>
#include <sal/core/libc.h>
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/triumph3.h>
#include <bcm/error.h>
#include <bcm/cosq.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/cosq.h>
#include <bcm_int/esw/triumph3.h>

#include <bcm_int/esw_dispatch.h>

#include <bcm_int/bst.h>
#include <bcm_int/esw/cosq.h>

#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm_int/esw/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */

#define TR3_CELL_FIELD_MAX       0x7fff

/* MMU cell size in bytes */
#define _BCM_TR3_PACKET_HEADER_BYTES       64    /* bytes */
#define _BCM_TR3_DEFAULT_MTU_BYTES         1536  /* bytes */
#define _BCM_TR3_TOTAL_CELLS               24576 /* 24k cells */
#define _BCM_TR3_BYTES_PER_CELL            208   /* bytes */
#define _BCM_TR3_BYTES_TO_CELLS(_byte_)  \
    (((_byte_) + _BCM_TR3_BYTES_PER_CELL - 1) / _BCM_TR3_BYTES_PER_CELL)

#define _BCM_TR3_NUM_PORT_SCHEDULERS              64
#define _BCM_TR3_NUM_L0_SCHEDULER                 256
#define _BCM_TR3_NUM_L1_SCHEDULER                 512
#define _BCM_TR3_NUM_L2_UC_LEAVES                 1024
#define _BCM_TR3_NUM_L2_MC_LEAVES                 560
#define _BCM_TR3_NUM_TOTAL_SCHEDULERS     (_BCM_TR3_NUM_PORT_SCHEDULERS + \
                                           _BCM_TR3_NUM_L0_SCHEDULER +    \
                                           _BCM_TR3_NUM_L1_SCHEDULER)


#define _BCM_TR3_COSQ_LIST_NODE_INIT(list, node)         \
    list[node].in_use            = FALSE;               \
    list[node].gport             = -1;                  \
    list[node].base_index        = -1;                  \
    list[node].numq              = 0;                   \
    list[node].base_size         = 0;                   \
    list[node].numq_expandable   = 0;                   \
    list[node].hw_index          = -1;                  \
    list[node].level             = -1;                  \
    list[node].attached_to_input       = -1;                  \
    list[node].hw_cosq           = 0;                   \
    list[node].remote_modid      = -1;                  \
    list[node].remote_port       = -1;                  \
    list[node].local_port        = -1;                  \
    list[node].parent            = NULL;                \
    list[node].sibling           = NULL;                \
    list[node].child             = NULL;                \
    list[node].type              = _BCM_TR3_NODE_UNKNOWN;

#define _BCM_TR3_COSQ_NODE_INIT(node)                    \
    node->in_use            = FALSE;                    \
    node->gport             = -1;                       \
    node->base_index        = -1;                       \
    node->numq              = 0;                        \
    node->base_size         = 0;                        \
    node->numq_expandable   = 0;                        \
    node->hw_index          = -1;                       \
    node->level             = -1;                       \
    node->attached_to_input = -1;                       \
    node->hw_cosq           = 0;                        \
    node->remote_modid      = -1;                       \
    node->remote_port       = -1;                       \
    node->local_port        = -1;                       \
    node->parent            = NULL;                     \
    node->sibling           = NULL;                     \
    node->child             = NULL;                     \
    node->num_child         = 0;                        \
    node->type              = _BCM_TR3_NODE_UNKNOWN;


typedef enum {
    _BCM_TR3_NODE_UNKNOWN = 0,
    _BCM_TR3_NODE_UCAST,
    _BCM_TR3_NODE_MCAST,
    _BCM_TR3_NODE_DMVOQ,
    _BCM_TR3_NODE_VLAN,
    _BCM_TR3_NODE_SCHEDULER
} _bcm_tr3_node_type_e;

typedef struct _bcm_tr3_cosq_node_s {
    struct _bcm_tr3_cosq_node_s *parent;
    struct _bcm_tr3_cosq_node_s *sibling;
    struct _bcm_tr3_cosq_node_s *child;
    bcm_gport_t gport;
    int in_use;
    int base_index;
    uint16 numq_expandable;
    uint16 base_size;
    int numq;
    int hw_index;
    soc_tr3_node_lvl_e level;
    _bcm_tr3_node_type_e type;
    int attached_to_input;
    int hw_cosq;

    /* DPVOQ specific information */
    bcm_port_t   local_port;
    bcm_module_t remote_modid;
    bcm_port_t   remote_port;
    int num_child;
}_bcm_tr3_cosq_node_t;

typedef struct _bcm_tr3_cosq_list_s {
    int count;
    SHR_BITDCL *bits;
}_bcm_tr3_cosq_list_t;

#define _BCM_TR3_MAX_PORT   SOC_MAX_NUM_PORTS

#define _BCM_TR3_IS_UC_QUEUE(mmu_info,qid)   \
                (((qid) < mmu_info->num_base_queues) ? 1 : 0)

#define _BCM_TR3_IS_DESTMOD_QUEUE_GROUP(mmu_info, gport) \
           (((BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) &&  \
            ((BCM_GPORT_UCAST_QUEUE_GROUP_QID_GET(gport)) >= \
                                    (mmu_info)->num_base_queues)) ? 1 : 0)

#define _SOC_TR3_ROOT_SCHEDULER_INDEX(u,p)   \
       SOC_INFO((u)).max_port_p2m_mapping[SOC_INFO((u)).port_l2p_mapping[(p)]]

typedef struct _bcm_tr3_cosq_port_info_s {
    int mc_base;
    int mc_limit;
    int uc_base;
    int uc_limit;
    int eq_base;
    int eq_limit;
} _bcm_tr3_cosq_port_info_t;

typedef struct _bcm_tr3_mmu_info_s {
    int num_base_queues;   /* Number of classical queues */

    _bcm_tr3_cosq_list_t ext_qlist;      /* list of extended queues */
    _bcm_tr3_cosq_list_t l0_sched_list;  /* list of l0 sched nodes */
    _bcm_tr3_cosq_list_t l1_sched_list;  /* list of l1 sched nodes */

    _bcm_tr3_cosq_node_t sched_node[_BCM_TR3_NUM_TOTAL_SCHEDULERS]; /* sched nodes */
    _bcm_tr3_cosq_node_t queue_node[_BCM_TR3_NUM_L2_UC_LEAVES];    /* queue nodes */
    _bcm_tr3_cosq_node_t mc_queue_node[_BCM_TR3_NUM_L2_MC_LEAVES]; /* queue nodes */
    _bcm_tr3_cosq_port_info_t port_info[_BCM_TR3_MAX_PORT];
   
    bcm_pbmp_t                gport_unavail_pbm;

    int                       ets_mode;
}_bcm_tr3_mmu_info_t;

static _bcm_tr3_mmu_info_t *_bcm_tr3_mmu_info[BCM_MAX_NUM_UNITS];  /* MMU info */

static soc_profile_mem_t *_bcm_tr3_wred_profile[BCM_MAX_NUM_UNITS];
static soc_profile_mem_t *_bcm_tr3_cos_map_profile[BCM_MAX_NUM_UNITS];
static soc_profile_mem_t *_bcm_tr3_sample_int_profile[BCM_MAX_NUM_UNITS];
static soc_profile_reg_t *_bcm_tr3_feedback_profile[BCM_MAX_NUM_UNITS];
static soc_profile_reg_t *_bcm_tr3_llfc_profile[BCM_MAX_NUM_UNITS];
static soc_profile_mem_t *_bcm_tr3_voq_port_map_profile[BCM_MAX_NUM_UNITS];
static soc_profile_mem_t *_bcm_tr3_service_cos_map_profile[BCM_MAX_NUM_UNITS];
static soc_profile_mem_t *_bcm_tr3_service_port_map_profile[BCM_MAX_NUM_UNITS];
static soc_profile_mem_t *_bcm_tr3_ifp_cos_map_profile[BCM_MAX_NUM_UNITS];

typedef enum {
    _BCM_TR3_COSQ_TYPE_UCAST,
    _BCM_TR3_COSQ_TYPE_MCAST,
    _BCM_TR3_COSQ_TYPE_EXT_UCAST
} _bcm_tr3_cosq_type_t;

#define IS_TR3_HSP_PORT(u,p)   \
        ((IS_CE_PORT((u),(p))) ||   \
        ((IS_HG_PORT((u),(p))) && (SOC_INFO((u)).port_speed_max[(p)] >= 100000)))


/* Number of COSQs configured for this device */
#if 0
static int _bcm_tr3_num_cosq[SOC_MAX_NUM_DEVICES];
#define _TR3_NUM_COS(unit)  _bcm_tr3_num_cosq[(unit)]
#else
#define _TR3_NUM_COS    NUM_COS
#endif

STATIC int
_bcm_tr3_cosq_node_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cos,
                       bcm_module_t *modid, bcm_port_t *port, 
                       int *id, _bcm_tr3_cosq_node_t **node);

STATIC int
_bcm_tr3_cosq_mapping_set(int unit, bcm_port_t ing_port, bcm_cos_t priority,
                         uint32 flags, bcm_gport_t gport, bcm_cos_queue_t cosq);

STATIC int
_bcm_tr3_cosq_sched_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                       int mode, int weight);

STATIC int 
_bcm_tr3_cosq_egress_sp_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq, 
                        int *p_pool_start, int *p_pool_end);

STATIC int
_bcm_tr3_node_index_set(_bcm_tr3_cosq_list_t *list, int start, int range);

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_VERSION_1_1                SOC_SCACHE_VERSION(1,1)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_1

#define _BCM_TR3_COSQ_WB_NUMCOSQ_MASK         0xf    /* [3:0] */
#define _BCM_TR3_COSQ_WB_NUMCOSQ_SHIFT        0

#define SET_FIELD(_field, _value)                       \
     (((_value) & _BCM_TR3_COSQ_WB_## _field## _MASK) <<  \
      _BCM_TR3_COSQ_WB_## _field## _SHIFT)
#define GET_FIELD(_field, _byte)                        \
     (((_byte) >> _BCM_TR3_COSQ_WB_## _field## _SHIFT) &  \
     _BCM_TR3_COSQ_WB_## _field## _MASK)

/*
 * This funtion encodes the 1 byte of the common  data to be written in to
 * scache.Currently only last 4 bits of the byte are used for storing
 * numcosq value rest can be used if necessary later.
 */ 
STATIC void 
 _bcm_tr3_cosq_wb_encode_data(int unit,char *scache_ptr)
 {
     char data = 0;
     int numq = 0;

     bcm_tr3_cosq_config_get(unit,&numq);
     data  = SET_FIELD(NUMCOSQ, numq);
     *scache_ptr = data; 

 }

STATIC int
_bcm_tr3_cosq_wb_alloc(int unit)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    soc_scache_handle_t scache_handle;
    uint8 *scache_ptr;
    int rv, alloc_size, node_list_sizes[3], ii, jj;
    
    mmu_info = _bcm_tr3_mmu_info[unit];

    if (!mmu_info) {
        return BCM_E_INIT;
    }

    node_list_sizes[0] = _BCM_TR3_NUM_L2_UC_LEAVES;
    node_list_sizes[1] = _BCM_TR3_NUM_L2_MC_LEAVES;
    node_list_sizes[2] = _BCM_TR3_NUM_TOTAL_SCHEDULERS;

    alloc_size = 0;
    for(ii = 0; ii < 3; ii++) {
        alloc_size += sizeof(uint32);
        for(jj = 0; jj < node_list_sizes[ii]; jj++) {
            alloc_size += 4 * sizeof(uint32);
        }
    }
/* 
 * Allocating 1 byte of data first 4 bits will be used
 * for storing numcosq
 */
    alloc_size += sizeof(char); 
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_COSQ, 0);

    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE, alloc_size,
                                 &scache_ptr, BCM_WB_DEFAULT_VERSION, NULL);
    if (rv == BCM_E_NOT_FOUND) {
        rv = BCM_E_NONE;
    }

    return rv;
}

#define _BCM_TR3_WB_SET_NODE_W1(ntype,nid,sz,parentid,cosqatt,lvl)    \
    (((ntype & 3)) | \
     (((sz) & 7) << 2) | \
     (((nid) & 0x3ff) << 5) | \
     (((parentid) & 0x3ff) << 15) | \
     (((cosqatt) & 0xf) << 25) | \
     (((lvl) & 0x7) << 29))

#define _BCM_TR3_WB_GET_NODE_ID_W1(w1) (((w1) >> 5) & 0x3ff)

#define _BCM_TR3_WB_GET_NODE_TYPE_W1(w1) ((w1) & 3)

#define _BCM_TR3_WB_GET_NODE_SIZE_W1(w1) (((w1) >> 2) & 7)

#define _BCM_TR3_WB_GET_NODE_PARENT_W1(w1) (((w1) >> 15) & 0x3ff)

#define _BCM_TR3_WB_GET_NODE_COSQATTT_W1(w1) (((w1) >> 25) & 0xf)

#define _BCM_TR3_WB_GET_NODE_LEVEL_W1(w1) (((w1) >> 29) & 0x7)


/* 12 bit hw index, 11 bit sibling, 3 bit hw cosq */
#define _BCM_TR3_WB_SET_NODE_W2(hwindex,hwcosq)    \
    ((((hwindex) & 0x7ff)) | (((hwcosq) & 0xf) << 11))

#define _BCM_TR3_WB_GET_NODE_HW_INDEX_W2(w1) ((w1) & 0x7ff)

#define _BCM_TR3_WB_GET_NODE_HW_COSQ_W2(w1) (((w1) >> 11) & 0xf)

#define _BCM_TR3_WB_SET_NODE_W3(gport)    (gport)

#define _BCM_TR3_WB_SET_NODE_W4(remote_modid,remote_port,voq_cosq)    \
    ((((remote_modid) & 0x1ff)) | \
     (((remote_port) & 0x1ff) << 9) | \
     (((voq_cosq) & 0xf) << 18))

#define _BCM_TR3_WB_GET_NODE_DEST_MODID_W4(w1) ((w1) & 0x1ff)

#define _BCM_TR3_WB_GET_NODE_DEST_PORT_W4(w1) (((w1) >> 9) & 0x1ff)

#define _BCM_TR3_WB_GET_NODE_VOQ_COSQ_W4(w1) (((w1) >> 18) & 0xf)

#define _BCM_TR3_NODE_ID(np,np0) (np - np0)

int
bcm_tr3_cosq_sync(int unit)
{
    soc_scache_handle_t scache_handle;
    uint8 *scache_ptr;
    uint32 *u32_scache_ptr;
    _bcm_tr3_cosq_node_t *node;
    int node_list_sizes[3], ii, jj;
    _bcm_tr3_cosq_node_t *nodes[3];
    int xnode_id, node_size, count;
    uint32 *psize;
    _bcm_tr3_mmu_info_t *mmu_info;

    mmu_info = _bcm_tr3_mmu_info[unit];

    if (!mmu_info) {
        return BCM_E_INIT;
    }

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_COSQ, 0);
    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE, 0,
                                 &scache_ptr, BCM_WB_DEFAULT_VERSION, NULL));

    u32_scache_ptr = (uint32*) scache_ptr;

    nodes[0] = &mmu_info->queue_node[0];
    node_list_sizes[0] = _BCM_TR3_NUM_L2_UC_LEAVES;
    nodes[1] = &mmu_info->mc_queue_node[0];
    node_list_sizes[1] = _BCM_TR3_NUM_L2_MC_LEAVES;
    nodes[2] = &mmu_info->sched_node[0];
    node_list_sizes[2] = _BCM_TR3_NUM_TOTAL_SCHEDULERS;

    for(ii = 0; ii < 3; ii++) {
        psize = u32_scache_ptr++;
        for(count = 0, jj = 0; jj < node_list_sizes[ii]; jj++) {
            node = nodes[ii] + jj;
            if (node->in_use == FALSE) {
                continue;
            }
            count += 1;
            node_size = 3;
            if ((node->remote_modid != -1) || (node->remote_port != -1) ||
                (node->hw_cosq != -1)) {
                node_size += 1;
            }
            
            if (node->parent == NULL) {
                xnode_id = 0x200;
            } else {
                xnode_id = _BCM_TR3_NODE_ID(node->parent, mmu_info->sched_node);
            }

            *u32_scache_ptr++ = _BCM_TR3_WB_SET_NODE_W1(ii, jj, node_size,
                                xnode_id,
                ((node->attached_to_input == -1) ? 0x8 : node->attached_to_input), 
                                node->level);

            *u32_scache_ptr++ = _BCM_TR3_WB_SET_NODE_W2(
                   (node->hw_index == -1) ? 0x400 : node->hw_index,
                    ((node->hw_cosq == -1) ? 8 : node->hw_cosq));

            *u32_scache_ptr++ = _BCM_TR3_WB_SET_NODE_W3(node->gport);

            if ((node->remote_modid != -1) || (node->remote_port != -1) ||
                (node->hw_cosq != -1)) {
                node_size += 1;
                *u32_scache_ptr++ = _BCM_TR3_WB_SET_NODE_W4(
                    (node->remote_modid == -1) ? 0x100 : node->remote_modid,
                    (node->remote_port == -1) ? 0x100 : node->remote_port,
                    (node->hw_cosq == -1) ? 0x8 : node->hw_cosq);
            }
        }
        *psize = count;
    }
/* add numcosq info to the scache */ 
    _bcm_tr3_cosq_wb_encode_data( unit,(char *)u32_scache_ptr);
    return BCM_E_NONE;
}

int
bcm_tr3_cosq_reinit(int unit)
{
    soc_scache_handle_t scache_handle;
    uint8 *scache_ptr;
    uint32 *u32_scache_ptr, wval, fval;
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t port;
    int rv, ii, jj, set_index;
    _bcm_tr3_cosq_node_t *nodes[3];
    int xnode_id, stable_size, node_size, count;
    _bcm_tr3_mmu_info_t *mmu_info;
    uint32 entry[SOC_MAX_MEM_WORDS], tmp32;
    soc_profile_mem_t *profile_mem;
    soc_info_t *si;
    int phy_port, mmu_port, profile_index;
    uint64  rval64, index_map;
    soc_reg_t reg;
    mmu_wred_config_entry_t qentry;
    int numq = 0;
    uint16 recovered_ver = 0;
    char data;
    int additional_scache_size = 0;
    static const soc_reg_t llfc_cfgr[] = {
        PORT_PFC_CFG0r, PORT_PFC_CFG1r
    };

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if ((stable_size == 0) || (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit))) { 
        /* COSQ warmboot requires extended scache support i.e. level2 warmboot*/  
        return BCM_E_NONE;
    }

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_COSQ, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE, 0, &scache_ptr,
                                 BCM_WB_DEFAULT_VERSION, &recovered_ver);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    mmu_info = _bcm_tr3_mmu_info[unit];

    if (!mmu_info) {
        return BCM_E_INIT;
    }

    si = &SOC_INFO(unit);
    u32_scache_ptr = (uint32*) scache_ptr;

    nodes[0] = &mmu_info->queue_node[0];
    nodes[1] = &mmu_info->mc_queue_node[0];
    nodes[2] = &mmu_info->sched_node[0];

    for(ii = 0; ii < 3; ii++) {
        count = *u32_scache_ptr;
        u32_scache_ptr++;
        for(jj = 0; jj < count; jj++) {
            wval = *u32_scache_ptr;
            u32_scache_ptr++;
            xnode_id = _BCM_TR3_WB_GET_NODE_ID_W1(wval);
            node = nodes[ii] + xnode_id;
            node->in_use = TRUE;
            node->numq = 1;
            node_size = _BCM_TR3_WB_GET_NODE_SIZE_W1(wval);
            fval =  _BCM_TR3_WB_GET_NODE_PARENT_W1(wval);
            if (fval & 0x200) {
            } else {
                node->parent = &mmu_info->sched_node[fval];
                if (node->parent->child) {
                    node->sibling = node->parent->child;
                }
                node->parent->child = node;
            }

            fval = _BCM_TR3_WB_GET_NODE_COSQATTT_W1(wval);
            if (fval & 8) {
                node->attached_to_input = -1;
            } else {
                node->attached_to_input = fval;
            }

            node->level = _BCM_TR3_WB_GET_NODE_LEVEL_W1(wval);

            wval = *u32_scache_ptr;
            u32_scache_ptr++;
            
            fval = _BCM_TR3_WB_GET_NODE_HW_INDEX_W2(wval);
            if (fval & 0x400) {
                node->hw_index = -1;
            } else {
                node->hw_index = fval;
            }

            fval = _BCM_TR3_WB_GET_NODE_HW_COSQ_W2(wval);
            if (fval & 8) {
                node->hw_cosq = -1;
            } else {
                node->hw_cosq = fval;
            }

            wval = *u32_scache_ptr;
            u32_scache_ptr++;
            node->gport = wval;

            if (node->hw_index != -1) {
                if (node->level == SOC_TR3_NODE_LVL_L0) {
                    _bcm_tr3_node_index_set(&mmu_info->l0_sched_list, 
                                            node->hw_index, 1);
                } else if (node->level == SOC_TR3_NODE_LVL_L1) {
                    _bcm_tr3_node_index_set(&mmu_info->l1_sched_list, 
                                            node->hw_index, 1);
                } else if (node->level == SOC_TR3_NODE_LVL_L2) {
                    if (_BCM_TR3_IS_DESTMOD_QUEUE_GROUP(mmu_info, node->gport)) {
                        _bcm_tr3_node_index_set(&mmu_info->ext_qlist, 
                                                node->hw_index, 1);
                    }
                }
            }

            if (node_size > 3) {
                wval = *u32_scache_ptr;
                u32_scache_ptr++;
    
                fval = _BCM_TR3_WB_GET_NODE_DEST_MODID_W4(wval);
                if (fval & 0x100) {
                    node->remote_modid = -1;
                } else {
                    node->remote_modid = fval;
                }

                fval = _BCM_TR3_WB_GET_NODE_DEST_PORT_W4(wval);
                if (fval & 0x100) {
                    node->remote_port = -1;
                } else {
                    node->remote_port = fval;
                }

                fval = _BCM_TR3_WB_GET_NODE_VOQ_COSQ_W4(wval);
                if (fval & 0xf) {
                    node->hw_cosq = -1;
                } else {
                    node->hw_cosq = fval;
                }
            }
        }
    }
    if(recovered_ver >= BCM_WB_DEFAULT_VERSION) {
       data=*((char *)u32_scache_ptr);
       numq = GET_FIELD(NUMCOSQ,data); 
       bcm_tr3_cosq_config_set(unit,numq);
    }
    else { 
       additional_scache_size = sizeof(char);  
    }  
    /* Update PORT_COS_MAP memory profile reference counter */
    profile_mem = _bcm_tr3_cos_map_profile[unit];
    PBMP_ALL_ITER(unit, port) {
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, COS_MAP_SELm, MEM_BLOCK_ALL, port, &entry));
        set_index = soc_mem_field32_get(unit, COS_MAP_SELm, &entry, SELECTf);
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_reference(unit, profile_mem, set_index * 16, 16));
    }
    if (SOC_INFO(unit).cpu_hg_index != -1) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, COS_MAP_SELm, MEM_BLOCK_ALL,
                                         SOC_INFO(unit).cpu_hg_index, &entry));
        set_index = soc_mem_field32_get(unit, COS_MAP_SELm, &entry, SELECTf);
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_reference(unit, profile_mem, set_index, 1));
    }

    PBMP_ALL_ITER(unit, port) {
        if (IS_CPU_PORT(unit, port)) {
            continue;
        }
    
        phy_port = si->port_l2p_mapping[port];
        mmu_port = si->port_p2m_mapping[phy_port];

        reg = llfc_cfgr[mmu_port/32];

        BCM_IF_ERROR_RETURN(soc_reg64_get(unit, reg, 0, 0, &rval64));

        index_map = soc_reg64_field_get(unit, reg, rval64, PROFILE_INDEXf);
        COMPILER_64_SHR(index_map, ((mmu_port % 32) * 2));
        COMPILER_64_TO_32_LO(tmp32, index_map);
        set_index = (tmp32 & 3)*16;

        SOC_IF_ERROR_RETURN
            (soc_profile_reg_reference(unit, _bcm_tr3_llfc_profile[unit], 
                                        set_index * 16, 16));
    }

    for (ii = soc_mem_index_min(unit, MMU_WRED_CONFIGm);
         ii <= soc_mem_index_max(unit, MMU_WRED_CONFIGm); ii++) {
        BCM_IF_ERROR_RETURN(
              soc_mem_read(unit, MMU_WRED_CONFIGm, MEM_BLOCK_ALL, ii, &qentry));
         
        profile_index = soc_mem_field32_get(unit, MMU_WRED_CONFIGm,
                                        &qentry, PROFILE_INDEXf);
         
        BCM_IF_ERROR_RETURN
            (soc_profile_mem_reference(unit, _bcm_tr3_wred_profile[unit],
                                       profile_index, 1));
    }
  /* addtional space is required for scache  */
  if(additional_scache_size) {
     rv = soc_scache_realloc(unit,scache_handle,additional_scache_size); 
     if(BCM_FAILURE(rv)) {
        return rv;
     }
  }
    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
STATIC char*
_bcm_tr3_cosq_node_lvl_str(soc_tr3_node_lvl_e lvl)
{
    switch(lvl) {
        case SOC_TR3_NODE_LVL_ROOT:
            return "Root";
        break;
        case SOC_TR3_NODE_LVL_L0:
            return " L0";
        break;
        case SOC_TR3_NODE_LVL_L1:
            return "  L1";
        break;
        case SOC_TR3_NODE_LVL_L2:
            return "   Q";
        break;
        case SOC_TR3_NODE_LVL_MAX:
        break;
    }
    return "";
}

STATIC void
_bcm_tr3_cosq_dump_node(_bcm_tr3_cosq_node_t *node, int loff)
{
    LOG_CLI((BSL_META("%s.%d index=%d cosq=%d gport=0x%08x numq=%d\n"), 
_bcm_tr3_cosq_node_lvl_str(node->level),
             loff, node->hw_index, node->attached_to_input,
             node->gport, node->numq));

    if (node->child) {
        _bcm_tr3_cosq_dump_node(node->child, 0);
    }

    if (node->sibling) {
        _bcm_tr3_cosq_dump_node(node->sibling, loff+1);
    }
}

STATIC void
_bcm_tr3_cosq_port_sw_dump(int unit, bcm_port_t port)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    int phy_port, mmu_port, ii;
    _bcm_tr3_cosq_node_t *node = NULL, *tnode;
    soc_info_t *si;

    si = &SOC_INFO(unit);
    mmu_info = _bcm_tr3_mmu_info[unit];

    phy_port = si->port_l2p_mapping[port];
    mmu_port = si->port_p2m_mapping[phy_port];

    for (ii = 0; ii < _BCM_TR3_NUM_PORT_SCHEDULERS; ii++) {
        tnode = &mmu_info->sched_node[ii];
        if (tnode->in_use == FALSE) {
            continue;
        }
        if ((tnode->level == SOC_TR3_NODE_LVL_ROOT) &&
            (tnode->hw_index == mmu_port)) {   
            node = tnode;
            break;
        }
    }
    if (node == NULL) {
        return;
    }

    _bcm_tr3_cosq_dump_node(node, 0);
}

/*
 * Function:
 *     bcm_tr3_cosq_sw_dump
 * Purpose:
 *     Displays COS Queue information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
bcm_tr3_cosq_sw_dump(int unit)
{
    bcm_port_t port;

    PBMP_ALL_ITER(unit, port) {
        _bcm_tr3_cosq_port_sw_dump(unit, port);
    }

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

/*
 * Function:
 *     _bcm_tr3_cosq_localport_resolve
 * Purpose:
 *     Resolve GRPOT if it is a local port
 * Parameters:
 *     unit       - (IN) unit number
 *     gport      - (IN) GPORT identifier
 *     local_port - (OUT) local port number
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_cosq_localport_resolve(int unit, bcm_gport_t gport,
                               bcm_port_t *local_port)
{
    bcm_module_t module;
    bcm_port_t port;
    bcm_trunk_t trunk;
    int id, result;

    if (!BCM_GPORT_IS_SET(gport)) {
        if (!SOC_PORT_VALID(unit, gport)) {
            return BCM_E_PORT;
        }
        *local_port = gport;
        return BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, gport, &module, &port, &trunk, &id));

    BCM_IF_ERROR_RETURN(_bcm_esw_modid_is_local(unit, module, &result));
    if (result == FALSE) {
        return BCM_E_PARAM;
    }

    *local_port = port;

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_port_has_ets(int unit, bcm_port_t port)
{
    _bcm_tr3_mmu_info_t *mmu_info = _bcm_tr3_mmu_info[unit];
    return mmu_info->ets_mode;
}

/*
 * Function:
 *     _bcm_tr3_node_index_get
 * Purpose:
 *     Allocate free index from the given list
 * Parameters:
 *     list       - (IN) bit list
 *     start      - (IN) start index
 *     end        - (IN) end index
 *     qset       - (IN) size of queue set
 *     range      - (IN) range bits
 *     id         - (OUT) start index
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_node_index_get(SHR_BITDCL *list, int start, int end,
                       int count, int align, int *id)
{
    int i, j, range_empty;

    *id = -1;

    if ((align <= 0) || (count == 0)) {
        return BCM_E_PARAM;
    }

    if (start & (align - 1)) {
        start = (start + align - 1) & ~(align - 1);
    }

    end = end - align + 1;

    for (i = start; i < end; i += align) {
        range_empty = 1;
        for (j = i; j < (i + count); j++) {
            if (SHR_BITGET(list, j) != 0) {
                range_empty = 0;
                break;
            }
        }

        if (range_empty) {
            *id = i;
            return BCM_E_NONE;
        }
    }
    return BCM_E_RESOURCE;
}

/*
 * Function:
 *     _bcm_tr3_node_index_set
 * Purpose:
 *     Mark indices as allocated
 * Parameters:
 *     list       - (IN) bit list
 *     start      - (IN) start index
 *     range      - (IN) range bits
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_node_index_set(_bcm_tr3_cosq_list_t *list, int start, int range)
{
    list->count += range;
    SHR_BITSET_RANGE(list->bits, start, range);

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_node_index_clear
 * Purpose:
 *     Mark indices as free
 * Parameters:
 *     list       - (IN) bit list
 *     start      - (IN) start index
 *     range      - (IN) range bits
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_node_index_clear(_bcm_tr3_cosq_list_t *list, int start, int range)
{
    list->count -= range;
    SHR_BITCLR_RANGE(list->bits, start, range);

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_alloc_clear
 * Purpose:
 *     Allocate cosq memory and clear
 * Parameters:
 *     size       - (IN) size of memory required
 *     description- (IN) description about the structure
 * Returns:
 *     BCM_E_XXX
 */
STATIC void *
_bcm_tr3_cosq_alloc_clear(void *cp, unsigned int size, char *description)
{
    void *block_p;

    block_p = (cp) ? cp : sal_alloc(size, description);

    if (block_p != NULL) {
        sal_memset(block_p, 0, size);
    }

    return block_p;
}

/*
 * Function:
 *     _bcm_tr3_cosq_free_memory
 * Purpose:
 *     Free memory allocated for mmu info members
 * Parameters:
 *     mmu_info       - (IN) MMU info pointer
 * Returns:
 *     BCM_E_XXX
 */
STATIC void
_bcm_tr3_cosq_free_memory(_bcm_tr3_mmu_info_t *mmu_info_p)
{
    sal_free(mmu_info_p->ext_qlist.bits);
    sal_free(mmu_info_p->l0_sched_list.bits);
    sal_free(mmu_info_p->l1_sched_list.bits);
}

STATIC int
_bcm_tr3_higig_intf_index(int unit, bcm_port_t local_port)
{
    int index = 0, port;

    PBMP_HG_ITER(unit, port) {
        if (port == local_port) {
            return index;
        }
        index += 1;
    }
    return -1;
}

STATIC int
_bcm_tr3_voq_min_hw_index(int unit, bcm_port_t port, 
                          bcm_module_t modid, 
                          bcm_port_t remote_port,
                          int *q_offset)
{
    int i, hw_index_base = -1;
    _bcm_tr3_cosq_node_t *node;
    _bcm_tr3_mmu_info_t *mmu_info;

    mmu_info = _bcm_tr3_mmu_info[unit];

    for (i = mmu_info->num_base_queues; i < _BCM_TR3_NUM_L2_UC_LEAVES; i++) {
        node = &mmu_info->queue_node[i];
        if ((node->in_use == FALSE) || (node->hw_index == -1)) {
            continue;
        }

        if (((modid == -1) ? 1 : (node->remote_modid == modid)) && 
            (node->remote_port == remote_port) &&
            ((port == -1) ? 1 : (port == node->local_port))) {
            if ((hw_index_base == -1) || (hw_index_base > node->hw_index)) {
                hw_index_base = node->hw_index;
            }
        }
    }
   
    if (hw_index_base != -1) {
        *q_offset = hw_index_base & ~7;
        return BCM_E_NONE;
    }

    return BCM_E_NOT_FOUND;
}

STATIC int
_bcm_tr3_port_voq_base_set(int unit, bcm_port_t local_port, int base)
{
    uint32 rval;

    BCM_IF_ERROR_RETURN(READ_ING_COS_MODEr(unit, local_port, &rval));
    soc_reg_field_set(unit, ING_COS_MODEr, &rval, BASE_QUEUE_NUM_1f, base);
    BCM_IF_ERROR_RETURN(WRITE_ING_COS_MODEr(unit, local_port, rval));
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_port_voq_base_get(int unit, bcm_port_t local_port, int *base)
{
    uint32 rval;

    BCM_IF_ERROR_RETURN(READ_ING_COS_MODEr(unit, local_port, &rval));
    if (soc_reg_field_get(unit, ING_COS_MODEr, rval, QUEUE_MODEf) == 1) {
        *base = soc_reg_field_get(unit, ING_COS_MODEr, rval, 
                                  BASE_QUEUE_NUM_1f);
        return BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_tr3_voq_min_hw_index(unit, local_port, -1, -1, base));

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_resolve_dmvoq_hw_index(int unit, _bcm_tr3_cosq_node_t *node, 
                    int cosq, bcm_module_t modid, bcm_port_t local_port)
{
    int try, preffered_offset = -1, alloc_size;
    int from_offset, port, rv, q_base, port_voq_base;
    _bcm_tr3_mmu_info_t *mmu_info;

    mmu_info = _bcm_tr3_mmu_info[unit];

    from_offset = mmu_info->num_base_queues;
    alloc_size = _TR3_NUM_COS(unit);

    for (try = 0; try < 2; try++) {
        PBMP_HG_ITER(unit, port) {
            if ((try == 1) && (port == local_port)) {
                continue;
            }
            
            if ((try == 0) && (port != local_port)) {
                continue;
           }

            rv = _bcm_tr3_voq_min_hw_index(unit, port, modid, -1, &q_base);
            if (!rv) {
                if (port == local_port) {
                    /* Found VOQ for the same port/dest modid */
                    node->hw_index = q_base + cosq;
                    return BCM_E_NONE;
                } else {
                    rv = _bcm_tr3_port_voq_base_get(unit, port, &port_voq_base);
                    if (rv) {
                        continue;
                    }
                    preffered_offset = q_base - port_voq_base;
                    rv = _bcm_tr3_port_voq_base_get(unit, local_port, 
                                                    &port_voq_base);
                    if (rv) {
                        /* No VOQ on the port yet */
                        alloc_size += preffered_offset;
                    } else {
                        /* port has already VOQs */
                        from_offset = port_voq_base + preffered_offset;
                    }
                    break;
                }
            }
        }
        if (preffered_offset != -1) {
            break;
        }
    }

    /* Alloc hw index. */
    rv = _bcm_tr3_node_index_get(mmu_info->ext_qlist.bits, 
                         from_offset, _BCM_TR3_NUM_L2_UC_LEAVES, 
                         alloc_size, _TR3_NUM_COS(unit), &q_base);
    if (rv) {
        return BCM_E_RESOURCE;
    }
    
    if (_bcm_tr3_port_voq_base_get(unit, local_port, &port_voq_base)) {
        _bcm_tr3_port_voq_base_set(unit, local_port, q_base);
    }
    q_base = q_base + alloc_size - _TR3_NUM_COS(unit);
    node->hw_index = q_base + cosq;
    node->remote_modid = modid;
    _bcm_tr3_node_index_set(&mmu_info->ext_qlist, q_base, _TR3_NUM_COS(unit));
    return rv;
}

typedef struct _bcm_tr3_voq_info_s {
    int          valid;
    _bcm_tr3_cosq_node_t *node;
    bcm_module_t remote_modid;
    bcm_port_t   remote_port;
    bcm_port_t   local_port;
    int          hw_cosq;
    int          hw_index;
} _bcm_tr3_voq_info_t;

STATIC int
_bcm_tr3_cosq_sort_voq_list(void *a, void *b)
{
    _bcm_tr3_voq_info_t *va, *vb;
    int sa, sb;

    va = (_bcm_tr3_voq_info_t*)a;
    vb = (_bcm_tr3_voq_info_t*)b;
    
    sa = (va->remote_modid << 19) | (va->remote_port << 11) |
         (va->local_port << 3) | (va->hw_cosq);

    sb = (vb->remote_modid << 19) | (vb->remote_port << 11) |
         (vb->local_port << 3) | (vb->hw_cosq);

    return sa - sb;
}
    
STATIC int
_bcm_tr3_map_fc_status_to_node(int unit, int spad_offset, int cosq,
                               uint32 hw_index, int pfc, int lls_level)
{
    int map_entry_index, eindex;
    soc_mem_t mem;
    uint32 map_entry[SOC_MAX_MEM_WORDS];
    static const soc_field_t self[] = {
        SEL0f, SEL1f, SEL2f, SEL3f
    };
    static const soc_field_t indexf[] = {
        INDEX0f, INDEX1f, INDEX2f, INDEX3f
    };
    static const soc_reg_t mems[] = {
        INVALIDm, MMU_INTFI_FC_MAP_TBL0m,
        MMU_INTFI_FC_MAP_TBL1m, MMU_INTFI_FC_MAP_TBL2m
    };
    
    mem = mems[lls_level];
    if (mem == INVALIDm) {
        return BCM_E_PARAM;
    }
    
    map_entry_index = hw_index/16;
    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, mem, MEM_BLOCK_ALL, map_entry_index, &map_entry));

    eindex = (hw_index % 16)/4;
    soc_mem_field32_set(unit, mem, &map_entry, 
                        indexf[eindex], spad_offset + cosq/4);
    soc_mem_field32_set(unit, mem, &map_entry, self[eindex], !!pfc);
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, mem,
                        MEM_BLOCK_ALL, map_entry_index, &map_entry));
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_get_queue_skip_count_on_cos(uint32 cbmp, int cur_cos, int next_cos)
{
    int i,skip = 0;

    if ((next_cos >= 4) && (cur_cos/4 != next_cos/4)) {
        cur_cos = -1;
        next_cos -= 4;
    }
    
    for (i = cur_cos + 1; i < next_cos; i++) {
        if (cbmp & (1 << i)) {
            skip += 1;
        }
    }
    return skip;
}

STATIC int
_bcm_tr3_gport_dpvoq_config_set(int unit, bcm_gport_t gport, 
                                bcm_cos_queue_t cosq,
                                bcm_module_t remote_modid,
                                bcm_port_t remote_port)
{
    uint32 profile = 0xFFFFFFFF;
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port;
    uint32 cng_offset;
    uint8 cbmp = 0, nib_cbmp = 0;
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_voq_info_t *plist = NULL, *vlist = NULL;
    int pcount, ii, vcount = 0;
    voq_port_map_entry_t *voq_port_map_entries = NULL;
    int rv = BCM_E_NONE, mod_base = 0, num_cos = 0, dm = -1, eq_base;
    voq_mod_map_entry_t voq_mod_map;
    void *entries[1];
    int cng_bytes_per_e2ecc_msg = 32;
    int p2ioff[64], ioff, port, skip, *dm2off, count;

    if ((cosq < 0) || (cosq > _TR3_NUM_COS(unit))) {
        return BCM_E_PARAM;
    }

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    BCM_IF_ERROR_RETURN
      (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, &local_port, NULL, &node));

    eq_base = (mmu_info->num_base_queues + 7) & ~7;
    
    node->remote_modid = remote_modid;
    node->remote_port = remote_port;
    node->hw_cosq = cosq;

    dm2off = sal_alloc(sizeof(int) * 256, "voq_vlist");
    if (!dm2off) {
        return BCM_E_MEMORY;
    }
    sal_memset(dm2off, 0, sizeof(int)*256);

    /* alloc temp vlist */
    vlist = sal_alloc(sizeof(_bcm_tr3_voq_info_t) * 1024, "voq_vlist");
    if (!vlist) {
        return BCM_E_MEMORY;
    }
    sal_memset(vlist, 0, sizeof(_bcm_tr3_voq_info_t)*1024);

    for (ii = 0; ii < _BCM_TR3_NUM_L2_UC_LEAVES; ii++) {
        node = &mmu_info->queue_node[ii];
        if ((node->remote_modid == -1) || (node->remote_port == -1)) {
            continue;
        }
        vlist[vcount].remote_modid = node->remote_modid;
        vlist[vcount].remote_port  = node->remote_port; 
        vlist[vcount].local_port   = node->local_port;
        vlist[vcount].hw_cosq     = node->hw_cosq; 
        vlist[vcount].node         = node;
        vcount += 1;
    }

    /* Sort the nodes */
    _shr_sort(vlist, vcount, sizeof(_bcm_tr3_voq_info_t), 
                _bcm_tr3_cosq_sort_voq_list);

    for (ii = 0; ii < 64; ii++) {
        p2ioff[ii] = -1;
    }
    
    PBMP_HG_ITER(unit, port) {
        ioff = _bcm_tr3_higig_intf_index(unit, port);
        p2ioff[port] = ioff;
    }

    for (ii = 0; ii < vcount; ii++) {
        cbmp |= 1 << vlist[ii].hw_cosq;
    }

    nib_cbmp = (cbmp >> 4) | (cbmp & 0xf);

    for (ii = 0; ii < 4; ii++) {
        if ((1 << ii) & nib_cbmp) {
            num_cos += 1;
        }
    }

    for (count = 1, ii = 0; ii < vcount; ii++) {
        if (dm2off[vlist[ii].remote_modid]==0) {
            dm2off[vlist[ii].remote_modid] = count;
        }
    }

    plist = sal_alloc(sizeof(_bcm_tr3_voq_info_t) * 1024, "voq_vlist");
    if (!plist) {
        sal_free(vlist);
        return BCM_E_MEMORY;
    }
    sal_memset(plist, 0, sizeof(_bcm_tr3_voq_info_t)*1024);

    for (pcount = 0, ii = 0; ii < vcount; ii++) {
        if (pcount % 4) {
            if ((plist[pcount - 1].remote_modid != vlist[ii].remote_modid) ||
                (plist[pcount - 1].remote_port != vlist[ii].remote_port)) {
                pcount = (pcount + 3) & ~3;
            } else if (plist[pcount - 1].local_port != vlist[ii].local_port) {
                skip = (p2ioff[vlist[ii].local_port] - 
                            p2ioff[plist[pcount - 1].local_port] - 1) * num_cos;
                skip +=  _bcm_tr3_get_queue_skip_count_on_cos(nib_cbmp, 
                                        plist[pcount - 1].hw_cosq,
                                        vlist[ii].hw_cosq);
                if (skip >= 4) {
                    pcount = ((pcount + 3) & ~3) + (skip % 4);
                } else {
                    pcount += skip;
                }
            } else if ((vlist[ii].hw_cosq - plist[pcount - 1].hw_cosq) > 1) {
                if (plist[pcount - 1].hw_cosq/4 != vlist[ii].hw_cosq/4) {
                    pcount = (pcount + 3) & ~3;
                }
                pcount += _bcm_tr3_get_queue_skip_count_on_cos(nib_cbmp, 
                                        plist[pcount - 1].hw_cosq,
                                        vlist[ii].hw_cosq);
            }
        } else {
            pcount += _bcm_tr3_get_queue_skip_count_on_cos(nib_cbmp, -1,
                                                    vlist[ii].hw_cosq);
        }
       
        plist[pcount].valid = 1;
        plist[pcount].local_port = vlist[ii].local_port;
        plist[pcount].remote_modid = vlist[ii].remote_modid;
        plist[pcount].remote_port = vlist[ii].remote_port;
        plist[pcount].hw_cosq = vlist[ii].hw_cosq;
        plist[pcount].node = vlist[ii].node;
        vlist[ii].node->hw_index = pcount + eq_base;
        
        #ifdef DEBUG_TR3_COSQ
          LOG_CLI((BSL_META_U(unit,
                              "DPVOQ DM=%d DP=%d LocalPort=%d COS=%d HWIndex=%d\n"),
                   plist[pcount].remote_modid,
                   plist[pcount].remote_port,
                   plist[pcount].local_port,
                   plist[pcount].hw_cosq,
                   plist[pcount].node->hw_index));
        #endif
        pcount += 1;
    }

    /* program the voq port profiles. */
    voq_port_map_entries = sal_alloc(sizeof(voq_port_map_entry_t)*128, 
                                        "voq port map entries");
    if (!voq_port_map_entries) {
        goto fail;
    }
    sal_memset(voq_port_map_entries, 0, sizeof(voq_port_map_entries)*128);

    entries[0] = voq_port_map_entries;
    dm = -1;
    for (ii = 0; ii <= pcount; ii++) {
        if ((ii == pcount) ||
            ((dm != -1) && (plist[ii - 1].remote_modid != dm))) {
            
            READ_VOQ_MOD_MAPm(unit, MEM_BLOCK_ALL, dm, &voq_mod_map);
            if (soc_mem_field32_get(unit, VOQ_MOD_MAPm, 
                                                &voq_mod_map, VOQ_VALIDf)) {
                profile = soc_mem_field32_get(unit, VOQ_MOD_MAPm, 
                              &voq_mod_map, VOQ_MOD_PORT_PROFILE_INDEXf);
                
                rv = soc_profile_mem_delete(unit, 
                            _bcm_tr3_voq_port_map_profile[unit], profile*128);
                if (rv) {
                    goto fail;
                }
            }
            rv = soc_profile_mem_add(unit, 
                                _bcm_tr3_voq_port_map_profile[unit], 
                                entries, 128, &profile);
            if (rv) {
                goto fail;
            }
           
            soc_mem_field32_set(unit, VOQ_MOD_MAPm, &voq_mod_map, 
                                VOQ_MOD_PORT_PROFILE_INDEXf, profile/128);
            soc_mem_field32_set(unit, VOQ_MOD_MAPm, &voq_mod_map, 
                                VOQ_VALIDf, 1);
            soc_mem_field32_set(unit, VOQ_MOD_MAPm, &voq_mod_map, 
                                VOQ_MOD_OFFSETf, mod_base);
            rv = soc_mem_write(unit, VOQ_MOD_MAPm, MEM_BLOCK_ALL, 
                               dm, &voq_mod_map);
            if (rv) {
                goto fail;
            }
            sal_memset(voq_port_map_entries, 0, sizeof(voq_port_map_entries)*128);
            if (ii != pcount) {
                mod_base = plist[ii].hw_index & ~3;
                dm = plist[ii].remote_modid;
            }
        }

        if (ii == pcount) {
            break;
        }

        if (plist[ii].valid == 0) {
            continue;
        }

        dm = plist[ii].remote_modid;
        soc_mem_field32_set(unit, VOQ_PORT_MAPm, 
                            &voq_port_map_entries[plist[ii].remote_port], 
                            VOQ_PORT_OFFSETf, 
                            plist[ii].node->hw_index - mod_base);
    }

    dm = -1;
    for (ii = 0; ii < pcount; ii += 1) {
        if (plist[ii].valid == 0) {
            continue;
        }

        cng_offset = (dm2off[plist[ii].remote_modid] - 1) * 
                        cng_bytes_per_e2ecc_msg;

        _bcm_tr3_map_fc_status_to_node(unit, 
                                       cng_offset + plist[ii].remote_port, 
                                       plist[ii].hw_cosq,
                                       plist[ii].node->hw_index, 0, 
                                       SOC_TR3_NODE_LVL_L2);
    }
   
fail:
    if (vlist) {
        sal_free(vlist);
    }

    if (plist) {
        sal_free(plist);
    }

    if (dm2off) {
        sal_free(dm2off);
    }
    
    if (voq_port_map_entries) {
        sal_free(voq_port_map_entries);
    }

    if (rv) {
        node->remote_modid = -1;
        node->remote_port = -1;
    }
    return rv;
}

STATIC int
_bcm_tr3_gport_dmvoq_config_set(int unit, bcm_gport_t gport, 
                                bcm_cos_queue_t cosq,
                                bcm_module_t fabric_modid, 
                                bcm_module_t dest_modid,
                                bcm_port_t fabric_port)
{
    int port_voq_base, intf_index;
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port;
    voq_mod_map_entry_t voq_mod_map_entry;
    mmu_intfi_base_index_tbl_entry_t base_tbl_entry;
    uint32 cng_offset = 0;
    _bcm_tr3_mmu_info_t *mmu_info;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if (fabric_modid >= 64) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
      (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, &local_port, NULL, &node));

    if (!node) {
        return BCM_E_NOT_FOUND;
    }

    BCM_IF_ERROR_RETURN(_bcm_tr3_resolve_dmvoq_hw_index(unit, node, cosq,
                                                  dest_modid, local_port));

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, VOQ_MOD_MAPm, MEM_BLOCK_ALL, 
                                    dest_modid, &voq_mod_map_entry));
    if (!soc_mem_field32_get(unit, VOQ_MOD_MAPm, &voq_mod_map_entry,
                            VOQ_VALIDf)) {

        BCM_IF_ERROR_RETURN(_bcm_tr3_port_voq_base_get(unit, local_port, 
                                                        &port_voq_base));
        soc_mem_field32_set(unit, VOQ_MOD_MAPm, &voq_mod_map_entry,
                        VOQ_VALIDf, 1);

        soc_mem_field32_set(unit, VOQ_MOD_MAPm, &voq_mod_map_entry,
                        VOQ_MOD_OFFSETf, 
                        (node->hw_index & ~7) - port_voq_base);

        BCM_IF_ERROR_RETURN(
            soc_mem_write(unit, VOQ_MOD_MAPm, MEM_BLOCK_ALL, dest_modid, 
                       &voq_mod_map_entry));
    }

    intf_index = _bcm_tr3_higig_intf_index(unit, local_port);
    if (intf_index < 0) {
        return BCM_E_INTERNAL;
    }

    cng_offset = intf_index * 128;

    /* set the mapping from congestion bits in flow control table
     * to corresponding cosq. */
    BCM_IF_ERROR_RETURN(_bcm_tr3_map_fc_status_to_node(unit, 
              cng_offset + fabric_port, cosq, node->hw_index, 0,
              SOC_TR3_NODE_LVL_L2));


    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, MMU_INTFI_BASE_INDEX_TBLm, 
                      MEM_BLOCK_ALL, fabric_modid, &base_tbl_entry));
    
    soc_mem_field32_set(unit, MMU_INTFI_BASE_INDEX_TBLm, &base_tbl_entry, 
                        BASE_ADDRf, cng_offset);
    soc_mem_field32_set(unit, MMU_INTFI_BASE_INDEX_TBLm, &base_tbl_entry, 
                        ENf, 2);

    BCM_IF_ERROR_RETURN(soc_mem_write(unit, MMU_INTFI_BASE_INDEX_TBLm,
                        MEM_BLOCK_ALL, fabric_modid, &base_tbl_entry));

    #ifdef DEBUG_TR3_COSQ
    LOG_CLI((BSL_META_U(unit,
                        "Gport=0x%08x hw_index=%d cosq=%d\n"),
             gport, node->hw_index, cosq));
    #endif

    return BCM_E_NONE;
}

int bcm_tr3_cosq_gport_congestion_config_set(int unit, bcm_gport_t gport, 
                                         bcm_cos_queue_t cosq, 
                                         bcm_cosq_congestion_info_t *config)
{
    _bcm_tr3_mmu_info_t *mmu_info;

    if (!config) {
        return BCM_E_PARAM;
    }

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if (!_BCM_TR3_IS_DESTMOD_QUEUE_GROUP(mmu_info, gport)) {
        return BCM_E_PARAM;
    }
    
    /* determine if this gport is DPVOQ or DMVOQ */
    if ((config->fabric_port != -1) && (config->dest_modid != -1)) {
        if (config->fabric_modid == -1) {
            return BCM_E_PARAM;
        }
        return _bcm_tr3_gport_dmvoq_config_set(unit, gport, cosq, 
                                               config->fabric_modid,
                                               config->dest_modid,
                                               config->fabric_port);
    } else if ((config->dest_modid != -1) && (config->dest_port != -1)) {
        return _bcm_tr3_gport_dpvoq_config_set(unit, gport, cosq, 
                                               config->dest_modid,
                                               config->dest_port);
    }
    
    return BCM_E_PARAM;
}

int bcm_tr3_cosq_gport_congestion_config_get(int unit, bcm_gport_t port, 
                                         bcm_cos_queue_t cosq, 
                                         bcm_cosq_congestion_info_t *config)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    bcm_port_t local_port;
    _bcm_tr3_cosq_node_t *node;

    if (!config) {
        return BCM_E_PARAM;
    }

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if (!_BCM_TR3_IS_DESTMOD_QUEUE_GROUP(mmu_info, port)) {
        return BCM_E_PARAM;
    }
    
    BCM_IF_ERROR_RETURN
      (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, &local_port, NULL, &node));


    if (!node) {
        return BCM_E_NOT_FOUND;
    }

    config->dest_modid = node->remote_modid;
    config->dest_port = node->remote_port;

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_node_get
 * Purpose:
 *     Get internal information of queue group or scheduler type GPORT
 * Parameters:
 *     unit       - (IN) unit number
 *     gport      - (IN) queue group/scheduler GPORT identifier
 *     modid      - (OUT) module identifier
 *     port       - (OUT) port number
 *     id         - (OUT) queue group or scheduler identifier
 *     node       - (OUT) node structure for the specified GPORT
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_cosq_node_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                       bcm_module_t *modid, bcm_port_t *port, 
                       int *id, _bcm_tr3_cosq_node_t **node)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_node_t *node_base = NULL;
    bcm_module_t modid_out = 0;
    bcm_port_t port_out = 0;
    uint32 encap_id = -1;
    int index;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &modid_out));
        port_out = BCM_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_GET(gport);
    } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &modid_out));
        port_out = BCM_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_GET(gport);
    } else if (BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &modid_out));
        port_out = (gport >> 16) & 0xff;
    } else if (BCM_GPORT_IS_SCHEDULER(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &modid_out));
        port_out = BCM_GPORT_SCHEDULER_GET(gport) & 0xff;
    } else if (BCM_GPORT_IS_LOCAL(gport)) {
        encap_id = BCM_GPORT_LOCAL_GET(gport);
        port_out = encap_id;
    } else if (BCM_GPORT_IS_MODPORT(gport)) {
        modid_out = SOC_GPORT_MODPORT_MODID_GET(gport);
        port_out = SOC_GPORT_MODPORT_PORT_GET(gport);
        encap_id = port_out;
    } else {
        return BCM_E_PORT;
    }

    if (!SOC_PORT_VALID(unit, port_out)) {
        return BCM_E_PORT;
    }

    if (port != NULL) {
        *port = port_out;
    }

    if (!_bcm_tr3_cosq_port_has_ets(unit, port_out)) {
        return BCM_E_NOT_FOUND;
    }

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
        encap_id = BCM_GPORT_UCAST_QUEUE_GROUP_QID_GET(gport);
        index = encap_id;
        node_base = mmu_info->queue_node;
    } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport)) {
        encap_id = BCM_GPORT_MCAST_QUEUE_GROUP_QID_GET(gport);
        index = encap_id;
        node_base = mmu_info->mc_queue_node;
    } else if (BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(gport)) {
        encap_id = BCM_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_GET(gport);
        index = encap_id;
        node_base = mmu_info->queue_node;
    } else if (BCM_GPORT_IS_SCHEDULER(gport)) {
        encap_id = (BCM_GPORT_SCHEDULER_GET(gport) >> 8) & 0x7ff;
        index = encap_id;
        node_base = mmu_info->sched_node;
    } else {
        index = encap_id;
        node_base = mmu_info->sched_node;
    }

    if (index < 0) {
        return BCM_E_NOT_FOUND;
    }

    if (node_base[index].numq == 0) {
        return BCM_E_NOT_FOUND;
    }

    if (modid != NULL) {
        *modid = modid_out;
    }
    if (id != NULL) {
        *id = encap_id;
    }
    if (node != NULL) {
        *node = &node_base[index];
    }

    return BCM_E_NONE;
}

int
_bcm_tr3_cosq_port_resolve(int unit, bcm_gport_t gport, bcm_module_t *modid,
                          bcm_port_t *port, bcm_trunk_t *trunk_id, int *id,
                          int *qnum)
{
    _bcm_tr3_cosq_node_t *node;

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_node_get(unit, gport, 0, modid, port, id, &node));
    *trunk_id = -1;

    if (qnum) {
        if (node->hw_index == -1) {
            return BCM_E_PARAM;
        }
        *qnum = node->hw_index;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_gport_traverse
 * Purpose:
 *     Walks through the valid COSQ GPORTs and calls
 *     the user supplied callback function for each entry.
 * Parameters:
 *     unit       - (IN) unit number
 *     gport      - (IN)
 *     cb         - (IN) Callback function.
 *     user_data  - (IN) User data to be passed to callback function
 * Returns:
 *     BCM_E_NONE - Success.
 *     BCM_E_XXX
 */
int
_bcm_tr3_cosq_gport_traverse(int unit, bcm_gport_t gport,
                            bcm_cosq_gport_traverse_cb cb,
                            void *user_data)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_node_t *node;
    uint32 flags = BCM_COSQ_GPORT_SCHEDULER;
    int rv = BCM_E_NONE, id;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, NULL, NULL, &node));

    if (node != NULL) {
        if (node->level == SOC_TR3_NODE_LVL_L2) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(node->gport)) {
                id = BCM_GPORT_UCAST_QUEUE_GROUP_QID_GET(node->gport);
                if (_BCM_TR3_IS_UC_QUEUE(mmu_info, id)) {
                    flags = BCM_COSQ_GPORT_UCAST_QUEUE_GROUP;
                } else if (node->type == _BCM_TR3_NODE_DMVOQ) {
                    flags = BCM_COSQ_GPORT_DESTMOD_UCAST_QUEUE_GROUP;
                } else if (node->type == _BCM_TR3_NODE_VLAN) {
                    flags = BCM_COSQ_GPORT_VLAN_UCAST_QUEUE_GROUP;
                }
            } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(node->gport)) {
                flags = BCM_COSQ_GPORT_MCAST_QUEUE_GROUP;
            }
        } else {
            flags = BCM_COSQ_GPORT_SCHEDULER;
        }
        rv = cb(unit, gport, node->numq, flags,
                  node->gport, user_data);
       if (BCM_FAILURE(rv)) {
#ifdef BCM_CB_ABORT_ON_ERR
           if (SOC_CB_ABORT_ON_ERR(unit)) {
               return rv;
           }
#endif
       }
    } else {
            return BCM_E_NONE;
    }

    if (node->sibling != NULL) {
        _bcm_tr3_cosq_gport_traverse(unit, node->sibling->gport, cb, user_data);
    }

    if (node->child != NULL) {
        _bcm_tr3_cosq_gport_traverse(unit, node->child->gport, cb, user_data);
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_traverse_lls_tree(int unit, int port, _bcm_tr3_cosq_node_t *node,
                                _soc_tr3_lls_traverse_cb cb, void *cookie)
{
    if (node->child) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_traverse_lls_tree(unit, port, node->child,
                                                            cb, cookie));
    }

    if (node->sibling) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_traverse_lls_tree(unit, port, 
                                                node->sibling, cb, cookie));
    }

    if (node->level != SOC_TR3_NODE_LVL_ROOT) {
        BCM_IF_ERROR_RETURN(cb(unit, port, node->level, node->hw_index, cookie));
    }
    
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_child_node_at_input(_bcm_tr3_cosq_node_t *node, int cosq,
                                  _bcm_tr3_cosq_node_t **child_node)
{
    _bcm_tr3_cosq_node_t *cur_node;

    for (cur_node = node->child; cur_node; cur_node = cur_node->sibling) {
        if (cur_node->attached_to_input == cosq) {
            break;
        }
    }
    if (!cur_node) {
        return BCM_E_NOT_FOUND;
    }

    *child_node = cur_node;

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_l2_gport(int unit, int port, int cosq,
                       _bcm_tr3_node_type_e type, bcm_gport_t *gport,
                       _bcm_tr3_cosq_node_t **p_node)
{
    int i, max;
    _bcm_tr3_cosq_node_t *node, *fnode;
    _bcm_tr3_mmu_info_t *mmu_info;

    mmu_info = _bcm_tr3_mmu_info[unit];
    
    max  = 0;
    if ((type == _BCM_TR3_NODE_UCAST) || (type == _BCM_TR3_NODE_DMVOQ) ||
        (type == _BCM_TR3_NODE_VLAN)) {
        max = _BCM_TR3_NUM_L2_UC_LEAVES;
        fnode = &mmu_info->queue_node[0];
    } else if (type == _BCM_TR3_NODE_MCAST) {
        max = _BCM_TR3_NUM_L2_MC_LEAVES;
        fnode = &mmu_info->mc_queue_node[0];
    } else {
        return BCM_E_PARAM;
    }

    for (i = 0; i < max; i++) {
        node = fnode + i;
        if (node->in_use && (node->local_port == port) &&
            (node->type == type) && (node->hw_cosq == cosq)) {
            if (gport) {
                *gport = node->gport;
            }
            if (p_node) {
                *p_node = node;
            }
            return BCM_E_NONE;
        }
    }
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *     _bcm_tr3_cosq_index_resolve
 * Purpose:
 *     Find hardware index for the given port/cosq
 * Parameters:
 *     unit       - (IN) unit number
 *     port       - (IN) port number or GPORT identifier
 *     cosq       - (IN) COS queue number
 *     style      - (IN) index style (_BCM_KA_COSQ_INDEX_STYLE_XXX)
 *     local_port - (OUT) local port number
 *     index      - (OUT) result index
 *     count      - (OUT) number of index
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_tr3_cosq_index_resolve(int unit, bcm_port_t port, bcm_cos_queue_t cosq, 
                           _bcm_tr3_cosq_index_style_t style,                 
                           bcm_port_t *local_port, int *index, int *count)
{
    _bcm_tr3_cosq_node_t *node, *cur_node;
    bcm_port_t resolved_port;
    int resolved_index = -1;
    int id, startq, numq, pool_start, pool_end, lvl;
    soc_info_t *si;
    int phy_port, mmu_port;

    si = &SOC_INFO(unit);

    if (cosq < -1) {
        return BCM_E_PARAM;
    } else if (cosq == -1) {
        startq = 0;
        numq = _TR3_NUM_COS(unit);
    } else {
        startq = cosq;
        numq = 1;
    }

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port) || 
        BCM_GPORT_IS_MCAST_QUEUE_GROUP(port) ||
        BCM_GPORT_IS_SCHEDULER(port)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_node_get(unit, port, cosq, NULL, &resolved_port, &id,
                                   &node));
        if (node->attached_to_input < 0) { /* Not resolved yet */
            return BCM_E_PARAM;
        }
        numq = (node->numq != -1) ? node->numq : 32;
    } else {
        /* optionally validate port */
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &resolved_port));
        if (resolved_port < 0) {
            return BCM_E_PORT;
        }
        node = NULL;
        numq = (IS_CPU_PORT(unit, resolved_port)) ? NUM_CPU_COSQ(unit) : _TR3_NUM_COS(unit);
    }

    if (startq >= numq) {
        return BCM_E_PARAM;
    }

    phy_port = si->port_l2p_mapping[resolved_port];
    mmu_port = si->port_p2m_mapping[phy_port];
    

    if ((node == NULL) && (BCM_GPORT_IS_MODPORT(port)) &&
        (_bcm_tr3_cosq_port_has_ets(unit, resolved_port))) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_node_get(unit, port, 0, 
                                                    NULL, NULL, NULL, &node));
    }

    switch (style) {
    case _BCM_TR3_COSQ_INDEX_STYLE_BUCKET:
        if (node != NULL) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
                resolved_index = node->hw_index;
            } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
                resolved_index = node->hw_index;
            } else if (BCM_GPORT_IS_SCHEDULER(port) || BCM_GPORT_IS_MODPORT(port)) {
                /* resolve the child attached to input cosq */
                BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, startq, &cur_node));
                resolved_index = cur_node->hw_index;
            } else {
                return BCM_E_PARAM;
            }
        } else { /* node == NULL */
            if (IS_CPU_PORT(unit, resolved_port)) {
                resolved_index = SOC_INFO(unit).port_cosq_base[resolved_port] +
                                 startq;
                resolved_index = soc_tr3_l2_hw_index(unit, resolved_index, 0);
            } else {
                if (IS_TR3_HSP_PORT(unit, resolved_port)) {
                    /* 
                     * HSP ports use Level-0 scheduler nodes 
                     * Adjust queue to allow for the MC-Group node (L0.0)
                     */
                    lvl = SOC_TR3_NODE_LVL_L0; 
                    startq++; 
                } else {
                    lvl = SOC_TR3_NODE_LVL_L1; 
                } 
                BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, 
                                     resolved_port, lvl, 
                                     startq, &resolved_index));
            }
        }
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_QCN:
        if (node != NULL) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
                resolved_index = node->hw_index;
            } else {
                return BCM_E_PARAM;
            }
        } else { /* node == NULL */
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, resolved_port));
            if (IS_CPU_PORT(unit, resolved_port)) {
                return BCM_E_PARAM;
            } else {
                resolved_index = si->port_uc_cosq_base[resolved_port] + startq;
            }
        }
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_WRED:
        if (node != NULL) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
                resolved_index = node->hw_index;
            } else if (BCM_GPORT_IS_SCHEDULER(port)) {
                BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, startq, &cur_node));
                if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(cur_node->gport)) {
                    resolved_index = cur_node->hw_index;
                } else {
                    return BCM_E_PARAM;
                }
            } else if (BCM_GPORT_IS_MODPORT(port)) {
                BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_l2_gport(unit, node->local_port, 
                                   startq, _BCM_TR3_NODE_UCAST, NULL, &cur_node));
                resolved_index = cur_node->hw_index;
            } else {
                return BCM_E_PARAM;
            }
        } else { /* node == NULL */
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, resolved_port));
            resolved_index = si->port_uc_cosq_base[resolved_port] + startq;
            numq = 1;
        }
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_WRED_PORT:
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_egress_sp_get(unit, resolved_port, 
                                            startq, &pool_start, &pool_end));
        resolved_index = 1280 + (mmu_port*4) + pool_start;
        numq = 4;
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_SCHEDULER:
        if (node != NULL) {
            if (!BCM_GPORT_IS_SCHEDULER(port)) {
                return BCM_E_PARAM;
            }
            BCM_IF_ERROR_RETURN(
                _bcm_tr3_cosq_child_node_at_input(node, startq, &cur_node));
            resolved_index = cur_node->hw_index;
        } else { /* node == NULL */
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, resolved_port));
            if (IS_CPU_PORT(unit, resolved_port)) {
                resolved_index = 1024 + si->port_cosq_base[resolved_port] + startq;
            } else {
                if (IS_TR3_HSP_PORT(unit, resolved_port)) {
                    /* 
                     * HSP ports use Level-0 scheduler nodes 
                     * Adjust queue to allow for the MC-Group node (L0.0)
                     */
                    lvl = SOC_TR3_NODE_LVL_L0; 
                    startq++; 
                } else {
                    lvl = SOC_TR3_NODE_LVL_L1; 
                } 
                BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, 
                                    resolved_port, lvl, startq, 
                                    &resolved_index));
            }
        }
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_COS:
        if (node) {
            if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) ||
                (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port))) {
                resolved_index = node->hw_cosq;
            } else if (BCM_GPORT_IS_SCHEDULER(port)) {
                BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, startq, &cur_node));
                if (BCM_GPORT_IS_SCHEDULER(cur_node->gport)) {
                    return BCM_E_PARAM;
                }
                resolved_index = cur_node->hw_cosq;
            }
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, resolved_port));
            resolved_index = startq;
        }
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE:
        if (node) {
            resolved_index = node->hw_index;
            numq = 1;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, resolved_port));
            if (IS_CPU_PORT(unit, resolved_port)) {
                numq = 0;
                resolved_index = -1;
            } else {
                numq = 1;
                resolved_index = si->port_uc_cosq_base[resolved_port] + startq;
            }
        }
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE:
        if (node) {
            resolved_index = node->hw_index;
            numq = 1;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, resolved_port));
            numq = 1;
            resolved_index = 1024 + si->port_cosq_base[resolved_port] + startq;
        }
        break;

    case _BCM_TR3_COSQ_INDEX_STYLE_EGR_POOL:
        if (node != NULL) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
                    /* regular unicast queue */
                resolved_index = node->hw_index + startq;
            } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
                resolved_index = node->hw_index - si->port_cosq_base[resolved_port];
            } else { /* scheduler */
                return BCM_E_PARAM;
            }
        } else { /* node == NULL */
            numq = si->port_num_cosq[resolved_port];
            if (startq >= numq) {
                return BCM_E_PARAM;
            }
            resolved_index = startq;
        }
        break;

    default:
        return BCM_E_INTERNAL;
    }

    if (local_port != NULL) {
        *local_port = resolved_port;
    }
    if (index != NULL) {
        *index = resolved_index;
    }
    if (count != NULL) {
        *count = (cosq == -1) ? numq : 1;
    }

    return BCM_E_NONE;
}

STATIC int _bcm_tr3_cosq_min_child_index(_bcm_tr3_cosq_node_t *child, int lvl, int uc)
{
    int min_index = -1;

    if ((lvl == SOC_TR3_NODE_LVL_L2) && (uc)) {
        while (child) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(child->gport)) {
                if (min_index == -1) {
                    min_index = child->hw_index;
                }
                
                if (child->hw_index < min_index) {
                    min_index = child->hw_index;
                }
            }
            child = child->sibling;
        }
    } else if ((lvl == SOC_TR3_NODE_LVL_L2) && (!uc)) {
        while (child) {
            if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(child->gport)) {
                if (min_index == -1) {
                    min_index = child->hw_index;
                }
                
                if (child->hw_index < min_index) {
                    min_index = child->hw_index;
                }
            }
            child = child->sibling;
        }
    } else {
        while (child) {
            if (min_index == -1) {
                min_index = child->hw_index;
            }
            
            if (child->hw_index < min_index) {
                min_index = child->hw_index;
            }
            child = child->sibling;
        }
    }

    return min_index;
}

STATIC int
_bcm_tr3_attach_node_in_hw(int unit, _bcm_tr3_cosq_node_t *node)
{
    int local_port, fc = 0, fmc = 0;
    _bcm_tr3_cosq_node_t *child_node;

    local_port = node->local_port;

    if (node->level == SOC_TR3_NODE_LVL_ROOT) {
        return 0;
    } else if (node->level == SOC_TR3_NODE_LVL_L2) {
        fc = _bcm_tr3_cosq_min_child_index(node->parent->child, SOC_TR3_NODE_LVL_L2, 1);
        if (fc < 0) {
            fc = 0;
        }
        fmc = _bcm_tr3_cosq_min_child_index(node->parent->child, SOC_TR3_NODE_LVL_L2, 0);
        if (fmc < 0) {
            fmc = 1024;
        }
    } else {
        fc = _bcm_tr3_cosq_min_child_index(node->parent->child, node->level, 0);
        fmc = 0;
    }

    /* If node is scheduler, reset the scheduler and attach it to the parent.*/
    BCM_IF_ERROR_RETURN(
        soc_tr3_cosq_set_sched_parent(unit, local_port, node->level, 
                                  node->hw_index, node->parent->hw_index));

    /* set the node by default in WRR mode with wt=1 */
    BCM_IF_ERROR_RETURN(soc_tr3_cosq_set_sched_config(unit, 
                          local_port, node->parent->level,
                          node->parent->hw_index, node->hw_index, 
                          0, fc, fmc, 0, 0,
                          SOC_TR3_SCHED_MODE_WRR, 1));

    if (node->child) {
        child_node = node->child;
        while (child_node) {
            BCM_IF_ERROR_RETURN(_bcm_tr3_attach_node_in_hw(unit, child_node));
            child_node = child_node->sibling;
        }
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_node_resolve
 * Purpose:
 *     Resolve queue number in the specified scheduler tree and its subtree
 * Parameters:
 *     unit       - (IN) unit number
 *     node       - (IN) node structure for the specified scheduler node
 *     cosq       - (IN) COS queue to attach to
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_cosq_node_resolve(int unit, _bcm_tr3_cosq_node_t *node,
                          bcm_cos_queue_t cosq)
{
    _bcm_tr3_cosq_node_t *cur_node, *parent;
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_list_t *list;
    int numq, id, i, ii, prev_base = -1;
    bcm_port_t port;
    uint64 map, tmp_map;

    if ((parent = node->parent) == NULL) {
        /* Not attached, should never happen */
        return BCM_E_NOT_FOUND;
    }

    if (parent->numq == 0 || parent->numq < -1) {
        return BCM_E_PARAM;
    }

    if (parent->numq != -1) {
         /* find the current attached_to_input usage */
        COMPILER_64_ZERO(map);
        for (cur_node = parent->child; cur_node != NULL;
             cur_node = cur_node->sibling) {
            if (cur_node->attached_to_input >= 0) {
                COMPILER_64_SET(tmp_map, 0, 1);
                COMPILER_64_SHL(tmp_map, cur_node->attached_to_input);
                COMPILER_64_OR(map, tmp_map);
            }
        }

        if (cosq < 0) {
            for (i = 0; i < parent->numq; i++) {
                if (!COMPILER_64_BITTEST(map, i)) {
                    node->attached_to_input = i;
                    cosq = i;
                    break;
                }
            }
            if (i == parent->numq) {
                return BCM_E_PARAM;
            }
        } else {
            if (COMPILER_64_BITTEST(map, cosq)) {
                return BCM_E_EXISTS;
            }
            node->attached_to_input = cosq;
        }
    } else if (parent->numq == -1) {
        COMPILER_64_ZERO(map);
        for (cur_node = parent->child; cur_node != NULL;
             cur_node = cur_node->sibling) {
            if (cur_node->attached_to_input >= 0) {
                COMPILER_64_SET(tmp_map, 0, 1);
                COMPILER_64_SHL(tmp_map, cur_node->attached_to_input);
                COMPILER_64_OR(map, tmp_map);
            }
        }
        if (cosq >= 0) {
            if (COMPILER_64_BITTEST(map, cosq)) {
                return BCM_E_EXISTS;
            }
        } else {
            /* Attach anywhere. find first free slot */
            for (ii = 0; ii < 64; ii++) {
                if (!COMPILER_64_BITTEST(map, ii)) {
                    cosq = ii;
                    break;
                }
            }
        }
        node->attached_to_input = cosq;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_node_get(unit, node->gport, 0, NULL, &port, NULL, NULL));

    mmu_info = _bcm_tr3_mmu_info[unit];
    numq = (parent->numq == -1) ? 1 : parent->numq;

    switch (parent->level) {
        case SOC_TR3_NODE_LVL_ROOT:
            list = &mmu_info->l0_sched_list;
            /* if parent node has queue number which is dynamic, 
             * free the previos queues, and get the new number of queues.
             * NOTE : This attach routine should not be called when live
             * traffic, this should be dont in the beginning only. */
            if (parent->numq_expandable && parent->base_index >= 0 &&
                parent->base_size != parent->numq) {
                _bcm_tr3_node_index_clear(list, parent->base_index, 
                                          parent->base_size);
                prev_base = parent->base_index;
                parent->base_index =  -1;
            }
            
            if ((parent->base_index < 0) || (parent->numq == -1)) {
                id = -1;
                if (IS_CPU_PORT(unit, node->local_port)) {
                    BCM_IF_ERROR_RETURN(
                      _bcm_tr3_node_index_get(list->bits, 0, NUM_COS(unit), 
                                              numq, 1, &id));
                }
                if (id == -1) {
                    BCM_IF_ERROR_RETURN(
                      _bcm_tr3_node_index_get(list->bits, NUM_COS(unit), 
                                    _BCM_TR3_NUM_L0_SCHEDULER, numq, 1, &id));
                }
                if ((id == -1) && (!IS_CPU_PORT(unit, node->local_port))) {
                    BCM_IF_ERROR_RETURN(
                      _bcm_tr3_node_index_get(list->bits, 0, NUM_COS(unit), 
                                                numq, 1, &id));
                }
                _bcm_tr3_node_index_set(list, id, numq);
                if (parent->numq != -1) {
                    parent->base_index = id;
                    node->hw_index = parent->base_index + node->attached_to_input;
                    parent->base_size = numq;
                    if ((prev_base >= 0) && (prev_base != id)) {
                        for (cur_node = parent->child; cur_node; 
                                                cur_node = cur_node->sibling) {
                            if (cur_node->attached_to_input >= 0) {
                                cur_node->hw_index = parent->base_index + 
                                                     cur_node->attached_to_input;
                                BCM_IF_ERROR_RETURN(_bcm_tr3_attach_node_in_hw(unit, cur_node));
                            }
                        }
                    }
                } else {
                    node->hw_index = id;
                }
            } else {
                node->hw_index = parent->base_index + node->attached_to_input;
           }
            node->hw_cosq = node->hw_index % NUM_COS(unit);

            break;

        case SOC_TR3_NODE_LVL_L0:
            if ((parent->base_index < 0) || (parent->numq == -1)) {
                list = &mmu_info->l1_sched_list;
                BCM_IF_ERROR_RETURN(_bcm_tr3_node_index_get(list->bits, 0, 
                                  _BCM_TR3_NUM_L1_SCHEDULER, numq, 1, &id));
                _bcm_tr3_node_index_set(list, id, numq);
                parent->base_size = numq;
                if (parent->numq != -1) {
                    parent->base_index = id;
                    node->hw_index = parent->base_index + node->attached_to_input;
                } else {
                    node->hw_index = id;
                }
            } else {
                node->hw_index = parent->base_index + node->attached_to_input;
            }
            node->hw_cosq = node->hw_index % NUM_COS(unit);
            break;

        case SOC_TR3_NODE_LVL_L1:
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(node->gport)) {
                if (node->type == _BCM_TR3_NODE_VLAN) {
                    if ((parent->base_index < 0) || (parent->numq == -1)) {
                        BCM_IF_ERROR_RETURN(
                                _bcm_tr3_node_index_get(mmu_info->ext_qlist.bits,
                                    mmu_info->num_base_queues, _BCM_TR3_NUM_L2_UC_LEAVES,
                                    numq, 1, &id));
                        _bcm_tr3_node_index_set(&mmu_info->ext_qlist, id, numq);
                        /* Since hardware, require continous queues only if 
                           numq <= 8 */
                        if ((parent->numq != -1) && (parent->numq <=8)) {
                            parent->base_index = id;
                            node->hw_index = id; 
                        } else {
                            node->hw_index = id; 
                        }
                    } else {
                        /* Since indexes are already reserverved in the first iteration,
                           just assigning the hardware index */
                        if (node->hw_index < 0) {
                            node->hw_index = parent->base_index + node->attached_to_input;
                        }
                    }
                    node->hw_cosq = node->attached_to_input; 
                    parent->num_child++;
                } else if (node->hw_index == -1) {
                    return BCM_E_PARAM;
                }
            } else if (node->hw_index == -1) {
                return BCM_E_PARAM;
            }
            break;

        case SOC_TR3_NODE_LVL_L2:
            /* passthru */
        default:
            return BCM_E_PARAM;

    }

    BCM_IF_ERROR_RETURN(_bcm_tr3_attach_node_in_hw(unit, node));

    #ifdef DEBUG_TR3_COSQ
    LOG_CLI((BSL_META_U(unit,
                        "Resolved gport: 0x%08x level:%d to hw_index=%d\n"),
             node->gport, node->level, node->hw_index));
    #endif

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_node_unresolve
 * Purpose:
 *     Unresolve queue number in the specified scheduler tree and its subtree
 * Parameters:
 *     unit       - (IN) unit number
 *     node       - (IN) node structure for the specified scheduler node
 *     cosq       - (IN) COS queue to attach to
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_cosq_node_unresolve(int unit, _bcm_tr3_cosq_node_t *node, 
                             bcm_cos_queue_t cosq)
{
    _bcm_tr3_cosq_node_t *parent;
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_list_t *list;
    int numq = 1;

    if (node->attached_to_input < 0) {
        /* Has been unresolved already */
        return BCM_E_NONE;
    }

    parent = node->parent;

    if (!parent) {
        return BCM_E_PARAM;
    }

    mmu_info = _bcm_tr3_mmu_info[unit];

    list = NULL;

    switch (parent->level) {
        case SOC_TR3_NODE_LVL_ROOT:
            list = &mmu_info->l0_sched_list;
            break;

        case SOC_TR3_NODE_LVL_L0:
            list = &mmu_info->l1_sched_list;
            break;

        case SOC_TR3_NODE_LVL_L1:
            if (node->type == _BCM_TR3_NODE_VLAN) {
                parent->num_child--;
                if ( parent->numq == -1) {
                    list = &mmu_info->ext_qlist;
                } else {
                    if (parent->num_child == 0) {
                        parent->base_index = -1;
                        numq = parent->numq;
                        list = &mmu_info->ext_qlist;
                    }
                }
            }
            break;

        case SOC_TR3_NODE_LVL_L2:
            /* passthru */
        default:
            return BCM_E_PARAM;

    }

    if (list) {
        _bcm_tr3_node_index_clear(list, node->hw_index, numq);
    }
    node->attached_to_input = -1;

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_add
 * Purpose:
 *     Create a cosq gport structure
 * Parameters:
 *     unit  - (IN) unit number
 *     port  - (IN) port number
 *     numq  - (IN) number of COS queues
 *     flags - (IN) flags (BCM_COSQ_GPORT_XXX)
 *     gport - (OUT) GPORT identifier
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_add(int unit, bcm_gport_t port, int numq, uint32 flags,
                      bcm_gport_t *gport)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_node_t *node = NULL;
    _bcm_tr3_cosq_port_info_t *port_info;
    uint32 sched_encap;
    int phy_port, mmu_port, local_port;
    int id;
    _bcm_tr3_cosq_list_t *list;

    LOG_INFO(BSL_LS_BCM_COSQ,
             (BSL_META_U(unit,
                         "bcm_tr3_cosq_gport_add: unit=%d port=0x%x numq=%d flags=0x%x\n"),
              unit, port, numq, flags));

    if (gport == NULL) {
        return BCM_E_PARAM;
    }

    if (_bcm_tr3_mmu_info[unit] == NULL) {
        return BCM_E_INIT;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    if (local_port < 0) {
        return BCM_E_PORT;
    }

    mmu_info = _bcm_tr3_mmu_info[unit];
    port_info = &mmu_info->port_info[local_port];

    if (!mmu_info->ets_mode) {
        SOC_IF_ERROR_RETURN(soc_tr3_lls_reset(unit));
        /*
         * stale bits still be set as the detach is not called
         * Reset the complete list.
         */
        list = &mmu_info->l0_sched_list;
        SOC_IF_ERROR_RETURN(
            _bcm_tr3_node_index_clear(list, 0, _BCM_TR3_NUM_L0_SCHEDULER));
        list = &mmu_info->l1_sched_list;
        SOC_IF_ERROR_RETURN(
            _bcm_tr3_node_index_clear(list, 0, _BCM_TR3_NUM_L1_SCHEDULER));
        SOC_IF_ERROR_RETURN(soc_tr3_lb_lls_init(unit));
        mmu_info->ets_mode = 1;
    }

    if (BCM_PBMP_MEMBER(mmu_info->gport_unavail_pbm, local_port)) {
        return BCM_E_UNAVAIL;
    }

    switch (flags) {
    case BCM_COSQ_GPORT_UCAST_QUEUE_GROUP:
        if (numq != 1) {
            return BCM_E_PARAM;
        }

        if (IS_CPU_PORT(unit, local_port)) {
            return BCM_E_PARAM;
        }

        for (id = port_info->uc_base; id < port_info->uc_limit; id++) {
            if (mmu_info->queue_node[id].numq == 0) {
                break;
            }
        }

        /* allocating more then num-cos uc queues */
        if (id == port_info->uc_limit) {
            for (id = mmu_info->num_base_queues; 
                                id < _BCM_TR3_NUM_L2_UC_LEAVES; id++) {
                if (mmu_info->queue_node[id].numq == 0) {
                    break;
                }
            }
        }

        if (id == _BCM_TR3_NUM_L2_UC_LEAVES) {
            return BCM_E_RESOURCE;
        }

        BCM_GPORT_UCAST_QUEUE_GROUP_SYSQID_SET(*gport, local_port, id);
        node = &mmu_info->queue_node[id];
        node->gport = *gport;
        node->numq = numq;
        node->level = SOC_TR3_NODE_LVL_L2;
        node->hw_cosq = id - port_info->uc_base;
        node->hw_index = soc_tr3_l2_hw_index(unit, id, 1);
        node->local_port = local_port;
        node->remote_modid = -1;
        node->remote_port  = -1;
        node->type = _BCM_TR3_NODE_UCAST;
        node->in_use = TRUE;
        break;

    case BCM_COSQ_GPORT_VLAN_UCAST_QUEUE_GROUP:
        /* Service Queueing are outside default queues. Hence NO alignment
         * required.
         */
        if (numq != 1) {
            return BCM_E_PARAM;
        }
        for (id = mmu_info->num_base_queues; 
                id < _BCM_TR3_NUM_L2_UC_LEAVES; id++) {
            if (mmu_info->queue_node[id].numq == 0) {
                break;
            }
        }

        if (id >= _BCM_TR3_NUM_L2_UC_LEAVES) {
            return BCM_E_RESOURCE;
        }

        BCM_GPORT_UCAST_QUEUE_GROUP_SYSQID_SET(*gport, local_port, id);
        node = &mmu_info->queue_node[id];
        node->gport = *gport;
        node->numq = numq ;
        node->level = SOC_TR3_NODE_LVL_L2;
        node->hw_index = -1;
        node->local_port = local_port;
        node->remote_modid = -1;
        node->remote_port  = -1;
        node->type = _BCM_TR3_NODE_VLAN;
        node->in_use = TRUE;

        break;

    case BCM_COSQ_GPORT_DESTMOD_UCAST_QUEUE_GROUP:
        if (numq != 1) {
            return BCM_E_PARAM;
        }

        if (!IS_HG_PORT(unit, local_port)) {
            return BCM_E_PORT;
        }

        for (id = mmu_info->num_base_queues; 
                            id < _BCM_TR3_NUM_L2_UC_LEAVES; id++) {
            if (mmu_info->queue_node[id].numq == 0) {
                break;
            }
        }

        if (id >= _BCM_TR3_NUM_L2_UC_LEAVES) {
            return BCM_E_RESOURCE;
        }

        BCM_GPORT_UCAST_QUEUE_GROUP_SYSQID_SET(*gport, local_port, id);
        node = &mmu_info->queue_node[id];
        node->gport = *gport;
        node->numq = 1;
        node->level = SOC_TR3_NODE_LVL_L2;
        node->hw_index = -1;
        /* by default assign the cosq on order of order of alocation,
         * can be overriddedn by cos_mapping_set */
        node->hw_cosq = -1;
        node->local_port = local_port;
        node->remote_modid = -1;
        node->remote_port  = -1;
        node->in_use = TRUE;
        node->type = _BCM_TR3_NODE_DMVOQ;
        break;

    case BCM_COSQ_GPORT_MCAST_QUEUE_GROUP:
        if (numq != 1) {
            return BCM_E_PARAM;
        }

        for (id = port_info->mc_base; id < port_info->mc_limit; id++) {
            if (mmu_info->mc_queue_node[id].numq == 0) {
                break;
            }
        }
        
        if (id == port_info->mc_limit) {
            return BCM_E_RESOURCE;
        }            

        /* set index bits */
        BCM_GPORT_MCAST_QUEUE_GROUP_SYSQID_SET(*gport, local_port, id);
        node = &mmu_info->mc_queue_node[id];
        node->gport = *gport;
        node->numq = numq;
        node->level = SOC_TR3_NODE_LVL_L2;
        node->hw_cosq = id - port_info->mc_base;
        node->hw_index = soc_tr3_l2_hw_index(unit, id, 0);
        node->local_port = local_port;
        node->remote_modid = -1;
        node->remote_port  = -1;
        node->type = _BCM_TR3_NODE_MCAST;
        node->in_use = TRUE;
        break;

    case BCM_COSQ_GPORT_SCHEDULER:
        /* passthru */
    case BCM_COSQ_GPORT_WITH_ID:
    case 0:
        if (numq < -1) {
            return BCM_E_PARAM;
        }

        if (( flags & BCM_COSQ_GPORT_WITH_ID) || (flags == 0)) {
            /* this is a port level scheduler */
            id = local_port;

            if ( id < 0 || id >= _BCM_TR3_NUM_PORT_SCHEDULERS) {
                return BCM_E_PARAM;
            }

            node = &mmu_info->sched_node[id];
            sched_encap = (id << 8) | local_port;
            BCM_GPORT_SCHEDULER_SET(*gport, sched_encap);
            node->gport = *gport;
            node->level = SOC_TR3_NODE_LVL_ROOT;
            phy_port = SOC_INFO(unit).port_l2p_mapping[local_port];
            mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
            node->hw_index = mmu_port;
            node->local_port = local_port;
            node->numq = numq;
            node->in_use = TRUE;
            node->type = _BCM_TR3_NODE_SCHEDULER;
            node->attached_to_input = 0;
        } else {
            for (id = _BCM_TR3_NUM_PORT_SCHEDULERS; 
                 id < _BCM_TR3_NUM_TOTAL_SCHEDULERS; id++) {
                if (mmu_info->sched_node[id].in_use == FALSE) {
                    node = &mmu_info->sched_node[id];
                    node->in_use = TRUE;
                    break;
                }
            }
             if (!node) {
                return BCM_E_RESOURCE;
            }

            node = &mmu_info->sched_node[id];
            sched_encap = (id << 8) | local_port;
            BCM_GPORT_SCHEDULER_SET(*gport, sched_encap);
            node->gport = *gport;
            node->numq = numq;
            node->local_port = local_port;
            node->type = _BCM_TR3_NODE_SCHEDULER;
            node->attached_to_input = -1;
       }
       break;

    default:
        return BCM_E_PARAM;
    }

    LOG_INFO(BSL_LS_BCM_COSQ,
             (BSL_META_U(unit,
                         "                       gport=0x%x\n"),
              *gport));

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_detach
 * Purpose:
 *     Detach sched_port from the specified index (cosq) of input_port
 * Parameters:
 *     unit       - (IN) unit number
 *     sched_port - (IN) scheduler GPORT identifier
 *     input_port - (IN) GPORT to detach from
 *     cosq       - (IN) COS queue to detach from
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_detach(int unit, bcm_gport_t sched_gport,
                         bcm_gport_t input_gport, bcm_cos_queue_t cosq)
{
    _bcm_tr3_cosq_node_t *sched_node, *input_node = NULL, *prev_node, *parent;
    bcm_port_t sched_port, input_port;
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_list_t *list;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(input_gport)) ||
        (BCM_GPORT_IS_MCAST_QUEUE_GROUP(input_gport))) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_node_get(unit, sched_gport, 0, NULL, &sched_port, NULL,
                               &sched_node));

    if (sched_node->attached_to_input < 0) {
        /* Not attached yet */
        return BCM_E_PORT;
    }

    if (input_gport != BCM_GPORT_INVALID) {
        if (BCM_GPORT_IS_SCHEDULER(input_gport) || 
            BCM_GPORT_IS_MODPORT(input_gport)   || 
            BCM_GPORT_IS_LOCAL(input_gport)) {
               BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, input_gport, 0, NULL, 
                                    &input_port, NULL, &input_node));
        } else {
            if (!(BCM_GPORT_IS_SCHEDULER(sched_gport) || 
                  BCM_GPORT_IS_UCAST_QUEUE_GROUP(sched_gport) ||
                  BCM_GPORT_IS_MCAST_QUEUE_GROUP(sched_gport))) {
                return BCM_E_PARAM;
            }
            else {
                BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_localport_resolve(unit, input_gport,
                                                &input_port));
                input_node = NULL;
            }
        }
    }

    if (sched_port != input_port) {
        return BCM_E_PORT;
    }

    if (sched_node->parent != input_node) {
        return BCM_E_PORT;
    }

    if ((cosq < -1) || 
        (input_node && (input_node->numq != -1) && (cosq >= input_node->numq))) {
        return BCM_E_PARAM;
    }

    if (cosq != -1) {
        if (sched_node->attached_to_input != cosq) {
            return BCM_E_PARAM;
        }
    }

    if (sched_node->parent == NULL) {
        return BCM_E_PARAM;
    }
    
    BCM_IF_ERROR_RETURN(
        _bcm_tr3_cosq_sched_set(unit, sched_node->parent->gport, 
                                sched_node->attached_to_input,
                                BCM_COSQ_WEIGHTED_ROUND_ROBIN, 1));

        BCM_IF_ERROR_RETURN(
            soc_tr3_cosq_set_sched_parent(unit, input_port, 
                        sched_node->level, sched_node->hw_index, 
                        _soc_tr3_invalid_parent_index(unit, sched_node->level)));
        /* unresolve the node - delete this node from parent's child list */
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_node_unresolve(unit, sched_node, cosq));

        /* now remove from the sw tree */
        if (sched_node->parent != NULL) {
            parent = sched_node->parent;
            if (sched_node->parent->child == sched_node) {
                sched_node->parent->child = sched_node->sibling;
            } else {
                prev_node = sched_node->parent->child;
                while (prev_node != NULL && prev_node->sibling != sched_node) {
                    prev_node = prev_node->sibling;
                }
                if (prev_node == NULL) {
                    return BCM_E_INTERNAL;
                }
                prev_node->sibling = sched_node->sibling;
            }
            sched_node->parent = NULL;
            sched_node->sibling = NULL;
            sched_node->attached_to_input = -1;

            if (parent->child == NULL) {
                list = NULL;
                switch (parent->level) {
                    case SOC_TR3_NODE_LVL_ROOT:
                        list = &mmu_info->l0_sched_list;
                        break;
                    case SOC_TR3_NODE_LVL_L0:
                        list = &mmu_info->l1_sched_list;
                        break;
                    case SOC_TR3_NODE_LVL_L1:
                    default:
                        break;
                }
                if (list) {
                    BCM_IF_ERROR_RETURN(_bcm_tr3_node_index_clear(list, 
                                parent->base_index, parent->base_size));
                    parent->base_index = -1;
                    parent->base_size  = 0;
                }
            }
        }

    LOG_INFO(BSL_LS_BCM_COSQ,
             (BSL_META_U(unit,
                         "                         hw_cosq=%d\n"),
              sched_node->attached_to_input));

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_delete
 * Purpose:
 *     Destroy a cosq gport structure
 * Parameters:
 *     unit  - (IN) unit number
 *     gport - (IN) GPORT identifier
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_delete(int unit, bcm_gport_t gport)
{
    _bcm_tr3_cosq_node_t *node = NULL, *tnode;
    int local_port;
    soc_info_t *si;
    _bcm_tr3_mmu_info_t *mmu_info;
    int phy_port, mmu_port, ii;

    LOG_INFO(BSL_LS_BCM_COSQ,
             (BSL_META_U(unit,
                         "bcm_tr3_cosq_gport_delete: unit=%d gport=0x%x\n"),
              unit, gport));

    si = &SOC_INFO(unit);

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport) || 
        BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport) ||
        BCM_GPORT_IS_SCHEDULER(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, &local_port, NULL, &node));
    } else {
        local_port = (BCM_GPORT_IS_LOCAL(gport)) ?
          BCM_GPORT_LOCAL_GET(gport) : BCM_GPORT_MODPORT_PORT_GET(gport);

        if (!SOC_PORT_VALID(unit, local_port)) {
            return BCM_E_PORT;
        }
       
        mmu_info = _bcm_tr3_mmu_info[unit];
        phy_port = si->port_l2p_mapping[local_port];
        mmu_port = si->port_p2m_mapping[phy_port];
        
        for (ii = 0; ii < _BCM_TR3_NUM_PORT_SCHEDULERS; ii++) {
            tnode = &mmu_info->sched_node[ii];
            if (tnode->in_use == FALSE) {
                continue;
            }
            if ((tnode->level == SOC_TR3_NODE_LVL_ROOT) &&
                (tnode->hw_index == mmu_port)) {   
                node = tnode;
                break;
            }
        }
        if (node == NULL) {
            return BCM_E_NONE;
        }
    }

    if (node->child != NULL) {
        BCM_IF_ERROR_RETURN(bcm_tr3_cosq_gport_delete(unit, node->child->gport));
    }

    if (node->sibling != NULL) {
        BCM_IF_ERROR_RETURN(bcm_tr3_cosq_gport_delete(unit, node->sibling->gport));
    }

    if (node->level != SOC_TR3_NODE_LVL_ROOT && node->attached_to_input >= 0) {
        BCM_IF_ERROR_RETURN
            (bcm_tr3_cosq_gport_detach(unit, node->gport, 
                                node->parent->gport, node->attached_to_input));
    }

    _BCM_TR3_COSQ_NODE_INIT(node);
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_get
 * Purpose:
 *     Get a cosq gport structure
 * Parameters:
 *     unit  - (IN) unit number
 *     gport - (IN) GPORT identifier
 *     port  - (OUT) port number
 *     numq  - (OUT) number of COS queues
 *     flags - (OUT) flags (BCM_COSQ_GPORT_XXX)
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_get(int unit, bcm_gport_t gport, bcm_gport_t *port,
                      int *numq, uint32 *flags)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_node_t *node;
    bcm_module_t modid;
    bcm_port_t local_port;
    int id;
    _bcm_gport_dest_t dest;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if (port == NULL || numq == NULL || flags == NULL) {
        return BCM_E_PARAM;
    }

    LOG_INFO(BSL_LS_BCM_COSQ,
             (BSL_META_U(unit,
                         "bcm_tr3_cosq_gport_get: unit=%d gport=0x%x\n"),
              unit, gport));

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, &local_port, &id, &node));

    if (SOC_USE_GPORT(unit)) {
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &modid));
        dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
        dest.modid = modid;
        dest.port = local_port;
        BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &dest, port));
    } else {
        *port = local_port;
    }

    *numq = node->numq;

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
        id = BCM_GPORT_UCAST_QUEUE_GROUP_QID_GET(gport);
        if (_BCM_TR3_IS_UC_QUEUE(mmu_info, id)) {
            *flags = BCM_COSQ_GPORT_UCAST_QUEUE_GROUP;
        } else if (node->type == _BCM_TR3_NODE_DMVOQ) {
            *flags = BCM_COSQ_GPORT_DESTMOD_UCAST_QUEUE_GROUP;
        } else if (node->type == _BCM_TR3_NODE_VLAN) {
            *flags = BCM_COSQ_GPORT_VLAN_UCAST_QUEUE_GROUP;
        }
    } else if (BCM_GPORT_IS_SCHEDULER(gport)) {
        *flags = BCM_COSQ_GPORT_SCHEDULER;
    } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport)) {
        *flags = BCM_COSQ_GPORT_MCAST_QUEUE_GROUP;
    } else {
        *flags = 0;
    }

    LOG_INFO(BSL_LS_BCM_COSQ,
             (BSL_META_U(unit,
                         "                       port=0x%x numq=%d flags=0x%x\n"),
              *port, *numq, *flags));

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_traverse
 * Purpose:
 *     Walks through the valid COSQ GPORTs and calls
 *     the user supplied callback function for each entry.
 * Parameters:
 *     unit       - (IN) unit number
 *     trav_fn    - (IN) Callback function.
 *     user_data  - (IN) User data to be passed to callback function
 * Returns:
 *     BCM_E_NONE - Success.
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_traverse(int unit, bcm_cosq_gport_traverse_cb cb,
                           void *user_data)
{
    _bcm_tr3_cosq_node_t *port_info;
    _bcm_tr3_mmu_info_t *mmu_info;
    bcm_module_t my_modid, modid_out;
    bcm_port_t port, port_out;

    if (_bcm_tr3_mmu_info[unit] == NULL) {
        return BCM_E_INIT;
    }
    mmu_info = _bcm_tr3_mmu_info[unit];

    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
    PBMP_ALL_ITER(unit, port) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                    my_modid, port, &modid_out, &port_out));

        /* root node */
        port_info = &mmu_info->sched_node[port_out];

        if (port_info->gport >= 0) {
            _bcm_tr3_cosq_gport_traverse(unit, port_info->gport, cb, user_data);
        }
      }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_attach
 * Purpose:
 *     Attach sched_port to the specified index (cosq) of input_port
 * Parameters:
 *     unit       - (IN) unit number
 *     sched_port - (IN) scheduler GPORT identifier
 *     input_port - (IN) GPORT to attach to
 *     cosq       - (IN) COS queue to attach to (-1 for first available cosq)
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_attach(int unit, bcm_gport_t gport, 
                           bcm_gport_t to_gport, 
                           bcm_cos_queue_t to_cosq)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_node_t *node, *to_node;
    bcm_port_t port, to_port, local_port;
    int rv;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(to_gport)) ||
        (BCM_GPORT_IS_MCAST_QUEUE_GROUP(to_gport)) ||
        (BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(to_gport))) {
        return BCM_E_PARAM;
    }

    if(!(BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport) ||
         BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport) ||
         BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(gport) ||
         BCM_GPORT_IS_SCHEDULER(gport))) {
        return BCM_E_PORT;
    }

    if (to_cosq >= 64) {
        return BCM_E_RESOURCE;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, &port, NULL, &node));

    if (node->attached_to_input >= 0) {
        /* Has already attached */
        return BCM_E_EXISTS;
    }

    if (BCM_GPORT_IS_SCHEDULER(to_gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_node_get(unit, to_gport, 0, NULL, &to_port, NULL,
                                   &to_node));
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, to_gport, &to_port));
        to_node = &mmu_info->sched_node[to_port];
    }

    if (port != to_port) {
        return BCM_E_PORT;
    }

    /* Identify the levels of schedulers
     * to_port == phy_port && gport == scheduler => gport = L0
     * to_port == schduler && gport == scheduler => to_port = L0 and port = L1
     * to_port == scheduler && port == queue_group => to_port = L1 and port = L2
     */
    if (to_node != NULL) {
        if (!BCM_GPORT_IS_SCHEDULER(to_gport)) {
            if ((to_node->numq == 0) || (to_node->numq_expandable)) {
                local_port = (BCM_GPORT_IS_LOCAL(to_gport)) ?
                              BCM_GPORT_LOCAL_GET(to_gport) :
                              BCM_GPORT_MODPORT_PORT_GET(to_gport);
                to_node->gport = to_gport;
                to_node->hw_index = 
                    SOC_INFO(unit).port_p2m_mapping[SOC_INFO(unit).port_l2p_mapping[local_port]];
                to_node->in_use = TRUE;
                to_node->level = SOC_TR3_NODE_LVL_ROOT;
                to_node->local_port = port;
                to_node->attached_to_input = 0;
                to_node->numq_expandable = 1;

                /* How many children do we need, 1 if unknown, else 
                 * take the max of to_cosq or current numq */
                if (to_cosq == -1) {
                    to_node->numq += 1;
                } else {
                    if (to_cosq >= to_node->numq) {
                        to_node->numq = to_cosq + 1;
                    }
                }
            }

            if (!BCM_GPORT_IS_SCHEDULER(gport)) {
                return BCM_E_PARAM;
            }

            node->level = SOC_TR3_NODE_LVL_L0;
        } else {
             if (to_node->level == SOC_TR3_NODE_LVL_ROOT) {
                 to_node->attached_to_input = 0;
                 node->level = SOC_TR3_NODE_LVL_L0;
             }

             if (IS_TR3_HSP_PORT(unit, to_port)) {
                 if ((to_node->level == -1)) {
                     to_node->level = 
                         (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport) ||
                          BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport) ||
                          BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(gport)) ?
                           SOC_TR3_NODE_LVL_L0 : 
                           SOC_TR3_NODE_LVL_ROOT;
                 }

                 if (node->level == -1) {
                     node->level = 
                         (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport) ||
                          BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport) ||
                          BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(gport)) ?
                         SOC_TR3_NODE_LVL_L2 : SOC_TR3_NODE_LVL_L0;
                 }
             } else {
                 if ((to_node->level == -1)) {
                     to_node->level = (BCM_GPORT_IS_SCHEDULER(gport)) ?
                         SOC_TR3_NODE_LVL_L0 : SOC_TR3_NODE_LVL_L1;
                 }

                 if (node->level == -1) {
                     node->level = 
                       (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport) ||
                        BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport) ||
                        BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(gport)) ?
                       SOC_TR3_NODE_LVL_L2 : SOC_TR3_NODE_LVL_L1;
                 }
             }
        }
    }

    if ((to_cosq < -1) || ((to_node->numq != -1) && (to_cosq >= to_node->numq))) {
        return BCM_E_PARAM;
    }

    if (BCM_GPORT_IS_SCHEDULER(to_gport) || BCM_GPORT_IS_LOCAL(to_gport) ||
        BCM_GPORT_IS_MODPORT(to_gport)) {
        if (to_node->attached_to_input < 0) {
            /* dont allow to attach to a node that has already attached */
            return BCM_E_PARAM;
        }

            node->parent = to_node;
            node->sibling = to_node->child;
            to_node->child = node;
            /* resolve the nodes */
            rv = _bcm_tr3_cosq_node_resolve(unit, node, to_cosq);
            if (BCM_FAILURE(rv)) {
                to_node->child = node->sibling;
                node->parent = NULL;
                return rv;
            }
            LOG_INFO(BSL_LS_BCM_COSQ,
                     (BSL_META_U(unit,
                                 "                         hw_cosq=%d\n"),
                      node->attached_to_input));

    } else {
            return BCM_E_PORT;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_attach_get
 * Purpose:
 *     Get attached status of a scheduler port
 * Parameters:
 *     unit       - (IN) unit number
 *     sched_port - (IN) scheduler GPORT identifier
 *     input_port - (OUT) GPORT to attach to
 *     cosq       - (OUT) COS queue to attached to
 * Returns:
 *     BCM_E_XXX
 * Notes:
 */
int
bcm_tr3_cosq_gport_attach_get(int unit, bcm_gport_t sched_gport,
                             bcm_gport_t *input_gport, bcm_cos_queue_t *cosq)
{
    _bcm_tr3_cosq_node_t *sched_node;
    bcm_module_t modid, modid_out;
    bcm_port_t port, port_out;

    if (input_gport == NULL || cosq == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_node_get(unit, sched_gport, 0, &modid, &port, NULL,
                               &sched_node));

    if (sched_node->attached_to_input < 0) {
        /* Not attached yet */
        return BCM_E_NOT_FOUND;
    }

    if (sched_node->parent != NULL) { /* sched_node is not an S1 scheduler */
        *input_gport = sched_node->parent->gport;
    } else {  /* sched_node is an S1 scheduler */
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, modid, port,
                                    &modid_out, &port_out));
        BCM_GPORT_MODPORT_SET(*input_gport, modid_out, port_out);
    }
    *cosq = sched_node->attached_to_input;

    return BCM_E_NONE;
}

/*
 *  Convert HW drop probability to percent value
 */
static int
_bcm_tr3_hw_drop_prob_to_percent[] = {
    0,     /* 0  */
    1,     /* 1  */
    2,     /* 2  */
    3,     /* 3  */
    4,     /* 4  */
    5,     /* 5  */
    6,     /* 6  */
    7,     /* 7  */
    8,     /* 8  */
    9,     /* 9  */
    10,    /* 10 */
    25,    /* 11 */
    50,    /* 12 */
    75,    /* 13 */
    100,   /* 14 */
    -1     /* 15 */
};

STATIC int
_bcm_tr3_percent_to_drop_prob(int percent)
{
   int i;

   for (i = 14; i > 0 ; i--) {
      if (percent >= _bcm_tr3_hw_drop_prob_to_percent[i]) {
          break;
      }
   }
   return i;
}

STATIC int
_bcm_tr3_drop_prob_to_percent(int drop_prob) {
   return (_bcm_tr3_hw_drop_prob_to_percent[drop_prob]);
}

/*
 * index: degree, value: contangent(degree) * 100
 * max value is 0xffff (16-bit) at 0 degree
 */
static int
_bcm_tr3_cotangent_lookup_table[] =
{
    /*  0.. 5 */  65535, 5728, 2863, 1908, 1430, 1143,
    /*  6..11 */    951,  814,  711,  631,  567,  514,
    /* 12..17 */    470,  433,  401,  373,  348,  327,
    /* 18..23 */    307,  290,  274,  260,  247,  235,
    /* 24..29 */    224,  214,  205,  196,  188,  180,
    /* 30..35 */    173,  166,  160,  153,  148,  142,
    /* 36..41 */    137,  132,  127,  123,  119,  115,
    /* 42..47 */    111,  107,  103,  100,   96,   93,
    /* 48..53 */     90,   86,   83,   80,   78,   75,
    /* 54..59 */     72,   70,   67,   64,   62,   60,
    /* 60..65 */     57,   55,   53,   50,   48,   46,
    /* 66..71 */     44,   42,   40,   38,   36,   34,
    /* 72..77 */     32,   30,   28,   26,   24,   23,
    /* 78..83 */     21,   19,   17,   15,   14,   12,
    /* 84..89 */     10,    8,    6,    5,    3,    1,
    /* 90     */      0
};

/*
 * Given a slope (angle in degrees) from 0 to 90, return the number of cells
 * in the range from 0% drop probability to 100% drop probability.
 */
STATIC int
_bcm_tr3_angle_to_cells(int angle)
{
    return (_bcm_tr3_cotangent_lookup_table[angle]);
}

/*
 * Given a number of packets in the range from 0% drop probability
 * to 100% drop probability, return the slope (angle in degrees).
 */
STATIC int
_bcm_tr3_cells_to_angle(int packets)
{
    int angle;

    for (angle = 90; angle >= 0 ; angle--) {
        if (packets <= _bcm_tr3_cotangent_lookup_table[angle]) {
            break;
        }
    }
    return angle;
}

/*
 * Function:
 *     _bcm_tr3_cosq_bucket_set
 * Purpose:
 *     Configure COS queue bandwidth control bucket
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     min_quantum   - (IN) kbps or packet/second
 *     max_quantum   - (IN) kbps or packet/second
 *     burst_quantum - (IN) kbps or packet/second
 *     flags         - (IN)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
STATIC int
_bcm_tr3_cosq_bucket_set(int unit, bcm_gport_t port, bcm_cos_queue_t cosq,
                        uint32 min_quantum, uint32 max_quantum,
                        uint32 kbits_burst_min, uint32 kbits_burst_max, 
                        uint32 flags)
{
    _bcm_tr3_cosq_node_t *node = NULL;
    bcm_port_t local_port;
    int index;
    uint32 rval;
    uint32 meter_flags;
    uint32 bucketsize_max, bucketsize_min;
    uint32 granularity_max, granularity_min;
    uint32 refresh_rate_max, refresh_rate_min;
    int refresh_bitsize = 0, bucket_bitsize = 0;
    soc_mem_t config_mem = INVALIDm;
    uint32 entry[SOC_MAX_MEM_WORDS];

    /* caller should resolve the cosq. */
    if (cosq < 0) {
        if (cosq == -1) {
            /* caller needs to resolve the wild card value (-1) */
            return BCM_E_INTERNAL;
        } else { /* reject all other negative value */
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_BUCKET,
                                    &local_port, &index, NULL));

    if (BCM_GPORT_IS_SET(port) && 
        ((BCM_GPORT_IS_SCHEDULER(port)) ||
          BCM_GPORT_IS_UCAST_QUEUE_GROUP(port) ||
          BCM_GPORT_IS_MCAST_QUEUE_GROUP(port) ||
          BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(port))) {

        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_node_get(unit, port, cosq, NULL, 
                                    NULL, NULL, &node));
    }

    if (node) {
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            if (node->level == SOC_TR3_NODE_LVL_ROOT) {
                config_mem = MMU_MTRO_L0_MEMm;
            } else if (node->level == SOC_TR3_NODE_LVL_L0) {
                config_mem = MMU_MTRO_L1_MEMm;
            } else if (node->level == SOC_TR3_NODE_LVL_L1) {
                config_mem = MMU_MTRO_L2_MEMm;
            } else {
                return BCM_E_PARAM;
            }
        } else if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port) ||
                   BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            config_mem = MMU_MTRO_L2_MEMm;
        } else {
            return BCM_E_PARAM;
        }
    } else {
        if (IS_CPU_PORT(unit, local_port)) {
            config_mem = MMU_MTRO_L2_MEMm;
        } else {
            config_mem = MMU_MTRO_L1_MEMm;
        }
    }

    meter_flags = flags & BCM_COSQ_BW_PACKET_MODE ?
                            _BCM_TD_METER_FLAG_PACKET_MODE : 0;
    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &rval));
    if (soc_reg_field_get(unit, MISCCONFIGr, rval, ITU_MODE_SELf)) {
        meter_flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    refresh_bitsize = soc_mem_field_length(unit, config_mem, MAX_REFRESHf);
    bucket_bitsize = soc_mem_field_length(unit, config_mem, MAX_THD_SELf);

    BCM_IF_ERROR_RETURN
        (_bcm_td_rate_to_bucket_encoding(unit, max_quantum, kbits_burst_max,
                                         meter_flags, refresh_bitsize,
                                         bucket_bitsize, &refresh_rate_max,
                                         &bucketsize_max, &granularity_max));

    BCM_IF_ERROR_RETURN
        (_bcm_td_rate_to_bucket_encoding(unit, min_quantum, kbits_burst_min,
                                         meter_flags, refresh_bitsize,
                                         bucket_bitsize, &refresh_rate_min,
                                         &bucketsize_min, &granularity_min));

    sal_memset(entry, 0, sizeof(uint32)*SOC_MAX_MEM_WORDS);
    soc_mem_field32_set(unit, config_mem, &entry, MAX_METER_GRANf, granularity_max);
    soc_mem_field32_set(unit, config_mem, &entry, MAX_REFRESHf, refresh_rate_max);
    soc_mem_field32_set(unit, config_mem, &entry, MAX_THD_SELf, bucketsize_max);

    soc_mem_field32_set(unit, config_mem, &entry, MIN_METER_GRANf, granularity_min);
    soc_mem_field32_set(unit, config_mem, &entry, MIN_REFRESHf, refresh_rate_min);
    soc_mem_field32_set(unit, config_mem, &entry, MIN_THD_SELf, bucketsize_min);

    soc_mem_field32_set(unit, config_mem, &entry, SHAPER_CONTROLf,
                        (flags & BCM_COSQ_BW_PACKET_MODE) ? 1 : 0);

    BCM_IF_ERROR_RETURN
        (soc_mem_write(unit, config_mem, MEM_BLOCK_ALL, index, &entry));
    
    if ((node==NULL) && (!IS_CPU_PORT(unit, local_port)) &&
        ((!soc_feature(unit, soc_feature_tr3_sp_vector_mask)) &&
         soc_property_port_get(unit, local_port, spn_PORT_SCHED_DYNAMIC, 0))) {
        if (soc_port_hg_capable(unit, port)) {
            /* As the 8th index is meant for SC/QM adding offset of 9 on dyn Q's */
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, config_mem, MEM_BLOCK_ALL, index + 9, &entry));
        } else {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, config_mem, MEM_BLOCK_ALL, index + 8, &entry));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_bucket_get
 * Purpose:
 *     Get COS queue bandwidth control bucket setting
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     min_quantum   - (OUT) kbps or packet/second
 *     max_quantum   - (OUT) kbps or packet/second
 *     burst_quantum - (OUT) kbps or packet/second
 *     flags         - (IN)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
STATIC int
_bcm_tr3_cosq_bucket_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                        uint32 *min_quantum, uint32 *max_quantum,
                        uint32 *kbits_burst_min, uint32 *kbits_burst_max,
                        uint32 *flags)
{
    _bcm_tr3_cosq_node_t *node = NULL;
    bcm_port_t local_port;
    int index;
    uint32 rval;
    uint32 refresh_rate, bucketsize, granularity, meter_flags;
    soc_mem_t config_mem = INVALIDm;
    uint32 entry[SOC_MAX_MEM_WORDS];

    if (cosq < 0) {
        if (cosq == -1) {
            /* caller needs to resolve the wild card value (-1) */
            return BCM_E_INTERNAL;
        } else { /* reject all other negative value */
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_BUCKET,
                                    &local_port, &index, NULL));

    if (BCM_GPORT_IS_SET(port) && 
        ((BCM_GPORT_IS_SCHEDULER(port)) ||
          BCM_GPORT_IS_UCAST_QUEUE_GROUP(port) ||
          BCM_GPORT_IS_MCAST_QUEUE_GROUP(port) ||
          BCM_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(port))) {

        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_node_get(unit, port, cosq, NULL, 
                                    &local_port, NULL, &node));
    }

    if (node) {
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            if (node->level == SOC_TR3_NODE_LVL_ROOT) {
                config_mem = MMU_MTRO_L0_MEMm;
            } else if (node->level == SOC_TR3_NODE_LVL_L0) {
                config_mem = MMU_MTRO_L1_MEMm;
            } else if (node->level == SOC_TR3_NODE_LVL_L1) {
                config_mem = MMU_MTRO_L2_MEMm;
            } else {
                return BCM_E_PARAM;
            }
        } else if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port) ||
                   BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            config_mem = MMU_MTRO_L2_MEMm;
        } else {
            return BCM_E_PARAM;
        }
    } else {
        if (IS_CPU_PORT(unit, local_port)) {
            config_mem = MMU_MTRO_L2_MEMm;
        } else {
            config_mem = MMU_MTRO_L1_MEMm;
        }
    }

    if (min_quantum == NULL || max_quantum == NULL || 
        kbits_burst_max == NULL || kbits_burst_min == NULL) {
        return BCM_E_PARAM;
    }

    #ifdef DEBUG_TR3_COSQ
    LOG_CLI((BSL_META_U(unit,
                        "_bcm_tr3_cosq_bucket_get : gport=0x%08x Index : %d mem=%d\n"),
             port, index, config_mem));
    #endif

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, config_mem, MEM_BLOCK_ALL, index, &entry));

    meter_flags = 0;
    *flags = 0;
    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &rval));
    if (soc_reg_field_get(unit, MISCCONFIGr, rval, ITU_MODE_SELf)) {
        meter_flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }
    if (soc_mem_field32_get(unit, config_mem, &entry, SHAPER_CONTROLf)) {
        meter_flags |= _BCM_TD_METER_FLAG_PACKET_MODE;
        *flags |= BCM_COSQ_BW_PACKET_MODE;
    }

    granularity = soc_mem_field32_get(unit, config_mem, &entry, MAX_METER_GRANf);
    refresh_rate = soc_mem_field32_get(unit, config_mem, &entry, MAX_REFRESHf);
    bucketsize = soc_mem_field32_get(unit, config_mem, &entry, MAX_THD_SELf);
    BCM_IF_ERROR_RETURN
        (_bcm_td_bucket_encoding_to_rate(unit, refresh_rate, bucketsize,
                                           granularity, meter_flags,
                                           max_quantum, kbits_burst_max));

    granularity = soc_mem_field32_get(unit, config_mem, &entry, MIN_METER_GRANf);
    refresh_rate = soc_mem_field32_get(unit, config_mem, &entry, MIN_REFRESHf);
    bucketsize = soc_mem_field32_get(unit, config_mem, &entry, MIN_THD_SELf);
    BCM_IF_ERROR_RETURN
        (_bcm_td_bucket_encoding_to_rate(unit, refresh_rate, bucketsize,
                                           granularity, meter_flags,
                                           min_quantum, kbits_burst_min));
    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_wred_set
 * Purpose:
 *     Configure unicast queue WRED setting
 * Parameters:
 *     unit                - (IN) unit number
 *     port                - (IN) port number or GPORT identifier
 *     cosq                - (IN) COS queue number
 *     flags               - (IN) BCM_COSQ_DISCARD_XXX
 *     min_thresh          - (IN)
 *     max_thresh          - (IN)
 *     drop_probablity     - (IN)
 *     gain                - (IN)
 *     ignore_enable_flags - (IN)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
STATIC int
_bcm_tr3_cosq_wred_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                      uint32 flags, uint32 min_thresh, uint32 max_thresh,
                      int drop_probability, int gain, int ignore_enable_flags,
                      uint32 lflags)
{
    soc_mem_t wred_mem;
    bcm_port_t local_port;
    int index, to_index;
    uint32 profile_index, old_profile_index;
    mmu_wred_drop_curve_profile_0_entry_t entry_tcp_green;
    mmu_wred_drop_curve_profile_1_entry_t entry_tcp_yellow;
    mmu_wred_drop_curve_profile_2_entry_t entry_tcp_red;
    mmu_wred_drop_curve_profile_3_entry_t entry_nontcp_green;
    mmu_wred_drop_curve_profile_4_entry_t entry_nontcp_yellow;
    mmu_wred_drop_curve_profile_5_entry_t entry_nontcp_red;
    mmu_wred_config_entry_t qentry;
    void *entries[6];
    soc_mem_t mems[6];
    int rate, i, jj, exists = 0;
    static soc_mem_t wred_mems[6] = {
        MMU_WRED_DROP_CURVE_PROFILE_0m, MMU_WRED_DROP_CURVE_PROFILE_1m,
        MMU_WRED_DROP_CURVE_PROFILE_2m, MMU_WRED_DROP_CURVE_PROFILE_3m,
        MMU_WRED_DROP_CURVE_PROFILE_4m, MMU_WRED_DROP_CURVE_PROFILE_5m
    };

    if ((port == -1) && (lflags & BCM_COSQ_DISCARD_DEVICE)) {
        index = 1532;
        to_index = 1535;
    } else {
        if (lflags & BCM_COSQ_DISCARD_PORT) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                     _BCM_TR3_COSQ_INDEX_STYLE_WRED_PORT,
                                     &local_port, &index, NULL));
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                     _BCM_TR3_COSQ_INDEX_STYLE_WRED,
                                     &local_port, &index, NULL));
        }
        to_index = index;
    }

    wred_mem = MMU_WRED_CONFIGm;
    
    for (jj = index; jj <= to_index; jj++) {
        BCM_IF_ERROR_RETURN
            (soc_mem_read(unit, wred_mem, MEM_BLOCK_ALL, jj, &qentry));

        old_profile_index = 0xffffffff;
        if (flags & (BCM_COSQ_DISCARD_NONTCP | BCM_COSQ_DISCARD_COLOR_ALL |
                     BCM_COSQ_DISCARD_TCP)) {
            old_profile_index = soc_mem_field32_get(unit, wred_mem, 
                                                    &qentry, PROFILE_INDEXf);
            entries[0] = &entry_tcp_green;
            entries[1] = &entry_tcp_yellow;
            entries[2] = &entry_tcp_red;
            entries[3] = &entry_nontcp_green;
            entries[4] = &entry_nontcp_yellow;
            entries[5] = &entry_nontcp_red;
            BCM_IF_ERROR_RETURN
                (soc_profile_mem_get(unit, _bcm_tr3_wred_profile[unit],
                                     old_profile_index, 1, entries));
            for (i = 0; i < 6; i++) {
                mems[i] = INVALIDm;
            }
            if (!(flags & (BCM_COSQ_DISCARD_COLOR_GREEN |
                           BCM_COSQ_DISCARD_COLOR_YELLOW |
                           BCM_COSQ_DISCARD_COLOR_RED))) {
                flags |= BCM_COSQ_DISCARD_COLOR_ALL;
            }
            if (!(flags & BCM_COSQ_DISCARD_NONTCP) || (flags & BCM_COSQ_DISCARD_TCP)) {
                if (flags & BCM_COSQ_DISCARD_COLOR_GREEN) {
                    mems[0] = MMU_WRED_DROP_CURVE_PROFILE_0m;
                }
                if (flags & BCM_COSQ_DISCARD_COLOR_YELLOW) {
                    mems[1] = MMU_WRED_DROP_CURVE_PROFILE_1m;
                }
                if (flags & BCM_COSQ_DISCARD_COLOR_RED) {
                    mems[2] = MMU_WRED_DROP_CURVE_PROFILE_2m;
                }
            }
            if (flags & BCM_COSQ_DISCARD_NONTCP) {
                if (flags & BCM_COSQ_DISCARD_COLOR_GREEN) {
                    mems[3] = MMU_WRED_DROP_CURVE_PROFILE_3m;
                }
                if (flags & BCM_COSQ_DISCARD_COLOR_YELLOW) {
                    mems[4] = MMU_WRED_DROP_CURVE_PROFILE_4m;
                }
                if (flags & BCM_COSQ_DISCARD_COLOR_RED) {
                    mems[5] = MMU_WRED_DROP_CURVE_PROFILE_5m;
                }
            }
            rate = _bcm_tr3_percent_to_drop_prob(drop_probability);
            for (i = 0; i < 6; i++) {
                exists = 0;
                if ((soc_mem_field32_get(unit, wred_mems[i], entries[i], 
                              MIN_THDf) != 0x7fff) && (mems[i] == INVALIDm)) {
                    mems[i] = wred_mems[i];
                    exists = 1;
                } else {
                    soc_mem_field32_set(unit, wred_mems[i], entries[i], MIN_THDf,
                                 (mems[i] == INVALIDm) ? 0x7fff : min_thresh);
                }

                if ((soc_mem_field32_get(unit, wred_mems[i], entries[i], 
                     MAX_THDf) != 0x7fff) && ((mems[i] == INVALIDm) || exists)) {
                    mems[i] = wred_mems[i];
                    exists = 1;
                } else {
                    soc_mem_field32_set(unit, wred_mems[i], entries[i], MAX_THDf,
                                 (mems[i] == INVALIDm) ? 0x7fff : max_thresh);
                }

                if (!exists) {
                    soc_mem_field32_set(unit, wred_mems[i], entries[i], MAX_DROP_RATEf,
                                 (mems[i] == INVALIDm) ? 0 : rate);
                }
            }

            BCM_IF_ERROR_RETURN
                (soc_profile_mem_add(unit, _bcm_tr3_wred_profile[unit], entries, 1,
                                     &profile_index));
            soc_mem_field32_set(unit, wred_mem, &qentry, PROFILE_INDEXf, profile_index);
            soc_mem_field32_set(unit, wred_mem, &qentry, WEIGHTf, gain);
        }

        /* Some APIs only modify profiles */
        if (!ignore_enable_flags) {
            soc_mem_field32_set(unit, wred_mem, &qentry, CAP_AVERAGEf,
                              flags & BCM_COSQ_DISCARD_CAP_AVERAGE ? 1 : 0);
            soc_mem_field32_set(unit, wred_mem, &qentry, WRED_ENf,
                              flags & BCM_COSQ_DISCARD_ENABLE ? 1 : 0);
            soc_mem_field32_set(unit, wred_mem, &qentry, ECN_MARKING_ENf,
                              flags & BCM_COSQ_DISCARD_MARK_CONGESTION ?  1 : 0);
        }
        BCM_IF_ERROR_RETURN(
              soc_mem_write(unit, wred_mem, MEM_BLOCK_ALL, jj, &qentry));

        if (old_profile_index != 0xffffffff) {

            SOC_IF_ERROR_RETURN
                (soc_profile_mem_delete(unit, _bcm_tr3_wred_profile[unit],
                                        old_profile_index));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_wred_get
 * Purpose:
 *     Get unicast queue WRED setting
 * Parameters:
 *     unit                - (IN) unit number
 *     port                - (IN) port number or GPORT identifier
 *     cosq                - (IN) COS queue number
 *     flags               - (IN/OUT) BCM_COSQ_DISCARD_XXX
 *     min_thresh          - (OUT)
 *     max_thresh          - (OUT)
 *     drop_probablity     - (OUT)
 *     gain                - (OUT)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
STATIC int
_bcm_tr3_cosq_wred_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                      uint32 *flags, uint32 *min_thresh, uint32 *max_thresh,
                      int *drop_probability, int *gain, uint32 lflags)
{
    bcm_port_t local_port;
    int index, profile_index;
    mmu_wred_drop_curve_profile_0_entry_t entry_tcp_green;
    mmu_wred_drop_curve_profile_1_entry_t entry_tcp_yellow;
    mmu_wred_drop_curve_profile_2_entry_t entry_tcp_red;
    mmu_wred_drop_curve_profile_3_entry_t entry_nontcp_green;
    mmu_wred_drop_curve_profile_4_entry_t entry_nontcp_yellow;
    mmu_wred_drop_curve_profile_5_entry_t entry_nontcp_red;
    void *entries[6];
    void *entry_p;
    soc_mem_t mem;
    soc_mem_t wred_mem = 0;
    mmu_wred_config_entry_t qentry;
    int rate;

    if ((port == -1) && (lflags & BCM_COSQ_DISCARD_DEVICE)) {
        index = 1532;
    } else {
        if (lflags & BCM_COSQ_DISCARD_PORT) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                     _BCM_TR3_COSQ_INDEX_STYLE_WRED_PORT,
                                     &local_port, &index, NULL));
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                     _BCM_TR3_COSQ_INDEX_STYLE_WRED,
                                     &local_port, &index, NULL));
        }
    }

    wred_mem = MMU_WRED_CONFIGm;

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, wred_mem, MEM_BLOCK_ALL, index, &qentry));
    profile_index = soc_mem_field32_get(unit, wred_mem,
                                        &qentry, PROFILE_INDEXf);

    if (!(*flags & BCM_COSQ_DISCARD_NONTCP)) {
        if (*flags & BCM_COSQ_DISCARD_COLOR_GREEN) {
            mem = MMU_WRED_DROP_CURVE_PROFILE_0m;
            entry_p = &entry_tcp_green;
        } else if (*flags & BCM_COSQ_DISCARD_COLOR_YELLOW) {
            mem = MMU_WRED_DROP_CURVE_PROFILE_1m;
            entry_p = &entry_tcp_yellow;
        } else if (*flags & BCM_COSQ_DISCARD_COLOR_RED) {
            mem = MMU_WRED_DROP_CURVE_PROFILE_2m;
            entry_p = &entry_tcp_red;
        } else {
           mem = MMU_WRED_DROP_CURVE_PROFILE_0m;
           entry_p = &entry_tcp_green;
        }
    } else {
        if (*flags & BCM_COSQ_DISCARD_COLOR_GREEN) {
            mem = MMU_WRED_DROP_CURVE_PROFILE_3m;
            entry_p = &entry_nontcp_green;
        } else if (*flags & BCM_COSQ_DISCARD_COLOR_YELLOW) {
            mem = MMU_WRED_DROP_CURVE_PROFILE_4m;
            entry_p = &entry_nontcp_yellow;
        } else if (*flags & BCM_COSQ_DISCARD_COLOR_RED) {
            mem = MMU_WRED_DROP_CURVE_PROFILE_5m;
            entry_p = &entry_nontcp_red;
        } else {
           mem = MMU_WRED_DROP_CURVE_PROFILE_3m;
           entry_p = &entry_nontcp_green;
        }
    }

    entries[0] = &entry_tcp_green;
    entries[1] = &entry_tcp_yellow;
    entries[2] = &entry_tcp_red;
    entries[3] = &entry_nontcp_green;
    entries[4] = &entry_nontcp_yellow;
    entries[5] = &entry_nontcp_red;
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_get(unit, _bcm_tr3_wred_profile[unit],
                             profile_index, 1, entries));
    if (min_thresh != NULL) {
        *min_thresh = soc_mem_field32_get(unit, mem, entry_p, MIN_THDf);
    }
    if (max_thresh != NULL) {
        *max_thresh = soc_mem_field32_get(unit, mem, entry_p, MAX_THDf);
    }
    if (drop_probability != NULL) {
        rate = soc_mem_field32_get(unit, mem, entry_p, MAX_DROP_RATEf);
        *drop_probability = _bcm_tr3_drop_prob_to_percent(rate);
    }

    if (gain) {
        *gain = soc_mem_field32_get(unit, wred_mem, &qentry, WEIGHTf);
    }

    *flags &= ~(BCM_COSQ_DISCARD_CAP_AVERAGE | BCM_COSQ_DISCARD_ENABLE);
    if (soc_mem_field32_get(unit, wred_mem, &qentry, CAP_AVERAGEf)) {
        *flags |= BCM_COSQ_DISCARD_CAP_AVERAGE;
    }
    if (soc_mem_field32_get(unit, wred_mem, &qentry, WRED_ENf)) {
        *flags |= BCM_COSQ_DISCARD_ENABLE;
    }
    if (soc_mem_field32_get(unit, wred_mem, &qentry, ECN_MARKING_ENf)) {
        *flags |= BCM_COSQ_DISCARD_MARK_CONGESTION;
    }
    return BCM_E_NONE;
}
    
STATIC int
_bcm_tr3_decode_sp_masks(int num_sp, uint32 ucmap, uint32 *umap, int *numuc,
                            uint32 *mmap, int *nummc)
{
    int i, uindex, mindex;
    uint32 tmp_umap = 0, tmp_mmap = 0;
    
    mindex = uindex = 0;

    for (i = 0; i < num_sp; i++) {
        if (ucmap & (1 << i)) {
            /* MC */
            tmp_mmap |= 1 << mindex;
            mindex += 1;
        } else {
            /* UC */
            tmp_umap |= 1 << uindex;
            uindex += 1;
        }
    }

    if (numuc) {
        *numuc = uindex;
    }

    if (umap) {
        *umap = tmp_umap;
    }

    if (mmap) {
        *mmap = tmp_mmap;
    }

    if (nummc) {
        *nummc = mindex;
    }

    return 0;
}

STATIC uint32 _bcm_tr3_num_ones(uint32 m)
{
    uint32 ii = 0;

    while (m) {
        m &= m - 1;
        ii +=  1;
    }
    return ii;
}


STATIC int
_bcm_tr3_compute_lls_config_b0(int unit, int port, int level, 
                                 int *first_sp_child, 
                                 int *first_sp_mc_child,
                                 int *num_spri,
                                 uint32 *ucmap, 
                                 uint32 *spmap, 
                                 int child_index, 
                                 soc_tr3_sched_mode_e cur_mode,
                                 soc_tr3_sched_mode_e mode,
                                 int max_children)
{
    uint32 ucm = 0, mcm = 0, cur_spmap, cur_num_spri, lucmap = 0;
    int ii, *pfc, uc, mc, fc, oc, *use_fc;

    cur_num_spri = _bcm_tr3_num_ones(*spmap);
    cur_spmap = *spmap;

    if (level == SOC_TR3_NODE_LVL_L1) {
        if (IS_CPU_PORT(unit, port)) {
            use_fc = first_sp_mc_child;
        } else {
            use_fc = (child_index >= 1024) ? first_sp_mc_child : first_sp_child;
        }
    } else {
        use_fc = first_sp_child;
    }
    
    if (cur_num_spri == 0) {
        *ucmap = 0;
        *spmap = 0;
    }

    if (child_index > *use_fc) {
        if (((child_index - *use_fc) > max_children)) {
            return BCM_E_PARAM;
        }
    } else {
        if (((*use_fc + cur_num_spri) - child_index) > max_children) {
            return BCM_E_PARAM;
        }
    }
    
    if (level == SOC_TR3_NODE_LVL_L1) {
        for (mcm = 0, mc = 0, ucm = 0, uc = 0, ii = 0; ii < 8; ii++) {
            if (*ucmap & (1 << ii)) {
                mcm |= (cur_spmap & (1 << ii)) ? (1 << mc) : 0;
                mc += 1;
            } else {
                ucm |= (cur_spmap & (1 << ii)) ? (1 << uc) : 0;
                uc += 1;
            }
        }

        if (child_index < 1024) {
            fc = *first_sp_child;
            oc =  _bcm_tr3_num_ones(ucm);
            BCM_IF_ERROR_RETURN(_bcm_tr3_compute_lls_config_b0(unit, port,
                        level - 1, &fc, NULL, &oc, &lucmap, &ucm, child_index, 
                        cur_mode, mode, 8));
        } else {
            fc = *first_sp_mc_child;
            oc =  _bcm_tr3_num_ones(mcm);
            BCM_IF_ERROR_RETURN(_bcm_tr3_compute_lls_config_b0(unit, port,
                    level - 1, &fc, NULL, &oc, &lucmap, &mcm, 
                    child_index, cur_mode, mode, 8));
        }

        cur_spmap = 0;
        *ucmap = 0;
        for (fc = 0, ii = 0; ii < 8; ii++) {
            if (ucm) {
                cur_spmap |= (ucm & (1 << ii)) ? 1 << fc : 0;
                fc += 1;
                ucm &= ~(1 << ii);
            }
            
            if (mcm) {
                cur_spmap |= (mcm & (1 << ii)) ? 1 << fc : 0;
                *ucmap |= 1 << fc;
                fc += 1;
                mcm &= ~(1 << ii);
            }

            if (fc > max_children) {
                return BCM_E_PARAM;
            }

            if ((mcm == 0) && (ucm == 0)) {
                break;
            }
        }

    } else {
        pfc = first_sp_child;
        if (mode == SOC_TR3_SCHED_MODE_STRICT) {
            if (*pfc > child_index) {
                cur_spmap <<= (*pfc - child_index);
                cur_spmap |= 1;
                *pfc = child_index;
                *num_spri += 1;
            } else {
                if (((1 << (child_index - *pfc)) & cur_spmap) == 0) {
                    cur_spmap |= 1 << (child_index - *pfc);
                    *num_spri += 1;
                }
            }
        } else {
            if (1 << (child_index - *pfc) & cur_spmap) {
                cur_spmap &= ~(1 << (child_index - *pfc));
                *num_spri -= 1;
            }
        }
    }

    *num_spri = _bcm_tr3_num_ones(cur_spmap);

    *spmap = cur_spmap;

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_compute_lls_config(int unit, int port, int level, 
                                 int *first_sp_child, 
                                 int *first_sp_mc_child,
                                 int *num_spri,
                                 uint32 *ucmap, 
                                 uint32 *spmap, 
                                 int child_index, 
                                 soc_tr3_sched_mode_e cur_mode,
                                 soc_tr3_sched_mode_e mode)
{
    int cur_first_child;
    int cur_num_spri;
    int cur_ucmap;
    int diff;
    uint32 umap = 0, mmap = 0;
    int  numuc = 0, nummc = 0;
    uint32 pnumn = 0;

    if (((cur_mode != SOC_TR3_SCHED_MODE_STRICT) &&
        (mode != SOC_TR3_SCHED_MODE_STRICT)) ||
       ((cur_mode == SOC_TR3_SCHED_MODE_STRICT) && 
        (mode == SOC_TR3_SCHED_MODE_STRICT))) {
        return BCM_E_NONE;
    }

    if (soc_feature(unit, soc_feature_tr3_sp_vector_mask)) {
        return _bcm_tr3_compute_lls_config_b0(unit, port, level, first_sp_child,
                                         first_sp_mc_child, num_spri,
                                         ucmap, spmap, child_index,
                                         cur_mode, mode, 8);
    }

    cur_num_spri = *num_spri;
    cur_ucmap = *ucmap;
  
    if (level == SOC_TR3_NODE_LVL_L1) {
        _bcm_tr3_decode_sp_masks(cur_num_spri, cur_ucmap, &umap, 
                                            &numuc, &mmap, &nummc);
        if (child_index < 1024) {
            /* UC */
            pnumn = numuc;
            cur_first_child = *first_sp_child;
        } else {
            /* MC */
            pnumn = nummc;
            cur_first_child = *first_sp_mc_child;
        }
    } else {
        pnumn = cur_num_spri;
        cur_first_child = *first_sp_child;
    }

    if (cur_mode == SOC_TR3_SCHED_MODE_STRICT) {
        /* this node is not in the middle of SP nodes */
        if (child_index != (cur_first_child + pnumn - 1)) {
            return BCM_E_UNAVAIL;
        }

        cur_num_spri -= 1;
        pnumn -= 1;

        if (level == SOC_TR3_NODE_LVL_L1) {
            if (child_index < 1024) {
                *num_spri = pnumn + nummc;
                *ucmap = (1 << pnumn) - 1;
            } else {
                *num_spri = pnumn + numuc;
                *ucmap = (1 << numuc) - 1;
            }
        } else {
            *num_spri = cur_num_spri;
            *ucmap = cur_num_spri ? (1 << cur_num_spri) -1 : 0;
        }
        if (*num_spri == 0) {
            *ucmap = 0;
        }
    } else {
        if (cur_num_spri) {
            /* adding new sp */
            diff = child_index - cur_first_child;
            if (diff != cur_num_spri) {
                return BCM_E_UNAVAIL;
            }

            if (level == SOC_TR3_NODE_LVL_L1) {
                if (child_index < 1024) {
                    pnumn += 1;
                    *num_spri = pnumn + nummc;
                    *ucmap = (1 << pnumn) - 1;
                } else {
                    pnumn += 1;
                    *num_spri = pnumn + numuc;
                    *ucmap = (1 << numuc) - 1;
                }
            } else {
                pnumn += 1;
                *num_spri = pnumn;
                *ucmap = (1 << pnumn) - 1;
            }
        } else {
            *num_spri = 1;
            *ucmap = 1;
        }
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_dyn_move_queue(int unit, bcm_port_t local_port, 
                             int cosq, int *l2_index, int numl2,
                             int from_sched_idx, int to_sched_index)
{
    lls_l2_parent_entry_t l2p[2];
    mmu_mtro_l2_mem_entry_t mtro[2], tmp_mtro;
    mmu_mtro_l1_mem_entry_t l1mtro, l1mtro_tmp;
    lls_l2_child_state1_entry_t l2cse;
    lls_l1_parent_state_entry_t l1pe_old;
    int ii, empty_count, actives[2], sync;
    uint32 rval;
    int rv = BCM_E_NONE;
    soc_timeout_t timeout;
    uint32 timeout_val;
    int rcnt = 0;

    timeout_val = soc_property_get(unit, spn_MMU_QUEUE_FLUSH_TIMEOUT, 2000000);

    for (ii = 0; ii < numl2; ii++) {
        SOC_IF_ERROR_RETURN(READ_MMU_MTRO_L2_MEMm(unit, MEM_BLOCK_ANY,
                                                  l2_index[ii], &mtro[ii]));
        /* configure max shaper */
        sal_memset(&tmp_mtro, 0, sizeof(mmu_mtro_l2_mem_entry_t));
        soc_mem_field32_set(unit, MMU_MTRO_L2_MEMm, &tmp_mtro, MAX_METER_GRANf, 0);
        soc_mem_field32_set(unit, MMU_MTRO_L2_MEMm, &tmp_mtro, MAX_THD_SELf, 1);
        soc_mem_field32_set(unit, MMU_MTRO_L2_MEMm, &tmp_mtro, MAX_REFRESHf, 0);
        SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L2_MEMm(unit, MEM_BLOCK_ANY,
                                                    l2_index[ii], &tmp_mtro));
        /* cache the L2 parent mem. */
        SOC_IF_ERROR_RETURN(READ_LLS_L2_PARENTm(unit, MEM_BLOCK_ANY,
                                                l2_index[ii], &l2p[ii]));
        soc_mem_field32_set(unit, LLS_L2_PARENTm, &l2p[ii], C_PARENTf, to_sched_index);
    }

    /* give l1 a small min guarantee */
    SOC_IF_ERROR_RETURN(READ_MMU_MTRO_L1_MEMm(unit, MEM_BLOCK_ANY, 
                                              from_sched_idx, &l1mtro));
    sal_memset(&l1mtro_tmp, 0, sizeof(sizeof(mmu_mtro_l1_mem_entry_t)));
    soc_mem_field32_set(unit, MMU_MTRO_L1_MEMm, &l1mtro_tmp, MIN_METER_GRANf, 3);
    soc_mem_field32_set(unit, MMU_MTRO_L1_MEMm, &l1mtro_tmp, MIN_THD_SELf, 64);
    soc_mem_field32_set(unit, MMU_MTRO_L1_MEMm, &l1mtro_tmp, MIN_REFRESHf, 15625);
    SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L1_MEMm(unit, MEM_BLOCK_ANY,
                                                    from_sched_idx, &l1mtro_tmp));
    
    soc_timeout_init(&timeout, timeout_val, 0);

    do {
        empty_count = 0;
        if (soc_timeout_check(&timeout)) {
            LOG_ERROR(BSL_LS_BCM_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: timeout on L2 child c_on_active_list not zero (read count=%d)\n"),
                       rcnt));
            rv = BCM_E_BUSY;
            goto failed;
        }
        for (empty_count = 0, ii = 0; ii < numl2; ii++) {
            rcnt += 1;
            SOC_IF_ERROR_RETURN(READ_LLS_L2_CHILD_STATE1m(unit, 
                                    MEM_BLOCK_ANY, l2_index[ii], &l2cse));
            if (soc_mem_field32_get(unit, LLS_L2_CHILD_STATE1m, &l2cse, 
                                   C_ON_ACTIVE_LISTf)==0) {
                empty_count += 1;
            } else {
                break;
            }
        }
    } while (empty_count != numl2);

    /* change the parent */
    for (ii = 0; ii < numl2; ii++) {
        SOC_IF_ERROR_RETURN(
          WRITE_LLS_L2_PARENTm(unit, MEM_BLOCK_ANY, l2_index[ii], &l2p[ii]));
    }

    for (empty_count = 0, ii = 0; ii < numl2; ii++) {
        SOC_IF_ERROR_RETURN(READ_LLS_L2_CHILD_STATE1m(unit, 
                                MEM_BLOCK_ANY, l2_index[ii], &l2cse));
        actives[ii] = 0;
        if (soc_mem_field32_get(unit, LLS_L2_CHILD_STATE1m, &l2cse, 
                               C_ON_ACTIVE_LISTf)==0) {
            empty_count += 1;
        } else {
            actives[ii] = 1;
        }
    }

    sync = 0;
    if (empty_count != numl2) {
        for (ii = 0; ii < numl2; ii++) {
            if (actives[ii] == 0) {
                continue;
            }
            SOC_IF_ERROR_RETURN(READ_LLS_L1_PARENT_STATEm(unit, 
                                MEM_BLOCK_ANY, from_sched_idx, &l1pe_old));
            if ((soc_mem_field32_get(unit, LLS_L1_PARENT_STATEm, &l1pe_old, 
                                   P_WRR_0_NOT_EMPTYf)) ||
                (soc_mem_field32_get(unit, LLS_L1_PARENT_STATEm, &l1pe_old, 
                                   P_WRR_1_NOT_EMPTYf))) {
                sync = 1;
            }
        }
    }

    if (sync) {
        empty_count = 0;
        soc_timeout_init(&timeout, timeout_val, 0);
        while (empty_count != numl2) {
            if (soc_timeout_check(&timeout)) {
                rv = BCM_E_BUSY;
                break;
            }
            for (empty_count = 0, ii = 0; ii < numl2; ii++) {
                SOC_IF_ERROR_RETURN(READ_LLS_L2_CHILD_STATE1m(unit, 
                                        MEM_BLOCK_ANY, l2_index[ii], &l2cse));
                if (soc_mem_field32_get(unit, LLS_L2_CHILD_STATE1m, &l2cse, 
                                       C_ON_ACTIVE_LISTf)==0) {
                    empty_count += 1;
                } else {
                    break;
                }
            }
        }
    }

failed:
    for (ii = 0; ii < numl2; ii++) {
        SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L2_MEMm(unit, MEM_BLOCK_ANY,
                                                  l2_index[ii], &mtro[ii]));
    }

    if (!rv) {
        for (ii = 0; ii < numl2; ii++) {
            rval = 0;
            soc_reg_field_set(unit, LLS_DEBUG_INJECT_ACTIVATIONr, &rval, INJECTf, 1);
            soc_reg_field_set(unit, LLS_DEBUG_INJECT_ACTIVATIONr, &rval, ENTITY_TYPEf, 1);
            soc_reg_field_set(unit, LLS_DEBUG_INJECT_ACTIVATIONr, &rval, ENTITY_LEVELf, 3);
            soc_reg_field_set(unit, LLS_DEBUG_INJECT_ACTIVATIONr, &rval, 
                              ENTITY_IDENTIFIERf, l2_index[ii]);
            SOC_IF_ERROR_RETURN(WRITE_LLS_DEBUG_INJECT_ACTIVATIONr(unit, rval));
        }
    }

    SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L1_MEMm(unit, MEM_BLOCK_ANY, 
                                              from_sched_idx, &l1mtro));
    return rv;
}

typedef struct _bcm_tr3_lls_info_s {
    int child_lvl;
    int child_hw_index;
    int node_count[SOC_TR3_NODE_LVL_MAX];
    int count[SOC_TR3_NODE_LVL_MAX];
    int offset[SOC_TR3_NODE_LVL_MAX];
   
    int kbit_max;
    int kbit_min;

    uint32  *mtro_entries;
} _bcm_tr3_lls_info_t;

STATIC int _bcm_tr3_lls_node_count(int unit, int port, int level, 
                                   int hw_index, void *cookie)
{
    _bcm_tr3_lls_info_t *i_lls_tree = (_bcm_tr3_lls_info_t *) cookie;
    i_lls_tree->node_count[level] += 1;
    return 0;
}

STATIC int _bcm_tr3_lls_shapers_remove(int unit, int port, int level, 
                                       int hw_index, void *cookie)
{
    _bcm_tr3_lls_info_t *i_lls_tree = (_bcm_tr3_lls_info_t *) cookie;
    soc_mem_t config_mem;
    uint32  *p_entry;
    uint32 entry[SOC_MAX_MEM_WORDS];
    int index, kbit_max;
    uint32 meter_flags;
    uint32 bucketsize_max, granularity_max, refresh_rate_max,
           refresh_bitsize = 0, bucket_bitsize = 0;

    if (level == SOC_TR3_NODE_LVL_ROOT) {
        return SOC_E_NONE;
    } else if (level == SOC_TR3_NODE_LVL_L0) {
        config_mem = MMU_MTRO_L0_MEMm;
    } else if (level == SOC_TR3_NODE_LVL_L1) {
        config_mem = MMU_MTRO_L1_MEMm;
    } else if (level == SOC_TR3_NODE_LVL_L2) {
        config_mem = MMU_MTRO_L2_MEMm;
    } else {
        return SOC_E_PARAM;
    }

    index = i_lls_tree->offset[level] + i_lls_tree->count[level];
    p_entry = &i_lls_tree->mtro_entries[index * SOC_MAX_MEM_WORDS];

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, config_mem, MEM_BLOCK_ALL, hw_index, 
                                     p_entry));
    
    sal_memset(entry, 0, sizeof(uint32) * SOC_MAX_MEM_WORDS);

    if ((i_lls_tree->child_lvl == level) && 
            (hw_index != i_lls_tree->child_hw_index)) {
        
        meter_flags = 0;

        kbit_max = i_lls_tree->kbit_max;
        refresh_bitsize = soc_mem_field_length(unit, config_mem, MAX_REFRESHf);
        bucket_bitsize = soc_mem_field_length(unit, config_mem, MAX_THD_SELf);

        BCM_IF_ERROR_RETURN
            (_bcm_td_rate_to_bucket_encoding(unit, kbit_max, kbit_max,
                                             meter_flags, refresh_bitsize,
                                             bucket_bitsize, &refresh_rate_max,
                                             &bucketsize_max, &granularity_max));

        sal_memset(entry, 0, sizeof(uint32)*SOC_MAX_MEM_WORDS);
        soc_mem_field32_set(unit, config_mem, &entry, MAX_METER_GRANf, granularity_max);
        soc_mem_field32_set(unit, config_mem, &entry, MAX_REFRESHf, refresh_rate_max);
        soc_mem_field32_set(unit, config_mem, &entry, MAX_THD_SELf, bucketsize_max);

        BCM_IF_ERROR_RETURN
            (soc_mem_write(unit, config_mem, MEM_BLOCK_ALL, hw_index, &entry));
    
    } else {
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, config_mem, MEM_BLOCK_ALL, 
                                                    hw_index, &entry));
    }

    i_lls_tree->count[level] += 1;

    return 0;
}

STATIC int _bcm_tr3_lls_shapers_restore(int unit, int port, int level, 
                                        int hw_index, void *cookie)
{
    _bcm_tr3_lls_info_t *i_lls_tree = (_bcm_tr3_lls_info_t *) cookie;
    soc_mem_t config_mem;
    uint32  *p_entry;
    int index;

    if (level == SOC_TR3_NODE_LVL_ROOT) {
        return SOC_E_NONE;
    } else if (level == SOC_TR3_NODE_LVL_L0) {
        config_mem = MMU_MTRO_L0_MEMm;
    } else if (level == SOC_TR3_NODE_LVL_L1) {
        config_mem = MMU_MTRO_L1_MEMm;
    } else if (level == SOC_TR3_NODE_LVL_L2) {
        config_mem = MMU_MTRO_L2_MEMm;
    } else {
        return SOC_E_PARAM;
    }

    index = i_lls_tree->offset[level] + i_lls_tree->count[level];
    p_entry = &i_lls_tree->mtro_entries[index * SOC_MAX_MEM_WORDS];

    SOC_IF_ERROR_RETURN(soc_mem_write(unit, config_mem, MEM_BLOCK_ALL, 
                                                    hw_index, p_entry));

    i_lls_tree->count[level] += 1;

    return 0;
}

STATIC int
_bcm_tr3_adjust_lls_bw(int unit, bcm_port_t port, _bcm_tr3_cosq_node_t *node, 
                       int child_lvl, int child_hw_index, int start, 
                       _bcm_tr3_lls_info_t *i_lls_tree)
{
    int rv = BCM_E_NONE, count, mem_size, ii, speed, jj;
    soc_info_t *si;
    
    if (!soc_feature(unit, soc_feature_tr3_sp_vector_mask)) {
        return BCM_E_NONE;
    }
    
    if (start) {
        sal_memset(i_lls_tree, 0, sizeof(_bcm_tr3_lls_info_t));

        i_lls_tree->child_lvl = child_lvl;
        i_lls_tree->child_hw_index = child_hw_index;

        if (node) {

            /* get to top node */
            while(node->parent) { node = node->parent; }

            if ((rv = _bcm_tr3_cosq_traverse_lls_tree(unit, port, node, 
                                    _bcm_tr3_lls_node_count, i_lls_tree))) {
                goto error;
            }
        } else {
            if ((rv = soc_tr3_port_lls_traverse(unit, port, 
                                    _bcm_tr3_lls_node_count, i_lls_tree))) {
                goto error;
            }
        }

        for (count = 0, ii = 0; ii < SOC_TR3_NODE_LVL_MAX; ii++) {
            count += i_lls_tree->node_count[ii];
            for (jj = ii - 1; jj >= 0; jj--) {
                i_lls_tree->offset[ii] += i_lls_tree->node_count[jj];
            }
        }
        
        if (count == 0) {
            return BCM_E_INIT;
        }
        
        mem_size = sizeof(uint32)*SOC_MAX_MEM_WORDS*count;
        i_lls_tree->mtro_entries = sal_alloc(mem_size, "lls_war_buf");

        rv = soc_phyctrl_speed_get(unit, port, &speed);
        if (rv == BCM_E_UNAVAIL) {
            si = &SOC_INFO(unit);
            speed = si->port_speed_max[port];
        }

        i_lls_tree->kbit_min = 0;
        i_lls_tree->kbit_max = ((speed * 9)/10)/i_lls_tree->node_count[child_lvl];

        for (count = 0, ii = 0; ii < SOC_TR3_NODE_LVL_MAX; ii++) {
            i_lls_tree->count[ii] = 0;
        }

        sal_memset(i_lls_tree->mtro_entries, 0, mem_size);

        if (node) {

            /* get to top node */
            while(node->parent) { node = node->parent; }

            if ((rv = _bcm_tr3_cosq_traverse_lls_tree(unit, port, node, 
                                    _bcm_tr3_lls_shapers_remove, i_lls_tree))) {
                goto error;
            }
        } else {
            if ((rv = soc_tr3_port_lls_traverse(unit, port, 
                                    _bcm_tr3_lls_shapers_remove, i_lls_tree))) {
                goto error;
            }
        }

    } else {
        for (count = 0, ii = 0; ii < SOC_TR3_NODE_LVL_MAX; ii++) {
            i_lls_tree->count[ii] = 0;
        }

        if (node) {
            /* get to top node */
            while(node->parent) { node = node->parent; }

            if ((rv = _bcm_tr3_cosq_traverse_lls_tree(unit, port, node, 
                                    _bcm_tr3_lls_shapers_restore, i_lls_tree))) {
                goto error;
            }
        } else {
            if ((rv = soc_tr3_port_lls_traverse(unit, port, 
                                    _bcm_tr3_lls_shapers_restore, i_lls_tree))) {
                goto error;
            }
        }

        /* cleanup */
        if (i_lls_tree->mtro_entries) {
            sal_free(i_lls_tree->mtro_entries);
            i_lls_tree->mtro_entries = NULL;
        }
    }

    return BCM_E_NONE;

error:
    /* cleanup */
    if (i_lls_tree->mtro_entries) {
        sal_free(i_lls_tree->mtro_entries);
        i_lls_tree->mtro_entries = NULL;
    }

    return rv;
}

STATIC int
_bcm_tr3_cosq_sched_parent_child_set(int unit, int port, int level,
                                     int sched_index, int child_index,
                                     soc_tr3_sched_mode_e sched_mode, 
                                     int weight)
{
    int wt, rv = BCM_E_NONE, num_spri, first_sp_child, first_sp_mc_child;
    uint32 ucmap = 0, spmap = 0;
    soc_tr3_sched_mode_e cur_sched_mode;
    _bcm_tr3_lls_info_t lls_tree_info;

    BCM_IF_ERROR_RETURN(soc_tr3_cosq_get_sched_config(unit, 
                                                      port, 
                                                      level,
                                                      sched_index, 
                                                      child_index,
                                                      &num_spri,
                                                      &first_sp_child,
                                                      &first_sp_mc_child,
                                                      &ucmap,
                                                      &spmap,
                                                      &cur_sched_mode,
                                                      &wt));

    if (_bcm_tr3_compute_lls_config(unit, port,
                                   level, 
                                   &first_sp_child, 
                                   &first_sp_mc_child, 
                                   &num_spri, 
                                   &ucmap, 
                                   &spmap,
                                   child_index, 
                                   cur_sched_mode, 
                                   sched_mode)) {
        return BCM_E_UNAVAIL;
    }

    if ((rv = _bcm_tr3_adjust_lls_bw(unit, port, NULL, 
                    level + 1, child_index, 1, &lls_tree_info))) {
        goto error;
    }

    if ((rv = soc_tr3_cosq_set_sched_config(unit, port, level,
                       sched_index, child_index, num_spri, first_sp_child,
                       first_sp_mc_child, ucmap, spmap, sched_mode, weight))) {
        goto error;
    }

error:
    BCM_IF_ERROR_RETURN(_bcm_tr3_adjust_lls_bw(unit, port, NULL, 
                    level + 1, child_index, 0, &lls_tree_info));

    return rv;
}

/*
 * Function:
 *     _bcm_tr3_cosq_sched_set
 * Purpose:
 *     Set scheduling mode
 * Parameters:
 *     unit                - (IN) unit number
 *     port                - (IN) port number or GPORT identifier
 *     cosq                - (IN) COS queue number
 *     mode                - (IN) scheduling mode (BCM_COSQ_XXX)
 *     num_weights         - (IN) number of entries in weights array
 *     weights             - (IN) weights array
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_cosq_sched_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                       int mode, int weight)
{
    _bcm_tr3_cosq_node_t *node = NULL, *child_node;
    bcm_port_t local_port;
    int sch_index, lvl = SOC_TR3_NODE_LVL_L0, child_index;
    /* , root_index; */
    int numq, lwts = 1, to_sched_index, cpu_l0_index, cpu_l1_index;
    soc_tr3_sched_mode_e sched_mode;
    int l2_index[2], l1_base, l0_base, ii, rv, lvl_off;
    uint32 entry[SOC_MAX_MEM_WORDS];
    mmu_mtro_l1_mem_entry_t l1mtro[16], l1mtro_tmp;
    mmu_mtro_l1_mem_entry_t l0mtro;
    uint64 egrm, tmp64;
    
    if (cosq < 0) {
        if (cosq == -1) {
            /* caller needs to resolve the wild card value (-1) */
            return BCM_E_INTERNAL;
        } else { /* reject all other negative value */
            return BCM_E_PARAM;
        }
    }

    if ((weight < 0) || (weight > 127)) {
        return BCM_E_PARAM;
    }

    switch(mode) {
        case BCM_COSQ_STRICT:
            sched_mode = SOC_TR3_SCHED_MODE_STRICT;
        break;
        case BCM_COSQ_ROUND_ROBIN:
            sched_mode = SOC_TR3_SCHED_MODE_WRR;
            lwts = 1;
        break;
        case BCM_COSQ_WEIGHTED_ROUND_ROBIN:
            sched_mode = SOC_TR3_SCHED_MODE_WRR;
            lwts = weight;
        break;
        case BCM_COSQ_DEFICIT_ROUND_ROBIN:
            sched_mode = SOC_TR3_SCHED_MODE_WDRR;
            lwts = weight;
        break;
        default:
            return BCM_E_PARAM;
    }

    if (lwts == 0) {
        /* On weight 0 it should be considered as SP */ 
        sched_mode = SOC_TR3_SCHED_MODE_STRICT;
    }

    BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    if (_bcm_tr3_cosq_port_has_ets(unit, local_port)) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_node_get(unit, port, 
                                    0, NULL, &local_port, NULL, &node));
        lvl = node->level;
        numq = node->numq;
        sch_index = node->hw_index;

        if ((node->numq > 0) && (cosq >= node->numq)) {
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN(
            _bcm_tr3_cosq_child_node_at_input(node, cosq, &child_node));
        child_index = child_node->hw_index;

        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_sched_parent_child_set(unit, 
                            local_port, node->level, node->hw_index, 
                            child_node->hw_index, sched_mode, lwts));
    } else {
        numq = IS_CPU_PORT(unit, local_port) ? 
                        SOC_INFO(unit).num_cpu_cosq : _TR3_NUM_COS(unit);

        if (cosq >= numq) {
            return BCM_E_PARAM;
        }

        if (IS_TR3_HSP_PORT(unit, local_port)) {
            BCM_IF_ERROR_RETURN(
                    soc_tr3_hsp_sched_weight_set(unit, local_port, cosq, lwts));
            BCM_IF_ERROR_RETURN(
                    soc_tr3_hsp_set_sched_config(unit, local_port, sched_mode));
            return BCM_E_NONE;
        }

        if (!IS_CPU_PORT(unit, local_port) &&
            (!soc_feature(unit, soc_feature_tr3_sp_vector_mask) &&
             soc_property_port_get(unit, local_port, spn_PORT_SCHED_DYNAMIC, 0))) {

            BCM_IF_ERROR_RETURN(
                _bcm_tr3_cosq_index_resolve(unit, local_port, cosq,
                   _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL, &l2_index[0], NULL));

            BCM_IF_ERROR_RETURN(
                _bcm_tr3_cosq_index_resolve(unit, local_port, cosq,
                   _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE, NULL, &l2_index[1], NULL));

            BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, local_port, 
                                                           SOC_TR3_NODE_LVL_L1, 
                                                           cosq, &to_sched_index));

            if (sched_mode == SOC_TR3_SCHED_MODE_STRICT) {
                to_sched_index += soc_port_hg_capable(unit, local_port) ? 9 : 8;
            }

            /* current parent */
            SOC_IF_ERROR_RETURN(READ_LLS_L2_PARENTm(unit, MEM_BLOCK_ANY,
                                                        l2_index[0], entry));
            sch_index = soc_mem_field32_get(unit, LLS_L2_PARENTm, entry, C_PARENTf);

            if (sch_index != to_sched_index) { 
                SOC_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, local_port, &egrm));
                COMPILER_64_ZERO(tmp64);
                SOC_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, local_port, tmp64));
                sal_memset(&l1mtro_tmp, 0, sizeof(l1mtro_tmp)); 
                BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, local_port, 
                                                           SOC_TR3_NODE_LVL_L0, 
                                                           0, &l0_base));
                SOC_IF_ERROR_RETURN(READ_MMU_MTRO_L0_MEMm(unit, MEM_BLOCK_ANY, 
                                                          l0_base, &l0mtro));
                
                SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L0_MEMm(unit, MEM_BLOCK_ANY, 
                                                           l0_base, &l1mtro_tmp));

                BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, local_port, 
                                                           SOC_TR3_NODE_LVL_L1, 
                                                           0, &l1_base));
                for (ii = 0; ii < 16; ii++) {
                    SOC_IF_ERROR_RETURN(READ_MMU_MTRO_L1_MEMm(unit, MEM_BLOCK_ANY, 
                                                              l1_base + ii, &l1mtro[ii]));
                    
                    SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L1_MEMm(unit, MEM_BLOCK_ANY, l1_base + ii,
                                                                &l1mtro_tmp));
                }
                
                rv = _bcm_tr3_cosq_dyn_move_queue(unit, local_port, cosq, 
                                                  l2_index, 2, sch_index, to_sched_index);

                /* restore l1 mtros */
                for (ii = 0; ii < 16; ii++) {
                    SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L1_MEMm(unit, MEM_BLOCK_ANY, l1_base + ii,
                                                                &l1mtro[ii]));
                }
                    
                SOC_IF_ERROR_RETURN(WRITE_MMU_MTRO_L0_MEMm(unit, MEM_BLOCK_ANY, 
                                                          l0_base, &l0mtro));
                
                SOC_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, local_port, egrm));

                if (rv) {
                    return rv;
                }
            }

            if (sched_mode != SOC_TR3_SCHED_MODE_STRICT) {
                SOC_IF_ERROR_RETURN(soc_tr3_cosq_set_sched_mode(unit, 
                        local_port, SOC_TR3_NODE_LVL_L1, to_sched_index, 
                        sched_mode, lwts));
            }

            return SOC_E_NONE;
        } else {
            if (IS_CPU_PORT(unit, local_port)) {
                /* set the L0.0 index as parent, L0.(cosq/8) as child in 
                 * schduling mode */
                BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, local_port, 
                                     SOC_TR3_NODE_LVL_L0, 0, &cpu_l0_index));
                BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, local_port, 
                                     SOC_TR3_NODE_LVL_L1, cosq/8, &cpu_l1_index));
                BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_sched_parent_child_set(unit, 
                                  local_port, SOC_TR3_NODE_LVL_L0, cpu_l0_index, 
                                  cpu_l1_index, sched_mode, 1));
            }
            
            lvl_off = cosq/8;
            lvl = IS_CPU_PORT(unit, local_port) ? 
                                SOC_TR3_NODE_LVL_L1 : SOC_TR3_NODE_LVL_L0;

            BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, local_port, 
                                                   lvl, lvl_off, &sch_index));

            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_index_resolve(unit, local_port, cosq,
                       _BCM_TR3_COSQ_INDEX_STYLE_SCHEDULER, NULL, &child_index, NULL));
            
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_sched_parent_child_set(unit, 
                  local_port, lvl, sch_index, child_index, sched_mode, lwts));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_sched_get
 * Purpose:
 *     Get scheduling mode setting
 * Parameters:
 *     unit                - (IN) unit number
 *     port                - (IN) port number or GPORT identifier
 *     cosq                - (IN) COS queue number
 *     mode                - (OUT) scheduling mode (BCM_COSQ_XXX)
 *     num_weights         - (IN) number of entries in weights array
 *     weights             - (OUT) weights array
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_cosq_sched_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                       int *mode, int *weight)
{
    _bcm_tr3_cosq_node_t *node, *child_node;
    bcm_port_t local_port;
    int sch_index, lvl = SOC_TR3_NODE_LVL_L1, numq;
    int child_index;
    soc_tr3_sched_mode_e    sched_mode;
    _bcm_tr3_mmu_info_t *mmu_info;
    lls_l2_parent_entry_t l2pe;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    if (cosq < 0) {
        if (cosq == -1) {
            /* caller needs to resolve the wild card value (-1) */
            return BCM_E_INTERNAL;
        } else { /* reject all other negative value */
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    if (_bcm_tr3_cosq_port_has_ets(unit, local_port)) {
        BCM_IF_ERROR_RETURN
          (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, &local_port, NULL, &node));
        sch_index = node->hw_index;
        numq = node->numq;

        BCM_IF_ERROR_RETURN(
            _bcm_tr3_cosq_child_node_at_input(node, cosq, &child_node));
        lvl = child_node->level;
        sch_index = child_node->hw_index;
    } else {
        numq = (IS_CPU_PORT(unit, local_port)) ? SOC_INFO(unit).num_cpu_cosq : _TR3_NUM_COS(unit);

        if (cosq >= numq) {
            return BCM_E_PARAM;
        }

        if (IS_TR3_HSP_PORT(unit, local_port)) {
            BCM_IF_ERROR_RETURN(
                        soc_tr3_hsp_sched_weight_get(unit, local_port, cosq, weight));
            BCM_IF_ERROR_RETURN(
                        soc_tr3_hsp_get_sched_config(unit, local_port, &sched_mode));
            return BCM_E_NONE;
        }

        if (!IS_CPU_PORT(unit, local_port) &&
            (!soc_feature(unit, soc_feature_tr3_sp_vector_mask) &&
             soc_property_port_get(unit, local_port, spn_PORT_SCHED_DYNAMIC, 0))) {

            BCM_IF_ERROR_RETURN(
                _bcm_tr3_cosq_index_resolve(unit, local_port, cosq,
                   _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL, &child_index, NULL));

            SOC_IF_ERROR_RETURN(READ_LLS_L2_PARENTm(unit, MEM_BLOCK_ANY,
                                                        child_index, &l2pe));
            sch_index = soc_mem_field32_get(unit, LLS_L2_PARENTm, &l2pe, C_PARENTf);
            lvl = SOC_TR3_NODE_LVL_L1;
        } else {
            lvl = (IS_CPU_PORT(unit, local_port)) ? 
                                SOC_TR3_NODE_LVL_L2 : SOC_TR3_NODE_LVL_L1;
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_index_resolve(unit, local_port, cosq,
                   _BCM_TR3_COSQ_INDEX_STYLE_SCHEDULER, NULL, &sch_index, NULL));
        }
    }

    BCM_IF_ERROR_RETURN(
            soc_tr3_cosq_get_sched_mode(unit, local_port, lvl, sch_index,
                            &sched_mode, weight));

    switch(sched_mode) {
        case SOC_TR3_SCHED_MODE_STRICT:
            *mode = BCM_COSQ_STRICT;
        break;
        case SOC_TR3_SCHED_MODE_WRR:
            *mode = BCM_COSQ_WEIGHTED_ROUND_ROBIN;
        break;
        case SOC_TR3_SCHED_MODE_WDRR:
            *mode = BCM_COSQ_DEFICIT_ROUND_ROBIN;
        break;
        default:
            return BCM_E_INTERNAL;
        break;
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_detach
 * Purpose:
 *     Discard all COS schedule/mapping state.
 * Parameters:
 *     unit - unit number
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_detach(int unit, int software_state_only)
{
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_config_set
 * Purpose:
 *     Set the number of COS queues
 * Parameters:
 *     unit - unit number
 *     numq - number of COS queues (1-8).
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_config_set(int unit, int numq)
{
    port_cos_map_entry_t cos_map_entries[16];
    void *entries[1], *hg_entries[1];
    uint32 index, hg_index;
    int count, hg_count;
    bcm_port_t port;
    int cos, prio;
    uint32 i;
#ifdef BCM_COSQ_HIGIG_MAP_DISABLE
    port_cos_map_entry_t hg_cos_map_entries[16];
#endif
    voq_cos_map_entry_t voq_cos_map;
    int ref_count;

    if (numq < 1 || numq > 8) {
        return BCM_E_PARAM;
    }

    /* clear out old profiles. */
    index = 0;
    while (index < soc_mem_index_count(unit, PORT_COS_MAPm)) {
        BCM_IF_ERROR_RETURN(soc_profile_mem_ref_count_get(unit,
                                      _bcm_tr3_cos_map_profile[unit],
                                      index, &ref_count));
        if (ref_count > 0) {
            while (ref_count) {
                BCM_IF_ERROR_RETURN(soc_profile_mem_delete(unit, 
                                        _bcm_tr3_cos_map_profile[unit], index));
                ref_count -= 1;
            }
        }
        index += 16;
    }

    /* Distribute first 8 internal priority levels into the specified number
     * of cosq, map remaining internal priority levels to highest priority
     * cosq */
    sal_memset(cos_map_entries, 0, sizeof(cos_map_entries));
    entries[0] = &cos_map_entries;
    hg_entries[0] = &cos_map_entries;
    prio = 0;
    for (cos = 0; cos < numq; cos++) {
        for (i = 8 / numq + (cos < 8 % numq ? 1 : 0); i > 0; i--) {
            soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[prio],
                                UC_COS1f, cos);
            soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[prio],
                                MC_COS1f, cos);
            soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[prio],
                                HG_COSf, cos);
            prio++;
        }
    }
    for (prio = 8; prio < 16; prio++) {
        soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[prio], UC_COS1f,
                            numq - 1);
        soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[prio], MC_COS1f,
                            numq - 1);
        soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[prio], HG_COSf,
                            numq - 1);
    }

    /* Map internal priority levels to CPU CoS queues */
    BCM_IF_ERROR_RETURN(_bcm_esw_cpu_cosq_mapping_default_set(unit, numq));

#ifdef BCM_COSQ_HIGIG_MAP_DISABLE
    /* Use identical mapping for Higig port */
    sal_memset(hg_cos_map_entries, 0, sizeof(hg_cos_map_entries));
    hg_entries[0] = &hg_cos_map_entries;
    prio = 0;
    for (prio = 0; prio < 8; prio++) {
        soc_mem_field32_set(unit, PORT_COS_MAPm, &hg_cos_map_entries[prio],
                            COSf, prio);
    }
    for (prio = 8; prio < 16; prio++) {
        soc_mem_field32_set(unit, PORT_COS_MAPm, &hg_cos_map_entries[prio],
                            COSf, 7);
    }
#endif /* BCM_COSQ_HIGIG_MAP_DISABLE */

    BCM_IF_ERROR_RETURN
        (soc_profile_mem_add(unit, _bcm_tr3_cos_map_profile[unit], entries, 16,
                             &index));
    /* for higig ports the mapping is slight different */
    soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[14],
                               HG_COSf, 8);
    soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[15],
                               HG_COSf, 9);

    BCM_IF_ERROR_RETURN
        (soc_profile_mem_add(unit, _bcm_tr3_cos_map_profile[unit], hg_entries,
                             16, &hg_index));
    count = 0;
    hg_count = 0;
    PBMP_ALL_ITER(unit, port) {
        if (IS_LB_PORT(unit, port)) {
            continue;
        }

        if (IS_HG_PORT(unit, port)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, COS_MAP_SELm, port, SELECTf,
                                        hg_index / 16));
            hg_count++;
        } else {
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, COS_MAP_SELm, port, SELECTf,
                                        index / 16));
            count++;
        }
    }
    for (i = 1; i < count; i++) {
        soc_profile_mem_reference(unit, _bcm_tr3_cos_map_profile[unit], index,
                                  0);
    }
    for (i = 1; i < hg_count; i++) {
        soc_profile_mem_reference(unit, _bcm_tr3_cos_map_profile[unit],
                                  hg_index, 0);
    }

    sal_memset(&voq_cos_map, 0, sizeof(voq_cos_map));
    for (cos = 0; cos < numq; cos++) {
        for (i = 8 / numq + (i < 8 % numq ? 1 : 0); i > 0; i--) {
            soc_mem_field32_set(unit, VOQ_COS_MAPm, &voq_cos_map, VOQ_COS_OFFSETf, cos);
            BCM_IF_ERROR_RETURN(
                soc_mem_write(unit, VOQ_COS_MAPm, MEM_BLOCK_ANY, cos,
                          &voq_cos_map));
            prio++;
        }
    }
    for (prio = 8; prio < 16; prio++) {
        soc_mem_field32_set(unit, VOQ_COS_MAPm, &voq_cos_map, VOQ_COS_OFFSETf, numq - 1);
        BCM_IF_ERROR_RETURN(
                soc_mem_write(unit, VOQ_COS_MAPm, MEM_BLOCK_ANY, prio,
                          &voq_cos_map));
    }

    _TR3_NUM_COS(unit) = numq;

    return BCM_E_NONE;
}

STATIC int 
_bcm_tr3_cosq_egress_sp_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq, 
                        int *p_pool_start, int *p_pool_end)
{
    int local_port, uc, hw_index;
    _bcm_tr3_cosq_node_t *node;
    uint32 entry[SOC_MAX_MEM_WORDS], pool, rval;
    
    BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, gport, &local_port));

    if (!SOC_PORT_VALID(unit, local_port)) {
        return BCM_E_PORT;
    }

    if (cosq == BCM_COS_INVALID) {
        *p_pool_start = 0;
        *p_pool_end = 3;
        return BCM_E_NONE;
    }

    node = NULL;
    if (BCM_GPORT_IS_SCHEDULER(gport)) {
        BCM_IF_ERROR_RETURN(
            _bcm_tr3_cosq_node_get(unit, gport, ((cosq < 0) ? 0 : cosq), 
                                NULL, &local_port, NULL, &node));
        BCM_IF_ERROR_RETURN(
            _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
        cosq = 0;
    } else if ((BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) ||
                (BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport))) {
        BCM_IF_ERROR_RETURN(
            _bcm_tr3_cosq_node_get(unit, gport, 0, 
                                NULL, &local_port, NULL, &node));
    }

    if (node) {
        if (node->hw_index < 0) {
            return BCM_E_PARAM;
        }

        hw_index = node->hw_index;
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(node->gport)) {
            uc = 1;
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(node->gport)) {
            uc = 0;
        } else {
            return BCM_E_PARAM;
        }
    } else {
        uc = 1;
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_index_resolve(unit, local_port, cosq, 
            _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE, NULL, &hw_index, NULL));

    }

    if (uc) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_THDO_Q_TO_QGRP_MAPm,
                                     MEM_BLOCK_ANY, hw_index, &entry));
        pool = soc_mem_field32_get(unit, MMU_THDO_Q_TO_QGRP_MAPm, 
                                   entry, Q_SPIDf);
    } else {
        BCM_IF_ERROR_RETURN(soc_reg32_get(unit, OP_QUEUE_CONFIG1_CELLr,
                                          local_port, hw_index, &rval));
        pool = soc_reg_field_get(unit, OP_QUEUE_CONFIG1_CELLr, 
                                    rval, Q_SPIDf);
    }
    
    *p_pool_start = *p_pool_end = pool;
    return BCM_E_NONE;
}

STATIC int 
_bcm_tr3_cosq_ingress_sp_get(int unit, bcm_gport_t gport, bcm_cos_queue_t pri_grp, 
                         int *p_pool_start, int *p_pool_end)
{
    int local_port;
    uint32 rval, pool;
    static const soc_field_t prigroup_spid_field[] = {
        PG0_SPIDf, PG1_SPIDf, PG2_SPIDf, PG3_SPIDf,
        PG4_SPIDf, PG5_SPIDf, PG6_SPIDf, PG7_SPIDf
    };
    
    if (pri_grp == BCM_COS_INVALID) {
        *p_pool_start = 0;
        *p_pool_end = 3;
        return BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, gport, &local_port));

    if (!SOC_PORT_VALID(unit, local_port)) {
        return BCM_E_PORT;
    }

    if (pri_grp >= 8) {
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(READ_PORT_PG_SPIDr(unit, local_port, &rval));
    pool = soc_reg_field_get(unit, PORT_PG_SPIDr, rval, 
                                prigroup_spid_field[pri_grp]);
    
    *p_pool_start = *p_pool_end = pool;
    return BCM_E_NONE;
}

STATIC _bcm_bst_cb_ret_t
_bcm_tr3_bst_index_resolve(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                    bcm_bst_stat_id_t bid, int *pipe, int *start_hw_index, 
                    int *end_hw_index, int *rcb_data, void **cb_data, int *bcm_rv)
{
    int phy_port, mmu_port, local_port;
    _bcm_tr3_cosq_node_t *node = NULL;
    soc_info_t *si;

    *bcm_rv = BCM_E_NONE;
    *pipe = 0;
    si = &SOC_INFO(unit);

    if (bid == bcmBstStatIdDevice) {
        *start_hw_index = *end_hw_index = 0;
        *bcm_rv = BCM_E_NONE;
        return _BCM_BST_RV_OK;
    }

    /* resolve pool */
    if (bid == bcmBstStatIdEgrPool) {
        if(_bcm_tr3_cosq_egress_sp_get(unit, gport, cosq, start_hw_index, 
                                                    end_hw_index)) {
            goto error;
        }
        return _BCM_BST_RV_OK;
    } else if (bid == bcmBstStatIdIngPool) {
        /* ingress pool */
        if (_bcm_tr3_cosq_ingress_sp_get(unit, gport, cosq, 
                                    start_hw_index, end_hw_index)) {
            goto error;
        }
        return _BCM_BST_RV_OK;
    }

    BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, gport, &local_port));

    phy_port = si->port_l2p_mapping[local_port];
    mmu_port = si->port_p2m_mapping[phy_port];

    if (bid == bcmBstStatIdPortPool) {
        *start_hw_index = (mmu_port * 4) + *start_hw_index;
        *end_hw_index = (mmu_port * 4) + *end_hw_index;
    } else if ((bid == bcmBstStatIdPriGroupShared) || 
               (bid == bcmBstStatIdPriGroupHeadroom)) {
        *start_hw_index = (mmu_port * 8) + *start_hw_index;
        *end_hw_index = (mmu_port * 8) + *end_hw_index;

    } else {
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            if (bid != bcmBstStatIdUcast) {
                goto error;
            }
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, 
                                        &local_port, NULL, &node));
            if (!node) {
                goto error;
            }
            *start_hw_index = *end_hw_index = node->hw_index;
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport)) {
            if (bid != bcmBstStatIdMcast) {
                goto error;
            }
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, 
                                        &local_port, NULL, &node));
            if (!node) {
                goto error;
            }
            if (IS_CPU_PORT(unit, local_port)) {
                *start_hw_index = *end_hw_index = node->hw_index - 1536;
            } else {
                *start_hw_index = *end_hw_index = node->hw_index - 1024;
            }
        } else { 
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            
            if (cosq < 0) {
                if (bid == bcmBstStatIdUcast) {
                    *start_hw_index = si->port_uc_cosq_base[local_port];
                    *end_hw_index = si->port_uc_cosq_base[local_port] + 7;
                } else {
                    *start_hw_index = si->port_cosq_base[local_port];
                    *end_hw_index = si->port_cosq_base[local_port] + 7;
                }
            } else {
                if (bid == bcmBstStatIdUcast) {
                    *start_hw_index = si->port_uc_cosq_base[local_port] + cosq;
                    *end_hw_index = si->port_uc_cosq_base[local_port] + cosq;
                } else {
                    *start_hw_index = si->port_cosq_base[local_port] + cosq;
                    *end_hw_index = si->port_cosq_base[local_port] + cosq;
                }
            }
        }
    }
   
    *bcm_rv = BCM_E_NONE;
    return _BCM_BST_RV_OK;

error:
    *bcm_rv = BCM_E_PARAM;
    return _BCM_BST_RV_ERROR;
}

STATIC int
_bcm_tr3_cosq_bst_map_resource_to_gport_cos(int unit, bcm_bst_stat_id_t bid, int port, 
                         int index, bcm_gport_t *gport, bcm_cos_t *cosq)
{
    soc_info_t *si;
    _bcm_tr3_cosq_node_t *node;
    _bcm_tr3_mmu_info_t *mmu_info;
    int resolved = 0, ii;

    mmu_info = _bcm_tr3_mmu_info[unit];
    si = &SOC_INFO(unit);

    switch(bid) {
        case bcmBstStatIdDevice:
            *gport = -1;
            *cosq = BCM_COS_INVALID;
            break;

        case bcmBstStatIdIngPool:
        case bcmBstStatIdEgrPool:
            *gport = -1;
            *cosq = index;
            break;

        case bcmBstStatIdPortPool:
            *gport = index/4;
            *cosq = index % 4;
            break;

        case bcmBstStatIdPriGroupHeadroom:
        case bcmBstStatIdPriGroupShared:
            *gport = index/8;
            *cosq = index % 8;
            break;

        case bcmBstStatIdUcast:
            for (ii = mmu_info->num_base_queues; 
                 ii < _BCM_TR3_NUM_L2_UC_LEAVES; ii++) {
                node = &mmu_info->queue_node[ii];
                if ((node->in_use == TRUE) && (node->hw_index == index)) {
                    *gport = node->gport;
                    *cosq = 0;
                    resolved = 1;
                    break;
                }
            }

            if (!resolved) {
                for (ii = index/8; ii < 64; ii++) {
                    if ((index - si->port_uc_cosq_base[ii]) < 8) {
                        *gport = ii;
                        *cosq = index - si->port_uc_cosq_base[ii];
                        break;
                    }
                }
            }
            break;
        default:
            *gport = -1;
            *cosq = BCM_COS_INVALID;            
            break;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_reserve_gport_resources(int unit, bcm_port_t port)
{
    int lvl, lvl_offset, hw_index, rv;
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_list_t *list;

    mmu_info = _bcm_tr3_mmu_info[unit];

    for (lvl = SOC_TR3_NODE_LVL_L0; lvl <= SOC_TR3_NODE_LVL_L1; lvl++) {
        lvl_offset = 0;

        list = NULL;
        if (lvl == SOC_TR3_NODE_LVL_L0) {
            list = &mmu_info->l0_sched_list;
        } else if (lvl == SOC_TR3_NODE_LVL_L1) {
            list = &mmu_info->l1_sched_list;
        }

        if (!list) {
            continue;
        }
        while ((rv = soc_tr3_sched_hw_index_get(unit, port, lvl, 
                                   lvl_offset, &hw_index)) == SOC_E_NONE) {
            _bcm_tr3_node_index_set(list, hw_index, 1);
            lvl_offset += 1;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_init
 * Purpose:
 *     Initialize (clear) all COS schedule/mapping state.
 * Parameters:
 *     unit - unit number
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_init(int unit)
{
    int numq, alloc_size, ii;
    soc_info_t *si;
    bcm_port_t port;
    soc_reg_t reg;
    soc_reg_t mem;
    static soc_mem_t wred_mems[6] = {
        MMU_WRED_DROP_CURVE_PROFILE_0m, MMU_WRED_DROP_CURVE_PROFILE_1m,
        MMU_WRED_DROP_CURVE_PROFILE_2m, MMU_WRED_DROP_CURVE_PROFILE_3m,
        MMU_WRED_DROP_CURVE_PROFILE_4m, MMU_WRED_DROP_CURVE_PROFILE_5m
    };
    int entry_words[6];
    mmu_wred_drop_curve_profile_0_entry_t entry_tcp_green;
    mmu_wred_drop_curve_profile_1_entry_t entry_tcp_yellow;
    mmu_wred_drop_curve_profile_2_entry_t entry_tcp_red;
    mmu_wred_drop_curve_profile_3_entry_t entry_nontcp_green;
    mmu_wred_drop_curve_profile_4_entry_t entry_nontcp_yellow;
    mmu_wred_drop_curve_profile_5_entry_t entry_nontcp_red;
    void *entries[6];
    uint64 rval64s[16], *prval64s[1];
    uint32 rval, profile_index;
    _bcm_tr3_mmu_info_t *mmu_info;
    int i, wred_prof_count, index;
    _bcm_bst_device_handlers_t bst_callbks;
    _bcm_tr3_cosq_list_t *list = NULL;
    if (!SOC_WARM_BOOT(unit)) {    /* Cold Boot */
        BCM_IF_ERROR_RETURN (bcm_tr3_cosq_detach(unit, 0));
    }

    si = &SOC_INFO(unit);
    numq = soc_property_get(unit, spn_BCM_NUM_COS, BCM_COS_DEFAULT);
    
    if (numq < 1) {
        numq = 1;
    } else if (numq > 8) {
        numq = 8;
    }
    _TR3_NUM_COS(unit) = numq;

    /* Create profile for PORT_COS_MAP table */
    if (_bcm_tr3_cos_map_profile[unit] == NULL) {
        _bcm_tr3_cos_map_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                                  "COS_MAP Profile Mem");
        if (_bcm_tr3_cos_map_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_cos_map_profile[unit]);
    }
    mem = PORT_COS_MAPm;
    entry_words[0] = sizeof(port_cos_map_entry_t) / sizeof(uint32);
    BCM_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, entry_words, 1,
                                               _bcm_tr3_cos_map_profile[unit]));

    /* Create profile for SERVICE_COS_MAP table */
    if (_bcm_tr3_service_cos_map_profile[unit] == NULL) {
        _bcm_tr3_service_cos_map_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                                  "SERVICE_COS_MAP Profile Mem");
        if (_bcm_tr3_service_cos_map_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_service_cos_map_profile[unit]);
    }
    mem = SERVICE_COS_MAPm;
    entry_words[0] = sizeof(service_cos_map_entry_t) / sizeof(uint32);
    BCM_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, entry_words, 1,
                                               _bcm_tr3_service_cos_map_profile[unit]));

    /* Create profile for SERVICE_PORT_MAP table */
    if (_bcm_tr3_service_port_map_profile[unit] == NULL) {
        _bcm_tr3_service_port_map_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                                  "SERVICE_PORT_MAP Profile Mem");
        if (_bcm_tr3_service_port_map_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_service_port_map_profile[unit]);
    }

    /* Create profile for IFP_COS_MAP table. */
    if (_bcm_tr3_ifp_cos_map_profile[unit] == NULL) {
        _bcm_tr3_ifp_cos_map_profile[unit]
            = sal_alloc(sizeof(soc_profile_mem_t), "IFP_COS_MAP Profile Mem");
        if (_bcm_tr3_ifp_cos_map_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_ifp_cos_map_profile[unit]);
    }
    mem = IFP_COS_MAPm;
    entry_words[0] = sizeof(ifp_cos_map_entry_t) / sizeof(uint32);
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_create(unit, &mem, entry_words, 1,
                                _bcm_tr3_ifp_cos_map_profile[unit]));

    mem = SERVICE_PORT_MAPm;
    entry_words[0] = sizeof(service_port_map_entry_t) / sizeof(uint32);
    BCM_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, entry_words, 1,
                                               _bcm_tr3_service_port_map_profile[unit]));

    alloc_size = sizeof(_bcm_tr3_mmu_info_t) * 1;
    if (_bcm_tr3_mmu_info[unit] == NULL) {
        _bcm_tr3_mmu_info[unit] =
            sal_alloc(alloc_size, "_bcm_tr3_mmu_info");

        if (_bcm_tr3_mmu_info[unit] == NULL) {
            return BCM_E_MEMORY;
        }

        sal_memset(_bcm_tr3_mmu_info[unit], 0, alloc_size);
    }

    mmu_info = _bcm_tr3_mmu_info[unit];

    /* assign queues to ports */
    PBMP_ALL_ITER(unit, port) {
        mmu_info->port_info[port].mc_base = si->port_cosq_base[port];
        mmu_info->port_info[port].mc_limit = si->port_cosq_base[port] + 
                                                si->port_num_cosq[port];

        mmu_info->port_info[port].uc_base = si->port_uc_cosq_base[port];;
        mmu_info->port_info[port].uc_limit = si->port_uc_cosq_base[port] + 
                                                si->port_num_uc_cosq[port];

        if (!SOC_WARM_BOOT(unit)) {
            rval = 0;
            soc_reg_field_set(unit, ING_COS_MODEr, &rval, BASE_QUEUE_NUM_0f,
                              mmu_info->port_info[port].uc_base);
            soc_reg_field_set(unit, ING_COS_MODEr, &rval, BASE_QUEUE_NUM_1f,
                              mmu_info->port_info[port].uc_base);
            BCM_IF_ERROR_RETURN(soc_reg32_set(unit, ING_COS_MODEr, port, 0, rval));
        }
    }

    mmu_info->num_base_queues =  soc_tr3_get_ucq_count(unit);
    mmu_info->ets_mode = 0;
    mmu_info->ext_qlist.count = _BCM_TR3_NUM_L2_UC_LEAVES;
    mmu_info->ext_qlist.bits = _bcm_tr3_cosq_alloc_clear(
                                mmu_info->ext_qlist.bits,
      SHR_BITALLOCSIZE(_BCM_TR3_NUM_L2_UC_LEAVES), "ext_qlist");
    if (mmu_info->ext_qlist.bits == NULL) {
        _bcm_tr3_cosq_free_memory(mmu_info);
        return BCM_E_MEMORY;
    }

    mmu_info->l0_sched_list.count = _BCM_TR3_NUM_L0_SCHEDULER;
    mmu_info->l0_sched_list.bits = _bcm_tr3_cosq_alloc_clear(
                                    mmu_info->l0_sched_list.bits,
      SHR_BITALLOCSIZE(_BCM_TR3_NUM_L0_SCHEDULER), "l0_sched_list");
    if (mmu_info->l0_sched_list.bits == NULL) {
        _bcm_tr3_cosq_free_memory(mmu_info);
        return BCM_E_MEMORY;
    }

    mmu_info->l1_sched_list.count = _BCM_TR3_NUM_L1_SCHEDULER;
    mmu_info->l1_sched_list.bits = _bcm_tr3_cosq_alloc_clear(
                                    mmu_info->l1_sched_list.bits,
        SHR_BITALLOCSIZE(_BCM_TR3_NUM_L1_SCHEDULER), "l1_sched_list");
    if (mmu_info->l1_sched_list.bits == NULL) {
        _bcm_tr3_cosq_free_memory(mmu_info);
        return BCM_E_MEMORY;
    }

    for (i = 0; i < _BCM_TR3_NUM_TOTAL_SCHEDULERS; i++) {
        _BCM_TR3_COSQ_LIST_NODE_INIT(mmu_info->sched_node, i);
    }

    for (i = 0; i < _BCM_TR3_NUM_L2_UC_LEAVES; i++) {
        _BCM_TR3_COSQ_LIST_NODE_INIT(mmu_info->queue_node, i);
    }

    for (i = 0; i < _BCM_TR3_NUM_L2_MC_LEAVES; i++) {
        _BCM_TR3_COSQ_LIST_NODE_INIT(mmu_info->mc_queue_node, i);
    }

    for (i = SOC_TR3_NODE_LVL_L1; i <= SOC_TR3_NODE_LVL_L2; i++) {
        index = _soc_tr3_invalid_parent_index(unit, i);
        if (i == SOC_TR3_NODE_LVL_L1) {
            list = &mmu_info->l0_sched_list;
        } else if (i == SOC_TR3_NODE_LVL_L2) {
            list = &mmu_info->l1_sched_list;
        } else {
            continue;
        }
        if (index >= 0) {
            _bcm_tr3_node_index_set(list, index, 1);
        }
    }

    /* Create profile for MMU_QCN_SITB table */
    if (_bcm_tr3_sample_int_profile[unit] == NULL) {
        _bcm_tr3_sample_int_profile[unit] =
            sal_alloc(sizeof(soc_profile_mem_t), "QCN sample Int Profile Mem");
        if (_bcm_tr3_sample_int_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_sample_int_profile[unit]);
    }
    mem = MMU_QCN_SITBm;
    entry_words[0] = sizeof(mmu_qcn_sitb_entry_t) / sizeof(uint32);
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_create(unit, &mem, entry_words, 1,
                                _bcm_tr3_sample_int_profile[unit]));

    /* Create profile for MMU_QCN_CPQ_SEQ register */
    if (_bcm_tr3_feedback_profile[unit] == NULL) {
        _bcm_tr3_feedback_profile[unit] =
            sal_alloc(sizeof(soc_profile_reg_t), "QCN Feedback Profile Reg");
        if (_bcm_tr3_feedback_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_reg_t_init(_bcm_tr3_feedback_profile[unit]);
    }
    reg = MMU_QCN_CPQ_SEQr;
    BCM_IF_ERROR_RETURN
        (soc_profile_reg_create(unit, &reg, 1,
                                _bcm_tr3_feedback_profile[unit]));

    /* Create profile for PRIO2COS_LLFC register */
    if (_bcm_tr3_llfc_profile[unit] == NULL) {
        _bcm_tr3_llfc_profile[unit] =
            sal_alloc(sizeof(soc_profile_reg_t), "PRIO2COS_LLFC Profile Reg");
        if (_bcm_tr3_llfc_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_reg_t_init(_bcm_tr3_llfc_profile[unit]);
    }
    reg = PRIO2COS_PROFILEr;
    BCM_IF_ERROR_RETURN
        (soc_profile_reg_create(unit, &reg, 1, _bcm_tr3_llfc_profile[unit]));

    /* Create profile for MMU_WRED_DROP_CURVE_PROFILE_x tables */
    if (_bcm_tr3_wred_profile[unit] == NULL) {
        _bcm_tr3_wred_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                               "WRED Profile Mem");
        if (_bcm_tr3_wred_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_wred_profile[unit]);
    }

    entry_words[0] =
        sizeof(mmu_wred_drop_curve_profile_0_entry_t) / sizeof(uint32);
    entry_words[1] =
        sizeof(mmu_wred_drop_curve_profile_1_entry_t) / sizeof(uint32);
    entry_words[2] =
        sizeof(mmu_wred_drop_curve_profile_2_entry_t) / sizeof(uint32);
    entry_words[3] =
        sizeof(mmu_wred_drop_curve_profile_3_entry_t) / sizeof(uint32);
    entry_words[4] =
        sizeof(mmu_wred_drop_curve_profile_4_entry_t) / sizeof(uint32);
    entry_words[5] =
        sizeof(mmu_wred_drop_curve_profile_5_entry_t) / sizeof(uint32);
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_create(unit, wred_mems, entry_words, 6,
                                _bcm_tr3_wred_profile[unit]));

    /* Create profile for VOQ_MOD_MAP table */
    if (_bcm_tr3_voq_port_map_profile[unit] == NULL) {
        _bcm_tr3_voq_port_map_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                                  "VOQ_PORT_MAP Profile Mem");
        if (_bcm_tr3_voq_port_map_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_bcm_tr3_voq_port_map_profile[unit]);
    }
    mem = VOQ_PORT_MAPm;
    entry_words[0] = sizeof(voq_port_map_entry_t) / sizeof(uint32);
    BCM_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, entry_words, 1,
                                           _bcm_tr3_voq_port_map_profile[unit]));
   
    sal_memset(&bst_callbks, 0, sizeof(_bcm_bst_device_handlers_t));
    bst_callbks.resolve_index = &_bcm_tr3_bst_index_resolve;
    bst_callbks.reverse_resolve_index = &_bcm_tr3_cosq_bst_map_resource_to_gport_cos;
    BCM_IF_ERROR_RETURN(_bcm_bst_attach(unit, &bst_callbks));

    /* reserve resources ie schedulers and queues which are allocated by
     * soc layer and no gport support is available for the ports. */
    BCM_PBMP_CLEAR(mmu_info->gport_unavail_pbm);
    PBMP_CE_ITER(unit, port) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_reserve_gport_resources(unit, port));
        BCM_PBMP_PORT_ADD(mmu_info->gport_unavail_pbm, port);
    }
    
    PBMP_ITER(PBMP_AXP_ALL(unit), port) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_reserve_gport_resources(unit, port));
        BCM_PBMP_PORT_ADD(mmu_info->gport_unavail_pbm, port);
    }
    
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        return bcm_tr3_cosq_reinit(unit);
    } else {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_wb_alloc(unit));
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    /* Add default entries for PORT_COS_MAP memory profile */
    BCM_IF_ERROR_RETURN(bcm_tr3_cosq_config_set(unit, numq));

    /* Add default entries for PRIO2COS_PROFILE register profile */
    sal_memset(rval64s, 0, sizeof(rval64s));
    prval64s[0] = rval64s;
    profile_index = 0xffffffff;
    PBMP_PORT_ITER(unit, port) {
        if (profile_index == 0xffffffff) {
            BCM_IF_ERROR_RETURN
                (soc_profile_reg_add(unit, _bcm_tr3_llfc_profile[unit],
                                     prval64s, 16, &profile_index));
        } else {
            BCM_IF_ERROR_RETURN
                (soc_profile_reg_reference(unit, _bcm_tr3_llfc_profile[unit],
                                           profile_index, 0));
        }
    }

    /* Add default entries for MMU_WRED_DROP_CURVE_PROFILE_x memory profile */
    sal_memset(&entry_tcp_green, 0, sizeof(entry_tcp_green));
    sal_memset(&entry_tcp_yellow, 0, sizeof(entry_tcp_yellow));
    sal_memset(&entry_tcp_red, 0, sizeof(entry_tcp_red));
    sal_memset(&entry_nontcp_green, 0, sizeof(entry_nontcp_green));
    sal_memset(&entry_nontcp_yellow, 0, sizeof(entry_nontcp_yellow));
    sal_memset(&entry_nontcp_red, 0, sizeof(entry_nontcp_red));
    entries[0] = &entry_tcp_green;
    entries[1] = &entry_tcp_yellow;
    entries[2] = &entry_tcp_red;
    entries[3] = &entry_nontcp_green;
    entries[4] = &entry_nontcp_yellow;
    entries[5] = &entry_nontcp_red;
    for (ii = 0; ii < 6; ii++) {
        soc_mem_field32_set(unit, wred_mems[ii], entries[ii], MIN_THDf, 0x7fff);
        soc_mem_field32_set(unit, wred_mems[ii], entries[ii], MAX_THDf, 0x7fff);
    }
    profile_index = 0xffffffff;
    wred_prof_count = soc_mem_index_count(unit, MMU_WRED_CONFIGm);
    while (wred_prof_count) {
        if (profile_index == 0xffffffff) {
            BCM_IF_ERROR_RETURN
                (soc_profile_mem_add(unit, _bcm_tr3_wred_profile[unit],
                                     entries, 1, &profile_index));
        } else {
            BCM_IF_ERROR_RETURN
                (soc_profile_mem_reference(unit,
                                           _bcm_tr3_wred_profile[unit],
                                           profile_index, 0));
        }
        wred_prof_count -= 1;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *     bcm_tr3_cosq_config_get
 * Purpose:
 *     Get the number of cos queues
 * Parameters:
 *     unit - unit number
 *     numq - (Output) number of cosq
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_config_get(int unit, int *numq)
{
    if (numq != NULL) {
        *numq = _TR3_NUM_COS(unit);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_mapping_set
 * Purpose:
 *     Set COS queue for the specified priority of an ingress port
 * Parameters:
 *     unit     - (IN) unit number
 *     ing_port - (IN) ingress port
 *     gport    - (IN) queue group GPORT identifier
 *     priority - (IN) priority value to map
 *     flags    - (IN) BCM_COSQ_GPORT_XXX_QUEUE_GROUP
 *     gport    - (IN) queue group GPORT identifier
 *     cosq     - (IN) COS queue number
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_cosq_mapping_set(int unit, bcm_port_t ing_port, bcm_cos_t priority,
                         uint32 flags, bcm_gport_t gport, bcm_cos_queue_t cosq)
{
    bcm_port_t local_port, outport = -1;
    bcm_cos_queue_t hw_cosq;
    soc_field_t field = INVALIDf, field2 = INVALIDf;
    cos_map_sel_entry_t cos_map_sel_entry;
    port_cos_map_entry_t cos_map_entries[16];
    void *entries[1];
    uint32 old_index, new_index;
    voq_cos_map_entry_t voq_cos_map;
    _bcm_tr3_cosq_node_t *node = NULL;
    _bcm_tr3_mmu_info_t *mmu_info;
    int rv;

    if (priority < 0 || priority >= 16) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, ing_port, &local_port));

    if (gport != -1) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, gport, &outport));
    }

    mmu_info = _bcm_tr3_mmu_info[unit];

    switch (flags) {
    case BCM_COSQ_GPORT_UCAST_QUEUE_GROUP:
         if(IS_CPU_PORT(unit, local_port)) {  
            return BCM_E_PARAM; 
         }
        if (gport == -1) {
            hw_cosq = cosq;
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, gport, cosq, _BCM_TR3_COSQ_INDEX_STYLE_COS,
                  NULL, &hw_cosq, NULL));

            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
                BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_node_get(unit, gport, cosq, 
                                            NULL, NULL, NULL, &node));

                if (node->hw_index >= mmu_info->num_base_queues) {
                    return BCM_E_PARAM;
                }
            }
        }

        if ((outport != -1) && (IS_HG_PORT(unit, outport))) {
            field = HG_COSf;
        } else {
            field = UC_COS1f;
        }
        break;
    case BCM_COSQ_GPORT_MCAST_QUEUE_GROUP:
        if (gport == -1) {
            hw_cosq = cosq;
        } else {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, gport, cosq, _BCM_TR3_COSQ_INDEX_STYLE_COS,
                  NULL, &hw_cosq, NULL));
        }
        field = MC_COS1f;
        break;
    case BCM_COSQ_GPORT_UCAST_QUEUE_GROUP | BCM_COSQ_GPORT_MCAST_QUEUE_GROUP:
        if (gport == -1) {
            hw_cosq = cosq;
        } else {
            return BCM_E_PARAM;
        }
        field = UC_COS1f;
        field2 = MC_COS1f;
        break;
    case BCM_COSQ_GPORT_DESTMOD_UCAST_QUEUE_GROUP:
         if(IS_CPU_PORT(unit, local_port)) {  
            return BCM_E_PARAM; 
         }
        if (gport == -1) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_index_resolve
             (unit, gport, cosq, _BCM_TR3_COSQ_INDEX_STYLE_COS,
              NULL, &hw_cosq, NULL));

        BCM_IF_ERROR_RETURN(
            READ_VOQ_COS_MAPm(unit, MEM_BLOCK_ALL, priority, &voq_cos_map));
        if (soc_mem_field32_get(unit, VOQ_COS_MAPm, &voq_cos_map, 
                                                VOQ_COS_OFFSETf) == hw_cosq) {
            return BCM_E_NONE;
        }
        soc_mem_field32_set(unit, VOQ_COS_MAPm, 
                            &voq_cos_map, VOQ_COS_OFFSETf, hw_cosq);
        BCM_IF_ERROR_RETURN(
            soc_mem_write(unit, VOQ_COS_MAPm, MEM_BLOCK_ANY, priority,
                          &voq_cos_map));
        return BCM_E_NONE;
        break;
    default:
        return BCM_E_PARAM;
    }

    entries[0] = &cos_map_entries;

    if (local_port == CMIC_PORT(unit)) {
        local_port = SOC_INFO(unit).cpu_hg_index;
    }
    BCM_IF_ERROR_RETURN
        (READ_COS_MAP_SELm(unit, MEM_BLOCK_ANY, local_port,
                           &cos_map_sel_entry));
    old_index = soc_mem_field32_get(unit, COS_MAP_SELm, &cos_map_sel_entry,
                                    SELECTf);
    old_index *= 16;

    BCM_IF_ERROR_RETURN
        (soc_profile_mem_get(unit, _bcm_tr3_cos_map_profile[unit],
                             old_index, 16, entries));
    soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[priority], field,
                        hw_cosq);
    if (field2 != INVALIDf) {
        soc_mem_field32_set(unit, PORT_COS_MAPm, &cos_map_entries[priority],
                            field2,hw_cosq);
    }

    soc_mem_lock(unit, PORT_COS_MAPm);

    rv = soc_profile_mem_delete(unit, _bcm_tr3_cos_map_profile[unit],
                                old_index);

    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, PORT_COS_MAPm);
        return rv;
    }

    rv = soc_profile_mem_add(unit, _bcm_tr3_cos_map_profile[unit], entries,
                             16, &new_index);

    if (rv == SOC_E_RESOURCE) {
        (void)soc_profile_mem_reference(unit, _bcm_tr3_cos_map_profile[unit],
                                        old_index, 16);
    }

    soc_mem_unlock(unit, PORT_COS_MAPm);

    if (BCM_FAILURE(rv)) {
        return rv;
    }

    soc_mem_field32_set(unit, COS_MAP_SELm, &cos_map_sel_entry, SELECTf,
                        new_index / 16);
    BCM_IF_ERROR_RETURN
        (WRITE_COS_MAP_SELm(unit, MEM_BLOCK_ANY, local_port,
                            &cos_map_sel_entry));

#ifndef BCM_COSQ_HIGIG_MAP_DISABLE
    if (IS_CPU_PORT(unit, local_port)) {
        BCM_IF_ERROR_RETURN
            (soc_mem_field32_modify(unit, COS_MAP_SELm,
                                    SOC_INFO(unit).cpu_hg_index, SELECTf,
                                    new_index / 16));
    }
#endif /* BCM_COSQ_HIGIG_MAP_DISABLE */

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_mapping_set
 * Purpose:
 *     Set which cosq a given priority should fall into
 * Parameters:
 *     unit     - (IN) unit number
 *     gport    - (IN) queue group GPORT identifier
 *     priority - (IN) priority value to map
 *     cosq     - (IN) COS queue to map to
 * Returns:
 *     BCM_E_XXX
 */

int
bcm_tr3_cosq_mapping_set(int unit, bcm_port_t port, bcm_cos_t priority,
                         bcm_cos_queue_t cosq)
{
    bcm_port_t local_port;
    bcm_pbmp_t pbmp;

    BCM_PBMP_CLEAR(pbmp);

    if (port == -1) {
        BCM_PBMP_ASSIGN(pbmp, PBMP_ALL(unit));
    } else {
        if (BCM_GPORT_IS_SET(port)) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port) ||
                BCM_GPORT_IS_MCAST_QUEUE_GROUP(port) ||
                BCM_GPORT_IS_SCHEDULER(port)) {
                return BCM_E_PARAM;
            } else if (BCM_GPORT_IS_LOCAL(port)) {
                local_port = BCM_GPORT_LOCAL_GET(port);
            } else {
                return BCM_E_PARAM;
            }
        } else {
            local_port = port;
        }

        if (!SOC_PORT_VALID(unit, local_port)) {
            return BCM_E_PORT;
        }

        BCM_PBMP_PORT_ADD(pbmp, local_port);
    }

    PBMP_ITER(pbmp, local_port) {
        if (_bcm_tr3_cosq_port_has_ets(unit, local_port))
	    return BCM_E_PARAM;
    }

    if ((cosq < 0) || (cosq >= _TR3_NUM_COS(unit))) {
        return BCM_E_PARAM;
    }

    PBMP_ITER(pbmp, local_port) {
        if (IS_LB_PORT(unit, local_port)) {
            continue;
        }

        /* If no ETS/port, map the int prio symmetrically for ucast and
         * mcast */
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_mapping_set(unit, local_port, 
            priority,
            BCM_COSQ_GPORT_UCAST_QUEUE_GROUP | BCM_COSQ_GPORT_MCAST_QUEUE_GROUP,
            -1, cosq));
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_mapping_get
 * Purpose:
 *     Determine which COS queue a given priority currently maps to.
 * Parameters:
 *     unit     - (IN) unit number
 *     gport    - (IN) queue group GPORT identifier
 *     priority - (IN) priority value to map
 *     cosq     - (OUT) COS queue to map to
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_tr3_cosq_mapping_get(int unit, bcm_port_t ing_port, bcm_cos_t priority,
                          uint32 flags, bcm_gport_t *gport,
                          bcm_cos_queue_t *cosq)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_port_info_t *port_info;
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port;
    bcm_cos_queue_t hw_cosq = BCM_COS_INVALID;
    int index, ii;
    cos_map_sel_entry_t cos_map_sel_entry;
    voq_cos_map_entry_t voq_cos_map;
    void *entry_p;

    if (priority < 0 || priority >= 16) {
        return BCM_E_PARAM;
    }

    if (flags != BCM_COSQ_GPORT_UCAST_QUEUE_GROUP &&
        flags != BCM_COSQ_GPORT_MCAST_QUEUE_GROUP &&
        flags != BCM_COSQ_GPORT_DESTMOD_UCAST_QUEUE_GROUP) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, ing_port, &local_port));

    if (gport) {
        *gport = BCM_GPORT_INVALID;
    }

    *cosq = BCM_COS_INVALID;

    if (local_port == CMIC_PORT(unit)) {
        BCM_IF_ERROR_RETURN
            (READ_COS_MAP_SELm(unit, MEM_BLOCK_ANY, SOC_INFO(unit).cpu_hg_index,
                               &cos_map_sel_entry));
    } else {
        BCM_IF_ERROR_RETURN
            (READ_COS_MAP_SELm(unit, MEM_BLOCK_ANY, local_port,
                               &cos_map_sel_entry));
    }
    index = soc_mem_field32_get(unit, COS_MAP_SELm, &cos_map_sel_entry,
                                SELECTf);
    index *= 16;

    mmu_info = _bcm_tr3_mmu_info[unit];
    port_info = &mmu_info->port_info[local_port];

    switch (flags) {
    case BCM_COSQ_GPORT_UCAST_QUEUE_GROUP:
        entry_p = SOC_PROFILE_MEM_ENTRY(unit, _bcm_tr3_cos_map_profile[unit],
                                    port_cos_map_entry_t *,
                                    index + priority);

        hw_cosq = soc_mem_field32_get(unit, PORT_COS_MAPm, entry_p, UC_COS1f);
        if (gport) {
            for (ii = port_info->uc_base; ii < port_info->uc_limit; ii++) {
                node = &mmu_info->queue_node[ii];
                if (node->numq > 0 && node->hw_cosq == hw_cosq) {
                    *gport = node->gport;
                    *cosq = 0;
                    break;
                }
            }
            if (ii == port_info->uc_limit) {
                *cosq = hw_cosq;
            }
        }
        break;
    case BCM_COSQ_GPORT_MCAST_QUEUE_GROUP:
        entry_p = SOC_PROFILE_MEM_ENTRY(unit, _bcm_tr3_cos_map_profile[unit],
                                    port_cos_map_entry_t *,
                                    index + priority);

        hw_cosq = soc_mem_field32_get(unit, PORT_COS_MAPm, entry_p, MC_COS1f);
        if (gport) {
            for (ii = port_info->mc_base; ii < port_info->mc_limit; ii++) {
                node = &mmu_info->mc_queue_node[ii];
                if (node->numq > 0 && node->hw_cosq == hw_cosq) {
                    *gport = node->gport;
                    *cosq = 0;
                    break;
                }
            }
            if (ii == port_info->mc_limit) {
                *cosq = hw_cosq;
            }
        }
        break;
    case BCM_COSQ_GPORT_DESTMOD_UCAST_QUEUE_GROUP:
        BCM_IF_ERROR_RETURN(
            READ_VOQ_COS_MAPm(unit, MEM_BLOCK_ALL, priority, &voq_cos_map));
        hw_cosq = soc_mem_field32_get(unit, VOQ_COS_MAPm, 
                                        &voq_cos_map, VOQ_COS_OFFSETf);
        if (gport) {
            for (ii = mmu_info->num_base_queues; 
                            ii < _BCM_TR3_NUM_L2_UC_LEAVES; ii++) {
                
                node = &mmu_info->queue_node[ii];
                if ((node->in_use == FALSE) || (node->local_port == local_port)) {
                    continue;
                }
                
                if (node->hw_cosq == hw_cosq) {
                    *gport = node->gport;
                    break;
                }
            }
        }
        break;
    default:
        break;
    }

    if (gport &&
        (*gport == BCM_GPORT_INVALID) && (*cosq == BCM_COS_INVALID)) {
        return BCM_E_NOT_FOUND;
    }

    *cosq = hw_cosq;

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_mapping_get(int unit, bcm_port_t port, bcm_cos_t priority,
                        bcm_cos_queue_t *cosq)
{
    bcm_port_t local_port;
    bcm_pbmp_t pbmp;

    BCM_PBMP_CLEAR(pbmp);

    if (port == -1) {
        BCM_PBMP_ASSIGN(pbmp, PBMP_ALL(unit));
    } else {
        if (BCM_GPORT_IS_SET(port)) {
            if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port) ||
                BCM_GPORT_IS_MCAST_QUEUE_GROUP(port) ||
                BCM_GPORT_IS_SCHEDULER(port)) {
                return BCM_E_PARAM;
            } else if (BCM_GPORT_IS_LOCAL(port)) {
                local_port = BCM_GPORT_LOCAL_GET(port);
            } else {
                return BCM_E_PARAM;
            }
        } else {
            local_port = port;
        }

        if (!SOC_PORT_VALID(unit, local_port)) {
            return BCM_E_PORT;
        }

        BCM_PBMP_PORT_ADD(pbmp, local_port);
    }

    PBMP_ITER(pbmp, local_port) {
        if (IS_LB_PORT(unit, local_port)) {
            continue;
        }

        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_mapping_get(unit, local_port, 
                 priority, BCM_COSQ_GPORT_UCAST_QUEUE_GROUP, NULL, cosq));
        break;
    }    

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_gport_mapping_set(int unit, bcm_port_t ing_port,
                              bcm_cos_t int_pri, uint32 flags,
                              bcm_gport_t gport, bcm_cos_queue_t cosq)
{
    return _bcm_tr3_cosq_mapping_set(unit, ing_port, int_pri, flags, gport,
                                    cosq);
}

int
bcm_tr3_cosq_gport_mapping_get(int unit, bcm_port_t ing_port,
                              bcm_cos_t int_pri, uint32 flags,
                              bcm_gport_t *gport, bcm_cos_queue_t *cosq)
{
    if (gport == NULL || cosq == NULL) {
        return BCM_E_PARAM;
    }

    return _bcm_tr3_cosq_mapping_get(unit, ing_port, int_pri, flags, gport,
                                    cosq);
}

/*
 * Function:
 *     bcm_tr3_cosq_port_sched_set
 * Purpose:
 *     Set up class-of-service policy and corresponding weights and delay
 * Parameters:
 *     unit    - (IN) unit number
 *     pbm     - (IN) port bitmap
 *     mode    - (IN) Scheduling mode (BCM_COSQ_xxx)
 *     weights - (IN) Weights for each COS queue
 *     delay   - This parameter is not used
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_port_sched_set(int unit, bcm_pbmp_t pbm,
                           int mode, const int *weights, int delay)
{
    bcm_port_t port;
    int num_weights, i;

    BCM_PBMP_ITER(pbm, port) {
        num_weights = _TR3_NUM_COS(unit);
        for (i = 0; i < num_weights; i++) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_sched_set(unit, port, i, mode, weights[i]));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_port_sched_get
 * Purpose:
 *     Retrieve class-of-service policy and corresponding weights and delay
 * Parameters:
 *     unit     - (IN) unit number
 *     pbm      - (IN) port bitmap
 *     mode     - (OUT) Scheduling mode (BCM_COSQ_XXX)
 *     weights  - (OUT) Weights for each COS queue
 *     delay    - This parameter is not used
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_port_sched_get(int unit, bcm_pbmp_t pbm,
                           int *mode, int *weights, int *delay)
{
    bcm_port_t port;
    int num_weights, i;

    BCM_PBMP_ITER(pbm, port) {
        num_weights = _TR3_NUM_COS(unit);
        for (i = 0 ; i < num_weights; i++) {
            BCM_IF_ERROR_RETURN(
                _bcm_tr3_cosq_sched_get(unit, port, i, mode, &weights[i]));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_sched_weight_max_get
 * Purpose:
 *     Retrieve maximum weights for given COS policy
 * Parameters:
 *     unit    - (IN) unit number
 *     mode    - (IN) Scheduling mode (BCM_COSQ_xxx)
 *     weight_max - (OUT) Maximum weight for COS queue.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_sched_weight_max_get(int unit, int mode, int *weight_max)
{
    switch (mode) {
    case BCM_COSQ_STRICT:
        *weight_max = BCM_COSQ_WEIGHT_STRICT;
        break;
    case BCM_COSQ_ROUND_ROBIN:
        *weight_max = 1;
        break;
    case BCM_COSQ_WEIGHTED_ROUND_ROBIN:
    case BCM_COSQ_DEFICIT_ROUND_ROBIN:
        *weight_max = (1 << 7) - 1;
        break;
    default:
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_bandwidth_set
 * Purpose:
 *     Configure COS queue bandwidth control
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     min_quantum   - (IN)
 *     max_quantum   - (IN)
 *     burst_quantum - (IN)
 *     flags         - (IN)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
int
bcm_tr3_cosq_port_bandwidth_set(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 min_quantum, uint32 max_quantum,
                               uint32 burst_quantum, uint32 flags)
{
    uint32 burst_min, burst_max;

    if (cosq < 0) {
        return BCM_E_PARAM;
    }

    burst_min = (min_quantum > 0) ?
          _bcm_td_default_burst_size(unit, port, min_quantum) : 0;

    burst_max = (max_quantum > 0) ?
          _bcm_td_default_burst_size(unit, port, max_quantum) : 0;

    return _bcm_tr3_cosq_bucket_set(unit, port, cosq, min_quantum, max_quantum,
                                    burst_min, burst_max, flags);
}

/*
 * Function:
 *     bcm_tr3_cosq_bandwidth_get
 * Purpose:
 *     Get COS queue bandwidth control configuration
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     min_quantum   - (OUT)
 *     max_quantum   - (OUT)
 *     burst_quantum - (OUT)
 *     flags         - (OUT)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
int
bcm_tr3_cosq_port_bandwidth_get(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 *min_quantum, uint32 *max_quantum,
                               uint32 *burst_quantum, uint32 *flags)
{
    uint32 kbit_burst_min;

    if (cosq < -1) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_bucket_get(unit, port, cosq,
                        min_quantum, max_quantum, &kbit_burst_min,
                        burst_quantum, flags));
    return BCM_E_NONE;
}

int
bcm_tr3_cosq_port_pps_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                         int pps)
{
    uint32 min, max, burst, burst_min, flags;
    bcm_port_t  local_port = port;
    
    if (!IS_CPU_PORT(unit, port)) {
        return BCM_E_PORT;
    } else if (cosq < 0 || cosq >= NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

    if (_bcm_tr3_cosq_port_has_ets(unit, port)) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_l2_gport(unit, port, cosq, 
                                            _BCM_TR3_NODE_MCAST, &port, NULL));
        cosq = 0;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_bucket_get(unit, port, cosq, &min, &max, &burst_min,
                                &burst, &flags));
    
    min = pps;
    burst_min = (min > 0) ?
          _bcm_td_default_burst_size(unit, local_port, min) : 0;
    burst = burst_min;

    return _bcm_tr3_cosq_bucket_set(unit, port, cosq, min, pps, burst_min, burst,
                                    flags | BCM_COSQ_BW_PACKET_MODE);
}

int
bcm_tr3_cosq_port_pps_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                         int *pps)
{
    uint32 min, max, burst, flags;

    if (!IS_CPU_PORT(unit, port)) {
        return BCM_E_PORT;
    } else if (cosq < 0 || cosq >= NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

    if (_bcm_tr3_cosq_port_has_ets(unit, port)) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_l2_gport(unit, port, cosq,
                                          _BCM_TR3_NODE_MCAST, &port, NULL));
        cosq = 0;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_bucket_get(unit, port, cosq, &min, &max, &burst, &burst,
                                  &flags));
    *pps = max;

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_port_burst_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                           int burst)
{
    uint32 min, max, cur_burst, flags;

    if (!IS_CPU_PORT(unit, port)) {
        return BCM_E_PORT;
    } else if (cosq < 0 || cosq >= NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

    if (_bcm_tr3_cosq_port_has_ets(unit, port)) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_l2_gport(unit, port, cosq,
                                                _BCM_TR3_NODE_MCAST, &port, NULL));
        cosq = 0;
    }

    /* Get the current PPS and BURST settings */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_bucket_get(unit, port, cosq, &min, &max, &cur_burst,
                                  &cur_burst, &flags));

    /* Replace the current BURST setting, keep PPS the same */
    return _bcm_tr3_cosq_bucket_set(unit, port, cosq, min, max, burst, burst,
                                    flags | BCM_COSQ_BW_PACKET_MODE);
}

int
bcm_tr3_cosq_port_burst_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                           int *burst)
{
    uint32 min, max, cur_burst, cur_burst_min, flags;

    if (!IS_CPU_PORT(unit, port)) {
        return BCM_E_PORT;
    } else if (cosq < 0 || cosq >= NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

    if (_bcm_tr3_cosq_port_has_ets(unit, port)) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_l2_gport(unit, port, cosq, 
                                            _BCM_TR3_NODE_MCAST, &port, NULL));
        cosq = 0;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_bucket_get(unit, port, cosq, &min, &max, 
                                  &cur_burst_min, &cur_burst, &flags));
    *burst = cur_burst;

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_discard_set(int unit, uint32 flags)
{
    bcm_port_t port;
    int pool;
    _bcm_tr3_mmu_info_t *mmu_info;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }
    
    flags &= ~(BCM_COSQ_DISCARD_NONTCP | BCM_COSQ_DISCARD_COLOR_ALL);

    PBMP_PORT_ITER(unit, port) {
        for (pool = 0; pool < 4; pool++) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_wred_set(unit, port, pool, flags, 0, 0, 0, 0,
                                        FALSE, BCM_COSQ_DISCARD_PORT));
        }
    }

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_discard_get(int unit, uint32 *flags)
{
    bcm_port_t port;

    PBMP_PORT_ITER(unit, port) {
        *flags = 0;
        /* use setting from hardware cosq index 0 of the first port */
        return _bcm_tr3_cosq_wred_get(unit, port, 0, flags, NULL, NULL, NULL,
                                     NULL, BCM_COSQ_DISCARD_PORT);
    }

    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *     bcm_tr3_cosq_discard_port_set
 * Purpose:
 *     Configure port WRED setting
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     color         - (IN)
 *     drop_start    - (IN)
 *     drop_slot     - (IN)
 *     average_time  - (IN)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
int
bcm_tr3_cosq_discard_port_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                             uint32 color, int drop_start, int drop_slope,
                             int average_time)
{
    bcm_port_t local_port;
    bcm_pbmp_t pbmp;
    int gain;
    uint32 min_thresh, max_thresh, shared_limit;
    uint32 rval, bits;
    int numq, i, flags = 0;

    if (drop_start < 0 || drop_start > 100 ||
        drop_slope < 0 || drop_slope > 90) {
        return BCM_E_PARAM;
    }

    /*
     * average queue size is reculated every 8us, the formula is
     * avg_qsize(t + 1) =
     *     avg_qsize(t) + (cur_qsize - avg_qsize(t)) / (2 ** gain)
     * gain = log2(average_time / 8)
     */
    bits = (average_time / 8) & 0xffff;
    if (bits != 0) {
        bits |= bits >> 1;
        bits |= bits >> 2;
        bits |= bits >> 4;
        bits |= bits >> 8;
        gain = _shr_popcount(bits) - 1;
    } else {
        gain = 0;
    }

    /*
     * per-port cell limit may be disabled.
     * per-port per-cos cell limit may be set to use dynamic method.
     * therefore drop_start percentage is based on per-device total shared
     * cells.
     */
    BCM_IF_ERROR_RETURN(READ_OP_BUFFER_SHARED_LIMIT_CELLr(unit, &rval));
    shared_limit = soc_reg_field_get(unit, OP_BUFFER_SHARED_LIMIT_CELLr,
                                     rval, OP_BUFFER_SHARED_LIMIT_CELLf);
    min_thresh = drop_start * shared_limit / 100;

    /* Calculate the max threshold. For a given slope (angle in
     * degrees), determine how many packets are in the range from
     * 0% drop probability to 100% drop probability. Add that
     * number to the min_treshold to the the max_threshold.
     */
    max_thresh = min_thresh + _bcm_tr3_angle_to_cells(drop_slope);
    if (max_thresh > TR3_CELL_FIELD_MAX) {
        max_thresh = TR3_CELL_FIELD_MAX;
    }

    if (BCM_GPORT_IS_SET(port)) {
        if (cosq == -1) {
            cosq = 0;
            flags = BCM_COSQ_DISCARD_PORT;
        }
        numq = 1;
        for (i = 0; i < numq; i++) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_wred_set(unit, port, cosq + i, color,
                                       min_thresh, max_thresh, 100, gain,
                                       TRUE, flags));
        }
    } else {
        if (port == -1) {
            BCM_PBMP_ASSIGN(pbmp, PBMP_PORT_ALL(unit));
        } else if (!SOC_PORT_VALID(unit, port)) {
            return BCM_E_PORT;
        } else {
            BCM_PBMP_PORT_SET(pbmp, port);
        }

        BCM_PBMP_ITER(pbmp, local_port) {
            if (cosq == -1) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve(unit, local_port, cosq,
                                                _BCM_TR3_COSQ_INDEX_STYLE_WRED,
                                                NULL, NULL, &numq));
                cosq = 0;
            } else {
                numq = 1;
            }
            for (i = 0; i < numq; i++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_wred_set(unit, local_port, cosq + i,
                                           color, min_thresh, max_thresh, 100,
                                           gain, TRUE, 0));
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_discard_port_get
 * Purpose:
 *     Get port WRED setting
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     color         - (OUT)
 *     drop_start    - (OUT)
 *     drop_slot     - (OUT)
 *     average_time  - (OUT)
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     If port is any form of local port, cosq is the hardware queue index.
 */
int
bcm_tr3_cosq_discard_port_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                             uint32 color, int *drop_start, int *drop_slope,
                             int *average_time)
{
    bcm_port_t local_port;
    bcm_pbmp_t pbmp;
    int gain, drop_prob;
    uint32 min_thresh, max_thresh, shared_limit;
    uint32 rval;

    if (drop_start == NULL || drop_slope == NULL || average_time == NULL) {
        return BCM_E_PARAM;
    }

    if (BCM_GPORT_IS_SET(port)) {
        local_port = port;
    } else {
        if (port == -1) {
            BCM_PBMP_ASSIGN(pbmp, PBMP_PORT_ALL(unit));
        } else if (!SOC_PORT_VALID(unit, port)) {
            return BCM_E_PORT;
        } else {
            BCM_PBMP_PORT_SET(pbmp, port);
        }
        BCM_PBMP_ITER(pbmp, local_port) {
            break;
        }
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_wred_get(unit, local_port, cosq == -1 ? 0 : cosq,
                               &color, &min_thresh, &max_thresh, &drop_prob,
                               &gain, 0));

    /*
     * average queue size is reculated every 4us, the formula is
     * avg_qsize(t + 1) =
     *     avg_qsize(t) + (cur_qsize - avg_qsize(t)) / (2 ** gain)
     */
    *average_time = (1 << gain) * 4;

    /*
     * per-port cell limit may be disabled.
     * per-port per-cos cell limit may be set to use dynamic method.
     * therefore drop_start percentage is based on per-device total shared
     * cells.
     */
    BCM_IF_ERROR_RETURN(READ_OP_BUFFER_SHARED_LIMIT_CELLr(unit, &rval));
    shared_limit = soc_reg_field_get(unit, OP_BUFFER_SHARED_LIMIT_CELLr,
                                     rval, OP_BUFFER_SHARED_LIMIT_CELLf);

    if (min_thresh > shared_limit) {
        min_thresh = shared_limit;
    }

    if (max_thresh > shared_limit) {
        max_thresh = shared_limit;
    }

    *drop_start = min_thresh * 100 / shared_limit;

    /* Calculate the slope using the min and max threshold.
     * The angle is calculated knowing drop probability at min
     * threshold is 0% and drop probability at max threshold is 100%.
     */
    *drop_slope = _bcm_tr3_cells_to_angle(max_thresh - min_thresh);

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_distribute_u64(uint64 value, uint64 *l, uint64 *r)
{
    uint64 tmp, tmp1;

    tmp = value;
    COMPILER_64_SHR(tmp, 1);
    *l = tmp;

    COMPILER_64_SET(tmp1, 0, 1);
    COMPILER_64_AND(tmp1, value);
    COMPILER_64_ADD_64(tmp1, tmp);
    *r = tmp1;
    return 0;
}

int
bcm_tr3_cosq_stat_set(int unit, bcm_gport_t port, bcm_cos_queue_t cosq,
                     bcm_cosq_stat_t stat, uint64 value)
{
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port;
    int startq, numq, i, from_cos, to_cos, ci;
    uint64  l, r;

    _bcm_tr3_distribute_u64(value, &l, &r);

    switch (stat) {
    case bcmCosqStatDroppedPackets:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                                 SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC, startq,
                                 value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                             SOC_COUNTER_NON_DMA_COSQ_DROP_PKT, startq - 1024,
                                 value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            if (cosq == BCM_COS_INVALID) {
                from_cos = to_cos = 0;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, REG_PORT_ANY,
                                         SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC,
                                         startq + i, l));
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, -1,
                                         SOC_COUNTER_NON_DMA_COSQ_DROP_PKT,
                                     startq + i - 1024, r));
                }
            }
        }
        break;
    case bcmCosqStatDroppedBytes:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                                 SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC, startq,
                                 value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                   SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE, startq - 1024, value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            if (cosq == BCM_COS_INVALID) {
                from_cos = to_cos = 0;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, REG_PORT_ANY,
                               SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC, startq + i, l));
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, REG_PORT_ANY,
                           SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE, startq + i - 1024, r));
                }
            }
        }
        break;
    case bcmCosqStatYellowCongestionDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (soc_counter_set(unit, local_port,
                             SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW, 0,
                             value));
        break;
    case bcmCosqStatRedCongestionDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (soc_counter_set(unit, local_port,
                             SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED, 0,
                             value));
        break;
    case bcmCosqStatGreenDiscardDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (soc_counter_set(unit, local_port,
                             SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN, 0,
                             value));
        break;
    case bcmCosqStatYellowDiscardDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (soc_counter_set(unit, local_port,
                             SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW, 0,
                             value));
        break;
    case bcmCosqStatRedDiscardDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (soc_counter_set(unit, local_port,
                             SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED, 0, value));
        break;
    case bcmCosqStatOutPackets:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                                 SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC, startq,
                                 value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                                 SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT, startq,
                                 value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            if (cosq == BCM_COS_INVALID) {
                from_cos = to_cos = 0;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, REG_PORT_ANY,
                            SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC, startq + i, l));
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, REG_PORT_ANY,
                            SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT, startq + i, r));
                }
            }
        }
        break;
    case bcmCosqStatOutBytes:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                                 SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC, startq,
                                 value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (soc_counter_set(unit, REG_PORT_ANY,
                                 SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE, startq,
                                 value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            if (cosq == BCM_COS_INVALID) {
                from_cos = to_cos = 0;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, REG_PORT_ANY,
                           SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC, startq + i, l));
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    BCM_IF_ERROR_RETURN
                        (soc_counter_set(unit, REG_PORT_ANY,
                            SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE, startq + i, r));
                }
            }
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_stat_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                     bcm_cosq_stat_t stat, int sync_mode, uint64 *value)
{
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port;
    int startq, numq, i, from_cos, to_cos, ci;
    uint64 sum, value64;
    int (*counter_get) (int , soc_port_t , soc_reg_t , int , uint64 *);

    if (value == NULL) {
        return BCM_E_PARAM;
    }

    /*
     * if sync-mode is set, update the software cached counter value, 
     * with the hardware count and then retrieve the count.
     * else return the software cache counter value.
     */
    counter_get = (sync_mode == 1)? soc_counter_sync_get: soc_counter_get;

    switch (stat) {
    case bcmCosqStatDroppedPackets:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                            SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC, startq,
                                 value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                    SOC_COUNTER_NON_DMA_COSQ_DROP_PKT, startq - 1024, value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            COMPILER_64_ZERO(sum);
            if (cosq == BCM_COS_INVALID) {
                from_cos = 0;
                to_cos = (IS_CPU_PORT(unit, local_port)) ? 
                            NUM_CPU_COSQ(unit) - 1 : _TR3_NUM_COS(unit) - 1;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC,
                                         startq + i, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_COSQ_DROP_PKT,
                                         startq + i - 1024, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
            }
            *value = sum;
        }
        break;
    case bcmCosqStatDroppedBytes:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                             SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC, startq,
                             value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                             SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE, 
                             startq - 1024, value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            COMPILER_64_ZERO(sum);
            if (cosq == BCM_COS_INVALID) {
                from_cos = 0;
                to_cos = (IS_CPU_PORT(unit, local_port)) ? 
                            NUM_CPU_COSQ(unit) - 1 : _TR3_NUM_COS(unit) - 1;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC,
                                         startq + i, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE,
                                         startq + i - 1024, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
            }
            *value = sum;
        }
        break;
    case bcmCosqStatYellowCongestionDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (counter_get(unit, local_port,
                         SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW, 0,
                             value));
        break;
    case bcmCosqStatRedCongestionDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (counter_get(unit, local_port,
                         SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED, 0,
                             value));
        break;
    case bcmCosqStatGreenDiscardDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (counter_get(unit, local_port,
                         SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN, 0,
                             value));
        break;
    case bcmCosqStatYellowDiscardDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (counter_get(unit, local_port,
                         SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW, 0,
                             value));
        break;
    case bcmCosqStatRedDiscardDroppedPackets:
        if (cosq != BCM_COS_INVALID) {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
        BCM_IF_ERROR_RETURN
            (counter_get(unit, local_port,
                         SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED, 0, value));
        break;
    case bcmCosqStatOutPackets:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                             SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC, startq,
                                 value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                             SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT, startq,
                                 value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            COMPILER_64_ZERO(sum);
            if (cosq == BCM_COS_INVALID) {
                from_cos = 0;
                to_cos = (IS_CPU_PORT(unit, local_port)) ? 
                            NUM_CPU_COSQ(unit) - 1 : _TR3_NUM_COS(unit) - 1;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC,
                                     startq + i, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT,
                                     startq + i, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
            }
            *value = sum;
        }
        break;
    case bcmCosqStatOutBytes:
        if (BCM_GPORT_IS_SCHEDULER(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, port, 0, NULL, 
                                        &local_port, NULL, &node));
            BCM_IF_ERROR_RETURN(
                   _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
            port = node->gport;
            cosq = 0;
        }

        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                             SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC, startq,
                             value));
        } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(port)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, port, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &startq, NULL));
            /* coverity[NEGATIVE_RETURNS: FALSE] */
            BCM_IF_ERROR_RETURN
                (counter_get(unit, REG_PORT_ANY,
                             SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE, startq,
                             value));
        } else if (BCM_GPORT_IS_SCHEDULER(port)) {
            return BCM_E_PARAM;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
            COMPILER_64_ZERO(sum);
            if (cosq == BCM_COS_INVALID) {
                from_cos = 0;
                to_cos = (IS_CPU_PORT(unit, local_port)) ? 
                            NUM_CPU_COSQ(unit) - 1 : _TR3_NUM_COS(unit) - 1;
            } else {
                from_cos = to_cos = cosq;
            }
            for (ci = from_cos; ci <= to_cos; ci++) {
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC,
                                     startq + i, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_tr3_cosq_index_resolve
                     (unit, port, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                      &local_port, &startq, &numq));
                for (i = 0; i < numq; i++) {
                    /* coverity[NEGATIVE_RETURNS: FALSE] */
                    BCM_IF_ERROR_RETURN
                        (counter_get(unit, REG_PORT_ANY,
                                     SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE,
                                     startq + i, &value64));
                    COMPILER_64_ADD_64(sum, value64);
                }
            }
            *value = sum;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr3_cosq_quantize_table_set
 * Purpose:
 *     Generate quantized feedback lookup table
 * Parameters:
 *     unit          - (IN) unit number
 *     profile_index - (IN) profile index
 *     weight_code   - (IN) weight encoding
 *     set_point     - (IN) queue size set point
 *     active_offset - (OUT) most significant non-zero bit position
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     The formular to quantize feedback value is:
 *     quantized_feedbad = celling(feedback / max_feedback * 63)
 *         where max_feedback = (2 * cpW + 1) * cpQsp
 *         cpW is feedback weight
 *         cpQsp is queue size set point
 */
STATIC int
_bcm_tr3_cosq_quantize_table_set(int unit, int profile_index, int weight_code,
                                int set_point, int *active_offset)
{
    int ref_count;
    int weight_x_4, feedback, max_feedback, quantized_feedback, shift;
    int base_index, index;

    /* weight = 2 ** (weight_code - 2) e.g. 0 for 1/4, 1 for 1/2, ...  */
    weight_x_4 = 1 << weight_code;
    max_feedback = ((2 * weight_x_4 + 4) * set_point + 3) / 4;
    shift = 0;
    while ((max_feedback >> shift) > 127) {
        shift++;
    }

    BCM_IF_ERROR_RETURN
        (soc_profile_reg_ref_count_get(unit, _bcm_tr3_feedback_profile[unit],
                                       profile_index, &ref_count));

    /*
     * Hardware use lookup table to find quantized feedback in order to avoid
     * division, use most significant 7-bit of feedback value as lookup table
     * index to find quantized_feedback value
     */
    if (ref_count == 1) {
        base_index = profile_index << 7;
        for (index = 0; index < 128; index++) {
            feedback = index << shift;
            if (feedback > max_feedback) {
                quantized_feedback = 63;
            } else {
                quantized_feedback = (feedback * 63 + max_feedback - 1) /
                    max_feedback;
            }
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, MMU_QCN_QFBTBm,
                                        base_index  + index, CPQ_QNTZFBf,
                                        quantized_feedback));
        }
    }

    *active_offset = shift + 6;

    return BCM_E_NONE;
}


STATIC int
_bcm_tr3_cosq_sample_int_table_set(int unit, int min, int max,
                                  uint32 *profile_index)
{
    mmu_qcn_sitb_entry_t sitb_entries[64];
    void *entries[1];
    int i, j, center, inc, value, index;

    sal_memset(sitb_entries, 0, sizeof(sitb_entries));
    entries[0] = &sitb_entries;
    for (i = 0; i < 8; i++) {
        center = max - (max - min) * i / 7;
        inc = (max - min) / 7 / 8;
        for (j = 0; j < 8; j++, inc = -inc) {
            index = i * 8 + j;
            value = center + (j + 1) / 2 * inc;
            if (value > 255) {
                value = 255;
            } else if (value < 1) {
                value = 1;
            }
            soc_mem_field32_set(unit, MMU_QCN_SITBm, &sitb_entries[index],
                                CPQ_SIf, value);
        }
    }

    return soc_profile_mem_add(unit, _bcm_tr3_sample_int_profile[unit], entries,
                               64, (uint32 *)profile_index);
}

/*
 * Function:
 *     bcm_tr3_cosq_congestion_queue_set
 * Purpose:
 *     Enable/Disable a cos queue as congestion managed queue
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     index         - (IN) congestion managed queue index
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_congestion_queue_set(int unit, bcm_port_t port,
                                 bcm_cos_queue_t cosq, int index)
{
    bcm_port_t local_port;
    uint32 rval;
    uint64 rval64, *rval64s[1];
    mmu_qcn_enable_entry_t enable_entry;
    int active_offset, qindex;
    uint32 profile_index, sample_profile_index;
    int weight_code, set_point;

    if (cosq < 0 || cosq >= _TR3_NUM_COS(unit)) {
        return BCM_E_PARAM;
    }
    if (index < -1 || index >= 512) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_QCN,
                                    &local_port, &qindex, NULL));

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_QCN_ENABLEm,
                                     MEM_BLOCK_ANY, qindex, &enable_entry));
    if (index == -1) {
        if (!soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &enable_entry, CPQ_ENf)) {
            return BCM_E_NONE;
        }

        index = soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &enable_entry,
                                    CPQ_PROFILE_INDEXf);
        soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &enable_entry,
                            CPQ_ENf, 0);
        BCM_IF_ERROR_RETURN
            (soc_mem_write(unit, MMU_QCN_ENABLEm, MEM_BLOCK_ANY, qindex,
                           &enable_entry));

        profile_index =
            soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &enable_entry,
                                 EQTB_INDEXf);

        BCM_IF_ERROR_RETURN
            (soc_profile_reg_delete(unit, _bcm_tr3_feedback_profile[unit],
                                    profile_index));

        profile_index =
            soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &enable_entry, SITB_SELf);

        BCM_IF_ERROR_RETURN
            (soc_profile_mem_delete(unit, _bcm_tr3_sample_int_profile[unit],
                                profile_index * 64));
    } else {
        if (soc_mem_field32_get(unit, MMU_QCN_ENABLEm, 
                                    &enable_entry, CPQ_ENf)) {
            return BCM_E_BUSY;
        }

        /* Use hardware reset value as default quantize setting */
        weight_code = 2;
        set_point = 0x96;
        rval = 0;
        soc_reg_field_set(unit, MMU_QCN_CPQ_SEQr, &rval, CPWf, weight_code);
        soc_reg_field_set(unit, MMU_QCN_CPQ_SEQr, &rval, CPQEQf, set_point);
        COMPILER_64_SET(rval64, 0, rval);
        rval64s[0] = &rval64;
        BCM_IF_ERROR_RETURN
            (soc_profile_reg_add(unit, _bcm_tr3_feedback_profile[unit],
                                 rval64s, 1, (uint32 *)&profile_index));

        /* Update quantized feedback calculation lookup table */
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_quantize_table_set(unit, profile_index, weight_code,
                                             set_point, &active_offset));

        /* Pick some sample interval */
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_sample_int_table_set(unit, 13, 127,
                                               &sample_profile_index));

        soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &enable_entry,
                            QNTZ_ACT_OFFSETf, active_offset);

        soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &enable_entry,
                            SITB_SELf, sample_profile_index/64);

        soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &enable_entry,
                            CPQ_INDEXf, index);

        soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &enable_entry,
                            CPQ_ENf, 1);

        BCM_IF_ERROR_RETURN
            (soc_mem_write(unit, MMU_QCN_ENABLEm, MEM_BLOCK_ANY, qindex,
                           &enable_entry));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_congestion_queue_get
 * Purpose:
 *     Get congestion managed queue index of the specified cos queue
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     index         - (OUT) congestion managed queue index
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_congestion_queue_get(int unit, bcm_port_t port,
                                 bcm_cos_queue_t cosq, int *index)
{
    bcm_port_t local_port;
    int qindex;
    mmu_qcn_enable_entry_t entry;

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    if (cosq < 0 || cosq >= _TR3_NUM_COS(unit)) {
        return BCM_E_PARAM;
    }
    if (index == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_QCN,
                                    &local_port, &qindex, NULL));

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, MMU_QCN_ENABLEm, MEM_BLOCK_ANY, qindex, &entry));
    if (soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &entry, CPQ_ENf)) {
        *index = soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &entry,
                                     CPQ_INDEXf);
    } else {
        *index = -1;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_congestion_quantize_set
 * Purpose:
 *     Set quantized congestion feedback algorithm parameters
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     weight_code   - (IN) weight encoding (use -1 for unchange)
 *     set_point     - (IN) queue size set point (use -1 for unchange)
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_congestion_quantize_set(int unit, bcm_port_t port,
                                    bcm_cos_queue_t cosq, int weight_code,
                                    int set_point)
{
    bcm_port_t local_port;
    uint32 rval;
    uint64 rval64, *rval64s[1];
    mmu_qcn_enable_entry_t enable_entry;
    int cpq_index, active_offset, qindex;
    uint32 profile_index, old_profile_index;

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    BCM_IF_ERROR_RETURN
        (bcm_tr3_cosq_congestion_queue_get(unit, port, cosq, &cpq_index));
    if (cpq_index == -1) {
        /* The cosq specified is not enabled as congestion managed queue */
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_QCN,
                                    &local_port, &qindex, NULL));

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_QCN_ENABLEm,
                                     MEM_BLOCK_ANY, qindex, &enable_entry));
    old_profile_index =
        soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &enable_entry, CPQ_PROFILE_INDEXf);

    BCM_IF_ERROR_RETURN(READ_MMU_QCN_CPQ_SEQr(unit, old_profile_index, &rval));
    if (weight_code == -1) {
        weight_code = soc_reg_field_get(unit, MMU_QCN_CPQ_SEQr, rval, CPWf);
    } else {
        if (weight_code < 0 || weight_code > 7) {
            return BCM_E_PARAM;
        }
        soc_reg_field_set(unit, MMU_QCN_CPQ_SEQr, &rval, CPWf, weight_code);
    }
    if (set_point == -1) {
        set_point = soc_reg_field_get(unit, MMU_QCN_CPQ_SEQr, rval, CPQEQf);
    } else {
        if (set_point < 0 || set_point > 0xffff) {
            return BCM_E_PARAM;
        }
        soc_reg_field_set(unit, MMU_QCN_CPQ_SEQr, &rval, CPQEQf, set_point);
    }
    COMPILER_64_SET(rval64, 0, rval);
    rval64s[0] = &rval64;
    /* Add new feedback parameter profile */
    BCM_IF_ERROR_RETURN
        (soc_profile_reg_add(unit, _bcm_tr3_feedback_profile[unit], rval64s,
                             1, (uint32 *)&profile_index));
    /* Delete original feedback parameter profile */
    BCM_IF_ERROR_RETURN
        (soc_profile_reg_delete(unit, _bcm_tr3_feedback_profile[unit],
                                old_profile_index));

    /* Update quantized feedback calculation lookup table */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_quantize_table_set(unit, profile_index, weight_code,
                                         set_point, &active_offset));

    soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &enable_entry,
                            QNTZ_ACT_OFFSETf, active_offset);
    soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &enable_entry,
                            CPQ_PROFILE_INDEXf, profile_index);
    BCM_IF_ERROR_RETURN
            (soc_mem_write(unit, MMU_QCN_ENABLEm, MEM_BLOCK_ANY, qindex,
                           &enable_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_congestion_quantize_get
 * Purpose:
 *     Get quantized congestion feedback algorithm parameters
 * Parameters:
 *     unit          - (IN) unit number
 *     port          - (IN) port number or GPORT identifier
 *     cosq          - (IN) COS queue number
 *     weight_code   - (OUT) weight encoding
 *     set_point     - (OUT) queue size set point
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_congestion_quantize_get(int unit, bcm_port_t port,
                                    bcm_cos_queue_t cosq, int *weight_code,
                                    int *set_point)
{
    bcm_port_t local_port;
    int cpq_index, profile_index, qindex;
    uint32 rval;
    mmu_qcn_enable_entry_t enable_entry;

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    BCM_IF_ERROR_RETURN
        (bcm_tr3_cosq_congestion_queue_get(unit, port, cosq, &cpq_index));
    if (cpq_index == -1) {
        /* The cosq specified is not enabled as congestion managed queue */
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_QCN,
                                    &local_port, &qindex, NULL));
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_QCN_ENABLEm,
                                     MEM_BLOCK_ANY, qindex, &enable_entry));

    profile_index =
        soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &enable_entry, CPQ_PROFILE_INDEXf);

    BCM_IF_ERROR_RETURN(READ_MMU_QCN_CPQ_SEQr(unit, profile_index, &rval));
    if (weight_code != NULL) {
        *weight_code = soc_reg_field_get(unit, MMU_QCN_CPQ_SEQr, rval, CPWf);
    }
    if (set_point != NULL) {
        *set_point = soc_reg_field_get(unit, MMU_QCN_CPQ_SEQr, rval, CPQEQf);
    }

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_congestion_sample_int_set(int unit, bcm_port_t port,
                                      bcm_cos_queue_t cosq, int min, int max)
{
    bcm_port_t local_port;
    mmu_qcn_enable_entry_t entry;
    mmu_qcn_sitb_entry_t sitb_entry;
    int qindex;
    uint32 profile_index, old_profile_index;

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_QCN,
                                    &local_port, &qindex, NULL));

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, MMU_QCN_ENABLEm, MEM_BLOCK_ANY, qindex, &entry));

    if (!soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &entry, CPQ_ENf)) {
        return BCM_E_PARAM;
    }

    old_profile_index =
        soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &entry, SITB_SELf);

    if (max == -1) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_QCN_SITBm, MEM_BLOCK_ANY,
                                         old_profile_index * 64, &sitb_entry));
        max = soc_mem_field32_get(unit, MMU_QCN_SITBm, &sitb_entry, CPQ_SIf);
    } else {
        if (max < 1 || max > 255) {
            return BCM_E_PARAM;
        }
    }
    if (min == -1) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_QCN_SITBm, MEM_BLOCK_ANY,
                                         old_profile_index * 64 + 63,
                                         &sitb_entry));
        min = soc_mem_field32_get(unit, MMU_QCN_SITBm, &sitb_entry, CPQ_SIf);
    } else {
        if (min < 1 || min > 255) {
            return BCM_E_PARAM;
        }
    }

    /* Add new sample interval profile */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_sample_int_table_set(unit, min, max, &profile_index));

    /* Delete original sample interval profile */
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_delete(unit, _bcm_tr3_sample_int_profile[unit],
                                old_profile_index * 64));

    soc_mem_field32_set(unit, MMU_QCN_ENABLEm, &entry, SITB_SELf, 
                        profile_index / 64);
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, MMU_QCN_ENABLEm, MEM_BLOCK_ANY,
                                      qindex, &entry));
    return BCM_E_NONE;
}

int
bcm_tr3_cosq_congestion_sample_int_get(int unit, bcm_port_t port,
                                      bcm_cos_queue_t cosq, int *min, int *max)
{
    bcm_port_t local_port;
    mmu_qcn_enable_entry_t entry;
    mmu_qcn_sitb_entry_t sitb_entry;
    int qindex;
    uint32 old_profile_index;

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, port, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_QCN,
                                    &local_port, &qindex, NULL));

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, MMU_QCN_ENABLEm, MEM_BLOCK_ANY, qindex, &entry));

    if (!soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &entry, CPQ_ENf)) {
        return BCM_E_PARAM;
    }

    old_profile_index =
    soc_mem_field32_get(unit, MMU_QCN_ENABLEm, &entry, SITB_SELf);

    if (max) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_QCN_SITBm, MEM_BLOCK_ANY,
                                     old_profile_index * 64, &sitb_entry));
        *max = soc_mem_field32_get(unit, MMU_QCN_SITBm, &sitb_entry, CPQ_SIf);
    }

    if (min) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, MMU_QCN_SITBm, MEM_BLOCK_ANY,
                                     old_profile_index * 64 + 63,
                                     &sitb_entry));
        *min = soc_mem_field32_get(unit, MMU_QCN_SITBm, &sitb_entry, CPQ_SIf);
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_egr_pool_set(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                          bcm_cosq_control_t type, int arg)
{
    bcm_port_t local_port;
    int startq;
    uint32 rval;
    int num_cells;
    uint32 entry[SOC_MAX_MEM_WORDS];

    num_cells = 0;

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, gport, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_EGR_POOL,
                                    &local_port, &startq, NULL));

    if (type == bcmCosqControlEgressPoolLimitEnable) {
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(soc_reg32_get(unit, OP_PORT_CONFIG1_CELLr, 
                                            local_port, 0, &rval));
            soc_reg_field_set(unit, OP_PORT_CONFIG1_CELLr, &rval, 
                          PORT_LIMIT_ENABLE_CELLf, arg ? 1 : 0);
            BCM_IF_ERROR_RETURN(soc_reg32_set(unit, OP_PORT_CONFIG1_CELLr, 
                                            local_port, 0, rval));
        } else {
            BCM_IF_ERROR_RETURN(soc_reg32_get(unit, OP_QUEUE_CONFIG_CELLr, 
                                            local_port, startq, &rval));
            soc_reg_field_set(unit, OP_QUEUE_CONFIG_CELLr, 
                            &rval, PORT_LIMIT_ENABLE_CELLf, arg ? 1 : 0);

            BCM_IF_ERROR_RETURN(soc_reg32_set(unit, OP_QUEUE_CONFIG_CELLr, 
                                                    local_port, startq, rval));
        }
        return BCM_E_NONE;
    } else if (type == bcmCosqControlEgressPool) {
        if (arg < 0 || arg > 3) {
            return BCM_E_PARAM;
        }
    } else if (type == bcmCosqControlEgressPoolLimitBytes ||
               type == bcmCosqControlEgressPoolYellowLimitBytes ||
               type == bcmCosqControlEgressPoolRedLimitBytes) {
        if (arg < 0) {
            return BCM_E_PARAM;
        }
        num_cells = arg / _BCM_TR3_BYTES_PER_CELL;
        if (num_cells > _BCM_TR3_TOTAL_CELLS) {
            return BCM_E_PARAM;
        }
    }

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
        BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_Q_TO_QGRP_MAPm, MEM_BLOCK_ALL,
                             startq, entry));
    } else {
        BCM_IF_ERROR_RETURN(
            soc_reg32_get(unit, OP_QUEUE_CONFIG1_CELLr, local_port, startq, &rval));
    }

    switch (type) {
    case bcmCosqControlEgressPool:
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            soc_mem_field32_set(unit, MMU_THDO_Q_TO_QGRP_MAPm, entry, Q_SPIDf, arg);
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, MMU_THDO_Q_TO_QGRP_MAPm, 
                                        MEM_BLOCK_ALL, startq, entry));
        } else {
            soc_reg_field_set(unit, OP_QUEUE_CONFIG1_CELLr, &rval, Q_SPIDf, arg);
            SOC_IF_ERROR_RETURN
                (WRITE_OP_QUEUE_CONFIG1_CELLr(unit, local_port, startq, rval));
        }
        break;

    case bcmCosqControlEgressPoolLimitBytes:
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_CONFIG_QUEUEm, MEM_BLOCK_ALL,
                             startq, entry));
            soc_mem_field32_set(unit, MMU_THDO_CONFIG_QUEUEm, entry,
                                Q_SHARED_LIMIT_CELLf, num_cells);
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, MMU_THDO_CONFIG_QUEUEm, 
                                        MEM_BLOCK_ALL, startq, entry));
        } else {
            BCM_IF_ERROR_RETURN(
             soc_reg32_get(unit, OP_QUEUE_CONFIG_CELLr, local_port, startq, &rval));
            soc_reg_field_set(unit, OP_QUEUE_CONFIG_CELLr, &rval,
                              Q_SHARED_LIMIT_CELLf, num_cells);
            BCM_IF_ERROR_RETURN(soc_reg32_set(unit, OP_QUEUE_CONFIG_CELLr, 
                                local_port, startq, rval));
        }

        break;
    case bcmCosqControlEgressPoolYellowLimitBytes:
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_CONFIG_QUEUEm, MEM_BLOCK_ALL,
                             startq, entry));
            soc_mem_field32_set(unit, MMU_THDO_CONFIG_QUEUEm, 
                            entry, LIMIT_YELLOW_CELLf, num_cells / 8);
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, MMU_THDO_CONFIG_QUEUEm, 
                                        MEM_BLOCK_ALL, startq, entry));
        } else {
            return BCM_E_UNAVAIL;
        }
        break;
    case bcmCosqControlEgressPoolRedLimitBytes:
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_CONFIG_QUEUEm, MEM_BLOCK_ALL,
                             startq, entry));
            soc_mem_field32_set(unit, MMU_THDO_CONFIG_QUEUEm, 
                            entry, LIMIT_RED_CELLf, num_cells / 8);
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, MMU_THDO_CONFIG_QUEUEm, 
                                        MEM_BLOCK_ALL, startq, entry));
        } else {
            BCM_IF_ERROR_RETURN(
             soc_reg32_get(unit, OP_QUEUE_LIMIT_COLOR_CELLr, 
                            local_port, startq, &rval));
            soc_reg_field_set(unit, OP_QUEUE_CONFIG_CELLr, &rval, REDf, num_cells/8);
            BCM_IF_ERROR_RETURN(soc_reg32_set(unit, OP_QUEUE_LIMIT_COLOR_CELLr, 
                                local_port, startq, rval));
        }
        break;
    default:
        return BCM_E_UNAVAIL;
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_egr_pool_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                          bcm_cosq_control_t type, int *arg)
{
    bcm_port_t local_port;
    int startq, pool;
    uint32 rval;
    uint32 entry[SOC_MAX_MEM_WORDS];

    if (!arg) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_index_resolve(unit, gport, cosq,
                                    _BCM_TR3_COSQ_INDEX_STYLE_EGR_POOL,
                                    &local_port, &startq, NULL));

    if (type == bcmCosqControlEgressPoolLimitEnable) {
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(soc_reg32_get(unit, OP_PORT_CONFIG1_CELLr, 
                                            local_port, 0, &rval));
            *arg = soc_reg_field_get(unit, OP_PORT_CONFIG1_CELLr, rval, 
                                          PORT_LIMIT_ENABLE_CELLf);
        } else {
            BCM_IF_ERROR_RETURN(soc_reg32_get(unit, OP_QUEUE_CONFIG_CELLr, 
                                            local_port, startq, &rval));
            *arg = soc_reg_field_get(unit, OP_QUEUE_CONFIG_CELLr, 
                                        rval, PORT_LIMIT_ENABLE_CELLf);
        }
        return BCM_E_NONE;
    }

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
        BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_Q_TO_QGRP_MAPm, MEM_BLOCK_ALL,
                             startq, entry));
        pool = soc_mem_field32_get(unit, MMU_THDO_Q_TO_QGRP_MAPm, entry, Q_SPIDf);
    } else {
        BCM_IF_ERROR_RETURN(
            soc_reg32_get(unit, OP_QUEUE_CONFIG1_CELLr, local_port, startq, &rval));
        pool = soc_reg_field_get(unit, OP_QUEUE_CONFIG1_CELLr, rval, Q_SPIDf);
    }

    switch (type) {
    case bcmCosqControlEgressPool:
        *arg = pool;
        break;

    case bcmCosqControlEgressPoolLimitBytes:
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_CONFIG_QUEUEm, MEM_BLOCK_ALL,
                             startq, entry));
            *arg = soc_mem_field32_get(unit, MMU_THDO_CONFIG_QUEUEm, entry,
                                Q_SHARED_LIMIT_CELLf);
        } else {
            BCM_IF_ERROR_RETURN(
             soc_reg32_get(unit, OP_QUEUE_CONFIG_CELLr, local_port, startq, &rval));
            *arg = soc_reg_field_get(unit, OP_QUEUE_CONFIG_CELLr, rval,
                              Q_SHARED_LIMIT_CELLf) * 8;
        }
        break;
    case bcmCosqControlEgressPoolYellowLimitBytes:
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_CONFIG_QUEUEm, MEM_BLOCK_ALL,
                             startq, entry));
            *arg = soc_mem_field32_get(unit, MMU_THDO_CONFIG_QUEUEm, 
                            entry, LIMIT_YELLOW_CELLf) * 8;
        } else {
            return BCM_E_UNAVAIL;
        }
        break;
    case bcmCosqControlEgressPoolRedLimitBytes:
        if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
            BCM_IF_ERROR_RETURN(
                soc_mem_read(unit, MMU_THDO_CONFIG_QUEUEm, MEM_BLOCK_ALL,
                             startq, entry));
            *arg = soc_mem_field32_get(unit, MMU_THDO_CONFIG_QUEUEm, 
                                        entry, LIMIT_RED_CELLf) * 8;
        } else {
            BCM_IF_ERROR_RETURN(
             soc_reg32_get(unit, OP_QUEUE_LIMIT_COLOR_CELLr, 
                            local_port, startq, &rval));
            *arg = soc_reg_field_get(unit, OP_QUEUE_CONFIG_CELLr, rval, REDf)*8;
        }
        break;
    default:
        return BCM_E_UNAVAIL;
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_ef_op_at_index(int unit, int hw_index, _bcm_cosq_op_t op, 
                             int *ef_val)
{
    lls_l2_parent_entry_t entry;

    SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, LLS_L2_PARENTm, MEM_BLOCK_ALL, hw_index, &entry));

    if (op == _BCM_COSQ_OP_GET) {
        *ef_val = soc_mem_field32_get(unit, LLS_L2_PARENTm, &entry, C_EFf);
    } else {
        soc_mem_field32_set(unit, LLS_L2_PARENTm, &entry, C_EFf, *ef_val);
        SOC_IF_ERROR_RETURN
            (soc_mem_write(unit, LLS_L2_PARENTm, MEM_BLOCK_ALL, 
                            hw_index, &entry));
    }
   
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_ef_op(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq, int *arg,
                    _bcm_cosq_op_t op)
{
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port;
    int index, numq, from_cos, to_cos, ci;

    if (BCM_GPORT_IS_SCHEDULER(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, 
                                    &local_port, NULL, &node));
        BCM_IF_ERROR_RETURN(
               _bcm_tr3_cosq_child_node_at_input(node, cosq, &node));
        gport = node->gport;
        cosq = 0;
    }

    if (BCM_GPORT_IS_UCAST_QUEUE_GROUP(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_index_resolve
             (unit, gport, cosq, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
              &local_port, &index, NULL));
        return _bcm_tr3_cosq_ef_op_at_index(unit, index, op, arg);
    } else if (BCM_GPORT_IS_MCAST_QUEUE_GROUP(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_index_resolve
             (unit, gport, cosq, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
              &local_port, &index, NULL));
        return _bcm_tr3_cosq_ef_op_at_index(unit, index, op, arg);
    } else if (BCM_GPORT_IS_SCHEDULER(gport)) {
        return BCM_E_PARAM;
    } else {
        BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_port_has_ets(unit, local_port));
        if (cosq == BCM_COS_INVALID) {
            from_cos = 0;
            to_cos = _TR3_NUM_COS(unit) - 1;
        } else {
            from_cos = to_cos = cosq;
        }
        for (ci = from_cos; ci <= to_cos; ci++) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, gport, ci, _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
                  &local_port, &index, &numq));

            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_ef_op_at_index(unit, 
                                index, op, arg));
            if (op == _BCM_COSQ_OP_GET) {
                return BCM_E_NONE;
            }

            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_index_resolve
                 (unit, gport, ci, _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
                  &local_port, &index, &numq));

            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_ef_op_at_index(unit, 
                                index, op, arg));
        }
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_port_qnum_get(int unit, bcm_gport_t gport,
        bcm_cos_queue_t cosq, bcm_cosq_control_t type, int *arg)
{
    bcm_port_t local_port;
    if (!arg) {
        return BCM_E_PARAM;
    }

    if (!BCM_COSQ_QUEUE_VALID(unit, cosq)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, gport, &local_port));
    if (local_port < 0) {
        return BCM_E_PORT;
    }

    if (type == bcmCosqControlPortQueueUcast) {
        *arg = SOC_INFO(unit).port_uc_cosq_base[local_port] + cosq;
    } else {
        *arg = SOC_INFO(unit).port_cosq_base[local_port] + cosq;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_tr3_cosq_qcn_proxy_set(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq, 
                            bcm_cosq_control_t type, int arg)
{
    int local_port;
    uint32 rval;
    
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, gport, &local_port));
    
    BCM_IF_ERROR_RETURN(READ_QCN_CNM_PRP_CTRLr(unit, local_port, &rval));
    soc_reg_field_set(unit, QCN_CNM_PRP_CTRLr, &rval, ENABLEf, !!arg);
    soc_reg_field_set(unit, QCN_CNM_PRP_CTRLr, &rval, PRIORITY_BMAPf, 0xff);
    BCM_IF_ERROR_RETURN(WRITE_QCN_CNM_PRP_CTRLr(unit, local_port, rval));
    return BCM_E_NONE;
}

int
bcm_tr3_cosq_control_set(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                        bcm_cosq_control_t type, int arg)
{
    switch (type) {
    case bcmCosqControlEgressPool:
    case bcmCosqControlEgressPoolLimitBytes:
    case bcmCosqControlEgressPoolYellowLimitBytes:
    case bcmCosqControlEgressPoolRedLimitBytes:
    case bcmCosqControlEgressPoolLimitEnable:
        return _bcm_tr3_cosq_egr_pool_set(unit, gport, cosq, type, arg);
    case bcmCosqControlEfPropagation:
        return _bcm_tr3_cosq_ef_op(unit, gport, cosq, &arg, _BCM_COSQ_OP_SET);
    case bcmCosqControlCongestionProxy:
        return _bcm_tr3_cosq_qcn_proxy_set(unit, gport, cosq, type, arg);
    default:
        break;
    }

    return BCM_E_UNAVAIL;
}

int
bcm_tr3_cosq_control_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                        bcm_cosq_control_t type, int *arg)
{
    switch (type) {
    case bcmCosqControlEgressPool:
    case bcmCosqControlEgressPoolLimitBytes:
    case bcmCosqControlEgressPoolYellowLimitBytes:
    case bcmCosqControlEgressPoolRedLimitBytes:
    case bcmCosqControlEgressPoolLimitEnable:
        return _bcm_tr3_cosq_egr_pool_get(unit, gport, cosq, type, arg);
    case bcmCosqControlEfPropagation:
        return _bcm_tr3_cosq_ef_op(unit, gport, cosq, arg, _BCM_COSQ_OP_GET);
    case bcmCosqControlPortQueueUcast:
    case bcmCosqControlPortQueueMcast:
        return _bcm_tr3_cosq_port_qnum_get(unit, gport, cosq, type, arg);
    default:
        break;
    }

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_bandwidth_set
 * Purpose:
 *     Configure COS queue bandwidth control
 * Parameters:
 *     unit   - (IN) unit number
 *     gport  - (IN) GPORT identifier
 *     cosq   - (IN) COS queue to configure, -1 for all COS queues
 *     kbits_sec_min - (IN) minimum bandwidth, kbits/sec
 *     kbits_sec_max - (IN) maximum bandwidth, kbits/sec
 *     flags  - (IN) BCM_COSQ_BW_*
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_bandwidth_set(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 kbits_sec_min,
                                uint32 kbits_sec_max, uint32 flags)
{
    int i, start_cos, end_cos, local_port;
    _bcm_tr3_cosq_node_t *node;
    uint32 burst_min, burst_max;

    if (cosq <= -1) {
        if (BCM_GPORT_IS_SET(gport)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, 
                                        NULL, NULL, &node));
            start_cos = 0;
            end_cos = node->numq - 1;
        } else {
            start_cos = 0;
            end_cos = _TR3_NUM_COS(unit) - 1;
        }
    } else {
        start_cos = end_cos = cosq;
    }
    
    BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_localport_resolve(unit, gport, &local_port));

    burst_min = (kbits_sec_min > 0) ?
          _bcm_td_default_burst_size(unit, local_port, kbits_sec_min) : 0;

    burst_max = (kbits_sec_max > 0) ?
          _bcm_td_default_burst_size(unit, local_port, kbits_sec_max) : 0;

    for (i = start_cos; i <= end_cos; i++) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_bucket_set(unit, gport, i, kbits_sec_min,
                         kbits_sec_max, burst_min, burst_max, flags));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_bandwidth_get
 * Purpose:
 *     Get COS queue bandwidth control configuration
 * Parameters:
 *     unit   - (IN) unit number
 *     gport  - (IN) GPORT identifier
 *     cosq   - (IN) COS queue to get, -1 for any COS queue
 *     kbits_sec_min - (OUT) minimum bandwidth, kbits/sec
 *     kbits_sec_max - (OUT) maximum bandwidth, kbits/sec
 *     flags  - (OUT) BCM_COSQ_BW_*
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_bandwidth_get(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 *kbits_sec_min,
                                uint32 *kbits_sec_max, uint32 *flags)
{
    uint32 kbits_sec_burst;

    if (cosq == -1) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_bucket_get(unit, gport, cosq,
                                 kbits_sec_min, kbits_sec_max,
                                 &kbits_sec_burst, &kbits_sec_burst, flags));
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_bandwidth_burst_set
 * Purpose:
 *     Configure COS queue bandwidth burst setting
 * Parameters:
 *      unit        - (IN) unit number
 *      gport       - (IN) GPORT identifier
 *      cosq        - (IN) COS queue to configure, -1 for all COS queues
 *      kbits_burst - (IN) maximum burst, kbits
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_bandwidth_burst_set(int unit, bcm_gport_t gport,
                                      bcm_cos_queue_t cosq,
                                      uint32 kbits_burst_min,
                                      uint32 kbits_burst_max)
{
    int i, start_cos, end_cos;
    uint32 kbits_sec_min, kbits_sec_max, kbits_sec_burst, flags;
    _bcm_tr3_cosq_node_t *node;

    if (cosq < -1) {
        return BCM_E_PARAM;
    }

    if (cosq == -1) {
        if (BCM_GPORT_IS_SET(gport)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_cosq_node_get(unit, gport, 0, NULL, 
                                        NULL, NULL, &node));
            start_cos = 0;
            end_cos = node->numq - 1;
        } else {
            start_cos = 0;
            end_cos = _TR3_NUM_COS(unit) - 1;
        }
    } else {
        start_cos = end_cos = cosq;
    }

    for (i = start_cos; i <= end_cos; i++) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_bucket_get(unit, gport, i, &kbits_sec_min,
                                     &kbits_sec_max, &kbits_sec_burst,
                                     &kbits_sec_burst, &flags));
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_bucket_set(unit, gport, i, kbits_sec_min,
                                     kbits_sec_max, kbits_burst_min, 
                                     kbits_burst_max, flags));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_bandwidth_burst_get
 * Purpose:
 *     Get COS queue bandwidth burst setting
 * Parameters:
 *     unit        - (IN) unit number
 *     gport       - (IN) GPORT identifier
 *     cosq        - (IN) COS queue to get, -1 for any queue
 *     kbits_burst - (OUT) maximum burst, kbits
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_bandwidth_burst_get(int unit, bcm_gport_t gport,
                                       bcm_cos_queue_t cosq,
                                       uint32 *kbits_burst_min,
                                       uint32 *kbits_burst_max)
{
    uint32 kbits_sec_min, kbits_sec_max, flags;

    if (cosq < -1) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_bucket_get(unit, gport, cosq == -1 ? 0 : cosq,
                                 &kbits_sec_min, &kbits_sec_max, kbits_burst_min,
                                 kbits_burst_max, &flags));
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_sched_set
 * Purpose:
 *     Configure COS queue scheduler setting
 * Parameters:
 *      unit   - (IN) unit number
 *      gport  - (IN) GPORT identifier
 *      cosq   - (IN) COS queue to configure, -1 for all COS queues
 *      mode   - (IN) Scheduling mode, one of BCM_COSQ_xxx
 *      weight - (IN) queue weight
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_sched_set(int unit, bcm_gport_t gport,
                            bcm_cos_queue_t cosq, int mode, int weight)
{
    int rv, numq, i, count;

    if (cosq == -1) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_index_resolve(unit, gport, cosq,
                                        _BCM_TR3_COSQ_INDEX_STYLE_SCHEDULER,
                                        NULL, NULL, &numq));
        cosq = 0;
    } else {
        numq = 1;
    }

    count = 0;
    for (i = 0; i < numq; i++) {
        rv = _bcm_tr3_cosq_sched_set(unit, gport, cosq + i, mode, weight);
        if (rv == BCM_E_NOT_FOUND) {
            continue;
        } else if (BCM_FAILURE(rv)) {
            return rv;
        } else {
            count++;
        }
    }

    return count > 0 ? BCM_E_NONE : BCM_E_NOT_FOUND;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_sched_get
 * Purpose:
 *     Get COS queue scheduler setting
 * Parameters:
 *     unit   - (IN) unit number
 *     gport  - (IN) GPORT identifier
 *     cosq   - (IN) COS queue to get, -1 for any queue
 *     mode   - (OUT) Scheduling mode, one of BCM_COSQ_xxx
 *     weight - (OUT) queue weight
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_sched_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                            int *mode, int *weight)
{
    int rv, numq, i;

    if (cosq == -1) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_index_resolve(unit, gport, 0,
                                        _BCM_TR3_COSQ_INDEX_STYLE_SCHEDULER,
                                        NULL, NULL, &numq));
        cosq = 0;
    } else {
        numq = 1;
    }

    for (i = 0; i < numq; i++) {
        rv = _bcm_tr3_cosq_sched_get(unit, gport, cosq + i, mode, weight);
        if (rv == BCM_E_NONE) {
            continue;
        } else {
            return rv;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_discard_set
 * Purpose:
 *     Configure port WRED setting
 * Parameters:
 *     unit    - (IN) unit number
 *     port    - (IN) GPORT identifier
 *     cosq    - (IN) COS queue to configure, -1 for all COS queues
 *     discard - (IN) discard settings
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_discard_set(int unit, bcm_gport_t gport,
                               bcm_cos_queue_t cosq,
                               bcm_cosq_gport_discard_t *discard)
{
    int numq, i;
    uint32 min_thresh, max_thresh;
    int cell_size, cell_field_max, flags = 0;

    if (discard == NULL ||
        discard->gain < 0 || discard->gain > 15 ||
        discard->drop_probability < 0 || discard->drop_probability > 100) {
        return BCM_E_PARAM;
    }

    cell_size = _BCM_TR3_BYTES_PER_CELL;
    cell_field_max = TR3_CELL_FIELD_MAX;

    min_thresh = discard->min_thresh;
    max_thresh = discard->max_thresh;
    if (discard->flags & BCM_COSQ_DISCARD_BYTES) {
        /* Convert bytes to cells */
        min_thresh += (cell_size - 1);
        min_thresh /= cell_size;

        max_thresh += (cell_size - 1);
        max_thresh /= cell_size;

        if ((min_thresh > cell_field_max) ||
            (max_thresh > cell_field_max)) {
            return BCM_E_PARAM;
        }
    } else {
        /* Packet mode not supported */
        return BCM_E_PARAM;
    }

    if (cosq == -1) {
        cosq = 0;
        flags = BCM_COSQ_DISCARD_PORT;
    }
    numq = 1;

    for (i = 0; i < numq; i++) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_cosq_wred_set(unit, gport, cosq + i, discard->flags,
                                   min_thresh, max_thresh,
                                   discard->drop_probability, discard->gain,
                                   FALSE, flags));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_tr3_cosq_gport_discard_get
 * Purpose:
 *     Get port WRED setting
 * Parameters:
 *     unit    - (IN) unit number
 *     port    - (IN) GPORT identifier
 *     cosq    - (IN) COS queue to get, -1 for any queue
 *     discard - (OUT) discard settings
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_cosq_gport_discard_get(int unit, bcm_gport_t gport,
                              bcm_cos_queue_t cosq,
                              bcm_cosq_gport_discard_t *discard)
{
    uint32 min_thresh, max_thresh;

    if (discard == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_wred_get(unit, gport, cosq == -1 ? 0 : cosq,
                               &discard->flags, &min_thresh, &max_thresh,
                               &discard->drop_probability, &discard->gain, 0));

    /* Convert number of cells to number of bytes */
    discard->min_thresh = min_thresh * _BCM_TR3_BYTES_PER_CELL;
    discard->max_thresh = max_thresh * _BCM_TR3_BYTES_PER_CELL;

    return BCM_E_NONE;
}

int
_bcm_tr3_cosq_pfc_class_resolve(bcm_switch_control_t sctype, int *type,
                               int *class)
{
    switch (sctype) {
    case bcmSwitchPFCClass7Queue:
    case bcmSwitchPFCClass6Queue:
    case bcmSwitchPFCClass5Queue:
    case bcmSwitchPFCClass4Queue:
    case bcmSwitchPFCClass3Queue:
    case bcmSwitchPFCClass2Queue:
    case bcmSwitchPFCClass1Queue:
    case bcmSwitchPFCClass0Queue:
        *type = _BCM_TR3_COSQ_TYPE_UCAST;
        break;
    case bcmSwitchPFCClass7McastQueue:
    case bcmSwitchPFCClass6McastQueue:
    case bcmSwitchPFCClass5McastQueue:
    case bcmSwitchPFCClass4McastQueue:
    case bcmSwitchPFCClass3McastQueue:
    case bcmSwitchPFCClass2McastQueue:
    case bcmSwitchPFCClass1McastQueue:
    case bcmSwitchPFCClass0McastQueue:
        *type = _BCM_TR3_COSQ_TYPE_MCAST;
        break;
    case bcmSwitchPFCClass7DestmodQueue:
    case bcmSwitchPFCClass6DestmodQueue:
    case bcmSwitchPFCClass5DestmodQueue:
    case bcmSwitchPFCClass4DestmodQueue:
    case bcmSwitchPFCClass3DestmodQueue:
    case bcmSwitchPFCClass2DestmodQueue:
    case bcmSwitchPFCClass1DestmodQueue:
    case bcmSwitchPFCClass0DestmodQueue:
        *type = _BCM_TR3_COSQ_TYPE_EXT_UCAST;
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (sctype) {
    case bcmSwitchPFCClass7Queue:
    case bcmSwitchPFCClass7McastQueue:
    case bcmSwitchPFCClass7DestmodQueue:
        *class = 7;
        break;
    case bcmSwitchPFCClass6Queue:
    case bcmSwitchPFCClass6McastQueue:
    case bcmSwitchPFCClass6DestmodQueue:
        *class = 6;
        break;
    case bcmSwitchPFCClass5Queue:
    case bcmSwitchPFCClass5McastQueue:
    case bcmSwitchPFCClass5DestmodQueue:
        *class = 5;
        break;
    case bcmSwitchPFCClass4Queue:
    case bcmSwitchPFCClass4McastQueue:
    case bcmSwitchPFCClass4DestmodQueue:
        *class = 4;
        break;
    case bcmSwitchPFCClass3Queue:
    case bcmSwitchPFCClass3McastQueue:
    case bcmSwitchPFCClass3DestmodQueue:
        *class = 3;
        break;
    case bcmSwitchPFCClass2Queue:
    case bcmSwitchPFCClass2McastQueue:
    case bcmSwitchPFCClass2DestmodQueue:
        *class = 2;
        break;
    case bcmSwitchPFCClass1Queue:
    case bcmSwitchPFCClass1McastQueue:
    case bcmSwitchPFCClass1DestmodQueue:
        *class = 1;
        break;
    case bcmSwitchPFCClass0Queue:
    case bcmSwitchPFCClass0McastQueue:
    case bcmSwitchPFCClass0DestmodQueue:
        *class = 0;
        break;
    /*
     * COVERITY
     *
     * This default is unreachable but needed for some compiler. 
     */ 
    /* coverity[dead_error_begin] */
    default:
        return BCM_E_INTERNAL;
    }
    return BCM_E_NONE;
}

int
bcm_tr3_cosq_port_pfc_op(int unit, bcm_port_t port,
                        bcm_switch_control_t sctype, _bcm_cosq_op_t op,
                        bcm_gport_t *gport, int gport_count)
{
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port, resolved_port;
    int type, class = -1, id, index, hw_cosq, hw_cosq1;
    uint32 profile_index, old_profile_index;
    uint64 rval64[16], tmp, *rval64s[1], rval, index_map;
    uint32 fval, cos_bmp, tmp32;
    int hw_index, hw_index1;
    _bcm_tr3_mmu_info_t *mmu_info;
    _bcm_tr3_cosq_port_info_t *port_info;
    soc_reg_t   reg;
    int phy_port, mmu_port, lvl;
    soc_info_t *si;
    static const soc_reg_t llfc_cfgr[] = {
        PORT_PFC_CFG0r, PORT_PFC_CFG1r
    };

    if (gport_count < 0) {
        return BCM_E_PARAM;
    }
    
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    if (IS_CPU_PORT(unit, local_port)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_pfc_class_resolve(sctype, &type, &class));

    si = &SOC_INFO(unit);
    phy_port = si->port_l2p_mapping[local_port];
    mmu_port = si->port_p2m_mapping[phy_port];

    mmu_info = _bcm_tr3_mmu_info[unit];
    port_info = &mmu_info->port_info[local_port];

    cos_bmp = 0;
    for (index = 0; index < gport_count; index++) {
        hw_index = hw_index1 = -1;
        hw_cosq = hw_cosq1 = -1;
        if (BCM_GPORT_IS_SET(gport[index])) {
            BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_node_get(unit, gport[index], 
                                0, NULL, &resolved_port, &id, &node));
            
            if (!((node->type == _BCM_TR3_NODE_UCAST) ||
                  (node->type == _BCM_TR3_NODE_MCAST) ||
                  (node->type == _BCM_TR3_NODE_SCHEDULER))) {
                return BCM_E_PARAM;
            }

            hw_index = node->hw_index;
            hw_cosq = node->hw_cosq;
            lvl = node->level;
        } else {
            if (_bcm_tr3_cosq_port_has_ets(unit, local_port)) {
                hw_cosq = gport[index];
                node = &mmu_info->queue_node[port_info->uc_base + hw_cosq];
                if ((!node->in_use) || (node->attached_to_input == -1)) {
                    return BCM_E_PARAM;
                }
                hw_index = node->hw_index;
                node = &mmu_info->mc_queue_node[port_info->mc_base + hw_cosq];
                if ((!node->in_use) || (node->attached_to_input == -1)) {
                    return BCM_E_PARAM;
                }
                hw_index1 = node->hw_index;
                hw_cosq1 = hw_cosq + 8;
                lvl = SOC_TR3_NODE_LVL_L2;
            } else {
                lvl = SOC_TR3_NODE_LVL_L1;
                hw_cosq = gport[index];
                BCM_IF_ERROR_RETURN(soc_tr3_sched_hw_index_get(unit, local_port, 
                                    SOC_TR3_NODE_LVL_L1, hw_cosq, &hw_index));
            }
        }
        if ((hw_cosq < 0) || (hw_cosq >= _TR3_NUM_COS(unit))) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(
            _bcm_tr3_map_fc_status_to_node(unit, mmu_port*4, hw_cosq,
                         hw_index, ((op == _BCM_COSQ_OP_CLEAR) ? 0 : 1), lvl));
        cos_bmp |= 1 << hw_cosq;
        if (hw_cosq1 >= 0) {
            BCM_IF_ERROR_RETURN(
                _bcm_tr3_map_fc_status_to_node(unit, mmu_port*4, hw_cosq1,
                         hw_index1, ((op == _BCM_COSQ_OP_CLEAR) ? 0 : 1), lvl));
            cos_bmp |= 1 << hw_cosq1;
        }
    }

    if (op == _BCM_COSQ_OP_CLEAR) {
        cos_bmp = (1 << NUM_COS(unit)) - 1;
        cos_bmp |= (cos_bmp << 8);
    }

    reg = llfc_cfgr[mmu_port/32];
    rval64s[0] = rval64;
    BCM_IF_ERROR_RETURN(soc_reg64_get(unit, reg, 0, 0, &rval));
    if (op == _BCM_COSQ_OP_SET || cos_bmp != 0) {
        index_map = soc_reg64_field_get(unit, reg, rval, PROFILE_INDEXf);
        COMPILER_64_SHR(index_map, ((mmu_port % 32) * 2));
        COMPILER_64_TO_32_LO(tmp32, index_map);
        old_profile_index = (tmp32 & 3)*16;
        BCM_IF_ERROR_RETURN
            (soc_profile_reg_get(unit, _bcm_tr3_llfc_profile[unit],
                                 old_profile_index, 16, rval64s));
        if (op == _BCM_COSQ_OP_SET) {
            soc_reg64_field32_set(unit, PRIO2COS_PROFILEr, &rval64[class],
                                  COS_BMPf, cos_bmp);
        } else if (cos_bmp != 0) {
            fval = soc_reg64_field32_get(unit, PRIO2COS_PROFILEr, rval64[class],
                                         COS_BMPf);
            if (op == _BCM_COSQ_OP_ADD) {
                fval |= cos_bmp;
            } else { /* _BCM_COSQ_OP_DELETE */
                fval &= ~cos_bmp;
            }
            soc_reg64_field32_set(unit, PRIO2COS_PROFILEr, &rval64[class],
                                  COS_BMPf, fval);
        }
        BCM_IF_ERROR_RETURN
            (soc_profile_reg_add(unit, _bcm_tr3_llfc_profile[unit], rval64s,
                                 16, &profile_index));


        index_map = soc_reg64_field_get(unit, reg, rval, PROFILE_INDEXf);

        COMPILER_64_SET(tmp, 0, 3);
        COMPILER_64_SHL(tmp, (mmu_port % 32) * 2);
        COMPILER_64_NOT(tmp);
        COMPILER_64_AND(index_map, tmp);
        COMPILER_64_SET(tmp, 0, profile_index/16);
        COMPILER_64_SHL(tmp, (mmu_port % 32) * 2);
        COMPILER_64_OR(index_map, tmp);
        
        soc_reg64_field_set(unit, reg, &rval, PROFILE_INDEXf,
                            index_map);

        BCM_IF_ERROR_RETURN
            (soc_profile_reg_delete(unit, _bcm_tr3_llfc_profile[unit],
                                    old_profile_index));
    }

    BCM_IF_ERROR_RETURN(soc_reg64_set(unit, reg, 0, 0, rval));

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_port_pfc_get(int unit, bcm_port_t port,
                         bcm_switch_control_t sctype,
                         bcm_gport_t *gport, int gport_count,
                         int *actual_gport_count)
{
    _bcm_tr3_cosq_node_t *node;
    bcm_port_t local_port;
    int type = -1, class = -1, hw_cosq, count = 0, j, inv_mapped, hw_index;
    uint32 profile_index;
    uint64 rval64[16], *rval64s[1], rval, index_map;
    uint32 tmp32, bmp;
    _bcm_tr3_mmu_info_t *mmu_info;
    int phy_port, mmu_port;
    int eindex;
    soc_info_t *si;
    soc_reg_t   reg;
    soc_mem_t   mem;
    static const soc_reg_t llfc_cfgr[] = {
        PORT_PFC_CFG0r, PORT_PFC_CFG1r
    };
    static const soc_field_t self[] = {
        SEL0f, SEL1f, SEL2f, SEL3f
    };
    uint32 map_entry[SOC_MAX_MEM_WORDS];

    if (IS_CPU_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    if (gport == NULL || gport_count <= 0 || actual_gport_count == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_cosq_localport_resolve(unit, port, &local_port));

    BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_pfc_class_resolve(sctype, &type, &class));

    si = &SOC_INFO(unit);
    mmu_info = _bcm_tr3_mmu_info[unit];
    phy_port = si->port_l2p_mapping[local_port];
    mmu_port = si->port_p2m_mapping[phy_port];

    rval64s[0] = rval64;
    reg = llfc_cfgr[mmu_port/32];
    BCM_IF_ERROR_RETURN(soc_reg64_get(unit, reg, 0, 0, &rval));

    index_map = soc_reg64_field_get(unit, reg, rval, PROFILE_INDEXf);
    COMPILER_64_SHR(index_map, ((mmu_port % 32) * 2));
    COMPILER_64_TO_32_LO(tmp32, index_map);
    profile_index = (tmp32 & 3)*16;

    BCM_IF_ERROR_RETURN
        (soc_profile_reg_get(unit, _bcm_tr3_llfc_profile[unit],
                             profile_index, 16, rval64s));

    bmp = soc_reg64_field32_get(unit, PRIO2COS_PROFILEr, rval64[class],
                                COS_BMPf);
    for (hw_cosq = 0; hw_cosq < 16; hw_cosq++) {
        if (!(bmp & (1 << hw_cosq))) {
            continue;
        }
        if (_bcm_tr3_cosq_port_has_ets(unit, local_port)) {
            inv_mapped = 0;
            for (j = _BCM_TR3_NUM_PORT_SCHEDULERS; 
                 j < _BCM_TR3_NUM_TOTAL_SCHEDULERS; j++) {
                node = &mmu_info->sched_node[j];
                if ((!node->in_use) || (node->local_port != local_port) ||
                    (node->hw_cosq != hw_cosq)) {
                    continue;
                }
                
                hw_index = (node->hw_index / 16);
                eindex = (node->hw_index % 16)/4;
                if (node->level == SOC_TR3_NODE_LVL_L0) {
                    mem = MMU_INTFI_FC_MAP_TBL0m;
                } else if (node->level == SOC_TR3_NODE_LVL_L1) {
                    mem = MMU_INTFI_FC_MAP_TBL1m;
                } else {
                    continue;
                }

                BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, 
                        MEM_BLOCK_ALL, hw_index, &map_entry));
                
                if (soc_mem_field32_get(unit, mem, &map_entry, self[eindex])) {
                    inv_mapped = 1;
                    gport[count++] = node->gport;
                    break;
                }
            }

            for (j = 0; inv_mapped==0 && j < _BCM_TR3_NUM_L2_UC_LEAVES; j++) {
                node = &mmu_info->queue_node[j];
                if ((!node->in_use) || (node->local_port != local_port) ||
                    (node->hw_cosq != hw_cosq)) {
                    continue;
                }
                
                mem = MMU_INTFI_FC_MAP_TBL2m;
                hw_index = (node->hw_index / 16);
                eindex = (node->hw_index % 16)/4;
                BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, 
                        MEM_BLOCK_ALL, hw_index, &map_entry));

                if (soc_mem_field32_get(unit, mem, &map_entry, self[eindex])) {
                    inv_mapped = 1;
                    gport[count++] = node->hw_cosq;
                    break;
                }
                break;
            }
        } else {
            gport[count++] = hw_cosq;
        }
        if (count == gport_count) {
            break;
        }
    }

    if (count == 0) {
        return BCM_E_NOT_FOUND;
    }
    *actual_gport_count = count;

    return BCM_E_NONE;
}

int
bcm_tr3_cosq_drop_status_enable_set(int unit, bcm_port_t port, int enable)
{
    _bcm_tr3_mmu_info_t *mmu_info;
    soc_info_t *si;
    int base, count, idx;
    uint32 rval;
    _bcm_tr3_cosq_node_t *node;

    if ((mmu_info = _bcm_tr3_mmu_info[unit]) == NULL) {
        return BCM_E_INIT;
    }

    si = &SOC_INFO(unit);

    count = si->port_num_cosq[port];
    for (idx = 0; idx < count; idx++) {
        BCM_IF_ERROR_RETURN
            (READ_OP_QUEUE_CONFIG1_CELLr(unit, port, idx, &rval));
        soc_reg_field_set(unit,OP_QUEUE_CONFIG1_CELLr, &rval,
                          Q_E2E_DS_EN_CELLf, enable ? 1 : 0);
        BCM_IF_ERROR_RETURN
            (WRITE_OP_QUEUE_CONFIG1_CELLr(unit, port, idx, rval));
    }

    count = si->port_num_uc_cosq[port];
    base = si->port_uc_cosq_base[port];
    for (idx = 0; idx < count; idx++) {
        BCM_IF_ERROR_RETURN
            (soc_mem_field32_modify(unit, MMU_THDO_Q_TO_QGRP_MAPm, 
                                    base + idx, Q_E2E_DS_EN_CELLf, !!enable));
    }

    for (idx = mmu_info->num_base_queues; 
                                idx < _BCM_TR3_NUM_L2_UC_LEAVES; idx++) {
        node = &mmu_info->queue_node[idx];
        if ((node->in_use == FALSE) || (node->local_port != port)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, MMU_THDO_Q_TO_QGRP_MAPm, 
                                        idx, Q_E2E_DS_EN_CELLf, !!enable));
        }
    }

    BCM_IF_ERROR_RETURN(READ_OP_THR_CONFIGr(unit, &rval));
    soc_reg_field_set(unit, OP_THR_CONFIGr, &rval, EARLY_E2E_SELECTf,
                      enable ? 1 : 0);
    BCM_IF_ERROR_RETURN(WRITE_OP_THR_CONFIGr(unit, rval));

    return BCM_E_NONE;
}



int bcm_tr3_cosq_bst_profile_set(int unit, 
                                 bcm_gport_t port, 
                                 bcm_cos_queue_t cosq, 
                                 bcm_bst_stat_id_t bid,
                                 bcm_cosq_bst_profile_t *profile)
{
    BCM_IF_ERROR_RETURN(_bcm_bst_cmn_profile_set(unit, port, cosq, bid, profile));
    return BCM_E_NONE;
}

int bcm_tr3_cosq_bst_profile_get(int unit, 
                                 bcm_gport_t port, 
                                 bcm_cos_queue_t cosq, 
                                 bcm_bst_stat_id_t bid,
                                 bcm_cosq_bst_profile_t *profile)
{
    BCM_IF_ERROR_RETURN(_bcm_bst_cmn_profile_get(unit, port, cosq, bid, profile));
    return BCM_E_NONE;
}

int bcm_tr3_cosq_bst_stat_get(int unit, 
                              bcm_gport_t port, 
                              bcm_cos_queue_t cosq, 
                              bcm_bst_stat_id_t bid,
                              uint32 options,
                              uint64 *value)
{
    return _bcm_bst_cmn_stat_get(unit, port, cosq, bid, options, value);
}

int bcm_tr3_cosq_bst_stat_multi_get(int unit,
                                bcm_gport_t port,
                                bcm_cos_queue_t cosq,
                                uint32 options,
                                int max_values,
                                bcm_bst_stat_id_t *id_list,
                                uint64 *values)
{
    return _bcm_bst_cmn_stat_multi_get(unit, port, cosq, options, max_values, 
                                    id_list, values);
}

int bcm_tr3_cosq_bst_stat_clear(int unit, 
                            bcm_gport_t port, 
                            bcm_cos_queue_t cosq, 
                            bcm_bst_stat_id_t bid)
{
    return _bcm_bst_cmn_stat_clear(unit, port, cosq, bid);
}

int bcm_tr3_cosq_bst_stat_sync(int unit, bcm_bst_stat_id_t bid)
{
    return _bcm_bst_cmn_stat_sync(unit, bid);
}

/*
 * Function:
 *      bcm_tr3_cosq_field_classifier_id_create
 * Purpose:
 *      Create an endpoint.
 * Parameters: 
 *      unit          - (IN) Unit number.
 *      classifier    - (IN) Classifier attributes
 *      classifier_id - (OUT) Classifier ID
 * Returns:
 *      BCM_E_xxx
 */     
int
bcm_tr3_cosq_field_classifier_id_create(
    int unit,
    bcm_cosq_classifier_t *classifier,
    int *classifier_id)
{
    int rv;
    int ref_count = 0;
    int ent_per_set = 16;
    int i;

    if (NULL == classifier || NULL == classifier_id) {
        return BCM_E_PARAM;
    }

    for (i = 0; i < SOC_MEM_SIZE(unit, IFP_COS_MAPm);) {
        rv = soc_profile_mem_ref_count_get
                (unit, _bcm_tr3_ifp_cos_map_profile[unit], i, &ref_count);
        if (SOC_E_NONE != rv) {
            return (rv);
        }
        if (0 == ref_count) {
            break;
        }
        i = i + ent_per_set;
    }

    if (i >= SOC_MEM_SIZE(unit, IFP_COS_MAPm) && ref_count != 0) {
        *classifier_id = 0;
        return (BCM_E_RESOURCE);
    }

    _BCM_COSQ_CLASSIFIER_FIELD_SET(*classifier_id, (i / ent_per_set));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_tr3_cosq_field_classifier_get
 * Purpose:
 *      Get classifier type information.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      classifier_id - (IN) Classifier ID
 *      classifier    - (OUT) Classifier info
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_field_classifier_get(
    int unit,
    int classifier_id,
    bcm_cosq_classifier_t *classifier)
{
    sal_memset(classifier, 0, sizeof(bcm_cosq_classifier_t));

    if (_BCM_COSQ_CLASSIFIER_IS_FIELD(classifier_id)) {
        classifier->flags |= BCM_COSQ_CLASSIFIER_FIELD;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_cosq_service_classifier_id_create
 * Purpose:
 *      Create an endpoint.
 * Parameters: 
 *      unit          - (IN) Unit number.
 *      classifier    - (IN) Classifier attributes
 *      classifier_id - (OUT) Classifier ID
 * Returns:
 *      BCM_E_xxx
 */     
int
bcm_tr3_cosq_service_classifier_id_create(
    int unit,
    bcm_cosq_classifier_t *classifier,
    int *classifier_id)
{
    if (NULL == classifier || NULL == classifier_id) {
        return BCM_E_PARAM;
    }
    /* XXX check classifier->vlan for valid vlan/vfi */
    _BCM_COSQ_CLASSIFIER_SERVICE_SET(*classifier_id,classifier->vlan);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_cosq_service_classifier_id_destroy
 * Purpose:
 *      free resource associated with this service classifier id.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      classifier_id - (IN) Classifier ID
 * Returns:
 *      BCM_E_xxx
 */     
int
bcm_tr3_cosq_service_classifier_id_destroy(
    int unit,
    int classifier_id)
{
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_cosq_field_classifier_map_set
 * Purpose:
 *      Set internal priority to ingress field processor CoS queue
 *      override mapping.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      classifier_id  - (IN) Classifier ID
 *      count          - (IN) Number of elements in priority_array and
 *                            cosq_array
 *      priority_array - (IN) Array of internal priorities
 *      cosq_array     - (IN) Array of COS queues
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_field_classifier_map_set(int unit,
                                      int classifier_id,
                                      int count,
                                      bcm_cos_t *priority_array,
                                      bcm_cos_queue_t *cosq_array)
{
    int rv;
    int i;
    int max_entries = 16;
    uint32 index;
    void *entries[1];
    ifp_cos_map_entry_t *ent_buf;

    /* Input parameter check. */
    if (!_BCM_COSQ_CLASSIFIER_IS_FIELD(classifier_id)) {
        return (BCM_E_PARAM);
    }

    if (count > max_entries) {
        return (BCM_E_PARAM);
    }

    ent_buf = sal_alloc(sizeof(ifp_cos_map_entry_t) * max_entries,
                        "IFP_COS_MAP entry");
    if (ent_buf == NULL) {
        return (BCM_E_MEMORY);
    }

    sal_memset(ent_buf, 0, sizeof(ifp_cos_map_entry_t) * max_entries);
    entries[0] = ent_buf;
    
    for (i = 0; i < count; i++) {
        if (priority_array[i] < max_entries) {
            soc_mem_field32_set(unit, IFP_COS_MAPm, &ent_buf[priority_array[i]],
                                IFP_COSf, cosq_array[i]);
        }
    }
    
    rv = soc_profile_mem_add(unit, _bcm_tr3_ifp_cos_map_profile[unit],
                             entries, max_entries, &index);
    sal_free(ent_buf);
    if (rv != SOC_E_NONE) {
        return rv;
    }
    return (rv);
}

/*
 * Function:
 *      bcm_tr3_cosq_field_classifier_map_get
 * Purpose:
 *      Get internal priority to ingress field processor CoS queue
 *      override mapping information.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      classifier_id  - (IN) Classifier ID
 *      array_max      - (IN) Size of priority_array and cosq_array
 *      priority_array - (IN) Array of internal priorities
 *      cosq_array     - (OUT) Array of COS queues
 *      array_count    - (OUT) Size of cosq_array
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_field_classifier_map_get(int unit,
                                      int classifier_id,
                                      int array_max,
                                      bcm_cos_t *priority_array,
                                      bcm_cos_queue_t *cosq_array,
                                      int *array_count)
{
    int rv;
    int i;
    int ent_per_set = 16;
    uint32 index;
    void *entries[1];
    ifp_cos_map_entry_t *ent_buf;

    /* Input parameter check. */
    if (NULL == priority_array || NULL == cosq_array || NULL == array_count) {
        return (BCM_E_PARAM);
    }

    /* Allocate entry buffer. */
    ent_buf = sal_alloc(sizeof(ifp_cos_map_entry_t) * ent_per_set,
                        "IFP_COS_MAP entry");
    if (ent_buf == NULL) {
        return (BCM_E_MEMORY);
    }
    sal_memset(ent_buf, 0, sizeof(ifp_cos_map_entry_t) * ent_per_set);
    entries[0] = ent_buf;

    /* Get profile table entry set base index. */
    index = _BCM_COSQ_CLASSIFIER_FIELD_GET(classifier_id);

    /* Read out entries from profile table into allocated buffer. */
    rv = soc_profile_mem_get(unit, _bcm_tr3_ifp_cos_map_profile[unit],
                             index * ent_per_set, ent_per_set, entries);
    if (!(rv == SOC_E_NOT_FOUND || rv == SOC_E_NONE)) {
        sal_free(ent_buf);
        return rv;
    }

    *array_count = array_max > ent_per_set ? ent_per_set : array_max;

    /* Copy values into API OUT parameters. */
    for (i = 0; i < *array_count; i++) {
        if (priority_array[i] >= 16) {
            sal_free(ent_buf);
            return BCM_E_PARAM;
        }
        cosq_array[i] = soc_mem_field32_get(unit, IFP_COS_MAPm,
                                            &ent_buf[priority_array[i]],
                                            IFP_COSf);
    }

    sal_free(ent_buf);
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_tr3_cosq_field_classifier_map_clear
 * Purpose:
 *      Delete an internal priority to ingress field processor CoS queue
 *      override mapping profile set.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      classifier_id - (IN) Classifier ID
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_field_classifier_map_clear(int unit, int classifier_id)
{
    int ent_per_set = 16;
    uint32 index;

    /* Get profile table entry set base index. */
    index = _BCM_COSQ_CLASSIFIER_FIELD_GET(classifier_id);

    /* Delete the profile entries set. */
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_delete(unit,
                                _bcm_tr3_ifp_cos_map_profile[unit],
                                index * ent_per_set));
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_tr3_cosq_field_classifier_id_destroy
 * Purpose:
 *      Free resource associated with this field classifier id.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      classifier_id - (IN) Classifier ID
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_field_classifier_id_destroy(int unit, int classifier_id)
{
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_tr3_cosq_service_map_set
 * Purpose:
 *      Set the mapping from port, classifier, and multiple internal priorities
 *      to multiple COS queues in a queue group.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      port           - (IN) local port ID
 *      classifier_id  - (IN) Classifier ID
 *      queue_group    - (IN) Base queue ID
 *      array_count    - (IN) Number of elements in priority_array and
 *                            cosq_array
 *      priority_array - (IN) Array of internal priorities
 *      cosq_array     - (IN) Array of COS queues
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_service_map_set(
    int unit,
    bcm_port_t port,
    int classifier_id,
    bcm_gport_t queue_group,
    int array_count,
    bcm_cos_t *priority_array,
    bcm_cos_queue_t *cosq_array)
{
    int rv = BCM_E_NONE;
    int i;
    int j;
    service_queue_map_entry_t sqm;
    service_port_map_entry_t *spm;
    service_cos_map_entry_t *scm;
    void *spm_entries[1];
    void *scm_entries[1];
    int local_port,local_id;
    int spm_entries_per_set = 128;
    int scm_entries_per_set = 16;
    int q_offset;
    uint32 spm_idx;
    uint32 scm_idx;
    int spm_old_index = 0;
    int scm_old_index = 0;
    int qptr;
    int numq;
    int vid;
    int valid_entry;
    _bcm_tr3_cosq_node_t *node;

    if (!_BCM_COSQ_CLASSIFIER_IS_SERVICE(classifier_id)) {
        return BCM_E_PARAM;
    }

    /* index above 4K is for VFI */
    vid = _BCM_COSQ_CLASSIFIER_SERVICE_GET(classifier_id);
    
    /* validate the vlan id */
    if (vid < 0 || vid >= SOC_MEM_SIZE(unit, SERVICE_QUEUE_MAPm)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_tr3_cosq_node_get(unit, queue_group, 0, 
               NULL, &local_port, &local_id, &node));
    /* Reading the parent numq so that , it will not exceed 
       max number of inputs of the parent can have*/
    numq = node->parent->numq;

    /* validate the user parameters */
    if (array_count > scm_entries_per_set) {
        return BCM_E_PARAM;
    }

    /* queues allocated for this queue_group should be contiguous.*/
    /* check if user's queue number is valid */
    for (i = 0; i < array_count; i++) {
        if (cosq_array[i] >= numq) {
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, SERVICE_QUEUE_MAPm, MEM_BLOCK_ANY, (int)vid, &sqm));
   
    valid_entry = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm, VALIDf);
    if (!valid_entry) {
        qptr = local_id;
        sal_memset(&sqm,0, sizeof(service_queue_map_entry_t));
    } else {
        qptr = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm, SERVICE_QUEUE_PTRf);
        spm_old_index = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm,
                               SERVICE_PORT_PROFILE_INDEXf);
        scm_old_index = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm,
                               SERVICE_COS_PROFILE_INDEXf);
    }

    /* allocate space for service port map profile */
    spm = sal_alloc(sizeof(service_port_map_entry_t) *
                    spm_entries_per_set,
              "SERVICE_PORT_MAP temp Mem");
    if (spm == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(spm,0,
          sizeof(service_port_map_entry_t) * spm_entries_per_set);
    spm_entries[0] = spm;

    /* allocate space for service cos map profile */
    scm = sal_alloc(sizeof(service_cos_map_entry_t) *
                    scm_entries_per_set,
              "SERVICE_COS_MAP temp Mem");
    if (scm == NULL) {
        sal_free(spm);
        return BCM_E_MEMORY;
    }
    sal_memset(scm,0,
          sizeof(service_cos_map_entry_t) * scm_entries_per_set);
    scm_entries[0] = scm;

    if (valid_entry) {
        /* only one service_cos_map profile is allowed per vlan/vfi. The first one is
         * used. If the subsequent profile doesn't match the existing one, 
         * it is treated as error. Should always use the xxx_multi_set instead of xxx_set.
         */
        rv = soc_profile_mem_get(unit, _bcm_tr3_service_cos_map_profile[unit],
                             scm_old_index * scm_entries_per_set,
                             scm_entries_per_set, scm_entries);
        if (rv != SOC_E_NONE) {
            goto cleanup;
        }
        scm_idx = scm_old_index * scm_entries_per_set;

        for (i = 0; i < array_count; i++) {
            if (cosq_array[i] != soc_mem_field32_get(unit,SERVICE_COS_MAPm,
                               &scm[priority_array[i]],SERVICE_COS_OFFSETf) ) {
                rv = BCM_E_PARAM;
                goto cleanup;
            }
        }

        for (i = 0; i < scm_entries_per_set; i++) {
            for (j = 0; j < array_count; j++) {
                if (i == priority_array[j]) {
                    break;
                }
            }
            /* for priorities not in priority_array, the cosq should be 0 */
            if (j == array_count) {
                if (soc_mem_field32_get(unit,SERVICE_COS_MAPm,&scm[i],SERVICE_COS_OFFSETf)) {
                    rv = BCM_E_PARAM;
                    goto cleanup;
                }
            }
        }
        sal_free(scm);
 
        rv = soc_profile_mem_get(unit, _bcm_tr3_service_port_map_profile[unit],
                             spm_old_index * spm_entries_per_set,
                             spm_entries_per_set, spm_entries);
        if (!(rv == SOC_E_NOT_FOUND || rv == SOC_E_NONE)) {
            sal_free(spm);
            return rv;
        }

        if (local_id < qptr) {
            /* need to adjust the offset in service_port_map.
             * The new base -> qptr, offset added to qptr-local_id
             */
            for (i = 0; i < spm_entries_per_set; i++) {
                q_offset = soc_mem_field32_get(unit,SERVICE_PORT_MAPm,&spm[i],
                            SERVICE_PORT_OFFSETf);
                soc_mem_field32_set(unit,SERVICE_PORT_MAPm,&spm[i],
                            SERVICE_PORT_OFFSETf,
                            (qptr - local_id) + q_offset);
            }
            q_offset = 0;  /* use the new base queue */
        } else {
            q_offset = local_id - qptr;
            /* no change in queue base */
        }
    } else {  /* first time setup */
        q_offset = 0;  /* use the new base queue */

        /* add the cos profile if first time */
        for (i = 0; i < array_count; i++) {
            if (priority_array[i] < scm_entries_per_set) {
                soc_mem_field32_set(unit,SERVICE_COS_MAPm,&scm[priority_array[i]],
                        SERVICE_COS_OFFSETf,
                        cosq_array[i]);
            }
        }
        /* add a new profile */
        rv = soc_profile_mem_add(unit, _bcm_tr3_service_cos_map_profile[unit],
                             scm_entries, scm_entries_per_set,
                             &scm_idx);
        sal_free(scm);
        if (rv != SOC_E_NONE) {
            return rv;
        }
    }

    soc_mem_field32_set(unit, SERVICE_PORT_MAPm,
                      &spm[local_port], SERVICE_PORT_OFFSETf,
                           q_offset);
    /* add a new profile */
    rv = soc_profile_mem_add(unit, _bcm_tr3_service_port_map_profile[unit], spm_entries,
                             spm_entries_per_set,
                             &spm_idx);
    sal_free(spm);
    if (rv != SOC_E_NONE) {
        sal_free(scm);
        return rv;
    }

    /* program the vlan base queue */
    soc_mem_field32_set(unit, SERVICE_QUEUE_MAPm, &sqm,
                           SERVICE_QUEUE_PTRf, qptr);

    /* set the SERVICE_COS_MAP profile index
     * scm_idx: hardware table base index for the set */
    soc_mem_field32_set(unit, SERVICE_QUEUE_MAPm, &sqm,
                SERVICE_COS_PROFILE_INDEXf,
                scm_idx/scm_entries_per_set);

    /* set the SERVICE_PORT_MAP profile index
     * spm_idx: hardware table base index for the set */
    soc_mem_field32_set(unit, SERVICE_QUEUE_MAPm, &sqm,
                SERVICE_PORT_PROFILE_INDEXf,
                spm_idx/spm_entries_per_set);

    if (!valid_entry) {   /* first time */
        /* use port indexed and cos indexed for this base queue */
        soc_mem_field32_set(unit, SERVICE_QUEUE_MAPm, &sqm,
                           SERVICE_QUEUE_MODELf, 3);
        soc_mem_field32_set(unit, SERVICE_QUEUE_MAPm, &sqm,
                           VALIDf, 1);
    } 

    /* Write back updated buffer. */
    rv = soc_mem_write(unit, SERVICE_QUEUE_MAPm, MEM_BLOCK_ALL,
                       (int)vid, &sqm);

    if (valid_entry) {
        /* delete the old spm porfile */
        if ((spm_old_index * spm_entries_per_set) != spm_idx) {
            BCM_IF_ERROR_RETURN
            (soc_profile_mem_delete(unit, _bcm_tr3_service_port_map_profile[unit],
                             spm_old_index * spm_entries_per_set));
        }
    }
    return BCM_E_NONE;

cleanup:
    sal_free(spm);
    sal_free(scm);
    return rv;
}

/*
 * Function:
 *      bcm_tr3_cosq_service_classifier_get
 * Purpose:
 *      Get info about an endpoint.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      classifier_id - (IN) Classifier ID
 *      classifier    - (OUT) Classifier info
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_service_classifier_get(
    int unit,
    int classifier_id,
    bcm_cosq_classifier_t *classifier)
{
    sal_memset(classifier, 0, sizeof(bcm_cosq_classifier_t));
    classifier->flags = BCM_COSQ_CLASSIFIER_VLAN;
    classifier->vlan  = _BCM_COSQ_CLASSIFIER_SERVICE_GET(classifier_id);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_cosq_service_map_get
 * Purpose:
 *      Get the mapping from port, classifier, and multiple internal priorities
 *      to multiple COS queues in a queue group.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      port           - (IN) Local port ID
 *      classifier_id  - (IN) Classifier ID
 *      queue_group    - (OUT) Queue group
 *      array_max      - (IN) Size of priority_array and cosq_array
 *      priority_array - (IN) Array of internal priorities
 *      cosq_array     - (OUT) Array of COS queues
 *      array_count    - (OUT) Size of cosq_array
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_service_map_get(
    int unit,
    bcm_port_t port,
    int classifier_id,
    bcm_gport_t *queue_group,
    int array_max,
    bcm_cos_t *priority_array,
    bcm_cos_queue_t *cosq_array,
    int *array_count)
{
    service_queue_map_entry_t sqm;
    service_port_map_entry_t *spm;
    service_cos_map_entry_t *scm;
    void *spm_entries[1];
    void *scm_entries[1];
    int spm_entries_per_set = 128;
    int scm_entries_per_set = 16;
    int q_offset;
    int spm_index;
    int scm_index;
    int qptr;
    int vid;
    int valid_entry;
    int rv;
    int i;

    vid = _BCM_COSQ_CLASSIFIER_SERVICE_GET(classifier_id);

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, SERVICE_QUEUE_MAPm, MEM_BLOCK_ANY, (int)vid, &sqm));

    valid_entry = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm, VALIDf);
    if (!valid_entry) {
        return BCM_E_CONFIG;  /* service queue is configured */
    }

    qptr = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm, SERVICE_QUEUE_PTRf);
    spm_index = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm,
                               SERVICE_PORT_PROFILE_INDEXf);
    scm_index = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm,
                               SERVICE_COS_PROFILE_INDEXf);

    /* allocate space for service port map profile */
    spm = sal_alloc(sizeof(service_port_map_entry_t) *
                    spm_entries_per_set,
              "SERVICE_PORT_MAP temp Mem");
    if (spm == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(spm,0,
          sizeof(service_port_map_entry_t) * spm_entries_per_set);
    spm_entries[0] = spm;

    /* allocate space for service cos map profile */
    scm = sal_alloc(sizeof(service_cos_map_entry_t) *
                    scm_entries_per_set,
              "SERVICE_COS_MAP temp Mem");
    if (scm == NULL) {
        sal_free(spm);
        return BCM_E_MEMORY;
    }
    sal_memset(scm,0,
          sizeof(service_cos_map_entry_t) * scm_entries_per_set);
    scm_entries[0] = scm;

    rv = soc_profile_mem_get(unit, _bcm_tr3_service_port_map_profile[unit],
                         spm_index * spm_entries_per_set,
                         spm_entries_per_set, spm_entries);
    if (!(rv == SOC_E_NOT_FOUND || rv == SOC_E_NONE)) {
        sal_free(spm);
        sal_free(scm);
        return rv;
    }

    rv = soc_profile_mem_get(unit, _bcm_tr3_service_cos_map_profile[unit],
                         scm_index * scm_entries_per_set,
                         scm_entries_per_set, scm_entries);
    if (!(rv == SOC_E_NOT_FOUND || rv == SOC_E_NONE)) {
        sal_free(spm);
        sal_free(scm);
        return rv;
    }

    q_offset = soc_mem_field32_get(unit, SERVICE_PORT_MAPm,
                      &spm[port], SERVICE_PORT_OFFSETf);
    BCM_GPORT_UCAST_QUEUE_GROUP_SYSQID_SET(*queue_group, port, (q_offset + qptr));

    *array_count = array_max > scm_entries_per_set? scm_entries_per_set :
                   array_max;
    for (i = 0; i < *array_count; i++) {
        if (priority_array[i] >= 16) {
            sal_free(spm);
            sal_free(scm);
            return BCM_E_PARAM;
        }
        cosq_array[i]     = soc_mem_field32_get(unit,SERVICE_COS_MAPm,
                               &scm[priority_array[i]],SERVICE_COS_OFFSETf);
    }
    sal_free(spm);
    sal_free(scm);
    return BCM_E_NONE;      
}

/*
 * Function:
 *      bcm_tr3_cosq_service_map_clear
 * Purpose:
 *      Clear the mapping from port, classifier, and internal priorities
 *      to COS queues in a queue group.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      port          - (IN) Local port ID
 *      classifier_id - (IN) Classifier ID
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_service_map_clear(
    int unit,
    bcm_gport_t port,
    int classifier_id)
{
    service_queue_map_entry_t sqm;
    int spm_entries_per_set = 128;
    int scm_entries_per_set = 16;
    int spm_index;
    int scm_index;
#if 0
    int qptr;
#endif
    int vid;
    int valid_entry;

    vid = _BCM_COSQ_CLASSIFIER_SERVICE_GET(classifier_id);

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, SERVICE_QUEUE_MAPm, MEM_BLOCK_ANY,
                        (int)vid, &sqm));

    valid_entry = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm, VALIDf);
    if (!valid_entry) {
        return BCM_E_NONE;  /* service queue is already cleared  */
    }
#if 0
    qptr = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm, SERVICE_QUEUE_PTRf);
#endif
    spm_index = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm,
                               SERVICE_PORT_PROFILE_INDEXf);
    scm_index = soc_mem_field32_get(unit, SERVICE_QUEUE_MAPm, &sqm,
                               SERVICE_COS_PROFILE_INDEXf);

    /* delete the spm porfile */
    BCM_IF_ERROR_RETURN(soc_profile_mem_delete(unit, 
                         _bcm_tr3_service_port_map_profile[unit],
                         spm_index * spm_entries_per_set));

    /* delete the scm profile */
    BCM_IF_ERROR_RETURN(soc_profile_mem_delete(unit, 
                         _bcm_tr3_service_cos_map_profile[unit],
                         scm_index * scm_entries_per_set));

    sal_memset(&sqm,0, sizeof(service_queue_map_entry_t));

    /* Write back updated buffer. */
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, SERVICE_QUEUE_MAPm, MEM_BLOCK_ALL,
                       (int)vid, &sqm));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td_cosq_cpu_cosq_enable_set
 * Purpose:
 *      To enable/disable Rx of packets on the specified CPU cosq.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      cosq          - (IN) CPU Cosq ID
 *      enable        - (IN) Enable/Disable
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_cpu_cosq_enable_set(
    int unit, 
    bcm_cos_queue_t cosq, 
    int enable)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_td_cosq_cpu_cosq_enable_get
 * Purpose:
 *      To get enable/disable status on Rx of packets on the specified CPU cosq.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      cosq          - (IN) CPU Cosq ID
 *      enable        - (OUT) Enable/Disable
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_tr3_cosq_cpu_cosq_enable_get(
    int unit, 
    bcm_cos_queue_t cosq, 
    int *enable)
{
    return BCM_E_UNAVAIL;
}

int
bcm_tr3_switch_qcn_mac_set(int unit, bcm_switch_control_t type,
                      soc_mem_t mem,soc_field_t field, void* entry,
                      uint32 mac_field)
{
    bcm_mac_t   mac;
    sal_memset(mac, 0, sizeof(bcm_mac_t));
    soc_mem_mac_addr_get(unit, mem, entry,
                        field, mac);
    switch(type) {
        case bcmSwitchCongestionCnmSrcMacNonOui:
             mac[3] = (uint8) (mac_field >> 16 & 0xff);
             mac[4] = (uint8) (mac_field >> 8 & 0xff);
             mac[5] = (uint8) (mac_field & 0xff);
             break;
        case bcmSwitchCongestionCnmSrcMacOui:
             mac[0] = (uint8) (mac_field >> 16 & 0xff);
             mac[1] = (uint8) (mac_field >> 8 & 0xff);
             mac[2] = (uint8) (mac_field & 0xff);
             break;
        default :
             return BCM_E_PARAM;
    }
    soc_mem_mac_addr_set(unit, mem, entry,
                        field, mac);
    return BCM_E_NONE;
}
int
bcm_tr3_switch_qcn_mac_get(int unit, bcm_switch_control_t type,
                      soc_mem_t mem,soc_field_t field, void* entry,
                      uint32 *mac_field)
{
     bcm_mac_t mac;
     soc_mem_mac_addr_get(unit, mem, entry,
                        field, mac);
    switch (type) {
        case bcmSwitchCongestionCnmSrcMacNonOui:
             *mac_field = ((mac[3] << 16) | (mac[4] << 8)  | (mac[5] << 0));
             break;
        case bcmSwitchCongestionCnmSrcMacOui:
             *mac_field = ((mac[0] << 16) | (mac[1] << 8)  | (mac[2] << 0));
             break;
        default :
           return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}
#endif  /* BCM_TRIUMPH3_SUPPORT */

